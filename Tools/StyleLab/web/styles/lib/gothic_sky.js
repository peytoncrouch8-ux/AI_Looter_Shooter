// Ember Gothic's night sky: a deep blue-black dome, a huge full moon where the moonlight comes from (so the god rays
// and the backlit graves line up with it), its aureole and a faint corona, thin moonlit cloud banks with silver edges,
// stars that fade near the moon, and a small pale crescent second moon low in the south-east (Teropa has two).
// The kit's dome draws moons lit by the sun, which would make a moon at the light's own direction a new moon: this
// one draws it full, as the source of the light.

const VS = /* glsl */`
varying vec3 vDir;
void main() {
  vDir = normalize((modelMatrix * vec4(position, 0.0)).xyz);
  vec4 p = projectionMatrix * modelViewMatrix * vec4(position, 1.0);
  gl_Position = p.xyww;
}`;

const FS = /* glsl */`
varying vec3 vDir;
uniform float uSlTime;
uniform vec3 uSlSunDirW;
uniform vec3 uSlFogColor;
uniform vec3 uSlFogSunColor;
uniform vec4 uSlFogSun;
uniform vec3 uZenith, uHorizon, uGround, uMoonCol, uGlowCol, uCloudLit, uCloudDark, uMoon2Dir, uMoon2Col;
uniform float uMoonR, uMoon2R, uStars, uCloudAmt, uHorizonFog, uMoonBright;

float gHash12(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
float gHash13(vec3 p3) { p3 = fract(p3 * 0.1031); p3 += dot(p3, p3.zyx + 31.32); return fract((p3.x + p3.y) * p3.z); }
float gNoise(vec2 p) { vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
  return mix(mix(gHash12(i), gHash12(i + vec2(1, 0)), f.x), mix(gHash12(i + vec2(0, 1)), gHash12(i + vec2(1, 1)), f.x), f.y); }
float gFbm(vec2 p) { float v = 0.0, a = 0.5; for (int i = 0; i < 5; i++) { v += a * gNoise(p); p = mat2(1.6, 1.2, -1.2, 1.6) * p + 7.3; a *= 0.5; } return v; }

// a disc's local coordinates, azimuthal-equidistant round its centre: q in disc radii, no singularity away from it
vec2 discQ(vec3 d, vec3 b, float r) {
  vec3 e = normalize(cross(abs(b.y) > 0.999 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0), b));
  vec3 n = cross(b, e);
  vec2 t = vec2(dot(d, e), dot(d, n));
  float l = length(t);
  float ang = acos(clamp(dot(d, b), -1.0, 1.0));
  return l > 1e-7 ? t / l * (ang / r) : vec2(0.0);
}

void main() {
  vec3 d = normalize(vDir);
  float h = d.y;
  float up = max(h, 0.0);
  vec3 col = mix(uHorizon, uZenith, pow(smoothstep(0.0, 1.0, up), 0.45));
  col = mix(col, uGround, smoothstep(0.0, -0.06, h));
  vec3 M = normalize(uSlSunDirW);
  float cm = dot(d, M);
  // the moon's wide glow brightens the sky around it
  col += uGlowCol * (pow(max(cm, 0.0), 10.0) * 0.35 + pow(max(cm, 0.0), 120.0) * 0.6);
  // the horizon takes the fog's colour, so the fog sea melts into the sky
  float s = pow(max(cm, 0.0), uSlFogSun.y);
  vec3 fogC = mix(uSlFogColor, uSlFogSunColor, clamp(s * uSlFogSun.x, 0.0, 1.0));
  col = mix(col, fogC, uHorizonFog * (1.0 - smoothstep(-0.02, 0.2, h)));

  // stars, dimmer near the moon and the horizon
  if (uStars > 0.0 && h > 0.0) {
    vec3 sp = d * 300.0;
    vec3 cell = floor(sp);
    float hs = gHash13(cell);
    if (hs > 0.972) {
      vec3 c = cell + vec3(gHash13(cell + 1.7), gHash13(cell + 3.1), gHash13(cell + 5.3));
      float dist = length(sp - c);
      float tw = 0.65 + 0.35 * sin(uSlTime * (1.5 + hs * 4.0) + hs * 60.0);
      float st = smoothstep(0.11, 0.0, dist) * pow((hs - 0.972) / 0.028, 2.0) * tw;
      float away = 1.0 - smoothstep(0.75, 0.97, cm);
      col += mix(vec3(0.75, 0.85, 1.0), vec3(1.0, 0.9, 0.75), gHash13(cell + 9.1)) * st * uStars * 2.2 * smoothstep(0.02, 0.25, h) * away;
    }
  }

  // the moon: a full disc with maria and craters, a little limb darkening (derivatives outside any branch)
  vec2 q = discQ(d, M, uMoonR);
  float r = length(q);
  float aa = fwidth(r) * 1.5 + 1e-4;
  vec2 q2 = discQ(d, normalize(uMoon2Dir), uMoon2R);
  float r2 = length(q2);
  float aa2 = fwidth(r2) * 1.5 + 1e-4;
  float occl = 0.0;
  if (r < 8.0) {
    float disc = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, r);
    float z = sqrt(max(1.0 - r * r, 0.0));
    vec2 sq = q / (0.6 + 0.4 * z);
    float maria = smoothstep(0.42, 0.6, gFbm(sq * 2.3 + 4.0)) * 0.8 + smoothstep(0.5, 0.7, gFbm(sq * 5.0 + 11.0)) * 0.3;
    float crat = gFbm(sq * 7.0 + 1.3);
    vec3 surf = uMoonCol * (1.0 - 0.55 * maria) * (0.78 + 0.36 * crat) * (0.72 + 0.28 * pow(z, 0.5));
    col = mix(col, surf * uMoonBright, disc);
    occl = disc;
    // aureole: a tight bright halo hugging the limb, and a faint coloured corona ring
    float out_ = max(r - 1.0, 0.0);
    col += uGlowCol * (exp(-out_ * 9.0) * 1.2 + exp(-out_ * 2.0) * 0.2) * (1.0 - disc);
    float ring = exp(-pow((r - 2.35) * 3.2, 2.0));
    col += vec3(0.55, 0.6, 0.72) * ring * 0.05 * (1.0 - disc);
  }
  // the second moon: a thin pale crescent
  if (r2 < 6.0) {
    float disc2 = 1.0 - smoothstep(1.0 - aa2, 1.0 + aa2, r2);
    float lit = 1.0 - smoothstep(1.0 - aa2 * 2.0, 1.0 + aa2 * 2.0, length(q2 - vec2(-0.38, 0.22)));
    float earth = 0.06;
    vec3 c2 = uMoon2Col * mix(earth, 1.0, 1.0 - lit) * (0.85 + 0.25 * gFbm(q2 * 4.0 + 9.0));
    col = mix(col, c2, disc2);
    col += uMoon2Col * 0.12 * exp(-max(r2 - 1.0, 0.0) * 3.0) * (1.0 - disc2);
    occl = max(occl, disc2);
  }

  // thin moonlit cloud banks: stretched, layered, silver toward the moon, thin enough to show the moon through
  if (uCloudAmt > 0.0 && h > -0.02) {
    vec2 p = d.xz / (up + 0.08);
    vec2 pc = vec2(p.x * 0.28 + p.y * 0.05, p.y * 0.9) + vec2(uSlTime * 0.004, 0.0);
    float n = gFbm(pc * 0.9);
    float n2 = gFbm(pc * 2.6 + 3.0);
    float cover = 1.0 - uCloudAmt;
    float dens = smoothstep(cover - 0.08, cover + 0.22, n * 0.75 + n2 * 0.35) * smoothstep(-0.02, 0.12, h);
    dens *= 0.85;
    float toward = pow(max(cm, 0.0), 6.0);
    float edge = 1.0 - smoothstep(0.0, 0.5, dens);
    vec3 cc = mix(uCloudDark, uCloudLit, clamp(toward * 1.6 + n2 * 0.25, 0.0, 1.0));
    cc += uGlowCol * pow(max(cm, 0.0), 40.0) * edge * 2.5;
    cc = mix(cc, fogC, uHorizonFog * (1.0 - smoothstep(0.0, 0.2, h)) * 0.7);
    col = mix(col, cc, dens * (1.0 - occl * 0.55));
  }
  gl_FragColor = vec4(col, 1.0);
}`;

// opts: linear THREE.Colors and numbers (see Ember Gothic's file for the values)
export function makeGothicSky(kit, o) {
  const THREE = kit.THREE;
  const c = (v) => (v && v.isColor ? v : new THREE.Color(v[0], v[1], v[2]));
  const deg = Math.PI / 180;
  const dir = (az, el) => new THREE.Vector3(-Math.sin(az * deg) * Math.cos(el * deg), Math.sin(el * deg), Math.cos(az * deg) * Math.cos(el * deg));
  const uniforms = {
    uSlTime: kit.shared.uSlTime, uSlSunDirW: kit.shared.uSlSunDirW, uSlFogColor: kit.shared.uSlFogColor,
    uSlFogSunColor: kit.shared.uSlFogSunColor, uSlFogSun: kit.shared.uSlFogSun,
    uZenith: { value: c(o.zenith) }, uHorizon: { value: c(o.horizon) }, uGround: { value: c(o.ground) },
    uMoonCol: { value: c(o.moonColor) }, uGlowCol: { value: c(o.glowColor) },
    uCloudLit: { value: c(o.cloudLit) }, uCloudDark: { value: c(o.cloudDark) },
    uMoon2Dir: { value: dir(o.moon2.azimuth, o.moon2.elevation) }, uMoon2Col: { value: c(o.moon2.color) },
    uMoonR: { value: o.moonSize * 0.5 * deg }, uMoon2R: { value: o.moon2.size * 0.5 * deg },
    uStars: { value: o.stars }, uCloudAmt: { value: o.clouds }, uHorizonFog: { value: o.horizonFog },
    uMoonBright: { value: o.moonBright },
  };
  return new THREE.ShaderMaterial({
    uniforms, vertexShader: VS, fragmentShader: FS, side: THREE.BackSide, depthWrite: false, depthTest: true,
    toneMapped: false, fog: false,
  });
}
