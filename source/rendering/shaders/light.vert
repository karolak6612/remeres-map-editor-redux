#version 460 core
layout (location = 0) in vec2 aPos; // Quad vertices
layout (location = 1) in vec2 aTexCoord; // Quad UVs
layout (location = 2) in vec2 aLightPos; // Instance Pos
layout (location = 3) in vec2 aLightParams; // Instance Color (packed), Intensity

out vec2 TexCoord;
out vec4 LightColor;
out float Intensity;

uniform mat4 uMVP;

vec4 unpackColor(float f) {
    uint u = floatBitsToUint(f);
    return vec4(
        float((u >> 0) & 0xFF) / 255.0,
        float((u >> 8) & 0xFF) / 255.0,
        float((u >> 16) & 0xFF) / 255.0,
        1.0
    );
}

void main() {
    // Light sprite size depends on intensity.
    float size = aLightParams.y * 64.0; // Arbitrary scale factor

    vec2 pos = aLightPos + (aPos - 0.5) * size; // Center quad on position
    gl_Position = uMVP * vec4(pos, 0.0, 1.0);
    TexCoord = aTexCoord;
    LightColor = unpackColor(aLightParams.x);
    Intensity = aLightParams.y;
}
