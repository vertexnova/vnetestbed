#pragma once
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
 * @file imgui/imgui_layer.h
 * @brief ImGui overlay layer with docking and viewports.
 *
 * Provides a shared UI for all demos: settings panel, viewport layout (1/2/4),
 * and dockspace. Uses Dear ImGui docking branch with GLFW + OpenGL3 backends.
 *
 * When VNE_TESTBED_IMGUI is not defined, only a forward declaration is provided.
 * Build with -DVNE_TESTBED_IMGUI to enable the full API.
 */

#if !defined(VNE_TESTBED_IMGUI)
// Stub: ImGuiLayer is unavailable when VNE_TESTBED_IMGUI is not defined.
// Build with -DVNE_TESTBED_IMGUI to enable. Safe for IDE indexers and doc generators.
namespace vne {
namespace testbed {
class ImGuiLayer;
}
}  // namespace vne
#else

namespace vne {
namespace testbed {
class ImGuiEventListener;
}
}  // namespace vne

#include "vertexnova/testbed/app_context.h"
#include "vertexnova/testbed/layer.h"
#include "vertexnova/testbed/render_context.h"

#include <array>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
#include "vertexnova/testbed/gl/framebuffer.h"
#endif

#include <imgui.h>

namespace vne {
namespace testbed {

namespace detail {

/** Screen-space hit rect for the Settings window when ImGui::Begin returns false. */
struct SettingsWindowScreenRect {
    bool valid{false};
    float min_x{0.f};
    float min_y{0.f};
    float max_x{0.f};
    float max_y{0.f};
};

/** Collapsed windows keep the title-bar rect; skipped (hidden tab) windows invalidate it. */
[[nodiscard]] inline SettingsWindowScreenRect makeSettingsWindowScreenRectWhenBeginSkipped(
    bool window_collapsed,
    float pos_x,
    float pos_y,
    float width,
    float height) noexcept {
    if (!window_collapsed) {
        return {};
    }
    return SettingsWindowScreenRect{true, pos_x, pos_y, pos_x + width, pos_y + height};
}

}  // namespace detail

/**
 * @class ImGuiLayer
 * @brief ImGui overlay with docking, viewports, settings panel, and viewport layout.
 *
 * Recommended to be added early in the layer stack so onGuiBegin runs before other
 * UI layers and onGuiEnd runs after layers that are added later in the stack. Handles
 * ImGui context lifecycle, GLFW + OpenGL3 backend init, and input forwarding.
 */
class ImGuiLayer : public ILayer {
   public:
    /**
     * @brief Viewport layout: number and arrangement of viewport windows.
     * eThree = left: two equal stacked; right: one full height (same height as left).
     */
    enum class ViewportLayout { eOne = 1, eTwo = 2, eThree = 3, eFour = 4 };

    /** @brief Number of viewport windows for the current layout (1, 2, 3, or 4). */
    [[nodiscard]] int getViewportCount() const;

    /**
     * @brief Callback for demo-specific settings (optional).
     */
    using SettingsCallback = std::function<void()>;

    /**
     * @brief Callback invoked immediately after ImGui::Image() for each viewport.
     *
     * Called with the 0-based viewport index. Use it to add drag-drop targets
     * (ImGui::BeginDragDropTarget / AcceptDragDropPayload) on the scene image.
     * Only called when the viewport window is open and has a valid FBO texture.
     */
    using ViewportOverlayCallback = std::function<void(int viewport_index)>;

    ImGuiLayer();
    ~ImGuiLayer() override;

    ImGuiLayer(const ImGuiLayer&) = delete;
    ImGuiLayer& operator=(const ImGuiLayer&) = delete;

    void onAttach(AppContext& ctx) override;
    void onDetach() override;

    void onBeginRender(const RenderContext& ctx) override;
    void onGuiBegin(const RenderContext& ctx) override;
    void onGuiRender(const RenderContext& ctx) override;
    void onGuiEnd(const RenderContext& ctx) override;

    /** @brief Set viewport layout (1, 2, 3, or 4 viewports). */
    void setViewportLayout(ViewportLayout layout) { viewport_layout_ = layout; }

    /** @brief Get current viewport layout. */
    [[nodiscard]] ViewportLayout getViewportLayout() const { return viewport_layout_; }

    /** @brief Set optional callback for demo-specific settings in the Settings panel. */
    void setSettingsCallback(SettingsCallback cb) { settings_callback_ = std::move(cb); }

    /**
     * @brief Set callback invoked after ImGui::Image() for each viewport (for drag-drop targets).
     * Pass nullptr to clear.
     */
    void setViewportOverlayCallback(ViewportOverlayCallback cb) { viewport_overlay_callback_ = std::move(cb); }

    /**
     * @brief Set the fixed pixel width of the Settings panel (default 320).
     *
     * The panel does not resize when the main window grows — only the viewport
     * area absorbs the extra space.  Call before or after initialization; the
     * dock layout is rebuilt on the next frame if the width changes.
     */
    void setSettingsPanelWidth(float width_px) {
        if (width_px != settings_panel_width_) {
            settings_panel_width_ = width_px;
            dock_layout_dirty_ = true;
        }
    }
    [[nodiscard]] float getSettingsPanelWidth() const { return settings_panel_width_; }

    /** @brief Whether scene should render to FBO for viewport display. */
#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    [[nodiscard]] bool useSceneFbo() const { return scene_fbo_ != nullptr && scene_fbo_->isValid(); }
#else
    [[nodiscard]] bool useSceneFbo() const { return false; }
#endif

    /** @brief Scene FBO texture id for ImGui::Image; 0 if not available. */
    [[nodiscard]] unsigned int getSceneTextureId() const;

    /** @brief Scene texture id for viewport index (multi-viewport); 0 if not available. */
    [[nodiscard]] unsigned int getSceneTextureId(int viewport_index) const;

    /** @brief Whether ImGui context and backends are initialized (for ImGuiEventListener). */
    [[nodiscard]] bool isInitialized() const { return initialized_; }

    /**
     * @brief Check if mouse is over any scene viewport window.
     * @param mouse_x Screen-space X (e.g. from GLFW or event)
     * @param mouse_y Screen-space Y
     * @return true if over Viewport / Viewport 1 / Viewport 2 / etc.
     */
    [[nodiscard]] bool isMouseOverSceneViewport(float mouse_x, float mouse_y) const;

    /**
     * @brief Map GLFW client-area mouse coordinates into ImGui screen space used by viewport_rects_.
     *
     * With ImGuiConfigFlags_ViewportsEnable (docking multi-viewport), Dear ImGui uses OS-absolute
     * coordinates (same as ImGui::GetCursorScreenPos). GLFW events use client-relative coordinates;
     * this adds glfwGetWindowPos for the main window so hit tests match after moving the window
     * between monitors. No-op when viewports are disabled or ImGui is not initialized.
     */
    void clientMouseToImGuiScreen(float& x, float& y) const;

    /**
     * @brief Get the viewport index under the mouse (0-based).
     * @param mouse_x Client-area X (e.g. glfwGetCursorPos / MouseMovedEvent)
     * @param mouse_y Client-area Y
     * @return 0..(N-1) for viewport index, or -1 if not over any scene viewport
     */
    [[nodiscard]] int getHoveredViewportIndex(float mouse_x, float mouse_y) const;

    /**
     * @brief Get the window title for a viewport index (e.g. "Viewport", "Viewport 1", "Viewport 2").
     * @param index 0-based viewport index
     * @return Title for the current layout (owned copy); empty string if index out of range.
     *         Caller may store or use the returned string without lifetime concerns.
     */
    [[nodiscard]] std::string getViewportName(int index) const;

    /**
     * @brief Get the screen-space rect of a viewport (min_x, min_y, max_x, max_y).
     * @param index 0-based viewport index
     * @param[out] min_x, min_y, max_x, max_y Rect in window coordinates
     * @return true if index valid and rect available
     */
    [[nodiscard]] bool getViewportRect(int index, float& min_x, float& min_y, float& max_x, float& max_y) const;

   private:
    void tryInitFromContext();  // Init ImGui; no-op if already initialized or no window
    void ensureSceneFbo(int width, int height);
    void ensureViewportFbos(int count, int width, int height);
    void setupDockLayout(ImGuiID dockspace_id, const ImVec2& size);

    void renderSettingsPanel(const RenderContext& ctx);
    void renderViewportWindows(const RenderContext& ctx);

    AppContext* app_ctx_{nullptr};
    ViewportLayout viewport_layout_{ViewportLayout::eOne};
    ViewportLayout last_dock_layout_{ViewportLayout::eOne};  // Track layout for dock rebuild
    ImVec2 last_dock_size_{0.0f, 0.0f};                      // Track size for rebuild on window resize
    SettingsCallback settings_callback_;
    ViewportOverlayCallback viewport_overlay_callback_;
    bool initialized_{false};
    float settings_panel_width_{320.0f};  ///< Fixed pixel width; viewport absorbs remaining space.
    float font_scale_{1.25f};             ///< Global font scale (style.FontScaleMain); user-adjustable in Settings.
    bool dock_layout_dirty_{false};       ///< True when settings_panel_width_ changed mid-session.

    /// FPS display: rolling average over last N frame times
    static constexpr size_t kFpsAverageFrames = 10;
    std::array<float, kFpsAverageFrames> fps_dt_buf_{};
    size_t fps_buf_index_{0};
    size_t fps_buf_filled_{0};
    float fps_dt_sum_{0.0f};

#if defined(VNE_TESTBED_OPENGL) || defined(VNE_TESTBED_OPENGLES)
    std::unique_ptr<gl::Framebuffer> scene_fbo_;
    int scene_fbo_width_{0};
    int scene_fbo_height_{0};
    std::vector<std::unique_ptr<gl::Framebuffer> > viewport_fbos_;
    int viewport_fbo_count_{0};
    int viewport_fbo_width_{0};
    int viewport_fbo_height_{0};
#endif

    /// Viewport window rects in screen space (min_x, min_y, max_x, max_y); updated each frame in renderViewportWindows
    std::vector<float> viewport_rects_;

    /// "Settings" window AABB in ImGui screen space; excludes it from scene viewport hit-testing so dragging sliders
    /// does not orbit the camera. Updated in renderSettingsPanel().
    bool settings_window_rect_valid_{false};
    float settings_screen_min_x_{0.f};
    float settings_screen_min_y_{0.f};
    float settings_screen_max_x_{0.f};
    float settings_screen_max_y_{0.f};

#if defined(VNE_TESTBED_EVENTS)
    std::shared_ptr<ImGuiEventListener> event_listener_;
#endif
};

}  // namespace testbed
}  // namespace vne

#endif  // VNE_TESTBED_IMGUI
