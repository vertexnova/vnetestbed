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
#include "vertexnova/testbed/plugin_manager.h"
#include "stub_plugin.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <memory>

namespace vne {
namespace testbed_ns {

// --- Runner-side implementations of core interfaces (GLFW/OpenGL) ---

class GlfwWindow : public IWindow {
public:
    explicit GlfwWindow(GLFWwindow* w) : window_(w) {}
    int getWidth() const override {
        int w = 0;
        glfwGetWindowSize(window_, &w, nullptr);
        return w;
    }
    int getHeight() const override {
        int h = 0;
        glfwGetWindowSize(window_, &h, nullptr);
        return h;
    }
    void pollEvents() override { glfwPollEvents(); }
    bool shouldClose() const override { return glfwWindowShouldClose(window_) != 0; }

private:
    GLFWwindow* window_;
};

class StubRendererAdapter : public IRendererAdapter {
public:
    void beginFrame() override {
        glClearColor(0.2f, 0.2f, 0.25f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    void endFrame() override { /* swap done by runner */ }
};

}  // namespace testbed_ns
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
    GLFWwindow* window = glfwCreateWindow(800, 600, "vnetestbed plugin runner", nullptr, nullptr);
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

    vne::testbed_ns::GlfwWindow glfwWindow(window);
    vne::testbed_ns::StubRendererAdapter stubRenderer;

    vne::testbed_ns::AppContext ctx;
    ctx.window = &glfwWindow;
    ctx.renderer = &stubRenderer;
    ctx.debugDraw = nullptr;

    vne::testbed_ns::PluginManager mgr;
    mgr.addPlugin(std::make_unique<vne::testbed_ns::StubPlugin>());
    mgr.init();

    auto prev = std::chrono::steady_clock::now();
    while (!ctx.window->shouldClose()) {
        auto now = std::chrono::steady_clock::now();
        float dt = static_cast<float>(std::chrono::duration<double>(now - prev).count());
        prev = now;

        mgr.update(dt);
        if (ctx.renderer)
            ctx.renderer->beginFrame();
        mgr.render();
        mgr.imGui();
        if (ctx.renderer)
            ctx.renderer->endFrame();

        glfwSwapBuffers(window);
        ctx.window->pollEvents();
    }

    mgr.shutdown();

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
