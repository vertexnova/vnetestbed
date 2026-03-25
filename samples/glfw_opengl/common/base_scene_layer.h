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

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/debug_draw.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include "vertexnova/scene/camera/camera.h"
#include "vertexnova/scene/camera/orthographic_camera.h"
#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/scene_state.h"

#ifdef VNE_TESTBED_INTERACTION
#include "vertexnova/events/event.h"
#include "vertexnova/events/event_listener.h"
#include "vertexnova/interaction/inspect_controller.h"
#endif

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#endif

#include <memory>
#include <vector>

// ---------------------------------------------------------------------------
// BaseSceneLayer — grid + axes + perspective or orthographic camera
// Exposes getActiveCamera(i), getActiveCameraPtr(i), and getActiveCameras() for interaction layers.
// Supports per-viewport cameras for 2 or 4 viewport layouts.
//
// UiSettings: sample/ImGui tunables (stable addresses for ImGui::SliderFloat etc.).
// ---------------------------------------------------------------------------
class BaseSceneLayer : public vne::testbed::ILayer {
   public:
    /**
     * Mutable camera/grid UI state for samples (ImGui binds to these fields).
     */
    struct UiSettings {
        bool show_grid{true};
        bool show_axes{true};
        bool use_perspective{true};
        float fov{60.0f};
        float near_plane{0.1f};
        float far_plane{1000.0f};
        float ortho_half{6.0f};
        float ortho_near{-100.0f};
        float ortho_far{100.0f};
        int last_viewport_w{1280};
        int last_viewport_h{720};
        float cam_position[3]{4.f, 3.f, 6.f};
        float cam_target[3]{0.f, 0.f, 0.f};
        float cam_up[3]{0.f, 1.f, 0.f};
    };

    explicit BaseSceneLayer(const char* name = "BaseSceneLayer");

    void onAttach(vne::testbed::AppContext& app_context) override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onRender(const vne::testbed::RenderContext& render_context) override;

    /**
     * @brief ownership shared ownership; perspective or ortho per ui settings.
     *
     */
    [[nodiscard]] std::shared_ptr<vne::scene::ICamera> getActiveCamera(int index) const;
    /**
     * Non-owning pointer; same camera as getActiveCamera(index).get().
     */
    [[nodiscard]] vne::scene::ICamera* getActiveCameraPtr(int index) const;
    [[nodiscard]] std::vector<std::shared_ptr<vne::scene::ICamera>> getActiveCameras() const;

    [[nodiscard]] UiSettings& uiSettings() noexcept { return ui_settings_; }
    [[nodiscard]] const UiSettings& uiSettings() const noexcept { return ui_settings_; }

    void setUsePerspective(bool use_persp);
    void syncCameraPositionTargetUp();
    void rebuildCameras(int w, int h);

   private:
    void buildCameras(int w, int h);
    void drawGrid() const;
    void drawAxes() const;

    UiSettings ui_settings_{};
    std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>> camera_persp_;
    std::vector<std::shared_ptr<vne::scene::OrthographicCamera>> cameras_ortho_;
    vne::scene::SceneState scene_state_;
    vne::testbed::IDebugDraw* debug_draw_{nullptr};
};

// ---------------------------------------------------------------------------
// BaseInteractionLayer — orbit-arcball driven by EventManager
// Pair with BaseSceneLayer: call setCamera(scene->getActiveCamera()) before attach.
// Only compiled when VNE_TESTBED_INTERACTION is defined.
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_INTERACTION

class BaseInteractionLayer : public vne::testbed::ILayer, public vne::events::EventListener {
   public:
    explicit BaseInteractionLayer(const char* name = "BaseInteractionLayer");

    void setCamera(std::shared_ptr<vne::scene::ICamera> camera);
    void setSceneLayer(const BaseSceneLayer* scene);
    void setCameras(const std::vector<std::shared_ptr<vne::scene::PerspectiveCamera>>& cameras);
    void setCameras(const std::vector<std::shared_ptr<vne::scene::ICamera>>& cameras);

#ifdef VNE_TESTBED_IMGUI
    void setImGuiLayer(vne::testbed::ImGuiLayer* layer);
#endif

    void setInteractionEnabled(bool enabled);
    [[nodiscard]] bool getInteractionEnabled() const;

    void onAttach(vne::testbed::AppContext& app_context) override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onEvent(const vne::events::Event& event) override;

    [[nodiscard]] vne::interaction::InspectController* getInspectController();
    [[nodiscard]] vne::interaction::InspectController* getInspectController(int index);

   private:
    std::vector<vne::interaction::InspectController> controllers_;
    bool interaction_enabled_{true};
    double last_x_{0.0};
    double last_y_{0.0};
    bool first_mouse_{true};
#ifdef VNE_TESTBED_IMGUI
    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
#endif
};

#endif  // VNE_TESTBED_INTERACTION
