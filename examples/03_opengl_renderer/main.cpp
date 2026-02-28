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
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/logging/logging.h"

#include "common/logging_guard.h"

#include <GLFW/glfw3.h>

#include <chrono>
#include <memory>

CREATE_VNE_LOGGER_CATEGORY("vnetestbed.examples")

// ---------------------------------------------------------------------------
// GlfwWindow — IWindow concrete implementation (runner-local, not in lib)
// ---------------------------------------------------------------------------

namespace vne {
namespace testbed {

class GlfwWindow : public IWindow {
   public:
    explicit GlfwWindow(GLFWwindow* w)
        : window_(w) {}

    int getWidth() const override {
        int w = 0;
        glfwGetWindowSize(window_, &w, nullptr);
        return w;
    }
    int getHeight() const override {
        int h = 0;
        glfwGetWindowSize(window_, nullptr, &h);
        return h;
    }
    void pollEvents() override { glfwPollEvents(); }
    bool shouldClose() const override { return glfwWindowShouldClose(window_) != 0; }
    void* getNativeHandle() const override { return window_; }

   private:
    GLFWwindow* window_;
};

}  // namespace testbed
}  // namespace vne

// ---------------------------------------------------------------------------
// GLFW → vne::events bridge callbacks
// ---------------------------------------------------------------------------

static void onGlfwKey(GLFWwindow* /*w*/, int key, int /*scan*/, int action, int /*mods*/) {
    auto& mgr = vne::events::EventManager::instance();
    if (action == GLFW_PRESS) {
        mgr.pushEvent(std::make_unique<vne::events::KeyPressedEvent>(static_cast<vne::events::KeyCode>(key)));
    } else if (action == GLFW_RELEASE) {
        mgr.pushEvent(std::make_unique<vne::events::KeyReleasedEvent>(static_cast<vne::events::KeyCode>(key)));
    }
}

static void onGlfwMouseButton(GLFWwindow* /*w*/, int button, int action, int /*mods*/) {
    auto& mgr = vne::events::EventManager::instance();
    if (action == GLFW_PRESS) {
        mgr.pushEvent(
            std::make_unique<vne::events::MouseButtonPressedEvent>(static_cast<vne::events::MouseButton>(button)));
    } else {
        mgr.pushEvent(
            std::make_unique<vne::events::MouseButtonReleasedEvent>(static_cast<vne::events::MouseButton>(button)));
    }
}

static void onGlfwCursorPos(GLFWwindow* /*w*/, double x, double y) {
    vne::events::EventManager::instance().pushEvent(std::make_unique<vne::events::MouseMovedEvent>(x, y));
}

static void onGlfwScroll(GLFWwindow* /*w*/, double xoff, double yoff) {
    vne::events::EventManager::instance().pushEvent(std::make_unique<vne::events::MouseScrolledEvent>(xoff, yoff));
}

// ---------------------------------------------------------------------------
// main
// ---------------------------------------------------------------------------

int main() {
    vne::testbed::examples::LoggingGuard logging_guard;

    if (!glfwInit()) {
        VNE_LOG_ERROR << "glfwInit failed";
        return 1;
    }

#if defined(VNE_TESTBED_OPENGLES)
    glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
#else
    // OpenGL 4.1 Core — highest supported on macOS.
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
#endif

    GLFWwindow* window = glfwCreateWindow(1280, 720, "vnetestbed — OpenGL renderer demo", nullptr, nullptr);
    if (!window) {
        VNE_LOG_ERROR << "glfwCreateWindow failed";
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);  // vsync

    // Register GLFW → vne event bridge.
    glfwSetKeyCallback(window, onGlfwKey);
    glfwSetMouseButtonCallback(window, onGlfwMouseButton);
    glfwSetCursorPosCallback(window, onGlfwCursorPos);
    glfwSetScrollCallback(window, onGlfwScroll);

    // -----------------------------------------------------------------------
    // Backend objects (only these lines change when swapping backends)
    // -----------------------------------------------------------------------

    vne::testbed::GlfwWindow glfwWin(window);
    vne::testbed::gl::OpenGLRenderAdapter renderer;
    vne::testbed::gl::OpenGLRenderDevice device;
    vne::testbed::gl::OpenGLDebugDraw debugDraw;

    if (!renderer.init(glfwWin.getNativeHandle())) {
        VNE_LOG_ERROR << "OpenGLRenderAdapter::init failed";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }
    if (!debugDraw.init()) {
        VNE_LOG_ERROR << "OpenGLDebugDraw::init failed";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // -----------------------------------------------------------------------
    // AppContext — purely interface pointers; no backend types visible here
    // -----------------------------------------------------------------------

    vne::testbed::AppContext app_ctx;
    app_ctx.window = &glfwWin;
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

        glfwSwapBuffers(window);
    }

    // -----------------------------------------------------------------------
    // Cleanup
    // -----------------------------------------------------------------------

    layer_stack.clear();
    device.shutdown();
    debugDraw.shutdown();
    renderer.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
