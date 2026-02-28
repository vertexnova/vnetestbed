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
 * This stub provides no overrides so the ILayer defaults apply; real
 * ImGui logic will be added in onGuiRender() once the UI backend is wired.
 */

#include "vertexnova/testbed/layer.h"

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
};

}  // namespace testbed
}  // namespace vne
