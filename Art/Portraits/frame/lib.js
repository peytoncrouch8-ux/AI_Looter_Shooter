// Geometry + SVG helpers for the portrait frame generator.
const D2R = Math.PI / 180;
const R = v => Math.round(v * 10) / 10;
const P = p => R(p[0]) + ' ' + R(p[1]);
const INK = '#0a1218';

function poly(pts, close = true) { return 'M' + pts.map(P).join('L') + (close ? 'Z' : ''); }

// Catmull-Rom through points -> cubic segments (no leading M when cont=true)
function smoothSeg(pts, closed = false) {
	const n = pts.length;
	let d = '';
	const g = i => closed ? pts[(i + n) % n] : pts[Math.max(0, Math.min(n - 1, i))];
	const last = closed ? n : n - 1;
	for (let i = 0; i < last; i++) {
		const p0 = g(i - 1), p1 = g(i), p2 = g(i + 1), p3 = g(i + 2);
		const c1 = [p1[0] + (p2[0] - p0[0]) / 6, p1[1] + (p2[1] - p0[1]) / 6];
		const c2 = [p2[0] - (p3[0] - p1[0]) / 6, p2[1] - (p3[1] - p1[1]) / 6];
		d += 'C' + P(c1) + ' ' + P(c2) + ' ' + P(p2);
	}
	return d;
}
function smooth(pts, closed = false) { return 'M' + P(pts[0]) + smoothSeg(pts, closed) + (closed ? 'Z' : ''); }

// Spiral/volute centreline: curvature ramps from k0 to k1 (per px), angle in degrees.
function curl(x, y, ang, len, k0, k1, p = 2, n = 44) {
	const pts = [[x, y]];
	let th = ang * D2R;
	const ds = len / n;
	for (let i = 1; i <= n; i++) {
		const s = (i - 0.5) / n;
		th += (k0 + (k1 - k0) * Math.pow(s, p)) * ds;
		x += Math.cos(th) * ds; y += Math.sin(th) * ds;
		pts.push([x, y]);
	}
	return pts;
}
function bez(p0, p1, p2, p3, n = 36) {
	const pts = [];
	for (let i = 0; i <= n; i++) {
		const t = i / n, u = 1 - t;
		pts.push([u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t * t * t * p3[0],
			u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t * t * t * p3[1]]);
	}
	return pts;
}
// Resample a polyline to n points evenly by arc length.
function resample(pts, n) {
	const acc = [0];
	for (let i = 1; i < pts.length; i++) acc.push(acc[i - 1] + Math.hypot(pts[i][0] - pts[i - 1][0], pts[i][1] - pts[i - 1][1]));
	const L = acc[acc.length - 1], out = [];
	let j = 0;
	for (let i = 0; i < n; i++) {
		const s = L * i / (n - 1);
		while (j < acc.length - 2 && acc[j + 1] < s) j++;
		const t = (s - acc[j]) / ((acc[j + 1] - acc[j]) || 1);
		out.push([pts[j][0] + (pts[j + 1][0] - pts[j][0]) * t, pts[j][1] + (pts[j + 1][1] - pts[j][1]) * t]);
	}
	return out;
}
function tangents(pts) {
	const n = pts.length;
	return pts.map((p, i) => {
		const a = pts[Math.max(0, i - 1)], b = pts[Math.min(n - 1, i + 1)];
		let dx = b[0] - a[0], dy = b[1] - a[1];
		const d = Math.hypot(dx, dy) || 1;
		return [dx / d, dy / d];
	});
}
// Tapered ribbon around a centreline; wf(t) gives half width (t 0..1), optional lobes on one side.
function ribbon(pts, wf, opt = {}) {
	const T = tangents(pts), n = pts.length, L = [], Rr = [];
	for (let i = 0; i < n; i++) {
		const t = i / (n - 1);
		let wl = wf(t), wr = wf(t);
		if (opt.lobes) { const b = Math.abs(Math.sin(Math.PI * t * opt.lobes)); if (opt.lobeSide > 0) wl *= 1 + opt.lobeAmt * b; else wr *= 1 + opt.lobeAmt * b; }
		L.push([pts[i][0] - T[i][1] * wl, pts[i][1] + T[i][0] * wl]);
		Rr.push([pts[i][0] + T[i][1] * wr, pts[i][1] - T[i][0] * wr]);
	}
	Rr.reverse();
	return 'M' + P(L[0]) + smoothSeg(L) + 'L' + P(Rr[0]) + smoothSeg(Rr) + 'Z';
}
const taper = (w0, w1, ease = 1) => t => w0 + (w1 - w0) * Math.pow(t, ease);
// swelling scroll: thin start, fat belly, thin end
const swell = (w0, wm, w1) => t => t < 0.35 ? w0 + (wm - w0) * Math.sin(t / 0.35 * Math.PI / 2) : wm + (w1 - wm) * Math.pow((t - 0.35) / 0.65, 0.8);

function circle(cx, cy, r, attrs = '') { return `<circle cx="${R(cx)}" cy="${R(cy)}" r="${R(r)}" ${attrs}/>`; }
function ringPath(cx, cy, r1, r2) {
	const c = r => `M${R(cx - r)} ${R(cy)}A${r} ${r} 0 1 0 ${R(cx + r)} ${R(cy)}A${r} ${r} 0 1 0 ${R(cx - r)} ${R(cy)}Z`;
	return c(r2) + c(r1);
}
const mirror = s => `<g transform="matrix(-1 0 0 1 1024 0)">${s}</g>`;
const both = s => s + mirror(s);

// gradients
function lin(id, stops, x1 = 0, y1 = 0, x2 = 0, y2 = 1, user = false) {
	return `<linearGradient id="${id}" x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}"${user ? ' gradientUnits="userSpaceOnUse"' : ''}>` +
		stops.map(s => `<stop offset="${s[0]}" stop-color="${s[1]}"${s[2] !== undefined ? ` stop-opacity="${s[2]}"` : ''}/>`).join('') + '</linearGradient>';
}
function rad(id, stops, cx = 0.5, cy = 0.5, r = 0.5, fx, fy, user = false) {
	return `<radialGradient id="${id}" cx="${cx}" cy="${cy}" r="${r}"${fx !== undefined ? ` fx="${fx}" fy="${fy}"` : ''}${user ? ' gradientUnits="userSpaceOnUse"' : ''}>` +
		stops.map(s => `<stop offset="${s[0]}" stop-color="${s[1]}"${s[2] !== undefined ? ` stop-opacity="${s[2]}"` : ''}/>`).join('') + '</radialGradient>';
}

// Lit-metal filter: diffuse + specular from the upper left, crisp ink outline, soft drop shadow.
function metal(id, o = {}) {
	const { blur = 1.5, surf = 3, diffK = 1.2, elev = 50, specK = 0.9, specExp = 16, specAmt = 0.7, ol = 1.1, sdx = 2, sdy = 3, sblur = 2.5, sop = 0.6, pad = 25, light = '#fff3dc' } = o;
	return `<filter id="${id}" x="-${pad}%" y="-${pad}%" width="${100 + 2 * pad}%" height="${100 + 2 * pad}%" color-interpolation-filters="sRGB">` +
		`<feGaussianBlur in="SourceAlpha" stdDeviation="${blur}" result="b"/>` +
		`<feDiffuseLighting in="b" surfaceScale="${surf}" diffuseConstant="${diffK}" lighting-color="#ffffff" result="d"><feDistantLight azimuth="225" elevation="${elev}"/></feDiffuseLighting>` +
		`<feSpecularLighting in="b" surfaceScale="${surf}" specularConstant="${specK}" specularExponent="${specExp}" lighting-color="${light}" result="s"><feDistantLight azimuth="225" elevation="${elev - 10}"/></feSpecularLighting>` +
		`<feComposite in="SourceGraphic" in2="d" operator="arithmetic" k1="1" k2="0" k3="0" k4="0" result="lit"/>` +
		`<feComposite in="s" in2="SourceAlpha" operator="in" result="sIn"/>` +
		`<feComposite in="lit" in2="sIn" operator="arithmetic" k1="0" k2="1" k3="${specAmt}" k4="0" result="lit2"/>` +
		`<feComposite in="lit2" in2="SourceAlpha" operator="in" result="obj"/>` +
		(ol > 0 ? `<feMorphology in="SourceAlpha" operator="dilate" radius="${ol}" result="dil"/>` : `<feOffset in="SourceAlpha" result="dil"/>`) +
		`<feFlood flood-color="${INK}" result="ink"/><feComposite in="ink" in2="dil" operator="in" result="olS"/>` +
		`<feGaussianBlur in="dil" stdDeviation="${sblur}" result="sb"/><feOffset in="sb" dx="${sdx}" dy="${sdy}" result="so"/>` +
		`<feFlood flood-color="#000" flood-opacity="${sop}"/><feComposite in2="so" operator="in" result="sh"/>` +
		`<feMerge><feMergeNode in="sh"/><feMergeNode in="olS"/><feMergeNode in="obj"/></feMerge></filter>`;
}
// outline + drop shadow only (hand-shaded pieces)
function drop(id, o = {}) {
	const { ol = 1.1, sdx = 2.5, sdy = 3.5, sblur = 3, sop = 0.6, pad = 25 } = o;
	return `<filter id="${id}" x="-${pad}%" y="-${pad}%" width="${100 + 2 * pad}%" height="${100 + 2 * pad}%" color-interpolation-filters="sRGB">` +
		(ol > 0 ? `<feMorphology in="SourceAlpha" operator="dilate" radius="${ol}" result="dil"/>` : `<feOffset in="SourceAlpha" result="dil"/>`) +
		`<feFlood flood-color="${INK}" result="ink"/><feComposite in="ink" in2="dil" operator="in" result="olS"/>` +
		`<feGaussianBlur in="dil" stdDeviation="${sblur}" result="sb"/><feOffset in="sb" dx="${sdx}" dy="${sdy}" result="so"/>` +
		`<feFlood flood-color="#000" flood-opacity="${sop}"/><feComposite in2="so" operator="in" result="sh"/>` +
		`<feMerge><feMergeNode in="sh"/><feMergeNode in="olS"/><feMergeNode in="SourceGraphic"/></feMerge></filter>`;
}
// grime: multiply by two scales of noise, keep shape
function grit(id, o = {}) {
	const { f1 = 0.9, f2 = 0.035, a1 = 0.35, a2 = 0.45, seed = 3 } = o;
	return `<filter id="${id}" x="0%" y="0%" width="100%" height="100%" color-interpolation-filters="sRGB">` +
		`<feTurbulence type="fractalNoise" baseFrequency="${f1}" numOctaves="2" seed="${seed}" result="n1"/>` +
		`<feTurbulence type="fractalNoise" baseFrequency="${f2}" numOctaves="3" seed="${seed + 7}" result="n2"/>` +
		`<feColorMatrix in="n1" type="matrix" values="0 0 0 0 1  0 0 0 0 1  0 0 0 0 1  ${a1} 0 0 0 ${-a1 * 0.3}" result="g1"/>` +
		`<feColorMatrix in="n2" type="matrix" values="0 0 0 0 1  0 0 0 0 1  0 0 0 0 1  ${a2 * 2} 0 0 0 ${-a2 * 0.75}" result="g2"/>` +
		`<feComposite in="SourceGraphic" in2="g1" operator="arithmetic" k1="0" k2="1" k3="0" k4="0" result="src"/>` +
		`<feFlood flood-color="#000000"/><feComposite in2="g1" operator="in" result="d1"/>` +
		`<feFlood flood-color="#000000"/><feComposite in2="g2" operator="in" result="d2"/>` +
		`<feMerge result="m"><feMergeNode in="SourceGraphic"/><feMergeNode in="d1"/><feMergeNode in="d2"/></feMerge>` +
		`<feComposite in="m" in2="SourceAlpha" operator="in"/></filter>`;
}
function glow(id, std, amt = 1) {
	return `<filter id="${id}" x="-100%" y="-100%" width="300%" height="300%" color-interpolation-filters="sRGB">` +
		`<feGaussianBlur in="SourceGraphic" stdDeviation="${std}" result="b"/>` +
		`<feComponentTransfer in="b" result="b2"><feFuncA type="linear" slope="${amt}"/></feComponentTransfer>` +
		`<feMerge><feMergeNode in="b2"/><feMergeNode in="SourceGraphic"/></feMerge></filter>`;
}
function blurF(id, std, pad = 50) {
	return `<filter id="${id}" x="-${pad}%" y="-${pad}%" width="${100 + 2 * pad}%" height="${100 + 2 * pad}%"><feGaussianBlur stdDeviation="${std}"/></filter>`;
}

module.exports = { D2R, R, P, INK, poly, smooth, smoothSeg, curl, bez, resample, tangents, ribbon, taper, swell, circle, ringPath, mirror, both, lin, rad, metal, drop, grit, glow, blurF };
