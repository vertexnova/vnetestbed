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

#include "vertexnova/testbed/gl/draw.h"
#include "vertexnova/testbed/gl/vertex_array.h"

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif

namespace vne {
namespace testbed {
namespace gl {

namespace {

GLenum drawModeToGL(DrawMode mode) {
    switch (mode) {
        case DrawMode::eTriangles:
            return GL_TRIANGLES;
        case DrawMode::eLines:
            return GL_LINES;
        case DrawMode::eLineStrip:
            return GL_LINE_STRIP;
        case DrawMode::ePoints:
            return GL_POINTS;
    }
    return GL_TRIANGLES;
}

}  // namespace

void draw(const VertexArray& vao, DrawMode mode, std::size_t vertex_count) {
    vao.bind();
    const GLenum gl_mode = drawModeToGL(mode);

    if (vao.getIndexCount() > 0u) {
        glDrawElements(gl_mode, static_cast<GLsizei>(vao.getIndexCount()), GL_UNSIGNED_INT, nullptr);
    } else {
        glDrawArrays(gl_mode, 0, static_cast<GLsizei>(vertex_count));
    }

    vao.unbind();
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
