void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord.xy / iResolution.xy;

    // current terminal image
    vec4 color = texture(iChannel0, uv);

    // simple scanline effect
    float scanline = 0.92 + 0.08 * sin(fragCoord.y * 3.14159);

    fragColor = vec4(color.rgb * scanline, color.a);
}
