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

#include "vertexnova/testbed/gl/vertex_array.h"

#include <glad/glad.h>

namespace vne {
namespace testbed {
namespace gl {

VertexArray::VertexArray() {
    glGenVertexArrays(1, &vao_id_);
}

VertexArray::~VertexArray() {
    if (vao_id_ != 0u) {
        glDeleteVertexArrays(1, &vao_id_);
    }
}

void VertexArray::bind() const {
    glBindVertexArray(vao_id_);
}

void VertexArray::unbind() const {
    glBindVertexArray(0);
}

void VertexArray::addVertexBuffer(VertexBuffer& vb,
                                  std::initializer_list<VertexElement> layout) {
    glBindVertexArray(vao_id_);
    vb.bind();

    // Calculate stride from the total component count across all attributes.
    std::size_t stride = 0u;
    for (const auto& elem : layout) {
        stride += static_cast<std::size_t>(elem.count) * sizeof(float);
    }

    std::size_t offset = 0u;
    GLuint location = 0u;
    for (const auto& elem : layout) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(
            location,
            elem.count,
            GL_FLOAT,
            elem.normalized ? GL_TRUE : GL_FALSE,
            static_cast<GLsizei>(stride),
            reinterpret_cast<const void*>(offset));
        offset += static_cast<std::size_t>(elem.count) * sizeof(float);
        ++location;
    }

    glBindVertexArray(0);
    vb.unbind();
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
