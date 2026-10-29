// Two new elements (the user, 2026-10-29: "add 2 other elements ... concept art of their pictures and name options").
// Four candidate elements, each with what it does, four glyph concepts and four name options; the user picks two elements,
// then a glyph and a name for each. Glyphs are even-odd polygons in a 24-unit box (y down), ported to C++ as FPaintedIcon.
// Run from the repo root: node Docs/Work/2026-10-29-NewElements/make.js -> elements.png, elements.json
const fs = require('fs');
const path = require('path');
const here = __dirname;
const F = 'file:///C:/Dev/AI_Looter_Shooter/';
const C = { glass: '#1e2228', glassFoot: '#12151a', band: 'rgba(212,238,244,0.06)', label: '#b4bec8', text: '#f2f6f8', line: '#d4eef4', dim: '#98a4ae' };

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
const rot = (P, cx, cy, deg) => P.map(([x, y]) => { const c = Math.cos(rad(deg)), s = Math.sin(rad(deg)); return [cx + (x - cx) * c - (y - cy) * s, cy + (x - cx) * s + (y - cy) * c]; });
// A thick polyline as a polygon (w wide), mitred simply by offsetting each segment.
function stroke(P, w) {
	const L = [], R = [];
	for (let i = 0; i < P.length; i++) {
		const a = P[Math.max(i - 1, 0)], b = P[Math.min(i + 1, P.length - 1)];
		const dx = b[0] - a[0], dy = b[1] - a[1], len = Math.hypot(dx, dy) || 1;
		const nx = -dy / len * w / 2, ny = dx / len * w / 2;
		L.push([P[i][0] + nx, P[i][1] + ny]); R.push([P[i][0] - nx, P[i][1] - ny]);
	}
	return L.concat(R.reverse());
}
const drop = (cx, by, w, h, lean = 0) => { const r = w / 2; return path3([cx, by], [[[cx - r * 1.38, by], [cx - r, by - h * 0.5], [cx + lean, by - h]], [[cx + r, by - h * 0.5], [cx + r * 1.38, by], [cx, by]]]); };
function star(cx, cy, N, R, r, turn = 0) {
	const out = [];
	for (let i = 0; i < N * 2; i++) { const a = rad(-90 + turn + i * 180 / N); const d = i % 2 ? r : R; out.push([cx + d * Math.cos(a), cy + d * Math.sin(a)]); }
	return out;
}
const hex = (cx, cy, r) => Array.from({ length: 6 }, (_, i) => [cx + r * Math.cos(rad(30 + i * 60)), cy + r * Math.sin(rad(30 + i * 60))]);

// ------------------------------------------------------------------ the candidates
const ELEMENTS = [
	{
		key: 'Shock', name: 'SHOCK', color: '#9fb4ff', verb: 'jolt',
		what: 'A jolt that staggers and arcs to the next living thing. Weak: slimes, the living. Resists: the dead.',
		rows: [['Jolt damage', '14'], ['Chance to jolt', '22%']],
		names: ['Spark', 'Jolt', 'Static', 'Thunderhead'],
		options: [
			{ name: 'Bolt', idea: 'The classic lightning bolt.', fills: [[[13.6, 0.8], [5.2, 13.4], [11, 13.4], [8.8, 23.2], [18.8, 9.6], [12.8, 9.6]]] },
			{ name: 'Arc', idea: 'A spark jumping between two poles.', fills: [
				[[2.4, 20], [6.4, 20], [6.4, 23.2], [2.4, 23.2]], [[17.6, 20], [21.6, 20], [21.6, 23.2], [17.6, 23.2]],
				stroke([[4.4, 20], [4.4, 14], [8.2, 10.4], [11.4, 13.6], [14.4, 7.2], [17, 11.8], [19.6, 14], [19.6, 20]], 2.2),
				diamond(14.4, 3.4, 1.2, 2.2)] },
			{ name: 'Storm Cloud', idea: 'A thunderhead with a bolt dropping out of it.', fills: [
				path3([3.2, 13], [[[1, 13], [1, 8.4], [4.6, 8.2]], [[5, 4.4], [9.4, 2.4], [12.4, 4.8]], [[15, 2.6], [20, 4.6], [19.6, 8.4]], [[23, 8.6], [23, 13], [20.4, 13]]]),
				[[13.2, 13], [9, 18.6], [12.2, 18.6], [10.6, 23.4], [16, 16.4], [13, 16.4], [15.4, 13]]] },
			{ name: 'Coil', idea: 'A charged coil throwing sparks.', fills: [
				circle(12, 13, 8.4), circle(12, 13, 5.6), circle(12, 13, 2.4),
				...[-90, -30, 30, 90, 150, 210].map((a) => rot([[12, 2.2], [13.3, 4.4], [12, 6.2], [10.7, 4.4]], 12, 13, a + 90))] },
		],
	},
	{
		key: 'Blight', name: 'BLIGHT', color: '#a8e05a', verb: 'rot',
		what: 'A creeping rot: damage that grows, and spreads to whoever stands near the dying. Weak: spiders, the living. Resists: slimes.',
		rows: [['Rot dmg/sec', '2.4'], ['Chance to rot', '22%']],
		names: ['Blight', 'Rot', 'Gall', 'Miasma'],
		options: [
			{ name: 'Drip', idea: 'Three thick drips of poison.', fills: [drop(12, 15, 8, 13.6), drop(5, 21, 4, 6.4), drop(19, 21, 4, 6.4)] },
			{ name: 'Skull', idea: 'A small skull, the old poison mark.', fills: [
				path3([5, 14], [[[5, 8], [8, 3.2], [12, 3.2]], [[16, 3.2], [19, 8], [19, 14]], [[19, 16], [17.4, 17.4], [16, 17.4]], [[16, 19], [16, 20.6], [15, 20.6]],
					[[14, 20.6], [10, 20.6], [9, 20.6]], [[8, 20.6], [8, 19], [8, 17.4]], [[6.6, 17.4], [5, 16], [5, 14]]]),
				circle(9.2, 12, 2.2), circle(14.8, 12, 2.2), [[12, 14.4], [13.4, 17], [10.6, 17]]] },
			{ name: 'Spore', idea: 'A bursting spore pod, dust in the air.', fills: [
				circle(12, 14, 7.2), circle(12, 14, 4.2), ...[-110, -70, -30, 20, 200, 160].map((a) => circle(12 + 10.2 * Math.cos(rad(a)), 14 + 10.2 * Math.sin(rad(a)), 1.4, 12))] },
			{ name: 'Fang', idea: 'A viper\'s two fangs dripping.', fills: [
				[[4, 2], [10, 2], [8.2, 14], [6.2, 14]], [[14, 2], [20, 2], [17.8, 14], [15.8, 14]], drop(7.2, 21.6, 3.2, 5.4), drop(16.8, 21.6, 3.2, 5.4)] },
		],
	},
	{
		key: 'Hex', name: 'HEX', color: '#c89aff', verb: 'mark',
		what: 'A brand: the marked take more from everything for a while, and a mark that ends bursts. Weak: the risen. Resists: spirits.',
		rows: [['Mark bonus', '+30%'], ['Chance to mark', '22%']],
		names: ['Hex', 'Bane', 'Brand', 'Mark'],
		options: [
			{ name: 'Evil Eye', idea: 'An open eye, the hex\'s stare.', fills: [
				path3([1.6, 12], [[[6, 4.6], [18, 4.6], [22.4, 12]], [[18, 19.4], [6, 19.4], [1.6, 12]]]), circle(12, 12, 4.6), circle(12, 12, 2)] },
			{ name: 'Hex Sign', idea: 'A hexagram in a ring.', fills: [circle(12, 12, 10.8), circle(12, 12, 9), star(12, 12, 6, 8, 4.6), star(12, 12, 6, 4.4, 2.5)] },
			{ name: 'Brand', idea: 'A branding iron\'s mark: a crossed X in a ring.', fills: [
				circle(12, 12, 10.4), circle(12, 12, 8.2), stroke([[7.2, 7.2], [16.8, 16.8]], 2.6), stroke([[16.8, 7.2], [7.2, 16.8]], 2.6)] },
			{ name: 'Pins', idea: 'A poppet with pins in it.', fills: [
				circle(12, 5, 3.2), [[8.2, 8.4], [15.8, 8.4], [17, 16], [14.6, 16], [14.2, 22.6], [9.8, 22.6], [9.4, 16], [7, 16]],
				stroke([[3, 9.6], [10.4, 13.4]], 1.2), stroke([[21, 9], [13.4, 13.8]], 1.2), stroke([[4.2, 20], [10, 17]], 1.2)] },
		],
	},
	{
		key: 'Blast', name: 'BLAST', color: '#ffc46b', verb: 'blast',
		what: 'Black powder: a hit may burst, hurting everything near it and knocking the small off their feet. Weak: slimes, chitin. Resists: nothing (no spread on bosses).',
		rows: [['Blast damage', '20'], ['Chance to blast', '14%']],
		names: ['Blast', 'Powder', 'Thunder', 'Shrapnel'],
		options: [
			{ name: 'Burst', idea: 'A starburst with a hollow heart.', fills: [star(12, 12, 8, 11.2, 5.4), circle(12, 12, 2.6)] },
			{ name: 'Keg', idea: 'A powder keg, fuse lit.', fills: [
				path3([5, 8.4], [[[5, 6.2], [19, 6.2], [19, 8.4]], [[20.2, 14], [20.2, 17], [19, 22.4]], [[19, 23.2], [5, 23.2], [5, 22.4]], [[3.8, 17], [3.8, 14], [5, 8.4]]]),
				[[5.6, 13.6], [18.4, 13.6], [18.4, 15.2], [5.6, 15.2]], stroke([[12, 6.2], [12.6, 3.8], [15, 2.6]], 1.4), star(16.6, 2, 5, 2.2, 1, 0)] },
			{ name: 'Round Bomb', idea: 'A ball bomb with a sparking fuse.', fills: [
				circle(11, 14.4, 8.4), [[8.4, 6.6], [13.6, 6.6], [13.6, 8.6], [8.4, 8.6]], stroke([[11, 6.6], [12.4, 3.8], [15.6, 2.6]], 1.3), star(18.2, 2.6, 4, 2.6, 1, 45)] },
			{ name: 'Shrapnel', idea: 'A burst of flying shards.', fills: [
				circle(12, 12, 3.4), ...[0, 51, 103, 154, 206, 257, 309].map((a, i) => rot([[11, 2.6 + (i % 2) * 1.8], [13, 2.6 + (i % 2) * 1.8], [12.6, 7.4], [11.4, 7.4]], 12, 12, a))] },
		],
	},
];

// ------------------------------------------------------------------ the sheet
const svgPath = (fills) => fills.map((P) => 'M' + P.map(([x, y]) => x.toFixed(2) + ' ' + y.toFixed(2)).join('L') + 'Z').join('');
const glyph = (fills, col, size) => `<svg width="${size}" height="${size}" viewBox="0 0 24 24"><path fill="${col}" fill-rule="evenodd" d="${svgPath(fills)}"/></svg>`;

function cell(el, o, i) {
	const rows = el.rows.map(([l, v], r) =>
		`<div style="display:flex;align-items:center;height:26px;padding:0 9px;border-radius:6px;background:${r === 0 ? C.band : 'transparent'}">
			${glyph(o.fills, el.color, 18)}<span style="margin-left:8px;color:${el.color};font:700 15px Chakra">${l}</span>
			<span style="margin-left:auto;color:${el.color};font:700 18px Chakra">${v}</span></div>`).join('');
	return `<div style="width:300px;height:300px;padding:12px 16px;box-sizing:border-box;border:2px solid rgba(212,238,244,.55);border-radius:10px;
		background:linear-gradient(${C.glass},${C.glassFoot});display:flex;flex-direction:column;align-items:center">
		<div style="font:700 18px Chakra;color:${C.text};letter-spacing:2px">${i + 1} · ${o.name.toUpperCase()}</div>
		<div style="display:flex;align-items:center;gap:18px;margin:8px 0 6px">${glyph(o.fills, el.color, 118)}
			<div style="display:flex;flex-direction:column;gap:10px;align-items:center">${glyph(o.fills, el.color, 22)}${glyph(o.fills, el.color, 34)}</div></div>
		<div style="font:400 13px Chakra;color:${C.label};text-align:center;height:20px">${o.idea}</div>
		<div style="align-self:stretch;margin-top:6px">${rows}</div></div>`;
}
const sheet = () => ELEMENTS.map((el, k) => `<div style="display:flex;align-items:center;gap:16px">
	<div style="width:330px;display:flex;flex-direction:column;gap:8px">
		<div style="font:700 26px Chakra;color:${el.color};letter-spacing:2px">${String.fromCharCode(65 + k)} · ${el.name}</div>
		<div style="font:400 15px Chakra;color:${C.label};line-height:1.35">${el.what}</div>
		<div style="font:700 13px Chakra;color:${C.dim};letter-spacing:1px;margin-top:4px">NAME OPTIONS</div>
		<div style="font:700 18px Chakra;color:${C.text}">${el.names.map((n, i) => `${i + 1} ${n}`).join(' &nbsp;·&nbsp; ')}</div>
	</div>
	${el.options.map((o, i) => cell(el, o, i)).join('')}</div>`).join('');

const css = `@font-face{font-family:Chakra;src:url('${F}Content/UI/Fonts/ChakraPetch-Regular.ttf');font-weight:400}` +
	`@font-face{font-family:Chakra;src:url('${F}Content/UI/Fonts/ChakraPetch-Bold.ttf');font-weight:700}` +
	`body{margin:0;background:#07080a;width:1920px;padding:20px 0;display:flex;flex-direction:column;gap:18px;align-items:center}`;
(async () => {
	const r2 = (v) => Math.round(v * 100) / 100;
	fs.writeFileSync(path.join(here, 'elements.json'), JSON.stringify(ELEMENTS.map((el) => ({ key: el.key, color: el.color, names: el.names,
		options: el.options.map((o) => ({ name: o.name, fills: o.fills.map((P) => P.map(([x, y]) => [r2(x), r2(y)])) })) })), null, 1));
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1320 } });
	const file = path.join(here, 'elements.html');
	fs.writeFileSync(file, `<!doctype html><html><head><meta charset="utf-8"><style>${css}</style></head><body>${sheet()}</body></html>`);
	await pg.goto('file:///' + file.split(path.sep).join('/'));
	await pg.evaluate(() => document.fonts.ready);
	await pg.waitForTimeout(500);
	await pg.screenshot({ path: path.join(here, 'elements.png'), fullPage: true });
	await browser.close();
	console.log('elements.png, elements.json');
})();
