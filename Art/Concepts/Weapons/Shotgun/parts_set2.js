// Set 2: Roundhouse (Roundhouse Rail Works: railway-heavy, built to be thrown off a flatcar). Six new designs for the
// Ranchhand's Armored body, Shielded barrel, Brake muzzle, Drum20 magazine, Prism sight and Mule stock: freight-car
// oxblood, black iron, big brass rivets, cream stencil stripes; boxy stepped slabs, riveted plates, louvres, hex bolts,
// railway motifs (a cab and louvred hood, a boarding-step trigger guard, a boiler-plate barrel roof, a driving-wheel
// drum, a signal-lamp prism, a buffer-beam butt). Interfaces as Ranchhand.py (contract in the README).
const L = require('./lib');
const { N, pth, R4, shp, circ, ring, pl, wear, ao, screw, cb, cd, sk, clip, inside, pipe, hexHead, rounded, r2, star, seam,
	GRADS, lin, rad, knurlRing } = L;

GRADS.push(
	lin('s2oxCyl', [[0, '#3e140f'], [0.22, '#7d2f24'], [0.33, '#b4584a'], [0.48, '#7d2f24'], [1, '#3a120d']]),
	lin('s2iron', [[0, '#4a4b4f'], [0.5, '#2e2f33'], [1, '#1c1d20']]),
	lin('s2ironHi', [[0, '#5c5e63'], [1, '#3a3b3f']]),
	lin('s2ironLo', [[0, '#2a2b2e'], [1, '#141517']]),
	lin('s2rubber', [[0, '#3a3a3a'], [0.5, '#262626'], [1, '#151515']]),
	lin('s2rubberH', [[0, '#151515'], [0.3, '#3a3a3a'], [0.6, '#262626'], [1, '#121212']], false),
	rad('s2hub', [[0, '#fff3c8'], [0.35, '#e6b45c'], [0.8, '#a3762b'], [1, '#5f400f']]),
	'<pattern id="s2tread" width="9" height="9" patternUnits="userSpaceOnUse"><path d="M1,7L4,2M5.5,8.5L8,4.5" stroke="#9a9da2" stroke-width="1.6" stroke-linecap="round" opacity=".55"/><path d="M1.6,7.6L4.6,2.6" stroke="#000" stroke-width="1" opacity=".4"/></pattern>',
	'<pattern id="s2perf" width="6" height="6" patternUnits="userSpaceOnUse"><circle cx="3" cy="3" r="1.1" fill="#000" opacity=".35"/></pattern>',
);

const CREAM = '#efe3c4';
const RR = (V, a0, v0, a1, v1, r, fill, filt, extra = '') => shp(pth(V, R4(a0, v0, a1, v1), r), fill, filt, extra);
const bR = (V, a, v, r = 0.3) => ao(circ(V, a, v, r, 'url(#rBrass)', 'bS')) +
	circ(V, a, v, r * 0.35, '#fff6d8', null, `opacity=".6" transform="translate(${r2(-r * V.s * 0.3)},${r2(-r * V.s * 0.3)})"`);
const rowR = (V, a0, a1, v, step, r) => { let s = ''; for (let a = a0; a <= a1 + 1e-6; a += step) s += bR(V, a, v, r); return s; };
const colR = (V, a, v0, v1, step, r) => { let s = ''; for (let v = v0; v <= v1 + 1e-6; v += step) s += bR(V, a, v, r); return s; };
// Cream stencil stripes, slanted, inside an outline.
function stripes(V, d, a0, a1, v0, v1, w = 0.55, gap = 0.75, lean = 1.0) {
	let t = '';
	for (let a = a0 - (v1 - v0) * lean; a < a1; a += w + gap) t += shp(pth(V, [[a, v0], [a + w, v0], [a + w + (v1 - v0) * lean, v1], [a + (v1 - v0) * lean, v1]], 0), CREAM, null, 'opacity=".92"');
	return inside(d, t);
}
// A cut edge highlight and shade along a box's top and bottom (side views).
const edges = (V, a0, v0, a1, v1, hi = '#ffffff', op = 0.35) => pl(V, [[a0 + 0.1, v1 - 0.05], [a1 - 0.1, v1 - 0.05]], hi, 1, `opacity="${op}"`) +
	pl(V, [[a0 + 0.1, v0 + 0.04], [a1 - 0.1, v0 + 0.04]], '#000', 1.2, 'opacity=".35"');
const railTeeth = (V, u0, u1, v, fill) => { let s = ''; for (let u = u0 + 0.35; u + 0.55 <= u1 - 0.2; u += 1.0) s += RR(V, u, v + 0.4, u + 0.53, v + 0.95, 0.05, fill, 'mS'); return s; };
// The picatinny rail seen end on, base at v 2.2 (top of the teeth 3.15).
const RAILX = [[-1.05, 2.2], [1.05, 2.2], [1.05, 2.55], [0.86, 2.67], [1.05, 2.8], [0.84, 3.15], [-0.84, 3.15], [-1.05, 2.8], [-0.86, 2.67], [-1.05, 2.55]];
const evenodd = (d1, d2, fill, filt) => `<path d="${d1}${d2}" fill="${fill}" fill-rule="evenodd"${filt ? ` filter="url(#${filt})"` : ''}/>`;

// ==== Body_Armored: a locomotive cab and louvred hood on an oxblood receiver ==========================================
// Stepped: a tall riveted iron cab plate over the rear (its skirt hangs below the receiver), a flat oxblood waist (a
// brass builder's plate on the left, the ejection port under a hinged hatch on the right), an iron hood with four
// ember louvres at the front; a boarding-step trigger guard with a tread; cream end stripes.
const CORE = [[0, -3.0], [20, -3.0], [20, 1.7], [19.4, 2.2], [0.6, 2.2], [0, 1.8]];
const CAB = [[-0.3, -4.0], [3.0, -4.0], [3.5, -3.0], [5.8, -3.0], [5.8, 2.2], [-0.3, 2.2]];
const HOOD = [[14.0, -3.0], [20.0, -3.0], [20.0, 1.6], [19.4, 2.2], [14.0, 2.2]];
const GUARD_O = [[4.0, -4.4], [13.2, -4.6], [13.2, -8.4], [4.0, -8.4]], GUARD_I = [[4.8, -4.6], [12.4, -4.7], [12.4, -7.55], [4.8, -7.55]];
const HOUSING = [[3.0, -3.0], [13.2, -3.0], [13.2, -4.65], [3.4, -4.45]];
const TRIG = [[7.4, -4.4], [8.5, -4.4], [8.4, -5.4], [8.1, -6.7], [7.5, -6.95], [7.7, -5.6]];

function bodySide(V) {
	const right = V.kind === 'right';
	let s = '';
	// trigger group: the step guard (behind), the lever, the riveted housing
	s += ao(evenodd(pth(V, GUARD_O, 0.3), pth(V, GUARD_I, 0.2), 'url(#s2iron)', 'mM'));
	s += inside(pth(V, R4(4.0, -8.4, 13.2, -7.55), 0.1), `<rect x="0" y="0" width="${N}" height="${N}" fill="url(#s2tread)"/>`);
	s += pl(V, [[4.3, -7.6], [12.9, -7.6]], '#8a8d92', 1, 'opacity=".5"');
	s += bR(V, 4.4, -8.0, 0.22) + bR(V, 12.8, -8.0, 0.22) + bR(V, 12.8, -5.0, 0.2);
	s += ao(shp(pth(V, TRIG, 0.15), 'url(#steelPlate)', 'mS')) + pl(V, [[7.7, -4.6], [8.0, -6.6]], '#ffffff', 1, 'opacity=".35"');
	const hd = pth(V, HOUSING, 0.2);
	s += shp(hd, 'url(#oxblood)', 'pM') + rowR(V, 4.0, 12.4, -3.85, 2.8, 0.2);
	s += bR(V, 7.95, -3.5, 0.28) + ao(circ(V, 10.6, -3.75, 0.3, 'url(#rSteel)', 'mS')) + circ(V, 10.6, -3.75, 0.12, '#2a2b2e');
	// the oxblood receiver
	const core = pth(V, CORE, 0.25);
	s += shp(core, 'url(#oxblood)', 'pL');
	s += inside(core, `<rect x="0" y="${r2(V.Y(2.2))}" width="${N}" height="${r2(5.2 * V.s)}" fill="url(#form)"/>`);
	// the iron skirt along the waist with the rarity accent in it
	s += ao(RR(V, 5.8, -3.05, 14.0, -2.2, 0.08, 'url(#s2iron)', 'mS'));
	s += L.accentPale(V, [[6.3, -2.62], [13.5, -2.62]], 0.13);
	// cream stencil line along the waist's top
	s += RR(V, 6.1, 1.55, 13.7, 1.8, 0.03, CREAM, null, 'opacity=".9"');
	if (right) {
		// the ejection port and the steel bolt in it, a hinged hatch strap above
		s += RR(V, 6.2, -0.8, 13.8, 1.2, 0.2, '#120d0c', null);
		s += ao(RR(V, 6.6, -0.55, 12.2, 0.95, 0.35, 'url(#steelCyl)', 'mS'));
		for (let a = 7.2; a < 11.8; a += 0.55) s += pl(V, [[a, -0.45], [a, 0.85]], '#3c3f44', 1, 'opacity=".6"');
		s += ao(circ(V, 11.4, 0.2, 0.3, 'url(#rSteel)', 'mS')) + circ(V, 11.4, 0.2, 0.12, '#1a1b1d');
		s += ao(RR(V, 6.0, 1.22, 14.0, 1.5, 0.05, 'url(#s2iron)', 'mS'));
		for (const a of [6.2, 9.6, 13.0]) s += ao(RR(V, a, 1.12, a + 0.8, 1.6, 0.15, 'url(#ironCylH)', 'mS'));
	} else {
		// an oval brass builder's plate, flush on the flat left flank (ticks, no text), a star in it
		const ov = (rx, ry) => { const [x, y] = V.P(9.8, -0.4); return `M${r2(x - rx * V.s)},${r2(y)}a${r2(rx * V.s)},${r2(ry * V.s)} 0 1,0 ${r2(2 * rx * V.s)},0a${r2(rx * V.s)},${r2(ry * V.s)} 0 1,0 ${r2(-2 * rx * V.s)},0Z`; };
		s += ao(shp(ov(2.9, 1.35), 'url(#brass)', 'bS')) + shp(ov(2.55, 1.05), 'url(#oxLo)', null);
		for (let k = 0; k < 9; k++) s += pl(V, [[8.2 + k * 0.4, -0.75], [8.2 + k * 0.4, -0.75 + [0.5, 0.3, 0.6, 0.35][k % 4]]], '#e6b45c', 1.3);
		s += pl(V, [[8.0, -1.0], [11.6, -1.0]], '#e6b45c', 1.2) + pl(V, [[8.4, 0.35], [11.2, 0.35]], '#e6b45c', 1.2);
		s += shp(rounded(star(V.X(9.8), V.Y(0.05), 0.24 * V.s, 0.1 * V.s), 0), '#f7dc9c', null);
		for (const a of [7.2, 12.4]) s += bR(V, a, -0.4, 0.17);
	}
	// the cab plate over the rear: proud iron, rivet border, ribs, cream end stripes on its skirt
	const cab = pth(V, CAB, 0.25);
	s += ao(shp(cab, 'url(#s2iron)', 'mM'));
	s += stripes(V, cab, -0.3, 3.2, -4.0, -2.2, 0.5, 0.6, 0.6);
	s += inside(cab, `<rect x="0" y="${r2(V.Y(2.2))}" width="${N}" height="${r2(6.2 * V.s)}" fill="url(#form)" opacity=".6"/>`);
	for (const v of [-1.2, 0.1, 1.2]) s += RR(V, 0.6, v - 0.12, 5.0, v + 0.12, 0.1, 'url(#s2ironHi)', 'mS') + pl(V, [[0.7, v - 0.14], [4.9, v - 0.14]], '#0c0c0d', 1, 'opacity=".6"');
	s += colR(V, 0.15, -1.9, 1.7, 1.2, 0.24) + colR(V, 5.4, -2.6, 1.7, 1.08, 0.24) + rowR(V, 1.3, 4.3, 1.85, 1.5, 0.22);
	s += wear(cab, '#b8bec6', 1.2, 0.6);
	// the hood: iron, four louvres glowing ember behind their blades
	const hood = pth(V, HOOD, 0.25);
	s += ao(shp(hood, 'url(#s2iron)', 'mM'));
	s += inside(hood, `<rect x="0" y="${r2(V.Y(2.2))}" width="${N}" height="${r2(5.2 * V.s)}" fill="url(#form)" opacity=".6"/>`);
	for (const v of [-2.15, -1.05, 0.05]) {
		s += RR(V, 15.0, v, 19.3, v + 0.62, 0.12, '#120806', null);
		s += RR(V, 15.15, v + 0.08, 19.15, v + 0.4, 0.1, 'url(#emberLin)', null);
		s += ao(shp(pth(V, [[14.95, v + 0.62], [19.35, v + 0.62], [19.25, v + 0.38], [15.05, v + 0.38]], 0.05), 'url(#s2ironHi)', 'mS'));
	}
	s += colR(V, 14.45, -2.5, 1.6, 1.36, 0.22) + RR(V, 14.9, 1.15, 19.3, 1.4, 0.03, CREAM, null, 'opacity=".9"');
	s += wear(hood, '#b8bec6', 1.2, 0.6);
	// the rail on its riveted gussets
	s += ao(RR(V, 0.6, 2.2, 19.6, 2.65, 0.08, 'url(#black)', 'mS') + railTeeth(V, 0.6, 19.6, 2.2, 'url(#black)'));
	s += ao(shp(pth(V, [[0.6, 2.2], [0.6, 2.65], [-0.2, 2.2]], 0.02), 'url(#black)', 'mS') + shp(pth(V, [[19.6, 2.2], [19.6, 2.65], [20.0, 2.2]], 0.02), 'url(#black)', 'mS'));
	for (let a = 1.8; a < 19; a += 2.6) s += bR(V, a, 2.42, 0.13);
	return s + wear(core, '#c9bdb4', 1.2, 0.6);
}
function bodyEnd(V, back) {
	let s = '';
	// trigger group under it: the guard's leg and tread, the housing
	s += ao(RR(V, -0.85, -8.4, 0.85, -4.4, 0.15, 'url(#s2iron)', 'mM'));
	s += ao(RR(V, -0.85, -8.4, 0.85, -7.55, 0.1, 'url(#s2ironLo)', 'mS'));
	s += ao(RR(V, -1.2, -4.65, 1.2, -2.9, 0.15, back ? 'url(#oxMid)' : 'url(#oxLo)', 'pM'));
	// the core and the side plates (cab wider at the back, hood at the front)
	s += ao(RR(V, -1.5, -3.0, 1.5, 2.2, 0.2, back ? 'url(#oxMid)' : 'url(#oxLo)', 'pL'));
	for (const sd of [-1, 1]) {
		s += ao(RR(V, sd > 0 ? 1.5 : -2.2, -4.0, sd > 0 ? 2.2 : -1.5, 2.2, 0.15, back ? 'url(#s2iron)' : 'url(#s2ironLo)', 'mM'));
		if (back) s += stripes(V, pth(V, R4(sd > 0 ? 1.5 : -2.2, -4.0, sd > 0 ? 2.2 : -1.5, -2.6), 0.1), -2.3, 2.3, -4.0, -2.6, 0.3, 0.4, 0.4);
		s += ao(RR(V, sd > 0 ? 1.5 : -2.0, -3.0, sd > 0 ? 2.0 : -1.5, 2.2, 0.12, back ? 'url(#s2ironLo)' : 'url(#s2iron)', 'mM'));
		for (const v of [-2.0, -0.6, 0.8]) s += bR(V, sd * 2.2, v, 0.16);
	}
	s += ao(shp(pth(V, RAILX, 0.03), 'url(#black)', 'mS'));
	if (back) {
		// the face the stock seats on: an iron seat plate, two bolts, a locating boss on the bore line
		const sp = pth(V, R4(-1.4, -2.9, 1.4, 1.7), 0.3);
		s += ao(shp(sp, 'url(#s2iron)', 'mM'));
		s += screw(V, -0.85, -2.3, 0.28) + screw(V, 0.85, 1.1, 0.28) + screw(V, 0.85, -2.3, 0.28) + screw(V, -0.85, 1.1, 0.28);
		s += ao(circ(V, 0, 0, 0.6, 'url(#rIron)', 'mS')) + circ(V, 0, 0, 0.26, '#0e0f10') + ring(V, 0, 0, 0.72, '#d9a441', 1.4);
	} else {
		// the hood's front: the barrel nut (brass band on an iron collar), the magazine seat, the pump bars' slots
		s += ao(RR(V, -1.5, -3.0, 1.5, 1.6, 0.2, 'url(#oxMid)', 'pM'));
		s += ao(circ(V, 0, 0, 1.6, 'url(#rBrass)', 'bS')) + ao(circ(V, 0, 0, 1.38, 'url(#rIron)', 'mM'));
		s += knurlRing(V, 0, 0, 1.38, 40, '#121314') + circ(V, 0, 0, 1.15, '#0b0b0c') + circ(V, 0, 0, 0.75, 'url(#bore)', null);
		s += ao(circ(V, 0, -2.5, 1.18, 'url(#rIron)', 'mS')) + circ(V, 0, -2.5, 1.0, '#0d0d0e') + ring(V, 0, -2.5, 0.85, '#3a3b3e', 1);
		for (const sd of [-1, 1]) s += RR(V, sd * 1.5 - 0.17, -2.3, sd * 1.5 + 0.17, -1.8, 0.05, '#050505', null);
		for (const [a, v] of [[-1.15, 1.25], [1.15, 1.25]]) s += bR(V, a, v, 0.17);
	}
	return s;
}

// ==== Barrel_Shielded: a riveted boiler-plate roof over an ember-hot barrel (local u 0..46) ============================
// A long iron roof with a flat top and slanted sides over the top half, two rows of round boiler ports with the hot
// barrel glowing in them, three oxblood straps with brass bolts and a cream band, a riveted blade front sight with a
// cyan bead; a brass breech collar; the underside plain round (the pump and the tube clamp ride there).
function brlSide(V) {
	let s = '';
	s += ao(RR(V, 0, -1.15, 46, 1.15, 0.05, 'url(#blackCyl)', 'mS'));
	for (let a = 44.9; a < 46; a += 0.14) s += pl(V, [[a, -1.1], [a, 1.1]], '#000', 0.8, 'opacity=".25"');
	s += ao(RR(V, 0, -1.42, 1.6, 1.42, 0.08, 'url(#brassCyl)', 'bS'));
	for (const a of [0.35, 1.25]) s += pl(V, [[a, -1.35], [a, 1.35]], '#6a4813', 1.2, 'opacity=".7"');
	// the roof: v 0.2 (its skirt) to 2.0 (the flat top), slanted sides seen as a lower band
	const roof = pth(V, [[2.2, 0.25], [39.6, 0.25], [40.4, 0.9], [40.4, 2.0], [39.8, 2.32], [2.6, 2.32], [2.2, 2.0]], 0.12);
	s += ao(shp(roof, 'url(#s2iron)', 'mM'));
	s += inside(roof, RR(V, 0, 0.25, 50, 0.95, 0, 'url(#s2ironLo)', null, 'opacity=".75"') + pl(V, [[2.4, 0.95], [40, 0.95]], '#6d7075', 1, 'opacity=".5"') +
		pl(V, [[2.4, 2.22], [39.8, 2.22]], '#ffffff', 1.2, 'opacity=".35"'));
	// boiler ports: two staggered rows, ember barrel behind
	for (let a = 4.6, k = 0; a < 38; a += 1.55, k++) {
		const v = 1.6, aa = a;
		if (a > 13.4 && a < 15.4) continue;
		if (a > 25.4 && a < 27.4) continue;
		s += circ(V, aa, v, 0.36, '#150806') + circ(V, aa, v - 0.04, 0.26, 'url(#ember)', null) + ring(V, aa, v, 0.38, '#77797e', 0.9, 'opacity=".6"');
		if (k % 2 === 0) s += circ(V, aa + 0.78, 0.6, 0.22, '#150806') + circ(V, aa + 0.78, 0.58, 0.15, 'url(#ember)', null);
	}
	// three oxblood straps with brass bolts, the middle one with a cream band
	for (const a of [2.6, 14.0, 26.0, 38.3]) {
		const w = a === 2.6 || a === 38.3 ? 1.3 : 1.6;
		s += ao(RR(V, a, 0.15, a + w, 2.45, 0.1, 'url(#oxblood)', 'pS'));
		s += bR(V, a + w / 2, 1.95, 0.2) + bR(V, a + w / 2, 0.58, 0.2);
		if (a === 14.0 || a === 26.0) s += RR(V, a, 1.12, a + w, 1.32, 0, CREAM, null, 'opacity=".9"');
	}
	// the front sight: a riveted blade on a block, cyan bead on top
	s += ao(shp(pth(V, [[43.3, 1.0], [45.6, 1.0], [45.6, 1.5], [45.0, 2.5], [44.2, 2.5]], 0.08), 'url(#s2iron)', 'mS'));
	s += bR(V, 44.0, 1.35, 0.14) + bR(V, 45.1, 1.35, 0.14);
	s += ao(RR(V, 44.45, 2.4, 44.85, 2.85, 0.15, 'url(#rBrass)', 'bS')) + circ(V, 44.65, 2.9, 0.2, 'url(#cyan)', null);
	return s + wear(roof, '#b8bec6', 1.1, 0.55);
}
function brlEnd(V, back) {
	let s = '';
	const roofX = [[-1.75, 0.25], [1.75, 0.25], [1.75, 1.0], [1.15, 2.32], [-1.15, 2.32], [-1.75, 1.0]];
	s += ao(shp(pth(V, roofX, 0.15), back ? 'url(#s2iron)' : 'url(#s2ironLo)', 'mM'));
	s += ao(shp(pth(V, roofX.map(([w, v]) => [w * 1.06, v === 0.25 ? 0.15 : v + 0.13]), 0.15), 'url(#oxblood)', 'pS'));
	s += ao(shp(pth(V, roofX, 0.15), back ? 'url(#s2ironHi)' : 'url(#s2iron)', 'mM'));
	for (const sd of [-1, 1]) s += bR(V, sd * 1.82, 0.58, 0.18);
	if (!back) s += ao(shp(pth(V, [[-0.42, 1.0], [0.42, 1.0], [0.42, 1.5], [0.22, 2.5], [-0.22, 2.5], [-0.42, 1.5]], 0.06), 'url(#s2ironHi)', 'mS')) + ao(RR(V, -0.2, 2.4, 0.2, 2.85, 0.1, 'url(#brassCylH)', 'bS')) + circ(V, 0, 2.9, 0.2, 'url(#cyan)', null);
	if (back) {
		s += ao(circ(V, 0, 0, 1.42, 'url(#rBrass)', 'bS')) + ring(V, 0, 0, 1.25, '#6a4813', 1.2);
		s += circ(V, 0, 0, 1.15, 'url(#rIron)', 'mS') + circ(V, 0, 0, 0.82, 'url(#bore)', null);
		s += RR(V, -0.12, 1.15, 0.12, 1.4, 0.03, '#2a1c06', null);
	} else {
		s += ao(circ(V, 0, 0, 1.15, 'url(#rIron)', 'mM')) + ring(V, 0, 0, 0.98, '#8a8d92', 1.2, 'opacity=".7"');
		s += circ(V, 0, 0, 0.88, 'url(#bore)', null);
		for (const r of [0.8, 0.7]) s += ring(V, 0, 0, r, '#3a3c40', 0.9, 'opacity=".7"');
	}
	return s;
}

// ==== Muzzle_Brake: an octagonal iron brake (local u 0..5.6) ============================================================
// Eight flats, two big square side ports through it (the blast leaves sideways, ember-lit), an oxblood baffle band
// between them with brass rivets, a knurled brass collar at the back, a bolted iron face plate.
function brkSide(V) {
	let s = ao(RR(V, 0, -1.5, 0.8, 1.5, 0.08, 'url(#brassCyl)', 'bS'));
	for (let a = 0.08; a < 0.75; a += 0.11) s += pl(V, [[a, -1.42], [a + 0.05, 1.42]], '#6a4813', 1, 'opacity=".6"');
	const body = pth(V, [[0.8, -1.62], [5.0, -1.62], [5.6, -1.25], [5.6, 1.25], [5.0, 1.62], [0.8, 1.62]], 0.08);
	s += ao(shp(body, 'url(#s2iron)', 'mM'));
	s += inside(body, RR(V, 0, 0.68, 6, 1.62, 0, 'url(#s2ironHi)', null, 'opacity=".9"') + RR(V, 0, -1.62, 6, -0.68, 0, 'url(#s2ironLo)', null, 'opacity=".9"') +
		pl(V, [[0.8, 0.68], [5.6, 0.68]], '#0c0c0d', 1.2, 'opacity=".6"') + pl(V, [[0.8, -0.68], [5.6, -0.68]], '#77797e', 1, 'opacity=".5"'));
	for (const a of [1.1, 3.35]) {
		s += RR(V, a, -0.72, a + 1.15, 0.72, 0.1, '#100605', null);
		s += RR(V, a + 0.12, -0.5, a + 1.03, 0.5, 0.08, 'url(#emberLin)', null, 'opacity=".85"');
		s += pl(V, [[a + 0.05, -0.72], [a + 1.1, -0.72]], '#9a9da2', 1, 'opacity=".6"');
	}
	s += ao(RR(V, 2.4, -1.7, 3.2, 1.7, 0.08, 'url(#s2oxCyl)', 'pS')) + bR(V, 2.8, 1.12, 0.17) + bR(V, 2.8, -1.12, 0.17);
	s += RR(V, 2.4, -0.12, 3.2, 0.12, 0, CREAM, null, 'opacity=".85"');
	s += ao(RR(V, 4.95, -1.28, 5.6, 1.28, 0.1, 'url(#steelCyl)', 'mS'));
	return s + wear(body, '#b8bec6', 1.2, 0.6);
}
const OCT = r => Array.from({ length: 8 }, (_, i) => [r * Math.cos((i + 0.5) * Math.PI / 4), r * Math.sin((i + 0.5) * Math.PI / 4)]);
function brkEnd(V, back) {
	let s = ao(shp(pth(V, OCT(1.75), 0.08), 'url(#s2oxCyl)', 'pS'));
	s += ao(shp(pth(V, OCT(1.66), 0.08), back ? 'url(#s2ironLo)' : 'url(#s2iron)', 'mM'));
	if (back) {
		s += ao(circ(V, 0, 0, 1.5, 'url(#rBrass)', 'bS')) + knurlRing(V, 0, 0, 1.5, 36, '#7a5414');
		s += circ(V, 0, 0, 1.2, '#0c0c0d');
		for (const r of [1.12, 1.0, 0.88]) s += ring(V, 0, 0, r, '#3c3d40', 1, 'opacity=".8"');
		s += circ(V, 0, 0, 0.7, '#050505');
	} else {
		s += ao(circ(V, 0, 0, 1.28, 'url(#rSteel)', 'mS'));
		for (let k = 0; k < 4; k++) { const t = Math.PI / 4 + k * Math.PI / 2; s += hexHead(V, 1.0 * Math.cos(t), 1.0 * Math.sin(t), 0.17); }
		s += circ(V, 0, 0, 0.72, '#0b0b0c') + circ(V, 0, 0, 0.58, 'url(#bore)', null) + ring(V, 0, 0, 0.74, '#d9dde1', 1.2, 'opacity=".6"');
	}
	return s;
}

// ==== Magazine_Drum20: a driving-wheel drum (gun space) ================================================================
// The drum's face is a locomotive driving wheel: an iron tyre, eight spokes from a brass hub with the ember core, a
// cream-edged oxblood counterweight, brass shell heads showing between the spokes; a riveted iron band round its rim; a
// riveted well block under the receiver; a brass winding key on the right; the pump's guide tube forward.
const DC = [21.2, -13.6];
function drumSide(V) {
	const right = V.kind === 'right';
	let s = '';
	// the pump's guide tube and its brass end cap
	s += ao(RR(V, 19.6, -3.3, 47.0, -1.7, 0.05, 'url(#blackCyl)', 'mS'));
	s += ao(RR(V, 47.0, -3.45, 48.0, -1.55, 0.12, 'url(#brassCyl)', 'bS'));
	for (const a of [24.0, 44.0]) s += ao(RR(V, a, -3.36, a + 0.5, -1.64, 0.05, 'url(#s2oxCyl)', 'pS'));
	// the feed neck and the well block
	s += ao(shp(pth(V, [[15.2, -5.9], [19.4, -5.9], [20.4, -7.6], [14.6, -7.6]], 0.15), 'url(#s2iron)', 'mM'));
	const well = pth(V, R4(13.4, -6.0, 19.6, -3.0), 0.25);
	s += ao(shp(well, 'url(#s2iron)', 'mM')) + RR(V, 13.6, -3.4, 19.4, -3.2, 0.03, CREAM, null, 'opacity=".85"');
	s += bR(V, 14.1, -4.6, 0.22) + bR(V, 18.9, -4.6, 0.22) + bR(V, 14.1, -5.5, 0.18) + bR(V, 18.9, -5.5, 0.18);
	s += ao(RR(V, 15.6, -5.0, 17.4, -4.1, 0.2, 'url(#steelPlate)', 'mS')) + pl(V, [[15.8, -4.55], [17.2, -4.55]], '#2a2b2e', 1.2);
	// the wheel
	const [cu, cv] = DC;
	s += ao(circ(V, cu, cv, 7.6, 'url(#rIron)', 'mM'));
	s += ring(V, cu, cv, 7.25, '#8a8d92', 1.4, 'opacity=".5"');
	s += circ(V, cu, cv, 6.75, '#1a1112');
	for (let k = 0; k < 16; k++) { const t = k * Math.PI / 8; s += bR(V, cu + 7.18 * Math.cos(t), cv + 7.18 * Math.sin(t), 0.2); }
	// shell heads between the spokes (behind them)
	for (let k = 0; k < 8; k++) {
		const t = (k + 0.5) * Math.PI / 4, a = cu + 4.6 * Math.cos(t), v = cv + 4.6 * Math.sin(t);
		s += circ(V, a, v, 1.1, 'url(#rBrass)', 'bS') + ring(V, a, v, 0.85, '#7a5414', 1) + circ(V, a, v, 0.32, '#a3762b');
	}
	// the counterweight: an oxblood crescent over the lower spokes, cream-edged
	const cw = (() => {
		const pts = [];
		for (let i = 0; i <= 20; i++) { const t = Math.PI * (1.18 + 0.64 * i / 20); pts.push([cu + 6.7 * Math.cos(t), cv + 6.7 * Math.sin(t)]); }
		for (let i = 20; i >= 0; i--) { const t = Math.PI * (1.18 + 0.64 * i / 20); pts.push([cu + 3.4 * Math.cos(t) , cv + 2.2 * Math.sin(t) - 1.6]); }
		return pts;
	})();
	// spokes: tapered iron bars hub to tyre
	for (let k = 0; k < 8; k++) {
		const t = k * Math.PI / 4, c = Math.cos(t), sn = Math.sin(t), px = -sn, py = c;
		const P = (r, w) => [cu + r * c + px * w, cv + r * sn + py * w];
		s += ao(shp(pth(V, [P(2.4, 0.55), P(6.8, 0.38), P(6.8, -0.38), P(2.4, -0.55)], 0.1), 'url(#s2iron)', 'mS'));
	}
	s += ao(shp(pth(V, cw, 0.2), 'url(#oxblood)', 'pM')) + inside(pth(V, cw, 0.2), sk(V, cw.slice(0, 21), CREAM, 0.22));
	for (const t of [1.32, 1.5, 1.68]) s += bR(V, cu + 5.6 * Math.cos(Math.PI * t), cv + 5.6 * Math.sin(Math.PI * t), 0.22);
	// the hub: brass boss, ember core, crank pin
	s += ao(circ(V, cu, cv, 2.6, 'url(#s2hub)', 'bM')) + ring(V, cu, cv, 2.1, '#7a5414', 1.3);
	for (let k = 0; k < 6; k++) { const t = k * Math.PI / 3 + 0.5; s += hexHead(V, cu + 1.85 * Math.cos(t), cv + 1.85 * Math.sin(t), 0.2, 'url(#rIron)'); }
	s += circ(V, cu, cv, 1.5, '#140806') + circ(V, cu, cv, 1.25, 'url(#ember)', null) + ring(V, cu, cv, 1.5, '#2a1c06', 1.6);
	if (right) {   // the winding key on the right
		s += ao(RR(V, cu - 0.35, cv - 0.35, cu + 0.35, cv + 0.35, 0.1, 'url(#rBrass)', 'bS'));
		s += ao(shp(pth(V, [[cu - 2.4, cv + 0.4], [cu + 2.4, cv + 0.4], [cu + 2.6, cv - 0.1], [cu + 2.4, cv - 0.6], [cu - 2.4, cv - 0.6], [cu - 2.6, cv - 0.1]], 0.3), 'url(#brass)', 'bS'));
		s += circ(V, cu - 1.9, cv - 0.1, 0.25, '#6a4813') + circ(V, cu + 1.9, cv - 0.1, 0.25, '#6a4813');
	}
	return s + wear(L.discD(V, cu, cv, 7.55), '#b8bec6', 1.3, 0.6);
}
function drumEnd(V, back) {
	let s = '';
	const [, cv] = DC;
	if (back) s += ao(circ(V, 0, -2.5, 0.8, 'url(#rIron)', 'mS')) + circ(V, 0, -2.5, 0.3, '#0c0c0d');
	s += ao(RR(V, -1.9, -6.0, 1.9, -3.0, 0.25, back ? 'url(#s2ironHi)' : 'url(#s2iron)', 'mM'));
	s += ao(RR(V, -1.5, -7.6, 1.5, -5.9, 0.15, 'url(#s2ironLo)', 'mS'));
	// the drum's rim seen edge on: a band, shaded round, riveted at its edges
	const rim = pth(V, R4(-2.7, cv - 7.6, 2.7, cv + 7.6), 1.4);
	s += ao(shp(rim, 'url(#blackCyl)', 'mM'));
	s += inside(rim, `<rect x="0" y="${r2(V.Y(cv + 7.6))}" width="${N}" height="${r2(15.2 * V.s)}" fill="url(#form)" opacity=".7"/>`);
	for (const sd of [-1, 1]) for (let v = cv - 6.6; v <= cv + 6.6; v += 1.65) s += bR(V, sd * 2.25, v, 0.17);
	// hubs out to either side; the key on the right (-w)
	for (const sd of [-1, 1]) s += ao(RR(V, sd > 0 ? 2.7 : -3.3, cv - 2.6, sd > 0 ? 3.3 : -2.7, cv + 2.6, 0.2, 'url(#brassCylH)', 'bS'));
	s += ao(RR(V, -3.9, cv - 2.6, -3.3, cv + 2.6, 0.2, 'url(#brass)', 'bS'));
	if (!back) s += ao(circ(V, 0, -2.5, 0.95, 'url(#rBrass)', 'bS')) + circ(V, 0, -2.5, 0.35, '#6a4813');
	return s;
}

// ==== Sight_Prism: a railway signal lamp (gun space; SOCKET_Aim (5.15, 5.75)) =========================================
// A riveted iron lamp box with oxblood side doors, a hooded front lens in a brass bezel (cyan objective ring), a brass
// eyepiece bezel at the back, a bail handle on top like a lantern's, a cream band, a heavy clamp with two hex bolts.
const SV0 = 3.15, AV = 5.75;
function prismSide(V) {
	const right = V.kind === 'right';
	let s = '';
	// the clamp on the rail
	s += ao(RR(V, 6.2, SV0, 14.4, SV0 + 0.85, 0.15, 'url(#s2iron)', 'mM'));
	for (const a of [7.4, 13.2]) s += right ? hexHead(V, a, SV0 + 0.42, 0.32, 'url(#rSteel)') : bR(V, a, SV0 + 0.42, 0.22);
	// the bail handle (behind the box's top)
	s += L.rod(V, [[6.4, 7.6], [7.0, 8.75], [13.5, 8.75], [14.1, 7.6]], 0.2, '#2e2f33', '#7d8086');
	for (const a of [6.4, 14.1]) s += bR(V, a, 7.55, 0.22);
	// the lamp box
	const box = pth(V, [[5.25, SV0 + 0.8], [15.25, SV0 + 0.8], [15.25, 7.3], [14.8, 7.75], [5.7, 7.75], [5.25, 7.3]], 0.2);
	s += ao(shp(box, 'url(#s2iron)', 'mM'));
	const door = pth(V, R4(6.6, 4.35, 13.9, 7.15), 0.25);
	s += ao(shp(door, 'url(#oxblood)', 'pM')) + inside(door, `<rect x="0" y="${r2(V.Y(7.15))}" width="${N}" height="${r2(2.8 * V.s)}" fill="url(#form)"/>`);
	s += RR(V, 6.6, 5.55, 13.9, 5.8, 0, CREAM, null, 'opacity=".9"');
	for (const v of [4.75, 5.05]) s += pl(V, [[8.0, v], [13.3, v]], '#3a120d', 1.6, 'opacity=".8"') + pl(V, [[8.0, v - 0.05], [13.3, v - 0.05]], '#c26a5a', 0.8, 'opacity=".6"');
	for (let a = 8.2; a < 13.3; a += 0.9) s += RR(V, a, 6.25, a + 0.5, 6.9, 0.1, '#1a0806', null) + pl(V, [[a + 0.05, 6.88], [a + 0.45, 6.88]], '#c26a5a', 0.9, 'opacity=".7"');
	if (right) { for (const v of [4.85, 6.65]) s += ao(RR(V, 6.3, v - 0.22, 7.2, v + 0.22, 0.1, 'url(#ironCylH)', 'mS')); }
	else { s += ao(RR(V, 6.85, 4.6, 7.7, 5.15, 0.1, 'url(#brass)', 'bS')); }
	for (const [a, v] of [[5.75, 4.4], [5.75, 7.25], [14.75, 4.4], [14.75, 7.25], [10.25, 7.45]]) s += bR(V, a, v, 0.19);
	// the hood over the objective, its brass bezel and cyan ring; the brass eyepiece bezel
	s += ao(shp(pth(V, [[15.25, 4.2], [16.3, 4.4], [16.3, 7.6], [15.25, 7.75]], 0.08), 'url(#s2iron)', 'mS'));
	s += ao(RR(V, 15.25, AV - 1.3, 15.6, AV + 1.3, 0.06, 'url(#brassCylH)', 'bS'));
	s += RR(V, 15.6, AV - 1.25, 15.75, AV + 1.25, 0.03, 'url(#cyanLin)', null);
	s += ao(RR(V, 4.6, AV - 1.3, 5.25, AV + 1.3, 0.1, 'url(#brassCylH)', 'bS'));
	for (let v = AV - 1.1; v < AV + 1.2; v += 0.3) s += pl(V, [[4.65, v], [5.2, v]], '#6a4813', 0.9, 'opacity=".6"');
	return s + wear(box, '#b8bec6', 1.1, 0.6);
}
function prismEnd(V, back) {
	let s = '';
	s += ao(RR(V, -1.3, SV0, 1.3, SV0 + 0.85, 0.15, 'url(#s2iron)', 'mM'));
	s += ao(RR(V, -1.75, SV0 + 0.1, -1.3, SV0 + 0.75, 0.1, 'url(#rSteel)', 'mS'));   // bolt heads (gun's right)
	s += ao(RR(V, -0.2, 7.5, 0.2, 8.95, 0.12, 'url(#blackCylH)', 'mS'));
	s += ao(RR(V, -1.5, SV0 + 0.8, 1.5, 7.75, 0.3, back ? 'url(#s2iron)' : 'url(#s2ironLo)', 'mM'));
	for (const sd of [-1, 1]) s += ao(RR(V, sd > 0 ? 1.5 : -1.62, 4.35, sd > 0 ? 1.62 : -1.5, 7.15, 0.04, 'url(#oxblood)', 'pS'));
	for (const [a, v] of [[-1.1, 4.4], [1.1, 4.4], [-1.1, 7.35], [1.1, 7.35]]) s += bR(V, a, v, 0.17);
	if (!back) s += ao(shp(pth(V, [[-1.55, 7.6], [1.55, 7.6], [1.55, 4.2], [1.4, 4.2], [1.4, 7.4], [-1.4, 7.4], [-1.4, 4.2], [-1.55, 4.2]], 0.05), 'url(#s2ironHi)', 'mS'));
	s += ao(circ(V, 0, AV, 1.3, 'url(#rBrass)', 'bS'));
	if (back) {
		s += circ(V, 0, AV, 1.05, 'url(#rLens)', null) + ring(V, 0, AV, 1.05, '#2a1c06', 1.6);
		s += pl(V, [[-0.05, AV], [0.05, AV]], '#ff4a3a', 0.12 * V.s) + RR(V, -0.01, AV - 0.35, 0.01, AV - 0.12, 0, '#ff4a3a', null) + RR(V, -0.01, AV + 0.12, 0.01, AV + 0.35, 0, '#ff4a3a', null) +
			RR(V, -0.35, AV - 0.01, -0.12, AV + 0.01, 0, '#ff4a3a', null) + RR(V, 0.12, AV - 0.01, 0.35, AV + 0.01, 0, '#ff4a3a', null);
		s += clip(L.discD(V, 0, AV, 1.05), pl(V, [[-0.9, AV + 0.3], [-0.1, AV + 1.0]], '#ffffff', 0.12 * V.s, 'opacity=".25"'));
	} else {
		s += ring(V, 0, AV, 1.2, '#5fd6f5', 0.12 * V.s) + circ(V, 0, AV, 1.05, 'url(#rLens)', null) + circ(V, 0, AV, 1.05, 'url(#rCoat)', null);
		s += clip(L.discD(V, 0, AV, 1.05), pl(V, [[-0.9, AV + 0.2], [0.0, AV + 1.0]], '#ffffff', 0.14 * V.s, 'opacity=".3"'));
	}
	return s;
}

// ==== Stock_Mule: a buffer-beam stock (gun space, u <= 0) ==============================================================
// A heavy straight-lined oxblood slab with an iron top beam (the cheek riser, riveted) and an iron keel channel, a cream
// hazard block, a sling bar; the butt is a railway buffer: an iron headstock, two brass plunger housings with springs
// showing, and a thick ribbed rubber buffer pad; an iron seat plate at the front.
const MULE = [[0, 1.8], [-3.6, 1.8], [-14.6, 1.0], [-36.8, 0.6], [-38.4, 0.3], [-38.4, -12.5], [-36.8, -12.6], [-12.4, -7.6],
	[-9.6, -8.2], [-6.0, -7.6], [-3.0, -4.8], [-0.6, -3.4], [0, -3.0]];
function muleSide(V) {
	const right = V.kind === 'right';
	let s = '';
	// the sling bar under the belly
	s += L.rod(V, [[-24.0, -9.9], [-24.6, -11.0], [-28.4, -11.8], [-29.2, -11.0]], 0.22, '#2e2f33', '#7d8086');
	const body = pth(V, MULE, 0.45);
	s += shp(body, 'url(#oxblood)', 'pL');
	s += inside(body, `<rect x="0" y="${r2(V.Y(2.0))}" width="${N}" height="${r2(15 * V.s)}" fill="url(#form)" opacity=".8"/>`);
	// a stepped panel line: the slab's thick rear half and the wrist
	s += seam(V, [[-14.6, 0.85], [-12.6, -7.4]]);
	// the keel channel along the belly
	s += ao(shp(pth(V, [[-12.0, -7.6], [-36.9, -12.65], [-37.0, -11.5], [-12.6, -6.6]], 0.08), 'url(#s2iron)', 'mS'));
	for (let k = 0; k < 6; k++) { const a = -14.5 - k * 4.2; s += bR(V, a, -7.6 + (a + 12) * 0.2028 + 0.5, 0.18); }
	// plating seams with rivet rows: along the slab under the comb and down the butt end
	s += seam(V, [[-15.4, -0.55], [-37.6, -0.95]]) + seam(V, [[-36.6, -1.2], [-36.6, -11.9]]);
	for (let a = -16.4; a > -36; a -= 2.4) s += bR(V, a, -0.2 - (a + 15.4) * 0.018, 0.17);
	for (let v = -2.0; v > -11.6; v -= 1.6) s += bR(V, -37.4, v, 0.17);
	// the cheek riser: an iron beam on the comb, riveted, a cream stripe
	const riser = pth(V, [[-15.0, 1.0], [-33.0, 0.62], [-33.0, 2.5], [-31.6, 2.8], [-16.2, 2.8], [-15.0, 2.2]], 0.15);
	s += ao(shp(riser, 'url(#s2iron)', 'mM')) + RR(V, -31.4, 2.05, -16.6, 2.25, 0, CREAM, null, 'opacity=".9"');
	for (let a = -31.8; a < -15.5; a += 2.6) s += bR(V, a, 1.45, 0.2);
	// the wrist: stippled oxblood grip panel where the hand holds (-4.5, -6), two screws
	const gp = pth(V, [[-2.0, -3.6], [-4.3, -3.6], [-7.6, -6.8], [-6.0, -7.4], [-3.0, -5.0]], 0.4);
	s += shp(gp, 'url(#stipDark)', null, 'opacity=".7"') + shp(gp, 'none', null, 'stroke="#2a0d09" stroke-width="1.3"');
	// side plate with the cream hazard block
	const hz = pth(V, R4(-28.0, -6.8, -19.0, -3.2), 0.3);
	s += ao(shp(hz, 'url(#s2iron)', 'mS')) + stripes(V, pth(V, R4(-27.4, -6.2, -19.6, -3.8), 0.15), -28, -19, -6.2, -3.8, 0.55, 0.7, 0.8);
	for (const [a, v] of [[-27.5, -6.3], [-19.5, -6.3], [-27.5, -3.7], [-19.5, -3.7]]) s += bR(V, a, v, 0.2);
	if (!right) s += ao(RR(V, -10.0, -1.6, -6.4, 0.4, 0.15, 'url(#brass)', 'bS')) + pl(V, [[-9.6, -0.6], [-6.8, -0.6]], '#7a5414', 1.2);
	// the seat plate at the front
	s += ao(RR(V, -0.45, -3.2, 0.3, 1.9, 0.08, 'url(#s2iron)', 'mS'));
	// the buffer beam: headstock, plungers with springs, the rubber buffer pad
	s += ao(RR(V, -39.1, -12.9, -38.2, 0.6, 0.12, 'url(#s2iron)', 'mM'));
	for (const v of [-9.6, -2.4]) {
		s += ao(RR(V, -40.2, v - 1.0, -39.1, v + 1.0, 0.1, 'url(#brassCyl)', 'bS'));
		for (let a = -40.1; a < -39.2; a += 0.22) s += pl(V, [[a, v - 0.95], [a + 0.11, v + 0.95]], '#6a4813', 1.1, 'opacity=".7"');
	}
	const pad = pth(V, [[-40.1, -13.2], [-41.3, -13.0], [-41.6, -12.4], [-41.6, 0.2], [-41.3, 0.8], [-40.1, 0.8]], 0.3);
	s += ao(shp(pad, 'url(#s2rubberH)', 'rM'));
	for (let v = -12.0; v < 0; v += 1.2) s += pl(V, [[-41.55, v], [-40.2, v]], '#0b0b0b', 1.3, 'opacity=".8"');
	for (const [a, v] of [[-38.65, -12.1], [-38.65, -0.2], [-38.65, -6.0]]) s += bR(V, a, v, 0.24);
	return s + wear(body, '#c9bdb4', 1.2, 0.6) + wear(riser, '#b8bec6', 1.1, 0.6);
}
function muleEnd(V, back) {
	let s = '';
	// bulk of the slab behind, the riser on top
	s += ao(RR(V, -2.3, -12.6, 2.3, 1.8, 0.7, back ? 'url(#oxMid)' : 'url(#oxLo)', 'pL'));
	s += ao(RR(V, -1.8, 0.8, 1.8, 2.8, 0.3, 'url(#s2iron)', 'mM')) + RR(V, -1.8, 2.05, 1.8, 2.25, 0, CREAM, null, 'opacity=".85"');
	s += ao(RR(V, -2.0, -12.65, 2.0, -11.6, 0.2, 'url(#s2ironLo)', 'mS'));
	if (back) {
		s += ao(RR(V, -2.4, -12.9, 2.4, 0.6, 0.4, 'url(#s2iron)', 'mM'));
		const pad = pth(V, R4(-2.25, -13.2, 2.25, 0.8), 0.9);
		s += ao(shp(pad, 'url(#rRubber)', 'rM')) + shp(pad, 'url(#stip4)', null, 'opacity=".35"');
		s += inside(pad, Array.from({ length: 11 }, (_, k) => { const v = -12.0 + k * 1.2; return pl(V, [[-2.3, v], [2.3, v]], '#0b0b0b', 1.6) + pl(V, [[-2.3, v - 0.1], [2.3, v - 0.1]], '#5a5a5a', 0.8, 'opacity=".5"'); }).join(''));
		s += wear(pad, '#7a7d82', 1.2, 0.5);
	} else {
		const sp = pth(V, R4(-1.6, -3.2, 1.6, 1.9), 0.3);
		s += ao(shp(sp, 'url(#s2iron)', 'mM'));
		for (const [a, v] of [[-1.0, -2.6], [1.0, -2.6], [-1.0, 1.3], [1.0, 1.3]]) s += screw(V, a, v, 0.25);
		s += ao(circ(V, 0, 0, 0.5, 'url(#rSteel)', 'mS')) + circ(V, 0, 0, 0.2, '#2a2b2e') + ring(V, 0, 0, 0.65, '#d9a441', 1.3);
		s += ao(RR(V, -0.25, -11.8, 0.25, -10.8, 0.1, 'url(#ironCylH)', 'mS'));
	}
	return s;
}

module.exports = [
	{ dir: 'Body_Armored', b: { u: [-0.3, 20.0], v: [-8.4, 3.15], w: [-2.25, 2.25] }, draw: { left: bodySide, right: bodySide, front: V => bodyEnd(V, false), back: V => bodyEnd(V, true) } },
	{ dir: 'Barrel_Shielded', b: { u: [0, 46], v: [-1.42, 3.1], w: [-1.95, 1.95] }, draw: { left: brlSide, right: brlSide, front: V => brlEnd(V, false), back: V => brlEnd(V, true) } },
	{ dir: 'Muzzle_Brake', b: { u: [0, 5.6], v: [-1.75, 1.75], w: [-1.75, 1.75] }, draw: { left: brkSide, right: brkSide, front: V => brkEnd(V, false), back: V => brkEnd(V, true) } },
	{ dir: 'Magazine_Drum20', b: { u: [13.4, 48.0], v: [-21.2, -1.55], w: [-3.9, 3.3] }, draw: { left: drumSide, right: drumSide, front: V => drumEnd(V, false), back: V => drumEnd(V, true) } },
	{ dir: 'Sight_Prism', b: { u: [4.6, 16.3], v: [3.15, 8.95], w: [-1.75, 1.75] }, draw: { left: prismSide, right: prismSide, front: V => prismEnd(V, false), back: V => prismEnd(V, true) } },
	{ dir: 'Stock_Mule', b: { u: [-41.6, 0.3], v: [-13.2, 2.8], w: [-2.55, 2.55] }, draw: { left: muleSide, right: muleSide, front: V => muleEnd(V, false), back: V => muleEnd(V, true) } },
];
