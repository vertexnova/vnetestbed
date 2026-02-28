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

#include "vertexnova/testbed/gl/opengl_render_device.h"

// Pull in the gl/ RAII types (implementation detail only).
#include "vertexnova/testbed/gl/index_buffer.h"
#include "vertexnova/testbed/gl/shader.h"
#include "vertexnova/testbed/gl/texture2d.h"
#include "vertexnova/testbed/gl/vertex_buffer.h"

#if defined(VNE_TESTBED_OPENGL)
#include <glad/glad.h>
#elif defined(VNE_TESTBED_OPENGLES)
#include <glad/glad_es3.h>
#endif

#include "vertexnova/common/macros.h"
#include <vertexnova/logging/logging.h>

namespace {

CREATE_VNE_LOGGER_CATEGORY("vnetestbed.gl")

}  // namespace

namespace vne {
namespace testbed {
namespace gl {

// ============================================================================
// Destructor / shutdown
// ============================================================================

OpenGLRenderDevice::~OpenGLRenderDevice() {
    shutdown();
}

void OpenGLRenderDevice::shutdown() {
    // Release VAOs owned by pipeline slots before clearing pools.
    for (auto& slot : pipelines_) {
        if (slot && slot->vao_id != 0u) {
            glDeleteVertexArrays(1, &slot->vao_id);
            slot->vao_id = 0u;
        }
    }
    // Remaining GPU objects are freed via RAII in the unique_ptrs.
    pipelines_.clear();
    shaders_.clear();
    buffers_.clear();
    textures_.clear();
}

// ============================================================================
// Slot allocation helper
// ============================================================================

template<typename T>
uint32_t OpenGLRenderDevice::allocSlot(std::vector<std::unique_ptr<T>>& pool, std::unique_ptr<T> entry) {
    // Slot index 0 is reserved as "invalid"; the pool always starts with a null.
    if (pool.empty()) {
        pool.emplace_back(nullptr);  // index 0 — never used
    }
    // Reuse the first freed slot.
    for (uint32_t i = 1u; i < static_cast<uint32_t>(pool.size()); ++i) {
        if (!pool[i]) {
            pool[i] = std::move(entry);
            return i;
        }
    }
    pool.push_back(std::move(entry));
    return static_cast<uint32_t>(pool.size() - 1u);
}

// ============================================================================
// Resource creation
// ============================================================================

ShaderHandle OpenGLRenderDevice::compileShader(const char* vert_src, const char* frag_src) {
    auto slot = std::make_unique<ShaderSlot>();
    slot->shader = std::make_unique<Shader>(vert_src, frag_src);
    if (!slot->shader->isValid()) {
        VNE_LOG_ERROR << "compileShader: compilation failed";
        return {};
    }
    const uint32_t id = allocSlot(shaders_, std::move(slot));
    return {id};
}

ShaderHandle OpenGLRenderDevice::createShader(const std::filesystem::path& vert_path,
                                              const std::filesystem::path& frag_path) {
    auto slot = std::make_unique<ShaderSlot>();
    slot->shader = std::make_unique<Shader>(vert_path, frag_path);
    if (!slot->shader->isValid()) {
        VNE_LOG_ERROR << "createShader: failed";
        return {};
    }
    const uint32_t id = allocSlot(shaders_, std::move(slot));
    return {id};
}

BufferHandle OpenGLRenderDevice::createVertexBuffer(const void* data, uint32_t bytes, bool dynamic) {
    auto slot = std::make_unique<BufferSlot>();
    slot->is_index = false;
    if (dynamic) {
        slot->vbo = std::make_unique<VertexBuffer>(static_cast<std::size_t>(bytes));
        if (data) {
            slot->vbo->setData(data, bytes);
        }
    } else {
        slot->vbo = std::make_unique<VertexBuffer>(data, static_cast<std::size_t>(bytes));
    }
    const uint32_t id = allocSlot(buffers_, std::move(slot));
    return {id};
}

BufferHandle OpenGLRenderDevice::createIndexBuffer(const uint32_t* data, uint32_t count) {
    auto slot = std::make_unique<BufferSlot>();
    slot->is_index = true;
    slot->ibo = std::make_unique<IndexBuffer>(data, static_cast<std::size_t>(count));
    const uint32_t id = allocSlot(buffers_, std::move(slot));
    return {id};
}

PipelineHandle OpenGLRenderDevice::createPipeline(const PipelineDesc& desc) {
    if (!desc.shader.isValid()) {
        VNE_LOG_ERROR << "createPipeline: invalid shader handle";
        return {};
    }
    if (desc.shader.id >= shaders_.size() || !shaders_[desc.shader.id]) {
        VNE_LOG_ERROR << "createPipeline: shader " << desc.shader.id << " not found";
        return {};
    }

    auto slot = std::make_unique<PipelineSlot>();
    slot->shader_idx = desc.shader.id;
    slot->layout = desc.layout;
    slot->blend = desc.blend;
    slot->depth = desc.depth;
    slot->rasterizer = desc.rasterizer;

    // Allocate a VAO for this pipeline.
    glGenVertexArrays(1, &slot->vao_id);

    const uint32_t id = allocSlot(pipelines_, std::move(slot));
    return {id};
}

TextureHandle OpenGLRenderDevice::createTexture(const TextureDesc& desc) {
    Texture2DDescriptor gl_desc;
    gl_desc.width = desc.width;
    gl_desc.height = desc.height;
    switch (desc.format) {
        case TextureFormat::eRGBA8:
            gl_desc.format = Texture2DFormat::eRGBA8;
            break;
        case TextureFormat::eRGB8:
            gl_desc.format = Texture2DFormat::eRGB8;
            break;
        case TextureFormat::eR8:
            gl_desc.format = Texture2DFormat::eR8;
            break;
    }
    auto slot = std::make_unique<TextureSlot>();
    slot->tex = std::make_unique<Texture2D>(gl_desc);
    if (!slot->tex->isValid()) {
        VNE_LOG_ERROR << "createTexture: allocation failed";
        return {};
    }
    const uint32_t id = allocSlot(textures_, std::move(slot));
    return {id};
}

// ============================================================================
// Resource update
// ============================================================================

void OpenGLRenderDevice::updateBuffer(BufferHandle handle, const void* data, uint32_t bytes) {
    if (!handle.isValid() || handle.id >= buffers_.size() || !buffers_[handle.id]) {
        return;
    }
    if (data == nullptr || bytes == 0u) {
        if (data == nullptr && bytes > 0u) {
            VNE_ASSERT_MSG(data != nullptr, "updateBuffer: data must not be null when bytes > 0");
        }
        return;
    }
    auto& slot = *buffers_[handle.id];
    if (!slot.is_index && slot.vbo) {
        slot.vbo->setData(data, bytes);
    }
}

void OpenGLRenderDevice::updateTexture(TextureHandle handle, const void* data, uint32_t bytes) {
    if (!handle.isValid() || handle.id >= textures_.size() || !textures_[handle.id]) {
        return;
    }
    textures_[handle.id]->tex->updateData(0, 0, data, bytes);
}

// ============================================================================
// Resource destruction
// ============================================================================

void OpenGLRenderDevice::destroy(ShaderHandle handle) {
    if (!handle.isValid() || handle.id >= shaders_.size()) {
        return;
    }
    shaders_[handle.id].reset();
}

void OpenGLRenderDevice::destroy(BufferHandle handle) {
    if (!handle.isValid() || handle.id >= buffers_.size()) {
        return;
    }
    buffers_[handle.id].reset();
    invalidatePipelineBufferCaches();
}

void OpenGLRenderDevice::destroy(PipelineHandle handle) {
    if (!handle.isValid() || handle.id >= pipelines_.size()) {
        return;
    }
    auto& slot = pipelines_[handle.id];
    if (slot && slot->vao_id != 0u) {
        glDeleteVertexArrays(1, &slot->vao_id);
    }
    slot.reset();
}

void OpenGLRenderDevice::destroy(TextureHandle handle) {
    if (!handle.isValid() || handle.id >= textures_.size()) {
        return;
    }
    textures_[handle.id].reset();
}

// ============================================================================
// Uniform setters
// ============================================================================

void OpenGLRenderDevice::setInt(ShaderHandle sh, const char* name, int value) {
    if (!sh.isValid() || sh.id >= shaders_.size() || !shaders_[sh.id]) {
        return;
    }
    auto& s = *shaders_[sh.id]->shader;
    s.bind();
    s.setInt(name, value);
    s.unbind();
}

void OpenGLRenderDevice::setFloat(ShaderHandle sh, const char* name, float value) {
    if (!sh.isValid() || sh.id >= shaders_.size() || !shaders_[sh.id]) {
        return;
    }
    auto& s = *shaders_[sh.id]->shader;
    s.bind();
    s.setFloat(name, value);
    s.unbind();
}

void OpenGLRenderDevice::setVec3(ShaderHandle sh, const char* name, const vne::math::Vec3f& v) {
    if (!sh.isValid() || sh.id >= shaders_.size() || !shaders_[sh.id]) {
        return;
    }
    auto& s = *shaders_[sh.id]->shader;
    s.bind();
    s.setVec3(name, v);
    s.unbind();
}

void OpenGLRenderDevice::setVec4(ShaderHandle sh, const char* name, const vne::math::Vec4f& v) {
    if (!sh.isValid() || sh.id >= shaders_.size() || !shaders_[sh.id]) {
        return;
    }
    auto& s = *shaders_[sh.id]->shader;
    s.bind();
    s.setVec4(name, v);
    s.unbind();
}

void OpenGLRenderDevice::setMat4(ShaderHandle sh, const char* name, const vne::math::Mat4f& m) {
    if (!sh.isValid() || sh.id >= shaders_.size() || !shaders_[sh.id]) {
        return;
    }
    auto& s = *shaders_[sh.id]->shader;
    s.bind();
    s.setMat4(name, m);
    s.unbind();
}

// ============================================================================
// Draw helpers
// ============================================================================

void OpenGLRenderDevice::applyPipelineState(const PipelineSlot& ps) const {
    // --- Blending ---
    if (ps.blend.enabled) {
        glEnable(GL_BLEND);
        glBlendFunc(toGLBlendFactor(ps.blend.src), toGLBlendFactor(ps.blend.dst));
        glBlendEquation(toGLBlendEquation(ps.blend.eq));
    } else {
        glDisable(GL_BLEND);
    }

    // --- Depth ---
    if (ps.depth.testEnabled) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(toGLCompareFunc(ps.depth.func));
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(ps.depth.writeEnabled ? GL_TRUE : GL_FALSE);

    // --- Culling ---
    if (ps.rasterizer.cull != CullMode::eNone) {
        glEnable(GL_CULL_FACE);
        glCullFace(ps.rasterizer.cull == CullMode::eFront ? GL_FRONT : GL_BACK);
    } else {
        glDisable(GL_CULL_FACE);
    }

    // --- Fill mode (desktop GL only; glPolygonMode absent in ES) ---
#if defined(VNE_TESTBED_OPENGL)
    glPolygonMode(GL_FRONT_AND_BACK, ps.rasterizer.wireframe ? GL_LINE : GL_FILL);
#endif
}

void OpenGLRenderDevice::configureVao(PipelineSlot& ps, const BufferSlot& vbo_slot, const BufferSlot* ibo_slot) {
    const uint32_t vbo_id = vbo_slot.vbo->getId();
    const uint32_t ibo_id = ibo_slot && ibo_slot->ibo ? ibo_slot->ibo->getId() : 0u;

    if (vbo_id == ps.last_vbo_id && ibo_id == ps.last_ibo_id) {
        return;  // VAO is already configured for this VBO/IBO pair.
    }

    // Compute stride from layout.
    std::size_t stride = 0u;
    for (const auto& attr : ps.layout) {
        stride += static_cast<std::size_t>(attr.num_floats) * sizeof(float);
    }

    glBindVertexArray(ps.vao_id);
    vbo_slot.vbo->bind();

    // Configure vertex attributes.
    std::size_t offset = 0u;
    for (uint32_t loc = 0u; loc < static_cast<uint32_t>(ps.layout.size()); ++loc) {
        const auto& attr = ps.layout[loc];
        glEnableVertexAttribArray(loc);
        glVertexAttribPointer(loc,
                              static_cast<GLint>(attr.num_floats),
                              GL_FLOAT,
                              attr.normalized ? GL_TRUE : GL_FALSE,
                              static_cast<GLsizei>(stride),
                              reinterpret_cast<const void*>(offset));
        offset += static_cast<std::size_t>(attr.num_floats) * sizeof(float);
    }

    // Bind index buffer (recorded into the VAO).
    if (ibo_slot && ibo_slot->ibo) {
        ibo_slot->ibo->bind();
    }

    glBindVertexArray(0);
    vbo_slot.vbo->unbind();
    if (ibo_slot && ibo_slot->ibo) {
        ibo_slot->ibo->unbind();
    }

    ps.last_vbo_id = vbo_id;
    ps.last_ibo_id = ibo_id;
}

void OpenGLRenderDevice::invalidatePipelineBufferCaches() {
    for (auto& slot : pipelines_) {
        if (slot) {
            slot->last_vbo_id = 0u;
            slot->last_ibo_id = 0u;
        }
    }
}

// ============================================================================
// Draw calls
// ============================================================================

void OpenGLRenderDevice::draw(PipelineHandle ph, BufferHandle vbo_h, uint32_t vertex_count, DrawMode mode) {
    if (!ph.isValid() || ph.id >= pipelines_.size() || !pipelines_[ph.id]) {
        return;
    }
    if (!vbo_h.isValid() || vbo_h.id >= buffers_.size() || !buffers_[vbo_h.id]) {
        return;
    }

    auto& ps = *pipelines_[ph.id];
    auto& bs = *buffers_[vbo_h.id];
    if (!bs.vbo) {
        return;
    }

    if (ps.shader_idx == 0u || ps.shader_idx >= shaders_.size() || !shaders_[ps.shader_idx]) {
        return;
    }

    configureVao(ps, bs, nullptr);
    applyPipelineState(ps);
    shaders_[ps.shader_idx]->shader->bind();

    glBindVertexArray(ps.vao_id);
    glDrawArrays(toGLPrimitive(mode), 0, static_cast<GLsizei>(vertex_count));
    glBindVertexArray(0);
}

void OpenGLRenderDevice::drawIndexed(
    PipelineHandle ph, BufferHandle vbo_h, BufferHandle ibo_h, uint32_t index_count, DrawMode mode) {
    if (!ph.isValid() || ph.id >= pipelines_.size() || !pipelines_[ph.id]) {
        return;
    }
    if (!vbo_h.isValid() || vbo_h.id >= buffers_.size() || !buffers_[vbo_h.id]) {
        return;
    }
    if (!ibo_h.isValid() || ibo_h.id >= buffers_.size() || !buffers_[ibo_h.id]) {
        return;
    }

    auto& ps = *pipelines_[ph.id];
    auto& vbs = *buffers_[vbo_h.id];
    auto& ibs = *buffers_[ibo_h.id];
    if (!vbs.vbo || !ibs.ibo) {
        return;
    }

    if (ps.shader_idx == 0u || ps.shader_idx >= shaders_.size() || !shaders_[ps.shader_idx]) {
        return;
    }

    configureVao(ps, vbs, &ibs);
    applyPipelineState(ps);
    shaders_[ps.shader_idx]->shader->bind();

    glBindVertexArray(ps.vao_id);
    glDrawElements(toGLPrimitive(mode), static_cast<GLsizei>(index_count), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

// ============================================================================
// Debug markers
// ============================================================================

void OpenGLRenderDevice::pushDebugGroup(const char* label) {
#if defined(VNE_TESTBED_OPENGL)
    // glPushDebugGroup is available in core GL 4.3+ and via GL_KHR_debug on 4.1.
    // GLAD loads it as a function pointer; check for null to avoid crash on 4.1
    // without the extension.
    if (glPushDebugGroup) {
        glPushDebugGroup(GL_DEBUG_SOURCE_APPLICATION, 0u, -1, label);
    }
#else
    // OpenGL ES: GL_KHR_debug extension provides glPushDebugGroupKHR.
    // Marked as a no-op here; wire up via extension loader when needed.
    (void)label;
#endif
}

void OpenGLRenderDevice::popDebugGroup() {
#if defined(VNE_TESTBED_OPENGL)
    if (glPopDebugGroup) {
        glPopDebugGroup();
    }
#endif
}

// ============================================================================
// GL enum mapping helpers
// ============================================================================

uint32_t OpenGLRenderDevice::toGLPrimitive(DrawMode mode) {
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

uint32_t OpenGLRenderDevice::toGLBlendFactor(BlendFactor f) {
    switch (f) {
        case BlendFactor::eZero:
            return GL_ZERO;
        case BlendFactor::eOne:
            return GL_ONE;
        case BlendFactor::eSrcAlpha:
            return GL_SRC_ALPHA;
        case BlendFactor::eOneMinusSrcAlpha:
            return GL_ONE_MINUS_SRC_ALPHA;
        case BlendFactor::eDstAlpha:
            return GL_DST_ALPHA;
        case BlendFactor::eOneMinusDstAlpha:
            return GL_ONE_MINUS_DST_ALPHA;
        case BlendFactor::eSrcColor:
            return GL_SRC_COLOR;
        case BlendFactor::eOneMinusSrcColor:
            return GL_ONE_MINUS_SRC_COLOR;
    }
    return GL_ONE;
}

uint32_t OpenGLRenderDevice::toGLBlendEquation(BlendEquation eq) {
    switch (eq) {
        case BlendEquation::eAdd:
            return GL_FUNC_ADD;
        case BlendEquation::eSubtract:
            return GL_FUNC_SUBTRACT;
        case BlendEquation::eReverseSubtract:
            return GL_FUNC_REVERSE_SUBTRACT;
        case BlendEquation::eMin:
            return GL_MIN;
        case BlendEquation::eMax:
            return GL_MAX;
    }
    return GL_FUNC_ADD;
}

uint32_t OpenGLRenderDevice::toGLCompareFunc(CompareFunc func) {
    switch (func) {
        case CompareFunc::eNever:
            return GL_NEVER;
        case CompareFunc::eLess:
            return GL_LESS;
        case CompareFunc::eEqual:
            return GL_EQUAL;
        case CompareFunc::eLessEqual:
            return GL_LEQUAL;
        case CompareFunc::eGreater:
            return GL_GREATER;
        case CompareFunc::eNotEqual:
            return GL_NOTEQUAL;
        case CompareFunc::eGreaterEqual:
            return GL_GEQUAL;
        case CompareFunc::eAlways:
            return GL_ALWAYS;
    }
    return GL_LESS;
}

}  // namespace gl
}  // namespace testbed
}  // namespace vne
