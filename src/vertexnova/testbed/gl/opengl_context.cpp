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

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif
#include <GLFW/glfw3.h>

#include <vertexnova/logging/logging.h>

CREATE_VNE_LOGGER_CATEGORY("vnetestbed.gl")

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

#if defined(VNE_TESTBED_OPENGL)
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        VNE_LOG_ERROR << "glad failed to load OpenGL";
        window_handle_ = nullptr;
        return false;
    }
#elif defined(VNE_TESTBED_OPENGLES)
    if (!gladLoadGLES2Loader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        VNE_LOG_ERROR << "glad failed to load OpenGL ES";
        window_handle_ = nullptr;
        return false;
    }
#endif
    glad_loaded_ = true;
#if defined(VNE_TESTBED_OPENGL)
    VNE_LOG_INFO << "OpenGL context created";
#else
    VNE_LOG_INFO << "OpenGL ES context created";
#endif
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
