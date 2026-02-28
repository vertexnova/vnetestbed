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
 * @file gl/draw.h
 * @brief Draw helper: single entry point for indexed and non-indexed draw calls.
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#ifdef VNE_TESTBED_OPENGL

#include <cstddef>

namespace vne {
namespace testbed {
namespace gl {

class VertexArray;

/**
 * @brief Primitive type for draw calls.
 */
enum class DrawMode {
    Triangles,
    Lines,
    LineStrip,
    Points
};

/**
 * @brief Bind the VAO and issue one draw call (indexed or non-indexed).
 *
 * If the VAO has an index buffer set (getIndexCount() > 0), calls
 * glDrawElements with that count. Otherwise calls glDrawArrays with
 * @p vertex_count (caller must pass the vertex count for non-indexed).
 *
 * @param vao          Vertex array to bind and draw.
 * @param mode         Primitive mode (Triangles, Lines, etc.).
 * @param vertex_count Used only when the VAO has no index buffer; number of
 *                     vertices for glDrawArrays. Ignored when indexed.
 */
void draw(const VertexArray& vao, DrawMode mode, std::size_t vertex_count = 0u);

}  // namespace gl
}  // namespace testbed
}  // namespace vne

#endif  // VNE_TESTBED_OPENGL
