/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   January 2026
 *
 * Sample 02_test_scene — implementation (camera, cubes, lights, ImGui panels).
 * ----------------------------------------------------------------------
 */

#include "demo_test_scene.h"

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#include "../common/base_scene_layer.h"

#include "vertexnova/logging/logging.h"
#include "vertexnova/math/math.h"

#include <cmath>
#include <filesystem>

namespace {

CREATE_VNE_LOGGER_CATEGORY("vnetestbed.samples.test_scene")

// File-scope constants (anonymous namespace, kPascalCase per CODING_GUIDELINES)
constexpr int kDefaultWidth = 1280;
constexpr int kDefaultHeight = 720;
constexpr float kDefaultAspectRatio = 16.0f / 9.0f;

/** Resolve shader path: try cwd and cwd/bin/samples so it works from build or bin/samples. */
static std::filesystem::path resolveShaderPath(const char* filename) {
    std::error_code ec;
    std::filesystem::path p = std::filesystem::current_path(ec) / filename;
    if (ec) {
        return {};
    }
    if (std::filesystem::exists(p, ec)) {
        return p;
    }
    p = std::filesystem::current_path(ec) / "bin/samples" / filename;
    if (ec) {
        return {};
    }
    if (std::filesystem::exists(p, ec)) {
        return p;
    }
    return {};
}

// ---------------------------------------------------------------------------
// Cube geometry — 24 vertices (unique normals per face), 36 indices
// Layout: vec3 pos, vec3 normal, vec3 color
// ---------------------------------------------------------------------------
// clang-format off
const float kCubeVerts[] = {
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

const uint32_t kCubeIdx[] = {
     0, 1, 2,  2, 3, 0,   // +Z
     4, 5, 6,  6, 7, 4,   // -Z
     8, 9,10, 10,11, 8,   // +X
    12,13,14, 14,15,12,   // -X
    16,17,18, 18,19,16,   // +Y
    20,21,22, 22,23,20,   // -Y
};
// clang-format on

constexpr uint32_t kCubeIdxCount = 36u;

// Shader filenames by backend (kPascalCase constants in anonymous namespace)
#if defined(VNE_TESTBED_OPENGL)
constexpr const char* kSceneVertFilename = "scene_vert.glsl";
constexpr const char* kSceneFragFilename = "scene_frag.glsl";
#else
constexpr const char* kSceneVertFilename = "scene_vert_es.glsl";
constexpr const char* kSceneFragFilename = "scene_frag_es.glsl";
#endif

}  // namespace

// ---------------------------------------------------------------------------
// SceneTestLayer implementation
// ---------------------------------------------------------------------------

SceneTestLayer::SceneTestLayer()
    : vne::testbed::ILayer("SceneTestLayer") {}

void SceneTestLayer::onAttach(vne::testbed::AppContext& ctx) {
    device_ = ctx.device;
    debug_draw_ = ctx.debugDraw;

    buildCamera(kDefaultWidth, kDefaultHeight);
    buildGeometry();
    buildLights();
}

void SceneTestLayer::onDetach() {
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

void SceneTestLayer::onUpdate(float dt) {
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

void SceneTestLayer::onRender(const vne::testbed::RenderContext& ctx) {
    if (!device_ || !shader_.isValid())
        return;

    const int vp_idx =
        (ctx.active_viewport_index >= 0 && ctx.active_viewport_index < kMaxViewports) ? ctx.active_viewport_index : 0;

    // Aspect / resize — only update when the viewport size actually changes
    if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
        if (ctx.frame_info.width != last_vp_w_ || ctx.frame_info.height != last_vp_h_) {
            last_vp_w_ = ctx.frame_info.width;
            last_vp_h_ = ctx.frame_info.height;
            updateCameraAspect(last_vp_w_, last_vp_h_);
        }
    }

    const auto vp = getActiveViewProjectionMatrix(vp_idx);
    const auto cam_pos = getActiveCameraPosition(vp_idx);

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
            debug_draw_->line({dirEnd.x() - cs, dirEnd.y(), dirEnd.z()}, {dirEnd.x() + cs, dirEnd.y(), dirEnd.z()}, lc);
            debug_draw_->line({dirEnd.x(), dirEnd.y() - cs, dirEnd.z()}, {dirEnd.x(), dirEnd.y() + cs, dirEnd.z()}, lc);
        }
        debug_draw_->flush();
    }

    // Upload lighting uniforms
    device_->setVec3(shader_, "u_ambientColor", ambient_light_->getColor());
    device_->setFloat(shader_,
                      "u_ambientIntensity",
                      ambient_light_->isEnabled() ? ambient_light_->getIntensity() : 0.f);

    const auto& dl = dir_light_;
    device_->setInt(shader_, "u_dirLightEnabled", dl->isEnabled() ? 1 : 0);
    device_->setVec3(shader_, "u_dirLightDir", dl->getDirection());
    device_->setVec3(shader_, "u_dirLightColor", dl->getColor());
    device_->setFloat(shader_, "u_dirLightIntensity", dl->getIntensity());

    const int npt = static_cast<int>(point_lights_.size());
    device_->setInt(shader_, "u_numPointLights", npt);
    for (int i = 0; i < npt && i < 4; ++i) {
        const auto& e = point_lights_[static_cast<std::size_t>(i)];
        const std::string idx = "[" + std::to_string(i) + "]";
        device_->setVec3(shader_, ("u_ptLightPos" + idx).c_str(), e.light->getPosition());
        device_->setVec3(shader_, ("u_ptLightColor" + idx).c_str(), e.light->getColor());
        device_->setFloat(shader_, ("u_ptLightIntensity" + idx).c_str(), e.light->getIntensity());
        device_->setFloat(shader_, ("u_ptLightRange" + idx).c_str(), e.light->getRange());
        device_->setInt(shader_, ("u_ptLightEnabled" + idx).c_str(), e.enabled ? 1 : 0);
    }
    device_->setInt(shader_, "u_spotLightEnabled", spot_light_->isEnabled() ? 1 : 0);
    device_->setVec3(shader_, "u_spotLightPos", spot_light_->getPosition());
    device_->setVec3(shader_, "u_spotLightDir", spot_light_->getDirection());
    device_->setVec3(shader_, "u_spotLightColor", spot_light_->getColor());
    device_->setFloat(shader_, "u_spotLightIntensity", spot_light_->getIntensity());
    device_->setFloat(shader_, "u_spotLightRange", spot_light_->getRange());
    // Enforce outer > inner + eps so (innerCos - outerCos) in shader is never 0 (avoids NaNs)
    float innerDeg = spot_light_inner_deg_;
    float outerDeg = spot_light_outer_deg_;
    if (outerDeg <= innerDeg + kSpotAngleEpsDeg) {
        outerDeg = innerDeg + kSpotAngleEpsDeg;
        spot_light_outer_deg_ = outerDeg;
    }
    if (innerDeg > outerDeg - kSpotAngleEpsDeg) {
        innerDeg = outerDeg - kSpotAngleEpsDeg;
        spot_light_inner_deg_ = innerDeg;
    }
    const float innerRad = innerDeg * 3.14159265f / 180.f;
    const float outerRad = outerDeg * 3.14159265f / 180.f;
    device_->setFloat(shader_, "u_spotLightInnerCos", std::cos(innerRad));
    device_->setFloat(shader_, "u_spotLightOuterCos", std::cos(outerRad));
    device_->setInt(shader_, "u_useAttnFormula", use_attn_formula_ ? 1 : 0);
    device_->setFloat(shader_, "u_attnConst", attn_const_);
    device_->setFloat(shader_, "u_attnLinear", attn_linear_);
    device_->setFloat(shader_, "u_attnQuad", attn_quad_);
    device_->setVec3(shader_, "u_camPos", cam_pos);

    // Draw cubes
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
        model[3][0] = cube_position_[i][0];
        model[3][1] = cube_position_[i][1];
        model[3][2] = cube_position_[i][2];

        const vne::math::Mat4f mvp = vp * model;
        device_->setMat4(shader_, "u_mvp", mvp);
        device_->setMat4(shader_, "u_model", model);
        device_->drawIndexed(pipeline_, vbo_, ibo_, kCubeIdxCount, vne::testbed::DrawMode::eTriangles);
    }
}

void SceneTestLayer::rebuildCamera(int w, int h) {
    if (w > 0 && h > 0) {
        last_vp_w_ = w;
        last_vp_h_ = h;
    }
    buildCamera(last_vp_w_, last_vp_h_);
}

void SceneTestLayer::syncCameraPositionTargetUp() {
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

[[nodiscard]] vne::math::Mat4f SceneTestLayer::getCubeModelMatrix(int i) const {
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
    model[3][0] = cube_position_[i][0];
    model[3][1] = cube_position_[i][1];
    model[3][2] = cube_position_[i][2];
    return model;
}

vne::scene::ICamera* SceneTestLayer::activeCamera(int vp_idx) const {
    const size_t i = static_cast<size_t>(vp_idx);
    if (use_perspective_ && i < cameras_persp_.size() && cameras_persp_[i])
        return cameras_persp_[i].get();
    if (!use_perspective_ && i < cameras_ortho_.size() && cameras_ortho_[i])
        return cameras_ortho_[i].get();
    return nullptr;
}

void SceneTestLayer::syncAmbientLight() {
    ambient_light_->setEnabled(ambient_light_enabled_);
    ambient_light_->setColor({ambient_light_color_[0], ambient_light_color_[1], ambient_light_color_[2]});
    ambient_light_->setIntensity(ambient_light_intensity_);
}

void SceneTestLayer::syncDirLight() {
    dir_light_->setEnabled(dir_light_enabled_);
    dir_light_->setDirection({dir_light_dir_[0], dir_light_dir_[1], dir_light_dir_[2]});
    dir_light_->setColor({dir_light_color_[0], dir_light_color_[1], dir_light_color_[2]});
    dir_light_->setIntensity(dir_light_intensity_);
}

void SceneTestLayer::syncSpotLight() {
    // Enforce outer > inner + eps so shader (innerCos - outerCos) is never 0
    if (spot_light_outer_deg_ <= spot_light_inner_deg_ + kSpotAngleEpsDeg)
        spot_light_outer_deg_ = spot_light_inner_deg_ + kSpotAngleEpsDeg;
    if (spot_light_inner_deg_ > spot_light_outer_deg_ - kSpotAngleEpsDeg)
        spot_light_inner_deg_ = spot_light_outer_deg_ - kSpotAngleEpsDeg;

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

void SceneTestLayer::addPointLight() {
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

void SceneTestLayer::removeLastPointLight() {
    if (point_lights_.empty())
        return;
    scene_state_.removeLight(point_lights_.back().light);
    point_lights_.pop_back();
}

void SceneTestLayer::resetToDefault() {
    use_perspective_ = true;
    fov_ = 60.0f;
    near_ = 0.1f;
    far_ = 1000.0f;
    ortho_half_ = 6.0f;
    ortho_near_ = -100.0f;
    ortho_far_ = 100.0f;
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
    show_frustum_ = true;
    show_cam_axes_ = true;
    show_near_far_planes_ = true;

    cube_count_ = 3;
    cube_rotation_speed_ = 0.5f;
    cube_angle_ = 0.f;
    cube_position_[0][0] = 0.f;
    cube_position_[0][1] = 0.5f;
    cube_position_[0][2] = 0.f;
    cube_position_[1][0] = 2.5f;
    cube_position_[1][1] = 0.5f;
    cube_position_[1][2] = 0.f;
    cube_position_[2][0] = -2.5f;
    cube_position_[2][1] = 0.5f;
    cube_position_[2][2] = 0.f;
    cube_position_[3][0] = 0.f;
    cube_position_[3][1] = 0.5f;
    cube_position_[3][2] = 2.5f;

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

void SceneTestLayer::syncPointLight(std::size_t i) {
    if (i >= point_lights_.size())
        return;
    auto& e = point_lights_[i];
    e.light->setColor({e.color[0], e.color[1], e.color[2]});
    e.light->setIntensity(e.intensity);
    e.light->setRange(e.range);
    e.light->setEnabled(e.enabled);
}

[[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> SceneTestLayer::cameraPersp() const {
    return cameras_persp_.empty() ? nullptr : cameras_persp_[0];
}
[[nodiscard]] const std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>>& SceneTestLayer::getCameras() const {
    return cameras_persp_;
}
[[nodiscard]] std::vector<std::shared_ptr<vne::scene::ICamera>> SceneTestLayer::getActiveCameras() const {
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

void SceneTestLayer::buildCamera(int w, int h) {
    const float aspect = (h > 0) ? (static_cast<float>(w) / static_cast<float>(h)) : kDefaultAspectRatio;
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
                                                                      ortho_near_,
                                                                      ortho_far_,
                                                                      "OrthoCamera" + std::to_string(i));
        ortho->setPosition({cam_position_[0], cam_position_[1], cam_position_[2]});
        ortho->setTarget({cam_target_[0], cam_target_[1], cam_target_[2]});
        ortho->setUp({cam_up_[0], cam_up_[1], cam_up_[2]});
        ortho->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
        ortho->updateMatrices();
        cameras_ortho_[static_cast<size_t>(i)] = std::move(ortho);
    }
    scene_state_.setActiveCamera(use_perspective_ ? std::static_pointer_cast<vne::scene::ICamera>(cameras_persp_[0])
                                                  : std::static_pointer_cast<vne::scene::ICamera>(cameras_ortho_[0]));
}

void SceneTestLayer::buildGeometry() {
    if (!device_) {
        return;
    }
    vbo_ = device_->createVertexBuffer(kCubeVerts, sizeof(kCubeVerts));
    ibo_ = device_->createIndexBuffer(kCubeIdx, kCubeIdxCount);

    std::filesystem::path vert_path = resolveShaderPath(kSceneVertFilename);
    std::filesystem::path frag_path = resolveShaderPath(kSceneFragFilename);
    if (!vert_path.empty() && !frag_path.empty()) {
        shader_ = device_->createShader(vert_path, frag_path);
    }
    if (!shader_.isValid()) {
        VNE_LOG_ERROR << "Scene shaders not loaded: ensure " << kSceneVertFilename << " and " << kSceneFragFilename
                      << " are next to the executable (e.g. run from build bin/samples).";
    }

    vne::testbed::PipelineDesc pd{};
    pd.shader = shader_;
    pd.layout = {{3}, {3}, {3}};
    pd.depth.testEnabled = true;
    pd.depth.writeEnabled = true;
    pd.rasterizer.cull = vne::testbed::CullMode::eBack;
    pipeline_ = device_->createPipeline(pd);
}

void SceneTestLayer::buildLights() {
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

    vne::math::Vec3f spot_dir(spot_light_dir_[0], spot_light_dir_[1], spot_light_dir_[2]);
    float len = std::sqrt(spot_dir.x() * spot_dir.x() + spot_dir.y() * spot_dir.y() + spot_dir.z() * spot_dir.z());
    if (len < vne::math::kFloatEpsilon) {
        spot_dir = vne::math::Vec3f(0.f, -1.f, 0.f);
    } else {
        spot_dir = vne::math::Vec3f(spot_dir.x() / len, spot_dir.y() / len, spot_dir.z() / len);
    }
    spot_light_ = std::make_shared<vne::scene::SpotLight>(
        vne::math::Vec3f{spot_light_pos_[0], spot_light_pos_[1], spot_light_pos_[2]},
        spot_dir,
        vne::math::Vec3f{spot_light_color_[0], spot_light_color_[1], spot_light_color_[2]},
        spot_light_intensity_,
        spot_light_range_,
        spot_light_inner_deg_,
        spot_light_outer_deg_,
        "Spot");
    spot_light_->setEnabled(spot_light_enabled_);
    scene_state_.addLight(spot_light_);
}

void SceneTestLayer::updateCameraAspect(int w, int h) {
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

vne::math::Mat4f SceneTestLayer::getActiveViewProjectionMatrix(int vp_idx) const {
    const auto i = static_cast<size_t>(vp_idx);
    if (use_perspective_ && i < cameras_persp_.size() && cameras_persp_[i]) {
        return cameras_persp_[i]->getViewProjectionMatrix();
    }
    if (!use_perspective_ && i < cameras_ortho_.size() && cameras_ortho_[i]) {
        return cameras_ortho_[i]->getViewProjectionMatrix();
    }
    return vne::math::Mat4f::identity();
}

vne::math::Vec3f SceneTestLayer::getActiveCameraPosition(int vp_idx) const {
    const auto i = static_cast<size_t>(vp_idx);
    if (use_perspective_ && i < cameras_persp_.size() && cameras_persp_[i]) {
        return cameras_persp_[i]->getPosition();
    }
    if (!use_perspective_ && i < cameras_ortho_.size() && cameras_ortho_[i]) {
        return cameras_ortho_[i]->getPosition();
    }
    return {};
}

void SceneTestLayer::drawCameraVisuals(int vp_idx) const {
    vne::scene::ICamera* cam = activeCamera(vp_idx);
    if (!cam || !debug_draw_) {
        return;
    }
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

    // Camera position cross (yellow)
    constexpr float s = 0.25f;
    const vne::math::Vec3f posColor{1.f, 1.f, 0.3f};
    debug_draw_->line({pos.x() - s, pos.y(), pos.z()}, {pos.x() + s, pos.y(), pos.z()}, posColor);
    debug_draw_->line({pos.x(), pos.y() - s, pos.z()}, {pos.x(), pos.y() + s, pos.z()}, posColor);
    debug_draw_->line({pos.x(), pos.y(), pos.z() - s}, {pos.x(), pos.y(), pos.z() + s}, posColor);

    // Target cross (green)
    const vne::math::Vec3f tgtColor{0.3f, 1.f, 0.3f};
    debug_draw_->line({tgt.x() - s, tgt.y(), tgt.z()}, {tgt.x() + s, tgt.y(), tgt.z()}, tgtColor);
    debug_draw_->line({tgt.x(), tgt.y() - s, tgt.z()}, {tgt.x(), tgt.y() + s, tgt.z()}, tgtColor);
    debug_draw_->line({tgt.x(), tgt.y(), tgt.z() - s}, {tgt.x(), tgt.y(), tgt.z() + s}, tgtColor);

    // Look direction line: pos → tgt (orange)
    debug_draw_->line(pos, tgt, {1.f, 0.5f, 0.2f});

    if (show_cam_axes_) {
        constexpr float axLen = 1.0f;
        // Up    — blue
        debug_draw_->line(pos,
                          {pos.x() + upOrtho.x() * axLen, pos.y() + upOrtho.y() * axLen, pos.z() + upOrtho.z() * axLen},
                          {0.3f, 0.5f, 1.f});
        // Right — red
        debug_draw_->line(pos,
                          {pos.x() + right.x() * axLen, pos.y() + right.y() * axLen, pos.z() + right.z() * axLen},
                          {1.f, 0.3f, 0.3f});
        // Forward — yellow-green
        debug_draw_->line(pos,
                          {pos.x() + fwd.x() * axLen, pos.y() + fwd.y() * axLen, pos.z() + fwd.z() * axLen},
                          {0.8f, 1.f, 0.2f});
    }

    if (show_frustum_)
        drawFrustum(pos, fwd, right, upOrtho);
}

void SceneTestLayer::drawFrustum(vne::math::Vec3f pos,
                                 vne::math::Vec3f fwd,
                                 vne::math::Vec3f right,
                                 vne::math::Vec3f up) const {
    // Draw frustum using actual camera clip distances, matching the
    // reference (matrixModelView_mac) technique: compute vertices in
    // camera-local space (apex at origin, planes along -Z) then
    // transform to world space via pos/fwd/right/up basis vectors.
    //
    // Cap far at 10 world units so the frustum stays scene-visible
    // when the user orbits around with the interaction layer.
    const float nearD = std::max(near_, 0.05f);
    const float farD = std::min(far_, 10.0f);

    // Helper: point along the look direction at distance d from pos
    auto along = [&](float d) -> vne::math::Vec3f {
        return {pos.x() + fwd.x() * d, pos.y() + fwd.y() * d, pos.z() + fwd.z() * d};
    };

    // Helper: offset a plane-centre by (rr * right + uu * up)
    auto corner = [&](vne::math::Vec3f c, float rr, float uu) -> vne::math::Vec3f {
        return {c.x() + right.x() * rr + up.x() * uu,
                c.y() + right.y() * rr + up.y() * uu,
                c.z() + right.z() * rr + up.z() * uu};
    };

    const vne::math::Vec3f nearCol{0.9f, 0.9f, 0.3f};  // yellow — near plane
    const vne::math::Vec3f farCol{0.3f, 0.8f, 1.0f};   // cyan   — far  plane
    const vne::math::Vec3f edgeCol{0.7f, 0.7f, 0.7f};  // gray   — connecting edges

    const float aspect = (last_vp_h_ > 0) ? (static_cast<float>(last_vp_w_) / static_cast<float>(last_vp_h_)) : 1.f;

    if (use_perspective_) {
        // Tangent of half-FOV gives the slope; multiply by distance to get half-extents.
        // Same formula as reference: tangent = tan(fovY/2), nearHeight = nearPlane * tangent
        const float halfFovRad = fov_ * 0.5f * 3.14159265f / 180.f;
        const float tanHalf = std::tan(halfFovRad);

        const float nH = tanHalf * nearD;
        const float nW = nH * aspect;
        const float fH = tanHalf * farD;
        const float fW = fH * aspect;

        const vne::math::Vec3f nc = along(nearD);
        const vne::math::Vec3f fc = along(farD);

        const vne::math::Vec3f nTL = corner(nc, -nW, nH);
        const vne::math::Vec3f nTR = corner(nc, nW, nH);
        const vne::math::Vec3f nBL = corner(nc, -nW, -nH);
        const vne::math::Vec3f nBR = corner(nc, nW, -nH);

        const vne::math::Vec3f fTL = corner(fc, -fW, fH);
        const vne::math::Vec3f fTR = corner(fc, fW, fH);
        const vne::math::Vec3f fBL = corner(fc, -fW, -fH);
        const vne::math::Vec3f fBR = corner(fc, fW, -fH);

        // Near plane rect
        debug_draw_->line(nTL, nTR, nearCol);
        debug_draw_->line(nTR, nBR, nearCol);
        debug_draw_->line(nBR, nBL, nearCol);
        debug_draw_->line(nBL, nTL, nearCol);

        // Far plane rect
        debug_draw_->line(fTL, fTR, farCol);
        debug_draw_->line(fTR, fBR, farCol);
        debug_draw_->line(fBR, fBL, farCol);
        debug_draw_->line(fBL, fTL, farCol);

        // 4 frustum edges from apex (camera position) to far corners
        debug_draw_->line(pos, fTL, edgeCol);
        debug_draw_->line(pos, fTR, edgeCol);
        debug_draw_->line(pos, fBL, edgeCol);
        debug_draw_->line(pos, fBR, edgeCol);
    } else {
        // Orthographic: constant extents across all depths — box shape.
        const float hw = ortho_half_ * aspect;
        const float hh = ortho_half_;

        const vne::math::Vec3f nc = along(nearD);
        const vne::math::Vec3f fc = along(farD);

        const vne::math::Vec3f nTL = corner(nc, -hw, hh);
        const vne::math::Vec3f nTR = corner(nc, hw, hh);
        const vne::math::Vec3f nBL = corner(nc, -hw, -hh);
        const vne::math::Vec3f nBR = corner(nc, hw, -hh);

        const vne::math::Vec3f fTL = corner(fc, -hw, hh);
        const vne::math::Vec3f fTR = corner(fc, hw, hh);
        const vne::math::Vec3f fBL = corner(fc, -hw, -hh);
        const vne::math::Vec3f fBR = corner(fc, hw, -hh);

        // Near plane rect
        debug_draw_->line(nTL, nTR, nearCol);
        debug_draw_->line(nTR, nBR, nearCol);
        debug_draw_->line(nBR, nBL, nearCol);
        debug_draw_->line(nBL, nTL, nearCol);

        // Far plane rect
        debug_draw_->line(fTL, fTR, farCol);
        debug_draw_->line(fTR, fBR, farCol);
        debug_draw_->line(fBR, fBL, farCol);
        debug_draw_->line(fBL, fTL, farCol);

        // Parallel edges (box, not pyramid)
        debug_draw_->line(nTL, fTL, edgeCol);
        debug_draw_->line(nTR, fTR, edgeCol);
        debug_draw_->line(nBL, fBL, edgeCol);
        debug_draw_->line(nBR, fBR, edgeCol);
    }
}

void SceneTestLayer::drawGrid() const {
    // Lighter than viewport clear (#4A4A4C) so grid is visible
    const vne::math::Vec3f col{0.50f, 0.50f, 0.52f};
    for (int i = -kGridLines; i <= kGridLines; ++i) {
        const float t = static_cast<float>(i) * kGridSpacing;
        debug_draw_->line({t, 0.f, -kGridHalf}, {t, 0.f, kGridHalf}, col);
        debug_draw_->line({-kGridHalf, 0.f, t}, {kGridHalf, 0.f, t}, col);
    }
}

void SceneTestLayer::drawAxes() const {
    const float len = kGridHalf;
    debug_draw_->line({0, 0, 0}, {len, 0, 0}, {1.f, 0.2f, 0.2f});
    debug_draw_->line({0, 0, 0}, {0, len, 0}, {0.2f, 1.f, 0.2f});
    debug_draw_->line({0, 0, 0}, {0, 0, len}, {0.2f, 0.2f, 1.f});
}

// ---------------------------------------------------------------------------
// SceneSettingsLayer implementation
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI
SceneSettingsLayer::SceneSettingsLayer()
    : vne::testbed::ILayer("SceneSettingsLayer") {
    setRenderSortKey(999);
}

void SceneSettingsLayer::setImGuiLayer(vne::testbed::ImGuiLayer* l) {
    imgui_layer_ = l;
}
void SceneSettingsLayer::setSceneLayer(SceneTestLayer* l) {
    scene_layer_ = l;
}
#ifdef VNE_TESTBED_INTERACTION
void SceneSettingsLayer::setInteractionLayer(BaseInteractionLayer* l) {
    interaction_layer_ = l;
}
#endif

void SceneSettingsLayer::onAttach(vne::testbed::AppContext& /*ctx*/) {
    if (imgui_layer_) {
        imgui_layer_->setSettingsCallback([this]() { renderPanel(); });
    }
}

void SceneSettingsLayer::onDetach() {
    if (imgui_layer_) {
        imgui_layer_->setSettingsCallback(nullptr);
        imgui_layer_ = nullptr;
    }
    scene_layer_ = nullptr;
#ifdef VNE_TESTBED_INTERACTION
    interaction_layer_ = nullptr;
#endif
}

void SceneSettingsLayer::renderPanel() {
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
            proj_changed |= ImGui::SliderFloat("Near", &sl.near_, -5.f, 10.f);
            proj_changed |= ImGui::SliderFloat("Far", &sl.far_, -5000.f, 5000.f);
        } else {
            proj_changed |= ImGui::SliderFloat("Half-extent", &sl.ortho_half_, 1.f, 20.f);
            proj_changed |= ImGui::SliderFloat("Near##ortho", &sl.ortho_near_, -500.f, 500.f);
            proj_changed |= ImGui::SliderFloat("Far##ortho", &sl.ortho_far_, -500.f, 500.f);
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
        if (sl.show_camera_visuals_) {
            ImGui::Indent();
            ImGui::Checkbox("Frustum wireframe", &sl.show_frustum_);
            ImGui::Checkbox("Camera axes (fwd/right/up)", &sl.show_cam_axes_);
            ImGui::Checkbox("Near plane crosshair", &sl.show_near_far_planes_);
            ImGui::Unindent();
        }
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
        ImGui::SliderInt("Count##cubes", &sl.cube_count_, 0, 4);
        ImGui::SliderFloat("Rotation speed##cubes", &sl.cube_rotation_speed_, 0.f, 4.f);
        for (int i = 0; i < 4; ++i) {
            ImGui::PushID(i);
            const std::string label = "Cube " + std::to_string(i + 1);
            if (ImGui::CollapsingHeader(label.c_str())) {
                const bool active = (i < sl.cube_count_);
                if (!active)
                    ImGui::TextDisabled("(not drawn)");
                else
                    ImGui::SliderFloat3("Position", sl.cube_position_[i], -120.f, 120.f);
                vne::math::Mat4f model = sl.getCubeModelMatrix(i);
                ImGui::Text("Model matrix (column-major):");
                for (size_t row = 0; row < 4u; ++row) {
                    ImGui::Text("%.4f  %.4f  %.4f  %.4f",
                                static_cast<double>(model[0][row]),
                                static_cast<double>(model[1][row]),
                                static_cast<double>(model[2][row]),
                                static_cast<double>(model[3][row]));
                }
            }
            ImGui::PopID();
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

VNETESTBED_REGISTER_DEMO("test_scene", RegisterTestSceneDemo)
