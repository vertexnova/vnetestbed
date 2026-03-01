/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Sample 00_hello_testbed
 * -----------------------
 * The baseline demo that every subsequent sample is built on top of.
 * Proves the FBO → ImGui viewport pipeline and the Settings panel.
 *
 * What you can test here:
 *   • FBO render target fills the ImGui "Viewport" window correctly
 *   • Viewport resize keeps the image filling the panel (no stretching)
 *   • VSync toggle responds immediately
 *   • Grid and axes give a spatial reference and confirm the camera works
 *   • Orbit-arcball: LMB drag rotates, RMB drag pans, scroll zooms
 *
 * ImGui Settings panel (demo-specific section):
 *   [Viewport]  controls reference, axis legend
 *
 * Libraries exercised: vne::testbed, vne::scene, vne::interaction (optional)
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#include "../common/base_scene_layer.h"

namespace {

// ---------------------------------------------------------------------------
// HelloSettingsLayer — adds the demo-specific section to the Settings panel
// ---------------------------------------------------------------------------
class HelloSettingsLayer : public vne::testbed::ILayer {
   public:
    HelloSettingsLayer()
        : vne::testbed::ILayer("HelloSettingsLayer") {
        setRenderSortKey(999);  // runs just before ImGuiLayer (1000)
    }

    void onAttach(vne::testbed::AppContext& /*ctx*/) override {
#ifdef VNE_TESTBED_IMGUI
        if (imgui_layer_) {
            imgui_layer_->setSettingsCallback([this]() { renderPanel(); });
        }
#endif
    }

    void onDetach() override {
#ifdef VNE_TESTBED_IMGUI
        if (imgui_layer_) {
            imgui_layer_->setSettingsCallback(nullptr);
            imgui_layer_ = nullptr;
        }
#endif
    }

#ifdef VNE_TESTBED_IMGUI
    void setImGuiLayer(vne::testbed::ImGuiLayer* l) { imgui_layer_ = l; }
#endif

   private:
#ifdef VNE_TESTBED_IMGUI
    void renderPanel() {
        if (ImGui::CollapsingHeader("Viewport", ImGuiTreeNodeFlags_DefaultOpen)) {
            ImGui::TextDisabled("Controls");
            ImGui::BulletText("Orbit:  Left-mouse drag");
            ImGui::BulletText("Pan:    Right-mouse drag");
            ImGui::BulletText("Zoom:   Scroll wheel");
            ImGui::Spacing();

            ImGui::TextDisabled("Legend");
            ImGui::TextColored({1.0f, 0.3f, 0.3f, 1.f}, "X");
            ImGui::SameLine();
            ImGui::Text("+X axis (red)");
            ImGui::TextColored({0.3f, 1.0f, 0.3f, 1.f}, "Y");
            ImGui::SameLine();
            ImGui::Text("+Y axis (green)");
            ImGui::TextColored({0.3f, 0.3f, 1.0f, 1.f}, "Z");
            ImGui::SameLine();
            ImGui::Text("+Z axis (blue)");
        }
    }

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
#endif
};

// ---------------------------------------------------------------------------

void RegisterHelloTestbedDemo(vne::testbed::Application& app) {
    // Layer 1: grid + axes + perspective camera
    auto* scene = new BaseSceneLayer("HelloBaseSceneLayer");
    app.getLayerStack().pushLayer(std::unique_ptr<BaseSceneLayer>(scene), app.getAppContext());

#ifdef VNE_TESTBED_INTERACTION
    // Layer 2: orbit-arcball interaction
    auto* interaction = new BaseInteractionLayer("HelloInteractionLayer");
    interaction->setCamera(scene->getCamera());
    app.getLayerStack().pushLayer(std::unique_ptr<BaseInteractionLayer>(interaction), app.getAppContext());
#endif

#ifdef VNE_TESTBED_IMGUI
    // Layer 3: demo-specific Settings panel section
    auto* settings = new HelloSettingsLayer();
    auto* imgui = dynamic_cast<vne::testbed::ImGuiLayer*>(app.getLayerStack().findLayerByName("ImGuiLayer"));
    if (imgui) {
        settings->setImGuiLayer(imgui);
#ifdef VNE_TESTBED_INTERACTION
        interaction->setImGuiLayer(imgui);
#endif
    }
    app.getLayerStack().pushLayer(std::unique_ptr<HelloSettingsLayer>(settings), app.getAppContext());
#endif
}

}  // namespace

VNETESTBED_REGISTER_DEMO("hello_testbed", RegisterHelloTestbedDemo)
