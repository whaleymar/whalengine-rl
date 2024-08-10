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

// Output fragment color
out vec4 finalColor;

void main() {
    // Get the position of the current fragment (screen coordinates!)
    vec2 position = vec2( gl_FragCoord.x - iResolution.x/2., iResolution.y/2. - gl_FragCoord.y);

    // clamp delta to be on the light's bounds. that is the closest point on the light to the pixel 
    vec2 delta = position - lightpos; // vector pointing from light center to cur pixel
    float closestX = clamp(delta.x, -lighthalflen.x, lighthalflen.x);
    float closestY = clamp(delta.y, -lighthalflen.y, lighthalflen.y);
    vec2 closestPoint = lightpos + vec2(closestX, closestY);

    float dist = distance(position , closestPoint);

    // outside of light's bounds, decrease intensity until radius
    float intensity = clamp(1. - dist/lightradius, 0., 1.);

    finalColor = vec4(fragColor * intensity);

    // testing
    // finalColor = vec4(intensity, 0., 0., 1.);
}
