// Lays the finished portraits side by side on one sheet with their labels, in the game's font.
//   node Art/Portraits/sheet.js <dir> [Scene...]     (default EllisA EllisB EllisC) -> <dir>/Sheet.png
const fs = require('fs');
const path = require('path');
let playwright;
try { playwright = require('playwright'); } catch { playwright = require('/opt/node22/lib/node_modules/playwright'); }

const dir = path.resolve(process.argv[2] || path.join(__dirname, '../../Saved/ArtPreviews/Portraits'));
const scenes = process.argv.slice(3).length ? process.argv.slice(3) : ['EllisA', 'EllisB', 'EllisC'];
const LABELS = {
	EllisA: ['A', 'THE REVENANT', 'Hat low, bandana up, eyes burning in the brim’s shadow; duster with a shoulder cape.'],
	EllisB: ['B', 'CAME HOME AT THE LAST', 'Bandana down, hat pushed back, face bare and grave-pale; open duster, bandolier.'],
	EllisC: ['C', 'DRESSED FOR THE GRAVE', 'The burial clothes Tilly put on them: black coat, string tie, a dead flower, frost.'],
};
const font = f => fs.readFileSync(path.join(__dirname, '../../Content/UI/Fonts', f)).toString('base64');

(async () => {
	const browser = await playwright.chromium.launch();
	const page = await browser.newPage();
	const png = await page.evaluate(async ({ imgs, labels, bold, regular }) => {
		for (const [name, data] of [['ChakraBold', bold], ['ChakraRegular', regular]]) {
			const f = new FontFace(name, `url(data:font/ttf;base64,${data})`);
			await f.load(); document.fonts.add(f);
		}
		const load = src => new Promise(r => { const i = new Image(); i.onload = () => r(i); i.src = src; });
		const ims = await Promise.all(imgs.map(load));
		const S = 1024, gap = 56, top = 170, bottom = 220;
		const c = document.createElement('canvas');
		c.width = ims.length * S + (ims.length + 1) * gap; c.height = top + S + bottom;
		const g = c.getContext('2d');
		g.fillStyle = '#12131a'; g.fillRect(0, 0, c.width, c.height);
		g.textAlign = 'center';
		g.fillStyle = '#ff9f1c'; g.font = '78px ChakraBold';
		g.letterSpacing = '14px';
		g.fillText('ELLIS RANSOM', c.width / 2, 98);
		g.letterSpacing = '6px';
		g.fillStyle = '#6ee7ff'; g.font = '30px ChakraRegular';
		g.fillText('PORTRAIT CONCEPTS  ·  PICK ONE FOR THE NEW 3D MODEL', c.width / 2, 146);
		ims.forEach((im, i) => {
			const x = gap + i * (S + gap);
			g.drawImage(im, x, top, S, S);
			const [k, title, line] = labels[i];
			g.letterSpacing = '8px';
			g.fillStyle = '#ff9f1c'; g.font = '50px ChakraBold';
			g.fillText(`${k}  ·  ${title}`, x + S / 2, top + S + 72);
			g.letterSpacing = '1px';
			g.fillStyle = '#c9ced6'; g.font = '30px ChakraRegular';
			// wrap the description to the portrait's width
			const words = line.split(' '), rows = [''];
			for (const w of words) {
				const t = rows[rows.length - 1] ? rows[rows.length - 1] + ' ' + w : w;
				if (g.measureText(t).width > S - 60) rows.push(w); else rows[rows.length - 1] = t;
			}
			rows.forEach((r, k) => g.fillText(r, x + S / 2, top + S + 122 + k * 40));
		});
		return c.toDataURL('image/png');
	}, {
		imgs: scenes.map(s => 'data:image/png;base64,' + fs.readFileSync(path.join(dir, s + '.png')).toString('base64')),
		labels: scenes.map(s => LABELS[s] || [s, s, '']),
		bold: font('ChakraPetch-Bold.ttf'), regular: font('ChakraPetch-Regular.ttf'),
	});
	fs.writeFileSync(path.join(dir, 'Sheet.png'), Buffer.from(png.split(',')[1], 'base64'));
	await browser.close();
	console.log('wrote', path.join(dir, 'Sheet.png'));
})();
