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

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif

namespace vne {
namespace testbed {
namespace gl {

namespace {

GLenum shaderDataTypeToGL(ShaderDataType type) {
    switch (type) {
        case ShaderDataType::eFloat:
        case ShaderDataType::eFloat2:
        case ShaderDataType::eFloat3:
        case ShaderDataType::eFloat4:
        case ShaderDataType::eMat3:
        case ShaderDataType::eMat4:
            return GL_FLOAT;
        case ShaderDataType::eInt:
        case ShaderDataType::eInt2:
        case ShaderDataType::eInt3:
        case ShaderDataType::eInt4:
            return GL_INT;
        case ShaderDataType::eBool:
            // GL_BOOL is not valid for glVertexAttribIPointer; use GL_UNSIGNED_BYTE (1 byte, 0/1).
            return GL_UNSIGNED_BYTE;
        case ShaderDataType::eNone:
        default:
            return GL_FLOAT;
    }
}

bool isIntegerType(ShaderDataType type) {
    return type == ShaderDataType::eInt || type == ShaderDataType::eInt2 || type == ShaderDataType::eInt3
           || type == ShaderDataType::eInt4 || type == ShaderDataType::eBool;
}

}  // namespace

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

void VertexArray::addVertexBuffer(VertexBuffer& vb, const BufferLayout& layout) {
    glBindVertexArray(vao_id_);
    vb.bind();

    const std::size_t stride = layout.getStride();
    const GLsizei gl_stride = static_cast<GLsizei>(stride);
    GLuint location = 0u;
    for (const auto& elem : layout) {
        const GLenum gl_type = shaderDataTypeToGL(elem.type);
        const GLboolean normalized = elem.normalized ? GL_TRUE : GL_FALSE;

        if (elem.type == ShaderDataType::eMat3) {
            for (uint32_t col = 0; col < 3; ++col) {
                glEnableVertexAttribArray(location);
                const void* offset_ptr = reinterpret_cast<const void*>(elem.offset + col * 3 * sizeof(float));
                glVertexAttribPointer(location, 3, GL_FLOAT, normalized, gl_stride, offset_ptr);
                ++location;
            }
        } else if (elem.type == ShaderDataType::eMat4) {
            for (uint32_t col = 0; col < 4; ++col) {
                glEnableVertexAttribArray(location);
                const void* offset_ptr = reinterpret_cast<const void*>(elem.offset + col * 4 * sizeof(float));
                glVertexAttribPointer(location, 4, GL_FLOAT, normalized, gl_stride, offset_ptr);
                ++location;
            }
        } else {
            glEnableVertexAttribArray(location);
            const void* offset_ptr = reinterpret_cast<const void*>(elem.offset);
            if (isIntegerType(elem.type)) {
                glVertexAttribIPointer(location,
                                       static_cast<GLint>(elem.getComponentCount()),
                                       gl_type,
                                       gl_stride,
                                       offset_ptr);
            } else {
                glVertexAttribPointer(location,
                                      static_cast<GLint>(elem.getComponentCount()),
                                      gl_type,
                                      normalized,
                                      gl_stride,
                                      offset_ptr);
            }
            ++location;
        }
    }

    glBindVertexArray(0);
    vb.unbind();
}

void VertexArray::addVertexBuffer(VertexBuffer& vb, std::initializer_list<VertexElement> layout) {
    glBindVertexArray(vao_id_);
    vb.bind();

    std::size_t stride = 0u;
    for (const auto& elem : layout) {
        stride += static_cast<std::size_t>(elem.count) * sizeof(float);
    }

    std::size_t offset = 0u;
    GLuint location = 0u;
    for (const auto& elem : layout) {
        glEnableVertexAttribArray(location);
        glVertexAttribPointer(location,
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

void VertexArray::setIndexBuffer(IndexBuffer& ib) {
    glBindVertexArray(vao_id_);
    ib.bind();
    index_count_ = ib.getCount();
    glBindVertexArray(0);
    ib.unbind();
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
