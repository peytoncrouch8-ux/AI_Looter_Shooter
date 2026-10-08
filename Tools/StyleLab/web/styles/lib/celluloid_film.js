// Celluloid West's film stage: the frame as a 1960s widescreen print. Scene-referred passes first (an anamorphic
// streak on the brightest lights, then the "print": exposure into log stops, dye-layer crosstalk that saturates reds
// and blues, a per-channel characteristic curve with a crushed toe and a soft shoulder, greens pulled toward olive),
// then, on the display image, the projector: gate weave and a breath of flicker, dye-cloud grain that jumps at 24 frames
// a second, the odd speck of dust, and the lens's warm falloff.
// In Unreal: the print stage is a LUT (bake this pass over a neutral LUT strip); streak, weave, grain and specks are
// one post-process material after tonemapping (the streak can reuse the bloom's convolution kernel).

// ---------------------------------------------------------------- scene-referred: anamorphic streak
const STREAK = /* glsl */`
uniform float uThresh, uStrength, uSpread;
uniform vec3 uStreakCol;
vec4 effect(vec2 uv) {
  vec3 c = texture2D(tColor, uv).rgb;
  vec3 acc = vec3(0.0);
  float px = uSpread * (uResolution.y / 1080.0) / uResolution.x;
  // each pixel samples between the taps a little differently, so a small bright source smears into a line, not dashes
  float jit = slIGN(gl_FragCoord.xy) - 0.5;
  for (int i = -20; i <= 20; i++) {
    if (i == 0) continue;
    float fi = float(i) + jit;
    vec3 s = texture2D(tColor, uv + vec2(fi * px, 0.0)).rgb;
    float b = max(max(s.r, max(s.g, s.b)) - uThresh, 0.0);
    acc += s / max(max(s.r, max(s.g, s.b)), 1e-4) * b * exp(-abs(fi) * 0.16);
  }
  c += acc * uStreakCol * uStrength * 0.1;
  return vec4(c, 1.0);
}`;

export function streak(kit, o = {}) {
  const params = { threshold: o.threshold ?? 6, strength: o.strength ?? 0.5, spread: o.spread ?? 9, color: o.color || '#ffd9b0' };
  return kit.post.custom({
    stage: 'hdr', fragment: STREAK, params,
    uniforms: { uThresh: { value: 6 }, uStrength: { value: 0.5 }, uSpread: { value: 9 }, uStreakCol: { value: new kit.THREE.Color() } },
    bind: (p, u) => { u.uThresh.value = p.threshold; u.uStrength.value = p.strength; u.uSpread.value = p.spread; u.uStreakCol.value.copy(kit.color(p.color)); },
  });
}

// ---------------------------------------------------------------- scene-referred: the print
// Output is display-referred but linear (the engine's 'none' tone mapping then only encodes sRGB).
const PRINT = /* glsl */`
uniform float uExpo, uCross, uToeK, uShK, uMid, uOlive, uSat;
uniform vec3 uOffset;     // per-channel shift of the curve, in stops (warm highlights, inky blue-black shadows)
uniform vec3 uBlack;      // the print's base density colour (what pure black prints as)
vec3 rgb2hsv(vec3 c) { vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g)); vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
  float d = q.x - min(q.w, q.y); float e = 1.0e-10; return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x); }
vec3 hsv2rgb(vec3 c) { vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0); vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y); }
float curve(float x) {
  // a logistic in stops around mid grey (0 stops prints at uMid): steeper below (the crushed toe), gentler above (the
  // soft shoulder), the slope easing from one to the other so gradients stay smooth
  float k = mix(uToeK, uShK, smoothstep(-1.5, 1.5, x));
  return 1.0 / (1.0 + exp(-k * x + log(1.0 / uMid - 1.0)));
}
vec4 effect(vec2 uv) {
  vec3 c = max(texture2D(tColor, uv).rgb, 0.0) * uExpo;
  vec3 le = log2(c + 1e-4) - log2(0.18);
  // dye crosstalk: each layer sharpened against the other two (interimage effect); keeps grey grey
  vec3 m = vec3((le.r + le.g + le.b) / 3.0);
  le = m + (le - m) * (1.0 + uCross);
  le += uOffset;
  vec3 y = vec3(curve(le.r), curve(le.g), curve(le.b));
  // greens toward olive and a little quieter; reds and blues kept rich
  vec3 h = rgb2hsv(clamp(y, 0.0, 1.0));
  // greens lean to olive and go quieter; yellows burn toward sienna (no mustard); reds and blues stay rich
  float green = exp(-pow((h.x - 0.3) * 6.0, 2.0));
  float yellow = exp(-pow((h.x - 0.14) * 14.0, 2.0));
  h.x = mix(h.x, 0.2, green * uOlive * 0.3);
  h.x -= yellow * 0.025 * uOlive;
  h.y *= mix(uSat, uSat * 0.7, green * uOlive);
  y = hsv2rgb(h);
  y = uBlack + y * (1.0 - uBlack);
  return vec4(slToLinear(y), 1.0);
}`;

export function print(kit, o = {}) {
  const params = Object.assign({ exposure: 1, crosstalk: 0.22, toe: 0.95, shoulder: 0.68, mid: 0.4, olive: 1, saturation: 1.08,
    offset: [0.12, 0.0, -0.1], black: '#0b0907' }, o);
  return kit.post.custom({
    stage: 'hdr', fragment: PRINT, params,
    uniforms: { uExpo: { value: 1 }, uCross: { value: 0.2 }, uToeK: { value: 0.9 }, uShK: { value: 0.7 }, uMid: { value: 0.4 },
      uOlive: { value: 1 }, uSat: { value: 1 }, uOffset: { value: new kit.THREE.Vector3() }, uBlack: { value: new kit.THREE.Vector3() } },
    bind: (p, u) => {
      u.uExpo.value = p.exposure; u.uCross.value = p.crosstalk; u.uToeK.value = p.toe; u.uShK.value = p.shoulder; u.uMid.value = p.mid;
      u.uOlive.value = p.olive; u.uSat.value = p.saturation; u.uOffset.value.set(...p.offset);
      const b = kit.color(p.black).convertLinearToSRGB();
      u.uBlack.value.set(b.r, b.g, b.b);
    },
  });
}

// ---------------------------------------------------------------- display-referred: the projector
const PROJECTOR = /* glsl */`
uniform float uWeave, uFlicker, uSoften, uGrain, uGrainSize, uSpecks;
float frameN() { return floor(uTime * 24.0); }
vec4 effect(vec2 uv) {
  float f = frameN();
  // gate weave: the frame drifts a fraction of a pixel, mostly sideways, new every frame; scaled a hair to hide edges
  vec2 w = vec2(slVNoise(vec2(f * 0.37, 1.3)) - 0.5, slVNoise(vec2(3.1, f * 0.29)) - 0.5) * vec2(1.0, 0.6);
  w += vec2(slHash12(vec2(f, 7.0)) - 0.5, slHash12(vec2(f, 13.0)) - 0.5) * 0.35;
  vec2 st = (uv - 0.5) * (1.0 - 0.004 * uWeave) + 0.5 + w * uWeave * 2.2 / uResolution;
  // a print is softer than a render: a light four-tap blur
  vec2 t = uSoften / uResolution;
  vec3 c = texture2D(tColor, st).rgb * 0.4
    + (texture2D(tColor, st + vec2(t.x, t.y)).rgb + texture2D(tColor, st + vec2(-t.x, t.y)).rgb
     + texture2D(tColor, st + vec2(t.x, -t.y)).rgb + texture2D(tColor, st + vec2(-t.x, -t.y)).rgb) * 0.15;
  // the projector lamp breathes
  c *= 1.0 + (slHash12(vec2(f, 3.7)) - 0.5) * uFlicker;
  // dye-cloud grain: three layers, partly correlated, clumped at two scales, strongest in the mid and low tones
  vec2 p = gl_FragCoord.xy / max(uGrainSize * uResolution.y / 1080.0, 0.5);
  vec2 o = vec2(slHash12(vec2(f, 1.0)), slHash12(vec2(f, 2.0))) * 512.0;
  float mono = slVNoise(p + o) * 0.65 + slVNoise(p * 2.3 + o.yx) * 0.35;
  vec3 dye = vec3(slVNoise(p * 1.1 + o + 17.0), slVNoise(p * 1.05 + o + 41.0), slVNoise(p * 0.95 + o + 73.0));
  vec3 n = mix(vec3(mono), dye, 0.45) - 0.5;
  float y = slLuma(c);
  float resp = 0.35 + 2.4 * y * (1.0 - y) * (1.2 - y);
  c += n * uGrain * resp * 2.0;
  // dust: now and then a dark speck or a hair on the print, gone the next frame
  if (uSpecks > 0.0) {
    for (int i = 0; i < 3; i++) {
      vec2 sd = vec2(f * 1.7 + float(i) * 11.0, float(i) * 3.3);
      if (slHash12(sd) > 1.0 - uSpecks * 0.35) {
        vec2 sp = vec2(slHash12(sd + 1.0), slHash12(sd + 2.0));
        vec2 d = (uv - sp) * vec2(uResolution.x / uResolution.y, 1.0);
        float r = 0.0015 + 0.003 * slHash12(sd + 3.0);
        float blob = 1.0 - smoothstep(r * 0.5, r, length(d + (slVNoise(d * 900.0) - 0.5) * r * 0.8));
        c = mix(c, c * 0.25, blob * 0.8);
      }
    }
  }
  return vec4(clamp(c, 0.0, 1.0), 1.0);
}`;

export function projector(kit, o = {}) {
  const params = Object.assign({ weave: 1, flicker: 0.025, soften: 0.6, grain: 0.05, grainSize: 1.6, specks: 1 }, o);
  return kit.post.custom({
    stage: 'ldr', fragment: PROJECTOR, params,
    uniforms: { uWeave: { value: 1 }, uFlicker: { value: 0 }, uSoften: { value: 0.6 }, uGrain: { value: 0.05 }, uGrainSize: { value: 1.6 }, uSpecks: { value: 1 } },
    bind: (p, u) => {
      u.uWeave.value = p.weave; u.uFlicker.value = p.flicker; u.uSoften.value = p.soften; u.uGrain.value = p.grain;
      u.uGrainSize.value = p.grainSize; u.uSpecks.value = p.specks;
    },
  });
}
