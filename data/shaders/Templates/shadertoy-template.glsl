#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine:
// global uniform float _Time;
// global uniform vec2 _GameResolution;

out vec4 finalColor;

void fragment() {
    // iChannel0 -> texture0
    // fragColor -> finalColor
    // fragCoord = fragTexCoord * _GameResolution
}
