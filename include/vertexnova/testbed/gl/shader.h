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
 * @file gl/shader.h
 * @brief RAII wrapper for an OpenGL shader program.
 *
 * Compiles a vertex + fragment shader pair and links them into a program.
 * Provides typed uniform setters so callers never need to touch OpenGL
 * uniform API directly.
 *
 * Only available when VNE_TESTBED_OPENGL is defined (i.e. glad is present).
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "shader.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES. Build with OpenGL or OpenGL ES enabled."
#endif

#include "vertexnova/math/core/core.h"

#include <filesystem>
#include <string>

namespace vne {
namespace testbed {
namespace gl {

/**
 * @class Shader
 * @brief RAII OpenGL shader program (vertex + fragment).
 *
 * Compiles and links on construction.  Call isValid() to check success.
 * The program is deleted in the destructor; it cannot be copied or moved.
 */
class Shader {
   public:
    /**
     * @brief Compile and link a shader program from GLSL source strings.
     * @param vert_src  Null-terminated GLSL vertex shader source.
     * @param frag_src  Null-terminated GLSL fragment shader source.
     *
     * Compilation errors are logged.  isValid() returns false if
     * compilation or linking failed.
     */
    Shader(const char* vert_src, const char* frag_src);

    /**
     * @brief Load and compile a shader program from two GLSL files.
     *
     * Reads vert_path and frag_path, then compiles and links. Returns an
     * invalid Shader (isValid() == false) if a file could not be read or
     * compilation/linking failed.
     *
     * @param vert_path Path to vertex shader source file.
     * @param frag_path Path to fragment shader source file.
     */
    Shader(const std::filesystem::path& vert_path, const std::filesystem::path& frag_path);

    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&&) = delete;
    Shader& operator=(Shader&&) = delete;

    /** @brief Bind this program as the current OpenGL program. */
    void bind() const;

    /** @brief Unbind (restore default program 0). */
    void unbind() const;

    /** @brief Returns true if the program compiled and linked successfully. */
    [[nodiscard]] bool isValid() const { return program_id_ != 0u; }

    // -----------------------------------------------------------------------
    // Uniform setters — all require bind() to have been called first.
    // -----------------------------------------------------------------------

    /** @brief Set a scalar int uniform. */
    void setInt(const char* name, int value) const;

    /** @brief Set a scalar float uniform. */
    void setFloat(const char* name, float value) const;

    /** @brief Set a vec3 uniform. */
    void setVec3(const char* name, const vne::math::Vec3f& v) const;

    /** @brief Set a vec4 uniform. */
    void setVec4(const char* name, const vne::math::Vec4f& v) const;

    /** @brief Set a column-major mat4 uniform. */
    void setMat4(const char* name, const vne::math::Mat4f& m) const;

   private:
    uint32_t program_id_{0u};

    /** @brief Compile and link from source; sets program_id_ or leaves 0. */
    void compileAndLink(const char* vert_src, const char* frag_src);

    /** @brief Compile a single shader stage; returns 0 on error. */
    static uint32_t compileStage(uint32_t type, const char* src);
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
