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
 * @file context_test.cpp
 * @brief Unit tests for context structs and interface type traits.
 *
 * Covers: FrameInfo and RenderContext default values, AppContext default
 * null pointers, and static assertions for abstract interfaces (IWindow,
 * IRenderAdapter, IDebugDraw).
 */

#include <gtest/gtest.h>

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/debug_draw.h"
#include "vertexnova/testbed/render_adapter.h"
#include "vertexnova/testbed/render_context.h"
#include "vertexnova/testbed/window.h"

// ---------------------------------------------------------------------------
// FrameInfo: default values
// ---------------------------------------------------------------------------

TEST(FrameInfo, DefaultWidthZero) {
    vne::testbed::FrameInfo fi{};
    EXPECT_EQ(fi.width, 0);
}

TEST(FrameInfo, DefaultHeightZero) {
    vne::testbed::FrameInfo fi{};
    EXPECT_EQ(fi.height, 0);
}

TEST(FrameInfo, DefaultDtZero) {
    vne::testbed::FrameInfo fi{};
    EXPECT_FLOAT_EQ(fi.dt, 0.0F);
}

// ---------------------------------------------------------------------------
// RenderContext: default values
// ---------------------------------------------------------------------------

TEST(RenderContext, DefaultDebugDrawNull) {
    vne::testbed::RenderContext ctx{};
    EXPECT_EQ(ctx.debug_draw, nullptr);
}

TEST(RenderContext, DefaultFrameInfoZeroed) {
    vne::testbed::RenderContext ctx{};
    EXPECT_EQ(ctx.frame_info.width, 0);
    EXPECT_EQ(ctx.frame_info.height, 0);
    EXPECT_FLOAT_EQ(ctx.frame_info.dt, 0.0F);
}

TEST(RenderContext, FrameInfoCanBeAssigned) {
    vne::testbed::RenderContext ctx{};
    ctx.frame_info.width = 1920;
    ctx.frame_info.height = 1080;
    ctx.frame_info.dt = 0.016F;
    EXPECT_EQ(ctx.frame_info.width, 1920);
    EXPECT_EQ(ctx.frame_info.height, 1080);
    EXPECT_FLOAT_EQ(ctx.frame_info.dt, 0.016F);
}

// ---------------------------------------------------------------------------
// AppContext: default null pointers
// ---------------------------------------------------------------------------

TEST(AppContext, DefaultWindowNull) {
    vne::testbed::AppContext ctx{};
    EXPECT_EQ(ctx.window, nullptr);
}

TEST(AppContext, DefaultRendererNull) {
    vne::testbed::AppContext ctx{};
    EXPECT_EQ(ctx.renderer, nullptr);
}

TEST(AppContext, DefaultDebugDrawNull) {
    vne::testbed::AppContext ctx{};
    EXPECT_EQ(ctx.debugDraw, nullptr);
}

// ---------------------------------------------------------------------------
// Static assertions: abstract interface type traits
// ---------------------------------------------------------------------------

TEST(IWindow, IsAbstract) {
    static_assert(std::is_abstract_v<vne::testbed::IWindow>);
}

TEST(IWindow, IsPolymorphic) {
    static_assert(std::is_polymorphic_v<vne::testbed::IWindow>);
}

TEST(IRenderAdapter, IsAbstract) {
    static_assert(std::is_abstract_v<vne::testbed::IRenderAdapter>);
}

TEST(IRenderAdapter, IsPolymorphic) {
    static_assert(std::is_polymorphic_v<vne::testbed::IRenderAdapter>);
}

TEST(IDebugDraw, IsAbstract) {
    static_assert(std::is_abstract_v<vne::testbed::IDebugDraw>);
}

TEST(IDebugDraw, IsPolymorphic) {
    static_assert(std::is_polymorphic_v<vne::testbed::IDebugDraw>);
}
