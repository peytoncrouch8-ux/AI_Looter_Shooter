// Teropa Pulp's sky: an airbrushed teal-to-tangerine dome with the ringed gas giant, two moons, low painted stratus
// streaks, the pink anti-sun band and faint daylight stars. A sky-sphere material: everything is analytic (no textures),
// so the Unreal version is one material on the sky sphere with the same maths.
//
// The giant is drawn in its own frame: x to the right of it on screen, y up, z toward the viewer, the disc of radius 1.
// Its bands follow the ring plane (the rings are equatorial); the sun lights it from a fixed direction in that frame
// (an art-directed light, so it is always a fat gibbous with the rings' shadow across it, whatever the shot), the
// rings take the planet's shadow and throw theirs onto it, and our own sky stays in front of it: its night side is the
// sky's colour, as the Moon's is by day.
import * as THREE from 'three';
import { inverseToneMap } from '../../engine/tonecurve.js';

const DEG = Math.PI / 180;

// The three.js direction of a UE azimuth/elevation (as the engine's dir3).
function dir3(az, el) {
  const a = az * DEG, p = el * DEG;
  return new THREE.Vector3(-Math.sin(a) * Math.cos(p), Math.sin(p), Math.cos(a) * Math.cos(p));
}

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
uniform vec3 uSlFogColor, uSlFogSunColor;
uniform vec4 uSlFogSun;
uniform float uCapture;
uniform vec3 uZen, uMid, uLow, uHor, uGnd, uBelt, uSunCol, uStreakLit, uStreakShade;
uniform float uHaze, uStars, uStreaks;
// the giant
uniform vec3 uPDir, uPRight, uPUp;
uniform float uPTan, uPBright, uPVeil;
uniform vec3 uPL, uRingN;
uniform vec4 uRing;            // inner, outer, opacity, brightness
uniform vec3 uB0, uB1, uB2, uB3, uB4, uB5;   // cream, peach, ochre, rust, deep sienna, teal
uniform vec3 uRingA, uRingB, uRingC;
// moons
uniform vec3 uMDir[2], uMCol[2], uMCol2[2];
uniform float uMTan[2];

float h12(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
float h13(vec3 p3) { p3 = fract(p3 * 0.1031); p3 += dot(p3, p3.zyx + 31.32); return fract((p3.x + p3.y) * p3.z); }
float vn(vec2 p) { vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
  return mix(mix(h12(i), h12(i + vec2(1, 0)), f.x), mix(h12(i + vec2(0, 1)), h12(i + vec2(1, 1)), f.x), f.y); }
float fbm(vec2 p) { float v = 0.0, a = 0.5; for (int i = 0; i < 5; i++) { v += a * vn(p); p = mat2(1.6, 1.2, -1.2, 1.6) * p + 7.3; a *= 0.5; } return v; }
float fbm3(vec2 p) { float v = 0.0, a = 0.5; for (int i = 0; i < 3; i++) { v += a * vn(p); p = mat2(1.6, 1.2, -1.2, 1.6) * p + 3.1; a *= 0.5; } return v; }

vec3 fogFor(vec3 d) {
  float s = pow(max(dot(d, uSlSunDirW), 0.0), uSlFogSun.y);
  return mix(uSlFogColor, uSlFogSunColor, clamp(s * uSlFogSun.x, 0.0, 1.0));
}

// The ring's density and colour at radius rr (planet radii).
vec4 ringAt(float rr) {
  float x = (rr - uRing.x) / (uRing.y - uRing.x);
  if (x <= 0.0 || x >= 1.0) return vec4(0.0);
  // broad structure first (a faint inner ring, the bright middle ring, the great gap, the outer ring), fine ringlets
  // only as a whisper on top so it reads at a glance
  float d = mix(0.28, 0.95, smoothstep(0.16, 0.24, x));
  d *= mix(1.0, 0.72, smoothstep(0.62, 0.68, x));
  d *= 1.0 - 0.95 * (smoothstep(0.575, 0.59, x) * (1.0 - smoothstep(0.64, 0.655, x)));
  d *= 1.0 - 0.75 * (smoothstep(0.88, 0.886, x) * (1.0 - smoothstep(0.896, 0.902, x)));
  d *= 0.82 + 0.18 * sin(x * 150.0 + 3.0 * sin(x * 23.0)) * h12(vec2(floor(x * 45.0), 3.0));
  d *= 0.9 + 0.1 * sin(x * 18.0);
  d *= smoothstep(0.0, 0.03, x) * (1.0 - smoothstep(0.96, 1.0, x));
  vec3 c = mix(uRingC, uRingA, smoothstep(0.1, 0.3, x));
  c = mix(c, uRingB, smoothstep(0.66, 0.98, x) * 0.8);
  c *= 0.9 + 0.2 * h12(vec2(floor(x * 60.0), 7.0));
  return vec4(c, clamp(d, 0.0, 1.0));
}

// The giant's surface colour at unit-sphere point S (local frame).
vec3 bandsAt(vec3 S) {
  vec3 rx = vec3(1.0, 0.0, 0.0);
  vec3 rz = normalize(cross(rx, uRingN));
  float lat = dot(S, uRingN);
  float lon = atan(dot(S, rz), dot(S, rx));
  float drift = uSlTime * 0.004;
  // wavy band edges: flow warped by low-frequency turbulence, stronger near the belts
  vec2 fp = vec2(lon * 2.0 + drift, lat * 8.0);
  float w = fbm(fp + vec2(fbm(fp * 1.7), 0.0) * 1.1) - 0.5;
  float z = lat + w * 0.05 + 0.008 * sin(lon * 9.0 + lat * 40.0);
  float belts = 0.5 + 0.5 * sin(z * 26.0 + 0.9 * sin(z * 7.0) + 0.6);
  float fine = 0.5 + 0.5 * sin(z * 70.0 + fbm3(vec2(lon * 5.0, z * 30.0)) * 3.0);
  vec3 c = mix(uB0, uB1, smoothstep(0.2, 0.55, fine) * 0.4);
  c = mix(c, uB3, smoothstep(0.42, 0.62, belts));
  c = mix(c, uB2, smoothstep(0.55, 0.8, fine) * (1.0 - smoothstep(0.4, 0.8, belts)) * 0.6);
  c = mix(c, uB4, smoothstep(0.86, 0.96, belts) * 0.85);
  // a cold teal belt south of the equator and the teal poles
  c = mix(c, uB5, exp(-pow((z + 0.24) / 0.035, 2.0)) * 0.85);
  c = mix(c, uB5 * 0.8, smoothstep(0.62, 0.9, abs(z)));
  // the great storm: a cream oval with a rust collar
  vec2 so = vec2((lon - 0.55) * 2.4, (z - 0.28) * 9.0);
  float sw = length(so);
  float ang = atan(so.y, so.x) + sw * 2.0;
  float swirl = 0.5 + 0.5 * sin(ang * 3.0 + sw * 6.0);
  c = mix(c, uB3 * 1.05, smoothstep(1.3, 0.95, sw) * 0.85);
  c = mix(c, mix(uB0, uB1, swirl * 0.6), smoothstep(0.85, 0.45, sw));
  return c;
}

void main() {
  vec3 d = normalize(vDir);
  float h = d.y;
  float el = asin(clamp(h, -1.0, 1.0));
  vec2 sxz = normalize(uSlSunDirW.xz + 1e-5);
  float anti = 0.5 - 0.5 * dot(normalize(d.xz + 1e-5), sxz);   // 1 opposite the sun

  // ---- the airbrushed dome: tangerine horizon, peach, turquoise, deep teal
  float up = max(h, 0.0);
  vec3 col = mix(uHor, uLow, smoothstep(0.0, 0.07, up));
  col = mix(col, uMid, smoothstep(0.05, 0.32, up));
  col = mix(col, uZen, smoothstep(0.28, 0.95, up));
  // the pink band over the horizon opposite the sun
  col = mix(col, uBelt, exp(-pow((el - 0.07) / 0.055, 2.0)) * anti * 0.75);
  // the sun's side glows
  float cs = dot(d, uSlSunDirW);
  col += uSunCol * (pow(max(cs, 0.0), 5.0) * 0.25 + pow(max(cs, 0.0), 40.0) * 0.5) * (1.0 - uCapture * 0.5);
  col = mix(col, uGnd, smoothstep(0.0, -0.06, h));

  // ---- daylight stars, only high up and away from the sun
  if (uStars > 0.0 && h > 0.35) {
    vec3 sp = d * 300.0; vec3 cell = floor(sp); float hs = h13(cell);
    if (hs > 0.988) {
      vec3 cc = cell + vec3(h13(cell + 1.7), h13(cell + 3.1), h13(cell + 5.3));
      float s = smoothstep(0.12, 0.0, length(sp - cc)) * (hs - 0.988) / 0.012;
      col += vec3(0.9, 0.97, 1.0) * s * uStars * smoothstep(0.35, 0.8, h) * (1.0 - smoothstep(0.2, 0.7, cs)) * 0.6;
    }
  }

  vec3 skyCol = col;

  // ---- the gas giant and its rings
  float cb = dot(d, uPDir);
  if (cb > 0.0) {
    vec2 q = vec2(dot(d, uPRight), dot(d, uPUp)) / (cb * uPTan);
    float r2 = dot(q, q);
    float r = sqrt(r2);
    float aa = fwidth(r) * 1.2 + 1e-4;
    float disc = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, r);
    float zs = sqrt(max(1.0 - r2, 0.0));
    vec3 S = vec3(q, zs);
    // the ring plane under this pixel (an orthographic view: the giant is far away)
    float zr = -(q.x * uRingN.x + q.y * uRingN.y) / uRingN.z;
    vec3 P = vec3(q, zr);
    float rr = length(P);
    vec4 ring = ringAt(rr);
    if (ring.a > 0.0) {
      // the planet's shadow on the ring
      float pl = dot(P, uPL);
      float inShadow = (pl < 0.0 && length(P - pl * uPL) < 1.0) ? 1.0 : 0.0;
      float litFace = step(0.0, dot(uRingN, uPL)) == step(0.0, uRingN.z) ? 1.0 : 0.62;
      ring.rgb *= uRing.w * litFace * mix(1.0, 0.06, inShadow);
      ring.a *= uRing.z;
    }
    // the ring shows beside the disc, and over it only on its near side
    bool ringShown = ring.a > 0.0 && (r >= 1.0 || zr > zs);
    float lam = dot(S, uPL);
    if (disc > 0.0) {
      vec3 surf = bandsAt(S);
      // soft gas terminator, limb darkening, the lit limb's haze
      float day = smoothstep(-0.1, 0.3, lam);
      float shade = day * (0.3 + 0.7 * max(lam, 0.0));
      shade *= mix(0.42, 1.0, pow(zs, 0.5));
      // the rings' shadow across the disc
      float tRing = -dot(S, uRingN) / dot(uPL, uRingN);
      if (tRing > 0.0) {
        vec4 rs = ringAt(length(S + tRing * uPL));
        shade *= 1.0 - rs.a * uRing.z * 0.9;
      }
      // a moon's shadow in transit, and the moon beside it
      vec2 tq = vec2(0.3, 0.3);
      float tr = length((q - tq) * vec2(1.0, 1.1));
      shade *= mix(0.12, 1.0, smoothstep(0.024, 0.03, tr));
      vec3 pc = surf * shade * uPBright;
      float rim = pow(1.0 - zs, 2.5) * smoothstep(-0.1, 0.4, lam);
      pc += mix(uB0, uB5, 0.35) * rim * 0.9 * uPBright;
      // ringshine on the night side
      pc += surf * 0.035 * (1.0 - day) * uPBright;
      // our sky in front of it: the night side takes the sky's colour, the day side adds to it
      vec3 seen = skyCol * uPVeil * (1.0 - 0.65 * day) + pc;
      col = mix(col, seen, disc);
    }
    if (ringShown) {
      // over the sky the rings add to it (they are behind our air too); over the disc they hide it
      float occ = mix(0.65, 0.9, disc);
      col = col * (1.0 - ring.a * occ) + ring.rgb * ring.a;
    }
    // a faint glow round the giant
    col += mix(uB1, uB5, 0.4) * exp(-max(r - 1.0, 0.0) * 6.0) * (1.0 - disc) * 0.12 * uPBright;
  }

  // ---- two moons, lit by the real sun (crescents and gibbous as the sun goes)
  for (int i = 0; i < 2; i++) {
    float mc = dot(d, uMDir[i]);
    if (mc <= 0.0) continue;
    vec3 mb = uMDir[i];
    vec3 me = normalize(cross(mb, vec3(0.0, 1.0, 0.0)));
    vec3 mn = cross(me, mb);
    vec2 q = vec2(dot(d, me), dot(d, mn)) / (mc * uMTan[i]);
    float r2 = dot(q, q);
    if (r2 > 1.6) continue;
    float r = sqrt(r2);
    float aa = fwidth(r) * 1.2 + 1e-4;
    float disc = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, r);
    float z = sqrt(max(1.0 - r2, 0.0));
    vec3 Nw = normalize(me * q.x + mn * q.y - mb * z);
    float lit = dot(Nw, uSlSunDirW);
    float cr = fbm(q * 2.6 + float(i) * 9.0), cr2 = fbm(q * 7.0 + 3.0);
    vec3 surf = mix(uMCol2[i], uMCol[i], smoothstep(0.38, 0.62, cr)) * (0.85 + 0.3 * cr2);
    vec3 mcol = surf * smoothstep(-0.05, 0.25, lit) * (0.4 + 0.6 * max(lit, 0.0)) * uPBright * 1.15;
    col = mix(col, skyCol * uPVeil + mcol, disc);
  }

  // ---- low painted stratus streaks, in front of everything in the sky
  if (uStreaks > 0.0 && h > -0.02 && h < 0.4) {
    float az = atan(d.x, d.z);
    float s = fbm(vec2(az * 3.2 + uSlTime * 0.002, el * 34.0)) + 0.35 * fbm(vec2(az * 11.0, el * 120.0)) - 0.17;
    float band = smoothstep(0.01, 0.05, el) * (1.0 - smoothstep(0.14, 0.3, el));
    float k = smoothstep(0.5, 0.78, s) * band * uStreaks;
    float sunward = pow(max(cs, 0.0), 2.0);
    vec3 sc = mix(uStreakShade, uStreakLit, 0.45 + 0.55 * smoothstep(0.55, 0.85, s));
    sc += uSunCol * sunward * 0.35;
    col = mix(col, sc, k * 0.8);
  }

  // ---- the horizon melts into the haze (the giant's foot too: it is far beyond the ridges)
  vec3 fogC = fogFor(d);
  col = mix(col, fogC, uHaze * (1.0 - smoothstep(-0.02, 0.16, h)));

  // ---- the sun
  float sunR = 0.012;
  float sAA = fwidth(cs) * 2.0 + 1e-6;
  float sdisc = smoothstep(cos(sunR) - sAA, cos(sunR) + sAA, cs) * (1.0 - uCapture);
  col = mix(col, uSunCol * 12.0, sdisc);

  // airbrush tooth: a whisper of spatter
  col *= 1.0 + (h12(gl_FragCoord.xy + floor(uSlTime * 0.0)) - 0.5) * 0.025;
  gl_FragColor = vec4(max(col, 0.0), 1.0);
}`;

// cfg: colours as they should look on screen ('#hex'), the giant ({ azimuth, elevation, size (deg across), roll,
// ringTilt, ring: [inner, outer], light: [x, y, z] in its frame }), moons ([{ azimuth, elevation, size, color, color2 }]).
export function pulpSky(kit, cfg) {
  const env = kit.env || {};
  const op = env.toneMapping || 'aces', ex = env.exposure ?? 1;
  const inv = (hex) => inverseToneMap(new THREE.Color(hex), op, ex);
  const lin = (hex) => new THREE.Color(hex);
  const p = cfg.planet;
  const b = dir3(p.azimuth, p.elevation);
  const right0 = new THREE.Vector3().crossVectors(b, new THREE.Vector3(0, 1, 0)).normalize();
  const up0 = new THREE.Vector3().crossVectors(right0, b).normalize();
  const roll = (p.roll || 0) * DEG;
  const right = right0.clone().multiplyScalar(Math.cos(roll)).addScaledVector(up0, Math.sin(roll));
  const up = up0.clone().multiplyScalar(Math.cos(roll)).addScaledVector(right0, -Math.sin(roll));
  const tilt = (p.ringTilt ?? 16) * DEG;
  // positive tilt: we see the rings' lit top face (the near arc below the centre); negative: their underside, the near
  // arc crossing the upper disc
  const ringN = new THREE.Vector3(0, Math.cos(tilt), Math.sin(tilt));
  if (ringN.z < 0) ringN.negate();
  const L = new THREE.Vector3(...(p.light || [-0.5, 0.45, 0.74])).normalize();
  const moons = (cfg.moons || []).slice(0, 2);
  while (moons.length < 2) moons.push({ azimuth: 0, elevation: -90, size: 0.01, color: '#000000' });
  const sunCol = kit.sun.sunColor.clone().multiplyScalar(Math.max(0.2, kit.sun.sunIntensity) * 0.3);
  const uniforms = Object.assign({}, kit.shared, {
    uCapture: kit.sky.uniforms.uCapture,
    uZen: { value: inv(cfg.zenith) }, uMid: { value: inv(cfg.mid) }, uLow: { value: inv(cfg.low) }, uHor: { value: inv(cfg.horizon) },
    uGnd: { value: inv(cfg.ground) }, uBelt: { value: inv(cfg.belt) }, uSunCol: { value: sunCol },
    uStreakLit: { value: inv(cfg.streakLit) }, uStreakShade: { value: inv(cfg.streakShade) },
    uHaze: { value: cfg.haze ?? 0.8 }, uStars: { value: cfg.stars ?? 0.5 }, uStreaks: { value: cfg.streaks ?? 1 },
    uPDir: { value: b }, uPRight: { value: right }, uPUp: { value: up },
    uPTan: { value: Math.tan((p.size || 36) * 0.5 * DEG) }, uPBright: { value: p.brightness ?? 1 }, uPVeil: { value: p.veil ?? 0.6 },
    uPL: { value: L }, uRingN: { value: ringN },
    uRing: { value: new THREE.Vector4(p.ring ? p.ring[0] : 1.35, p.ring ? p.ring[1] : 2.3, p.ringOpacity ?? 0.9, p.ringBrightness ?? 1) },
    uB0: { value: lin(p.bands[0]) }, uB1: { value: lin(p.bands[1]) }, uB2: { value: lin(p.bands[2]) },
    uB3: { value: lin(p.bands[3]) }, uB4: { value: lin(p.bands[4]) }, uB5: { value: lin(p.bands[5]) },
    uRingA: { value: lin(p.ringColors[0]) }, uRingB: { value: lin(p.ringColors[1]) }, uRingC: { value: lin(p.ringColors[2]) },
    uMDir: { value: moons.map((m) => dir3(m.azimuth, m.elevation)) },
    uMCol: { value: moons.map((m) => lin(m.color)) },
    uMCol2: { value: moons.map((m) => lin(m.color2 || m.color).multiplyScalar(m.color2 ? 1 : 0.6)) },
    uMTan: { value: moons.map((m) => Math.tan((m.size || 4) * 0.5 * DEG)) },
  });
  const mat = new THREE.ShaderMaterial({
    uniforms, vertexShader: VS, fragmentShader: FS, side: THREE.BackSide, depthWrite: false, depthTest: true,
    toneMapped: false, fog: false,
  });
  mat.name = 'PulpSky';
  return mat;
}
