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
 * @file layer_stack_test.cpp
 * @brief Unit tests for LayerStack: lifecycle dispatch, overlay management,
 *        visibility/enable gating, pop/clear ordering, and event propagation.
 */

#include <gtest/gtest.h>

#include "recording_layer.h"

#include "vertexnova/testbed/layer_stack.h"

// ---------------------------------------------------------------------------
// Fixture
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
// Basic lifecycle
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

TEST_F(LayerStackTest, PushLayerCallsOnAttachAndOnEnable) {
    auto* a = pushRecordingLayer("A");
    auto* b = pushRecordingLayer("B");
    EXPECT_EQ(a->attach_count, 1);
    EXPECT_EQ(a->enable_count, 1);
    EXPECT_EQ(b->attach_count, 1);
    EXPECT_EQ(b->enable_count, 1);
}

TEST_F(LayerStackTest, PushLayerWithNullDoesNothing) {
    stack.pushLayer(nullptr, app_ctx);
    EXPECT_EQ(stack.getCount(), 0u);
}

TEST_F(LayerStackTest, PopLayerWhenEmptyReturnsNull) {
    EXPECT_EQ(stack.popLayer(), nullptr);
}

TEST_F(LayerStackTest, PopLayerCallsOnDisableAndOnDetach) {
    pushRecordingLayer();
    pushRecordingLayer();
    auto ptr2 = stack.popLayer();
    auto ptr1 = stack.popLayer();
    auto* r1 = dynamic_cast<RecordingLayer*>(ptr1.get());
    auto* r2 = dynamic_cast<RecordingLayer*>(ptr2.get());
    ASSERT_NE(r1, nullptr);
    ASSERT_NE(r2, nullptr);
    EXPECT_EQ(r1->disable_count, 1);
    EXPECT_EQ(r1->detach_count, 1);
    EXPECT_EQ(r2->disable_count, 1);
    EXPECT_EQ(r2->detach_count, 1);
    EXPECT_EQ(stack.getCount(), 0u);
}

TEST_F(LayerStackTest, PopLayerSetsEnabledFalse) {
    pushRecordingLayer("Pop");
    auto ptr = stack.popLayer();
    auto* r = dynamic_cast<RecordingLayer*>(ptr.get());
    ASSERT_NE(r, nullptr);
    // is_enabled_ must be updated — not just the callback fired
    EXPECT_FALSE(r->isEnabled());
    EXPECT_EQ(r->disable_count, 1);
}

TEST_F(LayerStackTest, PopAlreadyDisabledLayerDoesNotCallOnDisableAgain) {
    auto* p = pushRecordingLayer();
    p->setEnabled(false);  // disable_count = 1
    auto ptr = stack.popLayer();
    auto* r = dynamic_cast<RecordingLayer*>(ptr.get());
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->disable_count, 1);  // no second onDisable on pop
}

TEST_F(LayerStackTest, ClearCallsOnDetach) {
    pushRecordingLayer();
    pushRecordingLayer();
    stack.clear();
    EXPECT_EQ(RecordingLayer::total_detach_count, 2);
    EXPECT_EQ(stack.getCount(), 0u);
}

TEST_F(LayerStackTest, ClearDetachesLayersInReverseOrder) {
    std::vector<std::string> log;
    auto* a = pushRecordingLayer("A");
    auto* b = pushRecordingLayer("B");
    auto* c = pushRecordingLayer("C");
    a->call_log = &log;
    b->call_log = &log;
    c->call_log = &log;

    stack.clear();  // clears stack, so log is safe after this

    // Last pushed must be first detached (LIFO)
    ASSERT_EQ(log.size(), 3u);
    EXPECT_EQ(log[0], "C::onDetach");
    EXPECT_EQ(log[1], "B::onDetach");
    EXPECT_EQ(log[2], "A::onDetach");
}

// ---------------------------------------------------------------------------
// Query methods
// ---------------------------------------------------------------------------

TEST_F(LayerStackTest, FindLayerByName) {
    pushRecordingLayer("Target");
    ASSERT_NE(stack.findLayerByName("Target"), nullptr);
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

// ---------------------------------------------------------------------------
// Per-frame dispatch
// ---------------------------------------------------------------------------

TEST_F(LayerStackTest, UpdatePassesDeltaTime) {
    auto* p = pushRecordingLayer();
    stack.onUpdate(0.016F);
    EXPECT_EQ(p->update_count, 1);
    EXPECT_FLOAT_EQ(p->last_dt, 0.016F);
}

TEST_F(LayerStackTest, OnBeginRenderCallsAllLayers) {
    auto* a = pushRecordingLayer();
    auto* b = pushRecordingLayer();
    stack.onBeginRender(render_ctx);
    EXPECT_EQ(a->begin_render_count, 1);
    EXPECT_EQ(b->begin_render_count, 1);
}

TEST_F(LayerStackTest, OnRenderCallsAllLayers) {
    auto* a = pushRecordingLayer();
    auto* b = pushRecordingLayer();
    stack.onRender(render_ctx);
    EXPECT_EQ(a->render_count, 1);
    EXPECT_EQ(b->render_count, 1);
}

TEST_F(LayerStackTest, OnGuiBeginCallsAllLayers) {
    auto* p = pushRecordingLayer();
    stack.onGuiBegin(render_ctx);
    EXPECT_EQ(p->gui_begin_count, 1);
}

TEST_F(LayerStackTest, OnGuiRenderCallsAllLayers) {
    auto* p = pushRecordingLayer();
    stack.onGuiRender(render_ctx);
    EXPECT_EQ(p->gui_render_count, 1);
}

TEST_F(LayerStackTest, OnGuiEndCallsAllLayers) {
    auto* p = pushRecordingLayer();
    stack.onGuiEnd(render_ctx);
    EXPECT_EQ(p->gui_end_count, 1);
}

// ---------------------------------------------------------------------------
// onGuiEnd: full reverse of onGuiBegin
// ---------------------------------------------------------------------------

TEST_F(LayerStackTest, OnGuiEndFiresInFullReverseOrder) {
    std::vector<std::string> log;
    auto* l0 = pushRecordingLayer("L0");
    auto* l1 = pushRecordingLayer("L1");
    auto* o0 = pushRecordingOverlay("O0");
    auto* o1 = pushRecordingOverlay("O1");
    l0->call_log = &log;
    l1->call_log = &log;
    o0->call_log = &log;
    o1->call_log = &log;

    stack.onGuiEnd(render_ctx);

    // Null before assertions: fixture teardown calls clear() → onDetach() which
    // would access the local `log` via call_log after it goes out of scope.
    l0->call_log = nullptr;
    l1->call_log = nullptr;
    o0->call_log = nullptr;
    o1->call_log = nullptr;

    // onGuiBegin order: L0, L1, O0, O1
    // onGuiEnd full reverse: overlays reversed (O1, O0), then layers reversed (L1, L0)
    ASSERT_EQ(log.size(), 4u);
    EXPECT_EQ(log[0], "O1::onGuiEnd");
    EXPECT_EQ(log[1], "O0::onGuiEnd");
    EXPECT_EQ(log[2], "L1::onGuiEnd");
    EXPECT_EQ(log[3], "L0::onGuiEnd");
}

// ---------------------------------------------------------------------------
// Disabled and invisible layer gating
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

TEST_F(LayerStackTest, SetEnabledFalseStopsAllFrameCallbacks) {
    auto* p = pushRecordingLayer();
    p->setEnabled(false);
    stack.onUpdate(0.016F);
    stack.onRender(render_ctx);
    stack.onGuiRender(render_ctx);
    EXPECT_EQ(p->update_count, 0);
    EXPECT_EQ(p->render_count, 0);
    EXPECT_EQ(p->gui_render_count, 0);
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

// ---------------------------------------------------------------------------
// Overlays
// ---------------------------------------------------------------------------

TEST_F(LayerStackTest, PushOverlayIncreasesOverlayCount) {
    pushRecordingOverlay();
    EXPECT_EQ(stack.getOverlayCount(), 1u);
    pushRecordingOverlay("Overlay2");
    EXPECT_EQ(stack.getOverlayCount(), 2u);
}

TEST_F(LayerStackTest, PushOverlayCallsOnAttachAndOnEnable) {
    auto* o = pushRecordingOverlay("Overlay");
    EXPECT_EQ(o->attach_count, 1);
    EXPECT_EQ(o->enable_count, 1);
}

TEST_F(LayerStackTest, PushOverlayWithNullDoesNothing) {
    stack.pushOverlay(nullptr, app_ctx);
    EXPECT_EQ(stack.getOverlayCount(), 0u);
}

TEST_F(LayerStackTest, PopOverlayWhenEmptyReturnsNull) {
    EXPECT_EQ(stack.popOverlay(), nullptr);
}

TEST_F(LayerStackTest, PopOverlayCallsOnDisableAndOnDetach) {
    pushRecordingOverlay();
    auto ptr = stack.popOverlay();
    auto* r = dynamic_cast<RecordingLayer*>(ptr.get());
    ASSERT_NE(r, nullptr);
    EXPECT_EQ(r->disable_count, 1);
    EXPECT_EQ(r->detach_count, 1);
    EXPECT_EQ(stack.getOverlayCount(), 0u);
}

TEST_F(LayerStackTest, PopOverlaySetsEnabledFalse) {
    pushRecordingOverlay("PopOv");
    auto ptr = stack.popOverlay();
    auto* r = dynamic_cast<RecordingLayer*>(ptr.get());
    ASSERT_NE(r, nullptr);
    EXPECT_FALSE(r->isEnabled());
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

TEST_F(LayerStackTest, FindLayerByNameSearchesOverlays) {
    pushRecordingLayer("Layer");
    pushRecordingOverlay("Overlay");
    ASSERT_NE(stack.findLayerByName("Layer"), nullptr);
    ASSERT_NE(stack.findLayerByName("Overlay"), nullptr);
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
