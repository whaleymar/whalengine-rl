#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
// default:
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// mine:
uniform float _Threshold;
uniform float _SoftThreshold;

// Output fragment color
out vec4 finalColor;

const float EPSILON = 0.00001;

// Based on a tutorial from catlikecoding.com
vec3 prefilter(vec3 color) {
    // define brightness as brightest channel
    float brightness = max(color.r, max(color.g, color.b));
    float knee = _Threshold * _SoftThreshold;
    float soft = brightness - _Threshold + knee;
    soft = clamp(soft, 0., 2. * knee);
    soft = soft * soft / (4. * knee + EPSILON);

    // weight color contribution by how much it exceeds the threshold
    float contrib = max(soft, brightness - _Threshold);
    contrib = contrib / max(brightness, EPSILON); // avoid DBZ 
    return color * contrib;
}

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);

    finalColor = vec4(prefilter(texelColor.rgb), 1.);
}


