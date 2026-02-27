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
 * @file plugin_registry_test.cpp
 * @brief Unit tests for PluginRegistry: registration, null safety,
 *        layer creation, and singleton isolation via reset().
 *
 * Each test runs with a clean registry (reset() in SetUp and TearDown) so
 * that plugins registered in one test do not affect another.  Tests that
 * need a specific plugin register it explicitly.
 */

#include <gtest/gtest.h>

#include "recording_layer.h"

#include "vertexnova/testbed/plugin.h"
#include "vertexnova/testbed/plugin_registry.h"
#include "vertexnova/testbed/plugins/scene_inspector_plugin.h"

// ---------------------------------------------------------------------------
// MockPlugin: configurable layer count for isolation testing
// ---------------------------------------------------------------------------
struct MockPlugin : public vne::testbed::IPlugin {
    std::string name_;
    int layer_count_{0};

    MockPlugin(std::string name, int layer_count)
        : name_(std::move(name))
        , layer_count_(layer_count) {}

    std::string getName() const override { return name_; }

    std::vector<std::unique_ptr<vne::testbed::ILayer>> createLayers() override {
        std::vector<std::unique_ptr<vne::testbed::ILayer>> out;
        for (int i = 0; i < layer_count_; ++i) {
            out.push_back(std::make_unique<RecordingLayer>("MockLayer" + std::to_string(i)));
        }
        return out;
    }
};

// ---------------------------------------------------------------------------
// Fixture — resets the singleton before and after every test
// ---------------------------------------------------------------------------
class PluginRegistryTest : public ::testing::Test {
   protected:
    void SetUp() override {
        RecordingLayer::total_detach_count = 0;
        vne::testbed::PluginRegistry::instance().reset();
    }
    void TearDown() override {
        vne::testbed::PluginRegistry::instance().reset();
    }

    vne::testbed::LayerStack stack;
    vne::testbed::AppContext ctx{};
};

// ---------------------------------------------------------------------------
// Registration
// ---------------------------------------------------------------------------

TEST_F(PluginRegistryTest, StartsEmptyAfterReset) {
    EXPECT_EQ(vne::testbed::PluginRegistry::instance().getPluginCount(), 0u);
}

TEST_F(PluginRegistryTest, RegisterPluginWithNullDoesNotIncreaseCount) {
    auto& reg = vne::testbed::PluginRegistry::instance();
    reg.registerPlugin(nullptr);
    EXPECT_EQ(reg.getPluginCount(), 0u);
}

TEST_F(PluginRegistryTest, RegisterPluginAddsPlugin) {
    auto& reg = vne::testbed::PluginRegistry::instance();
    reg.registerPlugin(std::make_unique<MockPlugin>("PluginA", 0));
    EXPECT_EQ(reg.getPluginCount(), 1u);
    reg.registerPlugin(std::make_unique<MockPlugin>("PluginB", 0));
    EXPECT_EQ(reg.getPluginCount(), 2u);
}

// ---------------------------------------------------------------------------
// createAndPushLayers
// ---------------------------------------------------------------------------

TEST_F(PluginRegistryTest, CreateAndPushLayersWithNoPluginsDoesNothing) {
    vne::testbed::PluginRegistry::instance().createAndPushLayers(stack, ctx);
    EXPECT_EQ(stack.getCount(), 0u);
}

TEST_F(PluginRegistryTest, CreateAndPushLayersFromSinglePlugin) {
    auto& reg = vne::testbed::PluginRegistry::instance();
    reg.registerPlugin(std::make_unique<MockPlugin>("P", 2));

    reg.createAndPushLayers(stack, ctx);

    EXPECT_EQ(stack.getCount(), 2u);
    EXPECT_NE(stack.findLayerByName("MockLayer0"), nullptr);
    EXPECT_NE(stack.findLayerByName("MockLayer1"), nullptr);
    stack.clear();
}

TEST_F(PluginRegistryTest, CreateAndPushLayersFromPluginWithZeroLayers) {
    auto& reg = vne::testbed::PluginRegistry::instance();
    reg.registerPlugin(std::make_unique<MockPlugin>("EmptyPlugin", 0));

    reg.createAndPushLayers(stack, ctx);

    EXPECT_EQ(stack.getCount(), 0u);
}

TEST_F(PluginRegistryTest, CreateAndPushLayersFromMultiplePlugins) {
    auto& reg = vne::testbed::PluginRegistry::instance();
    reg.registerPlugin(std::make_unique<MockPlugin>("P1", 1));
    reg.registerPlugin(std::make_unique<MockPlugin>("P2", 2));

    reg.createAndPushLayers(stack, ctx);

    // P1 contributes MockLayer0; P2 also contributes MockLayer0 and MockLayer1.
    // findLayerByName returns the first match, so check count instead.
    EXPECT_EQ(stack.getCount(), 3u);
    stack.clear();
}

// ---------------------------------------------------------------------------
// Integration: SceneInspectorPlugin creates SceneInspectorLayer
// ---------------------------------------------------------------------------

TEST_F(PluginRegistryTest, CreateAndPushLayersPopulatesSceneInspector) {
    auto& reg = vne::testbed::PluginRegistry::instance();
    // Explicitly register since static-init plugins were cleared by reset()
    reg.registerPlugin(std::make_unique<vne::testbed::SceneInspectorPlugin>());
    EXPECT_EQ(reg.getPluginCount(), 1u);

    reg.createAndPushLayers(stack, ctx);

    EXPECT_EQ(stack.getCount(), 1u);
    auto* found = stack.findLayerByName("SceneInspector");
    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getName(), "SceneInspector");

    stack.onUpdate(0.016F);
    stack.onGuiRender({});
    stack.clear();
}

// ---------------------------------------------------------------------------
// Static assertions: IPlugin type traits
// ---------------------------------------------------------------------------

TEST(IPlugin, IsPolymorphic) {
    static_assert(std::is_polymorphic_v<vne::testbed::IPlugin>);
}

TEST(IPlugin, SceneInspectorPluginIsConcreteIPlugin) {
    static_assert(std::is_base_of_v<vne::testbed::IPlugin, vne::testbed::SceneInspectorPlugin>);
    static_assert(!std::is_abstract_v<vne::testbed::SceneInspectorPlugin>);
}
