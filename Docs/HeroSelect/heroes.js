(() => {
  // ================================================================ the heroes and their powers (a design sketch)
  const W = '#f4f8fb', INK = '#0a1218';
  const GLYPH = {
    luck: `<g stroke="${INK}" stroke-width="3" stroke-linejoin="round"><rect x="-19" y="-19" width="22" height="30" rx="2.5" fill="${W}" transform="rotate(-16 -8 -4)"/>
      <rect x="-4" y="-15" width="22" height="30" rx="2.5" fill="${W}" transform="rotate(12 7 0)"/></g>
      <path d="M7 -8 C2 -2 -1 2 2.5 5.5 C4.5 7.5 6.5 6.5 7 5 C7.5 6.5 9.5 7.5 11.5 5.5 C15 2 12 -2 7 -8 Z M7 4 L5 10 L9 10 Z" fill="${INK}" transform="rotate(12 7 0)"/>
      <path d="M-14 -11 l2.5 4 l-2.5 4 l-2.5 -4 Z" fill="#d6463a" transform="rotate(-16 -8 -4)"/>`,
    dash: `<g fill="none" stroke-linecap="round"><g stroke="${INK}" stroke-width="9"><path d="M-22 -11 H6 C15 -11 16 -22 8 -22"/><path d="M-26 1 H14 C25 1 25 14 15 14"/><path d="M-18 13 H-2"/></g>
      <g stroke="${W}" stroke-width="5"><path d="M-22 -11 H6 C15 -11 16 -22 8 -22"/><path d="M-26 1 H14 C25 1 25 14 15 14"/><path d="M-18 13 H-2"/></g></g>`,
    slag: `<path d="M9 -9 Q14 -19 21 -17" fill="none" stroke="${INK}" stroke-width="6" stroke-linecap="round"/><path d="M9 -9 Q14 -19 21 -17" fill="none" stroke="#c9b28a" stroke-width="3" stroke-linecap="round"/>
      <circle cx="-2" cy="5" r="15" fill="#3a3034" stroke="${INK}" stroke-width="3"/><path d="M-12 1 Q-2 -4 8 1" fill="none" stroke="#ff8a1c" stroke-width="3.5" stroke-linecap="round"/>
      <path d="M-8 -3 Q-6 -8 -1 -9" fill="none" stroke="#ffffff" stroke-opacity=".55" stroke-width="2" stroke-linecap="round"/>
      <path d="M22 -24 l2 5 l5 1 l-4 3 l1 5 l-4 -3 l-4 3 l1 -5 l-4 -3 l5 -1 Z" fill="#ffcf4a" stroke="${INK}" stroke-width="1.6" stroke-linejoin="round"/>`,
    sight: `<path d="M-22 0 Q0 -18 22 0 Q0 18 -22 0 Z" fill="${W}" stroke="${INK}" stroke-width="3" stroke-linejoin="round"/>
      <circle r="8.5" fill="#ff9f1c" stroke="${INK}" stroke-width="2.5"/><circle r="3.5" fill="${INK}"/><circle cx="-2.5" cy="-3" r="1.6" fill="#fff"/>
      <g stroke="${INK}" stroke-width="5" stroke-linecap="round"><path d="M0 -27 V-21 M0 21 V27 M-28 0 H-25 M25 0 H28"/></g>
      <g stroke="${W}" stroke-width="2.4" stroke-linecap="round"><path d="M0 -27 V-21 M0 21 V27 M-28 0 H-25 M25 0 H28"/></g>`,
    sundown: `<g stroke-linecap="round"><g stroke="${INK}" stroke-width="5.5"><path d="M0 -13 V-21 M-14 -6 L-20 -12 M14 -6 L20 -12 M-21 3 H-27 M21 3 H27"/></g>
      <g stroke="${W}" stroke-width="2.5"><path d="M0 -13 V-21 M-14 -6 L-20 -12 M14 -6 L20 -12 M-21 3 H-27 M21 3 H27"/></g></g>
      <path d="M-15 8 A15 15 0 0 1 15 8 Z" fill="#ff9f1c" stroke="${INK}" stroke-width="3" stroke-linejoin="round"/>
      <path d="M-26 8 H26" stroke="${INK}" stroke-width="5" stroke-linecap="round"/>
      <path d="M-15 16 H15 M-8 23 H8" stroke="${INK}" stroke-width="3" stroke-linecap="round"/>`,
    borrowed: `<path d="M-14 -22 H14 M-14 22 H14" stroke="${INK}" stroke-width="5" stroke-linecap="round"/>
      <path d="M-10 -20 C-10 -6 -3 -4 -3 0 C-3 4 -10 6 -10 20 H10 C10 6 3 4 3 0 C3 -4 10 -6 10 -20 Z" fill="${W}" stroke="${INK}" stroke-width="3" stroke-linejoin="round"/>
      <path d="M-7 -14 H7 C6 -9 2 -6 0 -3 C-2 -6 -6 -9 -7 -14 Z" fill="#5ac8ff"/><path d="M-7 18 C-5 12 -2 10 0 9 C2 10 5 12 7 18 Z" fill="#5ac8ff"/>
      <path d="M17 -6 l6 -6 M19 2 h8 M17 10 l6 6" stroke="${INK}" stroke-width="3" stroke-linecap="round"/>`,
    pickup: `<g stroke="${INK}" stroke-width="2.6" stroke-linejoin="round" fill="${W}">
      <rect x="-22" y="-14" width="17" height="25" rx="2" transform="rotate(-24 -13 -1)"/><rect x="-9" y="-19" width="17" height="25" rx="2" transform="rotate(-4 0 -6)"/>
      <rect x="4" y="-16" width="17" height="25" rx="2" transform="rotate(18 12 -3)"/></g>
      <path d="M12 -8 l3 5 l-3 5 l-3 -5 Z" fill="#d6463a" transform="rotate(18 12 -3)"/>
      <path d="M-4 16 L-8 26 M6 15 L8 26 M16 13 L22 22" stroke="${INK}" stroke-width="3" stroke-linecap="round"/>`,
    double: `<circle cx="-7" cy="-5" r="15" fill="#e0b44a" stroke="${INK}" stroke-width="3"/><circle cx="-7" cy="-5" r="9.5" fill="none" stroke="${INK}" stroke-width="2"/>
      <path d="M-7 -11 l1.8 3.8 l4.2 .6 l-3 3 l.7 4.2 l-3.7 -2 l-3.7 2 l.7 -4.2 l-3 -3 l4.2 -.6 Z" fill="${INK}"/>
      <circle cx="9" cy="8" r="15" fill="${W}" stroke="${INK}" stroke-width="3"/><path d="M3 2 L15 14 M15 2 L3 14" stroke="#d6463a" stroke-width="3.5" stroke-linecap="round"/>`,
    chain: `<g fill="none"><g stroke="${INK}" stroke-width="7"><ellipse cx="-16" cy="-16" rx="8.5" ry="5" transform="rotate(45 -16 -16)"/>
      <ellipse cx="-6" cy="-6" rx="8.5" ry="5" transform="rotate(-45 -6 -6)"/><path d="M2 2 L9 9 C15 15 14 24 7 24 C2 24 1 18 4 16"/></g>
      <g stroke="#9aa6b2" stroke-width="3"><ellipse cx="-16" cy="-16" rx="8.5" ry="5" transform="rotate(45 -16 -16)"/>
      <ellipse cx="-6" cy="-6" rx="8.5" ry="5" transform="rotate(-45 -6 -6)"/><path d="M2 2 L9 9 C15 15 14 24 7 24 C2 24 1 18 4 16"/></g></g>
      <path d="M20 -24 l5 5 M24 -14 h7 M12 -26 v-6" stroke="#6ee7ff" stroke-width="3" stroke-linecap="round"/>`,
    ward: `<circle r="25" fill="none" stroke="${INK}" stroke-width="6"/><circle r="25" fill="none" stroke="#6ee7ff" stroke-width="3" stroke-dasharray="7 5"/>
      <path d="M-5 -13 C-5 -20 5 -20 5 -13" fill="none" stroke="${INK}" stroke-width="2.8"/>
      <path d="M-8 -13 H8 L10 -9 V9 L8 13 H-8 L-10 9 V-9 Z" fill="${W}" stroke="${INK}" stroke-width="2.8" stroke-linejoin="round"/>
      <rect x="-5.5" y="-7" width="11" height="14" rx="1.5" fill="#6ee7ff" stroke="${INK}" stroke-width="1.6"/>`,
    rites: `<path d="M0 -26 C7 -18 6 -10 0 -8 C-6 -10 -7 -18 0 -26 Z" fill="#6ee7ff" stroke="${INK}" stroke-width="2.6" stroke-linejoin="round"/>
      <path d="M0 -19 C2.6 -15 2 -12 0 -11 C-2 -12 -2.6 -15 0 -19 Z" fill="${W}"/><path d="M0 -8 V-3" stroke="${INK}" stroke-width="2.4"/>
      <rect x="-8" y="-3" width="16" height="23" rx="2" fill="${W}" stroke="${INK}" stroke-width="3"/><path d="M-8 3 Q-5 9 -2.5 3" fill="none" stroke="${INK}" stroke-width="2"/>
      <path d="M-18 22 H18" stroke="${INK}" stroke-width="5" stroke-linecap="round"/>`,
    hide: `<path d="M0 -25 L20 -17 V1 C20 13 10 20 0 25 C-10 20 -20 13 -20 1 V-17 Z" fill="#9aa6b2" stroke="${INK}" stroke-width="3" stroke-linejoin="round"/>
      <path d="M-20 -4 H20" stroke="${INK}" stroke-width="3"/><path d="M-12 -18 L0 -22 L12 -18" fill="none" stroke="${W}" stroke-opacity=".7" stroke-width="2.4" stroke-linecap="round"/>
      <g fill="#e0b44a" stroke="${INK}" stroke-width="1.6"><circle cx="-12" cy="-11" r="2.6"/><circle cx="12" cy="-11" r="2.6"/><circle cx="-12" cy="5" r="2.6"/><circle cx="12" cy="5" r="2.6"/><circle cx="0" cy="15" r="2.6"/></g>`,
    spike: `<g transform="rotate(40)"><rect x="-4.5" y="-18" width="9" height="30" fill="#9aa6b2" stroke="${INK}" stroke-width="3"/>
      <path d="M-4.5 12 L0 23 L4.5 12 Z" fill="#9aa6b2" stroke="${INK}" stroke-width="3" stroke-linejoin="round"/>
      <path d="M-10 -24 H6 L10 -18 H-10 Z" fill="#e0b44a" stroke="${INK}" stroke-width="3" stroke-linejoin="round"/></g>
      <path d="M-26 6 H-16 M-27 14 H-19 M-22 22 H-14" stroke="${INK}" stroke-width="3" stroke-linecap="round"/>`,
    tripwire: `<path d="M-26 22 H26" stroke="${INK}" stroke-width="4" stroke-linecap="round"/>
      <g stroke-linecap="round"><path d="M-20 -8 L-16 21 M20 -8 L16 21" stroke="${INK}" stroke-width="8"/><path d="M-20 -8 L-16 21 M20 -8 L16 21" stroke="#b08a5a" stroke-width="4"/></g>
      <path d="M-19 -1 L19 -1" stroke="${INK}" stroke-width="2.2"/>
      <path d="M0 -14 l2.2 5 l5.3 .8 l-3.9 3.6 l1 5.3 l-4.6 -2.6 l-4.6 2.6 l1 -5.3 l-3.9 -3.6 l5.3 -.8 Z" fill="#ffcf4a" stroke="${INK}" stroke-width="1.6" stroke-linejoin="round"/>`,
    longshot: `<path d="M-25 21 Q-12 7 -1 1" fill="none" stroke="${INK}" stroke-width="3" stroke-dasharray="4 4" stroke-linecap="round"/>
      <circle cx="10" cy="-7" r="13" fill="${W}" stroke="${INK}" stroke-width="3"/><path d="M10 -24 V-15 M10 1 V10 M-7 -7 H2 M18 -7 H27" stroke="${INK}" stroke-width="3" stroke-linecap="round"/>
      <circle cx="10" cy="-7" r="3.2" fill="#ff9f1c" stroke="${INK}" stroke-width="1.5"/>`,
  };
  const HEROES = [
    { id: 'Ellis', name: 'Ellis Ransom', role: 'THE REVENANT', color: '#c8443a', height: 1.80, build: 'Medium',
      bio: "Shot on Ransom's Point by the Dunne Gang and raised by Sexton to bring the stolen embers home. Pa's lantern still burns at the hip.",
      powers: [
        { g: 'dash', name: 'Dust Devil', tag: 'WIND', ok: true, d: "Dash 9 m the way you're moving. Creatures in your path are flung aside." },
        { g: 'sundown', name: 'Sundown', tag: 'DUSK', d: 'For 6 s the world slows to a crawl round you, while you move and shoot at full speed.' },
        { g: 'borrowed', name: 'Borrowed Time', tag: 'DEBT', d: "For 8 s the damage you take goes in Sexton's ledger instead. You pay it back after, and every kill strikes some off." },
      ] },
    { id: 'Odessa', name: 'Odessa Lark', role: 'THE CARDSHARP', color: '#e0b44a', height: 1.72, build: 'Slight',
      bio: 'A riverboat gambler who caught Lucky Ned dealing seconds, and caught his bullet for it. Sexton bought her marker.',
      powers: [
        { g: 'luck', name: 'Lucky Streak', tag: 'FORTUNE', ok: true, d: 'For 8 s every shot ricochets into a second creature, and one in three is a critical hit.' },
        { g: 'pickup', name: 'Fifty-Two Pickup', tag: 'FORTUNE', d: 'Fling the whole deck: a cone of razor cards that cut whatever they meet.' },
        { g: 'double', name: 'Double or Nothing', tag: 'RISK', d: 'For 6 s every shot deals double damage or misses entirely, at even odds.' },
      ] },
    { id: 'Crane', name: 'Hollis Crane', role: 'THE UNPAID', color: '#6ee7ff', height: 1.95, build: 'Gaunt',
      bio: "Marshal of a town whose saint went dark, forty years walking. One eye still wears the ferryman's coin. Sexton promised him a fare.",
      powers: [
        { g: 'chain', name: 'Toll Chain', tag: 'DEBT', d: 'Hurl the chain: it hooks a creature and drags it to your feet, stunned.' },
        { g: 'ward', name: 'Grave Ward', tag: 'MERCY', d: 'Plant your lantern. For 8 s creatures in its light are slowed, and you mend inside it.' },
        { g: 'rites', name: 'Last Rites', tag: 'JUDGMENT', d: 'Mark a creature. If it dies within 8 s its soul bursts on its pack; if not, the mark strikes it hard.' },
      ] },
    { id: 'Gauge', name: 'Gauge', role: 'THE IRON HAND', color: '#ff8a1c', height: 1.92, build: 'Massive',
      bio: "The railroad's track-laying engine, woken in the wreck the gang left behind by a spark of ember sealed in its firebox.",
      powers: [
        { g: 'slag', name: 'Slag Bomb', tag: 'FORGE', ok: true, d: 'Lob a bomb from the firebox that bursts into a pool of burning slag.' },
        { g: 'hide', name: 'Iron Hide', tag: 'FORGE', d: 'Lock the plates: 60% less damage for 6 s at a slower pace, and biters burn their teeth.' },
        { g: 'spike', name: 'Rail Spike', tag: 'STEAM', d: 'Drive a spike from the forearm through every creature in a line.' },
      ] },
    { id: 'Pike', name: 'Wendell Pike', role: 'THE SURVEYOR', color: '#2f8f93', height: 1.68, build: 'Stocky',
      bio: 'He walked every mile the railroad will ever run, until the gang left him at the bottom of a ravine. He still knows the way.',
      powers: [
        { g: 'sight', name: 'Spyglass', tag: 'SIGHT', ok: true, d: 'Mark every creature within 60 m for 10 s, through walls. Marked ones take 30% more damage.' },
        { g: 'tripwire', name: 'Tripwire', tag: 'CRAFT', d: 'String a wire between two stakes; whatever crosses it trips, takes a hit and lies stunned.' },
        { g: 'longshot', name: 'Long Shot', tag: 'SIGHT', d: 'For 8 s your shots hit harder the farther they fly, and marked creatures are always crit.' },
      ] },
  ];

  // ================================================================ loading a hero (a skinned GLB from build.py)
  const X = new THREE.Vector3(1, 0, 0), Y = new THREE.Vector3(0, 1, 0), Z = new THREE.Vector3(0, 0, 1);
  const V = (x = 0, y = 0, z = 0) => new THREE.Vector3(x, y, z);
  const $id = (id) => document.getElementById(id);
  async function loadHero(name) {
    const [buf, info] = await Promise.all([fetchBinary(`models/Hero_${name}.json`), fetch(`models/Hero_${name}.info.json`).then((r) => r.json())]);
    const gltf = await loader.parseAsync(buf, '');
    const model = gltf.scene;
    let skinned = null;
    model.traverse((o) => { if (o.isSkinnedMesh) skinned = o; });
    skinned.material = MAT; skinned.castShadow = true; skinned.receiveShadow = true; skinned.frustumCulled = false;
    const bones = {}; for (const b of skinned.skeleton.bones) bones[b.name] = b;
    model.updateMatrixWorld(true);
    const rest = {}, restWorld = {}, restMat = {}, dir = {};
    for (const [n, b] of Object.entries(bones)) {
      rest[n] = b.quaternion.clone(); restWorld[n] = b.getWorldQuaternion(new THREE.Quaternion()); restMat[n] = b.matrixWorld.clone();
      const [h, t] = info.bones[n]; dir[n] = V(t[0] - h[0], t[1] - h[1], t[2] - h[2]).normalize();
    }
    const root = new THREE.Group(); root.add(model);
    return { name, root, model, skinned, bones, rest, restWorld, restMat, dir, info, yaw: 0, spin: 0 };
  }
  // A socket's point now: its rest point carried by its bone.
  function socketWorld(h, name) {
    const s = h.info.sockets[name]; if (!s) return null;
    const b = h.bones[s.bone], m = b.matrixWorld.clone().multiply(h.restMat[s.bone].clone().invert());
    return V(...s.pos).applyMatrix4(m);
  }

  // ================================================================ posing: turns about axes in the hero's own space (x its left, y up, z forward), down the chain
  function applyPose(h, rots) {
    for (const [n, b] of Object.entries(h.bones)) b.quaternion.copy(h.rest[n]);
    for (const [n, list] of Object.entries(rots)) {
      const b = h.bones[n]; if (!b) continue;
      const inv = h.restWorld[n].clone().invert();
      for (const [axis, ang] of list) b.quaternion.multiply(new THREE.Quaternion().setFromAxisAngle(axis.clone().applyQuaternion(inv).normalize(), ang));
    }
  }
  const add = (rots, bone, axis, ang) => { (rots[bone] = rots[bone] || []).push([axis, ang]); };
  function relaxed(h, rots, o = {}) {
    const down = o.down ?? 0.62, bend = o.bend ?? 0.22;
    for (const [s, sx] of [['l', 1], ['r', -1]]) {
      add(rots, `upperarm_${s}`, Z, -sx * down);
      add(rots, `upperarm_${s}`, X, -0.06);
      const d = h.dir[`lowerarm_${s}`], axis = d.clone().cross(Z).normalize();
      add(rots, `lowerarm_${s}`, axis, bend);
      const curl = { index: [0.22, 0.32, 0.26], middle: [0.28, 0.4, 0.3], ring: [0.34, 0.46, 0.32], pinky: [0.4, 0.52, 0.36] };
      const axis2 = Z.clone().multiplyScalar(-sx);
      for (const [f, angles] of Object.entries(curl)) angles.forEach((a, k) => add(rots, `${f}_0${k + 1}_${s}`, axis2, a * (o.curl ?? 1)));
      add(rots, `thumb_02_${s}`, axis2, 0.15); add(rots, `thumb_03_${s}`, axis2, 0.2);
    }
    return rots;
  }
  function walk(h, rots, phase, amt = 1) {
    const p = phase * Math.PI * 2, s1 = Math.sin(p);
    add(rots, 'thigh_l', X, -0.42 * s1 * amt); add(rots, 'thigh_r', X, 0.42 * s1 * amt);
    add(rots, 'calf_l', X, (0.55 * Math.max(0, -Math.sin(p + 0.9)) + 0.08) * amt); add(rots, 'calf_r', X, (0.55 * Math.max(0, Math.sin(p + 0.9)) + 0.08) * amt);
    add(rots, 'foot_l', X, -0.2 * s1 * amt); add(rots, 'foot_r', X, 0.2 * s1 * amt);
    add(rots, 'upperarm_l', X, 0.32 * s1 * amt); add(rots, 'upperarm_r', X, -0.32 * s1 * amt);
    add(rots, 'spine_03', Y, 0.08 * s1 * amt); add(rots, 'pelvis', Y, -0.06 * s1 * amt);
    return rots;
  }
  // Idle: breathing, a slow weight shift, the head looking about. Each hero breathes at their own pace.
  const PACE = { Ellis: 1.0, Odessa: 1.15, Crane: 0.7, Gauge: 0.85, Pike: 1.05 };
  function idle(h, rots, t, k) {
    const sp = PACE[h.name] || 1, ph = k * 1.7;
    const br = Math.sin(t * 1.6 * sp + ph), sh = Math.sin(t * 0.45 * sp + ph);
    add(rots, 'spine_03', X, 0.018 * br); add(rots, 'spine_05', X, 0.014 * br);
    add(rots, 'pelvis', Z, 0.025 * sh); add(rots, 'spine_02', Z, -0.03 * sh);
    add(rots, 'neck_01', Y, 0.12 * Math.sin(t * 0.33 * sp + ph * 2)); add(rots, 'head', X, -0.02 + 0.02 * Math.sin(t * 0.5 + ph));
    for (const [s, sx] of [['l', 1], ['r', -1]]) add(rots, `upperarm_${s}`, Z, -sx * 0.02 * br);
    if (h.name === 'Crane') add(rots, 'spine_04', X, 0.05);
    return rots;
  }

  // ================================================================ the stage
  const SLOTS = [[-2.7, -0.55], [-1.35, -0.15], [0, 0], [1.35, -0.15], [2.7, -0.55]];
  const LINEUP = { pos: V(0, 1.45, 8.9), look: V(0, 1.02, 0) };
  const G = { t: 0, heroes: [], sel: -1, mode: 'idle', cam: { pos: LINEUP.pos.clone(), look: LINEUP.look.clone(), fov: 34 }, want: null, drag: null, puffs: [] };
  function stage() {
    const ground = new THREE.Mesh(paint(new THREE.CircleGeometry(60, 72).rotateX(-Math.PI / 2), '#cdb995', 1), MAT);
    ground.receiveShadow = true; scene.add(ground);
    for (const [x, z] of SLOTS) {
      const disc = new THREE.Mesh(paint(new THREE.CylinderGeometry(0.62, 0.66, 0.06, 40), '#a99273', 1), MAT);
      disc.position.set(x, 0.03, z); disc.receiveShadow = true; scene.add(disc);
      const rim = new THREE.Mesh(paint(new THREE.TorusGeometry(0.64, 0.018, 6, 48).rotateX(Math.PI / 2), '#5ac8ff', 0.5), MAT);
      rim.position.set(x, 0.062, z); scene.add(rim);
    }
    sun.position.set(2.6, 4.6, 4.2).normalize().multiplyScalar(40); sun.target.position.set(0, 1, 0);
    Object.assign(sun.shadow.camera, { left: -6, right: 6, top: 6, bottom: -6, near: 20, far: 70 });
    sun.shadow.camera.updateProjectionMatrix(); sun.shadow.bias = -0.0004; sun.shadow.normalBias = 0.02;
    renderer.shadowMap.autoUpdate = true;
    scene.fog.near = 40; scene.fog.far = 260;
    camera.near = 0.05; camera.far = 1700; postMat.uniforms.uNear.value = camera.near; postMat.uniforms.uFar.value = camera.far;
  }
  // Smoke from Gauge's stacks: print-style puffs that rise, swell and shrink away.
  const puffGeo = paint(new THREE.IcosahedronGeometry(0.5, 1), '#f3eee4', 1);
  const puffMesh = new THREE.InstancedMesh(puffGeo, MAT, 80); puffMesh.count = 0; puffMesh.frustumCulled = false; scene.add(puffMesh);
  const tmpM = new THREE.Matrix4(), tmpQ = new THREE.Quaternion(), tmpS = V();
  function updatePuffs(dt) {
    const gauge = G.heroes.find((h) => h.name === 'Gauge');
    if (gauge && Math.random() < dt * 5.5) for (const s of ['Stack_l', 'Stack_r']) {
      const p = socketWorld(gauge, s); if (p && G.puffs.length < 78) G.puffs.push({ p, v: V((Math.random() - 0.5) * 0.08, 0.55 + Math.random() * 0.25, -0.05), age: 0, life: 1.8 + Math.random() * 0.8, s0: 0.06 + Math.random() * 0.03 });
    }
    let n = 0;
    for (let i = G.puffs.length - 1; i >= 0; i--) {
      const f = G.puffs[i]; f.age += dt; if (f.age > f.life) { G.puffs.splice(i, 1); continue; }
      f.p.addScaledVector(f.v, dt); f.v.x += 0.05 * dt;
      const k = f.age / f.life, s = f.s0 * (1 + k * 2.6) * (k > 0.7 ? 1 - (k - 0.7) / 0.3 : 1);
      puffMesh.setMatrixAt(n++, tmpM.compose(f.p, tmpQ, tmpS.setScalar(Math.max(0.001, s))));
    }
    puffMesh.count = n; puffMesh.instanceMatrix.needsUpdate = true;
  }

  // ================================================================ the camera: the lineup, or one hero up close
  const narrow = () => innerWidth < 860;
  function lineupCam() {
    const aspect = innerWidth / innerHeight, fitW = 7.4, dist = Math.max(8.9, (fitW / 2) / Math.tan((34 * DEG) / 2) / Math.max(0.5, aspect) * 1.05);
    return { pos: V(0, 1.45 + (dist - 8.9) * 0.06, dist), look: V(0, narrow() ? 0.8 : 1.02, 0), fov: 34 };
  }
  // The chosen hero steps forward off their stand, and the camera frames them: left of the panel on a wide screen, above it
  // on a narrow one.
  const STEP = 1.9;
  function focusCam(i) {
    const at = G.heroes[i].home.clone().add(V(0, 0, STEP)), H = HEROES[i].height, k = H / 1.8;
    if (narrow()) {
      // Above the panel: the hero's middle sits a quarter of the way down the screen.
      const d = 8.6 * k * Math.max(1, 0.5 / (innerWidth / innerHeight)), vis = 2 * d * Math.tan(17 * DEG);
      const look = at.clone().add(V(0, H * 0.5 - 0.24 * vis, 0));
      return { pos: look.clone().add(V(0, 0.3, d)), look, fov: 34 };
    }
    const aspect = innerWidth / innerHeight, d = 4.7 * k * Math.max(1, 1.3 / aspect), shift = 0.62 * k;
    const look = at.clone().add(V(shift, H * 0.5, 0));
    return { pos: look.clone().add(V(0.35 * k, 0.12, d)), look, fov: 28 };
  }

  // ================================================================ UI
  function glyphSvg(id) {
    return `<svg viewBox="-46 -46 92 92" aria-hidden="true"><circle r="38.5" fill="#0e1116"/><circle r="36.5" fill="url(#gmRing)"/><circle r="32.5" fill="rgba(7,26,40,0.92)"/>
      <circle r="33" fill="none" stroke="#5ac8ff" stroke-width="1" opacity="0.55"/><g transform="scale(0.92)">${GLYPH[id]}</g></svg>`;
  }
  function buildUI() {
    document.body.insertAdjacentHTML('beforeend', `<svg width="0" height="0" style="position:absolute" aria-hidden="true"><defs>
      <linearGradient id="gmRing" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#7d8894"/><stop offset=".5" stop-color="#3a424b"/><stop offset="1" stop-color="#1a1f25"/></linearGradient></defs></svg>`);
    const roster = $id('roster');
    HEROES.forEach((h, i) => {
      const b = document.createElement('button'); b.className = 'rcard'; b.style.setProperty('--c', h.color); b.setAttribute('aria-pressed', 'false');
      b.innerHTML = `<span class="k">${i + 1}</span><div class="n">${h.name.split(' ')[0].toUpperCase()}</div><div class="r">${h.role.replace('THE ', '')}</div>`;
      b.addEventListener('click', () => select(G.sel === i ? -1 : i));
      roster.appendChild(b);
    });
    document.querySelectorAll('.seg[data-mode]').forEach((b) => b.addEventListener('click', () => setMode(b.dataset.mode)));
    $id('allBtn').addEventListener('click', () => select(-1));
  }
  function fillPanel(i) {
    const h = HEROES[i], info = G.heroes[i].info;
    $id('pRole').textContent = h.role; $id('pName').textContent = h.name.toUpperCase(); $id('pBio').textContent = h.bio;
    $id('pFacts').textContent = `${h.height.toFixed(2)} m tall · ${h.build} build · ${info.tris.toLocaleString('en-US')} triangles · ${Object.keys(info.bones).length} bones`;
    $id('pPowers').innerHTML = h.powers.map((p) => `<div class="power">${glyphSvg(p.g)}<div class="pn">${p.name.toUpperCase()} <span class="tag">${p.tag}</span>${p.ok ? '<span class="ok">APPROVED</span>' : ''}</div><div class="pd">${p.d}</div></div>`).join('');
  }
  function select(i) {
    G.sel = i;
    document.querySelectorAll('.rcard').forEach((b, k) => b.setAttribute('aria-pressed', String(k === i)));
    document.body.classList.toggle('focus', i >= 0);
    if (i >= 0) { fillPanel(i); $id('panel').hidden = false; G.want = focusCam(i); }
    else { $id('panel').hidden = true; G.want = lineupCam(); setMode('idle'); }
  }
  function setMode(m) {
    G.mode = m;
    document.querySelectorAll('.seg[data-mode]').forEach((b) => b.setAttribute('aria-pressed', String(b.dataset.mode === m)));
  }

  // ================================================================ input: click a hero, drag to turn, keys 1-5
  const proj = V();
  function heroAt(x, y) {
    let best = -1, bd = 1e9;
    G.heroes.forEach((h, i) => {
      for (const f of [0.35, 0.6, 0.85]) {
        proj.copy(h.root.position).add(V(0, HEROES[i].height * f, 0)).project(camera);
        const sx = (proj.x * 0.5 + 0.5) * innerWidth, sy = (-proj.y * 0.5 + 0.5) * innerHeight, d = Math.hypot(sx - x, sy - y);
        if (d < bd) { bd = d; best = i; }
      }
    });
    return bd < Math.max(70, innerHeight * 0.12) ? best : -1;
  }
  canvas.addEventListener('pointerdown', (e) => { G.drag = { x: e.clientX, y: e.clientY, moved: 0, id: e.pointerId }; canvas.setPointerCapture(e.pointerId); canvas.classList.add('dragging'); });
  canvas.addEventListener('pointermove', (e) => {
    if (!G.drag || e.pointerId !== G.drag.id) return;
    const dx = e.clientX - G.drag.x; G.drag.x = e.clientX; G.drag.moved += Math.abs(dx);
    if (G.sel >= 0) G.heroes[G.sel].spin += dx * 0.012; else G.heroes.forEach((h) => { h.spin += dx * 0.008; });
  });
  const endDrag = (e) => {
    if (!G.drag || e.pointerId !== G.drag.id) return;
    canvas.classList.remove('dragging');
    if (G.drag.moved < 6) { const i = heroAt(e.clientX, e.clientY); if (i >= 0) select(i === G.sel ? -1 : i); }
    G.drag = null;
  };
  canvas.addEventListener('pointerup', endDrag); canvas.addEventListener('pointercancel', endDrag);
  window.addEventListener('keydown', (e) => {
    if (e.target && e.target.tagName === 'BUTTON' && (e.key === 'Enter' || e.key === ' ')) return;
    if (/^[1-5]$/.test(e.key)) select(+e.key - 1);
    else if (e.key === 'Escape') select(-1);
    else if (e.key === 'ArrowRight') select((G.sel + 1 + 5) % 5);
    else if (e.key === 'ArrowLeft') select(((G.sel < 0 ? 0 : G.sel) - 1 + 5) % 5);
  });
  window.addEventListener('resize', () => { resize(); G.want = G.sel >= 0 ? focusCam(G.sel) : lineupCam(); });

  // ================================================================ the frame
  let last = performance.now();
  function frame(now) {
    requestAnimationFrame(frame);
    const dt = clamp((now - last) / 1000, 0, 0.05); last = now; G.t += dt;
    G.heroes.forEach((h, i) => {
      // The chosen hero walks forward off their stand, the others back onto theirs.
      const target = i === G.sel ? h.home.clone().add(V(0, 0, STEP)) : h.home, to = target.clone().sub(h.root.position), d = to.length();
      const moving = d > 0.02;
      if (moving) h.root.position.addScaledVector(to, Math.min(1, (1.5 * dt) / d));
      h.stride = (h.stride || 0) + dt * 1.15;
      h.walkAmt = lerp(h.walkAmt || 0, moving ? 1 : 0, Math.min(1, dt * 6));
      // Everyone faces the camera, plus the turn the viewer gave them; a hero drifts back to facing over time.
      const face = Math.atan2(camera.position.x - h.root.position.x, camera.position.z - h.root.position.z);
      if (!G.drag) h.spin *= Math.max(0, 1 - dt * 0.35);
      h.root.rotation.y = face * (i === G.sel ? 1 : 0.75) + h.spin;
      const rots = {};
      const mode = i === G.sel ? G.mode : 'idle';
      if (mode !== 'apose') {
        relaxed(h, rots);
        idle(h, rots, G.t, i);
        if (mode === 'walk') walk(h, rots, (G.t * 0.9) % 1);
        else if (h.walkAmt > 0.01) walk(h, rots, h.stride % 1, h.walkAmt);
      }
      applyPose(h, rots);
    });
    if (G.want) {
      const k = Math.min(1, dt * 3.2); G.cam.pos.lerp(G.want.pos, k); G.cam.look.lerp(G.want.look, k);
      G.cam.fov = lerp(G.cam.fov, G.want.fov || 34, k);
      if (Math.abs(camera.fov - G.cam.fov) > 0.01) { camera.fov = G.cam.fov; camera.updateProjectionMatrix(); }
    }
    camera.position.copy(G.cam.pos); camera.lookAt(G.cam.look);
    updatePuffs(dt);
    sky.position.copy(camera.position); sky.material.uniforms.uTime.value = G.t; postMat.uniforms.uTime.value = G.t;
    renderFrame();
  }

  // ================================================================ boot
  async function boot() {
    camera.fov = 34; resize(); stage(); buildUI();
    let done = 0;
    const heroes = await Promise.all(HEROES.map((h) => loadHero(h.id).then((x) => { done++; $id('prog').style.width = (done / HEROES.length * 100) + '%'; return x; })));
    heroes.forEach((h, i) => { h.home = V(SLOTS[i][0], 0.06, SLOTS[i][1]); h.root.position.copy(h.home); scene.add(h.root); });
    G.heroes = heroes;
    G.want = lineupCam(); G.cam.pos.copy(G.want.pos); G.cam.look.copy(G.want.look);
    const params = new URLSearchParams(location.search);
    if (params.has('sel')) select(+params.get('sel'));
    if (params.has('mode')) setMode(params.get('mode'));
    if (G.sel >= 0) {   // a link straight to a hero starts with them already stepped forward
      G.heroes[G.sel].root.position.copy(G.heroes[G.sel].home).add(V(0, 0, STEP));
      G.cam.pos.copy(G.want.pos); G.cam.look.copy(G.want.look); G.cam.fov = G.want.fov; camera.fov = G.cam.fov; camera.updateProjectionMatrix();
    }
    $id('loading').classList.add('done');
    requestAnimationFrame(frame);
    window.__heroes = { ready: true, G, select, setMode, info: Object.fromEntries(heroes.map((h) => [h.name, h.info])) };
  }
  boot().catch((e) => { console.error(e); $id('loading').querySelector('.eyebrow').textContent = 'COULD NOT LOAD THE HEROES: ' + e.message; });
})();
