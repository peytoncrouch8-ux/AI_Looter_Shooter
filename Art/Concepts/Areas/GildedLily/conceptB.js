// Concept B, The Long Spit: one long sandbar out from the mainland, the rail running out along it; linear.
const L = require('./lib');
const { C } = L;

module.exports = function conceptB(SHORE_M2, SHIP_M) {
	const landRaw = [[1560, -80], [560, -80], [540, 0], [580, 50], [700, 70], [860, 86], [1000, 70], [1140, 96], [1260, 150], [1320, 260], [1300, 420], [1320, 600], [1290, 740], [1220, 820], [1140, 830], [1060, 790], [960, 724], [850, 650], [740, 572], [640, 490], [560, 420], [490, 352], [440, 300], [392, 252], [330, 220], [272, 232], [236, 280], [240, 340], [282, 386], [330, 420], [372, 470], [440, 540], [520, 618], [612, 700], [700, 772], [790, 846], [880, 918], [960, 990], [1012, 1060], [1000, 1140], [960, 1220], [930, 1290], [920, 1360], [1560, 1360]];
	const coast = L.roughen(landRaw, true, 5, 81, 10), coastD = L.d(coast, true);
	const rocksOff = [[[160, 250], [186, 238], [200, 262], [176, 276]], [[196, 420], [214, 412], [222, 430], [204, 440]], [[410, 600], [428, 594], [434, 612], [414, 618]]].map((r, i) => L.roughen(r, true, 2, 90 + i, 6));
	const ridge = L.roughen([[1560, -80], [480, -80], [500, 60], [700, 110], [900, 120], [1100, 120], [1210, 190], [1250, 330], [1230, 480], [1250, 640], [1290, 780], [1330, 920], [1300, 1060], [1220, 1170], [1150, 1260], [1120, 1360], [1560, 1360]], true, 6, 82, 8);
	const isLand = p => L.pip(p, coast) || L.inAny(p, rocksOff);
	const isPlay = p => L.pip(p, coast) && !L.pip(p, ridge);
	const isWater = p => !isLand(p);
	const px = L.playArea(isPlay), mpp = Math.sqrt(SHORE_M2 / px), shipPx = SHIP_M / mpp;
	console.log(`B: play ${px} px2, mpp ${mpp.toFixed(3)}, ship ${shipPx.toFixed(0)} px`);

	// the spit's spine: the road and the rail run out along it
	const spine = [[1180, 1000], [1080, 930], [980, 860], [880, 790], [780, 710], [690, 630], [600, 550], [520, 480], [450, 410], [380, 330]];
	const railPts = [[1480, 1080], [1330, 1060], [1240, 1030], [1180, 990], [1090, 920], [990, 850], [890, 778], [800, 700], [740, 640]];
	const mainSt = [[1240, 1070], [1180, 1000], [1110, 960], [1060, 1010], [1050, 1110], [1080, 1190]];
	const crossSt = [[1110, 960], [1170, 900], [1210, 860]];
	const tipRd = [[450, 410], [400, 400], [340, 380], [290, 330]];
	const roads = [spine, mainSt, crossSt];
	const bld = [];
	let m = L.waterLayer([coastD, ...rocksOff.map(r => L.d(r, true))]);
	m += L.waves(isWater, 900, 5);
	m += L.flats([[1060, 700], [1160, 740], [1250, 770], [1200, 830], [1120, 836], [1020, 780]], 83);
	m += L.flats([[640, 760], [760, 820], [880, 920], [960, 1000], [990, 1120], [900, 1150], [760, 1050], [640, 920], [580, 820]], 84);
	m += L.flats([[200, 300], [250, 380], [300, 430], [260, 460], [190, 420], [160, 340]], 85);
	const upland = [L.d(L.roughen([[960, 820], [1300, 820], [1320, 1300], [1000, 1300], [1030, 1060], [930, 930]], true, 8, 86, 6), true),
		L.d(L.roughen([[930, 780], [820, 720], [700, 630], [600, 540], [530, 470], [560, 450], [660, 530], [760, 620], [870, 700], [960, 760]], true, 4, 87, 6), true)];
	m += L.landLayer('landB', coastD, upland);
	m += rocksOff.map(r => `<path d="${L.d(r, true)}" fill="#6e5c4c" stroke="#2b211b" stroke-width="1.5"/>`).join('');
	m += L.hill([[260, 240], [340, 228], [410, 280], [420, 330], [370, 380], [300, 380], [250, 330]], 88, 3);
	m += L.ridgeLayer([L.d(ridge, true)], [], 'landB');
	m += L.trees(p => L.pip(p, ridge) && L.pip(p, coast), [480, 0, 1440, 1264], 200, 89, { col: '#2e3922', r: 5 });
	m += L.cliff([[392, 252], [330, 220], [272, 232], [236, 280], [240, 340], [282, 386]], -1, 91, { len: 13 });
	m += L.cliff([[500, 60], [700, 110], [900, 120], [1100, 120], [1210, 190], [1250, 330], [1230, 480], [1250, 640], [1290, 780]], 1, 92, { len: 10, w: 3 });
	m += L.cliff([[1290, 780], [1330, 920], [1300, 1060], [1220, 1170], [1150, 1260]], 1, 93, { len: 10, w: 3 });
	for (const t of [[[880, 790], [820, 860], [760, 900]], [[600, 550], [540, 620], [480, 640]], [[1050, 1110], [990, 1180], [960, 1240]]]) m += L.trail(t);
	for (const r of roads) m += L.road(r, r === mainSt ? 11 : 8);
	m += L.road(tipRd, 6, { col: '#bfa27a' });
	m += L.rail(railPts);
	// the port town at the root
	const t1 = L.town(mainSt, { step: 22, off: 15, w: 19, h: 15 }, 101, bld);
	const t2 = L.town(spine.slice(0, 3), { step: 24, off: 15, w: 18, h: 14, gaps: 0.3, skip: p => L.dist(p, [1180, 1000]) < 30 }, 102, bld);
	const t3 = L.town(crossSt, { step: 20, off: 13, w: 15, h: 13, sides: [-1] }, 103, bld);
	m += t1.s + t2.s + t3.s;
	m += L.roof(1236, 1010, 56, 16, 34, '#4a3a30'); // station roof
	m += L.roof(1150, 1130, 34, 20, -20, '#e8e0cc', { big: true }); // chapel
	m += L.roof(1010, 960, 42, 28, 30, C.velvet, { big: true }); // hotel
	m += L.roof(1110, 1060, 30, 22, 0, '#6a6a70', { big: true, trim: '#cfd6dd' }); // bank
	m += L.roof(1196, 846, 28, 20, 30, '#3e5a5a'); // harbor master
	// docks & shipyards on the sheltered side, mid-spit
	m += L.pier([1206, 838], [1250, 760], 10, { head: 14 });
	m += L.pier([980, 735], [1030, 660], 11, { head: 16 }) + L.pier([900, 676], [950, 600], 11, { head: 16 });
	m += L.dryDock(820, 600, 120, -52);
	m += L.pier([760, 560], [800, 490], 12, { col: '#3a3634', lamps: false });
	for (const [x, y, a] of [[990, 728, -60], [910, 668, -60], [770, 556, -58], [860, 640, -50]]) m += L.crane(x, y, a);
	for (const [x, y, w, h, a] of [[940, 760, 62, 24, 36], [860, 700, 58, 22, 38], [700, 600, 50, 20, 42]]) { m += L.warehouse(x, y, w, h, a); bld.push([x, y]); }
	for (const [x, y, r] of [[740, 620, 12], [720, 596, 10], [760, 650, 9], [700, 570, 9]]) m += L.coalHeap(x, y, r);
	// Ned's card barge moored at the docks (the marker game)
	m += `<g transform="translate(1070 640) rotate(-55)"><rect x="-34" y="-12" width="68" height="24" rx="10" fill="#3a1418" stroke="${C.gold}" stroke-width="2"/><rect x="-22" y="-7" width="44" height="14" fill="${C.velvet}" stroke="${C.gold}" stroke-width="1"/></g>` + L.lamp(1070, 640, 1.6);
	m += L.pier([1012, 716], [1060, 650], 6, { lamps: false });
	// beaches & coves on the sea side
	m += L.wreck(560, 720, 44, 40) + L.wreck(720, 860, 36, -20) + L.wreck(180, 262, 30, 10);
	for (const [x, y] of [[588, 700], [600, 730], [574, 748]]) m += `<circle cx="${x}" cy="${y}" r="5" fill="#d9c3a0" stroke="#3a2a1a"/>`;
	m += `<circle cx="594" cy="724" r="10" fill="url(#lampGlow)"/>`;
	m += L.cave(250, 300, 100) + L.cave(330, 226, 180) + L.cave(470, 540, 140);
	// tip: the lighthouse and the launch's pier under it
	m += L.pier([300, 400], [250, 460], 12, { head: 20, col: '#9a7044' });
	m += `<rect x="292" y="392" width="16" height="8" fill="${C.gold}" stroke="#3a2a10" transform="rotate(-50 300 396)"/>`;
	m += L.lighthouse(310, 268, 200, 520);
	const avoid = [...roads, tipRd, railPts];
	const okT = p => isPlay(p) && avoid.every(l => L.lineDist(p, l) > 12) && bld.every(q => L.dist(p, q) > 16);
	m += L.trees(p => okT(p) && L.dist(p, [1150, 1130]) > 30, [960, 820, 1320, 1300], 50, 104);
	m += L.trees(p => okT(p) && !L.pip(p, [[260, 240], [340, 228], [410, 280], [420, 330], [370, 380], [300, 380], [250, 330]]), [420, 380, 980, 900], 50, 105, { r: 5 });
	m += L.rocks(isPlay, [240, 220, 420, 390], 24, 106);
	m += L.rocks(isWater, [140, 200, 260, 480], 16, 107, { r: 3 });
	m += t1.lamps + t2.lamps + t3.lamps;
	for (const p of L.walk(L.spline(spine.slice(3), false, 6), 46)) m += L.lamp(p.p[0] + 12, p.p[1] - 12, 0.7);

	// the liner anchored off the sea side, the launch from the tip
	const ship = [250, 840];
	m += L.liner(ship[0], ship[1], shipPx, -62, 9);
	m += L.anchorIcon(ship[0] + 100, ship[1] - 190);
	m += L.seaRoute([[250, 460], [250, 560], [270, 660], [300, 760]]);
	m += L.launchBoat(256, 580, 90);

	// enemy areas, in order along the spit
	m += L.zone(735, 610, 60, 40, 'crew', { rot: -40 }) + L.foeIcon(690, 640, 'crew');
	m += L.zone(480, 600, 48, 34, 'crew', { rot: 40 }) + L.foeIcon(500, 640, 'crew');
	m += L.zone(330, 420, 46, 30, 'crew', { rot: -40 }) + L.foeIcon(370, 440, 'crew');
	m += L.zone(840, 900, 80, 40, 'beast', { rot: 40 }) + L.foeIcon(850, 906, 'crab');
	m += L.zone(640, 760, 50, 34, 'beast', { rot: 40 }) + L.foeIcon(650, 790, 'crab');
	m += L.zone(220, 360, 44, 64, 'beast') + L.foeIcon(212, 380, 'eel');
	m += L.zone(330, 270, 70, 44, 'beast', { rot: -20 }) + L.foeIcon(372, 300, 'gull');
	m += L.zone(1150, 780, 70, 26, 'beast', { rot: 20 }) + L.foeIcon(1110, 774, 'eel');
	// boats on the sound: coal lighters by the bunkers, fishing boats, buoys on the channel
	for (const [x, y, a, l, k] of [[800, 470, -40, 34, 2], [830, 450, -30, 34, 2], [1110, 690, 20, 26, 1], [1150, 650, 40, 24, 1], [1000, 560, -60, 40, 0], [620, 300, 10, 44, 0], [900, 380, 70, 26, 1], [560, 160, -20, 30, 1]]) m += L.boat(x, y, a, l, k);
	for (const [x, y] of [[500, 220], [560, 270], [640, 380], [720, 420]]) m += L.buoy(x, y);

	// the player's path: one road out to the end of the spit
	m += L.route([[1236, 1040], [1180, 1000], [1080, 930], [980, 860], [880, 790], [780, 710], [690, 630], [600, 550], [520, 480], [450, 410], [360, 400], [300, 400]]);
	m += L.route([[1110, 960], [1180, 880]], { w: 3, dash: '5 7' }) + L.route([[1060, 1010], [1050, 1110], [1080, 1180]], { w: 3, dash: '5 7' });
	m += L.route([[880, 790], [800, 880], [740, 900]], { w: 3, dash: '5 7' }) + L.route([[600, 550], [560, 690]], { w: 3, dash: '5 7' });
	m += L.route([[1012, 716], [1040, 676]], { w: 3, dash: '5 7' });
	m += L.step(1270, 1004, 1) + L.step(1124, 1004, 2) + L.step(1050, 700, 3) + L.step(286, 446, 4) + L.step(352, 784, 5) + L.step(205, 925, 6);
	m += L.giver(1196, 870) + L.giver(1150, 1158) + L.giver(1012, 736);
	m += L.giver(940, 790, 'side') + L.giver(596, 760, 'side') + L.giver(330, 300, 'side') + L.giver(1040, 1040, 'side');
	m += L.bounty(1290, 1050);
	m += L.chest(470, 520) + L.chest(250, 320) + L.chest(720, 880) + L.chest(820, 570) + L.chest(186, 270) + L.chest(1280, 900) + L.chest(300, 800);
	m += L.secret(330, 240) + L.secret(1080, 1230) + L.secret(420, 610);
	m += L.slots(326, 846);
	m += L.station(1290, 980);
	// names
	m += L.district(880, 1196, 'The Port', 'station · saloons · hotel · bank · chapel', { size: 26 });
	m += L.district(900, 520, 'Docks & Shipyards', 'cranes · dry dock · coal bunkers · Ned\'s barge', { size: 22 });
	m += L.district(270, 160, 'The Point', 'sea cliffs · lighthouse · caves', { size: 22 });
	m += L.district(760, 1010, 'Beaches & Tide Flats', 'wreck camps · smugglers\' coves', { size: 22 });
	m += L.district(830, 330, 'Lily Sound', 'sheltered water', { size: 26, fill: '#bfeaff' });
	m += L.district(470, 470, 'The Neck', null, { size: 16, fill: '#f6e7c1', rot: -45 });
	m += L.district(1362, 560, 'Blackrock Heights', null, { size: 18, fill: '#cdb89a', rot: 82 });
	m += L.district(200, 1030, 'The Gilded Lily', 'anchored off the bar', { size: 20, fill: C.lamp });
	m += L.chip(1150, 1186, "Saint Hester's Chapel") + L.chip(1000, 1000, 'Marquee Hotel') + L.chip(1196, 820, 'Harbor Master', { anchor: 'start' });
	m += L.chip(1100, 1080, 'Bank', { anchor: 'end' }) + L.chip(1102, 610, "Ned's card barge", { anchor: 'start', col: C.lamp });
	m += L.chip(806, 650, 'dry dock', { anchor: 'end' }) + L.chip(690, 560, 'coal bunkers', { anchor: 'end' }) + L.chip(350, 250, 'lighthouse', { anchor: 'start' });
	m += L.chip(560, 694, 'wreck camp', { anchor: 'end' }) + L.chip(470, 572, "smugglers' cave", { anchor: 'end' }) + L.chip(150, 470, 'tide pools');
	m += L.chip(236, 498, "launch pier", { anchor: 'end', col: C.lamp });
	m += L.chip(1310, 960, 'station', { anchor: 'start' }) + L.chip(1340, 1106, 'rail');
	return {
		key: 'B', name: 'The Long Spit', tag: 'LINEAR', area: Math.round(px * mpp * mpp), mpp, shipM: SHIP_M,
		subtitle: 'A sandbar a half-kilometre long: town at the root, docks on the sheltered sound, beaches on the sea side, cliffs and the lighthouse at the tip.',
		plays: ['Linear: one road and one rail out along the spit, each district', 'a step further from town and a little more Hollow. Side', 'trails dip to the sea side and come back. The Neck is a', 'choke; the Lily rides at anchor ahead the whole walk out.'],
		path: [['Arrive by train', 'the station at the root of the spit'], ['Port missions', 'in town, then outward: docks, beaches'], ['Win a gilded marker', 'the rigged game on Ned\'s card barge'], ['The launch', 'from the pier under the lighthouse'], ['Board', 'by the coaling port, low and quiet'], ['The ballroom', 'Ned deals the last hand']],
		deck: {
			lines: [['a', 'coaling port: in with the coal'], ['b', 'boiler rooms, then the vault'], ['c', 'service stairs up: the casino'], ['d', 'grand staircase up: promenade'], ['e', 'fight forward to the bridge'], ['f', 'back down: the ballroom']],
			route: [{ k: 'a', deck: 2, t: -0.1, y: 0.48 }, { k: 'b', deck: 2, t: 0.39, y: 0.0 }, { k: 'c', deck: 1, t: 0.14, y: 0.0, via: 0.04 }, { k: 'd', deck: 0, t: -0.2, y: 0.32, via: -0.13, viaY: 0.0 }, { k: 'e', deck: 0, t: 0.3, y: 0.0 }, { k: 'f', deck: 1, t: -0.31, y: 0.12, via: -0.13 }],
			foes: [{ deck: 2, t: -0.02, y: 0 }, { deck: 1, t: 0.22, y: 0.12 }, { deck: 0, t: 0.12, y: -0.38 }], chests: [{ deck: 2, t: 0.24, y: 0.36 }, { deck: 0, t: -0.4, y: 0.3 }],
		},
		map: m, scaleAt: [40, 1230], compassAt: [1380, 60],
	};
};
