// Shared drawing of the portrait frames (frame/gen.js): the window, the gradient and filter defs, the band and its
// filigree, the corner and side bosses, the cyan line, Saint Ada's sun-ring with the crow feathers and Sexton's
// ledger (the saga's pieces every hero's frame carries), the side path the straps hang on, and the small helpers
// the heroes' kits share. `defs` is the one list every module pushes into, in drawing order.
const L = require('./lib.js');
const { D2R, R, P, INK, poly, smooth, smoothSeg, curl, bez, resample, tangents, ribbon, taper, swell, circle, ringPath, mirror, lin, rad, metal, drop, grit, glow, blurF,
	polyLen, pip, linkRing, chainLinks, starPts, gearPath, hexPts, compassRose } = L;
const el = d => `<path d="${d}"/>`;
const sym = (attrs, c) => `<g ${attrs}>${c}${mirror(c)}</g>`;
const mirD = c => c + mirror(c);

// ---------- window ----------
const WA = 96, WB = 928, WN = 28; // window square and corner notch radius (notch centred on the square's corner)
function winPath(inset) {
	const a = WA + inset, b = WB - inset, r = WN + inset;
	const e = Math.sqrt(r * r - inset * inset);
	return `M${R(WA + e)} ${R(a)}H${R(WB - e)}A${R(r)} ${R(r)} 0 0 0 ${R(b)} ${R(WA + e)}V${R(WB - e)}A${R(r)} ${R(r)} 0 0 0 ${R(WB - e)} ${R(b)}H${R(WA + e)}A${R(r)} ${R(r)} 0 0 0 ${R(a)} ${R(WB - e)}V${R(WA + e)}A${R(r)} ${R(r)} 0 0 0 ${R(WA + e)} ${R(a)}Z`;
}
const rectPath = i => `M${i} ${i}H${1024 - i}V${1024 - i}H${i}Z`;

// ---------- defs ----------
const defs = [];
defs.push(
	lin('gIron', [[0, '#59616b'], [0.3, '#2b3138'], [0.7, '#1b2026'], [1, '#15191e']], 0, 0, 1, 1),
	lin('gIronV', [[0, '#5a626c'], [0.35, '#343b43'], [1, '#191d23']]),
	lin('gIronL', [[0, '#6b737d'], [0.4, '#3d444d'], [1, '#20252c']]),
	lin('gIronBand', [[0, '#2b3138'], [0.5, '#20252c'], [1, '#15191e']], 0, 0, 1, 1),
	lin('gSilverD', [[0, '#d6d0c2'], [0.45, '#9a9da2'], [1, '#5a5e64']], 0, 0, 1, 1),
	lin('gBrass', [[0, '#ffe7a8'], [0.22, '#e0b44a'], [0.6, '#b8862b'], [1, '#6e4a14']]),
	lin('gBrassD', [[0, '#e0b44a'], [0.4, '#b8862b'], [1, '#6e4a14']], 0, 0, 1, 1),
	lin('gLeather', [[0, '#8a3a2c'], [0.55, '#6e2e24'], [1, '#4a1d19']], 0, 0, 1, 1),
	lin('gStrap', [[0, '#7a3a28'], [1, '#5a2a1e']], 0, 0, 1, 1),
	lin('gWood', [[0, '#9a7650'], [0.45, '#7d5b3a'], [1, '#4a3222']]),
	lin('gBone', [[0, '#f6eedc'], [0.45, '#e8dcc4'], [1, '#a8987a']], 0, 0, 1, 1),
	lin('gPages', [[0, '#e8dcc4'], [1, '#9a8a6c']]),
	lin('gSweep', [[0, '#ffffff', 0.10], [0.45, '#ffffff', 0], [0.55, '#000000', 0], [1, '#000000', 0.35]], 0, 0, 1, 1),
	lin('gCart', [[0, '#ffe7a8'], [0.3, '#e0b44a'], [0.7, '#b8862b'], [1, '#6e4a14']], 0, -5.5, 0, 5.5, true),
	lin('gLeadC', [[0, '#d6d1c4'], [0.4, '#8c9096'], [1, '#4a4e54']], 0, -5, 0, 5, true),
	lin('gLoop', [[0, '#4a1e16'], [0.35, '#9a4c34'], [0.55, '#8a4030'], [1, '#3e1812']], 0, -8, 0, 8, true),
	rad('gEmber', [[0, '#fffbe6'], [0.25, '#ffe7a8'], [0.55, '#ff9f1c'], [0.85, '#e0600e'], [1, '#8a2e08']], 0.45, 0.4, 0.6),
	rad('gEmberGlow', [[0, '#ffb347', 0.95], [0.4, '#ff8a1c', 0.55], [1, '#ff8a1c', 0]]),
	rad('gHalo', [[0, '#ff9f1c', 0.30], [0.45, '#ff8a1c', 0.12], [1, '#ff8a1c', 0]]),
	rad('gLampGlow', [[0, '#ffb347', 0.55], [0.3, '#ff9f1c', 0.26], [0.65, '#ff8a1c', 0.08], [1, '#ff8a1c', 0]]),
	rad('gGlass', [[0, '#fffbe6'], [0.2, '#ffe7a8'], [0.45, '#ffb347'], [0.78, '#f07a14'], [1, '#8a3a0c']], 0.5, 0.55, 0.6),
	rad('gDiscIron', [[0, '#2b3138'], [0.7, '#15191e'], [1, '#0e1114']], 0.4, 0.35, 0.7),
	rad('gCoin', [[0, '#e8e2d4'], [0.4, '#a9a69e'], [0.8, '#6e7076'], [1, '#4a4c50']], 0.35, 0.3, 0.8),
	metal('fIron', { blur: 1.8, surf: 3, specK: 0.9, specExp: 14, specAmt: 0.7 }),
	metal('fIronBig', { blur: 2.6, surf: 4, specK: 0.8, specExp: 12, specAmt: 0.6, sdx: 3, sdy: 4.5, sblur: 4, pad: 15 }),
	metal('fBand', { blur: 2.4, surf: 3, specK: 0.5, specExp: 12, specAmt: 0.45, ol: 0.8, sdx: 1.5, sdy: 2.5, sblur: 2.5, sop: 0.7, pad: 3 }),
	metal('fSilver', { blur: 1.1, surf: 2.2, specK: 1.1, specExp: 18, specAmt: 0.8, ol: 0.9, sdx: 1.2, sdy: 1.8, sblur: 1.2, sop: 0.85, pad: 5 }),
	metal('fSilverBig', { blur: 1.5, surf: 2.6, specK: 1.0, specExp: 16, specAmt: 0.75, ol: 1.1, sdx: 2, sdy: 3, sblur: 2.2, sop: 0.75 }),
	metal('fBrass', { blur: 1.6, surf: 3, specK: 1.1, specExp: 20, specAmt: 0.85, light: '#fff1c8' }),
	metal('fBrassSm', { blur: 1, surf: 2.2, specK: 1.1, specExp: 22, specAmt: 0.9, ol: 0.9, sdx: 1.2, sdy: 1.8, sblur: 1.4, light: '#fff1c8' }),
	metal('fLeather', { blur: 3.5, surf: 3, diffK: 1.15, specK: 0.35, specExp: 8, specAmt: 0.35, sdx: 3, sdy: 5, sblur: 4, sop: 0.65 }),
	metal('fLeatherSm', { blur: 1.6, surf: 2, diffK: 1.15, specK: 0.4, specExp: 10, specAmt: 0.35, ol: 0.9, sdx: 1.5, sdy: 2.5, sblur: 2, sop: 0.6 }),
	metal('fWood', { blur: 3, surf: 2.5, diffK: 1.15, specK: 0.25, specExp: 8, specAmt: 0.3, sdx: 3, sdy: 5, sblur: 5, sop: 0.7, pad: 15 }),
	drop('fDrop', {}),
	drop('fDropSm', { sdx: 1.5, sdy: 2.5, sblur: 1.8, ol: 0.9 }),
	drop('fDropNoOl', { ol: 0, sdx: 2, sdy: 3, sblur: 2.5 }),
	drop('fFeather', { ol: 1.0, sdx: 3, sdy: 5, sblur: 4, sop: 0.7 }),
	grit('fGrit', {}),
	grit('fGritSoft', { f1: 0.7, f2: 0.03, a1: 0.22, a2: 0.3, seed: 21 }),
	grit('fGritWood', { f1: 0.6, f2: 0.05, a1: 0.3, a2: 0.35, seed: 11 }),
	glow('fGlow2', 2),
	glow('fGlow4', 4, 1.2),
	glow('fCyan', 2.5, 1.4),
	blurF('fB1', 1), blurF('fB2', 2), blurF('fB3', 3), blurF('fB6', 6), blurF('fB10', 10),
	`<mask id="mGlow" maskUnits="userSpaceOnUse" x="0" y="0" width="1024" height="1024"><rect width="1024" height="1024" fill="#fff"/><path d="${winPath(0)}" fill="#5a5a5a" filter="url(#fB10)"/></mask>`,
	`<clipPath id="cWin"><path d="${winPath(0)}"/></clipPath>`,
	`<clipPath id="cBand"><path d="${rectPath(40)} ${winPath(0)}" clip-rule="evenodd"/></clipPath>`
);

// ---------- band ----------
// S-scroll: one continuous centreline through (xc,y0), both ends rolled into opposite spirals.
function sScrollPts(xc, y0, ang, len, k0, k1, p, flip) {
	const f = flip ? -1 : 1;
	const a = curl(xc, y0, ang * f, len, f * k0, f * k1, p, 40);
	const b = curl(xc, y0, 180 + ang * f, len, f * k0, f * k1, p, 40);
	return b.reverse().concat(a.slice(1));
}
function sScroll(xc, y0, o, flip) {
	const pts = sScrollPts(xc, y0, o.ang, o.len, o.k0, o.k1, o.p, flip);
	const e0 = pts[0], e1 = pts[pts.length - 1];
	return el(ribbon(pts, t => o.w1 + (o.w0 - o.w1) * Math.pow(1 - Math.abs(2 * t - 1), 0.7))) + circle(e0[0], e0[1], o.ball) + circle(e1[0], e1[1], o.ball);
}
const FIL = { ang: -33, len: 31, k0: 0.02, k1: 0.34, p: 1.6, w0: 2.8, w1: 0.9, ball: 2, sp: 44 };
function filigreeTopHalf() {
	// alternating S / Z scrolls; neighbouring spirals are bound by small brass collars
	let out = '', collars = '';
	const units = [];
	let i = 0;
	for (let x = 512 - FIL.sp / 2; x > 118; x -= FIL.sp, i++) {
		const flip = i % 2 === 1;
		out += sScroll(x, 68, FIL, flip);
		units.push(sScrollPts(x, 68, FIL.ang, FIL.len, FIL.k0, FIL.k1, FIL.p, flip));
	}
	for (let k = 0; k + 1 < units.length; k++) {
		const a = units[k].reduce((m, p) => p[0] < m[0] ? p : m), b = units[k + 1].reduce((m, p) => p[0] > m[0] ? p : m);
		const cx = (a[0] + b[0]) / 2, cy = (a[1] + b[1]) / 2;
		collars += `<rect x="${R(cx - 2.6)}" y="${R(cy - 4.8)}" width="5.2" height="9.6" rx="1.8"/>`;
	}
	return { out, collars };
}
function band() {
	let s = '';
	s += `<path d="${rectPath(40)} ${winPath(0)}" fill-rule="evenodd" fill="url(#gIronBand)" filter="url(#fGrit)"/>`;
	s += `<path d="${rectPath(51)} ${winPath(-11)}" fill-rule="evenodd" fill="#101317"/>`;
	s += `<g clip-path="url(#cBand)"><path d="${rectPath(51)}" fill="none" stroke="#000" stroke-opacity="0.8" stroke-width="6" filter="url(#fB3)"/></g>`;
	const half = filigreeTopHalf();
	defs.push(`<g id="filTop">${half.out}${mirror(half.out)}</g><g id="filCol">${half.collars}${mirror(half.collars)}</g>`);
	const four = id => [0, 90, 180, 270].map(a => `<use href="#${id}" transform="rotate(${a} 512 512)"/>`).join('');
	s += `<g filter="url(#fSilver)" fill="url(#gSilverD)">${four('filTop')}</g>`;
	s += `<g filter="url(#fBrassSm)" fill="url(#gBrassD)">${four('filCol')}</g>`;
	s += `<path d="${rectPath(40)} ${rectPath(51)}" fill-rule="evenodd" fill="url(#gIron)" filter="url(#fBand)"/>`;
	let pearls = '';
	for (let u = 62; u <= 962; u += 10) pearls += circle(u, 45.5, 3.3) + circle(978.5, u, 3.3) + circle(u, 978.5, 3.3) + circle(45.5, u, 3.3);
	s += `<g fill="url(#gSilverD)" filter="url(#fSilver)">${pearls}</g>`;
	s += `<path d="${winPath(-11)} ${winPath(-2.5)}" fill-rule="evenodd" fill="url(#gIron)" filter="url(#fBand)"/>`;
	s += `<path d="${winPath(-3)} ${winPath(0)}" fill-rule="evenodd" fill="url(#gBrassD)" filter="url(#fBrassSm)"/>`;
	let nails = '';
	for (const u of [214, 318, 706, 810]) nails += circle(u, 90.5, 3.1);
	defs.push(`<g id="lipNails">${nails}</g>`);
	s += `<g fill="url(#gIronL)" filter="url(#fBrassSm)">${four('lipNails')}</g>`;
	s += `<path d="${rectPath(40)} ${winPath(0)}" fill-rule="evenodd" fill="url(#gSweep)"/>`;
	return `<g filter="url(#fGritSoft)">${s}</g>`;
}

// ---------- corner bosses + side bosses ----------
// Spade-like fleur: a spike with two scrolls curling outward, built pointing +x, placed with translate/rotate/scale.
const FLEUR = { L: 60, ws: 6, sx: 0, sy: 5, ang: 60, len: 60, k0: 0.0, k1: 0.26, p: 2.2, w0: 3, wm: 5, w1: 1.1, ball: 2.5 };
function fleur(x, y, rot, sc = 1, o = FLEUR) {
	let s = el(ribbon(bez([0, 0], [o.L * 0.3, 0], [o.L * 0.7, 0], [o.L, 0], 20), t => o.ws * Math.pow(1 - t, 0.9) + 0.5));
	for (const sg of [1, -1]) {
		const pts = curl(o.sx, sg * o.sy, -sg * o.ang, o.len, sg * o.k0, sg * o.k1, o.p, 50);
		const e = pts[pts.length - 1];
		s += el(ribbon(pts, swell(o.w0, o.wm, o.w1))) + circle(e[0], e[1], o.ball);
	}
	return `<g transform="translate(${x} ${y}) rotate(${rot}) scale(${sc})">${s}</g>`;
}
function cornerTL() {
	const c = [68, 68];
	let silver = '', iron = '', brass = '';
	silver += fleur(50, 50, -135, 1.25);
	// iron eight-point plate
	let d = '';
	for (let i = 0; i < 8; i++) {
		const a = i * 45 * D2R, r = i % 2 === 0 ? 44 : 30;
		const p = [c[0] + Math.cos(a) * r, c[1] + Math.sin(a) * r];
		const am = (i * 45 + 22.5) * D2R, cp = [c[0] + Math.cos(am) * 14, c[1] + Math.sin(am) * 14];
		const a2 = (i + 1) * 45 * D2R, r2 = (i + 1) % 2 === 0 ? 44 : 30;
		const p2 = [c[0] + Math.cos(a2) * r2, c[1] + Math.sin(a2) * r2];
		d += (i === 0 ? 'M' + P(p) : '') + 'Q' + P(cp) + ' ' + P(p2);
	}
	iron += el(d + 'Z');
	iron += circle(c[0], c[1], 19);
	silver += `<path d="${ringPath(c[0], c[1], 14, 17.5)}" fill-rule="evenodd"/>`;
	const studs = [[c[0] + 30, c[1]], [c[0] - 30, c[1]], [c[0], c[1] + 30], [c[0], c[1] - 30]].map(p => circle(p[0], p[1], 3.2)).join('');
	brass += circle(c[0], c[1], 10);
	return { silver, iron, brass, studs };
}
function sideBossL(y) {
	// small boss on the left band with a silver fleur pointing out of the frame
	const c = [64, y];
	let silver = '', iron = '', brass = '';
	silver += fleur(52, y, 180, 0.95);
	iron += `<ellipse cx="${c[0]}" cy="${c[1]}" rx="17" ry="25"/>`;
	silver += `<path d="M${c[0] - 10} ${c[1]}A10 15 0 1 0 ${c[0] + 10} ${c[1]}A10 15 0 1 0 ${c[0] - 10} ${c[1]}ZM${c[0] - 7.5} ${c[1]}A7.5 12 0 1 0 ${c[0] + 7.5} ${c[1]}A7.5 12 0 1 0 ${c[0] - 7.5} ${c[1]}Z" fill-rule="evenodd"/>`;
	brass += circle(c[0], c[1], 6.5);
	return { silver, iron, brass };
}
function corners() {
	const k = cornerTL();
	const sb = sideBossL(690);
	defs.push(`<g id="cnIron">${k.iron}</g><g id="cnSilver">${k.silver}</g><g id="cnBrass">${k.brass}${k.studs}</g>`);
	const rot = id => [0, 90, 180, 270].map(a => `<use href="#${id}" transform="rotate(${a} 512 512)"/>`).join('');
	return `<g filter="url(#fSilverBig)" fill="url(#gSilverD)">${rot('cnSilver')}${mirD(sb.silver)}</g>` +
		`<g filter="url(#fIronBig)" fill="url(#gIronL)">${rot('cnIron')}${mirD(sb.iron)}</g>` +
		`<g filter="url(#fBrass)" fill="url(#gBrassD)">${rot('cnBrass')}${mirD(sb.brass)}</g>`;
}

// ---------- cyan line + inner shadow ----------
function innerShadow() {
	return `<g clip-path="url(#cWin)"><path d="${winPath(0)}" fill="none" stroke="#000" stroke-opacity="0.75" stroke-width="34" filter="url(#fB10)"/>` +
		`<path d="${winPath(0)}" fill="none" stroke="#000" stroke-opacity="0.6" stroke-width="8" filter="url(#fB3)"/></g>`;
}
function cyanLine() {
	return `<path d="${winPath(5)}" fill="none" stroke="#6ee7ff" stroke-width="5" stroke-opacity="0.35" filter="url(#fB3)"/>` +
		`<path d="${winPath(5)}" fill="none" stroke="#6ee7ff" stroke-width="1.6" filter="url(#fCyan)"/>` +
		`<path d="${winPath(5)}" fill="none" stroke="#e6fbff" stroke-width="0.6" stroke-opacity="0.8"/>`;
}

// ---------- sun-ring (top centre) ----------
const SC = [512, 72];
const hexmix = (a, b, t) => { const h = x => [parseInt(x.slice(1, 3), 16), parseInt(x.slice(3, 5), 16), parseInt(x.slice(5, 7), 16)]; const A = h(a), B = h(b); return '#' + A.map((v, i) => Math.round(v + (B[i] - v) * t).toString(16).padStart(2, '0')).join(''); };
function sunRing() {
	let s = '';
	s += circle(SC[0], SC[1], 120, 'fill="url(#gHalo)" mask="url(#mGlow)"');
	const Ldir = [-0.7071, -0.7071];
	let wavy = '', rays = '';
	for (let i = 0; i < 12; i++) {
		const a = i * 30 * D2R;
		const dir = [Math.sin(a), -Math.cos(a)], per = [Math.cos(a), Math.sin(a)];
		if (i % 2 === 1) {
			// flame ray
			const pts = [];
			for (let k = 0; k <= 16; k++) { const t = k / 16, r = 44 + 22 * t, o = 3.4 * Math.sin(Math.PI * 1.7 * t) * (0.4 + t); pts.push([SC[0] + dir[0] * r + per[0] * o, SC[1] + dir[1] * r + per[1] * o]); }
			wavy += el(ribbon(pts, t => 7.2 * Math.pow(1 - t, 0.85) + 0.4));
		} else {
			const rt = 70, hw = 10, rb = 46;
			const B = [SC[0] + dir[0] * rb, SC[1] + dir[1] * rb], T = [SC[0] + dir[0] * rt, SC[1] + dir[1] * rt];
			const Lb = [B[0] - per[0] * hw, B[1] - per[1] * hw], Rb = [B[0] + per[0] * hw, B[1] + per[1] * hw];
			const nl = -per[0] * Ldir[0] - per[1] * Ldir[1], nr = -nl;
			const col = n => n > 0 ? hexmix('#c8962f', '#ffe7a8', n * 0.95) : hexmix('#c8962f', '#5e3e10', -n * 0.9);
			const up = -dir[1]; // rays facing up catch more light
			rays += `<path d="${poly([Lb, T, B])}" fill="${col(nl * 0.8 + up * 0.2)}"/><path d="${poly([B, T, Rb])}" fill="${col(nr * 0.8 + up * 0.2)}"/>`;
			rays += `<path d="M${P(B)}L${P(T)}" stroke="#3a2608" stroke-width="0.6" stroke-opacity="0.5"/>`;
		}
	}
	s += `<g fill="url(#gBrassD)" filter="url(#fBrassSm)">${wavy}</g>`;
	s += `<g filter="url(#fDrop)">${rays}</g>`;
	s += `<g filter="url(#fBrass)"><path d="${ringPath(SC[0], SC[1], 32, 50)}" fill-rule="evenodd" fill="url(#gBrass)"/></g>`;
	s += circle(SC[0], SC[1], 46.5, `fill="none" stroke="#4a3008" stroke-width="1.2" stroke-opacity="0.7"`);
	s += circle(SC[0], SC[1], 36, `fill="none" stroke="#4a3008" stroke-width="1.2" stroke-opacity="0.7"`);
	let studs = '';
	for (let i = 0; i < 12; i++) { const a = (i * 30 + 15) * D2R; studs += circle(SC[0] + Math.sin(a) * 41.3, SC[1] - Math.cos(a) * 41.3, 2.8); }
	s += `<g fill="url(#gBrass)" filter="url(#fBrassSm)">${studs}</g>`;
	// inner disc + ember
	s += circle(SC[0], SC[1], 32, `fill="url(#gDiscIron)"`);
	s += circle(SC[0], SC[1], 32, `fill="none" stroke="${INK}" stroke-width="1.6"`);
	s += circle(SC[0], SC[1] + 1, 30, `fill="url(#gEmberGlow)"`);
	s += circle(SC[0], SC[1], 31, `fill="none" stroke="#ff9f1c" stroke-width="2" stroke-opacity="0.55" filter="url(#fB1)"`);
	const ember = [];
	const er = [12.5, 10.5, 13.5, 11, 13, 10, 12];
	for (let i = 0; i < 7; i++) { const a = (i / 7) * Math.PI * 2 + 0.3; ember.push([SC[0] + Math.cos(a) * er[i], SC[1] + 1 + Math.sin(a) * er[i] * 0.95]); }
	s += circle(SC[0], SC[1] + 1, 15, 'fill="#ff9f1c" fill-opacity="0.8" filter="url(#fB6)"');
	s += `<path d="${poly(ember)}" fill="url(#gEmber)" filter="url(#fGlow4)"/>`;
	s += `<path d="${poly(ember)}" fill="none" stroke="#6a2406" stroke-width="0.9" stroke-opacity="0.7" stroke-linejoin="round"/>`;
	s += `<path d="M${SC[0] - 6} ${SC[1] - 2}l4 3.5l5 -2.5l2 5M${SC[0] + 3} ${SC[1] - 6}l1 4" fill="none" stroke="#b8420a" stroke-width="1" stroke-opacity="0.8"/>`;
	s += circle(SC[0] - 1, SC[1], 3.5, 'fill="#ffffff" filter="url(#fGlow2)"');
	return s;
}

// ---------- top scrollwork + feathers ----------
const QUILL = [364, 57], FANG = 187;
function scrollLeft() {
	const p = [];
	const add = (pts, wf, ball = 0) => { p.push(el(ribbon(pts, wf))); if (ball) { const e = pts[pts.length - 1]; p.push(circle(e[0], e[1], ball)); } };
	add(curl(478, 48, 190, 96, 0.004, 0.24, 2.2, 60), swell(2.2, 4.6, 1.5), 3.2);           // upper volute above the band
	add(curl(478, 92, 168, 82, -0.004, -0.22, 2.2, 50), swell(2.2, 4.2, 1.4), 3);           // lower volute
	add(resample(bez([486, 68], [444, 78], [404, 50], [QUILL[0] + 4, QUILL[1]], 40), 40), swell(3, 4.4, 3.2)); // arm to the quill
	add(curl(436, 70, 120, 34, -0.03, -0.3, 1.6, 30), swell(1.2, 2.8, 0.9), 2.2);
	add(curl(404, 56, 230, 30, 0.03, 0.32, 1.6, 26), swell(1.2, 2.6, 0.9), 2);
	return p.join('');
}
function feather() {
	// local: quill at 0, tip at +x; +y is the wide vane
	const Lf = 238;
	const bend = x => -14 * Math.pow(x / Lf, 2);
	const W = 28, Wn = 15;
	const prof = (x, w) => { const t = (x - 26) / (Lf - 26); if (t <= 0) return 1.5; const rise = Math.sin(Math.min(1, t / 0.28) * Math.PI / 2); const fall = t > 0.6 ? Math.pow(Math.max(0, 1 - (t - 0.6) / 0.4), 0.55) : 1; return Math.max(0.5, w * rise * fall); };
	const lower = [], upper = [];
	for (let x = 26; x <= Lf; x += 6) lower.push([x, prof(x, W)]);
	for (let x = Lf; x >= 26; x -= 6) upper.push([x, -prof(x, Wn)]);
	lower.push([Lf, 0]);
	const notch = (arr, x, depth, wdt = 4) => { for (let i = 0; i < arr.length - 1; i++) if ((arr[i][0] - x) * (arr[i + 1][0] - x) <= 0) { const y = arr[i][1]; arr.splice(i + 1, 0, [x - 1, y * 0.98], [x + wdt * 0.6, y * (1 - depth)], [x + wdt, y * 0.97]); return; } };
	notch(lower, 96, 0.5, 5); notch(lower, 160, 0.42, 5); notch(upper, 128, 0.5, -4); notch(lower, 200, 0.3, 4);
	const outline = lower.concat(upper).map(p => [p[0], p[1] + bend(p[0])]);
	const od = smooth(outline, true);
	defs.push(lin('gFeath', [[0, '#07080a'], [0.58, '#0d0f12'], [0.68, '#1c1e21'], [0.8, '#56575a'], [0.9, '#8f8f8a'], [1, '#b4b2aa']], 26, 0, Lf, 0, true));
	defs.push(lin('gFeathSheen', [[0, '#6a86a8', 0], [0.4, '#6a86a8', 0.5], [0.7, '#4a5c78', 0.25], [1, '#4a5c78', 0]], 0, 2, 0, 24, true));
	defs.push(lin('gFeathFade', [[0, '#000', 0], [0.12, '#fff', 1], [0.72, '#fff', 1], [0.86, '#000', 0]], 26, 0, Lf, 0, true));
	defs.push(`<mask id="mFeathFade" maskUnits="userSpaceOnUse" x="-10" y="-40" width="260" height="80"><rect x="-10" y="-40" width="260" height="80" fill="url(#gFeathFade)"/></mask>`);
	defs.push(`<clipPath id="cFeath"><path d="${od}"/></clipPath>`);
	let s = '';
	// down fluff at the base
	let fluff = '';
	for (let i = 0; i < 9; i++) { const x = 8 + i * 2.2; const sg = i % 2 ? 1 : -1; fluff += `<path d="M${x} ${R(bend(x))}q${4 + i * 0.8} ${sg * (6 + i)} ${-3 + i} ${sg * (12 + i * 0.6)}"/>`; }
	defs.push(`<g id="featherFluff" fill="none" stroke="#8a8a86" stroke-width="0.6" stroke-opacity="0.6" stroke-linecap="round">${fluff}</g>`);
	s += `<path d="${od}" fill="url(#gFeath)"/>`;
	// iridescent sheen on both vanes (fades out before the ash tip)
	s += `<g clip-path="url(#cFeath)" mask="url(#mFeathFade)"><path d="M20 ${R(bend(100) + 8)}C80 4 160 8 230 ${R(bend(230) + 4)}" fill="none" stroke="#5a7898" stroke-width="9" stroke-opacity="0.45" filter="url(#fB3)"/>` +
		`<path d="M30 -5C90 -9 160 -6 220 ${R(bend(220) - 4)}" fill="none" stroke="#6a86a8" stroke-width="5" stroke-opacity="0.35" filter="url(#fB2)"/></g>`;
	// barbs
	let barbs = '', ash = '';
	for (let x = 28; x < Lf - 4; x += 3.2) {
		const wl = prof(x + 10, W), wu = prof(x + 9, Wn);
		const a = `M${R(x)} ${R(bend(x) + 0.6)}Q${R(x + 5)} ${R(bend(x) + wl * 0.45)} ${R(x + 12)} ${R(bend(x + 12) + wl * 1.02)}` +
			`M${R(x)} ${R(bend(x) - 0.6)}Q${R(x + 4)} ${R(bend(x) - wu * 0.45)} ${R(x + 10)} ${R(bend(x + 10) - wu * 1.02)}`;
		if (x > Lf * 0.66) ash += a; else barbs += a;
	}
	s += `<g fill="none" clip-path="url(#cFeath)"><path d="${barbs}" stroke="#2c3644" stroke-width="0.7" stroke-opacity="0.9"/><path d="${ash}" stroke="#d6d1c4" stroke-width="0.7" stroke-opacity="0.5"/></g>`;
	// rachis + calamus
	const shaft = [];
	for (let x = 0; x <= Lf - 3; x += 6) shaft.push([x, bend(x)]);
	defs.push(lin('gRachis', [[0, '#c9c4b8'], [0.1, '#6a6c70'], [0.22, '#1e2228'], [0.7, '#24282e'], [0.86, '#6a6a68'], [1, '#a8a6a0']], 0, 0, Lf, 0, true));
	s += `<path d="${ribbon(shaft, t => 2.6 * (1 - t) + 0.4)}" fill="url(#gRachis)"/>`;
	s += `<path d="${smooth(shaft.slice(0, 30))}" fill="none" stroke="#8a9aae" stroke-width="0.6" stroke-opacity="0.55" transform="translate(0 -0.7)"/>`;
	return s;
}
function topCrest() {
	let s = '';
	s += `<g filter="url(#fIron)" fill="url(#gIronL)">${mirD(scrollLeft())}</g>`;
	const f = feather();
	defs.push(`<g id="featherL">${f}</g>`);
	const one = `<g transform="translate(${QUILL[0]} ${QUILL[1]}) rotate(${FANG}) scale(1 -1)"><use href="#featherL"/></g>`;
	const fl = `<g transform="translate(${QUILL[0]} ${QUILL[1]}) rotate(${FANG}) scale(1 -1)"><use href="#featherFluff"/></g>`;
	s += mirD(fl) + `<g filter="url(#fFeather)">${mirD(one)}</g>`;
	const collar = `<g transform="translate(${QUILL[0]} ${QUILL[1]}) rotate(${FANG})"><rect x="-6" y="-8.5" width="13" height="17" rx="3"/><rect x="9" y="-6" width="5" height="12" rx="2"/></g>`;
	s += `<g fill="url(#gBrass)" filter="url(#fBrassSm)">${mirD(collar)}</g>`;
	s += sunRing();
	return s;
}

// ---------- the side path: Ellis's bandolier hangs on it, Odessa's sash follows it ----------
const BCL = resample(bez([80, 184], [38, 252], [30, 396], [70, 482], 80), 70);
const BT = tangents(BCL);
const BTS = [0.2, 0.33, 0.46, 0.59, 0.72];

// ---------- ledger (lower right) ----------
function ledger() {
	let s = '';
	const cx = 938, cy = 948, rot = -8;
	let b = '';
	b += `<rect x="-64" y="-42" width="132" height="98" rx="5" fill="#2e110e"/>`;
	b += `<path d="M-60 -38H64V50Q2 54 -60 50Z" fill="url(#gPages)"/>`;
	let pl = '';
	for (let i = 0; i < 6; i++) pl += `M-58 ${44 + i * 1.3}Q2 ${48 + i * 1.3} 62 ${44 + i * 1.3}`;
	b += `<path d="${pl}" fill="none" stroke="#8a7a5e" stroke-width="0.6" stroke-opacity="0.7"/>`;
	b += `<g filter="url(#fLeather)"><rect x="-66" y="-50" width="132" height="94" rx="5" fill="url(#gLeather)" filter="url(#fGrit)"/></g>`;
	b += `<rect x="-66" y="-50" width="14" height="94" rx="4" fill="#4a1a16"/>`;
	b += `<path d="${[-34, -14, 6, 26].map(y => `M-65 ${y}H-53`).join('')}" stroke="#2a0e0c" stroke-width="2.2"/>`;
	b += `<path d="${[-35.5, -15.5, 4.5, 24.5].map(y => `M-64 ${y}H-54`).join('')}" stroke="#a8603e" stroke-width="0.8" stroke-opacity="0.6"/>`;
	b += `<rect x="-44" y="-40" width="100" height="74" rx="2" fill="none" stroke="#c8962f" stroke-opacity="0.55" stroke-width="1.2"/>`;
	b += `<rect x="-40" y="-36" width="92" height="66" rx="1.5" fill="none" stroke="#c8962f" stroke-opacity="0.35" stroke-width="0.8"/>`;
	let cn = `<path d="M44 -50H61Q66 -50 66 -45V-28Q62 -44 44 -50Z"/><path d="M44 44H61Q66 44 66 39V22Q62 38 44 44Z"/>` +
		`<path d="M-52 -50H-34Q-48 -46 -52 -32Z"/><path d="M-52 44H-34Q-48 40 -52 26Z"/>` +
		`<path d="M2 -20H24V-16L16 -3L24 10V14H2V10L10 -3L2 -16Z"/>` +
		`<rect x="58" y="-12" width="14" height="18" rx="3"/>`;
	b += `<g fill="url(#gBrassD)" filter="url(#fBrassSm)">${cn}</g>`;
	b += `<path d="M7 -15H19L13 -6ZM7 9H19L13 1Z" fill="#3a2608" opacity="0.75"/>`;
	b += circle(65, -3, 2.2, 'fill="#3a2608"');
	// ribbon tail leaving the pages at the lower left
	s += `<g transform="translate(${cx} ${cy}) rotate(${rot})" filter="url(#fDrop)">${b}</g>`;
	const rib = resample(bez([914, 1002], [904, 1020], [878, 1016], [867, 996], 30), 30);
	s += `<g filter="url(#fLeatherSm)"><path d="${ribbon(rib, t => 3.6 - t * 0.8)}" fill="#b8862b"/></g>`;
	s += `<path d="${smooth(rib.map(p => [p[0] + 0.8, p[1] - 0.8]))}" fill="none" stroke="#ffe7a8" stroke-width="0.8" stroke-opacity="0.45"/>`;
	const cc = [862, 1004];
	let coin = circle(cc[0], cc[1], 13, 'fill="url(#gCoin)"');
	coin += circle(cc[0], cc[1], 10.5, 'fill="none" stroke="#3a3c40" stroke-width="1" stroke-opacity="0.8"');
	const star = [];
	for (let i = 0; i < 10; i++) { const a = (i * 36 - 90) * D2R, r = i % 2 ? 2.8 : 6.5; star.push([cc[0] + Math.cos(a) * r, cc[1] + 1 + Math.sin(a) * r]); }
	coin += `<path d="${poly(star)}" fill="none" stroke="#3a3c40" stroke-width="1"/>`;
	coin += circle(cc[0] + 5, cc[1] - 8.5, 1.8, 'fill="#1a1c1e"');
	s += `<g filter="url(#fDropSm)">${coin}</g>`;
	return s;
}

// ---------- helpers the heroes' kits share ----------
const SERIF = `font-family="Caladea, 'DejaVu Serif', FreeSerif, serif" font-weight="bold" text-anchor="middle"`;
// Lettering cut into a plate: a light edge under the dark cut, as on Ellis's plank.
function cutText(t, x, y, sz, ls, light, dark) {
	return `<text x="${x}" y="${R(y + 1.3)}" ${SERIF} font-size="${sz}" letter-spacing="${ls}" fill="${light}" fill-opacity="0.55">${t}</text>` +
		`<text x="${x}" y="${y}" ${SERIF} font-size="${sz}" letter-spacing="${ls}" fill="${dark}">${t}</text>`;
}
// Raised lettering (gilt, brass): a dark shadow under the face.
function raisedText(t, x, y, sz, ls, face, shadow) {
	return `<text x="${x}" y="${R(y + 1.6)}" ${SERIF} font-size="${sz}" letter-spacing="${ls}" fill="${shadow}">${t}</text>` +
		`<text x="${x}" y="${y}" ${SERIF} font-size="${sz}" letter-spacing="${ls}" fill="${face}">${t}</text>`;
}
// Ellis's lantern sparks, in the hero's colour (the contract's paint sparks).
const SPARKS = [[98, 816, 1.8], [110, 788, 1.2], [86, 770, 1.5], [124, 830, 1.1], [150, 986, 1.3], [250, 1003, 1.1], [372, 999, 1.4], [652, 1006, 1.2], [770, 1000, 1.5], [540, 14, 1.3], [486, 10, 1.1], [528, 4, 0.9], [36, 756, 1.0], [114, 742, 0.9]];
function sparks(col) {
	return `<g fill="${col}" filter="url(#fGlow2)">${SPARKS.map(p => circle(p[0], p[1], p[2] + 0.4)).join('')}</g>` +
		`<g fill="#fffbe6">${SPARKS.map(p => circle(p[0], p[1], p[2] * 0.55)).join('')}</g>`;
}
// Local art across a side path at t (0..1): local +x points out of the frame, +y along the path.
function onPath(C, T, t, inner) {
	const i = Math.round(t * (C.length - 1)), n = [-T[i][1], T[i][0]];
	return `<g transform="translate(${R(C[i][0])} ${R(C[i][1])}) rotate(${R(Math.atan2(n[1], n[0]) / D2R)})">${inner}</g>`;
}
const at = (x, y, rot, c) => `<g transform="translate(${R(x)} ${R(y)})${rot ? ` rotate(${R(rot)})` : ''}">${c}</g>`;
// A pointed piece lying from its head (hx,hy) to its tip (tx,ty): the crossed pieces lie on the revolvers' line.
const lay = (hx, hy, tx, ty) => ({ tf: `translate(${hx} ${hy}) rotate(${R(Math.atan2(ty - hy, tx - hx) / D2R)})`, len: Math.hypot(tx - hx, ty - hy) });

module.exports = { el, sym, mirD, WA, WB, WN, winPath, rectPath, defs, band, corners, innerShadow, cyanLine, sunRing, topCrest, ledger,
	BCL, BT, BTS, SERIF, cutText, raisedText, SPARKS, sparks, onPath, at, lay };
