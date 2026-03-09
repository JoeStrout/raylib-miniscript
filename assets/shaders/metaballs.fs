// Metaball shader -- animated blobby shapes that merge together.

#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform float uTime;
uniform vec2 uResolution;
uniform vec3 uColorA;      // primary blob color
uniform vec3 uColorB;      // secondary / background glow color

out vec4 finalColor;

// Each metaball contributes 1/r^2 falloff
float metaball(vec2 p, vec2 center, float radius) {
    float d = length(p - center);
    float r2 = radius * radius;
    return r2 / (d * d + 0.0001);
}

void main() {
    vec2 uv = fragTexCoord * 2.0 - 1.0;
    uv.x *= uResolution.x / uResolution.y;

    float t = uTime * 0.5;

    // Animate 6 metaball positions in looping orbits
    float field = 0.0;
    field += metaball(uv, vec2(sin(t * 1.1) * 0.6,  cos(t * 0.9) * 0.6),  0.30);
    field += metaball(uv, vec2(cos(t * 0.8) * 0.7,  sin(t * 1.3) * 0.5),  0.25);
    field += metaball(uv, vec2(sin(t * 1.4) * 0.5, -cos(t * 1.0) * 0.7),  0.28);
    field += metaball(uv, vec2(-cos(t * 0.7) * 0.6, sin(t * 1.2) * 0.6),  0.22);
    field += metaball(uv, vec2(sin(t * 0.6 + 2.0) * 0.8, cos(t * 0.5 + 1.0) * 0.4), 0.26);
    field += metaball(uv, vec2(cos(t * 1.5) * 0.4, sin(t * 0.4) * 0.8),   0.20);

    // Threshold: values above 1.0 are "inside" the blobs
    float blob = smoothstep(0.9, 1.1, field);

    // Internal glow based on field intensity
    float glow = smoothstep(0.4, 1.0, field);

    // Color: blend between background glow and blob color
    vec3 bgCol = uColorB * glow * 0.4;
    vec3 blobCol = mix(uColorA, uColorB, sin(field * 2.0) * 0.5 + 0.5);
    vec3 col = mix(bgCol, blobCol, blob);

    // Add bright edge highlight at the blob boundary
    float edge = smoothstep(0.85, 1.0, field) - smoothstep(1.0, 1.15, field);
    col += vec3(1.0) * edge * 0.6;

    finalColor = vec4(col, 1.0);
}
