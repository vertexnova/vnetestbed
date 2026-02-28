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
 * @file gl/vertex_buffer.h
 * @brief RAII wrapper for an OpenGL vertex buffer object (VBO).
 *
 * Supports both static (upload once) and dynamic (update per frame) buffers.
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "vertex_buffer.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES. Build with OpenGL or OpenGL ES enabled."
#endif

#include <cstddef>

namespace vne {
namespace testbed {
namespace gl {

/**
 * @class VertexBuffer
 * @brief RAII OpenGL vertex buffer object (GL_ARRAY_BUFFER).
 *
 * Static buffers: pass data on construction — use GL_STATIC_DRAW.
 * Dynamic buffers: pass only size on construction — use GL_DYNAMIC_DRAW,
 *   then call setData() each frame before drawing.
 */
class VertexBuffer {
   public:
    /**
     * @brief Create a static buffer and upload initial data.
     * @param data      Pointer to float vertex data.
     * @param byte_size Number of bytes to upload.
     */
    VertexBuffer(const float* data, std::size_t byte_size);

    /**
     * @brief Create an empty dynamic buffer of a given capacity.
     * @param byte_size Maximum number of bytes the buffer will hold.
     */
    explicit VertexBuffer(std::size_t byte_size);

    ~VertexBuffer();

    VertexBuffer(const VertexBuffer&) = delete;
    VertexBuffer& operator=(const VertexBuffer&) = delete;
    VertexBuffer(VertexBuffer&&) = delete;
    VertexBuffer& operator=(VertexBuffer&&) = delete;

    /** @brief Bind as the current GL_ARRAY_BUFFER. */
    void bind() const;

    /** @brief Unbind (restore GL_ARRAY_BUFFER to 0). */
    void unbind() const;

    /**
     * @brief Upload new data to a dynamic buffer (GL_DYNAMIC_DRAW).
     * @param data      Pointer to new vertex data.
     * @param byte_size Number of bytes to upload (must not exceed capacity).
     */
    void setData(const void* data, std::size_t byte_size);

   private:
    unsigned int vbo_id_{0u};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
