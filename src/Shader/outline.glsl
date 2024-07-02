#version 330

in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// const float stepX = 1./640.;
const float stepX = 1./320.;
// const float stepX = 1./180.;
const float stepY = 1./180.;
const vec2 up = vec2(0., stepY);
const vec2 down = vec2(0., -stepY);
const vec2 left = vec2(-stepX, 0.);
const vec2 right = vec2(stepX, 0.);
const vec4 black = vec4(0., 0., 0., 1.);

void main() {
    // this works for up/down, but is broken for left/right 
    // also doesn't add outline for sprites which are against the edge of their frame

    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 color = vec4(texelColor.rgb, texelColor.a * fragColor.a);
    if (color.a <= 0.01) {
        // sample up/down/left/right 

        texelColor = texture(texture0, fragTexCoord + up);
        if (texelColor.a > 0.01) {
            finalColor = black;
            return;
        }

        texelColor = texture(texture0, fragTexCoord + down);
        if (texelColor.a > 0.01) {
            finalColor = black;
            return;
        }

        texelColor = texture(texture0, fragTexCoord + left);
        if (texelColor.a > 0.01) {
            finalColor = black;
            return;
        }

        texelColor = texture(texture0, fragTexCoord + right);
        if (texelColor.a > 0.01) {
            finalColor = black;
            return;
        }

        finalColor = vec4(0., 0., 0., 0.);


    } else {
        finalColor = color;
    }
}
