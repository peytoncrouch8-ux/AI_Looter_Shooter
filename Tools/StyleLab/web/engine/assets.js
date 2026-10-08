// Loading the export: the manifest and scene, the model packs, the texture sets and the terrain's heights.
// Binary files may also ship as base64 strings in .json files (when a host serves JSON but not .glb/.bin).
import * as THREE from 'three';
import { GLTFLoader } from 'three/addons/loaders/GLTFLoader.js';
import { logOnce } from './util.js';

async function fetchBuffer(url) {
  const r = await fetch(url);
  if (!r.ok) throw new Error(`${url}: HTTP ${r.status}`);
  if (url.endsWith('.json')) {
    const b64 = await r.json();
    const bin = atob(b64);
    const out = new Uint8Array(bin.length);
    for (let i = 0; i < bin.length; i++) out[i] = bin.charCodeAt(i);
    return out.buffer;
  }
  return r.arrayBuffer();
}
export async function fetchJSON(url) {
  const r = await fetch(url);
  if (!r.ok) throw new Error(`${url}: HTTP ${r.status}`);
  return r.json();
}

// A shared progress counter for the loading screen.
export class Progress {
  constructor(onChange) { this.total = 0; this.done = 0; this.label = ''; this.onChange = onChange || (() => {}); }
  add(n = 1) { this.total += n; this.onChange(this); }
  tick(label) { this.done++; if (label) this.label = label; this.onChange(this); }
  get fraction() { return this.total ? this.done / this.total : 0; }
}

export class Assets {
  constructor(base, renderer, progress) {
    this.base = base.endsWith('/') ? base : base + '/';
    this.renderer = renderer;
    this.progress = progress;
    this.gltf = new GLTFLoader();
    this.textures = new Map();      // url -> THREE.Texture
    this.images = new Map();        // url -> HTMLImageElement / ImageBitmap
    this.models = new Map();        // name -> model record
    this.packs = [];
    this.maxAniso = renderer.capabilities.getMaxAnisotropy();
    this.placeholder = {
      bc: this._solid(255, 255, 255, THREE.SRGBColorSpace), n: this._solid(128, 128, 255, THREE.NoColorSpace),
      orm: this._solid(255, 200, 0, THREE.NoColorSpace),
    };
  }

  url(rel) { return this.base + rel; }

  _solid(r, g, b, cs) {
    const t = new THREE.DataTexture(new Uint8Array([r, g, b, 255]), 1, 1);
    t.colorSpace = cs; t.needsUpdate = true;
    return t;
  }

  async loadManifest() {
    this.manifest = await fetchJSON(this.url('manifest.json'));
    try { this.scene = await fetchJSON(this.url('scene.json')); } catch (e) {
      logOnce('noscene', 'no scene.json:', e.message);
      this.scene = { instances: [], lights: [], scatter: [], smoke: [], creatures: [] };
    }
    for (const k of ['instances', 'lights', 'scatter', 'smoke', 'creatures']) this.scene[k] = this.scene[k] || [];
    return this.manifest;
  }

  // Loads an image and keeps it for CPU reads (masks, the macro map).
  async loadImage(rel) {
    const url = this.url(rel);
    if (this.images.has(url)) return this.images.get(url);
    const img = new Image();
    img.crossOrigin = 'anonymous';
    img.decoding = 'async';
    const p = new Promise((res, rej) => { img.onload = () => res(img); img.onerror = () => rej(new Error('image ' + url)); });
    img.src = url;
    await p;
    this.images.set(url, img);
    return img;
  }

  // A texture from the export. kind: 'bc' (sRGB colour) or anything else (linear data). Mapped the glTF way (flipY off).
  async loadTexture(rel, kind = 'bc') {
    const url = this.url(rel);
    if (this.textures.has(url)) return this.textures.get(url);
    const img = await this.loadImage(rel);
    const t = new THREE.Texture(img);
    t.flipY = false;
    t.wrapS = t.wrapT = THREE.RepeatWrapping;
    t.colorSpace = kind === 'bc' || kind === 'macro' ? THREE.SRGBColorSpace : THREE.NoColorSpace;
    t.anisotropy = Math.min(8, this.maxAniso);
    t.minFilter = THREE.LinearMipmapLinearFilter;
    t.magFilter = THREE.LinearFilter;
    t.generateMipmaps = true;
    t.name = rel;
    t.needsUpdate = true;
    this.textures.set(url, t);
    return t;
  }

  // Every texture set in the manifest, preloaded so kit.tex can answer synchronously.
  async loadTextureSets() {
    const sets = this.manifest.textureSets || {};
    const jobs = [];
    for (const [name, s] of Object.entries(sets)) {
      for (const kind of ['bc', 'n', 'orm']) {
        if (!s[kind]) continue;
        this.progress.add();
        jobs.push(this.loadTexture(s[kind], kind).then(() => this.progress.tick('textures: ' + name))
          .catch((e) => { this.progress.tick(); logOnce('tex' + name + kind, 'texture missing', name, kind, e.message); }));
      }
    }
    await Promise.all(jobs);
  }

  // kit.tex(set, 'bc'|'n'|'orm', {blur}): cached; blur (0-6) is a softer copy, halved per step.
  tex(set, kind = 'bc', opts = {}) {
    const s = (this.manifest.textureSets || {})[set];
    const rel = s && s[kind];
    if (!rel) { logOnce('notex' + set + kind, 'no texture', set, kind); return this.placeholder[kind] || this.placeholder.bc; }
    const base = this.textures.get(this.url(rel));
    if (!base) { logOnce('unloaded' + set + kind, 'texture not loaded', set, kind); return this.placeholder[kind] || this.placeholder.bc; }
    const blur = Math.max(0, Math.min(6, Math.round(opts.blur || 0)));
    const rep = opts.uvScale && opts.uvScale !== 1 ? opts.uvScale : 0;
    if (!blur && !rep) return base;
    if (!blur) {
      // the same image, repeated: three shares the upload between textures with one source
      const key = this.url(rel) + '#rep' + rep;
      if (!this.textures.has(key)) { const t = base.clone(); t.repeat.set(rep, rep); t.needsUpdate = true; this.textures.set(key, t); }
      return this.textures.get(key);
    }
    const key = this.url(rel) + '#blur' + blur + (rep ? '#rep' + rep : '');
    if (this.textures.has(key)) return this.textures.get(key);
    const img = base.image;
    const w = Math.max(4, (img.width >> blur)), h = Math.max(4, (img.height >> blur));
    const c = document.createElement('canvas'); c.width = w; c.height = h;
    const g = c.getContext('2d');
    g.imageSmoothingQuality = 'high';
    g.drawImage(img, 0, 0, w, h);
    const t = new THREE.Texture(c);
    for (const k of ['flipY', 'wrapS', 'wrapT', 'colorSpace', 'anisotropy', 'minFilter', 'magFilter']) t[k] = base[k];
    if (rep) t.repeat.set(rep, rep);
    t.needsUpdate = true;
    this.textures.set(key, t);
    return t;
  }

  async loadPacks() {
    const packs = this.manifest.packs || [];
    this.progress.add(packs.length);
    this.packs = await Promise.all(packs.map(async (rel, i) => {
      try {
        const buf = await fetchBuffer(this.url(rel));
        const g = await this.gltf.parseAsync(buf, this.url(rel).replace(/[^/]*$/, ''));
        this.progress.tick('models: pack ' + i);
        return g;
      } catch (e) {
        this.progress.tick();
        console.warn('[StyleLab] pack failed', rel, e.message);
        return null;
      }
    }));
    let tangents = 0, plain = 0;
    for (const [name, m] of Object.entries(this.manifest.models || {})) {
      const g = this.packs[m.pack || 0];
      if (!g) continue;
      let root = null;
      for (const c of g.scene.children) if (c.name === m.node || c.name === name) { root = c; break; }
      if (!root) root = g.scene.getObjectByName(m.node) || g.scene.getObjectByName(name);
      if (!root) { logOnce('nonode' + name, 'model node missing in pack:', name); continue; }
      root.updateMatrixWorld(true);
      const inv = new THREE.Matrix4().copy(root.matrixWorld).invert();
      const parts = [];
      let skinned = false;
      root.traverse((o) => {
        if (!o.isMesh) return;
        if (o.isSkinnedMesh) skinned = true;
        const mats = Array.isArray(o.material) ? o.material : [o.material];
        const geo = o.geometry;
        if (geo.attributes.tangent) tangents++; else plain++;
        prepareGeometry(geo);
        // Multi-material meshes are split by group.
        if (mats.length > 1 && geo.groups.length) {
          for (const gr of geo.groups) {
            const sub = geo.clone();
            sub.setIndex(Array.from(geo.index.array.slice(gr.start, gr.start + gr.count)));
            sub.clearGroups();
            parts.push({ mesh: o, geometry: sub, materialName: mats[gr.materialIndex].name, matrix: inv.clone().multiply(o.matrixWorld) });
          }
        } else {
          parts.push({ mesh: o, geometry: geo, materialName: mats[0].name, matrix: inv.clone().multiply(o.matrixWorld) });
        }
      });
      this.models.set(name, { name, root, parts, skinned: skinned || !!m.skinned, info: m, sockets: m.sockets || {} });
    }
    this.hasTangents = tangents > 0 && plain === 0;
    if (tangents && plain) logOnce('mixedtan', 'some meshes carry tangents and some do not; normal map green is per geometry');
  }

  model(name) {
    const m = this.models.get(name);
    if (!m) logOnce('nomodel' + name, 'model missing from the packs:', name);
    return m || null;
  }

  async loadGLB(rel) {
    const buf = await fetchBuffer(this.url(rel));
    const g = await this.gltf.parseAsync(buf, this.url(rel).replace(/[^/]*$/, ''));
    g.scene.traverse((o) => { if (o.isMesh) prepareGeometry(o.geometry); });
    return g;
  }

  async loadHeights() {
    const h = this.manifest.terrain && this.manifest.terrain.heights;
    if (!h) return null;
    const buf = await fetchBuffer(this.url(h.file));
    return new Heightfield(new Float32Array(buf), h.w, h.h, h.rect);
  }
}

// Every mesh gets a 4-component colour (white, fully lit) when it has none, so the kit's shaders can read occlusion and
// wind from it without a variant; a 3-component one gains alpha 1.
export function prepareGeometry(geo) {
  const n = geo.attributes.position.count;
  const c = geo.attributes.color;
  if (!c) {
    const a = new Float32Array(n * 4).fill(1);
    geo.setAttribute('color', new THREE.BufferAttribute(a, 4));
  } else if (c.itemSize === 3) {
    const a = new Float32Array(n * 4);
    for (let i = 0; i < n; i++) { a[i * 4] = c.getX(i); a[i * 4 + 1] = c.getY(i); a[i * 4 + 2] = c.getZ(i); a[i * 4 + 3] = 1; }
    geo.setAttribute('color', new THREE.BufferAttribute(a, 4));
  }
  if (!geo.attributes.normal) geo.computeVertexNormals();
  if (!geo.attributes.uv) geo.setAttribute('uv', new THREE.BufferAttribute(new Float32Array(n * 2), 2));
}

// The terrain's heights over a rectangle (three metres, x along columns, z along rows).
export class Heightfield {
  constructor(data, w, h, rect) {
    this.data = data; this.w = w; this.h = h; this.rect = rect;
    this.fallback = null;   // (x, z) -> height or NaN, for points outside the rectangle
  }
  inside(x, z) { const r = this.rect; return x >= r[0] && x <= r[2] && z >= r[1] && z <= r[3]; }
  height(x, z) {
    const r = this.rect;
    if (!this.inside(x, z) && this.fallback) {
      const f = this.fallback(x, z);
      if (Number.isFinite(f)) return f;
    }
    // samples sit at cell centres: (c + 0.5) / w across the rectangle
    const gx = Math.min(Math.max((x - r[0]) / (r[2] - r[0]) * this.w - 0.5, 0), this.w - 1.001);
    const gz = Math.min(Math.max((z - r[1]) / (r[3] - r[1]) * this.h - 0.5, 0), this.h - 1.001);
    const c = Math.floor(gx), rr = Math.floor(gz), fx = gx - c, fz = gz - rr;
    const d = this.data, w = this.w, i = rr * w + c;
    const a = d[i], b = d[i + 1], e = d[i + w], f = d[i + w + 1];
    return (a + (b - a) * fx) * (1 - fz) + (e + (f - e) * fx) * fz;
  }
  normal(x, z, out = new THREE.Vector3(), e = 0.5) {
    const hx = this.height(x + e, z) - this.height(x - e, z);
    const hz = this.height(x, z + e) - this.height(x, z - e);
    return out.set(-hx, 2 * e, -hz).normalize();
  }
  slope(x, z, e = 0.75) {
    const hx = (this.height(x + e, z) - this.height(x - e, z)) / (2 * e);
    const hz = (this.height(x, z + e) - this.height(x, z - e)) / (2 * e);
    return Math.hypot(hx, hz);
  }
  // March a ray against the heights (for hits on the ground). Returns the distance or Infinity.
  raycast(origin, dir, maxDist = 300, step = 0.25) {
    let prev = 0, prevAbove = origin.y - this.height(origin.x, origin.z);
    if (prevAbove < 0) return 0;
    for (let t = step; t <= maxDist; t += step * (1 + t * 0.02)) {
      const x = origin.x + dir.x * t, y = origin.y + dir.y * t, z = origin.z + dir.z * t;
      const above = y - this.height(x, z);
      if (above < 0) {
        const f = prevAbove / (prevAbove - above);
        return prev + (t - prev) * f;
      }
      prev = t; prevAbove = above;
    }
    return Infinity;
  }
}
