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
#include "vertexnova/events/event_listener.h"
#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/interaction/camera_system_controller.h"
#endif

#ifdef VNE_TESTBED_IMGUI
namespace vne { namespace testbed { class ImGuiLayer; } }
#endif

#include <memory>

#ifdef VNE_TESTBED_INTERACTION
// ---------------------------------------------------------------------------
// Non-owning shared_ptr helper (same pattern as 02_events / 04_scene_interaction)
// ---------------------------------------------------------------------------
inline vne::events::EventManager::ListenerPtr asBaseListenerPtr(vne::events::EventListener* raw) {
    return {raw, [](vne::events::EventListener*) {}};
}
#endif

// ---------------------------------------------------------------------------
// BaseSceneLayer — grid + axes + perspective camera
// Exposes getCamera() so interaction layers (or scene layers) can share it.
// ---------------------------------------------------------------------------
class BaseSceneLayer : public vne::testbed::ILayer {
   public:
    static constexpr int kGridLines = 20;
    static constexpr float kGridSpacing = 1.0f;
    static constexpr float kGridHalf = kGridLines * kGridSpacing * 0.5f;

    explicit BaseSceneLayer(const char* name = "BaseSceneLayer")
        : vne::testbed::ILayer(name) {}

    void onAttach(vne::testbed::AppContext& ctx) override {
        camera_ = std::make_shared<vne::scene::PerspectiveCamera>(60.0f, 1280.0f, 720.0f, 0.1f, 1000.0f, "BaseCamera");
        camera_->setPosition({4.0f, 3.0f, 6.0f});
        camera_->setTarget({0.0f, 0.0f, 0.0f});
        camera_->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
        camera_->updateMatrices();
        scene_state_.setActiveCamera(camera_);
        debug_draw_ = ctx.debugDraw;
    }

    void onDetach() override {
        debug_draw_ = nullptr;
        scene_state_.setActiveCamera(nullptr);
        camera_.reset();
    }

    void onUpdate(float /*dt*/) override {
        if (camera_) {
            camera_->updateMatrices();
        }
    }

    void onRender(const vne::testbed::RenderContext& ctx) override {
        if (!camera_ || !debug_draw_) {
            return;
        }
        if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
            camera_->setAspectRatio(static_cast<float>(ctx.frame_info.width)
                                    / static_cast<float>(ctx.frame_info.height));
            camera_->updateProjectionMatrix();
        }
        debug_draw_->setViewProjectionMatrix(camera_->getViewProjectionMatrix());
        drawGrid();
        drawAxes();
        debug_draw_->flush();
    }

    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> getCamera() const { return camera_; }

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

    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
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
        : vne::testbed::ILayer(name)
        , controller_(std::make_unique<vne::interaction::CameraSystemController>(
              vne::interaction::CameraManipulatorType::eOrbitArcball)) {}

    void setCamera(std::shared_ptr<vne::scene::ICamera> cam) {
        if (controller_) {
            controller_->setCamera(std::move(cam));
        }
    }

#ifdef VNE_TESTBED_IMGUI
    /** @brief Set ImGuiLayer for viewport-based hit-testing; only process interaction when mouse is over a viewport. */
    void setImGuiLayer(vne::testbed::ImGuiLayer* layer) { imgui_layer_ = layer; }
#endif

    void onAttach(vne::testbed::AppContext& ctx) override {
        auto& mgr = vne::events::EventManager::instance();
        auto self = asBaseListenerPtr(this);
        mgr.registerListener(vne::events::EventType::eMouseMoved, self);
        mgr.registerListener(vne::events::EventType::eMouseButtonPressed, self);
        mgr.registerListener(vne::events::EventType::eMouseButtonReleased, self);
        mgr.registerListener(vne::events::EventType::eMouseScrolled, self);
        mgr.registerListener(vne::events::EventType::eKeyPressed, self);
        mgr.registerListener(vne::events::EventType::eKeyReleased, self);
        if (ctx.window && controller_) {
            controller_->setViewportSize(static_cast<float>(ctx.window->getWidth()),
                                         static_cast<float>(ctx.window->getHeight()));
        }
    }

    void onDetach() override {
        auto& mgr = vne::events::EventManager::instance();
        mgr.unregisterListener(vne::events::EventType::eMouseMoved, this);
        mgr.unregisterListener(vne::events::EventType::eMouseButtonPressed, this);
        mgr.unregisterListener(vne::events::EventType::eMouseButtonReleased, this);
        mgr.unregisterListener(vne::events::EventType::eMouseScrolled, this);
        mgr.unregisterListener(vne::events::EventType::eKeyPressed, this);
        mgr.unregisterListener(vne::events::EventType::eKeyReleased, this);
    }

    void onUpdate(float dt) override {
        if (controller_) {
            controller_->update(static_cast<double>(dt));
        }
    }

    void onEvent(const vne::events::Event& event) override {
        if (!controller_) {
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
#ifdef VNE_TESTBED_IMGUI
        if (imgui_layer_ && !imgui_layer_->isMouseOverSceneViewport(check_x, check_y)) {
            return;
        }
#endif
        switch (event.type()) {
            case ET::eMouseMoved: {
                const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                const double dx = first_mouse_ ? 0.0 : (e.x() - last_x_);
                const double dy = first_mouse_ ? 0.0 : (e.y() - last_y_);
                last_x_ = e.x();
                last_y_ = e.y();
                first_mouse_ = false;
                controller_->handleMouseMove(static_cast<float>(e.x()),
                                             static_cast<float>(e.y()),
                                             static_cast<float>(dx),
                                             static_cast<float>(dy),
                                             kFixedDt);
                break;
            }
            case ET::eMouseButtonPressed: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller_->handleMouseButton(static_cast<int>(e.button()),
                                               true,
                                               static_cast<float>(last_x_),
                                               static_cast<float>(last_y_),
                                               kFixedDt);
                break;
            }
            case ET::eMouseButtonReleased: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller_->handleMouseButton(static_cast<int>(e.button()),
                                               false,
                                               static_cast<float>(last_x_),
                                               static_cast<float>(last_y_),
                                               kFixedDt);
                break;
            }
            case ET::eMouseScrolled: {
                const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
                controller_->handleMouseScroll(static_cast<float>(e.xOffset()),
                                               static_cast<float>(e.yOffset()),
                                               kFixedDt);
                break;
            }
            case ET::eKeyPressed: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller_->handleKeyboard(static_cast<int>(e.keyCode()), true, kFixedDt);
                break;
            }
            case ET::eKeyReleased: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller_->handleKeyboard(static_cast<int>(e.keyCode()), false, kFixedDt);
                break;
            }
            default:
                break;
        }
    }

    [[nodiscard]] vne::interaction::CameraSystemController* getController() const { return controller_.get(); }

   private:
    std::unique_ptr<vne::interaction::CameraSystemController> controller_;
    double last_x_{0.0};
    double last_y_{0.0};
    bool first_mouse_{true};
#ifdef VNE_TESTBED_IMGUI
    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
#endif
};

#endif  // VNE_TESTBED_INTERACTION
