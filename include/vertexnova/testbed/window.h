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
 * @file window.h
 * @brief Minimal window interface; implemented by runners (e.g. GLFW).
 *
 * The testbed core does not depend on any windowing library; runners
 * supply a concrete implementation and pass it through AppContext.
 */

namespace vne {
namespace testbed {

/**
 * @struct IWindow
 * @brief Backend-agnostic window interface.
 *
 * Provides the minimum surface area the testbed needs: dimensions,
 * event polling, and close detection.  Runners implement this with
 * their windowing library of choice (GLFW, SDL, Win32, etc.).
 */
struct IWindow {
    virtual ~IWindow() = default;

    /** @brief Width of the window framebuffer in pixels. */
    virtual int getWidth() const = 0;

    /** @brief Height of the window framebuffer in pixels. */
    virtual int getHeight() const = 0;

    /** @brief Poll and dispatch pending OS events. */
    virtual void pollEvents() = 0;

    /** @brief Returns true when the user has requested the window to close. */
    virtual bool shouldClose() const = 0;

    /**
     * @brief Returns the platform-specific native window handle.
     *
     * The type of the returned pointer depends on the backend:
     *  - GLFW runner:           GLFWwindow*
     *  - vnecrosswindow runner: vnecrosswindow native handle
     *
     * Used by IRenderAdapter::init() so the adapter can bind its rendering
     * context to the window without the runner needing to know the adapter type.
     */
    virtual void* getNativeHandle() const = 0;

    /**
     * @brief Swap front and back buffers (e.g. glfwSwapBuffers). No-op for backends that do not use double-buffering.
     */
    virtual void swapBuffers() = 0;
};

}  // namespace testbed
}  // namespace vne
