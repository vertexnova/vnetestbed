#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * MeshLayer: loads a mesh from path (vneio when available), uploads to GPU,
 * draws with scene shaders. Camera provided via setCameraProvider().
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"
#include "vertexnova/testbed/render_device.h"

#include "vertexnova/scene/camera/camera.h"

#include <functional>
#include <memory>
#include <string>

namespace vne {
namespace testbed {

class MeshRenderer;

/**
 * @class MeshLayer
 * @brief Layer that loads a single mesh from file and draws it each frame via MeshRenderer.
 *
 * Requires mesh path (setMeshPath), camera provider (setCameraProvider), and
 * AppContext with coreRenderer (getMeshRenderer() non-null) in onAttach.
 * When VNE_TESTBED_HAVE_VNEIO is defined, the mesh is loaded via Assimp; otherwise
 * the layer does nothing. Vertex layout: position (3) + normal (3) + color (3).
 */
class MeshLayer : public ILayer {
   public:
    using CameraProvider = std::function<std::shared_ptr<vne::scene::ICamera>(int viewport_index)>;

    MeshLayer();

    void setMeshPath(std::string path);
    void setCameraProvider(CameraProvider provider);

    /**
     * @brief Reload the mesh from a new path at runtime.
     *
     * Destroys the current GPU buffers (if any) and loads the mesh from @p path.
     * Safe to call any time after onAttach; no-op before device is available.
     * @param path Absolute or resolved path to the mesh file.
     */
    void reloadMesh(std::string path);

    /** @brief Returns the currently loaded mesh path (empty if none). */
    [[nodiscard]] const std::string& getMeshPath() const { return mesh_path_; }

    void onAttach(AppContext& ctx) override;
    void onDetach() override;
    void onRender(const RenderContext& ctx) override;

   private:
    void loadMeshFromPath();

    std::string mesh_path_;
    CameraProvider camera_provider_;

    IRenderDevice* device_{nullptr};
    MeshRenderer* mesh_renderer_{nullptr};
    BufferHandle vbo_{};
    BufferHandle ibo_{};

    uint32_t index_count_{0};
    bool ready_{false};
};

}  // namespace testbed
}  // namespace vne
