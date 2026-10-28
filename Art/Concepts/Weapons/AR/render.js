// Richer concept views of the Bullpup AR's default parts, for Tripo's multi-view image-to-3D (Main's job, 2026-10-27).
//   node Art/Concepts/Weapons/AR/render.js [Slot_Key...]
// Each part: front.png, back.png, left.png, right.png (1024 x 1024, orthographic, same scale across its four views,
// neutral grey, soft even light, no text, no ground shadow) and sheet.png (the four labelled, size in cm, attach points),
// in Art/Concepts/Weapons/AR/<Slot>_<Key>/. Shapes come from Art/Models/Weapons/Bullpup.py (gun space, cm: u along the
// gun from the back of the butt, v up from the bore, w to the gun's left), so the silhouettes and attach points match.
// Views: front = from the muzzle end looking back, back = from the butt end, left = the gun's left side (muzzle to the
// image's left), right = its right side (muzzle to the right).
const fs = require('fs');
const path = require('path');
const { chromium } = require('playwright');

const ROOT = path.resolve(__dirname, '../../../..');
const FONT = p => 'file:///' + path.join(ROOT, p).replace(/\\/g, '/');
// --upload: clean Tripo images only (upload/ transparent, upload_white/ on white): the part alone, filling ~84%, no
// contact shadows or glow spill past its edge, no background-coloured touches; the grey views and sheets are left alone.
const UPLOAD = process.argv.includes('--upload');
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

// ---- Body_Standard (gun space) --------------------------------------------------------------------------------------
const SHELL = [[1.2, -9.6], [0, -8.6], [0, 2.0], [2, 3.4], [40, 3.2], [47, 2.4], [51, 0.6], [51.5, -1.8], [49, -3.2], [38.5, -3.5],
	[37, -4.6], [19, -5.2], [18.6, -5.8], [9.5, -5.8], [3, -8.6]];
const LOOP = [[36.8, -3.8], [37.6, -12.0], [36.4, -14.0], [34, -14.8], [25, -15.2], [22.0, -14.8], [20.6, -12.5], [19.6, -7.0], [19.4, -3.8]];
const HOLE = [[25.0, -5.8], [34.4, -5.8], [35.4, -7.0], [35.6, -11.8], [34.2, -13.0], [27.0, -13.0], [25.4, -11.8], [24.2, -7.2]];
const PLATE = [[8, -4.8], [18.2, -4.8], [19.4, 1.6], [9.4, 1.6]];
const MOUNT = [[12, 3.0], [34, 3.0], [34, 4.4], [32.5, 5.2], [13.5, 5.2], [12, 4.4]];
const HANDLE = [[38.8, 2.9], [44.0, 2.6], [44.0, 3.6], [39.6, 3.9]];
const TRIGGER = [[26.4, -5.8], [27.4, -5.8], [27.2, -7.0], [26.8, -8.0], [26.2, -8.2], [26.5, -7.0]];
// The rail's cross-section (w, v): a dovetail.
const RAIL_X = [[-1.05, 5.2], [1.05, 5.2], [1.05, 5.5], [0.86, 5.62], [1.05, 5.78], [0.84, 6.15], [-0.84, 6.15], [-1.05, 5.78], [-0.86, 5.62], [-1.05, 5.5]];

function rail(V, u0, u1, v) {
	let s = shp(pth(V, R4(u0, v, u1, v + 0.45), 0.04), 'url(#black)', 'mS');
	for (let u = u0 + 0.35; u + 0.55 <= u1 - 0.2; u += 1.0) s += shp(pth(V, R4(u, v + 0.4, u + 0.53, v + 0.95), 0.05), 'url(#black)', 'mS');
	return ao(s) + wear(pth(V, R4(u0, v, u1, v + 0.45), 0.04), '#9ea4ab', 1, 0.6, '2 14 5 30');
}

// The bullpup bodies, parametrised by a look (ST: set by the main loop from the part's `look`; {} = set 1's sand shell).
let ST = {};
const L = (k, d) => (ST[k] !== undefined ? ST[k] : d);
const ARMOR_REAR = [[5, -5.4], [20.0, -5.0], [21.0, 2.6], [6.0, 2.6]], ARMOR_NOSE = [[38, -3.0], [49.8, -2.2], [50.6, 0.0], [38, 2.4]];
const CHEEK_REST = [[0.2, 2.0], [11.0, 3.0], [11.6, 4.8], [0.6, 4.6]];
const THUMB_REST = [[36.6, -3.6], [39.0, -3.6], [39.0, -6.4], [37.8, -6.8], [36.8, -5.6]];
const VENT = u => [[u, -1.6], [u + 0.9, -1.6], [u + 2.2, 1.2], [u + 1.3, 1.2]];
function brassRivet(V, a, v, r = 0.32) {
	return ao(circ(V, a, v, r, 'url(#rBrass)', 'bS')) + circ(V, a, v, r * 0.35, '#fff6d8', null, `opacity=".6" transform="translate(${-r * V.s * 0.3},${-r * V.s * 0.3})"`);
}
function bodySide(V) {
	const right = V.kind === 'right';
	const shellD = pth(V, SHELL, 0.9), loopD = pth(V, LOOP, 0.9) + pth(V, HOLE, 0.9);
	let s = `<clipPath id="shellClip"><path d="${shellD}"/></clipPath>`;
	const handle = ao(shp(pth(V, HANDLE, 0.25), L('handle', 'url(#black)'), L('handle', '').includes('rass') ? 'bS' : 'mS')) + pl(V, [[39.5, 3.45], [43.6, 3.22]], '#ffffff', 1.2, 'opacity=".35"');
	s += shp(pth(V, R4(-0.3, -8.4, 0.3, 1.8), 0.1), 'url(#black)', 'mS');                 // the butt plate's edge
	if (right) s += handle;                                                                 // the handle is on the left
	s += shp(pth(V, TRIGGER, 0.1), 'url(#black)', 'mS');
	s += shp(loopD, L('loop', 'url(#sand)'), 'pL', 'fill-rule="evenodd"');
	// grip stipple on the loop's back strap, framed by a raised edge
	const grip = [[20.1, -6.9], [24.25, -7.5], [25.25, -11.9], [24.7, -14.25], [22.2, -14.45], [20.9, -12.6]];
	s += shp(pth(V, grip, 0.5), L('stip', 'url(#stip)'), null) + shp(pth(V, grip, 0.5), 'none', null, `stroke="${L('seamDark', '#b3a47f')}" stroke-width="1.3"`);
	s += shp(shellD, L('shell', 'url(#sand)'), 'pL');
	s += `<g clip-path="url(#shellClip)">`;
	s += `<rect x="0" y="${V.Y(3.4)}" width="${N}" height="${13 * V.s}" fill="url(#form)"/>`;
	s += `<ellipse cx="${V.X(44)}" cy="${V.Y(-0.2)}" rx="${6 * V.s}" ry="${3.2 * V.s}" fill="url(#heat)"/>`;    // heat round the vents
	for (const [a, v, rx, ry, c, o] of [[6, -4.4, 2.4, 1.0, '#000000', 0.08], [30, -3.8, 3.5, 0.8, '#000000', 0.07],
		[46, 1.8, 2.2, 0.7, '#ffffff', 0.2], [14, 2.6, 4, 0.5, '#ffffff', 0.18], [2.6, -7.2, 1.2, 1.4, '#000000', 0.1]])
		s += `<ellipse cx="${V.X(a)}" cy="${V.Y(v)}" rx="${rx * V.s}" ry="${ry * V.s}" fill="${c}" opacity="${o}"/>`;
	if (ST.pinstripe) s += pl(V, [[2.4, 2.75], [39.5, 2.6]], ST.pinstripe, 0.12 * V.s, 'opacity=".9"');
	s += `</g>`;
	// panel seams: the butt cap and the nose cap
	for (const seam of [[[1.9, 3.25], [1.9, -8.95]], [[38.3, 3.15], [37.9, 1.0], [38.0, -1.6], [38.7, -3.45]]]) {
		s += pl(V, seam, L('seamDark', '#8a7a5a'), 1.3, 'opacity=".85"') + pl(V, seam.map(([a, v]) => [a + (right ? 0.08 : -0.08), v]), L('seamLight', '#fbf6e8'), 0.9, 'opacity=".6"');
	}
	if (right) {   // the ejection port and the bolt in it (right side only)
		s += shp(pth(V, R4(2.6, -1.0, 7.4, 1.0), 0.2), '#121316', 'pS');
		s += ao(shp(pth(V, R4(2.95, -0.72, 7.05, 0.72), 0.12), 'url(#steelCyl)', 'mS'));
		s += pl(V, [[3.3, 0.0], [6.7, 0.0]], '#3a3d42', 1.4) + shp(pth(V, R4(6.1, 0.25, 6.7, 0.6), 0.05), '#2a2c30', null);
	}
	// nose vents with the ember behind them
	for (const u of [40.6, 42.8, 45.0]) {
		s += shp(pth(V, VENT(u), 0.3), '#1b120c', 'pS');
		s += `<g filter="url(#glow)">${shp(pth(V, [[u + 0.2, -1.35], [u + 0.8, -1.35], [u + 1.98, 0.95], [u + 1.38, 0.95]], 0.2), 'url(#emberLin)', null)}</g>`;
	}
	if (ST.armor) {   // Roundhouse armour: iron plates over the shell, brass rivets and edge, the vents showing through the nose plate
		s += ao(shp(pth(V, ARMOR_REAR, 0.5), 'url(#black)', 'mM'));
		s += ao(shp(pth(V, R4(5.6, 2.62, 20.4, 2.86), 0.05), 'url(#brass)', 'bS'));
		s += ao(shp(pth(V, ARMOR_NOSE, 0.5) + [40.6, 42.8, 45.0].map(u => pth(V, VENT(u), 0.3)).join(''), 'url(#black)', 'mM', 'fill-rule="evenodd"'));
		for (const [a, v] of [[6.6, -4.4], [19.2, -4.0], [7.0, 1.8], [19.8, 1.8], [39.4, -2.2], [39.4, 1.6], [48.6, -1.4]]) s += brassRivet(V, a, v, 0.34);
		for (let v = -3.6; v <= 0.8; v += 1.1) s += pl(V, [[8.0 + (v + 5) * 0.13, v], [18.6 + (v + 5) * 0.13, v]], '#0b0c0d', 1.4) + pl(V, [[8.0 + (v + 5) * 0.13, v - 0.08], [18.6 + (v + 5) * 0.13, v - 0.08]], '#6c7076', 0.7, 'opacity=".6"');
		s += wear(pth(V, ARMOR_REAR, 0.5), '#a9b2bd', 1.1, 0.7) + wear(pth(V, ARMOR_NOSE, 0.5), '#a9b2bd', 1.1, 0.7);
	} else {
		// side plates: border groove, ribs, rivets
		const plate = L('plate', 'url(#blued)');
		s += ao(shp(pth(V, PLATE, 0.8), plate, 'mM'));
		const inset = [[8.55, -4.4], [17.85, -4.4], [18.85, 1.2], [9.8, 1.2]];
		s += shp(pth(V, inset, 0.55), 'none', null, 'stroke="#16191d" stroke-width="1.5" opacity=".8"') +
			shp(pth(V, inset.map(([a, v]) => [a, v - 0.06]), 0.55), 'none', null, 'stroke="#ffffff" stroke-width=".7" opacity=".35"');
		for (let v = -3.2; v <= 0.01; v += 0.8) {
			const l = 8.55 + (v + 4.4) / 5.6 * 1.25 + 1.1, r = 17.85 + (v + 4.4) / 5.6 * 1.0 - 1.1;
			s += pl(V, [[l, v], [r, v]], '#15181c', 1.6, 'opacity=".8"') + pl(V, [[l, v - 0.1], [r, v - 0.1]], '#ffffff', 0.8, 'opacity=".35"');
		}
		for (const [a, v] of [[9.6, -4.0], [17.6, -4.0], [10.6, 0.8], [18.2, 0.8]]) s += ST.brassRivets ? brassRivet(V, a, v, 0.28) : rivet(V, a, v);
		s += wear(pth(V, PLATE, 0.8), '#e3e8ee', 1.1, 0.75, '2 8 6 18 1 9');
	}
	// the accent line (glows the rarity colour in game): a pale inlay in a channel
	const [a0, a1] = L('accent', [21, 37]);
	s += shp(pth(V, R4(a0 - 0.1, -2.86, a1 + 0.1, -2.39), 0.1), '#00000055', null) + shp(pth(V, R4(a0, -2.75, a1, -2.5), 0.1), '#f3f3ef', 'pS');
	// the selector: a brass lever over the grip
	s += ao(pl(V, [[21.2, -3.9], [23.0, -3.35]], '#3a2a10', 0.42 * V.s) + pl(V, [[21.2, -3.9], [23.0, -3.35]], 'url(#brass)', 0.3 * V.s));
	s += ao(circ(V, 21.2, -3.9, 0.45, 'url(#rBrass)', 'bS')) + circ(V, 21.2, -3.9, 0.12, '#6a4813');
	// serial plate (ticks, no text) and a proof stamp: a star in a ring
	s += ao(shp(pth(V, R4(28.4, 0.1, 34.4, 1.3), 0.15), L('serial', 'url(#blued)'), 'mS'));
	for (let k = 0; k < 12; k++) {
		const a = 29.0 + k * 0.42, hgt = [0.5, 0.3, 0.6, 0.4][k % 4];
		s += pl(V, [[a, 0.45], [a, 0.45 + hgt]], '#aab3bd', 1.1, 'opacity=".7"');
	}
	for (const [a, v] of [[28.65, 0.35], [34.15, 0.35], [28.65, 1.05], [34.15, 1.05]]) s += circ(V, a, v, 0.08, '#c9d0d8');
	s += ring(V, 25.0, 0.7, 0.62, L('seamDark', '#9a8862'), 1.6) + ring(V, 25.0, 0.66, 0.62, L('seamLight', '#fbf5e6'), 0.8, 'opacity=".6"');
	s += shp(rounded(star(V.X(25.0), V.Y(0.7), 0.4 * V.s, 0.17 * V.s), 0), L('seamDark', '#a8956c'), null);
	for (const [a, v] of [[1.0, 1.4], [1.0, -7.0], [36.0, -3.4], [39.4, 2.3], [49.0, -2.0], [23.5, 2.2], [33.0, 2.2]]) {
		if (ST.armor && ((a > 4 && a < 22 && v < 2.7) || (a > 37.5 && v < 2.5 && v > -3.2))) continue;
		s += screw(V, a, v);
	}
	if (ST.thumbRest) s += ao(shp(pth(V, THUMB_REST, 0.5), L('shell', 'url(#sand)'), 'pM')) + wear(pth(V, THUMB_REST, 0.5), L('wearCol', '#fbf6ea'));
	if (ST.underRail) {   // a short rail under the nose, teeth down
		let r = shp(pth(V, R4(39.5, -3.75, 49.0, -3.3), 0.04), L('railFill', 'url(#black)'), 'mS');
		for (let u = 39.85; u + 0.55 <= 48.8; u += 1.0) r += shp(pth(V, R4(u, -4.25, u + 0.53, -3.7), 0.05), L('railFill', 'url(#black)'), 'mS');
		s += ao(r);
	}
	// optic mount with lightening pockets and cross bolts, the rail on it
	s += ao(shp(pth(V, MOUNT, 0.5), L('mount', 'url(#sand)'), 'pM'));
	for (const [p0, p1] of [[14.6, 21.4], [24.6, 31.4]]) {
		s += shp(pth(V, R4(p0, 3.5, p1, 4.6), 0.35), '#00000033', 'pS') + shp(pth(V, R4(p0, 3.5, p1, 4.6), 0.35), 'none', null, 'stroke="#000" stroke-width="1" opacity=".3"');
	}
	for (const a of [13.0, 33.0]) s += screw(V, a, 3.85, 0.28);
	s += wear(pth(V, MOUNT, 0.5), L('wearCol', '#fbf6ea'));
	if (ST.cheekRest) {   // Meridian: a walnut cheek rest over the butt
		s += ao(shp(pth(V, CHEEK_REST, 0.9), 'url(#walnut)', 'pM'));
		for (let k = 0; k < 6; k++) s += pl(V, [[1.2 + k * 1.6, 2.4 + k * 0.1], [2.6 + k * 1.6, 4.4]], '#2a1a10', 0.9, 'opacity=".45"');
		s += wear(pth(V, CHEEK_REST, 0.9), '#c9a47c', 1.1, 0.6);
	}
	s += railFill(V, 14, 32, 5.2);
	// barrel nut (knurled), a brass collar on finer shells, and the sling loop
	s += ao(shp(pth(V, R4(50.6, -1.25, 51.9, 1.25), 0.08), 'url(#blackCyl)', 'mS'));
	for (let a = 50.75; a < 51.8; a += 0.22) s += pl(V, [[a, -1.15], [a, 1.15]], '#0e0f10', 1, 'opacity=".55"');
	if (ST.nutRing) s += ao(shp(pth(V, R4(50.9, -1.42, 51.9, 1.42), 0.06), 'url(#brassCyl)', 'bS'));
	const sling = [[43, -3.0], [43.6, -4.6], [45.6, -4.6], [46.2, -3.0]];
	s += ao(pl(V, sling, '#26272a', 0.5 * V.s) + pl(V, sling, '#6c7076', 0.18 * V.s, 'opacity=".8"'));
	s += wear(shellD, L('wearCol', '#fbf6ea')) + wear(loopD, L('wearCol', '#fbf6ea'), 1.3, 0.8, '2 12 5 20 1 15') + wear(shellD, '#000000', 2.2, 0.12, '30 40 12 60');
	if (!right) s += handle;
	return s;
}
// The rail on the mount, in the look's metal.
function railFill(V, u0, u1, v) {
	const f = L('railFill', 'url(#black)');
	let s = shp(pth(V, R4(u0, v, u1, v + 0.45), 0.04), f, 'mS');
	for (let u = u0 + 0.35; u + 0.55 <= u1 - 0.2; u += 1.0) s += shp(pth(V, R4(u, v + 0.4, u + 0.53, v + 0.95), 0.05), f, 'mS');
	return ao(s) + wear(pth(V, R4(u0, v, u1, v + 0.45), 0.04), '#c9ced3', 1, 0.6, '2 14 5 30');
}
function bodyEndSides(V) {
	let s = '';
	for (const sd of [-1, 1]) {
		if (ST.armor) s += ao(shp(pth(V, R4(sd * 2.4, -5.4, sd * 3.1, 2.6), 0.2), 'url(#black)', 'mM')) + brassRivet(V, sd * 3.2, -2.2, 0.2) + brassRivet(V, sd * 3.2, 1.6, 0.2);
		else s += ao(shp(pth(V, R4(sd * 2.5, -4.8, sd * 2.85, 1.6), 0.1), L('plate', 'url(#blued)'), 'mS'));
	}
	return s;
}
function bodyFront(V) {
	let s = shp(pth(V, R4(-2.5, -9.6, 2.5, 3.4), 1.1), L('lo', 'url(#sandLo)'), 'pL');                // the shell's front view
	if (ST.cheekRest) s += ao(shp(pth(V, R4(-2.2, 2.0, 2.2, 4.8), 0.6), 'url(#walnut)', 'pM'));
	s += shp(pth(V, R4(-1.8, -15.2, 1.8, -3.8), 0.9), L('loopEnd', 'url(#sandLoop)'), 'pL');          // the loop's front face
	s += shp(pth(V, [[-2.38, -3.5], [2.38, -3.5], [2.42, -1.8], [-2.42, -1.8]], 0.4), L('lo', 'url(#sandLo)'), 'pM');
	s += shp(pth(V, R4(-2.42, -1.8, 2.42, 0.6), 0.3), L('mid', 'url(#sandMid)'), 'pM');
	s += shp(pth(V, [[-2.42, 0.6], [2.42, 0.6], [2.42, 2.4], [1.6, 3.38], [-1.6, 3.38], [-2.42, 2.4]], 0.5), L('hi', 'url(#sandHi)'), 'pM');
	if (ST.thumbRest) s += ao(shp(pth(V, R4(-1.5, -6.8, 1.5, -3.6), 0.5), L('mid', 'url(#sandMid)'), 'pM'));
	for (const sd of [-1, 1]) s += `<g filter="url(#glow)">${shp(pth(V, R4(sd * 2.3, -1.4, sd * 2.5, 1.0), 0.05), 'url(#emberLin)', null)}</g>`;   // vents' glow at the edge
	s += bodyEndSides(V);
	s += ao(shp(pth(V, R4(-1.8, 3.0, 1.8, 4.4), 0.2), L('mid', 'url(#sandMid)'), 'pM') + shp(pth(V, [[-1.8, 4.35], [1.8, 4.35], [1.45, 5.2], [-1.45, 5.2]], 0.2), L('hi', 'url(#sandHi)'), 'pS'));
	s += ao(shp(pth(V, RAIL_X, 0.03), L('railFill', 'url(#black)'), 'mS'));
	if (ST.underRail) s += ao(shp(pth(V, RAIL_X.map(([w, v]) => [w, -3.3 - (v - 5.2)]), 0.03), L('railFill', 'url(#black)'), 'mS'));
	s += ao(shp(pth(V, R4(1.4, 2.6, 2.4, 3.9), 0.25), L('handle', 'url(#black)'), 'mS'));                 // charging handle (gun's left)
	if (ST.nutRing) s += ao(circ(V, 0, 0, 1.42, 'url(#rBrass)', 'bS'));
	s += ao(circ(V, 0, 0, 1.25, 'url(#rBlack)', 'mM'));                                       // barrel nut, threaded bore
	s += ring(V, 0, 0, 1.0, '#101113', 1.4) + circ(V, 0, 0, 0.82, 'url(#bore)', null);
	for (const r of [0.74, 0.64, 0.54]) s += ring(V, 0, 0, r, '#4a4d52', 0.9, 'opacity=".8"');
	for (let k = 0; k < 4; k++) {
		const a = (45 + 90 * k) * Math.PI / 180;
		s += circ(V, 1.13 * Math.cos(a), 1.13 * Math.sin(a), 0.14, '#0f1012');
	}
	s += ao(shp(pth(V, R4(-0.25, -4.85, 0.25, -3.0), 0.22), 'url(#blackCylH)', 'mS'));
	s += wear(pth(V, R4(-2.5, -9.6, 2.5, 3.4), 1.1), L('wearCol', '#fbf6ea')) + wear(pth(V, R4(-1.8, -15.2, 1.8, -3.8), 0.9), L('wearCol', '#fbf6ea'), 1.2, 0.7);
	return s;
}
function bodyBack(V) {
	let s = shp(pth(V, R4(-1.8, -15.2, 1.8, -3.8), 0.9), L('loopEnd', 'url(#sandLoop)'), 'pL');
	s += shp(pth(V, R4(-2.5, -9.6, 2.5, 3.4), 1.1), L('mid', 'url(#sandMid)'), 'pL');
	s += shp(pth(V, [[-2.45, 2.0], [2.45, 2.0], [2.45, 2.6], [1.7, 3.38], [-1.7, 3.38], [-2.45, 2.6]], 0.4), L('hi', 'url(#sandHi)'), 'pM');
	s += shp(pth(V, R4(-2.4, -9.55, 2.4, -8.6), 0.6), L('lo', 'url(#sandLo)'), 'pS');
	s += bodyEndSides(V);
	// the butt plate the stock seats on: guide rails, two bolts, the locating boss at the Stock socket
	s += ao(shp(pth(V, R4(-2.2, -8.4, 2.2, 1.8), 0.3), 'url(#black)', 'mM'));
	for (const sd of [-1, 1]) s += shp(pth(V, R4(sd * 1.55, -7.8, sd * 1.85, 1.2), 0.1), '#5d6168', 'mS');
	s += screw(V, 0, -6.6, 0.38) + screw(V, 0, 0.2, 0.38);
	s += ao(circ(V, 0, -3.3, 0.75, 'url(#rBlack)', 'mS')) + circ(V, 0, -3.3, 0.38, '#0f1012');
	s += wear(pth(V, R4(-2.2, -8.4, 2.2, 1.8), 0.3), '#a3abb5', 1.1, 0.7);
	s += ao(shp(pth(V, R4(-1.8, 3.0, 1.8, 4.4), 0.2), L('mid', 'url(#sandMid)'), 'pM') + shp(pth(V, [[-1.8, 4.35], [1.8, 4.35], [1.45, 5.2], [-1.45, 5.2]], 0.2), L('hi', 'url(#sandHi)'), 'pS'));
	if (ST.cheekRest) s += ao(shp(pth(V, R4(-2.2, 2.0, 2.2, 4.6), 0.6), 'url(#walnut)', 'pM'));
	s += ao(shp(pth(V, RAIL_X, 0.03), L('railFill', 'url(#black)'), 'mS'));
	s += ao(shp(pth(V, R4(1.4, 2.6, 2.4, 3.9), 0.25), L('handle', 'url(#black)'), 'mS'));
	s += wear(pth(V, R4(-2.5, -9.6, 2.5, 3.4), 1.1), L('wearCol', '#fbf6ea')) + wear(pth(V, R4(-1.8, -15.2, 1.8, -3.8), 0.9), L('wearCol', '#fbf6ea'), 1.2, 0.7);
	return s;
}

// ---- Barrel_Carbine (local: u from the barrel nut) -------------------------------------------------------------------
const SHROUD = [[0, -1.6], [8.2, -1.6], [8.75, -1.3], [9.0, -1.0], [9.0, 1.0], [8.75, 1.3], [8.2, 1.6], [0, 1.6]];
function barrelSide(V) {
	let s = shp(pth(V, R4(8.6, -0.8, 12, 0.8), 0.04), 'url(#blackCyl)', 'mS');
	s += shp(pth(V, R4(9.0, -0.8, 11.0, 0.8), 0), 'url(#temper)', null, 'opacity=".7"');                     // heat tint off the shroud
	for (let u = 11.05; u < 11.92; u += 0.09) s += pl(V, [[u, -0.78], [u + 0.035, 0.78]], '#0c0d0e', 1.1, 'opacity=".7"');
	s += shp(pth(V, R4(11.92, -0.74, 12, 0.74), 0.02), '#9da2a8', null);
	s += ao(shp(pth(V, SHROUD, 0.08), 'url(#bluedCyl)', 'mM'));
	s += shp(pth(V, R4(0, -1.6, 0.35, 1.6), 0.04), 'url(#blackCyl)', 'mS');                   // seating flange
	for (let u = 0.55; u < 1.5; u += 0.085) s += pl(V, [[u, -1.5], [u, 1.5]], '#14171a', 1, 'opacity=".55"');   // knurl
	for (const a of [1.4, 3.2, 5.0, 6.8]) {                                                     // slots, ember inside
		s += shp(pth(V, R4(a, 0.52, a + 1.1, 1.24), 0.22), '#140c07', 'pS');
		s += `<g filter="url(#glow)">${shp(pth(V, R4(a + 0.13, 0.62, a + 0.97, 1.13), 0.15), 'url(#emberLin)', null)}</g>`;
	}
	for (const a of [2.3, 5.9]) s += screw(V, a, -0.95, 0.17);
	s += ao(shp(pth(V, R4(7.6, -1.66, 8.1, 1.66), 0.06), 'url(#brassCyl)', 'bS'));
	s += wear(pth(V, SHROUD, 0.08), '#aab3be', 1.2, 0.75, '4 12 1 20 9 26');
	s += pl(V, [[2.0, 1.45], [3.6, 1.42]], '#c3cad3', 0.8, 'opacity=".5"') + pl(V, [[5.6, -1.35], [6.4, -1.3]], '#c3cad3', 0.8, 'opacity=".45"');
	return s;
}
function barrelFront(V) {
	let s = ao(circ(V, 0, 0, 1.66, 'url(#rBrass)', 'bM'));
	s += circ(V, 0, 0, 1.56, 'url(#rBlued)', 'mM') + ring(V, 0, 0, 1.0, '#15181c', 1.4);
	s += ao(circ(V, 0, 0, 0.8, 'url(#rBlack)', 'mS'));
	for (const r of [0.77, 0.71, 0.65]) s += ring(V, 0, 0, r, '#0c0d0e', 1, 'opacity=".8"');
	s += ring(V, 0, 0, 0.5, '#b9bec4', 1.6) + circ(V, 0, 0, 0.29, 'url(#bore)', null);
	for (let k = 0; k < 6; k++) {
		const a = k * Math.PI / 3;
		s += pl(V, [[0.18 * Math.cos(a), 0.18 * Math.sin(a)], [0.28 * Math.cos(a + 0.2), 0.28 * Math.sin(a + 0.2)]], '#2e3034', 1);
	}
	return s + wear(`M${V.X(-1.66)},${V.Y(0)}a${1.66 * V.s},${1.66 * V.s} 0 1,0 ${3.32 * V.s},0a${1.66 * V.s},${1.66 * V.s} 0 1,0 ${-3.32 * V.s},0`, '#fff3c8', 1.3, 0.8);
}
function barrelBack(V) {
	let s = ao(circ(V, 0, 0, 1.6, 'url(#rBlued)', 'mM'));
	s += ring(V, 0, 0, 1.45, '#1b1e22', 1.2);
	s += ao(circ(V, 0, 0, 1.15, 'url(#rSteel)', 'mS'));
	for (const r of [1.08, 1.0, 0.92]) s += ring(V, 0, 0, r, '#5a5e64', 1, 'opacity=".8"');
	s += circ(V, 0, 0, 0.8, 'url(#rBlack)', 'mS') + circ(V, 0, 0, 0.32, 'url(#bore)', null);
	s += shp(pth(V, R4(-0.07, 0.3, 0.07, 0.48), 0.02), '#0a0b0c', null);                        // extractor cut
	s += shp(pth(V, R4(-0.13, 1.0, 0.13, 1.32), 0.03), 'url(#brass)', 'bS');                    // alignment key
	return s;
}

// ---- Muzzle_Cap (local) ---------------------------------------------------------------------------------------------
const CAP = [[0, -0.92], [1.5, -0.92], [1.8, -0.7], [1.8, 0.7], [1.5, 0.92], [0, 0.92]];
function capSide(V) {
	let s = `<clipPath id="knurl"><path d="${pth(V, R4(0.3, -0.92, 1.2, 0.92), 0)}"/></clipPath>`;
	s += ao(shp(pth(V, CAP, 0.04), 'url(#brassCyl)', 'bM'));
	s += shp(pth(V, R4(0, -0.92, 0.16, 0.92), 0.02), '#8a6420', null, 'opacity=".55"');
	s += `<g clip-path="url(#knurl)">`;
	for (let k = -30; k < 30; k++) {
		const a = 0.3 + k * 0.075;
		s += pl(V, [[a, -0.92], [a + 1.84 * 0.55, 0.92]], '#6a4813', 1.1, 'opacity=".55"') + pl(V, [[a, 0.92], [a + 1.84 * 0.55, -0.92]], '#6a4813', 1.1, 'opacity=".55"');
	}
	s += `<rect x="0" y="0" width="${N}" height="${N}" fill="url(#brassCyl)" opacity=".25"/></g>`;
	if (!UPLOAD) for (let a = 0.33; a < 1.2; a += 0.075) for (const v of [-0.92, 0.92]) s += circ(V, a, v, 0.018, BG);
	s += pl(V, [[1.33, -0.9], [1.33, 0.9]], '#5a3c0e', 1.6) + pl(V, [[1.36, -0.9], [1.36, 0.9]], '#fff0c0', 0.8, 'opacity=".7"');
	return s + wear(pth(V, CAP, 0.04), '#fff6d8', 1.4, 0.85);
}
function capFront(V) {
	let s = ao(circ(V, 0, 0, 0.92, 'url(#rBrass)', 'bM'));
	for (let k = 0; k < 96; k++) {
		const a = k * Math.PI * 2 / 96;
		s += pl(V, [[0.86 * Math.cos(a), 0.86 * Math.sin(a)], [0.92 * Math.cos(a), 0.92 * Math.sin(a)]], '#6a4813', 1.2, 'opacity=".6"');
	}
	s += circ(V, 0, 0, 0.7, 'url(#brass)', 'bS') + ring(V, 0, 0, 0.42, '#8a6420', 1.4) + ring(V, 0, -0.01, 0.42, '#fff0c0', 0.8, 'opacity=".6"');
	s += circ(V, 0, 0, 0.06, '#6a4813');
	return s;
}
function capBack(V) {
	let s = ao(circ(V, 0, 0, 0.92, 'url(#rBrass)', 'bM'));
	s += circ(V, 0, 0, 0.6, '#fff0c0', null, 'opacity=".55"') + circ(V, 0, 0, 0.55, '#2a1c08', null);
	for (const r of [0.52, 0.46, 0.4, 0.34]) s += ring(V, 0, 0, r, '#b88a3c', 1.2, 'opacity=".75"');
	return s + circ(V, 0, 0, 0.28, '#140d04', null);
}

// ---- Magazine_30 (gun space) ----------------------------------------------------------------------------------------
const MF = [[17.6, -5.6], [18.3, -15.0], [19.8, -24.0]], MR = [[10.4, -5.6], [10.9, -15.0], [12.0, -23.4]];
const MAG = [...Array.from({ length: 21 }, (_, i) => bez(MF, i / 20)), ...Array.from({ length: 21 }, (_, i) => bez(MR, 1 - i / 20))];
const MBASE = [[12.0, -23.4], [19.8, -24.0], [20.05, -24.8], [12.25, -24.2]];
const RIBS = [-17.0, -18.3, -19.6, -20.9];
function magSide(V) {
	let s = shp(pth(V, MAG, 0.25), 'url(#poly)', 'pM');
	s += shp(pth(V, MAG, 0.25), 'url(#stipDark)', null, 'opacity=".35"');
	// feed lips at the top (steel), the catch notch, the raised side panel with a cartridge emblem
	s += shp(pth(V, [[10.4, -5.6], [17.6, -5.6], [edgeU(MF, -6.3), -6.3], [edgeU(MR, -6.3), -6.3]], 0.08), 'url(#blued)', 'mS');
	const panel = [];
	for (let v = -7.2; v >= -15.8; v -= 0.8) panel.push([edgeU(MR, v) + 1.0, v]);
	for (let v = -15.8; v <= -7.2; v += 0.8) panel.push([edgeU(MF, v) - 1.0, v]);
	s += shp(pth(V, panel, 0.6), 'url(#poly)', 'pS');
	const cu = (edgeU(MR, -11.4) + edgeU(MF, -11.4)) / 2;
	s += ring(V, cu, -11.4, 0.9, '#1d1e20', 1.6) + ring(V, cu, -11.45, 0.9, '#6b6d72', 0.8, 'opacity=".6"');
	s += shp(pth(V, [[cu - 0.22, -12.0], [cu + 0.22, -12.0], [cu + 0.22, -11.1], [cu, -10.75], [cu - 0.22, -11.1]], 0.08), '#232427', null);
	for (const v of RIBS) s += ao(shp(pth(V, R4(edgeU(MR, v) - 0.2, v - 0.35, edgeU(MF, v) + 0.2, v + 0.35), 0.3), 'url(#poly)', 'pS'));
	s += ao(shp(pth(V, MBASE, 0.25), 'url(#black)', 'pS'));
	s += pl(V, [[12.25, -24.15], [20.0, -24.75]], '#8d939a', 1.4, 'opacity=".7"') + circ(V, 16.0, -24.25, 0.17, 'url(#rSteel)');
	s += wear(pth(V, MAG, 0.25), '#9a9da2', 1.2, 0.7, '3 14 1 22 6 30') + wear(pth(V, MBASE, 0.25), '#b5b9be', 1.3, 0.8);
	return s;
}
function magFront(V) {   // the front spine
	let s = shp(pth(V, R4(-1.2, -24.0, 1.2, -5.6), 0.35), 'url(#polyUp)', 'pM');
	s += shp(pth(V, R4(-1.2, -6.3, 1.2, -5.6), 0.06), 'url(#blued)', 'mS');
	s += shp(pth(V, R4(-0.32, -23.2, 0.32, -6.8), 0.2), '#4a4b4f', 'pS');
	s += shp(pth(V, R4(-0.55, -8.9, 0.55, -8.1), 0.1), '#141516', 'pS');
	for (const v of RIBS) s += ao(shp(pth(V, R4(-1.35, v - 0.35, 1.35, v + 0.35), 0.3), 'url(#poly)', 'pS'));
	s += ao(shp(pth(V, R4(-1.4, -24.8, 1.4, -23.85), 0.25), 'url(#black)', 'pS'));
	return s + wear(pth(V, R4(-1.2, -24.0, 1.2, -5.6), 0.35), '#9a9da2', 1.1, 0.6);
}
function magBack(V) {    // the rear spine
	let s = shp(pth(V, R4(-1.2, -23.4, 1.2, -5.6), 0.35), 'url(#polyDown)', 'pM');
	s += shp(pth(V, R4(-1.2, -6.3, 1.2, -5.6), 0.06), 'url(#blued)', 'mS');
	s += pl(V, [[0, -6.6], [0, -23.0]], '#1f2022', 1.2, 'opacity=".8"') + pl(V, [[0.04, -6.6], [0.04, -23.0]], '#6b6d72', 0.6, 'opacity=".5"');
	for (const v of RIBS) s += ao(shp(pth(V, R4(-1.35, v - 0.35, 1.35, v + 0.35), 0.3), 'url(#poly)', 'pS'));
	s += ao(shp(pth(V, R4(-1.4, -24.2, 1.4, -23.4), 0.25), 'url(#black)', 'pS'));
	return s + wear(pth(V, R4(-1.2, -23.4, 1.2, -5.6), 0.35), '#9a9da2', 1.1, 0.6);
}

// ---- Sight_Iron (gun space) -----------------------------------------------------------------------------------------
const V0 = 6.15, H = V0 + 2.6, UR = 16.8, UF = 30.8;
const LEAF = [[-1.2, V0 + 0.55], [1.2, V0 + 0.55], [1.45, H - 0.3], [1.45, H + 0.75], [0.95, H + 1.35], [-0.95, H + 1.35], [-1.45, H + 0.75], [-1.45, H - 0.3]];
const EARS = (() => {
	const half = 1.35, gap = 0.55, tip = H - 0.45, slant = 0.45, vb = V0 + 0.55, m = gap + (half - slant - gap) * 0.5;
	return [[-half, vb], [half, vb], [half - slant, tip - 0.2], [m, tip], [gap, tip - 0.2], [gap, vb + 0.45], [-gap, vb + 0.45], [-gap, tip - 0.2], [-m, tip], [-half + slant, tip - 0.2]];
})();
const POST_SIDE = [[UF - 0.3, V0 + 1.0], [UF + 0.3, V0 + 1.0], [UF + 0.2, H + 0.27], [UF - 0.2, H + 0.27]];
function clampSide(V, u) {
	let s = ao(shp(pth(V, R4(u - 1.2, V0, u + 1.2, V0 + 0.6), 0.15), 'url(#black)', 'mS'));
	s += screw(V, u - 0.55, V0 + 0.3, 0.16);
	s += ao(circ(V, u + 0.55, V0 + 0.85, 0.32, 'url(#sand)', 'pS')) + circ(V, u + 0.55, V0 + 0.85, 0.13, 'url(#rBrass)', 'bS');
	return s + wear(pth(V, R4(u - 1.2, V0, u + 1.2, V0 + 0.6), 0.15), '#9ea5ad', 1, 0.6);
}
function sightSide(V) {
	let s = clampSide(V, UR) + clampSide(V, UF);
	// rear leaf seen edge on: black, the sand lip on its eye side, a brass spring at its foot
	s += ao(shp(pth(V, R4(UR - 0.25, V0 + 0.55, UR + 0.25, H + 1.35), 0.12), 'url(#black)', 'mS'));
	s += shp(pth(V, R4(UR - 0.31, H - 0.98, UR - 0.25, H + 0.98), 0.02), '#d9caa4', null);
	for (let k = 0; k < 4; k++) s += pl(V, [[UR - 0.5, V0 + 0.62 + k * 0.1], [UR - 0.3, V0 + 0.67 + k * 0.1]], 'url(#brass)', 1.3);
	// front: the post (its cyan dot on the eye side) behind the ears
	s += ao(shp(pth(V, POST_SIDE, 0.08), 'url(#black)', 'mS'));
	s += `<g filter="url(#glow)">${shp(pth(V, R4(UF - 0.3, H - 0.2, UF - 0.2, H + 0.2), 0.03), 'url(#cyan)', null)}</g>`;
	s += ao(shp(pth(V, [[UF - 0.35, V0 + 0.55], [UF + 0.35, V0 + 0.55], [UF + 0.35, H - 0.6], [UF + 0.2, H - 0.45], [UF - 0.2, H - 0.45], [UF - 0.35, H - 0.6]], 0.06), 'url(#black)', 'mS'));
	s += wear(pth(V, POST_SIDE, 0.08), '#a9b0b8', 1, 0.6);
	return s;
}
function leafPath(V) {
	const [x, y] = V.P(0, H), r = 0.9 * V.s;
	return pth(V, LEAF, 0.25) + `M${x - r},${y}a${r},${r} 0 1,0 ${2 * r},0a${r},${r} 0 1,0 ${-2 * r},0Z`;
}
function sightFront(V) {
	let s = ao(shp(leafPath(V), 'url(#black)', 'mM', 'fill-rule="evenodd"'));                // rear leaf, far
	s += ao(shp(pth(V, R4(-1.2, V0, 1.2, V0 + 0.6), 0.15), 'url(#black)', 'mS'));
	s += ao(shp(pth(V, R4(-0.24, V0 + 1.0, 0.24, H + 0.27), 0.12), 'url(#black)', 'mS'));    // the post
	s += ao(shp(pth(V, EARS, 0.08), 'url(#black)', 'mM'));
	s += ao(shp(pth(V, R4(-1.05, V0 + 0.53, 1.05, V0 + 1.17), 0.1), 'url(#sand)', 'pS'));   // front hinge, nearest
	for (const sd of [-1, 1]) s += circ(V, sd * 1.05, V0 + 0.85, 0.12, 'url(#rBrass)', 'bS');
	return s + wear(pth(V, EARS, 0.08), '#a9b0b8', 1, 0.6);
}
function sightBack(V) {
	let s = ao(shp(pth(V, R4(-0.24, V0 + 1.0, 0.24, H + 0.27), 0.12), 'url(#black)', 'mS'));  // the post, far
	s += `<g filter="url(#glow)">${circ(V, 0, H, 0.2, 'url(#cyan)', null)}</g>`;
	s += ao(shp(pth(V, EARS, 0.08), 'url(#black)', 'mM'));
	s += ao(shp(leafPath(V), 'url(#black)', 'mM', 'fill-rule="evenodd"'));                   // rear leaf, near
	s += ring(V, 0, H, 0.94, '#e3d6b2', 0.08 * V.s) + ring(V, 0, H, 0.94, '#8a7a58', 0.9, 'opacity=".6"');
	s += pl(V, [[-0.9, V0 + 1.0], [0.9, V0 + 1.0]], '#d8d8d8', 1.2, 'opacity=".5"');           // a white index line
	s += ao(shp(pth(V, R4(-1.2, V0, 1.2, V0 + 0.6), 0.15), 'url(#black)', 'mS'));
	for (const sd of [-1, 1]) s += screw(V, sd * 0.7, V0 + 0.3, 0.13);
	return s + wear(pth(V, LEAF, 0.25), '#a9b0b8', 1.1, 0.65);
}

// ---- Stock_Pad (gun space) ------------------------------------------------------------------------------------------
const PAD = [[0, -8.8], [-1.0, -8.8], [-1.25, -8.5], [-1.3, -7.6], [-1.3, 1.0], [-1.25, 1.7], [-1.0, 2.0], [0, 2.0]];
function padSide(V) {
	let s = shp(pth(V, PAD, 0.2), 'url(#rubber)', 'rM') + shp(pth(V, PAD, 0.2), 'url(#stipDark)', null, 'opacity=".4"');
	for (let v = -7.0; v <= 0.6; v += 0.6) s += shp(pth(V, R4(-1.3, v - 0.08, -1.22, v + 0.08), 0), '#0e0e0e', null);   // ribs' ends
	s += shp(pth(V, R4(-0.95, -6.6, -0.35, 0.0), 0.3), '#1a1a1a', 'rM');                                    // a molded side recess
	s += ao(shp(pth(V, R4(0, -8.7, 0.3, 1.9), 0.06), 'url(#blued)', 'mS'));                                  // backing plate
	return s + wear(pth(V, PAD, 0.2), '#77777a', 1.4, 0.8, '4 10 2 18 8 22');
}
function padBack(V) {    // the pad's back face, where it meets the shoulder
	let s = shp(pth(V, R4(-2.6, -8.8, 2.6, 2.0), 0.95), 'url(#rRubber)', 'rM');
	s += shp(pth(V, R4(-2.6, -8.8, 2.6, 2.0), 0.95), 'url(#stipDark)', null, 'opacity=".45"');
	for (let v = -7.2; v <= 0.6; v += 0.6) {
		if (Math.abs(v + 3.3) < 1.3) continue;
		s += shp(pth(V, R4(-2.0, v - 0.12, 2.0, v + 0.12), 0.12), '#121212', null) + pl(V, [[-1.95, v - 0.16], [1.95, v - 0.16]], '#5a5a5a', 0.8, 'opacity=".6"');
	}
	s += ring(V, 0, -3.3, 0.95, '#121212', 1.6) + ring(V, 0, -3.36, 0.95, '#5c5c5c', 0.8, 'opacity=".6"');
	s += shp(rounded(star(V.X(0), V.Y(-3.3), 0.6 * V.s, 0.2 * V.s, 8, -90), 0), '#1a1a1a', null);
	for (const v of [-6.6, 0.2]) s += circ(V, 0, v, 0.32, '#0b0b0b') + circ(V, 0, v, 0.16, '#3a3c40');
	return s + wear(pth(V, R4(-2.6, -8.8, 2.6, 2.0), 0.95), '#77777a', 1.4, 0.8);
}
function padFront(V) {   // the face that seats on the body's butt plate
	let s = shp(pth(V, R4(-2.6, -8.8, 2.6, 2.0), 0.95), 'url(#rRubber)', 'rM');
	s += ao(shp(pth(V, R4(-2.45, -8.7, 2.45, 1.9), 0.75), 'url(#blued)', 'mM'));
	for (const v of [-6.6, 0.2]) s += circ(V, 0, v, 0.34, '#101114') + ring(V, 0, v, 0.27, '#5a6068', 0.9);
	s += ao(circ(V, 0, -3.3, 0.6, 'url(#rSteel)', 'mS')) + circ(V, 0, -3.3, 0.25, '#2a2c30');
	for (const sd of [-1, 1]) s += shp(pth(V, R4(sd * 1.5, -7.6, sd * 1.9, 1.0), 0.1), '#262a2f', null);
	return s + wear(pth(V, R4(-2.45, -8.7, 2.45, 1.9), 0.75), '#a9b2bd', 1.1, 0.7);
}

// ---- Set 2 (Roundhouse): new designs (the user, 2026-10-27: "each set is a completely new idea/model"; sets 3 and 4 have their own scripts beside this one) ----------
// Only the interfaces are shared with set 1, so every part mixes: barrel nut face at (51.9, 0) r 1.25 on the body's nose,
// the butt plate's locating boss at (0, -3.3), the magazine well (u 10.4-17.6 at v -5.6, 2.4 wide), the rail top at
// v 6.15 over u 14-32 (sight origin 23), the grip's hand at (22.5, -10). Everything else is each maker's own.
function rng(seed) {
	return () => {
		seed = (seed + 0x6D2B79F5) | 0;
		let t = Math.imul(seed ^ (seed >>> 15), 1 | seed);
		t = (t + Math.imul(t ^ (t >>> 7), 61 | t)) ^ t;
		return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
	};
}
function blob(V, cu, cv, r, rnd, n = 9) {
	const pts = Array.from({ length: n }, (_, i) => {
		const a = i / n * Math.PI * 2, rr = r * (0.55 + 0.6 * rnd());
		return [cu + rr * Math.cos(a), cv + rr * Math.sin(a) * 0.7];
	});
	return pth(V, pts, r * 0.25);
}
let clipN = 0;
const inside = (d, inner) => { const id = 'in' + clipN++; return `<clipPath id="${id}"><path d="${d}" clip-rule="evenodd"/></clipPath><g clip-path="url(#${id})">${inner}</g>`; };
const tube = (V, pts, r, col, hi) => ao(pl(V, pts, '#151515', 2 * r * V.s + 2) + pl(V, pts, col, 2 * r * V.s)) +
	pl(V, pts.map(([a, v]) => [a, v + r * 0.4]), hi, r * 0.55 * V.s, 'opacity=".7"');
// A path from commands in cm: ['M', u, v], ['L', u, v], ['C', u1, v1, u2, v2, u, v], ['Z'].
const cpath = (V, cmds) => cmds.map(([k, ...n]) => {
	if (k === 'Z') return 'Z';
	const pts = [];
	for (let i = 0; i < n.length; i += 2) pts.push(V.P(n[i], n[i + 1]).map(r2).join(','));
	return k + pts.join(' ');
}).join('');
const RR = (V, a0, v0, a1, v1, r, fill, filt, extra = '') => shp(pth(V, R4(a0, v0, a1, v1), r), fill, filt, extra);
const disc = (V, a, v, r) => { const [x, y] = V.P(a, v), R = r * V.s; return `M${r2(x - R)},${r2(y)}a${r2(R)},${r2(R)} 0 1,0 ${r2(2 * R)},0a${r2(R)},${r2(R)} 0 1,0 ${r2(-2 * R)},0Z`; };
const glowG = inner => `<g filter="url(#glow)">${inner}</g>`;
const welds = (V, pts, step = 0.26, r = 0.09) => {
	let s = '';
	for (let i = 0; i + 1 < pts.length; i++) {
		const [a0, v0] = pts[i], [a1, v1] = pts[i + 1], n = Math.max(1, Math.round(Math.hypot(a1 - a0, v1 - v0) / step));
		for (let k = 0; k < n; k++) s += circ(V, a0 + (a1 - a0) * k / n, v0 + (v1 - v0) * k / n, r, '#4a4038');
	}
	return s;
};
const stripes = (V, a0, v0, a1, v1, step, col, px, op = 0.6) => { let s = ''; for (let a = a0; a < a1; a += step) s += pl(V, [[a, v0], [a + (v1 - v0) * 0.4, v1]], col, px, `opacity="${op}"`); return s; };
// The butt plate face every stock seats on (front views of stocks): two holes and the locating boss.
const stockFace = (V, fill) => ao(RR(V, -2.2, -8.4, 2.2, 1.8, 0.4, fill, 'mM')) + [-6.6, 0.2].map(v => circ(V, 0, v, 0.34, '#101114') + ring(V, 0, v, 0.27, '#5a6068', 0.9)).join('')
	+ ao(circ(V, 0, -3.3, 0.6, 'url(#rSteel)', 'mS')) + circ(V, 0, -3.3, 0.25, '#2a2c30');
// The body's butt face (back views of bodies).
const buttFace = (V, fill) => ao(RR(V, -2.2, -8.4, 2.2, 1.8, 0.3, fill, 'mM')) + screw(V, 0, -6.6, 0.38) + screw(V, 0, 0.2, 0.38)
	+ ao(circ(V, 0, -3.3, 0.75, 'url(#rBlack)', 'mS')) + circ(V, 0, -3.3, 0.38, '#0f1012');
const nutFace = (V, ring1) => (ring1 ? ao(circ(V, 0, 0, 1.42, ring1, 'bS')) : '') + ao(circ(V, 0, 0, 1.25, 'url(#rBlack)', 'mM')) + ring(V, 0, 0, 1.0, '#101113', 1.4)
	+ circ(V, 0, 0, 0.82, 'url(#bore)', null) + [0.74, 0.64, 0.54].map(r => ring(V, 0, 0, r, '#4a4d52', 0.9, 'opacity=".8"')).join('');
const threadHole = (V, r) => circ(V, 0, 0, r + 0.04, '#0d0e0f') + [0.92, 0.82, 0.72, 0.62].map(k => ring(V, 0, 0, r * k, '#4a4d52', 1, 'opacity=".8"')).join('');
const railTeeth = (V, u0, u1, v, fill) => { let s = ''; for (let u = u0 + 0.35; u + 0.55 <= u1 - 0.2; u += 1.0) s += RR(V, u, v + 0.4, u + 0.53, v + 0.95, 0.05, fill, 'mS'); return s; };

// ======== Set 2: Roundhouse (heavy, industrial: oxblood, black iron, brass, cream) ========
// Body_Armored: a blocky slab receiver, a reinforced rail beam on gussets, a squared iron handguard with ember slots, an
// armour plate with big brass rivets, a chunky grip and a heavy bar trigger guard, a brass T-knob charging handle.
const RH_REC = [[0, -9.2], [0, 3.8], [41, 3.8], [41, -5.6], [9.6, -5.6], [8.6, -9.2]];
const RH_GRIP = [[20.4, -5.6], [25.2, -5.6], [25.8, -15.2], [24.8, -16.4], [21.0, -16.4], [19.8, -15.2]];
function rhBodySide(V) {
	const right = V.kind === 'right';
	const knob = ao(tube(V, [[39.6, 1.2], [39.6, 1.2001]], 0.7, '#e0ad52', '#fff1c2') + circ(V, 39.6, 1.2, 0.7, 'url(#rBrass)', 'bS')) + circ(V, 39.6, 1.2, 0.25, '#6a4813');
	let s = RR(V, -0.3, -8.4, 0.3, 1.8, 0.1, 'url(#black)', 'mS');
	s += shp(pth(V, RH_GRIP, 0.6), 'url(#black)', 'pM');
	s += shp(pth(V, [[21.0, -7.0], [24.5, -7.0], [25.0, -14.6], [21.5, -14.6]], 0.4), 'url(#oxblood)', 'pS') + shp(pth(V, [[21.0, -7.0], [24.5, -7.0], [25.0, -14.6], [21.5, -14.6]], 0.4), 'url(#stipDark)', null, 'opacity=".5"');
	s += screw(V, 22.9, -8.0, 0.22) + screw(V, 23.2, -13.6, 0.22);
	s += shp(pth(V, [[25.8, -5.6], [26.8, -5.6], [26.6, -7.4], [26.0, -8.6], [25.6, -7.2]], 0.1), 'url(#steelPlate)', 'mS');
	s += tube(V, [[25.3, -12.4], [33.6, -12.4], [34.8, -5.6]], 0.45, '#2c2d30', '#7d8086');
	const recD = pth(V, RH_REC, 0.35);
	s += shp(recD, 'url(#oxblood)', 'pL');
	let chev = `<rect x="0" y="${V.Y(3.8)}" width="${N}" height="${13 * V.s}" fill="url(#form)"/>`;
	for (let k = 0; k < 4; k++) chev += shp(pth(V, [[1.0 + k * 1.3, -9.2], [1.65 + k * 1.3, -9.2], [2.6 + k * 1.3, -6.4], [1.95 + k * 1.3, -6.4]], 0), '#efe3c4', null, 'opacity=".9"');
	s += inside(recD, chev);
	s += ao(RR(V, 4, -4.8, 27, 3.0, 0.4, 'url(#black)', 'mM'));                                   // the armour plate
	for (let u = 5.0; u < 26.6; u += 2.75) for (const v of [-4.15, 2.35]) s += brassRivet(V, u, v, 0.36);
	for (const v of [-3.2, -2.0, -0.8, 0.4, 1.6]) s += glowG(RR(V, 30.4, v - 0.22, 36.6, v + 0.22, 0.2, 'url(#emberLin)', null)) + shp(pth(V, R4(30.4, v - 0.22, 36.6, v + 0.22), 0.2), 'none', null, 'stroke="#0b0b0c" stroke-width="2"');
	if (right) {   // a hinged ejection hatch
		s += ao(RR(V, 8, -1.6, 15, 1.8, 0.2, 'url(#steelPlate)', 'mS')) + RR(V, 8.2, 1.3, 9.4, 2.1, 0.1, 'url(#blackCyl)', 'mS') + RR(V, 13.6, 1.3, 14.8, 2.1, 0.1, 'url(#blackCyl)', 'mS');
		s += pl(V, [[8.6, -0.2], [14.6, -0.2]], '#3a3d42', 1.4);
	} else {       // a serial plate (ticks, no text) and a star proof stamp
		s += ao(RR(V, 9, -1.0, 15, 0.6, 0.15, 'url(#steelPlate)', 'mS'));
		for (let k = 0; k < 12; k++) s += pl(V, [[9.5 + k * 0.42, -0.7], [9.5 + k * 0.42, -0.7 + [0.9, 0.5, 1.1, 0.7][k % 4]]], '#2a2c30', 1.1);
		s += shp(rounded(star(V.X(19.5), V.Y(-0.2), 0.6 * V.s, 0.25 * V.s), 0), '#c9a24a', null);
	}
	s += ao(shp(pth(V, [[9.4, -5.4], [18.6, -5.4], [18.2, -6.9], [9.8, -6.9]], 0.15), 'url(#black)', 'mM')) + brassRivet(V, 10.6, -6.1, 0.28) + brassRivet(V, 17.4, -6.1, 0.28);
	s += ao(RR(V, 41, -4.6, 51.9, 3.2, 0.5, 'url(#black)', 'mM'));                               // squared iron handguard
	for (let u = 42.0; u < 51.0; u += 0.9) s += pl(V, [[u, -4.2], [u, 2.8]], '#0b0b0c', 1.6, 'opacity=".7"') + pl(V, [[u + 0.12, -4.2], [u + 0.12, 2.8]], '#6c7076', 0.7, 'opacity=".5"');
	for (const v of [1.65, -0.35, -2.35]) s += glowG(RR(V, 42.4, v - 0.25, 50.4, v + 0.25, 0.2, 'url(#emberLin)', null)) + shp(pth(V, R4(42.4, v - 0.25, 50.4, v + 0.25), 0.2), 'none', null, 'stroke="#0b0b0c" stroke-width="2"');
	s += ao(RR(V, 51.3, -4.6, 51.9, 3.2, 0.1, 'url(#brassCylH)', 'bS'));
	s += ao(RR(V, 4.6, 3.8, 42.0, 5.65, 0.2, 'url(#black)', 'mM') + railTeeth(V, 4.6, 42.0, 5.2, 'url(#black)'));   // the rail beam
	s += ao(shp(pth(V, [[4.6, 3.8], [4.6, 5.6], [2.2, 3.8]], 0.05), 'url(#black)', 'mS') + shp(pth(V, [[42.0, 3.8], [42.0, 5.6], [44.6, 3.2]], 0.05), 'url(#black)', 'mS'));
	for (let u = 6.6; u < 41; u += 4) s += brassRivet(V, u, 4.7, 0.28);
	s += tube(V, [[2.0, -9.2], [2.6, -10.5], [5.4, -10.5], [6.0, -9.2]], 0.3, '#2c2d30', '#7d8086');   // sling bar
	s += wear(recD, '#c9bdb4', 1.3, 0.7) + wear(pth(V, R4(4, -4.8, 27, 3.0), 0.4), '#a9b0b8', 1.1, 0.7);
	return s + (right ? '' : knob);
}
function rhBodyEnd(V, back) {
	let s = '';
	const grip = () => ao(RR(V, -1.5, -16.4, 1.5, -5.6, 0.6, 'url(#black)', 'pM'));
	const knob = () => ao(RR(V, 3.0, 1.0, 3.6, 1.4, 0.05, 'url(#brassCylH)', 'bS') + RR(V, 3.6, 0.5, 4.0, 1.9, 0.12, 'url(#brass)', 'bS'));
	if (back) s += grip();
	s += shp(pth(V, R4(-3.0, -9.2, 3.0, 3.8), 0.35), back ? 'url(#oxMid)' : 'url(#oxLo)', 'pL');
	for (const sd of [-1, 1]) s += ao(RR(V, sd * 3.0, -4.8, sd * 3.3, 3.0, 0.1, 'url(#black)', 'mS'));
	s += ao(RR(V, -1.4, 3.8, 1.4, 5.5, 0.15, 'url(#black)', 'mM')) + ao(shp(pth(V, RAIL_X, 0.03), 'url(#black)', 'mS'));
	if (back) s += buttFace(V, 'url(#black)');
	else {
		s += grip() + ao(RR(V, -0.45, -12.85, 0.45, -5.6, 0.4, 'url(#blackCylH)', 'mS'));
		s += ao(RR(V, -2.6, -4.6, 2.6, 3.2, 0.5, 'url(#brass)', 'bM')) + RR(V, -2.3, -4.3, 2.3, 2.9, 0.4, 'url(#black)', 'mM');
		for (const [a, v] of [[-1.9, -3.9], [1.9, -3.9], [-1.9, 2.5], [1.9, 2.5]]) s += screw(V, a, v, 0.24);
		s += nutFace(V, null);
	}
	return s + knob() + wear(pth(V, R4(-3.0, -9.2, 3.0, 3.8), 0.35), '#c9bdb4', 1.2, 0.6);
}
// Barrel_Heavy: a thick squared shroud, riveted, ember in five slots, an oxblood end cap, a brass collar at the nut.
function rhBarSide(V) {
	let s = RR(V, 13.6, -0.9, 18, 0.9, 0.05, 'url(#blackCyl)', 'mS');
	for (let u = 17.2; u < 17.95; u += 0.09) s += pl(V, [[u, -0.88], [u + 0.035, 0.88]], '#0c0d0e', 1.1, 'opacity=".7"');
	s += ao(RR(V, 0.7, -1.8, 14, 1.8, 0.4, 'url(#black)', 'mM')) + RR(V, 0.9, 1.35, 13.8, 1.75, 0.1, '#5a5d62', null, 'opacity=".7"') + RR(V, 0.9, -1.75, 13.8, -1.35, 0.1, '#0e0f10', null, 'opacity=".7"');
	for (const u of [2.2, 4.4, 6.6, 8.8, 11.0]) s += RR(V, u, -1.1, u + 0.7, 1.1, 0.25, '#120a06', 'pS') + glowG(RR(V, u + 0.12, -0.95, u + 0.58, 0.95, 0.18, 'url(#emberCyl)', null));
	for (let u = 1.4; u < 13.4; u += 1.6) for (const v of [-1.55, 1.55]) s += brassRivet(V, u, v, 0.2);
	s += ao(RR(V, 13.4, -1.9, 14.0, 1.9, 0.15, 'url(#oxblood)', 'pS'));
	s += ao(RR(V, 0, -1.3, 0.7, 1.3, 0.06, 'url(#brassCyl)', 'bS'));
	return s + wear(pth(V, R4(0.7, -1.8, 14, 1.8), 0.4), '#a9b0b8', 1.2, 0.7);
}
function rhBarEnd(V, back) {
	if (back) return ao(RR(V, -1.8, -1.8, 1.8, 1.8, 0.4, 'url(#black)', 'mM')) + ao(circ(V, 0, 0, 1.3, 'url(#rBrass)', 'bM')) + ao(circ(V, 0, 0, 1.0, 'url(#rSteel)', 'mS'))
		+ [0.94, 0.86, 0.78].map(r => ring(V, 0, 0, r, '#5a5e64', 1, 'opacity=".8"')).join('') + circ(V, 0, 0, 0.32, 'url(#bore)', null);
	let s = ao(RR(V, -1.9, -1.9, 1.9, 1.9, 0.15, 'url(#oxMid)', 'pM'));
	for (const [a, v] of [[-1.45, -1.45], [1.45, -1.45], [-1.45, 1.45], [1.45, 1.45]]) s += screw(V, a, v, 0.2);
	return s + ao(circ(V, 0, 0, 0.9, 'url(#rBlack)', 'mS')) + ring(V, 0, 0, 0.6, '#9da2a8', 1.4) + circ(V, 0, 0, 0.32, 'url(#bore)', null);
}
// Muzzle_BoxBrake: an oversized iron box with oxblood baffle fins standing proud, ember through three ports, a brass face.
function rhBrkSide(V) {
	let s = glowG(RR(V, 1.2, -0.6, 9.0, 0.6, 0.05, 'url(#emberCyl)', null));
	const box = pth(V, R4(1.2, -2.4, 9.4, 2.4), 0.3) + [2.2, 4.4, 6.6].map(u => pth(V, R4(u, -1.7, u + 1.3, 1.7), 0.15)).join('');
	s += ao(shp(box, 'url(#black)', 'mM', 'fill-rule="evenodd"'));
	for (const u of [2.2, 4.4, 6.6]) s += shp(pth(V, R4(u, -1.7, u + 1.3, 1.7), 0.15), 'none', null, 'stroke="#0b0b0c" stroke-width="3"') + pl(V, [[u + 0.1, 1.62], [u + 1.2, 1.62]], '#ff8a2e', 1.4, 'opacity=".45"');
	for (const u of [3.7, 5.9, 8.1]) s += ao(RR(V, u - 0.2, -3.2, u + 0.2, 3.2, 0.1, 'url(#oxblood)', 'pS'));
	for (const [a, v] of [[1.7, -1.95], [1.7, 1.95], [9.0, -1.95], [9.0, 1.95]]) s += screw(V, a, v, 0.25);
	s += ao(RR(V, 0, -1.1, 1.2, 1.1, 0.1, 'url(#blackCyl)', 'mS'));
	for (let u = 0.15; u < 1.1; u += 0.12) s += pl(V, [[u, -1.0], [u, 1.0]], '#0e0f10', 1, 'opacity=".55"');
	s += ao(RR(V, 9.4, -2.6, 9.9, 2.6, 0.1, 'url(#brass)', 'bS'));
	return s + wear(pth(V, R4(1.2, -2.4, 9.4, 2.4), 0.3), '#a9b0b8', 1.2, 0.7);
}
function rhBrkEnd(V, back) {
	let s = ao(RR(V, -2.0, -3.2, 2.0, 3.2, 0.1, 'url(#oxblood)', 'pS'));
	if (back) return s + ao(RR(V, -2.4, -2.4, 2.4, 2.4, 0.3, 'url(#black)', 'mM')) + ao(circ(V, 0, 0, 1.1, 'url(#rBlack)', 'mS')) + threadHole(V, 0.9);
	s += ao(RR(V, -2.6, -2.6, 2.6, 2.6, 0.2, 'url(#brass)', 'bM')) + shp(pth(V, R4(-2.2, -2.2, 2.2, 2.2), 0.15), 'none', null, 'stroke="#8a6420" stroke-width="1.4"');
	for (const [a, v] of [[-1.9, -1.9], [1.9, -1.9], [-1.9, 1.9], [1.9, 1.9]]) s += screw(V, a, v, 0.22);
	return s + circ(V, 0, 0, 0.82, '#1a0f08') + glowG(circ(V, 0, 0, 0.55, 'url(#ember)', null));
}
// Magazine_Drum: a pan drum hanging under a riveted feed neck (axis upright), iron lid, hoop and base, a cartridge
// window, a cream band, a brass crank on its right.
function rhDrumSide(V) {
	const right = V.kind === 'right';
	let s = ao(shp(pth(V, [[10.4, -5.6], [17.6, -5.6], [18.2, -9.0], [9.8, -9.0]], 0.15), 'url(#black)', 'mM')) + brassRivet(V, 11.2, -7.3, 0.24) + brassRivet(V, 16.8, -7.3, 0.24);
	s += RR(V, 10.4, -6.2, 17.6, -5.6, 0.05, 'url(#steelPlate)', 'mS');
	s += ao(RR(V, 8.0, -15.0, 20.0, -9.0, 0.6, 'url(#oxCylH)', 'pL'));
	s += RR(V, 8.0, -10.55, 20.0, -10.2, 0.05, '#efe3c4', null, 'opacity=".9"');
	s += RR(V, 13.5, -14.3, 14.5, -9.6, 0.2, '#120a06', 'pS');
	for (let v = -14.0; v < -9.8; v += 0.42) s += RR(V, 13.65, v, 14.35, v + 0.28, 0.1, 'url(#brassCyl)', null);
	for (const [v0, v1] of [[-9.4, -8.6], [-12.3, -11.7], [-15.6, -14.8]]) s += ao(RR(V, 7.7, v0, 20.3, v1, 0.2, 'url(#ironCylH)', 'mS'));
	for (let u = 8.6; u < 19.6; u += 1.5) s += brassRivet(V, u, -12.0, 0.2);
	if (right) {
		s += ao(circ(V, 17.0, -13.4, 0.8, 'url(#rBrass)', 'bM')) + ao(pl(V, [[17.0, -13.4], [18.8, -14.4]], '#3a2a10', 0.42 * V.s) + pl(V, [[17.0, -13.4], [18.8, -14.4]], 'url(#brass)', 0.3 * V.s));
		s += ao(circ(V, 18.8, -14.4, 0.42, 'url(#rBlack)', 'mS')) + circ(V, 17.0, -13.4, 0.22, '#6a4813');
	}
	return s + wear(pth(V, R4(8.0, -15.0, 20.0, -9.0), 0.6), '#d9a08e', 1.2, 0.6);
}
function rhDrumEnd(V) {
	let s = ao(shp(pth(V, [[-1.2, -5.6], [1.2, -5.6], [1.3, -9.0], [-1.3, -9.0]], 0.12), 'url(#black)', 'mM'));
	s += ao(RR(V, -6.0, -15.0, 6.0, -9.0, 0.6, 'url(#oxCylH)', 'pL')) + RR(V, -6.0, -10.55, 6.0, -10.2, 0.05, '#efe3c4', null, 'opacity=".9"');
	for (const [v0, v1] of [[-9.4, -8.6], [-12.3, -11.7], [-15.6, -14.8]]) s += ao(RR(V, -6.3, v0, 6.3, v1, 0.2, 'url(#ironCylH)', 'mS'));
	s += ao(RR(V, -6.8, -14.2, -6.0, -12.6, 0.15, 'url(#brassCylH)', 'bS')) + ao(RR(V, -7.0, -14.9, -6.6, -13.9, 0.15, 'url(#black)', 'mS'));
	return s;
}
// Sight_Holo: a riveted oxblood holo box inside an iron roll cage, a heavy clamp with hex bolts, red dot in its glass.
const HV = V0;
function rhHoloSide(V) {
	let s = ao(RR(V, 17.2, HV, 28.8, HV + 1.0, 0.2, 'url(#black)', 'mS'));
	for (const a of [18.2, 27.8]) s += ao(circ(V, a, HV + 0.5, 0.42, 'url(#rSteel)', 'mS')) + circ(V, a, HV + 0.5, 0.16, '#15161a');
	s += ao(RR(V, 18.6, HV + 1.0, 27.4, HV + 4.6, 0.5, 'url(#oxblood)', 'pM'));
	for (const v of [HV + 2.2, HV + 3.0, HV + 3.8]) s += pl(V, [[20.0, v], [25.6, v]], '#3a120d', 1.6) + pl(V, [[20.0, v - 0.08], [25.6, v - 0.08]], '#c46a5a', 0.8, 'opacity=".6"');
	s += ao(RR(V, 26.8, HV + 0.9, 27.6, HV + 4.8, 0.15, 'url(#black)', 'mS')) + ao(RR(V, 18.4, HV + 0.9, 19.0, HV + 4.8, 0.15, 'url(#black)', 'mS'));
	if (V.kind === 'right') for (const v of [HV + 2.4, HV + 3.4]) s += ao(circ(V, 20.0, v, 0.3, 'url(#rBlack)', 'mS'));
	s += RR(V, 21.2, HV + 4.1, 25.0, HV + 4.35, 0.05, '#efe3c4', null);
	const cage = [[17.6, HV + 1.0], [17.6, HV + 5.8], [18.6, HV + 6.6], [27.4, HV + 6.6], [28.4, HV + 5.8], [28.4, HV + 1.0]];
	s += tube(V, cage, 0.4, '#2c2d30', '#7d8086');
	for (const [a, v] of [[17.6, HV + 1.4], [28.4, HV + 1.4], [18.6, HV + 6.6], [27.4, HV + 6.6]]) s += brassRivet(V, a, v, 0.26);
	return s + wear(pth(V, R4(18.6, HV + 1.0, 27.4, HV + 4.6), 0.5), '#c9bdb4', 1.2, 0.6);
}
function rhHoloEnd(V, back) {
	let s = ao(RR(V, -1.6, HV, 1.6, HV + 1.0, 0.2, 'url(#black)', 'mS'));
	s += RR(V, -1.2, HV + 1.55, 1.2, HV + 4.05, 0.12, 'url(#lens)', null);
	if (back) s += glowG(circ(V, 0, HV + 2.8, 0.12, 'url(#redDot)', null)) + ring(V, 0, HV + 2.8, 0.42, '#ff5a48', 1.2, 'opacity=".7"');
	s += ao(shp(pth(V, R4(-1.5, HV + 1.0, 1.5, HV + 4.6), 0.4) + pth(V, R4(-1.15, HV + 1.6, 1.15, HV + 4.0), 0.15), 'url(#oxblood)', 'pM', 'fill-rule="evenodd"'));
	for (const sd of [-1, 1]) s += ao(RR(V, sd * 1.55, HV + 1.0, sd * 1.95, HV + 6.6, 0.15, 'url(#blackCylH)', 'mS'));
	return s + ao(RR(V, -1.95, HV + 6.2, 1.95, HV + 7.0, 0.3, 'url(#blackCyl)', 'mS'));
}
// Stock_Monopod: a heavy brace stock (iron beams round an oxblood web with lightening holes), a thick ribbed pad, a
// monopod with an oxblood sleeve, brass collar and a steel spike.
function rhStkSide(V) {
	let s = RR(V, -15.0, -9.0, -13.2, 3.0, 0.5, 'url(#rubber)', 'rM') + RR(V, -15.0, -9.0, -13.2, 3.0, 0.5, 'url(#stipDark)', null, 'opacity=".4"');
	for (let v = -8.4; v < 2.6; v += 0.6) s += RR(V, -15.0, v - 0.08, -14.85, v + 0.08, 0, '#0e0e0e', null);
	const web = pth(V, [[-0.4, 1.6], [-13.2, 1.0], [-13.2, -7.0], [-0.4, -8.2]], 0.3) + disc(V, -4.6, -3.2, 1.6) + disc(V, -9.4, -3.0, 1.5);
	s += ao(shp(web, 'url(#oxblood)', 'pL', 'fill-rule="evenodd"'));
	s += ring(V, -4.6, -3.2, 1.6, '#2c2d30', 0.25 * V.s) + ring(V, -9.4, -3.0, 1.5, '#2c2d30', 0.25 * V.s);
	s += ao(RR(V, -13.4, 0.8, -0.4, 2.2, 0.25, 'url(#black)', 'mM')) + ao(shp(pth(V, [[-0.4, -8.6], [-13.4, -7.4], [-13.4, -6.2], [-0.4, -7.4]], 0.2), 'url(#black)', 'mM'));
	for (let u = -12; u < -1; u += 2.4) s += brassRivet(V, u, 1.5, 0.26) + brassRivet(V, u, -7.4 - (u + 0.4) * 0.09, 0.24);
	s += ao(RR(V, -0.4, -8.4, 0.3, 1.8, 0.1, 'url(#black)', 'mM'));
	s += ao(shp(rounded(Array.from({ length: 6 }, (_, i) => [V.X(-9) + 0.55 * V.s * Math.cos(i * Math.PI / 3), V.Y(-7.6) + 0.55 * V.s * Math.sin(i * Math.PI / 3)]), 2), 'url(#black)', 'mS'));
	s += ao(RR(V, -9.6, -13.5, -8.4, -7.9, 0.1, 'url(#oxCylH)', 'pS')) + ao(RR(V, -9.66, -13.9, -8.34, -13.5, 0.05, 'url(#brassCylH)', 'bS'));
	s += ao(RR(V, -9.35, -17.4, -8.65, -13.9, 0.05, 'url(#steelCylH)', 'mS')) + ao(shp(pth(V, [[-9.7, -17.3], [-8.3, -17.3], [-9.0, -18.6]], 0.05), 'url(#steelPlate)', 'mS'));
	return s + wear(web, '#d9a08e', 1.2, 0.6);
}
function rhStkEnd(V, back) {
	const leg = () => ao(RR(V, -0.6, -13.5, 0.6, -7.9, 0.1, 'url(#oxCylH)', 'pS')) + ao(RR(V, -0.66, -13.9, 0.66, -13.5, 0.05, 'url(#brassCylH)', 'bS'))
		+ ao(RR(V, -0.35, -17.4, 0.35, -13.9, 0.05, 'url(#steelCylH)', 'mS')) + ao(shp(pth(V, [[-0.35, -17.3], [0.35, -17.3], [0, -18.6]], 0.03), 'url(#steelPlate)', 'mS'));
	let s = '';
	if (back) {
		s += leg() + RR(V, -2.6, -9.0, 2.6, 3.0, 0.9, 'url(#rRubber)', 'rM') + RR(V, -2.6, -9.0, 2.6, 3.0, 0.9, 'url(#stipDark)', null, 'opacity=".45"');
		for (let v = -7.8; v <= 1.8; v += 0.6) s += RR(V, -2.0, v - 0.12, 2.0, v + 0.12, 0.12, '#121212', null);
	} else {
		s += RR(V, -2.6, -9.0, 2.6, 3.0, 0.9, 'url(#rRubber)', 'rM') + ao(RR(V, -1.4, 0.8, 1.4, 2.2, 0.25, 'url(#black)', 'mM')) + stockFace(V, 'url(#black)') + leg();
	}
	return s;
}


// ---- the parts ------------------------------------------------------------------------------------------------------
const PARTS = [
	{ dir: 'Body_Standard', title: 'BODY · Standard shell', key: 'Standard', mesh: 'BullpupBody_Standard',
		b: { u: [-0.3, 51.9], v: [-15.2, 6.15], w: [-2.85, 2.85] }, draw: { left: bodySide, right: bodySide, front: bodyFront, back: bodyBack },
		mats: 'Sand polymer shell and grip loop, gunmetal side plates with brass-free black rivets, black-metal rail, bolt and barrel nut, a brass selector, ember glow behind the nose vents, a pale accent inlay (the rarity glow in game).',
		attach: [
			{ n: 'BARREL', u: 51.9, v: 0, w: 0, views: ['left', 'right', 'front'], note: 'barrel nut face on the nose (u 51.9, v 0)' },
			{ n: 'STOCK', u: 0, v: -3.3, w: 0, views: ['left', 'right', 'back'], note: 'butt plate, its locating boss (u 0, v -3.3)' },
			{ n: 'MAGAZINE', u: 14.0, v: -5.6, w: 0, views: ['left', 'right'], note: 'magazine well behind the grip (u 14, v -5.6)' },
			{ n: 'SIGHT', u: 23.0, v: 6.15, w: 0, views: ['left', 'right', 'front', 'back'], note: 'top of the rail, its middle (u 23, v 6.15)' },
		] },
	{ dir: 'Barrel_Carbine', title: 'BARREL · Carbine barrel', key: 'Carbine', mesh: 'BullpupBarrel_Carbine',
		b: { u: [0, 12], v: [-1.66, 1.66], w: [-1.66, 1.66] }, draw: { left: barrelSide, right: barrelSide, front: barrelFront, back: barrelBack },
		mats: 'Blued slotted shroud with a knurled grip band, ember glow in the four slots, a brass collar, black barrel with a heat-tinted stretch and muzzle threads.',
		attach: [
			{ n: 'BODY', u: 0, v: 0, w: 0, views: ['left', 'right', 'back'], note: 'its back face seats on the body’s barrel nut (origin)' },
			{ n: 'MUZZLE', u: 12, v: 0, w: 0, views: ['left', 'right', 'front'], note: 'SOCKET_Muzzle at the threaded tip (12 cm)' },
		] },
	{ dir: 'Muzzle_Cap', title: 'MUZZLE · Thread cap', key: 'Cap', mesh: 'BullpupMuzzle_Cap',
		b: { u: [0, 1.8], v: [-0.92, 0.92], w: [-0.92, 0.92] }, draw: { left: capSide, right: capSide, front: capFront, back: capBack },
		mats: 'Solid brass thread protector: diamond knurl, a groove ring, a closed chamfered face with a struck proof ring, internal threads at the back.',
		attach: [
			{ n: 'BARREL', u: 0, v: 0, w: 0, views: ['left', 'right', 'back'], note: 'its back face screws onto the barrel’s tip (origin)' },
			{ n: 'FLASH', u: 1.8, v: 0, w: 0, views: ['left', 'right', 'front'], note: 'SOCKET_Muzzle at its tip (1.8 cm)' },
		] },
	{ dir: 'Magazine_30', title: 'MAGAZINE · 30-round magazine', key: '30', mesh: 'BullpupMagazine_30',
		b: { u: [10.4, 20.05], v: [-24.8, -5.6], w: [-1.4, 1.4] }, draw: { left: magSide, right: magSide, front: magFront, back: magBack },
		mats: 'Black textured polymer, steel feed lips, a raised side panel with a cartridge emblem, four grip ribs, a black base plate with a steel strip and pin.',
		attach: [
			{ n: 'BODY', u: 14.0, v: -5.6, w: 0, views: ['left', 'right', 'front', 'back'], note: 'seats at its top, in the body’s well (origin)' },
		] },
	{ dir: 'Sight_Iron', title: 'SIGHT · Flip-up irons', key: 'Iron', mesh: 'BullpupSight_Iron',
		b: { u: [15.6, 32.0], v: [6.15, 10.1], w: [-1.45, 1.45] }, draw: { left: sightSide, right: sightSide, front: sightFront, back: sightBack },
		mats: 'Black steel leaves on low rail clamps, sand hinge barrels with brass pins, a sand lip round the rear aperture, a cyan dot on the front post.',
		attach: [
			{ n: 'RAIL', u: 23.0, v: 6.15, w: 0, views: ['left', 'right', 'front', 'back'], note: 'origin: top of the rail, middle of the two clamps' },
			{ n: 'AIM', u: UR, v: H, w: 0, views: ['left', 'right', 'back'], note: 'SOCKET_Aim: the rear aperture’s centre (the post’s dot sits in it)' },
		] },
	{ dir: 'Stock_Pad', title: 'STOCK · Butt pad', key: 'Pad', mesh: 'BullpupStock_Pad',
		b: { u: [-1.3, 0.3], v: [-8.8, 2.0], w: [-2.6, 2.6] }, draw: { left: padSide, right: padSide, front: padFront, back: padBack },
		mats: 'Black rubber with grip ribs and a molded star roundel, recessed screw wells, a gunmetal backing plate with a locating boss.',
		attach: [
			{ n: 'BODY', u: 0, v: -3.3, w: 0, views: ['left', 'right', 'front'], note: 'its front face seats on the butt plate; origin at the boss (u 0, v -3.3)' },
		] },
	// Set 2, Roundhouse (new designs, the user 2026-10-27): upload images only, no sheets. b = bounds (cm).
	{ set: 2, maker: 'Roundhouse', dir: 'Body_Armored', b: { u: [-0.3, 51.9], v: [-16.4, 6.15], w: [-3.35, 4.0] }, draw: { left: rhBodySide, right: rhBodySide, front: V => rhBodyEnd(V, false), back: V => rhBodyEnd(V, true) } },
	{ set: 2, maker: 'Roundhouse', dir: 'Barrel_Heavy', b: { u: [0, 18], v: [-1.9, 1.9], w: [-1.9, 1.9] }, draw: { left: rhBarSide, right: rhBarSide, front: V => rhBarEnd(V, false), back: V => rhBarEnd(V, true) } },
	{ set: 2, maker: 'Roundhouse', dir: 'Muzzle_BoxBrake', b: { u: [0, 9.9], v: [-3.2, 3.2], w: [-2.6, 2.6] }, draw: { left: rhBrkSide, right: rhBrkSide, front: V => rhBrkEnd(V, false), back: V => rhBrkEnd(V, true) } },
	{ set: 2, maker: 'Roundhouse', dir: 'Magazine_Drum', b: { u: [7.7, 20.3], v: [-15.6, -5.6], w: [-7.0, 6.3] }, draw: { left: rhDrumSide, right: rhDrumSide, front: rhDrumEnd, back: rhDrumEnd } },
	{ set: 2, maker: 'Roundhouse', dir: 'Sight_Holo', b: { u: [17.2, 28.8], v: [6.15, 13.15], w: [-1.95, 1.95] }, draw: { left: rhHoloSide, right: rhHoloSide, front: V => rhHoloEnd(V, false), back: V => rhHoloEnd(V, true) } },
	{ set: 2, maker: 'Roundhouse', dir: 'Stock_Monopod', b: { u: [-15.0, 0.3], v: [-18.6, 3.0], w: [-2.6, 2.6] }, draw: { left: rhStkSide, right: rhStkSide, front: V => rhStkEnd(V, false), back: V => rhStkEnd(V, true) } },
];

const VIEWS = ['front', 'back', 'left', 'right'];
const LABEL = { front: 'FRONT · from the muzzle', back: 'BACK · from the butt', left: 'LEFT side', right: 'RIGHT side' };
const viewPage = (svg, bg = BG) => `<!doctype html><html><head><meta charset="utf-8"><style>html,body{margin:0;background:${bg}}svg{display:block}</style></head>
	<body><svg xmlns="http://www.w3.org/2000/svg" width="${N}" height="${N}" viewBox="0 0 ${N} ${N}">${svg}</svg></body></html>`;
const dims = b => [b.u[1] - b.u[0], b.v[1] - b.v[0], b.w[1] - b.w[0]].map(n => Math.round(n * 10) / 10);

function sheetPage(p, dir) {
	const T = 368, GAP = 22, X0 = (1600 - (4 * T + 3 * GAP)) / 2, Y0 = 210, k = T / N;
	const [L, Hh, W] = dims(p.b);
	const ext = Math.max(L, Hh, W), bar = [0.5, 1, 2, 5, 10, 20].filter(x => x <= ext * 0.32).pop() || 0.5;
	let tiles = '';
	VIEWS.forEach((vk, i) => {
		const V = makeView(vk, p.b), x = X0 + i * (T + GAP);
		let marks = '';
		p.attach.forEach((a, j) => {
			if (!a.views.includes(vk)) return;
			const [px, py] = V.P(V.side ? a.u : a.w, a.v);
			const cx = x + px * k, cy = Y0 + py * k;
			marks += `<g><circle cx="${cx}" cy="${cy}" r="11" fill="none" stroke="#FF8A2A" stroke-width="3"/><path d="M${cx - 17},${cy}h10M${cx + 7},${cy}h10M${cx},${cy - 17}v10M${cx},${cy + 7}v10" stroke="#FF8A2A" stroke-width="2.5"/>
				<circle cx="${cx + 16}" cy="${cy - 16}" r="11" fill="#FF8A2A"/><text x="${cx + 16}" y="${cy - 11.5}" font-size="14" font-weight="700" fill="#101216" text-anchor="middle">${j + 1}</text></g>`;
		});
		const bpx = bar * V.s * k;
		tiles += `<image href="${vk}.png" x="${x}" y="${Y0}" width="${T}" height="${T}"/>
			<rect x="${x}" y="${Y0}" width="${T}" height="${T}" fill="none" stroke="#2c3442" stroke-width="2"/>${marks}
			<text x="${x}" y="${Y0 - 14}" font-size="22" font-weight="700" fill="#FF8A2A" letter-spacing="1.5">${LABEL[vk].toUpperCase()}</text>
			<path d="M${x + 14},${Y0 + T - 18}h${bpx}M${x + 14},${Y0 + T - 24}v12M${x + 14 + bpx},${Y0 + T - 24}v12" stroke="#2a2f38" stroke-width="2.5"/>
			<text x="${x + 20 + bpx}" y="${Y0 + T - 12}" font-size="16" font-weight="700" fill="#2a2f38">${bar} cm</text>`;
	});
	const legend = p.attach.map((a, j) => `<div class="row"><span class="n">${j + 1}</span><b>${a.n}</b> ${a.note}</div>`).join('');
	return `<!doctype html><html><head><meta charset="utf-8"><style>
		@font-face { font-family: 'Chakra Petch'; src: url('${FONT('Content/UI/Fonts/ChakraPetch-Regular.ttf')}'); font-weight: 400; }
		@font-face { font-family: 'Chakra Petch'; src: url('${FONT('Content/UI/Fonts/ChakraPetch-Bold.ttf')}'); font-weight: 700; }
		html,body{margin:0;background:#0f1218;font-family:'Chakra Petch'} svg{display:block;font-family:'Chakra Petch'}
		.leg{color:#dfe6ee;font-size:21px;line-height:1.3} .row{margin:0 0 8px} .n{display:inline-block;width:26px;height:26px;border-radius:13px;background:#FF8A2A;color:#101216;text-align:center;font-weight:700;margin-right:8px;font-size:16px;line-height:26px}
		.leg b{color:#FF8A2A;letter-spacing:1px;margin-right:6px} .m{color:#b9c4d0;font-size:19px;margin-top:12px}
	</style></head><body><svg xmlns="http://www.w3.org/2000/svg" width="1600" height="1000" viewBox="0 0 1600 1000">
		<rect width="1600" height="1000" fill="#0f1218"/>
		<rect x="40" y="36" width="8" height="96" fill="#FF8A2A"/>
		<text x="66" y="80" font-size="44" font-weight="700" fill="#ffffff" letter-spacing="1">AR · ${p.title.toUpperCase()}</text>
		<text x="68" y="120" font-size="22" fill="#8fdcf0" letter-spacing="1.5">${p.mesh} · part key “${p.key}” · ${L} × ${Hh} × ${W} cm (length × height × width)</text>
		<text x="1560" y="62" font-size="22" font-weight="700" fill="#FF8A2A" text-anchor="end" letter-spacing="2">TRIPO MULTI-VIEW SET</text>
		<text x="1560" y="88" font-size="17" fill="#9fb0c4" text-anchor="end">Orthographic · one scale · each view its own 1024 px PNG</text>
		<line x1="40" y1="150" x2="1560" y2="150" stroke="#5AD8FF" stroke-opacity=".35" stroke-width="1.5"/>
		${tiles}
		<foreignObject x="${X0}" y="${Y0 + T + 36}" width="${1600 - 2 * X0}" height="${1000 - Y0 - T - 50}"><div xmlns="http://www.w3.org/1999/xhtml" class="leg">
			${legend}<div class="m"><b>MATERIALS</b> ${p.mats}</div></div></foreignObject>
	</svg></body></html>`;
}

(async () => {
	const want = process.argv.slice(2).filter(a => !a.startsWith('--'));
	const browser = await chromium.launch();
	const pg = await browser.newPage({ viewport: { width: N, height: N } });
	for (const p of PARTS) {
		if (want.length && !want.includes(p.dir)) continue;
		if (p.set > 1 && !UPLOAD) continue;   // sets 2-4 have upload images only
		ST = {};
		const dir = path.join(__dirname, p.dir);
		fs.mkdirSync(dir, { recursive: true });
		if (UPLOAD) {
			for (const [sub, bg] of [['upload', 'transparent'], ['upload_white', '#ffffff']]) {
				fs.mkdirSync(path.join(dir, sub), { recursive: true });
				for (const vk of VIEWS) {
					const V = makeView(vk, p.b);
					const file = path.join(dir, `.${vk}.html`);
					fs.writeFileSync(file, viewPage(defs(V) + p.draw[vk](V), bg));
					await pg.setViewportSize({ width: N, height: N });
					await pg.goto('file:///' + file.split('\\').join('/'));
					await pg.screenshot({ path: path.join(dir, sub, `${vk}.png`), omitBackground: bg === 'transparent' });
					fs.unlinkSync(file);
				}
			}
			console.log('wrote', dir, 'upload');
			continue;
		}
		for (const vk of VIEWS) {
			const V = makeView(vk, p.b);
			const file = path.join(dir, `.${vk}.html`);
			fs.writeFileSync(file, viewPage(defs(V) + p.draw[vk](V)));
			await pg.setViewportSize({ width: N, height: N });
			await pg.goto('file:///' + file.replace(/\\/g, '/'));
			await pg.screenshot({ path: path.join(dir, `${vk}.png`) });
			fs.unlinkSync(file);
		}
		const sheet = path.join(dir, '.sheet.html');
		fs.writeFileSync(sheet, sheetPage(p, dir));
		await pg.setViewportSize({ width: 1600, height: 1000 });
		await pg.goto('file:///' + sheet.replace(/\\/g, '/'));
		await pg.evaluate(() => document.fonts.ready);
		await pg.screenshot({ path: path.join(dir, 'sheet.png') });
		fs.unlinkSync(sheet);
		console.log('wrote', dir);
	}
	await browser.close();
})();
