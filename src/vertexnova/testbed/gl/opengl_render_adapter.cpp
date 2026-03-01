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

#include "vertexnova/testbed/gl/opengl_render_adapter.h"

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif
#include <GLFW/glfw3.h>

#include <cstdio>

namespace vne {
namespace testbed {
namespace gl {

OpenGLRenderAdapter::~OpenGLRenderAdapter() {
    if (initialized_) {
        initialized_ = false;
        context_.destroy();
    }
}

void OpenGLRenderAdapter::setClearColor(float r, float g, float b, float a) {
    clear_color_ = {r, g, b, a};
}

bool OpenGLRenderAdapter::init(void* window_handle) {
    if (initialized_) {
        return true;
    }

    if (!context_.create(window_handle)) {
        return false;
    }

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    initialized_ = true;
    return true;
}

void OpenGLRenderAdapter::beginFrame() {
    if (!initialized_) {
        return;
    }

    // Update viewport to match the current framebuffer size so the frame
    // adapts to window resizes without any extra plumbing.
    if (context_.getWindowHandle()) {
        int w = 0;
        int h = 0;
        glfwGetFramebufferSize(static_cast<GLFWwindow*>(context_.getWindowHandle()), &w, &h);
        if (w > 0 && h > 0) {
            glViewport(0, 0, w, h);
        }
    }

    glClearColor(clear_color_[0], clear_color_[1], clear_color_[2], clear_color_[3]);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRenderAdapter::shutdown() {
    initialized_ = false;
    context_.destroy();
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
