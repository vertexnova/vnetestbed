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
 * @file gl/texture2d.h
 * @brief OpenGL 2D texture (xgl-style: descriptor + raw data API).
 *
 * Create from a descriptor; upload pixel data via updateData(). No
 * file-path loading in the core API.
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "texture2d.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES. Build with OpenGL or OpenGL ES enabled."
#endif

#include <cstddef>
#include <cstdint>

namespace vne {
namespace testbed {
namespace gl {

/**
 * @brief Texture format for 2D textures (maps to GL internal/format/type).
 */
enum class Texture2DFormat { RGBA8, RGB8, R8 };

/**
 * @struct Texture2DDescriptor
 * @brief Descriptor for creating a 2D texture (dimensions + format).
 */
struct Texture2DDescriptor {
    uint32_t width{0};
    uint32_t height{0};
    Texture2DFormat format{Texture2DFormat::RGBA8};
};

/**
 * @class Texture2D
 * @brief RAII OpenGL 2D texture; data supplied via updateData() (xgl-style).
 */
class Texture2D {
   public:
    /**
     * @brief Create a texture from a descriptor (allocates GPU storage).
     * @param desc Descriptor (width, height, format). Data can be uploaded later.
     */
    explicit Texture2D(const Texture2DDescriptor& desc);
    ~Texture2D();

    Texture2D(const Texture2D&) = delete;
    Texture2D& operator=(const Texture2D&) = delete;
    Texture2D(Texture2D&&) = delete;
    Texture2D& operator=(Texture2D&&) = delete;

    /**
     * @brief Upload pixel data for a mip level and array layer.
     *
     * @param mip_level   Mip level (0 = base).
     * @param array_layer Array layer (0 for non-array 2D).
     * @param data        Raw pixel data (layout must match format).
     * @param data_size   Size in bytes.
     * @return true on success.
     */
    bool updateData(uint32_t mip_level, uint32_t array_layer, const void* data, std::size_t data_size);

    /**
     * @brief Bind this texture to a texture unit (for sampling in shaders).
     * @param unit Texture unit index (e.g. 0 for GL_TEXTURE0).
     */
    void bind(uint32_t unit) const;

    /** @brief Width of the base mip level. */
    [[nodiscard]] uint32_t getWidth() const { return width_; }
    /** @brief Height of the base mip level. */
    [[nodiscard]] uint32_t getHeight() const { return height_; }
    /** @brief Format. */
    [[nodiscard]] Texture2DFormat getFormat() const { return format_; }
    /** @brief Whether the texture was created successfully. */
    [[nodiscard]] bool isValid() const { return texture_id_ != 0u; }

   private:
    unsigned int texture_id_{0u};
    uint32_t width_{0u};
    uint32_t height_{0u};
    Texture2DFormat format_{Texture2DFormat::RGBA8};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
