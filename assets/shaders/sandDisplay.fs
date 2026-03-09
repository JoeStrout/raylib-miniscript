// Sand display shader — renders state texture with nice colors.
// Alpha encoding: 0 = empty, ~0.5 = obstacle, ~1.0 = sand.

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec3 uBgColor;

void main() {
    vec4 state = texture(texture0, fragTexCoord);

    if (state.a > 0.7) {
        // Sand grain
        finalColor = vec4(state.rgb, 1.0);
    } else if (state.a > 0.3) {
        // Obstacle
        finalColor = vec4(state.rgb, 1.0);
    } else {
        // Empty
        finalColor = vec4(uBgColor, 1.0);
    }
}
