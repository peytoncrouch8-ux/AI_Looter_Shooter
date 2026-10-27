// Melee tool concept sheets (CONTRACT3 note 5): one PNG per hero, options A/B/C, drawn as SVG and shot by Playwright.
//   node Art/Concepts/MeleeTools/sheets.js [Hero...]   -> Art/Concepts/MeleeTools/MeleeTools_<Hero>.png (1600 x 1000)
// Every tool is drawn in its own frame: handle along y, the right hand's grip at (0,0), the head toward -y, the forearm
// along +x. The page rotates and fits each into its panel. Original designs only.
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const ROOT = path.resolve(__dirname, '../../..');
const FONT = p => 'file:///' + path.join(ROOT, p).replace(/\\/g, '/');
const OUT = '#0b0c10';

// ---- shared bits -------------------------------------------------------------------------------------------------
const grad = (id, stops, v) => `<linearGradient id="${id}" x1="0" y1="0" x2="${v ? 0 : 1}" y2="${v ? 1 : 0}">${stops
	.map((c, i) => `<stop offset="${(i / (stops.length - 1)).toFixed(2)}" stop-color="${c}"/>`).join('')}</linearGradient>`;
const MATS = {
	wood: ['#4a2d17', '#8e5b31', '#b37a45', '#5a3820'],
	ash: ['#6d5233', '#b8935f', '#d6b383', '#75583a'],
	hickory: ['#5c3b1f', '#ad7f4b', '#c99a64', '#6b4524'],
	walnut: ['#2e1a0e', '#5e3a20', '#7a4d2b', '#341e10'],
	grey: ['#3f3a33', '#7d7466', '#958a79', '#4a443b'],
	iron: ['#202429', '#5d666f', '#98a2ad', '#30363d'],
	steel: ['#5d6670', '#b9c3cd', '#eef3f8', '#76808b'],
	brass: ['#6e4a12', '#c9963a', '#f2d483', '#8c6118'],
	gold: ['#7a5510', '#e8b83e', '#fff0a8', '#a07018'],
	leather: ['#24150c', '#5a3820', '#74492b', '#2e1b10'],
	ebony: ['#060608', '#24242c', '#40404c', '#0e0e12'],
	tin: ['#5f656c', '#b7bcc1', '#e9ecef', '#7d838a'],
	stag: ['#7d6d56', '#cfc1a5', '#e9dfc8', '#8f7f66'],
	bone: ['#b8ab92', '#efe6d2', '#fbf6ea', '#c9bca2'],
	card: ['#e6dcc4', '#f7f1e2', '#fffaf0', '#e9dfc8'],
};
function defs(hero) {
	let s = '';
	for (const [k, v] of Object.entries(MATS)) s += grad(k, v) + grad(k + 'V', v, true);
	s += `<radialGradient id="hot"><stop offset="0" stop-color="#fff6d0"/><stop offset=".35" stop-color="#ffb347"/>
		<stop offset=".7" stop-color="#ff5a1a" stop-opacity=".55"/><stop offset="1" stop-color="#ff3a00" stop-opacity="0"/></radialGradient>
		<radialGradient id="cyan"><stop offset="0" stop-color="#f0fdff"/><stop offset=".35" stop-color="#5AD8FF"/>
		<stop offset="1" stop-color="#5AD8FF" stop-opacity="0"/></radialGradient>
		<radialGradient id="hero"><stop offset="0" stop-color="${hero.color}" stop-opacity=".55"/>
		<stop offset="1" stop-color="${hero.color}" stop-opacity="0"/></radialGradient>
		<filter id="shadow" x="-30%" y="-30%" width="160%" height="160%"><feDropShadow dx="8" dy="14" stdDeviation="10" flood-color="#000" flood-opacity=".6"/></filter>
		<filter id="glow" x="-80%" y="-80%" width="260%" height="260%"><feGaussianBlur stdDeviation="6" result="b"/>
		<feMerge><feMergeNode in="b"/><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>
		<filter id="soft" x="-80%" y="-80%" width="260%" height="260%"><feGaussianBlur stdDeviation="10"/></filter>
		<pattern id="grid" width="40" height="40" patternUnits="userSpaceOnUse"><path d="M40 0H0V40" fill="none" stroke="#5AD8FF" stroke-opacity=".06"/></pattern>`;
	return `<defs>${s}</defs>`;
}
const W = 3; // outline width
const P = (d, mat, extra = '') => `<path d="${d}" fill="url(#${mat})" stroke="${OUT}" stroke-width="${W}" stroke-linejoin="round" ${extra}/>`;
const R = (x, y, w, h, mat, r = 0, extra = '') =>
	`<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="${r}" fill="url(#${mat})" stroke="${OUT}" stroke-width="${W}" ${extra}/>`;
const C = (x, y, r, mat, extra = '') => `<circle cx="${x}" cy="${y}" r="${r}" fill="url(#${mat})" stroke="${OUT}" stroke-width="${W}" ${extra}/>`;
const line = (d, col, w, extra = '') => `<path d="${d}" fill="none" stroke="${col}" stroke-width="${w}" stroke-linecap="round" stroke-linejoin="round" ${extra}/>`;
// A thick outlined stroke (a bent rod, a loop, a hook).
const rod = (d, mat, w) => line(d, OUT, w + 2 * W) + line(d, `url(#${mat})`, w);
const rivet = (x, y, r = 4) => `<circle cx="${x}" cy="${y}" r="${r}" fill="#c9d1d9" stroke="${OUT}" stroke-width="1.5"/>`;
const grain = (x0, x1, y0, y1, n, col = '#3a2412') => {
	let s = '';
	for (let i = 0; i < n; i++) {
		const x = x0 + ((i + 0.5) / n) * (x1 - x0), wob = (i % 2 ? 2 : -2);
		s += line(`M${x},${y0} Q${x + wob},${(y0 + y1) / 2} ${x},${y1}`, col, 1.2, 'stroke-opacity=".45"');
	}
	return s;
};
const wrap = (x0, x1, y0, y1, step, col = '#1a0f08') => {
	let s = '';
	for (let y = y0; y < y1; y += step) s += line(`M${x0},${y + step * 0.8} L${x1},${y}`, col, 2, 'stroke-opacity=".7"');
	return s;
};
const SUIT = {
	heart: 'M0,14 C-22,-2 -19,-19 -9,-19 C-4,-19 0,-15 0,-10 C0,-15 4,-19 9,-19 C19,-19 22,-2 0,14 Z',
	diamond: 'M0,-19 L14,0 L0,19 L-14,0 Z',
	spade: 'M0,-19 C-22,-2 -19,13 -9,13 C-5,13 -2,11 -1,8 L-6,20 L6,20 L1,8 C2,11 5,13 9,13 C19,13 22,-2 0,-19 Z',
	club: 'M0,-19 a8,8 0 1 1 0.1,0 Z M-9,-2 a8,8 0 1 1 0.1,0 Z M9,-2 a8,8 0 1 1 0.1,0 Z M-2,4 L-6,20 L6,20 L2,4 Z',
};
const suit = (k, x, y, s, col) => `<path d="${SUIT[k]}" transform="translate(${x},${y}) scale(${s})" fill="${col}"/>`;

// The right hand round a handle on the y axis at (0,0), the forearm out along +x. h: glove, cuff, sleeve colours.
function fist(h) {
	const g = h.glove, n = h.three ? 3 : 4, top = -42, span = 84, fh = span / n;
	let s = '';
	// sleeve and cuff
	s += `<path d="M84,-46 L196,-56 L204,66 L84,54 Z" fill="${h.sleeve}" stroke="${OUT}" stroke-width="${W}" stroke-linejoin="round"/>`;
	s += line('M120,-48 L128,58', OUT, 1.5, 'stroke-opacity=".35"') + line('M160,-52 L168,62', OUT, 1.5, 'stroke-opacity=".35"');
	if (h.cuff) s += `<path d="M70,-42 L104,-46 L108,54 L70,48 Z" fill="${h.cuff}" stroke="${OUT}" stroke-width="${W}"/>`;
	// wrist and back of the hand
	s += `<path d="M20,-38 L80,-40 L82,48 L20,40 Z" fill="${g}" stroke="${OUT}" stroke-width="${W}" stroke-linejoin="round"/>`;
	s += `<rect x="-30" y="-46" width="66" height="92" rx="20" fill="${g}" stroke="${OUT}" stroke-width="${W}"/>`;
	// curled fingers across the front of the handle
	for (let i = 0; i < n; i++) {
		const y = top + i * fh;
		s += `<rect x="-44" y="${y}" width="44" height="${fh - 2}" rx="${(fh - 2) / 2}" fill="${g}" stroke="${OUT}" stroke-width="${W}"/>`;
		s += line(`M-34,${y + 5} Q-26,${y + 3} -16,${y + 5}`, '#ffffff', 2, 'stroke-opacity=".18"');
		if (h.three) s += rivet(-12, y + fh / 2 - 1, 3.5);
	}
	// thumb over the top finger
	s += `<rect x="-38" y="-60" width="64" height="24" rx="12" fill="${g}" stroke="${OUT}" stroke-width="${W}" transform="rotate(-8 0 -48)"/>`;
	if (h.three) s += rivet(14, -46, 4) + rivet(24, 20, 4) + `<rect x="30" y="-40" width="10" height="84" fill="#c9963a" stroke="${OUT}" stroke-width="2"/>`;
	return `<g>${s}</g>`;
}

// ---- the tools (local frame: grip at 0,0, head toward -y) ---------------------------------------------------------
const T = {};
// Ellis
T.graveSpade = () => rod('M-10,62 L-10,86 Q-46,92 -46,122 Q-46,150 0,150 Q46,150 46,122 Q46,92 10,86 L10,62', 'ash', 13)
	+ R(-11, -232, 22, 300, 'ash', 4) + grain(-8, 8, -228, 64, 3)
	+ P('M-11,-230 L11,-230 L17,-282 L-17,-282 Z', 'iron') + rivet(0, -250)
	+ P('M-58,-276 L58,-276 L54,-385 Q50,-428 0,-446 Q-50,-428 -54,-385 Z', 'iron')
	+ line('M-52,-392 Q-48,-428 0,-442 Q48,-428 52,-392', '#e3e9ef', 5, 'stroke-opacity=".85"')
	+ R(-62, -284, 124, 12, 'ironV', 3)
	+ `<path d="M-30,-420 Q-10,-436 20,-430 Q0,-410 -30,-420 Z" fill="#5a3d22" opacity=".55"/>`
	+ `<g filter="url(#glow)">${line('M6,-296 L-10,-322 L9,-346 L-6,-372 L4,-392', '#5AD8FF', 4)}</g>`
	+ line('M6,-296 L-10,-322 L9,-346 L-6,-372 L4,-392', '#f0fdff', 1.5);
T.ransomBrand = () => R(-15, -64, 30, 140, 'walnut', 12) + [-34, -6, 22, 50].map(y => line(`M-14,${y} L14,${y}`, OUT, 2)).join('')
	+ R(-12, -84, 24, 22, 'brass', 3)
	+ R(-6, -336, 12, 254, 'iron', 3)
	+ `<rect x="-6" y="-336" width="12" height="90" fill="url(#cyan)" opacity=".5"/>`
	+ `<g filter="url(#glow)">${line('M0,-338 L0,-348', '#9eeaff', 10)}
		<circle cx="0" cy="-396" r="48" fill="none" stroke="#5AD8FF" stroke-width="13"/>
		${line('M-18,-372 L-18,-420 L4,-420 Q22,-420 22,-404 Q22,-390 4,-390 L-18,-390 M2,-390 L22,-370', '#5AD8FF', 10)}
		${line('M-30,-416 Q0,-436 30,-416', '#5AD8FF', 6)}</g>
		<circle cx="0" cy="-396" r="48" fill="none" stroke="#f0fdff" stroke-width="4"/>
		${line('M-18,-372 L-18,-420 L4,-420 Q22,-420 22,-404 Q22,-390 4,-390 L-18,-390 M2,-390 L22,-370', '#f0fdff', 3.5)}
		${line('M-30,-416 Q0,-436 30,-416', '#f0fdff', 2.5)}`
	+ [[40, -440], [-52, -446], [58, -380], [-60, -360], [30, -470]].map(([x, y]) => `<circle cx="${x}" cy="${y}" r="3" fill="#bff3ff"/>`).join('');
T.drovers = () => P('M-14,-8 Q-17,40 -12,84 L14,84 Q18,40 14,-8 Z', 'stag')
	+ [[-6, 10], [6, 32], [-4, 56], [5, 72]].map(([x, y]) => `<ellipse cx="${x}" cy="${y}" rx="4" ry="2.5" fill="#5c4a35" opacity=".7"/>`).join('')
	+ R(-16, 82, 32, 16, 'brass', 7)
	+ rod('M42,-16 Q76,42 16,92', 'brass', 9)
	+ R(-44, -24, 88, 15, 'brassV', 5)
	+ P('M-13,-24 L-13,-222 Q-10,-248 -6,-282 Q22,-246 22,-180 L15,-24 Z', 'steel')
	+ line('M-3,-40 L-3,-196', '#6c7682', 3) + line('M19,-178 Q21,-240 -4,-276', '#ffffff', 2.5, 'stroke-opacity=".9"')
	+ rivet(0, 20, 3) + rivet(0, 58, 3);
// Odessa
T.razorFan = () => {
	const cards = [['spade', -36, '#16161c'], ['diamond', -18, '#c0283f'], ['club', 0, '#16161c'], ['heart', 18, '#c0283f'], ['spade', 36, '#16161c']];
	return cards.map(([k, a, col], i) => `<g transform="rotate(${a} 0 6)">
		<rect x="-50" y="-176" width="100" height="148" rx="11" fill="none" stroke="#dfe7ee" stroke-width="9"/>
		<rect x="-50" y="-176" width="100" height="148" rx="11" fill="url(#card)" stroke="${OUT}" stroke-width="${W}"/>
		<rect x="-42" y="-168" width="84" height="132" rx="7" fill="none" stroke="#c9963a" stroke-width="2.5"/>
		${suit(k, 0, -108, 1.35, col)}${suit(k, -30, -152, 0.45, col)}
		${i === 4 ? line('M-50,-160 L-50,-60', '#ffffff', 2) : ''}</g>`).join('');
};
T.rake = () => R(-10, -440, 20, 520, 'ebony', 5) + R(-13, 70, 26, 16, 'gold', 4) + C(0, 96, 14, 'gold')
	+ R(-13, -60, 26, 128, 'leather', 5) + wrap(-13, 13, -58, 66, 12)
	+ R(-14, -458, 28, 24, 'gold', 4)
	+ P('M-120,-500 L120,-500 L112,-456 L-112,-456 Z', 'brassV')
	+ R(-118, -466, 236, 12, 'steelV', 2) + line('M-114,-458 L114,-458', '#ffffff', 2.5)
	+ suit('spade', 0, -480, 0.8, '#2a1a08') + rivet(-80, -482, 4) + rivet(80, -482, 4);
T.spadeCane = () => R(-12, -318, 24, 410, 'ebony', 7) + line('M-5,-300 L-5,80', '#ffffff', 1.5, 'stroke-opacity=".25"')
	+ R(-14, 88, 28, 36, 'tin', 4) + R(-15, -332, 30, 20, 'gold', 4) + R(-11, -344, 22, 12, 'goldV', 3)
	+ `<path d="${SUIT.spade}" transform="translate(0,-400) scale(3.2)" fill="url(#gold)" stroke="${OUT}" stroke-width="1.2"/>`
	+ `<path d="M-18,-432 Q-26,-408 -16,-390" fill="none" stroke="#fff6c8" stroke-width="3" opacity=".8"/>`
	+ `<circle cx="0" cy="-404" r="10" fill="#9e1630" stroke="${OUT}" stroke-width="2"/><circle cx="-3" cy="-408" r="3" fill="#ffb3c0"/>`;
// Crane
T.ferryGaff = () => R(-12, -318, 24, 440, 'grey', 5) + grain(-9, 9, -310, 116, 3, '#2a2620')
	+ R(-13, 30, 26, 10, 'iron', 2) + R(-13, -90, 26, 10, 'iron', 2)
	+ P('M-12,-316 L12,-316 L15,-370 L-15,-370 Z', 'iron') + rivet(0, -340)
	+ P('M-9,-368 L9,-368 L0,-478 Z', 'iron') + line('M0,-470 L3,-380', '#d6dde4', 2)
	+ P('M10,-372 Q60,-392 76,-352 Q86,-314 56,-306 L50,-318 Q66,-324 62,-346 Q54,-372 12,-356 Z', 'ironV')
	+ C(0, -200, 17, 'brass') + `<circle cx="0" cy="-200" r="11" fill="none" stroke="#5a3d10" stroke-width="2"/>` + rivet(0, -200, 3.5)
	+ `<g filter="url(#glow)" opacity=".8">${line('M70,-300 Q86,-276 74,-250', '#B9D98C', 3)}${line('M40,-300 Q34,-280 46,-262', '#5AD8FF', 2)}</g>`;
T.coinSap = () => rod('M0,80 Q-34,118 -10,150 Q18,170 26,126 Q28,100 6,82', 'leather', 9)
	+ P('M-11,84 L-11,-34 Q-38,-66 -40,-110 Q-40,-170 0,-176 Q40,-170 40,-110 Q38,-66 11,-34 L11,84 Z', 'leather')
	+ line('M0,-172 L0,80', '#c8a477', 2, 'stroke-dasharray="6 6" stroke-opacity=".8"')
	+ `<path d="M-26,-128 Q-14,-150 -2,-128 Q-14,-104 -26,-128 Z" fill="#0d0907" stroke="${OUT}" stroke-width="2"/>`
	+ `<g filter="url(#glow)" opacity=".7"><ellipse cx="-14" cy="-128" rx="6" ry="12" fill="#5AD8FF"/></g>`
	+ C(-18, -132, 7, 'tin') + C(-9, -122, 7, 'brass')
	+ C(16, -88, 15, 'brass') + `<circle cx="16" cy="-88" r="10" fill="none" stroke="#5a3d10" stroke-width="2"/>`
	+ line('M10,-92 Q16,-80 22,-92 M16,-98 L16,-82', '#5a3d10', 2)
	+ R(-13, 64, 26, 12, 'iron', 3) + R(-13, -40, 26, 10, 'iron', 3);
T.billy = () => rod('M0,96 Q-30,130 0,160 Q30,130 0,96', 'leather', 6)
	+ P('M-14,-24 L-13,86 Q0,98 13,86 L14,-24 Z', 'hickory') + [0, 14, 28, 42, 56, 70].map(y => line(`M-13,${y} L13,${y}`, OUT, 2.5)).join('')
	+ R(-17, -40, 34, 18, 'hickoryV', 6)
	+ P('M-16,-40 L-19,-318 Q0,-336 19,-318 L16,-40 Z', 'hickory') + grain(-12, 12, -310, -48, 3)
	+ P('M-19,-318 Q0,-336 19,-318 L19,-302 L-19,-302 Z', 'iron')
	+ `<path d="M0,-226 L6,-212 L21,-212 L9,-203 L14,-188 L0,-197 L-14,-188 L-9,-203 L-21,-212 L-6,-212 Z" fill="url(#tin)" stroke="${OUT}" stroke-width="2"/>`
	+ [-120, -110, -100, -90, -80].map(y => line(`M-15,${y} L-6,${y + 2}`, '#2a1608', 2)).join('');
// Gauge
T.spikeMaul = () => R(-14, -350, 28, 470, 'hickory', 6) + grain(-10, 10, -340, 112, 3)
	+ R(-15, -60, 30, 130, 'leather', 4) + wrap(-15, 15, -58, 68, 14) + R(-16, -76, 32, 14, 'brass', 3)
	+ P('M-128,-376 L-104,-380 L-34,-394 L34,-394 L104,-380 L128,-376 L128,-344 L104,-340 L34,-326 L-34,-326 L-104,-340 L-128,-344 Z', 'ironV')
	+ R(-36, -398, 72, 76, 'ironV', 6) + rivet(-22, -384) + rivet(22, -384) + rivet(-22, -336) + rivet(22, -336)
	+ `<text x="0" y="-354" font-family="Chakra Petch" font-weight="700" font-size="17" fill="#c9963a" text-anchor="middle">No.7</text>`
	+ `<ellipse cx="-134" cy="-360" rx="26" ry="38" fill="url(#hot)"/><ellipse cx="134" cy="-360" rx="26" ry="38" fill="url(#hot)"/>`
	+ `<rect x="-134" y="-378" width="10" height="36" rx="2" fill="#ffb347" stroke="${OUT}" stroke-width="2"/>`
	+ `<rect x="124" y="-378" width="10" height="36" rx="2" fill="#ffb347" stroke="${OUT}" stroke-width="2"/>`;
T.trackWrench = () => C(0, 116, 34, 'iron') + `<circle cx="0" cy="116" r="18" fill="#14161a" stroke="${OUT}" stroke-width="2"/>`
	+ R(-18, -300, 36, 400, 'iron', 8) + R(-12, -270, 24, 340, 'ironV', 6, 'opacity=".35"')
	+ P('M-62,-300 L-62,-398 L-24,-424 L-24,-358 L24,-358 L24,-424 L62,-398 L62,-300 Q0,-278 -62,-300 Z', 'iron')
	+ R(-30, -424, 8, 66, 'brass', 2) + R(22, -424, 8, 66, 'brass', 2)
	+ R(-22, -300, 44, 26, 'brassV', 5) + [-14, -6, 2, 10].map(x => line(`M${x + 4},-298 L${x + 4},-276`, OUT, 1.5)).join('')
	+ rivet(-46, -330) + rivet(46, -330) + rivet(0, -200) + rivet(0, -140)
	+ `<text x="0" y="-236" font-family="Chakra Petch" font-weight="700" font-size="15" fill="#c9963a" text-anchor="middle" transform="rotate(-90 0 -236)">LINE CO.</text>`;
T.pistonFist = () =>
	`<ellipse cx="-20" cy="-90" rx="70" ry="50" fill="url(#hot)" opacity=".6"/>`
	+ R(-60, 30, 120, 230, 'iron', 14) + R(-60, 120, 120, 16, 'brassV', 3) + R(-60, 200, 120, 16, 'brassV', 3)
	+ R(44, 40, 40, 150, 'brass', 10) + R(54, -10, 20, 60, 'steel', 4) + R(48, 186, 32, 18, 'iron', 4)
	+ P('M-66,-40 L66,-40 L72,34 L-72,34 Z', 'ironV') + rivet(-50, -24) + rivet(50, -24) + rivet(-54, 20) + rivet(54, 20)
	+ [-46, 0, 46].map(x => R(x - 22, -96, 44, 64, 'iron', 18) + rivet(x, -82, 4.5)
		+ `<rect x="${x - 18}" y="-104" width="36" height="16" rx="6" fill="url(#hot)" opacity=".9"/>`).join('')
	+ R(-70, -4, 140, 16, 'brassV', 4)
	+ `<g filter="url(#soft)" opacity=".75"><circle cx="100" cy="60" r="24" fill="#eef2f5"/><circle cx="126" cy="34" r="30" fill="#eef2f5"/><circle cx="96" cy="16" r="18" fill="#eef2f5"/></g>`
	+ `<circle cx="-100" cy="-70" r="4" fill="#ffd27a"/><circle cx="-84" cy="-122" r="3" fill="#ffd27a"/><circle cx="60" cy="-130" r="3" fill="#ffd27a"/>`;
// Pike
T.rockPick = () => R(-11, -170, 22, 300, 'hickory', 6) + grain(-8, 8, -160, 124, 2)
	+ [40, 64, 88, 112].map(y => R(-12, y, 24, 7, 'brass', 2)).join('')
	+ `<rect x="-11" y="-60" width="22" height="10" fill="#3FBFAA" stroke="${OUT}" stroke-width="2"/>`
	+ P('M30,-208 Q118,-206 178,-160 Q172,-156 166,-158 Q112,-182 30,-170 Z', 'ironV')
	+ line('M40,-200 Q110,-196 170,-162', '#d9e1e8', 2)
	+ P('M-30,-206 L-72,-202 L-72,-172 L-30,-170 Z', 'ironV') + R(-90, -212, 20, 50, 'steel', 3)
	+ R(-32, -216, 64, 54, 'ironV', 6) + rivet(0, -190, 5);
T.tripodLeg = () => {
	let s = R(-16, -420, 32, 540, 'ash', 6);
	for (let y = -400, i = 0; y < 100; y += 62, i++) if (i % 2 === 0) s += `<rect x="-15" y="${y}" width="30" height="31" fill="#b8322b"/>`;
	for (let y = -412; y < 110; y += 15.5) s += line(`M15,${y} L${(Math.round((y + 412) / 15.5) % 4 === 0) ? 4 : 9},${y}`, OUT, 1.5);
	return s + R(-18, 96, 36, 30, 'brass', 6) + R(-19, -438, 38, 32, 'brass', 4)
		+ P('M-16,-438 L16,-438 L5,-510 L0,-522 L-5,-510 Z', 'steel') + line('M0,-508 L2,-440', '#ffffff', 1.5)
		+ R(-20, -180, 40, 30, 'brassV', 5) + rod('M-20,-165 L-46,-165', 'brass', 8) + P('M-50,-182 L-62,-182 L-58,-148 L-48,-148 Z', 'brass')
		+ R(-15, -62, 30, 118, 'leather', 4) + wrap(-15, 15, -60, 54, 11);
};
T.plumbBob = () => R(-22, -46, 44, 96, 'walnut', 10) + R(-28, -56, 56, 14, 'brassV', 5) + R(-28, 46, 56, 14, 'brassV', 5)
	+ [-34, -22, -10, 2, 14, 26, 38].map(y => line(`M-22,${y} L22,${y + 4}`, '#d9c8a0', 2.5)).join('')
	+ line('M0,-56 Q-60,-230 150,-330', '#d9c8a0', 3) + line('M0,-56 Q-60,-230 150,-330', OUT, 1, 'stroke-opacity=".5"')
	+ `<g transform="translate(150,-330) rotate(-62) scale(1.5)">
		${R(-8, -6, 16, 16, 'brass', 3)}
		${P('M-26,8 Q-32,40 -20,74 L0,128 L20,74 Q32,40 26,8 Z', 'brass')}
		${P('M-7,120 L0,146 L7,120 Z', 'steel')}
		${line('M-14,20 Q-20,50 -10,84', '#fff4c8', 3, 'stroke-opacity=".8"')}</g>`
	+ line('M70,-410 Q190,-420 250,-330', '#ffffff', 3, 'stroke-opacity=".25" stroke-dasharray="10 10"')
	+ line('M40,-440 Q220,-470 290,-330', '#ffffff', 3, 'stroke-opacity=".15" stroke-dasharray="10 10"');

// ---- the heroes ---------------------------------------------------------------------------------------------------
// swing: the arrow on the little screen view (box 200 x 120, crosshair at the middle), start to end.
const HEROES = {
	Ellis: {
		name: 'ELLIS RANSOM', role: 'THE REVENANT', color: '#5AD8FF',
		blurb: 'Shot on Ransom’s Point, raised by Sexton. Cattle-country tools, an ember that burns cyan.',
		hand: { glove: '#5b3a22', cuff: '#2b2f36', sleeve: '#a08866' },
		tools: [
			{ key: 'graveSpade', name: 'GRAVE SPADE', size: '95 cm', feel: 'STEADY', rot: 28,
				why: 'Sexton’s spare spade, the one that filled their grave; it digs them out of trouble now.',
				swing: 'Flat-blade backhand swat, high right to low left; a ringing clang.',
				mats: 'Black iron blade worn bright at the edge, ash shaft and D-grip, an ember crack glowing cyan.',
				arc: 'M175,14 Q130,70 25,104' },
			{ key: 'ransomBrand', name: 'RANSOM BRAND', size: '80 cm', feel: 'STEADY', rot: 24,
				why: 'The family ranch’s branding iron, kept hot by their own ember.',
				swing: 'Overhand chop, brand first, straight down; a hit leaves a glowing mark.',
				mats: 'Forged iron rod and ring brand, turned walnut grip, a brass collar; the head burns cyan-white.',
				arc: 'M118,6 Q112,50 100,104' },
			{ key: 'drovers', handY: 52, name: 'DROVER’S BOWIE', size: '40 cm', feel: 'QUICK', rot: 30,
				why: 'A cattle drover’s knife: plain, fast, always at the hip.',
				swing: 'Quick forehand slash, right to left at chest height.',
				mats: 'Clip-point steel blade, brass guard with a knuckle bow, stag-horn grip.',
				arc: 'M185,48 Q100,80 15,58' },
		],
	},
	Odessa: {
		name: 'ODESSA LARK', role: 'THE CARDSHARP', color: '#D04A68',
		blurb: 'A riverboat gambler who caught a cheat’s bullet. Gold, ebony and steel-edged cards.',
		hand: { glove: '#17161a', cuff: '#efe9de', sleeve: '#7a1f2e' },
		tools: [
			{ key: 'razorFan', name: 'RAZOR FAN', size: '5 cards', feel: 'QUICK', rot: 14,
				why: 'Her steel-edged trick cards, the same deck Fifty-Two Pickup throws.',
				swing: 'Backhand flick, low left to high right; the fan snaps open mid-cut.',
				mats: 'Cards rimmed in gold with honed steel edges, fanned from a black glove.',
				arc: 'M20,100 Q100,82 182,20' },
			{ key: 'rake', name: 'CROUPIER’S RAKE', size: '110 cm', feel: 'REACH', rot: 32,
				why: 'The card table’s rake, honed sharp: the house always takes its cut.',
				swing: 'Wide forehand sweep, right to left at waist height; the longest reach.',
				mats: 'Ebony rod, gold ferrules and pommel, brass rake head with a honed steel lip.',
				arc: 'M190,70 Q100,104 10,70' },
			{ key: 'spadeCane', name: 'SPADE CANE', size: '85 cm', feel: 'STEADY', rot: 26,
				why: 'A gambler’s walking cane with a gold spade for a head, held upside down.',
				swing: 'Short overhand rap with the gold head, straight down onto the crosshair.',
				mats: 'Ebony shaft, cast gold spade head set with a garnet, silver ferrule.',
				arc: 'M128,4 Q118,34 104,58' },
		],
	},
	Crane: {
		name: 'HOLLIS CRANE', role: 'THE UNPAID', color: '#B9D98C',
		blurb: 'A marshal forty years dead, owed a ferryman’s fare. Weathered wood, black iron, old coin.',
		hand: { glove: '#8a9a7e', cuff: '#d8d2c0', sleeve: '#4c5660' },
		tools: [
			{ key: 'ferryGaff', name: 'FERRY GAFF', size: '120 cm', feel: 'HEAVY', rot: 30,
				why: 'A ferryman’s boat hook for the man still owed his fare; it rhymes with his toll chain.',
				swing: 'Overhand hooking chop, then a yank back toward him.',
				mats: 'Weathered grey ash pole, black iron spike and hook, a fare coin nailed to the pole.',
				arc: 'M150,6 Q120,60 96,72 L124,98' },
			{ key: 'coinSap', name: 'COIN SAP', size: '30 cm', feel: 'QUICK', rot: 26,
				why: 'A lawman’s sap stuffed with the coins of fares never paid; they rattle as he walks.',
				swing: 'Short overhand cosh, high right to the centre; the coins clack.',
				mats: 'Cracked black leather, a split seam showing tarnished coins, a riveted fare coin, wrist strap.',
				arc: 'M170,10 Q140,30 104,60' },
			{ key: 'billy', name: 'MARSHAL’S BILLY', size: '60 cm', feel: 'STEADY', rot: 30,
				why: 'His old lawman’s club, forty years of notches cut in the grip.',
				swing: 'Forehand clubbing arc, right to left and a little downward.',
				mats: 'Turned hardwood, black iron end cap, a tin star set in, a leather thong.',
				arc: 'M185,30 Q110,58 20,92' },
		],
	},
	Gauge: {
		name: 'GAUGE', role: 'THE IRON HAND', color: '#FFB347',
		blurb: 'The railroad’s clockwork track-layer. Railroad iron, brass and firebox heat in a three-fingered hand.',
		hand: { glove: '#5d666f', cuff: null, sleeve: '#3a4048', three: true },
		tools: [
			{ key: 'spikeMaul', name: 'SPIKE MAUL', size: '110 cm', feel: 'HEAVY', rot: 22,
				why: 'The track-layer’s own hammer: it drove every spike on the line.',
				swing: 'Big overhead drive, straight down; the slowest wind-up, the hardest hit.',
				mats: 'Hickory handle with a leather wrap, narrow iron double head, faces glowing from the firebox.',
				arc: 'M104,2 Q110,50 100,110' },
			{ key: 'trackWrench', name: 'TRACK WRENCH', size: '90 cm', feel: 'STEADY', rot: 30,
				why: 'The railroad’s fishplate wrench, sized for a machine’s hand.',
				swing: 'Forehand haymaker, a wide arc right to left across the screen.',
				mats: 'Forged iron bar, open jaw lined in brass, box-end ring at the foot.',
				arc: 'M190,40 Q100,90 10,40' },
			{ key: 'pistonFist', name: 'PISTON FIST', size: 'built in', feel: 'QUICK', rot: 10, noHand: true,
				why: 'No tool at all: its right forearm drives a piston, the same one that fires Rail Spike.',
				swing: 'Straight jab into the crosshair; the piston fires and vents steam on a hit.',
				mats: 'Riveted iron knuckle plate, brass piston and cylinder, firebox-hot knuckles.',
				arc: 'M160,110 L104,64' },
		],
	},
	Pike: {
		name: 'WENDELL PIKE', role: 'THE SURVEYOR', color: '#3FBFAA',
		blurb: 'He walked every mile the railroad will run. Field gear: brass, hickory and a surveyor’s pole.',
		hand: { glove: '#9a7a52', cuff: '#2f8f80', sleeve: '#a39468' },
		tools: [
			{ key: 'rockPick', name: 'ROCK PICK', size: '35 cm', feel: 'QUICK', rot: 22,
				why: 'The surveyor’s field hammer, for chipping samples and driving marker stakes.',
				swing: 'Short overhand peck, pick first, down and to the left.',
				mats: 'Steel head with a square face and a curved pick, hickory handle bound in brass wire.',
				arc: 'M160,10 Q130,40 100,62' },
			{ key: 'tripodLeg', name: 'TRIPOD LEG', size: '120 cm', feel: 'REACH', rot: 34,
				why: 'One leg off his transit tripod: the steel spike that stakes his Tripwire.',
				swing: 'Forward thrust with the spike into the crosshair, then a short pull back.',
				mats: 'Banded survey pole with painted marks, brass clamp and shoe, steel spike.',
				arc: 'M150,104 L104,64 M104,64 L120,82' },
			{ key: 'plumbBob', name: 'PLUMB BOB', size: '1.2 m cord', feel: 'STEADY', rot: 4,
				why: 'His brass plumb bob on its line, the surveyor’s way to find true down.',
				swing: 'One whirl overhead, then a whip across, right to left.',
				mats: 'Solid brass bob with a steel point, waxed cord wound on a walnut spool.',
				arc: 'M150,40 C200,0 130,-6 140,26 Q110,70 15,60' },
		],
	},
};

// ---- the page -----------------------------------------------------------------------------------------------------
const PW = 488, PH = 790, PX = [36, 556, 1076], PY = 178;
function screenView(arc, col) {
	// A little first-person screen: the frame, the crosshair, the swing's path from wind-up (dot) to follow-through (arrowhead).
	return `<g><rect x="0" y="0" width="200" height="120" rx="8" fill="#0b0d12" fill-opacity=".85" stroke="#5AD8FF" stroke-opacity=".5" stroke-width="2"/>
		<g stroke="#ffffff" stroke-opacity=".5" stroke-width="2"><path d="M100,52 V57 M100,63 V68 M92,60 H97 M103,60 H108"/></g>
		<path d="${arc}" fill="none" stroke="${col}" stroke-width="13" stroke-opacity=".22" stroke-linecap="round"/>
		<path d="${arc}" fill="none" stroke="#FF8A2A" stroke-width="4" stroke-linecap="round" stroke-linejoin="round" marker-end="url(#arrow)"/>
		<text x="10" y="20" font-size="13" fill="#8fdcf0" font-weight="700" letter-spacing="1.5">SCREEN VIEW</text></g>`;
}
function startDot(arc) { const m = arc.match(/M\s*([-\d.]+),([-\d.]+)/); return `<circle cx="${m[1]}" cy="${m[2]}" r="6" fill="#FF8A2A" stroke="#0b0d12" stroke-width="2"/>`; }

function page(id) {
	const h = HEROES[id];
	const panels = h.tools.map((t, i) => {
		const x = PX[i], letter = 'ABC'[i];
		return `<g transform="translate(${x},${PY})">
			<rect width="${PW}" height="${PH}" rx="14" fill="#141821" fill-opacity=".92" stroke="#2c3442" stroke-width="2"/>
			<rect width="${PW}" height="${PH}" rx="14" fill="url(#grid)"/>
			<rect x="0" y="0" width="${PW}" height="6" rx="3" fill="${h.color}"/>
			<circle cx="46" cy="52" r="30" fill="#FF8A2A"/><text x="46" y="64" font-size="34" font-weight="700" fill="#0b0d12" text-anchor="middle">${letter}</text>
			<text x="90" y="54" font-size="31" font-weight="700" fill="#ffffff" letter-spacing="1">${t.name}</text>
			<text x="91" y="82" font-size="18" fill="#9fb0c4" letter-spacing="1.5">${t.size.toUpperCase()}</text>
			<g transform="translate(${PW - 22},60)"><rect x="-96" y="0" width="96" height="30" rx="15" fill="none" stroke="${h.color}" stroke-width="2"/>
			<text x="-48" y="21" font-size="16" font-weight="700" fill="${h.color}" text-anchor="middle" letter-spacing="1.5">${t.feel}</text></g>
			<ellipse cx="${PW / 2}" cy="290" rx="220" ry="180" fill="url(#hero)" opacity=".55"/>
			<g class="fit" data-box="14,92,${PW - 28},384"><g transform="rotate(${t.rot})" filter="url(#shadow)">${T[t.key]()}${t.noHand ? '' : `<g transform="translate(0,${t.handY || 0})">${fist(h.hand)}</g>`}</g></g>
			<g transform="translate(22,488)">${screenView(t.arc, h.color)}${startDot(t.arc)}</g>
			<foreignObject x="236" y="484" width="${PW - 252}" height="128"><div xmlns="http://www.w3.org/1999/xhtml" class="cap">
				<p><b>SWING</b> ${t.swing}</p></div></foreignObject>
			<foreignObject x="22" y="626" width="${PW - 44}" height="160"><div xmlns="http://www.w3.org/1999/xhtml" class="cap">
				<p><b>WHY</b> ${t.why}</p><p class="m"><b>MADE OF</b> ${t.mats}</p></div></foreignObject>
		</g>`;
	}).join('');
	return `<!doctype html><html><head><meta charset="utf-8"><style>
		@font-face { font-family: 'Chakra Petch'; src: url('${FONT('Content/UI/Fonts/ChakraPetch-Regular.ttf')}'); font-weight: 400; }
		@font-face { font-family: 'Chakra Petch'; src: url('${FONT('Content/UI/Fonts/ChakraPetch-Bold.ttf')}'); font-weight: 700; }
		@font-face { font-family: 'Rye'; src: url('${FONT('Art/Fonts/Rye-Regular.ttf')}'); }
		html, body { margin: 0; background: #0b0d12; }
		svg { display: block; font-family: 'Chakra Petch'; }
		.cap { font-family: 'Chakra Petch'; color: #dfe6ee; font-size: 21px; line-height: 1.25; }
		.cap p { margin: 0 0 10px; } .cap .m { color: #b9c4d0; font-size: 19px; }
		.cap b { color: #FF8A2A; letter-spacing: 1.5px; font-size: 16px; margin-right: 6px; }
	</style></head><body>
	<svg id="s" xmlns="http://www.w3.org/2000/svg" width="1600" height="1000" viewBox="0 0 1600 1000">${defs(h)}
		<marker id="arrow" viewBox="0 0 10 10" refX="5" refY="5" markerWidth="4" markerHeight="4" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 Z" fill="#FF8A2A"/></marker>
		<rect width="1600" height="1000" fill="#0b0d12"/>
		<radialGradient id="bg" cx=".5" cy="0" r="1"><stop offset="0" stop-color="${h.color}" stop-opacity=".16"/><stop offset=".6" stop-color="${h.color}" stop-opacity="0"/></radialGradient>
		<rect width="1600" height="1000" fill="url(#bg)"/>
		<rect x="36" y="34" width="8" height="98" fill="${h.color}"/>
		<text x="62" y="92" font-family="Rye" font-size="58" fill="#ffffff">${h.name}</text>
		<text x="64" y="128" font-size="24" font-weight="700" fill="${h.color}" letter-spacing="4">${h.role}</text>
		<text x="1564" y="70" font-size="28" font-weight="700" fill="#FF8A2A" text-anchor="end" letter-spacing="3">MELEE TOOL · PICK ONE</text>
		<text x="1564" y="102" font-size="19" fill="#c3ceda" text-anchor="end">Held and swung in the right hand; the gun drops to the left during the swing.</text>
		<text x="1564" y="130" font-size="19" fill="#8b98a8" text-anchor="end">${h.blurb}</text>
		<line x1="36" y1="154" x2="1564" y2="154" stroke="#5AD8FF" stroke-opacity=".35" stroke-width="1.5"/>
		${panels}
	</svg></body></html>`;
}

(async () => {
	const ids = process.argv.slice(2).length ? process.argv.slice(2) : Object.keys(HEROES);
	const browser = await chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1600, height: 1000 } });
	for (const id of ids) {
		const file = path.join(__dirname, `.page_${id}.html`);
		fs.writeFileSync(file, page(id));
		await pg.goto('file:///' + file.replace(/\\/g, '/'));
		await pg.evaluate(() => document.fonts.ready);
		// Fit each drawing (tool, hand and forearm as rotated) into its box, centred.
		await pg.evaluate(() => {
			for (const g of document.querySelectorAll('g.fit')) {
				const [x, y, w, h] = g.dataset.box.split(',').map(Number);
				const b = g.getBBox(), s = Math.min(w / b.width, h / b.height);
				g.setAttribute('transform', `translate(${x + (w - b.width * s) / 2 - b.x * s},${y + (h - b.height * s) / 2 - b.y * s}) scale(${s})`);
			}
		});
		const out = path.join(__dirname, `MeleeTools_${id}.png`);
		await pg.screenshot({ path: out });
		fs.unlinkSync(file);
		console.log('wrote', out);
	}
	await browser.close();
})();
