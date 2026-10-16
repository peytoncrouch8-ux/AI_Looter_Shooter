// Lays a hero's finished portraits side by side on one sheet with their labels, in the game's font.
//   node Art/Portraits/sheet.js <dir> [--hero <Hero>] [--script <file>] [--title <text>] [Scene...]
// No --hero: Ellis, scenes EllisA EllisB EllisC -> <dir>/Sheet.png (as before). With --hero: scenes <Hero>A <Hero>B
// <Hero>C -> <dir>/Sheet_<Hero>.png, so every hero's sheet can sit in one folder. The title is the hero's name in
// capitals; each portrait's key, title and line come from the hero script's OPTIONS ("label" and "line" per key).
const fs = require('fs');
const path = require('path');
let playwright;
try { playwright = require('playwright'); } catch { playwright = require('/opt/node22/lib/node_modules/playwright'); }

// The heroes of the pick: full name and the script that holds their OPTIONS.
const HEROES = {
	Ellis: ['Ellis Ransom', 'EllisBlender.py'],
	Odessa: ['Odessa Lark', 'Odessa.py'],
	Crane: ['Hollis Crane', 'Crane.py'],
	Gauge: ['Gauge', 'Gauge.py'],
	Pike: ['Wendell Pike', 'Pike.py'],
};
// Ellis's labels from before the scripts carried a "line": the fallback for anything a script doesn't say.
const LABELS = {
	EllisA: ['A', 'THE REVENANT', 'Hat low, bandana up, eyes burning in the brim’s shadow; duster with a shoulder cape.'],
	EllisB: ['B', 'CAME HOME AT THE LAST', 'Bandana down, hat pushed back, face bare and grave-pale; open duster, bandolier.'],
	EllisC: ['C', 'DRESSED FOR THE GRAVE', 'The burial clothes Tilly put on them: black coat, string tie, a dead flower, frost.'],
};

const args = process.argv.slice(2), pos = [], flags = {};
for (let i = 0; i < args.length; i++) {
	if (args[i].startsWith('--')) flags[args[i].slice(2)] = args[++i];
	else pos.push(args[i]);
}
const dir = path.resolve(pos[0] || path.join(__dirname, '../../Saved/ArtPreviews/Portraits'));
const heroArg = flags.hero;
const hero = heroArg ? (Object.keys(HEROES).find(k => k.toLowerCase() === heroArg.toLowerCase()) || heroArg) : 'Ellis';
const [fullName, scriptName] = HEROES[hero] || [hero, hero + '.py'];
const scenes = pos.length > 1 ? pos.slice(1) : ['A', 'B', 'C'].map(k => hero + k);
const title = (flags.title || fullName).toUpperCase();
const script = flags.script ? path.resolve(flags.script) : path.join(__dirname, scriptName);
const outName = heroArg ? `Sheet_${hero}.png` : 'Sheet.png';

// Python string literal(s) at the start of s: one or more adjacent "..." / '...' pieces (implicit concatenation,
// optionally in parentheses), unescaped.
function pyString(s) {
	const m = s.match(/^\s*\(?\s*((?:(?:"(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*')\s*)+)/);
	if (!m) return null;
	let out = '';
	for (const piece of m[1].match(/"(?:[^"\\\n]|\\.)*"|'(?:[^'\\\n]|\\.)*'/g)) out += piece.slice(1, -1).replace(/\\(.)/g, (_, c) => (c === 'n' ? ' ' : c));
	return out;
}
// OPTIONS' "label" and "line" per key, read by a tolerant regex over the script (the dicts are literal).
function readOptions(file) {
	const found = {};
	if (!fs.existsSync(file)) return found;
	const src = fs.readFileSync(file, 'utf8');
	const start = src.search(/^OPTIONS\s*=\s*\{/m);
	if (start < 0) return found;
	const end = src.slice(start).search(/^\}/m);
	const block = end > 0 ? src.slice(start, start + end) : src.slice(start);
	const keys = [...block.matchAll(/(["'])([A-Z])\1\s*:\s*\{/g)];
	keys.forEach((k, i) => {
		const seg = block.slice(k.index, i + 1 < keys.length ? keys[i + 1].index : block.length);
		const get = name => { const m = seg.match(new RegExp(`["']${name}["']\\s*:`)); return m ? pyString(seg.slice(m.index + m[0].length)) : null; };
		found[k[2]] = { label: get('label'), line: get('line') };
	});
	return found;
}
const opts = readOptions(script);
const pretty = s => s.replace(/'/g, '’');
const labels = scenes.map(s => {
	const key = s.slice(-1), o = opts[key] || {}, fb = LABELS[s] || [key, s, ''];
	const label = o.label ? pretty(o.label).toUpperCase() : fb[1], line = o.line ? pretty(o.line) : fb[2];
	console.log(`${s}: ${o.label ? 'label' : 'label (fallback)'}, ${o.line ? 'line' : 'line (fallback)'}`);
	return [key, label, line];
});
const font = f => fs.readFileSync(path.join(__dirname, '../../Content/UI/Fonts', f)).toString('base64');

(async () => {
	const browser = await playwright.chromium.launch();
	const page = await browser.newPage();
	const png = await page.evaluate(async ({ imgs, labels, title, bold, regular }) => {
		for (const [name, data] of [['ChakraBold', bold], ['ChakraRegular', regular]]) {
			const f = new FontFace(name, `url(data:font/ttf;base64,${data})`);
			await f.load(); document.fonts.add(f);
		}
		const load = src => new Promise(r => { const i = new Image(); i.onload = () => r(i); i.src = src; });
		const ims = await Promise.all(imgs.map(load));
		const S = 1024, gap = 56, top = 170;
		const c = document.createElement('canvas');
		const g = c.getContext('2d');
		// wrap each description to the portrait's width first: a third row grows the sheet instead of falling off it
		g.font = '30px ChakraRegular'; g.letterSpacing = '1px';
		const wrapped = labels.map(([, , line]) => {
			const rows = [''];
			for (const w of line.split(' ')) {
				const t = rows[rows.length - 1] ? rows[rows.length - 1] + ' ' + w : w;
				if (g.measureText(t).width > S - 60) rows.push(w); else rows[rows.length - 1] = t;
			}
			return rows;
		});
		const bottom = Math.max(220, 140 + Math.max(...wrapped.map(r => r.length)) * 40);
		c.width = ims.length * S + (ims.length + 1) * gap; c.height = top + S + bottom;
		g.fillStyle = '#12131a'; g.fillRect(0, 0, c.width, c.height);
		g.textAlign = 'center';
		g.fillStyle = '#ff9f1c'; g.font = '78px ChakraBold';
		g.letterSpacing = '14px';
		g.fillText(title, c.width / 2, 98);
		g.letterSpacing = '6px';
		g.fillStyle = '#6ee7ff'; g.font = '30px ChakraRegular';
		g.fillText('PORTRAIT CONCEPTS  ·  PICK ONE FOR THE NEW 3D MODEL', c.width / 2, 146);
		ims.forEach((im, i) => {
			const x = gap + i * (S + gap);
			g.drawImage(im, x, top, S, S);
			const [k, title] = labels[i];
			g.letterSpacing = '8px';
			g.fillStyle = '#ff9f1c'; g.font = '50px ChakraBold';
			g.fillText(`${k}  ·  ${title}`, x + S / 2, top + S + 72);
			g.letterSpacing = '1px';
			g.fillStyle = '#c9ced6'; g.font = '30px ChakraRegular';
			wrapped[i].forEach((r, k) => g.fillText(r, x + S / 2, top + S + 122 + k * 40));
		});
		return c.toDataURL('image/png');
	}, {
		imgs: scenes.map(s => 'data:image/png;base64,' + fs.readFileSync(path.join(dir, s + '.png')).toString('base64')),
		labels, title,
		bold: font('ChakraPetch-Bold.ttf'), regular: font('ChakraPetch-Regular.ttf'),
	});
	fs.writeFileSync(path.join(dir, outName), Buffer.from(png.split(',')[1], 'base64'));
	await browser.close();
	console.log('wrote', path.join(dir, outName));
})();
