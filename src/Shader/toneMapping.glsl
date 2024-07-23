#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;
// mine:
uniform float exposure;

// Output fragment color
out vec4 finalColor;


void main()
{             
    const float gamma = 2.2;
    vec4 hdrColorFull = texture(texture0, fragTexCoord);
    vec3 hdrColor = hdrColorFull.rgb;

    vec3 mapped = vec3(1.0) - exp(-hdrColor * exposure);
    // gamma correction 
    mapped = pow(mapped, vec3(1.0 / gamma));
  
    finalColor = vec4(mapped, 1.0);

    // if (hdrColorFull.r > 1.) {
    //     finalColor = vec4(1.);
    // } else {
    //     finalColor = vec4(0., 0., 0., 1.);
    // }
}    
