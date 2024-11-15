#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0; // occlusion color texture
uniform vec4 colDiffuse;

// mine:
uniform float iTime;
uniform vec2 iResolution;

uniform vec2 lp1;
uniform float radiusPixels;
uniform float lightDepth;
uniform sampler2D occlusionDepthTex;
uniform sampler2D allDepthTex;

// Output fragment color
out vec4 finalColor;

// good balance of speed & visuals
// can handle 5+ lights
// set steps to 100 for slightly less chunky shadows
const float STEPS = 80.;
// const float STEPS = 200.;
const int LIGHTPASSES = 10;

const vec3 wallColor = vec3(0.0);
const float pi = 3.1415926;

bool isWall(vec2 p) {
    vec4 sampleDepth = texture(occlusionDepthTex, p);
    float depth = sampleDepth.r;

    return lightDepth <= depth;
}

bool isBehindSomething(vec2 p) {
    float depth = texture(allDepthTex, p).r;
    return lightDepth < depth;

    // experimenting with a light below illuminating the foreground... needs work
    // maybe if i get the angle between the position and light and light if it's > 45 degrees?
    // return lightDepth < depth && lp1.y > p.y; 
}

vec4 getWallColor(vec2 p) {
    return texture(texture0, p);
}

// RESEARCH optimization/quality improvement: do more LIGHTPASSES the quicker I hit a wall. Shadows are very low quality when the light is right next to a wall & it probably wouldn't have a huge performance hit
vec3 getLighting(vec2 p, vec2 lp) {
    const float minOcclusionAlpha = 0.99;

	vec2 samplePixel = p;
    float distance = length(p - lp);

    // do fewer steps for smaller distances
    int nSteps = int(STEPS * distance);
	vec2 step = (lp-p)/float(nSteps);

    vec3 wallVal = vec3(0.);
    vec3 airVal = vec3(1.0);
    vec3 prevColor = vec3(0.);

    float currentDistance = 0.;
    float stepDistance = distance / float(nSteps);

	for (int i = 0 ; i < nSteps; i++) {
		if (isWall(samplePixel)) {
            // check for translucency
            vec4 wallCol = getWallColor(samplePixel);
            if (wallCol.a >= minOcclusionAlpha) {
                return wallVal;
            } else {
                airVal = mix(airVal, wallCol.rgb, wallCol.a);
            }
        }
		samplePixel += step;
	}
	
	return airVal;
}

// p = pixel position 
// lp = light position
vec3 blendLighting(const vec2 p, vec2 lp) {	
	vec2 r;
	vec3 c = vec3(0.,0.,0.);
    const float recip = 1./float(LIGHTPASSES);
	
    const float step = 2./float(LIGHTPASSES);
    float valX = -1.;
    float valY = -1.;

    // const float SCALAR = 0.05;
    vec2 SCALAR = 2. / iResolution;
    const float BIAS = 0.;
    const float t_denom = 1. / float(LIGHTPASSES);
    float t = 0.0;
	for (int i = 0; i < LIGHTPASSES; i++) {
        r = vec2(cos(2. * t * pi), sin(2. * t * pi)) * SCALAR;
		c += getLighting(p,lp+r) * recip;
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
    float pixelDistance = length(iResolution * p - iResolution * lightPos);
    if (pixelDistance > radiusPixels) {
        return vec3(0.);
    }

    // check if we are behind something else 
    vec3 light;

    // Because I'm doing an `else`, anything behind a wall but in front (depth-wise) of light gets lit, but I kinda like how it looks
    if (isBehindSomething(p)) {
        // return vec3(0.);
        light = vec3(1.0);
    } else {
        light = blendLighting(p, lightPos);
    }

    // 1.0 == full light; 0.0 == full shadow
    // vec3 light = blendLighting(p, lightPos);
    // vec3 light = blendLightingSimple(p, lightPos);
    // vec3 light = vec3(1.); // point light without shadows

    // This makes sure the pixel is lit less based on distance from light.
    float fraction = pixelDistance/radiusPixels;
    float weight = mix(0., 1., sqrt(fraction));
    light = mix(light, vec3(0.), weight);
    return light;
}

void main() {
    finalColor = vec4(processLight(fragTexCoord, lp1), 1.) * fragColor;
}
