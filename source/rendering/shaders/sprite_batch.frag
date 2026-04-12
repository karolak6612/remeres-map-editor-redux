#version 450 core

layout (location = 0) in vec3 TexCoord;
layout (location = 1) in vec4 Tint;

layout (location = 0) out vec4 FragColor;

layout (binding = 1) uniform sampler2DArray uAtlas;

layout (binding = 0) uniform UBO {
    mat4 uMVP;
    vec4 uGlobalTint;
} ubo;

void main() {
    vec4 texel = texture(uAtlas, TexCoord);
    FragColor = texel * Tint * ubo.uGlobalTint;
}
