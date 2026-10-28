// Set 3: Windfall (scrap-built, rattly) concept views of the Ranchhand's parts, for Tripo (2026-10-28).
// Six new designs, not another set recoloured: a welded square-tube receiver with a punched tin front box and its rail
// on three strap risers; a sawn-off barrel under a perforated tin heat shield held by hose clamps; a pipe end hammered
// into a duckbill; a tin-can box magazine; a washer ghost ring between ragged tin wings with a nail front post; a folding
// pipe-frame stock on a door hinge with a tyre-tread butt. Only the interfaces are Ranchhand.py's (the shared contract).
// Materials: rusted steel, galvanized tin, sea-teal and sky-blue paint over rust, black friction tape, tan tape, brass
// wire, hose clamps, welds, a little ember. Rendered by render.js (`node Art/Concepts/Weapons/Shotgun/render.js set3`).
const {
	N, pth, R4, shp, circ, ring, pl, wear, bez, edgeU, ao, screw, RAIL_X, inside, pipe, pipeD, rod, weld, rustOn, chipsOn,
	drip, wireWrap, tapeBands, hoseClamp, zipTie, coil, popRivet, hexHead, discD, ZINC, rng,
} = require('./lib');

const FULL = `<rect x="0" y="0" width="${N}" height="${N}"`;
const CLOTH = `${FULL} fill="url(#cloth)"/>`;
const plate = (V, pts, grad, r = 0.12, filt = 'mM') => ao(shp(pth(V, pts, r), `url(#${grad})`, filt));
const zinc = (V, pts, r = 0.12) => plate(V, pts, 'galv', r) + inside(pth(V, pts, r), ZINC);
const rusty = (V, pts, seed, n, grad = 'rust3', r = 0.12) => {
	const xs = pts.map(p => p[0]), vs = pts.map(p => p[1]);
	return plate(V, pts, grad, r) + rustOn(V, pth(V, pts, r), [Math.min(...xs), Math.max(...xs), Math.min(...vs), Math.max(...vs)], seed, n);
};
const painted = (V, pts, grad, seed, n, r = 0.12) => {
	const xs = pts.map(p => p[0]), vs = pts.map(p => p[1]);
	return plate(V, pts, grad, r, 'pM') + chipsOn(V, pth(V, pts, r), [Math.min(...xs), Math.max(...xs), Math.min(...vs), Math.max(...vs)], seed, n);
};
const tapeBox = (V, pts, grad, pitch, lean) => {
	const xs = pts.map(p => p[0]), vs = pts.map(p => p[1]), d = pth(V, pts, 0.08);
	return ao(shp(d, `url(#${grad})`, 'pS')) + tapeBands(V, d, [Math.min(...xs) - 1, Math.max(...xs) + 1, Math.min(...vs), Math.max(...vs)], pitch, lean,
		grad === 'tanTape' ? '#5a4a36' : '#0a0a0a', grad === 'tanTape' ? '#e0cba8' : '#77746e');
};
// A punched hole in tin with ember glowing behind it.
const emberHole = (V, a, v, r) => circ(V, a, v, r, '#160b05') + circ(V, a, v, r * 0.72, 'url(#ember)') + ring(V, a, v + r * 0.15, r, '#ffffff', 0.9, 'opacity=".45"');
// The picatinny rail on top of the body, base v 2.2..2.65, teeth to 3.15 (side view).
function railSide(V, u0, u1) {
	let s = ao(shp(pth(V, R4(u0, 2.2, u1, 2.65), 0.08), 'url(#black)', 'mS'));
	for (let u = u0 + 0.35; u + 0.55 <= u1 - 0.2; u += 1.0) s += ao(shp(pth(V, R4(u, 2.6, u + 0.53, 3.15), 0.07), 'url(#black)', 'mS'));
	return s + pl(V, [[u0 + 0.2, 2.6], [u1 - 0.2, 2.6]], '#a3a9b1', 0.9, 'opacity=".45"');
}
const RAIL_END = RAIL_X.map(([w, v]) => [w, v - 3.0]);

// ==== Body_Salvaged: a welded square-tube receiver, a punched tin front box, the rail on strap risers ===============
const CORE = [[0.45, -3.0], [13.4, -3.0], [13.4, 1.55], [12.7, 1.7], [1.3, 1.7], [0.45, 1.25]];
const BOX = [[13.0, -3.0], [19.9, -3.0], [20.0, -2.7], [20.0, 1.0], [19.6, 1.3], [13.15, 1.25]];
const BUTTP = [[-0.3, -3.4], [0.45, -3.4], [0.45, 2.0], [-0.3, 2.0]];
const TH5 = [[3.2, -3.0], [13.0, -3.0], [12.4, -4.6], [4.0, -4.4]];
const TRIG5 = [[7.4, -4.4], [8.3, -4.4], [8.15, -5.6], [7.8, -6.6], [7.2, -6.8], [7.5, -5.6]];
const GUARD5 = [[4.6, -4.4], [4.85, -6.9], [6.6, -7.75], [11.2, -7.35], [12.4, -4.6]];
const RISERS = [[1.7, 1.7], [9.3, 1.7], [17.0, 1.3]];
function bodySide(V) {
	const right = V.kind === 'right';
	let s = '';
	// the butt plate the stock seats on, the brass-wire sling loop under the back
	s += rod(V, [[0.9, -3.0], [0.95, -4.2], [1.6, -4.65], [2.35, -4.15], [2.35, -3.0]], 0.1, '#c99a4a', '#fff1c2');
	s += rusty(V, BUTTP, 3, 8, 'rust3', 0.15) + hexHead(V, 0.08, 1.35, 0.24, 'url(#rRust)') + hexHead(V, 0.08, -2.75, 0.24, 'url(#rRust)');
	// the core: a length of rusted square tube, welded to the plate
	s += rusty(V, CORE, 5, 22, 'rustDeep', 0.2) + weld(V, [[0.5, -2.9], [0.5, 1.2]]);
	if (!right) {   // left: a teal-painted plate riveted flat over the core (the element plate's flank), brass wire run
		const t = [[1.0, -2.55], [11.9, -2.75], [12.1, 1.25], [1.45, 1.35]];
		s += painted(V, t, 'tealPaint', 9, 24) + wear(pth(V, t, 0.12), '#d5e6e2', 1.2, 0.7);
		for (let a = 1.6; a < 11.8; a += 2.05) s += popRivet(V, a, 0.95, 0.17) + popRivet(V, a + 0.15, -2.2, 0.17);
		s += pl(V, [[3.0, -1.0], [9.8, -1.3]], '#2b5450', 1.3, 'opacity=".7"');                                  // a scratch
		s += drip(V, 5.7, -2.3, 0.5) + drip(V, 9.8, -2.35, 0.45);
	} else {        // right: the ejection port and its bolt, a sky-blue patch with a hazard tag at the back
		const sk = [[0.9, -2.6], [5.7, -2.5], [5.6, 1.35], [1.1, 1.3]];
		s += painted(V, sk, 'sky', 13, 16) + wear(pth(V, sk, 0.12), '#e3eef5', 1.2, 0.7);
		for (const [a, v] of [[1.5, 0.85], [5.1, 0.85], [1.5, -2.05], [5.1, -2.0]]) s += screw(V, a, v, 0.19);
		const tag = pth(V, R4(2.0, -1.35, 4.6, -0.35), 0.08);
		s += ao(shp(tag, 'url(#hazard)', 'pS')) + inside(tag, Array.from({ length: 8 }, (_, k) => shp(pth(V, [[1.6 + k * 0.45, -1.35], [1.82 + k * 0.45, -1.35], [2.2 + k * 0.45, -0.35], [1.98 + k * 0.45, -0.35]], 0), '#1c1c1c', null)).join(''));
		s += popRivet(V, 2.25, -0.85, 0.12) + popRivet(V, 4.35, -0.85, 0.12);
	}
	// lower edge: the rarity light, a salvaged glow strip taped on along both sides
	s += shp(pth(V, R4(1.1, -2.95, 12.9, -2.5), 0.2), '#1a1714', null) + shp(pth(V, R4(1.25, -2.84, 12.75, -2.6), 0.12), 'url(#accentLin)', null) + pl(V, [[1.4, -2.72], [12.6, -2.72]], '#ffffff', 1.2, 'opacity=".9"');
	for (const a of [3.4, 8.2, 11.9]) s += tapeBox(V, [[a, -3.02], [a + 0.75, -3.02], [a + 0.8, -2.42], [a + 0.05, -2.42]], 'tapeBlack', 0.2, 0.05);
	// the front box: a punched galvanized tin box lapped over the core, ember behind its holes
	const bd = pth(V, BOX, 0.12);
	s += plate(V, BOX, 'galv', 0.12) + inside(bd, ZINC);
	const rnd = rng(right ? 71 : 72);
	for (const [v, off] of [[0.55, 0], [-0.45, 0.55], [-1.45, 0], [-2.35, 0.55]])
		for (let a = 14.6 + off; a < 18.3; a += 1.1) s += emberHole(V, a, v, 0.24 + rnd() * 0.04);
	s += inside(bd, [[15.2, 0.0, 0.9], [18.4, -1.8, 0.7]].map(([a, v, r]) => `<ellipse cx="${V.X(a)}" cy="${V.Y(v)}" rx="${r * V.s}" ry="${r * 0.6 * V.s}" fill="#ffffff" opacity=".16"/>`).join(''));
	s += wear(bd, '#ffffff', 1.2, 0.6) + pl(V, [[13.15, -2.95], [13.25, 1.2]], '#5e646a', 1.6, 'opacity=".8"');   // the lapped edge
	s += ao(shp(pth(V, R4(13.5, -3.08, 13.95, 1.38), 0.05), 'url(#steelPlate)', 'mS')) + hexHead(V, 13.72, 0.9, 0.2) + hexHead(V, 13.72, -2.55, 0.2);
	if (right) {   // the port cut through core and box, the bolt, its brass-knobbed handle
		s += shp(pth(V, R4(6.0, -0.8, 13.6, 1.2), 0.12), '#120a06', null) + weld(V, [[6.0, 1.25], [13.6, 1.25]], 0.2, 0.09) + weld(V, [[6.0, -0.85], [13.6, -0.85]], 0.2, 0.09);
		s += pipe(V, [6.4, 0.2], [13.3, 0.2], 0.48, 'steel') + shp(pth(V, R4(9.0, 0.45, 10.4, 0.68), 0.05), '#1d1e20', null);
		s += coil(V, 6.5, 9.0, 0.2, 0.62, 5, 0.06);
		s += rod(V, [[12.7, 0.2], [12.9, 0.4]], 0.14, '#7d8083', '#d9dde1') + ao(circ(V, 12.9, 0.4, 0.38, 'url(#rBrass)', 'bS')) + circ(V, 12.78, 0.52, 0.11, '#fff6d8', null, 'opacity=".7"');
	}
	// trigger housing, trigger, a bent-rebar guard welded on
	s += rusty(V, TH5, 17, 10, 'rust3', 0.15) + weld(V, [[3.3, -3.05], [12.9, -3.05]], 0.22, 0.09);
	s += hexHead(V, 10.6, -3.7, 0.24);
	s += ao(shp(pth(V, TRIG5, 0.12), 'url(#steelPlate)', 'mS'));
	s += rod(V, GUARD5, 0.2, '#7d8083', '#d9dde1') + weld(V, [[4.4, -4.45], [4.85, -4.45]], 0.15, 0.1) + weld(V, [[12.2, -4.6], [12.7, -4.6]], 0.15, 0.1);
	// the front face: a pipe coupling for the barrel nut, a pipe stub seating the magazine tube
	s += pipe(V, [18.7, 0], [20.0, 0], 1.5, 'steel', 'mM') + rustOn(V, pipeD(V, [18.7, 0], [20.0, 0], 1.5), [18.7, 20, -1.5, 1.5], 23, 6) + weld(V, [[18.75, -1.4], [18.75, 1.4]], 0.16, 0.1);
	s += pipe(V, [19.1, -2.5], [20.0, -2.5], 1.15, 'galv') + weld(V, [[19.15, -3.5], [19.15, -2.9]], 0.16, 0.1);
	// three bent-strap risers and the rail on them, gaps under it
	for (const [a, v] of RISERS) {
		const rz = [[a, v - 0.1], [a + 1.2, v - 0.1], [a + 1.05, 2.25], [a + 0.15, 2.25]];
		s += rusty(V, rz, 21 + a, 3, 'rust3', 0.06) + weld(V, [[a - 0.05, v], [a + 1.25, v]], 0.15, 0.09);
	}
	s += railSide(V, 1.0, 19.5);
	if (!right) s += pl(V, [[0.6, 1.5], [6.0, 1.52], [12.6, 1.45]], '#3a2608', 2.4) + pl(V, [[0.6, 1.5], [6.0, 1.52], [12.6, 1.45]], '#d29d48', 1.5) + wireWrap(V, 6.0, 6.5, 1.5, 0.25, 0.12, 0.05);
	return s + wear(pth(V, CORE, 0.2), '#e0a27a', 1.3, 0.5, '6 30 3 50 12 40');
}
function bodyEnd(V, back) {
	let s = '';
	const knob = () => rod(V, [[-1.55, 0.2], [-2.05, 0.35]], 0.14, '#7d8083', '#d9dde1') + ao(circ(V, -2.1, 0.4, 0.36, 'url(#rBrass)', 'bS'));
	const rail = () => {
		let r = '';
		for (const sgn of [-1, 1]) r += ao(shp(pth(V, R4(-0.6, 1.6, 0.6, 2.25), 0.05), 'url(#rust3)', 'mS'));
		return r + ao(shp(pth(V, RAIL_END, 0.03), 'url(#black)', 'mS'));
	};
	const guard = () => ao(shp(pth(V, R4(-0.2, -7.95, 0.2, -4.4), 0.18), 'url(#steelCylH)', 'mS')) + ao(shp(pth(V, R4(-1.15, -4.6, 1.15, -3.0), 0.12), 'url(#rust3)', 'mS'));
	const sides = () => painted(V, R4(1.5, -2.75, 1.62, 1.35), 'tealPaint', 3, 2, 0.02) + painted(V, R4(-1.62, -2.6, -1.5, 1.35), 'sky', 4, 2, 0.02) +
		ao(shp(pth(V, R4(-1.6, -2.95, 1.6, -2.45), 0.1), '#1a1714', null)) + shp(pth(V, R4(-1.6, -2.85, -1.48, -2.6), 0.03), 'url(#accentLin)', null) + shp(pth(V, R4(1.48, -2.85, 1.6, -2.6), 0.03), 'url(#accentLin)', null);
	if (!back) {
		s += rod(V, [[-0.75, -3.0], [-0.75, -4.6], [0.75, -4.6], [0.75, -3.0]], 0.1, '#c99a4a', '#fff1c2');
		s += rusty(V, R4(-1.6, -3.4, 1.6, 2.0), 31, 6, 'rust3', 0.2);              // the butt plate, behind
		s += rusty(V, R4(-1.5, -3.0, 1.5, 1.7), 33, 8, 'rustDeep', 0.15) + sides();
		s += guard() + rail() + knob();
		// the tin box's front face, its lapped edges, the barrel coupling and the tube stub
		const bx = R4(-1.6, -3.0, 1.6, 1.3);
		s += zinc(V, bx, 0.15) + wear(pth(V, bx, 0.15), '#ffffff', 1.2, 0.6);
		for (const [a, v] of [[-1.2, 0.95], [1.2, 0.95]]) s += popRivet(V, a, v, 0.15);
		for (const sd of [-1, 1]) s += shp(pth(V, R4(sd * 1.2 - 0.12, -2.3, sd * 1.2 + 0.12, -1.8), 0.04), '#0d0a08', null);   // the action bars' slots
		s += ao(circ(V, 0, 0, 1.5, 'url(#rSteel)', 'mM')) + rustOn(V, discD(V, 0, 0, 1.5), [-1.5, 1.5, -1.5, 1.5], 37, 8) + ring(V, 0, 0, 1.5, '#5e646a', 1.6);
		s += circ(V, 0, 0, 1.2, '#161718') + ring(V, 0, 0, 1.15, '#8a9196', 1.4);
		for (const r of [1.0, 0.85]) s += ring(V, 0, 0, r, '#3a3c40', 1, 'opacity=".8"');
		const rn = rng(39);
		for (let k = 0; k < 20; k++) { const a = k / 20 * 6.283; s += circ(V, 1.62 * Math.cos(a), 1.62 * Math.sin(a), 0.1 + rn() * 0.04, '#4f463e') + circ(V, 1.62 * Math.cos(a), 1.62 * Math.sin(a) + 0.03, 0.05, '#b9afa2', null, 'opacity=".7"'); }
		s += ao(circ(V, 0, -2.5, 1.15, 'url(#rGalv)', 'mS')) + circ(V, 0, -2.5, 0.95, '#141516') + ring(V, 0, -2.5, 1.05, '#8a9196', 1);
	} else {
		s += rusty(V, R4(-1.5, -3.0, 1.5, 1.7), 34, 6, 'rustDeep', 0.15) + sides() + ao(circ(V, 0, -2.5, 1.15, 'url(#rGalv)', 'mS'));
		s += guard() + rail() + knob();
		// the butt plate's face (the stock seats here): guide strips, two bolts, the locating boss on the socket (0, 0)
		const bp = pth(V, R4(-1.6, -3.4, 1.6, 2.0), 0.2);
		s += ao(shp(bp, 'url(#rust3)', 'mM')) + rustOn(V, bp, [-1.6, 1.6, -3.4, 2.0], 41, 14);
		for (const sd of [-1, 1]) s += ao(shp(pth(V, R4(sd * 1.05 - 0.15, -3.0, sd * 1.05 + 0.15, 1.6), 0.06), 'url(#steelPlate)', 'mS'));
		s += hexHead(V, 0, 1.35, 0.3, 'url(#rRust)') + hexHead(V, 0, -2.75, 0.3, 'url(#rRust)') + drip(V, 0, 1.0, 0.8);
		s += ao(circ(V, 0, 0, 0.6, 'url(#rSteel)', 'mS')) + circ(V, 0, 0, 0.3, '#0f1012') + weld(V, [[-1.55, 1.95], [1.55, 1.95]], 0.22, 0.08);
		s += wear(bp, '#e0a27a', 1.2, 0.6) + rod(V, [[-0.75, -3.4], [-0.75, -4.6], [0.75, -4.6], [0.75, -3.4]], 0.1, '#c99a4a', '#fff1c2');
	}
	return s;
}

// ==== Barrel_Short: sawn off, a perforated tin heat shield on hose clamps (local u: 0 at the nut, tip 38) ===========
const SHIELD = [[1.6, 0.2], [15.4, 0.2], [15.4, 1.42], [15.05, 1.75], [2.0, 1.75], [1.75, 1.6], [1.95, 1.3], [1.6, 1.0]];   // hacksawed back edge
function brlSide(V) {
	const right = V.kind === 'right';
	let s = pipe(V, [0, 0], [38, 0], 1.15, 'steel', 'mM');
	const bd = pipeD(V, [0, 0], [38, 0], 1.15);
	s += rustOn(V, bd, [0, 38, -1.15, 1.15], 51, 30);
	for (let a = 36.6; a < 37.6; a += 0.22) s += pl(V, [[a, -0.9], [a + 0.18, -0.2]], '#e6eaee', 0.8, 'opacity=".5"');   // hacksaw slips
	s += shp(pth(V, R4(37.86, -1.15, 38, 1.15), 0.02), '#dfe3e6', null);                                                // the filed cut
	// the breech: a pipe coupling welded on
	s += pipe(V, [0, 0], [1.7, 0], 1.45, 'galv', 'mM') + inside(pipeD(V, [0, 0], [1.7, 0], 1.45), ZINC) + weld(V, [[1.75, -1.35], [1.75, 1.35]], 0.16, 0.1);
	// the shield: galvanized tin, hacksawed to a half pipe over the top, punched, ember-hot barrel showing through
	const sd = pth(V, SHIELD, 0.08);
	s += ao(shp(sd, 'url(#galv)', 'mM')) + inside(sd, ZINC);
	for (const [v, off] of [[1.3, 0], [0.7, 0.6]]) for (let a = 3.2 + off; a < 15.0; a += 1.2) if (Math.abs(a - 13.8) > 0.5 && Math.abs(a - 3.0) > 0.5) s += emberHole(V, a, v, 0.2);
	if (!right) { const p = [[4.2, 0.35], [7.4, 0.3], [7.5, 1.6], [4.3, 1.62]]; s += painted(V, p, 'tealPaint', 55, 8, 0.06) + popRivet(V, 4.55, 0.6, 0.12) + popRivet(V, 7.1, 0.6, 0.12) + popRivet(V, 4.55, 1.35, 0.12) + popRivet(V, 7.1, 1.35, 0.12); }
	else { const p = [[9.4, 0.3], [12.6, 0.35], [12.5, 1.62], [9.5, 1.6]]; s += painted(V, p, 'sky', 56, 8, 0.06) + popRivet(V, 9.8, 0.6, 0.12) + popRivet(V, 12.2, 0.6, 0.12); }
	s += wear(sd, '#ffffff', 1.2, 0.6) + pl(V, [[2.0, 1.72], [15.0, 1.72]], '#ffffff', 1, 'opacity=".55"');
	s += hoseClamp(V, 3.0, 0.3, 1.5, true) + hoseClamp(V, 13.8, 0.3, 1.5, false);
	// tan tape round the barrel with a lifted end, brass wire further on, a nut-and-nail front sight with a glass bead
	const tp = R4(18.0, -1.25, 20.6, 1.25);
	s += tapeBox(V, tp, 'tanTape', 0.4, 0.15) + shp(pth(V, [[20.5, 1.1], [21.4, 1.55], [21.1, 1.7], [20.4, 1.28]], 0.05), 'url(#tanTape)', 'pS');
	s += wireWrap(V, 26.0, 27.5, 0, 1.2, 0.14, 0.055);
	s += zipTie(V, 31.0, 0, 1.18, !right);
	s += ao(shp(pth(V, R4(35.0, 1.05, 36.4, 1.85), 0.08), 'url(#steelPlate)', 'mS')) + pl(V, [[35.25, 1.45], [36.15, 1.45]], '#4a4d52', 1, 'opacity=".7"');
	s += weld(V, [[34.9, 1.1], [36.5, 1.1]], 0.15, 0.09) + ao(shp(pth(V, R4(35.61, 1.8, 35.79, 2.1), 0.04), 'url(#steelCylH)', 'mS'));
	s += circ(V, 35.7, 2.22, 0.2, 'url(#cyan)') + circ(V, 35.64, 2.28, 0.07, '#ffffff', null, 'opacity=".8"');
	return s + wear(bd, '#d8a07a', 1, 0.45, '6 40 3 60');
}
function shieldArc(V) {   // the half-pipe shield end on (w, v): r 1.75 outside, 1.6 inside, above v 0.2
	const o = [], n = 18;
	const a0 = Math.asin(0.2 / 1.75), b0 = Math.asin(0.2 / 1.6);
	for (let k = 0; k <= n; k++) { const a = a0 + (Math.PI - 2 * a0) * k / n; o.push([1.75 * Math.cos(a), 1.75 * Math.sin(a)]); }
	for (let k = n; k >= 0; k--) { const a = b0 + (Math.PI - 2 * b0) * k / n; o.push([1.6 * Math.cos(a), 1.6 * Math.sin(a)]); }
	return o;
}
function brlEnd(V, back) {
	let s = '';
	s += ao(shp(pth(V, shieldArc(V), 0.02), 'url(#galv)', 'mM'));
	s += ao(shp(pth(V, R4(-0.45, 1.75, 0.45, 2.22), 0.08), 'url(#galv)', 'mS'));                       // the clamp's housing
	s += ring(V, 0, 0.3, 1.5, '#9aa1a7', 0.12 * V.s, 'opacity=".9"');                                   // the clamp band
	if (!back) {
		s += ao(circ(V, 0, 0, 1.45, 'url(#rGalv)', 'mS')) + ao(circ(V, 0, 0, 1.25, 'url(#rTan)', 'pS'));
		s += ao(shp(pth(V, R4(-0.55, 1.05, 0.55, 1.85), 0.08), 'url(#steelPlate)', 'mS')) + ao(shp(pth(V, R4(-0.09, 1.8, 0.09, 2.1), 0.04), 'url(#steelCylH)', 'mS'));
		s += circ(V, 0, 2.22, 0.2, 'url(#cyan)');
		s += ao(circ(V, 0, 0, 1.15, 'url(#rSteel)', 'mM')) + ring(V, 0, 0, 1.08, '#e6eaee', 1.6) + circ(V, 0, 0, 0.98, 'url(#bore)');
		for (const r of [0.86, 0.72]) s += ring(V, 0, 0, r, '#3a3c40', 1, 'opacity=".8"');
		s += pl(V, [[0.9, -0.55], [1.05, -0.35]], '#ffffff', 1.4, 'opacity=".6"');                         // a burr off the saw
	} else {
		s += ao(shp(pth(V, R4(-0.09, 1.8, 0.09, 2.1), 0.04), 'url(#steelCylH)', 'mS')) + circ(V, 0, 2.22, 0.2, 'url(#cyan)');
		s += ao(circ(V, 0, 0, 1.45, 'url(#rGalv)', 'mM')) + inside(discD(V, 0, 0, 1.45), ZINC);
		const rn = rng(57);
		for (let k = 0; k < 18; k++) { const a = k / 18 * 6.283; s += circ(V, 1.3 * Math.cos(a), 1.3 * Math.sin(a), 0.09 + rn() * 0.03, '#4f463e'); }
		s += ao(circ(V, 0, 0, 1.15, 'url(#rSteel)', 'mS')) + circ(V, 0, 0, 0.95, 'url(#bore)');
		for (const r of [1.05, 0.98]) s += ring(V, 0, 0, r, '#4a4d52', 1, 'opacity=".8"');
		s += shp(pth(V, R4(-0.12, 1.0, 0.12, 1.2), 0.03), '#c99a4a', null);                                 // a filed key notch
	}
	return s;
}

// ==== Muzzle_Duckbill: a pipe end hammered flat sideways, a wire-wrapped collar (local u 0..6.6) =================
const BILL = [[1.4, -1.15], [2.4, -1.02], [6.35, -0.58], [6.6, -0.5], [6.6, 0.5], [6.35, 0.58], [2.4, 1.02], [1.4, 1.15]];
function duckSide(V) {
	const right = V.kind === 'right';
	const bd = pth(V, BILL, 0.1);
	let s = ao(shp(bd, 'url(#steelCyl)', 'mM')) + rustOn(V, bd, [1.4, 6.6, -1.15, 1.15], right ? 61 : 62, 14);
	s += inside(bd, [[3.2, 0.5, 0.45], [4.9, -0.35, 0.4], [5.8, 0.25, 0.3]].map(([a, v, r]) => `<ellipse cx="${V.X(a)}" cy="${V.Y(v)}" rx="${r * V.s}" ry="${r * 0.55 * V.s}" fill="#000" opacity=".14"/>`).join(''));   // hammer dents
	s += pl(V, [[2.0, 0.0], [6.5, 0.0]], '#2b2d30', 1.6, 'opacity=".55"') + pl(V, [[2.0, 0.05], [6.5, 0.05]], '#e6eaee', 0.9, 'opacity=".5"');   // the folded edge's crease
	if (!right) s += weld(V, [[2.3, 0.0], [6.3, 0.0]], 0.14, 0.08);                                  // welded shut on one side
	else s += drip(V, 4.0, -0.2, 0.55) + drip(V, 5.6, -0.2, 0.4);
	for (const sgn of [-1, 1]) s += ao(circ(V, 4.3, sgn * 0.82, 0.16, 'url(#rGalv)', 'mS'));          // rivet heads through the bill
	s += shp(pth(V, R4(6.5, -0.5, 6.6, 0.5), 0.02), '#2a2c2f', null);                                // the mouth's lip
	s += pipe(V, [0, 0], [1.55, 0], 1.3, 'galv', 'mM') + inside(pipeD(V, [0, 0], [1.55, 0], 1.3), ZINC);
	s += wireWrap(V, 0.2, 1.1, 0, 1.3, 0.12, 0.05);
	s += weld(V, [[1.55, -1.2], [1.55, 1.2]], 0.15, 0.09);
	return s + wear(bd, '#e0a27a', 1.2, 0.5, '5 24 3 40');
}
const BILL_X = [[-1.2, 1.15], [1.2, 1.15], [2.6, 0.55], [2.6, -0.55], [1.2, -1.15], [-1.2, -1.15], [-2.6, -0.55], [-2.6, 0.55]];
function duckEnd(V, back) {
	let s = '';
	if (back) s += ao(shp(pth(V, BILL_X, 0.15), 'url(#steelPlate)', 'mM')) + rustOn(V, pth(V, BILL_X, 0.15), [-2.6, 2.6, -1.15, 1.15], 63, 10);
	if (!back) {
		s += ao(circ(V, 0, 0, 1.3, 'url(#rGalv)', 'mM'));
		s += ao(shp(pth(V, BILL_X, 0.15), 'url(#steelPlate)', 'mM')) + rustOn(V, pth(V, BILL_X, 0.15), [-2.6, 2.6, -1.15, 1.15], 64, 12);
		for (const sgn of [-1, 1]) s += pl(V, [[sgn * 1.2, 1.1], [sgn * 2.5, 0.55]], '#ffffff', 1, 'opacity=".4"');
		for (const [w, v] of [[-1.7, 0.82], [1.7, 0.82], [-1.7, -0.82], [1.7, -0.82]]) s += popRivet(V, w, v, 0.16);
		const mouth = pth(V, R4(-2.6, -0.5, 2.6, 0.5), 0.45);
		s += ao(shp(mouth, 'url(#steelCyl)', 'mS')) + shp(pth(V, R4(-2.38, -0.3, 2.38, 0.3), 0.28), '#0b0b0c', null);
		s += weld(V, [[2.45, -0.25], [2.45, 0.25]], 0.12, 0.08) + pl(V, [[-2.3, 0.33], [2.3, 0.33]], '#e6eaee', 1, 'opacity=".5"');
	} else {
		for (const [w, v] of [[-1.7, 0.82], [1.7, 0.82], [-1.7, -0.82], [1.7, -0.82]]) s += popRivet(V, w, v, 0.16);
		s += ao(circ(V, 0, 0, 1.3, 'url(#rGalv)', 'mM')) + inside(discD(V, 0, 0, 1.3), ZINC);
		for (let k = 0; k < 14; k++) { const a = k / 14 * 6.283; s += pl(V, [[1.25 * Math.cos(a), 1.25 * Math.sin(a)], [1.3 * Math.cos(a + 0.2), 1.3 * Math.sin(a + 0.2)]], '#c99a4a', 1.6); }
		s += circ(V, 0, 0, 1.15, '#151617') + ring(V, 0, 0, 1.15, '#8a9196', 1.2);
		for (const r of [1.05, 0.95, 0.85]) s += ring(V, 0, 0, r, '#3a3c40', 1, 'opacity=".85"');
		s += shp(pth(V, R4(-0.55, -0.12, 0.55, 0.12), 0.1), '#2a2c2f', null, 'opacity=".9"');       // the flat throat far down the bore
	}
	return s;
}

// ==== Magazine_Box8: a tin-can box under a bent-tin well, a rusted pipe for the pump (gun space) ===================
const MF8 = [[19.4, -5.2], [20.4, -13.0], [22.6, -21.0]], MR8 = [[13.6, -5.2], [14.2, -13.0], [16.0, -20.4]];
const BOX8 = [...Array.from({ length: 21 }, (_, i) => bez(MF8, i / 20)), ...Array.from({ length: 21 }, (_, i) => bez(MR8, 1 - i / 20))];
const band8 = (v0, v1, inset = 0) => [[edgeU(MR8, v0) + inset, v0], [edgeU(MF8, v0) - inset, v0], [edgeU(MF8, v1) - inset, v1], [edgeU(MR8, v1) + inset, v1]];
const BASE8 = [[15.7, -20.3], [23.0, -20.85], [23.25, -22.2], [15.9, -21.75]];
function magSide(V) {
	const right = V.kind === 'right';
	let s = '';
	// the pump's guide: a rusty pipe from the body's front, a galvanized pipe cap, a tab welded to the well
	s += pipe(V, [20, -2.5], [47, -2.5], 0.8, 'rust', 'mM') + rustOn(V, pipeD(V, [20, -2.5], [47, -2.5], 0.8), [20, 47, -3.3, -1.7], 81, 30);
	s += pipe(V, [47, -2.5], [48, -2.5], 0.95, 'galv') + pl(V, [[47.15, -3.4], [47.15, -1.6]], '#5e646a', 1.2) + pl(V, [[47.85, -3.3], [47.85, -1.7]], '#5e646a', 1.2);
	s += plate(V, R4(19.45, -3.45, 20.25, -3.0), 'steelPlate', 0.05, 'mS') + weld(V, [[20.05, -3.35], [20.05, -1.75]], 0.14, 0.08);
	// the box: a galvanized tin box, a teal-painted wrap low on it, black tape, brass wire, pressed ribs
	const bd = pth(V, BOX8, 0.22);
	s += ao(shp(bd, 'url(#galv)', 'mM')) + inside(bd, ZINC);
	s += inside(bd, [[16.5, -9.5, 1.2], [18.6, -16.0, 0.9]].map(([a, v, r]) => `<ellipse cx="${V.X(a)}" cy="${V.Y(v)}" rx="${r * V.s}" ry="${r * 0.6 * V.s}" fill="#ffffff" opacity=".16"/>`).join(''));
	s += rustOn(V, bd, [13.5, 23, -21, -5.4], 89, 9, ['#8a4a2a', '#b06a40', '#6e3a22']) + drip(V, 14.3, -5.3, 1.6) + drip(V, 18.8, -5.3, 1.2);
	for (const v of [-6.4, -12.2]) { const p = [[edgeU(MR8, v) + 0.4, v], [edgeU(MF8, v) - 0.4, v]]; s += pl(V, p, '#5e646a', 2.2, 'opacity=".7"') + pl(V, p.map(([a, vv]) => [a, vv - 0.1]), '#ffffff', 1.2, 'opacity=".6"'); }
	const teal = band8(-15.0, -20.45);
	s += inside(bd, ao(shp(pth(V, teal, 0.05), 'url(#tealPaint)', 'pM')) + chipsOn(V, pth(V, teal, 0.05), [14, 23, -21, -15], right ? 83 : 84, 18));
	s += inside(bd, pl(V, [[edgeU(MR8, -15.0), -15.0], [edgeU(MF8, -15.0), -15.0]], '#2b5450', 1.4));
	const tp = band8(-13.2, -14.4, -0.08);
	s += tapeBox(V, tp, 'tapeBlack', 0.35, 0.1) + shp(pth(V, [[edgeU(MF8, -13.3) + 0.05, -13.25], [edgeU(MF8, -13.3) + 0.75, -13.6], [edgeU(MF8, -13.8) + 0.6, -14.0], [edgeU(MF8, -13.9), -13.9]], 0.05), 'url(#tapeBlack)', 'pS');
	s += inside(bd, Array.from({ length: 5 }, (_, k) => { const v = -17.0 - k * 0.17; return pl(V, [[13, v], [24, v - 0.15]], '#3a2608', 0.11 * V.s + 1.2) + pl(V, [[13, v], [24, v - 0.15]], '#d29d48', 0.11 * V.s); }).join(''));
	if (!right) {   // witness holes punched along the back: red shells inside
		for (const v of [-7.4, -9.4, -11.4]) {
			const a = edgeU(MR8, v) + 0.95;
			s += circ(V, a, v, 0.42, '#1a0c08') + inside(discD(V, a, v, 0.36), shp(pth(V, R4(a - 0.6, v - 0.3, a + 0.6, v + 0.3), 0.1), 'url(#shellCyl)', null)) + ring(V, a, v + 0.05, 0.42, '#ffffff', 1, 'opacity=".5"');
		}
	} else {        // a riveted hazard tag, dents
		const tag = [[edgeU(MR8, -7.0) + 0.7, -7.0], [edgeU(MF8, -7.0) - 0.9, -7.05], [edgeU(MF8, -10.4) - 0.9, -10.45], [edgeU(MR8, -10.4) + 0.7, -10.4]];
		s += ao(shp(pth(V, tag, 0.08), 'url(#hazard)', 'pS')) + inside(pth(V, tag, 0.08), Array.from({ length: 10 }, (_, k) => shp(pth(V, [[13.5 + k * 0.7, -10.6], [13.85 + k * 0.7, -10.6], [15.6 + k * 0.7, -6.8], [15.25 + k * 0.7, -6.8]], 0), '#1c1c1c', null)).join(''));
		s += popRivet(V, tag[0][0] + 0.35, -7.35, 0.13) + popRivet(V, tag[2][0] - 0.35, -10.1, 0.13);
		s += drip(V, tag[2][0] - 0.35, -10.45, 0.9);
	}
	s += wear(bd, '#ffffff', 1.2, 0.55);
	// the well: a bent galvanized collar with a rolled lip, welds, a leaf-spring catch at the front
	const wd = R4(13.4, -5.2, 19.6, -3.0);
	s += zinc(V, wd, 0.1) + pipe(V, [13.3, -5.25], [19.7, -5.25], 0.17, 'galv') + weld(V, [[13.5, -3.08], [19.5, -3.08]], 0.22, 0.09);
	for (const a of [14.2, 18.8]) s += popRivet(V, a, -4.2, 0.17);
	s += rod(V, [[19.65, -3.4], [20.2, -3.9], [20.35, -4.9], [19.9, -5.5]], 0.09, '#8d9196', '#e6eaee');
	// the base: a rusty plate on two bolts, a brass-wire pull loop
	s += rod(V, [[16.2, -21.3], [15.0, -21.5], [14.45, -21.95], [14.9, -22.35], [16.1, -22.05]], 0.09, '#c99a4a', '#fff1c2');
	s += rusty(V, BASE8, 85, 8, 'rust3', 0.15) + hexHead(V, 17.4, -21.35, 0.24, 'url(#rRust)') + hexHead(V, 21.6, -21.55, 0.24, 'url(#rRust)');
	return s;
}
function magEnd(V, back) {
	let s = '';
	const box = pth(V, R4(-1.7, -21.0, 1.7, -5.2), 0.3);
	if (back) s += ao(circ(V, 0, -2.5, 0.8, 'url(#rRust)', 'mM')) + ring(V, 0, -2.5, 0.8, '#3a1d10', 1);
	s += ao(shp(box, 'url(#galv)', 'mM')) + inside(box, ZINC);
	s += inside(box, ao(shp(pth(V, R4(-2, -21, 2, -15.0), 0), 'url(#tealPaint)', 'pM')) + chipsOn(V, pth(V, R4(-2, -21, 2, -15.0), 0), [-2, 2, -21, -15], back ? 86 : 87, 10));
	s += tapeBox(V, R4(-1.78, -14.4, 1.78, -13.2), 'tapeBlack', 0.35, 0.05);
	s += inside(box, Array.from({ length: 5 }, (_, k) => { const v = -17.0 - k * 0.17; return pl(V, [[-2, v], [2, v - 0.05]], '#3a2608', 0.11 * V.s + 1.2) + pl(V, [[-2, v], [2, v - 0.05]], '#d29d48', 0.11 * V.s); }).join(''));
	if (back) s += pl(V, [[0, -5.6], [0, -12.8]], '#5e646a', 1.6, 'opacity=".7"') + pl(V, [[0.08, -5.6], [0.08, -12.8]], '#ffffff', 0.9, 'opacity=".5"');   // the folded seam
	else s += ao(shp(pth(V, R4(-0.5, -12.6, 0.5, -5.8), 0.25), 'url(#steelCylH)', 'mS'));             // a pressed spine
	s += wear(box, '#ffffff', 1.2, 0.55);
	s += zinc(V, R4(-1.9, -5.2, 1.9, -3.0), 0.1) + ao(shp(pth(V, R4(-1.98, -5.42, 1.98, -5.08), 0.15), 'url(#steelCylH)', 'mS'));
	s += weld(V, [[-1.85, -3.08], [1.85, -3.08]], 0.22, 0.09);
	if (!back) s += ao(circ(V, 0, -2.5, 0.95, 'url(#rGalv)', 'mM')) + ring(V, 0, -2.5, 0.62, '#5e646a', 1.4) + circ(V, 0, -2.5, 0.22, '#5e646a');
	s += rusty(V, R4(-1.95, -22.2, 1.95, -20.85), 88, 6, 'rust3', 0.15);
	s += rod(V, [[-0.55, -22.15], [-0.6, -22.38], [0.6, -22.38], [0.55, -22.15]], 0.07, '#c99a4a', '#fff1c2');
	return s;
}

// ==== Sight_GhostRing: a washer ring between ragged tin wings, a nail post, all on one strap (gun space) ==========
const H = 3.15 + 2.2, UR = 5.65, UF = 16.6;
const WING_L = [[3.7, 3.5], [7.85, 3.5], [7.5, 4.75], [7.15, 5.05], [6.7, 4.9], [6.3, 5.3], [5.0, 5.25], [4.6, 4.95], [4.1, 5.0], [3.85, 4.4]];
const WING_R = [[3.9, 3.5], [7.6, 3.5], [7.3, 4.6], [6.4, 5.05], [5.4, 5.15], [4.3, 4.8], [4.05, 4.2]];
const CLAMP = (a0, a1) => [[a0, 2.3], [a1, 2.3], [a1, 3.55], [a0, 3.55]];
function ghostSide(V) {
	const right = V.kind === 'right';
	let s = '';
	// the strap along the rail top, the two bent-strap clamps
	s += rusty(V, R4(3.3, 3.15, 17.8, 3.5), 91, 10, 'rust3', 0.06) + popRivet(V, 3.7, 3.33, 0.12) + popRivet(V, 17.4, 3.33, 0.12);
	s += tapeBox(V, R4(10.3, 3.1, 11.6, 3.55), 'tanTape', 0.2, 0.08);
	for (const [a0, a1] of [[3.4, 8.0], [15.4, 17.8]]) {
		s += rusty(V, CLAMP(a0, a1), 92 + a0, 6, 'rust3', 0.1) + weld(V, [[a0 + 0.1, 3.52], [a1 - 0.1, 3.52]], 0.16, 0.08);
		s += pl(V, [[a0 + 0.2, 2.75], [a1 - 0.2, 2.75]], '#3a1d10', 1.2, 'opacity=".6"');
	}
	if (right) { s += hexHead(V, 5.7, 2.9, 0.3) + hexHead(V, 16.6, 2.9, 0.26); }
	else {          // a wing nut on the left of the rear clamp, a hex bolt in front
		s += ao(shp(pth(V, [[4.6, 2.55], [5.2, 2.65], [5.5, 2.9], [5.2, 3.15], [4.6, 3.25], [4.45, 2.9]], 0.15), 'url(#steelPlate)', 'mS'));
		s += ao(shp(pth(V, [[6.8, 2.55], [6.2, 2.65], [5.9, 2.9], [6.2, 3.15], [6.8, 3.25], [6.95, 2.9]], 0.15), 'url(#steelPlate)', 'mS'));
		s += ao(circ(V, 5.7, 2.9, 0.3, 'url(#rSteel)', 'mS')) + hexHead(V, 16.6, 2.9, 0.26);
	}
	// the ring: a steel washer on a flat-bar stem wrapped in brass wire, its top above the wings
	s += ao(shp(pth(V, R4(UR - 0.4, 3.5, UR + 0.4, 4.15), 0.05), 'url(#steelPlate)', 'mS'));
	s += ao(shp(pth(V, R4(UR - 0.25, H - 1.35, UR + 0.25, H + 1.35), 0.1), 'url(#steelCyl)', 'mM')) + pl(V, [[UR - 0.25, H + 1.0], [UR + 0.25, H + 1.0]], '#c99a4a', 1.4);
	// the near wing: ragged galvanized tin on the left, a shorter sky-blue scrap on the right
	if (!right) s += zinc(V, WING_L, 0.08) + wear(pth(V, WING_L, 0.08), '#ffffff', 1.2, 0.6);
	else s += painted(V, WING_R, 'sky', 95, 10, 0.08) + wear(pth(V, WING_R, 0.08), '#e3eef5', 1.2, 0.7);
	s += popRivet(V, 4.4, 3.85, 0.13) + popRivet(V, 7.2, 3.85, 0.13) + weld(V, [[3.8, 3.52], [7.7, 3.52]], 0.16, 0.08);
	s += wireWrap(V, UR - 0.45, UR + 0.4, 3.82, 0.3, 0.13, 0.05);
	// the front: two bent-strap ears round a nail post, its glass bead on the line of sight
	const ears = [[UF - 0.7, 3.5], [UF + 0.7, 3.5], [UF + 0.55, H - 0.3], [UF - 0.55, H - 0.3]];
	s += ao(shp(pth(V, R4(UF - 0.13, 3.5, UF + 0.13, H - 0.05), 0.06), 'url(#steelCylH)', 'mS')) + circ(V, UF, H, 0.2, 'url(#cyan)') + circ(V, UF - 0.06, H + 0.06, 0.07, '#ffffff', null, 'opacity=".8"');
	s += rusty(V, ears, right ? 97 : 98, 5, 'rust3', 0.08);
	if (!right) s += painted(V, [[UF - 0.55, 4.3], [UF + 0.5, 4.3], [UF + 0.45, 4.85], [UF - 0.5, 4.85]], 'tealPaint', 99, 3, 0.05);
	s += weld(V, [[UF - 0.65, 3.55], [UF + 0.65, 3.55]], 0.15, 0.08);
	return s;
}
const ringD = (V, a, v, ro, ri) => discD(V, a, v, ro) + discD(V, a, v, ri);
function ghostEnd(V, back) {
	let s = '';
	const rear = () => {
		let r = '';
		r += zinc(V, R4(0.9, 3.45, 1.92, 3.72), 0.04) + zinc(V, R4(1.52, 3.5, 1.92, 5.3), 0.05) + painted(V, R4(-1.92, 3.45, -0.9, 3.72), 'sky', 100, 1, 0.04) + painted(V, R4(-1.92, 3.5, -1.52, 5.15), 'sky', 101, 2, 0.05);   // the wings, edge on
		r += ao(shp(pth(V, R4(-0.4, 3.5, 0.4, 4.15), 0.05), 'url(#steelPlate)', 'mS'));
		r += ao(shp(ringD(V, 0, H, 1.35, 1.0), 'url(#rSteel)', 'mM', 'fill-rule="evenodd"'));
		r += ring(V, 0, H, 1.0, '#2a2c30', 1.2) + ring(V, 0, H, 1.35, '#4a4d52', 1);
		return r;
	};
	const front = () => {
		const ears = [[-1.2, 3.5], [1.2, 3.5], [1.2, H - 0.5], [0.79, H - 0.3], [0.38, H - 0.5], [0.38, 3.95], [-0.38, 3.95], [-0.38, H - 0.5], [-0.79, H - 0.3], [-1.2, H - 0.5]];
		return rusty(V, ears, 103, 6, 'rust3', 0.08) + ao(shp(pth(V, R4(0 - 0.13, 3.5, 0 + 0.13, H - 0.05), 0.06), 'url(#steelCylH)', 'mS')) + circ(V, 0, H, 0.2, 'url(#cyan)') + circ(V, -0.06, H + 0.06, 0.07, '#ffffff', null, 'opacity=".8"');
	};
	const base = () => {
		const u = [[-1.3, 2.3], [-1.05, 2.3], [-1.05, 3.15], [1.05, 3.15], [1.05, 2.3], [1.3, 2.3], [1.3, 3.55], [-1.3, 3.55]];
		let b = rusty(V, u, 105, 5, 'rust3', 0.06);
		b += ao(shp(pth(V, [[1.3, 2.65], [1.65, 2.7], [2.45, 2.45], [2.5, 3.0], [1.75, 3.15], [1.3, 3.15]], 0.12), 'url(#steelPlate)', 'mS'));   // the wing nut (left)
		b += ao(shp(pth(V, R4(-1.62, 2.6, -1.3, 3.2), 0.06), 'url(#steelPlate)', 'mS'));                                                             // the bolt head (right)
		return b;
	};
	if (!back) s += rear() + base() + front();
	else {
		s += front() + base() + rear();
		for (let k = 0; k < 16; k++) { const a = k / 16 * 6.283; s += pl(V, [[1.05 * Math.cos(a), H + 1.05 * Math.sin(a)], [1.05 * Math.cos(a + 0.36), H + 1.05 * Math.sin(a + 0.36)]], '#c99a4a', 0.09 * V.s); }   // brass wire lip
	}
	return s;
}

// ==== Stock_Folding: a pipe frame on a door hinge, a taped raked grip, a tyre-tread butt (gun space, u <= 0) ======
const GRIP_F = [[0.2, 1.4], [0.2, -3.0], [-0.4, -5.0], [-0.6, -11.0], [-1.6, -13.4], [-3.6, -14.6], [-6.6, -14.0], [-7.6, -12.0], [-7.0, -9.0], [-5.2, -4.6], [-4.4, -1.0], [-3.4, 0.8], [-1.6, 1.6]];
const HEEL = [[-1.75, -13.2], [-3.6, -14.7], [-6.6, -14.1], [-7.6, -12.1], [-7.25, -11.0], [-6.2, -12.4], [-3.8, -12.9], [-2.0, -12.0]];
function stockSide(V) {
	const right = V.kind === 'right';
	let s = '';
	// the frame: a rusted upper pipe, a galvanized conduit upright and lower tube, elbow fittings
	const up0 = [-2.0, 1.0], up1 = [-30.6, 0.4], cr = [-31.4, -0.3], cl = [-31.4, -9.6], lo1 = [-30.6, -10.4], lo0 = [-6.2, -6.0];
	s += pipe(V, [-31.4, -0.4], [-31.4, -9.6], 0.6, 'galv', 'mM');
	s += pipe(V, lo1, lo0, 0.6, 'galv', 'mM') + inside(pipeD(V, lo1, lo0, 0.6), ZINC);
	s += pipe(V, up0, up1, 0.6, 'rust', 'mM') + rustOn(V, pipeD(V, up0, up1, 0.6), [-31, -2, -0.4, 1.8], 111, 30);
	for (const [a, v] of [cr, cl]) s += ao(circ(V, a, v, 0.82, 'url(#rRust)', 'mM')) + ring(V, a, v, 0.82, '#2a150b', 1.2);
	s += hoseClamp(V, -31.4, -5.0, 0.62, false);
	// a tan-taped foam cheek roll zip-tied to the upper pipe
	const roll = pipeD(V, [-24.5, 0.65], [-14.0, 0.85], 1.0);
	s += pipe(V, [-24.5, 0.65], [-14.0, 0.85], 1.0, 'tan') + tapeBands(V, roll, [-25, -13.5, -0.6, 2.0], 99, 0);
	for (let a = -24.3; a < -14.2; a += 0.6) s += pl(V, [[a, 0.65 - 0.95], [a + 0.3, 0.7 + 0.95]], '#5a4a36', 1.4, 'opacity=".7"');
	s += zipTie(V, -23.4, 0.67, 1.02, !right) + zipTie(V, -15.2, 0.83, 1.02, !right);
	if (right) {   // a diagonal strap brace between the tubes, on the right
		s += ao(shp(pipeD(V, [-9.0, -6.6], [-17.0, 0.6], 0.32), 'url(#steelPlate)', 'mS')) + hexHead(V, -9.2, -6.5, 0.22) + hexHead(V, -16.8, 0.5, 0.22);
	} else s += rod(V, [[-28.6, -10.0], [-28.9, -11.2], [-27.9, -11.7], [-27.2, -11.0], [-27.3, -9.9]], 0.09, '#c99a4a', '#fff1c2');   // sling loop
	s += weld(V, [[-6.7, -6.6], [-5.9, -5.3]], 0.15, 0.09) + weld(V, [[-29.9, -10.0], [-30.9, -10.8]], 0.15, 0.09);
	// the butt: a slab of tyre tread on a rusted backing plate
	const bp = R4(-31.95, -10.85, -31.35, 1.45);
	s += rusty(V, bp, 113, 6, 'rust3', 0.08);
	const pad = [[-32.0, -10.6], [-33.05, -10.5], [-33.2, -9.8], [-33.2, 0.5], [-33.05, 1.15], [-32.0, 1.25]];
	s += ao(shp(pth(V, pad, 0.15), 'url(#rubber)', 'rM'));
	for (let v = -9.9; v < 0.8; v += 0.9) s += shp(pth(V, R4(-33.25, v, -32.75, v + 0.42), 0.06), '#0d0d0d', null) + pl(V, [[-32.7, v + 0.48], [-32.1, v + 0.48]], '#4a4a4a', 1, 'opacity=".6"');
	for (const v of [-8.6, -1.0]) s += hexHead(V, -31.65, v, 0.22, 'url(#rRust)');
	// the mount plate (seats on the body's butt plate) and the grip: black friction tape, a tan band, brass wire, a rusty heel
	s += rusty(V, R4(-0.05, -3.3, 0.45, 1.95), 115, 4, 'rust3', 0.06);
	const gd = pth(V, GRIP_F, 0.6);
	s += ao(shp(gd, 'url(#tapeBlack)', 'pM')) + tapeBands(V, gd, [-8, 0.5, -14.8, 1.8], 0.55, 0.35);
	s += inside(gd, shp(pth(V, [[-8, -3.2], [0.5, -2.6], [0.5, -3.9], [-8, -4.5]], 0), 'url(#tanTape)', null) + `${FULL} fill="url(#cloth)" opacity=".4"/>`);
	s += inside(gd, Array.from({ length: 6 }, (_, k) => { const v = -10.4 - k * 0.17; return pl(V, [[-8, v - 0.3], [0.5, v + 0.2]], '#3a2608', 0.12 * V.s + 1.2) + pl(V, [[-8, v - 0.3], [0.5, v + 0.2]], '#d29d48', 0.12 * V.s); }).join(''));
	s += rusty(V, HEEL, 117, 6, 'rust3', 0.25) + hexHead(V, -4.6, -13.4, 0.2, 'url(#rRust)');
	// the hinge: a galvanized door hinge from the mount plate to the frame, its knuckle across the gun
	const leaf = [[0.0, -0.55], [-2.35, -0.2], [-2.35, 1.0], [0.0, 1.35]];
	s += zinc(V, leaf, 0.06) + screw(V, -0.9, 0.42, 0.16) + screw(V, -1.75, 0.42, 0.16);
	s += ao(circ(V, -2.4, 0.4, 0.58, 'url(#rGalv)', 'mM')) + circ(V, -2.4, 0.4, 0.2, '#3a3c40') + weld(V, [[-2.9, 0.9], [-2.6, 1.1]], 0.12, 0.08);
	if (!right) s += painted(V, [[-0.35, -6.4], [-0.5, -9.2], [-2.0, -9.0], [-1.85, -6.3]], 'tealPaint', 119, 3, 0.1);     // a teal patch under the thumb
	s += wear(gd, '#8a8781', 1.3, 0.7, '3 10 2 16 6 20');
	return s;
}
function stockEnd(V, back) {
	let s = '';
	const knuckle = () => pipe(V, [-2.0, 0.4], [2.0, 0.4], 0.56, 'galv') + pl(V, [[-0.7, -0.15], [-0.7, 0.95]], '#5e646a', 1.2) + pl(V, [[0.7, -0.15], [0.7, 0.95]], '#5e646a', 1.2);
	const pad = () => {
		const d = pth(V, R4(-1.7, -10.6, 1.7, 1.25), 0.35);
		let p = ao(shp(d, 'url(#rRubber)', 'rM'));
		p += inside(d, Array.from({ length: 13 }, (_, k) => { const v = -10.2 + k * 0.9; return shp(pth(V, [[-1.8, v], [-0.2, v + 0.35], [-0.2, v + 0.62], [-1.8, v + 0.27]], 0), '#0d0d0d', null) + shp(pth(V, [[1.8, v], [0.2, v + 0.35], [0.2, v + 0.62], [1.8, v + 0.27]], 0), '#0d0d0d', null); }).join(''));
		p += pl(V, [[0, -10.5], [0, 1.15]], '#0d0d0d', 0.14 * V.s);
		return p + wear(d, '#7a7d82', 1.2, 0.5, '4 24 2 40');
	};
	const grip = () => {
		const d = pth(V, R4(-1.6, -14.6, 1.6, -3.0), 0.7);
		return ao(shp(d, 'url(#tapeBlack)', 'pM')) + tapeBands(V, d, [-1.7, 1.7, -14.6, -3.0], 0.55, back ? -0.2 : 0.2) + inside(d, shp(pth(V, R4(-1.7, -4.3, 1.7, -3.0), 0), 'url(#tanTape)', null)) +
			rusty(V, R4(-1.6, -14.7, 1.6, -13.0), 121, 3, 'rust3', 0.4);
	};
	if (!back) {
		s += knuckle() + pad() + grip();
		const pd = pth(V, R4(-1.6, -3.3, 1.6, 1.95), 0.2);
		s += ao(shp(pd, 'url(#rust3)', 'mM')) + rustOn(V, pd, [-1.6, 1.6, -3.3, 1.95], 123, 12);
		for (const sd of [-1, 1]) s += pl(V, [[sd * 1.05, -3.0], [sd * 1.05, 1.6]], '#2a150b', 1.4, 'opacity=".6"');
		s += hexHead(V, 0, 1.35, 0.28, 'url(#rRust)') + hexHead(V, 0, -2.65, 0.28, 'url(#rRust)');
		s += ao(circ(V, 0, 0, 0.6, 'url(#rSteel)', 'mS')) + ring(V, 0, 0, 0.42, '#3a3c40', 1.2) + circ(V, 0, 0, 0.25, '#1a1b1d');
		s += weld(V, [[-1.55, -3.25], [1.55, -3.25]], 0.22, 0.08) + wear(pd, '#e0a27a', 1.2, 0.6);
	} else {
		s += rusty(V, R4(-1.6, -3.3, 1.6, 1.95), 125, 4, 'rust3', 0.2) + ao(circ(V, 0, 0.65, 1.0, 'url(#rTan)', 'pS')) + knuckle() + grip();
		s += rusty(V, R4(-1.75, -10.85, 1.75, 1.45), 127, 6, 'rust3', 0.15) + pad();
		for (const v of [-8.6, -1.0]) s += hexHead(V, 0, v, 0.26, 'url(#rRust)');
	}
	return s;
}

module.exports = [
	{ dir: 'Body_Salvaged', b: { u: [-0.3, 20.0], v: [-8.0, 3.15], w: [-2.5, 1.8] }, draw: { left: bodySide, right: bodySide, front: V => bodyEnd(V, false), back: V => bodyEnd(V, true) } },
	{ dir: 'Barrel_Short', b: { u: [0, 38], v: [-1.5, 2.45], w: [-1.8, 1.8] }, draw: { left: brlSide, right: brlSide, front: V => brlEnd(V, false), back: V => brlEnd(V, true) } },
	{ dir: 'Muzzle_Duckbill', b: { u: [0, 6.6], v: [-1.32, 1.32], w: [-2.65, 2.65] }, draw: { left: duckSide, right: duckSide, front: V => duckEnd(V, false), back: V => duckEnd(V, true) } },
	{ dir: 'Magazine_Box8', b: { u: [13.3, 48.05], v: [-22.45, -1.55], w: [-2.0, 2.0] }, draw: { left: magSide, right: magSide, front: V => magEnd(V, false), back: V => magEnd(V, true) } },
	{ dir: 'Sight_GhostRing', b: { u: [3.25, 17.85], v: [2.3, 6.75], w: [-1.95, 2.55] }, draw: { left: ghostSide, right: ghostSide, front: V => ghostEnd(V, false), back: V => ghostEnd(V, true) } },
	{ dir: 'Stock_Folding', b: { u: [-33.3, 0.45], v: [-14.75, 2.0], w: [-2.05, 2.05] }, draw: { left: stockSide, right: stockSide, front: V => stockEnd(V, false), back: V => stockEnd(V, true) } },
];
