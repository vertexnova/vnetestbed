#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file plugins/interaction_demo_layer.h
 * @brief Demo layer integrating vneinteraction: orbit camera with mouse drag.
 *
 * Wraps a vneinteraction::CameraSystemController and routes vne events to it.
 *
 * Usage in a runner:
 * @code
 *   auto* scene       = new SceneDemoLayer();
 *   auto* interaction = new InteractionDemoLayer();
 *   stack.pushLayer(unique_ptr(scene),       ctx);
 *   interaction->setCamera(scene->getCamera());   // wire before push
 *   stack.pushLayer(unique_ptr(interaction), ctx);
 * @endcode
 *
 * Event routing (via EventManager — runner must push vne events from GLFW):
 *   - MouseMovedEvent    → handleMouseMove()
 *   - MouseButtonEvent   → handleMouseButton()
 *   - MouseScrolledEvent → handleMouseScroll()
 *   - KeyPressedEvent    → handleKeyboard()
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/events/event_listener.h"
#include "vertexnova/events/event.h"

#include "vertexnova/interaction/camera_system_controller.h"
#include "vertexnova/scene/camera/camera.h"

#include <memory>

namespace vne {
namespace testbed {

/**
 * @class InteractionDemoLayer
 * @brief ILayer that drives a CameraSystemController with mouse/keyboard input.
 *
 * Call setCamera() before pushing to connect to a SceneDemoLayer's camera.
 */
class InteractionDemoLayer : public ILayer, public vne::events::EventListener {
   public:
    InteractionDemoLayer();

    /**
     * @brief Bind a camera for the controller to drive.
     *
     * Must be called before onAttach() (i.e. before pushLayer()).
     */
    void setCamera(std::shared_ptr<vne::scene::ICamera> camera);

    void onAttach(AppContext& ctx) override;
    void onDetach() override;
    void onUpdate(float dt) override;

    /** @brief Receives vne mouse/keyboard events and forwards to the controller. */
    void onEvent(const vne::events::Event& event) override;

   private:
    std::unique_ptr<vne::interaction::CameraSystemController> controller_;

    // Last known mouse position, for computing deltas in MouseMovedEvent.
    double last_mouse_x_{0.0};
    double last_mouse_y_{0.0};
    bool first_mouse_{true};
};

}  // namespace testbed
}  // namespace vne
