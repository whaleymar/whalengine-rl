varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0; // occlusion color texture
uniform vec2 lightpos;
// uniform float lightDepth;
// uniform sampler2D occlusionDepthTex;

out vec4 finalColor;

// bool isWall(vec2 p) {
//     vec4 sampleDepth = texture(occlusionDepthTex, p);
//     float depth = sampleDepth.r;
//
//     return lightDepth < depth;
// }
//
// vec4 getWallColor(vec2 p) {
//     return texture(texture0, p);
// }

vec4 getLighting() {
    vec2 adj = vec2(fragTexCoord.x - 0.5, fragTexCoord.y - 0.5);
    float distance = length(adj - lightpos);

    float intensity = 1. - distance;

    // zero if outside
    intensity = intensity * step(0.5, intensity);
    intensity = clamp((intensity - 0.5) * 2., 0., 1.);

    return vec4(fragColor * intensity);

    // full lighting if pretty close. This is like a 1.5 tile radius
    // if (distance < 0.1) {
    //     return fragColor;
    // } else {
    //     float intensity = 1. - distance + 0.1;
    //
    //     // zero if outside
    //     intensity = intensity * step(0.5, intensity);
    //     intensity = (intensity - 0.5) * 2.;
    //
    //     return vec4(fragColor * intensity);
    // }
}

void fragment() {
    // Checking the occlusion texture doesn't work because fragTexCoord is not actually the screen coord, but the uv of the light (because I'm drawing it as a rect)
    // I'm thinking I keep this as-is and add a switch to shadowLight if I want it to behave like a point light

    // if (isWall(fragTexCoord)) {
    // finalColor = vec4(0.);
    // } else {
    finalColor = getLighting();
    // }
}
