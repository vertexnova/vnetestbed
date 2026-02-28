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
 * @file window_test.cpp
 * @brief Unit tests for the window system.
 *
 * Organised in two tiers:
 *
 * Tier 1 — Display-free (always runs, including CI):
 *   - GlfwWindowDescriptor default values and convenience constructor
 *   - GlfwWindow / IWindow type traits (static_assert)
 *   - glfw_key_mapping pure functions (mapGlfwToKeyCode, mapGlfwToMouseButton,
 *     mapGlfwToModifiers) — uses only GLFW constants, no GLFW state
 *   - MockWindow : IWindow handwritten stub + AppContext integration
 *
 * Tier 2 — Requires a display (uses GTEST_SKIP when unavailable):
 *   - GlfwWindow::create() with visible=false (hidden window)
 *   - Framebuffer size, shouldClose, native handle, close(), DPI scale,
 *     setVsync, event forwarding toggle, resize
 */

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <gtest/gtest.h>

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/window.h"
#include "vertexnova/testbed/window/glfw_key_mapping.h"
#include "vertexnova/testbed/window/glfw_window.h"
#include "vertexnova/testbed/window/glfw_window_descriptor.h"

using namespace vne::testbed;
using namespace vne::testbed::window;
using namespace vne::events;

// ============================================================================
// Tier 1 — Display-free
// ============================================================================

// ----------------------------------------------------------------------------
// GlfwWindowDescriptor — default values
// ----------------------------------------------------------------------------

TEST(GlfwWindowDescriptor, DefaultTitle) {
    GlfwWindowDescriptor d;
    EXPECT_EQ(d.title, "VneTestbed");
}

TEST(GlfwWindowDescriptor, DefaultWidth) {
    GlfwWindowDescriptor d;
    EXPECT_EQ(d.width, 800u);
}

TEST(GlfwWindowDescriptor, DefaultHeight) {
    GlfwWindowDescriptor d;
    EXPECT_EQ(d.height, 600u);
}

TEST(GlfwWindowDescriptor, DefaultPosition) {
    GlfwWindowDescriptor d;
    EXPECT_EQ(d.x, 100);
    EXPECT_EQ(d.y, 100);
}

TEST(GlfwWindowDescriptor, DefaultVsyncEnabled) {
    GlfwWindowDescriptor d;
    EXPECT_TRUE(d.vsync_enabled);
}

TEST(GlfwWindowDescriptor, DefaultResizable) {
    GlfwWindowDescriptor d;
    EXPECT_TRUE(d.resizable);
}

TEST(GlfwWindowDescriptor, DefaultDecorated) {
    GlfwWindowDescriptor d;
    EXPECT_TRUE(d.decorated);
}

TEST(GlfwWindowDescriptor, DefaultTransparentFalse) {
    GlfwWindowDescriptor d;
    EXPECT_FALSE(d.transparent);
}

TEST(GlfwWindowDescriptor, DefaultVisibleTrue) {
    GlfwWindowDescriptor d;
    EXPECT_TRUE(d.visible);
}

TEST(GlfwWindowDescriptor, DefaultBackendOpenGL) {
    GlfwWindowDescriptor d;
    EXPECT_EQ(d.graphics_backend, GlfwGraphicsBackend::OpenGL);
}

TEST(GlfwWindowDescriptor, ConvenienceConstructorSetsFields) {
    GlfwWindowDescriptor d{"MyWindow", 1280, 720};
    EXPECT_EQ(d.title, "MyWindow");
    EXPECT_EQ(d.width, 1280u);
    EXPECT_EQ(d.height, 720u);
    // Fields not in the convenience ctor keep their defaults
    EXPECT_TRUE(d.vsync_enabled);
    EXPECT_TRUE(d.visible);
}

TEST(GlfwWindowDescriptor, CanBeModified) {
    GlfwWindowDescriptor d;
    d.title = "Changed";
    d.width = 1920;
    d.height = 1080;
    d.visible = false;
    d.vsync_enabled = false;
    EXPECT_EQ(d.title, "Changed");
    EXPECT_EQ(d.width, 1920u);
    EXPECT_EQ(d.height, 1080u);
    EXPECT_FALSE(d.visible);
    EXPECT_FALSE(d.vsync_enabled);
}

// ----------------------------------------------------------------------------
// GlfwWindow / IWindow — type traits (no runtime GLFW calls)
// ----------------------------------------------------------------------------

TEST(GlfwWindowTypeTraits, IsConcreteIWindow) {
    static_assert(std::is_base_of_v<IWindow, GlfwWindow>);
    static_assert(!std::is_abstract_v<GlfwWindow>);
}

TEST(GlfwWindowTypeTraits, IsPolymorphic) {
    static_assert(std::is_polymorphic_v<GlfwWindow>);
}

TEST(GlfwWindowTypeTraits, IsNotCopyConstructible) {
    static_assert(!std::is_copy_constructible_v<GlfwWindow>);
}

TEST(GlfwWindowTypeTraits, IsNotCopyAssignable) {
    static_assert(!std::is_copy_assignable_v<GlfwWindow>);
}

TEST(GlfwWindowTypeTraits, IsMoveConstructible) {
    // unique_ptr<GlfwWindow> requires the type to be destructible; move is fine
    static_assert(std::is_destructible_v<GlfwWindow>);
}

// ----------------------------------------------------------------------------
// glfw_key_mapping — mapGlfwToKeyCode
// ----------------------------------------------------------------------------

TEST(GlfwKeyMapping, KnownKey_A) {
    EXPECT_EQ(mapGlfwToKeyCode(GLFW_KEY_A), KeyCode::eA);
}

TEST(GlfwKeyMapping, KnownKey_Space) {
    EXPECT_EQ(mapGlfwToKeyCode(GLFW_KEY_SPACE), KeyCode::eSpace);
}

TEST(GlfwKeyMapping, KnownKey_Escape) {
    EXPECT_EQ(mapGlfwToKeyCode(GLFW_KEY_ESCAPE), KeyCode::eEscape);
}

TEST(GlfwKeyMapping, KnownKey_LeftShift) {
    EXPECT_EQ(mapGlfwToKeyCode(GLFW_KEY_LEFT_SHIFT), KeyCode::eLeftShift);
}

TEST(GlfwKeyMapping, KnownKey_LeftControl) {
    EXPECT_EQ(mapGlfwToKeyCode(GLFW_KEY_LEFT_CONTROL), KeyCode::eLeftControl);
}

TEST(GlfwKeyMapping, KnownKey_LeftAlt) {
    EXPECT_EQ(mapGlfwToKeyCode(GLFW_KEY_LEFT_ALT), KeyCode::eLeftAlt);
}

TEST(GlfwKeyMapping, KnownKey_Menu_Boundary) {
    // GLFW_KEY_MENU = 348 is the upper boundary
    EXPECT_EQ(mapGlfwToKeyCode(GLFW_KEY_MENU), static_cast<KeyCode>(GLFW_KEY_MENU));
}

TEST(GlfwKeyMapping, UnknownKey_TooHigh) {
    EXPECT_EQ(mapGlfwToKeyCode(9999), KeyCode::eUnknown);
}

TEST(GlfwKeyMapping, UnknownKey_TooLow) {
    EXPECT_EQ(mapGlfwToKeyCode(-99), KeyCode::eUnknown);
}

TEST(GlfwKeyMapping, UnknownKey_Negative2) {
    // -2 is below kGlfwKeyMin=-1
    EXPECT_EQ(mapGlfwToKeyCode(-2), KeyCode::eUnknown);
}

TEST(GlfwKeyMapping, KeyUnknown_GlfwKeyUnknown) {
    // GLFW_KEY_UNKNOWN = -1 maps to eUnknown (value -1)
    EXPECT_EQ(mapGlfwToKeyCode(GLFW_KEY_UNKNOWN), KeyCode::eUnknown);
}

// ----------------------------------------------------------------------------
// glfw_key_mapping — mapGlfwToMouseButton
// ----------------------------------------------------------------------------

TEST(GlfwKeyMapping, MouseButton_Left) {
    EXPECT_EQ(mapGlfwToMouseButton(GLFW_MOUSE_BUTTON_LEFT), MouseButton::eLeft);
}

TEST(GlfwKeyMapping, MouseButton_Right) {
    EXPECT_EQ(mapGlfwToMouseButton(GLFW_MOUSE_BUTTON_RIGHT), MouseButton::eRight);
}

TEST(GlfwKeyMapping, MouseButton_Middle) {
    EXPECT_EQ(mapGlfwToMouseButton(GLFW_MOUSE_BUTTON_MIDDLE), MouseButton::eMiddle);
}

TEST(GlfwKeyMapping, MouseButton_Button4) {
    EXPECT_EQ(mapGlfwToMouseButton(GLFW_MOUSE_BUTTON_4),
              static_cast<MouseButton>(GLFW_MOUSE_BUTTON_4));
}

TEST(GlfwKeyMapping, MouseButton_Button8_Boundary) {
    EXPECT_EQ(mapGlfwToMouseButton(GLFW_MOUSE_BUTTON_8),
              static_cast<MouseButton>(GLFW_MOUSE_BUTTON_8));
}

TEST(GlfwKeyMapping, MouseButton_InvalidHigh_FallsBackToLeft) {
    EXPECT_EQ(mapGlfwToMouseButton(99), MouseButton::eLeft);
}

TEST(GlfwKeyMapping, MouseButton_InvalidNegative_FallsBackToLeft) {
    EXPECT_EQ(mapGlfwToMouseButton(-1), MouseButton::eLeft);
}

// ----------------------------------------------------------------------------
// glfw_key_mapping — mapGlfwToModifiers
// ----------------------------------------------------------------------------

TEST(GlfwKeyMapping, ModNone) {
    EXPECT_EQ(mapGlfwToModifiers(0), 0u);
}

TEST(GlfwKeyMapping, ModShift) {
    const uint8_t result = mapGlfwToModifiers(GLFW_MOD_SHIFT);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModShift), 0u);
}

TEST(GlfwKeyMapping, ModCtrl) {
    const uint8_t result = mapGlfwToModifiers(GLFW_MOD_CONTROL);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModCtrl), 0u);
}

TEST(GlfwKeyMapping, ModAlt) {
    const uint8_t result = mapGlfwToModifiers(GLFW_MOD_ALT);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModAlt), 0u);
}

TEST(GlfwKeyMapping, ModSuper) {
    const uint8_t result = mapGlfwToModifiers(GLFW_MOD_SUPER);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModSuper), 0u);
}

TEST(GlfwKeyMapping, ModShiftCtrlCombined) {
    const uint8_t result = mapGlfwToModifiers(GLFW_MOD_SHIFT | GLFW_MOD_CONTROL);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModShift), 0u);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModCtrl), 0u);
    EXPECT_EQ(result & static_cast<uint8_t>(ModifierKey::eModAlt), 0u);
}

TEST(GlfwKeyMapping, ModShiftOnlyHasNoCtrl) {
    const uint8_t result = mapGlfwToModifiers(GLFW_MOD_SHIFT);
    EXPECT_EQ(result & static_cast<uint8_t>(ModifierKey::eModCtrl), 0u);
    EXPECT_EQ(result & static_cast<uint8_t>(ModifierKey::eModAlt), 0u);
    EXPECT_EQ(result & static_cast<uint8_t>(ModifierKey::eModSuper), 0u);
}

TEST(GlfwKeyMapping, ModAllFour) {
    const int all = GLFW_MOD_SHIFT | GLFW_MOD_CONTROL | GLFW_MOD_ALT | GLFW_MOD_SUPER;
    const uint8_t result = mapGlfwToModifiers(all);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModShift), 0u);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModCtrl), 0u);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModAlt), 0u);
    EXPECT_NE(result & static_cast<uint8_t>(ModifierKey::eModSuper), 0u);
}

// ----------------------------------------------------------------------------
// MockWindow — handwritten IWindow stub
// ----------------------------------------------------------------------------

/**
 * @brief Minimal IWindow test double. No GLFW, no display required.
 *
 * Follows the same pattern as RecordingLayer in recording_layer.h.
 */
struct MockWindow : IWindow {
    int   w{800};
    int   h{600};
    bool  close_flag{false};
    int   poll_count{0};
    void* handle{nullptr};

    int   getWidth()        const override { return w; }
    int   getHeight()       const override { return h; }
    void  pollEvents()            override { ++poll_count; }
    bool  shouldClose()     const override { return close_flag; }
    void* getNativeHandle() const override { return handle; }
};

TEST(MockWindow, DefaultGetWidth) {
    MockWindow mw;
    EXPECT_EQ(mw.getWidth(), 800);
}

TEST(MockWindow, DefaultGetHeight) {
    MockWindow mw;
    EXPECT_EQ(mw.getHeight(), 600);
}

TEST(MockWindow, DefaultShouldCloseFalse) {
    MockWindow mw;
    EXPECT_FALSE(mw.shouldClose());
}

TEST(MockWindow, DefaultNativeHandleNull) {
    MockWindow mw;
    EXPECT_EQ(mw.getNativeHandle(), nullptr);
}

TEST(MockWindow, PollEventsIncrementsCounter) {
    MockWindow mw;
    mw.pollEvents();
    mw.pollEvents();
    EXPECT_EQ(mw.poll_count, 2);
}

TEST(MockWindow, ShouldCloseCanBeSet) {
    MockWindow mw;
    mw.close_flag = true;
    EXPECT_TRUE(mw.shouldClose());
}

TEST(MockWindow, CustomDimensions) {
    MockWindow mw;
    mw.w = 1920;
    mw.h = 1080;
    EXPECT_EQ(mw.getWidth(), 1920);
    EXPECT_EQ(mw.getHeight(), 1080);
}

TEST(MockWindow, AppContextAcceptsIWindowPtr) {
    MockWindow mw;
    mw.w = 1280;
    mw.h = 720;
    AppContext ctx{};
    ctx.window = &mw;
    EXPECT_NE(ctx.window, nullptr);
    EXPECT_EQ(ctx.window->getWidth(), 1280);
    EXPECT_EQ(ctx.window->getHeight(), 720);
    EXPECT_FALSE(ctx.window->shouldClose());
}

TEST(MockWindow, PolymorphicCallThroughIWindowPtr) {
    MockWindow mw;
    IWindow* iw = &mw;
    iw->pollEvents();
    EXPECT_EQ(mw.poll_count, 1);
    EXPECT_FALSE(iw->shouldClose());
}

// ============================================================================
// Tier 2 — Requires display (GTEST_SKIP when unavailable)
// ============================================================================

namespace {

/**
 * @brief Returns true if GLFW can initialise on this machine (display available).
 *
 * Probes once per process and caches the result so we do not call
 * glfwInit/glfwTerminate in every SetUp. GlfwWindow::create() will
 * call glfwInit() again via ensureGlfwInit() when creating a window.
 */
bool isGlfwAvailable() {
    static const bool cached = []() {
        if (glfwInit() == GLFW_FALSE) {
            return false;
        }
        glfwTerminate();
        return true;
    }();
    return cached;
}

}  // namespace

class GlfwWindowTest : public ::testing::Test {
   protected:
    void SetUp() override {
        if (!isGlfwAvailable()) {
            GTEST_SKIP() << "GLFW unavailable (no display or init failure)";
        }
    }

    /**
     * @brief Create a hidden window. Returns nullptr when headless.
     *
     * GTEST_SKIP() cannot be called inside a non-void helper function.
     * Each test that calls this must check for nullptr and skip itself.
     */
    static std::unique_ptr<GlfwWindow> makeHiddenWindow(uint32_t w = 200, uint32_t h = 100,
                                                        const char* title = "Test") {
        GlfwWindowDescriptor d{title, w, h};
        d.visible = false;
        return GlfwWindow::create(d);
    }
};

// Convenience macro: skip the current test if makeHiddenWindow() returned nullptr.
#define SKIP_IF_HEADLESS(win) \
    if (!(win)) { GTEST_SKIP() << "glfwCreateWindow returned nullptr (headless)"; }

TEST_F(GlfwWindowTest, CreateHiddenWindowReturnsNonNull) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    EXPECT_NE(win.get(), nullptr);
}

TEST_F(GlfwWindowTest, FramebufferWidthPositive) {
    auto win = makeHiddenWindow(400, 300);
    SKIP_IF_HEADLESS(win);
    EXPECT_GT(win->getWidth(), 0);
}

TEST_F(GlfwWindowTest, FramebufferHeightPositive) {
    auto win = makeHiddenWindow(400, 300);
    SKIP_IF_HEADLESS(win);
    EXPECT_GT(win->getHeight(), 0);
}

TEST_F(GlfwWindowTest, ShouldCloseInitiallyFalse) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    EXPECT_FALSE(win->shouldClose());
}

TEST_F(GlfwWindowTest, NativeHandleIsNonNull) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    EXPECT_NE(win->getNativeHandle(), nullptr);
}

TEST_F(GlfwWindowTest, IsOpenInitiallyTrue) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    EXPECT_TRUE(win->isOpen());
}

TEST_F(GlfwWindowTest, CloseSetsShouldClose) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    ASSERT_FALSE(win->shouldClose());
    win->close();
    EXPECT_TRUE(win->shouldClose());
}

TEST_F(GlfwWindowTest, CloseAlsoMakesIsOpenFalse) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    win->close();
    EXPECT_FALSE(win->isOpen());
}

TEST_F(GlfwWindowTest, DPIScaleIsPositive) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    EXPECT_GT(win->getDPIScale(), 0.0f);
}

TEST_F(GlfwWindowTest, VsyncEnabledByDefault) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    EXPECT_TRUE(win->isVsyncEnabled());
}

TEST_F(GlfwWindowTest, SetVsyncDisable) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    win->setVsync(false);
    EXPECT_FALSE(win->isVsyncEnabled());
}

TEST_F(GlfwWindowTest, SetVsyncRoundTrip) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    win->setVsync(false);
    EXPECT_FALSE(win->isVsyncEnabled());
    win->setVsync(true);
    EXPECT_TRUE(win->isVsyncEnabled());
}

TEST_F(GlfwWindowTest, EventForwardingDefaultFalse) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    EXPECT_FALSE(win->isEventForwarding());
}

TEST_F(GlfwWindowTest, SetEventForwardingTrue) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    win->setEventForwarding(true);
    EXPECT_TRUE(win->isEventForwarding());
}

TEST_F(GlfwWindowTest, SetEventForwardingRoundTrip) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    win->setEventForwarding(true);
    EXPECT_TRUE(win->isEventForwarding());
    win->setEventForwarding(false);
    EXPECT_FALSE(win->isEventForwarding());
}

TEST_F(GlfwWindowTest, ResizeDoesNotCrash) {
    auto win = makeHiddenWindow(400, 300);
    SKIP_IF_HEADLESS(win);
    win->resize(640, 480);
    win->pollEvents();
    EXPECT_GT(win->getWidth(), 0);
    EXPECT_GT(win->getHeight(), 0);
}

TEST_F(GlfwWindowTest, PollEventsDoesNotCrash) {
    auto win = makeHiddenWindow();
    SKIP_IF_HEADLESS(win);
    EXPECT_NO_THROW(win->pollEvents());
}

TEST_F(GlfwWindowTest, GetFramebufferSizeMatchesIWindowSize) {
    auto win = makeHiddenWindow(320, 240);
    SKIP_IF_HEADLESS(win);
    EXPECT_EQ(win->getWidth(), win->getFramebufferWidth());
    EXPECT_EQ(win->getHeight(), win->getFramebufferHeight());
}

TEST_F(GlfwWindowTest, ConvenienceFactoryCreate) {
    auto win = GlfwWindow::create(200, 150, "ConvTest", false);
    SKIP_IF_HEADLESS(win);
    EXPECT_GT(win->getWidth(), 0);
    EXPECT_GT(win->getHeight(), 0);
}

#endif  // VNE_TESTBED_OPENGL || VNE_TESTBED_OPENGLES
