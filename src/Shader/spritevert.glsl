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
// out vec3 hdrColor;

// NOTE: Add here your custom variables

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    // hdrColor = vertexNormal;

    // this is from the raylib template, idk what it does, can't just do the commented version
    gl_Position = mvp*vec4(vertexPosition, 1.0);
    // gl_Position = vec4(vertexPosition, 1.0);
}
