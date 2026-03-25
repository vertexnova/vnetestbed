#pragma once
/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * Autodoc:   yes
 *
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/render_device.h"

#include "vertexnova/math/core/core.h"

namespace vne {
namespace testbed {

/** @brief Single point light for Phong. */
struct PhongPointLight {
    vne::math::Vec3f position{0.0F, 0.0F, 0.0F};
    vne::math::Vec3f color{1.0F, 1.0F, 1.0F};
    float intensity{1.0F};
    float range{10.0F};
    bool enabled{false};
};

/** @brief Spot light for Phong. */
struct PhongSpotLight {
    vne::math::Vec3f position{0.0F, 0.0F, 0.0F};
    vne::math::Vec3f direction{0.0F, -1.0F, 0.0F};
    vne::math::Vec3f color{1.0F, 1.0F, 1.0F};
    float intensity{1.0F};
    float range{10.0F};
    float inner_angle_rad{0.0F};  ///< inner cone angle (radians)
    float outer_angle_rad{0.785398F};
    bool enabled{false};
};

/** @brief Full light parameters for Blinn-Phong (ambient, directional, point[4], spot, attenuation). */
struct PhongLightParams {
    vne::math::Vec3f ambient_color{0.4F, 0.4F, 0.45F};
    float ambient_intensity{1.0F};

    vne::math::Vec3f dir_light_dir{0.0F, -1.0F, 0.0F};
    vne::math::Vec3f dir_light_color{1.0F, 1.0F, 1.0F};
    float dir_light_intensity{0.0F};
    bool dir_light_enabled{false};

    static constexpr int kMaxPointLights = 4;
    PhongPointLight point_lights[kMaxPointLights]{};

    PhongSpotLight spot_light{};

    float attn_const{1.0F};
    float attn_linear{0.09F};
    float attn_quad{0.032F};
    bool use_attn_formula{false};
};

/**
 * @class PhongMaterial
 * @brief Holds shader and pipeline for Blinn-Phong; compiles embedded GLSL and exposes setUniforms.
 *
 * Vertex shader transforms normals with the inverse-transpose of the model matrix upper-left 3×3
 * (correct for non-uniform scale; rotation+uniform scale are unchanged vs mat3(u_model)).
 */
class PhongMaterial {
   public:
    PhongMaterial() = default;
    ~PhongMaterial();

    PhongMaterial(const PhongMaterial&) = delete;
    PhongMaterial& operator=(const PhongMaterial&) = delete;
    PhongMaterial(PhongMaterial&&) = delete;
    PhongMaterial& operator=(PhongMaterial&&) = delete;

    /**
     * @brief Compile embedded vert/frag and create pipeline (layout pos3 + normal3 + color3).
     * @param device Backend-agnostic render device.
     * @return true on success.
     */
    [[nodiscard]] bool init(IRenderDevice* device);

    /** @brief Release shader and pipeline. */
    void shutdown();

    [[nodiscard]] bool isReady() const { return shader_.isValid() && pipeline_.isValid(); }
    [[nodiscard]] ShaderHandle getShader() const { return shader_; }
    [[nodiscard]] PipelineHandle getPipeline() const { return pipeline_; }

    /**
     * @brief Set MVP, model, camera position and light uniforms on the shader.
     */
    void setUniforms(IRenderDevice* device,
                     const vne::math::Mat4f& mvp,
                     const vne::math::Mat4f& model,
                     const vne::math::Vec3f& cam_pos,
                     const PhongLightParams& lights) const;

   private:
    IRenderDevice* device_{nullptr};
    ShaderHandle shader_{};
    PipelineHandle pipeline_{};
};

}  // namespace testbed
}  // namespace vne
