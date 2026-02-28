/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Sample 02_events: logs input events via vneevents.
 * Tests: vneevents EventManager, EventListener, event dispatch.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/layer.h"

#include "vertexnova/events/event.h"
#include "vertexnova/events/event_listener.h"
#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/types.h"
#include "vertexnova/logging/logging.h"

#include <deque>
#include <memory>
#include <string>

namespace {
CREATE_VNE_LOGGER_CATEGORY("vnetestbed.samples")
}  // namespace

namespace {

// ---------------------------------------------------------------------------
// Non-owning shared_ptr helper (LayerStack owns the layer via unique_ptr;
// EventManager needs a shared_ptr<EventListener>).
// Safe because onDetach() always unregisters before the object is destroyed.
// ---------------------------------------------------------------------------
vne::events::EventManager::ListenerPtr asListenerPtr(vne::events::EventListener* raw) {
    return {raw, [](vne::events::EventListener*) {}};
}

// ---------------------------------------------------------------------------
// EventsDemoLayer
// ---------------------------------------------------------------------------

class EventsDemoLayer : public vne::testbed::ILayer, public vne::events::EventListener {
   public:
    static constexpr std::size_t kMaxEvents = 12u;

    EventsDemoLayer()
        : vne::testbed::ILayer("EventsDemoLayer") {}

    void onAttach(vne::testbed::AppContext& /*ctx*/) override {
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

    void onDetach() override {
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

    void onEvent(const vne::events::Event& event) override {
        const std::string desc = event.toString();
        VNE_LOG_INFO << desc;
        recent_events_.push_back(desc);
        if (recent_events_.size() > kMaxEvents) {
            recent_events_.pop_front();
        }
    }

   private:
    std::deque<std::string> recent_events_;
};

// ---------------------------------------------------------------------------

void RegisterEventsDemo(vne::testbed::Application& app) {
    app.getLayerStack().pushLayer(std::make_unique<EventsDemoLayer>(), app.getAppContext());
}

}  // namespace

VNETESTBED_REGISTER_DEMO("events", RegisterEventsDemo)
