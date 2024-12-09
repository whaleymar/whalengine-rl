#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
// in vec2 vertexTexCoord2;
in vec3 vertexNormal;
// in vec4 vertexTangent;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 fragColor;
out float fragDepth;
out float isOccluder;
out float isUI;
out float isMask;
out vec2 maskTexCoord;

// NOTE: Add here your custom variables

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;

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

    // 11th bit 
    if ((intData & 0x400u) != 0u) {
        isMask = 1.0;
    } else {
        isMask = 0.0;
    }

    maskTexCoord = vertexNormal.gb + vertexTexCoord;

    // this is from the raylib template, idk what it does, can't just do the commented version
    gl_Position = mvp*vec4(vertexPosition, 1.0);
    // gl_Position = vec4(vertexPosition, 1.0);
}
