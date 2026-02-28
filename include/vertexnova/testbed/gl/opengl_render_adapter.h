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
 * @file gl/opengl_render_adapter.h
 * @brief IRenderAdapter implementation backed by raw OpenGL + glad.
 *
 * Swap point for vnecrossgl: replace OpenGLRenderAdapter with a
 * CrossGLRenderAdapter that implements the same IRenderAdapter interface.
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#ifndef VNE_TESTBED_OPENGL
#error "opengl_render_adapter.h requires VNE_TESTBED_OPENGL to be defined. Build with OpenGL enabled (glad)."
#endif

#include "vertexnova/testbed/gl/opengl_context.h"
#include "vertexnova/testbed/render_adapter.h"
#include "vertexnova/math/core/core.h"

namespace vne {
namespace testbed {
namespace gl {

/**
 * @class OpenGLRenderAdapter
 * @brief Concrete IRenderAdapter using raw OpenGL 4.1 (macOS-compatible).
 *
 * Lifecycle:
 *  1. Construct (no GL calls yet).
 *  2. Call init(window_handle) after the GLFW context is current.
 *     Pass the GLFWwindow* (or IWindow::getNativeHandle()).
 *  3. Each frame: beginFrame() → layer rendering → endFrame().
 *  4. Call shutdown() before destroying the GL context.
 *
 * Buffer swapping is performed by the runner (glfwSwapBuffers), not here.
 */
class OpenGLRenderAdapter : public IRenderAdapter {
   public:
    OpenGLRenderAdapter() = default;
    ~OpenGLRenderAdapter() override;

    OpenGLRenderAdapter(const OpenGLRenderAdapter&) = delete;
    OpenGLRenderAdapter& operator=(const OpenGLRenderAdapter&) = delete;

    /**
     * @brief Set the background clear colour (RGBA, each in [0, 1]).
     *
     * Must be called before or after init(); takes effect on the next
     * beginFrame().
     */
    void setClearColor(float r, float g, float b, float a = 1.0f);

    // -----------------------------------------------------------------------
    // IRenderAdapter
    // -----------------------------------------------------------------------

    /**
     * @brief Initialise OpenGL via glad and set up render state.
     * @param window_handle  GLFWwindow* cast to void*, used to load glad.
     * @return true on success; false if glad failed to load.
     */
    bool init(void* window_handle) override;

    /**
     * @brief Clear the colour and depth buffers and set the viewport.
     *
     * Must be called at the start of each frame, before any layer onRender()
     * calls.  The viewport is set to the full framebuffer size each frame to
     * handle window resizes automatically.
     */
    void beginFrame() override;

    /**
     * @brief No-op — buffer swapping is the runner's responsibility.
     *
     * Kept as a hook for future use (e.g. inserting a GPU fence).
     */
    void endFrame() override {}

    /** @brief Reset GL state and release owned resources. */
    void shutdown() override;

    /** @brief Returns true after a successful init(). */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

   private:
    OpenGLContext context_;
    vne::math::Vec4f clear_color_{0.12f, 0.12f, 0.16f, 1.0f};
    bool initialized_{false};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
