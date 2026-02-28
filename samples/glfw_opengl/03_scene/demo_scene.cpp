/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Sample 03_scene: perspective camera + debug grid via IDebugDraw.
 * Tests: vnescene PerspectiveCamera, SceneState, IDebugDraw.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/debug_draw.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include "vertexnova/scene/camera/perspective_camera.h"
#include "vertexnova/scene/scene_state.h"

#include <memory>

namespace {

// ---------------------------------------------------------------------------
// SceneDemoLayer
// ---------------------------------------------------------------------------

class SceneDemoLayer : public vne::testbed::ILayer {
   public:
    SceneDemoLayer()
        : vne::testbed::ILayer("SceneDemoLayer") {}

    void onAttach(vne::testbed::AppContext& ctx) override {
        camera_ = std::make_shared<vne::scene::PerspectiveCamera>(60.0f, 1280.0f, 720.0f, 0.1f, 1000.0f, "DemoCamera");
        camera_->setPosition({3.0f, 2.0f, 5.0f});
        camera_->setTarget({0.0f, 0.0f, 0.0f});
        camera_->setGraphicsApi(vne::math::GraphicsApi::eOpenGL);
        camera_->updateMatrices();

        scene_state_.setActiveCamera(camera_);
        debug_draw_ = ctx.debugDraw;
    }

    void onDetach() override {
        debug_draw_ = nullptr;
        scene_state_.setActiveCamera(nullptr);
        camera_.reset();
    }

    void onUpdate(float /*dt*/) override {
        if (camera_) {
            camera_->updateMatrices();
        }
    }

    void onRender(const vne::testbed::RenderContext& ctx) override {
        if (!camera_ || !debug_draw_) {
            return;
        }
        if (ctx.frame_info.width > 0 && ctx.frame_info.height > 0) {
            const float aspect = static_cast<float>(ctx.frame_info.width) / static_cast<float>(ctx.frame_info.height);
            camera_->setAspectRatio(aspect);
            camera_->updateProjectionMatrix();
        }
        debug_draw_->setViewProjectionMatrix(camera_->getViewProjectionMatrix());
        drawGrid();
        drawAxes();
        debug_draw_->flush();
    }

    [[nodiscard]] std::shared_ptr<vne::scene::PerspectiveCamera> getCamera() const { return camera_; }

   private:
    static constexpr int kGridLines = 20;
    static constexpr float kGridSpacing = 1.0f;
    static constexpr float kGridHalfWidth = kGridLines * kGridSpacing * 0.5f;

    void drawGrid() {
        const vne::math::Vec3f grid_color{0.35f, 0.35f, 0.35f};
        for (int i = -kGridLines; i <= kGridLines; ++i) {
            const float t = static_cast<float>(i) * kGridSpacing;
            debug_draw_->line({t, 0.0f, -kGridHalfWidth}, {t, 0.0f, kGridHalfWidth}, grid_color);
            debug_draw_->line({-kGridHalfWidth, 0.0f, t}, {kGridHalfWidth, 0.0f, t}, grid_color);
        }
    }

    void drawAxes() {
        const float len = kGridHalfWidth;
        debug_draw_->line({0.0f, 0.0f, 0.0f}, {len, 0.0f, 0.0f}, {1.0f, 0.15f, 0.15f});
        debug_draw_->line({0.0f, 0.0f, 0.0f}, {0.0f, len, 0.0f}, {0.15f, 1.0f, 0.15f});
        debug_draw_->line({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, len}, {0.15f, 0.15f, 1.0f});
    }

    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
    vne::scene::SceneState scene_state_;
    vne::testbed::IDebugDraw* debug_draw_{nullptr};
};

// ---------------------------------------------------------------------------

void RegisterSceneDemo(vne::testbed::Application& app) {
    app.getLayerStack().pushLayer(std::make_unique<SceneDemoLayer>(), app.getAppContext());
}

}  // namespace

VNETESTBED_REGISTER_DEMO("scene", RegisterSceneDemo)
