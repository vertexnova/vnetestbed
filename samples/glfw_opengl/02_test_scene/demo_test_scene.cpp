/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 *
 * ----------------------------------------------------------------------
 */

#include "demo_test_scene.h"

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/renderer/core_renderer.h"
#include "vertexnova/testbed/renderer/mesh_renderer.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#include "../common/base_scene_layer.h"

#include "vertexnova/logging/logging.h"
#include "vertexnova/math/math.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace {

CREATE_VNE_LOGGER_CATEGORY("vnetestbed.samples.test_scene")

// File-scope constants (anonymous namespace, kPascalCase per CODING_GUIDELINES)
constexpr int kRenderSortKey = 999;  //!< Settings panel layer — just before ImGuiLayer (1000)
constexpr int kDefaultWidth = 1280;
constexpr int kDefaultHeight = 720;
constexpr float kDefaultAspectRatio = 16.0f / 9.0f;

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

}  // namespace

namespace vne::samples::test_scene {

// ---------------------------------------------------------------------------
// SceneTestLayer implementation
// ---------------------------------------------------------------------------

SceneTestLayer::SceneTestLayer()
    : vne::testbed::ILayer("SceneTestLayer") {}

void SceneTestLayer::onAttach(vne::testbed::AppContext& app_context) {
    device_ = app_context.device;
    debug_draw_ = app_context.debugDraw;
    if (app_context.coreRenderer) {
        mesh_renderer_ = app_context.coreRenderer->getMeshRenderer();
    }
    if (!mesh_renderer_) {
        VNE_LOG_ERROR << "SceneTestLayer: AppContext has no MeshRenderer (coreRenderer missing or not initialized)";
    }

    buildCamera(kDefaultWidth, kDefaultHeight);
    buildGeometry();
    buildLights();
}

void SceneTestLayer::onDetach() {
    if (device_) {
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
    mesh_renderer_ = nullptr;
    device_ = nullptr;
}

void SceneTestLayer::onUpdate(float dt) {
    cube_angle_ += ui_.cube_rotation_speed * dt;

    // Orbit point lights
    for (auto& entry : ui_.point_lights) {
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

void SceneTestLayer::onRender(const vne::testbed::RenderContext& render_context) {
    if (!device_ || !mesh_renderer_ || !vbo_.isValid())
        return;

    const int vp_idx =
        (render_context.active_viewport_index >= 0 && render_context.active_viewport_index < kMaxViewports)
            ? render_context.active_viewport_index
            : 0;

    // Aspect / resize — only update when the viewport size actually changes
    if (render_context.frame_info.width > 0 && render_context.frame_info.height > 0) {
        if (render_context.frame_info.width != ui_.last_vp_w || render_context.frame_info.height != ui_.last_vp_h) {
            ui_.last_vp_w = render_context.frame_info.width;
            ui_.last_vp_h = render_context.frame_info.height;
            updateCameraAspect(ui_.last_vp_w, ui_.last_vp_h);
        }
    }

    const auto vp = getActiveViewProjectionMatrix(vp_idx);

    // Draw grid + axes via debug draw
    if (debug_draw_) {
        debug_draw_->setViewProjectionMatrix(vp);
        drawGrid();
        drawAxes();

        // Draw point-light positions as small crosses
        constexpr float kCrossSize = 0.2f;
        const vne::math::Vec3f kAxis[3] = {vne::math::Vec3f::xAxis(),
                                           vne::math::Vec3f::yAxis(),
                                           vne::math::Vec3f::zAxis()};
        for (const auto& e : ui_.point_lights) {
            if (!e.enabled)
                continue;
            const vne::math::Vec3f p = e.light->getPosition();
            const vne::math::Vec3f lc(e.color[0], e.color[1], e.color[2]);
            for (const auto& ax : kAxis)
                debug_draw_->line(p - ax * kCrossSize, p + ax * kCrossSize, lc);
        }
        if (ui_.show_camera_visuals)
            drawCameraVisuals(vp_idx);
        if (ui_.spot_light_enabled) {
            const vne::math::Vec3f p = spot_light_->getPosition();
            const vne::math::Vec3f d = spot_light_->getDirection();
            const vne::math::Vec3f lc(ui_.spot_light_color[0], ui_.spot_light_color[1], ui_.spot_light_color[2]);
            for (const auto& ax : kAxis)
                debug_draw_->line(p - ax * kCrossSize, p + ax * kCrossSize, lc);
            const vne::math::Vec3f dir_end = p + d * 1.5f;
            debug_draw_->line(p, dir_end, lc);
            constexpr float kAimCross = 0.12f;
            for (const auto& ax : kAxis)
                debug_draw_->line(dir_end - ax * kAimCross, dir_end + ax * kAimCross, lc);
        }
        debug_draw_->flush();
    }

    // Enforce outer > inner + eps so (innerCos - outerCos) in shader is never 0 (avoids NaNs)
    {
        float inner_deg = ui_.spot_light_inner_deg;
        float outer_deg = ui_.spot_light_outer_deg;
        if (outer_deg <= inner_deg + kSpotAngleEpsDeg) {
            outer_deg = inner_deg + kSpotAngleEpsDeg;
            ui_.spot_light_outer_deg = outer_deg;
        }
        if (inner_deg > outer_deg - kSpotAngleEpsDeg) {
            inner_deg = outer_deg - kSpotAngleEpsDeg;
            ui_.spot_light_inner_deg = inner_deg;
        }
    }

    vne::scene::ICamera* cam = activeCamera(vp_idx);
    if (!cam) {
        return;
    }

    const vne::testbed::PhongLightParams lights = buildPhongLightParams();

    for (int i = 0; i < ui_.cube_count; ++i) {
        const vne::math::Mat4f model = getCubeModelMatrix(i);
        mesh_renderer_->drawMesh(device_, cam, vbo_, ibo_, kCubeIdxCount, model, lights);
    }
}

void SceneTestLayer::rebuildCamera(int w, int h) {
    if (w > 0 && h > 0) {
        ui_.last_vp_w = w;
        ui_.last_vp_h = h;
    }
    buildCamera(ui_.last_vp_w, ui_.last_vp_h);
}

void SceneTestLayer::syncCameraPositionTargetUp() {
    const vne::math::Vec3f pos{ui_.cam_position[0], ui_.cam_position[1], ui_.cam_position[2]};
    const vne::math::Vec3f tgt{ui_.cam_target[0], ui_.cam_target[1], ui_.cam_target[2]};
    const vne::math::Vec3f up{ui_.cam_up[0], ui_.cam_up[1], ui_.cam_up[2]};
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
    const vne::math::Vec3f trans(ui_.cube_position[i][0], ui_.cube_position[i][1], ui_.cube_position[i][2]);
    return vne::math::Mat4f::translate(trans) * vne::math::Mat4f::rotateY(angle);
}

vne::scene::ICamera* SceneTestLayer::activeCamera(int vp_idx) const {
    const size_t i = static_cast<size_t>(vp_idx);
    if (ui_.use_perspective && i < cameras_persp_.size() && cameras_persp_[i])
        return cameras_persp_[i].get();
    if (!ui_.use_perspective && i < cameras_ortho_.size() && cameras_ortho_[i])
        return cameras_ortho_[i].get();
    return nullptr;
}

vne::testbed::PhongLightParams SceneTestLayer::buildPhongLightParams() {
    vne::testbed::PhongLightParams out{};
    out.ambient_color = ambient_light_->getColor();
    out.ambient_intensity = ambient_light_->isEnabled() ? ambient_light_->getIntensity() : 0.f;

    out.dir_light_enabled = dir_light_->isEnabled();
    out.dir_light_dir = dir_light_->getDirection();
    out.dir_light_color = dir_light_->getColor();
    out.dir_light_intensity = dir_light_->getIntensity();

    const int npt = static_cast<int>(ui_.point_lights.size());
    for (int i = 0; i < npt && i < vne::testbed::PhongLightParams::kMaxPointLights; ++i) {
        const auto& e = ui_.point_lights[static_cast<std::size_t>(i)];
        auto& pt = out.point_lights[i];
        pt.enabled = e.enabled;
        pt.position = e.light->getPosition();
        pt.color = e.light->getColor();
        pt.intensity = e.light->getIntensity();
        pt.range = e.light->getRange();
    }

    out.spot_light.enabled = spot_light_->isEnabled();
    out.spot_light.position = spot_light_->getPosition();
    out.spot_light.direction = spot_light_->getDirection();
    out.spot_light.color = spot_light_->getColor();
    out.spot_light.intensity = spot_light_->getIntensity();
    out.spot_light.range = spot_light_->getRange();
    const float inner_rad = ui_.spot_light_inner_deg * 3.14159265f / 180.f;
    const float outer_rad = ui_.spot_light_outer_deg * 3.14159265f / 180.f;
    out.spot_light.inner_angle_rad = inner_rad;
    out.spot_light.outer_angle_rad = outer_rad;

    out.use_attn_formula = ui_.use_attn_formula;
    out.attn_const = ui_.attn_const;
    out.attn_linear = ui_.attn_linear;
    out.attn_quad = ui_.attn_quad;

    return out;
}

void SceneTestLayer::syncAmbientLight() {
    ambient_light_->setEnabled(ui_.ambient_light_enabled);
    ambient_light_->setColor({ui_.ambient_light_color[0], ui_.ambient_light_color[1], ui_.ambient_light_color[2]});
    ambient_light_->setIntensity(ui_.ambient_light_intensity);
}

void SceneTestLayer::syncDirLight() {
    dir_light_->setEnabled(ui_.dir_light_enabled);
    dir_light_->setDirection({ui_.dir_light_dir[0], ui_.dir_light_dir[1], ui_.dir_light_dir[2]});
    dir_light_->setColor({ui_.dir_light_color[0], ui_.dir_light_color[1], ui_.dir_light_color[2]});
    dir_light_->setIntensity(ui_.dir_light_intensity);
}

void SceneTestLayer::syncSpotLight() {
    if (ui_.spot_light_outer_deg <= ui_.spot_light_inner_deg + kSpotAngleEpsDeg)
        ui_.spot_light_outer_deg = ui_.spot_light_inner_deg + kSpotAngleEpsDeg;
    if (ui_.spot_light_inner_deg > ui_.spot_light_outer_deg - kSpotAngleEpsDeg)
        ui_.spot_light_inner_deg = ui_.spot_light_outer_deg - kSpotAngleEpsDeg;

    spot_light_->setEnabled(ui_.spot_light_enabled);
    spot_light_->setPosition({ui_.spot_light_pos[0], ui_.spot_light_pos[1], ui_.spot_light_pos[2]});
    vne::math::Vec3f dir(ui_.spot_light_dir[0], ui_.spot_light_dir[1], ui_.spot_light_dir[2]);
    dir = (dir.length() < 1e-6f) ? vne::math::Vec3f(0.f, -1.f, 0.f) : dir.normalized();
    spot_light_->setDirection(dir);
    spot_light_->setColor({ui_.spot_light_color[0], ui_.spot_light_color[1], ui_.spot_light_color[2]});
    spot_light_->setIntensity(ui_.spot_light_intensity);
    spot_light_->setRange(ui_.spot_light_range);
    spot_light_->setInnerOuterAnglesDeg(ui_.spot_light_inner_deg, ui_.spot_light_outer_deg);
}

void SceneTestLayer::addPointLight() {
    if (ui_.point_lights.size() >= 4)
        return;
    PointLightEntry e;
    static const float kColors[4][3] = {{1, 0.4f, 0.2f}, {0.2f, 0.6f, 1}, {0.4f, 1, 0.4f}, {1, 0.2f, 1}};
    const std::size_t idx = ui_.point_lights.size();
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
    ui_.point_lights.push_back(std::move(e));
}

void SceneTestLayer::removeLastPointLight() {
    if (ui_.point_lights.empty())
        return;
    scene_state_.removeLight(ui_.point_lights.back().light);
    ui_.point_lights.pop_back();
}

void SceneTestLayer::resetToDefault() {
    const int save_w = ui_.last_vp_w;
    const int save_h = ui_.last_vp_h;
    ui_ = SceneUiSettings{};
    ui_.last_vp_w = save_w;
    ui_.last_vp_h = save_h;
    cube_angle_ = 0.f;

    while (!ui_.point_lights.empty())
        removeLastPointLight();

    rebuildCamera(ui_.last_vp_w, ui_.last_vp_h);
    syncCameraPositionTargetUp();
    syncAmbientLight();
    syncDirLight();
    syncSpotLight();
}

void SceneTestLayer::syncPointLight(std::size_t i) {
    if (i >= ui_.point_lights.size())
        return;
    auto& e = ui_.point_lights[i];
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
    if (ui_.use_perspective) {
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
        auto persp = std::make_shared<vne::scene::PerspectiveCamera>(ui_.fov,
                                                                     static_cast<float>(w),
                                                                     static_cast<float>(h),
                                                                     ui_.near_plane,
                                                                     ui_.far_plane,
                                                                     "SceneCamera" + std::to_string(i));
        persp->setPosition({ui_.cam_position[0], ui_.cam_position[1], ui_.cam_position[2]});
        persp->setTarget({ui_.cam_target[0], ui_.cam_target[1], ui_.cam_target[2]});
        persp->setUp({ui_.cam_up[0], ui_.cam_up[1], ui_.cam_up[2]});
        persp->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
        persp->updateMatrices();
        cameras_persp_[static_cast<size_t>(i)] = std::move(persp);

        const float hw = ui_.ortho_half * aspect;
        auto ortho = std::make_shared<vne::scene::OrthographicCamera>(-hw,
                                                                      hw,
                                                                      -ui_.ortho_half,
                                                                      ui_.ortho_half,
                                                                      ui_.ortho_near,
                                                                      ui_.ortho_far,
                                                                      "OrthoCamera" + std::to_string(i));
        ortho->setPosition({ui_.cam_position[0], ui_.cam_position[1], ui_.cam_position[2]});
        ortho->setTarget({ui_.cam_target[0], ui_.cam_target[1], ui_.cam_target[2]});
        ortho->setUp({ui_.cam_up[0], ui_.cam_up[1], ui_.cam_up[2]});
        ortho->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
        ortho->updateMatrices();
        cameras_ortho_[static_cast<size_t>(i)] = std::move(ortho);
    }
    scene_state_.setActiveCamera(ui_.use_perspective
                                     ? std::static_pointer_cast<vne::scene::ICamera>(cameras_persp_[0])
                                     : std::static_pointer_cast<vne::scene::ICamera>(cameras_ortho_[0]));
}

void SceneTestLayer::buildGeometry() {
    if (!device_) {
        return;
    }
    vbo_ = device_->createVertexBuffer(kCubeVerts, sizeof(kCubeVerts));
    ibo_ = device_->createIndexBuffer(kCubeIdx, kCubeIdxCount);
}

void SceneTestLayer::buildLights() {
    ambient_light_ = std::make_shared<vne::scene::AmbientLight>(
        vne::math::Vec3f{ui_.ambient_light_color[0], ui_.ambient_light_color[1], ui_.ambient_light_color[2]},
        ui_.ambient_light_intensity,
        "Ambient");
    ambient_light_->setEnabled(ui_.ambient_light_enabled);
    scene_state_.addLight(ambient_light_);

    dir_light_ = std::make_shared<vne::scene::DirectionalLight>(
        vne::math::Vec3f{ui_.dir_light_dir[0], ui_.dir_light_dir[1], ui_.dir_light_dir[2]},
        vne::math::Vec3f{ui_.dir_light_color[0], ui_.dir_light_color[1], ui_.dir_light_color[2]},
        ui_.dir_light_intensity,
        "SunLight");
    scene_state_.addLight(dir_light_);

    vne::math::Vec3f spot_dir(ui_.spot_light_dir[0], ui_.spot_light_dir[1], ui_.spot_light_dir[2]);
    spot_dir = (spot_dir.length() < 1e-6f) ? vne::math::Vec3f(0.f, -1.f, 0.f) : spot_dir.normalized();
    spot_light_ = std::make_shared<vne::scene::SpotLight>(
        vne::math::Vec3f{ui_.spot_light_pos[0], ui_.spot_light_pos[1], ui_.spot_light_pos[2]},
        spot_dir,
        vne::math::Vec3f{ui_.spot_light_color[0], ui_.spot_light_color[1], ui_.spot_light_color[2]},
        ui_.spot_light_intensity,
        ui_.spot_light_range,
        ui_.spot_light_inner_deg,
        ui_.spot_light_outer_deg,
        "Spot");
    spot_light_->setEnabled(ui_.spot_light_enabled);
    scene_state_.addLight(spot_light_);
}

void SceneTestLayer::updateCameraAspect(int w, int h) {
    const float aspect = static_cast<float>(w) / static_cast<float>(h);
    if (ui_.use_perspective) {
        for (auto& cam : cameras_persp_) {
            if (cam) {
                cam->setAspectRatio(aspect);
                cam->updateProjectionMatrix();
            }
        }
    } else {
        const float hw = ui_.ortho_half * aspect;
        for (auto& cam : cameras_ortho_) {
            if (cam) {
                cam->resize(hw * 2.f, ui_.ortho_half * 2.f);
                cam->updateProjectionMatrix();
            }
        }
    }
}

vne::math::Mat4f SceneTestLayer::getActiveViewProjectionMatrix(int vp_idx) const {
    const auto i = static_cast<size_t>(vp_idx);
    if (ui_.use_perspective && i < cameras_persp_.size() && cameras_persp_[i]) {
        return cameras_persp_[i]->getViewProjectionMatrix();
    }
    if (!ui_.use_perspective && i < cameras_ortho_.size() && cameras_ortho_[i]) {
        return cameras_ortho_[i]->getViewProjectionMatrix();
    }
    return vne::math::Mat4f::identity();
}

vne::math::Vec3f SceneTestLayer::getActiveCameraPosition(int vp_idx) const {
    const auto i = static_cast<size_t>(vp_idx);
    if (ui_.use_perspective && i < cameras_persp_.size() && cameras_persp_[i]) {
        return cameras_persp_[i]->getPosition();
    }
    if (!ui_.use_perspective && i < cameras_ortho_.size() && cameras_ortho_[i]) {
        return cameras_ortho_[i]->getPosition();
    }
    return {};
}

void SceneTestLayer::drawCameraVisuals(int vp_idx) const {
    vne::scene::ICamera* cam = activeCamera(vp_idx);
    if (!cam || !debug_draw_)
        return;
    const vne::math::Vec3f pos = cam->getPosition();
    const vne::math::Vec3f tgt = cam->getTarget();
    const vne::math::Vec3f up = cam->getUp();

    const vne::math::Vec3f fwd_vec = tgt - pos;
    if (fwd_vec.lengthSquared() < 1e-12f)
        return;
    const vne::math::Vec3f fwd = fwd_vec.normalized();
    vne::math::Vec3f right = fwd.cross(up).normalized();
    if (right.lengthSquared() < 1e-12f)
        return;
    const vne::math::Vec3f up_ortho = right.cross(fwd);

    constexpr float s = 0.25f;
    const vne::math::Vec3f pos_color{1.f, 1.f, 0.3f};
    const vne::math::Vec3f tgt_color{0.3f, 1.f, 0.3f};
    using V = vne::math::Vec3f;
    debug_draw_->line(pos - V::xAxis() * s, pos + V::xAxis() * s, pos_color);
    debug_draw_->line(pos - V::yAxis() * s, pos + V::yAxis() * s, pos_color);
    debug_draw_->line(pos - V::zAxis() * s, pos + V::zAxis() * s, pos_color);
    debug_draw_->line(tgt - V::xAxis() * s, tgt + V::xAxis() * s, tgt_color);
    debug_draw_->line(tgt - V::yAxis() * s, tgt + V::yAxis() * s, tgt_color);
    debug_draw_->line(tgt - V::zAxis() * s, tgt + V::zAxis() * s, tgt_color);
    debug_draw_->line(pos, tgt, {1.f, 0.5f, 0.2f});

    if (ui_.show_cam_axes) {
        constexpr float ax_len = 1.0f;
        debug_draw_->line(pos, pos + up_ortho * ax_len, {0.3f, 0.5f, 1.f});
        debug_draw_->line(pos, pos + right * ax_len, {1.f, 0.3f, 0.3f});
        debug_draw_->line(pos, pos + fwd * ax_len, {0.8f, 1.f, 0.2f});
    }
    if (ui_.show_frustum)
        drawFrustum(pos, fwd, right, up_ortho);
}

void SceneTestLayer::drawFrustum(vne::math::Vec3f pos,
                                 vne::math::Vec3f fwd,
                                 vne::math::Vec3f right,
                                 vne::math::Vec3f up) const {
    // Draw frustum using the camera clip distances, matching the
    // reference (matrixModelView_mac) technique: compute vertices in
    // camera-local space (apex at origin, planes along -Z) then
    // transform to world space via pos/fwd/right/up basis vectors.
    //
    // Clamp near to a small positive value and ensure far is beyond near
    // so the frustum visualization stays well-defined even for
    // misconfigured clip planes.
    const float near_dist = std::max(ui_.near_plane, 0.05f);
    const float far_dist = std::max(ui_.far_plane, near_dist + 0.01f);

    const auto along = [&](float d) { return pos + fwd * d; };
    const auto corner = [&](const vne::math::Vec3f& c, float rr, float uu) { return c + right * rr + up * uu; };

    const vne::math::Vec3f near_col{0.9f, 0.9f, 0.3f};  // yellow — near plane
    const vne::math::Vec3f far_col{0.3f, 0.8f, 1.0f};   // cyan   — far  plane
    const vne::math::Vec3f edge_col{0.7f, 0.7f, 0.7f};  // gray   — connecting edges

    const float aspect =
        (ui_.last_vp_h > 0) ? (static_cast<float>(ui_.last_vp_w) / static_cast<float>(ui_.last_vp_h)) : 1.f;

    if (ui_.use_perspective) {
        // Tangent of half-FOV gives the slope; multiply by distance to get half-extents.
        const float half_fov_rad = ui_.fov * 0.5f * 3.14159265f / 180.f;
        const float tan_half = std::tan(half_fov_rad);

        const float near_half_h = tan_half * near_dist;
        const float near_half_w = near_half_h * aspect;
        const float far_half_h = tan_half * far_dist;
        const float far_half_w = far_half_h * aspect;

        const vne::math::Vec3f near_center = along(near_dist);
        const vne::math::Vec3f far_center = along(far_dist);

        const vne::math::Vec3f near_tl = corner(near_center, -near_half_w, near_half_h);
        const vne::math::Vec3f near_tr = corner(near_center, near_half_w, near_half_h);
        const vne::math::Vec3f near_bl = corner(near_center, -near_half_w, -near_half_h);
        const vne::math::Vec3f near_br = corner(near_center, near_half_w, -near_half_h);

        const vne::math::Vec3f far_tl = corner(far_center, -far_half_w, far_half_h);
        const vne::math::Vec3f far_tr = corner(far_center, far_half_w, far_half_h);
        const vne::math::Vec3f far_bl = corner(far_center, -far_half_w, -far_half_h);
        const vne::math::Vec3f far_br = corner(far_center, far_half_w, -far_half_h);

        debug_draw_->line(near_tl, near_tr, near_col);
        debug_draw_->line(near_tr, near_br, near_col);
        debug_draw_->line(near_br, near_bl, near_col);
        debug_draw_->line(near_bl, near_tl, near_col);

        debug_draw_->line(far_tl, far_tr, far_col);
        debug_draw_->line(far_tr, far_br, far_col);
        debug_draw_->line(far_br, far_bl, far_col);
        debug_draw_->line(far_bl, far_tl, far_col);

        debug_draw_->line(pos, far_tl, edge_col);
        debug_draw_->line(pos, far_tr, edge_col);
        debug_draw_->line(pos, far_bl, edge_col);
        debug_draw_->line(pos, far_br, edge_col);

        if (ui_.show_near_far_planes) {
            debug_draw_->line(near_tl, near_br, near_col);
            debug_draw_->line(near_tr, near_bl, near_col);
            debug_draw_->line(far_tl, far_br, far_col);
            debug_draw_->line(far_tr, far_bl, far_col);
        }
    } else {
        const float half_w = ui_.ortho_half * aspect;
        const float half_h = ui_.ortho_half;

        const vne::math::Vec3f near_center = along(near_dist);
        const vne::math::Vec3f far_center = along(far_dist);

        const vne::math::Vec3f near_tl = corner(near_center, -half_w, half_h);
        const vne::math::Vec3f near_tr = corner(near_center, half_w, half_h);
        const vne::math::Vec3f near_bl = corner(near_center, -half_w, -half_h);
        const vne::math::Vec3f near_br = corner(near_center, half_w, -half_h);

        const vne::math::Vec3f far_tl = corner(far_center, -half_w, half_h);
        const vne::math::Vec3f far_tr = corner(far_center, half_w, half_h);
        const vne::math::Vec3f far_bl = corner(far_center, -half_w, -half_h);
        const vne::math::Vec3f far_br = corner(far_center, half_w, -half_h);

        debug_draw_->line(near_tl, near_tr, near_col);
        debug_draw_->line(near_tr, near_br, near_col);
        debug_draw_->line(near_br, near_bl, near_col);
        debug_draw_->line(near_bl, near_tl, near_col);

        debug_draw_->line(far_tl, far_tr, far_col);
        debug_draw_->line(far_tr, far_br, far_col);
        debug_draw_->line(far_br, far_bl, far_col);
        debug_draw_->line(far_bl, far_tl, far_col);

        debug_draw_->line(near_tl, far_tl, edge_col);
        debug_draw_->line(near_tr, far_tr, edge_col);
        debug_draw_->line(near_bl, far_bl, edge_col);
        debug_draw_->line(near_br, far_br, edge_col);

        if (ui_.show_near_far_planes) {
            debug_draw_->line(near_tl, near_br, near_col);
            debug_draw_->line(near_tr, near_bl, near_col);
            debug_draw_->line(far_tl, far_br, far_col);
            debug_draw_->line(far_tr, far_bl, far_col);
        }
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
    setRenderSortKey(kRenderSortKey);
}

void SceneSettingsLayer::setImGuiLayer(vne::testbed::ImGuiLayer* layer) {
    imgui_layer_ = layer;
}
void SceneSettingsLayer::setSceneLayer(SceneTestLayer* layer) {
    scene_layer_ = layer;
}
#ifdef VNE_TESTBED_INTERACTION
void SceneSettingsLayer::setInteractionLayer(BaseInteractionLayer* layer) {
    interaction_layer_ = layer;
}
#endif

void SceneSettingsLayer::onAttach(vne::testbed::AppContext& /*app_context*/) {
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
    auto& ui = sl.uiSettings();

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
            const vne::math::Vec3f pos = cam->getPosition();
            const vne::math::Vec3f tgt = cam->getTarget();
            const vne::math::Vec3f up = cam->getUp();
            ui.cam_position[0] = pos.x();
            ui.cam_position[1] = pos.y();
            ui.cam_position[2] = pos.z();
            ui.cam_target[0] = tgt.x();
            ui.cam_target[1] = tgt.y();
            ui.cam_target[2] = tgt.z();
            ui.cam_up[0] = up.x();
            ui.cam_up[1] = up.y();
            ui.cam_up[2] = up.z();
        }
        bool pos_changed = false;
        bool proj_changed = false;
        const char* types[] = {"Perspective", "Orthographic"};
        int type_index = ui.use_perspective ? 0 : 1;
        if (ImGui::Combo("Type", &type_index, types, 2)) {
            ui.use_perspective = (type_index == 0);
            proj_changed = true;
        }
        pos_changed |= ImGui::SliderFloat3("Position", ui.cam_position, -20.f, 20.f);
        pos_changed |= ImGui::SliderFloat3("Target", ui.cam_target, -20.f, 20.f);
        pos_changed |= ImGui::SliderFloat3("Up", ui.cam_up, -1.f, 1.f);
        if (ui.use_perspective) {
            proj_changed |= ImGui::SliderFloat("FOV", &ui.fov, 20.f, 120.f);
            proj_changed |= ImGui::SliderFloat("Near", &ui.near_plane, 0.01f, 10.f);
            proj_changed |= ImGui::SliderFloat("Far", &ui.far_plane, ui.near_plane + 0.01f, 5000.f);
        } else {
            proj_changed |= ImGui::SliderFloat("Half-extent", &ui.ortho_half, 1.f, 20.f);
            proj_changed |= ImGui::SliderFloat("Near##ortho", &ui.ortho_near, -500.f, 500.f);
            proj_changed |= ImGui::SliderFloat("Far##ortho", &ui.ortho_far, -500.f, 500.f);
        }
        if (pos_changed)
            sl.syncCameraPositionTargetUp();
        if (proj_changed) {
            sl.rebuildCamera(ui.last_vp_w, ui.last_vp_h);
#ifdef VNE_TESTBED_INTERACTION
            if (interaction_layer_)
                interaction_layer_->setCameras(sl.getActiveCameras());
#endif
        }
        ImGui::Checkbox("Show camera visuals", &ui.show_camera_visuals);
        if (ui.show_camera_visuals) {
            ImGui::Indent();
            ImGui::Checkbox("Frustum wireframe", &ui.show_frustum);
            ImGui::Checkbox("Camera axes (fwd/right/up)", &ui.show_cam_axes);
            ImGui::Checkbox("Near plane crosshair", &ui.show_near_far_planes);
            ImGui::Unindent();
        }
        ImGui::Checkbox("Show view matrix", &ui.show_view_matrix);
        ImGui::Checkbox("Show projection matrix", &ui.show_projection_matrix);
        if (ui.show_view_matrix) {
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
        if (ui.show_projection_matrix) {
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
        ImGui::SliderInt("Count##cubes", &ui.cube_count, 0, 4);
        ImGui::SliderFloat("Rotation speed##cubes", &ui.cube_rotation_speed, 0.f, 4.f);
        for (int i = 0; i < 4; ++i) {
            ImGui::PushID(i);
            const std::string label = "Cube " + std::to_string(i + 1);
            if (ImGui::CollapsingHeader(label.c_str())) {
                const bool active = (i < ui.cube_count);
                if (!active)
                    ImGui::TextDisabled("(not drawn)");
                else
                    ImGui::SliderFloat3("Position", ui.cube_position[i], -120.f, 120.f);
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
        sync |= ImGui::Checkbox("Enabled##amb", &ui.ambient_light_enabled);
        sync |= ImGui::ColorEdit3("Color##amb", ui.ambient_light_color);
        sync |= ImGui::SliderFloat("Intensity##amb", &ui.ambient_light_intensity, 0.f, 2.f);
        if (sync)
            sl.syncAmbientLight();
    }

    // ---- Directional light ----
    if (ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
        bool sync = false;
        sync |= ImGui::Checkbox("Enabled##dir", &ui.dir_light_enabled);
        sync |= ImGui::SliderFloat3("Direction", ui.dir_light_dir, -1.f, 1.f);
        sync |= ImGui::ColorEdit3("Color##dir", ui.dir_light_color);
        sync |= ImGui::SliderFloat("Intensity##dir", &ui.dir_light_intensity, 0.f, 5.f);
        if (sync)
            sl.syncDirLight();
    }

    // ---- Point lights ----
    if (ImGui::CollapsingHeader("Point Lights", ImGuiTreeNodeFlags_DefaultOpen)) {
        const int npt = static_cast<int>(ui.point_lights.size());
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
            auto& e = ui.point_lights[static_cast<std::size_t>(i)];
            ImGui::PushID(i);
            const std::string point_light_label = "Point Light " + std::to_string(i + 1);
            if (ImGui::TreeNode(point_light_label.c_str())) {
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
        sync |= ImGui::Checkbox("Enabled##spot", &ui.spot_light_enabled);
        sync |= ImGui::SliderFloat3("Position##spot", ui.spot_light_pos, -10.f, 10.f);
        sync |= ImGui::SliderFloat3("Direction##spot", ui.spot_light_dir, -1.f, 1.f);
        sync |= ImGui::ColorEdit3("Color##spot", ui.spot_light_color);
        sync |= ImGui::SliderFloat("Intensity##spot", &ui.spot_light_intensity, 0.f, 10.f);
        sync |= ImGui::SliderFloat("Range##spot", &ui.spot_light_range, 0.5f, 20.f);
        sync |= ImGui::SliderFloat("Inner angle (deg)##spot", &ui.spot_light_inner_deg, 1.f, 89.f);
        sync |= ImGui::SliderFloat("Outer angle (deg)##spot", &ui.spot_light_outer_deg, 1.f, 90.f);
        if (sync)
            sl.syncSpotLight();
    }

    if (ImGui::CollapsingHeader("Attenuation (point/spot)", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Use attenuation formula (const + linear*d + quad*d^2)", &ui.use_attn_formula);
        if (ui.use_attn_formula) {
            ImGui::SliderFloat("Constant##attn", &ui.attn_const, 0.01f, 2.f);
            ImGui::SliderFloat("Linear##attn", &ui.attn_linear, 0.f, 0.5f);
            ImGui::SliderFloat("Quadratic##attn", &ui.attn_quad, 0.f, 0.1f);
        }
    }
}

#endif  // VNE_TESTBED_IMGUI

// ---------------------------------------------------------------------------

void registerTestSceneDemo(vne::testbed::Application& app) {
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

VNETESTBED_REGISTER_DEMO("test_scene", registerTestSceneDemo)

}  // namespace vne::samples::test_scene
