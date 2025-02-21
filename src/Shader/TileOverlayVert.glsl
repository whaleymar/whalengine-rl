#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;
uniform mat4 matModel;

out vec2 fragTexCoord;
out vec4 fragColor;
out float fragDepth;
// out float isOccluder;
out float isUI;
out vec2 WorldPosition;

#ifdef PLATFORM_WEB
float extractBit(int value, int bitPos) {
    return 0.;
}
#else
float extractBit(uint bitData, int bitPosition) {
    uint bitPos = 0x1u << (uint(bitPosition) - 1u);
    if ((bitData & bitPos) != 0u) {
        return 1.0;
    } else {
        return 0.0;
    }
}
#endif

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    WorldPosition = (matModel * vec4(vertexPosition.xy, 0.0, 1.0)).xy;

    // Write the color buffer data
    #ifdef PLATFORM_WEB
    // nothing is working
    int bitData = int(floor(vertexNormal.r + 0.5));
    fragDepth = 0.;

    #else
    uint bitData = floatBitsToUint(vertexNormal.r);

    // Extract the depth (first 8 bits) and normalize
    fragDepth = float(bitData & 0xFFu) / 255.0;
    #endif

    // isOccluder = extractBit(bitData, 9);
    isUI = extractBit(bitData, 10);
    // isMask = extractBit(bitData, 11);
    // isSilhouette = extractBit(bitData, 12);
    // isMaskBlendAdditive = extractBit(bitData, 13);

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
