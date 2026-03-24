#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 *
 * ----------------------------------------------------------------------
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "debug_renderer.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES."
#endif

#include "vertexnova/testbed/debug_draw.h"
#include "vertexnova/testbed/renderer/irenderer.h"

#include "vertexnova/math/core/core.h"

#include <memory>
#include <string_view>
#include <vector>

namespace vne {
namespace testbed {
namespace gl {
class Shader;
class VertexBuffer;
class VertexArray;
}  // namespace gl
}  // namespace testbed
}  // namespace vne

namespace vne {
namespace testbed {

/**
 * @class DebugRenderer
 * @brief IDebugDraw backed by batched OpenGL line rendering; implements IRenderer.
 *
 * Each vertex is { pos.x, pos.y, pos.z, col.r, col.g, col.b }. Primitives are
 * accumulated and uploaded on flush().
 */
class DebugRenderer : public IDebugDraw, public IRenderer {
   public:
    DebugRenderer() = default;
    ~DebugRenderer() override;

    DebugRenderer(const DebugRenderer&) = delete;
    DebugRenderer& operator=(const DebugRenderer&) = delete;

    [[nodiscard]] bool init(IRenderDevice* device) override;
    void shutdown() override;

    void setViewProjectionMatrix(const vne::math::Mat4f& vp) override;

    void line(vne::math::Vec3f from, vne::math::Vec3f to, vne::math::Vec3f color) override;
    void aabb(DebugAabb box, vne::math::Vec3f color) override;
    void text(vne::math::Vec3f pos, std::string_view label) override {
        (void)pos;
        (void)label;
    }
    void flush() override;

   private:
    static constexpr std::size_t kMaxLineVertices = 65536u;
    static constexpr std::size_t kFloatsPerVertex = 6u;

    std::vector<float> vertex_data_;
    std::unique_ptr<gl::Shader> shader_;
    std::unique_ptr<gl::VertexBuffer> vbo_;
    std::unique_ptr<gl::VertexArray> vao_;
    vne::math::Mat4f vp_matrix_{};
    bool initialized_{false};
};

}  // namespace testbed
}  // namespace vne
