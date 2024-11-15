#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine:
uniform sampler2D iPalette;

// Output fragment color
out vec4 finalColor;

 #define MAXCOLOR 15.0
 #define COLORS 16.0
 #define WIDTH 256.0
 #define HEIGHT 16.0

// REGULAR POSTERIZATION SHADER 
float gamma = 0.6;
float numColors = 64.0;
vec4 quantize(vec4 color) {
    vec3 texelColor = color.rgb;
    texelColor = pow(texelColor, vec3(gamma, gamma, gamma));
    texelColor = texelColor*numColors;
    texelColor = floor(texelColor);
    texelColor = texelColor/numColors;
    texelColor = pow(texelColor, vec3(1.0/gamma));

    return vec4(texelColor, color.a);
}

// CUSTOM PALETTE POSTERIZATION
vec4 applyPalette(vec4 px) {
    float cell = px.b * MAXCOLOR;

    float cell_l = floor(cell); 
    float cell_h = ceil(cell);

    float half_px_x = 0.5 / WIDTH;
    float half_px_y = 0.5 / HEIGHT;
    float r_offset = half_px_x + px.r / COLORS * (MAXCOLOR / COLORS);
    float g_offset = half_px_y + px.g * (MAXCOLOR / COLORS);

    vec2 lut_pos_l = vec2(cell_l / COLORS + r_offset, g_offset); 
    vec2 lut_pos_h = vec2(cell_h / COLORS + r_offset, g_offset);

    vec4 graded_color_l = texture(iPalette, lut_pos_l);
    vec4 graded_color_h = texture(iPalette, lut_pos_h);

    vec4 graded_color = mix(graded_color_l, graded_color_h, fract(cell));

    return graded_color * fragColor;
}

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord.xy);
    // finalColor = quantize(texelColor);
    // finalColor = applyPalette(texelColor);
    finalColor = applyPalette(quantize(texelColor));
}
