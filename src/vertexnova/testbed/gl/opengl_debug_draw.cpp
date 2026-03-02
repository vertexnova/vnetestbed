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

// Expand each line segment into a quad so thickness is visible (glLineWidth is ignored on macOS Core Profile)
static const char* kDebugGeomSrc = R"(
#version 410 core
layout(lines) in;
layout(triangle_strip, max_vertices = 4) out;

in vec3 vColor[];

out vec3 fColor;

uniform float uHalfWidth = 0.004;

void main() {
    vec4 a = gl_in[0].gl_Position;
    vec4 b = gl_in[1].gl_Position;
    vec2 ndc0 = a.xy / a.w;
    vec2 ndc1 = b.xy / b.w;
    vec2 dir = ndc1 - ndc0;
    float len = length(dir);
    vec2 perp = len > 1e-6 ? vec2(-dir.y, dir.x) / len : vec2(1.0, 0.0);
    perp *= uHalfWidth;

    vec4 v0 = vec4((ndc0 + perp) * a.w, a.z, a.w);
    vec4 v1 = vec4((ndc0 - perp) * a.w, a.z, a.w);
    vec4 v2 = vec4((ndc1 + perp) * b.w, b.z, b.w);
    vec4 v3 = vec4((ndc1 - perp) * b.w, b.z, b.w);

    fColor = vColor[0];
    gl_Position = v0; EmitVertex();
    gl_Position = v1; EmitVertex();
    gl_Position = v2; EmitVertex();
    gl_Position = v3; EmitVertex();
    EndPrimitive();
}
)";

static const char* kDebugFragSrc = R"(
#version 410 core
in  vec3 fColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(fColor, 1.0);
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

#if defined(VNE_TESTBED_OPENGL)
    // Use geometry shader to draw lines as quads (thick lines work on macOS Core Profile)
    shader_ = std::make_unique<Shader>(kDebugVertSrc, kDebugGeomSrc, kDebugFragSrc);
#else
    shader_ = std::make_unique<Shader>(kDebugVertSrc, kDebugFragSrc);
#endif
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
#if defined(VNE_TESTBED_OPENGL)
    shader_->setFloat("uHalfWidth", 0.004f);
#endif
    // Don't write depth so overlapping line quads don't z-fight
    GLboolean depth_mask_prev = GL_TRUE;
    glGetBooleanv(GL_DEPTH_WRITEMASK, &depth_mask_prev);
    glDepthMask(GL_FALSE);
    // Disable depth test so debug lines (camera frustum, grid, axes) are always visible and don't flicker
    GLboolean depth_test_prev = glIsEnabled(GL_DEPTH_TEST);
    glDisable(GL_DEPTH_TEST);
    glDrawArrays(GL_LINES, 0, static_cast<GLsizei>(vertex_data_.size() / kFloatsPerVertex));
    if (depth_test_prev) {
        glEnable(GL_DEPTH_TEST);
    }
    glDepthMask(depth_mask_prev);
    vao_->unbind();

    shader_->unbind();
    vertex_data_.clear();
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
