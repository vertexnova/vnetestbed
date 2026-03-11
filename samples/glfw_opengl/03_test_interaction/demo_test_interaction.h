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

#include "vertexnova/scene/camera/perspective_camera.h"

#include <vertexnova/math/core/core.h>

#include <memory>
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
}
#endif

class BaseSceneLayer;

namespace vne::samples::test_interaction {

// ---------------------------------------------------------------------------
// InteractionTestLayer — owns camera + per-viewport controllers, exposes control API
// ---------------------------------------------------------------------------
class InteractionTestLayer : public vne::testbed::ILayer {
   public:
    // 1. Types and constants
    static constexpr int kMaxViewports = 4;
    static constexpr double kFixedDt = 0.016;

    // 2. Constructors and destructor
    InteractionTestLayer();

    // 3. Public methods (ILayer overrides)
    void onAttach(vne::testbed::AppContext& ctx) override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onEvent(const vne::events::Event& event) override;

    // 4. Public methods (camera and manipulator control)
    void setCamera(std::shared_ptr<vne::scene::PerspectiveCamera> cam);
    void setSceneLayer(const BaseSceneLayer* scene);

#ifdef VNE_TESTBED_IMGUI
    void setImGuiLayer(vne::testbed::ImGuiLayer* layer);
#endif

    void setManipulatorType(vne::interaction::CameraManipulatorType type);
    [[nodiscard]] vne::interaction::CameraManipulatorType getManipulatorType() const;

    void setZoomMethod(vne::interaction::ZoomMethod method);
    void setViewDirection(vne::interaction::ViewDirection dir);
    void resetCamera();
    void setFpsSpeed(float speed);
    void setFpsSensitivity(float sensitivity);

    [[nodiscard]] vne::math::Vec3f cameraPosition() const;
    [[nodiscard]] vne::math::Vec3f cameraTarget() const;

   private:
    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
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
// InteractionSettingsLayer — ImGui panel for manipulator type, zoom, view, FPS settings
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI
class InteractionSettingsLayer : public vne::testbed::ILayer {
   public:
    InteractionSettingsLayer();

    void setImGuiLayer(vne::testbed::ImGuiLayer* layer);
    void setInteractionLayer(InteractionTestLayer* layer);

    void onAttach(vne::testbed::AppContext& ctx) override;
    void onDetach() override;

   private:
    void renderPanel();

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
    InteractionTestLayer* interaction_layer_{nullptr};

    int zoom_idx_{0};
    float fps_speed_{3.0f};
    float fps_sensitivity_{0.15f};
};
#endif

void RegisterTestInteractionDemo(vne::testbed::Application& app);

}  // namespace vne::samples::test_interaction

#endif  // VNE_TESTBED_INTERACTION
