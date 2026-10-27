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

function bodySide(V) {
	const right = V.kind === 'right';
	const shellD = pth(V, SHELL, 0.9), loopD = pth(V, LOOP, 0.9) + pth(V, HOLE, 0.9);
	let s = `<clipPath id="shellClip"><path d="${shellD}"/></clipPath>`;
	const handle = ao(shp(pth(V, HANDLE, 0.25), 'url(#black)', 'mS')) + pl(V, [[39.5, 3.45], [43.6, 3.22]], '#9097a0', 1.2, 'opacity=".55"');
	s += shp(pth(V, R4(-0.3, -8.4, 0.3, 1.8), 0.1), 'url(#black)', 'mS');                 // the butt plate's edge
	if (right) s += handle;                                                                 // the handle is on the left
	s += shp(pth(V, TRIGGER, 0.1), 'url(#black)', 'mS');
	s += shp(loopD, 'url(#sand)', 'pL', 'fill-rule="evenodd"');
	// grip stipple on the loop's back strap, framed by a raised edge
	const grip = [[20.1, -6.9], [24.25, -7.5], [25.25, -11.9], [24.7, -14.25], [22.2, -14.45], [20.9, -12.6]];
	s += shp(pth(V, grip, 0.5), 'url(#stip)', null) + shp(pth(V, grip, 0.5), 'none', null, 'stroke="#b3a47f" stroke-width="1.3"');
	s += shp(shellD, 'url(#sand)', 'pL');
	s += `<g clip-path="url(#shellClip)">`;
	s += `<rect x="0" y="${V.Y(3.4)}" width="${N}" height="${13 * V.s}" fill="url(#form)"/>`;
	s += `<ellipse cx="${V.X(44)}" cy="${V.Y(-0.2)}" rx="${6 * V.s}" ry="${3.2 * V.s}" fill="url(#heat)"/>`;    // heat round the vents
	for (const [a, v, rx, ry, c, o] of [[6, -4.4, 2.4, 1.0, '#8a7a58', 0.16], [30, -3.8, 3.5, 0.8, '#8a7a58', 0.14],
		[46, 1.8, 2.2, 0.7, '#ffffff', 0.25], [14, 2.6, 4, 0.5, '#ffffff', 0.22], [2.6, -7.2, 1.2, 1.4, '#7a6a4c', 0.2]])
		s += `<ellipse cx="${V.X(a)}" cy="${V.Y(v)}" rx="${rx * V.s}" ry="${ry * V.s}" fill="${c}" opacity="${o}"/>`;
	s += `</g>`;
	// panel seams: the butt cap and the nose cap
	for (const seam of [[[1.9, 3.25], [1.9, -8.95]], [[38.3, 3.15], [37.9, 1.0], [38.0, -1.6], [38.7, -3.45]]]) {
		s += pl(V, seam, '#8a7a5a', 1.3, 'opacity=".85"') + pl(V, seam.map(([a, v]) => [a + (right ? 0.08 : -0.08), v]), '#fbf6e8', 0.9, 'opacity=".8"');
	}
	if (right) {   // the ejection port and the bolt in it (right side only)
		s += shp(pth(V, R4(2.6, -1.0, 7.4, 1.0), 0.2), '#121316', 'pS');
		s += ao(shp(pth(V, R4(2.95, -0.72, 7.05, 0.72), 0.12), 'url(#steelCyl)', 'mS'));
		s += pl(V, [[3.3, 0.0], [6.7, 0.0]], '#3a3d42', 1.4) + shp(pth(V, R4(6.1, 0.25, 6.7, 0.6), 0.05), '#2a2c30', null);
	}
	// gunmetal side plates: border groove, ribs, rivets
	s += ao(shp(pth(V, PLATE, 0.8), 'url(#blued)', 'mM'));
	const inset = [[8.55, -4.4], [17.85, -4.4], [18.85, 1.2], [9.8, 1.2]];
	s += shp(pth(V, inset, 0.55), 'none', null, 'stroke="#16191d" stroke-width="1.5"') +
		shp(pth(V, inset.map(([a, v]) => [a, v - 0.06]), 0.55), 'none', null, 'stroke="#7f8894" stroke-width=".7" opacity=".6"');
	for (let v = -3.2; v <= 0.01; v += 0.8) {
		const l = 8.55 + (v + 4.4) / 5.6 * 1.25 + 1.1, r = 17.85 + (v + 4.4) / 5.6 * 1.0 - 1.1;
		s += pl(V, [[l, v], [r, v]], '#15181c', 1.6) + pl(V, [[l, v - 0.1], [r, v - 0.1]], '#8a94a0', 0.8, 'opacity=".55"');
	}
	for (const [a, v] of [[9.6, -4.0], [17.6, -4.0], [10.6, 0.8], [18.2, 0.8]]) s += rivet(V, a, v);
	s += wear(pth(V, PLATE, 0.8), '#a9b2bd', 1.1, 0.75, '2 8 6 18 1 9');
	// nose vents with the ember behind them
	for (const u of [40.6, 42.8, 45.0]) {
		s += shp(pth(V, [[u, -1.6], [u + 0.9, -1.6], [u + 2.2, 1.2], [u + 1.3, 1.2]], 0.3), '#1b120c', 'pS');
		s += `<g filter="url(#glow)">${shp(pth(V, [[u + 0.2, -1.35], [u + 0.8, -1.35], [u + 1.98, 0.95], [u + 1.38, 0.95]], 0.2), 'url(#emberLin)', null)}</g>`;
	}
	// the accent line (glows the rarity colour in game): a pale inlay in a channel
	s += shp(pth(V, R4(20.9, -2.86, 37.1, -2.39), 0.1), '#9a8b6c', null) + shp(pth(V, R4(21, -2.75, 37, -2.5), 0.1), '#f3f3ef', 'pS');
	// the selector: a brass lever over the grip
	s += ao(pl(V, [[21.2, -3.9], [23.0, -3.35]], '#3a2a10', 0.42 * V.s) + pl(V, [[21.2, -3.9], [23.0, -3.35]], 'url(#brass)', 0.3 * V.s));
	s += ao(circ(V, 21.2, -3.9, 0.45, 'url(#rBrass)', 'bS')) + circ(V, 21.2, -3.9, 0.12, '#6a4813');
	// serial plate (ticks, no text) and a proof stamp: a star in a ring
	s += ao(shp(pth(V, R4(28.4, 0.1, 34.4, 1.3), 0.15), 'url(#blued)', 'mS'));
	for (let k = 0; k < 12; k++) {
		const a = 29.0 + k * 0.42, hgt = [0.5, 0.3, 0.6, 0.4][k % 4];
		s += pl(V, [[a, 0.45], [a, 0.45 + hgt]], '#aab3bd', 1.1, 'opacity=".7"');
	}
	for (const [a, v] of [[28.65, 0.35], [34.15, 0.35], [28.65, 1.05], [34.15, 1.05]]) s += circ(V, a, v, 0.08, '#c9d0d8');
	s += ring(V, 25.0, 0.7, 0.62, '#9a8862', 1.6) + ring(V, 25.0, 0.66, 0.62, '#fbf5e6', 0.8, 'opacity=".7"');
	s += shp(rounded(star(V.X(25.0), V.Y(0.7), 0.4 * V.s, 0.17 * V.s), 0), '#a8956c', null);
	for (const [a, v] of [[1.0, 1.4], [1.0, -7.0], [36.0, -3.4], [39.4, 2.3], [49.0, -2.0], [23.5, 2.2], [33.0, 2.2]]) s += screw(V, a, v);
	// optic mount with lightening pockets and cross bolts, the rail on it
	s += ao(shp(pth(V, MOUNT, 0.5), 'url(#sand)', 'pM'));
	for (const [a0, a1] of [[14.6, 21.4], [24.6, 31.4]]) {
		s += shp(pth(V, R4(a0, 3.5, a1, 4.6), 0.35), '#c7b78f', 'pS') + shp(pth(V, R4(a0, 3.5, a1, 4.6), 0.35), 'none', null, 'stroke="#8f7f5e" stroke-width="1" opacity=".7"');
	}
	for (const a of [13.0, 33.0]) s += screw(V, a, 3.85, 0.28);
	s += wear(pth(V, MOUNT, 0.5));
	s += rail(V, 14, 32, 5.2);
	// barrel nut (knurled) and sling loop
	s += ao(shp(pth(V, R4(50.6, -1.25, 51.9, 1.25), 0.08), 'url(#blackCyl)', 'mS'));
	for (let a = 50.75; a < 51.8; a += 0.22) s += pl(V, [[a, -1.15], [a, 1.15]], '#0e0f10', 1, 'opacity=".55"');
	const sling = [[43, -3.0], [43.6, -4.6], [45.6, -4.6], [46.2, -3.0]];
	s += ao(pl(V, sling, '#26272a', 0.5 * V.s) + pl(V, sling, '#6c7076', 0.18 * V.s, 'opacity=".8"'));
	// edge wear on the shell and loop
	s += wear(shellD) + wear(loopD, '#fbf6ea', 1.3, 0.8, '2 12 5 20 1 15') + wear(shellD, '#6f6046', 2.2, 0.18, '30 40 12 60');
	if (!right) s += handle;
	return s;
}
function bodyFront(V) {
	let s = shp(pth(V, R4(-2.5, -9.6, 2.5, 3.4), 1.1), 'url(#sandLo)', 'pL');                // the shell's front view
	s += shp(pth(V, R4(-1.8, -15.2, 1.8, -3.8), 0.9), 'url(#sandLoop)', 'pL');               // the loop's front face
	s += shp(pth(V, [[-2.38, -3.5], [2.38, -3.5], [2.42, -1.8], [-2.42, -1.8]], 0.4), 'url(#sandLo)', 'pM');
	s += shp(pth(V, R4(-2.42, -1.8, 2.42, 0.6), 0.3), 'url(#sandMid)', 'pM');
	s += shp(pth(V, [[-2.42, 0.6], [2.42, 0.6], [2.42, 2.4], [1.6, 3.38], [-1.6, 3.38], [-2.42, 2.4]], 0.5), 'url(#sandHi)', 'pM');
	for (const sd of [-1, 1]) {
		s += ao(shp(pth(V, R4(sd * 2.5, -4.8, sd * 2.85, 1.6), 0.1), 'url(#blued)', 'mS'));
		s += `<g filter="url(#glow)">${shp(pth(V, R4(sd * 2.3, -1.4, sd * 2.5, 1.0), 0.05), 'url(#emberLin)', null)}</g>`;   // vents' glow at the edge
	}
	s += ao(shp(pth(V, R4(-1.8, 3.0, 1.8, 4.4), 0.2), 'url(#sandMid)', 'pM') + shp(pth(V, [[-1.8, 4.35], [1.8, 4.35], [1.45, 5.2], [-1.45, 5.2]], 0.2), 'url(#sandHi)', 'pS'));
	s += ao(shp(pth(V, RAIL_X, 0.03), 'url(#black)', 'mS'));
	s += ao(shp(pth(V, R4(1.4, 2.6, 2.4, 3.9), 0.25), 'url(#black)', 'mS'));                 // charging handle (gun's left)
	s += ao(circ(V, 0, 0, 1.25, 'url(#rBlack)', 'mM'));                                       // barrel nut, threaded bore
	s += ring(V, 0, 0, 1.0, '#101113', 1.4) + circ(V, 0, 0, 0.82, 'url(#bore)', null);
	for (const r of [0.74, 0.64, 0.54]) s += ring(V, 0, 0, r, '#4a4d52', 0.9, 'opacity=".8"');
	for (let k = 0; k < 4; k++) {
		const a = (45 + 90 * k) * Math.PI / 180;
		s += circ(V, 1.13 * Math.cos(a), 1.13 * Math.sin(a), 0.14, '#0f1012');
	}
	s += ao(shp(pth(V, R4(-0.25, -4.85, 0.25, -3.0), 0.22), 'url(#blackCylH)', 'mS'));
	s += wear(pth(V, R4(-2.5, -9.6, 2.5, 3.4), 1.1)) + wear(pth(V, R4(-1.8, -15.2, 1.8, -3.8), 0.9), '#fbf6ea', 1.2, 0.7);
	return s;
}
function bodyBack(V) {
	let s = shp(pth(V, R4(-1.8, -15.2, 1.8, -3.8), 0.9), 'url(#sandLoop)', 'pL');
	s += shp(pth(V, R4(-2.5, -9.6, 2.5, 3.4), 1.1), 'url(#sandMid)', 'pL');
	s += shp(pth(V, [[-2.45, 2.0], [2.45, 2.0], [2.45, 2.6], [1.7, 3.38], [-1.7, 3.38], [-2.45, 2.6]], 0.4), 'url(#sandHi)', 'pM');
	s += shp(pth(V, R4(-2.4, -9.55, 2.4, -8.6), 0.6), 'url(#sandLo)', 'pS');
	for (const sd of [-1, 1]) s += ao(shp(pth(V, R4(sd * 2.5, -4.8, sd * 2.85, 1.6), 0.1), 'url(#blued)', 'mS'));
	// the butt plate the stock seats on: guide rails, two bolts, the locating boss at the Stock socket
	s += ao(shp(pth(V, R4(-2.2, -8.4, 2.2, 1.8), 0.3), 'url(#black)', 'mM'));
	for (const sd of [-1, 1]) s += shp(pth(V, R4(sd * 1.55, -7.8, sd * 1.85, 1.2), 0.1), '#5d6168', 'mS');
	s += screw(V, 0, -6.6, 0.38) + screw(V, 0, 0.2, 0.38);
	s += ao(circ(V, 0, -3.3, 0.75, 'url(#rBlack)', 'mS')) + circ(V, 0, -3.3, 0.38, '#0f1012');
	s += wear(pth(V, R4(-2.2, -8.4, 2.2, 1.8), 0.3), '#a3abb5', 1.1, 0.7);
	s += ao(shp(pth(V, R4(-1.8, 3.0, 1.8, 4.4), 0.2), 'url(#sandMid)', 'pM') + shp(pth(V, [[-1.8, 4.35], [1.8, 4.35], [1.45, 5.2], [-1.45, 5.2]], 0.2), 'url(#sandHi)', 'pS'));
	s += ao(shp(pth(V, RAIL_X, 0.03), 'url(#black)', 'mS'));
	s += ao(shp(pth(V, R4(1.4, 2.6, 2.4, 3.9), 0.25), 'url(#black)', 'mS'));
	s += wear(pth(V, R4(-2.5, -9.6, 2.5, 3.4), 1.1)) + wear(pth(V, R4(-1.8, -15.2, 1.8, -3.8), 0.9), '#fbf6ea', 1.2, 0.7);
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
