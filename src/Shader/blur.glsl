#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine:
// uniform float iTime;
uniform vec2 iResolution;

// Output fragment color
out vec4 finalColor;

// iChannel0 -> texture0 
// fragColor -> finalColor 
// fragCoord = fragTexCoord * iResolution

float normpdf(in float x, in float sigma)
{
	return 0.39894*exp(-0.5*x*x/(sigma*sigma))/sigma;
}


void main() {
	vec3 c = texture(texture0, fragTexCoord).rgb;

    //declare stuff
    // const int mSize = 11; // try this at full resolution
    const int mSize = 5;
    const int kSize = (mSize-1)/2;
    float kernel[mSize];
    vec3 final_colour = vec3(0.0);

    //create the 1-D kernel
    float sigma = 7.0;
    float Z = 0.0;
    for (int j = 0; j <= kSize; ++j)
    {
        kernel[kSize+j] = kernel[kSize-j] = normpdf(float(j), sigma);
    }

    //get the normalization factor (as the gaussian has been clamped)
    for (int j = 0; j < mSize; ++j)
    {
        Z += kernel[j];
    }

    vec2 coord = fragTexCoord * iResolution;
    //read out the texels
    for (int i=-kSize; i <= kSize; ++i)
    {
        for (int j=-kSize; j <= kSize; ++j)
        {
            vec2 sampleCoord = (coord.xy+vec2(float(i),float(j)))/iResolution;
            sampleCoord = clamp(sampleCoord, vec2(0.), vec2(0.999999)); // no texture wrapping
            final_colour += kernel[kSize+j]*kernel[kSize+i]*texture(texture0, sampleCoord).rgb;

        }
    }


    const float alpha = 1.0;
    finalColor = vec4(final_colour/(Z*Z), alpha);
}
