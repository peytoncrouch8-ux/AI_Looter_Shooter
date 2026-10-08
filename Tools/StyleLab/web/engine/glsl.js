// GLSL shared by the kit's materials, the sky and the post passes.

export const NOISE = /* glsl */`
float slHash12(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
float slHash13(vec3 p3) { p3 = fract(p3 * 0.1031); p3 += dot(p3, p3.zyx + 31.32); return fract((p3.x + p3.y) * p3.z); }
vec2 slHash22(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * vec3(0.1031, 0.1030, 0.0973)); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.xx + p3.yz) * p3.zy); }
float slIGN(vec2 p) { return fract(52.9829189 * fract(dot(p, vec2(0.06711056, 0.00583715)))); }
float slVNoise(vec2 p) { vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
  return mix(mix(slHash12(i), slHash12(i + vec2(1, 0)), f.x), mix(slHash12(i + vec2(0, 1)), slHash12(i + vec2(1, 1)), f.x), f.y); }
float slFbm(vec2 p) { float v = 0.0, a = 0.5; for (int i = 0; i < 5; i++) { v += a * slVNoise(p); p = mat2(1.6, 1.2, -1.2, 1.6) * p + 7.3; a *= 0.5; } return v; }
float slLuma(vec3 c) { return dot(c, vec3(0.2126, 0.7152, 0.0722)); }
`;

// Height fog with in-scattering toward the sun, the same in every kit material, the sky and the smoke.
// uSlFog = (density per metre at the base height, height falloff per metre, base height, start distance)
// uSlFogSun = (sun scatter amount, exponent, max opacity, unused)
export const FOG = /* glsl */`
uniform vec3 uSlFogColor;
uniform vec3 uSlFogSunColor;
uniform vec4 uSlFog;
uniform vec4 uSlFogSun;
uniform vec3 uSlSunDirW;
float slFogAmount(vec3 camPos, vec3 wp) {
  vec3 d = wp - camPos;
  float dist = length(d);
  float far = max(dist - uSlFog.w, 0.0);
  if (far <= 0.0 || uSlFog.x <= 0.0) return 0.0;
  float k = uSlFog.y;
  float h0 = camPos.y - uSlFog.z;
  float dy = d.y * (far / max(dist, 1e-4));
  float base = uSlFog.x * exp(-k * h0);
  float lineInt = (abs(k * dy) > 1e-4) ? (1.0 - exp(-k * dy)) / (k * dy) : 1.0;
  float od = base * far * lineInt;
  return min(1.0 - exp(-max(od, 0.0)), uSlFogSun.z);
}
vec3 slFogColorFor(vec3 viewDir) {
  float s = pow(max(dot(viewDir, uSlSunDirW), 0.0), uSlFogSun.y);
  return mix(uSlFogColor, uSlFogSunColor, clamp(s * uSlFogSun.x, 0.0, 1.0));
}
vec3 slApplyFog(vec3 col, vec3 camPos, vec3 wp) {
  float f = slFogAmount(camPos, wp);
  if (f <= 0.0) return col;
  return mix(col, slFogColorFor(normalize(wp - camPos)), f);
}
`;

// Tone mapping operators (three's own, with exposure as a parameter).
export const TONEMAP = /* glsl */`
vec3 slRRTAndODTFit(vec3 v) { vec3 a = v * (v + 0.0245786) - 0.000090537; vec3 b = v * (0.983729 * v + 0.4329510) + 0.238081; return a / b; }
vec3 slACES(vec3 color) {
  const mat3 I = mat3(vec3(0.59719, 0.07600, 0.02840), vec3(0.35458, 0.90834, 0.13383), vec3(0.04823, 0.01566, 0.83777));
  const mat3 O = mat3(vec3(1.60475, -0.10208, -0.00327), vec3(-0.53108, 1.10813, -0.07276), vec3(-0.07367, -0.00605, 1.07602));
  color = I * (color / 0.6); color = slRRTAndODTFit(color); color = O * color; return clamp(color, 0.0, 1.0);
}
vec3 slAgxContrast(vec3 x) { vec3 x2 = x * x; vec3 x4 = x2 * x2;
  return 15.5 * x4 * x2 - 40.14 * x4 * x + 31.96 * x4 - 6.868 * x2 * x + 0.4298 * x2 + 0.1191 * x - 0.00232; }
vec3 slAgX(vec3 color) {
  const mat3 S2R = mat3(vec3(0.6274, 0.0691, 0.0164), vec3(0.3293, 0.9195, 0.0880), vec3(0.0433, 0.0113, 0.8956));
  const mat3 R2S = mat3(vec3(1.6605, -0.1246, -0.0182), vec3(-0.5876, 1.1329, -0.1006), vec3(-0.0728, -0.0083, 1.1187));
  const mat3 Ins = mat3(vec3(0.856627153315983, 0.137318972929847, 0.11189821299995), vec3(0.0951212405381588, 0.761241990602591, 0.0767994186031903), vec3(0.0482516061458583, 0.101439036467562, 0.811302368396859));
  const mat3 Out = mat3(vec3(1.1271005818144368, -0.1413297634984383, -0.14132976349843826), vec3(-0.11060664309660323, 1.157823702216272, -0.11060664309660294), vec3(-0.016493938717834573, -0.016493938717834257, 1.2519364065950405));
  color = Ins * (S2R * color);
  color = clamp((log2(max(color, 1e-10)) + 12.47393) / (4.026069 + 12.47393), 0.0, 1.0);
  color = Out * slAgxContrast(color);
  color = pow(max(vec3(0.0), color), vec3(2.2));
  return clamp(R2S * color, 0.0, 1.0);
}
vec3 slNeutral(vec3 color) {
  const float S = 0.76; const float D = 0.15;
  float x = min(color.r, min(color.g, color.b)); float off = x < 0.08 ? x - 6.25 * x * x : 0.04; color -= off;
  float peak = max(color.r, max(color.g, color.b)); if (peak < S) return color;
  float d = 1.0 - S; float np = 1.0 - d * d / (peak + d - S); color *= np / peak;
  float g = 1.0 - 1.0 / (D * (peak - np) + 1.0); return mix(color, vec3(np), g);
}
vec3 slToSRGB(vec3 c) { c = clamp(c, 0.0, 1.0); return mix(c * 12.92, 1.055 * pow(c, vec3(1.0 / 2.4)) - 0.055, step(0.0031308, c)); }
vec3 slToLinear(vec3 c) { c = max(c, 0.0); return mix(c / 12.92, pow((c + 0.055) / 1.055, vec3(2.4)), step(0.04045, c)); }
`;

export const COLOR = /* glsl */`
vec3 slRgb2Hsv(vec3 c) { vec4 K = vec4(0.0, -1.0 / 3.0, 2.0 / 3.0, -1.0);
  vec4 p = mix(vec4(c.bg, K.wz), vec4(c.gb, K.xy), step(c.b, c.g)); vec4 q = mix(vec4(p.xyw, c.r), vec4(c.r, p.yzx), step(p.x, c.r));
  float d = q.x - min(q.w, q.y); float e = 1.0e-10; return vec3(abs(q.z + (q.w - q.y) / (6.0 * d + e)), d / (q.x + e), q.x); }
vec3 slHsv2Rgb(vec3 c) { vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0); vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
  return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y); }
`;
