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
#include <vertexnova/math/color.h>
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

namespace vne::samples {

struct PointLightEntry {
    std::shared_ptr<vne::scene::PointLight> light;
    float orbit_radius{3.0f};                  //!< Horizontal orbit radius in world units (XZ plane).
    float orbit_speed{1.0f};                   //!< Radians per second; sign = direction.
    float orbit_angle{0.0f};                   //!< Initial phase on the orbit.
    vne::math::Color color{1.0f, 0.8f, 0.4f};  //!< RGB tint for debug crosshair and light.
    float intensity{2.0f};                     //!< Point light intensity multiplier.
    float range{8.0f};                         //!< Max influence distance for falloff.
    bool enabled{true};                        //!< On/off without removing from the scene.
};

// Camera / cubes / lights state edited via ImGui (private on SceneTestLayer; access via uiSettings()).
struct SceneUiSettings {
    bool use_perspective{true};                    //!< If false, orthographic cameras are used instead.
    float fov{60.0f};                              //!< Vertical field of view (degrees) for perspective.
    float near_plane{0.1f};                        //!< Perspective near clip distance.
    float far_plane{1000.0f};                      //!< Perspective far clip distance.
    float ortho_half{6.0f};                        //!< Ortho half-extent on Y; X width follows aspect.
    float ortho_near{-100.0f};                     //!< Ortho near plane (can be negative in this sample).
    float ortho_far{100.0f};                       //!< Ortho far plane.
    vne::math::Vec3f cam_position{4.f, 3.f, 6.f};  //!< Default eye position (world).
    vne::math::Vec3f cam_target{0.f, 0.f, 0.f};    //!< Look-at point (world).
    vne::math::Vec3f cam_up{0.f, 1.f, 0.f};        //!< World up for view basis.
    bool show_view_matrix{false};                  //!< ImGui: print 4x4 view M in settings.
    bool show_projection_matrix{false};            //!< ImGui: print 4x4 projection M in settings.
    bool show_camera_visuals{true};                //!< Debug draw: frustum / axes / near-plane helpers.
    bool show_frustum{true};                       //!< Wireframe frustum when show_camera_visuals.
    bool show_cam_axes{true};                      //!< Forward/right/up at camera position.
    bool show_near_far_planes{true};               //!< Extra diagonals on near/far rects.
    int last_vp_w{1280};                           //!< Last known framebuffer width (px); drives aspect.
    int last_vp_h{720};                            //!< Last known framebuffer height (px).

    int cube_count{3};                //!< Number of cubes drawn (0–4); positions are still defined for all four.
    float cube_rotation_speed{0.5f};  //!< Radians per second; shared by all cubes.
    vne::math::Vec3f cube_position[4]{
        {0.f, 0.5f, 0.f},
        {2.5f, 0.5f, 0.f},
        {-2.5f, 0.5f, 0.f},
        {0.f, 0.5f, 2.5f},
    };  //!< Default cube centers (world); y = 0.5 on the grid.

    bool ambient_light_enabled{true};                          //!< Master switch for ambient term.
    vne::math::Color ambient_light_color{0.08f, 0.08f, 0.1f};  //!< Dim ambient RGB.
    float ambient_light_intensity{1.0f};                       //!< Scales ambient contribution.

    bool dir_light_enabled{true};                         //!< Master switch for directional (sun) light.
    vne::math::Vec3f dir_light_dir{-0.5f, -1.0f, -0.3f};  //!< Directional light (toward scene); need not be unit.
    vne::math::Color dir_light_color{1.0f, 0.97f, 0.9f};  //!< Warm sunlight RGB.
    float dir_light_intensity{1.0f};

    bool spot_light_enabled{false};                      //!< Master switch for scene spot light.
    vne::math::Vec3f spot_light_pos{0.f, 4.f, 4.f};      //!< Spot origin (world).
    vne::math::Vec3f spot_light_dir{0.f, -0.7f, -0.7f};  //!< Spot aim direction; normalized in sync.
    vne::math::Color spot_light_color{1.f, 0.9f, 0.7f};  //!< Spot RGB (warm).
    float spot_light_intensity{2.f};                     //!< Spot intensity multiplier.
    float spot_light_range{10.f};                        //!< Spot influence distance.
    float spot_light_inner_deg{15.f};                    //!< Inner cone half-angle (degrees).
    float spot_light_outer_deg{30.f};                    //!< Outer cone half-angle (degrees).

    bool use_attn_formula{false};  //!< If true, use k0+k1*d+k2*d^2 for point/spot falloff.
    float attn_const{1.f};         //!< Attenuation constant term (when formula enabled).
    float attn_linear{0.09f};      //!< Linear coefficient k1 in (k0 + k1*d + k2*d^2).
    float attn_quad{0.032f};       //!< Quadratic coefficient k2.

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
    void syncAmbientLight();
    void syncDirLight();
    void syncSpotLight();
    void addPointLight();
    void removeLastPointLight();
    void resetToDefault();
    void syncPointLight(std::size_t i);

    void rebuildCamera(int w, int h);
    void syncCameraPositionTargetUp();

    [[nodiscard]] vne::math::Mat4f getCubeModelMatrix(int i) const;
    [[nodiscard]] vne::scene::ICamera* activeCameraPtr(int vp_idx) const;
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

}  // namespace vne::samples
