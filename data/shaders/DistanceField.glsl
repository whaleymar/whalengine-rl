uniform sampler2D texture0;
out vec4 finalColor;

void fragment() {
    vec2 nearestSeed = texture(texture0, UV).xy;
    float distance = clamp(distance(UV, nearestSeed), 0.0, 1.0);
    finalColor = vec4(vec3(distance), 1.0);
}
