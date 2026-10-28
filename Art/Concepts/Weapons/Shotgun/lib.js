// Shared drawing kit for the Ranchhand (shotgun) Tripo concept views (2026-10-28): the AR scripts' helpers
// (AR/render_set4.js lines 1-250: views, paths, gradients, bevel filters, screws, seams, accents; AR/render_set3.js:
// pipes, welds, rust, tape, wire, springs), gathered as a module for Shotgun/render.js and its parts_set<N>.js files.
// Gun space, cm: u along the gun from the back of the receiver, v up from the bore, w to the gun's left.
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const ROOT = path.resolve(__dirname, '../../../..');
const FONT = p => 'file:///' + path.join(ROOT, p).replace(/\\/g, '/');
// --upload: clean Tripo images only (upload/ transparent, upload_white/ on white): the part alone, filling ~84%, no
// contact shadows or glow spill past its edge, no background-coloured touches; the grey views and sheets are left alone.
const UPLOAD = true;   // Tripo upload images only
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


// ---- set 2 (the user, 2026-10-27: a second, different-looking set; same attach points, so every part fits every part) --
function rng(seed) {
	return () => {
		seed = (seed + 0x6D2B79F5) | 0;
		let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
		t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
		return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
	};
}
// An irregular blob (paint chips, scuffs).
function blob(V, cu, cv, r, rnd, n = 9) {
	const pts = Array.from({ length: n }, (_, i) => {
		const a = i / n * Math.PI * 2, rr = r * (0.55 + 0.6 * rnd());
		return [cu + rr * Math.cos(a), cv + rr * Math.sin(a) * 0.7];
	});
	return pth(V, pts, r * 0.25);
}
let clipN = 0;
const inside = (d, inner) => { const id = 'in' + clipN++; return `<clipPath id="${id}"><path d="${d}"/></clipPath><g clip-path="url(#${id})">${inner}</g>`; };
// A round tube along points (pipes, wire loops): a dark body and a highlight along its top.
const tube = (V, pts, r, col, hi) => ao(pl(V, pts, '#151515', 2 * r * V.s + 2) + pl(V, pts, col, 2 * r * V.s)) +
	pl(V, pts.map(([a, v]) => [a, v + r * 0.4]), hi, r * 0.55 * V.s, 'opacity=".7"');
// An engraved spiral curl (Saint Maud's scrollwork).
function curl(V, cu, cv, r0, turns, dir, col, px) {
	const pts = Array.from({ length: 28 }, (_, k) => {
		const t = k / 27, a = Math.PI * 2 * turns * t, r = r0 * (1 - 0.8 * t);
		return [cu + dir * r * Math.cos(a), cv + r * Math.sin(a)];
	});
	return pl(V, pts, col, px);
}

// ==== Windfall (set 3), redrawn 2026-10-27 ===========================================================================
// The user rejected the first set 3 (set 1's outlines recoloured). Six new scrap-built designs: an exposed tube frame,
// welded salvage plates, asymmetric, springs, wire and zip-ties, mismatched panels. Only the interfaces stay (gun space,
// cm): barrel-nut face (51.9, 0) radius 1.25; the stock's locating boss (0, -3.3) on a butt plate at u 0; magazine well
// top u 10.4-17.6 at v -5.6, 2.8 wide; rail u 14-32, top v 6.15, sight origin (23, 6.15); grip hand (22.5, -10).
// Materials: rusted steel, galvanized tin (spangle), sea-teal and sky-blue paint over rust, black friction tape, tan tape,
// brass wire, a little ember.
const CYL = {
	rust: [[0, '#2a150b'], [0.18, '#5e3320'], [0.3, '#a8694a'], [0.42, '#73412a'], [0.78, '#4a2718'], [1, '#1e0f08']],
	steel: [[0, '#2c2e31'], [0.2, '#6c6e70'], [0.31, '#c6c9cc'], [0.43, '#7d8083'], [0.8, '#45474a'], [1, '#222426']],
	galv: [[0, '#596066'], [0.2, '#a3abb1'], [0.31, '#eef2f4'], [0.43, '#aeb5ba'], [0.8, '#7a8288'], [1, '#4c5257']],
	brass: [[0, '#5f400f'], [0.18, '#b8862f'], [0.3, '#ffe9b0'], [0.42, '#d9a64e'], [0.78, '#9a6e25'], [1, '#4f340b']],
	tape: [[0, '#101010'], [0.22, '#33322f'], [0.33, '#5d5b56'], [0.48, '#2e2d2b'], [1, '#101010']],
	tan: [[0, '#4a3c2c'], [0.22, '#9a8466'], [0.33, '#c4ae8c'], [0.48, '#8f7a5d'], [1, '#43362a']],
	teal: [[0, '#1f3a38'], [0.2, '#4f8f8a'], [0.31, '#8fc4be'], [0.43, '#5a948e'], [1, '#1f3a38']],
	rubber: [[0, '#0c0c0c'], [0.22, '#2c2c2c'], [0.33, '#4a4a4a'], [0.5, '#262626'], [1, '#0c0c0c']],
};
GRADS.push(
	lin('galv', [[0, '#dfe4e7'], [0.5, '#adb4b9'], [1, '#7f878d']]),
	lin('sky', [[0, '#a5c1d3'], [0.5, '#7d9fb5'], [1, '#587a90']]),
	lin('tealPaint', [[0, '#6faaa4'], [0.5, '#4f8f8a'], [1, '#376a66']]),
	lin('rustDeep', [[0, '#6e3e26'], [0.5, '#55301e'], [1, '#3a1d10']]),
	lin('rust3', [[0, '#8c5537'], [0.5, '#6d3c25'], [1, '#4a2618']]),
	lin('steelDark', [[0, '#7a7d81'], [0.5, '#5a5d61'], [1, '#3a3c40']]),
	lin('tapeBlack', [[0, '#403f3c'], [0.5, '#2b2a28'], [1, '#181716']]),
	lin('tanTape', [[0, '#c2ad8c'], [0.5, '#a08a6a'], [1, '#7a6750']]),
	lin('hazard', [[0, '#ecc04a'], [1, '#c4952a']]),
	lin('lensAmber', [[0, '#5a8e9a', 0.85], [0.5, '#2d5560', 0.8], [1, '#1b3138', 0.9]]),
	rad('rRust', [[0, '#b07a58'], [0.45, '#73412a'], [1, '#33190c']]),
	rad('rGalv', [[0, '#f4f7f9'], [0.45, '#aab1b6'], [1, '#5f666b']]),
	rad('rTeal', [[0, '#9fd0ca'], [0.45, '#4f8f8a'], [1, '#1f3a38']]),
	rad('rTape', [[0, '#5a5853'], [0.5, '#2b2a28'], [1, '#121212']]),
	rad('rTan', [[0, '#d2bd9a'], [0.5, '#9a8466'], [1, '#4a3c2c']]),
	(() => {   // galvanized spangle: big pale and dark zinc crystals
		const rnd = rng(5);
		let p = '';
		for (let k = 0; k < 11; k++) {
			const cx = rnd() * 34, cy = rnd() * 28, r = 4 + rnd() * 7;
			const pts = Array.from({ length: 6 }, (_, i) => { const a = i / 6 * 6.283 + rnd() * 0.7, rr = r * (0.55 + 0.55 * rnd()); return [cx + rr * Math.cos(a), cy + rr * Math.sin(a)]; });
			p += `<path d="${rounded(pts, 0)}" fill="${k % 2 ? '#000000' : '#ffffff'}" opacity="${(0.07 + rnd() * 0.12).toFixed(2)}"/>`;
		}
		return `<pattern id="spangle" width="34" height="28" patternUnits="userSpaceOnUse">${p}</pattern>`;
	})(),
	// rust: dark scale and orange bloom; zinc: sharp-edged spangle patches (noise, so nothing tiles)
	'<filter id="rustN" filterUnits="userSpaceOnUse" x="0" y="0" width="1024" height="1024"><feTurbulence type="fractalNoise" baseFrequency="0.085" numOctaves="4" seed="3"/><feColorMatrix type="matrix" values="0 0 0 0 0.16  0 0 0 0 0.07  0 0 0 0 0.03  3.2 0 0 0 -1.4"/></filter>',
	'<filter id="rustHiN" filterUnits="userSpaceOnUse" x="0" y="0" width="1024" height="1024"><feTurbulence type="fractalNoise" baseFrequency="0.12" numOctaves="3" seed="9"/><feColorMatrix type="matrix" values="0 0 0 0 0.69  0 0 0 0 0.37  0 0 0 0 0.18  0 3 0 0 -1.72"/></filter>',
	'<filter id="zincN" filterUnits="userSpaceOnUse" x="0" y="0" width="1024" height="1024"><feTurbulence type="fractalNoise" baseFrequency="0.028" numOctaves="2" seed="21"/><feColorMatrix type="matrix" values="0 0 0 0 1  0 0 0 0 1  0 0 0 0 1  9 0 0 0 -4.6"/><feComponentTransfer><feFuncA type="linear" slope=".2"/></feComponentTransfer></filter>',
	'<filter id="zincD" filterUnits="userSpaceOnUse" x="0" y="0" width="1024" height="1024"><feTurbulence type="fractalNoise" baseFrequency="0.028" numOctaves="2" seed="21"/><feColorMatrix type="matrix" values="0 0 0 0 0  0 0 0 0 0  0 0 0 0 0  0 -9 0 0 4.4"/><feComponentTransfer><feFuncA type="linear" slope=".16"/></feComponentTransfer></filter>',
	'<pattern id="cloth" width="4" height="4" patternUnits="userSpaceOnUse"><path d="M0,0L4,4M4,0L0,4" stroke="#000" stroke-width=".6" opacity=".28"/></pattern>',
);

const ZINC = `<rect x="0" y="0" width="${N}" height="${N}" filter="url(#zincN)"/><rect x="0" y="0" width="${N}" height="${N}" filter="url(#zincD)"/>`;
const RUSTTEX = `<rect x="0" y="0" width="${N}" height="${N}" filter="url(#rustN)"/><rect x="0" y="0" width="${N}" height="${N}" filter="url(#rustHiN)"/>`;
// ---- set 3 helpers --------------------------------------------------------------------------------------------------
let pgN = 0;
// A straight pipe between two points, shaded across its axis whatever its angle (its own user-space gradient).
function pipe(V, p0, p1, r, mat, filt = 'mS') {
	const [x0, y0] = V.P(...p0), [x1, y1] = V.P(...p1);
	const len = Math.hypot(x1 - x0, y1 - y0) || 1, R = r * V.s;
	let nx = -(y1 - y0) / len, ny = (x1 - x0) / len;
	if (ny > 0 || (Math.abs(ny) < 1e-6 && nx > 0)) { nx = -nx; ny = -ny; }
	const mx = (x0 + x1) / 2, my = (y0 + y1) / 2, id = 'pg' + pgN++;
	const g = `<linearGradient id="${id}" gradientUnits="userSpaceOnUse" x1="${r2(mx + nx * R)}" y1="${r2(my + ny * R)}" x2="${r2(mx - nx * R)}" y2="${r2(my - ny * R)}">${CYL[mat]
		.map(([o, c]) => `<stop offset="${o}" stop-color="${c}"/>`).join('')}</linearGradient>`;
	const d = rounded([[x0 + nx * R, y0 + ny * R], [x1 + nx * R, y1 + ny * R], [x1 - nx * R, y1 - ny * R], [x0 - nx * R, y0 - ny * R]], Math.min(R * 0.15, 2));
	return g + ao(shp(d, `url(#${id})`, filt));
}
const pipeD = (V, p0, p1, r) => {
	const [x0, y0] = V.P(...p0), [x1, y1] = V.P(...p1), len = Math.hypot(x1 - x0, y1 - y0) || 1, R = r * V.s;
	const nx = -(y1 - y0) / len * R, ny = (x1 - x0) / len * R;
	return rounded([[x0 + nx, y0 + ny], [x1 + nx, y1 + ny], [x1 - nx, y1 - ny], [x0 - nx, y0 - ny]], 0);
};
// A bent rod or wire (solid colours: a dark edge, the body, a highlight along its upper side).
const rod = (V, pts, r, col, hi) => ao(pl(V, pts, '#141210', 2 * r * V.s + 1.6) + pl(V, pts, col, 2 * r * V.s)) +
	pl(V, pts.map(([a, v]) => [a, v + r * 0.35]), hi, Math.max(0.8, r * 0.5 * V.s), 'opacity=".75"');
// Weld bead: heat tint, then overlapping ripples along a polyline.
function weld(V, pts, step = 0.2, r = 0.11) {
	r = Math.min(r, 3.2 / V.s); step = Math.min(step, r * 1.7);
	let s = pl(V, pts, '#2a1a10', r * 3.4 * V.s, 'opacity=".32"');
	for (let i = 0; i + 1 < pts.length; i++) {
		const [a0, v0] = pts[i], [a1, v1] = pts[i + 1], n = Math.max(1, Math.ceil(Math.hypot(a1 - a0, v1 - v0) / step));
		for (let k = 0; k <= n; k++) {
			const a = a0 + (a1 - a0) * k / n, v = v0 + (v1 - v0) * k / n;
			s += circ(V, a, v, r, '#4f463e') + circ(V, a, v + r * 0.3, r * 0.5, '#b9afa2', null, 'opacity=".7"');
		}
	}
	return s;
}
// Rust inside an outline: blotches, streaks and pits.
function rustOn(V, d, [a0, a1, v0, v1], seed, n, cols = ['#5a2c17', '#b06a40', '#7a3e22', '#3e1d0e']) {
	const rnd = rng(seed), k0 = Math.max(0.15, Math.min(1, 20 / V.s)), tex = cols[0] === '#5a2c17';
	let t = tex ? RUSTTEX : '';
	for (let k = 0; k < n; k++) {
		const a = a0 + rnd() * (a1 - a0), v = v0 + rnd() * (v1 - v0), r = (0.1 + rnd() * 0.4) * k0;
		t += shp(blob(V, a, v, r, rnd), cols[k % cols.length], null, `opacity="${(0.35 + rnd() * 0.45).toFixed(2)}"`);
	}
	for (let k = 0; k < n; k++) t += circ(V, a0 + rnd() * (a1 - a0), v0 + rnd() * (v1 - v0), (0.03 + rnd() * 0.05) * k0, '#1e0e06', null, 'opacity=".65"');
	return inside(d, t);
}
// Paint chips: the metal under a painted panel showing through.
function chipsOn(V, d, [a0, a1, v0, v1], seed, n, cols = ['#8f4a2a', '#a7aaad', '#6e3a22']) {
	const rnd = rng(seed);
	let t = '';
	for (let k = 0; k < n; k++) {
		const a = a0 + rnd() * (a1 - a0), v = v0 + rnd() * (v1 - v0), r = (0.1 + rnd() * 0.35) * Math.max(0.2, Math.min(1, 20 / V.s));
		t += shp(blob(V, a, v, r, rnd), cols[k % cols.length], null, 'opacity=".85"') + shp(blob(V, a + 0.03, v - 0.03, r * 0.9, rnd), 'none', null, 'stroke="#000" stroke-width=".8" opacity=".25"');
	}
	return inside(d, t);
}
// Rust run-off below a fixing.
const drip = (V, a, v, len) => shp(pth(V, [[a - 0.09, v], [a + 0.09, v], [a + 0.03, v - len], [a - 0.02, v - len]], 0.04), '#7a3e22', null, 'opacity=".45"');
// Brass wire wound round a cylinder (axis along u, centre vc, radius R).
function wireWrap(V, u0, u1, vc, R, pitch = 0.15, wr = 0.065) {
	wr = Math.min(wr, 2.2 / V.s); pitch = Math.min(pitch, wr * 2.3);
	let s = '';
	for (let u = u0; u <= u1 + 1e-6; u += pitch) {
		const p = [[u, vc - R], [u + pitch * 0.55, vc + R]];
		s += pl(V, p, '#3a2608', 2 * wr * V.s + 1.4) + pl(V, p, '#d29d48', 2 * wr * V.s) +
			pl(V, [[u + pitch * 0.32, vc + R * 0.15], [u + pitch * 0.47, vc + R * 0.75]], '#fff1c2', Math.max(0.7, wr * V.s * 0.8), 'opacity=".85"');
	}
	return s;
}
// Tape bands (slanted wraps) laid over an outline.
function tapeBands(V, d, [a0, a1, v0, v1], pitch, lean, edge = '#0a0a0a', hi = '#77746e') {
	let t = `<rect x="0" y="0" width="${N}" height="${N}" fill="url(#cloth)"/>`;
	for (let v = v0; v <= v1; v += pitch) t += pl(V, [[a0, v], [a1, v + lean]], edge, 1.6, 'opacity=".85"') + pl(V, [[a0, v + 0.05], [a1, v + lean + 0.05]], hi, 1.0, 'opacity=".55"');
	return inside(d, t);
}
// A worm-drive hose clamp round a cylinder: perforated band, the housing on top or not.
function hoseClamp(V, u, vc, R, housing) {
	let s = ao(shp(pth(V, R4(u - 0.33, vc - R - 0.07, u + 0.33, vc + R + 0.07), 0.05), 'url(#galv)', 'mS'));
	for (let v = vc - R + 0.15; v < vc + R - 0.1; v += 0.3) s += shp(pth(V, R4(u - 0.2, v, u + 0.2, v + 0.1), 0.02), '#2a2c30', null, 'opacity=".7"');
	if (housing) s += ao(shp(pth(V, R4(u - 0.45, vc + R - 0.05, u + 0.45, vc + R + 0.42), 0.08), 'url(#galv)', 'mS')) + ao(circ(V, u, vc + R + 0.2, 0.17, 'url(#rGalv)', 'mS')) + pl(V, [[u - 0.1, vc + R + 0.2], [u + 0.1, vc + R + 0.2]], '#1a1b1d', 1.4);
	return s;
}
// A black zip-tie round a cylinder; its head on the near side when `head`.
function zipTie(V, u, vc, R, head) {
	let s = ao(shp(pth(V, R4(u - 0.1, vc - R - 0.06, u + 0.1, vc + R + 0.06), 0.04), '#1b1b1c', 'pS'));
	for (let v = vc - R; v < vc + R; v += 0.12) s += pl(V, [[u - 0.08, v], [u + 0.08, v]], '#3c3c3e', 0.8, 'opacity=".7"');
	if (head) s += ao(shp(pth(V, R4(u - 0.22, vc - 0.2, u + 0.22, vc + 0.2), 0.05), '#1e1e1f', 'pS')) + pl(V, [[u + 0.2, vc - 0.05], [u + 0.75, vc - 0.32]], '#1b1b1c', 0.12 * V.s);
	return s;
}
// A coil spring along u; `inner` (a guide rod) is drawn between its far and near halves.
function coil(V, a0, a1, vc, R, turns, wr, inner = '') {
	const p = (a1 - a0) / turns, flip = V.kind === 'left';
	let back = '', front = '';
	for (let i = 0; i < turns; i++) {
		const a = a0 + i * p;
		const near = [[a, vc - R], [a + p / 2, vc + R]], far = [[a + p / 2, vc + R], [a + p, vc - R]];
		const [n, f] = flip ? [far, near] : [near, far];
		back += pl(V, f, '#141516', wr * V.s * 2 + 1.2) + pl(V, f, '#4d5054', wr * V.s * 2);
		front += pl(V, n, '#101112', wr * V.s * 2 + 1.8) + pl(V, n, '#a9adb2', wr * V.s * 2) +
			pl(V, [n[0].map((x, j) => x + (n[1][j] - x) * 0.25), n[0].map((x, j) => x + (n[1][j] - x) * 0.55)], '#ffffff', Math.max(0.8, wr * V.s * 0.7), 'opacity=".6"');
	}
	return back + inner + front;
}
const popRivet = (V, a, v, r = 0.2) => ao(circ(V, a, v, r, 'url(#rGalv)', 'mS')) + circ(V, a, v, r * 0.32, '#2a2c30');
const hexHead = (V, a, v, r = 0.3, fill = 'url(#rSteel)') => {
	const [x, y] = V.P(a, v), h = r * V.s;
	const hex = Array.from({ length: 6 }, (_, i) => [x + h * Math.cos(i * Math.PI / 3 + 0.3), y + h * Math.sin(i * Math.PI / 3 + 0.3)]);
	return ao(`<path d="${rounded(hex, 0)}" fill="${fill}" filter="url(#mS)"/>`) + circ(V, a, v, r * 0.45, 'none', null, `stroke="#2a2c30" stroke-width="1" opacity=".6"`);
};
const discD = (V, a, v, r) => { const [x, y] = V.P(a, v), R = r * V.s; return `M${r2(x - R)},${r2(y)}a${r2(R)},${r2(R)} 0 1,0 ${r2(2 * R)},0a${r2(R)},${r2(R)} 0 1,0 ${r2(-2 * R)},0Z`; };

// ---- extra cylinder looks for pipe() (any angle) ----------------------------------------------------------------------
Object.assign(CYL, {
	iron: [[0, '#141516'], [0.2, '#3f3d3b'], [0.31, '#76726d'], [0.42, '#3b3936'], [0.75, '#232120'], [1, '#100f0f']],
	ox: [[0, '#3e140f'], [0.22, '#7d2f24'], [0.33, '#b4584a'], [0.48, '#7d2f24'], [1, '#3a120d']],
	ivory: [[0, '#8f8670'], [0.2, '#d8cfb6'], [0.32, '#fbf6e8'], [0.45, '#e2d9c2'], [0.8, '#b3aa92'], [1, '#7a725e']],
	gph: [[0, '#141518'], [0.2, '#3d4046'], [0.31, '#868b93'], [0.42, '#43474d'], [0.75, '#26282c'], [1, '#111214']],
	alu: [[0, '#686d74'], [0.2, '#b8bdc4'], [0.32, '#f7f9fa'], [0.45, '#c3c8ce'], [0.8, '#8f949b'], [1, '#5c6167']],
	blued: [[0, '#1f2328'], [0.2, '#4b535d'], [0.31, '#8d96a0'], [0.42, '#4e5660'], [0.75, '#2c3137'], [1, '#181b1f']],
	black: [[0, '#17181a'], [0.2, '#46484d'], [0.31, '#82858b'], [0.42, '#3f4145'], [0.75, '#252629'], [1, '#121314']],
	ember: [[0, '#8a300c'], [0.3, '#ff8a2e'], [0.5, '#ffe9a8'], [0.7, '#ff8a2e'], [1, '#8a300c']],
	shell: [[0, '#4a130c'], [0.22, '#a8382a'], [0.33, '#e0705e'], [0.48, '#b8402f'], [1, '#4a130c']],
	gilt: [[0, '#6b5320'], [0.2, '#c9a24a'], [0.31, '#fff4cf'], [0.43, '#e6c06a'], [0.8, '#a8843a'], [1, '#5c4618']],
});
// Set 1 (Saint Maud): ivory enamel, cast iron, gilt brass (MAKERS.md #e8e0cc, #3a3633, #f6dfa0).
GRADS.push(
	lin('ivory', [[0, '#fbf7ec'], [0.4, '#ece4cf'], [1, '#cfc4a8']]),
	lin('ivoryHi', [[0, '#fffdf6'], [1, '#efe8d6']]),
	lin('ivoryMid', [[0, '#ebe3cd'], [1, '#d9cfb4']]),
	lin('ivoryLo', [[0, '#d6ccb0'], [1, '#b5aa8c']]),
	lin('ironCyl2', [[0, '#141516'], [0.2, '#3f3d3b'], [0.31, '#76726d'], [0.42, '#3b3936'], [0.75, '#232120'], [1, '#100f0f']]),
	lin('iron', [[0, '#57524d'], [0.5, '#3a3633'], [1, '#24211f']]),
	lin('giltCyl', [[0, '#6b5320'], [0.2, '#c9a24a'], [0.31, '#fff4cf'], [0.43, '#e6c06a'], [0.8, '#a8843a'], [1, '#5c4618']]),
	lin('giltCylH', [[0, '#6b5320'], [0.2, '#c9a24a'], [0.31, '#fff4cf'], [0.43, '#e6c06a'], [0.8, '#a8843a'], [1, '#5c4618']], false),
	lin('shellCyl', [[0, '#4a130c'], [0.22, '#a8382a'], [0.33, '#e0705e'], [0.48, '#b8402f'], [1, '#4a130c']]),
	lin('shellCylH', [[0, '#4a130c'], [0.22, '#a8382a'], [0.33, '#e0705e'], [0.48, '#b8402f'], [1, '#4a130c']], false),
	lin('emberCylH', [[0, '#8a300c'], [0.3, '#ff8a2e'], [0.5, '#ffe9a8'], [0.7, '#ff8a2e'], [1, '#8a300c']], false),
	lin('accentLin', [[0, '#ffffff'], [0.5, '#f1eee6'], [1, '#c9c4b6']]),
	rad('rIvory', [[0, '#ffffff'], [0.5, '#ece4cf'], [1, '#a99f84']]),
	rad('rShell', [[0, '#f08a78'], [0.5, '#b8402f'], [1, '#5a1a10']]),
	rad('rEmber', [[0, '#fff6d2'], [0.35, '#ffc25c'], [0.75, '#ff7a24'], [1, '#b8420f']], 0.5, 0.5, 0.6),
);
// A pale glowing rarity accent line (the in-game GunAccentGlow takes the rarity colour), in a dark channel.
const accentPale = (V, segs, cm = 0.13) => sk(V, segs, '#1a1714', cm * 1.9, 'opacity=".8"') + sk(V, segs, 'url(#accentLin)', cm) + sk(V, segs, '#ffffff', cm * 0.35, 'opacity=".9"');
// A cylinder seen side on (axis along u from a0 to a1, centre v, radius r), shaded across v; mat = a 'xxxCyl' gradient id.
const cylSide = (V, a0, a1, v, r, grad, filt = 'mS', rr = 0.05) => ao(shp(pth(V, R4(a0, v - r, a1, v + r), rr), `url(#${grad})`, filt));
// The same seen end on: a disc shaded radially.
const disc = (V, a, v, r, grad, filt = 'mS') => ao(circ(V, a, v, r, `url(#${grad})`, filt));
// A red shotgun shell seen side on, standing along v (brass head at the bottom) or lying along u.
function shellUp(V, a, v0, r = 0.95) {
	return ao(shp(pth(V, R4(a - 1.02, v0, a + 1.02, v0 + 1.1), 0.08), 'url(#brassCylH)', 'bS')) +
		ao(shp(pth(V, R4(a - r, v0 + 1.0, a + r, v0 + 6.0), 0.18), 'url(#shellCylH)', 'pS')) +
		pl(V, [[a - r, v0 + 1.6], [a + r, v0 + 1.6]], '#5a1a10', 0.9, 'opacity=".5"');
}

module.exports = {
	fs, path, chromium, ROOT, FONT, UPLOAD, N, FILL, makeView, r2, rounded, pth, R4, F, shp, circ, ring, pl, wear, bez, edgeU,
	star, lin, rad, GRADS, bevel, defs, ao, screw, rivet, RAIL_X, cb, cd, sk, clip, flt, full, seam, accent, knurlRing, qd,
	pin, G4, defs4, rng, blob, inside, tube, curl, CYL, ZINC, RUSTTEX, pipe, pipeD, rod, weld, rustOn, chipsOn, drip,
	wireWrap, tapeBands, hoseClamp, zipTie, coil, popRivet, hexHead, discD, accentPale, cylSide, disc, shellUp,
};
