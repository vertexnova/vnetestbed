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

ImGuiLayer::ImGuiLayer()
    : ILayer("ImGuiLayer") {
    setRenderSortKey(1000);  // Render last (on top)
}

ImGuiLayer::~ImGuiLayer() = default;

void ImGuiLayer::onAttach(AppContext& ctx) {
    app_ctx_ = &ctx;
    if (!app_ctx_->window || !app_ctx_->window->getNativeHandle()) {
        return;
    }

    GLFWwindow* window = static_cast<GLFWwindow*>(app_ctx_->window->getNativeHandle());

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    (void)io;

    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

    ImGui::StyleColorsDark();

    ImGuiStyle& style = ImGui::GetStyle();
    if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable) {
        style.WindowRounding = 0.0f;
        style.Colors[ImGuiCol_WindowBg].w = 1.0f;
    }

#if defined(VNE_TESTBED_OPENGLES)
    const char* glsl_version = "#version 300 es";
#else
    const char* glsl_version = "#version 410 core";
#endif

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init(glsl_version);

    initialized_ = true;
}

void ImGuiLayer::onDetach() {
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    scene_fbo_.reset();
    scene_fbo_width_ = 0;
    scene_fbo_height_ = 0;
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
    ensureSceneFbo(ctx.frame_info.width, ctx.frame_info.height);
    if (scene_fbo_ && scene_fbo_->isValid()) {
        scene_fbo_->bind();
        glClearColor(0.12f, 0.12f, 0.16f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        glViewport(0, 0, ctx.frame_info.width, ctx.frame_info.height);
    }
#endif
}

void ImGuiLayer::onGuiBegin(const RenderContext& ctx) {
    (void)ctx;
    if (!initialized_) {
        return;
    }
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    if (scene_fbo_ && scene_fbo_->isValid()) {
        scene_fbo_->unbind();
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        if (app_ctx_ && app_ctx_->window) {
            glViewport(0, 0, app_ctx_->window->getWidth(), app_ctx_->window->getHeight());
        }
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

        ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_None);

        renderSettingsPanel(ctx);
        renderViewportWindows(ctx);

        if (show_demo_window_) {
            ImGui::ShowDemoWindow(&show_demo_window_);
        }
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

void ImGuiLayer::setupDockLayout(ImGuiID dockspace_id, const ImVec2& size) {
    ImGui::DockBuilderRemoveNodeChildNodes(dockspace_id);
    ImGui::DockBuilderSetNodeSize(dockspace_id, size);

    // Split: Settings (left 25%) | Viewport area (right 75%)
    ImGuiID id_right{};
    ImGuiID id_settings{};
    ImGui::DockBuilderSplitNode(dockspace_id, ImGuiDir_Right, 0.75f, &id_right, &id_settings);
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
    }

    ImGui::DockBuilderFinish(dockspace_id);
}

void ImGuiLayer::renderSettingsPanel(const RenderContext& ctx) {
    if (!ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    const char* layout_items[] = {"1 Viewport", "2 Viewports", "4 Viewports"};
    static constexpr ViewportLayout LAYOUTS[] = {
        ViewportLayout::eOne, ViewportLayout::eTwo, ViewportLayout::eFour};
    int layout_idx = 0;
    for (int i = 0; i < 3; ++i) {
        if (LAYOUTS[i] == viewport_layout_) {
            layout_idx = i;
            break;
        }
    }
    if (ImGui::Combo("Viewport Layout", &layout_idx, layout_items, 3)) {
        layout_idx = (layout_idx >= 0 && layout_idx < 3) ? layout_idx : 0;
        viewport_layout_ = LAYOUTS[layout_idx];
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

    ImGui::Checkbox("Show ImGui Demo", &show_demo_window_);

    if (ctx.frame_info.dt > 0.0f) {
        float fps = 1.0f / ctx.frame_info.dt;
        ImGui::Text("FPS: %.1f", static_cast<double>(fps));
        ImGui::Text("Frame: %.3f ms", static_cast<double>(ctx.frame_info.dt * 1000.0f));
    }

    if (settings_callback_) {
        ImGui::Separator();
        settings_callback_();
    }

    ImGui::End();
}

void ImGuiLayer::renderViewportWindows(const RenderContext& ctx) {
    (void)ctx;
    const unsigned int tex_id = getSceneTextureId();
    const bool has_scene = (tex_id != 0u);
    const ImTextureID im_tex_id = static_cast<ImTextureID>(tex_id);

    auto drawViewport = [im_tex_id, has_scene](const char* title) {
        if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_None)) {
            ImVec2 size = ImGui::GetContentRegionAvail();
            if (has_scene && size.x > 0 && size.y > 0) {
#if IMGUI_VERSION_NUM >= 19200
                ImGui::Image(ImTextureRef(im_tex_id), size, ImVec2(0, 1), ImVec2(1, 0));
#else
                ImGui::Image(im_tex_id, size, ImVec2(0, 1), ImVec2(1, 0));
#endif
            } else {
                ImGui::Text("No scene");
            }
        }
        ImGui::End();
    };

    switch (viewport_layout_) {
        case ViewportLayout::eOne:
            drawViewport("Viewport");
            break;
        case ViewportLayout::eTwo:
            drawViewport("Viewport 1");
            drawViewport("Viewport 2");
            break;
        case ViewportLayout::eFour:
            drawViewport("Viewport 1");
            drawViewport("Viewport 2");
            drawViewport("Viewport 3");
            drawViewport("Viewport 4");
            break;
    }
}

unsigned int ImGuiLayer::getSceneTextureId() const {
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    if (scene_fbo_ && scene_fbo_->isValid()) {
        return scene_fbo_->getColorTextureId();
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

}  // namespace testbed
}  // namespace vne
