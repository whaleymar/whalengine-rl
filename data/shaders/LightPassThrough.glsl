varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void fragment() {
    vec3 c = texture(texture0, fragTexCoord).rgb;
    finalColor = vec4(c, 1.0);
}
