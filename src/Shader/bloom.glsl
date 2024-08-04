#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine:
uniform float iTime;
uniform vec2 iResolution;

// Output fragment color
out vec4 finalColor;

// const float samples = 5.;          // finalColors per axis; higher = bigger glow, worse performance. MIN 3
// const float quality = 1.;          // Defines size factor: Lower = smaller glow, better quality

// playing w/ new values
const float samples = 13.;          // finalColors per axis; higher = bigger glow, worse performance. MIN 3
const float quality = 1.;          // Defines size factor: Lower = smaller glow, better quality

// super over-the-top settings, for testing if the shader is working
// const float samples = 5.;          
// const float quality = 3.;

// TODO needs HDR + tone mapping step first? So i just extract the bright stuff and don't bloom things like black outlines

// can set this below 1. if i only want to bloom the outside of the object
const float maxAlpha = 1.1;

void main()
{
    vec4 sum = vec4(0);
    vec2 sizeFactor = vec2(1)/iResolution*quality;

    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);
    if (source.a >= maxAlpha) {
        finalColor = vec4(0.);
    } else {
        const int range = int(samples - 1)/2;

        for (int x = -range; x <= range; x++)
        {
            for (int y = -range; y <= range; y++)
            {
                vec2 coord = fragTexCoord + vec2(x, y)*sizeFactor;
                // no wrapping
                if (coord.x < 0. || coord.x > 1. || coord.y < 0. || coord.y > 1.) {
                    continue;
                }
                vec4 sampleCol = texture(texture0, coord);
                sum += sampleCol;
            }
        }

        // Calculate final fragment color

        vec4 toAdd = (sum/(samples*samples));
        // toAdd = pow(toAdd, vec4(0.5));

        // finalColor = (toAdd + source)*colDiffuse;
        // finalColor = (toAdd + finalSourceCol)*colDiffuse;


        // just include the stuff to add, and let the normal part of the texture be drawn separately
        finalColor = toAdd * colDiffuse;
    }

}


