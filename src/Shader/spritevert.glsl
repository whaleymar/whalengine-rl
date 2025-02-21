#version 330

in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

uniform mat4 mvp;

out vec2 fragTexCoord;
out vec4 fragColor;
out float fragDepth;
// out float isOccluder;
out float isUI;
out float isMask;
out vec2 maskTexCoord;
out float isSilhouette;
out float isMaskBlendAdditive;

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

void main() {
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
