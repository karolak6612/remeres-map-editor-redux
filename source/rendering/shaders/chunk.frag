#version 460 core
in vec3 TexCoord;
in vec4 Tint;
out vec4 FragColor;

uniform sampler2DArray uAtlas;
uniform vec4 uGlobalTint;

void main() {
    FragColor = texture(uAtlas, TexCoord) * Tint * uGlobalTint;
}
