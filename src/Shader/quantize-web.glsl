#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform sampler2D palette;
uniform vec4 colDiffuse;

const float N_COLORS_R = 4.;
const float N_COLORS_G = 4.;
const float N_COLORS_B = 4.;

void main() {
    vec3 texelColor = texture2D(texture0, fragTexCoord.xy).rgb;

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
    gl_FragColor = vec4(texture2D(palette, vec2(x,y)).rgb, 1.0);
}

