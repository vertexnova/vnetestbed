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
 * @file gl/opengl_render_device.h
 * @brief IRenderDevice implementation backed by raw OpenGL / OpenGL ES.
 *
 * Internally uses the gl/ RAII primitives (Shader, VertexBuffer,
 * IndexBuffer, VertexArray, Texture2D) as implementation details.
 * No gl/ type leaks into the public interface.
 *
 * Swap point for CrossGL: replace OpenGLRenderDevice with
 * CrossGLRenderDevice that implements the same IRenderDevice interface.
 *
 * Only available when VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES is defined.
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "opengl_render_device.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES."
#endif

#include "vertexnova/testbed/render_device.h"

#include <cstdint>
#include <memory>
#include <vector>

// Forward-declare the gl/ impl types so they don't pollute this header.
namespace vne {
namespace testbed {
namespace gl {
class Shader;
class VertexBuffer;
class IndexBuffer;
class Texture2D;
}  // namespace gl
}  // namespace testbed
}  // namespace vne

namespace vne {
namespace testbed {
namespace gl {

/**
 * @class OpenGLRenderDevice
 * @brief Concrete IRenderDevice using OpenGL 4.1 / OpenGL ES 3.0.
 *
 * Resources are stored in generation-free slot pools (vector<unique_ptr<Slot>>).
 * Handle ids are 1-based indices: id == 0 is always invalid.
 *
 * Lifecycle:
 *  1. Construct after a valid OpenGL context is current (i.e. after
 *     OpenGLRenderAdapter::init()).
 *  2. Demo layers call create*() in onAttach() and destroy() in onDetach().
 *  3. Call shutdown() before destroying the GL context to free all remaining
 *     GPU objects (or rely on the destructor).
 *
 * Thread safety: not thread-safe; all calls must happen on the GL thread.
 */
class OpenGLRenderDevice : public IRenderDevice {
   public:
    OpenGLRenderDevice() = default;
    ~OpenGLRenderDevice() override;

    OpenGLRenderDevice(const OpenGLRenderDevice&) = delete;
    OpenGLRenderDevice& operator=(const OpenGLRenderDevice&) = delete;

    /**
     * @brief Release all remaining GPU resources.
     *
     * Called automatically by the destructor.  Can be called explicitly when
     * deterministic cleanup ordering is required (e.g. before glfwTerminate).
     */
    void shutdown();

    // -----------------------------------------------------------------------
    // IRenderDevice — resource creation
    // -----------------------------------------------------------------------

    ShaderHandle compileShader(const char* vert_src, const char* frag_src) override;
    ShaderHandle createShader(const std::filesystem::path& vert_path, const std::filesystem::path& frag_path) override;
    BufferHandle createVertexBuffer(const void* data, uint32_t bytes, bool dynamic = false) override;
    BufferHandle createIndexBuffer(const uint32_t* data, uint32_t count) override;
    PipelineHandle createPipeline(const PipelineDesc& desc) override;
    TextureHandle createTexture(const TextureDesc& desc) override;

    // -----------------------------------------------------------------------
    // IRenderDevice — resource update
    // -----------------------------------------------------------------------

    void updateBuffer(BufferHandle handle, const void* data, uint32_t bytes) override;
    void updateTexture(TextureHandle handle, const void* data, uint32_t bytes) override;

    // -----------------------------------------------------------------------
    // IRenderDevice — resource destruction
    // -----------------------------------------------------------------------

    void destroy(ShaderHandle handle) override;
    void destroy(BufferHandle handle) override;
    void destroy(PipelineHandle handle) override;
    void destroy(TextureHandle handle) override;

    // -----------------------------------------------------------------------
    // IRenderDevice — uniform setters
    // -----------------------------------------------------------------------

    void setInt(ShaderHandle shader, const char* name, int value) override;
    void setFloat(ShaderHandle shader, const char* name, float value) override;
    void setVec3(ShaderHandle shader, const char* name, const vne::math::Vec3f& v) override;
    void setVec4(ShaderHandle shader, const char* name, const vne::math::Vec4f& v) override;
    void setMat4(ShaderHandle shader, const char* name, const vne::math::Mat4f& m) override;

    // -----------------------------------------------------------------------
    // IRenderDevice — draw calls
    // -----------------------------------------------------------------------

    void draw(PipelineHandle pipeline, BufferHandle vbo, uint32_t vertex_count, DrawMode mode) override;
    void drawIndexed(
        PipelineHandle pipeline, BufferHandle vbo, BufferHandle ibo, uint32_t index_count, DrawMode mode) override;

    // -----------------------------------------------------------------------
    // IRenderDevice — debug markers
    // -----------------------------------------------------------------------

    void pushDebugGroup(const char* label) override;
    void popDebugGroup() override;

   private:
    // -----------------------------------------------------------------------
    // Internal slot types — not exposed publicly
    // -----------------------------------------------------------------------

    struct ShaderSlot {
        std::unique_ptr<Shader> shader;
    };

    struct BufferSlot {
        std::unique_ptr<VertexBuffer> vbo;  ///< Non-null for vertex buffers.
        std::unique_ptr<IndexBuffer> ibo;   ///< Non-null for index buffers.
        bool is_index{false};
    };

    struct PipelineSlot {
        uint32_t shader_idx{0};            ///< Index into shaders_ (1-based).
        std::vector<VertexAttrib> layout;  ///< Vertex attrib descriptors.
        BlendState blend{};
        DepthState depth{};
        RasterizerState rasterizer{};
        unsigned int vao_id{0};             ///< OpenGL VAO for this pipeline.
        const void* last_vbo_ptr{nullptr};  ///< Last VBO bound to VAO (dirty tracking).
        const void* last_ibo_ptr{nullptr};  ///< Last IBO bound to VAO (dirty tracking).
    };

    struct TextureSlot {
        std::unique_ptr<Texture2D> tex;
    };

    // -----------------------------------------------------------------------
    // Slot pools (index 0 unused; handle id == slot index, 1-based)
    // -----------------------------------------------------------------------

    std::vector<std::unique_ptr<ShaderSlot>> shaders_;  ///< Index 0 is unused.
    std::vector<std::unique_ptr<BufferSlot>> buffers_;
    std::vector<std::unique_ptr<PipelineSlot>> pipelines_;
    std::vector<std::unique_ptr<TextureSlot>> textures_;

    // -----------------------------------------------------------------------
    // Private helpers
    // -----------------------------------------------------------------------

    /** @brief Allocate a slot in @p pool, returning its 1-based id. */
    template<typename T>
    static uint32_t allocSlot(std::vector<std::unique_ptr<T>>& pool, std::unique_ptr<T> entry);

    /** @brief Apply blend, depth, and rasterizer GL state for a pipeline draw. */
    void applyPipelineState(const PipelineSlot& ps) const;

    /**
     * @brief Bind the pipeline's VAO, wire the VBO attrib pointers, and
     *        optionally bind an IBO.  Skipped when the VBO/IBO haven't changed.
     */
    void configureVao(PipelineSlot& ps, const BufferSlot& vbo_slot, const BufferSlot* ibo_slot);

    /** @brief Map DrawMode to the GL_* constant. */
    static unsigned int toGLPrimitive(DrawMode mode);

    /** @brief Map BlendFactor to the GL_* constant. */
    static unsigned int toGLBlendFactor(BlendFactor f);

    /** @brief Map BlendEquation to the GL_* constant. */
    static unsigned int toGLBlendEquation(BlendEquation eq);

    /** @brief Map CompareFunc to the GL_* constant. */
    static unsigned int toGLCompareFunc(CompareFunc func);
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
