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
 * @file app_context.h
 * @brief Backend-agnostic app context: window, renderer adapter, optional debug draw.
 *
 * The testbed core does not depend on GLFW, OpenGL, or any specific
 * rendering backend.  The runner constructs concrete implementations
 * of IWindow, IRenderAdapter, and optionally IDebugDraw, then passes
 * them through AppContext to every layer via onAttach().
 *
 * @see window.h       IWindow interface
 * @see render_adapter.h  IRenderAdapter interface
 * @see debug_draw.h   IDebugDraw interface
 */

#include "vertexnova/testbed/debug_draw.h"
#include "vertexnova/testbed/render_adapter.h"
#include "vertexnova/testbed/render_device.h"
#include "vertexnova/testbed/render_context.h"
#include "vertexnova/testbed/window.h"

#include <functional>

namespace vne {
namespace testbed {

/**
 * @struct AppContext
 * @brief Opaque container filled by the runner; plugins access window/renderer/debugDraw.
 *
 * All pointers are non-owning.  The runner is responsible for the lifetime
 * of each object and must ensure they outlive the LayerStack.
 * @c debugDraw may be @c nullptr when debug drawing is not required.
 *
 * @c renderSceneForViewport is set by Application for multi-viewport rendering.
 * When ImGuiLayer has 2 or 4 viewports, it calls this for each viewport.
 *
 * @c scene_rendered_by_imgui is set by ImGuiLayer when it performs the scene
 * render; Application skips the normal onRender in that case.
 */
struct AppContext {
    IWindow* window{nullptr};
    IRenderAdapter* renderer{nullptr};
    IRenderDevice* device{nullptr};  ///< Backend-agnostic resource factory + draw.
    IDebugDraw* debugDraw{nullptr};

    /// Callback to render scene for a specific viewport (used when layout has 2 or 4 viewports)
    std::function<void(const RenderContext&)> renderSceneForViewport;

    /// Set by ImGuiLayer when it renders the scene for multi-viewport; Application skips onRender
    bool scene_rendered_by_imgui{false};
};

}  // namespace testbed
}  // namespace vne
