// The other heroes' portrait frames for frame/gen.js: Odessa, Crane, Gauge and Pike, each a kit of motif builders
// and a table of layers (KITS) that gen.js draws in order.
const L = require('./lib.js');
const { D2R, R, P, INK, poly, smooth, smoothSeg, curl, bez, resample, tangents, ribbon, taper, swell, circle, ringPath, mirror, lin, rad, metal, drop, grit, glow, blurF,
	polyLen, pip, linkRing, chainLinks, starPts, gearPath, hexPts, compassRose } = L;
const { el, sym, mirD, WA, WB, WN, winPath, rectPath, defs, band, corners, innerShadow, cyanLine, sunRing, topCrest, ledger,
	BCL, BT, BTS, SERIF, cutText, raisedText, SPARKS, sparks, onPath, at, lay } = require('./common.js');

// =====================================================================================================================
// The other heroes' frames. Each keeps the saga's pieces (band, corners, sun-ring and crow feathers, Sexton's ledger)
// and lays its own motifs where Ellis's personal ones sit: down the sides (bandolier), along the bottom band (barbed
// wire), crossed behind the name plate (revolvers), the plate (grave plank), at the plate's foot (flowers), the lower
// left (Pa's lantern) and the loose bits (casings). Same reach into the window, same metals and lighting filters.

// ---------- Odessa Lark, the Cardsharp: cards, dice, a watch chain, a paddle-wheel, a torn marker ----------
const RED = '#a51d2d', BLK = '#15171b';
function odessaDefs() {
	defs.push(
		lin('gWine', [[0, '#8e2c3c'], [0.45, '#5e1626'], [1, '#2a0812']], 0, 0, 1, 1),
		lin('gVelvet', [[0, '#7a2232'], [0.5, '#4e1222'], [1, '#26080f']], 0, 0, 1, 1),
		lin('gCard', [[0, '#fffaf0'], [0.55, '#efe5cf'], [1, '#bfae8c']], 0, 0, 1, 1),
		lin('gMarker', [[0, '#eadcbc'], [1, '#a8946c']], 0, 0, 1, 1),
		lin('gFace', [[0, '#fbf5e6'], [1, '#d6c8a8']], 0, 0, 1, 1),
		`<pattern id="pLattice" width="6" height="6" patternUnits="userSpaceOnUse" patternTransform="rotate(45)"><path d="M0 0H6M0 0V6" stroke="#e0b44a" stroke-width="0.8" stroke-opacity="0.5"/></pattern>`,
		metal('fLacq', { blur: 3, surf: 2.5, diffK: 1.15, specK: 0.6, specExp: 18, specAmt: 0.5, sdx: 3, sdy: 5, sblur: 5, sop: 0.7, pad: 15 }),
		metal('fVelvet', { blur: 3.5, surf: 3, diffK: 1.2, specK: 0.25, specExp: 6, specAmt: 0.25, sdx: 3, sdy: 5, sblur: 4, sop: 0.65 }),
	);
}
// A playing card centred on x,y (portrait w x h): a plain ace of the suit, or (suit null) a wine and gilt back.
function card(w, h, suit, x, y, rot) {
	let s = `<rect x="${-w / 2}" y="${-h / 2}" width="${w}" height="${h}" rx="3.2" fill="${suit ? 'url(#gCard)' : 'url(#gWine)'}" stroke="${INK}" stroke-width="1"/>`;
	if (!suit) return at(x, y, rot, s + `<rect x="${-w / 2 + 3}" y="${-h / 2 + 3}" width="${w - 6}" height="${h - 6}" rx="2" fill="url(#pLattice)" stroke="#e0b44a" stroke-opacity="0.65" stroke-width="0.8"/>`);
	const f = `fill="${suit === 'heart' || suit === 'diamond' ? RED : BLK}"`;
	s += `<rect x="${-w / 2 + 2.6}" y="${-h / 2 + 2.6}" width="${w - 5.2}" height="${h - 5.2}" rx="2" fill="none" stroke="#c9b48a" stroke-width="0.7"/>`;
	s += pip(suit, 0, 0, h * 0.34, f) + pip(suit, -w / 2 + w * 0.18, -h / 2 + h * 0.15, h * 0.12, f) + pip(suit, w / 2 - w * 0.18, h / 2 - h * 0.15, h * 0.12, f, 180);
	return at(x, y, rot, s);
}
// A die in three-quarter view: front face showing n, the lit top face showing `top`, the right face in shade.
const DIE = { 1: [[0, 0]], 2: [[-1, -1], [1, 1]], 3: [[-1, -1], [0, 0], [1, 1]], 4: [[-1, -1], [1, -1], [-1, 1], [1, 1]], 5: [[-1, -1], [1, -1], [0, 0], [-1, 1], [1, 1]], 6: [[-1, -1], [1, -1], [-1, 0], [1, 0], [-1, 1], [1, 1]] };
function die(x, y, s, rot, n, top) {
	const h = s / 2, dx = s * 0.3, dy = -s * 0.24, pr = s * 0.09, o = s * 0.26;
	let g = `<path d="${poly([[-h, -h], [h, -h], [h + dx, -h + dy], [-h + dx, -h + dy]])}" fill="#efe4c8" stroke="${INK}" stroke-width="0.9" stroke-linejoin="round"/>`;
	g += `<path d="${poly([[h, -h], [h + dx, -h + dy], [h + dx, h + dy], [h, h]])}" fill="#9a8a6c" stroke="${INK}" stroke-width="0.9" stroke-linejoin="round"/>`;
	g += `<rect x="${-h}" y="${-h}" width="${s}" height="${s}" rx="${R(s * 0.14)}" fill="url(#gFace)" stroke="${INK}" stroke-width="1"/>`;
	g += DIE[n].map(p => circle(p[0] * o, p[1] * o, n === 1 ? pr * 1.6 : pr, `fill="${n === 1 ? RED : '#1a1210'}"`)).join('');
	g += DIE[top].map(p => { const v = 0.5 + p[1] * 0.3; return `<ellipse cx="${R(p[0] * o * 0.95 + v * dx)}" cy="${R(-h + v * dy)}" rx="${R(pr * 0.85)}" ry="${R(pr * 0.4)}" fill="#3a2e22"/>`; }).join('');
	return at(x, y, rot, g);
}
function pocketWatch(x, y) {
	let s = `<g fill="url(#gBrass)" filter="url(#fBrass)">${circle(x, y, 20)}<rect x="${x - 4}" y="${y - 27}" width="8" height="7" rx="1.5"/><path d="${ringPath(x, y - 31, 3.4, 6)}" fill-rule="evenodd"/></g>`;
	s += circle(x, y, 15.5, `fill="url(#gFace)" stroke="#4a3008" stroke-width="1"`);
	let ticks = '';
	for (let i = 0; i < 12; i++) { const a = i * 30 * D2R, r0 = i % 3 ? 12.8 : 11; ticks += `M${R(x + Math.sin(a) * r0)} ${R(y - Math.cos(a) * r0)}L${R(x + Math.sin(a) * 14.4)} ${R(y - Math.cos(a) * 14.4)}`; }
	s += `<path d="${ticks}" stroke="#2a1a10" stroke-width="1.1"/>`;
	s += `<path d="M${x} ${y}L${R(x - 6.5)} ${R(y - 3.8)}M${x} ${y}L${R(x + 9.5)} ${R(y - 5.5)}" stroke="#1a1210" stroke-width="1.5" stroke-linecap="round"/>` + circle(x, y, 1.7, 'fill="#e0b44a"');
	s += `<path d="M${x - 11} ${y - 6}A12.5 12.5 0 0 1 ${x - 4} ${y - 12}" fill="none" stroke="#ffffff" stroke-opacity="0.6" stroke-width="1.6" stroke-linecap="round"/>`;
	return s;
}
function odessaSides() {
	// a wine velvet sash down each side (the bandolier's place) with five aces held by gilt bands, a garnet brooch at
	// the top, a gold watch chain looping down outside it to her watch (left) and a pair of dice (right)
	let sash = el(ribbon(BCL, () => 14));
	const tl = resample(bez([70, 474], [77, 500], [73, 526], [64, 554], 20), 16), tt = tangents(tl);
	const side = sg => tl.map((p, i) => [p[0] - sg * tt[i][1] * 12, p[1] + sg * tt[i][0] * 12]);
	const e = tl[tl.length - 1], et = tt[tt.length - 1];
	sash += el(poly(side(1).concat([[e[0] - et[0] * 10, e[1] - et[1] * 10]], side(-1).reverse()))); // swallowtail end
	const edge = w => smooth(BCL.map((p, i) => [p[0] - BT[i][1] * w, p[1] + BT[i][0] * w]));
	const braid = `<path d="${edge(11.5)}${edge(-11.5)}" fill="none" stroke="#e0b44a" stroke-opacity="0.7" stroke-width="1.3" stroke-dasharray="2.2 1.6"/>`;
	let cards = '', bands = '';
	['spade', 'heart', 'club', 'diamond', 'spade'].forEach((suit, k) => {
		cards += onPath(BCL, BT, BTS[k], card(26, 42, suit, 9, 0, 90));
		bands += onPath(BCL, BT, BTS[k], `<rect x="-12" y="-15.5" width="5" height="31" rx="1.6"/>`);
	});
	const ch = chainLinks(bez([70, 190], [14, 244], [4, 430], [52, 574], 60), 8, 5.6, 1.5);
	const brooch = el(poly([[79, 159], [93, 176], [79, 193], [65, 176]]));
	const garnet = `<path d="${poly([[79, 166], [87, 176], [79, 186], [71, 176]])}" fill="#6e0e1c" stroke="#2a0408" stroke-width="0.8"/><path d="M76 172l3 -3l2.5 2.5" fill="none" stroke="#ffd0d8" stroke-width="1" stroke-opacity="0.8"/>`;
	return `<g filter="url(#fVelvet)" fill="url(#gVelvet)">${mirD(sash)}</g>` + mirD(braid) +
		`<g filter="url(#fDropNoOl)">${mirD(cards)}</g>` +
		`<g fill="url(#gBrassD)" filter="url(#fBrassSm)">${mirD(bands + ch.edge + ch.flat)}</g>` +
		`<g fill="url(#gBrass)" filter="url(#fBrass)">${mirD(brooch)}</g>` + mirD(garnet) +
		pocketWatch(52, 604) +
		`<g fill="url(#gBrassD)" filter="url(#fBrassSm)"><path d="${ringPath(972, 580, 2.6, 4.6)}" fill-rule="evenodd"/></g>` +
		`<g filter="url(#fDropSm)">${die(968, 598, 17, 12, 5, 2)}${die(979, 624, 15, -16, 3, 1)}</g>`;
}
function odessaSpread() {
	// a dealer's ribbon spread along the bottom band (the barbed wire's place), the last card turned up
	let s = '';
	const n = 20;
	for (let i = 0; i < n; i++) s += card(30, 44, i === n - 1 ? 'diamond' : null, 142 + i * 9.6, 957 - Math.sin(i / (n - 1) * Math.PI) * 3, -6 + i * 0.6);
	return `<g filter="url(#fDropNoOl)">${mirD(s)}</g>`;
}
function odessaCards() {
	// two aces crossed on each side of the name board (the revolvers' place), their inner edges tucked behind it
	return `<g filter="url(#fDrop)">${mirD(card(62, 88, 'heart', 626, 952, -17) + card(62, 88, 'spade', 640, 957, 13))}</g>`;
}
function odessaPlate() {
	// a riverboat's name board (the plank's place): wine lacquer, a gilt rule, the four suits, gilt letters
	const shape = `M428 1006V914Q428 903 440 902C468 900 484 880 512 880C540 880 556 900 584 902Q596 903 596 914V1006Q596 1012 590 1012H434Q428 1012 428 1006Z`;
	const inset = `M436 999V917Q436 910 444 909C470 907 486 889 512 889C538 889 554 907 580 909Q588 910 588 917V999Q588 1004 583 1004H441Q436 1004 436 999Z`;
	let s = `<g filter="url(#fLacq)"><path d="${shape}" fill="url(#gWine)" filter="url(#fGritSoft)"/></g>`;
	s += `<path d="${inset}" fill="none" stroke="#1a0408" stroke-width="2.4" stroke-opacity="0.6" transform="translate(0.8 1.2)"/>`;
	s += `<path d="${inset}" fill="none" stroke="#e0b44a" stroke-width="1.8"/>`;
	const g = 'fill="url(#gBrassD)"';
	s += `<g filter="url(#fBrassSm)">${pip('spade', 483, 908, 11, g)}${pip('heart', 500, 902, 11, g)}${pip('diamond', 524, 902, 11, g)}${pip('club', 541, 908, 11, g)}</g>`;
	s += raisedText('ODESSA', 512, 945, 27, 3.5, 'url(#gBrass)', '#16040a');
	s += `<path d="M474 955.5H502M522 955.5H550" stroke="#e0b44a" stroke-width="1.2"/>` + pip('diamond', 512, 955.5, 10, 'fill="#e0b44a"');
	s += raisedText('LARK', 512, 985, 28, 6, 'url(#gBrass)', '#16040a');
	let studs = '';
	for (const p of [[442, 917], [582, 917], [442, 994], [582, 994]]) studs += circle(p[0], p[1], 3.4);
	return s + `<g fill="url(#gBrass)" filter="url(#fBrassSm)">${studs}</g>`;
}
function odessaMarker() {
	// her torn marker (the IOU Sexton bought) at the board's foot, where Ellis's flowers lie
	let s = `<path d="M-36 -17H18L22 -12L17 -8L24 -3L18 2L23 7L17 12L21 17H-36Z" fill="url(#gMarker)" stroke="${INK}" stroke-width="0.9"/>`;
	s += `<text x="-31" y="-3" font-family="Caladea, 'DejaVu Serif', FreeSerif, serif" font-style="italic" font-weight="bold" font-size="12" fill="#2a1a10">I.O.U.</text>`;
	s += `<path d="M-31 7q3 -5 6 0t6 0t6 -1q4 -3 7 1M-31 12H8" fill="none" stroke="#2a1a10" stroke-width="0.9"/>`;
	return `<g filter="url(#fDropSm)">${at(440, 994, -9, s)}</g>`;
}
function paddleWheel() {
	// the riverboat's paddle-wheel in the lower left (where Ellis's frame hangs Pa's lantern): twelve boards between
	// iron rims under the paddle box's gilt fan, the river running off the lower boards
	const c = [62, 908];
	const p = (deg, r, w = 0) => { const a = deg * D2R; return [c[0] + Math.cos(a) * r - Math.sin(a) * w, c[1] + Math.sin(a) * r + Math.cos(a) * w]; };
	let box = `M${P(p(196, 58))}A58 58 0 0 1 ${P(p(344, 58))}L${P(p(344, 47))}A47 47 0 0 0 ${P(p(196, 47))}Z`;
	let louvres = '';
	for (let d = 202; d <= 338; d += 8.5) louvres += `M${P(p(d, 49.5))}L${P(p(d, 55.5))}`;
	let iron = `<path d="${ringPath(c[0], c[1], 37, 42)}" fill-rule="evenodd"/><path d="${ringPath(c[0], c[1], 13, 17)}" fill-rule="evenodd"/>`;
	let wood = '', bolts = '';
	for (let i = 0; i < 12; i++) {
		const d = 8 + i * 30;
		iron += el(poly([p(d, 9, -1.6), p(d, 41, -1.6), p(d, 41, 1.6), p(d, 9, 1.6)]));
		wood += el(poly([p(d, 25, -5.5), p(d, 46, -5.5), p(d, 46, 5.5), p(d, 25, 5.5)]));
		bolts += circle(...p(d, 39.5), 1.8);
	}
	let s = `<g fill="url(#gWine)" filter="url(#fWood)">${wood}</g>`;
	s += `<g fill="url(#gIronL)" filter="url(#fIron)">${iron}</g>`;
	s += `<g fill="url(#gBrass)" filter="url(#fBrassSm)">${bolts}${circle(c[0], c[1], 8.5)}</g>` + circle(c[0], c[1], 2.8, `fill="#3a2608"`);
	s += `<g fill="url(#gWine)" filter="url(#fLacq)"><path d="${box}"/></g><path d="${louvres}" stroke="#e0b44a" stroke-width="1.6" stroke-opacity="0.9"/>`;
	s += `<path d="M${P(p(196, 57))}A57 57 0 0 1 ${P(p(344, 57))}" fill="none" stroke="#e0b44a" stroke-width="1.4"/>`;
	s += `<path d="M14 966q8 -4 16 0t16 0t16 0t16 0t16 0M20 974q7 -3 14 0t14 0t14 0t14 0" fill="none" stroke="#bfe4f0" stroke-width="1.3" stroke-opacity="0.6" stroke-linecap="round"/>`;
	s += `<g fill="#cfefff" fill-opacity="0.75">${[[40, 958, 1.6], [86, 956, 1.8], [52, 984, 1.3], [76, 990, 1.5]].map(q => `<ellipse cx="${q[0]}" cy="${q[1]}" rx="${q[2] * 0.7}" ry="${q[2] * 1.3}"/>`).join('')}</g>`;
	return s;
}
function odessaBits() {
	return `<g filter="url(#fDropSm)">${die(156, 1004, 16, -12, 6, 2)}${die(184, 1012, 14, 22, 1, 4)}${die(742, 1008, 15, 8, 4, 5)}` +
		`${card(22, 32, 'club', 312, 1008, 74)}${card(22, 32, null, 806, 1010, -66)}</g>` + sparks('#ffd27a');
}

// ---------- Hollis Crane, the Unpaid: the toll chain and hook, a tin star, ferryman's coins, a lantern, cypress ----------
function craneDefs() {
	defs.push(
		lin('gPewter', [[0, '#c4cbc4'], [0.35, '#8f9891'], [0.75, '#5f6862'], [1, '#3e4640']], 0, 0, 1, 1),
		lin('gTin', [[0, '#e6e8e2'], [0.45, '#a6aca8'], [1, '#5c625e']], 0, 0, 1, 1),
		lin('gChain', [[0, '#a6aea4'], [0.4, '#666e66'], [1, '#2e342e']], 0, 0, 1, 1),
		lin('gSootW', [[0, '#04140e', 0.75], [0.25, '#04140e', 0.25], [0.45, '#04140e', 0], [0.9, '#04140e', 0], [1, '#04140e', 0.4]], 0, 864, 0, 966, true),
		rad('gTarnish', [[0, '#d6d8c8'], [0.45, '#9aa094'], [0.85, '#5e665a'], [1, '#3e463c']], 0.35, 0.3, 0.8),
		rad('gWispGlow', [[0, '#a8ffe0', 0.5], [0.3, '#6affc8', 0.24], [0.65, '#3ad8a8', 0.08], [1, '#3ad8a8', 0]]),
		rad('gWispGlass', [[0, '#f0fff8'], [0.18, '#c8ffe8'], [0.48, '#6ae8c0'], [0.8, '#2a8a72'], [1, '#0e3a30']], 0.5, 0.55, 0.6),
		metal('fPewter', { blur: 2, surf: 3, specK: 0.8, specExp: 14, specAmt: 0.55, sdx: 3, sdy: 5, sblur: 5, sop: 0.7, pad: 15, light: '#eef6ee' }),
	);
}
// A ferryman's coin: tarnished silver struck with a plain boat and oar.
function coin(x, y, r) {
	let s = circle(x, y, r, 'fill="url(#gTarnish)"') + circle(x, y, r * 0.8, 'fill="none" stroke="#2e342e" stroke-width="0.9" stroke-opacity="0.8"');
	s += `<path d="M${R(x - r * 0.45)} ${R(y + r * 0.08)}Q${x} ${R(y + r * 0.5)} ${R(x + r * 0.45)} ${R(y + r * 0.08)}ZM${R(x + r * 0.1)} ${R(y - r * 0.5)}L${R(x - r * 0.18)} ${R(y + r * 0.22)}" fill="none" stroke="#2e342e" stroke-width="1"/>`;
	return s + `<path d="M${R(x - r * 0.6)} ${R(y - r * 0.45)}A${R(r * 0.75)} ${R(r * 0.75)} 0 0 1 ${R(x + r * 0.1)} ${R(y - r * 0.75)}" fill="none" stroke="#f0f2e8" stroke-opacity="0.5" stroke-width="1"/>`;
}
const edgeCoin = (x, y, rot) => `<ellipse cx="${x}" cy="${y}" rx="9" ry="3.2" transform="rotate(${rot} ${x} ${y})" fill="url(#gTarnish)" stroke="${INK}" stroke-width="0.8"/>`;
const CCL = resample(bez([80, 190], [42, 256], [32, 400], [64, 520], 80), 70);
function craneSides() {
	// the toll chain hung down each side from an iron ring (the bandolier's place), fare coins hung on it, its hook
	// at the end
	const ch = chainLinks(CCL, 18, 14, 3.4);
	const ring = `<rect x="64" y="158" width="32" height="14" rx="4"/><path d="${ringPath(80, 181, 6, 11)}" fill-rule="evenodd"/>`;
	const hk = resample(bez([64, 532], [66, 566], [62, 592], [45, 592], 30).concat(bez([45, 592], [27, 592], [21, 572], [30, 551], 20)), 44);
	const hook = `<path d="${ringPath(64, 526, 4.4, 9.4)}" fill-rule="evenodd"/>` + el(ribbon(hk, taper(7, 1, 1.5))) + el(poly([[30, 551], [25, 562], [35, 558]]));
	let hangers = '', coins = '';
	for (const t of [0.28, 0.52, 0.76]) {
		const i = Math.round(t * (CCL.length - 1)), p = CCL[i], q = [p[0] - 15, p[1] + 17];
		hangers += `M${P(p)}L${P([q[0] + 3, q[1] - 10])}`;
		coins += coin(q[0], q[1], 10.5);
	}
	// a cypress spray tucked under the ring, hanging outside the chain
	const sp = frond([72, 176], [22, 300], 10, 10);
	const spray = `<path d="${sp.stem}" fill="none" stroke="#24180f" stroke-width="1.8"/><path d="${sp.needles}" fill="none" stroke="#1a2620" stroke-width="3.4" stroke-linecap="round"/>` +
		`<path d="${sp.needles}" fill="none" stroke="#56785f" stroke-width="2.2" stroke-linecap="round"/><path d="${sp.needles}" fill="none" stroke="#a8c8b0" stroke-width="0.6" stroke-opacity="0.55" stroke-linecap="round" transform="translate(-0.4 -0.5)"/>`;
	return `<g filter="url(#fDropNoOl)">${mirD(spray)}</g>` +
		`<g fill="none" stroke="#4a524a" stroke-width="1.8">${mirD(`<path d="${hangers}"/>`)}</g>` +
		`<g fill="url(#gChain)" filter="url(#fIron)">${mirD(ch.edge + ch.flat + ring + hook)}</g>` +
		`<g filter="url(#fDropSm)">${mirD(coins)}</g>`;
}
function frond(base, tip, bend, len) {
	// a bald cypress spray: a twig with flat needles in two ranks, swept forward and shortening to the tip
	const d = [tip[0] - base[0], tip[1] - base[1]], nn = [-d[1] / Math.hypot(...d), d[0] / Math.hypot(...d)];
	const pts = resample(bez(base, [base[0] + d[0] / 3 + nn[0] * bend, base[1] + d[1] / 3 + nn[1] * bend], [base[0] + d[0] * 2 / 3 + nn[0] * bend, base[1] + d[1] * 2 / 3 + nn[1] * bend], tip, 30), 34);
	const T = tangents(pts);
	let needles = '';
	for (let i = 2; i < pts.length - 1; i++) {
		const l = len * (1 - (i / pts.length) * 0.65);
		for (const sg of [1, -1]) { const a = Math.atan2(T[i][1], T[i][0]) + sg * 0.9; needles += `M${P(pts[i])}l${R(Math.cos(a) * l)} ${R(Math.sin(a) * l)}`; }
	}
	return { stem: smooth(pts), needles };
}
function cypress() {
	// cypress fronds along the bottom band (the barbed wire's place), a few strands of moss hanging from them
	const fr = [frond([428, 966], [226, 944], -10, 14), frond([416, 994], [146, 1000], 9, 15), frond([346, 958], [170, 934], -6, 12), frond([300, 1002], [206, 1016], 4, 10), frond([380, 944], [292, 934], -4, 10)];
	const st = fr.map(f => f.stem).join(''), nd = fr.map(f => f.needles).join('');
	let moss = '';
	for (const [x, y, l] of [[244, 948, 34], [268, 950, 48], [292, 952, 30], [196, 942, 40], [356, 958, 28], [322, 952, 40]]) moss += `M${x} ${y}q-3 ${l * 0.3} 1 ${l * 0.5}t-1 ${l * 0.5}`;
	const draw = `<path d="${moss}" fill="none" stroke="#8a9a8e" stroke-width="1.5" stroke-opacity="0.7" stroke-linecap="round"/>` +
		`<path d="${st}" fill="none" stroke="#24180f" stroke-width="2" stroke-linecap="round"/>` +
		`<path d="${nd}" fill="none" stroke="#1a2620" stroke-width="3.4" stroke-linecap="round"/>` +
		`<path d="${nd}" fill="none" stroke="#56785f" stroke-width="2.2" stroke-linecap="round"/>` +
		`<path d="${nd}" fill="none" stroke="#a8c8b0" stroke-width="0.6" stroke-opacity="0.55" stroke-linecap="round" transform="translate(-0.4 -0.5)"/>`;
	return `<g filter="url(#fDropNoOl)">${mirD(draw)}</g>`;
}
function craneChains() {
	// two lengths of toll chain crossed behind the coffin plate (the revolvers' place)
	const ch = chainLinks(bez([716, 1012], [612, 984], [498, 948], [396, 910], 40), 20, 15, 3.6);
	return `<g fill="url(#gChain)" filter="url(#fIron)">${mirD(ch.edge + ch.flat + `<path d="${ringPath(394, 909, 5, 10)}" fill-rule="evenodd"/>`)}</g>`;
}
function coffinPlate() {
	// his coffin plate (the plank's place): tarnished pewter, a crest struck with the ferryman's coin, engraved name
	const shape = `M444 898H492Q498 898 502 890Q507 881 512 881Q517 881 522 890Q526 898 532 898H580A10 10 0 0 0 590 908V996A10 10 0 0 0 580 1006H444A10 10 0 0 0 434 996V908A10 10 0 0 0 444 898Z`;
	const inset = `M449 905H575A8 8 0 0 0 583 913V991A8 8 0 0 0 575 999H449A8 8 0 0 0 441 991V913A8 8 0 0 0 449 905Z`;
	defs.push(`<clipPath id="cCoffin"><path d="${shape}"/></clipPath>`);
	let s = `<g filter="url(#fPewter)"><path d="${shape}" fill="url(#gPewter)" filter="url(#fGrit)"/></g>`;
	s += `<g clip-path="url(#cCoffin)"><path d="M440 960C470 950 480 1000 520 990S580 940 600 970M500 900C520 920 560 900 590 930" fill="none" stroke="#4a6a50" stroke-width="16" stroke-opacity="0.22" filter="url(#fB6)"/></g>`;
	s += `<path d="${inset}" fill="none" stroke="#e6ece4" stroke-opacity="0.45" stroke-width="1.4" transform="translate(0.8 1)"/><path d="${inset}" fill="none" stroke="#1e2420" stroke-width="1.4"/>`;
	s += `<path d="${ringPath(512, 892, 4.5, 6.5)}M510 892a2 2 0 1 0 4 0a2 2 0 1 0 -4 0" fill="#1e2420" fill-rule="evenodd"/>`;
	s += cutText('HOLLIS', 512, 944, 27, 3.5, '#e6ece4', '#1a201c');
	s += `<path d="M478 954.5H546" stroke="#e6ece4" stroke-opacity="0.45" stroke-width="1.2" transform="translate(0 1)"/><path d="M478 954.5H546" stroke="#1a201c" stroke-width="1.5"/>`;
	s += cutText('CRANE', 512, 984, 27, 3.5, '#e6ece4', '#1a201c');
	let screws = '';
	for (const p of [[447, 952], [577, 952]]) screws += circle(p[0], p[1], 3.6);
	return s + `<g fill="url(#gTin)" filter="url(#fBrassSm)">${screws}</g><path d="M444.5 950.5l5 3M574.5 950.5l5 3" stroke="#2a302a" stroke-width="1"/>`;
}
function tinStar() {
	// his tin star, dented, at the plate's foot where Ellis's flowers lie
	const pts = starPts(0, 0, 5, 22, 9.5);
	let shape = el(poly(pts)) + [0, 2, 4, 6, 8].map(i => circle(pts[i][0] * 1.06, pts[i][1] * 1.06, 3.2)).join('');
	let s = `<g filter="url(#fSilver)" fill="url(#gTin)">${at(428, 994, -14, shape)}</g>`;
	const inner = starPts(0, 0, 5, 13, 5.6);
	s += at(428, 994, -14,`<path d="${poly(inner)}" fill="none" stroke="#3a403c" stroke-width="0.9" stroke-opacity="0.8"/>` + circle(0, 0, 3.6, 'fill="none" stroke="#3a403c" stroke-width="0.9"') +
		`<path d="M5 -8q4 3 3 8" fill="none" stroke="#2a302c" stroke-width="1.2" stroke-opacity="0.7"/><path d="M6 -8.6q4 3 3 8" fill="none" stroke="#ffffff" stroke-width="0.7" stroke-opacity="0.5"/>`);
	return s;
}
function wispLantern() {
	// his lantern in the lower left: square tin with a peaked roof, a wire guard and a pale will-o'-wisp flame
	// (Pa's round brass lantern stays Ellis's)
	const X = 72;
	let s = circle(X, 920, 150, 'fill="url(#gWispGlow)" mask="url(#mGlow)"');
	s += `<g filter="url(#fIron)"><path d="M50 846C46 818 58 802 ${X} 800C86 802 98 818 94 846" fill="none" stroke="url(#gIronL)" stroke-width="3.4"/></g>`;
	const glass = `M50 864H94V966H50Z`;
	defs.push(`<clipPath id="cWisp"><path d="${glass}"/></clipPath>`);
	s += `<path d="${glass}" fill="#6affc8" opacity="0.55" filter="url(#fB6)"/><path d="${glass}" fill="url(#gWispGlass)"/><path d="${glass}" fill="url(#gSootW)"/>`;
	let guard = '';
	for (let k = -68; k <= 34; k += 34) guard += `M${50 + k} 864l102 102M${94 - k} 864l-102 102`;
	s += `<g clip-path="url(#cWisp)"><path d="${guard}" fill="none" stroke="#14201c" stroke-width="1.3" stroke-opacity="0.8"/><path d="M55 874C51 900 51 936 56 958" fill="none" stroke="#f0fff8" stroke-width="2.2" stroke-opacity="0.35" stroke-linecap="round"/></g>`;
	// the wisp burns in front of its own guard wires so it reads at a glance
	s += `<path d="M${X} 884C${X + 10} 902 ${X + 15} 926 ${X + 1} 954C${X - 13} 934 ${X - 11} 906 ${X} 884Z" fill="#a8ffe0" opacity="0.9" filter="url(#fB3)"/>`;
	s += `<path d="M${X} 898C${X + 6} 911 ${X + 8} 930 ${X + 1} 948C${X - 7} 934 ${X - 6} 914 ${X} 898Z" fill="#e6fff4" filter="url(#fGlow2)"/>`;
	s += `<path d="M${X} 914C${X + 3} 923 ${X + 3.5} 934 ${X + 0.5} 942C${X - 3} 934 ${X - 3} 923 ${X} 914Z" fill="#ffffff"/>`;
	s += `<path d="${glass}" fill="none" stroke="${INK}" stroke-width="1.2"/>`;
	const frame = `M42 856H102V866H42ZM42 964H102V974H42ZM38 974H106L101 990H43ZM43 866H51V964H43ZM93 866H101V964H93Z`;
	s += `<g fill="url(#gPewter)" filter="url(#fIron)"><path d="${frame}"/></g>`;
	s += `<g fill="url(#gTin)" filter="url(#fSilver)"><path d="M38 858L${X} 828L106 858Z"/><rect x="${X - 7}" y="816" width="14" height="14" rx="2"/><path d="M${X - 11} 818H${X + 11}L${X + 6} 809H${X - 6}Z"/></g>`;
	s += `<g fill="#06140f">${[56, 66, 78, 88].map(x => `<path d="M${x - 2} 853L${x} 847L${x + 2} 853Z"/>`).join('')}</g>`;
	s += `<g fill="#a8ffe0" opacity="0.7">${[56, 66, 78, 88].map(x => `<path d="M${x - 1} 853L${x} 850L${x + 1} 853Z"/>`).join('')}</g>`;
	s += `<g fill="url(#gIronL)" filter="url(#fBrassSm)"><path d="${ringPath(X, 804, 4, 7.5)}" fill-rule="evenodd"/></g>`;
	return s;
}
function craneBits() {
	return `<g filter="url(#fDropSm)">${coin(152, 1004, 9)}${edgeCoin(178, 1012, -8)}${coin(334, 1008, 8)}${coin(724, 1006, 9)}${edgeCoin(808, 1010, 12)}</g>` + sparks('#a8ffe0');
}

// ---------- Gauge, the Iron Hand: rails and spikes, gears, a pressure gauge, rivet rows, a steam whistle ----------
function gaugeDefs() {
	defs.push(
		lin('gPipe', [[0, '#3e464e'], [0.3, '#8a929c'], [0.55, '#4a525c'], [1, '#1a1e24']], 0, 0, 1, 0),
		lin('gRail', [[0, '#9aa2ac'], [0.3, '#5a626c'], [1, '#22282e']]),
		lin('gTie', [[0, '#6a5038'], [0.5, '#4a3626'], [1, '#2a1e14']], 0, 0, 1, 1),
		lin('gEnamel', [[0, '#2a2e34'], [1, '#0c0e10']]),
		lin('gDial', [[0, '#f6eedc'], [1, '#cfc2a2']], 0, 0, 1, 1),
		rad('gForge', [[0, '#ff9f1c', 0.42], [0.35, '#ff8a1c', 0.18], [0.7, '#e0600e', 0.06], [1, '#e0600e', 0]]),
		blurF('fSteam', 5, 200),
	);
}
function gaugeSides() {
	// a riveted iron strap down each side (the bandolier's place) carrying a steam pipe, flanged and banded in brass,
	// that turns out of the frame below; a steam whistle on top lets off a wisp
	const strap = `<rect x="46" y="178" width="38" height="366" rx="5"/>`;
	let rivets = '';
	for (let y = 190; y <= 534; y += 14.5) rivets += circle(51, y, 2.5) + circle(79, y, 2.5);
	const pipe = `<rect x="56" y="194" width="18" height="326" rx="2"/>` + el(ribbon(resample(bez([65, 512], [65, 540], [58, 552], [34, 552], 20), 20), () => 9)) + `<rect x="10" y="543" width="26" height="18" rx="2"/>`;
	let flanges = '', bolts = '', bands = '';
	for (const y of [300, 420]) { flanges += `<rect x="49" y="${y}" width="32" height="9" rx="2"/>`; bolts += circle(53, y + 4.5, 2) + circle(77, y + 4.5, 2); }
	flanges += `<rect x="30" y="539" width="7" height="26" rx="2"/>`;
	for (const y of [240, 360, 476]) bands += `<rect x="55" y="${y}" width="20" height="4.5" rx="1.5"/>`;
	const valve = `<rect x="57" y="184" width="16" height="14" rx="2"/><path d="M73 188L97 181L98 185L74 193Z"/>`;
	// the whistle: a wide brass bell over its cup, the slot between them, a domed cap and finial; a pull cord
	const bell = `<path d="M52 168V151Q52 146 57 146H73Q78 146 78 151V168Z"/><path d="M54 172H76V184H54Z"/><path d="M54 146Q65 132 76 146Z"/>` + circle(65, 134, 3.2);
	const mouth = `<rect x="52" y="168" width="26" height="4" fill="#120c06"/><path d="M56 150V165" stroke="#fff1c8" stroke-width="1.4" stroke-opacity="0.6"/><path d="M97 184V214" stroke="#6a4a2a" stroke-width="1.2"/><rect x="94.5" y="212" width="5" height="10" rx="2" fill="#7d5b3a"/>`;
	const steam = `<g fill="#f2f4f6" filter="url(#fSteam)">${circle(62, 122, 8, 'opacity="0.55"')}${circle(54, 106, 11, 'opacity="0.42"')}${circle(46, 88, 14, 'opacity="0.3"')}</g>`;
	return `<g fill="url(#gIronL)" filter="url(#fIronBig)">${mirD(strap)}</g>` +
		`<g fill="url(#gSilverD)" filter="url(#fSilver)">${mirD(rivets)}</g>` +
		`<g fill="url(#gPipe)" filter="url(#fIron)">${mirD(pipe)}</g>` +
		`<g fill="url(#gIronL)" filter="url(#fIron)">${mirD(flanges + valve)}</g>` +
		`<g fill="url(#gBrassD)" filter="url(#fBrassSm)">${mirD(bolts + bands)}</g>` +
		`<g fill="url(#gBrass)" filter="url(#fBrass)">${mirD(bell)}</g>` + mirD(mouth + steam);
}
function rail() {
	// a length of track along the bottom band (the barbed wire's place): ties, the rail, its spikes, a joint bar
	let ties = '', heads = '', bolts = '';
	for (let x = 142; x < 420; x += 46) {
		ties += `<rect x="${x}" y="966" width="28" height="19" rx="2"/>`;
		for (const sx of [x + 4, x + 18]) heads += `<path d="M${sx} 957H${sx + 7}V966H${sx}Z"/><path d="M${sx - 2} 955H${sx + 7}V959H${sx - 2}Z"/>`;
	}
	const steel = `<rect x="128" y="943" width="298" height="10" rx="2"/><rect x="132" y="953" width="294" height="7"/><rect x="124" y="959" width="306" height="7" rx="1.5"/>`;
	const joint = `<rect x="246" y="950" width="58" height="13" rx="2"/>`;
	for (const x of [254, 268, 282, 296]) bolts += el(poly(hexPts(x, 956.5, 3.4, 30)));
	const shine = `<path d="M130 944.5H424" stroke="#dfe4ea" stroke-width="1.2" stroke-opacity="0.7"/><path d="M134 956H424" stroke="#0e1114" stroke-width="2" stroke-opacity="0.6"/>`;
	return `<g fill="url(#gTie)" filter="url(#fWood)">${mirD(ties)}</g>` +
		`<g fill="url(#gRail)" filter="url(#fIron)">${mirD(steel)}</g>` + mirD(shine) +
		`<g fill="url(#gIronV)" filter="url(#fIron)">${mirD(joint + heads)}</g>` +
		`<g fill="url(#gIronL)" filter="url(#fBrassSm)">${mirD(bolts)}</g>`;
}
function railSpike(len) {
	// along +x from the head: square shank, chisel point, the head's lip overhanging one side
	return `<path d="M0 -9H${R(len - 40)}L${R(len)} -1.6V1.6L${R(len - 40)} 9H0Z"/><path d="M-15 -11Q-15 -14 -12 -14H3V12H-12Q-15 12 -15 9Z"/><path d="M-15 11H9Q14 11 14 16V22H-12Q-15 22 -15 19Z"/>`;
}
function railSpikes() {
	// two great rail spikes crossed behind the builder's plate (the revolvers' place), rust-streaked
	const r = lay(704, 1000, 392, 902);
	const iron = `<g transform="${r.tf}">${railSpike(r.len - 10)}</g>`;
	const rust = `<g transform="${r.tf}"><path d="M40 -4H70M110 3H128M170 -3H196M60 5H76M236 -2H262" stroke="#8a4a22" stroke-width="2.8" stroke-opacity="0.6" stroke-linecap="round"/><path d="M4 -6.5H${R(r.len - 54)}" stroke="#c4ccd4" stroke-width="1.4" stroke-opacity="0.55"/></g>`;
	return `<g fill="url(#gIronL)" filter="url(#fIronBig)">${mirD(iron)}</g>` + mirD(rust);
}
function builderPlate() {
	// a riveted brass builder's plate reading GAUGE (the plank's place): black enamel field, raised brass letters
	let s = `<g filter="url(#fBrass)"><ellipse cx="512" cy="946" rx="88" ry="63" fill="url(#gBrass)"/></g>`;
	s += `<ellipse cx="512" cy="946" rx="73" ry="49" fill="url(#gEnamel)" stroke="#3a2608" stroke-width="1.6"/>`;
	s += `<ellipse cx="512" cy="946" rx="66" ry="42" fill="none" stroke="#c8962f" stroke-width="1.4" stroke-opacity="0.8"/>`;
	let rivets = '';
	for (let i = 0; i < 18; i++) { const a = i * 20 * D2R; rivets += circle(512 + Math.cos(a) * 80.5, 946 + Math.sin(a) * 56, 2.9); }
	s += `<g fill="url(#gBrass)" filter="url(#fBrassSm)">${rivets}</g>`;
	s += `<g fill="url(#gBrassD)" filter="url(#fBrassSm)"><path d="${gearPath(512, 916, 6, 10, 2.4, 2.2)}" fill-rule="evenodd"/><path d="M470 917H498M526 917H554"/></g>`;
	s += `<path d="M470 917H498M526 917H554" stroke="#c8962f" stroke-width="1.4"/>`;
	s += raisedText('GAUGE', 512, 961, 30, 4, 'url(#gBrass)', '#000000');
	s += `<text x="512" y="979" ${SERIF} font-size="8.5" letter-spacing="3" fill="#c8962f">IRON HAND</text>`;
	return s;
}
function gaugeGear() {
	// a loose gear at the plate's foot where Ellis's flowers lie
	let holes = '';
	for (let i = 0; i < 4; i++) { const a = (i * 90 + 45) * D2R; const x = 444 + Math.cos(a) * 11, y = 992 + Math.sin(a) * 11; holes += `M${R(x - 4)} ${R(y)}a4 4 0 1 0 8 0a4 4 0 1 0 -8 0Z`; }
	return `<g fill="url(#gIronL)" filter="url(#fIron)"><path d="${gearPath(444, 992, 19, 12, 5, 0, 8)}${holes}" fill-rule="evenodd"/></g>` +
		`<g fill="url(#gBrass)" filter="url(#fBrassSm)"><path d="${ringPath(444, 992, 2.4, 5.6)}" fill-rule="evenodd"/></g>`;
}
function pressureDial() {
	// the pressure gauge in the lower left (Pa's lantern's place), its needle in the red, the firebox glow behind it
	const c = [70, 898];
	const pt = (th, r) => [c[0] + Math.sin(th * D2R) * r, c[1] - Math.cos(th * D2R) * r];
	let s = circle(70, 940, 150, 'fill="url(#gForge)" mask="url(#mGlow)"');
	s += `<g fill="url(#gPipe)" filter="url(#fIron)"><rect x="62" y="938" width="16" height="62" rx="2"/></g>`;
	s += `<g fill="url(#gBrass)" filter="url(#fBrassSm)"><path d="${poly(hexPts(70, 950, 10, 0))}"/></g>`;
	let wheel = `<path d="${ringPath(70, 980, 9.5, 13)}" fill-rule="evenodd"/>`;
	for (let i = 0; i < 4; i++) { const a = (i * 90 + 20) * D2R; wheel += `<path d="M70 980L${R(70 + Math.cos(a) * 11)} ${R(980 + Math.sin(a) * 11)}" stroke="#3a1a10" stroke-width="3"/>`; }
	s += `<g fill="#7a2a1a" filter="url(#fIron)">${wheel}${circle(70, 980, 3.6)}</g>`;
	s += `<g fill="url(#gBrass)" filter="url(#fBrass)">${circle(c[0], c[1], 46)}</g>`;
	s += circle(c[0], c[1], 40.5, `fill="none" stroke="#4a3008" stroke-width="1.4"`);
	let bolts = '';
	for (let i = 0; i < 6; i++) { const p = pt(i * 60 + 30, 43); bolts += circle(p[0], p[1], 2); }
	s += `<g fill="url(#gIronL)" filter="url(#fBrassSm)">${bolts}</g>`;
	s += circle(c[0], c[1], 38, `fill="url(#gDial)" stroke="${INK}" stroke-width="1.2"`);
	const a0 = pt(81, 31.5), a1 = pt(135, 31.5);
	s += `<path d="M${P(a0)}A31.5 31.5 0 0 1 ${P(a1)}" fill="none" stroke="#b8231a" stroke-width="5"/>`;
	let ticks = '';
	for (let i = 0; i <= 20; i++) { const th = -135 + i * 13.5, p0 = pt(th, i % 2 ? 31 : 28), p1 = pt(th, 35); ticks += `M${P(p0)}L${P(p1)}`; }
	s += `<path d="${ticks}" stroke="#1a1410" stroke-width="1.1"/>`;
	s += `<text x="70" y="921" ${SERIF} font-size="6.5" letter-spacing="1" fill="#3a2a1e">STEAM</text>`;
	const tip = pt(104, 31), tl = pt(104 - 90, 2.4), tr = pt(104 + 90, 2.4), tail = pt(284, 9);
	s += `<path d="${poly([tip, tl, tail, tr])}" fill="#1a1410"/>` + circle(c[0], c[1], 4, 'fill="url(#gBrass)" stroke="#3a2608" stroke-width="0.8"');
	s += `<path d="M44 884A28 28 0 0 1 60 868" fill="none" stroke="#ffffff" stroke-width="3" stroke-opacity="0.45" stroke-linecap="round"/>`;
	return s;
}
function gaugeBits() {
	const nut = (x, y, r, rot) => `<path d="${poly(hexPts(x, y, r, rot))}M${R(x - r * 0.45)} ${y}a${R(r * 0.45)} ${R(r * 0.45)} 0 1 0 ${R(r * 0.9)} 0a${R(r * 0.45)} ${R(r * 0.45)} 0 1 0 ${R(-r * 0.9)} 0Z" fill-rule="evenodd"/>`;
	const bolt = at(318, 1010, 168, `<rect x="0" y="-3" width="26" height="6" rx="1"/><path d="${poly(hexPts(-3, 0, 6, 0))}"/>`);
	return `<g fill="url(#gIronL)" filter="url(#fIron)">${nut(152, 1006, 6.5, 10)}${nut(182, 1012, 5, 40)}${bolt}${nut(806, 1009, 6, 20)}</g>` +
		`<g fill="url(#gBrassD)" filter="url(#fBrassSm)"><path d="${gearPath(724, 1006, 8, 10, 3, 2.6)}" fill-rule="evenodd"/>${circle(760, 1012, 3)}${circle(236, 1010, 3)}</g>` + sparks('#ff8a1c');
}

// ---------- Wendell Pike, the Surveyor: a tripod, a spyglass, a compass rose, survey lines, a measuring chain, a bowler ----------
function pikeDefs() {
	defs.push(
		lin('gParch', [[0, '#f0e2c0'], [0.5, '#dcc89c'], [1, '#a88e62']], 0, 0, 1, 1),
		lin('gRoll', [[0, '#6a5434'], [0.3, '#ead9b2'], [0.6, '#c8b088'], [1, '#5a4628']], 0, 0, 1, 0),
		lin('gStaff', [[0, '#e8dcc0'], [0.6, '#d0c09c'], [1, '#948466']], 0, 0, 1, 0),
		lin('gStake', [[0, '#c8a878'], [0.5, '#a07c52'], [1, '#6a4c30']]),
		lin('gFlag', [[0, '#e0703a'], [1, '#9a3a1a']], 0, 0, 1, 1),
		lin('gFelt', [[0, '#8a6440'], [0.5, '#5e4026'], [1, '#2e1e12']], 0, 0, 1, 1),
		lin('gLeg', [[0, '#a8835a'], [0.5, '#7d5b3a'], [1, '#4a3222']], 0, 0, 1, 0),
		lin('gTube', [[0, '#ffe7a8'], [0.3, '#e0b44a'], [0.65, '#b8862b'], [1, '#5e3e10']]),
		lin('gWrap', [[0, '#6a4630'], [0.4, '#3e2616'], [1, '#1e120a']]),
	);
}
function pikeSides() {
	// a graduated levelling staff down each side (the bandolier's place), brass-capped, with the measuring chain
	// wound down it: long wire links joined by rings, brass tallies, a brass handle at each end
	const staff = `<rect x="50" y="170" width="22" height="384" rx="2"/>`;
	let marks = '', red = '';
	for (let k = 0; k < 12; k++) {
		const y = 182 + k * 30, e = `M52 ${y}H64V${y + 3}H56V${y + 6}H62V${y + 9}H56V${y + 12}H64V${y + 15}H52Z`;
		if (k % 2) red += e; else marks += e;
	}
	let ticks = '';
	for (let y = 176; y < 548; y += 6) ticks += `M66 ${y}H${y % 30 < 6 ? 72 : 69.5}`;
	const caps = `<rect x="48" y="164" width="26" height="12" rx="2"/><rect x="48" y="548" width="26" height="10" rx="2"/><path d="M52 558H70L61 572Z"/>`;
	const path = resample(bez([82, 190], [24, 250], [98, 330], [36, 398], 50).concat(bez([36, 398], [-4, 440], [94, 480], [50, 536], 40)), 80);
	const n = 14, q = resample(path, n + 1);
	let rods = '', rings = '', tallies = '';
	for (let i = 0; i < n; i++) {
		const a = q[i], b = q[i + 1], ang = Math.atan2(b[1] - a[1], b[0] - a[0]) / D2R, len = Math.hypot(b[0] - a[0], b[1] - a[1]);
		rods += at(a[0], a[1], ang, `<rect x="3" y="-1.5" width="${R(len - 6)}" height="3" rx="1.5"/><path transform="translate(5 0)" d="${linkRing(4.6, 3.2, 1.3)}" fill-rule="evenodd"/><path transform="translate(${R(len - 5)} 0)" d="${linkRing(4.6, 3.2, 1.3)}" fill-rule="evenodd"/>`);
		if (i) rings += `<path d="${ringPath(a[0], a[1], 1.8, 3.6)}" fill-rule="evenodd"/>`;
		if (i === 4 || i === 9) tallies += at(a[0], a[1] + 4, 0, `<path d="M-4 0H4L5 8L2.5 6L0 10L-2.5 6L-5 8Z"/>`);
	}
	const handles = `<path d="M74 190Q74 176 84 176Q94 176 94 190" fill="none" stroke="url(#gBrassD)" stroke-width="4"/><path d="M44 538Q44 552 54 552Q64 552 62 538" fill="none" stroke="url(#gBrassD)" stroke-width="4"/>`;
	return `<g fill="url(#gStaff)" filter="url(#fWood)">${mirD(staff)}</g>` +
		mirD(`<path d="${marks}" fill="#1a1612"/><path d="${red}" fill="#a52a1a"/><path d="${ticks}" stroke="#1a1612" stroke-width="1"/>`) +
		`<g fill="url(#gBrassD)" filter="url(#fBrassSm)">${mirD(caps + tallies + rings)}</g>` +
		`<g filter="url(#fBrassSm)">${mirD(handles)}</g>` +
		`<g fill="url(#gSilverD)" filter="url(#fSilver)">${mirD(rods)}</g>`;
}
function surveyLine() {
	// a strip of his survey along the bottom left band (the barbed wire's place): the traverse with its stations,
	// station ticks and chainages, a compass rose, the strip's end rolled
	defs.push(`<clipPath id="cStrip"><path d="M138 941Q240 937 330 942T426 940V983Q330 986 240 981T138 984Z"/></clipPath>`);
	let s = `<g filter="url(#fDropSm)"><path d="M138 941Q240 937 330 942T426 940V983Q330 986 240 981T138 984Z" fill="url(#gParch)" filter="url(#fGritSoft)"/></g>`;
	s += `<g clip-path="url(#cStrip)"><path d="M138 941Q240 937 330 942T426 940V983Q330 986 240 981T138 984Z" fill="none" stroke="#5a3a1a" stroke-width="7" stroke-opacity="0.35" filter="url(#fB3)"/></g>`;
	const ink = '#3a2414';
	const rose = compassRose(166, 962, 15);
	s += `<g fill="${ink}">${rose.dark}</g><g fill="#a8885a">${rose.lit}</g>` + circle(166, 962, 9, `fill="none" stroke="${ink}" stroke-width="0.7"`);
	s += `<text x="166" y="944.5" ${SERIF} font-size="7" fill="${ink}">N</text>`;
	const st = [[194, 970], [238, 954], [286, 972], [334, 956], [382, 970], [420, 958]];
	let line = `M${P(st[0])}`, ticks = '', marks = '', labels = '';
	for (let i = 1; i < st.length; i++) {
		const a = st[i - 1], b = st[i], d = [b[0] - a[0], b[1] - a[1]], L0 = Math.hypot(...d), u = [d[0] / L0, d[1] / L0];
		line += `L${P(b)}`;
		for (let s0 = 7; s0 < L0 - 3; s0 += 7) { const p = [a[0] + u[0] * s0, a[1] + u[1] * s0]; ticks += `M${P([p[0] - u[1] * 2.6, p[1] + u[0] * 2.6])}L${P([p[0] + u[1] * 2.6, p[1] - u[0] * 2.6])}`; }
	}
	st.forEach((p, i) => { marks += `<path d="${poly([[p[0], p[1] - 4.5], [p[0] + 4, p[1] + 2.5], [p[0] - 4, p[1] + 2.5]])}" fill="none" stroke="${ink}" stroke-width="0.9"/>` + circle(p[0], p[1], 0.9, `fill="${ink}"`); if (i < 5) labels += `<text x="${p[0] + (i % 2 ? 0 : 0)}" y="${p[1] + (p[1] > 962 ? -6 : 11)}" ${SERIF} font-size="6" fill="${ink}">${i}+00</text>`; });
	s += `<path d="${line}" fill="none" stroke="${ink}" stroke-width="1.1" stroke-dasharray="5 2"/><path d="${ticks}" stroke="${ink}" stroke-width="0.7"/>` + marks + labels;
	s += `<g filter="url(#fDropSm)"><rect x="128" y="935" width="15" height="54" rx="7" fill="url(#gRoll)"/></g><ellipse cx="135.5" cy="936.5" rx="7" ry="2.6" fill="#c8b088" stroke="#4a3a20" stroke-width="0.7"/><path d="M135.5 936.5m-3 0a3 1.2 0 1 0 6 0" fill="none" stroke="#4a3a20" stroke-width="0.6"/>`;
	return s;
}
function surveyStake(top, point, label) {
	// a wooden survey stake lying from its top to its point, its chainage inked on it
	const r = lay(top[0], top[1], point[0], point[1]), L0 = r.len;
	const flip = Math.cos(Math.atan2(point[1] - top[1], point[0] - top[0])) < 0;
	const txt = `<text x="${R(L0 * 0.18)}" y="2.8" ${SERIF} font-size="8" letter-spacing="1" fill="#2a1a0e"${flip ? ` transform="rotate(180 ${R(L0 * 0.18)} 0)"` : ''}>${label}</text>`;
	return { wood: `<g transform="${r.tf}"><path d="M0 -7H${R(L0 - 30)}L${R(L0)} 0L${R(L0 - 30)} 7H0Z"/></g>`, det: `<g transform="${r.tf}"><path d="M2 -7V7" stroke="#4a3222" stroke-width="2.4"/><path d="M6 -4.5H${R(L0 - 34)}" stroke="#e8d0a4" stroke-width="1" stroke-opacity="0.45"/>${txt}</g>` };
}
function surveyStakes() {
	// two survey stakes crossed behind the cartouche (the revolvers' place), orange flagging tied at their tops
	const a = surveyStake([690, 906], [384, 1006], 'STA 4+50'), b = surveyStake([334, 906], [640, 1006], 'STA 0+00');
	// flagging: a knot round the stake and two long tails streaming out of the frame
	const tail = (x0, y0, ang, len, amp, w) => { const pts = []; for (let i = 0; i <= 24; i++) { const t = i / 24, a = ang * D2R; const o = amp * Math.sin(t * Math.PI * 2.2) * t; pts.push([x0 + Math.cos(a) * len * t - Math.sin(a) * o, y0 + Math.sin(a) * len * t + Math.cos(a) * o]); } return el(ribbon(pts, t => w * (1 - t * 0.55))); };
	const flag = `<path d="M670 905Q676 902 681 906L680 915Q675 918 669 914Z"/>` + tail(680, 909, 14, 52, 2.6, 3.2) + tail(679, 913, 48, 40, 2.2, 2.8);
	return `<g fill="url(#gStake)" filter="url(#fWood)">${a.wood}${b.wood}</g>` + a.det + b.det + `<g fill="url(#gFlag)" filter="url(#fLeatherSm)">${mirD(flag)}</g>`;
}
function cartouche() {
	// a mapmaker's cartouche (the plank's place): parchment between two rolled ends, inked border and name, a small
	// compass star, and a chained scale under WENDELL
	const body = `M440 892Q476 886 512 892T584 892V1004Q548 998 512 1004T440 1004Z`;
	let s = `<g filter="url(#fDrop)"><path d="${body}" fill="url(#gParch)" filter="url(#fGritSoft)"/></g>`;
	defs.push(`<clipPath id="cCart"><path d="${body}"/></clipPath>`);
	s += `<g clip-path="url(#cCart)"><path d="${body}" fill="none" stroke="#5a3a1a" stroke-width="9" stroke-opacity="0.3" filter="url(#fB3)"/></g>`;
	const ink = '#2e1c0e';
	s += `<path d="M448 902Q480 897 512 902T576 902V994Q544 989 512 994T448 994Z" fill="none" stroke="${ink}" stroke-width="1.3"/><path d="M452 906Q482 901 512 906T572 906V990Q542 985 512 990T452 990Z" fill="none" stroke="${ink}" stroke-width="0.6"/>`;
	const rose = compassRose(512, 916, 8.5, 8);
	s += `<g fill="${ink}">${rose.dark}</g><g fill="#a8885a">${rose.lit}</g>`;
	s += cutText('WENDELL', 512, 947, 20, 2, '#fff4dc', ink);
	let sc = `M478 957H546`;
	for (let x = 478; x <= 546; x += 8.5) sc += `M${R(x)} 954V${(x - 478) % 34 ? 957 : 960}`;
	s += `<path d="${sc}" fill="none" stroke="${ink}" stroke-width="1"/>`;
	s += cutText('PIKE', 512, 985, 27, 6, '#fff4dc', ink);
	const rollL = `<rect x="426" y="884" width="16" height="128" rx="7"/>`, rollR = `<rect x="582" y="884" width="16" height="128" rx="7"/>`;
	s += `<g fill="url(#gRoll)" filter="url(#fDropSm)">${rollL}${rollR}</g>`;
	s += [434, 590].map(x => `<ellipse cx="${x}" cy="885.5" rx="7.5" ry="2.8" fill="#d8c49a" stroke="#4a3a20" stroke-width="0.7"/><path d="M${x - 3.5} 885.5a3.5 1.3 0 1 0 7 0a2 0.8 0 1 0 -4 0" fill="none" stroke="#4a3a20" stroke-width="0.6"/>`).join('');
	return s;
}
function bowler() {
	// his bowler, set down at the cartouche's foot where Ellis's flowers lie
	const x = 432, y = 1002;
	const crown = `M${x - 21} ${y - 5}C${x - 23} ${y - 30} ${x - 12} ${y - 36} ${x} ${y - 36}C${x + 12} ${y - 36} ${x + 23} ${y - 30} ${x + 21} ${y - 5}Q${x} ${y - 1} ${x - 21} ${y - 5}Z`;
	const brim = `M${x - 34} ${y}Q${x - 37} ${y - 9} ${x - 26} ${y - 6}Q${x} ${y - 1} ${x + 26} ${y - 6}Q${x + 37} ${y - 9} ${x + 34} ${y}Q${x} ${y + 9} ${x - 34} ${y}Z`;
	let s = `<g fill="url(#gFelt)" filter="url(#fLeather)">${el(brim)}${el(crown)}</g>`;
	s += `<path d="M${x - 21.6} ${y - 13}Q${x} ${y - 9} ${x + 21.6} ${y - 13}L${x + 21} ${y - 5}Q${x} ${y - 1} ${x - 21} ${y - 5}Z" fill="#20140c"/>`;
	s += `<path d="M${x - 17} ${y - 11}l-4 -2v6z" fill="#3a2616"/>`;
	s += `<path d="M${x - 14} ${y - 28}Q${x - 6} ${y - 34} ${x + 4} ${y - 33}" fill="none" stroke="#c8a274" stroke-width="1.6" stroke-opacity="0.45" stroke-linecap="round"/>`;
	return s;
}
function tripod() {
	// his tripod in the lower left (Pa's lantern's place): wooden legs shod in brass, a levelling telescope with its
	// bubble vial, a plumb bob hung under the head
	const leg = (a, b, w0, w1) => el(ribbon(resample(bez(a, [a[0] + (b[0] - a[0]) / 3, a[1] + (b[1] - a[1]) / 3], [a[0] + (b[0] - a[0]) * 2 / 3, a[1] + (b[1] - a[1]) * 2 / 3], b, 12), 12), taper(w0, w1)));
	let s = `<g fill="#4a3222" filter="url(#fWood)">${leg([72, 858], [76, 990], 4, 3)}</g>`;
	s += `<path d="M72 858V944" stroke="#2a1e14" stroke-width="0.8"/><g fill="url(#gBrass)" filter="url(#fBrassSm)"><path d="M66 944H78L72 964Z"/><rect x="67" y="940" width="10" height="5" rx="1.5"/></g>`;
	s += `<g fill="url(#gLeg)" filter="url(#fWood)">${leg([66, 858], [38, 990], 4.6, 3.2)}${leg([78, 858], [106, 990], 4.6, 3.2)}</g>`;
	const shoes = `<path d="M34 988L42 990L36 1006Z"/><path d="M102 990L110 988L108 1006Z"/><path d="M72 988L80 990L77 1004Z"/>`;
	const clamps = `<rect x="53" y="900" width="10" height="5" rx="1.5" transform="rotate(12 58 902)"/><rect x="81" y="900" width="10" height="5" rx="1.5" transform="rotate(-12 86 902)"/>`;
	const head = `<ellipse cx="72" cy="853" rx="19" ry="5.5"/><rect x="67" y="836" width="10" height="16" rx="2"/>` + circle(56, 851, 2.6) + circle(88, 851, 2.6) + circle(72, 858, 2.6);
	const tube = `<rect x="34" y="821" width="72" height="12" rx="2"/><rect x="100" y="818" width="14" height="18" rx="2"/><rect x="26" y="823" width="10" height="8" rx="1.5"/><rect x="60" y="812" width="24" height="8" rx="2"/>`;
	s += `<g fill="url(#gBrassD)" filter="url(#fBrassSm)">${shoes}${clamps}</g><g fill="url(#gBrass)" filter="url(#fBrass)">${head}</g><g fill="url(#gTube)" filter="url(#fBrassSm)">${tube}</g>`;
	s += `<rect x="63" y="813.5" width="18" height="4.5" rx="2.2" fill="#9ef0e0" stroke="#2a3a34" stroke-width="0.6"/><ellipse cx="73" cy="815" rx="3" ry="1.2" fill="#ffffff" opacity="0.85"/>`;
	s += `<ellipse cx="113.5" cy="827" rx="1.6" ry="7" fill="#6ee7ff" opacity="0.65"/><path d="M38 824H100" stroke="#fff1c8" stroke-width="1.2" stroke-opacity="0.55"/>`;
	return s;
}
function spyglass() {
	// his spyglass, drawn out, lying along the bottom right band (the barbed wire's place there)
	let brass = `<rect x="-6" y="-11.5" width="10" height="23" rx="2"/><rect x="96" y="-10.5" width="8" height="21" rx="1.5"/><rect x="104" y="-8" width="56" height="16"/>` +
		`<rect x="158" y="-9" width="6" height="18" rx="1.5"/><rect x="164" y="-6.5" width="40" height="13"/><rect x="202" y="-7.5" width="5" height="15" rx="1.5"/><rect x="207" y="-5" width="20" height="10" rx="1"/><rect x="226" y="-6" width="5" height="12" rx="1.5"/>`;
	const wrap = `<rect x="4" y="-10" width="92" height="20"/>`;
	const det = `<path d="M6 -7.5H94M6 7.5H94" stroke="#c8a274" stroke-width="0.8" stroke-opacity="0.5" stroke-dasharray="2.4 1.8"/><path d="M106 -5H158M166 -3.6H202" stroke="#fff1c8" stroke-width="1.1" stroke-opacity="0.6"/><ellipse cx="-6.5" cy="0" rx="2.4" ry="9" fill="#6ee7ff" opacity="0.6"/>`;
	const tf = 'translate(652 994) rotate(-3) scale(0.86)';
	return `<g filter="url(#fLeatherSm)" fill="url(#gWrap)"><g transform="${tf}">${wrap}</g></g><g filter="url(#fBrassSm)" fill="url(#gTube)"><g transform="${tf}">${brass}</g></g><g transform="${tf}">${det}</g>`;
}
function pikeBits() {
	const pin = (x, y, rot) => at(x, y, rot, `<path d="M0 -1.2H40L46 0L40 1.2H0Z" fill="url(#gSilverD)"/><path d="${ringPath(-5, 0, 2.6, 4.6)}" fill="url(#gSilverD)" fill-rule="evenodd"/><path d="M2 -1L10 -7L12 -1Z" fill="url(#gFlag)"/>`);
	const pencil = at(302, 1010, -6, `<path d="M0 -3H40V3H0Z" fill="#c8a060"/><path d="M40 -3L48 0L40 3Z" fill="#e8d0a0"/><path d="M45.5 -1L48 0L45.5 1Z" fill="#2a2a2e"/><path d="M0 -3H40" stroke="#fff1c8" stroke-width="0.8" stroke-opacity="0.6"/>`);
	return `<g filter="url(#fDropSm)">${pin(150, 1010, -8)}${pin(214, 1014, 172)}${pencil}${pin(760, 1014, 4)}</g>` + sparks('#ffb55c');
}

// Each kit: the plate's name, its own defs, and its layers in drawing order (the saga's pieces from common.js).
const KITS = {
	Odessa: { name: 'Odessa Lark', defs: odessaDefs, layers: { inner: innerShadow, band, cyan: cyanLine, corners, crest: topCrest, sides: odessaSides, spread: odessaSpread, cross: odessaCards, plate: odessaPlate, marker: odessaMarker, wheel: paddleWheel, ledger, bits: odessaBits } },
	Crane: { name: 'Hollis Crane', defs: craneDefs, layers: { inner: innerShadow, band, cyan: cyanLine, corners, crest: topCrest, sides: craneSides, fronds: cypress, cross: craneChains, plate: coffinPlate, star: tinStar, lantern: wispLantern, ledger, bits: craneBits } },
	Gauge: { name: 'Gauge', defs: gaugeDefs, layers: { inner: innerShadow, band, cyan: cyanLine, corners, crest: topCrest, sides: gaugeSides, rail, cross: railSpikes, plate: builderPlate, gear: gaugeGear, dial: pressureDial, ledger, bits: gaugeBits } },
	Pike: { name: 'Wendell Pike', defs: pikeDefs, layers: { inner: innerShadow, band, cyan: cyanLine, corners, crest: topCrest, sides: pikeSides, survey: surveyLine, cross: surveyStakes, plate: cartouche, hat: bowler, tripod, spyglass, ledger, bits: pikeBits } },
};

module.exports = { KITS };
