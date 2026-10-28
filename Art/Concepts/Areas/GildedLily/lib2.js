// Page kit for the second round of Gilded Lily concepts (D-G, gen2.js): a cleaner map, an eye-level arrival sketch,
// sized fight rings, and the liner's played decks at true scale across the full page width.
const L = require('./lib');
const { C, f1, text, chip } = L;

const W2 = 2048, H2 = 2250;
const MAP2 = { x: 28, y: 112, w: 1300, h: 1020 };
const COL = { x: 1350, y: 112, w: 670 };
const SHIP = { x: 28, y: 1156, w: 1992, h: 1072 };
// lib's painters size their sea and sampling grid from its MAP: point it at this round's map window.
Object.assign(L.MAP, MAP2);

// ---------- map marks ----------
const FIGHT = { crew: C.crew, beast: '#ffb347', bounty: C.line };
// A big fight: a ring at its real size (diameter in metres) with a short tag.
function fight(x, y, diam, kind, tag, mpp, o = {}) {
	const r = diam / 2 / mpp, col = FIGHT[kind];
	let s = `<circle cx="${f1(x)}" cy="${f1(y)}" r="${f1(r)}" fill="${col}" fill-opacity="0.14"/><circle cx="${f1(x)}" cy="${f1(y)}" r="${f1(r)}" fill="none" stroke="#000" stroke-opacity="0.5" stroke-width="6"/><circle cx="${f1(x)}" cy="${f1(y)}" r="${f1(r)}" fill="none" stroke="${col}" stroke-width="3.5" stroke-dasharray="10 6"/>`;
	const ty = o.below ? y + r + 18 : y - r - 14;
	s += chip(x, ty, `${tag} · ${diam} m`, { size: 16, col, weight: 700 });
	return s;
}
function bigLabel(x, y, name, note, o = {}) {
	let s = text(x, y, name.toUpperCase(), { size: o.size || 30, weight: 700, fill: o.fill || '#f6e7c1', ls: 3, haloW: 9, rot: o.rot });
	if (note) s += text(x, y + (o.size || 30) * 0.8 + 6, note, { size: 17, weight: 400, fill: '#efe0bd', italic: true, haloW: 5, rot: o.rot });
	return s;
}
function scaleBar(x, y, mpp, metres = [0, 50, 100]) {
	const k = 1 / mpp, last = metres[metres.length - 1];
	let s = `<g transform="translate(${f1(x)} ${f1(y)})"><rect x="-12" y="-34" width="${f1(last * k + 60)}" height="52" rx="4" fill="#07131c" fill-opacity="0.8" stroke="${C.line}" stroke-opacity="0.5"/>`;
	for (let i = 1; i < metres.length; i++) s += `<rect x="${f1(metres[i - 1] * k)}" y="-7" width="${f1((metres[i] - metres[i - 1]) * k)}" height="10" fill="${i % 2 ? '#f6e7c1' : '#120d0a'}" stroke="${i % 2 ? '#120d0a' : '#f6e7c1'}"/>`;
	metres.forEach((m, i) => { s += text(m * k, -14, i === metres.length - 1 ? `${m} m` : String(m), { size: 15, halo: false, anchor: i === metres.length - 1 ? 'start' : 'middle' }); });
	return s + '</g>';
}
function compass(x, y) {
	return `<g transform="translate(${f1(x)} ${f1(y)})"><circle r="32" fill="#07131c" fill-opacity="0.8" stroke="${C.line}" stroke-opacity="0.6" stroke-width="1.5"/><path d="M0,-25 L8,0 L0,-5 L-8,0 Z" fill="${C.accent}"/><path d="M0,25 L8,0 L0,5 L-8,0 Z" fill="${C.dim}" opacity="0.6"/>` + text(0, -38, 'N', { size: 17, weight: 700, fill: C.accent }) + '</g>';
}

// ---------- the eye-level arrival sketch ----------
// Shared pieces in sketch coordinates (0..w, 0..h); each concept adds its own land between sky and platform.
const SK = { w: 670, h: 377 };
function skDefs() {
	return `<linearGradient id="skSky" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#1a1d3c"/><stop offset="0.45" stop-color="#5a3050"/><stop offset="0.78" stop-color="#c8573a"/><stop offset="1" stop-color="#f59a42"/></linearGradient>
<linearGradient id="skSea" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#e07a3a" stop-opacity="0.9"/><stop offset="0.25" stop-color="#4a3048"/><stop offset="1" stop-color="#121a2c"/></linearGradient>
<radialGradient id="skSun" cx="0.5" cy="0.5" r="0.5"><stop offset="0" stop-color="#fff1c2"/><stop offset="0.25" stop-color="#ffc46a" stop-opacity="0.9"/><stop offset="1" stop-color="#ff7a2a" stop-opacity="0"/></radialGradient>
<linearGradient id="skHaze" x1="0" y1="0" x2="0" y2="1"><stop offset="0" stop-color="#f08a50" stop-opacity="0"/><stop offset="1" stop-color="#f08a50" stop-opacity="0.35"/></linearGradient>`;
}
function skSky(w, h, horizon, sunX) {
	let s = `<rect width="${w}" height="${h}" fill="url(#skSky)"/>`;
	const R = L.rng(Math.round(sunX));
	for (let i = 0; i < 40; i++) s += `<circle cx="${f1(R() * w)}" cy="${f1(R() * horizon * 0.45)}" r="${f1(0.6 + R() * 0.9)}" fill="#fff" opacity="${f1(0.3 + R() * 0.5)}"/>`;
	for (let i = 0; i < 6; i++) { const y = horizon * (0.25 + R() * 0.6), x = R() * w; s += `<ellipse cx="${f1(x)}" cy="${f1(y)}" rx="${f1(60 + R() * 120)}" ry="${f1(3 + R() * 5)}" fill="#ffb27a" opacity="${f1(0.15 + R() * 0.2)}" filter="url(#blur3)"/>`; }
	s += `<circle cx="${sunX}" cy="${horizon - 4}" r="70" fill="url(#skSun)"/>`;
	s += `<rect y="${horizon}" width="${w}" height="${h - horizon}" fill="url(#skSea)"/>`;
	s += `<path d="M${sunX - 40},${horizon + 4} L${sunX + 40},${horizon + 4} L${sunX + 14},${h} L${sunX - 14},${h} Z" fill="#ffc46a" opacity="0.25" filter="url(#blur6)"/>`;
	for (let i = 0; i < 30; i++) { const y = horizon + 4 + R() * (h - horizon), x = R() * w; s += `<line x1="${f1(x)}" y1="${f1(y)}" x2="${f1(x + 10 + R() * 30)}" y2="${f1(y)}" stroke="#ffd9a0" stroke-opacity="${f1(0.08 + R() * 0.15)}" stroke-width="1.2"/>`; }
	return s;
}
// The liner side-on, far off: dark hull, gilt line, four raked funnels, rows of lit windows, a glow and its reflection.
function skLiner(x, y, len, o = {}) {
	const h = len * 0.1, R = L.rng(Math.round(x * 7 + len));
	let s = `<ellipse cx="${f1(x + len / 2)}" cy="${f1(y)}" rx="${f1(len * 0.75)}" ry="${f1(h * 2.2)}" fill="#ffc35a" opacity="0.3" filter="url(#blur10)"/>`;
	s += `<path d="M${f1(x)},${f1(y - h)} L${f1(x + len)},${f1(y - h * 1.15)} L${f1(x + len * 0.97)},${f1(y)} L${f1(x + len * 0.04)},${f1(y)} Z" fill="#160c0e"/>`;
	s += `<line x1="${f1(x + len * 0.02)}" y1="${f1(y - h * 0.55)}" x2="${f1(x + len * 0.99)}" y2="${f1(y - h * 0.62)}" stroke="${C.gold}" stroke-width="${f1(Math.max(1, len / 160))}"/>`;
	s += `<rect x="${f1(x + len * 0.14)}" y="${f1(y - h * 1.75)}" width="${f1(len * 0.66)}" height="${f1(h * 0.7)}" fill="#1e1416"/>`;
	for (const t of [0.26, 0.4, 0.54, 0.68]) s += `<path d="M${f1(x + len * t)},${f1(y - h * 1.7)} l${f1(len * 0.05)},0 l${f1(-len * 0.012)},${f1(-h * 1.6)} l${f1(-len * 0.05)},0 Z" fill="#c98a30"/><path d="M${f1(x + len * t - len * 0.012)},${f1(y - h * 3.3)} l${f1(len * 0.05)},0 l${f1(-len * 0.002)},${f1(-h * 0.3)} l${f1(-len * 0.05)},0 Z" fill="#111"/>`;
	for (const t of [0.08, 0.9]) s += `<line x1="${f1(x + len * t)}" y1="${f1(y - h)}" x2="${f1(x + len * t + len * 0.01)}" y2="${f1(y - h * 4.4)}" stroke="#160c0e" stroke-width="1.4"/>`;
	s += `<path d="M${f1(x + len * 0.09)},${f1(y - h * 4.3)} Q${f1(x + len * 0.5)},${f1(y - h * 3.2)} ${f1(x + len * 0.91)},${f1(y - h * 4.3)}" fill="none" stroke="#ffe3a0" stroke-width="0.8" stroke-dasharray="1.5 3"/>`;
	for (let row = 0; row < 3; row++) for (let i = 0; i < len / 4; i++) if (R() < 0.75) s += `<rect x="${f1(x + len * 0.06 + i * 4 + R())}" y="${f1(y - h * (0.35 + row * 0.42) - (row === 2 ? h * 0.75 : 0))}" width="1.8" height="1.4" fill="#ffd27a"/>`;
	s += `<rect x="${f1(x + len * 0.1)}" y="${f1(y + 2)}" width="${f1(len * 0.8)}" height="${f1(h * 1.6)}" fill="#ffd27a" opacity="0.18" filter="url(#blur3)"/>`;
	for (let i = 0; i < 14; i++) s += `<line x1="${f1(x + len * (0.1 + R() * 0.8))}" y1="${f1(y + 3 + R() * h * 2)}" x2="${f1(x + len * (0.1 + R() * 0.8) + 8)}" y2="${f1(y + 3 + R() * h * 2)}" stroke="#ffd27a" stroke-opacity="0.5" stroke-width="1"/>`;
	return s;
}
// A silhouette shape from points (land, hills), filled with a vertical gradient-ish colour and a soft top rim.
function skShape(pts, col, rim = '#e2774a', rimOp = 0.5) {
	const dd = L.smooth(pts, true, 6);
	return `<path d="${dd}" fill="${col}"/><path d="${L.smooth(pts.slice(0, -2), false, 6)}" fill="none" stroke="${rim}" stroke-width="1.6" stroke-opacity="${rimOp}"/>`;
}
// A row of town buildings in silhouette with lit windows, from x0 to x1 standing on y.
function skTown(x0, x1, y, hMin, hMax, seed, col = '#24161c', o = {}) {
	const R = L.rng(seed); let s = '', x = x0;
	while (x < x1) {
		const w = 14 + R() * 26, h = hMin + R() * (hMax - hMin);
		const roof = R();
		s += `<rect x="${f1(x)}" y="${f1(y - h)}" width="${f1(w)}" height="${f1(h + 2)}" fill="${col}"/>`;
		if (roof < 0.4) s += `<path d="M${f1(x - 1)},${f1(y - h)} L${f1(x + w / 2)},${f1(y - h - w * 0.4)} L${f1(x + w + 1)},${f1(y - h)} Z" fill="${col}"/>`;
		else if (roof < 0.55) s += `<rect x="${f1(x + w * 0.6)}" y="${f1(y - h - 8)}" width="3" height="8" fill="${col}"/>`;
		for (let wy = y - h + 5; wy < y - 4; wy += 7) for (let wx = x + 3; wx < x + w - 4; wx += 6) if (R() < (o.lit || 0.35)) s += `<rect x="${f1(wx)}" y="${f1(wy)}" width="2.4" height="3.2" fill="#ffcf73" opacity="${f1(0.6 + R() * 0.4)}"/>`;
		x += w + R() * 3;
	}
	return s;
}
function skLamp(x, y, h = 70) {
	return `<line x1="${x}" y1="${y}" x2="${x}" y2="${y - h}" stroke="#0d0a0c" stroke-width="4"/><path d="M${x - 9},${y - h} h18 l-3,-12 h-12 Z" fill="#0d0a0c"/><circle cx="${x}" cy="${y - h + 4}" r="26" fill="url(#lampGlow)"/><rect x="${x - 5}" y="${y - h - 9}" width="10" height="8" fill="#ffe7a8"/>`;
}
// Station platform foreground: canopy edge top-left, the platform's edge, a waiting train car on the right, lamps.
function skPlatform(w, h, o = {}) {
	const py = h - 46;
	let s = `<path d="M0,${py} L${w},${py - 8} L${w},${h} L0,${h} Z" fill="#1a1416"/><path d="M0,${py} L${w},${py - 8}" stroke="${C.gold}" stroke-width="2" opacity="0.8"/>`;
	s += `<path d="M0,${py + 10} L${w},${py + 2}" stroke="#3a2a22" stroke-width="2"/>`;
	for (let x = 20; x < w; x += 46) s += `<line x1="${x}" y1="${py + 4}" x2="${x - 6}" y2="${h}" stroke="#2a1f1c" stroke-width="1.4"/>`;
	s += `<path d="M0,0 L${o.canopy || 230},0 L${(o.canopy || 230) - 50},34 L0,48 Z" fill="#120d0e"/><path d="M0,48 L${(o.canopy || 230) - 50},34" stroke="${C.gold}" stroke-width="2"/>`;
	for (let x = 14; x < (o.canopy || 230) - 60; x += 30) s += `<path d="M${x},${46 - x * 0.08} q6,10 12,0" fill="none" stroke="#3a2a22" stroke-width="1.5"/>`;
	s += `<line x1="38" y1="44" x2="38" y2="${py}" stroke="#120d0e" stroke-width="7"/>`;
	s += `<g transform="translate(${w - 130} ${py - 92})"><rect width="160" height="88" rx="6" fill="#140c0e"/><rect y="6" width="160" height="4" fill="${C.gold}" opacity="0.7"/>${[14, 50, 86, 122].map(x => `<rect x="${x}" y="22" width="24" height="26" rx="2" fill="#ffc35a" opacity="0.85"/><rect x="${x}" y="22" width="24" height="26" rx="2" fill="url(#lampGlow)"/>`).join('')}<rect x="0" y="80" width="160" height="10" fill="#0a0708"/><circle cx="30" cy="92" r="9" fill="#0a0708"/><circle cx="120" cy="92" r="9" fill="#0a0708"/></g>`;
	s += skLamp(o.lampX || 300, py - 2, 76) + skLamp(o.lampX2 || 470, py - 5, 66);
	s += `<g transform="translate(${o.signX || 90} ${py - 118})"><line x1="14" y1="20" x2="14" y2="118" stroke="#120d0e" stroke-width="4"/><line x1="106" y1="20" x2="106" y2="118" stroke="#120d0e" stroke-width="4"/><rect x="0" y="0" width="120" height="26" rx="2" fill="#0e2c42" stroke="${C.gold}" stroke-width="1.5"/>${text(60, 18, o.sign || 'END OF THE LINE', { size: 11, weight: 700, fill: '#f6e7c1', halo: false, ls: 1.5 })}</g>`;
	return s;
}
function skFrame(x, y, inner, caption) {
	const { w, h } = SK;
	return `<clipPath id="skClip${x}${y}"><rect width="${w}" height="${h}" rx="6"/></clipPath><g transform="translate(${x} ${y})"><g clip-path="url(#skClip${x}${y})">${inner}<rect width="${w}" height="${h}" filter="url(#paper)" opacity="0.08"/><rect width="${w}" height="${h}" fill="url(#vignette)" opacity="0.7"/></g>`
		+ `<rect width="${w}" height="${h}" rx="6" fill="none" stroke="${C.line}" stroke-width="2" stroke-opacity="0.8"/>`
		+ `<rect x="12" y="${h - 40}" width="${caption.length * 9.4 + 24}" height="28" rx="3" fill="#07131c" fill-opacity="0.85" stroke="${C.accent}" stroke-width="1.2"/>` + text(24, h - 21, caption, { size: 15, weight: 700, fill: '#f6e7c1', anchor: 'start', halo: false, ls: 1 }) + '</g>';
}

// ---------- the liner's played decks at true scale ----------
// Where a deck point (deck 0-2, t along -0.5..0.5 stern to bow, v across -0.5..0.5) lands on the page.
function shipXY(cn, deck, t, v) {
	const P = { ...SHIP, y: SHIP.y + (cn.shift || 0) }, Lh = P.w - 120, kpm = Lh / 180, Wd = 24 * kpm, cx = P.x + P.w / 2 + 20;
	return [cx + t * Lh, P.y + 136 + Wd / 2 + deck * (Wd + 66) + v * Wd, kpm];
}
function shipPanel(cn) {
	const P = { ...SHIP, y: SHIP.y + (cn.shift || 0) }, Lh = P.w - 120, kpm = Lh / 180, Wd = 24 * kpm, cx = P.x + P.w / 2 + 20;
	const dy = i => P.y + 136 + Wd / 2 + i * (Wd + 66);
	const X = t => cx + t * Lh, Y = (i, v) => dy(i) + v * Wd;
	const lab = (x, y, s, o = {}) => text(x, y, s, { size: o.size || 17, weight: 700, fill: o.fill || '#f6e7c1', haloW: 5, ls: o.ls === undefined ? 1.5 : o.ls, anchor: o.anchor });
	const R = L.rng(77);
	let s = `<rect x="${P.x}" y="${P.y}" width="${P.w}" height="${P.h}" rx="6" fill="#071a28" stroke="${C.line}" stroke-width="1.5" stroke-opacity="0.7"/>`;
	s += text(P.x + 24, P.y + 44, 'THE GILDED LILY · THE DECKS WE PLAY', { size: 28, weight: 700, fill: '#f6e7c1', anchor: 'start', halo: false, ls: 3 });
	s += text(P.x + 24, P.y + 74, `true scale: 180 m long, 24 m across · bow to the right · ${cn.deck.how}`, { size: 17, weight: 400, fill: C.dim, anchor: 'start', halo: false });
	// ship scale bar, top right
	s += `<g transform="translate(${f1(P.x + P.w - 30 - 50 * kpm - 60)} ${P.y + 56})">${[0, 10, 20, 30, 40].map((m, i) => i ? `<rect x="${f1((m - 10) * kpm)}" y="-6" width="${f1(10 * kpm)}" height="9" fill="${i % 2 ? '#f6e7c1' : '#120d0a'}" stroke="${i % 2 ? '#120d0a' : '#f6e7c1'}"/>` : '').join('')}${text(0, -14, '0', { size: 14, halo: false })}${text(20 * kpm, -14, '20', { size: 14, halo: false })}${text(40 * kpm, -14, '40 m', { size: 14, halo: false })}</g>`;
	// stair links between decks (drawn under the hulls)
	const links = [[0, 1, -0.16, 'grand staircase'], [1, 2, 0.05, 'service stairs'], [0, 2, 0.37, 'cargo hatch']];
	for (const [a, b, t] of links) s += `<line x1="${f1(X(t))}" y1="${f1(dy(a))}" x2="${f1(X(t))}" y2="${f1(dy(b))}" stroke="${C.line}" stroke-width="4" stroke-dasharray="8 6" opacity="0.9"/>`;
	const names = ['UPPER DECKS · promenade, lifeboats, funnels, bridge', 'CASINO & BALLROOM DECK', 'ENGINE ROOM & HOLD'];
	for (let i = 0; i < 3; i++) {
		const y = dy(i);
		s += text(P.x + 24, y - Wd / 2 - 14, names[i], { size: 19, weight: 700, fill: C.accent, anchor: 'start', halo: false, ls: 2 });
		s += `<g transform="translate(${f1(cx)} ${f1(y)})"><path d="${L.hullPath(Lh, Wd)}" fill="#000" opacity="0.45" transform="translate(6 8)"/><path d="${L.hullPath(Lh, Wd)}" fill="#24100f" stroke="${C.gold}" stroke-width="3.5"/>`;
		const inner = L.hullPath(Lh * 0.975, Wd * 0.9);
		const x = t => t * Lh, yy = v => v * Wd;
		const room = (t0, t1, v0, v1, fill, o = {}) => `<rect x="${f1(x(t0))}" y="${f1(yy(v0))}" width="${f1(x(t1) - x(t0))}" height="${f1(yy(v1) - yy(v0))}" rx="${o.rx || 3}" fill="${fill}" stroke="${o.stroke || '#c9a050'}" stroke-width="${o.sw || 2}"/>`;
		if (i === 0) {
			s += `<path d="${inner}" fill="#9c7448"/>`;
			for (let v = -0.42; v <= 0.42; v += 0.035) s += `<line x1="${f1(x(-0.47))}" y1="${f1(yy(v))}" x2="${f1(x(0.44))}" y2="${f1(yy(v))}" stroke="#7b5634" stroke-width="1" opacity="0.55"/>`;
			s += room(-0.335, 0.27, -0.29, 0.29, '#e6d8b8', { stroke: C.goldDeep, sw: 2.5 });
			for (let t = -0.32; t < 0.26; t += 0.025) s += `<line x1="${f1(x(t))}" y1="${f1(yy(-0.29))}" x2="${f1(x(t))}" y2="${f1(yy(0.29))}" stroke="#cdbb92" stroke-width="1"/>`;
			for (let k = 0; k < 14; k++) s += `<ellipse cx="${f1(x(-0.33 + R() * 0.58))}" cy="${f1(yy((R() - 0.5) * 0.5))}" rx="${f1(6 + R() * 16)}" ry="${f1(3 + R() * 7)}" fill="#a48c66" opacity="0.55"/>`;
			for (const t of [-0.3, -0.1, 0.12]) s += `<rect x="${f1(x(t))}" y="${f1(yy(-0.06))}" width="${f1(Lh * 0.03)}" height="${f1(Wd * 0.12)}" fill="#ffe3a0" stroke="${C.goldDeep}" stroke-width="1.2"/>`;
			s += `<circle cx="${f1(x(-0.25))}" cy="0" r="${f1(Wd * 0.13)}" fill="#ffe3a0" stroke="${C.gold}" stroke-width="2.5"/><circle cx="${f1(x(-0.25))}" cy="0" r="${f1(Wd * 0.3)}" fill="url(#lampGlow)" opacity="0.8"/>`;
			for (let a = 0; a < 8; a++) s += `<line x1="${f1(x(-0.25))}" y1="0" x2="${f1(x(-0.25) + Math.cos(a * Math.PI / 4) * Wd * 0.13)}" y2="${f1(Math.sin(a * Math.PI / 4) * Wd * 0.13)}" stroke="${C.goldDeep}" stroke-width="1.2"/>`;
			for (const t of [-0.17, -0.05, 0.07, 0.19]) {
				const fx = x(t), rx = 3.8 * kpm, ry = 2.9 * kpm;
				for (const sd of [-1, 1]) s += `<line x1="${f1(fx)}" y1="0" x2="${f1(fx - rx * 1.6)}" y2="${f1(sd * Wd * 0.44)}" stroke="#2a1a10" stroke-width="1"/><line x1="${f1(fx)}" y1="0" x2="${f1(fx + rx * 0.8)}" y2="${f1(sd * Wd * 0.44)}" stroke="#2a1a10" stroke-width="1"/>`;
				s += `<ellipse cx="${f1(fx - 12)}" cy="10" rx="${f1(rx)}" ry="${f1(ry)}" fill="#000" opacity="0.4"/><ellipse cx="${f1(fx)}" cy="0" rx="${f1(rx)}" ry="${f1(ry)}" fill="#d4952e" stroke="#5a3a10" stroke-width="2"/><ellipse cx="${f1(fx)}" cy="0" rx="${f1(rx * 0.86)}" ry="${f1(ry * 0.86)}" fill="none" stroke="${C.gold}" stroke-width="2"/><ellipse cx="${f1(fx - rx * 0.1)}" cy="0" rx="${f1(rx * 0.66)}" ry="${f1(ry * 0.64)}" fill="#121010"/>`;
			}
			s += room(0.272, 0.305, -0.47, 0.47, '#f0e6cf', { stroke: C.goldDeep, sw: 2.5 }) + room(0.278, 0.3, -0.16, 0.16, '#3a2a1c', { stroke: C.gold });
			for (let v = -0.13; v <= 0.13; v += 0.04) s += `<rect x="${f1(x(0.298))}" y="${f1(yy(v))}" width="4" height="${f1(Wd * 0.025)}" fill="#ffd27a"/>`;
			s += room(0.345, 0.395, -0.15, 0.15, '#4a3220', { stroke: C.line, sw: 3 });
			for (let t = 0.35; t < 0.39; t += 0.01) s += `<line x1="${f1(x(t))}" y1="${f1(yy(-0.15))}" x2="${f1(x(t))}" y2="${f1(yy(0.15))}" stroke="#2a1a10" stroke-width="1.5"/>`;
			for (const v of [-0.3, 0.3]) s += `<circle cx="${f1(x(0.42))}" cy="${f1(yy(v))}" r="9" fill="#5a5048" stroke="#1a1410" stroke-width="1.5"/><path d="M${f1(x(0.42))},${f1(yy(v))} L${f1(x(0.485))},${f1(yy(v * 0.25))}" stroke="#3a3430" stroke-width="3" stroke-dasharray="4 2"/>`;
			for (const t of [-0.41, 0.375]) s += `<circle cx="${f1(x(t))}" cy="0" r="7" fill="#2a1a10"/>`;
			s += `<path d="M${f1(x(-0.41))},0 L${f1(x(0.375))},0" stroke="#3a2a1c" stroke-width="1.2" opacity="0.6"/>`;
			for (let t = -0.32; t < 0.23; t += 0.0455) for (const sd of [-1, 1]) s += `<rect x="${f1(x(t))}" y="${f1(sd * Wd * 0.385 - Wd * 0.05)}" width="${f1(9 * kpm)}" height="${f1(Wd * 0.1)}" rx="${f1(Wd * 0.05)}" fill="#f3efe6" stroke="#5a4a3a" stroke-width="1.2"/><rect x="${f1(x(t) + 6)}" y="${f1(sd * Wd * 0.385 - Wd * 0.025)}" width="${f1(9 * kpm - 12)}" height="${f1(Wd * 0.05)}" rx="3" fill="#8a6440"/>`;
			for (let k = 0; k < 26; k++) s += `<rect x="${f1(x(-0.46 + R() * 0.11))}" y="${f1(yy((R() - 0.5) * 0.7))}" width="12" height="6" rx="2" fill="#5a3a26" opacity="0.8"/>`;
			for (const sd of [-1, 1]) for (let t = -0.45; t < 0.42; t += 0.018) s += `<circle cx="${f1(x(t))}" cy="${f1(sd * Wd * 0.445)}" r="5" fill="url(#lampGlow)"/><circle cx="${f1(x(t))}" cy="${f1(sd * Wd * 0.445)}" r="1.4" fill="#fff3c4"/>`;
			s += lab(x(-0.02), yy(-0.345) + 6, 'PROMENADE (PORT SIDE)', { size: 15 }) + lab(x(-0.02), yy(0.345) + 6, 'PROMENADE (STARBOARD)', { size: 15 });
			s += lab(x(0.288), yy(-0.47) - 8, 'BRIDGE', { size: 16 }) + lab(x(0.37), yy(0.15) + 22, 'CARGO HATCH', { size: 14 });
			s += lab(x(-0.415), yy(0.06) + 6, 'AFT DECK', { size: 16 }) + lab(x(0.44), 6, 'FOREDECK', { size: 14 });
			s += lab(x(-0.25), yy(0.13) + 22, 'DOME', { size: 13 });
		} else if (i === 1) {
			s += `<path d="${inner}" fill="#2a1214"/>`;
			s += room(-0.47, -0.2, -0.4, 0.4, '#4e171d', { sw: 3 });
			s += `<ellipse cx="${f1(x(-0.335))}" cy="0" rx="${f1(Lh * 0.1)}" ry="${f1(Wd * 0.3)}" fill="#8a5a30" stroke="${C.gold}" stroke-width="3"/>`;
			for (let k = -6; k <= 6; k++) s += `<line x1="${f1(x(-0.335) + k * Lh * 0.014)}" y1="${f1(-Wd * 0.3 * Math.sqrt(Math.max(0, 1 - (k / 7.1) ** 2)))}" x2="${f1(x(-0.335) + k * Lh * 0.014)}" y2="${f1(Wd * 0.3 * Math.sqrt(Math.max(0, 1 - (k / 7.1) ** 2)))}" stroke="#6a4222" stroke-width="1.4"/>`;
			for (const [t, v] of [[-0.42, -0.25], [-0.42, 0.25], [-0.25, -0.25], [-0.25, 0.25], [-0.335, -0.33], [-0.335, 0.33]]) s += `<circle cx="${f1(x(t))}" cy="${f1(yy(v))}" r="28" fill="url(#lampGlow)"/><circle cx="${f1(x(t))}" cy="${f1(yy(v))}" r="7" fill="#fff0bf" stroke="${C.gold}" stroke-width="1.5"/>`;
			for (let t = -0.46; t < -0.21; t += 0.022) for (const v of [-0.37, 0.37]) s += `<circle cx="${f1(x(t))}" cy="${f1(yy(v))}" r="4" fill="${C.gold}"/>`;
			s += `<path d="M${f1(x(-0.468))},${f1(yy(-0.14))} A${f1(Wd * 0.14)},${f1(Wd * 0.14)} 0 0 1 ${f1(x(-0.468))},${f1(yy(0.14))}" fill="#3a1418" stroke="${C.gold}" stroke-width="2"/>`;
			s += `<ellipse cx="${f1(x(-0.335))}" cy="0" rx="40" ry="26" fill="#1f5a3a" stroke="${C.gold}" stroke-width="3.5"/>`;
			for (let k = 0; k < 7; k++) { const a = k / 7 * Math.PI * 2; s += `<circle cx="${f1(x(-0.335) + Math.cos(a) * 50)}" cy="${f1(Math.sin(a) * 34)}" r="6" fill="#3a1a10" stroke="${C.gold}" stroke-width="1"/>`; }
			for (let k = 0; k < 5; k++) s += `<rect x="${f1(x(-0.335) - 18 + k * 8)}" y="-6" width="6" height="9" fill="#f3efe6" transform="rotate(${k * 8 - 16} ${f1(x(-0.335))} 0)"/>`;
			s += lab(x(-0.335), yy(-0.4) + 24, 'BALLROOM', { size: 19 }) + lab(x(-0.335), 62, "NED'S TABLE", { size: 15, fill: C.lamp });
			s += lab(x(-0.432), yy(0.2), 'BAND', { size: 12 });
			// grand staircase: two curved flights meeting at a landing
			s += room(-0.2, -0.12, -0.4, 0.4, '#3a1418', { sw: 2.5 });
			for (const sd of [-1, 1]) for (let k = 0; k < 9; k++) s += `<path d="M${f1(x(-0.195) + k * 7)},${f1(sd * Wd * 0.36)} Q${f1(x(-0.13))},${f1(sd * Wd * (0.36 - k * 0.012))} ${f1(x(-0.13) + 4)},${f1(sd * Wd * 0.08)}" fill="none" stroke="${C.gold}" stroke-width="1.5" opacity="${f1(0.5 + k * 0.05)}"/>`;
			s += `<rect x="${f1(x(-0.175))}" y="${f1(yy(-0.08))}" width="${f1(Lh * 0.05)}" height="${f1(Wd * 0.16)}" fill="#6a2a2a" stroke="${C.gold}" stroke-width="2"/>`;
			s += lab(x(-0.16), yy(-0.4) + 22, 'GRAND', { size: 14 }) + lab(x(-0.16), yy(-0.4) + 40, 'STAIRCASE', { size: 14 });
			// casino floor
			s += room(-0.12, 0.22, -0.4, 0.4, '#3d161a', { sw: 3 });
			for (let k = 0; k < 220; k++) s += `<circle cx="${f1(x(-0.115 + R() * 0.33))}" cy="${f1(yy((R() - 0.5) * 0.78))}" r="1.4" fill="#7a2a2a" opacity="0.6"/>`;
			for (let t = -0.095; t < 0.19; t += 0.038) for (const v of [-0.19, 0.19]) {
				s += `<ellipse cx="${f1(x(t))}" cy="${f1(yy(v))}" rx="22" ry="14" fill="#1f5a3a" stroke="${C.gold}" stroke-width="2"/>`;
				for (let k = 0; k < 6; k++) { const a = k / 6 * Math.PI * 2; s += `<circle cx="${f1(x(t) + Math.cos(a) * 28)}" cy="${f1(yy(v) + Math.sin(a) * 19)}" r="3.5" fill="#1a0c0a" stroke="#8a6440" stroke-width="0.8"/>`; }
			}
			for (const t of [-0.06, 0.04, 0.14]) s += `<circle cx="${f1(x(t))}" cy="0" r="20" fill="#2a1010" stroke="${C.gold}" stroke-width="2.5"/><circle cx="${f1(x(t))}" cy="0" r="12" fill="#5a1a14" stroke="${C.gold}" stroke-width="1"/>${[0, 1, 2, 3].map(k => `<line x1="${f1(x(t) - 12 * Math.cos(k * Math.PI / 4))}" y1="${f1(-12 * Math.sin(k * Math.PI / 4))}" x2="${f1(x(t) + 12 * Math.cos(k * Math.PI / 4))}" y2="${f1(12 * Math.sin(k * Math.PI / 4))}" stroke="${C.gold}" stroke-width="0.8"/>`).join('')}`;
			for (const t of [-0.01, 0.09]) s += `<rect x="${f1(x(t) - 26)}" y="-12" width="52" height="24" rx="10" fill="#1f5a3a" stroke="${C.gold}" stroke-width="2"/>`;
			for (let t = -0.105; t < 0.2; t += 0.0175) for (const v of [-0.345, 0.345]) s += L.slots(x(t), yy(v)).replace('translate(', 'translate(').replace(/<g transform="translate\(([^)]*)\)">/, (m0, a) => `<g transform="translate(${a}) scale(1.05)">`);
			s += room(0.195, 0.215, -0.12, 0.12, '#3a2a10', { stroke: C.gold, sw: 2 });
			for (let v = -0.11; v < 0.12; v += 0.02) s += `<line x1="${f1(x(0.195))}" y1="${f1(yy(v))}" x2="${f1(x(0.215))}" y2="${f1(yy(v))}" stroke="${C.gold}" stroke-width="0.8"/>`;
			s += lab(x(0.05), yy(-0.4) + 24, 'CASINO FLOOR', { size: 19 });
			s += lab(x(0.205), yy(0.12) + 20, 'CASHIER', { size: 12 });
			// bar and lounge, then the galley forward
			s += room(0.22, 0.32, -0.4, 0.4, '#45201a', { sw: 2.5 });
			s += `<path d="M${f1(x(0.245))},${f1(yy(-0.3))} Q${f1(x(0.3))},0 ${f1(x(0.245))},${f1(yy(0.3))}" fill="none" stroke="#7a4a24" stroke-width="14"/><path d="M${f1(x(0.245))},${f1(yy(-0.3))} Q${f1(x(0.3))},0 ${f1(x(0.245))},${f1(yy(0.3))}" fill="none" stroke="${C.gold}" stroke-width="2"/>`;
			for (let k = 0; k < 8; k++) s += `<rect x="${f1(x(0.29) + (k % 2) * 34)}" y="${f1(yy(-0.32) + Math.floor(k / 2) * Wd * 0.17)}" width="28" height="22" rx="6" fill="#6a1d24" stroke="${C.gold}" stroke-width="1"/>`;
			s += lab(x(0.27), yy(-0.4) + 24, 'BAR & LOUNGE', { size: 15 });
			s += room(0.32, 0.44, -0.36, 0.36, '#3a3434', { stroke: '#8a8078', sw: 2 }) + `<line x1="${f1(x(0.32))}" y1="0" x2="${f1(x(0.44))}" y2="0" stroke="#8a8078" stroke-width="2"/>`;
			s += lab(x(0.41), yy(-0.18) + 6, 'GALLEY', { size: 14, fill: '#d8d0c8' }) + lab(x(0.41), yy(0.18) + 6, 'CREW', { size: 14, fill: '#d8d0c8' });
		} else {
			s += `<path d="${inner}" fill="#1d1a1a"/>`;
			s += room(-0.47, -0.22, -0.4, 0.4, '#2c2a2a', { stroke: '#7a7070', sw: 2 });
			for (const v of [-0.2, 0.2]) { s += `<rect x="${f1(x(-0.44))}" y="${f1(yy(v) - 22)}" width="${f1(Lh * 0.19)}" height="44" rx="10" fill="#7a7068" stroke="#2a2420" stroke-width="2"/>`; for (let k = 0; k < 4; k++) s += `<circle cx="${f1(x(-0.425) + k * Lh * 0.045)}" cy="${f1(yy(v))}" r="15" fill="#9a9088" stroke="#2a2420" stroke-width="2"/>`; }
			s += `<rect x="${f1(x(-0.42))}" y="-16" width="${f1(Lh * 0.12)}" height="32" rx="12" fill="#6a6058" stroke="#2a2420" stroke-width="2"/>`;
			for (const v of [-0.2, 0, 0.2]) s += `<line x1="${f1(x(-0.44))}" y1="${f1(yy(v))}" x2="${f1(x(-0.5))}" y2="${f1(yy(v * 0.6))}" stroke="#8a8078" stroke-width="5"/>`;
			s += lab(x(-0.345), yy(-0.4) + 24, 'ENGINE ROOM', { size: 17 });
			for (let r = 0; r < 5; r++) {
				const t0 = -0.215 + r * 0.071;
				s += room(t0, t0 + 0.068, -0.27, 0.27, '#2a2020', { stroke: '#6a6060', sw: 2 });
				for (const v of [-0.12, 0.12]) for (const dt of [0.018, 0.05]) s += `<circle cx="${f1(x(t0 + dt))}" cy="${f1(yy(v))}" r="40" fill="url(#lampGlow)" opacity="0.8"/><circle cx="${f1(x(t0 + dt))}" cy="${f1(yy(v))}" r="22" fill="#5a3020" stroke="#ff9f1c" stroke-width="2.5"/><circle cx="${f1(x(t0 + dt))}" cy="${f1(yy(v))}" r="10" fill="#ffb347"/>`;
			}
			for (const sd of [-1, 1]) { s += room(-0.215, 0.14, sd > 0 ? 0.28 : -0.4, sd > 0 ? 0.4 : -0.28, '#0c0b0c', { stroke: '#444', sw: 1.5 }); for (let k = 0; k < 90; k++) s += `<circle cx="${f1(x(-0.21 + R() * 0.345))}" cy="${f1(yy(sd * (0.29 + R() * 0.1)))}" r="${f1(1.5 + R() * 2.5)}" fill="#3a3a40" opacity="0.8"/>`; }
			s += lab(x(-0.04), yy(-0.34) + 6, 'COAL BUNKERS', { size: 15, fill: '#d8d0c8' }) + lab(x(-0.04), yy(0.34) + 6, 'COAL BUNKERS', { size: 15, fill: '#d8d0c8' });
			s += lab(x(-0.04), 6, 'BOILER ROOMS 1-5', { size: 17 });
			s += room(0.145, 0.33, -0.4, 0.4, '#2a241c', { stroke: '#8a7458', sw: 2 });
			for (let t = 0.155; t < 0.32; t += 0.022) for (let v = -0.34; v < 0.34; v += 0.11) if (R() < 0.7) s += `<rect x="${f1(x(t))}" y="${f1(yy(v))}" width="${f1(16 + R() * 14)}" height="${f1(16 + R() * 10)}" fill="#8a6a44" stroke="#2a1a10" stroke-width="1.2"/>`;
			s += lab(x(0.24), yy(-0.4) + 24, 'CARGO HOLD', { size: 17 });
			s += room(0.335, 0.415, -0.3, 0.3, '#3a2a10', { stroke: C.gold, sw: 4 });
			for (let k = 0; k < 16; k++) s += `<ellipse cx="${f1(x(0.345 + R() * 0.06))}" cy="${f1(yy((R() - 0.5) * 0.5))}" rx="${f1(10 + R() * 14)}" ry="${f1(6 + R() * 8)}" fill="${C.gold}" stroke="#8a5a10" stroke-width="1"/>`;
			s += `<circle cx="${f1(x(0.335))}" cy="0" r="26" fill="#5a5048" stroke="${C.gold}" stroke-width="3"/><circle cx="${f1(x(0.335))}" cy="0" r="12" fill="none" stroke="${C.gold}" stroke-width="2.5"/><circle cx="${f1(x(0.375))}" cy="0" r="90" fill="url(#lampGlow)" opacity="0.55"/>`;
			s += lab(x(0.375), yy(-0.3) - 10, "THE HOUSE'S VAULT", { size: 16, fill: C.lamp }) + lab(x(0.375), yy(0.3) + 26, 'grave gold', { size: 13, fill: C.lamp });
			s += lab(x(0.452), 6, 'CHAIN', { size: 12, fill: '#d8d0c8' });
		}
		s += '</g>';
	}
	for (const [a, b, t, nm] of links) {
		const yy = a === 0 && b === 2 ? (dy(1) + dy(2)) / 2 : (dy(a) + dy(b)) / 2;
		s += chip(X(t) + 10, yy + 5, nm, { size: 15, col: C.line, anchor: 'start', weight: 700 });
	}
	// fights on board, then the way through
	for (const f of cn.deck.fights) s += fight(X(f.t), Y(f.deck, f.v || 0), f.d, f.kind || 'crew', f.tag, 1 / kpm, { below: f.below });
	const Rt = cn.deck.route, pts = Rt.map(r => [X(r.t), Y(r.deck, r.v)]), rp = [pts[0]];
	for (let k = 1; k < Rt.length; k++) { for (const [t, v] of Rt[k].pre || []) rp.push([X(t), Y(Rt[k - 1].deck, v)]); if (Rt[k].via !== undefined) rp.push([X(Rt[k].via), Y(Rt[k - 1].deck, 0)], [X(Rt[k].via), Y(Rt[k].deck, 0)]); for (const [t, v] of Rt[k].post || []) rp.push([X(t), Y(Rt[k].deck, v)]); rp.push(pts[k]); }
	s += `<path d="${L.d(rp)}" fill="none" stroke="#000" stroke-width="9" opacity="0.5" stroke-linejoin="round"/><path d="${L.d(rp)}" fill="none" stroke="${C.accent}" stroke-width="4.5" stroke-dasharray="12 7" stroke-linejoin="round"/>`;
	Rt.forEach((r, k) => { const [px, py] = pts[k]; s += `<circle cx="${f1(px)}" cy="${f1(py)}" r="17" fill="${C.accent}" stroke="#1a0f00" stroke-width="3"/>` + text(px, py + 7, r.k, { size: 20, weight: 700, fill: '#1a0f00', halo: false }); });
	if (cn.deck.board) { const b = cn.deck.board; s += chip(X(b.t), Y(b.deck, b.v), b.label, { size: 15, col: C.lamp, weight: 700, anchor: b.anchor || 'middle' }); }
	// the route in words along the bottom
	let x = P.x + 24; const yb = P.y + P.h - 26;
	Rt.forEach(r => {
		s += `<circle cx="${x + 13}" cy="${yb - 6}" r="13" fill="${C.accent}"/>` + text(x + 13, yb + 0.5, r.k, { size: 16, weight: 700, fill: '#1a0f00', halo: false });
		s += text(x + 32, yb, r.say, { size: 16.5, weight: 400, fill: C.text, anchor: 'start', halo: false });
		x += 40 + r.say.length * 8.3 + 18;
	});
	return s;
}

// ---------- page ----------
function section(x, y, w, title) {
	return `<line x1="${x}" y1="${y + 7}" x2="${x + w}" y2="${y + 7}" stroke="${C.line}" stroke-opacity="0.35"/><rect x="${x}" y="${y - 14}" width="${title.length * 12 + 18}" height="28" fill="#071a28"/>` + text(x + 2, y + 8, title, { size: 19, weight: 700, fill: C.title, anchor: 'start', halo: false, ls: 2.5 });
}
function page(cn) {
	const HH = H2 + (cn.shift || 0);
	let s = `<svg xmlns="http://www.w3.org/2000/svg" width="${W2}" height="${HH}" viewBox="0 0 ${W2} ${HH}">${L.defs().replace('</defs>', skDefs() + '</defs>')}`;
	s += `<rect width="${W2}" height="${HH}" fill="#06121c"/>`;
	s += text(32, 66, 'THE GILDED LILY', { size: 48, weight: 700, fill: '#f6e7c1', anchor: 'start', halo: false, ls: 3 });
	s += `<rect x="470" y="26" width="${f1(cn.name.length * 23 + 50 + `CONCEPT ${cn.key}`.length * 15.6)}" height="52" rx="4" fill="${C.accent}"/>`;
	s += text(488, 63, `CONCEPT ${cn.key}`, { size: 25, weight: 700, fill: '#1a0f00', anchor: 'start', halo: false, ls: 1 });
	s += text(488 + `CONCEPT ${cn.key}`.length * 15.6 + 14, 64, cn.name.toUpperCase(), { size: 29, weight: 700, fill: '#1a0f00', anchor: 'start', halo: false, ls: 2 });
	s += text(32, 100, cn.subtitle, { size: 18, weight: 400, fill: C.dim, anchor: 'start', halo: false });
	s += `<rect x="${COL.x}" y="22" width="${COL.w}" height="64" rx="4" fill="#0e2c42" stroke="${C.line}" stroke-width="1.5"/>`;
	s += text(COL.x + 18, 46, `${cn.kind} · PLAYS`, { size: 14, weight: 700, fill: C.dim, anchor: 'start', halo: false, ls: 2 });
	s += text(COL.x + 18, 75, cn.tag, { size: 27, weight: 700, fill: C.accent, anchor: 'start', halo: false, ls: 2 });
	s += text(COL.x + COL.w - 16, 48, `shore ≈ ${(Math.round(cn.area / 1000) * 1000).toLocaleString('en-US')} m² · 1.5× Ransom's Rest`, { size: 15, weight: 600, fill: C.text, anchor: 'end', halo: false });
	s += text(COL.x + COL.w - 16, 72, 'liner 180 m, anchored offshore', { size: 14, weight: 400, fill: C.dim, anchor: 'end', halo: false });
	// map
	s += `<clipPath id="mapClip2"><rect width="${MAP2.w}" height="${MAP2.h}" rx="6"/></clipPath><g transform="translate(${MAP2.x} ${MAP2.y})"><g clip-path="url(#mapClip2)">${cn.map}`;
	s += `<rect width="${MAP2.w}" height="${MAP2.h}" fill="url(#vignette)"/><rect width="${MAP2.w}" height="${MAP2.h}" filter="url(#paper)" opacity="0.06"/>`;
	s += scaleBar(cn.scaleAt[0], cn.scaleAt[1], cn.mpp) + compass(cn.compassAt[0], cn.compassAt[1]) + '</g>';
	s += `<rect width="${MAP2.w}" height="${MAP2.h}" rx="6" fill="none" stroke="${C.line}" stroke-width="2" stroke-opacity="0.8"/>`;
	for (const [cx, cy, sx, sy] of [[0, 0, 1, 1], [MAP2.w, 0, -1, 1], [0, MAP2.h, 1, -1], [MAP2.w, MAP2.h, -1, -1]]) s += `<path d="M${cx + sx * 50},${cy} H${cx} V${cy + sy * 50}" fill="none" stroke="${C.accent}" stroke-width="5"/>`;
	s += '</g>';
	// right column: arrival sketch, then the notes
	s += skFrame(COL.x, COL.y, `<g>${cn.sketch}</g>`, 'ARRIVING BY TRAIN, AT DUSK');
	let y = COL.y + SK.h + 18;
	s += `<rect x="${COL.x}" y="${y}" width="${COL.w}" height="${MAP2.y + MAP2.h - y}" rx="6" fill="#071a28" stroke="${C.line}" stroke-width="1.5" stroke-opacity="0.7"/>`;
	const px = COL.x + 20, pw = COL.w - 40;
	y += 32; s += section(px, y, pw, 'HOW IT PLAYS'); y += 36;
	for (const ln of cn.plays) { s += text(px, y, ln, { size: 17.5, weight: 400, fill: C.text, anchor: 'start', halo: false }); y += 25; }
	y += 14; s += section(px, y, pw, 'THE PATH'); y += 36;
	cn.path.forEach((ln, i) => {
		s += `<circle cx="${px + 14}" cy="${y - 6}" r="13" fill="${C.accent}"/>` + text(px + 14, y + 0.5, String(i + 1), { size: 16, weight: 700, fill: '#1a0f00', halo: false });
		s += text(px + 36, y, ln, { size: 17, weight: 400, fill: C.text, anchor: 'start', halo: false });
		y += 27;
	});
	y += 14; s += section(px, y, pw, 'BIG FIGHTS (ring = arena size)'); y += 36;
	cn.fights.forEach(fl => {
		s += `<circle cx="${px + 14}" cy="${y - 6}" r="11" fill="none" stroke="${FIGHT[fl[0]]}" stroke-width="3" stroke-dasharray="5 3"/>`;
		s += text(px + 34, y, fl[1], { size: 17, weight: 700, fill: FIGHT[fl[0]], anchor: 'start', halo: false });
		s += text(px + 34 + fl[1].length * 9.4 + 8, y, fl[2], { size: 16.5, weight: 400, fill: C.text, anchor: 'start', halo: false });
		y += 27;
	});
	// legend in one line under the notes
	y = MAP2.y + MAP2.h - 22;
	const leg = [[L.step(0, 0, 1).replace('<g transform="translate(0 0)">', '<g transform="translate(0 0) scale(0.8)">'), 'path'], [L.giver(0, 0), 'mission'], [`<circle r="10" fill="none" stroke="${C.crew}" stroke-width="3" stroke-dasharray="5 3"/>`, "Ned's crew"], [`<circle r="10" fill="none" stroke="${FIGHT.beast}" stroke-width="3" stroke-dasharray="5 3"/>`, 'creatures'], [`<circle r="10" fill="none" stroke="${C.line}" stroke-width="3" stroke-dasharray="5 3"/>`, 'bounty'], [L.chest(0, 0), 'chest'], [L.secret(0, 0), 'secret']];
	let lx = px + 10;
	for (const [ic, lb] of leg) { s += `<g transform="translate(${lx} ${y - 6})">${ic}</g>` + text(lx + 18, y, lb, { size: 15, weight: 400, fill: C.dim, anchor: 'start', halo: false }); lx += 26 + lb.length * 7.4 + 12; }
	s += `<g id="shipDecks">${shipPanel(cn)}</g>`;
	if (cn.boarding) s += cn.boarding(cn);
	return s + '</svg>';
}

module.exports = { W2, H2, MAP2, COL, SK, fight, bigLabel, scaleBar, compass, skSky, skLiner, skShape, skTown, skLamp, skPlatform, page, FIGHT, shipXY, skFrame, SHIP };
