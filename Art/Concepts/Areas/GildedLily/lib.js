// Shared drawing kit for the Gilded Lily top-down concept maps (gen.js builds the three maps with it).
// Everything is plain SVG strings: painted-look terrain from layered strokes, blur and fractal noise; no images.
const fs = require('fs');
const path = require('path');

// ---------- canvas and palette ----------
const W = 2048, H = 1400;
const MAP = { x: 28, y: 108, w: 1440, h: 1264 };           // map window on the canvas
const PANEL = { x: 1490, y: 108, w: 530, h: 1264 };        // right-hand info panel
const C = {
	bg: '#071a28', plate: '#0e2c42', line: '#5ac8ff', accent: '#ff9f1c', title: '#bfeaff', text: '#dcefff',
	dim: '#8fb3cc', danger: '#ff5b4a', ink: '#120d0a',
	deep: '#0d2233', mid: '#173a4c', shelf: '#24566a', shallow: '#3a7f86', surf: '#a9d8cc',
	sand: '#d6b47a', wetsand: '#b69460', grass: '#7c7f45', grass2: '#8f8a4f', scrub: '#5f6b3a', rock: '#6e5c4c',
	rockHi: '#9a8670', cliff: '#2b211b', ridge: '#4a3a30', mud: '#7b6a4b', road: '#c9a87a', roadEdge: '#5a4533',
	roofs: ['#8e3b2c', '#a2492f', '#6f3326', '#7d5a3c', '#5c4a3e', '#9b6a3a', '#7a2f2a'],
	gold: '#e3b04b', goldDeep: '#a8741f', velvet: '#7a1d24', lamp: '#ffd27a', crew: '#ff5b4a', beast: '#ff7a5c',
	slot: '#ff5fb0', chest: '#e3b04b', secret: '#5ac8ff',
};

// ---------- tiny maths ----------
function rng(seed) { let s = seed >>> 0 || 1; return () => { s = (Math.imul(s, 1664525) + 1013904223) >>> 0; return s / 4294967296; }; }
const f1 = v => Math.round(v * 10) / 10;
function dist(a, b) { return Math.hypot(a[0] - b[0], a[1] - b[1]); }
function lerp(a, b, t) { return [a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t]; }
function pip(p, poly) {
	let inside = false;
	for (let i = 0, j = poly.length - 1; i < poly.length; j = i++) {
		const [xi, yi] = poly[i], [xj, yj] = poly[j];
		if ((yi > p[1]) !== (yj > p[1]) && p[0] < (xj - xi) * (p[1] - yi) / (yj - yi) + xi) inside = !inside;
	}
	return inside;
}
function inAny(p, polys) { return polys.some(poly => pip(p, poly)); }
function segDist(p, a, b) {
	const dx = b[0] - a[0], dy = b[1] - a[1], l2 = dx * dx + dy * dy;
	let t = l2 ? ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / l2 : 0; t = Math.max(0, Math.min(1, t));
	return Math.hypot(p[0] - a[0] - t * dx, p[1] - a[1] - t * dy);
}
function lineDist(p, pts) { let d = 1e9; for (let i = 1; i < pts.length; i++) d = Math.min(d, segDist(p, pts[i - 1], pts[i])); return d; }
// Densify a polyline with a Catmull-Rom spline so offsets and samples follow the curve.
function spline(pts, closed, per = 8) {
	const out = [], n = pts.length;
	const get = i => closed ? pts[(i + n) % n] : pts[Math.max(0, Math.min(n - 1, i))];
	const segs = closed ? n : n - 1;
	for (let i = 0; i < segs; i++) {
		const p0 = get(i - 1), p1 = get(i), p2 = get(i + 1), p3 = get(i + 2);
		for (let k = 0; k < per; k++) {
			const t = k / per, t2 = t * t, t3 = t2 * t;
			out.push([0, 1].map(c => 0.5 * (2 * p1[c] + (-p0[c] + p2[c]) * t + (2 * p0[c] - 5 * p1[c] + 4 * p2[c] - p3[c]) * t2 + (-p0[c] + 3 * p1[c] - 3 * p2[c] + p3[c]) * t3)));
		}
	}
	if (!closed) out.push(pts[n - 1]);
	return out;
}
// Roughen a coastline: spline it, then push each point along its normal by layered noise.
function roughen(pts, closed, amp = 5, seed = 1, per = 10) {
	const R = rng(seed), sp = spline(pts, closed, per), n = sp.length;
	const ph = [R() * 6, R() * 6, R() * 6];
	let s = 0;
	return sp.map((p, i) => {
		const a = sp[(i - 1 + n) % n], b = sp[(i + 1) % n];
		if (i > 0) s += dist(p, sp[i - 1]);
		let nx = -(b[1] - a[1]), ny = b[0] - a[0]; const l = Math.hypot(nx, ny) || 1; nx /= l; ny /= l;
		const k = amp * (0.55 * Math.sin(s * 0.045 + ph[0]) + 0.3 * Math.sin(s * 0.13 + ph[1]) + 0.25 * (R() - 0.5) * 2);
		return [p[0] + nx * k, p[1] + ny * k];
	});
}
function d(pts, closed) { return 'M' + pts.map(p => f1(p[0]) + ',' + f1(p[1])).join('L') + (closed ? 'Z' : ''); }
function smooth(pts, closed, per = 8) { return d(spline(pts, closed, per), closed); }
// Offset a polyline sideways (for building rows along streets, beach bands).
function offsetLine(pts, off) {
	return pts.map((p, i) => {
		const a = pts[Math.max(0, i - 1)], b = pts[Math.min(pts.length - 1, i + 1)];
		let nx = -(b[1] - a[1]), ny = b[0] - a[0]; const l = Math.hypot(nx, ny) || 1;
		return [p[0] + nx / l * off, p[1] + ny / l * off];
	});
}
// Points every `step` along a polyline with their heading.
function walk(pts, step, start = 0) {
	const out = []; let carry = start;
	for (let i = 1; i < pts.length; i++) {
		const a = pts[i - 1], b = pts[i], L = dist(a, b), ang = Math.atan2(b[1] - a[1], b[0] - a[0]);
		while (carry <= L) { out.push({ p: lerp(a, b, carry / L), ang }); carry += step; }
		carry -= L;
	}
	return out;
}
// Grid-sampled area of the playable land (land minus the closing ridges and water channels).
function playArea(isLand, step = 4) {
	let n = 0;
	for (let y = 0; y < MAP.h; y += step) for (let x = 0; x < MAP.w; x += step) if (isLand([x + step / 2, y + step / 2])) n++;
	return n * step * step;
}

// ---------- SVG bits ----------
const esc = s => String(s).replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;');
function text(x, y, s, o = {}) {
	const size = o.size || 15, fill = o.fill || C.text, w = o.weight || 600, anchor = o.anchor || 'middle';
	const halo = o.halo === false ? '' : ` stroke="${o.haloColor || C.ink}" stroke-width="${o.haloW || Math.max(3, size * 0.3)}" stroke-linejoin="round" paint-order="stroke"`;
	const ls = o.ls ? ` letter-spacing="${o.ls}"` : '';
	const it = o.italic ? ' font-style="italic"' : '';
	const rot = o.rot ? ` transform="rotate(${o.rot} ${f1(x)} ${f1(y)})"` : '';
	return `<text x="${f1(x)}" y="${f1(y)}" font-size="${size}" font-weight="${w}" fill="${fill}" text-anchor="${anchor}"${ls}${it}${halo}${rot}>${esc(s)}</text>`;
}
// A district title with its one-line note under it.
function district(x, y, name, note, o = {}) {
	let s = text(x, y, name.toUpperCase(), { size: o.size || 24, weight: 700, fill: o.fill || '#f6e7c1', ls: 2.5, haloW: 7, rot: o.rot });
	if (note) s += text(x, y + (o.size || 24) * 0.9 + 4, note, { size: 14, weight: 400, fill: '#e9d9b6', italic: true, haloW: 4.5, rot: o.rot });
	return s;
}

// ---------- terrain painters ----------
// Water: deep base, sunset glow from the west, then depth bands painted as wide blurred strokes of every coastline.
function waterLayer(coasts, o = {}) {
	const strokes = (w, col, op) => coasts.map(c => `<path d="${c}" fill="none" stroke="${col}" stroke-width="${w}" stroke-opacity="${op}" stroke-linejoin="round"/>`).join('');
	return `<rect x="-50" y="-50" width="${MAP.w + 100}" height="${MAP.h + 100}" fill="url(#seaGrad)"/>
<rect x="-50" y="-50" width="${MAP.w + 100}" height="${MAP.h + 100}" filter="url(#seaNoise)" opacity="0.55"/>
<rect x="-50" y="-50" width="${MAP.w + 100}" height="${MAP.h + 100}" fill="url(#sunGlow)" style="mix-blend-mode:screen"/>
<g filter="url(#blur18)">${strokes(170, C.mid, 0.9)}</g>
<g filter="url(#blur10)">${strokes(96, C.shelf, 0.95)}</g>
<g filter="url(#blur6)">${strokes(46, C.shallow, 0.95)}</g>
<g filter="url(#blur3)">${strokes(16, C.surf, 0.55)}</g>`;
}
// Small wave strokes scattered over open water.
function waves(isWater, n, seed) {
	const R = rng(seed); let s = '';
	for (let i = 0; i < n; i++) {
		const x = R() * MAP.w, y = R() * MAP.h;
		if (!isWater([x, y]) || !isWater([x + 30, y]) || !isWater([x - 30, y])) continue;
		const w = 10 + R() * 16;
		s += `<path d="M${f1(x - w)},${f1(y)} q${f1(w / 2)},-5 ${f1(w)},0 t${f1(w)},0" fill="none" stroke="#cfe6e8" stroke-opacity="${f1(0.08 + R() * 0.12)}" stroke-width="1.6" stroke-linecap="round"/>`;
	}
	return s;
}
// Land: sand base, then an inner upland fill, mottled by noise and clipped to the land.
function landLayer(id, coastD, uplandDs, o = {}) {
	return `<clipPath id="${id}"><path d="${coastD}"/></clipPath>
<path d="${coastD}" fill="${C.sand}"/>
<g clip-path="url(#${id})">
<path d="${coastD}" fill="none" stroke="${C.wetsand}" stroke-width="10" stroke-opacity="0.8"/>
${uplandDs.map(u => `<path d="${u}" fill="${C.grass}" filter="url(#soft)"/>`).join('')}
<rect x="-50" y="-50" width="${MAP.w + 100}" height="${MAP.h + 100}" filter="url(#mottleDark)" opacity="0.5"/>
<rect x="-50" y="-50" width="${MAP.w + 100}" height="${MAP.h + 100}" filter="url(#mottleLight)" opacity="0.35"/>
</g>
<path d="${coastD}" fill="none" stroke="#3b2c20" stroke-width="1.6" stroke-opacity="0.75"/>`;
}
// Closing hills: dark hatched ridges with crest lines.
function ridgeLayer(ridgeDs, crests = [], clipId) {
	const open = clipId ? `<g clip-path="url(#${clipId})">` : '<g>';
	return open + ridgeDs.map(r => `<path d="${r}" fill="${C.ridge}"/><path d="${r}" fill="url(#hatch)" opacity="0.55"/><path d="${r}" fill="none" stroke="#1f1712" stroke-width="3" stroke-dasharray="2 5" stroke-linecap="round"/>`).join('')
		+ crests.map(c => `<path d="${c}" fill="none" stroke="#8a7460" stroke-width="2" stroke-opacity="0.6" stroke-dasharray="10 6"/>`).join('') + '</g>';
}
// A raised headland or hill: stacked rings lighter toward the top (a painted contour look).
function hill(pts, seed, rings = 3, col = C.rock) {
	let s = '';
	for (let i = 0; i < rings; i++) {
		const k = 1 - i * 0.24, cx = pts.reduce((a, p) => a + p[0], 0) / pts.length, cy = pts.reduce((a, p) => a + p[1], 0) / pts.length;
		const ring = roughen(pts.map(p => [cx + (p[0] - cx) * k, cy + (p[1] - cy) * k]), true, 4, seed + i, 6);
		s += `<path d="${d(ring, true)}" fill="${i === 0 ? col : i === 1 ? '#7f6b58' : '#94806a'}" opacity="${i === 0 ? 0.9 : 0.85}" stroke="#3a2b20" stroke-width="1" stroke-opacity="0.5" filter="url(#soft)"/>`;
	}
	return s;
}
// A cliff edge: dark line with hachures pointing downhill (side +1 or -1 picks which side is down).
function cliff(pts, side = 1, seed = 1, o = {}) {
	const R = rng(seed), line = roughen(pts, false, 3, seed, 8);
	let s = `<path d="${d(line)}" fill="none" stroke="${C.cliff}" stroke-width="${o.w || 5}" stroke-linecap="round" stroke-linejoin="round"/>`;
	for (const { p, ang } of walk(line, o.step || 9)) {
		const nx = -Math.sin(ang) * side, ny = Math.cos(ang) * side, L = (o.len || 12) * (0.6 + R() * 0.6);
		s += `<line x1="${f1(p[0])}" y1="${f1(p[1])}" x2="${f1(p[0] + nx * L)}" y2="${f1(p[1] + ny * L)}" stroke="${C.cliff}" stroke-width="2" stroke-opacity="0.8" stroke-linecap="round"/>`;
	}
	return s;
}
// Tide flats: mud with silver runnels and a few pools.
function flats(pts, seed) {
	const R = rng(seed), ring = roughen(pts, true, 6, seed, 10), dd = d(ring, true);
	const xs = ring.map(p => p[0]), ys = ring.map(p => p[1]);
	const [x0, x1, y0, y1] = [Math.min(...xs), Math.max(...xs), Math.min(...ys), Math.max(...ys)];
	let s = `<path d="${dd}" fill="${C.mud}" opacity="0.88" filter="url(#soft)"/><clipPath id="fl${seed}"><path d="${dd}"/></clipPath><g clip-path="url(#fl${seed})">`;
	for (let i = 0; i < 26; i++) {
		const x = x0 + R() * (x1 - x0), y = y0 + R() * (y1 - y0), L = 30 + R() * 70, a = R() * 0.8 - 0.4;
		s += `<path d="M${f1(x)},${f1(y)} q${f1(L / 3)},${f1(8 * (R() - 0.5) + a * L)} ${f1(L)},${f1(a * L)}" fill="none" stroke="#a9c3bd" stroke-opacity="0.45" stroke-width="${f1(1 + R() * 2)}" stroke-linecap="round"/>`;
	}
	for (let i = 0; i < 9; i++) {
		const x = x0 + R() * (x1 - x0), y = y0 + R() * (y1 - y0);
		s += `<ellipse cx="${f1(x)}" cy="${f1(y)}" rx="${f1(5 + R() * 10)}" ry="${f1(3 + R() * 6)}" fill="${C.shallow}" opacity="0.85"/>`;
	}
	return s + `</g><path d="${dd}" fill="none" stroke="#5b4c35" stroke-width="1.2" stroke-dasharray="3 4" opacity="0.7"/>`;
}
// Trees (dark coastal pines and scrub) scattered inside an area, away from things that must stay clear.
function trees(isOk, box, n, seed, o = {}) {
	const R = rng(seed); let s = '', placed = 0;
	for (let i = 0; i < n * 6 && placed < n; i++) {
		const x = box[0] + R() * (box[2] - box[0]), y = box[1] + R() * (box[3] - box[1]);
		if (!isOk([x, y])) continue;
		placed++;
		const r = (o.r || 6) * (0.7 + R() * 0.6), col = o.col || (R() < 0.5 ? '#3f4f2c' : '#4d5a30');
		s += `<circle cx="${f1(x + 2.5)}" cy="${f1(y + 2.5)}" r="${f1(r)}" fill="#1a140e" opacity="0.35"/><circle cx="${f1(x)}" cy="${f1(y)}" r="${f1(r)}" fill="${col}"/><circle cx="${f1(x - r * 0.3)}" cy="${f1(y - r * 0.3)}" r="${f1(r * 0.45)}" fill="#7d8a4a" opacity="0.55"/>`;
	}
	return s;
}
function rocks(isOk, box, n, seed, o = {}) {
	const R = rng(seed); let s = '', placed = 0;
	for (let i = 0; i < n * 8 && placed < n; i++) {
		const x = box[0] + R() * (box[2] - box[0]), y = box[1] + R() * (box[3] - box[1]);
		if (!isOk([x, y])) continue;
		placed++;
		const r = (o.r || 4) * (0.6 + R() * 0.9);
		s += `<ellipse cx="${f1(x + 1.5)}" cy="${f1(y + 1.5)}" rx="${f1(r * 1.2)}" ry="${f1(r)}" fill="#1a140e" opacity="0.4"/><ellipse cx="${f1(x)}" cy="${f1(y)}" rx="${f1(r * 1.2)}" ry="${f1(r)}" fill="${o.col || '#8b7a68'}" stroke="#3a2e24" stroke-width="0.8"/>`;
	}
	return s;
}
function road(pts, w = 9, o = {}) {
	const dd = smooth(pts, false, 6);
	return `<path d="${dd}" fill="none" stroke="${C.roadEdge}" stroke-width="${w + 3}" stroke-linecap="round" stroke-linejoin="round" opacity="0.85"/><path d="${dd}" fill="none" stroke="${o.col || C.road}" stroke-width="${w}" stroke-linecap="round" stroke-linejoin="round"/>`;
}
function trail(pts) { return `<path d="${smooth(pts, false, 6)}" fill="none" stroke="#5a4533" stroke-width="2.4" stroke-dasharray="6 5" stroke-linecap="round" opacity="0.9"/>`; }
function rail(pts) {
	const sp = spline(pts, false, 6), dd = d(sp);
	let ties = '';
	for (const { p, ang } of walk(sp, 7)) { const nx = -Math.sin(ang) * 7, ny = Math.cos(ang) * 7; ties += `M${f1(p[0] - nx)},${f1(p[1] - ny)}L${f1(p[0] + nx)},${f1(p[1] + ny)}`; }
	return `<path d="${ties}" stroke="#2a1d14" stroke-width="3"/><path d="${dd}" fill="none" stroke="#1b1410" stroke-width="5"/><path d="${dd}" fill="none" stroke="#9aa3ad" stroke-width="1.2" opacity="0.9"/>`;
}
// A rooftop: drop shadow, body, ridge line; lit windows optional.
function roof(x, y, w, h, ang, col, o = {}) {
	const g = `translate(${f1(x)} ${f1(y)}) rotate(${f1(ang)})`;
	let s = `<g transform="${g}"><rect x="${f1(-w / 2 + 3)}" y="${f1(-h / 2 + 3)}" width="${f1(w)}" height="${f1(h)}" fill="#0c0806" opacity="0.45"/>`
		+ `<rect x="${f1(-w / 2)}" y="${f1(-h / 2)}" width="${f1(w)}" height="${f1(h)}" fill="${col}" stroke="#24150f" stroke-width="1.2"/>`
		+ `<rect x="${f1(-w / 2)}" y="${f1(-h / 2)}" width="${f1(w)}" height="${f1(h / 2)}" fill="#fff" opacity="0.10"/>`
		+ `<line x1="${f1(-w / 2 + 1.5)}" y1="0" x2="${f1(w / 2 - 1.5)}" y2="0" stroke="#24150f" stroke-width="1" opacity="0.7"/>`;
	if (o.big) s += `<rect x="${f1(-w / 2 + 3)}" y="${f1(-h / 2 + 3)}" width="${f1(w - 6)}" height="${f1(h - 6)}" fill="none" stroke="${o.trim || C.gold}" stroke-width="1.2" opacity="0.8"/>`;
	return s + '</g>';
}
// Buildings along a street: rows on both sides, with warm windows/lamps that glow.
function town(street, o, seed, out) {
	const R = rng(seed), sp = spline(street, false, 6);
	let s = '', lamps = '';
	for (const side of o.sides || [1, -1]) {
		for (const { p, ang } of walk(sp, o.step || 22, o.start || 6)) {
			if (R() < (o.gaps || 0.12)) continue;
			const off = (o.off || 20) + R() * 4, nx = -Math.sin(ang) * side, ny = Math.cos(ang) * side;
			const w = (o.w || 18) * (0.8 + R() * 0.5), h = (o.h || 15) * (0.8 + R() * 0.45);
			const bx = p[0] + nx * (off + h / 2 - 6), by = p[1] + ny * (off + h / 2 - 6);
			if (o.skip && o.skip([bx, by])) continue;
			s += roof(bx, by, w, h, ang * 180 / Math.PI + (R() - 0.5) * 6, o.cols ? o.cols[Math.floor(R() * o.cols.length)] : C.roofs[Math.floor(R() * C.roofs.length)]);
			if (out) out.push([bx, by]);
			if (R() < 0.55) lamps += lamp(p[0] + nx * (off - 8), p[1] + ny * (off - 8), 0.8);
		}
	}
	return { s, lamps };
}
function lamp(x, y, k = 1) {
	return `<circle cx="${f1(x)}" cy="${f1(y)}" r="${f1(9 * k)}" fill="url(#lampGlow)"/><circle cx="${f1(x)}" cy="${f1(y)}" r="${f1(1.8 * k)}" fill="#fff3c4"/>`;
}
// A pier into the water from a, toward b; with mooring posts and lamps.
function pier(a, b, w = 10, o = {}) {
	const ang = Math.atan2(b[1] - a[1], b[0] - a[0]) * 180 / Math.PI, L = dist(a, b);
	let s = `<g transform="translate(${f1(a[0])} ${f1(a[1])}) rotate(${f1(ang)})"><rect x="3" y="${f1(-w / 2 + 3)}" width="${f1(L)}" height="${f1(w)}" fill="#0b0705" opacity="0.4"/>`
		+ `<rect x="0" y="${f1(-w / 2)}" width="${f1(L)}" height="${f1(w)}" fill="${o.col || '#8a6440'}" stroke="#3a2618" stroke-width="1.2"/>`;
	for (let x = 4; x < L; x += 5) s += `<line x1="${x}" y1="${f1(-w / 2)}" x2="${x}" y2="${f1(w / 2)}" stroke="#3a2618" stroke-width="0.6" opacity="0.6"/>`;
	if (o.head) s += `<rect x="${f1(L - o.head)}" y="${f1(-w / 2 - o.head / 3)}" width="${o.head}" height="${f1(w + o.head * 2 / 3)}" fill="${o.col || '#8a6440'}" stroke="#3a2618" stroke-width="1.2"/>`;
	s += '</g>';
	if (o.lamps !== false) for (let t = 0.3; t <= 1; t += 0.35) { const p = lerp(a, b, t); s += lamp(p[0], p[1], 0.7); }
	return s;
}
function crane(x, y, ang = 0, s = 1) {
	return `<g transform="translate(${f1(x)} ${f1(y)}) rotate(${ang}) scale(${s})"><rect x="-6" y="-6" width="12" height="12" fill="#3b3330" stroke="#151010" stroke-width="1"/><line x1="0" y1="0" x2="34" y2="0" stroke="#c68a2e" stroke-width="3.2"/><line x1="0" y1="0" x2="-10" y2="0" stroke="#c68a2e" stroke-width="3.2"/><rect x="-14" y="-4" width="5" height="8" fill="#2a2321"/><line x1="30" y1="0" x2="30" y2="7" stroke="#d8d0c0" stroke-width="1"/></g>`;
}
function coalHeap(x, y, r) {
	return `<ellipse cx="${f1(x + 3)}" cy="${f1(y + 3)}" rx="${f1(r * 1.25)}" ry="${f1(r)}" fill="#000" opacity="0.4"/><ellipse cx="${f1(x)}" cy="${f1(y)}" rx="${f1(r * 1.25)}" ry="${f1(r)}" fill="#1b1a1c" stroke="#3c3a3e" stroke-width="1"/><ellipse cx="${f1(x - r * 0.35)}" cy="${f1(y - r * 0.3)}" rx="${f1(r * 0.5)}" ry="${f1(r * 0.35)}" fill="#4a4850" opacity="0.7"/>`;
}
// A broken wreck hull lying on a beach or rocks.
function wreck(x, y, L, ang, o = {}) {
	const w = L * 0.3;
	return `<g transform="translate(${f1(x)} ${f1(y)}) rotate(${ang})"><path d="M${-L / 2},${-w / 2} L${L * 0.25},${-w / 2} Q${L / 2},0 ${L * 0.25},${w / 2} L${-L / 2},${w / 2} Z" fill="#000" opacity="0.35" transform="translate(3 3)"/>`
		+ `<path d="M${-L / 2},${-w / 2} L${L * 0.25},${-w / 2} Q${L / 2},0 ${L * 0.25},${w / 2} L${-L / 2},${w / 2} Z" fill="#4a3426" stroke="#1d130c" stroke-width="1.4"/>`
		+ `<path d="M${-L / 2 + 3},${-w / 2 + 3} L${-L / 2 + L * 0.35},${w / 2 - 2}" stroke="#1d130c" stroke-width="2"/>`
		+ [0.1, 0.3, 0.5].map(t => `<line x1="${f1(-L / 2 + L * t)}" y1="${f1(-w / 2)}" x2="${f1(-L / 2 + L * t)}" y2="${f1(w / 2)}" stroke="#8a6a4a" stroke-width="1.6" opacity="0.8"/>`).join('')
		+ `<rect x="${f1(-L / 2 - 6)}" y="${f1(-w / 2 + 2)}" width="8" height="${f1(w - 4)}" fill="#2b1d14" transform="rotate(14 ${-L / 2} 0)"/></g>`;
}
function cave(x, y, ang = 0) {
	return `<g transform="translate(${f1(x)} ${f1(y)}) rotate(${ang})"><path d="M-11,6 Q-11,-10 0,-11 Q11,-10 11,6 Z" fill="#0a0706" stroke="#c9b08c" stroke-width="1.4"/><path d="M-6,6 Q-6,-4 0,-5 Q6,-4 6,6" fill="none" stroke="#3a2d22" stroke-width="1.4"/></g>`;
}
// Lighthouse with its beam swept over the sea (ang = beam direction in degrees).
function lighthouse(x, y, ang, len = 420) {
	return `<g transform="translate(${f1(x)} ${f1(y)})"><path d="M0,0 L${f1(len * Math.cos((ang - 9) * Math.PI / 180))},${f1(len * Math.sin((ang - 9) * Math.PI / 180))} L${f1(len * Math.cos((ang + 9) * Math.PI / 180))},${f1(len * Math.sin((ang + 9) * Math.PI / 180))} Z" fill="url(#beam)" opacity="0.75"/>`
		+ `<circle r="26" fill="url(#lampGlow)" opacity="0.9"/><circle r="11" fill="#f2ecdf" stroke="#2a1f1a" stroke-width="2"/><circle r="11" fill="none" stroke="#a0362b" stroke-width="3" stroke-dasharray="6 5"/><circle r="4.5" fill="#ffd27a"/></g>`;
}
// Dry dock: a stone basin cut into the shore with a hull inside on blocks.
function dryDock(x, y, L, ang) {
	const w = L * 0.26;
	return `<g transform="translate(${f1(x)} ${f1(y)}) rotate(${ang})"><rect x="${-L / 2 - 6}" y="${-w / 2 - 6}" width="${L + 12}" height="${w + 12}" fill="#8c8578" stroke="#2a2622" stroke-width="1.5"/><rect x="${-L / 2}" y="${-w / 2}" width="${L}" height="${w}" fill="#2a3a40"/>`
		+ `<path d="M${-L / 2 + 6},${-w / 2 + 5} L${L * 0.3},${-w / 2 + 5} Q${L / 2 - 4},0 ${L * 0.3},${w / 2 - 5} L${-L / 2 + 6},${w / 2 - 5} Z" fill="#6b3a2a" stroke="#1d120c" stroke-width="1.2"/>`
		+ `<path d="M${-L / 2 + 10},${-w / 2 + 9} L${L * 0.26},${-w / 2 + 9} Q${L / 2 - 12},0 ${L * 0.26},${w / 2 - 9} L${-L / 2 + 10},${w / 2 - 9} Z" fill="#9b8a6c" opacity="0.8"/>`
		+ `<rect x="${L / 2 - 2}" y="${-w / 2 - 6}" width="5" height="${w + 12}" fill="#4d4842"/></g>`;
}
function warehouse(x, y, w, h, ang) { return roof(x, y, w, h, ang, '#5d5650', {}) + `<g transform="translate(${f1(x)} ${f1(y)}) rotate(${ang})">${[-0.25, 0, 0.25].map(t => `<line x1="${f1(w * t)}" y1="${f1(-h / 2)}" x2="${f1(w * t)}" y2="${f1(h / 2)}" stroke="#2a2420" stroke-width="1"/>`).join('')}</g>`; }
function bridge(a, b, w = 9) { return pier(a, b, w, { col: '#7a5636', lamps: false }) + `<path d="${d([offsetLine([a, b], w / 2 + 1)[0], offsetLine([a, b], w / 2 + 1)[1]])}" stroke="#2a1a10" stroke-width="1.5"/><path d="${d([offsetLine([a, b], -w / 2 - 1)[0], offsetLine([a, b], -w / 2 - 1)[1]])}" stroke="#2a1a10" stroke-width="1.5"/>`; }
function stilts(x, y, w, h, ang, seed) {
	const R = rng(seed);
	let s = `<g transform="translate(${f1(x)} ${f1(y)}) rotate(${ang})">`;
	for (let i = 0; i < 6; i++) s += `<circle cx="${f1(-w / 2 + R() * w)}" cy="${f1(h / 2 + 2 + R() * 3)}" r="1.4" fill="#2a1a10"/>`;
	return s + `<rect x="${-w / 2 - 3}" y="${-h / 2 - 3}" width="${w + 6}" height="${h + 6}" fill="#6a4a2e" stroke="#2a1a10" stroke-width="1"/></g>` + roof(x, y, w, h, ang, C.roofs[Math.floor(R() * C.roofs.length)]);
}

// ---------- the liner (top view, bow toward +x before rotation) ----------
function hullPath(L, Wd) {
	return `M${-L / 2 + Wd * 0.45},${-Wd / 2} L${L / 2 - L * 0.2},${-Wd / 2} C${L / 2 - L * 0.06},${-Wd * 0.45} ${L / 2 - L * 0.01},${-Wd * 0.18} ${L / 2},0 C${L / 2 - L * 0.01},${Wd * 0.18} ${L / 2 - L * 0.06},${Wd * 0.45} ${L / 2 - L * 0.2},${Wd / 2} L${-L / 2 + Wd * 0.45},${Wd / 2} C${-L / 2 + Wd * 0.05},${Wd / 2} ${-L / 2},${Wd * 0.3} ${-L / 2},0 C${-L / 2},${-Wd * 0.3} ${-L / 2 + Wd * 0.05},${-Wd / 2} ${-L / 2 + Wd * 0.45},${-Wd / 2} Z`;
}
function liner(cx, cy, L, ang, seed = 7) {
	const Wd = L * 0.115, R = rng(seed);
	const hull = hullPath(L, Wd), deck = hullPath(L * 0.94, Wd * 0.8);
	let s = `<g transform="translate(${f1(cx)} ${f1(cy)}) rotate(${ang})">`;
	// warm light spilling on the sea, then reflections
	s += `<ellipse cx="0" cy="0" rx="${f1(L * 0.62)}" ry="${f1(Wd * 2.4)}" fill="url(#shipGlow)"/>`;
	for (let i = 0; i < 18; i++) { const x = -L * 0.4 + R() * L * 0.75, side = R() < 0.5 ? -1 : 1, y = side * (Wd / 2 + 5 + R() * Wd * 0.5); s += `<line x1="${f1(x)}" y1="${f1(y)}" x2="${f1(x)}" y2="${f1(y + side * (4 + R() * 8))}" stroke="#ffd27a" stroke-width="2.4" stroke-opacity="${f1(0.2 + R() * 0.35)}" stroke-linecap="round"/>`; }
	s += `<path d="${hull}" fill="#000" opacity="0.45" transform="translate(5 6)"/>`;
	s += `<path d="${hull}" fill="#24100f" stroke="${C.gold}" stroke-width="2.6"/>`;
	s += `<path d="${deck}" fill="#a3794c" stroke="#5d3e22" stroke-width="1"/>`;
	for (let y = -Wd * 0.36; y <= Wd * 0.36; y += Wd * 0.09) s += `<line x1="${f1(-L * 0.44)}" y1="${f1(y)}" x2="${f1(L * 0.36)}" y2="${f1(y)}" stroke="#7b5634" stroke-width="0.6" opacity="0.6"/>`;
	// superstructure: long cream deckhouse with gilt edge, peeling patches
	const sx0 = -L * 0.33, sx1 = L * 0.27, sw = Wd * 0.62;
	s += `<rect x="${f1(sx0 + 3)}" y="${f1(-sw / 2 + 4)}" width="${f1(sx1 - sx0)}" height="${f1(sw)}" fill="#000" opacity="0.35"/>`;
	s += `<rect x="${f1(sx0)}" y="${f1(-sw / 2)}" width="${f1(sx1 - sx0)}" height="${f1(sw)}" rx="3" fill="#e6d8b8" stroke="${C.goldDeep}" stroke-width="1.6"/>`;
	for (let i = 0; i < 7; i++) s += `<ellipse cx="${f1(sx0 + R() * (sx1 - sx0))}" cy="${f1((R() - 0.5) * sw * 0.8)}" rx="${f1(3 + R() * 6)}" ry="${f1(1.5 + R() * 3)}" fill="#a48c66" opacity="0.6"/>`;
	s += `<rect x="${f1(sx0 + 6)}" y="${f1(-sw * 0.28)}" width="${f1(sx1 - sx0 - 12)}" height="${f1(sw * 0.56)}" fill="#d7c59c" opacity="0.7"/>`;
	// glass dome over the grand staircase (aft of the funnels)
	s += `<ellipse cx="${f1(-L * 0.25)}" cy="0" rx="${f1(Wd * 0.2)}" ry="${f1(Wd * 0.17)}" fill="#ffe3a0" stroke="${C.gold}" stroke-width="1.4"/><ellipse cx="${f1(-L * 0.25)}" cy="0" rx="${f1(Wd * 0.2)}" ry="${f1(Wd * 0.17)}" fill="url(#lampGlow)"/>`;
	// bridge forward
	s += `<rect x="${f1(sx1 - Wd * 0.18)}" y="${f1(-Wd * 0.42)}" width="${f1(Wd * 0.22)}" height="${f1(Wd * 0.84)}" fill="#f0e6cf" stroke="${C.goldDeep}" stroke-width="1.4"/><line x1="${f1(sx1 + Wd * 0.02)}" y1="${f1(-Wd * 0.38)}" x2="${f1(sx1 + Wd * 0.02)}" y2="${f1(Wd * 0.38)}" stroke="#ffd27a" stroke-width="2"/>`;
	// lifeboats both sides
	for (let x = sx0 + 10; x < sx1 - Wd * 0.3; x += L * 0.052) for (const sd of [-1, 1]) s += `<rect x="${f1(x)}" y="${f1(sd * Wd * 0.36 - Wd * 0.05)}" width="${f1(L * 0.036)}" height="${f1(Wd * 0.1)}" rx="${f1(Wd * 0.05)}" fill="#f3efe6" stroke="#5a4a3a" stroke-width="0.8"/>`;
	// four funnels, buff-gold with black tops, raked aft; shadows east
	for (const t of [-0.17, -0.05, 0.07, 0.19]) {
		const x = L * t, rx = Wd * 0.2, ry = Wd * 0.15;
		s += `<ellipse cx="${f1(x - 8)}" cy="${f1(6)}" rx="${f1(rx)}" ry="${f1(ry)}" fill="#000" opacity="0.4"/>`;
		s += `<ellipse cx="${f1(x)}" cy="0" rx="${f1(rx)}" ry="${f1(ry)}" fill="#d4952e" stroke="#5a3a10" stroke-width="1.4"/><ellipse cx="${f1(x - rx * 0.12)}" cy="0" rx="${f1(rx * 0.68)}" ry="${f1(ry * 0.66)}" fill="#141010"/><ellipse cx="${f1(x - rx * 0.12)}" cy="0" rx="${f1(rx * 0.68)}" ry="${f1(ry * 0.66)}" fill="none" stroke="${C.gold}" stroke-width="1"/>`;
	}
	// smoke drifting east (light trail)
	s += `<path d="M${f1(-L * 0.17)},0 C${f1(-L * 0.4)},${f1(-Wd)} ${f1(-L * 0.6)},${f1(-Wd * 0.5)} ${f1(-L * 0.8)},${f1(-Wd * 1.4)}" fill="none" stroke="#cbbfae" stroke-width="${f1(Wd * 0.35)}" stroke-opacity="0.14" stroke-linecap="round" filter="url(#blur6)"/>`;
	// masts and the light strings between them
	for (const t of [-0.4, 0.37]) s += `<circle cx="${f1(L * t)}" cy="0" r="3" fill="#2a1a10"/>`;
	s += `<path d="M${f1(-L * 0.4)},0 L${f1(L * 0.37)},0" stroke="#3a2a1c" stroke-width="0.8"/>`;
	for (let x = -L * 0.4; x < L * 0.37; x += L * 0.035) s += `<circle cx="${f1(x)}" cy="0" r="1.4" fill="#fff0bf"/>`;
	// deck-edge lamps, the gilded rails
	for (const sd of [-1, 1]) for (let x = -L * 0.42; x < L * 0.36; x += L * 0.03) s += `<circle cx="${f1(x)}" cy="${f1(sd * Wd * 0.43)}" r="3.4" fill="url(#lampGlow)"/><circle cx="${f1(x)}" cy="${f1(sd * Wd * 0.43)}" r="1" fill="#fff3c4"/>`;
	s += `<path d="${deck}" fill="none" stroke="${C.gold}" stroke-width="1" opacity="0.8"/>`;
	return s + '</g>';
}
// A small boat at anchor: schooner, fishing boat or coal lighter (kind 0/1/2).
function boat(x, y, ang, len = 30, kind = 0) {
	const w = len * 0.32, hull = kind === 2 ? '#1c1a1c' : kind === 1 ? '#5a3a26' : '#3b2a22', deck = kind === 2 ? '#333' : '#a3794c';
	const shape = (l, ww, i) => `M${f1(-l / 2 + i)},${f1(-ww / 2 + i)} L${f1(l * 0.25 - i)},${f1(-ww / 2 + i)} Q${f1(l / 2 - i * 1.5)},0 ${f1(l * 0.25 - i)},${f1(ww / 2 - i)} L${f1(-l / 2 + i)},${f1(ww / 2 - i)} Z`;
	let s = `<g transform="translate(${f1(x)} ${f1(y)}) rotate(${ang})"><path d="${shape(len, w, 0)}" fill="#000" opacity="0.35" transform="translate(2 2)"/><path d="${shape(len, w, 0)}" fill="${hull}" stroke="#140c08" stroke-width="1"/><path d="${shape(len, w, 2)}" fill="${deck}"/>`;
	if (kind === 2) s += `<ellipse cx="0" cy="0" rx="${f1(len * 0.3)}" ry="${f1(w * 0.3)}" fill="#0c0b0c"/>`;
	else s += `<line x1="${f1(-len * 0.3)}" y1="0" x2="${f1(len * 0.35)}" y2="0" stroke="#e8dcc0" stroke-width="1.4" opacity="0.8"/><circle cx="${f1(len * 0.05)}" cy="0" r="1.8" fill="#2a1a10"/>`;
	return s + '</g>' + lamp(x, y, 0.55);
}
function buoy(x, y) { return `<circle cx="${f1(x)}" cy="${f1(y)}" r="4" fill="#a0362b" stroke="#f2ecdf" stroke-width="1.4"/>`; }
// The launch: a small steam launch with a canopy.
function launchBoat(x, y, ang) {
	return `<g transform="translate(${f1(x)} ${f1(y)}) rotate(${ang})"><path d="M-14,-5 L8,-5 Q16,0 8,5 L-14,5 Z" fill="#3a1a14" stroke="${C.gold}" stroke-width="1.4"/><rect x="-10" y="-3" width="12" height="6" fill="${C.velvet}" stroke="${C.gold}" stroke-width="0.8"/><circle cx="5" cy="0" r="1.8" fill="#ffd27a"/></g>`;
}

// ---------- markers ----------
function step(x, y, n) {
	return `<g transform="translate(${f1(x)} ${f1(y)})"><circle r="21" fill="${C.accent}" opacity="0.25"/><circle r="16" fill="${C.accent}" stroke="#1a0f00" stroke-width="2.5"/><circle r="12.5" fill="none" stroke="#fff2d6" stroke-width="1.2" opacity="0.8"/><text y="6.5" font-size="18" font-weight="700" fill="#1a0f00" text-anchor="middle">${n}</text></g>`;
}
function giver(x, y, kind = 'main') {
	const col = kind === 'main' ? C.accent : C.line;
	return `<g transform="translate(${f1(x)} ${f1(y)})"><rect x="-10" y="-10" width="20" height="20" rx="3" fill="${col}" stroke="#08121a" stroke-width="2" transform="rotate(45)"/><text y="5.5" font-size="15" font-weight="700" fill="#08121a" text-anchor="middle">!</text></g>`;
}
function bounty(x, y) { return `<g transform="translate(${f1(x)} ${f1(y)})"><rect x="-10" y="-9" width="20" height="16" fill="#3a2a1a" stroke="${C.line}" stroke-width="2"/><line x1="-6" y1="7" x2="-6" y2="12" stroke="${C.line}" stroke-width="2"/><line x1="6" y1="7" x2="6" y2="12" stroke="${C.line}" stroke-width="2"/><text y="3.5" font-size="11" font-weight="700" fill="${C.line}" text-anchor="middle">$</text></g>`; }
function chest(x, y) { return `<g transform="translate(${f1(x)} ${f1(y)})"><circle r="13" fill="url(#lampGlow)" opacity="0.7"/><rect x="-8" y="-5" width="16" height="11" rx="1.5" fill="#6a3d1c" stroke="${C.chest}" stroke-width="1.8"/><line x1="-8" y1="-0.5" x2="8" y2="-0.5" stroke="${C.chest}" stroke-width="1.4"/><rect x="-1.6" y="-2" width="3.2" height="3.4" fill="${C.chest}"/></g>`; }
function secret(x, y) { return `<g transform="translate(${f1(x)} ${f1(y)})"><rect x="-8" y="-8" width="16" height="16" fill="#0b2232" stroke="${C.secret}" stroke-width="1.8" transform="rotate(45)"/><text y="5" font-size="13" font-weight="700" fill="${C.secret}" text-anchor="middle">?</text></g>`; }
function slots(x, y) { return `<g transform="translate(${f1(x)} ${f1(y)})"><rect x="-9" y="-10" width="18" height="20" rx="3" fill="#3a0d22" stroke="${C.slot}" stroke-width="1.8"/><rect x="-6" y="-5" width="12" height="8" fill="#ffe7f3"/><text y="2.5" font-size="8.5" font-weight="700" fill="#b0134f" text-anchor="middle">777</text><line x1="11" y1="-6" x2="11" y2="2" stroke="${C.slot}" stroke-width="1.6"/><circle cx="11" cy="-7" r="1.8" fill="${C.slot}"/></g>`; }
function station(x, y) { return `<g transform="translate(${f1(x)} ${f1(y)})"><rect x="-13" y="-11" width="26" height="22" rx="3" fill="#0e2c42" stroke="${C.line}" stroke-width="2"/><rect x="-7" y="-6" width="14" height="9" rx="2" fill="none" stroke="${C.line}" stroke-width="1.6"/><circle cx="-4" cy="6" r="1.8" fill="${C.line}"/><circle cx="4" cy="6" r="1.8" fill="${C.line}"/></g>`; }
function anchorIcon(x, y) { return `<g transform="translate(${f1(x)} ${f1(y)})" stroke="${C.gold}" stroke-width="2" fill="none"><circle cx="0" cy="-7" r="2.5"/><line x1="0" y1="-4.5" x2="0" y2="8"/><path d="M-7,3 Q-6,9 0,9 Q6,9 7,3"/><line x1="-4" y1="-1" x2="4" y2="-1"/></g>`; }
// Enemy area: soft red wash inside a dashed ring, with a chip naming who lives there.
function zone(x, y, rx, ry, kind, o = {}) {
	const col = kind === 'crew' ? C.crew : C.beast;
	return `<ellipse cx="${f1(x)}" cy="${f1(y)}" rx="${rx}" ry="${ry}" fill="${col}" fill-opacity="0.13" stroke="${col}" stroke-width="2" stroke-dasharray="7 5" stroke-opacity="0.85" transform="rotate(${o.rot || 0} ${f1(x)} ${f1(y)})"/>`;
}
// The glyph for each enemy kind (drawn, so no font is needed).
function foeIcon(x, y, kind) {
	const g = {
		crew: `<path d="M0,-7 C4,-3 8,0 4,4 C2,6 1,4 0.8,3.2 L2,8 L-2,8 L-0.8,3.2 C-1,4 -2,6 -4,4 C-8,0 -4,-3 0,-7 Z" fill="#1a0806"/>`,
		crab: `<ellipse cx="0" cy="1" rx="5" ry="3.6" fill="#1a0806"/><path d="M-5,-1 Q-9,-6 -6,-8 M5,-1 Q9,-6 6,-8 M-4,4 L-8,7 M4,4 L8,7 M-5,2 L-9,3 M5,2 L9,3" stroke="#1a0806" stroke-width="1.6" fill="none" stroke-linecap="round"/>`,
		eel: `<path d="M-8,4 C-5,-4 -1,6 2,-1 C4,-6 7,-3 8,-5" stroke="#1a0806" stroke-width="2.4" fill="none" stroke-linecap="round"/><circle cx="7.6" cy="-5" r="1.6" fill="#1a0806"/>`,
		gull: `<path d="M-9,1 Q-5,-6 0,1 Q5,-6 9,1" stroke="#1a0806" stroke-width="2.4" fill="none" stroke-linecap="round"/>`,
	}[kind];
	const col = kind === 'crew' ? C.crew : C.beast;
	return `<g transform="translate(${f1(x)} ${f1(y)})"><circle r="12" fill="${col}" stroke="#1a0806" stroke-width="2"/>${g}</g>`;
}
// A small label chip (dark glass) for names of spots.
function chip(x, y, s, o = {}) {
	const size = o.size || 14, w = s.length * size * 0.56 + 14, h = size + 9, col = o.col || C.text;
	const ax = o.anchor === 'start' ? x : o.anchor === 'end' ? x - w : x - w / 2;
	return `<rect x="${f1(ax)}" y="${f1(y - h / 2)}" width="${f1(w)}" height="${h}" rx="3" fill="#07131c" fill-opacity="0.82" stroke="${o.stroke || col}" stroke-opacity="0.6" stroke-width="1"/>` + text(ax + w / 2, y + size * 0.36, s, { size, fill: col, weight: o.weight || 600, halo: false });
}
// Player's path: glowing orange dashes with arrowheads.
function route(pts, o = {}) {
	const dd = smooth(pts, false, 8);
	return `<path d="${dd}" fill="none" stroke="#000" stroke-width="9" stroke-opacity="0.35" stroke-linecap="round"/><path d="${dd}" fill="none" stroke="${o.col || C.accent}" stroke-width="${o.w || 5}" stroke-dasharray="${o.dash || '14 9'}" stroke-linecap="round" marker-mid="url(#arrow)"/>`;
}
function seaRoute(pts) {
	const dd = smooth(pts, false, 8);
	return `<path d="${dd}" fill="none" stroke="${C.gold}" stroke-width="10" stroke-opacity="0.18" stroke-linecap="round"/><path d="${dd}" fill="none" stroke="${C.gold}" stroke-width="3" stroke-dasharray="3 7" stroke-linecap="round"/>`;
}

// ---------- defs ----------
function defs() {
	const font = f => fs.readFileSync(path.join(__dirname, '../../../../Content/UI/Fonts', f)).toString('base64');
	return `<defs><style>
@font-face{font-family:'Chakra Petch';font-weight:400;src:url(data:font/ttf;base64,${font('ChakraPetch-Regular.ttf')}) format('truetype');}
@font-face{font-family:'Chakra Petch';font-weight:600 700;src:url(data:font/ttf;base64,${font('ChakraPetch-Bold.ttf')}) format('truetype');}
text{font-family:'Chakra Petch','Bahnschrift','Segoe UI',sans-serif;}
</style>
<linearGradient id="seaGrad" x1="0" y1="0" x2="1" y2="0.3"><stop offset="0" stop-color="#0a1b2a"/><stop offset="0.6" stop-color="#0f2638"/><stop offset="1" stop-color="#132c3e"/></linearGradient>
<radialGradient id="sunGlow" cx="-0.05" cy="0.45" r="0.9"><stop offset="0" stop-color="#ff8a3d" stop-opacity="0.55"/><stop offset="0.35" stop-color="#d3602e" stop-opacity="0.22"/><stop offset="1" stop-color="#d3602e" stop-opacity="0"/></radialGradient>
<radialGradient id="lampGlow"><stop offset="0" stop-color="#ffd27a" stop-opacity="0.95"/><stop offset="0.35" stop-color="#ffb347" stop-opacity="0.45"/><stop offset="1" stop-color="#ff9f1c" stop-opacity="0"/></radialGradient>
<radialGradient id="shipGlow"><stop offset="0" stop-color="#ffc35a" stop-opacity="0.5"/><stop offset="0.5" stop-color="#ff9f1c" stop-opacity="0.16"/><stop offset="1" stop-color="#ff9f1c" stop-opacity="0"/></radialGradient>
<linearGradient id="beam" x1="0" y1="0" x2="1" y2="0"><stop offset="0" stop-color="#fff1c2" stop-opacity="0.75"/><stop offset="1" stop-color="#fff1c2" stop-opacity="0"/></linearGradient>
<radialGradient id="vignette" cx="0.5" cy="0.5" r="0.75"><stop offset="0.6" stop-color="#000" stop-opacity="0"/><stop offset="1" stop-color="#000" stop-opacity="0.55"/></radialGradient>
<linearGradient id="panelGrad" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#0e2c42" stop-opacity="0.95"/><stop offset="1" stop-color="#071a28" stop-opacity="0.97"/></linearGradient>
<pattern id="hatch" width="9" height="9" patternUnits="userSpaceOnUse" patternTransform="rotate(40)"><line x1="0" y1="0" x2="0" y2="9" stroke="#2a1f18" stroke-width="3"/></pattern>
<pattern id="planks" width="6" height="6" patternUnits="userSpaceOnUse"><rect width="6" height="6" fill="#8d6a45"/><line x1="0" y1="0" x2="6" y2="0" stroke="#6d4e30" stroke-width="1"/></pattern>
<filter id="blur3"><feGaussianBlur stdDeviation="3"/></filter>
<filter id="blur6"><feGaussianBlur stdDeviation="6"/></filter>
<filter id="blur10"><feGaussianBlur stdDeviation="10"/></filter>
<filter id="blur18"><feGaussianBlur stdDeviation="18"/></filter>
<filter id="soft"><feGaussianBlur stdDeviation="1.6"/></filter>
<filter id="seaNoise" x="0" y="0" width="1" height="1"><feTurbulence type="fractalNoise" baseFrequency="0.006 0.02" numOctaves="3" seed="4"/><feColorMatrix values="0 0 0 0 0.16  0 0 0 0 0.34  0 0 0 0 0.42  0 0 0 0.9 -0.35"/></filter>
<filter id="mottleDark" x="0" y="0" width="1" height="1"><feTurbulence type="fractalNoise" baseFrequency="0.012" numOctaves="4" seed="9"/><feColorMatrix values="0 0 0 0 0.22  0 0 0 0 0.2  0 0 0 0 0.1  0 0 0 1.6 -0.75"/></filter>
<filter id="mottleLight" x="0" y="0" width="1" height="1"><feTurbulence type="fractalNoise" baseFrequency="0.03" numOctaves="3" seed="2"/><feColorMatrix values="0 0 0 0 0.95  0 0 0 0 0.82  0 0 0 0 0.55  0 0 0 1.4 -0.78"/></filter>
<filter id="paper" x="0" y="0" width="1" height="1"><feTurbulence type="fractalNoise" baseFrequency="0.9" numOctaves="2" seed="5"/><feColorMatrix values="0 0 0 0 0.5  0 0 0 0 0.45  0 0 0 0 0.4  0 0 0 0.5 0"/></filter>
<marker id="arrow" viewBox="0 0 10 10" refX="5" refY="5" markerWidth="0" markerHeight="0"/>
</defs>`;
}

module.exports = { W, H, MAP, PANEL, C, rng, f1, dist, lerp, pip, inAny, segDist, lineDist, spline, roughen, d, smooth, offsetLine, walk, playArea,
	esc, text, district, waterLayer, waves, landLayer, ridgeLayer, hill, cliff, flats, trees, rocks, road, trail, rail, roof, town, lamp, pier,
	crane, coalHeap, wreck, cave, lighthouse, dryDock, warehouse, bridge, stilts, hullPath, liner, launchBoat, boat, buoy, step, giver, bounty, chest,
	secret, slots, station, anchorIcon, zone, foeIcon, chip, route, seaRoute, defs };
