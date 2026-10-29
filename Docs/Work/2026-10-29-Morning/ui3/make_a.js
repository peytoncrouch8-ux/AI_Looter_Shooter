// UI round 3, lane P1 (2026-10-29 morning): three new directions for the pause menu, the main menu and the level-up
// popup, each its own idea (see BRIEF.md). Mockup only. Run from the repo root:
//   node Docs/Work/2026-10-29-Morning/ui3/make_a.js  ->  ui3/c1.png (Ledger & Brass), c2.png (Slipstream), c3.png (Halo Reticle)
// Words are the game's own (SettingsMenuWidget/Pages/Help, MainMenuWidget, HudLevelUpBannerWidget).
const fs = require('fs');
const path = require('path');
const here = __dirname;
const F = 'file:///C:/Dev/AI_Looter_Shooter/';
const IMG = {
	game: F + 'Saved/Screenshots/Tools/emb_00_ready.png',	// 1920x1040, Ransom's Rest street
	sky: F + 'Saved/Screenshots/Tools/camo1.png',		// 1936x1056, bright sky over the farmhouse
	menu: F + 'Saved/Screenshots/Tools/main_menu.png',	// 1870x858, Teropa from orbit
};
// Slate & Salt (the brief's fixed colours) plus the kit colours that stay.
const C = { line: '#d4eef4', acc: '#ffe6a6', lab: '#b4bec8', text: '#f2f6f8', fl: '#ffd38a', icon: '#98a4ae',
	gem: '#0e1115', red: '#ff7a6b', cyan: '#9fe0ff', dark: '#04060a' };
const OUTS = '0 0 2px #04060a,0 0 2px #04060a,0 0 3px #04060a,0 2px 6px rgba(0,0,0,.8)';

// ------------------------------------------------------------------ small helpers
const rad = (a) => a * Math.PI / 180;
const pt = (cx, cy, r, a) => [cx + r * Math.cos(rad(a)), cy + r * Math.sin(rad(a))];
const pd = (P) => 'M' + P.map(q => q[0].toFixed(1) + ' ' + q[1].toFixed(1)).join(' L') + ' Z';
const at = (x, y, inner, css = '') => '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;white-space:nowrap;' + css + '">' + inner + '</div>';
const box = (x, y, w, h, inner, css = '') => '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;width:' + w + 'px;height:' + h + 'px;' + css + '">' + inner + '</div>';
let seed = 7;
const rnd = () => { seed = (seed * 16807) % 2147483647; return (seed - 1) / 2147483646; };

// SVG text, its y the middle of the line. o: fs, ls, fill, a (start|middle|end), w (weight), op, out (dark outline width), tr.
function T(x, y, s, o = {}) {
	const ls = o.ls ?? 0, a = o.a ?? 'start';
	const xx = a === 'middle' ? x + ls / 2 : a === 'end' ? x + ls : x;
	return '<text x="' + xx + '" y="' + y + '" dominant-baseline="central" text-anchor="' + a + '" style="font-family:Chakra;font-weight:' + (o.w ?? 700) +
		';font-size:' + (o.fs ?? 20) + 'px;letter-spacing:' + ls + 'px" fill="' + (o.fill ?? C.text) + '"' + (o.op != null ? ' opacity="' + o.op + '"' : '') +
		(o.out ? ' stroke="#04060a" stroke-opacity="' + (o.outOp ?? .85) + '" stroke-width="' + o.out + '" stroke-linejoin="round" paint-order="stroke"' : '') +
		(o.tr ? ' transform="' + o.tr + '"' : '') + '>' + s + '</text>';
}
// A path with an optional dark edge under its line (every line in the kit sits on a dark edge).
function P(d, o = {}) {
	let s = '';
	if (o.stroke && o.edge) { s += '<path d="' + d + '" fill="none" stroke="#04060a" stroke-opacity="' + (o.edgeOp ?? .65) + '" stroke-width="' + ((o.sw ?? 3) + 3) + '" stroke-linejoin="miter" stroke-linecap="' + (o.cap ?? 'butt') + '"/>'; }
	s += '<path d="' + d + '" fill="' + (o.fill ?? 'none') + '"' + (o.fo != null ? ' fill-opacity="' + o.fo + '"' : '') +
		(o.stroke ? ' stroke="' + o.stroke + '" stroke-width="' + (o.sw ?? 3) + '" stroke-opacity="' + (o.so ?? 1) + '" stroke-linejoin="miter" stroke-linecap="' + (o.cap ?? 'butt') + '"' : '') +
		(o.dash ? ' stroke-dasharray="' + o.dash + '"' : '') + (o.op != null ? ' opacity="' + o.op + '"' : '') + '/>';
	return s;
}
const L = (x1, y1, x2, y2, o) => P('M' + x1 + ' ' + y1 + ' L' + x2 + ' ' + y2, o);
const cham = (x, y, w, h, c) => [[x + c, y], [x + w - c, y], [x + w, y + c], [x + w, y + h - c], [x + w - c, y + h], [x + c, y + h], [x, y + h - c], [x, y + c]];
const rect = (x, y, w, h) => [[x, y], [x + w, y], [x + w, y + h], [x, y + h]];
function arc(cx, cy, r, a0, a1) {
	const [x0, y0] = pt(cx, cy, r, a0), [x1, y1] = pt(cx, cy, r, a1);
	return 'M' + x0.toFixed(1) + ' ' + y0.toFixed(1) + ' A' + r + ' ' + r + ' 0 ' + (Math.abs(a1 - a0) > 180 ? 1 : 0) + ' 1 ' + x1.toFixed(1) + ' ' + y1.toFixed(1);
}
const DEFS = '<defs>' +
	'<linearGradient id="glass" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="rgb(30,34,40)" stop-opacity=".95"/><stop offset="1" stop-color="rgb(18,21,26)" stop-opacity=".93"/></linearGradient>' +
	'<linearGradient id="goldwash" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="' + C.acc + '" stop-opacity=".30"/><stop offset=".8" stop-color="' + C.acc + '" stop-opacity=".02"/></linearGradient>' +
	'<linearGradient id="speedG" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="' + C.acc + '" stop-opacity="0"/><stop offset="1" stop-color="' + C.acc + '" stop-opacity=".95"/></linearGradient>' +
	'<linearGradient id="speedS" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="' + C.line + '" stop-opacity="0"/><stop offset="1" stop-color="' + C.line + '" stop-opacity=".8"/></linearGradient>' +
	'<radialGradient id="rivM" cx=".35" cy=".35" r=".75"><stop offset="0" stop-color="#e6ecf0"/><stop offset=".45" stop-color="#7c8790"/><stop offset="1" stop-color="#262b31"/></radialGradient>' +
	'<radialGradient id="rivB" cx=".35" cy=".35" r=".75"><stop offset="0" stop-color="#fff3d2"/><stop offset=".45" stop-color="#c9a464"/><stop offset="1" stop-color="#4a3a1e"/></radialGradient>' +
	'<radialGradient id="wax" cx=".4" cy=".35" r=".8"><stop offset="0" stop-color="#fff0c8"/><stop offset=".45" stop-color="' + C.acc + '"/><stop offset=".85" stop-color="#c9a264"/><stop offset="1" stop-color="#8c6a34"/></radialGradient>' +
	'<radialGradient id="waxIn" cx=".5" cy=".5" r=".6"><stop offset="0" stop-color="#f3d79a"/><stop offset=".8" stop-color="#e2bd78"/><stop offset="1" stop-color="#b48c4a"/></radialGradient>' +
	'</defs>';
const svg = (w, h, inner) => '<svg width="' + w + '" height="' + h + '" style="position:absolute;left:0;top:0;overflow:visible">' + DEFS + inner + '</svg>';

// Hollow rounded keys (the user's pick) in HTML, so their widths fit the words.
const cap = (k) => '<span class="cap">' + k + '</span>';
const kp = (k, words) => '<span class="kp">' + cap(k) + words + '</span>';
const keys = (x, y, w, list, css = '') => box(x, y, w, 34, list.map(([k, t]) => kp(k, t)).join(''), 'display:flex;justify-content:center;gap:30px;' + css);
const label = (t, css) => at(0, 0, t, 'padding:5px 11px;border-radius:5px;background:rgba(0,0,0,.66);box-shadow:inset 0 0 0 1px rgba(255,255,255,.22);font-family:Chakra;font-weight:700;font-size:14px;letter-spacing:3px;color:#fff;' + css);
const frame = (x, y, w, h, bg, inner) => box(x, y, w, h, inner, 'overflow:hidden;border-radius:6px;' + bg);
const BG = {
	pause: 'background:#0b0d10 url(\'' + IMG.game + '\') -600px -165px/2150px auto',
	menu: 'background:#04060a url(\'' + IMG.menu + '\') -340px -124px/1496px auto',
	sky: 'background:#4a6c90 url(\'' + IMG.sky + '\') -560px -150px/1936px auto',
};
const dim = '<div style="position:absolute;inset:0;background:radial-gradient(ellipse at 45% 50%,rgba(5,7,10,.5),rgba(5,7,10,.78) 85%)"></div>';
const menuShade = '<div style="position:absolute;inset:0;background:linear-gradient(90deg,rgba(4,6,9,.9) 0,rgba(4,6,9,.6) 42%,rgba(4,6,9,0) 70%)"></div>';
const popShade = (cx, cy, w, h) => box(cx - w / 2, cy - h / 2, w, h, '', 'background:radial-gradient(ellipse at 50% 50%,rgba(4,6,9,.5),rgba(4,6,9,0) 70%)');

// The page: a title strip, the pause menu big on the left, the main menu top right, the level-up under it, the notes.
const FR = { pause: [32, 116, 1080, 932], menu: [1136, 116, 752, 420], level: [1136, 552, 752, 292], notes: [1136, 860, 752, 188] };
function page(cfg) {
	const g = (t) => '<b style="color:' + C.acc + '">' + t + '</b>';
	const notes = '<div class="nh">MOTION</div>' + cfg.motion.map(m => '<div class="li">' + m + '</div>').join('') +
		'<div class="nh" style="margin-top:8px">SOUND</div><div class="li">' + cfg.sound + '</div>';
	const [px, py, pw, ph] = FR.pause, [mx, my, mw, mh] = FR.menu, [lx, ly, lw, lh] = FR.level, [nx, ny, nw, nh] = FR.notes;
	return '<div style="position:relative;width:1920px;height:1080px;overflow:hidden;background:radial-gradient(ellipse at 50% 10%,#1a1d22,#07080a 72%)">' +
		box(0, 0, 1920, 100, '', 'background:linear-gradient(180deg,rgba(0,0,0,.55),rgba(0,0,0,0))') +
		box(32, 22, 8, 58, '', 'background:' + C.acc + ';' + (cfg.barCss || '')) +
		at(58, 16, cfg.num + ' · ' + cfg.name, 'font-family:Chakra;font-weight:700;font-size:40px;letter-spacing:6px;color:' + C.line) +
		at(60, 66, cfg.idea, 'font-family:Chakra;font-size:20px;color:' + C.lab) +
		at(1888, 30, 'UI ROUND 3 · CONCEPT ' + cfg.num + ' OF 6', 'transform:translateX(-100%);font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:4px;color:' + C.icon) +
		frame(px, py, pw, ph, BG.pause, dim + cfg.pause() + box(pw - 14, 14, 0, 0, label('PAUSE MENU', 'transform:translateX(-100%)'))) +
		frame(mx, my, mw, mh, cfg.menuBg || BG.menu, menuShade + cfg.menu() + box(mw - 14, mh - 46, 0, 0, label('MAIN MENU', 'transform:translateX(-100%)'))) +
		frame(lx, ly, lw, lh, BG.sky, cfg.level() + box(14, 14, 0, 0, label('LEVEL UP · HUD, NO GLASS'))) +
		box(nx, ny, nw, nh, notes.replace(/\{([^}]+)\}/g, (_, t) => g(t)).replace(/\[([^\]]+)\]/g, '<span style="color:' + C.cyan + '">$1</span>'), 'font-family:Chakra;font-size:17px;line-height:1.36;color:' + C.text) +
		'</div>';
}

// ------------------------------------------------------------------ shared bits
function rivet(x, y, r = 5, brass = false) {
	return '<circle cx="' + x + '" cy="' + y + '" r="' + (r + 1.6) + '" fill="#04060a" opacity=".75"/><circle cx="' + x + '" cy="' + y + '" r="' + r + '" fill="url(#' + (brass ? 'rivB' : 'rivM') + ')"/>';
}
// The ember flame (a stand-in for MainMenuArt::TitleMark), drawn in a 30x40 box.
const FLAME = 'M15 2 C22 12 25 18 22 27 C20 33 17 36 15 38 C13 36 10 33 8 27 C5 18 8 12 15 2 Z';
const FLAME_IN = 'M15 16 C18 21 18 26 15 31 C12 26 12 21 15 16 Z';
// The level gem as a diamond (dark glass, gold edge, the lit top half).
function diamond(cx, cy, r, inner, sw = 4) {
	const D = [[cx, cy - r], [cx + r, cy], [cx, cy + r], [cx - r, cy]];
	return P(pd(D), { fill: C.gem, stroke: C.acc, sw, edge: true }) + P(pd([[cx - r, cy], [cx, cy - r], [cx + r, cy]]), { fill: C.acc, fo: .1 }) +
		P(pd([[cx, cy - r * .74], [cx + r * .74, cy], [cx, cy + r * .74], [cx - r * .74, cy]]), { stroke: C.acc, sw: 1.6, so: .38 }) + inner;
}

// ================================================================== 1 · LEDGER & BRASS
// Weird-west craft, kept crisp: a slate plate trimmed with an engraved brass hairline and gunmetal, rivets in the corners,
// stamped serials, every item on a ruled ledger line with a ruled margin, a brass ribbon marking the choice, a wax seal.
function waxSeal(cx, cy, R, inner) {
	const Q = [];
	for (let i = 0; i < 160; i++) {
		const a = i / 160 * Math.PI * 2;
		const r = R * (1 + .05 * Math.sin(a * 11) + .03 * Math.sin(a * 5 + 1.3) + .022 * Math.sin(a * 17 + .4));
		Q.push([cx + r * Math.cos(a), cy + r * Math.sin(a)]);
	}
	let s = '<path d="' + pd(Q) + '" fill="#04060a" opacity=".55" transform="translate(2 4)"/>';
	s += '<path d="' + pd(Q) + '" fill="url(#wax)" stroke="#6b4f22" stroke-width="1.5"/>';
	s += '<circle cx="' + cx + '" cy="' + cy + '" r="' + R * .72 + '" fill="url(#waxIn)" stroke="#8c6a34" stroke-width="2"/>';
	s += '<circle cx="' + cx + '" cy="' + (cy + 1.2) + '" r="' + R * .72 + '" fill="none" stroke="#fff6dc" stroke-opacity=".5" stroke-width="1.2" stroke-dasharray="' + (R * 1.6) + ' ' + (R * 3) + '"/>';
	// The pressed ring of beads, like a die's border.
	for (let i = 0; i < 24; i++) { const [x, y] = pt(cx, cy, R * .62, i * 15); s += '<circle cx="' + x.toFixed(1) + '" cy="' + y.toFixed(1) + '" r="' + (R * .028 + .6) + '" fill="#8c6a34" opacity=".7"/>'; }
	return s + inner;
}
const emboss = (x, y, t, fs, ls = 0) => T(x, y + 1.6, t, { fs, ls, a: 'middle', fill: '#fff6dc', op: .6 }) + T(x, y, t, { fs, ls, a: 'middle', fill: '#5a4120' });
// A swallowtail ribbon in brass gold, its tails out past the left edge.
function ribbon(x0, x1, cy, h, t) {
	const Q = [[x0, cy - h / 2], [x1, cy - h / 2], [x1 + 10, cy], [x1, cy + h / 2], [x0, cy + h / 2], [x0 + 14, cy]];
	return P(pd(Q), { fill: C.acc, stroke: '#04060a', sw: 1.5, so: .6 }) + L(x0 + 4, cy - h / 2 + 4, x1 - 2, cy - h / 2 + 4, { stroke: '#fff6dc', sw: 1, so: .7 }) +
		(t ? T((x0 + x1) / 2 + 8, cy, t, { fs: 15, ls: 2, a: 'middle', fill: C.gem }) : '') + rivet(x1 - 8, cy, 3.6, true);
}
const dbl = (x1, x2, y, col, so = 1) => L(x1, y, x2, y, { stroke: col, sw: 1.2, so }) + L(x1, y + 5, x2, y + 5, { stroke: col, sw: 2.6, so });

function c1Pause() {
	const X = 100, Y = 50, W = 880, H = 770;
	let s = '';
	s += P(pd(cham(X, Y, W, H, 16)), { fill: 'url(#glass)' });
	s += P(pd(cham(X, Y, W, H, 16)), { stroke: C.icon, sw: 3.5, edge: true });
	s += P(pd(cham(X + 10, Y + 10, W - 20, H - 20, 10)), { stroke: C.fl, sw: 1.4, so: .55 });
	s += P(pd(cham(X + 15, Y + 15, W - 30, H - 30, 8)), { stroke: C.icon, sw: 1, so: .28 });
	[[X + 28, Y + 28], [X + W - 28, Y + 28], [X + 28, Y + H - 28], [X + W - 28, Y + H - 28]].forEach(([x, y]) => { s += rivet(x, y, 5.5); });
	// The header and its stamped serial.
	s += T(X + 124, Y + 66, 'GAME PAUSED', { fs: 38, ls: 10 });
	s += P(pd(rect(X + W - 314, Y + 47, 254, 38)), { stroke: C.fl, sw: 1.6, so: .8 }) + P(pd(rect(X + W - 310, Y + 51, 246, 30)), { stroke: C.fl, sw: .8, so: .45 });
	s += T(X + W - 187, Y + 66, 'NO. 01 · RANSOM\'S REST', { fs: 15, ls: 3, a: 'middle', fill: C.fl });
	s += L(X + 30, Y + 112, X + W - 30, Y + 112, { stroke: C.fl, sw: 1.6, so: .75 }) + L(X + 30, Y + 117, X + W - 30, Y + 117, { stroke: C.icon, sw: 1, so: .5 });
	s += rivet(X + 30, Y + 114.5, 3.2, true) + rivet(X + W - 30, Y + 114.5, 3.2, true);
	// The ruled margin and the ledger lines.
	const top = Y + 124, rh = 64, M = X + 96;
	s += L(M, Y + 117, M, Y + H - 44, { stroke: C.fl, sw: 1.6, so: .6 }) + L(M + 5, Y + 117, M + 5, Y + H - 44, { stroke: C.fl, sw: .8, so: .35 });
	for (let k = 1; k <= 9; k++) { s += L(X + 30, top + rh * k, X + W - 30, top + rh * k, { stroke: C.line, sw: 1.2, so: .15 }); }
	const cy = (k) => top + rh * k + rh / 2;
	// Row 0: Resume, the choice. The ribbon, the gold ink double rule of a ledger total.
	s += P(pd(rect(M + 6, top + 2, W - (M + 6 - X) - 30, rh - 4)), { fill: 'url(#goldwash)' });
	s += dbl(M + 12, X + W - 40, top + rh - 11, C.acc);
	s += ribbon(X - 26, X + 74, cy(0), 38, '');
	s += T(M + 32, cy(0) - 2, 'RESUME', { fs: 31, ls: 10, fill: C.acc });
	// Row 1: the tabs as the ledger's column heads, brass hairlines between them.
	const tx = X + 202, tw = 146;
	['GRAPHICS', 'AUDIO', 'INTERFACE', 'CONTROLS'].forEach((t, i) => {
		const x = tx + i * tw, on = i === 0;
		if (i) { s += L(x, cy(1) - 16, x, cy(1) + 16, { stroke: C.fl, sw: 1.2, so: .5 }); }
		s += T(x + tw / 2, cy(1) - 2, t, { fs: 18, ls: 3, a: 'middle', fill: on ? C.acc : C.lab });
		if (on) { s += dbl(x + 16, x + tw - 16, cy(1) + 15, C.acc) + rivet(x + 12, cy(1) + 18, 2.8, true) + rivet(x + tw - 12, cy(1) + 18, 2.8, true); }
	});
	// Rows 2-4: the Graphics page, stamped numbers in the margin, values ruled to the right edge.
	const R = X + W - 44;
	[['01', 'Quality'], ['02', 'Motion blur'], ['03', 'Field of view (1st person)']].forEach(([n, t], i) => {
		s += T(X + 52, cy(2 + i), n, { fs: 17, ls: 2, a: 'middle', fill: C.fl, op: .85 });
		s += T(M + 32, cy(2 + i), t, { fs: 22, fill: C.lab });
	});
	const chips = (k, list, chosen) => {
		const ws = list.map(t => t.length * 12.5 + 30);
		let x = R - ws.reduce((a, b) => a + b, 0) - 8 * (list.length - 1), out = '';
		list.forEach((t, i) => {
			if (i === chosen) { out += P(pd(cham(x, cy(k) - 18, ws[i], 36, 6)), { fill: C.acc, fo: .14, stroke: C.acc, sw: 3, edge: true }); }
			out += T(x + ws[i] / 2, cy(k), t, { fs: 16, ls: 2, a: 'middle', fill: i === chosen ? C.acc : C.lab });
			x += ws[i] + 8;
		});
		return out;
	};
	s += chips(2, ['LOW', 'MEDIUM', 'HIGH', 'EPIC'], 1);
	s += chips(3, ['ON', 'OFF'], 1);
	// Field of view: an engraved rule with ticks, the knob a brass rivet in a gold ring.
	const sx0 = R - 330, sx1 = R - 90, k4 = cy(4);
	for (let i = 0; i <= 24; i++) { const x = sx0 + (sx1 - sx0) * i / 24, big = i % 6 === 0; s += L(x, k4 - (big ? 11 : 5), x, k4, { stroke: C.fl, sw: big ? 1.8 : 1, so: big ? .8 : .45 }); }
	s += L(sx0, k4 + 1, sx1, k4 + 1, { stroke: C.icon, sw: 2, so: .6 }) + L(sx0, k4 + 1, sx0 + (sx1 - sx0) * .5, k4 + 1, { stroke: C.acc, sw: 3.5, edge: true });
	s += '<circle cx="' + (sx0 + (sx1 - sx0) * .5) + '" cy="' + (k4 + 1) + '" r="11" fill="' + C.gem + '" stroke="' + C.acc + '" stroke-width="3"/>' + rivet(sx0 + (sx1 - sx0) * .5, k4 + 1, 5, true);
	s += T(R, k4, '90°', { fs: 22, a: 'end' });
	// Rows 5-6: the footnote (Resume's description), written on the ledger's lines.
	s += T(X + 52, cy(5), 'NOTE', { fs: 13, ls: 2, a: 'middle', fill: C.fl, op: .85 });
	// Rows 7-8: Save & Quit as a red rubber stamp across two lines; the plate's serial bottom left.
	const qx = X + W - 330, qy = top + rh * 7 + 10, qw = 270, qh = rh * 2 - 20;
	s += '<g transform="rotate(-3 ' + (qx + qw / 2) + ' ' + (qy + qh / 2) + ')">' + P(pd(rect(qx, qy, qw, qh)), { stroke: C.red, sw: 3.5, edge: true, fill: C.red, fo: .06 }) +
		P(pd(rect(qx + 7, qy + 7, qw - 14, qh - 14)), { stroke: C.red, sw: 1.3, so: .8 }) + T(qx + qw / 2, qy + qh / 2, 'SAVE &amp; QUIT', { fs: 26, ls: 6, a: 'middle', fill: C.red }) + '</g>';
	s += T(M + 32, top + rh * 8 + 32, 'SER. RV-0001-07', { fs: 14, ls: 5, fill: C.icon, op: .75 });
	let h = svg(1080, 932, s);
	h += at(M + 32, top + rh * 5 + 4, '<span style="color:' + C.acc + ';font-weight:700;letter-spacing:3px">RESUME</span> &nbsp;Closes the menu and goes back to the game. Every change here is already saved.',
		'white-space:normal;width:' + (W - 150) + 'px;font-family:Chakra;font-size:21px;line-height:' + rh + 'px;margin-top:-8px;color:' + C.text);
	h += at(M + 18, cy(1) - 17, cap('PGUP'), 'transform:translateX(-4px)');
	h += at(tx + 4 * tw + 14, cy(1) - 17, cap('PGDN'));
	h += keys(X, Y + H + 22, W, [['ENTER', 'SELECT'], ['PGUP / PGDN', 'TABS'], ['ESC', 'RESUME']]);
	return h;
}
function c1Menu() {
	let s = '';
	s += waxSeal(62, 62, 42, '<g transform="translate(47 42)"><path d="' + FLAME + '" fill="none" stroke="#5a4120" stroke-width="3.2"/><path d="' + FLAME + '" fill="none" stroke="#fff6dc" stroke-opacity=".5" stroke-width="1" transform="translate(0 1.4)"/><path d="' + FLAME_IN + '" fill="#5a4120"/></g>');
	s += L(118, 16, 560, 16, { stroke: C.fl, sw: 1.4, so: .75 }) + L(118, 20, 560, 20, { stroke: C.icon, sw: 1, so: .5 }) + rivet(560, 18, 3.2, true);
	s += T(124, 60, 'REVENANT', { fs: 52, ls: 15, fill: '#eef8fb', out: 3 });
	s += L(118, 100, 560, 100, { stroke: C.fl, sw: 1.4, so: .75 }) + L(118, 104, 560, 104, { stroke: C.icon, sw: 1, so: .5 }) + rivet(560, 102, 3.2, true);
	s += T(126, 122, 'TEROPA', { fs: 15, ls: 10, fill: C.fl });
	s += T(560, 122, 'NO. 0001', { fs: 13, ls: 4, a: 'end', fill: C.icon });
	// The list on ledger lines with a ruled margin.
	const top = 142, rh = 54, M = 96;
	s += L(M, top - 2, M, top + rh * 4 + 2, { stroke: C.fl, sw: 1.6, so: .6 }) + L(M + 5, top - 2, M + 5, top + rh * 4 + 2, { stroke: C.fl, sw: .8, so: .35 });
	for (let k = 1; k <= 4; k++) { s += L(24, top + rh * k, 560, top + rh * k, { stroke: C.line, sw: 1.2, so: .18 }); }
	const items = [['01', 'SINGLE PLAYER'], ['02', 'MULTIPLAYER'], ['03', 'SETTINGS'], ['04', 'QUIT GAME']];
	items.forEach(([n, t], k) => {
		const y = top + rh * k + rh / 2, on = k === 0, off = k === 1;
		if (on) {
			s += P(pd(rect(M + 6, top + rh * k + 2, 458, rh - 4)), { fill: 'url(#goldwash)' }) + dbl(M + 12, 550, top + rh * (k + 1) - 10, C.acc);
			s += ribbon(4, 80, y, 34, '01');
		} else { s += T(54, y, n, { fs: 16, ls: 2, a: 'middle', fill: C.fl, op: off ? .4 : .85 }); }
		s += T(M + 26, y - 2, t, { fs: 28, ls: 6, fill: on ? C.acc : C.text, op: off ? .42 : 1, out: on ? 0 : 3, outOp: .5 });
	});
	// "Soon" pressed on like a rubber stamp.
	s += '<g transform="rotate(-7 455 223)">' + P(pd(rect(418, 210, 78, 28)), { stroke: C.red, sw: 2.4, so: .85 }) + T(457, 224, 'SOON', { fs: 14, ls: 4, a: 'middle', fill: C.red, op: .9 }) + '</g>';
	return svg(752, 420, s) + at(M + 26, 368, kp('ENTER', 'SELECT'));
}
function c1Level() {
	let s = '';
	const cx = 236, cy = 150;
	s += waxSeal(cx, cy, 70, emboss(cx, cy - 4, '7', 66) + T(cx, cy + 34, 'LEVEL', { fs: 12, ls: 4, a: 'middle', fill: '#5a4120' }));
	// No glass: outlined words on engraved double rules.
	s += T(330, 108, 'LEVEL UP', { fs: 62, ls: 10, out: 5 });
	s += L(326, 152, 690, 152, { stroke: C.fl, sw: 2, edge: true }) + L(326, 158, 690, 158, { stroke: C.fl, sw: 1.2, edge: true, edgeOp: .4 });
	s += rivet(326, 155, 4, true) + rivet(690, 155, 4, true);
	s += T(330, 192, 'MAX HEALTH', { fs: 25, ls: 4, out: 4 });
	for (let x = 546; x < 626; x += 10) { s += '<circle cx="' + x + '" cy="200" r="2.2" fill="' + C.fl + '" stroke="#04060a" stroke-width="1.5" paint-order="stroke"/>'; }
	s += T(690, 192, '+8%', { fs: 30, a: 'end', fill: C.acc, out: 4 });
	return popShade(470, 150, 640, 300) + svg(752, 292, s);
}
const C1 = {
	num: 1, name: 'LEDGER &amp; BRASS',
	idea: 'Weird-west craft, kept crisp: engraved brass and gunmetal trim, rivets, stamped serials, a wax seal for the level, every item on a ruled ledger line.',
	pause: c1Pause, menu: c1Menu, level: c1Level,
	motion: ['Opens: the plate {drops 16 px and settles}; a glint runs along the brass trim.',
		'Move: {the brass ribbon slides to the row} (0.09 s); the gold double rule {inks in}.',
		'Level up: {the seal stamps down} (1.4 → 1, a dust puff); the rules draw out.'],
	sound: 'open [UI.Open] · move [UI.Hover] · pick [UI.Menu.Select] · seal [UI.LevelUp] + a stamp thud (new)',
};

// ================================================================== 2 · SLIPSTREAM
// Everything leans 12° forward; panels are layered parallelogram strips that slide past each other; the choice is a solid
// gold slash trailing speed lines. Text leans with it.
const SK = -12, TANK = Math.tan(rad(12));
const sk = (x, y, inner) => '<g transform="translate(' + x + ' ' + y + ') skewX(' + SK + ')">' + inner + '</g>';
// x of a strip's left edge so stacked strips line up along the 12° lean (y0/x0 the first strip).
const lean = (x0, y0, y) => x0 - (y - y0) * TANK;
function speed(x1, x2, y0, y1, n, gold, o = {}) {
	let s = '';
	for (let i = 0; i < n; i++) {
		const y = y0 + (y1 - y0) * rnd(), len = (x2 - x1) * (.35 + .65 * rnd()), th = o.th ?? (1 + rnd() * 2.5);
		s += '<rect x="' + (x2 - len).toFixed(1) + '" y="' + y.toFixed(1) + '" width="' + len.toFixed(1) + '" height="' + th.toFixed(1) + '" fill="url(#' + (gold ? 'speedG' : 'speedS') + ')" opacity="' + (o.op ?? (.35 + .5 * rnd())).toFixed(2) + '"/>';
	}
	return s;
}
const strip = (w, h, o = {}) => P(pd(rect(0, 0, w, h)), { fill: o.fill ?? 'url(#glass)', fo: o.fo }) +
	(o.top !== false ? L(0, 0, w, 0, { stroke: o.line ?? C.line, sw: o.sw ?? 3, edge: true }) : '') + (o.bottom ? L(0, h, w, h, { stroke: o.line ?? C.line, sw: o.sw ?? 3, edge: true }) : '') +
	(o.stub ? P(pd(rect(0, 0, o.stub, h)), { fill: o.stubCol ?? C.acc }) : '');
function c2Pause() {
	seed = 11;
	let s = '';
	// Wake lines across the whole view, then the back slab and its edge strip (the layer the strips slide over).
	s += speed(0, 1080, 40, 900, 26, false, { op: .18, th: 1.5 });
	s += sk(200, 40, P(pd(rect(0, 0, 820, 800)), { fill: 'rgb(18,21,26)', fo: .55 }) + P(pd(rect(820, 0, 12, 800)), { fill: C.line, fo: .55 }));
	// Header strip: big leaning title, a gold stub, a thin salt strip offset under it.
	s += sk(150, 60, strip(660, 92, { stub: 16 }) + T(44, 46, 'GAME PAUSED', { fs: 48, ls: 7 }));
	s += sk(lean(150, 60, 160) + 120, 160, P(pd(rect(0, 0, 560, 6)), { fill: C.line, fo: .75 }));
	// Resume: the gold slash, with its speed lines trailing left.
	s += speed(24, 150, 196, 262, 9, true, { th: 2.5 });
	s += sk(lean(150, 60, 190) - 10, 190, P(pd(rect(0, 0, 720, 76)), { fill: C.acc }) + P(pd(rect(720 + 14, 0, 18, 76)), { fill: C.acc, fo: .7 }) + P(pd(rect(720 + 44, 0, 8, 76)), { fill: C.acc, fo: .4 }) +
		T(48, 38, 'RESUME', { fs: 36, ls: 10, fill: C.gem }) + T(690, 38, 'ENTER', { fs: 15, ls: 3, a: 'end', fill: C.gem, op: .7 }));
	// Tabs: one strip, slanted separators, the chosen tab's gold underslash.
	const ty = 296, tx = lean(150, 60, ty);
	s += sk(tx, ty, strip(780, 60) + ['GRAPHICS', 'AUDIO', 'INTERFACE', 'CONTROLS'].map((t, i) => {
		const x = 96 + i * 148, on = i === 0;
		return (i ? L(x, 14, x, 46, { stroke: C.line, sw: 2, so: .35 }) : '') + T(x + 74, 30, t, { fs: 18, ls: 3, a: 'middle', fill: on ? C.acc : C.lab }) +
			(on ? P(pd(rect(x + 10, 50, 128, 6)), { fill: C.acc }) : '');
	}).join(''));
	// The three rows, each its own strip along the lean; values as leaning chips.
	const rows = [['Quality', 'q'], ['Motion blur', 'm'], ['Field of view (1st person)', 'f']];
	const chip = (x, y, w, t, on) => P(pd(rect(x, y, w, 36)), on ? { fill: C.acc, fo: .16, stroke: C.acc, sw: 3, edge: true } : { stroke: C.line, sw: 1.6, so: .35 }) +
		T(x + w / 2, y + 18, t, { fs: 15, ls: 2, a: 'middle', fill: on ? C.acc : C.lab });
	rows.forEach(([t, k], i) => {
		const y = 384 + i * 78, x = lean(150, 60, y) - 16 + (i % 2) * 18;
		let r = strip(620, 66, { top: false, bottom: true, sw: 2, line: C.line });
		r += T(30, 33, t, { fs: 22, fill: C.lab });
		if (k === 'q') { [['LOW', 64], ['MEDIUM', 96], ['HIGH', 70], ['EPIC', 70]].reduce((xx, [w0, w], j) => { r += chip(xx, 15, w, w0, j === 1); return xx + w + 8; }, 280); }
		if (k === 'm') { r += chip(456, 15, 70, 'ON', false) + chip(534, 15, 70, 'OFF', true); }
		if (k === 'f') {
			r += P(pd(rect(330, 29, 190, 8)), { fill: C.line, fo: .2 }) + P(pd(rect(330, 29, 95, 8)), { fill: C.acc });
			r += P(pd(rect(418, 17, 16, 32)), { fill: C.gem, stroke: C.acc, sw: 3, edge: true });
			r += T(600, 33, '90°', { fs: 22, a: 'end' });
		}
		s += sk(x, y, r);
	});
	// The description strip, a layer further right, sliding the other way.
	const dy = 384, dx = lean(150, 60, dy) + 640;
	s += sk(dx, dy, strip(260, 222, { fill: 'rgb(30,34,40)', fo: .97, line: C.acc, sw: 3 }) + T(26, 30, 'RESUME', { fs: 17, ls: 4, fill: C.acc }));
	// Save & Quit: a red outlined slash at the bottom, its own layer.
	const qy = 650, qx = lean(150, 60, qy) + 470;
	s += sk(qx, qy, P(pd(rect(0, 0, 290, 62)), { fill: C.red, fo: .07, stroke: C.red, sw: 3.5, edge: true }) + T(145, 31, 'SAVE &amp; QUIT', { fs: 24, ls: 6, a: 'middle', fill: C.red }) +
		P(pd(rect(-30, 0, 12, 62)), { fill: C.red, fo: .6 }) + P(pd(rect(-52, 0, 8, 62)), { fill: C.red, fo: .35 }));
	let h = svg(1080, 932, s);
	h += at(dx - 44 * TANK + 24, dy + 52, 'Closes the menu and goes back to the game. Every change here is already saved.',
		'white-space:normal;width:220px;font-family:Chakra;font-size:18px;line-height:1.36;color:' + C.text + ';transform:skewX(-12deg);transform-origin:0 0');
	h += at(tx + 14 - 30 * TANK, ty + 13, cap('PGUP'));
	h += at(tx + 700 - 30 * TANK, ty + 13, cap('PGDN'));
	h += keys(100, 760, 760, [['ENTER', 'SELECT'], ['PGUP / PGDN', 'TABS'], ['ESC', 'RESUME']], 'justify-content:flex-start;padding-left:40px');
	return h;
}
function c2Menu() {
	seed = 5;
	let s = '';
	s += speed(0, 120, 30, 100, 8, false, { op: .5 });
	s += sk(70, 22, T(0, 40, 'REVENANT', { fs: 62, ls: 10, fill: '#eef8fb', out: 3 }) + P(pd(rect(4, 84, 120, 6)), { fill: C.acc }) + T(140, 87, 'TEROPA', { fs: 15, ls: 9, fill: C.cyan }));
	const items = [['01', 'SINGLE PLAYER'], ['02', 'MULTIPLAYER'], ['03', 'SETTINGS'], ['04', 'QUIT GAME']];
	items.forEach(([n, t], k) => {
		const y = 140 + k * 56, x = lean(80, 140, y), on = k === 0, off = k === 1;
		if (on) {
			s += speed(0, x - 4, y + 4, y + 46, 7, true, { th: 2.5 });
			s += sk(x, y, P(pd(rect(0, 0, 470, 50)), { fill: C.acc }) + P(pd(rect(482, 0, 14, 50)), { fill: C.acc, fo: .6 }) + P(pd(rect(506, 0, 7, 50)), { fill: C.acc, fo: .35 }) +
				T(20, 25, n, { fs: 15, ls: 2, fill: C.gem, op: .7 }) + T(60, 25, t, { fs: 29, ls: 6, fill: C.gem }));
		} else {
			s += sk(x, y, strip(k === 3 ? 330 : k === 1 ? 430 : 380, 50, { top: false, bottom: true, sw: 2, fo: .7 }) + T(20, 25, n, { fs: 15, ls: 2, fill: C.lab, op: off ? .45 : 1 }) +
				T(60, 25, t, { fs: 27, ls: 6, fill: C.text, op: off ? .42 : 1 }) +
				(off ? P(pd(rect(342, 13, 66, 24)), { stroke: C.lab, sw: 2, so: .7 }) + T(375, 25, 'SOON', { fs: 12, ls: 3, a: 'middle', fill: C.lab, op: .8 }) : ''));
		}
	});
	return svg(752, 420, s) + at(70, 376, kp('ENTER', 'SELECT'));
}
function c2Level() {
	seed = 21;
	let s = '';
	s += speed(0, 330, 70, 230, 16, false, { op: .55 }) + speed(0, 260, 100, 200, 7, true);
	// The level badge: the gem as a leaning plate.
	s += sk(170, 64, P(pd(rect(0, 0, 112, 112)), { fill: C.gem, stroke: C.acc, sw: 4.5, edge: true }) + P(pd(rect(9, 9, 94, 94)), { stroke: C.acc, sw: 1.5, so: .4 }) +
		P(pd([[0, 0], [112, 0], [0, 56]]), { fill: C.acc, fo: .1 }) + T(56, 58, '7', { fs: 78, a: 'middle', fill: C.acc }));
	s += sk(300, 56, T(0, 42, 'LEVEL UP', { fs: 74, ls: 8, out: 5 }));
	// The reward on a gold slash (a bar, not a glass panel), trailing two shards.
	s += sk(300, 142, P(pd(rect(0, 0, 330, 48)), { fill: C.acc }) + T(22, 24, 'MAX HEALTH +8%', { fs: 26, ls: 4, fill: C.gem }) +
		P(pd(rect(342, 0, 16, 48)), { fill: C.acc, fo: .65 }) + P(pd(rect(368, 0, 8, 48)), { fill: C.acc, fo: .35 }));
	return popShade(430, 140, 700, 300) + svg(752, 292, s);
}
const C2 = {
	num: 2, name: 'SLIPSTREAM',
	idea: 'Fast and forward: everything leans 12°, panels are layered strips that slide past each other, the choice is a solid gold slash trailing speed lines.',
	pause: c2Pause, menu: c2Menu, level: c2Level, barCss: 'transform:skewX(-12deg)',
	motion: ['Opens: {the strips shoot in from the right}, 35 ms apart, overshoot and settle.',
		'Move: {the gold slash whips to the row} (0.08 s); its speed lines fade in 0.2 s.',
		'Level up: the badge punches in, {LEVEL UP streaks in}, the gold slash wipes under.'],
	sound: 'open [UI.Open] + a whoosh · move [UI.Hover] · pick [UI.Menu.Select] · [UI.LevelUp]',
};

// ================================================================== 3 · HALO RETICLE
// The gun's sight as the motif: halo rings of ticks, range marks, rulers; the choice held in a reticle bracket that turns
// slowly; the level gem inside a ring of ticks.
function ticks(cx, cy, r1, r2, a0, a1, step, o = {}) {
	let s = '';
	for (let a = a0; a <= a1 + 1e-6; a += step) {
		const big = o.major && Math.abs(((a - (o.majorFrom ?? a0)) % o.major)) < 1e-6;
		const [x1, y1] = pt(cx, cy, r1, a), [x2, y2] = pt(cx, cy, big ? r2 + (o.majorLen ?? 8) : r2, a);
		s += '<line x1="' + x1.toFixed(1) + '" y1="' + y1.toFixed(1) + '" x2="' + x2.toFixed(1) + '" y2="' + y2.toFixed(1) + '" stroke="' + (o.col ?? C.line) + '" stroke-width="' + (big ? (o.bw ?? 2.4) : (o.w ?? 1.4)) + '" stroke-opacity="' + (big ? 1 : (o.op ?? .6)) + '"/>';
	}
	return s;
}
// The reticle bracket: a ring with four gaps and inward ticks, turned by rot degrees.
function reticle(cx, cy, r, rot, col = C.acc, sw = 3) {
	let s = '<g transform="rotate(' + rot + ' ' + cx + ' ' + cy + ')">';
	for (let q = 0; q < 4; q++) { s += P(arc(cx, cy, r, q * 90 + 14, q * 90 + 76), { stroke: col, sw, edge: true, cap: 'round' }); }
	for (let q = 0; q < 4; q++) { const [x1, y1] = pt(cx, cy, r + 5, q * 90), [x2, y2] = pt(cx, cy, r - 7, q * 90); s += L(x1, y1, x2, y2, { stroke: col, sw: 2.5, edge: true }); }
	return s + '</g><circle cx="' + cx + '" cy="' + cy + '" r="3.5" fill="' + col + '"/>';
}
// Arc brackets ( ) round a row: the bracket a selected row wears.
function rowBrackets(x, y, w, h, col = C.acc) {
	const r = h * .75, cyy = y + h / 2;
	const lx = x + r, rx = x + w - r;
	return P(arc(lx, cyy, r, 140, 220), { stroke: col, sw: 4, edge: true, cap: 'round' }) + P(arc(rx, cyy, r, -40, 40), { stroke: col, sw: 4, edge: true, cap: 'round' }) +
		L(x - r * .25 + 2, cyy, x + 14, cyy, { stroke: col, sw: 2.5, edge: true }) + L(x + w - 14, cyy, x + w + r * .25 - 2, cyy, { stroke: col, sw: 2.5, edge: true });
}
function ruler(x1, x2, y, step, major, o = {}) {
	let s = L(x1, y, x2, y, { stroke: o.col ?? C.line, sw: 1.6, so: o.so ?? .55 });
	for (let x = x1, i = 0; x <= x2 + .1; x += step, i++) { const big = i % major === 0; s += L(x, y, x, y + (o.up ? -1 : 1) * (big ? 12 : 5), { stroke: o.col ?? C.line, sw: big ? 2 : 1.2, so: big ? .85 : .45 }); }
	return s;
}
function c3Pause() {
	let s = '';
	const X = 120, Y = 70, W = 850, H = 750;
	s += P(pd(cham(X, Y, W, H, 22)), { fill: 'url(#glass)' }) + P(pd(cham(X, Y, W, H, 22)), { stroke: C.line, sw: 3, edge: true });
	// The halo on the corner: a ring of ticks, range marks, the pause bars, two gold arcs that turn.
	const hx = X + 86, hy = Y + 96;
	s += '<circle cx="' + hx + '" cy="' + hy + '" r="112" fill="rgb(18,21,26)" fill-opacity=".96"/>';
	s += P(arc(hx, hy, 112, 0, 359.9), { stroke: C.line, sw: 3, edge: true });
	s += ticks(hx, hy, 118, 126, 0, 354, 6, { major: 30, majorLen: 10 });
	[[200, '100'], [230, '200'], [260, '300'], [290, '400']].forEach(([a, t]) => { const [x, y] = pt(hx, hy, 152, a); s += T(x, y, t, { fs: 13, ls: 1, a: 'middle', fill: C.lab, out: 3 }); });
	s += P(arc(hx, hy, 98, -60, 0), { stroke: C.acc, sw: 4.5, edge: true, cap: 'round' }) + P(arc(hx, hy, 98, 120, 180), { stroke: C.acc, sw: 4.5, edge: true, cap: 'round' });
	s += L(hx - 86, hy, hx - 44, hy, { stroke: C.line, sw: 1.5, so: .6 }) + L(hx + 44, hy, hx + 86, hy, { stroke: C.line, sw: 1.5, so: .6 }) + L(hx, hy - 86, hx, hy - 44, { stroke: C.line, sw: 1.5, so: .6 }) + L(hx, hy + 44, hx, hy + 86, { stroke: C.line, sw: 1.5, so: .6 });
	s += P(pd(rect(hx - 20, hy - 30, 13, 60)), { fill: C.text }) + P(pd(rect(hx + 7, hy - 30, 13, 60)), { fill: C.text });
	// Title and a mil-dot rule.
	s += T(X + 260, Y + 64, 'GAME PAUSED', { fs: 40, ls: 9 });
	s += L(X + 260, Y + 108, X + W - 40, Y + 108, { stroke: C.line, sw: 1.6, so: .5 });
	for (let i = 0; i <= 13; i++) { const x = X + 260 + i * 42; s += i % 3 === 0 ? L(x, Y + 100, x, Y + 116, { stroke: C.line, sw: 2, so: .8 }) : '<circle cx="' + x + '" cy="' + (Y + 108) + '" r="3" fill="' + C.line + '" opacity=".7"/>'; }
	// Resume, the choice: the bracket ( ) round the row and a turning reticle at its left.
	const rx = X + 290, ry = Y + 150, rw = W - 340, rh = 66;
	s += P(pd(rect(rx, ry, rw, rh)), { fill: 'url(#goldwash)' });
	s += rowBrackets(rx, ry, rw, rh);
	s += reticle(rx + 56, ry + rh / 2, 17, 22);
	s += T(rx + rw / 2 + 20, ry + rh / 2 - 1, 'RESUME', { fs: 32, ls: 11, a: 'middle', fill: C.acc });
	// Tabs over a range scale; a gold pointer and the scale's gold run under the chosen one.
	const ty = Y + 286, t0 = X + 120, tw = 150;
	['GRAPHICS', 'AUDIO', 'INTERFACE', 'CONTROLS'].forEach((t, i) => { s += T(t0 + i * tw + tw / 2, ty, t, { fs: 18, ls: 3, a: 'middle', fill: i === 0 ? C.acc : C.lab }); });
	s += ruler(X + 40, X + W - 40, ty + 30, 10, 5);
	s += L(t0 + 6, ty + 30, t0 + tw - 6, ty + 30, { stroke: C.acc, sw: 4, edge: true });
	s += P(pd([[t0 + tw / 2 - 9, ty + 46], [t0 + tw / 2 + 9, ty + 46], [t0 + tw / 2, ty + 34]]), { fill: C.acc });
	// The rows: a target bullet, the label, the values; the description in a column with a vertical scale.
	const r0 = Y + 360, RH = 74, RX = X + 40, RW = 560;
	const chip = (x, y, w, t, on) => P(pd(cham(x, y - 18, w, 36, 7)), on ? { fill: C.acc, fo: .14, stroke: C.acc, sw: 3, edge: true } : { stroke: C.line, sw: 1.6, so: .3 }) +
		T(x + w / 2, y, t, { fs: 15, ls: 2, a: 'middle', fill: on ? C.acc : C.lab }) +
		(on ? L(x + w / 2, y - 25, x + w / 2, y - 18, { stroke: C.acc, sw: 2.5 }) + L(x + w / 2, y + 18, x + w / 2, y + 25, { stroke: C.acc, sw: 2.5 }) : '');
	['Quality', 'Motion blur', 'Field of view (1st person)'].forEach((t, i) => {
		const cy = r0 + RH * i + RH / 2;
		if (i % 2 === 0) { s += P(pd(rect(RX, r0 + RH * i + 4, RW, RH - 8)), { fill: C.line, fo: .05 }); }
		s += '<circle cx="' + (RX + 24) + '" cy="' + cy + '" r="9" fill="none" stroke="' + C.line + '" stroke-width="2" stroke-opacity=".7"/><circle cx="' + (RX + 24) + '" cy="' + cy + '" r="2.5" fill="' + C.line + '"/>';
		s += T(RX + 46, cy, t, { fs: i === 2 ? 20 : 22, fill: C.lab });
	});
	const cyR = (i) => r0 + RH * i + RH / 2, RE = RX + RW - 12;
	[['LOW', 58], ['MEDIUM', 92], ['HIGH', 66], ['EPIC', 66]].reduce((x, [t, w], j) => { s += chip(x, cyR(0), w, t, j === 1); return x + w + 6; }, RE - (58 + 92 + 66 + 66 + 18));
	s += chip(RE - 136, cyR(1), 64, 'ON', false) + chip(RE - 66, cyR(1), 66, 'OFF', true);
	// Field of view on a range scale (60 to 120), the knob a small reticle.
	const fx0 = RX + 316, fx1 = RX + 486, fy = cyR(2) + 4;
	s += ruler(fx0, fx1, fy, 8.5, 5, { up: true });
	s += L(fx0, fy, fx0 + (fx1 - fx0) * .5, fy, { stroke: C.acc, sw: 4, edge: true });
	s += '<circle cx="' + (fx0 + (fx1 - fx0) * .5) + '" cy="' + fy + '" r="11" fill="' + C.gem + '" stroke="' + C.acc + '" stroke-width="3"/>' + L(fx0 + (fx1 - fx0) * .5, fy - 17, fx0 + (fx1 - fx0) * .5, fy - 7, { stroke: C.acc, sw: 2.5 }) + '<circle cx="' + (fx0 + (fx1 - fx0) * .5) + '" cy="' + fy + '" r="2.6" fill="' + C.acc + '"/>';
	s += T(fx0, fy + 16, '60', { fs: 12, a: 'middle', fill: C.lab }) + T(fx1, fy + 16, '120', { fs: 12, a: 'middle', fill: C.lab });
	s += T(RE + 6, cyR(2), '90°', { fs: 21, a: 'end' });
	const DX = X + W - 228;
	s += L(DX - 14, r0 + 6, DX - 14, r0 + RH * 3 - 6, { stroke: C.line, sw: 1.6, so: .45 });
	for (let y = r0 + 6, i = 0; y <= r0 + RH * 3 - 6; y += 12, i++) { s += L(DX - 14, y, DX - 14 + (i % 4 === 0 ? 10 : 5), y, { stroke: C.line, sw: 1.2, so: .5 }); }
	s += T(DX + 4, r0 + 20, 'RESUME', { fs: 17, ls: 4, fill: C.acc });
	// Save & Quit: hollow red, its own corner brackets; a quarter halo scale on the far corner echoes the top one.
	const qx = X + W - 300, qy = Y + H - 104, qw = 256, qh = 60;
	s += P(pd(cham(qx, qy, qw, qh, 10)), { stroke: C.red, sw: 3.5, edge: true, fill: C.red, fo: .06 }) + T(qx + qw / 2, qy + qh / 2, 'SAVE &amp; QUIT', { fs: 23, ls: 6, a: 'middle', fill: C.red });
	s += P(arc(X + 40, Y + H - 40, 120, -90, 0), { stroke: C.line, sw: 1.6, so: .45 }) + ticks(X + 40, Y + H - 40, 120, 128, -90, 0, 6, { major: 30, op: .45 });
	let h = svg(1080, 932, s);
	h += at(DX + 4, r0 + 36, 'Closes the menu and goes back to the game. Every change here is already saved.', 'white-space:normal;width:200px;font-family:Chakra;font-size:18px;line-height:1.38;color:' + C.text);
	h += at(X + 40, ty - 16, cap('PGUP'));
	h += at(X + W - 106, ty - 16, cap('PGDN'));
	h += keys(X, Y + H + 26, W, [['ENTER', 'SELECT'], ['PGUP / PGDN', 'TABS'], ['ESC', 'RESUME']]);
	return h;
}
function c3Menu() {
	let s = '';
	// Teropa held in the sight: a ticked halo just outside the planet, a dashed range ring, the list on an arc round it.
	// The planet sits at (800, 250), radius ~346 (the menu picture at 0.9 scale); the halo has a gap where SOON sits.
	const pcx = 800, pcy = 250;
	for (const [a0, a1] of [[146, 180], [192, 206]]) { s += P(arc(pcx, pcy, 372, a0, a1), { stroke: C.line, sw: 2, so: .7, edge: true }) + ticks(pcx, pcy, 372, 380, a0, a1, 2, { major: 10, majorFrom: 0, majorLen: 8, op: .5 }); }
	s += P(arc(pcx, pcy, 404, 158, 202), { stroke: C.line, sw: 1.4, so: .4, dash: '3 7' });
	[[159, '200']].forEach(([a, t]) => { const [x, y] = pt(pcx, pcy, 394, a); s += T(x + 10, y, t, { fs: 11, ls: 1, fill: C.lab, op: .8, out: 3 }); });
	// Title: the mark inside a small ring of ticks.
	s += '<circle cx="58" cy="60" r="34" fill="rgb(18,21,26)" fill-opacity=".8" stroke="' + C.line + '" stroke-width="2.5"/>' + ticks(58, 60, 38, 44, 0, 350, 10, { major: 90, majorLen: 7, op: .55 });
	s += '<g transform="translate(43 40)"><path d="' + FLAME + '" fill="none" stroke="' + C.line + '" stroke-width="3"/><path d="' + FLAME_IN + '" fill="' + C.acc + '"/></g>';
	s += T(116, 54, 'REVENANT', { fs: 48, ls: 13, fill: '#eef8fb', out: 3 });
	s += ruler(118, 430, 86, 8, 5, { so: .6 }) + T(120, 110, 'TEROPA', { fs: 14, ls: 8, fill: C.cyan });
	// The list hangs on a range arc round Teropa: words to its left, numbers (range marks) to its right.
	const items = [['01', 'SINGLE PLAYER'], ['02', 'MULTIPLAYER'], ['03', 'SETTINGS'], ['04', 'QUIT GAME']];
	const R = 440;
	const ax = (y) => pcx - Math.sqrt(R * R - (y - pcy) * (y - pcy));
	let d = '';
	for (let y = 132; y <= 362; y += 5) { d += (d ? ' L' : 'M') + ax(y).toFixed(1) + ' ' + y; }
	s += P(d, { stroke: C.line, sw: 2, so: .55, edge: true });
	for (let y = 136; y <= 360; y += 8) { const x = ax(y); s += L(x, y, x + 5, y, { stroke: C.line, sw: 1.2, so: .45 }); }
	items.forEach(([n, t], k) => {
		const y = 158 + k * 60, x = ax(y), on = k === 0, off = k === 1;
		if (on) {
			let d2 = ''; for (let yy = y - 24; yy <= y + 24; yy += 4) { d2 += (d2 ? ' L' : 'M') + ax(yy).toFixed(1) + ' ' + yy; }
			s += P(d2, { stroke: C.acc, sw: 5, edge: true }) + reticle(x, y, 19, 20);
			s += L(x - 40, y, x - 26, y, { stroke: C.acc, sw: 2.5, edge: true });
		} else {
			s += L(x - 10, y, x + 10, y, { stroke: C.line, sw: 2.2, so: off ? .4 : .9 });
		}
		if (off) { s += P(pd(cham(x + 22, y - 12, 66, 24, 5)), { stroke: C.lab, sw: 2, so: .7 }) + T(x + 55, y, 'SOON', { fs: 12, ls: 3, a: 'middle', fill: C.lab, op: .8 }); }
		else { s += T(x + 30, y, n, { fs: 14, ls: 1, fill: on ? C.acc : C.lab, out: 3 }); }
		s += T(x - 48, y - 1, t, { fs: 28, ls: 6, a: 'end', fill: on ? C.acc : C.text, op: off ? .42 : 1, out: 3, outOp: .6 });
	});
	return svg(752, 420, s) + at(34, 376, kp('ENTER', 'SELECT'));
}
function c3Level() {
	let s = '';
	const cx = 376, cy = 112;
	// The gem inside a ring of 72 ticks with crosshair gaps, two gold arcs part-way round (the motion fills them).
	s += '<circle cx="' + cx + '" cy="' + cy + '" r="70" fill="none" stroke="#04060a" stroke-opacity=".6" stroke-width="6"/>' + P(arc(cx, cy, 70, 0, 359.9), { stroke: C.line, sw: 3 });
	s += ticks(cx, cy, 78, 86, 0, 355, 5, { major: 30, majorLen: 8, op: .8, w: 2 }).replace(/stroke="#d4eef4"/g, 'stroke="#d4eef4" style="filter:drop-shadow(0 0 1.5px #04060a)"');
	s += P(arc(cx, cy, 62, -80, 40), { stroke: C.acc, sw: 5, edge: true, cap: 'round' }) + P(arc(cx, cy, 62, 100, 220), { stroke: C.acc, sw: 5, edge: true, cap: 'round' });
	for (const a of [0, 180]) { const [x1, y1] = pt(cx, cy, 98, a), [x2, y2] = pt(cx, cy, 120, a); s += L(x1, y1, x2, y2, { stroke: C.line, sw: 3, edge: true }); }
	s += diamond(cx, cy, 42, T(cx, cy + 1, '7', { fs: 46, a: 'middle', fill: C.acc }), 4);
	// LEVEL ( 7 ) UP: the words either side of the sight, the reward on a range rule under it.
	s += T(cx - 132, cy, 'LEVEL', { fs: 60, ls: 9, a: 'end', out: 5 });
	s += T(cx + 132, cy, 'UP', { fs: 60, ls: 9, out: 5 });
	s += ruler(cx - 200, cx + 200, 224, 10, 5, { so: .9, col: C.line }).replace(/<path /g, '<path style="filter:drop-shadow(0 0 1.5px #04060a)" ');
	s += L(cx - 60, 224, cx + 60, 224, { stroke: C.acc, sw: 4, edge: true });
	s += T(cx, 258, 'MAX HEALTH +8%', { fs: 27, ls: 5, a: 'middle', fill: C.acc, out: 4 });
	return popShade(cx, 150, 720, 300) + svg(752, 292, s);
}
const C3 = {
	num: 3, name: 'HALO RETICLE',
	idea: 'The gun\'s sight as the motif: halo rings of ticks and range marks, rulers for rules, the choice held in a slowly turning reticle bracket.',
	pause: c3Pause, menu: c3Menu, level: c3Level,
	menuBg: 'background:#04060a url(\'' + IMG.menu + '\') -370px -160px/1683px auto',
	motion: ['Opens: {the halo spins up 90°} (0.25 s); its ticks light up clockwise.',
		'Move: {the reticle snaps to the row}, then turns slowly (8 s a turn); a pick closes it.',
		'Level up: {the gold arcs sweep round the gem}, then LEVEL and UP slide out.'],
	sound: 'open [UI.Open] · move [UI.Hover] (a dial click) · pick [UI.Menu.Select] · [UI.LevelUp]',
};

// ------------------------------------------------------------------ render
const css = '@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Regular.ttf\');font-weight:400}' +
	'@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Bold.ttf\');font-weight:700}' +
	'body{margin:0;background:#07080a}' +
	'.nh{font-family:Chakra;font-weight:700;font-size:14px;letter-spacing:4px;color:' + C.lab + '}' +
	'.li{padding-left:16px;text-indent:-16px}.li:before{content:"▸ ";color:' + C.acc + '}' +
	'.cap{display:inline-flex;align-items:center;justify-content:center;min-width:32px;height:32px;padding:0 10px;margin-right:10px;box-sizing:border-box;color:' + C.acc + ';font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:1px;box-shadow:inset 0 0 0 3px ' + C.acc + ',0 0 0 2px rgba(0,0,0,.6);border-radius:7px}' +
	'.kp{display:inline-flex;align-items:center;font-family:Chakra;font-weight:700;font-size:18px;letter-spacing:2px;color:#f3f3f3;text-shadow:' + OUTS + '}';

(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	for (const [i, cfg] of [[1, C1], [2, C2], [3, C3]]) {
		const html = '<!doctype html><html><head><meta charset="utf-8"><style>' + css + '</style></head><body>' + page(cfg) + '</body></html>';
		const file = path.join(here, 'c' + i + '.html');
		fs.writeFileSync(file, html);
		await pg.goto('file:///' + file.split(path.sep).join('/'));
		await pg.evaluate(() => document.fonts.ready);
		await pg.waitForTimeout(700);
		await pg.screenshot({ path: path.join(here, 'c' + i + '.png') });
		console.log('c' + i + '.png');
	}
	await browser.close();
})();
