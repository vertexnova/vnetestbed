/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Shared entry point for glfw_opengl samples. Each sample links this
 * and registers one demo via VNETESTBED_REGISTER_DEMO.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/demo_application.h"
#include "vertexnova/testbed/logging_guard.h"

int main(int /*argc*/, char** /*argv*/) {
    vne::testbed::LoggingGuard logging_guard;

    vne::testbed::ApplicationDescriptor desc;
    desc.title = "VneTestbed — GLFW OpenGL sample";
    desc.width = 1280;
    desc.height = 720;
    desc.window_backend = vne::testbed::WindowBackend::GLFW;
    desc.render_backend = vne::testbed::RenderBackend::OpenGL;

    vne::testbed::DemoApplication app;
    if (!app.initialize(desc)) {
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}
