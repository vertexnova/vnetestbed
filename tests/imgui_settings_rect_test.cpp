/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 * ----------------------------------------------------------------------
 */

#include <gtest/gtest.h>

#ifdef VNE_TESTBED_IMGUI
#include "vertexnova/testbed/imgui/imgui_layer.h"

using vne::testbed::detail::makeSettingsWindowScreenRectWhenBeginSkipped;

TEST(ImGuiSettingsRect, CollapsedWindowKeepsTitleBarRect) {
    const auto rect = makeSettingsWindowScreenRectWhenBeginSkipped(true, 10.f, 20.f, 300.f, 24.f);
    EXPECT_TRUE(rect.valid);
    EXPECT_FLOAT_EQ(rect.min_x, 10.f);
    EXPECT_FLOAT_EQ(rect.min_y, 20.f);
    EXPECT_FLOAT_EQ(rect.max_x, 310.f);
    EXPECT_FLOAT_EQ(rect.max_y, 44.f);
}

TEST(ImGuiSettingsRect, SkippedNonCollapsedWindowInvalidatesRect) {
    const auto rect = makeSettingsWindowScreenRectWhenBeginSkipped(false, 0.f, 0.f, 1920.f, 1080.f);
    EXPECT_FALSE(rect.valid);
    EXPECT_FLOAT_EQ(rect.min_x, 0.f);
    EXPECT_FLOAT_EQ(rect.min_y, 0.f);
    EXPECT_FLOAT_EQ(rect.max_x, 0.f);
    EXPECT_FLOAT_EQ(rect.max_y, 0.f);
}
#endif
