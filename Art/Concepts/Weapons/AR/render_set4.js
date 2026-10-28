// Set 4 (Meridian: clean, precise) concept views of the Bullpup AR's parts, for Tripo's multi-view image-to-3D (2026-10-27).
//   node Art/Concepts/Weapons/AR/render_set4.js [Folder...]
// Six new designs (not set 1 recoloured): a long streamlined two-tone shell, a fluted marksman barrel, a slim
// compensator, a short 20-round magazine, a compact enclosed red dot, an adjustable cheek-riser stock. Only the
// interfaces match set 1 (Bullpup.py): barrel-nut face (51.9, 0) r 1.25, butt plate and its boss (0, -3.3), magazine
// well (14, -5.6) 7.2 cm long, rail top 6.15 over u 14-32, grip at (22.5, -10). Gun space, cm: u along the gun from the
// back of the butt, v up from the bore, w to the gun's left. Writes <Folder>/upload/ (transparent) and upload_white/.
// Views: front = from the muzzle end, back = from the butt end, left = the gun's left side (muzzle to the image's left),
// right = its right side (muzzle to the right).
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const ROOT = path.resolve(__dirname, '../../../..');
const FONT = p => 'file:///' + path.join(ROOT, p).replace(/\\/g, '/');
// --upload: clean Tripo images only (upload/ transparent, upload_white/ on white): the part alone, filling ~84%, no
// contact shadows or glow spill past its edge, no background-coloured touches; the grey views and sheets are left alone.
const UPLOAD = true;   // set 4 makes Tripo upload images only
const N = 1024, FILL = UPLOAD ? 0.84 : 0.8, BG = '#d9d9d9';

// ---- geometry helpers (cm in, px out) -----------------------------------------------------------------------------
function makeView(kind, b) {
	const ext = Math.max(b.u[1] - b.u[0], b.v[1] - b.v[0], b.w[1] - b.w[0]);
	const s = N * FILL / ext;
	const uc = (b.u[0] + b.u[1]) / 2, vc = (b.v[0] + b.v[1]) / 2, wc = (b.w[0] + b.w[1]) / 2;
	const X = { right: a => N / 2 + (a - uc) * s, left: a => N / 2 - (a - uc) * s,
		front: a => N / 2 + (a - wc) * s, back: a => N / 2 - (a - wc) * s }[kind];
	const Y = v => N / 2 - (v - vc) * s;
	return { kind, s, X, Y, P: (a, v) => [X(a), Y(v)], side: kind === 'left' || kind === 'right' };
}
const r2 = n => Math.round(n * 100) / 100;
function rounded(P, r) {
	if (!r) return 'M' + P.map(p => p.map(r2).join(',')).join('L') + 'Z';
	let d = '';
	const n = P.length;
	for (let i = 0; i < n; i++) {
		const p0 = P[(i - 1 + n) % n], p1 = P[i], p2 = P[(i + 1) % n];
		const d1 = Math.hypot(p0[0] - p1[0], p0[1] - p1[1]), d2 = Math.hypot(p2[0] - p1[0], p2[1] - p1[1]);
		const rr = Math.min(r, d1 / 2, d2 / 2);
		const a = [p1[0] + (p0[0] - p1[0]) * rr / d1, p1[1] + (p0[1] - p1[1]) * rr / d1];
		const c = [p1[0] + (p2[0] - p1[0]) * rr / d2, p1[1] + (p2[1] - p1[1]) * rr / d2];
		d += (i ? 'L' : 'M') + a.map(r2).join(',') + 'Q' + p1.map(r2).join(',') + ' ' + c.map(r2).join(',');
	}
	return d + 'Z';
}
const pth = (V, pts, r = 0) => rounded(pts.map(([a, v]) => V.P(a, v)), r * V.s);
const R4 = (a0, v0, a1, v1) => [[a0, v0], [a1, v0], [a1, v1], [a0, v1]];
const F = (filt) => (filt ? ` filter="url(#${filt})"` : '');
const shp = (d, fill, filt, extra = '') => `<path d="${d}" fill="${fill}"${F(filt)} ${extra}/>`;
const circ = (V, a, v, r, fill, filt, extra = '') =>
	`<circle cx="${r2(V.X(a))}" cy="${r2(V.Y(v))}" r="${r2(r * V.s)}" fill="${fill}"${F(filt)} ${extra}/>`;
const ring = (V, a, v, r, col, px, extra = '') => circ(V, a, v, r, 'none', null, `stroke="${col}" stroke-width="${px}" ${extra}`);
const pl = (V, pts, col, px, extra = '') =>
	`<path d="M${pts.map(([a, v]) => V.P(a, v).map(r2).join(',')).join('L')}" fill="none" stroke="${col}" stroke-width="${px}" stroke-linecap="round" stroke-linejoin="round" ${extra}/>`;
// Edge wear: a broken light line along an outline; grime: a soft dark one.
// In upload images it is clipped to its own outline, so nothing pokes past the part's edge.
let wearId = 0;
const wear = (d, col = '#fbf6ea', px = 1.2, op = 0.55, dash = '16 38 6 64 26 52 4 30') => {
	const line = `<path d="${d}" fill="none" stroke="${col}" stroke-width="${UPLOAD ? px * 2 : px}" stroke-dasharray="${dash}" opacity="${op}"`;
	if (!UPLOAD) return line + '/>';
	const id = 'wc' + wearId++;
	return `<clipPath id="${id}"><path d="${d}"/></clipPath>${line} clip-path="url(#${id})"/>`;
};
function bez(c, t) {
	const [[a, b], [p, q], [e, f]] = c;
	return [(1 - t) ** 2 * a + 2 * (1 - t) * t * p + t * t * e, (1 - t) ** 2 * b + 2 * (1 - t) * t * q + t * t * f];
}
function edgeU(c, v) {
	let prev = bez(c, 0);
	for (let i = 1; i <= 80; i++) {
		const p = bez(c, i / 80);
		if ((prev[1] - v) * (p[1] - v) <= 0 && prev[1] !== p[1]) return prev[0] + (p[0] - prev[0]) * (v - prev[1]) / (p[1] - prev[1]);
		prev = p;
	}
	return prev[0];
}
const star = (cx, cy, ro, ri, n = 5, rot = -90) => Array.from({ length: n * 2 }, (_, i) => {
	const a = (rot + i * 180 / n) * Math.PI / 180, r = i % 2 ? ri : ro;
	return [cx + r * Math.cos(a), cy + r * Math.sin(a)];
});

// ---- materials: gradients and lighting filters ----------------------------------------------------------------------
const lin = (id, stops, vert = true) => `<linearGradient id="${id}" x1="0" y1="0" x2="${vert ? 0 : 1}" y2="${vert ? 1 : 0}">${stops
	.map(([o, c, a]) => `<stop offset="${o}" stop-color="${c}"${a !== undefined ? ` stop-opacity="${a}"` : ''}/>`).join('')}</linearGradient>`;
const rad = (id, stops, cx = 0.38, cy = 0.34, r = 0.75) => `<radialGradient id="${id}" cx="${cx}" cy="${cy}" r="${r}">${stops
	.map(([o, c, a]) => `<stop offset="${o}" stop-color="${c}"${a !== undefined ? ` stop-opacity="${a}"` : ''}/>`).join('')}</radialGradient>`;
const GRADS = [
	lin('sand', [[0, '#f2e9d2'], [0.45, '#e6d8b6'], [1, '#cfbf98']]),
	lin('form', [[0, '#ffffff', 0.32], [0.12, '#ffffff', 0.08], [0.45, '#000000', 0], [0.85, '#3a2a10', 0.12], [1, '#3a2a10', 0.22]]),
	lin('sandHi', [[0, '#f9f3e4'], [1, '#ebdfc2']]),
	lin('sandMid', [[0, '#e6d9b8'], [1, '#d8c9a4']]),
	lin('sandLo', [[0, '#d2c29c'], [1, '#b3a17a']]),
	lin('sandLoop', [[0, '#e0d2ae'], [0.7, '#c8b890'], [1, '#a99872']]),
	lin('blued', [[0, '#5d6570'], [0.5, '#3f454e'], [1, '#2b2f35']]),
	lin('black', [[0, '#505257'], [0.5, '#323337'], [1, '#202124']]),
	lin('bluedCyl', [[0, '#1f2328'], [0.2, '#4b535d'], [0.31, '#8d96a0'], [0.42, '#4e5660'], [0.75, '#2c3137'], [1, '#181b1f']]),
	lin('blackCyl', [[0, '#17181a'], [0.2, '#46484d'], [0.31, '#82858b'], [0.42, '#3f4145'], [0.75, '#252629'], [1, '#121314']]),
	lin('blackCylH', [[0, '#17181a'], [0.2, '#46484d'], [0.31, '#82858b'], [0.42, '#3f4145'], [0.75, '#252629'], [1, '#121314']], false),
	lin('brassCyl', [[0, '#6a4813'], [0.18, '#c4913b'], [0.3, '#fff1c2'], [0.42, '#e3b35c'], [0.75, '#a3762b'], [1, '#5f400f']]),
	lin('brass', [[0, '#f7dc9c'], [0.5, '#e0ad52'], [1, '#a5772b']]),
	lin('steelCyl', [[0, '#45484d'], [0.22, '#959aa1'], [0.32, '#e6eaee'], [0.45, '#8e9399'], [1, '#4d5156']]),
	lin('rubber', [[0, '#414141'], [0.5, '#2c2c2c'], [1, '#1d1d1d']]),
	lin('poly', [[0, '#636468'], [0.45, '#4a4b4f'], [1, '#36373a']]),
	lin('polyUp', [[0, '#3a3b3e'], [1, '#56575b']]),
	lin('polyDown', [[0, '#525357'], [1, '#333436']]),
	lin('emberLin', [[0, '#ffe08a'], [0.55, '#ff8a2e'], [1, '#a8400f']]),
	lin('temper', [[0, '#5b4f86', 0], [0.3, '#5b4f86', 0.55], [0.6, '#b08a4c', 0.5], [1, '#b08a4c', 0]], false),
	rad('rBlack', [[0, '#80848a'], [0.45, '#3b3d41'], [1, '#18191b']]),
	rad('rBlued', [[0, '#8a939e'], [0.45, '#434a53'], [1, '#1d2125']]),
	rad('rBrass', [[0, '#fff3c8'], [0.4, '#e6b45c'], [1, '#93661f']]),
	rad('rSteel', [[0, '#f0f3f6'], [0.45, '#999ea5'], [1, '#4b4f54']]),
	rad('rRubber', [[0, '#4a4a4a'], [0.5, '#2b2b2b'], [1, '#1a1a1a']]),
	rad('ember', [[0, '#fff6d2'], [0.35, '#ffc25c'], [0.75, '#ff7a24'], [1, '#b8420f']], 0.5, 0.5, 0.6),
	rad('cyan', [[0, '#f2feff'], [0.35, '#8be9ff'], [0.8, '#36c4f0'], [1, '#127aa0']], 0.45, 0.42, 0.6),
	rad('heat', [[0, '#6b4424', 0.32], [1, '#6b4424', 0]], 0.5, 0.5, 0.5),
	rad('bore', [[0, '#050506'], [0.75, '#121315'], [1, '#2a2c30']], 0.5, 0.5, 0.5),
	// set 2: the makers' looks (Docs/Work/2026-10-22-Next/MAKERS.md)
	lin('teal', [[0, '#79b0aa'], [0.45, '#5a948e'], [1, '#3f716c']]),                       // Windfall sea-teal paint
	lin('tealHi', [[0, '#8cc0ba'], [1, '#6aa49e']]),
	lin('tealLo', [[0, '#55898a'], [1, '#3a6662']]),
	lin('rust', [[0, '#b2683f'], [0.5, '#9a5534'], [1, '#6e3a22']]),                        // primer rust
	lin('steelPlate', [[0, '#9a9da1'], [0.5, '#7a7d81'], [1, '#55585c']]),
	lin('tape', [[0, '#a08a6a'], [0.5, '#8a765a'], [1, '#6a5a44']]),
	lin('copperCyl', [[0, '#6e3418'], [0.25, '#c9794a'], [0.38, '#ffd0a8'], [0.55, '#c27045'], [1, '#5e2c14']]),
	lin('navy', [[0, '#3d5373'], [0.5, '#2b3c55'], [1, '#1b2638']]),                       // Meridian blued navy
	lin('navyCyl', [[0, '#141c29'], [0.2, '#34496a'], [0.31, '#7d95b8'], [0.42, '#3a5072'], [0.75, '#22304a'], [1, '#111824']]),
	lin('nickel', [[0, '#d5dade'], [0.5, '#a9aeb3'], [1, '#7c8186']]),
	lin('nickelCyl', [[0, '#5f6368'], [0.22, '#b4b9be'], [0.32, '#f4f7f9'], [0.45, '#aeb3b8'], [1, '#62666b']]),
	lin('ironCyl', [[0, '#141516'], [0.2, '#3f3d3b'], [0.31, '#76726d'], [0.42, '#3b3936'], [0.75, '#232120'], [1, '#100f0f']]),
	lin('ironCylH', [[0, '#141516'], [0.2, '#3f3d3b'], [0.31, '#76726d'], [0.42, '#3b3936'], [0.75, '#232120'], [1, '#100f0f']], false),
	lin('oxblood', [[0, '#9a4434'], [0.5, '#7d2f24'], [1, '#5a1f17']]),                    // Roundhouse freight-car red
	lin('oxCylH', [[0, '#3e140f'], [0.22, '#7d2f24'], [0.33, '#b4584a'], [0.48, '#7d2f24'], [1, '#3a120d']], false),
	lin('brassCylH', [[0, '#6a4813'], [0.18, '#c4913b'], [0.3, '#fff1c2'], [0.42, '#e3b35c'], [0.75, '#a3762b'], [1, '#5f400f']], false),
	lin('gilt', [[0, '#fff1c4'], [0.45, '#f0d184'], [1, '#b08a3a']]),                       // Saint Maud gilt brass
	lin('leather', [[0, '#4e382c'], [0.5, '#3a2a22'], [1, '#261a14']]),
	lin('emberCyl', [[0, '#8a300c'], [0.3, '#ff8a2e'], [0.5, '#ffe9a8'], [0.7, '#ff8a2e'], [1, '#8a300c']]),
	rad('rOx', [[0, '#b4584a'], [0.5, '#7d2f24'], [1, '#4a1812']]),
	rad('rNickel', [[0, '#ffffff'], [0.45, '#b4b9be'], [1, '#62666b']]),
	rad('rNavy', [[0, '#6a82a6'], [0.45, '#2b3c55'], [1, '#121a28']]),
	rad('rGilt', [[0, '#fffbe6'], [0.45, '#f0d184'], [1, '#9c7a2e']]),
	rad('rIron', [[0, '#7a7570'], [0.45, '#3a3633'], [1, '#161413']]),
	rad('lens', [[0, '#7fd8ff'], [0.3, '#2d5d7c'], [0.7, '#162a3a'], [1, '#0a1218']], 0.36, 0.32, 0.8),
	lin('oxHi', [[0, '#b4584a'], [1, '#8e3a2a']]),
	lin('oxMid', [[0, '#8e3a2a'], [1, '#7a2c22']]),
	lin('oxLo', [[0, '#6e271e'], [1, '#4f1a14']]),
	lin('oxLoop', [[0, '#8a3628'], [0.7, '#6a2419'], [1, '#4a1812']]),
	lin('navyHi', [[0, '#55709a'], [1, '#3a5072']]),
	lin('navyMid', [[0, '#34496a'], [1, '#2b3c55']]),
	lin('navyLo', [[0, '#24324a'], [1, '#172133']]),
	lin('navyLoop', [[0, '#34496a'], [0.7, '#22304a'], [1, '#151e2e']]),
	lin('walnut', [[0, '#6a5240'], [0.5, '#4a3a2e'], [1, '#2e231a']]),
	lin('steelCylH', [[0, '#45484d'], [0.22, '#959aa1'], [0.32, '#e6eaee'], [0.45, '#8e9399'], [1, '#4d5156']], false),
	rad('redDot', [[0, '#fff0ee'], [0.4, '#ff5a48'], [1, '#ff3b2e', 0]], 0.5, 0.5, 0.5),
];
function bevel(id, blur, surf, specK, specE, gf, ga) {
	return `<filter id="${id}" x="-2%" y="-2%" width="104%" height="104%" color-interpolation-filters="sRGB">
	<feGaussianBlur in="SourceAlpha" stdDeviation="${r2(blur)}" result="b"/>
	<feDiffuseLighting in="b" surfaceScale="${r2(surf)}" diffuseConstant="1.15" lighting-color="#ffffff" result="d"><feDistantLight azimuth="225" elevation="60"/></feDiffuseLighting>
	<feSpecularLighting in="b" surfaceScale="${r2(surf)}" specularConstant="${specK}" specularExponent="${specE}" lighting-color="#ffffff" result="s"><feDistantLight azimuth="225" elevation="32"/></feSpecularLighting>
	<feComposite in="s" in2="SourceAlpha" operator="in" result="s2"/>
	<feBlend in="SourceGraphic" in2="d" mode="multiply" result="lit"/>
	<feComposite in="lit" in2="s2" operator="arithmetic" k2="1" k3="1" result="ls"/>
	<feTurbulence type="fractalNoise" baseFrequency="${gf}" numOctaves="2" seed="11" result="n"/>
	<feColorMatrix in="n" type="matrix" values="1 0 0 0 0  1 0 0 0 0  1 0 0 0 0  0 0 0 0 1" result="ng"/>
	<feComposite in="ls" in2="ng" operator="arithmetic" k2="1" k3="${ga}" k4="${-ga / 2}" result="g"/>
	<feComposite in="g" in2="SourceAlpha" operator="in"/></filter>`;
}
function defs(V) {
	const c = (cm, lo, hi) => Math.max(lo, Math.min(hi, cm * V.s));
	const B = (id, cm, lo, hi, k, e, gf, ga) => { const b = c(cm, lo, hi); return bevel(id, b, b * 2.0, k, e, gf, ga); };
	return `<defs>${GRADS.join('')}
	${B('pL', 0.45, 2, 9, 0.35, 14, 0.85, 0.05)}${B('pM', 0.2, 1.5, 6, 0.35, 14, 0.85, 0.05)}${B('pS', 0.08, 1, 3, 0.3, 14, 0.85, 0.04)}
	${B('mM', 0.18, 1.5, 6, 0.9, 28, 1.3, 0.05)}${B('mS', 0.07, 0.9, 3, 0.9, 28, 1.3, 0.05)}
	${B('bM', 0.25, 1.5, 7, 1.2, 22, 1.1, 0.06)}${B('bS', 0.1, 1, 4, 1.2, 22, 1.1, 0.06)}
	${B('rM', 0.35, 2, 8, 0.12, 10, 1.5, 0.09)}
	${UPLOAD ? '<filter id="ao"><feOffset dx="0" dy="0"/></filter><filter id="glow"><feOffset dx="0" dy="0"/></filter>' : `<filter id="ao" x="-10%" y="-10%" width="120%" height="120%"><feDropShadow dx="0.6" dy="1.2" stdDeviation="1.4" flood-color="#000" flood-opacity=".42"/></filter>
	<filter id="glow" x="-50%" y="-50%" width="200%" height="200%"><feGaussianBlur stdDeviation="${r2(c(0.08, 1.2, 5))}" result="b"/><feMerge><feMergeNode in="b"/><feMergeNode in="SourceGraphic"/></feMerge></filter>`}
	<pattern id="stip" width="5" height="5" patternUnits="userSpaceOnUse"><circle cx="1.5" cy="1.5" r="1.05" fill="#6e6046" opacity=".55"/><circle cx="4" cy="3.8" r=".9" fill="#ffffff" opacity=".35"/></pattern>
	<pattern id="stipDark" width="4" height="4" patternUnits="userSpaceOnUse"><circle cx="1.2" cy="1.2" r=".8" fill="#000" opacity=".4"/><circle cx="3.1" cy="3" r=".7" fill="#ffffff" opacity=".12"/></pattern>
	</defs>`;
}
const ao = (inner) => `<g filter="url(#ao)">${inner}</g>`;
// A screw: a steel head with a hex socket.
function screw(V, a, v, r = 0.3) {
	const [x, y] = V.P(a, v), h = r * 0.45 * V.s;
	const hex = Array.from({ length: 6 }, (_, i) => [x + h * Math.cos(i * Math.PI / 3), y + h * Math.sin(i * Math.PI / 3)]);
	return ao(circ(V, a, v, r, 'url(#rSteel)', 'mS')) + `<path d="${rounded(hex, 0)}" fill="#15161a"/>`;
}
const rivet = (V, a, v, r = 0.28) => ao(circ(V, a, v, r, 'url(#rBlack)', 'mS')) + circ(V, a, v, r * 0.35, '#c9cfd6', null, `opacity=".55" transform="translate(${-r * V.s * 0.3},${-r * V.s * 0.3})"`);
// The rail's cross-section (w, v): a dovetail, as set 1 (the sight interface).
const RAIL_X = [[-1.05, 5.2], [1.05, 5.2], [1.05, 5.5], [0.86, 5.62], [1.05, 5.78], [0.84, 6.15], [-0.84, 6.15], [-1.05, 5.78], [-0.86, 5.62], [-1.05, 5.5]];

// ---- set 4 helpers: smooth outlines (cubic curves in gun space), clips, strokes in cm -------------------------------
const cb = (a, b, c) => ['C', a, b, c];
function cd(V, segs, close = true) {
	let d = '';
	for (const s of segs) {
		if (s[0] === 'C') d += 'C' + s.slice(1).map(p => V.P(p[0], p[1]).map(r2).join(',')).join(' ');
		else d += (d ? 'L' : 'M') + V.P(s[0], s[1]).map(r2).join(',');
	}
	return close ? d + 'Z' : d;
}
const sk = (V, segs, col, cm, extra = '') =>
	`<path d="${cd(V, segs, false)}" fill="none" stroke="${col}" stroke-width="${r2(Math.max(0.6, cm * V.s))}" stroke-linecap="round" stroke-linejoin="round" ${extra}/>`;
let cid = 0;
const clip = (d, inner) => { const id = 'k4_' + cid++; return `<clipPath id="${id}"><path d="${d}"/></clipPath><g clip-path="url(#${id})">${inner}</g>`; };
const flt = (f, inner) => `<g filter="url(#${f})">${inner}</g>`;
const full = `<rect x="0" y="0" width="${N}" height="${N}"`;
// A recessed seam: a dark groove with a light lip under it.
const seam = (V, segs, dv = -0.05) => sk(V, segs, '#5c5f64', 0.07, 'opacity=".75"') + sk(V, segs.map(p => p[0] === 'C' ? ['C', ...p.slice(1).map(([a, v]) => [a, v + dv])] : [p[0], p[1] + dv]), '#ffffff', 0.04, 'opacity=".6"');
// The rarity accent: a cyan inlay in a dark channel.
const accent = (V, segs, cm = 0.13) => sk(V, segs, '#14181c', cm * 1.9, 'opacity=".85"') + sk(V, segs, 'url(#cyanLin)', cm) + sk(V, segs, '#e8fcff', cm * 0.35, 'opacity=".8"');
const knurlRing = (V, a, v, r, n, col) => Array.from({ length: n }, (_, k) => {
	const t = k * Math.PI * 2 / n;
	return pl(V, [[a + r * 0.84 * Math.cos(t), v + r * 0.84 * Math.sin(t)], [a + r * Math.cos(t), v + r * Math.sin(t)]], col, 1);
}).join('');
const qd = (V, a, v, r = 0.42) => ao(circ(V, a, v, r, 'url(#rAlu)', 'mS')) + circ(V, a, v, r * 0.55, '#1a1c1f') + circ(V, a, v, r * 0.25, 'url(#rGph)');
const pin = (V, a, v, r = 0.17) => ao(circ(V, a, v, r, 'url(#rBrass)', 'bS')) + ring(V, a, v, r * 1.25, '#2a2c30', 0.9, 'opacity=".5"');

const G4 = [
	lin('wht', [[0, '#ffffff'], [0.15, '#f6f5f1'], [0.45, '#dedcd5'], [0.6, '#cdcbc4'], [1, '#b0aea6']]),   // satin white polymer
	lin('whtHi', [[0, '#ffffff'], [1, '#efeee9']]),
	lin('whtMid', [[0, '#f0efea'], [1, '#d9d7cf']]),
	lin('whtLo', [[0, '#d9d7d0'], [1, '#b9b7af']]),
	lin('gph', [[0, '#62666d'], [0.45, '#43464c'], [1, '#2a2c30']]),                         // graphite polymer / anodized
	lin('gphLo', [[0, '#3c3f44'], [1, '#222427']]),
	lin('gphCyl', [[0, '#141518'], [0.2, '#3d4046'], [0.31, '#868b93'], [0.42, '#43474d'], [0.75, '#26282c'], [1, '#111214']]),
	lin('gphCylH', [[0, '#141518'], [0.2, '#3d4046'], [0.31, '#868b93'], [0.42, '#43474d'], [0.75, '#26282c'], [1, '#111214']], false),
	lin('alu', [[0, '#f2f4f6'], [0.5, '#c9ced4'], [1, '#9ba1a8']]),                          // machined aluminium
	lin('aluCyl', [[0, '#686d74'], [0.2, '#b8bdc4'], [0.32, '#f7f9fa'], [0.45, '#c3c8ce'], [0.8, '#8f949b'], [1, '#5c6167']]),
	lin('aluCylH', [[0, '#686d74'], [0.2, '#b8bdc4'], [0.32, '#f7f9fa'], [0.45, '#c3c8ce'], [0.8, '#8f949b'], [1, '#5c6167']], false),
	lin('cyanLin', [[0, '#d2f8ff'], [0.5, '#5fd6f5'], [1, '#1f9cc6']]),
	lin('smoke', [[0, '#33404c'], [0.5, '#1a222a'], [1, '#0c1014']]),
	lin('form4', [[0, '#ffffff', 0], [0.1, '#ffffff', 0.35], [0.24, '#ffffff', 0.05], [0.55, '#000000', 0], [1, '#1c2230', 0.22]]),
	rad('rAlu', [[0, '#ffffff'], [0.45, '#c5cad0'], [1, '#6c7178']]),
	rad('rGph', [[0, '#80858d'], [0.45, '#3f4248'], [1, '#141518']]),
	rad('rWht', [[0, '#ffffff'], [0.5, '#e8e6e0'], [1, '#aeaca4']]),
	rad('rLens', [[0, '#9fe4ff'], [0.25, '#3a6f8c'], [0.65, '#1a2f40'], [1, '#0a1218']], 0.34, 0.3, 0.85),
	rad('rCoat', [[0, '#ffd9a0', 0.55], [0.5, '#c58cff', 0.25], [1, '#7a5cff', 0]], 0.3, 0.25, 0.7),
];
function defs4(V) {
	return `<defs>${G4.join('')}
	<filter id="soft" x="-5%" y="-5%" width="110%" height="110%"><feGaussianBlur stdDeviation="${r2(Math.max(1, 0.16 * V.s))}"/></filter>
	<filter id="soft2" x="-10%" y="-10%" width="120%" height="120%"><feGaussianBlur stdDeviation="${r2(Math.max(1.5, 0.4 * V.s))}"/></filter>
	<pattern id="brush" width="80" height="3" patternUnits="userSpaceOnUse"><path d="M0,1H80" stroke="#ffffff" stroke-opacity=".28" stroke-width=".7"/><path d="M10,2.4H56" stroke="#000" stroke-opacity=".08" stroke-width=".6"/></pattern>
	<pattern id="brushV" width="3" height="80" patternUnits="userSpaceOnUse"><path d="M1,0V80" stroke="#ffffff" stroke-opacity=".28" stroke-width=".7"/><path d="M2.4,10V56" stroke="#000" stroke-opacity=".08" stroke-width=".6"/></pattern>
	<pattern id="stip4" width="4.5" height="4.5" patternUnits="userSpaceOnUse"><circle cx="1.2" cy="1.2" r=".85" fill="#000" opacity=".45"/><circle cx="3.4" cy="3.3" r=".75" fill="#fff" opacity=".16"/></pattern>
	</defs>`;
}

// ---- Body_Marksman: a long streamlined two-tone shell ----------------------------------------------------------------
// Satin-white upper, graphite lower, split by one flowing parting line with the cyan accent; a raised comb over the
// butt, a flat spine with a flush low rail, a long tapering snout to the barrel nut, a hand-stop lip, a raked separate
// grip and a slim swept trigger guard (set 1 has a boxy shell and a closed grip loop).
const SH4 = [[0, -8.0], [0, 2.0], cb([0, 3.0], [0.6, 3.6], [1.8, 3.6]), cb([5.0, 3.8], [9.0, 4.4], [12.0, 5.0]), cb([12.6, 5.15], [13.0, 5.2], [13.6, 5.2]),
	[33.0, 5.2], cb([39.0, 5.2], [45.0, 3.9], [49.4, 2.3]), cb([50.5, 1.95], [51.0, 1.8], [51.0, 1.3]), [51.0, -1.3], cb([51.0, -1.9], [50.4, -2.1], [49.4, -2.4]),
	cb([46.5, -3.3], [42.5, -4.4], [39.6, -4.9]), cb([38.4, -5.1], [37.8, -5.6], [37.6, -6.4]), cb([37.4, -7.2], [36.5, -7.35], [36.0, -6.6]),
	cb([35.6, -6.0], [35.0, -5.7], [34.0, -5.7]), [10.0, -5.7], cb([8.0, -5.7], [6.0, -6.6], [4.0, -8.0]), cb([2.6, -9.0], [1.6, -9.4], [0.8, -9.3]),
	cb([0.2, -9.2], [0, -8.8], [0, -8.0])];
const PART4 = [[-1, -2.6], cb([8, -2.4], [15, -3.6], [23, -3.4]), cb([31, -3.3], [41, -1.5], [53, -0.4]), [53, -20], [-1, -20]];
const ACC4 = [[0.8, -2.2], cb([8, -2.0], [15, -3.2], [23, -3.0]), cb([31, -2.9], [41, -1.1], [50.8, -0.1])];
const PANEL4 = [[3.4, 0.9], cb([3.4, 2.4], [4.3, 2.8], [6.0, 2.8]), [15.6, 3.0], cb([17.4, 3.05], [18.3, 2.4], [18.2, 1.0]), [17.9, -0.6],
	cb([17.75, -1.2], [17.3, -1.35], [16.6, -1.35]), [5.4, -0.95], cb([4.0, -0.9], [3.4, -0.3], [3.4, 0.9])];
const GRIP4 = [[20.8, -5.4], [20.8, -6.2], cb([20.0, -8.8], [19.4, -12.0], [19.3, -14.6]), cb([19.25, -15.5], [19.7, -16.1], [20.4, -16.1]), [23.8, -16.0],
	cb([24.4, -16.0], [24.7, -15.6], [24.6, -15.0]), cb([24.8, -12.6], [25.4, -9.0], [25.8, -7.4]), cb([26.0, -6.6], [26.3, -6.0], [26.6, -5.4])];
const GUARD4 = [[24.5, -13.4], cb([28, -14.4], [33.6, -12.6], [36.7, -6.9])];
const TRIG4 = [[27.0, -5.7], [28.1, -5.7], [28.0, -7.0], [27.7, -8.6], [27.2, -9.2], [27.0, -8.4], [27.3, -7.2]];
const STRIP4 = [[39.6, 0.2], [47.6, 0.55], [47.3, 1.9], [39.8, 2.3]];
const SPINE_HI = [[1.6, 2.8], cb([6, 3.05], [10, 3.7], [13.6, 4.3]), [33, 4.35], cb([39, 4.35], [44.6, 3.2], [49.6, 1.45])];

function rail4(V, u0, u1, v) {
	let s = shp(pth(V, R4(u0, v, u1, v + 0.42), 0.08), 'url(#gph)', 'mS');
	for (let u = u0 + 0.45; u + 0.55 <= u1 - 0.3; u += 1.0) s += shp(pth(V, R4(u, v + 0.38, u + 0.55, v + 0.95), 0.07), 'url(#gph)', 'mS');
	return ao(s) + pl(V, [[u0 + 0.2, v + 0.36], [u1 - 0.2, v + 0.36]], '#a3a9b1', 0.9, 'opacity=".45"');
}
function body4Side(V) {
	const right = V.kind === 'right', shellD = cd(V, SH4), gripD = cd(V, GRIP4);
	let s = '';
	// the raked grip, its stippled panel and machined aluminium cap
	s += shp(gripD, 'url(#gph)', 'pL');
	s += flt('mS', clip(gripD, shp(pth(V, R4(17, -17, 27, -15.3), 0), 'url(#alu)')));
	s += clip(gripD, pl(V, [[18, -15.3], [26, -15.3]], '#1d1f22', 1.4) + pl(V, [[18, -15.36], [26, -15.36]], '#ffffff', 0.7, 'opacity=".5"'));
	const gp = pth(V, [[21.3, -7.2], [25.1, -7.7], [24.4, -14.4], [20.0, -14.4], [20.3, -11]], 0.6);
	s += shp(gp, 'url(#stip4)', null) + shp(gp, 'none', null, 'stroke="#1b1d20" stroke-width="1.3"') + shp(gp, 'none', null, 'stroke="#8d929a" stroke-width=".6" opacity=".45" transform="translate(0,1)"');
	s += pin(V, 22.2, -15.7, 0.15);
	// the slim swept guard and the flat-faced aluminium trigger
	s += flt('pM', sk(V, GUARD4, '#45484e', 0.72)) + sk(V, GUARD4.map(p => p[0] === 'C' ? ['C', ...p.slice(1).map(([a, v]) => [a - 0.06, v + 0.2])] : [p[0] - 0.06, p[1] + 0.2]), '#9ca1a9', 0.12, 'opacity=".55"');
	s += ao(shp(pth(V, TRIG4, 0.18), 'url(#alu)', 'mS'));
	// the shell: white, the graphite lower half under the parting line, soft form light, seams
	s += shp(pth(V, R4(-0.3, -8.4, 0.05, 1.8), 0.08), 'url(#alu)', 'mS');            // butt plate's edge
	s += shp(shellD, 'url(#wht)', 'pL');
	s += flt('pM', clip(shellD, shp(cd(V, PART4), 'url(#gph)')));
	const upper = ACC4.map(p => p[0] === 'C' ? ['C', ...p.slice(1).map(([a, v]) => [a, v + 0.75])] : [p[0], p[1] + 0.75]);
	s += clip(shellD, `<rect x="0" y="${r2(V.Y(5.3))}" width="${N}" height="${r2(9.5 * V.s)}" fill="url(#form4)"/>` +
		flt('soft2', sk(V, upper, '#2a3040', 1.4, 'opacity=".16"')) + flt('soft2', sk(V, [[0.6, -0.4], [0.6, 3.0]], '#2a3040', 0.9, 'opacity=".12"')) +
		flt('soft', sk(V, SPINE_HI, '#ffffff', 0.55, 'opacity=".7"')) +
		flt('soft', sk(V, [[38.5, -4.0], cb([42, -3.5], [46, -2.6], [49.8, -1.3])], '#ffffff', 0.3, 'opacity=".22"')) +
		flt('soft', sk(V, [[1.0, -7.8], cb([3, -6.6], [6, -5.4], [10, -4.9])], '#ffffff', 0.3, 'opacity=".18"')) +
		seam(V, [[38.4, 5.4], cb([37.6, 2.0], [37.9, -1.5], [38.8, -5.2])]) + seam(V, [[2.2, 4.0], cb([1.7, 0], [1.7, -5], [2.6, -9.6])]) +
		pl(V, [[13.8, 4.9], [32.8, 4.9]], '#ffffff', 1.1, 'opacity=".8"') + accent(V, ACC4));
	// the flush aluminium side panel (the ejection cover on the right), its seam and two flush screws
	const panD = cd(V, PANEL4);
	s += shp(panD, 'url(#alu)', 'mS') + clip(panD, `${full} fill="url(#brush)"/>`);
	s += shp(panD, 'none', null, 'stroke="#3b3e43" stroke-width="1.3" opacity=".75"');
	if (right) {
		const port = pth(V, R4(5.2, -0.2, 11.4, 2.1), 0.45);
		s += shp(port, 'url(#aluCyl)', 'mS') + shp(port, 'none', null, 'stroke="#2a2c30" stroke-width="1.2"');
		s += pl(V, [[5.6, -0.05], [11.0, -0.05]], '#5e636a', 1.1) + pin(V, 10.7, 1.55, 0.13);
	} else {
		for (const dv of [0, 0.35, 0.7]) s += pl(V, [[6.2, -0.35 + dv], [16.4, -0.7 + dv]], '#7f858c', 0.9, 'opacity=".45"');
	}
	for (const [a, v] of [[4.5, 1.9], [17.0, 2.1]]) s += screw(V, a, v, 0.17);
	// heat slits in a recessed graphite strip on the nose cap, ember behind them
	s += shp(pth(V, STRIP4, 0.5), 'url(#gph)', 'pS');
	for (let i = 0; i < 6; i++) {
		const u = 40.2 + i * 1.18, k = i * 0.04;
		s += shp(pth(V, [[u, 0.5 + k], [u + 0.36, 0.52 + k], [u + 0.66, 1.72 - k], [u + 0.3, 1.74 - k]], 0.14), '#140b06', null);
		s += `<g filter="url(#glow)">${shp(pth(V, [[u + 0.08, 0.66 + k], [u + 0.28, 0.67 + k], [u + 0.55, 1.6 - k], [u + 0.36, 1.61 - k]], 0.08), 'url(#emberLin)', null)}</g>`;
	}
	// the charging handle runs in a slot on the gun's left
	if (!right) {
		s += shp(pth(V, R4(30.2, 3.28, 41.2, 3.58), 0.15), '#4a4e55', null) + pl(V, [[30.4, 3.26], [41.0, 3.26]], '#ffffff', 0.8, 'opacity=".8"');
		s += ao(shp(pth(V, R4(39.6, 3.0, 41.5, 3.95), 0.22), 'url(#alu)', 'mS'));
		for (const a of [40.0, 40.4, 40.8, 41.2]) s += pl(V, [[a, 3.15], [a, 3.8]], '#7a8087', 0.9);
	}
	// mag well flare, release button, ambidextrous brass-tipped selector, pins, sling cups
	s += ao(shp(pth(V, R4(10.0, -6.35, 18.0, -5.5), 0.22), 'url(#alu)', 'mS')) + pl(V, [[10.3, -5.62], [17.7, -5.62]], '#ffffff', 0.8, 'opacity=".6"');
	s += ao(circ(V, 19.0, -4.5, 0.38, 'url(#rAlu)', 'mS')) + ring(V, 19.0, -4.5, 0.25, '#7a8087', 0.9);
	s += ao(sk(V, [[21.4, -4.0], [23.1, -3.62]], '#2a2c30', 0.36) + sk(V, [[21.4, -4.0], [23.1, -3.62]], 'url(#brass)', 0.24));
	s += ao(circ(V, 21.4, -4.0, 0.42, 'url(#rAlu)', 'mS')) + circ(V, 21.4, -4.0, 0.14, '#3a3d42');
	for (const [a, v] of [[8.6, -4.6], [28.0, -4.6], [35.4, -4.5]]) s += pin(V, a, v);
	s += qd(V, 2.9, -7.3) + qd(V, 45.2, -2.55, 0.36);
	// the low rail on the spine with faired ends
	for (const [a0, a1, up] of [[12.9, 14.2, true], [31.8, 33.1, false]])
		s += shp(pth(V, up ? [[a0, 5.15], [a1, 5.15], [a1, 5.62]] : [[a0, 5.15], [a1, 5.15], [a0, 5.62]], 0.05), 'url(#whtHi)', 'pS');
	s += rail4(V, 14, 32, 5.2);
	// barrel nut: a fine brass ring, then the graphite nut
	s += ao(shp(pth(V, R4(50.75, -1.5, 51.12, 1.5), 0.06), 'url(#brassCyl)', 'bS'));
	s += ao(shp(pth(V, R4(51.1, -1.25, 51.9, 1.25), 0.08), 'url(#gphCyl)', 'mS'));
	for (let a = 51.22; a < 51.85; a += 0.14) s += pl(V, [[a, -1.12], [a, 1.12]], '#0d0e10', 0.9, 'opacity=".5"');
	return s + wear(gripD, '#a7adb5', 1, 0.4, '3 22 2 40');
}
// Cross-sections seen end on (w, v).
const XS_MAIN = [[-1.4, 5.2], [1.4, 5.2], cb([2.2, 5.2], [2.6, 4.2], [2.6, 2.6]), [2.6, -3.4], cb([2.6, -5.0], [2.4, -5.7], [1.9, -5.7]), [-1.9, -5.7],
	cb([-2.4, -5.7], [-2.6, -5.0], [-2.6, -3.4]), [-2.6, 2.6], cb([-2.6, 4.2], [-2.2, 5.2], [-1.4, 5.2])];
const XS_BUTT = [[-1.7, 3.6], [1.7, 3.6], cb([2.3, 3.6], [2.5, 3.0], [2.5, 2.2]), [2.5, -8.2], cb([2.5, -9.0], [2.1, -9.3], [1.5, -9.3]), [-1.5, -9.3],
	cb([-2.1, -9.3], [-2.5, -9.0], [-2.5, -8.2]), [-2.5, 2.2], cb([-2.5, 3.0], [-2.3, 3.6], [-1.7, 3.6])];
const XS_LIP = [[-2.1, -5.4], [2.1, -5.4], [1.9, -6.8], cb([1.8, -7.25], [1.4, -7.35], [1.0, -7.35]), [-1.0, -7.35], cb([-1.4, -7.35], [-1.8, -7.25], [-1.9, -6.8])];
const XS_SNOUT = [[0, 3.9], cb([1.5, 3.9], [2.3, 2.6], [2.3, 0.4]), cb([2.3, -2.0], [1.7, -3.4], [0, -3.4]), cb([-1.7, -3.4], [-2.3, -2.0], [-2.3, 0.4]), cb([-2.3, 2.6], [-1.5, 3.9], [0, 3.9])];
const XS_NOSE = [[0, 2.05], cb([1.2, 2.05], [2.0, 1.4], [2.0, 0]), cb([2.0, -1.5], [1.3, -2.45], [0, -2.45]), cb([-1.3, -2.45], [-2.0, -1.5], [-2.0, 0]), cb([-2.0, 1.4], [-1.2, 2.05], [0, 2.05])];
// A section in white over graphite below the parting height pv.
function xsec(V, segs, top, low, pv, filt = 'pM') {
	const d = cd(V, segs);
	return shp(d, top, filt) + flt(filt, clip(d, shp(pth(V, R4(-5, -30, 5, pv), 0), low)));
}
function body4Grip(V) {
	let s = shp(pth(V, R4(-1.5, -16.1, 1.5, -5.6), 0.9), 'url(#gph)', 'pL');
	return s + flt('mS', clip(pth(V, R4(-1.5, -16.1, 1.5, -5.6), 0.9), shp(pth(V, R4(-3, -17, 3, -15.3), 0), 'url(#alu)'))) + pl(V, [[-1.45, -15.3], [1.45, -15.3]], '#1d1f22', 1.3);
}
function body4Front(V) {
	let s = flt('pM', shp(cd(V, XS_BUTT), 'url(#gphLo)'));
	s += body4Grip(V);
	s += xsec(V, XS_MAIN, 'url(#whtMid)', 'url(#gph)', -3.0, 'pL');
	s += shp(pth(V, RAIL_X, 0.03), 'url(#gph)', 'mS');
	s += ao(shp(pth(V, R4(2.15, 3.0, 2.75, 3.95), 0.2), 'url(#alu)', 'mS'));                    // charging handle (gun's left)
	for (const sd of [-1, 1]) s += ao(shp(pth(V, R4(sd * 2.55, -4.85, sd * 2.75, -4.15), 0.06), 'url(#alu)', 'mS'));   // mag release
	s += ao(shp(pth(V, R4(-0.38, -14.0, 0.38, -6.9), 0.3), 'url(#gphCylH)', 'pM'));           // the guard, end on
	s += flt('pM', shp(cd(V, XS_LIP), 'url(#gph)'));
	s += xsec(V, XS_SNOUT, 'url(#whtHi)', 'url(#gph)', -1.0);
	for (const sd of [-1, 1]) s += `<g filter="url(#glow)">${shp(pth(V, R4(sd * 2.2, 0.5, sd * 2.29, 1.7), 0.03), 'url(#emberLin)', null)}</g>`;
	s += clip(cd(V, XS_SNOUT), accent(V, [[-2.4, -0.55], [-2.1, -0.55]]) + accent(V, [[2.1, -0.55], [2.4, -0.55]]));
	s += xsec(V, XS_NOSE, 'url(#wht)', 'url(#gph)', -0.45);
	s += clip(cd(V, XS_NOSE), flt('soft', sk(V, [[-1.2, 1.55], cb([-0.4, 1.95], [0.4, 1.95], [1.2, 1.55])], '#ffffff', 0.25, 'opacity=".8"')));
	s += ao(circ(V, 0, 0, 1.5, 'url(#rBrass)', 'bS')) + ao(circ(V, 0, 0, 1.25, 'url(#rGph)', 'mM'));
	s += ring(V, 0, 0, 1.0, '#101113', 1.4) + circ(V, 0, 0, 0.82, 'url(#bore)', null);
	for (const r of [0.74, 0.64, 0.54]) s += ring(V, 0, 0, r, '#4a4d52', 0.9, 'opacity=".8"');
	return s;
}
function body4Back(V) {
	let s = flt('pM', shp(cd(V, XS_LIP), 'url(#gph)'));
	s += ao(shp(pth(V, R4(-0.38, -14.0, 0.38, -6.9), 0.3), 'url(#gphCylH)', 'pM'));
	s += body4Grip(V);
	s += xsec(V, XS_MAIN, 'url(#whtLo)', 'url(#gphLo)', -3.2, 'pL');
	s += shp(pth(V, RAIL_X, 0.03), 'url(#gph)', 'mS');
	s += ao(shp(pth(V, R4(2.15, 3.0, 2.75, 3.95), 0.2), 'url(#alu)', 'mS'));
	s += xsec(V, XS_BUTT, 'url(#whtMid)', 'url(#gph)', -2.6, 'pL');
	s += clip(cd(V, XS_BUTT), accent(V, [[-2.6, -2.2], [-2.3, -2.2]]) + accent(V, [[2.3, -2.2], [2.6, -2.2]]));
	s += clip(cd(V, XS_BUTT), flt('soft', sk(V, [[-1.4, 3.15], [1.4, 3.15]], '#ffffff', 0.25, 'opacity=".8"')));
	// the butt plate the stock seats on (the interface): guide rails, two bolts, the locating boss at (0, -3.3)
	const bp = pth(V, R4(-2.2, -8.4, 2.2, 1.8), 0.45);
	s += ao(shp(bp, 'url(#alu)', 'mM')) + clip(bp, `${full} fill="url(#brushV)"/>`);
	for (const sd of [-1, 1]) s += shp(pth(V, R4(sd * 1.55, -7.8, sd * 1.85, 1.2), 0.1), '#7d838a', 'mS');
	s += screw(V, 0, -6.6, 0.36) + screw(V, 0, 0.2, 0.36);
	s += ao(circ(V, 0, -3.3, 0.75, 'url(#rGph)', 'mS')) + circ(V, 0, -3.3, 0.38, '#0f1012') + ring(V, 0, -3.3, 0.9, '#c9a24a', 1.2, 'opacity=".9"');
	for (const sd of [-1, 1]) s += ao(shp(pth(V, R4(sd * 2.5, -7.75, sd * 2.62, -6.85), 0.05), 'url(#alu)', 'mS'));
	return s;
}

// ---- Barrel_Marksman: thin fluted barrel (local: u from the barrel nut) ------------------------------------------------
function brl4Side(V) {
	let s = ao(shp(pth(V, R4(22.35, -0.76, 24, 0.76), 0.04), 'url(#steelCyl)', 'mS'));          // muzzle threads
	for (let u = 22.5; u < 23.9; u += 0.11) s += pl(V, [[u, -0.74], [u + 0.04, 0.74]], '#2e3135', 0.9, 'opacity=".55"');
	s += shp(pth(V, R4(23.93, -0.7, 24, 0.7), 0.02), '#dfe3e7', null);
	s += ao(shp(pth(V, [[1.7, -1.25], [2.7, -0.88], [2.7, 0.88], [1.7, 1.25]], 0.05), 'url(#gphCyl)', 'mS'));
	s += ao(shp(pth(V, R4(2.6, -0.88, 22.45, 0.88), 0.06), 'url(#gphCyl)', 'mM'));
	for (const [u0, u1] of [[3.6, 9.15], [11.85, 19.4]]) for (const [vc, hw] of [[0.5, 0.1], [0, 0.14], [-0.5, 0.1]]) {
		s += shp(pth(V, R4(u0, vc - hw, u1, vc + hw), hw), '#111214', null, 'opacity=".9"');
		s += pl(V, [[u0 + hw, vc - hw + 0.02], [u1 - hw, vc - hw + 0.02]], '#a9aeb5', 0.8, 'opacity=".55"');
	}
	s += ao(shp(pth(V, R4(0, -1.6, 1.75, 1.6), 0.12), 'url(#aluCyl)', 'mM'));                    // the seating collar, a wrench flat
	s += shp(pth(V, R4(0.3, -0.73, 1.42, 0.73), 0.04), 'url(#alu)', null, 'opacity=".9"') + pl(V, [[0.32, 0.73], [1.4, 0.73]], '#ffffff', 0.9, 'opacity=".8"') + pl(V, [[0.32, -0.73], [1.4, -0.73]], '#5c6167', 0.9, 'opacity=".8"');
	for (const u of [0.18, 1.58]) s += pl(V, [[u, -1.55], [u, 1.55]], '#4e5359', 1.1, 'opacity=".7"');
	s += ao(shp(pth(V, R4(9.6, -1.02, 11.4, 1.2), 0.18), 'url(#aluCyl)', 'mS'));                  // low gas block
	s += pl(V, [[9.75, 0.95], [11.25, 0.95]], '#ffffff', 0.8, 'opacity=".6"');
	for (const u of [9.98, 11.02]) s += screw(V, u, -0.62, 0.13);
	s += ao(shp(pth(V, R4(19.9, -0.96, 20.26, 0.96), 0.05), 'url(#brassCyl)', 'bS'));
	return s + wear(pth(V, R4(2.6, -0.88, 22.45, 0.88), 0.06), '#b3b9c1', 1, 0.4, '4 40 2 60');
}
function fluted(r, n, depth) {
	return Array.from({ length: 96 }, (_, i) => {
		const t = i * Math.PI * 2 / 96, c = Math.cos(n * t + Math.PI / 2 * 0);
		const rr = r - depth * Math.max(0, (c - 0.4) / 0.6);
		return [rr * Math.cos(t), rr * Math.sin(t)];
	});
}
function collar4(V) {
	const cl = pth(V, R4(-1.42, -1.7, 1.42, 1.7), 0);
	return flt('mM', clip(cl, circ(V, 0, 0, 1.6, 'url(#rAlu)'))) + pl(V, [[-1.42, -0.7], [-1.42, 0.7]], '#f4f6f8', 1.4) + pl(V, [[1.42, -0.7], [1.42, 0.7]], '#6c7178', 1.4);
}
function brl4Front(V) {
	let s = collar4(V);
	s += ao(shp(pth(V, R4(-1.02, -1.02, 1.02, 1.2), 0.28), 'url(#alu)', 'mS'));
	s += ao(circ(V, 0, 0, 0.96, 'url(#rBrass)', 'bS'));
	s += ao(shp(pth(V, fluted(0.88, 6, 0.14), 0), 'url(#rGph)', 'mM'));
	s += ao(circ(V, 0, 0, 0.76, 'url(#rSteel)', 'mS'));
	for (const r of [0.72, 0.66]) s += ring(V, 0, 0, r, '#3a3d42', 0.9, 'opacity=".7"');
	s += circ(V, 0, 0, 0.58, 'url(#rGph)', 'mS') + ring(V, 0, 0, 0.46, '#d6dade', 1.6) + circ(V, 0, 0, 0.3, 'url(#bore)', null);
	return s;
}
function brl4Back(V) {
	let s = collar4(V) + ring(V, 0, 0, 1.38, '#5b6067', 1.2, 'opacity=".8"');
	s += ao(circ(V, 0, 0, 1.15, 'url(#rSteel)', 'mS'));
	for (const r of [1.08, 1.0, 0.92]) s += ring(V, 0, 0, r, '#5a5e64', 1, 'opacity=".8"');
	s += circ(V, 0, 0, 0.8, 'url(#rGph)', 'mS') + circ(V, 0, 0, 0.32, 'url(#bore)', null);
	s += shp(pth(V, R4(-0.07, 0.3, 0.07, 0.48), 0.02), '#0a0b0c', null);
	s += shp(pth(V, R4(-0.13, 1.0, 0.13, 1.32), 0.03), 'url(#brass)', 'bS');                        // alignment key
	return s;
}

// ---- Muzzle_Compensator: slim, three raked side ports (local) --------------------------------------------------------
const CMP4 = [[1.1, -1.0], [4.3, -1.0], [5.0, -0.72], [5.0, 0.72], [4.3, 1.0], [1.1, 1.0]];
function cmp4Side(V) {
	let s = ao(shp(pth(V, R4(0, -0.9, 1.05, 0.9), 0.05), 'url(#gphCyl)', 'mS'));
	s += shp(pth(V, R4(0.15, -0.41, 0.88, 0.41), 0.03), '#4c5057', null) + pl(V, [[0.17, 0.41], [0.86, 0.41]], '#9aa0a8', 0.9) + pl(V, [[0.17, -0.41], [0.86, -0.41]], '#0e0f11', 0.9);   // wrench flat
	s += ao(shp(pth(V, CMP4, 0.05), 'url(#gphCyl)', 'mM'));
	s += shp(pth(V, [[4.3, -1.0], [5.0, -0.72], [5.0, 0.72], [4.3, 1.0]], 0.03), 'url(#steelCyl)', 'mS');   // bright machined chamfer
	for (const u0 of [1.45, 2.15, 2.85, 3.55]) {   // four raked side ports, wider toward the front
		const k = (u0 - 1.45) * 0.05, port = [[u0, -0.58], [u0 + 0.32 + k, -0.58], [u0 + 0.6 + k, 0.58], [u0 + 0.28, 0.58]];
		s += shp(pth(V, port, 0.13), '#0b0c0e', null);
		s += `<g filter="url(#glow)">${shp(pth(V, [[u0 + 0.12, -0.36], [u0 + 0.24 + k, -0.36], [u0 + 0.44 + k, 0.36], [u0 + 0.32, 0.36]], 0.05), 'url(#emberLin)', null, 'opacity=".7"')}</g>`;
		s += pl(V, [[u0 + 0.36 + k, -0.56], [u0 + 0.63 + k, 0.56]], '#d5dade', 1.1, 'opacity=".6"');
	}
	s += pl(V, [[1.2, 0.86], [4.2, 0.86]], '#0b0c0e', 1.6, 'opacity=".8"') + pl(V, [[1.2, 0.8], [4.2, 0.8]], '#9aa0a8', 0.8, 'opacity=".5"');   // a milled top flat
	s += ao(shp(pth(V, R4(0.98, -1.04, 1.16, 1.04), 0.03), 'url(#brassCyl)', 'bS'));
	return s + wear(pth(V, CMP4, 0.05), '#b9bec5', 1.2, 0.5, '6 30 3 50');
}
function cmp4Front(V) {
	let s = ao(circ(V, 0, 0, 1.04, 'url(#rBrass)', 'bS'));
	s += ao(circ(V, 0, 0, 1.0, 'url(#rSteel)', 'mM'));
	s += circ(V, 0, 0, 0.72, 'url(#rGph)', 'mS') + ring(V, 0, 0, 0.72, '#2a2c30', 1.2);
	s += pl(V, [[-0.5, 0.86], [0.5, 0.86]], '#3a3d42', 1.4);   // the top flat, end on
	s += circ(V, 0, 0, 0.44, '#120a06') + `<g filter="url(#glow)">${circ(V, 0, 0, 0.28, 'url(#ember)', null)}</g>`;
	return s + ring(V, 0, 0, 0.46, '#d9dde1', 1.2, 'opacity=".8"');
}
function cmp4Back(V) {
	let s = ao(circ(V, 0, 0, 1.04, 'url(#rBrass)', 'bS')) + ao(circ(V, 0, 0, 0.9, 'url(#rGph)', 'mM'));
	s += ring(V, 0, 0, 0.8, '#1c1d20', 1.2) + circ(V, 0, 0, 0.74, '#0d0e0f');
	for (const r of [0.7, 0.62, 0.54]) s += ring(V, 0, 0, r, '#4a4d52', 1, 'opacity=".8"');
	return s + circ(V, 0, 0, 0.42, '#060607');
}

// ---- Magazine_20: short graphite 20 with a brass-showing witness window (gun space) --------------------------------------
const MF4 = [[17.6, -5.6], [17.85, -11], [18.6, -15.95]], MR4 = [[10.4, -5.6], [10.5, -11], [11.0, -15.5]];
const MAG4 = [...Array.from({ length: 21 }, (_, i) => bez(MF4, i / 20)), ...Array.from({ length: 21 }, (_, i) => bez(MR4, 1 - i / 20))];
const BASE4 = [[10.9, -15.4], [18.75, -15.8], cb([19.35, -15.85], [19.55, -16.25], [19.3, -16.65]), [19.0, -16.9], [11.5, -16.75], cb([11.0, -16.7], [10.75, -16.3], [10.9, -15.4])];
function m4Side(V) {
	const body = pth(V, MAG4, 0.25);
	let s = shp(body, 'url(#gph)', 'pM') + shp(body, 'url(#stip4)', null, 'opacity=".25"');
	s += shp(pth(V, [[10.4, -5.6], [17.6, -5.6], [edgeU(MF4, -6.25), -6.25], [edgeU(MR4, -6.25), -6.25]], 0.08), 'url(#alu)', 'mS');
	// a molded border inset round the side face
	const inset = [...Array.from({ length: 9 }, (_, i) => { const v = -6.7 - i * 0.875; return [edgeU(MF4, v) - 0.4, v]; }),
		...Array.from({ length: 9 }, (_, i) => { const v = -13.7 + i * 0.875; return [edgeU(MR4, v) + 0.4, v]; })];
	s += shp(pth(V, inset, 0.5), 'none', null, 'stroke="#1b1d20" stroke-width="1.3"') + shp(pth(V, inset, 0.5), 'none', null, 'stroke="#8d929a" stroke-width=".7" opacity=".4" transform="translate(0,1.3)"');
	// the witness window: brass rounds stacked behind smoked glass, index ticks beside it
	const w0 = v => edgeU(MR4, v) + 1.1, win = pth(V, [[w0(-7.0), -7.0], [w0(-7.0) + 1.25, -7.0], [w0(-13.6) + 1.25, -13.6], [w0(-13.6), -13.6]], 0.5);
	let rounds = shp(win, 'url(#smoke)', null);
	for (let v = -7.2, k = 0; v > -13.9; v -= 0.48, k++) rounds += shp(pth(V, R4(9 + (k % 2) * 0.3, v - 0.2, 16, v + 0.2), 0.1), 'url(#brassCyl)', null);
	rounds += `${full} fill="#1a2a36" opacity=".35"/>` + pl(V, [[w0(-7.6), -9.4], [w0(-7.6) + 1.6, -7.2]], '#ffffff', 0.12 * V.s, 'opacity=".28"');
	s += clip(win, rounds) + shp(win, 'none', null, 'stroke="#141518" stroke-width="2"') + shp(win, 'none', null, 'stroke="#a1a7af" stroke-width=".8" opacity=".5" transform="translate(0,1.5)"');
	for (const [k, v] of [[0, -7.6], [1, -9.2], [2, -10.8], [3, -12.4]]) {
		const a = w0(v) + 1.45;
		s += pl(V, [[a, v], [a + (k ? 0.45 : 0.75), v]], k ? '#e6e8ea' : '#5fd6f5', 1.4, `opacity="${k ? 0.75 : 1}"`);
	}
	for (const v of [-14.1, -14.6, -15.05]) s += pl(V, [[edgeU(MR4, v) + 0.45, v], [edgeU(MF4, v) - 0.45, v]], '#16171a', 1.4) + pl(V, [[edgeU(MR4, v) + 0.45, v - 0.06], [edgeU(MF4, v) - 0.45, v - 0.06]], '#8d929a', 0.7, 'opacity=".5"');
	// the machined base plate with its toe, a brass pin and a cyan inlay
	const base = cd(V, BASE4);
	s += ao(shp(base, 'url(#alu)', 'mS')) + clip(base, `${full} fill="url(#brush)"/>`) + pin(V, 12.0, -16.15, 0.15);
	s += accent(V, [[12.9, -16.2], [18.4, -16.32]], 0.09);
	return s + wear(body, '#9aa0a8', 1, 0.45, '3 30 2 50');
}
function m4End(V, back) {
	const bottom = back ? -15.5 : -15.95, sp = pth(V, R4(-1.2, bottom, 1.2, -5.6), 0.4);
	let s = shp(sp, 'url(#gph)', 'pM') + shp(sp, 'url(#stip4)', null, 'opacity=".25"');
	s += shp(pth(V, R4(-1.2, -6.25, 1.2, -5.6), 0.06), 'url(#alu)', 'mS');
	if (back) s += pl(V, [[0, -6.6], [0, bottom + 0.4]], '#16171a', 1.2) + pl(V, [[0.05, -6.6], [0.05, bottom + 0.4]], '#7a8087', 0.6, 'opacity=".5"');
	else s += shp(pth(V, R4(-0.32, -13.6, 0.32, -6.7), 0.25), 'url(#gphCylH)', 'pS');
	for (const v of [-14.1, -14.6, -15.05]) s += pl(V, [[-1.1, v], [1.1, v]], '#16171a', 1.4);
	const bp = pth(V, R4(-1.45, -16.9, 1.45, -15.6), 0.35);
	s += ao(shp(bp, 'url(#alu)', 'mS')) + clip(bp, `${full} fill="url(#brushV)"/>`);
	return s + (back ? '' : shp(pth(V, R4(-1.3, -16.75, 1.3, -16.45), 0.1), '#8e949b', null, 'opacity=".6"'));
}

// ---- Sight_RedDot: compact enclosed red dot on a machined clamp (gun space; window centre = SOCKET_Aim) --------------
const RV0 = 6.15, AIM = 8.95;
const HOUS4 = [[20.2, 7.0], [20.2, 10.0], cb([20.2, 10.6], [20.6, 10.8], [21.2, 10.8]), [24.6, 11.1], cb([25.6, 11.2], [26.3, 10.8], [26.45, 10.2]), [26.7, 7.6],
	cb([26.75, 7.2], [26.6, 7.0], [26.2, 7.0])];
const POCKET4 = [[21.0, 7.6], [21.0, 9.9], cb([21.0, 10.2], [21.2, 10.3], [21.5, 10.3]), [24.8, 10.5], cb([25.4, 10.5], [25.7, 10.2], [25.75, 9.8]), [25.9, 7.9],
	cb([25.92, 7.7], [25.8, 7.6], [25.6, 7.6])];
function rd4Side(V) {
	const right = V.kind === 'right';
	const base = pth(V, R4(19.9, RV0, 26.3, RV0 + 0.95), 0.25);
	let s = ao(shp(base, 'url(#alu)', 'mS')) + clip(base, `${full} fill="url(#brush)"/>`);
	s += pl(V, [[20.1, RV0 + 0.85], [26.1, RV0 + 0.85]], '#ffffff', 0.9, 'opacity=".6"');
	if (right) {   // the thumb nut that clamps it to the rail
		s += ao(circ(V, 23.1, RV0 + 0.5, 0.5, 'url(#rAlu)', 'mS')) + knurlRing(V, 23.1, RV0 + 0.5, 0.5, 28, '#6a6f76') + circ(V, 23.1, RV0 + 0.5, 0.16, 'url(#rBrass)');
	} else s += screw(V, 23.1, RV0 + 0.5, 0.22);
	const hd = cd(V, HOUS4);
	s += ao(shp(hd, 'url(#gph)', 'mM'));
	s += clip(hd, flt('soft', sk(V, [[20.6, 10.45], [24.6, 10.75], cb([25.4, 10.82], [25.9, 10.6], [26.1, 10.2])], '#ffffff', 0.18, 'opacity=".55"')));
	s += shp(cd(V, POCKET4), 'none', null, 'stroke="#17181b" stroke-width="1.4"') + `<path d="${cd(V, POCKET4)}" fill="none" stroke="#9aa0a8" stroke-width=".7" opacity=".45" transform="translate(0,1.2)"/>`;
	s += clip(hd, shp(pth(V, R4(20.0, 7.2, 20.5, 10.45), 0.1), 'url(#alu)', 'mS'));                 // rear window bezel
	s += clip(hd, shp(pth(V, [[26.0, 10.6], [26.8, 10.6], [26.8, 7.0], [26.3, 7.0]], 0.04), 'url(#alu)', 'mS'));   // front hood chamfer
	s += accent(V, [[20.7, 7.4], [26.25, 7.5]], 0.09);
	s += ao(shp(pth(V, R4(22.7, 10.9, 24.5, 11.5), 0.15), 'url(#aluCylH)', 'mS'));                // elevation cap, a brass top ring
	for (let a = 22.85; a < 24.4; a += 0.16) s += pl(V, [[a, 10.98], [a, 11.34]], '#6a6f76', 0.8, 'opacity=".6"');
	s += shp(pth(V, R4(22.7, 11.36, 24.5, 11.5), 0.05), 'url(#brass)', 'bS');
	if (right) {   // windage cap and battery tray
		s += ao(circ(V, 23.3, AIM - 0.1, 0.62, 'url(#rBrass)', 'bS')) + knurlRing(V, 23.3, AIM - 0.1, 0.62, 30, '#8a6420');
		s += pl(V, [[23.0, AIM - 0.35], [23.6, AIM + 0.15]], '#5a3c0e', 1.6);
		s += ao(shp(pth(V, R4(24.55, 7.85, 25.55, 9.95), 0.22), 'url(#alu)', 'mS')) + screw(V, 25.05, 8.25, 0.12);
	} else {       // brightness buttons, + and -
		for (const [a, plus] of [[21.6, true], [23.15, false]]) {
			s += ao(shp(pth(V, R4(a, 9.45, a + 1.2, 10.05), 0.26), 'url(#rubber)', 'rM'));
			s += pl(V, [[a + 0.42, 9.75], [a + 0.78, 9.75]], '#9aa0a8', 1.4);
			if (plus) s += pl(V, [[a + 0.6, 9.57], [a + 0.6, 9.93]], '#9aa0a8', 1.4);
		}
	}
	return s + wear(hd, '#b9bec5', 1, 0.4, '3 30 2 50');
}
function rd4End(V, back) {
	const base = pth(V, R4(-1.5, RV0, 1.5, RV0 + 0.95), 0.2);
	let s = ao(shp(base, 'url(#alu)', 'mS'));
	s += ao(shp(pth(V, R4(-2.25, RV0 + 0.05, -1.5, RV0 + 0.95), 0.15), 'url(#alu)', 'mS'));          // thumb nut (gun's right)
	for (let a = -2.15; a < -1.55; a += 0.1) s += pl(V, [[a, RV0 + 0.1], [a, RV0 + 0.9]], '#6a6f76', 0.8);
	s += ao(shp(pth(V, R4(1.5, RV0 + 0.25, 1.75, RV0 + 0.75), 0.06), 'url(#gph)', 'mS'));
	s += ao(shp(pth(V, R4(-0.9, 10.9, 0.9, 11.5), 0.15), 'url(#aluCylH)', 'mS')) + shp(pth(V, R4(-0.9, 11.36, 0.9, 11.5), 0.05), 'url(#brass)', 'bS');
	s += ao(shp(pth(V, R4(-1.55, 7.0, 1.55, 11.15), 0.55), 'url(#gph)', 'mM'));
	if (back) s += shp(pth(V, R4(-1.5, 7.0, 1.5, 10.8), 0.5), 'url(#gphLo)', 'mS');
	s += ao(shp(pth(V, R4(1.55, 9.45, 1.72, 10.1), 0.05), 'url(#rubber)', 'rM'));                    // buttons (gun's left)
	s += ao(shp(pth(V, R4(-1.78, AIM - 0.7, -1.55, AIM + 0.5), 0.05), 'url(#brassCylH)', 'bS'));    // windage cap (gun's right)
	const bez4 = pth(V, R4(-1.36, AIM - 1.68, 1.36, AIM + 1.68), 0.55), win = pth(V, R4(-1.2, AIM - 1.5, 1.2, AIM + 1.5), 0.45);
	s += ao(shp(bez4, 'url(#alu)', 'mS'));
	s += shp(win, 'url(#rLens)', null);
	if (back) {
		s += clip(win, pl(V, [[-1.0, AIM + 0.9], [0.2, AIM + 1.6]], '#ffffff', 0.12 * V.s, 'opacity=".18"'));
		s += `<g filter="url(#glow)">${circ(V, 0, AIM, 0.14, 'url(#redDot)', null)}</g>` + circ(V, 0, AIM, 0.05, '#fff4f0');
	} else {
		s += shp(win, 'url(#rCoat)', null);
		s += clip(win, pl(V, [[-1.1, AIM + 0.6], [0.1, AIM + 1.7]], '#ffffff', 0.14 * V.s, 'opacity=".3"'));
	}
	s += shp(win, 'none', null, 'stroke="#141518" stroke-width="1.6"');
	if (!back) s += accent(V, [[-1.4, 7.25], [1.4, 7.25]], 0.07);
	return s;
}

// ---- Stock_Cheek: adjustable length and cheek riser (gun space, behind the butt plate) --------------------------------
const BLOCK4 = [[-2.8, 1.7], [-2.8, -6.6], cb([-2.8, -8.2], [-4.4, -9.5], [-6.6, -9.75]), [-7.9, -9.85], cb([-8.35, -9.85], [-8.5, -9.6], [-8.5, -9.2]),
	[-8.5, 2.1], cb([-8.5, 2.55], [-8.1, 2.7], [-7.6, 2.7]), cb([-6.0, 2.7], [-4.2, 2.6], [-3.4, 2.35]), cb([-2.95, 2.2], [-2.8, 2.0], [-2.8, 1.7])];
const PAD4 = [[-8.55, 2.75], cb([-9.1, 2.85], [-9.6, 2.5], [-9.6, 1.7]), cb([-9.35, -2.0], [-9.35, -5.4], [-9.75, -8.8]), cb([-9.85, -9.7], [-9.35, -10.05], [-8.55, -10.0])];
const COMB4 = [[-1.2, 3.3], cb([-1.2, 4.1], [-1.7, 4.6], [-2.7, 4.6]), [-7.6, 4.45], cb([-8.2, 4.45], [-8.5, 4.15], [-8.5, 3.7]), [-8.5, 3.4],
	cb([-8.5, 3.15], [-8.3, 3.05], [-8.0, 3.05]), [-1.6, 3.05], cb([-1.35, 3.05], [-1.2, 3.15], [-1.2, 3.3])];
function st4Side(V) {
	const right = V.kind === 'right';
	let s = '';
	// two aluminium guide rods and a graphite locking bar with depth marks: the length adjustment
	for (const v of [0.6, -7.0]) s += ao(shp(pth(V, R4(-2.95, v - 0.42, 0.05, v + 0.42), 0.1), 'url(#aluCyl)', 'mS'));
	s += ao(shp(pth(V, R4(-2.95, -3.85, 0.05, -2.75), 0.1), 'url(#gphCyl)', 'mS'));
	for (const a of [-0.6, -1.2, -1.8, -2.4]) s += pl(V, [[a, -3.15], [a, -2.85]], '#e6e8ea', 1.1, 'opacity=".8"');
	s += ao(shp(pth(V, R4(0, -8.7, 0.35, 1.9), 0.08), 'url(#alu)', 'mS'));                          // seat plate
	// brass riser posts and the satin-white cheek comb
	for (const a of [-3.15, -7.15]) {
		s += ao(shp(pth(V, R4(a - 0.24, 2.3, a + 0.24, 3.25), 0.05), 'url(#brassCylH)', 'bS'));
		for (const v of [2.55, 2.8]) s += pl(V, [[a - 0.2, v], [a + 0.2, v]], '#6a4813', 0.9, 'opacity=".7"');
	}
	const comb = cd(V, COMB4);
	s += shp(comb, 'url(#wht)', 'pM') + clip(comb, flt('soft', sk(V, [[-1.8, 4.25], [-7.6, 4.15]], '#ffffff', 0.22, 'opacity=".8"')));
	s += clip(comb, accent(V, [[-1.5, 3.32], [-8.2, 3.32]], 0.09));
	// the block, two-tone like the body, and the pad behind it
	const pad = cd(V, PAD4);
	s += shp(pad, 'url(#rubber)', 'rM') + shp(pad, 'url(#stip4)', null, 'opacity=".35"');
	s += ao(shp(pth(V, R4(-8.7, -9.85, -8.4, 2.7), 0.06), 'url(#alu)', 'mS'));
	const blk = cd(V, BLOCK4);
	s += shp(blk, 'url(#wht)', 'pL');
	s += flt('pM', clip(blk, shp(cd(V, [[-10, -2.0], cb([-7, -2.2], [-4.5, -2.5], [-1, -2.6]), [-1, -12], [-10, -12]]), 'url(#gph)')));
	s += clip(blk, `${full} fill="url(#form4)" opacity=".7"/>` + flt('soft', sk(V, [[-3.3, 1.9], [-8.0, 2.05]], '#ffffff', 0.25, 'opacity=".7"')) +
		accent(V, [[-1, -2.2], cb([-4.5, -2.1], [-7, -1.8], [-10, -1.6])], 0.12));
	if (right) {   // the riser's lock knob
		s += ao(circ(V, -5.6, 0.4, 0.85, 'url(#rAlu)', 'mS')) + knurlRing(V, -5.6, 0.4, 0.85, 36, '#6a6f76');
		s += circ(V, -5.6, 0.4, 0.32, 'url(#rBrass)', 'bS') + pl(V, [[-5.6, 0.95], [-5.6, 1.2]], '#5fd6f5', 1.6);
	} else {       // a flush index plate with height marks
		const ip = pth(V, R4(-7.4, -0.6, -3.8, 1.4), 0.25);
		s += shp(ip, 'url(#alu)', 'mS') + clip(ip, `${full} fill="url(#brush)"/>`) + shp(ip, 'none', null, 'stroke="#3b3e43" stroke-width="1.1"');
		for (let k = 0; k < 6; k++) s += pl(V, [[-6.9 + k * 0.5, 0.0], [-6.9 + k * 0.5, k % 2 ? 0.5 : 0.85]], '#3a3d42', 1.1);
		s += pin(V, -4.3, 0.4, 0.17);
	}
	s += qd(V, -5.6, -7.3, 0.45);
	return s + wear(pad, '#7a7d82', 1.2, 0.5, '4 24 2 40');
}
function st4End(V, back) {
	let s = '';
	const comb = () => {
		let c = ao(shp(pth(V, R4(-0.24, 2.3, 0.24, 3.25), 0.05), 'url(#brassCylH)', 'bS'));
		const cs = pth(V, R4(-1.5, 3.05, 1.5, 4.6), 0.6);
		c += shp(cs, 'url(#whtMid)', 'pM') + clip(cs, flt('soft', sk(V, [[-1.0, 4.3], [1.0, 4.3]], '#ffffff', 0.2, 'opacity=".8"')));
		return c;
	};
	const knob = () => ao(shp(pth(V, R4(-3.05, -0.45, -2.45, 1.25), 0.15), 'url(#aluCyl)', 'mS')) +
		Array.from({ length: 6 }, (_, k) => pl(V, [[-3.0, -0.3 + k * 0.28], [-2.5, -0.3 + k * 0.28]], '#6a6f76', 0.9)).join('');
	const padD = pth(V, R4(-2.6, -10.05, 2.6, 2.85), 1.0), blkD = pth(V, R4(-2.4, -9.7, 2.4, 2.65), 0.9);
	if (back) {
		s += comb() + flt('pL', shp(blkD, 'url(#whtLo)')) + knob();
		s += shp(padD, 'url(#rRubber)', 'rM') + shp(padD, 'url(#stip4)', null, 'opacity=".4"');
		s += clip(padD, Array.from({ length: 15 }, (_, k) => { const v = -9.2 + k * 0.78; return pl(V, [[-2.6, v], [2.6, v]], '#101012', 1.4) + pl(V, [[-2.6, v - 0.08], [2.6, v - 0.08]], '#5c5c60', 0.7, 'opacity=".5"'); }).join(''));
		for (const v of [-7.4, 0.6]) s += ao(circ(V, 0, v, 0.34, 'url(#rAlu)', 'mS')) + circ(V, 0, v, 0.12, '#2a2c30');
		s += wear(padD, '#7a7d82', 1.2, 0.5, '4 24 2 40');
	} else {
		s += shp(padD, 'url(#rRubber)', 'rM') + comb();
		s += xsec(V, [[-2.4, 2.65], [2.4, 2.65], [2.4, -9.7], [-2.4, -9.7]].map(p => p), 'url(#whtMid)', 'url(#gph)', -2.3, 'pL');
		s += clip(blkD, accent(V, [[-2.4, -1.95], [-2.15, -1.95]]) + accent(V, [[2.15, -1.95], [2.4, -1.95]]));
		s += knob();
		// the seat plate (the interface): guide slots, screw holes, the locating boss at (0, -3.3)
		const sp = pth(V, R4(-2.45, -8.7, 2.45, 1.9), 0.6);
		s += ao(shp(sp, 'url(#alu)', 'mM')) + clip(sp, `${full} fill="url(#brushV)"/>`);
		for (const v of [-6.6, 0.2]) s += circ(V, 0, v, 0.34, '#101114') + ring(V, 0, v, 0.27, '#5a6068', 0.9);
		s += ao(circ(V, 0, -3.3, 0.6, 'url(#rSteel)', 'mS')) + circ(V, 0, -3.3, 0.25, '#2a2c30');
		for (const sd of [-1, 1]) s += shp(pth(V, R4(sd * 1.5, -7.6, sd * 1.9, 1.0), 0.1), '#3a3e44', null);
	}
	return s;
}

// ---- the parts (keys and folders as before; b = bounds in cm, gun or part space) -------------------------------------
const PARTS = [
	{ dir: 'Body_Marksman', b: { u: [-0.3, 51.9], v: [-16.1, 6.15], w: [-2.75, 2.75] }, draw: { left: body4Side, right: body4Side, front: body4Front, back: body4Back } },
	{ dir: 'Barrel_Marksman', b: { u: [0, 24], v: [-1.6, 1.6], w: [-1.6, 1.6] }, draw: { left: brl4Side, right: brl4Side, front: brl4Front, back: brl4Back } },
	{ dir: 'Muzzle_Compensator', b: { u: [0, 5.0], v: [-1.04, 1.04], w: [-1.04, 1.04] }, draw: { left: cmp4Side, right: cmp4Side, front: cmp4Front, back: cmp4Back } },
	{ dir: 'Magazine_20', b: { u: [10.4, 19.55], v: [-16.9, -5.6], w: [-1.45, 1.45] }, draw: { left: m4Side, right: m4Side, front: V => m4End(V, false), back: V => m4End(V, true) } },
	{ dir: 'Sight_RedDot', b: { u: [19.9, 26.75], v: [6.15, 11.5], w: [-2.25, 1.75] }, draw: { left: rd4Side, right: rd4Side, front: V => rd4End(V, false), back: V => rd4End(V, true) } },
	{ dir: 'Stock_Cheek', b: { u: [-9.85, 0.35], v: [-10.05, 4.6], w: [-3.05, 2.6] }, draw: { left: st4Side, right: st4Side, front: V => st4End(V, false), back: V => st4End(V, true) } },
];
const VIEWS = ['front', 'back', 'left', 'right'];
const viewPage = (svg, bg) => `<!doctype html><html><head><meta charset="utf-8"><style>html,body{margin:0;background:${bg}}svg{display:block}</style></head>
	<body><svg xmlns="http://www.w3.org/2000/svg" width="${N}" height="${N}" viewBox="0 0 ${N} ${N}">${svg}</svg></body></html>`;

(async () => {
	const want = process.argv.slice(2).filter(a => !a.startsWith('--'));
	const browser = await chromium.launch();
	const pg = await browser.newPage({ viewport: { width: N, height: N } });
	for (const p of PARTS) {
		if (want.length && !want.includes(p.dir)) continue;
		const dir = path.join(__dirname, p.dir);
		for (const [sub, bg] of [['upload', 'transparent'], ['upload_white', '#ffffff']]) {
			fs.mkdirSync(path.join(dir, sub), { recursive: true });
			for (const vk of VIEWS) {
				const V = makeView(vk, p.b);
				const file = path.join(dir, `.${vk}.s4.html`);
				fs.writeFileSync(file, viewPage(defs(V) + defs4(V) + p.draw[vk](V), bg));
				await pg.goto('file:///' + file.split('\\').join('/'));
				await pg.screenshot({ path: path.join(dir, sub, `${vk}.png`), omitBackground: bg === 'transparent' });
				fs.unlinkSync(file);
			}
		}
		console.log('wrote', dir);
	}
	await browser.close();
})();
