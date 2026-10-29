// UI round 3 build, lane D, logo round 2 (2026-10-29): the user asked for more options after logos.png. Eight new marks
// (numbered 6-13, after round 1's five) in a wider range: solid salt cuts with gold, monograms, one that breaks the ring.
// Same presentation: c3's halo ring large, then c3Menu's lockup at real size. Mockup only. Run from the repo root:
//   node Docs/Work/2026-10-29-UI3Build/logos/make2.js  ->  logos2.png, marks2.json (same format as marks.json)
const fs = require('fs');
const path = require('path');
const here = __dirname;
const F = 'file:///C:/Dev/AI_Looter_Shooter/';
const MENU_IMG = F + 'Saved/Screenshots/Tools/main_menu.png';
// dark = the ring's own fill (rgb 18,21,26): cut-outs and the thin gaps round gold pieces.
const C = { line: '#d4eef4', acc: '#ffe6a6', lab: '#b4bec8', text: '#f2f6f8', icon: '#98a4ae', cyan: '#9fe0ff', dark: '#12151a' };

// ------------------------------------------------------------------ geometry helpers (mark space round 0,0, y down, 1 unit = 1 px)
const rad = (a) => a * Math.PI / 180;
const r1 = (v) => Math.round(v * 100) / 100;
const pt = (cx, cy, r, a) => [cx + r * Math.cos(rad(a)), cy + r * Math.sin(rad(a))];
function ell(cx, cy, rx, ry, a0, a1, step = 10) {
	const out = [], n = Math.max(2, Math.ceil(Math.abs(a1 - a0) / step));
	for (let i = 0; i <= n; i++) { const a = a0 + (a1 - a0) * i / n; out.push([cx + rx * Math.cos(rad(a)), cy + ry * Math.sin(rad(a))]); }
	return out;
}
const circ = (cx, cy, r, step = 20) => ell(cx, cy, r, r, 0, 360 - step, step);
function qb(p0, p1, p2, n = 8) {
	const out = [];
	for (let i = 0; i <= n; i++) { const t = i / n, u = 1 - t; out.push([u * u * p0[0] + 2 * u * t * p1[0] + t * t * p2[0], u * u * p0[1] + 2 * u * t * p1[1] + t * t * p2[1]]); }
	return out;
}
const cat = (...lists) => lists.reduce((a, l) => a.concat(a.length && l.length && a[a.length - 1][0] === l[0][0] && a[a.length - 1][1] === l[0][1] ? l.slice(1) : l), []);
const S = (pts, w = 3, o = {}) => ({ k: 'stroke', col: o.col || 'line', w, closed: !!o.closed, pts });
const Fi = (pts, o = {}) => ({ k: 'fill', col: o.col || 'acc', pts });

// ------------------------------------------------------------------ the eight marks
const MARKS = [];

// 6 · SPUR ROWEL (solid): ten slim spikes round a hub, the hub an ember in a dark cut.
{
	const star = [];
	for (let i = 0; i < 10; i++) { const a = -90 + 36 * i; star.push(pt(0, 0, 8.5, a - 12), pt(0, 0, 21, a), pt(0, 0, 8.5, a + 12)); }
	MARKS.push({
		key: 'SpurRowel', name: 'SPUR ROWEL',
		idea: 'A drifter\'s spur, ten sharp points round a hub that still glows.',
		parts: [Fi(star, { col: 'line' }), Fi(circ(0, 0, 6.2), { col: 'dark' }), Fi(circ(0, 0, 4.2), { col: 'acc' }), Fi(circ(0, 0, 1.4, 45), { col: 'dark' })],
	});
}

// 7 · CRACKED STAR (solid): a six-point star with blunt tips (no badge balls), split by a crack with an ember in it.
{
	const star = [];
	for (let i = 0; i < 6; i++) { const a = -90 + 60 * i; star.push(pt(0, 0, 20.5, a - 5), pt(0, 0, 20.5, a + 5), pt(0, 0, 10.5, a + 30)); }
	const crack = [[2.5, -21], [-0.5, -10], [2.5, -3.5], [-2.5, 3.5], [0.5, 10.5], [-2, 21]];
	MARKS.push({
		key: 'CrackedStar', name: 'CRACKED STAR',
		idea: 'The law\'s star split clean through, an ember burning in the break.',
		parts: [Fi(star, { col: 'line' }), S(crack, 5, { col: 'dark' }), S(crack, 1.8, { col: 'acc' }),
			Fi([[0.2, -6.5], [5.2, -1], [0.2, 4.5], [-4.8, -1]], { col: 'dark' }), Fi([[0.2, -4.3], [3.2, -1], [0.2, 2.3], [-2.8, -1]], { col: 'acc' })],
	});
}

// 8 · LIT CYLINDER (solid): a revolver cylinder end-on, fluted, six chambers cut dark, the top one lit gold.
{
	const body = [];
	for (let i = 0; i < 6; i++) {
		const m = -60 + 60 * i;			// flutes between the chambers
		body.push(...ell(0, 0, 19, 19, m - 52, m - 9, 7).slice(0, -1), pt(0, 0, 19, m - 8), pt(0, 0, 15.5, m - 4), pt(0, 0, 15.5, m + 4), pt(0, 0, 19, m + 8));
	}
	const parts = [Fi(body, { col: 'line' })];
	for (let i = 0; i < 6; i++) { const [x, y] = pt(0, 0, 10.5, -90 + 60 * i); parts.push(Fi(circ(x, y, 4.3), { col: i === 0 ? 'acc' : 'dark' })); }
	parts.push(Fi(circ(0, 0, 2.6, 30), { col: 'dark' }));
	MARKS.push({ key: 'LitCylinder', name: 'LIT CYLINDER', idea: 'Six chambers seen end-on, one of them lit: the shot you saved.', parts });
}

// 9 · ROOST CROW (solid, breaks the ring): the roost's crow cut as a sharp head, its beak through the ring.
{
	// The beak thick at the base with a curved top (a crow, not a heron); a ruff of throat feathers below.
	const head = cat([[-12, 21], [-15, 12], [-15.5, 3], [-13, -5], [-8, -11], [-1, -14], [6, -13]], qb([11, -10], [26, -8.5], [37, -1], 6),
		qb([37, -1], [25, 2], [12, 5], 4), [[13.5, 8.5], [9, 9.5], [11, 13.5], [6, 13.5], [7, 18], [1.5, 17], [1, 21]]);
	MARKS.push({
		key: 'RoostCrow', name: 'ROOST CROW',
		idea: 'The roost\'s crow, cut sharp, its beak breaking out through the ring.',
		parts: [S(head, 3.4, { col: 'dark', closed: true }), Fi(head, { col: 'line' }),
			S([[12, -1.6], [29, -1.2]], 1.6, { col: 'dark' }),				// the beak's seam
			Fi([[2.5, -9.5], [6.3, -6], [2.5, -2.5], [-1.3, -6]], { col: 'acc' })],	// the ember eye
	});
}

// 10 · SPIKED HALO (line): a railroad spike driven down through a gold halo; the halo's front crosses over the spike.
{
	const spike = [[-6.5, -17], [-4.5, -20.5], [4.5, -20.5], [6.5, -17], [2.8, -15], [2.8, 10], [0, 21], [-2.8, 10], [-2.8, -15]];
	MARKS.push({
		key: 'SpikedHalo', name: 'SPIKED HALO',
		idea: 'A rail spike driven through a halo: nailed back to the world.',
		parts: [S(ell(0, -3, 18, 6.5, 180, 360, 10), 3, { col: 'acc' }),			// the halo's back, behind the spike
			S(spike, 2.6, { col: 'dark', closed: true }), Fi(spike, { col: 'line' }),
			S(ell(0, -3, 18, 6.5, 0, 180, 10), 6.5, { col: 'dark' }), S(ell(0, -3, 18, 6.5, 0, 180, 10), 3, { col: 'acc' })],
	});
}

// 11 · EMBER HOURGLASS (line + gold): salt frame and glass, the sand gold embers running down.
{
	MARKS.push({
		key: 'EmberHourglass', name: 'EMBER HOURGLASS',
		idea: 'Time already ran out once: an hourglass running on embers.',
		parts: [
			Fi([[-14, -21], [14, -21], [14, -17], [-14, -17]], { col: 'line' }), Fi([[-14, 17], [14, 17], [14, 21], [-14, 21]], { col: 'line' }),
			S([[-11.5, -17], [-11.5, 17]], 2), S([[11.5, -17], [11.5, 17]], 2),
			S([[-8, -17], [8, -17], [1.7, 0], [8, 17], [-8, 17], [-1.7, 0]], 2.6, { closed: true }),
			Fi([[-3.8, -9], [3.8, -9], [0.7, -2.2], [-0.7, -2.2]]),				// the sand left above
			S([[0, -1.5], [0, 10]], 1.4, { col: 'acc' }),						// its stream
			Fi([[-5.7, 15], [-2, 11], [0, 9.6], [2, 11], [5.7, 15]]),			// the heap below
		],
	});
}

// 12 · CUT R (monogram, solid): a geometric R cut from salt steel, the counter cut dark, the leg gold.
{
	MARKS.push({
		key: 'CutR', name: 'CUT R',
		idea: 'The R as a monogram cut from steel plate, its leg cast in gold.',
		parts: [
			Fi([[-12, -14.5], [-8.5, -18], [-5, -18], [-5, 18], [-12, 18]], { col: 'line' }),			// the stem, its top corner cut
			Fi([[-5, -18], [6.5, -18], [12.5, -12], [12.5, -4], [6.5, 2], [-5, 2]], { col: 'line' }),	// the bowl
			Fi([[-5, -11.5], [3.8, -11.5], [5.8, -9.5], [5.8, -6.5], [3.8, -4.5], [-5, -4.5]], { col: 'dark' }),	// its counter
			Fi([[-1, 3.6], [6.4, 3.6], [14.5, 18], [7, 18]], { col: 'acc' }),					// the leg
		],
	});
}

// 13 · BRAND IRON R (monogram, line): a slab-serif R drawn at an iron's even width over a gold rocker, as a ranch brand.
{
	const bowl = cat([[-12.5, -16]], [[3, -16]], qb([3, -16], [10.5, -16], [10.5, -9], 6), qb([10.5, -9], [10.5, -2], [3, -2], 6), [[-7, -2]]);
	MARKS.push({
		key: 'BrandIronR', name: 'BRAND IRON R',
		idea: 'A slab-serif R burned like a ranch brand, sitting on a gold rocker.',
		parts: [
			S(bowl, 3.6), S([[-7, -16], [-7, 12]], 3.6),
			S([[-12.5, 12], [-1.5, 12]], 3.6),						// foot serif
			S([[1, -2], [8.5, 12]], 3.6), S([[4.5, 12], [14, 12]], 3.6),	// the leg and its serif
			S(qb([-17, 15.5], [0, 24], [17, 15.5], 10), 3.4, { col: 'acc' }),	// the rocker
		],
	});
}

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
const ring = (cx, cy, m) => '<circle cx="' + cx + '" cy="' + cy + '" r="34" fill="rgb(18,21,26)" fill-opacity=".8" stroke="' + C.line + '" stroke-width="2.5"/>' +
	ticks(cx, cy, 38, 44, 0, 350, 10, { major: 90, majorLen: 7, op: .55 }) + '<g transform="translate(' + cx + ' ' + cy + ')">' + markSvg(m) + '</g>';
const lockup = (m) => ring(58, 60, m) + T(116, 54, 'REVENANT', { fs: 48, ls: 13, fill: '#eef8fb', out: 3 }) + ruler(118, 430, 86, 8, 5, .6) + T(120, 110, 'TEROPA', { fs: 14, ls: 8, fill: C.cyan });

const at = (x, y, inner, css = '') => '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;' + css + '">' + inner + '</div>';
const box = (x, y, w, h, inner, css = '') => '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;width:' + w + 'px;height:' + h + 'px;' + css + '">' + inner + '</div>';
const svg = (w, h, inner, x = 0, y = 0) => '<svg width="' + w + '" height="' + h + '" style="position:absolute;left:' + x + 'px;top:' + y + 'px;overflow:visible">' + inner + '</svg>';
const MENU_BG = 'background:#04060a url(\'' + MENU_IMG + '\') -370px -160px/1683px auto';
const menuShade = '<div style="position:absolute;inset:0;background:linear-gradient(90deg,rgba(4,6,9,.9) 0,rgba(4,6,9,.6) 42%,rgba(4,6,9,0) 70%)"></div>';
// A w x h window onto the c3 menu frame from (ox, oy); inner is drawn in the frame's own coordinates.
const menuCrop = (x, y, w, h, inner, ox = 0, oy = 0) => box(x, y, w, h, box(-ox, -oy, 752, 420, menuShade + svg(752, 420, inner), MENU_BG),
	'overflow:hidden;border-radius:5px;box-shadow:0 0 0 1px rgba(212,238,244,.16)');
const chip = (t) => '<span style="padding:4px 9px;border-radius:4px;background:rgba(0,0,0,.66);box-shadow:inset 0 0 0 1px rgba(255,255,255,.22);font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:3px;color:#fff">' + t + '</span>';

// ------------------------------------------------------------------ the sheet: 2 columns x 4 rows
const TW = 936, TH = 232, GX = 16, GY = 96, G = 16, GR = 10, FIRST = 6;
function tile(i, m) {
	const big = 2, cx = 116, cy = 116;
	let h = '';
	// The mark large: c3's ring at 2x on a dark disc.
	h += box(cx - 108, cy - 108, 216, 216, '', 'border-radius:50%;background:radial-gradient(circle,rgba(4,6,9,.75) 55%,rgba(4,6,9,0) 72%)');
	h += svg(240, TH, '<g transform="translate(' + cx + ' ' + cy + ') scale(' + big + ') translate(-58 -60)">' + ring(58, 60, m) + '</g>');
	// Number, name, the idea on one line.
	h += box(240, 16, 40, 40, '', 'box-sizing:border-box;border:3px solid ' + C.acc + ';border-radius:7px') +
		at(240, 16, String(FIRST + i), 'width:40px;line-height:40px;text-align:center;font-family:Chakra;font-weight:700;font-size:22px;color:' + C.acc);
	h += at(294, 19, m.name, 'white-space:nowrap;font-family:Chakra;font-weight:700;font-size:25px;letter-spacing:4px;color:' + C.line);
	h += at(242, 64, m.idea, 'white-space:nowrap;font-family:Chakra;font-size:18px;color:' + C.text);
	// The real lockup, real size (frame rows 6-128).
	h += menuCrop(240, 98, 680, 122, lockup(m), 0, 6) + at(910, 188, chip('REAL SIZE'), 'white-space:nowrap;transform:translateX(-100%)');
	return h;
}
function sheet() {
	let h = '<div style="position:relative;width:1920px;height:1080px;overflow:hidden;background:radial-gradient(ellipse at 50% 10%,#1a1d22,#07080a 72%)">';
	h += box(32, 20, 8, 58, '', 'background:' + C.acc);
	h += at(58, 14, 'NEW LOGO · ROUND 2 · PICK ONE', 'white-space:nowrap;font-family:Chakra;font-weight:700;font-size:40px;letter-spacing:6px;color:' + C.line);
	h += at(60, 64, 'Eight more, a wider range: solid cuts with gold, two monograms, one that breaks the ring. Numbers go on from round 1 (1-5).', 'white-space:nowrap;font-family:Chakra;font-size:20px;color:' + C.lab);
	h += at(1888, 30, 'UI ROUND 3 · MAIN MENU MARK', 'white-space:nowrap;transform:translateX(-100%);font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:4px;color:' + C.icon);
	MARKS.forEach((m, k) => {
		const x = GX + (k % 2) * (TW + G), y = GY + Math.floor(k / 2) * (TH + GR);
		h += box(x, y, TW, TH, tile(k, m), 'border-radius:8px;background:linear-gradient(180deg,rgba(30,34,40,.9),rgba(18,21,26,.9));box-shadow:inset 0 0 0 1px rgba(212,238,244,.14)');
	});
	return h + '</div>';
}

// ------------------------------------------------------------------ marks2.json: the points for the C++ port
function writeJson() {
	const out = {
		about: 'Lane D logo marks, round 2 (numbers 6-13), for the c3 main menu. Same format as marks.json. Mark space centred on 0,0, y down, ' +
			'1 unit = 1 px at real size; drawn at the ring centre (58,60 in the c3 menu frame), ring r 34 (Roost Crow\'s beak runs out to x 37 on purpose). ' +
			'Parts draw in order. Strokes: miter joins, butt caps, width in px. Colours: line = #d4eef4 (salt), acc = #ffe6a6 (gold), ' +
			'dark = #12151a (the ring\'s fill: cut-outs and the gaps round gold). Fills are simple polygons, each star-shaped round its own centre.',
		marks: MARKS.map((m, i) => ({
			num: FIRST + i, key: m.key, name: m.name, idea: m.idea,
			parts: m.parts.map(p => Object.assign({ k: p.k, col: p.col }, p.k === 'stroke' ? { w: p.w, closed: p.closed } : {},
				{ d: pd(p.pts, p.k === 'fill' || p.closed), pts: p.pts.map(q => [r1(q[0]), r1(q[1])]) })),
		})),
	};
	fs.writeFileSync(path.join(here, 'marks2.json'), JSON.stringify(out, null, 1));
}

const css = '@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Regular.ttf\');font-weight:400}' +
	'@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Bold.ttf\');font-weight:700}body{margin:0;background:#07080a}';
(async () => {
	writeJson();
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	const file = path.join(here, 'logos2.html');
	fs.writeFileSync(file, '<!doctype html><html><head><meta charset="utf-8"><style>' + css + '</style></head><body>' + sheet() + '</body></html>');
	await pg.goto('file:///' + file.split(path.sep).join('/'));
	await pg.evaluate(() => document.fonts.ready);
	await pg.waitForTimeout(700);
	await pg.screenshot({ path: path.join(here, 'logos2.png') });
	await browser.close();
	console.log('logos2.png, marks2.json');
})();
