// Ember glyph options (2026-10-29): the user found the three-tongue glyph "doesn't look like a flame" and asked for three
// to choose from. Each is straight-segment polygons (curves sampled) in a 24-unit box, y down, filled even-odd, so Main
// ports the points to C++ as an FPaintedIcon. Run from the repo root: node Docs/Archive/Work/2026-10-29-Embers/make.js -> embers.png, embers.json
const fs = require('fs');
const path = require('path');
const here = __dirname;
const F = 'file:///C:/Dev/AI_Looter_Shooter/';
const C = { ember: '#ff5e2e', glass: '#1e2228', glassFoot: '#12151a', band: 'rgba(255,255,255,0.05)', label: '#b4bec8', text: '#f2f6f8', line: '#d4eef4' };

// A path of cubic segments from a start point: [p1, p2, p3] each.
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
	out.pop(); // the last point repeats the first
	return out;
}
const diamond = (cx, cy, rx, ry) => [[cx, cy - ry], [cx + rx, cy], [cx, cy + ry], [cx - rx, cy]];

const OPTIONS = [];

// 1 · HEARTH: the classic flame, a round belly rising to one tip that curls over, with a small flame cut out of its heart.
OPTIONS.push({
	key: 'Hearth', name: 'HEARTH', idea: 'The classic flame: a round belly, one curling tip, a flame cut from its heart.',
	fills: [
		path3([12, 23], [
			[[6.6, 23], [3.6, 19.6], [4.2, 15.2]],
			[[4.8, 11.2], [8.6, 9.4], [9.4, 5.4]],
			[[9.8, 3.6], [9.4, 2.2], [8.6, 1]],
			[[14.2, 2.4], [20, 8.2], [20, 15.2]],
			[[20, 19.8], [16.8, 23], [12, 23]]]),
		path3([12, 21], [
			[[9.4, 21], [8, 19.2], [8.4, 17.2]],
			[[8.8, 15.2], [10.8, 14.2], [11.6, 11.4]],
			[[13.6, 13.2], [16, 15.2], [15.8, 17.6]],
			[[15.6, 19.8], [14, 21], [12, 21]]]),
	],
});

// 2 · BONFIRE: one solid fire of three licks, the tall middle one leaning, curved sides and deep curved notches.
OPTIONS.push({
	key: 'Bonfire', name: 'BONFIRE', idea: 'A solid campfire blaze: three curved licks, the middle one tallest.',
	fills: [
		path3([12, 23], [
			[[7, 23], [3.6, 19.8], [3.8, 15.4]],
			[[4, 12.2], [4.8, 9.8], [5.6, 7]],
			[[6.8, 9.4], [7.8, 10.8], [8.8, 11.6]],
			[[8.6, 7.6], [10.6, 4.2], [12.8, 0.8]],
			[[13.8, 4.4], [15, 7.8], [15.2, 10.6]],
			[[16.6, 9.6], [17.8, 8.2], [18.6, 6.2]],
			[[20.2, 9.4], [20.4, 12.6], [20.2, 15.4]],
			[[20, 19.8], [16.8, 23], [12, 23]]]),
	],
});

// 3 · EMBER: a lean flame flickering up from a glowing coal, two sparks lifting off it.
OPTIONS.push({
	key: 'Ember', name: 'EMBER', idea: 'A flame with a side lick off a glowing coal, two sparks lifting away.',
	fills: [
		path3([11.6, 18.4], [
			[[8, 18.4], [5.8, 16.2], [6, 12.8]],
			[[6.1, 11], [6.6, 9.6], [7.2, 8.2]],
			[[8, 9.6], [8.8, 10.4], [9.6, 10.8]],
			[[9.8, 7.2], [11.4, 4.4], [12.4, 1.2]],
			[[16, 4], [17.6, 8.6], [17.2, 12.6]],
			[[16.8, 16.2], [14.8, 18.4], [11.6, 18.4]]]),
		// the coal: a wide low lozenge under the flame, a gap between them
		[[5, 21.6], [7.4, 19.8], [16.6, 19.8], [19, 21.6], [16.6, 23.4], [7.4, 23.4]],
		diamond(18.6, 4.6, 1.3, 1.9),
		diamond(5, 7.4, 1, 1.4),
	],
});

const svgPath = (fills) => fills.map((P) => 'M' + P.map(([x, y]) => x.toFixed(2) + ' ' + y.toFixed(2)).join('L') + 'Z').join('');
const glyph = (o, size) => `<svg width="${size}" height="${size}" viewBox="0 0 24 24"><path fill="${C.ember}" fill-rule="evenodd" d="${svgPath(o.fills)}"/></svg>`;

function card(o, i) {
	const rows = [['Burn dmg/sec', '3.9'], ['Chance to burn', '22%']].map(([l, v], r) =>
		`<div style="display:flex;align-items:center;height:30px;padding:0 10px;border-radius:6px;background:${r === 0 ? C.band : 'transparent'}">
			${glyph(o, 18)}<span style="margin-left:9px;color:${C.ember};font:700 18px Chakra;letter-spacing:.5px">${l}</span>
			<span style="margin-left:auto;color:${C.ember};font:700 21px Chakra">${v}</span></div>`).join('');
	return `<div style="width:560px;padding:28px 32px;box-sizing:border-box;border:3px solid ${C.line};border-radius:10px;
		background:linear-gradient(${C.glass},${C.glassFoot});display:flex;flex-direction:column;align-items:center">
		<div style="font:700 30px Chakra;color:${C.text};letter-spacing:3px">${i + 1} · ${o.name}</div>
		<div style="margin:22px 0">${glyph(o, 230)}</div>
		<div style="font:400 17px Chakra;color:${C.label};text-align:center;height:46px">${o.idea}</div>
		<div style="align-self:stretch;margin-top:10px">${rows}</div>
		<div style="display:flex;align-items:center;gap:18px;margin-top:16px">${glyph(o, 22)}${glyph(o, 32)}${glyph(o, 48)}</div>
	</div>`;
}

const css = `@font-face{font-family:Chakra;src:url('${F}Content/UI/Fonts/ChakraPetch-Regular.ttf');font-weight:400}` +
	`@font-face{font-family:Chakra;src:url('${F}Content/UI/Fonts/ChakraPetch-Bold.ttf');font-weight:700}` +
	`body{margin:0;background:#07080a;width:1860px;height:900px;display:flex;gap:40px;align-items:center;justify-content:center}`;
(async () => {
	fs.writeFileSync(path.join(here, 'embers.json'), JSON.stringify(OPTIONS.map((o) => ({ key: o.key, fills: o.fills.map((P) => P.map(([x, y]) => [Math.round(x * 100) / 100, Math.round(y * 100) / 100])) })), null, 1));
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1860, height: 900 } });
	const file = path.join(here, 'embers.html');
	fs.writeFileSync(file, `<!doctype html><html><head><meta charset="utf-8"><style>${css}</style></head><body>${OPTIONS.map(card).join('')}</body></html>`);
	await pg.goto('file:///' + file.split(path.sep).join('/'));
	await pg.evaluate(() => document.fonts.ready);
	await pg.waitForTimeout(500);
	await pg.screenshot({ path: path.join(here, 'embers.png') });
	await browser.close();
	console.log('embers.png, embers.json');
})();
