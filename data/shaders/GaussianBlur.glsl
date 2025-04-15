uniform sampler2D texture0;
uniform vec2 _Resolution;

out vec4 finalColor;

float normpdf(in float x, in float sigma)
{
    return 0.39894 * exp(-0.5 * x * x / (sigma * sigma)) / sigma;
}

void fragment() {
    vec3 c = texture(texture0, UV).rgb;

    // can't make this a uniform because it needs to be const...
    const int mSize = 3;
    const int kSize = (mSize - 1) / 2;
    float kernel[mSize];
    vec3 final_colour = vec3(0.0);

    //create the 1-D kernel
    float sigma = 7.0;
    float Z = 0.0;
    for (int j = 0; j <= kSize; ++j)
    {
        kernel[kSize + j] = kernel[kSize - j] = normpdf(float(j), sigma);
    }

    //get the normalization factor (as the gaussian has been clamped)
    for (int j = 0; j < mSize; ++j)
    {
        Z += kernel[j];
    }

    vec2 coord = UV * _Resolution;
    //read out the texels
    for (int i = -kSize; i <= kSize; ++i)
    {
        for (int j = -kSize; j <= kSize; ++j)
        {
            vec2 sampleCoord = (coord.xy + vec2(float(i), float(j))) / _Resolution;
            sampleCoord = clamp(sampleCoord, vec2(0.), vec2(0.999999)); // no texture wrapping
            final_colour += kernel[kSize + j] * kernel[kSize + i] * texture(texture0, sampleCoord).rgb;
        }
    }

    const float alpha = 1.0;
    finalColor = vec4(final_colour / (Z * Z), alpha);
}
