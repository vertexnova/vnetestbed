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

#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/scene_state.h"

#ifdef VNE_TESTBED_INTERACTION
#include "vertexnova/events/event.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/interaction/camera_system_controller.h"
#endif

#ifdef VNE_TESTBED_IMGUI
namespace vne {
namespace testbed {
class ImGuiLayer;
}
}  // namespace vne
#endif

#include <memory>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// BaseSceneLayer — grid + axes + perspective camera
// Exposes getCamera() / getCamera(i) so interaction layers can share cameras.
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
        cameras_.resize(kMaxViewports);
        for (size_t i = 0; i < static_cast<size_t>(kMaxViewports); ++i) {
            auto cam = std::make_shared<vne::scene::PerspectiveCamera>(60.0f,
                                                                       1280.0f,
                                                                       720.0f,
                                                                       0.1f,
                                                                       1000.0f,
                                                                       std::string("BaseCamera") + std::to_string(i));
            cam->setPosition({4.0f, 3.0f, 6.0f});
            cam->setTarget({0.0f, 0.0f, 0.0f});
            cam->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
            cam->updateMatrices();
            cameras_[i] = std::move(cam);
        }
        scene_state_.setActiveCamera(cameras_[0]);
        debug_draw_ = ctx.debugDraw;
    }

    void onDetach() override {
        debug_draw_ = nullptr;
        scene_state_.setActiveCamera(nullptr);
        cameras_.clear();
    }

    void onUpdate(float /*dt*/) override {
        for (auto& cam : cameras_) {
            if (cam) {
                cam->updateMatrices();
            }
        }
    }

    void onRender(const vne::testbed::RenderContext& ctx) override {
        const int idx =
            (ctx.active_viewport_index >= 0 && ctx.active_viewport_index < static_cast<int>(cameras_.size()))
                ? ctx.active_viewport_index
                : 0;
        auto& camera = cameras_[static_cast<size_t>(idx)];
        if (!camera || !debug_draw_) {
            return;
        }
        if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
            camera->setAspectRatio(static_cast<float>(ctx.frame_info.width)
                                   / static_cast<float>(ctx.frame_info.height));
            camera->updateProjectionMatrix();
        }
        debug_draw_->setViewProjectionMatrix(camera->getViewProjectionMatrix());
        drawGrid();
        drawAxes();
        debug_draw_->flush();
    }

    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> getCamera() const {
        return cameras_.empty() ? nullptr : cameras_[0];
    }
    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> getCamera(int index) const {
        if (index >= 0 && index < static_cast<int>(cameras_.size())) {
            return cameras_[static_cast<size_t>(index)];
        }
        return getCamera();
    }
    [[nodiscard]] const std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>>& getCameras() const {
        return cameras_;
    }

   private:
    void drawGrid() const {
        const vne::math::Vec3f col{0.30f, 0.30f, 0.30f};
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
        for (size_t i = 0; i < static_cast<size_t>(BaseSceneLayer::kMaxViewports); ++i) {
            controllers_[i] = std::make_unique<vne::interaction::CameraSystemController>(
                vne::interaction::CameraManipulatorType::eOrbitArcball);
        }
    }

    void setCamera(std::shared_ptr<vne::scene::ICamera> cam) {
        if (!controllers_.empty() && controllers_[0]) {
            controllers_[0]->setCamera(std::move(cam));
        }
    }

    void setSceneLayer(const BaseSceneLayer* scene) {
        if (scene) {
            setCameras(scene->getCameras());
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

#ifdef VNE_TESTBED_IMGUI
    /** @brief Set ImGuiLayer for viewport-based hit-testing; only process interaction when mouse is over a viewport. */
    void setImGuiLayer(vne::testbed::ImGuiLayer* layer) { imgui_layer_ = layer; }
#endif

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
        for (auto& ctrl : controllers_) {
            if (ctrl) {
                ctrl->update(static_cast<double>(dt));
            }
        }
    }

    void onEvent(const vne::events::Event& event) override {
        if (controllers_.empty() || !controllers_[0]) {
            return;
        }
        using ET = vne::events::EventType;
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
        }
#endif
        auto* controller = (viewport_index >= 0 && viewport_index < static_cast<int>(controllers_.size()))
                               ? controllers_[static_cast<size_t>(viewport_index)].get()
                               : controllers_[0].get();
        if (!controller) {
            return;
        }
        switch (event.type()) {
            case ET::eMouseMoved: {
                const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                const double dx = first_mouse_ ? 0.0 : (e.x() - last_x_);
                const double dy = first_mouse_ ? 0.0 : (e.y() - last_y_);
                last_x_ = e.x();
                last_y_ = e.y();
                first_mouse_ = false;
                controller->handleMouseMove(static_cast<float>(e.x()),
                                            static_cast<float>(e.y()),
                                            static_cast<float>(dx),
                                            static_cast<float>(dy),
                                            kFixedDt);
                break;
            }
            case ET::eMouseButtonPressed: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller->handleMouseButton(static_cast<int>(e.button()),
                                              true,
                                              static_cast<float>(last_x_),
                                              static_cast<float>(last_y_),
                                              kFixedDt);
                break;
            }
            case ET::eMouseButtonReleased: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller->handleMouseButton(static_cast<int>(e.button()),
                                              false,
                                              static_cast<float>(last_x_),
                                              static_cast<float>(last_y_),
                                              kFixedDt);
                break;
            }
            case ET::eMouseScrolled: {
                const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
                controller->handleMouseScroll(static_cast<float>(e.xOffset()),
                                              static_cast<float>(e.yOffset()),
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
    double last_x_{0.0};
    double last_y_{0.0};
    bool first_mouse_{true};
#ifdef VNE_TESTBED_IMGUI
    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
#endif
};

#endif  // VNE_TESTBED_INTERACTION
