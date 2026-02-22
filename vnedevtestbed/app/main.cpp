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

#include "vertexnova/devtestbed/app_context.h"
#include "vertexnova/devtestbed/plugin_registry.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <chrono>
#include <vector>

namespace vne {
namespace devtestbed {

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

}  // namespace devtestbed
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
    GLFWwindow* window = glfwCreateWindow(800, 600, "vnedevtestbed runner", nullptr, nullptr);
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

    vne::devtestbed::GlfwWindow glfwWindow(window);
    vne::devtestbed::StubRendererAdapter stubRenderer;

    vne::devtestbed::AppContext ctx;
    ctx.window = &glfwWindow;
    ctx.renderer = &stubRenderer;
    ctx.debugDraw = nullptr;

    std::vector<vne::devtestbed::IPlugin*> plugins = vne::devtestbed::PluginRegistry::instance().getPlugins();
    for (vne::devtestbed::IPlugin* p : plugins)
        p->onInit(ctx);

    auto prev = std::chrono::steady_clock::now();
    while (!ctx.window->shouldClose()) {
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - prev).count();
        prev = now;

        for (vne::devtestbed::IPlugin* p : plugins)
            p->onUpdate(ctx, dt);
        if (ctx.renderer)
            ctx.renderer->beginFrame();
        for (vne::devtestbed::IPlugin* p : plugins)
            p->onRender(ctx);
        for (vne::devtestbed::IPlugin* p : plugins)
            p->onGui(ctx);
        if (ctx.renderer)
            ctx.renderer->endFrame();

        glfwSwapBuffers(window);
        ctx.window->pollEvents();
    }

    for (auto it = plugins.rbegin(); it != plugins.rend(); ++it)
        (*it)->onShutdown(ctx);

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
