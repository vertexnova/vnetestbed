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
 * @file scene_inspector_test.cpp
 * @brief Unit tests for SceneInspectorLayer and SceneInspectorPlugin.
 *
 * Verifies that the layer can be instantiated, pushed onto a stack, and
 * survives a full frame dispatch cycle; and that the plugin produces the
 * expected layer with the correct name.
 */

#include <gtest/gtest.h>

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/layer_stack.h"
#include "vertexnova/testbed/render_context.h"
#include "vertexnova/testbed/plugins/scene_inspector_layer.h"
#include "vertexnova/testbed/plugins/scene_inspector_plugin.h"

// ---------------------------------------------------------------------------
// SceneInspectorLayer
// ---------------------------------------------------------------------------

TEST(SceneInspectorLayer, HasCorrectName) {
    vne::testbed::SceneInspectorLayer layer;
    EXPECT_EQ(layer.getName(), "SceneInspector");
}

TEST(SceneInspectorLayer, IsEnabledAndVisibleByDefault) {
    vne::testbed::SceneInspectorLayer layer;
    EXPECT_TRUE(layer.isEnabled());
    EXPECT_TRUE(layer.isVisible());
}

TEST(SceneInspectorLayer, CanBePushedAndDrivesFullFrameCycle) {
    vne::testbed::LayerStack stack;
    vne::testbed::AppContext app_ctx{};
    vne::testbed::RenderContext render_ctx{};

    stack.pushLayer(std::make_unique<vne::testbed::SceneInspectorLayer>(), app_ctx);
    EXPECT_EQ(stack.getCount(), 1u);

    // Full frame cycle must not crash
    stack.onUpdate(0.016F);
    stack.onGuiBegin(render_ctx);
    stack.onGuiRender(render_ctx);
    stack.onGuiEnd(render_ctx);
    stack.onBeginRender(render_ctx);
    stack.onRender(render_ctx);

    stack.clear();
    EXPECT_EQ(stack.getCount(), 0u);
}

TEST(SceneInspectorLayer, CanBeDisabledAndEnabled) {
    vne::testbed::SceneInspectorLayer layer;
    layer.setEnabled(false);
    EXPECT_FALSE(layer.isEnabled());
    layer.setEnabled(true);
    EXPECT_TRUE(layer.isEnabled());
}

// ---------------------------------------------------------------------------
// SceneInspectorPlugin
// ---------------------------------------------------------------------------

TEST(SceneInspectorPlugin, GetName) {
    vne::testbed::SceneInspectorPlugin plugin;
    EXPECT_EQ(plugin.getName(), "SceneInspector");
}

TEST(SceneInspectorPlugin, CreateLayersReturnsOneLayer) {
    vne::testbed::SceneInspectorPlugin plugin;
    auto layers = plugin.createLayers();
    ASSERT_EQ(layers.size(), 1u);
    ASSERT_NE(layers[0], nullptr);
}

TEST(SceneInspectorPlugin, CreatedLayerHasCorrectName) {
    vne::testbed::SceneInspectorPlugin plugin;
    auto layers = plugin.createLayers();
    ASSERT_FALSE(layers.empty());
    EXPECT_EQ(layers[0]->getName(), "SceneInspector");
}

TEST(SceneInspectorPlugin, CreateLayersEachCallProducesNewInstance) {
    vne::testbed::SceneInspectorPlugin plugin;
    auto layers1 = plugin.createLayers();
    auto layers2 = plugin.createLayers();
    ASSERT_EQ(layers1.size(), 1u);
    ASSERT_EQ(layers2.size(), 1u);
    EXPECT_NE(layers1[0].get(), layers2[0].get());
}
