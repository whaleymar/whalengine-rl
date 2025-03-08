varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform sampler2D texture0;
out vec4 finalColor;

void fragment() {
    vec2 nearestSeed = texture(texture0, fragTexCoord).xy;
    float distance = clamp(distance(fragTexCoord, nearestSeed), 0.0, 1.0);
    finalColor = vec4(vec3(distance), 1.0);
}
