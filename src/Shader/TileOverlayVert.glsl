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
out float isOccluder;
out float isUI;
out vec2 WorldPosition;

// NOTE: Add here your custom variables

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    WorldPosition = (matModel * vec4(vertexPosition.xy, 0.0, 1.0)).xy;

    // Write the color buffer data
    uint intData = floatBitsToUint(vertexNormal.r);

    // Extract the depth (first 8 bits) and normalize
    fragDepth = float(intData & 0xFFu) / 255.0;

    // Extract flags
    // 9th bit
    if ((intData & 0x100u) != 0u) {
        isOccluder = 1.0;
    } else {
        isOccluder = 0.0;
    }

    // 10th bit
    if ((intData & 0x200u) != 0u) {
        isUI = 1.0;
    } else {
        isUI = 0.0;
    }

    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
