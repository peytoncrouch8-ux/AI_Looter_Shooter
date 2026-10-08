
// ================================================================ camera: orbiting from above, walking on foot, flying between views
const REDUCED_MOTION = !!(window.matchMedia && matchMedia('(prefers-reduced-motion: reduce)').matches);
const IS_TOUCH = 'ontouchstart' in window || (navigator.maxTouchPoints || 0) > 0;
const orbit = new OrbitControls(camera, canvas);
Object.assign(orbit, { enableDamping: true, dampingFactor: 0.09, minDistance: 3, maxDistance: 560, maxPolarAngle: 1.47,
  screenSpacePanning: false, zoomToCursor: true, rotateSpeed: 0.7, panSpeed: 0.9 });
orbit.target.set(0, 3, 0);
// Portrait screens get a wider lens from above too, and stand further back, so the subject fits across.
const aboveFov = () => (camera.aspect < 1 ? lerp(50, 64, clamp((1 - camera.aspect) / 0.55, 0, 1)) : 50);
// Portrait screens get a wider vertical field on foot, so the view is not a keyhole.
const footFov = () => clamp(68 / Math.sqrt(Math.min(1, camera.aspect)), 68, 88);
const walk = { on: false, yaw: 0, pitch: 0, pos: new THREE.Vector3(), keys: new Set(), stick: { x: 0, y: 0 }, h: 1.7 };
const forwardOf = (yaw) => new THREE.Vector3(-Math.sin(yaw), 0, Math.cos(yaw));
const rightOf = (yaw) => new THREE.Vector3(-Math.cos(yaw), 0, -Math.sin(yaw));
const camDir = () => camera.getWorldDirection(new THREE.Vector3());
const headingOf = (d) => Math.atan2(-d.x, d.z);   // the Unreal yaw (radians) of a three.js direction
const groundY = (x, z) => { const g = groundAt(x, z); return Number.isNaN(g) ? 0 : g; };
const nextFrame = () => new Promise((r) => requestAnimationFrame(() => requestAnimationFrame(r)));

function setWalking(on) {
  walk.on = on; orbit.enabled = !on;
  $('#mode').textContent = on ? 'Fly up' : 'Walk here';
  $('#stick').style.display = on && IS_TOUCH ? 'block' : 'none';
}
function applyWalkCamera() {
  camera.position.copy(walk.pos);
  camera.lookAt(tmpV2.copy(walk.pos).add(dir3(walk.yaw / DEG, walk.pitch / DEG)));
}
// A pose is where the camera stands and the point it looks at. From above, a view names its target (Unreal cm), the
// distance (m), the heading it looks along and how far down it looks; narrow screens back off so the subject fits.
function abovePose(v) {
  const x = toX(v.Y), z = toZ(v.X), target = new THREE.Vector3(x, v.Z !== undefined ? v.Z / 100 : groundY(x, z) + (v.h || 0), z);
  const boost = camera.aspect < 1.25 ? clamp(Math.pow(1.25 / camera.aspect, 0.85), 1, 3) : 1;
  return { kind: 'above', pos: target.clone().addScaledVector(dir3(v.yaw, -v.pitch), -v.dist * boost), look: target, fov: aboveFov() };
}
function footPose(v) {
  const x = toX(v.Y), z = toZ(v.X), pos = new THREE.Vector3(x, groundY(x, z) + walk.h, z), pitch = v.pitch ?? -3;
  return { kind: 'foot', pos, look: pos.clone().add(dir3(v.yaw, pitch).multiplyScalar(12)), fov: footFov(), yaw: v.yaw * DEG, pitch: pitch * DEG };
}
function applyPose(p) {
  camera.fov = p.fov; camera.updateProjectionMatrix();
  if (p.kind === 'foot') { walk.pos.copy(p.pos); walk.yaw = p.yaw; walk.pitch = p.pitch; setWalking(true); applyWalkCamera(); }
  else { setWalking(false); camera.position.copy(p.pos); orbit.target.copy(p.look); camera.lookAt(p.look); orbit.update(); }
}
let fly = null;
function flyTo(pose, instant = false) {
  const look = walk.on ? camera.position.clone().add(camDir().multiplyScalar(12)) : orbit.target.clone();
  if (instant || REDUCED_MOTION) { fly = null; applyPose(pose); return; }
  const d = camera.position.distanceTo(pose.pos) + look.distanceTo(pose.look) * 0.5;
  orbit.enabled = false;
  fly = { from: { pos: camera.position.clone(), look, fov: camera.fov }, to: pose, look: look.clone(), t0: performance.now(),
    dur: clamp(700 + d * 7, 1000, 2800), arc: Math.min(70, d * 0.22) };
}
function updateFly(now) {
  const k = clamp((now - fly.t0) / fly.dur, 0, 1), e = k < 0.5 ? 4 * k * k * k : 1 - Math.pow(-2 * k + 2, 3) / 2;
  camera.position.lerpVectors(fly.from.pos, fly.to.pos, e); camera.position.y += fly.arc * Math.sin(Math.PI * e);
  fly.look.lerpVectors(fly.from.look, fly.to.look, e); camera.lookAt(fly.look);
  camera.fov = lerp(fly.from.fov, fly.to.fov, e); camera.updateProjectionMatrix();
  if (k >= 1) { const p = fly.to; fly = null; applyPose(p); }
}
// A hand on the controls mid-flight stops there and orbits the point the camera was looking at.
function cancelFly() {
  if (!fly) return;
  const look = fly.look.clone(); fly = null;
  setWalking(false); orbit.target.copy(look); orbit.update();
}

// Mouse and touch: OrbitControls from above; on foot, drag to look and a thumbstick on touch screens.
let drag = null;
canvas.addEventListener('pointerdown', (e) => {
  userActed();
  if (!walk.on) return;
  drag = { id: e.pointerId, x: e.clientX, y: e.clientY }; canvas.setPointerCapture(e.pointerId);
});
canvas.addEventListener('pointermove', (e) => {
  if (!drag || e.pointerId !== drag.id || !walk.on) return;
  const k = e.pointerType === 'touch' ? 0.006 : 0.0042, dx = e.clientX - drag.x, dy = e.clientY - drag.y;
  drag.x = e.clientX; drag.y = e.clientY;
  walk.yaw += dx * k; walk.pitch = clamp(walk.pitch - dy * k, -1.35, 1.35);
});
const endDrag = (e) => { if (drag && e.pointerId === drag.id) drag = null; };
canvas.addEventListener('pointerup', endDrag); canvas.addEventListener('pointercancel', endDrag);
canvas.addEventListener('wheel', () => userActed(), { passive: true });
// Double-click a place to swoop the orbit onto it, or to walk to it on foot.
canvas.addEventListener('dblclick', (e) => {
  const r = canvas.getBoundingClientRect(), hit = groundHit(((e.clientX - r.left) / r.width) * 2 - 1, -((e.clientY - r.top) / r.height) * 2 + 1);
  if (!hit) return;
  const { X, Y } = uePos(hit.x, hit.z);
  if (walk.on) { flyTo(footPose({ X, Y, yaw: walk.yaw / DEG, pitch: walk.pitch / DEG })); return; }
  const off = camera.position.clone().sub(orbit.target).multiplyScalar(0.6);
  if (off.length() < 12) off.setLength(12);
  flyTo({ kind: 'above', pos: hit.clone().add(off), look: hit, fov: aboveFov() });
});
function groundHit(nx, ny) {
  const ray = new THREE.Raycaster(); ray.setFromCamera(new THREE.Vector2(nx, ny), camera);
  const o = ray.ray.origin, d = ray.ray.direction, p = new THREE.Vector3();
  for (let s = 0.5; s < 1500; s += 0.4 + s * 0.012) { p.copy(o).addScaledVector(d, s); const g = groundAt(p.x, p.z); if (!Number.isNaN(g) && p.y <= g) return p.setY(g); }
  return null;
}
const MOVE_KEYS = new Set(['KeyW', 'KeyA', 'KeyS', 'KeyD', 'ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight']);
window.addEventListener('keydown', (e) => {
  if (e.target.closest && e.target.closest('input, textarea, select')) return;
  if (compareOpen) { if (e.key === 'Escape') closeCompare(); return; }
  if (e.ctrlKey || e.metaKey || e.altKey) return;
  if (e.key >= '1' && e.key <= '4') { selectConcept(+e.key - 1); return; }
  if (e.key === 'Escape') { stopTour(); return; }
  walk.keys.add(e.code);
  if (MOVE_KEYS.has(e.code)) { userActed(); e.preventDefault(); }
});
window.addEventListener('keyup', (e) => walk.keys.delete(e.code));
window.addEventListener('blur', () => walk.keys.clear());
const stick = $('#stick'), stickBase = stick.querySelector('.base'), stickKnob = stick.querySelector('.knob');
let stickId = null, stickOrigin = null;
stick.addEventListener('pointerdown', (e) => {
  userActed(); stickId = e.pointerId; stickOrigin = { x: e.clientX, y: e.clientY }; stick.setPointerCapture(e.pointerId);
  for (const el of [stickBase, stickKnob]) { el.style.display = 'block'; el.style.left = e.clientX + 'px'; el.style.top = e.clientY + 'px'; }
});
stick.addEventListener('pointermove', (e) => {
  if (e.pointerId !== stickId) return;
  const dx = e.clientX - stickOrigin.x, dy = e.clientY - stickOrigin.y, l = Math.hypot(dx, dy), m = Math.min(l, 50), nx = l ? dx / l : 0, ny = l ? dy / l : 0;
  walk.stick.x = nx * m / 50; walk.stick.y = -ny * m / 50;
  stickKnob.style.left = (stickOrigin.x + nx * m) + 'px'; stickKnob.style.top = (stickOrigin.y + ny * m) + 'px';
});
const stickEnd = (e) => { if (e.pointerId !== stickId) return; stickId = null; walk.stick.x = walk.stick.y = 0; stickBase.style.display = stickKnob.style.display = 'none'; };
stick.addEventListener('pointerup', stickEnd); stick.addEventListener('pointercancel', stickEnd);

function moveInput() {
  const k = walk.keys; let mx = walk.stick.x, mz = walk.stick.y;
  if (k.has('KeyW') || k.has('ArrowUp')) mz += 1;
  if (k.has('KeyS') || k.has('ArrowDown')) mz -= 1;
  if (k.has('KeyD') || k.has('ArrowRight')) mx += 1;
  if (k.has('KeyA') || k.has('ArrowLeft')) mx -= 1;
  const l = Math.hypot(mx, mz); if (l > 1) { mx /= l; mz /= l; }
  return { mx, mz, run: k.has('ShiftLeft') || k.has('ShiftRight') };
}
function updateWalk(dt) {
  const { mx, mz, run } = moveInput(), speed = run ? 9 : 4.5;
  const step = forwardOf(walk.yaw).multiplyScalar(mz).add(rightOf(walk.yaw).multiplyScalar(mx)).multiplyScalar(speed * dt);
  if (step.lengthSq() > 0) { const nx = walk.pos.x + step.x, nz = walk.pos.z + step.z; if (!Number.isNaN(groundAt(nx, nz))) { walk.pos.x = nx; walk.pos.z = nz; } }
  const g = groundAt(walk.pos.x, walk.pos.z); if (!Number.isNaN(g)) walk.pos.y = lerp(walk.pos.y, g + walk.h, Math.min(1, dt * 8));
  applyWalkCamera();
}
// From above, the movement keys slide the view over the island, faster the higher it is.
function updateOrbitKeys(dt) {
  const { mx, mz, run } = moveInput(); if (!mx && !mz) return;
  const yaw = headingOf(camDir()), speed = Math.max(10, camera.position.distanceTo(orbit.target) * 0.9) * (run ? 2 : 1);
  const step = forwardOf(yaw).multiplyScalar(mz).add(rightOf(yaw).multiplyScalar(mx)).multiplyScalar(speed * dt);
  orbit.target.add(step); camera.position.add(step);
  const g = groundAt(orbit.target.x, orbit.target.z); if (!Number.isNaN(g)) { const dy = (g - orbit.target.y) * Math.min(1, dt * 4); orbit.target.y += dy; camera.position.y += dy; }
}
function keepAboveGround() {
  orbit.target.x = clamp(orbit.target.x, -150, 150); orbit.target.z = clamp(orbit.target.z, -150, 150);
  const g = groundAt(camera.position.x, camera.position.z), floor = (Number.isNaN(g) ? -80 : g) + 1.4;
  if (camera.position.y < floor) camera.position.y = floor;
}

// ================================================================ concepts: built once, then switched by visibility
const LAYERS = [], BUILDING = new Map();
let current = null;
function layerFor(i) {
  if (!BUILDING.has(i)) BUILDING.set(i, buildLayer(i).then((L) => (LAYERS[i] = L)));
  return BUILDING.get(i);
}
function showLayer(L) {
  for (const x of LAYERS) if (x) x.group.visible = x === L;
  current = L; renderer.shadowMap.needsUpdate = true;
  showText(L.S); buildLabels(L.S); drawMapBase(L.S);
}
let switching = null;
async function selectConcept(i) {
  if (current && current.index === i) return;
  if (switching !== null) return;
  switching = i;
  try {
    if (!LAYERS[i]) { toast(`Building ${CONCEPT_NAMES[i]}…`, 60000); await nextFrame(); }
    showLayer(await layerFor(i));
    toast(`${i + 1} · ${CONCEPT_NAMES[i]}`, 1400);
    try { localStorage.setItem('skyreach.concept', String(i)); } catch (e) { /* storage is optional */ }
  } finally { switching = null; }
}

// ================================================================ the panels: tabs, text, views, tools
const tabsEl = $('#tabs');
CONCEPT_NAMES.forEach((name, i) => {
  const b = document.createElement('button'); b.className = 'tab'; b.setAttribute('aria-pressed', 'false');
  const n = document.createElement('b'); n.textContent = String(i + 1); b.append(n, name);
  b.addEventListener('click', () => selectConcept(i)); tabsEl.appendChild(b);
});
function showText(S) {
  [...tabsEl.children].forEach((b, j) => b.setAttribute('aria-pressed', String(j === current.index)));
  $('#name').textContent = S.text.name; $('#tagline').textContent = S.text.tagline;
  $('#bullets').innerHTML = S.text.bullets.map((t) => `<li>${t}</li>`).join('');
  for (const kind of ['above', 'foot']) {
    const row = $('#' + kind); row.innerHTML = '';
    for (const v of S.views[kind]) {
      const b = document.createElement('button'); b.className = 'chip'; b.textContent = v.name; b.dataset.kind = kind; b.dataset.name = v.name;
      b.addEventListener('click', () => { stopTour(); goView(kind, v.name); });
      row.appendChild(b);
    }
  }
}
function goView(kind, name, instant = false) {
  const v = current && current.S.views[kind].find((x) => x.name === name);
  if (!v) return false;
  flyTo(kind === 'above' ? abovePose(v) : footPose(v), instant);
  markChip(kind, name);
  return true;
}
function markChip(kind, name) {
  for (const b of document.querySelectorAll('#above .chip, #foot .chip')) b.classList.toggle('on', b.dataset.kind === kind && b.dataset.name === name);
}
let hintTimer = 0;
function userActed() {
  stopTour(); cancelFly(); markChip(null);
  if (!hintTimer) hintTimer = setTimeout(() => { $('#hint').style.display = 'none'; }, 6000);
}
$('#fold').addEventListener('click', (e) => {
  const list = $('#bullets'), open = list.hidden; list.hidden = !open; document.body.classList.toggle('details', open); panelAge = 99;
  e.currentTarget.setAttribute('aria-expanded', String(open)); e.currentTarget.textContent = open ? 'Less' : 'Details';
});
$('#mode').addEventListener('click', () => {
  stopTour(); markChip(null);
  if (walk.on) { const { X, Y } = uePos(walk.pos.x, walk.pos.z); flyTo(abovePose({ X, Y, dist: 60, yaw: walk.yaw / DEG, pitch: 45, h: 0 })); return; }
  // Stand a few steps short of the point the view is centered on, looking at it.
  const yaw = headingOf(camDir()), p = orbit.target.clone().addScaledVector(forwardOf(yaw), -10), { X, Y } = uePos(p.x, p.z);
  if (Number.isNaN(groundUE(X, Y))) { toast('Center the view over the island first'); return; }
  flyTo(footPose({ X, Y, yaw: yaw / DEG, pitch: -3 }));
});
let labelsOn = true;
$('#labelsBtn').addEventListener('click', (e) => { labelsOn = !labelsOn; e.currentTarget.setAttribute('aria-pressed', String(labelsOn)); });
let toastTimer = 0;
function toast(msg, ms = 2200) {
  const t = $('#toast'); t.textContent = msg; t.style.display = 'block';
  clearTimeout(toastTimer); toastTimer = setTimeout(() => { t.style.display = 'none'; }, ms);
}

// ================================================================ the tour: every view of the concept in turn, drifting while it holds
let tour = null;
function startTour() {
  const S = current.S;
  tour = { stops: [...S.views.above.map((v) => ['above', v.name]), ...S.views.foot.map((v) => ['foot', v.name])], i: -1, next: 0 };
  $('#tour').textContent = 'Stop';
}
function stopTour() { if (!tour) return; tour = null; $('#tour').textContent = 'Tour'; }
$('#tour').addEventListener('click', () => { if (tour) stopTour(); else startTour(); });
function updateTour(now, dt) {
  if (!tour || fly) return;
  if (now < tour.next) {
    if (REDUCED_MOTION) return;
    if (walk.on) { walk.yaw += dt * 0.06; applyWalkCamera(); }
    else { const off = camera.position.clone().sub(orbit.target).applyAxisAngle(UP, dt * 0.06); camera.position.copy(orbit.target).add(off); }
    return;
  }
  tour.i++;
  if (tour.i >= tour.stops.length) { stopTour(); toast('That was every view of this concept'); return; }
  const [kind, name] = tour.stops[tour.i];
  goView(kind, name);
  tour.next = now + (fly ? fly.dur : 0) + 3400;
}

// ================================================================ compare: the same view in all four concepts
let compareOpen = false;
async function openCompare() {
  if (compareOpen) return;
  stopTour();
  toast('Rendering the four concepts…', 60000); await nextFrame();
  const keep = current, shots = [];
  for (let i = 0; i < CONCEPTS.length; i++) {
    const L = await layerFor(i);
    for (const x of LAYERS) if (x) x.group.visible = x === L;
    renderer.shadowMap.needsUpdate = true;
    animate(L, clockT); renderFrame();
    shots.push(canvas.toDataURL('image/jpeg', 0.88));
  }
  for (const x of LAYERS) if (x) x.group.visible = x === keep;
  renderer.shadowMap.needsUpdate = true;
  const grid = $('#compareGrid'); grid.innerHTML = '';
  shots.forEach((src, i) => {
    const f = document.createElement('figure'), img = document.createElement('img'), cap = document.createElement('figcaption');
    img.src = src; img.alt = `${CONCEPT_NAMES[i]} from this viewpoint`; cap.textContent = `${i + 1} · ${CONCEPT_NAMES[i]}`;
    f.append(img, cap); f.tabIndex = 0; f.setAttribute('role', 'button');
    const pick = () => { closeCompare(); selectConcept(i); };
    f.addEventListener('click', pick); f.addEventListener('keydown', (e) => { if (e.key === 'Enter' || e.key === ' ') { e.preventDefault(); pick(); } });
    grid.appendChild(f);
  });
  $('#toast').style.display = 'none';
  $('#compare').hidden = false; compareOpen = true; $('#compareClose').focus();
}
function closeCompare() { $('#compare').hidden = true; compareOpen = false; }
$('#compareBtn').addEventListener('click', openCompare);
$('#compareClose').addEventListener('click', closeCompare);

// ================================================================ labels over the places that matter, decluttered each frame
const labelsEl = $('#labels');
let labelList = [];
function buildLabels(S) {
  labelsEl.innerHTML = '';
  labelList = S.labels.map((l) => {
    const el = document.createElement('div'), span = document.createElement('span');
    el.className = 'lbl' + (l.kind ? ' ' + l.kind : ''); span.textContent = l.text; el.append(span, document.createElement('i'));
    labelsEl.appendChild(el);
    const x = toX(l.Y), z = toZ(l.X);
    return { el, p: new THREE.Vector3(x, groundY(x, z) + l.h, z), w: 0, h: 0, prio: l.kind ? 1 : 0 };
  });
}
// The panels count as taken space, so no label hides under one (their rects are re-read twice a second).
let panelRects = [], panelAge = 99;
function panelSpace() {
  if (++panelAge < 30) return panelRects;
  panelAge = 0;
  panelRects = [...document.querySelectorAll('.panel')].map((el) => el.getBoundingClientRect()).filter((r) => r.width > 0)
    .map((r) => ({ x0: r.left, x1: r.right, y0: r.top, y1: r.bottom }));
  return panelRects;
}
function updateLabels() {
  if (!labelsOn) { labelsEl.style.display = 'none'; return; }
  labelsEl.style.display = '';
  const W = canvas.clientWidth, H = canvas.clientHeight, placed = [...panelSpace()];
  const list = labelList.map((L) => {
    tmpV.copy(L.p).project(camera);
    return { L, x: (tmpV.x * 0.5 + 0.5) * W, y: (-tmpV.y * 0.5 + 0.5) * H, z: tmpV.z, dist: camera.position.distanceTo(L.p) };
  }).sort((a, b) => (b.L.prio - a.L.prio) || (a.dist - b.dist));
  for (const e of list) {
    const { L } = e;
    let show = e.z < 1 && e.y > 20 && e.y < H + 40 && e.dist > 6 && !(walk.on && e.dist > 90) && !fly;
    if (show) {
      if (!L.w) { L.el.style.display = ''; L.w = L.el.offsetWidth || 120; L.h = L.el.offsetHeight || 34; }
      if (e.x - L.w / 2 < 2 || e.x + L.w / 2 > W - 2) show = false;
    }
    if (show) {
      const r = { x0: e.x - L.w / 2 - 4, x1: e.x + L.w / 2 + 4, y0: e.y - L.h - 2, y1: e.y + 2 };
      if (placed.some((q) => r.x0 < q.x1 && r.x1 > q.x0 && r.y0 < q.y1 && r.y1 > q.y0)) show = false; else placed.push(r);
    }
    L.el.style.display = show ? '' : 'none';
    if (show) L.el.style.transform = `translate(${e.x.toFixed(1)}px, ${e.y.toFixed(1)}px) translate(-50%, -100%)`;
  }
}

// ================================================================ the minimap: north up, tap to walk there
const map = $('#map'), mctx = map.getContext('2d'), mapBase = document.createElement('canvas');
mapBase.width = map.width; mapBase.height = map.height;
const MAP_B = (() => {
  const xs = OUTLINE.map((p) => p[0]), ys = OUTLINE.map((p) => p[1]);
  return { cx: (Math.min(...xs) + Math.max(...xs)) / 2, cy: (Math.min(...ys) + Math.max(...ys)) / 2,
    half: Math.max(Math.max(...xs) - Math.min(...xs), Math.max(...ys) - Math.min(...ys)) / 2 * 1.05 };
})();
const mapPt = (X, Y) => [((Y - MAP_B.cy) / MAP_B.half * 0.5 + 0.5) * map.width, ((MAP_B.cx - X) / MAP_B.half * 0.5 + 0.5) * map.height];
const mapToUE = (u, v) => ({ X: MAP_B.cx - (v - 0.5) * 2 * MAP_B.half, Y: MAP_B.cy + (u - 0.5) * 2 * MAP_B.half });
const MAP_ROAD = { brick: '#d08f6e', plank: '#c9a46a', dirt: '#d6bf95', path: '#e0cfaa' };
const ROCKY = /^(Boulder|Outcrop|Cairn|Rock_)/;
function drawMapBase(S) {
  const c = mapBase.getContext('2d'), w = mapBase.width, px = w / (2 * MAP_B.half);
  c.clearRect(0, 0, w, w);
  const path = (pts, close = true) => { c.beginPath(); pts.forEach(([X, Y], i) => { const [x, y] = mapPt(X, Y); if (i) c.lineTo(x, y); else c.moveTo(x, y); }); if (close) c.closePath(); };
  path(OUTLINE); c.fillStyle = '#5b6a3b'; c.fill(); c.strokeStyle = '#d7ccb4'; c.lineWidth = 2.5; c.stroke();
  path(PLATEAU.polygon); c.fillStyle = '#737157'; c.fill();
  for (const p of S.patches) { path(p.poly); c.fillStyle = p.color; c.fill(); }
  for (const f of S.fields) { path(f.poly); c.fillStyle = (FIELD_LOOK[f.kind] || FIELD_LOOK.pasture)[0]; c.fill(); }
  c.fillStyle = '#9ccfd3';
  const [pcx, pcy] = mapPt(POND.center[0], POND.center[1]);
  c.beginPath(); c.ellipse(pcx, pcy, POND.radii[1] * px, POND.radii[0] * px, 0, 0, Math.PI * 2); c.fill();
  for (const wtr of S.water) { const [x, y] = mapPt(wtr.X, wtr.Y); c.beginPath(); c.arc(x, y, Math.max(1.5, wtr.r * px), 0, Math.PI * 2); c.fill(); }
  path(CREEK.path, false); c.strokeStyle = '#9ccfd3'; c.lineWidth = 3; c.lineCap = 'round'; c.lineJoin = 'round'; c.stroke();
  for (const r of S.roads) { path(r.points, false); c.strokeStyle = MAP_ROAD[r.kind] || MAP_ROAD.dirt; c.lineWidth = Math.max(1.5, r.width * px); c.stroke(); }
  // trees as dots, buildings and rocks as their footprints
  c.fillStyle = '#2f3d22';
  for (const it of S.items) {
    const m = String(it.model);
    if (/^k:(oak|birch|pine|apple|deadpine)/.test(m)) { const [x, y] = mapPt(it.X, it.Y); c.beginPath(); c.arc(x, y, /^k:(oak|pine)/.test(m) ? 2.2 : 1.6, 0, 6.283); c.fill(); }
  }
  for (const it of S.items) {
    const m = String(it.model); if (m.startsWith('k:') || /^(CliffFace|SkyIsland|DeadTree|Fence|Stone|LilyPads|Reeds|Pebble)/.test(m)) continue;
    const b = modelBounds(m); if (!b) continue;
    const s = Array.isArray(it.scale) ? it.scale : [it.scale || 1, 1, it.scale || 1];
    if (Math.max((b.max[0] - b.min[0]) * s[0], (b.max[2] - b.min[2]) * s[2]) < 1.7) continue;
    const corners = [[b.min[0], b.min[2]], [b.max[0], b.min[2]], [b.max[0], b.max[2]], [b.min[0], b.max[2]]]
      .map(([mx, mz]) => local(it.X, it.Y, it.yaw || 0, mz * 100 * s[2], -mx * 100 * s[0]));
    path(corners); c.fillStyle = ROCKY.test(m) ? '#a39e90' : '#f1e8d6'; c.fill();
    if (!ROCKY.test(m)) { c.strokeStyle = '#2a2024'; c.lineWidth = 1; c.stroke(); }
  }
  for (const [kind, color] of [['slime', '#7fd494'], ['spider', '#d79be8']]) {
    const list = S.creatures.filter((cr) => cr.kind === kind); if (!list.length) continue;
    const X = list.reduce((a, cr) => a + cr.X, 0) / list.length, Y = list.reduce((a, cr) => a + cr.Y, 0) / list.length;
    const r = Math.max(...list.map((cr) => Math.hypot(cr.X - X, cr.Y - Y))) + 500, [x, y] = mapPt(X, Y);
    c.beginPath(); c.arc(x, y, r * px, 0, 6.283); c.strokeStyle = color; c.lineWidth = 3; c.setLineDash([6, 4]); c.stroke(); c.setLineDash([]);
  }
  const spawn = S.views.foot.find((v) => v.name === 'Spawn');
  if (spawn) { const [x, y] = mapPt(spawn.X, spawn.Y); c.beginPath(); c.arc(x, y, 6, 0, 6.283); c.fillStyle = '#ff9640'; c.fill(); c.strokeStyle = '#1a1208'; c.lineWidth = 2; c.stroke(); }
}
function drawMap() {
  const w = map.width; mctx.clearRect(0, 0, w, w); mctx.drawImage(mapBase, 0, 0);
  const { X, Y } = uePos(camera.position.x, camera.position.z);
  let [x, y] = mapPt(X, Y); x = clamp(x, 8, w - 8); y = clamp(y, 8, w - 8);
  mctx.save(); mctx.translate(x, y); mctx.rotate(headingOf(camDir()));
  mctx.beginPath(); mctx.moveTo(0, 0); mctx.arc(0, 0, 46, -Math.PI / 2 - 0.45, -Math.PI / 2 + 0.45); mctx.closePath();
  mctx.fillStyle = 'rgba(57,211,224,.22)'; mctx.fill();
  mctx.beginPath(); mctx.moveTo(0, -11); mctx.lineTo(7, 7); mctx.lineTo(0, 3); mctx.lineTo(-7, 7); mctx.closePath();
  mctx.fillStyle = '#39d3e0'; mctx.fill(); mctx.strokeStyle = '#0e1116'; mctx.lineWidth = 1.5; mctx.stroke();
  mctx.restore();
}
map.addEventListener('click', (e) => {
  const r = map.getBoundingClientRect(), { X, Y } = mapToUE((e.clientX - r.left) / r.width, (e.clientY - r.top) / r.height);
  if (Number.isNaN(groundUE(X, Y))) { toast('That spot is off the island'); return; }
  stopTour(); markChip(null);
  flyTo(footPose({ X, Y, yaw: headingOf(camDir()) / DEG, pitch: -3 }));
});

// ================================================================ the frame: fog by height, animation, render, labels, map, quality
// Fog thins as the camera climbs: on foot it gives depth, from above it would only wash the island out.
function updateFog() {
  const h = camera.position.y - groundY(camera.position.x, camera.position.z), t = smooth01((h - 4) / 110);
  scene.fog.near = lerp(60, 520, t); scene.fog.far = lerp(420, 1700, t);
}
// Resolution steps down when frames run long (never below 0.75), and back up after a long run of fast ones.
const AUTO_QUALITY = !navigator.webdriver, MAX_RATIO = pixelRatio;
let qT = 0, qN = 0, qGood = 0;
function adaptQuality(dt) {
  if (!AUTO_QUALITY) return;
  qT += dt; qN++; if (qT < 2.5) return;
  const avg = qT / qN; qT = 0; qN = 0;
  if (avg > 1 / 24 && pixelRatio > 0.8) { pixelRatio = Math.max(0.75, pixelRatio - 0.25); qGood = 0; resize(); }
  else if (avg < 1 / 55 && pixelRatio < MAX_RATIO) { if (++qGood > 3) { qGood = 0; pixelRatio = Math.min(MAX_RATIO, pixelRatio + 0.25); resize(); } }
  else qGood = 0;
}
const T0 = performance.now();
let lastT = T0, clockT = 0, mapTimer = 0;
function frame(now) {
  requestAnimationFrame(frame);
  const dt = clamp((now - lastT) / 1000, 0, 0.05); lastT = now; clockT = (now - T0) / 1000;
  if (compareOpen) return;
  updateTour(now, dt);
  if (fly) updateFly(now); else if (walk.on) updateWalk(dt); else { updateOrbitKeys(dt); orbit.update(); keepAboveGround(); }
  sky.position.copy(camera.position); sky.material.uniforms.uTime.value = clockT;
  updateFog();
  animate(current, clockT); animateBase(clockT, dt);
  renderFrame();
  updateLabels();
  if ((mapTimer += dt) > 0.1) { mapTimer = 0; drawMap(); }
  adaptQuality(dt);
}
window.addEventListener('resize', () => { resize(); if (walk.on && !fly) { camera.fov = footFov(); camera.updateProjectionMatrix(); } });

// ================================================================ boot
let readyResolve;
const READY = new Promise((r) => { readyResolve = r; });
async function boot() {
  resize();
  const progress = (f, text) => { $('#prog').style.width = Math.round(f * 100) + '%'; if (text) $('#progtext').textContent = text; };
  progress(0.04, 'Reading the island heights…');
  const hm = MANIFEST.terrain.heights;
  const buf = await fetchBinary(hm.file);
  HM = { n: hm.n, half: hm.half, data: new Float32Array(buf) };
  progress(0.1, 'Loading the terrain, cliffs and models…');
  await buildBase((f) => progress(0.1 + f * 0.5));
  let start = 0;
  try { start = clamp(parseInt(localStorage.getItem('skyreach.concept') || '0', 10) || 0, 0, CONCEPTS.length - 1); } catch (e) { start = 0; }
  const fromHash = parseInt((location.hash || '').replace(/\D/g, ''), 10);
  if (fromHash >= 1 && fromHash <= CONCEPTS.length) start = fromHash - 1;
  progress(0.65, `Placing ${CONCEPT_NAMES[start]}…`); await nextFrame();
  showLayer(await layerFor(start));
  progress(1, 'Ready');
  goView('above', 'Island', true);
  $('#loading').style.display = 'none';
  requestAnimationFrame(frame);
  readyResolve();
}
// For automated screenshots: switch concepts, jump to a view, hide the panels, read the frame.
window.__viewer = {
  ready: READY,
  selectConcept: async (i) => { await selectConcept(i); await nextFrame(); },
  view: (kind, name) => goView(kind, name, true),
  pose: (kind, v) => flyTo(kind === 'above' ? abovePose(v) : footPose(v), true),
  ui: (on) => document.body.classList.toggle('clean', !on),
  labels: (on) => { labelsOn = on; $('#labelsBtn').setAttribute('aria-pressed', String(on)); },
  state: () => ({ concept: current && current.index, walking: walk.on, flying: !!fly, ratio: pixelRatio, views: current && current.S.views, counts: current && current.group.children.length }),
  snapshot: () => { renderFrame(); return canvas.toDataURL('image/png'); },
};
boot().catch((e) => { console.error(e); $('#progtext').textContent = 'Could not load the island: ' + e.message; });
