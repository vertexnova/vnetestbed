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

#include "vertexnova/testbed/gl/sampler2d.h"

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif

namespace vne {
namespace testbed {
namespace gl {

namespace {

GLenum wrapToGL(SamplerWrap w) {
    switch (w) {
        case SamplerWrap::eRepeat:
            return GL_REPEAT;
        case SamplerWrap::eClampToEdge:
            return GL_CLAMP_TO_EDGE;
        case SamplerWrap::eMirroredRepeat:
            return GL_MIRRORED_REPEAT;
    }
    return GL_REPEAT;
}

GLenum filterToGL(SamplerFilter f) {
    switch (f) {
        case SamplerFilter::eNearest:
            return GL_NEAREST;
        case SamplerFilter::eLinear:
            return GL_LINEAR;
    }
    return GL_LINEAR;
}

void setSamplerParams(uint32_t sampler_id, const Sampler2DDescriptor& desc) {
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, static_cast<GLint>(wrapToGL(desc.wrap_s)));
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, static_cast<GLint>(wrapToGL(desc.wrap_t)));
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, static_cast<GLint>(filterToGL(desc.min_filter)));
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, static_cast<GLint>(filterToGL(desc.mag_filter)));
}

}  // namespace

Sampler2D::Sampler2D()
    : Sampler2D(Sampler2DDescriptor{}) {}

Sampler2D::Sampler2D(const Sampler2DDescriptor& desc) {
    glGenSamplers(1, &sampler_id_);
    if (sampler_id_ != 0u) {
        setSamplerParams(sampler_id_, desc);
    }
}

Sampler2D::~Sampler2D() {
    if (sampler_id_ != 0u) {
        glDeleteSamplers(1, &sampler_id_);
    }
}

void Sampler2D::bind(uint32_t unit) const {
    if (sampler_id_ != 0u) {
        glBindSampler(static_cast<GLuint>(unit), sampler_id_);
    }
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
