#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Application-level event listener; forwards events to Application::onEvent.
 * Mirrors vertexnova samples/core/app ApplicationEventListener_C pattern.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/events/event_listener.h"

namespace vne {
namespace testbed {

class Application;

/**
 * @class ApplicationEventListener
 * @brief Forwards events from EventManager to Application::onEvent.
 *
 * Registered once in Application::initialize() for all event types.
 * Handles common events (ESC, window close) in Application::onEvent,
 * then delegates to layer stack for sample-specific handling.
 */
class ApplicationEventListener : public vne::events::EventListener {
   public:
    explicit ApplicationEventListener(Application* application);

    void onEvent(const vne::events::Event& event) override;

   private:
    Application* application_;
};

}  // namespace testbed
}  // namespace vne
