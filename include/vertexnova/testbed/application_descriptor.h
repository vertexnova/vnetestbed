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
 * @file application_descriptor.h
 * @brief Descriptor for Application initialization (window/render backends, title, size).
 *
 * Used by Application::initialize(). Backend selection allows the same
 * Application type to run with GLFW + OpenGL (today) or vnecross + OpenGL (future).
 */

#include <cstdint>
#include <string>

namespace vne {
namespace testbed {

/**
 * @brief Window backend: which windowing API to use.
 */
enum class WindowBackend {
    GLFW,     ///< GLFW window (desktop); requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES.
    VneCross  ///< vnecross window (future).
};

/**
 * @brief Render backend: which rendering API to use.
 */
enum class RenderBackend {
    OpenGL  ///< OpenGL 4.1 or OpenGL ES 3.0; requires corresponding window/context.
};

/**
 * @struct ApplicationDescriptor
 * @brief Configuration for Application::initialize().
 */
struct ApplicationDescriptor {
    std::string title{"VneTestbed"};
    uint32_t width{800};
    uint32_t height{600};
    bool vsync_enabled{true};
    WindowBackend window_backend{WindowBackend::GLFW};
    RenderBackend render_backend{RenderBackend::OpenGL};

#if defined(VNE_TESTBED_OPENGLES)
    bool use_opengl_es{true};
#else
    bool use_opengl_es{false};
#endif

    ApplicationDescriptor() = default;

    ApplicationDescriptor(const std::string& title_in, uint32_t w, uint32_t h)
        : title(title_in)
        , width(w)
        , height(h) {}
};

}  // namespace testbed
}  // namespace vne
