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

#include "vertexnova/testbed/plugins/interaction_demo_layer.h"

#include "vertexnova/testbed/app_context.h"

#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/types.h"

namespace vne {
namespace testbed {

InteractionDemoLayer::InteractionDemoLayer()
    : ILayer("InteractionDemoLayer")
    , controller_(std::make_unique<vne::interaction::CameraSystemController>(
          vne::interaction::CameraManipulatorType::eOrbitArcball)) {}

void InteractionDemoLayer::setCamera(std::shared_ptr<vne::scene::ICamera> camera) {
    if (controller_) {
        controller_->setCamera(std::move(camera));
    }
}

void InteractionDemoLayer::onAttach(AppContext& ctx) {
    // Register for mouse/keyboard events from EventManager.
    auto& mgr = vne::events::EventManager::instance();
    mgr.registerListener(vne::events::EventType::eMouseMoved, this);
    mgr.registerListener(vne::events::EventType::eMouseButtonPressed, this);
    mgr.registerListener(vne::events::EventType::eMouseButtonReleased, this);
    mgr.registerListener(vne::events::EventType::eMouseScrolled, this);
    mgr.registerListener(vne::events::EventType::eKeyPressed, this);
    mgr.registerListener(vne::events::EventType::eKeyReleased, this);

    if (ctx.window && controller_) {
        controller_->setViewportSize(static_cast<float>(ctx.window->getWidth()),
                                     static_cast<float>(ctx.window->getHeight()));
    }
}

void InteractionDemoLayer::onDetach() {
    auto& mgr = vne::events::EventManager::instance();
    mgr.unregisterListener(vne::events::EventType::eMouseMoved, this);
    mgr.unregisterListener(vne::events::EventType::eMouseButtonPressed, this);
    mgr.unregisterListener(vne::events::EventType::eMouseButtonReleased, this);
    mgr.unregisterListener(vne::events::EventType::eMouseScrolled, this);
    mgr.unregisterListener(vne::events::EventType::eKeyPressed, this);
    mgr.unregisterListener(vne::events::EventType::eKeyReleased, this);
}

void InteractionDemoLayer::onUpdate(float dt) {
    if (controller_) {
        controller_->update(static_cast<double>(dt));
    }
}

void InteractionDemoLayer::onEvent(const vne::events::Event& event) {
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
                                         0.016);
            break;
        }
        case ET::eMouseButtonPressed: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            controller_->handleMouseButton(static_cast<int>(e.button()),
                                           true,
                                           static_cast<float>(last_mouse_x_),
                                           static_cast<float>(last_mouse_y_),
                                           0.016);
            break;
        }
        case ET::eMouseButtonReleased: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            controller_->handleMouseButton(static_cast<int>(e.button()),
                                           false,
                                           static_cast<float>(last_mouse_x_),
                                           static_cast<float>(last_mouse_y_),
                                           0.016);
            break;
        }
        case ET::eMouseScrolled: {
            const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
            controller_->handleMouseScroll(static_cast<float>(e.xOffset()), static_cast<float>(e.yOffset()), 0.016);
            break;
        }
        case ET::eKeyPressed: {
            const auto& e = static_cast<const vne::events::KeyEvent&>(event);
            controller_->handleKeyboard(static_cast<int>(e.keyCode()), true, 0.016);
            break;
        }
        case ET::eKeyReleased: {
            const auto& e = static_cast<const vne::events::KeyEvent&>(event);
            controller_->handleKeyboard(static_cast<int>(e.keyCode()), false, 0.016);
            break;
        }
        default:
            break;
    }
}

}  // namespace testbed
}  // namespace vne
