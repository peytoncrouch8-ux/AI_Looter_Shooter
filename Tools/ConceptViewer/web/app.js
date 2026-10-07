
// ================================================================ expanding a spec into placed items
const KIT_RADIUS = { shed: 170, coop: 120, stall: 140, haystack: 140, dock: 110, burrow: 160, barricade: 140, sandbags: 120, scarecrow: 60,
  dummy: 50, target: 70, apple: 190, log: 260, stump: 45, sluice: 110, jetty: 300, oak: 230, birch: 140, pine: 200, deadpine: 120 };
function itemRadius(it) {
  const name = String(it.model), s = Array.isArray(it.scale) ? Math.max(it.scale[0], it.scale[2]) : (it.scale || 1);
  if (name.startsWith('k:')) return (KIT_RADIUS[name.slice(2).split(':')[0]] || 0) * s;
  return modelRadius(name) * 100 * s;
}
// Fences follow each edge of their line in equal pieces (stretched to fit) and tilt with the ground, so corners close.
function expandFences(S) {
  const out = [];
  for (const f of S.fences) {
    const unit = f.kind === 'picket' ? 200 : 300;
    const model = f.kind === 'stone' ? 'StoneWall' : f.kind === 'picket' ? 'k:picket' : 'FenceRail';
    let k = 0;
    for (let i = 0; i < f.points.length - 1; i++) {
      const [ax, ay] = f.points[i], [bx, by] = f.points[i + 1], len = Math.hypot(bx - ax, by - ay);
      if (len < 40) continue;
      const n = Math.max(1, Math.round(len / unit)), seg = len / n, yaw = ueYawOf(bx - ax, by - ay);
      for (let j = 0; j < n; j++, k++) {
        if (f.gap !== undefined && f.kind !== 'stone' && k === f.gap && f.points.length > 3) continue;
        const X = ax + (bx - ax) * j / n, Y = ay + (by - ay) * j / n, X2 = ax + (bx - ax) * (j + 1) / n, Y2 = ay + (by - ay) * (j + 1) / n;
        const h0 = groundUE(X, Y), h1 = groundUE(X2, Y2);
        if (Number.isNaN(h0) || Number.isNaN(h1)) continue;
        const broken = f.broken && f.broken.includes(k) && f.kind === 'rail';
        out.push({ model: broken ? 'FenceBroken' : model, X, Y, yaw: yaw + 90, scale: [seg / unit, 1, 1], sink: 0.05, tiltZ: Math.atan2(h1 - h0, seg / 100) });
      }
      if (f.kind === 'rail' && !Number.isNaN(groundUE(bx, by))) out.push({ model: 'FencePost', X: bx, Y: by, yaw: yaw + 90, sink: 0.05 });
    }
  }
  return out;
}
function expandHedges(S) {
  const out = [];
  for (const h of S.hedges) alongLine(h.pts, 205, (X, Y, yaw, k) => { if (!Number.isNaN(groundUE(X, Y)) && slopeUE(X, Y) < 0.7) out.push({ model: `k:hedge:${k % 3}`, X, Y, yaw: yaw + 90 + ((k * 37) % 17 - 8), sink: 0.12, scale: 0.9 + ((k * 13) % 5) * 0.05 }); });
  return out;
}
function expandFlowerBeds(S) {
  const out = [];
  for (const b of S.flowerBeds) alongLine(b.pts, b.every || 40, (X, Y, yaw, k) => { if (!Number.isNaN(groundUE(X, Y))) out.push({ model: `k:flower:${b.colors[k % b.colors.length]}`, X, Y, yaw: k * 47, scale: 1.1 }); });
  return out;
}

// ================================================================ scattering vegetation, rocks and grass
const ROAD_MARGIN = { tree: 300, bush: 140, rock: 150, flower: 50, tuft: 35, reed: 30 };
const ITEM_MARGIN = { tree: 260, bush: 120, rock: 140, flower: 60, tuft: 40, reed: 20 };
function makeBlocker(S) {
  const cell = 1000, pad = 400, grid = new Map();
  for (const it of S.items) {
    const r = itemRadius(it); if (r < 35) continue;
    const e = { X: it.X, Y: it.Y, r };
    for (let gx = Math.floor((it.X - r - pad) / cell); gx <= Math.floor((it.X + r + pad) / cell); gx++)
      for (let gy = Math.floor((it.Y - r - pad) / cell); gy <= Math.floor((it.Y + r + pad) / cell); gy++) { const key = gx * 10007 + gy; if (!grid.has(key)) grid.set(key, []); grid.get(key).push(e); }
  }
  const roads = S.roads.map((r) => ({ pts: r.points, hw: r.width / 2 }));
  const fields = S.fields.map((f) => f.poly), solid = S.patches.filter((p) => p.block).map((p) => p.poly);
  // Nothing tall grows on an on-foot viewpoint or in a narrow lane straight ahead of it.
  const eyes = S.views.foot.map((v) => ({ X: v.X, Y: v.Y, fx: Math.cos(v.yaw * DEG), fy: Math.sin(v.yaw * DEG) }));
  return (X, Y, kind) => {
    if (kind === 'tree' || kind === 'bush' || kind === 'rock') for (const e of eyes) {
      const dx = X - e.X, dy = Y - e.Y, d = Math.hypot(dx, dy);
      if (d < 700 || (d < 2200 && (dx * e.fx + dy * e.fy) / d > 0.87)) return true;
    }
    if (Number.isNaN(groundUE(X, Y))) return true;
    const sl = slopeUE(X, Y);
    if (sl > (kind === 'tree' ? 0.6 : kind === 'tuft' || kind === 'flower' || kind === 'reed' ? 1.2 : 0.8)) return true;
    const rm = ROAD_MARGIN[kind];
    for (const r of roads) if (distToPolyline(X, Y, r.pts) < r.hw + rm) return true;
    const list = grid.get(Math.floor(X / cell) * 10007 + Math.floor(Y / cell));
    if (list) for (const e of list) if (Math.hypot(X - e.X, Y - e.Y) < e.r + ITEM_MARGIN[kind]) return true;
    if (kind === 'reed') return false;
    for (const f of fields) if (insidePoly(X, Y, f)) return true;
    for (const p of solid) if (insidePoly(X, Y, p)) return true;
    for (const w of S.water) if (Math.hypot(X - w.X, Y - w.Y) < w.r + 80) return true;
    const px = (X - POND.center[0]) / (POND.radii[0] + 180), py = (Y - POND.center[1]) / (POND.radii[1] + 180);
    if (px * px + py * py < 1) return true;
    if (distToPolyline(X, Y, CREEK.path) < 380) return true;
    if (kind === 'tree') for (const z of S.noTrees) if (z.poly ? insidePoly(X, Y, z.poly) : Math.hypot(X - z.X, Y - z.Y) < z.r) return true;
    return false;
  };
}
function pickMix(mix, rnd) { const total = Object.values(mix).reduce((a, b) => a + b, 0); let r = rnd() * total; for (const [k, w] of Object.entries(mix)) { r -= w; if (r <= 0) return k; } return Object.keys(mix)[0]; }
function scatterVeg(S, blocked) {
  const rnd = mulberry32(S.seed), out = [];
  const tint = () => { const v = Math.round(222 + rnd() * 33).toString(16).padStart(2, '0'); return `#${v}${v}${v}`; };
  const tree = (kind, X, Y) => out.push({ model: `k:${kind}:${Math.floor(rnd() * (TREE_VARIANTS[kind] || 1))}`, X, Y, yaw: rnd() * 360, scale: 0.85 + rnd() * 0.4, sink: 0.12, color: tint() });
  const gridIn = (poly, spacing, fn) => {
    const xs = poly.map((p) => p[0]), ys = poly.map((p) => p[1]);
    for (let X = Math.min(...xs); X < Math.max(...xs); X += spacing) for (let Y = Math.min(...ys); Y < Math.max(...ys); Y += spacing) {
      const jx = X + (rnd() - 0.5) * spacing * 0.9, jy = Y + (rnd() - 0.5) * spacing * 0.9;
      if (insidePoly(jx, jy, poly)) fn(jx, jy);
    }
  };
  for (const v of S.veg) {
    if (v.kind === 'woods') gridIn(v.poly, v.spacing, (X, Y) => { if (v.clear && v.clear.some((c) => Math.hypot(X - c.X, Y - c.Y) < c.r)) return; if (!blocked(X, Y, 'tree')) tree(pickMix(v.mix, rnd), X, Y); });
    else if (v.kind === 'grove') { let made = 0; for (let i = 0; i < v.n * 5 && made < v.n; i++) { const a = rnd() * 6.283, r = Math.sqrt(rnd()) * v.r, X = v.X + Math.cos(a) * r, Y = v.Y + Math.sin(a) * r; if (!blocked(X, Y, 'tree')) { tree(pickMix(v.mix, rnd), X, Y); made++; } } }
    else if (v.kind === 'scatter') { const xs = v.poly.map((p) => p[0]), ys = v.poly.map((p) => p[1]); let made = 0; for (let t = 0; t < v.n * 14 && made < v.n; t++) { const X = lerp(Math.min(...xs), Math.max(...xs), rnd()), Y = lerp(Math.min(...ys), Math.max(...ys), rnd()); if (insidePoly(X, Y, v.poly) && !blocked(X, Y, 'tree')) { tree(v.mix ? pickMix(v.mix, rnd) : v.tree, X, Y); made++; } } }
    else if (v.kind === 'line') alongLine(offsetPolyline(v.pts, v.offset || 0), v.every, (X, Y) => { const jx = X + (rnd() - 0.5) * 200, jy = Y + (rnd() - 0.5) * 200; if (!blocked(jx, jy, 'tree')) tree(v.tree, jx, jy); });
    else if (v.kind === 'bushes') gridIn(v.poly, v.spacing, (X, Y) => {
      if (v.ring) { const k = Math.hypot(X, Y) / Math.max(1, Math.hypot(...nearestOutline(X, Y))); if (k < v.ring) return; }
      if (!blocked(X, Y, 'bush')) out.push({ model: `k:bush:${Math.floor(rnd() * 3)}`, X, Y, yaw: rnd() * 360, scale: 0.75 + rnd() * 0.6, sink: 0.15, color: tint() });
    });
    else if (v.kind === 'flowers') {
      // A wildflower meadow: a lighter, warmer sward under dense clumps, so it reads as a meadow from the air too.
      const cols = v.colors || ['pink', 'yellow', 'white', 'blue', 'red'];
      if (v.tint !== false) S.patches.push({ poly: v.poly, color: v.tint || PAL.meadow, lift: 0.035 });
      gridIn(v.poly, v.spacing, (X, Y) => { if (!blocked(X, Y, 'flower')) out.push({ model: `k:flower:${cols[Math.floor(rnd() * cols.length)]}`, X, Y, yaw: rnd() * 360, scale: 1.0 + rnd() * 0.7 }); });
    }
    else if (v.kind === 'rocks') { const xs = v.poly.map((p) => p[0]), ys = v.poly.map((p) => p[1]); const kinds = ['Boulder_A', 'Boulder_B', 'Boulder_C', 'Rock_A', 'Rock_B', 'Rock_A', 'Rock_B']; let made = 0;
      for (let t = 0; t < v.n * 6 && made < v.n; t++) { const X = lerp(Math.min(...xs), Math.max(...xs), rnd()), Y = lerp(Math.min(...ys), Math.max(...ys), rnd()); if (insidePoly(X, Y, v.poly) && !blocked(X, Y, 'rock')) { out.push({ model: kinds[Math.floor(rnd() * kinds.length)], X, Y, yaw: rnd() * 360, scale: 0.75 + rnd() * 0.5, sink: 0.08 }); made++; } } }
    else if (v.kind === 'reeds') {
      const add = (X, Y) => { if (!blocked(X, Y, 'reed')) out.push({ model: rnd() < 0.3 && MANIFEST.models.Reeds_B ? 'Reeds_B' : `k:reeds:${Math.floor(rnd() * 2)}`, X, Y, yaw: rnd() * 360, scale: 0.8 + rnd() * 0.5, sink: 0.1 }); };
      if (v.ring) { const n = Math.ceil(Math.PI * (v.ring.rx + v.ring.ry) / v.every); for (let i = 0; i < n; i++) { const a = i / n * 6.283; add(v.ring.X + Math.cos(a) * v.ring.rx + (rnd() - 0.5) * 140, v.ring.Y + Math.sin(a) * v.ring.ry + (rnd() - 0.5) * 140); } }
      else gridIn(v.poly, v.spacing, add);
    }
  }
  // grass tufts over the open ground, and verges along the roads
  if (S.tuftField) { const sp = S.tuftField.spacing; for (let X = -9800; X < 9800; X += sp) for (let Y = -9800; Y < 9800; Y += sp) { const jx = X + (rnd() - 0.5) * sp, jy = Y + (rnd() - 0.5) * sp; if (!blocked(jx, jy, 'tuft')) out.push({ model: `k:tuft:${Math.floor(rnd() * 3)}`, X: jx, Y: jy, yaw: rnd() * 360, scale: 0.8 + rnd() * 0.7 }); } }
  for (const t of S.tufts) for (const side of [-1, 1]) alongLine(offsetPolyline(t.along, side * t.offset), t.every, (X, Y) => { const jx = X + (rnd() - 0.5) * 60, jy = Y + (rnd() - 0.5) * 60; if (!blocked(jx, jy, 'tuft')) out.push({ model: `k:tuft:${Math.floor(rnd() * 3)}`, X: jx, Y: jy, yaw: rnd() * 360, scale: 1.0 + rnd() * 0.6 }); });
  return out;
}
function nearestOutline(X, Y) { let best = null, bd = Infinity; for (const [x, y] of OUTLINE) { const d = Math.abs(Math.atan2(y, x) - Math.atan2(Y, X)); const dd = Math.min(d, 6.283 - d); if (dd < bd) { bd = dd; best = [x, y]; } } return best; }

// ================================================================ fields, roads, cobbles, trails
const FIELD_LOOK = { wheat: [PAL.wheat, PAL.wheatRow, 90], plowed: [PAL.plowed, PAL.furrow, 70], crop: [PAL.cropRow, PAL.crop, 110], pasture: [PAL.pasture, null, 0], bog: [PAL.bog, null, 0] };
function fieldParts(S, f, decals, items) {
  const [base, row, spacing] = FIELD_LOOK[f.kind] || FIELD_LOOK.pasture;
  const g = patch(f.poly, base, 0.05, 1, 100); if (g) decals.push(g);
  if (!row) return;
  const rows = Math.floor((f.width - 60) / spacing);
  for (let k = 0; k <= rows; k++) {
    const r = -f.width / 2 + 30 + k * spacing;
    const a = local(f.X, f.Y, f.yaw, -f.length / 2 + 30, r), b = local(f.X, f.Y, f.yaw, f.length / 2 - 30, r);
    decals.push(ribbon(curve3([a, b], 1.0), -0.13, 0.13, row, 0.075));
    if (f.kind === 'crop') alongLine([a, b], 55, (X, Y, yaw, i) => items.push({ model: 'k:cabbage', X, Y, yaw: i * 61, scale: 0.8 + (i % 3) * 0.15 }));
    if (f.kind === 'wheat') alongLine([a, b], 48, (X, Y, yaw, i) => items.push({ model: `k:wheat:${i % 2}`, X, Y, yaw: i * 53, scale: 0.85 + (i % 4) * 0.08 }));
  }
}
BUILDERS.cabbage = () => merged([paint(lumpy(ico(0.17, 1, 1, 0.75, 1), 0.08, 4), PAL.cabbage), paint(T(ico(0.1, 0, 1, 0.7, 1), 0, 0.08, 0), '#b7d58a')]);
function roadParts(r, decals, items) {
  const w = r.width / 100, s = curve3(r.points, 1.0), kind = r.kind;
  if (kind === 'plank') {
    decals.push(ribbon(s, -w / 2, w / 2, PAL.plank, 0.2));
    decals.push(ribbon(s, -w / 2 - 0.05, -w / 2 + 0.05, PAL.plankDark, 0.215), ribbon(s, w / 2 - 0.05, w / 2 + 0.05, PAL.plankDark, 0.215));
    alongLine(r.points, 280, (X, Y, yaw) => { for (const side of [-1, 1]) { const [x, y] = local(X, Y, yaw, 0, side * (r.width / 2 - 8)); items.push({ model: 'k:post', X: x, Y: y, yaw }); } });
    return;
  }
  const base = kind === 'path' ? PAL.path : PAL.dirt;
  decals.push(ribbon(s, -w / 2, w / 2, base, 0.05));
  if (kind !== 'path') for (const off of [-0.78, 0.78]) decals.push(ribbon(s, off - 0.15, off + 0.15, PAL.rut, 0.06));
  const edge = kind === 'path' ? PAL.dirtEdge : PAL.pencil;
  decals.push(ribbon(s, -w / 2 - 0.05, -w / 2 + 0.03, edge, 0.065), ribbon(s, w / 2 - 0.03, w / 2 + 0.05, edge, 0.065));
  if (kind === 'brick') for (const side of [-1, 1]) for (const [row, start] of [[12, 0], [32, 21]]) {
    alongLine(offsetPolyline(r.points, side * (r.width / 2 + row)), 42, (X, Y, yaw, k) => items.push({ model: 'k:brick', X, Y, yaw: yaw + 90, lift: -0.01, color: (k + (row > 20 ? 1 : 0)) % 2 ? PAL.brickA : PAL.brickB }), start);
  }
}
BUILDERS.post = () => paint(T(cyl(0.07, 0.07, 0.9, 5), 0, 0.15, 0), PAL.barkOak);
function cobbleItems(c) {
  const out = [], rnd = mulberry32(77), xs = c.poly.map((p) => p[0]), ys = c.poly.map((p) => p[1]), cols = [PAL.cobbleA, PAL.cobbleB, PAL.cobbleC];
  for (let X = Math.min(...xs); X < Math.max(...xs); X += c.spacing) for (let Y = Math.min(...ys) + ((Math.round(X / c.spacing) % 2) ? c.spacing / 2 : 0); Y < Math.max(...ys); Y += c.spacing) {
    if (!insidePoly(X, Y, c.poly) || (c.keep || []).some((k) => Math.hypot(X - k.X, Y - k.Y) < k.r)) continue;
    out.push({ model: 'k:cobble', X: X + (rnd() - 0.5) * 6, Y: Y + (rnd() - 0.5) * 6, yaw: (rnd() - 0.5) * 18, scale: [0.9 + rnd() * 0.25, 0.8 + rnd() * 0.5, 0.9 + rnd() * 0.25], lift: -0.02, color: cols[Math.floor(rnd() * 3)] });
  }
  return out;
}

// A slime's meandering trail through the given points.
function wavy(pts, amp, every, seed) {
  const out = []; let k = 0;
  alongLine(pts, every, (X, Y, yaw) => { out.push(local(X, Y, yaw, 0, Math.sin(k * 0.55 + seed) * amp + Math.sin(k * 1.31 + seed * 2) * amp * 0.4)); k++; });
  out.push(pts[pts.length - 1]);
  return out;
}

// ================================================================ building a concept's layer
const DOUBLE = /^k:(tuft|wheat|flower|reeds|cocoon)|^Reeds_B$|^LilyPads_A$/;
const NO_SHADOW = /^k:(tuft|wheat|flower|cobble|brick|cabbage|post|puddle)|^LilyPads_A$|^PebbleCluster/;
const FINE = /^k:(tuft|wheat|flower|cobble|brick|cabbage|post|reeds|mush|stepstone|sacs)|^LilyPads_A$|^PebbleCluster|^Reeds_B$/;
async function buildLayer(index, progress) {
  const S = CONCEPTS[index]();
  const group = new THREE.Group(); group.visible = false;
  S.items.push(...expandFences(S), ...expandHedges(S), ...expandFlowerBeds(S));
  for (const c of S.cobbles) S.items.push(...cobbleItems(c));
  const decals = [];
  const fieldItems = [];
  for (const f of S.fields) fieldParts(S, f, decals, fieldItems);
  const roadItems = [];
  for (const r of S.roads) roadParts(r, decals, roadItems);
  S.items.push(...fieldItems, ...roadItems);
  const blocked = makeBlocker(S);
  S.items.push(...scatterVeg(S, blocked));
  if (progress) progress(0.5);
  const byModel = new Map(), tufts = [];
  for (const it of S.items) { if (!byModel.has(it.model)) byModel.set(it.model, []); byModel.get(it.model).push(it); }
  await Promise.all([...byModel.keys()].filter((n) => !n.startsWith('k:')).map((n) => geometry(n)));
  for (const [name, list] of byModel) {
    const geom = name.startsWith('k:') ? proc(name.slice(2)) : await geometry(name);
    if (!geom) { console.warn('skipped', name, list.length); continue; }
    const mesh = instanced(group, geom, list, DOUBLE.test(name) ? MAT2 : MAT, { shadow: !NO_SHADOW.test(name) });
    if (mesh && FINE.test(name)) FINE_MESHES.push(mesh);
    if (mesh && name.startsWith('k:tuft')) tufts.push(mesh);
  }
  for (const p of S.patches) { const g = patch(p.poly, p.color, p.lift || 0.045, 1, 110); if (g) decals.push(g); }
  for (const [k, t] of S.trails.entries()) decals.push(ribbon(curve3(wavy(t.pts, 55, 130, k + 1), 0.6), -t.width / 250, t.width / 250, PAL.slimeTrail, 0.085, 1));
  if (decals.length) { const m = new THREE.Mesh(merged(decals), MAT_DECAL); m.receiveShadow = true; group.add(m); }
  if (S.water.length) instanced(group, proc('puddle'), S.water.map((w) => ({ X: w.X, Y: w.Y, yaw: 0, scale: [w.r / 100, 1, w.r / 100], lift: 0.09 })), MAT_DECAL, { shadow: false });
  const wg = webGeometry(S.webs); if (wg) { const m = new THREE.Mesh(wg, MAT2); m.receiveShadow = true; group.add(m); }
  // creatures, livestock and folk, animated
  const creatures = [], kinds = new Map();
  for (const c of S.creatures) { const key = `${c.kind}:${c.variant || 0}`; if (!kinds.has(key)) kinds.set(key, []); kinds.get(key).push(c); }
  for (const [key, list] of kinds) {
    const [kind, variant] = key.split(':'), glb = { slime: 'Slime', spider: 'Spider' }[kind];
    const geom = (glb && (await geometry(glb))) || proc(`${kind}:${variant}`);
    if (!geom) continue;
    const roams = kind === 'slime' || kind === 'spider';
    const mesh = instanced(group, geom, list.map((c) => ({ ...c, sink: 0.01 })), kind === 'slime' ? MAT2 : MAT, { shadow: !roams });
    creatures.push({ kind, mesh, list: list.map((c, i) => ({ ...c, phase: i * 1.7 + (c.X % 7), base: new THREE.Vector3(toX(c.Y), (groundUE(c.X, c.Y) || 0) + (c.lift || 0), toZ(c.X)) })) });
  }
  // chimney smoke, animated
  const emitters = S.smoke.map((s) => {
    const p = new THREE.Vector3(...s.socket.pos).multiplyScalar(s.scale || 1).applyAxisAngle(new THREE.Vector3(0, 1, 0), rotY(s.yaw));
    const g = groundUE(s.X, s.Y) || 0;
    return new THREE.Vector3(toX(s.Y), g - 0.14, toZ(s.X)).add(p);
  });
  let smoke = null;
  if (emitters.length) { smoke = new THREE.InstancedMesh(proc('smoke'), MAT, emitters.length * PUFFS); smoke.frustumCulled = false; group.add(smoke); }
  scene.add(group);
  return { index, S, group, creatures, smoke, emitters, tufts };
}
const PUFFS = 8;

// ================================================================ the shared base: terrain, cliffs, water, waterfall, sky islands, fan, birds
let fan = null, birds = null;
async function buildBase(progress) {
  const [terrain, under, water] = await Promise.all([geometry('Terrain'), geometry('Underside'), geometry('Water')]);
  if (terrain) { const m = new THREE.Mesh(terrain, MAT); m.receiveShadow = true; base.add(m); }
  if (under) base.add(new THREE.Mesh(under, MAT));
  if (water) {
    const col = water.attributes.color, wc = C(PAL.water);
    for (let i = 0; i < col.count; i++) col.setXYZW(i, wc.r, wc.g, wc.b, 0.5);
    col.needsUpdate = true;
    const m = new THREE.Mesh(water, MAT); m.position.y = 0.02; m.receiveShadow = true; base.add(m);
  }
  progress(0.3);
  // cliff faces as build_area.py places them, and the knobs' outcrops
  const pieces = [];
  for (const name of ['CliffFace_A', 'CliffFace_B', 'CliffFace_C', 'CliffFace_D']) { const b = modelBounds(name); if (b) pieces.push({ name, height: b.max[1], width: b.max[0] - b.min[0] }); }
  const rnd = mulberry32(23), byPiece = new Map(pieces.map((p) => [p.name, []])), outcrops = new Map();
  for (const [, points] of Object.entries(COMPUTED.cliffs || {})) points.forEach((point, i) => {
    if (point.kind === 'bank') return;
    if (point.kind === 'outcrop') { const name = 'Outcrop_' + (point.piece || 'TorA'), b = modelBounds(name); if (!b) return; if (!outcrops.has(name)) outcrops.set(name, []); outcrops.get(name).push({ X: point.location[0], Y: point.location[1], Z: point.location[2], yaw: point.yaw || 0, scale: point.height / 100 / Math.max(b.max[1], 0.01) }); return; }
    if (!pieces.length) return;
    const [x, y, z] = point.location;
    let bottom, height;
    if ('drop' in point) { bottom = z - point.drop; height = point.drop; } else { bottom = z; height = point.height ?? ((point.top ?? z) - z); }
    const courses = point.courses || [{ location: [x, y, bottom], height }];
    const gaps = [i - 1, i + 1].filter((j) => j >= 0 && j < points.length).map((j) => Math.hypot(point.location[0] - points[j].location[0], point.location[1] - points[j].location[1]));
    const gap = gaps.length ? Math.min(...gaps) : 1000, yaw = point.yaw;
    courses.forEach((course, k) => {
      const [cx, cy, cz] = course.location, reach = (course.height + (k === courses.length - 1 ? 20 : 30)) / 100;
      const choice = pieces.map((p) => ({ p, score: Math.abs(Math.log(reach / p.height)) + rnd() * 0.25 })).sort((a, b) => a.score - b.score)[0].p;
      const width = clamp((gap + 250) / (choice.width * 100), 0.75, 1.6);
      byPiece.get(choice.name).push({ X: cx - Math.cos(yaw * DEG) * 120, Y: cy - Math.sin(yaw * DEG) * 120, Z: cz, yaw: yaw + (rnd() - 0.5) * 8, scale: [width, reach / choice.height, 1] });
    });
  });
  for (const [name, list] of byPiece) if (list.length) instanced(base, await geometry(name), list);
  for (const [name, list] of outcrops) instanced(base, await geometry(name), list);
  progress(0.45);
  // the creek's falls off the rim
  const wf = COMPUTED.waterfall || (DATA.layoutComputed && DATA.layoutComputed.waterfall);
  if (wf) base.add(waterfallMesh(wf));
  // the sky islands, far off in the haze
  const far = [['SkyIsland_A', -34000, 22000, -3500, 20], ['SkyIsland_B', 30000, 32000, -1800, 110], ['SkyIsland_C', 38000, -20000, 2000, 200], ['SkyIsland_D', -22000, -38000, -4500, 300]];
  for (const [name, X, Y, Z, yaw] of far) { const g = await geometry(name); if (g) instanced(base, g, [{ X, Y, Z, yaw }], MAT, { shadow: false }); }
  // the windmill's fan on its socket, turning
  const fanGeo = await geometry('WindmillFan'), [sock] = socketOf('Windmill', 'Fan');
  if (fanGeo && sock) {
    fan = new THREE.Mesh(fanGeo, MAT);
    const g = groundUE(6200, -3800) || 0, yaw = rotY(30);
    fan.position.set(toX(-3800), g - 0.15, toZ(6200)).add(new THREE.Vector3(...sock.pos).applyAxisAngle(new THREE.Vector3(0, 1, 0), yaw));
    fan.rotation.set(0, yaw, 0, 'YXZ'); base.add(fan);   // no shadow: the shadow map is drawn once
  }
  birds = new THREE.InstancedMesh(proc('bird'), MAT2, 9); birds.frustumCulled = false; fx.add(birds);
}
function waterfallMesh(wf) {
  const [X, Y, Z] = wf.location, dir = dir3(wf.yaw, 0), side = new THREE.Vector3(dir.z, 0, -dir.x);
  const lip = new THREE.Vector3(toX(Y), Z / 100, toZ(X)), pos = [], cols = [], flags = [];
  const N = 26, H = 46;
  const P = (s, off) => { const out = 0.3 + 3.2 * Math.sqrt(s); return lip.clone().addScaledVector(dir, out).addScaledVector(side, off * (1.2 + s * 1.6)).add(new THREE.Vector3(0, -H * s * s * 0.6 - H * s * 0.4, 0)); };
  for (const [a, b, c] of [[-1, 1, PAL.water], [-0.55, -0.35, '#dcecec'], [0.2, 0.42, '#dcecec'], [-0.1, 0.05, '#c2dfe0']]) for (let i = 0; i < N; i++) {
    const s0 = i / N, s1 = (i + 1) / N, p0 = P(s0, a), p1 = P(s0, b), p2 = P(s1, b), p3 = P(s1, a);
    pos.push(p0.x, p0.y, p0.z, p1.x, p1.y, p1.z, p2.x, p2.y, p2.z, p0.x, p0.y, p0.z, p2.x, p2.y, p2.z, p3.x, p3.y, p3.z);
    cols.push(c, c); flags.push(0, 0);
  }
  const m = new THREE.Mesh(soup(pos, cols, flags), MAT2);
  return m;
}

// ================================================================ animation
const tmpV = new THREE.Vector3(), tmpV2 = new THREE.Vector3(), UP = new THREE.Vector3(0, 1, 0);
function animate(layer, t) {
  if (!layer) return;
  for (const c of layer.creatures) {
    c.list.forEach((e, i) => {
      if (c.kind === 'slime') {
        const w = 0.35, a = t * w + e.phase, r = 0.7;
        const hop = Math.max(0, Math.sin(t * 3.1 + e.phase));
        tmpP.set(e.base.x + Math.cos(a) * r, e.base.y + hop * 0.18, e.base.z + Math.sin(a) * r);
        const sq = 1 + Math.sin(t * 6.2 + e.phase) * 0.08;
        tmpS.set(1 / Math.sqrt(sq), sq, 1 / Math.sqrt(sq));
        tmpQ.setFromAxisAngle(UP, -a);
      } else if (c.kind === 'spider') {
        const a = t * 0.22 + e.phase, r = 1.3 + Math.sin(e.phase) * 0.5, jitter = Math.sin(t * 9 + e.phase) * 0.04;
        tmpP.set(e.base.x + Math.cos(a) * r, e.base.y, e.base.z + Math.sin(a) * r);
        tmpS.set(1, 1, 1); tmpQ.setFromAxisAngle(UP, -a + jitter);
      } else if (c.kind === 'chicken') {
        // scratching about and pecking
        tmpP.set(e.base.x + Math.sin(t * 0.45 + e.phase) * 0.35, e.base.y, e.base.z + Math.cos(t * 0.31 + e.phase * 1.3) * 0.35);
        const peck = Math.pow(Math.max(0, Math.sin(t * 3.2 + e.phase * 2)), 3);
        tmpQ.setFromEuler(tmpE.set(peck * 0.6, rotY(e.yaw) + Math.sin(t * 0.8 + e.phase) * 1.2, 0, 'YXZ')); tmpS.set(1, 1, 1);
      } else {
        // sheep, cows and townsfolk turn slowly where they stand
        tmpP.copy(e.base); tmpS.setScalar(e.scale || 1);
        tmpQ.setFromAxisAngle(UP, rotY(e.yaw) + Math.sin(t * 0.12 + e.phase) * (c.kind === 'villager' ? 0.25 : 0.5));
      }
      const gy = groundAt(tmpP.x, tmpP.z); if (!Number.isNaN(gy)) tmpP.y += gy + (e.lift || 0) - e.base.y;
      c.mesh.setMatrixAt(i, tmpM.compose(tmpP, tmpQ, tmpS));
    });
    c.mesh.instanceMatrix.needsUpdate = true;
  }
  // Smoke: each puff rises fast then slows, leans downwind and swells, then thins away; puffs are staggered unevenly so
  // a plume never reads as a string of beads.
  if (layer.smoke) {
    let k = 0;
    for (const [e, p] of layer.emitters.entries()) for (let j = 0; j < PUFFS; j++, k++) {
      const jit = Math.sin(j * 12.9898 + e * 78.233) * 0.5 + 0.5, life = ((t * 0.12 + (j + jit * 0.6) / PUFFS + e * 0.37) % 1);
      const rise = Math.sqrt(life) * 6.5, lean = Math.pow(life, 1.4) * 3.4;
      tmpP.copy(p).add(tmpV.set(lean + Math.sin(t * 0.7 + j * 2.1) * 0.25 * life, rise, -lean * 0.45 + Math.cos(t * 0.6 + j) * 0.2 * life));
      const s = (0.5 + life * 1.7 + jit * 0.3) * (life > 0.78 ? Math.max(0, 1 - (life - 0.78) / 0.22) : 1);
      tmpS.set(s * (1 + jit * 0.3), s * 0.8, s); tmpQ.setFromAxisAngle(UP, j * 1.3 + e);
      layer.smoke.setMatrixAt(k, tmpM.compose(tmpP, tmpQ, tmpS));
    }
    layer.smoke.instanceMatrix.needsUpdate = true;
  }
  // Grass tufts only show from low down: from high above they are noise, and costly.
  const low = camera.position.y - (groundAt(camera.position.x, camera.position.z) || 0) < 55;
  for (const m of layer.tufts) m.visible = low;
}
function animateBase(t, dt) {
  if (fan) fan.rotateZ(dt * 0.9);
  if (birds) {
    for (let i = 0; i < 9; i++) {
      const r = 45 + (i % 4) * 14, a = t * (0.09 + (i % 3) * 0.015) + i * 0.7, h = 28 + (i % 5) * 4 + Math.sin(t * 0.5 + i) * 2;
      tmpP.set(Math.cos(a) * r - 5, h, Math.sin(a) * r + 6);
      tmpQ.setFromAxisAngle(UP, -a);
      const flap = Math.sin(t * 7 + i * 1.3);
      tmpS.set(1.6, 1.6 * (flap > 0 ? 1 : 0.3 + 0.7 * (1 + flap)), 1.6);
      birds.setMatrixAt(i, tmpM.compose(tmpP, tmpQ, tmpS));
    }
    birds.instanceMatrix.needsUpdate = true;
  }
}
