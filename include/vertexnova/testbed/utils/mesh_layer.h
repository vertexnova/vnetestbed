#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * MeshLayer: loads a mesh from path (vneio when available), uploads to GPU,
 * draws with MeshRenderer (Blinn-Phong). Lights are managed as vnescene ILight
 * objects in a SceneState; buildLightParams() converts them to PhongLightParams
 * just before each draw call.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"
#include "vertexnova/testbed/render_device.h"
#include "vertexnova/testbed/renderer/phong_material.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/light/ambient_light.h"
#include "vertexnova/scene/light/directional_light.h"
#include "vertexnova/scene/light/point_light.h"
#include "vertexnova/scene/scene_state.h"

#include <functional>
#include <memory>
#include <string>

namespace vne {
namespace testbed {

class MeshRenderer;

/**
 * @class MeshLayer
 * @brief Layer that loads a single mesh from file and draws it with MeshRenderer.
 *
 * Lights are stored as vnescene ILight objects in `scene_state_`.  Call
 * getAmbientLight() / getDirectionalLight() / addPointLight() to configure them.
 * buildLightParams() converts them to PhongLightParams every frame.
 *
 * Vertex layout: position (3) + normal (3) + color (3).
 */
class MeshLayer : public ILayer {
   public:
    using CameraProvider = std::function<std::shared_ptr<vne::scene::ICamera>(int viewport_index)>;

    MeshLayer();

    // ---- Mesh path and camera ----
    void setMeshPath(std::string path);
    void setCameraProvider(CameraProvider provider);

    /**
     * @brief Reload mesh at runtime. Destroys old GPU buffers and loads from new path.
     * Safe after onAttach; stores path and defers load if device not yet available.
     */
    void reloadMesh(std::string path);

    /** @brief Returns the currently loaded mesh path (empty if none). */
    [[nodiscard]] const std::string& getMeshPath() const { return mesh_path_; }

    // ---- Scene lights (vnescene objects) ----

    /** @brief Access the SceneState holding all lights. */
    [[nodiscard]] vne::scene::SceneState& getSceneState() { return scene_state_; }
    [[nodiscard]] const vne::scene::SceneState& getSceneState() const { return scene_state_; }

    /** @brief Convenience: typed pointer to the default ambient light (always present). */
    [[nodiscard]] std::shared_ptr<vne::scene::AmbientLight> getAmbientLight() const { return ambient_light_; }

    /** @brief Convenience: typed pointer to the default directional light (always present). */
    [[nodiscard]] std::shared_ptr<vne::scene::DirectionalLight> getDirectionalLight() const { return dir_light_; }

    /**
     * @brief Add a point light to the scene state.
     * Returns the typed pointer so callers can later modify or remove it.
     */
    std::shared_ptr<vne::scene::PointLight> addPointLight(std::shared_ptr<vne::scene::PointLight> light) {
        if (light) {
            scene_state_.addLight(light);
        }
        return light;
    }

    // ---- Model matrix ----
    void setModelMatrix(const vne::math::Mat4f& m) { model_ = m; }
    [[nodiscard]] const vne::math::Mat4f& getModelMatrix() const { return model_; }

    /**
     * @brief Apply a uniform scale centered on the mesh's load-time center offset.
     *
     * Builds model = Translation(center_offset) * Scale(s) so the mesh stays on
     * the grid origin regardless of scale value. Resets to scale=1 when s==1.
     */
    void setUniformScale(float s);

    /** @brief Returns the center offset applied when the last mesh was loaded. */
    [[nodiscard]] const float* getCenterOffset() const { return center_offset_; }

    // ---- AABB of last loaded mesh (model space) ----
    [[nodiscard]] const float* getAabbMin() const { return aabb_min_; }
    [[nodiscard]] const float* getAabbMax() const { return aabb_max_; }

    /** @brief Current uniform scale (1.0 after load or reset). */
    [[nodiscard]] float getUniformScale() const { return uniform_scale_; }

    void onAttach(AppContext& ctx) override;
    void onDetach() override;
    void onRender(const RenderContext& ctx) override;

   private:
    void loadMeshFromPath();

    /**
     * @brief Convert scene_state_ lights to PhongLightParams for drawMesh.
     * Iterates ILight objects by type: first AmbientLight → ambient, first
     * DirectionalLight → dir light, up to 4 PointLights → point_lights[].
     */
    [[nodiscard]] PhongLightParams buildLightParams() const;

    std::string mesh_path_;
    CameraProvider camera_provider_;

    IRenderDevice* device_{nullptr};
    MeshRenderer* mesh_renderer_{nullptr};
    BufferHandle vbo_{};
    BufferHandle ibo_{};

    uint32_t index_count_{0};
    bool ready_{false};

    // vnescene lights
    vne::scene::SceneState scene_state_;
    std::shared_ptr<vne::scene::AmbientLight> ambient_light_;
    std::shared_ptr<vne::scene::DirectionalLight> dir_light_;

    vne::math::Mat4f model_{vne::math::Mat4f::identity()};

    float aabb_min_[3]{0.0f, 0.0f, 0.0f};
    float aabb_max_[3]{0.0f, 0.0f, 0.0f};
    float center_offset_[3]{0.0f, 0.0f, 0.0f};  ///< Translation set by loadMeshFromPath to grid origin
    float uniform_scale_{1.0f};                 ///< Current scale applied via setUniformScale()
};

}  // namespace testbed
}  // namespace vne
