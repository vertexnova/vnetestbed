#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   January 2026
 *
 * Sample 02_test_scene — declarations (SceneTestLayer, SceneSettingsLayer).
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"
#include "vertexnova/testbed/render_device.h"

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
}

#ifdef VNE_TESTBED_IMGUI
namespace vne::testbed {
class ImGuiLayer;
}
#endif

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
    void onAttach(vne::testbed::AppContext& ctx) override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onRender(const vne::testbed::RenderContext& ctx) override;

    // 4. Public methods (camera, cubes, lights, reset)
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

    // 5. Public data (Settings panel / ImGui bindings)
    bool use_perspective_{true};
    float fov_{60.0f};
    float near_{0.1f};
    float far_{1000.0f};
    float ortho_half_{6.0f};
    float ortho_near_{-100.0f};
    float ortho_far_{100.0f};
    float cam_position_[3]{4.f, 3.f, 6.f};
    float cam_target_[3]{0.f, 0.f, 0.f};
    float cam_up_[3]{0.f, 1.f, 0.f};
    bool show_view_matrix_{false};
    bool show_projection_matrix_{false};
    bool show_camera_visuals_{true};
    bool show_frustum_{true};
    bool show_cam_axes_{true};
    bool show_near_far_planes_{true};
    int last_vp_w_{1280};
    int last_vp_h_{720};

    int cube_count_{3};
    float cube_rotation_speed_{0.5f};
    float cube_position_[4][3] = {
        {0.f, 0.5f, 0.f},
        {2.5f, 0.5f, 0.f},
        {-2.5f, 0.5f, 0.f},
        {0.f, 0.5f, 2.5f},
    };

    bool ambient_light_enabled_{true};
    float ambient_light_color_[3]{0.08f, 0.08f, 0.1f};
    float ambient_light_intensity_{1.0f};

    bool dir_light_enabled_{true};
    float dir_light_dir_[3]{-0.5f, -1.0f, -0.3f};
    float dir_light_color_[3]{1.0f, 0.97f, 0.9f};
    float dir_light_intensity_{1.0f};

    bool spot_light_enabled_{false};
    float spot_light_pos_[3]{0.f, 4.f, 4.f};
    float spot_light_dir_[3]{0.f, -0.7f, -0.7f};
    float spot_light_color_[3]{1.f, 0.9f, 0.7f};
    float spot_light_intensity_{2.f};
    float spot_light_range_{10.f};
    float spot_light_inner_deg_{15.f};
    float spot_light_outer_deg_{30.f};

    bool use_attn_formula_{false};
    float attn_const_{1.f};
    float attn_linear_{0.09f};
    float attn_quad_{0.032f};

    std::vector<PointLightEntry> point_lights_;

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

    // Private members
    vne::testbed::IRenderDevice* device_{nullptr};
    vne::testbed::IDebugDraw* debug_draw_{nullptr};
    vne::testbed::ShaderHandle shader_{};
    vne::testbed::BufferHandle vbo_{};
    vne::testbed::BufferHandle ibo_{};
    vne::testbed::PipelineHandle pipeline_{};

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
class BaseInteractionLayer;
class SceneSettingsLayer : public vne::testbed::ILayer {
   public:
    SceneSettingsLayer();

    void setImGuiLayer(vne::testbed::ImGuiLayer* l);
    void setSceneLayer(SceneTestLayer* l);
#ifdef VNE_TESTBED_INTERACTION
    void setInteractionLayer(BaseInteractionLayer* l);
#endif

    void onAttach(vne::testbed::AppContext& ctx) override;
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

void RegisterTestSceneDemo(vne::testbed::Application& app);
