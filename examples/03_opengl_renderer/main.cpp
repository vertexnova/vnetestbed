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
 * @brief Interactive demo: OpenGL renderer + vnescene camera + vneevents.
 *
 * Demonstrates the full vnetestbed stack with the backend-agnostic
 * IRenderDevice interface.  Demo layers include NO gl/ headers.
 *
 *   Swap points:
 *   - Replace GlfwWindow          with a vnecrosswindow adapter
 *   - Replace OpenGLRenderAdapter with CrossGLRenderAdapter
 *   - Replace OpenGLRenderDevice  with CrossGLRenderDevice
 *   - Replace OpenGLDebugDraw     with CrossGLDebugDraw
 */

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/layer_stack.h"
#include "vertexnova/testbed/render_context.h"

// Backend-specific — only in this runner file.
#include "vertexnova/testbed/window/glfw_window.h"
#include "vertexnova/testbed/gl/opengl_render_adapter.h"
#include "vertexnova/testbed/gl/opengl_render_device.h"
#include "vertexnova/testbed/gl/opengl_debug_draw.h"

// Demo layers — no gl/ headers inside.
#include "vertexnova/testbed/plugins/triangle_demo_layer.h"
#include "vertexnova/testbed/plugins/scene_demo_layer.h"
#include "vertexnova/testbed/plugins/events_demo_layer.h"
#ifdef VNE_TESTBED_INTERACTION
#include "vertexnova/testbed/plugins/interaction_demo_layer.h"
#endif

#include "vertexnova/events/event_manager.h"
#include "common/logging_guard.h"

#include <chrono>
#include <memory>

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    vne::testbed::examples::LoggingGuard logging_guard;

#if defined(VNE_TESTBED_OPENGLES)
    const bool use_opengl_es = true;
#else
    const bool use_opengl_es = false;
#endif
    auto glfwWin =
        vne::testbed::window::GlfwWindow::create(1280, 720, "vnetestbed — OpenGL renderer demo", use_opengl_es);
    if (!glfwWin) {
        VNE_LOG_ERROR << "GlfwWindow::create failed";
        return 1;
    }
    glfwWin->setEventForwarding(true);

    // -----------------------------------------------------------------------
    // Backend objects (only these lines change when swapping backends)
    // -----------------------------------------------------------------------

    vne::testbed::gl::OpenGLRenderAdapter renderer;
    vne::testbed::gl::OpenGLRenderDevice device;
    vne::testbed::gl::OpenGLDebugDraw debugDraw;

    if (!renderer.init(glfwWin->getNativeHandle())) {
        VNE_LOG_ERROR << "OpenGLRenderAdapter::init failed";
        return 1;
    }
    if (!debugDraw.init()) {
        VNE_LOG_ERROR << "OpenGLDebugDraw::init failed";
        return 1;
    }

    // -----------------------------------------------------------------------
    // AppContext — purely interface pointers; no backend types visible here
    // -----------------------------------------------------------------------

    vne::testbed::AppContext app_ctx;
    app_ctx.window = glfwWin.get();
    app_ctx.renderer = &renderer;
    app_ctx.device = &device;
    app_ctx.debugDraw = &debugDraw;

    // -----------------------------------------------------------------------
    // Layer stack — layers use only IRenderDevice, IDebugDraw, IWindow
    // -----------------------------------------------------------------------

    vne::testbed::LayerStack layer_stack;

    // Scene layer — owns the camera; must be pushed first.
    auto* scene = new vne::testbed::SceneDemoLayer();
    layer_stack.pushLayer(std::unique_ptr<vne::testbed::SceneDemoLayer>(scene), app_ctx);

#ifdef VNE_TESTBED_INTERACTION
    // Interaction layer — drives the scene camera with mouse/keyboard.
    auto* interaction = new vne::testbed::InteractionDemoLayer();
    interaction->setCamera(scene->getCamera());
    layer_stack.pushLayer(std::unique_ptr<vne::testbed::InteractionDemoLayer>(interaction), app_ctx);
#endif

    // Triangle layer — validates the pipeline end-to-end.
    layer_stack.pushLayer(std::make_unique<vne::testbed::TriangleDemoLayer>(), app_ctx);

    // Events layer — logs input events to stdout.
    layer_stack.pushLayer(std::make_unique<vne::testbed::EventsDemoLayer>(), app_ctx);

    // -----------------------------------------------------------------------
    // Main loop
    // -----------------------------------------------------------------------

    auto prev = std::chrono::steady_clock::now();

    while (!app_ctx.window->shouldClose()) {
        app_ctx.window->pollEvents();
        vne::events::EventManager::instance().processEvents();

        auto now = std::chrono::steady_clock::now();
        const float dt = static_cast<float>(std::chrono::duration<double>(now - prev).count());
        prev = now;

        vne::testbed::RenderContext render_ctx{};
        render_ctx.frame_info.width = app_ctx.window->getWidth();
        render_ctx.frame_info.height = app_ctx.window->getHeight();
        render_ctx.frame_info.dt = dt;
        render_ctx.debug_draw = &debugDraw;

        layer_stack.onUpdate(dt);

        renderer.beginFrame();
        layer_stack.onBeginRender(render_ctx);
        layer_stack.onRender(render_ctx);
        renderer.endFrame();

        glfwWin->swapBuffers();
    }

    // -----------------------------------------------------------------------
    // Cleanup
    // -----------------------------------------------------------------------

    layer_stack.clear();
    device.shutdown();
    debugDraw.shutdown();
    renderer.shutdown();
    glfwWin.reset();
    return 0;
}
