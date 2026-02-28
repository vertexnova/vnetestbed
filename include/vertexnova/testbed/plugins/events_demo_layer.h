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
 * @file plugins/events_demo_layer.h
 * @brief Demo layer integrating vneevents: logs and displays input events.
 *
 * Implements vne::events::EventListener to receive events from
 * vne::events::EventManager.  The runner is responsible for converting GLFW
 * callbacks into vne events and pushing them to the EventManager.
 *
 * Displays the last N event strings via IDebugDraw::text() (no-op in the
 * current OpenGL debug draw; will be useful once ImGui is integrated).
 * Also prints each event to stdout for immediate feedback.
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/events/event_listener.h"
#include "vertexnova/events/event.h"

#include <deque>
#include <string>

namespace vne {
namespace testbed {

/**
 * @class EventsDemoLayer
 * @brief ILayer + EventListener that records and displays recent input events.
 *
 * Registers itself with EventManager on onAttach() and unregisters on onDetach().
 * Stores the last kMaxEvents event description strings for display.
 */
class EventsDemoLayer : public ILayer, public vne::events::EventListener {
   public:
    static constexpr std::size_t kMaxEvents = 12u;

    EventsDemoLayer()
        : ILayer("EventsDemoLayer") {}

    void onAttach(AppContext& ctx) override;
    void onDetach() override;

    /** @brief Called by EventManager when a subscribed event is dispatched. */
    void onEvent(const vne::events::Event& event) override;

   private:
    std::deque<std::string> recent_events_;
};

}  // namespace testbed
}  // namespace vne
