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

#include "base_scene_layer.h"

#include <vertexnova/math/core/core.h>

#include <string>

// ---------------------------------------------------------------------------
// BaseSceneLayer
// ---------------------------------------------------------------------------

BaseSceneLayer::BaseSceneLayer(const char* name)
    : vne::testbed::ILayer(name) {}

void BaseSceneLayer::onAttach(vne::testbed::AppContext& app_context) {
    buildCameras(1280, 720);
    scene_state_.setActiveCamera(use_perspective_ ? std::static_pointer_cast<vne::scene::ICamera>(cameras_[0])
                                                  : std::static_pointer_cast<vne::scene::ICamera>(cameras_ortho_[0]));
    debug_draw_ = app_context.debugDraw;
}

void BaseSceneLayer::onDetach() {
    debug_draw_ = nullptr;
    scene_state_.setActiveCamera(nullptr);
    cameras_.clear();
    cameras_ortho_.clear();
}

void BaseSceneLayer::onUpdate(float /*dt*/) {
    for (auto& cam : cameras_) {
        if (cam) {
            cam->updateMatrices();
        }
    }
    for (auto& cam : cameras_ortho_) {
        if (cam) {
            cam->updateMatrices();
        }
    }
}

void BaseSceneLayer::onRender(const vne::testbed::RenderContext& render_context) {
    const int idx = (render_context.active_viewport_index >= 0 && render_context.active_viewport_index < kMaxViewports)
                        ? render_context.active_viewport_index
                        : 0;
    vne::scene::ICamera* camera = getActiveCamera(idx);
    if (!camera || !debug_draw_) {
        return;
    }
    if (render_context.frame_info.width > 0 && render_context.frame_info.height > 0) {
        last_vp_w_ = render_context.frame_info.width;
        last_vp_h_ = render_context.frame_info.height;
        const float aspect =
            static_cast<float>(render_context.frame_info.width) / static_cast<float>(render_context.frame_info.height);
        if (use_perspective_) {
            if (auto* p = dynamic_cast<vne::scene::PerspectiveCamera*>(camera)) {
                p->setAspectRatio(aspect);
                p->updateProjectionMatrix();
            }
        } else {
            if (auto* o = dynamic_cast<vne::scene::OrthographicCamera*>(camera)) {
                const float hw = ortho_half_ * aspect;
                o->setBounds(-hw, hw, -ortho_half_, ortho_half_, ortho_near_, ortho_far_);
                o->updateProjectionMatrix();
            }
        }
    }
    debug_draw_->setViewProjectionMatrix(camera->getViewProjectionMatrix());
    if (show_grid_) {
        drawGrid();
    }
    if (show_axes_) {
        drawAxes();
    }
    debug_draw_->flush();
}

vne::scene::ICamera* BaseSceneLayer::getActiveCamera(int index) const {
    const size_t i = static_cast<size_t>(index >= 0 && index < kMaxViewports ? index : 0);
    if (use_perspective_ && i < cameras_.size() && cameras_[i]) {
        return cameras_[i].get();
    }
    if (!use_perspective_ && i < cameras_ortho_.size() && cameras_ortho_[i]) {
        return cameras_ortho_[i].get();
    }
    return nullptr;
}

std::vector<std::shared_ptr<vne::scene::ICamera>> BaseSceneLayer::getActiveCameras() const {
    std::vector<std::shared_ptr<vne::scene::ICamera>> out;
    if (use_perspective_) {
        for (const auto& c : cameras_) {
            if (c) {
                out.push_back(c);
            }
        }
    } else {
        for (const auto& c : cameras_ortho_) {
            if (c) {
                out.push_back(c);
            }
        }
    }
    return out;
}

std::shared_ptr<vne::scene::ICamera> BaseSceneLayer::getCamera(int index) const {
    vne::scene::ICamera* cam = getActiveCamera(index);
    if (!cam) {
        return nullptr;
    }
    if (use_perspective_) {
        const size_t i = static_cast<size_t>(index >= 0 && index < kMaxViewports ? index : 0);
        if (i < cameras_.size() && cameras_[i]) {
            return cameras_[i];
        }
    } else {
        const size_t i = static_cast<size_t>(index >= 0 && index < kMaxViewports ? index : 0);
        if (i < cameras_ortho_.size() && cameras_ortho_[i]) {
            return cameras_ortho_[i];
        }
    }
    return nullptr;
}

std::shared_ptr<vne::scene::PerspectiveCamera> BaseSceneLayer::getPerspectiveCamera(int index) const {
    if (index >= 0 && index < static_cast<int>(cameras_.size())) {
        return cameras_[static_cast<size_t>(index)];
    }
    return cameras_.empty() ? nullptr : cameras_[0];
}

const std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>>& BaseSceneLayer::getCameras() const {
    return cameras_;
}

void BaseSceneLayer::setUsePerspective(bool use_persp) {
    if (use_perspective_ == use_persp) {
        return;
    }
    syncCameraPositionTargetUp();
    const vne::math::Vec3f pos(cam_position_[0], cam_position_[1], cam_position_[2]);
    const vne::math::Vec3f tgt(cam_target_[0], cam_target_[1], cam_target_[2]);
    const vne::math::Vec3f up(cam_up_[0], cam_up_[1], cam_up_[2]);
    use_perspective_ = use_persp;
    if (use_perspective_) {
        for (auto& c : cameras_) {
            if (c) {
                c->setPosition(pos);
                c->setTarget(tgt);
                c->setUp(up);
                c->updateMatrices();
            }
        }
    } else {
        for (auto& c : cameras_ortho_) {
            if (c) {
                c->setPosition(pos);
                c->setTarget(tgt);
                c->setUp(up);
                c->updateMatrices();
            }
        }
    }
    scene_state_.setActiveCamera(use_perspective_ ? std::static_pointer_cast<vne::scene::ICamera>(cameras_[0])
                                                  : std::static_pointer_cast<vne::scene::ICamera>(cameras_ortho_[0]));
}

void BaseSceneLayer::syncCameraPositionTargetUp() {
    vne::scene::ICamera* active = getActiveCamera(0);
    if (!active) {
        return;
    }
    cam_position_[0] = active->getPosition().x();
    cam_position_[1] = active->getPosition().y();
    cam_position_[2] = active->getPosition().z();
    cam_target_[0] = active->getTarget().x();
    cam_target_[1] = active->getTarget().y();
    cam_target_[2] = active->getTarget().z();
    cam_up_[0] = active->getUp().x();
    cam_up_[1] = active->getUp().y();
    cam_up_[2] = active->getUp().z();
}

void BaseSceneLayer::rebuildCameras(int w, int h) {
    buildCameras(w, h);
    scene_state_.setActiveCamera(use_perspective_ ? std::static_pointer_cast<vne::scene::ICamera>(cameras_[0])
                                                  : std::static_pointer_cast<vne::scene::ICamera>(cameras_ortho_[0]));
}

void BaseSceneLayer::buildCameras(int w, int h) {
    const float aspect = (h > 0) ? (static_cast<float>(w) / static_cast<float>(h)) : (16.0f / 9.0f);
    cameras_.resize(static_cast<size_t>(kMaxViewports));
    cameras_ortho_.resize(static_cast<size_t>(kMaxViewports));
    const vne::math::Vec3f pos(cam_position_[0], cam_position_[1], cam_position_[2]);
    const vne::math::Vec3f tgt(cam_target_[0], cam_target_[1], cam_target_[2]);
    const vne::math::Vec3f up(cam_up_[0], cam_up_[1], cam_up_[2]);
    for (int i = 0; i < kMaxViewports; ++i) {
        auto persp = std::make_shared<vne::scene::PerspectiveCamera>(fov_,
                                                                     static_cast<float>(w),
                                                                     static_cast<float>(h),
                                                                     near_plane_,
                                                                     far_plane_,
                                                                     "BaseCamera" + std::to_string(i));
        persp->setPosition(pos);
        persp->setTarget(tgt);
        persp->setUp(up);
        persp->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
        persp->updateMatrices();
        cameras_[static_cast<size_t>(i)] = std::move(persp);

        const float hw = ortho_half_ * aspect;
        auto ortho = std::make_shared<vne::scene::OrthographicCamera>(-hw,
                                                                      hw,
                                                                      -ortho_half_,
                                                                      ortho_half_,
                                                                      ortho_near_,
                                                                      ortho_far_,
                                                                      "OrthoCamera" + std::to_string(i));
        ortho->setPosition(pos);
        ortho->setTarget(tgt);
        ortho->setUp(up);
        ortho->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
        ortho->updateMatrices();
        cameras_ortho_[static_cast<size_t>(i)] = std::move(ortho);
    }
}

void BaseSceneLayer::drawGrid() const {
    // Lighter than viewport clear (#4A4A4C) so grid is visible
    const vne::math::Vec3f col{0.50f, 0.50f, 0.52f};
    for (int i = -kGridLines; i <= kGridLines; ++i) {
        const float t = static_cast<float>(i) * kGridSpacing;
        debug_draw_->line({t, 0.0f, -kGridHalf}, {t, 0.0f, kGridHalf}, col);
        debug_draw_->line({-kGridHalf, 0.0f, t}, {kGridHalf, 0.0f, t}, col);
    }
}

void BaseSceneLayer::drawAxes() const {
    const float len = kGridHalf;
    debug_draw_->line({0.f, 0.f, 0.f}, {len, 0.f, 0.f}, {1.0f, 0.15f, 0.15f});  // +X red
    debug_draw_->line({0.f, 0.f, 0.f}, {0.f, len, 0.f}, {0.15f, 1.0f, 0.15f});  // +Y green
    debug_draw_->line({0.f, 0.f, 0.f}, {0.f, 0.f, len}, {0.15f, 0.15f, 1.0f});  // +Z blue
}

// ---------------------------------------------------------------------------
// BaseInteractionLayer
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_INTERACTION

BaseInteractionLayer::BaseInteractionLayer(const char* name)
    : vne::testbed::ILayer(name) {
    controllers_.reserve(static_cast<size_t>(BaseSceneLayer::kMaxViewports));
    for (int i = 0; i < BaseSceneLayer::kMaxViewports; ++i) {
        vne::interaction::InspectController c;
        c.setRotationMode(vne::interaction::OrbitRotationMode::eOrbit);
        controllers_.push_back(std::move(c));
    }
}

void BaseInteractionLayer::setCamera(std::shared_ptr<vne::scene::ICamera> camera) {
    if (!controllers_.empty()) {
        controllers_[0].setCamera(std::move(camera));
    }
}

void BaseInteractionLayer::setSceneLayer(const BaseSceneLayer* scene) {
    if (scene) {
        setCameras(scene->getActiveCameras());
    }
}

void BaseInteractionLayer::setCameras(const std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>>& cameras) {
    for (size_t i = 0; i < cameras.size() && i < controllers_.size(); ++i) {
        if (cameras[i]) {
            controllers_[i].setCamera(cameras[i]);
        }
    }
}

void BaseInteractionLayer::setCameras(const std::vector<std::shared_ptr<vne::scene::ICamera>>& cameras) {
    for (size_t i = 0; i < cameras.size() && i < controllers_.size(); ++i) {
        if (cameras[i]) {
            controllers_[i].setCamera(cameras[i]);
        }
    }
}

#ifdef VNE_TESTBED_IMGUI
void BaseInteractionLayer::setImGuiLayer(vne::testbed::ImGuiLayer* layer) {
    imgui_layer_ = layer;
}
#endif

void BaseInteractionLayer::setInteractionEnabled(bool enabled) {
    interaction_enabled_ = enabled;
}

bool BaseInteractionLayer::getInteractionEnabled() const {
    return interaction_enabled_;
}

void BaseInteractionLayer::onAttach(vne::testbed::AppContext& app_context) {
    const float vpw = app_context.window ? static_cast<float>(app_context.window->getWidth()) : 1280.0f;
    const float vph = app_context.window ? static_cast<float>(app_context.window->getHeight()) : 720.0f;
    for (auto& ctrl : controllers_) {
        ctrl.onResize(vpw, vph);
    }
}

void BaseInteractionLayer::onDetach() {}

void BaseInteractionLayer::onUpdate(float dt) {
#ifdef VNE_TESTBED_IMGUI
    if (imgui_layer_) {
        const int n = static_cast<int>(controllers_.size());
        for (int i = 0; i < n; ++i) {
            float min_x = 0.0f, min_y = 0.0f, max_x = 0.0f, max_y = 0.0f;
            if (imgui_layer_->getViewportRect(i, min_x, min_y, max_x, max_y)) {
                const float vp_w = max_x - min_x;
                const float vp_h = max_y - min_y;
                controllers_[static_cast<size_t>(i)].onResize(vp_w, vp_h);
            }
        }
    }
#endif
    for (auto& ctrl : controllers_) {
        ctrl.onUpdate(static_cast<double>(dt));
    }
}

void BaseInteractionLayer::onEvent(const vne::events::Event& event) {
    if (!interaction_enabled_ || controllers_.empty()) {
        return;
    }
    using ET = vne::events::EventType;
    if (event.type() == ET::eWindowResize) {
        const auto& e = static_cast<const vne::events::WindowResizeEvent&>(event);
        const float vpw = static_cast<float>(e.width());
        const float vph = static_cast<float>(e.height());
        for (auto& ctrl : controllers_) {
            ctrl.onResize(vpw, vph);
        }
        return;
    }
    float check_x = static_cast<float>(last_x_);
    float check_y = static_cast<float>(last_y_);
    if (event.type() == ET::eMouseMoved) {
        const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
        check_x = static_cast<float>(e.x());
        check_y = static_cast<float>(e.y());
    }
    int viewport_index = 0;
#ifdef VNE_TESTBED_IMGUI
    if (imgui_layer_) {
        const int idx = imgui_layer_->getHoveredViewportIndex(check_x, check_y);
        if (idx < 0) {
            return;
        }
        viewport_index = idx;
        float vp_min_x = 0.0f, vp_min_y = 0.0f, vp_max_x = 0.0f, vp_max_y = 0.0f;
        if (imgui_layer_->getViewportRect(viewport_index, vp_min_x, vp_min_y, vp_max_x, vp_max_y)) {
            const size_t vi = static_cast<size_t>(viewport_index);
            if (vi < controllers_.size()) {
                controllers_[vi].onResize(vp_max_x - vp_min_x, vp_max_y - vp_min_y);
            }
        }
    }
#endif
    const size_t vi = static_cast<size_t>(viewport_index);
    if (vi < controllers_.size()) {
        controllers_[vi].onEvent(event, kFixedDt);
    }
    if (event.type() == ET::eMouseMoved) {
        const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
        last_x_ = e.x();
        last_y_ = e.y();
        first_mouse_ = false;
    }
}

vne::interaction::InspectController* BaseInteractionLayer::getInspectController() {
    return controllers_.empty() ? nullptr : &controllers_[0];
}

vne::interaction::InspectController* BaseInteractionLayer::getInspectController(int index) {
    if (index >= 0 && index < static_cast<int>(controllers_.size())) {
        return &controllers_[static_cast<size_t>(index)];
    }
    return getInspectController();
}

#endif  // VNE_TESTBED_INTERACTION
