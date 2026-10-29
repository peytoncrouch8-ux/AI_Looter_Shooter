// Wardbreak (was Grave-salt) symbol and colour concepts (the user, 2026-10-29: "more concepts of what wardbreak symbols look
// like. The color scheme of this element needs adjusted as well"). Four glyphs (even-odd polygons in a 24 box, y down, ported
// as FPaintedIcon) and four colours, each shown on the card's rows. Run from the repo root:
//   node Docs/Archive/Work/2026-10-29-NewElements/wardbreak.js -> wardbreak.png, wardbreak.json
const fs = require('fs');
const path = require('path');
const here = __dirname;
const F = 'file:///C:/Dev/AI_Looter_Shooter/';
const C = { glass: '#1e2228', glassFoot: '#12151a', band: 'rgba(212,238,244,0.06)', label: '#b4bec8', text: '#f2f6f8', dim: '#98a4ae' };

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
const hex = (cx, cy, r, turn = 30) => Array.from({ length: 6 }, (_, i) => [cx + r * Math.cos(rad(turn + i * 60)), cy + r * Math.sin(rad(turn + i * 60))]);
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
// A ring with a gap (from a0 to a1 open), as a thick arc.
function arc(cx, cy, r, w, a0, a1, n = 30) {
	const o = ell(cx, cy, r + w / 2, r + w / 2, a0, a1, n), i = ell(cx, cy, r - w / 2, r - w / 2, a0, a1, n);
	return o.concat(i.reverse());
}
const shieldOutline = () => path3([12, 1.6], [[[17, 2.6], [20.4, 3], [21.2, 4.2]], [[21.6, 12.4], [18.4, 19.2], [12, 23]], [[5.6, 19.2], [2.4, 12.4], [2.8, 4.2]], [[3.6, 3], [7, 2.6], [12, 1.6]]]);

const GLYPHS = [
	{ name: 'Cracked Ward', idea: 'A ward plate split by a crack running down it.', fills: [
		shieldOutline(),
		// the crack: a jagged gap cut through the plate
		[[11.2, 1.6], [13.4, 1.6], [12.2, 7.4], [14.8, 10.6], [11.6, 14.2], [13.6, 18.6], [12, 23], [11.2, 18.4], [9.6, 14.4], [12.6, 10.8], [10.2, 7.6]]] },
	{ name: 'Broken Ring', idea: 'A ward ring bursting: a shard flung off, a salt grain where it broke.', fills: [
		arc(12, 13, 8.2, 3, 20, 330), arc(12, 13, 8.2, 3, 340, 365, 6).map((p) => [p[0] + 1.6, p[1] - 2.2]),
		[[19.4, 2.2], [22.6, 1.2], [21.8, 4.6]], diamond(12, 13, 2.2, 2.2)] },
	{ name: 'Shattered Pane', idea: 'A hexagonal ward in pieces, breaking apart.', fills: [
		[[12, 2], [20.2, 6.6], [19.6, 8.6], [12.4, 5.4], [11.6, 10.6], [5.4, 9], [3.8, 6.6]],
		[[3, 8.6], [10.2, 11.6], [9.4, 17.6], [3, 16.4]],
		[[12.8, 7.2], [20.8, 10.4], [21, 16.8], [13.4, 19.6], [11.2, 12.4]],
		[[4.6, 18.6], [9.8, 19.8], [12, 23], [12, 22]], [[14.6, 21.2], [19.6, 18.6], [13.6, 23.2]]] },
	{ name: 'Salt Sigil', idea: 'A salt circle with its line broken, the salt falling through.', fills: [
		arc(12, 12, 9.4, 2.4, -60, 250), stroke([[12, 12], [12, 2.6]], 1.6), stroke([[12, 12], [3.86, 16.7]], 1.6), stroke([[12, 12], [20.14, 16.7]], 1.6),
		diamond(17.6, 3.4, 1.1, 1.6), diamond(20.6, 7.2, 0.9, 1.3), diamond(14.8, 1.6, 0.8, 1.1)] },
];
const COLORS = [
	{ name: 'Teal', hex: '#4FD4C4', idea: 'Sea-glass teal: cold and salt, nothing else on the cards is teal.' },
	{ name: 'Violet', hex: '#B48CFF', idea: 'Spirit violet: the element that hunts spirits and wards.' },
	{ name: 'Rose', hex: '#F08CC8', idea: 'Salt-rose: pink salt, reads at once beside orange, gold, blue and green.' },
	{ name: 'Tide', hex: '#3FA7D6', idea: 'Deep tide blue: darker and bluer than Thunder\'s pale blue.' },
];
const CURRENT = '#CCF0F6';

const svgPath = (fills) => fills.map((P) => 'M' + P.map(([x, y]) => x.toFixed(2) + ' ' + y.toFixed(2)).join('L') + 'Z').join('');
const glyph = (fills, col, size) => `<svg width="${size}" height="${size}" viewBox="0 0 24 24"><path fill="${col}" fill-rule="evenodd" d="${svgPath(fills)}"/></svg>`;
const rows = (fills, col) => [['Salt dmg/sec', '1.8'], ['Chance to salt', '22%']].map(([l, v], r) =>
	`<div style="display:flex;align-items:center;height:27px;padding:0 9px;border-radius:6px;background:${r === 0 ? C.band : 'transparent'}">
		${glyph(fills, col, 18)}<span style="margin-left:8px;color:${col};font:700 16px Chakra">${l}</span>
		<span style="margin-left:auto;color:${col};font:700 19px Chakra">${v}</span></div>`).join('');
const card = (title, body, w = 420) => `<div style="width:${w}px;padding:14px 20px;box-sizing:border-box;border:2px solid rgba(212,238,244,.55);border-radius:10px;
	background:linear-gradient(${C.glass},${C.glassFoot});display:flex;flex-direction:column;align-items:center">
	<div style="font:700 20px Chakra;color:${C.text};letter-spacing:2px">${title}</div>${body}</div>`;

const glyphCell = (g, i) => card(`${i + 1} · ${g.name.toUpperCase()}`, `
	<div style="display:flex;align-items:center;gap:22px;margin:12px 0 8px">${glyph(g.fills, CURRENT, 140)}
		<div style="display:flex;flex-direction:column;gap:12px;align-items:center">${glyph(g.fills, CURRENT, 22)}${glyph(g.fills, CURRENT, 34)}</div></div>
	<div style="font:400 14px Chakra;color:${C.label};text-align:center;height:22px">${g.idea}</div>
	<div style="align-self:stretch;margin-top:8px">${rows(g.fills, CURRENT)}</div>`);
const colorCell = (c, i) => card(`${String.fromCharCode(65 + i)} · ${c.name.toUpperCase()}`, `
	<div style="display:flex;align-items:center;gap:16px;margin:10px 0 6px">
		<div style="width:70px;height:70px;border-radius:10px;background:${c.hex}"></div>${glyph(GLYPHS[0].fills, c.hex, 70)}${glyph(GLYPHS[1].fills, c.hex, 70)}</div>
	<div style="font:400 13px Chakra;color:${C.label};text-align:center;height:36px">${c.idea}<br>${c.hex}</div>
	<div style="align-self:stretch;margin-top:4px">${rows(GLYPHS[0].fills, c.hex)}</div>
	<div style="align-self:stretch;margin-top:6px;font:700 13px Chakra;color:${c.hex}">• Wardbreak: salts and slows them, wards crumble</div>`);

const swatches = `<div style="display:flex;gap:14px;align-items:center;font:700 14px Chakra;color:${C.dim};letter-spacing:1px">THE OTHERS:
	${[['CINDER', '#ff5e2e'], ['HALLOW', '#ffe6a6'], ['THUNDER', '#9fb4ff'], ['ROT', '#a8e05a'], ['WARDBREAK NOW', CURRENT]].map(([n, h]) =>
		`<span style="display:inline-flex;align-items:center;gap:6px;color:${h}"><span style="width:18px;height:18px;border-radius:4px;background:${h}"></span>${n}</span>`).join('')}</div>`;
const sheet = () => `<div style="font:700 26px Chakra;color:${CURRENT};letter-spacing:2px">WARDBREAK · SYMBOLS (shown in today's colour)</div>
	<div style="display:flex;gap:16px">${GLYPHS.map(glyphCell).join('')}</div>
	<div style="font:700 26px Chakra;color:${C.text};letter-spacing:2px;margin-top:10px">WARDBREAK · COLOURS</div>${swatches}
	<div style="display:flex;gap:16px">${COLORS.map(colorCell).join('')}</div>`;

const css = `@font-face{font-family:Chakra;src:url('${F}Content/UI/Fonts/ChakraPetch-Regular.ttf');font-weight:400}` +
	`@font-face{font-family:Chakra;src:url('${F}Content/UI/Fonts/ChakraPetch-Bold.ttf');font-weight:700}` +
	`body{margin:0;background:#07080a;width:1800px;padding:24px 0;display:flex;flex-direction:column;gap:16px;align-items:center}`;
(async () => {
	const r2 = (v) => Math.round(v * 100) / 100;
	fs.writeFileSync(path.join(here, 'wardbreak.json'), JSON.stringify({ glyphs: GLYPHS.map((g) => ({ name: g.name, fills: g.fills.map((P) => P.map(([x, y]) => [r2(x), r2(y)])) })), colors: COLORS }, null, 1));
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1800, height: 1000 } });
	const file = path.join(here, 'wardbreak.html');
	fs.writeFileSync(file, `<!doctype html><html><head><meta charset="utf-8"><style>${css}</style></head><body>${sheet()}</body></html>`);
	await pg.goto('file:///' + file.split(path.sep).join('/'));
	await pg.evaluate(() => document.fonts.ready);
	await pg.waitForTimeout(500);
	await pg.screenshot({ path: path.join(here, 'wardbreak.png'), fullPage: true });
	await browser.close();
	console.log('wardbreak.png, wardbreak.json');
})();
