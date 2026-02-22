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
 * @file scene_inspector_plugin.h
 * @brief Example plugin: scene graph inspector panel.
 *
 * Displays a tree view of the scene graph with transform, camera, and
 * light panels using ImGui.  This plugin does **not** depend on any
 * specific rendering backend; it only reads scene data and draws UI.
 *
 * This stub provides empty default implementations so that derived
 * classes (or unit tests) can override only the hooks they care about.
 */

#include "vertexnova/testbed/plugin.h"

namespace vne {
namespace testbed_ns {

/**
 * @class SceneInspectorPlugin
 * @brief Example plugin: scene graph inspector panel (stub).
 */
class SceneInspectorPlugin : public IPlugin {
public:
    /** @brief Called once on startup; cache any scene references here. */
    void onInit() override {}

    /** @brief No per-frame logic needed for a pure UI plugin. */
    void onUpdate(float /*dt*/) override {}

    /** @brief No direct render output; all output is via onImGui(). */
    void onRender() override {}

    /**
     * @brief Draw the scene inspector ImGui window.
     *
     * Shows a collapsible tree of scene nodes with per-node transform,
     * camera settings, and light parameters.
     */
    void onImGui() override {}

    /** @brief Called on shutdown; release any held references. */
    void onShutdown() override {}
};

}  // namespace testbed_ns
}  // namespace vne
