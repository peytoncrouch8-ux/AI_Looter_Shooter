// The sky dome: a gradient (zenith, horizon, ground) that meets the height fog at the horizon, the sun's disc and glow,
// procedural clouds lit from the sun's side, stars, and celestial bodies (planets with bands and rings, cratered moons,
// a striped retro sun). Colours are what the style wants to see on screen: they are inverse tone mapped first.
import * as THREE from 'three';
import { NOISE, FOG } from './glsl.js';
import { SHARED } from './materials.js';
import { dir3, linColor, DEG } from './util.js';
import { inverseToneMap } from './tonecurve.js';

const MAXB = 4;
const KINDS = { planet: 0, moon: 1, sun: 2, retrosun: 3, striped: 3 };

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
uniform vec3 uZenith, uHorizon, uGround, uSunCol;
uniform float uSunDisc, uSunGlow, uHorizonFog, uStars, uCapture, uBright, uPixAngle;
uniform vec4 uClouds;     // amount, scale, sharpness, speed
uniform vec3 uCloudCol, uCloudShade;
uniform int uBodyN;
uniform vec3 uBodyDir[${MAXB}];
uniform vec3 uBodyCol[${MAXB}];
uniform vec3 uBodyCol2[${MAXB}];
uniform vec4 uBodyP[${MAXB}];     // angular radius (rad), kind, stripes/detail, glow
uniform vec4 uRingCol[${MAXB}];   // rgb, on
uniform vec4 uRingP[${MAXB}];     // tilt (rad), inner, outer, roll (rad)
${NOISE}
${FOG}

vec3 slBody(int i, vec3 d, vec3 col, inout float occl) {
  vec3 b = uBodyDir[i];
  vec4 P = uBodyP[i];
  float cb = dot(d, b);
  // only directions near the body: its glow and ring reach a few radii, and the projection below (divided by cb)
  // blows up toward 90 degrees from it
  float reach = min(P.x * max(uRingP[i].z, 1.0) * 4.0 + 0.05, 1.2);
  if (cb < cos(reach)) return col;
  vec3 e = normalize(cross(abs(b.y) > 0.999 ? vec3(1.0, 0.0, 0.0) : vec3(0.0, 1.0, 0.0), b));
  vec3 n = cross(b, e);
  float tr = tan(P.x);
  vec2 q = vec2(dot(d, e), dot(d, n)) / cb / tr;
  float roll = uRingP[i].w;
  q = mat2(cos(roll), sin(roll), -sin(roll), cos(roll)) * q;
  float r2 = dot(q, q);
  // the edge's anti-aliasing from the pixel's angular size (no derivatives: this runs in a branch)
  float aa = uPixAngle / (tr * cb * cb) * 1.5 + 1e-4;
  int kind = int(P.y + 0.5);
  float disc = 1.0 - smoothstep(1.0 - aa, 1.0 + aa, sqrt(r2));
  vec3 bodyCol = col;
  if (kind == 2 || kind == 3) {
    // a sun: hot disc, limb darkening; kind 3 is the striped retro sun (gradient, gaps toward its foot)
    float glow = exp(-max(sqrt(r2) - 1.0, 0.0) * 2.2) * P.w;
    vec3 c = uBodyCol[i];
    if (kind == 3) {
      c = mix(uBodyCol2[i], uBodyCol[i], smoothstep(-1.0, 0.9, q.y));
      float stripes = max(P.z, 1.0);
      float y = (q.y + 1.0) * 0.5;
      float band = fract(y * stripes);
      float gap = (1.0 - smoothstep(0.0, 0.62, y)) * 0.55;
      float g = smoothstep(gap - 0.02, gap + 0.02, band);
      disc *= (y < 0.62) ? g : 1.0;
    } else {
      c *= 1.0 - 0.35 * r2;
    }
    col += uBodyCol[i] * glow * 0.35 * (1.0 - disc);
    col = mix(col, c * 6.0, disc);
    occl = max(occl, disc);
    return col;
  }
  // planets and moons: a lit sphere
  float z = sqrt(max(1.0 - r2, 0.0));
  vec3 Nw = normalize(e * q.x + n * q.y - b * z);
  float lit = dot(Nw, uSlSunDirW);
  float day = smoothstep(-0.12, 0.25, lit);
  vec3 surf = uBodyCol[i];
  if (kind == 0) {
    float lat = q.y * 6.0 + slFbm(vec2(q.x * 2.0, q.y * 7.0) + float(i) * 3.1) * 1.6;
    float bands = 0.5 + 0.5 * sin(lat * max(P.z, 1.0) * 0.5);
    surf = mix(uBodyCol[i], uBodyCol2[i], bands * 0.65);
  } else {
    float cr = slFbm(q * 3.5 + float(i) * 7.0);
    float cr2 = slFbm(q * 9.0 + 2.0);
    surf = mix(uBodyCol2[i], uBodyCol[i], smoothstep(0.35, 0.65, cr)) * (0.85 + 0.3 * cr2);
  }
  vec3 shaded = surf * (0.04 + 1.25 * day * (0.35 + 0.65 * max(lit, 0.0)));
  // a thin bright atmosphere at the limb on the day side
  float rim = smoothstep(0.75, 1.0, sqrt(r2)) * day * P.w;
  shaded += uBodyCol[i] * rim * 0.6;
  bodyCol = shaded;
  // the ring: an ellipse around it, its near half in front of the disc
  vec4 RP = uRingP[i];
  if (uRingCol[i].a > 0.5) {
    float st = max(sin(RP.x), 0.02);
    vec2 rq = vec2(q.x, q.y / st);
    float rr = length(rq);
    float inRing = smoothstep(RP.y, RP.y + 0.04, rr) * (1.0 - smoothstep(RP.z - 0.04, RP.z, rr));
    if (inRing > 0.0) {
      float t = (rr - RP.y) / max(RP.z - RP.y, 1e-3);
      float ringTex = 0.55 + 0.45 * sin(t * 37.0 + slVNoise(vec2(t * 60.0, 0.0)) * 3.0);
      ringTex *= 1.0 - 0.6 * smoothstep(0.42, 0.47, t) * (1.0 - smoothstep(0.5, 0.55, t));
      bool front = q.y < 0.0;
      // the planet's shadow across the ring, on the side away from the sun
      vec3 rpos = e * q.x + n * q.y;
      float shadowed = (dot(normalize(rpos), uSlSunDirW) < -0.2 && length(q.x) < 1.0) ? 0.35 : 1.0;
      vec3 rc = uRingCol[i].rgb * ringTex * shadowed;
      float ra = inRing * (0.55 + 0.45 * ringTex);
      if (front || disc < 0.5) { bodyCol = mix(disc > 0.5 ? bodyCol : col, rc, ra); disc = max(disc, ra); }
    }
  }
  col = mix(col, bodyCol, disc);
  // a soft glow around big bodies
  col += uBodyCol[i] * exp(-max(sqrt(r2) - 1.0, 0.0) * 5.0) * (1.0 - disc) * 0.15 * P.w * day;
  occl = max(occl, disc);
  return col;
}

void main() {
  vec3 d = normalize(vDir);
  float h = d.y;
  vec3 col;
  float up = max(h, 0.0);
  col = mix(uHorizon, uZenith, pow(smoothstep(0.0, 1.0, up), 0.55));
  col = mix(col, uGround, smoothstep(0.0, -0.08, h));
  // the haze at the horizon is the fog's own colour, so far hills melt into the sky
  vec3 fogC = slFogColorFor(d);
  col = mix(col, fogC, uHorizonFog * (1.0 - smoothstep(-0.02, 0.22, h)));
  col *= uBright;
  float occl = 0.0;
  // stars
  if (uStars > 0.0 && h > 0.0) {
    vec3 sp = d * 240.0;
    vec3 cell = floor(sp);
    float hs = slHash13(cell);
    if (hs > 0.985) {
      vec3 c = cell + vec3(slHash13(cell + 1.7), slHash13(cell + 3.1), slHash13(cell + 5.3));
      float dist = length(sp - c);
      float tw = 0.7 + 0.3 * sin(uSlTime * (2.0 + hs * 5.0) + hs * 50.0);
      float s = smoothstep(0.09, 0.0, dist) * (hs - 0.985) / 0.015 * tw;
      col += vec3(0.9, 0.95, 1.0) * s * uStars * 3.0 * smoothstep(0.0, 0.15, h);
    }
  }
  // bodies
  for (int i = 0; i < ${MAXB}; i++) { if (i >= uBodyN) break; col = slBody(i, d, col, occl); }
  // the sun
  float cs = dot(d, uSlSunDirW);
  float sunR = 0.0085;
  float sunAA = fwidth(cs) * 2.0 + 1e-6;
  float disc = smoothstep(cos(sunR) - sunAA, cos(sunR) + sunAA, cs) * (1.0 - occl) * (1.0 - uCapture);
  col += uSunCol * (pow(max(cs, 0.0), 6.0) * 0.18 + pow(max(cs, 0.0), 60.0) * 0.35 + pow(max(cs, 0.0), 900.0) * 1.2) * uSunGlow;
  // clouds: an overhead layer seen through the dome, lit from the sun's side
  if (uClouds.x > 0.0 && h > -0.02) {
    vec2 drift = vec2(uSlTime * uClouds.w, uSlTime * uClouds.w * 0.37);
    vec2 p = d.xz / (max(h, 0.0) + 0.12) * 0.32 * uClouds.y + drift;
    float n = slFbm(p);
    vec2 toSun = normalize(uSlSunDirW.xz + 1e-4);
    float ns = slFbm(p + toSun * 0.09);
    float cover = 1.0 - uClouds.x;
    float soft = mix(0.32, 0.02, uClouds.z);
    float dens = smoothstep(cover - soft, cover + soft, n) * smoothstep(-0.02, 0.1, h);
    float lit = clamp(0.55 + (n - ns) * 4.0, 0.0, 1.0);
    vec3 cc = mix(uCloudShade, uCloudCol, lit);
    float edge = (1.0 - smoothstep(cover, cover + soft * 3.0 + 0.1, n));
    cc += uSunCol * pow(max(cs, 0.0), 8.0) * edge * 0.5 * uSunGlow;
    // far clouds take the horizon haze
    cc = mix(cc, fogC * uBright, uHorizonFog * (1.0 - smoothstep(0.0, 0.25, h)) * 0.8);
    col = mix(col, cc * uBright, dens * (1.0 - occl * 0.0));
    disc *= 1.0 - dens;
  }
  col += uSunCol * disc * 40.0 * uSunDisc;
  gl_FragColor = vec4(col, 1.0);
}`;

export class Sky {
  constructor() {
    this.uniforms = Object.assign({
      uZenith: { value: new THREE.Color() }, uHorizon: { value: new THREE.Color() }, uGround: { value: new THREE.Color() },
      uSunCol: { value: new THREE.Color() }, uSunDisc: { value: 1 }, uSunGlow: { value: 1 }, uHorizonFog: { value: 0.6 },
      uStars: { value: 0 }, uCapture: { value: 0 }, uBright: { value: 1 }, uPixAngle: { value: 0.001 },
      uClouds: { value: new THREE.Vector4(0.4, 1, 0.5, 0.01) },
      uCloudCol: { value: new THREE.Color(1, 1, 1) }, uCloudShade: { value: new THREE.Color(0.7, 0.7, 0.75) },
      uBodyN: { value: 0 },
      uBodyDir: { value: Array.from({ length: MAXB }, () => new THREE.Vector3(0, 1, 0)) },
      uBodyCol: { value: Array.from({ length: MAXB }, () => new THREE.Color()) },
      uBodyCol2: { value: Array.from({ length: MAXB }, () => new THREE.Color()) },
      uBodyP: { value: Array.from({ length: MAXB }, () => new THREE.Vector4()) },
      uRingCol: { value: Array.from({ length: MAXB }, () => new THREE.Vector4()) },
      uRingP: { value: Array.from({ length: MAXB }, () => new THREE.Vector4()) },
    }, SHARED);
    this.material = new THREE.ShaderMaterial({
      uniforms: this.uniforms, vertexShader: VS, fragmentShader: FS, side: THREE.BackSide, depthWrite: false, depthTest: true,
      toneMapped: false, fog: false,
    });
    this.mesh = new THREE.Mesh(new THREE.SphereGeometry(1, 64, 32), this.material);
    this.mesh.frustumCulled = false;
    this.mesh.renderOrder = -100;
    this.mesh.userData.noNormal = true;
    this.mesh.name = 'SkyDome';
    this.custom = null;
    this.radius = 5000;
  }

  // env.sky plus the style's tone mapping (to invert) and the sun.
  set(sky = {}, env = {}, sunColor, sunIntensity) {
    const op = env.toneMapping || 'aces', ex = env.exposure ?? 1;
    const inv = (hex, fb) => inverseToneMap(linColor(hex, fb), op, ex);
    const u = this.uniforms;
    u.uZenith.value.copy(inv(sky.zenith, '#3d6fb0'));
    u.uHorizon.value.copy(inv(sky.horizon, '#f0c79a'));
    u.uGround.value.copy(inv(sky.ground, '#8a7a66'));
    u.uSunDisc.value = sky.sunDisc ?? 1;
    u.uSunGlow.value = sky.sunGlow ?? 1;
    u.uHorizonFog.value = sky.horizonFog ?? 0.65;
    u.uStars.value = sky.stars ?? 0;
    u.uBright.value = sky.brightness ?? 1;
    u.uSunCol.value.copy(sunColor).multiplyScalar(Math.max(0.2, sunIntensity) * 0.3);
    const c = sky.clouds || {};
    u.uClouds.value.set(c.amount ?? 0, c.scale ?? 1, c.sharpness ?? 0.5, c.speed ?? 0.01);
    u.uCloudCol.value.copy(inv(c.color, '#ffffff'));
    u.uCloudShade.value.copy(inv(c.shade, '#a0a8b8'));
    const bodies = (sky.bodies || []).slice(0, MAXB);
    u.uBodyN.value = bodies.length;
    bodies.forEach((b, i) => {
      dir3(b.azimuth ?? 0, b.elevation ?? 30, u.uBodyDir.value[i]);
      const kind = KINDS[b.kind] ?? 0;
      const base = linColor(b.color || '#c99a7a');
      u.uBodyCol.value[i].copy(kind >= 2 ? base : inv(b.color || '#c99a7a'));
      const c2 = b.color2 || b.bands || null;
      u.uBodyCol2.value[i].copy(c2 ? (kind >= 2 ? linColor(c2) : inv(c2)) : u.uBodyCol.value[i].clone().multiplyScalar(kind === 1 ? 0.55 : 0.7));
      u.uBodyP.value[i].set((b.size ?? 10) * 0.5 * DEG, kind, b.stripes ?? (kind === 0 ? 9 : 7), b.glow ?? 1);
      const ring = b.ring;
      if (ring) {
        u.uRingCol.value[i].set(...inv(ring.color || '#e9d3b0').toArray(), 1);
        u.uRingP.value[i].set((ring.tilt ?? 18) * DEG, ring.inner ?? 1.4, ring.outer ?? 2.3, (ring.roll ?? b.roll ?? 0) * DEG);
      } else {
        u.uRingCol.value[i].set(0, 0, 0, 0);
        u.uRingP.value[i].set(0.3, 1.4, 2.3, (b.roll ?? 0) * DEG);
      }
    });
  }

  follow(camera, heightPx = 1080) {
    this.uniforms.uPixAngle.value = (camera.fov * Math.PI / 180) / Math.max(1, heightPx);
    const r = Math.min(camera.far * 0.9, this.radius);
    const target = this.custom && this.custom.isMesh ? this.custom : this.mesh;
    target.position.setFromMatrixPosition(camera.matrixWorld);
    if (target === this.mesh) target.scale.setScalar(r);
  }

  // The sky alone, for the environment map (the sun's disc left out: the direct light already draws its highlight).
  captureScene() {
    if (!this._capture) {
      this._capture = new THREE.Scene();
      this._captureMesh = new THREE.Mesh(new THREE.SphereGeometry(40, 48, 24), this.material);
      this._capture.add(this._captureMesh);
    }
    this._captureMesh.material = this.custom && this.custom.isMaterial ? this.custom : this.material;
    return this._capture;
  }
}

// The environment map for reflections, from the sky (PMREM).
export function captureEnv(renderer, sky, pmrem, prev) {
  const u = sky.uniforms;
  u.uCapture.value = 1;
  const pa = u.uPixAngle.value;
  u.uPixAngle.value = (Math.PI / 2) / 256;
  const rt = pmrem.fromScene(sky.captureScene(), 0.02, 0.1, 100);
  u.uPixAngle.value = pa;
  u.uCapture.value = 0;
  if (prev) prev.dispose();
  return rt;
}
