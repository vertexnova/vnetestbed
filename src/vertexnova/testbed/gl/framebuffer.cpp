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

#include "vertexnova/testbed/gl/framebuffer.h"

#if defined(VNE_TESTBED_OPENGL)
#  include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#  include <glad/glad_es3.h>
#endif

namespace vne {
namespace testbed {
namespace gl {

Framebuffer::Framebuffer(const FramebufferDescriptor& desc)
    : width_(desc.width), height_(desc.height) {
    if (width_ == 0u || height_ == 0u) {
        return;
    }

    glGenFramebuffers(1, &fbo_id_);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);

    // Color attachment (RGBA8 texture)
    glGenTextures(1, &color_texture_id_);
    glBindTexture(GL_TEXTURE_2D, color_texture_id_);
    glTexImage2D(GL_TEXTURE_2D,
                 0,
                 GL_RGBA8,
                 static_cast<GLsizei>(width_),
                 static_cast<GLsizei>(height_),
                 0,
                 GL_RGBA,
                 GL_UNSIGNED_BYTE,
                 nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
                           GL_COLOR_ATTACHMENT0,
                           GL_TEXTURE_2D,
                           color_texture_id_,
                           0);
    glBindTexture(GL_TEXTURE_2D, 0);

    // Optional depth attachment (renderbuffer)
    if (desc.attach_depth) {
        glGenRenderbuffers(1, &depth_rbo_id_);
        glBindRenderbuffer(GL_RENDERBUFFER, depth_rbo_id_);
        glRenderbufferStorage(GL_RENDERBUFFER,
                              GL_DEPTH_COMPONENT24,
                              static_cast<GLsizei>(width_),
                              static_cast<GLsizei>(height_));
        glFramebufferRenderbuffer(GL_FRAMEBUFFER,
                                  GL_DEPTH_ATTACHMENT,
                                  GL_RENDERBUFFER,
                                  depth_rbo_id_);
        glBindRenderbuffer(GL_RENDERBUFFER, 0);
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        glDeleteFramebuffers(1, &fbo_id_);
        glDeleteTextures(1, &color_texture_id_);
        if (depth_rbo_id_ != 0u) {
            glDeleteRenderbuffers(1, &depth_rbo_id_);
            depth_rbo_id_ = 0u;
        }
        fbo_id_ = 0u;
        color_texture_id_ = 0u;
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

Framebuffer::~Framebuffer() {
    if (fbo_id_ != 0u) {
        glDeleteFramebuffers(1, &fbo_id_);
        fbo_id_ = 0u;
    }
    if (color_texture_id_ != 0u) {
        glDeleteTextures(1, &color_texture_id_);
        color_texture_id_ = 0u;
    }
    if (depth_rbo_id_ != 0u) {
        glDeleteRenderbuffers(1, &depth_rbo_id_);
        depth_rbo_id_ = 0u;
    }
}

void Framebuffer::bind() const {
    if (fbo_id_ != 0u) {
        glBindFramebuffer(GL_FRAMEBUFFER, fbo_id_);
    }
}

void Framebuffer::unbind() const {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
