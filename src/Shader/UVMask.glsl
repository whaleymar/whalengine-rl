varying vec2 fragTexCoord;
varying vec4 fragColor;
uniform sampler2D texture0;
out vec4 finalColor;

void fragment() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    if (texelColor.r == 0.) {
        finalColor = vec4(1000000., 1000000., 1000000., 1.);
    } else {
        finalColor = vec4(fragTexCoord.x, fragTexCoord.y, 0., 1.);
    }
}
