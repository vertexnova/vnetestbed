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
 * @file gl/opengl_context.h
 * @brief OpenGL context abstraction (GLFW); separate from window (xgl-style).
 *
 * Wraps the GL context associated with a window: make current, release,
 * and loading glad. Does not create or destroy the window.
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "opengl_context.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES. Build with OpenGL or OpenGL ES enabled."
#endif

namespace vne {
namespace testbed {
namespace gl {

/**
 * @class OpenGLContext
 * @brief OpenGL context tied to a window handle; owns make-current and glad loading.
 *
 * The window (e.g. GLFWwindow) is created by the runner; this class stores
 * the handle and makes the context current / releases it. Call create()
 * with the window handle after the window exists; then makeCurrent() before
 * any GL calls if the runner has not already made it current.
 */
class OpenGLContext {
   public:
    OpenGLContext() = default;
    ~OpenGLContext();

    OpenGLContext(const OpenGLContext&) = delete;
    OpenGLContext& operator=(const OpenGLContext&) = delete;
    OpenGLContext(OpenGLContext&&) = delete;
    OpenGLContext& operator=(OpenGLContext&&) = delete;

    /**
     * @brief Associate with a window and load OpenGL (glad).
     *
     * Makes the window's context current, then loads glad. Call once after
     * the window is created. The context is already created by GLFW with
     * the window; we only store the handle and load function pointers.
     *
     * @param window_handle  Native window handle (e.g. GLFWwindow*).
     * @return true if glad loaded successfully, false otherwise.
     */
    bool create(void* window_handle);

    /**
     * @brief Make this context current on the calling thread.
     * @return true on success.
     */
    bool makeCurrent();

    /**
     * @brief Release this context from the current thread.
     */
    void release();

    /**
     * @brief Release and clear the stored handle. Does not destroy the window.
     */
    void destroy();

    /**
     * @brief Whether create() succeeded and the context is valid.
     */
    [[nodiscard]] bool isValid() const { return window_handle_ != nullptr; }

    /**
     * @brief Stored window handle (e.g. for viewport / framebuffer size).
     */
    [[nodiscard]] void* getWindowHandle() const { return window_handle_; }

   private:
    void* window_handle_{nullptr};
    bool glad_loaded_{false};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
