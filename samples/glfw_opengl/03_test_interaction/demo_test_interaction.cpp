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

#include "vertexnova/interaction/arcball_manipulator.h"
#include "vertexnova/interaction/fly_manipulator.h"
#include "vertexnova/interaction/fps_manipulator.h"
#include "vertexnova/interaction/orbit_manipulator.h"

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"
#include <imgui.h>
#endif

#ifdef VNE_TESTBED_HAVE_VNEIO
#include "vertexnova/testbed/utils/mesh_layer.h"
#endif

#include <algorithm>

#include "../common/base_scene_layer.h"
#include "../common/path_utils.h"

#include <cmath>
#include <filesystem>
#include <memory>
#include <string>
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

void InteractionTestLayer::setCamera(std::shared_ptr<vne::scene::PerspectiveCamera> cam) {
    camera_ = std::move(cam);
    if (!controllers_.empty() && controllers_[0]) {
        controllers_[0]->setCamera(camera_);
    }
}

void InteractionTestLayer::setSceneLayer(const BaseSceneLayer* scene) {
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
        if (idx < 0) {
            return;
        }
        viewport_index = idx;
    }
#endif
    auto* controller = (viewport_index >= 0 && viewport_index < static_cast<int>(controllers_.size()))
                           ? controllers_[static_cast<size_t>(viewport_index)].get()
                           : controllers_[0].get();
    if (!controller) {
        return;
    }
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
            const auto [mx, my] = vne::events::Input::mousePosition();
            last_x_ = static_cast<double>(mx);
            last_y_ = static_cast<double>(my);
            controller->handleMouseScroll(static_cast<float>(e.xOffset()),
                                          static_cast<float>(e.yOffset()),
                                          static_cast<float>(last_x_),
                                          static_cast<float>(last_y_),
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

void InteractionTestLayer::setZoomMethod(vne::interaction::ZoomMethod method) {
    for (auto& ctrl : controllers_) {
        if (!ctrl) {
            continue;
        }
        auto m = ctrl->getManipulator();
        if (!m) {
            continue;
        }
        if (auto* orbit = dynamic_cast<vne::interaction::OrbitManipulator*>(m.get())) {
            orbit->setZoomMethod(method);
        } else if (auto* arc = dynamic_cast<vne::interaction::ArcballManipulator*>(m.get())) {
            arc->setZoomMethod(method);
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
#ifdef VNE_TESTBED_HAVE_VNEIO
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

void InteractionSettingsLayer::renderPanel() {
    if (!interaction_layer_) {
        return;
    }
    auto& il = *interaction_layer_;

    // ---- Manipulator type ----
    if (ImGui::CollapsingHeader("Manipulator", ImGuiTreeNodeFlags_DefaultOpen)) {
        using MT = vne::interaction::CameraManipulatorType;
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
        if (ImGui::Combo("Type", &idx, types, 6)) {
            il.setManipulatorType(values[idx]);
        }

        ImGui::Spacing();
        switch (il.getManipulatorType()) {
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
    }

    const auto cur_type = il.getManipulatorType();

    // ---- Zoom method (Orbit / Arcball only) ----
    if (cur_type == vne::interaction::CameraManipulatorType::eOrbit
        || cur_type == vne::interaction::CameraManipulatorType::eArcball) {
        if (ImGui::CollapsingHeader("Zoom Method", ImGuiTreeNodeFlags_DefaultOpen)) {
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

    // ---- Fps / Fly controls ----
    if (cur_type == vne::interaction::CameraManipulatorType::eFps
        || cur_type == vne::interaction::CameraManipulatorType::eFly) {
        if (ImGui::CollapsingHeader("Fps/Fly Settings", ImGuiTreeNodeFlags_DefaultOpen)) {
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

    // ---- Mesh Browser ----
    renderMeshBrowser();

    // ---- Lighting ----
    renderLightingSettings();

    // ---- Mesh Transform ----
    renderMeshTransform();
}

void InteractionSettingsLayer::renderMeshBrowser() {
    if (ImGui::CollapsingHeader("Mesh Browser", ImGuiTreeNodeFlags_DefaultOpen)) {
#ifndef VNE_TESTBED_HAVE_VNEIO
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
                ImGui::SetDragDropPayload("VNE_MESH_PATH",
                                          full.c_str(),
                                          full.size() + 1u,
                                          ImGuiCond_Once);
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
#endif  // VNE_TESTBED_HAVE_VNEIO
    }
}

void InteractionSettingsLayer::renderLightingSettings() {
#ifdef VNE_TESTBED_HAVE_VNEIO
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
                if (ImGui::SmallButton("Top"))   { dir->setDirection({0.0f, -1.0f,  0.0f}); }
                ImGui::SameLine();
                if (ImGui::SmallButton("Front")) { dir->setDirection({0.0f,  0.0f, -1.0f}); }
                ImGui::SameLine();
                if (ImGui::SmallButton("3/4"))   { dir->setDirection({-0.4f, -1.0f, -0.6f}); }
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
#ifdef VNE_TESTBED_HAVE_VNEIO
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
                            static_cast<double>(mn[0]), static_cast<double>(mn[1]), static_cast<double>(mn[2]));
        ImGui::TextDisabled("AABB max: %.2f %.2f %.2f",
                            static_cast<double>(mx[0]), static_cast<double>(mx[1]), static_cast<double>(mx[2]));
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

#ifdef VNE_TESTBED_HAVE_VNEIO
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

#ifdef VNE_TESTBED_HAVE_VNEIO
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
