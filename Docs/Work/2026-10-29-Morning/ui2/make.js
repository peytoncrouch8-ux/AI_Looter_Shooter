// UI round 2 mockup (2026-10-29 morning): the tag card's language on the pause menu, the main menu, the level-up and
// mission complete popups and the settings rows, in Slate & Salt. Mockup only. Run from the repo root:
//   node Docs/Work/2026-10-29-Morning/ui2/make.js  ->  ui2.html, ui2.png
// Words are the game's own (SettingsMenuWidget/Pages/Help, MainMenuWidget, HudLevelUpBannerWidget, HudMissionCompleteWidget).
const fs = require('fs');
const path = require('path');
const here = __dirname;
const F = 'file:///C:/Dev/AI_Looter_Shooter/';
const shot = F + 'Saved/Screenshots/Tools/maud_shotgun.png';
const menuShot = F + 'Saved/Screenshots/Tools/main_menu.png';

// Slate & Salt (LooterUI::Color::Slate*, SaltLine, SoulGold) and the kit colours that stay.
const C = { line: '#d4eef4', acc: '#ffe6a6', lab: '#b4bec8', text: '#f2f6f8', fl: '#ffd38a', icon: '#98a4ae',
	band: 'rgba(212,238,244,.06)', gem: '#0e1115', worse: '#ff7a6b', better: '#6dff7a', cyan: '#9fe0ff', amber: '#ffc06a', unc: '#7ce77c' };

let gid = 0;
const pd = (P) => 'M' + P.map(q => q[0].toFixed(1) + ' ' + q[1].toFixed(1)).join(' L') + ' Z';
// The tag: cut shoulders c, the pennant notch (nw wide at its floor, nd deep, ns slope), the toe-tag point p.
function tagPts(w, h, o = {}) {
	const c = o.c ?? 26, nw = o.nw || 0, nd = o.nd ?? 11, ns = o.ns ?? 12, p = o.p ?? 28;
	const P = [[0, c], [c, 0]];
	if (nw) { P.push([w / 2 - nw / 2 - ns, 0], [w / 2 - nw / 2, nd], [w / 2 + nw / 2, nd], [w / 2 + nw / 2 + ns, 0]); }
	P.push([w - c, 0], [w, c]);
	if (p) { P.push([w, h - p], [w / 2, h], [0, h - p]); } else { P.push([w, h], [0, h]); }
	return P;
}
// A button: the tag on its side (cut shoulders on the left, the point to the right).
const btnPts = (w, h, c = 10, p = 16) => [[c, 0], [w - p, 0], [w, h / 2], [w - p, h], [c, h], [0, h - c], [0, c]];
const rowPts = (w, h, c = 12) => [[0, 0], [w - c, 0], [w, c], [w, h], [0, h]];
const tabPts = (w, h, c = 9) => [[c, 0], [w - c, 0], [w, c], [w, h], [0, h], [0, c]];
const lozPts = (w, h, c) => [[c, 0], [w - c, 0], [w, h / 2], [w - c, h], [c, h], [0, h / 2]];
const diaPts = (r, cx, cy) => [[cx, cy - r], [cx + r, cy], [cx, cy + r], [cx - r, cy]];

// One shape as an SVG at (x, y): glass fill, an optional wash left to right, the line over a dark edge.
function shape(x, y, w, h, P, o = {}) {
	const id = 'g' + (gid++);
	const d = pd(P);
	let defs = '', fill = o.fill ?? 'glass';
	if (fill === 'glass') {
		defs += '<linearGradient id="' + id + '" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="rgb(30,34,40)" stop-opacity="' + (o.ga ?? .95) + '"/><stop offset="1" stop-color="rgb(18,21,26)" stop-opacity="' + (o.ga ?? .93) + '"/></linearGradient>';
		fill = 'url(#' + id + ')';
	}
	let wash = '';
	if (o.wash) {
		defs += '<linearGradient id="' + id + 'w" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="' + o.wash + '" stop-opacity="' + (o.washA ?? .3) + '"/><stop offset="' + (o.washTo ?? 1) + '" stop-color="' + o.wash + '" stop-opacity=".03"/></linearGradient>';
		wash = '<path d="' + d + '" fill="url(#' + id + 'w)"/>';
	}
	const sw = o.sw ?? 4;
	const edge = o.stroke && o.edge !== false ? '<path d="' + d + '" fill="none" stroke="#04060a" stroke-opacity=".65" stroke-width="' + (sw + 3) + '" stroke-linejoin="miter"/>' : '';
	const st = o.stroke ? '<path d="' + d + '" fill="none" stroke="' + o.stroke + '" stroke-opacity="' + (o.so ?? 1) + '" stroke-width="' + sw + '" stroke-linejoin="miter"/>' : '';
	const pad = 14;
	return '<svg style="position:absolute;left:' + (x - pad) + 'px;top:' + (y - pad) + 'px;overflow:visible" width="' + (w + 2 * pad) + '" height="' + (h + 2 * pad) +
		'" viewBox="' + (-pad) + ' ' + (-pad) + ' ' + (w + 2 * pad) + ' ' + (h + 2 * pad) + '"><defs>' + defs + '</defs>' + edge + '<path d="' + d + '" fill="' + fill + '"/>' + wash + st + '</svg>';
}
const at = (x, y, inner, css = '') => '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;white-space:nowrap;' + css + '">' + inner + '</div>';
const box = (x, y, w, h, inner, css = '') => '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;width:' + w + 'px;height:' + h + 'px;' + css + '">' + inner + '</div>';
const cap = (t, cls = 'cap') => '<span class="' + cls + '">' + t + '</span>';
const kp = (k, words) => '<span class="kp">' + cap(k) + words + '</span>';

// The level gem: a diamond, dark glass, gold edge, a fainter inner diamond, its lit top half.
function gem(cx, cy, r, inner, o = {}) {
	const s = r * 2 + 8;
	const c = s / 2;
	const top = [[c - r, c], [c, c - r], [c + r, c]];
	return '<div style="position:absolute;left:' + (cx - c) + 'px;top:' + (cy - c) + 'px;width:' + s + 'px;height:' + s + 'px;' + (o.css || '') + '">' +
		'<svg width="' + s + '" height="' + s + '" style="position:absolute;left:0;top:0;overflow:visible">' +
		'<path d="' + pd(diaPts(r, c, c)) + '" fill="' + C.gem + '" stroke="#04060a" stroke-opacity=".7" stroke-width="' + ((o.sw ?? 4) + 3) + '"/>' +
		'<path d="' + pd(top) + '" fill="' + C.acc + '" fill-opacity=".10"/>' +
		'<path d="' + pd(diaPts(r * 0.74, c, c)) + '" fill="none" stroke="' + C.acc + '" stroke-opacity=".35" stroke-width="1.6"/>' +
		'<path d="' + pd(diaPts(r, c, c)) + '" fill="none" stroke="' + C.acc + '" stroke-width="' + (o.sw ?? 4) + '"/></svg>' +
		'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center">' + inner + '</div></div>';
}
// Rays round a gem: four long and four short gold strokes, only those above the frame's line.
function rays(cx, cy, r, o = {}) {
	const L = [];
	const ways = [[0, -1, 1], [-1, 0, 1], [1, 0, 1], [-0.707, -0.707, 0], [0.707, -0.707, 0]];
	for (const [x, y, long] of ways) {
		const a = r + 10, b = r + (long ? 32 : 20);
		L.push('<line x1="' + (cx + x * a) + '" y1="' + (cy + y * a) + '" x2="' + (cx + x * b) + '" y2="' + (cy + y * b) + '" stroke="' + C.acc + '" stroke-width="3" stroke-opacity="' + (o.op ?? .8) + '" stroke-linecap="round"/>');
	}
	return '<svg style="position:absolute;left:0;top:0;overflow:visible" width="1" height="1">' + L.join('') + '</svg>';
}
const star = (cx, cy, R, r) => { const P = []; for (let i = 0; i < 10; i++) { const a = -Math.PI / 2 + i * Math.PI / 5, q = i % 2 ? r : R; P.push([cx + q * Math.cos(a), cy + q * Math.sin(a)]); } return P; };

// Settings parts: chips (n normal, h hover, c chosen), a slanted slider, a banded row, a tab.
function chip(x, y, w, h, t, st, dot) {
	const look = st === 'c' ? 'box-shadow:inset 0 0 0 3.5px ' + C.acc + ',0 0 0 1.5px rgba(0,0,0,.5);color:' + C.acc + ';background:rgba(255,230,166,.14)'
		: st === 'h' ? 'box-shadow:inset 0 0 0 3px ' + C.line + ',0 0 0 1.5px rgba(0,0,0,.5);color:' + C.text + ';background:rgba(212,238,244,.08)'
			: 'box-shadow:inset 0 0 0 2px rgba(212,238,244,.26);color:' + C.lab;
	const d = dot ? '<span style="display:inline-block;width:9px;height:9px;border-radius:50%;background:' + dot + ';box-shadow:0 0 0 2px #04060a;margin-right:7px"></span>' : '';
	return box(x, y, w, h, d + t, 'display:flex;align-items:center;justify-content:center;border-radius:6px;font-family:Chakra;font-weight:700;font-size:' + (dot ? 12 : h > 30 ? 14 : 12.5) + 'px;letter-spacing:' + (dot ? 1 : 1.5) + 'px;text-transform:uppercase;' + look);
}
function slider(x, y, w, frac) {
	const k = x + w * frac;
	return box(x, y - 3, w, 7, '', 'background:rgba(212,238,244,.18);transform:skewX(-24deg)') +
		box(x, y - 3, w * frac, 7, '', 'background:' + C.acc + ';transform:skewX(-24deg)') +
		'<svg style="position:absolute;left:' + (k - 13) + 'px;top:' + (y - 13) + 'px;overflow:visible" width="26" height="26"><path d="' + pd(diaPts(10, 13, 13)) + '" fill="' + C.gem + '" stroke="#04060a" stroke-opacity=".7" stroke-width="6"/><path d="' + pd(diaPts(10, 13, 13)) + '" fill="' + C.gem + '" stroke="' + C.acc + '" stroke-width="3.5"/><path d="' + pd(diaPts(3.5, 13, 13)) + '" fill="' + C.acc + '"/></svg>';
}
function row(x, y, w, h, label, st, band, fs = 15) {
	let s = '';
	if (band) { s += shape(x, y, w, h, rowPts(w, h), { fill: C.band }); }
	if (st === 'h') { s += shape(x, y, w, h, rowPts(w, h), { fill: 'none', stroke: C.line, sw: 2.5, so: .9, edge: false }); }
	if (st === 's') {
		s += shape(x, y, w, h, rowPts(w, h), { fill: 'none', wash: C.acc, washA: .2, washTo: .7, stroke: C.acc, sw: 3, edge: false });
		s += box(x + 9, y + 9, 5, h - 18, '', 'background:' + C.acc + ';transform:skewX(-16deg)');
	}
	s += at(x + 24, y, label, 'height:' + h + 'px;display:flex;align-items:center;font-family:Chakra;font-weight:700;font-size:' + fs + 'px;color:' + (st === 's' ? C.text : C.lab));
	return s;
}
function tab(x, y, w, h, t, sel, fs = 13) {
	let s = '';
	if (sel) {
		s += shape(x, y, w, h, tabPts(w, h), { stroke: C.line, sw: 3, ga: .9 });
	}
	s += box(x, y, w, h, t, 'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:' + fs + 'px;letter-spacing:3px;color:' + (sel ? C.acc : C.lab));
	return s;
}
// The rule under the tabs, with the selected tab's gold bar and its point (the notch that slides).
function tabRule(x, y, w, tx, tw) {
	return box(x, y, w, 2, '', 'background:rgba(212,238,244,.32)') +
		box(tx, y - 1, tw, 4, '', 'background:' + C.acc) +
		'<svg style="position:absolute;left:' + (tx + tw / 2 - 9) + 'px;top:' + (y + 2) + 'px;overflow:visible" width="18" height="9"><path d="M0 0 L18 0 L9 8 Z" fill="' + C.acc + '"/></svg>';
}
// A callout on a picture: a small note with a dashed leader to a dot.
function callout(x, y, tx, ty, t, w = 170) {
	const x2 = tx + (tx > x ? 0 : w), y2 = ty + 12;
	return '<svg style="position:absolute;left:0;top:0;overflow:visible" width="1" height="1"><line x1="' + x + '" y1="' + y + '" x2="' + x2 + '" y2="' + y2 + '" stroke="#ffffff" stroke-opacity=".75" stroke-width="1.5" stroke-dasharray="4 3"/><circle cx="' + x + '" cy="' + y + '" r="4" fill="#fff" stroke="#000" stroke-width="1.5"/></svg>' +
		box(tx, ty, w, 'auto', t, 'height:auto;padding:5px 9px;border-radius:6px;background:rgba(0,0,0,.72);box-shadow:inset 0 0 0 1px rgba(255,255,255,.25);font-family:Chakra;font-weight:700;font-size:14px;line-height:1.25;color:#fff');
}

// ---------------------------------------------------------------- 1 · Pause menu
function pausePanel() {
	const W = 680, H = 452;
	let s = shape(0, 0, W, H, tagPts(W, H, { c: 26, nw: 200, nd: 12, ns: 12, p: 22 }), { stroke: C.line });
	s += shape(W / 2 - 95, -17, 190, 36, lozPts(190, 36, 14), { stroke: C.line, sw: 3.5 });
	s += box(W / 2 - 95, -17, 190, 36, 'SETTINGS', 'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:16px;letter-spacing:6px;color:' + C.line);
	s += at(W - 66, 22, cap('X', 'capS'));
	s += at(30, 30, 'GAME PAUSED', 'font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:4px;color:' + C.lab);
	// Resume, selected: gold line, the gold filling in from the left.
	s += shape(30, 56, 620, 44, btnPts(620, 44, 10, 16), { ga: .6, wash: C.acc, washA: .34, washTo: .85, stroke: C.acc, edge: false });
	s += box(30, 56, 620, 44, 'RESUME', 'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:19px;letter-spacing:7px;color:' + C.acc);
	// Tabs.
	s += at(30, 121, cap('PGUP'));
	['GRAPHICS', 'AUDIO', 'INTERFACE', 'CONTROLS'].forEach((t, i) => { s += tab(102 + i * 120, 116, 114, 34, t, i === 0); });
	s += at(586, 121, cap('PGDN'));
	s += tabRule(30, 152, 620, 102, 114);
	// The Graphics page's three rows.
	const RW = 400;
	s += row(30, 170, RW, 46, 'Quality', 'n', true);
	['Low', 'Medium', 'High', 'Epic'].forEach((t, i) => { s += chip(160 + i * 67, 179, 64, 28, t, i === 1 ? 'c' : 'n'); });
	s += row(30, 224, RW, 46, 'Motion blur', 'n', false);
	['On', 'Off'].forEach((t, i) => { s += chip(300 + i * 66, 233, 62, 28, t, i === 1 ? 'c' : 'n'); });
	s += row(30, 278, RW, 46, 'Field of view (1st person)', 'n', true);
	s += slider(282, 301, 80, 0.5);
	s += at(372, 278, '90°', 'height:46px;display:flex;align-items:center;font-family:Chakra;font-weight:700;font-size:15px;color:' + C.text);
	// The description column: a rule, the gold title, the words (Resume's, as it has the focus).
	s += box(446, 170, 2, 154, '', 'background:rgba(212,238,244,.3)');
	s += box(462, 168, 190, 160, '<div style="white-space:normal"><div style="font-family:Chakra;font-weight:700;font-size:14px;letter-spacing:3px;color:' + C.acc + '">RESUME</div>' +
		'<div style="margin-top:6px;font-family:Chakra;font-size:14.5px;line-height:1.35;color:' + C.text + '">Closes the menu and goes back to the game. Every change here is already saved.</div></div>');
	// Save & Quit: hollow, the danger red.
	s += shape(446, 352, 204, 40, btnPts(204, 40, 10, 15), { fill: 'none', stroke: C.worse, sw: 3, edge: true });
	s += box(446, 352, 196, 40, 'SAVE &amp; QUIT', 'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:4px;color:' + C.worse);
	// The keys float under the panel, as under the gun card.
	s += box(0, H + 14, W, 30, kp('ENTER', 'SELECT') + kp('PGUP / PGDN', 'TABS') + kp('ESC', 'RESUME'), 'display:flex;justify-content:center;gap:24px');
	return s;
}

// ---------------------------------------------------------------- 2 · Main menu
function mainMenu() {
	let s = '<div style="position:absolute;inset:0;background:linear-gradient(90deg,rgba(4,6,9,.92) 0,rgba(4,6,9,.7) 38%,rgba(4,6,9,0) 66%)"></div>';
	// The mark: a gem with an ember flame (a stand-in for MainMenuArt::TitleMark), its glow breathing.
	s += box(18, 22, 140, 140, '', 'border-radius:50%;background:radial-gradient(circle,rgba(255,192,106,.42),rgba(255,192,106,0) 65%)');
	s += gem(88, 92, 34, '<svg width="30" height="40" viewBox="0 0 30 40"><path d="M15 2 C22 12 25 18 22 27 C20 33 17 36 15 38 C13 36 10 33 8 27 C5 18 8 12 15 2 Z" fill="none" stroke="' + C.line + '" stroke-width="3"/><path d="M15 16 C18 21 18 26 15 31 C12 26 12 21 15 16 Z" fill="' + C.amber + '"/></svg>');
	s += at(146, 52, 'REVENANT', 'font-family:Chakra;font-weight:700;font-size:64px;letter-spacing:17px;color:#e8f6fb;text-shadow:0 2px 10px rgba(0,0,0,.6)');
	s += box(150, 140, 78, 3, '', 'background:' + C.acc);
	s += at(240, 131, 'TEROPA', 'font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:10px;color:' + C.cyan);
	// The items: one gold tag marks the choice.
	const items = [['01', 'SINGLE PLAYER', 's'], ['02', 'MULTIPLAYER', 'd'], ['03', 'SETTINGS', 'n'], ['04', 'QUIT GAME', 'n']];
	const ys = [214, 272, 330, 402];
	s += shape(40, 214, 420, 48, btnPts(420, 48, 11, 20), { ga: .82, wash: C.acc, washA: .3, washTo: .9, stroke: C.acc });
	s += box(52, 226, 5, 24, '', 'background:' + C.acc + ';transform:skewX(-16deg)');
	items.forEach(([n, t, st], i) => {
		const y = ys[i], on = st === 's', dim = st === 'd';
		s += at(on ? 74 : 64, y, n, 'height:48px;display:flex;align-items:center;font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:2px;color:' + (on ? C.acc : C.lab) + ';opacity:' + (dim ? .5 : 1));
		s += at(on ? 108 : 98, y, t, 'height:48px;display:flex;align-items:center;font-family:Chakra;font-weight:700;font-size:27px;letter-spacing:6px;color:' + C.text + ';opacity:' + (dim ? .42 : 1) + ';' + (on ? '' : 'text-shadow:0 0 4px #000,0 0 8px rgba(0,0,0,.8)'));
		if (dim) { s += at(352, y + 13, '<span style="display:inline-flex;align-items:center;height:22px;padding:0 9px;border-radius:5px;box-shadow:inset 0 0 0 2px ' + C.lab + ';font-family:Chakra;font-weight:700;font-size:11px;letter-spacing:3px;color:' + C.lab + ';opacity:.8">SOON</span>'); }
	});
	s += at(64, 500, kp('ENTER', 'SELECT'));
	return s;
}

// ---------------------------------------------------------------- 3 · HUD popups
// The level-up banner, 380 wide; its gem's middle at (190, 50), the frame's top line through it. o: the motion's state.
function levelUp(o = {}) {
	const W = 380, H = 168, gy = 50;
	const fScale = o.frame ?? 1, gScale = o.gem ?? 1, tOp = o.text ?? 1, sp = o.spacing ?? 12;
	let s = box(-40, gy - 10, W + 80, H + 40, '', 'background:radial-gradient(ellipse at 50% 45%,rgba(4,6,9,.55),rgba(4,6,9,0) 68%);opacity:' + Math.min(1, fScale));
	let f = shape(0, 0, W, H, tagPts(W, H, { c: 22, nw: 92, nd: 16, ns: 14, p: 24 }), { fill: 'none', stroke: C.line });
	f += box(0, 58, W, 50, 'LEVEL UP', 'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:46px;letter-spacing:' + sp + 'px;padding-left:' + sp + 'px;box-sizing:border-box;color:' + C.text + ';opacity:' + tOp + ';text-shadow:' + OUT);
	f += box(0, 116, W, 26, 'MAX HEALTH +8%', 'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:19px;letter-spacing:5px;color:' + C.acc + ';opacity:' + (o.reward ?? 1) + ';transform:translateY(' + (o.rise ?? 0) + 'px);text-shadow:' + OUT);
	s += box(0, gy, W, H, f, 'transform-origin:50% 0;transform:scaleY(' + fScale + ')');
	if (o.rays !== false) { s += '<div style="position:absolute;left:0;top:0;opacity:' + (o.raysOp ?? 1) + '">' + rays(W / 2, gy, 46) + '</div>'; }
	if (o.ring) { s += box(W / 2 - 70, gy - 70, 140, 140, '', 'border-radius:50%;box-shadow:inset 0 0 0 3px rgba(255,230,166,.7),0 0 22px rgba(255,230,166,.5)'); }
	s += gem(W / 2, gy, 46, '<span style="font-family:Chakra;font-weight:700;font-size:40px;color:' + C.acc + ';text-shadow:0 0 10px rgba(255,230,166,.45)">7</span>', { css: 'transform:scale(' + gScale + ')', sw: 4.5 });
	return s;
}
function missionComplete() {
	const W = 460, H = 186, gy = 50;
	let s = box(-40, gy - 10, W + 80, H + 40, '', 'background:radial-gradient(ellipse at 50% 45%,rgba(4,6,9,.55),rgba(4,6,9,0) 68%)');
	s += shape(0, gy, W, H, tagPts(W, H, { c: 22, nw: 84, nd: 15, ns: 14, p: 24 }), { fill: 'none', stroke: C.line });
	s += rays(W / 2, gy, 40);
	s += gem(W / 2, gy, 40, '<svg width="40" height="40" viewBox="0 0 40 40" style="overflow:visible"><path d="' + pd(star(20, 21, 15, 6.4)) + '" fill="' + C.line + '" stroke="#04060a" stroke-width="1.5"/></svg>', { sw: 4.5 });
	s += box(0, gy + 50, W, 44, 'MISSION COMPLETE', 'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:31px;letter-spacing:7px;padding-left:7px;box-sizing:border-box;color:' + C.text + ';text-shadow:' + OUT);
	s += box(0, gy + 100, W, 30, 'COLD WELCOME', 'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:23px;letter-spacing:4px;color:' + C.fl + ';text-shadow:' + OUT);
	s += box(0, gy + 138, W, 26, '<span style="color:' + C.cyan + '">+1,000 XP</span><svg width="12" height="12" viewBox="0 0 12 12" style="margin:0 14px"><path d="M6 0 L12 6 L6 12 L0 6 Z" fill="' + C.acc + '"/></svg><span style="color:' + C.unc + '">UNCOMMON GUN</span>',
		'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:18px;letter-spacing:3px;text-shadow:' + OUT);
	return s;
}
const OUT = '0 0 2px #04060a,0 0 2px #04060a,0 0 3px #04060a,0 2px 6px rgba(0,0,0,.8)';
function keyframes() {
	const K = [['0.00 s', 'gem drops in', { gem: .35, frame: 0, text: 0, reward: 0, rays: false }],
		['0.10 s', 'gem pops 1.25, ring', { gem: 1.25, frame: .15, text: 0, reward: 0, ring: true, raysOp: .5 }],
		['0.22 s', 'tag unfolds 1.06', { gem: 1, frame: 1.06, text: .6, spacing: 20, reward: 0 }],
		['0.40 s', 'settled, reward rises', { gem: 1, frame: 1, text: 1, reward: 1 }]];
	return K.map(([t, w, o], i) => box(20 + i * 218, 0, 205, 136,
		'<div style="position:absolute;left:8px;top:6px;font-family:Chakra;font-weight:700;font-size:14px;color:' + C.acc + '">' + t + '</div>' +
		'<div style="position:absolute;right:8px;top:7px;font-family:Chakra;font-weight:700;font-size:13px;color:#fff">' + w + '</div>' +
		'<div style="position:absolute;left:30px;top:30px;transform:scale(.38);transform-origin:0 0">' + levelUp(o) + '</div>',
		'border-radius:6px;background:rgba(4,6,9,.55);box-shadow:inset 0 0 0 1px rgba(255,255,255,.14);overflow:hidden')).join('');
}

// ---------------------------------------------------------------- 4 · Settings rows
function settingsStrip() {
	const W = 860, H = 440;
	let s = shape(0, 0, W, H, tagPts(W, H, { c: 26, p: 0 }), { stroke: C.line });
	s += at(28, 31, cap('PGUP'));
	['GRAPHICS', 'AUDIO', 'INTERFACE', 'CONTROLS'].forEach((t, i) => { s += tab(104 + i * 140, 24, 132, 42, t, i === 2, 15); });
	s += at(672, 31, cap('PGDN'));
	s += tabRule(28, 70, 804, 384, 132);
	const RW = 548;
	s += row(28, 92, RW, 54, 'Minimap size', 'n', true, 17);
	s += slider(300, 119, 170, 0.33);
	s += at(490, 92, '100%', 'height:54px;display:flex;align-items:center;font-family:Chakra;font-weight:700;font-size:18px;color:' + C.text);
	s += row(28, 154, RW, 54, 'FPS counter', 'h', false, 17);
	['On', 'Off'].forEach((t, i) => { s += chip(400 + i * 74, 164, 68, 34, t, i === 1 ? 'c' : 'n'); });
	s += row(28, 216, RW, 54, 'Crosshair colour', 's', true, 17);
	[['White', C.text], ['Salt', C.line], ['Gold', C.amber], ['Cyan', C.cyan], ['Green', C.better]].forEach(([t, d], i) => { s += chip(214 + i * 72, 226, 68, 34, t, i === 0 ? 'c' : i === 2 ? 'h' : 'n', d); });
	// The description column, with a crosshair preview in the chosen colour (an idea).
	s += box(598, 92, 2, 178, '', 'background:rgba(212,238,244,.3)');
	s += box(614, 90, 222, 200, '<div style="white-space:normal"><div style="font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:3px;color:' + C.acc + '">CROSSHAIR COLOUR</div>' +
		'<div style="margin-top:6px;font-family:Chakra;font-size:15px;line-height:1.35;color:' + C.text + '">The colour of the crosshair\'s lines and dot, for every gun. Each colour keeps its dark edge, so it reads over bright sky and dark ground alike.</div>' +
		'<div style="margin-top:6px;font-family:Chakra;font-size:14px;color:' + C.lab + '">Default: White.</div></div>');
	const XH = (c) => ['M0 -22 V-8', 'M0 8 V22', 'M-22 0 H-8', 'M8 0 H22'].map(d => '<path d="' + d + '" stroke="#05080b" stroke-width="4.6" stroke-linecap="round" opacity=".85"/><path d="' + d + '" stroke="' + c + '" stroke-width="2.2" stroke-linecap="round"/>').join('');
	s += box(614, 300, 222, 112, '<svg style="position:absolute;left:50%;top:50%;transform:translate(-50%,-50%)" width="60" height="60" viewBox="-30 -30 60 60">' + XH(C.text) + '</svg>' +
		'<div style="position:absolute;left:8px;bottom:6px;font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:2px;color:#fff;text-shadow:' + OUT + '">PREVIEW</div>',
		'border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') -1180px -560px/2403px auto;box-shadow:inset 0 0 0 2px rgba(212,238,244,.3)');
	// The states, small.
	s += at(28, 296, 'STATES', 'font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:4px;color:' + C.lab);
	const lg = (x, y, t) => at(x, y, t, 'font-family:Chakra;font-weight:700;font-size:13px;color:#fff;letter-spacing:1px');
	s += row(28, 320, 170, 40, 'Row', 'n', true, 14) + lg(28, 364, 'normal: band');
	s += row(214, 320, 170, 40, 'Row', 'h', false, 14) + lg(214, 364, 'hover: salt line');
	s += row(400, 320, 176, 40, 'Row', 's', true, 14) + lg(400, 364, 'selected: gold, tick');
	s += chip(28, 392, 64, 30, 'Off', 'n') + chip(100, 392, 64, 30, 'Off', 'h') + chip(172, 392, 64, 30, 'Off', 'c');
	s += lg(248, 398, 'choice: normal · hover · chosen (gold)');
	return s;
}

// ---------------------------------------------------------------- The sheet
const notes = (motion, sound) => '<div class="nt"><div class="nh">MOTION</div>' + motion.map(m => '<div class="li">' + m + '</div>').join('') +
	'<div class="nh" style="margin-top:6px">SOUND</div><div class="li">' + sound + '</div></div>';
const g = (t) => '<b style="color:' + C.acc + '">' + t + '</b>';
const cue = (t) => '<span style="color:' + C.cyan + '">' + t + '</span>';
const panel = (x, y, ph, title, sub, pic, picCss, nts) => box(x, y, 900, 'auto',
	'<div class="h" style="font-size:22px">' + title + '</div><div class="note" style="margin-top:4px">' + sub + '</div>' +
	'<div style="position:relative;margin-top:10px;width:900px;height:' + ph + 'px;border-radius:6px;overflow:hidden;' + picCss + '">' + pic + '</div>' + nts, 'height:auto');

const P1 = panel(40, 112, 540, '1 · PAUSE MENU', 'Real items: Game paused, Resume, the four tabs, the page and its description, Save &amp; Quit, the keys.',
	'<div style="position:absolute;inset:0;background:rgba(5,7,10,.62)"></div>' + at(34, 34, pausePanel()) +
	callout(444, 18, 732, 10, 'title plate set in the notch, like the card\'s gem', 160) +
	callout(684, 90, 732, 78, 'selected: gold line, gold fills in from the left', 160) +
	callout(196, 188, 732, 186, 'gold bar + point under the tab slides', 160) +
	callout(684, 406, 732, 370, 'Save &amp; Quit: hollow, red', 160) +
	callout(374, 486, 732, 450, 'toe-tag point; keys float under, as under the card', 160),
	'background:url(\'' + shot + '\') -420px -330px/2403px auto',
	notes(['Opens: the panel ' + g('unfolds from its notch like the gun card') + ' (0.15 → 1.06 → 1, 0.14 s); the title plate drops in.',
		'Resume, tabs and rows ' + g('slide in 12 px from the left, 30 ms apart') + '. Closing folds it back into the notch.',
		'Selected button: ' + g('the gold fills in from the left') + ' (0.12 s), like the HOLD E cap.'],
		'open ' + cue('UI.Open') + ' + ' + cue('UI.Card.Open') + ' · move ' + cue('UI.Hover') + ' · Resume ' + cue('UI.Close') + ' · Save &amp; Quit ' + cue('UI.Click')));

const P2 = panel(980, 112, 540, '2 · MAIN MENU', 'Over Teropa. Real items: REVENANT, Teropa, Single Player, Multiplayer (Soon), Settings, Quit Game, Enter.',
	mainMenu() +
	callout(458, 238, 560, 186, 'one gold tag marks the choice; its point aims at Teropa', 175) +
	callout(124, 104, 330, 22, 'gem mark pops in, then breathes', 175) +
	callout(260, 426, 520, 410, 'Quit Game\'s tag turns red', 175) +
	callout(66, 300, 520, 300, 'others: floating words, no plate (simple)', 175),
	'background:#04060a url(\'' + menuShot + '\') -610px -50px/1870px auto',
	notes(['Arrives: ' + g('the gem mark pops') + ' (0 → 1.2 → 1) then breathes; REVENANT ' + g('closes up its spacing') + '; the gold rule draws out.',
		'The four items ' + g('slide in 24 px from the left, 30 ms apart') + '; the key hint last.',
		g('The gold tag slides to the next item') + ' (0.09 s, 4 px overshoot); the label steps 10 px right, the number goes gold.'],
		'arrival ' + cue('UI.Menu.Reveal') + ' · move ' + cue('UI.Hover') + ' · pick ' + cue('UI.Menu.Select') + ' (the tag flashes, the page slides away)'));

const P3 = panel(40, 874, 460, '3 · HUD POPUPS: LEVEL UP, MISSION COMPLETE', 'Real words: LEVEL UP, MAX HEALTH +8%; MISSION COMPLETE (or BOUNTY PAID), the mission, its rewards.',
	at(26, 20, levelUp()) + at(426, 20, missionComplete()) + at(0, 314, keyframes()),
	'background:url(\'' + shot + '\') -700px -200px/2403px auto',
	notes(['No glass behind HUD popups (the HUD rule): ' + g('4 px salt lines on a dark edge') + ', outlined words, a soft shade.',
		g('The gem drops into the notch') + ' and pops 1.25 → 1 with a ring; then ' + g('the tag unfolds down') + '; the reward rises 10 px.',
		'Holds ~2.2 s, folds back up into the gem. Mission: ' + g('rewards land one by one, 0.12 s apart') + '.'],
		'gem ' + cue('UI.LevelUp') + ' (with the XP bar, as now) + unfold ' + cue('UI.Card.Open') + ' · ' + cue('UI.MissionComplete') + ' + a tick per reward (new ' + cue('UI.Tag.Tick') + ')'));

const P4 = panel(980, 874, 460, '4 · SETTINGS ROWS (INTERFACE PAGE)', 'Real rows: Minimap size, FPS counter, the new Crosshair colour, and its real description.',
	'<div style="position:absolute;inset:0;background:rgba(5,7,10,.7)"></div>' + at(20, 10, settingsStrip()),
	'background:url(\'' + shot + '\') -300px -420px/2403px auto',
	notes(['Tab: ' + g('the gold bar and its point slide to the new tab') + ' (0.12 s); the rows slide in 14 px, 30 ms apart.',
		'Row: ' + g('the gold tick slides to the focused row') + ' (0.08 s), its wash fills left to right; mouse hover: salt line.',
		'Choice: ' + g('the gold outline jumps to the chosen chip') + ', which pops 1.08 → 1; the preview takes the colour.'],
		'tab ' + cue('UI.Tab') + ' · row ' + cue('UI.Hover') + ' · choice ' + cue('UI.Click') + ' · slider a quiet ' + cue('UI.Hover') + ' every 10%'));

const css = '@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Regular.ttf\');font-weight:400}' +
	'@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Bold.ttf\');font-weight:700}' +
	'body{margin:0;background:#0b0d10}.h{font-family:Chakra;font-weight:700;letter-spacing:3px;color:' + C.line + '}' +
	'.note{font-family:Chakra;font-size:16px;color:' + C.lab + '}' +
	'.nt{margin-top:12px;font-family:Chakra;font-size:16px;line-height:1.38;color:' + C.text + '}.nh{font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:4px;color:' + C.lab + '}' +
	'.li{padding-left:14px;text-indent:-14px}.li:before{content:"▸ ";color:' + C.acc + '}' +
	'.cap{display:inline-flex;align-items:center;justify-content:center;min-width:28px;height:27px;padding:0 9px;margin-right:9px;box-sizing:border-box;color:' + C.acc + ';font-family:Chakra;font-weight:700;font-size:13.5px;letter-spacing:1px;box-shadow:inset 0 0 0 3px ' + C.acc + ',0 0 0 2px rgba(0,0,0,.6);border-radius:6px}' +
	'.capS{display:inline-flex;align-items:center;justify-content:center;width:32px;height:30px;color:' + C.line + ';font-family:Chakra;font-weight:700;font-size:15px;box-shadow:inset 0 0 0 3px ' + C.line + ',0 0 0 2px rgba(0,0,0,.6);border-radius:6px}' +
	'.kp{display:inline-flex;align-items:center;font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:1.5px;color:#f3f3f3;text-shadow:0 0 3px #000,0 0 6px #000}';

const html = '<!doctype html><html><head><meta charset="utf-8"><style>' + css + '</style></head><body>' +
	'<div style="position:relative;width:1920px;height:1600px;overflow:hidden;background:radial-gradient(ellipse at 50% 15%,#1a1d22,#07080a 70%)">' +
	at(40, 28, 'UI ROUND 2 · THE TAG CARD ON EVERY SCREEN', 'font-family:Chakra;font-weight:700;font-size:32px;letter-spacing:4px;color:' + C.line) +
	at(40, 74, 'Slate &amp; Salt glass, bold 4 px salt lines, the pennant notch, cut shoulders and the toe-tag point. ' + g('Gold = selected.') + ' Hollow round-cornered keys. Real words from the game. Mockup only: nothing built yet.', 'font-family:Chakra;font-size:17px;color:' + C.lab) +
	P1 + P2 + P3 + P4 + '</div></body></html>';

fs.writeFileSync(path.join(here, 'ui2.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1600 } });
	await pg.goto('file:///' + path.join(here, 'ui2.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1200);
	await pg.screenshot({ path: path.join(here, 'ui2.png') });
	await browser.close();
	console.log('done');
})();
