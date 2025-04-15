uniform sampler2D texture0;
out vec4 finalColor;

void fragment() {
    vec3 c = texture(texture0, UV).rgb;
    finalColor = vec4(c, 1.0);
}
