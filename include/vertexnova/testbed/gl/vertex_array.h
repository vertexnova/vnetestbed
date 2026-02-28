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
 * @file gl/vertex_array.h
 * @brief RAII wrapper for an OpenGL vertex array object (VAO).
 *
 * Encapsulates the vertex attribute layout binding. Call addVertexBuffer()
 * while this VAO is bound to describe how a VertexBuffer's bytes map to
 * shader attribute locations.
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#ifndef VNE_TESTBED_OPENGL
#error "vertex_array.h requires VNE_TESTBED_OPENGL to be defined. Build with OpenGL enabled (glad)."
#endif

#include "vertexnova/testbed/gl/buffer_layout.h"
#include "vertexnova/testbed/gl/index_buffer.h"
#include "vertexnova/testbed/gl/vertex_buffer.h"

#include <initializer_list>

namespace vne {
namespace testbed {
namespace gl {

/**
 * @struct VertexElement
 * @brief Describes one attribute within a vertex buffer layout.
 *
 * @param name         Shader attribute name (for documentation only; location
 *                     is bound by the order attributes are declared).
 * @param count        Number of float components (1, 2, 3, or 4).
 * @param normalized   If true, integer values are normalized to [0, 1].
 */
struct VertexElement {
    const char* name{nullptr};
    int count{0};
    bool normalized{false};
};

/**
 * @class VertexArray
 * @brief RAII OpenGL vertex array object (VAO).
 *
 * Typical usage:
 * @code
 *   gl::VertexBuffer vbo(vertices, sizeof(vertices));
 *   gl::VertexArray  vao;
 *   vao.addVertexBuffer(vbo, {{"aPos", 3}, {"aColor", 3}});
 *   // Draw:
 *   vao.bind();
 *   glDrawArrays(GL_TRIANGLES, 0, vertex_count);
 * @endcode
 */
class VertexArray {
   public:
    VertexArray();
    ~VertexArray();

    VertexArray(const VertexArray&) = delete;
    VertexArray& operator=(const VertexArray&) = delete;
    VertexArray(VertexArray&&) = delete;
    VertexArray& operator=(VertexArray&&) = delete;

    /** @brief Bind as the current VAO. */
    void bind() const;

    /** @brief Unbind (restore VAO to 0). */
    void unbind() const;

    /**
     * @brief Bind a VertexBuffer and declare its attribute layout (BufferLayout).
     *
     * @param vb     The buffer whose data is described by @p layout.
     *               Must be alive for the lifetime of this VAO.
     * @param layout BufferLayout (ShaderDataType per element, stride, offsets).
     *               Attributes are assigned locations 0, 1, 2, … in order.
     */
    void addVertexBuffer(VertexBuffer& vb, const BufferLayout& layout);

    /**
     * @brief Bind a VertexBuffer with ad-hoc VertexElement list (legacy).
     *
     * @param vb     The buffer whose data is described by @p layout.
     * @param layout List of VertexElement descriptors (float components only).
     */
    void addVertexBuffer(VertexBuffer& vb, std::initializer_list<VertexElement> layout);

    /**
     * @brief Set the index buffer for indexed drawing (glDrawElements).
     *
     * Must be called while no VAO is bound, or the implementation will bind
     * this VAO and bind the index buffer so the VAO records the EBO.
     *
     * @param ib Index buffer (must be alive for the lifetime of this VAO).
     */
    void setIndexBuffer(IndexBuffer& ib);

    /**
     * @brief Index count from the last set index buffer (0 if none).
     */
    [[nodiscard]] std::size_t getIndexCount() const { return index_count_; }

   private:
    unsigned int vao_id_{0u};
    std::size_t index_count_{0u};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
