// Small shared helpers: math, seeded randomness, the Unreal <-> three.js mapping, and a once-per-key logger.
import * as THREE from 'three';

export const DEG = Math.PI / 180;
export const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
export const lerp = (a, b, t) => a + (b - a) * t;
export const smooth01 = (t) => { t = clamp(t, 0, 1); return t * t * (3 - 2 * t); };
export const damp = (a, b, rate, dt) => lerp(a, b, 1 - Math.exp(-rate * dt));

export function mulberry32(a) {
  return function () {
    a |= 0; a = a + 0x6D2B79F5 | 0;
    let t = Math.imul(a ^ a >>> 15, 1 | a);
    t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t;
    return ((t ^ t >>> 14) >>> 0) / 4294967296;
  };
}
export function hash2(x, y, seed = 0) {
  let h = 2166136261 ^ seed;
  h = Math.imul(h ^ (x | 0), 16777619); h = Math.imul(h ^ (y | 0), 16777619);
  h ^= h >>> 13; h = Math.imul(h, 0x5bd1e995); h ^= h >>> 15;
  return (h >>> 0) / 4294967296;
}

// The level's data is Unreal centimetres (X north, Y east, Z up); the page works in three.js metres with
// x = -Y/100, y = Z/100, z = X/100. A UE yaw (degrees from +X toward +Y) is rotation.y = -yaw.
export const ueToThree = (X, Y, Z = 0) => new THREE.Vector3(-Y / 100, Z / 100, X / 100);
export const threeToUE = (v) => ({ X: v.z * 100, Y: -v.x * 100, Z: v.y * 100 });
export const yawToRotY = (yawDeg) => -yawDeg * DEG;
// The three.js unit direction of a UE yaw and a pitch (degrees, up positive).
export function dir3(yawDeg, pitchDeg, out = new THREE.Vector3()) {
  const a = yawDeg * DEG, p = pitchDeg * DEG;
  return out.set(-Math.sin(a) * Math.cos(p), Math.sin(p), Math.cos(a) * Math.cos(p));
}
// The UE yaw (degrees) a three.js direction looks along.
export const ueYawOf = (d) => ((Math.atan2(-d.x, d.z) / DEG) % 360 + 360) % 360;
// UE keeps the horizontal FOV fixed; three wants the vertical one.
export const vfovFromH = (hDeg, aspect) => 2 * Math.atan(Math.tan(hDeg * DEG / 2) / aspect) / DEG;

const _logged = new Set();
export function logOnce(key, ...args) {
  if (_logged.has(key)) return;
  _logged.add(key);
  console.info('[StyleLab]', ...args);
}

// '#rrggbb' (sRGB) or a THREE.Color or [r, g, b] (linear) -> a linear THREE.Color.
export function linColor(c, fallback = '#ffffff') {
  if (c === undefined || c === null) c = fallback;
  if (c && c.isColor) return c.clone();
  if (Array.isArray(c)) return new THREE.Color().setRGB(c[0], c[1], c[2], THREE.LinearSRGBColorSpace);
  if (typeof c === 'number') return new THREE.Color().setScalar(c);
  return new THREE.Color(c);   // three converts sRGB hex to the linear working space
}
// '#rrggbb' -> display (sRGB) components, for post passes that work after tone mapping.
export function srgbVec(c, fallback = '#000000') {
  const col = linColor(c, fallback);
  const s = col.clone().convertLinearToSRGB();
  return new THREE.Vector3(s.r, s.g, s.b);
}

export const nextFrame = () => new Promise((r) => requestAnimationFrame(() => r()));
export const sleep = (ms) => new Promise((r) => setTimeout(r, ms));

// A tiny string hash for shader cache keys.
export function strHash(s) {
  let h = 5381;
  for (let i = 0; i < s.length; i++) h = (Math.imul(h, 33) ^ s.charCodeAt(i)) | 0;
  return (h >>> 0).toString(36);
}
