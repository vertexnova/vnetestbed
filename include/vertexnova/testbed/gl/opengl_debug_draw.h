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
 * @file gl/opengl_debug_draw.h
 * @brief IDebugDraw implementation backed by raw OpenGL line rendering.
 *
 * All draw calls are batched per frame.  Call flush() once — typically at
 * the end of onRender() — to upload and draw all queued primitives.
 *
 * Call setViewProjectionMatrix() with the scene camera's VP matrix before
 * flush() so lines appear in the correct world position.
 *
 * text() is a no-op in this implementation (ImGui integration is planned).
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "opengl_debug_draw.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES. Build with OpenGL or OpenGL ES enabled."
#endif

#include "vertexnova/testbed/debug_draw.h"
#include "vertexnova/testbed/gl/shader.h"
#include "vertexnova/testbed/gl/vertex_buffer.h"
#include "vertexnova/testbed/gl/vertex_array.h"

#include "vertexnova/math/core/core.h"

#include <memory>
#include <vector>

namespace vne {
namespace testbed {
namespace gl {

/**
 * @class OpenGLDebugDraw
 * @brief IDebugDraw backed by a dynamic OpenGL line-list VBO.
 *
 * Each vertex is { pos.x, pos.y, pos.z, col.r, col.g, col.b } (6 floats).
 * Primitives are accumulated in a CPU-side vector and GPU-uploaded on flush().
 */
class OpenGLDebugDraw : public IDebugDraw {
   public:
    OpenGLDebugDraw() = default;
    ~OpenGLDebugDraw() override;

    OpenGLDebugDraw(const OpenGLDebugDraw&) = delete;
    OpenGLDebugDraw& operator=(const OpenGLDebugDraw&) = delete;

    /**
     * @brief Allocate GPU resources (shader + VBO + VAO).
     *
     * Call after a valid OpenGL context exists (i.e. after
     * OpenGLRenderAdapter::init()).
     * @return true on success.
     */
    bool init();

    /** @brief Release all GPU resources. */
    void shutdown();

    /**
     * @brief Set the view-projection matrix used to transform lines to clip space.
     *
     * Typically called once per frame from the scene layer after updating
     * the camera.  Takes effect on the next flush().
     */
    void setViewProjectionMatrix(const vne::math::Mat4f& vp);

    // -----------------------------------------------------------------------
    // IDebugDraw
    // -----------------------------------------------------------------------

    void line(vne::math::Vec3f from, vne::math::Vec3f to, vne::math::Vec3f color) override;

    void aabb(DebugAabb box, vne::math::Vec3f color) override;

    /** @brief No-op in this implementation (text requires ImGui or a font atlas). */
    void text(vne::math::Vec3f pos, std::string_view label) override {
        (void)pos;
        (void)label;
    }

    /**
     * @brief Upload all queued primitives to the GPU and issue draw calls.
     *
     * Clears the CPU-side queue after drawing.  The bound VAO/shader are
     * restored to 0 after the call.
     */
    void flush() override;

   private:
    static constexpr std::size_t kMaxLineVertices = 65536u;  ///< 32 K lines
    static constexpr std::size_t kFloatsPerVertex = 6u;      ///< xyz + rgb

    std::vector<float> vertex_data_;
    std::unique_ptr<Shader> shader_;
    std::unique_ptr<VertexBuffer> vbo_;
    std::unique_ptr<VertexArray> vao_;
    vne::math::Mat4f vp_matrix_{};
    bool initialized_{false};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
