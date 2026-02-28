/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Sample 04_scene_interaction: perspective camera + orbit-arcball camera
 * manipulator driven by mouse/keyboard via vneevents.
 * Tests: vnescene PerspectiveCamera, vneinteraction CameraSystemController,
 *        vneevents EventManager.
 *
 * Only compiled when vne::interaction is available (VNE_TESTBED_INTERACTION).
 * ----------------------------------------------------------------------
 */

#ifdef VNE_TESTBED_INTERACTION

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/debug_draw.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include "vertexnova/events/event.h"
#include "vertexnova/events/event_listener.h"
#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"

#include "vertexnova/interaction/camera_system_controller.h"
#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/scene_state.h"

#include <memory>

namespace {

// ---------------------------------------------------------------------------
// Non-owning shared_ptr helper
// ---------------------------------------------------------------------------
vne::events::EventManager::ListenerPtr asListenerPtr(vne::events::EventListener* raw) {
    return {raw, [](vne::events::EventListener*) {}};
}

// ---------------------------------------------------------------------------
// SceneDemoLayer — owns the camera; other layers get it via getCamera()
// ---------------------------------------------------------------------------

class SceneDemoLayer : public vne::testbed::ILayer {
   public:
    SceneDemoLayer()
        : vne::testbed::ILayer("SceneDemoLayer") {}

    void onAttach(vne::testbed::AppContext& ctx) override {
        camera_ = std::make_shared<vne::scene::PerspectiveCamera>(60.0f, 1280.0f, 720.0f, 0.1f, 1000.0f, "DemoCamera");
        camera_->setPosition({3.0f, 2.0f, 5.0f});
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
            const float aspect = static_cast<float>(ctx.frame_info.width) / static_cast<float>(ctx.frame_info.height);
            camera_->setAspectRatio(aspect);
            camera_->updateProjectionMatrix();
        }
        debug_draw_->setViewProjectionMatrix(camera_->getViewProjectionMatrix());
        drawGrid();
        drawAxes();
        debug_draw_->flush();
    }

    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> getCamera() const { return camera_; }

   private:
    static constexpr int kGridLines = 20;
    static constexpr float kGridSpacing = 1.0f;
    static constexpr float kGridHalfWidth = kGridLines * kGridSpacing * 0.5f;

    void drawGrid() {
        const vne::math::Vec3f grid_color{0.35f, 0.35f, 0.35f};
        for (int i = -kGridLines; i <= kGridLines; ++i) {
            const float t = static_cast<float>(i) * kGridSpacing;
            debug_draw_->line({t, 0.0f, -kGridHalfWidth}, {t, 0.0f, kGridHalfWidth}, grid_color);
            debug_draw_->line({-kGridHalfWidth, 0.0f, t}, {kGridHalfWidth, 0.0f, t}, grid_color);
        }
    }

    void drawAxes() {
        const float len = kGridHalfWidth;
        debug_draw_->line({0.0f, 0.0f, 0.0f}, {len, 0.0f, 0.0f}, {1.0f, 0.15f, 0.15f});
        debug_draw_->line({0.0f, 0.0f, 0.0f}, {0.0f, len, 0.0f}, {0.15f, 1.0f, 0.15f});
        debug_draw_->line({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, len}, {0.15f, 0.15f, 1.0f});
    }

    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
    vne::scene::SceneState scene_state_;
    vne::testbed::IDebugDraw* debug_draw_{nullptr};
};

// ---------------------------------------------------------------------------
// InteractionDemoLayer — drives the camera with mouse/keyboard
// ---------------------------------------------------------------------------

// Fixed timestep for event-driven controller input (~60Hz).
constexpr double kEventFixedDeltaTime = 0.016;

class InteractionDemoLayer : public vne::testbed::ILayer, public vne::events::EventListener {
   public:
    InteractionDemoLayer()
        : vne::testbed::ILayer("InteractionDemoLayer")
        , controller_(std::make_unique<vne::interaction::CameraSystemController>(
              vne::interaction::CameraManipulatorType::eOrbitArcball)) {}

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera) {
        if (controller_) {
            controller_->setCamera(std::move(camera));
        }
    }

    void onAttach(vne::testbed::AppContext& ctx) override {
        auto& mgr = vne::events::EventManager::instance();
        auto self = asListenerPtr(this);
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
        switch (event.type()) {
            case ET::eMouseMoved: {
                const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                const double dx = first_mouse_ ? 0.0 : (e.x() - last_mouse_x_);
                const double dy = first_mouse_ ? 0.0 : (e.y() - last_mouse_y_);
                last_mouse_x_ = e.x();
                last_mouse_y_ = e.y();
                first_mouse_ = false;
                controller_->handleMouseMove(static_cast<float>(e.x()),
                                             static_cast<float>(e.y()),
                                             static_cast<float>(dx),
                                             static_cast<float>(dy),
                                             kEventFixedDeltaTime);
                break;
            }
            case ET::eMouseButtonPressed: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller_->handleMouseButton(static_cast<int>(e.button()),
                                               true,
                                               static_cast<float>(last_mouse_x_),
                                               static_cast<float>(last_mouse_y_),
                                               kEventFixedDeltaTime);
                break;
            }
            case ET::eMouseButtonReleased: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller_->handleMouseButton(static_cast<int>(e.button()),
                                               false,
                                               static_cast<float>(last_mouse_x_),
                                               static_cast<float>(last_mouse_y_),
                                               kEventFixedDeltaTime);
                break;
            }
            case ET::eMouseScrolled: {
                const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
                controller_->handleMouseScroll(static_cast<float>(e.xOffset()),
                                               static_cast<float>(e.yOffset()),
                                               kEventFixedDeltaTime);
                break;
            }
            case ET::eKeyPressed: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller_->handleKeyboard(static_cast<int>(e.keyCode()), true, kEventFixedDeltaTime);
                break;
            }
            case ET::eKeyReleased: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller_->handleKeyboard(static_cast<int>(e.keyCode()), false, kEventFixedDeltaTime);
                break;
            }
            default:
                break;
        }
    }

   private:
    std::unique_ptr<vne::interaction::CameraSystemController> controller_;
    double last_mouse_x_{0.0};
    double last_mouse_y_{0.0};
    bool first_mouse_{true};
};

// ---------------------------------------------------------------------------

void RegisterSceneInteractionDemo(vne::testbed::Application& app) {
    auto* scene = new SceneDemoLayer();
    app.getLayerStack().pushLayer(std::unique_ptr<SceneDemoLayer>(scene), app.getAppContext());

    auto* interaction = new InteractionDemoLayer();
    interaction->setCamera(scene->getCamera());
    app.getLayerStack().pushLayer(std::unique_ptr<InteractionDemoLayer>(interaction), app.getAppContext());
}

}  // namespace

VNETESTBED_REGISTER_DEMO("scene_interaction", RegisterSceneInteractionDemo)

#endif  // VNE_TESTBED_INTERACTION
