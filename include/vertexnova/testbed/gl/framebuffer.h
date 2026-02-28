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
 * @file gl/framebuffer.h
 * @brief OpenGL framebuffer object for offscreen rendering.
 *
 * Creates an FBO with an internal color texture (RGBA8) and optional
 * depth renderbuffer. Use bind() to render to it; use getColorTextureId()
 * to sample the result.
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#ifndef VNE_TESTBED_OPENGL
#error "framebuffer.h requires VNE_TESTBED_OPENGL to be defined. Build with OpenGL enabled (glad)."
#endif

#include <cstdint>

namespace vne {
namespace testbed {
namespace gl {

/**
 * @struct FramebufferDescriptor
 * @brief Descriptor for creating an offscreen framebuffer.
 */
struct FramebufferDescriptor {
    uint32_t width{0};
    uint32_t height{0};
    bool attach_depth{true};  ///< If true, attach a depth renderbuffer
};

/**
 * @class Framebuffer
 * @brief RAII OpenGL framebuffer (FBO) with color texture and optional depth.
 */
class Framebuffer {
   public:
    /**
     * @brief Create an offscreen framebuffer from a descriptor.
     *
     * Allocates a color attachment (RGBA8 texture) and optionally a depth
     * renderbuffer. Check isValid() after construction.
     *
     * @param desc Width, height, and whether to attach depth.
     */
    explicit Framebuffer(const FramebufferDescriptor& desc);
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;
    Framebuffer(Framebuffer&&) = delete;
    Framebuffer& operator=(Framebuffer&&) = delete;

    /**
     * @brief Bind this framebuffer for rendering (glBindFramebuffer).
     */
    void bind() const;

    /**
     * @brief Unbind (bind default framebuffer 0).
     */
    void unbind() const;

    /**
     * @brief OpenGL texture id of the color attachment (for use as sampler2D).
     *
     * Valid after construction if isValid(). Do not delete this texture;
     * it is owned by this Framebuffer.
     */
    [[nodiscard]] unsigned int getColorTextureId() const { return color_texture_id_; }

    /** @brief Framebuffer width. */
    [[nodiscard]] uint32_t getWidth() const { return width_; }
    /** @brief Framebuffer height. */
    [[nodiscard]] uint32_t getHeight() const { return height_; }
    /** @brief Whether the framebuffer was created and is complete. */
    [[nodiscard]] bool isValid() const { return fbo_id_ != 0u; }

   private:
    unsigned int fbo_id_{0u};
    unsigned int color_texture_id_{0u};
    unsigned int depth_rbo_id_{0u};
    uint32_t width_{0u};
    uint32_t height_{0u};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
