#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform sampler2D uExtraTex;
uniform vec4 colDiffuse;
uniform float uTime;
uniform vec3 uRgb;
uniform mat4 uTintMat;

out vec4 finalColor;

void main() {
    vec4 base = texture(texture0, fragTexCoord);
    vec4 extra = texture(uExtraTex, fragTexCoord * 2.0);
    float pulse = 0.5 + 0.5 * sin(uTime);
    float m = uTintMat[0][0];
    vec3 tint = uRgb * mix(vec3(0.5), extra.rgb, 0.35);
    finalColor = vec4(base.rgb * tint * pulse * m, base.a) * colDiffuse * fragColor;
}
