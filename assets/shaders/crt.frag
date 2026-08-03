uniform sampler2D texture;
uniform vec2 resolution;

vec2 curve(vec2 uv) {
    uv = (uv - 0.5) * 2.0;
    uv.x *= 1.0 + pow((abs(uv.y) / 5.0), 2.0);
    uv.y *= 1.0 + pow((abs(uv.x) / 4.0), 2.0);
    uv = (uv / 2.0) + 0.5;
    uv = uv * 0.92 + 0.04;
    return uv;
}

void main() {
    vec2 q = gl_TexCoord[0].xy;
    vec2 uv = curve(q);

    if (uv.x < 0.0 || uv.x > 1.0 || uv.y < 0.0 || uv.y > 1.0) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        return;
    }

    vec2 offset = vec2(0.0022, 0.0);
    float r = texture2D(texture, uv - offset).r;
    float g = texture2D(texture, uv).g;
    float b = texture2D(texture, uv + offset).b;
    vec3 col = vec3(r, g, b);

    float scanline = sin(uv.y * resolution.y * 3.14159 * 0.5) * 0.12;
    col -= scanline;

    float vignette = uv.x * uv.y * (1.0 - uv.x) * (1.0 - uv.y);
    vignette = clamp(pow(16.0 * vignette, 0.25), 0.0, 1.0);
    col *= vignette;

    gl_FragColor = vec4(col, 1.0);
}