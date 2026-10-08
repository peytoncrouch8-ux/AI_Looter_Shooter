// Sunbleached's material hooks (GLSL run on the lit colour; see the README's `hook`): a warm rim that blazes when the
// surface stands between the eye and the sun, a warm bounce lifting the shade, the sand's broad sheen toward the sun
// (Journey's "ocean specular"), and glitter: sparse facets in world-space cells that flash when they mirror the sun
// into the eye. The cells double in size with each doubling of distance, so a glint stays about two pixels wide.

// A warm rim, brightest backlit; colour as a GLSL vec3, strength k.
export const rimHook = (rim, k, pow = 3.0) => /* glsl */`
  {
    float ndv = max(dot(N, V), 0.0);
    float toward = pow(max(dot(-V, L), 0.0), 2.0);
    col += ${rim} * pow(1.0 - ndv, ${pow.toFixed(2)}) * (0.22 + 1.1 * toward) * (0.45 + 0.55 * ao) * ${k.toFixed(3)};
  }`;

// Lavender shade: whatever the sun doesn't reach shifts toward lavender (blue up, green down), so shadows read as cool
// colour, never grey, against the white-gold light.
export const shadeHook = (shade, k = 1) => /* glsl */`
  {
    float litK = shadow * smoothstep(-0.05, 0.3, NdL);
    col *= mix(vec3(1.0), ${shade}, (1.0 - litK) * ${k.toFixed(3)});
  }`;

// The shade lifted by light bouncing off the sunlit sand (warm, from below and the sides).
export const bounceHook = (bounce, k) => /* glsl */`
  {
    float lit = shadow * max(NdL, 0.0);
    float below = 0.55 - 0.45 * N.y;
    col += diffuseColor.rgb * ${bounce} * (1.0 - lit) * below * ao * ${k.toFixed(3)};
  }`;

// Glitter: density (share of cells that can glint), size (cell size at 2 m), strength, fade distance, spread (how far
// the facets tilt from the surface: small keeps the glints in the sheen toward the sun, as on real sand).
export const glitterHook = (density, size, k, far = 70, spread = 0.55) => /* glsl */`
  {
    float gd = length(worldPos - cameraPosition);
    float glod = floor(log2(max(gd, 2.0) / 2.0));
    float gcs = ${size.toFixed(4)} * exp2(glod);
    vec3 gcp = floor(worldPos / gcs);
    vec3 grn = vec3(slHash13(gcp), slHash13(gcp + 11.7), slHash13(gcp + 23.1)) * 2.0 - 1.0;
    vec3 gN = normalize(N + grn * ${spread.toFixed(3)});
    vec3 gH = normalize(L + V);
    float gsp = smoothstep(0.972, 0.994, dot(gN, gH));
    float gkeep = step(1.0 - ${density.toFixed(3)}, slHash13(gcp + 37.3));
    float gfade = 1.0 - smoothstep(${(far * 0.45).toFixed(1)}, ${far.toFixed(1)}, gd);
    float gtw = 0.75 + 0.25 * sin(uSlTime * 3.0 + slHash13(gcp + 5.0) * 40.0);
    col += slSunCol * gsp * gkeep * gfade * gtw * shadow * step(0.0, NdL) * ${k.toFixed(3)};
  }`;

// Glow from within: sunlight carried a little past the terminator and warmed, as through sandstone and sun-dried timber;
// the lit side takes a soft gold lift, the first stretch of shade a warm wrap.
export const innerGlowHook = (glow, k) => /* glsl */`
  {
    float wrapL = clamp((NdL + 0.35) / 1.35, 0.0, 1.0);
    float band = wrapL * (1.0 - smoothstep(0.3, 0.9, NdL));
    col += diffuseColor.rgb * slSunCol * ${glow} * band * mix(0.35, 1.0, shadow) * ${k.toFixed(3)};
  }`;

// The sand's sheen: a broad specular lobe toward the sun at grazing angles.
export const sheenHook = (k) => /* glsl */`
  {
    vec3 sR = reflect(-V, N);
    float sndv = max(dot(N, V), 0.0);
    float sh = pow(max(dot(sR, L), 0.0), 10.0) * (0.25 + 0.75 * pow(1.0 - sndv, 2.0));
    col += slSunCol * sh * shadow * ${k.toFixed(3)};
  }`;
