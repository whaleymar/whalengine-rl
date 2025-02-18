#version 330
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 Depth;

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in float fragDepth;
// in float isOccluder;
in float isUI;
in vec2 WorldPosition;

uniform sampler2D texture0;
uniform sampler2D _Overlay;

// ratio should be (1/virtual_screen_ratio) / (texture_size)
uniform vec2 _Scale;
uniform float _Time;

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 overlayColor = texture(_Overlay, WorldPosition * _Scale);
    FragColor = texelColor * overlayColor;

    // To make things more visible when debugging, scale the colors
    // During release, this can just be 1.0
    const float scalar = 20.0;
    if (isUI < 0.5) {
        Depth = vec4(fragDepth * scalar, 0., 0., texelColor.a);
    }
}
