varying vec2 lightpos;
varying float lightradius;

uniform sampler2D texture0;

out vec4 finalColor;

void vertex() {
    lightpos = NORMAL.rg;
    lightradius = NORMAL.b;
    gl_Position = mvp * vec4(VERTEX, 1.0);
}

void fragment() {
    // Get the position of the current fragment (screen coordinates! y=0 at thebottom)
    vec2 position = vec2(gl_FragCoord.x, gl_FragCoord.y);
    vec2 lighthalflen = spriteSize * 0.5;

    // clamp delta to be on the light's bounds. that is the closest point on the light to the pixel
    vec2 delta = position - lightpos; // vector pointing from light center to cur pixel
    float closestX = clamp(delta.x, -lighthalflen.x, lighthalflen.x);
    float closestY = clamp(delta.y, -lighthalflen.y, lighthalflen.y);
    vec2 closestPoint = lightpos + vec2(closestX, closestY);

    // if inside the box, fully lit
    if (abs(delta.x) < lighthalflen.x && abs(delta.y) < lighthalflen.y) {
        finalColor = COLOR;
        return;
    }

    // outside of the box, decrease intensity until radius
    float dist = distance(position, closestPoint);
    float intensity = clamp(1. - dist / lightradius, 0., 1.);

    finalColor = COLOR * intensity;
}
