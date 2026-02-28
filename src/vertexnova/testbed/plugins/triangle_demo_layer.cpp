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

#include "vertexnova/testbed/plugins/triangle_demo_layer.h"

#include <glad/glad.h>

namespace vne {
namespace testbed {

// ---------------------------------------------------------------------------
// GLSL 4.10 shaders — triangle in NDC, per-vertex colour
// ---------------------------------------------------------------------------

static const char* kTriVertSrc = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;

out vec3 vColor;

void main() {
    vColor      = aColor;
    gl_Position = vec4(aPos, 1.0);
}
)";

static const char* kTriFragSrc = R"(
#version 410 core
in  vec3 vColor;
out vec4 FragColor;

void main() {
    FragColor = vec4(vColor, 1.0);
}
)";

// ---------------------------------------------------------------------------
// Vertex data: positions (NDC) + RGB colours, interleaved
// ---------------------------------------------------------------------------

// clang-format off
static const float kTriangleVertices[] = {
    // pos             // color
     0.0f,  0.5f, 0.0f,   1.0f, 0.2f, 0.2f,   // top    — red
    -0.5f, -0.5f, 0.0f,   0.2f, 1.0f, 0.2f,   // left   — green
     0.5f, -0.5f, 0.0f,   0.2f, 0.2f, 1.0f,   // right  — blue
};
// clang-format on

// ---------------------------------------------------------------------------

void TriangleDemoLayer::onAttach(AppContext& /*ctx*/) {
    shader_ = std::make_unique<gl::Shader>(kTriVertSrc, kTriFragSrc);
    vbo_    = std::make_unique<gl::VertexBuffer>(kTriangleVertices, sizeof(kTriangleVertices));
    vao_    = std::make_unique<gl::VertexArray>();
    vao_->addVertexBuffer(*vbo_, {{"aPos", 3}, {"aColor", 3}});
}

void TriangleDemoLayer::onDetach() {
    vao_.reset();
    vbo_.reset();
    shader_.reset();
}

void TriangleDemoLayer::onRender(const RenderContext& /*ctx*/) {
    if (!shader_ || !shader_->isValid()) {
        return;
    }
    shader_->bind();
    vao_->bind();
    glDrawArrays(GL_TRIANGLES, 0, 3);
    vao_->unbind();
    shader_->unbind();
}

}  // namespace testbed
}  // namespace vne
