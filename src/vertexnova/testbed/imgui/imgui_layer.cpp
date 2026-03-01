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
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

#include <cstdio>

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

void ImGuiLayer::renderSettingsPanel(const RenderContext& ctx) {
    if (!ImGui::Begin("Settings", nullptr, ImGuiWindowFlags_None)) {
        ImGui::End();
        return;
    }

    const char* layout_items[] = {"1 Viewport", "2 Viewports", "4 Viewports"};
    int layout_idx = 0;
    switch (viewport_layout_) {
    case ViewportLayout::eOne:
        layout_idx = 0;
        break;
    case ViewportLayout::eTwo:
        layout_idx = 1;
        break;
    case ViewportLayout::eFour:
        layout_idx = 2;
        break;
    default:
        layout_idx = 0;
        break;
    }

    if (ImGui::Combo("Viewport Layout", &layout_idx, layout_items, 3)) {
        switch (layout_idx) {
        case 0:
            viewport_layout_ = ViewportLayout::eOne;
            break;
        case 1:
            viewport_layout_ = ViewportLayout::eTwo;
            break;
        case 2:
            viewport_layout_ = ViewportLayout::eFour;
            break;
        default:
            // Ignore invalid indices; keep previous layout.
            break;
        }
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
    const ImTextureRef tex_ref(static_cast<ImTextureID>(getSceneTextureId()));
    const bool has_scene = (getSceneTextureId() != 0u);

    auto drawViewport = [tex_ref, has_scene](const char* title) {
        if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_None)) {
            ImVec2 size = ImGui::GetContentRegionAvail();
            if (has_scene && size.x > 0 && size.y > 0) {
                ImGui::Image(tex_ref, size, ImVec2(0, 1), ImVec2(1, 0));
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
