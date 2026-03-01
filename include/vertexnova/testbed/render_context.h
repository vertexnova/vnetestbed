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
 * @file render_context.h
 * @brief Frame info and render context passed to layers during rendering.
 *
 * Same structure pattern as samples/core RenderContext_C; vnetestbed subset
 * has frame_info and debug_draw (no CrossGL encoder/session).
 */

namespace vne {
namespace testbed {

class IDebugDraw;

/** @brief Per-frame information passed to layers. */
struct FrameInfo {
    int width{0};
    int height{0};
    float dt{0.0F};
};

/**
 * @struct RenderContext
 * @brief Context passed to layers during onBeginRender, onRender, onGuiBegin, onGuiRender, onGuiEnd.
 *
 * Provides frame dimensions, delta time, and optional debug draw. Layers receive
 * const RenderContext& and can use debug_draw for geometry visualization.
 *
 * When rendering to multiple viewports, active_viewport_index identifies which
 * viewport (0..N-1) is being rendered; scene layers use this to select the camera.
 */
struct RenderContext {
    FrameInfo frame_info{};
    IDebugDraw* debug_draw{nullptr};
    int active_viewport_index{0};  ///< 0 for single viewport; 0..N-1 for multi-viewport
};

}  // namespace testbed
}  // namespace vne
