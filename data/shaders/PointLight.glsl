varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0; // occlusion color texture
uniform mat4 mvp;

out vec4 finalColor;

vec4 getLighting() {
    vec2 adj = vec2(fragTexCoord.x - 0.5, fragTexCoord.y - 0.5);
    float distance = length(adj);

    float intensity = 1. - distance;

    // zero if outside
    intensity = intensity * step(0.5, intensity);
    intensity = clamp((intensity - 0.5) * 2., 0., 1.);

    return vec4(fragColor * intensity);
}

void fragment() {
    finalColor = getLighting();
}
