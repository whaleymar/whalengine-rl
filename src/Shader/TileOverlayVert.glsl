#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;
in vec4 vertexColor;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;

// not needed:
// uniform mat4 matView;
// uniform mat4 matProjection;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec4 fragColor;
out vec2 WorldPosition;

// NOTE: Add here your custom variables

void main()
{
    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;
    fragColor = vertexColor;
    WorldPosition = (matModel * vec4(vertexPosition.xy, 0.0, 1.0)).xy;

    gl_Position = mvp*vec4(vertexPosition, 1.0);
}

