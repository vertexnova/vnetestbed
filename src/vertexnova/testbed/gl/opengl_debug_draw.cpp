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

#include "vertexnova/testbed/gl/opengl_debug_draw.h"

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif

namespace vne {
namespace testbed {
namespace gl {

// ---------------------------------------------------------------------------
// GLSL 4.10 / GLSL ES 3.00 shaders for debug line rendering
// ---------------------------------------------------------------------------

#if defined(VNE_TESTBED_OPENGL)
static const char* kDebugVertSrc = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 uVP;

out vec3 vColor;

void main() {
    vColor = aColor;
    gl_Position = uVP * vec4(aPos, 1.0);
}
)";

static const char* kDebugFragSrc = R"(
#version 410 core
in  vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
)";
#else  // OpenGL ES 3.0
static const char* kDebugVertSrc = R"(
#version 300 es
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

uniform mat4 uVP;

out vec3 vColor;

void main() {
    vColor = aColor;
    gl_Position = uVP * vec4(aPos, 1.0);
}
)";

static const char* kDebugFragSrc = R"(
#version 300 es
precision mediump float;
in  vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
)";
#endif

// ---------------------------------------------------------------------------

OpenGLDebugDraw::~OpenGLDebugDraw() {
    if (initialized_) {
        shutdown();
    }
}

bool OpenGLDebugDraw::init() {
    if (initialized_) {
        return true;
    }

    shader_ = std::make_unique<Shader>(kDebugVertSrc, kDebugFragSrc);
    if (!shader_->isValid()) {
        return false;
    }

    // Dynamic VBO sized to hold kMaxLineVertices vertices (2 per line → 32 K lines).
    vbo_ = std::make_unique<VertexBuffer>(kMaxLineVertices * kFloatsPerVertex * sizeof(float));

    vao_ = std::make_unique<VertexArray>();
    vao_->addVertexBuffer(*vbo_, {{"aPos", 3}, {"aColor", 3}});

    vertex_data_.reserve(kMaxLineVertices * kFloatsPerVertex);
    initialized_ = true;
    return true;
}

void OpenGLDebugDraw::shutdown() {
    vao_.reset();
    vbo_.reset();
    shader_.reset();
    vertex_data_.clear();
    initialized_ = false;
}

void OpenGLDebugDraw::setViewProjectionMatrix(const vne::math::Mat4f& vp) {
    vp_matrix_ = vp;
}

// ---------------------------------------------------------------------------
// Queue primitives
// ---------------------------------------------------------------------------

void OpenGLDebugDraw::line(vne::math::Vec3f from, vne::math::Vec3f to, vne::math::Vec3f color) {
    // Drop the line silently if the buffer is already full.
    if (vertex_data_.size() + 2 * kFloatsPerVertex > kMaxLineVertices * kFloatsPerVertex) {
        return;
    }
    // Vertex 1
    vertex_data_.push_back(from[0]);
    vertex_data_.push_back(from[1]);
    vertex_data_.push_back(from[2]);
    vertex_data_.push_back(color[0]);
    vertex_data_.push_back(color[1]);
    vertex_data_.push_back(color[2]);
    // Vertex 2
    vertex_data_.push_back(to[0]);
    vertex_data_.push_back(to[1]);
    vertex_data_.push_back(to[2]);
    vertex_data_.push_back(color[0]);
    vertex_data_.push_back(color[1]);
    vertex_data_.push_back(color[2]);
}

void OpenGLDebugDraw::aabb(DebugAabb box, vne::math::Vec3f color) {
    // Draw 12 edges of an AABB as 12 line segments.
    auto& mn = box.min;
    auto& mx = box.max;

    // Bottom face
    line({mn[0], mn[1], mn[2]}, {mx[0], mn[1], mn[2]}, color);
    line({mx[0], mn[1], mn[2]}, {mx[0], mn[1], mx[2]}, color);
    line({mx[0], mn[1], mx[2]}, {mn[0], mn[1], mx[2]}, color);
    line({mn[0], mn[1], mx[2]}, {mn[0], mn[1], mn[2]}, color);
    // Top face
    line({mn[0], mx[1], mn[2]}, {mx[0], mx[1], mn[2]}, color);
    line({mx[0], mx[1], mn[2]}, {mx[0], mx[1], mx[2]}, color);
    line({mx[0], mx[1], mx[2]}, {mn[0], mx[1], mx[2]}, color);
    line({mn[0], mx[1], mx[2]}, {mn[0], mx[1], mn[2]}, color);
    // Vertical pillars
    line({mn[0], mn[1], mn[2]}, {mn[0], mx[1], mn[2]}, color);
    line({mx[0], mn[1], mn[2]}, {mx[0], mx[1], mn[2]}, color);
    line({mx[0], mn[1], mx[2]}, {mx[0], mx[1], mx[2]}, color);
    line({mn[0], mn[1], mx[2]}, {mn[0], mx[1], mx[2]}, color);
}

// ---------------------------------------------------------------------------
// Flush
// ---------------------------------------------------------------------------

void OpenGLDebugDraw::flush() {
    if (!initialized_ || vertex_data_.empty()) {
        return;
    }

    const std::size_t byte_size = vertex_data_.size() * sizeof(float);
    vbo_->setData(vertex_data_.data(), byte_size);

    shader_->bind();
    shader_->setMat4("uVP", vp_matrix_);

    vao_->bind();
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertex_data_.size() / kFloatsPerVertex));
    vao_->unbind();

    shader_->unbind();
    vertex_data_.clear();
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
