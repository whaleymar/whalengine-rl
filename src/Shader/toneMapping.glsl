#version 330 

// Input HDR color texture
uniform sampler2D texture0; // hdrTexture
// Exposure level for tone mapping
uniform float exposure;

// Texture coordinates
in vec2 fragTexCoord;
// Final output color
out vec4 finalColor;

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

void main() {
    // Sample the HDR texture
    vec3 hdrColor = texture(texture0, fragTexCoord).rgb;

    // testing
    // if (hdrColor.r > 1.) {
    //     finalColor = vec4(1., 0., 0., 1.);
    // } else {
    //     finalColor = vec4(0., 1., 0., 1.);
    // }

    vec3 mappedColor = reinhard(hdrColor);
    // vec3 mappedColor = ACESFilm(hdrColor * exposure);

    // Output the tone-mapped color
    finalColor = vec4(mappedColor, 1.0);
}
