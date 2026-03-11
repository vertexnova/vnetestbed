#version 300 es
precision mediump float;
/*
 * Shader: scene.frag (OpenGL ES)
 * Description: Blinn-Phong lighting (ambient, directional, point, spot).
 *
 * Inputs:  v_world_position (vec3), v_normal (vec3), v_color (vec3)
 * Outputs: o_color (vec4)
 * Uniforms: ambient, directional, point lights [4], spot light, attenuation, u_camPos
 */

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
