/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Sample 02_test_scene
 * --------------------
 * Everything from 01_test_events plus a full Scene panel:
 *   • Switch between PerspectiveCamera and OrthographicCamera at runtime
 *   • 1–4 rotating cubes drawn with IRenderDevice (indexed geometry)
 *   • Directional light + up to 4 point lights, all configurable via ImGui
 *   • Each light has enable/disable, color, intensity controls
 *   • Point lights orbit the cubes at adjustable radius and speed
 *
 * What you can test here:
 *   • vnescene PerspectiveCamera vs OrthographicCamera — parallel lines prove ortho
 *   • SceneState::addLight / clearLights lifecycle
 *   • DirectionalLight and PointLight APIs (setColor, setIntensity, setPosition)
 *   • IRenderDevice::createIndexBuffer + drawIndexed
 *   • Basic Blinn-Phong lighting in GLSL validates light→GPU data flow
 *
 * ImGui Settings panel sections:
 *   [Camera]      type switcher, FOV/near/far (perspective) or half-extents (ortho)
 *   [Cubes]       count (1-4), rotation speed
 *   [Lights]      directional enable/dir/color/intensity,
 *                 add/remove point lights (max 4), per-light color/intensity/orbit
 *
 * Libraries exercised: vne::testbed, vne::scene, vne::events,
 *                      vne::interaction (optional)
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"
#include "vertexnova/testbed/render_device.h"

#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/light/directional_light.h"
#include "vertexnova/scene/light/point_light.h"
#include "vertexnova/scene/scene_state.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#include "../common/base_scene_layer.h"

#include <array>
#include <cmath>
#include <memory>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// Cube geometry — 24 vertices (unique normals per face), 36 indices
// Layout: vec3 pos, vec3 normal, vec3 color
// ---------------------------------------------------------------------------
// clang-format off
static const float kCubeVerts[] = {
    // +Z face (blue-ish)
    -0.5f,-0.5f, 0.5f,  0,0,1,  0.5f,0.5f,1.0f,
     0.5f,-0.5f, 0.5f,  0,0,1,  0.5f,0.5f,1.0f,
     0.5f, 0.5f, 0.5f,  0,0,1,  0.5f,0.5f,1.0f,
    -0.5f, 0.5f, 0.5f,  0,0,1,  0.5f,0.5f,1.0f,
    // -Z face (dark blue)
     0.5f,-0.5f,-0.5f,  0,0,-1, 0.2f,0.2f,0.7f,
    -0.5f,-0.5f,-0.5f,  0,0,-1, 0.2f,0.2f,0.7f,
    -0.5f, 0.5f,-0.5f,  0,0,-1, 0.2f,0.2f,0.7f,
     0.5f, 0.5f,-0.5f,  0,0,-1, 0.2f,0.2f,0.7f,
    // +X face (red-ish)
     0.5f,-0.5f, 0.5f,  1,0,0,  1.0f,0.4f,0.4f,
     0.5f,-0.5f,-0.5f,  1,0,0,  1.0f,0.4f,0.4f,
     0.5f, 0.5f,-0.5f,  1,0,0,  1.0f,0.4f,0.4f,
     0.5f, 0.5f, 0.5f,  1,0,0,  1.0f,0.4f,0.4f,
    // -X face (dark red)
    -0.5f,-0.5f,-0.5f, -1,0,0,  0.6f,0.2f,0.2f,
    -0.5f,-0.5f, 0.5f, -1,0,0,  0.6f,0.2f,0.2f,
    -0.5f, 0.5f, 0.5f, -1,0,0,  0.6f,0.2f,0.2f,
    -0.5f, 0.5f,-0.5f, -1,0,0,  0.6f,0.2f,0.2f,
    // +Y face (green-ish)
    -0.5f, 0.5f, 0.5f,  0,1,0,  0.4f,1.0f,0.4f,
     0.5f, 0.5f, 0.5f,  0,1,0,  0.4f,1.0f,0.4f,
     0.5f, 0.5f,-0.5f,  0,1,0,  0.4f,1.0f,0.4f,
    -0.5f, 0.5f,-0.5f,  0,1,0,  0.4f,1.0f,0.4f,
    // -Y face (dark green)
    -0.5f,-0.5f,-0.5f,  0,-1,0, 0.2f,0.5f,0.2f,
     0.5f,-0.5f,-0.5f,  0,-1,0, 0.2f,0.5f,0.2f,
     0.5f,-0.5f, 0.5f,  0,-1,0, 0.2f,0.5f,0.2f,
    -0.5f,-0.5f, 0.5f,  0,-1,0, 0.2f,0.5f,0.2f,
};

static const uint32_t kCubeIdx[] = {
     0, 1, 2,  2, 3, 0,   // +Z
     4, 5, 6,  6, 7, 4,   // -Z
     8, 9,10, 10,11, 8,   // +X
    12,13,14, 14,15,12,   // -X
    16,17,18, 18,19,16,   // +Y
    20,21,22, 22,23,20,   // -Y
};
// clang-format on

static constexpr uint32_t kCubeIdxCount = 36u;

// Vertex shader: Blinn-Phong, one directional + up to 4 point lights
#if defined(VNE_TESTBED_OPENGL)
static const char* kSceneVert = R"(
#version 410 core
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;
uniform mat4 u_MVP;
uniform mat4 u_Model;
out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;
void main() {
    vec4 wp = u_Model * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal   = normalize(mat3(u_Model) * aNormal);
    vColor    = aColor;
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
)";
static const char* kSceneFrag = R"(
#version 410 core
in  vec3 vWorldPos;
in  vec3 vNormal;
in  vec3 vColor;
out vec4 FragColor;

uniform vec3  u_DirLightDir;
uniform vec3  u_DirLightColor;
uniform float u_DirLightIntensity;
uniform int   u_DirLightEnabled;

uniform int   u_NumPointLights;
uniform vec3  u_PtLightPos[4];
uniform vec3  u_PtLightColor[4];
uniform float u_PtLightIntensity[4];
uniform float u_PtLightRange[4];
uniform int   u_PtLightEnabled[4];

uniform vec3  u_CamPos;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(u_CamPos - vWorldPos);
    vec3 lighting = vec3(0.08);  // ambient

    // Directional
    if (u_DirLightEnabled != 0) {
        vec3 L = normalize(-u_DirLightDir);
        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        lighting += u_DirLightColor * u_DirLightIntensity * (diff * 0.8 + spec * 0.4);
    }

    // Point lights
    for (int i = 0; i < u_NumPointLights && i < 4; ++i) {
        if (u_PtLightEnabled[i] == 0) continue;
        vec3  toLight = u_PtLightPos[i] - vWorldPos;
        float dist    = length(toLight);
        float range   = max(u_PtLightRange[i], 0.001);
        float atten   = clamp(1.0 - (dist / range), 0.0, 1.0);
        atten *= atten;
        vec3 L = normalize(toLight);
        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        lighting += u_PtLightColor[i] * u_PtLightIntensity[i] * atten
                    * (diff * 0.8 + spec * 0.3);
    }

    FragColor = vec4(vColor * lighting, 1.0);
}
)";
#else
static const char* kSceneVert = R"(
#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;
uniform mat4 u_MVP;
uniform mat4 u_Model;
out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;
void main() {
    vec4 wp = u_Model * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal   = normalize(mat3(u_Model) * aNormal);
    vColor    = aColor;
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
)";
static const char* kSceneFrag = R"(
#version 300 es
precision mediump float;
in  vec3 vWorldPos;
in  vec3 vNormal;
in  vec3 vColor;
out vec4 FragColor;
uniform vec3  u_DirLightDir;
uniform vec3  u_DirLightColor;
uniform float u_DirLightIntensity;
uniform int   u_DirLightEnabled;
uniform int   u_NumPointLights;
uniform vec3  u_PtLightPos[4];
uniform vec3  u_PtLightColor[4];
uniform float u_PtLightIntensity[4];
uniform float u_PtLightRange[4];
uniform int   u_PtLightEnabled[4];
uniform vec3  u_CamPos;
void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(u_CamPos - vWorldPos);
    vec3 lighting = vec3(0.08);
    if (u_DirLightEnabled != 0) {
        vec3 L = normalize(-u_DirLightDir);
        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        lighting += u_DirLightColor * u_DirLightIntensity * (diff * 0.8 + spec * 0.4);
    }
    for (int i = 0; i < u_NumPointLights && i < 4; ++i) {
        if (u_PtLightEnabled[i] == 0) continue;
        vec3  toLight = u_PtLightPos[i] - vWorldPos;
        float dist    = length(toLight);
        float range   = max(u_PtLightRange[i], 0.001);
        float atten   = clamp(1.0 - (dist / range), 0.0, 1.0);
        atten *= atten;
        vec3 L = normalize(toLight);
        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        lighting += u_PtLightColor[i] * u_PtLightIntensity[i] * atten
                    * (diff * 0.8 + spec * 0.3);
    }
    FragColor = vec4(vColor * lighting, 1.0);
}
)";
#endif

// ---------------------------------------------------------------------------
// SceneTestLayer — camera management + cube rendering + lights
// ---------------------------------------------------------------------------

struct PointLightEntry {
    std::shared_ptr<vne::scene::PointLight> light;
    float orbit_radius{3.0f};
    float orbit_speed{1.0f};
    float orbit_angle{0.0f};
    float color[3]{1.0f, 0.8f, 0.4f};
    float intensity{2.0f};
    float range{8.0f};
    bool enabled{true};
};

class SceneTestLayer : public vne::testbed::ILayer {
   public:
    static constexpr int kMaxViewports = 4;

    SceneTestLayer()
        : vne::testbed::ILayer("SceneTestLayer") {}

    // -----------------------------------------------------------------------
    void onAttach(vne::testbed::AppContext& ctx) override {
        device_ = ctx.device;
        debug_draw_ = ctx.debugDraw;

        buildCamera(1280, 720);
        buildGeometry();
        buildLights();
    }

    void onDetach() override {
        if (device_) {
            if (pipeline_.isValid())
                device_->destroy(pipeline_);
            if (shader_.isValid())
                device_->destroy(shader_);
            if (ibo_.isValid())
                device_->destroy(ibo_);
            if (vbo_.isValid())
                device_->destroy(vbo_);
        }
        scene_state_.clearLights();
        scene_state_.setActiveCamera(nullptr);
        cameras_persp_.clear();
        cameras_ortho_.clear();
        debug_draw_ = nullptr;
        device_ = nullptr;
    }

    void onUpdate(float dt) override {
        cube_angle_ += cube_rotation_speed_ * dt;

        // Orbit point lights
        for (auto& entry : point_lights_) {
            entry.orbit_angle += entry.orbit_speed * dt;
            const float r = entry.orbit_radius;
            entry.light->setPosition({r * std::cos(entry.orbit_angle), 1.5f, r * std::sin(entry.orbit_angle)});
        }

        for (auto& cam : cameras_persp_) {
            if (cam)
                cam->updateMatrices();
        }
        for (auto& cam : cameras_ortho_) {
            if (cam)
                cam->updateMatrices();
        }
    }

    void onRender(const vne::testbed::RenderContext& ctx) override {
        if (!device_ || !shader_.isValid())
            return;

        const int vp_idx = (ctx.active_viewport_index >= 0 && ctx.active_viewport_index < kMaxViewports)
                               ? ctx.active_viewport_index
                               : 0;

        // Aspect / resize
        if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
            updateCameraAspect(ctx.frame_info.width, ctx.frame_info.height);
        }

        const auto vp = activeVP(vp_idx);
        const auto cam_pos = activePosition(vp_idx);

        // Draw grid + axes via debug draw
        if (debug_draw_) {
            debug_draw_->setViewProjectionMatrix(vp);
            drawGrid();
            drawAxes();

            // Draw point-light positions as small crosses
            for (const auto& e : point_lights_) {
                if (!e.enabled)
                    continue;
                const auto p = e.light->getPosition();
                const float s = 0.15f;
                const vne::math::Vec3f lc{e.color[0], e.color[1], e.color[2]};
                debug_draw_->line({p.x() - s, p.y(), p.z()}, {p.x() + s, p.y(), p.z()}, lc);
                debug_draw_->line({p.x(), p.y() - s, p.z()}, {p.x(), p.y() + s, p.z()}, lc);
                debug_draw_->line({p.x(), p.y(), p.z() - s}, {p.x(), p.y(), p.z() + s}, lc);
            }
            debug_draw_->flush();
        }

        // Upload lighting uniforms
        const auto& dl = dir_light_;
        device_->setInt(shader_, "u_DirLightEnabled", dl->isEnabled() ? 1 : 0);
        device_->setVec3(shader_, "u_DirLightDir", dl->getDirection());
        device_->setVec3(shader_, "u_DirLightColor", dl->getColor());
        device_->setFloat(shader_, "u_DirLightIntensity", dl->getIntensity());

        const int npt = static_cast<int>(point_lights_.size());
        device_->setInt(shader_, "u_NumPointLights", npt);
        for (int i = 0; i < npt && i < 4; ++i) {
            const auto& e = point_lights_[static_cast<std::size_t>(i)];
            const std::string idx = "[" + std::to_string(i) + "]";
            device_->setVec3(shader_, ("u_PtLightPos" + idx).c_str(), e.light->getPosition());
            device_->setVec3(shader_, ("u_PtLightColor" + idx).c_str(), e.light->getColor());
            device_->setFloat(shader_, ("u_PtLightIntensity" + idx).c_str(), e.light->getIntensity());
            device_->setFloat(shader_, ("u_PtLightRange" + idx).c_str(), e.light->getRange());
            device_->setInt(shader_, ("u_PtLightEnabled" + idx).c_str(), e.enabled ? 1 : 0);
        }
        device_->setVec3(shader_, "u_CamPos", cam_pos);

        // Draw cubes
        static const float kOffsets[4][3] = {
            {0.f, 0.5f, 0.f},
            {2.5f, 0.5f, 0.f},
            {-2.5f, 0.5f, 0.f},
            {0.f, 0.5f, 2.5f},
        };
        for (int i = 0; i < cube_count_; ++i) {
            const float angle = cube_angle_ + static_cast<float>(i) * 1.0472f;  // 60°
            const float c = std::cos(angle);
            const float s_a = std::sin(angle);
            // Build model matrix: translate + rotate Y
            vne::math::Mat4f model = vne::math::Mat4f::identity();
            // Column-major Mat4f: columns[col][row]
            model[0][0] = c;
            model[2][0] = s_a;
            model[0][2] = -s_a;
            model[2][2] = c;
            model[3][0] = kOffsets[i][0];
            model[3][1] = kOffsets[i][1];
            model[3][2] = kOffsets[i][2];

            const vne::math::Mat4f mvp = vp * model;
            device_->setMat4(shader_, "u_MVP", mvp);
            device_->setMat4(shader_, "u_Model", model);
            device_->drawIndexed(pipeline_, vbo_, ibo_, kCubeIdxCount, vne::testbed::DrawMode::eTriangles);
        }
    }

    // -----------------------------------------------------------------------
    // Accessors for the Settings panel
    // -----------------------------------------------------------------------
    bool use_perspective_{true};
    float fov_{60.0f};
    float near_{0.1f};
    float far_{1000.0f};
    float ortho_half_{6.0f};

    int cube_count_{1};
    float cube_rotation_speed_{0.5f};

    bool dir_light_enabled_{true};
    float dir_light_dir_[3]{-0.5f, -1.0f, -0.3f};
    float dir_light_color_[3]{1.0f, 0.97f, 0.9f};
    float dir_light_intensity_{1.0f};

    std::vector<PointLightEntry> point_lights_;

    void rebuildCamera(int w, int h) { buildCamera(w, h); }

    void syncDirLight() {
        dir_light_->setEnabled(dir_light_enabled_);
        dir_light_->setDirection({dir_light_dir_[0], dir_light_dir_[1], dir_light_dir_[2]});
        dir_light_->setColor({dir_light_color_[0], dir_light_color_[1], dir_light_color_[2]});
        dir_light_->setIntensity(dir_light_intensity_);
    }

    void addPointLight() {
        if (point_lights_.size() >= 4)
            return;
        PointLightEntry e;
        static const float kColors[4][3] = {{1, 0.4f, 0.2f}, {0.2f, 0.6f, 1}, {0.4f, 1, 0.4f}, {1, 0.2f, 1}};
        const std::size_t idx = point_lights_.size();
        e.color[0] = kColors[idx][0];
        e.color[1] = kColors[idx][1];
        e.color[2] = kColors[idx][2];
        e.orbit_angle = static_cast<float>(idx) * 1.5708f;
        e.light = std::make_shared<vne::scene::PointLight>(vne::math::Vec3f{e.orbit_radius, 1.5f, 0.f},
                                                           vne::math::Vec3f{e.color[0], e.color[1], e.color[2]},
                                                           e.intensity,
                                                           e.range,
                                                           "PointLight" + std::to_string(idx));
        scene_state_.addLight(e.light);
        point_lights_.push_back(std::move(e));
    }

    void removeLastPointLight() {
        if (point_lights_.empty())
            return;
        scene_state_.removeLight(point_lights_.back().light);
        point_lights_.pop_back();
    }

    void syncPointLight(std::size_t i) {
        if (i >= point_lights_.size())
            return;
        auto& e = point_lights_[i];
        e.light->setColor({e.color[0], e.color[1], e.color[2]});
        e.light->setIntensity(e.intensity);
        e.light->setRange(e.range);
        e.light->setEnabled(e.enabled);
    }

    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> cameraPersp() const {
        return cameras_persp_.empty() ? nullptr : cameras_persp_[0];
    }
    [[nodiscard]] const std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>>& getCameras() const {
        return cameras_persp_;
    }

   private:
    // -----------------------------------------------------------------------
    void buildCamera(int w, int h) {
        const float aspect = (h > 0) ? (static_cast<float>(w) / static_cast<float>(h)) : 1.f;
        cameras_persp_.resize(kMaxViewports);
        cameras_ortho_.resize(kMaxViewports);
        for (int i = 0; i < kMaxViewports; ++i) {
            auto persp = std::make_shared<vne::scene::PerspectiveCamera>(fov_,
                                                                         static_cast<float>(w),
                                                                         static_cast<float>(h),
                                                                         near_,
                                                                         far_,
                                                                         "SceneCamera" + std::to_string(i));
            persp->setPosition({4.f, 3.f, 6.f});
            persp->setTarget({0.f, 0.f, 0.f});
            persp->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
            persp->updateMatrices();
            cameras_persp_[static_cast<size_t>(i)] = std::move(persp);

            const float hw = ortho_half_ * aspect;
            auto ortho = std::make_shared<vne::scene::OrthographicCamera>(-hw,
                                                                          hw,
                                                                          -ortho_half_,
                                                                          ortho_half_,
                                                                          -100.f,
                                                                          100.f,
                                                                          "OrthoCamera" + std::to_string(i));
            ortho->setPosition({4.f, 3.f, 6.f});
            ortho->setTarget({0.f, 0.f, 0.f});
            ortho->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
            ortho->updateMatrices();
            cameras_ortho_[static_cast<size_t>(i)] = std::move(ortho);
        }
        scene_state_.setActiveCamera(use_perspective_
                                         ? std::static_pointer_cast<vne::scene::ICamera>(cameras_persp_[0])
                                         : std::static_pointer_cast<vne::scene::ICamera>(cameras_ortho_[0]));
    }

    void buildGeometry() {
        if (!device_)
            return;
        vbo_ = device_->createVertexBuffer(kCubeVerts, sizeof(kCubeVerts));
        ibo_ = device_->createIndexBuffer(kCubeIdx, kCubeIdxCount);

        // Vertex layout: pos(3) + normal(3) + color(3)
        // Shader uses mat4 for normal matrix, so we update kSceneVert to use mat4
        shader_ = device_->compileShader(kSceneVert, kSceneFrag);

        vne::testbed::PipelineDesc pd{};
        pd.shader = shader_;
        pd.layout = {{3}, {3}, {3}};
        pd.depth.testEnabled = true;
        pd.depth.writeEnabled = true;
        pd.rasterizer.cull = vne::testbed::CullMode::eBack;
        pipeline_ = device_->createPipeline(pd);
    }

    void buildLights() {
        dir_light_ = std::make_shared<vne::scene::DirectionalLight>(
            vne::math::Vec3f{dir_light_dir_[0], dir_light_dir_[1], dir_light_dir_[2]},
            vne::math::Vec3f{dir_light_color_[0], dir_light_color_[1], dir_light_color_[2]},
            dir_light_intensity_,
            "SunLight");
        scene_state_.addLight(dir_light_);
    }

    void updateCameraAspect(int w, int h) {
        const float aspect = static_cast<float>(w) / static_cast<float>(h);
        if (use_perspective_) {
            for (auto& cam : cameras_persp_) {
                if (cam) {
                    cam->setAspectRatio(aspect);
                    cam->updateProjectionMatrix();
                }
            }
        } else {
            const float hw = ortho_half_ * aspect;
            for (auto& cam : cameras_ortho_) {
                if (cam) {
                    cam->resize(hw * 2.f, ortho_half_ * 2.f);
                    cam->updateProjectionMatrix();
                }
            }
        }
    }

    vne::math::Mat4f activeVP(int vp_idx) const {
        const size_t i = static_cast<size_t>(vp_idx);
        if (use_perspective_ && i < cameras_persp_.size() && cameras_persp_[i]) {
            return cameras_persp_[i]->getViewProjectionMatrix();
        }
        if (!use_perspective_ && i < cameras_ortho_.size() && cameras_ortho_[i]) {
            return cameras_ortho_[i]->getViewProjectionMatrix();
        }
        return vne::math::Mat4f::identity();
    }

    vne::math::Vec3f activePosition(int vp_idx) const {
        const size_t i = static_cast<size_t>(vp_idx);
        if (use_perspective_ && i < cameras_persp_.size() && cameras_persp_[i]) {
            return cameras_persp_[i]->getPosition();
        }
        if (!use_perspective_ && i < cameras_ortho_.size() && cameras_ortho_[i]) {
            return cameras_ortho_[i]->getPosition();
        }
        return {};
    }

    static constexpr int kGridLines = 20;
    static constexpr float kGridSpacing = 1.0f;
    static constexpr float kGridHalf = kGridLines * kGridSpacing * 0.5f;

    void drawGrid() const {
        // Lighter than viewport clear (#4A4A4C) so grid is visible
        const vne::math::Vec3f col{0.50f, 0.50f, 0.52f};
        for (int i = -kGridLines; i <= kGridLines; ++i) {
            const float t = static_cast<float>(i) * kGridSpacing;
            debug_draw_->line({t, 0.f, -kGridHalf}, {t, 0.f, kGridHalf}, col);
            debug_draw_->line({-kGridHalf, 0.f, t}, {kGridHalf, 0.f, t}, col);
        }
    }
    void drawAxes() const {
        const float len = kGridHalf;
        debug_draw_->line({0, 0, 0}, {len, 0, 0}, {1.f, 0.2f, 0.2f});
        debug_draw_->line({0, 0, 0}, {0, len, 0}, {0.2f, 1.f, 0.2f});
        debug_draw_->line({0, 0, 0}, {0, 0, len}, {0.2f, 0.2f, 1.f});
    }

    vne::testbed::IRenderDevice* device_{nullptr};
    vne::testbed::IDebugDraw* debug_draw_{nullptr};
    vne::testbed::ShaderHandle shader_{};
    vne::testbed::BufferHandle vbo_{};
    vne::testbed::BufferHandle ibo_{};
    vne::testbed::PipelineHandle pipeline_{};

    std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>> cameras_persp_;
    std::vector<std::shared_ptr<vne::scene::OrthographicCamera>> cameras_ortho_;
    vne::scene::SceneState scene_state_;

    std::shared_ptr<vne::scene::DirectionalLight> dir_light_;

    float cube_angle_{0.f};
};

// ---------------------------------------------------------------------------
// SceneSettingsLayer — ImGui panel
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI
class SceneSettingsLayer : public vne::testbed::ILayer {
   public:
    SceneSettingsLayer()
        : vne::testbed::ILayer("SceneSettingsLayer") {
        setRenderSortKey(999);
    }

    void setImGuiLayer(vne::testbed::ImGuiLayer* l) { imgui_layer_ = l; }
    void setSceneLayer(SceneTestLayer* l) { scene_layer_ = l; }

    void onAttach(vne::testbed::AppContext& /*ctx*/) override {
        if (imgui_layer_) {
            imgui_layer_->setSettingsCallback([this]() { renderPanel(); });
        }
    }
    void onDetach() override {
        if (imgui_layer_) {
            imgui_layer_->setSettingsCallback(nullptr);
            imgui_layer_ = nullptr;
        }
        scene_layer_ = nullptr;
    }

   private:
    void renderPanel() {
        if (!scene_layer_)
            return;
        auto& sl = *scene_layer_;

        // ---- Camera ----
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool changed = false;
            const char* types[] = {"Perspective", "Orthographic"};
            int type_idx = sl.use_perspective_ ? 0 : 1;
            if (ImGui::Combo("Type", &type_idx, types, 2)) {
                sl.use_perspective_ = (type_idx == 0);
                changed = true;
            }
            if (sl.use_perspective_) {
                changed |= ImGui::SliderFloat("FOV", &sl.fov_, 20.f, 120.f);
                changed |= ImGui::SliderFloat("Near", &sl.near_, 0.01f, 10.f);
                changed |= ImGui::SliderFloat("Far", &sl.far_, 100.f, 5000.f);
            } else {
                changed |= ImGui::SliderFloat("Half-extent", &sl.ortho_half_, 1.f, 20.f);
            }
            if (changed)
                sl.rebuildCamera(1280, 720);
        }

        // ---- Cubes ----
        if (ImGui::CollapsingHeader("Cubes", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("Count", &sl.cube_count_, 1, 4);
            ImGui::SliderFloat("Rotation speed", &sl.cube_rotation_speed_, 0.f, 4.f);
        }

        // ---- Directional light ----
        if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool sync = false;
            sync |= ImGui::Checkbox("Enabled##dir", &sl.dir_light_enabled_);
            sync |= ImGui::SliderFloat3("Direction", sl.dir_light_dir_, -1.f, 1.f);
            sync |= ImGui::ColorEdit3("Color##dir", sl.dir_light_color_);
            sync |= ImGui::SliderFloat("Intensity##dir", &sl.dir_light_intensity_, 0.f, 5.f);
            if (sync)
                sl.syncDirLight();
        }

        // ---- Point lights ----
        if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
            const int npt = static_cast<int>(sl.point_lights_.size());
            ImGui::Text("Active: %d / 4", npt);
            if (npt < 4 && ImGui::Button("+ Add point light")) {
                sl.addPointLight();
            }
            if (npt > 0) {
                ImGui::SameLine();
                if (ImGui::Button("- Remove last")) {
                    sl.removeLastPointLight();
                }
            }
            for (int i = 0; i < npt; ++i) {
                auto& e = sl.point_lights_[static_cast<std::size_t>(i)];
                ImGui::PushID(i);
                const std::string hdr = "Point Light " + std::to_string(i + 1);
                if (ImGui::TreeNode(hdr.c_str())) {
                    bool s = false;
                    s |= ImGui::Checkbox("Enabled##pt", &e.enabled);
                    s |= ImGui::ColorEdit3("Color##pt", e.color);
                    s |= ImGui::SliderFloat("Intensity##pt", &e.intensity, 0.f, 10.f);
                    s |= ImGui::SliderFloat("Range##pt", &e.range, 0.5f, 20.f);
                    ImGui::SliderFloat("Orbit radius##pt", &e.orbit_radius, 0.5f, 10.f);
                    ImGui::SliderFloat("Orbit speed##pt", &e.orbit_speed, -4.f, 4.f);
                    if (s)
                        sl.syncPointLight(static_cast<std::size_t>(i));
                    ImGui::TreePop();
                }
                ImGui::PopID();
            }
        }
    }

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
    SceneTestLayer* scene_layer_{nullptr};
};
#endif  // VNE_TESTBED_IMGUI

// ---------------------------------------------------------------------------

void RegisterTestSceneDemo(vne::testbed::Application& app) {
    // Layer 1: scene (camera + cubes + lights)
    auto* scene = new SceneTestLayer();
    app.getLayerStack().pushLayer(std::unique_ptr<SceneTestLayer>(scene), app.getAppContext());

#ifdef VNE_TESTBED_INTERACTION
    // Layer 2: orbit-arcball (per-viewport cameras when using 2 or 4 viewports)
    auto* interaction = new BaseInteractionLayer("TestSceneInteractionLayer");
    interaction->setCameras(scene->getCameras());
    app.getLayerStack().pushLayer(std::unique_ptr<BaseInteractionLayer>(interaction), app.getAppContext());
#endif

#ifdef VNE_TESTBED_IMGUI
    auto* settings = new SceneSettingsLayer();
    auto* imgui = dynamic_cast<vne::testbed::ImGuiLayer*>(app.getLayerStack().findLayerByName("ImGuiLayer"));
    if (imgui) {
        settings->setImGuiLayer(imgui);
#ifdef VNE_TESTBED_INTERACTION
        interaction->setImGuiLayer(imgui);
#endif
    }
    settings->setSceneLayer(scene);
    app.getLayerStack().pushLayer(std::unique_ptr<SceneSettingsLayer>(settings), app.getAppContext());
#endif
}

}  // namespace

VNETESTBED_REGISTER_DEMO("test_scene", RegisterTestSceneDemo)
