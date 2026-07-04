// ============================================================
//  Liquid Glass × Spectrum Strands
//  iOS 26-style glass lens containing a woven rainbow ribbon.
//  iChannel0 = any texture.  Set its Filter = "mipmap" (rim blur)
//  and Wrap = "clamp".  Drag the mouse to move the puck.
// ============================================================

const float PI = 3.14159265;

// ---- lens ----
const float RADIUS = 0.22;   // lens radius (screen-height units)
const float RIM    = 5.0;    // higher = flatter centre, harder rim onset
const float PULL   = 0.62;   // how far rim pixels fold back inward
const float MAG    = 0.07;   // gentle magnification across the flat centre
const float CA     = 0.020;  // chromatic aberration at the rim
const float BLUR   = 2.5;    // rim blur (mip lod) applied to iChannel0
const float FROST  = 0.3;    // frosting of the strands toward the rim

// ---- vertical fade (inside the marble) ----
// everything below — backdrop, strands AND the glass lighting — is keyed to this.
const float GRAD_FADE0 = 0.30; // gone/black above this (0 = top of marble)
const float GRAD_FADE1 = 0.80; // full strength below this. widen the gap = softer

// ---- strands (the glowing content) ----
#define STRANDS 4
const float STRAND_SIZE  = 2.50; // overall ribbon scale. bigger = larger / fills more
const float STRAND_AMP   = 1.40; // extra vertical stretch (ribbon height only)
const float STRAND_LEVEL = 0.90; // "energy" 0..1 (brightness + amplitude)

// ---------- helpers ----------
vec3 spectrum(float t) {                       // IQ cosine rainbow palette
    return 0.5 + 0.5 * cos(2.0 * PI * (t + vec3(0.00, 0.33, 0.67)));
}

float bendCurve(float x)     { return pow(clamp(x, 0.0, 1.0), RIM); }
float heightProfile(float x) { return sqrt(max(0.0, 1.0 - pow(clamp(x, 0.0, 1.0), 8.0))); }

// quintic ease — softer S-curve than smoothstep
float smootherstep(float a, float b, float x) {
    x = clamp((x - a) / (b - a), 0.0, 1.0);
    return x * x * x * (x * (x * 6.0 - 15.0) + 10.0);
}

// 0 at the top of the marble, easing to 1 below GRAD_FADE1
float vFade(float localY) {
    return smootherstep(GRAD_FADE0, GRAD_FADE1, clamp((1.0 - localY) * 0.5, 0.0, 1.0));
}

vec2 toUV(vec2 p, float asp) { return vec2(p.x / asp, p.y) + 0.5; }

// backdrop = the source texture, optionally blurred via mip lod
vec3 background(vec2 uv, float lod) {
    return textureLod(iChannel0, uv, lod).rgb;
}

// woven rainbow strands in LOCAL lens space (disc ~ [-1,1]).
// 'soft' (0..1) frosts the glow toward the rim. Returns tonemapped colour.
vec3 strands(vec2 local, float soft) {
    vec2  s   = local / STRAND_SIZE;
    float e   = 0.06 + STRAND_LEVEL * 0.94;
    float env = pow(max(cos(s.x * PI * 1.3), 0.0), 2.9);   // spindle envelope

    vec3 col = vec3(0.0);
    for (int i = 0; i < STRANDS; i++) {
        float fi   = float(i);
        float ph   = fi * 1.7;
        float freq = 2.0 + fi * 0.35;
        float spd  = 1.4 + fi * 1.2;                       // keep it fast

        float w = sin(s.x * freq       + iTime * spd       + ph)       * 0.60
                + sin(s.x * freq * 1.1 - iTime * spd * 0.7 + ph * 1.7) * 0.40;

        float amp = (0.10 + 0.02 * e) * env * STRAND_AMP;
        float y   = w * amp;

        float d     = abs(s.y - y);
        float thick = (0.001 + 0.05 * e) * (0.35 + env) * (1.0 + soft * FROST);
        float g     = thick / (d + thick * 0.45);
        g = g * g;

        float h = fi / float(STRANDS) + s.x * 0.30 + iTime * 0.04;
        col += spectrum(h) * g * env;
    }

    col *= (0.45 + 0.7 * e) / (1.0 + soft * FROST * 0.6);  // tame frosted rim bloom
    return 1.0 - exp(-col * 1.45);                         // exposure tone map
}

// full scene as sampled inside the glass, at centred point 'pc'
vec3 scene(vec2 pc, vec2 c, float soft) {
    float asp = iResolution.x / iResolution.y;
    vec3  col = background(toUV(pc, asp), soft * BLUR);     // refracted texture (frosted at rim)

    vec2  local = (pc - c) / RADIUS;
    float r     = length(local);
    if (r >= 1.0) return col;                              // outside disc: backdrop only

    float fade = vFade(local.y);                           // black atop -> full lower down
    col *= fade;                                           // backdrop fades into the black
    float mask = smoothstep(1.0, 0.72, r);
    col += strands(local, soft) * mask * fade;             // strands emerge from the black
    return col;
}

void mainImage(out vec4 O, in vec2 F) {
    vec2  res = iResolution.xy;
    float asp = res.x / res.y;
    vec2  p   = (F - 0.5 * res) / res.y;                   // centred, aspect-correct

    // puck centre: mouse drag, else lazy drift
    vec2 c = (iMouse.z > 0.5)
        ? (iMouse.xy - 0.5 * res) / res.y
        : vec2(cos(iTime * 0.50) * 0.30 * asp,
               sin(iTime * 0.37) * 0.22);

    vec2  q   = p - c;
    float d   = length(q);
    float x   = d / RADIUS;                                // 0 centre -> 1 rim
    vec2  dir = d > 1e-5 ? q / d : vec2(0.0, 1.0);

    // base layer = crisp source texture (strands live *inside* the glass)
    vec3  col = background(toUV(p, asp), 0.0);

    // soft drop shadow just outside the puck
    float sh  = smoothstep(1.25, 0.98, length(q - vec2(0.0, -0.02)) / RADIUS);
    col *= 1.0 - 0.22 * sh * step(1.0, x);

    float aa     = 2.0 / (res.y * RADIUS);
    float inside = smoothstep(1.0, 1.0 - aa, x);

    if (inside > 0.0) {
        float bend = bendCurve(x);
        float soft = bend;                                 // frost grows toward rim

        // volumetric inward refraction: collapse rim samples toward centre
        float sd = d * (1.0 - MAG * (1.0 - bend)) * (1.0 - PULL * bend);

        // chromatic aberration: each channel bends slightly differently
        vec3 g;
        for (int i = 0; i < 3; i++) {
            float f  = 1.0 + (float(i) - 1.0) * CA * (0.3 + bend);
            vec2  sp = c + dir * sd * f;
            g[i] = scene(sp, c, soft)[i];
        }

        // dome lighting — faded along the black top, picks up from GRAD_FADE0
        float glassFade = vFade(p.y - c.y >= 0.0 ? (p.y - c.y) / RADIUS : (p.y - c.y) / RADIUS);
        float ee = 0.01;
        float dh = (heightProfile(x + ee) - heightProfile(x - ee)) / (2.0 * ee) / RADIUS;
        vec3  n  = normalize(vec3(-dh * dir, 1.0));
        vec3  L  = normalize(vec3(-0.45, 0.65, 0.55));

        float fres  = pow(1.0 - n.z, 1.5);
        float along = dot(dir, normalize(L.xy));
        float rimGl = fres * (0.55 + 0.45 * pow(abs(along), 1.5));

        g += rimGl * 0.55 * glassFade;                     // rim light, suppressed up top
        g  = g * 1.04 + 0.015 * glassFade;                 // glassy lift, suppressed up top

        col = mix(col, g, inside);
    }

    O = vec4(col, 1.0);
}
