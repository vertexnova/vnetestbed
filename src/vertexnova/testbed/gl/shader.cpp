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

#include "vertexnova/testbed/gl/shader.h"

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif

#include <cstdio>
#include <fstream>
#include <sstream>

namespace vne {
namespace testbed {
namespace gl {

namespace {

std::string readFile(const char* path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        std::fprintf(stderr, "[Shader] Failed to open file: %s\n", path);
        return {};
    }
    std::ostringstream oss;
    oss << file.rdbuf();
    return oss.str();
}

}  // namespace

// ---------------------------------------------------------------------------
// Construction / destruction
// ---------------------------------------------------------------------------

Shader Shader::fromFile(const char* vert_path, const char* frag_path) {
    std::string vert_src = readFile(vert_path);
    std::string frag_src = readFile(frag_path);
    if (vert_src.empty() || frag_src.empty()) {
        return Shader("", "");
    }
    return Shader(vert_src.c_str(), frag_src.c_str());
}

unsigned int Shader::compileStage(unsigned int type, const char* src) {
    unsigned int id = glCreateShader(type);
    glShaderSource(id, 1, &src, nullptr);
    glCompileShader(id);

    int ok = 0;
    glGetShaderiv(id, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(id, 512, nullptr, log);
        const char* stage = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        std::fprintf(stderr, "[Shader] %s compile error:\n%s\n", stage, log);
        glDeleteShader(id);
        return 0u;
    }
    return id;
}

Shader::Shader(const char* vert_src, const char* frag_src) {
    unsigned int vs = compileStage(GL_VERTEX_SHADER, vert_src);
    unsigned int fs = compileStage(GL_FRAGMENT_SHADER, frag_src);
    if (vs == 0u || fs == 0u) {
        glDeleteShader(vs);
        glDeleteShader(fs);
        return;
    }

    program_id_ = glCreateProgram();
    glAttachShader(program_id_, vs);
    glAttachShader(program_id_, fs);
    glLinkProgram(program_id_);

    int ok = 0;
    glGetProgramiv(program_id_, GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetProgramInfoLog(program_id_, 512, nullptr, log);
        std::fprintf(stderr, "[Shader] link error:\n%s\n", log);
        glDeleteProgram(program_id_);
        program_id_ = 0u;
    }

    glDeleteShader(vs);
    glDeleteShader(fs);
}

Shader::~Shader() {
    if (program_id_ != 0u) {
        glDeleteProgram(program_id_);
    }
}

// ---------------------------------------------------------------------------
// Bind / unbind
// ---------------------------------------------------------------------------

void Shader::bind() const {
    glUseProgram(program_id_);
}

void Shader::unbind() const {
    glUseProgram(0);
}

// ---------------------------------------------------------------------------
// Uniform setters
// ---------------------------------------------------------------------------

void Shader::setInt(const char* name, int value) const {
    glUniform1i(glGetUniformLocation(program_id_, name), value);
}

void Shader::setFloat(const char* name, float value) const {
    glUniform1f(glGetUniformLocation(program_id_, name), value);
}

void Shader::setVec3(const char* name, const vne::math::Vec3f& v) const {
    glUniform3f(glGetUniformLocation(program_id_, name), v[0], v[1], v[2]);
}

void Shader::setVec4(const char* name, const vne::math::Vec4f& v) const {
    glUniform4f(glGetUniformLocation(program_id_, name), v[0], v[1], v[2], v[3]);
}

void Shader::setMat4(const char* name, const vne::math::Mat4f& m) const {
    // Mat4f stores columns as std::array<Vec4f, 4>; Vec4f has std::array<float, 4>.
    // Both are standard-layout with no padding, so &columns[0][0] gives a valid
    // pointer to 16 contiguous floats in column-major order.
    glUniformMatrix4fv(glGetUniformLocation(program_id_, name), 1, GL_FALSE, &m.columns[0][0]);
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
