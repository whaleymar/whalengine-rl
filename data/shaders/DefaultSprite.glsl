#use MRT

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;
in vec4 vertexCustom0;
in vec4 vertexCustom1;

varying vec2 fragTexCoord;
varying vec4 fragColor;
varying float fragDepth;
varying float isUI;
varying float isMask;
varying vec2 maskTexCoord;
varying float isSilhouette;
varying float isMaskBlendAdditive;

uniform mat4 mvp;
uniform sampler2D texture0;

#ifdef PLATFORM_WEB
float extractBit(int value, int bitPos) {
    return 0.;
}
#else
float extractBit(uint intData, int bitPosition) {
    uint bitPos = 0x1u << (uint(bitPosition) - 1u);
    if ((intData & bitPos) != 0u) {
        return 1.0;
    } else {
        return 0.0;
    }
}
#endif

void vertex() {
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

    #ifdef PLATFORM_WEB
    // nothing is working :(
    int bitData = int(floor(vertexNormal.r + 0.5));
    fragDepth = 0.;

    #else
    // Write the color buffer data
    uint bitData = floatBitsToUint(vertexNormal.r);

    // Extract the depth (first 8 bits) and normalize
    fragDepth = float(bitData & 0xFFu) / 255.0;

    #endif

    // Extract flags
    // isOccluder = extractBit(bitData, 9);
    isUI = extractBit(bitData, 10);
    isMask = extractBit(bitData, 11);
    isSilhouette = extractBit(bitData, 12);
    isMaskBlendAdditive = extractBit(bitData, 13);

    maskTexCoord = vertexNormal.gb + vertexTexCoord;

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}

void fragment() {
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, fragTexCoord);

    if (isMask > 0.) {
        vec4 maskColor = texture(texture0, maskTexCoord);
        if (isMaskBlendAdditive > 0.) {
            texelColor = vec4((texelColor + texelColor * maskColor).xyz, texelColor.a);
        } else {
            texelColor *= maskColor;
        }
    }

    if (isSilhouette > 0.) {
        FragColor = fragColor * vec4(1., 1., 1., texelColor.a);
    } else {
        FragColor = fragColor * texelColor;
    }

    // To make things more visible when debugging, scale the colors
    // During release, this can just be 1.0
    const float scalar = 20.0;
    if (isUI < 0.5) {
        Depth = vec4(fragDepth * scalar, 0., 0., texelColor.a * fragColor.a);
    }
}
