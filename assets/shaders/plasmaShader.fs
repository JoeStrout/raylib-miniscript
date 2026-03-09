// Plasma shader demo -- procedural animated plasma effect.

#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float uTime;
uniform vec2 uResolution;
uniform vec3 uPalette;    // RGB shift offsets for palette variety

out vec4 finalColor;

void main() {
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;

    float t = uTime * 0.6;

    // Layered sine waves create the plasma effect
    float v = 0.0;
    v += sin(uv.x * 3.0 + t);
    v += sin(uv.y * 4.0 - t * 0.7);
    v += sin((uv.x + uv.y) * 2.5 + t * 1.3);
    v += sin(length(uv * 3.0) - t * 1.1);
    v += sin(length(uv - vec2(sin(t * 0.3), cos(t * 0.5))) * 5.0);
    v *= 0.5;

    // Map to color using offset sine palette
    vec3 col;
    col.r = 0.5 + 0.5 * sin(v * 3.14159 + uPalette.r);
    col.g = 0.5 + 0.5 * sin(v * 3.14159 + uPalette.g + 2.094);
    col.b = 0.5 + 0.5 * sin(v * 3.14159 + uPalette.b + 4.189);

    finalColor = vec4(col, 1.0);
}
