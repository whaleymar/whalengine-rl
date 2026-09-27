#pragma mrt FragColor Depth

uniform sampler2D texture0;

float extractBit(uint intData, int bitPosition) {
    uint bitPos = 0x1u << (uint(bitPosition) - 1u);
    if ((intData & bitPos) != 0u) {
        return 1.0;
    } else {
        return 0.0;
    }
}

void vertex() {
    // Write the color buffer data
    uint bitData = floatBitsToUint(NORMAL.r);

    // Extract the depth (first 8 bits) and normalize
    fragDepth = float(bitData & 0xFFu) / 255.0;


    // Extract flags
    // isOccluder = extractBit(bitData, 9);
    isUI = extractBit(bitData, 10);
    isMask = extractBit(bitData, 11);
    isSilhouette = extractBit(bitData, 12);
    isMaskBlendAdditive = extractBit(bitData, 13);

    maskTexCoord = NORMAL.gb + vertexTexCoord;
}

void fragment() {
    // Texel color fetching from texture sampler
    vec4 texelColor = texture(texture0, UV);

    if (isMask > 0.) {
        vec4 maskColor = texture(texture0, maskTexCoord);
        if (isMaskBlendAdditive > 0.) {
            texelColor = vec4((texelColor + texelColor * maskColor).xyz, texelColor.a);
        } else {
            texelColor *= maskColor;
        }
    }

    if (isSilhouette > 0.) {
        FragColor = COLOR * vec4(1., 1., 1., texelColor.a);
    } else {
        FragColor = COLOR * texelColor;
    }

    // To make things more visible when debugging, scale the colors
    // During release, this can just be 1.0
    const float scalar = 20.0;
    if (isUI < 0.5) {
        Depth = vec4(fragDepth * scalar, 0., 0., texelColor.a * COLOR.a);
    }
}
