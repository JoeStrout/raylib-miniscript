#version 330
in vec2 fragTexCoord;
in vec4 fragColor;
in vec3 fragNormal;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 uLightDir;   // direction TOWARDS the light, world space
out vec4 finalColor;
void main() {
    vec4 base = texture(texture0, fragTexCoord) * colDiffuse * fragColor;
    float ndl = max(dot(normalize(fragNormal), normalize(uLightDir)), 0.0);
    finalColor = vec4(base.rgb * (vec3(0.2) + vec3(ndl)), base.a);
}
