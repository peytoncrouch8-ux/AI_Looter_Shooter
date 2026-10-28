// Frame, header, side panel (how it plays, the path, the Lily's deck plan, the legend) around one concept map.
const L = require('./lib');
const { C, MAP, PANEL, W, H, f1, text, chip } = L;

// ---------- the Lily's deck plan (same ship in every concept; the way through differs) ----------
// Bow to the right. Widths drawn about 2x for legibility.
function deckPlan(x0, y0, w, variant) {
	const Lh = w - 36, Wd = 92, gap = 52, cx = x0 + w / 2 + 6;
	const decks = [
		{ name: 'UPPER DECKS', note: 'promenade · lifeboats · four funnels · the bridge · sea wind' },
		{ name: 'CASINO & BALLROOM', note: 'card tables · slots · grand staircase · Ned\'s table' },
		{ name: 'ENGINE ROOM & HOLD', note: 'boilers · coal bunkers · cargo · the house\'s vault' },
	];
	const dy = i => y0 + 30 + i * (Wd + gap) + Wd / 2;
	const X = t => cx + t * Lh, Y = (i, t) => dy(i) + t * Wd;
	let s = '';
	// stair connectors first so the decks sit on them
	const links = [[0, 1, -0.13, 'grand staircase'], [1, 2, 0.04, 'service stairs'], [0, 2, 0.36, 'cargo hatch']];
	for (const [a, b, t] of links) s += `<line x1="${f1(X(t))}" y1="${f1(dy(a))}" x2="${f1(X(t))}" y2="${f1(dy(b))}" stroke="${C.line}" stroke-width="2.4" stroke-dasharray="5 4" opacity="0.9"/>`;
	decks.forEach((dk, i) => {
		const y = dy(i);
		s += text(x0 + 4, y - Wd / 2 - 8, dk.name, { size: 14, weight: 700, fill: C.accent, anchor: 'start', halo: false, ls: 1.5 });
		s += text(x0 + w - 2, y - Wd / 2 - 8, dk.note, { size: 12, weight: 400, fill: C.dim, anchor: 'end', halo: false });
		s += `<g transform="translate(${f1(cx)} ${f1(y)})"><path d="${L.hullPath(Lh, Wd)}" fill="#24100f" stroke="${C.gold}" stroke-width="2"/>`;
		const inner = L.hullPath(Lh * 0.95, Wd * 0.82);
		if (i === 0) {
			s += `<path d="${inner}" fill="url(#planks)"/>`;
			s += `<rect x="${f1(-Lh * 0.31)}" y="${f1(-Wd * 0.22)}" width="${f1(Lh * 0.57)}" height="${f1(Wd * 0.44)}" rx="3" fill="#e6d8b8" stroke="${C.goldDeep}" stroke-width="1.4"/>`;
			for (const t of [-0.17, -0.05, 0.07, 0.19]) s += `<ellipse cx="${f1(Lh * t)}" cy="0" rx="13" ry="10" fill="#d4952e" stroke="#5a3a10" stroke-width="1.2"/><ellipse cx="${f1(Lh * t - 1.5)}" cy="0" rx="8.5" ry="6.5" fill="#141010"/>`;
			s += `<rect x="${f1(Lh * 0.27)}" y="${f1(-Wd * 0.36)}" width="14" height="${f1(Wd * 0.72)}" fill="#f0e6cf" stroke="${C.goldDeep}" stroke-width="1.2"/>`;
			for (let t = -0.3; t < 0.24; t += 0.05) for (const sd of [-1, 1]) s += `<rect x="${f1(Lh * t)}" y="${f1(sd * Wd * 0.31 - 3.5)}" width="13" height="7" rx="3.5" fill="#f3efe6" stroke="#5a4a3a" stroke-width="0.6"/>`;
			s += `<ellipse cx="${f1(Lh * -0.25)}" cy="0" rx="9" ry="8" fill="#ffe3a0" stroke="${C.gold}" stroke-width="1.2"/>`;
			s += `<rect x="${f1(Lh * 0.33)}" y="-10" width="16" height="20" fill="#3a2a1c" stroke="${C.line}" stroke-width="1.5"/>`;
			s += text(Lh * 0.285 + 7, -Wd * 0.36 - 5, 'BRIDGE', { size: 10, weight: 700, fill: '#f6e7c1', haloW: 3 });
			s += text(-Lh * 0.42, 4, 'AFT DECK', { size: 10, weight: 700, fill: '#f6e7c1', haloW: 3 });
			s += text(-Lh * 0.02, -Wd * 0.38 + 1, 'PROMENADE', { size: 9.5, weight: 700, fill: '#f6e7c1', haloW: 3, ls: 1 });
		} else if (i === 1) {
			s += `<path d="${inner}" fill="#3a1418"/>`;
			// ballroom aft: velvet floor in a gilt ring, Ned's table in the middle
			s += `<rect x="${f1(-Lh * 0.45)}" y="${f1(-Wd * 0.38)}" width="${f1(Lh * 0.27)}" height="${f1(Wd * 0.76)}" rx="6" fill="#5a1a20" stroke="${C.gold}" stroke-width="1.2"/>`;
			s += `<ellipse cx="${f1(-Lh * 0.315)}" cy="0" rx="${f1(Lh * 0.1)}" ry="${f1(Wd * 0.3)}" fill="${C.velvet}" stroke="${C.gold}" stroke-width="2"/>`;
			s += `<circle cx="${f1(-Lh * 0.315)}" cy="0" r="9" fill="#1f5a3a" stroke="${C.gold}" stroke-width="2"/><circle cx="${f1(-Lh * 0.315)}" cy="0" r="16" fill="url(#lampGlow)" opacity="0.8"/>`;
			s += text(-Lh * 0.315, -Wd * 0.3 - 4, 'BALLROOM', { size: 10, weight: 700, fill: '#f6e7c1', haloW: 3 });
			s += text(-Lh * 0.315, Wd * 0.3 + 12, "NED'S TABLE", { size: 9.5, weight: 700, fill: C.lamp, haloW: 3 });
			// grand staircase: a fan of steps
			for (let k = 0; k < 6; k++) s += `<path d="M${f1(-Lh * 0.13 - 4 - k * 3.2)},${f1(-16 + k * 0.5)} Q${f1(-Lh * 0.13 + 12 - k * 3.2)},0 ${f1(-Lh * 0.13 - 4 - k * 3.2)},${f1(16 - k * 0.5)}" fill="none" stroke="${C.gold}" stroke-width="1.6"/>`;
			// casino floor: tables, a wheel, slot banks along both walls
			for (let t = -0.05; t < 0.26; t += 0.062) for (const yy of [-0.12, 0.12]) s += `<ellipse cx="${f1(Lh * t)}" cy="${f1(Wd * yy)}" rx="8" ry="5.5" fill="#1f5a3a" stroke="${C.gold}" stroke-width="1"/>`;
			s += `<circle cx="${f1(Lh * 0.29)}" cy="0" r="9" fill="#2a1010" stroke="${C.gold}" stroke-width="1.4"/><path d="M${f1(Lh * 0.29 - 9)},0 h18 M${f1(Lh * 0.29)},-9 v18" stroke="${C.gold}" stroke-width="0.8"/>`;
			s += `<rect x="${f1(Lh * 0.34)}" y="${f1(-Wd * 0.25)}" width="18" height="${f1(Wd * 0.5)}" fill="#4a2a18" stroke="${C.gold}" stroke-width="1"/>`;
			s += text(Lh * 0.1, -Wd * 0.25 - 1, 'CASINO FLOOR', { size: 9.5, weight: 700, fill: '#f6e7c1', haloW: 3, ls: 1 });
			s += text(Lh * 0.35 + 9, Wd * 0.25 + 11, 'BAR', { size: 9.5, weight: 700, fill: '#f6e7c1', haloW: 3 });
			for (const t of [0.0, 0.12, 0.24]) s += `<g transform="scale(0.72)">${L.slots(Lh * t / 0.72, Wd * 0.35 / 0.72)}</g>`;
		} else {
			s += `<path d="${inner}" fill="#1d1a1a"/>`;
			s += `<rect x="${f1(-Lh * 0.45)}" y="${f1(-Wd * 0.34)}" width="${f1(Lh * 0.23)}" height="${f1(Wd * 0.68)}" fill="#2c2a2a" stroke="#6a6060" stroke-width="1"/>`;
			for (const yy of [-0.15, 0.15]) s += `<rect x="${f1(-Lh * 0.42)}" y="${f1(Wd * yy - 7)}" width="${f1(Lh * 0.17)}" height="14" rx="7" fill="#7a7068" stroke="#2a2420" stroke-width="1"/>`;
			s += text(-Lh * 0.335, 4, 'ENGINES', { size: 10, weight: 700, fill: '#f6e7c1', haloW: 3 });
			// coal bunkers along the sides, boilers glowing between
			for (const sd of [-1, 1]) s += `<rect x="${f1(-Lh * 0.2)}" y="${f1(sd > 0 ? Wd * 0.22 : -Wd * 0.36)}" width="${f1(Lh * 0.36)}" height="${f1(Wd * 0.14)}" fill="#0c0b0c" stroke="#444" stroke-width="0.8"/>`;
			for (let t = -0.17; t < 0.15; t += 0.055) for (const yy of [-0.1, 0.1]) s += `<circle cx="${f1(Lh * t)}" cy="${f1(Wd * yy)}" r="9" fill="url(#lampGlow)"/><circle cx="${f1(Lh * t)}" cy="${f1(Wd * yy)}" r="6" fill="#5a3020" stroke="#ff9f1c" stroke-width="1.2"/>`;
			s += text(-Lh * 0.02, -Wd * 0.38 + 1, 'COAL BUNKERS', { size: 9.5, weight: 700, fill: '#d8d0c8', haloW: 3, ls: 1 });
			s += text(-Lh * 0.02, Wd * 0.24 + 1, 'BOILER ROOMS', { size: 9.5, weight: 700, fill: '#f6e7c1', haloW: 3, ls: 1 });
			for (let t = 0.18; t < 0.3; t += 0.035) for (const yy of [-0.2, 0, 0.2]) s += `<rect x="${f1(Lh * t)}" y="${f1(Wd * yy - 5)}" width="10" height="10" fill="#8a6a44" stroke="#2a1a10" stroke-width="0.8"/>`;
			s += text(Lh * 0.24, -Wd * 0.3, 'HOLD', { size: 10, weight: 700, fill: '#f6e7c1', haloW: 3 });
			s += `<rect x="${f1(Lh * 0.31 + 20)}" y="-15" width="30" height="30" fill="#3a2a10" stroke="${C.gold}" stroke-width="2.4"/><circle cx="${f1(Lh * 0.31 + 35)}" cy="0" r="7" fill="none" stroke="${C.gold}" stroke-width="2"/><circle cx="${f1(Lh * 0.31 + 35)}" cy="0" r="22" fill="url(#lampGlow)" opacity="0.6"/>`;
			s += text(Lh * 0.31 + 35, -20, 'VAULT', { size: 10, weight: 700, fill: C.lamp, haloW: 3 });
		}
		s += '</g>';
	});
	// labels on the stair links (between decks)
	for (const [a, b, t, nm] of links) {
		const yy = a === 0 && b === 2 ? (dy(1) + dy(2)) / 2 + 4 : (dy(a) + dy(b)) / 2 + 4;
		if (a === 0 && b === 2) continue;
		s += chip(X(t) + 6, yy, nm, { size: 10.5, col: C.line, anchor: 'start' });
	}
	s += chip(X(0.36) + 6, (dy(0) + dy(1)) / 2 + 4, 'cargo hatch', { size: 10.5, col: C.line, anchor: 'start' });
	// the way through: lettered steps joined by a dashed line
	const R = variant.route;
	const pts = R.map(r => [X(r.t), Y(r.deck, r.y)]);
	s += `<path d="${L.d(pts)}" fill="none" stroke="${C.accent}" stroke-width="2.6" stroke-dasharray="6 4" opacity="0.95"/>`;
	R.forEach((r, k) => {
		const [px, py] = pts[k];
		s += `<circle cx="${f1(px)}" cy="${f1(py)}" r="10.5" fill="${C.accent}" stroke="#1a0f00" stroke-width="2"/>` + text(px, py + 4.5, r.k, { size: 13, weight: 700, fill: '#1a0f00', halo: false });
	});
	for (const e of variant.foes || []) s += L.foeIcon(X(e.t), Y(e.deck, e.y), 'crew').replace('r="12"', 'r="9"');
	for (const e of variant.chests || []) s += L.chest(X(e.t), Y(e.deck, e.y));
	const yEnd = dy(2) + Wd / 2 + 14;
	return { s, yEnd };
}

// ---------- the legend ----------
function legend(x, y, w) {
	const items = [
		[(a, b) => L.step(a, b, 1).replace(/r="21"/, 'r="0"').replace('scale', ''), "the player's path, in order"],
		[(a, b) => L.giver(a, b, 'main'), 'main mission giver'],
		[(a, b) => L.giver(a, b, 'side'), 'side mission giver'],
		[(a, b) => L.bounty(a, b), 'bounty board'],
		[(a, b) => L.foeIcon(a, b, 'crew'), "Ned's crew, going Hollow"],
		[(a, b) => L.foeIcon(a - 13, b, 'crab') + L.foeIcon(a + 13, b, 'eel'), 'crabs · tide-pool eels'],
		[(a, b) => L.foeIcon(a, b, 'gull'), 'cliff gulls'],
		[(a, b) => `<ellipse cx="${a}" cy="${b}" rx="18" ry="11" fill="${C.crew}" fill-opacity="0.13" stroke="${C.crew}" stroke-width="2" stroke-dasharray="6 4"/>`, 'enemy area'],
		[(a, b) => L.chest(a, b), 'chest'],
		[(a, b) => L.secret(a, b), 'secret'],
		[(a, b) => L.slots(a, b), 'slot machines (open after Ned)'],
		[(a, b) => L.station(a, b), 'station: end of the line'],
		[(a, b) => `<path d="M${a - 20},${b} h40" stroke="${C.gold}" stroke-width="3" stroke-dasharray="3 6" stroke-linecap="round"/>` + L.launchBoat(a, b, 0), "the Lily's launch"],
		[(a, b) => `<path d="M${a - 20},${b} h40" stroke="#1b1410" stroke-width="5"/><path d="M${a - 20},${b} h40" stroke="#9aa3ad" stroke-width="1.2"/>`, 'railroad'],
		[(a, b) => `<path d="M${a - 20},${b - 3} h40" stroke="${C.cliff}" stroke-width="4"/>` + [0, 8, 16, 24, 32, 40].map(k => `<line x1="${a - 20 + k}" y1="${b - 3}" x2="${a - 20 + k}" y2="${b + 7}" stroke="${C.cliff}" stroke-width="2"/>`).join(''), 'cliff (ticks point down)'],
		[(a, b) => `<rect x="${a - 20}" y="${b - 9}" width="40" height="18" fill="${C.mud}"/><path d="M${a - 16},${b - 2} q8,4 16,0 t16,0" stroke="#a9c3bd" stroke-width="1.5" fill="none"/>`, 'tide flats'],
		[(a, b) => `<rect x="${a - 20}" y="${b - 9}" width="40" height="18" fill="${C.ridge}"/><rect x="${a - 20}" y="${b - 9}" width="40" height="18" fill="url(#hatch)" opacity="0.55"/>`, 'closing hills (out of bounds)'],
		[(a, b) => L.lighthouse(a, b, 0, 0).replace('r="26"', 'r="16"'), 'lighthouse'],
	];
	let s = '';
	const colW = w / 2, rowH = 31;
	items.forEach(([icon, label], i) => {
		const cx = x + (i % 2) * colW + 24, cy = y + Math.floor(i / 2) * rowH;
		s += icon(cx, cy) + text(cx + 30, cy + 5, label, { size: 13.5, weight: 400, fill: C.text, anchor: 'start', halo: false });
	});
	return { s, yEnd: y + Math.ceil(items.length / 2) * rowH };
}

function section(x, y, w, title) {
	return `<line x1="${x}" y1="${y + 6}" x2="${x + w}" y2="${y + 6}" stroke="${C.line}" stroke-opacity="0.35" stroke-width="1"/>`
		+ `<rect x="${x}" y="${y - 12}" width="${title.length * 9.6 + 20}" height="24" fill="#071a28"/>`
		+ text(x + 2, y + 6, title, { size: 15, weight: 700, fill: C.title, anchor: 'start', halo: false, ls: 2 });
}

// Scale bar in metres (mpp = metres per map px) and a compass.
function scaleBar(x, y, mpp) {
	const seg = 50 / mpp; let s = `<g transform="translate(${f1(x)} ${f1(y)})"><rect x="-10" y="-30" width="${f1(seg * 2 + 70)}" height="50" rx="4" fill="#07131c" fill-opacity="0.75" stroke="${C.line}" stroke-opacity="0.5"/>`;
	s += `<rect x="0" y="-6" width="${f1(seg)}" height="9" fill="#f6e7c1" stroke="#120d0a"/><rect x="${f1(seg)}" y="-6" width="${f1(seg)}" height="9" fill="#120d0a" stroke="#f6e7c1"/>`;
	s += text(0, -12, '0', { size: 12, halo: false }) + text(seg, -12, '50', { size: 12, halo: false }) + text(seg * 2, -12, '100 m', { size: 12, halo: false });
	s += text(seg * 2 + 28, 3, '', { size: 1, halo: false });
	return s + '</g>';
}
function compass(x, y) {
	return `<g transform="translate(${f1(x)} ${f1(y)})"><circle r="30" fill="#07131c" fill-opacity="0.75" stroke="${C.line}" stroke-opacity="0.6" stroke-width="1.5"/><circle r="22" fill="none" stroke="${C.line}" stroke-opacity="0.3"/>`
		+ `<path d="M0,-24 L7,0 L0,-4 L-7,0 Z" fill="${C.accent}"/><path d="M0,24 L7,0 L0,4 L-7,0 Z" fill="${C.dim}" opacity="0.6"/>`
		+ text(0, -34, 'N', { size: 15, weight: 700, fill: C.accent }) + '</g>';
}

// ---------- whole page ----------
function page(cn) {
	let s = `<svg xmlns="http://www.w3.org/2000/svg" width="${W}" height="${H}" viewBox="0 0 ${W} ${H}">${L.defs()}`;
	s += `<rect width="${W}" height="${H}" fill="#06121c"/><rect width="${W}" height="${H}" fill="url(#panelGrad)" opacity="0.6"/>`;
	// header
	s += text(34, 62, 'THE GILDED LILY', { size: 44, weight: 700, fill: '#f6e7c1', anchor: 'start', halo: false, ls: 3 });
	s += `<rect x="440" y="28" width="${f1(cn.name.length * 17 + 150)}" height="46" rx="4" fill="${C.accent}"/>`;
	s += text(456, 61, `CONCEPT ${cn.key}`, { size: 22, weight: 700, fill: '#1a0f00', anchor: 'start', halo: false, ls: 1 });
	s += text(590, 61, cn.name.toUpperCase(), { size: 26, weight: 700, fill: '#1a0f00', anchor: 'start', halo: false, ls: 2 });
	s += text(34, 92, cn.subtitle, { size: 16, weight: 400, fill: C.dim, anchor: 'start', halo: false });
	s += `<rect x="${PANEL.x}" y="26" width="${PANEL.w}" height="58" rx="4" fill="#0e2c42" stroke="${C.line}" stroke-width="1.5"/>`;
	s += text(PANEL.x + 16, 50, 'PLAYS', { size: 13, weight: 700, fill: C.dim, anchor: 'start', halo: false, ls: 2 });
	s += text(PANEL.x + 16, 75, cn.tag, { size: 24, weight: 700, fill: C.accent, anchor: 'start', halo: false, ls: 2 });
	s += text(PANEL.x + PANEL.w - 14, 50, `shore ≈ ${(Math.round(cn.area / 1000) * 1000).toLocaleString('en-US')} m²`, { size: 14, weight: 600, fill: C.text, anchor: 'end', halo: false });
	s += text(PANEL.x + PANEL.w - 14, 72, `${(cn.area / 64000).toFixed(1)}× Ransom's Rest · liner ≈ ${cn.shipM} m`, { size: 13, weight: 400, fill: C.dim, anchor: 'end', halo: false });
	// map window
	s += `<clipPath id="mapClip"><rect x="0" y="0" width="${MAP.w}" height="${MAP.h}" rx="6"/></clipPath>`;
	s += `<g transform="translate(${MAP.x} ${MAP.y})"><g clip-path="url(#mapClip)">${cn.map}`;
	s += `<rect width="${MAP.w}" height="${MAP.h}" fill="url(#vignette)"/><rect width="${MAP.w}" height="${MAP.h}" filter="url(#paper)" opacity="0.06"/>`;
	s += scaleBar(cn.scaleAt[0], cn.scaleAt[1], cn.mpp) + compass(cn.compassAt[0], cn.compassAt[1]);
	s += `</g><rect x="0" y="0" width="${MAP.w}" height="${MAP.h}" rx="6" fill="none" stroke="${C.line}" stroke-width="2" stroke-opacity="0.8"/>`;
	for (const [cx, cy, sx, sy] of [[0, 0, 1, 1], [MAP.w, 0, -1, 1], [0, MAP.h, 1, -1], [MAP.w, MAP.h, -1, -1]]) s += `<path d="M${cx + sx * 46},${cy} H${cx} V${cy + sy * 46}" fill="none" stroke="${C.accent}" stroke-width="5"/>`;
	s += '</g>';
	// panel
	const P = PANEL, px = P.x + 18, pw = P.w - 36;
	s += `<rect x="${P.x}" y="${P.y}" width="${P.w}" height="${P.h}" rx="6" fill="#071a28" stroke="${C.line}" stroke-width="1.5" stroke-opacity="0.7"/>`;
	let y = P.y + 30;
	s += section(px, y, pw, 'HOW IT PLAYS'); y += 34;
	for (const ln of cn.plays) { s += text(px, y, ln, { size: 15, weight: 400, fill: C.text, anchor: 'start', halo: false }); y += 22; }
	y += 18; s += section(px, y, pw, "THE PLAYER'S PATH"); y += 36;
	cn.path.forEach((ln, i) => {
		s += `<circle cx="${px + 13}" cy="${y - 5}" r="12" fill="${C.accent}"/>` + text(px + 13, y + 0.5, String(i + 1), { size: 15, weight: 700, fill: '#1a0f00', halo: false });
		s += text(px + 34, y, ln[0], { size: 15, weight: 700, fill: '#f6e7c1', anchor: 'start', halo: false });
		s += text(px + 34 + ln[0].length * 8.6 + 8, y, ln[1], { size: 14, weight: 400, fill: C.dim, anchor: 'start', halo: false });
		y += 29;
	});
	y += 14; s += section(px, y, pw, "THE LILY: DECKS PLAYED  (bow →)"); y += 14;
	const dp = deckPlan(px, y, pw, cn.deck); s += dp.s; y = dp.yEnd + 6;
	for (const ln of cn.deck.lines) { s += text(px, y, ln, { size: 13.5, weight: 400, fill: C.text, anchor: 'start', halo: false }); y += 19; }
	y += 18; s += section(px, y, pw, 'LEGEND'); y += 34;
	s += legend(px, y, pw).s;
	return s + '</svg>';
}

module.exports = { page };
