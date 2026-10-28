// Set 4 (Meridian: clean, precise, a surveyors'-instrument maker) concept views of the Ranchhand's parts (2026-10-28).
// Six new designs (no other set's outlines recoloured): a flowing two-tone receiver with a level vial and status light,
// a fluted slug barrel with machined micrometer sights, a slim knurled choke, a graphite tube with a smoked shell window,
// a transit-telescope 1-4x scope on aluminium standards, a sculpted two-tone thumbhole stock. Satin-white polymer,
// graphite, machined aluminium, fine brass, a cyan accent line. Interfaces as Ranchhand.py (contract in README).
const L = require('./lib');
const { pth, R4, shp, circ, ring, pl, cb, cd, sk, clip, flt, full, seam, accent, knurlRing, qd, pin, screw, ao, wear,
	lin, GRADS, N, r2 } = L;

GRADS.push(
	lin('s4whtCyl', [[0, '#9d9b93'], [0.16, '#dddbd3'], [0.3, '#ffffff'], [0.46, '#eceae4'], [0.8, '#c3c1b9'], [1, '#8a8881']]),
	lin('s4whtCylH', [[0, '#9d9b93'], [0.16, '#dddbd3'], [0.3, '#ffffff'], [0.46, '#eceae4'], [0.8, '#c3c1b9'], [1, '#8a8881']], false),
	lin('s4vial', [[0, '#d8fbff'], [0.45, '#7fdcf0'], [1, '#2a94b0']]),
	lin('s4smoke', [[0, '#3a4652', 0.92], [0.5, '#1d262f', 0.88], [1, '#0e1318', 0.92]]),
);
const shpE = (d, fill, filt, extra = '') => `<path d="${d}" fill="${fill}" fill-rule="evenodd"${filt ? ` filter="url(#${filt})"` : ''} ${extra}/>`;
const shift = (segs, du, dv) => segs.map(p => (p[0] === 'C' ? ['C', ...p.slice(1).map(([a, v]) => [a + du, v + dv])] : [p[0] + du, p[1] + dv]));
// A section seen end on: white above the parting height pv, graphite below.
function xsec(V, d, top, low, pv, filt = 'pM') {
	return shp(d, top, filt) + flt(filt, clip(d, shp(pth(V, R4(-9, -40, 9, pv), 0), low)));
}
const glowShp = (d, fill, extra = '') => shp(d, fill, null, extra);
// Fine graduation ticks along u at height v (every `step`, long every `big`th).
function ticks(V, u0, u1, v, step, len, col = '#4a4e55', big = 4, px = 1) {
	let s = '';
	for (let k = 0, u = u0; u <= u1 + 1e-6; u += step, k++) s += pl(V, [[u, v], [u, v - (k % big ? len : len * 1.9)]], col, px);
	return s;
}
const slot = (V, a, v, r0, r1) => pl(V, [[a - r0, v], [a + r0, v]], '#2a2c30', Math.max(1, r1 * V.s));

// =============================================================================================================== Body
// A flowing two-tone receiver: satin-white upper over a graphite lower, split by one sweeping parting line with the
// cyan accent; the trigger housing grows out of the belly into one swept oversized guard; a nose that rolls over into
// a brass-ringed knurled nut; a flush brushed panel on the left (the element plate's place), the ejection port in it on
// the right; a cyan status light (left), a level vial (right); ember slits in a recessed nose strip; graduations under
// the aluminium rail.
const SHB = [[0, -2.8], [0, 1.5], cb([0, 2.0], [0.35, 2.25], [1.0, 2.25]), [15.2, 2.25], cb([17.6, 2.25], [19.1, 2.05], [19.55, 1.45]),
	[19.55, -3.0], [9.0, -3.0], cb([8.3, -3.0], [8.1, -3.4], [7.75, -4.0]), cb([7.45, -4.45], [7.0, -4.55], [6.4, -4.55]), [0.5, -4.55],
	cb([0.15, -4.55], [0, -4.4], [0, -4.1]), [0, -2.8]];
const PARTB = [[-1, -0.9], cb([4, -1.3], [10, -1.95], [14.6, -1.3]), cb([17, -0.95], [18.6, -0.1], [21, 0.35])];
const PANELB = [[5.7, -0.85], cb([5.7, 0.9], [6.0, 1.45], [6.8, 1.45]), [13.6, 1.45], cb([14.2, 1.45], [14.35, 1.1], [14.3, 0.5]), [14.15, -1.15],
	cb([14.1, -1.5], [13.8, -1.55], [13.3, -1.5]), [6.4, -1.15], cb([5.95, -1.12], [5.7, -1.05], [5.7, -0.85])];
// The trigger housing and guard at the receiver's back (moved 5 cm back on 2026-10-28 so the first-person finger reaches
// the trigger: Ranchhand.py's trigger_group), the guard's rear meeting the pistol grip's front.
const GUARDB = [[-0.2, -4.3], cb([-0.45, -6.4], [0.0, -8.1], [1.8, -8.1]), [5.4, -8.1], cb([7.4, -8.1], [8.2, -6.8], [7.9, -4.2])];
const TRIGB = [[2.5, -4.55], [3.4, -4.55], [3.3, -5.6], [3.05, -6.7], [2.45, -7.05], [2.62, -5.8]];
const NOSEB = [[15.0, -0.55], [18.9, -0.25], cb([19.25, -0.23], [19.35, 0.0], [19.3, 0.4]), [19.2, 1.2], cb([19.15, 1.45], [18.95, 1.5], [18.6, 1.5]), [15.3, 1.4],
	cb([14.95, 1.38], [14.8, 1.2], [14.8, 0.9]), [14.8, -0.25], cb([14.8, -0.45], [14.85, -0.55], [15.0, -0.55])];

function railS(V, u0, u1) {
	let s = ao(shp(pth(V, R4(u0, 2.2, u1, 2.65), 0.08), 'url(#alu)', 'mS'));
	for (let u = u0 + 0.35; u + 0.55 <= u1 - 0.2; u += 1.0) s += ao(shp(pth(V, R4(u, 2.6, u + 0.53, 3.15), 0.06), 'url(#alu)', 'mS'));
	return s + pl(V, [[u0 + 0.15, 2.58], [u1 - 0.15, 2.58]], '#6e737a', 0.9, 'opacity=".6"') + pl(V, [[u0 + 0.15, 2.28], [u1 - 0.15, 2.28]], '#ffffff', 0.8, 'opacity=".7"');
}
function bodySide(V) {
	const right = V.kind === 'right', shell = cd(V, SHB);
	let s = '';
	// the swept guard and the flat-faced aluminium trigger
	s += flt('pM', sk(V, GUARDB, '#3f4248', 0.72)) + sk(V, shift(GUARDB, -0.05, 0.2), '#9ca1a9', 0.1, 'opacity=".5"');
	s += ao(shp(pth(V, TRIGB, 0.16), 'url(#alu)', 'mS')) + pl(V, [[2.7, -4.9], [2.85, -6.4]], '#ffffff', 0.9, 'opacity=".6"');
	// the shell: white, the graphite lower under the parting line, form light, seams
	s += shp(shell, 'url(#wht)', 'pL');
	s += flt('pM', clip(shell, shp(cd(V, [...PARTB, [21, -10], [-1, -10]]), 'url(#gph)')));
	s += clip(shell, `<rect x="0" y="${r2(V.Y(2.4))}" width="${N}" height="${r2(5 * V.s)}" fill="url(#form4)"/>` +
		flt('soft', sk(V, [[0.8, 1.75], [16.6, 1.8], cb([18, 1.8], [18.8, 1.55], [19.2, 1.1])], '#ffffff', 0.4, 'opacity=".75"')) +
		flt('soft', sk(V, [[0.3, -4.1], cb([0.8, -4.15], [3, -4.15], [7, -4.1])], '#9aa0a8', 0.22, 'opacity=".35"')) +
		flt('soft2', sk(V, shift(PARTB, 0, 0.55), '#2a3040', 0.9, 'opacity=".14"')) +
		seam(V, [[14.65, 2.4], cb([14.3, 0.6], [14.4, -1.6], [14.9, -3.2])]) + seam(V, [[0.7, 2.4], cb([0.5, 0.5], [0.5, -1.6], [0.8, -3.3])]) +
		accent(V, PARTB, 0.12));
	// graduations along the top edge under the rail
	s += ticks(V, 1.5, 16.0, 2.05, 0.5, 0.16, '#6a6f76', 4, 0.9);
	// the flush brushed panel (left: plain, the element plate's place; right: the ejection port and bolt)
	const pan = cd(V, PANELB);
	s += shp(pan, 'url(#alu)', 'mS') + clip(pan, `${full} fill="url(#brush)"/>`) + shp(pan, 'none', null, 'stroke="#3b3e43" stroke-width="1.3" opacity=".75"');
	if (right) {
		const port = pth(V, R4(6.1, -0.75, 13.9, 1.15), 0.35);
		s += shp(port, '#16171a', null);
		s += clip(port, ao(shp(pth(V, R4(6.4, -0.55, 13.5, 0.95), 0.2), 'url(#steelCyl)', 'mS')) +
			pl(V, [[6.7, 0.2], [13.2, 0.2]], '#3a3d42', 1.2) + shp(pth(V, R4(12.2, -0.2, 13.2, 0.6), 0.1), '#2a2c30', null) +
			pl(V, [[6.6, 0.85], [13.3, 0.85]], '#ffffff', 0.9, 'opacity=".6"'));
		s += shp(port, 'none', null, 'stroke="#0f1012" stroke-width="1.6"');
		for (const [a, v] of [[6.4, 1.3], [13.9, 1.3], [6.2, -1.0], [13.8, -1.3]]) s += screw(V, a, v, 0.13);
	} else {
		for (const [a, v] of [[6.5, 1.1], [13.7, 1.1], [6.3, -0.75], [13.65, -1.15]]) s += screw(V, a, v, 0.15);
		for (const dv of [0, 0.4]) s += pl(V, [[7.2, 0.1 + dv], [12.8, 0.0 + dv]], '#7f858c', 0.9, 'opacity=".4"');
	}
	// the nose: a recessed graphite strip, four slanted ember slits
	s += shp(cd(V, NOSEB), 'url(#gph)', 'pS');
	for (let i = 0; i < 4; i++) {
		const u = 15.4 + i * 0.85;
		s += shp(pth(V, [[u, -0.05], [u + 0.32, -0.05], [u + 0.72, 1.0], [u + 0.4, 1.0]], 0.12), '#140b06', null);
		s += glowShp(pth(V, [[u + 0.07, 0.08], [u + 0.25, 0.08], [u + 0.6, 0.88], [u + 0.42, 0.88]], 0.06), 'url(#emberLin)');
	}
	// the barrel nut: a fine brass ring, then a knurled graphite nut whose face is at u 20
	s += ao(shp(pth(V, R4(19.12, -1.55, 19.32, 1.55), 0.05), 'url(#brassCyl)', 'bS'));
	s += ao(shp(pth(V, R4(19.3, -1.45, 20.0, 1.45), 0.08), 'url(#gphCyl)', 'mS'));
	for (let a = 19.4; a < 19.95; a += 0.11) s += pl(V, [[a, -1.3], [a, 1.3]], '#0d0e10', 0.9, 'opacity=".55"');
	// the magazine tube's seat collar under it
	s += ao(shp(pth(V, R4(19.7, -3.65, 20.0, -1.35), 0.06), 'url(#aluCyl)', 'mS'));
	// left: the cyan status light in an aluminium bezel; right: a level vial set in the flank
	if (!right) {
		s += ao(circ(V, 2.6, 0.9, 0.44, 'url(#rAlu)', 'mS')) + circ(V, 2.6, 0.9, 0.3, '#0f1214') + circ(V, 2.6, 0.9, 0.24, 'url(#cyan)') + circ(V, 2.52, 0.98, 0.07, '#ffffff', null, 'opacity=".8"');
	} else {
		const vh = pth(V, R4(1.15, 0.38, 4.45, 1.18), 0.36), vg = pth(V, R4(1.45, 0.55, 4.15, 1.01), 0.23);
		s += ao(shp(vh, 'url(#alu)', 'mS')) + shp(vg, 'url(#s4vial)', null) + shp(vg, 'none', null, 'stroke="#1b4e5c" stroke-width="1.1"');
		s += `<ellipse cx="${r2(V.X(2.95))}" cy="${r2(V.Y(0.85))}" rx="${r2(0.42 * V.s)}" ry="${r2(0.12 * V.s)}" fill="#ffffff" opacity=".9"/>`;
		for (const a of [2.35, 3.55]) s += pl(V, [[a, 0.56], [a, 1.0]], '#123a46', 1.1);
	}
	// trigger pins, a brass cross-bolt safety, a QD sling cup at the rear
	for (const [a, v] of [[1.0, -3.9], [6.4, -3.9]]) s += pin(V, a, v, 0.15);
	s += ao(circ(V, 7.25, -3.55, 0.3, 'url(#rBrass)', 'bS')) + ring(V, 7.25, -3.55, 0.3, '#2a2c30', 1, 'opacity=".6"');
	s += qd(V, 1.6, -2.25, 0.36);
	s += railS(V, 1, 19.5);
	return s + wear(shell, '#b9bec5', 1, 0.35, '3 34 2 60');
}
const XB_MAIN = [[-1.1, 2.25], [1.1, 2.25], cb([1.45, 2.25], [1.6, 2.0], [1.6, 1.6]), [1.6, -2.6], cb([1.6, -2.9], [1.45, -3.05], [1.15, -3.05]), [-1.15, -3.05],
	cb([-1.45, -3.05], [-1.6, -2.9], [-1.6, -2.6]), [-1.6, 1.6], cb([-1.6, 2.0], [-1.45, 2.25], [-1.1, 2.25])];
const XB_HOUS = [[-1.35, -2.9], [1.35, -2.9], cb([1.35, -4.0], [1.0, -4.55], [0.5, -4.55]), [-0.5, -4.55], cb([-1.0, -4.55], [-1.35, -4.0], [-1.35, -2.9])];
function railX(V) {
	let s = ao(shp(pth(V, R4(-1.05, 2.2, 1.05, 2.65), 0.06), 'url(#alu)', 'mS'));
	s += ao(shp(pth(V, [[-1.05, 2.6], [1.05, 2.6], [1.05, 2.82], [0.85, 2.95], [1.05, 3.05], [0.9, 3.15], [-0.9, 3.15], [-1.05, 3.05], [-0.85, 2.95], [-1.05, 2.82]], 0.03), 'url(#alu)', 'mS'));
	return s + pl(V, [[-0.85, 3.08], [0.85, 3.08]], '#ffffff', 0.9, 'opacity=".7"');
}
function bodyEnd(V, back) {
	let s = ao(shp(pth(V, R4(-0.36, -8.45, 0.36, -4.2), 0.3), 'url(#gphCylH)', 'pM'));      // the guard, end on
	s += flt('pM', shp(cd(V, XB_HOUS), back ? 'url(#gphLo)' : 'url(#gph)'));
	s += railX(V);
	s += xsec(V, cd(V, XB_MAIN), back ? 'url(#whtLo)' : 'url(#whtMid)', back ? 'url(#gphLo)' : 'url(#gph)', back ? -0.9 : 0.25, 'pL');
	s += clip(cd(V, XB_MAIN), flt('soft', sk(V, [[-1.0, 1.95], [1.0, 1.95]], '#ffffff', 0.18, 'opacity=".8"')));
	const ap = back ? -0.75 : 0.4;   // the cyan accent where it meets the outline
	s += clip(cd(V, XB_MAIN), accent(V, [[-1.7, ap], [-1.45, ap]], 0.1) + accent(V, [[1.45, ap], [1.7, ap]], 0.1));
	// nubs: the status light (gun's left, +w) and the vial (gun's right)
	s += ao(shp(pth(V, R4(1.58, 0.5, 1.7, 1.3), 0.04), 'url(#alu)', 'mS')) + ao(shp(pth(V, R4(-1.68, 0.4, -1.58, 1.16), 0.04), 'url(#alu)', 'mS'));
	if (!back) {
		// the front face: brass ring, knurled nut and the barrel's seat; the tube seat and the pump's bar slots
		s += ao(circ(V, 0, -2.5, 1.15, 'url(#rAlu)', 'mS')) + circ(V, 0, -2.5, 1.02, '#16171a') + ring(V, 0, -2.5, 0.9, '#3c3f44', 1) + circ(V, 0, -2.5, 0.6, 'url(#bore)');
		s += ao(circ(V, 0, 0, 1.5, 'url(#rBrass)', 'bS')) + ao(circ(V, 0, 0, 1.4, 'url(#rGph)', 'mM')) + knurlRing(V, 0, 0, 1.4, 44, '#0d0e10');
		s += circ(V, 0, 0, 1.15, '#101113') + circ(V, 0, 0, 0.95, 'url(#bore)');
		for (const r of [1.08, 1.0]) s += ring(V, 0, 0, r, '#3c3f44', 1, 'opacity=".8"');
		for (const sd of [-1, 1]) s += shp(pth(V, R4(sd > 0 ? 1.24 : -1.48, -2.3, sd > 0 ? 1.48 : -1.24, -1.8), 0.05), '#0c0d0e', null);
	} else {
		// the back face: a brushed aluminium seat plate, guide rails, two screws and the stock's locating boss at (0, 0)
		const bp = pth(V, R4(-1.42, -2.9, 1.42, 1.55), 0.3);
		s += ao(shp(bp, 'url(#alu)', 'mM')) + clip(bp, `${full} fill="url(#brushV)"/>`);
		for (const sd of [-1, 1]) s += shp(pth(V, R4(sd * 1.0 - 0.12, -2.4, sd * 1.0 + 0.12, 1.05), 0.06), '#7d838a', 'mS');
		s += screw(V, 0, -2.2, 0.24) + screw(V, 0, 1.12, 0.22);
		s += ao(circ(V, 0, 0, 0.5, 'url(#rGph)', 'mS')) + circ(V, 0, 0, 0.24, '#0f1012') + ring(V, 0, 0, 0.6, '#c9a24a', 1.2, 'opacity=".9"');
	}
	return s;
}

// ============================================================================================================= Barrel
// Slug barrel (local u from the nut, 46 long): graphite, r 1.3, two runs of upper flutes, an aluminium seating collar
// with wrench flats, a brass ring and a cyan index ring; a machined rear sight with a graduated elevation slide and a
// brass windage drum (right); a hooded front post with a cyan fibre; plain underside; bright steel threads at the tip.
const BRL = [[1.6, -1.3], [44.6, -1.3], [45.0, -1.15], [45.0, 1.15], [44.6, 1.3], [1.6, 1.3]];
const RSB = [[7.6, 1.0], [11.4, 1.0], [11.4, 1.45], cb([11.4, 1.7], [11.2, 1.78], [10.9, 1.78]), [8.1, 1.78], cb([7.8, 1.78], [7.6, 1.62], [7.6, 1.4])];
const SADDLE = [[2.6, 0.2], [7.35, 0.2], cb([7.5, 0.2], [7.55, 0.4], [7.55, 0.6]), [7.55, 1.25], cb([7.55, 1.5], [7.35, 1.58], [7.05, 1.58]), [3.2, 1.58],
	cb([2.8, 1.58], [2.6, 1.42], [2.6, 1.15])];
const saddleX = V => pth(V, [...Array.from({ length: 21 }, (_, i) => { const t = 0.13 + i / 20 * (Math.PI - 0.26); return [1.58 * Math.cos(t), 1.58 * Math.sin(t)]; }),
	...Array.from({ length: 21 }, (_, i) => { const t = Math.PI - 0.15 - i / 20 * (Math.PI - 0.3); return [1.28 * Math.cos(t), 1.28 * Math.sin(t)]; })], 0);
const FSB =[[40.9, 1.0], [44.8, 1.0], [44.8, 1.35], cb([44.8, 1.6], [44.6, 1.65], [44.3, 1.65]), [41.8, 1.65], cb([41.4, 1.65], [40.9, 1.3], [40.9, 1.0])];
function brlSide(V) {
	const right = V.kind === 'right';
	let s = ao(shp(pth(V, R4(44.9, -1.15, 46.0, 1.15), 0.04), 'url(#steelCyl)', 'mS'));          // muzzle threads
	for (let u = 45.05; u < 45.95; u += 0.12) s += pl(V, [[u, -1.12], [u + 0.05, 1.12]], '#2e3135', 0.9, 'opacity=".55"');
	s += ao(shp(pth(V, BRL, 0.06), 'url(#gphCyl)', 'mM'));
	for (const [u0, u1] of [[12.5, 25.0], [27.0, 38.5]]) for (const [vc, hw] of [[0.95, 0.07], [0.45, 0.11]]) {
		s += shp(pth(V, R4(u0, vc - hw, u1, vc + hw), hw), '#0f1012', null, 'opacity=".9"');
		s += pl(V, [[u0 + hw, vc - hw + 0.02], [u1 - hw, vc - hw + 0.02]], '#a9aeb5', 0.8, 'opacity=".5"');
	}
	s += clip(pth(V, BRL, 0.06), pl(V, [[2, 0.62], [44.4, 0.62]], '#ffffff', Math.max(1, 0.1 * V.s), 'opacity=".18"'));
	// seating collar and rings
	s += ao(shp(pth(V, R4(0, -1.42, 1.42, 1.42), 0.1), 'url(#aluCyl)', 'mM'));
	s += shp(pth(V, R4(0.3, -0.6, 1.15, 0.6), 0.04), 'url(#alu)', null, 'opacity=".9"') + pl(V, [[0.32, 0.6], [1.13, 0.6]], '#ffffff', 0.9, 'opacity=".8"') + pl(V, [[0.32, -0.6], [1.13, -0.6]], '#5c6167', 0.9, 'opacity=".8"');
	s += ao(shp(pth(V, R4(1.42, -1.37, 1.64, 1.37), 0.05), 'url(#brassCyl)', 'bS'));
	s += pl(V, [[2.3, -1.28], [2.3, 1.28]], '#14181c', 0.2 * V.s) + pl(V, [[2.3, -1.25], [2.3, 1.25]], '#5fd6f5', 0.11 * V.s) + pl(V, [[2.3, -1.2], [2.3, 1.2]], '#e8fcff', Math.max(0.7, 0.04 * V.s), 'opacity=".8"');
	s += ao(shp(pth(V, R4(39.9, -1.33, 40.1, 1.33), 0.04), 'url(#brassCyl)', 'bS'));
	// a satin-white saddle over the breech's top half, graduated, with the cyan line
	const sad = cd(V, SADDLE);
	s += shp(sad, 'url(#wht)', 'pM') + clip(sad, flt('soft', pl(V, [[3.0, 1.3], [7.2, 1.3]], '#ffffff', 0.12 * V.s, 'opacity=".8"')));
	s += ticks(V, 3.4, 6.8, 1.2, 0.34, 0.16, '#6a6f76', 5, 0.9) + clip(sad, accent(V, [[2.4, 0.55], [7.7, 0.55]], 0.08));
	// rear sight: a machined base with an elevation scale, a notched leaf, a brass windage drum on the right
	s += ao(shp(cd(V, RSB), 'url(#alu)', 'mS')) + clip(cd(V, RSB), `${full} fill="url(#brush)"/>`);
	s += ticks(V, 9.5, 11.0, 1.7, 0.25, 0.18, '#3a3d42', 2, 1) + pl(V, [[9.4, 1.3], [11.1, 1.3]], '#5fd6f5', 1.2);
	s += ao(shp(pth(V, [[8.3, 1.7], [9.1, 1.7], [9.0, 2.6], [8.4, 2.6]], 0.06), 'url(#alu)', 'mS')) + pl(V, [[8.45, 2.55], [8.95, 2.55]], '#ffffff', 0.9, 'opacity=".7"');
	s += screw(V, 8.0, 1.3, 0.12);
	if (right) s += ao(circ(V, 10.25, 1.4, 0.32, 'url(#rBrass)', 'bS')) + knurlRing(V, 10.25, 1.4, 0.32, 20, '#7a5718');
	// front sight: a graphite ramp, an aluminium hood with side windows, the post and its cyan fibre
	s += ao(shp(cd(V, FSB), 'url(#gph)', 'mS'));
	s += ao(shp(pth(V, [[43.05 - 0.2, 1.6], [43.05 + 0.2, 1.6], [43.05 + 0.13, 2.55], [43.05 - 0.13, 2.55]], 0.04), 'url(#gph)', 'mS'));
	s += glowShp(pth(V, R4(42.85, 2.48, 43.25, 2.72), 0.1), 'url(#cyanLin)');
	const hoodD = cd(V, [[42.0, 1.6], [44.1, 1.6], [44.1, 2.75], cb([44.1, 3.05], [43.9, 3.05], [43.6, 3.05]), [42.5, 3.05], cb([42.2, 3.05], [42.0, 3.0], [42.0, 2.75])]);
	const win = pth(V, R4(42.45, 1.95, 43.65, 2.75), 0.3);
	s += `<path d="${hoodD} ${win}" fill="url(#alu)" fill-rule="evenodd" filter="url(#mS)"/>` + shp(win, 'none', null, 'stroke="#3b3e43" stroke-width="1"');
	return s + wear(pth(V, BRL, 0.06), '#b3b9c1', 1, 0.35, '4 40 2 60');
}
function brlEnd(V, back) {
	let s = '';
	const rear = () => {
		let r = ao(shp(pth(V, R4(-0.8, 1.0, 0.8, 1.78), 0.18), 'url(#alu)', 'mS'));
		r += ao(shpE(pth(V, R4(-0.3, 1.7, 0.3, 2.6), 0.05) + ' ' + pth(V, [[-0.1, 2.6], [0.1, 2.6], [0.1, 2.35], [-0.1, 2.35]], 0.08), 'url(#alu)', 'mS'));
		r += ao(shp(pth(V, R4(-1.05, 1.08, -0.8, 1.72), 0.05), 'url(#brassCylH)', 'bS'));   // windage drum, gun's right
		return r;
	};
	const front = () => {
		let f = ao(shp(pth(V, R4(-0.55, 1.0, 0.55, 1.65), 0.15), 'url(#gph)', 'mS'));
		f += ao(shpE(pth(V, R4(-0.62, 1.55, 0.62, 3.05), 0.55) + ' ' + pth(V, R4(-0.45, 1.75, 0.45, 2.88), 0.42), 'url(#alu)', 'mS'));
		f += ao(shp(pth(V, [[-0.2, 1.6], [0.2, 1.6], [0.13, 2.55], [-0.13, 2.55]], 0.04), 'url(#gph)', 'mS'));
		f += circ(V, 0, 2.6, 0.13, 'url(#cyan)') + circ(V, -0.04, 2.64, 0.04, '#ffffff');
		return f;
	};
	const sad = () => shp(saddleX(V), back ? 'url(#whtLo)' : 'url(#whtMid)', 'pS');
	if (!back) {
		s += ao(circ(V, 0, 0, 1.42, 'url(#rAlu)', 'mM')) + sad() + rear();
		s += ao(circ(V, 0, 0, 1.3, 'url(#rGph)', 'mM')) + front();
		s += ao(circ(V, 0, 0, 1.15, 'url(#rSteel)', 'mS')) + ring(V, 0, 0, 1.08, '#5c6066', 1);
		s += circ(V, 0, 0, 0.92, 'url(#bore)') + ring(V, 0, 0, 0.92, '#d6dade', 1.6);
	} else {
		s += front() + rear() + sad();
		s += ao(circ(V, 0, 0, 1.42, 'url(#rAlu)', 'mM'));
		for (const r of [1.3, 1.2]) s += ring(V, 0, 0, r, '#6c7178', 1, 'opacity=".8"');
		s += circ(V, 0, 0, 1.12, 'url(#rGph)', 'mS') + circ(V, 0, 0, 0.92, 'url(#bore)');
		s += shp(pth(V, R4(-0.13, 1.12, 0.13, 1.38), 0.03), 'url(#brass)', 'bS');   // alignment key
	}
	return s;
}

// ============================================================================================================= Muzzle
// Choke (local, 4.6 long): a fine diamond-knurled aluminium grip ring, a brass hairline, a slim graphite body with
// four raked oval ports (ember in them), a cyan index ring, a bright machined chamfer, a graduated face.
function chokeSide(V) {
	let s = ao(shp(pth(V, R4(0, -1.22, 0.32, 1.22), 0.04), 'url(#gphCyl)', 'mS'));
	const kr = pth(V, R4(0.3, -1.38, 1.5, 1.38), 0.08);
	s += ao(shp(kr, 'url(#aluCyl)', 'mM'));
	let k = '';
	for (let a = -1.5; a < 3; a += 0.16) k += pl(V, [[a, -1.4], [a + 1.4, 1.4]], '#5c6167', 0.9, 'opacity=".55"') + pl(V, [[a + 1.4, -1.4], [a, 1.4]], '#5c6167', 0.9, 'opacity=".55"');
	s += clip(kr, k) + clip(kr, pl(V, [[0.3, 0.6], [1.5, 0.6]], '#ffffff', 0.3 * V.s, 'opacity=".25"'));
	s += ao(shp(pth(V, R4(1.5, -1.3, 1.65, 1.3), 0.03), 'url(#brassCyl)', 'bS'));
	const bd = pth(V, R4(1.65, -1.25, 4.22, 1.25), 0.05);
	s += ao(shp(bd, 'url(#gphCyl)', 'mM'));
	// the side port (face on) and the top and bottom ports at the silhouette
	const port = pth(V, [[2.2, -0.3], [3.55, -0.3], [3.8, 0.3], [2.45, 0.3]], 0.28);
	s += shp(port, '#0b0c0e', null) + glowShp(pth(V, [[2.45, -0.14], [3.4, -0.14], [3.58, 0.14], [2.62, 0.14]], 0.12), 'url(#emberLin)', 'opacity=".85"');
	s += pl(V, [[2.3, -0.33], [3.6, -0.33]], '#a9aeb5', 1, 'opacity=".55"');
	for (const sd of [-1, 1]) {
		s += clip(bd, shp(pth(V, R4(2.25, sd > 0 ? 1.0 : -1.3, 3.75, sd > 0 ? 1.3 : -1.0), 0.12), '#0b0c0e', null));
		s += clip(bd, glowShp(pth(V, R4(2.45, sd > 0 ? 1.1 : -1.3, 3.55, sd > 0 ? 1.3 : -1.1), 0.06), 'url(#emberLin)', 'opacity=".7"'));
	}
	s += pl(V, [[3.98, -1.24], [3.98, 1.24]], '#14181c', 0.16 * V.s) + pl(V, [[3.98, -1.22], [3.98, 1.22]], '#5fd6f5', 0.08 * V.s);
	s += ao(shp(pth(V, [[4.2, -1.25], [4.6, -1.1], [4.6, 1.1], [4.2, 1.25]], 0.02), 'url(#steelCyl)', 'mS'));
	return s + wear(bd, '#b9bec5', 1.1, 0.45, '6 30 3 50');
}
function chokeEnd(V, back) {
	let s = ao(circ(V, 0, 0, 1.38, 'url(#rAlu)', 'mM')) + knurlRing(V, 0, 0, 1.38, 52, '#6a6f76');
	if (!back) {
		s += ao(circ(V, 0, 0, 1.25, 'url(#rSteel)', 'mS'));
		s += circ(V, 0, 0, 1.1, 'url(#rAlu)', 'mS');
		for (let k = 0; k < 24; k++) {
			const t = k * Math.PI / 12, r0 = k % 6 ? 1.0 : 0.94;
			s += pl(V, [[r0 * Math.cos(t), r0 * Math.sin(t)], [1.07 * Math.cos(t), 1.07 * Math.sin(t)]], k === 6 ? '#2aa8cf' : '#4a4e55', k === 6 ? 2 : 1);
		}
		s += circ(V, 0, 0, 0.88, '#0d0e0f') + ring(V, 0, 0, 0.88, '#dfe3e7', 1.4) + circ(V, 0, 0, 0.78, 'url(#bore)');
	} else {
		s += ao(circ(V, 0, 0, 1.22, 'url(#rGph)', 'mM'));
		s += circ(V, 0, 0, 1.16, '#0d0e0f');
		for (const r of [1.1, 1.02, 0.94]) s += ring(V, 0, 0, r, '#4a4d52', 1, 'opacity=".8"');
		s += circ(V, 0, 0, 0.8, '#060607');
	}
	return s;
}

// =========================================================================================================== Magazine
// Tube6Window (gun space): a graphite tube on an aluminium seat ring, a graduated scale where it shows behind the pump,
// a long smoked window (right side) with the red shells and brass heads behind it and index ticks (the top one cyan),
// an aluminium transit clamp (a lower ring, a waisted web, a saddle round the barrel's underside) with a knurled brass
// thumbscrew (left), a knurled brass cap with a cyan centre.
const MV = -2.5;
let mN = 0;
const CLAMP = [[52.45, -3.25], cb([52.45, -3.62], [52.7, -3.82], [53.1, -3.82]), [53.9, -3.82], cb([54.3, -3.82], [54.55, -3.62], [54.55, -3.25]), [54.55, -1.8],
	cb([54.55, -1.25], [54.15, -1.0], [54.15, -0.45]), [54.15, -0.05], cb([54.15, 0.3], [54.5, 0.42], [54.5, 0.72]), [54.5, 0.92], [52.5, 0.92], [52.5, 0.72],
	cb([52.5, 0.42], [52.85, 0.3], [52.85, -0.05]), [52.85, -0.45], cb([52.85, -1.0], [52.45, -1.25], [52.45, -1.8])];
function magSide(V) {
	const right = V.kind === 'right';
	// the brass cap (under the clamp's front edge)
	const cap = pth(V, [[54.0, MV - 1.2], [55.6, MV - 1.2], [56.0, MV - 0.9], [56.0, MV + 0.9], [55.6, MV + 1.2], [54.0, MV + 1.2]], 0.06);
	let s = ao(shp(cap, 'url(#brassCyl)', 'bM'));
	for (let a = 54.6; a < 55.5; a += 0.12) s += pl(V, [[a, MV - 1.12], [a, MV + 1.12]], '#7a5718', 0.9, 'opacity=".6"');
	// the tube
	const t = pth(V, R4(20.4, MV - 1.05, 54.1, MV + 1.05), 0.04);
	s += ao(shp(t, 'url(#gphCyl)', 'mM'));
	s += ao(shp(pth(V, R4(20.0, MV - 1.08, 20.6, MV + 1.08), 0.05), 'url(#aluCyl)', 'mS'));
	s += ticks(V, 21.0, 26.6, MV + 0.95, 0.4, 0.22, '#c9ced4', 5, 1);
	s += pl(V, [[20.8, MV + 0.95], [26.8, MV + 0.95]], '#8d929a', 0.8, 'opacity=".6"');
	if (right) {   // the smoked window, the shells behind it, its index ticks
		const win = pth(V, R4(40, MV - 0.55, 50, MV + 0.55), 0.45);
		let sh = shp(win, '#120d0c', null);
		for (const a of [38.6, 45.3]) {
			sh += shp(pth(V, R4(a, MV - 0.88, a + 1.1, MV + 0.88), 0.06), 'url(#brassCyl)', null);
			sh += shp(pth(V, R4(a + 1.0, MV - 0.85, a + 6.5, MV + 0.85), 0.1), 'url(#shellCyl)', null);
			sh += pl(V, [[a + 1.6, MV - 0.85], [a + 1.6, MV + 0.85]], '#5a1a10', 1, 'opacity=".5"');
		}
		sh += `${full} fill="url(#s4smoke)" opacity=".42"/>` + pl(V, [[40.5, MV + 0.3], [49.5, MV + 0.38]], '#ffffff', 0.08 * V.s, 'opacity=".25"');
		s += clip(win, sh);
		s += shp(win, 'none', null, `stroke="#c9ced4" stroke-width="${r2(Math.max(1.4, 0.1 * V.s))}"`) + shp(win, 'none', null, 'stroke="#141518" stroke-width="1"');
		for (let k = 0; k < 6; k++) { const a = 40.6 + k * 1.75; s += pl(V, [[a, MV + 0.72], [a, MV + 0.95]], k === 5 ? '#5fd6f5' : '#e6e8ea', 1.4); }
		s += pl(V, [[50.5, MV], [52.3, MV]], '#14181c', 0.2 * V.s) + pl(V, [[50.5, MV], [52.3, MV]], '#5fd6f5', 0.09 * V.s);
	} else {
		s += pl(V, [[47.8, MV], [52.3, MV]], '#14181c', 0.2 * V.s) + pl(V, [[47.8, MV], [52.3, MV]], '#5fd6f5', 0.09 * V.s) + pl(V, [[47.8, MV], [52.3, MV]], '#e8fcff', 0.035 * V.s, 'opacity=".8"');
		for (let k = 0; k < 4; k++) s += pl(V, [[48.2 + k * 1.1, MV - 0.25], [48.2 + k * 1.1, MV - 0.55]], '#c9ced4', 1);
	}
	s += clip(t, pl(V, [[20.5, MV + 0.5], [54, MV + 0.5]], '#ffffff', 0.12 * V.s, 'opacity=".14"'));
	// the transit clamp
	const cl = cd(V, CLAMP);
	s += ao(shp(cl, 'url(#alu)', 'mM')) + clip(cl, `${full} fill="url(#brushV)"/>`);
	s += shp(cl, 'none', null, 'stroke="#5c6167" stroke-width="1" opacity=".6"');
	s += pl(V, [[52.9, 0.2], [54.1, 0.2]], '#3b3e43', 1.1, 'opacity=".7"');                      // the saddle's split
	if (!right) s += ao(circ(V, 53.5, -0.95, 0.42, 'url(#rBrass)', 'bS')) + knurlRing(V, 53.5, -0.95, 0.42, 24, '#7a5718') + circ(V, 53.5, -0.95, 0.12, '#5fd6f5');
	else s += screw(V, 53.5, -0.95, 0.2);
	s += screw(V, 53.5, MV - 0.9, 0.13);
	return s + wear(t, '#9aa0a8', 1, 0.35, '3 30 2 50');
}
function magEnd(V, back) {
	// the clamp end on: a lower block round the tube, two arms hugging the barrel (its circle masked out)
	const clampD = pth(V, R4(-1.1, -3.82, 1.1, -0.9), 0.3) + ' ' + pth(V, R4(-1.32, -1.4, 1.32, 0.92), 0.28);
	const mid = 'm4_' + (mN++);
	let s = '';
	const clampS = () => `<mask id="${mid}"><rect x="0" y="0" width="${N}" height="${N}" fill="#fff"/><path d="${L.discD(V, 0, 0, 1.15)}" fill="#000"/></mask>` +
		`<g mask="url(#${mid})">${ao(shp(clampD, back ? 'url(#gphLo)' : 'url(#alu)', 'mM')) + (back ? '' : clip(clampD, `${full} fill="url(#brushV)"/>`))}` +
		`<path d="${L.discD(V, 0, 0, 1.19)}" fill="none" stroke="#3b3e43" stroke-width="1.2"/></g>` +
		ao(shp(pth(V, R4(1.32, -1.37, 1.62, -0.53), 0.12), 'url(#brassCylH)', 'bS'));   // thumbscrew, the gun's left
	if (!back) {
		s += clampS();
		s += ao(circ(V, 0, MV, 1.2, 'url(#rBrass)', 'bM')) + knurlRing(V, 0, MV, 1.2, 40, '#7a5718');
		s += circ(V, 0, MV, 0.9, 'url(#rBrass)', 'bS') + ring(V, 0, MV, 0.9, '#6a4813', 1);
		s += circ(V, 0, MV, 0.28, '#0f1214') + circ(V, 0, MV, 0.2, 'url(#cyan)');
	} else {
		s += clampS();
		s += circ(V, 0, MV, 1.2, 'url(#rBrass)', 'bS');
		s += ao(circ(V, 0, MV, 1.08, 'url(#rAlu)', 'mM')) + circ(V, 0, MV, 0.98, '#141518');
		s += circ(V, 0, MV, 0.88, 'url(#rBrass)', 'bS') + ring(V, 0, MV, 0.72, '#6a4813', 1) + circ(V, 0, MV, 0.22, 'url(#rSteel)');
	}
	return s;
}

// ============================================================================================================== Sight
// Variable 1-4x as a surveyor's transit telescope: a satin-white tube, an aluminium eyepiece with a graduated power ring
// and a rubber eyecup (SOCKET_Aim (2.0, 6.75)), an objective bell with a cyan ring; held in split rings on two machined
// aluminium standards with lightening holes; a graduated elevation turret (brass cap); a windage knob (right); a level
// vial along the tube (left).
const H = 6.75;
function stand(V, uc) {
	const d = pth(V, [[uc - 1.1, 3.15], [uc + 1.1, 3.15], [uc + 1.1, 3.7], [uc + 0.62, 4.1], [uc + 0.58, 5.4], [uc - 0.58, 5.4], [uc - 0.62, 4.1], [uc - 1.1, 3.7]], 0.15);
	let s = ao(shpE(d + ' ' + pth(V, R4(uc - 0.22, 4.25, uc + 0.22, 5.0), 0.22), 'url(#alu)', 'mS')) + clip(d, `${full} fill="url(#brushV)"/>`);
	return s + pl(V, [[uc - 1.0, 3.62], [uc + 1.0, 3.62]], '#5c6167', 1, 'opacity=".7"');
}
function ringS(V, uc) {
	const d = pth(V, R4(uc - 0.6, H - 1.48, uc + 0.6, H + 1.48), 0.12);
	let s = ao(shp(d, 'url(#aluCyl)', 'mM')) + pl(V, [[uc - 0.6, H], [uc + 0.6, H]], '#3b3e43', 1.2);
	return s + screw(V, uc, H + 0.75, 0.15) + screw(V, uc, H - 0.75, 0.15);
}
function scopeSide(V) {
	const right = V.kind === 'right';
	let s = stand(V, 6.0) + stand(V, 14.5);
	// tube pieces
	s += ao(shp(pth(V, [[5.0, H - 1.8], [5.5, H - 1.3], [15.3, H - 1.3], [15.9, H - 1.75], [18.6, H - 1.75], [18.6, H + 1.75], [15.9, H + 1.75], [15.3, H + 1.3], [5.5, H + 1.3], [5.0, H + 1.8]], 0.05), 'url(#s4whtCyl)', 'pM'));
	s += seam(V, [[15.3, H - 1.28], [15.3, H + 1.28]], 0) + seam(V, [[5.5, H - 1.28], [5.5, H + 1.28]], 0);
	s += ao(shp(pth(V, R4(1.8, H - 1.8, 5.05, H + 1.8), 0.08), 'url(#aluCyl)', 'mM'));
	const pr = pth(V, R4(2.9, H - 1.82, 4.5, H + 1.82), 0.1);                                    // power ring
	s += ao(shp(pr, 'url(#gphCyl)', 'mS'));
	for (let a = 3.0; a < 4.45; a += 0.11) s += pl(V, [[a, H - 1.7], [a, H - 0.7]], '#0b0c0e', 0.9, 'opacity=".7"');
	for (let k = 0; k < 4; k++) s += pl(V, [[3.1 + k * 0.42, H + 0.9], [3.1 + k * 0.42, H + 1.35]], k === 1 ? '#5fd6f5' : '#e6e8ea', 1.4);
	s += ao(shp(pth(V, R4(1.62, H - 1.86, 2.25, H + 1.86), 0.22), 'url(#rubber)', 'rM'));          // eyecup
	s += ao(shp(pth(V, R4(18.45, H - 1.76, 18.85, H + 1.76), 0.06), 'url(#aluCyl)', 'mS'));
	s += pl(V, [[18.65, H - 1.74], [18.65, H + 1.74]], '#14181c', 0.14 * V.s) + pl(V, [[18.65, H - 1.72], [18.65, H + 1.72]], '#5fd6f5', 0.07 * V.s);
	s += ao(shp(pth(V, R4(15.9, H - 1.76, 16.12, H + 1.76), 0.04), 'url(#brassCyl)', 'bS'));
	s += clip(pth(V, R4(5.5, H - 1.3, 15.3, H + 1.3), 0), flt('soft', pl(V, [[5.6, H + 0.75], [15.2, H + 0.75]], '#ffffff', 0.16 * V.s, 'opacity=".6"')));
	s += ringS(V, 6.0) + ringS(V, 14.5);
	// the elevation turret on a saddle
	s += ao(shp(pth(V, [[9.0, H + 1.2], [11.5, H + 1.2], [11.2, H + 1.62], [9.3, H + 1.62]], 0.1), 'url(#alu)', 'mS'));
	const tur = pth(V, R4(9.4, H + 1.58, 11.1, 9.15), 0.1);
	s += ao(shp(tur, 'url(#aluCylH)', 'mS'));
	for (let a = 9.5; a < 11.05; a += 0.12) s += pl(V, [[a, H + 1.62], [a, H + 1.95]], '#6a6f76', 0.8, 'opacity=".7"');
	s += ticks(V, 9.6, 10.9, 8.95, 0.13, 0.22, '#3a3d42', 5, 0.9);
	s += ao(shp(pth(V, R4(9.55, 9.12, 10.95, 9.35), 0.08), 'url(#brassCylH)', 'bS'));
	if (right) {   // windage knob
		s += ao(circ(V, 10.25, H, 0.85, 'url(#rAlu)', 'mM')) + knurlRing(V, 10.25, H, 0.85, 40, '#6a6f76');
		s += circ(V, 10.25, H, 0.5, 'url(#rAlu)', 'mS') + circ(V, 10.25, H, 0.2, 'url(#rBrass)', 'bS');
		for (let k = 0; k < 12; k++) { const t = k * Math.PI / 6; s += pl(V, [[10.25 + 0.38 * Math.cos(t), H + 0.38 * Math.sin(t)], [10.25 + 0.48 * Math.cos(t), H + 0.48 * Math.sin(t)]], k === 3 ? '#2aa8cf' : '#4a4e55', 1); }
		for (const uc of [6.0, 14.5]) s += ao(circ(V, uc, 3.45, 0.3, 'url(#rBrass)', 'bS')) + knurlRing(V, uc, 3.45, 0.3, 16, '#7a5718');
	} else {       // the level vial along the tube
		const vh = pth(V, R4(7.6, H - 0.38, 12.6, H + 0.38), 0.36), vg = pth(V, R4(7.95, H - 0.22, 12.25, H + 0.22), 0.2);
		s += ao(shp(vh, 'url(#alu)', 'mS')) + shp(vg, 'url(#s4vial)', null) + shp(vg, 'none', null, 'stroke="#1b4e5c" stroke-width="1.1"');
		s += `<ellipse cx="${r2(V.X(10.1))}" cy="${r2(V.Y(H + 0.02))}" rx="${r2(0.55 * V.s)}" ry="${r2(0.12 * V.s)}" fill="#ffffff" opacity=".9"/>`;
		for (const a of [9.35, 10.85]) s += pl(V, [[a, H - 0.2], [a, H + 0.2]], '#123a46', 1.2);
		s += screw(V, 7.85, H, 0.1) + screw(V, 12.35, H, 0.1);
		for (const uc of [6.0, 14.5]) s += screw(V, uc, 3.45, 0.14);
	}
	return s + wear(pth(V, R4(5.5, H - 1.3, 15.3, H + 1.3), 0.05), '#b9bec5', 1, 0.3, '3 34 2 60');
}
function scopeEnd(V, back) {
	let s = '';
	// the standards seen end on, the turret, the windage knob (gun's right, -w) and the vial (gun's left, +w)
	for (let k = 0; k < 1; k++) {
		const d = pth(V, [[-1.1, 3.15], [1.1, 3.15], [1.1, 3.7], [0.7, 4.1], [0.7, 5.4], [-0.7, 5.4], [-0.7, 4.1], [-1.1, 3.7]], 0.15);
		s += ao(shp(d, 'url(#alu)', 'mS')) + clip(d, `${full} fill="url(#brushV)"/>`);
	}
	s += ao(shp(pth(V, R4(-1.45, 3.25, -1.1, 3.65), 0.08), 'url(#brassCylH)', 'bS'));
	s += ao(shp(pth(V, R4(-0.85, H + 1.58, 0.85, 9.15), 0.1), 'url(#aluCylH)', 'mS')) + ao(shp(pth(V, R4(-0.7, 9.12, 0.7, 9.35), 0.08), 'url(#brassCylH)', 'bS'));
	s += ao(shp(pth(V, R4(-2.6, H - 0.85, -1.2, H + 0.85), 0.15), 'url(#aluCylH)', 'mS'));
	for (let a = -2.5; a < -1.3; a += 0.12) s += pl(V, [[a, H - 0.78], [a, H + 0.78]], '#6a6f76', 0.8, 'opacity=".55"');
	s += ao(shp(pth(V, R4(1.2, H - 0.38, 1.88, H + 0.38), 0.15), 'url(#alu)', 'mS')) + shp(pth(V, R4(1.3, H - 0.2, 1.85, H + 0.2), 0.12), 'url(#s4vial)', null);
	s += ao(circ(V, 0, H, 1.48, 'url(#rAlu)', 'mM'));
	if (!back) {
		s += ao(circ(V, 0, H, 1.76, 'url(#rAlu)', 'mM')) + ring(V, 0, H, 1.65, '#14181c', 3) + ring(V, 0, H, 1.65, '#5fd6f5', 1.6);
		s += circ(V, 0, H, 1.52, 'url(#rLens)') + circ(V, 0, H, 1.52, 'url(#rCoat)');
		s += clip(L.discD(V, 0, H, 1.52),
			pl(V, [[-1.3, H + 0.5], [0.0, H + 1.6]], '#ffffff', 0.18 * V.s, 'opacity=".3"'));
		s += ring(V, 0, H, 1.52, '#141518', 1.6);
	} else {
		s += ao(circ(V, 0, H, 1.86, 'url(#rRubber)', 'rM')) + ring(V, 0, H, 1.6, '#0c0c0c', 1.6);
		s += circ(V, 0, H, 1.45, 'url(#aluCyl)', 'mS') + circ(V, 0, H, 1.3, 'url(#rLens)');
		s += pl(V, [[-1.25, H], [-0.12, H]], '#0a0b0c', 1.2) + pl(V, [[0.12, H], [1.25, H]], '#0a0b0c', 1.2) + pl(V, [[0, H - 1.25], [0, H - 0.12]], '#0a0b0c', 1.2) + pl(V, [[0, H + 0.12], [0, H + 1.25]], '#0a0b0c', 1.2);
		s += circ(V, 0, H, 0.05, '#ff3b2e');
		s += clip(L.discD(V, 0, H, 1.3),
			pl(V, [[-1.1, H + 0.4], [-0.1, H + 1.3]], '#ffffff', 0.14 * V.s, 'opacity=".25"'));
		s += ring(V, 0, H, 1.3, '#141518', 1.4);
	}
	return s;
}

// ============================================================================================================== Stock
// Thumbhole (gun space, u <= 0): one sculpted two-tone piece, satin white over a graphite grip and belly split by a
// sweeping parting line with the cyan accent; a bean-shaped thumbhole; a stippled grip front with an aluminium cap;
// an adjustable cheek riser on graduated aluminium posts; a dial medallion (left) or a brass lock lever (right); an
// aluminium butt plate with a spacer and a curved rubber pad; a QD cup.
const STK = [[0.4, 1.75], [0.4, -3.0], cb([0.4, -4.2], [-0.4, -5.0], [-0.6, -6.2]), [-0.75, -12.9], cb([-0.8, -14.1], [-1.6, -14.8], [-2.8, -14.8]),
	[-5.4, -14.75], cb([-7.0, -14.7], [-8.2, -13.9], [-9.4, -12.9]), cb([-14, -11.0], [-21, -10.9], [-27, -11.6]), cb([-31.5, -12.2], [-35, -12.9], [-38.4, -13.0]),
	[-38.4, 0.15], cb([-37, 0.3], [-35.5, 0.32], [-34, 0.28]), [-31.6, 0.25], [-15.2, 0.4], cb([-13.6, 0.5], [-12.6, 1.1], [-11.4, 1.35]), cb([-8, 1.75], [-4, 1.75], [-1.5, 1.75])];
const HOLE = [[-3.8, -3.8], cb([-7, -3.65], [-10.6, -3.9], [-12.2, -5.2]), cb([-13.4, -6.4], [-13.0, -9.0], [-11.4, -10.2]), cb([-9.6, -11.1], [-6.4, -11.0], [-5.0, -9.8]),
	cb([-4.0, -9.0], [-3.4, -7.0], [-3.5, -5.4]), cb([-3.55, -4.5], [-3.6, -3.85], [-3.8, -3.8])];
const PARTS_ = [[1, -2.3], cb([-6, -2.4], [-16, -3.4], [-26, -4.6]), cb([-32, -5.3], [-36, -5.6], [-40, -5.7])];
const RISER = [[-31.4, 0.5], [-15.6, 0.5], cb([-15.2, 0.5], [-15.1, 0.8], [-15.1, 1.2]), cb([-15.2, 1.65], [-15.6, 1.8], [-16.2, 1.8]), [-30.4, 1.75],
	cb([-31.2, 1.75], [-31.5, 1.45], [-31.5, 1.15]), cb([-31.5, 0.7], [-31.45, 0.5], [-31.4, 0.5])];
const PADS = [[-38.85, 0.2], cb([-39.5, 0.2], [-40.2, -0.3], [-40.2, -1.2]), cb([-40.0, -5], [-40.0, -9], [-40.2, -12.0]), cb([-40.2, -12.7], [-39.7, -13.05], [-38.85, -13.05])];
function stockSide(V) {
	const right = V.kind === 'right';
	const body = cd(V, STK), hole = cd(V, HOLE), both = body + ' ' + hole;
	let s = '';
	const pad = cd(V, PADS);
	s += shp(pad, 'url(#rubber)', 'rM') + shp(pad, 'url(#stip4)', null, 'opacity=".3"');
	for (let v = -11.8; v < 0; v += 1.6) s += clip(pad, pl(V, [[-40.4, v], [-39.2, v]], '#0e0e10', 1.3));
	s += ao(shp(pth(V, R4(-38.95, -13.0, -38.35, 0.18), 0.08), 'url(#alu)', 'mS')) + pl(V, [[-38.65, -12.9], [-38.65, 0.1]], '#4e5359', 1);
	// riser posts with height marks, the riser
	for (const a of [-28.5, -18.5]) {
		s += ao(shp(pth(V, R4(a - 0.28, 0.1, a + 0.28, 0.85), 0.05), 'url(#aluCylH)', 'mS'));
		for (const v of [0.35, 0.55]) s += pl(V, [[a - 0.24, v], [a + 0.24, v]], '#4a4e55', 0.9);
	}
	const rs = cd(V, RISER);
	s += shp(rs, 'url(#wht)', 'pM') + clip(rs, flt('soft', sk(V, [[-30.6, 1.5], [-16.2, 1.55]], '#ffffff', 0.2, 'opacity=".8"')));
	s += clip(rs, pl(V, [[-31.5, 0.95], [-15.1, 0.95]], '#9a988f', 1, 'opacity=".6"'));
	// the body: white, graphite below the parting line, through the thumbhole
	s += `<path d="${both}" fill="url(#wht)" fill-rule="evenodd" filter="url(#pL)"/>`;
	s += flt('pM', clip(body, `<path d="${cd(V, [...PARTS_, [-41, -20], [2, -20]])} ${hole}" fill="url(#gph)" fill-rule="evenodd"/>`));
	s += clip(body, flt('soft', sk(V, [[-37.6, -0.4], [-31.8, -0.2], [-15.0, -0.05], cb([-12, 0.5], [-8, 1.3], [-1, 1.3])], '#ffffff', 0.35, 'opacity=".7"')) +
		flt('soft2', sk(V, shift(PARTS_, 0, 0.7), '#2a3040', 1.2, 'opacity=".14"')) +
		flt('soft', sk(V, [[-12, -11.3], cb([-20, -10.6], [-30, -11.4], [-38, -12.3])], '#8d929a', 0.25, 'opacity=".3"')) +
		accent(V, PARTS_, 0.13));
	s += `<path d="${hole}" fill="none" stroke="#0f1012" stroke-width="${r2(Math.max(1.2, 0.08 * V.s))}" opacity=".7"/>`;
	// grip: stippled front, an aluminium cap
	const gp = pth(V, [[-1.05, -6.4], [-3.05, -6.2], [-3.3, -9.0], [-3.2, -12.4], [-1.35, -13.4]], 0.5);
	s += shp(gp, 'url(#stip4)', null) + shp(gp, 'none', null, 'stroke="#1b1d20" stroke-width="1.3"');
	s += flt('mS', clip(body, shp(pth(V, R4(-9, -15.2, 1, -14.3), 0), 'url(#alu)'))) + clip(body, pl(V, [[-9, -14.3], [1, -14.3]], '#1d1f22', 1.3));
	// the seat plate on the front face
	s += ao(shp(pth(V, R4(0.1, -3.0, 0.45, 1.72), 0.06), 'url(#alu)', 'mS'));
	if (!right) {   // a dial medallion: a ring of graduations round a brass centre
		s += ao(circ(V, -23.5, -1.2, 1.0, 'url(#rAlu)', 'mS')) + ring(V, -23.5, -1.2, 0.85, '#3b3e43', 1);
		for (let k = 0; k < 24; k++) { const t = k * Math.PI / 12, r0 = k % 6 ? 0.72 : 0.6; s += pl(V, [[-23.5 + r0 * Math.cos(t), -1.2 + r0 * Math.sin(t)], [-23.5 + 0.84 * Math.cos(t), -1.2 + 0.84 * Math.sin(t)]], k === 6 ? '#2aa8cf' : '#4a4e55', 1); }
		s += circ(V, -23.5, -1.2, 0.22, 'url(#rBrass)', 'bS');
	} else {        // the riser's brass lock lever
		s += ao(circ(V, -23.5, -0.6, 0.55, 'url(#rAlu)', 'mS')) + ao(sk(V, [[-23.5, -0.6], [-21.6, -1.1]], '#2a2c30', 0.36) + sk(V, [[-23.5, -0.6], [-21.6, -1.1]], 'url(#brass)', 0.24));
		s += circ(V, -23.5, -0.6, 0.18, 'url(#rBrass)', 'bS');
	}
	s += qd(V, -31.0, -11.25, 0.42) + pin(V, -2.2, -3.4, 0.15);
	return s + wear(body, '#a7adb5', 1, 0.35, '3 26 2 44');
}
function stockEnd(V, back) {
	let s = '';
	const padD = pth(V, R4(-2.2, -13.05, 2.2, 0.2), 0.9);
	const gripD = pth(V, R4(-1.55, -14.8, 1.55, -3.0), 0.7);
	const riser = pth(V, R4(-1.25, 0.5, 1.25, 1.8), 0.45);
	const butt = pth(V, R4(-2.0, -12.9, 2.0, 0.35), 0.8);
	const lever = () => ao(shp(pth(V, R4(-2.3, -0.85, -1.95, -0.35), 0.1), 'url(#brassCylH)', 'bS'));
	if (!back) {
		s += shp(padD, 'url(#rRubber)', 'rM');
		s += xsec(V, butt, 'url(#whtLo)', 'url(#gphLo)', -5.6, 'pL');
		s += lever();
		for (const sd of [-1, 1]) s += ao(shp(pth(V, R4(sd * 0.75 - 0.25, 0.1, sd * 0.75 + 0.25, 0.85), 0.05), 'url(#aluCylH)', 'mS'));
		s += shp(riser, 'url(#whtMid)', 'pM');
		s += xsec(V, pth(V, R4(-1.6, -3.0, 1.6, 1.75), 0.6), 'url(#whtMid)', 'url(#gph)', -2.3, 'pL');
		s += flt('pL', shp(gripD, 'url(#gph)'));
		s += shp(pth(V, R4(-1.5, -14.8, 1.5, -14.3), 0.3), 'url(#alu)', 'mS');
		// the seat plate: guide slots, two screws, the locating bolt at (0, 0)
		const sp = pth(V, R4(-1.5, -3.0, 1.5, 1.7), 0.35);
		s += ao(shp(sp, 'url(#alu)', 'mM')) + clip(sp, `${full} fill="url(#brushV)"/>`);
		for (const sd of [-1, 1]) s += shp(pth(V, R4(sd * 1.0 - 0.12, -2.5, sd * 1.0 + 0.12, 1.2), 0.06), '#3a3e44', null);
		s += screw(V, 0, -2.2, 0.24) + screw(V, 0, 1.0, 0.24);
		s += ao(circ(V, 0, -0.45, 0.48, 'url(#rSteel)', 'mS')) + circ(V, 0, -0.45, 0.22, '#2a2c30');
	} else {
		s += flt('pL', shp(gripD, 'url(#gphLo)'));
		s += xsec(V, pth(V, R4(-1.6, -3.0, 1.6, 1.75), 0.6), 'url(#whtLo)', 'url(#gphLo)', -2.3, 'pL');
		s += shp(riser, 'url(#whtMid)', 'pM') + clip(riser, flt('soft', sk(V, [[-0.9, 1.55], [0.9, 1.55]], '#ffffff', 0.15, 'opacity=".8"')));
		s += lever();
		s += ao(shp(pth(V, R4(-2.12, -13.0, 2.12, 0.18), 0.85), 'url(#alu)', 'mS'));
		s += shp(padD, 'url(#rRubber)', 'rM') + shp(padD, 'url(#stip4)', null, 'opacity=".35"');
		s += clip(padD, Array.from({ length: 16 }, (_, k) => { const v = -12.4 + k * 0.8; return pl(V, [[-2.2, v], [2.2, v]], '#101012', 1.4) + pl(V, [[-2.2, v - 0.08], [2.2, v - 0.08]], '#5c5c60', 0.7, 'opacity=".5"'); }).join(''));
		for (const v of [-10.0, -2.2]) s += ao(circ(V, 0, v, 0.32, 'url(#rAlu)', 'mS')) + circ(V, 0, v, 0.12, '#2a2c30');
		s += wear(padD, '#7a7d82', 1.2, 0.5, '4 24 2 40');
	}
	return s;
}

module.exports = [
	{ dir: 'Body_Tactical', b: { u: [-0.6, 20.05], v: [-8.5, 3.15], w: [-1.75, 1.75] }, draw: { left: bodySide, right: bodySide, front: V => bodyEnd(V, false), back: V => bodyEnd(V, true) } },
	{ dir: 'Barrel_Slug', b: { u: [0, 46], v: [-1.45, 3.05], w: [-1.45, 1.45] }, draw: { left: brlSide, right: brlSide, front: V => brlEnd(V, false), back: V => brlEnd(V, true) } },
	{ dir: 'Muzzle_Choke', b: { u: [0, 4.6], v: [-1.4, 1.4], w: [-1.4, 1.4] }, draw: { left: chokeSide, right: chokeSide, front: V => chokeEnd(V, false), back: V => chokeEnd(V, true) } },
	{ dir: 'Magazine_Tube6Window', b: { u: [20, 56], v: [-3.85, 0.95], w: [-1.35, 1.65] }, draw: { left: magSide, right: magSide, front: V => magEnd(V, false), back: V => magEnd(V, true) } },
	{ dir: 'Sight_Variable', b: { u: [1.6, 18.9], v: [3.15, 9.4], w: [-2.65, 1.9] }, draw: { left: scopeSide, right: scopeSide, front: V => scopeEnd(V, false), back: V => scopeEnd(V, true) } },
	{ dir: 'Stock_Thumbhole', b: { u: [-40.25, 0.45], v: [-14.85, 1.85], w: [-2.3, 2.25] }, draw: { left: stockSide, right: stockSide, front: V => stockEnd(V, false), back: V => stockEnd(V, true) } },
];
