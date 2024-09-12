#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine 
uniform sampler2D iMask;

// Output fragment color
out vec4 finalColor;

// for now, I'll just treat each channel as a binary flag
// r == bloom 
// g == glow
// b == occlusion 
void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 currentColor = texture(iMask, fragTexCoord);
    if (texelColor.a > 0.) {
        // blue only
        float b = max(currentColor.b, fragColor.b);
        finalColor = vec4(currentColor.r, currentColor.g, b, currentColor.a);

        // all 
        // finalColor = max(fragColor, currentColor);
    } else {
        finalColor = currentColor;
    }
}

