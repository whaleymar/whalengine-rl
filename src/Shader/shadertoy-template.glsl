#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine:
uniform float iTime;

// Output fragment color
out vec4 finalColor;

vec2 norm(vec2 coord) {
    return vec2(coord.x/2. + 1., coord.y/2. + 1.);
}

const vec2 iResolution = vec2(320, 180);

void main() {
    // iChannel0 -> texture0 
    // fragColor -> finalColor 
    // fragCoord = fragTexCoord * iResolution
}

