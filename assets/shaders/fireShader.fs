// Fire background shader for main menu

#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform float uTime;

out vec4 finalColor;

// Simple hash-based noise
float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float noise(vec2 p) {
    vec2 i = floor(p);
    vec2 f = fract(p);
    f = f * f * (3.0 - 2.0 * f);
    float a = hash(i);
    float b = hash(i + vec2(1.0, 0.0));
    float c = hash(i + vec2(0.0, 1.0));
    float d = hash(i + vec2(1.0, 1.0));
    return mix(mix(a, b, f.x), mix(c, d, f.x), f.y);
}

float fbm(vec2 p) {
    float v = 0.0;
    float a = 0.5;
    vec2 shift = vec2(100.0);
    for (int i = 0; i < 5; i++) {
        v += a * noise(p);
        p = p * 2.0 + shift;
        a *= 0.5;
    }
    return v;
}

void main() {
    vec2 uv = fragTexCoord;

    // Scroll downward (screen-space up) and scale for fire look
    float t = uTime * 0.8;
    vec2 fireUV = vec2(uv.x * 3.0, uv.y * 4.0 + t);

    // Layered turbulence
    float n = fbm(fireUV);
    n += 0.5 * fbm(fireUV * 2.0 + vec2(t * 0.3, t * 0.5));

    // Fade: intensify at bottom of screen (high uv.y), fade at top (low uv.y)
    float heightFade = pow(uv.y, 1.8);
    n *= heightFade;

    // Also fade at left/right edges
    float edgeFade = smoothstep(0.0, 0.15, uv.x) * smoothstep(1.0, 0.85, uv.x);
    n *= edgeFade;

    // Fire color ramp: black -> dark red -> orange -> yellow -> white
    vec3 col = vec3(0.0);
    col = mix(col, vec3(0.5, 0.0, 0.0), smoothstep(0.1, 0.3, n));
    col = mix(col, vec3(1.0, 0.3, 0.0), smoothstep(0.3, 0.5, n));
    col = mix(col, vec3(1.0, 0.7, 0.1), smoothstep(0.5, 0.7, n));
    col = mix(col, vec3(1.0, 1.0, 0.8), smoothstep(0.7, 0.9, n));

    // Overall intensity control
    float alpha = smoothstep(0.05, 0.2, n) * 0.85;

    finalColor = vec4(col * alpha, 1.0);
}
