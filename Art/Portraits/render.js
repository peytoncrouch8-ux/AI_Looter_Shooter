// Paints and frames a rendered portrait bust (a hero script's passes, bust.py) in headless Chromium.
//
//   node Art/Portraits/render.js <Scene> --passes <dir> [--script EllisBlender.py] [--frame Frame.svg] [--size 1024] [--out <dir>]
//
// Reads <dir>/<Scene>_color.png (the render, transparent background) and <dir>/<Scene>_aux.png (R glow mask,
// B material id / 16), paints them with paint.js, clips the painting to Window.svg and lays the frame over it:
// <out>/<Scene>.png (<out> defaults to <dir>). The paint settings are the "# PAINT <X> {...}" line for the
// scene's option (its last character) in the hero script (--script, beside this file unless absolute; default
// EllisBlender.py). The frame is Ellis's Frame.svg unless --frame names a hero's (Frame_<Hero>.svg from frame/gen.js;
// beside this file unless absolute). Playwright comes from the repo's node_modules (or the cloud image's global ones).
const fs = require('fs');
const path = require('path');
let playwright;
try { playwright = require('playwright'); } catch { playwright = require('/opt/node22/lib/node_modules/playwright'); }

const here = __dirname;
const args = process.argv.slice(2);
const scene = args[0];
const opt = (name, def) => { const i = args.indexOf('--' + name); return i >= 0 ? args[i + 1] : def; };
const passDir = path.resolve(opt('passes', path.join(here, '../../Saved/ArtPreviews/Portraits')));
const size = +opt('size', 1024);
const outDir = path.resolve(opt('out', passDir));
fs.mkdirSync(outDir, { recursive: true });

const read = f => fs.readFileSync(path.join(here, f), 'utf8');
const dataUrl = f => 'data:image/png;base64,' + fs.readFileSync(path.join(passDir, f)).toString('base64');
const key = scene.slice(-1);
const script = path.resolve(here, opt('script', 'EllisBlender.py'));
const line = fs.readFileSync(script, 'utf8').match(new RegExp('# PAINT ' + key + ' (\\{.*\\})'));
const opts = line ? JSON.parse(line[1]) : {};
const frameFile = path.resolve(here, opt('frame', 'Frame.svg'));

(async () => {
	const browser = await playwright.chromium.launch();
	const page = await browser.newPage();
	page.on('console', m => console.log('[page]', m.text()));
	const t0 = Date.now();
	const passes = [dataUrl(scene + '_color.png'), dataUrl(scene + '_aux.png')];
	const done = await page.evaluate(new Function('a', read('paint.js') + '\nreturn paintPortrait(a);'),
		{ passes, size, frame: fs.readFileSync(frameFile, 'utf8'), windowSvg: read('Window.svg'), opts });
	fs.writeFileSync(path.join(outDir, scene + '.png'), Buffer.from(done.split(',')[1], 'base64'));
	console.log(`painted in ${((Date.now() - t0) / 1000).toFixed(1)} s -> ${path.join(outDir, scene + '.png')}`);
	await browser.close();
})().catch(e => { console.error(e.message || e); process.exit(1); });
