uniform sampler2D texture0; // TEXTURE WRAP == CLAMP
uniform vec2 _Offset;
out vec4 finalColor;

void fragment() {
    vec4 nearestSeed = vec4(1000000.0);
    float nearestDist = 999999.9;
    for (float y = -1.0; y <= 1.0; y += 1.0) {
        for (float x = -1.0; x <= 1.0; x += 1.0) {
            vec2 sampleUV = UV + vec2(x, y) * _Offset;

            vec4 sampleValue = texture(texture0, sampleUV);
            vec2 sampleSeed = sampleValue.xy;

            vec2 diff = sampleSeed - UV;
            float dist = dot(diff, diff);
            if (dist < nearestDist) {
                nearestDist = dist;
                nearestSeed = sampleValue;
            }
        }
    }

    finalColor = nearestSeed;
}
