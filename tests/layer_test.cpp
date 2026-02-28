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
 * @file layer_test.cpp
 * @brief Unit tests for ILayer state management (standalone, no LayerStack).
 *
 * Covers: default state, setEnabled lifecycle callbacks, setVisible,
 * setWantsInput, setBlocksInput, setRenderSortKey, and getName.
 */

#include <gtest/gtest.h>

#include "recording_layer.h"

// ---------------------------------------------------------------------------
// ILayer: default state
// ---------------------------------------------------------------------------

TEST(ILayer, DefaultName) {
    RecordingLayer layer("MyLayer");
    EXPECT_EQ(layer.getName(), "MyLayer");
}

TEST(ILayer, DefaultEnabledTrue) {
    RecordingLayer layer;
    EXPECT_TRUE(layer.isEnabled());
}

TEST(ILayer, DefaultVisibleTrue) {
    RecordingLayer layer;
    EXPECT_TRUE(layer.isVisible());
}

TEST(ILayer, DefaultWantsInputTrue) {
    RecordingLayer layer;
    EXPECT_TRUE(layer.wantsInput());
}

TEST(ILayer, DefaultBlocksInputFalse) {
    RecordingLayer layer;
    EXPECT_FALSE(layer.blocksInput());
}

TEST(ILayer, DefaultRenderSortKeyZero) {
    RecordingLayer layer;
    EXPECT_EQ(layer.getRenderSortKey(), 0);
}

// ---------------------------------------------------------------------------
// ILayer: setEnabled — state transitions and callback firing
// ---------------------------------------------------------------------------

TEST(ILayer, SetEnabledFalseUpdatesStateAndCallsOnDisable) {
    RecordingLayer layer("Test");
    ASSERT_TRUE(layer.isEnabled());

    layer.setEnabled(false);

    EXPECT_FALSE(layer.isEnabled());
    EXPECT_EQ(layer.disable_count, 1);
    EXPECT_EQ(layer.enable_count, 0);
}

TEST(ILayer, SetEnabledTrueFromFalseUpdatesStateAndCallsOnEnable) {
    RecordingLayer layer("Test");
    layer.setEnabled(false);
    ASSERT_FALSE(layer.isEnabled());

    layer.setEnabled(true);

    EXPECT_TRUE(layer.isEnabled());
    EXPECT_EQ(layer.enable_count, 1);
    EXPECT_EQ(layer.disable_count, 1);
}

TEST(ILayer, SetEnabledNoOpWhenAlreadyTrue) {
    RecordingLayer layer("Test");
    ASSERT_TRUE(layer.isEnabled());

    layer.setEnabled(true);  // no-op

    EXPECT_EQ(layer.enable_count, 0);
    EXPECT_EQ(layer.disable_count, 0);
}

TEST(ILayer, SetEnabledNoOpWhenAlreadyFalse) {
    RecordingLayer layer("Test");
    layer.setEnabled(false);
    ASSERT_FALSE(layer.isEnabled());

    layer.setEnabled(false);  // no-op

    EXPECT_EQ(layer.disable_count, 1);  // only from the first call
    EXPECT_EQ(layer.enable_count, 0);
}

TEST(ILayer, SetEnabledRoundTrip) {
    RecordingLayer layer("Test");

    layer.setEnabled(false);  // disable_count = 1
    layer.setEnabled(true);   // enable_count  = 1
    layer.setEnabled(false);  // disable_count = 2

    EXPECT_FALSE(layer.isEnabled());
    EXPECT_EQ(layer.enable_count, 1);
    EXPECT_EQ(layer.disable_count, 2);
}

// ---------------------------------------------------------------------------
// ILayer: setVisible
// ---------------------------------------------------------------------------

TEST(ILayer, SetVisibleFalse) {
    RecordingLayer layer;
    ASSERT_TRUE(layer.isVisible());

    layer.setVisible(false);

    EXPECT_FALSE(layer.isVisible());
}

TEST(ILayer, SetVisibleTrue) {
    RecordingLayer layer;
    layer.setVisible(false);

    layer.setVisible(true);

    EXPECT_TRUE(layer.isVisible());
}

// ---------------------------------------------------------------------------
// ILayer: setWantsInput
// ---------------------------------------------------------------------------

TEST(ILayer, SetWantsInputFalse) {
    RecordingLayer layer;
    layer.setWantsInput(false);
    EXPECT_FALSE(layer.wantsInput());
}

TEST(ILayer, SetWantsInputTrue) {
    RecordingLayer layer;
    layer.setWantsInput(false);
    layer.setWantsInput(true);
    EXPECT_TRUE(layer.wantsInput());
}

// ---------------------------------------------------------------------------
// ILayer: setBlocksInput
// ---------------------------------------------------------------------------

TEST(ILayer, SetBlocksInputTrue) {
    RecordingLayer layer;
    layer.setBlocksInput(true);
    EXPECT_TRUE(layer.blocksInput());
}

TEST(ILayer, SetBlocksInputFalse) {
    RecordingLayer layer;
    layer.setBlocksInput(true);
    layer.setBlocksInput(false);
    EXPECT_FALSE(layer.blocksInput());
}

// ---------------------------------------------------------------------------
// ILayer: setRenderSortKey
// ---------------------------------------------------------------------------

TEST(ILayer, SetRenderSortKey) {
    RecordingLayer layer;
    layer.setRenderSortKey(42);
    EXPECT_EQ(layer.getRenderSortKey(), 42);
}

TEST(ILayer, SetRenderSortKeyNegative) {
    RecordingLayer layer;
    layer.setRenderSortKey(-5);
    EXPECT_EQ(layer.getRenderSortKey(), -5);
}

// ---------------------------------------------------------------------------
// Static assertions: ILayer type traits
// ---------------------------------------------------------------------------

TEST(ILayer, RecordingLayerIsConcreteILayer) {
    static_assert(std::is_base_of_v<vne::testbed::ILayer, RecordingLayer>);
    static_assert(!std::is_abstract_v<RecordingLayer>);
}

TEST(ILayer, ILayerIsPolymorphic) {
    static_assert(std::is_polymorphic_v<vne::testbed::ILayer>);
}
