// The Style Lab: boots the renderer, loads the export, builds Ransom's Rest, and switches between the styles.
// URL: ?style=N&shot=<id>&still=1&hud=0|1&ui=0&data=<export dir>   window.lab: setStyle, setShot, setHud, still, ready, styles
import * as THREE from 'three';
import { clone as skeletonClone } from 'three/addons/utils/SkeletonUtils.js';
import { Assets, Progress } from './assets.js';
import { MaterialKit, SHARED } from './materials.js';
import { buildTerrainMaterial } from './terrainmat.js';
import { Lighting, GUN_LAYER } from './lighting.js';
import { Sky, captureEnv } from './sky.js';
import { Post, makePostKit } from './post.js';
import { World } from './world.js';
import { Spiders } from './creatures.js';
import { Effects } from './fx.js';
import { Player } from './player.js';
import { SHOTS, MISSION } from './shots.js';
import { UI } from './ui.js';
import { createStubHud } from './hudstub.js';
import { inverseToneMap } from './tonecurve.js';
import { DEG, clamp, linColor, logOnce, mulberry32, nextFrame, vfovFromH, ueToThree } from './util.js';

const params = new URLSearchParams(location.search);
const OPT = {
  style: params.has('style') ? +params.get('style') : 0,
  shot: params.get('shot') || 'street',
  still: params.get('still') === '1',
  hud: params.get('hud') !== '0',
  ui: params.get('ui') !== '0',
  data: params.get('data') || 'data/',
  scale: params.has('scale') ? +params.get('scale') : 1,
  stats: params.get('stats') === '1',
  sync: params.get('sync') === '1',    // finish every frame (software GL in tests, so commands don't pile up)
};

class Engine {
  constructor() {
    this.time = 0; this.animTime = 0; this.animAcc = 0; this.animStep = 0;
    this.rand = mulberry32(7);
    this.cloned = [];            // meshes of cloned models (creatures, gun): { mesh, matName, owner }
    this.styleCache = new Map(); // style number -> { materials: Map, terrain, envRT }
    this.baseHFov = 80;
    this.isStill = OPT.still;
    this.hudOn = OPT.hud;
    this.timings = {};
    this.frames = 0;
    this._readyResolve = null;
    this.lab = this._labApi();
    window.lab = this.lab;
  }

  // ---------------------------------------------------------------- boot
  async boot() {
    const ui = this.ui = new UI({
      setStyle: (n) => this.setStyle(n), setShot: (id) => this.setShot(id), shotStep: (d) => this.shotStep(d),
      toggleHud: () => this.setHud(!this.hudOn), toggleCard: () => this.ui.showCard(!this.ui.cardShown),
    });
    ui.setVisible(OPT.ui);
    if (!OPT.ui) document.querySelector('#lab').classList.add('clean');
    const stage = document.querySelector('#stage');
    const renderer = this.renderer = new THREE.WebGLRenderer({ antialias: false, powerPreference: 'high-performance', alpha: false, stencil: false });
    renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 1) * OPT.scale);
    renderer.outputColorSpace = THREE.SRGBColorSpace;
    renderer.toneMapping = THREE.NoToneMapping;
    renderer.shadowMap.enabled = true;
    renderer.shadowMap.type = THREE.PCFShadowMap;
    renderer.shadowMap.autoUpdate = false;
    renderer.autoClear = false;
    renderer.info.autoReset = false;
    stage.appendChild(renderer.domElement);
    renderer.domElement.tabIndex = 0;

    const scene = this.scene = new THREE.Scene();
    scene.matrixWorldAutoUpdate = true;
    const camera = this.camera = new THREE.PerspectiveCamera(50, 16 / 9, 0.12, 18000);   // near 0.12: the depth buffer's precision at range (the gun starts 0.2 m out)
    camera.layers.enable(0);
    scene.add(camera);

    this.lighting = new Lighting(scene);
    this.sky = new Sky();
    scene.add(this.sky.mesh);
    this.sky.mesh.renderOrder = 1000;
    this.post = new Post(renderer);
    this.pmrem = new THREE.PMREMGenerator(renderer);
    this.placeholderMaterial = new THREE.MeshStandardMaterial({ color: 0x888888 });
    this.styleRoot = new THREE.Group(); this.styleRoot.name = 'StyleRoot';
    scene.add(this.styleRoot);

    this._resize();
    window.addEventListener('resize', () => this._resize());

    // load
    const progress = new Progress((p) => ui.progress(p.fraction * 0.85, p.label));
    const assets = this.assets = new Assets(OPT.data, renderer, progress);
    try {
      ui.progress(0.02, 'Reading the manifest');
      await assets.loadManifest();
    } catch (e) {
      ui.error('The export is missing: ' + e.message);
      console.error('[StyleLab] no manifest', e);
      return;
    }
    const hudP = this._loadHud();
    const stylesP = this._loadStyles();
    await Promise.all([assets.loadTextureSets(), assets.loadPacks(), (async () => { this.heights = await assets.loadHeights().catch((e) => { console.warn('[StyleLab] no heights', e.message); return null; }); })()]);
    this.world = new World(this);
    await this.world.loadMaps();
    await this.world.buildTerrain();
    ui.progress(0.88, 'Placing the town');
    this.world.buildInstances();
    this.world.placeCharacters();
    this.world.buildScatter();
    this.world.finishBatches();
    scene.add(this.world.root);
    this.lighting.setSources(assets.scene.lights || []);
    SHARED.uSlMacro.value = this.world.macroTex || null;
    SHARED.uSlMacroRect.value.set(...this.world.macroRect);
    if (this.world.macroMean) SHARED.uSlMacroMean.value.copy(this.world.macroMean);

    this.kitMaterials = new MaterialKit({ assets, manifest: assets.manifest, envMap: () => this.envMap });
    this.fx = new Effects(this);
    scene.add(this.fx.root);
    this.fx.buildSmoke(assets.scene.smoke || []);
    this.spiders = new Spiders(this);
    this.spiders.init(7);
    scene.add(this.spiders.root);
    this.player = new Player(this);
    this.player.buildGun();
    this.kit = this._makeKit();

    ui.progress(0.92, 'Loading the HUD and styles');
    this.hud = await hudP;
    this.styles = await stylesP;
    if (!this.styles.length) { ui.error('No styles could be loaded.'); return; }
    ui.setStyles(this.styles, OPT.style);
    ui.setShots(SHOTS, OPT.shot);
    ui.setHud(this.hudOn);
    this.hud.setMission(MISSION);
    this.hud.setArea('MAIN STREET', 'RANSOM’S REST');
    this.hud.setVisible(this.hudOn);
    this._input();

    ui.progress(0.95, 'Compiling the look');
    const want = this.styles.find((s) => s.number === OPT.style) ? OPT.style : this.styles[0].number;
    await this.setStyle(want, true);
    this.setShot(OPT.shot, true);
    ui.progress(1, 'Ready');
    ui.doneLoading();
    this.last = performance.now();
    this._loop = this._loop.bind(this);
    requestAnimationFrame(this._loop);
  }

  async _loadHud() {
    const host = document.querySelector('#stage');
    try {
      const m = await import('../hud/hud.js');
      const hud = m.createHud(host);
      if (hud.ready) await Promise.race([hud.ready, new Promise((r) => setTimeout(r, 4000))]);
      return hud;
    } catch (e) {
      console.warn('[StyleLab] HUD module missing, using the stub:', e.message);
      return createStubHud(host);
    }
  }

  async _loadStyles() {
    try {
      const m = await import('../styles/index.js');
      const list = await m.loadStyles((f, e) => console.warn('[StyleLab] style skipped:', f, e.message));
      // ?extra=a.js,b.js: style modules to try out alongside the listed ones (relative to the page)
      for (const u of (params.get('extra') || '').split(',').filter(Boolean)) {
        try { const x = (await import(new URL(u, location.href).href)).default; if (x && !list.some((s) => s.number === x.number)) list.push(x); }
        catch (e) { console.warn('[StyleLab] extra style failed', u, e.message); }
      }
      return list.sort((a, b) => a.number - b.number);
    } catch (e) {
      console.error('[StyleLab] styles/index.js failed', e);
      return [];
    }
  }

  _resize() {
    const w = window.innerWidth, h = window.innerHeight;
    this.renderer.setSize(w, h, true);
    const pr = this.renderer.getPixelRatio();
    this.post.setSize(w * pr, h * pr);
    this.camera.aspect = w / h;
    this.setHFov(this.hfov || this.baseHFov);
    if (this.hud) this.hud.resize(w, h);
    this.needsRender = true;
  }

  setHFov(h) {
    this.hfov = h;
    const v = vfovFromH(h, this.camera.aspect);
    if (Math.abs(this.camera.fov - v) > 1e-4) { this.camera.fov = v; this.camera.updateProjectionMatrix(); }
  }

  ground(x, z) { return this.heights ? this.heights.height(x, z) : 0; }
  groundColorAt(p) {
    const c = this.world.macroMean ? this.world.macroMean.clone() : new THREE.Color(0.3, 0.26, 0.2);
    return c.multiplyScalar(0.9);
  }

  // A model's clone for creatures and the gun, registered for material swaps.
  cloneModel(name, own = false) {
    const m = this.assets.model(name);
    if (!m) return null;
    const c = m.skinned ? skeletonClone(m.root) : m.root.clone(true);
    c.position.set(0, 0, 0); c.quaternion.identity(); c.scale.set(1, 1, 1);
    c.traverse((o) => {
      if (!o.isMesh) return;
      const mats = Array.isArray(o.material) ? o.material : [o.material];
      o.userData.matNames = mats.map((x) => x.name);
      o.castShadow = true; o.receiveShadow = true;
      if (o.isSkinnedMesh) o.frustumCulled = false;
      this.cloned.push({ mesh: o, own, owner: c });
    });
    return c;
  }

  // ---------------------------------------------------------------- the kit styles code to
  _makeKit() {
    const e = this;
    const km = this.kitMaterials;
    const kit = {
      THREE, renderer: this.renderer, scene: this.scene, camera: this.camera, manifest: this.assets.manifest,
      get time() { return e.animTime; },
      get style() { return e.style; },
      color: (hex) => linColor(hex),
      tex: (set, kind, opts) => e.assets.tex(set, kind, opts),
      pbr: (src, o) => km.pbr(src, o),
      toon: (src, o) => km.toon(src, o),
      flat: (src, o) => km.flat(src, o),
      terrainMaterial: (o) => buildTerrainMaterial(km, o || {}, {
        assets: e.assets, manifest: e.assets.manifest, macroTex: e.world.macroTex, masksTex: e.world.masksTex,
        macroMean: e.world.macroMean ? e.world.macroMean.clone() : null, ringTex: e.world.ringTex,
      }),
      post: makePostKit(this.post),
      styleRoot: this.styleRoot,
      viewmodel: this.player.gun,
      player: { position: this.player.pos, get yaw() { return e.player.yaw; } },
      lights: this.lighting.sources,
      sun: this.lighting,
      sky: this.sky,
      shared: SHARED,
      env: null,
      fx: {
        setFlash: (o) => e.fx.setFlash(o),
        resetFlash: () => e.fx.resetFlash(),
        get colors() { return e.fx.colors; },
      },
      heights: this._heightsKit(),
    };
    return kit;
  }

  // kit.heights: the ground's height for code (height, normal) and for shaders (a half-float texture over the rect).
  _heightsKit() {
    const e = this, H = this.heights;
    let tex = null;
    const uniforms = { uSlHeights: { value: null }, uSlHeightsRect: { value: new THREE.Vector4(...(H ? H.rect : [-1, -1, 1, 1])) } };
    const build = () => {
      if (tex) return tex;
      if (H) {
        const n = H.w * H.h, data = new Uint16Array(n);
        for (let i = 0; i < n; i++) data[i] = THREE.DataUtils.toHalfFloat(H.data[i]);
        tex = new THREE.DataTexture(data, H.w, H.h, THREE.RedFormat, THREE.HalfFloatType);
      } else {
        tex = new THREE.DataTexture(new Uint16Array([0]), 1, 1, THREE.RedFormat, THREE.HalfFloatType);
      }
      tex.minFilter = tex.magFilter = THREE.LinearFilter;
      tex.wrapS = tex.wrapT = THREE.ClampToEdgeWrapping;
      tex.flipY = false; tex.generateMipmaps = false; tex.needsUpdate = true;
      uniforms.uSlHeights.value = tex;
      return tex;
    };
    return {
      rect: H ? H.rect.slice() : null, w: H ? H.w : 0, h: H ? H.h : 0, data: H ? H.data : null,
      height: (x, z) => e.ground(x, z),
      normal: (x, z, out = new THREE.Vector3()) => (H ? H.normal(x, z, out) : out.set(0, 1, 0)),
      get texture() { return build(); },
      get uniforms() { build(); return uniforms; },
      glsl: 'uniform sampler2D uSlHeights;\nuniform vec4 uSlHeightsRect;\n'
        + 'float slGroundHeight(vec2 xz) { vec2 uv = (xz - uSlHeightsRect.xy) / (uSlHeightsRect.zw - uSlHeightsRect.xy); '
        + 'return texture2D(uSlHeights, clamp(uv, 0.0, 1.0)).r; }\n',
    };
  }

  // ---------------------------------------------------------------- styles
  async setStyle(n, initial = false, noStill = false) {
    const style = this.styles.find((s) => s.number === n);
    if (!style) { logOnce('nostyle' + n, 'no style', n); return false; }
    const t0 = performance.now();
    const token = (this._styleToken = (this._styleToken || 0) + 1);
    const prev = this.style;
    if (prev && prev.deactivate) { try { prev.deactivate(this.kit); } catch (err) { console.warn('[StyleLab] deactivate', err); } }
    for (const c of [...this.styleRoot.children]) this.styleRoot.remove(c);
    this.fx.resetFlash();
    this.style = style;
    const env = style.env || {};
    this.kit.env = env;
    // reset the scene's lanterns (a style may have recoloured them)
    this.lighting.setSources(this.assets.scene.lights || []);
    this.kit.lights = this.lighting.sources;
    this.lighting.setSun(env.sun || {});
    this.lighting.setAmbient(env.ambient || {});
    this.lighting.lightScale = env.lights && env.lights.scale !== undefined ? env.lights.scale : 1;
    this.post.exposure = env.exposure ?? 1;
    // unlit surfaces (the far backdrop) show about what sunlit ground of their colour would
    {
      const sun = env.sun || {}, amb = env.ambient || {};
      const el = Math.max(0.15, Math.sin((sun.elevation ?? 20) * DEG));
      const sky = linColor(amb.sky || '#8899bb');
      SHARED.uSlUnlit.value = ((sun.intensity ?? 3) * el + (amb.intensity ?? 0.6) * (0.2126 * sky.r + 0.7152 * sky.g + 0.0722 * sky.b)) / Math.PI;
    }
    this.post.toneMapping = env.toneMapping || 'aces';
    this.animStep = env.animStep || 0;
    this._setFog(env);
    // sky
    this.sky.custom = null;
    let customSky = null;
    if (style.sky) { try { customSky = style.sky(this.kit); } catch (err) { console.warn('[StyleLab] style.sky', err); } }
    this.sky.set(env.sky || {}, env, this.lighting.sunColor, this.lighting.sunIntensity);
    this.sky.mesh.visible = true;
    if (this._customSkyMesh) { this.scene.remove(this._customSkyMesh); this._customSkyMesh = null; }
    this.sky.mesh.material = this.sky.material;
    if (customSky && customSky.isMesh) {
      this.sky.mesh.visible = false; this.sky.custom = customSky; customSky.frustumCulled = false; customSky.userData.noNormal = true;
      this._customSkyMesh = customSky; this.scene.add(customSky);
    } else if (customSky && customSky.isMaterial) {
      this.sky.custom = customSky; this.sky.mesh.material = customSky;
    }
    // environment map from the sky
    let cache = this.styleCache.get(n);
    if (!cache) { cache = { materials: new Map() }; this.styleCache.set(n, cache); }
    if (!cache.envRT) cache.envRT = captureEnv(this.renderer, this.sky, this.pmrem, null);
    this.envMap = cache.envRT.texture;
    // materials
    this._applyMaterials(style, cache);
    // post
    let passes = null;
    try { passes = style.post ? style.post(this.kit) : null; } catch (err) { console.warn('[StyleLab] style.post', err); }
    if (!passes) passes = [this.kit.post.ao({}), this.kit.post.bloom({}), this.kit.post.grade({})];
    this.post.setPasses(passes);
    this.fx.setColors(style.fx || {});
    this.fx.setParticles(env.particles || {});
    this.fx.lightSmoke(this.lighting.sunColor, this.lighting.sunIntensity, this.lighting.hemi, env.smoke && env.smoke.opacity);
    if (style.activate) { try { style.activate(this.kit); } catch (err) { console.warn('[StyleLab] activate', err); } }
    this.lighting.repickLamps();
    this.ui.setActive(n, style);
    // compile everything before the first frame in the new look
    try { await this.renderer.compileAsync(this.scene, this.camera); } catch (err) { /* compile on first draw instead */ }
    if (token !== this._styleToken) return false;     // another switch came in meanwhile
    this._renderMinimap();
    this.timings['style' + n] = performance.now() - t0;
    this.needsRender = true;
    if (!initial && !this.isStill) this.ui.toast(`${style.number} · ${style.name}`);
    if (this.isStill && !noStill) await this._finishStill();
    return true;
  }

  _setFog(env) {
    const f = env.fog || {};
    const op = env.toneMapping || 'aces', ex = env.exposure ?? 1;
    SHARED.uSlFogColor.value.copy(inverseToneMap(linColor(f.color || '#b0b8c0'), op, ex));
    SHARED.uSlFogSunColor.value.copy(inverseToneMap(linColor(f.sunColor || f.color || '#ffd0a0'), op, ex));
    const base = f.height ?? 0;
    SHARED.uSlFog.value.set(f.density ?? 0.002, f.heightFalloff ?? 0.05, base, f.start ?? 0);
    SHARED.uSlFogSun.value.set(f.sunScatter ?? 0.5, f.sunExponent ?? 8, f.maxOpacity ?? 1, 0);
    const w = env.wind || {};
    const a = (w.direction ?? 60) * DEG;
    SHARED.uSlWind.value.set(-Math.sin(a), Math.cos(a), w.strength ?? 0.12, w.speed ?? 1.4);
  }

  _materialFor(style, cache, name) {
    if (cache.materials.has(name)) return cache.materials.get(name);
    const src = Object.assign({ name }, this.assets.manifest.materials[name] || { master: 'World', set: null, color: '#8a8a8a', roughness: 0.8, metallic: 0, role: 'other' });
    let m = null;
    try { m = style.material ? style.material(src, this.kit) : null; } catch (err) { logOnce('matfail' + style.number + name, 'style.material failed for', name, err.message); }
    if (!m) m = this.kitMaterials.pbr(src, {});
    m = this.kitMaterials.adopt(m);
    this._autoFlat(m);
    cache.materials.set(name, m);
    return m;
  }

  // flat materials follow the sun's and the sky's hue unless the style set them
  _autoFlat(m) {
    const sl = m.userData && m.userData.sl;
    if (!sl || sl.mode !== 'flat' || !sl.flatAuto) return;
    const u = sl.uniforms;
    const norm = (c) => { const l = 0.2126 * c.r + 0.7152 * c.g + 0.0722 * c.b; return c.clone().multiplyScalar(1 / Math.max(l, 1e-3)); };
    if (sl.flatAuto.lit) u.uSlFlatLit.value.copy(norm(this.lighting.sunColor)).multiplyScalar(typeof sl.opts.light === 'number' ? sl.opts.light : 1);
    if (sl.flatAuto.shade) u.uSlFlatShade.value.copy(norm(this.lighting.hemi.color)).multiplyScalar(0.45);
  }

  _applyMaterials(style, cache) {
    const km = this.kitMaterials;
    // terrain
    if (!cache.terrain) {
      let t = null;
      try { t = style.terrain ? style.terrain({ name: 'Terrain', master: 'Terrain', role: 'ground' }, this.kit) : null; } catch (err) { console.warn('[StyleLab] style.terrain', err); }
      cache.terrain = km.adopt(t || this.kit.terrainMaterial({}));
      this._autoFlat(cache.terrain);
    }
    for (const { mesh, matName, batched } of this.world.meshes) {
      let m = matName === '__terrain__' ? cache.terrain : this._materialFor(style, cache, matName);
      // a material drawn by BatchedMeshes gets its own copy (three recompiles a material shared by both kinds)
      if (batched) {
        if (!cache.batched) cache.batched = new Map();
        if (!cache.batched.has(m)) cache.batched.set(m, this._duplicate(m));
        m = cache.batched.get(m);
      }
      this._assign(mesh, m);
    }
    for (const { mesh, own, owner } of this.cloned) {
      const mats = mesh.userData.matNames.map((nm) => {
        const base = this._materialFor(style, cache, nm);
        if (!own) return base;
        // creatures get their own copy, so each can fade alone
        const key = owner.uuid + '|' + nm;
        if (!cache.own) cache.own = new Map();
        if (!cache.own.has(key)) cache.own.set(key, this._duplicate(base));
        return cache.own.get(key);
      });
      this._assign(mesh, mats.length === 1 ? mats[0] : mats);
    }
  }

  _duplicate(m) {
    const sl = m.userData && m.userData.sl;
    if (!sl || !sl.kit) return m;
    const raw = Object.assign({}, sl.rawOpts || {});
    const fresh = this.kitMaterials._build(sl.src, raw, sl.mode);
    this._autoFlat(fresh);
    return fresh;
  }

  _assign(mesh, m) {
    mesh.material = m;
    const one = Array.isArray(m) ? m[0] : m;
    mesh.customDepthMaterial = this.kitMaterials.depthFor(one) || undefined;
    mesh.userData.normalMat = this.kitMaterials.normalFor(one);
  }

  // ---------------------------------------------------------------- shots
  setShot(id, initial = false) {
    const shot = SHOTS.find((s) => s.id === id) || SHOTS[0];
    this.shot = shot;
    this.ui.setShot(shot.id);
    const p = ueToThree(shot.X, shot.Y);
    this.baseHFov = shot.fov;
    this.setHFov(shot.fov);
    const g = this.ground(p.x, p.z);
    this.player.placeAt(p.x, p.z, shot.yaw, shot.pitch);
    // the eye at the shot's height (the player's eye is 1.4 m; a higher shot lifts the feet)
    this.player.pos.y = g + shot.height / 100 - 1.4;
    this.player.flying = shot.height > 400;
    this.player.aiming = false; this.player.aim = 0;
    const sh = shot.shadow || { near: 0.1, far: 220, lambda: 0.75 };
    this.lighting.setShadowRange(sh.near, sh.far, sh.lambda);
    this.lighting.repickLamps();
    this.player.gun.visible = shot.hud !== false;
    this.gunVisible = shot.hud !== false;
    this.hud.setArea(shot.place || '', shot.place ? 'RANSOM’S REST' : '');
    this.hud.setVisible(this.hudOn && shot.hud !== false);
    this.world.updateScatter(this.camera.position.set(p.x, g + shot.height / 100, p.z), true);
    this.needsRender = true;
    if (this.isStill) this._finishStill();
  }
  shotStep(d) {
    const i = SHOTS.findIndex((s) => s.id === (this.shot && this.shot.id));
    this.setShot(SHOTS[(i + d + SHOTS.length) % SHOTS.length].id);
  }

  setHud(on) {
    this.hudOn = on;
    this.ui.setHud(on);
    this.hud.setVisible(on && (!this.shot || this.shot.hud !== false));
    this.needsRender = true;
  }

  // ---------------------------------------------------------------- stills
  // Poses the shot's creatures and effects at a fixed time, renders the final frame, then resolves lab.ready.
  async _finishStill() {
    const token = (this._stillToken = (this._stillToken || 0) + 1);
    if (!this.lab.ready || this.lab.ready === true) this._newReady();
    await nextFrame();
    if (token !== this._stillToken || !this.style || !this.shot) return;
    const shot = this.shot;
    this.time = 7.25; this.animTime = 7.25;
    SHARED.uSlTime.value = this.animTime; SHARED.uSlAnimTime.value = this.animTime;
    this.fx.clear();
    this.player.placeAt(this.player.pos.x, this.player.pos.z, shot.yaw, shot.pitch);
    const p = ueToThree(shot.X, shot.Y);
    this.player.pos.y = this.ground(p.x, p.z) + shot.height / 100 - 1.4;
    Object.assign(this.player, { health: shot.hudState.health, mag: shot.hudState.mag, reserve: 168, reloading: 0, vel: new THREE.Vector3() });
    this.player.update(0, true);
    this.spiders.pose(shot.spiders || [], this.camera.position, shot.yaw);
    // the HUD settles on this shot's state (every hit, heal and flash from the last shot runs out), then freezes
    if (this.hud.ready) await this.hud.ready;
    this.hud.unfreeze && this.hud.unfreeze();
    const hs = this.player.state();
    for (let i = 0; i < 40; i++) this.hud.update(0.25, hs);
    this.hud.freeze(0);
    if (shot.fight) {
      // the frame just after a burst: a shot into the ground by the spider (sparks), then one into it (blood, hit marker)
      this.player.update(0, true);
      const sp = this.spiders.list[0];
      const save = { yaw: this.player.yaw, pitch: this.player.pitch };
      if (sp) {
        const aimAt = (pt) => {
          const d = pt.clone().sub(this.camera.position).normalize();
          this.player.yaw = (Math.atan2(-d.x, d.z) / DEG + 360) % 360; this.player.pitch = Math.asin(d.y) / DEG;
          this.player.update(0, true);
        };
        const body = sp.pos.clone(); body.y += this.spiders.height * 0.55;
        aimAt(body.clone().add(new THREE.Vector3(1.2, -1.0, 0.6)));
        this.player.fire({ noSpread: true, noRecoil: true });
        this.fx.add.update(0.06); this.fx.alpha.update(0.06);
        aimAt(body);
        this.player.fire({ noSpread: true, noRecoil: true });
        this.fx.add.update(0.016); this.fx.alpha.update(0.016);
        // the spider keeps its pose (the hit only marks it)
        sp.hp = 3; sp.state = 'leap';
      }
      Object.assign(this.player, save, { mag: shot.hudState.mag });
      this.player.update(0, true);
      this.fx.flashT = 0.05; this.fx.flashI = 1; this.fx.frozenFlash = true;
      this.fx.update(0, this.camera);
      this.fx.flashMat && (this.fx.flashMat.uniforms.uI.value = 0.9);
      if (this.fx.flash) this.fx.flash.visible = true;
      this.lighting.muzzle.intensity = 20;
      this.hud.step && this.hud.step(0.06);
    } else {
      this.lighting.muzzle.intensity = 0;
      if (this.fx.flash) this.fx.flash.visible = false;
    }
    this.fx.add.upload(); this.fx.alpha.upload();
    this.hud.update(0, this.player.state());
    this.world.updateScatter(this.camera.position, true);
    try { await this.renderer.compileAsync(this.scene, this.camera); } catch (e) { /* fine */ }
    if (document.fonts && document.fonts.ready) await document.fonts.ready;
    if (token !== this._stillToken) return;
    // two frames: the first settles shadows and targets, the second is the one kept
    for (let i = 0; i < 2; i++) {
      this._render(0);
      await nextFrame();
      if (token !== this._stillToken) return;
    }
    this.renderer.getContext().finish();
    this.needsRender = false;
    this._resolveReady();
  }

  _newReady() {
    let res;
    const p = new Promise((r) => { res = r; });
    this._readyResolve = res;
    this.lab.ready = p;
  }
  _resolveReady() {
    const r = this._readyResolve;
    this._readyResolve = null;
    this.lab.ready = true;
    this.lab.isReady = true;
    if (r) r(true);
  }

  still(on) {
    this.isStill = !!on;
    if (on) { this._newReady(); this._finishStill(); }
    else {
      this.hud.unfreeze && this.hud.unfreeze();
      this.fx.clear();
      for (const s of this.spiders.list) this.spiders._respawn(s);
      this.lighting.muzzle.intensity = 0;
    }
  }

  // ---------------------------------------------------------------- input
  _input() {
    const c = this.renderer.domElement;
    const p = this.player;
    this.locked = false;
    c.addEventListener('click', () => {
      if (this.isStill) return;
      if (!this.locked) { try { const r = c.requestPointerLock(); if (r && r.catch) r.catch(() => {}); } catch (e) { /* no lock: drag to look */ } }
    });
    document.addEventListener('pointerlockchange', () => {
      this.locked = document.pointerLockElement === c;
      this.ui.setLocked(this.locked);
      if (!this.locked) { p.firing = false; p.aiming = false; p.keys.clear(); }
    });
    let drag = null;
    c.addEventListener('mousedown', (e) => {
      if (this.isStill) return;
      if (this.locked) {
        if (e.button === 0) { p.firing = true; p.tap = true; }
        if (e.button === 2) p.aiming = true;
      } else if (e.button === 0) drag = { x: e.clientX, y: e.clientY };
    });
    window.addEventListener('mouseup', (e) => { if (e.button === 0) { p.firing = false; drag = null; } if (e.button === 2) p.aiming = false; });
    window.addEventListener('mousemove', (e) => {
      if (this.isStill) return;
      if (this.locked) p.onMouse(e.movementX, e.movementY);
      else if (drag) { p.onMouse(e.clientX - drag.x, e.clientY - drag.y); drag.x = e.clientX; drag.y = e.clientY; }
    });
    c.addEventListener('contextmenu', (e) => e.preventDefault());
    window.addEventListener('keydown', (e) => {
      if (e.target && (e.target.tagName === 'SELECT' || e.target.tagName === 'INPUT')) return;
      const k = e.code;
      if (k === 'Space' || k.startsWith('Arrow')) e.preventDefault();
      if (/^Digit[0-9]$/.test(k)) { const d = +k.slice(5); this.setStyle(d === 0 ? 10 : d); return; }
      if (k === 'Backquote') { this.setStyle(0); return; }
      if (k === 'KeyV') { this.shotStep(1); return; }
      if (k === 'KeyB') { this.shotStep(-1); return; }
      if (k === 'KeyH') { this.setHud(!this.hudOn); return; }
      if (k === 'KeyI') { this.ui.showCard(!this.ui.cardShown); return; }
      if (k === 'KeyU') { const lab = document.querySelector('#lab'); const clean = !lab.classList.contains('clean'); lab.classList.toggle('clean', clean); this.ui.setVisible(!clean); return; }
      if (k === 'KeyR') { p.reload(); return; }
      p.keys.add(k);
    });
    window.addEventListener('keyup', (e) => p.keys.delete(e.code));
    window.addEventListener('blur', () => { p.keys.clear(); p.firing = false; });
  }

  onPlayerBitten(amount) {
    const p = this.player;
    if (!p.alive || this.isStill) return;
    p.health = Math.max(0, p.health - amount);
    this.hud.onPlayerHit(amount);
    p.recoilV.x -= 2.5; p.recoilV.y += (this.rand() - 0.5) * 3;
    if (p.health <= 0) { p.alive = false; p.respawnT = 2.5; this.ui.toast('Down'); }
  }

  // ---------------------------------------------------------------- frame
  _loop(now) {
    requestAnimationFrame(this._loop);
    const dtReal = Math.min(0.05, Math.max(0, (now - this.last) / 1000));
    this.last = now;
    if (this.isStill) {
      if (this.needsRender && !this._readyResolve) { this._render(0); this.needsRender = false; }
      return;
    }
    const t0 = performance.now();
    this._simulate(dtReal);
    this._render(dtReal);
    if (OPT.sync) this.renderer.getContext().finish();
    const cpu = performance.now() - t0;
    this.frames++;
    this._fpsAcc = (this._fpsAcc || 0) + dtReal; this._fpsN = (this._fpsN || 0) + 1; this._cpuAcc = (this._cpuAcc || 0) + cpu;
    if (this._fpsAcc > 0.5) {
      this.fps = this._fpsN / this._fpsAcc; this.cpuMs = this._cpuAcc / this._fpsN;
      this._fpsAcc = 0; this._fpsN = 0; this._cpuAcc = 0;
      if (OPT.stats) this.ui.stats(`${this.fps.toFixed(0)} fps · cpu ${this.cpuMs.toFixed(1)} ms · ${this.renderer.info.render.calls} draws · ${(this.renderer.info.render.triangles / 1e6).toFixed(2)} M tris`);
    }
  }

  // One step of the world: time, the player, spiders, effects, scatter, the style's update and the HUD.
  _simulate(dtReal) {
    this.time += dtReal;
    // stepped animation (env.animStep): creatures and effects move in steps, the camera and HUD stay smooth
    let adt = dtReal;
    if (this.animStep > 0) {
      this.animAcc += dtReal; adt = 0;
      if (this.animAcc >= this.animStep) { adt = Math.floor(this.animAcc / this.animStep) * this.animStep; this.animAcc -= adt; }
    }
    this.animTime += adt;
    SHARED.uSlTime.value = this.animTime; SHARED.uSlAnimTime.value = this.animTime;
    const p = this.player;
    if (!p.alive) {
      p.respawnT -= dtReal;
      if (p.respawnT <= 0) { p.alive = true; p.health = p.maxHealth; this.hud.onHeal(p.maxHealth); this.setShot('street'); }
    }
    if (p.flying && (p.keys.size || p.firing)) p.flying = false;
    if (!p.flying) p.update(dtReal, false); else p.update(dtReal, true);
    this.spiders.update(adt, { pos: p.pos, alive: p.alive && !p.flying });
    this.fx.update(adt, this.camera);
    this.world.updateScatter(this.camera.position);
    if (this.style && this.style.update) { try { this.style.update(dtReal, this.time, this.kit); } catch (err) { logOnce('upd' + this.style.number, 'style.update', err.message); } }
    this.hud.update(dtReal, p.state());
  }

  _render(dt, forceShadows = false) {
    const cam = this.camera;
    cam.updateMatrixWorld(true);
    this.lighting.update(cam, dt, forceShadows || this.isStill);
    this.sky.follow(cam, this.post.size.y);
    if (this._customSkyMesh) this._customSkyMesh.position.copy(cam.position);
    const c = this.post.common;
    c.uTime.value = this.animTime;
    this.lighting.sunScreen(cam, c.uSunScreen.value);
    this.renderer.info.reset();
    this.post.render(cam, (t) => this._drawScene(t), (t) => this._drawNormals(t));
  }

  _drawScene(target) {
    const r = this.renderer, cam = this.camera;
    r.setRenderTarget(target);
    r.setClearColor(0x000000, 1);
    r.clear(true, true, true);
    cam.layers.set(0);
    r.shadowMap.needsUpdate = true;
    r.render(this.scene, cam);
    if (this.gunVisible && this.player.gun.visible) this._drawGun();
  }

  // The gun over the world without clearing the depth (the post passes need the world's): its depth is squeezed into
  // the first thousandth of the range, so it always wins and still sorts against itself.
  _drawGun() {
    const r = this.renderer, cam = this.camera;
    const P = cam.projectionMatrix, saved = P.clone(), e = P.elements, k = 0.001;
    for (const i of [0, 1, 2, 3]) e[i * 4 + 2] = saved.elements[i * 4 + 2] * k + saved.elements[i * 4 + 3] * (k - 1);
    cam.layers.set(GUN_LAYER);
    r.render(this.scene, cam);
    cam.layers.set(0);
    P.copy(saved);
  }

  _drawNormals(target) {
    const r = this.renderer, cam = this.camera;
    const swapped = [];
    this.scene.traverseVisible((o) => {
      if (!o.isMesh && !o.isPoints) return;
      if (o.userData.noNormal || !o.userData.normalMat || o.isPoints) { swapped.push([o, null, o.visible]); o.visible = false; return; }
      swapped.push([o, o.material, true]);
      o.material = o.userData.normalMat;
    });
    r.setRenderTarget(target);
    r.setClearColor(0x8080ff, 1);
    r.clear(true, true, true);
    cam.layers.set(0);
    r.render(this.scene, cam);
    if (this.gunVisible && this.player.gun.visible) this._drawGun();
    for (const [o, m, vis] of swapped) { if (m) o.material = m; else o.visible = vis; }
    r.setClearColor(0x000000, 1);
  }

  // The region from above, once per style, for the HUD's minimap.
  _renderMinimap() {
    if (!this.world || !this.hud) return;
    const rect = this.world.macroRect;
    const w = rect[2] - rect[0], h = rect[3] - rect[1];
    const W = 1024, H = Math.round(1024 * h / w);
    const cam = new THREE.OrthographicCamera(-w / 2, w / 2, h / 2, -h / 2, 1, 2000);
    cam.position.set((rect[0] + rect[2]) / 2, 900, (rect[1] + rect[3]) / 2);
    cam.up.set(0, 0, -1);
    cam.lookAt(cam.position.x, 0, cam.position.z);
    cam.updateMatrixWorld(true);
    const hdr = new THREE.WebGLRenderTarget(W, H, { type: THREE.HalfFloatType });
    const ldr = new THREE.WebGLRenderTarget(W, H, { type: THREE.UnsignedByteType });
    const hide = [this.sky.mesh, this.spiders.root, this.fx.root, this.player.gunPivot, this.styleRoot, this._customSkyMesh].filter(Boolean);
    const vis = hide.map((o) => o.visible);
    hide.forEach((o) => { o.visible = false; });
    const fog = SHARED.uSlFog.value.clone(), shadow = SHARED.uSlShadowStrength.value;
    SHARED.uSlFog.value.x = 0; SHARED.uSlShadowStrength.value = 0;
    const r = this.renderer;
    r.setRenderTarget(hdr); r.setClearColor(0x101418, 1); r.clear();
    r.render(this.scene, cam);
    this.post.tonemapPass.uniforms.uExposure.value = this.post.exposure;
    this.post.tonemapPass.uniforms.uOp.value = { aces: 1, agx: 2, neutral: 3, none: 0 }[this.post.toneMapping] ?? 1;
    this.post.tonemapPass.uniforms.uDither.value = 0;
    this.post.draw(this.post.tonemapPass, ldr, hdr.texture);
    const px = new Uint8Array(W * H * 4);
    r.readRenderTargetPixels(ldr, 0, 0, W, H, px);
    SHARED.uSlFog.value.copy(fog); SHARED.uSlShadowStrength.value = shadow;
    hide.forEach((o, i) => { o.visible = vis[i]; });
    r.setRenderTarget(null);
    hdr.dispose(); ldr.dispose();
    const canvas = this._minimapCanvas || (this._minimapCanvas = document.createElement('canvas'));
    canvas.width = W; canvas.height = H;
    const g = canvas.getContext('2d');
    const img = g.createImageData(W, H);
    for (let y = 0; y < H; y++) img.data.set(px.subarray((H - 1 - y) * W * 4, (H - y) * W * 4), y * W * 4);
    g.putImageData(img, 0, 0);
    this.hud.setMinimap(canvas, rect.slice());
  }

  // ---------------------------------------------------------------- window.lab
  _labApi() {
    const e = this;
    let res;
    const api = {
      ready: new Promise((r) => { res = r; }),
      isReady: false,
      get styles() { return (e.styles || []).map((s) => ({ number: s.number, id: s.id, name: s.name, family: s.family })); },
      setStyle: async (n) => { if (e.isStill) e._newReady(); const ok = await e.setStyle(n); return ok; },
      setShot: (id) => { if (e.isStill) e._newReady(); e.setShot(id); return e.isStill ? e.lab.ready : Promise.resolve(true); },
      setHud: (on) => { e.setHud(!!on); if (e.isStill) { e._newReady(); e._finishStill(); } },
      still: (on) => { e.still(on); return e.lab.ready; },
      // tests: run the world for `seconds` in fixed steps without drawing (software GL is too slow to play in real time)
      tick: (seconds = 1, dt = 1 / 30) => { if (e.isStill) return; for (let t = 0; t < seconds - 1e-6; t += dt) e._simulate(dt); },
      // one still of a style at a shot (a single render)
      view: async (n, id) => { if (e.isStill) e._newReady(); await e.setStyle(n, false, true); e.setShot(id); if (!e.isStill) return true; return e.lab.ready; },
      state: () => ({
        style: e.style && e.style.number, shot: e.shot && e.shot.id, fps: e.fps, cpuMs: e.cpuMs, frames: e.frames,
        draws: e.renderer.info.render.calls, triangles: e.renderer.info.render.triangles, timings: e.timings,
        player: e.player && { pos: e.player.pos.toArray(), health: e.player.health, mag: e.player.mag, yaw: e.player.yaw },
        spiders: e.spiders && e.spiders.list.map((s) => ({ state: s.state, alive: s.alive, hp: s.hp, pos: s.pos.toArray() })),
        programs: e.renderer.info.programs ? e.renderer.info.programs.length : 0, still: e.isStill,
      }),
      engine: e,
    };
    this._readyResolve = (v) => res(v);
    return api;
  }
}

const engine = new Engine();
engine.boot().then(() => {
  if (!engine.isStill && engine._readyResolve) {
    requestAnimationFrame(() => requestAnimationFrame(() => engine._resolveReady()));
  }
}).catch((e) => {
  console.error('[StyleLab] boot failed', e);
  try { engine.ui && engine.ui.error('Something went wrong: ' + e.message); } catch (x) { /* ignore */ }
});
