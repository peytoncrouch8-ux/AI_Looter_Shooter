// Round two of the Gilded Lily concepts: D Fjord Head, E Breakwater Harbor (grounded), F Hulltown, G The Tiers
// (wild). Each page: a cleaner map with sized fight rings, an eye-level arrival sketch, and the liner's played decks at
// true scale. Renders D_..G_ .svg/.png and the phone sheet Lily_Concepts2.png.
//   node Art/Concepts/Areas/GildedLily/gen2.js [D|E|F|G ...] [--no-png] [--no-sheet]
const fs = require('fs');
const path = require('path');
const L = require('./lib');
const K = require('./lib2');
const { C } = L;
const here = __dirname;
const SHORE_M2 = 95000, SHIP_M = 180;
const scale = isPlay => { const px = L.playArea(isPlay); return { px, mpp: Math.sqrt(SHORE_M2 / px) }; };
const tile = (pts, col) => `<path d="${L.d(pts, true)}" fill="${col}" stroke="#2b211b" stroke-width="1.5"/>`;

// ======================================================================= D: Fjord Head (grounded)
function conceptD() {
	const north = L.roughen([[-80, -80], [1380, -80], [1380, 455], [1230, 452], [1160, 440], [1090, 420], [1000, 405], [900, 395], [790, 382], [680, 386], [570, 372], [470, 346], [380, 322], [300, 302], [240, 282], [206, 240], [190, 190], [204, 130], [186, 60], [172, -80]], true, 5, 201, 10);
	const south = L.roughen([[1380, 1100], [1380, 540], [1230, 546], [1160, 562], [1080, 582], [980, 602], [870, 622], [760, 650], [650, 680], [540, 700], [430, 720], [340, 742], [270, 772], [222, 822], [184, 890], [150, 962], [142, 1100]], true, 5, 202, 10);
	const ridgeN = L.roughen([[-80, -80], [1380, -80], [1380, 250], [1250, 300], [1150, 290], [1050, 262], [950, 250], [850, 240], [750, 236], [650, 222], [550, 202], [450, 182], [370, 172], [300, 124], [262, 60], [252, -80]], true, 7, 203, 8);
	const ridgeS = L.roughen([[1380, 1100], [1380, 720], [1250, 702], [1150, 722], [1050, 742], [950, 762], [850, 782], [750, 802], [650, 832], [550, 852], [450, 882], [380, 922], [334, 980], [316, 1100]], true, 7, 204, 8);
	const lands = [north, south], ridges = [ridgeN, ridgeS];
	const isLand = p => L.inAny(p, lands), isPlay = p => isLand(p) && !L.inAny(p, ridges), isWater = p => !isLand(p);
	const { px, mpp } = scale(isPlay), shipPx = SHIP_M / mpp;
	console.log(`D: play ${px}, mpp ${mpp.toFixed(3)}, ship ${shipPx.toFixed(0)} px`);
	const coastD = lands.map(l => L.d(l, true)).join(' ');
	let m = L.waterLayer([coastD]) + L.waves(isWater, 700, 9);
	m += L.flats([[1130, 470], [1240, 460], [1300, 480], [1300, 530], [1230, 540], [1150, 545], [1110, 510]], 205);
	m += L.flats([[40, 860], [140, 860], [190, 900], [160, 980], [130, 1060], [40, 1060]], 206);
	m += L.landLayer('landD', coastD, [L.d(L.roughen([[400, 200], [1300, 200], [1300, 420], [1000, 395], [700, 370], [420, 320]], true, 8, 207, 6), true), L.d(L.roughen([[1300, 570], [1300, 760], [400, 900], [420, 740], [800, 660]], true, 8, 208, 6), true)]);
	m += L.hill([[200, 140], [280, 120], [340, 180], [330, 270], [260, 290], [210, 250]], 209, 3);
	m += L.ridgeLayer(ridges.map(r => L.d(r, true)), [], 'landD');
	m += L.trees(p => L.inAny(p, ridges) && isLand(p), [0, 0, 1300, 1020], 240, 210, { col: '#2b3720', r: 6 });
	// the fjord's walls: cliff bands under both ridges, and the headland's sea face
	m += L.cliff([[1380, 250], [1250, 300], [1150, 290], [1050, 262], [950, 250], [850, 240], [750, 236], [650, 222], [550, 202], [450, 182], [370, 172]], 1, 211, { len: 11, w: 3.5 });
	m += L.cliff([[1380, 720], [1250, 702], [1150, 722], [1050, 742], [950, 762], [850, 782], [750, 802], [650, 832], [550, 852], [450, 882], [380, 922]], -1, 212, { len: 11, w: 3.5 });
	m += L.cliff([[300, 302], [240, 282], [206, 240], [190, 190], [204, 130], [186, 60], [172, -20]], -1, 213, { len: 15 });
	const railPts = [[1380, 372], [1300, 384], [1250, 392]], spur = [[1250, 392], [1120, 372], [980, 352], [860, 338]];
	const nRoad = [[1250, 410], [1100, 398], [950, 376], [800, 356], [680, 360], [560, 344], [440, 312], [330, 280], [250, 268]];
	const sRoad = [[1210, 590], [1120, 600], [1000, 628], [880, 652], [760, 684], [640, 712], [520, 736], [400, 756], [300, 800], [230, 880], [200, 960]];
	m += L.road(nRoad, 8) + L.road(sRoad, 8) + L.rail(railPts) + L.rail(spur);
	m += L.bridge([1210, 444], [1210, 556], 12);
	// the chain ferry across the middle of the fjord
	m += `<path d="M560,372 L560,700" stroke="#2a1d14" stroke-width="2.5" stroke-dasharray="3 3"/>` + `<rect x="540" y="520" width="40" height="30" fill="#7a5636" stroke="#2a1a10" stroke-width="1.5"/>` + L.lamp(560, 535, 1.2);
	const bld = [];
	const t1 = L.town([[1300, 420], [1160, 412], [1060, 396]], { step: 22, off: 13, w: 18, h: 14, gaps: 0.15 }, 214, bld);
	const t2 = L.town([[1300, 612], [1200, 598], [1080, 618], [980, 640]], { step: 22, off: 13, w: 18, h: 14, gaps: 0.2 }, 215, bld);
	m += t1.s + t2.s + L.roof(1270, 372, 60, 16, -6, '#4a3a30') + L.roof(1150, 660, 34, 20, -10, '#e8e0cc', { big: true }) + L.roof(1086, 650, 40, 26, -12, '#3a1418', { big: true });
	for (const x of [760, 840, 920]) m += L.pier([x, 384 + (x - 760) * 0.06], [x - 4, 456], 11, { head: 16 });
	m += L.dryDock(1000, 420, 100, 2);
	for (const [x, y, r] of [[860, 330, 12], [890, 320, 10], [830, 318, 9]]) m += L.coalHeap(x, y, r);
	for (const [x, y, a] of [[760, 392, 90], [840, 396, 90], [920, 402, 90]]) m += L.crane(x, y, a);
	m += L.warehouse(720, 340, 60, 22, -4) + L.warehouse(960, 350, 54, 22, -6);
	m += L.pier([650, 386], [650, 462], 12, { head: 20, col: '#9a7044' });
	m += L.wreck(230, 900, 40, 70) + L.wreck(120, 210, 34, -30) + L.wreck(440, 690, 30, 10);
	m += L.cave(196, 160, 180) + L.cave(184, 80, 180) + L.cave(160, 960, 160);
	m += rocksAt(isWater, [100, 120, 190, 320], 14, 216);
	const okT = p => isPlay(p) && [nRoad, sRoad, spur].every(l => L.lineDist(p, l) > 12) && bld.every(q => L.dist(p, q) > 16);
	m += L.trees(okT, [180, 150, 1300, 900], 110, 217, { r: 6 });
	m += t1.lamps + t2.lamps;
	m += L.lighthouse(232, 268, 175, 560);
	for (const [x, y, a, l, k] of [[880, 500, 0, 30, 2], [910, 506, 0, 30, 2], [700, 560, 180, 34, 0], [1000, 520, 170, 24, 1], [380, 470, 190, 26, 1]]) m += L.boat(x, y, a, l, k);
	m += L.liner(95, 600, shipPx, -90, 21);
	m += L.seaRoute([[650, 470], [500, 520], [330, 560], [200, 590], [120, 600]]) + L.launchBoat(420, 532, 190);
	// fights
	m += K.fight(870, 318, 50, 'crew', 'F1', mpp) + K.fight(200, 940, 35, 'crew', 'F2', mpp) + K.fight(268, 826, 45, 'beast', 'F3', mpp) + K.fight(268, 210, 34, 'beast', 'F4', mpp, { below: true }) + K.fight(560, 535, 26, 'bounty', 'F5', mpp, { below: true });
	// path
	m += L.route([[1250, 392], [1200, 412], [1160, 420]]) + L.route([[1160, 420], [1210, 450], [1210, 560], [1150, 600], [1100, 640]]);
	m += L.route([[1100, 640], [1120, 560], [1210, 500], [1160, 420], [1000, 392], [800, 372], [650, 400], [650, 460]]);
	m += L.route([[1000, 628], [700, 700], [400, 760], [260, 840]], { w: 3, dash: '5 7' }) + L.route([[800, 372], [500, 330], [300, 290]], { w: 3, dash: '5 7' }) + L.route([[560, 372], [560, 700]], { w: 3, dash: '5 7' });
	m += L.step(1262, 346, 1) + L.step(1160, 372, 2) + L.step(1060, 690, 3) + L.step(682, 470, 4) + L.step(140, 650, 5) + L.step(60, 680, 6);
	m += L.giver(1180, 440) + L.giver(1150, 690) + L.giver(1110, 650);
	m += L.chest(196, 152) + L.chest(470, 690) + L.chest(1000, 450) + L.chest(160, 990);
	m += L.secret(170, 70) + L.secret(1320, 500);
	m += K.bigLabel(1150, 230, 'Fjord Head', 'the port: station, hotel, bank, chapel') + K.bigLabel(810, 175, 'Docks & Shipyards', null, { size: 24 });
	m += K.bigLabel(380, 70, 'Sea Cliffs', 'the lighthouse, sea caves', { size: 24 }) + K.bigLabel(450, 990, 'Beaches & Tide Flats', 'wreck camp, smugglers\' cove', { size: 24 });
	m += K.bigLabel(820, 530, 'Kestrel Fjord', null, { size: 26, fill: '#bfeaff' }) + K.bigLabel(95, 360, 'The Lily', null, { size: 22, fill: C.lamp });
	return {
		key: 'D', name: 'Fjord Head', kind: 'GROUNDED', tag: 'TWO SHORES', area: Math.round(px * mpp * mpp), mpp, map: m, scaleAt: [920, 980], compassAt: [1250, 960],
		subtitle: 'A deep fjord: the port at its head where the rail comes down the valley, a shore on each side under the walls, the sea and the Lily at its mouth.',
		plays: ['The water splits the map: north shore and south shore, joined', 'only by the head bridge and a chain ferry. Long rifle fights', 'across the fjord; each shore is its own run out to the sea.'],
		path: ['Train down the valley to Fjord Head', 'Port missions on both shores', 'Win the marker: the Gilded Parlor, south side', 'The launch from the north-shore pier', 'Board by the starboard gangway', 'The ballroom: Ned deals'],
		fights: [['crew', 'F1 coal yard', "Ned's bouncers hold the coal"], ['crew', 'F2 smugglers\' cove', 'sharps moving grave gold'], ['beast', 'F3 crab beach', 'a giant crab den'], ['beast', 'F4 gull cliffs', 'gulls nest round the light'], ['bounty', 'F5 chain ferry', 'a bounty fought on the raft']],
		deck: {
			how: 'the launch puts you at the starboard gangway',
			route: [{ k: 'a', deck: 0, t: 0.0, v: 0.5, say: 'starboard gangway' }, { k: 'b', deck: 0, t: 0.288, v: 0, pre: [[0.24, 0.36]], say: 'fight forward to the bridge' }, { k: 'c', deck: 2, t: 0.375, v: 0, via: 0.37, say: 'cargo hatch down: the vault' }, { k: 'd', deck: 1, t: 0.05, v: 0, via: 0.05, say: 'service stairs up: the casino' }, { k: 'e', deck: 1, t: -0.335, v: 0.12, pre: [[-0.16, 0]], say: 'grand staircase: Ned' }],
			fights: [{ deck: 0, t: 0.13, v: 0.3, d: 14, tag: 'promenade', below: true }, { deck: 2, t: 0.375, v: 0, d: 16, tag: 'vault guard', below: true }, { deck: 1, t: 0.05, v: 0, d: 22, tag: 'casino brawl', below: true }, { deck: 1, t: -0.335, v: 0, d: 22, tag: 'NED', below: true }],
		},
		sketch: sketchD(),
	};
}
function rocksAt(ok, box, n, seed) { return L.rocks(ok, box, n, seed, { r: 3.5 }); }
function sketchD() {
	const { w, h } = K.SK, hz = 200;
	let s = K.skSky(w, h, hz, 335);
	s += K.skLiner(300, hz + 2, 70);
	s += K.skShape([[-10, 40], [120, 70], [230, 150], [300, 196], [300, 230], [-10, 330]], '#2a2038', '#e2774a', 0.6);
	s += K.skShape([[w + 10, 20], [540, 80], [420, 160], [366, 198], [366, 230], [w + 10, 330]], '#2a2038', '#e2774a', 0.6);
	s += K.skShape([[-10, 120], [90, 150], [200, 210], [250, 236], [-10, 300]], '#1d1628', '#c8573a', 0.4);
	s += K.skShape([[w + 10, 110], [560, 150], [440, 214], [400, 240], [w + 10, 300]], '#1d1628', '#c8573a', 0.4);
	s += K.skTown(0, 230, 262, 12, 30, 31, '#160f16', { lit: 0.45 }) + K.skTown(420, w, 270, 12, 34, 32, '#160f16', { lit: 0.45 });
	s += `<rect y="262" width="${w}" height="${h - 262}" fill="#100c14" opacity="0.6"/>`;
	s += K.skPlatform(w, h, { sign: 'FJORD HEAD', lampX: 310, lampX2: 500 });
	return s;
}

// ======================================================================= render
async function render(items, opts) {
	let pw; try { pw = require('playwright'); } catch { pw = require('/opt/node22/lib/node_modules/playwright'); }
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: K.W2, height: K.H2 } });
	for (const it of items) {
		await pg.setContent(`<!doctype html><html><body style="margin:0;background:#06121c">${it.svg}</body></html>`);
		await pg.evaluate(() => document.fonts.ready);
		await pg.screenshot({ path: it.png, clip: { x: 0, y: 0, width: K.W2, height: K.H2 } });
		console.log('png ->', it.png);
	}
	if (opts.sheet) {
		const imgs = Object.values(CONCEPTS).map(c => path.join(here, c[1] + '.png')).filter(f => fs.existsSync(f));
		const cw = 1200, ch = Math.round(cw * K.H2 / K.W2), gap = 28, top = 100;
		const cols = 2, rows = Math.ceil(imgs.length / cols);
		const SW = cols * cw + (cols + 1) * gap, SH = top + rows * (ch + gap);
		const font = f => fs.readFileSync(path.join(here, '../../../../Content/UI/Fonts', f)).toString('base64');
		const html = `<!doctype html><html><head><style>@font-face{font-family:CP;font-weight:700;src:url(data:font/ttf;base64,${font('ChakraPetch-Bold.ttf')})}
body{margin:0;background:#06121c;width:${SW}px;height:${SH}px;font-family:CP,sans-serif}h1{margin:0;position:absolute;left:${gap}px;top:24px;color:#f6e7c1;font-size:44px;letter-spacing:3px}
h1 span{color:#ff9f1c;font-size:32px}img{position:absolute;width:${cw}px;height:${ch}px;border:2px solid #5ac8ff88;box-sizing:border-box;border-radius:6px}</style></head><body>
<h1>THE GILDED LILY <span>· FOUR MORE: D FJORD HEAD · E BREAKWATER HARBOR · F HULLTOWN · G THE TIERS</span></h1>
${imgs.map((f, i) => `<img style="left:${gap + (i % cols) * (cw + gap)}px;top:${top + Math.floor(i / cols) * (ch + gap)}px" src="data:image/png;base64,${fs.readFileSync(f).toString('base64')}">`).join('')}</body></html>`;
		await pg.setViewportSize({ width: SW, height: SH });
		await pg.setContent(html); await pg.evaluate(() => document.fonts.ready);
		const out = path.join(here, 'Lily_Concepts2.png');
		await pg.screenshot({ path: out, clip: { x: 0, y: 0, width: SW, height: SH } });
		console.log('sheet ->', out);
	}
	await browser.close();
}

const EFG = require('./conceptsEFG');
const CONCEPTS = { D: [conceptD, 'D_FjordHead'], E: [EFG.conceptE, 'E_BreakwaterHarbor'], F: [EFG.conceptF, 'F_Hulltown'], G: [EFG.conceptG, 'G_TheTiers'] };
if (require.main === module) {
	const args = process.argv.slice(2), keys = args.filter(a => CONCEPTS[a]);
	const items = (keys.length ? keys : Object.keys(CONCEPTS)).map(k => {
		const [fn, name] = CONCEPTS[k], svg = K.page(fn());
		fs.writeFileSync(path.join(here, name + '.svg'), svg);
		return { svg, png: path.join(here, name + '.png') };
	});
	if (!args.includes('--no-png')) render(items, { sheet: !args.includes('--no-sheet') }).catch(e => { console.error(e); process.exit(1); });
}
