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

#include "vertexnova/testbed/window/glfw_window.h"

#include <GLFW/glfw3.h>

#include <atomic>
#include <algorithm>
#include <memory>

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"
#endif

namespace vne {
namespace testbed {
namespace window {

namespace {

static std::atomic<int> g_glfw_init_count{0};

void glfwErrorCallback(int error, const char* description) {
    (void)error;
    (void)description;
    // Can be wired to vne::logging if desired
}

void setOpenGLHints(GlfwGraphicsBackend backend) {
    if (backend == GlfwGraphicsBackend::OpenGLES) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_OPENGL_ES_API);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 0);
    } else {
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
        glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    }
}

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
void onGlfwKey(GLFWwindow* w, int key, int /*scan*/, int action, int /*mods*/) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    auto& mgr = vne::events::EventManager::instance();
    if (action == GLFW_PRESS) {
        mgr.pushEvent(std::make_unique<vne::events::KeyPressedEvent>(static_cast<vne::events::KeyCode>(key)));
    } else if (action == GLFW_RELEASE) {
        mgr.pushEvent(std::make_unique<vne::events::KeyReleasedEvent>(static_cast<vne::events::KeyCode>(key)));
    }
}

void onGlfwMouseButton(GLFWwindow* w, int button, int action, int /*mods*/) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    auto& mgr = vne::events::EventManager::instance();
    if (action == GLFW_PRESS) {
        mgr.pushEvent(
            std::make_unique<vne::events::MouseButtonPressedEvent>(static_cast<vne::events::MouseButton>(button)));
    } else {
        mgr.pushEvent(
            std::make_unique<vne::events::MouseButtonReleasedEvent>(static_cast<vne::events::MouseButton>(button)));
    }
}

void onGlfwCursorPos(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    vne::events::EventManager::instance().pushEvent(std::make_unique<vne::events::MouseMovedEvent>(x, y));
}

void onGlfwScroll(GLFWwindow* w, double xoff, double yoff) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    vne::events::EventManager::instance().pushEvent(std::make_unique<vne::events::MouseScrolledEvent>(xoff, yoff));
}
#endif

}  // namespace

GlfwWindow::GlfwWindow(GLFWwindow* window)
    : window_(window) {}

GlfwWindow::~GlfwWindow() {
    if (!window_) {
        return;
    }
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    glfwSetKeyCallback(window_, nullptr);
    glfwSetMouseButtonCallback(window_, nullptr);
    glfwSetCursorPosCallback(window_, nullptr);
    glfwSetScrollCallback(window_, nullptr);
#endif
    glfwSetWindowUserPointer(window_, nullptr);
    glfwDestroyWindow(window_);
    window_ = nullptr;

    if (g_glfw_init_count.fetch_sub(1) == 1) {
        glfwTerminate();
    }
}

std::unique_ptr<GlfwWindow> GlfwWindow::create(const GlfwWindowDescriptor& descriptor) {
    if (g_glfw_init_count.fetch_add(1) == 0) {
        glfwSetErrorCallback(glfwErrorCallback);
        if (!glfwInit()) {
            g_glfw_init_count.fetch_sub(1);
            return nullptr;
        }
    }

    glfwWindowHint(GLFW_VISIBLE, descriptor.visible ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, descriptor.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, descriptor.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, descriptor.transparent ? GLFW_TRUE : GLFW_FALSE);
    setOpenGLHints(descriptor.graphics_backend);

    GLFWwindow* raw = glfwCreateWindow(static_cast<int>(descriptor.width),
                                       static_cast<int>(descriptor.height),
                                       descriptor.title.c_str(),
                                       nullptr,
                                       nullptr);
    if (!raw) {
        if (g_glfw_init_count.fetch_sub(1) == 1) {
            glfwTerminate();
        }
        return nullptr;
    }

    glfwSetWindowPos(raw, descriptor.x, descriptor.y);
    glfwMakeContextCurrent(raw);
    glfwSwapInterval(descriptor.vsync_enabled ? 1 : 0);

    auto win = std::unique_ptr<GlfwWindow>(new GlfwWindow(raw));
    glfwSetWindowUserPointer(raw, win.get());
    win->vsync_enabled_ = descriptor.vsync_enabled;
    win->updateDPIScale();
    win->setupCallbacks();
    return win;
}

std::unique_ptr<GlfwWindow> GlfwWindow::create(int width, int height, const char* title, bool use_opengl_es) {
    GlfwWindowDescriptor desc;
    desc.width = static_cast<uint32_t>(width > 0 ? width : 800);
    desc.height = static_cast<uint32_t>(height > 0 ? height : 600);
    desc.title = title ? title : "";
    desc.graphics_backend = use_opengl_es ? GlfwGraphicsBackend::OpenGLES : GlfwGraphicsBackend::OpenGL;
    return create(desc);
}

int GlfwWindow::getWidth() const {
    return getFramebufferWidth();
}

int GlfwWindow::getHeight() const {
    return getFramebufferHeight();
}

void GlfwWindow::pollEvents() {
    if (g_glfw_init_count.load() > 0) {
        glfwPollEvents();
    }
}

bool GlfwWindow::shouldClose() const {
    return window_ && glfwWindowShouldClose(window_) != 0;
}

void* GlfwWindow::getNativeHandle() const {
    return window_;
}

void GlfwWindow::setTitle(const std::string& title) {
    if (window_) {
        glfwSetWindowTitle(window_, title.c_str());
    }
}

void GlfwWindow::setPosition(int x, int y) {
    if (window_) {
        glfwSetWindowPos(window_, x, y);
    }
}

void GlfwWindow::getPosition(int& x, int& y) const {
    if (window_) {
        glfwGetWindowPos(window_, &x, &y);
    } else {
        x = y = 0;
    }
}

void GlfwWindow::setVsync(bool enabled) {
    if (window_) {
        glfwMakeContextCurrent(window_);
        glfwSwapInterval(enabled ? 1 : 0);
        vsync_enabled_ = enabled;
    }
}

void GlfwWindow::updateDPIScale() {
    if (!window_) {
        return;
    }
    float x_scale = 1.0f;
    float y_scale = 1.0f;
    glfwGetWindowContentScale(window_, &x_scale, &y_scale);
    dpi_scale_ = std::max(x_scale, y_scale);
}

int GlfwWindow::getFramebufferWidth() const {
    if (!window_) {
        return 0;
    }
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window_, &w, &h);
    return w;
}

int GlfwWindow::getFramebufferHeight() const {
    if (!window_) {
        return 0;
    }
    int w = 0;
    int h = 0;
    glfwGetFramebufferSize(window_, &w, &h);
    return h;
}

void GlfwWindow::swapBuffers() {
    if (window_) {
        glfwSwapBuffers(window_);
    }
}

void GlfwWindow::minimize() {
    if (window_) {
        glfwIconifyWindow(window_);
    }
}

void GlfwWindow::maximize() {
    if (window_) {
        glfwMaximizeWindow(window_);
    }
}

void GlfwWindow::restore() {
    if (window_) {
        glfwRestoreWindow(window_);
    }
}

void GlfwWindow::resize(uint32_t width, uint32_t height) {
    if (window_) {
        glfwSetWindowSize(window_, static_cast<int>(width), static_cast<int>(height));
    }
}

void GlfwWindow::close() {
    if (window_) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
    }
}

bool GlfwWindow::isOpen() const {
    return window_ != nullptr && glfwWindowShouldClose(window_) == 0;
}

void GlfwWindow::setEventForwarding(bool enable) {
    event_forwarding_ = enable;
    if (!window_) {
        return;
    }
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    if (enable) {
        glfwSetKeyCallback(window_, onGlfwKey);
        glfwSetMouseButtonCallback(window_, onGlfwMouseButton);
        glfwSetCursorPosCallback(window_, onGlfwCursorPos);
        glfwSetScrollCallback(window_, onGlfwScroll);
    } else {
        glfwSetKeyCallback(window_, nullptr);
        glfwSetMouseButtonCallback(window_, nullptr);
        glfwSetCursorPosCallback(window_, nullptr);
        glfwSetScrollCallback(window_, nullptr);
    }
#endif
}

void GlfwWindow::setupCallbacks() {
    (void)this;
    // Optional: add content scale callback to refresh dpi_scale_ on DPI change
}

}  // namespace window
}  // namespace testbed
}  // namespace vne
