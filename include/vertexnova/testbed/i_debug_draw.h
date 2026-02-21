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

#include <string_view>

namespace vne {
namespace testbed_ns {

/** @brief Minimal 3-component float vector used by IDebugDraw. */
struct Vec3 {
    float x{0.0F};
    float y{0.0F};
    float z{0.0F};
};

/** @brief Axis-aligned bounding box used by IDebugDraw. */
struct DebugAabb {
    Vec3 min;
    Vec3 max;
};

/**
 * @brief Immediate-mode debug draw interface.
 *
 * Both render adapters implement this interface so that subsystem
 * plugins (scene inspector, gizmos, picking rays) can visualise
 * geometry without depending on a specific rendering backend.
 *
 * Typical usage per frame:
 *   debug_draw->line(...);
 *   debug_draw->aabb(...);
 *   debug_draw->flush();   // upload and draw all queued primitives
 */
class IDebugDraw {
public:
    virtual ~IDebugDraw() = default;

    /**
     * @brief Queue a line segment.
     * @param from  Start position in world space.
     * @param to    End position in world space.
     * @param color RGB colour (each component in [0, 1]).
     */
    virtual void line(Vec3 from, Vec3 to, Vec3 color) = 0;

    /**
     * @brief Queue an axis-aligned bounding box outline.
     * @param box   The AABB to draw.
     * @param color RGB colour (each component in [0, 1]).
     */
    virtual void aabb(DebugAabb box, Vec3 color) = 0;

    /**
     * @brief Queue a world-space text label.
     * @param pos   Position in world space.
     * @param label Text to display.
     */
    virtual void text(Vec3 pos, std::string_view label) = 0;

    /** @brief Upload all queued primitives and issue draw calls. */
    virtual void flush() = 0;
};

}  // namespace testbed_ns
}  // namespace vne
