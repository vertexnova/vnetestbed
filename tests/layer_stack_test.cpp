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

#include "vertexnova/testbed/app_context.h"
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
    int enable_count{0};
    int disable_count{0};
    static int total_detach_count;  // for tests that clear() and cannot keep pointers
    int update_count{0};
    int begin_render_count{0};
    int render_count{0};
    int gui_begin_count{0};
    int gui_render_count{0};
    int gui_end_count{0};
    float last_dt{0.0F};

    explicit RecordingLayer(const std::string& name = "RecordingLayer")
        : ILayer(name) {}

    void onAttach(vne::testbed::AppContext& /*ctx*/) override { ++attach_count; }
    void onDetach() override {
        ++detach_count;
        ++total_detach_count;
    }
    void onEnable() override { ++enable_count; }
    void onDisable() override { ++disable_count; }
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

int RecordingLayer::total_detach_count = 0;

// ---------------------------------------------------------------------------
// LayerStack tests
// ---------------------------------------------------------------------------

class LayerStackTest : public ::testing::Test {
   protected:
    void SetUp() override { RecordingLayer::total_detach_count = 0; }

    vne::testbed::LayerStack stack;
    vne::testbed::AppContext app_ctx{};
    vne::testbed::RenderContext render_ctx{};

    RecordingLayer* pushRecordingLayer(const std::string& name = "RecordingLayer") {
        auto* p = new RecordingLayer(name);
        stack.pushLayer(std::unique_ptr<RecordingLayer>(p), app_ctx);
        return p;
    }

    RecordingLayer* pushRecordingOverlay(const std::string& name = "RecordingOverlay") {
        auto* p = new RecordingLayer(name);
        stack.pushOverlay(std::unique_ptr<RecordingLayer>(p), app_ctx);
        return p;
    }
};

// ---------------------------------------------------------------------------
// LayerStack: basic lifecycle (using fixture)
// ---------------------------------------------------------------------------

TEST_F(LayerStackTest, StartsEmpty) {
    EXPECT_EQ(stack.getCount(), 0u);
    EXPECT_EQ(stack.getOverlayCount(), 0u);
}

TEST_F(LayerStackTest, PushLayerIncreasesCount) {
    pushRecordingLayer();
    EXPECT_EQ(stack.getCount(), 1u);
    pushRecordingLayer();
    EXPECT_EQ(stack.getCount(), 2u);
}

TEST_F(LayerStackTest, PushLayerCallsOnAttach) {
    auto* a = pushRecordingLayer("A");
    auto* b = pushRecordingLayer("B");
    EXPECT_EQ(a->attach_count, 1);
    EXPECT_EQ(b->attach_count, 1);
}

TEST_F(LayerStackTest, UpdatePassesDeltaTime) {
    auto* p = pushRecordingLayer();
    stack.onUpdate(0.016F);
    EXPECT_EQ(p->update_count, 1);
    EXPECT_FLOAT_EQ(p->last_dt, 0.016F);
}

TEST_F(LayerStackTest, OnBeginRenderCallsOnBeginRenderOnAllLayers) {
    auto* a = pushRecordingLayer();
    auto* b = pushRecordingLayer();
    stack.onBeginRender(render_ctx);
    EXPECT_EQ(a->begin_render_count, 1);
    EXPECT_EQ(b->begin_render_count, 1);
}

TEST_F(LayerStackTest, OnRenderCallsOnRenderOnAllLayers) {
    auto* a = pushRecordingLayer();
    auto* b = pushRecordingLayer();
    stack.onRender(render_ctx);
    EXPECT_EQ(a->render_count, 1);
    EXPECT_EQ(b->render_count, 1);
}

TEST_F(LayerStackTest, OnGuiBeginCallsOnGuiBeginOnAllLayers) {
    auto* p = pushRecordingLayer();
    stack.onGuiBegin(render_ctx);
    EXPECT_EQ(p->gui_begin_count, 1);
}

TEST_F(LayerStackTest, OnGuiRenderCallsOnGuiRenderOnAllLayers) {
    auto* p = pushRecordingLayer();
    stack.onGuiRender(render_ctx);
    EXPECT_EQ(p->gui_render_count, 1);
}

TEST_F(LayerStackTest, OnGuiEndCallsOnGuiEndOnAllLayers) {
    auto* p = pushRecordingLayer();
    stack.onGuiEnd(render_ctx);
    EXPECT_EQ(p->gui_end_count, 1);
}

TEST_F(LayerStackTest, PopLayerCallsOnDetach) {
    pushRecordingLayer();
    pushRecordingLayer();
    auto ptr2 = stack.popLayer();
    auto ptr1 = stack.popLayer();
    auto* r1 = dynamic_cast<RecordingLayer*>(ptr1.get());
    auto* r2 = dynamic_cast<RecordingLayer*>(ptr2.get());
    ASSERT_NE(r1, nullptr);
    ASSERT_NE(r2, nullptr);
    EXPECT_EQ(r1->detach_count, 1);
    EXPECT_EQ(r2->detach_count, 1);
    EXPECT_EQ(stack.getCount(), 0u);
}

TEST_F(LayerStackTest, ClearClearsLayers) {
    pushRecordingLayer();
    EXPECT_EQ(stack.getCount(), 1u);
    stack.clear();
    EXPECT_EQ(stack.getCount(), 0u);
}

TEST_F(LayerStackTest, ClearCallsOnDetach) {
    pushRecordingLayer();
    pushRecordingLayer();
    stack.clear();
    EXPECT_EQ(RecordingLayer::total_detach_count, 2);
}

TEST_F(LayerStackTest, FindLayerByName) {
    pushRecordingLayer("RecordingLayer");
    auto* found = stack.findLayerByName("RecordingLayer");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "RecordingLayer");
    EXPECT_EQ(stack.findLayerByName("NonExistent"), nullptr);
}

TEST_F(LayerStackTest, GetAllReturnsAllLayers) {
    pushRecordingLayer("A");
    pushRecordingLayer("B");
    auto all = stack.getAll();
    ASSERT_EQ(all.size(), 2u);
    EXPECT_EQ(all[0]->getName(), "A");
    EXPECT_EQ(all[1]->getName(), "B");
}

TEST_F(LayerStackTest, PopLayerWhenEmptyReturnsNull) {
    auto ptr = stack.popLayer();
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(LayerStackTest, PushLayerWithNullDoesNothing) {
    stack.pushLayer(nullptr, app_ctx);
    EXPECT_EQ(stack.getCount(), 0u);
}

// ---------------------------------------------------------------------------
// LayerStack: overlays
// ---------------------------------------------------------------------------

TEST_F(LayerStackTest, PushOverlayIncreasesOverlayCount) {
    pushRecordingOverlay();
    EXPECT_EQ(stack.getOverlayCount(), 1u);
    pushRecordingOverlay("Overlay2");
    EXPECT_EQ(stack.getOverlayCount(), 2u);
}

TEST_F(LayerStackTest, PushOverlayCallsOnAttach) {
    auto* o = pushRecordingOverlay("Overlay");
    EXPECT_EQ(o->attach_count, 1);
}

TEST_F(LayerStackTest, PopOverlayCallsOnDetach) {
    pushRecordingOverlay();
    auto ptr = stack.popOverlay();
    auto* r = dynamic_cast<RecordingLayer*>(ptr.get());
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->detach_count, 1);
    EXPECT_EQ(stack.getOverlayCount(), 0u);
}

TEST_F(LayerStackTest, PopOverlayWhenEmptyReturnsNull) {
    auto ptr = stack.popOverlay();
    EXPECT_EQ(ptr, nullptr);
}

TEST_F(LayerStackTest, PushOverlayWithNullDoesNothing) {
    stack.pushOverlay(nullptr, app_ctx);
    EXPECT_EQ(stack.getOverlayCount(), 0u);
}

TEST_F(LayerStackTest, FindLayerByNameSearchesOverlays) {
    pushRecordingLayer("Layer");
    pushRecordingOverlay("Overlay");
    auto* found_layer = stack.findLayerByName("Layer");
    auto* found_overlay = stack.findLayerByName("Overlay");
    ASSERT_NE(found_layer, nullptr);
    ASSERT_NE(found_overlay, nullptr);
    EXPECT_EQ(found_layer->getName(), "Layer");
    EXPECT_EQ(found_overlay->getName(), "Overlay");
}

TEST_F(LayerStackTest, GetAllReturnsLayersThenOverlays) {
    pushRecordingLayer("L1");
    pushRecordingLayer("L2");
    pushRecordingOverlay("O1");
    auto all = stack.getAll();
    ASSERT_EQ(all.size(), 3u);
    EXPECT_EQ(all[0]->getName(), "L1");
    EXPECT_EQ(all[1]->getName(), "L2");
    EXPECT_EQ(all[2]->getName(), "O1");
}

TEST_F(LayerStackTest, OverlaysReceiveUpdateAndRender) {
    auto* o = pushRecordingOverlay();
    stack.onUpdate(0.1F);
    stack.onRender(render_ctx);
    stack.onGuiRender(render_ctx);
    EXPECT_EQ(o->update_count, 1);
    EXPECT_EQ(o->render_count, 1);
    EXPECT_EQ(o->gui_render_count, 1);
}

TEST_F(LayerStackTest, ClearDetachesOverlays) {
    pushRecordingOverlay();
    stack.clear();
    EXPECT_EQ(RecordingLayer::total_detach_count, 1);
    EXPECT_EQ(stack.getOverlayCount(), 0u);
}

// ---------------------------------------------------------------------------
// LayerStack: disabled and invisible layers
// ---------------------------------------------------------------------------

TEST_F(LayerStackTest, DisabledLayerDoesNotReceiveUpdate) {
    auto* p = pushRecordingLayer();
    p->setEnabled(false);
    stack.onUpdate(0.016F);
    EXPECT_EQ(p->update_count, 0);
}

TEST_F(LayerStackTest, DisabledLayerDoesNotReceiveRender) {
    auto* p = pushRecordingLayer();
    p->setEnabled(false);
    stack.onRender(render_ctx);
    EXPECT_EQ(p->render_count, 0);
}

TEST_F(LayerStackTest, InvisibleLayerDoesNotReceiveOnRender) {
    auto* p = pushRecordingLayer();
    p->setVisible(false);
    stack.onRender(render_ctx);
    EXPECT_EQ(p->render_count, 0);
}

TEST_F(LayerStackTest, InvisibleLayerStillReceivesOnGuiRender) {
    auto* p = pushRecordingLayer();
    p->setVisible(false);
    stack.onGuiRender(render_ctx);
    EXPECT_EQ(p->gui_render_count, 1);
}

TEST_F(LayerStackTest, PopLayerCallsOnDisableBeforeOnDetach) {
    auto* p = pushRecordingLayer();
    p->setEnabled(true);
    auto ptr = stack.popLayer();
    auto* r = dynamic_cast<RecordingLayer*>(ptr.get());
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->disable_count, 1);
    EXPECT_EQ(r->detach_count, 1);
}

// ---------------------------------------------------------------------------
// SceneInspectorLayer: compile and instantiate (runtime layer)
// ---------------------------------------------------------------------------

TEST_F(LayerStackTest, SceneInspectorLayerCanBeInstantiatedAndPushed) {
    stack.pushLayer(std::make_unique<vne::testbed::SceneInspectorLayer>(), app_ctx);
    EXPECT_EQ(stack.getCount(), 1u);

    stack.onUpdate(0.016F);
    stack.onBeginRender(render_ctx);
    stack.onRender(render_ctx);
    stack.onGuiBegin(render_ctx);
    stack.onGuiRender(render_ctx);
    stack.onGuiEnd(render_ctx);
    stack.clear();
    EXPECT_EQ(stack.getCount(), 0u);
}

// ---------------------------------------------------------------------------
// SceneInspectorPlugin: creates SceneInspectorLayer (discovery)
// ---------------------------------------------------------------------------

TEST(SceneInspectorPlugin, GetName) {
    vne::testbed::SceneInspectorPlugin plugin;
    EXPECT_EQ(plugin.getName(), "SceneInspector");
}

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

TEST_F(LayerStackTest, CreateAndPushLayersPopulatesStack) {
    EXPECT_GE(vne::testbed::PluginRegistry::instance().getPluginCount(), 1u);

    vne::testbed::PluginRegistry::instance().createAndPushLayers(stack, app_ctx);
    EXPECT_GE(stack.getCount(), 1u);

    auto* found = stack.findLayerByName("SceneInspector");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "SceneInspector");

    stack.onUpdate(0.016F);
    stack.onGuiRender(render_ctx);
    stack.clear();
}

// ---------------------------------------------------------------------------
// Interface sanity: ILayer, IPlugin, IRenderAdapter, IDebugDraw
// ---------------------------------------------------------------------------

TEST(ILayer, RecordingLayerIsConcreteILayer) {
    static_assert(std::is_base_of_v<vne::testbed::ILayer, RecordingLayer>);
    static_assert(!std::is_abstract_v<RecordingLayer>);
}

TEST(IPlugin, SceneInspectorPluginIsConcreteIPlugin) {
    static_assert(std::is_base_of_v<vne::testbed::IPlugin, vne::testbed::SceneInspectorPlugin>);
    static_assert(!std::is_abstract_v<vne::testbed::SceneInspectorPlugin>);
}

TEST(IRenderAdapter, IsAbstract) {
    static_assert(std::is_abstract_v<vne::testbed::IRenderAdapter>);
}

TEST(IDebugDraw, IsAbstract) {
    static_assert(std::is_abstract_v<vne::testbed::IDebugDraw>);
}
