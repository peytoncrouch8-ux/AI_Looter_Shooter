
// ================================================================ the procedural kit (flat-colored, flat-shaded)
// Long things (fences, hedges, barricades) run along their local +x like the game's FenceRail, so a piece placed at a
// line's start takes yaw = line yaw + 90.
const BUILDERS = {
  oak(seed) {
    const r = mulberry32(1000 + seed * 77), parts = [];
    const h = 2.3 + r() * 0.9, tr = 0.24 + r() * 0.08;
    parts.push(paint(T(cyl(tr, tr * 0.72, h + 0.8, 7), 0, (h + 0.8) / 2, 0), PAL.barkOak));
    for (let b = 0; b < 3; b++) {
      const a = b * 2.1 + r() * 0.8, L = 1.3 + r() * 0.5, g = cyl(tr * 0.42, tr * 0.22, L, 5);
      g.translate(0, L / 2, 0); g.rotateZ(0.95 + r() * 0.3); g.rotateY(a); g.translate(0, h * 0.72 + r() * 0.3, 0);
      parts.push(paint(g, PAL.barkOak));
    }
    const n = 7 + Math.floor(r() * 3);
    for (let i = 0; i < n; i++) {
      const a = (i / n) * Math.PI * 2 + r() * 0.5, rad = 1.1 + r() * 0.9, y = h + 0.7 + r() * 1.4, s = 1.05 + r() * 0.65;
      const col = y > h + 1.6 ? PAL.oakLeafUp : (r() < 0.35 ? PAL.oakLeafDark : PAL.oakLeaf);
      parts.push(paint(T(lumpy(ico(s, 1, 1, 0.8, 1), 0.07, seed * 13 + i), Math.cos(a) * rad, y, Math.sin(a) * rad), col));
    }
    parts.push(paint(T(lumpy(ico(1.55 + r() * 0.35, 1, 1, 0.82, 1), 0.07, seed * 7 + 99), 0, h + 2.4 + r() * 0.4, 0), PAL.oakLeafUp));
    return merged(parts);
  },
  birch(seed) {
    const r = mulberry32(2000 + seed * 31), parts = [];
    const h = 4.0 + r() * 1.2;
    parts.push(paint(T(cyl(0.15, 0.09, h, 7), 0, h / 2, 0), PAL.barkBirch));
    for (let k = 0; k < 5; k++) { const y = 0.5 + r() * (h - 1.2), rr = 0.155 - (y / h) * 0.06; parts.push(paint(T(cyl(rr, rr - 0.004, 0.07, 7), 0, y, 0), PAL.birchMark)); }
    const n = 4 + Math.floor(r() * 2);
    for (let i = 0; i < n; i++) {
      const a = i / n * 6.28 + r(), rad = 0.45 + r() * 0.5, y = h * 0.62 + r() * h * 0.38, s = 0.8 + r() * 0.45;
      parts.push(paint(T(lumpy(ico(s, 1, 1, 1.25, 1), 0.08, seed * 17 + i), Math.cos(a) * rad, y, Math.sin(a) * rad), y > h * 0.85 ? PAL.birchLeafUp : PAL.birchLeaf));
    }
    return merged(parts);
  },
  pine(seed) {
    const r = mulberry32(3000 + seed * 19), parts = [];
    const h = 6.5 + r() * 2.5, layers = 4 + (seed % 2);
    parts.push(paint(T(cyl(0.22, 0.12, h * 0.55, 6), 0, h * 0.275, 0), PAL.barkPine));
    for (let k = 0; k < layers; k++) {
      const t = k / layers, rad = (1 - t) * 2.2 + 0.5, ch = h * 0.36 * (1 - t * 0.35), y = h * 0.18 + t * h * 0.72 + ch / 2;
      const g = jagCone(rad, ch, 12, 0.72); g.rotateY(r() * 6.28); g.translate(0, y, 0);
      parts.push(paint(g, k >= layers - 2 ? PAL.pineUp : PAL.pine));
    }
    return merged(parts);
  },
  deadpine(seed) {
    const r = mulberry32(4000 + seed), parts = [];
    const h = 5.2 + r() * 1.5;
    parts.push(paint(T(cyl(0.2, 0.06, h, 6), 0, h / 2, 0), PAL.barkDead));
    for (let k = 0; k < 9; k++) {
      const y = 1.2 + r() * (h - 1.6), L = (1 - y / h) * 1.6 + 0.3, g = cyl(0.05, 0.015, L, 4);
      g.translate(0, L / 2, 0); g.rotateZ(1.2 + r() * 0.5); g.rotateY(r() * 6.28); g.translate(0, y, 0);
      parts.push(paint(g, PAL.barkDead));
    }
    return merged(parts);
  },
  apple(seed) {
    const r = mulberry32(5000 + seed), parts = [];
    const h = 1.4 + r() * 0.3;
    parts.push(paint(T(cyl(0.16, 0.11, h + 0.4, 6), 0, (h + 0.4) / 2, 0), PAL.barkOak));
    for (let i = 0; i < 5; i++) {
      const a = i / 5 * 6.28 + r(), rad = 0.7 + r() * 0.4, y = h + 0.5 + r() * 0.6, s = 0.8 + r() * 0.3;
      parts.push(paint(T(lumpy(ico(s, 1, 1, 0.8, 1), 0.07, i + seed), Math.cos(a) * rad, y, Math.sin(a) * rad), PAL.apple));
    }
    parts.push(paint(T(ico(1.0, 1, 1, 0.8, 1), 0, h + 1.2, 0), PAL.apple));
    for (let i = 0; i < 10; i++) { const a = r() * 6.28, rad = 1.0 + r() * 0.5, y = h + 0.3 + r() * 1.2; parts.push(paint(T(ico(0.12, 0), Math.cos(a) * rad, y, Math.sin(a) * rad), PAL.appleFruit, 0)); }
    return merged(parts);
  },
  bush(seed) {
    const r = mulberry32(6000 + seed * 11), parts = [];
    const n = 3 + Math.floor(r() * 3);
    for (let i = 0; i < n; i++) {
      const a = i / n * 6.28 + r(), rad = 0.35 + r() * 0.35, s = 0.55 + r() * 0.35;
      parts.push(paint(T(lumpy(ico(s, 1, 1.1, 0.8, 1), 0.08, seed + i), Math.cos(a) * rad, s * 0.6, Math.sin(a) * rad), r() < 0.4 ? PAL.bushDark : PAL.bush));
    }
    if (seed === 2) for (let i = 0; i < 10; i++) { const a = r() * 6.28, rr = 0.5 + r() * 0.4; parts.push(paint(T(ico(0.07, 0), Math.cos(a) * rr, 0.5 + r() * 0.5, Math.sin(a) * rr), r() < 0.5 ? PAL.flowerWhite : PAL.flowerPink, 0)); }
    return merged(parts);
  },
  hedge(seed) {
    const r = mulberry32(7000 + seed), parts = [];
    for (let i = 0; i < 3; i++) {
      const x = -0.8 + i * 0.8 + (r() - 0.5) * 0.2, s = 0.72 + r() * 0.15;
      parts.push(paint(T(lumpy(ico(s, 1, 1.15, 0.95, 0.9), 0.06, seed + i), x, 0.62, (r() - 0.5) * 0.15), r() < 0.5 ? PAL.hedge : PAL.bushDark));
    }
    return merged(parts);
  },
  tuft(seed) {
    const r = mulberry32(8000 + seed), pos = [], cols = [];
    for (let i = 0; i < 7; i++) {
      const a = r() * 6.28, lean = 0.25 + r() * 0.35, h = 0.22 + r() * 0.3, w = 0.035 + r() * 0.02;
      const bx = Math.cos(a) * 0.05, bz = Math.sin(a) * 0.05, px = -Math.sin(a) * w, pz = Math.cos(a) * w;
      pos.push(bx - px, 0, bz - pz, bx + px, 0, bz + pz, bx + Math.cos(a) * lean * h, h, bz + Math.sin(a) * lean * h);
      cols.push(i % 2 ? PAL.tuftA : PAL.tuftB);
    }
    return soup(pos, cols);
  },
  wheat(seed) {
    const r = mulberry32(8500 + seed), pos = [], cols = [];
    for (let i = 0; i < 9; i++) {
      const a = r() * 6.28, h = 0.75 + r() * 0.25, w = 0.03, bx = Math.cos(a) * 0.12, bz = Math.sin(a) * 0.12, px = -Math.sin(a) * w, pz = Math.cos(a) * w;
      const tx = bx + Math.cos(a) * 0.12, tz = bz + Math.sin(a) * 0.12;
      pos.push(bx - px, 0, bz - pz, bx + px, 0, bz + pz, tx, h, tz);
      pos.push(tx - px * 2, h - 0.18, tz - pz * 2, tx + px * 2, h - 0.18, tz + pz * 2, tx, h + 0.06, tz);
      cols.push(PAL.wheatRow, PAL.wheatTuft);
    }
    return soup(pos, cols);
  },
  flower(_, color) {
    const r = mulberry32(9000 + color.length * 7), hex = { pink: PAL.flowerPink, yellow: PAL.flowerYellow, white: PAL.flowerWhite, blue: PAL.flowerBlue, red: PAL.flowerRed }[color] || PAL.flowerPink;
    const pos = [], cols = [], heads = [];
    for (let i = 0; i < 6; i++) {
      const a = r() * 6.28, d = 0.05 + r() * 0.16, h = 0.22 + r() * 0.2, bx = Math.cos(a) * d, bz = Math.sin(a) * d;
      pos.push(bx - 0.012, 0, bz, bx + 0.012, 0, bz, bx, h, bz); cols.push(PAL.stem);
      heads.push(paint(T(new THREE.OctahedronGeometry(0.055 + r() * 0.02, 0), bx, h + 0.02, bz), hex, 0));
    }
    return merged([soup(pos, cols), ...heads]);
  },
  reeds(seed) {
    const r = mulberry32(9500 + seed), pos = [], cols = [], parts = [];
    for (let i = 0; i < 9; i++) {
      const a = r() * 6.28, h = 0.9 + r() * 0.7, w = 0.03, bx = Math.cos(a) * 0.12, bz = Math.sin(a) * 0.12, px = -Math.sin(a) * w, pz = Math.cos(a) * w;
      pos.push(bx - px, 0, bz - pz, bx + px, 0, bz + pz, bx + Math.cos(a) * 0.18, h, bz + Math.sin(a) * 0.18); cols.push(PAL.reed);
    }
    parts.push(soup(pos, cols));
    for (let i = 0; i < 3; i++) { const a = r() * 6.28, x = Math.cos(a) * 0.1, z = Math.sin(a) * 0.1, h = 1.2 + r() * 0.3; parts.push(paint(T(cyl(0.008, 0.008, h, 3), x, h / 2, z), PAL.stem), paint(T(cyl(0.04, 0.04, 0.2, 6), x, h + 0.05, z), PAL.cattail)); }
    return merged(parts);
  },
  mush(seed) {
    const r = mulberry32(9700 + seed), parts = [];
    for (let i = 0; i < 4; i++) {
      const x = (r() - 0.5) * 0.4, z = (r() - 0.5) * 0.4, h = 0.08 + r() * 0.1, s = 0.06 + r() * 0.06;
      parts.push(paint(T(cyl(s * 0.4, s * 0.35, h, 6), x, h / 2, z), PAL.mushStem));
      const cap = new THREE.SphereGeometry(s, 8, 4, 0, Math.PI * 2, 0, Math.PI / 2); cap.scale(1, 0.7, 1); cap.translate(x, h, z);
      parts.push(paint(cap, i % 3 ? PAL.mushCap : PAL.mushStem, 0));
    }
    return merged(parts);
  },
  cocoon() { return merged([paint(lumpy(ico(0.2, 1, 1, 2.1, 1), 0.08, 3), PAL.cocoon, 0), paint(T(cyl(0.012, 0.012, 1.1, 3), 0, 0.95, 0), PAL.web, 0)]); },
  sacs(seed) {
    const r = mulberry32(9800 + seed), parts = [];
    for (let i = 0; i < 5; i++) { const s = 0.16 + r() * 0.14, a = r() * 6.28, d = r() * 0.35; parts.push(paint(T(lumpy(ico(s, 1, 1, 1.15, 1), 0.06, i), Math.cos(a) * d, s * 0.9, Math.sin(a) * d), PAL.sac, 0)); }
    return merged(parts);
  },
  burrow() {
    const parts = [];
    const hole = new THREE.CircleGeometry(1.0, 18); hole.rotateX(-Math.PI / 2); hole.scale(1.25, 1, 0.85); hole.translate(0, 0.07, 0);
    parts.push(paint(hole, '#2f2629', 1));
    const r = mulberry32(31);
    for (let i = 0; i < 9; i++) { const a = i / 9 * 6.28, s = 0.25 + r() * 0.2; parts.push(paint(T(lumpy(ico(s, 0, 1.3, 0.6, 1), 0.1, i), Math.cos(a) * 1.35, s * 0.3, Math.sin(a) * 1.05), r() < 0.5 ? PAL.stone : PAL.stoneDark)); }
    return merged(parts);
  },
  picket() {
    const parts = [];
    for (const x of [0.05, 1.95]) parts.push(paint(T(box(0.09, 1.05, 0.09), x, 0.52, 0), PAL.picket));
    for (const y of [0.32, 0.78]) parts.push(paint(T(box(2.0, 0.06, 0.03), 1.0, y, -0.05), PAL.picket));
    for (let i = 0; i < 9; i++) {
      const x = 0.2 + i * 0.2;
      parts.push(paint(T(box(0.075, 0.82, 0.022), x, 0.48, -0.075), PAL.picket));
      const tip = new THREE.ConeGeometry(0.055, 0.12, 4); tip.rotateY(Math.PI / 4); tip.scale(1, 1, 0.4); tip.translate(x, 0.95, -0.075);
      parts.push(paint(tip, PAL.picket));
    }
    return merged(parts);
  },
  barricade() {
    const parts = [];
    for (const x of [-1.15, 1.15]) for (const s of [-1, 1]) { const g = box(0.08, 1.35, 0.1); g.rotateX(s * 0.42); g.translate(x, 0.6, 0); parts.push(paint(g, PAL.plankDark)); }
    for (let i = 0; i < 2; i++) for (let k = 0; k < 5; k++) parts.push(paint(T(box(0.52, 0.22, 0.05), -1.04 + k * 0.52, 0.55 + i * 0.44, 0.09), (k + i) % 2 ? PAL.awningCream : PAL.awningRed));
    return merged(parts);
  },
  sandbags(seed) {
    const r = mulberry32(9900 + seed), parts = [];
    for (let row = 0; row < 3; row++) for (let k = 0; k < 5 - row; k++) {
      const x = -1.0 + k * 0.5 + row * 0.25 + (r() - 0.5) * 0.05;
      parts.push(paint(T(lumpy(ico(0.26, 1, 1.0, 0.42, 0.62), 0.06, row * 7 + k), x, 0.11 + row * 0.2, (r() - 0.5) * 0.06), r() < 0.5 ? '#c8b48c' : '#b9a57e'));
    }
    return merged(parts);
  },
  scarecrow() {
    return merged([paint(T(cyl(0.05, 0.05, 2.0, 5), 0, 1.0, 0), PAL.barkOak), paint(T(box(1.5, 0.07, 0.07), 0, 1.45, 0), PAL.barkOak),
      paint(T(box(0.55, 0.62, 0.25), 0, 1.22, 0), PAL.shirt), paint(T(box(1.25, 0.18, 0.2), 0, 1.45, 0), PAL.shirt),
      paint(T(ico(0.2, 1), 0, 1.84, 0), PAL.hay), paint(T(cyl(0.32, 0.32, 0.03, 10), 0, 2.0, 0), PAL.hat), paint(T(cyl(0.17, 0.14, 0.22, 8), 0, 2.12, 0), PAL.hat)]);
  },
  stall() {
    const parts = [];
    for (const x of [-1.1, 1.1]) for (const z of [-0.55, 0.55]) parts.push(paint(T(cyl(0.05, 0.05, 2.3, 5), x, 1.15, z), PAL.plankDark));
    parts.push(paint(T(box(2.2, 0.9, 0.7), 0, 0.45, 0.3), PAL.plank));
    for (let k = 0; k < 6; k++) { const g = box(0.38, 0.04, 1.6); g.rotateX(0.3); g.translate(-0.95 + k * 0.38, 2.35, 0.05); parts.push(paint(g, k % 2 ? PAL.awningCream : PAL.awningRed)); }
    const goods = [PAL.appleFruit, PAL.cabbage, PAL.wheat, PAL.flowerYellow, PAL.cabbage, PAL.appleFruit];
    goods.forEach((c, k) => parts.push(paint(T(ico(0.16, 0, 1, 0.7, 1), -0.8 + k * 0.32, 0.98, 0.35), c, 0)));
    return merged(parts);
  },
  shed() {
    return merged([paint(T(box(2.6, 2.0, 2.0), 0, 1.0, 0), PAL.plank), paint(T(box(0.8, 1.6, 0.05), 0.4, 0.8, 1.01), PAL.plankDark),
      (() => { const g = box(3.0, 0.08, 2.5); g.rotateX(-0.22); g.translate(0, 2.2, 0); return paint(g, '#8a7f74'); })()]);
  },
  coop() {
    const parts = [paint(T(box(1.8, 1.1, 1.3), 0, 0.85, 0), '#b2604f')];
    for (const s of [-1, 1]) { const g = box(1.05, 0.06, 2.0); g.rotateZ(s * 0.62); g.translate(s * 0.43, 1.66, 0); g.rotateY(Math.PI / 2); parts.push(paint(g, '#7c6f63')); }
    const tri = soup([-0.9, 1.4, 0.66, 0.9, 1.4, 0.66, 0, 2.0, 0.66, 0.9, 1.4, -0.66, -0.9, 1.4, -0.66, 0, 2.0, -0.66], ['#b2604f']);
    parts.push(tri);
    for (const x of [-0.75, 0.75]) for (const z of [-0.55, 0.55]) parts.push(paint(T(box(0.08, 0.3, 0.08), x, 0.15, z), PAL.plankDark));
    const ramp = box(0.35, 0.03, 0.9); ramp.rotateX(0.55); ramp.translate(0.3, 0.2, 0.95); parts.push(paint(ramp, PAL.plank));
    return merged(parts);
  },
  haystack() { return merged([paint(lumpy(T(cyl(1.35, 0.25, 2.1, 10), 0, 1.05, 0), 0.05, 5), PAL.hay), paint(T(ico(0.32, 0, 1, 0.7, 1), 0, 2.12, 0), PAL.hay)]); },
  smoke() { return merged([paint(lumpy(ico(0.55, 1), 0.14, 3), PAL.smoke, 0), paint(T(lumpy(ico(0.36, 1), 0.12, 5), 0.42, -0.08, 0.1), PAL.smoke, 0), paint(T(lumpy(ico(0.3, 1), 0.12, 7), -0.36, -0.12, -0.12), PAL.smoke, 0)]); },
  bird() { return soup([0, 0, 0, -0.5, 0.12, -0.18, -0.06, 0, 0.12, 0, 0, 0, 0.06, 0, 0.12, 0.5, 0.12, -0.18], [PAL.bird], [0]); },
  brick() { return paint(box(0.4, 0.1, 0.19), '#ffffff'); },
  cobble() { return paint(box(0.33, 0.08, 0.33), '#ffffff'); },
  stepstone(seed) { return paint(lumpy(ico(0.36, 0, 1, 0.22, 0.85), 0.12, seed), PAL.stone); },
  log() { const g = cyl(0.34, 0.3, 5.0, 8); g.rotateZ(Math.PI / 2); g.translate(0, 0.32, 0); return merged([paint(g, PAL.barkOak)]); },
  stump() { return merged([paint(T(cyl(0.38, 0.32, 0.55, 8), 0, 0.27, 0), PAL.barkOak), paint(T(cyl(0.31, 0.31, 0.02, 8), 0, 0.555, 0), '#cdb08a')]); },
  dummy() {
    return merged([paint(T(cyl(0.08, 0.08, 1.7, 5), 0, 0.85, 0), PAL.barkOak), (() => { const g = cyl(0.05, 0.05, 1.1, 5); g.rotateZ(Math.PI / 2); g.translate(0, 1.45, 0); return paint(g, PAL.barkOak); })(),
      paint(T(cyl(0.3, 0.26, 0.9, 8), 0, 1.2, 0), PAL.target), paint(T(ico(0.24, 1, 1, 1.1, 1), 0, 1.95, 0), PAL.hay),
      paint(T(cyl(0.18, 0.18, 0.02, 12).rotateX(Math.PI / 2), 0, 1.25, 0.3), PAL.awningRed, 0)]);
  },
  target() {
    const parts = [paint(T(box(0.08, 1.6, 0.08), -0.5, 0.8, 0), PAL.plankDark), paint(T(box(0.08, 1.6, 0.08), 0.5, 0.8, 0), PAL.plankDark), paint(T(box(1.1, 1.1, 0.06), 0, 1.15, 0.05), PAL.target)];
    for (const [rr, c] of [[0.45, PAL.awningRed], [0.31, PAL.target], [0.17, PAL.awningRed]]) { const g = new THREE.CircleGeometry(rr, 20); g.translate(0, 1.15, 0.085 + (0.45 - rr) * 0.02); parts.push(paint(g, c, 0)); }
    return merged(parts);
  },
  dock() {
    const parts = [paint(T(box(1.5, 0.1, 5.0), 0, 0.45, 2.5), PAL.plank)];
    for (let k = 0; k < 5; k++) parts.push(paint(T(box(1.5, 0.02, 0.04), 0, 0.51, 0.5 + k), PAL.plankDark));
    for (const x of [-0.7, 0.7]) for (const z of [0.3, 2.5, 4.7]) parts.push(paint(T(cyl(0.08, 0.08, 1.3, 6), x, 0.0, z), PAL.barkOak));
    return merged(parts);
  },
  sluice() {
    return merged([paint(T(box(0.18, 1.6, 0.18), -0.9, 0.6, 0), PAL.plankDark), paint(T(box(0.18, 1.6, 0.18), 0.9, 0.6, 0), PAL.plankDark),
      paint(T(box(1.9, 0.14, 0.18), 0, 1.35, 0), PAL.plankDark), (() => { const g = box(1.6, 0.9, 0.06); g.rotateZ(0.18); g.translate(0.05, 0.45, 0.1); return paint(g, PAL.plank); })()]);
  },
  puddle() { const g = new THREE.CircleGeometry(1, 16); g.rotateX(-Math.PI / 2); return paint(g, PAL.puddle); },
  trail() { const g = new THREE.PlaneGeometry(1, 1); g.rotateX(-Math.PI / 2); return paint(g, PAL.slimeTrail, 0); },
  jetty() {
    const parts = [paint(T(box(3.0, 0.16, 22), 0, 0.42, 11), PAL.plank)];
    for (const zz of [1, 3.5, 6]) for (const xx of [-1.3, 1.3]) parts.push(paint(T(cyl(0.12, 0.12, 0.5, 6), xx, 0.17, zz), PAL.barkOak));
    for (const zz of [8, 13, 18]) for (const xx of [-1.3, 1.3]) { const g = cyl(0.1, 0.1, 5.5, 6); g.rotateX(0.55); g.rotateZ(xx > 0 ? -0.35 : 0.35); g.translate(xx * 1.6, -2.2, zz - 1.4); parts.push(paint(g, PAL.barkOak)); }
    for (const xx of [-1.45, 1.45]) { parts.push(paint(T(box(0.08, 0.08, 21), xx, 1.45, 11.5), PAL.barkOak)); for (let zz = 1; zz < 22; zz += 3) parts.push(paint(T(cyl(0.06, 0.06, 1.0, 5), xx, 1.0, zz), PAL.barkOak)); }
    parts.push(paint(T(cyl(0.09, 0.09, 2.4, 6), 1.2, 1.7, 21.5), PAL.barkOak), paint(T(ico(0.22, 1, 1, 1.2, 1), 1.2, 2.95, 21.5), PAL.lantern, 0));
    return merged(parts);
  },
  // Livestock and townsfolk: simple flat-colored shapes, facing +z like every model.
  sheep(seed) {
    const parts = [], grazing = seed % 2 === 1;
    parts.push(paint(T(lumpy(ico(0.4, 1, 1.0, 0.8, 1.32), 0.1, 3 + seed), 0, 0.72, 0), PAL.wool));
    const head = ico(0.16, 1, 0.85, 1.0, 1.3);
    if (grazing) { head.rotateX(0.9); parts.push(paint(T(head, 0, 0.42, 0.62), PAL.sheepFace)); }
    else parts.push(paint(T(head, 0, 0.86, 0.6), PAL.sheepFace));
    for (const sx of [-1, 1]) { const e = box(0.16, 0.04, 0.07); e.rotateZ(sx * 0.4); parts.push(paint(T(e, sx * 0.14, grazing ? 0.52 : 0.93, 0.55), PAL.sheepFace)); }
    for (const [x, z] of [[-0.17, 0.3], [0.17, 0.3], [-0.17, -0.3], [0.17, -0.3]]) parts.push(paint(T(cyl(0.05, 0.045, 0.48, 5), x, 0.24, z), PAL.sheepFace));
    return merged(parts);
  },
  cow(seed) {
    const col = seed % 2 ? PAL.cowA : PAL.cowB, spot = seed % 2 ? PAL.cowB : PAL.cowA, parts = [];
    parts.push(paint(T(box(0.62, 0.62, 1.55), 0, 1.0, 0), col), paint(T(box(0.64, 0.4, 0.5), 0, 1.02, 0.22), spot), paint(T(box(0.64, 0.34, 0.36), 0, 0.92, -0.5), spot));
    parts.push(paint(T(box(0.34, 0.36, 0.45), 0, 1.18, 0.96), col), paint(T(box(0.3, 0.2, 0.14), 0, 1.06, 1.2), '#e3b8a6'));
    for (const sx of [-1, 1]) { const h = new THREE.ConeGeometry(0.035, 0.16, 5); h.rotateZ(-sx * 0.7); parts.push(paint(T(h, sx * 0.2, 1.4, 0.9), '#efe6d3')); }
    for (const [x, z] of [[-0.21, 0.58], [0.21, 0.58], [-0.21, -0.58], [0.21, -0.58]]) parts.push(paint(T(box(0.14, 0.7, 0.14), x, 0.35, z), col));
    parts.push(paint(T(box(0.05, 0.55, 0.05), 0, 0.95, -0.8), col));
    return merged(parts);
  },
  chicken(seed) {
    const col = seed % 2 ? PAL.henBrown : PAL.hen, parts = [];
    parts.push(paint(T(lumpy(ico(0.15, 1, 1.0, 0.95, 1.3), 0.06, seed + 1), 0, 0.26, 0), col), paint(T(ico(0.085, 1), 0, 0.42, 0.14), col));
    parts.push(paint(T(box(0.03, 0.06, 0.07), 0, 0.51, 0.13), PAL.comb, 0));
    const beak = new THREE.ConeGeometry(0.025, 0.07, 4); beak.rotateX(Math.PI / 2); parts.push(paint(T(beak, 0, 0.41, 0.24), PAL.beak, 0));
    const tail = box(0.1, 0.16, 0.05); tail.rotateX(-0.5); parts.push(paint(T(tail, 0, 0.38, -0.2), col));
    for (const x of [-0.05, 0.05]) parts.push(paint(T(cyl(0.012, 0.012, 0.14, 3), x, 0.07, 0), PAL.beak));
    return merged(parts);
  },
  villager(seed) {
    const V = [
      { shirt: '#6f9fae', legs: '#5b5148', hat: '#5c4b40', skin: PAL.skin },
      { shirt: '#c9665a', hat: '#7a5a44', skin: PAL.skin, dress: true, apron: '#f3e8d2' },
      { shirt: '#e7d9b5', legs: '#4d5566', hat: '#3a3036', skin: PAL.skinB, vest: '#7d6a4f' },
      { shirt: '#9fb06a', hair: '#3a3036', skin: PAL.skinB, dress: true, apron: '#efe2c4' },
    ][seed % 4];
    const parts = [];
    if (V.dress) { parts.push(paint(T(cyl(0.3, 0.2, 0.86, 10), 0, 0.43, 0), V.shirt)); parts.push(paint(T(box(0.3, 0.6, 0.04), 0, 0.56, 0.23), V.apron)); }
    else for (const x of [-0.1, 0.1]) parts.push(paint(T(box(0.14, 0.86, 0.17), x, 0.43, 0), V.legs));
    parts.push(paint(T(cyl(0.21, 0.19, 0.62, 8), 0, 1.16, 0), V.shirt));
    if (V.vest) parts.push(paint(T(cyl(0.22, 0.2, 0.44, 8), 0, 1.24, 0), V.vest));
    for (const x of [-0.27, 0.27]) { const arm = box(0.1, 0.6, 0.12); arm.rotateZ(x > 0 ? -0.08 : 0.08); parts.push(paint(T(arm, x, 1.13, 0), V.shirt), paint(T(ico(0.06, 0), x * 1.07, 0.8, 0), V.skin)); }
    parts.push(paint(T(cyl(0.07, 0.07, 0.1, 6), 0, 1.5, 0), V.skin), paint(T(ico(0.135, 1, 1, 1.1, 1), 0, 1.66, 0), V.skin));
    if (V.hat) parts.push(paint(T(cyl(0.25, 0.25, 0.025, 12), 0, 1.78, 0), V.hat), paint(T(cyl(0.14, 0.13, 0.15, 10), 0, 1.87, 0), V.hat));
    else parts.push(paint(T(ico(0.145, 1, 1, 0.75, 1), 0, 1.72, -0.02), V.hair));
    return merged(parts);
  },
  slime() { return merged([paint(lumpy(ico(0.6, 2, 1.1, 0.75, 1.1), 0.04, 2), '#7ccd8f'), paint(T(ico(0.22, 1), 0, 0.12, 0), '#2d4a33')]); },
  spider() {
    const parts = [paint(T(ico(0.5, 1, 1.1, 0.6, 1.3), 0, 0.5, 0), '#6b5340'), paint(T(ico(0.32, 1, 1, 0.7, 1), 0, 0.45, 0.75), '#6b5340')];
    for (let i = 0; i < 8; i++) { const side = i < 4 ? 1 : -1, k = i % 4, g = cyl(0.03, 0.05, 1.4, 4); g.rotateZ(side * 1.15); g.rotateY((k - 1.5) * 0.45); g.translate(side * 0.55, 0.55, 0.3 - k * 0.3); parts.push(paint(g, '#6b5340')); }
    return merged(parts);
  },
};
const PROC = new Map();
// A kit geometry by name: 'oak:2', 'flower:pink', 'picket' ...
function proc(name) {
  if (PROC.has(name)) return PROC.get(name);
  const [kind, v = '0'] = name.split(':');
  const b = BUILDERS[kind];
  const g = b ? b(parseInt(v, 10) || 0, v) : null;
  PROC.set(name, g);
  return g;
}
const TREE_VARIANTS = { oak: 3, birch: 2, pine: 2, deadpine: 2, apple: 2, bush: 3 };

// ================================================================ webs: radial webs between two anchors, sheet webs on the ground
function webGeometry(webs) {
  const pos = [], cols = [], flags = [];
  const strip = (p, q, nrm, w) => {
    const d = q.clone().sub(p), side = new THREE.Vector3().crossVectors(d, nrm).normalize().multiplyScalar(w);
    const a = p.clone().add(side), b = p.clone().sub(side), c = q.clone().sub(side), e = q.clone().add(side);
    pos.push(a.x, a.y, a.z, b.x, b.y, b.z, c.x, c.y, c.z, a.x, a.y, a.z, c.x, c.y, c.z, e.x, e.y, e.z);
    cols.push(PAL.web, PAL.web); flags.push(0, 0);
  };
  for (const w of webs) {
    if (w.sheet) {
      // a sheet web on the ground: threads radiating from a burrow, with a few rings
      const cx = toX(w.Y), cz = toZ(w.X), gy = (groundAt(cx, cz) || 0);
      const up = new THREE.Vector3(0, 1, 0), spokes = 12, R = w.r / 100;
      const pt = (ang, rad) => { const x = cx + Math.cos(ang) * rad, z = cz + Math.sin(ang) * rad; const y = groundAt(x, z); return new THREE.Vector3(x, (Number.isNaN(y) ? gy : y) + 0.07, z); };
      for (let k = 0; k < spokes; k++) strip(pt(k / spokes * 6.283, 1.2), pt(k / spokes * 6.283 + 0.05, R), up, 0.03);
      for (const f of [0.45, 0.7, 0.92]) for (let k = 0; k < spokes; k++) strip(pt(k / spokes * 6.283, R * f), pt((k + 1) / spokes * 6.283, R * f), up, 0.025);
      continue;
    }
    const ax = toX(w.a[1]), az = toZ(w.a[0]), bx = toX(w.b[1]), bz = toZ(w.b[0]);
    const ga = groundAt(ax, az) || 0, gb = groundAt(bx, bz) || 0, mid = (w.h0 + w.h1) / 2;
    const A = new THREE.Vector3(ax, ga + mid, az), B = new THREE.Vector3(bx, gb + mid, bz);
    const center = A.clone().lerp(B, 0.5); center.y -= 0.1;
    const u = B.clone().sub(A); const span = u.length(); u.normalize();
    const v = new THREE.Vector3(0, 1, 0), nrm = new THREE.Vector3().crossVectors(u, v).normalize();
    const R = Math.min(span * 0.42, (w.h1 - w.h0) / 2 + 0.35, 1.9);
    const pt = (ang, rad) => center.clone().addScaledVector(u, Math.cos(ang) * rad).addScaledVector(v, Math.sin(ang) * rad * 0.92);
    const spokes = 10, rings = 6;
    for (let k = 0; k < spokes; k++) strip(center, pt(k / spokes * 6.283, R), nrm, 0.018);
    for (let j = 1; j <= rings; j++) { const rad = R * j / rings; for (let k = 0; k < spokes; k++) strip(pt(k / spokes * 6.283, rad), pt((k + 1) / spokes * 6.283, rad), nrm, 0.014); }
    strip(pt(0, R), A.clone().addScaledVector(u, 0.2), nrm, 0.018);
    strip(pt(Math.PI, R), B.clone().addScaledVector(u, -0.2), nrm, 0.018);
    strip(pt(Math.PI * 1.5, R), new THREE.Vector3(center.x, Math.min(ga, gb) + 0.05, center.z), nrm, 0.018);
  }
  if (!pos.length) return null;
  return soup(pos, cols, flags);
}

// ================================================================ a concept's spec and the helpers that fill it
function newSpec(seed) {
  return { seed, items: [], roads: [], fences: [], fields: [], patches: [], water: [], veg: [], webs: [], creatures: [], noTrees: [],
    hedges: [], smoke: [], labels: [], views: { above: [], foot: [] }, text: {}, flowerBeds: [], tufts: [], bricks: [], cobbles: [], trails: [] };
}
const item = (model, X, Y, yaw = 0, extra = {}) => ({ model, X, Y, yaw, sink: 0.03, ...extra });
const bldg = (model, X, Y, yaw = 0, extra = {}) => ({ model, X, Y, yaw, sink: 0.14, ...extra });
const circle = (X, Y, r) => ({ X, Y, r });
const label = (S, text, X, Y, h = 6, kind = '') => S.labels.push({ text, X, Y, h, kind });
const above = (S, name, X, Y, dist, yaw, pitch, h = 2) => S.views.above.push({ name, X, Y, dist, yaw, pitch, h });
const foot = (S, name, X, Y, yaw, pitch = -3) => S.views.foot.push({ name, X, Y, yaw, pitch });

// A building with a lived-in yard: a bench and a lamp by the door, a flower bed along the front, firewood and barrels
// at the side, a fenced kitchen garden behind it, washing on a line, and smoke when the model has a chimney socket.
function house(S, model, X, Y, yaw, o = {}) {
  const sc = o.scale || 1;
  S.items.push(bldg(model, X, Y, yaw, { scale: sc }));
  for (const sk of socketOf(model, 'Smoke')) S.smoke.push({ X, Y, yaw, model, scale: sc, socket: sk });
  const b = modelBounds(model); if (!b) return;
  const halfW = (b.max[0] - b.min[0]) / 2 * sc * 100, front = b.max[2] * sc * 100, back = -b.min[2] * sc * 100;
  const rnd = mulberry32(hashSeed(X, Y, 7));
  const at = (f, r) => local(X, Y, yaw, f, r);
  S.noTrees.push(circle(X, Y, Math.max(halfW, front, back) + 450));
  if (o.bench !== false) { const [x, y] = at(front + 80, halfW * 0.45); S.items.push(item('Bench', x, y, yaw)); }
  if (o.lamp !== false) { const [x, y] = at(front + 110, -halfW - 60); S.items.push(item(o.lamp === 'lantern' ? 'LanternPost' : 'LampPost', x, y, yaw + 180)); }
  if (o.flowers !== false) {
    const [x0, y0] = at(front + 40, -halfW * 0.85), [x1, y1] = at(front + 40, -halfW * 0.2);
    S.flowerBeds.push({ pts: [[x0, y0], [x1, y1]], every: 38, colors: o.flowerColors || ['pink', 'yellow', 'white'] });
  }
  if (o.firewood !== false) { const [x, y] = at(0, -halfW - 90); S.items.push(item('FirewoodStack', x, y, yaw + 90)); }
  if (o.barrels !== false) {
    const [x, y] = at(-back - 70, halfW * 0.6); S.items.push(item(rnd() < 0.5 ? 'Barrel_A' : 'Barrel_B', x, y, rnd() * 360));
    const [x2, y2] = at(-back - 70, halfW * 0.6 + 85); S.items.push(item('Barrel_A', x2, y2, rnd() * 360));
    const [x3, y3] = at(-back - 60, halfW * 0.6 - 110); S.items.push(item('Crate_A', x3, y3, yaw + rnd() * 30));
  }
  if (o.garden) {
    const gw = o.garden.w || 900, gd = o.garden.d || 650, [gx, gy] = at(-back - 180 - gd / 2, o.garden.r || 0);
    fieldRect(S, gx, gy, gd, gw, yaw, 'crop', { fence: 'picket' });
    if (o.garden.scarecrow) { S.items.push({ model: 'k:scarecrow', X: gx, Y: gy, yaw: yaw + 20, sink: 0.05 }); }
  }
  if (o.laundry) { const [x, y] = at(-back - 120, halfW + 260); S.items.push(item('LaundryLine', x, y, yaw + 90)); }
  if (o.outhouse) { const [x, y] = at(-back - 450, halfW + 420); S.items.push(bldg('Outhouse', x, y, yaw + 180, { sink: 0.08 })); }
  if (o.path) { const [dx, dy] = at(front + 60, 0); S.roads.push({ kind: 'path', width: 150, points: [[dx, dy], ...o.path] }); }
}
// A rectangular field (cm): crop rows, wheat, plowed furrows or pasture, with an optional fence around it.
function fieldRect(S, X, Y, length, width, yaw, kind, o = {}) {
  const corners = [[-length / 2, -width / 2], [length / 2, -width / 2], [length / 2, width / 2], [-length / 2, width / 2]].map(([f, r]) => local(X, Y, yaw, f, r));
  S.fields.push({ poly: corners, kind, X, Y, length, width, yaw });
  S.noTrees.push({ poly: corners });
  if (o.fence) S.fences.push({ kind: o.fence, points: [...corners, corners[0]], broken: o.broken, gap: o.gap ?? 1 });
}
function road(S, kind, width, points, o = {}) {
  S.roads.push({ kind, width, points });
  if (o.lamps) alongLine(points, o.lamps, (X, Y, yaw, k) => { const [x, y] = local(X, Y, yaw, 0, (k % 2 ? 1 : -1) * (width / 2 + 90)); S.items.push(item('LampPost', x, y, yaw + (k % 2 ? -90 : 90))); }, o.lampStart || 300);
  if (o.hedges) for (const side of o.hedges) S.hedges.push({ pts: offsetPolyline(points, side * (width / 2 + 170)) });
  if (o.fences) for (const side of o.fences) S.fences.push({ kind: o.fenceKind || 'rail', points: offsetPolyline(points, side * (width / 2 + 120)), broken: o.broken });
  if (o.verge !== false) S.tufts.push({ along: points, offset: width / 2 + 40, every: 70 });
}
function slimeGroup(S, X, Y, n, spread, seed = 1) {
  const rnd = mulberry32(seed);
  for (let i = 0; i < n; i++) { const a = i / n * 6.283 + rnd() * 0.6, r = spread * (0.35 + rnd() * 0.65); S.creatures.push({ kind: 'slime', X: X + Math.cos(a) * r, Y: Y + Math.sin(a) * r, yaw: rnd() * 360 }); }
}
function spiderGroup(S, X, Y, n, spread, seed = 2) {
  const rnd = mulberry32(seed);
  for (let i = 0; i < n; i++) { const a = i / n * 6.283 + rnd() * 0.5, r = spread * (0.3 + rnd() * 0.7); S.creatures.push({ kind: 'spider', X: X + Math.cos(a) * r, Y: Y + Math.sin(a) * r, yaw: rnd() * 360 }); }
}
// Livestock in a loose flock, and townsfolk standing where they work.
function flock(S, kind, X, Y, n, spread, seed = 3) {
  const rnd = mulberry32(seed);
  for (let i = 0; i < n; i++) { const a = rnd() * 6.283, r = Math.sqrt(rnd()) * spread; S.creatures.push({ kind, X: X + Math.cos(a) * r, Y: Y + Math.sin(a) * r, yaw: rnd() * 360, variant: Math.floor(rnd() * 2) }); }
}
const folk = (S, X, Y, yaw, variant = 0) => S.creatures.push({ kind: 'villager', X, Y, yaw, variant, scale: 0.94 + ((variant * 37 + Math.abs(X)) % 9) / 100 });
// Dead trees with webs strung between neighbours, cocoons hanging from them, egg sacs at their feet.
function deadGrove(S, trees, o = {}) {
  trees.forEach(([X, Y, yaw], i) => {
    S.items.push({ model: o.pines ? `k:deadpine:${i % 2}` : 'DeadTree_A', X, Y, yaw, sink: 0.05, scale: 0.9 + ((i * 37) % 5) * 0.06 });
    if (i % 2 === 0) { const [cx, cy] = local(X, Y, yaw, 120, 60); S.items.push({ model: 'k:cocoon', X: cx, Y: cy, yaw: i * 40, lift: 2.4 + (i % 3) * 0.4 }); }
    if (i % 3 === 1) { const [sx, sy] = local(X, Y, yaw, -110, 80); S.items.push({ model: `k:sacs:${i}`, X: sx, Y: sy, yaw: i * 50 }); }
  });
  for (let i = 0; i < trees.length - 1; i++) {
    const a = trees[i], b = trees[i + 1];
    if (Math.hypot(a[0] - b[0], a[1] - b[1]) < 1500) S.webs.push({ a: [a[0], a[1]], b: [b[0], b[1]], h0: 1.6 + (i % 2) * 0.5, h1: 3.6 + (i % 3) * 0.4 });
  }
}
