// Set 1: Saint Maud (Saint Maud's Foundry: cast iron, ivory enamel, gilt brass; plain and careful, not ornate). The
// Ranchhand's default parts (Standard body, Field barrel, Crown muzzle, Tube6 magazine, Flip sight, Skeleton stock)
// drawn on their in-game shapes (Art/Models/Weapons/Ranchhand.py) with sculpted detail: bevels, seams, screws, gilt
// hairlines, proof stamps, heat tint, edge wear. Gun space, cm (barrel and muzzle in their own u from their back face).
const L = require('./lib');
const { N, pth, R4, shp, circ, ring, pl, wear, ao, screw, cb, cd, sk, clip, flt, full, seam, knurlRing, star, rounded, r2,
	accentPale, cylSide, disc, shellUp, RAIL_X, GRADS, lin, rad } = L;

GRADS.push(
	lin('s1heatV', [[0, '#6b4424', 0], [0.5, '#6b4424', 0.35], [1, '#6b4424', 0]], false),
	lin('s1cuff', [[0, '#5d5853'], [0.5, '#45413d'], [1, '#2e2b28']]),
	rad('s1face', [[0, '#7d7770'], [0.55, '#3a3633'], [1, '#1c1a18']], 0.4, 0.35, 0.8),
);
const GILT = '#c9a24a';
const RR = (V, a0, v0, a1, v1, r, fill, filt, extra = '') => shp(pth(V, R4(a0, v0, a1, v1), r), fill, filt, extra);
// A thin gilt hairline (inlaid brass) along points.
const gline = (V, pts, cm = 0.06) => pl(V, pts, '#5a4214', Math.max(1, cm * 1.8 * V.s), 'opacity=".55"') + pl(V, pts, GILT, Math.max(0.8, cm * V.s));
// An engraved line: a dark cut with a light lip under it.
const engr = (V, pts, cm = 0.05) => pl(V, pts, '#3a332a', Math.max(0.8, cm * V.s), 'opacity=".55"') + pl(V, pts.map(([a, v]) => [a, v - cm * 0.8]), '#ffffff', Math.max(0.6, cm * 0.6 * V.s), 'opacity=".5"');
// A small proof stamp: a five-point star in a ring (a generic proof mark, not the maker's).
const proof = (V, a, v, r) => ring(V, a, v, r, '#5a4c3a', Math.max(0.8, r * 0.16 * V.s), 'opacity=".55"') +
	`<path d="${rounded(star(V.X(a), V.Y(v), r * 0.62 * V.s, r * 0.26 * V.s), 0)}" fill="#5a4c3a" opacity=".5"/>`;
const giltScrew = (V, a, v, r) => ao(circ(V, a, v, r, 'url(#rGilt)', 'bS')) + pl(V, [[a - r * 0.6, v - r * 0.2], [a + r * 0.6, v + r * 0.2]], '#5a4214', Math.max(1, r * 0.3 * V.s));
let eo = 0;
// Clip to an outline with holes (even-odd), so nothing shows inside the holes.
const clipEO = (d, inner) => { const id = 's1eo' + eo++; return `<clipPath id="${id}"><path d="${d}" clip-rule="evenodd"/></clipPath><g clip-path="url(#${id})">${inner}</g>`; };
const ember = (V, d) => shp(d, '#160a04', null) + shp(d, 'url(#emberLin)', null, 'opacity=".95"');

// ==== Body_Standard: molded ivory over a cast-iron lower, three ember vents, the bolt in a right-hand port ===========
const RCV = [[0, -3.0], [20, -3.0], [20, 1.4], [18.5, 2.2], [1.5, 2.2], [0, 1.6]];
const BEV = [[0.06, 1.6], [1.5, 2.16], [18.5, 2.16], [19.94, 1.42], [19.5, 1.28], [18.35, 1.86], [1.65, 1.86], [0.45, 1.38]];
const VENTS = [15.2, 16.6, 18.0];
const vent = u => [[u, -0.4], [u + 0.5, -0.4], [u + 1.2, 1.2], [u + 0.7, 1.2]];
// The trigger group sits at the receiver's back (moved 5 cm back on 2026-10-28 so the first-person finger reaches it):
// Ranchhand.py's trigger_group, the guard's rear meeting the pistol grip's front.
const HOUSING = [[0, -3.0], [8, -3.0], [7.4, -4.6], [-0.6, -4.4]];
const GUARD = [[-0.4, -4.4], [0.0, -6.8], [2.0, -7.6], [6.0, -7.2], [7.4, -4.6]];
const TRIG = [[2.4, -4.4], [3.4, -4.4], [3.2, -5.6], [2.8, -6.6], [2.2, -6.8], [2.5, -5.6]];

function railSide(V, u0, u1, v, fill = 'url(#iron)') {
	let s = ao(RR(V, u0, v, u1, v + 0.45, 0.08, fill, 'mS'));
	for (let u = u0 + 0.35; u + 0.55 <= u1 - 0.2; u += 1.0) s += ao(RR(V, u, v + 0.4, u + 0.53, v + 0.95, 0.07, fill, 'mS'));
	return s + pl(V, [[u0 + 0.15, v + 0.4], [u1 - 0.15, v + 0.4]], '#9a948c', 0.9, 'opacity=".4"');
}
function triggerSide(V) {
	let s = ao(shp(pth(V, HOUSING, 0.25), 'url(#iron)', 'mM'));
	s += pl(V, [[0.3, -3.3], [7.7, -3.3]], '#8a847c', 0.9, 'opacity=".4"');
	s += flt('mS', pl(V, GUARD, '#2a2725', 0.42 * V.s)) + pl(V, GUARD.map(([a, v]) => [a - 0.04, v + 0.1]), '#8c857c', Math.max(1, 0.08 * V.s), 'opacity=".55"');
	s += ao(shp(pth(V, TRIG, 0.15), 'url(#steelCyl)', 'mS')) + pl(V, [[2.55, -4.6], [2.65, -6.3]], '#ffffff', 1, 'opacity=".5"');
	s += ao(circ(V, 5.6, -3.75, 0.3, 'url(#rSteel)', 'mS')) + circ(V, 5.6, -3.75, 0.1, '#2a2725');
	s += giltScrew(V, 0.4, -3.75, 0.22);                                                       // the crossbolt safety
	return s;
}
function bodySide(V) {
	const right = V.kind === 'right', rd = pth(V, RCV, 0.3);
	let s = triggerSide(V);
	s += shp(rd, 'url(#ivory)', 'pL');
	s += clip(rd,
		shp(pth(V, BEV, 0.1), 'url(#ivoryHi)', null, 'opacity=".9"') +                         // the top bevel, catching light
		flt('soft', pl(V, [[1.7, 1.65], [18.3, 1.65]], '#ffffff', 0.25 * V.s, 'opacity=".6"')) +
		flt('soft2', pl(V, [[0.6, -1.6], [19.4, -1.6]], '#6d6250', 0.9 * V.s, 'opacity=".14"')) +
		RR(V, -0.5, -3.1, 20.5, -2.2, 0, 'url(#iron)', 'mS') +                                    // the cast-iron lower band
		pl(V, [[0.1, -2.22], [19.9, -2.22]], '#ffffff', 0.9, 'opacity=".55"'));
	for (const u of [4.6, 14.6]) s += seam(V, [[u, -2.2], [u, 1.95]], 0) + seam(V, [[u + 0.07, -2.2], [u + 0.07, 1.95]], 0);
	// the vents: slanted slits through the ivory, ember behind, a heat tint round them
	s += clip(rd, flt('soft2', RR(V, 14.6, -0.9, 19.8, 1.6, 0.6, '#6b4424', null, 'opacity=".22"')));
	for (const u of VENTS) {
		const d = pth(V, vent(u), 0.18);
		s += ember(V, d) + shp(d, 'none', null, 'stroke="#3a2a1a" stroke-width="1.4"') + pl(V, [[u + 0.06, -0.48], [u + 0.56, -0.48]], '#ffffff', 0.9, 'opacity=".6"');
	}
	if (right) {   // the ejection port, the bolt in it, its face and extractor
		const port = pth(V, R4(6, -0.8, 14, 1.2), 0.15);
		s += shp(port, '#141312', null);
		s += ao(RR(V, 6.2, -0.6, 13.8, 1.0, 0.12, 'url(#steelCyl)', 'mS'));
		s += pl(V, [[6.4, 0.2], [13.6, 0.2]], '#5b5f64', 1, 'opacity=".6"') + RR(V, 12.9, -0.45, 13.6, 0.85, 0.08, 'url(#steelCyl)', 'mS');
		for (const a of [7.4, 8.1]) s += circ(V, a, 0.6, 0.12, '#2a2c30');
		s += shp(port, 'none', null, 'stroke="#d9cfb4" stroke-width="1.4" opacity=".7" transform="translate(0,1.2)"') + shp(port, 'none', null, 'stroke="#2b2620" stroke-width="1.2"');
	} else {       // the left: a plain flank (an element plate seats here), a gilt hairline frame, a proof stamp
		s += gline(V, [[5.6, -1.85], [14.2, -1.85], [14.2, 1.4], [5.6, 1.4], [5.6, -1.85]], 0.05);
		s += proof(V, 2.6, -0.4, 0.55) + engr(V, [[1.6, -1.45], [3.6, -1.45]], 0.04);
		for (let k = 0; k < 6; k++) s += pl(V, [[1.7 + k * 0.32, -1.85], [1.7 + k * 0.32, -1.62]], '#5a4c3a', 1, 'opacity=".5"');
	}
	for (const [a, v] of [[1.0, -1.5], [1.0, 1.0], [19.3, -1.6]]) s += screw(V, a, v, 0.2);
	s += accentPale(V, [[3.0, -2.62], [18.0, -2.62]], 0.12);
	s += railSide(V, 1, 19.5, 2.2) + giltScrew(V, 1.6, 2.42, 0.15) + giltScrew(V, 18.9, 2.42, 0.15);
	return s + wear(rd, '#fffdf6', 1.2, 0.6);
}
const XS = [[-1.5, -3.05], [1.5, -3.05], [1.5, 1.4], [0.95, 2.2], [-0.95, 2.2], [-1.5, 1.4]];
const railX = V => ao(shp(pth(V, RAIL_X.map(([w, v]) => [w, v - 3.0]), 0.03), 'url(#iron)', 'mS'));
function triggerEnd(V) {
	let s = ao(shp(pth(V, [[-1.2, -3.0], [1.2, -3.0], [1.1, -4.6], [-1.1, -4.6]], 0.15), 'url(#iron)', 'mM'));
	s += ao(shp(pth(V, R4(-0.4, -7.6, 0.4, -4.4), 0.15), 'url(#blackCylH)', 'mS'));
	s += ao(shp(pth(V, R4(-0.25, -6.8, 0.25, -4.4), 0.1), 'url(#steelCylH)', 'mS'));
	s += ao(shp(pth(V, R4(-1.5, -4.0, -1.2, -3.5), 0.05), 'url(#rGilt)', 'bS'));             // the safety's ends
	return s + ao(shp(pth(V, R4(1.2, -4.0, 1.5, -3.5), 0.05), 'url(#rGilt)', 'bS'));
}
function bodyEnd(V, back) {
	let s = triggerEnd(V);
	const xd = pth(V, XS, 0.3);
	s += shp(xd, back ? 'url(#ivoryLo)' : 'url(#ivoryMid)', 'pL');
	s += clip(xd, RR(V, -2, -3.2, 2, -2.2, 0, 'url(#iron)', 'mS') + flt('soft', pl(V, [[-0.8, 2.0], [0.8, 2.0]], '#ffffff', 0.2 * V.s, 'opacity=".7"')));
	s += railX(V);
	for (const sd of [-1, 1]) s += pl(V, [[sd * 1.48, -2.75], [sd * 1.48, -2.5]], '#f4f1ea', 1.6);   // the accent's ends
	if (!back) {   // the front face: the barrel nut, the magazine seat, the pump bars' slots
		const face = pth(V, [[-1.35, -2.95], [1.35, -2.95], [1.35, 1.3], [0.85, 2.05], [-0.85, 2.05], [-1.35, 1.3]], 0.25);
		s += shp(face, 'url(#ivoryMid)', 'pM') + clip(face, RR(V, -2, -3.1, 2, -2.2, 0, 'url(#iron)', 'mS'));
		s += clip(face, ao(circ(V, 0, 0, 1.48, 'url(#rIron)', 'mM')));
		s += ring(V, 0, 0, 1.33, GILT, 0.1 * V.s) + circ(V, 0, 0, 1.15, '#0c0b0a');
		for (const r of [1.08, 0.98, 0.88]) s += ring(V, 0, 0, r, '#4a4540', 1.2, 'opacity=".8"');
		s += circ(V, 0, 0, 0.8, 'url(#bore)', null);
		for (const t of [0.6, 2.2, 3.8, 5.4]) s += ao(circ(V, 1.48 * Math.cos(t + 0.4) * 0.82, 1.48 * Math.sin(t + 0.4) * 0.82, 0.09, 'url(#rSteel)', 'mS'));
		s += clip(face, ao(circ(V, 0, -2.5, 1.05, 'url(#rIron)', 'mS')) + circ(V, 0, -2.5, 0.85, '#0e0d0c') + ring(V, 0, -2.5, 0.72, '#4a4540', 1));
		for (const sd of [-1, 1]) s += RR(V, sd * 1.2 - 0.16, -2.3, sd * 1.2 + 0.16, -1.8, 0.05, '#0c0b0a', null);
	} else {       // the back face: the stock's seat, a locating boss and four screws
		const plate = pth(V, R4(-1.32, -2.9, 1.32, 1.35), 0.3);
		s += shp(plate, 'url(#s1face)', 'mM') + pl(V, [[-1.2, 1.2], [1.2, 1.2]], '#8a847c', 1, 'opacity=".5"');
		s += ao(circ(V, 0, 0, 0.55, 'url(#rIron)', 'mS')) + circ(V, 0, 0, 0.26, '#0c0b0a') + ring(V, 0, 0, 0.66, GILT, 0.07 * V.s);
		for (const [w, v] of [[-0.85, -2.3], [0.85, -2.3], [-0.85, 0.85], [0.85, 0.85]]) s += screw(V, w, v, 0.2);
	}
	return s + wear(xd, '#fffdf6', 1.2, 0.5);
}

// ==== Barrel_Field: heat shield, ventilated rib, a gilt breech ring, a cyan bead (local u from the barrel nut) =========
const SHIELD = [[1, -0.5], [18.2, -0.5], [19, 0.4], [19, 1.2], [17.8, 1.85], [1.6, 1.85], [1, 1.3]];
const svent = u => [[u, -0.1], [u + 0.6, -0.1], [u + 1.4, 1.3], [u + 0.8, 1.3]];
function barrelSide(V) {
	let s = cylSide(V, 0, 46, 0, 1.15, 'ironCyl', 'mM', 0.03);
	s += pl(V, [[0.2, 0.62], [45.8, 0.62]], '#a29b92', 1, 'opacity=".35"');
	s += RR(V, 45.75, -1.1, 46, 1.1, 0.04, 'url(#steelCyl)', null, 'opacity=".9"');                   // the crown's bright edge
	s += cylSide(V, 0.3, 1.0, 0, 1.27, 'giltCyl', 'bS', 0.04);
	for (let k = 0; k < 4; k++) s += pl(V, [[2.0 + k * 0.28, -0.75], [2.0 + k * 0.28, -0.6]], '#8a8278', 1, 'opacity=".55"');   // proof marks
	s += proof(V, 3.5, -0.75, 0.22);
	// the heat shield: vents onto an ember-hot barrel, an ivory strip along its top, a gilt strap, two screws
	const sd = pth(V, SHIELD, 0.3);
	s += ao(shp(sd, 'url(#iron)', 'mM'));
	s += clip(sd, flt('soft', pl(V, [[1.6, 1.62], [17.8, 1.62]], '#b9b2a8', 0.16 * V.s, 'opacity=".55"')) + RR(V, 0, -0.6, 20, -0.25, 0, '#1a1816', null, 'opacity=".35"'));
	s += ao(RR(V, 1.25, 1.74, 18.3, 2.02, 0.1, 'url(#ivoryHi)', 'pS'));
	for (const u of [3.0, 4.8, 6.6]) {
		const d = pth(V, svent(u), 0.15);
		s += ember(V, d) + shp(d, 'none', null, 'stroke="#14110e" stroke-width="1.2"') + pl(V, [[u + 0.05, -0.17], [u + 0.6, -0.17]], '#a59d92', 0.9, 'opacity=".6"');
	}
	s += ao(RR(V, 10.3, -0.55, 10.8, 1.9, 0.08, 'url(#giltCylH)', 'bS'));
	for (const [a, v] of [[1.7, 0.4], [17.6, 0.4], [13.8, 0.6]]) s += screw(V, a, v, 0.17);
	s += engr(V, [[11.2, 0.8], [16.6, 0.8]], 0.04);
	// the vent rib on posts, the bead
	for (let u = 22; u <= 43; u += 3) s += ao(RR(V, u, 1.0, u + 1.0, 1.32, 0.05, 'url(#steelCyl)', 'mS'));
	s += ao(RR(V, 19, 1.28, 46, 1.72, 0.08, 'url(#iron)', 'mS')) + pl(V, [[19.2, 1.66], [45.8, 1.66]], '#a49c92', 0.9, 'opacity=".55"');
	for (let u = 19.4; u < 45.8; u += 0.5) s += pl(V, [[u, 1.66], [u, 1.72]], '#1c1a18', 0.8, 'opacity=".5"');
	s += ao(RR(V, 44.78, 1.7, 45.22, 1.86, 0.04, 'url(#brassCylH)', 'bS')) + ao(RR(V, 44.8, 1.84, 45.2, 2.06, 0.08, 'url(#cyan)', null));
	return s + wear(sd, '#c9c2b6', 1, 0.45, '6 40 3 60');
}
function barrelEnd(V, back) {
	let s = ao(circ(V, 0, 0, 1.27, 'url(#rGilt)', 'bS'));
	const sd = pth(V, [[-1.4, -0.5], [1.4, -0.5], [1.4, 1.3], [0.85, 1.85], [-0.85, 1.85], [-1.4, 1.3]], 0.3);
	s += ao(shp(sd, 'url(#rIron)', 'mM')) + RR(V, -0.6, 1.78, 0.6, 1.98, 0.06, 'url(#ivoryHi)', 'pS');
	s += clip(sd, flt('soft', pl(V, [[-0.8, 1.66], [0.8, 1.66]], '#cfc8bd', 0.14 * V.s, 'opacity=".5"')));
	for (const sdn of [-1, 1]) s += RR(V, sdn * 1.4 - 0.02, -0.1, sdn * 1.4 + 0.02, 1.3, 0, '#ff8a2e', null, 'opacity=".55"');   // ember at the vents' edges
	if (!back) {
		s += ao(RR(V, -0.35, 1.28, 0.35, 1.72, 0.06, 'url(#iron)', 'mS'));
		s += ao(circ(V, 0, 1.95, 0.22, 'url(#rBrass)', 'bS')) + circ(V, 0, 1.95, 0.16, 'url(#cyan)');
		s += disc(V, 0, 0, 1.15, 'rIron', 'mM') + ring(V, 0, 0, 1.1, '#d6dade', 1.4, 'opacity=".7"');
		s += circ(V, 0, 0, 0.95, 'url(#bore)', null) + ring(V, 0, 0, 0.88, '#3a3632', 1.1, 'opacity=".8"');
	} else {
		s += ao(circ(V, 0, 1.95, 0.22, 'url(#rBrass)', 'bS'));
		s += disc(V, 0, 0, 1.15, 'rSteel', 'mM');
		for (const r of [1.05, 0.98]) s += ring(V, 0, 0, r, '#5a5e64', 1.1, 'opacity=".8"');
		s += circ(V, 0, 0, 0.92, '#1a1918') + circ(V, 0, 0, 0.82, 'url(#bore)', null);
		s += RR(V, -0.12, 1.0, 0.12, 1.12, 0.03, GILT, null);                                    // an alignment mark
	}
	return s;
}

// ==== Muzzle_Crown: a flush choke, diamond knurl, a gilt hairline, a chamfered face (local u) ==========================
const CROWN = [[0, -1.18], [0.8, -1.18], [1.0, -1.0], [1.0, 1.0], [0.8, 1.18], [0, 1.18]];
function crownSide(V) {
	const d = pth(V, CROWN, 0.02);
	let s = ao(shp(d, 'url(#ironCyl)', 'mM'));
	s += clip(d, Array.from({ length: 13 }, (_, k) => {
		const v = -1.25 + k * 0.2;
		return pl(V, [[0.08, v], [0.48, v + 0.26]], '#0a0909', 2, 'opacity=".75"') + pl(V, [[0.08, v + 0.26], [0.48, v]], '#0a0909', 2, 'opacity=".75"') + pl(V, [[0.1, v + 0.05], [0.46, v + 0.31]], '#9a948c', 1, 'opacity=".45"');
	}).join('') + RR(V, 0.06, -1.3, 0.5, 1.3, 0, 'url(#steelCyl)', null, 'opacity=".22"'));
	s += RR(V, 0.55, -1.18, 0.65, 1.18, 0, 'url(#giltCyl)', 'bS');
	s += shp(pth(V, [[0.8, -1.18], [1.0, -1.0], [1.0, 1.0], [0.8, 1.18]], 0.02), 'url(#steelCyl)', 'mS');
	s += pl(V, [[0.02, -1.18], [0.02, 1.18]], '#0c0b0a', 1.4, 'opacity=".7"');
	return s + wear(d, '#d8dbe0', 1.2, 0.5, '4 30 2 50');
}
function crownEnd(V, back) {
	let s = disc(V, 0, 0, 1.18, 'rIron', 'mM');
	if (!back) {
		s += ao(circ(V, 0, 0, 1.0, 'url(#rSteel)', 'mS')) + ring(V, 0, 0, 1.09, '#ffffff', 1.2, 'opacity=".45"');
		s += circ(V, 0, 0, 0.93, 'url(#rIron)', 'mS') + ring(V, 0, 0, 0.76, GILT, 0.03 * V.s, 'opacity=".9"');
		for (let k = 0; k < 8; k++) { const t = k * Math.PI / 4; s += circ(V, 0.84 * Math.cos(t), 0.84 * Math.sin(t), 0.025, '#2a2622', null, 'opacity=".7"'); }
		s += circ(V, 0, 0, 0.66, '#0c0b0a');
		for (const r of [0.62, 0.56]) s += ring(V, 0, 0, r, '#46423d', 1.3, 'opacity=".8"');
		s += circ(V, 0, 0, 0.5, 'url(#bore)', null);
	} else {
		s += ring(V, 0, 0, 1.165, '#a8a29a', 0.03 * V.s) + circ(V, 0, 0, 1.15, 'url(#rIron)', null, 'transform="rotate(180 512 512)"');
		for (let r = 1.13, k = 0; r > 0.95; r -= 0.04, k++) s += ring(V, 0, 0, r, k % 2 ? '#2a2724' : '#7d776f', 0.018 * V.s, 'opacity=".9"');   // internal threads
		s += circ(V, 0, 0, 0.94, 'url(#rIron)', 'mS') + ring(V, 0, 0, 0.94, '#0c0b0a', 1.4) + ring(V, 0, 0, 0.8, '#8a847c', 1, 'opacity=".5"');
		s += circ(V, 0, 0, 0.66, '#0c0b0a') + circ(V, 0, 0, 0.56, 'url(#bore)', null);
	}
	return s;
}

// ==== Magazine_Tube6: iron tube, a gilt-ringed cap, an ivory clamp under the barrel (gun space) =========================
const MV = -2.5;
const CLAMP = [[52.5, -3.4], [54.5, -3.4], [54.5, 0.9], [52.5, 0.9]];
function tubeSide(V) {
	let s = cylSide(V, 20, 54, MV, 1.05, 'ironCyl', 'mM', 0.03);
	s += pl(V, [[20.2, MV + 0.55], [53.8, MV + 0.55]], '#a39c92', 1, 'opacity=".3"');
	for (const u of [20.6, 21.0]) s += pl(V, [[u, MV - 1.0], [u, MV + 1.0]], '#0e0d0c', 1, 'opacity=".55"');
	s += cylSide(V, 47.6, 48.0, MV, 1.09, 'giltCyl', 'bS', 0.02) + cylSide(V, 26.6, 26.9, MV, 1.08, 'giltCyl', 'bS', 0.02);
	for (let k = 0; k < 3; k++) s += pl(V, [[49 + k * 0.3, MV - 0.85], [49 + k * 0.3, MV - 0.65]], '#8a8278', 1, 'opacity=".5"');
	// the cap: knurled, a gilt ring, a turned chamfer
	const cap = pth(V, [[54, MV - 1.2], [55.6, MV - 1.2], [56, MV - 0.9], [56, MV + 0.9], [55.6, MV + 1.2], [54, MV + 1.2]], 0.03);
	s += ao(shp(cap, 'url(#ironCyl)', 'mM'));
	s += clip(cap, Array.from({ length: 13 }, (_, k) => { const v = MV - 1.3 + k * 0.2; return pl(V, [[54.6, v], [55.4, v + 0.4]], '#121110', 1.2, 'opacity=".5"') + pl(V, [[54.6, v + 0.4], [55.4, v]], '#121110', 1.2, 'opacity=".5"'); }).join(''));
	s += RR(V, 54.0, MV - 1.2, 54.35, MV + 1.2, 0.03, 'url(#giltCyl)', 'bS');
	s += shp(pth(V, [[55.6, MV - 1.2], [56, MV - 0.9], [56, MV + 0.9], [55.6, MV + 1.2]], 0.02), 'url(#steelCyl)', 'mS');
	// the clamp: ivory, a cross bolt, the barrel's seat shaded at its top
	const cd2 = pth(V, CLAMP, 0.35);
	s += shp(cd2, 'url(#ivory)', 'pL');
	s += clip(cd2, flt('soft', pl(V, [[52.7, 0.55], [54.3, 0.55]], '#6d6250', 0.25 * V.s, 'opacity=".3"')) + flt('soft', pl(V, [[52.7, -3.1], [52.7, 0.6]], '#ffffff', 0.12 * V.s, 'opacity=".6"')));
	s += gline(V, [[52.5, -1.15], [54.5, -1.15]], 0.05);
	s += screw(V, 53.5, -0.25, 0.26) + seam(V, [[52.5, MV + 1.15], [54.5, MV + 1.15]], 0);
	return s + wear(cd2, '#fffdf6', 1.2, 0.6);
}
// The clamp end on: horns hugging the barrel's underside (r 1.15) above a block round the tube.
function clampEnd(V, fill) {
	const P = (r, t) => [r * Math.cos(t), r * Math.sin(t)];
	const t0 = Math.asin(0.9 / 1.4), t1 = Math.asin(0.9 / 1.15);
	const arc = (r, a, b, n = 18) => Array.from({ length: n + 1 }, (_, k) => P(r, a + (b - a) * k / n));
	const horn = [...arc(1.4, t0, -Math.PI - t0), ...arc(1.15, -Math.PI - t1, t1)];
	const block = [[-1.1, -3.4], [1.1, -3.4], [1.1, -1.0], [-1.1, -1.0]];
	return shp(pth(V, block, 0.3), fill, 'pL') + shp(pth(V, horn, 0), fill, 'pL') +
		pl(V, arc(1.17, -Math.PI - t1 + 0.1, t1 - 0.1), '#5d5240', Math.max(1, 0.05 * V.s), 'opacity=".5"');   // the barrel's seat
}
function tubeEnd(V, back) {
	let s = clampEnd(V, back ? 'url(#ivoryLo)' : 'url(#ivoryMid)');
	if (!back) {
		s += disc(V, 0, MV, 1.2, 'rIron', 'mM') + ring(V, 0, MV, 1.2, GILT, 0.12 * V.s);
		s += ao(circ(V, 0, MV, 0.9, 'url(#rSteel)', 'mS')) + ring(V, 0, MV, 0.72, '#4a4540', 1.2) + circ(V, 0, MV, 0.3, 'url(#rIron)', 'mS');
		s += proof(V, 0, MV, 0.55);
	} else {
		s += disc(V, 0, MV, 1.05, 'rIron', 'mM') + ring(V, 0, MV, 0.92, '#4a4540', 1.2);
		s += circ(V, 0, MV, 0.84, '#141312') + circ(V, 0, MV, 0.6, 'url(#rShell)') + circ(V, 0, MV, 0.24, 'url(#rBrass)');   // the next shell's head
	}
	return s;
}

// ==== Sight_Flip: a flip-up rear aperture with a gilt ring, a post on an ivory tower (gun space; Aim (4.9, 5.1)) =======
const V0 = 3.15, H = V0 + 1.95, UR = 4.9, UF = 16.4;
const LEAF = [[-1.1, V0 + 0.45], [1.1, V0 + 0.45], [1.3, H - 0.2], [1.3, H + 0.6], [0.85, H + 1.15], [-0.85, H + 1.15], [-1.3, H + 0.6], [-1.3, H - 0.2]];
const TOWER = [[-1.1, V0 + 0.45], [1.1, V0 + 0.45], [0.8, H - 0.5], [-0.8, H - 0.5]];
function flipSide(V) {
	let s = '';
	for (const [u, half] of [[UR, 1.1], [UF, 1.0]]) {   // the clamps on the rail, their gilt cross pins and screws
		s += ao(RR(V, u - half, V0, u + half, V0 + 0.5, 0.12, 'url(#iron)', 'mS')) + pl(V, [[u - half + 0.1, V0 + 0.44], [u + half - 0.1, V0 + 0.44]], '#9a948c', 0.9, 'opacity=".5"');
		s += giltScrew(V, u + half - 0.4, V0 + 0.27, 0.15);
	}
	// the rear leaf: its hinge knuckle, a flat spring behind it, the gilt aperture ring
	s += ao(shp(pth(V, [[UR + 0.22, V0 + 0.5], [UR + 1.2, V0 + 0.5], [UR + 1.25, V0 + 0.7], [UR + 0.22, V0 + 1.3]], 0.08), 'url(#steelCyl)', 'mS'));
	s += ao(RR(V, UR - 0.22, V0 + 0.45, UR + 0.22, H + 1.15, 0.12, 'url(#iron)', 'mM'));
	s += ao(RR(V, UR - 0.32, H - 0.92, UR - 0.16, H + 0.92, 0.04, 'url(#giltCylH)', 'bS')) + pl(V, [[UR - 0.1, H - 0.8], [UR - 0.1, H + 0.8]], '#1a1816', 1, 'opacity=".5"');
	s += ao(circ(V, UR, V0 + 0.62, 0.3, 'url(#rIron)', 'mS')) + circ(V, UR, V0 + 0.62, 0.12, 'url(#rGilt)', 'bS');
	for (let k = 0; k < 4; k++) s += pl(V, [[UR - 0.2, H + 0.95 - k * 0.12], [UR + 0.2, H + 0.95 - k * 0.12]], '#1a1816', 0.8, 'opacity=".5"');
	// the tower: ivory, an index line, the post with its cyan dot toward the eye
	const tw = pth(V, R4(UF - 0.6, V0 + 0.45, UF + 0.6, H - 0.5), 0.1);
	s += shp(tw, 'url(#ivory)', 'pM') + clip(tw, flt('soft', pl(V, [[UF - 0.5, H - 0.62], [UF + 0.5, H - 0.62]], '#ffffff', 0.12 * V.s, 'opacity=".8"')));
	s += gline(V, [[UF - 0.6, V0 + 1.0], [UF + 0.6, V0 + 1.0]], 0.04) + screw(V, UF + 0.1, V0 + 0.75, 0.13);
	s += ao(shp(pth(V, [[UF - 0.3, H - 0.6], [UF + 0.3, H - 0.6], [UF + 0.2, H + 0.27], [UF - 0.2, H + 0.27]], 0.06), 'url(#iron)', 'mS'));
	s += ao(RR(V, UF - 0.27, H - 0.2, UF - 0.17, H + 0.2, 0.03, 'url(#cyan)', null));
	return s;
}
function flipEnd(V, back) {
	let s = '';
	const clampX = half => ao(RR(V, -half, V0, half, V0 + 0.5, 0.1, 'url(#iron)', 'mS')) +
		ao(RR(V, -half - 0.15, V0 + 0.13, -half, V0 + 0.41, 0.05, 'url(#rGilt)', 'bS')) + ao(RR(V, half, V0 + 0.13, half + 0.15, V0 + 0.41, 0.05, 'url(#rGilt)', 'bS'));
	const leafD = pth(V, LEAF, 0.15) + ' ' + rounded(Array.from({ length: 40 }, (_, k) => V.P(0.8 * Math.cos(-k * Math.PI / 20), H + 0.8 * Math.sin(-k * Math.PI / 20))), 0);
	const leaf = () => clampX(1.15) + ao(`<path d="${leafD}" fill="url(#iron)" fill-rule="evenodd" filter="url(#mM)"/>`) +
		(back ? ring(V, 0, H, 0.86, 'url(#gilt)', 0.13 * V.s) + ring(V, 0, H, 0.86, '#5a4214', 0.8, 'opacity=".5"') : ring(V, 0, H, 0.82, '#8a847c', 1.2, 'opacity=".6"')) +
		pl(V, [[-0.08, V0 + 0.6], [0.08, V0 + 0.6]], GILT, 0.06 * V.s);
	const tower = () => clampX(1.05) + shp(pth(V, TOWER, 0.12), back ? 'url(#ivoryLo)' : 'url(#ivoryMid)', 'pM') +
		ao(shp(pth(V, [[-0.24, H - 0.6], [0.24, H - 0.6], [0.24, H + 0.15], [0, H + 0.27], [-0.24, H + 0.15]], 0.08), 'url(#iron)', 'mS'));
	if (back) {
		s += tower() + ao(circ(V, 0, H, 0.2, 'url(#cyan)')) + leaf();   // the eye's view: the dot sits in the aperture
	} else {
		s += leaf() + tower() + gline(V, [[-0.95, V0 + 1.0], [0.95, V0 + 1.0]], 0.04);
	}
	return s;
}

// ==== Stock_Skeleton: lightened ivory field stock, a shell cuff (shells on the right), a ribbed pad (gun space) ========
const FIELD = [[0, 1.8], [0, -3.0], [-2.5, -4.4], [-5.5, -7.4], [-9, -8.0], [-12, -7.4], [-37, -12.6], [-38.5, -12.4], [-38.5, 0.2], [-36, 0.6], [-14, 1.0], [-4, 1.8]];
const HOLE = [[-16.5, -8.0], [-33.0, -10.9], [-34.2, -10.0], [-34.0, -6.4], [-32.5, -5.6], [-18.0, -4.9]];
const CUFF = [[-30, -5.8], [-20, -5.5], [-20, 0.96], [-30, 0.9]];
const PAD = [[-38.4, -12.7], [-40.2, -12.6], [-40.2, 0.4], [-38.4, 0.3]];
const SHELLS = [-28.2, -26.0, -23.8, -21.6];
function stockSide(V) {
	const right = V.kind === 'right';
	const outer = pth(V, FIELD, 0.9), hole = pth(V, HOLE, 0.9);
	let s = '';
	// the pad: rubber, ribbed, on an iron spacer
	const pad = pth(V, PAD, 0.4);
	s += shp(pad, 'url(#rubber)', 'rM') + clip(pad, Array.from({ length: 13 }, (_, k) => pl(V, [[-40.4, -12 + k], [-39.1, -12 + k]], '#111', 1.4) + pl(V, [[-40.4, -11.92 + k], [-39.1, -11.92 + k]], '#555', 0.8, 'opacity=".6"')).join(''));
	s += ao(RR(V, -38.75, -12.55, -38.3, 0.3, 0.06, 'url(#ironCylH)', 'mS'));
	// the stock, its lightening hole, an inset gilt hairline, stippling at the wrist
	const sd = `<path d="${outer} ${hole}" fill="url(#ivory)" fill-rule="evenodd" filter="url(#pL)"/>`;
	s += sd;
	s += clipEO(outer + ' ' + hole, `<path d="${hole}" fill="none" stroke="#6d6250" stroke-width="${r2(0.5 * V.s)}" opacity=".35" filter="url(#soft)"/>` +
		flt('soft', pl(V, [[-4, 1.45], [-14, 0.68], [-36, 0.3]], '#ffffff', 0.3 * V.s, 'opacity=".7"')) +
		flt('soft2', pl(V, [[-12.5, -6.9], [-36.5, -11.9]], '#6d6250', 0.5 * V.s, 'opacity=".2"')));
	s += `<path d="${hole}" fill="none" stroke="#fffdf6" stroke-width="1.3" opacity=".7" transform="translate(0,1.2)"/>`;
	s += gline(V, [[-15.5, 0.4], [-35.6, 0.0], [-37.6, -0.6], [-37.6, -11.4], [-36.6, -11.6]], 0.045);
	const grip = pth(V, [[-0.8, -3.4], [-2.8, -4.6], [-5.6, -7.0], [-8.6, -7.4], [-10.6, -6.9], [-9.4, -5.2], [-6.6, -3.6], [-3.4, -2.2], [-0.8, -2.0]], 0.6);
	s += shp(grip, 'url(#stip)', null, 'opacity=".75"') + shp(grip, 'none', null, `stroke="${GILT}" stroke-width="${r2(Math.max(0.8, 0.04 * V.s))}" opacity=".8"`);
	// the front seat plate, a sling stud at the toe
	s += ao(RR(V, -0.4, -3.0, 0.02, 1.75, 0.06, 'url(#iron)', 'mS'));
	s += ao(circ(V, -35.2, -11.5, 0.42, 'url(#rIron)', 'mS')) + ring(V, -35.2, -11.5, 0.22, '#0e0d0c', 0.09 * V.s);
	// the cuff: dark polymer, gilt rivets, the shells (right side only)
	const cf = pth(V, CUFF, 0.7);
	s += shp(cf, 'url(#s1cuff)', 'pM') + clip(cf, flt('soft', pl(V, [[-29.4, 0.55], [-20.6, 0.62]], '#ffffff', 0.18 * V.s, 'opacity=".25"')));
	if (right) {
		s += RR(V, -29.6, -4.6, -20.4, 0.6, 0.3, '#22201e', null, 'opacity=".9"');
		SHELLS.forEach((u, k) => { s += shellUp(V, u, -4.9 + k * 0.08); });
		s += ao(RR(V, -29.8, -1.6, -20.2, -0.9, 0.15, 'url(#s1cuff)', 'pS'));                   // the strap across the shells
	} else {
		s += seam(V, [[-29.2, -2.4], [-20.8, -2.3]], 0) + seam(V, [[-29.2, -1.3], [-20.8, -1.2]], 0);
	}
	for (const [a, v] of [[-29.3, -5.1], [-20.7, -4.85], [-29.3, 0.3], [-20.7, 0.35]]) s += giltScrew(V, a, v, 0.2);
	return s + wear(outer, '#fffdf6', 1.2, 0.55) + wear(pad, '#6a6a6a', 1, 0.4, '3 22 2 40');
}
function stockEnd(V, back) {
	const right = -1;
	let s = '';
	const shellsCol = () => {   // seen end on, the shells stand as a column of red past the cuff on the gun's right
		const a = right * 2.8;
		return ao(RR(V, a - 1.02, -4.9, a + 1.02, -3.8, 0.08, 'url(#brassCylH)', 'bS')) + ao(RR(V, a - 0.95, -3.9, a + 0.95, 1.1, 0.18, 'url(#shellCylH)', 'pS'));
	};
	const cuffD = pth(V, R4(-2.45, -5.8, 2.45, 0.96), 0.7);
	if (back) {
		s += shp(cuffD, 'url(#s1cuff)', 'pM') + shellsCol();
		s += shp(pth(V, R4(-2.1, -12.6, 2.1, 1.8), 1.0), 'url(#ivoryLo)', 'pL');
		const pad = pth(V, R4(-2.2, -12.7, 2.2, 0.4), 0.5);
		s += shp(pad, 'url(#rRubber)', 'rM') + clip(pad, shp(pad, 'url(#stipDark)', null, 'opacity=".6"'));
		s += clip(pad, Array.from({ length: 12 }, (_, k) => pl(V, [[-2.2, -11.7 + k], [2.2, -11.7 + k]], '#0f0f0f', 1.4) + pl(V, [[-2.2, -11.62 + k], [2.2, -11.62 + k]], '#4a4a4a', 0.8, 'opacity=".6"')).join(''));
		for (const v of [-9.8, -1.8]) s += circ(V, 0, v, 0.32, '#0c0c0c') + ring(V, 0, v, 0.22, '#5a5a5a', 1) + circ(V, 0, v, 0.12, 'url(#rSteel)');
		s += wear(pad, '#6a6a6a', 1, 0.45, '3 22 2 40');
	} else {
		s += shp(pth(V, R4(-2.2, -12.7, 2.2, 0.4), 0.5), 'url(#rubber)', 'rM');
		s += shp(pth(V, R4(-2.1, -12.6, 2.1, 1.8), 1.0), 'url(#ivoryMid)', 'pL');
		s += clip(pth(V, R4(-2.1, -12.6, 2.1, 1.8), 1.0), RR(V, -2.2, -12.7, 2.2, -8.2, 0, 'url(#ivoryLo)', null, 'opacity=".85"') +
			flt('soft', pl(V, [[-1.4, 1.4], [1.4, 1.4]], '#ffffff', 0.2 * V.s, 'opacity=".8"')));
		s += shp(cuffD, 'url(#s1cuff)', 'pM') + clip(pth(V, R4(-2.1, -12.6, 2.1, 1.8), 1.0), RR(V, -2.1, -5.8, 2.1, 0.96, 0.6, 'url(#ivoryMid)', 'pL'));
		s += shellsCol();
		// the seat plate the body's back face meets: a locating hole, two screws
		const plate = pth(V, R4(-1.55, -3.0, 1.55, 1.75), 0.3);
		s += ao(shp(plate, 'url(#s1face)', 'mM')) + circ(V, 0, 0, 0.58, '#0e0d0c') + ring(V, 0, 0, 0.66, GILT, 0.06 * V.s);
		for (const v of [-2.2, 1.0]) s += screw(V, 0, v, 0.22);
		s += ao(circ(V, 0, -11.5, 0.3, 'url(#rIron)', 'mS'));
	}
	return s;
}

module.exports = [
	{ dir: 'Body_Standard', b: { u: [-0.6, 20],v: [-7.85, 3.15], w: [-1.55, 1.55] }, draw: { left: bodySide, right: bodySide, front: V => bodyEnd(V, false), back: V => bodyEnd(V, true) } },
	{ dir: 'Barrel_Field', b: { u: [0, 46], v: [-1.27, 2.06], w: [-1.4, 1.4] }, draw: { left: barrelSide, right: barrelSide, front: V => barrelEnd(V, false), back: V => barrelEnd(V, true) } },
	{ dir: 'Muzzle_Crown', b: { u: [0, 1.0], v: [-1.18, 1.18], w: [-1.18, 1.18] }, draw: { left: crownSide, right: crownSide, front: V => crownEnd(V, false), back: V => crownEnd(V, true) } },
	{ dir: 'Magazine_Tube6', b: { u: [20, 56], v: [-3.7, 0.9], w: [-1.4, 1.4] }, draw: { left: tubeSide, right: tubeSide, front: V => tubeEnd(V, false), back: V => tubeEnd(V, true) } },
	{ dir: 'Sight_Flip', b: { u: [3.8, 17.4], v: [3.15, 6.25], w: [-1.3, 1.3] }, draw: { left: flipSide, right: flipSide, front: V => flipEnd(V, false), back: V => flipEnd(V, true) } },
	{ dir: 'Stock_Skeleton', b: { u: [-40.2, 0], v: [-12.7, 1.8], w: [-3.75, 2.45] }, draw: { left: stockSide, right: stockSide, front: V => stockEnd(V, false), back: V => stockEnd(V, true) } },
];
