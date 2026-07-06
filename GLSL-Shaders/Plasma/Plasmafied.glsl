void mainImage(out vec4 o, vec2 u) {
    float i, s;
    vec3 p,r = iResolution;
    for(o *=i; i++<32.;s = .002 + abs(s)*.3, o += 1. / s)
        p += vec3((u+u-r.xy)/r.y/2. * s, s),
        s +=1e1-length(p.xz)+length(ceil(p).xy);
   o = tanh( abs(vec4(2,5,1,0) / dot(cos(iTime+p),vec3(.2)))*o/6e4);
}

