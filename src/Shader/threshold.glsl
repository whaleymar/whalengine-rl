#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine:
uniform float lum_threshold;

// Output fragment color
out vec4 finalColor;

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);

    // Get luminance. Weights from LearnOpenGL
    float lum = dot(texelColor.rgb, vec3(0.2126, 0.7152, 0.0722));
    if (lum > lum_threshold) {
        finalColor = vec4(texelColor.rgb, 1.0);
    } else {
        finalColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}


