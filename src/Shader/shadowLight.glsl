#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine:
uniform float iTime;
uniform vec2 iResolution;

uniform vec2 lp1;
uniform float radiusPixels;

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
    vec4 sampleCol = texture(texture0, p);
    return sampleCol.b > 0.05;
}

vec3 getColor(vec2 p) {	 
    vec4 sampleCol = texture(texture0, p);
    const float MIN_OCCLUSION_VAL = 0.9;
    return step(MIN_OCCLUSION_VAL, sampleCol.b) * wallColor + (1. - step(MIN_OCCLUSION_VAL, sampleCol.b)) * vec3(1.);
}

vec3 getLighting(vec2 p, vec2 lp) {
	vec2 samplePixel = p;
    float distance = length(p - lp);

    // do fewer steps for smaller distances
    int nSteps = int(STEPS * distance);
	vec2 step = (lp-p)/float(nSteps);

    // This makes sure that pixels further from the light source get less light.
    vec3 wallVal = 1./distance*0.075*vec3(0.0,0.0,0.0);
    vec3 airVal = vec3(1.0,1.0,1.0) + 1./distance*0.075*vec3(1.0,1.0,1.0);

	for (int i = 0 ; i < nSteps; i++) {
		if (isWall(samplePixel)) {
            return wallVal;
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
    // return getColor(p) * blendLighting(p, lightPos);

    // doing this effectively makes the light color the ambient, since the light rendertex is multiplied
    float pixelDistance = length(iResolution * p - iResolution * lightPos);
    if (pixelDistance > radiusPixels) {
        return vec3(0.);
    }
    // 0.0 == full light; 1.0 == full shadow
    float fraction = pixelDistance/radiusPixels;
    float weight = mix(0., 1., sqrt(fraction));
    vec3 shadow = blendLighting(p, lightPos);
    // vec3 shadow = blendLightingSimple(p, lightPos);
    // vec3 shadow = blendLightingQuad(p, lightPos);
    shadow = mix(shadow, vec3(0.), weight);
    return shadow;
}

void main() {
    finalColor = vec4(processLight(fragTexCoord, lp1), 1.) * fragColor;
}
