#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

const vec2 size = vec2(320, 180);   // Framebuffer size
// const float samples = 13.;          // finalColors per axis; higher = bigger glow, worse performance. MIN 3
// const float quality = 1.;          // Defines size factor: Lower = smaller glow, better quality

// playing w/ new values
const float samples = 9.;          // finalColors per axis; higher = bigger glow, worse performance. MIN 3
const float quality = 2.;          // Defines size factor: Lower = smaller glow, better quality

// super over-the-top settings, for testing if the shader is working
// const float samples = 5.;          
// const float quality = 3.;

// TODO needs HDR + tone mapping step first? So i just extract the bright stuff and don't bloom things like black outlines

void main()
{
    vec4 sum = vec4(0);
    vec2 sizeFactor = vec2(1)/size*quality;

    // Texel color fetching from texture sampler
    vec4 source = texture(texture0, fragTexCoord);

    // const int range = 2;            // should be = (samples - 1)/2;
    const int range = 4;            // should be = (samples - 1)/2;

    for (int x = -range; x <= range; x++)
    {
        for (int y = -range; y <= range; y++)
        {
            sum += texture(texture0, fragTexCoord + vec2(x, y)*sizeFactor);
        }
    }

    // Calculate final fragment color
    finalColor = ((sum/(samples*samples)) + source)*colDiffuse;
}


