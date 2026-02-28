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
#include "vertexnova/testbed/window/glfw_key_mapping.h"

#include <GLFW/glfw3.h>

#include <atomic>
#include <algorithm>
#include <memory>
#include <mutex>

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/window_event.h"
#include "vertexnova/events/input/input.h"
#include "vertexnova/events/types.h"
#endif

namespace vne {
namespace testbed {
namespace window {

namespace {

// GLFW init runs exactly once; no thread proceeds to hints/create until init has completed.
static std::once_flag g_glfw_init_once;
static std::atomic<bool> g_glfw_init_ok{false};
// Number of GlfwWindow instances; terminate when the last is destroyed.
static std::atomic<int> g_glfw_window_count{0};

void glfwErrorCallback(int error, const char* description) {
    (void)error;
    (void)description;
    // Can be wired to vne::logging if desired
}

void ensureGlfwInit() {
    std::call_once(g_glfw_init_once, []() {
        glfwSetErrorCallback(glfwErrorCallback);
        g_glfw_init_ok.store(glfwInit() != 0);
    });
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
void onGlfwClose(GLFWwindow* w) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (self) {
        vne::events::EventManager::instance().pushEvent(std::make_unique<vne::events::WindowCloseEvent>());
    }
}

void onGlfwFramebufferSize(GLFWwindow* w, int width, int height) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (self) {
        vne::events::EventManager::instance().pushEvent(
            std::make_unique<vne::events::WindowResizeEvent>(static_cast<uint32_t>(width), static_cast<uint32_t>(height)));
        vne::events::Input::updateWindowSize(width, height);
        self->updateDPIScale();
    }
}

void onGlfwKey(GLFWwindow* w, int key, int /*scan*/, int action, int mods) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    const vne::events::KeyCode k = mapGlfwToKeyCode(key);
    const uint8_t modifiers = mapGlfwToModifiers(mods);
    auto& mgr = vne::events::EventManager::instance();
    static int s_repeat_count = 0;
    if (action == GLFW_PRESS) {
        s_repeat_count = 0;
        mgr.pushEvent(std::make_unique<vne::events::KeyPressedEvent>(k, modifiers));
        vne::events::Input::updateKeyState(static_cast<int>(k), true);
    } else if (action == GLFW_RELEASE) {
        s_repeat_count = 0;
        mgr.pushEvent(std::make_unique<vne::events::KeyReleasedEvent>(k, modifiers));
        vne::events::Input::updateKeyState(static_cast<int>(k), false);
    } else if (action == GLFW_REPEAT) {
        ++s_repeat_count;
        mgr.pushEvent(std::make_unique<vne::events::KeyRepeatEvent>(k, static_cast<uint32_t>(s_repeat_count)));
    }
}

void onGlfwChar(GLFWwindow* w, unsigned int codepoint) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    vne::events::EventManager::instance().pushEvent(
        std::make_unique<vne::events::KeyTypedEvent>(static_cast<vne::events::KeyCode>(codepoint)));
}

void onGlfwMouseButton(GLFWwindow* w, int button, int action, int mods) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    const vne::events::MouseButton b = mapGlfwToMouseButton(button);
    const uint8_t modifiers = mapGlfwToModifiers(mods);
    auto& mgr = vne::events::EventManager::instance();
    if (action == GLFW_PRESS) {
        mgr.pushEvent(std::make_unique<vne::events::MouseButtonPressedEvent>(b, modifiers));
        vne::events::Input::updateMouseButtonState(static_cast<int>(b), true);
    } else {
        mgr.pushEvent(std::make_unique<vne::events::MouseButtonReleasedEvent>(b, modifiers));
        vne::events::Input::updateMouseButtonState(static_cast<int>(b), false);
    }
}

uint8_t queryGlfwModifiers(GLFWwindow* w) {
    int mods = 0;
    if (glfwGetKey(w, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS) {
        mods |= GLFW_MOD_SHIFT;
    }
    if (glfwGetKey(w, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS) {
        mods |= GLFW_MOD_CONTROL;
    }
    if (glfwGetKey(w, GLFW_KEY_LEFT_ALT) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS) {
        mods |= GLFW_MOD_ALT;
    }
    if (glfwGetKey(w, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS || glfwGetKey(w, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS) {
        mods |= GLFW_MOD_SUPER;
    }
    return mapGlfwToModifiers(mods);
}

void onGlfwCursorPos(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    const uint8_t modifiers = queryGlfwModifiers(w);
    vne::events::EventManager::instance().pushEvent(std::make_unique<vne::events::MouseMovedEvent>(x, y, modifiers));
    vne::events::Input::updateMousePosition(static_cast<int>(x), static_cast<int>(y));
}

void onGlfwScroll(GLFWwindow* w, double xoff, double yoff) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    vne::events::EventManager::instance().pushEvent(std::make_unique<vne::events::MouseScrolledEvent>(xoff, yoff));
    vne::events::Input::updateMouseScroll(static_cast<float>(xoff), static_cast<float>(yoff));
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
    glfwSetWindowCloseCallback(window_, nullptr);
    glfwSetFramebufferSizeCallback(window_, nullptr);
    glfwSetKeyCallback(window_, nullptr);
    glfwSetCharCallback(window_, nullptr);
    glfwSetMouseButtonCallback(window_, nullptr);
    glfwSetCursorPosCallback(window_, nullptr);
    glfwSetScrollCallback(window_, nullptr);
#endif
    glfwSetWindowUserPointer(window_, nullptr);
    glfwDestroyWindow(window_);
    window_ = nullptr;

    if (g_glfw_window_count.fetch_sub(1) == 1) {
        glfwTerminate();
    }
}

std::unique_ptr<GlfwWindow> GlfwWindow::create(const GlfwWindowDescriptor& descriptor) {
    ensureGlfwInit();
    if (!g_glfw_init_ok.load()) {
        return nullptr;
    }

    g_glfw_window_count.fetch_add(1);

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
        if (g_glfw_window_count.fetch_sub(1) == 1) {
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
    if (g_glfw_init_ok.load()) {
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
        glfwSetCharCallback(window_, onGlfwChar);
        glfwSetMouseButtonCallback(window_, onGlfwMouseButton);
        glfwSetCursorPosCallback(window_, onGlfwCursorPos);
        glfwSetScrollCallback(window_, onGlfwScroll);
    } else {
        glfwSetKeyCallback(window_, nullptr);
        glfwSetCharCallback(window_, nullptr);
        glfwSetMouseButtonCallback(window_, nullptr);
        glfwSetCursorPosCallback(window_, nullptr);
        glfwSetScrollCallback(window_, nullptr);
    }
#endif
}

void GlfwWindow::setupCallbacks() {
    if (!window_) {
        return;
    }
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    glfwSetWindowCloseCallback(window_, onGlfwClose);
    glfwSetFramebufferSizeCallback(window_, onGlfwFramebufferSize);
#endif
}

}  // namespace window
}  // namespace testbed
}  // namespace vne
