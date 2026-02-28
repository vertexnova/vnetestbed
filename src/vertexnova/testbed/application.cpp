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

#include "vertexnova/testbed/application.h"

#include "vertexnova/testbed/application_descriptor.h"
#include "vertexnova/testbed/demo_application.h"
#include "vertexnova/testbed/logging_guard.h"

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
#include "vertexnova/testbed/gl/opengl_debug_draw.h"
#include "vertexnova/testbed/gl/opengl_render_adapter.h"
#include "vertexnova/testbed/gl/opengl_render_device.h"
#include "vertexnova/testbed/gl/texture2d.h"  // full type for OpenGLRenderDevice destruction
#include "vertexnova/testbed/window/glfw_window.h"
#endif

#if defined(VNE_TESTBED_EVENTS)
#include "vertexnova/events/event_manager.h"
#endif

#include <chrono>

namespace vne {
namespace testbed {

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)

struct Application::Impl {
    std::unique_ptr<IWindow> window;
    std::unique_ptr<gl::OpenGLRenderAdapter> render_adapter;
    std::unique_ptr<gl::OpenGLRenderDevice> render_device;
    std::unique_ptr<gl::OpenGLDebugDraw> debug_draw;
    std::chrono::steady_clock::time_point last_frame_time{std::chrono::steady_clock::now()};
};

Application::~Application() {
    shutdown();
}

bool Application::initialize(const ApplicationDescriptor& descriptor) {
    if (running_) {
        return true;
    }
    if (descriptor.window_backend != WindowBackend::eGLFW || descriptor.render_backend != RenderBackend::eOpenGL) {
        return false;
    }

    auto window = window::GlfwWindow::create(static_cast<int>(descriptor.width),
                                             static_cast<int>(descriptor.height),
                                             descriptor.title.c_str(),
                                             descriptor.use_opengl_es);
    if (!window) {
        return false;
    }
    window->setEventForwarding(true);

    impl_ = std::make_unique<Impl>();
    impl_->window = std::move(window);

    impl_->render_adapter = std::make_unique<gl::OpenGLRenderAdapter>();
    if (!impl_->render_adapter->init(impl_->window->getNativeHandle())) {
        impl_.reset();
        return false;
    }

    impl_->render_device = std::make_unique<gl::OpenGLRenderDevice>();
    impl_->debug_draw = std::make_unique<gl::OpenGLDebugDraw>();
    if (!impl_->debug_draw->init()) {
        impl_->render_adapter->shutdown();
        impl_.reset();
        return false;
    }

    app_ctx_.window = impl_->window.get();
    app_ctx_.renderer = impl_->render_adapter.get();
    app_ctx_.device = impl_->render_device.get();
    app_ctx_.debugDraw = impl_->debug_draw.get();

    running_ = true;
    return true;
}

void Application::run() {
    if (!running_ || !impl_ || !app_ctx_.window || !app_ctx_.renderer) {
        return;
    }

    while (running_ && !app_ctx_.window->shouldClose()) {
        mainLoop();
    }
}

void Application::mainLoop() {
    // Pump OS events first (same order as vertexnova Application_C::MainLoop)
    app_ctx_.window->pollEvents();

#if defined(VNE_TESTBED_EVENTS)
    vne::events::EventManager::instance().processEvents();
#endif

    // Check exit conditions after processing events
    if (!running_ || (app_ctx_.window && app_ctx_.window->shouldClose())) {
        return;
    }

    // Frame timing
    const auto now = std::chrono::steady_clock::now();
    const float dt =
        static_cast<float>(std::chrono::duration<double>(now - impl_->last_frame_time).count());
    impl_->last_frame_time = now;

    RenderContext render_ctx{};
    render_ctx.frame_info.width = app_ctx_.window->getWidth();
    render_ctx.frame_info.height = app_ctx_.window->getHeight();
    render_ctx.frame_info.dt = dt;
    render_ctx.debug_draw = app_ctx_.debugDraw;

    // Layer update and GUI (CPU phase)
    layer_stack_.onUpdate(dt);
    layer_stack_.onGuiBegin(render_ctx);
    layer_stack_.onGuiRender(render_ctx);
    layer_stack_.onGuiEnd(render_ctx);

    // GPU phase: begin frame, layers render, end frame, swap
    app_ctx_.renderer->beginFrame();
    layer_stack_.onBeginRender(render_ctx);
    layer_stack_.onRender(render_ctx);
    app_ctx_.renderer->endFrame();

    impl_->window->swapBuffers();
}

void Application::shutdown() {
    if (!running_) {
        return;
    }
    layer_stack_.clear();
    if (impl_) {
        if (impl_->debug_draw) {
            impl_->debug_draw->shutdown();
        }
        if (impl_->render_device) {
            impl_->render_device->shutdown();
        }
        if (impl_->render_adapter) {
            impl_->render_adapter->shutdown();
        }
        impl_.reset();
    }
    app_ctx_ = AppContext{};
    running_ = false;
}

int runDemoApplication(int argc, char** argv, const ApplicationDescriptor* descriptor) {
    (void)argc;
    (void)argv;
    LoggingGuard guard;
    ApplicationDescriptor desc;
    if (descriptor) {
        desc = *descriptor;
    } else {
        desc.title = "VneTestbed — GLFW OpenGL sample";
        desc.width = 1280;
        desc.height = 720;
        desc.window_backend = WindowBackend::eGLFW;
        desc.render_backend = RenderBackend::eOpenGL;
    }
    DemoApplication app;
    if (!app.initialize(desc)) {
        return 1;
    }
    app.run();
    app.shutdown();
    return 0;
}

#else

struct Application::Impl {};

Application::~Application() {
    shutdown();
}

bool Application::initialize(const ApplicationDescriptor&) {
    return false;
}

void Application::run() {}

void Application::shutdown() {
    layer_stack_.clear();
    app_ctx_ = AppContext{};
    running_ = false;
}

int runDemoApplication(int /*argc*/, char** /*argv*/, const ApplicationDescriptor* /*descriptor*/) {
    return 1;  // No backend in this build
}

#endif

}  // namespace testbed
}  // namespace vne
