// Sunbleached's sky: bleached white-gold at the horizon to a pale turquoise overhead, a huge soft sun glow with a
// white-hot disc, the 22-degree ice halo round the sun (warm inside, cool outside: the mythic sign in the sky), thin
// sheared cirrus lit gold toward the sun, and a lavender band low on the horizon opposite it. Colours are given as
// they should look on screen and inverse tone mapped here; the sun and its glow are scene-referred so they bloom.
import * as THREE from 'three';
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
uniform vec3 uZen, uMid, uHor, uGround, uAnti, uCirrus, uCirrusShade;
uniform vec3 uSunC, uGlowC, uHaloC;
uniform float uHalo, uCirrusAmt, uHorizonFog, uGlowK;
${NOISE}
${FOG}
void main() {
  vec3 d = normalize(vDir);
  float h = d.y;
  vec3 L = uSlSunDirW;
  float cs = dot(d, L);
  float up = max(h, 0.0);
  vec3 col = mix(uHor, uMid, smoothstep(0.0, 0.22, up));
  col = mix(col, uZen, smoothstep(0.18, 0.8, up));
  // opposite the sun, a lavender band low on the horizon
  vec2 dh = normalize(d.xz + 1e-5), lh = normalize(L.xz + 1e-5);
  float away = pow(max(-dot(dh, lh) * 0.5 + 0.5, 0.0), 2.0);
  col = mix(col, uAnti, away * (1.0 - smoothstep(0.0, 0.2, up)) * 0.75);
  col = mix(col, uGround, smoothstep(0.0, -0.06, h));
  vec3 fogC = slFogColorFor(d);
  col = mix(col, fogC, uHorizonFog * (1.0 - smoothstep(-0.02, 0.1, h)));
  // cirrus: long sheared wisps on a high layer, lit gold toward the sun
  if (h > 0.0 && uCirrusAmt > 0.0) {
    vec2 p = d.xz / (h + 0.12);
    p = mat2(0.8, 0.6, -0.6, 0.8) * p;
    vec2 drift = vec2(uSlTime * 0.004, 0.0);
    float n = slFbm(vec2(p.x * 0.35, p.y * 2.6) + drift);
    float n2 = slFbm(vec2(p.x * 1.3, p.y * 7.0) + 3.0 + drift * 2.0);
    float dens = smoothstep(0.52, 0.86, n * 0.72 + n2 * 0.42) * smoothstep(0.03, 0.22, h) * uCirrusAmt;
    vec3 cc = mix(uCirrusShade, uCirrus, smoothstep(0.3, 0.8, n2));
    cc += uGlowC * pow(max(cs, 0.0), 6.0) * 0.25;
    col = mix(col, cc, dens);
  }
  // the sun's glow: a huge soft bloom, a brighter middle, a tight core. The angle to the sun comes from the cross
  // product near it (acos of a dot product loses the small angles to float precision on some GPUs)
  float sinA = length(cross(d, L));
  float ang = cs > 0.7 ? asin(min(sinA, 1.0)) : acos(clamp(cs, -1.0, 1.0));
  col += uGlowC * uGlowK * (exp(-ang * 2.4) * 0.06 + exp(-ang * 10.0) * 0.16 + exp(-ang * 50.0) * 1.2);
  // the 22-degree halo: a thin ring, reddish on its inner edge, bluish outside, fading toward the ground
  float hr = (ang - 0.3840) / 0.011;
  float ring = exp(-hr * hr) + 0.3 * exp(-hr * hr / 9.0);
  vec3 rc = mix(vec3(1.0, 0.72, 0.5), vec3(0.78, 0.88, 1.0), smoothstep(-1.2, 2.0, hr));
  col += uHaloC * rc * ring * uHalo * smoothstep(-0.04, 0.12, h);
  // the disc, white-hot
  float disc = (1.0 - smoothstep(0.012, 0.0145, ang)) * step(0.0, cs);
  col += uSunC * disc;
  gl_FragColor = vec4(col, 1.0);
}`;

export function bleachedSky(kit, o) {
  const op = o.toneMapping || 'aces', ex = o.exposure ?? 1;
  const inv = (hex) => inverseToneMap(new THREE.Color(hex), op, ex);
  const lin = (hex, k) => new THREE.Color(hex).multiplyScalar(k);
  const uniforms = Object.assign({
    uZen: { value: inv(o.zenith) }, uMid: { value: inv(o.mid) }, uHor: { value: inv(o.horizon) },
    uGround: { value: inv(o.ground) }, uAnti: { value: inv(o.anti) },
    uCirrus: { value: inv(o.cirrus) }, uCirrusShade: { value: inv(o.cirrusShade) },
    uSunC: { value: lin(o.sun, o.sunBright) }, uGlowC: { value: lin(o.glow, 1) }, uHaloC: { value: lin(o.halo, 1) },
    uHalo: { value: o.haloAmount }, uCirrusAmt: { value: o.cirrusAmount }, uHorizonFog: { value: o.horizonFog },
    uGlowK: { value: o.glowAmount },
  }, kit.shared);
  return new THREE.ShaderMaterial({
    uniforms, vertexShader: VS, fragmentShader: FS, side: THREE.BackSide, depthWrite: false, depthTest: true,
    toneMapped: false, fog: false,
  });
}
