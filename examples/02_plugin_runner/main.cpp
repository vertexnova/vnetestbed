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

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/layer_stack.h"
#include "vertexnova/testbed/plugin_registry.h"
#include "vertexnova/testbed/render_context.h"
#include "stub_layer.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <memory>

namespace vne {
namespace testbed {

class GlfwWindow : public IWindow {
   public:
    explicit GlfwWindow(GLFWwindow* w)
        : window_(w) {}
    int getWidth() const override {
        int w = 0;
        glfwGetWindowSize(window_, &w, nullptr);
        return w;
    }
    int getHeight() const override {
        int h = 0;
        glfwGetWindowSize(window_, nullptr, &h);
        return h;
    }
    void pollEvents() override { glfwPollEvents(); }
    bool shouldClose() const override { return glfwWindowShouldClose(window_) != 0; }
    void* getNativeHandle() const override { return window_; }

   private:
    GLFWwindow* window_;
};

class StubRendererAdapter : public IRenderAdapter {
   public:
    bool init(void* /*window_handle*/) override { return true; }
    void beginFrame() override {
        glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    void endFrame() override { /* swap done by runner */ }
    void shutdown() override {}
};

}  // namespace testbed
}  // namespace vne

int main() {
    if (!glfwInit())
        return 1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
#endif
    GLFWwindow* window = glfwCreateWindow(800, 600, "vnetestbed layer runner", nullptr, nullptr);
    if (!window) {
        glfwTerminate();
        return 1;
    }
    glfwMakeContextCurrent(window);
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    vne::testbed::GlfwWindow glfwWindow(window);
    vne::testbed::StubRendererAdapter stubRenderer;

    vne::testbed::AppContext app_ctx;
    app_ctx.window = &glfwWindow;
    app_ctx.renderer = &stubRenderer;
    app_ctx.debugDraw = nullptr;

    vne::testbed::LayerStack layer_stack;
    layer_stack.pushLayer(std::make_unique<vne::testbed::StubLayer>(), app_ctx);
    vne::testbed::PluginRegistry::instance().createAndPushLayers(layer_stack, app_ctx);

    auto prev = std::chrono::steady_clock::now();
    while (!app_ctx.window->shouldClose()) {
        app_ctx.window->pollEvents();

        auto now = std::chrono::steady_clock::now();
        float dt = static_cast<float>(std::chrono::duration<double>(now - prev).count());
        prev = now;

        vne::testbed::RenderContext render_ctx{};
        render_ctx.frame_info.width = app_ctx.window->getWidth();
        render_ctx.frame_info.height = app_ctx.window->getHeight();
        render_ctx.frame_info.dt = dt;
        render_ctx.debug_draw = nullptr;

        layer_stack.onUpdate(dt);
        layer_stack.onGuiBegin(render_ctx);
        layer_stack.onGuiRender(render_ctx);
        layer_stack.onGuiEnd(render_ctx);

        if (app_ctx.renderer)
            app_ctx.renderer->beginFrame();
        layer_stack.onBeginRender(render_ctx);
        layer_stack.onRender(render_ctx);
        if (app_ctx.renderer)
            app_ctx.renderer->endFrame();

        glfwSwapBuffers(window);
    }

    layer_stack.clear();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
