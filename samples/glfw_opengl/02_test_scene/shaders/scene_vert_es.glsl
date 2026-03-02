#version 300 es
layout(location=0) in vec3 aPos;
layout(location=1) in vec3 aNormal;
layout(location=2) in vec3 aColor;
uniform mat4 u_MVP;
uniform mat4 u_Model;
out vec3 vWorldPos;
out vec3 vNormal;
out vec3 vColor;
void main() {
    vec4 wp = u_Model * vec4(aPos, 1.0);
    vWorldPos = wp.xyz;
    vNormal   = normalize(mat3(u_Model) * aNormal);
    vColor    = aColor;
    gl_Position = u_MVP * vec4(aPos, 1.0);
}
