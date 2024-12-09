#version 330
layout (location = 0) out vec4 FragColor;
layout (location = 1) out vec4 AllDepth;
layout (location = 2) out vec4 OcclColor;
layout (location = 3) out vec4 OcclDepth;

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;
in float fragDepth;
in float isOccluder;
in float isUI;
in float isMask;
in vec2 maskTexCoord;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color

void main() {
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 maskColor = vec4(1.);

    if (isMask > 0.) {
        maskColor = texture(texture0, maskTexCoord);
    }

    FragColor = texelColor * fragColor * maskColor; 

    // To make things more visible when debugging, scale the colors
    // During release, this can just be 1.0
    const float scalar = 20.0; 
    if (isOccluder > 0.5) {
        OcclDepth = vec4(fragDepth * scalar, 0., 0., texelColor.a);
    } else {
        OcclDepth = vec4(0.);
    }
    if (isUI < 0.5) {
        AllDepth = vec4(fragDepth * scalar, 0., 0., texelColor.a);
    } else {
        AllDepth = vec4(0., 0., 0., 0.0);
    }

    OcclColor = FragColor * vec4(isOccluder);

}
