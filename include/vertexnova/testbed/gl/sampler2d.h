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
 * @file gl/sampler2d.h
 * @brief OpenGL sampler object for 2D textures (wrap + filter state).
 *
 * Bind to the same texture unit as a Texture2D to control sampling.
 *
 * Only available when VNE_TESTBED_OPENGL is defined.
 */

#if !defined(VNE_TESTBED_OPENGL) && !defined(VNE_TESTBED_OPENGLES)
#error "sampler2d.h requires VNE_TESTBED_OPENGL or VNE_TESTBED_OPENGLES. Build with OpenGL or OpenGL ES enabled."
#endif

#include <cstdint>

namespace vne {
namespace testbed {
namespace gl {

/**
 * @brief Texture wrap mode (S/T).
 */
enum class SamplerWrap {
    Repeat,
    ClampToEdge,
    MirroredRepeat
};

/**
 * @brief Texture filter (min/mag).
 */
enum class SamplerFilter {
    Nearest,
    Linear
};

/**
 * @struct Sampler2DDescriptor
 * @brief Descriptor for sampler wrap and filter state.
 */
struct Sampler2DDescriptor {
    SamplerWrap wrap_s{SamplerWrap::Repeat};
    SamplerWrap wrap_t{SamplerWrap::Repeat};
    SamplerFilter min_filter{SamplerFilter::Linear};
    SamplerFilter mag_filter{SamplerFilter::Linear};
};

/**
 * @class Sampler2D
 * @brief RAII OpenGL sampler object; bind to unit with Texture2D for sampling.
 */
class Sampler2D {
   public:
    /** @brief Create with default descriptor (Repeat, Linear). */
    Sampler2D();

    /** @brief Create with custom wrap and filter. */
    explicit Sampler2D(const Sampler2DDescriptor& desc);
    ~Sampler2D();

    Sampler2D(const Sampler2D&) = delete;
    Sampler2D& operator=(const Sampler2D&) = delete;
    Sampler2D(Sampler2D&&) = delete;
    Sampler2D& operator=(Sampler2D&&) = delete;

    /**
     * @brief Bind this sampler to a texture unit (same unit as the texture).
     * @param unit Texture unit index.
     */
    void bind(uint32_t unit) const;

    /** @brief Whether the sampler was created successfully. */
    [[nodiscard]] bool isValid() const { return sampler_id_ != 0u; }

   private:
    unsigned int sampler_id_{0u};
};

}  // namespace gl
}  // namespace testbed
}  // namespace vne
