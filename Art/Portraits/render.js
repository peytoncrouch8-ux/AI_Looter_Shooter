// Renders a portrait bust: a signed-distance scene (<Scene>.glsl) marched in headless Chromium's WebGL2
// (SwiftShader on the CPU, so no GPU is needed), then painted and framed by paint.js.
//
//   node Art/Portraits/render.js <Scene> [--size 1024] [--ss 2] [--out <dir>] [--raw]
//
// --size is the finished square in pixels, --ss the supersampling factor of the march.
// --raw writes only the marched passes (colour, glow/depth/id, normal) for quick looks.
// Output: <out>/<Scene>.png (finished) and <out>/<Scene>_raw.png; <out> defaults to
// Saved/ArtPreviews/Portraits. Playwright comes from the cloud image's global node modules.
const fs = require('fs');
const path = require('path');
let playwright;
try { playwright = require('playwright'); } catch { playwright = require('/opt/node22/lib/node_modules/playwright'); }

const here = __dirname;
const args = process.argv.slice(2);
const scene = args[0];
const opt = (name, def) => { const i = args.indexOf('--' + name); return i >= 0 ? args[i + 1] : def; };
const size = +opt('size', 1024);
const ss = +opt('ss', 2);
const outDir = path.resolve(opt('out', path.join(here, '../../Saved/ArtPreviews/Portraits')));
const raw = args.includes('--raw');
// --passes <dir>: paint passes rendered elsewhere (EllisBlender.py) instead of marching <Scene>.glsl.
const passDir = opt('passes', null);
fs.mkdirSync(outDir, { recursive: true });

const read = f => fs.readFileSync(path.join(here, f), 'utf8');
// Scenes pull in shared parts with #include "File.glsl".
const expand = src => src.replace(/^#include "(.+)"$/gm, (_, f) => expand(read(f)));
const frag = passDir ? '' : '#version 300 es\nprecision highp float;\n' + read('lib.glsl') + '\n' + expand(read(scene + '.glsl')) + '\n' + read('main.glsl');

(async () => {
	const browser = await playwright.chromium.launch({ args: ['--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
	const page = await browser.newPage();
	page.on('console', m => console.log('[page]', m.text()));
	const t0 = Date.now();
	const dataUrl = f => 'data:image/png;base64,' + fs.readFileSync(path.join(passDir, f)).toString('base64');
	const passes = passDir ? [dataUrl(scene + '_color.png'), dataUrl(scene + '_aux.png'), dataUrl(scene + '_aux.png')] : await page.evaluate(async ({ frag, W }) => {
		const c = document.createElement('canvas'); c.width = W; c.height = W;
		const gl = c.getContext('webgl2', { preserveDrawingBuffer: true, antialias: false, premultipliedAlpha: false });
		const mk = (type, src) => {
			const s = gl.createShader(type); gl.shaderSource(s, src); gl.compileShader(s);
			if (!gl.getShaderParameter(s, gl.COMPILE_STATUS)) throw new Error(gl.getShaderInfoLog(s));
			return s;
		};
		const prog = gl.createProgram();
		gl.attachShader(prog, mk(gl.VERTEX_SHADER, '#version 300 es\nin vec2 a; void main(){ gl_Position = vec4(a, 0, 1); }'));
		gl.attachShader(prog, mk(gl.FRAGMENT_SHADER, frag));
		gl.bindAttribLocation(prog, 0, 'a');
		gl.linkProgram(prog);
		if (!gl.getProgramParameter(prog, gl.LINK_STATUS)) throw new Error(gl.getProgramInfoLog(prog));
		gl.useProgram(prog);
		gl.bindBuffer(gl.ARRAY_BUFFER, gl.createBuffer());
		gl.bufferData(gl.ARRAY_BUFFER, new Float32Array([-1, -1, 3, -1, -1, 3]), gl.STATIC_DRAW);
		gl.enableVertexAttribArray(0); gl.vertexAttribPointer(0, 2, gl.FLOAT, false, 0, 0);
		gl.uniform2f(gl.getUniformLocation(prog, 'uRes'), W, W);
		gl.viewport(0, 0, W, W);
		gl.enable(gl.SCISSOR_TEST);
		const out = [];
		const tile = 192;
		for (let pass = 0; pass < 3; pass++) {
			gl.uniform1i(gl.getUniformLocation(prog, 'uPass'), pass);
			gl.clearColor(0, 0, 0, 0); gl.scissor(0, 0, W, W); gl.clear(gl.COLOR_BUFFER_BIT);
			// Small tiles keep each draw short, so the GPU process's watchdog never fires.
			for (let y = 0; y < W; y += tile) for (let x = 0; x < W; x += tile) {
				gl.scissor(x, y, tile, tile); gl.drawArrays(gl.TRIANGLES, 0, 3); gl.finish();
				await new Promise(r => setTimeout(r, 0));
			}
			out.push(c.toDataURL('image/png'));
			console.log('pass ' + pass + ' done');
		}
		return out;
	}, { frag, W: size * ss });
	console.log(`marched in ${((Date.now() - t0) / 1000).toFixed(1)} s`);

	// Downsample the passes and lay them side by side for the raw sheet.
	const rawSheet = await page.evaluate(async ({ passes, size }) => {
		const load = src => new Promise(r => { const i = new Image(); i.onload = () => r(i); i.src = src; });
		const imgs = await Promise.all(passes.map(load));
		const c = document.createElement('canvas'); c.width = size * 3; c.height = size;
		const g = c.getContext('2d'); g.imageSmoothingQuality = 'high';
		g.fillStyle = '#20242a'; g.fillRect(0, 0, c.width, c.height);
		imgs.forEach((im, i) => g.drawImage(im, i * size, 0, size, size));
		return c.toDataURL('image/png');
	}, { passes, size });
	fs.writeFileSync(path.join(outDir, scene + '_raw.png'), Buffer.from(rawSheet.split(',')[1], 'base64'));

	if (!raw) {
		const paint = read('paint.js');
		const frame = fs.existsSync(path.join(here, 'Frame.svg')) ? read('Frame.svg') : null;
		const windowSvg = fs.existsSync(path.join(here, 'Window.svg')) ? read('Window.svg') : null;
		const optsFile = fs.existsSync(path.join(here, scene + '.glsl')) ? scene + '.glsl' : 'EllisBlender.py';
		const sceneOpts = JSON.parse((read(optsFile).match(new RegExp('(?://|#) PAINT ' + (passDir ? scene.slice(-1) + ' ' : '') + '(\\{.*\\})')) || [0, '{}'])[1]);
		const t1 = Date.now();
		const done = await page.evaluate(new Function('a', paint + '\nreturn paintPortrait(a);'), { passes, size, frame, windowSvg, opts: sceneOpts });
		fs.writeFileSync(path.join(outDir, scene + '.png'), Buffer.from(done.split(',')[1], 'base64'));
		console.log(`painted in ${((Date.now() - t1) / 1000).toFixed(1)} s`);
	}
	await browser.close();
	console.log('wrote', outDir);
})().catch(e => { console.error(e.message || e); process.exit(1); });
