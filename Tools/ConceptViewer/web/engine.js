import * as THREE from 'three';
import { OrbitControls } from 'three/addons/controls/OrbitControls.js';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';

// ================================================================ basics
const $ = (s) => document.querySelector(s);
const DEG = Math.PI / 180;
const clamp = (v, a, b) => Math.min(b, Math.max(a, v));
const lerp = (a, b, t) => a + (b - a) * t;
const smooth01 = (t) => { t = clamp(t, 0, 1); return t * t * (3 - 2 * t); };
function mulberry32(a) { return function () { a |= 0; a = a + 0x6D2B79F5 | 0; let t = Math.imul(a ^ a >>> 15, 1 | a); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; }; }
function hashSeed(...nums) { let h = 2166136261; for (const n of nums) { h ^= Math.round(n) | 0; h = Math.imul(h, 16777619); } return h >>> 0; }

const DATA = window.__DATA__;
const LAYOUT = DATA.layout, COMPUTED = DATA.computed, MANIFEST = DATA.manifest;
const IS_MOBILE = (window.matchMedia && matchMedia('(pointer: coarse)').matches) || Math.min(screen.width, screen.height) < 700;

// ================================================================ coordinates
// The layout is in Unreal centimeters: X north, Y east, Z up. three.js meters: x = -Y, y = Z, z = X (a mirror, which
// turns Unreal's left-handed frame into three's right-handed one). An Unreal yaw turns +X toward +Y.
const toX = (Y) => -Y / 100, toZ = (X) => X / 100;
const rotY = (yawDeg) => -yawDeg * DEG;
const ueYawOf = (dX, dY) => Math.atan2(dY, dX) / DEG;
const uePos = (x3, z3) => ({ X: z3 * 100, Y: -x3 * 100 });
// A point f cm ahead and r cm to the right of (X, Y) facing yaw.
function local(X, Y, yawDeg, f, r) { const a = yawDeg * DEG; return [X + Math.cos(a) * f - Math.sin(a) * r, Y + Math.sin(a) * f + Math.cos(a) * r]; }
// The three.js direction of an Unreal yaw and a pitch (degrees, up positive).
function dir3(yawDeg, pitchDeg) { const a = yawDeg * DEG, p = pitchDeg * DEG; return new THREE.Vector3(-Math.sin(a) * Math.cos(p), Math.sin(p), Math.cos(a) * Math.cos(p)); }

// ================================================================ heights (sampled from the decimated terrain the viewer draws)
let HM = null;
function groundAt(x, z) {
  if (!HM) return 0;
  const { n, half, data } = HM;
  const gx = (x + half) / (2 * half) * n - 0.5, gz = (z + half) / (2 * half) * n - 0.5;
  const c0 = Math.floor(gx), r0 = Math.floor(gz);
  if (c0 < 0 || r0 < 0 || c0 >= n - 1 || r0 >= n - 1) return NaN;
  const fx = gx - c0, fz = gz - r0;
  const h00 = data[r0 * n + c0], h01 = data[r0 * n + c0 + 1], h10 = data[(r0 + 1) * n + c0], h11 = data[(r0 + 1) * n + c0 + 1];
  if (h00 < -900 || h01 < -900 || h10 < -900 || h11 < -900) return NaN;
  return lerp(lerp(h00, h01, fx), lerp(h10, h11, fx), fz);
}
const groundUE = (X, Y) => groundAt(toX(Y), toZ(X));
function slopeUE(X, Y) {
  const x = toX(Y), z = toZ(X), d = 1.2;
  const a = groundAt(x - d, z), b = groundAt(x + d, z), c = groundAt(x, z - d), e = groundAt(x, z + d);
  if ([a, b, c, e].some(Number.isNaN)) return 9;
  return Math.max(Math.abs(b - a), Math.abs(e - c)) / (2 * d);
}

// ================================================================ palette (display colors, Docs/Art/ScreenPrintWash.md)
const PAL = {
  grass: '#68753e', dirt: '#bba47e', dirtEdge: '#a68f6b', rut: '#9a8462', path: '#c4ae8a', brickA: '#ad6b5b', brickB: '#97594c',
  cobbleA: '#c2baa9', cobbleB: '#aaa293', cobbleC: '#b8ab95', plank: '#ad936f', plankDark: '#8d7656', pencil: '#4a3f44', ink: '#2a2024',
  wheat: '#d8bf72', wheatRow: '#c0a65a', plowed: '#9b7c5e', furrow: '#7e634b', crop: '#86a457', cropRow: '#8a6f55', cabbage: '#9cbf6b',
  pasture: '#7d8c48', hay: '#d4bb6c', bog: '#5b6f40', bogDark: '#4d5f37', needles: '#8b7663', moss: '#7c9a58', puddle: '#a9d0cf',
  oakLeaf: '#6b8a49', oakLeafUp: '#89a55b', oakLeafDark: '#5a7a40', birchLeaf: '#a0b766', birchLeafUp: '#b6c978', pine: '#4e6b47', pineUp: '#5f7f55',
  bush: '#7a9853', bushDark: '#62813f', hedge: '#6d8c4a', apple: '#7fa058', appleFruit: '#d3604f', barkOak: '#7a6a58', barkBirch: '#e1dbcd',
  birchMark: '#4a4246', barkPine: '#8c6a55', barkDead: '#958879', flowerPink: '#ec9db4', flowerYellow: '#f0d67a', flowerWhite: '#f7f2e6',
  flowerBlue: '#a5b8e8', flowerRed: '#e07a6a', stem: '#6f8a48', reed: '#93a35c', cattail: '#7d5a44', water: '#a9cfd3', web: '#f6f1e8',
  sac: '#f1e8d8', cocoon: '#e9e0cf', mushCap: '#cf6552', mushStem: '#f0e7d6', slimeTrail: '#7d9a5c', picket: '#efe7d6', awningRed: '#c9665a',
  awningCream: '#f3e8d2', smoke: '#f6f1e8', bird: '#3a3036', stone: '#a8a093', stoneDark: '#8f887c', tuftA: '#72863f', tuftB: '#8c9c4d',
  wheatTuft: '#dcc477', shirt: '#6f9fae', hat: '#5c4b40', target: '#d4c08e', cream: '#f6eedc', lantern: '#f6c97c',
  wool: '#f4eee2', sheepFace: '#3d3437', cowA: '#8a5a44', cowB: '#f1e9da', hen: '#f3ece0', henBrown: '#b9774f', comb: '#d65a4a', beak: '#e7b04a',
  skin: '#e6bf98', skinB: '#b98a64', meadow: '#7f8d45', meadowB: '#8a9449',
};
const C = (hex) => new THREE.Color(hex);

// ================================================================ the print material (Screen Print Wash)
// Two tones of light: a toon ramp softened over a short band of N·L (lit above about 0.45), shade = fill x violet, or x
// deep teal for accents. Vertex color alpha flags the fill: 1 normal, 0.5 accent (teal shade), 0 unshaded (glows,
// flower heads, webs, smoke). The lit fraction goes to alpha, where the post-process puts pigment granulation.
function makeGradient() {
  const n = 64, data = new Uint8Array(n * 4);
  for (let i = 0; i < n; i++) {
    const ndl = ((i + 0.5) / n) * 2 - 1, t = clamp((ndl - 0.38) / 0.14, 0, 1), v = Math.round(255 * t * t * (3 - 2 * t));
    data.set([v, v, v, 255], i * 4);
  }
  const t = new THREE.DataTexture(data, n, 1, THREE.RGBAFormat);
  t.minFilter = t.magFilter = THREE.NearestFilter; t.needsUpdate = true;
  return t;
}
const GRAD = makeGradient();
const LIGHT_LINE = 'vec3 outgoingLight = reflectedLight.directDiffuse + reflectedLight.indirectDiffuse + totalEmissiveRadiance;';
const PRINT_LIGHT = `
  float lp_lit = clamp(max(reflectedLight.directDiffuse.r, max(reflectedLight.directDiffuse.g, reflectedLight.directDiffuse.b)) * PI
                       / max(max(diffuseColor.r, max(diffuseColor.g, diffuseColor.b)), 1e-4), 0.0, 1.0);
  float lp_flag = 1.0;
  #ifdef USE_COLOR_ALPHA
    lp_flag = vColor.a;
  #endif
  vec3 lp_tint = (lp_flag > 0.25 && lp_flag < 0.75) ? vec3(0.60, 0.74, 0.80) : vec3(0.70, 0.64, 0.86);
  lp_tint = pow(lp_tint, vec3(2.2));
  vec3 outgoingLight = (lp_flag <= 0.25) ? diffuseColor.rgb : diffuseColor.rgb * mix(lp_tint, vec3(1.0), lp_lit);
  float lp_alpha = (lp_flag <= 0.25) ? 1.0 : lp_lit;`;
function printMaterial({ side = THREE.FrontSide, decal = false } = {}) {
  const m = new THREE.MeshToonMaterial({ vertexColors: true, gradientMap: GRAD, fog: true, side });
  if (decal) { m.polygonOffset = true; m.polygonOffsetFactor = -2; m.polygonOffsetUnits = -4; }
  m.onBeforeCompile = (shader) => {
    shader.fragmentShader = shader.fragmentShader
      .replace(LIGHT_LINE, PRINT_LIGHT)
      .replace('#include <opaque_fragment>', 'gl_FragColor = vec4( outgoingLight, lp_alpha );')
      .replace('#include <output_fragment>', 'gl_FragColor = vec4( outgoingLight, lp_alpha );');
  };
  m.customProgramCacheKey = () => `print-${side}-${decal}`;
  return m;
}
const MAT = printMaterial();
const MAT2 = printMaterial({ side: THREE.DoubleSide });
const MAT_DECAL = printMaterial({ decal: true });

// ================================================================ sky: Sable's pastel gradient with flat clouds outlined in pencil
const SKY_FS = `
  uniform vec3 cHorizon, cLow, cMid, cTop, cUnder, cCloud, cPencil; uniform float uTime;
  varying vec3 vDir;
  float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453); }
  float vnoise(vec2 p){ vec2 i = floor(p), f = fract(p); f = f*f*(3.-2.*f);
    return mix(mix(hash(i), hash(i+vec2(1,0)), f.x), mix(hash(i+vec2(0,1)), hash(i+vec2(1,1)), f.x), f.y); }
  float fbm(vec2 p){ float v = 0., a = 0.55; for (int i = 0; i < 4; i++) { v += a * vnoise(p); p = p * 2.07 + 5.3; a *= 0.48; } return v; }
  void main(){
    vec3 d = normalize(vDir); float h = d.y; vec3 c;
    if (h < 0.0) c = mix(cHorizon, cUnder, smoothstep(0.0, -0.4, h));
    else { float t = pow(clamp(h * 1.45, 0., 1.), 0.8);
      c = t < 0.28 ? mix(cHorizon, cLow, t / 0.28) : (t < 0.62 ? mix(cLow, cMid, (t - 0.28) / 0.34) : mix(cMid, cTop, (t - 0.62) / 0.38)); }
    if (h > 0.02) {
      vec2 p = d.xz / (h + 0.22) * 0.9 + vec2(uTime * 0.0025, 0.0);
      float n = fbm(p);
      float band = smoothstep(0.04, 0.16, h) * (1.0 - smoothstep(0.55, 0.92, h));
      float thr = 0.62, fw = fwidth(n) * 1.1 + 1e-4;
      float mask = smoothstep(thr - fw, thr + fw, n) * band;
      float ring = (1.0 - smoothstep(0.0, fw * 1.7, abs(n - thr))) * band;
      c = mix(c, cCloud, mask);
      c = mix(c, cPencil, ring * 0.55);
    }
    gl_FragColor = vec4(c, 1.0);
  }`;
function makeSky() {
  const u = { cHorizon: { value: C('#f6dcb8') }, cLow: { value: C('#e9cddb') }, cMid: { value: C('#bcd4ee') }, cTop: { value: C('#98bde9') },
    cUnder: { value: C('#ead9c3') }, cCloud: { value: C('#fefbf4') }, cPencil: { value: C('#5a4c52') }, uTime: { value: 0 } };
  const m = new THREE.ShaderMaterial({ uniforms: u, side: THREE.BackSide, depthWrite: false, fog: false,
    vertexShader: 'varying vec3 vDir; void main(){ vDir = position; gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }',
    fragmentShader: SKY_FS });
  const mesh = new THREE.Mesh(new THREE.SphereGeometry(1500, 40, 20), m);
  mesh.frustumCulled = false; mesh.renderOrder = -10;
  return mesh;
}

// ================================================================ the print post-process
// Lines come from inverse depth: on any plane 1/z is linear across the screen, so its second difference is zero and
// flat ground never draws false lines however grazing the view. A line is drawn on the near side of a depth step or a
// convex corner only (crisp, one pixel), plus creases from the view-space normals. The color plate lands a couple of
// pixels off the lines (misregistration), shade carries pigment granulation, and cream paper sits over everything.
const POST_FS = `
  precision highp float;
  varying vec2 vUv;
  uniform sampler2D tColor, tDepth, tNormal; uniform vec2 uRes;
  uniform float uNear, uFar, uLines, uNormals, uTime, uS, uNFar;
  float lin(vec2 uv){ float z = texture2D(tDepth, uv).x * 2.0 - 1.0; return (2.0 * uNear * uFar) / (uFar + uNear - z * (uFar - uNear)); }
  float hash(vec2 p){ return fract(sin(dot(p, vec2(127.1,311.7))) * 43758.5453); }
  float vnoise(vec2 p){ vec2 i = floor(p), f = fract(p); f = f*f*(3.-2.*f);
    return mix(mix(hash(i), hash(i+vec2(1,0)), f.x), mix(hash(i+vec2(0,1)), hash(i+vec2(1,1)), f.x), f.y); }
  float inkAt(vec2 uv, float px){
    vec2 o = px / uRes; float d0 = lin(uv); float w0 = 1.0 / d0;
    float wl = 1.0 / lin(uv - vec2(o.x, 0.)), wr = 1.0 / lin(uv + vec2(o.x, 0.));
    float wd = 1.0 / lin(uv - vec2(0., o.y)), wu = 1.0 / lin(uv + vec2(0., o.y));
    float lapS = min(wl + wr - 2.0 * w0, wd + wu - 2.0 * w0) / w0;
    float dEdge = smoothstep(0.022, 0.06, -lapS);
    float nEdge = 0.0;
    if (uNormals > 0.5) {
      vec3 n = texture2D(tNormal, uv).xyz * 2. - 1.;
      vec3 nr = texture2D(tNormal, uv + vec2(o.x, 0.)).xyz * 2. - 1., nu = texture2D(tNormal, uv + vec2(0., o.y)).xyz * 2. - 1.;
      float nd = max(1. - dot(n, nr), 1. - dot(n, nu));
      nEdge = smoothstep(0.2, 0.4, nd) * (1.0 - smoothstep(uNFar * 0.27, uNFar, d0));
    }
    return max(dEdge, nEdge) * (1.0 - smoothstep(700.0, 1300.0, d0));
  }
  vec3 toSRGB(vec3 c){ return pow(max(c, 0.), vec3(1. / 2.2)); }
  void main(){
    float s = uS; vec2 px = 1.0 / uRes; vec2 fc = gl_FragCoord.xy;
    vec4 cA = texture2D(tColor, vUv - vec2(2.0 * s, -1.2 * s) * px);
    vec3 c = toSRGB(cA.rgb); float lit = cA.a;
    float mottle = vnoise(fc / (5.0 * s)) - 0.5;
    float speck = step(0.955, hash(floor(fc / max(1.0, 1.2 * s))));
    c *= mix(1.0, 1.0 + mottle * 0.16 - speck * 0.12, (1.0 - lit) * 0.9);
    float fiber = (vnoise(fc * 0.8 / s) - 0.5) * 2.0, mot2 = (vnoise(fc / (120.0 * s)) - 0.5) * 2.0;
    float show = 0.35 + 0.65 * dot(c, vec3(0.2126, 0.7152, 0.0722));
    c *= 1.0 + (0.018 * fiber + 0.03 * mot2) * show;
    c = (c - 0.55) * 1.05 + 0.55;
    float lw = max(1.0, s);
    float ink = inkAt(vUv, lw) * uLines;
    vec2 cell = floor(fc / (9.0 * s));
    vec2 pj = (vec2(1.2, -0.8) * s + (vec2(hash(cell), hash(cell + 13.0)) - 0.5) * 1.2 * s) * px;
    float pencil = inkAt(vUv + pj, lw) * uLines * (1.0 - smoothstep(35.0, 130.0, lin(vUv)));
    c = mix(c, vec3(0.290, 0.247, 0.267), pencil * 0.45);
    c = mix(c, vec3(0.165, 0.125, 0.141), ink * 0.86);
    c += (hash(fc + fract(uTime)) - 0.5) * 0.012;
    gl_FragColor = vec4(clamp(c, 0., 1.), 1.0);
  }`;

// ================================================================ renderer, scene, light
const canvas = $('#view');
const renderer = new THREE.WebGLRenderer({ canvas, antialias: false, alpha: false, powerPreference: 'high-performance', preserveDrawingBuffer: false });
let pixelRatio = Math.min(window.devicePixelRatio || 1, IS_MOBILE ? 2 : 1.75);
renderer.setPixelRatio(pixelRatio);
renderer.outputColorSpace = THREE.SRGBColorSpace;
renderer.toneMapping = THREE.NoToneMapping;
renderer.shadowMap.enabled = true;
renderer.shadowMap.type = THREE.PCFShadowMap;
renderer.shadowMap.autoUpdate = false;

const scene = new THREE.Scene();
const FOG = C('#f1dcc0');
scene.fog = new THREE.Fog(FOG, 80, 500);
const camera = new THREE.PerspectiveCamera(60, 1, 0.4, 3200);
scene.add(camera);
const sky = makeSky(); scene.add(sky);

// The game's afternoon sun: from the west-northwest, 38 degrees up (build_area.py's DirectionalLight).
const sunDirUE = new THREE.Vector3(0.295, -0.731, 0.616);
const sunDir = new THREE.Vector3(-sunDirUE.y, sunDirUE.z, sunDirUE.x).normalize();
const sun = new THREE.DirectionalLight(0xffffff, 1.0);
// One fixed shadow map over the whole island (it is about 200 m across), drawn once per concept: crisp and free.
const SHADOW = IS_MOBILE ? 2048 : 4096, SHADOW_EXT = 116;
sun.castShadow = true;
sun.shadow.mapSize.set(SHADOW, SHADOW);
Object.assign(sun.shadow.camera, { left: -SHADOW_EXT, right: SHADOW_EXT, top: SHADOW_EXT, bottom: -SHADOW_EXT, near: 220, far: 640 });
sun.shadow.bias = -0.00012; sun.shadow.normalBias = 0.05;
sun.position.copy(sunDir).multiplyScalar(420); sun.target.position.set(0, 0, 0);
scene.add(sun); scene.add(sun.target);
sun.shadow.camera.updateProjectionMatrix();

const base = new THREE.Group(); scene.add(base);   // terrain, cliffs, water, sky islands, birds
const fx = new THREE.Group(); scene.add(fx);       // animated things shared by all concepts

// Render targets and the post pass.
let rtColor = null, rtNormal = null;
const normalMat = new THREE.MeshNormalMaterial();
const postMat = new THREE.ShaderMaterial({
  uniforms: { tColor: { value: null }, tDepth: { value: null }, tNormal: { value: null }, uRes: { value: new THREE.Vector2(1, 1) },
    uNear: { value: camera.near }, uFar: { value: camera.far }, uLines: { value: 1 }, uNormals: { value: 1 }, uTime: { value: 0 }, uS: { value: 1 }, uNFar: { value: 260 } },
  vertexShader: 'varying vec2 vUv; void main(){ vUv = uv; gl_Position = vec4(position.xy, 0.0, 1.0); }',
  fragmentShader: POST_FS, depthTest: false, depthWrite: false });
const postScene = new THREE.Scene();
postScene.add(new THREE.Mesh(new THREE.PlaneGeometry(2, 2), postMat));
const postCam = new THREE.OrthographicCamera(-1, 1, 1, -1, 0, 1);

function resize() {
  const w = canvas.clientWidth || window.innerWidth, h = canvas.clientHeight || window.innerHeight;
  renderer.setPixelRatio(pixelRatio);
  renderer.setSize(w, h, false);
  camera.aspect = w / h; camera.updateProjectionMatrix();
  const size = renderer.getDrawingBufferSize(new THREE.Vector2());
  if (rtColor) { rtColor.dispose(); rtNormal.dispose(); }
  const depthTexture = new THREE.DepthTexture(size.x, size.y); depthTexture.type = THREE.UnsignedIntType;
  rtColor = new THREE.WebGLRenderTarget(size.x, size.y, { depthTexture, depthBuffer: true, samples: IS_MOBILE ? 0 : 4,
    minFilter: THREE.LinearFilter, magFilter: THREE.LinearFilter });
  rtNormal = new THREE.WebGLRenderTarget(size.x, size.y, { depthBuffer: true, minFilter: THREE.NearestFilter, magFilter: THREE.NearestFilter });
  postMat.uniforms.tColor.value = rtColor.texture; postMat.uniforms.tDepth.value = depthTexture;
  postMat.uniforms.tNormal.value = rtNormal.texture; postMat.uniforms.uRes.value.set(size.x, size.y);
  postMat.uniforms.uS.value = Math.max(0.75, size.y / 900);
}

// Small, many things (grass, flowers, cobbles, bricks) stay out of the normal pass: their creases would only speckle the
// ground from any distance. Their outlines still come from depth, up close.
const FINE_MESHES = [];
function renderFrame() {
  renderer.setRenderTarget(rtColor); renderer.setClearColor(0xf6dcb8, 1); renderer.render(scene, camera);
  if (postMat.uniforms.uNormals.value > 0.5) {
    scene.overrideMaterial = normalMat; const skyVisible = sky.visible; sky.visible = false;
    for (const m of FINE_MESHES) m.visible = false;
    renderer.setRenderTarget(rtNormal); renderer.setClearColor(0x8080ff, 1); renderer.render(scene, camera);
    for (const m of FINE_MESHES) m.visible = true;
    scene.overrideMaterial = null; sky.visible = skyVisible;
  }
  renderer.setRenderTarget(null); renderer.render(postScene, postCam);
}

// ================================================================ geometry helpers
// Every mesh shares one vertex layout: position, normal (flat), color (RGBA, alpha = the shading flag).
function paint(geo, hex, flag = 1) {
  const g = geo.index ? geo.toNonIndexed() : geo;
  for (const k of Object.keys(g.attributes)) if (!['position', 'normal', 'color'].includes(k)) g.deleteAttribute(k);
  g.computeVertexNormals();
  const n = g.attributes.position.count, c = C(hex), arr = new Float32Array(n * 4);
  for (let i = 0; i < n; i++) { arr[i * 4] = c.r; arr[i * 4 + 1] = c.g; arr[i * 4 + 2] = c.b; arr[i * 4 + 3] = flag; }
  g.setAttribute('color', new THREE.BufferAttribute(arr, 4));
  return g;
}
function merged(parts) {
  const g = mergeGeometries(parts.filter(Boolean), false);
  g.computeBoundingBox(); g.computeBoundingSphere();
  return g;
}
const T = (geo, x, y, z) => { geo.translate(x, y, z); return geo; };
const ico = (r, detail = 1, sx = 1, sy = 1, sz = 1) => { const g = new THREE.IcosahedronGeometry(r, detail); g.scale(sx, sy, sz); return g; };
const cyl = (r0, r1, h, seg = 7) => new THREE.CylinderGeometry(r1, r0, h, seg);
const box = (w, h, d) => new THREE.BoxGeometry(w, h, d);
// Lumps a geometry centered on the origin by a smooth function of position, so shared corners move together.
function lumpy(geo, amount, seed) {
  const p = geo.attributes.position, v = new THREE.Vector3();
  for (let i = 0; i < p.count; i++) {
    v.fromBufferAttribute(p, i);
    const k = 1 + amount * Math.sin(v.x * 3.1 + seed) * Math.sin(v.y * 2.7 + seed * 1.3) * Math.sin(v.z * 3.7 + seed * 0.7);
    p.setXYZ(i, v.x * k, v.y * k, v.z * k);
  }
  return geo;
}
// A cone whose rim alternates in and out: a pine's ragged skirt.
function jagCone(r, h, seg, jag) {
  const g = new THREE.ConeGeometry(r, h, seg, 1, false), p = g.attributes.position;
  for (let i = 0; i < p.count; i++) {
    const x = p.getX(i), y = p.getY(i), z = p.getZ(i);
    if (y < -h / 2 + 1e-4 && (x * x + z * z) > 1e-6) { const a = Math.atan2(z, x), k = 1 - (1 - jag) * 0.5 * (1 - Math.cos(seg / 2 * a)); p.setX(i, x * k); p.setZ(i, z * k); }
  }
  return g;
}
// A triangle soup from flat [x,y,z,...] positions and one color per triangle.
function soup(pos, colors, flags) {
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  const n = pos.length / 3, arr = new Float32Array(n * 4);
  for (let t = 0; t < n / 3; t++) { const c = C(colors[t % colors.length]), f = flags ? flags[t % flags.length] : 1; for (let k = 0; k < 3; k++) arr.set([c.r, c.g, c.b, f], (t * 3 + k) * 4); }
  g.setAttribute('color', new THREE.BufferAttribute(arr, 4));
  g.computeVertexNormals();
  return g;
}

// ================================================================ model loading: GLB -> one geometry in the shared layout
const loader = new GLTFLoader();
const GEO = new Map();
// Binary files travel as base64 inside .json (the page's host serves JSON but not .glb or .bin).
function b64ToBuffer(s) { const bin = atob(s), n = bin.length, out = new Uint8Array(n); for (let i = 0; i < n; i++) out[i] = bin.charCodeAt(i); return out.buffer; }
const fetchBinary = (file) => fetch(file).then((r) => { if (!r.ok) throw new Error(`${file}: ${r.status}`); return r.json(); }).then(b64ToBuffer);
function modelEntry(name) { return MANIFEST.models[name] || (MANIFEST.terrain && MANIFEST.terrain[name.toLowerCase()]); }
async function geometry(name) {
  if (GEO.has(name)) return GEO.get(name);
  const entry = modelEntry(name);
  if (!entry || !entry.file) { GEO.set(name, Promise.resolve(null)); return null; }
  const p = fetchBinary(entry.file).then((buf) => loader.parseAsync(buf, '')).then((gltf) => {
    gltf.scene.updateMatrixWorld(true);
    const parts = [];
    gltf.scene.traverse((o) => {
      if (!o.isMesh) return;
      const g = o.geometry.index ? o.geometry.toNonIndexed() : o.geometry.clone();
      g.applyMatrix4(o.matrixWorld);
      for (const k of Object.keys(g.attributes)) if (!['position', 'normal', 'color'].includes(k)) g.deleteAttribute(k);
      if (!g.attributes.normal) g.computeVertexNormals();
      const n = g.attributes.position.count, src = g.attributes.color, arr = new Float32Array(n * 4);
      for (let i = 0; i < n; i++) {
        arr[i * 4] = src ? src.getX(i) : 0.7; arr[i * 4 + 1] = src ? src.getY(i) : 0.7; arr[i * 4 + 2] = src ? src.getZ(i) : 0.7;
        arr[i * 4 + 3] = src && src.itemSize === 4 ? src.getW(i) : 1;
      }
      g.setAttribute('color', new THREE.BufferAttribute(arr, 4));
      parts.push(g);
    });
    if (!parts.length) return null;
    return merged(parts);
  }).catch((e) => { console.warn('failed', name, e); return null; });
  GEO.set(name, p);
  return p;
}
function modelBounds(name) { const e = MANIFEST.models[name]; return e ? e.bounds : null; }
function modelRadius(name) { const b = modelBounds(name); return b ? Math.max(b.max[0] - b.min[0], b.max[2] - b.min[2]) / 2 : 0.6; }
function socketOf(name, socket) { const e = MANIFEST.models[name]; return e && e.sockets && e.sockets[socket] ? e.sockets[socket] : []; }

// ================================================================ placing instances
const tmpM = new THREE.Matrix4(), tmpQ = new THREE.Quaternion(), tmpP = new THREE.Vector3(), tmpS = new THREE.Vector3(), tmpE = new THREE.Euler();
// An item: { X, Y (cm), yaw (deg) | rotY (rad), Z (cm, absolute) | sink/lift (m), scale (number or [x,y,z]), tiltX, tiltZ, color }
function itemMatrix(it, out) {
  const x = toX(it.Y), z = toZ(it.X);
  let y = it.Z !== undefined ? it.Z / 100 : groundAt(x, z);
  if (Number.isNaN(y)) y = 0;
  tmpP.set(x, y - (it.sink || 0) + (it.lift || 0), z);
  tmpQ.setFromEuler(tmpE.set(it.tiltX || 0, it.rotY !== undefined ? it.rotY : rotY(it.yaw || 0), it.tiltZ || 0, 'YXZ'));
  const s = it.scale === undefined ? 1 : it.scale;
  if (Array.isArray(s)) tmpS.set(s[0], s[1], s[2]); else tmpS.set(s, s, s);
  return out.compose(tmpP, tmpQ, tmpS);
}
function instanced(group, geom, items, mat = MAT, { shadow = true } = {}) {
  if (!geom || !items.length) return null;
  const mesh = new THREE.InstancedMesh(geom, mat, items.length);
  let tinted = false;
  items.forEach((it, i) => { mesh.setMatrixAt(i, itemMatrix(it, tmpM)); if (it.color) tinted = true; });
  if (tinted) { items.forEach((it, i) => mesh.setColorAt(i, C(it.color || '#ffffff'))); mesh.instanceColor.needsUpdate = true; }
  mesh.castShadow = shadow; mesh.receiveShadow = true;
  mesh.computeBoundingSphere();
  group.add(mesh);
  return mesh;
}

// ================================================================ draped geometry: roads, fields, patches
function insidePoly(X, Y, poly) {
  let inside = false;
  for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
    const [xi, yi] = poly[i], [xj, yj] = poly[j];
    if ((yi > Y) !== (yj > Y) && X < (xj - xi) * (Y - yi) / ((yj - yi) || 1e-9) + xi) inside = !inside;
  }
  return inside;
}
function distToPolyline(X, Y, pts) {
  let best = Infinity;
  for (let i = 0; i < pts.length - 1; i++) {
    const [ax, ay] = pts[i], [bx, by] = pts[i + 1], dx = bx - ax, dy = by - ay, l2 = dx * dx + dy * dy || 1;
    const t = clamp(((X - ax) * dx + (Y - ay) * dy) / l2, 0, 1);
    best = Math.min(best, Math.hypot(X - (ax + t * dx), Y - (ay + t * dy)));
  }
  return best;
}
function polylineLength(pts) { let l = 0; for (let i = 1; i < pts.length; i++) l += Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]); return l; }
// A parallel line d cm to the right of the path's direction.
function offsetPolyline(pts, d) {
  return pts.map((p, i) => {
    const a = pts[Math.max(0, i - 1)], b = pts[Math.min(pts.length - 1, i + 1)];
    const dx = b[0] - a[0], dy = b[1] - a[1], l = Math.hypot(dx, dy) || 1;
    return [p[0] - (dy / l) * d, p[1] + (dx / l) * d];
  });
}
// Calls fn(X, Y, yawDeg, k) every `every` cm along a polyline.
function alongLine(pts, every, fn, start = 0) {
  let acc = start, k = 0;
  for (let i = 0; i < pts.length - 1; i++) {
    const [ax, ay] = pts[i], [bx, by] = pts[i + 1], dx = bx - ax, dy = by - ay, len = Math.hypot(dx, dy);
    if (len < 1e-6) continue;
    const yaw = ueYawOf(dx, dy);
    let t = acc;
    while (t <= len) { fn(ax + dx * t / len, ay + dy * t / len, yaw, k++); t += every; }
    acc = t - len;
  }
}
function circlePoly(X, Y, r, n = 16, wobble = 0, seed = 1) {
  const rnd = mulberry32(seed), out = [];
  for (let i = 0; i < n; i++) { const a = i / n * Math.PI * 2, rr = r * (1 + (rnd() - 0.5) * wobble); out.push([X + Math.cos(a) * rr, Y + Math.sin(a) * rr]); }
  return out;
}
// A smooth curve through Unreal points, sampled every `step` meters, as three.js Vector3s (y = 0).
function curve3(ptsUE, step = 1.2) {
  const pts = ptsUE.map(([X, Y]) => new THREE.Vector3(toX(Y), 0, toZ(X)));
  if (pts.length < 2) return pts;
  if (pts.length === 2) { const n = Math.max(2, Math.ceil(pts[0].distanceTo(pts[1]) / step)); return Array.from({ length: n + 1 }, (_, i) => pts[0].clone().lerp(pts[1], i / n)); }
  const curve = new THREE.CatmullRomCurve3(pts, false, 'centripetal', 0.5);
  return curve.getSpacedPoints(Math.max(2, Math.ceil(curve.getLength() / step)));
}
// A strip draped over the ground between offsets left..right (m) of a sampled curve.
function ribbon(samples, left, right, hex, lift, flag = 1) {
  const pos = [], col = [], c = C(hex), n = samples.length;
  for (let i = 0; i < n; i++) {
    const p = samples[i], a = samples[Math.max(0, i - 1)], b = samples[Math.min(n - 1, i + 1)];
    const tx = b.x - a.x, tz = b.z - a.z, tl = Math.hypot(tx, tz) || 1, nx = -tz / tl, nz = tx / tl;
    for (const off of [left, right]) {
      const x = p.x + nx * off, z = p.z + nz * off; let y = groundAt(x, z); if (Number.isNaN(y)) y = groundAt(p.x, p.z) || 0;
      pos.push(x, y + lift, z); col.push(c.r, c.g, c.b, flag);
    }
  }
  const idx = [];
  for (let i = 0; i < n - 1; i++) { const a = i * 2; idx.push(a, a + 1, a + 2, a + 1, a + 3, a + 2); }
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('color', new THREE.Float32BufferAttribute(col, 4));
  g.setIndex(idx);
  return faceUp(g.toNonIndexed());
}
// Turns every triangle of a draped (non-indexed, one-color) geometry to face up.
function faceUp(g) {
  g.computeVertexNormals();
  const p = g.attributes.position, nrm = g.attributes.normal;
  for (let i = 0; i < p.count; i += 3) {
    if (nrm.getY(i) < 0) { const x = p.getX(i + 1), y = p.getY(i + 1), z = p.getZ(i + 1); p.setXYZ(i + 1, p.getX(i + 2), p.getY(i + 2), p.getZ(i + 2)); p.setXYZ(i + 2, x, y, z); }
  }
  g.computeVertexNormals();
  return g;
}
// A polygon (Unreal points) draped over the ground: cut along a grid of cells so it bends with the terrain, with each
// boundary cell clipped to the polygon exactly, so edges stay smooth and neighbouring cells share their vertices.
function clipToCell(poly, x0, y0, x1, y1) {
  let out = poly;
  const cut = (inside, at) => {
    const res = [];
    for (let i = 0; i < out.length; i++) { const a = out[i], b = out[(i + 1) % out.length], ia = inside(a), ib = inside(b); if (ia) res.push(a); if (ia !== ib) res.push(at(a, b)); }
    out = res;
  };
  const onX = (v) => (a, b) => { const t = (v - a[0]) / (b[0] - a[0]); return [v, a[1] + (b[1] - a[1]) * t]; };
  const onY = (v) => (a, b) => { const t = (v - a[1]) / (b[1] - a[1]); return [a[0] + (b[0] - a[0]) * t, v]; };
  cut((p) => p[0] >= x0, onX(x0)); if (out.length) cut((p) => p[0] <= x1, onX(x1));
  if (out.length) cut((p) => p[1] >= y0, onY(y0)); if (out.length) cut((p) => p[1] <= y1, onY(y1));
  return out;
}
function patch(polyUE, hex, lift = 0.05, flag = 1, cell = 120) {
  const xs = polyUE.map((p) => p[0]), ys = polyUE.map((p) => p[1]);
  const gx0 = Math.floor(Math.min(...xs) / cell) * cell, gx1 = Math.max(...xs), gy0 = Math.floor(Math.min(...ys) / cell) * cell, gy1 = Math.max(...ys);
  const pos = [], col = [], c = C(hex);
  const push = (X, Y) => { const x = toX(Y), z = toZ(X); let y = groundAt(x, z); if (Number.isNaN(y)) y = 0; pos.push(x, y + lift, z); col.push(c.r, c.g, c.b, flag); };
  const full = cell * cell * 0.999;
  for (let X = gx0; X < gx1; X += cell) for (let Y = gy0; Y < gy1; Y += cell) {
    const piece = clipToCell(polyUE, X, Y, X + cell, Y + cell);
    if (piece.length < 3) continue;
    let area = 0; for (let i = 0; i < piece.length; i++) { const a = piece[i], b = piece[(i + 1) % piece.length]; area += a[0] * b[1] - b[0] * a[1]; }
    if (Math.abs(area) / 2 < 1) continue;
    if (Math.abs(area) / 2 >= full) { push(X, Y); push(X + cell, Y); push(X, Y + cell); push(X + cell, Y); push(X + cell, Y + cell); push(X, Y + cell); continue; }
    const tris = THREE.ShapeUtils.triangulateShape(piece.map(([a, b]) => new THREE.Vector2(a, b)), []);
    for (const t of tris) for (const k of t) push(piece[k][0], piece[k][1]);
  }
  if (!pos.length) return null;
  const g = new THREE.BufferGeometry();
  g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
  g.setAttribute('color', new THREE.Float32BufferAttribute(col, 4));
  return faceUp(g);
}
