#pragma once
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
 * @file window/glfw_window.h
 * @brief GLFW-based IWindow implementation (xwin-style, multi-platform).
 *
 * Full-featured window: descriptor-based creation, title/position/size,
 * vsync, minimize/maximize/restore, DPI scale, event forwarding.
 * Supports OpenGL 4.1 and OpenGL ES 3.0 only.
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "glfw_window.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES."
#endif

#include "vertexnova/testbed/window.h"
#include "vertexnova/testbed/window/glfw_window_descriptor.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <memory>

namespace vne {
namespace testbed {
namespace window {

/**
 * @class GlfwWindow
 * @brief IWindow implementation using GLFW; xwin-style API.
 *
 * Create via create(descriptor) or create(width, height, title, use_opengl_es).
 * Owns GLFW lifecycle (ref-counted glfwInit/glfwTerminate).
 */
class GlfwWindow : public IWindow {
   public:
    GlfwWindow(const GlfwWindow&) = delete;
    GlfwWindow& operator=(const GlfwWindow&) = delete;

    ~GlfwWindow() override;

    /**
     * @brief Create a GLFW window from a descriptor.
     */
    static std::unique_ptr<GlfwWindow> create(const GlfwWindowDescriptor& descriptor);

    /**
     * @brief Create a GLFW window with minimal parameters.
     */
    static std::unique_ptr<GlfwWindow> create(int width, int height, const char* title, bool use_opengl_es);

    // -----------------------------------------------------------------------
    // IWindow
    // -----------------------------------------------------------------------
    int getWidth() const override;
    int getHeight() const override;
    void pollEvents() override;
    bool shouldClose() const override;
    void* getNativeHandle() const override;
    void swapBuffers() override;

    // -----------------------------------------------------------------------
    // Window properties
    // -----------------------------------------------------------------------
    void setTitle(const std::string& title);
    void setPosition(int x, int y);
    void getPosition(int& x, int& y) const;
    void setVsync(bool enabled);
    bool isVsyncEnabled() const { return vsync_enabled_; }
    float getDPIScale() const { return dpi_scale_; }
    int getFramebufferWidth() const;
    int getFramebufferHeight() const;

    // -----------------------------------------------------------------------
    // Window operations
    // -----------------------------------------------------------------------
    void minimize();
    void maximize();
    void restore();
    void resize(uint32_t width, uint32_t height);
    void close();
    bool isOpen() const;

    // -----------------------------------------------------------------------
    // Event forwarding (GLFW → vne::events)
    // -----------------------------------------------------------------------
    void setEventForwarding(bool enable);
    bool isEventForwarding() const { return event_forwarding_; }

    /** @brief Refresh DPI scale from GLFW (e.g. after resize). */
    void updateDPIScale();

   private:
    explicit GlfwWindow(GLFWwindow* window);

    void setupCallbacks();

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    // GLFW callback stubs — registered via glfwSet*Callback().
    // Declared as private statics so they can access per-window members
    // (e.g. key_repeat_count_) without exposing them publicly.
    static void cbClose(GLFWwindow* w);
    static void cbFramebufferSize(GLFWwindow* w, int width, int height);
    static void cbKey(GLFWwindow* w, int key, int scan, int action, int mods);
    static void cbChar(GLFWwindow* w, unsigned int codepoint);
    static void cbMouseButton(GLFWwindow* w, int button, int action, int mods);
    static void cbCursorPos(GLFWwindow* w, double x, double y);
    static void cbScroll(GLFWwindow* w, double xoff, double yoff);
#endif

    GLFWwindow* window_{nullptr};
    bool event_forwarding_{false};
    bool vsync_enabled_{true};
    float dpi_scale_{1.0f};
    // Per-window key repeat counter; avoids sharing state across multiple windows.
    uint32_t key_repeat_count_{0};
};

}  // namespace window
}  // namespace testbed
}  // namespace vne
