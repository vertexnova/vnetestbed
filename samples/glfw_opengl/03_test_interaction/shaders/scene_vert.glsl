#version 410 core
/*
 * Shader: scene.vert
 * Description: Vertex shader for test scene cubes (position, normal, color).
 *
 * Inputs:  a_position (vec3), a_normal (vec3), a_color (vec3)
 * Outputs: v_world_position (vec3), v_normal (vec3), v_color (vec3)
 * Uniforms: u_mvp (mat4), u_model (mat4)
 */

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
