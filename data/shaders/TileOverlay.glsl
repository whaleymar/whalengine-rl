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
// varying float isOccluder;
varying float isUI;
varying vec2 WorldPosition;

uniform mat4 mvp;
uniform mat4 matModel;

uniform sampler2D texture0;
uniform sampler2D _Overlay;

// ratio should be (1/virtual_screen_ratio) / (texture_size)
uniform vec2 _Scale;

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

void vertex()
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

void fragment() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    vec4 overlayColor = texture(_Overlay, WorldPosition * _Scale);
    FragColor = texelColor * overlayColor;

    // To make things more visible when debugging, scale the colors
    // During release, this can just be 1.0
    const float scalar = 20.0;
    if (isUI < 0.5) {
        Depth = vec4(fragDepth * scalar, 0., 0., texelColor.a);
    }
}
