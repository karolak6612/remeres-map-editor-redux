#version 460 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;
layout (location = 2) in vec4 aRect;
layout (location = 3) in vec4 aUV;
layout (location = 4) in vec4 aTint;
layout (location = 5) in float aLayer;

out vec3 TexCoord;
out vec4 Tint;

uniform mat4 uMVP;

void main() {
    vec2 pos = aRect.xy + aPos * aRect.zw;
    gl_Position = uMVP * vec4(pos, 0.0, 1.0);
    TexCoord = vec3(mix(aUV.xy, aUV.zw, aTexCoord), aLayer);
    Tint = aTint;
}
