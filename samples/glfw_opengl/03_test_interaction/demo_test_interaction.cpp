/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 *
 * ----------------------------------------------------------------------
 */

#ifdef VNE_TESTBED_INTERACTION

#include "demo_test_interaction.h"

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/demo_factory.h"

#include "vertexnova/events/event.h"
#include "vertexnova/events/input/input.h"
#include "vertexnova/events/mouse_event.h"
#include "vertexnova/events/types.h"
#include "vertexnova/events/window_event.h"

#include "vertexnova/interaction/orbital_camera_behavior.h"
#include "vertexnova/interaction/ortho_2d_behavior.h"
#include "vertexnova/interaction/free_look_behavior.h"
#include "vertexnova/interaction/follow_behavior.h"

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

#include <filesystem>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace vne::samples {

namespace {

constexpr double kFixedDt = 0.016;
constexpr int kRenderSortKey = 999;
constexpr float kDefaultViewportW = 1280.0f;
constexpr float kDefaultViewportH = 720.0f;

const vne::math::Vec3f kDefaultCameraPosition{4.0f, 3.0f, 6.0f};
const vne::math::Vec3f kDefaultTargetPosition{0.0f, 0.0f, 0.0f};

ControllerVariant makeController(ControllerKind kind, vne::interaction::NavigateMode nav_mode) {
    switch (kind) {
        case ControllerKind::eInspectOrbit: {
            vne::interaction::Inspect3DController c;
            c.setRotationMode(vne::interaction::OrbitRotationMode::eOrbit);
            return c;
        }
        case ControllerKind::eInspectTrackball: {
            vne::interaction::Inspect3DController c;
            c.setRotationMode(vne::interaction::OrbitRotationMode::eTrackball);
            return c;
        }
        case ControllerKind::eNavigation: {
            vne::interaction::Navigation3DController c;
            c.setMode(nav_mode);
            return c;
        }
        case ControllerKind::eOrtho: {
            return vne::interaction::Ortho2DController{};
        }
        case ControllerKind::eFollow: {
            return vne::interaction::FollowController{};
        }
    }
    return vne::interaction::Inspect3DController{};
}

}  // namespace

// ---------------------------------------------------------------------------
// InteractionTestLayer implementation
// ---------------------------------------------------------------------------

InteractionTestLayer::InteractionTestLayer()
    : vne::testbed::ILayer("InteractionTestLayer") {
    for (size_t i = 0; i < static_cast<size_t>(kMaxViewports); ++i) {
        controllers_[i] = makeController(ControllerKind::eInspectTrackball, navigation_mode_);
    }
}

void InteractionTestLayer::setCamera(std::shared_ptr<vne::scene::ICamera> camera) {
    camera_ = std::move(camera);
    dispatchSetCamera(camera_);
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
    for (size_t i = 0; i < static_cast<size_t>(kMaxViewports); ++i) {
        auto viewport_camera = (i < cams.size()) ? cams[i] : (cams.empty() ? nullptr : cams[0]);
        std::visit(
            [&viewport_camera](auto& c) {
                if (viewport_camera) {
                    c.setCamera(viewport_camera);
                }
            },
            controllers_[i]);
    }
}

bool InteractionTestLayer::isManipulatorCompatibleWithCamera(bool use_perspective) const {
    if (current_kind_ == ControllerKind::eOrtho) {
        return !use_perspective;
    }
    return true;
}

#ifdef VNE_TESTBED_IMGUI
void InteractionTestLayer::setImGuiLayer(vne::testbed::ImGuiLayer* layer) {
    imgui_layer_ = layer;
}

void InteractionTestLayer::setMeshLayer(vne::testbed::MeshLayer* layer) noexcept {
    mesh_layer_ = layer;
}
#endif

void InteractionTestLayer::dispatchViewportSize(float w, float h) {
    for (auto& v : controllers_) {
        std::visit([w, h](auto& c) { c.onResize(w, h); }, v);
    }
}

void InteractionTestLayer::dispatchEvent(const vne::events::Event& event, int viewport_index) {
    if (viewport_index < 0 || viewport_index >= kMaxViewports) {
        viewport_index = 0;
    }
    auto& v = controllers_[static_cast<size_t>(viewport_index)];
    std::visit(
        [&event](auto& c) {
            const bool needs_cursor_position = (event.type() == vne::events::EventType::eMouseScrolled
                                                || event.type() == vne::events::EventType::eMouseButtonPressed
                                                || event.type() == vne::events::EventType::eMouseButtonReleased
                                                || event.type() == vne::events::EventType::eMouseButtonDoubleClicked);

            // Scroll events don't carry cursor position; some mouse button events may also omit it.
            // Controllers depend on their last seen mouse position to compute zoom-to-cursor.
            if (needs_cursor_position) {
                const auto [mx, my] = vne::events::Input::mousePosition();
                const vne::events::MouseMovedEvent mm(mx, my);
                if constexpr (std::is_same_v<std::decay_t<decltype(c)>, vne::interaction::Ortho2DController>
                              || std::is_same_v<std::decay_t<decltype(c)>, vne::interaction::FollowController>) {
                    c.onEvent(mm);
                } else {
                    c.onEvent(mm, 0.0);
                }
            }
            if constexpr (std::is_same_v<std::decay_t<decltype(c)>, vne::interaction::Ortho2DController>
                          || std::is_same_v<std::decay_t<decltype(c)>, vne::interaction::FollowController>) {
                c.onEvent(event);
            } else {
                c.onEvent(event, kFixedDt);
            }
        },
        v);
}

void InteractionTestLayer::dispatchUpdate(double dt) {
    for (auto& v : controllers_) {
        std::visit([dt](auto& c) { c.onUpdate(dt); }, v);
    }
}

void InteractionTestLayer::dispatchSetCamera(std::shared_ptr<vne::scene::ICamera> camera) {
    if (!camera) {
        return;
    }
    for (auto& v : controllers_) {
        std::visit([&camera](auto& c) { c.setCamera(camera); }, v);
    }
}

void InteractionTestLayer::dispatchReset() {
    for (auto& v : controllers_) {
        std::visit([](auto& c) { c.reset(); }, v);
    }
}

void InteractionTestLayer::onAttach(vne::testbed::AppContext& app_context) {
    const auto vpw = app_context.window ? static_cast<float>(app_context.window->getWidth()) : kDefaultViewportW;
    const auto vph = app_context.window ? static_cast<float>(app_context.window->getHeight()) : kDefaultViewportH;
    dispatchViewportSize(vpw, vph);
}

void InteractionTestLayer::onDetach() {}

void InteractionTestLayer::onUpdate(float dt) {
#ifdef VNE_TESTBED_IMGUI
    if (imgui_layer_) {
        for (int i = 0; i < kMaxViewports; ++i) {
            float min_x = 0.0f, min_y = 0.0f, max_x = 0.0f, max_y = 0.0f;
            if (imgui_layer_->getViewportRect(i, min_x, min_y, max_x, max_y)) {
                const float vp_w = max_x - min_x;
                const float vp_h = max_y - min_y;
                auto& v = controllers_[static_cast<size_t>(i)];
                std::visit([vp_w, vp_h](auto& c) { c.onResize(vp_w, vp_h); }, v);
            }
        }
    }
#endif
    dispatchUpdate(static_cast<double>(dt));
}

void InteractionTestLayer::onEvent(const vne::events::Event& event) {
    using ET = vne::events::EventType;
    if (event.type() == ET::eWindowResize) {
        const auto& e = static_cast<const vne::events::WindowResizeEvent&>(event);
        dispatchViewportSize(static_cast<float>(e.width()), static_cast<float>(e.height()));
        return;
    }
    auto check_x = static_cast<float>(last_x_);
    auto check_y = static_cast<float>(last_y_);
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
        if (idx < 0) {
            const auto t = event.type();
            if (t == ET::eMouseScrolled || t == ET::eMouseMoved || t == ET::eMouseButtonPressed
                || t == ET::eMouseButtonReleased) {
                if (t == ET::eMouseMoved) {
                    const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
                    last_x_ = e.x();
                    last_y_ = e.y();
                }
                return;
            }
        }
    }
#endif
#ifdef VNE_TESTBED_IMGUI
    if (imgui_layer_) {
        float vp_min_x = 0.0f, vp_min_y = 0.0f, vp_max_x = 0.0f, vp_max_y = 0.0f;
        if (imgui_layer_->getViewportRect(viewport_index, vp_min_x, vp_min_y, vp_max_x, vp_max_y)) {
            const float vp_w = vp_max_x - vp_min_x;
            const float vp_h = vp_max_y - vp_min_y;
            auto& v = controllers_[static_cast<size_t>(viewport_index)];
            std::visit([vp_w, vp_h](auto& c) { c.onResize(vp_w, vp_h); }, v);
        }
    }
#endif
    dispatchEvent(event, viewport_index);
    if (event.type() == ET::eMouseMoved) {
        const auto& e = static_cast<const vne::events::MouseMovedEvent&>(event);
        last_x_ = e.x();
        last_y_ = e.y();
    }
}

void InteractionTestLayer::setControllerKind(ControllerKind kind) {
    current_kind_ = kind;
    const auto vpw = kDefaultViewportW;
    const auto vph = kDefaultViewportH;
    for (size_t i = 0; i < static_cast<size_t>(kMaxViewports); ++i) {
        controllers_[i] = makeController(kind, navigation_mode_);
        std::visit(
            [this, vpw, vph](auto& c) {
                c.onResize(vpw, vph);
                if (camera_) {
                    c.setCamera(camera_);
                }
            },
            controllers_[i]);
    }
}

void InteractionTestLayer::setZoomMethod(vne::interaction::ZoomMethod method) {
    for (auto& v : controllers_) {
        std::visit(
            [method](auto& c) {
                if constexpr (std::is_same_v<std::decay_t<decltype(c)>, vne::interaction::Inspect3DController>) {
                    c.orbitalCameraBehavior().setZoomMethod(method);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(c)>,
                                                    vne::interaction::Navigation3DController>) {
                    c.freeLookBehavior().setZoomMethod(method);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(c)>, vne::interaction::Ortho2DController>) {
                    c.ortho2DBehavior().setZoomMethod(method);
                } else if constexpr (std::is_same_v<std::decay_t<decltype(c)>, vne::interaction::FollowController>) {
                    c.followBehavior().setZoomMethod(method);
                }
            },
            v);
    }
}

void InteractionTestLayer::setViewDirection(vne::interaction::ViewDirection dir) {
    for (auto& v : controllers_) {
        std::visit(
            [dir](auto& c) {
                if constexpr (std::is_same_v<std::decay_t<decltype(c)>, vne::interaction::Inspect3DController>) {
                    c.orbitalCameraBehavior().setViewDirection(dir);
                }
            },
            v);
    }
}

void InteractionTestLayer::resetCamera() {
    if (!camera_) {
        return;
    }
    camera_->setPosition(kDefaultCameraPosition);
    camera_->setTarget(kDefaultTargetPosition);
    camera_->updateMatrices();
    dispatchReset();
}

void InteractionTestLayer::setMoveSpeed(float speed) {
    auto* nav = getNavController(0);
    if (nav) {
        nav->setMoveSpeed(speed);
    }
}

void InteractionTestLayer::setMouseSensitivity(float sensitivity) {
    auto* nav = getNavController(0);
    if (nav) {
        nav->setMouseSensitivity(sensitivity);
    }
}

void InteractionTestLayer::setSprintMultiplier(float mult) {
    auto* nav = getNavController(0);
    if (nav) {
        nav->setSprintMultiplier(mult);
    }
}

void InteractionTestLayer::setSlowMultiplier(float mult) {
    auto* nav = getNavController(0);
    if (nav) {
        nav->setSlowMultiplier(mult);
    }
}

void InteractionTestLayer::setRotationPivotMode(vne::interaction::OrbitPivotMode mode) {
    for (auto& v : controllers_) {
        std::visit(
            [mode](auto& c) {
                if constexpr (std::is_same_v<std::decay_t<decltype(c)>, vne::interaction::Inspect3DController>) {
                    c.orbitalCameraBehavior().setPivotMode(mode);
                }
            },
            v);
    }
}

void InteractionTestLayer::setRotationEnabled(bool enabled) {
    auto* insp = getInspectController(0);
    if (insp) {
        insp->setRotationEnabled(enabled);
    }
    auto* ortho = getOrthoController(0);
    if (ortho) {
        ortho->setRotationEnabled(enabled);
    }
}

void InteractionTestLayer::setPanEnabled(bool enabled) {
    auto* insp = getInspectController(0);
    if (insp) {
        insp->setPanEnabled(enabled);
    }
    auto* ortho = getOrthoController(0);
    if (ortho) {
        ortho->setPanEnabled(enabled);
    }
}

void InteractionTestLayer::setZoomEnabled(bool enabled) {
    auto* insp = getInspectController(0);
    if (insp) {
        insp->setZoomEnabled(enabled);
    }
    auto* ortho = getOrthoController(0);
    if (ortho) {
        ortho->setZoomEnabled(enabled);
    }
}

void InteractionTestLayer::setNavigationMode(vne::interaction::NavigateMode mode) {
    navigation_mode_ = mode;
    for (auto& v : controllers_) {
        if (auto* nav = std::get_if<vne::interaction::Navigation3DController>(&v)) {
            nav->setMode(mode);
        }
    }
}

vne::math::Vec3f InteractionTestLayer::cameraPosition() const {
    return camera_ ? camera_->getPosition() : vne::math::Vec3f{};
}

vne::math::Vec3f InteractionTestLayer::cameraTarget() const {
    return camera_ ? camera_->getTarget() : vne::math::Vec3f{};
}

vne::interaction::Inspect3DController* InteractionTestLayer::getInspectController(int index) noexcept {
    if (index < 0 || index >= kMaxViewports
        || (current_kind_ != ControllerKind::eInspectOrbit && current_kind_ != ControllerKind::eInspectTrackball)) {
        return nullptr;
    }
    return std::get_if<vne::interaction::Inspect3DController>(&controllers_[static_cast<size_t>(index)]);
}

vne::interaction::Navigation3DController* InteractionTestLayer::getNavController(int index) noexcept {
    if (index < 0 || index >= kMaxViewports || current_kind_ != ControllerKind::eNavigation) {
        return nullptr;
    }
    return std::get_if<vne::interaction::Navigation3DController>(&controllers_[static_cast<size_t>(index)]);
}

vne::interaction::Ortho2DController* InteractionTestLayer::getOrthoController(int index) noexcept {
    if (index < 0 || index >= kMaxViewports || current_kind_ != ControllerKind::eOrtho) {
        return nullptr;
    }
    return std::get_if<vne::interaction::Ortho2DController>(&controllers_[static_cast<size_t>(index)]);
}

vne::interaction::FollowController* InteractionTestLayer::getFollowController(int index) noexcept {
    if (index < 0 || index >= kMaxViewports || current_kind_ != ControllerKind::eFollow) {
        return nullptr;
    }
    return std::get_if<vne::interaction::FollowController>(&controllers_[static_cast<size_t>(index)]);
}

// ---------------------------------------------------------------------------
// InteractionSettingsLayer implementation
// ---------------------------------------------------------------------------
#ifdef VNE_TESTBED_IMGUI

InteractionSettingsLayer::InteractionSettingsLayer()
    : vne::testbed::ILayer("InteractionSettingsLayer") {
    setRenderSortKey(kRenderSortKey);
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

void InteractionSettingsLayer::onAttach(vne::testbed::AppContext& /*app_context*/) {
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
    auto& ui = uiSettings();

    if (scene_layer_ && ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::Checkbox("Show grid", &scene_layer_->uiSettings().show_grid);
        ImGui::Checkbox("Show axes", &scene_layer_->uiSettings().show_axes);
    }

    renderCameraSettings();
    renderManipulatorSettings();

    const ControllerKind cur = il.getControllerKind();
    const bool show_zoom =
        (cur == ControllerKind::eInspectOrbit || cur == ControllerKind::eInspectTrackball
         || cur == ControllerKind::eNavigation || cur == ControllerKind::eOrtho || cur == ControllerKind::eFollow);
    if (show_zoom) {
        if (ImGui::CollapsingHeader("Zoom Method", ImGuiTreeNodeFlags_DefaultOpen)) {
            using ZM = vne::interaction::ZoomMethod;
            const char* znames[] = {"SceneScale", "ChangeFov", "DollyToCoi"};
            const ZM zvals[] = {ZM::eSceneScale, ZM::eChangeFov, ZM::eDollyToCoi};
            if (ImGui::Combo("Method##zoom", &ui.zoom_idx, znames, 3)) {
                il.setZoomMethod(zvals[ui.zoom_idx]);
            }
            ImGui::Spacing();
            ImGui::TextDisabled("SceneScale: XY scene scale in view (virtual zoom)");
            ImGui::TextDisabled("ChangeFov: widen/narrow FOV (perspective) or ortho extents");
            ImGui::TextDisabled("DollyToCoi: move along view ray toward pivot");
        }
    }

    if (cur == ControllerKind::eInspectOrbit || cur == ControllerKind::eInspectTrackball) {
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

    renderMeshBrowser();
    renderLightingSettings();
    renderMeshTransform();
}

void InteractionSettingsLayer::renderCameraSettings() {
    if (!scene_layer_ || !interaction_layer_) {
        return;
    }
    auto& sl = *scene_layer_;
    auto& il = *interaction_layer_;
    auto& ui = uiSettings();

    if (ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen)) {
        int persp_idx = sl.uiSettings().use_perspective ? 0 : 1;
        const char* types[] = {"Perspective", "Orthographic"};
        if (ImGui::Combo("Type##cam", &persp_idx, types, 2)) {
            const bool use_persp = (persp_idx == 0);
            if (use_persp && !il.isManipulatorCompatibleWithCamera(true)) {
                ImGui::OpenPopup("ManipulatorIncompatible");
            } else {
                sl.syncCameraPositionTargetUp();
                sl.setUsePerspective(use_persp);
                if (!use_persp && !il.isManipulatorCompatibleWithCamera(false)) {
                    il.setControllerKind(ControllerKind::eInspectTrackball);
                }
                il.setCamerasFromScene();
            }
        }
        if (ImGui::BeginPopupModal("ManipulatorIncompatible", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Ortho controller requires Orthographic camera.");
            ImGui::Text("Switch to Perspective first, or change controller to Inspect.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        bool proj_changed = false;
        if (sl.uiSettings().use_perspective) {
            if (ImGui::TreeNodeEx("Perspective", ImGuiTreeNodeFlags_DefaultOpen)) {
                proj_changed |= ImGui::SliderFloat("FOV##persp", &sl.uiSettings().fov, 10.f, 120.f, "%.0f deg");
                proj_changed |= ImGui::SliderFloat("Near##persp", &sl.uiSettings().near_plane, 0.01f, 10.f, "%.3f");
                proj_changed |= ImGui::SliderFloat("Far##persp", &sl.uiSettings().far_plane, 100.f, 5000.f, "%.0f");
                ImGui::TreePop();
            }
        } else {
            if (ImGui::TreeNodeEx("Orthographic", ImGuiTreeNodeFlags_DefaultOpen)) {
                proj_changed |=
                    ImGui::SliderFloat("Half extent##ortho", &sl.uiSettings().ortho_half, 0.5f, 50.f, "%.1f");
                proj_changed |= ImGui::SliderFloat("Near##ortho", &sl.uiSettings().ortho_near, -500.f, 500.f);
                proj_changed |= ImGui::SliderFloat("Far##ortho", &sl.uiSettings().ortho_far, -500.f, 500.f);
                ImGui::TreePop();
            }
        }

        if (proj_changed) {
            sl.rebuildCameras(sl.uiSettings().last_viewport_w, sl.uiSettings().last_viewport_h);
            il.setCamerasFromScene();
        }

        ImGui::Checkbox("Show view matrix", &ui.show_view_matrix);
        ImGui::Checkbox("Show projection matrix", &ui.show_projection_matrix);
        if (ui.show_view_matrix && sl.getActiveCameraPtr(0)) {
            const vne::math::Mat4f view = sl.getActiveCameraPtr(0)->getViewMatrix();
            ImGui::Text("View matrix (column-major):");
            for (size_t row = 0; row < 4u; ++row) {
                ImGui::Text("%.4f  %.4f  %.4f  %.4f",
                            static_cast<double>(view[0][row]),
                            static_cast<double>(view[1][row]),
                            static_cast<double>(view[2][row]),
                            static_cast<double>(view[3][row]));
            }
        }
        if (ui.show_projection_matrix && sl.getActiveCameraPtr(0)) {
            const vne::math::Mat4f proj = sl.getActiveCameraPtr(0)->getProjectionMatrix();
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
    auto& ui = uiSettings();
    const ControllerKind cur = il.getControllerKind();

    if (ImGui::CollapsingHeader("Controller", ImGuiTreeNodeFlags_DefaultOpen)) {
        const char* types[] = {"Inspect (Orbit)", "Inspect (Trackball)", "Navigation", "Ortho", "Follow"};
        const ControllerKind values[] = {ControllerKind::eInspectOrbit,
                                         ControllerKind::eInspectTrackball,
                                         ControllerKind::eNavigation,
                                         ControllerKind::eOrtho,
                                         ControllerKind::eFollow};
        int idx = 0;
        for (int i = 0; i < 5; ++i) {
            if (values[i] == cur) {
                idx = i;
                break;
            }
        }
        const bool ortho_only = (cur == ControllerKind::eOrtho);
        const bool need_ortho = ortho_only && scene_layer_->uiSettings().use_perspective;
        if (need_ortho) {
            ImGui::TextColored(ImVec4(1.f, 0.4f, 0.4f, 1.f), "Ortho requires Orthographic camera");
        }
        if (ImGui::Combo("Type##ctrl", &idx, types, 5)) {
            const ControllerKind new_kind = values[idx];
            if (new_kind == ControllerKind::eOrtho && scene_layer_->uiSettings().use_perspective) {
                ImGui::OpenPopup("OrthoNeedsOrtho");
            } else {
                il.setControllerKind(new_kind);
                il.setCamerasFromScene();
            }
        }
        if (ImGui::BeginPopupModal("OrthoNeedsOrtho", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
            ImGui::Text("Ortho works only with Orthographic camera.");
            ImGui::Text("Switch camera to Orthographic first.");
            if (ImGui::Button("OK")) {
                ImGui::CloseCurrentPopup();
            }
            ImGui::EndPopup();
        }

        if (cur == ControllerKind::eNavigation) {
            if (auto* nav = il.getNavController()) {
                const int mode_from_controller = static_cast<int>(nav->getMode());
                if (mode_from_controller >= 0 && mode_from_controller <= 2) {
                    ui.nav_mode_idx = mode_from_controller;
                }
            }
            const char* modes[] = {"Fps", "Fly", "Game"};
            const vne::interaction::NavigateMode modes_val[] = {vne::interaction::NavigateMode::eFps,
                                                                vne::interaction::NavigateMode::eFly,
                                                                vne::interaction::NavigateMode::eGame};
            if (ImGui::Combo("Navigation mode##nav_sub", &ui.nav_mode_idx, modes, 3)) {
                il.setNavigationMode(modes_val[ui.nav_mode_idx]);
            }
        }

        ImGui::Spacing();
        switch (cur) {
            case ControllerKind::eInspectOrbit:
                ImGui::TextDisabled("LMB rotate  RMB pan  Scroll zoom");
                break;
            case ControllerKind::eInspectTrackball:
                ImGui::TextDisabled("LMB rotate  RMB pan  Scroll zoom (trackball)");
                break;
            case ControllerKind::eNavigation:
                ImGui::TextDisabled("RMB + WASD/QE move  Mouse look (Fps/Fly/Game)");
                break;
            case ControllerKind::eOrtho:
                ImGui::TextDisabled("LMB/RMB pan  Scroll zoom (no rotate)");
                break;
            case ControllerKind::eFollow:
                ImGui::TextDisabled("Camera follows the target");
                break;
        }

        // Per-controller settings
        if (auto* insp = il.getInspectController()) {
            auto& orb = insp->orbitalCameraBehavior();
            if (ImGui::TreeNodeEx("Inspect Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                if (cur == ControllerKind::eInspectTrackball) {
                    using TPM = vne::interaction::TrackballBehavior::ProjectionMode;
                    int proj_idx = (orb.getTrackballProjectionMode() == TPM::eHyperbolic) ? 0 : 1;
                    const char* proj_names[] = {"Hyperbolic", "Rim"};
                    if (ImGui::Combo("Trackball projection##insp", &proj_idx, proj_names, 2)) {
                        orb.setTrackballProjectionMode(proj_idx == 0 ? TPM::eHyperbolic : TPM::eRim);
                    }
                    ImGui::TextDisabled("Hyperbolic: cap + continuation; Rim: hemisphere + equatorial rim");
                }
                using OPM = vne::interaction::OrbitPivotMode;
                int pivot_idx = static_cast<int>(orb.getPivotMode());
                const char* pivot_names[] = {"COI (pan moves pivot)",
                                             "View center (pan end updates COI)",
                                             "Fixed world (pan translates eye+target)"};
                if (ImGui::Combo("Rotation pivot##insp", &pivot_idx, pivot_names, 3)) {
                    orb.setPivotMode(static_cast<OPM>(pivot_idx));
                }
                if (ImGui::Checkbox("Rotation enabled##insp", &ui.rotation_enabled_insp)) {
                    insp->setRotationEnabled(ui.rotation_enabled_insp);
                }
                if (ImGui::Checkbox("Pan enabled##insp", &ui.pan_enabled_insp)) {
                    insp->setPanEnabled(ui.pan_enabled_insp);
                }
                if (ImGui::Checkbox("Zoom enabled##insp", &ui.zoom_enabled_insp)) {
                    insp->setZoomEnabled(ui.zoom_enabled_insp);
                }
                float rs = orb.getRotationSpeed();
                if (ImGui::SliderFloat("Rotation speed##insp", &rs, 0.1f, 5.f)) {
                    orb.setRotationSpeed(rs);
                }
                float ps = orb.getPanSpeed();
                if (ImGui::SliderFloat("Pan speed##insp", &ps, 0.1f, 10.f)) {
                    orb.setPanSpeed(ps);
                }
                float zs = orb.getZoomSpeed();
                if (ImGui::SliderFloat("Zoom speed##insp", &zs, 1.01f, 1.5f, "%.3f")) {
                    orb.setZoomSpeed(zs);
                }
                float fs = orb.getFovZoomSpeed();
                if (ImGui::SliderFloat("FOV zoom speed##insp", &fs, 1.01f, 1.2f, "%.3f")) {
                    orb.setFovZoomSpeed(fs);
                }
                float rd = orb.getRotationDamping();
                if (ImGui::SliderFloat("Rotation damping##insp", &rd, 0.f, 20.f)) {
                    orb.setRotationDamping(rd);
                }
                float pd = orb.getPanDamping();
                if (ImGui::SliderFloat("Pan damping##insp", &pd, 0.f, 20.f)) {
                    orb.setPanDamping(pd);
                }
                ImGui::TreePop();
            }
        } else if (auto* nav = il.getNavController()) {
            if (ImGui::TreeNodeEx("Navigation Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                ui.move_speed = nav->getMoveSpeed();
                if (ImGui::SliderFloat("Move speed##nav", &ui.move_speed, 0.5f, 20.f)) {
                    nav->setMoveSpeed(ui.move_speed);
                }
                ui.mouse_sensitivity = nav->getMouseSensitivity();
                if (ImGui::SliderFloat("Mouse sensitivity##nav", &ui.mouse_sensitivity, 0.05f, 0.5f)) {
                    nav->setMouseSensitivity(ui.mouse_sensitivity);
                }
                ui.sprint_mult = nav->getSprintMultiplier();
                if (ImGui::SliderFloat("Sprint multiplier##nav", &ui.sprint_mult, 1.f, 5.f)) {
                    nav->setSprintMultiplier(ui.sprint_mult);
                }
                ui.slow_mult = nav->getSlowMultiplier();
                if (ImGui::SliderFloat("Slow multiplier##nav", &ui.slow_mult, 0.1f, 1.f)) {
                    nav->setSlowMultiplier(ui.slow_mult);
                }
                ImGui::TreePop();
            }
        } else if (auto* ortho = il.getOrthoController()) {
            auto& opz = ortho->ortho2DBehavior();
            if (ImGui::TreeNodeEx("Ortho Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                bool rot_en = ortho->isRotationEnabled();
                if (ImGui::Checkbox("Rotation enabled##ortho", &rot_en)) {
                    ortho->setRotationEnabled(rot_en);
                }
                float zs = opz.getZoomSpeed();
                if (ImGui::SliderFloat("Zoom speed##ortho", &zs, 1.01f, 1.5f, "%.3f")) {
                    opz.setZoomSpeed(zs);
                }
                float pd = opz.getPanDamping();
                if (ImGui::SliderFloat("Pan damping##ortho", &pd, 0.f, 20.f)) {
                    opz.setPanDamping(pd);
                }
                ImGui::TreePop();
            }
        } else if (auto* follow = il.getFollowController()) {
            if (ImGui::TreeNodeEx("Follow Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
                vne::math::Vec3f off = follow->getOffset();
                float o[3] = {off.x(), off.y(), off.z()};
                if (ImGui::SliderFloat3("Offset##follow", o, -20.f, 20.f)) {
                    follow->setOffset({o[0], o[1], o[2]});
                }
                float lag = follow->getLag();
                if (ImGui::SliderFloat("Lag##follow", &lag, 0.01f, 0.5f, "%.3f")) {
                    follow->setLag(lag);
                }
                ImGui::TreePop();
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
#endif
    }
}

void InteractionSettingsLayer::renderLightingSettings() {
#ifdef VNE_TESTBED_VNEIO
    if (!mesh_layer_) {
        return;
    }
    auto amb = mesh_layer_->getAmbientLight();
    auto dir = mesh_layer_->getDirectionalLight();

    if (ImGui::CollapsingHeader("Lighting", ImGuiTreeNodeFlags_DefaultOpen)) {
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
        float scale = mesh_layer_->getUniformScale();
        if (ImGui::SliderFloat("Scale##mesh", &scale, 0.001f, 10.0f, "%.3f")) {
            mesh_layer_->setUniformScale(scale);
        }

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

void registerTestInteractionDemo(vne::testbed::Application& app) {
    auto* scene = new BaseSceneLayer("TestInteractionBaseSceneLayer");
    app.getLayerStack().pushLayer(std::unique_ptr<BaseSceneLayer>(scene), app.getAppContext());

    auto* interaction = new InteractionTestLayer();
    interaction->setSceneLayer(scene);
    app.getLayerStack().pushLayer(std::unique_ptr<InteractionTestLayer>(interaction), app.getAppContext());

#ifdef VNE_TESTBED_VNEIO
    auto* mesh_layer = new vne::testbed::MeshLayer();
    mesh_layer->setMeshPath(vne::samples::common::getTestdataPath("resources/meshes/box.ply"));
    mesh_layer->setCameraProvider([scene](int i) { return scene->getActiveCamera(i); });
    mesh_layer->setRenderSortKey(10);
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
    settings->setMeshLayer(mesh_layer);
    settings->setMeshesDir(vne::samples::common::getTestdataPath("resources/meshes"));
    interaction->setMeshLayer(mesh_layer);
#endif

    app.getLayerStack().pushLayer(std::unique_ptr<InteractionSettingsLayer>(settings), app.getAppContext());
#endif
}

VNETESTBED_REGISTER_DEMO("test_interaction", registerTestInteractionDemo)

}  // namespace vne::samples

#endif  // VNE_TESTBED_INTERACTION
