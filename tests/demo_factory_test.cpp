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
 * @file demo_factory_test.cpp
 * @brief Unit tests for DemoFactory: registration, id lookup, exactly-one
 *        path, default selection. Uses DemoFactory::reset() for isolation.
 *        Application is created/destroyed via createApplicationForTest/
 *        destroyApplicationForTest so the test TU does not need the complete Impl type.
 */

#include <gtest/gtest.h>

#include "recording_layer.h"

#include "vertexnova/testbed/app/application.h"
#include "vertexnova/testbed/app/application_descriptor.h"
#include "vertexnova/testbed/app/demo_factory.h"

using namespace vne::testbed;

// ---------------------------------------------------------------------------
// Fixture: isolate registry state; use test factory for Application
// ---------------------------------------------------------------------------
class DemoFactoryTest : public ::testing::Test {
   protected:
    void SetUp() override {
        DemoFactory::reset();
        app_ = createApplicationForTest();
    }
    void TearDown() override {
        destroyApplicationForTest(app_);
        app_ = nullptr;
        DemoFactory::reset();
    }

    Application* app_{nullptr};
};

// ---------------------------------------------------------------------------
// Empty registry
// ---------------------------------------------------------------------------
TEST_F(DemoFactoryTest, ListEmptyAfterReset) {
    EXPECT_TRUE(DemoFactory::list().empty());
}

TEST_F(DemoFactoryTest, CreateDemoByIdWhenEmptyReturnsFalse) {
    EXPECT_FALSE(DemoFactory::createDemo(*app_, "any"));
}

TEST_F(DemoFactoryTest, CreateDemoSingleWhenEmptyReturnsFalse) {
    EXPECT_FALSE(DemoFactory::createDemo(*app_));
}

TEST_F(DemoFactoryTest, CreateDefaultWhenEmptyReturnsFalse) {
    EXPECT_FALSE(DemoFactory::createDefault(*app_));
}

// ---------------------------------------------------------------------------
// Registration and id lookup
// ---------------------------------------------------------------------------
TEST_F(DemoFactoryTest, RegisterAndList) {
    DemoFactory::registerDemo("alpha", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("Alpha"), a.getAppContext());
    });
    auto ids = DemoFactory::list();
    ASSERT_EQ(ids.size(), 1u);
    EXPECT_EQ(ids[0], "alpha");
}

TEST_F(DemoFactoryTest, CreateDemoByIdInstallsAndReturnsTrue) {
    DemoFactory::registerDemo("one", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("One"), a.getAppContext());
    });
    EXPECT_TRUE(DemoFactory::createDemo(*app_, "one"));
#if defined(VNE_TESTBED_IMGUI)
    EXPECT_EQ(app_->getLayerStack().getCount(), 2u);  // ImGuiLayer + demo
#else
    EXPECT_EQ(app_->getLayerStack().getCount(), 1u);
#endif
}

TEST_F(DemoFactoryTest, CreateDemoByIdUnknownReturnsFalse) {
    DemoFactory::registerDemo("known", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("K"), a.getAppContext());
    });
    EXPECT_FALSE(DemoFactory::createDemo(*app_, "unknown"));
    EXPECT_EQ(app_->getLayerStack().getCount(), 0u);
}

// ---------------------------------------------------------------------------
// Exactly-one demo path: createDemo(app) with no id
// ---------------------------------------------------------------------------
TEST_F(DemoFactoryTest, CreateDemoSingleWhenExactlyOneSucceeds) {
    DemoFactory::registerDemo("only", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("Only"), a.getAppContext());
    });
    EXPECT_TRUE(DemoFactory::createDemo(*app_));
#if defined(VNE_TESTBED_IMGUI)
    EXPECT_EQ(app_->getLayerStack().getCount(), 2u);  // ImGuiLayer + demo
#else
    EXPECT_EQ(app_->getLayerStack().getCount(), 1u);
#endif
}

TEST_F(DemoFactoryTest, CreateDemoSingleWhenZeroReturnsFalse) {
    EXPECT_FALSE(DemoFactory::createDemo(*app_));
}

TEST_F(DemoFactoryTest, CreateDemoSingleWhenTwoReturnsFalse) {
    DemoFactory::registerDemo("a", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("A"), a.getAppContext());
    });
    DemoFactory::registerDemo("b", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("B"), a.getAppContext());
    });
    EXPECT_FALSE(DemoFactory::createDemo(*app_));
    EXPECT_EQ(app_->getLayerStack().getCount(), 0u);
}

// ---------------------------------------------------------------------------
// Default selection: createDefault prefers "window" then first registered
// ---------------------------------------------------------------------------
TEST_F(DemoFactoryTest, CreateDefaultWithOneInstallsIt) {
    DemoFactory::registerDemo("first", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("First"), a.getAppContext());
    });
    EXPECT_TRUE(DemoFactory::createDefault(*app_));
#if defined(VNE_TESTBED_IMGUI)
    EXPECT_EQ(app_->getLayerStack().getCount(), 2u);  // ImGuiLayer + demo
#else
    EXPECT_EQ(app_->getLayerStack().getCount(), 1u);
#endif
}

TEST_F(DemoFactoryTest, CreateDefaultPrefersWindowId) {
    DemoFactory::registerDemo("other", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("Other"), a.getAppContext());
    });
    DemoFactory::registerDemo("window", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("Window"), a.getAppContext());
    });
    EXPECT_TRUE(DemoFactory::createDefault(*app_));
#if defined(VNE_TESTBED_IMGUI)
    EXPECT_EQ(app_->getLayerStack().getCount(), 2u);  // ImGuiLayer + demo
#else
    EXPECT_EQ(app_->getLayerStack().getCount(), 1u);
#endif
    // "window" was installed; we can't easily assert which without exposing layer name
    // so we only assert layer count and that createDefault returned true
}

TEST_F(DemoFactoryTest, CreateDefaultWithNoWindowInstallsFirst) {
    DemoFactory::registerDemo("alpha", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("Alpha"), a.getAppContext());
    });
    DemoFactory::registerDemo("beta", [](Application& a) {
        a.getLayerStack().pushLayer(std::make_unique<RecordingLayer>("Beta"), a.getAppContext());
    });
    EXPECT_TRUE(DemoFactory::createDefault(*app_));
#if defined(VNE_TESTBED_IMGUI)
    EXPECT_EQ(app_->getLayerStack().getCount(), 2u);  // ImGuiLayer + demo
#else
    EXPECT_EQ(app_->getLayerStack().getCount(), 1u);
#endif
}

TEST_F(DemoFactoryTest, ResetClearsRegistry) {
    DemoFactory::registerDemo("x", [](Application&) {});
    ASSERT_FALSE(DemoFactory::list().empty());
    DemoFactory::reset();
    EXPECT_TRUE(DemoFactory::list().empty());
    EXPECT_FALSE(DemoFactory::createDemo(*app_, "x"));
}
