#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * MeshRenderer: implements IRenderer; draws meshes with Phong material.
 * Does not load meshes; MeshLayer owns VBO/IBO and calls drawMesh.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/renderer/irenderer.h"
#include "vertexnova/testbed/renderer/phong_material.h"
#include "vertexnova/testbed/render_device.h"

#include "vertexnova/math/core/core.h"

namespace vne {
namespace scene {
class ICamera;
}
}  // namespace vne

namespace vne {
namespace testbed {

/**
 * @class MeshRenderer
 * @brief Renders indexed meshes with Blinn-Phong; owns PhongMaterial and pipeline.
 */
class MeshRenderer : public IRenderer {
   public:
    MeshRenderer();
    ~MeshRenderer() override;

    MeshRenderer(const MeshRenderer&) = delete;
    MeshRenderer& operator=(const MeshRenderer&) = delete;
    MeshRenderer(MeshRenderer&&) = delete;
    MeshRenderer& operator=(MeshRenderer&&) = delete;

    [[nodiscard]] bool init(IRenderDevice* device) override;
    void shutdown() override;
    void resize(int width, int height) override;

    /**
     * @brief Draw an indexed mesh with the given model matrix and lights.
     * @param device       Render device.
     * @param camera       Camera for view-projection and position.
     * @param vbo          Vertex buffer (position 3 + normal 3 + color 3).
     * @param ibo          Index buffer.
     * @param index_count  Number of indices to draw.
     * @param model        Model matrix.
     * @param lights       Phong light parameters (ambient, dir, point, spot, attn).
     */
    void drawMesh(IRenderDevice* device,
                  vne::scene::ICamera* camera,
                  BufferHandle vbo,
                  BufferHandle ibo,
                  uint32_t index_count,
                  const vne::math::Mat4f& model,
                  const PhongLightParams& lights);

   private:
    PhongMaterial phong_material_;
    IRenderDevice* device_{nullptr};
    int framebuffer_width_{0};
    int framebuffer_height_{0};
};

}  // namespace testbed
}  // namespace vne
