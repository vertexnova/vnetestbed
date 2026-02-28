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

#include "vertexnova/testbed/app_context.h"

namespace vne {
namespace testbed {

// ---------------------------------------------------------------------------
// GLSL 4.10 / GLSL ES 3.00 shaders — triangle in NDC, per-vertex colour
// ---------------------------------------------------------------------------

#if defined(VNE_TESTBED_OPENGL)
static const char* kVertSrc = R"(
#version 410 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 vColor;
void main() {
    vColor      = aColor;
    gl_Position = vec4(aPos, 1.0);
}
)";

static const char* kFragSrc = R"(
#version 410 core
in  vec3 vColor;
out vec4 FragColor;
void main() {
    FragColor = vec4(vColor, 1.0);
}
)";
#else
// OpenGL ES 3.0 shaders
static const char* kVertSrc = R"(
#version 300 es
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aColor;
out vec3 vColor;
void main() {
    vColor      = aColor;
    gl_Position = vec4(aPos, 1.0);
}
)";

static const char* kFragSrc = R"(
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
// Vertex data: xyz positions (NDC) + rgb colours, interleaved
// ---------------------------------------------------------------------------

// clang-format off
static const float kVertices[] = {
    // pos               // color
     0.0f,  0.5f, 0.0f,   1.0f, 0.2f, 0.2f,   // top   — red
    -0.5f, -0.5f, 0.0f,   0.2f, 1.0f, 0.2f,   // left  — green
     0.5f, -0.5f, 0.0f,   0.2f, 0.2f, 1.0f,   // right — blue
};
// clang-format on

// ---------------------------------------------------------------------------

void TriangleDemoLayer::onAttach(AppContext& ctx) {
    device_ = ctx.device;
    if (!device_) {
        return;
    }

    shader_ = device_->compileShader(kVertSrc, kFragSrc);

    vbo_ = device_->createVertexBuffer(kVertices, sizeof(kVertices));

    // Pipeline: two float3 attributes (aPos @ loc 0, aColor @ loc 1).
    // Default depth/blend/rasterizer — opaque, depth test on, back-cull.
    PipelineDesc desc;
    desc.shader = shader_;
    desc.layout = {{3}, {3}};  // {num_floats} for aPos, aColor
    pipeline_ = device_->createPipeline(desc);
}

void TriangleDemoLayer::onDetach() {
    if (!device_) {
        return;
    }
    device_->destroy(pipeline_);
    device_->destroy(vbo_);
    device_->destroy(shader_);
    device_ = nullptr;
}

void TriangleDemoLayer::onRender(const RenderContext& /*ctx*/) {
    if (!device_ || !pipeline_.isValid()) {
        return;
    }
    DebugGroupScope _{*device_, "TriangleDemoLayer"};
    device_->draw(pipeline_, vbo_, 3u, DrawMode::eTriangles);
}

}  // namespace testbed
}  // namespace vne
