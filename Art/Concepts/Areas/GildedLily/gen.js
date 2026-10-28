// Builds the three Gilded Lily top-down concept maps (A Crescent Bay, B The Long Spit, C Estuary Delta) as SVG,
// renders each to a 2048 px PNG in headless Chromium (Playwright from the repo's node_modules) and lays all three on
// one sheet for a phone: Lily_Concepts.png.
//   node Art/Concepts/Areas/GildedLily/gen.js [A|B|C ...] [--no-png] [--no-sheet]
// Shore size is the working number (Docs/Areas/GildedLily.md): ~95,000 m2 of playable shore, so each map's scale bar
// is worked out from its own playable land (land minus the closing hills and channels).
const fs = require('fs');
const path = require('path');
const L = require('./lib');
const { page } = require('./frame');
const { C, MAP } = L;
const here = __dirname;
const SHORE_M2 = 95000, SHIP_M = 180;

// Common finishing: scale from area, then the map's layers in paint order.
function scaleFor(isPlay) { const px = L.playArea(isPlay); return { px, mpp: Math.sqrt(SHORE_M2 / px) }; }
function okTree(isPlay, avoidLines, avoidPts, clear = 14) {
	return p => isPlay(p) && avoidLines.every(l => L.lineDist(p, l) > clear) && avoidPts.every(q => L.dist(p, q) > clear + 4);
}
const deckLinesA = [['a', 'side gangway onto the promenade'], ['b', 'take the bridge'], ['c', 'cargo hatch down: the vault'], ['d', 'service stairs up: the casino'], ['e', 'grand staircase: the ballroom']];

// ======================================================================= A
function conceptA() {
	const landRaw = [[1560, -80], [610, -80], [585, 0], [560, 46], [522, 72], [494, 64], [480, 96], [455, 140], [420, 170], [388, 196], [374, 232], [388, 262], [352, 276], [322, 300], [300, 338], [302, 380], [326, 410], [372, 432], [440, 446], [520, 444], [600, 420], [690, 404], [780, 418], [850, 462], [900, 530], [926, 610], [928, 690], [905, 770], [860, 836], [790, 880], [700, 904], [600, 908], [510, 896], [440, 878], [380, 872], [330, 890], [298, 936], [282, 1000], [300, 1046], [270, 1066], [282, 1100], [322, 1112], [370, 1150], [420, 1186], [470, 1230], [500, 1300], [520, 1360], [1560, 1360]];
	const coast = L.roughen(landRaw, true, 5, 11, 10), coastD = L.d(coast, true);
	const rocksOff = [[[236, 150], [262, 140], [276, 160], [256, 176], [234, 170]], [[200, 300], [222, 292], [232, 310], [210, 322]], [[226, 1180], [248, 1172], [258, 1190], [238, 1200]]].map((r, i) => L.roughen(r, true, 2, 40 + i, 6));
	const ridgeN = L.roughen([[1560, -80], [900, -80], [880, 40], [930, 150], [1000, 260], [1050, 380], [1074, 500], [1100, 592], [1560, 592]], true, 6, 12, 8);
	const ridgeS = L.roughen([[1560, 672], [1100, 672], [1074, 760], [1080, 860], [1020, 960], [940, 1060], [860, 1160], [820, 1260], [800, 1360], [1560, 1360]], true, 6, 13, 8);
	const ridges = [ridgeN, ridgeS];
	const isLand = p => L.pip(p, coast) || L.inAny(p, rocksOff);
	const isPlay = p => L.pip(p, coast) && !L.inAny(p, ridges);
	const isWater = p => !isLand(p);
	const { px, mpp } = scaleFor(isPlay);
	const shipPx = SHIP_M / mpp;
	console.log(`A: play ${px} px2, mpp ${mpp.toFixed(3)}, ship ${shipPx.toFixed(0)} px`);

	// roads and rails
	const frontSt = [[862, 420], [935, 480], [985, 560], [1006, 640], [996, 724], [958, 804], [900, 862]];
	const stationSt = [[1080, 640], [1006, 640], [935, 642]];
	const quayRd = [[862, 420], [780, 380], [690, 372], [600, 384], [520, 404], [460, 410]];
	const lighthouseRd = [[600, 384], [560, 300], [500, 240], [420, 300], [345, 352]];
	const southRd = [[900, 862], [820, 940], [700, 980], [560, 1000], [440, 1020], [360, 1060]];
	const chapelRd = [[985, 560], [1010, 510], [1016, 482]];
	const roads = [frontSt, stationSt, quayRd, southRd, chapelRd];
	const mainRail = [[1480, 631], [1300, 631], [1160, 632], [1100, 634]];
	const spur = [[1100, 618], [1090, 540], [1050, 440], [980, 372], [880, 340], [790, 330], [720, 326]];
	const trails = [[[700, 980], [700, 1100], [620, 1180], [520, 1230]], [[440, 1020], [380, 960], [340, 930]], [[500, 240], [520, 140], [520, 90]], [[1016, 482], [960, 360], [900, 260], [820, 200]]];

	const bld = [];
	let m = L.waterLayer([coastD, ...rocksOff.map(r => L.d(r, true))]);
	m += L.waves(isWater, 900, 3);
	m += L.flats([[150, 960], [280, 930], [292, 1046], [262, 1072], [290, 1120], [360, 1170], [420, 1250], [430, 1300], [200, 1300], [110, 1140]], 21);
	m += L.flats([[430, 852], [560, 842], [700, 852], [760, 880], [640, 900], [500, 890]], 22);
	const upland = [L.d(L.roughen([[640, -60], [980, -60], [980, 300], [860, 300], [700, 296], [620, 250], [590, 150], [610, 60]], true, 8, 5, 6), true),
		L.d(L.roughen([[900, 400], [1160, 400], [1160, 900], [890, 900], [950, 700], [960, 560]], true, 8, 6, 6), true),
		L.d(L.roughen([[480, 960], [900, 900], [980, 1080], [940, 1330], [560, 1330], [490, 1180], [430, 1040]], true, 10, 7, 6), true)];
	m += L.landLayer('landA', coastD, upland);
	m += rocksOff.map(r => `<path d="${L.d(r, true)}" fill="#6e5c4c" stroke="#2b211b" stroke-width="1.5"/>`).join('');
	m += L.hill([[560, 20], [620, 90], [600, 200], [540, 280], [440, 320], [350, 330], [380, 250], [440, 170], [500, 96]], 31, 3);
	m += L.hill([[620, 1060], [700, 1040], [760, 1090], [740, 1160], [660, 1180], [600, 1130]], 32, 2, '#8d7b52');
	m += L.ridgeLayer(ridges.map(r => L.d(r, true)));
	// cliffs: the north horn's sea face, and the bay side of the headland
	m += L.cliff([[585, 0], [560, 46], [522, 72], [494, 64], [480, 96], [455, 140], [420, 170], [388, 196], [374, 232], [388, 262], [352, 276], [322, 300], [300, 338], [302, 380]], -1, 51, { len: 14 });
	m += L.cliff([[326, 410], [372, 432], [430, 440]], -1, 52, { len: 10 });
	m += L.cliff([[880, 40], [930, 150], [1000, 260], [1050, 380], [1074, 500], [1100, 592]], 1, 53, { len: 10, w: 3 });
	m += L.cliff([[1100, 672], [1074, 760], [1080, 860], [1020, 960], [940, 1060], [860, 1160], [820, 1260]], 1, 54, { len: 10, w: 3 });
	for (const t of trails) m += L.trail(t);
	for (const r of roads) m += L.road(r, r === frontSt ? 11 : 8);
	m += L.road(lighthouseRd, 6, { col: '#bfa27a' });
	m += L.rail(mainRail) + L.rail(spur);
	// docks & shipyards on the north shore of the bay
	m += L.dryDock(520, 418, 120, -8);
	for (const x of [600, 690, 780]) m += L.pier([x, 412 + (x - 600) * 0.03], [x - 6, 520], 11, { head: 16 });
	m += L.pier([850, 470], [790, 560], 9);
	for (const [x, y, a] of [[640, 402, 90], [730, 398, 90], [820, 410, 80], [470, 436, 60]]) m += L.crane(x, y, a);
	for (const [x, y, w, h, a] of [[640, 350, 70, 26, -4], [725, 352, 66, 24, -2], [560, 350, 50, 22, -8], [920, 452, 40, 22, 40]]) { m += L.warehouse(x, y, w, h, a); bld.push([x, y]); }
	for (const [x, y, r] of [[800, 352, 13], [830, 342, 11], [860, 358, 12], [775, 335, 9]]) m += L.coalHeap(x, y, r);
	m += L.pier([760, 420], [760, 470], 14, { col: '#3a3634', lamps: false });
	m += L.roof(905, 470, 30, 18, 40, '#3e5a5a');
	// the town on the curve
	const t1 = L.town(frontSt, { step: 24, off: 16, w: 20, h: 16, skip: p => !isPlay(p) }, 61, bld);
	const t2 = L.town([[1006, 640], [1080, 640]], { step: 22, off: 14, w: 16, h: 14, sides: [1] }, 62, bld);
	const t3 = L.town(southRd.slice(0, 3), { step: 26, off: 14, w: 16, h: 13, gaps: 0.4 }, 63, bld);
	m += t1.s + t2.s + t3.s;
	m += L.roof(1016, 468, 34, 20, -60, '#e8e0cc', { big: true }); // Saint Hester's chapel
	m += L.roof(958, 735, 44, 30, 70, C.velvet, { big: true }); // hotel
	m += L.roof(1046, 700, 40, 26, 85, '#3a1418', { big: true }); // the Gilded Parlor
	m += L.roof(1040, 576, 30, 22, 85, '#6a6a70', { big: true, trim: '#cfd6dd' }); // bank
	m += L.pier([928, 650], [826, 646], 14, { head: 22, col: '#9a7044' }); // the Lily's pier
	m += `<rect x="922" y="632" width="8" height="36" fill="${C.gold}" stroke="#3a2a10"/>`;
	m += L.roof(1100, 650, 50, 16, 0, '#4a3a30'); // station platform roof
	// south horn: wreck camp, smugglers' cove
	m += L.wreck(345, 1100, 46, 30) + L.wreck(240, 120, 38, -40) + L.wreck(330, 960, 30, 70);
	for (const [x, y] of [[372, 1070], [390, 1092], [360, 1120]]) m += `<circle cx="${x}" cy="${y}" r="5" fill="#d9c3a0" stroke="#3a2a1a"/>`;
	m += `<circle cx="378" cy="1094" r="10" fill="url(#lampGlow)"/>`;
	m += L.cave(498, 84, 30) + L.cave(386, 250, 60) + L.cave(292, 1076, 90);
	// trees and rocks
	const avoid = [...roads, lighthouseRd, mainRail, spur];
	const okT = okTree(isPlay, avoid, bld, 13);
	m += L.trees(p => okT(p) && !L.pip(p, [[560, 20], [620, 90], [600, 200], [540, 280], [440, 320], [350, 330], [380, 250], [440, 170], [500, 96]]), [600, 0, 1000, 330], 70, 71);
	m += L.trees(okT, [1000, 380, 1100, 900], 16, 72);
	m += L.trees(p => L.inAny(p, ridges), [800, 0, 1440, 1264], 160, 77, { col: '#2e3922', r: 5 });
	m += L.trees(p => okT(p) && L.dist(p, [700, 1110]) > 60, [460, 900, 980, 1300], 90, 73);
	m += L.rocks(p => isPlay(p), [330, 40, 600, 330], 40, 74);
	m += L.rocks(p => isPlay(p) && L.dist(p, [330, 960]) < 60, [280, 900, 400, 1020], 14, 75);
	m += L.rocks(isWater, [200, 100, 330, 360], 18, 76, { r: 3 });
	m += t1.lamps + t2.lamps + t3.lamps;
	for (const p of L.walk(L.spline(quayRd, false, 6), 40)) m += L.lamp(p.p[0], p.p[1] + 10, 0.8);
	m += L.lighthouse(332, 352, 172, 520);

	for (const [x, y, a2, l, k] of [[700, 560, 80, 30, 2], [740, 556, 90, 30, 2], [560, 600, 20, 40, 0], [470, 720, -10, 28, 1], [760, 760, 30, 26, 1], [640, 790, -20, 24, 1], [520, 520, 60, 30, 0]]) m += L.boat(x, y, a2, l, k);
	for (const [x, y] of [[300, 600], [300, 690], [420, 640]]) m += L.buoy(x, y);
	// the liner beyond the harbor mouth, and the launch route
	const ship = [150, 640];
	m += L.liner(ship[0], ship[1], shipPx, -84, 7);
	m += L.anchorIcon(ship[0] + 6, ship[1] - shipPx / 2 - 22);
	m += L.seaRoute([[826, 646], [640, 660], [450, 650], [300, 640], [182, 650]]);
	m += L.launchBoat(560, 656, 180);

	// enemy areas
	m += L.zone(815, 345, 70, 40, 'crew') + L.foeIcon(815, 300, 'crew');
	m += L.zone(860, 645, 48, 32, 'crew') + L.foeIcon(870, 612, 'crew');
	m += L.zone(300, 1078, 46, 36, 'crew') + L.foeIcon(256, 1046, 'crew');
	m += L.zone(462, 176, 78, 50, 'beast', { rot: -30 }) + L.foeIcon(470, 176, 'gull');
	m += L.zone(410, 1200, 70, 40, 'beast', { rot: 40 }) + L.foeIcon(430, 1205, 'crab');
	m += L.zone(200, 1050, 60, 80, 'beast') + L.foeIcon(196, 1052, 'eel');
	m += L.zone(560, 878, 70, 22, 'beast') + L.foeIcon(600, 876, 'crab');

	// the player's path: station -> town and the four districts -> marker -> pier
	m += L.route([[1100, 634], [1040, 650], [960, 620], [940, 600]]);
	m += L.route([[940, 600], [860, 430], [700, 360], [560, 300], [380, 340]], { w: 3, dash: '5 7' });
	m += L.route([[940, 600], [900, 860], [700, 980], [420, 1060]], { w: 3, dash: '5 7' });
	m += L.route([[960, 690], [1020, 705]], { w: 3, dash: '5 7' });
	m += L.route([[1030, 690], [980, 660], [880, 650], [840, 648]]);
	m += L.step(1104, 600, 1) + L.step(952, 586, 2) + L.step(1046, 736, 3) + L.step(846, 676, 4) + L.step(206, 676, 5) + L.step(150, 772, 6);
	// mission givers, bounties, loot, secrets
	m += L.giver(918, 596) + L.giver(1040, 482) + L.giver(1066, 716);
	m += L.giver(360, 372, 'side') + L.giver(540, 456, 'side') + L.giver(392, 1060, 'side') + L.giver(880, 444, 'side');
	m += L.bounty(1176, 560);
	m += L.chest(498, 108) + L.chest(386, 284) + L.chest(348, 1130) + L.chest(780, 300) + L.chest(248, 128) + L.chest(700, 1120) + L.chest(150, 520);
	m += L.secret(300, 1100) + L.secret(900, 250) + L.secret(232, 304);
	m += L.slots(176, 600);
	m += L.station(1112, 666);
	// names
	m += L.district(1250, 300, 'Saltgrass Hills', 'out of bounds', { size: 18, fill: '#cdb89a' }) + L.district(1220, 1000, 'Saltgrass Hills', 'out of bounds', { size: 18, fill: '#cdb89a' });
	m += L.district(860, 1010, 'The Port', 'station · saloons · hotel · bank · chapel', { size: 26 });
	m += L.district(700, 270, 'Docks & Shipyards', 'cranes · dry dock · coal bunkers', { size: 22 });
	m += L.district(470, 32, 'Sea Cliffs', 'coves · wrecks · sea caves', { size: 22 });
	m += L.district(630, 1220, 'Beaches & Tide Flats', 'wreck camp · smugglers\' cove · tide pools', { size: 22 });
	m += L.district(310, 560, 'The Harbor Mouth', null, { size: 18, fill: '#bfeaff' });
	m += L.district(640, 610, 'Crescent Bay', 'sheltered water', { size: 24, fill: '#bfeaff' });
	m += L.chip(1016, 436, "Saint Hester's Chapel") + L.chip(990, 772, 'Marquee Hotel', { anchor: 'start' }) + L.chip(1072, 734, 'the Gilded Parlor', { anchor: 'start', col: C.lamp });
	m += L.chip(1022, 556, 'Bank', { anchor: 'end' }) + L.chip(892, 572, 'Harbor Master', { anchor: 'end' }) + L.chip(820, 700, "the Lily's pier", { col: C.lamp });
	m += L.chip(520, 470, 'dry dock') + L.chip(840, 395, 'coal bunkers') + L.chip(332, 402, 'lighthouse', { anchor: 'end' });
	m += L.chip(400, 1124, 'wreck camp', { anchor: 'start' }) + L.chip(250, 1135, "smugglers' cove", { anchor: 'end' }) + L.chip(150, 990, 'tide pools');
	m += L.chip(1140, 700, 'station', { anchor: 'start' }) + L.chip(1196, 560, 'bounties', { anchor: 'start', col: C.line }) + L.chip(1290, 600, 'rail from Ransom\'s Rest');
	m += L.district(170, 900, 'The Gilded Lily', 'anchored · lit all night', { size: 20, fill: C.lamp });
	return {
		key: 'A', name: 'Crescent Bay', tag: 'OPEN HUB', area: Math.round(px * mpp * mpp), mpp, shipM: SHIP_M,
		subtitle: 'A horseshoe harbor: the town on the curve, cliffs and the lighthouse on the north horn, tide flats on the south; the liner beyond the mouth.',
		plays: ['Open: the town is the hub and every district is a short walk', 'from Front Street; take them in any order. The bay is always', 'in view, the Lily lit up across the mouth: a promise all day.', 'Water at the centre makes long sightlines for gunfights.'],
		path: [['Arrive by train', 'the station at the town\'s east end'], ['Port missions', 'harbor master, chapel; out to docks, cliffs, beaches'], ['Win a gilded marker', 'Ned\'s rigged table in the Gilded Parlor'], ['The launch', 'from the Lily\'s pier, across the bay'], ['Board', 'the side gangway, under the lights'], ['The ballroom', 'Ned deals the last hand']],
		deck: { lines: deckLinesA, route: [{ k: 'a', deck: 0, t: -0.08, y: 0.48 }, { k: 'b', deck: 0, t: 0.3, y: 0.0 }, { k: 'c', deck: 2, t: 0.39, y: 0.0, via: 0.36 }, { k: 'd', deck: 1, t: 0.1, y: 0.0, via: 0.04 }, { k: 'e', deck: 1, t: -0.31, y: 0.12 }], foes: [{ deck: 0, t: 0.12, y: -0.38 }, { deck: 2, t: 0.0, y: 0 }, { deck: 1, t: 0.2, y: 0.12 }], chests: [{ deck: 0, t: 0.42, y: 0.0 }, { deck: 2, t: -0.36, y: 0.27 }] },
		map: m, scaleAt: [40, 1230], compassAt: [1380, 60],
	};
}

// ======================================================================= render
async function render(items, opts) {
	let pw; try { pw = require('playwright'); } catch { pw = require('/opt/node22/lib/node_modules/playwright'); }
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: L.W, height: L.H } });
	for (const it of items) {
		await pg.setContent(`<!doctype html><html><body style="margin:0;background:#06121c">${it.svg}</body></html>`);
		await pg.evaluate(() => document.fonts.ready);
		await pg.screenshot({ path: it.png, clip: { x: 0, y: 0, width: L.W, height: L.H } });
		console.log('png ->', it.png);
	}
	if (opts.sheet) {
		const imgs = ['A_CrescentBay', 'B_LongSpit', 'C_EstuaryDelta'].map(n => path.join(here, n + '.png')).filter(f => fs.existsSync(f));
		const cw = 1400, ch = Math.round(cw * L.H / L.W), gap = 28, top = 96;
		const SW = imgs.length * cw + (imgs.length + 1) * gap, SH = top + ch + gap;
		const font = f => fs.readFileSync(path.join(here, '../../../../Content/UI/Fonts', f)).toString('base64');
		const html = `<!doctype html><html><head><style>@font-face{font-family:CP;font-weight:700;src:url(data:font/ttf;base64,${font('ChakraPetch-Bold.ttf')})}
body{margin:0;background:#06121c;width:${SW}px;height:${SH}px;font-family:CP,sans-serif}h1{margin:0;position:absolute;left:${gap}px;top:26px;color:#f6e7c1;font-size:46px;letter-spacing:4px}
h1 span{color:#ff9f1c}img{position:absolute;top:${top}px;width:${cw}px;height:${ch}px;border:2px solid #5ac8ff88;box-sizing:border-box;border-radius:6px}</style></head><body>
<h1>THE GILDED LILY <span>· THREE TOP-DOWN CONCEPTS: A CRESCENT BAY (OPEN) · B THE LONG SPIT (LINEAR) · C ESTUARY DELTA (MAZE)</span></h1>
${imgs.map((f, i) => `<img style="left:${gap + i * (cw + gap)}px" src="data:image/png;base64,${fs.readFileSync(f).toString('base64')}">`).join('')}</body></html>`;
		await pg.setViewportSize({ width: SW, height: SH });
		await pg.setContent(html); await pg.evaluate(() => document.fonts.ready);
		const out = path.join(here, 'Lily_Concepts.png');
		await pg.screenshot({ path: out, clip: { x: 0, y: 0, width: SW, height: SH } });
		console.log('sheet ->', out);
	}
	await browser.close();
}

const CONCEPTS = { A: [conceptA, 'A_CrescentBay'], B: [() => require('./conceptB')(SHORE_M2, SHIP_M), 'B_LongSpit'], C: [() => require('./conceptC')(SHORE_M2, SHIP_M), 'C_EstuaryDelta'] };
if (require.main === module) {
	const args = process.argv.slice(2), keys = args.filter(a => CONCEPTS[a]);
	const pick = keys.length ? keys : Object.keys(CONCEPTS);
	const items = pick.map(k => {
		const [fn, name] = CONCEPTS[k], cn = fn(), svg = page(cn);
		fs.writeFileSync(path.join(here, name + '.svg'), svg);
		return { svg, png: path.join(here, name + '.png') };
	});
	if (!args.includes('--no-png')) render(items, { sheet: !args.includes('--no-sheet') }).catch(e => { console.error(e); process.exit(1); });
}
module.exports = { CONCEPTS };
