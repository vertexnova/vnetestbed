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
 * Validates the full render pipeline inside vnetestbed via the backend-agnostic
 * IRenderDevice interface.  No gl/ headers are included here; the concrete
 * backend is selected by the runner.
 *
 *   onAttach  → compileShader/createShader + createVertexBuffer + createPipeline
 *   onRender  → DebugGroupScope + draw
 *   onDetach  → destroy pipeline, buffer, shader
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_device.h"

namespace vne {
namespace testbed {

/**
 * @class TriangleDemoLayer
 * @brief ILayer that draws a static coloured triangle via IRenderDevice.
 *
 * No camera: the triangle is defined in NDC space so it is always centred.
 * Purpose: confirm that pipeline creation, vertex layout, and draw work
 * end-to-end for any backend.
 */
class TriangleDemoLayer : public ILayer {
   public:
    TriangleDemoLayer()
        : ILayer("TriangleDemoLayer") {}

    void onAttach(AppContext& ctx) override;
    void onDetach() override;
    void onRender(const RenderContext& ctx) override;

   private:
    IRenderDevice* device_{nullptr};  ///< Non-owning; obtained from AppContext.
    ShaderHandle shader_;
    BufferHandle vbo_;
    PipelineHandle pipeline_;
};

}  // namespace testbed
}  // namespace vne
