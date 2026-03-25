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

#include "demo_hello_testbed.h"

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#include "../common/base_scene_layer.h"

namespace {

constexpr int kRenderSortKey = 999;  //!< runs just before ImGuiLayer (1000)

}  // namespace

namespace vne::samples {

HelloSettingsLayer::HelloSettingsLayer()
    : vne::testbed::ILayer("HelloSettingsLayer") {
    setRenderSortKey(kRenderSortKey);
}

void HelloSettingsLayer::onAttach(vne::testbed::AppContext& /*app_context*/) {
#ifdef VNE_TESTBED_IMGUI
    if (imgui_layer_) {
        imgui_layer_->setSettingsCallback([this]() { renderPanel(); });
    }
#endif
}

void HelloSettingsLayer::onDetach() {
#ifdef VNE_TESTBED_IMGUI
    if (imgui_layer_) {
        imgui_layer_->setSettingsCallback(nullptr);
        imgui_layer_ = nullptr;
    }
#endif
}

#ifdef VNE_TESTBED_IMGUI
void HelloSettingsLayer::setImGuiLayer(vne::testbed::ImGuiLayer* layer) {
    imgui_layer_ = layer;
}

void HelloSettingsLayer::renderPanel() {
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
#endif

void registerHelloTestbedDemo(vne::testbed::Application& app) {
    // Layer 1: grid + axes + perspective camera
    auto* scene = new BaseSceneLayer("HelloBaseSceneLayer");
    app.getLayerStack().pushLayer(std::unique_ptr<BaseSceneLayer>(scene), app.getAppContext());

#ifdef VNE_TESTBED_INTERACTION
    // Layer 2: orbit-arcball interaction (per-viewport cameras when using 2 or 4 viewports)
    auto* interaction = new BaseInteractionLayer("HelloInteractionLayer");
    interaction->setSceneLayer(scene);
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

VNETESTBED_REGISTER_DEMO("hello_testbed", registerHelloTestbedDemo)

}  // namespace vne::samples
