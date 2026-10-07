
// ================================================================ the island's fixed places (Unreal cm)
const POND = COMPUTED.pond;
const CREEK = LAYOUT.features.find((f) => f.id === 'creek');
const PLATEAU = LAYOUT.features.find((f) => f.id === 'plateau');
const RANGE_ZONE = LAYOUT.zones.find((z) => z.id === 'range').polygon;
const FOREST_ZONE = LAYOUT.zones.find((z) => z.id === 'forest').polygon;
const OUTLINE = LAYOUT.outline;
const INNER = OUTLINE.map(([x, y]) => [x * 0.9, y * 0.9]);
const RAMP = COMPUTED.ramp.points.map((p) => [p[0], p[1]]);
const MAIN_ROAD = [[-5350, -3750], [-3600, -2000], [-1800, -900], [0, 0], [1900, -500], [3600, -900], [4600, -1200]];
const FARM_ROAD = MAIN_ROAD.slice(0, 4);
const RANGE_ROAD = MAIN_ROAD.slice(3);
const FOREST_ROAD = [[0, 0], [1400, 1800], [2800, 3200], [3700, 4400], [4050, 5200], [4080, 6000], [4250, 6800], [4600, 7650]];
const PLATEAU_PATH = [[0, 0], [0, 1400], [-1200, 2700], [-2600, 2600]];
const WINDMILL_PATH = [[3600, -900], [4000, -2600], [5100, -3400], [5750, -3650]];
const ORCHARD_PATH = [[-5000, -4500], [-4550, -5500], [-4800, -6500], [-5400, -7300]];
const FARMYARD = [[-6500, -5050], [-6450, -2700], [-4200, -2800], [-4100, -4700], [-4900, -5000]];
// The jetty stands where the plateau's rim faces south-east past the lookout (between the two rim points nearest it).
const JETTY = (() => {
  const rim = (COMPUTED.cliffs.rim || []).filter((p) => p.plateau);
  const near = rim.map((p) => ({ p, d: Math.hypot(p.location[0] + 6600, p.location[1] - 7300) })).sort((a, b) => a.d - b.d).slice(0, 2).map((e) => e.p);
  if (near.length < 2) return { X: -6523, Y: 7372, yaw: 121.5, Z: 844 };
  const X = (near[0].location[0] + near[1].location[0]) / 2, Y = (near[0].location[1] + near[1].location[1]) / 2, yaw = (near[0].yaw + near[1].yaw) / 2;
  const [x, y] = local(X, Y, yaw, -60, 0);
  return { X: x, Y: y, yaw, Z: (near[0].location[2] + near[1].location[2]) / 2 };
})();
const TOWER_DOOR = local(-6200, 6000, 45, 420, 0);

// ================================================================ placement helpers
function pointAt(pts, t) {
  let acc = 0;
  for (let i = 0; i < pts.length - 1; i++) {
    const [ax, ay] = pts[i], [bx, by] = pts[i + 1], len = Math.hypot(bx - ax, by - ay);
    if (acc + len >= t || i === pts.length - 2) { const k = clamp((t - acc) / (len || 1), 0, 1); return [ax + (bx - ax) * k, ay + (by - ay) * k, ueYawOf(bx - ax, by - ay)]; }
    acc += len;
  }
  return [pts[0][0], pts[0][1], 0];
}
// A house beside a road, facing it: t cm along the road, on its right (side +1) or left (-1), its front setback cm off
// the road's centre line.
function besideRoad(S, pts, t, side, model, setback, o = {}) {
  const [X, Y, yaw] = pointAt(pts, t);
  const b = modelBounds(model), front = b ? b.max[2] * 100 * (o.scale || 1) : 300;
  const [hx, hy] = local(X, Y, yaw, 0, side * (setback + front));
  const faceYaw = yaw + (side > 0 ? -90 : 90);
  if (o.plain) S.items.push(bldg(model, hx, hy, faceYaw, { scale: o.scale })); else house(S, model, hx, hy, faceYaw, o);
  return [hx, hy, faceYaw];
}
// A building on a square's edge facing its centre: at angle theta (Unreal yaw from the centre), its front dFront cm out.
function aroundSquare(S, cx, cy, model, theta, dFront, o = {}) {
  const b = modelBounds(model), front = b ? b.max[2] * 100 * (o.scale || 1) : 300;
  const d = dFront + front, X = cx + Math.cos(theta * DEG) * d, Y = cy + Math.sin(theta * DEG) * d;
  if (o.plain) S.items.push(bldg(model, X, Y, theta + 180, { scale: o.scale })); else house(S, model, X, Y, theta + 180, o);
  return [X, Y, theta + 180];
}

// ================================================================ what every concept shares
function farmstead(S) {
  house(S, 'Farmhouse', -5600, -5400, 90, { garden: { w: 1100, d: 700, scarecrow: true }, laundry: true, lamp: 'lantern', flowerColors: ['red', 'yellow', 'white'] });
  S.items.push(bldg('Barn', -6700, -3300, 0), bldg('Well', -4500, -4100, 0), item('k:coop', -6250, -2380, 90), item('k:haystack', -7550, -2650, 0),
    item('k:haystack', -7800, -3250, 40), item('HayBale_Round', -5800, -2600, 80), item('HayBale_Round', -5650, -2330, 100), item('HayBale_Square', -5950, -2120, 10),
    item('Cart', -5100, -3000, 150), item('Wheelbarrow', -6250, -4150, 230), item('WaterTrough', -4850, -4700, 90), item('Signpost', -5200, -3600, 125),
    item('LanternPost', -5750, -3850, 215), item('Crate_B', -6200, -4500, 30), item('Barrel_A', -6350, -4700, 0), item('k:shed', -7300, -4600, 90));
  S.patches.push({ poly: FARMYARD, color: PAL.dirtEdge });
  flock(S, 'chicken', -6000, -2600, 8, 420, 41);
  folk(S, -4950, -3480, 215, 0); folk(S, -6100, -4380, 60, 1);
  label(S, 'Spawn · the farmstead', -5600, -4000, 3, 'town');
}
function orchard(S, o = {}) {
  // The layout's apple trees, filled out to full rows where the ground allows.
  const done = [];
  for (const row of COMPUTED.orchardRows) for (const t of row.trees) done.push([t[0], t[1]]);
  for (let X = -4800; X >= -6900; X -= 450) for (let Y = -6050; Y >= -7900; Y -= 450) {
    if (done.some(([a, b]) => Math.hypot(a - X, b - Y) < 300)) continue;
    if (Number.isNaN(groundUE(X, Y)) || slopeUE(X, Y) > 0.45 || distToPolyline(X, Y, ORCHARD_PATH) < 220) continue;
    done.push([X, Y]);
  }
  done.forEach(([X, Y], i) => S.items.push({ model: `k:apple:${i % 2}`, X, Y, yaw: (i * 67) % 360, sink: 0.08, scale: o.overgrown ? 1.15 : 1 }));
  road(S, 'path', 220, ORCHARD_PATH);
  if (!o.overgrown) S.fences.push({ kind: 'rail', points: [[-4350, -5850], [-4350, -8050], [-7150, -8050]] });
  S.items.push(item('Crate_B', -5000, -6900, 20), item('Crate_A', -5050, -6650, 70), item('Wheelbarrow', -5550, -6950, 120), item('Barrel_A', -4950, -7300, 0));
  label(S, 'Orchard', -5800, -7000, 5);
}
function windmill(S) {
  S.items.push(bldg('Windmill', 6200, -3800, 30, { sink: 0.15 }), item('WaterTrough', 5850, -3450, 120), item('k:shed', 6700, -3300, 210), item('Barrel_B', 6450, -3150, 0));
  road(S, 'path', 200, WINDMILL_PATH);
  S.noTrees.push(circle(6200, -3800, 900));
  label(S, 'Windmill', 6200, -3800, 12);
}
function range(S, o = {}) {
  for (const [X, Y] of [[5600, -1000], [5800, -1600], [5500, -2200]]) S.items.push({ model: 'k:dummy', X, Y, yaw: 180, sink: 0.02 });
  for (const [X, Y] of [[6150, -1150], [6280, -1700], [6100, -2350]]) S.items.push({ model: 'k:target', X, Y, yaw: 180, sink: 0.02 });
  for (const [X, Y, yaw] of [[6550, -1050, 90], [6650, -1700, 80], [6500, -2400, 95]]) S.items.push(item('HayBale_Round', X, Y, yaw));
  S.items.push(item('HayBale_Square', 6480, -1380, 20), item('HayBale_Square', 6420, -2030, 60), item('Bench', 4300, -1500, 0), item('Crate_A', 4250, -2200, 160),
    item('Crate_B', 4180, -1930, 0), item('Barrel_A', 4300, -2450, 0), item('LanternPost', 4150, -800, 0));
  if (o.cover !== false) S.items.push(item('k:sandbags:1', 4850, -1200, 0), item('k:barricade', 4900, -1850, 0), item('k:sandbags:2', 4800, -2450, 0));
  if (o.fence !== false) S.fences.push({ kind: 'rail', points: [[3750, -2650], [6750, -2650], [6750, -250], [4400, -250]], broken: [4] });
  S.patches.push({ poly: [[3900, -2550], [6650, -2550], [6650, -350], [3900, -350]], color: '#8b9450', lift: 0.03 });
  S.noTrees.push({ poly: RANGE_ZONE });
  label(S, 'Target range', 5300, -1600, 3, 'town');
}
function plateau(S) {
  S.items.push(bldg('LookoutTower', -6200, 6000, 45));
  road(S, 'path', 300, RAMP, { verge: false });
  road(S, 'path', 240, [RAMP[RAMP.length - 1], [-5050, 5300], [-5600, 5800], TOWER_DOOR]);
  const root = local(JETTY.X, JETTY.Y, JETTY.yaw, -600, 0);
  road(S, 'path', 220, [TOWER_DOOR, local(-6200, 6000, 45, 450, -500), root]);
  S.items.push({ model: 'SkiffJetty', X: JETTY.X, Y: JETTY.Y, yaw: JETTY.yaw, Z: JETTY.Z - 4 });
  const atJ = (sock) => { const [s] = socketOf('SkiffJetty', sock); return s ? local(JETTY.X, JETTY.Y, JETTY.yaw, s.pos[2] * 100, -s.pos[0] * 100) : null; };
  const bell = atJ('BellPost'), slate = atJ('Slate');
  if (bell) S.items.push(item('JettyBellPost', bell[0], bell[1], JETTY.yaw + 180));
  if (slate) S.items.push(item('JettySlate', slate[0], slate[1], JETTY.yaw + 180));
  const [land] = socketOf('SkiffJetty', 'Gangplank_Land'), [plank] = socketOf('Skiff_A_Packet', 'Gangplank');
  const along = land && plank ? (land.pos[2] - plank.pos[2]) * 100 : 900;
  const [sx, sy] = local(JETTY.X, JETTY.Y, JETTY.yaw, along, 503);
  S.items.push({ model: 'Skiff_A_Packet', X: sx, Y: sy, yaw: JETTY.yaw, Z: JETTY.Z + 30 - 45 });
  S.items.push(item('Cairn_A', -4700, 5100, 0), item('Cairn_C', -5450, 5600, 30), item('Bench', -6650, 6650, JETTY.yaw), item('LanternPost', root[0] + 120, root[1] - 160, JETTY.yaw + 180));
  S.noTrees.push(circle(-6200, 6000, 1000), circle(JETTY.X, JETTY.Y, 1100));
  label(S, 'Ruined lookout', -6200, 6000, 11);
  label(S, 'Skiff jetty · the way off the island', JETTY.X, JETTY.Y, 3);
}
function pond(S, o = {}) {
  S.veg.push({ kind: 'reeds', ring: { X: POND.center[0], Y: POND.center[1], rx: POND.radii[0] + 110, ry: POND.radii[1] + 110 }, every: 210, skip: o.skipReeds });
  for (let i = 0; i < 7; i++) { const a = i * 0.95 + 0.3; S.items.push({ model: 'LilyPads_A', X: POND.center[0] + Math.cos(a) * POND.radii[0] * 0.72, Y: POND.center[1] + Math.sin(a) * POND.radii[1] * 0.72, yaw: i * 70, Z: POND.waterZ + 3 }); }
  if (o.dock !== false) S.items.push({ model: 'k:dock', X: 2600, Y: 4470, yaw: 90, sink: 0.3 }, item('Bench', 2900, 4150, 270), item('Barrel_B', 2350, 4250, 0));
  const br = COMPUTED.bridge;
  S.items.push({ model: 'Bridge', X: br.location[0], Y: br.location[1], Z: br.location[2], yaw: br.yaw, scale: [1, 1, br.span / 872] });
  S.veg.push({ kind: 'line', pts: CREEK.path, every: 780, tree: 'birch', offset: 470 }, { kind: 'line', pts: CREEK.path, every: 830, tree: 'birch', offset: -470 });
  label(S, 'Pond', POND.center[0] + 300, POND.center[1], 1);
}
function meadows(S, o = {}) {
  S.veg.push({ kind: 'rocks', poly: INNER, n: o.rocks || 26 });
  S.veg.push({ kind: 'bushes', poly: OUTLINE.map(([x, y]) => [x * 0.97, y * 0.97]), spacing: 900, ring: 0.82 });
  S.tuftField = { spacing: o.tuftSpacing || 240 };
}
// Someone on a building's porch (or at its door), facing the way the building does.
function porch(S, [X, Y, yaw], model, r, variant, back = 140, lift = 0.4) {
  const b = modelBounds(model), [x, y] = local(X, Y, yaw, (b ? b.max[2] * 100 : 300) - back, r);
  folk(S, x, y, yaw + ((variant * 47) % 50) - 25, variant); S.creatures[S.creatures.length - 1].lift = lift;
}
// Lone trees and bushes over the open meadows, so no stretch of grass is bare.
function openGround(S, trees = 20, bushSpacing = 1500) {
  S.veg.push({ kind: 'scatter', poly: INNER, n: trees, mix: { oak: 3, birch: 1, apple: 1 } }, { kind: 'bushes', poly: INNER, spacing: bushSpacing });
}
function common(S, o = {}) {
  farmstead(S); orchard(S, o.orchard); windmill(S); range(S, o.range); plateau(S); pond(S, o.pond); meadows(S, o.meadows);
  road(S, 'path', 200, PLATEAU_PATH.slice(1));
}
function views(S, center) {
  above(S, 'Island', -2990, -1300, 172, 30, 50, 0);
  above(S, center.name, center.X, center.Y, center.dist || 72, center.yaw ?? 32, 48, 2);
  above(S, 'Spawn farm', -5500, -4300, 62, 42, 46, 2);
  // the jetty from out over the edge: tower, jetty and moored skiff side on, the plateau behind
  S.views.above.push({ name: 'Plateau', X: JETTY.X - 380, Y: JETTY.Y + 190, Z: JETTY.Z + 60, dist: 58, yaw: 30, pitch: 26 });
}

// ================================================================ creature grounds
function slimeWallow(S, X, Y) {
  S.patches.push({ poly: circlePoly(X, Y, 1450, 22, 0.22, 5), color: PAL.bog }, { poly: circlePoly(X - 150, Y - 200, 760, 16, 0.25, 6), color: PAL.bogDark, lift: 0.07 });
  S.water.push(circle(X - 200, Y - 300, 420), circle(X + 380, Y + 320, 300), circle(X - 470, Y + 480, 240), circle(X + 80, Y + 820, 200), circle(X + 650, Y - 480, 180));
  S.veg.push({ kind: 'reeds', ring: { X, Y, rx: 1280, ry: 1120 }, every: 200 }, { kind: 'reeds', ring: { X: X - 200, Y: Y - 300, rx: 520, ry: 500 }, every: 190 });
  S.items.push({ model: 'LilyPads_A', X: X - 200, Y: Y - 300, yaw: 20, lift: 0.05 }, { model: 'LilyPads_A', X: X + 380, Y: Y + 320, yaw: 200, lift: 0.05 },
    item('Rock_A', X + 900, Y - 700, 30, { color: PAL.moss }), item('Rock_B', X - 1000, Y + 300, 80, { color: PAL.moss }), item('Boulder_B', X + 500, Y + 1250, 120, { color: PAL.moss }),
    item('Rock_A', X - 300, Y - 1250, 200, { color: PAL.moss }), item('FenceBroken', X + 1350, Y - 900, 185), item('FenceBroken', X + 1250, Y + 500, 200), item('FenceRail', X + 1330, Y - 300, 182),
    item('Signpost', X + 1550, Y + 1100, -60), { model: 'Cart', X: X - 700, Y: Y - 800, yaw: 70, sink: 0.35, tiltZ: 0.25, color: '#a8c79a' });
  for (const [dx, dy, s] of [[600, 200, 3], [-800, -300, 4], [200, -900, 5], [-500, 900, 6], [950, 900, 7], [-1100, 700, 8]]) S.items.push({ model: `k:mush:${s}`, X: X + dx, Y: Y + dy, yaw: s * 40 });
  S.trails.push({ pts: [[X + 800, Y + 500], [X + 1500, Y + 1300], [X + 1900, Y + 2400]], width: 55 }, { pts: [[X + 600, Y - 800], [X + 1300, Y - 1600]], width: 45 }, { pts: [[X - 900, Y + 200], [X - 1700, Y + 700]], width: 45 });
  slimeGroup(S, X, Y, 5, 1000, 11);
  S.noTrees.push(circle(X, Y, 2000));
  label(S, 'Slime Wallow · 5 slimes', X, Y, 3, 'slime');
}
function slimeShallows(S) {
  const X = 1250, Y = 4250;
  S.patches.push({ poly: [[450, 3500], [1700, 3350], [2300, 3950], [2150, 4700], [1200, 5000], [450, 4500]], color: PAL.bog });
  S.water.push(circle(900, 4100, 330), circle(1500, 3900, 280), circle(1300, 4550, 420), circle(800, 4700, 220), circle(1950, 4350, 200));
  S.roads.push({ kind: 'plank', width: 150, points: [[350, 3450], [900, 3950], [1450, 4300], [2150, 4500]] });
  S.items.push(bldg('Outhouse', 200, 3200, 125, { sink: 0.08 }), item('k:shed', -150, 3650, 40), item('WaterTrough', 150, 3900, 20), item('Barrel_B', 550, 3250, 0), item('Barrel_B', 380, 3480, 40),
    item('Crate_A', 230, 3600, 95), item('LanternPost', 2250, 4650, 300), item('Signpost', 350, 3900, 60), item('LaundryLine', -300, 3150, 30));
  S.veg.push({ kind: 'reeds', ring: { X: 1300, Y: 4300, rx: 950, ry: 820 }, every: 200 });
  S.items.push({ model: 'LilyPads_A', X: 1300, Y: 4550, yaw: 10, lift: 0.05 }, { model: 'LilyPads_A', X: 900, Y: 4100, yaw: 120, lift: 0.05 }, item('Rock_B', 1900, 3600, 20, { color: PAL.moss }), item('Rock_A', 2100, 4950, 70, { color: PAL.moss }));
  for (const [dx, dy, s] of [[-500, -300, 3], [700, -700, 4], [200, 800, 5]]) S.items.push({ model: `k:mush:${s}`, X: X + dx, Y: Y + dy, yaw: s * 50 });
  S.trails.push({ pts: [[1100, 3500], [700, 2900], [400, 2300]], width: 50 }, { pts: [[2100, 3800], [2600, 3300]], width: 45 });
  slimeGroup(S, X, Y, 5, 800, 13);
  S.noTrees.push(circle(X, Y, 1700));
  label(S, 'The Shallows · 5 slimes', X, Y, 3, 'slime');
}
function slimeBog(S) {
  const X = 2400, Y = -5600;
  fieldRect(S, X, Y, 2200, 2200, 0, 'bog', { fence: 'rail', broken: [2, 5, 9] });
  S.water.push(circle(2000, -6000, 460), circle(2800, -5300, 380), circle(2500, -6300, 260), circle(1800, -5000, 300), circle(3000, -6100, 220));
  S.items.push({ model: 'k:sluice', X: 3500, Y: -5650, yaw: 90 }, item('WaterTrough', 3700, -5250, 0), item('Signpost', 3700, -4400, -50), item('Rock_A', 1600, -5500, 0, { color: PAL.moss }), item('Rock_B', 3100, -6400, 60, { color: PAL.moss }),
    { model: 'Wheelbarrow', X: 2200, Y: -4800, yaw: 40, sink: 0.25, tiltZ: 0.2 });
  S.roads.push({ kind: 'plank', width: 120, points: [[3500, -5600], [2900, -5700], [2300, -5900]] });
  S.veg.push({ kind: 'reeds', poly: [[1300, -6700], [3500, -6700], [3500, -4500], [1300, -4500]], spacing: 300 });
  S.items.push({ model: 'LilyPads_A', X: 2000, Y: -6000, yaw: 30, lift: 0.05 });
  S.trails.push({ pts: [[3550, -5000], [4200, -4500], [4600, -3700]], width: 50 });
  slimeGroup(S, X, Y, 5, 850, 17);
  S.noTrees.push(circle(X, Y, 2000));
  label(S, 'Bog Field · 5 slimes', X, Y, 3, 'slime');
}
function slimeSpring(S) {
  const X = 1650, Y = 4300;
  S.patches.push({ poly: circlePoly(X, Y, 1150, 18, 0.2, 21), color: PAL.bog });
  S.water.push(circle(X, Y, 520), circle(X + 650, Y + 450, 230), circle(X - 500, Y + 650, 200));
  S.items.push(item('Boulder_B', X - 650, Y - 300, 20, { color: PAL.moss }), item('Boulder_A', X + 700, Y - 520, 110, { color: PAL.moss }), item('Rock_A', X + 300, Y + 900, 0, { color: PAL.moss }),
    item('Rock_C', X - 900, Y + 200, 60, { color: PAL.moss }), item('Rock_B', X - 100, Y - 820, 140, { color: PAL.moss }), item('Cairn_B', X - 1350, Y - 800, 0), item('Cairn_A', X + 1250, Y + 950, 30),
    item('CampfireRing', X - 1550, Y + 700, 0), item('Bench', X - 1750, Y + 400, 100));
  S.veg.push({ kind: 'reeds', ring: { X, Y, rx: 660, ry: 660 }, every: 190 });
  S.items.push({ model: 'LilyPads_A', X, Y: Y + 100, yaw: 0, lift: 0.05 }, { model: 'LilyPads_A', X: X + 650, Y: Y + 450, yaw: 90, lift: 0.05 });
  for (const [dx, dy, s] of [[-800, -600, 3], [800, 300, 4], [-300, 1000, 6], [1000, -900, 7]]) S.items.push({ model: `k:mush:${s}`, X: X + dx, Y: Y + dy, yaw: s * 40 });
  slimeGroup(S, X, Y, 5, 950, 19);
  S.noTrees.push(circle(X, Y, 1500));
  label(S, 'Mossy Spring · 5 slimes', X, Y, 3, 'slime');
}
function spiderHollow(S) {
  const cx = 5800, cy = 5600;
  S.patches.push({ poly: circlePoly(cx, cy, 1750, 20, 0.2, 9), color: PAL.needles });
  const ring = [[5000, 5400, 0], [5700, 4700, 40], [6500, 5100, 90], [6800, 6000, 120], [6200, 6700, 200], [5300, 6500, 260], [4900, 5900, 300]];
  deadGrove(S, ring);
  S.webs.push({ a: [4900, 5900], b: [5000, 5400], h0: 1.8, h1: 3.8 });
  S.items.push({ model: 'k:burrow', X: 5600, Y: 5450, yaw: 20 }, { model: 'k:burrow', X: 6250, Y: 6050, yaw: 140 });
  S.webs.push({ sheet: true, X: 5600, Y: 5450, r: 430 }, { sheet: true, X: 6250, Y: 6050, r: 360 });
  S.items.push(bldg('Outcrop_TorB', 7050, 6650, 20, { sink: 0.3 }), item('Boulder_A', 4700, 5000, 30), item('Boulder_C', 6600, 4500, 70), item('Rock_C', 5900, 6950, 0), item('Rock_D', 6350, 5850, 0));
  spiderGroup(S, cx, cy, 8, 1250, 21);
  S.noTrees.push(circle(cx, cy, 1750));
  label(S, 'Web Hollow · 8 spiders', cx, cy, 7, 'spider');
}
function spiderHomestead(S) {
  const cx = 5100, cy = 4600;
  S.patches.push({ poly: circlePoly(cx, cy, 1550, 20, 0.2, 12), color: PAL.needles });
  S.items.push(bldg('Ruin_Chimney', 5150, 4350, 30, { sink: 0.15 }), bldg('Ruin_Springhouse', 5850, 3750, -60, { sink: 0.15 }), item('StoneWall_Broken', 4450, 3700, 20), item('StoneWall_Fallen', 4500, 4950, 110),
    item('StoneWall_Corner', 4400, 4350, 20), { model: 'Wagon_BurntA', X: 4700, Y: 5350, yaw: 100, sink: 0.08 }, item('Cairn_C', 5650, 5200, 0), item('Crate_A', 5450, 4050, 30));
  deadGrove(S, [[4300, 4000, 0], [5900, 4700, 50], [6150, 5550, 100], [5250, 5650, 180], [4300, 5350, 250]]);
  S.webs.push({ a: [4300, 4000], b: [5150, 4350], h0: 2.2, h1: 4.4 }, { a: [5150, 4350], b: [5900, 4700], h0: 2.4, h1: 4.6 });
  S.items.push({ model: 'k:burrow', X: 5250, Y: 4950, yaw: 60 }); S.webs.push({ sheet: true, X: 5250, Y: 4950, r: 420 });
  spiderGroup(S, cx, cy, 8, 1100, 23);
  S.noTrees.push(circle(cx, cy, 1600));
  label(S, 'Old Homestead · 8 spiders', cx, cy, 7, 'spider');
}
function spiderCliffDens(S) {
  const cx = -2150, cy = 6250;
  S.patches.push({ poly: circlePoly(cx, cy, 1300, 20, 0.2, 14), color: PAL.needles });
  S.items.push(bldg('Outcrop_Slab', -2700, 6050, -10, { sink: 0.3 }), item('Boulder_A', -2600, 5400, 50), item('Boulder_C', -2550, 7050, 10), item('Rock_C', -2300, 5600, 0), item('Rock_C', -2450, 6800, 70), item('Rock_D', -1900, 6000, 0));
  deadGrove(S, [[-2300, 5250, 0], [-1600, 5600, 70], [-1350, 6300, 130], [-1700, 7000, 200], [-2350, 7250, 260]]);
  S.webs.push({ a: [-2300, 5250], b: [-2700, 6050], h0: 1.8, h1: 4.2 }, { a: [-2700, 6050], b: [-2350, 7250], h0: 1.8, h1: 4.4 });
  S.items.push({ model: 'k:burrow', X: -2550, Y: 5800, yaw: 90 }, { model: 'k:burrow', X: -2600, Y: 6600, yaw: 80 }, { model: 'k:sacs:3', X: -2750, Y: 5550 });
  S.webs.push({ sheet: true, X: -2550, Y: 5800, r: 380 }, { sheet: true, X: -2600, Y: 6600, r: 340 });
  spiderGroup(S, cx, cy, 8, 900, 25);
  S.noTrees.push(circle(cx, cy, 1400));
  label(S, 'Cliff Dens · 8 spiders', cx, cy, 6, 'spider');
}
function spiderThicket(S) {
  const cx = 4250, cy = 6500;
  S.patches.push({ poly: circlePoly(cx, cy, 1450, 20, 0.2, 15), color: PAL.needles });
  const trees = [[4300, 5450, 0], [3750, 5800, 60], [4500, 6250, 110], [3700, 6600, 170], [4650, 7000, 230], [3900, 7300, 280], [4750, 7650, 330]];
  deadGrove(S, trees, { pines: true });
  S.webs.push({ a: [3750, 5800], b: [4500, 6250], h0: 1.4, h1: 3.0 }, { a: [3700, 6600], b: [4650, 7000], h0: 1.3, h1: 2.9 }, { a: [3900, 7300], b: [4750, 7650], h0: 1.5, h1: 3.1 });
  S.items.push({ model: 'k:burrow', X: 3650, Y: 6200, yaw: 30 }); S.webs.push({ sheet: true, X: 3650, Y: 6200, r: 380 });
  S.items.push(bldg('Outcrop_Fin', 6400, 6650, 40, { sink: 0.3 }), item('Boulder_B', 3350, 6250, 20), item('Rock_C', 5000, 6800, 0));
  spiderGroup(S, cx, cy, 8, 1000, 27);
  S.noTrees.push(circle(cx, cy, 1300));
  label(S, 'Pine Thicket · 8 spiders', cx, cy, 6, 'spider');
}

// ================================================================ 1 · Crossroads Town
function conceptCrossroads() {
  const S = newSpec(101);
  S.text = { name: '1 · Crossroads Town',
    tagline: 'The crossroads grows into a small town: a cobbled square ringed by a saloon, a store, the sheriff, the undertaker and homes, with more houses along the brick-kerbed road from the farm.',
    bullets: ['<b>Town</b>: seven buildings face a cobbled square with the memorial, the well, the gun rack, market stalls, benches and lamps; three more homes line the farm road, each with a garden, washing and a smoking chimney.',
      '<b>Roads</b>: brick kerbs and lamps from the farm to the range, cobbles in the square, dirt with wheel ruts elsewhere; barricades and sandbags at the range give cover.',
      '<b>Slimes</b>: the Wallow, a bog in the west meadow with pools, reeds, mushrooms, a sunken cart and slime trails heading for town.',
      '<b>Spiders</b>: Web Hollow on the forest rise, a ring of dead trees strung with webs round two burrows, cocoons hanging from the branches.',
      '<b>Green</b>: oak and birch groves round the town, hedgerows by the farm road, wildflower meadows west and south, pines on the rise, reeds at the pond.',
      '<b>Life</b>: townsfolk at the stalls, on the porches, at the well and on the road; a sheep pasture between the farm road and the plateau; hens and farmhands at the farm.'] };
  common(S);
  const sq = 1300;
  S.patches.push({ poly: [[-sq, -sq], [sq, -sq], [sq, sq], [-sq, sq]], color: PAL.cobbleB, lift: 0.04 });
  S.cobbles.push({ poly: [[-sq, -sq], [sq, -sq], [sq, sq], [-sq, sq]], spacing: 40, keep: [circle(0, 0, 160), circle(-450, 650, 160)] });
  S.items.push(item('TownMemorial', 0, 0, 200), bldg('Well', -450, 650, 20), item('GunRack', 500, -500, 200), item('NoticeBoard', 950, -250, 200));
  label(S, 'Crossroads Town', 0, 0, 12, 'town');
  const saloon = aroundSquare(S, 0, 0, 'FalseFront_Saloon', 15, 1400, { bench: false, flowers: false, firewood: false });
  aroundSquare(S, 0, 0, 'Cottage', 72, 1450, { garden: { w: 800, d: 600 }, laundry: true });
  aroundSquare(S, 0, 0, 'LogCabin', 126, 1400, { garden: { w: 800, d: 600, scarecrow: true } });
  const sheriff = aroundSquare(S, 0, 0, 'FalseFront_Sheriff', 168, 1400, { flowers: false, firewood: false });
  aroundSquare(S, 0, 0, 'Cottage', 243, 1400, { scale: 0.95, laundry: true, flowerColors: ['blue', 'white'] });
  const store = aroundSquare(S, 0, 0, 'FalseFront_Store', 280, 1400, { bench: false, firewood: false });
  aroundSquare(S, 0, 0, 'FalseFront_Undertaker', 318, 1450, { flowers: false, firewood: false, barrels: false });
  S.items.push(bldg('Outhouse', 2450, -2500, 160, { sink: 0.08 }), item('k:stall', -1000, -500, 70), item('k:stall', 880, 850, 215), item('k:stall', -300, 1050, 275),
    item('Bench', 380, 330, 225), item('Bench', -380, -330, 45), item('HitchRail', 1250, 380, 108), item('WaterTrough', 1150, 750, 108), item('Cart', -700, -1050, 60),
    item('Barrel_A', 1050, -1050, 0), item('Barrel_B', 1150, -900, 0), item('Crate_A', 1180, -1180, 20), item('Crate_B', -1150, 1150, 40));
  for (const [X, Y] of [[-1150, -1150], [1150, -1150], [1150, 1150], [-1150, 1150]]) S.items.push(item('LampPost', X, Y, ueYawOf(-X, -Y)));
  road(S, 'brick', 460, FARM_ROAD, { lamps: 1500, verge: true }); road(S, 'brick', 440, RANGE_ROAD, { lamps: 1700 });
  road(S, 'dirt', 400, FOREST_ROAD.slice(0, 4)); road(S, 'dirt', 380, FOREST_ROAD.slice(3));
  besideRoad(S, FARM_ROAD, 2300, -1, 'LogCabin', 650, { garden: { w: 800, d: 600 }, laundry: true, outhouse: true });
  besideRoad(S, FARM_ROAD, 3500, 1, 'Cottage', 620, { garden: { w: 700, d: 600, scarecrow: true }, flowerColors: ['pink', 'white'] });
  besideRoad(S, FARM_ROAD, 4700, -1, 'Farmhouse', 700, { scale: 0.88, garden: { w: 900, d: 650 }, laundry: true });
  S.hedges.push({ pts: offsetPolyline(FARM_ROAD.slice(0, 2), 420) }, { pts: offsetPolyline(RANGE_ROAD.slice(1), -420) }, { pts: offsetPolyline(RANGE_ROAD.slice(1), 420) });
  S.fences.push({ kind: 'stone', points: [[3700, -2750], [3700, -3600], [5200, -4700]] });
  S.items.push(item('k:barricade', 3300, -800, 105), item('k:sandbags:4', 2600, -650, 100), item('Crate_A', 2800, -300, 10), item('Crate_B', 2950, -150, 40));
  // townsfolk: at the stalls, on the porches, at the well and the notice board, on the road
  for (const [X, Y, yaw] of [[-1000, -500, 70], [880, 850, 215], [-300, 1050, 275]]) {
    const [sx, sy] = local(X, Y, yaw, -95, 0), [cx, cy] = local(X, Y, yaw, 170, 40);
    folk(S, sx, sy, yaw, (X > 0 ? 1 : 3)); folk(S, cx, cy, yaw + 180, (Y > 0 ? 0 : 2));
  }
  porch(S, saloon, 'FalseFront_Saloon', 220, 2); porch(S, saloon, 'FalseFront_Saloon', -260, 0); porch(S, store, 'FalseFront_Store', 150, 1); porch(S, sheriff, 'FalseFront_Sheriff', 0, 2, 60, 0);
  folk(S, -250, 900, 200, 3); folk(S, ...local(950, -250, 200, 120, 0), 20, 0); folk(S, -2950, -1550, 45, 1); folk(S, 4400, -1750, 0, 2);
  // a sheep pasture between the farm road and the plateau
  fieldRect(S, -5900, -100, 1800, 2200, 0, 'pasture', { fence: 'rail', gap: 3 });
  flock(S, 'sheep', -5900, -100, 9, 720, 51);
  S.items.push(item('WaterTrough', -5250, 700, 90), item('k:shed', -6450, 750, 180));
  slimeWallow(S, 2400, -5600); spiderHollow(S); openGround(S, 22, 1500);
  S.veg.push({ kind: 'woods', poly: FOREST_ZONE, spacing: 640, mix: { pine: 5, oak: 2, birch: 2 } },
    { kind: 'grove', X: 300, Y: 3600, r: 1300, n: 15, mix: { oak: 3, birch: 2 } }, { kind: 'grove', X: -3600, Y: 400, r: 1300, n: 12, mix: { oak: 3, birch: 1 } },
    { kind: 'grove', X: -3600, Y: -5600, r: 900, n: 7, mix: { oak: 2, birch: 1 } }, { kind: 'grove', X: 7400, Y: 1300, r: 1500, n: 13, mix: { pine: 3, oak: 1 } },
    { kind: 'grove', X: 3000, Y: 1600, r: 900, n: 7, mix: { oak: 2, birch: 2 } }, { kind: 'grove', X: -1800, Y: -4800, r: 1100, n: 9, mix: { oak: 2, birch: 1 } },
    { kind: 'flowers', poly: [[1300, -4600], [4300, -4800], [5000, -3000], [2800, -2000], [1500, -2600]], spacing: 170 },
    { kind: 'flowers', poly: [[-3000, 1000], [-800, 2300], [-2600, 2500], [-3500, 1900]], spacing: 170 },
    { kind: 'flowers', poly: [[-2600, -6200], [-800, -6800], [-400, -5000], [-2200, -4200]], spacing: 190 },
    { kind: 'bushes', poly: FOREST_ZONE, spacing: 850 });
  views(S, { name: 'Town', X: 0, Y: 0, dist: 78 });
  above(S, 'Slimes', 2400, -5600, 52, 60, 52, 0); above(S, 'Spiders', 5800, 5600, 55, 30, 50, 0);
  foot(S, 'Spawn', -5930, -4230, 35); foot(S, 'Square', -1900, -1450, 38, -2); foot(S, 'Farm road', -3900, -2400, 40, -2);
  foot(S, 'Slimes', 4400, -4600, -150, -5); foot(S, 'Spiders', 3950, 4700, 30, 0); foot(S, 'Lookout', -4300, 2650, -30, -9);
  return S;
}

// ================================================================ 2 · Main Street
function conceptMainStreet() {
  const S = newSpec(202);
  S.text = { name: '2 · Main Street',
    tagline: 'One frontier street runs from the crossroads to the foot of the plateau ramp: false fronts and porches on both sides, hitching rails and lamps, a water tower, and a barricade at the far end.',
    bullets: ['<b>Town</b>: store, saloon, sheriff and a cabin on the north side; the undertaker with his coffin shed, two cottages and the water tower on the south; back lanes with sheds, outhouses, washing and gardens.',
      '<b>Roads</b>: Main Street is brick with kerbs and lamps; the old roads stay dirt with ruts; a wagon, crates and sandbags block the far end as cover.',
      '<b>Slimes</b>: the Shallows on the pond\'s west shore, with pools, reeds, a rickety plank walk, a fishing shed and slime trails up the bank.',
      '<b>Spiders</b>: the Old Homestead in the forest: a burnt chimney, a springhouse, fallen walls and a burnt wagon, all webbed over around a burrow.',
      '<b>Green</b>: birches along the creek, oaks behind the street, pines on the forest rise, hedges along the back lanes, flower verges by the road.',
      '<b>Fields</b>: west of the farm road a hedged lane runs to the town fields: wheat, a plowed field, a cottage with a garden and a sheep pasture.',
      '<b>Life</b>: townsfolk on the porches and in the street, farmers in the fields, hens and farmhands at the farm.'] };
  common(S, { pond: { dock: false } });
  const A = [300, -200], B = [-2300, 2420];
  const STREET = [A, [lerp(A[0], B[0], 0.5), lerp(A[1], B[1], 0.5)], B];
  road(S, 'brick', 520, STREET, { lamps: 800, lampStart: 200 });
  label(S, 'Main Street', STREET[1][0], STREET[1][1], 12, 'town');
  road(S, 'dirt', 450, FARM_ROAD.slice(0, 3).concat([A])); road(S, 'dirt', 420, [A, ...RANGE_ROAD.slice(1)]);
  road(S, 'dirt', 400, [A, ...FOREST_ROAD.slice(1, 4)]); road(S, 'dirt', 380, FOREST_ROAD.slice(3));
  const store = besideRoad(S, STREET, 520, -1, 'FalseFront_Store', 330, { bench: false, flowers: false, firewood: false });
  const saloon = besideRoad(S, STREET, 1500, -1, 'FalseFront_Saloon', 330, { bench: false, flowers: false, firewood: false });
  const sheriff = besideRoad(S, STREET, 2450, -1, 'FalseFront_Sheriff', 330, { flowers: false, firewood: false });
  besideRoad(S, STREET, 3250, -1, 'LogCabin', 420, { garden: { w: 800, d: 600 }, laundry: true });
  const undertaker = besideRoad(S, STREET, 600, 1, 'FalseFront_Undertaker', 330, { flowers: false, firewood: false, barrels: false });
  besideRoad(S, STREET, 1260, 1, 'CoffinShed', 360, { plain: true });
  besideRoad(S, STREET, 1950, 1, 'Cottage', 420, { garden: { w: 700, d: 600, scarecrow: true }, flowerColors: ['pink', 'white'] });
  besideRoad(S, STREET, 2700, 1, 'WaterTower', 420, { plain: true });
  besideRoad(S, STREET, 3420, 1, 'Cottage', 420, { scale: 0.95, laundry: true, outhouse: true, flowerColors: ['blue', 'yellow'] });
  const at = (t, side) => { const [X, Y, yaw] = pointAt(STREET, t); return [...local(X, Y, yaw, 0, side), yaw]; };
  const [h1x, h1y, sy] = at(1500, -330); S.items.push(item('HitchRail', h1x, h1y, sy - 180), item('WaterTrough', ...at(1150, -320).slice(0, 2), sy));
  const [h2x, h2y] = at(520, -330); S.items.push(item('HitchRail', h2x, h2y, sy - 180));
  S.items.push(item('NoticeBoard', ...at(-80, -420).slice(0, 2), sy + 90), item('Cart', ...at(2150, 300).slice(0, 2), sy + 10), item('Barrel_A', ...at(950, -360).slice(0, 2), 0),
    item('Barrel_B', ...at(1000, -380).slice(0, 2), 0), item('Crate_A', ...at(2900, -360).slice(0, 2), 20), item('Bench', ...at(1850, -370).slice(0, 2), sy - 90),
    item('GunRack', 600, -650, 200), bldg('Well', ...at(1950, 0).slice(0, 2), 10));
  const [ex, ey] = at(3900, 0);
  S.items.push({ model: 'Wagon_BurntB', X: ex, Y: ey, yaw: sy + 70, sink: 0.05, color: '#d8c8b0' }, item('k:sandbags:6', ...at(3650, -180).slice(0, 2), sy + 90), item('k:barricade', ...at(3700, 230).slice(0, 2), sy + 90),
    item('Crate_B', ...at(3600, 380).slice(0, 2), 30), item('Crate_A', ...at(3560, 520).slice(0, 2), 70));
  S.roads.push({ kind: 'path', width: 220, points: [at(200, -1900).slice(0, 2), at(3600, -1900).slice(0, 2)] }, { kind: 'path', width: 200, points: [at(300, 1900).slice(0, 2), at(3500, 1900).slice(0, 2)] });
  S.items.push(item('k:shed', ...at(1100, -2500).slice(0, 2), sy + 180), item('k:shed', ...at(2300, 2450).slice(0, 2), sy), bldg('Outhouse', ...at(1800, -2350).slice(0, 2), sy + 180, { sink: 0.08 }),
    item('LaundryLine', ...at(900, 2400).slice(0, 2), sy), item('FirewoodStack', ...at(1500, 2300).slice(0, 2), sy));
  S.hedges.push({ pts: [at(100, -2250).slice(0, 2), at(3700, -2250).slice(0, 2)] }, { pts: [at(150, 2250).slice(0, 2), at(3600, 2250).slice(0, 2)] });
  S.flowerBeds.push({ pts: [at(100, -330).slice(0, 2), at(3700, -330).slice(0, 2)], every: 120, colors: ['yellow', 'white'] }, { pts: [at(100, 330).slice(0, 2), at(3700, 330).slice(0, 2)], every: 120, colors: ['pink', 'white'] });
  besideRoad(S, FARM_ROAD, 3400, 1, 'Cottage', 620, { scale: 0.92, garden: { w: 700, d: 550 } });
  S.fences.push({ kind: 'rail', points: offsetPolyline(FARM_ROAD.slice(0, 2), 340), broken: [4] }, { kind: 'rail', points: offsetPolyline(FARM_ROAD.slice(0, 2), -340) });
  porch(S, store, 'FalseFront_Store', 200, 1); porch(S, saloon, 'FalseFront_Saloon', -250, 2); porch(S, saloon, 'FalseFront_Saloon', 180, 3);
  porch(S, sheriff, 'FalseFront_Sheriff', 0, 0, 60, 0); porch(S, undertaker, 'FalseFront_Undertaker', 120, 2);
  folk(S, ...at(1100, 60).slice(0, 2), sy, 1); folk(S, ...at(2300, -80).slice(0, 2), sy + 180, 0); folk(S, ...at(1950, 250).slice(0, 2), sy - 90, 3); folk(S, ...at(3550, 0).slice(0, 2), sy + 180, 2);
  // the town's fields, west of the farm road: a hedged lane, wheat, a plowed field, a cottage and a sheep pasture
  const LANE = [[-1800, -900], [-1000, -2600], [0, -3900], [900, -5300]];
  road(S, 'dirt', 320, LANE, { hedges: [-1] });
  fieldRect(S, -2050, -4050, 1500, 1500, 0, 'wheat', { fence: 'rail' });
  fieldRect(S, 600, -2600, 1400, 1400, 0, 'plowed', { fence: 'rail', broken: [3] });
  besideRoad(S, LANE, 3500, 1, 'Cottage', 520, { scale: 0.95, garden: { w: 700, d: 550, scarecrow: true }, laundry: true });
  fieldRect(S, 2100, -6300, 1600, 1800, 0, 'pasture', { fence: 'rail', gap: 2 });
  flock(S, 'sheep', 2100, -6300, 8, 650, 61);
  S.items.push(item('WaterTrough', 2700, -5700, 0), item('HayBale_Round', -2700, -3500, 30), item('HayBale_Round', -1500, -4500, 80), item('k:haystack', -1200, -3400, 0));
  folk(S, -2000, -4000, 30, 2); folk(S, 1400, -5700, 250, 0);
  label(S, 'Town fields', -1000, -3800, 4, 'town');
  slimeShallows(S); spiderHomestead(S); openGround(S, 24, 1400);
  S.veg.push({ kind: 'woods', poly: FOREST_ZONE, spacing: 620, mix: { pine: 4, oak: 3, birch: 1 } },
    { kind: 'grove', X: -4200, Y: -1300, r: 1400, n: 13, mix: { oak: 3, birch: 1 } }, { kind: 'grove', X: -2900, Y: -5600, r: 1000, n: 8, mix: { oak: 2 } },
    { kind: 'grove', X: 7400, Y: 1300, r: 1400, n: 11, mix: { pine: 3 } }, { kind: 'grove', X: 1500, Y: 1300, r: 800, n: 6, mix: { oak: 2, birch: 1 } },
    { kind: 'grove', X: -3600, Y: 1200, r: 900, n: 7, mix: { birch: 2, oak: 1 } },
    { kind: 'flowers', poly: [[1400, -4700], [4200, -4900], [4900, -3000], [2800, -2000], [1500, -2700]], spacing: 180 },
    { kind: 'flowers', poly: [[-3000, 300], [-1600, 1100], [-2800, 2400], [-3600, 1900]], spacing: 160 },
    { kind: 'bushes', poly: FOREST_ZONE, spacing: 820 });
  views(S, { name: 'Street', X: -1000, Y: 1100, dist: 80, yaw: 60 });
  above(S, 'Slimes', 1250, 4250, 50, 80, 52, 0); above(S, 'Spiders', 5100, 4600, 55, 40, 50, 0);
  foot(S, 'Spawn', -5930, -4230, 35); foot(S, 'Street', ...at(-250, 0).slice(0, 2), sy, -1); foot(S, 'Street end', ...at(3450, -100).slice(0, 2), sy + 180, -1);
  foot(S, 'Slimes', 2600, 2950, 136, -6); foot(S, 'Spiders', 3600, 3800, 30, 0); foot(S, 'Lookout', -4300, 2650, -30, -9);
  return S;
}

// ================================================================ 3 · Hill Farms
function conceptHillFarms() {
  const S = newSpec(303);
  S.text = { name: '3 · Hill Farms',
    tagline: 'No town, but four homesteads tied together by lanes between dry-stone walls: wheat, plowed fields and pasture in a patchwork, hedgerows, and a well and a market stall at the crossroads.',
    bullets: ['<b>Homesteads</b>: the spawn farm and its hay; Cross Farm\'s cabin with a walled paddock; the Mill Cottage under the windmill; the Pond Farm with a farmhouse, barn, coop and haystacks.',
      '<b>Roads</b>: lanes sunk between dry-stone walls, rail fences round every field, a signpost, a well and a stall where the lanes cross; stone walls and hay give cover everywhere.',
      '<b>Slimes</b>: the Bog Field, a flooded low field with a broken sluice gate, pools in the furrows and a fence they came through.',
      '<b>Spiders</b>: the Cliff Dens at the plateau\'s foot, webs across the rock, burrows in the scree and cocoons on the dead trees.',
      '<b>Green</b>: hedgerows along the walls, lone oaks in the pastures, the orchard, birches at the pond, flower margins between field and lane.',
      '<b>Life</b>: sheep in Cross Farm\'s paddock, cows in a pasture under the plateau, hens at the Pond Farm and the farmstead, farmers in the fields and at the stall.'] };
  common(S, { range: { fence: false } });
  fieldRect(S, -3000, -4400, 2400, 1700, 0, 'wheat', { fence: 'rail' });
  fieldRect(S, -6500, -1800, 1600, 2000, 0, 'plowed', { fence: 'rail', broken: [6] });
  fieldRect(S, -2900, -6600, 1500, 1800, 20, 'crop', { fence: 'rail' });
  S.items.push(item('HayBale_Round', -3400, -3900, 20), item('HayBale_Round', -2900, -4700, 70), item('HayBale_Round', -2300, -4100, 110), item('k:haystack', -2500, -5000, 0), item('k:scarecrow', -2900, -6500, 30));
  // Cross Farm
  house(S, 'LogCabin', 1300, -1500, 180, { garden: { w: 800, d: 600 }, laundry: true });
  S.fences.push({ kind: 'stone', points: [[1700, -2600], [3000, -2600], [3000, -1100]] });
  fieldRect(S, 2350, -1850, 1300, 1400, 0, 'pasture', {});
  S.items.push(item('WaterTrough', 2500, -2200, 0), item('HayBale_Square', 2700, -2350, 30), item('HayBale_Square', 2550, -2450, 80), item('k:shed', 1050, -2900, 0));
  // the crossroads
  S.items.push(bldg('Well', 0, 0, 0), item('GunRack', 500, -500, 200), item('Signpost', -300, -600, 30), item('k:stall', 650, 500, 220), item('Bench', -550, 450, 135), item('LampPost', -700, 300, 0), item('LampPost', 700, -250, 180), item('NoticeBoard', -500, -450, 45));
  S.patches.push({ poly: circlePoly(0, 0, 950, 16, 0.12, 2), color: PAL.dirtEdge });
  house(S, 'Cottage', -1100, 1400, -45, { garden: { w: 700, d: 550 } });
  // Mill Cottage
  house(S, 'Cottage', 4400, -4500, 115, { scale: 0.95, garden: { w: 700, d: 550 }, laundry: true });
  fieldRect(S, 5300, -2850, 1300, 900, 10, 'plowed', { fence: 'rail' });
  // Pond Farm
  house(S, 'Farmhouse', 300, 3900, 0, { garden: { w: 900, d: 600 }, laundry: true, flowerColors: ['red', 'yellow'] });
  S.items.push(bldg('Barn', -1150, 4700, 90, { scale: 0.9 }), item('k:coop', -100, 5150, 180), item('k:haystack', -1900, 3700, 0), item('k:haystack', -2050, 4300, 30), bldg('Well', -350, 3250, 0),
    item('WaterTrough', -550, 4050, 90), item('Cart', 850, 3200, 200), item('HayBale_Round', -1500, 3550, 40), item('Crate_B', 750, 4600, 0));
  fieldRect(S, 2200, 2850, 1700, 1300, 0, 'wheat', { fence: 'rail', broken: [5] });
  S.fences.push({ kind: 'stone', points: [[-2000, 3000], [-2000, 4600]] });
  const lanes = [FARM_ROAD, [[0, 0], [600, 1500], [300, 2800], [-200, 3400]], RANGE_ROAD, [[3600, -900], [4000, -2600], [4300, -3900]]];
  for (const lane of lanes) road(S, 'dirt', 380, lane, {});
  for (const lane of lanes.slice(0, 2)) S.fences.push({ kind: 'stone', points: offsetPolyline(lane, 330) }, { kind: 'stone', points: offsetPolyline(lane, -330) });
  S.fences.push({ kind: 'rail', points: offsetPolyline(lanes[2], -330) });
  S.noTrees.push(circle(0, 0, 1800), circle(300, 4000, 2100));
  // livestock and the people working the farms
  flock(S, 'sheep', 2350, -1850, 7, 500, 71);
  fieldRect(S, -4400, 700, 1600, 2000, 0, 'pasture', { fence: 'rail', gap: 2 });
  flock(S, 'cow', -4400, 700, 5, 600, 73); S.items.push(item('WaterTrough', -3900, 1350, 90), item('HayBale_Round', -4900, 1400, 20));
  flock(S, 'chicken', 150, 4950, 6, 380, 75);
  folk(S, -2900, -4600, 120, 2); folk(S, -6300, -1500, 90, 0); folk(S, 2500, 2600, 200, 3); folk(S, 820, 640, 220, 1); folk(S, 430, 280, 40, 0); folk(S, -4000, 300, 160, 2);
  slimeBog(S); spiderCliffDens(S); openGround(S, 16, 1600);
  S.hedges.push({ pts: offsetPolyline(FARM_ROAD, 560) }, { pts: offsetPolyline(lanes[1], -560) }, { pts: [[-4300, -5400], [-4300, -3400]] }, { pts: [[1300, 2050], [3100, 2050]] }, { pts: [[-7400, -700], [-5600, -700]] });
  S.veg.push({ kind: 'scatter', poly: [[-3500, -1500], [-1500, -2500], [1500, 1500], [-1500, 2500], [-3500, 1000]], n: 8, tree: 'oak' },
    { kind: 'scatter', poly: [[2000, -4200], [3600, -3200], [3600, -1200], [1500, -1000]], n: 5, tree: 'oak' },
    { kind: 'woods', poly: FOREST_ZONE, spacing: 740, mix: { pine: 3, oak: 3, birch: 1 } }, { kind: 'grove', X: 2600, Y: 6400, r: 900, n: 6, mix: { birch: 2 } },
    { kind: 'grove', X: 7300, Y: 1200, r: 1300, n: 9, mix: { pine: 2, oak: 1 } },
    { kind: 'flowers', poly: [[-1500, -3000], [-200, -2300], [-500, -900], [-2500, -1500]], spacing: 160 }, { kind: 'flowers', poly: [[3700, -600], [6400, -600], [6400, 500], [3700, 500]], spacing: 180 },
    { kind: 'bushes', poly: FOREST_ZONE, spacing: 900 });
  label(S, 'Cross Farm', 1300, -1500, 8, 'town'); label(S, 'Mill Cottage', 4400, -4500, 8, 'town'); label(S, 'Pond Farm', 0, 4300, 9, 'town'); label(S, 'Crossroads', 0, 0, 3, 'town');
  views(S, { name: 'Crossroads', X: -400, Y: 600, dist: 95, yaw: 40 });
  above(S, 'Slimes', 2400, -5600, 55, 60, 52, 0); above(S, 'Spiders', -2150, 6250, 50, 185, 46, 0);
  foot(S, 'Spawn', -5930, -4230, 35); foot(S, 'Lane', -2950, -1600, 32, -2); foot(S, 'Pond farm', -1400, 2600, 45, -2);
  foot(S, 'Slimes', 2500, -4150, -95, -8); foot(S, 'Spiders', -200, 6500, -171, -1); foot(S, 'Lookout', -4300, 2650, -30, -9);
  return S;
}

// ================================================================ 4 · The Hollow Way
function conceptHollowWay() {
  const S = newSpec(404);
  S.text = { name: '4 · The Hollow Way',
    tagline: 'A wooded island. One sunken lane winds through glades of oak, birch and pine to a woodcutters\' hamlet round a campfire; fallen logs, boulders and cairns mark the way and give cover.',
    bullets: ['<b>Hamlet</b>: a cottage, a cabin and a farmhouse round a clearing, with the well, the gun rack, a campfire and benches, cordwood stacks, a log yard with stumps, a shed and lamps.',
      '<b>Roads</b>: dirt lanes winding between the trees, fallen logs and boulders as barriers, cairns as waymarkers, the creek bridge kept.',
      '<b>Slimes</b>: Mossy Spring, a spring pool by the pond among mossy boulders, reeds and mushrooms, with a hunter\'s camp beside it.',
      '<b>Spiders</b>: the Pine Thicket, where the forest road runs between dead pines strung with webs right across the way.',
      '<b>Green</b>: woods over most of the island, with glades for the farm, the hamlet, the range and a flower meadow; bushes at every edge.',
      '<b>Life</b>: woodcutters at the log yard and the firewood, folk at the well and the campfire, hens and farmhands at the farm.'] };
  common(S, { orchard: { overgrown: true }, range: { fence: false }, meadows: { rocks: 34 } });
  house(S, 'Cottage', -1100, 1300, -45, { garden: { w: 700, d: 500 } });
  house(S, 'LogCabin', 1300, -1500, 180, { laundry: true });
  house(S, 'Farmhouse', -1750, -1050, 65, { scale: 0.9, garden: { w: 800, d: 550, scarecrow: true } });
  S.items.push(bldg('Well', 150, 250, 0), item('GunRack', 600, -500, 200), item('CampfireRing', -500, 700, 0), item('Bench', -500, 1150, 180), item('Bench', -950, 650, 90), item('Bench', -50, 650, 270),
    item('LampPost', -900, -500, 60), item('LampPost', 900, 400, 240), item('LampPost', 300, 1600, 330), item('FirewoodStack', 1500, -300, 90), item('FirewoodStack', 1500, 50, 90),
    item('FirewoodStack', 1900, -900, 0), item('FirewoodStack', -2200, 300, 0), item('Cart', 800, 1200, 140), item('Crate_A', 1300, 900, 20), item('Crate_B', 1600, 1000, 70),
    item('Barrel_A', -1300, -200, 0), item('Signpost', -200, -800, 15), item('Wheelbarrow', 1100, -800, 190), item('k:shed', 2200, 200, 270), item('k:stump', 1700, 500, 0), item('k:stump', 1950, 650, 0), item('k:stump', 1750, 850, 0),
    { model: 'k:log', X: 2300, Y: 900, yaw: 80 }, { model: 'k:log', X: 2500, Y: 1100, yaw: 85, lift: 0.0 });
  S.patches.push({ poly: circlePoly(100, 100, 1350, 18, 0.15, 3), color: PAL.dirtEdge });
  label(S, 'Woodcutters\' hamlet', 100, 100, 6, 'town');
  const lane = [[-5350, -3750], [-4300, -3200], [-3700, -2200], [-2600, -1700], [-1600, -600], [0, 0], [1600, -300], [2700, -1000], [3600, -900], [4600, -1200]];
  road(S, 'dirt', 400, lane); road(S, 'dirt', 380, FOREST_ROAD);
  S.items.push({ model: 'k:log', X: -3300, Y: -2500, yaw: 30 }, { model: 'k:log', X: -2000, Y: -600, yaw: 100 }, { model: 'k:log', X: 2300, Y: 2900, yaw: 60 }, { model: 'k:log', X: 3900, Y: 4900, yaw: 150 },
    { model: 'k:log', X: 3100, Y: -1500, yaw: 20 }, item('Boulder_B', -4100, -2800, 0), item('Boulder_A', 1000, 1900, 40), item('Boulder_C', 3300, 3700, 70), item('Rock_A', -2700, -1300, 0), item('Rock_B', 2200, -1300, 0),
    item('Cairn_A', -4300, -3600, 0), item('Cairn_B', -2600, -2100, 0), item('Cairn_C', -600, -900, 0), item('Cairn_A', 1400, 1500, 0), item('Cairn_B', 2900, 2900, 0), item('Cairn_C', 4000, 4000, 0),
    item('k:barricade', 3200, -700, 100));
  S.noTrees.push(circle(100, 100, 2300));
  folk(S, 1750, 650, 250, 2); folk(S, 1600, -150, 90, 0); folk(S, -500, 1000, 180, 3); folk(S, -150, 420, 60, 1); folk(S, -2900, -2350, 40, 0);
  slimeSpring(S); spiderThicket(S);
  const clear = [circle(-5300, -4300, 3200), circle(100, 100, 2400), circle(5000, -1500, 2300), circle(2800, -4800, 1900), circle(1650, 4300, 1500), circle(6200, -3800, 1000), circle(-5900, -6900, 1500)];
  S.veg.push({ kind: 'woods', poly: INNER, spacing: 700, mix: { oak: 4, birch: 3, pine: 3 }, clear },
    { kind: 'woods', poly: FOREST_ZONE, spacing: 520, mix: { pine: 6, oak: 1 }, clear: [circle(4250, 6500, 1300)] },
    { kind: 'woods', poly: PLATEAU.polygon, spacing: 880, mix: { pine: 3, birch: 1 }, clear: [circle(-6200, 6000, 1300), circle(JETTY.X, JETTY.Y, 1300), circle(-4300, 2650, 1300)] },
    { kind: 'flowers', poly: circlePoly(2800, -4800, 1750, 14), spacing: 140 }, { kind: 'flowers', poly: circlePoly(100, 100, 2200, 14), spacing: 230 },
    { kind: 'bushes', poly: INNER, spacing: 1050 });
  label(S, 'Flower glade', 2800, -4800, 2);
  views(S, { name: 'Hamlet', X: 100, Y: 100, dist: 72, yaw: 30 });
  above(S, 'Slimes', 1650, 4300, 50, 60, 52, 0); above(S, 'Spiders', 4250, 6500, 55, 30, 52, 0);
  foot(S, 'Spawn', -5930, -4230, 35); foot(S, 'Hamlet', 500, -900, 125, -4); foot(S, 'Lane', -4000, -2900, 40, -2);
  foot(S, 'Slimes', -150, 3650, 25, -4); foot(S, 'Spiders', 3100, 3800, 45, 0); foot(S, 'Lookout', -4300, 2650, -30, -9);
  return S;
}
const CONCEPTS = [conceptCrossroads, conceptMainStreet, conceptHillFarms, conceptHollowWay];
const CONCEPT_NAMES = ['Crossroads Town', 'Main Street', 'Hill Farms', 'Hollow Way'];
