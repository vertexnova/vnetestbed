/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/app/application_event_listener.h"
#include "vertexnova/testbed/app/application.h"

namespace vne {
namespace testbed {

ApplicationEventListener::ApplicationEventListener(Application* application)
    : application_(application) {}

void ApplicationEventListener::onEvent(const vne::events::Event& event) {
    if (application_) {
        application_->onEvent(event);
    }
}

}  // namespace testbed
}  // namespace vne
