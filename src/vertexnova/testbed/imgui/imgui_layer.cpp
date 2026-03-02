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

#include "vertexnova/testbed/imgui/imgui_layer.h"

#if defined(VNE_TESTBED_EVENTS)
#include "vertexnova/testbed/imgui/imgui_event_listener.h"
#include "vertexnova/events/event_manager.h"
#include "vertexnova/events/types.h"
#endif

#if defined(VNE_TESTBED_OPENGL)
#include "vertexnova/testbed/window/glfw_window.h"
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif
#include <GLFW/glfw3.h>

#include <imgui.h>
#include <imgui_internal.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace vne {
namespace testbed {

namespace {
// VertexNova color palette (https://learnvertexnova.com/docs/docs/misc/color-palette)
// Primary palette for decks & UI
constexpr float kBgR = 28.0f / 255.0f;  // #1C1C1E Background
constexpr float kBgG = 28.0f / 255.0f;
constexpr float kBgB = 30.0f / 255.0f;
constexpr float kPanelR = 44.0f / 255.0f;  // #2C2C2E Primary panel
constexpr float kPanelG = 44.0f / 255.0f;
constexpr float kPanelB = 46.0f / 255.0f;
constexpr float kMidR = 58.0f / 255.0f;  // #3A3A3C Mid panel
constexpr float kMidG = 58.0f / 255.0f;
constexpr float kMidB = 60.0f / 255.0f;
constexpr float kBorderR = 72.0f / 255.0f;  // #48484A Border
constexpr float kBorderG = 72.0f / 255.0f;
constexpr float kBorderB = 74.0f / 255.0f;
constexpr float kOrangeR = 232.0f / 255.0f;  // #E8622A Orange (primary)
constexpr float kOrangeG = 98.0f / 255.0f;
constexpr float kOrangeB = 42.0f / 255.0f;
constexpr float kOrangeLightR = 242.0f / 255.0f;  // #F28C5E Orange light
constexpr float kOrangeLightG = 140.0f / 255.0f;
constexpr float kOrangeLightB = 94.0f / 255.0f;
constexpr float kOrangeDarkR = 122.0f / 255.0f;  // #7A3315 Orange dark
constexpr float kOrangeDarkG = 51.0f / 255.0f;
constexpr float kOrangeDarkB = 21.0f / 255.0f;
constexpr float kTextR = 235.0f / 255.0f;  // #EBEBF0 Near-white text
constexpr float kTextG = 235.0f / 255.0f;
constexpr float kTextB = 240.0f / 255.0f;
constexpr float kMutedR = 174.0f / 255.0f;  // #AEAEB2 Muted text
constexpr float kMutedG = 174.0f / 255.0f;
constexpr float kMutedB = 178.0f / 255.0f;

// Viewport/screen clear color — mid gray (#4A4A4C), matches OpenGLRenderAdapter
constexpr float kClearR = 74.0f / 255.0f;
constexpr float kClearG = 74.0f / 255.0f;
constexpr float kClearB = 76.0f / 255.0f;

// Fixed spacing so layout does not shift when Font scale slider changes (up/down).
constexpr float kItemSpacingX = 8.0f;
constexpr float kItemSpacingY = 4.0f;
constexpr float kFramePaddingX = 6.0f;
constexpr float kFramePaddingY = 3.0f;

void applyVertexNovaStyle() {
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* c = style.Colors;

    style.ItemSpacing = ImVec2(kItemSpacingX, kItemSpacingY);
    style.FramePadding = ImVec2(kFramePaddingX, kFramePaddingY);

    c[ImGuiCol_Text] = ImVec4(kTextR, kTextG, kTextB, 1.0f);
    c[ImGuiCol_TextDisabled] = ImVec4(kMutedR, kMutedG, kMutedB, 1.0f);
    c[ImGuiCol_WindowBg] = ImVec4(kPanelR, kPanelG, kPanelB, 0.94f);
    c[ImGuiCol_ChildBg] = ImVec4(kBgR, kBgG, kBgB, 0.0f);
    c[ImGuiCol_PopupBg] = ImVec4(kPanelR, kPanelG, kPanelB, 0.94f);
    c[ImGuiCol_Border] = ImVec4(kBorderR, kBorderG, kBorderB, 0.5f);
    c[ImGuiCol_FrameBg] = ImVec4(kMidR, kMidG, kMidB, 0.54f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.4f);
    c[ImGuiCol_FrameBgActive] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.67f);
    c[ImGuiCol_TitleBg] = ImVec4(kBgR, kBgG, kBgB, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(kOrangeDarkR, kOrangeDarkG, kOrangeDarkB, 1.0f);
    c[ImGuiCol_TitleBgCollapsed] = ImVec4(kBgR, kBgG, kBgB, 0.51f);
    c[ImGuiCol_MenuBarBg] = ImVec4(kPanelR, kPanelG, kPanelB, 1.0f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(kBgR, kBgG, kBgB, 0.53f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(kMidR, kMidG, kMidB, 1.0f);
    c[ImGuiCol_ScrollbarGrabHovered] = ImVec4(kBorderR, kBorderG, kBorderB, 1.0f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 1.0f);
    c[ImGuiCol_CheckMark] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 1.0f);
    c[ImGuiCol_SliderGrab] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 1.0f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(kOrangeLightR, kOrangeLightG, kOrangeLightB, 1.0f);
    c[ImGuiCol_Button] = ImVec4(kOrangeDarkR, kOrangeDarkG, kOrangeDarkB, 0.4f);
    c[ImGuiCol_ButtonHovered] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(kOrangeLightR, kOrangeLightG, kOrangeLightB, 1.0f);
    c[ImGuiCol_Header] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.31f);
    c[ImGuiCol_HeaderHovered] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.8f);
    c[ImGuiCol_HeaderActive] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 1.0f);
    c[ImGuiCol_Separator] = c[ImGuiCol_Border];
    c[ImGuiCol_SeparatorHovered] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.78f);
    c[ImGuiCol_SeparatorActive] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 1.0f);
    c[ImGuiCol_ResizeGrip] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.2f);
    c[ImGuiCol_ResizeGripHovered] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.67f);
    c[ImGuiCol_ResizeGripActive] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.95f);
    c[ImGuiCol_Tab] = ImVec4(kOrangeDarkR, kOrangeDarkG, kOrangeDarkB, 0.8f);
    c[ImGuiCol_TabHovered] = c[ImGuiCol_HeaderHovered];
    c[ImGuiCol_TabSelected] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.6f);
    c[ImGuiCol_TabSelectedOverline] = c[ImGuiCol_HeaderActive];
    // Unfocused tab bar: lighter gray (Muted #AEAEB2) instead of default blue
    c[ImGuiCol_TabDimmed] = ImVec4(kMutedR, kMutedG, kMutedB, 0.8f);
    c[ImGuiCol_TabDimmedSelected] = ImVec4(kOrangeDarkR, kOrangeDarkG, kOrangeDarkB, 0.6f);
    c[ImGuiCol_TabDimmedSelectedOverline] = ImVec4(kOrangeDarkR, kOrangeDarkG, kOrangeDarkB, 0.8f);
    c[ImGuiCol_DockingPreview] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.7f);
    c[ImGuiCol_DockingEmptyBg] = ImVec4(kBgR, kBgG, kBgB, 1.0f);
    c[ImGuiCol_PlotLines] = ImVec4(kMutedR, kMutedG, kMutedB, 1.0f);
    c[ImGuiCol_PlotLinesHovered] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 1.0f);
    c[ImGuiCol_PlotHistogram] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 1.0f);
    c[ImGuiCol_PlotHistogramHovered] = ImVec4(kOrangeLightR, kOrangeLightG, kOrangeLightB, 1.0f);
    c[ImGuiCol_TableHeaderBg] = ImVec4(kMidR, kMidG, kMidB, 1.0f);
    c[ImGuiCol_TableBorderStrong] = ImVec4(kBorderR, kBorderG, kBorderB, 1.0f);
    c[ImGuiCol_TableBorderLight] = ImVec4(kMidR, kMidG, kMidB, 1.0f);
    c[ImGuiCol_TextSelectedBg] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 0.35f);
    c[ImGuiCol_TextLink] = c[ImGuiCol_HeaderActive];
    c[ImGuiCol_NavCursor] = ImVec4(kOrangeR, kOrangeG, kOrangeB, 1.0f);
}

}  // namespace

ImGuiLayer::ImGuiLayer()
    : ILayer("ImGuiLayer") {
    setRenderSortKey(1000);  // Render last (on top)
}

ImGuiLayer::~ImGuiLayer() = default;

void ImGuiLayer::onAttach(AppContext& ctx) {
    app_ctx_ = &ctx;
    tryInitFromContext();
}

void ImGuiLayer::tryInitFromContext() {
    if (initialized_ || !app_ctx_ || !app_ctx_->window || !app_ctx_->window->getNativeHandle()) {
        return;
    }

    GLFWwindow* window = static_cast<GLFWwindow*>(app_ctx_->window->getNativeHandle());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();

    // Use dedicated ini file for layout persistence (saved/restored automatically)
    io.IniFilename = "vnetestbed_imgui.ini";

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();
    applyVertexNovaStyle();

    ImGuiStyle& style = ImGui::GetStyle();
    style.FontSizeBase = 17.0f;  // Slightly larger default than ImGui's 13
    style.FontScaleMain = font_scale_;
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

#if defined(VNE_TESTBED_OPENGLES)
    const char* glsl_version = "#version 300 es";
#else
    const char* glsl_version = "#version 410 core";
#endif

    // install_callbacks=false: ImGui gets input from ImGuiEventListener (EventManager) instead of GLFW.
    // Matches vertexnova samples/core/imgui pattern for unified event handling.
    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplOpenGL3_Init(glsl_version);

    // Set initial display size (listener will update on WindowResizeEvent)
    int w = 0, h = 0;
    glfwGetFramebufferSize(window, &w, &h);
    io.DisplaySize = ImVec2(static_cast<float>(w), static_cast<float>(h));

    initialized_ = true;

#if defined(VNE_TESTBED_EVENTS)
    // Register ImGui event listener for manual input forwarding (like application event listeners).
    event_listener_ = std::make_shared<ImGuiEventListener>(this);
    auto& mgr = vne::events::EventManager::instance();
    using ET = vne::events::EventType;
    mgr.registerListener(ET::eKeyPressed, event_listener_);
    mgr.registerListener(ET::eKeyReleased, event_listener_);
    mgr.registerListener(ET::eKeyTyped, event_listener_);
    mgr.registerListener(ET::eMouseButtonPressed, event_listener_);
    mgr.registerListener(ET::eMouseButtonReleased, event_listener_);
    mgr.registerListener(ET::eMouseMoved, event_listener_);
    mgr.registerListener(ET::eMouseScrolled, event_listener_);
    mgr.registerListener(ET::eWindowResize, event_listener_);
#endif
}

void ImGuiLayer::onDetach() {
#if defined(VNE_TESTBED_EVENTS)
    if (event_listener_) {
        auto& mgr = vne::events::EventManager::instance();
        using ET = vne::events::EventType;
        mgr.unregisterListener(ET::eKeyPressed, event_listener_.get());
        mgr.unregisterListener(ET::eKeyReleased, event_listener_.get());
        mgr.unregisterListener(ET::eKeyTyped, event_listener_.get());
        mgr.unregisterListener(ET::eMouseButtonPressed, event_listener_.get());
        mgr.unregisterListener(ET::eMouseButtonReleased, event_listener_.get());
        mgr.unregisterListener(ET::eMouseMoved, event_listener_.get());
        mgr.unregisterListener(ET::eMouseScrolled, event_listener_.get());
        mgr.unregisterListener(ET::eWindowResize, event_listener_.get());
        event_listener_.reset();
    }
#endif
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    scene_fbo_.reset();
    scene_fbo_width_ = 0;
    scene_fbo_height_ = 0;
    viewport_fbos_.clear();
    viewport_fbo_count_ = 0;
    viewport_fbo_width_ = 0;
    viewport_fbo_height_ = 0;
#endif
    if (initialized_) {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        initialized_ = false;
    }
    app_ctx_ = nullptr;
}

void ImGuiLayer::onBeginRender(const RenderContext& ctx) {
    if (!initialized_ || !app_ctx_) {
        return;
    }
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    const int w = ctx.frame_info.width;
    const int h = ctx.frame_info.height;
    if (w <= 0 || h <= 0) {
        return;
    }

    const int viewport_count = getViewportCount();
    if (viewport_count > 1 && app_ctx_->renderSceneForViewport) {
        // Multi-viewport: render each viewport with its own FBO
        ensureViewportFbos(viewport_count, w, h);
        if (static_cast<int>(viewport_fbos_.size()) >= viewport_count) {
            RenderContext vp_ctx = ctx;
            for (int i = 0; i < viewport_count; ++i) {
                if (viewport_fbos_[static_cast<size_t>(i)] && viewport_fbos_[static_cast<size_t>(i)]->isValid()) {
                    viewport_fbos_[static_cast<size_t>(i)]->bind();
                    glClearColor(kClearR, kClearG, kClearB, 1.0f);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glViewport(0, 0, w, h);
                    vp_ctx.active_viewport_index = i;
                    app_ctx_->renderSceneForViewport(vp_ctx);
                }
            }
            if (!viewport_fbos_.empty() && viewport_fbos_[0] && viewport_fbos_[0]->isValid()) {
                viewport_fbos_[0]->unbind();
            }
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            app_ctx_->scene_rendered_by_imgui = true;
        }
    } else {
        // Single viewport: use shared scene FBO
        ensureSceneFbo(w, h);
        if (scene_fbo_ && scene_fbo_->isValid()) {
            scene_fbo_->bind();
            glClearColor(kClearR, kClearG, kClearB, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glViewport(0, 0, w, h);
        }
    }
#endif
}

void ImGuiLayer::onGuiBegin(const RenderContext& ctx) {
    tryInitFromContext();  // Deferred init if window wasn't ready at onAttach
    // Update FPS rolling average (frame time in seconds)
    if (ctx.frame_info.dt > 0.0f) {
        if (fps_buf_filled_ == kFpsAverageFrames) {
            fps_dt_sum_ -= fps_dt_buf_[fps_buf_index_];
        } else {
            ++fps_buf_filled_;
        }
        fps_dt_buf_[fps_buf_index_] = ctx.frame_info.dt;
        fps_dt_sum_ += ctx.frame_info.dt;
        fps_buf_index_ = (fps_buf_index_ + 1) % kFpsAverageFrames;
    }
    if (!initialized_) {
        return;
    }
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    const int viewport_count = getViewportCount();
    if (viewport_count <= 1 && scene_fbo_ && scene_fbo_->isValid()) {
        scene_fbo_->unbind();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    if (app_ctx_ && app_ctx_->window) {
        glViewport(0, 0, app_ctx_->window->getWidth(), app_ctx_->window->getHeight());
    }
#endif
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void ImGuiLayer::onGuiRender(const RenderContext& ctx) {
    (void)ctx;
    if (!initialized_) {
        return;
    }

    ImGuiViewport* viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->WorkPos);
    ImGui::SetNextWindowSize(viewport->WorkSize);
    ImGui::SetNextWindowViewport(viewport->ID);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar
                             | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove
                             | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

    if (ImGui::Begin("DockSpaceWindow", nullptr, flags)) {
        ImGui::PopStyleVar(3);

        // Use stable ImGui ID for dock layout persistence (saved to imgui.ini)
        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        // Rebuild when: (1) no saved layout yet, (2) viewport count changed, (3) panel width changed,
        // (4) window size changed — so Settings stays fixed and Viewport absorbs resize.
        ImGuiDockNode* node = ImGui::DockBuilderGetNode(dockspace_id);
        ImGuiViewport* vp = ImGui::GetMainViewport();
        const ImVec2 current_size = vp->WorkSize;
        bool has_saved_layout = (node && !node->IsEmpty());
        bool size_changed = (current_size.x != last_dock_size_.x || current_size.y != last_dock_size_.y);
        bool layout_changed = (last_dock_layout_ != viewport_layout_) || dock_layout_dirty_ || size_changed;

        if (!has_saved_layout || layout_changed) {
            setupDockLayout(dockspace_id, current_size);
            last_dock_layout_ = viewport_layout_;
            last_dock_size_ = current_size;
            dock_layout_dirty_ = false;
        }

        renderSettingsPanel(ctx);
        renderViewportWindows(ctx);
    } else {
        ImGui::PopStyleVar(3);
    }
    ImGui::End();
}

void ImGuiLayer::onGuiEnd(const RenderContext& ctx) {
    (void)ctx;
    if (!initialized_) {
        return;
    }
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    ImGuiIO& io = ImGui::GetIO();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        GLFWwindow* backup = glfwGetCurrentContext();
        ImGui::UpdatePlatformWindows();
        ImGui::RenderPlatformWindowsDefault();
        glfwMakeContextCurrent(backup);
    }
}

int ImGuiLayer::getViewportCount() const {
    switch (viewport_layout_) {
        case ViewportLayout::eOne:
            return 1;
        case ViewportLayout::eTwo:
            return 2;
        case ViewportLayout::eThree:
            return 3;
        case ViewportLayout::eFour:
            return 4;
    }
    return 1;
}

void ImGuiLayer::setupDockLayout(ImGuiID dockspace_id, const ImVec2& size) {
    // Ensure the dock builder node exists before manipulating it.
    if (ImGui::DockBuilderGetNode(dockspace_id) == nullptr) {
        ImGui::DockBuilderRemoveNode(dockspace_id);
        ImGui::DockBuilderAddNode(dockspace_id, ImGuiDockNodeFlags_DockSpace);
    } else {
        ImGui::DockBuilderRemoveNodeChildNodes(dockspace_id);
    }
    ImGui::DockBuilderSetNodeSize(dockspace_id, size);

    // Settings panel: fixed pixel width so it does not grow with the window.
    // Viewport area absorbs all remaining space to the right.
    const float settings_w = settings_panel_width_;
    const float total_w = (size.x > 0.0f) ? size.x : 1.0f;
    const float ratio_right = (total_w - settings_w) / total_w;  // fraction kept on right

    ImGuiID id_right{};
    ImGuiID id_settings{};
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, ratio_right, &id_right, &id_settings);

    ImGui::DockBuilderDockWindow("Settings", id_settings);

    ImGuiID id_viewport = id_right;
    switch (viewport_layout_) {
        case ViewportLayout::eOne:
            ImGui::DockBuilderDockWindow("Viewport", id_viewport);
            break;
        case ViewportLayout::eTwo: {
            ImGuiID id_v1{};
            ImGuiID id_v2{};
            ImGui::DockBuilderSplitNode(id_viewport, ImGuiDir_Right, 0.5f, &id_v2, &id_v1);
            ImGui::DockBuilderDockWindow("Viewport 1", id_v1);
            ImGui::DockBuilderDockWindow("Viewport 2", id_v2);
            break;
        }
        case ViewportLayout::eFour: {
            // Top two and bottom two: split by Y, then each half by X
            ImGuiID id_top{};
            ImGuiID id_bottom{};
            ImGui::DockBuilderSplitNode(id_viewport, ImGuiDir_Down, 0.5f, &id_bottom, &id_top);
            ImGuiID id_top_left{};
            ImGuiID id_top_right{};
            ImGui::DockBuilderSplitNode(id_top, ImGuiDir_Right, 0.5f, &id_top_right, &id_top_left);
            ImGuiID id_bottom_left{};
            ImGuiID id_bottom_right{};
            ImGui::DockBuilderSplitNode(id_bottom, ImGuiDir_Right, 0.5f, &id_bottom_right, &id_bottom_left);
            ImGui::DockBuilderDockWindow("Viewport 1", id_top_left);
            ImGui::DockBuilderDockWindow("Viewport 2", id_top_right);
            ImGui::DockBuilderDockWindow("Viewport 3", id_bottom_left);
            ImGui::DockBuilderDockWindow("Viewport 4", id_bottom_right);
            break;
        }
        case ViewportLayout::eThree: {
            // Left: two equal size stacked; right: one full height (same height as left)
            ImGuiID id_left{};
            ImGuiID id_right_col{};
            ImGui::DockBuilderSplitNode(id_viewport,
                                        ImGuiDir_Left,
                                        0.5f,
                                        &id_left,
                                        &id_right_col);  // 50% left, 50% right
            ImGuiID id_left_top{};
            ImGuiID id_left_bottom{};
            ImGui::DockBuilderSplitNode(id_left,
                                        ImGuiDir_Down,
                                        0.5f,
                                        &id_left_bottom,
                                        &id_left_top);  // top 50%, bottom 50%
            ImGui::DockBuilderDockWindow("Viewport 1", id_left_top);
            ImGui::DockBuilderDockWindow("Viewport 2", id_left_bottom);
            ImGui::DockBuilderDockWindow("Viewport 3", id_right_col);  // full height on right
            break;
        }
    }

    ImGui::DockBuilderFinish(dockspace_id);
}

void ImGuiLayer::renderSettingsPanel(const RenderContext& ctx) {
    (void)ctx;
    // NoScrollbar on the outer window — we manage scrolling ourselves below.
    if (!ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::End();
        return;
    }

    // ---- Fixed header (always visible, not scrolled) -------------------------

    const char* layout_items[] = {"1 Viewport", "2 Viewports", "3 Viewports", "4 Viewports"};
    static constexpr ViewportLayout LAYOUTS[] = {ViewportLayout::eOne,
                                                 ViewportLayout::eTwo,
                                                 ViewportLayout::eThree,
                                                 ViewportLayout::eFour};
    constexpr int layout_count = 4;
    int layout_idx = 0;
    for (int i = 0; i < layout_count; ++i) {
        if (LAYOUTS[i] == viewport_layout_) {
            layout_idx = i;
            break;
        }
    }
    if (ImGui::Combo("Viewport Layout", &layout_idx, layout_items, layout_count)) {
        layout_idx = (layout_idx >= 0 && layout_idx < layout_count) ? layout_idx : 0;
        viewport_layout_ = LAYOUTS[layout_idx];
    }

    // Font scale: applies to all ImGui text (same font, larger/smaller).
    // Keep spacing fixed so layout does not shift when changing the slider.
    if (ImGui::SliderFloat("Font scale", &font_scale_, 0.8f, 2.0f, "%.2f")) {
        ImGuiStyle& style = ImGui::GetStyle();
        style.FontScaleMain = font_scale_;
        style.ItemSpacing = ImVec2(kItemSpacingX, kItemSpacingY);
        style.FramePadding = ImVec2(kFramePaddingX, kFramePaddingY);
    }

#if defined(VNE_TESTBED_OPENGL)
    if (app_ctx_ && app_ctx_->window) {
        auto* glfw_win = dynamic_cast<window::GlfwWindow*>(app_ctx_->window);
        if (glfw_win) {
            bool vsync = glfw_win->isVsyncEnabled();
            if (ImGui::Checkbox("Vsync", &vsync)) {
                glfw_win->setVsync(vsync);
            }
        }
    }
#endif

    if (fps_buf_filled_ > 0 && fps_dt_sum_ > 0.0f) {
        const float avg_fps = static_cast<float>(fps_buf_filled_) / fps_dt_sum_;
        const float avg_ms = (fps_dt_sum_ / static_cast<float>(fps_buf_filled_)) * 1000.0f;
        ImGui::Text("FPS: %.1f (avg %zu frames)", static_cast<double>(avg_fps), fps_buf_filled_);
        ImGui::Text("Frame: %.3f ms", static_cast<double>(avg_ms));
    }

    // ---- Scrollable demo content (fills remainder of panel) -----------------

    if (settings_callback_) {
        ImGui::Separator();
        // Height = 0 means "fill to the bottom of the parent window".
        // The child scrolls independently of the viewport area.
        ImGui::BeginChild("SettingsScrollArea", ImVec2(0.0f, 0.0f), false, ImGuiWindowFlags_HorizontalScrollbar);
        settings_callback_();
        ImGui::EndChild();
    }

    ImGui::End();
}

void ImGuiLayer::renderViewportWindows(const RenderContext& ctx) {
    (void)ctx;
    viewport_rects_.clear();

    const int viewport_count = getViewportCount();
    const bool multi_viewport = (viewport_count > 1);

    auto drawViewport = [this, multi_viewport](const char* title, int index) {
        const unsigned int tex_id = multi_viewport ? getSceneTextureId(index) : getSceneTextureId();
        const bool has_scene = (tex_id != 0u);
        // C-style cast (ImTextureID)(intptr_t) works for both void* and ImU64 ImTextureID (ImGui FAQ)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
        const ImTextureID im_tex_id = (ImTextureID)(intptr_t)tex_id;
#if defined(__clang__)
#pragma clang diagnostic pop
#endif

        if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_None)) {
            ImVec2 pos = ImGui::GetWindowPos();
            ImVec2 size = ImGui::GetWindowSize();
            viewport_rects_.push_back(pos.x);
            viewport_rects_.push_back(pos.y);
            viewport_rects_.push_back(pos.x + size.x);
            viewport_rects_.push_back(pos.y + size.y);

            ImVec2 content_size = ImGui::GetContentRegionAvail();
            if (has_scene && content_size.x > 0 && content_size.y > 0) {
                // ImTextureID via (ImTextureID)(intptr_t) (OpenGL convention; works for void* or ImU64)
                ImGui::Image(im_tex_id, content_size, ImVec2(0, 1), ImVec2(1, 0));
            } else {
                ImGui::Text("No scene");
            }
        }
        ImGui::End();
    };

    switch (viewport_layout_) {
        case ViewportLayout::eOne:
            drawViewport("Viewport", 0);
            break;
        case ViewportLayout::eTwo:
            drawViewport("Viewport 1", 0);
            drawViewport("Viewport 2", 1);
            break;
        case ViewportLayout::eThree:
            drawViewport("Viewport 1", 0);
            drawViewport("Viewport 2", 1);
            drawViewport("Viewport 3", 2);
            break;
        case ViewportLayout::eFour:
            drawViewport("Viewport 1", 0);
            drawViewport("Viewport 2", 1);
            drawViewport("Viewport 3", 2);
            drawViewport("Viewport 4", 3);
            break;
    }
}

bool ImGuiLayer::isMouseOverSceneViewport(float mouse_x, float mouse_y) const {
    return getHoveredViewportIndex(mouse_x, mouse_y) >= 0;
}

int ImGuiLayer::getHoveredViewportIndex(float mouse_x, float mouse_y) const {
    const size_t n = viewport_rects_.size() / 4u;
    for (size_t i = 0; i < n; ++i) {
        const float min_x = viewport_rects_[i * 4u + 0u];
        const float min_y = viewport_rects_[i * 4u + 1u];
        const float max_x = viewport_rects_[i * 4u + 2u];
        const float max_y = viewport_rects_[i * 4u + 3u];
        if (mouse_x >= min_x && mouse_x <= max_x && mouse_y >= min_y && mouse_y <= max_y) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

std::string ImGuiLayer::getViewportName(int index) const {
    const int n = getViewportCount();
    if (index < 0 || index >= n) {
        return {};
    }
    if (viewport_layout_ == ViewportLayout::eOne) {
        return "Viewport";
    }
    return "Viewport " + std::to_string(index + 1);
}

unsigned int ImGuiLayer::getSceneTextureId() const {
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    if (scene_fbo_ && scene_fbo_->isValid()) {
        return scene_fbo_->getColorTextureId();
    }
#endif
    return 0u;
}

unsigned int ImGuiLayer::getSceneTextureId(int viewport_index) const {
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    if (viewport_index >= 0 && viewport_index < static_cast<int>(viewport_fbos_.size())) {
        if (viewport_fbos_[static_cast<size_t>(viewport_index)]
            && viewport_fbos_[static_cast<size_t>(viewport_index)]->isValid()) {
            return viewport_fbos_[static_cast<size_t>(viewport_index)]->getColorTextureId();
        }
    }
#endif
    return 0u;
}

void ImGuiLayer::ensureSceneFbo(int width, int height) {
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    if (width <= 0 || height <= 0) {
        return;
    }
    if (scene_fbo_ && scene_fbo_width_ == width && scene_fbo_height_ == height) {
        return;
    }
    gl::FramebufferDescriptor desc{};
    desc.width = static_cast<uint32_t>(width);
    desc.height = static_cast<uint32_t>(height);
    desc.attach_depth = true;
    scene_fbo_ = std::make_unique<gl::Framebuffer>(desc);
    scene_fbo_width_ = width;
    scene_fbo_height_ = height;
#endif
}

void ImGuiLayer::ensureViewportFbos(int count, int width, int height) {
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    if (count <= 0 || width <= 0 || height <= 0) {
        return;
    }
    if (viewport_fbo_count_ == count && viewport_fbo_width_ == width && viewport_fbo_height_ == height) {
        return;
    }
    viewport_fbos_.clear();
    viewport_fbos_.reserve(static_cast<size_t>(count));
    gl::FramebufferDescriptor desc{};
    desc.width = static_cast<uint32_t>(width);
    desc.height = static_cast<uint32_t>(height);
    desc.attach_depth = true;
    for (int i = 0; i < count; ++i) {
        viewport_fbos_.push_back(std::make_unique<gl::Framebuffer>(desc));
    }
    viewport_fbo_count_ = count;
    viewport_fbo_width_ = width;
    viewport_fbo_height_ = height;
#endif
}

}  // namespace testbed
}  // namespace vne
