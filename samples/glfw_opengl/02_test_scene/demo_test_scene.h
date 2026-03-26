#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 *
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"
#include "vertexnova/testbed/render_device.h"
#include "vertexnova/testbed/renderer/phong_material.h"

#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/light/ambient_light.h"
#include "vertexnova/scene/light/directional_light.h"
#include "vertexnova/scene/light/point_light.h"
#include "vertexnova/scene/light/spot_light.h"
#include "vertexnova/scene/scene_state.h"
#include <vertexnova/math/core/core.h>

#include <memory>
#include <vector>

namespace vne::scene {
class ICamera;
}
namespace vne::testbed {
class Application;
class MeshRenderer;
}  // namespace vne::testbed

#ifdef VNE_TESTBED_IMGUI
namespace vne::testbed {
class ImGuiLayer;
}
class BaseInteractionLayer;  // from samples/glfw_opengl/common/base_scene_layer.h (global scope)
#endif

namespace vne::samples::test_scene {

struct PointLightEntry {
    std::shared_ptr<vne::scene::PointLight> light;
    float orbit_radius{3.0f};
    float orbit_speed{1.0f};
    float orbit_angle{0.0f};
    float color[3]{1.0f, 0.8f, 0.4f};
    float intensity{2.0f};
    float range{8.0f};
    bool enabled{true};
};

// Camera / cubes / lights state edited via ImGui (private on SceneTestLayer; access via uiSettings()).
struct SceneUiSettings {
    bool use_perspective{true};
    float fov{60.0f};
    float near_plane{0.1f};
    float far_plane{1000.0f};
    float ortho_half{6.0f};
    float ortho_near{-100.0f};
    float ortho_far{100.0f};
    float cam_position[3]{4.f, 3.f, 6.f};
    float cam_target[3]{0.f, 0.f, 0.f};
    float cam_up[3]{0.f, 1.f, 0.f};
    bool show_view_matrix{false};
    bool show_projection_matrix{false};
    bool show_camera_visuals{true};
    bool show_frustum{true};
    bool show_cam_axes{true};
    bool show_near_far_planes{true};
    int last_vp_w{1280};
    int last_vp_h{720};

    int cube_count{3};
    float cube_rotation_speed{0.5f};
    float cube_position[4][3] = {
        {0.f, 0.5f, 0.f},
        {2.5f, 0.5f, 0.f},
        {-2.5f, 0.5f, 0.f},
        {0.f, 0.5f, 2.5f},
    };

    bool ambient_light_enabled{true};
    float ambient_light_color[3]{0.08f, 0.08f, 0.1f};
    float ambient_light_intensity{1.0f};

    bool dir_light_enabled{true};
    float dir_light_dir[3]{-0.5f, -1.0f, -0.3f};
    float dir_light_color[3]{1.0f, 0.97f, 0.9f};
    float dir_light_intensity{1.0f};

    bool spot_light_enabled{false};
    float spot_light_pos[3]{0.f, 4.f, 4.f};
    float spot_light_dir[3]{0.f, -0.7f, -0.7f};
    float spot_light_color[3]{1.f, 0.9f, 0.7f};
    float spot_light_intensity{2.f};
    float spot_light_range{10.f};
    float spot_light_inner_deg{15.f};
    float spot_light_outer_deg{30.f};

    bool use_attn_formula{false};
    float attn_const{1.f};
    float attn_linear{0.09f};
    float attn_quad{0.032f};

    std::vector<PointLightEntry> point_lights;
};

// ---------------------------------------------------------------------------
// SceneTestLayer — camera management + cube rendering + lights
// ---------------------------------------------------------------------------
class SceneTestLayer : public vne::testbed::ILayer {
   public:
    // 1. Types and constants
    static constexpr int kMaxViewports = 4;

    // 2. Constructors and destructor
    SceneTestLayer();

    // 3. Public methods (ILayer overrides)
    void onAttach(vne::testbed::AppContext& app_context) override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onRender(const vne::testbed::RenderContext& render_context) override;

    // 4. UI / settings bundle (mutable for ImGui bindings)
    [[nodiscard]] SceneUiSettings& uiSettings() { return ui_; }
    [[nodiscard]] const SceneUiSettings& uiSettings() const { return ui_; }

    // 5. Public methods (camera, cubes, lights, reset)
    void rebuildCamera(int w, int h);
    void syncCameraPositionTargetUp();
    [[nodiscard]] vne::math::Mat4f getCubeModelMatrix(int i) const;
    [[nodiscard]] vne::scene::ICamera* activeCamera(int vp_idx) const;
    void syncAmbientLight();
    void syncDirLight();
    void syncSpotLight();
    void addPointLight();
    void removeLastPointLight();
    void resetToDefault();
    void syncPointLight(std::size_t i);
    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> cameraPersp() const;
    [[nodiscard]] const std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>>& getCameras() const;
    [[nodiscard]] std::vector<std::shared_ptr<vne::scene::ICamera>> getActiveCameras() const;

   private:
    // Private constants
    static constexpr int kGridLines = 20;
    static constexpr float kGridSpacing = 1.0f;
    static constexpr float kGridHalf = kGridLines * kGridSpacing * 0.5f;
    static constexpr float kSpotAngleEpsDeg = 0.5f;

    // Private methods
    void buildCamera(int w, int h);
    void buildGeometry();
    void buildLights();
    void updateCameraAspect(int w, int h);
    [[nodiscard]] vne::math::Mat4f getActiveViewProjectionMatrix(int vp_idx) const;
    [[nodiscard]] vne::math::Vec3f getActiveCameraPosition(int vp_idx) const;
    void drawCameraVisuals(int vp_idx) const;
    void drawFrustum(vne::math::Vec3f pos, vne::math::Vec3f fwd, vne::math::Vec3f right, vne::math::Vec3f up) const;
    void drawGrid() const;
    void drawAxes() const;

    [[nodiscard]] vne::testbed::PhongLightParams buildPhongLightParams();

    // Private members
    SceneUiSettings ui_{};
    vne::testbed::IRenderDevice* device_{nullptr};
    vne::testbed::IDebugDraw* debug_draw_{nullptr};
    vne::testbed::MeshRenderer* mesh_renderer_{nullptr};
    vne::testbed::BufferHandle vbo_{};
    vne::testbed::BufferHandle ibo_{};

    std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>> cameras_persp_;
    std::vector<std::shared_ptr<vne::scene::OrthographicCamera>> cameras_ortho_;
    vne::scene::SceneState scene_state_;

    std::shared_ptr<vne::scene::AmbientLight> ambient_light_;
    std::shared_ptr<vne::scene::DirectionalLight> dir_light_;
    std::shared_ptr<vne::scene::SpotLight> spot_light_;

    float cube_angle_{0.f};
};

// ---------------------------------------------------------------------------
// SceneSettingsLayer — ImGui panel
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI
class SceneSettingsLayer : public vne::testbed::ILayer {
   public:
    SceneSettingsLayer();

    void setImGuiLayer(vne::testbed::ImGuiLayer* layer);
    void setSceneLayer(SceneTestLayer* layer);
#ifdef VNE_TESTBED_INTERACTION
    void setInteractionLayer(BaseInteractionLayer* layer);
#endif

    void onAttach(vne::testbed::AppContext& app_context) override;
    void onDetach() override;

   private:
    void renderPanel();

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
    SceneTestLayer* scene_layer_{nullptr};
#ifdef VNE_TESTBED_INTERACTION
    BaseInteractionLayer* interaction_layer_{nullptr};
#endif
};
#endif

void registerTestSceneDemo(vne::testbed::Application& app);

}  // namespace vne::samples::test_scene
