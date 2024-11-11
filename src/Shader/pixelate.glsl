#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec2 iResolution;

// mine:

// Output fragment color
out vec4 finalColor;

const float virtualRatio = 4.0; // TODO hard coded

// TODO web version
void main() {
    vec2 pixelSize = 1./(iResolution/virtualRatio); 
    vec2 uv = fragTexCoord;

    // add half pixel AFTER flooring so it gets the nearest color correctly
    // makes pixel art nearly unaffected by this shader
    vec2 pxUV = floor(uv/pixelSize) * pixelSize;
    pxUV += pixelSize/2.;

    finalColor = texture(texture0, pxUV) * fragColor;
}
