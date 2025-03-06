varying vec2 fragTexCoord;
varying vec4 fragColor;

uniform sampler2D texture0;
uniform vec4 colDiffuse;

out vec4 finalColor;

void fragment() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    finalColor = vec4(texelColor.rgb, 1.);
}
