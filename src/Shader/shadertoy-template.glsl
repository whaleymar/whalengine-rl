#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine:
// uniform float iTime;
// uniform vec2 iResolution;

// Output fragment color
out vec4 finalColor;

void main() {
    // iChannel0 -> texture0 
    // fragColor -> finalColor 
    // fragCoord = fragTexCoord * iResolution
}

