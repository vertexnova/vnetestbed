/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   January 2026
 *
 * Sample 03_test_interaction — implementation (interaction layer, settings panel).
 * ----------------------------------------------------------------------
 */

#ifdef VNE_TESTBED_INTERACTION

#include "demo_test_interaction.h"

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"

#include "vertexnova/events/event.h"
#include "vertexnova/events/input/input.h"
#include "vertexnova/events/key_event.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/events/window_event.h"

#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/interaction/fly_manipulator.h"
#include "vertexnova/interaction/fps_manipulator.h"
#include "vertexnova/interaction/orbit_manipulator.h"
#include "vertexnova/interaction/ortho_pan_zoom_manipulator.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#ifdef VNE_TESTBED_VNEIO
#include "vertexnova/testbed/utils/mesh_layer.h"
#endif

#include <algorithm>

#include "../common/base_scene_layer.h"
#include "../common/path_utils.h"

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace {}  // namespace

namespace vne::samples::test_interaction {

// ---------------------------------------------------------------------------
// InteractionTestLayer implementation
// ---------------------------------------------------------------------------

InteractionTestLayer::InteractionTestLayer()
    : vne::testbed::ILayer("InteractionTestLayer") {
    controllers_.resize(static_cast<size_t>(kMaxViewports));
    for (size_t i = 0; i < static_cast<size_t>(kMaxViewports); ++i) {
        auto ctrl = std::make_unique<vne::interaction::CameraSystemController>();
        ctrl->setManipulator(factory_.create(vne::interaction::CameraManipulatorType::eOrbit));
        controllers_[i] = std::move(ctrl);
    }
}

void InteractionTestLayer::setCamera(std::shared_ptr<vne::scene::ICamera> cam) {
    camera_ = std::move(cam);
    if (!controllers_.empty() && controllers_[0]) {
        controllers_[0]->setCamera(camera_);
    }
}

void InteractionTestLayer::setSceneLayer(BaseSceneLayer* scene) {
    scene_layer_ = scene;
    setCamerasFromScene();
}

void InteractionTestLayer::setCamerasFromScene() {
    if (!scene_layer_) {
        return;
    }
    const auto cams = scene_layer_->getActiveCameras();
    camera_ = cams.empty() ? nullptr : cams[0];
    for (size_t i = 0; i < cams.size() && i < controllers_.size(); ++i) {
        if (controllers_[i] && cams[i]) {
            controllers_[i]->setCamera(cams[i]);
        }
    }
}

bool InteractionTestLayer::isManipulatorCompatibleWithCamera(bool use_perspective) const {
    if (current_manipulator_type_ == vne::interaction::CameraManipulatorType::eOrthoPanZoom) {
        return !use_perspective;  // OrthoPanZoom requires orthographic
    }
    return true;  // Orbit, Arcball, Fps, Fly, Follow support both
}

#ifdef VNE_TESTBED_IMGUI
void InteractionTestLayer::setImGuiLayer(vne::testbed::ImGuiLayer* layer) {
    imgui_layer_ = layer;
}
#endif

void InteractionTestLayer::onAttach(vne::testbed::AppContext& ctx) {
    const float vpw = ctx.window ? static_cast<float>(ctx.window->getWidth()) : 1280.0f;
    const float vph = ctx.window ? static_cast<float>(ctx.window->getHeight()) : 720.0f;
    for (auto& ctrl : controllers_) {
        if (ctrl) {
            ctrl->setViewportSize(vpw, vph);
        }
    }
}

void InteractionTestLayer::onDetach() {}

void InteractionTestLayer::onUpdate(float dt) {
#ifdef VNE_TESTBED_IMGUI
    if (imgui_layer_) {
        const int n = static_cast<int>(controllers_.size());
        for (int i = 0; i < n; ++i) {
            float min_x = 0.0f, min_y = 0.0f, max_x = 0.0f, max_y = 0.0f;
            if (imgui_layer_->getViewportRect(i, min_x, min_y, max_x, max_y) && controllers_[static_cast<size_t>(i)]) {
                const float vp_w = max_x - min_x;
                const float vp_h = max_y - min_y;
                controllers_[static_cast<size_t>(i)]->setViewportSize(vp_w, vp_h);
            }
        }
    }
#endif
    for (auto& ctrl : controllers_) {
        if (ctrl) {
            ctrl->update(static_cast<double>(dt));
        }
    }
}

void InteractionTestLayer::onEvent(const vne::events::Event& event) {
    if (controllers_.empty() || !controllers_[0]) {
        return;
    }
    using ET = vne::events::EventType;
    // Handle resize early so viewport is updated regardless of mouse position.
    if (event.type() == ET::eWindowResize) {
        const auto& e = static_cast<const vne::events::WindowResizeEvent&>(event);
        const float vpw = static_cast<float>(e.width());
        const float vph = static_cast<float>(e.height());
        for (auto& ctrl : controllers_) {
            if (ctrl) {
                ctrl->setViewportSize(vpw, vph);
            }
        }
        return;
    }
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
        viewport_index = (idx >= 0) ? idx : 0;
        // When mouse is over ImGui panel (idx < 0), do not forward mouse events to
        // the camera — e.g. scrolling in the control panel should scroll the panel,
        // not zoom the scene.
        if (idx < 0) {
            const auto t = event.type();
            if (t == ET::eMouseScrolled || t == ET::eMouseMoved || t == ET::eMouseButtonPressed
                || t == ET::eMouseButtonReleased) {
                if (t == ET::eMouseMoved) {
                    const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                    last_x_ = e.x();
                    last_y_ = e.y();
                    first_mouse_ = false;
                } else if (t == ET::eMouseScrolled) {
                    const auto [mx, my] = vne::events::Input::mousePosition();
                    last_x_ = static_cast<double>(mx);
                    last_y_ = static_cast<double>(my);
                }
                return;
            }
        }
    }
#endif
    auto* controller = (viewport_index >= 0 && viewport_index < static_cast<int>(controllers_.size()))
                           ? controllers_[static_cast<size_t>(viewport_index)].get()
                           : controllers_[0].get();
    if (!controller) {
        return;
    }
    // Viewport-local coordinates: when ImGui viewports are used, convert window coords to viewport-local
    float vp_min_x = 0.0f;
    float vp_min_y = 0.0f;
    float vp_max_x = 0.0f;
    float vp_max_y = 0.0f;
    bool use_viewport_local = false;
#ifdef VNE_TESTBED_IMGUI
    if (imgui_layer_ && imgui_layer_->getViewportRect(viewport_index, vp_min_x, vp_min_y, vp_max_x, vp_max_y)) {
        use_viewport_local = true;
        const float vp_w = vp_max_x - vp_min_x;
        const float vp_h = vp_max_y - vp_min_y;
        controller->setViewportSize(vp_w, vp_h);
    }
#endif
    auto toLocal = [&](float wx, float wy) -> std::pair<float, float> {
        if (use_viewport_local) {
            return {wx - vp_min_x, wy - vp_min_y};
        }
        return {wx, wy};
    };
    switch (event.type()) {
        case ET::eMouseMoved: {
            const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
            const double dx = first_mouse_ ? 0.0 : (e.x() - last_x_);
            const double dy = first_mouse_ ? 0.0 : (e.y() - last_y_);
            last_x_ = e.x();
            last_y_ = e.y();
            first_mouse_ = false;
            const auto [lx, ly] = toLocal(static_cast<float>(e.x()), static_cast<float>(e.y()));
            controller->handleMouseMove(lx, ly, static_cast<float>(dx), static_cast<float>(dy), kFixedDt);
            break;
        }
        case ET::eMouseButtonPressed: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            const auto [mx, my] = vne::events::Input::mousePosition();
            last_x_ = static_cast<double>(mx);
            last_y_ = static_cast<double>(my);
            first_mouse_ = false;
            const auto [lx, ly] = toLocal(static_cast<float>(mx), static_cast<float>(my));
            controller->handleMouseButton(static_cast<int>(e.button()), true, lx, ly, kFixedDt);
            break;
        }
        case ET::eMouseButtonReleased: {
            const auto& e = static_cast<const vne::events::MouseButtonEvent&>(event);
            const auto [mx, my] = vne::events::Input::mousePosition();
            last_x_ = static_cast<double>(mx);
            last_y_ = static_cast<double>(my);
            const auto [lx, ly] = toLocal(static_cast<float>(mx), static_cast<float>(my));
            controller->handleMouseButton(static_cast<int>(e.button()), false, lx, ly, kFixedDt);
            break;
        }
        case ET::eMouseScrolled: {
            const auto& e = static_cast<const vne::events::MouseScrolledEvent&>(event);
            const auto [mx, my] = vne::events::Input::mousePosition();
            last_x_ = static_cast<double>(mx);
            last_y_ = static_cast<double>(my);
            const auto [lx, ly] = toLocal(static_cast<float>(mx), static_cast<float>(my));
            controller->handleMouseScroll(static_cast<float>(e.xOffset()),
                                          static_cast<float>(e.yOffset()),
                                          lx,
                                          ly,
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

void InteractionTestLayer::setManipulatorType(vne::interaction::CameraManipulatorType type) {
    current_manipulator_type_ = type;
    for (auto& ctrl : controllers_) {
        if (ctrl) {
            ctrl->setManipulator(factory_.create(type));
            if (camera_) {
                ctrl->setCamera(camera_);
            }
        }
    }
}

vne::interaction::CameraManipulatorType InteractionTestLayer::getManipulatorType() const {
    return current_manipulator_type_;
}

vne::interaction::CameraSystemController* InteractionTestLayer::getController() const {
    return controllers_.empty() ? nullptr : controllers_[0].get();
}

vne::interaction::CameraSystemController* InteractionTestLayer::getController(int index) const {
    if (index >= 0 && index < static_cast<int>(controllers_.size())) {
        return controllers_[static_cast<size_t>(index)].get();
    }
    return getController();
}

void InteractionTestLayer::setZoomMethod(vne::interaction::ZoomMethod method) {
    for (auto& ctrl : controllers_) {
        if (!ctrl)
            continue;
        auto m = ctrl->getManipulator();
        if (!m)
            continue;
        if (auto* orbit = dynamic_cast<vne::interaction::OrbitManipulator*>(m.get())) {
            orbit->setZoomMethod(method);
        } else if (auto* arc = dynamic_cast<vne::interaction::ArcballManipulator*>(m.get())) {
            arc->setZoomMethod(method);
        } else if (auto* fps = dynamic_cast<vne::interaction::FpsManipulator*>(m.get())) {
            fps->setZoomMethod(method);
        } else if (auto* fly = dynamic_cast<vne::interaction::FlyManipulator*>(m.get())) {
            fly->setZoomMethod(method);
        } else if (auto* ortho = dynamic_cast<vne::interaction::OrthoPanZoomManipulator*>(m.get())) {
            ortho->setZoomMethod(method);
        }
    }
}

void InteractionTestLayer::setViewDirection(vne::interaction::ViewDirection dir) {
    for (auto& ctrl : controllers_) {
        if (!ctrl) {
            continue;
        }
        auto m = ctrl->getManipulator();
        if (!m) {
            continue;
        }
        if (auto* orbit = dynamic_cast<vne::interaction::OrbitManipulator*>(m.get())) {
            orbit->setViewDirection(dir);
        }
    }
}

void InteractionTestLayer::resetCamera() {
    if (!camera_) {
        return;
    }
    camera_->setPosition({4.f, 3.f, 6.f});
    camera_->setTarget({0.f, 0.f, 0.f});
    camera_->updateMatrices();
    for (auto& ctrl : controllers_) {
        if (ctrl) {
            ctrl->reset();
        }
    }
}

void InteractionTestLayer::setFpsSpeed(float speed) {
    for (auto& ctrl : controllers_) {
        if (!ctrl) {
            continue;
        }
        auto m = ctrl->getManipulator();
        if (!m) {
            continue;
        }
        if (auto* fps = dynamic_cast<vne::interaction::FpsManipulator*>(m.get())) {
            fps->setMoveSpeed(speed);
        } else if (auto* fly = dynamic_cast<vne::interaction::FlyManipulator*>(m.get())) {
            fly->setMoveSpeed(speed);
        }
    }
}

void InteractionTestLayer::setFpsSensitivity(float sensitivity) {
    for (auto& ctrl : controllers_) {
        if (!ctrl) {
            continue;
        }
        auto m = ctrl->getManipulator();
        if (!m) {
            continue;
        }
        if (auto* fps = dynamic_cast<vne::interaction::FpsManipulator*>(m.get())) {
            fps->setMouseSensitivity(sensitivity);
        } else if (auto* fly = dynamic_cast<vne::interaction::FlyManipulator*>(m.get())) {
            fly->setMouseSensitivity(sensitivity);
        }
    }
}

vne::math::Vec3f InteractionTestLayer::cameraPosition() const {
    return camera_ ? camera_->getPosition() : vne::math::Vec3f{};
}

vne::math::Vec3f InteractionTestLayer::cameraTarget() const {
    return camera_ ? camera_->getTarget() : vne::math::Vec3f{};
}

// ---------------------------------------------------------------------------
// InteractionSettingsLayer implementation
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI

InteractionSettingsLayer::InteractionSettingsLayer()
    : vne::testbed::ILayer("InteractionSettingsLayer") {
    setRenderSortKey(999);
}

void InteractionSettingsLayer::setImGuiLayer(vne::testbed::ImGuiLayer* layer) {
    imgui_layer_ = layer;
}

void InteractionSettingsLayer::setInteractionLayer(InteractionTestLayer* layer) {
    interaction_layer_ = layer;
}

void InteractionSettingsLayer::setMeshLayer(vne::testbed::MeshLayer* layer) {
    mesh_layer_ = layer;
}

void InteractionSettingsLayer::setMeshesDir(std::string dir) {
    meshes_dir_ = std::move(dir);
    mesh_files_.clear();
    selected_mesh_idx_ = -1;
    if (meshes_dir_.empty()) {
        return;
    }
    std::error_code ec;
    if (!std::filesystem::exists(meshes_dir_, ec) || ec) {
        return;
    }
    static const std::vector<std::string> kExts = {".ply", ".obj", ".stl", ".fbx", ".gltf", ".glb"};
    for (const auto& entry : std::filesystem::directory_iterator(meshes_dir_, ec)) {
        if (ec) {
            break;
        }
        if (!entry.is_regular_file()) {
            continue;
        }
        std::string ext = entry.path().extension().string();
        std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
        for (const auto& e : kExts) {
            if (ext == e) {
                mesh_files_.push_back(entry.path());
                break;
            }
        }
    }
    std::sort(mesh_files_.begin(), mesh_files_.end());

    // Pre-select the currently loaded mesh if known.
    if (mesh_layer_ && !mesh_layer_->getMeshPath().empty()) {
        std::filesystem::path cur(mesh_layer_->getMeshPath());
        for (int i = 0; i < static_cast<int>(mesh_files_.size()); ++i) {
            if (mesh_files_[static_cast<size_t>(i)] == cur) {
                selected_mesh_idx_ = i;
                break;
            }
        }
    }
}

void InteractionSettingsLayer::onAttach(vne::testbed::AppContext& /*ctx*/) {
    if (imgui_layer_) {
        imgui_layer_->setSettingsCallback([this]() { renderPanel(); });
        imgui_layer_->setViewportOverlayCallback([this](int idx) { handleViewportDrop(idx); });
    }
}

void InteractionSettingsLayer::onDetach() {
    if (imgui_layer_) {
        imgui_layer_->setSettingsCallback(nullptr);
        imgui_layer_->setViewportOverlayCallback(nullptr);
        imgui_layer_ = nullptr;
    }
    interaction_layer_ = nullptr;
    mesh_layer_ = nullptr;
}

void InteractionSettingsLayer::loadMesh(const std::filesystem::path& path) {
#ifdef VNE_TESTBED_VNEIO
    if (mesh_layer_) {
        mesh_layer_->reloadMesh(path.string());
    }
#else
    (void)path;
#endif
}

void InteractionSettingsLayer::handleViewportDrop(int /*viewport_index*/) {
    if (ImGui::BeginDragDropTarget()) {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("VNE_MESH_PATH")) {
            const char* path_str = static_cast<const char*>(payload->Data);
            std::filesystem::path dropped(path_str);
            loadMesh(dropped);
            // Update selection highlight.
            for (int i = 0; i < static_cast<int>(mesh_files_.size()); ++i) {
                if (mesh_files_[static_cast<size_t>(i)] == dropped) {
                    selected_mesh_idx_ = i;
                    break;
                }
            }
        }
        ImGui::EndDragDropTarget();
    }
}

void InteractionSettingsLayer::setSceneLayer(BaseSceneLayer* layer) {
    scene_layer_ = layer;
}

void InteractionSettingsLayer::renderPanel() {
    if (!interaction_layer_) {
        return;
    }
    auto& il = *interaction_layer_;

    // ---- Scene (grid, axes) ----
    if (scene_layer_ && ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show grid", &scene_layer_->show_grid_);
        ImGui::Checkbox("Show axes", &scene_layer_->show_axes_);
    }

    // ---- Camera (perspective / orthographic) ----
    renderCameraSettings();

    // ---- Manipulator ----
    renderManipulatorSettings();

    const auto cur_type = il.getManipulatorType();

    // ---- Zoom method (Orbit, Arcball, Fps, Fly, OrthoPanZoom) ----
    if (cur_type == vne::interaction::CameraManipulatorType::eOrbit
        || cur_type == vne::interaction::CameraManipulatorType::eArcball
        || cur_type == vne::interaction::CameraManipulatorType::eFps
        || cur_type == vne::interaction::CameraManipulatorType::eFly
        || cur_type == vne::interaction::CameraManipulatorType::eOrthoPanZoom) {
        if (ImGui::CollapsingHeader("Zoom Method", ImGuiTreeNodeFlags_DefaultOpen)) {
            using ZM = vne::interaction::ZoomMethod;
            const char* znames[] = {"DollyToCoi", "SceneScale", "ChangeFov"};
            const ZM zvals[] = {ZM::eDollyToCoi, ZM::eSceneScale, ZM::eChangeFov};
            if (ImGui::Combo("Method##zoom", &zoom_idx_, znames, 3)) {
                il.setZoomMethod(zvals[zoom_idx_]);
            }
            ImGui::Spacing();
            ImGui::TextDisabled("DollyToCoi: move along ray to pivot");
            ImGui::TextDisabled("SceneScale: virtual scene scale");
            ImGui::TextDisabled("ChangeFov:  widen/narrow FOV angle");
        }
    }

    // ---- View direction presets (Orbit / Arcball only) ----
    if (cur_type == vne::interaction::CameraManipulatorType::eOrbit
        || cur_type == vne::interaction::CameraManipulatorType::eArcball) {
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

    // ---- Mesh Browser ----
    renderMeshBrowser();

    // ---- Lighting ----
    renderLightingSettings();

    // ---- Mesh Transform ----
    renderMeshTransform();
}

void InteractionSettingsLayer::renderCameraSettings() {
    if (!scene_layer_ || !interaction_layer_) {
        return;
    }
    auto& sl = *scene_layer_;
    auto& il = *interaction_layer_;

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        int persp_idx = sl.use_perspective_ ? 0 : 1;
        const char* types[] = {"Perspective", "Orthographic"};
        if (ImGui::Combo("Type##cam", &persp_idx, types, 2)) {
            const bool use_persp = (persp_idx == 0);
            if (use_persp && !il.isManipulatorCompatibleWithCamera(true)) {
                ImGui::OpenPopup("ManipulatorIncompatible");
            } else {
                sl.syncCameraPositionTargetUp();
                sl.setUsePerspective(use_persp);
                if (!use_persp && !il.isManipulatorCompatibleWithCamera(false)) {
                    il.setManipulatorType(vne::interaction::CameraManipulatorType::eOrbit);
                }
                il.setCamerasFromScene();
            }
        }
        if (ImGui::BeginPopupModal("ManipulatorIncompatible", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("OrthoPanZoom requires Orthographic camera.");
            ImGui::Text("Switch to Perspective first, or change manipulator to Orbit/Arcball.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        bool proj_changed = false;
        if (sl.use_perspective_) {
            if (ImGui::TreeNodeEx("Perspective", ImGuiTreeNodeFlags_DefaultOpen)) {
                proj_changed |= ImGui::SliderFloat("FOV##persp", &sl.fov_, 10.f, 120.f, "%.0f deg");
                proj_changed |= ImGui::SliderFloat("Near##persp", &sl.near_plane_, 0.01f, 10.f, "%.3f");
                proj_changed |= ImGui::SliderFloat("Far##persp", &sl.far_plane_, 100.f, 5000.f, "%.0f");
                ImGui::TreePop();
            }
        } else {
            if (ImGui::TreeNodeEx("Orthographic", ImGuiTreeNodeFlags_DefaultOpen)) {
                proj_changed |= ImGui::SliderFloat("Half extent##ortho", &sl.ortho_half_, 0.5f, 50.f, "%.1f");
                proj_changed |= ImGui::SliderFloat("Near##ortho", &sl.ortho_near_, -500.f, 500.f);
                proj_changed |= ImGui::SliderFloat("Far##ortho", &sl.ortho_far_, -500.f, 500.f);
                ImGui::TreePop();
            }
        }

        if (proj_changed) {
            sl.rebuildCameras(sl.last_vp_w_, sl.last_vp_h_);
            il.setCamerasFromScene();
        }

        ImGui::Checkbox("Show view matrix", &show_view_matrix_);
        ImGui::Checkbox("Show projection matrix", &show_projection_matrix_);
        if (show_view_matrix_ && sl.getActiveCamera(0)) {
            const vne::math::Mat4f view = sl.getActiveCamera(0)->getViewMatrix();
            ImGui::Text("View matrix (column-major):");
            for (size_t row = 0; row < 4u; ++row) {
                ImGui::Text("%.4f  %.4f  %.4f  %.4f",
                            static_cast<double>(view[0][row]),
                            static_cast<double>(view[1][row]),
                            static_cast<double>(view[2][row]),
                            static_cast<double>(view[3][row]));
            }
        }
        if (show_projection_matrix_ && sl.getActiveCamera(0)) {
            const vne::math::Mat4f proj = sl.getActiveCamera(0)->getProjectionMatrix();
            ImGui::Text("Projection matrix (column-major):");
            for (size_t row = 0; row < 4u; ++row) {
                ImGui::Text("%.4f  %.4f  %.4f  %.4f",
                            static_cast<double>(proj[0][row]),
                            static_cast<double>(proj[1][row]),
                            static_cast<double>(proj[2][row]),
                            static_cast<double>(proj[3][row]));
            }
        }
    }
}

void InteractionSettingsLayer::renderManipulatorSettings() {
    if (!interaction_layer_ || !scene_layer_) {
        return;
    }
    auto& il = *interaction_layer_;
    using MT = vne::interaction::CameraManipulatorType;

    if (ImGui::CollapsingHeader("Manipulator", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* types[] = {"Orbit", "Arcball", "Fps", "Fly", "OrthoPanZoom", "Follow"};
        const MT values[] = {MT::eOrbit, MT::eArcball, MT::eFps, MT::eFly, MT::eOrthoPanZoom, MT::eFollow};
        int idx = 0;
        const MT cur = il.getManipulatorType();
        for (int i = 0; i < 6; ++i) {
            if (values[i] == cur) {
                idx = i;
                break;
            }
        }
        const bool ortho_only = (cur == MT::eOrthoPanZoom);
        const bool need_ortho = ortho_only && scene_layer_->use_perspective_;
        if (need_ortho) {
            ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "OrthoPanZoom requires Orthographic camera");
        }
        if (ImGui::Combo("Type##manip", &idx, types, 6)) {
            const MT new_type = values[idx];
            if (new_type == MT::eOrthoPanZoom && scene_layer_->use_perspective_) {
                ImGui::OpenPopup("OrthoPanZoomNeedsOrtho");
            } else {
                il.setManipulatorType(new_type);
                il.setCamerasFromScene();
            }
        }
        if (ImGui::BeginPopupModal("OrthoPanZoomNeedsOrtho", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("OrthoPanZoom works only with Orthographic camera.");
            ImGui::Text("Switch camera to Orthographic first.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        ImGui::Spacing();
        switch (cur) {
            case MT::eOrbit:
                ImGui::TextDisabled("LMB rotate  RMB pan  Scroll zoom");
                break;
            case MT::eArcball:
                ImGui::TextDisabled("LMB rotate  RMB pan  Scroll zoom (arcball)");
                break;
            case MT::eFps:
                ImGui::TextDisabled("RMB + WASD/QE move  Mouse look");
                break;
            case MT::eFly:
                ImGui::TextDisabled("RMB + WASD/QE move  Mouse look (fly)");
                break;
            case MT::eOrthoPanZoom:
                ImGui::TextDisabled("LMB/RMB pan  Scroll zoom (no rotate)");
                break;
            case MT::eFollow:
                ImGui::TextDisabled("Camera follows the target object");
                break;
        }

        // Per-manipulator settings
        auto* ctrl = il.getController();
        if (ctrl) {
            auto m = ctrl->getManipulator();
            if (m) {
                if (auto* orbit = dynamic_cast<vne::interaction::OrbitManipulator*>(m.get())) {
                    if (ImGui::TreeNodeEx("Orbit Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                        using RPM = vne::interaction::RotationPivotMode;
                        int pivot_idx = static_cast<int>(orbit->getPivotMode());
                        const char* pivot_names[] = {"COI (pan moves pivot)",
                                                     "View center (pan end updates COI)",
                                                     "Fixed world (pan translates eye+target)"};
                        if (ImGui::Combo("Rotation pivot##orb", &pivot_idx, pivot_names, 3))
                            orbit->setPivotMode(static_cast<RPM>(pivot_idx));
                        float rs = orbit->getRotationSpeed();
                        if (ImGui::SliderFloat("Rotation speed##orb", &rs, 0.1f, 5.f))
                            orbit->setRotationSpeed(rs);
                        float ps = orbit->getPanSpeed();
                        if (ImGui::SliderFloat("Pan speed##orb", &ps, 0.1f, 10.f))
                            orbit->setPanSpeed(ps);
                        float zs = orbit->getZoomSpeed();
                        if (ImGui::SliderFloat("Zoom speed##orb", &zs, 1.01f, 1.5f, "%.3f"))
                            orbit->setZoomSpeed(zs);
                        float fs = orbit->getFovZoomSpeed();
                        if (ImGui::SliderFloat("FOV zoom speed##orb", &fs, 1.01f, 1.2f, "%.3f"))
                            orbit->setFovZoomSpeed(fs);
                        float rd = orbit->getRotationDamping();
                        if (ImGui::SliderFloat("Rotation damping##orb", &rd, 0.f, 20.f))
                            orbit->setRotationDamping(rd);
                        float pd = orbit->getPanDamping();
                        if (ImGui::SliderFloat("Pan damping##orb", &pd, 0.f, 20.f))
                            orbit->setPanDamping(pd);
                        ImGui::TreePop();
                    }
                } else if (auto* arc = dynamic_cast<vne::interaction::ArcballManipulator*>(m.get())) {
                    if (ImGui::TreeNodeEx("Arcball Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                        using RPM = vne::interaction::RotationPivotMode;
                        int pivot_idx = static_cast<int>(arc->getPivotMode());
                        const char* pivot_names[] = {"COI (pan moves pivot)",
                                                     "View center (pan end updates COI)",
                                                     "Fixed world (pan translates eye+target)"};
                        if (ImGui::Combo("Rotation pivot##arc", &pivot_idx, pivot_names, 3))
                            arc->setPivotMode(static_cast<RPM>(pivot_idx));
                        float rs = arc->getRotationSpeed();
                        if (ImGui::SliderFloat("Rotation speed##arc", &rs, 0.1f, 5.f))
                            arc->setRotationSpeed(rs);
                        float ps = arc->getPanSpeed();
                        if (ImGui::SliderFloat("Pan speed##arc", &ps, 0.1f, 10.f))
                            arc->setPanSpeed(ps);
                        float zs = arc->getZoomSpeed();
                        if (ImGui::SliderFloat("Zoom speed##arc", &zs, 1.01f, 1.5f, "%.3f"))
                            arc->setZoomSpeed(zs);
                        float rd = arc->getRotationDamping();
                        if (ImGui::SliderFloat("Rotation damping##arc", &rd, 0.f, 20.f))
                            arc->setRotationDamping(rd);
                        float pd = arc->getPanDamping();
                        if (ImGui::SliderFloat("Pan damping##arc", &pd, 0.f, 20.f))
                            arc->setPanDamping(pd);
                        ImGui::TreePop();
                    }
                } else if (auto* ortho = dynamic_cast<vne::interaction::OrthoPanZoomManipulator*>(m.get())) {
                    if (ImGui::TreeNodeEx("OrthoPanZoom Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                        float zs = ortho->getZoomSpeed();
                        if (ImGui::SliderFloat("Zoom speed##ortho", &zs, 1.01f, 1.5f, "%.3f"))
                            ortho->setZoomSpeed(zs);
                        float pd = ortho->getPanDamping();
                        if (ImGui::SliderFloat("Pan damping##ortho", &pd, 0.f, 20.f))
                            ortho->setPanDamping(pd);
                        ImGui::TreePop();
                    }
                } else if (auto* fps = dynamic_cast<vne::interaction::FpsManipulator*>(m.get())) {
                    if (ImGui::TreeNodeEx("Fps Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                        float ms = fps->getMoveSpeed();
                        if (ImGui::SliderFloat("Move speed##fps", &ms, 0.5f, 20.f))
                            fps->setMoveSpeed(ms);
                        float sens = fps->getMouseSensitivity();
                        if (ImGui::SliderFloat("Mouse sensitivity##fps", &sens, 0.05f, 0.5f))
                            fps->setMouseSensitivity(sens);
                        float zs = fps->getZoomSpeed();
                        if (ImGui::SliderFloat("Zoom speed##fps", &zs, 0.1f, 5.f))
                            fps->setZoomSpeed(zs);
                        float sprint = fps->getSprintMultiplier();
                        if (ImGui::SliderFloat("Sprint multiplier##fps", &sprint, 1.f, 5.f))
                            fps->setSprintMultiplier(sprint);
                        float slow = fps->getSlowMultiplier();
                        if (ImGui::SliderFloat("Slow multiplier##fps", &slow, 0.1f, 1.f))
                            fps->setSlowMultiplier(slow);
                        ImGui::TreePop();
                    }
                } else if (auto* fly = dynamic_cast<vne::interaction::FlyManipulator*>(m.get())) {
                    if (ImGui::TreeNodeEx("Fly Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                        float ms = fly->getMoveSpeed();
                        if (ImGui::SliderFloat("Move speed##fly", &ms, 0.5f, 20.f))
                            fly->setMoveSpeed(ms);
                        float sens = fly->getMouseSensitivity();
                        if (ImGui::SliderFloat("Mouse sensitivity##fly", &sens, 0.05f, 0.5f))
                            fly->setMouseSensitivity(sens);
                        float zs = fly->getZoomSpeed();
                        if (ImGui::SliderFloat("Zoom speed##fly", &zs, 0.1f, 5.f))
                            fly->setZoomSpeed(zs);
                        float sprint = fly->getSprintMultiplier();
                        if (ImGui::SliderFloat("Sprint multiplier##fly", &sprint, 1.f, 5.f))
                            fly->setSprintMultiplier(sprint);
                        float slow = fly->getSlowMultiplier();
                        if (ImGui::SliderFloat("Slow multiplier##fly", &slow, 0.1f, 1.f))
                            fly->setSlowMultiplier(slow);
                        ImGui::TreePop();
                    }
                }
            }
        }
    }
}

void InteractionSettingsLayer::renderMeshBrowser() {
    if (ImGui::CollapsingHeader("Mesh Browser", ImGuiTreeNodeFlags_DefaultOpen)) {
#ifndef VNE_TESTBED_VNEIO
        ImGui::TextDisabled("(mesh loading requires vneio)");
        return;
#else
        if (meshes_dir_.empty()) {
            ImGui::TextDisabled("No meshes directory set.");
            return;
        }

        // Currently loaded mesh.
        if (mesh_layer_ && !mesh_layer_->getMeshPath().empty()) {
            std::filesystem::path cur(mesh_layer_->getMeshPath());
            ImGui::TextDisabled("Loaded: %s", cur.filename().string().c_str());
        } else {
            ImGui::TextDisabled("No mesh loaded.");
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Click to load  |  Drag to viewport");
        ImGui::Separator();

        if (mesh_files_.empty()) {
            ImGui::TextDisabled("No mesh files found in:");
            ImGui::TextWrapped("%s", meshes_dir_.c_str());
            return;
        }

        // Scrollable list of mesh files.
        const float list_height = std::min(static_cast<float>(mesh_files_.size()) * 22.0f + 8.0f, 200.0f);
        ImGui::BeginChild("MeshFileList", ImVec2(0.0f, list_height), true);
        for (int i = 0; i < static_cast<int>(mesh_files_.size()); ++i) {
            const auto& p = mesh_files_[static_cast<size_t>(i)];
            const std::string fname = p.filename().string();
            const bool selected = (i == selected_mesh_idx_);

            ImGui::PushID(i);
            if (ImGui::Selectable(fname.c_str(), selected, ImGuiSelectableFlags_AllowDoubleClick)) {
                selected_mesh_idx_ = i;
                loadMesh(p);
            }

            // Drag source: drag any item onto the viewport to load it.
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                const std::string full = p.string();
                ImGui::SetDragDropPayload("VNE_MESH_PATH", full.c_str(), full.size() + 1u, ImGuiCond_Once);
                ImGui::Text("Load: %s", fname.c_str());
                ImGui::EndDragDropSource();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::BeginTooltip();
                ImGui::TextUnformatted(p.string().c_str());
                ImGui::EndTooltip();
            }

            ImGui::PopID();
        }
        ImGui::EndChild();
#endif  // VNE_TESTBED_VNEIO
    }
}

void InteractionSettingsLayer::renderLightingSettings() {
#ifdef VNE_TESTBED_VNEIO
    if (!mesh_layer_) {
        return;
    }
    // Access the real vnescene light objects stored in MeshLayer.
    auto amb = mesh_layer_->getAmbientLight();
    auto dir = mesh_layer_->getDirectionalLight();

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
        // ---- Ambient light (vne::scene::AmbientLight) ----
        if (amb && ImGui::TreeNodeEx("Ambient", ImGuiTreeNodeFlags_DefaultOpen)) {
            vne::math::Vec3f ac = amb->getColor();
            float ambCol[3] = {ac.x(), ac.y(), ac.z()};
            if (ImGui::ColorEdit3("Color##amb", ambCol)) {
                amb->setColor({ambCol[0], ambCol[1], ambCol[2]});
            }
            float ambI = amb->getIntensity();
            if (ImGui::SliderFloat("Intensity##amb", &ambI, 0.0f, 2.0f)) {
                amb->setIntensity(ambI);
            }
            ImGui::TreePop();
        }

        // ---- Directional light (vne::scene::DirectionalLight) ----
        if (dir && ImGui::TreeNodeEx("Directional Light", ImGuiTreeNodeFlags_DefaultOpen)) {
            bool enabled = dir->isEnabled();
            if (ImGui::Checkbox("Enabled##dir", &enabled)) {
                dir->setEnabled(enabled);
            }
            if (enabled) {
                vne::math::Vec3f dc = dir->getColor();
                float dirCol[3] = {dc.x(), dc.y(), dc.z()};
                if (ImGui::ColorEdit3("Color##dir", dirCol)) {
                    dir->setColor({dirCol[0], dirCol[1], dirCol[2]});
                }
                float dirI = dir->getIntensity();
                if (ImGui::SliderFloat("Intensity##dir", &dirI, 0.0f, 5.0f)) {
                    dir->setIntensity(dirI);
                }
                vne::math::Vec3f dd = dir->getDirection();
                float dirDir[3] = {dd.x(), dd.y(), dd.z()};
                if (ImGui::SliderFloat3("Direction##dir", dirDir, -1.0f, 1.0f)) {
                    dir->setDirection({dirDir[0], dirDir[1], dirDir[2]});
                }
                ImGui::Spacing();
                ImGui::TextDisabled("Presets:");
                ImGui::SameLine();
                if (ImGui::SmallButton("Top")) {
                    dir->setDirection({0.0f, -1.0f, 0.0f});
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("Front")) {
                    dir->setDirection({0.0f, 0.0f, -1.0f});
                }
                ImGui::SameLine();
                if (ImGui::SmallButton("3/4")) {
                    dir->setDirection({-0.4f, -1.0f, -0.6f});
                }
            }
            ImGui::TreePop();
        }

        // ---- Point lights (if any) ----
        int ptIdx = 0;
        for (const auto& light : mesh_layer_->getSceneState().getLights()) {
            if (!light || light->getLightType() != vne::scene::LightType::ePoint) {
                continue;
            }
            const std::string label = std::string("Point Light ") + std::to_string(ptIdx);
            if (ImGui::TreeNodeEx(label.c_str())) {
                bool ptEn = light->isEnabled();
                const std::string enLabel = std::string("Enabled##pt") + std::to_string(ptIdx);
                if (ImGui::Checkbox(enLabel.c_str(), &ptEn)) {
                    light->setEnabled(ptEn);
                }
                vne::math::Vec3f pc = light->getColor();
                float ptCol[3] = {pc.x(), pc.y(), pc.z()};
                const std::string colLabel = std::string("Color##pt") + std::to_string(ptIdx);
                if (ImGui::ColorEdit3(colLabel.c_str(), ptCol)) {
                    light->setColor({ptCol[0], ptCol[1], ptCol[2]});
                }
                float ptI = light->getIntensity();
                const std::string intLabel = std::string("Intensity##pt") + std::to_string(ptIdx);
                if (ImGui::SliderFloat(intLabel.c_str(), &ptI, 0.0f, 5.0f)) {
                    light->setIntensity(ptI);
                }
                ImGui::TreePop();
            }
            ++ptIdx;
        }
    }
#endif
}

void InteractionSettingsLayer::renderMeshTransform() {
#ifdef VNE_TESTBED_VNEIO
    if (!mesh_layer_) {
        return;
    }
    if (ImGui::CollapsingHeader("Mesh Transform")) {
        // Sync scale from MeshLayer so it resets properly when a new mesh is loaded.
        float scale = mesh_layer_->getUniformScale();
        if (ImGui::SliderFloat("Scale##mesh", &scale, 0.001f, 10.0f, "%.3f")) {
            mesh_layer_->setUniformScale(scale);
        }

        // Auto-fit: scale so the mesh AABB fits inside a ~1.5-unit radius sphere.
        if (ImGui::Button("Auto-fit")) {
            const float* mn = mesh_layer_->getAabbMin();
            const float* mx = mesh_layer_->getAabbMax();
            float ext = 0.0f;
            for (int k = 0; k < 3; ++k) {
                const float half = (mx[k] - mn[k]) * 0.5f;
                if (half > ext) {
                    ext = half;
                }
            }
            static constexpr float kFitRadius = 1.5f;
            if (ext > 1e-6f) {
                mesh_layer_->setUniformScale(kFitRadius / ext);
            }
        }
        ImGui::SameLine();
        if (ImGui::Button("Reset scale")) {
            mesh_layer_->setUniformScale(1.0f);
        }

        // AABB display (mesh-local space)
        const float* mn = mesh_layer_->getAabbMin();
        const float* mx = mesh_layer_->getAabbMax();
        ImGui::TextDisabled("AABB min: %.2f %.2f %.2f",
                            static_cast<double>(mn[0]),
                            static_cast<double>(mn[1]),
                            static_cast<double>(mn[2]));
        ImGui::TextDisabled("AABB max: %.2f %.2f %.2f",
                            static_cast<double>(mx[0]),
                            static_cast<double>(mx[1]),
                            static_cast<double>(mx[2]));
    }
#endif
}

#endif  // VNE_TESTBED_IMGUI

// ---------------------------------------------------------------------------
// Demo registration
// ---------------------------------------------------------------------------

void RegisterTestInteractionDemo(vne::testbed::Application& app) {
    auto* scene = new BaseSceneLayer("TestInteractionBaseSceneLayer");
    app.getLayerStack().pushLayer(std::unique_ptr<BaseSceneLayer>(scene), app.getAppContext());

    auto* interaction = new InteractionTestLayer();
    interaction->setSceneLayer(scene);
    app.getLayerStack().pushLayer(std::unique_ptr<InteractionTestLayer>(interaction), app.getAppContext());

#ifdef VNE_TESTBED_VNEIO
    // Default mesh: box.ply so there's something to see on startup.
    auto* mesh_layer = new vne::testbed::MeshLayer();
    mesh_layer->setMeshPath(vne::samples::common::getTestdataPath("resources/meshes/box.ply"));
    mesh_layer->setCameraProvider([scene](int i) { return scene->getCamera(i); });
    mesh_layer->setRenderSortKey(10);  // after grid (0) so mesh draws on top
    app.getLayerStack().pushLayer(std::unique_ptr<vne::testbed::MeshLayer>(mesh_layer), app.getAppContext());
#endif

#ifdef VNE_TESTBED_IMGUI
    auto* settings = new InteractionSettingsLayer();
    auto* imgui = dynamic_cast<vne::testbed::ImGuiLayer*>(app.getLayerStack().findLayerByName("ImGuiLayer"));
    if (imgui) {
        settings->setImGuiLayer(imgui);
        interaction->setImGuiLayer(imgui);
    }
    settings->setInteractionLayer(interaction);
    settings->setSceneLayer(scene);

#ifdef VNE_TESTBED_VNEIO
    // Connect mesh layer to the settings panel for the mesh browser.
    settings->setMeshLayer(mesh_layer);
    // Point the browser at the testdata meshes directory.
    settings->setMeshesDir(vne::samples::common::getTestdataPath("resources/meshes"));
#endif

    app.getLayerStack().pushLayer(std::unique_ptr<InteractionSettingsLayer>(settings), app.getAppContext());
#endif
}

VNETESTBED_REGISTER_DEMO("test_interaction", RegisterTestInteractionDemo)

}  // namespace vne::samples::test_interaction

#endif  // VNE_TESTBED_INTERACTION
