varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0; // occlusion color texture
uniform vec4 colDiffuse;

global uniform vec2 _GameResolution;

uniform vec2 _DistanceFieldSize; // also the size of `depthBuf`
uniform vec2 lp1;
uniform float radiusPixels;
uniform float lightDepth;
uniform sampler2D depthBuf;
uniform sampler2D _DistanceField;
uniform sampler2D _AllDepth;

out vec4 finalColor;

#ifndef PLATFORM_WEB
const int STEPS = 64;
const int LIGHTPASSES = 10;
const float hitEpsilon = 0.005;

const vec3 wallColor = vec3(0.0);
const float pi = 3.1415926;

bool isWall(vec2 p) {
    float depth = texture(depthBuf, p).r;
    return lightDepth <= depth;
}

bool isBehindSomething(vec2 p) {
    vec4 samp = texture(_AllDepth, p);
    float depth = samp.r;
    // only do this for things that are mostly opaque
    return samp.a > 0.5 && lightDepth < depth;
}

bool isOutOfBounds(vec2 p) {
    return p.x < 0.0 || p.x > 1.0 || p.y < 0.0 || p.y > 1.0;
}

vec4 getWallColor(vec2 p) {
    return texture(texture0, p);
}

vec3 getLighting(vec2 p, vec2 lp) {
    const float minOcclusionAlpha = 0.99;

    vec2 dfSizeRatio = _GameResolution / _DistanceFieldSize;
    vec2 dfSizeRatioInv = vec2(1.) / dfSizeRatio;
    float distScalar = (dfSizeRatioInv.x + dfSizeRatioInv.y) / 2.;
    vec2 samplePixel = p;
    vec2 bias = (_DistanceFieldSize - _GameResolution) * 0.5 / _DistanceFieldSize; // distance from distance field origin to camera view origin (in screen coords)
    vec2 samplePixelSDF = p * dfSizeRatio + bias;
    vec2 deltaStart = lp - p;
    vec2 rayDir = normalize(lp - p);

    vec3 wallVal = vec3(0.);
    vec3 airVal = vec3(1.0);

    for (int i = 0; i < STEPS; i++) {
        // get distance to closest occluder from SDF. use that as step size.
        float dist = texture(_DistanceField, samplePixelSDF).r;
        vec2 nextStep = rayDir * vec2(dist);
        samplePixelSDF += nextStep;
        samplePixel += nextStep * dfSizeRatioInv;

        if (isOutOfBounds(samplePixelSDF)) {
            return airVal;
        }

        // check if we passed the light
        vec2 deltaNow = lp - samplePixel;
        if (sign(deltaStart.x) != sign(deltaNow.x) && sign(deltaStart.y) != sign(deltaNow.y)) {
            return airVal;
        }

        // isWall checks depth conditions
        // if ((dist * distScalar) < hitEpsilon && isWall(samplePixelSDF)) {
        if ((dist * distScalar) < hitEpsilon) {
            // if (isWall(samplePixelSDF)) {
            // check for translucency
            // vec4 wallCol = getWallColor(samplePixel);
            // if (wallCol.a >= minOcclusionAlpha) {
            //     return wallVal * airVal;
            // } else {
            //     airVal = mix(airVal, wallCol.rgb, wallCol.a);
            // }
            return wallVal * airVal;
        }
    }

    // We never hit a wall but never reached the light either.
    // This is almost always because the point is surrounded by walls and every step size is tiny.
    // This can happen for points in a narrow corridor, or when a light is near a wall (for the points along that same wall).
    // For the latter case, best to assume it's not in shadow. For the former, the only thing I can do is increase the number of ray steps.
    return airVal;
}

// p = pixel position
// lp = light position
vec3 blendLighting(const vec2 p, vec2 lp) {
    vec2 r;
    vec3 c = vec3(0., 0., 0.);
    const float recip = 1. / float(LIGHTPASSES);

    const float step = 2. / float(LIGHTPASSES);
    float valX = -1.;
    float valY = -1.;

    // const float SCALAR = 0.05;
    vec2 SCALAR = 2. / _GameResolution;
    const float BIAS = 0.;
    const float t_denom = 1. / float(LIGHTPASSES);
    float t = 0.0;
    for (int i = 0; i < LIGHTPASSES; i++) {
        r = vec2(cos(2. * t * pi), sin(2. * t * pi)) * SCALAR;
        c += getLighting(p, lp + r) * recip;
        valX += step;
        valY += step;
        t += t_denom;
    }

    return c;
}

// p = pixel position
// lp = light position
// this does one pass instead of adding some offsets to the lighting pass and then averaging the light values
// sacrifices soft shadows, but is much faster
vec3 blendLightingSimple(const vec2 p, vec2 lp) {
    vec3 c = vec3(0., 0., 0.);
    c += getLighting(p, lp);

    return c;
}

vec3 processLight(vec2 p, vec2 lightPos) {
    // doing this effectively makes the light color the ambient, since the light rendertex is multiplied
    float pixelDistance = length(_GameResolution * p - _GameResolution * lightPos);
    if (pixelDistance > radiusPixels) {
        return vec3(0.);
    }

    // check if we are behind something else
    vec3 light;

    // Because I'm doing an `else`, anything behind a wall but in front (depth-wise) of light gets lit, but I kinda like how it looks
    if (isBehindSomething(p)) {
        // return vec3(0.);
        light = vec3(1.0);
        // EXPERIMENT walk towards light & decrease light every time there's something in the way
        // RESULT: looks interesting, but I would like the light to walk "up" more to sell the top down look. Also darkens the player
        //         I think this effect would be must better if I just added normals to my sprites

        // const int fg_steps = 8;
        // float denom = float(fg_steps) * 1.5;
        // vec2 rayDir = normalize(lightPos - p) / _GameResolution;
        // vec2 curP = p;
        // vec2 deltaStart = lightPos - p;
        // for (int i = 0; i < fg_steps; i++) {
        //     curP += rayDir;
        //
        //     // check if we passed the light
        //     vec2 deltaNow = lightPos - curP;
        //     if (sign(deltaStart.x) != sign(deltaNow.x) && sign(deltaStart.y) != sign(deltaNow.y)) {
        //         light = vec3(0.33);
        //         break;
        //     }
        //
        //     if (isBehindSomething(curP)) {
        //         light -= (vec3(1.0) / denom);
        //     } else {
        //         break;
        //     }
        // }
    } else {
        light = blendLighting(p, lightPos);
    }

    // 1.0 == full light; 0.0 == full shadow
    // vec3 light = blendLighting(p, lightPos);
    // vec3 light = blendLightingSimple(p, lightPos);
    // vec3 light = vec3(1.); // point light without shadows

    // This makes sure the pixel is lit less based on distance from light.
    float fraction = pixelDistance / radiusPixels;
    float weight = mix(0., 1., sqrt(fraction));
    light = mix(light, vec3(0.), weight);
    return light;
}

#else // ifndef PLATFORM_WEB

// just do a point light bc we don't have a SDF
vec3 processLight(vec2 p, vec2 lightPos) {
    // doing this effectively makes the light color the ambient, since the light rendertex is multiplied
    float pixelDistance = length(_GameResolution * p - _GameResolution * lightPos);
    if (pixelDistance > radiusPixels) {
        return vec3(0.);
    }

    vec3 light = vec3(1.);

    // This makes sure the pixel is lit less based on distance from light.
    float fraction = pixelDistance / radiusPixels;
    float weight = mix(0., 1., sqrt(fraction));
    light = mix(light, vec3(0.), weight);
    return light;
}

#endif

void fragment() {
    finalColor = vec4(processLight(fragTexCoord, lp1), 1.) * fragColor;
}
