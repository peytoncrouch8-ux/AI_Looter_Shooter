// Effects: CPU particles drawn as camera-facing (or velocity-stretched) sprites in one additive and one alpha batch
// (muzzle sparks, tracers, impact sparks, dust, blood), the muzzle flash on the gun, chimney smoke computed on the GPU
// from time alone (so stills are deterministic), and ambient dust motes and embers wrapped around the camera.
import * as THREE from 'three';
import { FOG, NOISE } from './glsl.js';
import { SHARED } from './materials.js';
import { GUN_LAYER } from './lighting.js';
import { linColor, mulberry32 } from './util.js';

const MAX = 1200;

const SPRITE_VS = /* glsl */`
attribute vec3 iPos;
attribute vec3 iVel;
attribute vec4 iCol;
attribute vec4 iPar;   // size, stretch (s), kind, rotation
varying vec2 vUv;
varying vec4 vCol;
varying float vKind;
varying vec3 vWorld;
void main() {
  vUv = uv;
  vCol = iCol;
  vKind = iPar.z;
  vec3 camRight = vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
  vec3 camUp = vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
  vec3 p = iPos;
  vec2 c = (uv - 0.5) * 2.0;
  float size = iPar.x;
  if (iPar.y > 0.0 && length(iVel) > 1e-4) {
    // stretched along its velocity, as seen on screen
    vec3 v = iVel * iPar.y;
    vec3 toCam = normalize(cameraPosition - p);
    vec3 side = normalize(cross(v, toCam)) * size;
    p += v * (c.y * 0.5 - 0.5) + side * c.x;
  } else {
    float r = iPar.w;
    vec2 rc = vec2(c.x * cos(r) - c.y * sin(r), c.x * sin(r) + c.y * cos(r));
    p += (camRight * rc.x + camUp * rc.y) * size;
  }
  vWorld = p;
  gl_Position = projectionMatrix * viewMatrix * vec4(p, 1.0);
}`;
const SPRITE_FS = /* glsl */`
varying vec2 vUv;
varying vec4 vCol;
varying float vKind;
varying vec3 vWorld;
uniform float uAdditive;
${NOISE}
${FOG}
void main() {
  vec2 c = (vUv - 0.5) * 2.0;
  float r = length(c);
  float a;
  int k = int(vKind + 0.5);
  if (k == 1) a = (1.0 - smoothstep(0.0, 1.0, abs(c.x))) * (1.0 - smoothstep(0.6, 1.0, abs(c.y)));      // streak
  else if (k == 2) a = (1.0 - smoothstep(0.2, 1.0, r)) * (0.6 + 0.4 * slVNoise(c * 3.0 + vCol.a * 10.0)); // puff
  else if (k == 3) { float ang = atan(c.y, c.x); a = (1.0 - smoothstep(0.0, 0.9 + 0.25 * sin(ang * 5.0), r)); a = a * a; } // flash star
  else a = pow(max(1.0 - r, 0.0), 2.0);                                                           // soft dot
  float f = slFogAmount(cameraPosition, vWorld);
  if (uAdditive > 0.5) {
    gl_FragColor = vec4(vCol.rgb * a * vCol.a * (1.0 - f), 1.0);
  } else {
    vec3 col = mix(vCol.rgb, slFogColorFor(normalize(vWorld - cameraPosition)), f);
    gl_FragColor = vec4(col, a * vCol.a);
  }
}`;

class SpriteBatch {
  constructor(additive) {
    const geo = new THREE.InstancedBufferGeometry();
    const quad = new THREE.PlaneGeometry(1, 1);
    geo.index = quad.index; geo.attributes.position = quad.attributes.position; geo.attributes.uv = quad.attributes.uv;
    this.pos = new Float32Array(MAX * 3); this.vel = new Float32Array(MAX * 3); this.col = new Float32Array(MAX * 4); this.par = new Float32Array(MAX * 4);
    const mk = (a, n) => { const b = new THREE.InstancedBufferAttribute(a, n); b.setUsage(THREE.DynamicDrawUsage); return b; };
    geo.setAttribute('iPos', mk(this.pos, 3)); geo.setAttribute('iVel', mk(this.vel, 3));
    geo.setAttribute('iCol', mk(this.col, 4)); geo.setAttribute('iPar', mk(this.par, 4));
    geo.instanceCount = 0;
    this.geo = geo;
    this.material = new THREE.ShaderMaterial({
      uniforms: Object.assign({ uAdditive: { value: additive ? 1 : 0 } }, SHARED),
      vertexShader: SPRITE_VS, fragmentShader: SPRITE_FS, transparent: true, depthWrite: false, toneMapped: false,
      blending: additive ? THREE.AdditiveBlending : THREE.NormalBlending,
    });
    this.mesh = new THREE.Mesh(geo, this.material);
    this.mesh.frustumCulled = false;
    this.mesh.renderOrder = additive ? 20 : 10;
    this.mesh.userData.noNormal = true;
    this.parts = [];
  }
  add(p) { if (this.parts.length < MAX) this.parts.push(p); }
  update(dt) {
    const out = [];
    for (const p of this.parts) {
      p.age += dt;
      if (p.age >= p.life) continue;
      p.vel.y -= (p.gravity || 0) * dt;
      if (p.drag) p.vel.multiplyScalar(Math.exp(-p.drag * dt));
      p.pos.addScaledVector(p.vel, dt);
      if (p.floor !== undefined && p.pos.y < p.floor) { p.pos.y = p.floor; p.vel.y *= -0.3; p.vel.x *= 0.6; p.vel.z *= 0.6; }
      out.push(p);
    }
    this.parts = out;
  }
  upload() {
    const n = this.parts.length;
    for (let i = 0; i < n; i++) {
      const p = this.parts[i], k = p.age / p.life;
      const fade = p.fade ? p.fade(k) : (1 - k);
      this.pos.set([p.pos.x, p.pos.y, p.pos.z], i * 3);
      const v = p.fixedDir || p.vel;
      this.vel.set([v.x, v.y, v.z], i * 3);
      const c = p.color;
      this.col.set([c.r * (p.bright || 1), c.g * (p.bright || 1), c.b * (p.bright || 1), fade * (p.alpha ?? 1)], i * 4);
      this.par.set([p.size * (1 + (p.grow || 0) * k), p.stretch || 0, p.kind || 0, p.rot || 0], i * 4);
    }
    this.geo.instanceCount = n;
    for (const a of ['iPos', 'iVel', 'iCol', 'iPar']) this.geo.attributes[a].needsUpdate = true;
  }
}

// Chimney smoke: each puff's age comes from the time, so the same time gives the same frame.
const SMOKE_VS = /* glsl */`
attribute vec4 iSrc;    // source position, seed
uniform float uSlAnimTime;
uniform vec4 uSlWind;
varying vec2 vUv;
varying float vAge;
varying vec3 vWorld;
varying float vSeed;
void main() {
  float life = 11.0;
  float seed = iSrc.w;
  float age = mod(uSlAnimTime + seed * life, life) / life;
  vAge = age; vUv = uv; vSeed = seed;
  vec3 p = iSrc.xyz;
  // rises fast out of the chimney, slows, and leans off downwind as it spreads
  p.y += 5.5 * (1.0 - exp(-age * 2.2)) + age * 2.0;
  p.xz += uSlWind.xy * (age * age * 10.0 + age * 1.5);
  p.x += sin(seed * 40.0 + age * 4.0) * age * 1.2;
  p.z += cos(seed * 31.0 + age * 3.0) * age * 1.2;
  float size = 0.35 + pow(age, 0.7) * 3.4;
  vec3 camRight = vec3(viewMatrix[0][0], viewMatrix[1][0], viewMatrix[2][0]);
  vec3 camUp = vec3(viewMatrix[0][1], viewMatrix[1][1], viewMatrix[2][1]);
  vec2 c = (uv - 0.5) * 2.0;
  float r = seed * 6.28 + age * 0.8;
  vec2 rc = vec2(c.x * cos(r) - c.y * sin(r), c.x * sin(r) + c.y * cos(r));
  p += (camRight * rc.x + camUp * rc.y) * size;
  vWorld = p;
  gl_Position = projectionMatrix * viewMatrix * vec4(p, 1.0);
}`;
const SMOKE_FS = /* glsl */`
varying vec2 vUv; varying float vAge; varying vec3 vWorld; varying float vSeed;
uniform vec3 uLit, uShade; uniform float uOpacity;
${NOISE}
${FOG}
void main() {
  vec2 c = (vUv - 0.5) * 2.0;
  float n = slFbm(c * 1.8 + vSeed * 13.0 + vAge * 1.5);
  float a = (1.0 - smoothstep(0.05, 1.0, length(c) * 1.1 + (n - 0.5) * 0.9));
  a *= smoothstep(0.0, 0.06, vAge) * (1.0 - smoothstep(0.25, 1.0, vAge)) * uOpacity;
  float lit = clamp(0.5 + dot(normalize(vec3(c.x, c.y, 0.6)), normalize(vec3(0.3, 0.8, 0.3))) * 0.5, 0.0, 1.0);
  vec3 col = mix(uShade, uLit, lit * (0.6 + 0.4 * n));
  float f = slFogAmount(cameraPosition, vWorld);
  col = mix(col, slFogColorFor(normalize(vWorld - cameraPosition)), f);
  gl_FragColor = vec4(col, a);
}`;

// Ambient motes and embers, wrapped in a box around the camera.
const MOTE_VS = /* glsl */`
attribute vec4 aSeed;
uniform float uSlAnimTime;
uniform vec3 uBox;
uniform vec3 uCam;
uniform float uKind;   // 0 dust motes, 1 embers
uniform float uPix;
varying float vFade;
varying float vTw;
varying vec3 vWorld;
void main() {
  vec3 drift = uKind > 0.5 ? vec3(0.25, 0.9 + aSeed.w * 0.6, 0.15) : vec3(0.18, 0.05, 0.12);
  vec3 wob = vec3(sin(uSlAnimTime * (0.4 + aSeed.w) + aSeed.x * 20.0), sin(uSlAnimTime * 0.5 + aSeed.y * 30.0), cos(uSlAnimTime * (0.35 + aSeed.w * 0.5) + aSeed.z * 25.0)) * 0.6;
  vec3 p = aSeed.xyz * uBox + drift * uSlAnimTime + wob;
  p = mod(p - uCam + uBox * 0.5, uBox) - uBox * 0.5 + uCam;
  vec3 rel = (p - uCam) / (uBox * 0.5);
  vFade = 1.0 - smoothstep(0.6, 1.0, max(max(abs(rel.x), abs(rel.y)), abs(rel.z)));
  vTw = 0.6 + 0.4 * sin(uSlAnimTime * (3.0 + aSeed.w * 5.0) + aSeed.x * 50.0);
  vWorld = p;
  vec4 mv = viewMatrix * vec4(p, 1.0);
  gl_Position = projectionMatrix * mv;
  float s = uKind > 0.5 ? 0.05 : 0.03;
  gl_PointSize = clamp(s * uPix / -mv.z, 1.0, 14.0);
}`;
const MOTE_FS = /* glsl */`
varying float vFade; varying float vTw; varying vec3 vWorld;
uniform vec3 uColor; uniform float uAmount; uniform float uKind;
${NOISE}
${FOG}
void main() {
  vec2 c = gl_PointCoord * 2.0 - 1.0;
  float a = max(1.0 - dot(c, c), 0.0);
  a *= a;
  vec3 v = normalize(vWorld - cameraPosition);
  float toward = uKind > 0.5 ? 1.0 : 0.35 + 1.6 * pow(max(dot(v, uSlSunDirW), 0.0), 3.0);
  float f = slFogAmount(cameraPosition, vWorld);
  gl_FragColor = vec4(uColor * a * vFade * vTw * toward * uAmount * (1.0 - f), 1.0);
}`;

export class Effects {
  constructor(engine) {
    this.e = engine;
    this.root = new THREE.Group(); this.root.name = 'Effects';
    this.add = new SpriteBatch(true); this.alpha = new SpriteBatch(false);
    this.root.add(this.add.mesh, this.alpha.mesh);
    this.colors = { muzzle: linColor('#ffd27a'), tracer: linColor('#ffe6a8'), impact: linColor('#ffcf7a'), blood: linColor('#9be35a'), hitFlash: linColor('#ffffff') };
    this.rand = mulberry32(99);
    this.flash = null;
    this._motes();
  }

  setColors(fx = {}) {
    for (const k of Object.keys(this.colors)) if (fx[k]) this.colors[k] = linColor(fx[k]);
    if (this.flashMat) this.flashMat.uniforms.uColor.value.copy(this.colors.muzzle);
    this.e.lighting.muzzle.color.copy(this.colors.muzzle);
  }

  // env.particles: { dust, embers, emberColor, motes }
  setParticles(p = {}) {
    this.dust.material.uniforms.uAmount.value = (p.dust ?? 0.5) * 1.0;
    this.dust.material.uniforms.uColor.value.copy(linColor(p.motes || '#ffe2b0')).multiplyScalar(0.8);
    this.dust.visible = (p.dust ?? 0.5) > 0.001;
    this.embers.material.uniforms.uAmount.value = (p.embers ?? 0) * 1.0;
    this.embers.material.uniforms.uColor.value.copy(linColor(p.emberColor || '#ff9a3a')).multiplyScalar(6.0);
    this.embers.visible = (p.embers ?? 0) > 0.001;
  }

  _motes() {
    const mk = (count, kind, box) => {
      const g = new THREE.BufferGeometry();
      const seeds = new Float32Array(count * 4), r = mulberry32(kind ? 77 : 55);
      for (let i = 0; i < count * 4; i++) seeds[i] = r();
      g.setAttribute('aSeed', new THREE.BufferAttribute(seeds, 4));
      g.setAttribute('position', new THREE.BufferAttribute(new Float32Array(count * 3), 3));
      const m = new THREE.ShaderMaterial({
        uniforms: Object.assign({ uBox: { value: new THREE.Vector3(...box) }, uCam: { value: new THREE.Vector3() }, uKind: { value: kind },
          uColor: { value: new THREE.Color(1, 0.9, 0.7) }, uAmount: { value: 0.5 }, uPix: { value: 800 } }, SHARED),
        vertexShader: MOTE_VS, fragmentShader: MOTE_FS, transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, toneMapped: false,
      });
      const pts = new THREE.Points(g, m);
      pts.frustumCulled = false; pts.userData.noNormal = true; pts.renderOrder = 30;
      this.root.add(pts);
      return pts;
    };
    this.dust = mk(1600, 0, [30, 12, 30]);
    this.embers = mk(700, 1, [40, 18, 40]);
  }

  buildSmoke(sources) {
    const per = 26;
    const n = sources.length * per;
    if (!n) return;
    const geo = new THREE.InstancedBufferGeometry();
    const quad = new THREE.PlaneGeometry(1, 1);
    geo.index = quad.index; geo.attributes.position = quad.attributes.position; geo.attributes.uv = quad.attributes.uv;
    const src = new Float32Array(n * 4);
    sources.forEach((s, i) => { for (let k = 0; k < per; k++) src.set([s.p[0], s.p[1], s.p[2], (k + this.rand() * 0.5) / per], (i * per + k) * 4); });
    geo.setAttribute('iSrc', new THREE.InstancedBufferAttribute(src, 4));
    geo.instanceCount = n;
    this.smokeMat = new THREE.ShaderMaterial({
      uniforms: Object.assign({ uLit: { value: new THREE.Color(0.8, 0.78, 0.75) }, uShade: { value: new THREE.Color(0.35, 0.36, 0.4) }, uOpacity: { value: 0.22 } }, SHARED),
      vertexShader: SMOKE_VS, fragmentShader: SMOKE_FS, transparent: true, depthWrite: false, toneMapped: false,
    });
    const m = new THREE.Mesh(geo, this.smokeMat);
    m.frustumCulled = false; m.userData.noNormal = true; m.renderOrder = 5;
    this.root.add(m);
    this.smoke = m;
  }

  // Smoke takes the sun's and the sky's light.
  lightSmoke(sunColor, sunIntensity, hemi, opacity) {
    if (!this.smokeMat) return;
    const u = this.smokeMat.uniforms;
    u.uLit.value.copy(sunColor).multiplyScalar(sunIntensity * 0.16).add(hemi.color.clone().multiplyScalar(hemi.intensity * 0.3));
    u.uShade.value.copy(hemi.color).multiplyScalar(hemi.intensity * 0.3).add(hemi.groundColor.clone().multiplyScalar(hemi.intensity * 0.15));
    u.uOpacity.value = opacity ?? 0.22;
  }

  // The flash on the gun's muzzle: three crossed star cards, additive, in the gun's layer.
  makeFlash(parent, at) {
    const g = new THREE.BufferGeometry();
    const cards = [];
    const add = (rotZ, axis) => {
      const p = new THREE.PlaneGeometry(1, 1);
      if (axis === 'y') p.rotateY(Math.PI / 2);
      if (axis === 'x') p.rotateX(Math.PI / 2);
      p.rotateZ(rotZ);
      cards.push(p);
    };
    add(0, 'z'); add(0, 'y'); add(0, 'x');
    const merged = new THREE.BufferGeometry();
    const pos = [], uv = [], idx = [];
    cards.forEach((c, i) => {
      const o = pos.length / 3;
      pos.push(...c.attributes.position.array); uv.push(...c.attributes.uv.array);
      idx.push(...Array.from(c.index.array).map((v) => v + o));
    });
    merged.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    merged.setAttribute('uv', new THREE.Float32BufferAttribute(uv, 2));
    merged.setIndex(idx);
    this.flashMat = new THREE.ShaderMaterial({
      uniforms: { uColor: { value: this.colors.muzzle.clone() }, uI: { value: 0 }, uSeed: { value: 0 } },
      vertexShader: 'varying vec2 vUv; varying vec3 vP; void main(){ vUv = uv; vP = position; gl_Position = projectionMatrix * modelViewMatrix * vec4(position, 1.0); }',
      fragmentShader: /* glsl */`varying vec2 vUv; varying vec3 vP; uniform vec3 uColor; uniform float uI, uSeed;
        void main(){ vec2 c = (vUv - 0.5) * 2.0; float r = length(c); float ang = atan(c.y, c.x);
          float star = 1.0 - smoothstep(0.0, 0.35 + 0.55 * pow(abs(sin(ang * 3.0 + uSeed * 6.0)), 6.0), r);
          float core = 1.0 - smoothstep(0.0, 0.35, r);
          float a = (star * 0.8 + core) * uI;
          gl_FragColor = vec4(uColor * a * 6.0 + vec3(1.0, 0.95, 0.85) * core * uI * 4.0, 1.0); }`,
      transparent: true, depthWrite: false, blending: THREE.AdditiveBlending, side: THREE.DoubleSide, toneMapped: false,
    });
    const m = new THREE.Mesh(merged, this.flashMat);
    m.layers.set(GUN_LAYER);
    m.position.copy(at);
    m.scale.set(0.16, 0.16, 0.28);
    m.frustumCulled = false; m.userData.noNormal = true; m.renderOrder = 50;
    m.visible = false;
    parent.add(m);
    this.flash = m;
    return m;
  }

  muzzleFlash(worldPos, intensity = 1) {
    this.flashT = 0.055;
    this.flashI = intensity;
    if (this.flash) {
      this.flashMat.uniforms.uSeed.value = this.rand();
      this.flash.rotation.z = this.rand() * Math.PI;
      this.flash.scale.set(0.13 + this.rand() * 0.06, 0.13 + this.rand() * 0.06, 0.24 + this.rand() * 0.1);
    }
    const L = this.e.lighting.muzzle;
    L.position.copy(worldPos).addScaledVector(this.e.camera.getWorldDirection(new THREE.Vector3()), 0.6);
    // sparks from the muzzle
    const dir = this.e.camera.getWorldDirection(new THREE.Vector3());
    for (let i = 0; i < 4; i++) {
      const v = dir.clone().multiplyScalar(8 + this.rand() * 8).add(new THREE.Vector3(this.rand() - 0.5, this.rand() - 0.5, this.rand() - 0.5).multiplyScalar(3));
      this.add.add({ pos: worldPos.clone(), vel: v, life: 0.06 + this.rand() * 0.05, age: 0, size: 0.006, stretch: 0.012, kind: 1, color: this.colors.muzzle, bright: 8, drag: 6 });
    }
  }

  tracer(from, to) {
    const d = to.clone().sub(from), len = d.length();
    if (len < 0.5) return;
    const dir = d.clone().normalize();
    const speed = 420;
    // it starts a little out from the muzzle, so it never streaks across the gun
    const start = Math.min(2.0, len * 0.4);
    const segLen = Math.min(len - start, 7);
    if (segLen <= 0.2) return;
    this.add.add({ pos: from.clone().addScaledVector(dir, start + segLen), vel: dir.clone().multiplyScalar(speed),
      life: Math.max(0.03, (len - start - segLen) / speed), age: 0, size: 0.011, stretch: segLen / speed, kind: 1,
      color: this.colors.tracer, bright: 9, fade: () => 1 });
  }

  impact(point, normal, kind = 'world', groundColor = null) {
    const n = normal.clone().normalize();
    const sparks = kind === 'flesh' ? 0 : 12;
    for (let i = 0; i < sparks; i++) {
      const v = n.clone().multiplyScalar(2 + this.rand() * 5).add(new THREE.Vector3(this.rand() - 0.5, this.rand() - 0.2, this.rand() - 0.5).multiplyScalar(6));
      this.add.add({ pos: point.clone().addScaledVector(n, 0.02), vel: v, life: 0.18 + this.rand() * 0.3, age: 0, size: 0.012, stretch: 0.03,
        kind: 1, color: this.colors.impact, bright: 7, gravity: 9.8, drag: 1.5, floor: point.y - 2 });
    }
    // a flash at the point (sparks only: flesh takes the blood puffs)
    if (kind !== 'flesh') this.add.add({ pos: point.clone().addScaledVector(n, 0.05), vel: new THREE.Vector3(), life: 0.07, age: 0, size: kind === 'flesh' ? 0.1 : 0.2, kind: 3,
      color: kind === 'flesh' ? this.colors.blood : this.colors.impact, bright: kind === 'flesh' ? 1.5 : 4, rot: this.rand() * 6 });
    // the puffs are unlit sprites: blood is dimmed to about what it would show in light
    const dustCol = kind === 'flesh' ? this.colors.blood.clone().multiplyScalar(0.45) : (groundColor || new THREE.Color(0.35, 0.3, 0.24));
    const puffs = kind === 'flesh' ? 4 : 5;
    for (let i = 0; i < puffs; i++) {
      const v = n.clone().multiplyScalar(0.6 + this.rand() * 1.4).add(new THREE.Vector3(this.rand() - 0.5, this.rand() * 0.6, this.rand() - 0.5));
      this.alpha.add({ pos: point.clone().addScaledVector(n, 0.08), vel: v, life: 0.6 + this.rand() * 0.7, age: 0, size: kind === 'flesh' ? 0.05 : 0.14,
        grow: kind === 'flesh' ? 1.2 : 4, kind: 2, color: dustCol, alpha: kind === 'flesh' ? 0.85 : 0.55, drag: 3, gravity: kind === 'flesh' ? 6 : -0.2,
        rot: this.rand() * 6, fade: (k) => (1 - k) * (1 - k) });
    }
    if (kind === 'flesh') {
      for (let i = 0; i < 10; i++) {
        const v = n.clone().multiplyScalar(1 + this.rand() * 3).add(new THREE.Vector3(this.rand() - 0.5, this.rand(), this.rand() - 0.5).multiplyScalar(3));
        this.alpha.add({ pos: point.clone(), vel: v, life: 0.4 + this.rand() * 0.3, age: 0, size: 0.02, stretch: 0.02, kind: 1, color: dustCol,
          alpha: 1, gravity: 9.8, drag: 1 });
      }
    }
  }

  update(dt, camera) {
    this.add.update(dt); this.alpha.update(dt);
    this.add.upload(); this.alpha.upload();
    const L = this.e.lighting.muzzle;
    if (this.flashT > 0) {
      this.flashT -= dt;
      const k = Math.max(0, this.flashT / 0.055);
      if (this.flash) { this.flash.visible = true; this.flashMat.uniforms.uI.value = k * this.flashI; }
      L.intensity = 25 * k * this.flashI;
    } else {
      if (this.flash) this.flash.visible = this.frozenFlash || false;
      if (!this.frozenFlash) L.intensity = 0;
    }
    const cp = camera.position;
    for (const m of [this.dust, this.embers]) {
      m.material.uniforms.uCam.value.copy(cp);
      m.material.uniforms.uPix.value = this.e.renderer.domElement.height / (2 * Math.tan(camera.fov * Math.PI / 360));
    }
  }

  clear() { this.add.parts = []; this.alpha.parts = []; this.flashT = 0; this.frozenFlash = false; }
}
