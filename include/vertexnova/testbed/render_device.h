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
 * @file render_device.h
 * @brief Backend-agnostic render device: resource factory + draw interface.
 *
 * IRenderDevice is the central swap point between OpenGL, CrossGL, WebGL,
 * and any future rendering backend.  Demo layers use only this interface;
 * they never include any gl/ header directly.
 *
 * Typical layer usage:
 * @code
 *   // onAttach
 *   shader_   = ctx.device->createShader(vert_src, frag_src);
 *   vbo_      = ctx.device->createVertexBuffer(verts, sizeof(verts));
 *   pipeline_ = ctx.device->createPipeline({shader_, {{3},{3}}, {}, {}, {}});
 *
 *   // onRender
 *   DebugGroupScope _{*ctx.device, "MyLayer"};
 *   ctx.device->setMat4(shader_, "u_VP", vp);
 *   ctx.device->draw(pipeline_, vbo_, 3, DrawMode::Triangles);
 *
 *   // onDetach
 *   ctx.device->destroy(pipeline_);
 *   ctx.device->destroy(vbo_);
 *   ctx.device->destroy(shader_);
 * @endcode
 */

#include "vertexnova/math/core/core.h"

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <initializer_list>
#include <sstream>
#include <vector>

namespace vne {
namespace testbed {

// ============================================================================
// Opaque resource handles
// ============================================================================

/** @brief Handle to a compiled shader program. */
struct ShaderHandle {
    uint32_t id{0};
    [[nodiscard]] bool isValid() const { return id != 0u; }
};

/** @brief Handle to a GPU vertex or index buffer. */
struct BufferHandle {
    uint32_t id{0};
    [[nodiscard]] bool isValid() const { return id != 0u; }
};

/** @brief Handle to a baked pipeline state (shader + layout + blend + depth + raster). */
struct PipelineHandle {
    uint32_t id{0};
    [[nodiscard]] bool isValid() const { return id != 0u; }
};

/** @brief Handle to a 2D texture. */
struct TextureHandle {
    uint32_t id{0};
    [[nodiscard]] bool isValid() const { return id != 0u; }
};

// ============================================================================
// Pipeline state descriptors
// ============================================================================

/** @brief Source / destination factor for blending. */
enum class BlendFactor {
    Zero,
    One,
    SrcAlpha,
    OneMinusSrcAlpha,
    DstAlpha,
    OneMinusDstAlpha,
    SrcColor,
    OneMinusSrcColor
};

/** @brief Blending equation. */
enum class BlendEquation { Add, Subtract, ReverseSubtract, Min, Max };

/** @brief Depth/stencil comparison function. */
enum class CompareFunc { Never, Less, Equal, LessEqual, Greater, NotEqual, GreaterEqual, Always };

/** @brief Face culling mode. */
enum class CullMode { None, Front, Back };

/** @brief Primitive topology for draw calls. */
enum class DrawMode { Triangles, Lines, LineStrip, Points };

// ---------------------------------------------------------------------------

/**
 * @struct BlendState
 * @brief Colour blending configuration.
 *
 * When @c enabled is false the GPU writes fragment colour directly without
 * blending.  Default: disabled (opaque).
 */
struct BlendState {
    bool enabled{false};
    BlendFactor src{BlendFactor::SrcAlpha};
    BlendFactor dst{BlendFactor::OneMinusSrcAlpha};
    BlendEquation eq{BlendEquation::Add};
};

/**
 * @struct DepthState
 * @brief Depth buffer configuration.
 *
 * Default: depth test ON, writes ON, compare Less (standard opaque geometry).
 */
struct DepthState {
    bool testEnabled{true};
    bool writeEnabled{true};
    CompareFunc func{CompareFunc::Less};
};

/**
 * @struct RasterizerState
 * @brief Polygon rasterization configuration.
 *
 * Default: back-face culling, filled polygons.
 */
struct RasterizerState {
    CullMode cull{CullMode::Back};
    bool wireframe{false};  ///< Not supported on OpenGL ES / WebGL.
};

// ---------------------------------------------------------------------------

/**
 * @struct VertexAttrib
 * @brief Describes one float-based vertex attribute in a buffer layout.
 *
 * Attributes are assigned shader locations 0, 1, 2, … in the order they
 * appear in the @c layout field of @c PipelineDesc.
 *
 * @param num_floats  Number of float components (1–4).
 * @param normalized  If true, values are normalized to [0, 1] (unused for
 *                    GL_FLOAT; useful for packed normals/colours).
 */
struct VertexAttrib {
    uint32_t num_floats{3};
    bool normalized{false};
};

/**
 * @struct PipelineDesc
 * @brief Full description of a baked pipeline state object.
 *
 * Pass to IRenderDevice::createPipeline().  The resulting PipelineHandle is
 * bound implicitly by every draw call that references it.
 */
struct PipelineDesc {
    ShaderHandle shader;               ///< Compiled shader program.
    std::vector<VertexAttrib> layout;  ///< Vertex attribute layout (location 0, 1, …).
    BlendState blend{};                ///< Blend state (default: disabled).
    DepthState depth{};                ///< Depth state (default: test+write, Less).
    RasterizerState rasterizer{};      ///< Rasterizer state (default: back-cull, fill).
};

// ============================================================================
// Texture descriptor
// ============================================================================

/** @brief Pixel format for 2D textures. */
enum class TextureFormat { RGBA8, RGB8, R8 };

/** @brief Descriptor for creating a 2D texture. */
struct TextureDesc {
    uint32_t width{0};
    uint32_t height{0};
    TextureFormat format{TextureFormat::RGBA8};
};

// ============================================================================
// IRenderDevice
// ============================================================================

/**
 * @class IRenderDevice
 * @brief Backend-agnostic GPU resource factory and draw interface.
 *
 * Concrete implementations:
 *  - OpenGLRenderDevice  (raw OpenGL 4.1 / OpenGL ES 3.0)
 *  - CrossGLRenderDevice (validates the CrossGL library)
 *
 * All resource handles are pool-allocated by the device.  Destroy each
 * handle via the matching destroy() overload before destroying the device.
 * Destroying a pipeline does not destroy its shader — manage those lifetimes
 * explicitly.
 */
class IRenderDevice {
   public:
    virtual ~IRenderDevice() = default;

    // -----------------------------------------------------------------------
    // Resource creation
    // -----------------------------------------------------------------------

    /**
     * @brief Compile and link a shader program from GLSL source strings.
     *
     * @param vert_src  Null-terminated vertex shader source.
     * @param frag_src  Null-terminated fragment shader source.
     * @return Valid ShaderHandle on success; {0} on compilation failure.
     */
    virtual ShaderHandle createShader(const char* vert_src, const char* frag_src) = 0;

    /**
     * @brief Compile and link a shader program from GLSL source files.
     *
     * Reads both files into strings and delegates to createShader().
     * Returns an invalid handle if either file cannot be opened or if
     * compilation / linking fails.
     *
     * @param vert_path  Path to vertex shader source file.
     * @param frag_path  Path to fragment shader source file.
     * @return Valid ShaderHandle on success; {0} on failure.
     */
    ShaderHandle createShaderFromFile(const std::filesystem::path& vert_path, const std::filesystem::path& frag_path) {
        auto readFile = [](const std::filesystem::path& p) -> std::string {
            std::ifstream f(p);
            if (!f.is_open()) {
                return {};
            }
            std::ostringstream ss;
            ss << f.rdbuf();
            return ss.str();
        };
        const std::string vert_src = readFile(vert_path);
        const std::string frag_src = readFile(frag_path);
        if (vert_src.empty() || frag_src.empty()) {
            return {};
        }
        return createShader(vert_src.c_str(), frag_src.c_str());
    }

    /**
     * @brief Allocate a GPU vertex buffer.
     *
     * @param data     Initial data pointer (may be nullptr for dynamic buffers).
     * @param bytes    Buffer size in bytes.
     * @param dynamic  If true, the buffer is optimised for per-frame uploads.
     * @return Valid BufferHandle on success; {0} on failure.
     */
    virtual BufferHandle createVertexBuffer(const void* data, uint32_t bytes, bool dynamic = false) = 0;

    /**
     * @brief Allocate a GPU index buffer (32-bit unsigned indices).
     *
     * @param data   Pointer to index data.
     * @param count  Number of indices.
     * @return Valid BufferHandle on success; {0} on failure.
     */
    virtual BufferHandle createIndexBuffer(const uint32_t* data, uint32_t count) = 0;

    /**
     * @brief Bake a pipeline state object.
     *
     * Records the shader reference, vertex layout, and render states so each
     * draw call can apply them atomically without per-draw state churn.
     *
     * @param desc  Full pipeline description (shader, layout, blend/depth/rasterizer).
     * @return Valid PipelineHandle on success; {0} on failure.
     */
    virtual PipelineHandle createPipeline(const PipelineDesc& desc) = 0;

    /**
     * @brief Allocate a 2D texture (empty; upload data via updateTexture()).
     *
     * @param desc  Texture dimensions and format.
     * @return Valid TextureHandle on success; {0} on failure.
     */
    virtual TextureHandle createTexture(const TextureDesc& desc) = 0;

    // -----------------------------------------------------------------------
    // Resource update
    // -----------------------------------------------------------------------

    /**
     * @brief Upload new data to a dynamic vertex buffer.
     *
     * @param handle  Buffer returned by createVertexBuffer() with dynamic=true.
     * @param data    Pointer to new data.
     * @param bytes   Number of bytes to upload (must not exceed the buffer size).
     */
    virtual void updateBuffer(BufferHandle handle, const void* data, uint32_t bytes) = 0;

    /**
     * @brief Upload pixel data to a texture.
     *
     * @param handle  Texture returned by createTexture().
     * @param data    Raw pixel data matching the texture's format.
     * @param bytes   Size in bytes.
     */
    virtual void updateTexture(TextureHandle handle, const void* data, uint32_t bytes) = 0;

    // -----------------------------------------------------------------------
    // Resource destruction
    // -----------------------------------------------------------------------

    /** @brief Release a shader program. Undefined if a live pipeline references it. */
    virtual void destroy(ShaderHandle handle) = 0;

    /** @brief Release a vertex or index buffer. */
    virtual void destroy(BufferHandle handle) = 0;

    /** @brief Release a pipeline state object. */
    virtual void destroy(PipelineHandle handle) = 0;

    /** @brief Release a 2D texture. */
    virtual void destroy(TextureHandle handle) = 0;

    // -----------------------------------------------------------------------
    // Uniform setters
    //
    // Each setter binds the shader referenced by @p shader before writing the
    // uniform so that uniforms can be set in any order before a draw call.
    // -----------------------------------------------------------------------

    /** @brief Set a scalar integer uniform. */
    virtual void setInt(ShaderHandle shader, const char* name, int value) = 0;

    /** @brief Set a scalar float uniform. */
    virtual void setFloat(ShaderHandle shader, const char* name, float value) = 0;

    /** @brief Set a vec3 uniform. */
    virtual void setVec3(ShaderHandle shader, const char* name, const vne::math::Vec3f& v) = 0;

    /** @brief Set a vec4 uniform. */
    virtual void setVec4(ShaderHandle shader, const char* name, const vne::math::Vec4f& v) = 0;

    /** @brief Set a column-major mat4 uniform. */
    virtual void setMat4(ShaderHandle shader, const char* name, const vne::math::Mat4f& m) = 0;

    // -----------------------------------------------------------------------
    // Draw calls
    // -----------------------------------------------------------------------

    /**
     * @brief Non-indexed draw call.
     *
     * Applies the pipeline's state, binds the shader, and calls the equivalent
     * of @c glDrawArrays.
     *
     * @param pipeline     Pipeline state to apply.
     * @param vbo          Vertex buffer containing the geometry.
     * @param vertex_count Number of vertices to draw.
     * @param mode         Primitive topology.
     */
    virtual void draw(PipelineHandle pipeline, BufferHandle vbo, uint32_t vertex_count, DrawMode mode) = 0;

    /**
     * @brief Indexed draw call.
     *
     * Applies the pipeline's state, binds the shader, and calls the equivalent
     * of @c glDrawElements.
     *
     * @param pipeline     Pipeline state to apply.
     * @param vbo          Vertex buffer containing the geometry.
     * @param ibo          Index buffer (32-bit unsigned indices).
     * @param index_count  Number of indices to draw.
     * @param mode         Primitive topology.
     */
    virtual void drawIndexed(
        PipelineHandle pipeline, BufferHandle vbo, BufferHandle ibo, uint32_t index_count, DrawMode mode) = 0;

    // -----------------------------------------------------------------------
    // Debug markers
    // -----------------------------------------------------------------------

    /**
     * @brief Begin a named GPU debug group (visible in RenderDoc, Xcode GPU, etc.).
     *
     * Must be paired with popDebugGroup().  Prefer the RAII DebugGroupScope.
     */
    virtual void pushDebugGroup(const char* label) = 0;

    /** @brief End the innermost GPU debug group. */
    virtual void popDebugGroup() = 0;
};

// ============================================================================
// DebugGroupScope — RAII debug marker
// ============================================================================

/**
 * @struct DebugGroupScope
 * @brief RAII wrapper for pushDebugGroup / popDebugGroup.
 *
 * Usage:
 * @code
 *   void MyLayer::onRender(const RenderContext& ctx) {
 *       DebugGroupScope _{*ctx.device, "MyLayer"};
 *       // ... draw calls ...
 *   }  // popDebugGroup called automatically
 * @endcode
 */
struct DebugGroupScope {
    IRenderDevice& dev;

    explicit DebugGroupScope(IRenderDevice& device, const char* label)
        : dev(device) {
        dev.pushDebugGroup(label);
    }
    ~DebugGroupScope() { dev.popDebugGroup(); }

    DebugGroupScope(const DebugGroupScope&) = delete;
    DebugGroupScope& operator=(const DebugGroupScope&) = delete;
};

}  // namespace testbed
}  // namespace vne
