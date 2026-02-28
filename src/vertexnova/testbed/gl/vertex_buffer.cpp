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

#include "vertexnova/testbed/gl/vertex_buffer.h"

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif

namespace vne {
namespace testbed {
namespace gl {

VertexBuffer::VertexBuffer(const float* data, std::size_t byte_size) {
    glGenBuffers(1, &vbo_id_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(byte_size), data, GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexBuffer::VertexBuffer(std::size_t byte_size) {
    glGenBuffers(1, &vbo_id_);
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id_);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(byte_size), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

VertexBuffer::~VertexBuffer() {
    if (vbo_id_ != 0u) {
        glDeleteBuffers(1, &vbo_id_);
    }
}

void VertexBuffer::bind() const {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id_);
}

void VertexBuffer::unbind() const {
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void VertexBuffer::setData(const void* data, std::size_t byte_size) {
    glBindBuffer(GL_ARRAY_BUFFER, vbo_id_);
    glBufferSubData(GL_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(byte_size), data);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
