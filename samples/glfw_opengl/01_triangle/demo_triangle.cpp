/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Sample 01_triangle: demo that draws a single coloured triangle.
 * The demo triangle layer is defined in this file.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_device.h"

#include <memory>

namespace {

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
// Demo triangle layer — draws a single coloured triangle via IRenderDevice
// ---------------------------------------------------------------------------

class DemoTriangleLayer : public vne::testbed::ILayer {
   public:
    DemoTriangleLayer()
        : vne::testbed::ILayer("DemoTriangleLayer") {}

    void onAttach(vne::testbed::AppContext& ctx) override {
        device_ = ctx.device;
        if (!device_) {
            return;
        }
        shader_ = device_->compileShader(kVertSrc, kFragSrc);
        vbo_ = device_->createVertexBuffer(kVertices, sizeof(kVertices));
        vne::testbed::PipelineDesc desc;
        desc.shader = shader_;
        desc.layout = {{3}, {3}};  // aPos @ loc 0, aColor @ loc 1
        pipeline_ = device_->createPipeline(desc);
    }

    void onDetach() override {
        if (!device_) {
            return;
        }
        device_->destroy(pipeline_);
        device_->destroy(vbo_);
        device_->destroy(shader_);
        device_ = nullptr;
    }

    void onRender(const vne::testbed::RenderContext& /*ctx*/) override {
        if (!device_ || !pipeline_.isValid()) {
            return;
        }
        vne::testbed::DebugGroupScope _{*device_, "DemoTriangleLayer"};
        device_->draw(pipeline_, vbo_, 3u, vne::testbed::DrawMode::eTriangles);
    }

   private:
    vne::testbed::IRenderDevice* device_{nullptr};
    vne::testbed::ShaderHandle shader_;
    vne::testbed::BufferHandle vbo_;
    vne::testbed::PipelineHandle pipeline_;
};

// ---------------------------------------------------------------------------

void RegisterTriangleDemo(vne::testbed::Application& app) {
    app.getLayerStack().pushLayer(std::make_unique<DemoTriangleLayer>(), app.getAppContext());
}

}  // namespace

VNETESTBED_REGISTER_DEMO("triangle", RegisterTriangleDemo)
