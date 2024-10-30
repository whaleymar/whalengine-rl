#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;
// mine:
uniform vec2 lightpos;
uniform vec2 lighthalflen;
uniform float lightradius;
uniform vec2 iResolution;
// uniform float lightDepth;
// uniform sampler2D occlusionDepthTex;

// Output fragment color
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
    // Get the position of the current fragment (screen coordinates!)
    vec2 position = vec2( gl_FragCoord.x, iResolution.y - gl_FragCoord.y);

    // clamp delta to be on the light's bounds. that is the closest point on the light to the pixel 
    vec2 delta = position - lightpos; // vector pointing from light center to cur pixel
    float closestX = clamp(delta.x, -lighthalflen.x, lighthalflen.x);
    float closestY = clamp(delta.y, -lighthalflen.y, lighthalflen.y);
    vec2 closestPoint = lightpos + vec2(closestX, closestY);

    float dist = distance(position , closestPoint);

    // outside of light's bounds, decrease intensity until radius
    float intensity = clamp(1. - dist/lightradius, 0., 1.);

    return vec4(fragColor * intensity);
}

void main() {
    // check if behind an occluder 
    // if (isWall(fragTexCoord)) {
        // finalColor = vec4(0.);
    // } else {
        // vec2 p = fragTexCoord * iResolution / (lighthalflen * 2);
        // vec2 p = fragTexCoord * (lighthalflen * 2) / iResolution;
        // vec4 sampleDepth = texture(occlusionDepthTex, p);
        // finalColor = vec4(sampleDepth.r, 0., 0., 1.);
    finalColor = getLighting();
    // }



    // testing
    // finalColor = vec4(intensity, 0., 0., 1.);

    // finalColor = vec4(1., 1., 1., 1.);
}
