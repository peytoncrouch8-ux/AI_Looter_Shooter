// Concept C, Estuary Delta: a river mouth broken into islands; the town on stilts and bridges, tide flats between the
// islands, the lighthouse on an offshore rock; maze-like.
const L = require('./lib');
const { C, f1 } = L;

function channelPaint(sp, w) {
	const dd = L.d(sp);
	return `<path d="${dd}" fill="none" stroke="${C.wetsand}" stroke-width="${w + 12}" stroke-linecap="round" stroke-linejoin="round" filter="url(#soft)"/>`
		+ `<path d="${dd}" fill="none" stroke="${C.shallow}" stroke-width="${w}" stroke-linecap="round" stroke-linejoin="round"/>`
		+ `<path d="${dd}" fill="none" stroke="${C.shelf}" stroke-width="${f1(w * 0.45)}" stroke-linecap="round" stroke-linejoin="round" filter="url(#blur3)"/>`;
}
function ford(a, b) { return `<path d="${L.d([a, b])}" stroke="#d6c39a" stroke-width="9" stroke-dasharray="5 4" stroke-linecap="round" opacity="0.95"/>`; }

module.exports = function conceptC(SHORE_M2, SHIP_M) {
	const mainRaw = [[-80, -80], [1560, -80], [1560, 1360], [1390, 1360], [1350, 1120], [1370, 900], [1330, 760], [1296, 650], [1262, 540], [1196, 462], [1150, 450], [1096, 452], [1000, 474], [880, 486], [760, 502], [640, 520], [520, 540], [400, 560], [300, 590], [220, 630], [160, 652], [100, 640], [40, 610], [-80, 600]];
	const main = L.roughen(mainRaw, true, 5, 151, 10);
	const islandRaw = {
		I1: [[1040, 510], [1110, 496], [1166, 530], [1160, 600], [1096, 646], [1026, 636], [996, 576]],
		I2: [[874, 540], [952, 530], [986, 604], [960, 680], [890, 700], [844, 650], [848, 576]],
		I3: [[1196, 646], [1262, 630], [1300, 708], [1290, 820], [1230, 902], [1160, 884], [1128, 786], [1140, 694]],
		I4: [[1006, 704], [1084, 694], [1110, 776], [1090, 864], [1020, 904], [960, 860], [952, 772]],
		I5: [[694, 566], [786, 562], [818, 640], [800, 742], [740, 800], [660, 784], [628, 694], [648, 612]],
		I6: [[836, 762], [916, 750], [940, 830], [920, 930], [858, 982], [780, 962], [758, 880], [786, 800]],
		I7: [[1056, 944], [1136, 932], [1186, 990], [1166, 1082], [1092, 1112], [1032, 1072], [1020, 1000]],
		I8: [[616, 860], [700, 846], [724, 930], [690, 1012], [608, 1032], [556, 980], [566, 908]],
	};
	const islands = Object.fromEntries(Object.entries(islandRaw).map(([k, v], i) => [k, L.roughen(v, true, 7, 160 + i, 8)]));
	const rockRaw = [[190, 840], [236, 816], [282, 834], [292, 884], [258, 922], [204, 924], [176, 888]];
	const rock = L.roughen(rockRaw, true, 3, 170, 8);
	const skerries = [[[360, 930], [378, 922], [388, 940], [368, 950]], [[110, 760], [128, 752], [136, 770], [116, 778]], [[480, 1010], [496, 1004], [504, 1020], [486, 1026]]].map((r, i) => L.roughen(r, true, 2, 175 + i, 6));
	const ridgeN = L.roughen([[-80, -80], [1230, -80], [1220, 60], [1150, 200], [1080, 300], [960, 346], [760, 376], [540, 396], [330, 426], [150, 448], [-80, 466]], true, 6, 152, 8);
	const ridgeE = L.roughen([[1336, -80], [1560, -80], [1560, 1360], [1330, 1360], [1300, 1120], [1320, 900], [1300, 760], [1310, 620], [1262, 520], [1236, 400], [1266, 260], [1316, 100]], true, 6, 153, 8);
	const ridges = [ridgeN, ridgeE];
	const river = L.roughen([[1300, -80], [1268, 120], [1200, 300], [1150, 420], [1130, 470]], false, 2.5, 154, 10);
	const allIslands = Object.values(islands);
	const coastD = [L.d(main, true), ...allIslands.map(i => L.d(i, true))].join(' ');
	const inRiver = (p, pad = 0) => L.lineDist(p, river) < 22 + pad;
	const isLand = p => ((L.pip(p, main) && !inRiver(p)) || L.inAny(p, allIslands) || L.pip(p, rock) || L.inAny(p, skerries));
	const isPlay = p => ((L.pip(p, main) && !L.inAny(p, ridges) && !inRiver(p)) || L.inAny(p, allIslands) || L.pip(p, rock));
	const isWater = p => !isLand(p);
	const px = L.playArea(isPlay), mpp = Math.sqrt(SHORE_M2 / px), shipPx = SHIP_M / mpp;
	console.log(`C: play ${px} px2, mpp ${mpp.toFixed(3)}, ship ${shipPx.toFixed(0)} px`);

	const railPts = [[-80, 150], [120, 190], [300, 250], [430, 320], [540, 390], [620, 428], [740, 420]];
	const spur = [[620, 428], [520, 470], [450, 500], [430, 520]];
	const bankSt = [[600, 470], [720, 456], [840, 440], [960, 420], [1060, 400], [1110, 380]];
	const dockRd = [[600, 470], [520, 500], [420, 520], [330, 548]];
	const headRd = [[330, 548], [220, 560], [120, 590]];
	const roads = [bankSt, dockRd];
	const bld = [];
	let m = L.waterLayer([coastD, L.d(rock, true), ...skerries.map(r => L.d(r, true))]);
	m += L.waves(isWater, 1100, 7);
	// tide flats between the islands and along the front
	m += L.flats([[560, 1000], [640, 1050], [760, 1010], [860, 1010], [980, 1030], [1060, 1120], [980, 1170], [820, 1110], [680, 1110], [560, 1080]], 155);
	m += L.flats([[730, 800], [790, 820], [760, 900], [720, 900], [700, 840]], 156);
	m += L.flats([[940, 900], [1010, 920], [1030, 990], [960, 1000], [920, 960]], 157);
	m += L.flats([[150, 930], [210, 960], [290, 940], [320, 990], [240, 1010], [160, 990]], 158);
	const upland = [L.d(L.roughen([[-80, -80], [1300, -80], [1250, 380], [1000, 440], [700, 470], [450, 510], [200, 560], [-80, 560]], true, 8, 159, 6), true),
		...allIslands.map((isl, k) => { const cx = isl.reduce((a, p) => a + p[0], 0) / isl.length, cy = isl.reduce((a, p) => a + p[1], 0) / isl.length; return L.d(isl.map(p => [cx + (p[0] - cx) * 0.7, cy + (p[1] - cy) * 0.7]), true); })];
	m += L.landLayer('landC', coastD, upland);
	m += `<path d="${L.d(rock, true)}" fill="#6e5c4c" stroke="#2b211b" stroke-width="1.5"/>` + skerries.map(r => `<path d="${L.d(r, true)}" fill="#6e5c4c" stroke="#2b211b" stroke-width="1.5"/>`).join('');
	m += L.hill([[200, 830], [270, 830], [286, 880], [250, 914], [200, 910]], 171, 2);
	m += L.hill([[-60, 430], [110, 420], [200, 480], [200, 580], [130, 640], [30, 600], [-60, 560]], 172, 3);
	m += L.ridgeLayer(ridges.map(r => L.d(r, true)), [], 'landC');
	m += L.trees(p => L.inAny(p, ridges) && L.pip(p, main) && !inRiver(p, 8) && L.lineDist(p, railPts) > 20, [0, 0, 1440, 1264], 220, 173, { col: '#2e3922', r: 5 });
	m += channelPaint(river, 44);
	m += L.cliff([[220, 630], [160, 652], [100, 640], [40, 610], [-80, 600]], -1, 174, { len: 13 });
	m += L.cliff(rockRaw.slice(0, 5), -1, 175, { len: 9 });
	m += L.cliff([[1220, 60], [1150, 200], [1080, 300], [960, 346], [760, 376], [540, 396], [330, 426], [150, 448], [-80, 466]], 1, 176, { len: 9, w: 3 });
	m += L.cliff([[1330, 1360], [1300, 1120], [1320, 900], [1300, 760], [1310, 620], [1262, 520], [1236, 400], [1266, 260], [1316, 100]], 1, 177, { len: 9, w: 3 });
	for (const r of roads) m += L.road(r, 9);
	m += L.road(headRd, 6, { col: '#bfa27a' });
	m += `<path d="${L.smooth(railPts, false, 6)}" fill="none" stroke="#7a6450" stroke-width="26" stroke-linecap="round" opacity="0.9"/><path d="${L.smooth(railPts, false, 6)}" fill="none" stroke="#1f1712" stroke-width="30" stroke-dasharray="2 6" opacity="0.5"/>`;
	m += L.rail(railPts) + L.rail(spur);
	// boardwalks on the islands
	const walks = [[[1010, 560], [1090, 540], [1140, 580]], [[870, 600], [940, 590]], [[680, 640], [770, 620]], [[990, 760], [1070, 760]], [[810, 860], [900, 850]], [[1170, 740], [1260, 760], [1250, 840]], [[1050, 1010], [1140, 1010]], [[600, 940], [680, 920]]];
	for (const w of walks) m += `<path d="${L.d(w)}" fill="none" stroke="#3a2618" stroke-width="7" stroke-linecap="round"/><path d="${L.d(w)}" fill="none" stroke="#8a6440" stroke-width="5" stroke-linecap="round"/>`;
	// stilt houses round the island edges and along the river mouth
	const R = L.rng(180);
	const stiltRing = (isl, n, seed) => {
		const ring = L.spline(isl, true, 6), pts = L.walk([...ring, ring[0]], 22, 4);
		let s = '', c = 0;
		for (const { p, ang } of pts) { if (c >= n || R() < 0.35) continue; c++; const nx = -Math.sin(ang), ny = Math.cos(ang); const x = p[0] + nx * 4, y = p[1] + ny * 4; s += L.stilts(x, y, 15 + R() * 5, 11 + R() * 3, ang * 180 / Math.PI, seed + c); bld.push([x, y]); }
		return s;
	};
	m += stiltRing(islands.I1, 18, 400) + stiltRing(islands.I2, 12, 500) + stiltRing(islands.I4, 6, 600) + stiltRing(islands.I5, 5, 700);
	for (const { p, ang } of L.walk([[880, 492], [1000, 480], [1096, 462], [1160, 460]], 24, 4)) { const x = p[0], y = p[1] + 4; m += L.stilts(x, y, 16, 12, ang * 180 / Math.PI, 800 + Math.round(x)); bld.push([x, y]); }
	const tN = L.town(bankSt, { step: 24, off: 15, w: 18, h: 14, sides: [-1], gaps: 0.2 }, 181, bld);
	m += tN.s;
	m += L.hill([[946, 344], [1050, 330], [1066, 392], [992, 404], [946, 386]], 178, 2, '#8d7b52');
	m += L.roof(1004, 366, 34, 20, -14, '#e8e0cc', { big: true }); // chapel on the bank above Stilt Town
	m += L.roof(1066, 590, 44, 28, -20, '#3a1418', { big: true }); // the Gilded Parlor, on stilts
	m += L.roof(880, 470, 40, 22, -6, C.velvet, { big: true }); // hotel at the waterfront
	m += L.roof(920, 620, 28, 20, -20, '#6a6a70', { big: true, trim: '#cfd6dd' }); // bank, on its own island
	m += L.roof(760, 412, 56, 16, -6, '#4a3a30'); // station
	// docks at the west end of the north bank
	for (const [a, b] of [[[470, 546], [480, 612]], [[540, 534], [550, 600]], [[400, 562], [404, 630]]]) m += L.pier(a, b, 11, { head: 16 });
	m += L.dryDock(290, 600, 100, 74);
	for (const [x, y, a] of [[480, 552, 84], [548, 540, 84], [408, 570, 86]]) m += L.crane(x, y, a);
	for (const [x, y, w, h, a] of [[520, 500, 52, 22, -10], [600, 492, 40, 20, -8]]) { m += L.warehouse(x, y, w, h, a); bld.push([x, y]); }
	for (const [x, y, r] of [[440, 512, 12], [466, 500, 10], [418, 498, 9]]) m += L.coalHeap(x, y, r);
	m += L.pier([360, 572], [330, 660], 12, { head: 18, col: '#9a7044' }); // the launch's pier
	// bridges and fords between the islands: the maze's doors
	for (const [a, b] of [[[1070, 458], [1068, 506]], [[990, 576], [952, 586]], [[848, 600], [814, 610]], [[1060, 640], [1050, 698]], [[1112, 770], [1132, 770]], [[890, 702], [870, 754]], [[1168, 590], [1200, 650]], [[730, 520], [728, 566]]]) m += L.bridge(a, b, 10);
	m += ford([948, 830], [962, 830]) + ford([1030, 900], [1060, 944]) + ford([760, 900], [716, 910]) + ford([944, 930], [1022, 990]) + ford([1290, 700], [1310, 700]);
	// wrecks on the bar, the wreck camp, coves, caves
	m += L.wreck(420, 960, 40, 20) + L.wreck(620, 1070, 36, -30) + L.wreck(660, 1000, 30, 50) + L.wreck(90, 700, 30, 60);
	for (const [x, y] of [[640, 960], [660, 980], [630, 990]]) m += `<circle cx="${x}" cy="${y}" r="5" fill="#d9c3a0" stroke="#3a2a1a"/>`;
	m += `<circle cx="646" cy="976" r="10" fill="url(#lampGlow)"/>`;
	m += L.cave(60, 618, 170) + L.cave(150, 652, 190) + L.cave(282, 880, 90) + L.cave(1176, 1060, 60);
	const avoid = [...roads, headRd, railPts, spur, ...walks];
	const okT = p => isPlay(p) && !inRiver(p, 8) && avoid.every(l => L.lineDist(p, l) > 12) && bld.every(q => L.dist(p, q) > 16);
	m += L.trees(okT, [0, 380, 1320, 1120], 130, 182, { r: 5.5 });
	m += L.rocks(p => isPlay(p) && p[0] < 210, [-20, 420, 220, 650], 26, 183);
	m += L.rocks(isWater, [60, 700, 420, 1000], 22, 184, { r: 3 });
	m += tN.lamps;
	for (const w of walks) for (const p of L.walk(w, 30)) m += L.lamp(p.p[0], p.p[1], 0.7);
	m += L.lighthouse(238, 870, 160, 560);
	for (const [x, y, a, l, k] of [[500, 640, -10, 30, 2], [530, 650, -10, 30, 2], [980, 700, 60, 20, 1], [820, 720, -40, 20, 1], [1180, 920, 30, 20, 1], [380, 760, 30, 36, 0], [880, 1030, 10, 24, 1], [600, 760, -30, 22, 1]]) m += L.boat(x, y, a, l, k);
	for (const [x, y] of [[360, 700], [380, 800], [420, 880], [470, 960]]) m += L.buoy(x, y);

	// the liner out past the bar
	const ship = [470, 1150];
	m += L.liner(ship[0], ship[1], shipPx, 14, 11);
	m += L.anchorIcon(ship[0] + shipPx / 2 + 26, ship[1] + 30);
	m += L.seaRoute([[330, 660], [340, 760], [360, 860], [330, 980], [320, 1060], [330, 1110]]);
	m += L.launchBoat(352, 820, 90);

	// enemy areas
	m += L.zone(450, 500, 54, 34, 'crew') + L.foeIcon(480, 470, 'crew');
	m += L.zone(1100, 1020, 70, 60, 'crew') + L.foeIcon(1150, 1000, 'crew');
	m += L.zone(1215, 770, 60, 70, 'crew') + L.foeIcon(1250, 730, 'crew');
	m += L.zone(720, 690, 60, 50, 'crew') + L.foeIcon(700, 720, 'crew');
	m += L.zone(650, 940, 60, 50, 'beast') + L.foeIcon(600, 900, 'crab');
	m += L.zone(860, 1060, 110, 40, 'beast', { rot: 10 }) + L.foeIcon(800, 1060, 'crab');
	m += L.zone(240, 975, 70, 30, 'beast') + L.foeIcon(190, 976, 'eel');
	m += L.zone(990, 950, 44, 36, 'beast', { rot: -40 }) + L.foeIcon(982, 960, 'eel');
	m += L.zone(90, 520, 70, 60, 'beast') + L.foeIcon(80, 500, 'gull');
	m += L.zone(240, 860, 56, 50, 'beast') + L.foeIcon(214, 848, 'gull');

	// the player's path through the maze
	m += L.route([[770, 426], [860, 440], [980, 420], [1070, 440], [1070, 500], [1066, 560]]);
	m += L.route([[1066, 560], [1056, 680], [1040, 780], [960, 830], [880, 850], [880, 760], [900, 640], [820, 610], [740, 640], [730, 540]], { w: 3, dash: '5 7' });
	m += L.route([[1040, 780], [1130, 770], [1220, 760], [1180, 590]], { w: 3, dash: '5 7' });
	m += L.route([[880, 850], [880, 940], [990, 960], [1090, 1010]], { w: 3, dash: '5 7' }) + L.route([[880, 940], [740, 905], [660, 940]], { w: 3, dash: '5 7' });
	m += L.route([[1066, 560], [1000, 580], [930, 600], [850, 605], [760, 560], [730, 520], [640, 490], [520, 510], [420, 530], [360, 572], [340, 640]]);
	m += L.step(800, 380, 1) + L.step(1124, 420, 2) + L.step(1110, 620, 3) + L.step(300, 680, 4) + L.step(296, 1092, 5) + L.step(372, 1136, 6);
	m += L.giver(1030, 420) + L.giver(1004, 396) + L.giver(1046, 620);
	m += L.giver(420, 600, 'side') + L.giver(238, 900, 'side') + L.giver(660, 1010, 'side') + L.giver(1240, 840, 'side') + L.giver(140, 560, 'side');
	m += L.bounty(720, 424);
	m += L.chest(1180, 1040) + L.chest(70, 640) + L.chest(290, 900) + L.chest(620, 1050) + L.chest(1000, 980) + L.chest(1280, 790) + L.chest(760, 760) + L.chest(940, 640);
	m += L.secret(1170, 1080) + L.secret(110, 770) + L.secret(1100, 880) + L.secret(1330, 560);
	m += L.slots(416, 1124);
	m += L.station(690, 412);
	// names
	m += L.district(470, 330, 'The Port', 'station · saloons · hotel · bank · chapel', { size: 26 }) + L.district(470, 214, 'Tidewrack Bluffs', 'out of bounds', { size: 18, fill: '#cdb89a' });
	m += L.district(1186, 470, 'Stilt Town', null, { size: 20, fill: '#f6e7c1' });
	m += L.district(330, 404, 'Docks & Shipyards', null, { size: 18 });
	m += L.district(110, 700, 'Gull Head', 'sea cliffs · caves', { size: 18 });
	m += L.district(240, 790, "Hester's Rock", null, { size: 18 });
	m += L.district(830, 1150, 'The Delta Flats', 'wreck camp · tide pools · fords at low tide', { size: 22 });
	m += L.district(1170, 1160, "Smugglers' Reach", null, { size: 15 });
	m += L.district(1000, 730, 'The Islands', null, { size: 16 }).replace('1000', '1000');
	m += L.chip(1004, 336, "Saint Hester's Chapel") + L.chip(866, 500, 'Marquee Hotel', { anchor: 'end' }) + L.chip(1130, 650, 'the Gilded Parlor', { anchor: 'start', col: C.lamp });
	m += L.chip(920, 655, 'Bank') + L.chip(1052, 372, 'Harbor Master', { anchor: 'start' });
	m += L.chip(280, 650, 'dry dock', { anchor: 'end' }) + L.chip(410, 520, 'coal bunkers', { anchor: 'end' }) + L.chip(316, 720, 'launch pier', { anchor: 'end', col: C.lamp });
	m += L.chip(616, 1000, 'wreck camp', { anchor: 'end' }) + L.chip(1200, 1100, "smugglers' cove", { anchor: 'start' });
	m += L.chip(668, 412, 'station', { anchor: 'end' }) + L.chip(150, 160, "rail from Ransom's Rest");
	m += L.district(470, 1240, 'The Gilded Lily', 'anchored past the bar', { size: 20, fill: C.lamp });
	m += L.district(1385, 250, 'river gorge', null, { size: 14, fill: '#cdb89a', rot: 76 });
	return {
		key: 'C', name: 'Estuary Delta', tag: 'MAZE', area: Math.round(px * mpp * mpp), mpp, shipM: SHIP_M,
		subtitle: 'A river mouth broken into islands: the town on stilts and bridges, tide flats between, the lighthouse on an offshore rock.',
		plays: ['Maze: islands joined by bridges and tide fords, many ways', 'across and few straight lines. Bridges are chokepoints, creeks', 'hide coves and chests, and the low tide opens new fords. Easy', 'to get lost, rich to explore; the lighthouse is the landmark.'],
		path: [['Arrive by train', 'the station on the north bank'], ['Port missions', 'Stilt Town, then over the bridges'], ['Win a gilded marker', 'the Gilded Parlor, a card house on stilts'], ['The launch', 'from the docks, out over the bar'], ['Board', 'the stern gangway, past Hester\'s Rock'], ['The ballroom', 'Ned deals the last hand']],
		deck: {
			lines: [['a', 'stern gangway onto the aft deck'], ['b', 'down to the casino: the hub'], ['c', 'vault key below (either order)'], ['d', 'bridge key above (either order)'], ['e', 'both keys: the ballroom opens']],
			route: [{ k: 'a', deck: 0, t: -0.44, y: 0.22 }, { k: 'b', deck: 1, t: 0.08, y: -0.05, via: -0.13, viaY: 0.0 }, { k: 'c', deck: 2, t: 0.39, y: 0.0, via: 0.04 }, { k: 'd', deck: 0, t: 0.3, y: 0.0, via: 0.36 }, { k: 'e', deck: 1, t: -0.31, y: 0.12, via: -0.13 }],
			foes: [{ deck: 1, t: 0.22, y: 0.12 }, { deck: 2, t: -0.02, y: 0 }, { deck: 0, t: 0.12, y: -0.38 }, { deck: 0, t: -0.3, y: 0.38 }], chests: [{ deck: 2, t: -0.36, y: 0.27 }, { deck: 1, t: 0.3, y: -0.3 }],
		},
		map: m, scaleAt: [1000, 1232], compassAt: [1290, 60],
	};
};
