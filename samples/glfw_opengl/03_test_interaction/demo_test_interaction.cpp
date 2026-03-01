/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Sample 03_test_interaction
 * --------------------------
 * Everything from 00_hello_testbed (grid + axes + camera) plus a full
 * Interaction panel that lets you test every vneinteraction feature live.
 *
 * What you can test here:
 *   • Switch between all 4 CameraManipulatorType values at runtime:
 *       OrbitArcball  — LMB rotate, RMB pan, scroll zoom
 *       FpsFly        — RMB hold + WASD + QE, scroll speed, mouse look
 *       OrthoPanZoom  — LMB/RMB pan, scroll zoom (no rotation)
 *       Follow        — camera follows a moving target
 *   • ZoomMethod (OrbitArcball only):
 *       DollyToCoi    — moves camera along view ray
 *       SceneScale    — scales the "virtual" scene
 *       ChangeFov     — widens/narrows FOV
 *   • ViewDirection preset buttons: Front / Back / Left / Right / Top / Bottom / Iso
 *   • Camera readout: position (x,y,z), target, orbit distance
 *   • FpsFly speed and mouse-sensitivity sliders
 *
 * Libraries exercised: vne::testbed, vne::scene, vne::events, vne::interaction
 *
 * Only compiled when VNE_TESTBED_INTERACTION is defined.
 * ----------------------------------------------------------------------
 */

#ifdef VNE_TESTBED_INTERACTION

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include "vertexnova/events/event.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"

#include "vertexnova/interaction/camera_system_controller.h"
#include "vertexnova/interaction/fps_fly_manipulator.h"
#include "vertexnova/interaction/interaction_types.h"
#include "vertexnova/interaction/orbit_arcball_manipulator.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#include "../common/base_scene_layer.h"

#include <memory>
#include <string>
#include <vector>

namespace {

// ---------------------------------------------------------------------------
// InteractionTestLayer — owns camera + controller, exposes full control API
// ---------------------------------------------------------------------------
class InteractionTestLayer : public vne::testbed::ILayer {
   public:
    static constexpr double kFixedDt = 0.016;

    InteractionTestLayer()
        : vne::testbed::ILayer("InteractionTestLayer") {
        controllers_.resize(BaseSceneLayer::kMaxViewports);
        for (int i = 0; i < BaseSceneLayer::kMaxViewports; ++i) {
            controllers_[i] = std::make_unique<vne::interaction::CameraSystemController>(
                vne::interaction::CameraManipulatorType::eOrbitArcball);
        }
    }

    void setCamera(std::shared_ptr<vne::scene::PerspectiveCamera> cam) {
        camera_ = std::move(cam);
        if (!controllers_.empty() && controllers_[0]) {
            controllers_[0]->setCamera(camera_);
        }
    }

    void setSceneLayer(const BaseSceneLayer* scene) {
        if (scene) {
            const auto& cams = scene->getCameras();
            for (size_t i = 0; i < cams.size() && i < controllers_.size(); ++i) {
                if (controllers_[i] && cams[i]) {
                    controllers_[i]->setCamera(cams[i]);
                }
            }
            camera_ = scene->getCamera();
        }
    }

#ifdef VNE_TESTBED_IMGUI
    void setImGuiLayer(vne::testbed::ImGuiLayer* layer) { imgui_layer_ = layer; }
#endif

    void onAttach(vne::testbed::AppContext& ctx) override {
        const float vpw = ctx.window ? static_cast<float>(ctx.window->getWidth()) : 1280.0f;
        const float vph = ctx.window ? static_cast<float>(ctx.window->getHeight()) : 720.0f;
        for (auto& ctrl : controllers_) {
            if (ctrl) {
                ctrl->setViewportSize(vpw, vph);
            }
        }
    }

    void onDetach() override {}

    void onUpdate(float dt) override {
        for (auto& ctrl : controllers_) {
            if (ctrl)
                ctrl->update(static_cast<double>(dt));
        }
    }

    void onEvent(const vne::events::Event& event) override {
        if (controllers_.empty() || !controllers_[0])
            return;
        using ET = vne::events::EventType;
        float check_x = static_cast<float>(last_x_);
        float check_y = static_cast<float>(last_y_);
        if (event.type() == ET::eMouseMoved) {
            const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
            check_x = static_cast<float>(e.x());
            check_y = static_cast<float>(e.y());
        }
        int viewport_index = 0;
#ifdef VNE_TESTBED_IMGUI
        if (imgui_layer_) {
            const int idx = imgui_layer_->getHoveredViewportIndex(check_x, check_y);
            if (idx < 0)
                return;
            viewport_index = idx;
        }
#endif
        auto* controller = (viewport_index >= 0 && viewport_index < static_cast<int>(controllers_.size()))
                              ? controllers_[static_cast<size_t>(viewport_index)].get()
                              : controllers_[0].get();
        if (!controller)
            return;
        switch (event.type()) {
            case ET::eMouseMoved: {
                const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                const double dx = first_mouse_ ? 0.0 : (e.x() - last_x_);
                const double dy = first_mouse_ ? 0.0 : (e.y() - last_y_);
                last_x_ = e.x();
                last_y_ = e.y();
                first_mouse_ = false;
                controller->handleMouseMove(static_cast<float>(e.x()),
                                             static_cast<float>(e.y()),
                                             static_cast<float>(dx),
                                             static_cast<float>(dy),
                                             kFixedDt);
                break;
            }
            case ET::eMouseButtonPressed: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller->handleMouseButton(static_cast<int>(e.button()),
                                               true,
                                               static_cast<float>(last_x_),
                                               static_cast<float>(last_y_),
                                               kFixedDt);
                break;
            }
            case ET::eMouseButtonReleased: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller->handleMouseButton(static_cast<int>(e.button()),
                                               false,
                                               static_cast<float>(last_x_),
                                               static_cast<float>(last_y_),
                                               kFixedDt);
                break;
            }
            case ET::eMouseScrolled: {
                const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
                controller->handleMouseScroll(static_cast<float>(e.xOffset()),
                                               static_cast<float>(e.yOffset()),
                                               kFixedDt);
                break;
            }
            case ET::eKeyPressed: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller->handleKeyboard(static_cast<int>(e.keyCode()), true, kFixedDt);
                break;
            }
            case ET::eKeyReleased: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller->handleKeyboard(static_cast<int>(e.keyCode()), false, kFixedDt);
                break;
            }
            default:
                break;
        }
    }

    // -----------------------------------------------------------------------
    // Control API for the Settings panel
    // -----------------------------------------------------------------------
    void setManipulatorType(vne::interaction::CameraManipulatorType type) {
        for (auto& ctrl : controllers_) {
            if (ctrl) {
                ctrl->setManipulator(type);
                if (camera_)
                    ctrl->setCamera(camera_);
            }
        }
    }

    [[nodiscard]] vne::interaction::CameraManipulatorType getManipulatorType() const {
        return controllers_.empty() || !controllers_[0]
                   ? vne::interaction::CameraManipulatorType::eOrbitArcball
                   : controllers_[0]->getManipulatorType();
    }

    void setZoomMethod(vne::interaction::ZoomMethod method) {
        for (auto& ctrl : controllers_) {
            if (!ctrl)
                continue;
            auto* m = ctrl->getManipulator();
            if (!m)
                continue;
            auto* orbit = dynamic_cast<vne::interaction::OrbitArcballManipulator*>(m);
            if (orbit)
                orbit->setZoomMethod(method);
        }
    }

    void setViewDirection(vne::interaction::ViewDirection dir) {
        for (auto& ctrl : controllers_) {
            if (!ctrl)
                continue;
            auto* m = ctrl->getManipulator();
            if (!m)
                continue;
            auto* orbit = dynamic_cast<vne::interaction::OrbitArcballManipulator*>(m);
            if (orbit)
                orbit->setViewDirection(dir);
        }
    }

    void resetCamera() {
        if (!camera_)
            return;
        camera_->setPosition({4.f, 3.f, 6.f});
        camera_->setTarget({0.f, 0.f, 0.f});
        camera_->updateMatrices();
        for (auto& ctrl : controllers_) {
            if (ctrl)
                ctrl->reset();
        }
    }

    void setFpsSpeed(float s) {
        for (auto& ctrl : controllers_) {
            if (!ctrl)
                continue;
            auto* m = ctrl->getManipulator();
            if (!m)
                continue;
            auto* fps = dynamic_cast<vne::interaction::FpsFlyManipulator*>(m);
            if (fps)
                fps->setMoveSpeed(s);
        }
    }

    void setFpsSensitivity(float s) {
        for (auto& ctrl : controllers_) {
            if (!ctrl)
                continue;
            auto* m = ctrl->getManipulator();
            if (!m)
                continue;
            auto* fps = dynamic_cast<vne::interaction::FpsFlyManipulator*>(m);
            if (fps)
                fps->setMouseSensitivity(s);
        }
    }

    [[nodiscard]] vne::math::Vec3f cameraPosition() const {
        return camera_ ? camera_->getPosition() : vne::math::Vec3f{};
    }
    [[nodiscard]] vne::math::Vec3f cameraTarget() const { return camera_ ? camera_->getTarget() : vne::math::Vec3f{}; }

   private:
    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
    std::vector<std::unique_ptr<vne::interaction::CameraSystemController>> controllers_;
    double last_x_{0.0};
    double last_y_{0.0};
    bool first_mouse_{true};
#ifdef VNE_TESTBED_IMGUI
    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
#endif
};

// ---------------------------------------------------------------------------
// InteractionSettingsLayer — ImGui panel
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI
class InteractionSettingsLayer : public vne::testbed::ILayer {
   public:
    InteractionSettingsLayer()
        : vne::testbed::ILayer("InteractionSettingsLayer") {
        setRenderSortKey(999);
    }

    void setImGuiLayer(vne::testbed::ImGuiLayer* l) { imgui_layer_ = l; }
    void setInteractionLayer(InteractionTestLayer* l) { interaction_layer_ = l; }

    void onAttach(vne::testbed::AppContext& /*ctx*/) override {
        if (imgui_layer_) {
            imgui_layer_->setSettingsCallback([this]() { renderPanel(); });
        }
    }
    void onDetach() override {
        if (imgui_layer_) {
            imgui_layer_->setSettingsCallback(nullptr);
            imgui_layer_ = nullptr;
        }
        interaction_layer_ = nullptr;
    }

   private:
    void renderPanel() {
        if (!interaction_layer_)
            return;
        auto& il = *interaction_layer_;

        // ---- Manipulator type ----
        if (ImGui::CollapsingHeader("Manipulator", ImGuiTreeNodeFlags_DefaultOpen)) {
            using MT = vne::interaction::CameraManipulatorType;
            const char* types[] = {"OrbitArcball", "FpsFly", "OrthoPanZoom", "Follow"};
            const MT values[] = {MT::eOrbitArcball, MT::eFpsFly, MT::eOrthoPanZoom, MT::eFollow};
            int idx = 0;
            const MT cur = il.getManipulatorType();
            for (int i = 0; i < 4; ++i) {
                if (values[i] == cur) {
                    idx = i;
                    break;
                }
            }
            if (ImGui::Combo("Type", &idx, types, 4)) {
                il.setManipulatorType(values[idx]);
            }

            ImGui::Spacing();
            // Controls hint per manipulator
            switch (il.getManipulatorType()) {
                case MT::eOrbitArcball:
                    ImGui::TextDisabled("LMB rotate  RMB pan  Scroll zoom");
                    break;
                case MT::eFpsFly:
                    ImGui::TextDisabled("RMB + WASD/QE move  Mouse look");
                    break;
                case MT::eOrthoPanZoom:
                    ImGui::TextDisabled("LMB/RMB pan  Scroll zoom (no rotate)");
                    break;
                case MT::eFollow:
                    ImGui::TextDisabled("Camera follows the target object");
                    break;
            }
        }

        // ---- Zoom method (OrbitArcball only) ----
        if (il.getManipulatorType() == vne::interaction::CameraManipulatorType::eOrbitArcball) {
            if (ImGui::CollapsingHeader("Zoom Method (Orbit)", ImGuiTreeNodeFlags_DefaultOpen)) {
                using ZM = vne::interaction::ZoomMethod;
                const char* znames[] = {"DollyToCoi", "SceneScale", "ChangeFov"};
                const ZM zvals[] = {ZM::eDollyToCoi, ZM::eSceneScale, ZM::eChangeFov};
                if (ImGui::Combo("Method", &zoom_idx_, znames, 3)) {
                    il.setZoomMethod(zvals[zoom_idx_]);
                }
                ImGui::Spacing();
                ImGui::TextDisabled("DollyToCoi: move along ray to pivot");
                ImGui::TextDisabled("SceneScale: virtual scene scale");
                ImGui::TextDisabled("ChangeFov:  widen/narrow FOV angle");
            }
        }

        // ---- View direction presets (OrbitArcball only) ----
        if (il.getManipulatorType() == vne::interaction::CameraManipulatorType::eOrbitArcball) {
            if (ImGui::CollapsingHeader("View Direction", ImGuiTreeNodeFlags_DefaultOpen)) {
                using VD = vne::interaction::ViewDirection;
                struct {
                    const char* label;
                    VD dir;
                } presets[] = {
                    {"Front", VD::eFront},
                    {"Back", VD::eBack},
                    {"Left", VD::eLeft},
                    {"Right", VD::eRight},
                    {"Top", VD::eTop},
                    {"Bottom", VD::eBottom},
                    {"Iso", VD::eIso},
                };
                for (auto& p : presets) {
                    if (ImGui::Button(p.label)) {
                        il.setViewDirection(p.dir);
                    }
                    ImGui::SameLine();
                }
                ImGui::NewLine();
            }
        }

        // ---- FpsFly controls (when active) ----
        if (il.getManipulatorType() == vne::interaction::CameraManipulatorType::eFpsFly) {
            if (ImGui::CollapsingHeader("FpsFly Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (ImGui::SliderFloat("Move speed##fps", &fps_speed_, 0.5f, 20.f)) {
                    il.setFpsSpeed(fps_speed_);
                }
                if (ImGui::SliderFloat("Mouse sensitivity##fps", &fps_sensitivity_, 0.05f, 0.5f)) {
                    il.setFpsSensitivity(fps_sensitivity_);
                }
            }
        }

        // ---- Camera readout ----
        if (ImGui::CollapsingHeader("Camera State", ImGuiTreeNodeFlags_DefaultOpen)) {
            const auto pos = il.cameraPosition();
            const auto tgt = il.cameraTarget();
            ImGui::Text("Position: %.2f  %.2f  %.2f",
                        static_cast<double>(pos.x()),
                        static_cast<double>(pos.y()),
                        static_cast<double>(pos.z()));
            ImGui::Text("Target:   %.2f  %.2f  %.2f",
                        static_cast<double>(tgt.x()),
                        static_cast<double>(tgt.y()),
                        static_cast<double>(tgt.z()));
            ImGui::Spacing();
            if (ImGui::Button("Reset camera")) {
                il.resetCamera();
            }
        }
    }

    vne::testbed::ImGuiLayer* imgui_layer_{nullptr};
    InteractionTestLayer* interaction_layer_{nullptr};

    int zoom_idx_{0};
    float fps_speed_{3.0f};
    float fps_sensitivity_{0.15f};
};
#endif  // VNE_TESTBED_IMGUI

// ---------------------------------------------------------------------------

void RegisterTestInteractionDemo(vne::testbed::Application& app) {
    // Layer 1: grid + axes base scene (owns per-viewport cameras)
    auto* scene = new BaseSceneLayer("TestInteractionBaseSceneLayer");
    app.getLayerStack().pushLayer(std::unique_ptr<BaseSceneLayer>(scene), app.getAppContext());

    // Layer 2: interaction (per-viewport controllers when using 2 or 4 viewports)
    auto* interaction = new InteractionTestLayer();
    interaction->setSceneLayer(scene);
    app.getLayerStack().pushLayer(std::unique_ptr<InteractionTestLayer>(interaction), app.getAppContext());

#ifdef VNE_TESTBED_IMGUI
    auto* settings = new InteractionSettingsLayer();
    auto* imgui = dynamic_cast<vne::testbed::ImGuiLayer*>(app.getLayerStack().findLayerByName("ImGuiLayer"));
    if (imgui) {
        settings->setImGuiLayer(imgui);
        interaction->setImGuiLayer(imgui);
    }
    settings->setInteractionLayer(interaction);
    app.getLayerStack().pushLayer(std::unique_ptr<InteractionSettingsLayer>(settings), app.getAppContext());
#endif
}

}  // namespace

VNETESTBED_REGISTER_DEMO("test_interaction", RegisterTestInteractionDemo)

#endif  // VNE_TESTBED_INTERACTION
