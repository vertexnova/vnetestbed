/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Shared base layer for testbed samples: perspective camera + orbit-arcball
 * interaction + grid + axes drawn via IDebugDraw.
 *
 * Include this header inside an anonymous namespace in each sample .cpp.
 * The layer is intentionally self-contained so each sample binary has no
 * extra link-time dependencies beyond what its own CMakeLists already lists.
 *
 * Requires: vne::testbed, vne::scene, vne::events, vne::interaction
 *           (guards below; omit interaction layer if not available)
 * ----------------------------------------------------------------------
 */

#pragma once

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/debug_draw.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/scene_state.h"

#ifdef VNE_TESTBED_INTERACTION
#include "vertexnova/events/event.h"
#include "vertexnova/events/input/input.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/events/window_event.h"
#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/camera_system_controller.h"
#endif

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#endif

#include <memory>
#include <string>
#include <utility>
#include <vector>

// ---------------------------------------------------------------------------
// BaseSceneLayer — grid + axes + perspective or orthographic camera
// Exposes getCamera() / getCamera(i) / getActiveCameras() for interaction layers.
// Supports per-viewport cameras for 2 or 4 viewport layouts.
// ---------------------------------------------------------------------------
class BaseSceneLayer : public vne::testbed::ILayer {
   public:
    static constexpr int kMaxViewports = 4;
    static constexpr int kGridLines = 20;
    static constexpr float kGridSpacing = 1.0f;
    static constexpr float kGridHalf = kGridLines * kGridSpacing * 0.5f;

    explicit BaseSceneLayer(const char* name = "BaseSceneLayer")
        : vne::testbed::ILayer(name) {}

    void onAttach(vne::testbed::AppContext& ctx) override {
        buildCameras(1280, 720);
        scene_state_.setActiveCamera(use_perspective_
                                         ? std::static_pointer_cast<vne::scene::ICamera>(cameras_[0])
                                         : std::static_pointer_cast<vne::scene::ICamera>(cameras_ortho_[0]));
        debug_draw_ = ctx.debugDraw;
    }

    void onDetach() override {
        debug_draw_ = nullptr;
        scene_state_.setActiveCamera(nullptr);
        cameras_.clear();
        cameras_ortho_.clear();
    }

    void onUpdate(float /*dt*/) override {
        for (auto& cam : cameras_) {
            if (cam)
                cam->updateMatrices();
        }
        for (auto& cam : cameras_ortho_) {
            if (cam)
                cam->updateMatrices();
        }
    }

    void onRender(const vne::testbed::RenderContext& ctx) override {
        const int idx = (ctx.active_viewport_index >= 0 && ctx.active_viewport_index < kMaxViewports)
                            ? ctx.active_viewport_index
                            : 0;
        vne::scene::ICamera* camera = getActiveCamera(idx);
        if (!camera || !debug_draw_) {
            return;
        }
        if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
            last_vp_w_ = ctx.frame_info.width;
            last_vp_h_ = ctx.frame_info.height;
            const float aspect = static_cast<float>(ctx.frame_info.width) / static_cast<float>(ctx.frame_info.height);
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
        if (show_grid_)
            drawGrid();
        if (show_axes_)
            drawAxes();
        debug_draw_->flush();
    }

    /** @brief Active camera for viewport (ICamera for perspective or orthographic). */
    [[nodiscard]] vne::scene::ICamera* getActiveCamera(int index) const {
        const size_t i = static_cast<size_t>(index >= 0 && index < kMaxViewports ? index : 0);
        if (use_perspective_ && i < cameras_.size() && cameras_[i])
            return cameras_[i].get();
        if (!use_perspective_ && i < cameras_ortho_.size() && cameras_ortho_[i])
            return cameras_ortho_[i].get();
        return nullptr;
    }

    /** @brief Active cameras as ICamera for interaction layer. */
    [[nodiscard]] std::vector<std::shared_ptr<vne::scene::ICamera>> getActiveCameras() const {
        std::vector<std::shared_ptr<vne::scene::ICamera>> out;
        if (use_perspective_) {
            for (const auto& c : cameras_)
                if (c)
                    out.push_back(c);
        } else {
            for (const auto& c : cameras_ortho_)
                if (c)
                    out.push_back(c);
        }
        return out;
    }

    [[nodiscard]] std::shared_ptr<vne::scene::ICamera> getCamera(int index) const {
        vne::scene::ICamera* cam = getActiveCamera(index);
        if (!cam)
            return nullptr;
        if (use_perspective_) {
            const size_t i = static_cast<size_t>(index >= 0 && index < kMaxViewports ? index : 0);
            if (i < cameras_.size() && cameras_[i])
                return cameras_[i];
        } else {
            const size_t i = static_cast<size_t>(index >= 0 && index < kMaxViewports ? index : 0);
            if (i < cameras_ortho_.size() && cameras_ortho_[i])
                return cameras_ortho_[i];
        }
        return nullptr;
    }

    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> getPerspectiveCamera(int index) const {
        if (index >= 0 && index < static_cast<int>(cameras_.size()))
            return cameras_[static_cast<size_t>(index)];
        return cameras_.empty() ? nullptr : cameras_[0];
    }

    [[nodiscard]] const std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>>& getCameras() const {
        return cameras_;
    }

    bool show_grid_{true};
    bool show_axes_{true};
    bool use_perspective_{true};
    float fov_{60.0f};
    float near_plane_{0.1f};
    float far_plane_{1000.0f};
    float ortho_half_{6.0f};
    float ortho_near_{-100.0f};
    float ortho_far_{100.0f};
    int last_vp_w_{1280};
    int last_vp_h_{720};
    float cam_position_[3]{4.f, 3.f, 6.f};
    float cam_target_[3]{0.f, 0.f, 0.f};
    float cam_up_[3]{0.f, 1.f, 0.f};

    void setUsePerspective(bool use_persp) {
        if (use_perspective_ == use_persp)
            return;
        syncCameraPositionTargetUp();
        const vne::math::Vec3f pos(cam_position_[0], cam_position_[1], cam_position_[2]);
        const vne::math::Vec3f tgt(cam_target_[0], cam_target_[1], cam_target_[2]);
        const vne::math::Vec3f up(cam_up_[0], cam_up_[1], cam_up_[2]);
        use_perspective_ = use_persp;
        if (use_perspective_) {
            for (auto& c : cameras_)
                if (c) {
                    c->setPosition(pos);
                    c->setTarget(tgt);
                    c->setUp(up);
                    c->updateMatrices();
                }
        } else {
            for (auto& c : cameras_ortho_)
                if (c) {
                    c->setPosition(pos);
                    c->setTarget(tgt);
                    c->setUp(up);
                    c->updateMatrices();
                }
        }
        scene_state_.setActiveCamera(use_perspective_
                                         ? std::static_pointer_cast<vne::scene::ICamera>(cameras_[0])
                                         : std::static_pointer_cast<vne::scene::ICamera>(cameras_ortho_[0]));
    }

    void syncCameraPositionTargetUp() {
        vne::scene::ICamera* active = getActiveCamera(0);
        if (!active)
            return;
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

    void rebuildCameras(int w, int h) {
        buildCameras(w, h);
        scene_state_.setActiveCamera(use_perspective_
                                         ? std::static_pointer_cast<vne::scene::ICamera>(cameras_[0])
                                         : std::static_pointer_cast<vne::scene::ICamera>(cameras_ortho_[0]));
    }

   private:
    void buildCameras(int w, int h) {
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
    void drawGrid() const {
        // Lighter than viewport clear (#4A4A4C) so grid is visible
        const vne::math::Vec3f col{0.50f, 0.50f, 0.52f};
        for (int i = -kGridLines; i <= kGridLines; ++i) {
            const float t = static_cast<float>(i) * kGridSpacing;
            debug_draw_->line({t, 0.0f, -kGridHalf}, {t, 0.0f, kGridHalf}, col);
            debug_draw_->line({-kGridHalf, 0.0f, t}, {kGridHalf, 0.0f, t}, col);
        }
    }

    void drawAxes() const {
        const float len = kGridHalf;
        debug_draw_->line({0.f, 0.f, 0.f}, {len, 0.f, 0.f}, {1.0f, 0.15f, 0.15f});  // +X red
        debug_draw_->line({0.f, 0.f, 0.f}, {0.f, len, 0.f}, {0.15f, 1.0f, 0.15f});  // +Y green
        debug_draw_->line({0.f, 0.f, 0.f}, {0.f, 0.f, len}, {0.15f, 0.15f, 1.0f});  // +Z blue
    }

    std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>> cameras_;
    std::vector<std::shared_ptr<vne::scene::OrthographicCamera>> cameras_ortho_;
    vne::scene::SceneState scene_state_;
    vne::testbed::IDebugDraw* debug_draw_{nullptr};
};

// ---------------------------------------------------------------------------
// BaseInteractionLayer — orbit-arcball driven by EventManager
// Pair with BaseSceneLayer: call setCamera(scene->getCamera()) before attach.
// Only compiled when VNE_TESTBED_INTERACTION is defined.
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_INTERACTION

class BaseInteractionLayer : public vne::testbed::ILayer, public vne::events::EventListener {
   public:
    static constexpr double kFixedDt = 0.016;

    explicit BaseInteractionLayer(const char* name = "BaseInteractionLayer")
        : vne::testbed::ILayer(name) {
        controllers_.resize(BaseSceneLayer::kMaxViewports);
        vne::interaction::CameraManipulatorFactory factory;
        for (size_t i = 0; i < static_cast<size_t>(BaseSceneLayer::kMaxViewports); ++i) {
            auto ctrl = std::make_unique<vne::interaction::CameraSystemController>();
            ctrl->setManipulator(factory.create(vne::interaction::CameraManipulatorType::eOrbit));
            controllers_[i] = std::move(ctrl);
        }
    }

    void setCamera(std::shared_ptr<vne::scene::ICamera> cam) {
        if (!controllers_.empty() && controllers_[0]) {
            controllers_[0]->setCamera(std::move(cam));
        }
    }

    void setSceneLayer(const BaseSceneLayer* scene) {
        if (scene) {
            setCameras(scene->getActiveCameras());
        }
    }

    /** @brief Set per-viewport cameras (for SceneTestLayer or other custom scene layers). */
    void setCameras(const std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>>& cameras) {
        for (size_t i = 0; i < cameras.size() && i < controllers_.size(); ++i) {
            if (controllers_[i] && cameras[i]) {
                controllers_[i]->setCamera(cameras[i]);
            }
        }
    }

    /** @brief Set per-viewport cameras by ICamera (use when switching perspective/orthographic so interaction drives
     * the active camera). */
    void setCameras(const std::vector<std::shared_ptr<vne::scene::ICamera>>& cameras) {
        for (size_t i = 0; i < cameras.size() && i < controllers_.size(); ++i) {
            if (controllers_[i] && cameras[i]) {
                controllers_[i]->setCamera(cameras[i]);
            }
        }
    }

#ifdef VNE_TESTBED_IMGUI
    /** @brief Set ImGuiLayer for viewport-based hit-testing; only process interaction when mouse is over a viewport. */
    void setImGuiLayer(vne::testbed::ImGuiLayer* layer) { imgui_layer_ = layer; }
#endif

    /** @brief Enable or disable orbit-arcball camera interaction. When false, onEvent does not drive the controller. */
    void setInteractionEnabled(bool enabled) {
        interaction_enabled_ = enabled;
        for (auto& ctrl : controllers_) {
            if (ctrl)
                ctrl->setEnabled(enabled);
        }
    }
    /** @brief Whether camera interaction is enabled. */
    [[nodiscard]] bool getInteractionEnabled() const { return interaction_enabled_; }

    void onAttach(vne::testbed::AppContext& ctx) override {
        const float vpw = ctx.window ? static_cast<float>(ctx.window->getWidth()) : 1280.0f;
        const float vph = ctx.window ? static_cast<float>(ctx.window->getHeight()) : 720.0f;
        for (auto& ctrl : controllers_) {
            if (ctrl) {
                ctrl->setViewportSize(vpw, vph);
            }
        }
    }

    void onDetach() override {}

    void onUpdate(float dt) override {
#ifdef VNE_TESTBED_IMGUI
        if (imgui_layer_) {
            const int n = static_cast<int>(controllers_.size());
            for (int i = 0; i < n; ++i) {
                float min_x = 0.0f, min_y = 0.0f, max_x = 0.0f, max_y = 0.0f;
                if (imgui_layer_->getViewportRect(i, min_x, min_y, max_x, max_y)
                    && controllers_[static_cast<size_t>(i)]) {
                    const float vp_w = max_x - min_x;
                    const float vp_h = max_y - min_y;
                    controllers_[static_cast<size_t>(i)]->setViewportSize(vp_w, vp_h);
                }
            }
        }
#endif
        for (auto& ctrl : controllers_) {
            if (ctrl) {
                ctrl->update(static_cast<double>(dt));
            }
        }
    }

    void onEvent(const vne::events::Event& event) override {
        if (!interaction_enabled_ || controllers_.empty() || !controllers_[0]) {
            return;
        }
        using ET = vne::events::EventType;
        // Handle resize early so viewport is updated regardless of mouse position.
        if (event.type() == ET::eWindowResize) {
            const auto& e = static_cast<const vne::events::WindowResizeEvent&>(event);
            const float vpw = static_cast<float>(e.width());
            const float vph = static_cast<float>(e.height());
            for (auto& ctrl : controllers_) {
                if (ctrl) {
                    ctrl->setViewportSize(vpw, vph);
                }
            }
            return;
        }
        double prev_x = last_x_;
        double prev_y = last_y_;
        float check_x = static_cast<float>(last_x_);
        float check_y = static_cast<float>(last_y_);
        if (event.type() == ET::eMouseMoved) {
            const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
            prev_x = last_x_;
            prev_y = last_y_;
            check_x = static_cast<float>(e.x());
            check_y = static_cast<float>(e.y());
        } else if (event.type() == ET::eMouseScrolled || event.type() == ET::eMouseButtonPressed
                   || event.type() == ET::eMouseButtonReleased) {
            const auto [mx, my] = vne::events::Input::mousePosition();
            check_x = static_cast<float>(mx);
            check_y = static_cast<float>(my);
            // Keep last_x_/last_y_ consistent with the position used for this event.
            last_x_ = static_cast<double>(mx);
            last_y_ = static_cast<double>(my);
        }
        int viewport_index = 0;
#ifdef VNE_TESTBED_IMGUI
        if (imgui_layer_) {
            const int idx = imgui_layer_->getHoveredViewportIndex(check_x, check_y);
            if (idx < 0) {
                return;
            }
            viewport_index = idx;
        }
#endif
        auto* controller = (viewport_index >= 0 && viewport_index < static_cast<int>(controllers_.size()))
                               ? controllers_[static_cast<size_t>(viewport_index)].get()
                               : controllers_[0].get();
        if (!controller) {
            return;
        }
        float vp_min_x = 0.0f, vp_min_y = 0.0f, vp_max_x = 0.0f, vp_max_y = 0.0f;
        bool use_viewport_local = false;
#ifdef VNE_TESTBED_IMGUI
        if (imgui_layer_ && imgui_layer_->getViewportRect(viewport_index, vp_min_x, vp_min_y, vp_max_x, vp_max_y)) {
            use_viewport_local = true;
            const float vp_w = vp_max_x - vp_min_x;
            const float vp_h = vp_max_y - vp_min_y;
            controller->setViewportSize(vp_w, vp_h);
        }
#endif
        auto toLocal = [&](float wx, float wy) -> std::pair<float, float> {
            if (use_viewport_local) {
                return {wx - vp_min_x, wy - vp_min_y};
            }
            return {wx, wy};
        };
        switch (event.type()) {
            case ET::eMouseMoved: {
                const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                const double dx = first_mouse_ ? 0.0 : (e.x() - prev_x);
                const double dy = first_mouse_ ? 0.0 : (e.y() - prev_y);
                first_mouse_ = false;
                last_x_ = e.x();
                last_y_ = e.y();
                const auto [lx, ly] = toLocal(static_cast<float>(e.x()), static_cast<float>(e.y()));
                controller->handleMouseMove(lx, ly, static_cast<float>(dx), static_cast<float>(dy), kFixedDt);
                break;
            }
            case ET::eMouseButtonPressed: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                const auto [lx, ly] = toLocal(static_cast<float>(last_x_), static_cast<float>(last_y_));
                controller->handleMouseButton(static_cast<int>(e.button()), true, lx, ly, kFixedDt);
                break;
            }
            case ET::eMouseButtonReleased: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                const auto [lx, ly] = toLocal(static_cast<float>(last_x_), static_cast<float>(last_y_));
                controller->handleMouseButton(static_cast<int>(e.button()), false, lx, ly, kFixedDt);
                break;
            }
            case ET::eMouseScrolled: {
                const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
                const auto [lx, ly] = toLocal(static_cast<float>(last_x_), static_cast<float>(last_y_));
                controller->handleMouseScroll(static_cast<float>(e.xOffset()),
                                              static_cast<float>(e.yOffset()),
                                              lx,
                                              ly,
                                              kFixedDt);
                break;
            }
            case ET::eKeyPressed: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller->handleKeyboard(static_cast<int>(e.keyCode()), true, kFixedDt);
                break;
            }
            case ET::eKeyReleased: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller->handleKeyboard(static_cast<int>(e.keyCode()), false, kFixedDt);
                break;
            }
            default:
                break;
        }
    }

    [[nodiscard]] vne::interaction::CameraSystemController* getController() const {
        return controllers_.empty() ? nullptr : controllers_[0].get();
    }
    [[nodiscard]] vne::interaction::CameraSystemController* getController(int index) const {
        if (index >= 0 && index < static_cast<int>(controllers_.size())) {
            return controllers_[static_cast<size_t>(index)].get();
        }
        return getController();
    }

   private:
    std::vector<std::unique_ptr<vne::interaction::CameraSystemController>> controllers_;
    bool interaction_enabled_{true};
    double last_x_{0.0};
    double last_y_{0.0};
    bool first_mouse_{true};
#ifdef VNE_TESTBED_IMGUI
    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
#endif
};

#endif  // VNE_TESTBED_INTERACTION
