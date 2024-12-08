#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;

// mine:

// Output fragment color
out vec4 finalColor;

void main() {
    vec2 nearestSeed = texture(texture0, fragTexCoord).xy;
    float distance = clamp(distance(fragTexCoord, nearestSeed), 0.0, 1.0);
    finalColor = vec4(vec3(distance), 1.0);
}

