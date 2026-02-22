#version 460 core
in vec2 TexCoord;
in vec4 LightColor;
in float Intensity;
out vec4 FragColor;

void main() {
    // Simple radial falloff
    float dist = length(TexCoord - 0.5) * 2.0;
    if (dist > 1.0) discard;

    float alpha = (1.0 - dist) * (1.0 - dist); // Quadratic falloff

    // Additive blend: SRC_ALPHA, ONE. Output must be pre-multiplied by alpha logic?
    FragColor = vec4(LightColor.rgb * Intensity, alpha);
}
