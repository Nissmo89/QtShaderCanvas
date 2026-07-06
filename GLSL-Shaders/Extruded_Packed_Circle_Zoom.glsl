/*

    Extruded Packed Circle Zoom
    ---------------------------

    An extruded Apollonian-related packed circle zoom -- produced by calculating
    Soddy circles using a Descartes method and recursive subdivision.

    This is just a follow up to my previous 2D Soddy zoom shader, which was
    based on DjinnKahn's really cool "Soddy circles - infinite zoom" example.

    This version was actually a lot easier to code than the last, which is just
    as well, since I found the previous one a little laborious. :) Anyway, if
    you're after a reasonably straight-forward, brute-force implementation, feel
    free to take a look at the 2D "distField" function. If you're after a cleverer
    version, then it'd be worth perusing DjinnKahn's original.

    Apologies to anyone with a slower machine. The frame rate is OK, but I need
    to find some ways to improve it, which I'll do later on.



    Related examples:

    // Great example. Code for this effect is thin on the ground.
    Soddy circles - infinite zoom -- DjinnKahn
    https://www.shadertoy.com/view/ddyGRd

    // My overblown 2D version of the above. :) It's basically
    // the same thing, but produced using a Descartes circle method.
    Apollonian Soddy Circle Zoom -- Shane
    https://www.shadertoy.com/view/s32SzK


    // Further reading:

    Soddy circles of a triangle
    https://en.wikipedia.org/wiki/Soddy_circles_of_a_triangle

    Coxeter's loxodromic sequence of tangent circles
    https://en.wikipedia.org/wiki/Coxeter%27s_loxodromic_sequence_of_tangent_circles


*/

///// Variable defines ///////////

// Extra holes in the floor for extra depth.
#define FLOOR_HOLES

///////////////////////////////////

// Max ray distance.
#define FAR 20.

// Scene object ID.
int objID;

// Tri-Planar blending function: Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
vec3 tex3D(sampler2D tex, in vec3 p, in vec3 n) {

    // Abosolute normal with a bit of tightning.
    n = max(n * n - .2, .001); // max(abs(n), 0.001), etc.
    n /= dot(n, vec3(1));
    //n /= length(n);

    // Texure samples. One for each plane.
    vec3 tx = texture(tex, p.zy).xyz;
    vec3 ty = texture(tex, p.xz).xyz;
    vec3 tz = texture(tex, p.xy).xyz;

    // Multiply each texture plane by its normal dominance factor.... or however you wish
    // to describe it. For instance, if the normal faces up or down, the "ty" texture
    // sample, represnting the XZ plane, will be used, which makes sense.

    // Textures are stored in sRGB (I think), so you have to convert them to linear space
    // (squaring is a rough approximation) prior to working with them... or something like
    // that. :) Once the final color value is gamma corrected, you should see correct
    // looking colors.
    return mat3(tx * tx, ty * ty, tz * tz) * n;
}

// Height map value.
float hm(in vec2 p) {

    // Reading into "Buffer A".
    // Stretching to account for the varying buffer size.
    //p *= vec2(iResolution.y/iResolution.x, 1);
    return texture(iChannel0, p + .5).x;
}

// IQ's extrusion formula.
float opExtrusion(in float sdf, in float pz, in float h) {
    vec2 w = vec2(sdf, abs(pz) - h);
    return min(max(w.x, w.y), 0.) + length(max(w, 0.));

    /*
        // Slight rounding. A little nicer, but slower.
        const float sf = .015;
        vec2 w = vec2( sdf, abs(pz) - h - sf/2.);
      	return min(max(w.x, w.y), 0.) + length(max(w + sf, 0.)) - sf;
        */
}

vec2 gP;
float gSc;

vec4 vObj;

// The extruded image.
float map(vec3 p3) {

    // Floor.
    float fl = p3.y;

    // Fairly standard zoom setup.
    //
    // Cycle time.
    float t = iTime / 1.;
    iT = floor(t); // Number of cycles.

    float iT0 = iT; // Cycle number copy.
    // Fractional time component.
    t = t - (iT + .5);

    // We want each circle to expand (or contract) to the size of
    // the next circle each time that the fractional time scale
    // loops back to zero.
    float zoom = pow(GEOMETRIC_SCALE, t);
    // Scale and smoothing factor.
    float sc = 1. / 32. / zoom;

    // Global scale, for later use.
    gSc = sc;

    // Scene field calculations.
    vec2 p = p3.xz * sc;

    // Rotation.
    p *= rot2(iTime / 2.);

    // Saving the 2D coordinates for later use.
    gP = p;

    // The packed circle routine.
    vec3 d3 = distField(p);

    // Packed circle distance.
    float d2 = d3.x / sc;
    // Rendering rings, instead of solid circles.
    d2 = abs(d2 + .01) - .01;
    d2 += .002; // Adding a miniscule gap.

    vec2 ip = d3.yz;

    // Circle ID value.
    float val = mod(ip.x + iT0, 16.) / 16.;
    val += ip.y / 3.;

    // Giving the 2D objects some depth.
    float h = .25;
    float d = opExtrusion(d2, p3.y - h / 2., h / 2.);

    d += d2 * .35; // Raising the edges a bit.

    ///////////////

    #ifdef FLOOR_HOLES

    // Cutting out a single central circle from the floor to add a
    // little extra depth. I initially rendered a complete Steiner
    // circle pattern inside each circular cavity, but found it to
    // be a bit much, so settled for the central circle only.

    // Circular ring position.
    vec2 q = gP - gCir.xy;

    // Central Steiner circle setup.
    int numC = 3;
    float piDivN = PI / float(numC);
    float r = sin(piDivN) / (1. + sin(piDivN));

    // Floor circle.
    vec3 c3I = vec3(0, 0, 1. - 1. * r);
    float oR = gCir.z - .04 * sc;
    float d2B = length(q - c3I.xy * oR) - c3I.z * oR;
    d2B /= sc;
    // Cutting the circle out of the floor.
    fl = max(fl, -max(d2B, abs(p3.y) - .125 * 3.));
    d2B = abs(d2B + .01) - .01; // 2D ring.
    float dB = opExtrusion(d2, p3.y - h / 2., h / 2.); // 3D ring.
    // Add the ring to the packed circle (ring) structure.
    d = min(d, max(d2B, p3.y - .01) + d2B * .35);

    #endif

    // Object IDs.
    vObj = vec4(fl, d, 1e5, 1e5);

    // Minimum scene distance.
    return min(fl, d);
}

// Basic raymarcher.
float trace(in vec3 ro, in vec3 rd) {

    // Overall ray distance and scene distance.
    //
    // IQ's suggestion: Moving the ray's jump-off point closer to the
    // surface plane to gain some extra speed, especially when in
    // fullscreen mode.
    float t = (.35 - ro.y) / rd.y, d;

    for (int i = min(0, iFrame); i < 128; i++) {
        d = map(ro + rd * t);
        if (abs(d) < .001 || t > FAR) break;

        t += d * .65;
    }

    return min(t, FAR);
}

// Normal function. It's not as fast as the tetrahedral calculation, but more symmetrical.
vec3 normal(in vec3 p) {

    //return normalize(vec3(m(p + e.xyy) - m(p - e.xyy), m(p + e.yxy) - m(p - e.yxy),
    //                      m(p + e.yyx) - m(p - e.yyx)));

    // This mess is an attempt to speed up compiler time by contriving a break... It's
    // based on a suggestion by IQ. I think it works, but I really couldn't say for sure.
    float sgn = 1.;
    vec3 e = vec3(.001, 0, 0), mp = e.zzz; // Spalmer's clever zeroing.
    for (int i = min(iFrame, 0); i < 6; i++) {
        mp.x += map(p + sgn * e) * sgn;
        sgn = -sgn;
        if ((i & 1) == 1) {
            mp = mp.yzx;
            e = e.zxy;
        }
    }

    return normalize(mp);
}

// Cheap shadows are hard. In fact, I'd almost say, shadowing particular scenes with limited
// iterations is impossible... However, I'd be very grateful if someone could prove me wrong. :)
float softShadow(vec3 ro, vec3 lp, vec3 n, float k) {

    // More would be nicer. More is always nicer, but not always affordable. :)
    const int maxIterationsShad = 32;

    ro += n * .0015; // Coincides with the hit condition in the "trace" function.
    vec3 rd = lp - ro; // Unnormalized direction ray.

    float shade = 1.;
    float t = 0.;
    float end = max(length(rd), .0001);
    //float stepDist = end/float(maxIterationsShad);
    rd /= end;

    // Max shadow iterations - More iterations make nicer shadows, but slow things down. Obviously,
    // the lowest number to give a decent shadow is the best one to choose.
    for (int i = min(iFrame, 0); i < maxIterationsShad; i++) {
        float d = map(ro + rd * t);
        shade = min(shade, k * d / t);
        //shade = min(shade, smoothstep(0., 1., k*d/t)); // Thanks to IQ for this tidbit.
        // So many options here, and none are perfect: dist += clamp(d, .01, stepDist), etc.
        t += clamp(d, .005, .15);

        // Early exits from accumulative distance function calls tend to be a good thing.
        if (d < 0. || t > end) break;
    }

    // Sometimes, I'll add a constant to the final shade value, which lightens the shadow a bit --
    // It's a preference thing. Really dark shadows look too brutal to me. Sometimes, I'll add
    // AO also just for kicks. :)
    return max(shade, 0.);
}

// I keep a collection of occlusion routines... OK, that sounded really nerdy. :)
// Anyway, I like this one. I'm assuming it's based on IQ's original.
float calcAO(in vec3 p, in vec3 n) {
    float sca = 3., occ = 0.;
    for (int i = 0; i < 6; i++) {
        float hr = .01 + float(i) * .2 / 6.;
        float d = map(p + n * hr);
        occ += (hr - d) * sca;
        sca *= .75;
    }

    return clamp(1. - occ, 0., 1.);
}

// Regular fBm function that has existed for as long as I can
// remember, usually used in computer graphics to simulate natural
// phenomena. I normally know the history of various graphics
// functions, but I'm not sure who first started doing this...
// Just checking...
//
// A Soviet mathematician named Andrey Kolmogorov in the 1940s
// came up with the idea for fBm, apparently. It was adpated for
// computer usage by Benoît Mandelbrot and John W. Van Ness of all
// people... I learn something new every day. :)
vec4 fBm(vec2 p) {

    // Noise value, amplitue and total.
    vec4 n = vec4(0.);
    float a = 1., t = 0.;
    for (int i = 0; i < 5; i++) {
        // Accumulative noise frequency samples.
        n += texture(iChannel1, p / a) * a;
        t += a; // Total amplitude.
        a *= 2.; // Lacunarity factor.
    }

    // Average noise values. In this case there are
    // four of them.
    return n / t;
}

// I know of some pretty fancy marble routines, but as far
// as basic ones go, I like Belfry's example, here:
//
// Marmot -- Belfry
// https://www.shadertoy.com/view/3sfXzB
//
// This a just a quickly reworked version of that.
float marble(vec2 p) {

    // Four separate layers of FBM noise.
    vec4 n = fBm(vec2(1, 1.5) * p / 3.);
    // Two separate difference layers.
    vec2 m = pow(abs(vec2(n.x - n.z, n.y - n.w)) * 3., vec2(.15));
    //float c = 1. - (1. - m.x)*(1. - m.y);
    // Photoshop "screen" blend.
    float c = m.x + m.y - m.x * m.y;
    // Sharpen by tweaking to a higher power. Smoothstep would also work.
    c = pow(max(c * .9 + .1, 0.), 8.);

    return c; // m.x*m.y; //max(c, m.x*m.x);
}

// The normal function is just an application of the finite (central, forward)
// difference method. The less used curvature function is a second derivative
// extension of the former -- In fact, you can derive the curvature function
// from it.
//
// I think it's technically called a discrete finite difference approximation to
// the continuous Laplace differential operator? Either way, it gives you the
// curvature of a surface, which is pretty handy.
//
// Original pixelshader usage (I think?) - Cheap curvature:
// https://www.shadertoy.com/view/Xts3WM
//
// Other usage: Xyptonjtroz: https://www.shadertoy.com/view/4ts3z2
//
// spr: sample spread, amp: amplitude, offs: offset.
float curve(in vec3 p, in float spr, in float amp, in float offs) {
    spr /= 450.;

    float sgn = 1.;
    vec3 e = vec3(spr, 0, 0);
    float d = -map(p) * 6.;
    for (int i = min(iFrame, 0); i < 6; i++) {
        d += map(p + sgn * e);
        sgn = -sgn;
        if ((i & 1) == 1) {
            e = e.zxy;
        }
    }

    // By the way, I take a lot of liberties with this part of the formula.
    // Dividing by the sample spread squared (e.x*e.x) is technically correct,
    // but I'll sometimes divide by other things to get the result I want.
    //
    return clamp(d / e.x / e.x * amp / 16. + offs, -1., 1.) * .5 + .5;
    //return smoothstep(-1., 1., d/e.x/e.x*amp/16. + offs);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {

    // Screen coordinates.
    vec2 uv = (fragCoord - iResolution.xy * .5) / iResolution.y;

    // Camera Setup.
    vec3 ro = vec3(0, 1., -.65); // Camera position, or ray origin.
    vec3 lk = vec3(0, 0, 0); //vec3(0, -.25, iTime);  // "Look At" position.

    // Light positioning.
    vec3 lp = ro + vec3(.75, 0, 1); // Put it a bit in front of the camera.

    // Using the above to produce the unit ray-direction vector.
    float FOV = 1.; // FOV - Field of view.
    vec3 fwd = normalize(lk - ro); // Forward.
    //if(dot(fwd, vec3(fwd.z, 0, -fwd.x))==0.) fwd = normalize(fwd - vec3(0, 0, .00001));
    vec3 rgt = normalize(cross(vec3(0, 1, 0), fwd)); // Right.
    // "right" and "forward" are perpendicular normals, so the result is normalized.
    vec3 up = cross(fwd, rgt); // Up.

    // Camera.
    //mat3 mCam = mat3(rgt, up, fwd);
    // rd - Ray direction.
    //vec3 rd = mCam*normalize(vec3(uv, 1./FOV));//
    vec3 rd = normalize(uv.x * rgt + uv.y * up + fwd / FOV);

    // Raymarch to the scene.
    float t = trace(ro, rd);

    // Save the object ID.

    objID = vObj[0] < vObj[1] ? 0 : 1;

    /*
        objID = 0;
        float minDist = 1e5;
        // Sorting more objects.
        for(int i = 0; i<4; i++){
           if(vObj[i]<minDist){
               minDist = vObj[i];
               objID = i;
           }
        }
        */

    vec2 svP = gP;
    vec3 svCir = gCir;
    float svSc = gSc;

    // Initiate the scene color to black.
    vec3 col = vec3(0);

    // The ray has effectively hit the surface, so light it up.
    if (t < FAR) {

        // Surface position and surface normal.
        vec3 sp = ro + rd * t;
        vec3 sn = normal(sp);

        // Light direction vector.
        vec3 ld = lp - sp;

        // Distance from respective light to the surface point.
        float lDist = max(length(ld), .001);

        // Normalize the light direction vector.
        ld /= lDist;

        // Shadows and ambient self shadowing.
        float sh = softShadow(sp, lp, sn, 16.);
        float ao = calcAO(sp, sn); // Ambient occlusion.

        // Scene curvature.
        float spr = 5., ampC = 1., offs = .0;
        float crv = curve(sp, spr, ampC, offs);

        // Light attenuation, based on the distances above.
        float atten = 1. / (1. + lDist * .05);

        // Obtaining the texel color.
        vec3 texCol = vec3(1);

        //float sf = 1.5/iResolution.y;
        //float ew = .005;

        vec2 txP = svP / svCir.z;

        // Material properties.
        float type = .8; //ns2*.1;
        float rough = .25; //ns*.4 + .1;
        float fresRef = .5;

        if (objID > 0) {

            // The packed copper\gold ring structure.
            vec3 tx = tex3D(iChannel0, vec3(txP.x, sp.y, txP.y) / 2., sn);
            float gr = dot(tx, vec3(.299, .587, .114));

            rough = gr * 2.;

            texCol = vec3(.7, .4, .15);

            texCol *= tx * 1.;
        }
        else {

            // The cheap marble floor.
            vec3 tx = tex3D(iChannel0, vec3(txP.x, sp.y, txP.y), sn);
            float gr = dot(tx, vec3(.299, .587, .114));

            type = .2;
            rough = gr * 3.;

            // Cheap, but reasonably effective marble routine.
            float marb = marble(txP);

            texCol = vec3(marb);

            texCol *= tx * .5 + .5;
        }
        /////////////////

        // Backscatter.
        float bac = clamp(dot(sn, -normalize(vec3(ld.x, 0, ld.z))), 0., 1.);
        texCol += texCol * vec3(1, .3, .1) * bac * 4.;

        // Ambient light.
        //
        // Quick Lighting Tech - blackle
        // https://www.shadertoy.com/view/ttGfz1
        // Studio and outdoor.
        //float amb = pow(length(sin(sn*2.)*.5 + .5)/sqrt(3.), 2.);
        float amb = length(sin(sn * 2.) * .5 + .5) / sqrt(3.) * smoothstep(-1., 1., sn.y);

        ///////////////

        // Standard BRDF dot product calculations.
        vec3 h = normalize(ld - rd); // Half vector.
        float ndl = dot(sn, ld);
        float nr = clamp(dot(sn, -rd), 0., 1.); // Leaving it here.
        float nl = clamp(ndl, 0., 1.);
        float nh = clamp(dot(sn, h), 0., 1.);
        float vh = clamp(dot(-rd, h), 0., 1.);
        // Fresnel related.
        vec3 f0 = vec3(.16 * (fresRef * fresRef));
        // For metals, the base color is used for F0.
        f0 = mix(f0, texCol, type);
        vec3 FS = f0 + (1. - f0) * pow(1. - vh, 5.); // Fresnel-Schlick reflected light term.

        // BRDF style specular and diffuse calculations. There is so little
        // extra work involved, but the lighting quality is much better.
        vec3 spec = getSpec(FS, nh, nr, nl, rough);
        vec3 diff = getDiff(FS, nl, type);
        float speR = pow(nh, 5.);

        //////////////////

        // Combining the above terms to produce the final color.
        col = texCol * (diff * sh + spec * sh + amb * (sh * .5 + .5));

        // Curvature shading, for a little extra depth.
        col *= crv * 1.33 + .25;

        // Shading.
        col *= ao * atten;

        vec3 rf = reflect(rd, sn); // Surface reflection.
        vec3 rTx = texture(iChannel2, rf).xyz;
        rTx *= rTx;
        float rF = objID > 0 ? 8. : .5;
        col = col + col * speR * rTx * rF;

        // It's sometimes helpful to check things like shadows and AO by themselves.
        //col = vec3(ao);
    }

    // Horizon fog. Not visible here, but provided for completeness.
    col = mix(col, vec3(0), smoothstep(.2, .99, t / FAR));

    // Sigmoid tone mapping: Basically, it brings down the really bright
    // high-frequency lighting.
    //
    // This particular one liner was popularized by XOR, and is a favorite
    // amongst the code-golfing crowd for its obvious low charater count
    // and reasonble quality.
    col = tanh(col);

    // Rough gamma correction.
    fragColor = vec4(pow(max(col, 0.), vec3(1) / 2.2), 1);
}
