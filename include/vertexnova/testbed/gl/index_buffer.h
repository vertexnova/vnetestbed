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
 * @file gl/index_buffer.h
 * @brief RAII wrapper for an OpenGL element buffer object (EBO).
 *
 * 32-bit indices only. Use with VertexArray::setIndexBuffer() for indexed
 * drawing (glDrawElements).
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#ifndef VNE_TESTBED_OPENGL
#error "index_buffer.h requires VNE_TESTBED_OPENGL to be defined. Build with OpenGL enabled (glad)."
#endif

#include <cstddef>
#include <cstdint>

namespace vne {
namespace testbed {
namespace gl {

/**
 * @class IndexBuffer
 * @brief RAII OpenGL element buffer object (GL_ELEMENT_ARRAY_BUFFER).
 *
 * Static: pass indices on construction. Dynamic: construct with count only,
 * then call setData() each frame before drawing.
 */
class IndexBuffer {
   public:
    /**
     * @brief Create a static index buffer and upload initial data.
     * @param indices   Pointer to uint32_t indices.
     * @param count     Number of indices.
     */
    IndexBuffer(const uint32_t* indices, std::size_t count);

    /**
     * @brief Create an empty dynamic index buffer of a given capacity.
     * @param count     Maximum number of indices the buffer will hold.
     */
    explicit IndexBuffer(std::size_t count);

    ~IndexBuffer();

    IndexBuffer(const IndexBuffer&) = delete;
    IndexBuffer& operator=(const IndexBuffer&) = delete;
    IndexBuffer(IndexBuffer&&) = delete;
    IndexBuffer& operator=(IndexBuffer&&) = delete;

    /** @brief Bind as the current GL_ELEMENT_ARRAY_BUFFER. */
    void bind() const;

    /** @brief Unbind (restore GL_ELEMENT_ARRAY_BUFFER to 0). */
    void unbind() const;

    /**
     * @brief Upload new data to a dynamic buffer.
     * @param data      Pointer to index data.
     * @param byte_size Number of bytes to upload (must not exceed capacity).
     */
    void setData(const void* data, std::size_t byte_size);

    /** @brief Number of indices (for glDrawElements count). */
    [[nodiscard]] std::size_t getCount() const { return count_; }

   private:
    unsigned int ebo_id_{0u};
    std::size_t count_{0u};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
