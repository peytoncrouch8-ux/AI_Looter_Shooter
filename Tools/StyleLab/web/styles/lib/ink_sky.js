// Inkslinger's sky: a comic sky as a colourist paints it and an inker finishes it. A deep teal zenith over turquoise
// down to a hot ochre horizon, big cumulus banks lit cream on the sun's side, their shade hatched in ink (strokes fixed
// to the sky, not the screen) with broken contour strokes along the shaded undersides only, and a white sun ringed in
// ink. No outlines round the lit tops: the clouds are drawn, not cut out.
import * as THREE from 'three';
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
uniform vec3 uSlSunDirW;
uniform vec3 uSlFogColor, uSlFogSunColor;
uniform vec4 uSlFogSun;
uniform float uCapture;
uniform vec3 uZen, uMid, uLow, uHor, uGnd, uSun, uCloudLit, uCloudShade, uCloudDeep, uInk;
uniform float uHaze, uHatchDeg;

float h12(vec2 p) { vec3 p3 = fract(vec3(p.xyx) * 0.1031); p3 += dot(p3, p3.yzx + 33.33); return fract((p3.x + p3.y) * p3.z); }
float vn(vec2 p) { vec2 i = floor(p), f = fract(p); f = f * f * (3.0 - 2.0 * f);
  return mix(mix(h12(i), h12(i + vec2(1, 0)), f.x), mix(h12(i + vec2(0, 1)), h12(i + vec2(1, 1)), f.x), f.y); }
float fbm(vec2 p) { float v = 0.0, a = 0.5; for (int i = 0; i < 5; i++) { v += a * vn(p); p = mat2(1.6, 1.2, -1.2, 1.6) * p + 7.3; a *= 0.5; } return v; }

vec3 fogFor(vec3 d) {
  float s = pow(max(dot(d, uSlSunDirW), 0.0), uSlFogSun.y);
  return mix(uSlFogColor, uSlFogSunColor, clamp(s * uSlFogSun.x, 0.0, 1.0));
}

// Billowy noise: rounded bumps, the shape of cumulus.
float billow(vec2 p) { float v = 0.0, a = 0.5; for (int i = 0; i < 3; i++) { v += a * abs(vn(p) * 2.0 - 1.0); p = mat2(1.6, 1.2, -1.2, 1.6) * p * 1.05 + 5.1; a *= 0.5; } return v; }

// Cumulus banks standing on the horizon, in (azimuth, elevation) radians: > 0 inside, about the angular depth below
// the cloud's top edge. Towers of different heights with gaps between them.
float bankShape(vec2 ae) {
  float tower = fbm(vec2(ae.x * 1.5 + 2.0, 1.3));
  float top = 0.04 + 0.62 * tower * tower * tower;
  float there = smoothstep(0.34, 0.5, fbm(vec2(ae.x * 2.0 + 9.0, 4.2)));
  top = mix(-0.04, top, there);
  // the hero towers, composed like a panel: one over the end of Main Street (east) and one beside the chapel seen
  // from boot hill (north-west, where the knoll leaves the sky open); UE yaw y sits at azimuth -y here
  float dE = atan(sin(ae.x + 1.48), cos(ae.x + 1.48)), dN = atan(sin(ae.x - 0.8), cos(ae.x - 0.8));
  top = max(top, 0.34 * exp(-dE * dE / 0.03) - 0.02);
  top = max(top, 0.42 * exp(-dN * dN / 0.03) - 0.02);
  // billows grow with the tower: big round heads up top, small ones low down
  float b = billow(vec2(ae.x * 9.0, ae.y * 10.0) + 3.0);
  return top + (b - 0.32) * (0.07 + 0.12 * smoothstep(0.1, 0.35, top)) - ae.y;
}

// A sky-fixed pen stroke family across direction ang; sdx/sdy are sp's screen derivatives (taken outside any branch).
float strokes(vec2 sp, vec2 sdx, vec2 sdy, float ang, float width) {
  vec2 dd = vec2(cos(ang), sin(ang));
  vec2 nn = vec2(-dd.y, dd.x);
  float v = dot(sp, nn);
  float a = dot(sp, dd);
  float id = floor(v);
  v += (vn(vec2(a * 0.2, id * 1.3)) - 0.5) * 0.45;
  float f = abs(fract(v) - 0.5) * 2.0;
  float brk = smoothstep(0.2, 0.42, vn(vec2(a * 0.08 + id * 2.7, id)));
  float aa = (abs(dot(sdx, nn)) + abs(dot(sdy, nn))) * 0.9 + 1e-3;
  float wd = width * brk;
  return 1.0 - smoothstep(wd - aa, wd + aa, f);
}

// A cloud drawn the comic way: lit, mid and deep bands, hatching in the mid (one way) and deep (crossed), broken pen
// strokes along the shaded outline and round the lit billows. s0: inside > 0; s1: the same toward the light; g0, g1
// their screen derivatives.
vec3 inkCloud(vec3 col, float s0, float s1, float g0, float g1, float litW, float midW, float cs, vec2 sp, vec2 sdx, vec2 sdy, float farK) {
  float band = s1 < litW ? 1.0 : (s1 < midW ? 0.5 : 0.0);
  vec3 cc = band > 0.75 ? uCloudLit : (band > 0.25 ? uCloudShade : uCloudDeep);
  cc += uSun * pow(max(cs, 0.0), 6.0) * 0.3 * band;
  float dens = smoothstep(-0.002, 0.002, s0);
  float hatch = 0.0;
  if (band < 0.75) {
    hatch = strokes(sp, sdx, sdy, 0.85, band > 0.25 ? 0.16 : 0.26);
    if (band < 0.25) hatch = max(hatch, strokes(sp + 0.5, sdx, sdy, -0.5, 0.18));
  }
  float pen = smoothstep(0.3, 0.6, vn(sp * 0.18 + 2.0));
  float outline = (1.0 - smoothstep(0.0, g0 * 1.4 + 1e-5, abs(s0))) * (1.0 - band * 0.8) * pen;
  float billowLine = (1.0 - smoothstep(0.0, g1 * 1.2 + 1e-5, abs(s1 - litW))) * 0.8 * pen * smoothstep(0.015, 0.035, s0);
  cc = mix(cc, uInk, clamp(max(max(hatch * 0.85, outline), billowLine) * farK, 0.0, 1.0));
  return mix(col, cc, dens);
}

void main() {
  vec3 d = normalize(vDir);
  float h = d.y;
  float up = max(h, 0.0);
  vec3 col = mix(uHor, uLow, smoothstep(0.0, 0.035, up));
  col = mix(col, uMid, smoothstep(0.03, 0.17, up));
  col = mix(col, uZen, smoothstep(0.2, 0.75, up));
  col = mix(col, uGnd, smoothstep(0.0, -0.06, h));
  float cs = dot(d, uSlSunDirW);
  col += uSun * pow(max(cs, 0.0), 8.0) * 0.12;

  // sky-fixed stroke space: azimuth and elevation in degrees over the stroke spacing
  float az = atan(d.x, d.z) * 57.2958;
  float el = asin(clamp(h, -1.0, 1.0)) * 57.2958;
  vec2 sp = vec2(az * cos(radians(min(el, 70.0))), el) / uHatchDeg;

  // clouds: towering cumulus banks on the horizon, three-band light, inked in the shade. The shapes and their
  // derivatives are taken for every sky pixel, outside any branch.
  {
    vec2 sdx = dFdx(sp), sdy = dFdy(sp);
    vec2 ae = vec2(radians(az), radians(el));
    float s0 = bankShape(ae);
    // the light: toward the sun's side and from above; a point is lit when its neighbour toward the light is outside
    float sunAz = atan(uSlSunDirW.x, uSlSunDirW.z);
    float dAz = atan(sin(sunAz - ae.x), cos(sunAz - ae.x));
    vec2 Ld = normalize(vec2(clamp(dAz, -1.0, 1.0) * 0.8, 1.0));
    float inner = (billow(vec2(ae.x * 24.0, ae.y * 30.0) + 11.0) - 0.3) * 0.02;
    float s1 = bankShape(ae + Ld * 0.022) + inner;
    float g0 = fwidth(s0), g1 = fwidth(s1);
    // clouds opposite the sun face it: mostly lit, only the deep folds shaded
    float front = 0.5 - 0.5 * cos(dAz);
    float litW = 0.03 + 0.05 * front, midW = 0.09 + 0.05 * front;
    if (s0 > -0.02 && h > -0.03) col = inkCloud(col, s0, s1, g0, g1, litW, midW, cs, sp, sdx, sdy, smoothstep(0.0, 0.05, h));
  }

  vec3 fogC = fogFor(d);
  col = mix(col, fogC, uHaze * (1.0 - smoothstep(-0.02, 0.14, h)));

  // the sun: a white disc with an ink ring and a few ray strokes
  float ang = acos(clamp(cs, -1.0, 1.0));
  float sr = 0.035;
  float aa = fwidth(ang) * 1.2 + 1e-5;
  float disc = 1.0 - smoothstep(sr - aa, sr + aa, ang);
  float ring = (1.0 - smoothstep(sr + 0.004 - aa, sr + 0.004 + aa, ang)) - disc;
  col = mix(col, uInk, ring * (1.0 - uCapture));
  col = mix(col, uSun * 6.0, disc * (1.0 - uCapture));
  gl_FragColor = vec4(max(col, 0.0), 1.0);
}`;

export function inkSky(kit, cfg) {
  const env = kit.env || {};
  const op = env.toneMapping || 'aces', ex = env.exposure ?? 1;
  const inv = (hex) => inverseToneMap(new THREE.Color(hex), op, ex);
  const uniforms = Object.assign({}, kit.shared, {
    uCapture: kit.sky.uniforms.uCapture,
    uZen: { value: inv(cfg.zenith) }, uMid: { value: inv(cfg.mid) }, uLow: { value: inv(cfg.low) }, uHor: { value: inv(cfg.horizon) },
    uGnd: { value: inv(cfg.ground) }, uSun: { value: kit.sun.sunColor.clone().multiplyScalar(Math.max(0.2, kit.sun.sunIntensity) * 0.3) },
    uCloudLit: { value: inv(cfg.cloudLit) }, uCloudShade: { value: inv(cfg.cloudShade) }, uCloudDeep: { value: inv(cfg.cloudDeep) },
    uInk: { value: new THREE.Color(cfg.ink) },
    uHaze: { value: cfg.haze ?? 0.7 }, uHatchDeg: { value: cfg.hatchDeg ?? 0.3 },
  });
  const mat = new THREE.ShaderMaterial({
    uniforms, vertexShader: VS, fragmentShader: FS, side: THREE.BackSide, depthWrite: false, depthTest: true,
    toneMapped: false, fog: false,
  });
  mat.name = 'InkSky';
  return mat;
}
