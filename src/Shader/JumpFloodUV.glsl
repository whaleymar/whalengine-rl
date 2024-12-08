#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;

// mine:
uniform vec2 _Offset;

// Output fragment color
out vec4 finalColor;

void main() {
    vec4 nearestSeed = vec4(-2.0);
    float nearestDist = 999999.9;
    for (float y = -1.0; y <= 1.0; y += 1.0) {
        for (float x = -1.0; x <= 1.0; x += 1.0) {
            vec2 sampleUV = fragTexCoord + vec2(x, y) * _Offset;

            if (sampleUV.x < 0.0 || sampleUV.x > 1.0 || sampleUV.y < 0.0 || sampleUV.y > 1.0) {
                continue;
            }

            vec4 sampleValue = texture(texture0, sampleUV);
            vec2 sampleSeed = sampleValue.xy;

            if (sampleSeed.x != 0.0 || sampleSeed.y != 0.0) {
                vec2 diff = sampleSeed - fragTexCoord;
                float dist = dot(diff, diff);
                if (dist < nearestDist) {
                    nearestDist = dist;
                    nearestSeed = sampleValue;
                }
            }
        }
    }

    finalColor = nearestSeed;
}

