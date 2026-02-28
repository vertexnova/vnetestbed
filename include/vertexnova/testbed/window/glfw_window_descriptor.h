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
 * @file window/glfw_window_descriptor.h
 * @brief Descriptor for GLFW window creation (xwin-style).
 *
 * Used with GlfwWindow::create(descriptor). Only available when
 * VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES is defined.
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "glfw_window_descriptor.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES."
#endif

#include <cstdint>
#include <string>

namespace vne {
namespace testbed {
namespace window {

/**
 * @brief Graphics backend for window context hints.
 */
enum class GlfwGraphicsBackend {
    OpenGL,   ///< OpenGL 4.1 Core (desktop)
    OpenGLES  ///< OpenGL ES 3.0
};

/**
 * @brief Window creation parameters for GlfwWindow.
 */
struct GlfwWindowDescriptor {
    std::string title{"VneTestbed"};
    uint32_t width{800};
    uint32_t height{600};
    int x{100};
    int y{100};
    bool vsync_enabled{true};
    bool resizable{true};
    bool decorated{true};
    bool transparent{false};
    bool visible{true};
    GlfwGraphicsBackend graphics_backend{GlfwGraphicsBackend::OpenGL};

    GlfwWindowDescriptor() = default;

    GlfwWindowDescriptor(const std::string& title_in, uint32_t w, uint32_t h)
        : title(title_in)
        , width(w)
        , height(h) {}
};

}  // namespace window
}  // namespace testbed
}  // namespace vne
