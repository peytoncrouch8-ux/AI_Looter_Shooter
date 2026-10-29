// UI round 3, lane P2 (2026-10-29 morning): concepts 4-6 for the pause menu, the main menu list and the level-up popup.
// 4 Grave Marker, 5 Signal Glass, 6 Bounty Poster. Mockup only. Run from the repo root:
//   node Docs/Work/2026-10-29-Morning/ui3/make_b.js  ->  ui3/c4.png, c5.png, c6.png (and their .html)
// Words are the game's own (SettingsMenuWidget/Help, MainMenuWidget, HudLevelUpBannerWidget). Brief: ui3/BRIEF.md.
const fs = require('fs');
const path = require('path');
const here = __dirname;
const F = 'file:///C:/Dev/AI_Looter_Shooter/';
const IMG = {
	maud: F + 'Saved/Screenshots/Tools/maud_shotgun.png',      // 968x528, editor chrome round the viewport
	menu: F + 'Saved/Screenshots/Tools/main_menu.png',         // 1870x858, Teropa from orbit
	grave: F + 'Saved/Screenshots/Tools/grave_lantern.png',    // 1920x1040, the Ransom graves
	dusk: F + 'Saved/Screenshots/Tools/east_dusk_view.png',    // 1636x870, the east canyon at dusk
};

// Slate & Salt, fixed by the brief.
const C = { line: '#d4eef4', acc: '#ffe6a6', lab: '#b4bec8', text: '#f2f6f8', fl: '#ffd38a', icon: '#98a4ae',
	gem: '#0e1115', edge: '#04060a', red: '#ff7a6b', cyan: '#9fe0ff' };

// ---------- shared drawing helpers (frame-local px) ----------
const pd = (P, open) => 'M' + P.map(q => (+q[0]).toFixed(1) + ' ' + (+q[1]).toFixed(1)).join(' L') + (open ? '' : ' Z');
// A shape: dark edge under the line, fill, the line. o: fill ('glass' | colour | none), st, sw, so, edge, open, dash, fo.
function poly(P, o = {}) {
	const d = pd(P, o.open);
	const sw = o.sw ?? 4;
	let s = '';
	const fill = o.fill === 'glass' ? 'url(#glass)' : (o.fill || 'none');
	if (o.st && o.edge !== false) { s += '<path d="' + d + '" fill="none" stroke="' + (o.edgeSt || C.edge) + '" stroke-opacity="' + (o.eo ?? .7) + '" stroke-width="' + (sw + 3.5) + '" stroke-linejoin="miter"' + (o.dash ? ' stroke-dasharray="' + o.dash + '"' : '') + '/>'; }
	if (fill !== 'none') { s += '<path d="' + d + '" fill="' + fill + '"' + (o.fo != null ? ' fill-opacity="' + o.fo + '"' : '') + '/>'; }
	if (o.st) { s += '<path d="' + d + '" fill="none" stroke="' + o.st + '" stroke-opacity="' + (o.so ?? 1) + '" stroke-width="' + sw + '" stroke-linejoin="miter"' + (o.dash ? ' stroke-dasharray="' + o.dash + '"' : '') + '/>'; }
	return s;
}
// A carved groove: a dark cut with a faint salt lip just under it.
function carved(P, o = {}) {
	const d = pd(P, o.open);
	return '<path d="' + d + '" fill="none" stroke="#000" stroke-opacity=".7" stroke-width="' + (o.w ?? 3) + '"/>' +
		'<path d="' + d + '" transform="translate(0 1.6)" fill="none" stroke="' + (o.c || C.line) + '" stroke-opacity="' + (o.op ?? .42) + '" stroke-width="1.4"/>';
}
const ln = (x1, y1, x2, y2, c, w, op = 1, extra = '') => '<line x1="' + x1 + '" y1="' + y1 + '" x2="' + x2 + '" y2="' + y2 + '" stroke="' + c + '" stroke-width="' + w + '" stroke-opacity="' + op + '" ' + extra + '/>';
const rect = (x, y, w, h, c, op = 1, extra = '') => '<rect x="' + x + '" y="' + y + '" width="' + w + '" height="' + h + '" fill="' + c + '" fill-opacity="' + op + '" ' + extra + '/>';
const dia = (cx, cy, r, ry) => [[cx, cy - (ry ?? r)], [cx + r, cy], [cx, cy + (ry ?? r)], [cx - r, cy]];
const ell = (cx, cy, rx, ry, fill) => '<ellipse cx="' + cx + '" cy="' + cy + '" rx="' + rx + '" ry="' + ry + '" fill="' + fill + '"/>';

// Outlined text for HUD pieces (no glass): a dark ring and a soft shade.
const ring = (r = 2, c = C.edge) => [[r, 0], [-r, 0], [0, r], [0, -r], [r, r], [-r, -r], [r, -r], [-r, r]].map(([a, b]) => a + 'px ' + b + 'px 0 ' + c).join(',') + ',0 0 ' + (r * 5) + 'px rgba(0,0,0,.75)';
const soft = '0 2px 6px rgba(0,0,0,.85),0 0 2px #000';
// Text at (x, y top). o: s size, w weight, c colour, ls spacing, al 'l'|'c'|'r', wd wrap width, lh, sh, css.
function T(x, y, s, o = {}) {
	const ls = o.ls ?? 0;
	let pos;
	if (o.wd) { pos = 'left:' + x + 'px;width:' + o.wd + 'px;white-space:normal;text-align:' + (o.al === 'c' ? 'center' : 'left') + ';'; }
	else if (o.al === 'c') { pos = 'left:' + (x - 700) + 'px;width:1400px;text-align:center;white-space:nowrap;padding-left:' + ls + 'px;box-sizing:border-box;'; }
	else if (o.al === 'r') { pos = 'left:' + (x - 1400) + 'px;width:1400px;text-align:right;white-space:nowrap;margin-right:' + (-ls) + 'px;'; }
	else { pos = 'left:' + x + 'px;white-space:nowrap;'; }
	return '<div style="position:absolute;top:' + y + 'px;' + pos + 'font-family:Chakra;font-weight:' + (o.w ?? 700) + ';font-size:' + (o.s ?? 16) + 'px;letter-spacing:' + ls + 'px;line-height:' + (o.lh ?? 1.15) + ';color:' + (o.c ?? C.text) + ';' + (o.sh ? 'text-shadow:' + o.sh + ';' : '') + (o.css ?? '') + '">' + s + '</div>';
}
// Hollow rounded keys under panels (the user's taste), a row of them.
function keys(x, y, list, o = {}) {
	const kc = o.c ?? C.acc;
	const cap = (k) => '<span style="display:inline-flex;align-items:center;justify-content:center;min-width:30px;height:29px;padding:0 10px;margin-right:10px;box-sizing:border-box;color:' + kc + ';font-family:Chakra;font-weight:700;font-size:14px;letter-spacing:1px;box-shadow:inset 0 0 0 3px ' + kc + ',0 0 0 2px rgba(0,0,0,.6);border-radius:7px;text-shadow:none">' + k + '</span>';
	const items = list.map(([k, w]) => '<span style="display:inline-flex;align-items:center;margin-right:30px;font-family:Chakra;font-weight:700;font-size:16px;letter-spacing:2px;color:' + C.text + ';text-shadow:0 0 3px #000,0 0 7px #000">' + cap(k) + w + '</span>').join('');
	return '<div style="position:absolute;top:' + y + 'px;' + (o.center ? 'left:' + (x - 600) + 'px;width:1200px;justify-content:center;' : 'left:' + x + 'px;') + 'display:flex;white-space:nowrap">' + items + '</div>';
}

const DEFS = '<defs>' +
	'<linearGradient id="glass" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="rgb(30,34,40)" stop-opacity=".95"/><stop offset="1" stop-color="rgb(18,21,26)" stop-opacity=".93"/></linearGradient>' +
	'<radialGradient id="lantern"><stop offset="0" stop-color="#ffd38a" stop-opacity=".55"/><stop offset=".45" stop-color="#ffd38a" stop-opacity=".2"/><stop offset="1" stop-color="#ffd38a" stop-opacity="0"/></radialGradient>' +
	'<radialGradient id="soul"><stop offset="0" stop-color="#fff8e4" stop-opacity="1"/><stop offset=".18" stop-color="#ffe6a6" stop-opacity=".75"/><stop offset=".5" stop-color="#ffd38a" stop-opacity=".2"/><stop offset="1" stop-color="#ffd38a" stop-opacity="0"/></radialGradient>' +
	'<radialGradient id="shade"><stop offset="0" stop-color="#000" stop-opacity=".5"/><stop offset="1" stop-color="#000" stop-opacity="0"/></radialGradient>' +
	'<linearGradient id="fadeSalt" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="' + C.line + '" stop-opacity="1"/><stop offset=".6" stop-color="' + C.line + '" stop-opacity=".9"/><stop offset="1" stop-color="' + C.line + '" stop-opacity="0"/></linearGradient>' +
	'<linearGradient id="fadeEdge" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="' + C.edge + '" stop-opacity=".75"/><stop offset=".6" stop-color="' + C.edge + '" stop-opacity=".6"/><stop offset="1" stop-color="' + C.edge + '" stop-opacity="0"/></linearGradient>' +
	'</defs>';

// One piece of the sheet: a game shot behind (bg css), a darkening layer, the vector layer and the text layer.
function frame(x, y, w, h, o) {
	return '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;width:' + w + 'px;height:' + h + 'px;overflow:hidden;border-radius:4px;box-shadow:0 0 0 1px rgba(212,238,244,.12)">' +
		'<div style="position:absolute;inset:-12px;background:' + o.bg + ';' + (o.filter ? 'filter:' + o.filter + ';' : '') + '"></div>' +
		'<div style="position:absolute;inset:0;background:' + (o.dim ?? 'rgba(6,8,11,.55)') + '"></div>' +
		'<svg width="' + w + '" height="' + h + '" style="position:absolute;left:0;top:0;overflow:visible">' + DEFS + (o.svg || '') + '</svg>' +
		(o.html || '') +
		T(w - 12, 10, '<span style="display:inline-block;background:rgba(4,6,10,.72);padding:4px 6px 4px 10px;box-shadow:0 0 0 1px rgba(212,238,244,.25)">' + o.label + '</span>', { s: 12, ls: 4, c: C.line, al: 'r' }) +
		'</div>';
}
// The sheet: a title strip, the three pieces, the motion and sound notes.
function sheet(n, name, idea, L, MM, LU, motion, sound, titleCss = '') {
	const g = (t) => '<span style="color:' + C.acc + '">' + t + '</span>';
	const list = (a) => a.map(t => '<div style="padding-left:16px;text-indent:-16px"><span style="color:' + C.acc + '">▸ </span>' + t + '</div>').join('');
	const cue = (t) => '<span style="color:' + C.cyan + '">' + t + '</span>';
	const fmt = (t) => t.replace(/\{([^}]+)\}/g, (m, a) => g(a)).replace(/\[([^\]]+)\]/g, (m, a) => cue(a));
	return '<div style="position:relative;width:1920px;height:1080px;overflow:hidden;background:radial-gradient(ellipse at 50% 10%,#1a1d22,#07080a 75%)">' +
		'<div style="position:absolute;left:0;top:0;width:1920px;height:94px;background:linear-gradient(180deg,rgba(4,6,10,.9),rgba(4,6,10,.55));border-bottom:1px solid rgba(212,238,244,.22)"></div>' +
		T(40, 16, n + ' · ' + name, { s: 38, ls: 6, c: C.line, css: titleCss }) +
		T(42, 62, idea, { s: 18, w: 400, c: C.lab }) +
		T(1880, 22, 'UI ROUND 3 · CONCEPT ' + n + ' OF 6', { s: 15, ls: 4, c: C.lab, al: 'r' }) +
		T(1880, 50, g('gold = selected') + ' · mockup only', { s: 15, ls: 2, w: 400, c: C.lab, al: 'r' }) +
		L + MM + LU +
		T(40, 938, 'MOTION', { s: 13, ls: 5, c: C.lab }) +
		'<div style="position:absolute;left:40px;top:960px;width:1050px;font-family:Chakra;font-size:17px;line-height:1.42;color:' + C.text + '">' + list(motion.map(fmt)) + '</div>' +
		T(1116, 938, 'SOUND', { s: 13, ls: 5, c: C.lab }) +
		'<div style="position:absolute;left:1116px;top:960px;width:770px;font-family:Chakra;font-size:17px;line-height:1.42;color:' + C.text + '">' + list(sound.map(fmt)) + '</div>' +
		'</div>';
}
const FL = [32, 108, 1064, 816], FM = [1116, 108, 772, 440], FU = [1116, 564, 772, 360];
const menuBg = 'url(\'' + IMG.menu + '\') -560px -136px/1403px auto';
const DESC = 'Closes the menu and goes back to the game. Every change here is already saved.';

// =====================================================================================================
// 4 · GRAVE MARKER
// =====================================================================================================
function c4() {
	// --- pause: the tall headstone (gothic shoulders cut sharp) on its plinth, the epitaph stone beside it.
	let s = '', h = '';
	s += rect(0, 744, 1064, 72, '#000', .38) + ln(0, 744, 1064, 744, C.line, 1.5, .28);
	const stone = [[70, 230], [98, 140], [180, 66], [370, 36], [560, 66], [642, 140], [670, 230], [670, 700], [694, 700], [694, 744], [46, 744], [46, 700], [70, 700]];
	s += poly(stone, { fill: 'glass', st: C.line, sw: 4 });
	s += rect(46, 700, 648, 44, '#000', .28) + ln(46, 700, 694, 700, C.line, 3, .9);
	s += carved([[88, 236], [114, 152], [190, 86], [370, 58], [550, 86], [626, 152], [652, 236], [652, 682], [88, 682]]);
	// soul-light emblem in the arch
	s += ell(370, 104, 60, 44, 'url(#soul)') + poly(dia(370, 104, 15, 19), { fill: C.gem, st: C.acc, sw: 3 }) + poly(dia(370, 104, 5, 7), { fill: '#fff6dc' });
	h += T(370, 128, 'GAME PAUSED', { s: 32, ls: 9, c: C.line, al: 'c', sh: soft });
	s += ln(214, 186, 350, 186, C.line, 1.5, .5) + ln(390, 186, 526, 186, C.line, 1.5, .5) + poly(dia(370, 186, 6), { st: C.line, sw: 1.5, so: .7, edge: false });
	// Resume: lit like a lantern from behind
	s += ell(370, 246, 250, 48, 'url(#lantern)');
	const lamp = (cx, cy) => poly([[cx - 7, cy - 6], [cx, cy - 13], [cx + 7, cy - 6], [cx + 7, cy + 8], [cx, cy + 13], [cx - 7, cy + 8]], { st: C.acc, sw: 2.2 }) + poly(dia(cx, cy + 1, 3.2, 6), { fill: '#fff3cf' });
	s += lamp(212, 246) + lamp(528, 246);
	h += T(370, 224, 'RESUME', { s: 38, ls: 10, c: C.acc, al: 'c', sh: '0 0 14px rgba(255,211,138,.75),0 0 2px #000' });
	s += ln(262, 284, 478, 284, C.acc, 2, .55);
	// tabs: the open page sits in a little pointed arch
	const tabs = ['GRAPHICS', 'AUDIO', 'INTERFACE', 'CONTROLS'];
	tabs.forEach((t, i) => {
		const x = 110 + i * 130;
		if (i === 0) s += poly([[x, 346], [x, 320], [x + 22, 308], [x + 65, 300], [x + 108, 308], [x + 130, 320], [x + 130, 346]], { open: true, st: C.line, sw: 2.5 });
		h += T(x + 65, 318, t, { s: 14, ls: 3, c: i === 0 ? C.acc : C.lab, al: 'c' });
	});
	s += carved([[100, 352], [640, 352]], { open: true });
	// three rows
	const rowY = [396, 456, 516];
	h += T(110, rowY[0] - 11, 'Quality', { s: 17, c: C.lab }) + T(110, rowY[1] - 11, 'Motion blur', { s: 17, c: C.lab }) + T(110, rowY[2] - 11, 'Field of view (1st person)', { s: 17, c: C.lab });
	const archChip = (x1, x2, cy) => poly([[x1, cy + 15], [x1, cy - 7], [x1 + 10, cy - 13], [(x1 + x2) / 2, cy - 17], [x2 - 10, cy - 13], [x2, cy - 7], [x2, cy + 15]], { st: C.acc, sw: 2.5 });
	const opt = (cx, cy, t, on) => T(cx, cy - 8, t, { s: 13, ls: 2, c: on ? C.acc : C.lab, al: 'c', css: on ? '' : 'opacity:.8' });
	s += archChip(434, 512, rowY[0]);
	h += opt(404, rowY[0], 'LOW') + opt(473, rowY[0], 'MEDIUM', 1) + opt(550, rowY[0], 'HIGH') + opt(610, rowY[0], 'EPIC');
	s += archChip(582, 638, rowY[1]);
	h += opt(550, rowY[1], 'ON') + opt(610, rowY[1], 'OFF', 1);
	s += ln(400, rowY[2], 562, rowY[2], '#000', 6, .7) + ln(400, rowY[2], 562, rowY[2], C.line, 2.5, .3) + ln(400, rowY[2], 504, rowY[2], C.acc, 3);
	s += poly(dia(504, rowY[2], 8, 10), { fill: C.gem, st: C.acc, sw: 2.5 });
	h += T(580, rowY[2] - 11, '90°', { s: 17, c: C.text });
	s += carved([[110, 426], [630, 426]], { open: true, op: .22 }) + carved([[110, 486], [630, 486]], { open: true, op: .22 });
	// Save & Quit, carved in red
	h += T(370, 578, 'SAVE &amp; QUIT', { s: 21, ls: 7, c: C.red, al: 'c' });
	s += ln(150, 592, 244, 592, C.red, 2, .6) + ln(496, 592, 590, 592, C.red, 2, .6);
	s += poly(dia(370, 646, 5), { st: C.line, sw: 1.5, so: .5, edge: false });
	h += T(370, 714, 'RANSOM\'S REST', { s: 13, ls: 8, c: C.lab, al: 'c', css: 'opacity:.8' });
	h += keys(370, 768, [['ENTER', 'SELECT'], ['PGUP / PGDN', 'TABS'], ['ESC', 'RESUME']], { center: 1 });
	// the epitaph stone: the description of what the cursor rests on
	s += poly([[740, 700], [770, 236], [880, 150], [990, 236], [1020, 700]], { fill: 'glass', st: C.line, sw: 4 });
	s += poly([[724, 700], [1036, 700], [1036, 744], [724, 744]], { fill: 'glass', st: C.line, sw: 3 });
	s += carved([[758, 682], [786, 246], [880, 176], [974, 246], [1002, 682]], { open: true });
	s += ell(880, 226, 34, 26, 'url(#lantern)') + poly(dia(880, 226, 7, 10), { st: C.acc, sw: 2 });
	h += T(880, 296, 'RESUME', { s: 19, ls: 6, c: C.acc, al: 'c' });
	h += T(784, 336, DESC, { wd: 192, s: 16, w: 400, lh: 1.5, c: C.text, al: 'c' });
	s += ln(820, 496, 940, 496, C.line, 1.5, .35);
	h += T(784, 518, 'Point at a setting to see what it does.', { wd: 192, s: 14, w: 400, lh: 1.45, c: C.lab, al: 'c' });
	const lampBig = (cx, cy) => ln(cx, cy - 40, cx, cy - 22, C.line, 2, .6) + poly([[cx - 14, cy - 12], [cx, cy - 22], [cx + 14, cy - 12], [cx + 14, cy + 16], [cx, cy + 24], [cx - 14, cy + 16]], { st: C.acc, sw: 2.5 }) + poly(dia(cx, cy + 2, 5, 10), { fill: '#fff3cf' });
	s += ell(880, 628, 70, 60, 'url(#lantern)') + lampBig(880, 628);
	const L = frame(...FL, { bg: 'url(\'' + IMG.grave + '\') -470px -150px/1920px auto', dim: 'linear-gradient(180deg,rgba(6,8,11,.62),rgba(6,8,11,.78))', filter: 'blur(2px)', svg: s, html: h, label: 'PAUSE MENU' });

	// --- main menu: one stone holds the list; the chosen name is lantern-lit.
	s = ''; h = '';
	s += poly([[40, 110], [62, 60], [110, 32], [220, 20], [330, 32], [378, 60], [400, 110], [400, 460], [40, 460]], { fill: 'glass', st: C.line, sw: 4 });
	s += carved([[56, 460], [56, 114], [76, 70], [118, 46], [220, 36], [322, 46], [364, 70], [384, 114], [384, 460]], { open: true });
	s += ell(220, 64, 46, 34, 'url(#soul)') + poly(dia(220, 64, 12, 15), { fill: C.gem, st: C.acc, sw: 2.5 }) + poly(dia(220, 64, 4, 5.5), { fill: '#fff6dc' });
	h += T(220, 86, 'REVENANT', { s: 36, ls: 8, c: C.line, al: 'c' });
	h += T(220, 132, 'TEROPA', { s: 12, ls: 7, c: C.acc, al: 'c' });
	s += ln(110, 140, 168, 140, C.acc, 1.5, .6) + ln(272, 140, 330, 140, C.acc, 1.5, .6);
	const items = [['01', 'SINGLE PLAYER'], ['02', 'MULTIPLAYER'], ['03', 'SETTINGS'], ['04', 'QUIT GAME']];
	const glyph = (cx, cy, lit) => poly([[cx - 8, cy + 11], [cx - 8, cy - 4], [cx - 4, cy - 9], [cx, cy - 11], [cx + 4, cy - 9], [cx + 8, cy - 4], [cx + 8, cy + 11]], lit ? { fill: C.acc, st: C.acc, sw: 1.5 } : { st: C.line, sw: 2, so: .6 });
	items.forEach(([n, t], i) => {
		const cy = 190 + i * 50;
		const sel = i === 0, dim = i === 1;
		if (sel) s += ell(214, cy, 180, 30, 'url(#lantern)');
		s += glyph(80, cy, sel);
		h += T(100, cy - 7, n, { s: 12, c: sel ? C.acc : C.lab, css: dim ? 'opacity:.45' : '' });
		h += T(126, cy - 13, t, { s: dim ? 20 : 22, ls: dim ? 3 : 4, c: sel ? C.acc : dim ? C.lab : C.text, sh: sel ? '0 0 12px rgba(255,211,138,.7)' : '', css: dim ? 'opacity:.45' : '' });
	});
	h += T(316, 234, 'SOON', { s: 10, ls: 2, c: C.acc, css: 'padding:2px 5px;box-shadow:inset 0 0 0 1.5px ' + C.acc + ';border-radius:3px;opacity:.85' });
	h += keys(78, 392, [['ENTER', 'SELECT']]);
	const MM = frame(...FM, { bg: menuBg, dim: 'linear-gradient(90deg,rgba(6,8,11,.5),rgba(6,8,11,0) 70%)', svg: s, html: h, label: 'MAIN MENU' });

	// --- level-up: an outline headstone rises, the level gem burns in its arch like a soul-light. No glass.
	s = ''; h = '';
	s += ell(386, 170, 300, 160, 'url(#shade)');
	const out = [[186, 340], [186, 128], [212, 82], [290, 46], [386, 30], [482, 46], [560, 82], [586, 128], [586, 340]];
	s += '<path d="' + pd(out, 1) + '" fill="none" stroke="url(#fadeEdge)" stroke-width="7.5" stroke-linejoin="miter"/><path d="' + pd(out, 1) + '" fill="none" stroke="url(#fadeSalt)" stroke-width="4" stroke-linejoin="miter"/>';
	s += '<path d="' + pd([[204, 340], [204, 134], [226, 94], [298, 62], [386, 48], [474, 62], [546, 94], [568, 134], [568, 340]], 1) + '" fill="none" stroke="url(#fadeSalt)" stroke-opacity=".35" stroke-width="1.5"/>';
	s += ell(386, 100, 130, 96, 'url(#soul)');
	for (const d of ['M386 62 Q374 40 390 10', 'M370 70 Q348 48 360 18', 'M402 70 Q424 46 412 16']) s += '<path d="' + d + '" fill="none" stroke="#fff3cf" stroke-opacity=".55" stroke-width="1.6" stroke-linecap="round"/>';
	for (const [x, y, r] of [[350, 26, 2.2], [424, 34, 1.8], [398, 8, 1.6], [336, 50, 1.4]]) s += '<circle cx="' + x + '" cy="' + y + '" r="' + r + '" fill="#ffe6a6"/>';
	s += poly(dia(386, 100, 30, 36), { fill: C.gem, st: C.acc, sw: 4 }) + poly(dia(386, 100, 22, 27), { st: C.acc, sw: 1.5, so: .4, edge: false });
	h += T(386, 76, '7', { s: 42, c: '#fff6dc', al: 'c', sh: '0 0 12px rgba(255,230,166,.9),0 0 3px #000' });
	h += T(386, 156, 'LEVEL UP', { s: 50, ls: 11, c: C.line, al: 'c', sh: ring(2.5) });
	h += T(386, 232, 'MAX HEALTH +8%', { s: 23, ls: 6, c: C.acc, al: 'c', sh: ring(2) });
	const LU = frame(...FU, { bg: 'url(\'' + IMG.grave + '\') -560px -150px/1920px auto', dim: 'rgba(6,8,11,.32)', svg: s, html: h, label: 'LEVEL UP · HUD, NO GLASS' });

	return sheet(4, 'GRAVE MARKER', 'Revenant headstones as panels: sharp-cut shoulders, carved inset lines, a lantern glow behind the chosen word, the level gem burns as a soul-light.',
		L, MM, LU,
		['Opens: {the stones rise 40 px out of the ground} with a dust line along the plinth (0.18 s); the carved lines draw in from the top.',
			'Move: {the lantern glow slides to the new word} and flickers softly (±6%); the lamps either side swing in after it.',
			'Level up: {the soul-light rises up the stone}, flares 1.3 → 1, embers drift up; the outline fades in from the top; holds 2.2 s, sinks.'],
		['open [UI.Open] + a low stone grind (new [UI.Stone.Rise]) · move [UI.Hover] (a soft lamp chime) · pick [UI.Menu.Select]',
			'tab [UI.Tab] · Resume [UI.Close] · Save &amp; Quit [UI.Click]',
			'level up [UI.LevelUp] + a far bell (new [UI.SoulBell])'],
		'font-weight:700');
}

// =====================================================================================================
// 5 · SIGNAL GLASS
// =====================================================================================================
function c5() {
	let s = '', h = '';
	const arrowR = (x, cy, sz = 10) => poly([[x, cy - sz], [x + sz * 1.6, cy], [x, cy + sz]], { fill: C.acc });
	const ticks = (x1, x2, y, op = .4) => ln(x1, y, x2, y, C.line, 1, op) + ln(x1, y - 6, x1, y + 6, C.line, 1, op) + ln(x2, y - 6, x2, y + 6, C.line, 1, op);
	// the signal ladder: one hairline down the left, a tick at each line of the menu
	s += ln(80, 120, 80, 700, C.line, 1, .22);
	for (const y of [158, 278, 360, 424, 488, 612]) s += ln(74, y, 86, y, C.line, 1, .45);
	h += T(110, 58, 'GAME PAUSED', { s: 15, ls: 8, c: C.lab });
	h += T(990, 60, 'RANSOM\'S REST', { s: 13, ls: 5, c: C.lab, al: 'r' });
	s += ticks(80, 990, 92);
	// Resume: big type, the gold bar and the arrow
	h += T(108, 114, 'RESUME', { s: 76, ls: 12, c: C.text });
	s += arrowR(70, 158, 11);
	s += rect(110, 208, 196, 5, C.acc) + ln(306, 210.5, 990, 210.5, C.line, 1, .22);
	// tabs
	const tabs = [['GRAPHICS', 110, 156], ['AUDIO', 316, 96], ['INTERFACE', 462, 176], ['CONTROLS', 688, 160]];
	tabs.forEach(([t, x, w], i) => {
		h += T(x, 262, t, { s: 22, ls: 5, c: i === 0 ? C.text : C.lab, css: i === 0 ? '' : 'opacity:.62' });
		if (i === 0) s += rect(x, 296, w - 6, 3, C.acc) + poly([[x + w / 2 - 9, 308], [x + w / 2, 300], [x + w / 2 + 9, 308]], { fill: C.acc });
	});
	s += ln(110, 316, 990, 316, C.line, 1, .2);
	// rows
	const rowY = [360, 424, 488];
	h += T(110, rowY[0] - 13, 'Quality', { s: 21, w: 400, c: C.text }) + T(110, rowY[1] - 13, 'Motion blur', { s: 21, w: 400, c: C.text }) + T(110, rowY[2] - 13, 'Field of view (1st person)', { s: 21, w: 400, c: C.text });
	const opt = (cx, cy, t, on, bw) => T(cx, cy - 10, t, { s: 15, ls: 3, c: on ? C.text : C.lab, al: 'c', css: on ? '' : 'opacity:.55' }) + (on ? '<svg></svg>' : '');
	h += opt(462, rowY[0], 'LOW') + opt(546, rowY[0], 'MEDIUM', 1) + opt(632, rowY[0], 'HIGH') + opt(700, rowY[0], 'EPIC');
	s += rect(516, rowY[0] + 14, 60, 2.5, C.acc);
	h += opt(632, rowY[1], 'ON') + opt(700, rowY[1], 'OFF', 1);
	s += rect(680, rowY[1] + 14, 40, 2.5, C.acc);
	s += ln(450, rowY[2], 660, rowY[2], C.line, 1.5, .35) + ln(450, rowY[2], 590, rowY[2], C.acc, 3) + rect(588, rowY[2] - 9, 3.5, 18, C.acc);
	h += T(682, rowY[2] - 13, '90°', { s: 21, w: 400, c: C.text });
	for (const y of [392, 456, 520]) s += ln(110, y, 730, y, C.line, 1, .13);
	// the description floats beside, behind one hairline
	s += ln(770, 340, 770, 520, C.line, 1, .3);
	h += T(794, 342, 'RESUME', { s: 14, ls: 6, c: C.acc });
	h += T(794, 370, DESC, { wd: 200, s: 17, w: 400, lh: 1.5, c: C.text });
	// Save & Quit
	h += T(108, 590, 'SAVE &amp; QUIT', { s: 34, ls: 7, c: C.red });
	s += ln(110, 724, 990, 724, C.line, 1, .14);
	h += keys(110, 748, [['ENTER', 'SELECT'], ['PGUP / PGDN', 'TABS'], ['ESC', 'RESUME']], { c: C.line });
	const L = frame(...FL, { bg: 'url(\'' + IMG.maud + '\') -420px -150px/1975px auto', filter: 'blur(7px) saturate(.8)', dim: 'linear-gradient(90deg,rgba(6,8,11,.9),rgba(6,8,11,.76) 60%,rgba(6,8,11,.6))', svg: s, html: h, label: 'PAUSE MENU' });

	// --- main menu
	s = ''; h = '';
	h += T(54, 38, 'REVENANT', { s: 56, ls: 12, c: C.text });
	s += ticks(56, 716, 120, .45);
	h += T(56, 132, 'TEROPA', { s: 13, ls: 9, c: C.acc });
	const items = [['01', 'SINGLE PLAYER'], ['02', 'MULTIPLAYER'], ['03', 'SETTINGS'], ['04', 'QUIT GAME']];
	items.forEach(([n, t], i) => {
		const y = 176 + i * 54;
		const sel = i === 0, dim = i === 1;
		h += T(56, y + 9, n, { s: 13, ls: 1, c: sel ? C.acc : C.lab, css: dim ? 'opacity:.4' : '' });
		h += T(88, y, t, { s: 27, ls: 5, c: sel ? C.text : C.text, css: dim ? 'opacity:.36' : sel ? '' : 'opacity:.82' });
		if (sel) s += arrowR(34, y + 17, 7) + rect(88, y + 38, 282, 4, C.acc);
	});
	h += T(338, 240, 'SOON', { s: 11, ls: 3, c: C.lab, css: 'padding:3px 7px;box-shadow:inset 0 0 0 1px rgba(212,238,244,.55)' });
	h += keys(56, 396, [['ENTER', 'SELECT']], { c: C.line });
	const MM = frame(...FM, { bg: menuBg, dim: 'linear-gradient(90deg,rgba(6,8,11,.7),rgba(6,8,11,.2) 55%,rgba(6,8,11,0))', svg: s, html: h, label: 'MAIN MENU' });

	// --- level-up: hairlines wipe across, one huge number between them. No glass.
	s = ''; h = '';
	s += ell(386, 180, 330, 170, 'url(#shade)');
	h += T(386, 48, 'LEVEL UP', { s: 30, ls: 16, c: C.text, al: 'c', sh: ring(2) });
	s += rect(346, 96, 80, 4, C.acc);
	s += ticks(70, 300, 192, .8) + ticks(472, 702, 192, .8);
	s += poly([[306, 192], [316, 186], [316, 198]], { fill: C.acc }) + poly([[466, 192], [456, 186], [456, 198]], { fill: C.acc });
	h += T(386, 102, '7', { s: 150, c: C.text, al: 'c', sh: ring(3) });
	h += T(386, 292, 'MAX HEALTH +8%', { s: 23, ls: 7, c: C.acc, al: 'c', sh: ring(2) });
	const LU = frame(...FU, { bg: 'url(\'' + F + 'Saved/Screenshots/Tools/dusk_cool.jpg\') 12px -28px/900px auto', dim: 'rgba(6,8,11,.25)', svg: s, html: h, label: 'LEVEL UP · HUD, NO GLASS' });

	return sheet(5, 'SIGNAL GLASS', 'Almost no panels: thin salt hairlines, big confident type and room to breathe; a gold bar and a small arrow mark the choice. Calm, premium.',
		L, MM, LU,
		['Opens: the game blurs (0.2 s); {hairlines draw out from the left}, then the words fade in 8 px up, 40 ms apart.',
			'Move: {the gold bar glides under the new word} (0.1 s, no overshoot) and the arrow slides down the ladder with it.',
			'Level up: {two hairlines wipe out from the middle}, the number counts 6 → 7 with one soft flash; the gold bar draws last; holds 2 s.'],
		['open [UI.Open] (soft, airy) · move [UI.Hover] (a quiet tick) · tab [UI.Tab] · pick [UI.Menu.Select]',
			'Resume [UI.Close] · Save &amp; Quit [UI.Click]',
			'level up [UI.LevelUp] on the count, as now'],
		'font-weight:400;letter-spacing:10px');
}

// =====================================================================================================
// 6 · BOUNTY POSTER
// =====================================================================================================
// Stencil: a bridge cut through the middle of every letter.
const stencil = (gap = [47, 55]) => '-webkit-mask-image:linear-gradient(180deg,#000 0 ' + gap[0] + '%,transparent ' + gap[0] + '% ' + gap[1] + '%,#000 ' + gap[1] + '%);';
// An outline stamp, rotated.
function stamp(cx, cy, w, hgt, t, deg, o = {}) {
	const c = o.c ?? C.acc;
	return '<div style="position:absolute;left:' + (cx - w / 2) + 'px;top:' + (cy - hgt / 2) + 'px;width:' + w + 'px;height:' + hgt + 'px;transform:rotate(' + deg + 'deg);box-sizing:border-box;' +
		'border:' + (o.bw ?? 4) + 'px solid ' + c + ';box-shadow:0 0 0 2px rgba(4,6,10,.6),inset 0 0 0 2px rgba(4,6,10,.6);' +
		'display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:' + (o.s ?? 24) + 'px;letter-spacing:' + (o.ls ?? 5) + 'px;padding-left:' + (o.ls ?? 5) + 'px;color:' + c + ';opacity:' + (o.op ?? .92) + ';text-shadow:0 0 2px #000;' + (o.bg ? 'background:' + o.bg + ';' : '') + '">' +
		'<div style="position:absolute;inset:4px;border:1.5px solid ' + c + ';opacity:.8"></div>' + t + '</div>';
}
// A number plate: cut top-right and bottom-left corners.
const platePts = (x, y, w, hgt, c = 12) => [[x, y], [x + w - c, y], [x + w, y + c], [x + w, y + hgt], [x + c, y + hgt], [x, y + hgt - c]];
function c6() {
	let s = '', h = '';
	// --- pause: the poster, torn corners as crisp cuts, a print border, a stencil header and the PAUSED stamp
	const poster = [[70, 68], [104, 34], [596, 34], [618, 52], [638, 40], [690, 86], [690, 696], [662, 724], [120, 724], [98, 706], [84, 716], [70, 690]];
	s += poly(poster, { fill: 'glass', st: C.line, sw: 4 });
	s += poly([[96, 52], [664, 52], [674, 62], [674, 696], [664, 706], [96, 706], [86, 696], [86, 62]], { st: C.line, sw: 1.5, so: .32, edge: false });
	h += T(108, 64, '— RANSOM\'S REST —', { s: 13, ls: 6, c: C.lab });
	h += T(104, 82, 'SETTINGS', { s: 76, ls: 5, c: C.line, css: stencil() });
	s += rect(96, 176, 568, 4, C.line) + rect(96, 186, 568, 1.5, C.line, .6);
	// 01 Resume, chosen: the plate is inked gold
	s += poly(platePts(110, 206, 96, 80, 14), { fill: C.acc, st: C.acc, sw: 3 });
	h += T(158, 214, '01', { s: 48, c: C.gem, al: 'c' });
	h += T(228, 216, 'RESUME', { s: 46, ls: 6, c: C.acc, css: stencil([48, 54]) });
	// tabs as tickets with side notches
	const ticket = (x, y, w, hh) => [[x, y], [x + w, y], [x + w, y + 13], [x + w - 7, y + 20], [x + w, y + 27], [x + w, y + hh], [x, y + hh], [x, y + 27], [x + 7, y + 20], [x, y + 13]];
	['GRAPHICS', 'AUDIO', 'INTERFACE', 'CONTROLS'].forEach((t, i) => {
		const x = 110 + i * 138;
		s += poly(ticket(x, 310, 132, 40), i === 0 ? { fill: 'rgba(255,230,166,.14)', st: C.acc, sw: 3 } : { st: C.line, sw: 2, so: .35, edge: false });
		h += T(x + 66, 320, t, { s: 14, ls: 3, c: i === 0 ? C.acc : C.lab, al: 'c' });
	});
	// rows
	const rowY = [400, 458, 516];
	h += T(110, rowY[0] - 11, 'Quality', { s: 18, c: C.lab }) + T(110, rowY[1] - 11, 'Motion blur', { s: 18, c: C.lab }) + T(110, rowY[2] - 11, 'Field of view (1st person)', { s: 18, c: C.lab });
	const chip = (x1, x2, cy, t, on) => poly(platePts(x1, cy - 15, x2 - x1, 30, 7), on ? { fill: 'rgba(255,230,166,.16)', st: C.acc, sw: 3 } : { st: C.line, sw: 2, so: .3, edge: false });
	const chipT = (x1, x2, cy, t, on) => T((x1 + x2) / 2, cy - 8, t, { s: 13, ls: 2, c: on ? C.acc : C.lab, al: 'c' });
	for (const [a, b, t, on, r] of [[396, 450, 'LOW', 0, 0], [456, 534, 'MEDIUM', 1, 0], [540, 594, 'HIGH', 0, 0], [600, 656, 'EPIC', 0, 0], [540, 594, 'ON', 0, 1], [600, 656, 'OFF', 1, 1]]) {
		s += chip(a, b, rowY[r], t, on); h += chipT(a, b, rowY[r], t, on);
	}
	s += poly([[428, rowY[2] - 5], [582, rowY[2] - 5], [576, rowY[2] + 5], [422, rowY[2] + 5]], { fill: 'rgba(212,238,244,.2)' }) + poly([[428, rowY[2] - 5], [528, rowY[2] - 5], [522, rowY[2] + 5], [422, rowY[2] + 5]], { fill: C.acc });
	s += poly(platePts(520, rowY[2] - 11, 16, 22, 5), { fill: C.gem, st: C.acc, sw: 3 });
	h += T(604, rowY[2] - 11, '90°', { s: 18, c: C.text });
	for (const y of [429, 487, 545]) s += ln(110, y, 656, y, C.line, 1.5, .18);
	// 02 Save & Quit
	s += poly(platePts(110, 570, 96, 80, 14), { st: C.red, sw: 3 });
	h += T(158, 578, '02', { s: 48, c: C.red, al: 'c' });
	h += T(228, 588, 'SAVE &amp; QUIT', { s: 36, ls: 5, c: C.red, css: stencil([48, 54]) });
	s += rect(96, 670, 568, 1.5, C.line, .6) + rect(96, 676, 568, 4, C.line);
	h += T(380, 686, 'Every change here is already saved.', { s: 14, w: 400, ls: 1, c: C.lab, al: 'c' });
	h += stamp(650, 104, 262, 62, 'GAME PAUSED', -11, { s: 24, bg: 'rgba(18,21,26,.92)' });
	// the notice slip pinned beside it: what the cursor rests on
	const slip = '<div style="position:absolute;left:736px;top:246px;width:280px;height:276px;transform:rotate(2.5deg);transform-origin:140px 0">' +
		'<svg width="280" height="276" style="position:absolute;left:0;top:0;overflow:visible">' +
		poly([[0, 20], [20, 0], [196, 0], [212, 12], [228, 4], [280, 4], [280, 254], [258, 276], [0, 276]], { fill: 'glass', st: C.line, sw: 4 }) +
		rect(22, 74, 236, 3, C.acc, .9) +
		poly(dia(140, 2, 8, 10), { fill: C.gem, st: C.acc, sw: 3 }) + '</svg>' +
		T(140, 24, 'RESUME', { s: 32, ls: 5, c: C.acc, al: 'c', css: stencil([48, 54]) }) +
		T(26, 92, DESC, { wd: 228, s: 16, w: 400, lh: 1.5, c: C.text }) +
		T(26, 196, 'Point at a setting to see what it does.', { wd: 228, s: 14, w: 400, lh: 1.45, c: C.lab }) +
		'</div>';
	h += slip;
	h += keys(110, 768, [['ENTER', 'SELECT'], ['PGUP / PGDN', 'TABS'], ['ESC', 'RESUME']]);
	const L = frame(...FL, { bg: 'url(\'' + IMG.dusk + '\') -380px -40px/1800px auto', filter: 'blur(2px)', dim: 'rgba(6,8,11,.58)', svg: s, html: h, label: 'PAUSE MENU' });

	// --- main menu: a big number plate per item, SOON stamped on Multiplayer
	s = ''; h = '';
	h += T(38, 28, 'REVENANT', { s: 62, ls: 6, c: C.line, css: stencil() });
	s += rect(40, 106, 430, 4, C.line) + rect(40, 116, 430, 1.5, C.line, .6);
	h += T(40, 126, 'TEROPA', { s: 13, ls: 7, c: C.acc });
	const items = [['01', 'SINGLE PLAYER'], ['02', 'MULTIPLAYER'], ['03', 'SETTINGS'], ['04', 'QUIT GAME']];
	items.forEach(([n, t], i) => {
		const y = 158 + i * 68;
		const sel = i === 0, dim = i === 1;
		s += poly(platePts(40, y, 68, 56, 12), sel ? { fill: C.acc, st: C.acc, sw: 3 } : { st: C.line, sw: 3, so: dim ? .25 : .8 });
		h += T(74, y + 8, n, { s: 34, c: sel ? C.gem : dim ? C.lab : C.line, al: 'c', css: dim ? 'opacity:.45' : '' });
		h += T(128, y + 12, t, { s: 27, ls: 4, c: sel ? C.acc : C.text, css: (dim ? 'opacity:.4;' : '') + stencil([48, 54]), sh: soft });
	});
	h += stamp(404, 244, 96, 38, 'SOON', -9, { s: 19, ls: 4, bw: 3 });
	h += keys(596, 398, [['ENTER', 'SELECT']]);
	const MM = frame(...FM, { bg: menuBg, dim: 'linear-gradient(90deg,rgba(6,8,11,.66),rgba(6,8,11,.15) 62%,rgba(6,8,11,0))', svg: s, html: h, label: 'MAIN MENU' });

	// --- level-up: a WANTED-style tag drops on its string from a nail; the reward is stamped on. No glass.
	s = ''; h = '';
	s += ell(386, 190, 340, 170, 'url(#shade)');
	s += '<g transform="translate(-56 0)">';
	s += ln(386, 18, 262, 70, C.edge, 5, .7) + ln(386, 18, 510, 70, C.edge, 5, .7) + ln(386, 18, 262, 70, C.line, 2, .9) + ln(386, 18, 510, 70, C.line, 2, .9);
	s += poly(dia(386, 18, 8, 10), { fill: C.gem, st: C.acc, sw: 3 });
	const tag = [[236, 64], [536, 64], [566, 94], [566, 274], [548, 298], [522, 286], [502, 298], [226, 298], [206, 280], [206, 94]];
	s += '<g transform="rotate(3 386 18)">' + poly(tag, { st: C.line, sw: 4 }) +
		poly([[240, 76], [532, 76], [554, 98], [554, 270], [540, 286], [222, 286], [218, 282], [218, 98]], { st: C.line, sw: 1.5, so: .35, edge: false }) +
		rect(232, 156, 308, 4, C.line) + rect(232, 165, 308, 1.5, C.line, .6) +
		poly(platePts(331, 176, 110, 96, 16), { st: C.acc, sw: 4 }) + '</g></g>';
	h += '<div style="position:absolute;inset:0;transform:translateX(-56px) rotate(3deg);transform-origin:386px 18px">' +
		T(386, 90, 'LEVEL UP', { s: 52, ls: 6, c: C.line, al: 'c', sh: ring(2.5), css: stencil([48, 55]) }) +
		T(386, 174, '7', { s: 88, c: C.acc, al: 'c', sh: ring(3) }) + '</div>';
	h += stamp(652, 232, 178, 96, '<div style="text-align:center;line-height:1.05">MAX HEALTH<br><span style="font-size:38px;letter-spacing:4px">+8%</span></div>', -9, { s: 19, ls: 3, bw: 3.5 });
	const LU = frame(...FU, { bg: 'url(\'' + IMG.dusk + '\') -560px -130px/1636px auto', dim: 'rgba(6,8,11,.3)', svg: s, html: h, label: 'LEVEL UP · HUD, NO GLASS' });

	return sheet(6, 'BOUNTY POSTER', 'Weird-west print in Slate &amp; Salt: stencil headers, torn corners cut crisp, a big number plate per item, outline stamps. Loud, with character.',
		L, MM, LU,
		['Opens: {the poster slaps up} (scale 1.08 → 1, a 3° tilt settles, 0.12 s); 0.1 s later {the stamp thunks on} (1.6 → 1, 2 px shake).',
			'Move: {the chosen plate inks gold left to right} (0.08 s) and its number knocks 2 px in; the notice slip swaps its words.',
			'Level up: {the tag drops on its string}, swings twice (±4°) and settles; the 7 inks in, then {MAX HEALTH +8% is stamped}; holds 2.4 s.'],
		['open [UI.Open] + a paper slap (new [UI.Poster.Slap]) · stamp (new [UI.Stamp]) · move [UI.Hover] · pick [UI.Menu.Select] + [UI.Stamp]',
			'tab [UI.Tab] · Resume [UI.Close] · Save &amp; Quit [UI.Click]',
			'level up [UI.LevelUp] on the drop + [UI.Stamp] on the reward'],
		stencil());
}

const css = '@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Regular.ttf\');font-weight:400}' +
	'@font-face{font-family:Chakra;src:url(\'' + F + 'Content/UI/Fonts/ChakraPetch-Bold.ttf\');font-weight:700}' +
	'body{margin:0;background:#0b0d10}';

(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	for (const [name, fn] of [['c4', c4], ['c5', c5], ['c6', c6]]) {
		const html = '<!doctype html><html><head><meta charset="utf-8"><style>' + css + '</style></head><body>' + fn() + '</body></html>';
		const file = path.join(here, name + '.html');
		fs.writeFileSync(file, html);
		await pg.goto('file:///' + file.split(path.sep).join('/'));
		await pg.evaluate(() => document.fonts.ready);
		await pg.waitForTimeout(700);
		await pg.screenshot({ path: path.join(here, name + '.png') });
		console.log(name, 'done');
	}
	await browser.close();
})();
