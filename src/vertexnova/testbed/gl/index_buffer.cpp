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

#include "vertexnova/testbed/gl/index_buffer.h"

#if defined(VNE_TESTBED_OPENGL)
#  include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#  include <glad/glad_es3.h>
#endif

namespace vne {
namespace testbed {
namespace gl {

IndexBuffer::IndexBuffer(const uint32_t* indices, std::size_t count) : count_(count) {
    glGenBuffers(1, &ebo_id_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(count * sizeof(uint32_t)),
                 indices,
                 GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

IndexBuffer::IndexBuffer(std::size_t count) : count_(count) {
    glGenBuffers(1, &ebo_id_);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id_);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(count * sizeof(uint32_t)),
                 nullptr,
                 GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

IndexBuffer::~IndexBuffer() {
    if (ebo_id_ != 0u) {
        glDeleteBuffers(1, &ebo_id_);
    }
}

void IndexBuffer::bind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id_);
}

void IndexBuffer::unbind() const {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void IndexBuffer::setData(const void* data, std::size_t byte_size) {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo_id_);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, static_cast<GLsizeiptr>(byte_size), data);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
