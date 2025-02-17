#version 330

in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
out vec4 finalColor;

void main() {
    vec4 texelColor = texture(texture0, fragTexCoord);
    if (texelColor.r == 0.) {
        finalColor = vec4(1000000., 1000000., 1000000., 1.);
    } else {
        finalColor = vec4(fragTexCoord.x, fragTexCoord.y, 0., 1.);
    }
}
