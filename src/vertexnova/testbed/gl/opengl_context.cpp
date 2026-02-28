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

#include "vertexnova/testbed/gl/opengl_context.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <cstdio>

namespace vne {
namespace testbed {
namespace gl {

OpenGLContext::~OpenGLContext() {
    destroy();
}

bool OpenGLContext::create(void* window_handle) {
    if (window_handle == nullptr) {
        return false;
    }
    if (window_handle_ != nullptr) {
        return true;  // already created
    }
    window_handle_ = window_handle;

    if (!makeCurrent()) {
        window_handle_ = nullptr;
        return false;
    }

    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::fprintf(stderr, "[OpenGLContext] glad failed to load OpenGL.\n");
        window_handle_ = nullptr;
        return false;
    }
    glad_loaded_ = true;
    return true;
}

bool OpenGLContext::makeCurrent() {
    if (window_handle_ == nullptr) {
        return false;
    }
    glfwMakeContextCurrent(static_cast<GLFWwindow*>(window_handle_));
    return true;
}

void OpenGLContext::release() {
    glfwMakeContextCurrent(nullptr);
}

void OpenGLContext::destroy() {
    if (window_handle_ != nullptr) {
        release();
        window_handle_ = nullptr;
        glad_loaded_ = false;
    }
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
