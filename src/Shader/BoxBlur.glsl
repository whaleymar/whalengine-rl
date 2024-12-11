#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;

// Output fragment color
out vec4 finalColor;

bool isOutOfBounds(vec2 p) {
    return p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0;
}

vec3 Sample(vec2 uv) {
    if (isOutOfBounds(uv)) {
        return vec3(0.);
    }
    return texture(texture0, uv).rgb;
}

vec3 SampleBox(vec2 uv, float delta) {
    vec2 _MainTex_TexelSize = 1. / vec2(textureSize(texture0, 0).xy);
    vec2 offset1 = _MainTex_TexelSize * vec2(-delta, delta);
    vec2 offset2 = _MainTex_TexelSize * vec2(delta, delta);
    vec2 offset3 = _MainTex_TexelSize * vec2(-delta, -delta);
    vec2 offset4 = _MainTex_TexelSize * vec2(delta, -delta);

    vec3 sample = Sample(uv + offset1) + Sample(uv + offset2) + Sample(uv + offset3) + Sample(uv + offset4);
    return sample * 0.25;
}

void main() {
    const float delta = 1.;
    finalColor = vec4(SampleBox(fragTexCoord, delta), 1.0);
}

