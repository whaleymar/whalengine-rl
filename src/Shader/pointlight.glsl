#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;
// mine:
uniform vec2 lightpos;

// Output fragment color
out vec4 finalColor;

void main() {
    vec2 adj = vec2(fragTexCoord.x - 0.5, fragTexCoord.y - 0.5);
    float distance = length(adj - lightpos);

    float intensity = 1 - distance;

    // zero if outside 
    intensity = intensity * step(0.5, intensity); 
    intensity = (intensity - 0.5) * 2;

    finalColor = vec4(fragColor * intensity);

    // full lighting if pretty close. This is like a 1.5 tile radius
    // if (distance < 0.1) {
    //     finalColor = fragColor;
    // } else {
    //     float intensity = 1 - distance + 0.1;
    //
    //     // zero if outside 
    //     intensity = intensity * step(0.5, intensity); 
    //     intensity = (intensity - 0.5) * 2;
    //
    //     finalColor = vec4(fragColor * intensity);
    // }
}
