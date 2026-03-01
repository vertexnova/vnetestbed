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
#include "vertexnova/events/event_listener.h"
#include "vertexnova/events/event_manager.h"
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

namespace {

// Non-owning shared_ptr helper
vne::events::EventManager::ListenerPtr asListenerPtr(vne::events::EventListener* raw) {
    return {raw, [](vne::events::EventListener*) {}};
}

// ---------------------------------------------------------------------------
// InteractionTestLayer — owns camera + controller, exposes full control API
// ---------------------------------------------------------------------------
class InteractionTestLayer : public vne::testbed::ILayer, public vne::events::EventListener {
   public:
    static constexpr double kFixedDt = 0.016;

    InteractionTestLayer()
        : vne::testbed::ILayer("InteractionTestLayer")
        , controller_(std::make_unique<vne::interaction::CameraSystemController>(
              vne::interaction::CameraManipulatorType::eOrbitArcball)) {}

    void setCamera(std::shared_ptr<vne::scene::PerspectiveCamera> cam) {
        camera_ = std::move(cam);
        if (controller_) {
            controller_->setCamera(camera_);
        }
    }

    void onAttach(vne::testbed::AppContext& ctx) override {
        auto& mgr = vne::events::EventManager::instance();
        auto self = asListenerPtr(this);
        mgr.registerListener(vne::events::EventType::eMouseMoved, self);
        mgr.registerListener(vne::events::EventType::eMouseButtonPressed, self);
        mgr.registerListener(vne::events::EventType::eMouseButtonReleased, self);
        mgr.registerListener(vne::events::EventType::eMouseScrolled, self);
        mgr.registerListener(vne::events::EventType::eKeyPressed, self);
        mgr.registerListener(vne::events::EventType::eKeyReleased, self);
        if (ctx.window && controller_) {
            controller_->setViewportSize(static_cast<float>(ctx.window->getWidth()),
                                         static_cast<float>(ctx.window->getHeight()));
        }
    }

    void onDetach() override {
        auto& mgr = vne::events::EventManager::instance();
        mgr.unregisterListener(vne::events::EventType::eMouseMoved, this);
        mgr.unregisterListener(vne::events::EventType::eMouseButtonPressed, this);
        mgr.unregisterListener(vne::events::EventType::eMouseButtonReleased, this);
        mgr.unregisterListener(vne::events::EventType::eMouseScrolled, this);
        mgr.unregisterListener(vne::events::EventType::eKeyPressed, this);
        mgr.unregisterListener(vne::events::EventType::eKeyReleased, this);
    }

    void onUpdate(float dt) override {
        if (controller_)
            controller_->update(static_cast<double>(dt));
    }

    void onEvent(const vne::events::Event& event) override {
        if (!controller_)
            return;
        using ET = vne::events::EventType;
        switch (event.type()) {
            case ET::eMouseMoved: {
                const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                const double dx = first_mouse_ ? 0.0 : (e.x() - last_x_);
                const double dy = first_mouse_ ? 0.0 : (e.y() - last_y_);
                last_x_ = e.x();
                last_y_ = e.y();
                first_mouse_ = false;
                controller_->handleMouseMove(static_cast<float>(e.x()),
                                             static_cast<float>(e.y()),
                                             static_cast<float>(dx),
                                             static_cast<float>(dy),
                                             kFixedDt);
                break;
            }
            case ET::eMouseButtonPressed: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller_->handleMouseButton(static_cast<int>(e.button()),
                                               true,
                                               static_cast<float>(last_x_),
                                               static_cast<float>(last_y_),
                                               kFixedDt);
                break;
            }
            case ET::eMouseButtonReleased: {
                const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
                controller_->handleMouseButton(static_cast<int>(e.button()),
                                               false,
                                               static_cast<float>(last_x_),
                                               static_cast<float>(last_y_),
                                               kFixedDt);
                break;
            }
            case ET::eMouseScrolled: {
                const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
                controller_->handleMouseScroll(static_cast<float>(e.xOffset()),
                                               static_cast<float>(e.yOffset()),
                                               kFixedDt);
                break;
            }
            case ET::eKeyPressed: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller_->handleKeyboard(static_cast<int>(e.keyCode()), true, kFixedDt);
                break;
            }
            case ET::eKeyReleased: {
                const auto& e = static_cast<const vne::events::KeyEvent&>(event);
                controller_->handleKeyboard(static_cast<int>(e.keyCode()), false, kFixedDt);
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
        if (controller_) {
            controller_->setManipulator(type);
            // Re-apply camera after manipulator switch
            if (camera_)
                controller_->setCamera(camera_);
        }
    }

    [[nodiscard]] vne::interaction::CameraManipulatorType getManipulatorType() const {
        return controller_ ? controller_->getManipulatorType() : vne::interaction::CameraManipulatorType::eOrbitArcball;
    }

    void setZoomMethod(vne::interaction::ZoomMethod method) {
        if (!controller_)
            return;
        auto* m = controller_->getManipulator();
        if (!m)
            return;
        auto* orbit = dynamic_cast<vne::interaction::OrbitArcballManipulator*>(m);
        if (orbit)
            orbit->setZoomMethod(method);
    }

    void setViewDirection(vne::interaction::ViewDirection dir) {
        if (!controller_)
            return;
        auto* m = controller_->getManipulator();
        if (!m)
            return;
        auto* orbit = dynamic_cast<vne::interaction::OrbitArcballManipulator*>(m);
        if (orbit)
            orbit->setViewDirection(dir);
    }

    void resetCamera() {
        if (!camera_)
            return;
        camera_->setPosition({4.f, 3.f, 6.f});
        camera_->setTarget({0.f, 0.f, 0.f});
        camera_->updateMatrices();
        if (controller_)
            controller_->reset();
    }

    void setFpsSpeed(float s) {
        if (!controller_)
            return;
        auto* m = controller_->getManipulator();
        if (!m)
            return;
        auto* fps = dynamic_cast<vne::interaction::FpsFlyManipulator*>(m);
        if (fps)
            fps->setMoveSpeed(s);
    }

    void setFpsSensitivity(float s) {
        if (!controller_)
            return;
        auto* m = controller_->getManipulator();
        if (!m)
            return;
        auto* fps = dynamic_cast<vne::interaction::FpsFlyManipulator*>(m);
        if (fps)
            fps->setMouseSensitivity(s);
    }

    [[nodiscard]] vne::math::Vec3f cameraPosition() const {
        return camera_ ? camera_->getPosition() : vne::math::Vec3f{};
    }
    [[nodiscard]] vne::math::Vec3f cameraTarget() const { return camera_ ? camera_->getTarget() : vne::math::Vec3f{}; }

   private:
    std::shared_ptr<vne::scene::PerspectiveCamera> camera_;
    std::unique_ptr<vne::interaction::CameraSystemController> controller_;
    double last_x_{0.0};
    double last_y_{0.0};
    bool first_mouse_{true};
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
    // Layer 1: grid + axes base scene (owns camera)
    auto* scene = new BaseSceneLayer("TestInteractionBaseSceneLayer");
    app.getLayerStack().pushLayer(std::unique_ptr<BaseSceneLayer>(scene), app.getAppContext());

    // Layer 2: interaction (owns controller, shares camera with BaseSceneLayer)
    auto* interaction = new InteractionTestLayer();
    interaction->setCamera(scene->getCamera());
    app.getLayerStack().pushLayer(std::unique_ptr<InteractionTestLayer>(interaction), app.getAppContext());

#ifdef VNE_TESTBED_IMGUI
    auto* settings = new InteractionSettingsLayer();
    auto* imgui = dynamic_cast<vne::testbed::ImGuiLayer*>(app.getLayerStack().findLayerByName("ImGuiLayer"));
    if (imgui)
        settings->setImGuiLayer(imgui);
    settings->setInteractionLayer(interaction);
    app.getLayerStack().pushLayer(std::unique_ptr<InteractionSettingsLayer>(settings), app.getAppContext());
#endif
}

}  // namespace

VNETESTBED_REGISTER_DEMO("test_interaction", RegisterTestInteractionDemo)

#endif  // VNE_TESTBED_INTERACTION
