#version 100

precision mediump float;

// Input vertex attributes (from vertex shader)
varying vec2 fragTexCoord;
varying vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;
// mine:
uniform vec2 lightpos;

vec2 norm(vec2 coord) {
    return vec2(coord.x/2. + 1., coord.y/2. + 1.);
}

void main() {
    vec2 adj = vec2(fragTexCoord.x - 0.5, fragTexCoord.y - 0.5);
    float d = 1. - length(adj - lightpos);

    // zero if outside 
    d = d * step(0.5, d); 

    // normalize 
    d = (d - 0.5) * 2.;

    gl_FragColor = vec4(fragColor.xyz, fragColor.a * sqrt(d));
}


