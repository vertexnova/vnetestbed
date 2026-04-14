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

#ifdef VNE_TESTBED_INTERACTION

#include "vertexnova/testbed/layer.h"

#include "vertexnova/interaction/inspect_3d_controller.h"
#include "vertexnova/interaction/navigation_3d_controller.h"
#include "vertexnova/interaction/ortho_2d_controller.h"
#include "vertexnova/interaction/follow_controller.h"
#include "vertexnova/interaction/interaction_types.h"

#include "vertexnova/scene/camera/camera.h"

#include <vertexnova/math/core/core.h>

#include <array>
#include <filesystem>
#include <memory>
#include <optional>
#include <string>
#include <variant>
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

namespace vne::samples {

/** Controller type for the interaction demo; maps to one of the high-level controllers. */
enum class ControllerKind : int {
    eInspectOrbit = 0,
    eInspectTrackball,
    eNavigation,  // FPS / Fly via setNavigationMode (FreeLookMode on Navigation3DController)
    eOrtho2D,
    eFollow,
};

using ControllerVariant = std::variant<vne::interaction::Inspect3DController,
                                       vne::interaction::Navigation3DController,
                                       vne::interaction::Ortho2DController,
                                       vne::interaction::FollowController>;

// ---------------------------------------------------------------------------
// InteractionTestLayer — owns camera + per-viewport controllers, exposes control API
// ---------------------------------------------------------------------------
class InteractionTestLayer : public vne::testbed::ILayer {
   public:
    /** Needed in the header for `std::array<ControllerVariant, kMaxViewports>`. */
    static constexpr int kMaxViewports = 4;

    InteractionTestLayer();

    void onAttach(vne::testbed::AppContext& app_context) override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onEvent(const vne::events::Event& event) override;

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera);
    void setSceneLayer(BaseSceneLayer* scene);

#ifdef VNE_TESTBED_IMGUI
    void setImGuiLayer(vne::testbed::ImGuiLayer* layer);
#endif

    void setControllerKind(ControllerKind kind);
    [[nodiscard]] ControllerKind getControllerKind() const noexcept { return current_kind_; }

    /** @brief Check if current controller supports the given camera type. Ortho 2D requires orthographic. */
    [[nodiscard]] bool isManipulatorCompatibleWithCamera(bool use_perspective) const;

    /** @brief Set cameras from scene (call after scene camera type or params change). */
    void setCamerasFromScene();

    void setZoomMethod(vne::interaction::ZoomMethod method);
    void setViewDirection(vne::interaction::ViewDirection dir);
    void resetCamera();
    void setMoveSpeed(float speed);
    void setMouseSensitivity(float sensitivity);
    void setSprintMultiplier(float mult);
    void setSlowMultiplier(float mult);

    void setRotationPivotMode(vne::interaction::OrbitPivotMode mode);
    void setRotationEnabled(bool enabled);
    void setPanEnabled(bool enabled);
    void setZoomEnabled(bool enabled);

#ifdef VNE_TESTBED_IMGUI
    void setMeshLayer(vne::testbed::MeshLayer* layer) noexcept;
#endif
    void setNavigationMode(vne::interaction::FreeLookMode mode);

    [[nodiscard]] vne::math::Vec3f cameraPosition() const;
    [[nodiscard]] vne::math::Vec3f cameraTarget() const;

    [[nodiscard]] vne::interaction::Inspect3DController* getInspectController(int index = 0) noexcept;
    [[nodiscard]] vne::interaction::Navigation3DController* getNavController(int index = 0) noexcept;
    [[nodiscard]] vne::interaction::Ortho2DController* getOrtho2DController(int index = 0) noexcept;
    [[nodiscard]] vne::interaction::FollowController* getFollowController(int index = 0) noexcept;

   private:
    void dispatchViewportSize(float w, float h);
    void dispatchEvent(const vne::events::Event& event, int viewport_index);
    void dispatchUpdate(double dt);
    void dispatchSetCamera(std::shared_ptr<vne::scene::ICamera> camera);
    void dispatchReset();

    BaseSceneLayer* scene_layer_{nullptr};
    std::shared_ptr<vne::scene::ICamera> camera_;
    std::array<ControllerVariant, kMaxViewports> controllers_;
    ControllerKind current_kind_{ControllerKind::eInspectTrackball};
#ifdef VNE_TESTBED_IMGUI
    vne::testbed::MeshLayer* mesh_layer_{nullptr};
#endif
    vne::interaction::FreeLookMode navigation_mode_{vne::interaction::FreeLookMode::eFps};
    double last_x_{0.0};
    double last_y_{0.0};
#ifdef VNE_TESTBED_IMGUI
    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
#endif
};

// ---------------------------------------------------------------------------
// InteractionSettingsLayer — ImGui panel: controller kind, zoom, view, speeds + mesh browser
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI
// ImGui-tuned state (stable addresses for ImGui widgets).
struct InteractionUiSettings {
    bool show_view_matrix{false};
    bool show_projection_matrix{false};
    int zoom_idx{0};
    int nav_mode_idx{0};
    float move_speed{3.0f};
    float mouse_sensitivity{0.15f};
    float sprint_mult{4.0f};
    float slow_mult{0.2f};
    bool rotation_enabled_insp{true};
    int rotation_mode_insp_idx{0};  // 0=Orbit 1=Trackball (Inspect3D)
    bool pan_enabled_insp{true};
    bool zoom_enabled_insp{true};
};

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

    void onAttach(vne::testbed::AppContext& app_context) override;
    void onDetach() override;

    [[nodiscard]] InteractionUiSettings& uiSettings() noexcept { return ui_; }
    [[nodiscard]] const InteractionUiSettings& uiSettings() const noexcept { return ui_; }

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

    InteractionUiSettings ui_{};

    /** Last controller kind used for manipulator UI sync; reset on detach / new layer. */
    std::optional<ControllerKind> last_manip_synced_controller_kind_;
};
#endif

void registerTestInteractionDemo(vne::testbed::Application& app);

}  // namespace vne::samples

#endif  // VNE_TESTBED_INTERACTION
