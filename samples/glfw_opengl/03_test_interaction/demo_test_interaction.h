#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   January 2026
 *
 * Sample 03_test_interaction — declarations (InteractionTestLayer, InteractionSettingsLayer).
 * ----------------------------------------------------------------------
 */

#ifdef VNE_TESTBED_INTERACTION

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include "vertexnova/interaction/camera_manipulator_factory.h"
#include "vertexnova/interaction/camera_system_controller.h"
#include "vertexnova/interaction/interaction_types.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

namespace vne::testbed {
class Application;
}
namespace vne::events {
class Event;
}

#ifdef VNE_TESTBED_IMGUI
namespace vne::testbed {
class ImGuiLayer;
class MeshLayer;
}  // namespace vne::testbed
#endif

class BaseSceneLayer;

namespace vne::samples::test_interaction {

// ---------------------------------------------------------------------------
// InteractionTestLayer — owns camera + per-viewport controllers, exposes control API
// ---------------------------------------------------------------------------
class InteractionTestLayer : public vne::testbed::ILayer {
   public:
    static constexpr int kMaxViewports = 4;
    static constexpr double kFixedDt = 0.016;

    InteractionTestLayer();

    void onAttach(vne::testbed::AppContext& ctx) override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onEvent(const vne::events::Event& event) override;

    void setCamera(std::shared_ptr<vne::scene::ICamera> cam);
    void setSceneLayer(BaseSceneLayer* scene);

#ifdef VNE_TESTBED_IMGUI
    void setImGuiLayer(vne::testbed::ImGuiLayer* layer);
#endif

    void setManipulatorType(vne::interaction::CameraManipulatorType type);
    [[nodiscard]] vne::interaction::CameraManipulatorType getManipulatorType() const;

    /** @brief Check if manipulator supports current camera type. OrthoPanZoom requires orthographic. */
    [[nodiscard]] bool isManipulatorCompatibleWithCamera(bool use_perspective) const;

    /** @brief Set cameras from scene (call after scene camera type or params change). */
    void setCamerasFromScene();

    void setZoomMethod(vne::interaction::ZoomMethod method);
    void setViewDirection(vne::interaction::ViewDirection dir);
    void resetCamera();
    void setFpsSpeed(float speed);
    void setFpsSensitivity(float sensitivity);

    [[nodiscard]] vne::math::Vec3f cameraPosition() const;
    [[nodiscard]] vne::math::Vec3f cameraTarget() const;

    [[nodiscard]] vne::interaction::CameraSystemController* getController() const;
    [[nodiscard]] vne::interaction::CameraSystemController* getController(int index) const;

   private:
    BaseSceneLayer* scene_layer_{nullptr};
    std::shared_ptr<vne::scene::ICamera> camera_;
    std::vector<std::unique_ptr<vne::interaction::CameraSystemController>> controllers_;
    vne::interaction::CameraManipulatorFactory factory_;
    vne::interaction::CameraManipulatorType current_manipulator_type_{vne::interaction::CameraManipulatorType::eOrbit};
    double last_x_{0.0};
    double last_y_{0.0};
    bool first_mouse_{true};
#ifdef VNE_TESTBED_IMGUI
    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
#endif
};

// ---------------------------------------------------------------------------
// InteractionSettingsLayer — ImGui panel: manipulator, zoom, view, FPS + mesh browser
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI
class InteractionSettingsLayer : public vne::testbed::ILayer {
   public:
    InteractionSettingsLayer();

    void setImGuiLayer(vne::testbed::ImGuiLayer* layer);
    void setInteractionLayer(InteractionTestLayer* layer);
    void setSceneLayer(BaseSceneLayer* layer);

    /**
     * @brief Set the MeshLayer to drive from the mesh browser.
     *
     * When set, the mesh browser panel lists files from the meshes directory and
     * reloads the MeshLayer when the user clicks or drops a mesh file.
     */
    void setMeshLayer(vne::testbed::MeshLayer* layer);

    /**
     * @brief Set the root directory to browse for meshes.
     *
     * Typically VNETESTBED_TESTDATA_DIR + "/resources/meshes".
     * The browser lists .ply, .obj, .stl, and .fbx files in this directory.
     */
    void setMeshesDir(std::string dir);

    void onAttach(vne::testbed::AppContext& ctx) override;
    void onDetach() override;

   private:
    void renderPanel();
    void renderCameraSettings();
    void renderManipulatorSettings();
    void renderMeshBrowser();
    void renderLightingSettings();
    void renderMeshTransform();
    void loadMesh(const std::filesystem::path& path);
    void handleViewportDrop(int viewport_index);

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
    InteractionTestLayer* interaction_layer_{nullptr};
    BaseSceneLayer* scene_layer_{nullptr};
    vne::testbed::MeshLayer* mesh_layer_{nullptr};

    std::string meshes_dir_;
    std::vector<std::filesystem::path> mesh_files_;
    int selected_mesh_idx_{-1};

    bool show_view_matrix_{false};
    bool show_projection_matrix_{false};

    int zoom_idx_{0};
    float fps_speed_{3.0f};
    float fps_sensitivity_{0.15f};
};
#endif

void RegisterTestInteractionDemo(vne::testbed::Application& app);

}  // namespace vne::samples::test_interaction

#endif  // VNE_TESTBED_INTERACTION
