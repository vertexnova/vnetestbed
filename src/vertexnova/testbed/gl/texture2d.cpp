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

#include "vertexnova/testbed/gl/texture2d.h"

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif

namespace vne {
namespace testbed {
namespace gl {

namespace {

void formatToGL(Texture2DFormat format, GLenum* out_internal, GLenum* out_format, GLenum* out_type) {
    switch (format) {
        case Texture2DFormat::RGBA8:
            *out_internal = GL_RGBA8;
            *out_format = GL_RGBA;
            *out_type = GL_UNSIGNED_BYTE;
            break;
        case Texture2DFormat::RGB8:
            *out_internal = GL_RGB8;
            *out_format = GL_RGB;
            *out_type = GL_UNSIGNED_BYTE;
            break;
        case Texture2DFormat::R8:
            *out_internal = GL_R8;
            *out_format = GL_RED;
            *out_type = GL_UNSIGNED_BYTE;
            break;
    }
}

}  // namespace

Texture2D::Texture2D(const Texture2DDescriptor& desc)
    : width_(desc.width)
    , height_(desc.height)
    , format_(desc.format) {
    if (width_ == 0u || height_ == 0u) {
        return;
    }

    GLenum internal_format = GL_RGBA8;
    GLenum pixel_format = GL_RGBA;
    GLenum pixel_type = GL_UNSIGNED_BYTE;
    formatToGL(format_, &internal_format, &pixel_format, &pixel_type);

    glGenTextures(1, &texture_id_);
    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 static_cast<GLint>(internal_format),
                 static_cast<GLsizei>(width_),
                 static_cast<GLsizei>(height_),
                 0,
                 pixel_format,
                 pixel_type,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glBindTexture(GL_TEXTURE_2D, 0);
}

Texture2D::~Texture2D() {
    if (texture_id_ != 0u) {
        glDeleteTextures(1, &texture_id_);
    }
}

bool Texture2D::updateData(uint32_t mip_level, uint32_t array_layer, const void* data, std::size_t data_size) {
    (void)array_layer;  // 2D non-array: only layer 0
    if (texture_id_ == 0u || data == nullptr) {
        return false;
    }

    GLenum internal_format = GL_RGBA8;
    GLenum pixel_format = GL_RGBA;
    GLenum pixel_type = GL_UNSIGNED_BYTE;
    formatToGL(format_, &internal_format, &pixel_format, &pixel_type);

    // For mip 0, dimensions are width_ x height_; for higher mips we could compute.
    GLsizei w = static_cast<GLsizei>(width_);
    GLsizei h = static_cast<GLsizei>(height_);
    if (mip_level > 0u) {
        for (uint32_t i = 0; i < mip_level && (w > 1 || h > 1); ++i) {
            w = (w > 1) ? (w / 2) : 1;
            h = (h > 1) ? (h / 2) : 1;
        }
    }

    glBindTexture(GL_TEXTURE_2D, texture_id_);
    glTexSubImage2D(GL_TEXTURE_2D, static_cast<GLint>(mip_level), 0, 0, w, h, pixel_format, pixel_type, data);
    glBindTexture(GL_TEXTURE_2D, 0);

    (void)data_size;  // Caller responsible for correct size
    return true;
}

void Texture2D::bind(uint32_t unit) const {
    if (texture_id_ == 0u) {
        return;
    }
    glActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    glBindTexture(GL_TEXTURE_2D, texture_id_);
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
