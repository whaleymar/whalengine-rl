#version 330

// credit: https://github.com/kiwipxl/GLSL-shaders/blob/master/glow.glsl

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec4 fragColor;

// Input uniform values
uniform sampler2D texture0;
uniform vec4 colDiffuse;

// Output fragment color
out vec4 finalColor;

// hard coded vars: 
const float glow_size = .5;
const float glow_intensity = 1;
const float glow_threshold = .5;

void main() {
    finalColor = texture(texture0, fragTexCoord);
    if (finalColor.a <= glow_threshold) {
        ivec2 size = textureSize(texture0, 0);
	
        float uv_x = fragTexCoord.x * size.x;
        float uv_y = fragTexCoord.y * size.y;

        float sum = 0.0;
        for (int n = 0; n < 9; ++n) {
            uv_y = (fragTexCoord.y * size.y) + (glow_size * float(n - 4.5));
            float h_sum = 0.0;
            h_sum += texelFetch(texture0, ivec2(uv_x - (4.0 * glow_size), uv_y), 0).a;
            h_sum += texelFetch(texture0, ivec2(uv_x - (3.0 * glow_size), uv_y), 0).a;
            h_sum += texelFetch(texture0, ivec2(uv_x - (2.0 * glow_size), uv_y), 0).a;
            h_sum += texelFetch(texture0, ivec2(uv_x - glow_size, uv_y), 0).a;
            h_sum += texelFetch(texture0, ivec2(uv_x, uv_y), 0).a;
            h_sum += texelFetch(texture0, ivec2(uv_x + glow_size, uv_y), 0).a;
            h_sum += texelFetch(texture0, ivec2(uv_x + (2.0 * glow_size), uv_y), 0).a;
            h_sum += texelFetch(texture0, ivec2(uv_x + (3.0 * glow_size), uv_y), 0).a;
            h_sum += texelFetch(texture0, ivec2(uv_x + (4.0 * glow_size), uv_y), 0).a;
            sum += h_sum / 9.0;
        }

        finalColor = vec4(vec3(fragColor.r, fragColor.g, fragColor.b), (sum / 9.0) * glow_intensity);
    }
}
