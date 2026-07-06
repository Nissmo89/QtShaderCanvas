void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord / iResolution.xy;

    // Time-varying pixel color
    // vec3 col = 0.5 + 0.5 * sin(iTime + uv.xyx + vec3(0.0, 2.0, 4.0));

    // Output to screen
    // fragColor = vec4(col, 1.0);
    // fragColor = vec4(1.);
    float time = iTime * 0.90;
    fragColor = vec4(sin(time),cos(time) * tan(time / iTime),sin(time),1.0);
}
