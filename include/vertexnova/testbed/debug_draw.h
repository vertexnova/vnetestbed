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
 * @file debug_draw.h
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

#include "vertexnova/math/core/core.h"

#include <string_view>

namespace vne {
namespace testbed {

/** @brief Axis-aligned bounding box used by IDebugDraw. */
struct DebugAabb {
    vne::math::Vec3f min{};
    vne::math::Vec3f max{};
};

/**
 * @brief Immediate-mode debug draw interface.
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
    virtual void line(vne::math::Vec3f from, vne::math::Vec3f to, vne::math::Vec3f color) = 0;

    /**
     * @brief Queue an axis-aligned bounding box outline.
     * @param box   The AABB to draw.
     * @param color RGB colour (each component in [0, 1]).
     */
    virtual void aabb(DebugAabb box, vne::math::Vec3f color) = 0;

    /**
     * @brief Queue a world-space text label.
     * @param pos   Position in world space.
     * @param label Text to display.
     */
    virtual void text(vne::math::Vec3f pos, std::string_view label) = 0;

    /**
     * @brief Set the view-projection matrix used to transform world-space
     *        primitives into clip space.
     *
     * Must be called once per frame, before flush(), after the camera
     * matrices have been updated.  Implementations that do not need an
     * explicit VP matrix (e.g. a CPU-side recorder) may implement this as
     * a no-op.
     *
     * @param vp Column-major view-projection matrix.
     */
    virtual void setViewProjectionMatrix(const vne::math::Mat4f& vp) = 0;

    /** @brief Upload all queued primitives and issue draw calls. */
    virtual void flush() = 0;
};

}  // namespace testbed
}  // namespace vne
