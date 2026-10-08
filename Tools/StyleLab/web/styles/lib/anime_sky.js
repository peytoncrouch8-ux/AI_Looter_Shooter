// A hand-made sky dome for the cartoon styles (01 Clay Frontier, 02 Skyward Anime, 03 Painted Frontier): a three-stop
// gradient, towering cumulus standing on the horizon and floating fair-weather clusters, all built from lit spheres
// ("puffs") so every cloud has a real rounded form the sun can model. One shader, three temperaments: crisp two-tone cel
// clouds (anime), soft brushed clouds (painted), or felt cut-outs hung in front of a painted backdrop that they shadow
// (clay). Colours are given as they should look on screen and are inverse tone mapped here, as the engine does.
import { NOISE, FOG } from '../../engine/glsl.js';
import { inverseToneMap } from '../../engine/tonecurve.js';

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
uniform vec3 uZenith, uMid, uHorizon, uGround, uSunCol;
uniform vec3 uCloudLit, uCloudShade, uCloudDark, uCloudRim;
uniform vec4 uGrad;    // mid stop (0..1 of the way up), gradient power, horizon haze, brightness
uniform vec4 uSun;     // disc radius (rad), disc strength, glow strength, glare streaks
uniform vec4 uTow;     // towers around the horizon (0..12), height scale, width scale, seed
uniform vec4 uPuff;    // floating clusters: amount (0..1), size, lowest elevation, highest elevation (rad)
uniform vec4 uShade;   // cel edge, cel softness, wrap, rim strength
uniform vec4 uTex;     // felt fibres, brush strokes, edge warp, backdrop shadow
uniform vec4 uMisc;    // drift speed, bottom darkening, sun bleed on clouds, edge softness
uniform vec4 uMisc2;   // top light bias, mass blend, -, -
${NOISE}
${FOG}
const float PI = 3.14159265;
float hs(float n) { return fract(sin(n * 127.1 + 311.7) * 43758.5453); }

// the front-most puff under a direction wins: its sphere normal (x along azimuth, y up the dome, z toward the eye)
vec3 gN, gM, gMass; float gZ, gCov, gLvl;

void puff(vec2 q, float r, float zo, float lvl, float aboveFloor) {
  float d = length(q) / r;
  float soft = uMisc.w;
  if (d > 1.0 + soft) return;
  float cov = 1.0 - smoothstep(1.0 - soft, 1.0 + soft * 0.4, d);
  cov *= smoothstep(-0.003, 0.003, aboveFloor);
  if (cov <= 0.0) return;
  float nz = sqrt(max(1.0 - min(d * d, 1.0), 0.0));
  float z = nz * r + zo;
  gCov = max(gCov, cov);
  if (z > gZ) { gZ = z; gN = vec3(q / r, nz); gLvl = lvl; gM = gMass; }
}

float wrapPI(float x) { return mod(x + PI, 2.0 * PI) - PI; }

void towers(vec2 p) {
  int N = int(uTow.x + 0.5);
  for (int i = 0; i < 12; i++) {
    if (i >= N) break;
    float fi = float(i) * 1.731 + uTow.w * 17.0;
    float ca = (float(i) + 0.15 + 0.7 * hs(fi + 0.1)) * 2.0 * PI / float(N);
    float W = (0.10 + 0.12 * hs(fi + 0.2)) * uTow.z;
    float H = (0.06 + 0.17 * hs(fi + 0.3)) * uTow.y;
    float dx = wrapPI(p.x - ca);
    if (abs(dx) > W * 1.9 || p.y > H + W * 0.8) continue;
    // the whole tower's rounded mass, so the shading reads as one heaped form with bumps on it
    vec2 mq = vec2(dx / (W * 1.3), (p.y - H * 0.35) / (H * 0.75 + W * 0.5));
    gMass = normalize(vec3(mq, sqrt(max(1.0 - dot(mq, mq), 0.04))));
    for (int k = 0; k < 20; k++) {
      float fk = float(k);
      float r1 = hs(fi * 7.0 + fk * 1.3), r2 = hs(fi * 5.0 + fk * 2.7), r3 = hs(fi * 3.0 + fk * 4.1);
      float t, x, r;
      if (k < 6) {
        // a broad shelf at the base
        t = 0.06 * r3;
        x = (fk / 5.0 - 0.5) * 1.8 * W + (r1 - 0.5) * W * 0.25;
        r = W * (0.28 + 0.14 * r2);
      } else {
        // a heaped body and a cauliflower crown of mixed bulges, wide all the way up
        t = pow((fk - 6.0) / 13.0, 0.8);
        x = (r1 - 0.5) * 2.0 * W * (0.9 - 0.45 * t);
        r = W * (0.36 - 0.10 * t) * (0.65 + 0.7 * r2);
      }
      float y = -0.03 + H * pow(t, 0.85);
      puff(vec2(dx - x, p.y - y), r, r3 * r * 0.6 + t * W * 0.12, t, p.y + 0.03);
    }
  }
}

void floaters(vec2 p) {
  const float NC = 15.0;
  float cw = 2.0 * PI / NC;
  float ch = 0.17;
  vec2 g = vec2(p.x / cw, (p.y - uPuff.z) / ch);
  vec2 gi = floor(g);
  for (int j = -1; j <= 1; j++) for (int i = -1; i <= 1; i++) {
    vec2 cell = gi + vec2(float(i), float(j));
    float cy0 = uPuff.z + cell.y * ch;
    if (cell.y < 0.0 || cy0 > uPuff.w) continue;
    float cx = mod(cell.x, NC);
    vec2 key = vec2(cx, cell.y) + uTow.w * 3.7;
    if (slHash12(key) > uPuff.x) continue;
    vec2 c = vec2((cell.x + 0.2 + 0.6 * slHash12(key + 3.1)) * cw, cy0 + (0.25 + 0.5 * slHash12(key + 5.3)) * ch);
    float sz = uPuff.y * (0.45 + 0.75 * sin(c.y)) * (0.7 + 0.6 * slHash12(key + 7.9));
    float cs = cos(c.y);
    vec2 q0 = vec2(wrapPI(p.x - c.x) * cs, p.y - c.y);
    if (abs(q0.x) > sz * 2.2 || abs(q0.y) > sz * 1.3) continue;
    vec2 mq = vec2(q0.x / (sz * 1.6), (q0.y - sz * 0.1) / (sz * 0.9));
    gMass = normalize(vec3(mq, sqrt(max(1.0 - dot(mq, mq), 0.04))));
    float floorY = -sz * 0.32;
    for (int k = 0; k < 7; k++) {
      float fk = float(k);
      float r1 = slHash12(key + fk * 1.7), r2 = slHash12(key + fk * 2.9 + 1.0), r3 = slHash12(key + fk * 4.3 + 2.0);
      float x = (fk / 6.0 - 0.5) * 2.4 * sz + (r1 - 0.5) * sz * 0.4;
      float r = sz * (0.38 + 0.32 * r2) * (1.0 - 0.45 * abs(fk / 6.0 - 0.5) * 2.0);
      float y = floorY + r * (0.55 + 0.35 * r3);
      puff(q0 - vec2(x, y), r, r3 * r * 0.4, 0.55 + 0.45 * (y - floorY) / sz, q0.y - floorY);
    }
  }
}

void clouds(vec2 p) {
  gN = vec3(0.0, 0.0, 1.0); gM = gN; gMass = gN; gZ = -1e9; gCov = 0.0; gLvl = 1.0;
  if (uTow.x > 0.5 && p.y < 0.75) towers(p);
  if (uPuff.x > 0.0 && p.y > uPuff.z - 0.2) floaters(p);
}

void main() {
  vec3 d = normalize(vDir);
  float h = d.y;
  float up = max(h, 0.0);
  // the gradient: horizon -> mid -> zenith
  float t = pow(smoothstep(0.0, 1.0, up), uGrad.y);
  float m = uGrad.x;
  vec3 col = t < m ? mix(uHorizon, uMid, smoothstep(0.0, 1.0, t / max(m, 1e-3)))
                   : mix(uMid, uZenith, smoothstep(0.0, 1.0, (t - m) / max(1.0 - m, 1e-3)));
  col = mix(col, uGround, smoothstep(0.0, -0.06, h));
  vec3 S = uSlSunDirW;
  float cs = dot(d, S);
  // clouds in azimuth/elevation space, the domain warped a little so the edges are lumpy or brushed
  vec2 p = vec2(atan(d.x, d.z), asin(clamp(h, -1.0, 1.0)));
  p.x += uSlTime * uMisc.x;
  if (uTex.z > 0.0) p += (vec2(slFbm(p * vec2(9.0, 14.0)), slFbm(p * vec2(9.0, 14.0) + 5.2)) - 0.5) * uTex.z;
  float cov = 0.0;
  vec3 cc = vec3(0.0);
  if (h > -0.06) {
    // the backdrop shadow first (the clouds hang in front of a painted flat)
    float bshadow = 0.0;
    if (uTex.w > 0.0) {
      vec2 off = -normalize(vec2(dot(S, normalize(vec3(d.z, 0.0, -d.x) + 1e-5)), max(S.y, 0.15))) * 0.035;
      clouds(p + off);
      bshadow = gCov;
    }
    clouds(p);
    cov = gCov;
    col *= 1.0 - uTex.w * bshadow * (1.0 - cov);
    if (cov > 0.0) {
      vec3 T = normalize(vec3(d.z, 0.0, -d.x) + 1e-5);
      vec3 U = normalize(vec3(0.0, 1.0, 0.0) - d * h + 1e-5);
      vec3 nl = normalize(mix(gN, gM, uMisc2.y));
      vec3 N = normalize(T * nl.x + U * nl.y - d * nl.z);
      // the sun's side lit, with a bias to the tops (sky light from above), so the forms read as heaped domes
      float lit = mix(dot(N, S), N.y * 0.9 + 0.1, uMisc2.x);
      float lw = (lit + uShade.z) / (1.0 + uShade.z);
      float cel = smoothstep(uShade.x - uShade.y, uShade.x + uShade.y, lw);
      cc = mix(uCloudShade, uCloudLit, cel);
      // lower in a tower, deeper in its own shade
      cc = mix(mix(uCloudDark, cc, 0.35), cc, mix(1.0, smoothstep(0.0, 0.55, gLvl), uMisc.y));
      // a bright rim where the sun is behind the cloud, and a warm bleed near the sun
      float rim = pow(1.0 - gN.z, 3.0) * uShade.w * (0.25 + 1.5 * pow(max(cs, 0.0), 4.0));
      cc += uCloudRim * rim;
      cc += uSunCol * pow(max(cs, 0.0), 6.0) * uMisc.z * (1.0 - cel * 0.5);
      if (uTex.x > 0.0) {
        // felt: fine fibres and a fuzzy nap
        float f = slVNoise(p * vec2(520.0, 520.0)) * 0.6 + slVNoise(p * vec2(1400.0, 160.0)) * 0.4;
        cc *= 1.0 + (f - 0.5) * uTex.x;
      }
      if (uTex.y > 0.0) {
        // brush strokes: long dabs following the cloud's curve
        float a = atan(gN.y, gN.x);
        vec2 bp = p * 60.0;
        float b = slFbm(vec2(bp.x * cos(a) + bp.y * sin(a), (-bp.x * sin(a) + bp.y * cos(a)) * 3.0));
        cc *= 1.0 + (b - 0.5) * uTex.y;
      }
    }
  }
  col = mix(col, cc, cov);
  // the haze at the horizon is the fog's own colour, so far hills melt into it
  vec3 fogC = slFogColorFor(d);
  col = mix(col, fogC, uGrad.z * (1.0 - smoothstep(-0.02, 0.2, h)));
  col *= uGrad.w;
  // the sun: a glow, an optional glare of streaks, the disc behind the clouds
  float glow = pow(max(cs, 0.0), 8.0) * 0.12 + pow(max(cs, 0.0), 80.0) * 0.35 + pow(max(cs, 0.0), 1200.0) * 1.5;
  col += uSunCol * glow * uSun.z * (1.0 - cov * 0.6);
  if (uSun.w > 0.0 && cs > 0.9) {
    vec3 sr = normalize(cross(S, vec3(0.0, 1.0, 0.0)) + 1e-5);
    vec3 su = cross(sr, S);
    vec2 sq = vec2(dot(d, sr), dot(d, su));
    float ang = atan(sq.y, sq.x);
    float streak = pow(abs(cos(ang * 4.0)), 60.0) + 0.5 * pow(abs(cos(ang * 4.0 + 0.4)), 120.0);
    col += uSunCol * streak * exp(-length(sq) * 22.0) * uSun.w * (1.0 - cov);
  }
  float disc = smoothstep(cos(uSun.x) - 0.00004, cos(uSun.x) + 0.00004, cs) * (1.0 - cov);
  col += uSunCol * disc * uSun.y;
  gl_FragColor = vec4(col, 1.0);
}`;

// cfg: { zenith, mid, midAt, power, horizon, ground, haze, brightness, sun: {size (deg), disc, glow, glare},
//        towers: {count, height, width, seed}, floaters: {amount, size, low, high (deg)},
//        cel: {edge, softness, wrap, rim}, cloud: {lit, shade, dark, rim}, felt, brush, warp, backdropShadow,
//        drift, bottomDark, sunBleed, edgeSoft }
export function makeToonSky(kit, env, cfg) {
  const THREE = kit.THREE;
  const op = env.toneMapping || 'aces', ex = env.exposure ?? 1;
  const inv = (hex) => inverseToneMap(kit.color(hex), op, ex);
  const sun = cfg.sun || {}, tw = cfg.towers || {}, fl = cfg.floaters || {}, cel = cfg.cel || {}, cl = cfg.cloud || {};
  const sunEnv = env.sun || {};
  const sunCol = kit.color(sunEnv.color || '#ffffff').multiplyScalar(Math.max(0.2, sunEnv.intensity ?? 3) * 0.3);
  const D = Math.PI / 180;
  const uniforms = Object.assign({
    uZenith: { value: inv(cfg.zenith || '#3d6fb0') }, uMid: { value: inv(cfg.mid || cfg.zenith || '#6a9ad0') },
    uHorizon: { value: inv(cfg.horizon || '#d8e6ee') }, uGround: { value: inv(cfg.ground || '#8a7a66') },
    uSunCol: { value: sunCol },
    uCloudLit: { value: inv(cl.lit || '#ffffff') }, uCloudShade: { value: inv(cl.shade || '#b8bcd8') },
    uCloudDark: { value: inv(cl.dark || cl.shade || '#9a9cc0') }, uCloudRim: { value: inv(cl.rim || '#ffffff') },
    uGrad: { value: new THREE.Vector4(cfg.midAt ?? 0.35, cfg.power ?? 0.6, cfg.haze ?? 0.6, cfg.brightness ?? 1) },
    uSun: { value: new THREE.Vector4((sun.size ?? 1) * 0.5 * D, sun.disc ?? 30, sun.glow ?? 1, sun.glare ?? 0) },
    uTow: { value: new THREE.Vector4(tw.count ?? 8, tw.height ?? 1, tw.width ?? 1, tw.seed ?? 0) },
    uPuff: { value: new THREE.Vector4(fl.amount ?? 0.3, fl.size ?? 0.09, (fl.low ?? 10) * D, (fl.high ?? 45) * D) },
    uShade: { value: new THREE.Vector4(cel.edge ?? 0.1, cel.softness ?? 0.02, cel.wrap ?? 0.3, cel.rim ?? 0.3) },
    uTex: { value: new THREE.Vector4(cfg.felt ?? 0, cfg.brush ?? 0, cfg.warp ?? 0, cfg.backdropShadow ?? 0) },
    uMisc: { value: new THREE.Vector4(cfg.drift ?? 0.002, cfg.bottomDark ?? 0.6, cfg.sunBleed ?? 0.3, cfg.edgeSoft ?? 0.03) },
    uMisc2: { value: new THREE.Vector4(cfg.topLight ?? 0.35, cfg.mass ?? 0.5, 0, 0) },
  }, kit.shared);
  return new THREE.ShaderMaterial({
    uniforms, vertexShader: VS, fragmentShader: FS, side: THREE.BackSide, depthWrite: false, depthTest: true,
    toneMapped: false, fog: false,
  });
}
