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

#include "vertexnova/scene/camera/camera_utils.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/light/ambient_light.h"
#include "vertexnova/scene/light/directional_light.h"
#include "vertexnova/scene/light/point_light.h"
#include "vertexnova/scene/light/spot_light.h"
#include "vertexnova/scene/scene_state.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#include "../common/base_scene_layer.h"

#include <array>
#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace {

/** Resolve shader path: try cwd and cwd/bin/samples so it works from build or bin/samples. */
static std::filesystem::path resolveShaderPath(const char* filename) {
    std::error_code ec;
    std::filesystem::path p = std::filesystem::current_path(ec) / filename;
    if (ec)
        return {};
    if (std::filesystem::exists(p, ec))
        return p;
    p = std::filesystem::current_path(ec) / "bin/samples" / filename;
    if (ec)
        return {};
    if (std::filesystem::exists(p, ec))
        return p;
    return {};
}

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

uniform vec3  u_AmbientColor;
uniform float u_AmbientIntensity;

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

uniform int   u_SpotLightEnabled;
uniform vec3  u_SpotLightPos;
uniform vec3  u_SpotLightDir;
uniform vec3  u_SpotLightColor;
uniform float u_SpotLightIntensity;
uniform float u_SpotLightRange;
uniform float u_SpotLightInnerCos;
uniform float u_SpotLightOuterCos;

uniform float u_AttnConst;
uniform float u_AttnLinear;
uniform float u_AttnQuad;
uniform int   u_UseAttnFormula;

uniform vec3  u_CamPos;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(u_CamPos - vWorldPos);
    vec3 lighting = u_AmbientColor * u_AmbientIntensity;

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
        float atten;
        if (u_UseAttnFormula != 0) {
            atten = 1.0 / (u_AttnConst + u_AttnLinear * dist + u_AttnQuad * dist * dist);
            atten *= step(dist, range);
        } else {
            atten = clamp(1.0 - (dist / range), 0.0, 1.0);
            atten *= atten;
        }
        vec3 L = normalize(toLight);
        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        lighting += u_PtLightColor[i] * u_PtLightIntensity[i] * atten
                    * (diff * 0.8 + spec * 0.3);
    }

    if (u_SpotLightEnabled != 0) {
        vec3  toLight = u_SpotLightPos - vWorldPos;
        float dist    = length(toLight);
        float range   = max(u_SpotLightRange, 0.001);
        if (dist <= range) {
            vec3 L = normalize(toLight);
            float cosTheta = dot(L, normalize(-u_SpotLightDir));
            float spot = clamp((cosTheta - u_SpotLightOuterCos) / (u_SpotLightInnerCos - u_SpotLightOuterCos), 0.0, 1.0);
            float atten;
            if (u_UseAttnFormula != 0) {
                atten = 1.0 / (u_AttnConst + u_AttnLinear * dist + u_AttnQuad * dist * dist);
            } else {
                atten = clamp(1.0 - (dist / range), 0.0, 1.0);
                atten *= atten;
            }
            atten *= spot;
            float diff = max(dot(N, L), 0.0);
            vec3 H = normalize(L + V);
            float spec = pow(max(dot(N, H), 0.0), 32.0);
            lighting += u_SpotLightColor * u_SpotLightIntensity * atten * (diff * 0.8 + spec * 0.3);
        }
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
uniform vec3  u_AmbientColor;
uniform float u_AmbientIntensity;
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
uniform int   u_SpotLightEnabled;
uniform vec3  u_SpotLightPos;
uniform vec3  u_SpotLightDir;
uniform vec3  u_SpotLightColor;
uniform float u_SpotLightIntensity;
uniform float u_SpotLightRange;
uniform float u_SpotLightInnerCos;
uniform float u_SpotLightOuterCos;
uniform float u_AttnConst;
uniform float u_AttnLinear;
uniform float u_AttnQuad;
uniform int   u_UseAttnFormula;
uniform vec3  u_CamPos;
void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(u_CamPos - vWorldPos);
    vec3 lighting = u_AmbientColor * u_AmbientIntensity;
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
        float atten;
        if (u_UseAttnFormula != 0) {
            atten = 1.0 / (u_AttnConst + u_AttnLinear * dist + u_AttnQuad * dist * dist);
            atten *= step(dist, range);
        } else {
            atten = clamp(1.0 - (dist / range), 0.0, 1.0);
            atten *= atten;
        }
        vec3 L = normalize(toLight);
        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        lighting += u_PtLightColor[i] * u_PtLightIntensity[i] * atten
                    * (diff * 0.8 + spec * 0.3);
    }
    if (u_SpotLightEnabled != 0) {
        vec3  toLight = u_SpotLightPos - vWorldPos;
        float dist    = length(toLight);
        float range   = max(u_SpotLightRange, 0.001);
        if (dist <= range) {
            vec3 L = normalize(toLight);
            float cosTheta = dot(L, normalize(-u_SpotLightDir));
            float spot = clamp((cosTheta - u_SpotLightOuterCos) / (u_SpotLightInnerCos - u_SpotLightOuterCos), 0.0, 1.0);
            float atten;
            if (u_UseAttnFormula != 0) {
                atten = 1.0 / (u_AttnConst + u_AttnLinear * dist + u_AttnQuad * dist * dist);
            } else {
                atten = clamp(1.0 - (dist / range), 0.0, 1.0);
                atten *= atten;
            }
            atten *= spot;
            float diff = max(dot(N, L), 0.0);
            vec3 H = normalize(L + V);
            float spec = pow(max(dot(N, H), 0.0), 32.0);
            lighting += u_SpotLightColor * u_SpotLightIntensity * atten * (diff * 0.8 + spec * 0.3);
        }
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

        // Aspect / resize — only update when the viewport size actually changes
        if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
            if (ctx.frame_info.width != last_vp_w_ || ctx.frame_info.height != last_vp_h_) {
                last_vp_w_ = ctx.frame_info.width;
                last_vp_h_ = ctx.frame_info.height;
                updateCameraAspect(last_vp_w_, last_vp_h_);
            }
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
                const float s = 0.2f;
                const vne::math::Vec3f lc{e.color[0], e.color[1], e.color[2]};
                debug_draw_->line({p.x() - s, p.y(), p.z()}, {p.x() + s, p.y(), p.z()}, lc);
                debug_draw_->line({p.x(), p.y() - s, p.z()}, {p.x(), p.y() + s, p.z()}, lc);
                debug_draw_->line({p.x(), p.y(), p.z() - s}, {p.x(), p.y(), p.z() + s}, lc);
            }
            if (show_camera_visuals_) {
                drawCameraVisuals(vp_idx);
            }
            if (spot_light_enabled_) {
                const auto p = spot_light_->getPosition();
                const auto d = spot_light_->getDirection();
                const float s = 0.2f;
                const vne::math::Vec3f lc(spot_light_color_[0], spot_light_color_[1], spot_light_color_[2]);
                debug_draw_->line({p.x() - s, p.y(), p.z()}, {p.x() + s, p.y(), p.z()}, lc);
                debug_draw_->line({p.x(), p.y() - s, p.z()}, {p.x(), p.y() + s, p.z()}, lc);
                debug_draw_->line({p.x(), p.y(), p.z() - s}, {p.x(), p.y(), p.z() + s}, lc);
                const float dirLen = 1.5f;
                const vne::math::Vec3f dirEnd{p.x() + d.x() * dirLen, p.y() + d.y() * dirLen, p.z() + d.z() * dirLen};
                debug_draw_->line(p, dirEnd, lc);
                // Small cross at aim point to mark cone direction end
                const float cs = 0.12f;
                debug_draw_->line({dirEnd.x() - cs, dirEnd.y(), dirEnd.z()},
                                  {dirEnd.x() + cs, dirEnd.y(), dirEnd.z()},
                                  lc);
                debug_draw_->line({dirEnd.x(), dirEnd.y() - cs, dirEnd.z()},
                                  {dirEnd.x(), dirEnd.y() + cs, dirEnd.z()},
                                  lc);
            }
            debug_draw_->flush();
        }

        // Upload lighting uniforms
        device_->setVec3(shader_, "u_AmbientColor", ambient_light_->getColor());
        device_->setFloat(shader_,
                          "u_AmbientIntensity",
                          ambient_light_->isEnabled() ? ambient_light_->getIntensity() : 0.f);

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
        device_->setInt(shader_, "u_SpotLightEnabled", spot_light_->isEnabled() ? 1 : 0);
        device_->setVec3(shader_, "u_SpotLightPos", spot_light_->getPosition());
        device_->setVec3(shader_, "u_SpotLightDir", spot_light_->getDirection());
        device_->setVec3(shader_, "u_SpotLightColor", spot_light_->getColor());
        device_->setFloat(shader_, "u_SpotLightIntensity", spot_light_->getIntensity());
        device_->setFloat(shader_, "u_SpotLightRange", spot_light_->getRange());
        const float innerRad = spot_light_inner_deg_ * 3.14159265f / 180.f;
        const float outerRad = spot_light_outer_deg_ * 3.14159265f / 180.f;
        device_->setFloat(shader_, "u_SpotLightInnerCos", std::cos(innerRad));
        device_->setFloat(shader_, "u_SpotLightOuterCos", std::cos(outerRad));
        device_->setInt(shader_, "u_UseAttnFormula", use_attn_formula_ ? 1 : 0);
        device_->setFloat(shader_, "u_AttnConst", attn_const_);
        device_->setFloat(shader_, "u_AttnLinear", attn_linear_);
        device_->setFloat(shader_, "u_AttnQuad", attn_quad_);
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
    float cam_position_[3]{4.f, 3.f, 6.f};
    float cam_target_[3]{0.f, 0.f, 0.f};
    float cam_up_[3]{0.f, 1.f, 0.f};
    bool show_view_matrix_{false};
    bool show_projection_matrix_{false};
    bool show_camera_visuals_{true};
    int last_vp_w_{1280};
    int last_vp_h_{720};

    int cube_count_{3};
    float cube_rotation_speed_{0.5f};

    bool ambient_light_enabled_{true};
    float ambient_light_color_[3]{0.08f, 0.08f, 0.1f};
    float ambient_light_intensity_{1.0f};

    bool dir_light_enabled_{true};
    float dir_light_dir_[3]{-0.5f, -1.0f, -0.3f};
    float dir_light_color_[3]{1.0f, 0.97f, 0.9f};
    float dir_light_intensity_{1.0f};

    bool spot_light_enabled_{false};
    float spot_light_pos_[3]{0.f, 4.f, 4.f};
    float spot_light_dir_[3]{0.f, -0.7f, -0.7f};
    float spot_light_color_[3]{1.f, 0.9f, 0.7f};
    float spot_light_intensity_{2.f};
    float spot_light_range_{10.f};
    float spot_light_inner_deg_{15.f};
    float spot_light_outer_deg_{30.f};

    bool use_attn_formula_{false};
    float attn_const_{1.f};
    float attn_linear_{0.09f};
    float attn_quad_{0.032f};

    std::vector<PointLightEntry> point_lights_;

    void rebuildCamera(int w, int h) {
        if (w > 0 && h > 0) {
            last_vp_w_ = w;
            last_vp_h_ = h;
        }
        buildCamera(last_vp_w_, last_vp_h_);
    }

    void syncCameraPositionTargetUp() {
        const vne::math::Vec3f pos{cam_position_[0], cam_position_[1], cam_position_[2]};
        const vne::math::Vec3f tgt{cam_target_[0], cam_target_[1], cam_target_[2]};
        const vne::math::Vec3f up{cam_up_[0], cam_up_[1], cam_up_[2]};
        for (auto& cam : cameras_persp_) {
            if (cam) {
                cam->setPosition(pos);
                cam->setTarget(tgt);
                cam->setUp(up);
                cam->updateMatrices();
            }
        }
        for (auto& cam : cameras_ortho_) {
            if (cam) {
                cam->setPosition(pos);
                cam->setTarget(tgt);
                cam->setUp(up);
                cam->updateMatrices();
            }
        }
    }

    /** @brief Model matrix for cube i (same as world transform; no hierarchy). */
    [[nodiscard]] vne::math::Mat4f getCubeModelMatrix(int i) const {
        static const float kOffsets[4][3] = {
            {0.f, 0.5f, 0.f},
            {2.5f, 0.5f, 0.f},
            {-2.5f, 0.5f, 0.f},
            {0.f, 0.5f, 2.5f},
        };
        if (i < 0 || i >= 4)
            return vne::math::Mat4f::identity();
        const float angle = cube_angle_ + static_cast<float>(i) * 1.0472f;
        const float c = std::cos(angle);
        const float s_a = std::sin(angle);
        vne::math::Mat4f model = vne::math::Mat4f::identity();
        model[0][0] = c;
        model[2][0] = s_a;
        model[0][2] = -s_a;
        model[2][2] = c;
        model[3][0] = kOffsets[i][0];
        model[3][1] = kOffsets[i][1];
        model[3][2] = kOffsets[i][2];
        return model;
    }

    vne::scene::ICamera* activeCamera(int vp_idx) const {
        const size_t i = static_cast<size_t>(vp_idx);
        if (use_perspective_ && i < cameras_persp_.size() && cameras_persp_[i])
            return cameras_persp_[i].get();
        if (!use_perspective_ && i < cameras_ortho_.size() && cameras_ortho_[i])
            return cameras_ortho_[i].get();
        return nullptr;
    }

    void syncAmbientLight() {
        ambient_light_->setEnabled(ambient_light_enabled_);
        ambient_light_->setColor({ambient_light_color_[0], ambient_light_color_[1], ambient_light_color_[2]});
        ambient_light_->setIntensity(ambient_light_intensity_);
    }

    void syncDirLight() {
        dir_light_->setEnabled(dir_light_enabled_);
        dir_light_->setDirection({dir_light_dir_[0], dir_light_dir_[1], dir_light_dir_[2]});
        dir_light_->setColor({dir_light_color_[0], dir_light_color_[1], dir_light_color_[2]});
        dir_light_->setIntensity(dir_light_intensity_);
    }

    void syncSpotLight() {
        spot_light_->setEnabled(spot_light_enabled_);
        spot_light_->setPosition({spot_light_pos_[0], spot_light_pos_[1], spot_light_pos_[2]});
        vne::math::Vec3f dir(spot_light_dir_[0], spot_light_dir_[1], spot_light_dir_[2]);
        float len = std::sqrt(dir.x() * dir.x() + dir.y() * dir.y() + dir.z() * dir.z());
        if (len < 1e-6f)
            dir = vne::math::Vec3f(0.f, -1.f, 0.f);
        else
            dir = vne::math::Vec3f(dir.x() / len, dir.y() / len, dir.z() / len);
        spot_light_->setDirection(dir);
        spot_light_->setColor({spot_light_color_[0], spot_light_color_[1], spot_light_color_[2]});
        spot_light_->setIntensity(spot_light_intensity_);
        spot_light_->setRange(spot_light_range_);
        spot_light_->setInnerOuterAnglesDeg(spot_light_inner_deg_, spot_light_outer_deg_);
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

    /** @brief Reset camera, cubes, lights, and UI state to default values. */
    void resetToDefault() {
        use_perspective_ = true;
        fov_ = 60.0f;
        near_ = 0.1f;
        far_ = 1000.0f;
        ortho_half_ = 6.0f;
        cam_position_[0] = 4.f;
        cam_position_[1] = 3.f;
        cam_position_[2] = 6.f;
        cam_target_[0] = 0.f;
        cam_target_[1] = 0.f;
        cam_target_[2] = 0.f;
        cam_up_[0] = 0.f;
        cam_up_[1] = 1.f;
        cam_up_[2] = 0.f;
        show_view_matrix_ = false;
        show_projection_matrix_ = false;
        show_camera_visuals_ = true;

        cube_count_ = 3;
        cube_rotation_speed_ = 0.5f;
        cube_angle_ = 0.f;

        ambient_light_enabled_ = true;
        ambient_light_color_[0] = 0.08f;
        ambient_light_color_[1] = 0.08f;
        ambient_light_color_[2] = 0.1f;
        ambient_light_intensity_ = 1.0f;

        dir_light_enabled_ = true;
        dir_light_dir_[0] = -0.5f;
        dir_light_dir_[1] = -1.0f;
        dir_light_dir_[2] = -0.3f;
        dir_light_color_[0] = 1.0f;
        dir_light_color_[1] = 0.97f;
        dir_light_color_[2] = 0.9f;
        dir_light_intensity_ = 1.0f;

        spot_light_enabled_ = false;
        spot_light_pos_[0] = 0.f;
        spot_light_pos_[1] = 4.f;
        spot_light_pos_[2] = 4.f;
        spot_light_dir_[0] = 0.f;
        spot_light_dir_[1] = -0.7f;
        spot_light_dir_[2] = -0.7f;
        spot_light_color_[0] = 1.f;
        spot_light_color_[1] = 0.9f;
        spot_light_color_[2] = 0.7f;
        spot_light_intensity_ = 2.f;
        spot_light_range_ = 10.f;
        spot_light_inner_deg_ = 15.f;
        spot_light_outer_deg_ = 30.f;

        use_attn_formula_ = false;
        attn_const_ = 1.f;
        attn_linear_ = 0.09f;
        attn_quad_ = 0.032f;

        while (!point_lights_.empty())
            removeLastPointLight();

        rebuildCamera(last_vp_w_, last_vp_h_);
        syncCameraPositionTargetUp();
        syncAmbientLight();
        syncDirLight();
        syncSpotLight();
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
    /** @brief Returns the currently active camera set (perspective or orthographic) so interaction can drive the
     * visible camera. */
    [[nodiscard]] std::vector<std::shared_ptr<vne::scene::ICamera>> getActiveCameras() const {
        std::vector<std::shared_ptr<vne::scene::ICamera>> out;
        if (use_perspective_) {
            for (const auto& c : cameras_persp_)
                if (c)
                    out.push_back(c);
        } else {
            for (const auto& c : cameras_ortho_)
                if (c)
                    out.push_back(c);
        }
        return out;
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
            persp->setPosition({cam_position_[0], cam_position_[1], cam_position_[2]});
            persp->setTarget({cam_target_[0], cam_target_[1], cam_target_[2]});
            persp->setUp({cam_up_[0], cam_up_[1], cam_up_[2]});
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
            ortho->setPosition({cam_position_[0], cam_position_[1], cam_position_[2]});
            ortho->setTarget({cam_target_[0], cam_target_[1], cam_target_[2]});
            ortho->setUp({cam_up_[0], cam_up_[1], cam_up_[2]});
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

        std::filesystem::path vert_path = resolveShaderPath("scene_vert.glsl");
        std::filesystem::path frag_path = resolveShaderPath("scene_frag.glsl");
        if (!vert_path.empty() && !frag_path.empty()) {
            shader_ = device_->createShader(vert_path, frag_path);
        }
        if (!shader_.isValid()) {
            shader_ = device_->compileShader(kSceneVert, kSceneFrag);
        }

        vne::testbed::PipelineDesc pd{};
        pd.shader = shader_;
        pd.layout = {{3}, {3}, {3}};
        pd.depth.testEnabled = true;
        pd.depth.writeEnabled = true;
        pd.rasterizer.cull = vne::testbed::CullMode::eBack;
        pipeline_ = device_->createPipeline(pd);
    }

    void buildLights() {
        ambient_light_ = std::make_shared<vne::scene::AmbientLight>(
            vne::math::Vec3f{ambient_light_color_[0], ambient_light_color_[1], ambient_light_color_[2]},
            ambient_light_intensity_,
            "Ambient");
        ambient_light_->setEnabled(ambient_light_enabled_);
        scene_state_.addLight(ambient_light_);

        dir_light_ = std::make_shared<vne::scene::DirectionalLight>(
            vne::math::Vec3f{dir_light_dir_[0], dir_light_dir_[1], dir_light_dir_[2]},
            vne::math::Vec3f{dir_light_color_[0], dir_light_color_[1], dir_light_color_[2]},
            dir_light_intensity_,
            "SunLight");
        scene_state_.addLight(dir_light_);

        vne::math::Vec3f spotDir(spot_light_dir_[0], spot_light_dir_[1], spot_light_dir_[2]);
        float len = std::sqrt(spotDir.x() * spotDir.x() + spotDir.y() * spotDir.y() + spotDir.z() * spotDir.z());
        if (len < 1e-6f)
            spotDir = vne::math::Vec3f(0.f, -1.f, 0.f);
        else
            spotDir = vne::math::Vec3f(spotDir.x() / len, spotDir.y() / len, spotDir.z() / len);
        spot_light_ = std::make_shared<vne::scene::SpotLight>(
            vne::math::Vec3f{spot_light_pos_[0], spot_light_pos_[1], spot_light_pos_[2]},
            spotDir,
            vne::math::Vec3f{spot_light_color_[0], spot_light_color_[1], spot_light_color_[2]},
            spot_light_intensity_,
            spot_light_range_,
            spot_light_inner_deg_,
            spot_light_outer_deg_,
            "Spot");
        spot_light_->setEnabled(spot_light_enabled_);
        scene_state_.addLight(spot_light_);
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

    // Analytically compute and draw camera frustum, position/target/up markers.
    // Uses camera vectors + FOV/ortho params directly — stable across parameter changes,
    // no dependency on per-frame pixel dimensions or matrix inversion.
    void drawCameraVisuals(int vp_idx) const {
        vne::scene::ICamera* cam = activeCamera(vp_idx);
        if (!cam || !debug_draw_)
            return;

        const vne::math::Vec3f pos = cam->getPosition();
        const vne::math::Vec3f tgt = cam->getTarget();
        const vne::math::Vec3f up = cam->getUp();

        // Forward = normalize(tgt - pos)
        vne::math::Vec3f fwd{tgt.x() - pos.x(), tgt.y() - pos.y(), tgt.z() - pos.z()};
        const float fwdLen = std::sqrt(fwd.x() * fwd.x() + fwd.y() * fwd.y() + fwd.z() * fwd.z());
        if (fwdLen < 1e-6f)
            return;
        fwd = vne::math::Vec3f{fwd.x() / fwdLen, fwd.y() / fwdLen, fwd.z() / fwdLen};

        // Right = normalize(fwd × up)
        vne::math::Vec3f right{fwd.y() * up.z() - fwd.z() * up.y(),
                               fwd.z() * up.x() - fwd.x() * up.z(),
                               fwd.x() * up.y() - fwd.y() * up.x()};
        const float rLen = std::sqrt(right.x() * right.x() + right.y() * right.y() + right.z() * right.z());
        if (rLen < 1e-6f)
            return;
        right = vne::math::Vec3f{right.x() / rLen, right.y() / rLen, right.z() / rLen};

        // Ortho-up = right × fwd (re-orthogonalized)
        const vne::math::Vec3f upOrtho{right.y() * fwd.z() - right.z() * fwd.y(),
                                       right.z() * fwd.x() - right.x() * fwd.z(),
                                       right.x() * fwd.y() - right.y() * fwd.x()};

        // Visual frustum in world space at fixed near/far distances from camera
        const float nearDist = 0.4f;
        const float farDist = 2.5f;
        const float aspect =
            (last_vp_h_ > 0) ? (static_cast<float>(last_vp_w_) / static_cast<float>(last_vp_h_)) : (16.f / 9.f);

        const vne::math::Vec3f frustumColor{0.55f, 0.65f, 0.85f};

        if (use_perspective_) {
            const float halfFovY = fov_ * (3.14159265f / 180.f) * 0.5f;
            const float hN = nearDist * std::tan(halfFovY);
            const float hF = farDist * std::tan(halfFovY);
            const float wN = hN * aspect;
            const float wF = hF * aspect;

            const vne::math::Vec3f nC{pos.x() + fwd.x() * nearDist,
                                      pos.y() + fwd.y() * nearDist,
                                      pos.z() + fwd.z() * nearDist};
            const vne::math::Vec3f fC{pos.x() + fwd.x() * farDist,
                                      pos.y() + fwd.y() * farDist,
                                      pos.z() + fwd.z() * farDist};

            const vne::math::Vec3f nc[4] = {
                {nC.x() + right.x() * wN + upOrtho.x() * hN,
                 nC.y() + right.y() * wN + upOrtho.y() * hN,
                 nC.z() + right.z() * wN + upOrtho.z() * hN},
                {nC.x() - right.x() * wN + upOrtho.x() * hN,
                 nC.y() - right.y() * wN + upOrtho.y() * hN,
                 nC.z() - right.z() * wN + upOrtho.z() * hN},
                {nC.x() - right.x() * wN - upOrtho.x() * hN,
                 nC.y() - right.y() * wN - upOrtho.y() * hN,
                 nC.z() - right.z() * wN - upOrtho.z() * hN},
                {nC.x() + right.x() * wN - upOrtho.x() * hN,
                 nC.y() + right.y() * wN - upOrtho.y() * hN,
                 nC.z() + right.z() * wN - upOrtho.z() * hN},
            };
            const vne::math::Vec3f fc[4] = {
                {fC.x() + right.x() * wF + upOrtho.x() * hF,
                 fC.y() + right.y() * wF + upOrtho.y() * hF,
                 fC.z() + right.z() * wF + upOrtho.z() * hF},
                {fC.x() - right.x() * wF + upOrtho.x() * hF,
                 fC.y() - right.y() * wF + upOrtho.y() * hF,
                 fC.z() - right.z() * wF + upOrtho.z() * hF},
                {fC.x() - right.x() * wF - upOrtho.x() * hF,
                 fC.y() - right.y() * wF - upOrtho.y() * hF,
                 fC.z() - right.z() * wF - upOrtho.z() * hF},
                {fC.x() + right.x() * wF - upOrtho.x() * hF,
                 fC.y() + right.y() * wF - upOrtho.y() * hF,
                 fC.z() + right.z() * wF - upOrtho.z() * hF},
            };
            // Near rect, far rect, apex lines from camera pos to far corners
            for (int i = 0; i < 4; ++i) {
                debug_draw_->line(nc[i], nc[(i + 1) % 4], frustumColor);
                debug_draw_->line(fc[i], fc[(i + 1) % 4], frustumColor);
                debug_draw_->line(pos, fc[i], frustumColor);
            }
        } else {
            // Orthographic: parallel sides, same extents at near and far
            const float hH = ortho_half_;
            const float hW = hH * aspect;
            const vne::math::Vec3f nC{pos.x() + fwd.x() * nearDist,
                                      pos.y() + fwd.y() * nearDist,
                                      pos.z() + fwd.z() * nearDist};
            const vne::math::Vec3f fC{pos.x() + fwd.x() * farDist,
                                      pos.y() + fwd.y() * farDist,
                                      pos.z() + fwd.z() * farDist};
            const vne::math::Vec3f nc[4] = {
                {nC.x() + right.x() * hW + upOrtho.x() * hH,
                 nC.y() + right.y() * hW + upOrtho.y() * hH,
                 nC.z() + right.z() * hW + upOrtho.z() * hH},
                {nC.x() - right.x() * hW + upOrtho.x() * hH,
                 nC.y() - right.y() * hW + upOrtho.y() * hH,
                 nC.z() - right.z() * hW + upOrtho.z() * hH},
                {nC.x() - right.x() * hW - upOrtho.x() * hH,
                 nC.y() - right.y() * hW - upOrtho.y() * hH,
                 nC.z() - right.z() * hW - upOrtho.z() * hH},
                {nC.x() + right.x() * hW - upOrtho.x() * hH,
                 nC.y() + right.y() * hW - upOrtho.y() * hH,
                 nC.z() + right.z() * hW - upOrtho.z() * hH},
            };
            const vne::math::Vec3f fc[4] = {
                {fC.x() + right.x() * hW + upOrtho.x() * hH,
                 fC.y() + right.y() * hW + upOrtho.y() * hH,
                 fC.z() + right.z() * hW + upOrtho.z() * hH},
                {fC.x() - right.x() * hW + upOrtho.x() * hH,
                 fC.y() - right.y() * hW + upOrtho.y() * hH,
                 fC.z() - right.z() * hW + upOrtho.z() * hH},
                {fC.x() - right.x() * hW - upOrtho.x() * hH,
                 fC.y() - right.y() * hW - upOrtho.y() * hH,
                 fC.z() - right.z() * hW - upOrtho.z() * hH},
                {fC.x() + right.x() * hW - upOrtho.x() * hH,
                 fC.y() + right.y() * hW - upOrtho.y() * hH,
                 fC.z() + right.z() * hW - upOrtho.z() * hH},
            };
            for (int i = 0; i < 4; ++i) {
                debug_draw_->line(nc[i], nc[(i + 1) % 4], frustumColor);
                debug_draw_->line(fc[i], fc[(i + 1) % 4], frustumColor);
                debug_draw_->line(nc[i], fc[i], frustumColor);  // parallel side edges
            }
        }

        // Camera position cross (yellow)
        const float s = 0.25f;
        const vne::math::Vec3f posColor{1.f, 1.f, 0.3f};
        debug_draw_->line({pos.x() - s, pos.y(), pos.z()}, {pos.x() + s, pos.y(), pos.z()}, posColor);
        debug_draw_->line({pos.x(), pos.y() - s, pos.z()}, {pos.x(), pos.y() + s, pos.z()}, posColor);
        debug_draw_->line({pos.x(), pos.y(), pos.z() - s}, {pos.x(), pos.y(), pos.z() + s}, posColor);

        // Target cross (green)
        const vne::math::Vec3f tgtColor{0.3f, 1.f, 0.3f};
        debug_draw_->line({tgt.x() - s, tgt.y(), tgt.z()}, {tgt.x() + s, tgt.y(), tgt.z()}, tgtColor);
        debug_draw_->line({tgt.x(), tgt.y() - s, tgt.z()}, {tgt.x(), tgt.y() + s, tgt.z()}, tgtColor);
        debug_draw_->line({tgt.x(), tgt.y(), tgt.z() - s}, {tgt.x(), tgt.y(), tgt.z() + s}, tgtColor);

        // Up vector arrow (blue) from camera position, using re-orthogonalized up
        const float upLen = 1.0f;
        const vne::math::Vec3f upEnd{pos.x() + upOrtho.x() * upLen,
                                     pos.y() + upOrtho.y() * upLen,
                                     pos.z() + upOrtho.z() * upLen};
        debug_draw_->line(pos, upEnd, {0.3f, 0.5f, 1.f});

        // Look direction line: pos → tgt (orange)
        debug_draw_->line(pos, tgt, {1.f, 0.5f, 0.2f});
    }

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

    std::shared_ptr<vne::scene::AmbientLight> ambient_light_;
    std::shared_ptr<vne::scene::DirectionalLight> dir_light_;
    std::shared_ptr<vne::scene::SpotLight> spot_light_;

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
#ifdef VNE_TESTBED_INTERACTION
    void setInteractionLayer(BaseInteractionLayer* l) { interaction_layer_ = l; }
#endif

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
#ifdef VNE_TESTBED_INTERACTION
        interaction_layer_ = nullptr;
#endif
    }

   private:
    void renderPanel() {
        if (!scene_layer_)
            return;
        auto& sl = *scene_layer_;

        if (ImGui::Button("Reset scene to default")) {
            sl.resetToDefault();
#ifdef VNE_TESTBED_INTERACTION
            if (interaction_layer_) {
                interaction_layer_->setInteractionEnabled(true);
                interaction_layer_->setCameras(sl.getActiveCameras());
            }
#endif
        }
        if (ImGui::IsItemHovered())
            ImGui::SetTooltip("Reset camera, cubes, lights, and options to default values.");

        // ---- Camera ----
        if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (vne::scene::ICamera* cam = sl.activeCamera(0)) {
                vne::math::Vec3f p = cam->getPosition();
                vne::math::Vec3f t = cam->getTarget();
                vne::math::Vec3f u = cam->getUp();
                sl.cam_position_[0] = p.x();
                sl.cam_position_[1] = p.y();
                sl.cam_position_[2] = p.z();
                sl.cam_target_[0] = t.x();
                sl.cam_target_[1] = t.y();
                sl.cam_target_[2] = t.z();
                sl.cam_up_[0] = u.x();
                sl.cam_up_[1] = u.y();
                sl.cam_up_[2] = u.z();
            }
            bool pos_changed = false;
            bool proj_changed = false;
            const char* types[] = {"Perspective", "Orthographic"};
            int type_idx = sl.use_perspective_ ? 0 : 1;
            if (ImGui::Combo("Type", &type_idx, types, 2)) {
                sl.use_perspective_ = (type_idx == 0);
                proj_changed = true;
            }
            pos_changed |= ImGui::SliderFloat3("Position", sl.cam_position_, -20.f, 20.f);
            pos_changed |= ImGui::SliderFloat3("Target", sl.cam_target_, -20.f, 20.f);
            pos_changed |= ImGui::SliderFloat3("Up", sl.cam_up_, -1.f, 1.f);
            if (sl.use_perspective_) {
                proj_changed |= ImGui::SliderFloat("FOV", &sl.fov_, 20.f, 120.f);
                proj_changed |= ImGui::SliderFloat("Near", &sl.near_, 0.01f, 10.f);
                proj_changed |= ImGui::SliderFloat("Far", &sl.far_, 100.f, 5000.f);
            } else {
                proj_changed |= ImGui::SliderFloat("Half-extent", &sl.ortho_half_, 1.f, 20.f);
            }
            if (pos_changed)
                sl.syncCameraPositionTargetUp();
            if (proj_changed) {
                sl.rebuildCamera(sl.last_vp_w_, sl.last_vp_h_);
#ifdef VNE_TESTBED_INTERACTION
                if (interaction_layer_)
                    interaction_layer_->setCameras(sl.getActiveCameras());
#endif
            }
            ImGui::Checkbox("Show camera visuals", &sl.show_camera_visuals_);
            ImGui::Checkbox("Show view matrix", &sl.show_view_matrix_);
            ImGui::Checkbox("Show projection matrix", &sl.show_projection_matrix_);
            if (sl.show_view_matrix_) {
                if (vne::scene::ICamera* cam2 = sl.activeCamera(0)) {
                    vne::math::Mat4f view = cam2->getViewMatrix();
                    ImGui::Text("View matrix (column-major):");
                    for (size_t row = 0; row < 4u; ++row) {
                        ImGui::Text("%.4f  %.4f  %.4f  %.4f",
                                    static_cast<double>(view[0][row]),
                                    static_cast<double>(view[1][row]),
                                    static_cast<double>(view[2][row]),
                                    static_cast<double>(view[3][row]));
                    }
                }
            }
            if (sl.show_projection_matrix_) {
                if (vne::scene::ICamera* cam2 = sl.activeCamera(0)) {
                    vne::math::Mat4f proj = cam2->getProjectionMatrix();
                    ImGui::Text("Projection matrix (column-major):");
                    for (size_t row = 0; row < 4u; ++row) {
                        ImGui::Text("%.4f  %.4f  %.4f  %.4f",
                                    static_cast<double>(proj[0][row]),
                                    static_cast<double>(proj[1][row]),
                                    static_cast<double>(proj[2][row]),
                                    static_cast<double>(proj[3][row]));
                    }
                }
            }
        }

#ifdef VNE_TESTBED_INTERACTION
        if (ImGui::CollapsingHeader("Interaction", ImGuiTreeNodeFlags_DefaultOpen)) {
            if (interaction_layer_) {
                bool enabled = interaction_layer_->getInteractionEnabled();
                if (ImGui::Checkbox("Enable VNE camera interaction", &enabled)) {
                    interaction_layer_->setInteractionEnabled(enabled);
                }
            }
        }
#endif

        // ---- Cubes ----
        if (ImGui::CollapsingHeader("Cubes", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::SliderInt("Count", &sl.cube_count_, 3, 4);
            ImGui::SliderFloat("Rotation speed", &sl.cube_rotation_speed_, 0.f, 4.f);
            for (int i = 0; i < 4; ++i) {
                const std::string label = "Cube " + std::to_string(i + 1);
                if (ImGui::CollapsingHeader(label.c_str())) {
                    const bool active = (i < sl.cube_count_);
                    if (!active)
                        ImGui::TextDisabled("(not drawn)");
                    vne::math::Mat4f model = sl.getCubeModelMatrix(i);
                    ImGui::Text("Model matrix (column-major):");
                    for (size_t row = 0; row < 4u; ++row) {
                        ImGui::Text("%.4f  %.4f  %.4f  %.4f",
                                    static_cast<double>(model[0][row]),
                                    static_cast<double>(model[1][row]),
                                    static_cast<double>(model[2][row]),
                                    static_cast<double>(model[3][row]));
                    }
                    ImGui::Text("Model-to-world: same (no hierarchy)");
                }
            }
        }

        // ---- Ambient ----
        if (ImGui::CollapsingHeader("Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool sync = false;
            sync |= ImGui::Checkbox("Enabled##amb", &sl.ambient_light_enabled_);
            sync |= ImGui::ColorEdit3("Color##amb", sl.ambient_light_color_);
            sync |= ImGui::SliderFloat("Intensity##amb", &sl.ambient_light_intensity_, 0.f, 2.f);
            if (sync)
                sl.syncAmbientLight();
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

        // ---- Spot light ----
        if (ImGui::CollapsingHeader("Spot Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool sync = false;
            sync |= ImGui::Checkbox("Enabled##spot", &sl.spot_light_enabled_);
            sync |= ImGui::SliderFloat3("Position##spot", sl.spot_light_pos_, -10.f, 10.f);
            sync |= ImGui::SliderFloat3("Direction##spot", sl.spot_light_dir_, -1.f, 1.f);
            sync |= ImGui::ColorEdit3("Color##spot", sl.spot_light_color_);
            sync |= ImGui::SliderFloat("Intensity##spot", &sl.spot_light_intensity_, 0.f, 10.f);
            sync |= ImGui::SliderFloat("Range##spot", &sl.spot_light_range_, 0.5f, 20.f);
            sync |= ImGui::SliderFloat("Inner angle (deg)##spot", &sl.spot_light_inner_deg_, 1.f, 89.f);
            sync |= ImGui::SliderFloat("Outer angle (deg)##spot", &sl.spot_light_outer_deg_, 1.f, 90.f);
            if (sync)
                sl.syncSpotLight();
        }

        if (ImGui::CollapsingHeader("Attenuation (point/spot)", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::Checkbox("Use attenuation formula (const + linear*d + quad*d^2)", &sl.use_attn_formula_);
            if (sl.use_attn_formula_) {
                ImGui::SliderFloat("Constant##attn", &sl.attn_const_, 0.01f, 2.f);
                ImGui::SliderFloat("Linear##attn", &sl.attn_linear_, 0.f, 0.5f);
                ImGui::SliderFloat("Quadratic##attn", &sl.attn_quad_, 0.f, 0.1f);
            }
        }
    }

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
    SceneTestLayer* scene_layer_{nullptr};
#ifdef VNE_TESTBED_INTERACTION
    BaseInteractionLayer* interaction_layer_{nullptr};
#endif
};
#endif  // VNE_TESTBED_IMGUI

// ---------------------------------------------------------------------------

void RegisterTestSceneDemo(vne::testbed::Application& app) {
    // Layer 1: scene (camera + cubes + lights)
    auto* scene = new SceneTestLayer();
    app.getLayerStack().pushLayer(std::unique_ptr<SceneTestLayer>(scene), app.getAppContext());

#ifdef VNE_TESTBED_INTERACTION
    // Layer 2: orbit-arcball (per-viewport cameras; use active set so ortho/persp switch works)
    auto* interaction = new BaseInteractionLayer("TestSceneInteractionLayer");
    interaction->setCameras(scene->getActiveCameras());
    app.getLayerStack().pushLayer(std::unique_ptr<BaseInteractionLayer>(interaction), app.getAppContext());
#endif

#ifdef VNE_TESTBED_IMGUI
    auto* settings = new SceneSettingsLayer();
    auto* imgui = dynamic_cast<vne::testbed::ImGuiLayer*>(app.getLayerStack().findLayerByName("ImGuiLayer"));
    if (imgui) {
        settings->setImGuiLayer(imgui);
#ifdef VNE_TESTBED_INTERACTION
        interaction->setImGuiLayer(imgui);
        settings->setInteractionLayer(interaction);
#endif
    }
    settings->setSceneLayer(scene);
    app.getLayerStack().pushLayer(std::unique_ptr<SceneSettingsLayer>(settings), app.getAppContext());
#endif
}

}  // namespace

VNETESTBED_REGISTER_DEMO("test_scene", RegisterTestSceneDemo)
