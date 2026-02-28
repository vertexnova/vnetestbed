#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   February 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

/**
 * @file plugins/scene_demo_layer.h
 * @brief Demo layer integrating vnescene: perspective camera + world-space grid.
 *
 * Owns a PerspectiveCamera and a SceneState.  Each frame it:
 *   - Updates the camera aspect ratio on resize.
 *   - Uploads the view-projection matrix to a shared debug draw instance.
 *   - Draws a world-space XZ grid and axis cross via IDebugDraw.
 *
 * Other layers that need the camera (e.g. InteractionDemoLayer) can call
 * getCamera() after onAttach().
 *
 * Only compiled when VNE_TESTBED_OPENGL is defined (depends on gl/ utilities).
 */

#ifdef VNE_TESTBED_OPENGL

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/gl/opengl_debug_draw.h"

#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/scene_state.h"

#include <memory>

namespace vne {
namespace testbed {

/**
 * @class SceneDemoLayer
 * @brief ILayer that owns a perspective camera and renders a debug grid.
 *
 * Push this layer before InteractionDemoLayer so the camera exists when
 * the interaction layer's onAttach() runs.
 */
class SceneDemoLayer : public ILayer {
   public:
    SceneDemoLayer();

    void onAttach(AppContext& ctx) override;
    void onDetach() override;
    void onUpdate(float dt) override;
    void onRender(const RenderContext& ctx) override;

    /**
     * @brief Returns the layer's perspective camera (valid after onAttach()).
     *
     * InteractionDemoLayer uses this to attach the camera manipulator.
     */
    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> getCamera() const { return camera_; }

   private:
    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
    vne::scene::SceneState scene_state_;
    gl::OpenGLDebugDraw* debug_draw_{nullptr};

    // Grid configuration
    static constexpr int kGridLines = 20;
    static constexpr float kGridSpacing = 1.0f;
    static constexpr float kGridHalfWidth = kGridLines * kGridSpacing * 0.5f;

    void drawGrid();
    void drawAxes();
};

}  // namespace testbed
}  // namespace vne

#endif  // VNE_TESTBED_OPENGL
