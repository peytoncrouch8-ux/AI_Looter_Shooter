// Ember Gothic's ground fog: a scene-referred pass that marches each view ray through a low, rolling fog layer
// (exponential in height, broken into slow drifting banks by world-space noise), lit by the moon (brighter looking
// toward it) and by the lanterns, whose light scatters in the fog as warm glowing pools (an analytic in-scatter per
// lamp, so the halos are smooth however coarse the march). The kit's height fog stays on for the distance; this pass
// adds the body and motion of the fog where the player stands.
// In Unreal (Medium): a second exponential height fog for the base, plus camera-faded fog cards (soft particles with a
// panning noise) placed along the streets and graves, and the lanterns' lights with Volumetric Scattering Intensity
// faked by an additive glow sprite per lamp.

import * as THREE from 'three';

const MAXL = 8;

// The terrain's heights (the export's heights.bin, 1024 x 1024 float metres over [-200, 200] m), pooled to 256 x 256
// half floats, so the fog's floor can follow the ground down into pits (the Sink) while it stays in the valley where the
// ground rises above it (the chapel knoll). Fetched when the module loads, long before the first gothic frame.
const HEIGHTS = { tex: null, rect: [-200, -200, 200, 200], promise: null };
function loadHeights(THREE) {
  if (HEIGHTS.promise) return HEIGHTS.promise;
  const data = (new URLSearchParams(location.search).get('data') || 'data/').replace(/\/?$/, '/');
  HEIGHTS.promise = fetch(data + 'terrain/heights.bin').then((r) => (r.ok ? r.arrayBuffer() : null)).then((buf) => {
    if (!buf || buf.byteLength < 1024 * 1024 * 4) return null;
    const src = new Float32Array(buf), N = 1024, M = 256, k = N / M;
    const out = new Uint16Array(M * M);
    for (let r = 0; r < M; r++) for (let c = 0; c < M; c++) {
      let s = 0;
      for (let y = 0; y < k; y++) for (let x = 0; x < k; x++) s += src[(r * k + y) * N + c * k + x];
      out[r * M + c] = THREE.DataUtils.toHalfFloat(s / (k * k));
    }
    const t = new THREE.DataTexture(out, M, M, THREE.RedFormat, THREE.HalfFloatType);
    t.minFilter = THREE.LinearFilter; t.magFilter = THREE.LinearFilter; t.needsUpdate = true;
    HEIGHTS.tex = t;
    return t;
  }).catch(() => null);
  return HEIGHTS.promise;
}

const FRAG = /* glsl */`
uniform mat4 uInvView;
uniform vec3 uCamPos;
uniform vec3 uMoonDir, uMoonCol, uAmbCol;
uniform vec4 uFog;        // density, height falloff (per m), base height (m), max distance (m)
uniform sampler2D uHeights;
uniform vec4 uHRect;
uniform float uHasH;
uniform vec4 uNoise;      // scale, contrast, wind x, wind z
uniform float uLampScatter;
uniform int uNL;
uniform vec4 uLP[${MAXL}];  // position, range
uniform vec3 uLC[${MAXL}];  // colour x intensity

float fogNoise(vec3 p) {
  vec2 w = uNoise.zw * uTime;
  vec2 a = p.xz * uNoise.x + w + vec2(p.y * 0.21, -p.y * 0.17);
  float n = slVNoise(a) * 0.62 + slVNoise(a * 2.37 + 5.1 + w * 0.6) * 0.38;
  return clamp(0.5 + (n - 0.5) * uNoise.y, 0.0, 1.0);
}
float fogFloor(vec2 xz) {
  if (uHasH < 0.5) return uFog.z;
  vec2 t = (xz - uHRect.xy) / (uHRect.zw - uHRect.xy);
  if (t.x <= 0.0 || t.y <= 0.0 || t.x >= 1.0 || t.y >= 1.0) return uFog.z;
  // the fog lies on the ground where the ground dips below its level, and stays at its level where the ground rises
  return min(texture2D(uHeights, t).r - 0.3, uFog.z);
}
float fogDensity(vec3 p) {
  float h = p.y - fogFloor(p.xz);
  // below the base (the Sink's floor) the fog stays at its base density instead of thickening without end
  return uFog.x * exp(-max(h, 0.0) * uFog.y) * (0.15 + 1.7 * fogNoise(p));
}
// the optical depth of one stretch of the ray, the height profile integrated exactly (so a long stretch that only
// grazes the thin layer at the ground counts it right, with no dither), the noise taken at the stretch's middle
float fogDepth(vec3 ro, vec3 wd, float t0, float t1) {
  vec3 pm = ro + wd * (0.5 * (t0 + t1));
  float fl = fogFloor(pm.xz);
  float k = uFog.y;
  float y0 = max(ro.y + wd.y * t0 - fl, 0.0), y1 = max(ro.y + wd.y * t1 - fl, 0.0);
  float dy = y1 - y0, len = t1 - t0;
  float prof = abs(k * dy) > 1e-3 ? (exp(-k * y0) - exp(-k * y1)) / (k * dy) : exp(-k * y0);
  return uFog.x * prof * len * (0.15 + 1.7 * fogNoise(pm));
}

vec4 effect(vec2 uv) {
  vec3 col = texture2D(tColor, uv).rgb;
  float raw = rawDepth(uv);
  vec3 vp = viewPos(uv);
  bool sky = raw >= 0.99999;
  bool gun = raw < 0.0011;
  vec3 vdir = normalize(vp);
  float dist = sky ? uFog.w : min(length(vp), uFog.w);
  if (gun) return vec4(col, 1.0);
  vec3 wdir = normalize((uInvView * vec4(vdir, 0.0)).xyz);
  vec3 ro = uCamPos;
  float j = slIGN(gl_FragCoord.xy + fract(uTime * 3.7) * 61.0);
  const int N = 12;
  float T = 1.0;
  vec3 L = vec3(0.0);
  float mu = dot(wdir, uMoonDir);
  float g = 0.4;
  float hg = (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * mu, 1.5);
  vec3 lightIn = uAmbCol + uMoonCol * (0.25 + 0.6 * hg);
  float prevT = 0.0;
  for (int i = 0; i < N; i++) {
    float a = (float(i) + 0.5 + (j - 0.5) * 0.6) / float(N);
    float t = (i == N - 1) ? dist : dist * a * a;
    float tr = exp(-fogDepth(ro, wdir, prevT, t));
    prevT = t;
    L += T * lightIn * (1.0 - tr);
    T *= tr;
  }
  // the lanterns: the closed form of the in-scatter of an isotropic point light along the ray, weighted by the fog
  // density near the lamp and the transmittance up to it
  vec3 lamps = vec3(0.0);
  for (int i = 0; i < ${MAXL}; i++) {
    if (i >= uNL) break;
    vec3 lp = uLP[i].xyz - ro;
    float b = dot(lp, wdir);
    float h2 = max(dot(lp, lp) - b * b, 0.04);
    float hh = sqrt(h2);
    float I = (atan((dist - b) / hh) - atan(-b / hh)) / hh;
    // the fog where the ray passes the lamp scatters its light: a ray climbing into the sky meets little of it
    vec3 pc = ro + wdir * clamp(b, 0.0, dist);
    float dens = 0.75 * fogDensity(pc) + 0.25 * fogDensity(uLP[i].xyz) + uFog.x * 0.04;
    float near = clamp(b / max(dist, 1e-3), 0.0, 1.0);
    float Tb = mix(1.0, T, near);
    // keep a lamp's glow to a pool about its own range, so the warm stays local and the world stays cold
    float reach = 1.0 - smoothstep(uLP[i].w * 0.7, uLP[i].w * 2.2, hh);
    // a lamp right beside the camera would haze the whole frame: its glow is kept to the lamp itself
    float beside = mix(0.2, 1.0, smoothstep(2.0, 8.0, length(lp)));
    lamps += uLC[i] * I * dens * Tb * reach * beside;
  }
  col = col * T + L + lamps * uLampScatter;
  return vec4(col, 1.0);
}`;

// o: { density, falloff, base, maxDist, noiseScale, noiseContrast, wind: [x, z], moonColor, ambColor (linear
// THREE.Colors), moonIntensity, lampScatter, extraLights: () => [{ position, color, intensity, distance }] }
// Start loading the ground heights early (the style calls this when its module loads); the pass picks them up when
// they arrive.
export function preloadFogGround() { loadHeights(THREE); }

export function makeGothicFog(kit, o) {
  const THREE = kit.THREE;
  const uniforms = {
    uInvView: { value: new THREE.Matrix4() }, uCamPos: { value: new THREE.Vector3() },
    uMoonDir: { value: new THREE.Vector3(0, 1, 0) }, uMoonCol: { value: new THREE.Color() }, uAmbCol: { value: new THREE.Color() },
    uFog: { value: new THREE.Vector4() }, uNoise: { value: new THREE.Vector4() }, uLampScatter: { value: 1 },
    uNL: { value: 0 },
    uHeights: { value: null }, uHRect: { value: new THREE.Vector4(-200, -200, 200, 200) }, uHasH: { value: 0 },
    uLP: { value: Array.from({ length: MAXL }, () => new THREE.Vector4()) },
    uLC: { value: Array.from({ length: MAXL }, () => new THREE.Color()) },
  };
  const params = Object.assign({ density: 0.05, falloff: 0.3, base: 0, maxDist: 160, noiseScale: 0.06, noiseContrast: 1.6,
    wind: [0.35, 0.12], lampScatter: 1 }, o);
  const tmp = new THREE.Vector3();
  loadHeights(THREE);
  const hr = kit.manifest && kit.manifest.terrain && kit.manifest.terrain.heights && kit.manifest.terrain.heights.rect;
  if (hr) uniforms.uHRect.value.set(hr[0], hr[1], hr[2], hr[3]);
  const bind = (p, u) => {
    const cam = kit.camera;
    u.uInvView.value.copy(cam.matrixWorld);
    u.uCamPos.value.setFromMatrixPosition(cam.matrixWorld);
    u.uMoonDir.value.copy(kit.sun.sunDir);
    u.uMoonCol.value.copy(p.moonColor);
    u.uAmbCol.value.copy(p.ambColor);
    u.uFog.value.set(p.density, p.falloff, p.base, p.maxDist);
    u.uNoise.value.set(p.noiseScale, p.noiseContrast, p.wind[0], p.wind[1]);
    u.uLampScatter.value = p.lampScatter;
    u.uHeights.value = HEIGHTS.tex; u.uHasH.value = HEIGHTS.tex ? 1 : 0;
    // the lamps that are lit right now: the engine's pool, then the style's own
    const list = [];
    for (const l of kit.sun.pool || []) if (l.intensity > 0) list.push(l);
    if (p.extraLights) for (const l of p.extraLights()) if (l && l.intensity > 0) list.push(l);
    let n = 0;
    for (const l of list) {
      if (n >= MAXL) break;
      l.getWorldPosition(tmp);
      u.uLP.value[n].set(tmp.x, tmp.y, tmp.z, l.distance || 8);
      u.uLC.value[n].copy(l.color).multiplyScalar(l.intensity * (l.userData.fogScale ?? 1));
      n++;
    }
    u.uNL.value = n;
  };
  return kit.post.custom({ fragment: FRAG, uniforms, params, bind, needsDepth: true, stage: 'hdr' });
}
