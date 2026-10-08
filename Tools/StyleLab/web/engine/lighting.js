// The sun with three shadow cascades fitted to the camera (bounding spheres, texel-snapped, so they don't shimmer),
// the sky's hemisphere ambient, and a fixed pool of point lights given to the lanterns nearest the camera.
import * as THREE from 'three';
import { SHARED } from './materials.js';
import { DEG, dir3, linColor, clamp } from './util.js';

export const GUN_LAYER = 1;

export class Lighting {
  constructor(scene, opts = {}) {
    this.scene = scene;
    this.cascadeCount = opts.cascades || 3;
    this.mapSize = opts.mapSize || 2048;
    this.sunDir = new THREE.Vector3(0, 1, 0);
    this.sunColor = new THREE.Color(1, 1, 1);
    this.sunIntensity = 3;
    this.softness = 1;
    this.shadowFar = 220;
    this.shadowNear = 0.1;
    this.lambda = 0.75;
    this.margin = 160;
    this.cascades = [];
    for (let i = 0; i < this.cascadeCount; i++) {
      const l = new THREE.DirectionalLight(0xffffff, 3);
      l.castShadow = true;
      l.shadow.mapSize.set(this.mapSize, this.mapSize);
      l.shadow.camera.near = 0.1;
      l.shadow.camera.far = 600;
      l.shadow.autoUpdate = true;
      l.layers.enable(GUN_LAYER);
      scene.add(l); scene.add(l.target);
      this.cascades.push({ light: l, far: 0, radius: 1, texel: 0.01 });
    }
    this.hemi = new THREE.HemisphereLight(0x8899bb, 0x665544, 0.6);
    this.hemi.layers.enable(GUN_LAYER);
    scene.add(this.hemi);
    // Lantern pool: a fixed count so the shaders never recompile as the player walks.
    this.poolSize = opts.pool || 6;
    this.pool = [];
    for (let i = 0; i < this.poolSize; i++) {
      const p = new THREE.PointLight(0xffb24d, 0, 8, 2);
      p.layers.enable(GUN_LAYER);
      scene.add(p);
      this.pool.push(p);
    }
    this.muzzle = new THREE.PointLight(0xffc070, 0, 9, 2);
    this.muzzle.layers.enable(GUN_LAYER);
    scene.add(this.muzzle);
    this.sources = [];       // the scene's point lights: { p: Vector3, color, intensity, range, note, phase }
    this.lightScale = 1;
    this.lightBase = 0.65;   // the scene's lamps are Unreal candela under a 6.5 lux sun; ours is about 4
    this._poolTimer = 0;
    this._tmp = new THREE.Vector3();
    this._corners = Array.from({ length: 8 }, () => new THREE.Vector3());
  }

  setSources(list) {
    this.sources = list.map((l, i) => ({
      p: new THREE.Vector3(...l.p), color: linColor(l.color || '#ffb24d'), intensity: l.intensity ?? 1, range: l.range || 8,
      note: l.note || '', phase: i * 1.37, scale: 1, colorOverride: null,
    }));
  }

  // env.sun: azimuth (UE yaw the light comes from), elevation (deg), color, intensity, shadowSoftness, shadowStrength
  setSun(sun = {}) {
    dir3(sun.azimuth ?? 250, sun.elevation ?? 20, this.sunDir);
    this.sunColor.copy(linColor(sun.color || '#ffffff'));
    this.sunIntensity = sun.intensity ?? 3;
    this.softness = sun.shadowSoftness ?? 1;
    SHARED.uSlShadowStrength.value = clamp(sun.shadowStrength ?? 1, 0, 1);
    SHARED.uSlSunDirW.value.copy(this.sunDir);
    for (const c of this.cascades) { c.light.color.copy(this.sunColor); c.light.intensity = this.sunIntensity; }
  }

  setAmbient(a = {}) {
    this.hemi.color.copy(linColor(a.sky || '#8899bb'));
    this.hemi.groundColor.copy(linColor(a.ground || '#665544'));
    this.hemi.intensity = a.intensity ?? 0.6;
  }

  setShadowRange(near, far, lambda = 0.75) { this.shadowNear = near; this.shadowFar = far; this.lambda = lambda; }

  _splits(camera) {
    const n = Math.max(camera.near, this.shadowNear), f = Math.min(camera.far, this.shadowFar), k = this.cascadeCount;
    const out = [];
    for (let i = 1; i <= k; i++) {
      const log = n * Math.pow(f / n, i / k), uni = n + (f - n) * i / k;
      out.push(i === k ? f : this.lambda * log + (1 - this.lambda) * uni);
    }
    return out;
  }

  // Each cascade is a sphere of its split distance around the camera (the shaders pick a cascade by distance, not view
  // depth), so it doesn't move as the view turns and is texel-snapped as the camera walks. The far cascades need
  // re-rendering only every few frames or after the camera has moved.
  update(camera, dt, force = false) {
    this.frame = (this.frame || 0) + 1;
    const splits = this._splits(camera);
    const camPos = new THREE.Vector3().setFromMatrixPosition(camera.matrixWorld);
    const L = this.sunDir;
    const lx = new THREE.Vector3().crossVectors(Math.abs(L.y) > 0.99 ? new THREE.Vector3(1, 0, 0) : new THREE.Vector3(0, 1, 0), L).normalize();
    const ly = new THREE.Vector3().crossVectors(L, lx).normalize();
    const sunKey = L.x.toFixed(4) + L.y.toFixed(4) + L.z.toFixed(4) + this.mapSize;
    if (sunKey !== this._sunKey) { this._sunKey = sunKey; force = true; }
    for (let i = 0; i < this.cascadeCount; i++) {
      const c = this.cascades[i];
      const far = splits[i];
      const r = Math.ceil(far * 1.02 * 4) / 4;
      const every = [1, 2, 4][i] || 4;
      const moved = c.at ? c.at.distanceTo(camPos) : 1e9;
      const due = force || every === 1 || (this.frame + i) % every === 0 || moved > r * 0.06 || c.radius !== r;
      const l = c.light;
      l.shadow.autoUpdate = false;
      if (!due) { l.shadow.needsUpdate = false; continue; }
      l.shadow.needsUpdate = true;
      c.at = camPos.clone();
      const texel = (2 * r) / this.mapSize;
      let cx = camPos.dot(lx), cy = camPos.dot(ly);
      const cz = camPos.dot(L);
      cx = Math.round(cx / texel) * texel; cy = Math.round(cy / texel) * texel;
      const centre = new THREE.Vector3().copy(lx).multiplyScalar(cx).addScaledVector(ly, cy).addScaledVector(L, cz);
      const back = r + this.margin;
      l.position.copy(centre).addScaledVector(L, back);
      l.target.position.copy(centre);
      l.target.updateMatrixWorld();
      l.updateMatrixWorld();
      const cam = l.shadow.camera;
      cam.left = -r; cam.right = r; cam.top = r; cam.bottom = -r;
      cam.near = 0.5; cam.far = back + r + 2;
      cam.updateProjectionMatrix();
      const range = cam.far - cam.near;
      // the normal offset is done per pixel in the kit's shaders (slope-scaled); this only covers depth precision
      l.shadow.normalBias = 0;
      l.shadow.bias = -(texel * 0.2 + 0.005) / range;
      c.far = far; c.radius = r; c.texel = texel;
    }
    const cv = SHARED.uSlCascade.value;
    cv.set(this.cascades[0] ? this.cascades[0].far : 0, this.cascades[1] ? this.cascades[1].far : 0,
      this.cascades[2] ? this.cascades[2].far : 0, 0.12);
    const rw = 0.035 * Math.max(0.1, this.softness);
    SHARED.uSlCascadeTexel.value.set(this.cascades[0] ? this.cascades[0].texel : 0.02, this.cascades[1] ? this.cascades[1].texel : 0.05,
      this.cascades[2] ? this.cascades[2].texel : 0.2, 0);
    const rv = SHARED.uSlShadowRadius.value;
    const rad = this.cascades.map((c) => clamp(rw / c.texel, 1.0, 6.0));
    rv.set(rad[0] || 1, rad[1] || 1, rad[2] || 1, 0);
    this._updatePool(camPos, dt);
  }

  // Picks the lamps nearest the camera again on the next frame (after a shot change, or a style adding lamps).
  repickLamps() { this._chosen = null; this._poolTimer = 0; }

  _updatePool(camPos, dt) {
    this._poolTimer -= dt;
    if (this._poolTimer <= 0 || !this._chosen) {
      this._poolTimer = 0.25;
      const ranked = this.sources.map((s) => ({ s, d: s.p.distanceTo(camPos) - s.range }))
        .filter((x) => x.d < 70).sort((a, b) => a.d - b.d).slice(0, this.poolSize);
      this._chosen = ranked.map((x) => x.s);
    }
    const t = SHARED.uSlTime.value;
    for (let i = 0; i < this.poolSize; i++) {
      const p = this.pool[i], s = this._chosen[i];
      if (!s) { p.intensity = 0; continue; }
      p.position.copy(s.p);
      p.color.copy(s.colorOverride || s.color);
      p.distance = s.range;
      const flicker = s.note.includes('lantern') || s.note.includes('lamp') || s.note.includes('fire')
        ? 0.92 + 0.05 * Math.sin(t * 9.1 + s.phase) + 0.03 * Math.sin(t * 23.7 + s.phase * 2.1) : 1;
      p.intensity = this.lightBase * s.intensity * this.lightScale * s.scale * flicker;
    }
  }

  // The sun's position on screen (uv 0..1, z = 1 when in front of the camera).
  sunScreen(camera, out = new THREE.Vector3()) {
    const camPos = new THREE.Vector3().setFromMatrixPosition(camera.matrixWorld);
    const p = camPos.addScaledVector(this.sunDir, 1000).project(camera);
    const fwd = new THREE.Vector3(); camera.getWorldDirection(fwd);
    return out.set(p.x * 0.5 + 0.5, p.y * 0.5 + 0.5, fwd.dot(this.sunDir) > 0 ? 1 : 0);
  }
}
