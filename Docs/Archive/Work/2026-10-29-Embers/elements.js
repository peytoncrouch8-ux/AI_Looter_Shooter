// Element glyph concepts (2026-10-29): the user asked for four concept pictures for each element on the gun cards,
// Normal included. Every glyph is straight-segment polygons (curves sampled) in a 24-unit box, y down, filled even-odd
// (a polygon inside another cuts a hole), so Main ports the points to C++ as an FPaintedIcon, white for the tint.
// Run from the repo root: node Docs/Archive/Work/2026-10-29-Embers/elements.js -> elements.png, elements.json
const fs = require('fs');
const path = require('path');
const here = __dirname;
const F = 'file:///C:/Dev/AI_Looter_Shooter/';
const C = { glass: '#1e2228', glassFoot: '#12151a', band: 'rgba(212,238,244,0.06)', label: '#b4bec8', text: '#f2f6f8', line: '#d4eef4' };

// ------------------------------------------------------------------ geometry
const rad = (a) => a * Math.PI / 180;
function path3(start, segs, n = 10) {
	const out = [start];
	let p0 = start;
	for (const [p1, p2, p3] of segs) {
		for (let i = 1; i <= n; i++) {
			const t = i / n, u = 1 - t;
			out.push([u * u * u * p0[0] + 3 * u * u * t * p1[0] + 3 * u * t * t * p2[0] + t * t * t * p3[0],
				u * u * u * p0[1] + 3 * u * u * t * p1[1] + 3 * u * t * t * p2[1] + t * t * t * p3[1]]);
		}
		p0 = p3;
	}
	const last = out[out.length - 1];
	if (Math.abs(last[0] - start[0]) < 1e-6 && Math.abs(last[1] - start[1]) < 1e-6) out.pop();
	return out;
}
function ell(cx, cy, rx, ry, a0 = 0, a1 = 360, n = 36) {
	const out = [];
	const full = Math.abs(a1 - a0) >= 360;
	for (let i = 0; i < (full ? n : n + 1); i++) { const a = rad(a0 + (a1 - a0) * i / n); out.push([cx + rx * Math.cos(a), cy + ry * Math.sin(a)]); }
	return out;
}
const circle = (cx, cy, r, n = 36) => ell(cx, cy, r, r, 0, 360, n);
const diamond = (cx, cy, rx, ry) => [[cx, cy - ry], [cx + rx, cy], [cx, cy + ry], [cx - rx, cy]];
// A star of N points, outer radius R and inner r, first point straight up.
function star(cx, cy, N, R, r, turn = 0) {
	const out = [];
	for (let i = 0; i < N * 2; i++) { const a = rad(-90 + turn + i * 180 / N); const d = i % 2 ? r : R; out.push([cx + d * Math.cos(a), cy + d * Math.sin(a)]); }
	return out;
}
// A flame-like drop: bottom middle at (cx, by), w wide, h tall, its tip leaning by lean.
function drop(cx, by, w, h, lean = 0) {
	const r = w / 2;
	return path3([cx, by], [
		[[cx - r * 1.38, by], [cx - r * 1.0, by - h * 0.5], [cx + lean, by - h]],
		[[cx + r * 1.0, by - h * 0.5], [cx + r * 1.38, by], [cx, by]]]);
}
// A ray from radius r0 to r1 at angle a (deg, 0 = right), w wide at its foot.
function ray(cx, cy, a, r0, r1, w) {
	const d = [Math.cos(rad(a)), Math.sin(rad(a))], n = [-d[1], d[0]];
	return [[cx + d[0] * r0 + n[0] * w / 2, cy + d[1] * r0 + n[1] * w / 2], [cx + d[0] * r1, cy + d[1] * r1], [cx + d[0] * r0 - n[0] * w / 2, cy + d[1] * r0 - n[1] * w / 2]];
}
const rot = (P, cx, cy, deg) => P.map(([x, y]) => { const c = Math.cos(rad(deg)), s = Math.sin(rad(deg)); return [cx + (x - cx) * c - (y - cy) * s, cy + (x - cx) * s + (y - cy) * c]; });

// ------------------------------------------------------------------ the glyphs
const HEARTH = [
	path3([12, 23], [[[6.6, 23], [3.6, 19.6], [4.2, 15.2]], [[4.8, 11.2], [8.6, 9.4], [9.4, 5.4]], [[9.8, 3.6], [9.4, 2.2], [8.6, 1]],
		[[14.2, 2.4], [20, 8.2], [20, 15.2]], [[20, 19.8], [16.8, 23], [12, 23]]]),
	path3([12, 21], [[[9.4, 21], [8, 19.2], [8.4, 17.2]], [[8.8, 15.2], [10.8, 14.2], [11.6, 11.4]], [[13.6, 13.2], [16, 15.2], [15.8, 17.6]],
		[[15.6, 19.8], [14, 21], [12, 21]]]),
];
const BONFIRE = [path3([12, 23], [[[7, 23], [3.6, 19.8], [3.8, 15.4]], [[4, 12.2], [4.8, 9.8], [5.6, 7]], [[6.8, 9.4], [7.8, 10.8], [8.8, 11.6]],
	[[8.6, 7.6], [10.6, 4.2], [12.8, 0.8]], [[13.8, 4.4], [15, 7.8], [15.2, 10.6]], [[16.6, 9.6], [17.8, 8.2], [18.6, 6.2]],
	[[20.2, 9.4], [20.4, 12.6], [20.2, 15.4]], [[20, 19.8], [16.8, 23], [12, 23]]])];

const ELEMENTS = [
	{
		key: 'Ember', name: 'EMBER', color: '#ff5e2e', rows: [['Burn dmg/sec', '3.9'], ['Chance to burn', '22%']],
		options: [
			{ name: 'Hearth', idea: 'Your pick: a classic flame, a flame cut from its heart.', fills: HEARTH },
			{ name: 'Bonfire', idea: 'A solid blaze of three curved licks.', fills: BONFIRE },
			{ name: 'Torch', idea: 'A frontier torch: flame in an iron cup on a handle.', fills: [
				path3([12, 13.8], [[[8.4, 13.8], [6.8, 11.4], [7.4, 8.6]], [[8, 5.8], [10.4, 4.6], [11.2, 0.8]], [[13.8, 2.6], [17, 5.4], [16.8, 9]], [[16.6, 12.2], [15, 13.8], [12, 13.8]]]),
				[[6.6, 15], [17.4, 15], [15.8, 17.8], [8.2, 17.8]],
				[[10.5, 18.6], [13.5, 18.6], [12.9, 23.4], [11.1, 23.4]]] },
			{ name: 'Live Coal', idea: 'A cracked coal glowing, three small flames rising off it.', fills: [
				ell(12, 18.2, 9, 4.8, 0, 360, 40),
				[[5.4, 17.6], [9, 16.4], [11.6, 18.6], [15, 16.6], [18.6, 17.8], [15.2, 17.8], [11.8, 20.2], [8.8, 17.8]],
				drop(6.6, 12, 3.6, 6.4, -0.4), drop(12, 11.4, 4.6, 10.4, 0.6), drop(17.4, 12, 3.6, 6.4, 0.4)] },
		],
	},
	{
		key: 'SoulLight', name: 'SOUL-LIGHT', color: '#ffe6a6', rows: [['Smite damage', '17'], ['Chance to smite', '22%']],
		options: [
			{ name: 'Guiding Star', idea: 'Today\'s glyph: a four-point star of holy light.', fills: [star(12, 12, 4, 11.5, 2.6)] },
			{ name: 'Haloed Soul', idea: 'A soul\'s flame under a saint\'s halo.', fills: [
				drop(12, 22.6, 9, 14.6, 0), ell(12, 3.4, 6.4, 2.4), ell(12, 3.4, 4.6, 1.1)] },
			{ name: 'Sunburst', idea: 'A bright core throwing eight rays.', fills: [
				circle(12, 12, 4.2), ...[0, 45, 90, 135, 180, 225, 270, 315].map((a) => ray(12, 12, a, 6, a % 90 ? 10 : 11.6, a % 90 ? 2.2 : 2.8))] },
			{ name: 'Saint\'s Lantern', idea: 'A lantern with a soul-flame behind its glass.', fills: [
				ell(12, 2.8, 2.2, 2.2), ell(12, 2.8, 1.1, 1.1),
				[[9, 5.2], [15, 5.2], [18, 8], [6, 8]],
				[[7, 8], [17, 8], [17, 19], [7, 19]], [[9, 10], [15, 10], [15, 17], [9, 17]], drop(12, 16.4, 3.6, 5.8, 0),
				[[6, 19], [18, 19], [17, 21.4], [7, 21.4]]] },
		],
	},
	{
		key: 'GraveSalt', name: 'GRAVE-SALT', color: '#ccf0f6', rows: [['Salt dmg/sec', '1.8'], ['Chance to salt', '22%']],
		options: [
			{ name: 'Salt Diamond', idea: 'Today\'s glyph: a crystal\'s outline round a solid heart.', fills: [
				diamond(12, 12, 11, 11), diamond(12, 12, 8.2, 8.2), diamond(12, 12, 4.8, 4.8)] },
			{ name: 'Salt Cluster', idea: 'Three salt crystals growing up from the ground.', fills: [
				[[10, 22.4], [10, 7], [12, 2.6], [14, 7], [14, 22.4]],
				[[4.6, 22.4], [4, 13.4], [5.8, 10.2], [8.2, 12.6], [8.8, 22.4]],
				[[15.2, 22.4], [15.8, 11.6], [18.2, 8.8], [20, 12.4], [19.4, 22.4]]] },
			{ name: 'Salt Line', idea: 'A heap of salt poured from above, grains still falling.', fills: [
				path3([2.4, 22.6], [[[4.4, 18], [8, 14.4], [12, 14.2]], [[16, 14.4], [19.6, 18], [21.6, 22.6]]]),
				rot(diamond(12, 2.2, 1, 1), 12, 2.2, 0), diamond(11.2, 5.6, 1.1, 1.1), diamond(12.8, 8.8, 1.1, 1.1), diamond(11.6, 11.8, 1, 1),
				diamond(7.4, 9.6, 0.8, 0.8), diamond(16.6, 10.6, 0.8, 0.8)] },
			{ name: 'Salted Stone', idea: 'A grave\'s headstone with a salt crystal set in it.', fills: [
				[[5, 23], ...ell(12, 10, 7, 7, 180, 360, 24), [19, 23]],
				diamond(12, 13.6, 3.8, 5.2), diamond(12, 13.6, 1.8, 2.6)] },
		],
	},
	{
		key: 'Normal', name: 'NORMAL', color: '#98a4ae', rows: [['Element', 'Normal']], valueColor: '#f2f6f8', labelColor: '#b4bec8',
		options: [
			{ name: 'Plain Diamond', idea: 'Today\'s glyph: the card\'s diamond.', fills: [
				diamond(12, 12, 10, 10), diamond(12, 12, 7.4, 7.4), diamond(12, 12, 4.4, 4.4)] },
			{ name: 'Cartridge', idea: 'A plain round: lead and brass, nothing more.', fills: [
				path3([9, 9.4], [[[9, 5.8], [10.4, 2.8], [12, 1.4]], [[13.6, 2.8], [15, 5.8], [15, 9.4]]]),
				[[9, 10.4], [15, 10.4], [15, 20.6], [9, 20.6]], [[8.2, 21.2], [15.8, 21.2], [15.8, 23.2], [8.2, 23.2]]] },
			{ name: 'Empty Ring', idea: 'A hollow ring: no element loaded.', fills: [circle(12, 12, 9.4, 40), circle(12, 12, 6.8, 40)] },
			{ name: 'Hex Bolt', idea: 'A plain steel bolt head: hard iron, no tricks.', fills: [
				star(12, 12, 3, 10, 10, 0).map((p, i, a) => p), circle(12, 12, 3.8)] },
		],
	},
];
// The hex bolt: a hexagon (six corners of a 3-point star with equal radii).
ELEMENTS[3].options[3].fills[0] = Array.from({ length: 6 }, (_, i) => [12 + 10 * Math.cos(rad(30 + i * 60)), 12 + 10 * Math.sin(rad(30 + i * 60))]);

// ------------------------------------------------------------------ the sheet
const svgPath = (fills) => fills.map((P) => 'M' + P.map(([x, y]) => x.toFixed(2) + ' ' + y.toFixed(2)).join('L') + 'Z').join('');
const glyph = (fills, col, size) => `<svg width="${size}" height="${size}" viewBox="0 0 24 24"><path fill="${col}" fill-rule="evenodd" d="${svgPath(fills)}"/></svg>`;

function cell(el, o, i) {
	const rows = el.rows.map(([l, v], r) =>
		`<div style="display:flex;align-items:center;height:27px;padding:0 9px;border-radius:6px;background:${r === 0 ? C.band : 'transparent'}">
			${glyph(o.fills, el.color, 18)}<span style="margin-left:8px;color:${el.labelColor || el.color};font:700 16px Chakra">${l}</span>
			<span style="margin-left:auto;color:${el.valueColor || el.color};font:700 19px Chakra">${v}</span></div>`).join('');
	return `<div style="width:400px;height:342px;padding:14px 20px;box-sizing:border-box;border:2px solid rgba(212,238,244,.55);border-radius:10px;
		background:linear-gradient(${C.glass},${C.glassFoot});display:flex;flex-direction:column;align-items:center">
		<div style="font:700 20px Chakra;color:${C.text};letter-spacing:2px">${i + 1} · ${o.name.toUpperCase()}</div>
		<div style="display:flex;align-items:center;gap:22px;margin:12px 0 8px">${glyph(o.fills, el.color, 140)}
			<div style="display:flex;flex-direction:column;gap:12px;align-items:center">${glyph(o.fills, el.color, 22)}${glyph(o.fills, el.color, 34)}</div></div>
		<div style="font:400 14px Chakra;color:${C.label};text-align:center;height:22px">${o.idea}</div>
		<div style="align-self:stretch;margin-top:8px">${rows}</div></div>`;
}
const sheet = () => ELEMENTS.map((el) => `<div style="display:flex;align-items:center;gap:20px">
	<div style="width:140px;font:700 24px Chakra;color:${el.color};letter-spacing:2px;text-align:right">${el.name}</div>
	${el.options.map((o, i) => cell(el, o, i)).join('')}</div>`).join('');

const css = `@font-face{font-family:Chakra;src:url('${F}Content/UI/Fonts/ChakraPetch-Regular.ttf');font-weight:400}` +
	`@font-face{font-family:Chakra;src:url('${F}Content/UI/Fonts/ChakraPetch-Bold.ttf');font-weight:700}` +
	`body{margin:0;background:#07080a;width:1920px;padding:24px 0;display:flex;flex-direction:column;gap:20px;align-items:center}`;
(async () => {
	const r2 = (v) => Math.round(v * 100) / 100;
	fs.writeFileSync(path.join(here, 'elements.json'), JSON.stringify(ELEMENTS.map((el) => ({ key: el.key,
		options: el.options.map((o) => ({ name: o.name, fills: o.fills.map((P) => P.map(([x, y]) => [r2(x), r2(y)])) })) })), null, 1));
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1500 } });
	const file = path.join(here, 'elements.html');
	fs.writeFileSync(file, `<!doctype html><html><head><meta charset="utf-8"><style>${css}</style></head><body>${sheet()}</body></html>`);
	await pg.goto('file:///' + file.split(path.sep).join('/'));
	await pg.evaluate(() => document.fonts.ready);
	await pg.waitForTimeout(500);
	await pg.screenshot({ path: path.join(here, 'elements.png'), fullPage: true });
	await browser.close();
	console.log('elements.png, elements.json');
})();
