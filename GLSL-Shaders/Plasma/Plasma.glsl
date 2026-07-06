#define PLASMA_AMPLITUDE 32767.0
#define WAVE_FREQUENCY 0.0001
#define PALETTE_NORMALIZER 0.00390625
#define COLOR_QUANTIZATION 256.0
#define MIN_LINE_PIXEL_WIDTH 1.5
#define LINE_RATIO 0.18
#define SPEED 1.8
float
calculatePlasmaWave (float phase)
{
  return cos (phase * WAVE_FREQUENCY) * PLASMA_AMPLITUDE + PLASMA_AMPLITUDE;
}

vec3
convertHsvToRgb (vec3 hsv)
{
  vec3 rgb =
    clamp (abs (mod (hsv.x * 6.0 + vec3 (0.0, 4.0, 2.0), 6.0) - 3.0) - 1.0,
	   0.0, 1.0);
  rgb = rgb * rgb * (3.0 - 2.0 * rgb);
  return hsv.z * mix (vec3 (1.0), rgb, hsv.y);
}

vec3
getPaletteColor (int s)
{
  return s % 4 == 0 ? vec3 (0.0) : s % 4 == 2 ? vec3 (1.0) : s ==
    9 ? vec3 (1.0, 0.5, 0.0) : s == 15 ? vec3 (0.0, 1.0,
					       0.8) :
    convertHsvToRgb (vec3 ((float (s / 2) * 0.125 + 0.5), 1.0, 1.0));
}

vec3
sampleColorPalette (float t)
{
  t = fract (t) * 16.0;
  int s = int (t);
  return mix (getPaletteColor (s), getPaletteColor ((s + 1) % 16),
	      smoothstep (0.0, 1.0, fract (t)));
}

void
mainImage (out vec4 fragColor, in vec2 fragCoord)
{
  float time = iTime * SPEED;
  vec2 screenUV = fragCoord / iResolution.xy;
  float aspectRatio = min (iResolution.x, iResolution.y);
  float gridCellSize =
    min (160.0 * mix (1.0, 915.2 / length (iResolution.xy), 0.8),
	 aspectRatio * 0.2);
  vec2 aspectCorrectedCoord =
    (screenUV - 0.5) * iResolution.xy / aspectRatio + 0.5;
  vec2 gridCellCenter =
    floor (aspectCorrectedCoord * gridCellSize) / gridCellSize +
    0.5 / gridCellSize;
  vec4 timeOscillators =
    vec4 (sin (time * vec2 (0.05, 0.027)), cos (time * vec2 (0.013, 0.08)));
  vec2 animatedGridCenter =
    (gridCellCenter +
     vec2 (dot (timeOscillators, vec4 (0.3, 0.8, 0.15, 0.2)),
	   dot (timeOscillators.wzxy,
		vec4 (0.12, 0.6, sin (time * 0.019),
		      cos (time * 0.031)))) - 0.5) * (0.35 +
						      sin (time * 0.1) *
						      0.12);
  float c = cos (time * 0.0195), s = sin (time * 0.0195);
  vec2 rotatedCoordinate =
    (mat2 (c, -s, s, c) * animatedGridCenter + 0.5) * 180.0;
  vec4 plasmaTimeOscillators =
    mod (vec4
	 (sin (time * 0.23), cos (time * 0.15), sin (time * 0.28),
	  cos (time * 0.17)) * 20000.0 + 32768.0, 65536.0);
  vec4 plasmaPhaseAngles =
    vec4 (plasmaTimeOscillators.x * 0.3 + rotatedCoordinate.x * 367.0,
	  8192.0 + rotatedCoordinate.x * 453.0,
	  plasmaTimeOscillators.z * 0.3 + rotatedCoordinate.y * 472.0,
	  4096.0 + rotatedCoordinate.y * 419.0);
  vec4 plasmaWaveValues =
    cos (plasmaPhaseAngles * WAVE_FREQUENCY) * PLASMA_AMPLITUDE +
    PLASMA_AMPLITUDE;
  vec2 interferencePattern =
    (vec2 (plasmaPhaseAngles.w, plasmaPhaseAngles.y) +
     vec2 (plasmaWaveValues.x,
	   plasmaWaveValues.w)) * PALETTE_NORMALIZER * 0.5;
  float palettePosition =
    mod (dot
	 (vec4
	  (calculatePlasmaWave (interferencePattern.x * COLOR_QUANTIZATION),
	   calculatePlasmaWave (interferencePattern.y * COLOR_QUANTIZATION),
	   plasmaWaveValues.yz), vec4 (0.25)), 65536.0);
  float normalizedPaletteIndex = palettePosition * PALETTE_NORMALIZER / 255.0;
  float breathingEffect =
    0.02 + 0.372 * pow (sin (time * 0.03) * 0.5 + 0.5, 2.0);
  float animatedPaletteIndex =
    normalizedPaletteIndex +
    breathingEffect * sin (normalizedPaletteIndex * 6.283) * 0.25;
  vec3 finalColor =
    sampleColorPalette (animatedPaletteIndex) +
    (fract (sin (dot (fragCoord * 0.1, vec2 (12.9898, 78.233))) * 43758.5453)
     - 0.5) * 0.003;
  vec2 gridUV = aspectCorrectedCoord * gridCellSize;
  vec2 gridDist = min (fract (gridUV), 1.0 - fract (gridUV));
  vec2 gridDeriv = fwidth (gridUV);
  vec2 drawWidth = max (gridDeriv * MIN_LINE_PIXEL_WIDTH, vec2 (LINE_RATIO));
  finalColor =
    clamp (finalColor, 0.0,
	   1.0) * min (smoothstep (0.0, drawWidth.x, gridDist.x),
		       smoothstep (0.0, drawWidth.y, gridDist.y));
  fragColor = vec4 (finalColor, 1.0);
}
