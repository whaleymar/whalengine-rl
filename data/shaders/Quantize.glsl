uniform sampler2D texture0;
uniform sampler2D _Palette;

// let CPC = Colors Per Channel
// size is CPC*CPC X CPC (fake 3D texture)
// so a 16 CPC Lookup table would be 256x16 pixels
uniform vec2 _PaletteTexSize;

out vec4 finalColor;

// REGULAR QUNTIZATION SHADER
// float gamma = 0.6;
// vec4 quantize(vec4 color) {
//     float COLORS = _PaletteTexSize.y;
//     vec3 texelColor = color.rgb;
//     texelColor = pow(texelColor, vec3(gamma, gamma, gamma));
//     texelColor = texelColor * COLORS;
//     texelColor = floor(texelColor);
//     texelColor = texelColor / COLORS;
//     texelColor = pow(texelColor, vec3(1.0 / gamma));
//
//     return vec4(texelColor, color.a);
// }

// CUSTOM PALETTE POSTERIZATION
vec3 applyPalette(vec3 px) {
    float COLORS = _PaletteTexSize.y;
    float MAXCOLOR = COLORS - 1.0;
    float cell = px.b * MAXCOLOR;

    float cell_l = floor(cell);
    float cell_h = ceil(cell);

    float half_px_x = 0.5 / _PaletteTexSize.x;
    float half_px_y = 0.5 / _PaletteTexSize.y;
    float r_offset = half_px_x + px.r / COLORS * (MAXCOLOR / COLORS);
    float g_offset = half_px_y + px.g * (MAXCOLOR / COLORS);

    vec2 lut_pos_l = vec2(cell_l / COLORS + r_offset, g_offset);
    vec2 lut_pos_h = vec2(cell_h / COLORS + r_offset, g_offset);

    vec4 graded_color_l = texture(_Palette, lut_pos_l);
    vec4 graded_color_h = texture(_Palette, lut_pos_h);

    vec4 graded_color = mix(graded_color_l, graded_color_h, fract(cell));

    return graded_color.rgb;
}

void fragment() {
    vec4 texelColor = texture(texture0, UV.xy);

    // get brightest channel
    float r = texelColor.r;
    float g = texelColor.g;
    float b = texelColor.b;
    float maxChan = r > g ? (r > b ? r : b) : (g > b ? g : b);

    float a = texelColor.a;

    if (maxChan > 1.) {
        // convert to LDR, posterize, then go back to HDR
        vec3 query = texelColor.rgb / maxChan;
        vec3 newCol = applyPalette(query);
        finalColor = vec4(newCol * maxChan, a);
    } else {
        // normal LDR posterization
        finalColor = vec4(applyPalette(texelColor.rgb), a);
    }

    // quantization on pixel art seems unecessary
    // finalColor = quantize(texelColor);
    // finalColor = applyPalette(quantize(texelColor));
}
