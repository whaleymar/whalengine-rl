varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 lightpos;
uniform vec2 lighthalflen;
uniform float lightradius;
// global uniform vec2 _GameResolution;

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
    // Get the position of the current fragment (screen coordinates! y=0 at thebottom)
    vec2 position = vec2(gl_FragCoord.x, gl_FragCoord.y);
    // return vec4(position / _GameResolution, 0., 1.); // testing

    // clamp delta to be on the light's bounds. that is the closest point on the light to the pixel
    vec2 delta = position - lightpos; // vector pointing from light center to cur pixel
    float closestX = clamp(delta.x, -lighthalflen.x, lighthalflen.x);
    float closestY = clamp(delta.y, -lighthalflen.y, lighthalflen.y);
    vec2 closestPoint = lightpos + vec2(closestX, closestY);

    // return vec4(delta / _GameResolution, 0., 1.); // testing

    // if inside the box, fully lit
    if (abs(delta.x) < lighthalflen.x && abs(delta.y) < lighthalflen.y) {
        return fragColor;
    }

    // outside of the box, decrease intensity until radius
    float dist = distance(position, closestPoint);
    float intensity = clamp(1. - dist / lightradius, 0., 1.);
    // float intensity = 0.;

    return fragColor * intensity;
}

void fragment() {
    // check if behind an occluder
    // if (isWall(fragTexCoord)) {
    // finalColor = vec4(0.);
    // } else {
    // vec2 p = fragTexCoord * _GameResolution / (lighthalflen * 2);
    // vec2 p = fragTexCoord * (lighthalflen * 2) / _GameResolution;
    // vec4 sampleDepth = texture(occlusionDepthTex, p);
    // finalColor = vec4(sampleDepth.r, 0., 0., 1.);
    finalColor = getLighting();
    // }

    // testing
    // finalColor = vec4(intensity, 0., 0., 1.);

    // finalColor = vec4(1., 1., 1., 1.);
}
