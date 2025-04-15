uniform sampler2D texture0;
out vec4 finalColor;

void fragment() {
    vec4 texelColor = texture(texture0, UV);
    if (texelColor.r == 0.) {
        finalColor = vec4(1000000., 1000000., 1000000., 1.);
    } else {
        finalColor = vec4(UV.xy, 0., 1.);
    }
}
