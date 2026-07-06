// Created by Alex Kluchikov
// Licensed under CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/)


#define PI 3.141592654

float wave(float x)
{
    float s=sin(x);
    return s*abs(s);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-iResolution.xy*.5)/iResolution.yy*2.;
    float a=(atan(uv.x,uv.y)/2./PI)+.5;
    float l=length(uv);
    float w=0.1/(1.+l);
/*    
    a=uv.x*0.15;
    l=uv.y*0.2+.4;
    w=0.05;
*/    
    l*=(1.+w*wave(a*10.*PI));
    a+=w*wave(l*1.1*PI)-iTime*0.05;
    l*=(1.+w*wave(a*14.*PI));
    a+=w*wave(l*2.6*PI)+iTime*0.075;
    l*=(1.+w*wave(a*12.*PI));
    a+=w*wave(l*4.1*PI)-iTime*0.035;
    l*=(1.+w*wave(a*8.*PI));
    a+=w*wave(l*5.6*PI)+iTime*0.045;

    float r=a*12.+l-iTime*.2;
    float g=l*3.+a+iTime*.3;
    float b=-a*6.+l*2.+iTime*.25;
    
    vec3 c=vec3(
        fract(r)>.5?0.:1.,
        fract(g)>.5?0.:1.,
        fract(b)>.5?0.:1.
    );
    
    if(iMouse.z<0.1)
        c=(vec3(
    	    sin(r*2.*PI),
	        sin(g*2.*PI),
        	sin(b*2.*PI)
        )+1.)*0.5;
    
    c=mix(vec3(.7,.9,1),vec3(.2,.5,1),sqrt(dot(c,c)))*(1.2-c*.2);
    
    fragColor = vec4(c,1);
}
