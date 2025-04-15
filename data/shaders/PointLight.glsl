uniform sampler2D texture0; // occlusion color texture
uniform mat4 mvp;

out vec4 finalColor;

void fragment() {
    vec2 adj = vec2(UV.x - 0.5, UV.y - 0.5);
    float distance = length(adj);

    float intensity = 1. - distance;

    // zero if outside
    intensity = intensity * step(0.5, intensity);
    intensity = clamp((intensity - 0.5) * 2., 0., 1.);

    finalColor = vec4(COLOR * intensity);
}
