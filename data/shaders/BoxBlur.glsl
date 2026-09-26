uniform sampler2D texture0;
#ifdef PLATFORM_WEB
uniform vec2 _TextureSize;
#endif

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
    #ifndef PLATFORM_WEB
    vec2 _TextureSize = vec2(textureSize(texture0, 0).xy);
    #endif
    vec2 _MainTex_TexelSize = 1. / _TextureSize;
    vec2 offset1 = _MainTex_TexelSize * vec2(-delta, delta);
    vec2 offset2 = _MainTex_TexelSize * vec2(delta, delta);
    vec2 offset3 = _MainTex_TexelSize * vec2(-delta, -delta);
    vec2 offset4 = _MainTex_TexelSize * vec2(delta, -delta);

    vec3
    boxSample = Sample(uv+offset1)+ Sample(uv+offset2)+ Sample(uv+offset3)+ Sample(uv+offset4);
return boxSample * 0.25 ;
}

void fragment() {
    const float delta = 1.;
    finalColor = vec4(SampleBox(UV, delta), 1.0);
}
