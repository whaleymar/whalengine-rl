uniform sampler2D texture0;

out vec4 finalColor;

void fragment() {
    vec4 texelColor = texture(texture0, UV);
    finalColor = vec4(texelColor.rgb, 1.);
}
