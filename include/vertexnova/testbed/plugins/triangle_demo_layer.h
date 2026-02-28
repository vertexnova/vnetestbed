#pragma once
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

/**
 * @file plugins/triangle_demo_layer.h
 * @brief Minimal demo layer: draws a single coloured triangle.
 *
 * Validates the full OpenGL render pipeline inside vnetestbed:
 *   onAttach  → compile shader, upload VBO/VAO
 *   onRender  → bind + draw
 *   onDetach  → destroy GPU resources (RAII, automatic)
 *
 * Only compiled when VNE_TESTBED_OPENGL is defined.
 */

#ifndef VNE_TESTBED_OPENGL
#error "triangle_demo_layer.h requires VNE_TESTBED_OPENGL to be defined. Build with OpenGL enabled (glad)."
#endif

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/gl/shader.h"
#include "vertexnova/testbed/gl/vertex_buffer.h"
#include "vertexnova/testbed/gl/vertex_array.h"

#include <memory>

namespace vne {
namespace testbed {

/**
 * @class TriangleDemoLayer
 * @brief ILayer that draws a static coloured triangle using the gl/ primitives.
 *
 * No camera: the triangle is defined in NDC space so it is always centred.
 * Purpose: confirm that Shader, VertexBuffer, and VertexArray work end-to-end.
 */
class TriangleDemoLayer : public ILayer {
   public:
    TriangleDemoLayer()
        : ILayer("TriangleDemoLayer") {}

    void onAttach(AppContext& ctx) override;
    void onDetach() override;
    void onRender(const RenderContext& ctx) override;

   private:
    std::unique_ptr<gl::Shader> shader_;
    std::unique_ptr<gl::VertexBuffer> vbo_;
    std::unique_ptr<gl::VertexArray> vao_;
};

}  // namespace testbed
}  // namespace vne
