// Generates Art/Portraits/Frame.svg and Window.svg (1024x1024): the ornamental frame drawn over the portrait
// busts (original vector art) and the window the painting is clipped to.
//   node Art/Portraits/frame/gen.js [Ellis|Odessa|Crane|Gauge|Pike]
// No argument = Ellis: Frame.svg and Window.svg. Another hero writes Frame_<Hero>.svg: the same band, window,
// corners, sun-ring, crow feathers and Sexton's ledger (the saga's), the hero's own motifs in place of Ellis's.
// Filters always sit on untransformed groups (lighting stays top-left; Chrome mis-lights mirrored filter groups).
// The saga's shared pieces live in common.js, the other heroes' kits in heroes.js; Ellis's own kit is below.
const fs = require('fs');
const L = require('./lib.js');
const { D2R, R, P, INK, poly, smooth, smoothSeg, curl, bez, resample, tangents, ribbon, taper, swell, circle, ringPath, mirror, lin, rad, metal, drop, grit, glow, blurF,
	polyLen, pip, linkRing, chainLinks, starPts, gearPath, hexPts, compassRose } = L;
const OUT = process.env.OUT || require('path').join(__dirname, '..') + '/';
const ONLY = process.env.ONLY ? process.env.ONLY.split(',') : null;
const { el, sym, mirD, WA, WB, WN, winPath, rectPath, defs, band, corners, innerShadow, cyanLine, sunRing, topCrest, ledger,
	BCL, BT, BTS, SERIF, cutText, raisedText, SPARKS, sparks, onPath, at, lay } = require('./common.js');
const { KITS } = require('./heroes.js');

// ---------- bandolier (left, mirrored) ----------
function cartridge() {
	return `<rect x="-24" y="-6.6" width="4.2" height="13.2" rx="1.2" fill="#8a5f1c" stroke="${INK}" stroke-width="1"/>` +
		`<rect x="-20.5" y="-5.5" width="33" height="11" fill="url(#gCart)" stroke="${INK}" stroke-width="1"/>` +
		`<path d="M12.5 -5.2L19 -5.2Q28.5 -4.5 29.5 0Q28.5 4.5 19 5.2L12.5 5.2Z" fill="url(#gLeadC)" stroke="${INK}" stroke-width="1"/>` +
		`<path d="M14 -3.2Q24 -3.4 27 -1" fill="none" stroke="#f0ebe0" stroke-width="1" stroke-opacity="0.6" stroke-linecap="round"/>` +
		`<path d="M-19 -3H11" stroke="#fff1c8" stroke-width="1.2" stroke-opacity="0.6"/>`;
}
function bandolier() {
	const at = t => { const i = Math.round(t * (BCL.length - 1)); return { p: BCL[i], t: BT[i], i }; };
	const frame = (t, inner) => { const a = at(t); const n = [-a.t[1], a.t[0]]; return `<g transform="translate(${R(a.p[0])} ${R(a.p[1])}) rotate(${R(Math.atan2(n[1], n[0]) / D2R)})">${inner}</g>`; };
	const edge = (w, i0 = 0, i1 = BCL.length) => BCL.slice(i0, i1).map((p, j) => { const i = j + i0; return [p[0] - BT[i][1] * w, p[1] + BT[i][0] * w]; });
	let strap = el(ribbon(BCL, () => 17));
	// folded end through the ring at the top, tongue hanging past the buckle
	strap += el(ribbon(resample(bez([78, 196], [80, 186], [78, 178], [74, 172], 10), 10), () => 12));
	strap += el(ribbon(resample(bez([70, 478], [76, 500], [70, 520], [60, 544], 20), 20), t => 12 - t * 3));
	let stitch = `<path d="${smooth(edge(13.5))}${smooth(edge(-13.5))}" fill="none" stroke="#d6b48a" stroke-opacity="0.45" stroke-width="0.9" stroke-dasharray="3 2.4"/>`;
	// flat strip sewn between cartridges
	const i0 = Math.round(0.14 * (BCL.length - 1)), i1 = Math.round(0.78 * (BCL.length - 1));
	let strip = el(ribbon(BCL.slice(i0, i1), () => 10));
	let carts = '', loops = '';
	for (const t of BTS) {
		carts += frame(t, cartridge());
		loops += frame(t, `<rect x="-10.5" y="-8.2" width="21" height="16.4" rx="4" fill="url(#gLoop)"/>` +
			`<path d="M-8.5 -6.5V6.5M8.5 -6.5V6.5" stroke="#d6b48a" stroke-opacity="0.55" stroke-width="0.9" stroke-dasharray="1.8 1.6"/>` +
			`<path d="M-7 -3.6H7" stroke="#c87858" stroke-opacity="0.55" stroke-width="1.2"/>`);
	}
	let between = '';
	for (let k = 0; k < BTS.length - 1; k++) between += frame((BTS[k] + BTS[k + 1]) / 2, `<path d="M-9 -2.5H9M-9 2.5H9" stroke="#d6b48a" stroke-opacity="0.5" stroke-width="0.9" stroke-dasharray="1.8 1.6"/>`);
	const hasp = `<rect x="62" y="162" width="34" height="22" rx="5"/>`;
	const haspBr = circle(70, 173, 3) + circle(88, 173, 3) + `<path d="${ringPath(78, 187, 6.5, 10.5)}" fill-rule="evenodd"/>`;
	const bk = at(0.95);
	const bang = Math.atan2(bk.t[1], bk.t[0]) / D2R;
	const buckle = `<g transform="translate(${R(bk.p[0])} ${R(bk.p[1])}) rotate(${R(bang)})"><path d="M-8 -15H8V15H-8ZM-4 -11V11H4V-11Z" fill-rule="evenodd"/><rect x="-1.6" y="-13" width="3.2" height="11" rx="1"/></g>` +
		`<path d="M52 538L68 541L59 556Z"/>` + circle(71, 210, 2.6);
	return `<g filter="url(#fLeather)" fill="url(#gStrap)">${mirD(strap)}</g>` +
		mirD(stitch) +
		`<g filter="url(#fLeatherSm)" fill="#5e2a1e">${mirD(strip)}</g>` + mirD(between) +
		`<g filter="url(#fDropSm)">${mirD(carts)}</g>` +
		`<g filter="url(#fLeatherSm)">${mirD(loops)}</g>` +
		`<g fill="url(#gIronL)" filter="url(#fIron)">${mirD(hasp)}</g>` +
		`<g fill="url(#gBrass)" filter="url(#fBrassSm)">${mirD(haspBr + buckle)}</g>`;
}

// ---------- barbed wire ----------
const WX = [126, 424], WP = 58;
function twist(pts, amp = 1.4, per = 7) {
	const T = tangents(pts);
	let s = 0; const A = [], B = [];
	for (let i = 0; i < pts.length; i++) {
		if (i) s += Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]);
		const o = amp * Math.sin(2 * Math.PI * s / per);
		A.push([pts[i][0] - T[i][1] * o, pts[i][1] + T[i][0] * o]);
		B.push([pts[i][0] + T[i][1] * o, pts[i][1] - T[i][0] * o]);
	}
	return [smooth(A), smooth(B)];
}
function wirePaths(front) {
	let a = '', b = '';
	for (let x = WX[0]; x < WX[1]; x += WP) {
		const pts = front ? bez([x, 997], [x + 6, 968], [x + 26, 940], [x + 33, 917], 30) : bez([x + 33, 917], [x + 44, 904], [x + 52, 1008], [x + WP, 997], 30);
		const [pa, pb] = twist(resample(pts, 60));
		a += pa; b += pb;
	}
	return { a, b };
}
function wireDraw(w, dim) {
	const steel = dim ? '#3d434b' : '#858c95';
	return `<g fill="none" stroke-linecap="round"><path d="${w.b}" stroke="${INK}" stroke-width="3.9"/><path d="${w.b}" stroke="${dim ? '#2b3138' : '#5e656e'}" stroke-width="2.2"/>` +
		`<path d="${w.a}" stroke="${INK}" stroke-width="3.9"/><path d="${w.a}" stroke="${steel}" stroke-width="2.3"/>` +
		(dim ? '' : `<path d="${w.a}" stroke="#e0dbcf" stroke-width="0.7" stroke-opacity="0.7" stroke-dasharray="2 5"/>`) + `</g>`;
}
function barbs() {
	let s = '';
	let k = 0;
	for (let x = WX[0]; x < WX[1]; x += WP, k++) {
		const pts = bez([x, 997], [x + 6, 968], [x + 26, 940], [x + 33, 917], 30);
		const i = 14, p = pts[i];
		const tg = tangents(pts)[i];
		const ta = Math.atan2(tg[1], tg[0]) / D2R;
		for (const da of [62, 118, 242, 298]) {
			const a = (ta + da + (k % 2 ? 8 : -8)) * D2R;
			const d = [Math.cos(a), Math.sin(a)], n = [-d[1], d[0]];
			const tip = [p[0] + d[0] * 13, p[1] + d[1] * 13];
			s += el(poly([[p[0] + n[0] * 1.5, p[1] + n[1] * 1.5], tip, [p[0] - n[0] * 1.5, p[1] - n[1] * 1.5]]));
		}
		s += `<ellipse cx="${R(p[0])}" cy="${R(p[1])}" rx="2.8" ry="4.6" transform="rotate(${R(ta + 90)} ${R(p[0])} ${R(p[1])})"/>`;
	}
	return s;
}
function wireBack() { return `<g filter="url(#fDropNoOl)">${mirD(wireDraw(wirePaths(false), true))}</g>`; }
function wireFront() {
	return `<g filter="url(#fDropNoOl)">${mirD(wireDraw(wirePaths(true), false))}</g>` +
		`<g fill="#8a919a" filter="url(#fIron)">${mirD(barbs())}</g>`;
}

// ---------- revolvers ----------
const GUN = 'translate(634 944) rotate(11)';
function revolverParts() {
	const B = 250;
	const iron =
		`<path d="M${-B} -9.6L-26 -10.8L-26 0.8L${-B} 0.2Z"/>` +
		`<rect x="${-B - 1.5}" y="-10.4" width="6.5" height="11.4" rx="1.5"/>` +
		`<path d="M${-B + 6} -9.4Q${-B + 8} -15.5 ${-B + 12} -9.6Z"/>` +
		`<path d="M-26 0.8L-104 0.8Q-110 0.8 -111 4.6Q-110 8.6 -104 8.6L-26 8.6Z"/>` +
		circle(-113.5, 4.7, 3.4) +
		`<path d="M-30 -12Q-30 -15.5 -26 -15.5L22 -15.5L30 -10L33 2Q33 10 28 15L8 17Q-6 17 -16 13Q-26 9 -30 2Z"/>` +
		`<path d="M23 -13L26 -22Q29 -30 38 -31.5L44 -30.5Q45 -27 41.5 -26Q34.5 -24 32.5 -18L31 -10Z"/>` +
		`<path d="M-19.5 -9Q-19.5 -16 -12.5 -16L12.5 -16Q19.5 -16 19.5 -9L19.5 7Q19.5 14 12.5 14L-12.5 14Q-19.5 14 -19.5 7Z"/>`;
	const brass =
		`<path d="M30 6C42 18 55 36 70 52L77 60Q70 70 56 70Q48 69 46 65C42 51 30 35 12 18Z"/>` +
		`<path d="M-8 15C-12 37 25 43 32 15L27 15C21 35 -3 31 -3 15Z"/>`;
	const bone = `<path d="M28 12C38 22 50 37 64 55Q61 62 52 62Q48 60 47 57C43 46 31 33 17 20Z"/>`;
	const det =
		`<path d="M11 17Q15 24 9 30" stroke="${INK}" stroke-width="2.6" fill="none" stroke-linecap="round"/>` +
		`<g fill="#0a0c0f" opacity="0.9"><rect x="-13" y="-10.5" width="26" height="3.8" rx="1.9"/><rect x="-13" y="-2.6" width="26" height="3.8" rx="1.9"/><rect x="-13" y="5.2" width="26" height="3.4" rx="1.7"/></g>` +
		`<g stroke="#9aa1aa" stroke-width="0.8" stroke-opacity="0.55"><path d="M-12 -5.8H12M-12 2.1H12M-12 9.6H12"/></g>` +
		`<path d="M-19.5 -12V10" stroke="#9aa1aa" stroke-width="1.4" stroke-opacity="0.45"/>` +
		`<path d="M${-B + 4} -7.6H-28" stroke="#b4bac2" stroke-width="1.3" stroke-opacity="0.55"/>` +
		`<path d="M-108 6.6H-28" stroke="#08090b" stroke-width="1" stroke-opacity="0.8"/>` +
		`<path d="M33 -28.5L39 -29.5M33.5 -26.5L39.5 -27.5" stroke="#08090b" stroke-width="0.7"/>` +
		circle(25, -2, 1.7, 'fill="#c9c4b8"') + circle(-24, 4, 1.5, 'fill="#c9c4b8"') +
		circle(45, 38, 2.2, 'fill="#e0b44a" stroke="#4a3008" stroke-width="0.6"') +
		`<path d="M22 -7q5 3 1 8M-27 -9q-1 4 2 7" stroke="#c9c4b8" stroke-width="0.7" fill="none" stroke-opacity="0.5"/>` +
		`<path d="M33 24Q43 36 51 50M37 21Q47 33 55 46" stroke="#a8987a" stroke-width="0.7" fill="none" stroke-opacity="0.8"/>`;
	return { iron, brass, bone, det };
}
function revolvers() {
	const g = revolverParts();
	const w = c => `<g transform="${GUN}">${c}</g>`;
	return `<g fill="url(#gIronV)" filter="url(#fIron)">${mirD(w(g.iron))}</g>` +
		`<g fill="url(#gBrassD)" filter="url(#fBrassSm)">${mirD(w(g.brass))}</g>` +
		`<g fill="url(#gBone)" filter="url(#fDropSm)">${mirD(w(g.bone))}</g>` + mirD(w(g.det));
}

// ---------- grave plank ----------
const PK = { x0: 424, x1: 600, top: 878, sh: 912, bot: 1012 };
function plankPath() {
	const { x0, x1, top, sh, bot } = PK;
	return `M${x0 + 2} ${bot}L${x0} ${sh + 6}C${x0 + 6} ${sh - 14} 488 ${top + 6} 512 ${top}C536 ${top + 6} ${x1 - 6} ${sh - 14} ${x1} ${sh + 6}L${x1 - 1} ${bot - 6}L${x1 - 8} ${bot}L${x1 - 18} ${bot - 4}L${x1 - 30} ${bot + 1}L${x0 + 40} ${bot}L${x0 + 30} ${bot - 5}L${x0 + 18} ${bot + 1}Z`;
}
function plank() {
	let s = '';
	defs.push(`<clipPath id="cPlank"><path d="${plankPath()}"/></clipPath>`);
	s += `<g filter="url(#fWood)"><path d="${plankPath()}" fill="url(#gWood)" filter="url(#fGritWood)"/></g>`;
	let grain = '';
	const xs = [431, 440, 452, 461, 470, 483, 494, 506, 519, 531, 544, 556, 568, 579, 590];
	xs.forEach((x, i) => { const w = (i % 3) - 1; grain += `M${x} 870C${x + w * 3} 905 ${x - w * 4 + (x > 540 && x < 570 ? 9 : 0)} 950 ${x + w * 2} 1015`; });
	grain += `M551 958a5 9 0 1 0 10 0a5 9 0 1 0 -10 0M547 958a9 15 0 1 0 18 0a9 15 0 1 0 -18 0`;
	s += `<g clip-path="url(#cPlank)" fill="none"><path d="${grain}" stroke="#2e1f14" stroke-width="1.1" stroke-opacity="0.55"/><path d="${grain}" stroke="#c8a274" stroke-width="0.8" stroke-opacity="0.22" transform="translate(1.6 0)"/></g>`;
	s += `<g clip-path="url(#cPlank)"><path d="${plankPath()}" fill="none" stroke="#1a0f08" stroke-width="10" stroke-opacity="0.55" filter="url(#fB3)"/></g>`;
	s += `<path d="M470 1012L472 996L468 984L471 968M590 930L583 936L586 944M430 960l6 3" fill="none" stroke="#1a110a" stroke-width="1.6" stroke-linecap="round"/>`;
	// carved sun-ring emblem
	let em = `M517.5 902a5.5 5.5 0 1 0 -11 0a5.5 5.5 0 1 0 11 0`;
	for (let i = 0; i < 8; i++) { const a = i * 45 * D2R; em += `M${R(512 + Math.sin(a) * 8.5)} ${R(902 - Math.cos(a) * 8.5)}L${R(512 + Math.sin(a) * 12.5)} ${R(902 - Math.cos(a) * 12.5)}`; }
	s += `<path d="${em}" fill="none" stroke="#c8a274" stroke-opacity="0.5" stroke-width="2.2" stroke-linecap="round" transform="translate(0 1.2)"/><path d="${em}" fill="none" stroke="#1e140d" stroke-width="2.2" stroke-linecap="round"/>`;
	const font = `font-family="Caladea, 'DejaVu Serif', FreeSerif, serif" font-weight="bold" text-anchor="middle"`;
	for (const [t, y, fsz, ls] of [['ELLIS', 941, 29, 4], ['RANSOM', 975, 29, 2.5]]) {
		s += `<text x="512" y="${y + 1.3}" ${font} font-size="${fsz}" letter-spacing="${ls}" fill="#c8a274" fill-opacity="0.55">${t}</text>`;
		s += `<text x="512" y="${y}" ${font} font-size="${fsz}" letter-spacing="${ls}" fill="#1c120b">${t}</text>`;
	}
	s += `<path d="M482 951H542" stroke="#c8a274" stroke-opacity="0.45" stroke-width="1.2" transform="translate(0 1)"/><path d="M482 951H542" stroke="#1c120b" stroke-width="1.5"/>`;
	let nails = '';
	for (const p of [[437, 922], [587, 922], [437, 999], [587, 999]]) nails += circle(p[0], p[1], 4.2);
	s += `<g fill="none" stroke="#3a1c10" stroke-opacity="0.45" stroke-width="2.4" filter="url(#fB1)"><path d="M437 927V942M587 927V937M437 1004V1010"/></g>`;
	s += `<g fill="url(#gIronV)" filter="url(#fIron)">${nails}</g>`;
	return s;
}

// ---------- dried flowers ----------
function flowers() {
	let s = '';
	const stems = [[[446, 1016], [438, 1004], [424, 994], [408, 988]], [[448, 1016], [444, 1002], [434, 988], [428, 976]], [[450, 1016], [452, 1004], [454, 996], [458, 986]]];
	let st = '';
	for (const c of stems) st += `M${P(c[0])}C${P(c[1])} ${P(c[2])} ${P(c[3])}`;
	st += `M440 1003Q432 1000 424 1006M446 996Q452 990 452 984`;
	s += `<path d="${st}" fill="none" stroke="#1a1310" stroke-width="1.8" stroke-linecap="round"/>`;
	const head = (x, y, r, rot, n = 6) => {
		let h = '';
		for (let i = 0; i < n; i++) {
			const a = (rot + i * 360 / n) * D2R;
			const tip = [x + Math.cos(a) * r, y + Math.sin(a) * r];
			const l = [x + Math.cos(a - 0.5) * r * 0.5, y + Math.sin(a - 0.5) * r * 0.5], rr = [x + Math.cos(a + 0.5) * r * 0.5, y + Math.sin(a + 0.5) * r * 0.5];
			h += `M${R(x)} ${R(y)}Q${P(l)} ${P(tip)}Q${P(rr)} ${R(x)} ${R(y)}Z`;
		}
		return h;
	};
	const hs = [[408, 988, 8, 10], [428, 976, 9, 40], [458, 985, 7, 0], [424, 1006, 5, 20]];
	s += `<path d="${hs.map(h => head(...h)).join('')}" fill="#17121a" stroke="#5a4c54" stroke-width="0.6"/>`;
	s += hs.map(p => circle(p[0], p[1], 2.2, 'fill="#5a3a22"')).join('');
	s += `<path d="M444 1008q-12 -4 -18 -14q11 2 18 14zM452 1000q10 -8 20 -6q-7 8 -20 6z" fill="#2a2618" stroke="${INK}" stroke-width="0.7"/>`;
	s += `<path d="M441 1010q7 -3 13 0M447 1009q-6 6 -9 10M448 1009q5 6 10 8" fill="none" stroke="#b5a689" stroke-width="1.8" stroke-linecap="round"/>`;
	return `<g filter="url(#fDropSm)">${s}</g>`;
}

// ---------- lantern (lower left) ----------
const LX = 72;
function lantern() {
	let s = '';
	s += circle(LX, 924, 160, 'fill="url(#gLampGlow)" mask="url(#mGlow)"');
	s += `<g filter="url(#fIron)"><path d="M47 858C42 826 55 804 ${LX} 802C89 804 102 826 97 858" fill="none" stroke="url(#gIronL)" stroke-width="3.6"/></g>`;
	const glass = `M50 862C40 892 40 948 50 980L94 980C104 948 104 892 94 862Z`;
	defs.push(`<clipPath id="cGlass"><path d="${glass}"/></clipPath>`);
	defs.push(rad('gGlass2', [[0, '#fff6d8'], [0.16, '#ffd27a'], [0.42, '#ff9f1c'], [0.78, '#d0600f'], [1, '#7a2c08']], LX, 926, 50, LX, 930, true));
	defs.push(lin('gSoot', [[0, '#1e0c04', 0.75], [0.25, '#1e0c04', 0.25], [0.45, '#1e0c04', 0], [0.9, '#1e0c04', 0], [1, '#1e0c04', 0.4]], 0, 862, 0, 980, true));
	s += `<path d="${glass}" fill="#ff9f1c" opacity="0.75" filter="url(#fB6)"/>`;
	s += `<path d="${glass}" fill="url(#gGlass2)"/>`;
	s += `<path d="${glass}" fill="url(#gSoot)"/>`;
	// burner + flame
	s += `<path d="M${LX - 11} 968Q${LX} 952 ${LX + 11} 968Z" fill="url(#gBrass)" stroke="${INK}" stroke-width="0.8"/>`;
	s += `<rect x="${LX - 3}" y="950" width="6" height="8" fill="#3a220e"/>`;
	s += `<path d="M${LX} 892C${LX + 11} 912 ${LX + 13} 934 ${LX} 952C${LX - 13} 934 ${LX - 11} 912 ${LX} 892Z" fill="#ffd27a" opacity="0.8" filter="url(#fB3)"/>`;
	s += `<path d="M${LX} 902C${LX + 7} 916 ${LX + 9} 934 ${LX} 949C${LX - 9} 934 ${LX - 7} 916 ${LX} 902Z" fill="#fff3c4"/>`;
	s += `<path d="M${LX} 914C${LX + 3.5} 924 ${LX + 4.5} 936 ${LX} 945C${LX - 4.5} 936 ${LX - 3.5} 924 ${LX} 914Z" fill="#ffffff"/>`;
	s += `<path d="M${LX - 3} 949Q${LX} 943 ${LX + 3} 949Z" fill="#6ee7ff" opacity="0.5"/>`;
	// glass highlights
	s += `<g clip-path="url(#cGlass)"><path d="M54 874C49 900 49 936 55 962" fill="none" stroke="#fffbe6" stroke-width="2.4" stroke-opacity="0.38" stroke-linecap="round"/>` +
		`<ellipse cx="88" cy="884" rx="2.2" ry="6" fill="#fffbe6" opacity="0.45"/></g>`;
	s += `<path d="${glass}" fill="none" stroke="${INK}" stroke-width="1.2"/>`;
	let cage = '';
	cage += `M47 860C36 892 36 950 46 982L52 982C43 950 43 892 53 860Z`;
	cage += `M97 860C108 892 108 950 98 982L92 982C101 950 101 892 91 860Z`;
	cage += `M59.2 862C55 892 55 950 59.2 980L60.8 980C57 950 57 892 60.8 862ZM84.8 862C89 892 89 950 84.8 980L83.2 980C87 950 87 892 83.2 862Z`;
	for (const y of [892, 950]) cage += `M41 ${y}Q${LX} ${y + 8} 103 ${y}L103 ${y + 3.4}Q${LX} ${y + 11.4} 41 ${y + 3.4}Z`;
	cage += `M40 852H104V862H40ZM38 978H106V988H38ZM34 988H110L106 1004H38Z`;
	s += `<g fill="url(#gIronL)" filter="url(#fIron)"><path d="${cage}"/></g>`;
	let cap = `<path d="M42 854C46 836 58 828 ${LX} 826C86 828 98 836 102 854Z"/>`;
	cap += `<rect x="${LX - 8}" y="814" width="16" height="14" rx="2"/>`;
	cap += `<path d="M${LX - 13} 817H${LX + 13}L${LX + 8} 809H${LX - 8}Z"/>`;
	cap += `<rect x="36" y="986" width="72" height="5" rx="1.5"/>`;
	s += `<g fill="url(#gBrass)" filter="url(#fBrass)">${cap}</g>`;
	s += `<g fill="#1a1006">${[55, 65, 79, 89].map(x => `<ellipse cx="${x}" cy="845" rx="2" ry="3.4"/>`).join('')}</g>`;
	s += `<g fill="#ff9f1c" opacity="0.6">${[55, 65, 79, 89].map(x => `<ellipse cx="${x}" cy="846" rx="1" ry="2"/>`).join('')}</g>`;
	s += `<g fill="url(#gBrass)" filter="url(#fBrassSm)">${circle(44, 996, 2.4)}${circle(100, 996, 2.4)}<path d="${ringPath(LX, 804, 4, 7.5)}" fill-rule="evenodd"/></g>`;
	return s;
}

// ---------- casings + sparks ----------
function casing(x, y, rot, sc = 1.15) {
	return `<g transform="translate(${x} ${y}) rotate(${rot}) scale(${sc})">` +
		`<rect x="-2.5" y="-5.6" width="3" height="11.2" rx="1" fill="#8a5f1c"/>` +
		`<rect x="0" y="-4.6" width="22" height="9.2" fill="url(#gCart)"/>` +
		`<ellipse cx="22" cy="0" rx="2" ry="4.6" fill="#2a1a06" stroke="#e0b44a" stroke-width="0.9"/>` +
		`<path d="M1 -2.6H20" stroke="#fff1c8" stroke-width="1" stroke-opacity="0.7"/></g>`;
}
function looseBits() {
	let c = '';
	c += casing(150, 1004, 14) + casing(178, 1012, -28) + casing(318, 1010, 168) + casing(120, 1014, -8);
	c += casing(708, 1010, 196) + casing(800, 1006, 160) + casing(826, 1014, -16);
	let s = `<g filter="url(#fDropSm)">${c}</g>`;
	const sp = [[98, 816, 1.8], [110, 788, 1.2], [86, 770, 1.5], [124, 830, 1.1], [150, 986, 1.3], [250, 1003, 1.1], [372, 999, 1.4], [652, 1006, 1.2], [770, 1000, 1.5], [540, 14, 1.3], [486, 10, 1.1], [528, 4, 0.9], [36, 756, 1.0], [114, 742, 0.9]];
	s += `<g fill="#ffd27a" filter="url(#fGlow2)">${sp.map(p => circle(p[0], p[1], p[2] + 0.4)).join('')}</g>`;
	s += `<g fill="#fffbe6">${sp.map(p => circle(p[0], p[1], p[2] * 0.55)).join('')}</g>`;
	return s;
}

// ---------- assemble ----------
// Ellis's layers are the original frame; each hero's kit keeps the saga's pieces and swaps the rest.
const HEROES = {
	Ellis: { name: 'Ellis Ransom', layers: { inner: innerShadow, wireB: wireBack, band, cyan: cyanLine, corners, crest: topCrest, bandolier, wireF: wireFront, guns: revolvers, plank, flowers, lantern, ledger, bits: looseBits } },
	...KITS,
};
const arg = (process.argv[2] || 'Ellis').toLowerCase();
const HERO = Object.keys(HEROES).find(k => k.toLowerCase() === arg);
if (!HERO) { console.error(`unknown hero "${process.argv[2]}" (one of ${Object.keys(HEROES).join(', ')})`); process.exit(1); }
const kit = HEROES[HERO];
if (kit.defs) kit.defs();
let body = '';
for (const [k, fn] of Object.entries(kit.layers)) {
	const out = fn();
	if (!ONLY || ONLY.includes(k)) body += `<g id="L_${k}">${out}</g>\n`;
}
let svg = `<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="1024" height="1024" viewBox="0 0 1024 1024">\n` +
	`<!-- Portrait frame (${kit.name}): drawn over the bust painting; the window is Window.svg. Original vector art. -->\n` +
	`<defs>\n${defs.join('\n')}\n</defs>\n${body}</svg>\n`;
svg = svg.replace(/<use href="(#[^"]+)"/g, '<use href="$1" xlink:href="$1"');
const file = HERO === 'Ellis' ? 'Frame.svg' : `Frame_${HERO}.svg`;
fs.writeFileSync(OUT + file, svg);
if (HERO === 'Ellis') {
	// every hero shares this window, so only Ellis's run (the default) writes it
	const win = `<svg xmlns="http://www.w3.org/2000/svg" width="1024" height="1024" viewBox="0 0 1024 1024">\n` +
		`<!-- Portrait window: the bust painting is clipped to this shape (inner edge of the frame). -->\n<path d="${winPath(0)}" fill="#ffffff"/>\n</svg>\n`;
	fs.writeFileSync(OUT + 'Window.svg', win);
}
console.log(file, (svg.length / 1024).toFixed(1), 'KB');
