// Set 3 (Windfall) of the AR concept views for Tripo, redrawn 2026-10-27 with new scrap-built designs (render.js draws sets 1, 2, 4).
//   node Art/Concepts/Weapons/AR/render_set3.js [Body_Salvager ...]   -> <Folder>/upload/ (transparent) and upload_white/ (#ffffff)
// The helpers and set 1 code below are a copy of render.js's; the set 3 section and PARTS are this file's own.
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
const UPLOAD = true;   // set 3 has upload images only
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

// ---- Body_Salvager: a tube-frame receiver of welded scrap plates ------------------------------------------------------
const SV = 2.9, SR = 0.8;                                                                       // the spine pipe
const REC = [[4.0, -4.4], [5.2, -5.0], [21.5, -5.0], [21.5, 2.15], [4.6, 2.15], [4.0, 1.4]];
const PANEL_G = [[4.4, -4.6], [13.4, -4.8], [13.1, 1.9], [4.8, 1.75]];                         // galvanized tin, left
const PANEL_T = [[12.6, -4.45], [21.3, -4.75], [21.45, 1.6], [12.85, 2.0]];                    // teal-painted, left
const PANEL_R = [[4.3, -4.7], [15.3, -4.6], [15.45, 1.95], [4.5, 2.0]];                        // rusted plate, right
const PANEL_S = [[14.8, -4.25], [21.35, -4.6], [21.2, 1.45], [15.2, 1.7]];                     // sky-blue panel, right
const TH = [[21.3, -4.4], [29.6, -4.4], [30.4, -3.6], [30.4, 1.2], [21.3, 1.6]];               // trigger housing
const GRIP3 = [[20.9, -4.4], [25.0, -4.4], [25.3, -5.6], [23.4, -15.2], [22.6, -15.6], [19.9, -15.6], [19.2, -15.2], [20.6, -5.6]];
const GRIPCAP = [[19.0, -15.3], [23.6, -15.3], [23.2, -16.2], [19.4, -16.2]];
const TRIG3 = [[26.3, -4.4], [27.0, -4.4], [27.1, -5.6], [26.7, -7.0], [26.15, -7.35], [26.45, -6.0]];
const GUARD3 = [[24.9, -7.6], [25.4, -9.4], [26.6, -9.9], [29.4, -9.9], [30.6, -8.6], [30.8, -3.7]];
const LT0 = [30.0, -3.4], LT1 = [47.0, -1.3];                                                   // lower front tube
const ltV = u => LT0[1] + (u - LT0[0]) / (LT1[0] - LT0[0]) * (LT1[1] - LT0[1]);
const GUSSET = [[45.0, 2.15], [47.4, 2.15], [47.4, 1.5], [46.2, 1.5]];
function salvCanister(V, near) {   // the ember cell strapped to the right of the frame
	let s = pipe(V, [31.8, -0.6], [40.8, -0.6], 0.9, 'galv', 'mM');
	s += inside(pipeD(V, [31.8, -0.6], [40.8, -0.6], 0.9), ZINC);
	if (near) {
		s += shp(pth(V, R4(33.3, -1.05, 38.9, -0.15), 0.12), '#1a0f08', 'pS');
		s += `<g filter="url(#glow)">${shp(pth(V, R4(33.45, -0.95, 38.75, -0.25), 0.1), 'url(#emberLin)', null)}</g>`;
		for (let a = 33.9; a < 38.8; a += 0.7) s += shp(pth(V, R4(a, -1.05, a + 0.18, -0.15), 0.03), 'url(#galv)', 'mS');
		s += shp(pth(V, R4(39.3, -1.5, 40.4, 0.3), 0.05), 'url(#tanTape)', 'pS') + inside(pth(V, R4(39.3, -1.5, 40.4, 0.3), 0.05), `<rect x="0" y="0" width="${N}" height="${N}" fill="url(#cloth)"/>`);
	}
	s += pipe(V, [31.2, -0.6], [31.85, -0.6], 0.97, 'brass') + pipe(V, [40.75, -0.6], [41.4, -0.6], 0.97, 'brass');
	if (near) for (const u of [32.5, 39.0]) s += wireWrap(V, u, u + 0.45, -1.9, 1.7, 0.15, 0.06);   // wire straps down to the lower tube
	return s;
}
function salvSide(V) {
	const right = V.kind === 'right';
	let s = '';
	if (!right) s += salvCanister(V, false);                                                     // far side first
	if (right) s += ao(shp(pth(V, R4(37.6, 1.55, 40.4, 2.3), 0.12), 'url(#steelPlate)', 'mS'));      // charging slider, its knob on the left
	// rear: butt plate, lower tube, sling wire, the buffer spring
	const butt = pth(V, R4(-0.3, -8.6, 0.6, 2.6), 0.2);
	s += ao(shp(butt, 'url(#rust3)', 'mM')) + rustOn(V, butt, [-0.3, 0.6, -8.6, 2.6], 3, 10);
	s += rod(V, [[1.7, -6.5], [1.6, -8.0], [2.2, -8.8], [3.0, -8.6], [3.3, -7.6], [3.1, -5.9]], 0.11, '#c99a4a', '#fff1c2');
	s += pipe(V, [0.5, -7.0], [5.2, -4.4], 0.6, 'galv');
	s += coil(V, 0.6, 4.0, -1.4, 0.55, 6, 0.08, pipe(V, [0.5, -1.4], [4.2, -1.4], 0.16, 'steel'));
	s += weld(V, [[0.6, -7.6], [0.6, -6.4]]) + weld(V, [[0.6, -1.9], [0.6, -0.9]]);
	// the receiver: a rust core, two mismatched panels per side
	const recD = pth(V, REC, 0.25);
	s += ao(shp(recD, 'url(#rustDeep)', 'mM')) + rustOn(V, recD, [4, 21.5, -5, 2.2], 5, 18);
	if (!right) {
		const g = pth(V, PANEL_G, 0.15), t = pth(V, PANEL_T, 0.2);
		s += ao(shp(g, 'url(#galv)', 'mM')) + inside(g, ZINC);
		s += inside(g, [[6.2, -2.0, 1.4], [10.8, 0.4, 1.0], [8.4, -3.6, 0.8]].map(([a, v, r]) => `<ellipse cx="${V.X(a)}" cy="${V.Y(v)}" rx="${r * V.s}" ry="${r * 0.6 * V.s}" fill="#ffffff" opacity=".18"/>`).join(''));   // dents
		s += rustOn(V, g, [4.4, 13.4, -4.8, 1.9], 7, 6, ['#e9ece6', '#8a7a62', '#c9cfc6']);
		for (let a = 5.1; a < 13; a += 1.6) s += popRivet(V, a, 1.35) + popRivet(V, a + 0.1, -4.15);
		s += wear(g, '#ffffff', 1.2, 0.6);
		s += ao(shp(t, 'url(#tealPaint)', 'pM')) + chipsOn(V, t, [12.6, 21.4, -4.7, 2.0], 9, 26) + weld(V, [[12.9, 1.95], [21.4, 1.6]]);
		for (const [a, v] of [[14.0, -3.6], [20.4, -3.8], [14.0, 0.9], [20.4, 0.7]]) s += hexHead(V, a, v, 0.26) + drip(V, a, v - 0.3, 1.1);
		s += pl(V, [[15.2, -1.9], [19.6, -2.1]], '#2b5450', 1.4, 'opacity=".8"') + wear(t, '#d5e6e2', 1.2, 0.7);
	} else {
		const r = pth(V, PANEL_R, 0.15) + pth(V, R4(6.2, -0.6, 10.4, 1.0), 0.15), sk = pth(V, PANEL_S, 0.2);
		s += shp(pth(V, R4(6.2, -0.6, 10.4, 1.0), 0.15), '#120a06', null) + pipe(V, [6.5, 0.2], [10.1, 0.2], 0.42, 'steel');   // the port and its bolt
		s += shp(pth(V, R4(8.6, 0.45, 9.4, 0.75), 0.05), '#1d1e20', null);
		s += ao(shp(r, 'url(#rust3)', 'mM', 'fill-rule="evenodd"')) + rustOn(V, r, [4.3, 15.4, -4.7, 2.0], 11, 22);
		s += weld(V, [[4.5, 2.0], [15.45, 1.95]]) + weld(V, [[4.3, -4.7], [4.5, 2.0]]);
		for (const [a, v] of [[12.4, -3.8], [12.6, 1.0], [5.3, -3.9]]) s += hexHead(V, a, v, 0.3, 'url(#rRust)');
		s += ao(shp(sk, 'url(#sky)', 'pM')) + chipsOn(V, sk, [14.8, 21.4, -4.6, 1.7], 13, 20);
		for (const [a, v] of [[15.6, -3.7], [20.7, -3.9], [15.8, 1.1], [20.6, 0.9]]) s += screw(V, a, v, 0.22);
		const tag = pth(V, R4(16.4, -3.3, 19.8, -2.1), 0.08);
		s += ao(shp(tag, 'url(#hazard)', 'pS')) + inside(tag, Array.from({ length: 9 }, (_, k) => shp(pth(V, [[15.9 + k * 0.5, -3.3], [16.15 + k * 0.5, -3.3], [16.6 + k * 0.5, -2.1], [16.35 + k * 0.5, -2.1]], 0), '#1c1c1c', null)).join(''));
		s += popRivet(V, 16.7, -2.7, 0.14) + popRivet(V, 19.5, -2.7, 0.14) + wear(sk, '#e3eef5', 1.2, 0.7);
	}
	// magazine well: a bent-tin collar, a rolled lip, the leaf-spring catch
	const well = pth(V, R4(9.8, -6.4, 18.2, -4.7), 0.12);
	s += ao(shp(well, 'url(#galv)', 'mM')) + inside(well, ZINC);
	s += pipe(V, [9.7, -6.45], [18.3, -6.45], 0.18, 'galv') + weld(V, [[9.9, -4.75], [18.1, -4.75]], 0.24);
	s += rod(V, [[18.3, -4.9], [19.0, -5.3], [19.25, -6.3], [18.7, -6.8]], 0.09, '#8d9196', '#e6eaee') + popRivet(V, 18.45, -5.05, 0.13);
	// grip: a welded bracket, black friction tape, a tan tape end, brass wire at the heel, a pipe-cap foot
	const gd = pth(V, GRIP3, 0.5);
	s += ao(shp(gd, 'url(#tapeBlack)', 'pM')) + tapeBands(V, gd, [18.5, 26, -15.8, -4.4], 0.55, 0.38);
	s += inside(gd, shp(pth(V, [[18.5, -6.6], [26, -6.1], [26, -7.3], [18.5, -7.8]], 0), 'url(#tanTape)', null) + `<rect x="0" y="0" width="${N}" height="${N}" fill="url(#cloth)" opacity=".5"/>`);
	s += inside(gd, Array.from({ length: 6 }, (_, k) => { const v = -14.3 + k * 0.16; return pl(V, [[18.5, v], [26, v + 0.12]], '#3a2608', 0.13 * V.s + 1.2) + pl(V, [[18.5, v], [26, v + 0.12]], '#d29d48', 0.13 * V.s); }).join(''));
	s += ao(shp(pth(V, GRIPCAP, 0.2), 'url(#rust3)', 'mS')) + rustOn(V, pth(V, GRIPCAP, 0.2), [19, 23.6, -16.2, -15.3], 15, 5);
	s += ao(shp(pth(V, R4(20.3, -5.1, 25.7, -4.3), 0.1), 'url(#steelPlate)', 'mS')) + weld(V, [[20.4, -4.35], [25.6, -4.35]]);
	s += hexHead(V, 21.2, -4.7, 0.22) + hexHead(V, 24.8, -4.7, 0.22);
	s += wear(gd, '#8a8781', 1.3, 0.7, '3 10 2 16 6 20');
	// trigger housing, trigger, the bent-rod guard
	const th = pth(V, TH, 0.2);
	s += ao(shp(th, 'url(#rust3)', 'mM')) + rustOn(V, th, [21.3, 30.4, -4.4, 1.6], 17, 12) + weld(V, [[21.4, -4.3], [21.4, 1.5]]);
	s += screw(V, 27.2, -2.4, 0.24) + screw(V, 24.0, 0.6, 0.24) + drip(V, 27.2, -2.7, 1.0);
	s += ao(shp(pth(V, TRIG3, 0.12), 'url(#steelPlate)', 'mS'));
	s += rod(V, GUARD3, 0.2, '#7d8083', '#d9dde1') + weld(V, [[30.55, -3.85], [31.05, -3.85]], 0.15, 0.1);
	// front frame: lower tube, the strut, the recoil spring on its guide rod
	s += pipe(V, LT0, LT1, 0.55, 'galv');
	s += inside(pipeD(V, LT0, LT1, 0.55), ZINC);
	s += pipe(V, [37.0, ltV(37.0)], [44.8, ltV(44.8)], 0.68, 'tan') + tapeBands(V, pipeD(V, [37.0, ltV(37.0)], [44.8, ltV(44.8)], 0.68), [36.5, 45.5, -3.6, -0.4], 99, 0);
	for (let a = 37.2; a < 44.8; a += 0.55) s += pl(V, [[a, ltV(a) - 0.66], [a + 0.25, ltV(a + 0.25) + 0.66]], '#4a3c2c', 1.4, 'opacity=".7"');
	s += coil(V, 30.4, 43.3, 0.0, 0.6, 14, 0.085, pipe(V, [30.4, 0], [46.4, 0], 0.16, 'steel'));
	const strut = pth(V, [[43.3, ltV(43.3) - 0.3], [43.95, ltV(43.95) - 0.3], [43.95, 2.2], [43.3, 2.2]], 0.08);
	s += ao(shp(strut, 'url(#rust3)', 'mS')) + weld(V, [[43.2, 2.15], [44.05, 2.15]], 0.16, 0.1) + weld(V, [[43.2, ltV(43.2) + 0.5], [44.05, ltV(44.05) + 0.5]], 0.16, 0.1);
	if (right) s += salvCanister(V, true);
	// right side: two wires from the cell into the receiver, zip-tied
	if (right) {
		const w1 = [[31.2, -0.2], [29.6, 0.7], [27.4, 1.75], [24.2, 1.85], [21.8, 1.25]], w2 = w1.map(([a, v]) => [a, v - 0.32]);
		s += rod(V, w2, 0.11, '#2a2a2a', '#77746e') + rod(V, w1, 0.09, '#c99a4a', '#fff1c2');
		s += ao(circ(V, 21.7, 1.1, 0.34, 'url(#rRubber)', 'rM'));
		s += zipTie(V, 26.6, 1.62, 0.36, true);
	}
	// the spine pipe over everything, the rail on welded risers, the nose collar, the barrel nut
	const sp = pipeD(V, [0.0, SV], [47.4, SV], SR);
	s += pipe(V, [0.0, SV], [47.4, SV], SR, 'rust', 'mM') + rustOn(V, sp, [0, 47.4, 2.1, 3.7], 19, 40);
	s += pipe(V, [2.8, SV], [10.6, SV], SR + 0.08, 'tan') + tapeBands(V, pipeD(V, [2.8, SV], [10.6, SV], SR + 0.08), [2.8, 10.6, 2.0, 3.8], 99, 0);
	for (let a = 2.9; a < 10.6; a += 0.6) s += pl(V, [[a, SV - SR], [a + 0.3, SV + SR]], '#4a3c2c', 1.4, 'opacity=".7"');
	s += shp(pth(V, [[10.4, SV - SR - 0.08], [10.9, SV - SR + 0.2], [10.7, SV + SR + 0.08]], 0.05), 'url(#tanTape)', 'pS');   // a lifted tape end
	for (const [a, hd] of [[25.6, !right], [34.2, !right], [44.6, !right]]) s += zipTie(V, a, SV, SR, hd);
	for (const a0 of [15.0, 29.3]) {
		const rz = pth(V, R4(a0, 3.55, a0 + 1.7, 5.2), 0.1);
		s += ao(shp(rz, 'url(#rust3)', 'mS')) + rustOn(V, rz, [a0, a0 + 1.7, 3.55, 5.2], 21 + a0, 5) + weld(V, [[a0 - 0.05, 3.62], [a0 + 1.75, 3.62]], 0.17, 0.1);
		s += hexHead(V, a0 + 0.85, 4.35, 0.24);
	}
	s += rail(V, 14, 32, 5.2);
	s += weld(V, [[4.6, 2.15], [21.5, 2.15]], 0.26, 0.1);
	const col = pipeD(V, [46.4, 0], [50.7, 0], 1.75);
	s += pipe(V, [46.4, 0], [50.7, 0], 1.75, 'steel', 'mM') + rustOn(V, col, [46.4, 50.7, -1.75, 1.75], 23, 14);
	s += hoseClamp(V, 48.2, 0, 1.75, false);
	if (!right) s += ao(shp(pth(V, R4(47.75, -0.42, 48.65, 0.42), 0.08), 'url(#galv)', 'mS')) + hexHead(V, 48.2, 0, 0.24, 'url(#rGalv)');
	s += ao(shp(pth(V, GUSSET, 0.06), 'url(#steelPlate)', 'mS')) + weld(V, [[46.2, 1.55], [47.4, 1.55]], 0.16, 0.1) + weld(V, [[46.4, -1.75], [46.4, -0.9]], 0.16, 0.1);
	s += ao(shp(pth(V, R4(50.6, -1.25, 51.9, 1.25), 0.08), 'url(#blackCyl)', 'mS'));
	for (let a = 50.75; a < 51.8; a += 0.22) s += pl(V, [[a, -1.15], [a, 1.15]], '#0e0f10', 1, 'opacity=".55"');
	s += weld(V, [[50.65, -1.3], [50.65, 1.3]], 0.16, 0.1);
	if (!right) {   // the charging slider and its brass knob
		s += ao(shp(pth(V, R4(37.6, 1.55, 40.4, 2.3), 0.12), 'url(#steelPlate)', 'mS')) + screw(V, 38.0, 1.92, 0.16);
		s += ao(circ(V, 39.2, 1.75, 0.5, 'url(#rBrass)', 'bS')) + circ(V, 39.05, 1.9, 0.14, '#fff6d8', null, 'opacity=".7"');
	}
	s += wear(sp, '#e0a27a', 1.3, 0.5, '6 30 3 50 12 40') + wear(butt, '#e0a27a', 1.2, 0.6);
	return s;
}
function salvFront(V) {   // from the muzzle (+w to the image's right)
	let s = '';
	const butt = pth(V, R4(-2.5, -8.6, 2.5, 2.6), 0.3);
	s += ao(shp(butt, 'url(#rust3)', 'mM')) + rustOn(V, butt, [-2.5, 2.5, -8.6, 2.6], 31, 10);
	s += ao(shp(pth(V, R4(-0.6, -7.6, 0.6, -4.6), 0.5), 'url(#galv)', 'mS'));
	const rec = pth(V, R4(-2.3, -5.0, 2.3, 2.15), 0.15);
	s += ao(shp(rec, 'url(#rustDeep)', 'mM')) + rustOn(V, rec, [-2.3, 2.3, -5, 2.15], 33, 12);
	s += ao(shp(pth(V, R4(2.3, -4.75, 2.62, 1.6), 0.05), 'url(#tealPaint)', 'pS')) + ao(shp(pth(V, R4(-2.62, -4.6, -2.3, 1.45), 0.05), 'url(#sky)', 'pS'));
	s += ao(shp(pth(V, R4(-1.85, -6.6, 1.85, -4.7), 0.12), 'url(#galv)', 'mS'));
	s += ao(shp(pth(V, R4(-1.75, -5.1, 1.75, -4.3), 0.08), 'url(#steelPlate)', 'mS'));
	const g = pth(V, R4(-1.5, -15.4, 1.5, -4.4), 0.6);
	s += ao(shp(g, 'url(#tapeBlack)', 'pM')) + tapeBands(V, g, [-1.6, 1.6, -15.4, -4.4], 0.55, 0.2);
	s += inside(g, shp(pth(V, R4(-1.6, -7.6, 1.6, -6.4), 0), 'url(#tanTape)', null) + Array.from({ length: 6 }, (_, k) => pl(V, [[-1.6, -14.3 + k * 0.16], [1.6, -14.25 + k * 0.16]], '#d29d48', 0.13 * V.s)).join(''));
	s += ao(shp(pth(V, R4(-2.3, -16.2, 2.3, -15.3), 0.2), 'url(#rust3)', 'mS'));
	const th = pth(V, R4(-1.6, -4.4, 1.6, 1.6), 0.15);
	s += ao(shp(th, 'url(#rust3)', 'mM')) + rustOn(V, th, [-1.6, 1.6, -4.4, 1.6], 35, 8);
	s += ao(shp(pth(V, R4(-0.85, 3.55, 0.85, 5.2), 0.08), 'url(#rust3)', 'mS')) + ao(shp(pth(V, RAIL_X, 0.03), 'url(#black)', 'mS'));
	s += ao(shp(pth(V, R4(-0.22, -7.35, 0.22, -4.4), 0.08), 'url(#steelPlate)', 'mS'));
	s += ao(shp(pth(V, R4(-0.2, -10.1, 0.2, -3.6), 0.18), 'url(#steelCylH)', 'mS'));
	s += ao(circ(V, -2.6, -0.6, 0.97, 'url(#rBrass)', 'bM')) + ring(V, -2.6, -0.6, 0.7, '#6a4813', 1.4) + ao(circ(V, -2.6, -0.6, 0.28, 'url(#rSteel)', 'mS'));
	s += ao(shp(pth(V, R4(-0.55, -3.95, 0.55, -1.3), 0.5), 'url(#galv)', 'mS'));
	s += ao(shp(pth(V, R4(-0.32, -1.9, 0.32, 2.2), 0.06), 'url(#rust3)', 'mS'));
	s += ao(shp(pth(V, R4(-0.95, 1.55, 0.95, 2.3), 0.1), 'url(#steelPlate)', 'mS')) + ao(shp(pth(V, R4(0.9, 1.6, 2.5, 1.9), 0.1), 'url(#steelCyl)', 'mS'));
	s += ao(circ(V, 2.9, 1.75, 0.5, 'url(#rBrass)', 'bS'));
	s += ao(circ(V, 0, SV, SR, 'url(#rRust)', 'mM')) + circ(V, 0, SV, 0.56, '#140c08') + ring(V, 0, SV, 0.6, '#c4805a', 1.2, 'opacity=".6"');
	s += ao(shp(pth(V, R4(-0.6, 1.5, 0.6, 2.15), 0.05), 'url(#steelPlate)', 'mS'));
	const cd = discD(V, 0, 0, 1.75);
	s += ao(circ(V, 0, 0, 1.75, 'url(#rSteel)', 'mM')) + rustOn(V, cd, [-1.75, 1.75, -1.75, 1.75], 37, 10) + ring(V, 0, 0, 1.82, '#8a9196', 1.8);
	s += ao(shp(pth(V, R4(1.7, -0.42, 2.2, 0.42), 0.06), 'url(#galv)', 'mS'));
	s += ao(circ(V, 0, 0, 1.25, 'url(#rBlack)', 'mM')) + ring(V, 0, 0, 1.0, '#101113', 1.4) + circ(V, 0, 0, 0.82, 'url(#bore)', null);
	for (const r of [0.74, 0.64, 0.54]) s += ring(V, 0, 0, r, '#4a4d52', 0.9, 'opacity=".8"');
	for (let k = 0; k < 4; k++) { const a = (45 + 90 * k) * Math.PI / 180; s += circ(V, 1.13 * Math.cos(a), 1.13 * Math.sin(a), 0.14, '#0f1012'); }
	const rnd = rng(39);
	for (let k = 0; k < 22; k++) { const a = k / 22 * 6.283; s += circ(V, 1.4 * Math.cos(a), 1.4 * Math.sin(a), 0.12 + rnd() * 0.04, '#4f463e') + circ(V, 1.4 * Math.cos(a), 1.4 * Math.sin(a) + 0.04, 0.06, '#b9afa2', null, 'opacity=".7"'); }
	return s + wear(butt, '#e0a27a', 1.2, 0.6);
}
function salvBack(V) {    // from the butt (+w to the image's left)
	let s = '';
	s += ao(shp(pth(V, R4(-0.85, 3.55, 0.85, 5.2), 0.08), 'url(#rust3)', 'mS')) + ao(shp(pth(V, RAIL_X, 0.03), 'url(#black)', 'mS'));
	s += ao(circ(V, -2.6, -0.6, 0.97, 'url(#rBrass)', 'bM')) + ring(V, -2.6, -0.6, 0.7, '#6a4813', 1.4);
	s += ao(shp(pth(V, R4(0.9, 1.6, 2.5, 1.9), 0.1), 'url(#steelCyl)', 'mS')) + ao(circ(V, 2.9, 1.75, 0.5, 'url(#rBrass)', 'bS'));
	s += ao(shp(pth(V, R4(2.3, -4.8, 2.62, 1.75), 0.05), 'url(#galv)', 'mS')) + ao(shp(pth(V, R4(-2.62, -4.7, -2.3, 2.0), 0.05), 'url(#rust3)', 'mS'));
	const g = pth(V, R4(-1.5, -15.4, 1.5, -6.0), 0.6);
	s += ao(shp(g, 'url(#tapeBlack)', 'pM')) + tapeBands(V, g, [-1.6, 1.6, -15.4, -6.0], 0.55, -0.2);
	s += inside(g, Array.from({ length: 6 }, (_, k) => pl(V, [[-1.6, -14.3 + k * 0.16], [1.6, -14.25 + k * 0.16]], '#d29d48', 0.13 * V.s)).join(''));
	s += ao(shp(pth(V, R4(-2.3, -16.2, 2.3, -15.3), 0.2), 'url(#rust3)', 'mS'));
	// the butt plate the stock seats on: guide rails, two bolts, the locating boss at the Stock socket, welded edge
	const butt = pth(V, R4(-2.5, -8.6, 2.5, 2.6), 0.3);
	s += ao(shp(butt, 'url(#rust3)', 'mM')) + rustOn(V, butt, [-2.5, 2.5, -8.6, 2.6], 41, 16);
	for (const sd of [-1, 1]) s += ao(shp(pth(V, R4(sd * 1.55, -7.8, sd * 1.85, 1.2), 0.1), 'url(#steelPlate)', 'mS'));
	s += hexHead(V, 0, -6.6, 0.38, 'url(#rRust)') + hexHead(V, 0, 0.2, 0.38, 'url(#rRust)') + drip(V, 0, -0.2, 1.4) + drip(V, 0, -7.0, 1.0);
	s += ao(circ(V, 0, -3.3, 0.75, 'url(#rSteel)', 'mS')) + circ(V, 0, -3.3, 0.38, '#0f1012');
	s += weld(V, [[-2.4, -8.5], [2.4, -8.5]], 0.22, 0.1) + weld(V, [[2.4, -8.5], [2.4, 2.5]], 0.22, 0.1);
	s += ao(circ(V, 0, SV, SR, 'url(#rRust)', 'mM')) + circ(V, 0, SV, 0.6, 'url(#rRubber)', 'rM');
	return s + wear(butt, '#e0a27a', 1.2, 0.6);
}

// ---- Barrel_Shrouded: a perforated rust shroud on a steel adapter (local: u from the barrel nut) ------------------------
const SH0 = 1.25, SH1 = 14.55, SHR = 1.8;
function shrHoles(V, skip) {
	let s = '';
	[-72, -48, -24, 0, 24, 48, 72].forEach((phi, row) => {
		const v = SHR * Math.sin(phi * Math.PI / 180), ry = 0.21 * Math.cos(phi * Math.PI / 180);
		for (let u = 2.0 + (row % 2) * 0.29; u < 14.2; u += 0.58) {
			if (skip.some(([a, b]) => u > a - 0.35 && u < b + 0.35)) continue;
			s += `<ellipse cx="${r2(V.X(u))}" cy="${r2(V.Y(v))}" rx="${r2(0.21 * V.s)}" ry="${r2(ry * V.s)}" fill="#0e0907" stroke="#3a1b0d" stroke-width="1.2"/>`;
			if (Math.abs(phi) < 40) s += `<ellipse cx="${r2(V.X(u))}" cy="${r2(V.Y(v - ry * 0.25))}" rx="${r2(0.12 * V.s)}" ry="${r2(ry * 0.4 * V.s)}" fill="${phi === 0 ? '#ff8a2e' : '#5c5f63'}" opacity="${phi === 0 ? 0.55 : 0.6}"/>`;
			s += `<ellipse cx="${r2(V.X(u))}" cy="${r2(V.Y(v - ry * 0.8))}" rx="${r2(0.16 * V.s)}" ry="${r2(ry * 0.25 * V.s)}" fill="#d08a60" opacity=".35"/>`;
		}
	});
	return s;
}
function shSide(V) {
	const right = V.kind === 'right';
	let s = pipe(V, [14.6, 0], [16.5, 0], 0.75, 'steel');
	for (let u = 15.7; u < 16.45; u += 0.1) s += pl(V, [[u, -0.72], [u + 0.04, 0.72]], '#2a2c30', 1, 'opacity=".6"');
	s += shp(pth(V, R4(16.42, -0.7, 16.5, 0.7), 0.02), '#c9cdd1', null);
	s += pipe(V, [0, 0], [0.95, 0], 1.25, 'steel');
	s += ao(shp(pth(V, [[0.9, -1.25], [SH0 + 0.05, -SHR], [SH0 + 0.05, SHR], [0.9, 1.25]], 0.04), 'url(#steelCylH)', 'mS'));
	const shD = pipeD(V, [SH0, 0], [SH1, 0], SHR);
	s += pipe(V, [SH0, 0], [SH1, 0], SHR, 'rust', 'mM') + rustOn(V, shD, [SH0, SH1, -SHR, SHR], right ? 51 : 53, 30);
	const skip = [[2.0, 2.7], [6.3, 8.6], [12.3, 13.3]].concat(right ? [[9.1, 11.7]] : []);
	s += shrHoles(V, skip);
	s += weld(V, [[SH0, -SHR], [SH0, SHR]], 0.2, 0.11);
	if (!right) s += weld(V, [[SH0 + 0.3, 1.15], [SH1 - 0.2, 1.15]], 0.24, 0.08);                  // the shroud's rolled seam
	else {   // a galvanized patch riveted over burnt-through holes
		const p = pth(V, [[9.1, -1.3], [11.7, -1.15], [11.6, 0.75], [9.2, 0.6]], 0.08);
		s += ao(shp(p, 'url(#galv)', 'mS')) + inside(p, ZINC);
		for (const [a, v] of [[9.4, -1.0], [11.4, -0.9], [9.45, 0.35], [11.35, 0.45]]) s += popRivet(V, a, v, 0.13);
		s += wear(p, '#ffffff', 1.1, 0.6);
	}
	s += hoseClamp(V, 2.35, 0, SHR, false);
	if (right) s += ao(shp(pth(V, R4(1.9, -0.45, 2.8, 0.45), 0.08), 'url(#galv)', 'mS')) + hexHead(V, 2.35, 0, 0.22, 'url(#rGalv)');
	const tp = pipeD(V, [6.3, 0], [8.6, 0], SHR + 0.06);
	s += pipe(V, [6.3, 0], [8.6, 0], SHR + 0.06, 'tan') + inside(tp, `<rect x="0" y="0" width="${N}" height="${N}" fill="url(#cloth)"/>`);
	for (let u = 6.4; u < 8.6; u += 0.45) s += pl(V, [[u, -SHR], [u + 0.3, SHR]], '#4a3c2c', 1.4, 'opacity=".7"');
	s += shp(pth(V, [[8.5, -SHR - 0.06], [9.1, -SHR - 0.5], [9.2, -SHR + 0.3], [8.6, -SHR + 0.6]], 0.05), 'url(#tanTape)', 'pS');   // a loose tape end
	s += wireWrap(V, 12.35, 13.2, 0, SHR + 0.04, 0.15, 0.065);
	s += pipe(V, [SH1 - 0.1, 0], [SH1 + 0.35, 0], SHR + 0.06, 'steel');
	for (let v = -1.5; v <= 1.5; v += 0.6) s += pl(V, [[SH1 + 0.12, v - 0.12], [SH1 + 0.12, v + 0.12]], '#1a1b1d', 1.4);   // crimp tabs
	const tab = [[13.45, SHR - 0.1], [14.3, SHR - 0.1], [14.2, 2.75], [13.55, 2.75]];
	s += ao(shp(pth(V, tab, 0.08), 'url(#steelPlate)', 'mS')) + circ(V, 13.87, 2.35, 0.13, '#120c08') + weld(V, [[13.4, SHR - 0.02], [14.35, SHR - 0.02]], 0.15, 0.09);
	return s + wear(shD, '#e0a27a', 1.2, 0.6, '6 26 3 40 12 30');
}
function shFront(V) {
	let s = ao(shp(pth(V, R4(-2.35, -0.45, -1.8, 0.45), 0.06), 'url(#galv)', 'mS'));
	s += ao(shp(pth(V, R4(-0.33, SHR - 0.1, 0.33, 2.75), 0.06), 'url(#steelPlate)', 'mS'));
	s += ao(circ(V, 0, 0, SHR + 0.06, 'url(#rSteel)', 'mM'));
	for (let k = 0; k < 12; k++) { const a = k / 12 * 6.283; s += pl(V, [[1.6 * Math.cos(a), 1.6 * Math.sin(a)], [1.86 * Math.cos(a), 1.86 * Math.sin(a)]], '#1a1b1d', 1.6); }
	s += circ(V, 0, 0, 1.5, '#120b08') + ring(V, 0, 0, 1.5, '#3a1b0d', 1.4);
	const rnd = rng(55);
	for (let k = 0; k < 10; k++) { const a = rnd() * 6.283; s += circ(V, 1.2 * Math.cos(a), 1.2 * Math.sin(a), 0.09, '#ff8a2e', null, 'opacity=".35"'); }
	s += ao(circ(V, 0, 0, 0.75, 'url(#rSteel)', 'mS'));
	for (const r of [0.7, 0.62, 0.54]) s += ring(V, 0, 0, r, '#3a3d42', 0.9, 'opacity=".8"');
	s += ring(V, 0, 0, 0.45, '#c9cdd1', 1.4) + circ(V, 0, 0, 0.36, 'url(#bore)', null);
	return s;
}
function shBack(V) {
	let s = ao(shp(pth(V, R4(-2.35, -0.45, -1.8, 0.45), 0.06), 'url(#galv)', 'mS'));
	s += ao(shp(pth(V, R4(-0.33, SHR - 0.1, 0.33, 2.75), 0.06), 'url(#steelPlate)', 'mS'));
	s += ao(circ(V, 0, 0, SHR + 0.07, 'url(#rGalv)', 'mS')) + ao(circ(V, 0, 0, SHR, 'url(#rRust)', 'mM'));
	s += ao(circ(V, 0, 0, 1.25, 'url(#rSteel)', 'mS'));
	const rnd = rng(57);
	for (let k = 0; k < 20; k++) { const a = k / 20 * 6.283; s += circ(V, 1.3 * Math.cos(a), 1.3 * Math.sin(a), 0.11 + rnd() * 0.04, '#4f463e') + circ(V, 1.3 * Math.cos(a), 1.3 * Math.sin(a) + 0.04, 0.05, '#b9afa2', null, 'opacity=".7"'); }
	for (const r of [1.08, 1.0, 0.92]) s += ring(V, 0, 0, r, '#5a5e64', 1, 'opacity=".8"');
	s += circ(V, 0, 0, 0.8, 'url(#rBlack)', 'mS') + circ(V, 0, 0, 0.32, 'url(#bore)', null);
	s += shp(pth(V, R4(-0.13, 1.0, 0.13, 1.32), 0.03), 'url(#brass)', 'bS');                    // alignment key
	return s;
}

// ---- Muzzle_Prong: three uneven flat-bar prongs on a welded hex nut, brass wire collar --------------------------------
const PRONGS = [{ deg: 0, len: 6.2 }, { deg: 128, len: 6.2 }, { deg: 236, len: 5.45 }];
const prR = u => 0.72 + (u - 1.9) / (6.2 - 1.9) * 0.48;
function prSide(V) {
	const right = V.kind === 'right';
	const side = deg => Math.sin(deg * Math.PI / 180) * (right ? -1 : 1);                        // + toward the viewer
	let s = '';
	const prong = (p) => {
		const c = Math.cos(p.deg * Math.PI / 180), sn = Math.sin(p.deg * Math.PI / 180);
		const hh = 0.21 * Math.abs(c) + 0.15 * Math.abs(sn);
		const vc = u => prR(u) * c;
		const tipU = p.len, bent = p.len < 6;
		const pts = [[1.9, vc(1.9) - hh], [tipU - 0.25, vc(tipU) - hh + (bent ? -0.25 * Math.sign(c) : 0)], [tipU, vc(tipU) + (bent ? -0.3 * Math.sign(c) : 0)], [tipU - 0.1, vc(tipU) + hh], [1.9, vc(1.9) + hh]];
		let t = ao(shp(pth(V, pts, 0.05), side(p.deg) > 0.2 ? 'url(#steelPlate)' : 'url(#steelDark)', 'mS'));
		t += pl(V, [[2.0, vc(2.0) + hh * 0.6], [tipU - 0.3, vc(tipU - 0.3) + hh * 0.6]], '#e6eaee', 1.0, 'opacity=".45"');
		return t + weld(V, [[1.85, vc(1.9) - hh], [1.85, vc(1.9) + hh]], 0.08, 0.07) + wear(pth(V, pts, 0.05), '#e0a27a', 1.1, 0.6, '3 9 2 14');
	};
	const order = PRONGS.slice().sort((a, b) => side(a.deg) - side(b.deg));
	order.filter(p => side(p.deg) < 0).forEach(p => { s += prong(p); });
	s += pipe(V, [1.9, 0], [2.7, 0], 0.48, 'steel');
	// the hex nut, a flat on top: two faces seen from the side
	s += ao(shp(pth(V, R4(0, 0, 1.3, 1.1), 0.06), 'url(#galv)', 'mM') + shp(pth(V, R4(0, -1.1, 1.3, 0), 0.06), 'url(#steelPlate)', 'mM'));
	s += inside(pth(V, R4(0, -1.1, 1.3, 1.1), 0.06), ZINC) + pl(V, [[0.05, 0], [1.25, 0]], '#e6eaee', 1.2, 'opacity=".6"');
	s += rustOn(V, pth(V, R4(0, -1.1, 1.3, 1.1), 0.06), [0, 1.3, -1.1, 1.1], right ? 61 : 63, 10);
	for (const v of [-1.05, 1.05]) s += pl(V, [[0.05, v], [1.25, v]], '#2a2c30', 1.2, 'opacity=".5"');
	s += pipe(V, [1.25, 0], [2.0, 0], 0.95, 'steel') + wireWrap(V, 1.3, 1.9, 0, 0.98, 0.13, 0.05);
	s += weld(V, [[1.28, -1.0], [1.28, 1.0]], 0.12, 0.08);
	order.filter(p => side(p.deg) >= 0).forEach(p => { s += prong(p); });
	return s;
}
function prFront(V) {
	let s = ao(shp(pth(V, [[1.27, 0], [0.635, 1.1], [-0.635, 1.1], [-1.27, 0], [-0.635, -1.1], [0.635, -1.1]], 0.08), 'url(#rGalv)', 'mM'));
	s += ao(circ(V, 0, 0, 0.98, 'url(#rBrass)', 'bM'));
	for (const r of [0.92, 0.84, 0.76, 0.68]) s += ring(V, 0, 0, r, '#5f400f', 1.1, 'opacity=".7"');
	s += ao(circ(V, 0, 0, 0.5, 'url(#rSteel)', 'mS')) + circ(V, 0, 0, 0.36, 'url(#bore)', null);
	for (const p of PRONGS) {
		const a = p.deg * Math.PI / 180, d = [Math.sin(a), Math.cos(a)], t = [Math.cos(a), -Math.sin(a)];
		const P = (r, o) => [d[0] * r + t[0] * o, d[1] * r + t[1] * o];
		const r1 = p.len < 6 ? 1.05 : 1.2;
		s += ao(shp(pth(V, [P(0.5, -0.15), P(r1 + 0.21, -0.15), P(r1 + 0.21, 0.15), P(0.5, 0.15)], 0.04), 'url(#steelPlate)', 'mS'));
		s += shp(pth(V, [P(r1 - 0.21, -0.12), P(r1 + 0.19, -0.12), P(r1 + 0.19, 0.12), P(r1 - 0.21, 0.12)], 0.03), '#c6c9cc', null, 'opacity=".7"');
		s += circ(V, ...P(0.55, 0.2), 0.07, '#4f463e') + circ(V, ...P(0.55, -0.2), 0.07, '#4f463e');
	}
	return s;
}
function prBack(V) {
	let s = ao(shp(pth(V, [[1.27, 0], [0.635, 1.1], [-0.635, 1.1], [-1.27, 0], [-0.635, -1.1], [0.635, -1.1]], 0.08), 'url(#rGalv)', 'mM'));
	s += inside(pth(V, [[1.27, 0], [0.635, 1.1], [-0.635, 1.1], [-1.27, 0], [-0.635, -1.1], [0.635, -1.1]], 0.08), ZINC);
	s += rustOn(V, pth(V, [[1.27, 0], [0.635, 1.1], [-0.635, 1.1], [-1.27, 0], [-0.635, -1.1], [0.635, -1.1]], 0.08), [-1.27, 1.27, -1.1, 1.1], 65, 8);
	s += circ(V, 0, 0, 0.84, '#0d0e0f');
	for (const r of [0.8, 0.72, 0.64, 0.56]) s += ring(V, 0, 0, r, '#4a4d52', 1, 'opacity=".8"');
	s += circ(V, 0, 0, 0.36, 'url(#bore)', null);
	for (const [w, v] of [[0.8, 0.75], [-0.95, 0.35]]) s += circ(V, w, v, 0.06, '#2a2c30');               // punch marks
	return s;
}

// ---- Magazine_Coupled: a rusty steel box seated, a galvanized one taped upside down beside it (gun's left) -----------
const MAGA = [[10.4, -5.6], [17.6, -5.6], [18.4, -22.0], [11.2, -22.0]];
const BASEA = [[10.9, -21.95], [18.8, -21.95], [18.9, -22.4], [18.6, -23.0], [11.0, -23.0], [10.8, -22.4]];
const MAGB = [[11.0, -8.6], [18.6, -8.6], [17.8, -25.0], [10.6, -25.0]];
const BASEB = [[10.75, -8.65], [18.9, -8.65], [18.75, -7.6], [10.95, -7.6]];
const edgeAt = (P, i, j, v) => P[i][0] + (P[j][0] - P[i][0]) * (v - P[i][1]) / (P[j][1] - P[i][1]);
function magA(V) {
	const d = pth(V, MAGA, 0.25);
	let s = ao(shp(d, 'url(#rust3)', 'mM')) + rustOn(V, d, [10.4, 18.4, -22, -5.6], 71, 30);
	for (const f of [0.38, 0.66]) {   // two pressed ribs
		const top = 10.4 + 7.2 * f, bot = 11.2 + 7.2 * f;
		s += pl(V, [[top, -7.0], [bot, -20.6]], '#3e1d0e', 0.28 * V.s, 'opacity=".55"') + pl(V, [[top - 0.12, -7.0], [bot - 0.12, -20.6]], '#d08a60', 0.1 * V.s, 'opacity=".45"');
	}
	for (let k = 0; k < 6; k++) {   // witness holes, brass cases behind
		const v = -9.0 - k * 2.1, u = edgeAt(MAGA, 3, 0, v) + 1.2;
		s += `<ellipse cx="${r2(V.X(u))}" cy="${r2(V.Y(v))}" rx="${r2(0.36 * V.s)}" ry="${r2(0.5 * V.s)}" fill="url(#brass)" stroke="#2a1409" stroke-width="1.6"/>`;
	}
	s += ao(shp(pth(V, [[10.45, -5.6], [17.6, -5.6], [17.66, -6.3], [10.5, -6.3]], 0.06), 'url(#steelPlate)', 'mS'));
	s += ao(shp(pth(V, BASEA, 0.15), 'url(#rustDeep)', 'mS')) + rustOn(V, pth(V, BASEA, 0.15), [10.8, 18.9, -23, -21.95], 105, 5) + hexHead(V, 12.0, -22.5, 0.22, 'url(#rRust)');
	return s + wear(d, '#e0a27a', 1.2, 0.6);
}
function magB(V) {
	const d = pth(V, MAGB, 0.25);
	let s = ao(shp(d, 'url(#galv)', 'mM')) + inside(d, ZINC);
	s += rustOn(V, d, [10.6, 18.6, -25, -8.6], 73, 10, ['#e9ece6', '#8a7a62', '#c9cfc6']);
	for (let k = 0; k < 4; k++) {   // stamped dimples
		const v = -11.2 - k * 3.4, u = (edgeAt(MAGB, 3, 0, v) + edgeAt(MAGB, 2, 1, v)) / 2;
		s += `<ellipse cx="${r2(V.X(u))}" cy="${r2(V.Y(v))}" rx="${r2(2.4 * V.s)}" ry="${r2(0.55 * V.s)}" fill="#000" opacity=".12"/><ellipse cx="${r2(V.X(u))}" cy="${r2(V.Y(v + 0.15))}" rx="${r2(2.2 * V.s)}" ry="${r2(0.3 * V.s)}" fill="#fff" opacity=".25"/>`;
	}
	s += inside(d, shp(pth(V, [[10, -21.2], [19, -21.2], [19, -22.1], [10, -22.1]], 0), 'url(#tealPaint)', null, 'opacity=".9"') + chipsOn(V, pth(V, R4(10, -22.1, 19, -21.2), 0), [10, 19, -22.1, -21.2], 75, 6, ['#a7aaad']));
	s += ao(shp(pth(V, BASEB, 0.15), 'url(#steelPlate)', 'mS')) + popRivet(V, 12.0, -8.1, 0.16) + popRivet(V, 17.6, -8.1, 0.16);
	// its feed lips at the bottom, the top round showing: a brass case, a copper nose pointing forward
	s += ao(shp(pth(V, [[11.4, -25.0], [15.9, -25.0], [16.4, -25.25], [17.3, -25.45], [17.5, -25.55], [17.3, -25.65], [16.4, -25.85], [11.4, -25.85]], 0.08), 'url(#brass)', 'bS'));
	s += shp(pth(V, [[15.9, -25.0], [16.4, -25.25], [17.3, -25.45], [17.5, -25.55], [17.3, -25.65], [16.4, -25.85], [15.9, -25.85]], 0.08), 'url(#copperCyl)', 'bS');
	s += ao(shp(pth(V, [[10.6, -25.0], [17.8, -25.0], [17.7, -24.3], [10.65, -24.3]], 0.06), 'url(#steelPlate)', 'mS'));
	return s + wear(d, '#ffffff', 1.2, 0.6);
}
function cpSide(V) {
	const right = V.kind === 'right';
	let s = right ? magB(V) + magA(V) : magA(V) + magB(V);
	// a bent-strap bracket bolted round both, a brass-wire twist, black tape
	s += ao(shp(pth(V, R4(10.3, -11.6, 19.0, -10.4), 0.12), 'url(#steelPlate)', 'mS')) + rustOn(V, pth(V, R4(10.3, -11.6, 19.0, -10.4), 0.12), [10.3, 19, -11.6, -10.4], 77, 6);
	s += hexHead(V, 10.75, -11.0, 0.3) + hexHead(V, 18.55, -11.0, 0.3) + drip(V, 18.55, -11.4, 1.3);
	for (let k = 0; k < 6; k++) { const v = -14.6 + k * 0.16; s += pl(V, [[10.7, v], [18.5, v + 0.1]], '#3a2608', 0.12 * V.s + 1.2) + pl(V, [[10.7, v], [18.5, v + 0.1]], '#d29d48', 0.12 * V.s); }
	s += ao(circ(V, 18.8, -14.2, 0.22, '#d29d48', 'bS')) + rod(V, [[18.8, -14.2], [19.4, -14.9], [19.2, -15.6]], 0.06, '#d29d48', '#fff1c2');   // the twisted end
	const tp = pth(V, R4(10.4, -19.4, 18.95, -17.6), 0.1);
	s += ao(shp(tp, 'url(#tapeBlack)', 'pM')) + tapeBands(V, tp, [10, 19.2, -19.6, -17.4], 0.45, 0.1);
	s += shp(pth(V, [[18.9, -19.35], [19.6, -19.9], [19.4, -18.6], [18.9, -18.2]], 0.05), 'url(#tapeBlack)', 'pS');   // a lifted tape end
	return s;
}
function cpEnd(V, back) {
	let s = '';
	const A = pth(V, R4(-1.4, -22.0, 1.4, -5.6), 0.3), B = pth(V, R4(1.7, -25.0, 4.5, -8.6), 0.3);
	s += ao(shp(A, back ? 'url(#rustDeep)' : 'url(#rust3)', 'mM')) + rustOn(V, A, [-1.4, 1.4, -22, -5.6], back ? 79 : 81, 14);
	s += shp(pth(V, R4(-0.35, -20.8, 0.35, -7.0), 0.2), '#00000033', null) + ao(shp(pth(V, R4(-1.55, -23.0, 1.55, -21.95), 0.2), 'url(#rustDeep)', 'mS'));
	s += ao(shp(pth(V, R4(1.4, -20.0, 1.7, -9.2), 0.04), 'url(#rubber)', 'rM'));                            // a rubber spacer
	s += ao(shp(B, 'url(#galv)', 'mM')) + inside(B, ZINC);
	s += inside(B, shp(pth(V, R4(1.6, -22.1, 4.6, -21.2), 0), 'url(#tealPaint)', null));
	s += ao(shp(pth(V, R4(1.55, -8.65, 4.65, -7.6), 0.15), 'url(#steelPlate)', 'mS'));
	s += ao(shp(pth(V, R4(1.9, -25.0, 4.3, -24.3), 0.08), 'url(#steelPlate)', 'mS'));
	if (back) s += ao(circ(V, 3.1, -25.4, 0.42, 'url(#rBrass)', 'bS')) + circ(V, 3.1, -25.4, 0.15, '#c9cdd1');      // case head, primer
	else s += ao(circ(V, 3.1, -25.4, 0.42, 'url(#rBrass)', 'bS')) + ao(circ(V, 3.1, -25.4, 0.28, 'url(#copperCyl)', 'bS'));
	s += ao(shp(pth(V, R4(-1.6, -11.6, 4.7, -10.4), 0.12), 'url(#steelPlate)', 'mS')) + hexHead(V, -1.6, -11.0, 0.3) + hexHead(V, 4.7, -11.0, 0.3);
	s += inside(pth(V, R4(-1.5, -14.7, 4.6, -13.9), 0), Array.from({ length: 6 }, (_, k) => pl(V, [[-1.5, -14.6 + k * 0.16], [4.6, -14.6 + k * 0.16]], '#d29d48', 0.12 * V.s)).join(''));
	const tp = pth(V, R4(-1.6, -19.4, 4.7, -17.6), 0.15);
	s += ao(shp(tp, 'url(#tapeBlack)', 'pM')) + tapeBands(V, tp, [-1.7, 4.8, -19.6, -17.4], 0.45, 0.05);
	return s + wear(A, '#e0a27a', 1.1, 0.6) + wear(B, '#ffffff', 1.1, 0.6);
}

// ---- Sight_Reflex: a tilted glass on tin gussets, a flashlight-tube emitter, a taped cell (gun space) --------------------
const RV = V0;
const GUS = [[21.9, RV + 0.85], [26.3, RV + 0.85], [25.4, RV + 5.3], [24.0, RV + 5.3]];
function rfEmitter(V) {
	let s = pipe(V, [19.0, RV + 1.55], [21.6, RV + 1.55], 0.5, 'galv') + pipe(V, [21.55, RV + 1.55], [21.85, RV + 1.55], 0.56, 'brass');
	s += pipe(V, [19.6, RV + 1.55], [20.6, RV + 1.55], 0.53, 'tape') + tapeBands(V, pipeD(V, [19.6, RV + 1.55], [20.6, RV + 1.55], 0.53), [19.6, 20.6, RV + 1, RV + 2.1], 99, 0);
	return s + wireWrap(V, 20.9, 21.3, RV + 1.0, 0.95, 0.13, 0.05);
}
function rfCell(V) {
	return pipe(V, [19.8, RV + 1.5], [22.8, RV + 1.5], 0.48, 'teal') + pipe(V, [19.65, RV + 1.5], [19.85, RV + 1.5], 0.3, 'brass') +
		pipe(V, [20.8, RV + 1.5], [21.6, RV + 1.5], 0.51, 'tape') + chipsOn(V, pipeD(V, [19.8, RV + 1.5], [22.8, RV + 1.5], 0.48), [19.8, 22.8, RV + 1, RV + 2], 83, 6, ['#a7aaad']);
}
function rfSide(V) {
	const right = V.kind === 'right';
	let s = '';
	if (!right) s += rfCell(V);
	const base = pth(V, R4(19.6, RV, 26.4, RV + 0.85), 0.15);
	s += ao(shp(base, 'url(#steelPlate)', 'mS')) + rustOn(V, base, [19.6, 26.4, RV, RV + 0.85], 85, 8);
	s += pl(V, [[19.8, RV + 0.25], [26.2, RV + 0.25]], '#2a2c30', 1.2, 'opacity=".6"');
	s += rfEmitter(V);
	// the glass pane's edge leaning back toward the eye, its tin frame and top lip
	s += ao(shp(pth(V, [[24.75, RV + 0.85], [25.2, RV + 0.85], [24.15, RV + 5.3], [23.7, RV + 5.3]], 0.04), 'url(#galv)', 'mS'));
	const g = pth(V, GUS, 0.15) + discD(V, 24.9, RV + 2.0, 0.42);
	s += ao(shp(g, right ? 'url(#sky)' : 'url(#galv)', 'mM', 'fill-rule="evenodd"'));
	if (right) s += chipsOn(V, g, [21.9, 26.3, RV + 0.85, RV + 5.3], 87, 10);
	else s += inside(g, ZINC);
	s += weld(V, [[22.0, RV + 0.9], [26.2, RV + 0.9]], 0.16, 0.09);
	for (const [a, v] of [[25.4, RV + 1.3], [24.6, RV + 4.6]]) s += popRivet(V, a, v, 0.14);
	s += ao(shp(pth(V, [[23.3, RV + 5.15], [25.5, RV + 5.15], [25.6, RV + 5.55], [23.2, RV + 5.55]], 0.06), 'url(#galv)', 'mS'));
	s += shp(pth(V, [[23.5, RV + 3.3], [24.9, RV + 3.15], [24.75, RV + 4.1], [23.55, RV + 4.2]], 0.05), 'url(#tanTape)', 'pS') + inside(pth(V, [[23.5, RV + 3.3], [24.9, RV + 3.15], [24.75, RV + 4.1], [23.55, RV + 4.2]], 0.05), `<rect x="0" y="0" width="${N}" height="${N}" fill="url(#cloth)"/>`);   // tape over a crack
	s += wear(g, '#ffffff', 1.1, 0.6);
	if (!right) {   // a wing nut on the left
		s += ao(shp(pth(V, [[22.3, RV + 0.15], [22.7, RV + 0.3], [23.0, RV + 0.45], [23.3, RV + 0.3], [23.7, RV + 0.15], [23.75, RV + 0.75], [23.0, RV + 0.6], [22.25, RV + 0.75]], 0.08), 'url(#steelPlate)', 'mS'));
		s += ao(circ(V, 23.0, RV + 0.45, 0.26, 'url(#rSteel)', 'mS'));
	} else { s += hexHead(V, 23.0, RV + 0.45, 0.26) + rfCell(V); s += rod(V, [[19.9, RV + 1.9], [19.4, RV + 2.4], [19.6, RV + 2.0]], 0.05, '#d29d48', '#fff1c2'); }
	return s;
}
function rfEnd(V, back) {
	let s = '';
	s += ao(shp(pth(V, R4(-1.3, RV, 1.3, RV + 0.85), 0.12), 'url(#steelPlate)', 'mS')) + rustOn(V, pth(V, R4(-1.3, RV, 1.3, RV + 0.85), 0.12), [-1.3, 1.3, RV, RV + 0.85], back ? 101 : 103, 4);
	s += ao(circ(V, -1.95, RV + 1.5, 0.48, 'url(#rTeal)', 'pS')) + ao(circ(V, -1.95, RV + 1.5, 0.22, 'url(#rBrass)', 'bS'));
	s += ao(shp(pth(V, [[1.4, RV + 0.25], [2.3, RV + 0.05], [2.3, RV + 0.85], [1.4, RV + 0.65]], 0.06), 'url(#steelPlate)', 'mS'));
	if (!back) s += ao(circ(V, 0, RV + 1.55, 0.56, 'url(#rBrass)', 'bS')) + circ(V, 0, RV + 1.55, 0.36, 'url(#lens)', null);
	s += ao(shp(pth(V, R4(1.15, RV + 0.85, 1.45, RV + 5.3), 0.05), 'url(#galv)', 'mS')) + ao(shp(pth(V, R4(-1.45, RV + 0.85, -1.15, RV + 5.3), 0.05), 'url(#sky)', 'mS'));
	const fr = pth(V, R4(-1.2, RV + 0.95, 1.2, RV + 5.25), 0.1), gl = pth(V, R4(-0.98, RV + 1.15, 0.98, RV + 5.05), 0.06);
	s += ao(shp(fr + gl, 'url(#galv)', 'mS', 'fill-rule="evenodd"'));
	s += shp(gl, 'url(#lensAmber)', null) + shp(pth(V, [[-0.9, RV + 4.9], [-0.2, RV + 4.9], [-0.9, RV + 3.2]], 0.05), '#ffffff', null, 'opacity=".18"');
	if (back) s += `<g filter="url(#glow)">${circ(V, 0, RV + 3.2, 0.12, 'url(#redDot)', null)}</g>` + ring(V, 0, RV + 3.2, 0.3, '#ff6a4a', 1, 'opacity=".5"');
	s += ao(shp(pth(V, R4(-1.45, RV + 5.15, 1.45, RV + 5.55), 0.06), 'url(#galv)', 'mS'));
	s += shp(pth(V, R4(-0.6, RV + 4.7, 0.3, RV + 5.05), 0.04), 'url(#tanTape)', 'pS');
	if (back) s += ao(circ(V, 0, RV + 1.55, 0.5, 'url(#rGalv)', 'mS')) + circ(V, 0, RV + 1.55, 0.36, 'url(#rTape)', 'pS') + circ(V, 0, RV + 1.55, 0.12, '#0a0a0a');
	return s + wear(fr, '#ffffff', 1.1, 0.6);
}

// ---- Stock_Skeleton: two tubes and a brace to a tyre-tread pad, a buffer spring, a taped cheek roll (gun space) --------
const TIRE = [[-9.5, -8.9], [-10.9, -9.0], [-11.4, -7.8], [-11.55, -3.3], [-11.4, 1.3], [-10.9, 2.45], [-9.5, 2.35]];
function skSide(V) {
	const right = V.kind === 'right';
	let s = '';
	const loop = () => rod(V, [[-7.9, -6.4], [-8.2, -8.0], [-7.4, -9.0], [-6.5, -8.7], [-6.4, -6.6]], 0.11, '#c99a4a', '#fff1c2');
	const brace = () => pipe(V, [-1.4, -6.4], [-9.2, 0.2], 0.38, 'steel');
	s += right ? loop() : brace();                                                             // far side first
	const tire = pth(V, TIRE, 0.4);
	s += ao(shp(tire, 'url(#rubber)', 'rM')) + shp(tire, 'url(#stipDark)', null, 'opacity=".4"');
	s += inside(tire, Array.from({ length: 14 }, (_, k) => { const v = -8.5 + k * 0.78; return shp(pth(V, R4(-11.7, v, -11.05, v + 0.3), 0.05), '#070707', null) + pl(V, [[-11.7, v + 0.32], [-11.05, v + 0.32]], '#5a5a5a', 1, 'opacity=".5"'); }).join(''));   // tread grooves
	s += pl(V, [[-10.2, -5.0], [-10.6, -3.6], [-10.3, -2.9]], '#050505', 1.6);
	s += ao(shp(pth(V, R4(-9.9, -8.4, -9.35, 2.0), 0.1), 'url(#rust3)', 'mS')) + rustOn(V, pth(V, R4(-9.9, -8.4, -9.35, 2.0), 0.1), [-9.9, -9.35, -8.4, 2.0], 97, 6);
	s += hexHead(V, -9.62, -6.6, 0.2) + hexHead(V, -9.62, 0.4, 0.2);
	const mount = pth(V, R4(-0.5, -8.6, 0.3, 1.8), 0.15);
	s += ao(shp(mount, 'url(#rust3)', 'mM')) + rustOn(V, mount, [-0.5, 0.3, -8.6, 1.8], right ? 91 : 93, 8);
	s += coil(V, -5.4, -1.0, -7.15, 0.85, 7, 0.1, pipe(V, [-0.5, -7.4], [-9.4, -6.6], 0.6, 'rust') + rustOn(V, pipeD(V, [-0.5, -7.4], [-9.4, -6.6], 0.6), [-9.4, -0.5, -8.0, -6.0], 99, 10));
	s += pipe(V, [-0.5, 0.9], [-9.4, 0.9], 0.62, 'galv') + inside(pipeD(V, [-0.5, 0.9], [-9.4, 0.9], 0.62), ZINC);
	const roll = pipeD(V, [-2.4, 0.9], [-8.0, 0.9], 1.05);
	s += pipe(V, [-2.4, 0.9], [-8.0, 0.9], 1.05, 'tan', 'pM') + inside(roll, `<rect x="0" y="0" width="${N}" height="${N}" fill="url(#cloth)"/>`);
	for (let a = -2.5; a > -8.0; a -= 0.6) s += pl(V, [[a, -0.15], [a - 0.32, 1.95]], '#4a3c2c', 1.5, 'opacity=".7"');
	s += shp(pth(V, [[-8.0, 1.95], [-8.6, 2.3], [-8.4, 1.4], [-7.95, 1.2]], 0.05), 'url(#tanTape)', 'pS');
	s += wireWrap(V, -4.0, -3.6, 0.9, 1.08, 0.13, 0.05);
	for (const [a0, a1] of [[-0.55, -0.55]]) s += weld(V, [[a0, 0.1], [a1, 1.7]], 0.18, 0.1) + weld(V, [[a0, -8.0], [a1, -6.8]], 0.18, 0.1);
	s += weld(V, [[-9.3, 0.2], [-9.3, 1.6]], 0.18, 0.1) + weld(V, [[-9.3, -7.3], [-9.3, -5.9]], 0.18, 0.1);
	s += right ? brace() + weld(V, [[-1.5, -6.9], [-1.5, -5.9]], 0.16, 0.09) : loop();
	return s + wear(mount, '#e0a27a', 1.2, 0.6) + wear(tire, '#5a5a5a', 1.4, 0.8, '4 10 2 18 8 22');
}
function skEnd(V, back) {
	let s = '';
	const pad = pth(V, R4(-2.55, -9.0, 2.55, 2.45), 0.9);
	if (back) {
		s += ao(shp(pad, 'url(#rRubber)', 'rM')) + shp(pad, 'url(#stipDark)', null, 'opacity=".45"');
		let t = '';
		for (let v = -8.6; v < 2.4; v += 0.95) {   // tread: chevron blocks and a centre rib
			for (const sd of [-1, 1]) t += shp(pth(V, [[sd * 0.35, v], [sd * 2.6, v + 0.45], [sd * 2.6, v + 0.75], [sd * 0.35, v + 0.3]], 0), '#0b0b0b', null);
		}
		t += shp(pth(V, R4(-0.18, -9, 0.18, 2.5), 0), '#0b0b0b', null);
		s += inside(pad, t);
		s += pl(V, [[-1.4, 1.0], [-0.6, -0.9], [0.4, -1.7], [0.9, -3.0]], '#020202', 1.8) + pl(V, [[1.3, -6.0], [1.9, -7.4]], '#020202', 1.5);
		s += hexHead(V, 0, -6.6, 0.32) + hexHead(V, 0, 0.4, 0.32);
		s += wear(pad, '#6a6a6a', 1.4, 0.8);
	} else {
		s += ao(shp(pad, 'url(#rRubber)', 'rM'));
		s += ao(circ(V, 0, 0.9, 1.05, 'url(#rTan)', 'pM'));
		const m = pth(V, R4(-2.2, -8.6, 2.2, 1.8), 0.3);
		s += ao(shp(m, 'url(#rust3)', 'mM')) + rustOn(V, m, [-2.2, 2.2, -8.6, 1.8], 95, 14);
		for (const sd of [-1, 1]) s += shp(pth(V, R4(sd * 1.5, -7.6, sd * 1.9, 1.0), 0.1), '#2a160b', null, 'opacity=".7"');
		for (const v of [-6.6, 0.2]) s += circ(V, 0, v, 0.34, '#120a06') + ring(V, 0, v, 0.27, '#8e4a2a', 0.9);
		s += ao(circ(V, 0, -3.3, 0.62, 'url(#rSteel)', 'mS')) + circ(V, 0, -3.3, 0.26, '#2a2c30');
		s += weld(V, [[-2.1, -8.5], [2.1, -8.5]], 0.22, 0.1) + drip(V, 1.2, -5.5, 1.6);
		s += wear(m, '#e0a27a', 1.2, 0.6);
	}
	return s;
}

// ---- the parts (set 3 only; b = bounds in cm, gun or part space, one scale across a part's four views) ----------------
const LOOKS = {};
const PARTS = [
	{ set: 3, maker: 'Windfall', dir: 'Body_Salvager', b: { u: [-0.3, 51.9], v: [-16.2, 6.15], w: [-3.6, 3.4] }, draw: { left: salvSide, right: salvSide, front: salvFront, back: salvBack } },
	{ set: 3, maker: 'Windfall', dir: 'Barrel_Shrouded', b: { u: [0, 16.5], v: [-1.9, 2.75], w: [-2.35, 1.9] }, draw: { left: shSide, right: shSide, front: shFront, back: shBack } },
	{ set: 3, maker: 'Windfall', dir: 'Muzzle_Prong', b: { u: [0, 6.2], v: [-1.15, 1.45], w: [-1.3, 1.3] }, draw: { left: prSide, right: prSide, front: prFront, back: prBack } },
	{ set: 3, maker: 'Windfall', dir: 'Magazine_Coupled', b: { u: [10.3, 19.6], v: [-25.9, -5.6], w: [-1.65, 4.75] }, draw: { left: cpSide, right: cpSide, front: V => cpEnd(V, false), back: V => cpEnd(V, true) } },
	{ set: 3, maker: 'Windfall', dir: 'Sight_Reflex', b: { u: [18.9, 26.4], v: [6.15, 11.7], w: [-2.45, 2.3] }, draw: { left: rfSide, right: rfSide, front: V => rfEnd(V, false), back: V => rfEnd(V, true) } },
	{ set: 3, maker: 'Windfall', dir: 'Stock_Skeleton', b: { u: [-11.6, 0.3], v: [-9.2, 2.45], w: [-2.55, 2.55] }, draw: { left: skSide, right: skSide, front: V => skEnd(V, false), back: V => skEnd(V, true) } },
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
		ST = (p.look && LOOKS[p.look]) || {};
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
