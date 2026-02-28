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

#include "vertexnova/testbed/window/glfw_window.h"

#include <glad/glad.h>

#include <chrono>
#include <memory>

namespace vne {
namespace testbed {

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
    auto glfwWin = vne::testbed::window::GlfwWindow::create(800, 600, "vnetestbed layer runner", false);
    if (!glfwWin) {
        return 1;
    }
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        return 1;
    }

    vne::testbed::StubRendererAdapter stubRenderer;

    vne::testbed::AppContext app_ctx;
    app_ctx.window = glfwWin.get();
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

        glfwWin->swapBuffers();
    }

    layer_stack.clear();
    glfwWin.reset();
    return 0;
}
