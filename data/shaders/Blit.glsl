// Plain texture copy. Used on the web instead of raylib's built-in shader (see Renderer::blit).
uniform sampler2D texture0;
out vec4 finalColor;

void fragment() {
    finalColor = texture(texture0, UV) * COLOR;
}
