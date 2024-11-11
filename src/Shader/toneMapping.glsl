#version 330 

// Input HDR color texture
uniform sampler2D texture0; // hdrTexture
// Exposure level for tone mapping
// uniform float exposure;

// Texture coordinates
in vec2 fragTexCoord;
// Final output color
out vec4 finalColor;

// REFERENCE 
// https://64.github.io/tonemapping/

const float exposure = 1.0; // unused

float luminance(vec3 v) {
    return dot(v, vec3(0.2126f, 0.7152f, 0.0722f));
}

vec3 change_luminance(vec3 c_in, float l_out) {
    float l_in = luminance(c_in);
    return c_in * (l_out / l_in);
}

// approximation
vec3 ACESFilm(vec3 x) {
    const float a = 2.51;
    const float b = 0.03;
    const float c = 2.43;
    const float d = 0.59;
    const float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 reinhard(vec3 hdrColor) {
    // Apply exposure
    vec3 mappedColor = vec3(1.0) - exp(-hdrColor * exposure);

    // Reinhard tone mapping
    mappedColor = mappedColor / (mappedColor + vec3(1.0));

    // Gamma correction (optional, usually gamma 2.2)
    // mappedColor = pow(mappedColor, vec3(1.0 / 2.2));

    return mappedColor;
}

vec3 reinhard_extended(vec3 v, float max_white) {
    vec3 numerator = v * (1.0f + (v / vec3(max_white * max_white)));
    return numerator / (1.0f + v);
}

// operates on luminance instead of color channels. Looks better IMO
vec3 reinhard_extended_luminance(vec3 v, float max_white_l) {
    float l_old = luminance(v);
    float numerator = l_old * (1.0f + (l_old / (max_white_l * max_white_l)));
    float l_new = numerator / (1.0f + l_old);
    return change_luminance(v, l_new);
}

void main() {
    // Sample the HDR texture
    vec3 hdrColor = texture(texture0, fragTexCoord).rgb;

    // vec3 mappedColor = reinhard(hdrColor);
    // vec3 mappedColor = ACESFilm(hdrColor * exposure);
    // vec3 mappedColor = ACESFilm(hdrColor);
    vec3 mappedColor = reinhard_extended_luminance(hdrColor, 1.5);

    // Output the tone-mapped color
    finalColor = vec4(mappedColor, 1.0);
}
