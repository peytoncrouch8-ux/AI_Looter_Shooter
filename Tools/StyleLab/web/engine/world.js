// The world from the export: terrain near and far, every placed instance (InstancedMeshes per model x material, in
// 64 m tiles so the camera and shadow frustums can cull them, mirrored ones apart), PCG's scatter rules (trees and
// other long-range layers once over the core, grass and small things around the camera), colliders for walking and
// shooting, and the maps the scatter and the minimap read.
import * as THREE from 'three';
import { mergeGeometries } from 'three/addons/utils/BufferGeometryUtils.js';
import { mulberry32, logOnce, clamp } from './util.js';

const SOLID_TAGS = new Set(['building', 'prop', 'rock', 'cliff', 'tree', 'fence', 'grave', 'loot']);
const NO_SHADOW_TAGS = new Set(['grass', 'decal', 'effect']);
const DRESSING = new Set(['prop', 'fence', 'grave', 'loot', 'decal', 'effect', 'creature']);
const BIT = { building: 1, dressing: 2, rock: 4, cliff: 8, tree: 16, scrub: 32, water: 64 };
const TILE = 64;
const FLIP = new THREE.Matrix4().makeScale(-1, 1, 1);
const fract = (x) => x - Math.floor(x);

class Box {
  constructor(min, max, matrix, tag, inst) {
    this.min = min; this.max = max; this.matrix = matrix.clone(); this.inv = matrix.clone().invert(); this.tag = tag; this.inst = inst;
    const c = new THREE.Vector3().addVectors(min, max).multiplyScalar(0.5).applyMatrix4(matrix);
    this.scale = new THREE.Vector3(); matrix.decompose(new THREE.Vector3(), new THREE.Quaternion(), this.scale);
    this.scale.set(Math.abs(this.scale.x), Math.abs(this.scale.y), Math.abs(this.scale.z));
    this.centre = c;
    this.radius = new THREE.Vector3().subVectors(max, min).multiply(this.scale).length() * 0.5;
    const hy = (max.y - min.y) * this.scale.y * 0.5;
    this.top = c.y + hy; this.bottom = c.y - hy;
  }
}

export class World {
  constructor(engine) {
    this.e = engine;
    this.root = new THREE.Group(); this.root.name = 'World';
    this.meshes = [];          // every world mesh, for material swaps: { mesh, matName }
    this.boxes = [];
    this.grid = new Map();     // 8 m cells -> boxes (for walking and shooting)
    this.scatterMeshes = [];
    this.terrainMeshes = [];
    this.raycaster = new THREE.Raycaster();
    this.images = new Map();
    this.density = 1;
  }

  // -------------------------------------------------------------- terrain
  async buildTerrain() {
    const { assets } = this.e;
    const t = assets.manifest.terrain || {};
    const mats = assets.manifest.materials || {};
    const add = (g, near) => {
      g.scene.traverse((o) => {
        if (!o.isMesh) return;
        const mname = (Array.isArray(o.material) ? o.material[0] : o.material).name;
        const rec = mats[mname] || {};
        const isTerrain = rec.master === 'Terrain' || /^Terrain/i.test(mname);
        const backdrop = /Backdrop/i.test(o.name) || rec.unlit;
        o.castShadow = isTerrain || /Canyon/i.test(o.name);
        o.receiveShadow = !backdrop;
        o.userData.matName = isTerrain ? '__terrain__' : mname;
        o.userData.terrain = isTerrain;
        o.userData.near = near;
        if (backdrop) o.userData.noNormal = true;
        if (/Water/i.test(o.name)) this.waterMesh = o;
        if (isTerrain) this.terrainMeshes.push(o);
        this.meshes.push({ mesh: o, matName: o.userData.matName });
      });
      g.scene.updateMatrixWorld(true);
      this.root.add(g.scene);
    };
    const jobs = [];
    if (t.near) jobs.push(assets.loadGLB(t.near).then((g) => add(g, true)).catch((e) => console.warn('[StyleLab] terrain near', e.message)));
    for (const f of [].concat(t.far || [])) jobs.push(assets.loadGLB(f).then((g) => add(g, false)).catch((e) => console.warn('[StyleLab] terrain far', f, e.message)));
    assets.progress.add(jobs.length);
    await Promise.all(jobs.map((j) => j.then(() => assets.progress.tick('terrain'))));
    // heights past the heights file come from the terrain meshes, by a ray from above
    const ground = this.terrainMeshes;
    const down = new THREE.Vector3(0, -1, 0), o = new THREE.Vector3();
    const cache = new Map();
    if (this.e.heights) {
      this.e.heights.fallback = (x, z) => {
        const k = Math.round(x) + ',' + Math.round(z);
        if (cache.has(k)) return cache.get(k);
        o.set(x, 5000, z);
        this.raycaster.set(o, down); this.raycaster.far = 10000;
        const hit = this.raycaster.intersectObjects(ground, false)[0];
        const h = hit ? hit.point.y : NaN;
        if (cache.size < 50000) cache.set(k, h);
        return h;
      };
    }
  }

  async _readImage(rel) {
    if (this.images.has(rel)) return this.images.get(rel);
    const img = await this.e.assets.loadImage(rel);
    const w = Math.min(img.width, 2048), h = Math.min(img.height, 2048);
    const c = document.createElement('canvas'); c.width = w; c.height = h;
    const g = c.getContext('2d', { willReadFrequently: true });
    g.drawImage(img, 0, 0, w, h);
    const d = { w, h, data: g.getImageData(0, 0, w, h).data };
    this.images.set(rel, d);
    return d;
  }

  // The macro, ring macro and mask maps (textures for the terrain, CPU copies for the scatter).
  async loadMaps() {
    const { assets } = this.e;
    const t = assets.manifest.terrain || {};
    this.macroRect = t.macroRect || (t.heights && t.heights.rect) || [-100, -100, 100, 100];
    const tex = async (rel) => {
      const x = await assets.loadTexture(rel, 'macro');
      x.wrapS = x.wrapT = THREE.ClampToEdgeWrapping;
      return x;
    };
    try {
      if (t.macro) {
        this.macroTex = await tex(t.macro);
        const m = await this._readImage(t.macro);
        let r = 0, g = 0, b = 0, n = 0;
        for (let i = 0; i < m.data.length; i += 4 * 97) { r += m.data[i]; g += m.data[i + 1]; b += m.data[i + 2]; n++; }
        this.macroMean = new THREE.Color().setRGB(r / n / 255, g / n / 255, b / n / 255, THREE.SRGBColorSpace);
      }
    } catch (e) { logOnce('macro', 'no macro map', e.message); }
    try { if (t.ringMacro) this.ringTex = await tex(t.ringMacro); } catch (e) { logOnce('ring', 'no ring macro', e.message); }
    try {
      if (t.masks) {
        this.masks = await this._readImage(t.masks);
        this.masksTex = await assets.loadTexture(t.masks, 'mask');
        this.masksTex.wrapS = this.masksTex.wrapT = THREE.ClampToEdgeWrapping;
      }
    } catch (e) { logOnce('masks', 'no masks map', e.message); }
    // every scatter rule's mask
    const rels = new Set((assets.scene.scatter || []).map((r) => r.maskTex).filter(Boolean));
    await Promise.all([...rels].map((rel) => this._readImage(rel).catch((e) => logOnce('mask' + rel, 'scatter mask missing', rel, e.message))));
  }

  sample(img, rect, x, z, ch) {
    if (!img) return 0;
    const u = (x - rect[0]) / (rect[2] - rect[0]), v = (z - rect[1]) / (rect[3] - rect[1]);
    if (u < 0 || u >= 1 || v < 0 || v >= 1) return 0;
    const px = Math.floor(u * img.w), py = Math.floor(v * img.h);
    return img.data[(py * img.w + px) * 4 + ch] / 255;
  }
  mask(x, z, ch) { return this.sample(this.masks, this.macroRect, x, z, ch); }

  // -------------------------------------------------------------- instances
  // Everything static goes into one BatchedMesh per material (and mirror sign, and shadow casting): a draw call per
  // material instead of one per model, with per-instance culling for the camera and each shadow cascade.
  _addInstanced(model, mats, tag, opts = {}) {
    if (!this.batchItems) this.batchItems = new Map();
    const groups = new Map();
    for (const part of model.parts) {
      const nm = (opts.swaps && opts.swaps[part.materialName]) || part.materialName;
      if (!groups.has(nm)) groups.set(nm, []);
      groups.get(nm).push(part);
    }
    const cast = opts.castShadow ?? !NO_SHADOW_TAGS.has(tag);
    const recs = this.e.assets.manifest.materials || {};
    const pos = new THREE.Vector3();
    let tris = 0;
    for (const [matName, parts] of groups) {
      const geo = batchGeometry(mergedGeometry(parts), model.name + '|' + matName);
      if (!geo) continue;
      // the game's late-summer turning: each tree takes a random share of TintVariation instead of Tint
      const rec = recs[matName] || {}, prm = rec.params || {};
      const tv = Array.isArray(prm.TintVariation) ? prm.TintVariation : null;
      const base = Array.isArray(rec.tintLinear) ? rec.tintLinear : [1, 1, 1];
      for (const m of mats) {
        const neg = m.determinant() < 0;
        const key = matName + (neg ? '|n' : '|p') + (cast ? '|s' : '|0');
        if (!this.batchItems.has(key)) this.batchItems.set(key, { matName, neg, cast, items: [] });
        pos.setFromMatrixPosition(m);
        const h = fract(Math.sin(pos.x * 12.9898 + pos.z * 78.233) * 43758.5453);
        const shade = opts.instanceColor ? 0.88 + 0.22 * fract(h * 7.31) : 1;
        let color = null;
        if (tv) {
          const k = h * (prm.VariationAmount ?? 0.3);
          color = [0, 1, 2].map((i) => shade * (1 + (tv[i] / Math.max(base[i], 1e-3) - 1) * k));
        }
        this.batchItems.get(key).items.push({ geo, m, shade, color });
      }
      tris += (geo.index ? geo.index.count / 3 : geo.attributes.position.count / 3) * mats.length;
    }
    return tris;
  }

  _shadeRand() { if (!this._sr) this._sr = mulberry32(4242); return this._sr(); }

  // Builds the BatchedMeshes from everything _addInstanced collected.
  finishBatches() {
    const tmp = new THREE.Matrix4(), col = new THREE.Color();
    let n = 0;
    for (const b of (this.batchItems || new Map()).values()) {
      const geos = [...new Set(b.items.map((i) => i.geo))];
      let verts = 0, idx = 0;
      for (const g of geos) { verts += g.attributes.position.count; idx += g.index.count; }
      const bm = new THREE.BatchedMesh(b.items.length, verts, idx, this.e.placeholderMaterial);
      const ids = new Map();
      for (const g of geos) ids.set(g, bm.addGeometry(g));
      const colored = b.items.some((i) => i.shade !== 1 || i.color);
      for (const it of b.items) {
        const id = bm.addInstance(ids.get(it.geo));
        bm.setMatrixAt(id, b.neg ? tmp.multiplyMatrices(FLIP, it.m) : it.m);
        if (colored) bm.setColorAt(id, it.color ? col.setRGB(it.color[0], it.color[1], it.color[2]) : col.setScalar(it.shade));
      }
      if (b.neg) { bm.scale.x = -1; bm.updateMatrix(); }
      bm.computeBoundingSphere();
      bm.computeBoundingBox();
      bm.perObjectFrustumCulled = true;
      bm.sortObjects = true;
      bm.castShadow = b.cast;
      bm.receiveShadow = true;
      bm.name = 'batch:' + b.matName + (b.neg ? ':mirrored' : '');
      bm.userData = { matName: b.matName, batched: true };
      this.root.add(bm);
      this.meshes.push({ mesh: bm, matName: b.matName, batched: true });
      n++;
    }
    this.batchItems = null;
    this.batchCount = n;
  }

  buildInstances() {
    const { assets } = this.e;
    const byModel = new Map();
    for (const inst of assets.scene.instances) {
      if (!byModel.has(inst.model)) byModel.set(inst.model, []);
      byModel.get(inst.model).push(inst);
    }
    const q = new THREE.Quaternion(), e = new THREE.Euler(), p = new THREE.Vector3(), s = new THREE.Vector3();
    let tris = 0;
    for (const [name, list] of byModel) {
      const model = assets.model(name);
      if (!model) continue;
      const mats = list.map((inst) => {
        p.fromArray(inst.p || [0, 0, 0]); e.set(...(inst.r || [0, 0, 0]), 'XYZ'); q.setFromEuler(e); s.fromArray(inst.s || [1, 1, 1]);
        return new THREE.Matrix4().compose(p, q, s);
      });
      const tag = list[0].tag || 'prop';
      if (model.skinned) {
        list.forEach((inst, i) => {
          const c = this.e.cloneModel(name);
          if (!c) return;
          c.matrixAutoUpdate = false; c.matrix.copy(mats[i]); c.updateMatrixWorld(true);
          this.root.add(c);
        });
        continue;
      }
      tris += this._addInstanced(model, mats, tag, { instanceColor: tag === 'tree' || tag === 'scrub' || tag === 'rock' });
      const b = model.info.bounds;
      if (b && SOLID_TAGS.has(tag)) {
        const min = new THREE.Vector3(...b[0]), max = new THREE.Vector3(...b[1]);
        list.forEach((inst, i) => this._addBox(new Box(min, max, mats[i], inst.tag || tag, inst)));
      }
    }
    this.stats = { tris };
  }

  // Non-spider creatures and characters stand where the scene puts them (rest pose).
  placeCharacters() {
    const { assets } = this.e;
    for (const c of assets.scene.creatures || []) {
      // spiders are the live ones; the Unpaid are encounter spawns the lab has no AI for
      if (/^Spider$/i.test(c.model) || /^Unpaid/i.test(c.model)) continue;
      const obj = this.e.cloneModel(c.model, false);
      if (!obj) continue;
      obj.position.fromArray(c.p);
      obj.rotation.y = c.yaw || 0;
      if (c.scale) obj.scale.setScalar(c.scale);
      obj.updateMatrixWorld(true);
      this.root.add(obj);
    }
  }

  _addBox(box) {
    this.boxes.push(box);
    const r = box.radius, cs = 8;
    for (let gx = Math.floor((box.centre.x - r) / cs); gx <= Math.floor((box.centre.x + r) / cs); gx++) {
      for (let gz = Math.floor((box.centre.z - r) / cs); gz <= Math.floor((box.centre.z + r) / cs); gz++) {
        const k = gx + ',' + gz;
        if (!this.grid.has(k)) this.grid.set(k, []);
        this.grid.get(k).push(box);
      }
    }
  }

  boxesNear(x, z, r) {
    const cs = 8, out = new Set();
    for (let gx = Math.floor((x - r) / cs); gx <= Math.floor((x + r) / cs); gx++)
      for (let gz = Math.floor((z - r) / cs); gz <= Math.floor((z + r) / cs); gz++) {
        const l = this.grid.get(gx + ',' + gz); if (l) for (const b of l) out.add(b);
      }
    return out;
  }

  // Pushes a walking circle (feet at pos) out of the solid boxes it overlaps; trees are trunks.
  collide(pos, radius, height) {
    const lp = new THREE.Vector3();
    for (const b of this.boxesNear(pos.x, pos.z, radius + 6)) {
      if (b.tag === 'tree' || b.tag === 'scrub') continue;
      if (pos.y > b.top - 0.45 || pos.y + height < b.bottom) continue;
      lp.copy(pos).applyMatrix4(b.inv);
      const rx = radius / Math.max(b.scale.x, 1e-3), rz = radius / Math.max(b.scale.z, 1e-3);
      const cx = clamp(lp.x, b.min.x, b.max.x), cz = clamp(lp.z, b.min.z, b.max.z);
      const dx = lp.x - cx, dz = lp.z - cz;
      const inside = lp.x > b.min.x && lp.x < b.max.x && lp.z > b.min.z && lp.z < b.max.z;
      if (inside) {
        const ex = [lp.x - b.min.x, b.max.x - lp.x, lp.z - b.min.z, b.max.z - lp.z];
        const i = ex.indexOf(Math.min(...ex));
        if (i === 0) lp.x = b.min.x - rx; else if (i === 1) lp.x = b.max.x + rx; else if (i === 2) lp.z = b.min.z - rz; else lp.z = b.max.z + rz;
      } else {
        const d = Math.hypot(dx / rx, dz / rz);
        if (d >= 1) continue;
        lp.x = cx + dx / Math.max(d, 1e-4); lp.z = cz + dz / Math.max(d, 1e-4);
      }
      const y = pos.y;
      pos.copy(lp.applyMatrix4(b.matrix)); pos.y = y;
    }
    for (const b of this.boxesNear(pos.x, pos.z, radius + 3)) {
      if (b.tag !== 'tree') continue;
      const dx = pos.x - b.centre.x, dz = pos.z - b.centre.z, d = Math.hypot(dx, dz), rr = radius + 0.3 * b.scale.x;
      if (d < rr && d > 1e-4) { pos.x = b.centre.x + dx / d * rr; pos.z = b.centre.z + dz / d * rr; }
    }
  }

  // A shot: the nearest hit on the ground or a placed thing's mesh. Returns { point, normal, dist, tag } or null.
  raycast(origin, dir, maxDist = 250) {
    let best = null;
    const hd = this.e.heights ? this.e.heights.raycast(origin, dir, maxDist) : Infinity;
    if (hd < maxDist) {
      const pt = origin.clone().addScaledVector(dir, hd);
      best = { point: pt, normal: this.e.heights.normal(pt.x, pt.z), dist: hd, tag: 'ground' };
    }
    const lim = best ? best.dist : maxDist;
    const ray = new THREE.Ray(origin, dir);
    const cand = [];
    const seen = new Set();
    const box3 = new THREE.Box3(), hitP = new THREE.Vector3();
    for (let t = 0; t < lim + 8; t += 6) {
      const x = origin.x + dir.x * t, z = origin.z + dir.z * t;
      for (const b of this.boxesNear(x, z, 4)) {
        if (seen.has(b)) continue; seen.add(b);
        const lr = ray.clone().applyMatrix4(b.inv);
        box3.set(b.min, b.max);
        if (lr.intersectBox(box3, hitP)) cand.push({ b, d: hitP.applyMatrix4(b.matrix).distanceTo(origin) });
      }
    }
    cand.sort((a, b) => a.d - b.d);
    for (const c of cand.slice(0, 8)) {
      if (best && c.d > best.dist) break;
      const h = this._meshHit(c.b, origin, dir, best ? best.dist : maxDist);
      if (h && (!best || h.dist < best.dist)) best = h;
    }
    return best;
  }

  _meshHit(box, origin, dir, far) {
    const model = this.e.assets.model(box.inst.model);
    if (!model) return null;
    if (!this._tmpMesh) this._tmpMesh = new THREE.Mesh(undefined, new THREE.MeshBasicMaterial({ side: THREE.DoubleSide }));
    const tm = this._tmpMesh;
    this.raycaster.set(origin, dir); this.raycaster.far = far;
    let best = null;
    for (const part of model.parts) {
      tm.geometry = part.geometry;
      tm.matrixWorld.multiplyMatrices(box.matrix, part.matrix);
      const hits = this.raycaster.intersectObject(tm, false);
      if (hits[0] && (!best || hits[0].distance < best.dist)) {
        const n = hits[0].face ? hits[0].face.normal.clone().transformDirection(tm.matrixWorld) : dir.clone().negate();
        if (n.dot(dir) > 0) n.negate();
        best = { point: hits[0].point.clone(), normal: n, dist: hits[0].distance, tag: box.tag };
      }
    }
    return best;
  }

  // -------------------------------------------------------------- scatter (PCG's layers as data)
  // Each layer: candidates every `cell` m (moved up to jitter x cell), kept where mask x random >= keep, on ground whose
  // normal's up part is in [flatMin, flatMax], off roads, buildings, dressing and water; models by weight, scaled within
  // `scale`, upright or tilted with the slope, sunk `sink` m, culled past `cull` m. Layers that reach far (cull past
  // 120 m or never) are placed once over the core; the rest are rebuilt around the camera as it moves.
  buildScatter() {
    const { assets } = this.e;
    this.scatterRules = [];
    const r = this.macroRect;
    const cell = 0.5, gw = Math.ceil((r[2] - r[0]) / cell), gh = Math.ceil((r[3] - r[1]) / cell);
    const occ = new Uint8Array(gw * gh);
    const lp = new THREE.Vector3();
    const mark = (x0, z0, x1, z1, bit, test) => {
      const a = Math.max(0, Math.floor((x0 - r[0]) / cell)), b = Math.min(gw - 1, Math.ceil((x1 - r[0]) / cell));
      const c = Math.max(0, Math.floor((z0 - r[1]) / cell)), d = Math.min(gh - 1, Math.ceil((z1 - r[1]) / cell));
      for (let gz = c; gz <= d; gz++) for (let gx = a; gx <= b; gx++) {
        if (!test || test(r[0] + (gx + 0.5) * cell, r[1] + (gz + 0.5) * cell)) occ[gz * gw + gx] |= bit;
      }
    };
    for (const b of this.boxes) {
      const bit = b.tag === 'building' ? BIT.building : DRESSING.has(b.tag) ? BIT.dressing : BIT[b.tag] || BIT.dressing;
      const rad = b.radius;
      mark(b.centre.x - rad, b.centre.z - rad, b.centre.x + rad, b.centre.z + rad, bit, (x, z) => {
        lp.set(x, b.centre.y, z).applyMatrix4(b.inv);
        return lp.x >= b.min.x - 0.25 && lp.x <= b.max.x + 0.25 && lp.z >= b.min.z - 0.25 && lp.z <= b.max.z + 0.25;
      });
    }
    if (this.waterMesh) {
      const g = this.waterMesh.geometry, pos = g.attributes.position, idx = g.index;
      const n = idx ? idx.count : pos.count;
      const v = new THREE.Vector3();
      for (let i = 0; i < n; i += 3) {
        let x0 = 1e9, z0 = 1e9, x1 = -1e9, z1 = -1e9;
        for (let k = 0; k < 3; k++) {
          v.fromBufferAttribute(pos, idx ? idx.getX(i + k) : i + k).applyMatrix4(this.waterMesh.matrixWorld);
          x0 = Math.min(x0, v.x); x1 = Math.max(x1, v.x); z0 = Math.min(z0, v.z); z1 = Math.max(z1, v.z);
        }
        mark(x0 - 0.5, z0 - 0.5, x1 + 0.5, z1 + 0.5, BIT.water);
      }
    }
    this.occ = { data: occ, w: gw, h: gh, cell };
    // the game's NoTrees boxes (zones kept open: Main Street, the chapel, the lookouts)
    this.noTrees = (assets.scene.noTrees || []).map((z) => z.rect || z).filter((r) => Array.isArray(r) && r.length === 4);
    const chan = { R: 0, G: 1, B: 2, A: 3 };
    let ri = 0;
    for (const rule of assets.scene.scatter || []) {
      const names = rule.models || [];
      const models = names.map((n) => assets.model(n));
      const ok = models.map((m, i) => (m ? i : -1)).filter((i) => i >= 0);
      if (!ok.length) continue;
      const weights = names.map((n, i) => (models[i] ? (rule.weights ? rule.weights[i] : 1) : 0));
      const wsum = weights.reduce((a, b) => a + b, 0);
      let avoidBits = 0;
      for (const t of rule.avoidTags || []) if (BIT[t]) avoidBits |= BIT[t];
      const swaps = {};
      for (const [a, b] of Object.entries(rule.materialSwaps || {})) if (assets.manifest.materials[b]) swaps[a] = b;
      const cull = rule.cull === null || rule.cull === undefined ? Infinity : rule.cull;
      const sr = {
        rule, ri: ri++, models, weights: weights.map((w) => w / wsum), cull, isStatic: cull > 120,
        img: rule.maskTex ? this.images.get(rule.maskTex) : this.masks, rect: rule.maskRect || this.macroRect,
        ch: chan[rule.mask || 'B'] ?? 2, cell: rule.cell || (1 / Math.sqrt(rule.perM2 || 1)), keep: rule.keep ?? 0.1,
        jitter: rule.jitter ?? 0.5, flatMin: rule.flatMin ?? (rule.slopeMax !== undefined ? Math.cos(Math.atan(rule.slopeMax)) : 0.7),
        flatMax: rule.flatMax ?? 1, scale: rule.scale || [0.8, 1.2], upright: rule.upright !== false, sink: rule.sink || 0,
        avoidRoad: (rule.avoidTags || []).includes('road'), avoidBits, swaps, chunks: new Map(), centre: null,
        density: this.density * (rule.density ?? 1),
      };
      const isTree = /tree|pine/i.test(rule.name || '') || names.some((n) => /Pine|Oak|Birch|Tree|Juniper/.test(n));
      sr.tag = isTree ? 'tree' : 'scrub';
      sr.noTrees = rule.noTrees ?? isTree;
      if (sr.isStatic) this._buildStaticLayer(sr);
      else this._buildDynamicLayer(sr);
      this.scatterRules.push(sr);
    }
  }

  // The candidates of one chunk (chunkCells x chunkCells cells of the layer's grid), cached.
  _chunk(sr, cx, cz, n) {
    const key = cx + ',' + cz;
    if (sr.chunks.has(key)) return sr.chunks.get(key);
    const H = this.e.heights, occ = this.occ, r = this.macroRect, out = [];
    const nrm = new THREE.Vector3();
    for (let j = 0; j < n; j++) for (let i = 0; i < n; i++) {
      const gi = cx * n + i, gj = cz * n + j;
      const rnd = mulberry32((Math.imul(gi, 73856093) ^ Math.imul(gj, 19349663) ^ Math.imul(sr.ri + 1, 83492791)) >>> 0);
      const x = sr.rect[0] + (gi + 0.5) * sr.cell + (rnd() * 2 - 1) * sr.jitter * sr.cell;
      const z = sr.rect[1] + (gj + 0.5) * sr.cell + (rnd() * 2 - 1) * sr.jitter * sr.cell;
      const keepR = rnd(), pick = rnd(), sc = rnd(), rot = rnd(), shade = rnd(), thin = rnd();
      if (x < sr.rect[0] || x >= sr.rect[2] || z < sr.rect[1] || z >= sr.rect[3]) continue;
      if (thin > sr.density) continue;
      if (this.sample(sr.img, sr.rect, x, z, sr.ch) * keepR < sr.keep) continue;
      if (sr.avoidRoad && this.mask(x, z, 0) > 0.5) continue;
      if (sr.noTrees && this.noTrees.some((q) => x >= q[0] && x <= q[2] && z >= q[1] && z <= q[3])) continue;
      if (sr.avoidBits) {
        const gx = Math.floor((x - r[0]) / occ.cell), gz = Math.floor((z - r[1]) / occ.cell);
        if (gx >= 0 && gz >= 0 && gx < occ.w && gz < occ.h && (occ.data[gz * occ.w + gx] & sr.avoidBits)) continue;
      }
      if (!H) continue;
      H.normal(x, z, nrm, 0.6);
      if (nrm.y < sr.flatMin || nrm.y > sr.flatMax + 1e-6) continue;
      let acc = 0, mi = 0;
      for (; mi < sr.weights.length; mi++) { acc += sr.weights[mi]; if (pick < acc) break; }
      mi = Math.min(mi, sr.weights.length - 1);
      if (!sr.models[mi]) continue;
      out.push({ x, y: H.height(x, z) - sr.sink, z, n: sr.upright ? null : nrm.clone(), rot: rot * Math.PI * 2,
        s: sr.scale[0] + (sr.scale[1] - sr.scale[0]) * sc, m: mi, h: thin / Math.max(sr.density, 1e-3), shade: 0.84 + 0.26 * shade });
    }
    sr.chunks.set(key, out);
    return out;
  }

  _matrixOf(pt, grow = 1, out = new THREE.Matrix4()) {
    const q = new THREE.Quaternion().setFromAxisAngle(new THREE.Vector3(0, 1, 0), pt.rot);
    if (pt.n) q.premultiply(new THREE.Quaternion().setFromUnitVectors(new THREE.Vector3(0, 1, 0), pt.n));
    return out.compose(new THREE.Vector3(pt.x, pt.y, pt.z), q, new THREE.Vector3().setScalar(pt.s * grow));
  }

  _buildStaticLayer(sr) {
    const n = Math.max(1, Math.round(16 / sr.cell));
    const span = sr.cell * n;
    const per = sr.models.map(() => []);
    const c0 = Math.floor(0), cxN = Math.ceil((sr.rect[2] - sr.rect[0]) / span), czN = Math.ceil((sr.rect[3] - sr.rect[1]) / span);
    for (let cz = c0; cz < czN; cz++) for (let cx = c0; cx < cxN; cx++) for (const pt of this._chunk(sr, cx, cz, n)) per[pt.m].push(pt);
    sr.chunks.clear();
    per.forEach((pts, mi) => {
      const model = sr.models[mi];
      if (!model || !pts.length) return;
      const mats = pts.map((pt) => this._matrixOf(pt));
      this._addInstanced(model, mats, sr.tag, { swaps: sr.swaps, instanceColor: true, castShadow: true });
      if (sr.tag === 'tree' && model.info.bounds) {
        const b = model.info.bounds;
        const min = new THREE.Vector3(b[0][0] * 0.15, b[0][1], b[0][2] * 0.15), max = new THREE.Vector3(b[1][0] * 0.15, b[1][1], b[1][2] * 0.15);
        mats.forEach((m) => this._addBox(new Box(min, max, m, 'tree', { model: model.name })));
      }
    });
  }

  _buildDynamicLayer(sr) {
    const radius = Math.min(sr.cull, 90);
    sr.radius = radius;
    const area = Math.PI * radius * radius;
    const cap = Math.ceil(area / (sr.cell * sr.cell) * 0.8) + 64;
    sr.meshes = sr.models.map((model, mi) => {
      if (!model) return [];
      const share = Math.max(0.05, sr.weights[mi]);
      const capM = Math.min(60000, Math.ceil(cap * Math.min(1, share * 1.6)) + 32);
      const groups = new Map();
      for (const part of model.parts) {
        const nm = sr.swaps[part.materialName] || part.materialName;
        if (!groups.has(nm)) groups.set(nm, []);
        groups.get(nm).push(part);
      }
      return [...groups].map(([matName, parts]) => {
        const im = new THREE.InstancedMesh(mergedGeometry(parts), this.e.placeholderMaterial, capM);
        im.count = 0;
        im.instanceMatrix.setUsage(THREE.DynamicDrawUsage);
        im.instanceColor = new THREE.InstancedBufferAttribute(new Float32Array(capM * 3).fill(1), 3);
        im.castShadow = sr.cull > 60;
        im.receiveShadow = true;
        im.frustumCulled = false;
        im.name = 'scatter:' + sr.rule.name + ':' + model.name + '|' + matName;
        im.userData = { matName, model: model.name, tag: 'grass', scatter: true, noNormal: sr.cull < 60, cap: capM };
        this.root.add(im);
        this.meshes.push({ mesh: im, matName });
        this.scatterMeshes.push(im);
        return im;
      });
    });
  }

  updateScatter(camPos, force = false) {
    if (!this.scatterRules) return;
    const m4 = new THREE.Matrix4(), col = new THREE.Color();
    for (const sr of this.scatterRules) {
      if (sr.isStatic) continue;
      if (!force && sr.centre && sr.centre.distanceTo(camPos) < Math.max(3, sr.radius * 0.12)) continue;
      sr.centre = camPos.clone();
      const R = sr.radius;
      const n = Math.max(1, Math.round(12 / sr.cell)), span = n * sr.cell;
      const counts = sr.meshes.map(() => 0);
      const c0x = Math.floor((camPos.x - R - sr.rect[0]) / span), c1x = Math.floor((camPos.x + R - sr.rect[0]) / span);
      const c0z = Math.floor((camPos.z - R - sr.rect[1]) / span), c1z = Math.floor((camPos.z + R - sr.rect[1]) / span);
      for (let cx = c0x; cx <= c1x; cx++) for (let cz = c0z; cz <= c1z; cz++) {
        if (cx < 0 || cz < 0) continue;
        for (const pt of this._chunk(sr, cx, cz, n)) {
          const d = Math.hypot(pt.x - camPos.x, pt.y - camPos.y, pt.z - camPos.z);
          if (d > R) continue;
          // thinned over the last third of the range, the survivors a little larger
          const keep = d < R * 0.66 ? 1 : 1 - 0.75 * (d - R * 0.66) / (R * 0.34);
          if (pt.h > keep) continue;
          const mi = pt.m, list = sr.meshes[mi];
          if (!list.length || counts[mi] >= list[0].userData.cap) continue;
          this._matrixOf(pt, 1 + (1 - keep) * 0.5, m4);
          col.setScalar(pt.shade);
          for (const im of list) { im.setMatrixAt(counts[mi], m4); im.setColorAt(counts[mi], col); }
          counts[mi]++;
        }
      }
      sr.meshes.forEach((list, mi) => list.forEach((im) => {
        im.count = counts[mi]; im.instanceMatrix.needsUpdate = true; if (im.instanceColor) im.instanceColor.needsUpdate = true;
      }));
      if (sr.chunks.size > 3000) sr.chunks.clear();
    }
  }
}

// The parts of one model that share a material, merged into one geometry in the model's frame.
// Batches need one attribute layout: position, normal, uv, color (float, colour with alpha), indexed.
const _batchGeo = new Map();
export function batchGeometry(geo, key) {
  if (_batchGeo.has(key)) return _batchGeo.get(key);
  const g = new THREE.BufferGeometry();
  const n = geo.attributes.position.count;
  const f = (name, size, def) => {
    const a = geo.attributes[name], out = new Float32Array(n * size);
    for (let i = 0; i < n; i++) for (let k = 0; k < size; k++) out[i * size + k] = a ? (k < a.itemSize ? a.getComponent(i, k) : 1) : def[k];
    return new THREE.BufferAttribute(out, size);
  };
  g.setAttribute('position', f('position', 3, [0, 0, 0]));
  g.setAttribute('normal', f('normal', 3, [0, 1, 0]));
  g.setAttribute('uv', f('uv', 2, [0, 0]));
  g.setAttribute('color', f('color', 4, [1, 1, 1, 1]));
  g.setIndex(geo.index ? Array.from(geo.index.array) : [...Array(n).keys()]);
  g.computeBoundingSphere(); g.computeBoundingBox();
  _batchGeo.set(key, g);
  return g;
}

export function mergedGeometry(parts) {
  if (parts.length === 1 && parts[0].matrix.equals(new THREE.Matrix4())) return parts[0].geometry;
  const geos = parts.map((p) => { const g = p.geometry.clone(); g.applyMatrix4(p.matrix); return g; });
  const names = Object.keys(geos[0].attributes).filter((n) => geos.every((g) => g.attributes[n] && g.attributes[n].itemSize === geos[0].attributes[n].itemSize));
  for (const g of geos) {
    for (const n of Object.keys(g.attributes)) if (!names.includes(n)) g.deleteAttribute(n);
    for (const n of Object.keys(g.morphAttributes)) delete g.morphAttributes[n];
    if (!g.index) g.setIndex([...Array(g.attributes.position.count).keys()]);
  }
  for (const n of names) {
    const norm = geos.some((g) => g.attributes[n].normalized), types = new Set(geos.map((g) => g.attributes[n].array.constructor));
    if (types.size > 1 || norm) for (const g of geos) {
      const a = g.attributes[n]; const f = new Float32Array(a.count * a.itemSize);
      for (let i = 0; i < a.count; i++) for (let k = 0; k < a.itemSize; k++) f[i * a.itemSize + k] = a.getComponent(i, k);
      g.setAttribute(n, new THREE.BufferAttribute(f, a.itemSize));
    }
  }
  const merged = mergeGeometries(geos, false);
  if (!merged) logOnce('merge' + parts[0].materialName, 'could not merge parts of', parts[0].materialName);
  return merged || geos[0];
}
