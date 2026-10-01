// Skyreach as a model: the terrain rebuilt from Art/Levels/TutorialIsland/layout.json (outline, plateau and ramp,
// hills, pond, creek, pads, roads) with the placed buildings and trees, and a plan view drawn from the same data.
// Unreal centimeters in the data (X north, Y east); the model works in meters with three.js axes (x east, y up,
// z south).
const Island = (() => {
  const D = window.ISLAND;
  const smooth = (a, b, v) => { const t = Math.max(0, Math.min(1, (v - a) / (b - a))); return t * t * (3 - 2 * t); };
  const lerp = (a, b, t) => a + (b - a) * t;

  // Value noise, enough for gentle ground variation.
  function hash(i, j) {
    let h = (i * 374761393 + j * 668265263) | 0;
    h = (h ^ (h >>> 13)) * 1274126177;
    return ((h ^ (h >>> 16)) >>> 0) / 4294967295;
  }
  function noise(x, y) {
    const i = Math.floor(x), j = Math.floor(y), fx = x - i, fy = y - j;
    const u = fx * fx * (3 - 2 * fx), v = fy * fy * (3 - 2 * fy);
    return lerp(lerp(hash(i, j), hash(i + 1, j), u), lerp(hash(i, j + 1), hash(i + 1, j + 1), u), v) * 2 - 1;
  }
  // Distance from a point to a polyline, with where along it (0..1) the nearest point is and the interpolated z.
  function toPolyline(pts, X, Y) {
    let best = Infinity, bestZ = 0, along = 0, total = 0, bestAlong = 0;
    for (let i = 0; i < pts.length - 1; i++) {
      const [ax, ay, az = 0] = pts[i], [bx, by, bz = 0] = pts[i + 1];
      const dx = bx - ax, dy = by - ay, L2 = dx * dx + dy * dy, L = Math.sqrt(L2);
      let t = L2 ? ((X - ax) * dx + (Y - ay) * dy) / L2 : 0;
      t = Math.max(0, Math.min(1, t));
      const px = ax + dx * t, py = ay + dy * t;
      const d = Math.hypot(X - px, Y - py);
      if (d < best) { best = d; bestZ = az + (bz - az) * t; bestAlong = total + L * t; }
      total += L;
    }
    return { d: best, z: bestZ, t: total ? bestAlong / total : 0 };
  }
  function inside(poly, X, Y) {
    let c = false;
    for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
      const [xi, yi] = poly[i], [xj, yj] = poly[j];
      if ((yi > Y) !== (yj > Y) && X < ((xj - xi) * (Y - yi)) / (yj - yi) + xi) c = !c;
    }
    return c;
  }
  function signedDist(poly, X, Y) {
    const closed = poly.concat([poly[0]]);
    const d = toPolyline(closed, X, Y).d;
    return inside(poly, X, Y) ? d : -d;
  }

  const plateau = D.features.find((f) => f.id === 'plateau');
  const hills = D.features.filter((f) => f.type === 'hill');
  const pads = D.features.filter((f) => f.type === 'pad' && f.id !== 'lookout_pad');
  const pond = D.features.find((f) => f.type === 'pond');

  // Ground height in centimeters, and what covers it.
  function ground(X, Y) {
    let h = noise(X / 4200 + 3.1, Y / 4200 + 7.7) * 140 + noise(X / 600, Y / 600) * 25;
    for (const f of hills) {
      const d = Math.hypot(X - f.center[0], Y - f.center[1]);
      if (d < f.radius) h += f.height * 0.5 * (1 + Math.cos((Math.PI * d) / f.radius));
    }
    for (const p of pads) {
      const dx = Math.max(Math.abs(X - p.center[0]) - p.size[0] / 2, 0), dy = Math.max(Math.abs(Y - p.center[1]) - p.size[1] / 2, 0);
      h = lerp(h, p.height, 1 - smooth(0, 500, Math.hypot(dx, dy)));
    }
    const sdP = signedDist(plateau.polygon, X, Y);
    const onPlateau = smooth(-180, 180, sdP);
    h = lerp(h, 900 + noise(X / 900, Y / 900) * 30, onPlateau);
    let kind = onPlateau > 0.02 && onPlateau < 0.98 ? 'cliff' : onPlateau >= 0.98 ? 'plateau' : 'meadow';
    const ramp = toPolyline(D.ramp, X, Y);
    if (ramp.d < 480 && ramp.t > 0 && ramp.t < 1) {
      const w = 1 - smooth(225, 480, ramp.d);
      h = lerp(h, ramp.z, w);
      if (w > 0.5) kind = 'path';
    }
    const e = Math.hypot((X - pond.center[0]) / pond.radii[0], (Y - pond.center[1]) / pond.radii[1]);
    if (e < 1.3) h = lerp(h, -180, 1 - smooth(0.55, 1.05, e));
    if (e < 0.95) kind = 'water';
    const creek = toPolyline(D.creek, X, Y);
    if (creek.d < 300) {
      h = Math.min(h, lerp(h, creek.z - 60, 1 - smooth(90, 300, creek.d)));
      if (creek.d < 160) kind = 'water';
    }
    if (kind === 'meadow' || kind === 'plateau') {
      for (const r of D.roads) {
        const q = toPolyline(r.points, X, Y);
        if (q.d < r.width / 2) { kind = 'road'; break; }
      }
      if (kind !== 'road' && inside(D.zones.forest, X, Y)) kind = kind === 'plateau' ? 'plateau' : 'forest';
      if (kind === 'meadow' && (inside(D.zones.farmstead, X, Y) || inside(D.zones.orchard, X, Y))) kind = 'farm';
    }
    const sdO = signedDist(D.outline, X, Y);
    h -= 600 * (1 - smooth(0, 1800, sdO));
    if (sdO < 600 && kind !== 'water') kind = sdO < 250 ? 'rim' : kind;
    return { h, kind };
  }

  const COLORS = {
    meadow: [0.47, 0.58, 0.31], farm: [0.6, 0.6, 0.36], forest: [0.33, 0.45, 0.24], plateau: [0.53, 0.56, 0.35],
    cliff: [0.52, 0.49, 0.44], road: [0.58, 0.47, 0.33], path: [0.6, 0.5, 0.36], water: [0.3, 0.48, 0.56], rim: [0.45, 0.41, 0.36],
  };

  // The outline, smoothed, as a radius for every direction from the island's middle.
  function outlineRadius(M) {
    const P = D.outline;
    const pts = [];
    for (let i = 0; i < P.length; i++) {
      const p0 = P[(i - 1 + P.length) % P.length], p1 = P[i], p2 = P[(i + 1) % P.length], p3 = P[(i + 2) % P.length];
      for (let k = 0; k < 16; k++) {
        const t = k / 16, t2 = t * t, t3 = t2 * t;
        const f = (a, b, c, d) => 0.5 * (2 * b + (-a + c) * t + (2 * a - 5 * b + 4 * c - d) * t2 + (-a + 3 * b - 3 * c + d) * t3);
        pts.push([f(p0[0], p1[0], p2[0], p3[0]), f(p0[1], p1[1], p2[1], p3[1])]);
      }
    }
    const radii = [];
    for (let j = 0; j < M; j++) {
      const a = (j / M) * Math.PI * 2, cx = Math.cos(a), cy = Math.sin(a);
      let r = 0;
      for (let i = 0; i < pts.length; i++) {
        const [ax, ay] = pts[i], [bx, by] = pts[(i + 1) % pts.length];
        const ex = bx - ax, ey = by - ay, den = cx * ey - cy * ex;
        if (Math.abs(den) < 1e-9) continue;
        const t = (ax * ey - ay * ex) / den, u = (ax * cy - ay * cx) / den;
        if (t > 0 && u >= 0 && u <= 1) r = Math.max(r, t);
      }
      radii.push(r);
    }
    return radii;
  }

  function buildTerrain(THREE) {
    const M = 300, R = 96, K = 26;
    const radii = outlineRadius(M);
    const pos = [], col = [];
    const idx = [];
    const vid = (i, j) => (i === 0 ? 0 : 1 + (i - 1) * M + (j % M));
    // Center vertex.
    const c0 = ground(0, 0);
    pos.push(0, c0.h / 100, 0);
    col.push(...COLORS[c0.kind]);
    for (let i = 1; i <= R; i++) {
      const f = Math.sqrt(i / R);
      for (let j = 0; j < M; j++) {
        const a = (j / M) * Math.PI * 2;
        const X = Math.cos(a) * radii[j] * f, Y = Math.sin(a) * radii[j] * f;
        const g = ground(X, Y);
        pos.push(Y / 100, g.h / 100, -X / 100);
        const tint = 0.92 + noise(X / 700, Y / 700) * 0.08;
        const c = COLORS[g.kind];
        col.push(c[0] * tint, c[1] * tint, c[2] * tint);
      }
    }
    for (let j = 0; j < M; j++) idx.push(0, vid(1, j + 1), vid(1, j));
    for (let i = 2; i <= R; i++) {
      for (let j = 0; j < M; j++) {
        const a = vid(i - 1, j), b = vid(i - 1, j + 1), c = vid(i, j), d = vid(i, j + 1);
        idx.push(a, b, c, b, d, c);
      }
    }
    // The rocky underside: rings that narrow and drop to a jagged point.
    const rimStart = pos.length / 3 - M;
    const under = [];
    for (let k = 1; k <= K; k++) {
      const u = k / K;
      for (let j = 0; j < M; j++) {
        const a = (j / M) * Math.PI * 2;
        const jag = 1 + 0.16 * noise(j * 0.09, k * 0.55) * u;
        const s = (1 - Math.pow(u, 0.85)) * jag;
        const X = Math.cos(a) * radii[j] * s, Y = Math.sin(a) * radii[j] * s;
        const top = pos[(rimStart + j) * 3 + 1];
        const y = top - 4 - u * 66 + noise(j * 0.21, k * 0.9) * 3.5 * u;
        pos.push(Y / 100, y, -X / 100);
        const shade = 0.5 - u * 0.24 + noise(j * 0.3, k) * 0.04;
        col.push(shade * 0.95, shade * 0.85, shade * 0.75);
        under.push(pos.length / 3 - 1);
      }
    }
    const tip = pos.length / 3;
    pos.push(0.5, -74, -2);
    col.push(0.2, 0.17, 0.15);
    const ring = (k, j) => (k === 0 ? rimStart + (j % M) : rimStart + M + (k - 1) * M + (j % M));
    for (let k = 1; k <= K; k++) {
      for (let j = 0; j < M; j++) {
        const a = ring(k - 1, j), b = ring(k - 1, j + 1), c = ring(k, j), d = ring(k, j + 1);
        idx.push(a, b, c, b, d, c);
      }
    }
    for (let j = 0; j < M; j++) idx.push(ring(K, j), ring(K, j + 1), tip);
    const geo = new THREE.BufferGeometry();
    geo.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    geo.setAttribute('color', new THREE.Float32BufferAttribute(col, 3));
    geo.setIndex(idx);
    geo.computeVertexNormals();
    return new THREE.Mesh(geo, new THREE.MeshLambertMaterial({ vertexColors: true }));
  }

  // Simple buildings: a box and a pitched roof, turned to the placement's yaw.
  function house(THREE, w, d, h, wall, roof) {
    const g = new THREE.Group();
    const body = new THREE.Mesh(new THREE.BoxGeometry(w, h, d), new THREE.MeshLambertMaterial({ color: wall }));
    body.position.y = h / 2;
    g.add(body);
    const shape = new THREE.Shape();
    shape.moveTo(-w / 2 - 0.4, 0);
    shape.lineTo(0, h * 0.6);
    shape.lineTo(w / 2 + 0.4, 0);
    shape.lineTo(-w / 2 - 0.4, 0);
    const r = new THREE.Mesh(new THREE.ExtrudeGeometry(shape, { depth: d + 0.8, bevelEnabled: false }), new THREE.MeshLambertMaterial({ color: roof }));
    r.position.set(0, h, -(d + 0.8) / 2);
    g.add(r);
    return g;
  }
  function place(obj, P, yawOffset) {
    const [X, Y, Z] = P.location;
    obj.position.set(Y / 100, Z / 100 - 0.1, -X / 100);
    obj.rotation.y = -((P.yaw + (yawOffset || 0)) * Math.PI) / 180;
    return obj;
  }

  function buildProps(THREE, scene) {
    const P = D.placements;
    const wood = 0x7d5f45, tin = 0x7b858c, shake = 0x5e4636, plaster = 0xc9bfa6;
    scene.add(place(house(THREE, 12, 8, 5, wood, tin), P.farmhouse));
    scene.add(place(house(THREE, 10, 14, 6.5, 0x8a3f2e, tin), P.barn));
    scene.add(place(house(THREE, 9, 7, 4, 0x6b4c34, shake), P.log_cabin));
    scene.add(place(house(THREE, 8, 7, 4.5, plaster, shake), P.cottage));
    scene.add(place(house(THREE, 1.6, 1.6, 2.4, wood, shake), P.outhouse));
    const well = new THREE.Mesh(new THREE.CylinderGeometry(1.2, 1.3, 1, 14), new THREE.MeshLambertMaterial({ color: 0x8c877c }));
    well.position.y = 0.5;
    const wg = new THREE.Group();
    wg.add(well);
    scene.add(place(wg, P.well));
    const rack = new THREE.Mesh(new THREE.BoxGeometry(2.4, 1.4, 0.6), new THREE.MeshLambertMaterial({ color: 0x5b4331 }));
    rack.position.y = 0.7;
    const rg = new THREE.Group();
    rg.add(rack);
    scene.add(place(rg, P.gun_rack));
    // The windmill: a tapering tower and a turning fan.
    const mill = new THREE.Group();
    const tower = new THREE.Mesh(new THREE.CylinderGeometry(0.5, 1.6, 14, 4, 1, true), new THREE.MeshLambertMaterial({ color: 0x7a7066, side: THREE.DoubleSide }));
    tower.position.y = 7;
    mill.add(tower);
    const fan = new THREE.Group();
    for (let i = 0; i < 12; i++) {
      const blade = new THREE.Mesh(new THREE.BoxGeometry(0.5, 3.6, 0.08), new THREE.MeshLambertMaterial({ color: 0xb9b2a4 }));
      blade.position.y = 2.2;
      const arm = new THREE.Group();
      arm.add(blade);
      arm.rotation.z = (i / 12) * Math.PI * 2;
      fan.add(arm);
    }
    fan.position.set(0, 14.2, 0.9);
    mill.add(fan);
    scene.add(place(mill, P.windmill, 90));
    // The lookout tower on the plateau.
    const look = new THREE.Group();
    const legs = new THREE.Mesh(new THREE.CylinderGeometry(1.4, 2.6, 10, 4, 1, true), new THREE.MeshLambertMaterial({ color: 0x5f4a38, side: THREE.DoubleSide, wireframe: false }));
    legs.position.y = 5;
    look.add(legs);
    const deck = new THREE.Mesh(new THREE.BoxGeometry(4.4, 0.5, 4.4), new THREE.MeshLambertMaterial({ color: 0x4f3d2f }));
    deck.position.y = 10.2;
    look.add(deck);
    scene.add(place(look, P.lookout));
    for (const k of ['dummy_1', 'dummy_2', 'dummy_3']) {
      const g = new THREE.Group();
      const m = new THREE.Mesh(new THREE.CylinderGeometry(0.3, 0.3, 1.8, 8), new THREE.MeshLambertMaterial({ color: 0xc8b07a }));
      m.position.y = 0.9;
      g.add(m);
      scene.add(place(g, P[k]));
    }
    return fan;
  }

  // Trees: the grove, the orchard rows and a few lone meadow trees, as instances.
  function buildTrees(THREE, scene) {
    const R = (() => { let a = 1234567; return () => ((a = (a * 16807) % 2147483647) / 2147483647); })();
    const spots = [];
    const F = D.zones.forest;
    const bx = [Math.min(...F.map((p) => p[0])), Math.max(...F.map((p) => p[0]))], by = [Math.min(...F.map((p) => p[1])), Math.max(...F.map((p) => p[1]))];
    let tries = 0;
    while (spots.length < 170 && tries++ < 4000) {
      const X = lerp(bx[0], bx[1], R()), Y = lerp(by[0], by[1], R());
      if (!inside(F, X, Y)) continue;
      const g = ground(X, Y);
      if (g.kind === 'road' || g.kind === 'water') continue;
      spots.push([X, Y, g.h, R() < 0.45 ? 'pine' : 'oak', 0.8 + R() * 0.5]);
    }
    for (const row of D.orchardRows) {
      const [x0, y0] = row.start, [x1, y1] = row.end;
      const n = Math.max(2, Math.round(Math.hypot(x1 - x0, y1 - y0) / 450) + 1);
      for (let i = 0; i < n; i++) {
        const X = lerp(x0, x1, i / (n - 1)), Y = lerp(y0, y1, i / (n - 1));
        spots.push([X, Y, ground(X, Y).h, 'apple', 0.7]);
      }
    }
    tries = 0;
    let lone = 0;
    while (lone < 26 && tries++ < 3000) {
      const a = R() * Math.PI * 2, r = 2000 + R() * 7000;
      const X = Math.cos(a) * r, Y = Math.sin(a) * r;
      const g = ground(X, Y);
      if (g.kind !== 'meadow' && g.kind !== 'plateau') continue;
      spots.push([X, Y, g.h, R() < 0.3 ? 'pine' : 'oak', 0.9 + R() * 0.6]);
      lone++;
    }
    const kinds = {
      pine: { geo: new THREE.ConeGeometry(2.4, 9, 7), color: 0x2f4a2c, y: 6 },
      oak: { geo: new THREE.SphereGeometry(3.4, 8, 6), color: 0x3d5a2e, y: 6.2 },
      apple: { geo: new THREE.SphereGeometry(2.2, 8, 6), color: 0x55703a, y: 3.4 },
    };
    const trunkGeo = new THREE.CylinderGeometry(0.25, 0.35, 4, 5);
    const trunks = new THREE.InstancedMesh(trunkGeo, new THREE.MeshLambertMaterial({ color: 0x4b3a2c }), spots.length);
    const m = new THREE.Matrix4();
    const q = new THREE.Quaternion();
    spots.forEach(([X, Y, h, , s], i) => {
      m.compose(new THREE.Vector3(Y / 100, h / 100 + 2 * s, -X / 100), q, new THREE.Vector3(s, s, s));
      trunks.setMatrixAt(i, m);
    });
    scene.add(trunks);
    for (const [k, def] of Object.entries(kinds)) {
      const list = spots.filter((p) => p[3] === k);
      const inst = new THREE.InstancedMesh(def.geo, new THREE.MeshLambertMaterial({ color: def.color }), list.length);
      list.forEach(([X, Y, h, , s], i) => {
        m.compose(new THREE.Vector3(Y / 100, h / 100 + def.y * s, -X / 100), q, new THREE.Vector3(s, s, s));
        inst.setMatrixAt(i, m);
      });
      scene.add(inst);
    }
  }

  function buildWater(THREE, scene) {
    const water = new THREE.MeshLambertMaterial({ color: 0x4f86a0, transparent: true, opacity: 0.88 });
    const pondMesh = new THREE.Mesh(new THREE.CircleGeometry(1, 40), water);
    pondMesh.rotation.x = -Math.PI / 2;
    pondMesh.scale.set(pond.radii[1] / 100 * 0.95, pond.radii[0] / 100 * 0.95, 1);
    pondMesh.position.set(pond.center[1] / 100, pond.waterLevel / 100, -pond.center[0] / 100);
    scene.add(pondMesh);
    // The creek as a ribbon over its bed.
    const pts = D.creek;
    const pos = [], idx = [];
    for (let i = 0; i < pts.length; i++) {
      const a = pts[Math.max(0, i - 1)], b = pts[Math.min(pts.length - 1, i + 1)];
      const dx = b[1] - a[1], dz = -(b[0] - a[0]), L = Math.hypot(dx, dz) || 1;
      const nx = -dz / L * 1.6, nz = dx / L * 1.6;
      const x = pts[i][1] / 100, z = -pts[i][0] / 100, y = pts[i][2] / 100 + 0.05;
      pos.push(x + nx, y, z + nz, x - nx, y, z - nz);
      if (i) idx.push((i - 1) * 2, i * 2, (i - 1) * 2 + 1, (i - 1) * 2 + 1, i * 2, i * 2 + 1);
    }
    const g = new THREE.BufferGeometry();
    g.setAttribute('position', new THREE.Float32BufferAttribute(pos, 3));
    g.setIndex(idx);
    g.computeVertexNormals();
    scene.add(new THREE.Mesh(g, new THREE.MeshLambertMaterial({ color: 0x4f86a0, side: THREE.DoubleSide })));
    // The waterfall off the rim.
    const wf = D.waterfall;
    const fall = new THREE.Mesh(new THREE.PlaneGeometry(wf.width / 100, (wf.location[2] - wf.dropTo) / 100), new THREE.MeshBasicMaterial({ color: 0xeaf4fb, transparent: true, opacity: 0.55, side: THREE.DoubleSide }));
    const h = (wf.location[2] - wf.dropTo) / 100;
    fall.position.set(wf.location[1] / 100, wf.location[2] / 100 - h / 2, -wf.location[0] / 100);
    fall.rotation.y = -(wf.yaw * Math.PI) / 180;
    scene.add(fall);
  }

  // A soft cloud sea under the island, painted on a canvas.
  function buildClouds(THREE, scene) {
    const c = document.createElement('canvas');
    c.width = c.height = 256;
    const x = c.getContext('2d');
    const g = x.createRadialGradient(128, 128, 10, 128, 128, 128);
    g.addColorStop(0, 'rgba(255,255,255,0.95)');
    g.addColorStop(0.55, 'rgba(244,247,250,0.7)');
    g.addColorStop(1, 'rgba(244,247,250,0)');
    x.fillStyle = g;
    x.fillRect(0, 0, 256, 256);
    const tex = new THREE.CanvasTexture(c);
    const mat = new THREE.MeshBasicMaterial({ map: tex, transparent: true, depthWrite: false });
    const R = (() => { let a = 99; return () => ((a = (a * 16807) % 2147483647) / 2147483647); })();
    for (let i = 0; i < 26; i++) {
      const s = 120 + R() * 220;
      const m = new THREE.Mesh(new THREE.PlaneGeometry(s, s * 0.7), mat);
      m.rotation.x = -Math.PI / 2;
      const a = R() * Math.PI * 2, r = R() * 380;
      m.position.set(Math.cos(a) * r, -92 - R() * 14, Math.sin(a) * r);
      scene.add(m);
    }
  }

  // Story pins: tutorial steps placed where they happen.
  const PINS = [
    { step: 1, name: 'Farmstead', at: () => D.placements.spawn.location, up: 3 },
    { step: 3, name: 'Gun rack', at: () => D.placements.gun_rack.location, up: 3 },
    { step: 4, name: 'Range', at: () => D.placements.dummy_2.location, up: 3 },
    { step: 5, name: 'Grove', at: () => [5600, 5900, ground(5600, 5900).h], up: 12 },
    { step: 7, name: 'Lookout', at: () => D.placements.lookout.location, up: 13 },
  ];

  function mount3d(host, onReady) {
    const THREE = window.THREE;
    const canvas = host.querySelector('canvas');
    const pinLayer = host.querySelector('.pins');
    const renderer = new THREE.WebGLRenderer({ canvas, antialias: true, alpha: true });
    renderer.setPixelRatio(Math.min(window.devicePixelRatio || 1, 2));
    const scene = new THREE.Scene();
    scene.fog = new THREE.Fog(0xc9dbe6, 420, 900);
    const camera = new THREE.PerspectiveCamera(34, 1, 1, 3000);
    scene.add(new THREE.HemisphereLight(0xe6f2ff, 0x4d5a3c, 0.78));
    const sun = new THREE.DirectionalLight(0xffe0b0, 0.95);
    sun.position.set(-140, 120, 80);
    scene.add(sun);
    scene.add(buildTerrain(THREE));
    const fan = buildProps(THREE, scene);
    buildTrees(THREE, scene);
    buildWater(THREE, scene);
    buildClouds(THREE, scene);

    const pins = PINS.map((p) => {
      const L = p.at();
      const v = new THREE.Vector3(L[1] / 100, L[2] / 100 + p.up, -L[0] / 100);
      const el = document.createElement('div');
      el.className = 'pin';
      el.dataset.step = p.step;
      el.innerHTML = `<b>${p.step}</b><span>${p.name}</span>`;
      pinLayer.appendChild(el);
      return { v, el };
    });

    const view = { az: 2.35, el: 0.3, r: 350, target: new THREE.Vector3(0, -14, 0) };
    const reduce = window.matchMedia('(prefers-reduced-motion: reduce)').matches;
    let idle = 0, visible = true;
    function resize() {
      const w = host.clientWidth, h = host.clientHeight;
      renderer.setSize(w, h, false);
      camera.aspect = w / h;
      camera.updateProjectionMatrix();
    }
    function frame() {
      if (!visible) return requestAnimationFrame(frame);
      idle++;
      if (!reduce && idle > 180) view.az += 0.0016;
      fan.rotation.z += reduce ? 0 : 0.01;
      camera.position.set(
        view.target.x + view.r * Math.cos(view.el) * Math.cos(view.az),
        view.target.y + view.r * Math.sin(view.el),
        view.target.z + view.r * Math.cos(view.el) * Math.sin(view.az),
      );
      camera.lookAt(view.target);
      renderer.render(scene, camera);
      const w = host.clientWidth, h = host.clientHeight;
      for (const p of pins) {
        const s = p.v.clone().project(camera);
        const show = s.z < 1;
        p.el.style.transform = `translate(${((s.x + 1) / 2) * w}px, ${((1 - s.y) / 2) * h}px)`;
        p.el.hidden = !show;
      }
      requestAnimationFrame(frame);
    }
    // Drag to turn (horizontal on touch, so the page still scrolls), wheel or buttons to zoom.
    let drag = null;
    canvas.addEventListener('pointerdown', (e) => { drag = { x: e.clientX, y: e.clientY, touch: e.pointerType === 'touch' }; idle = 0; canvas.setPointerCapture(e.pointerId); });
    canvas.addEventListener('pointermove', (e) => {
      if (!drag) return;
      view.az += (e.clientX - drag.x) * 0.006;
      if (!drag.touch) view.el = Math.max(0.15, Math.min(1.35, view.el + (e.clientY - drag.y) * 0.004));
      drag.x = e.clientX;
      drag.y = e.clientY;
      idle = 0;
    });
    const end = () => { drag = null; };
    canvas.addEventListener('pointerup', end);
    canvas.addEventListener('pointercancel', end);
    canvas.addEventListener('wheel', (e) => { e.preventDefault(); view.r = Math.max(150, Math.min(620, view.r * (1 + e.deltaY * 0.001))); idle = 0; }, { passive: false });
    host.querySelector('[data-zoom="in"]').addEventListener('click', () => { view.r = Math.max(150, view.r * 0.82); idle = 0; });
    host.querySelector('[data-zoom="out"]').addEventListener('click', () => { view.r = Math.min(620, view.r * 1.22); idle = 0; });
    host.querySelector('[data-zoom="reset"]').addEventListener('click', () => { Object.assign(view, { az: 2.35, el: 0.3, r: 350 }); idle = 0; });
    new ResizeObserver(resize).observe(host);
    if ('IntersectionObserver' in window) new IntersectionObserver((es) => { visible = es[0].isIntersecting; }).observe(host);
    resize();
    frame();
    if (onReady) onReady();
  }

  // The plan view: north up, meters, from the same layout.
  function plan() {
    const m = (p) => [p[1] / 100, -p[0] / 100];
    const pts = (arr) => arr.map((p) => m(p).map((v) => v.toFixed(1)).join(',')).join(' ');
    let s = `<svg class="plan" viewBox="-108 -108 216 216" role="img" aria-label="Plan of Skyreach, north up: farmstead in the southwest, village at the center, range and windmill north, pond and creek east with the waterfall off the rim, the grove northeast, and the plateau with the lookout in the southeast.">`;
    s += `<polygon points="${pts(D.outline)}" class="p-land"/>`;
    s += `<polygon points="${pts(D.zones.forest)}" class="p-forest"/>`;
    s += `<polygon points="${pts(plateau.polygon)}" class="p-plateau"/>`;
    s += `<ellipse cx="${pond.center[1] / 100}" cy="${-pond.center[0] / 100}" rx="${pond.radii[1] / 100}" ry="${pond.radii[0] / 100}" class="p-water"/>`;
    s += `<polyline points="${pts(D.creek)}" class="p-creek"/>`;
    for (const r of D.roads) s += `<polyline points="${pts(r.points)}" class="p-road" stroke-width="${r.width / 100}"/>`;
    s += `<polyline points="${pts(D.ramp)}" class="p-road" stroke-width="4.5"/>`;
    const P = D.placements;
    const bld = (k, w, d) => {
      const [x, y] = m(P[k].location);
      return `<rect x="${(x - w / 2).toFixed(1)}" y="${(y - d / 2).toFixed(1)}" width="${w}" height="${d}" class="p-bld" transform="rotate(${P[k].yaw} ${x.toFixed(1)} ${y.toFixed(1)})"/>`;
    };
    s += bld('farmhouse', 12, 8) + bld('barn', 10, 14) + bld('log_cabin', 9, 7) + bld('cottage', 8, 7) + bld('outhouse', 2, 2) + bld('lookout', 4.4, 4.4);
    const [wx, wy] = m(P.windmill.location);
    s += `<circle cx="${wx}" cy="${wy}" r="2.2" class="p-bld"/>`;
    const wf = m(D.waterfall.location);
    s += `<path d="M${wf[0]},${wf[1]} l3,3" class="p-creek"/>`;
    const lab = (txt, X, Y) => { const [x, y] = m([X, Y]); return `<text x="${x}" y="${y}" class="p-lab" text-anchor="middle">${txt}</text>`; };
    s += lab('farmstead', -6900, -5300) + lab('village', -700, -300) + lab('range', 4400, -2400) + lab('pond', 1800, 5600) + lab('grove', 6800, 5200) + lab('plateau', -6300, 4000) + lab('orchard', -6400, -7300);
    PINS.forEach((p) => {
      const [x, y] = m(p.at());
      s += `<g class="p-pin" data-step="${p.step}"><circle cx="${x}" cy="${y}" r="5.2"/><text x="${x}" y="${y + 2.2}" text-anchor="middle">${p.step}</text></g>`;
    });
    // North arrow and a 50 m scale bar.
    s += `<g class="p-ink"><path d="M92,-100 L96,-88 L92,-91 L88,-88 Z"/><text x="92" y="-79" text-anchor="middle" class="p-lab">N</text></g>`;
    s += `<g class="p-ink"><rect x="-100" y="96" width="50" height="1.6"/><text x="-75" y="93" text-anchor="middle" class="p-lab">50 m</text></g>`;
    return s + '</svg>';
  }

  return { mount3d, plan, PINS };
})();
