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

#include "vertexnova/testbed/i_debug_draw.h"
#include "vertexnova/testbed/i_plugin.h"
#include "vertexnova/testbed/i_render_adapter.h"
#include "vertexnova/testbed/plugin_manager.h"
#include "vertexnova/testbed/plugins/scene_inspector_plugin.h"

// ---------------------------------------------------------------------------
// Helper: a minimal plugin that records every lifecycle call.
// ---------------------------------------------------------------------------
struct RecordingPlugin : public vne::testbed_ns::IPlugin {
    int init_count{0};
    int update_count{0};
    int render_count{0};
    int imgui_count{0};
    int shutdown_count{0};
    float last_dt{0.0F};

    void onInit() override { ++init_count; }
    void onUpdate(float dt) override {
        ++update_count;
        last_dt = dt;
    }
    void onRender() override { ++render_count; }
    void onImGui() override { ++imgui_count; }
    void onShutdown() override { ++shutdown_count; }
};

// ---------------------------------------------------------------------------
// PluginManager: basic lifecycle
// ---------------------------------------------------------------------------

TEST(PluginManager, StartsEmpty) {
    vne::testbed_ns::PluginManager mgr;
    EXPECT_EQ(mgr.pluginCount(), 0u);
}

TEST(PluginManager, AddPluginIncreasesCount) {
    vne::testbed_ns::PluginManager mgr;
    mgr.addPlugin(std::make_unique<RecordingPlugin>());
    EXPECT_EQ(mgr.pluginCount(), 1u);
    mgr.addPlugin(std::make_unique<RecordingPlugin>());
    EXPECT_EQ(mgr.pluginCount(), 2u);
}

TEST(PluginManager, InitCallsOnInitOnAllPlugins) {
    vne::testbed_ns::PluginManager mgr;
    auto* a = new RecordingPlugin;
    auto* b = new RecordingPlugin;
    mgr.addPlugin(std::unique_ptr<RecordingPlugin>(a));
    mgr.addPlugin(std::unique_ptr<RecordingPlugin>(b));

    mgr.init();

    EXPECT_EQ(a->init_count, 1);
    EXPECT_EQ(b->init_count, 1);
}

TEST(PluginManager, UpdatePassesDeltaTime) {
    vne::testbed_ns::PluginManager mgr;
    auto* p = new RecordingPlugin;
    mgr.addPlugin(std::unique_ptr<RecordingPlugin>(p));

    mgr.update(0.016F);

    EXPECT_EQ(p->update_count, 1);
    EXPECT_FLOAT_EQ(p->last_dt, 0.016F);
}

TEST(PluginManager, RenderCallsOnRenderOnAllPlugins) {
    vne::testbed_ns::PluginManager mgr;
    auto* a = new RecordingPlugin;
    auto* b = new RecordingPlugin;
    mgr.addPlugin(std::unique_ptr<RecordingPlugin>(a));
    mgr.addPlugin(std::unique_ptr<RecordingPlugin>(b));

    mgr.render();

    EXPECT_EQ(a->render_count, 1);
    EXPECT_EQ(b->render_count, 1);
}

TEST(PluginManager, ImGuiCallsOnImGuiOnAllPlugins) {
    vne::testbed_ns::PluginManager mgr;
    auto* p = new RecordingPlugin;
    mgr.addPlugin(std::unique_ptr<RecordingPlugin>(p));

    mgr.imGui();

    EXPECT_EQ(p->imgui_count, 1);
}

TEST(PluginManager, ShutdownCallsOnShutdownInReverseOrder) {
    vne::testbed_ns::PluginManager mgr;

    std::vector<int> shutdown_order;
    struct OrderedPlugin : vne::testbed_ns::IPlugin {
        int id;
        std::vector<int>* order;
        OrderedPlugin(int id_, std::vector<int>* order_) : id(id_), order(order_) {}
        void onInit() override {}
        void onUpdate(float) override {}
        void onRender() override {}
        void onImGui() override {}
        void onShutdown() override { order->push_back(id); }
    };

    mgr.addPlugin(std::make_unique<OrderedPlugin>(1, &shutdown_order));
    mgr.addPlugin(std::make_unique<OrderedPlugin>(2, &shutdown_order));
    mgr.addPlugin(std::make_unique<OrderedPlugin>(3, &shutdown_order));

    mgr.shutdown();

    ASSERT_EQ(shutdown_order.size(), 3u);
    EXPECT_EQ(shutdown_order[0], 3);
    EXPECT_EQ(shutdown_order[1], 2);
    EXPECT_EQ(shutdown_order[2], 1);
}

TEST(PluginManager, ShutdownClearsPlugins) {
    vne::testbed_ns::PluginManager mgr;
    mgr.addPlugin(std::make_unique<RecordingPlugin>());
    EXPECT_EQ(mgr.pluginCount(), 1u);

    mgr.shutdown();

    EXPECT_EQ(mgr.pluginCount(), 0u);
}

// ---------------------------------------------------------------------------
// SceneInspectorPlugin: compile and instantiate
// ---------------------------------------------------------------------------

TEST(SceneInspectorPlugin, CanBeInstantiatedAndRegistered) {
    vne::testbed_ns::PluginManager mgr;
    mgr.addPlugin(std::make_unique<vne::testbed_ns::SceneInspectorPlugin>());
    EXPECT_EQ(mgr.pluginCount(), 1u);

    // Exercise full lifecycle without crashing
    mgr.init();
    mgr.update(0.016F);
    mgr.render();
    mgr.imGui();
    mgr.shutdown();
    EXPECT_EQ(mgr.pluginCount(), 0u);
}

// ---------------------------------------------------------------------------
// Interface sanity: IPlugin is an abstract base class
// ---------------------------------------------------------------------------

TEST(IPlugin, AbstractBaseCannotBeInstantiatedDirectly) {
    // Compile-time check: RecordingPlugin is a valid concrete IPlugin.
    static_assert(std::is_base_of_v<vne::testbed_ns::IPlugin, RecordingPlugin>);
    static_assert(!std::is_abstract_v<RecordingPlugin>);
    static_assert(std::is_abstract_v<vne::testbed_ns::IPlugin>);
}

// ---------------------------------------------------------------------------
// Interface sanity: IRenderAdapter and IDebugDraw are abstract base classes
// ---------------------------------------------------------------------------

TEST(IRenderAdapter, IsAbstract) {
    static_assert(std::is_abstract_v<vne::testbed_ns::IRenderAdapter>);
}

TEST(IDebugDraw, IsAbstract) {
    static_assert(std::is_abstract_v<vne::testbed_ns::IDebugDraw>);
}
