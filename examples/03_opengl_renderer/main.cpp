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
 * @file examples/03_opengl_renderer/main.cpp
 * @brief Example: Application + DemoFactory wiring with the OpenGL backend.
 *
 * Demonstrates the full vnetestbed stack via Application::initialize() which
 * internally creates GlfwWindow + OpenGLRenderAdapter + OpenGLRenderDevice +
 * OpenGLDebugDraw.  The demo layer is selected by VNE_DEMO_ID (set by CMake)
 * or the VNE_DEMO environment variable at runtime.
 *
 * Backend swap points are inside Application::initialize() (application.cpp):
 *   - Replace GlfwWindow          with a vnecrosswindow adapter
 *   - Replace OpenGLRenderAdapter with CrossGLRenderAdapter
 *   - Replace OpenGLRenderDevice  with CrossGLRenderDevice
 *   - Replace OpenGLDebugDraw     with CrossGLDebugDraw
 */

#include "vertexnova/testbed/app/application_descriptor.h"
#include "vertexnova/testbed/app/demo_application.h"

#include "common/logging_guard.h"

int main(int argc, char** argv) {
    vne::testbed::examples::LoggingGuard logging_guard;

    vne::testbed::ApplicationDescriptor desc{};
    desc.title = "VneTestbed — OpenGL renderer demo";
    desc.width = 1280;
    desc.height = 720;
    desc.vsync_enabled = true;

    return vne::testbed::runDemoApplication(argc, argv, &desc);
}
