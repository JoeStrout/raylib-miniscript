// Sand simulation fragment shader (ping-pong cellular automaton).
// All state is encoded in texture0's alpha channel:
//   alpha = 0:         empty
//   alpha in (0.3,0.7): obstacle (rgb = obstacle color)
//   alpha > 0.7:       sand grain (rgb = grain color)
//
// Coordinate convention: fragTexCoord.y increases downward (gravity direction).

#version 330

in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0;
uniform vec2 uTexelSize;
uniform float uTime;
uniform int uPass;

bool isSand(vec4 c) {
    return c.a > 0.7;
}

bool isObstacle(vec4 c) {
    return c.a > 0.3 && c.a <= 0.7;
}

bool isBlocked(vec2 uv) {
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return true;
    vec4 c = texture(texture0, uv);
    return isObstacle(c) || isSand(c);
}

bool isEmpty(vec2 uv) {
    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) return false;
    vec4 c = texture(texture0, uv);
    return c.a < 0.1;
}

float hash(vec2 p) {
    return fract(sin(dot(p, vec2(127.1, 311.7)) + uTime * 17.13) * 43758.5453);
}

void main() {
    vec2 uv = fragTexCoord;
    vec2 dx = vec2(uTexelSize.x, 0.0);
    vec2 dy = vec2(0.0, uTexelSize.y);

    vec4 me = texture(texture0, uv);

    // Obstacles are immutable — always pass through
    if (isObstacle(me)) {
        finalColor = me;
        return;
    }

    // -dy = downward on screen (due to Y-flip in RT-to-RT draw)
    vec2 below  = uv - dy;
    vec2 above  = uv + dy;
    vec2 belowL = below - dx;
    vec2 belowR = below + dx;
    vec2 aboveL = above - dx;
    vec2 aboveR = above + dx;

    ivec2 coord = ivec2(uv / uTexelSize);
    float rnd = hash(vec2(coord));
    bool preferLeft = ((coord.x + uPass) % 2 == 0);

    if (isSand(me)) {
        // Try to fall straight down
        if (isEmpty(below)) {
            finalColor = vec4(0.0);
            return;
        }
        // Try diagonal
        bool bLEmpty = isEmpty(belowL);
        bool bREmpty = isEmpty(belowR);
        if (bLEmpty && bREmpty) {
            finalColor = vec4(0.0);
            return;
        } else if (preferLeft && bLEmpty) {
            finalColor = vec4(0.0);
            return;
        } else if (!preferLeft && bREmpty) {
            finalColor = vec4(0.0);
            return;
        }
        // Can't move
        finalColor = me;
    } else {
        // I'm empty — could sand arrive from above?
        vec4 aboveCell = texture(texture0, above);
        if (isSand(aboveCell) && isEmpty(uv)) {
            finalColor = aboveCell;
            return;
        }

        // Diagonal arrivals
        vec4 aLCell = texture(texture0, aboveL);
        vec4 aRCell = texture(texture0, aboveR);
        bool aLSand = isSand(aLCell);
        bool aRSand = isSand(aRCell);

        bool aLWants = false;
        if (aLSand) {
            // above-left's below = aboveL - dy; if blocked, it wants to slide
            if (!isEmpty(aboveL - dy)) {
                // above-left's other diagonal (its below-left) = aboveL - dy - dx
                bool aLBelowLEmpty = isEmpty(aboveL - dy - dx);
                bool aLPreferRight = ((coord.x - 1 + uPass) % 2 != 0);
                aLWants = !aLBelowLEmpty || aLPreferRight;
            }
        }

        bool aRWants = false;
        if (aRSand) {
            // above-right's below = aboveR - dy; if blocked, it wants to slide
            if (!isEmpty(aboveR - dy)) {
                // above-right's other diagonal (its below-right) = aboveR - dy + dx
                bool aRBelowREmpty = isEmpty(aboveR - dy + dx);
                bool aRPreferLeft = ((coord.x + 1 + uPass) % 2 == 0);
                aRWants = !aRBelowREmpty || aRPreferLeft;
            }
        }

        if (aLWants && aRWants) {
            finalColor = (rnd > 0.5) ? aLCell : aRCell;
        } else if (aLWants) {
            finalColor = aLCell;
        } else if (aRWants) {
            finalColor = aRCell;
        } else {
            finalColor = vec4(0.0);
        }
    }
}
