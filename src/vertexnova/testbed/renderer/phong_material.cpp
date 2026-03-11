/* ---------------------------------------------------------------------
 * Copyright (c) 2026 Ajeet Singh Yadav. All rights reserved.
 * Licensed under the Apache License, Version 2.0 (the "License")
 *
 * Author:    Ajeet Singh Yadav
 * Created:   March 2026
 *
 * PhongMaterial: embedded Blinn-Phong vert/frag, compile and setUniforms.
 * ----------------------------------------------------------------------
 */

#include "vertexnova/testbed/renderer/phong_material.h"

#include "vertexnova/testbed/render_device.h"

#ifdef VNE_TESTBED_LOGGING
#include "vertexnova/logging/logging.h"
#endif

#include <cmath>
#include <vector>

namespace vne {
namespace testbed {

#ifdef VNE_TESTBED_LOGGING
namespace {
CREATE_VNE_LOGGER_CATEGORY("vnetestbed.renderer.phong")
}  // namespace
#endif

#if defined(VNE_TESTBED_OPENGL)
static const char* kPhongVertSrc = R"(
#version 410 core
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_color;

layout(location = 0) out vec3 v_world_position;
layout(location = 1) out vec3 v_normal;
layout(location = 2) out vec3 v_color;

uniform mat4 u_mvp;
uniform mat4 u_model;

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_position = world_pos.xyz;
    v_normal = normalize(mat3(u_model) * a_normal);
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

static const char* kPhongFragSrc = R"(
#version 410 core
layout(location = 0) in vec3 v_world_position;
layout(location = 1) in vec3 v_normal;
layout(location = 2) in vec3 v_color;

layout(location = 0) out vec4 o_color;

uniform vec3 u_ambientColor;
uniform float u_ambientIntensity;
uniform vec3 u_dirLightDir;
uniform vec3 u_dirLightColor;
uniform float u_dirLightIntensity;
uniform int u_dirLightEnabled;
uniform int u_numPointLights;
uniform vec3 u_ptLightPos[4];
uniform vec3 u_ptLightColor[4];
uniform float u_ptLightIntensity[4];
uniform float u_ptLightRange[4];
uniform int u_ptLightEnabled[4];
uniform int u_spotLightEnabled;
uniform vec3 u_spotLightPos;
uniform vec3 u_spotLightDir;
uniform vec3 u_spotLightColor;
uniform float u_spotLightIntensity;
uniform float u_spotLightRange;
uniform float u_spotLightInnerCos;
uniform float u_spotLightOuterCos;
uniform float u_attnConst;
uniform float u_attnLinear;
uniform float u_attnQuad;
uniform int u_useAttnFormula;
uniform vec3 u_camPos;

const float MIN_RANGE = 0.001;
const float SPOT_EPSILON = 1e-6;
const int MAX_POINT_LIGHTS = 4;

void main() {
    vec3 normal = normalize(v_normal);
    vec3 view_dir = normalize(u_camPos - v_world_position);
    vec3 lighting = u_ambientColor * u_ambientIntensity;
    if (u_dirLightEnabled != 0) {
        vec3 light_dir = normalize(-u_dirLightDir);
        float diff = max(dot(normal, light_dir), 0.0);
        vec3 half_dir = normalize(light_dir + view_dir);
        float spec = pow(max(dot(normal, half_dir), 0.0), 32.0);
        lighting += u_dirLightColor * u_dirLightIntensity * (diff * 0.8 + spec * 0.4);
    }
    for (int i = 0; i < u_numPointLights && i < MAX_POINT_LIGHTS; ++i) {
        if (u_ptLightEnabled[i] == 0) continue;
        vec3 to_light = u_ptLightPos[i] - v_world_position;
        float dist = length(to_light);
        float range = max(u_ptLightRange[i], MIN_RANGE);
        float atten;
        if (u_useAttnFormula != 0) {
            atten = 1.0 / (u_attnConst + u_attnLinear * dist + u_attnQuad * dist * dist);
            atten *= step(dist, range);
        } else {
            atten = clamp(1.0 - (dist / range), 0.0, 1.0);
            atten *= atten;
        }
        vec3 light_dir = normalize(to_light);
        float diff = max(dot(normal, light_dir), 0.0);
        vec3 half_dir = normalize(light_dir + view_dir);
        float spec = pow(max(dot(normal, half_dir), 0.0), 32.0);
        lighting += u_ptLightColor[i] * u_ptLightIntensity[i] * atten * (diff * 0.8 + spec * 0.3);
    }
    if (u_spotLightEnabled != 0) {
        vec3 to_light = u_spotLightPos - v_world_position;
        float dist = length(to_light);
        float range = max(u_spotLightRange, MIN_RANGE);
        if (dist <= range) {
            vec3 light_dir = normalize(to_light);
            float cos_theta = dot(light_dir, normalize(-u_spotLightDir));
            float spot_denom = max(u_spotLightInnerCos - u_spotLightOuterCos, SPOT_EPSILON);
            float spot = clamp((cos_theta - u_spotLightOuterCos) / spot_denom, 0.0, 1.0);
            float atten;
            if (u_useAttnFormula != 0) {
                atten = 1.0 / (u_attnConst + u_attnLinear * dist + u_attnQuad * dist * dist);
            } else {
                atten = clamp(1.0 - (dist / range), 0.0, 1.0);
                atten *= atten;
            }
            atten *= spot;
            float diff = max(dot(normal, light_dir), 0.0);
            vec3 half_dir = normalize(light_dir + view_dir);
            float spec = pow(max(dot(normal, half_dir), 0.0), 32.0);
            lighting += u_spotLightColor * u_spotLightIntensity * atten * (diff * 0.8 + spec * 0.3);
        }
    }
    o_color = vec4(v_color * lighting, 1.0);
}
)";
#else
// OpenGL ES 3.0
static const char* kPhongVertSrc = R"(
#version 300 es
layout(location = 0) in vec3 a_position;
layout(location = 1) in vec3 a_normal;
layout(location = 2) in vec3 a_color;

out vec3 v_world_position;
out vec3 v_normal;
out vec3 v_color;

uniform mat4 u_mvp;
uniform mat4 u_model;

void main() {
    vec4 world_pos = u_model * vec4(a_position, 1.0);
    v_world_position = world_pos.xyz;
    v_normal = normalize(mat3(u_model) * a_normal);
    v_color = a_color;
    gl_Position = u_mvp * vec4(a_position, 1.0);
}
)";

static const char* kPhongFragSrc = R"(
#version 300 es
precision mediump float;
in vec3 v_world_position;
in vec3 v_normal;
in vec3 v_color;

out vec4 o_color;

uniform vec3 u_ambientColor;
uniform float u_ambientIntensity;
uniform vec3 u_dirLightDir;
uniform vec3 u_dirLightColor;
uniform float u_dirLightIntensity;
uniform int u_dirLightEnabled;
uniform int u_numPointLights;
uniform vec3 u_ptLightPos[4];
uniform vec3 u_ptLightColor[4];
uniform float u_ptLightIntensity[4];
uniform float u_ptLightRange[4];
uniform int u_ptLightEnabled[4];
uniform int u_spotLightEnabled;
uniform vec3 u_spotLightPos;
uniform vec3 u_spotLightDir;
uniform vec3 u_spotLightColor;
uniform float u_spotLightIntensity;
uniform float u_spotLightRange;
uniform float u_spotLightInnerCos;
uniform float u_spotLightOuterCos;
uniform float u_attnConst;
uniform float u_attnLinear;
uniform float u_attnQuad;
uniform int u_useAttnFormula;
uniform vec3 u_camPos;

const float MIN_RANGE = 0.001;
const float SPOT_EPSILON = 1e-6;
const int MAX_POINT_LIGHTS = 4;

void main() {
    vec3 normal = normalize(v_normal);
    vec3 view_dir = normalize(u_camPos - v_world_position);
    vec3 lighting = u_ambientColor * u_ambientIntensity;
    if (u_dirLightEnabled != 0) {
        vec3 light_dir = normalize(-u_dirLightDir);
        float diff = max(dot(normal, light_dir), 0.0);
        vec3 half_dir = normalize(light_dir + view_dir);
        float spec = pow(max(dot(normal, half_dir), 0.0), 32.0);
        lighting += u_dirLightColor * u_dirLightIntensity * (diff * 0.8 + spec * 0.4);
    }
    for (int i = 0; i < u_numPointLights && i < MAX_POINT_LIGHTS; ++i) {
        if (u_ptLightEnabled[i] == 0) continue;
        vec3 to_light = u_ptLightPos[i] - v_world_position;
        float dist = length(to_light);
        float range = max(u_ptLightRange[i], MIN_RANGE);
        float atten;
        if (u_useAttnFormula != 0) {
            atten = 1.0 / (u_attnConst + u_attnLinear * dist + u_attnQuad * dist * dist);
            atten *= step(dist, range);
        } else {
            atten = clamp(1.0 - (dist / range), 0.0, 1.0);
            atten *= atten;
        }
        vec3 light_dir = normalize(to_light);
        float diff = max(dot(normal, light_dir), 0.0);
        vec3 half_dir = normalize(light_dir + view_dir);
        float spec = pow(max(dot(normal, half_dir), 0.0), 32.0);
        lighting += u_ptLightColor[i] * u_ptLightIntensity[i] * atten * (diff * 0.8 + spec * 0.3);
    }
    if (u_spotLightEnabled != 0) {
        vec3 to_light = u_spotLightPos - v_world_position;
        float dist = length(to_light);
        float range = max(u_spotLightRange, MIN_RANGE);
        if (dist <= range) {
            vec3 light_dir = normalize(to_light);
            float cos_theta = dot(light_dir, normalize(-u_spotLightDir));
            float spot_denom = max(u_spotLightInnerCos - u_spotLightOuterCos, SPOT_EPSILON);
            float spot = clamp((cos_theta - u_spotLightOuterCos) / spot_denom, 0.0, 1.0);
            float atten;
            if (u_useAttnFormula != 0) {
                atten = 1.0 / (u_attnConst + u_attnLinear * dist + u_attnQuad * dist * dist);
            } else {
                atten = clamp(1.0 - (dist / range), 0.0, 1.0);
                atten *= atten;
            }
            atten *= spot;
            float diff = max(dot(normal, light_dir), 0.0);
            vec3 half_dir = normalize(light_dir + view_dir);
            float spec = pow(max(dot(normal, half_dir), 0.0), 32.0);
            lighting += u_spotLightColor * u_spotLightIntensity * atten * (diff * 0.8 + spec * 0.3);
        }
    }
    o_color = vec4(v_color * lighting, 1.0);
}
)";
#endif

PhongMaterial::~PhongMaterial() {
    shutdown();
}

bool PhongMaterial::init(IRenderDevice* device) {
    if (!device) {
        return false;
    }
    device_ = device;
    shader_ = device_->compileShader(kPhongVertSrc, kPhongFragSrc);
    if (!shader_.isValid()) {
#ifdef VNE_TESTBED_LOGGING
        VNE_LOG_ERROR << "PhongMaterial: shader compile failed";
#endif
        return false;
    }
    PipelineDesc pd{};
    pd.shader = shader_;
    pd.layout = {{3}, {3}, {3}};
    pd.depth.testEnabled = true;
    pd.depth.writeEnabled = true;
    pd.rasterizer.cull = CullMode::eBack;
    pipeline_ = device_->createPipeline(pd);
    if (!pipeline_.isValid()) {
#ifdef VNE_TESTBED_LOGGING
        VNE_LOG_ERROR << "PhongMaterial: pipeline creation failed";
#endif
        device_->destroy(shader_);
        shader_ = {};
        return false;
    }
#ifdef VNE_TESTBED_LOGGING
    VNE_LOG_DEBUG << "PhongMaterial: init OK";
#endif
    return true;
}

void PhongMaterial::shutdown() {
    if (!device_) {
        return;
    }
    if (pipeline_.isValid()) {
        device_->destroy(pipeline_);
        pipeline_ = {};
    }
    if (shader_.isValid()) {
        device_->destroy(shader_);
        shader_ = {};
    }
    device_ = nullptr;
}

void PhongMaterial::setUniforms(IRenderDevice* device,
                                const vne::math::Mat4f& mvp,
                                const vne::math::Mat4f& model,
                                const vne::math::Vec3f& cam_pos,
                                const PhongLightParams& lights) const {
    if (!device || !shader_.isValid()) {
        return;
    }
    device->setMat4(shader_, "u_mvp", mvp);
    device->setMat4(shader_, "u_model", model);
    device->setVec3(shader_, "u_camPos", cam_pos);
    device->setVec3(shader_, "u_ambientColor", lights.ambient_color);
    device->setFloat(shader_, "u_ambientIntensity", lights.ambient_intensity);
    device->setVec3(shader_, "u_dirLightDir", lights.dir_light_dir);
    device->setVec3(shader_, "u_dirLightColor", lights.dir_light_color);
    device->setFloat(shader_, "u_dirLightIntensity", lights.dir_light_intensity);
    device->setInt(shader_, "u_dirLightEnabled", lights.dir_light_enabled ? 1 : 0);

    static const char* kPtPos[] = {"u_ptLightPos[0]", "u_ptLightPos[1]", "u_ptLightPos[2]", "u_ptLightPos[3]"};
    static const char* kPtColor[] = {"u_ptLightColor[0]", "u_ptLightColor[1]", "u_ptLightColor[2]", "u_ptLightColor[3]"};
    static const char* kPtIntensity[] = {"u_ptLightIntensity[0]", "u_ptLightIntensity[1]", "u_ptLightIntensity[2]", "u_ptLightIntensity[3]"};
    static const char* kPtRange[] = {"u_ptLightRange[0]", "u_ptLightRange[1]", "u_ptLightRange[2]", "u_ptLightRange[3]"};
    static const char* kPtEnabled[] = {"u_ptLightEnabled[0]", "u_ptLightEnabled[1]", "u_ptLightEnabled[2]", "u_ptLightEnabled[3]"};
    int num_point = 0;
    for (int i = 0; i < PhongLightParams::kMaxPointLights; ++i) {
        const auto& pt = lights.point_lights[i];
        device->setVec3(shader_, kPtPos[i], pt.position);
        device->setVec3(shader_, kPtColor[i], pt.color);
        device->setFloat(shader_, kPtIntensity[i], pt.intensity);
        device->setFloat(shader_, kPtRange[i], pt.range);
        device->setInt(shader_, kPtEnabled[i], pt.enabled ? 1 : 0);
        if (pt.enabled)
            num_point++;
    }
    device->setInt(shader_, "u_numPointLights", num_point);

    const auto& sp = lights.spot_light;
    device->setInt(shader_, "u_spotLightEnabled", sp.enabled ? 1 : 0);
    device->setVec3(shader_, "u_spotLightPos", sp.position);
    device->setVec3(shader_, "u_spotLightDir", sp.direction);
    device->setVec3(shader_, "u_spotLightColor", sp.color);
    device->setFloat(shader_, "u_spotLightIntensity", sp.intensity);
    device->setFloat(shader_, "u_spotLightRange", sp.range);
    device->setFloat(shader_, "u_spotLightInnerCos", std::cos(sp.inner_angle_rad));
    device->setFloat(shader_, "u_spotLightOuterCos", std::cos(sp.outer_angle_rad));

    device->setFloat(shader_, "u_attnConst", lights.attn_const);
    device->setFloat(shader_, "u_attnLinear", lights.attn_linear);
    device->setFloat(shader_, "u_attnQuad", lights.attn_quad);
    device->setInt(shader_, "u_useAttnFormula", lights.use_attn_formula ? 1 : 0);
}

}  // namespace testbed
}  // namespace vne
