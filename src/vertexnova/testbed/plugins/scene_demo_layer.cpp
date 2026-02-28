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

#include "vertexnova/testbed/plugins/scene_demo_layer.h"

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/render_context.h"

namespace vne {
namespace testbed {

SceneDemoLayer::SceneDemoLayer()
    : ILayer("SceneDemoLayer") {}

void SceneDemoLayer::onAttach(AppContext& ctx) {
    // Create a perspective camera (60° FOV, assume 16:9 initially).
    camera_ = std::make_shared<vne::scene::PerspectiveCamera>(60.0f, 1280.0f, 720.0f, 0.1f, 1000.0f, "DemoCamera");
    camera_->setPosition({3.0f, 2.0f, 5.0f});
    camera_->setTarget({0.0f, 0.0f, 0.0f});
    camera_->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
    camera_->updateMatrices();

    scene_state_.setActiveCamera(camera_);

    // Grab the debug draw from AppContext (set by the runner).
    debug_draw_ = dynamic_cast<gl::OpenGLDebugDraw*>(ctx.debugDraw);
}

void SceneDemoLayer::onDetach() {
    debug_draw_ = nullptr;
    scene_state_.setActiveCamera(nullptr);
    camera_.reset();
}

void SceneDemoLayer::onUpdate(float /*dt*/) {
    if (camera_) {
        camera_->updateMatrices();
    }
}

void SceneDemoLayer::onRender(const RenderContext& ctx) {
    if (!camera_ || !debug_draw_) {
        return;
    }

    // Update camera aspect ratio if the window was resized.
    if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
        const float aspect = static_cast<float>(ctx.frame_info.width) / static_cast<float>(ctx.frame_info.height);
        camera_->setAspectRatio(aspect);
        camera_->updateProjectionMatrix();
    }

    // Provide the VP matrix to the debug draw so it can transform world-space lines.
    debug_draw_->setViewProjectionMatrix(camera_->getViewProjectionMatrix());

    drawGrid();
    drawAxes();

    debug_draw_->flush();
}

// ---------------------------------------------------------------------------
// Private helpers
// ---------------------------------------------------------------------------

void SceneDemoLayer::drawGrid() {
    const vne::math::Vec3f grid_color{0.35f, 0.35f, 0.35f};

    for (int i = -kGridLines; i <= kGridLines; ++i) {
        const float t = static_cast<float>(i) * kGridSpacing;
        // Lines parallel to Z axis
        debug_draw_->line({t, 0.0f, -kGridHalfWidth}, {t, 0.0f, kGridHalfWidth}, grid_color);
        // Lines parallel to X axis
        debug_draw_->line({-kGridHalfWidth, 0.0f, t}, {kGridHalfWidth, 0.0f, t}, grid_color);
    }
}

void SceneDemoLayer::drawAxes() {
    const float len = kGridHalfWidth;
    // X axis — red
    debug_draw_->line({0.0f, 0.0f, 0.0f}, {len, 0.0f, 0.0f}, {1.0f, 0.15f, 0.15f});
    // Y axis — green
    debug_draw_->line({0.0f, 0.0f, 0.0f}, {0.0f, len, 0.0f}, {0.15f, 1.0f, 0.15f});
    // Z axis — blue
    debug_draw_->line({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, len}, {0.15f, 0.15f, 1.0f});
}

}  // namespace testbed
}  // namespace vne
