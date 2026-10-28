// Ranchhand (shotgun) part concept views for Tripo's multi-view image-to-3D (2026-10-28).
//   node Art/Concepts/Weapons/Shotgun/render.js [Folder... | set1 set2 set3 set4]     upload/ and upload_white/ images
//   node Art/Concepts/Weapons/Shotgun/render.js --sheet                                 compare_bodies.png, compare_sets.png
// Four sets, one maker each, every part a new design (never another set recoloured); only the interfaces are shared
// (Art/Models/Weapons/Ranchhand.py: sockets, joint sizes), so any set's part fits any other's. The parts live in
// parts_set1.js (Saint Maud) .. parts_set4.js (Meridian); the drawing kit in lib.js.
// Views: front = from the muzzle end, back = from the butt end, left = the gun's left side (muzzle to the image's left),
// right = its right side (muzzle to the right). Each part fills ~84% of a 1024 px frame, one scale across its views.
const { fs, path, chromium, N, makeView, defs, defs4 } = require('./lib');

const SETS = [1, 2, 3, 4].map(n => {
	const f = path.join(__dirname, `parts_set${n}.js`);
	if (!fs.existsSync(f)) return [];
	try { return require(f).map(p => ({ ...p, set: n })); }
	catch (e) { console.warn(`parts_set${n}.js skipped: ${e.message}`); return []; }   // one set's error never stops another's
});
const PARTS = SETS.flat();
const VIEWS = ['front', 'back', 'left', 'right'];
const MAKER = { 1: 'Saint Maud', 2: 'Roundhouse', 3: 'Windfall', 4: 'Meridian' };
const SLOTS = ['Body', 'Barrel', 'Muzzle', 'Magazine', 'Sight', 'Stock'];
const viewPage = (svg, bg) => `<!doctype html><html><head><meta charset="utf-8"><style>html,body{margin:0;background:${bg}}svg{display:block}</style></head>
	<body><svg xmlns="http://www.w3.org/2000/svg" width="${N}" height="${N}" viewBox="0 0 ${N} ${N}">${svg}</svg></body></html>`;
const fileUrl = f => 'file:///' + f.split('\\').join('/');

// A sheet of left views: rows of parts, a column per set (for checking the sets are different designs).
function sheetPage(rows, title, T) {
	const W = 40 + 4 * (T + 20), H = 90 + rows.length * (T + 40);
	let s = `<rect width="${W}" height="${H}" fill="#ffffff"/><text x="30" y="46" font-size="30" font-weight="700" fill="#1a1d22">${title}</text>`;
	[1, 2, 3, 4].forEach((n, c) => { s += `<text x="${30 + c * (T + 20) + T / 2}" y="80" font-size="20" font-weight="700" fill="#c0601c" text-anchor="middle">${n} · ${MAKER[n].toUpperCase()}</text>`; });
	rows.forEach((row, r) => row.forEach((p, c) => {
		if (!p) return;
		const x = 30 + c * (T + 20), y = 96 + r * (T + 40);
		s += `<rect x="${x}" y="${y}" width="${T}" height="${T}" fill="#f2f2f2"/><image href="${fileUrl(path.join(__dirname, p.dir, 'upload', 'left.png'))}" x="${x}" y="${y}" width="${T}" height="${T}"/>` +
			`<text x="${x + T / 2}" y="${y + T + 24}" font-size="17" fill="#333" text-anchor="middle">${p.dir}</text>`;
	}));
	return [W, H, `<!doctype html><html><head><meta charset="utf-8"><style>html,body{margin:0}svg{display:block;font-family:Segoe UI,Arial}</style></head><body><svg xmlns="http://www.w3.org/2000/svg" width="${W}" height="${H}">${s}</svg></body></html>`];
}

(async () => {
	const args = process.argv.slice(2);
	const browser = await chromium.launch();
	const pg = await browser.newPage({ viewport: { width: N, height: N } });
	if (args.includes('--sheet')) {
		const pick = (slot, n) => SETS[n - 1].find(p => p.dir.startsWith(slot + '_'));
		for (const [name, rows, T] of [['compare_bodies', [[1, 2, 3, 4].map(n => pick('Body', n))], 560],
			['compare_sets', SLOTS.map(slot => [1, 2, 3, 4].map(n => pick(slot, n))), 300]]) {
			const [W, H, html] = sheetPage(rows, `Ranchhand ${name === 'compare_bodies' ? 'bodies' : 'parts'}: four makers, left views`, T);
			const f = path.join(__dirname, `.${name}.html`);
			fs.writeFileSync(f, html);
			await pg.setViewportSize({ width: W, height: H });
			await pg.goto(fileUrl(f));
			await pg.screenshot({ path: path.join(__dirname, `${name}.png`) });
			fs.unlinkSync(f);
			console.log('wrote', name + '.png');
		}
		await browser.close();
		return;
	}
	const want = args.filter(a => !a.startsWith('--'));
	for (const p of PARTS) {
		if (want.length && !want.includes(p.dir) && !want.includes('set' + p.set)) continue;
		const dir = path.join(__dirname, p.dir);
		for (const [sub, bg] of [['upload', 'transparent'], ['upload_white', '#ffffff']]) {
			fs.mkdirSync(path.join(dir, sub), { recursive: true });
			for (const vk of VIEWS) {
				const V = makeView(vk, p.b);
				const file = path.join(dir, `.${vk}.html`);
				fs.writeFileSync(file, viewPage(defs(V) + defs4(V) + p.draw[vk](V), bg));
				await pg.goto(fileUrl(file));
				await pg.screenshot({ path: path.join(dir, sub, `${vk}.png`), omitBackground: bg === 'transparent' });
				fs.unlinkSync(file);
			}
		}
		console.log('wrote', p.dir);
	}
	await browser.close();
})();
