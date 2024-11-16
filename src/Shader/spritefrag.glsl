#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
// in vec3 hdrColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// NOTE: Add here your custom variables

void main() {
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);

    // vec4 hdrColorFull = vec4(hdrColor, 1.);
    // vec4 hdrColorFull = vec4(1.);
    // finalColor = texelColor * fragColor * hdrColorFull;

    // finalColor = texelColor * fragColor;
    finalColor = texelColor * fragColor;
}
