
// ================================================================ Ember powers: a playable sketch on the concept viewer's engine
// The town is the viewer's Crossroads Town (the game's own models in Screen Print Wash). Everything below is the sketch:
// a first-person player with the Whisper Bullpup, spiders raiding the square, the six ember powers, the new HUD with two
// ember sockets, the ember page, and a scripted "watch all six".
(() => {
  // ---------------------------------------------------------------- the six embers
  const EMBERS = [
    { id: 'luck', name: 'Lucky Streak', who: 'Lucky Ned Purcell', where: 'the Gilded Lily', nature: 'FORTUNE', cd: 22, dur: 8, color: '#ffcf4a',
      desc: 'For 8 seconds every shot ricochets into a second spider, and one shot in three is a critical hit.' },
    { id: 'dash', name: 'Dust Devil', who: 'Whistling Ira Gale', where: 'Dustwater Mesa', nature: 'WIND', cd: 7, dur: 0, color: '#e8f6ff',
      desc: 'Dash 9 meters the way you are moving. Spiders in your path are flung aside.' },
    { id: 'flock', name: 'Raise the Flock', who: 'Sister Constance Holloway', where: 'Saint Agnes Mire', nature: 'MERCY', cd: 24, dur: 10, color: '#6ee7ff',
      desc: 'Raise a friendly Unpaid where you aim. For 10 seconds it draws the spiders off you and claws at them.' },
    { id: 'slag', name: 'Slag Bomb', who: 'Barrels Kessler', where: 'Furnace Hollow', nature: 'FORGE', cd: 12, dur: 6, color: '#ff8a1c',
      desc: 'Lob a bomb that bursts into a pool of slag, burning whatever stands in it for 6 seconds.' },
    { id: 'sight', name: 'Spyglass', who: 'Lena "Spyglass" Okoro', where: 'the Needles', nature: 'SIGHT', cd: 18, dur: 10, color: '#ffb24d',
      desc: 'Mark every spider within 60 meters for 10 seconds, through walls. Marked spiders take 30% more damage.' },
    { id: 'slam', name: 'Landslide', who: 'Tobias "Mule" Grant', where: 'Stoneweight Quarry', nature: 'BURDEN', cd: 15, dur: 0, color: '#e0cfa8',
      desc: 'Slam the ground. Spiders within 7 meters are thrown into the air and stunned for 2.5 seconds.' },
  ];
  const EMBER = Object.fromEntries(EMBERS.map((e) => [e.id, e]));
  // Glyphs drawn in a 64-unit box around the origin: white fills over an ink stroke, like the Inked icons.
  const W = '#f4f8fb', INK = '#0a1218';
  const GLYPH = {
    luck: `<g stroke="${INK}" stroke-width="3" stroke-linejoin="round"><rect x="-19" y="-19" width="22" height="30" rx="2.5" fill="${W}" transform="rotate(-16 -8 -4)"/>
      <rect x="-4" y="-15" width="22" height="30" rx="2.5" fill="${W}" transform="rotate(12 7 0)"/></g>
      <path d="M7 -8 C2 -2 -1 2 2.5 5.5 C4.5 7.5 6.5 6.5 7 5 C7.5 6.5 9.5 7.5 11.5 5.5 C15 2 12 -2 7 -8 Z M7 4 L5 10 L9 10 Z" fill="${INK}" transform="rotate(12 7 0)"/>
      <path d="M-14 -11 l2.5 4 l-2.5 4 l-2.5 -4 Z" fill="#d6463a" transform="rotate(-16 -8 -4)"/>`,
    dash: `<g fill="none" stroke-linecap="round"><g stroke="${INK}" stroke-width="9"><path d="M-22 -11 H6 C15 -11 16 -22 8 -22"/><path d="M-26 1 H14 C25 1 25 14 15 14"/><path d="M-18 13 H-2"/></g>
      <g stroke="${W}" stroke-width="5"><path d="M-22 -11 H6 C15 -11 16 -22 8 -22"/><path d="M-26 1 H14 C25 1 25 14 15 14"/><path d="M-18 13 H-2"/></g></g>`,
    flock: `<path d="M0 -23 C12 -23 17 -13 17 -1 L17 16 L11 11 L5.5 17 L0 11.5 L-5.5 17 L-11 11 L-17 16 L-17 -1 C-17 -13 -12 -23 0 -23 Z" fill="${W}" stroke="${INK}" stroke-width="3" stroke-linejoin="round"/>
      <ellipse cx="0" cy="-8" rx="9.5" ry="8" fill="#26343f"/><circle cx="-3.6" cy="-8" r="2.1" fill="#6ee7ff"/><circle cx="3.6" cy="-8" r="2.1" fill="#6ee7ff"/>
      <circle cx="0" cy="5" r="3.4" fill="#6ee7ff" stroke="${INK}" stroke-width="1.5"/>`,
    slag: `<path d="M9 -9 Q14 -19 21 -17" fill="none" stroke="${INK}" stroke-width="6" stroke-linecap="round"/><path d="M9 -9 Q14 -19 21 -17" fill="none" stroke="#c9b28a" stroke-width="3" stroke-linecap="round"/>
      <circle cx="-2" cy="5" r="15" fill="#3a3034" stroke="${INK}" stroke-width="3"/><path d="M-12 1 Q-2 -4 8 1" fill="none" stroke="#ff8a1c" stroke-width="3.5" stroke-linecap="round"/>
      <path d="M-8 -3 Q-6 -8 -1 -9" fill="none" stroke="#ffffff" stroke-opacity=".55" stroke-width="2" stroke-linecap="round"/>
      <path d="M22 -24 l2 5 l5 1 l-4 3 l1 5 l-4 -3 l-4 3 l1 -5 l-4 -3 l5 -1 Z" fill="#ffcf4a" stroke="${INK}" stroke-width="1.6" stroke-linejoin="round"/>`,
    sight: `<path d="M-22 0 Q0 -18 22 0 Q0 18 -22 0 Z" fill="${W}" stroke="${INK}" stroke-width="3" stroke-linejoin="round"/>
      <circle r="8.5" fill="#ff9f1c" stroke="${INK}" stroke-width="2.5"/><circle r="3.5" fill="${INK}"/><circle cx="-2.5" cy="-3" r="1.6" fill="#fff"/>
      <g stroke="${INK}" stroke-width="5" stroke-linecap="round"><path d="M0 -27 V-21 M0 21 V27 M-28 0 H-25 M25 0 H28"/></g>
      <g stroke="${W}" stroke-width="2.4" stroke-linecap="round"><path d="M0 -27 V-21 M0 21 V27 M-28 0 H-25 M25 0 H28"/></g>`,
    slam: `<path d="M-11 -24 H11 V-11 H18 L0 6 L-18 -11 H-11 Z" fill="${W}" stroke="${INK}" stroke-width="3" stroke-linejoin="round"/>
      <path d="M-25 15 H-6 L-2 10 L2 16 L6 12 L9 15 H25" fill="none" stroke="${INK}" stroke-width="6" stroke-linecap="round" stroke-linejoin="round"/>
      <path d="M-25 15 H-6 L-2 10 L2 16 L6 12 L9 15 H25" fill="none" stroke="#e0cfa8" stroke-width="3" stroke-linecap="round" stroke-linejoin="round"/>
      <path d="M-19 4 l5 -4 l3 5 Z M14 2 l6 -3 l1 6 Z M-8 23 l4 -5 l4 5 Z" fill="#cdbb95" stroke="${INK}" stroke-width="1.8" stroke-linejoin="round"/>`,
  };

  // ---------------------------------------------------------------- small helpers
  const V3 = (x = 0, y = 0, z = 0) => new THREE.Vector3(x, y, z);
  const rand = (a, b) => a + Math.random() * (b - a);
  const fwdOf = (yaw) => V3(-Math.sin(yaw), 0, Math.cos(yaw));
  const rightOf = (yaw) => V3(-Math.cos(yaw), 0, -Math.sin(yaw));
  const gy = (x, z) => { const g = groundAt(x, z); return Number.isNaN(g) ? null : g; };
  const ease = (t) => 1 - Math.pow(1 - clamp(t, 0, 1), 3);
  const el = (tag, cls, parent) => { const e = document.createElement(tag); if (cls) e.className = cls; if (parent) parent.appendChild(e); return e; };
  const hudScale = () => parseFloat(getComputedStyle(document.documentElement).getPropertyValue('--s')) || 1;

  // ---------------------------------------------------------------- state
  const G = { mode: 'loading', paused: true, t: 0, layer: null, kills: 0, level: 12, xp: 1240, xpMax: 2000, used: new Set(), swapped: false,
    slots: ['dash', 'slag'], playSlots: ['dash', 'slag'], cd: [0, 0], active: [0, 0], usedFlash: [0, 0], shake: 0, slamDip: 0, fovKick: 0, deadT: 0, hold: 0, step: 0, sway: 0 };
  const keys = new Set();
  const P = { pos: V3(), yaw: 0, pitch: 0, vel: V3(), health: 100, max: 100, hurtT: -99, dash: null, eye: 1.68, bob: 0, recoil: 0, regen: 0 };
  const GUN = { mag: 30, cap: 30, reserve: 240, reloadT: 0, reloadDur: 1.7, fireT: 0, rate: 9, firing: false, luckUntil: 0 };

  // ---------------------------------------------------------------- scene additions
  const DYN = new THREE.Group(); scene.add(DYN);
  const whiteGeo = (g, flag = 1) => paint(g, '#ffffff', flag);
  function pool(geo, mat, n, shadow = false) {
    const m = new THREE.InstancedMesh(geo, mat, n); m.frustumCulled = false; m.count = 0; m.castShadow = shadow; m.receiveShadow = false;
    for (let i = 0; i < n; i++) m.setColorAt(i, C('#ffffff'));
    DYN.add(m); return m;
  }

  // particles: shaded blobs, unshaded glows, shaded chunks; each fades by shrinking (no transparency, as in print)
  const PART_N = 700;
  const partMeshes = {
    blob: pool(whiteGeo(ico(0.5, 1), 1), MAT, PART_N),
    glow: pool(whiteGeo(ico(0.5, 0), 0), MAT, PART_N),
    chunk: pool(whiteGeo(new THREE.TetrahedronGeometry(0.5, 0), 1), MAT, 260),
    streak: pool(whiteGeo(box(0.04, 0.04, 1), 0), MAT, 120),
  };
  const parts = [];
  function spawnPart(kind, pos, o = {}) {
    if (parts.length > 1400) return null;
    const p = { kind, pos: pos.clone(), vel: o.vel ? o.vel.clone() : V3(), grav: o.grav ?? 0, drag: o.drag ?? 0, life: 0, max: o.life ?? 0.8,
      s0: o.s0 ?? 0.2, s1: o.s1 ?? o.s0 ?? 0.2, col: C(o.col || '#ffffff'), rot: V3(rand(0, 6), rand(0, 6), rand(0, 6)), spin: o.spin ?? 2,
      len: o.len, dir: o.dir, floor: o.floor ?? false, hold: o.hold ?? 0.7 };
    parts.push(p); return p;
  }
  const tmpMat = new THREE.Matrix4(), tmpQuat = new THREE.Quaternion(), tmpVec = V3(), tmpScale = V3(), tmpEul = new THREE.Euler();
  function updateParts(dt) {
    const counts = { blob: 0, glow: 0, chunk: 0, streak: 0 };
    for (let i = parts.length - 1; i >= 0; i--) {
      const p = parts[i];
      p.life += dt;
      if (p.life >= p.max) { parts.splice(i, 1); continue; }
      p.vel.y -= p.grav * dt; p.vel.multiplyScalar(Math.max(0, 1 - p.drag * dt)); p.pos.addScaledVector(p.vel, dt);
      if (p.floor) { const g = gy(p.pos.x, p.pos.z); if (g !== null && p.pos.y < g + 0.05) { p.pos.y = g + 0.05; p.vel.y *= -0.3; p.vel.x *= 0.6; p.vel.z *= 0.6; } }
      const mesh = partMeshes[p.kind], idx = counts[p.kind];
      if (idx >= mesh.instanceMatrix.count) continue;
      const k = p.life / p.max, dc = p.pos.distanceTo(camera.position);
      const s = lerp(p.s0, p.s1, ease(k)) * (k > p.hold ? Math.max(0, 1 - (k - p.hold) / (1 - p.hold)) : 1) * (dc < 1.9 ? Math.max(0, (dc - 0.8) / 1.1) : 1);
      if (p.kind === 'streak') {
        tmpQuat.setFromUnitVectors(V3(0, 0, 1), p.dir); tmpScale.set(s, s, p.len * (1 - k * 0.6));
      } else {
        p.rot.x += p.spin * dt; p.rot.y += p.spin * 0.7 * dt; tmpQuat.setFromEuler(tmpEul.set(p.rot.x, p.rot.y, p.rot.z)); tmpScale.setScalar(Math.max(0.0001, s));
      }
      mesh.setMatrixAt(idx, tmpMat.compose(p.pos, tmpQuat, tmpScale)); mesh.setColorAt(idx, p.col);
      counts[p.kind]++;
    }
    for (const [k, m] of Object.entries(partMeshes)) { m.count = counts[k]; m.instanceMatrix.needsUpdate = true; if (m.instanceColor) m.instanceColor.needsUpdate = true; }
  }
  const burst = (kind, at, n, o) => { for (let i = 0; i < n; i++) spawnPart(kind, at.clone().add(V3(rand(-1, 1), rand(-1, 1), rand(-1, 1)).multiplyScalar(o.jitter ?? 0.2)), { ...o, vel: V3(rand(-1, 1), rand(o.up ? 0.2 : -1, 1), rand(-1, 1)).normalize().multiplyScalar(rand(o.v0 ?? 1, o.v1 ?? 3)).add(o.base || V3()), s0: rand(o.sa ?? 0.1, o.sb ?? 0.25), life: rand(o.la ?? 0.4, o.lb ?? 0.9) }); };

  // rings on the ground: translucent, kept out of the line pass
  const rings = [];
  function ring(at, o) {
    const geo = new THREE.RingGeometry(0.88, 1, 72); geo.rotateX(-Math.PI / 2);
    const mat = new THREE.MeshBasicMaterial({ color: C(o.col || '#ffffff'), transparent: true, opacity: o.op ?? 0.85, depthWrite: false, side: THREE.DoubleSide, fog: true });
    mat.polygonOffset = true; mat.polygonOffsetFactor = -4; mat.polygonOffsetUnits = -8;
    const m = new THREE.Mesh(geo, mat); m.position.copy(at); m.renderOrder = 4; DYN.add(m); FINE_MESHES.push(m);
    rings.push({ m, life: 0, max: o.life ?? 0.6, r0: o.r0 ?? 0.3, r1: o.r1 ?? 5, w0: o.w0 ?? 0.3, w1: o.w1 ?? 0.06, op: o.op ?? 0.85 });
  }
  function updateRings(dt) {
    for (let i = rings.length - 1; i >= 0; i--) {
      const r = rings[i]; r.life += dt; const k = r.life / r.max;
      if (k >= 1) { DYN.remove(r.m); r.m.geometry.dispose(); r.m.material.dispose(); FINE_MESHES.splice(FINE_MESHES.indexOf(r.m), 1); rings.splice(i, 1); continue; }
      const rad = lerp(r.r0, r.r1, ease(k)), w = lerp(r.w0, r.w1, k);
      r.m.geometry.dispose(); r.m.geometry = new THREE.RingGeometry(Math.max(0.01, rad - w), rad, 72); r.m.geometry.rotateX(-Math.PI / 2);
      r.m.material.opacity = r.op * (1 - k * k);
    }
  }

  // tracers: thin unshaded bars from the muzzle, gone in a few frames
  const tracerGeo = whiteGeo(box(1, 1, 1), 0);
  const tracers = [];
  function tracer(a, b, col = '#fff4cf', w = 0.022) {
    const m = new THREE.Mesh(tracerGeo, new THREE.MeshBasicMaterial({ color: C(col), fog: true }));
    const d = b.clone().sub(a), len = d.length();
    m.position.copy(a).addScaledVector(d, 0.5); m.quaternion.setFromUnitVectors(V3(0, 0, 1), d.normalize()); m.scale.set(w, w, len);
    DYN.add(m); FINE_MESHES.push(m); tracers.push({ m, life: 0 });
  }
  function updateTracers(dt) {
    for (let i = tracers.length - 1; i >= 0; i--) { const t = tracers[i]; t.life += dt; if (t.life > (t.m.scale.x > 0.03 ? 0.14 : 0.07)) { DYN.remove(t.m); t.m.material.dispose(); FINE_MESHES.splice(FINE_MESHES.indexOf(t.m), 1); tracers.splice(i, 1); } }
  }

  // playing cards for Lucky Streak
  function cardTexture(suit, red) {
    const c = document.createElement('canvas'); c.width = 64; c.height = 88; const x = c.getContext('2d');
    x.fillStyle = '#fbf6ea'; x.fillRect(0, 0, 64, 88); x.strokeStyle = '#2a2024'; x.lineWidth = 5; x.strokeRect(2.5, 2.5, 59, 83);
    x.fillStyle = red ? '#d6463a' : '#2a2024'; x.font = 'bold 44px serif'; x.textAlign = 'center'; x.textBaseline = 'middle'; x.fillText(suit, 32, 48);
    x.font = 'bold 15px serif'; x.fillText('A', 12, 14);
    const t = new THREE.CanvasTexture(c); t.colorSpace = THREE.SRGBColorSpace; return t;
  }
  const CARD_MATS = [['♠', false], ['♥', true], ['♣', false], ['♦', true]].map(([s, r]) => new THREE.MeshBasicMaterial({ map: cardTexture(s, r), side: THREE.DoubleSide, fog: true }));
  const cardGeo = new THREE.PlaneGeometry(0.2, 0.28);
  const cards = [];
  function card(at, vel) {
    const m = new THREE.Mesh(cardGeo, CARD_MATS[Math.floor(Math.random() * 4)]); m.position.copy(at); DYN.add(m);
    cards.push({ m, vel: vel.clone(), spin: V3(rand(-7, 7), rand(-7, 7), rand(-7, 7)), life: 0, max: rand(0.9, 1.4) });
  }
  function updateCards(dt) {
    for (let i = cards.length - 1; i >= 0; i--) {
      const c = cards[i]; c.life += dt; const k = c.life / c.max;
      if (k >= 1) { DYN.remove(c.m); cards.splice(i, 1); continue; }
      c.vel.y -= 3 * dt; c.m.position.addScaledVector(c.vel, dt);
      c.m.rotation.x += c.spin.x * dt; c.m.rotation.y += c.spin.y * dt; c.m.rotation.z += c.spin.z * dt;
      c.m.scale.setScalar(k > 0.75 ? (1 - k) / 0.25 : 1);
    }
  }

  // ---------------------------------------------------------------- obstacles: the buildings and big props round the square
  const SOLID = /^(FalseFront_|Cottage|LogCabin|Farmhouse|Outhouse|Well|TownMemorial|NoticeBoard|GunRack|Cart|WaterTrough|Barrel_|Crate_|LampPost|Barn|Windmill|HitchRail|Depot)/;
  const BOXES = [];
  function buildObstacles(S) {
    for (const it of S.items) {
      const name = String(it.model);
      const x = toX(it.Y), z = toZ(it.X);
      if (Math.hypot(x, z) > 75) continue;
      if (name === 'k:stall' || name.startsWith('k:shed') || name === 'k:barricade') {
        const r = (KIT_RADIUS[name.slice(2).split(':')[0]] || 120) / 100 * 0.8;
        BOXES.push({ x, z, c: 1, s: 0, hx: r, hz: r, h: 2.6 }); continue;
      }
      if (!SOLID.test(name)) continue;
      const b = modelBounds(name); if (!b) continue;
      const sc = Array.isArray(it.scale) ? it.scale : [it.scale || 1, it.scale || 1, it.scale || 1];
      const th = it.rotY !== undefined ? it.rotY : rotY(it.yaw || 0), c = Math.cos(th), s = Math.sin(th);
      const cx = (b.min[0] + b.max[0]) / 2 * sc[0], cz = (b.min[2] + b.max[2]) / 2 * sc[2];
      // the box's centre in the world: its local offset turned by the item's rotation about +y
      const wx = x + c * cx + s * cz, wz = z - s * cx + c * cz;
      const hx = (b.max[0] - b.min[0]) / 2 * sc[0], hz = (b.max[2] - b.min[2]) / 2 * sc[2];
      if (hx < 0.12 && hz < 0.12) continue;
      BOXES.push({ x: wx, z: wz, c, s, hx: Math.max(hx, 0.15), hz: Math.max(hz, 0.15), h: b.max[1] * sc[1] });
    }
  }
  // Pushes a circle (x, z, r) out of every box; returns true when it moved.
  function collide(p, r) {
    let moved = false;
    for (const b of BOXES) {
      const dx = p.x - b.x, dz = p.z - b.z;
      if (Math.abs(dx) > b.hx + b.hz + r + 1 && Math.abs(dz) > b.hx + b.hz + r + 1) continue;
      const lx = b.c * dx - b.s * dz, lz = b.s * dx + b.c * dz;   // into the box's frame (inverse rotation)
      const qx = clamp(lx, -b.hx, b.hx), qz = clamp(lz, -b.hz, b.hz);
      let ex = lx - qx, ez = lz - qz, d = Math.hypot(ex, ez);
      if (d >= r) continue;
      if (d < 1e-5) { // inside: leave by the nearest face
        let nlx = lx, nlz = lz;
        if (b.hx - Math.abs(lx) < b.hz - Math.abs(lz)) nlx = (Math.sign(lx) || 1) * (b.hx + r); else nlz = (Math.sign(lz) || 1) * (b.hz + r);
        p.x = b.x + b.c * nlx + b.s * nlz; p.z = b.z - b.s * nlx + b.c * nlz; moved = true; continue;
      }
      const push = (r - d) / d, nlx = lx + ex * push, nlz = lz + ez * push;
      p.x = b.x + b.c * nlx + b.s * nlz; p.z = b.z - b.s * nlx + b.c * nlz; moved = true;
    }
    return moved;
  }
  // The nearest distance along a ray (origin o, unit dir d, horizontal only for the boxes) to a box face, under its height.
  function rayBoxes(o, d, maxT) {
    let best = maxT;
    for (const b of BOXES) {
      const dx = o.x - b.x, dz = o.z - b.z;
      const lox = b.c * dx - b.s * dz, loz = b.s * dx + b.c * dz, ldx = b.c * d.x - b.s * d.z, ldz = b.s * d.x + b.c * d.z;
      let t0 = 0, t1 = best;
      for (const [lo, ld, h] of [[lox, ldx, b.hx], [loz, ldz, b.hz]]) {
        if (Math.abs(ld) < 1e-8) { if (lo < -h || lo > h) { t0 = 1; t1 = 0; } continue; }
        let a = (-h - lo) / ld, c2 = (h - lo) / ld; if (a > c2) [a, c2] = [c2, a];
        t0 = Math.max(t0, a); t1 = Math.min(t1, c2);
      }
      if (t0 <= t1 && t0 < best) { const y = o.y + d.y * t0, g = gy(o.x + d.x * t0, o.z + d.z * t0) ?? 0; if (y < g + b.h) best = t0; }
    }
    return best;
  }
  function rayGround(o, d, maxT) {
    for (let s = 0.3; s < maxT; s += 0.25 + s * 0.01) { const x = o.x + d.x * s, z = o.z + d.z * s, g = gy(x, z); if (g !== null && o.y + d.y * s <= g) return s; }
    return maxT;
  }

  // ---------------------------------------------------------------- spiders
  const SPIDER_N = 16;
  let spiderMesh = null, xrayMesh = null, shadowMesh = null;
  const spiders = [];
  const BODY = 0.55;   // the game's spider model is 3 m across; this raid is younger
  function makeSpiderMeshes(geo) {
    spiderMesh = new THREE.InstancedMesh(geo, MAT, SPIDER_N); spiderMesh.frustumCulled = false; spiderMesh.count = 0;
    for (let i = 0; i < SPIDER_N; i++) spiderMesh.setColorAt(i, C('#ffffff'));
    DYN.add(spiderMesh);
    const xm = new THREE.MeshBasicMaterial({ color: C('#ff9f1c'), transparent: true, opacity: 0.8, depthWrite: false, depthFunc: THREE.GreaterDepth, fog: false });
    xrayMesh = new THREE.InstancedMesh(geo, xm, SPIDER_N); xrayMesh.frustumCulled = false; xrayMesh.count = 0; xrayMesh.renderOrder = 8;
    DYN.add(xrayMesh); FINE_MESHES.push(xrayMesh);
    const sg = new THREE.CircleGeometry(1, 20); sg.rotateX(-Math.PI / 2);
    const sm = new THREE.MeshBasicMaterial({ color: C('#2a2024'), transparent: true, opacity: 0.2, depthWrite: false, fog: true });
    sm.polygonOffset = true; sm.polygonOffsetFactor = -2; sm.polygonOffsetUnits = -4;
    shadowMesh = new THREE.InstancedMesh(sg, sm, SPIDER_N); shadowMesh.frustumCulled = false; shadowMesh.count = 0;
    DYN.add(shadowMesh); FINE_MESHES.push(shadowMesh);
  }
  function addSpider(x, z, o = {}) {
    if (spiders.length >= SPIDER_N) return null;
    const g = gy(x, z); if (g === null) return null;
    const sc = o.scale ?? BODY * rand(0.9, 1.12);
    const s = { pos: V3(x, g, z), vel: V3(), face: rand(0, 6.28), scale: sc, hp: 100, max: 100, state: 'walk', t: 0, atkT: rand(0.4, 1.2), lunge: 0,
      flash: 0, burn: 0, burnTick: 0, stun: 0, vy: 0, air: false, spinX: 0, markUntil: -1, tagUntil: -1, deadT: 0, phase: rand(0, 6), level: Math.floor(rand(11, 14)),
      hold: !!o.hold, speed: rand(2.8, 3.6) };
    spiders.push(s); return s;
  }
  const spiderCenter = (s) => V3(s.pos.x, s.pos.y + 0.5 * s.scale, s.pos.z);
  const spiderRadius = (s) => 0.8 * s.scale;
  const alive = () => spiders.filter((s) => s.state !== 'dead');
  function damage(s, amount, o = {}) {
    if (s.state === 'dead') return 0;
    let dmg = amount * (G.t < s.markUntil ? 1.3 : 1);
    if (o.crit) dmg *= 2;
    dmg = Math.round(dmg * rand(0.92, 1.08));
    s.hp -= dmg; s.flash = 0.12; s.tagUntil = G.t + 4;
    number(spiderCenter(s).add(V3(rand(-0.3, 0.3), 0.3, rand(-0.3, 0.3))), dmg, o.crit ? 'crit' : o.kind || (G.t < s.markUntil ? 'mark' : ''));
    if (s.hp <= 0) kill(s);
    return dmg;
  }
  function kill(s) {
    s.state = 'dead'; s.deadT = 0; s.hp = 0; s.air = false;
    burst('blob', s.pos.clone().add(V3(0, 0.3, 0)), 8, { col: '#d9cbb3', grav: -0.4, drag: 2.5, v0: 0.6, v1: 1.8, sa: 0.12, sb: 0.26, la: 0.5, lb: 0.9, up: true });
    G.kills++; GUN.reserve = Math.min(360, GUN.reserve + 9);
    gainXP(64);
  }
  function updateSpiders(dt) {
    const target = (s) => {
      if (ghost.on && ghost.pos.distanceTo(s.pos) < 16) return ghost.pos;
      return P.pos;
    };
    for (let i = spiders.length - 1; i >= 0; i--) {
      const s = spiders[i]; s.t += dt; s.flash = Math.max(0, s.flash - dt);
      if (s.state === 'dead') { s.deadT += dt; if (s.deadT > 1.6) spiders.splice(i, 1); continue; }
      // burning in slag
      if (s.burn > 0) { s.burn -= dt; s.burnTick -= dt; if (s.burnTick <= 0) { s.burnTick = 0.5; damage(s, 7, { kind: 'burn' }); spawnPart('glow', s.pos.clone().add(V3(rand(-0.3, 0.3), 0.3, rand(-0.3, 0.3))), { col: '#ffb347', vel: V3(0, rand(1, 2), 0), s0: 0.08, life: 0.6 }); } }
      if (s.state === 'dead') continue;
      if (s.air) {
        s.vy -= 16 * dt; s.pos.addScaledVector(s.vel, dt); s.pos.y += s.vy * dt; s.spinX += dt * 9;
        const g = gy(s.pos.x, s.pos.z) ?? s.pos.y;
        if (s.pos.y <= g && s.vy < 0) { s.pos.y = g; s.air = false; s.vel.set(0, 0, 0); s.spinX = 0; if (s.stunNext) { s.stun = s.stunNext; s.stunNext = 0; }
          burst('blob', s.pos.clone(), 5, { col: '#e3d6bd', drag: 3, v0: 0.5, v1: 1.5, sa: 0.12, sb: 0.22, la: 0.4, lb: 0.7, up: true }); }
        collide(s.pos, 0.7 * s.scale);
        continue;
      }
      if (s.stun > 0) { s.stun -= dt; if (Math.random() < dt * 6) spawnPart('glow', s.pos.clone().add(V3(Math.cos(s.t * 6) * 0.4, 0.9, Math.sin(s.t * 6) * 0.4)), { col: '#ffe27a', vel: V3(0, 0.3, 0), s0: 0.05, life: 0.35 }); continue; }
      if (s.hold) continue;
      const tgt = target(s), to = V3(tgt.x - s.pos.x, 0, tgt.z - s.pos.z), d = to.length();
      if (s.lunge > 0) { s.lunge -= dt; if (s.lunge <= 0.18 && !s.bit) { s.bit = true; if (tgt === P.pos && d < 2.4) hurtPlayer(7); else if (tgt === ghost.pos) ghostHit(); } continue; }
      if (d > 1.9) {
        to.normalize();
        // separation from the others, a slight weave
        const sep = V3();
        for (const o of spiders) { if (o === s || o.state === 'dead') continue; const dx = s.pos.x - o.pos.x, dz = s.pos.z - o.pos.z, dd = Math.hypot(dx, dz); if (dd < 1.5 && dd > 1e-3) sep.add(V3(dx / dd, 0, dz / dd).multiplyScalar((1.5 - dd) * 1.6)); }
        const weave = rightOf(Math.atan2(-to.x, to.z)).multiplyScalar(Math.sin(s.t * 1.7 + s.phase) * 0.35);
        const want = to.clone().add(sep).add(weave).normalize().multiplyScalar(s.speed * (G.t < s.markUntil ? 0.9 : 1));
        s.vel.lerp(want, Math.min(1, dt * 5));
        s.pos.addScaledVector(s.vel, dt);
        collide(s.pos, 0.7 * s.scale);
        const g = gy(s.pos.x, s.pos.z); if (g !== null) s.pos.y = g;
      } else {
        s.vel.multiplyScalar(0.8);
        s.atkT -= dt;
        if (s.atkT <= 0) { s.atkT = rand(1.2, 1.6); s.lunge = 0.36; s.bit = false; }
      }
      if (s.vel.lengthSq() > 0.05 || d < 2.2) { const want = Math.atan2(d < 2.2 ? to.x : s.vel.x, d < 2.2 ? to.z : s.vel.z); let dd = want - s.face; while (dd > Math.PI) dd -= 2 * Math.PI; while (dd < -Math.PI) dd += 2 * Math.PI; s.face += dd * Math.min(1, dt * 8); }
    }
  }
  function drawSpiders() {
    if (!spiderMesh) return;
    let n = 0, nx = 0;
    for (const s of spiders) {
      const moving = !s.air && s.state !== 'dead' && s.stun <= 0 && s.vel.lengthSq() > 0.3;
      const bob = moving ? Math.abs(Math.sin(s.t * 15 + s.phase)) * 0.035 : 0;
      const lungeK = s.lunge > 0 ? Math.sin((1 - s.lunge / 0.36) * Math.PI) : 0;
      let pitch = -lungeK * 0.45, roll = moving ? Math.sin(s.t * 15 + s.phase) * 0.05 : 0, sc = s.scale, y = s.pos.y + bob + lungeK * 0.12;
      if (s.air) { pitch = s.spinX; }
      if (s.state === 'dead') { const k = clamp(s.deadT / 0.35, 0, 1); roll = Math.PI * ease(k); y += Math.sin(k * Math.PI) * 0.3 + 0.18 * k; sc *= s.deadT > 0.9 ? Math.max(0.001, 1 - (s.deadT - 0.9) / 0.6) : 1; }
      const fwd = V3(Math.sin(s.face), 0, Math.cos(s.face));
      tmpQuat.setFromEuler(tmpEul.set(pitch, s.face, roll, 'YXZ'));
      tmpVec.set(s.pos.x + fwd.x * lungeK * 0.35, y, s.pos.z + fwd.z * lungeK * 0.35); tmpScale.setScalar(sc);
      tmpMat.compose(tmpVec, tmpQuat, tmpScale);
      spiderMesh.setMatrixAt(n, tmpMat);
      const col = s.flash > 0 ? [2.3, 2.3, 2.3] : s.burn > 0 ? [1.35 + Math.sin(G.t * 18) * 0.15, 0.85, 0.6] : s.stun > 0 ? [0.8, 0.9, 1.15] : s.state === 'dead' ? [0.8, 0.8, 0.8] : [1, 1, 1];
      spiderMesh.setColorAt(n, tmpColor.setRGB(col[0], col[1], col[2]));
      if (G.t < s.markUntil && s.state !== 'dead') xrayMesh.setMatrixAt(nx++, tmpMat);
      tmpVec.set(s.pos.x, (gy(s.pos.x, s.pos.z) ?? s.pos.y) + 0.03, s.pos.z); tmpQuat.identity(); tmpScale.set(sc * 1.25, 1, sc * 1.25);
      shadowMesh.setMatrixAt(n, tmpMat.compose(tmpVec, tmpQuat, tmpScale));
      n++;
    }
    spiderMesh.count = n; spiderMesh.instanceMatrix.needsUpdate = true; if (spiderMesh.instanceColor) spiderMesh.instanceColor.needsUpdate = true;
    xrayMesh.count = nx; xrayMesh.instanceMatrix.needsUpdate = true;
    shadowMesh.count = n; shadowMesh.instanceMatrix.needsUpdate = true;
  }
  const tmpColor = new THREE.Color();
  // Spawns come from a ring round the player, out of sight where possible, on open ground.
  function spawnRaid(n) {
    for (let k = 0; k < n; k++) {
      for (let tries = 0; tries < 24; tries++) {
        const behind = tries < 16, a = P.yaw + Math.PI + (behind ? rand(-1.6, 1.6) : rand(-Math.PI, Math.PI)), r = rand(20, 30);
        const p = V3(P.pos.x - Math.sin(a) * r, 0, P.pos.z + Math.cos(a) * r);
        if (gy(p.x, p.z) === null || Math.hypot(p.x, p.z) > 62) continue;
        const q = p.clone(); if (collide(q, 1.2)) continue;
        addSpider(p.x, p.z); break;
      }
    }
  }

  // ---------------------------------------------------------------- the friendly Unpaid (Raise the Flock)
  function ghostGeometry() {
    const pts = [[0, 1.82], [0.1, 1.8], [0.17, 1.71], [0.19, 1.58], [0.15, 1.47], [0.24, 1.4], [0.3, 1.27], [0.29, 1.02], [0.25, 0.78], [0.19, 0.55], [0.11, 0.33], [0.05, 0.16], [0, 0.06]]
      .map(([r, y]) => new THREE.Vector2(r, y));
    const shroud = paint(new THREE.LatheGeometry(pts, 14), '#e7edf0');
    const hood = paint(T(ico(0.135, 1, 1, 0.9, 0.55), 0, 1.6, 0.11), '#26343f');
    const eyeL = paint(T(ico(0.024, 0), -0.045, 1.61, 0.17), '#6ee7ff', 0), eyeR = paint(T(ico(0.024, 0), 0.045, 1.61, 0.17), '#6ee7ff', 0);
    const coal = paint(T(ico(0.065, 1), 0, 1.16, 0.25), '#6ee7ff', 0);
    const armL = paint(T(cyl(0.06, 0.025, 0.62, 6).rotateX(-1.25), -0.24, 1.2, 0.27), '#dfe6ea');
    const armR = paint(T(cyl(0.06, 0.025, 0.62, 6).rotateX(-1.25), 0.24, 1.2, 0.27), '#dfe6ea');
    return merged([shroud, hood, eyeL, eyeR, coal, armL, armR]);
  }
  const ghost = { on: false, mesh: null, pos: V3(), t: 0, until: 0, atk: 0, target: null };
  function ghostHit() { burst('glow', ghost.pos.clone().add(V3(0, 1.1, 0)), 4, { col: '#bff4ff', v0: 0.5, v1: 1.4, sa: 0.06, sb: 0.12, la: 0.3, lb: 0.5 }); }
  function updateGhost(dt) {
    if (!ghost.mesh) return;
    if (!ghost.on) { ghost.mesh.visible = false; return; }
    ghost.t += dt;
    const left = ghost.until - G.t;
    if (left <= 0) { ghost.on = false; burst('glow', ghost.pos.clone().add(V3(0, 1, 0)), 14, { col: '#bff4ff', v0: 0.5, v1: 2, sa: 0.06, sb: 0.14, la: 0.4, lb: 0.8 }); return; }
    // drift to the nearest spider and claw at it
    let best = null, bd = 1e9; for (const s of alive()) { const d = s.pos.distanceTo(ghost.pos); if (d < bd) { bd = d; best = s; } }
    if (best && bd > 1.6) { const to = best.pos.clone().sub(ghost.pos).setY(0).normalize(); ghost.pos.addScaledVector(to, Math.min(bd - 1.4, 5.5 * dt)); ghost.face = Math.atan2(to.x, to.z); }
    if (best && bd <= 1.9) { ghost.face = Math.atan2(best.pos.x - ghost.pos.x, best.pos.z - ghost.pos.z); ghost.atk -= dt; if (ghost.atk <= 0) { ghost.atk = 0.7; damage(best, 17, { kind: '' });
      const f = V3(Math.sin(ghost.face), 0, Math.cos(ghost.face)), at = ghost.pos.clone().add(V3(0, 1.0, 0)).addScaledVector(f, 0.8);
      for (let i = 0; i < 9; i++) { const a = -0.9 + i * 0.22; const d = V3(Math.sin(ghost.face + a), 0.15 - i * 0.04, Math.cos(ghost.face + a)); spawnPart('glow', at.clone().addScaledVector(d, 0.5), { col: '#9ff0ff', vel: d.multiplyScalar(2.2), s0: 0.09, s1: 0.03, life: 0.28 }); } } }
    const g = gy(ghost.pos.x, ghost.pos.z) ?? ghost.pos.y, rise = clamp(ghost.t / 0.6, 0, 1), sink = clamp(left / 0.6, 0, 1);
    ghost.mesh.visible = true;
    ghost.mesh.position.set(ghost.pos.x, g + lerp(-1.4, 0.32, ease(rise)) - (1 - sink) * 1.4 + Math.sin(ghost.t * 2.2) * 0.06, ghost.pos.z);
    ghost.mesh.rotation.set(Math.sin(ghost.t * 1.3) * 0.05, ghost.face || 0, Math.sin(ghost.t * 1.7) * 0.05);
    ghost.mesh.scale.setScalar(lerp(0.3, 1, ease(rise)));
    if (Math.random() < dt * 14) spawnPart('blob', ghost.mesh.position.clone().add(V3(rand(-0.2, 0.2), 0.2, rand(-0.2, 0.2))), { col: '#e7edf0', vel: V3(rand(-0.2, 0.2), -0.2, rand(-0.2, 0.2)), s0: 0.16, s1: 0.04, life: 0.7, drag: 1 });
  }

  // ---------------------------------------------------------------- slag pool and bomb
  function poolGeometry() {
    const rnd = mulberry32(5), parts2 = [];
    parts2.push(paint(T(cyl(1, 1, 0.05, 28), 0, 0.02, 0), '#ff8a1c', 0));
    for (let i = 0; i < 9; i++) { const a = rnd() * 6.28, r = rnd() * 0.75; parts2.push(paint(T(ico(0.22 + rnd() * 0.18, 1, 1, 0.22, 1), Math.cos(a) * r, 0.05, Math.sin(a) * r), '#ffd04a', 0)); }
    for (let i = 0; i < 7; i++) { const a = rnd() * 6.28, r = 0.3 + rnd() * 0.5; parts2.push(paint(T(lumpy(ico(0.07 + rnd() * 0.06, 1, 1, 0.45, 1), 0.2, i), Math.cos(a) * r, 0.06, Math.sin(a) * r), '#7a3418')); }
    for (let i = 0; i < 14; i++) { const a = i / 14 * 6.28, r = 0.98; parts2.push(paint(T(ico(0.12, 0, 1, 0.5, 1), Math.cos(a) * r, 0.03, Math.sin(a) * r), '#e2650f', 0)); }
    return merged(parts2);
  }
  const slag = { pools: [], bombs: [], geo: null };
  function throwBomb(target) {
    const m = new THREE.Mesh(merged([paint(ico(0.16, 1), '#3a3034'), paint(T(new THREE.TorusGeometry(0.16, 0.03, 6, 14).rotateX(Math.PI / 2), 0, 0, 0), '#ff8a1c', 0)]), MAT);
    const from = muzzleWorld().add(V3(0, 0.05, 0));
    DYN.add(m); slag.bombs.push({ m, from, to: target.clone(), t: 0, dur: clamp(from.distanceTo(target) / 24, 0.45, 0.9) });
  }
  function updateSlag(dt) {
    for (let i = slag.bombs.length - 1; i >= 0; i--) {
      const b = slag.bombs[i]; b.t += dt; const k = clamp(b.t / b.dur, 0, 1);
      const p = b.from.clone().lerp(b.to, k); p.y += Math.sin(k * Math.PI) * Math.max(1.6, b.from.distanceTo(b.to) * 0.28);
      b.m.position.copy(p); b.m.rotation.x += dt * 9; b.m.rotation.z += dt * 5;
      spawnPart('glow', p.clone(), { col: Math.random() < 0.5 ? '#ffd04a' : '#ff8a1c', vel: V3(rand(-0.5, 0.5), rand(0, 0.8), rand(-0.5, 0.5)), s0: rand(0.06, 0.12), s1: 0.02, life: 0.35 });
      if (k >= 1) { DYN.remove(b.m); slag.bombs.splice(i, 1); explode(b.to); }
    }
    for (let i = slag.pools.length - 1; i >= 0; i--) {
      const pl = slag.pools[i]; pl.t += dt; const left = pl.until - G.t;
      if (left <= 0) { DYN.remove(pl.m); slag.pools.splice(i, 1); continue; }
      const k = Math.min(ease(pl.t / 0.3), clamp(left / 0.5, 0, 1));
      pl.m.scale.set(pl.r * k * (1 + Math.sin(pl.t * 3) * 0.015), 1, pl.r * k * (1 + Math.cos(pl.t * 2.6) * 0.015));
      if (Math.random() < dt * 22) { const a = rand(0, 6.28), r = Math.sqrt(Math.random()) * pl.r * 0.9; spawnPart('glow', pl.at.clone().add(V3(Math.cos(a) * r, 0.1, Math.sin(a) * r)), { col: Math.random() < 0.6 ? '#ffd04a' : '#ff8a1c', vel: V3(rand(-0.2, 0.2), rand(1.2, 2.6), rand(-0.2, 0.2)), s0: rand(0.05, 0.1), s1: 0.02, life: rand(0.5, 1.0), drag: 0.5 }); }
      if (Math.random() < dt * 3) { const a = rand(0, 6.28), r = Math.sqrt(Math.random()) * pl.r * 0.8; spawnPart('blob', pl.at.clone().add(V3(Math.cos(a) * r, 0.3, Math.sin(a) * r)), { col: '#e6ddd0', vel: V3(rand(-0.2, 0.2), rand(0.8, 1.4), rand(-0.2, 0.2)), s0: 0.12, s1: 0.32, life: 1.4, drag: 0.8, hold: 0.6 }); }
      for (const s of alive()) if (Math.hypot(s.pos.x - pl.at.x, s.pos.z - pl.at.z) < pl.r) { if (s.burn <= 0.05) s.burnTick = 0; s.burn = Math.max(s.burn, 0.6); }
    }
  }
  function explode(at) {
    const g = gy(at.x, at.z) ?? at.y; at.y = g;
    spawnPart('glow', at.clone().add(V3(0, 0.4, 0)), { col: '#fff1b0', s0: 0.5, s1: 3.2, life: 0.32, hold: 0.45 });
    spawnPart('glow', at.clone().add(V3(0, 0.3, 0)), { col: '#ff9f1c', s0: 0.6, s1: 2.6, life: 0.42, hold: 0.5 });
    ring(at.clone().add(V3(0, 0.08, 0)), { col: '#ffb347', r0: 0.5, r1: 6, w0: 0.6, w1: 0.08, life: 0.45, op: 0.9 });
    burst('chunk', at.clone().add(V3(0, 0.3, 0)), 14, { col: '#3a2b26', grav: 14, v0: 3, v1: 7, sa: 0.06, sb: 0.14, la: 0.6, lb: 1.1, up: true, floor: true, spin: 9 });
    burst('glow', at.clone().add(V3(0, 0.3, 0)), 22, { col: '#ffb347', grav: 9, v0: 3, v1: 8, sa: 0.05, sb: 0.11, la: 0.4, lb: 0.9, up: true });
    burst('blob', at.clone().add(V3(0, 0.6, 0)), 9, { col: '#ddd3c4', grav: -0.6, drag: 1.6, v0: 0.8, v1: 2.2, sa: 0.25, sb: 0.4, la: 1.0, lb: 1.6, up: true, hold: 0.6 });
    const m = new THREE.Mesh(slag.geo, MAT); m.position.copy(at).add(V3(0, 0.02, 0)); m.scale.set(0.01, 1, 0.01); DYN.add(m);
    slag.pools.push({ m, at: at.clone(), r: 4.5, t: 0, until: G.t + EMBER.slag.dur });
    for (const s of alive()) { const d = Math.hypot(s.pos.x - at.x, s.pos.z - at.z); if (d < 4.5) { damage(s, d < 2.5 ? 34 : 18, { kind: 'burn' }); s.burn = 0.6; s.burnTick = 0.3; } }
    G.shake = Math.max(G.shake, 0.18 * clamp(1 - at.distanceTo(P.pos) / 30, 0.2, 1));
  }

  // ---------------------------------------------------------------- the gun in hand
  const gunRig = new THREE.Group(); camera.add(gunRig);
  let gunModel = null, gunMag = null;
  const MUZZLE_LOCAL = V3(0, 0, 0.691);
  async function buildGun() {
    const names = ['BullpupBody_Standard', 'BullpupBarrel_Carbine', 'BullpupMuzzle_Brake', 'BullpupMagazine_30', 'BullpupSight_RedDot', 'BullpupStock_Pad'];
    const geos = await Promise.all(names.map((n) => geometry(n)));
    if (geos.some((g) => !g)) return;
    const muzzle = geos[2].clone(); muzzle.computeBoundingBox();
    const bb = muzzle.boundingBox; muzzle.translate(-(bb.min.x + bb.max.x) / 2, -(bb.min.y + bb.max.y) / 2, -bb.min.z + 0.639);
    gunModel = new THREE.Group();
    const gunMat = printMaterial(); gunMat.color.setRGB(0.5, 0.52, 0.6);   // the export's flat cream, toned to gunmetal
    [geos[0], geos[1], muzzle, geos[4], geos[5]].forEach((g) => { const m = new THREE.Mesh(g, gunMat); gunModel.add(m); });
    gunMag = new THREE.Mesh(geos[3], gunMat); gunModel.add(gunMag);
    gunModel.rotation.y = Math.PI + 0.05;   // the gun's +z (toward the muzzle) to the camera's -z, aimed a touch inward
    gunRig.add(gunModel);
  }
  const GUN_REST = V3(0.2, -0.215, -0.3);
  const muzzleWorld = () => (gunModel ? gunModel.localToWorld(MUZZLE_LOCAL.clone()) : camera.localToWorld(V3(0.15, -0.1, -0.8)));
  let flashMesh = null;
  function makeFlash() {
    const g = []; for (let i = 0; i < 5; i++) { const a = i / 5 * 6.283; g.push(paint(T(box(0.025, 0.11, 0.02), Math.cos(a) * 0.05, Math.sin(a) * 0.05, 0).rotateZ(a - Math.PI / 2), '#fff3c4', 0)); }
    g.push(paint(ico(0.045, 0), '#ffd04a', 0));
    flashMesh = new THREE.Mesh(merged(g), MAT); flashMesh.visible = false; gunRig.add(flashMesh);
  }
  function updateGun(dt, moving, sprint) {
    const t = G.t;
    P.recoil = Math.max(0, P.recoil - dt * 5);
    const reloadK = GUN.reloadT > 0 ? 1 - GUN.reloadT / GUN.reloadDur : 0, dip = GUN.reloadT > 0 ? Math.sin(reloadK * Math.PI) : 0;
    const bobX = moving ? Math.sin(P.bob) * 0.012 : 0, bobY = moving ? Math.abs(Math.cos(P.bob)) * 0.012 : Math.sin(t * 1.6) * 0.002;
    const sprintK = sprint && moving ? 1 : 0;
    gunRig.position.set(GUN_REST.x + bobX - sprintK * 0.03, GUN_REST.y + bobY - dip * 0.06 - sprintK * 0.03 - G.slamDip * 0.4, GUN_REST.z + P.recoil * 0.05);
    gunRig.rotation.set(dip * 0.55 + P.recoil * 0.12 - sprintK * 0.25, sprintK * 0.5 + G.sway * 0.4, -dip * 0.3 + sprintK * 0.15);
    if (gunMag) gunMag.position.y = GUN.reloadT > 0 ? -Math.sin(clamp(reloadK * 1.6, 0, 1) * Math.PI) * 0.09 : 0;
    if (flashMesh) { flashMesh.visible = GUN.flash > 0; GUN.flash = Math.max(0, (GUN.flash || 0) - dt); if (gunModel) { flashMesh.position.copy(gunModel.localToWorld(MUZZLE_LOCAL.clone())); gunRig.worldToLocal(flashMesh.position); flashMesh.rotation.z = Math.random() * 6; } }
  }

  // ---------------------------------------------------------------- shooting
  const aimDir = () => V3(-Math.sin(P.yaw) * Math.cos(P.pitch), Math.sin(P.pitch), Math.cos(P.yaw) * Math.cos(P.pitch));
  function eyePos() { return V3(P.pos.x, P.pos.y + P.eye - G.slamDip, P.pos.z); }
  function raySpider(o, d, maxT, skip) {
    let best = null, bt = maxT;
    for (const s of spiders) {
      if (s.state === 'dead' || s === skip) continue;
      const c = spiderCenter(s), r = spiderRadius(s);
      const oc = c.clone().sub(o), t = oc.dot(d); if (t < 0) continue;
      const d2 = oc.lengthSq() - t * t; if (d2 > r * r) continue;
      const th = t - Math.sqrt(r * r - d2); if (th < bt) { bt = th; best = s; }
    }
    return { s: best, t: bt };
  }
  function fire() {
    if (GUN.reloadT > 0) return;
    if (GUN.mag <= 0) { startReload(); return; }
    GUN.mag--; GUN.flash = 0.04; P.recoil = Math.min(1, P.recoil + 0.35);
    const moving = moveInput().l > 0.1, spread = (moving ? 1.6 : 0.55) * DEG;
    const o = eyePos(), d = aimDir();
    d.applyAxisAngle(V3(0, 1, 0), rand(-spread, spread)); d.y += rand(-spread, spread); d.normalize();
    const lucky = G.t < GUN.luckUntil;
    const hitS = raySpider(o, d, 140), tBox = rayBoxes(o, d, 140), tGround = rayGround(o, d, 140);
    const tEnd = Math.min(hitS.t, tBox, tGround), end = o.clone().addScaledVector(d, tEnd);
    tracer(muzzleWorld(), end, lucky ? '#ffe08a' : '#fff4cf');
    P.pitch = clamp(P.pitch + 0.0045, -1.3, 1.3);
    pulse('xh', 'kick');
    if (hitS.s && hitS.t <= tEnd + 1e-3) {
      const crit = Math.random() < (lucky ? 0.34 : 0.1);
      damage(hitS.s, 14, { crit }); pulse('hm', 'on');
      burst('glow', end, 4, { col: crit ? '#ffe08a' : '#fff4cf', v0: 0.6, v1: 2, sa: 0.04, sb: 0.08, la: 0.12, lb: 0.25 });
      burst('blob', end, 3, { col: '#b9ad97', drag: 3, v0: 0.4, v1: 1.4, sa: 0.06, sb: 0.12, la: 0.3, lb: 0.5 });
      hitS.s.vel.addScaledVector(d.clone().setY(0), 0.6);
      if (lucky) {
        let next = null, nd = 12; for (const s of alive()) { if (s === hitS.s) continue; const dd = s.pos.distanceTo(hitS.s.pos); if (dd < nd) { nd = dd; next = s; } }
        card(end.clone(), V3(rand(-1.5, 1.5), rand(2, 3.5), rand(-1.5, 1.5)));
        if (next) { const c = spiderCenter(next); tracer(end, c, '#ffcf4a', 0.045); damage(next, 10, { crit: Math.random() < 0.34 }); burst('glow', c, 5, { col: '#ffcf4a', v0: 0.6, v1: 2, sa: 0.05, sb: 0.1, la: 0.15, lb: 0.3 }); }
      }
    } else if (tEnd < 140) {
      burst('blob', end, 4, { col: '#d8ccb6', grav: 1, drag: 2.5, v0: 0.4, v1: 1.5, sa: 0.06, sb: 0.14, la: 0.3, lb: 0.6, up: true });
      burst('chunk', end, 2, { col: '#6d625a', grav: 9, v0: 1, v1: 2.5, sa: 0.025, sb: 0.05, la: 0.3, lb: 0.5, up: true });
    }
    if (GUN.mag === 0) startReload();
  }
  function startReload() { if (GUN.reloadT > 0 || GUN.mag >= GUN.cap || GUN.reserve <= 0) return; GUN.reloadT = GUN.reloadDur; }

  // ---------------------------------------------------------------- the player
  function moveInput() {
    let mx = touch.stick.x, mz = touch.stick.y;
    if (keys.has('KeyW') || keys.has('ArrowUp')) mz += 1;
    if (keys.has('KeyS') || keys.has('ArrowDown')) mz -= 1;
    if (keys.has('KeyD')) mx += 1;
    if (keys.has('KeyA')) mx -= 1;
    const l = Math.hypot(mx, mz); if (l > 1) { mx /= l; mz /= l; }
    return { mx, mz, l: Math.min(1, l), run: keys.has('ShiftLeft') || keys.has('ShiftRight') };
  }
  function hurtPlayer(n) {
    if (P.dash || G.mode !== 'play') { if (G.mode === 'watch') pulse('vig', 'hit'); return; }
    P.health = Math.max(0, P.health - n); P.hurtT = G.t;
    pulse('vig', 'hit'); pulse('face', 'flinch'); pulse('hurtfx', 'flinch');
    G.hurtEyes = G.t + 0.6;
    if (P.health <= 0) { G.deadT = 2.2; $('#fade').style.opacity = '0.9'; showCallout('YOU WAKE IN THE NEAREST GRAVE', '#ffd9d3'); }
  }
  function updatePlayer(dt) {
    if (G.deadT > 0) { G.deadT -= dt; if (G.deadT <= 0) { P.health = P.max; $('#fade').style.opacity = '0'; placePlayer(START); for (const s of spiders) if (s.pos.distanceTo(P.pos) < 9) s.pos.addScaledVector(s.pos.clone().sub(P.pos).setY(0).normalize(), 6); } return; }
    if (G.t - P.hurtT > 3 && P.health < P.max) P.health = Math.min(P.max, P.health + 7 * dt);
    if (P.dash) {
      const d = P.dash; d.t += dt; const k = clamp(d.t / d.dur, 0, 1), e = 1 - Math.pow(1 - k, 2);
      const nx = lerp(d.from.x, d.to.x, e), nz = lerp(d.from.z, d.to.z, e);
      const q = V3(nx, 0, nz); collide(q, 0.4); if (gy(q.x, q.z) !== null) { P.pos.x = q.x; P.pos.z = q.z; }
      // fling whatever stands on the path
      for (const s of alive()) {
        if (d.hit.has(s)) continue;
        const ds = Math.hypot(s.pos.x - P.pos.x, s.pos.z - P.pos.z);
        if (ds < 2.6) {
          d.hit.add(s); const along = d.dir.clone(), perp = V3(-along.z, 0, along.x), rel = V3(s.pos.x - P.pos.x, 0, s.pos.z - P.pos.z);
          const lat = perp.multiplyScalar(Math.sign(rel.dot(perp)) || (Math.random() < 0.5 ? -1 : 1));
          s.vel.copy(lat.multiplyScalar(rand(4, 4.8)).addScaledVector(along, -0.8)); s.vy = 5.5; s.air = true; s.stunNext = 1.0; damage(s, 15);
          burst('blob', s.pos.clone().add(V3(0, 0.3, 0)), 5, { col: '#e8dcc4', drag: 2.5, v0: 1, v1: 2.5, sa: 0.15, sb: 0.3, la: 0.4, lb: 0.7, up: true });
        }
      }
      if (Math.random() < 0.9) for (let i = 0; i < 2; i++) { const side = rightOf(Math.atan2(-d.dir.x, d.dir.z)).multiplyScalar(rand(-1.2, 1.2)); spawnPart('streak', P.pos.clone().add(side).add(V3(0, rand(0.3, 2.0), 0)), { dir: d.dir.clone(), len: rand(1.2, 2.6), s0: 1, life: 0.3, vel: d.dir.clone().multiplyScalar(-2), col: '#fffaf0' }); }
      if (k >= 1) { P.dash = null; dustRing(P.pos, 0.75); G.slowT = 0.4; }
    } else {
      const { mx, mz, l, run } = moveInput(), speed = run && mz > 0 ? 7.2 : 4.6;
      const want = fwdOf(P.yaw).multiplyScalar(mz).add(rightOf(P.yaw).multiplyScalar(mx)).multiplyScalar(speed);
      P.vel.lerp(want, Math.min(1, dt * 12));
      if (l > 0.05) P.bob += dt * (run ? 11 : 8);
      const q = P.pos.clone().addScaledVector(P.vel, dt);
      collide(q, 0.4);
      if (gy(q.x, q.z) !== null && Math.hypot(q.x, q.z) < 70) { P.pos.x = q.x; P.pos.z = q.z; }
    }
    const g = gy(P.pos.x, P.pos.z); if (g !== null) P.pos.y = lerp(P.pos.y, g, Math.min(1, dt * 14));
  }
  function dustRing(at, scale) {
    for (let i = 0; i < 14; i++) { const a = i / 14 * 6.283, d = V3(Math.cos(a), 0, Math.sin(a)); spawnPart('blob', at.clone().add(d.clone().multiplyScalar(1.0)).add(V3(0, 0.2, 0)), { col: '#e9dec7', vel: d.multiplyScalar(rand(2, 3.5) * scale).add(V3(0, rand(0.3, 1), 0)), drag: 3, s0: rand(0.18, 0.3) * scale, s1: rand(0.35, 0.55) * scale, life: rand(0.5, 0.8), hold: 0.5 }); }
  }
  function placePlayer(at) { P.pos.set(at.x, gy(at.x, at.z) ?? 0, at.z); P.yaw = at.yaw; P.pitch = at.pitch ?? -0.04; P.vel.set(0, 0, 0); }

  // ---------------------------------------------------------------- ember powers
  function groundAim(maxT = 26) {
    const o = eyePos(), d = aimDir();
    const t = Math.min(rayGround(o, d, maxT), rayBoxes(o, d, maxT)), hit = raySpider(o, d, maxT);
    if (hit.s && hit.t <= t) return hit.s.pos.clone();
    const p = o.clone().addScaledVector(d, Math.min(t, maxT));
    if (t >= maxT) { const g = gy(p.x, p.z); if (g !== null) p.y = g; }
    const q = p.clone(); collide(q, 0.3); q.y = gy(q.x, q.z) ?? p.y;
    return q;
  }
  function useEmber(slot) {
    if (G.paused || G.deadT > 0) return false;
    const id = G.slots[slot], e = EMBER[id];
    if (G.cd[slot] > 0) { pulse('sock' + slot, 'denied'); return false; }
    G.cd[slot] = e.cd; G.active[slot] = e.dur; G.usedFlash[slot] = 1; G.used.add(id);
    pulse('sock' + slot, 'used');
    showCallout(e.name.toUpperCase(), e.color);
    POWERS[id]();
    return true;
  }
  const POWERS = {
    luck() {
      GUN.luckUntil = G.t + EMBER.luck.dur;
      const f = fwdOf(P.yaw), r = rightOf(P.yaw);
      for (let i = 0; i < 12; i++) { const k = i / 11 - 0.5; card(eyePos().addScaledVector(f, 1.8).addScaledVector(r, k * 1.2).add(V3(0, -0.3, 0)), r.clone().multiplyScalar(k * 3.6).addScaledVector(f, 1.2).add(V3(0, rand(1.6, 2.6), 0))); }
      burst('glow', eyePos().add(fwdOf(P.yaw).multiplyScalar(1.2)), 10, { col: '#ffcf4a', v0: 1, v1: 2.5, sa: 0.05, sb: 0.1, la: 0.3, lb: 0.6 });
    },
    dash() {
      const { mx, mz, l } = moveInput();
      let dir = l > 0.1 ? fwdOf(P.yaw).multiplyScalar(mz).add(rightOf(P.yaw).multiplyScalar(mx)).normalize() : fwdOf(P.yaw);
      let to = P.pos.clone().addScaledVector(dir, 9);
      for (let s = 9; s > 1; s -= 0.5) { const q = P.pos.clone().addScaledVector(dir, s); if (gy(q.x, q.z) !== null && Math.hypot(q.x, q.z) < 70) { to = q; break; } }
      P.dash = { from: P.pos.clone(), to, dir, t: 0, dur: 0.3, hit: new Set() };
      G.fovKick = 1; dustRing(P.pos, 1); pulse('speed', 'on');
      for (let i = 0; i < 18; i++) { const a = i / 18 * 6.283; spawnPart('blob', P.pos.clone().add(V3(Math.cos(a) * 1.4, rand(0.05, 0.5), Math.sin(a) * 1.4)), { col: '#efe6d2', vel: V3(Math.cos(a) * 2.5, rand(0.5, 1.6), Math.sin(a) * 2.5).addScaledVector(dir, -2), drag: 2, s0: 0.12, s1: 0.26, life: 0.6, spin: 6 }); }
    },
    flock() {
      const at = groundAim(22);
      ghost.on = true; ghost.pos.copy(at); ghost.t = 0; ghost.until = G.t + EMBER.flock.dur; ghost.atk = 0.8; ghost.face = Math.atan2(P.pos.x - at.x, P.pos.z - at.z);
      ring(at.clone().add(V3(0, 0.06, 0)), { col: '#6ee7ff', r0: 0.2, r1: 2.4, w0: 0.5, w1: 0.1, life: 0.7, op: 0.9 });
      ring(at.clone().add(V3(0, 0.06, 0)), { col: '#bff4ff', r0: 0.1, r1: 1.2, w0: 0.3, w1: 0.05, life: 0.9, op: 0.8 });
      burst('glow', at.clone().add(V3(0, 0.3, 0)), 18, { col: '#9ff0ff', grav: -1, v0: 0.6, v1: 2, sa: 0.05, sb: 0.12, la: 0.6, lb: 1.1, up: true });
    },
    slag() { throwBomb(groundAim(20)); },
    sight() {
      ring(P.pos.clone().add(V3(0, 0.08, 0)), { col: '#5ac8ff', r0: 0.5, r1: 60, w0: 0.6, w1: 2.2, life: 1.0, op: 0.75 });
      pulse('lens', 'on');
      for (const s of alive()) { const d = s.pos.distanceTo(P.pos); if (d < 60) setTimeout(() => { if (s.state !== 'dead') { s.markUntil = G.t + EMBER.sight.dur; s.tagUntil = G.t + EMBER.sight.dur; } }, d / 60 * 1000); }
      G.sightUntil = G.t + EMBER.sight.dur;
    },
    slam() {
      G.slamDip = 0.45; G.shake = 0.4; G.slamT = 0;
      const at = P.pos.clone();
      ring(at.clone().add(V3(0, 0.07, 0)), { col: '#d8c8a8', r0: 0.4, r1: 7.5, w0: 0.9, w1: 0.15, life: 0.5, op: 0.95 });
      ring(at.clone().add(V3(0, 0.07, 0)), { col: '#fff6e0', r0: 0.2, r1: 4.5, w0: 0.4, w1: 0.05, life: 0.35, op: 0.8 });
      cracks(at);
      for (let i = 0; i < 26; i++) { const a = rand(0, 6.28), r = rand(2, 4.5); spawnPart('chunk', at.clone().add(V3(Math.cos(a) * r, 0.1, Math.sin(a) * r)), { col: ['#7d6e5c', '#6b5f52', '#8c7d68'][i % 3], vel: V3(Math.cos(a) * rand(1, 4), rand(4, 9), Math.sin(a) * rand(1, 4)), grav: 18, s0: rand(0.08, 0.18), life: rand(0.8, 1.4), floor: true, spin: 8, hold: 0.8 }); }
      for (let i = 0; i < 32; i++) { const a = i / 32 * 6.283; spawnPart('blob', at.clone().add(V3(Math.cos(a) * 2.2, 0.1, Math.sin(a) * 2.2)), { col: '#e3d6bd', vel: V3(Math.cos(a) * 6, rand(0.2, 0.6), Math.sin(a) * 6), drag: 2.4, s0: 0.12, s1: 0.3, life: rand(0.7, 1.0), hold: 0.55 }); }
      for (const s of alive()) { const v = s.pos.clone().sub(at).setY(0), d = v.length(); if (d < 7) { v.normalize(); s.air = true; s.vy = rand(6, 8.5); s.vel.copy(v.multiplyScalar(2.5 + (7 - d) * 0.5)); s.stunNext = 2.5; s.spinX = 0; damage(s, 28); } }
    },
  };
  const crackList = [];
  function cracks(at) {
    const g = gy(at.x, at.z) ?? at.y;
    for (let i = 0; i < 9; i++) {
      const a = i / 9 * 6.283 + rand(-0.2, 0.2), len = rand(2.2, 4.5);
      const geo = paint(box(0.12, 0.02, len), '#3b3034', 1); geo.translate(0, 0, len / 2);
      const m = new THREE.Mesh(geo, MAT_DECAL); m.position.set(at.x, g + 0.05, at.z); m.rotation.y = Math.atan2(Math.cos(a), Math.sin(a)); DYN.add(m);
      crackList.push({ m, life: 0 });
      const geo2 = paint(box(0.08, 0.02, len * 0.5), '#3b3034', 1); geo2.translate(0, 0, len * 0.25);
      const m2 = new THREE.Mesh(geo2, MAT_DECAL); m2.position.set(at.x + Math.cos(a) * len * 0.55, g + 0.05, at.z + Math.sin(a) * len * 0.55); m2.rotation.y = m.rotation.y + rand(0.5, 0.9) * (i % 2 ? 1 : -1); DYN.add(m2);
      crackList.push({ m: m2, life: 0 });
    }
  }
  function updateCracks(dt) {
    for (let i = crackList.length - 1; i >= 0; i--) { const c = crackList[i]; c.life += dt; const k = c.life; if (k > 2.6) { DYN.remove(c.m); crackList.splice(i, 1); continue; } c.m.scale.set(1, 1, Math.min(1, k / 0.12) * (k > 2 ? 1 - (k - 2) / 0.6 : 1)); }
  }

  // ---------------------------------------------------------------- the HUD
  const $id = (id) => document.getElementById(id);
  const hud = { last: {} };
  function setText(id, v) { if (hud.last[id] !== v) { hud.last[id] = v; $id(id).textContent = v; } }
  function pulse(id, cls) { const e = $id(id); if (!e) return; e.classList.remove(cls); void e.getBoundingClientRect(); e.classList.add(cls); }
  function showCallout(text, color) { const c = $id('callout'); c.textContent = text; c.style.color = color || ''; pulse('callout', 'show'); }
  // xp sections
  const xpSecs = []; for (let i = 0; i < 10; i++) { const s = el('div', 'xpsec', $id('xpBar')); xpSecs.push(el('i', '', s)); }
  function gainXP(n) {
    G.xp += n;
    if (G.xp >= G.xpMax) { G.xp -= G.xpMax; G.level++; G.xpMax = Math.round(G.xpMax * 1.15 / 10) * 10; P.max = Math.round(P.max * 1.08); P.health = P.max; showCallout('LEVEL ' + G.level, '#9fe0ff'); }
  }
  // the ember sockets
  function sockSvg(id, slot) {
    const e = EMBER[id];
    return `<svg viewBox="-46 -46 92 92" aria-hidden="true">
      <defs><radialGradient id="eg${slot}" cx="50%" cy="50%" r="50%"><stop offset="0.55" stop-color="${e.color}" stop-opacity="0.55"/><stop offset="1" stop-color="${e.color}" stop-opacity="0"/></radialGradient></defs>
      <circle class="ember" r="46" fill="url(#eg${slot})"/>
      <circle r="38.5" fill="#0e1116"/><circle r="36.5" fill="url(#gmRing)"/><circle r="32.5" fill="rgba(7,26,40,0.92)"/>
      <circle r="33" fill="none" stroke="#5ac8ff" stroke-width="1" opacity="0.55"/>
      <g transform="scale(0.92)">${GLYPH[id]}</g>
      <path class="pie" d="" fill="rgba(3,9,14,0.74)"/>
      <circle class="ready" r="37.5" fill="none" stroke="#ff9f1c" stroke-width="3" opacity="0"/>
      <path class="arc" d="" fill="none" stroke="${e.color}" stroke-width="4" stroke-linecap="round"/>
      <path d="M-7 -40 L0 -48 L7 -40 L4 -37 L0 -42 L-4 -37 Z" fill="#ff9f1c" stroke="#0e1116" stroke-width="1.6" stroke-linejoin="round"/>
    </svg><div class="cd ol"></div><div class="keytab"><span class="key">${slot ? 'F' : 'Q'}</span></div><div class="nm ol">${e.name.toUpperCase()}</div>`;
  }
  function buildSockets() { for (const slot of [0, 1]) { const s = $id('sock' + slot); s.innerHTML = sockSvg(G.slots[slot], slot); s.dataset.id = G.slots[slot]; } hud.last = {}; }
  function arcPath(r, f) {
    if (f <= 0) return '';
    if (f >= 0.999) return `M0 ${-r} A${r} ${r} 0 1 1 -0.01 ${-r} Z`;
    const a = -Math.PI / 2 + f * 2 * Math.PI, x = (Math.cos(a) * r).toFixed(2), y = (Math.sin(a) * r).toFixed(2);
    return `M0 ${-r} A${r} ${r} 0 ${f > 0.5 ? 1 : 0} 1 ${x} ${y}`;
  }
  function piePath(r, f) {
    if (f <= 0) return '';
    if (f >= 0.999) return `M0 0 m${-r} 0 a${r} ${r} 0 1 0 ${2 * r} 0 a${r} ${r} 0 1 0 ${-2 * r} 0`;
    const a = -Math.PI / 2 + f * 2 * Math.PI, x = (Math.cos(a) * r).toFixed(2), y = (Math.sin(a) * r).toFixed(2);
    return `M0 0 L0 ${-r} A${r} ${r} 0 ${f > 0.5 ? 1 : 0} 1 ${x} ${y} Z`;
  }
  function updateHud() {
    const hp = clamp(P.health / P.max, 0, 1), low = hp <= 0.3;
    $id('hpFill').style.width = (hp * 100).toFixed(1) + '%';
    G.chip = Math.max(hp, (G.chip ?? hp) - (G.t - P.hurtT > 0.45 ? 0.02 : 0)); $id('hpChip').style.width = (G.chip * 100).toFixed(1) + '%';
    $id('hpFill').classList.toggle('low', low); $id('hpNum').classList.toggle('low', low);
    const hpHtml = `${Math.ceil(P.health)}<span>/ ${P.max}</span>`; if (hud.last.hp !== hpHtml) { hud.last.hp = hpHtml; $id('hpNum').innerHTML = hpHtml; }
    const xpK = G.xp / G.xpMax; xpSecs.forEach((e, i) => { e.style.width = (clamp((xpK - i / 10) * 10, 0, 1) * 100).toFixed(1) + '%'; });
    const xpHtml = `${G.xp.toLocaleString('en-US')}<span>/ ${G.xpMax.toLocaleString('en-US')} XP</span>`; if (hud.last.xp !== xpHtml) { hud.last.xp = xpHtml; $id('xpText').innerHTML = xpHtml; }
    setText('lvl', String(G.level));
    const hurtEyes = low || G.t < (G.hurtEyes || 0);
    $id('calmEyes').setAttribute('opacity', hurtEyes ? '0' : '1'); $id('hurtEyes').setAttribute('opacity', hurtEyes ? '1' : '0');
    // the cartridge
    const f = GUN.reloadT > 0 ? 0 : GUN.mag / GUN.cap, lowAmmo = f <= 0.25;
    $id('magFill').setAttribute('width', (211 * f).toFixed(1)); $id('magHatch').setAttribute('width', (211 * f).toFixed(1));
    $id('magFill').setAttribute('fill', lowAmmo ? 'url(#magOrange)' : 'url(#magCyan)');
    $id('magReload').setAttribute('width', GUN.reloadT > 0 ? (211 * (1 - GUN.reloadT / GUN.reloadDur)).toFixed(1) : '0');
    setText('magCount', String(GUN.mag)); setText('magRes', '/' + GUN.reserve);
    $id('magCount').setAttribute('fill', GUN.mag === 0 ? '#ff5b4a' : lowAmmo ? '#ffb54d' : '#ffffff');
    setText('status', GUN.reloadT > 0 ? 'RELOADING' : GUN.mag === 0 ? 'PRESS R' : '');
    // the sockets
    for (const slot of [0, 1]) {
      const s = $id('sock' + slot), e = EMBER[G.slots[slot]];
      const cdK = G.cd[slot] > 0 ? G.cd[slot] / e.cd : 0, act = G.active[slot] > 0 && e.dur ? G.active[slot] / e.dur : 0;
      const pie = s.querySelector('.pie'), arc = s.querySelector('.arc'), ready = s.querySelector('.ready'), cdEl = s.querySelector('.cd');
      const pd = piePath(32.5, act > 0 ? 0 : cdK); if (pie.getAttribute('d') !== pd) pie.setAttribute('d', pd);
      arc.setAttribute('d', arcPath(41, act));
      const isReady = G.cd[slot] <= 0;
      ready.setAttribute('opacity', isReady ? '1' : '0');
      s.classList.toggle('ready', isReady);
      const txt = !isReady && act <= 0 ? String(Math.ceil(G.cd[slot])) : ''; if (cdEl.textContent !== txt) cdEl.textContent = txt;
    }
    updateTracker();
  }

  // the tracker: a short tour of the embers, then the raid
  const TRK = () => [
    { text: `Use ${EMBER[G.slots[0]].name}`, key: 'Q', hint: EMBER[G.slots[0]].name, done: () => G.usedSlot0 },
    { text: `Use ${EMBER[G.slots[1]].name}`, key: 'F', hint: EMBER[G.slots[1]].name, done: () => G.usedSlot1 },
    { text: 'Clear the spider raid', count: 12, prog: () => Math.min(12, G.kills - (G.killsAtRaid ?? 0)) },
    { text: 'Swap an ember', key: 'Tab', hint: 'Ember page', done: () => G.swapped },
  ];
  const segs = []; for (let i = 0; i < 4; i++) segs.push(el('div', 'seg', $id('trkSteps')));
  let trkDoneAt = 0;
  function updateTracker() {
    const steps = TRK(), i = Math.min(G.step, steps.length - 1), st = steps[i];
    const prog = st.count ? st.prog() : 0, done = G.step >= steps.length || (st.count ? prog >= st.count : st.done());
    if (done && G.step < steps.length && !trkDoneAt) trkDoneAt = G.t;
    if (trkDoneAt && G.t - trkDoneAt > 1.4) { trkDoneAt = 0; G.step++; if (G.step === 2) G.killsAtRaid = G.kills; if (G.step < steps.length) pulse('trkObj', 'in'); }
    const final = G.step >= steps.length;
    setText('trkStep', final ? '4 / 4' : `${i + 1} / 4`);
    segs.forEach((s, k) => { s.className = 'seg' + (k < G.step || (k === G.step && done) ? ' done' : k === G.step ? ' now' : ''); });
    setText('trkTxt', final ? 'Hold the square' : st.text);
    setText('trkCount', final ? `${G.kills}` : st.count ? `${prog} / ${st.count}` : '');
    $id('trkObj').classList.toggle('done', done && !final);
    $id('trkChev').setAttribute('opacity', done && !final ? '0' : '1'); $id('trkTick1').setAttribute('opacity', done && !final ? '1' : '0'); $id('trkTick2').setAttribute('opacity', done && !final ? '1' : '0');
    const hint = !final && st.key; $id('trkHint').style.display = hint ? '' : 'none';
    if (hint) { setText('trkKey', st.key); setText('trkHintTxt', st.hint); }
  }

  // world labels: damage numbers, spider tags, Spyglass marks
  const world = $id('world');
  const nums = [];
  function number(at, n, kind) {
    let e = nums.find((x) => !x.on); if (!e) { if (nums.length > 40) return; e = { el: el('div', 'dmg ol', world) }; nums.push(e); }
    e.on = true; e.t = 0; e.at = at.clone(); e.el.className = 'dmg ol' + (kind ? ' ' + kind : ''); e.el.textContent = String(n); e.el.style.display = '';
  }
  const tags = [], marks = [];
  function tagEl(i) { if (!tags[i]) { const t = el('div', 'tag ol', world); t.innerHTML = '<span><b></b>SPIDER</span><div class="bar"><i></i></div>'; tags[i] = t; } return tags[i]; }
  function markEl(i) { if (!marks[i]) { const m = el('div', 'mk', world); m.innerHTML = '<svg viewBox="-11 -11 22 22"><path d="M-9 -8 L9 -8 L0 6 Z" fill="#ff9f1c" stroke="#0e1116" stroke-width="2.2" stroke-linejoin="round"/></svg><span class="ol"></span>'; marks[i] = m; } return marks[i]; }
  const proj = V3();
  function toScreen(p) { proj.copy(p).project(camera); if (proj.z > 1 || proj.z < -1) return null; return [(proj.x * 0.5 + 0.5) * innerWidth, (-proj.y * 0.5 + 0.5) * innerHeight]; }
  function updateWorldLabels(dt) {
    for (const e of nums) {
      if (!e.on) continue; e.t += dt;
      if (e.t > 0.9) { e.on = false; e.el.style.display = 'none'; continue; }
      const p = toScreen(e.at.clone().add(V3(0, e.t * 0.9, 0)));
      if (!p) { e.el.style.display = 'none'; continue; }
      e.el.style.display = ''; e.el.style.opacity = String(e.t > 0.6 ? 1 - (e.t - 0.6) / 0.3 : 1);
      e.el.style.transform = `translate(${p[0].toFixed(1)}px, ${p[1].toFixed(1)}px) translate(-50%, -50%) scale(${e.t < 0.08 ? 1.4 - e.t * 5 : 1})`;
    }
    let ti = 0, mi = 0;
    for (const s of spiders) {
      if (s.state === 'dead') continue;
      const head = V3(s.pos.x, s.pos.y + 1.3 * s.scale + 0.2, s.pos.z);
      const p = toScreen(head);
      if (p && G.t < s.tagUntil && G.mode !== 'menu') {
        const t = tagEl(ti++); t.style.display = ''; t.style.transform = `translate(${p[0].toFixed(1)}px, ${(p[1] - 6).toFixed(1)}px) translate(0, -100%)`;
        t.querySelector('b').textContent = String(s.level); t.querySelector('i').style.width = (clamp(s.hp / s.max, 0, 1) * 100).toFixed(0) + '%';
      }
      if (p && G.t < s.markUntil) {
        const m = markEl(mi++); m.style.display = ''; m.style.transform = `translate(${p[0].toFixed(1)}px, ${(p[1] - 30 * hudScale()).toFixed(1)}px)`;
        m.querySelector('span').textContent = Math.round(s.pos.distanceTo(P.pos)) + ' m';
      }
    }
    for (let i = ti; i < tags.length; i++) tags[i].style.display = 'none';
    for (let i = mi; i < marks.length; i++) marks[i].style.display = 'none';
  }
  // speed lines for Dust Devil
  (() => { const sv = $id('speed'); let s = ''; for (let i = 0; i < 34; i++) { const a = i / 34 * 6.283 + Math.sin(i * 7.1) * 0.08, r0 = 60 + (i % 3) * 8, r1 = 120; s += `<path d="M${(Math.cos(a) * r0).toFixed(1)} ${(Math.sin(a) * r0 * 0.62).toFixed(1)} L${(Math.cos(a) * r1).toFixed(1)} ${(Math.sin(a) * r1 * 0.62).toFixed(1)}" stroke="#fffaf0" stroke-width="${1.2 + (i % 4) * 0.5}" stroke-linecap="round"/>`; } sv.innerHTML = `<g opacity="0.9">${s}</g>`; })();

  // ---------------------------------------------------------------- the ember page
  function buildCards() {
    const box2 = $id('cards'); box2.innerHTML = '';
    for (const e of EMBERS) {
      const c = el('div', 'card', box2), on = G.slots.indexOf(e.id);
      if (on >= 0) c.classList.add('eq');
      c.innerHTML = `<svg class="ic" viewBox="-46 -46 92 92" aria-hidden="true"><circle r="38.5" fill="#0e1116"/><circle r="36.5" fill="url(#gmRing)"/><circle r="32.5" fill="rgba(7,26,40,0.92)"/>
        <circle r="33" fill="none" stroke="#5ac8ff" stroke-width="1" opacity="0.55"/><g transform="scale(0.92)">${GLYPH[e.id]}</g></svg>
        <div class="head"><div class="nature">${e.nature}</div><div class="nm">${e.name.toUpperCase()}</div><div class="who">${e.who}'s ember · ${e.where}</div></div>
        <div class="desc">${e.desc}</div>
        <div class="cdtxt">Cooldown ${e.cd} s${e.dur ? ` · lasts ${e.dur} s` : ''}</div>
        <div class="row"><button class="btn small eqbtn" data-slot="0" aria-pressed="${on === 0}">${on === 0 ? 'ON Q' : 'EQUIP ON Q'}</button>
          <button class="btn small eqbtn" data-slot="1" aria-pressed="${on === 1}">${on === 1 ? 'ON F' : 'EQUIP ON F'}</button></div>`;
      c.querySelectorAll('.eqbtn').forEach((b) => b.addEventListener('click', () => equip(e.id, +b.dataset.slot)));
    }
  }
  function equip(id, slot) {
    const other = 1 - slot;
    if (G.slots[slot] === id) return;
    if (G.slots[other] === id) { G.slots[other] = G.slots[slot]; [G.cd[0], G.cd[1]] = [G.cd[1], G.cd[0]]; [G.active[0], G.active[1]] = [G.active[1], G.active[0]]; }
    else { G.cd[slot] = 0; G.active[slot] = 0; }
    G.slots[slot] = id; G.playSlots = [...G.slots]; G.swapped = true;
    buildSockets(); buildCards();
  }
  function openEmbers() { if (G.mode === 'watch') stopWatch(); G.mode = 'play'; G.paused = true; buildCards(); show('embers'); if (document.pointerLockElement) document.exitPointerLock(); setTimeout(() => $id('embersBack').focus(), 30); }
  function closeEmbers() { hide('embers'); resume(); }

  // ---------------------------------------------------------------- menus and modes
  const show = (id) => { $id(id).hidden = false; };
  const hide = (id) => { $id(id).hidden = true; };
  const START = { x: toX(150), z: toZ(-950), yaw: 8 * DEG, pitch: -0.05 };
  function resetFight() {
    spiders.length = 0; ghost.on = false; for (const p of slag.pools) DYN.remove(p.m); slag.pools.length = 0;
    G.cd = [0, 0]; G.active = [0, 0]; GUN.mag = GUN.cap; GUN.reloadT = 0; GUN.luckUntil = 0; P.health = P.max;
  }
  function lockPointer() {
    if (IS_TOUCH_UI) return;
    try { const r = canvas.requestPointerLock && canvas.requestPointerLock(); if (r && r.catch) r.catch(() => { G.noLock = true; }); } catch (e) { G.noLock = true; }
  }
  function startPlay() {
    stopWatch(); hide('start'); hide('pause'); hide('embers'); $id('hud').hidden = false;
    if (G.mode !== 'play' || G.fresh) { resetFight(); placePlayer(START); G.slots = [...G.playSlots]; buildSockets(); G.fresh = false; G.step = 0; G.usedSlot0 = G.usedSlot1 = false; G.kills = 0; spawnRaid(3); }
    G.mode = 'play'; G.paused = false; $id('trk').style.display = ''; $id('cap').classList.remove('on'); lockPointer(); canvas.focus();
  }
  function resume() { hide('pause'); hide('start'); $id('hud').hidden = false; G.mode = 'play'; G.paused = false; lockPointer(); canvas.focus(); }
  function pauseGame() { if (G.mode !== 'play' || !$id('embers').hidden) return; G.paused = true; show('pause'); keys.clear(); GUN.firing = false; setTimeout(() => $id('resumeBtn').focus(), 30); }
  $id('playBtn').addEventListener('click', () => { G.fresh = true; startPlay(); });
  $id('watchBtn').addEventListener('click', () => startWatch());
  $id('pWatchBtn').addEventListener('click', () => { hide('pause'); startWatch(); });
  $id('resumeBtn').addEventListener('click', resume);
  $id('pEmbersBtn').addEventListener('click', () => { hide('pause'); openEmbers(); });
  $id('embersBack').addEventListener('click', closeEmbers);
  document.addEventListener('pointerlockerror', () => { G.noLock = true; });
  document.addEventListener('pointerlockchange', () => { if (!document.pointerLockElement && G.mode === 'play' && !G.paused && $id('embers').hidden) pauseGame(); });

  // ---------------------------------------------------------------- input
  const IS_TOUCH_UI = ('ontouchstart' in window || (navigator.maxTouchPoints || 0) > 0) && matchMedia('(pointer: coarse)').matches;
  if (IS_TOUCH_UI) document.body.classList.add('touch');
  const touch = { stick: { x: 0, y: 0 } };
  window.addEventListener('keydown', (e) => {
    if (e.code === 'Tab') { e.preventDefault(); if (G.mode === 'loading' || G.mode === 'menu') return; if (!$id('embers').hidden) closeEmbers(); else openEmbers(); return; }
    if (e.code === 'Escape') { if (!$id('embers').hidden) { closeEmbers(); return; } if (G.mode === 'watch') { stopWatch(); showStart(); return; } if (G.mode === 'play' && !G.paused && !document.pointerLockElement) pauseGame(); return; }
    if (G.paused || G.mode !== 'play') return;
    if (e.ctrlKey || e.metaKey || e.altKey) return;
    keys.add(e.code);
    if (e.code === 'KeyQ') { if (useEmber(0)) G.usedSlot0 = true; }
    if (e.code === 'KeyF') { e.preventDefault(); if (useEmber(1)) G.usedSlot1 = true; }
    if (e.code === 'KeyR') startReload();
    if (['Space', 'ArrowUp', 'ArrowDown'].includes(e.code)) e.preventDefault();
  });
  window.addEventListener('keyup', (e) => keys.delete(e.code));
  window.addEventListener('blur', () => { keys.clear(); GUN.firing = false; });
  canvas.addEventListener('mousedown', (e) => {
    if (G.mode === 'watch') { if (!IS_TOUCH_UI) { stopWatch(); G.fresh = true; startPlay(); } return; }
    if (G.mode !== 'play') return;
    if (G.paused) return;
    if (!document.pointerLockElement && !G.noLock) { lockPointer(); }
    if (e.button === 0) { GUN.firing = true; if (GUN.fireT <= 0) { fire(); GUN.fireT = 1 / GUN.rate; } }
  });
  window.addEventListener('mouseup', (e) => { if (e.button === 0) GUN.firing = false; });
  window.addEventListener('mousemove', (e) => {
    if (G.mode !== 'play' || G.paused) return;
    if (!document.pointerLockElement && !G.noLock) return;
    const k = 0.0022;
    P.yaw += e.movementX * k; P.pitch = clamp(P.pitch - e.movementY * k, -1.3, 1.3); G.sway = clamp((G.sway || 0) - e.movementX * 0.0006, -0.05, 0.05);
  });
  canvas.addEventListener('contextmenu', (e) => e.preventDefault());
  // touch: the left side moves, the right side looks, the buttons fire and use the embers
  if (IS_TOUCH_UI) {
    const stickEl = $id('stick'), base2 = stickEl.querySelector('.base'), knob = stickEl.querySelector('.knob');
    let sid = null, so = null;
    stickEl.addEventListener('pointerdown', (e) => { sid = e.pointerId; so = { x: e.clientX, y: e.clientY }; stickEl.setPointerCapture(e.pointerId); for (const x of [base2, knob]) { x.style.display = 'block'; x.style.left = e.clientX + 'px'; x.style.top = e.clientY + 'px'; } });
    stickEl.addEventListener('pointermove', (e) => { if (e.pointerId !== sid) return; const dx = e.clientX - so.x, dy = e.clientY - so.y, l = Math.hypot(dx, dy), m = Math.min(l, 50); touch.stick.x = l ? dx / l * m / 50 : 0; touch.stick.y = l ? -dy / l * m / 50 : 0; knob.style.left = (so.x + (l ? dx / l * m : 0)) + 'px'; knob.style.top = (so.y + (l ? dy / l * m : 0)) + 'px'; });
    const end = (e) => { if (e.pointerId !== sid) return; sid = null; touch.stick.x = touch.stick.y = 0; base2.style.display = knob.style.display = 'none'; };
    stickEl.addEventListener('pointerup', end); stickEl.addEventListener('pointercancel', end);
    let lid = null, lp = null;
    canvas.addEventListener('pointerdown', (e) => { if (G.mode !== 'play' || e.pointerType !== 'touch') return; lid = e.pointerId; lp = { x: e.clientX, y: e.clientY }; });
    canvas.addEventListener('pointermove', (e) => { if (e.pointerId !== lid) return; P.yaw += (e.clientX - lp.x) * 0.006; P.pitch = clamp(P.pitch - (e.clientY - lp.y) * 0.006, -1.3, 1.3); lp = { x: e.clientX, y: e.clientY }; });
    canvas.addEventListener('pointerup', (e) => { if (e.pointerId === lid) lid = null; });
    const tf = $id('tFire'); tf.addEventListener('pointerdown', (e) => { e.preventDefault(); GUN.firing = true; }); tf.addEventListener('pointerup', () => { GUN.firing = false; }); tf.addEventListener('pointercancel', () => { GUN.firing = false; });
    $id('tQ').addEventListener('click', () => { if (useEmber(0)) G.usedSlot0 = true; });
    $id('tF').addEventListener('click', () => { if (useEmber(1)) G.usedSlot1 = true; });
    $id('tMenu').addEventListener('click', () => { if (G.mode === 'play') pauseGame(); });
  }

  // ---------------------------------------------------------------- watch all six: a scripted showcase
  const WATCH_AT = { x: toX(-100), z: toZ(-900), yaw: 25 * DEG, pitch: -0.06 };
  const DASH_AT = { x: toX(60), z: toZ(-1150), yaw: 0, pitch: -0.05 };
  const SIGHT_AT = (() => { const X = -150, Y = -700; return { x: toX(Y), z: toZ(X), yaw: Math.atan2(-(toX(-1591) - toX(Y)), toZ(-811) - toZ(X)), pitch: -0.04 }; })();
  const SCENES = [
    { id: 'dash', pose: DASH_AT, setup: () => gauntlet(), at: [[1.0, 'use'], [2.9, 'burst'], [3.7, 'burst']], len: 6.5 },
    { id: 'slag', pose: WATCH_AT, setup: () => cluster(6, 9.5, 1.6), at: [[1.1, 'use'], [4.2, 'burst']], len: 7.2 },
    { id: 'sight', pose: SIGHT_AT, setup: () => hidden2(), at: [[1.0, 'use']], len: 6.6, look: 'hidden' },
    { id: 'luck', pose: WATCH_AT, setup: () => spread(6, 7, 12), at: [[0.8, 'use'], [1.3, 'burst'], [2.1, 'burst'], [2.9, 'burst'], [3.7, 'burst'], [4.5, 'burst']], len: 7.2 },
    { id: 'flock', pose: WATCH_AT, setup: () => cluster(5, 8, 2.2, true), at: [[0.9, 'use'], [5.5, 'burst']], len: 8.0 },
    { id: 'slam', pose: WATCH_AT, setup: () => ringAround(7, 3.8), at: [[1.6, 'use'], [4.0, 'burst'], [4.8, 'burst']], len: 7.0 },
  ];
  const W2 = { i: -1, t: 0, fired: 0, burstLeft: 0, aim: null };
  const ahead = (d, side = 0) => { const b = W2.pose || WATCH_AT, f = fwdOf(b.yaw), r = rightOf(b.yaw); return V3(b.x + f.x * d + r.x * side, 0, b.z + f.z * d + r.z * side); };
  // six along the dash's first stretch, a step to each side of its path
  function gauntlet() { for (const [d, sd] of [[2.8, 0.3], [3.6, -1.4], [4.4, 1.5], [5.2, -0.6], [6.0, 0.9], [6.8, -1.6]]) { const s = put(ahead(d, sd), { hold: true }); if (s) s.face = Math.PI; } }
  function put(p, o) { const q = p.clone(); collide(q, 0.9); return addSpider(q.x, q.z, o); }
  function line(n, d0, d1) { for (let i = 0; i < n; i++) { const s = put(ahead(lerp(d0, d1, i / (n - 1)) + rand(-0.3, 0.3), rand(-0.6, 0.6)), { hold: true }); if (s) s.face = WATCH_AT.yaw + Math.PI; } }
  function cluster(n, d, r, walk) { for (let i = 0; i < n; i++) { const a = i / n * 6.283, s = put(ahead(d + Math.sin(a) * r, Math.cos(a) * r), { hold: !walk }); if (s) { s.face = Math.atan2(P.pos.x - s.pos.x, P.pos.z - s.pos.z); s.speed = walk ? 1.6 : s.speed; } } }
  function spread(n, d0, d1) { for (let i = 0; i < n; i++) { const s = put(ahead(rand(d0, d1), (i / (n - 1) - 0.5) * 9), {}); if (s) s.speed = 1.3; } }
  function ringAround(n, r) { for (let i = 0; i < n; i++) { const a = (W2.pose || WATCH_AT).yaw + (i / n - 0.5) * 3.6, s = put(V3(P.pos.x - Math.sin(a) * r * 1.6, 0, P.pos.z + Math.cos(a) * r * 1.6), {}); if (s) s.speed = 1.6; } }
  function hidden2() {
    // three behind the cottage across the lane, two in the open in front of it
    const store = BOXES.reduce((b, x) => (Math.hypot(x.x - toX(-1591), x.z - toZ(-811)) < Math.hypot(b.x - toX(-1591), b.z - toZ(-811)) ? x : b), BOXES[0]);
    const c = V3(store.x, 0, store.z), back = c.clone().sub(P.pos).setY(0).normalize(), side = rightOf(Math.atan2(-back.x, back.z));
    for (let i = 0; i < 3; i++) put(c.clone().addScaledVector(back, 6.5 + (i % 2) * 1.6).addScaledVector(side, (i - 1) * 2.2), { hold: true });
    const dist = c.distanceTo(P.pos.clone().setY(0));
    for (const [k, sd] of [[0.5, -2.8], [0.62, 3.0]]) put(P.pos.clone().setY(0).addScaledVector(back, dist * k).addScaledVector(side, sd), { hold: true });
    W2.hiddenAt = c;
  }
  function startWatch() {
    hide('start'); hide('pause'); hide('embers'); $id('hud').hidden = false; if (document.pointerLockElement) document.exitPointerLock();
    G.mode = 'watch'; G.paused = false; W2.i = -1; W2.t = 0; nextScene();
  }
  function stopWatch() { if (G.mode !== 'watch') return; G.mode = 'menu'; $id('cap').classList.remove('on'); G.fresh = true; }
  function nextScene() {
    W2.i = (W2.i + 1) % SCENES.length; W2.t = 0; W2.fired = 0; W2.burstLeft = 0;
    const sc = SCENES[W2.i], e = EMBER[sc.id];
    resetFight(); W2.pose = sc.pose; placePlayer(sc.pose);
    G.slots = [sc.id, SCENES[(W2.i + 1) % SCENES.length].id]; buildSockets();
    sc.setup();
    $id('trk').style.display = 'none';
    setText('capN', `${W2.i + 1} / 6`); setText('capName', e.name.toUpperCase()); setText('capWho', `${e.who}'s ember · ${e.where}`); setText('capWhat', e.desc);
    $id('cap').classList.add('on');
  }
  function updateWatch(dt) {
    const sc = SCENES[W2.i]; W2.t += dt;
    for (; W2.fired < sc.at.length && W2.t >= sc.at[W2.fired][0]; W2.fired++) {
      const what = sc.at[W2.fired][1];
      if (what === 'use') { aimForScene(sc, true); useEmber(0); }
      else W2.burstLeft = 7;
    }
    aimForScene(sc, false, dt);
    if (W2.burstLeft > 0) { GUN.fireT -= 0; if (GUN.fireT <= 0) { fire(); GUN.fireT = 1 / GUN.rate; W2.burstLeft--; } }
    if (GUN.mag < 8) GUN.mag = GUN.cap;
    if (W2.t > sc.len - 0.4) $id('cap').classList.remove('on');
    if (W2.t > sc.len) nextScene();
  }
  function aimForScene(sc, snap, dt = 0.016) {
    let target = null;
    if (sc.look === 'hidden' && W2.hiddenAt) target = W2.hiddenAt.clone().setY((gy(W2.hiddenAt.x, W2.hiddenAt.z) ?? 0) + 1.2);
    if (!target) { let best = null, bd = 1e9; const f = fwdOf(W2.pose.yaw); for (const s of alive()) { const v = s.pos.clone().sub(P.pos), d = v.length(); if (v.setY(0).normalize().dot(f) < 0.2) continue; if (d < bd) { bd = d; best = s; } } if (best) target = spiderCenter(best); }
    if (sc.id === 'dash' && W2.t < 1.5) target = P.pos.clone().addScaledVector(fwdOf(DASH_AT.yaw), 12).setY(P.pos.y + 0.25);
    if (sc.id === 'dash' && W2.t >= 1.5) target = V3(DASH_AT.x, (gy(DASH_AT.x, DASH_AT.z) ?? 0) + 0.7, DASH_AT.z);
    if (sc.id === 'slam') target = ahead(8).setY(P.pos.y + 1.0);
    if (!target) return;
    const e = eyePos(), d = target.clone().sub(e), yaw = Math.atan2(-d.x, d.z), pitch = Math.atan2(d.y, Math.hypot(d.x, d.z));
    let dy = yaw - P.yaw; while (dy > Math.PI) dy -= 2 * Math.PI; while (dy < -Math.PI) dy += 2 * Math.PI;
    const k = snap ? 1 : Math.min(1, dt * 3.2);
    P.yaw += dy * k; P.pitch += (pitch - P.pitch) * k;
  }

  // ---------------------------------------------------------------- the frame
  let last = performance.now(), spawnT = 0, attractA = 0;
  function frame(now) {
    requestAnimationFrame(frame);
    const rawDt = clamp((now - last) / 1000, 0, 0.05); last = now;
    const slow = G.mode === 'watch' ? (P.dash ? 0.3 : G.slowT > 0 ? lerp(1, 0.3, G.slowT / 0.4) : 1) : 1;
    G.slowT = Math.max(0, (G.slowT || 0) - rawDt);
    const dt = G.paused ? 0 : rawDt * slow;
    G.t += dt;
    if (G.mode === 'play' && !G.paused) {
      const { l, run, mz } = moveInput();
      updatePlayer(dt);
      GUN.fireT -= dt;
      if (GUN.firing && GUN.fireT <= 0 && G.deadT <= 0) { fire(); GUN.fireT = 1 / GUN.rate; }
      if (GUN.reloadT > 0) { GUN.reloadT -= dt; if (GUN.reloadT <= 0) { GUN.reloadT = 0; const take = Math.min(GUN.cap - GUN.mag, GUN.reserve); GUN.mag += take; GUN.reserve -= take; } }
      spawnT -= dt; if (spawnT <= 0) { spawnT = 2.4; if (alive().length < 7) spawnRaid(alive().length < 3 ? 2 : 1); }
      updateGun(dt, l > 0.1, run && mz > 0);
    } else if (G.mode === 'watch') {
      updatePlayer(dt); GUN.fireT -= dt; updateWatch(dt); updateGun(dt, false, false);
    } else if (G.mode === 'menu') {
      attractA += rawDt * 0.05; P.yaw = START.yaw + Math.sin(attractA) * 0.6; P.pitch = -0.03;
    }
    if (!G.paused) {
      for (const slot of [0, 1]) { G.cd[slot] = Math.max(0, G.cd[slot] - dt); G.active[slot] = Math.max(0, G.active[slot] - dt); }
      updateSpiders(dt); updateGhost(dt); updateSlag(dt); updateParts(dt); updateRings(dt); updateTracers(dt); updateCards(dt); updateCracks(dt);
      G.slamDip = Math.max(0, G.slamDip - dt * 1.6); G.shake = Math.max(0, G.shake - dt); G.fovKick = Math.max(0, G.fovKick - dt * 2.4); G.sway = (G.sway || 0) * Math.max(0, 1 - dt * 6);
    }
    drawSpiders();
    // the camera
    const e = eyePos(), sh = G.shake > 0 ? G.shake * 0.12 : 0;
    camera.position.set(e.x + rand(-sh, sh), e.y + rand(-sh, sh) + (G.mode === 'play' && moveInput().l > 0.1 ? Math.abs(Math.cos(P.bob)) * 0.03 : 0), e.z + rand(-sh, sh));
    camera.lookAt(camera.position.clone().add(aimDir()));
    const baseFov = clamp(70 / Math.sqrt(Math.min(1, camera.aspect)), 70, 90);
    camera.fov = baseFov + ease(G.fovKick) * 14; camera.updateProjectionMatrix();
    sky.position.copy(camera.position); sky.material.uniforms.uTime.value = G.t;
    if (G.layer) animate(G.layer, G.t); animateBase(G.t, dt);
    postMat.uniforms.uTime.value = G.t;
    renderFrame();
    updateWorldLabels(dt);
    if (G.mode === 'play' || G.mode === 'watch') updateHud();
  }

  // ---------------------------------------------------------------- boot
  function fitHud() {
    const w = innerWidth, h = innerHeight, narrow = w < 900;
    const s = narrow ? Math.min(w / 1180, h / 900) : Math.min(w / 1920, h / 1080);
    document.documentElement.style.setProperty('--s', String(clamp(s, 0.3, 1.6)));
    document.body.classList.toggle('narrow', narrow);
  }
  window.addEventListener('resize', () => { resize(); fitHud(); });
  function showStart() { G.mode = 'menu'; G.paused = false; $id('hud').hidden = true; show('start'); setTimeout(() => $id('playBtn').focus(), 30); placePlayer(START); }
  async function boot() {
    camera.near = 0.1; postMat.uniforms.uNear.value = 0.1; camera.far = 1700; postMat.uniforms.uFar.value = 1700; camera.updateProjectionMatrix();
    resize(); fitHud();
    const progress = (f, text) => { $id('prog').style.width = Math.round(f * 100) + '%'; if (text) $id('progtext').textContent = text; };
    progress(0.04, 'Reading the island heights…');
    const hm = MANIFEST.terrain.heights;
    HM = { n: hm.n, half: hm.half, data: new Float32Array(await fetchBinary(hm.file)) };
    progress(0.12, 'Loading the terrain and the town…');
    await buildBase((f) => progress(0.12 + f * 0.45));
    progress(0.6, 'Placing Crossroads Town…');
    G.layer = await buildLayer(0, (f) => progress(0.6 + f * 0.25));
    G.layer.group.visible = true; renderer.shadowMap.needsUpdate = true;
    buildObstacles(G.layer.S);
    progress(0.88, 'Waking the spiders…');
    const sg = await geometry('Spider'); if (sg) makeSpiderMeshes(sg);
    ghost.mesh = new THREE.Mesh(ghostGeometry(), MAT); ghost.mesh.visible = false; DYN.add(ghost.mesh);
    slag.geo = poolGeometry();
    await buildGun(); makeFlash();
    scene.fog.near = 60; scene.fog.far = 430; postMat.uniforms.uNFar.value = 260;
    buildSockets();
    progress(1, 'Ready');
    hide('loading');
    if (IS_TOUCH_UI) $id('touchui').hidden = false;
    const hash = (location.hash || '').replace('#', '');
    if (hash === 'watch') startWatch(); else showStart();
    requestAnimationFrame(frame);
    window.__ember.ready = true;
  }
  // for automated screenshots and checks
  window.__ember = { ready: false, G, P, GUN, spiders, parts, partMeshes, startWatch, startPlay: () => { G.fresh = true; startPlay(); }, useEmber, openEmbers, closeEmbers, equip,
    spawn: (d = 10, side = 0) => { const p = P.pos.clone().addScaledVector(fwdOf(P.yaw), d).addScaledVector(rightOf(P.yaw), side); return !!addSpider(p.x, p.z); },
    fire, place: (x, z, yawDeg) => placePlayer({ x, z, yaw: yawDeg * DEG }),
    aim: (i = 0) => { const s = alive()[i]; if (!s) return false; const d = spiderCenter(s).sub(eyePos()); P.yaw = Math.atan2(-d.x, d.z); P.pitch = Math.atan2(d.y, Math.hypot(d.x, d.z)); return true; }, scene: (i) => { W2.i = i - 1; nextScene(); }, boxes: () => BOXES.length };
  boot().catch((e) => { console.error(e); $id('progtext').textContent = 'Could not load the town: ' + e.message; });
})();
