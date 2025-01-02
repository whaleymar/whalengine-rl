#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in vec2 WorldPosition;

// Input uniform values
// default:
uniform sampler2D texture0;

// mine:
uniform sampler2D _Overlay;

// ratio should be (1/virtual_screen_ratio) / (texture_size) 
uniform vec2 _Scale;

// Output fragment color
out vec4 finalColor;

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 overlayColor = texture(_Overlay, WorldPosition * _Scale);
    finalColor = texelColor * overlayColor;
}

