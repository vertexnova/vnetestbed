#version 410 core
in  vec3 vWorldPos;
in  vec3 vNormal;
in  vec3 vColor;
out vec4 FragColor;

uniform vec3  u_AmbientColor;
uniform float u_AmbientIntensity;

uniform vec3  u_DirLightDir;
uniform vec3  u_DirLightColor;
uniform float u_DirLightIntensity;
uniform int   u_DirLightEnabled;

uniform int   u_NumPointLights;
uniform vec3  u_PtLightPos[4];
uniform vec3  u_PtLightColor[4];
uniform float u_PtLightIntensity[4];
uniform float u_PtLightRange[4];
uniform int   u_PtLightEnabled[4];

uniform int   u_SpotLightEnabled;
uniform vec3  u_SpotLightPos;
uniform vec3  u_SpotLightDir;
uniform vec3  u_SpotLightColor;
uniform float u_SpotLightIntensity;
uniform float u_SpotLightRange;
uniform float u_SpotLightInnerCos;
uniform float u_SpotLightOuterCos;

uniform float u_AttnConst;
uniform float u_AttnLinear;
uniform float u_AttnQuad;
uniform int   u_UseAttnFormula;

uniform vec3  u_CamPos;

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(u_CamPos - vWorldPos);
    vec3 lighting = u_AmbientColor * u_AmbientIntensity;

    if (u_DirLightEnabled != 0) {
        vec3 L = normalize(-u_DirLightDir);
        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        lighting += u_DirLightColor * u_DirLightIntensity * (diff * 0.8 + spec * 0.4);
    }

    for (int i = 0; i < u_NumPointLights && i < 4; ++i) {
        if (u_PtLightEnabled[i] == 0) continue;
        vec3  toLight = u_PtLightPos[i] - vWorldPos;
        float dist    = length(toLight);
        float range   = max(u_PtLightRange[i], 0.001);
        float atten;
        if (u_UseAttnFormula != 0) {
            atten = 1.0 / (u_AttnConst + u_AttnLinear * dist + u_AttnQuad * dist * dist);
            atten *= step(dist, range);
        } else {
            atten = clamp(1.0 - (dist / range), 0.0, 1.0);
            atten *= atten;
        }
        vec3 L = normalize(toLight);
        float diff = max(dot(N, L), 0.0);
        vec3 H = normalize(L + V);
        float spec = pow(max(dot(N, H), 0.0), 32.0);
        lighting += u_PtLightColor[i] * u_PtLightIntensity[i] * atten * (diff * 0.8 + spec * 0.3);
    }

    if (u_SpotLightEnabled != 0) {
        vec3  toLight = u_SpotLightPos - vWorldPos;
        float dist    = length(toLight);
        float range   = max(u_SpotLightRange, 0.001);
        if (dist <= range) {
            vec3 L = normalize(toLight);
            float cosTheta = dot(L, normalize(-u_SpotLightDir));
            float spot = clamp((cosTheta - u_SpotLightOuterCos) / (u_SpotLightInnerCos - u_SpotLightOuterCos), 0.0, 1.0);
            float atten;
            if (u_UseAttnFormula != 0) {
                atten = 1.0 / (u_AttnConst + u_AttnLinear * dist + u_AttnQuad * dist * dist);
            } else {
                atten = clamp(1.0 - (dist / range), 0.0, 1.0);
                atten *= atten;
            }
            atten *= spot;
            float diff = max(dot(N, L), 0.0);
            vec3 H = normalize(L + V);
            float spec = pow(max(dot(N, H), 0.0), 32.0);
            lighting += u_SpotLightColor * u_SpotLightIntensity * atten * (diff * 0.8 + spec * 0.3);
        }
    }

    FragColor = vec4(vColor * lighting, 1.0);
}
