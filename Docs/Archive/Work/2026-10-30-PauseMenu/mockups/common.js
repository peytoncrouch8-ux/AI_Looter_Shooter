// Shared pieces for the pause-menu mockups: the real data the menu may show, SVG shapes and icons.
// Every number below is something the game tracks today (README.md says where it comes from).
const DATA = {
	area: "RANSOM'S REST",
	mission: { name: 'SEVEN DAYS', step: 3, steps: 5, text: 'Read the headboard beside yours.', optional: 'Read your own headboard.' },
	hero: { name: 'ELLIS RANSOM', level: 2, hp: 84, hpMax: 100, xp: 1208, xpNext: 2647 },
	flask: { charges: 3, max: 3 },
	grenades: { count: 2, max: 3 },
	session: { slot: 1, of: 3, played: '3 h 05 min', saved: 'Today 14:32', serial: 'SER. RV-0001-02' },
	options: [
		{ key: 'BACK TO GAME', help: 'Close the menu and pick up where you stopped.', sub: 'The world waits while you’re here.' },
		{ key: 'SETTINGS', help: 'Graphics, audio, interface and controls.', sub: 'Changes apply as you make them.' },
		{ key: 'SAVE & QUIT', help: 'Save Session 1 and go back to the main menu.', sub: 'You continue right here next time.' },
		{ key: 'QUIT GAME', help: 'Save Session 1 and close the game.', sub: 'Back to the desktop.', danger: true },
	],
	tabs: ['GRAPHICS', 'AUDIO', 'INTERFACE', 'CONTROLS'],
	rows: [
		{ name: 'Quality', kind: 'choice', values: ['LOW', 'MEDIUM', 'HIGH', 'EPIC'], sel: 1 },
		{ name: 'Motion blur', kind: 'choice', values: ['ON', 'OFF'], sel: 1 },
		{ name: 'Field of view (1st person)', kind: 'slider', min: 70, max: 110, value: 90 },
	],
	help: { title: 'QUALITY', text: 'Sets the whole look at once: mesh detail, shadows, lighting, reflections and edge smoothing. Higher presets look richer and cost far more frame time.', detail: 'Medium: Simpler meshes, two shadow cascades, TAA smoothing. Made to hold 120 fps at 1080p on a Radeon RX 580.' },
};
const C = { salt: '#D4EEF4', gold: '#FFE6A6', flavor: '#FFD38A', text: '#F2F6F8', label: '#B4BEC8', icon: '#98A4AE',
	heal: '#6DFF7A', red: '#FF7A6B', redDeep: '#7A2219', glass: 'rgba(30,34,40,.95)', glass2: 'rgba(18,21,26,.95)', ink: 'rgba(8,10,13,.9)' };
const fmt = n => n.toLocaleString('en-US');

// Polygon points of a box with cut corners: cuts = [topLeft, topRight, bottomRight, bottomLeft] in px.
function cutPts(x, y, w, h, cuts) {
	const [a, b, c, d] = cuts;
	return [[x + a, y], [x + w - b, y], [x + w, y + b], [x + w, y + h - c], [x + w - c, y + h], [x + d, y + h], [x, y + h - d], [x, y + a]]
		.map(p => p.join(',')).join(' ');
}
let gradId = 0;
// A glass panel: cut-corner polygon, glass gradient, solid line. Returns an absolutely placed SVG.
function panelSVG(w, h, o = {}) {
	const id = 'g' + (gradId++);
	const sw = o.sw ?? 2, cuts = o.cuts ?? [0, 0, 0, 0], inset = sw / 2;
	const top = o.top ?? C.glass, bottom = o.bottom ?? C.glass2;
	return `<svg class="abs" style="left:0;top:0" width="${w}" height="${h}">
		<defs><linearGradient id="${id}" x1="0" y1="0" x2="${o.horizontal ? 1 : 0}" y2="${o.horizontal ? 0 : 1}">
			<stop offset="0" stop-color="${top}"/><stop offset="1" stop-color="${bottom}"/></linearGradient></defs>
		<polygon points="${o.points ?? cutPts(inset, inset, w - sw, h - sw, cuts)}" fill="url(#${id})" stroke="${o.stroke ?? C.salt}" stroke-opacity="${o.strokeOpacity ?? 1}" stroke-width="${sw}" stroke-linejoin="miter"/>
		${o.extra ?? ''}</svg>`;
}
// A ruler: a base line with ticks every `step` px, a longer one every `major` ticks.
function rulerSVG(w, o = {}) {
	const step = o.step ?? 14, major = o.major ?? 5, col = o.color ?? C.salt, op = o.opacity ?? .55, up = o.up ?? false;
	const L = o.long ?? 12;
	let s = `<line x1="0" y1="${up ? L : 0}" x2="${w}" y2="${up ? L : 0}" stroke="${col}" stroke-opacity="${op}" stroke-width="${o.sw ?? 1.5}"/>`;
	for (let i = 0, x = 0; x <= w + .1; i++, x += step) {
		const len = i % major === 0 ? L : (o.short ?? 6);
		s += `<line x1="${x}" y1="${up ? L : 0}" x2="${x}" y2="${up ? L - len : len}" stroke="${col}" stroke-opacity="${op}" stroke-width="${i % major === 0 ? 2 : 1.2}"/>`;
	}
	return `<svg width="${w}" height="${L + 2}" style="display:block">${s}</svg>`;
}
// The level gem: a diamond with a 3.5 px line and the number inside.
function gemSVG(size, text, o = {}) {
	const h = size / 2;
	return `<svg width="${size}" height="${size}" style="display:block;flex:none"><polygon points="${h},2 ${size - 2},${h} ${h},${size - 2} 2,${h}"
		fill="${o.fill ?? C.glass2}" stroke="${o.stroke ?? C.gold}" stroke-width="${o.sw ?? 3.5}"/>
		<text x="${h}" y="${h + size * .15}" text-anchor="middle" font-family="Chakra Petch" font-weight="700" font-size="${size * .42}" fill="${o.color ?? C.gold}">${text}</text></svg>`;
}
// The hero's HUD portrait (Art/Icons/HudPortrait.svg) in a diamond medallion with a gunmetal-and-salt bezel.
function portraitHTML(size, o = {}) {
	const h = size / 2, ring = o.ring ?? C.salt;
	return `<div style="position:relative;width:${size}px;height:${size}px;flex:none">
		<img src="../../../../Art/Icons/HudPortrait.svg" style="position:absolute;left:5px;top:5px;width:${size - 10}px;height:${size - 10}px;filter:saturate(.45) brightness(1.05)">
		<svg class="abs" style="left:0;top:0" width="${size}" height="${size}">
			<polygon points="${h},3 ${size - 3},${h} ${h},${size - 3} 3,${h}" fill="none" stroke="#2a3038" stroke-width="7"/>
			<polygon points="${h},2 ${size - 2},${h} ${h},${size - 2} 2,${h}" fill="none" stroke="${ring}" stroke-width="${o.sw ?? 3}"/>
			<polygon points="${h},11 ${size - 11},${h} ${h},${size - 11} 11,${h}" fill="none" stroke="${ring}" stroke-opacity=".35" stroke-width="1.2"/>
		</svg></div>`;
}
// A slim slanted bar (the HUD's bars): track, fill, salt edge.
function barSVG(w, h, frac, col, o = {}) {
	const sk = o.skew ?? h * .7, f = Math.max(0, Math.min(1, frac));
	const pts = (x0, x1) => `${x0 + sk},0 ${x1},0 ${x1 - sk},${h} ${x0},${h}`;
	let notches = '';
	for (let i = 1; i < (o.notches ?? 0); i++) { const x = w * i / o.notches; notches += `<line x1="${x + sk / 2}" y1="1" x2="${x - sk / 2}" y2="${h - 1}" stroke="rgba(8,10,13,.85)" stroke-width="2"/>`; }
	return `<svg width="${w}" height="${h}" style="display:block">
		<polygon points="${pts(0, w)}" fill="rgba(8,10,13,.7)" stroke="${o.edge ?? C.salt}" stroke-opacity="${o.edgeOpacity ?? .45}" stroke-width="1.5"/>
		<polygon points="${pts(2, 2 + (w - 4) * f)}" fill="${col}"/>${notches}</svg>`;
}
// Pips: lit and unlit slanted pips (the flask's and grenades' rows).
function pipsSVG(n, lit, col, o = {}) {
	const w = o.w ?? 12, h = o.h ?? 22, gap = o.gap ?? 5, sk = o.skew ?? 5;
	let s = '';
	for (let i = 0; i < n; i++) {
		const x = i * (w + gap), on = i < lit;
		s += `<polygon points="${x + sk},1 ${x + w + sk},1 ${x + w},${h - 1} ${x},${h - 1}" fill="${on ? col : 'rgba(8,10,13,.6)'}" stroke="${on ? col : C.icon}" stroke-opacity="${on ? 1 : .6}" stroke-width="1.5"/>`;
	}
	return `<svg width="${n * (w + gap) + sk}" height="${h}" style="display:block;flex:none">${s}</svg>`;
}
// Icons, drawn in a 24 box and scaled.
const ICON = {
	flask: c => `<path d="M9.5 2h5v1.6h-.9v4.2a7.2 7.2 0 1 1-3.2 0V3.6h-.9z" fill="none" stroke="${c}" stroke-width="1.8" stroke-linejoin="round"/><path d="M6.6 14.5h10.8a5.6 5.6 0 0 1-10.8 0z" fill="${c}"/>`,
	grenade: c => `<rect x="7.5" y="2.5" width="9" height="3" rx=".6" fill="${c}"/><path d="M7 6.5h10l1.5 3v10.5a1.6 1.6 0 0 1-1.6 1.6H7.1a1.6 1.6 0 0 1-1.6-1.6V9.5z" fill="none" stroke="${c}" stroke-width="1.8" stroke-linejoin="round"/><path d="M9 12h6M9 15.5h6" stroke="${c}" stroke-width="1.8"/>`,
	health: c => `<path d="M12 2l10 10-10 10L2 12z" fill="none" stroke="${c}" stroke-width="1.8"/><path d="M12 7v10M7 12h10" stroke="${c}" stroke-width="2.6"/>`,
	xp: c => `<path d="M12 2l2.9 6.2 6.8.8-5 4.6 1.4 6.7L12 17l-6.1 3.3 1.4-6.7-5-4.6 6.8-.8z" fill="none" stroke="${c}" stroke-width="1.8" stroke-linejoin="round"/>`,
	mission: c => `<circle cx="12" cy="12" r="10" fill="none" stroke="${c}" stroke-width="1.8"/><path d="M12 5.5l1.9 4 4.4.5-3.3 3 .9 4.4L12 15.2l-3.9 2.2.9-4.4-3.3-3 4.4-.5z" fill="${c}"/>`,
	save: c => `<path d="M4 3h13l3 3v15H4z" fill="none" stroke="${c}" stroke-width="1.8" stroke-linejoin="round"/><rect x="7.5" y="3" width="8" height="5.5" fill="none" stroke="${c}" stroke-width="1.6"/><rect x="7" y="13" width="10" height="6" fill="${c}" opacity=".85"/>`,
	clock: c => `<circle cx="12" cy="12" r="9.5" fill="none" stroke="${c}" stroke-width="1.8"/><path d="M12 6.5V12l3.8 2.4" fill="none" stroke="${c}" stroke-width="2" stroke-linecap="round"/>`,
	play: c => `<path d="M7 4.5l13 7.5-13 7.5z" fill="${c}"/>`,
	gear: c => `<circle cx="12" cy="12" r="3.4" fill="none" stroke="${c}" stroke-width="2"/><path d="M12 2.5v3M12 18.5v3M2.5 12h3M18.5 12h3M5.3 5.3l2.1 2.1M16.6 16.6l2.1 2.1M5.3 18.7l2.1-2.1M16.6 7.4l2.1-2.1" stroke="${c}" stroke-width="2.4" stroke-linecap="round"/><circle cx="12" cy="12" r="6.6" fill="none" stroke="${c}" stroke-width="1.8"/>`,
	door: c => `<path d="M5 3h10v18H5z" fill="none" stroke="${c}" stroke-width="1.8"/><path d="M13 12h8M18 8.5l3.5 3.5-3.5 3.5" fill="none" stroke="${c}" stroke-width="2" stroke-linecap="round" stroke-linejoin="round"/><circle cx="11.5" cy="12" r="1.1" fill="${c}"/>`,
	power: c => `<path d="M7.2 6.2a8 8 0 1 0 9.6 0" fill="none" stroke="${c}" stroke-width="2.2" stroke-linecap="round"/><path d="M12 2.5v9" stroke="${c}" stroke-width="2.4" stroke-linecap="round"/>`,
	pause: c => `<rect x="6" y="4" width="4" height="16" fill="${c}"/><rect x="14" y="4" width="4" height="16" fill="${c}"/>`,
};
const OPTION_ICON = ['play', 'gear', 'door', 'power'];
function icon(name, size, col) { return `<svg width="${size}" height="${size}" viewBox="0 0 24 24" style="display:block;flex:none">${ICON[name](col)}</svg>`; }
// The pause dial of today's menu (c3's halo): ring, ticks, a gold arc and the pause bars.
function dialSVG(size, o = {}) {
	const r = size / 2, c = r;
	let t = '';
	for (let i = 0; i < 48; i++) {
		const a = i / 48 * Math.PI * 2, big = i % 6 === 0, r0 = r - (big ? 10 : 6) * size / 120, r1 = r - 1;
		t += `<line x1="${c + Math.cos(a) * r0}" y1="${c + Math.sin(a) * r0}" x2="${c + Math.cos(a) * r1}" y2="${c + Math.sin(a) * r1}" stroke="${C.salt}" stroke-opacity="${big ? .9 : .5}" stroke-width="${big ? 2 : 1.2}"/>`;
	}
	const ri = r * .66, a0 = -Math.PI * .45, a1 = Math.PI * .1;
	const arc = `M${c + Math.cos(a0) * ri},${c + Math.sin(a0) * ri} A${ri},${ri} 0 0 1 ${c + Math.cos(a1) * ri},${c + Math.sin(a1) * ri}`;
	const bw = size * .085, bh = size * .3;
	return `<svg width="${size}" height="${size}" style="display:block;flex:none">${t}
		<circle cx="${c}" cy="${c}" r="${r * .8}" fill="${C.glass2}" stroke="${C.salt}" stroke-width="2"/>
		<path d="${arc}" fill="none" stroke="${C.gold}" stroke-width="${size * .04}" stroke-linecap="round"/>
		<rect x="${c - bw * 1.6}" y="${c - bh / 2}" width="${bw}" height="${bh}" fill="${C.text}"/>
		<rect x="${c + bw * .6}" y="${c - bh / 2}" width="${bw}" height="${bh}" fill="${C.text}"/></svg>`;
}
// Key hints for the foot of the screen.
function keysHTML(list) {
	return `<div class="keys">${list.map(([k, w]) => `<div class="hint out"><span class="cap">${k}</span>${w}</div>`).join('')}</div>`;
}
const HUB_KEYS = [['ENTER', 'SELECT'], ['ESC', 'RESUME']];
const SET_KEYS = [['ENTER', 'SELECT'], ['PGUP / PGDN', 'TABS'], ['ESC', 'BACK']];
function place(html, css) { const d = document.createElement('div'); d.className = 'abs'; d.style.cssText = css; d.innerHTML = html; document.body.appendChild(d); return d; }
