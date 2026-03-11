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

#include "vertexnova/testbed/app/application.h"

#include "vertexnova/testbed/app/application_descriptor.h"
#include "vertexnova/testbed/app/application_event_listener.h"
#include "vertexnova/testbed/app/demo_application.h"
#include "vertexnova/testbed/logging_guard.h"

#if defined(VNE_TESTBED_EVENTS)
#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/events/window_event.h"
#endif

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
#include "vertexnova/testbed/renderer/core_renderer.h"
#include "vertexnova/testbed/renderer/debug_renderer.h"
#include "vertexnova/testbed/renderer/mesh_renderer.h"
#include "vertexnova/testbed/gl/opengl_render_adapter.h"
#include "vertexnova/testbed/gl/opengl_render_device.h"
#include "vertexnova/testbed/gl/texture2d.h"  // complete type for TextureSlot destruction when impl_ is destroyed in this TU
#include "vertexnova/testbed/gl/index_buffer.h"  // complete type for OpenGLRenderDevice::BufferSlot
#include "vertexnova/testbed/gl/vertex_array.h"   // complete type for DebugRenderer member
#include "vertexnova/testbed/gl/vertex_buffer.h" // complete type for DebugRenderer member
#include "vertexnova/testbed/gl/shader.h"         // complete type for DebugRenderer member
#include "vertexnova/testbed/window/glfw_window.h"
#include "vertexnova/testbed/window/glfw_window_descriptor.h"
#endif

#include <chrono>

namespace vne {
namespace testbed {

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)

struct Application::Impl {
    std::unique_ptr<IWindow> window;
    std::unique_ptr<gl::OpenGLRenderAdapter> render_adapter;
    std::unique_ptr<gl::OpenGLRenderDevice> render_device;
    std::unique_ptr<CoreRenderer> core_renderer;
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

    window::GlfwWindowDescriptor glfw_desc;
    glfw_desc.title = descriptor.title;
    glfw_desc.width = descriptor.width;
    glfw_desc.height = descriptor.height;
    glfw_desc.vsync_enabled = descriptor.vsync_enabled;
    glfw_desc.graphics_backend =
        descriptor.use_opengl_es ? window::GlfwGraphicsBackend::eOpenGLES : window::GlfwGraphicsBackend::eOpenGL;

    auto window = window::GlfwWindow::create(glfw_desc);
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
    impl_->core_renderer = std::make_unique<CoreRenderer>();
    impl_->core_renderer->registerRenderer(std::make_unique<MeshRenderer>(), 0, "mesh");
    impl_->core_renderer->registerRenderer(std::make_unique<DebugRenderer>(), 100, "debug");
    if (!impl_->core_renderer->init(impl_->render_device.get())) {
        impl_->render_adapter->shutdown();
        impl_.reset();
        return false;
    }

    app_ctx_.window = impl_->window.get();
    app_ctx_.renderer = impl_->render_adapter.get();
    app_ctx_.device = impl_->render_device.get();
    app_ctx_.coreRenderer = impl_->core_renderer.get();
    app_ctx_.debugDraw = impl_->core_renderer->getDebugDraw();

#if defined(VNE_TESTBED_EVENTS)
    registerAsListener();
#endif

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
    const float dt = static_cast<float>(std::chrono::duration<double>(now - impl_->last_frame_time).count());
    impl_->last_frame_time = now;

    RenderContext render_ctx{};
    render_ctx.frame_info.width = app_ctx_.window->getWidth();
    render_ctx.frame_info.height = app_ctx_.window->getHeight();
    render_ctx.frame_info.dt = dt;
    render_ctx.debug_draw = app_ctx_.debugDraw;

    // Layer update
    layer_stack_.onUpdate(dt);

    // Set callback for multi-viewport rendering (ImGuiLayer calls it for each viewport)
    app_ctx_.renderSceneForViewport = [this](const RenderContext& ctx) { layer_stack_.onRenderLayersOnly(ctx); };
    app_ctx_.scene_rendered_by_imgui = false;

    // GPU phase: scene first, then ImGui on top
    app_ctx_.renderer->beginFrame();
    layer_stack_.onBeginRender(render_ctx);
    if (!app_ctx_.scene_rendered_by_imgui) {
        layer_stack_.onRender(render_ctx);
    }

    // GUI phase: ImGui draws on top of scene
    layer_stack_.onGuiBegin(render_ctx);
    layer_stack_.onGuiRender(render_ctx);
    layer_stack_.onGuiEnd(render_ctx);

    app_ctx_.renderer->endFrame();
    impl_->window->swapBuffers();
}

void Application::registerAsListener() {
#if defined(VNE_TESTBED_EVENTS)
    application_event_listener_ = std::make_shared<ApplicationEventListener>(this);
    auto& mgr = vne::events::EventManager::instance();
    using ET = vne::events::EventType;
    mgr.registerListener(ET::eWindowClose, application_event_listener_);
    mgr.registerListener(ET::eWindowResize, application_event_listener_);
    mgr.registerListener(ET::eKeyPressed, application_event_listener_);
    mgr.registerListener(ET::eKeyReleased, application_event_listener_);
    mgr.registerListener(ET::eKeyRepeat, application_event_listener_);
    mgr.registerListener(ET::eKeyTyped, application_event_listener_);
    mgr.registerListener(ET::eMouseButtonPressed, application_event_listener_);
    mgr.registerListener(ET::eMouseButtonReleased, application_event_listener_);
    mgr.registerListener(ET::eMouseMoved, application_event_listener_);
    mgr.registerListener(ET::eMouseScrolled, application_event_listener_);
    mgr.registerListener(ET::eTouchPress, application_event_listener_);
    mgr.registerListener(ET::eTouchRelease, application_event_listener_);
    mgr.registerListener(ET::eTouchMove, application_event_listener_);
#endif
}

void Application::onEvent(const vne::events::Event& event) {
#if defined(VNE_TESTBED_EVENTS)
    using ET = vne::events::EventType;
    using KC = vne::events::KeyCode;

    if (event.type() == ET::eKeyPressed) {
        const auto& key_event = static_cast<const vne::events::KeyPressedEvent&>(event);
        if (key_event.keyCode() == KC::eEscape) {
            running_ = false;
            return;
        }
    }

    if (event.type() == ET::eWindowClose) {
        running_ = false;
        return;
    }

    layer_stack_.onEvent(event);
#else
    (void)event;
#endif
}

void Application::shutdown() {
    if (!running_) {
        return;
    }
    layer_stack_.clear();
    if (impl_) {
        if (impl_->core_renderer) {
            impl_->core_renderer->shutdown();
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

// Test-only: create/destroy Application so test TUs don't need the complete Impl type.
Application* createApplicationForTest() {
    return new Application();
}
void destroyApplicationForTest(Application* p) {
    delete p;
}

}  // namespace testbed
}  // namespace vne
