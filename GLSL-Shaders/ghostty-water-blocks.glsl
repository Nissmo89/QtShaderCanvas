vec3 mod289(vec3 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 mod289(vec4 x) { return x - floor(x * (1.0 / 289.0)) * 289.0; }
vec4 permute(vec4 x) { return mod289(((x * 34.0) + 10.0) * x); }
vec4 taylorInvSqrt(vec4 r) { return 1.79284291400159 - 0.85373472095314 * r; }
float snoise(vec3 v) {
  const vec2 C = vec2(1.0 / 6.0, 1.0 / 3.0);
  const vec4 D = vec4(0.0, 0.5, 1.0, 2.0);
  // First corner
  vec3 i = floor(v + dot(v, C.yyy));
  vec3 x0 = v - i + dot(i, C.xxx);
  // Other corners
  vec3 g = step(x0.yzx, x0.xyz);
  vec3 l = 1.0 - g;
  vec3 i1 = min(g.xyz, l.zxy);
  vec3 i2 = max(g.xyz, l.zxy);
  vec3 x1 = x0 - i1 + C.xxx;
  vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
  vec3 x3 = x0 - D.yyy;      // -1.0+3.0*C.x = -0.5 = -D.y
  // Permutations
  i = mod289(i);
  vec4 p = permute(permute(permute(i.z + vec4(0.0, i1.z, i2.z, 1.0)) + i.y +
                           vec4(0.0, i1.y, i2.y, 1.0)) +
                   i.x + vec4(0.0, i1.x, i2.x, 1.0));
  // Gradients: 7x7 points over a square, mapped onto an octahedron.
  float n_ = 0.142857142857; // 1.0/7.0
  vec3 ns = n_ * D.wyz - D.xzx;
  vec4 j = p - 49.0 * floor(p * ns.z * ns.z); //  mod(p,7*7)
  vec4 x_ = floor(j * ns.z);
  vec4 y_ = floor(j - 7.0 * x_); // mod(j,N)
  vec4 x = x_ * ns.x + ns.yyyy;
  vec4 y = y_ * ns.x + ns.yyyy;
  vec4 h = 1.0 - abs(x) - abs(y);
  vec4 b0 = vec4(x.xy, y.xy);
  vec4 b1 = vec4(x.zw, y.zw);
  vec4 s0 = floor(b0) * 2.0 + 1.0;
  vec4 s1 = floor(b1) * 2.0 + 1.0;
  vec4 sh = -step(h, vec4(0.0));
  vec4 a0 = b0.xzyw + s0.xzyw * sh.xxyy;
  vec4 a1 = b1.xzyw + s1.xzyw * sh.zzww;
  vec3 p0 = vec3(a0.xy, h.x);
  vec3 p1 = vec3(a0.zw, h.y);
  vec3 p2 = vec3(a1.xy, h.z);
  vec3 p3 = vec3(a1.zw, h.w);
  // Normalise gradients
  vec4 norm =
      taylorInvSqrt(vec4(dot(p0, p0), dot(p1, p1), dot(p2, p2), dot(p3, p3)));
  p0 *= norm.x;
  p1 *= norm.y;
  p2 *= norm.z;
  p3 *= norm.w;
  // Mix final noise value
  vec4 m =
      max(0.5 - vec4(dot(x0, x0), dot(x1, x1), dot(x2, x2), dot(x3, x3)), 0.0);
  m = m * m;
  return 105.0 *
         dot(m * m, vec4(dot(p0, x0), dot(p1, x1), dot(p2, x2), dot(p3, x3)));
}
float noise2D(vec2 uv) {
  uvec2 pos = uvec2(floor(uv * 1000.));
  return float((pos.x * 68657387u ^ pos.y * 361524851u + pos.x) % 890129u) *
         (1.0 / 890128.0);
}
float roundRectSDF(vec2 center, vec2 size, float radius) {
  return length(max(abs(center) - size + radius, 0.)) - radius;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  vec2 uv = fragCoord / iResolution.xy, sd = vec2(2.), sdh = vec2(1.);
  vec4 ghosttyCol = texture(iChannel0, uv);
  float ratio = iResolution.y / iResolution.x,
        fw = max(fwidth(uv.x), fwidth(uv.y));

  // Drift coordinate for the fill pattern: a slow diagonal current with a
  // gentle ripple sway instead of a straight vertical scroll.
  vec2 puv = floor(uv * vec2(60., 60. * ratio)) / 60.;
  puv += (smoothstep(0., 0.7, noise2D(puv)) - 0.5) * 0.04
       - vec2(iTime * 0.045, iTime * 0.015)
       + vec2(0.0, sin(iTime * 0.35 + puv.x * 6.0) * 0.012);

  // Block grid -- identical to the original
  uv = fract(vec2(uv.x, uv.y * ratio) * 10.);
  float d = roundRectSDF((sd + 0.01) * (uv - .5), sdh, 0.075),
        d2 = roundRectSDF((sd + 0.065) * (fract(uv * 6.) - .5), sdh, 0.2);

  // Water caustics: domain-warped layered noise, sharpened into thin
  // bright veins instead of soft nebula clouds.
  float t = iTime * 0.25;
  vec2 flow = vec2(snoise(vec3(puv * 1.3, t * 0.6)),
                    snoise(vec3(puv * 1.3 + 7.3, t * 0.6)));
  float n1 = snoise(vec3(puv * 2.0 + flow * 0.5, t));
  float n2 = snoise(vec3(puv * 3.5 - flow * 0.35, t * 1.4 + 2.0));
  float n3 = snoise(vec3(puv * 6.0 + flow * 0.2, t * 0.8 + 4.0));

  float caustic = 1.0 - abs(n1 * 0.6 + n2 * 0.3 + n3 * 0.1);
  caustic = pow(clamp(caustic, 0.0, 1.0), 6.0);

  float noise = pow(n1 * 0.5 + 0.5, 1.5);

  vec3 col1 = vec3(0.010, 0.045, 0.085);  // deep water
  vec3 col2 = vec3(0.0, 0.0, 0.0);
  vec3 col3 = vec3(0.020, 0.100, 0.160);  // mid-depth blue
  vec3 col4 = vec3(0.030, 0.140, 0.180);  // surface teal (also the grid-line fill)
  vec3 caustTint = vec3(0.25, 0.55, 0.60);

  vec3 fcol = mix(mix(mix(col1, col3, smoothstep(0.0, 0.3, noise)), col2,
                      smoothstep(0.0, 0.5, noise)),
                  col4, smoothstep(0.0, 1.0, noise));
  fcol += caustTint * caustic;

  fragColor = vec4(
      ghosttyCol.rgb +
          mix(col4, fcol, smoothstep(fw, -fw, d) * smoothstep(fw, -fw, d2)),
      ghosttyCol.a);
}
