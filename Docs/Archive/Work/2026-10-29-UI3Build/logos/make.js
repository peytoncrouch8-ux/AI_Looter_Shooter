// UI round 3 build, lane D (2026-10-29): five new marks for the top-left of concept 3's main menu (the user's pick),
// each inside c3's halo ring, shown large and in the real lockup (ring + REVENANT + ruler + TEROPA, c3Menu's sizes).
// Mockup only. Run from the repo root:  node Docs/Work/2026-10-29-UI3Build/logos/make.js  ->  logos.png, marks.json
// Every mark is straight-segment polylines and polygons (arcs sampled) so Main can port the points to C++ as they are.
const fs = require('fs');
const path = require('path');
const here = __dirname;
const F = 'file:///C:/Dev/AI_Looter_Shooter/';
const MENU_IMG = F + 'Saved/Screenshots/Tools/main_menu.png';
const C = { line: '#d4eef4', acc: '#ffe6a6', lab: '#b4bec8', text: '#f2f6f8', icon: '#98a4ae', cyan: '#9fe0ff', dark: '#04060a' };

// ------------------------------------------------------------------ geometry helpers (mark space: 40 x 40 round 0,0; y down)
const rad = (a) => a * Math.PI / 180;
const r1 = (v) => Math.round(v * 100) / 100;
const pt = (cx, cy, r, a) => [cx + r * Math.cos(rad(a)), cy + r * Math.sin(rad(a))];
function ell(cx, cy, rx, ry, a0, a1, step = 10) {
	const out = [], n = Math.max(2, Math.ceil(Math.abs(a1 - a0) / step));
	for (let i = 0; i <= n; i++) { const a = a0 + (a1 - a0) * i / n; out.push([cx + rx * Math.cos(rad(a)), cy + ry * Math.sin(rad(a))]); }
	return out;
}
function qb(p0, p1, p2, n = 8) {
	const out = [];
	for (let i = 0; i <= n; i++) { const t = i / n, u = 1 - t; out.push([u * u * p0[0] + 2 * u * t * p1[0] + t * t * p2[0], u * u * p0[1] + 2 * u * t * p1[1] + t * t * p2[1]]); }
	return out;
}
// Joins point lists, dropping a point repeated where two lists meet.
const cat = (...lists) => lists.reduce((a, l) => a.concat(a.length && l.length && a[a.length - 1][0] === l[0][0] && a[a.length - 1][1] === l[0][1] ? l.slice(1) : l), []);
const mirX = (P) => P.map(([x, y]) => [-x, y]);
const S = (pts, w = 3, o = {}) => ({ k: 'stroke', col: o.col || 'line', w, closed: !!o.closed, pts });
const Fi = (pts, o = {}) => ({ k: 'fill', col: o.col || 'acc', pts });

// ------------------------------------------------------------------ the five marks
const MARKS = [];

// 1 · SOUL LANTERN: a lantern's cage in salt, the soul-flame inside it in gold.
{
	const flame = cat(qb([0, -7], [5.5, 0], [4.2, 4.5], 6), qb([4.2, 4.5], [3, 9], [0, 9], 4), qb([0, 9], [-3, 9], [-4.2, 4.5], 4), qb([-4.2, 4.5], [-5.5, 0], [0, -7], 6));
	MARKS.push({
		key: 'SoulLantern', name: 'SOUL LANTERN',
		idea: 'Every revenant carries the light that brought them back: a lantern cage with a soul-flame in gold.',
		parts: [
			S(ell(0, -15.5, 3.2, 3.2, 0, 360, 30), 2.5, { closed: true }),			// hanging loop
			S([[-5, -12], [5, -12], [10, -7.5], [-10, -7.5]], 3, { closed: true }),	// the hood
			S([[-10, -7.5], [-12, 2.5], [-10, 12.5]], 3),					// cage sides, bowed
			S([[10, -7.5], [12, 2.5], [10, 12.5]], 3),
			S([[-13, 12.5], [13, 12.5], [11, 17], [-11, 17]], 3, { closed: true }),	// the base
			Fi(flame.map(([x, y]) => [x, y + 2.5])),					// the soul-flame
		],
	});
}

// 2 · KEYHOLE GRAVE: a headstone with a keyhole of gold light cut through it, on a ground line.
{
	const stone = cat([[-12, 15]], ell(0, -4, 12, 12, 180, 360, 12), [[12, 15]]);
	const hole = cat(ell(0, -1.5, 4.4, 4.4, 122, 418, 12), [[3.8, 9.5], [-3.8, 9.5]]);
	MARKS.push({
		key: 'KeyholeGrave', name: 'KEYHOLE GRAVE',
		idea: 'The grave is a door that does not stay shut: a headstone with a keyhole of light cut through it.',
		parts: [
			S(stone, 3),
			S([[-19, 15], [19, 15]], 3),						// the ground
			S([[-16, 19], [-6, 19]], 2), S([[6, 19], [16, 19]], 2),		// a second, broken ground line
			Fi(hole),
		],
	});
}

// 3 · BROKEN HALO: a halo snapped in two over the ember that came back, its right half tipped down and falling away.
{
	const hc = [0, -12.5], hinge = pt(0, -12.5, 16.5, -80).map((v, i) => i ? hc[1] + (v - hc[1]) * 5.5 / 16.5 : v);
	const tip = (p) => { const a = rad(24), dx = p[0] - hinge[0], dy = p[1] - hinge[1]; return [hinge[0] + dx * Math.cos(a) - dy * Math.sin(a), hinge[1] + dx * Math.sin(a) + dy * Math.cos(a)]; };
	MARKS.push({
		key: 'BrokenHalo', name: 'BROKEN HALO',
		idea: 'Sent back from the dead: the halo snapped in two and one half is falling, but the ember under it kept burning.',
		parts: [
			S(ell(hc[0], hc[1], 16.5, 5.5, 100, 258, 10), 3),				// the halo's left half
			S(ell(hc[0], hc[1], 16.5, 5.5, -80, 78, 10).map(tip), 3),		// its right half, tipped 24° down round the back break
			Fi([[0, -6], [2.8, 4], [7, 8], [2.8, 11.5], [0, 19.5], [-2.8, 11.5], [-7, 8], [-2.8, 4]]),	// the ember: a long four-point spark
		],
	});
}

// 4 · EMBER EYE: an eye held in the sight, an ember diamond for its pupil, the reticle's ticks above and below.
{
	const eye = cat(qb([-18, 0], [0, -17], [18, 0], 12), qb([18, 0], [0, 17], [-18, 0], 12));
	MARKS.push({
		key: 'EmberEye', name: 'EMBER EYE',
		idea: 'The sight that never closes: an eye drawn on the reticle, with an ember burning where the pupil should be.',
		parts: [
			S(eye, 3, { closed: true }),
			S(ell(0, 0, 7, 7, 0, 360, 15), 2.5, { closed: true }),		// the iris ring
			Fi([[0, -5], [4, 0], [0, 5], [-4, 0]]),			// the ember pupil
			S([[0, -19], [0, -12.5]], 3), S([[0, 12.5], [0, 19]], 3),	// reticle ticks
		],
	});
}

// 5 · THE STRANGER: a drifter's wide-brim hat, salt solid, over a skull's outline with two gold ember eyes.
{
	const crown = [[-8, -7], [-7.2, -14.5], [-2.5, -13], [0, -15.5], [2.5, -13], [7.2, -14.5], [8, -7]];
	const brim = cat(qb([-19.5, -10.5], [-15, -5.5], [0, -5.5], 6), qb([0, -5.5], [15, -5.5], [19.5, -10.5], 6),
		qb([19.5, -10.5], [16, -2.2], [0, -2.2], 6), qb([0, -2.2], [-16, -2.2], [-19.5, -10.5], 6));
	const jaw = [[-9.5, -2], [-9.5, 6], [-5.5, 11], [-5.5, 17], [5.5, 17], [5.5, 11], [9.5, 6], [9.5, -2]];
	const eyeL = [[-8, 1], [-2, 2], [-2.6, 5.4], [-7, 4.6]];
	MARKS.push({
		key: 'Stranger', name: 'THE STRANGER',
		idea: 'Who walks back into town: a drifter\'s hat over a face the grave gave back, two embers for eyes.',
		parts: [
			Fi(crown, { col: 'line' }), Fi(brim, { col: 'line' }),
			S(jaw, 3),
			Fi(eyeL), Fi(mirX(eyeL)),
			Fi([[0, 6.5], [1.6, 9.5], [-1.6, 9.5]], { col: 'line' }),	// the nose
			S([[-2, 12.5], [-2, 17]], 2), S([[2, 12.5], [2, 17]], 2),	// teeth
		],
	});
}

// Every mark 1.12x bigger than drawn above (fills the ring better at 68 px); line widths stay.
MARKS.forEach(m => m.parts.forEach(p => { p.pts = p.pts.map(([x, y]) => [x * 1.12, y * 1.12]); }));

// ------------------------------------------------------------------ SVG pieces (c3Menu's own helpers, cut down)
const pd = (pts, closed) => 'M' + pts.map(q => r1(q[0]) + ' ' + r1(q[1])).join(' L') + (closed ? ' Z' : '');
function markSvg(m) {
	return m.parts.map(p => p.k === 'fill'
		? '<path d="' + pd(p.pts, true) + '" fill="' + C[p.col] + '"/>'
		: '<path d="' + pd(p.pts, p.closed) + '" fill="none" stroke="' + C[p.col] + '" stroke-width="' + p.w + '" stroke-linejoin="miter" stroke-miterlimit="6" stroke-linecap="butt"/>').join('');
}
function T(x, y, s, o = {}) {
	return '<text x="' + x + '" y="' + y + '" dominant-baseline="central" text-anchor="' + (o.a ?? 'start') + '" style="font-family:Chakra;font-weight:' + (o.w ?? 700) +
		';font-size:' + (o.fs ?? 20) + 'px;letter-spacing:' + (o.ls ?? 0) + 'px" fill="' + (o.fill ?? C.text) + '"' +
		(o.out ? ' stroke="#04060a" stroke-opacity=".85" stroke-width="' + o.out + '" stroke-linejoin="round" paint-order="stroke"' : '') + '>' + s + '</text>';
}
const Ln = (x1, y1, x2, y2, col, sw, so) => '<line x1="' + x1 + '" y1="' + y1 + '" x2="' + x2 + '" y2="' + y2 + '" stroke="' + col + '" stroke-width="' + sw + '" stroke-opacity="' + so + '"/>';
function ticks(cx, cy, ra, rb, a0, a1, step, o = {}) {
	let s = '';
	for (let a = a0; a <= a1 + 1e-6; a += step) {
		const big = o.major && Math.abs(((a - a0) % o.major)) < 1e-6;
		const [x1, y1] = pt(cx, cy, ra, a), [x2, y2] = pt(cx, cy, big ? rb + (o.majorLen ?? 8) : rb, a);
		s += Ln(x1.toFixed(1), y1.toFixed(1), x2.toFixed(1), y2.toFixed(1), C.line, big ? 2.4 : 1.4, big ? 1 : (o.op ?? .6));
	}
	return s;
}
function ruler(x1, x2, y, step, major, so) {
	let s = Ln(x1, y, x2, y, C.line, 1.6, so);
	for (let x = x1, i = 0; x <= x2 + .1; x += step, i++) { const big = i % major === 0; s += Ln(x, y, x, y + (big ? 12 : 5), C.line, big ? 2 : 1.2, big ? .85 : .45); }
	return s;
}
// c3's halo ring round (cx, cy) at 1x, the mark in it.
const ring = (cx, cy, m) => '<circle cx="' + cx + '" cy="' + cy + '" r="34" fill="rgb(18,21,26)" fill-opacity=".8" stroke="' + C.line + '" stroke-width="2.5"/>' +
	ticks(cx, cy, 38, 44, 0, 350, 10, { major: 90, majorLen: 7, op: .55 }) + '<g transform="translate(' + cx + ' ' + cy + ')">' + markSvg(m) + '</g>';
// The lockup exactly as c3Menu draws it, in the menu frame's own coordinates.
const lockup = (m) => ring(58, 60, m) + T(116, 54, 'REVENANT', { fs: 48, ls: 13, fill: '#eef8fb', out: 3 }) + ruler(118, 430, 86, 8, 5, .6) + T(120, 110, 'TEROPA', { fs: 14, ls: 8, fill: C.cyan });

const at = (x, y, inner, css = '') => '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;' + css + '">' + inner + '</div>';
const box = (x, y, w, h, inner, css = '') => '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;width:' + w + 'px;height:' + h + 'px;' + css + '">' + inner + '</div>';
const svg = (w, h, inner) => '<svg width="' + w + '" height="' + h + '" style="position:absolute;left:0;top:0;overflow:visible">' + inner + '</svg>';
// The c3 menu frame's background and shade, cropped to w x h from (ox, oy).
const MENU_BG = 'background:#04060a url(\'' + MENU_IMG + '\') -370px -160px/1683px auto';
const menuShade = '<div style="position:absolute;inset:0;background:linear-gradient(90deg,rgba(4,6,9,.9) 0,rgba(4,6,9,.6) 42%,rgba(4,6,9,0) 70%)"></div>';
const menuCrop = (x, y, w, h, inner, ox = 0, oy = 0) => box(x, y, w, h, box(-ox, -oy, 752, 420, menuShade, MENU_BG) + inner,
	'overflow:hidden;border-radius:5px;box-shadow:0 0 0 1px rgba(212,238,244,.16)');
const chip = (t) => '<span style="padding:4px 9px;border-radius:4px;background:rgba(0,0,0,.66);box-shadow:inset 0 0 0 1px rgba(255,255,255,.22);font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:3px;color:#fff">' + t + '</span>';

// ------------------------------------------------------------------ the sheet
const TW = 608, TH = 472, GX = 32, GY = 98, G = 16;
function tile(i, m) {
	const big = 2.5, cx = 462, cy = 152;
	let h = '';
	// Number, name, the idea.
	h += box(20, 20, 46, 46, '', 'box-sizing:border-box;border:3px solid ' + C.acc + ';border-radius:7px') +
		at(20, 20, String(i + 1), 'width:46px;line-height:46px;text-align:center;font-family:Chakra;font-weight:700;font-size:26px;color:' + C.acc);
	h += at(82, 24, m.name, 'white-space:nowrap;font-family:Chakra;font-weight:700;font-size:27px;letter-spacing:4px;color:' + C.line);
	h += at(22, 92, m.idea, 'width:276px;font-family:Chakra;font-size:19px;line-height:1.4;color:' + C.text);
	// The mark large: c3's ring at 2.5x on a dark disc, as on the menu.
	h += box(cx - 132, cy - 132, 264, 264, '', 'border-radius:50%;background:radial-gradient(circle,rgba(4,6,9,.75) 55%,rgba(4,6,9,0) 72%)');
	h += svg(TW, TH, '<g transform="translate(' + cx + ' ' + cy + ') scale(' + big + ') translate(-58 -60)">' + ring(58, 60, m) + '</g>');
	// The real lockup, real size, on the menu's own background.
	h += menuCrop(20, 314, 568, 138, svg(752, 420, lockup(m))) + at(578, 422, chip('REAL SIZE'), 'white-space:nowrap;transform:translateX(-100%)');
	return h;
}
function sideBySide() {
	let h = at(22, 22, 'ALL FIVE · SIDE BY SIDE', 'white-space:nowrap;font-family:Chakra;font-weight:700;font-size:22px;letter-spacing:4px;color:' + C.line);
	h += at(22, 58, 'In the menu at real size (the ring 68 px), then at half size, as a save slot or loading icon would use it.', 'width:560px;font-family:Chakra;font-size:17px;line-height:1.4;color:' + C.lab);
	let s = '', s2 = '';
	MARKS.forEach((m, i) => {
		const x = 64 + i * 110;
		s += ring(x, 64, m) + T(x, 132, String(i + 1), { fs: 18, a: 'middle', fill: C.acc });
		s2 += '<g transform="translate(' + x + ' 52) scale(.5) translate(' + -x + ' -52)">' + ring(x, 52, m) + '</g>';
	});
	h += menuCrop(20, 124, 568, 152, svg(568, 152, s), 30, 120);
	h += menuCrop(20, 290, 568, 104, svg(568, 104, s2), 30, 220);
	h += at(22, 408, 'Salt lines and gold accents only, 3 px lines at real size: no gradients, no words in the mark.', 'width:560px;font-family:Chakra;font-size:17px;line-height:1.4;color:' + C.lab);
	return h;
}
function sheet() {
	let h = '<div style="position:relative;width:1920px;height:1080px;overflow:hidden;background:radial-gradient(ellipse at 50% 10%,#1a1d22,#07080a 72%)">';
	h += box(32, 20, 8, 58, '', 'background:' + C.acc);
	h += at(58, 14, 'NEW LOGO · PICK ONE', 'white-space:nowrap;font-family:Chakra;font-weight:700;font-size:40px;letter-spacing:6px;color:' + C.line);
	h += at(60, 64, 'Five new marks for the main menu\'s halo ring (concept 3). Each: the mark large, then in the real lockup at its real size.', 'white-space:nowrap;font-family:Chakra;font-size:20px;color:' + C.lab);
	h += at(1888, 30, 'UI ROUND 3 · MAIN MENU MARK', 'white-space:nowrap;transform:translateX(-100%);font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:4px;color:' + C.icon);
	const cells = MARKS.map((m, i) => tile(i, m)).concat([sideBySide()]);
	cells.forEach((inner, k) => {
		const x = GX + (k % 3) * (TW + G), y = GY + Math.floor(k / 3) * (TH + G);
		h += box(x, y, TW, TH, inner, 'border-radius:8px;background:linear-gradient(180deg,rgba(30,34,40,.9),rgba(18,21,26,.9));box-shadow:inset 0 0 0 1px rgba(212,238,244,.14)');
	});
	return h + '</div>';
}

// ------------------------------------------------------------------ marks.json: the points for the C++ port
function writeJson() {
	const out = {
		about: 'Lane D logo marks for the c3 main menu. Mark space: about 44x44 centred on 0,0, y down, 1 unit = 1 px at real size; ' +
			'drawn at the ring centre (58,60 in the c3 menu frame), ring r 34. Strokes: miter joins, butt caps, width in px. ' +
			'Colours: line = #d4eef4 (salt), acc = #ffe6a6 (gold). Fills are simple polygons, each star-shaped round its own centre.',
		marks: MARKS.map((m, i) => ({
			num: i + 1, key: m.key, name: m.name, idea: m.idea,
			parts: m.parts.map(p => Object.assign({ k: p.k, col: p.col }, p.k === 'stroke' ? { w: p.w, closed: p.closed } : {},
				{ d: pd(p.pts, p.k === 'fill' || p.closed), pts: p.pts.map(q => [r1(q[0]), r1(q[1])]) })),
		})),
	};
	fs.writeFileSync(path.join(here, 'marks.json'), JSON.stringify(out, null, 1));
}

const css = '@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Regular.ttf\');font-weight:400}' +
	'@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Bold.ttf\');font-weight:700}body{margin:0;background:#07080a}';
(async () => {
	writeJson();
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	const file = path.join(here, 'logos.html');
	fs.writeFileSync(file, '<!doctype html><html><head><meta charset="utf-8"><style>' + css + '</style></head><body>' + sheet() + '</body></html>');
	await pg.goto('file:///' + file.split(path.sep).join('/'));
	await pg.evaluate(() => document.fonts.ready);
	await pg.waitForTimeout(700);
	await pg.screenshot({ path: path.join(here, 'logos.png') });
	await browser.close();
	console.log('logos.png, marks.json');
})();
