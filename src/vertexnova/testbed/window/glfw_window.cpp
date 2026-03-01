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
#include "vertexnova/common/macros.h"

#include <GLFW/glfw3.h>

#include <algorithm>
#include <climits>
#include <memory>
#include <mutex>

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/touch_event.h"
#include "vertexnova/events/window_event.h"
#include "vertexnova/events/input/input.h"
#include "vertexnova/events/types.h"
#endif

namespace vne {
namespace testbed {
namespace window {

namespace {

// GLFW is not thread-safe: all calls (glfwCreateWindow, glfwDestroyWindow,
// glfwPollEvents, glfwTerminate) MUST be made from the same thread (the main
// thread).  The mutex below protects the ref-count and the glfwInit /
// glfwTerminate calls, but does not make GLFW itself thread-safe.
// Do not call create() or destroy GlfwWindow objects concurrently.

// Ref-count of live GlfwWindow instances.
// glfwInit() is called when the count goes 0 → 1.
// glfwTerminate() is called when the count goes 1 → 0.
std::mutex g_glfw_mutex;
int g_glfw_window_count{0};  // guarded by g_glfw_mutex
bool g_glfw_init_ok{false};  // guarded by g_glfw_mutex

void glfwErrorCallback(int error, const char* description) {
    VNE_UNUSED(error);
    VNE_UNUSED(description);
    // Can be wired to vne::logging if desired
}

// Increment the ref-count.  Calls glfwInit() on the first window.
// Returns false if glfwInit() failed.
bool glfwAddRef() {
    std::lock_guard<std::mutex> lock(g_glfw_mutex);
    if (g_glfw_window_count == 0) {
        glfwSetErrorCallback(glfwErrorCallback);
        g_glfw_init_ok = (glfwInit() != 0);
    }
    if (!g_glfw_init_ok) {
        return false;
    }
    ++g_glfw_window_count;
    return true;
}

// Decrement the ref-count.  Calls glfwTerminate() when the last window goes away.
void glfwReleaseRef() {
    std::lock_guard<std::mutex> lock(g_glfw_mutex);
    if (g_glfw_window_count > 0 && --g_glfw_window_count == 0) {
        glfwTerminate();
        g_glfw_init_ok = false;
    }
}

/** Clamp uint32_t to int range [1, INT_MAX] for GLFW size parameters. */
int clampGlfwSize(uint32_t v) {
    constexpr int kMax = INT_MAX;
    if (v == 0) {
        return 1;
    }
    if (v > static_cast<uint32_t>(kMax)) {
        return kMax;
    }
    return static_cast<int>(v);
}

void setOpenGLHints(GlfwGraphicsBackend backend) {
    if (backend == GlfwGraphicsBackend::eOpenGLES) {
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

}  // namespace

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)

// ---------------------------------------------------------------------------
// Private static GLFW callbacks — defined in the enclosing named namespace
// so the compiler recognises them as GlfwWindow member definitions.
// ---------------------------------------------------------------------------

void GlfwWindow::cbClose(GLFWwindow* w) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (self) {
        vne::events::EventManager::instance().pushEvent(std::make_unique<vne::events::WindowCloseEvent>());
    }
}

void GlfwWindow::cbFramebufferSize(GLFWwindow* w, int width, int height) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (self) {
        vne::events::EventManager::instance().pushEvent(
            std::make_unique<vne::events::WindowResizeEvent>(static_cast<uint32_t>(width),
                                                             static_cast<uint32_t>(height)));
        vne::events::Input::updateWindowSize(width, height);
        self->updateDPIScale();
    }
}

void GlfwWindow::cbKey(GLFWwindow* w, int key, int /*scan*/, int action, int mods) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    const vne::events::KeyCode k = mapGlfwToKeyCode(key);
    const uint8_t modifiers = mapGlfwToModifiers(mods);
    auto& mgr = vne::events::EventManager::instance();
    if (action == GLFW_PRESS) {
        self->key_repeat_count_ = 0;
        mgr.pushEvent(std::make_unique<vne::events::KeyPressedEvent>(k, modifiers));
        vne::events::Input::updateKeyState(static_cast<int>(k), true);
    } else if (action == GLFW_RELEASE) {
        self->key_repeat_count_ = 0;
        mgr.pushEvent(std::make_unique<vne::events::KeyReleasedEvent>(k, modifiers));
        vne::events::Input::updateKeyState(static_cast<int>(k), false);
    } else if (action == GLFW_REPEAT) {
        ++self->key_repeat_count_;
        mgr.pushEvent(std::make_unique<vne::events::KeyRepeatEvent>(k, self->key_repeat_count_));
    }
}

void GlfwWindow::cbChar(GLFWwindow* w, unsigned int codepoint) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    vne::events::EventManager::instance().pushEvent(
        std::make_unique<vne::events::KeyTypedEvent>(static_cast<vne::events::KeyCode>(codepoint)));
}

void GlfwWindow::cbMouseButton(GLFWwindow* w, int button, int action, int mods) {
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
        if (self->touch_emulation_enabled_ && button == GLFW_MOUSE_BUTTON_LEFT) {
            double x = 0, y = 0;
            glfwGetCursorPos(w, &x, &y);
            mgr.pushEvent(std::make_unique<vne::events::TouchPressEvent>(0, x, y));
        }
    } else {
        mgr.pushEvent(std::make_unique<vne::events::MouseButtonReleasedEvent>(b, modifiers));
        vne::events::Input::updateMouseButtonState(static_cast<int>(b), false);
        if (self->touch_emulation_enabled_ && button == GLFW_MOUSE_BUTTON_LEFT) {
            double x = 0, y = 0;
            glfwGetCursorPos(w, &x, &y);
            mgr.pushEvent(std::make_unique<vne::events::TouchReleaseEvent>(0, x, y));
        }
    }
}

namespace {

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

}  // namespace

void GlfwWindow::cbCursorPos(GLFWwindow* w, double x, double y) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    const uint8_t modifiers = queryGlfwModifiers(w);
    auto& mgr = vne::events::EventManager::instance();
    mgr.pushEvent(std::make_unique<vne::events::MouseMovedEvent>(x, y, modifiers));
    vne::events::Input::updateMousePosition(static_cast<int>(x), static_cast<int>(y));
    // Touch emulation (opt-in): one TouchMoveEvent per cursor move while LMB is down.
    if (self->touch_emulation_enabled_ && glfwGetMouseButton(w, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS) {
        mgr.pushEvent(std::make_unique<vne::events::TouchMoveEvent>(0, x, y));
    }
}

void GlfwWindow::cbScroll(GLFWwindow* w, double xoff, double yoff) {
    auto* self = static_cast<GlfwWindow*>(glfwGetWindowUserPointer(w));
    if (!self || !self->isEventForwarding()) {
        return;
    }
    vne::events::EventManager::instance().pushEvent(std::make_unique<vne::events::MouseScrolledEvent>(xoff, yoff));
    vne::events::Input::updateMouseScroll(static_cast<float>(xoff), static_cast<float>(yoff));
}

#endif  // VNE_TESTBED_OPENGL || VNE_TESTBED_OPENGLES

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
    key_repeat_count_ = 0;
    glfwSetWindowUserPointer(window_, nullptr);
    glfwDestroyWindow(window_);
    window_ = nullptr;

    glfwReleaseRef();
}

std::unique_ptr<GlfwWindow> GlfwWindow::create(const GlfwWindowDescriptor& descriptor) {
    if (!glfwAddRef()) {
        return nullptr;
    }

    glfwWindowHint(GLFW_VISIBLE, descriptor.visible ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_RESIZABLE, descriptor.resizable ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_DECORATED, descriptor.decorated ? GLFW_TRUE : GLFW_FALSE);
    glfwWindowHint(GLFW_TRANSPARENT_FRAMEBUFFER, descriptor.transparent ? GLFW_TRUE : GLFW_FALSE);
    setOpenGLHints(descriptor.graphics_backend);

    const int w = clampGlfwSize(descriptor.width);
    const int h = clampGlfwSize(descriptor.height);
    GLFWwindow* raw = glfwCreateWindow(w, h, descriptor.title.c_str(), nullptr, nullptr);
    if (!raw) {
        glfwReleaseRef();
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
    desc.graphics_backend = use_opengl_es ? GlfwGraphicsBackend::eOpenGLES : GlfwGraphicsBackend::eOpenGL;
    return create(desc);
}

int GlfwWindow::getWidth() const {
    return getFramebufferWidth();
}

int GlfwWindow::getHeight() const {
    return getFramebufferHeight();
}

void GlfwWindow::pollEvents() {
    std::lock_guard<std::mutex> lock(g_glfw_mutex);
    if (g_glfw_init_ok) {
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
        glfwSetWindowSize(window_, clampGlfwSize(width), clampGlfwSize(height));
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
        glfwSetKeyCallback(window_, GlfwWindow::cbKey);
        glfwSetCharCallback(window_, GlfwWindow::cbChar);
        glfwSetMouseButtonCallback(window_, GlfwWindow::cbMouseButton);
        glfwSetCursorPosCallback(window_, GlfwWindow::cbCursorPos);
        glfwSetScrollCallback(window_, GlfwWindow::cbScroll);
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
    glfwSetWindowCloseCallback(window_, GlfwWindow::cbClose);
    glfwSetFramebufferSizeCallback(window_, GlfwWindow::cbFramebufferSize);
#endif
}

}  // namespace window
}  // namespace testbed
}  // namespace vne
