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

// const float N_COLORS_R = 4.;
// const float N_COLORS_G = 4.;
// const float N_COLORS_B = 4.;
// const float N_COLORS_R = 16.;
// const float N_COLORS_G = 16.;
// const float N_COLORS_B = 16.;
//
// void main() {
//     vec3 texelColor = texture(texture0, fragTexCoord.xy).rgb;
//
//     float r = texelColor.r * N_COLORS_R ;
//     float g = texelColor.g * N_COLORS_G ;
//     float b = texelColor.b * N_COLORS_B;
//     float num = 1./256.;
//
//     float r = 256. * texelColor.r;
//     float g = 256. * texelColor.g;
//     float x = floor(r * 256. + r * g * N_COLORS_G) * num;
//     float y = texelColor.b;
//     finalColor = vec4(texture(palette, vec2(x,y)).rgb, 1.0);
// }

 #define MAXCOLOR 15.0
 #define COLORS 16.0
 #define WIDTH 256.0
 #define HEIGHT 16.0

void main()
{
    vec4 px = texture(texture0, fragTexCoord.xy);
    // finalColor = px; // just making sure this changes nothing

    float cell = px.b * MAXCOLOR;

    float cell_l = floor(cell); 
    float cell_h = ceil(cell);

    float half_px_x = 0.5 / WIDTH;
    float half_px_y = 0.5 / HEIGHT;
    float r_offset = half_px_x + px.r / COLORS * (MAXCOLOR / COLORS);
    float g_offset = half_px_y + px.g * (MAXCOLOR / COLORS);

    vec2 lut_pos_l = vec2(cell_l / COLORS + r_offset, g_offset); 
    vec2 lut_pos_h = vec2(cell_h / COLORS + r_offset, g_offset);

    vec4 graded_color_l = texture(palette, lut_pos_l);
    vec4 graded_color_h = texture(palette, lut_pos_h);

    vec4 graded_color = mix(graded_color_l, graded_color_h, fract(cell));

    finalColor = graded_color;
}
