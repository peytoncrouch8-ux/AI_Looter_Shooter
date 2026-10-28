// The user's pick, round three: "Hulltown in Crescent Bay". Concept A's horseshoe harbor with Concept F's beached,
// broken wreck town (the Sovereign) on it, in two variants:
//   H1 Along the Curve: the wreck's three sections lie along the bay's inner shore, the town in the stern.
//   H2 The Spine: the wreck runs bow-first out of the curve across the bay to the mouth, splitting it in two.
// Same page as D-G plus a boarding row: Ned's crew are hostile from the first step aboard.
//   node Art/Concepts/Areas/GildedLily/gen3.js [H1|H2 ...] [--no-png]
const fs = require('fs');
const path = require('path');
const L = require('./lib');
const K = require('./lib2');
const { C, f1, text, chip } = L;
const here = __dirname;
const SHORE_M2 = 95000, SHIP_M = 180, SHIFT = 450;

// ---------- the shared bay ----------
function bay() {
	const coast = L.roughen([[1400, -80], [560, -80], [540, 0], [505, 40], [470, 52], [452, 90], [420, 130], [382, 160], [352, 200], [356, 236], [322, 250], [292, 280], [276, 322], [286, 362], [322, 382], [392, 394], [472, 390], [562, 372], [652, 364], [742, 380], [812, 422], [862, 482], [887, 552], [890, 622], [868, 692], [822, 752], [752, 797], [662, 817], [562, 814], [472, 798], [402, 787], [342, 802], [302, 842], [286, 900], [300, 948], [278, 972], [296, 1004], [336, 1030], [370, 1100], [1400, 1100]], true, 4, 601, 10);
	const ridge = L.roughen([[1400, -80], [800, -80], [790, 60], [850, 170], [930, 270], [985, 390], [1005, 520], [995, 640], [1005, 750], [955, 850], [885, 950], [850, 1100], [1400, 1100]], true, 7, 602, 8);
	const rocks = [[[200, 140], [224, 130], [236, 150], [216, 166]], [[180, 290], [200, 282], [210, 300], [190, 312]], [[200, 1060], [220, 1052], [230, 1070], [210, 1080]]].map((r, i) => L.roughen(r, true, 2, 610 + i, 6));
	const headland = [[540, 20], [580, 90], [560, 200], [500, 280], [400, 320], [316, 330], [350, 250], [410, 170], [470, 96]];
	return { coast, ridge, rocks, headland };
}
// A wreck section: kind 'bow' (pointed at +t), 'stern' (rounded at -t) or 'mid' (torn both ends).
function section(cx, cy, len, wd, ang, kind, seed) {
	const a = ang * Math.PI / 180, ca = Math.cos(a), sa = Math.sin(a);
	const P = (t, v) => [cx + ca * t * len - sa * v * wd, cy + sa * t * len + ca * v * wd];
	const inT = p => { const dx = p[0] - cx, dy = p[1] - cy; return [(dx * ca + dy * sa) / len, (-dx * sa + dy * ca) / wd]; };
	const half = t => kind === 'bow' && t > 0.28 ? 0.5 * Math.max(0.04, Math.cos((t - 0.28) / 0.22 * Math.PI / 2)) : kind === 'stern' && t < -0.4 ? 0.5 * Math.sqrt(Math.max(0.02, 1 - ((-0.4 - t) / 0.1) ** 2)) : 0.5;
	const pts = [];
	for (let k = 0; k <= 30; k++) { const t = -0.5 + k / 30; pts.push(P(t, -half(t))); }
	for (let k = 30; k >= 0; k--) { const t = -0.5 + k / 30; pts.push(P(t, half(t))); }
	const poly = L.roughen(pts, true, kind === 'mid' ? 3 : 2, seed, 3);
	const inside = p => { const [t, v] = inT(p); return Math.abs(t) <= 0.5 && Math.abs(v) <= half(t); };
	return { P, inT, poly, inside, kind, ang, len, wd };
}
function paintWreck(secs, seed) {
	const R = L.rng(seed); let s = '';
	for (const sc of secs) s += `<path d="${L.d(sc.poly, true)}" fill="#000" opacity="0.45" transform="translate(7 9)"/><path d="${L.d(sc.poly, true)}" fill="#5e3424" stroke="#1d0f08" stroke-width="3"/>`;
	s += `<clipPath id="wreckClip${seed}"><path d="${secs.map(sc => L.d(sc.poly, true)).join(' ')}"/></clipPath><g clip-path="url(#wreckClip${seed})">`;
	for (const sc of secs) {
		for (let t = -0.5; t < 0.5; t += 0.04) s += `<path d="${L.d([sc.P(t, -0.5), sc.P(t, 0.5)])}" stroke="#3e2216" stroke-width="1.5" opacity="0.7"/>`;
		for (const v of [-0.3, 0, 0.3]) s += `<path d="${L.d([sc.P(-0.5, v), sc.P(0.5, v)])}" stroke="#7a4a30" stroke-width="2" opacity="0.6"/>`;
		for (let k = 0; k < 30; k++) { const [x, y] = sc.P(R() - 0.5, (R() - 0.5) * 0.9); s += `<ellipse cx="${f1(x)}" cy="${f1(y)}" rx="${f1(4 + R() * 12)}" ry="${f1(3 + R() * 7)}" fill="${R() < 0.5 ? '#8a4a2a' : '#2a160e'}" opacity="0.6"/>`; }
		for (const [t, v] of [[-0.2, -0.15], [0.15, 0.12]]) { const [x, y] = sc.P(t, v); s += `<ellipse cx="${f1(x)}" cy="${f1(y)}" rx="16" ry="10" fill="#140a06" stroke="#7a4a30" stroke-width="2" transform="rotate(${sc.ang} ${f1(x)} ${f1(y)})"/>`; }
	}
	s += `<rect x="-50" y="-50" width="1400" height="1100" filter="url(#mottleDark)" opacity="0.5"/></g>`;
	// ribs sticking out of the torn ends
	for (const sc of secs) for (const te of sc.kind === 'bow' ? [-0.5] : sc.kind === 'stern' ? [0.5] : [-0.5, 0.5]) for (let k = 0; k < 6; k++) { const v = -0.42 + k * 0.17; s += `<path d="${L.d([sc.P(te, v), sc.P(te + (te > 0 ? 0.06 : -0.06), v + 0.03)])}" stroke="#7a4a30" stroke-width="3"/>`; }
	return s;
}
function shacks(sc, n, seed, out, tr = [-0.45, 0.45]) {
	const R = L.rng(seed); let s = '', lamps = '';
	for (let k = 0; k < n; k++) { const t = tr[0] + R() * (tr[1] - tr[0]), v = (R() - 0.5) * 0.75; const [x, y] = sc.P(t, v); s += L.roof(x, y, 12 + R() * 8, 9 + R() * 6, sc.ang + (R() - 0.5) * 24, C.roofs[Math.floor(R() * C.roofs.length)]); if (out) out.push([x, y]); if (R() < 0.6) lamps += L.lamp(x + 6, y + 4, 0.6); }
	return s + lamps;
}

// ---------- common map painter: bay, horns, districts; the variant adds the wreck, town, roads and marks ----------
function baseMap(B, wreckSecs, isPlayExtra) {
	const isLandG = p => L.pip(p, B.coast) || L.inAny(p, B.rocks);
	const inWreck = p => wreckSecs.some(sc => sc.inside(p));
	const isPlay = p => (L.pip(p, B.coast) && !L.pip(p, B.ridge)) || inWreck(p) || (isPlayExtra && isPlayExtra(p));
	const isWater = p => !isLandG(p) && !inWreck(p);
	const coastD = L.d(B.coast, true);
	let m = L.waterLayer([coastD, ...B.rocks.map(r => L.d(r, true)), ...wreckSecs.map(sc => L.d(sc.poly, true))]) + L.waves(isWater, 700, 17);
	m += L.flats([[150, 900], [276, 880], [290, 980], [330, 1040], [360, 1100], [180, 1100], [110, 1000]], 620);
	m += L.landLayer('landH', coastD, [L.d(L.roughen([[600, -60], [820, -60], [840, 250], [760, 300], [620, 250], [580, 140]], true, 8, 621, 6), true), L.d(L.roughen([[880, 380], [1010, 380], [1010, 900], [800, 920], [870, 700], [900, 520]], true, 8, 622, 6), true), L.d(L.roughen([[460, 880], [840, 880], [880, 1080], [500, 1080], [440, 980]], true, 8, 623, 6), true)]);
	m += B.rocks.map(r => `<path d="${L.d(r, true)}" fill="#6e5c4c" stroke="#2b211b" stroke-width="1.5"/>`).join('');
	m += L.hill(B.headland, 624, 3);
	m += L.ridgeLayer([L.d(B.ridge, true)], [], 'landH');
	m += L.cliff([[540, 0], [505, 40], [470, 52], [452, 90], [420, 130], [382, 160], [352, 200], [356, 236], [322, 250], [292, 280], [276, 322], [286, 362]], -1, 625, { len: 14 });
	m += L.cliff([[790, 60], [850, 170], [930, 270], [985, 390], [1005, 520], [995, 640], [1005, 750], [955, 850], [885, 950]], 1, 626, { len: 10, w: 3 });
	// north shore docks, south horn camp, caves, wrecks
	for (const x of [470, 560]) m += L.pier([x, 392 - (x - 470) * 0.05], [x - 4, 470], 11, { head: 16 });
	m += L.dryDock(640, 392, 92, -4);
	for (const [x, y, w, h, a] of [[520, 340, 56, 22, -8], [610, 330, 50, 20, -4]]) m += L.warehouse(x, y, w, h, a);
	for (const [x, y, r] of [[690, 340, 12], [716, 330, 10], [670, 326, 9]]) m += L.coalHeap(x, y, r);
	for (const [x, y, a] of [[480, 398, 90], [568, 392, 90]]) m += L.crane(x, y, a);
	m += L.wreck(345, 960, 40, 30) + L.wreck(212, 150, 34, -40) + L.wreck(320, 880, 28, 70);
	for (const [x, y] of [[372, 940], [390, 962], [360, 984]]) m += `<circle cx="${x}" cy="${y}" r="5" fill="#d9c3a0" stroke="#3a2a1a"/>`;
	m += `<circle cx="378" cy="962" r="10" fill="url(#lampGlow)"/>`;
	m += L.cave(468, 72, 30) + L.cave(370, 224, 60) + L.cave(282, 986, 90);
	m += L.rocks(p => !isWater(p) && L.pip(p, B.headland), [300, 20, 580, 330], 36, 627);
	m += L.rocks(isWater, [160, 100, 300, 340], 16, 628, { r: 3 });
	m += L.trees(p => L.pip(p, B.ridge) && L.pip(p, B.coast), [780, 0, 1300, 1020], 160, 629, { col: '#2b3720', r: 6 });
	m += L.lighthouse(306, 344, 172, 560);
	return { m, isPlay, isWater, inWreck };
}
const lilyAt = (m, shipPx) => m + L.liner(110, 600, shipPx, -90, 61);

// ======================================================================= H1: Along the Curve
function conceptH1() {
	const B = bay();
	const secs = [section(724, 790, 210, 72, -24, 'stern', 631), section(852, 622, 180, 70, -82, 'mid', 632), section(772, 436, 220, 70, -140, 'bow', 633)];
	const [st, md, bw] = secs;
	const base = baseMap(B, secs);
	const px = L.playArea(base.isPlay), mpp = Math.sqrt(SHORE_M2 / px), shipPx = SHIP_M / mpp;
	console.log(`H1: play ${px}, mpp ${mpp.toFixed(3)}, ship ${shipPx.toFixed(0)} px`);
	let m = base.m;
	const ring = [[560, 870], [700, 880], [820, 850], [920, 760], [950, 650], [945, 540], [905, 440], [840, 380], [760, 350], [660, 352], [560, 355], [470, 362], [400, 360]];
	const south = [[560, 870], [460, 860], [380, 900], [330, 960]];
	const headRd = [[470, 362], [450, 300], [400, 300], [330, 345]];
	m += L.road(ring, 9) + L.road(south, 7) + L.road(headRd, 6, { col: '#bfa27a' });
	const railPts = [[1400, 880], [1180, 880], [1000, 868], [880, 836], [800, 812]];
	m += `<path d="${L.smooth(railPts, false, 6)}" fill="none" stroke="#7a6450" stroke-width="24" stroke-linecap="round" opacity="0.9"/>` + L.rail(railPts) + L.rail([[880, 836], [940, 700], [950, 560], [910, 450], [840, 370], [720, 340]]);
	m += paintWreck(secs, 634);
	const bld = [];
	m += shacks(st, 46, 635, bld, [-0.35, 0.45]) + shacks(md, 14, 636, bld) + shacks(bw, 12, 637, bld, [-0.45, 0.1]);
	m += L.roof(...st.P(0.05, 0.05), 60, 18, -24, '#4a3a30') + L.roof(...st.P(-0.32, -0.05), 40, 28, -24, '#3a1418', { big: true });
	const tn = L.town(ring.slice(0, 5), { step: 22, off: 14, w: 17, h: 14, sides: [-1], gaps: 0.2, skip: p => !base.isPlay(p) || base.inWreck(p) }, 638, bld);
	m += tn.s + tn.lamps + L.roof(960, 820, 34, 20, -30, '#e8e0cc', { big: true }) + L.roof(990, 720, 36, 24, -80, C.velvet, { big: true });
	for (const t of [-0.3, 0.0, 0.3]) m += L.crane(...md.P(t, -0.3), md.ang + 180);
	for (const t of [-0.2, 0.2]) m += L.pier(md.P(t, -0.5), md.P(t, -0.95), 10, { head: 12 });
	m += L.bridge(st.P(0.5, 0), md.P(-0.5, 0), 11) + L.bridge(md.P(0.5, 0), bw.P(-0.5, 0), 11);
	m += L.pier(bw.P(0.48, 0), [690, 430], 12, { head: 18, col: '#9a7044' });
	m += L.trees(p => base.isPlay(p) && !base.inWreck(p) && L.lineDist(p, ring) > 14 && bld.every(q => L.dist(p, q) > 15), [300, 0, 1010, 1080], 120, 639, { r: 6 });
	for (const [x, y, a, l, k] of [[640, 520, 70, 30, 2], [670, 512, 70, 30, 2], [560, 640, 20, 36, 0], [470, 700, -10, 26, 1], [700, 720, 30, 24, 1]]) m += L.boat(x, y, a, l, k);
	m = lilyAt(m, shipPx);
	m += L.seaRoute([[690, 436], [560, 520], [400, 660], [250, 760], [140, 790]]) + L.launchBoat(400, 660, 140);
	m += K.fight(...md.P(0, 0), 40, 'crew', 'F1', mpp) + K.fight(430, 220, 34, 'beast', 'F2', mpp) + K.fight(206, 950, 56, 'beast', 'F3', mpp) + K.fight(330, 996, 32, 'crew', 'F4', mpp, { below: false }) + K.fight(...bw.P(-0.05, 0), 28, 'bounty', 'F5', mpp, { below: true });
	m += L.route([[800, 812], st.P(0.05, 0.05), st.P(-0.32, -0.05)]);
	m += L.route([st.P(-0.32, -0.05), st.P(0.3, -0.25), md.P(-0.3, -0.25), md.P(0.3, -0.25), bw.P(-0.3, -0.25), bw.P(0.4, 0), [690, 430]]);
	m += L.route([[560, 870], [400, 890], [330, 950]], { w: 3, dash: '5 7' }) + L.route([[560, 355], [470, 300], [340, 340]], { w: 3, dash: '5 7' }) + L.route([[920, 760], [960, 820]], { w: 3, dash: '5 7' });
	m += L.step(836, 860, 1) + L.step(...st.P(0.2, 0.9), 2) + L.step(...st.P(-0.38, -0.85), 3) + L.step(700, 470, 4) + L.step(160, 820, 5) + L.step(60, 720, 6);
	m += L.giver(...st.P(-0.2, 0.25)) + L.giver(960, 790) + L.giver(640, 360);
	m += L.chest(468, 100) + L.chest(...md.P(0.1, 0.2)) + L.chest(350, 990) + L.chest(...bw.P(0.2, 0.2));
	m += L.secret(370, 250) + L.secret(...st.P(-0.45, 0.1));
	m += K.bigLabel(1010, 960, 'Hulltown', 'the port, in the stern', { size: 30 }) + K.bigLabel(760, 640, 'The Sovereign', null, { size: 22, rot: -82 });
	m += K.bigLabel(560, 600, 'Crescent Bay', null, { size: 28, fill: '#bfeaff' }) + K.bigLabel(640, 288, 'Docks & Shipyards', null, { size: 20 });
	m += K.bigLabel(440, 30, 'Sea Cliffs', null, { size: 22 }) + K.bigLabel(600, 960, 'Beaches & Tide Flats', null, { size: 22 }) + K.bigLabel(110, 380, 'The Lily', null, { size: 22, fill: C.lamp });
	return {
		key: 'H1', name: 'Hulltown Bay: The Curve', kind: 'THE PICK · VARIANT 1', tag: 'OPEN HUB + IN AND OUT', area: Math.round(px * mpp * mpp), mpp, map: m, scaleAt: [40, 120], compassAt: [1250, 60], shift: SHIFT,
		subtitle: 'Crescent Bay with the Sovereign beached along its curve in three pieces: the town in the stern, salvage yards on the midsection, the bow run up on the north shore.',
		plays: ['Open hub: the wreck lines the bay, and Wreck Row, the corridor', 'through all three pieces, is the main street. Every district is', 'a short walk off it; inside is tight, the shore outside is open.'],
		path: ['Train into the station in the stern', 'Port missions: Hulltown, yards, cliffs, beaches', 'Win the marker: the Gilded Parlor, stern saloon', 'Wreck Row to the launch stage on the bow', 'Board at the stern ladder: they are waiting', 'The ballroom: Ned deals'],
		fights: [['crew', 'F1 the midsection', "Ned's crew hold the holds"], ['beast', 'F2 gull cliffs', 'gulls round the light'], ['beast', 'F3 the flats', 'crabs, eels in the pools'], ['crew', 'F4 smugglers\' cove', 'sharps and grave gold'], ['bounty', 'F5 the bow hold', 'a bounty in the flooded bow']],
		deck: {
			how: "Ned's crew are hostile from the first step aboard",
			route: [{ k: 'a', deck: 0, t: -0.44, v: 0.02, say: 'stern ladder: fight the aft deck' }, { k: 'b', deck: 1, t: 0.08, v: 0, pre: [[-0.3, 0.38], [-0.16, 0.38]], via: -0.16, post: [[-0.12, 0]], say: 'grand staircase: casino brawl' }, { k: 'c', deck: 2, t: 0.375, v: 0, via: 0.05, say: 'down: the vault' }, { k: 'd', deck: 0, t: 0.288, v: 0, via: 0.37, say: 'cargo hatch up: the bridge' }, { k: 'e', deck: 1, t: -0.335, v: 0.12, pre: [[0.24, -0.37], [-0.16, -0.37]], via: -0.16, say: 'back down: the ballroom' }],
			fights: [{ deck: 0, t: -0.4, v: 0, d: 16, tag: 'FIRST FIGHT' }, { deck: 1, t: 0.08, v: 0, d: 22, tag: 'casino brawl', below: true }, { deck: 2, t: 0.375, v: 0, d: 16, tag: 'vault guard', below: true }, { deck: 0, t: 0.288, v: 0, d: 14, tag: 'bridge', below: true }, { deck: 1, t: -0.335, v: 0, d: 22, tag: 'NED', below: true }],
		},
		sketch: arriveH1(),
		boarding: cn => boardingRow(cn, { focus: [0, -0.39, 0.02], k: 1.45, crew: [[-0.46, -0.3], [-0.41, 0.33], [-0.36, -0.12], [-0.33, 0.22], [-0.42, 0.06], [-0.31, -0.32]], entry: { from: [-0.6, 0.0], to: [-0.47, 0.0] }, sketch: boardStern(),
			title: 'BOARDING AT THE STERN LADDER', lines: ["Ned's crew know who you are:", 'they line the aft rail and open', 'fire as the launch comes under', 'the stern. First fight: the aft', 'deck, 16 m across, deck chairs', 'and the aft mast for cover.', 'Win it and the grand staircase', 'is open below.'] }),
	};
}

// ======================================================================= H2: The Spine
function conceptH2() {
	const B = bay();
	const secs = [section(790, 600, 210, 74, 0, 'bow', 641), section(573, 604, 196, 72, 2, 'mid', 642), section(372, 598, 176, 70, -3, 'stern', 643)];
	const [bw, md, st] = secs;
	const base = baseMap(B, secs);
	const px = L.playArea(base.isPlay), mpp = Math.sqrt(SHORE_M2 / px), shipPx = SHIP_M / mpp;
	console.log(`H2: play ${px}, mpp ${mpp.toFixed(3)}, ship ${shipPx.toFixed(0)} px`);
	let m = base.m;
	m += L.flats([[440, 660], [560, 652], [700, 660], [790, 700], [760, 780], [640, 800], [500, 784], [420, 730]], 644);
	const north = [[950, 610], [940, 500], [890, 430], [820, 395], [720, 380], [620, 370], [520, 370], [450, 362], [400, 340]];
	const southR = [[950, 620], [930, 720], [870, 790], [770, 830], [650, 850], [540, 848], [440, 860], [370, 920], [330, 970]];
	m += L.road(north, 9) + L.road(southR, 9) + L.road([[450, 362], [420, 300], [330, 345]], 6, { col: '#bfa27a' });
	const railPts = [[1400, 600], [1200, 604], [1010, 606], [900, 604], [860, 602]];
	m += `<path d="${L.smooth(railPts, false, 6)}" fill="none" stroke="#7a6450" stroke-width="24" stroke-linecap="round" opacity="0.9"/>` + L.rail(railPts);
	m += paintWreck(secs, 645);
	const bld = [];
	m += shacks(bw, 40, 646, bld, [-0.45, 0.28]) + shacks(md, 14, 647, bld) + shacks(st, 10, 648, bld, [-0.1, 0.45]);
	m += L.roof(...bw.P(0.2, 0.0), 60, 18, 0, '#4a3a30') + L.roof(...bw.P(-0.3, 0.05), 40, 28, 0, '#3a1418', { big: true });
	const tn = L.town(north.slice(0, 4), { step: 22, off: 14, w: 17, h: 14, sides: [1], gaps: 0.2, skip: p => !base.isPlay(p) || base.inWreck(p) }, 649, bld);
	const ts = L.town(southR.slice(0, 4), { step: 22, off: 14, w: 17, h: 14, sides: [-1], gaps: 0.2, skip: p => !base.isPlay(p) || base.inWreck(p) }, 650, bld);
	m += tn.s + ts.s + tn.lamps + ts.lamps + L.roof(975, 520, 34, 20, -70, '#e8e0cc', { big: true }) + L.roof(960, 700, 36, 24, 80, C.velvet, { big: true });
	for (const t of [-0.3, 0.0, 0.3]) m += L.crane(...md.P(t, -0.3), -90);
	for (const t of [-0.2, 0.2]) m += L.pier(md.P(t, -0.5), md.P(t, -1.2), 10, { head: 12 });
	m += L.bridge(bw.P(-0.5, 0), md.P(0.5, 0), 11) + L.bridge(md.P(-0.5, 0), st.P(0.5, 0), 11);
	m += L.pier(st.P(-0.48, 0), [258, 598], 12, { head: 18, col: '#9a7044' });
	m += L.trees(p => base.isPlay(p) && !base.inWreck(p) && [north, southR].every(l => L.lineDist(p, l) > 14) && bld.every(q => L.dist(p, q) > 15), [300, 0, 1010, 1080], 110, 651, { r: 6 });
	for (const [x, y, a, l, k] of [[640, 470, 90, 30, 2], [670, 470, 90, 30, 2], [460, 480, 20, 36, 0], [520, 740, -10, 22, 1], [760, 480, 30, 24, 1]]) m += L.boat(x, y, a, l, k);
	m = lilyAt(m, shipPx);
	m += L.seaRoute([[258, 598], [200, 580], [140, 560]]) + L.launchBoat(206, 580, 190);
	m += K.fight(...md.P(0, 0), 40, 'crew', 'F1', mpp) + K.fight(430, 220, 34, 'beast', 'F2', mpp) + K.fight(620, 740, 56, 'beast', 'F3', mpp) + K.fight(310, 994, 32, 'crew', 'F4', mpp) + K.fight(...st.P(0.05, 0), 28, 'bounty', 'F5', mpp, { below: true });
	m += L.route([[880, 604], bw.P(0.2, 0.25), bw.P(-0.3, 0.25)]);
	m += L.route([bw.P(-0.3, -0.1), md.P(0.3, -0.1), md.P(-0.3, -0.1), st.P(0.3, -0.1), st.P(-0.4, 0), [262, 598]]);
	m += L.route([[950, 610], [890, 430], [600, 370], [420, 330], [340, 340]], { w: 3, dash: '5 7' }) + L.route([[950, 620], [870, 790], [650, 850], [440, 860], [340, 960]], { w: 3, dash: '5 7' });
	m += L.step(906, 560, 1) + L.step(1000, 610, 2) + L.step(...bw.P(-0.3, 0.95), 3) + L.step(300, 640, 4) + L.step(176, 530, 5) + L.step(60, 700, 6);
	m += L.giver(...bw.P(-0.12, 0.25)) + L.giver(975, 545) + L.giver(560, 380);
	m += L.chest(468, 100) + L.chest(...md.P(0.1, 0.2)) + L.chest(350, 990) + L.chest(...st.P(0.3, 0.2));
	m += L.secret(370, 250) + L.secret(...bw.P(0.4, -0.1));
	m += K.bigLabel(1080, 470, 'Hulltown', 'the port, in the bow', { size: 30 }) + K.bigLabel(573, 506, 'The Sovereign', null, { size: 22 });
	m += K.bigLabel(560, 444, 'The Harbor', null, { size: 22, fill: '#bfeaff' }) + K.bigLabel(470, 790, 'The Lagoon', null, { size: 22, fill: '#bfeaff' });
	m += K.bigLabel(640, 288, 'Docks & Shipyards', null, { size: 20 }) + K.bigLabel(440, 30, 'Sea Cliffs', null, { size: 22 }) + K.bigLabel(620, 960, 'Beaches & Tide Flats', null, { size: 22 }) + K.bigLabel(110, 380, 'The Lily', null, { size: 22, fill: C.lamp });
	return {
		key: 'H2', name: 'Hulltown Bay: The Spine', kind: 'THE PICK · VARIANT 2', tag: 'OPEN HUB + IN AND OUT', area: Math.round(px * mpp * mpp), mpp, map: m, scaleAt: [40, 120], compassAt: [1250, 60], shift: SHIFT,
		subtitle: 'Crescent Bay with the Sovereign run bow-first out of the curve: its three pieces cross the bay to the mouth, a harbor north of it and a lagoon of flats south.',
		plays: ['Open hub at the root: the town in the bow, a ring road north', 'to the docks and cliffs, south to the beaches. The wreck is the', 'only way across the bay, inside, all the way out to the launch.'],
		path: ['Train into the station in the beached bow', 'Port missions round both halves of the bay', 'Win the marker: the Gilded Parlor, in the bow', 'Through the wreck to the stern\'s launch stage', 'Board by the starboard gangway, under fire', 'The ballroom: Ned deals'],
		fights: [['crew', 'F1 the midsection', "Ned's crew hold the holds"], ['beast', 'F2 gull cliffs', 'gulls round the light'], ['beast', 'F3 the lagoon', 'crabs on the flats, eels'], ['crew', 'F4 smugglers\' cove', 'sharps and grave gold'], ['bounty', 'F5 the stern', 'a bounty on the launch stage']],
		deck: {
			how: "Ned's crew are hostile from the first step aboard",
			route: [{ k: 'a', deck: 0, t: 0.0, v: 0.3, say: 'starboard gangway: promenade fight' }, { k: 'b', deck: 0, t: 0.288, v: 0, pre: [[0.24, 0.37]], say: 'forward: the bridge' }, { k: 'c', deck: 2, t: 0.375, v: 0, via: 0.37, say: 'cargo hatch down: the vault' }, { k: 'd', deck: 1, t: 0.08, v: 0, via: 0.05, say: 'service stairs up: the casino' }, { k: 'e', deck: 1, t: -0.335, v: 0.12, pre: [[-0.16, 0]], say: 'grand staircase: Ned' }],
			fights: [{ deck: 0, t: 0.0, v: 0.24, d: 14, tag: 'FIRST FIGHT' }, { deck: 0, t: 0.288, v: 0, d: 14, tag: 'bridge' }, { deck: 2, t: 0.375, v: 0, d: 16, tag: 'vault guard', below: true }, { deck: 1, t: 0.08, v: 0, d: 22, tag: 'casino brawl', below: true }, { deck: 1, t: -0.335, v: 0, d: 22, tag: 'NED', below: true }],
		},
		sketch: arriveH2(),
		boarding: cn => boardingRow(cn, { focus: [0, 0.0, 0.14], k: 1.5, crew: [[-0.06, 0.37], [0.04, 0.39], [0.0, 0.2], [-0.07, 0.12], [0.07, 0.3], [0.03, -0.37]], entry: { from: [0.0, 0.62], to: [0.0, 0.48] }, sketch: boardSide(),
			title: 'BOARDING BY THE STARBOARD GANGWAY', lines: ['The launch runs in alongside', 'and the gangway is already', 'swarming. First fight: the', 'starboard promenade, 14 m,', 'lifeboats and the deckhouse', 'wall for cover; more crew come', 'over the roof from port.', 'Then push forward to the bridge.'] }),
	};
}

// ---------- arrival sketches (eye level, from the station platform) ----------
function rustHull(pts, rim = '#e2774a') { return `<path d="${L.d(pts, true)}" fill="#3a2018"/><path d="${L.d(pts.slice(0, 2))}" stroke="${rim}" stroke-width="2" opacity="0.6"/>`; }
function arriveH1() {
	const { w, h } = K.SK, hz = 190;
	let s = K.skSky(w, h, hz, 150);
	s += K.skLiner(110, hz + 2, 70);
	s += K.skShape([[-10, 150], [60, 140], [120, 170], [150, 196], [-10, 210]], '#241a2e', '#e2774a', 0.6) + `<circle cx="70" cy="132" r="18" fill="url(#lampGlow)"/><rect x="66" y="132" width="8" height="14" fill="#e8dcc0"/>`;
	// the wreck's pieces curving round the bay to the right, nearest largest
	s += rustHull([[250, 200], [380, 186], [384, 214], [250, 222]]) + rustHull([[390, 184], [520, 160], [528, 220], [392, 216]]) + rustHull([[536, 150], [w + 10, 70], [w + 10, 250], [540, 226]]);
	for (let k = 0; k < 7; k++) s += `<path d="M${550 + k * 18},${148 - k * 11} L${552 + k * 18},${226}" stroke="#5a3422" stroke-width="4"/>`;
	s += K.skTown(256, 380, 190, 4, 10, 71, '#1a1012', { lit: 0.6 }) + K.skTown(394, 520, 172, 5, 14, 72, '#1a1012', { lit: 0.6 }) + K.skTown(560, w, 150, 8, 24, 73, '#1a1012', { lit: 0.55 });
	s += `<rect y="226" width="${w}" height="${h - 226}" fill="#120c12" opacity="0.5"/>`;
	s += `<path d="M0,0 L${w},0 L${w},40 Q${w / 2},90 0,40 Z" fill="#2a1612"/>`;
	for (let k = 0; k < 12; k++) s += `<path d="M${k * 60},0 Q${k * 60 + 10},50 ${k * 60 + 30},72" stroke="#4a2a1c" stroke-width="7" fill="none"/>`;
	s += K.skPlatform(w, h, { sign: 'HULLTOWN', lampX: 290, lampX2: 470, canopy: 1 });
	return s;
}
function arriveH2() {
	const { w, h } = K.SK, hz = 180, vx = 300;
	let s = K.skSky(w, h, hz, vx - 40);
	s += K.skLiner(vx - 60, hz + 2, 60);
	s += K.skShape([[w + 10, 140], [560, 150], [470, 176], [440, 190], [w + 10, 200]], '#241a2e', '#e2774a', 0.6) + `<circle cx="590" cy="140" r="18" fill="url(#lampGlow)"/><rect x="586" y="140" width="8" height="14" fill="#e8dcc0"/>`;
	// the wreck's spine running straight away to the mouth: deck in perspective with shacks either side
	s += `<path d="M${vx - 6},${hz + 6} L${vx + 6},${hz + 6} L${w - 40},${h} L40,${h} Z" fill="#3a2018"/><path d="M${vx - 6},${hz + 6} L40,${h}" stroke="#e2774a" stroke-width="2" opacity="0.5"/><path d="M${vx + 6},${hz + 6} L${w - 40},${h}" stroke="#e2774a" stroke-width="2" opacity="0.5"/>`;
	for (let k = 1; k < 9; k++) { const t = k / 9, y = hz + 6 + (h - hz - 6) * t * t, x0 = vx - 6 + (40 - vx + 6) * t * t, x1 = vx + 6 + (w - 40 - vx - 6) * t * t; s += `<path d="M${f1(x0)},${f1(y)} L${f1(x1)},${f1(y)}" stroke="#5a3422" stroke-width="${f1(1 + t * 3)}"/>`; }
	for (let k = 0; k < 14; k++) { const t = 0.15 + k / 16, y = hz + 6 + (h - hz - 6) * t * t, sc = t * t; for (const sd of [-1, 1]) { const x = vx + sd * (14 + (w / 2 - 60) * sc); const bw = 6 + 60 * sc, bh = 5 + 50 * sc; s += `<rect x="${f1(sd < 0 ? x - bw : x)}" y="${f1(y - bh)}" width="${f1(bw)}" height="${f1(bh)}" fill="#1a1012"/><rect x="${f1((sd < 0 ? x - bw : x) + bw * 0.3)}" y="${f1(y - bh * 0.7)}" width="${f1(bw * 0.25)}" height="${f1(bh * 0.25)}" fill="#ffcf73" opacity="0.85"/>`; } }
	s += K.skPlatform(w, h, { sign: 'HULLTOWN', lampX: 120, lampX2: 560, canopy: 200 });
	return s;
}

// ---------- boarding sketches (from the launch) ----------
function crewman(x, y, k = 1, flash = true) {
	let s = `<g transform="translate(${f1(x)} ${f1(y)}) scale(${k})"><circle cx="0" cy="-22" r="5" fill="#0d0809"/><path d="M-9,-26 h18 v-2 h-4 v-5 h-10 v5 h-4 Z" fill="#0d0809"/><path d="M-6,-17 h12 l2,20 h-16 Z" fill="#0d0809"/><path d="M4,-14 L20,-18" stroke="#0d0809" stroke-width="3"/>`;
	if (flash) s += `<circle cx="23" cy="-18" r="14" fill="url(#lampGlow)"/><path d="M23,-26 L25,-20 L32,-19 L25,-16 L23,-10 L21,-16 L14,-19 L21,-20 Z" fill="#fff1c2"/>`;
	return s + '</g>';
}
function boardStern() {
	const w = 1000, h = 360, hz = 230;
	let s = K.skSky(w, h, hz, 180);
	s += `<g transform="scale(1.49 0.955)">${''}</g>`;
	// the Lily's stern towering: a curved dark wall, gilt bands, windows, the name, the ladder down
	s += `<ellipse cx="560" cy="300" rx="420" ry="60" fill="#ffc35a" opacity="0.25" filter="url(#blur10)"/>`;
	s += `<path d="M290,44 L830,44 C826,150 770,270 660,330 L460,330 C350,270 294,150 290,44 Z" fill="#1a0c0e"/><path d="M290,44 C294,150 350,270 460,330 M830,44 C826,150 770,270 660,330" fill="none" stroke="${C.gold}" stroke-width="3"/>`;
	s += `<path d="M296,100 Q560,124 824,100" fill="none" stroke="${C.gold}" stroke-width="2.5"/><path d="M312,190 Q560,214 808,190" fill="none" stroke="${C.goldDeep}" stroke-width="2"/>`;
	const R = L.rng(91);
	for (const y0 of [70, 130, 210, 250]) { const half = 270 - Math.max(0, y0 - 120) * 0.7 - 22; const n = Math.round(half / 9); for (let k = 0; k <= n; k++) { const t = k / n, x = 560 - half + t * half * 2, y = y0 + (1 - Math.abs(t - 0.5) * 2) * 10; if (R() < 0.8) s += `<rect x="${f1(x)}" y="${f1(y)}" width="5" height="6" rx="1.5" fill="#ffd27a"/>`; } }
	s += text(560, 166, 'GILDED LILY', { size: 22, weight: 700, fill: C.gold, halo: false, ls: 6 });
	s += `<path d="M548,330 L548,250 L572,250 L572,330" fill="#0d0607"/>`;
	for (let y = 70; y < 330; y += 12) s += `<line x1="616" y1="${y}" x2="636" y2="${y}" stroke="#8a6440" stroke-width="2"/>`;
	s += `<line x1="616" y1="60" x2="616" y2="332" stroke="#8a6440" stroke-width="2"/><line x1="636" y1="60" x2="636" y2="332" stroke="#8a6440" stroke-width="2"/>`;
	s += `<path d="M286,44 h548" stroke="#2a1a14" stroke-width="7"/><path d="M286,40 h548" stroke="${C.gold}" stroke-width="1.5"/>`;
	for (const [x, k] of [[380, 1.1], [450, 1.2], [520, 1.15], [680, 1.2], [740, 1.1], [600, 1.15]]) s += crewman(x, 40, k);
	s += `<path d="M620,40 L300,300 L420,330 Z" fill="url(#beam)" opacity="0.35"/>`;
	for (const [x0, y0, x1, y1] of [[402, 22, 300, 300], [470, 20, 180, 330], [700, 22, 250, 320]]) s += `<line x1="${x0}" y1="${y0}" x2="${x1}" y2="${y1}" stroke="#ffe7a8" stroke-width="2" opacity="0.6" stroke-dasharray="20 30"/>`;
	s += `<path d="M0,${h} L0,300 Q120,280 230,300 L260,${h} Z" fill="#2a1410"/><path d="M0,300 Q120,280 230,300" stroke="${C.gold}" stroke-width="3" fill="none"/><circle cx="200" cy="292" r="26" fill="url(#lampGlow)"/><rect x="194" y="282" width="12" height="10" fill="#ffe7a8"/>`;
	s += `<path d="M230,330 q40,-14 80,0 q40,14 80,0" stroke="#fff" stroke-width="2" opacity="0.35" fill="none"/>`;
	return s;
}
function boardSide() {
	const w = 1000, h = 360, hz = 220;
	let s = K.skSky(w, h, hz, 900);
	s += `<path d="M-10,40 L${w + 10},150 L${w + 10},300 L-10,330 Z" fill="#1a0c0e"/><path d="M-10,40 L${w + 10},150" stroke="${C.gold}" stroke-width="3"/><path d="M-10,120 L${w + 10},200" stroke="${C.gold}" stroke-width="2"/>`;
	const R = L.rng(93);
	for (const [a, b] of [[150, 214], [180, 234], [250, 266], [280, 280]]) for (let k = 0; k < 44; k++) { const t = k / 43, x = t * w, y = a + (b - a) * t - (1 - t) * (a - 120) * 0.6; if (R() < 0.8) s += `<circle cx="${f1(x)}" cy="${f1(y)}" r="${f1(4 - t * 2)}" fill="#ffd27a"/>`; }
	for (let k = 0; k < 8; k++) { const t = k / 8, x = 40 + t * 900, y = 40 + t * 110; s += `<rect x="${f1(x)}" y="${f1(y - 34 + t * 10)}" width="${f1(80 - t * 40)}" height="${f1(18 - t * 8)}" rx="8" fill="#e8dcc0" stroke="#5a4a3a"/><line x1="${f1(x + 6)}" y1="${f1(y - 36 + t * 10)}" x2="${f1(x + 6)}" y2="${f1(y)}" stroke="#2a1a10" stroke-width="2"/>`; }
	s += `<path d="M430,96 L470,104 L520,300 L470,312 Z" fill="#6a4a2e" stroke="${C.gold}" stroke-width="2"/>`;
	for (let k = 0; k < 10; k++) { const t = k / 10; s += `<line x1="${f1(434 + t * 52)}" y1="${f1(100 + t * 205)}" x2="${f1(470 + t * 44)}" y2="${f1(106 + t * 204)}" stroke="#2a1a10" stroke-width="2"/>`; }
	for (const [x, y, k] of [[300, 72, 1.3], [380, 86, 1.25], [460, 98, 1.25], [540, 112, 1.2], [640, 128, 1.15], [470, 220, 1.3], [496, 270, 1.35]]) s += crewman(x, y, k);
	s += `<path d="M0,${h} L0,310 Q240,288 520,312 L560,${h} Z" fill="#2a1410"/><path d="M0,310 Q240,288 520,312" stroke="${C.gold}" stroke-width="3" fill="none"/><circle cx="470" cy="302" r="26" fill="url(#lampGlow)"/><rect x="464" y="292" width="12" height="10" fill="#ffe7a8"/>`;
	s += `<path d="M520,330 q40,-12 80,0 q40,12 80,0" stroke="#fff" stroke-width="2" opacity="0.35" fill="none"/>`;
	return s;
}

// ---------- the boarding row: launch sketch, zoomed first fight, notes ----------
function boardingRow(cn, o) {
	const y0 = K.SHIP.y, x0 = K.SHIP.x, W = K.SHIP.w, Hh = SHIFT - 24;
	let s = `<rect x="${x0}" y="${y0}" width="${W}" height="${Hh}" rx="6" fill="#071a28" stroke="${C.line}" stroke-width="1.5" stroke-opacity="0.7"/>`;
	s += text(x0 + 24, y0 + 42, "BOARDING · NED'S CREW KNOW YOU: HOSTILE FROM THE FIRST STEP", { size: 26, weight: 700, fill: '#f6e7c1', anchor: 'start', halo: false, ls: 2.5 });
	const sx = x0 + 20, sy = y0 + 60, sw = 1000, sh = 340;
	s += `<clipPath id="bdClip"><rect x="${sx}" y="${sy}" width="${sw}" height="${sh}" rx="6"/></clipPath><g clip-path="url(#bdClip)"><g transform="translate(${sx} ${sy}) scale(1 ${f1(sh / 360)})">${o.sketch}</g><rect x="${sx}" y="${sy}" width="${sw}" height="${sh}" filter="url(#paper)" opacity="0.08"/></g>`;
	s += `<rect x="${sx}" y="${sy}" width="${sw}" height="${sh}" rx="6" fill="none" stroke="${C.line}" stroke-width="2"/>` + chip(sx + 16, sy + sh - 22, 'THE LAUNCH RIDE OUT, AT DUSK', { size: 15, col: '#f6e7c1', anchor: 'start', stroke: C.accent, weight: 700 });
	// zoom onto the deck plan below: the first fight
	const zx = sx + sw + 18, zw = 560, zh = sh, zy = sy;
	const [fx, fy, kpm] = K.shipXY(cn, o.focus[0], o.focus[1], o.focus[2]), k = o.k;
	const Z = (t, v) => { const [x, y] = K.shipXY(cn, o.focus[0], t, v); return [zx + zw / 2 + k * (x - fx), zy + zh / 2 + k * (y - fy)]; };
	s += `<clipPath id="zClip"><rect x="${zx}" y="${zy}" width="${zw}" height="${zh}" rx="6"/></clipPath><g clip-path="url(#zClip)"><rect x="${zx}" y="${zy}" width="${zw}" height="${zh}" fill="#0b1a26"/><use href="#shipDecks" transform="translate(${f1(zx + zw / 2)} ${f1(zy + zh / 2)}) scale(${k}) translate(${f1(-fx)} ${f1(-fy)})"/>`;
	const [ex0, ey0] = Z(...o.entry.from), [ex1, ey1] = Z(...o.entry.to);
	s += `<line x1="${f1(ex0)}" y1="${f1(ey0)}" x2="${f1(ex1)}" y2="${f1(ey1)}" stroke="${C.accent}" stroke-width="7"/><path d="M${f1(ex1)},${f1(ey1)} m0,0" />`;
	const ang = Math.atan2(ey1 - ey0, ex1 - ex0);
	s += `<path d="M${f1(ex1 + Math.cos(ang) * 16)},${f1(ey1 + Math.sin(ang) * 16)} L${f1(ex1 + Math.cos(ang + 2.4) * 16)},${f1(ey1 + Math.sin(ang + 2.4) * 16)} L${f1(ex1 + Math.cos(ang - 2.4) * 16)},${f1(ey1 + Math.sin(ang - 2.4) * 16)} Z" fill="${C.accent}"/>`;
	for (const [t, v] of o.crew) { const [x, y] = Z(t, v); s += L.foeIcon(x, y, 'crew'); }
	s += '</g>' + `<rect x="${zx}" y="${zy}" width="${zw}" height="${zh}" rx="6" fill="none" stroke="${C.line}" stroke-width="2"/>` + chip(zx + 16, zy + zh - 22, 'FIRST FIGHT, ZOOMED (DECK PLAN BELOW)', { size: 15, col: '#f6e7c1', anchor: 'start', stroke: C.crew, weight: 700 });
	// notes
	const nx = zx + zw + 22; let ny = sy + 26;
	s += text(nx, ny, o.title, { size: 17, weight: 700, fill: C.accent, anchor: 'start', halo: false, ls: 1 }); ny += 34;
	for (const ln of o.lines) { s += text(nx, ny, ln, { size: 17, weight: 400, fill: C.text, anchor: 'start', halo: false }); ny += 26; }
	ny += 12; s += L.foeIcon(nx + 12, ny - 6, 'crew') + text(nx + 32, ny, "Ned's crew: sharps, bouncers,", { size: 15.5, weight: 400, fill: C.dim, anchor: 'start', halo: false }) + text(nx + 32, ny + 21, 'croupiers going Hollow', { size: 15.5, weight: 400, fill: C.dim, anchor: 'start', halo: false });
	return s;
}

// ======================================================================= render
async function render(items) {
	let pw; try { pw = require('playwright'); } catch { pw = require('/opt/node22/lib/node_modules/playwright'); }
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: K.W2, height: K.H2 + SHIFT } });
	for (const it of items) {
		await pg.setContent(`<!doctype html><html><body style="margin:0;background:#06121c">${it.svg}</body></html>`);
		await pg.evaluate(() => document.fonts.ready);
		await pg.screenshot({ path: it.png, clip: { x: 0, y: 0, width: K.W2, height: K.H2 + SHIFT } });
		console.log('png ->', it.png);
	}
	await browser.close();
}
const CONCEPTS = { H1: [conceptH1, 'H1_AlongTheCurve'], H2: [conceptH2, 'H2_TheSpine'] };
if (require.main === module) {
	const args = process.argv.slice(2), keys = args.filter(a => CONCEPTS[a]);
	const items = (keys.length ? keys : Object.keys(CONCEPTS)).map(k => {
		const [fn, name] = CONCEPTS[k], svg = K.page(fn());
		fs.writeFileSync(path.join(here, name + '.svg'), svg);
		return { svg, png: path.join(here, name + '.png') };
	});
	if (!args.includes('--no-png')) render(items).catch(e => { console.error(e); process.exit(1); });
}
