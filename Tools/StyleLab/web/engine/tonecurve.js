// The tone curves in JavaScript, and their inverse: the sky's and the fog's colours are given as they should look on
// screen, so the engine feeds the renderer the scene-referred value that the style's tone mapping and exposure turn
// into that colour.
import * as THREE from 'three';

const mul = (m, v) => [m[0] * v[0] + m[3] * v[1] + m[6] * v[2], m[1] * v[0] + m[4] * v[1] + m[7] * v[2], m[2] * v[0] + m[5] * v[1] + m[8] * v[2]];
const ACES_IN = [0.59719, 0.07600, 0.02840, 0.35458, 0.90834, 0.13383, 0.04823, 0.01566, 0.83777];
const ACES_OUT = [1.60475, -0.10208, -0.00327, -0.53108, 1.10813, -0.07276, -0.07367, -0.00605, 1.07602];
const fit = (v) => (v * (v + 0.0245786) - 0.000090537) / (v * (0.983729 * v + 0.4329510) + 0.238081);
const sat = (x) => Math.min(1, Math.max(0, x));

function aces(c) { let v = mul(ACES_IN, c.map((x) => x / 0.6)); v = v.map(fit); return mul(ACES_OUT, v).map(sat); }
const S2R = [0.6274, 0.0691, 0.0164, 0.3293, 0.9195, 0.0880, 0.0433, 0.0113, 0.8956];
const R2S = [1.6605, -0.1246, -0.0182, -0.5876, 1.1329, -0.1006, -0.0728, -0.0083, 1.1187];
const INS = [0.856627153315983, 0.137318972929847, 0.11189821299995, 0.0951212405381588, 0.761241990602591, 0.0767994186031903, 0.0482516061458583, 0.101439036467562, 0.811302368396859];
const OUTS = [1.1271005818144368, -0.1413297634984383, -0.14132976349843826, -0.11060664309660323, 1.157823702216272, -0.11060664309660294, -0.016493938717834573, -0.016493938717834257, 1.2519364065950405];
const agxC = (x) => { const x2 = x * x, x4 = x2 * x2; return 15.5 * x4 * x2 - 40.14 * x4 * x + 31.96 * x4 - 6.868 * x2 * x + 0.4298 * x2 + 0.1191 * x - 0.00232; };
function agx(c) {
  let v = mul(INS, mul(S2R, c));
  v = v.map((x) => sat((Math.log2(Math.max(x, 1e-10)) + 12.47393) / (4.026069 + 12.47393)));
  v = mul(OUTS, v.map(agxC)).map((x) => Math.pow(Math.max(0, x), 2.2));
  return mul(R2S, v).map(sat);
}
function neutral(c) {
  const S = 0.76, D = 0.15;
  const x = Math.min(...c), off = x < 0.08 ? x - 6.25 * x * x : 0.04;
  let v = c.map((y) => y - off);
  const peak = Math.max(...v);
  if (peak < S) return v;
  const d = 1 - S, np = 1 - d * d / (peak + d - S);
  v = v.map((y) => y * np / peak);
  const g = 1 - 1 / (D * (peak - np) + 1);
  return v.map((y) => y + (np - y) * g);
}
export function toneMap(c, op) {
  switch (op) {
    case 'aces': return aces(c);
    case 'agx': return agx(c);
    case 'neutral': return neutral(c);
    default: return c.map(sat);
  }
}

// The linear colour that, times exposure and tone mapped, displays as `display` (a linear-sRGB THREE.Color).
export function inverseToneMap(display, op, exposure = 1) {
  const target = [display.r, display.g, display.b].map((x) => Math.min(x, 0.985));
  if (op === 'none' || !op) return new THREE.Color(target[0] / exposure, target[1] / exposure, target[2] / exposure);
  let x = target.slice();
  for (let it = 0; it < 60; it++) {
    const y = toneMap(x, op);
    let err = 0;
    for (let i = 0; i < 3; i++) {
      const e = target[i] - y[i];
      err += Math.abs(e);
      // multiplicative steps converge well across the curve's toe and shoulder
      x[i] = Math.max(1e-5, x[i] * Math.pow((target[i] + 1e-4) / (y[i] + 1e-4), 0.85));
    }
    if (err < 1e-4) break;
  }
  return new THREE.Color(x[0] / exposure, x[1] / exposure, x[2] / exposure);
}
