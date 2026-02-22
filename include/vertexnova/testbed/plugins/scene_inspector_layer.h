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
 * @file scene_inspector_layer.h
 * @brief Runtime layer: scene graph inspector panel.
 *
 * Displays a tree view of the scene graph with transform, camera, and
 * light panels using ImGui.  This layer does **not** depend on any
 * specific rendering backend; it only reads scene data and draws UI.
 *
 * This stub provides empty default implementations so that derived
 * classes (or unit tests) can override only the hooks they care about.
 */

#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

namespace vne {
namespace testbed {

/**
 * @class SceneInspectorLayer
 * @brief Runtime layer: scene graph inspector panel (stub).
 */
class SceneInspectorLayer : public ILayer {
   public:
    SceneInspectorLayer()
        : ILayer("SceneInspector") {}

    void onAttach() override {}
    void onDetach() override {}
    void onUpdate(float /*dt*/) override {}
    void onBeginRender(const RenderContext& /*ctx*/) override {}
    void onRender(const RenderContext& /*ctx*/) override {}
    void onGuiBegin(const RenderContext& /*ctx*/) override {}
    void onGuiRender(const RenderContext& /*ctx*/) override {}
    void onGuiEnd(const RenderContext& /*ctx*/) override {}
};

}  // namespace testbed
}  // namespace vne
