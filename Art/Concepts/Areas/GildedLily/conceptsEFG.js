// Concepts E (Breakwater Harbor, grounded), F (Hulltown, wild) and G (The Tiers, wild) for gen2.js.
const L = require('./lib');
const K = require('./lib2');
const { C, f1 } = L;
const SHORE_M2 = 95000, SHIP_M = 180;
const scale = isPlay => { const px = L.playArea(isPlay); return { px, mpp: Math.sqrt(SHORE_M2 / px) }; };
// A thick polyline as a closed polygon (breakwaters, moles, walkways).
function band(pts, w) { const sp = L.spline(pts, false, 6); return [...L.offsetLine(sp, w / 2), ...L.offsetLine(sp, -w / 2).reverse()]; }
function grid(x0, x1, y0, y1, bw, bh, street, ang, seed, skip, out) {
	const R = L.rng(seed); let s = '';
	for (let y = y0; y < y1; y += bh + street) for (let x = x0; x < x1; x += bw + street) {
		for (const [dx, dy] of [[0, 0], [bw / 2, 0], [0, bh / 2], [bw / 2, bh / 2]]) {
			const cx = x + dx + bw / 4, cy = y + dy + bh / 4;
			if (R() < 0.12 || (skip && skip([cx, cy]))) continue;
			s += L.roof(cx, cy, bw / 2 - 2, bh / 2 - 2, ang + (R() - 0.5) * 4, C.roofs[Math.floor(R() * C.roofs.length)]);
			if (out) out.push([cx, cy]);
		}
	}
	return s;
}

// ======================================================================= E: Breakwater Harbor (grounded)
function conceptE() {
	const main = L.roughen([[-80, 1100], [-80, 640], [20, 650], [70, 616], [130, 596], [190, 604], [226, 640], [262, 700], [340, 742], [460, 762], [600, 772], [740, 762], [860, 732], [930, 692], [990, 664], [1060, 646], [1150, 650], [1240, 630], [1380, 612], [1380, 1100]], true, 4, 301, 10);
	const brk = band([[226, 650], [232, 520], [300, 404], [420, 324], [560, 284], [700, 282], [800, 312]], 28);
	const mole = band([[930, 700], [942, 580], [912, 474]], 24);
	const quays = [340, 440, 540, 640, 740].map((x, i) => [[x - 15, 760 - i * 2], [x + 15, 760 - i * 2], [x + 15, 640], [x - 15, 640]]);
	const gilded = band([[830, 740], [842, 640], [852, 560]], 18);
	const hills = L.roughen([[1380, 1100], [1380, 820], [1240, 830], [1120, 860], [1040, 920], [980, 980], [960, 1100]], true, 6, 302, 8);
	const south = L.roughen([[-80, 1100], [-80, 990], [300, 984], [600, 990], [980, 990], [980, 1100]], true, 4, 303, 8);
	const ridges = [hills, south];
	const lands = [main, brk, mole, gilded, ...quays];
	const isLand = p => L.inAny(p, lands), isPlay = p => isLand(p) && !L.inAny(p, ridges), isWater = p => !isLand(p);
	const { px, mpp } = scale(isPlay), shipPx = SHIP_M / mpp;
	console.log(`E: play ${px}, mpp ${mpp.toFixed(3)}, ship ${shipPx.toFixed(0)} px`);
	const coastD = lands.map(l => L.d(l, true)).join(' ');
	let m = L.waterLayer([L.d(main, true), L.d(brk, true), L.d(mole, true)]) + L.waves(isWater, 700, 11);
	m += L.flats([[1000, 560], [1120, 540], [1260, 520], [1340, 560], [1340, 620], [1200, 640], [1060, 640], [990, 620]], 304);
	m += L.landLayer('landE', coastD, [L.d(L.roughen([[-60, 700], [180, 660], [200, 900], [-60, 980]], true, 8, 305, 6), true), L.d(L.roughen([[260, 800], [1000, 790], [1000, 990], [260, 990]], true, 6, 306, 6), true)]);
	// stone: breakwater, mole and quays paved
	for (const b of [brk, mole, gilded, ...quays]) m += `<path d="${L.d(b, true)}" fill="${b === gilded ? '#8a6440' : '#8c8578'}" stroke="#2a2622" stroke-width="2"/>`;
	m += `<path d="${L.smooth([[226, 650], [232, 520], [300, 404], [420, 324], [560, 284], [700, 282], [800, 312]], false, 6)}" fill="none" stroke="#b5ad9e" stroke-width="6" stroke-dasharray="14 8" opacity="0.7"/>`;
	m += `<path d="${L.d(gilded, true)}" fill="none" stroke="${C.gold}" stroke-width="2"/>`;
	m += L.hill([[-60, 640], [100, 610], [190, 640], [200, 760], [120, 860], [-60, 880]], 307, 3);
	m += L.ridgeLayer(ridges.map(r => L.d(r, true)), [], 'landE');
	m += L.trees(p => L.inAny(p, ridges) && isLand(p), [0, 800, 1300, 1020], 120, 308, { col: '#2b3720', r: 6 });
	m += L.cliff([[20, 650], [70, 616], [130, 596], [190, 604], [226, 640]], -1, 309, { len: 14 });
	m += L.cliff([[20, 870], [120, 860], [200, 780], [230, 700]], 1, 310, { len: 9, w: 3 });
	// terraced town: three streets of blocks stepping up from the quays
	const bld = [];
	m += grid(270, 990, 806, 980, 44, 34, 10, 0, 311, p => !isPlay(p) || (p[0] > 600 && p[0] < 700 && p[1] > 840 && p[1] < 900), bld);
	m += L.cliff([[260, 852], [1000, 848]], 1, 312, { len: 6, w: 2.5, step: 14 }) + L.cliff([[260, 912], [1000, 908]], 1, 313, { len: 6, w: 2.5, step: 14 });
	m += L.roof(650, 870, 60, 40, 0, C.velvet, { big: true }) + L.roof(820, 880, 46, 32, 0, '#3a1418', { big: true }) + L.roof(480, 940, 34, 22, 0, '#e8e0cc', { big: true }) + L.roof(560, 880, 34, 26, 0, '#6a6a70', { big: true, trim: '#cfd6dd' });
	for (let x = 290; x < 980; x += 66) m += L.warehouse(x, 784, 56, 18, 0);
	m += L.road([[260, 802], [1000, 798]], 8) + L.road([[1000, 798], [1100, 760], [1200, 700]], 7);
	for (const [x, y] of [[320, 880], [700, 880], [900, 930]]) m += L.lamp(x, y, 1);
	const railPts = [[1380, 770], [1250, 790], [1120, 806], [1010, 812]], spur = [[1010, 812], [930, 760], [900, 730]];
	m += L.rail(railPts) + L.rail(spur) + L.roof(1030, 822, 66, 18, -6, '#4a3a30');
	m += L.dryDock(300, 722, 96, -30);
	for (const [x, y, r] of [[880, 730, 12], [904, 716, 10], [866, 712, 9]]) m += L.coalHeap(x, y, r);
	for (const x of [340, 440, 540, 640, 740]) m += L.crane(x, 650, -90) + L.lamp(x, 700, 0.8);
	for (const [x, y, a, l, k] of [[390, 690, 90, 40, 0], [490, 700, 90, 34, 2], [590, 690, 90, 34, 2], [690, 700, 90, 40, 0], [600, 500, 30, 30, 1], [460, 560, -20, 26, 1], [990, 600, 10, 24, 1]]) m += L.boat(x, y, a, l, k);
	m += L.wreck(60, 560, 40, -20) + L.wreck(1160, 700, 40, 20) + L.wreck(1280, 680, 30, -40);
	m += L.cave(40, 640, 180) + L.cave(150, 600, 190) + L.cave(1300, 640, 160);
	m += L.rocks(isWater, [0, 520, 200, 620], 16, 314, { r: 3.5 });
	m += L.trees(p => isPlay(p) && p[0] < 210 && p[1] > 680, [0, 640, 230, 980], 30, 315, { r: 6 });
	m += L.lamp(800, 312, 1.8) + `<circle cx="800" cy="312" r="7" fill="#f2ecdf" stroke="#a0362b" stroke-width="3"/>`;
	m += L.lighthouse(96, 644, 205, 520);
	m += L.liner(1080, 300, shipPx, -12, 31);
	m += L.seaRoute([[852, 556], [860, 440], [920, 380], [1000, 340], [1040, 318]]) + L.launchBoat(880, 420, -60);
	m += K.fight(450, 316, 30, 'crew', 'F1', mpp) + K.fight(880, 740, 50, 'crew', 'F2', mpp) + K.fight(120, 640, 34, 'beast', 'F3', mpp, { below: true }) + K.fight(1170, 590, 56, 'beast', 'F4', mpp) + K.fight(300, 722, 40, 'bounty', 'F5', mpp, { below: true });
	m += L.route([[1010, 812], [900, 830], [720, 830], [660, 870]]) + L.route([[660, 870], [760, 880], [820, 880]]);
	m += L.route([[820, 880], [840, 800], [842, 700], [850, 600]]);
	m += L.route([[560, 830], [300, 830], [230, 700], [232, 520], [420, 330], [700, 290]], { w: 3, dash: '5 7' }) + L.route([[1000, 800], [1150, 700], [1220, 660]], { w: 3, dash: '5 7' });
	m += L.step(1050, 852, 1) + L.step(630, 920, 2) + L.step(820, 930, 3) + L.step(878, 600, 4) + L.step(1010, 352, 5) + L.step(944, 270, 6);
	m += L.giver(690, 830) + L.giver(480, 912) + L.giver(820, 846);
	m += L.chest(60, 590) + L.chest(780, 290) + L.chest(1260, 660) + L.chest(300, 760);
	m += L.secret(40, 670) + L.secret(960, 960);
	m += K.bigLabel(640, 1010 - 30, 'The Port', null, { size: 30 }).replace(/y="980"/, 'y="975"');
	m += K.bigLabel(540, 598, 'Docks & Shipyards', null, { size: 22 }) + K.bigLabel(560, 470, 'The Basin', null, { size: 26, fill: '#bfeaff' });
	m += K.bigLabel(500, 238, 'The Breakwater', null, { size: 22 }) + K.bigLabel(130, 520, 'Sea Cliffs', null, { size: 22 }) + K.bigLabel(1170, 490, 'Beaches & Flats', null, { size: 22 });
	m += K.bigLabel(1080, 210, 'The Lily', null, { size: 22, fill: C.lamp });
	return {
		key: 'E', name: 'Breakwater Harbor', kind: 'GROUNDED', tag: 'CLOSE QUARTERS', area: Math.round(px * mpp * mpp), mpp, map: m, scaleAt: [40, 120], compassAt: [1250, 60],
		subtitle: 'A stone harbor: a long breakwater round a basin of quays, a terraced town climbing behind, cliffs and the light on the west headland, flats to the east.',
		plays: ['Dense: quays, warehouses and terraced streets make short', 'sightlines, corners and rooftops. The breakwater is the one long', 'exposed walk, out to its beacon. The Lily waits past the gap.'],
		path: ['Train into the depot at the town\'s east end', 'Port missions up and down the terraces', 'Win the marker: the Gilded Parlor, upper street', 'The launch from the gilded quay', 'Board through the coaling port', 'The ballroom: Ned deals'],
		fights: [['crew', 'F1 the breakwater', 'a gauntlet with no cover'], ['crew', 'F2 coal yard', 'bouncers in the stacks'], ['beast', 'F3 gull cliffs', 'gulls on the headland'], ['beast', 'F4 the flats', 'crabs at low tide, eels in pools'], ['bounty', 'F5 the dry dock', 'a bounty in the empty pit']],
		deck: {
			how: 'the launch slips in with the coal lighters, low on the starboard side',
			route: [{ k: 'a', deck: 2, t: -0.05, v: 0.5, say: 'coaling port' }, { k: 'b', deck: 2, t: 0.375, v: 0, pre: [[-0.05, 0]], say: 'through the boilers to the vault' }, { k: 'c', deck: 0, t: 0.288, v: 0, via: 0.37, say: 'cargo hatch up: the bridge' }, { k: 'd', deck: 1, t: 0.08, v: 0, pre: [[0.24, 0.37], [-0.16, 0.37]], via: -0.16, post: [[-0.12, 0]], say: 'grand staircase: casino brawl' }, { k: 'e', deck: 1, t: -0.335, v: 0.12, pre: [[-0.16, 0]], say: 'the ballroom: Ned' }],
			fights: [{ deck: 2, t: 0.1, v: 0, d: 20, tag: 'boiler rooms' }, { deck: 0, t: 0.288, v: 0, d: 14, tag: 'bridge', below: true }, { deck: 1, t: 0.08, v: 0, d: 22, tag: 'casino brawl', below: true }, { deck: 1, t: -0.335, v: 0, d: 22, tag: 'NED', below: true }],
		},
		sketch: sketchE(),
	};
}
function sketchE() {
	const { w, h } = K.SK, hz = 150;
	let s = K.skSky(w, h, hz, 470);
	s += K.skLiner(470, hz + 4, 80);
	s += K.skShape([[-10, 120], [60, 110], [140, 140], [170, 175], [-10, 200]], '#241a2e', '#e2774a', 0.6);
	s += `<rect x="40" y="96" width="9" height="22" fill="#e8dcc0"/><circle cx="44" cy="96" r="22" fill="url(#lampGlow)"/><path d="M44,96 L260,70 L260,110 Z" fill="url(#beam)" opacity="0.6"/>`;
	s += `<path d="M150,192 Q330,160 430,172" fill="none" stroke="#2a2030" stroke-width="9"/><path d="M150,188 Q330,156 430,168" fill="none" stroke="#e2774a" stroke-width="1.2" opacity="0.5"/><circle cx="430" cy="166" r="16" fill="url(#lampGlow)"/><rect x="427" y="156" width="6" height="11" fill="#f2ecdf"/>`;
	for (const [x, ht] of [[220, 60], [300, 70], [380, 56], [520, 64]]) s += `<path d="M${x},230 v-${ht} h44 M${x},${230 - ht} l-14,8" fill="none" stroke="#1d1520" stroke-width="4"/>`;
	s += `<rect y="226" width="${w}" height="20" fill="#1a1420"/>`;
	s += K.skTown(0, w, 270, 18, 46, 41, '#1b121a', { lit: 0.5 }) + K.skTown(0, w, 300, 18, 50, 42, '#150e15', { lit: 0.45 });
	s += K.skPlatform(w, h, { sign: 'GILT HARBOR', lampX: 330, lampX2: 520 });
	return s;
}

// ======================================================================= F: Hulltown (wild)
function wreckGeom(cx, cy, len, ang) {
	const a = ang * Math.PI / 180, ca = Math.cos(a), sa = Math.sin(a), wd = len * 0.15;
	const P = (t, v) => [cx + ca * t * len - sa * v * wd, cy + sa * t * len + ca * v * wd];
	const inT = (p) => { const dx = p[0] - cx, dy = p[1] - cy; return [(dx * ca + dy * sa) / len, (-dx * sa + dy * ca) / wd]; };
	return { P, inT, wd };
}
function conceptF() {
	const main = L.roughen([[1380, 140], [1250, 210], [1140, 300], [1060, 400], [990, 500], [930, 590], [850, 680], [760, 760], [640, 830], [520, 880], [400, 920], [280, 980], [180, 1100], [1380, 1100]], true, 5, 401, 10);
	const islet = L.roughen([[240, 250], [310, 200], [380, 214], [410, 270], [380, 340], [300, 360], [240, 320]], true, 6, 402, 8);
	const W0 = wreckGeom(590, 520, 640, 38); // bow at the islet (north-west), stern on the beach (south-east)
	const secs = [[-0.5, -0.17], [-0.13, 0.15], [0.19, 0.5]]; // bow, midsection, stern (t from bow end -0.5)
	const inWreck = p => { const [t, v] = W0.inT(p); return Math.abs(v) < 0.5 * (t < -0.4 ? Math.max(0.1, (t + 0.5) * 5) : 1) && secs.some(([a, b]) => t >= a && t <= b); };
	const hills = L.roughen([[1380, 1100], [1380, 300], [1300, 360], [1240, 480], [1220, 600], [1160, 720], [1100, 820], [1000, 900], [900, 960], [820, 1100]], true, 7, 403, 8);
	const isLandG = p => L.pip(p, main) || L.pip(p, islet);
	const isPlay = p => (isLandG(p) && !L.pip(p, hills)) || inWreck(p);
	const isWater = p => !isLandG(p) && !inWreck(p);
	const { px, mpp } = scale(isPlay), shipPx = SHIP_M / mpp;
	console.log(`F: play ${px}, mpp ${mpp.toFixed(3)}, ship ${shipPx.toFixed(0)} px, wreck ${(640 * mpp).toFixed(0)} m`);
	const coastD = [main, islet].map(l => L.d(l, true)).join(' ');
	const hull = [];
	for (let k = 0; k <= 40; k++) { const t = -0.5 + k / 40; const v = t < -0.38 ? 0.5 * Math.sin(((t + 0.5) / 0.12) * Math.PI / 2) : 0.5; hull.push(W0.P(t, -v)); }
	for (let k = 40; k >= 0; k--) { const t = -0.5 + k / 40; const v = t < -0.38 ? 0.5 * Math.sin(((t + 0.5) / 0.12) * Math.PI / 2) : 0.5; hull.push(W0.P(t, v)); }
	let m = L.waterLayer([coastD, L.d(hull, true)]) + L.waves(isWater, 700, 13);
	m += L.flats([[420, 820], [560, 790], [700, 740], [800, 700], [760, 780], [640, 860], [500, 900], [400, 930], [330, 960], [300, 900]], 404);
	m += L.flats([[180, 380], [240, 360], [300, 380], [260, 430], [190, 430]], 405);
	m += L.landLayer('landF', coastD, [L.d(L.roughen([[1000, 560], [1300, 300], [1300, 1000], [700, 1000], [880, 760]], true, 8, 406, 6), true)]);
	m += L.hill([[260, 240], [320, 214], [380, 232], [392, 290], [350, 330], [280, 330]], 407, 3);
	m += L.ridgeLayer([L.d(hills, true)], [], 'landF');
	m += L.trees(p => L.pip(p, hills) && L.pip(p, main), [700, 250, 1300, 1020], 140, 408, { col: '#2b3720', r: 6 });
	m += L.cliff([[240, 250], [310, 200], [380, 214], [410, 270]], -1, 409, { len: 13 }) + L.cliff([[240, 320], [240, 250]], -1, 410, { len: 12 });
	// the wreck: a rusted giant broken in three; plating, holes, ribs at the breaks, shacks on its decks
	const R = L.rng(411);
	const sectPoly = ([a, b]) => { const pts = []; for (let k = 0; k <= 20; k++) { const t = a + (b - a) * k / 20; const v = t < -0.38 ? 0.5 * Math.max(0.08, Math.sin(((t + 0.5) / 0.12) * Math.PI / 2)) : 0.5; pts.push(W0.P(t, -v)); } for (let k = 20; k >= 0; k--) { const t = a + (b - a) * k / 20; const v = t < -0.38 ? 0.5 * Math.max(0.08, Math.sin(((t + 0.5) / 0.12) * Math.PI / 2)) : 0.5; pts.push(W0.P(t, v)); } return L.roughen(pts, true, 2.5, Math.round(a * 100 + 500), 4); };
	const polys = secs.map(sectPoly);
	for (const p of polys) m += `<path d="${L.d(p, true)}" fill="#000" opacity="0.45" transform="translate(8 10)"/><path d="${L.d(p, true)}" fill="#5e3424" stroke="#1d0f08" stroke-width="3"/>`;
	m += `<clipPath id="wreckClip"><path d="${polys.map(p => L.d(p, true)).join(' ')}"/></clipPath><g clip-path="url(#wreckClip)">`;
	for (let t = -0.5; t < 0.5; t += 0.02) m += `<path d="${L.d([W0.P(t, -0.5), W0.P(t, 0.5)])}" stroke="#3e2216" stroke-width="1.5" opacity="0.7"/>`;
	for (const v of [-0.3, 0, 0.3]) m += `<path d="${L.d([W0.P(-0.5, v), W0.P(0.5, v)])}" stroke="#7a4a30" stroke-width="2" opacity="0.6"/>`;
	for (let k = 0; k < 90; k++) { const [x, y] = W0.P(-0.5 + R(), (R() - 0.5) * 0.9); m += `<ellipse cx="${f1(x)}" cy="${f1(y)}" rx="${f1(4 + R() * 14)}" ry="${f1(3 + R() * 8)}" fill="${R() < 0.5 ? '#8a4a2a' : '#2a160e'}" opacity="0.6"/>`; }
	m += `<rect x="-50" y="-50" width="1400" height="1100" filter="url(#mottleDark)" opacity="0.5"/></g>`;
	for (const [a, b] of [[-0.17, -0.13], [0.15, 0.19]]) for (let k = 0; k < 7; k++) { const v = -0.45 + k * 0.15; m += `<path d="${L.d([W0.P(a, v), W0.P(b, v + 0.02)])}" stroke="#7a4a30" stroke-width="3" opacity="0.9"/>`; }
	for (const [t, v] of [[-0.02, -0.1], [0.06, 0.08], [0.32, 0]]) { const [x, y] = W0.P(t, v); m += `<ellipse cx="${f1(x)}" cy="${f1(y)}" rx="22" ry="14" fill="#140a06" stroke="#7a4a30" stroke-width="2" transform="rotate(38 ${f1(x)} ${f1(y)})"/>`; }
	const bld = [];
	for (let k = 0; k < 70; k++) { const t = [0.21, 0.48][0] + R() * 0.27, v = (R() - 0.5) * 0.8; if (Math.abs(t - 0.32) < 0.03 && Math.abs(v) < 0.15) continue; const [x, y] = W0.P(t, v); m += L.roof(x, y, 13 + R() * 8, 10 + R() * 6, 38 + (R() - 0.5) * 30, C.roofs[Math.floor(R() * C.roofs.length)]); bld.push([x, y]); }
	for (let k = 0; k < 18; k++) { const t = -0.12 + R() * 0.26, v = (R() - 0.5) * 0.8; const [x, y] = W0.P(t, v); m += L.roof(x, y, 12, 10, 38, '#5c4a3e'); }
	for (let k = 0; k < 40; k++) { const [x, y] = W0.P(0.2 + R() * 0.29, (R() - 0.5) * 0.8); m += L.lamp(x, y, 0.6); }
	m += L.warehouse(...W0.P(0.0, -0.2), 70, 22, 38);
	// gangplank walkways between the sections and to the beach
	m += L.bridge(W0.P(-0.175, 0), W0.P(-0.125, 0), 12) + L.bridge(W0.P(0.145, 0.1), W0.P(0.195, 0.1), 12);
	m += L.bridge(W0.P(-0.47, 0), [330, 300], 10);
	// the shore: shacks along the coast road, docks off the midsection's lee side, coal, the station in the stern
	const coastRd = [[1300, 230], [1180, 320], [1090, 420], [1020, 520], [960, 610], [900, 690], [860, 760], [760, 830], [620, 890]];
	m += L.road(coastRd, 9);
	const railPts = [[1380, 180], [1260, 250], [1150, 350], [1060, 470], [980, 590], [920, 680], [880, 730]];
	m += L.rail(railPts);
	const t1 = L.town(coastRd.slice(2, 7), { step: 24, off: 14, w: 17, h: 14, sides: [1], gaps: 0.25, skip: p => !isPlay(p) }, 412, bld);
	m += t1.s + t1.lamps;
	for (const t of [-0.08, 0.0, 0.08]) { const a = W0.P(t, -0.5), b = W0.P(t, -0.85); m += L.pier(a, b, 11, { head: 14 }); }
	for (const [t, v] of [[-0.05, -0.3], [0.1, 0.3], [-0.1, 0.35]]) { const [x, y] = W0.P(t, v); m += L.crane(x, y, 128); }
	for (const [x, y, r] of [[1010, 470, 12], [1036, 460, 10], [990, 452, 9]]) m += L.coalHeap(x, y, r);
	m += L.roof(...W0.P(0.33, 0.05), 54, 20, 38, '#4a3a30') + L.roof(...W0.P(0.42, -0.2), 40, 26, 38, C.velvet, { big: true }) + L.roof(...W0.P(0.26, 0.22), 40, 26, 38, '#3a1418', { big: true }) + L.roof(...W0.P(0.45, 0.25), 30, 18, 38, '#e8e0cc', { big: true });
	m += L.wreck(420, 880, 40, 20) + L.wreck(540, 850, 34, -30) + L.wreck(330, 930, 30, 60) + L.wreck(200, 400, 30, 40);
	m += L.cave(250, 300, 210) + L.cave(300, 210, 250) + L.cave(300, 960, 120);
	m += L.trees(p => isPlay(p) && !inWreck(p) && L.lineDist(p, coastRd) > 14 && bld.every(q => L.dist(p, q) > 16), [500, 300, 1300, 1000], 60, 413, { r: 6 });
	m += L.rocks(isWater, [160, 160, 440, 440], 22, 414, { r: 3.5 });
	const bow = W0.P(-0.46, 0);
	m += L.lighthouse(bow[0], bow[1], 200, 560);
	for (const [x, y, a, l, k] of [[760, 520, 40, 30, 2], [790, 500, 40, 30, 2], [880, 560, 30, 24, 1], [600, 380, 20, 26, 1]]) m += L.boat(x, y, a, l, k);
	m += L.liner(780, 170, shipPx, 8, 41);
	m += L.seaRoute([[...W0.P(-0.08, -0.85)], [700, 380], [640, 300], [622, 222]]) + L.launchBoat(780, 380, -80);
	const mid = W0.P(0.0, 0), stern = W0.P(0.4, 0);
	m += K.fight(mid[0], mid[1], 44, 'crew', 'F1', mpp) + K.fight(330, 280, 36, 'beast', 'F2', mpp, { below: true }) + K.fight(560, 830, 56, 'beast', 'F3', mpp, { below: true }) + K.fight(300, 940, 34, 'crew', 'F4', mpp) + K.fight(stern[0] - 30, stern[1] + 50, 28, 'bounty', 'F5', mpp, { below: true });
	const st = W0.P(0.33, 0.05);
	m += L.route([[880, 730], [st[0] + 30, st[1] + 20], [st[0], st[1]]]) + L.route([[st[0], st[1]], ...[W0.P(0.4, -0.2)]]);
	m += L.route([W0.P(0.4, -0.2), W0.P(0.26, 0.22)]);
	m += L.route([W0.P(0.26, 0.22), W0.P(0.17, 0.1), W0.P(0.0, -0.3), W0.P(-0.08, -0.85)]);
	m += L.route([W0.P(-0.02, 0), W0.P(-0.3, 0), W0.P(-0.47, 0), [330, 300]], { w: 3, dash: '5 7' }) + L.route([W0.P(0.4, 0.4), [600, 870], [400, 900]], { w: 3, dash: '5 7' });
	m += L.step(906, 760, 1) + L.step(...W0.P(0.42, -0.48), 2) + L.step(...W0.P(0.22, 0.5), 3) + L.step(...W0.P(-0.08, -1.2), 4) + L.step(586, 214, 5) + L.step(664, 140, 6);
	m += L.giver(...W0.P(0.42, -0.2)) + L.giver(...W0.P(0.26, 0.22)) + L.giver(...W0.P(0.45, 0.25));
	m += L.chest(...W0.P(-0.3, 0.2)) + L.chest(290, 210) + L.chest(450, 900) + L.chest(...W0.P(0.06, -0.3));
	m += L.secret(...W0.P(-0.25, -0.3)) + L.secret(200, 430);
	m += K.bigLabel(1070, 610, 'Hulltown', 'the port, in the wreck\'s stern', { size: 30 });
	m += K.bigLabel(...W0.P(-0.2, 1.25), 'The Sovereign', `the old wreck, ~${Math.round(640 * mpp / 10) * 10} m`, { size: 26, rot: 38 });
	m += K.bigLabel(310, 140, 'The Anvil', 'sea cliffs, the light on the bow', { size: 22 }) + K.bigLabel(420, 990, 'Beaches & Flats', null, { size: 22 });
	m += K.bigLabel(990, 400, 'Salvage Yards', null, { size: 20 }) + K.bigLabel(780, 80, 'The Lily', null, { size: 22, fill: C.lamp });
	return {
		key: 'F', name: 'Hulltown', kind: 'WILD', tag: 'IN AND OUT', area: Math.round(px * mpp * mpp), mpp, map: m, scaleAt: [40, 980], compassAt: [1250, 60],
		subtitle: 'A port built in a colossal old wreck, beached and broken in three: the town in its stern, salvage yards round its middle, its bow on a rock with the light.',
		plays: ['In and out: the wreck is a three-part dungeon of decks, holds', 'and ladders between open beaches. Tight corridor fights inside,', 'long open sand outside; gangplanks join the broken sections.'],
		path: ['The train runs into the wreck\'s stern', 'Port missions in Hulltown and the yards', 'Win the marker: the Gilded Parlor, stern deck', 'The launch from the midsection\'s piers', 'Board at the stern ladder', 'The ballroom: Ned deals'],
		fights: [['crew', 'F1 the midsection', "Ned's crew hold its holds"], ['beast', 'F2 the Anvil', 'gulls nest on the bow'], ['beast', 'F3 the flats', 'crabs, eels in the pools'], ['crew', 'F4 smugglers\' cove', 'sharps and grave gold'], ['bounty', 'F5 the old engine room', 'a bounty under the stern']],
		deck: {
			how: 'the launch ties up under the stern; up the ladder onto the aft deck',
			route: [{ k: 'a', deck: 0, t: -0.48, v: 0.05, say: 'stern ladder, aft deck' }, { k: 'b', deck: 1, t: -0.2, v: 0.3, via: -0.16, pre: [[-0.3, 0.38], [-0.16, 0.38]], say: 'grand staircase down: locked ballroom' }, { k: 'c', deck: 1, t: 0.12, v: 0, say: 'the casino floor' }, { k: 'd', deck: 2, t: 0.375, v: 0, via: 0.05, post: [[0.3, 0]], say: 'service stairs: the vault key' }, { k: 'e', deck: 1, t: -0.335, v: 0.12, via: 0.05, post: [[-0.16, 0]], say: 'back up: the ballroom' }],
			fights: [{ deck: 0, t: -0.42, v: 0, d: 16, tag: 'aft deck' }, { deck: 2, t: 0.375, v: 0, d: 16, tag: 'vault guard', below: true }, { deck: 1, t: -0.335, v: 0, d: 22, tag: 'NED', below: true }],
		},
		sketch: sketchF(),
	};
}
function sketchF() {
	const { w, h } = K.SK, hz = 210;
	let s = K.skSky(w, h, hz, 250);
	s += K.skLiner(300, hz + 3, 66);
	s += K.skShape([[80, 210], [110, 160], [170, 150], [200, 205]], '#221824', '#e2774a', 0.5);
	s += `<path d="M150,154 L150,90 M150,100 L130,120 M150,100 L170,118" stroke="#221824" stroke-width="4"/><circle cx="150" cy="90" r="20" fill="url(#lampGlow)"/><path d="M150,90 L0,70 L0,110 Z" fill="url(#beam)" opacity="0.5"/>`;
	s += `<path d="M360,${h} L380,40 L700,10 L700,${h} Z" fill="#2c1a18"/>`;
	for (let k = 0; k < 9; k++) s += `<path d="M${390 + k * 34},${44 - k * 3} L${388 + k * 34},${h}" stroke="#4a2a20" stroke-width="5"/>`;
	s += `<path d="M380,40 L700,10" stroke="#e2774a" stroke-width="2" opacity="0.6"/><rect x="430" y="70" width="80" height="60" fill="#150c0c"/><path d="M380,150 L700,140" stroke="#5a3422" stroke-width="8"/>`;
	s += K.skTown(392, w, 250, 14, 40, 51, '#1a1012', { lit: 0.55 }) + K.skTown(392, w, 150, 10, 22, 52, '#1a1012', { lit: 0.5 });
	s += K.skTown(0, 360, 262, 10, 26, 53, '#170f14', { lit: 0.4 });
	s += K.skPlatform(w, h, { sign: 'HULLTOWN', lampX: 250, lampX2: 330, canopy: 180 });
	return s;
}

// ======================================================================= G: The Tiers (wild)
function conceptG() {
	const arc = (Y, x) => Y - 130 * (1 - ((x - 650) / 620) ** 2);
	const line = (Y, x0 = 140, x1 = 1160) => { const pts = []; for (let x = x0; x <= x1; x += 40) pts.push([x, arc(Y, x)]); return pts; };
	const TY = [370, 460, 550, 640, 700]; // crest, tier lines, quay top, waterline
	const main = L.roughen([[-80, -80], [1380, -80], [1380, 1100], [1300, 1100], [1300, 900], [1250, 780], [1180, 720], ...line(TY[4], 160, 1140).reverse(), [120, 700], [60, 760], [-80, 780]], true, 4, 501, 8);
	const plateau = L.roughen([[-80, -80], [1380, -80], [1380, 340], ...line(TY[0] - 4, 80, 1220).reverse(), [-80, 320]], true, 4, 502, 6);
	const westOB = L.roughen([[-80, 300], [80, 300], [60, 500], [20, 600], [-80, 640]], true, 4, 503, 6);
	const ridges = [plateau, westOB];
	const isLand = p => L.pip(p, main), isPlay = p => isLand(p) && !L.inAny(p, ridges), isWater = p => !isLand(p);
	const { px, mpp } = scale(isPlay), shipPx = SHIP_M / mpp;
	console.log(`G: play ${px}, mpp ${mpp.toFixed(3)}, ship ${shipPx.toFixed(0)} px`);
	const coastD = L.d(main, true);
	let m = L.waterLayer([coastD]) + L.waves(isWater, 700, 15);
	m += L.flats([[1140, 720], [1240, 760], [1300, 860], [1300, 1000], [1200, 980], [1120, 860], [1080, 760]], 504);
	m += L.landLayer('landG', coastD, [L.d(L.roughen([[1180, 720], [1380, 700], [1380, 1100], [1300, 1100], [1250, 800]], true, 6, 505, 6), true)]);
	// tiers: each band a step lighter toward the top; stone wall faces between them
	const tierCol = ['#a89880', '#9a8a70', '#8c7c62', '#7e6e56'];
	for (let k = 0; k < 4; k++) {
		const top = line(TY[k], 100, 1200), bot = line(TY[k + 1], 100, 1200).reverse();
		m += `<path d="${L.d([...top, ...bot], true)}" fill="${tierCol[k]}" opacity="0.85"/>`;
	}
	m += `<g clip-path="url(#landG)"><rect x="-50" y="-50" width="1400" height="1100" filter="url(#mottleDark)" opacity="0.45"/></g>`;
	m += L.hill([[-60, 560], [80, 520], [150, 600], [140, 700], [60, 760], [-60, 760]], 506, 3);
	m += L.ridgeLayer(ridges.map(r => L.d(r, true)), [], 'landG');
	m += L.trees(p => L.pip(p, plateau), [0, 0, 1300, 330], 120, 507, { col: '#2b3720', r: 6 });
	for (let k = 0; k < 4; k++) m += L.cliff(line(TY[k], 100, 1200), 1, 510 + k, { len: k === 3 ? 18 : 12, w: k === 3 ? 6 : 4, step: 10 });
	m += L.cliff([[120, 700], [60, 760], [-20, 778]], 1, 520, { len: 14 });
	// streets along each tier, houses packed between
	const bld = [];
	for (let k = 0; k < 3; k++) {
		const st = line((TY[k] + TY[k + 1]) / 2 + 10, 160, 1140);
		m += L.road(st, 7);
		const t = L.town(st, { step: 22, off: 11, w: 17, h: 13, gaps: 0.15, skip: p => [300, 650, 980, 470, 820].some(x => Math.abs(p[0] - x) < 16) }, 530 + k, bld);
		m += t.s + t.lamps;
	}
	m += L.road(line(TY[3] + 30, 160, 1140), 9, { col: '#b5ad9e' });
	// lifts (cyan) and stairs (zigzag) down the faces
	for (const x of [300, 650, 980]) { m += `<line x1="${x}" y1="${f1(arc(TY[0], x) - 10)}" x2="${x}" y2="${f1(arc(TY[4], x))}" stroke="#0b2232" stroke-width="10"/><line x1="${x}" y1="${f1(arc(TY[0], x) - 10)}" x2="${x}" y2="${f1(arc(TY[4], x))}" stroke="${C.line}" stroke-width="3"/>`; for (let k = 0; k < 4; k++) m += `<rect x="${x - 7}" y="${f1(arc(TY[k] + 30, x))}" width="14" height="12" fill="#ffd27a" stroke="#0b2232" stroke-width="2"/>`; }
	for (const x of [470, 820]) { let pts = []; for (let y = arc(TY[0], x); y < arc(TY[3], x); y += 14) pts.push([x - 8, y], [x + 8, y + 7]); m += `<path d="${L.d(pts)}" fill="none" stroke="#f6e7c1" stroke-width="2" opacity="0.8"/>`; }
	// coal chutes from the rail on the crest down to the bunkers on the quay
	for (const x of [880, 920]) m += `<line x1="${x}" y1="${f1(arc(TY[0], x))}" x2="${x - 10}" y2="${f1(arc(TY[3], x) + 26)}" stroke="#111" stroke-width="6"/>`;
	for (const [x, r] of [[880, 12], [906, 10], [930, 11]]) m += L.coalHeap(x, arc(TY[3], x) + 34, r);
	// the quay: warehouses, cranes, piers out from the wall's foot, a dry dock
	for (let x = 200; x < 1120; x += 90) if (Math.abs(x - 650) > 50 && Math.abs(x - 900) > 50) m += L.warehouse(x, arc(TY[3], x) + 18, 60, 18, Math.atan2(arc(TY[3], x + 10) - arc(TY[3], x - 10), 20) * 180 / Math.PI);
	for (const x of [380, 540, 760, 1040]) { const y = arc(TY[4], x); m += L.pier([x, y - 4], [x, y + 80], 11, { head: 16 }) + L.crane(x, y - 2, 90); }
	m += L.pier([650, arc(TY[4], 650) - 4], [650, arc(TY[4], 650) + 120], 16, { head: 24, col: '#9a7044' });
	m += L.dryDock(220, arc(TY[4], 220) + 36, 90, 60);
	const railPts = [[1380, 250], [1200, 300], [1000, 320], [800, 300], [650, 290], [500, 300]];
	m += L.rail(railPts) + L.roof(650, 270, 70, 18, 0, '#4a3a30');
	m += L.roof(560, arc(TY[1] - 40, 560), 46, 26, 0, C.velvet, { big: true }) + L.roof(740, arc(TY[1] - 40, 740), 44, 28, 0, '#3a1418', { big: true }) + L.roof(430, arc(TY[2] - 40, 430), 32, 20, 6, '#e8e0cc', { big: true }) + L.roof(860, arc(TY[1] - 40, 860), 30, 22, -4, '#6a6a70', { big: true, trim: '#cfd6dd' });
	m += L.wreck(1200, 900, 44, 60) + L.wreck(1260, 960, 30, -20) + L.wreck(40, 820, 30, 30);
	m += L.cave(60, 770, 160) + L.cave(110, 720, 170) + L.cave(1290, 880, 120);
	m += L.rocks(isWater, [0, 760, 200, 900], 18, 540, { r: 3.5 });
	m += L.lighthouse(70, 640, 160, 560);
	for (const [x, y, a, l, k] of [[460, 760, 90, 36, 0], [860, 770, 90, 32, 2], [890, 770, 90, 32, 2], [1000, 820, 20, 24, 1], [300, 820, -20, 26, 1]]) m += L.boat(x, y, a, l, k);
	m += L.liner(650, 920, shipPx, 0, 51);
	m += L.seaRoute([[650, arc(TY[4], 650) + 124], [700, 840], [800, 890], [846, 912]]);
	m += K.fight(905, arc(TY[3], 905) + 34, 46, 'crew', 'F1', mpp, { below: true }) + K.fight(650, arc(TY[2], 650), 30, 'bounty', 'F2', mpp) + K.fight(90, 600, 34, 'beast', 'F3', mpp) + K.fight(1210, 860, 56, 'beast', 'F4', mpp, { below: true }) + K.fight(110, 760, 30, 'crew', 'F5', mpp, { below: true });
	m += L.route([[650, 290], [650, arc(TY[0], 650) + 20], [700, arc(TY[1] - 40, 700) + 20], [740, arc(TY[1] - 40, 740) + 14]]);
	m += L.route([[740, arc(TY[1] - 40, 740) + 14], [820, arc(TY[1], 820)], [820, arc(TY[3], 820)], [700, arc(TY[3] + 30, 700)], [650, arc(TY[4], 650) + 100]]);
	m += L.route([[560, arc(TY[1] - 20, 560)], [300, arc(TY[1] - 10, 300)], [300, arc(TY[3] + 30, 300)], [120, 680]], { w: 3, dash: '5 7' }) + L.route([[980, arc(TY[2], 980)], [1140, 700], [1200, 820]], { w: 3, dash: '5 7' });
	m += L.step(610, 262, 1) + L.step(520, arc(TY[1] - 40, 520) + 14, 2) + L.step(780, arc(TY[1] - 40, 780) - 14, 3) + L.step(690, arc(TY[4], 690) + 110, 4) + L.step(870, 892, 5) + L.step(530, 946, 6);
	m += L.giver(560, arc(TY[1] - 40, 560) + 22) + L.giver(430, arc(TY[2] - 40, 430) + 18) + L.giver(1000, arc(TY[3], 1000) + 26);
	m += L.chest(110, 740) + L.chest(1240, 940) + L.chest(240, arc(TY[1] - 40, 240)) + L.chest(1100, arc(TY[2] - 40, 1100));
	m += L.secret(20, 690) + L.secret(650, arc(TY[4], 650) - 10);
	m += K.bigLabel(1000, 210, 'The Crest', 'station on the wall top, +60 m', { size: 24 });
	m += `<g font-size="16">` + ['+60 m', '+40 m', '+20 m', 'quay, 0 m'].map((t, k) => L.chip(1192, arc(TY[k] + 45, 1192), t, { size: 15, col: '#f6e7c1', anchor: 'start' })).join('') + `</g>`;
	m += K.bigLabel(240, 420, 'The Tiers', 'the port: hotel, bank, chapel, saloons', { size: 28 });
	m += K.bigLabel(520, 762, 'Docks & Shipyards', null, { size: 20 });
	m += K.bigLabel(110, 520, 'Sea Cliffs', null, { size: 22 }) + K.bigLabel(1180, 680, 'Flats', 'beaches below the wall', { size: 22 });
	m += K.bigLabel(650, 990, 'The Lily', null, { size: 22, fill: C.lamp });
	return {
		key: 'G', name: 'The Tiers', kind: 'WILD', tag: 'VERTICAL', area: Math.round(px * mpp * mpp), mpp, map: m, scaleAt: [40, 60], compassAt: [1250, 140],
		subtitle: 'A city stacked down the face of a colossal sea wall: the station on the crest, three streets of tiers, the docks on the quay at its foot, the sea and the Lily beyond.',
		plays: ['Vertical: four tiers and 60 m of drop. Lifts, stairs and coal', 'chutes join them; fights come from above and below, and a fall', 'is a shortcut. Cliffs west, flats east, where the wall ends.'],
		path: ['Arrive at the crest station, on the wall top', 'Port missions down the tiers', 'Win the marker: the Gilded Parlor, tier one', 'Down the lifts to the launch quay', 'Board up the anchor chain to the bow', 'The ballroom: Ned deals'],
		fights: [['crew', 'F1 coal chutes', 'bouncers on the quay bunkers'], ['bounty', 'F2 the middle lift', 'a bounty fought on the cage'], ['beast', 'F3 gull cliffs', 'gulls round the light'], ['beast', 'F4 the flats', 'crabs and tide-pool eels'], ['crew', 'F5 sea cave', 'smugglers under the headland']],
		deck: {
			how: 'the launch drops you under the bow; climb the anchor chain to the foredeck',
			route: [{ k: 'a', deck: 0, t: 0.455, v: 0.14, say: 'up the anchor chain' }, { k: 'b', deck: 0, t: 0.288, v: 0.0, say: 'take the bridge' }, { k: 'c', deck: 0, t: -0.2, v: -0.37, pre: [[0.24, -0.37]], say: 'down the port promenade' }, { k: 'd', deck: 1, t: 0.08, v: 0, via: -0.16, pre: [[-0.16, -0.37]], post: [[-0.12, 0]], say: 'grand staircase: casino' }, { k: 'e', deck: 2, t: 0.375, v: 0, via: 0.05, say: 'down: the vault' }, { k: 'f', deck: 1, t: -0.335, v: 0.12, via: 0.05, post: [[-0.16, 0]], say: 'up: the ballroom, Ned' }],
			fights: [{ deck: 2, t: 0.375, v: 0, d: 16, tag: 'vault guard', below: true }, { deck: 0, t: 0.288, v: 0, d: 14, tag: 'bridge', below: true }, { deck: 0, t: 0.0, v: -0.22, d: 12, tag: 'promenade', below: true }, { deck: 1, t: 0.08, v: 0, d: 22, tag: 'casino brawl', below: true }, { deck: 1, t: -0.335, v: 0, d: 22, tag: 'NED', below: true }],
		},
		sketch: sketchG(),
	};
}
function sketchG() {
	const { w, h } = K.SK, hz = 130;
	let s = K.skSky(w, h, hz, 200);
	s += K.skLiner(360, hz + 4, 76);
	for (let k = 0; k < 4; k++) { const y = 170 + k * 44; s += `<path d="M-10,${y + 20} Q335,${y - 20} ${w + 10},${y + 20} L${w + 10},${y + 60} Q335,${y + 20} -10,${y + 60} Z" fill="${['#2a1e28', '#24182a', '#1f1524', '#1a121e'][k]}"/>`; s += K.skTown(0, w, y + 26 - (k === 0 ? 0 : 0), 8 + k * 4, 18 + k * 6, 61 + k, ['#2e202a', '#281a26', '#221622', '#1c121c'][k], { lit: 0.45 }); }
	for (const x of [170, 480]) s += `<line x1="${x}" y1="150" x2="${x}" y2="330" stroke="#5ac8ff" stroke-width="2" opacity="0.7"/><rect x="${x - 6}" y="${x === 170 ? 230 : 200}" width="12" height="12" fill="#ffd27a"/>`;
	s += K.skPlatform(w, h, { sign: 'THE CREST', lampX: 330, lampX2: 520 });
	return s;
}

module.exports = { conceptE, conceptF, conceptG };
