#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D palette;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;


// float gamma = 0.6;
// float numColors = 8.0;
//
// void main()
// {
//     // Texel color fetching from texture sampler
//     vec3 texelColor = texture(texture0, fragTexCoord.xy).rgb;
//
//     texelColor = pow(texelColor, vec3(gamma, gamma, gamma));
//     texelColor = texelColor*numColors;
//     texelColor = floor(texelColor);
//     texelColor = texelColor/numColors;
//     texelColor = pow(texelColor, vec3(1.0/gamma));
//
//     finalColor = vec4(texelColor, 1.0);
// }

const float N_COLORS_R = 4.;
const float N_COLORS_G = 4.;
const float N_COLORS_B = 4.;

void main() {
    vec3 texelColor = texture(texture0, fragTexCoord.xy).rgb;

    float r = texelColor.r * N_COLORS_R ;
    float g = texelColor.g * N_COLORS_G ;
    float b = texelColor.b * N_COLORS_B;
    float num = 1./16.;

    int iR = int(floor(r));
    if (iR==4) {
        iR = 3;
    }
    int iG = int(floor(g));
    if (iG == 4) {
        iG = 3;
    }
    r = float(iR);
    g = float(iG);
    float x = (r + g * N_COLORS_G) * num;


    float y = texelColor.b;
    finalColor = vec4(texture(palette, vec2(x,y)).rgb, 1.0);
}
