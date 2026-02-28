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

#include "vertexnova/testbed/plugins/events_demo_layer.h"

#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/types.h"
#include "vertexnova/logging/logging.h"

namespace { 
CREATE_VNE_LOGGER_CATEGORY("vnetestbed.plugins")
}  // namespace

namespace vne {
namespace testbed {

namespace {
// EventManager::registerListener requires shared_ptr<EventListener>.
// The layer's lifetime is managed by LayerStack (unique_ptr), so we create
// a non-owning shared_ptr with a no-op deleter.  It is safe because
// onDetach() always unregisters before the object is destroyed.
vne::events::EventManager::ListenerPtr asListenerPtr(vne::events::EventListener* raw) {
    return {raw, [](vne::events::EventListener*) {}};
}
}  // namespace

void EventsDemoLayer::onAttach(AppContext& /*ctx*/) {
    auto& mgr = vne::events::EventManager::instance();
    auto self = asListenerPtr(this);
    mgr.registerListener(vne::events::EventType::eKeyPressed, self);
    mgr.registerListener(vne::events::EventType::eKeyReleased, self);
    mgr.registerListener(vne::events::EventType::eMouseButtonPressed, self);
    mgr.registerListener(vne::events::EventType::eMouseButtonReleased, self);
    mgr.registerListener(vne::events::EventType::eMouseMoved, self);
    mgr.registerListener(vne::events::EventType::eMouseScrolled, self);
    mgr.registerListener(vne::events::EventType::eWindowResize, self);
}

void EventsDemoLayer::onDetach() {
    auto& mgr = vne::events::EventManager::instance();
    mgr.unregisterListener(vne::events::EventType::eKeyPressed, this);
    mgr.unregisterListener(vne::events::EventType::eKeyReleased, this);
    mgr.unregisterListener(vne::events::EventType::eMouseButtonPressed, this);
    mgr.unregisterListener(vne::events::EventType::eMouseButtonReleased, this);
    mgr.unregisterListener(vne::events::EventType::eMouseMoved, this);
    mgr.unregisterListener(vne::events::EventType::eMouseScrolled, this);
    mgr.unregisterListener(vne::events::EventType::eWindowResize, this);
    recent_events_.clear();
}

void EventsDemoLayer::onEvent(const vne::events::Event& event) {
    const std::string desc = event.toString();
    VNE_LOG_INFO << desc;

    recent_events_.push_back(desc);
    if (recent_events_.size() > kMaxEvents) {
        recent_events_.pop_front();
    }
}

}  // namespace testbed
}  // namespace vne
