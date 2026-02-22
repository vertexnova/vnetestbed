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

#include <gtest/gtest.h>

#include "vertexnova/testbed/debug_draw.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/layer_stack.h"
#include "vertexnova/testbed/plugin_registry.h"
#include "vertexnova/testbed/render_context.h"
#include "vertexnova/testbed/render_adapter.h"
#include "vertexnova/testbed/plugins/scene_inspector_layer.h"
#include "vertexnova/testbed/plugins/scene_inspector_plugin.h"

// ---------------------------------------------------------------------------
// Helper: a minimal layer that records every lifecycle call.
// ---------------------------------------------------------------------------
struct RecordingLayer : public vne::testbed::ILayer {
    int attach_count{0};
    int detach_count{0};
    int update_count{0};
    int begin_render_count{0};
    int render_count{0};
    int gui_begin_count{0};
    int gui_render_count{0};
    int gui_end_count{0};
    float last_dt{0.0F};

    RecordingLayer()
        : ILayer("RecordingLayer") {}

    void onAttach() override { ++attach_count; }
    void onDetach() override { ++detach_count; }
    void onUpdate(float dt) override {
        ++update_count;
        last_dt = dt;
    }
    void onBeginRender(const vne::testbed::RenderContext& /*ctx*/) override { ++begin_render_count; }
    void onRender(const vne::testbed::RenderContext& /*ctx*/) override { ++render_count; }
    void onGuiBegin(const vne::testbed::RenderContext& /*ctx*/) override { ++gui_begin_count; }
    void onGuiRender(const vne::testbed::RenderContext& /*ctx*/) override { ++gui_render_count; }
    void onGuiEnd(const vne::testbed::RenderContext& /*ctx*/) override { ++gui_end_count; }
};

// ---------------------------------------------------------------------------
// LayerStack: basic lifecycle
// ---------------------------------------------------------------------------

TEST(LayerStack, StartsEmpty) {
    vne::testbed::LayerStack stack;
    EXPECT_EQ(stack.getCount(), 0u);
    EXPECT_EQ(stack.getOverlayCount(), 0u);
}

TEST(LayerStack, PushLayerIncreasesCount) {
    vne::testbed::LayerStack stack;
    stack.pushLayer(std::make_unique<RecordingLayer>());
    EXPECT_EQ(stack.getCount(), 1u);
    stack.pushLayer(std::make_unique<RecordingLayer>());
    EXPECT_EQ(stack.getCount(), 2u);
}

TEST(LayerStack, PushLayerCallsOnAttach) {
    vne::testbed::LayerStack stack;
    auto* a = new RecordingLayer;
    auto* b = new RecordingLayer;
    stack.pushLayer(std::unique_ptr<RecordingLayer>(a));
    stack.pushLayer(std::unique_ptr<RecordingLayer>(b));

    EXPECT_EQ(a->attach_count, 1);
    EXPECT_EQ(b->attach_count, 1);
}

TEST(LayerStack, UpdatePassesDeltaTime) {
    vne::testbed::LayerStack stack;
    auto* p = new RecordingLayer;
    stack.pushLayer(std::unique_ptr<RecordingLayer>(p));

    stack.onUpdate(0.016F);

    EXPECT_EQ(p->update_count, 1);
    EXPECT_FLOAT_EQ(p->last_dt, 0.016F);
}

TEST(LayerStack, OnRenderCallsOnRenderOnAllLayers) {
    vne::testbed::LayerStack stack;
    auto* a = new RecordingLayer;
    auto* b = new RecordingLayer;
    stack.pushLayer(std::unique_ptr<RecordingLayer>(a));
    stack.pushLayer(std::unique_ptr<RecordingLayer>(b));

    vne::testbed::RenderContext ctx{};
    stack.onRender(ctx);

    EXPECT_EQ(a->render_count, 1);
    EXPECT_EQ(b->render_count, 1);
}

TEST(LayerStack, OnGuiRenderCallsOnGuiRenderOnAllLayers) {
    vne::testbed::LayerStack stack;
    auto* p = new RecordingLayer;
    stack.pushLayer(std::unique_ptr<RecordingLayer>(p));

    vne::testbed::RenderContext ctx{};
    stack.onGuiRender(ctx);

    EXPECT_EQ(p->gui_render_count, 1);
}

TEST(LayerStack, ClearCallsOnDetach) {
    vne::testbed::LayerStack stack;
    stack.pushLayer(std::make_unique<RecordingLayer>());
    stack.pushLayer(std::make_unique<RecordingLayer>());

    auto ptr1 = stack.popLayer();
    auto ptr2 = stack.popLayer();
    auto* r1 = dynamic_cast<RecordingLayer*>(ptr1.get());
    auto* r2 = dynamic_cast<RecordingLayer*>(ptr2.get());
    ASSERT_NE(r1, nullptr);
    ASSERT_NE(r2, nullptr);
    EXPECT_EQ(r1->detach_count, 1);
    EXPECT_EQ(r2->detach_count, 1);
    EXPECT_EQ(stack.getCount(), 0u);
}

TEST(LayerStack, ClearClearsLayers) {
    vne::testbed::LayerStack stack;
    stack.pushLayer(std::make_unique<RecordingLayer>());
    EXPECT_EQ(stack.getCount(), 1u);

    stack.clear();

    EXPECT_EQ(stack.getCount(), 0u);
}

TEST(LayerStack, FindLayerByName) {
    vne::testbed::LayerStack stack;
    stack.pushLayer(std::make_unique<RecordingLayer>());
    auto* found = stack.findLayerByName("RecordingLayer");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "RecordingLayer");
    EXPECT_EQ(stack.findLayerByName("NonExistent"), nullptr);
}

// ---------------------------------------------------------------------------
// SceneInspectorLayer: compile and instantiate (runtime layer)
// ---------------------------------------------------------------------------

TEST(SceneInspectorLayer, CanBeInstantiatedAndPushed) {
    vne::testbed::LayerStack stack;
    stack.pushLayer(std::make_unique<vne::testbed::SceneInspectorLayer>());
    EXPECT_EQ(stack.getCount(), 1u);

    vne::testbed::RenderContext ctx{};
    stack.onUpdate(0.016F);
    stack.onBeginRender(ctx);
    stack.onRender(ctx);
    stack.onGuiBegin(ctx);
    stack.onGuiRender(ctx);
    stack.onGuiEnd(ctx);
    stack.clear();
    EXPECT_EQ(stack.getCount(), 0u);
}

// ---------------------------------------------------------------------------
// SceneInspectorPlugin: creates SceneInspectorLayer (discovery)
// ---------------------------------------------------------------------------

TEST(SceneInspectorPlugin, CreateLayersReturnsValidLayer) {
    vne::testbed::SceneInspectorPlugin plugin;
    auto layers = plugin.createLayers();
    ASSERT_EQ(layers.size(), 1u);
    ASSERT_NE(layers[0], nullptr);
    EXPECT_EQ(layers[0]->getName(), "SceneInspector");
}

// ---------------------------------------------------------------------------
// PluginRegistry: createAndPushLayers populates from registered plugins
// ---------------------------------------------------------------------------

TEST(PluginRegistry, CreateAndPushLayersPopulatesStack) {
    vne::testbed::LayerStack stack;
    EXPECT_GE(vne::testbed::PluginRegistry::instance().getPluginCount(), 1u);

    vne::testbed::PluginRegistry::instance().createAndPushLayers(stack);
    EXPECT_GE(stack.getCount(), 1u);

    auto* found = stack.findLayerByName("SceneInspector");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "SceneInspector");

    vne::testbed::RenderContext ctx{};
    stack.onUpdate(0.016F);
    stack.onGuiRender(ctx);
    stack.clear();
}

// ---------------------------------------------------------------------------
// Interface sanity: ILayer is an abstract base class
// ---------------------------------------------------------------------------

TEST(ILayer, RecordingLayerIsConcreteILayer) {
    static_assert(std::is_base_of_v<vne::testbed::ILayer, RecordingLayer>);
    static_assert(!std::is_abstract_v<RecordingLayer>);
}

// ---------------------------------------------------------------------------
// Interface sanity: IRenderAdapter and IDebugDraw are abstract base classes
// ---------------------------------------------------------------------------

TEST(IRenderAdapter, IsAbstract) {
    static_assert(std::is_abstract_v<vne::testbed::IRenderAdapter>);
}

TEST(IDebugDraw, IsAbstract) {
    static_assert(std::is_abstract_v<vne::testbed::IDebugDraw>);
}
