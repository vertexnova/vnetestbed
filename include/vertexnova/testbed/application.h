#pragma once
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

/**
 * @file application.h
 * @brief Application harness: creates window and renderer from descriptor, owns LayerStack, runs main loop.
 *
 * Backend-agnostic API; concrete window/render creation is in the .cpp (GLFW + OpenGL today,
 * vnecross + OpenGL in a future build). Use initialize(descriptor), run(), shutdown().
 */

#include "vertexnova/testbed/application_descriptor.h"
#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/layer_stack.h"
#include "vertexnova/testbed/render_context.h"

#include <memory>

namespace vne {
namespace testbed {

/**
 * @class Application
 * @brief Owns window, render adapter, AppContext, and LayerStack; runs the main loop.
 *
 * Call initialize(descriptor) to create window and renderer from descriptor's
 * window_backend and render_backend. Then push layers (e.g. via DemoFactory)
 * and call run(). Call shutdown() before destruction.
 */
class Application {
   public:
    Application() = default;
    ~Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;
    Application(Application&&) noexcept = default;
    Application& operator=(Application&&) noexcept = default;

    /**
     * @brief Create window and renderer from descriptor; build AppContext and LayerStack.
     * @return true on success, false if backend is unsupported or creation failed.
     */
    bool initialize(const ApplicationDescriptor& descriptor);

    /**
     * @brief Run the main loop until the window requests close.
     */
    void run();

    /**
     * @brief Tear down layers, renderer, and window. Safe to call after run() or if initialize() failed.
     */
    void shutdown();

    /** @brief Layer stack; push layers here (e.g. from a demo installer) after initialize(). */
    LayerStack& getLayerStack() { return layer_stack_; }
    /** @brief Const access to the layer stack. */
    const LayerStack& getLayerStack() const { return layer_stack_; }

    /** @brief App context (window, renderer, device, debugDraw); valid after initialize(). */
    AppContext& getAppContext() { return app_ctx_; }
    /** @brief Const access to app context. */
    const AppContext& getAppContext() const { return app_ctx_; }

    /** @brief Current window; non-null after successful initialize(). */
    IWindow* getWindow() const { return app_ctx_.window; }

    /** @brief Whether initialize() succeeded and shutdown() has not been called. */
    [[nodiscard]] bool isRunning() const { return running_; }

   private:
    AppContext app_ctx_{};
    LayerStack layer_stack_;
    bool running_{false};

    // Opaque backend storage (window, render adapter, device, debug draw); owned in .cpp
    struct Impl;
    std::unique_ptr<Impl> impl_;
};

}  // namespace testbed
}  // namespace vne
