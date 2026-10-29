// Round 11 (the user, 2026-10-28): the compact card "bolder, less boxy", "bolder outlines as well", in theme 4 Grave Light.
// Three frame shapes, each with a 3 px outline that follows the shape. Builds bold.js from compact.js and renders bold.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'compact.js'), 'utf8');
let head = s.slice(0, s.indexOf('function compact(g)'));

const tail = String.raw`
// Grave Light: deep green-black glass, ghostly mint lines, soul-light gold accents.
const GL = { top: 'rgba(12,28,26,.95)', foot: 'rgba(7,17,16,.93)', line: '#8cf0d2', acc: '#ffe6a6', lab: '#a8c9bf', text: '#eef9f5', fl: '#ffd38a', icon: '#8fb3a8' };
const bcss = '.Bc .lr{display:grid;grid-template-columns:18px 1fr auto 16px;align-items:center;gap:8px;padding:2px 0}' +
	'.Bc .lab{font-family:Chakra;font-weight:700;font-size:14px;color:' + GL.lab + ';letter-spacing:.5px}' +
	'.Bc .val{font-family:Chakra;font-weight:700;font-size:17px;color:' + GL.text + ';text-align:right}' +
	'.Bc .arr{font-size:12px;text-align:right}.Bc .u{color:#7dff8a}.Bc .w{color:#ff7a6b}' +
	'.Bc .bul{font-family:Chakra;font-weight:700;font-size:13.5px;color:' + GL.text + ';line-height:1.3;padding-left:12px;text-indent:-12px}' +
	'.Bc .fl{font-family:Chakra;font-style:italic;font-weight:700;font-size:13.5px;color:' + GL.fl + ';line-height:1.3}' +
	'.Bc [fill="#9fb6c4"]{fill:' + GL.icon + '}.Bc [stroke="#9fb6c4"]{stroke:' + GL.icon + ';stroke-width:2.2}' +
	'.gcap{display:inline-flex;align-items:center;justify-content:center;min-width:26px;height:24px;padding:0 7px;margin-right:8px;color:' + GL.acc + ';font-family:Chakra;font-weight:700;font-size:14px;' +
	'box-shadow:inset 0 0 0 3px ' + GL.acc + ',0 0 0 1.5px rgba(0,0,0,.55);border-radius:4px}';

// Frames: an outer shape in the line colour, the glass inside it 3 px in, so the outline follows the shape exactly.
const W = 360;
function framed(shape, inner) {
	if (shape === 'round') {
		return '<div style="position:relative;width:' + W + 'px;border:3px solid ' + GL.line + ';border-radius:22px 6px 22px 6px;background:linear-gradient(180deg,' + GL.top + ',' + GL.foot + ');box-shadow:0 0 0 1.5px rgba(0,0,0,.5),0 0 18px rgba(140,240,210,.18)">' + inner + '</div>';
	}
	const clip = shape === 'cut'
		? (i) => 'polygon(' + (18 - i) + 'px 0,calc(100% - ' + (18 - i) + 'px) 0,100% ' + (18 - i) + 'px,100% calc(100% - ' + (18 - i) + 'px),calc(100% - ' + (18 - i) + 'px) 100%,' + (18 - i) + 'px 100%,0 calc(100% - ' + (18 - i) + 'px),0 ' + (18 - i) + 'px)'
		: (i) => 'polygon(0 ' + (14 - i) + 'px,' + (14 - i) + 'px 0,calc(50% - 40px) 0,calc(50% - 30px) 8px,calc(50% + 30px) 8px,calc(50% + 40px) 0,calc(100% - ' + (14 - i) + 'px) 0,100% ' + (14 - i) + 'px,100% calc(100% - 22px),50% 100%,0 calc(100% - 22px))';
	return '<div style="position:relative;width:' + W + 'px;filter:drop-shadow(0 0 1.5px rgba(0,0,0,.7)) drop-shadow(0 0 14px rgba(140,240,210,.16))">' +
		'<div style="position:absolute;inset:0;background:' + GL.line + ';clip-path:' + clip(0) + '"></div>' +
		'<div style="position:absolute;inset:3px;background:linear-gradient(180deg,' + GL.top + ',' + GL.foot + ');clip-path:' + clip(1.3) + '"></div>' +
		'<div style="position:relative">' + inner + '</div></div>';
}
function boldCard(g, shape) {
	const M = MAKER[g.maker];
	const lvl = '<div style="display:inline-flex;margin:0 0 7px 16px;padding:3px 14px;font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:2.5px;color:' + GL.acc + ';' +
		'border:2.5px solid ' + GL.acc + ';border-radius:12px;background:rgba(7,17,16,.85)">LEVEL ' + g.level + '</div>';
	const inner = '<div class="Bc" style="padding:' + (shape === 'tag' ? '20px' : '14px') + ' 20px ' + (shape === 'tag' ? '30px' : '14px') + '">' +
		'<div style="font-family:Chakra;font-weight:700;font-size:23px;letter-spacing:.5px;color:' + RAR[g.rarity] + ';text-align:center;text-shadow:0 0 10px rgba(0,0,0,.6);margin-bottom:8px">' + g.name + '</div>' +
		g.stats.map(lr).join('') +
		'<div style="height:2px;margin:9px 0 8px;background:linear-gradient(90deg,transparent,' + GL.line + ',transparent);opacity:.7"></div>' +
		g.lines.map(t => '<div class="bul">• ' + t + '</div>').join('') +
		(g.flavor ? '<div class="fl" style="margin-top:4px">' + g.flavor + '</div>' : '') +
		'<div style="display:flex;align-items:center;justify-content:' + (shape === 'tag' ? 'center' : 'space-between') + ';gap:14px;margin-top:11px">' +
		'<div style="display:flex;align-items:center;gap:7px;padding:4px 14px 4px 10px;border-radius:12px;background:linear-gradient(90deg,' + M.a + ',' + M.b + ');box-shadow:inset 0 0 0 2px ' + M.trim + ';font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:2.5px;color:' + M.ink + '">' + MARK[g.maker](M.ink) + M.word + '</div>' +
		(g.element || '') + '</div></div>';
	return '<div>' + lvl + framed(shape, inner) +
		'<div style="display:flex;gap:6px;align-items:center;margin:8px 0 0 16px;font-family:Chakra;font-weight:700;font-size:15px;color:' + GL.text + ';text-shadow:0 0 3px #000">' + coin + g.value + '</div></div>';
}
const A = { maker: 'maud', rarity: 'rare', name: 'SLUGGER PUMP SHOTGUN', level: 6,
	stats: [['dmg', 'Damage', '21 × 8', 'u'], ['acc', 'Accuracy', '62%', 'w'], ['hnd', 'Handling', '70%', 'u'], ['rel', 'Reload', '1.7 s', 'u'], ['rof', 'Fire rate', '66 rpm', 'u'], ['mag', 'Magazine', '10', 'u']],
	lines: ['Salts and slows them: wards crumble', 'Blessed Iron: +12% damage, slower fire'],
	flavor: 'Sister Agnes blessed this one twice.', element: salt, value: '412' };
const B = { maker: 'round', rarity: 'epic', name: 'BRUTE ASSAULT RIFLE', level: 7,
	stats: [['dmg', 'Damage', '34', 'u'], ['acc', 'Accuracy', '81%', 'u'], ['hnd', 'Handling', '58%', 'w'], ['rel', 'Reload', '2.2 s', 'w'], ['rof', 'Fire rate', '540 rpm', 'u'], ['mag', 'Magazine', '30', 'u']],
	lines: ['Sets them burning', 'Rolling Stock: +15% damage, more kick'], flavor: '', element: ember, value: '1,280' };
const C = { maker: 'mer', rarity: 'leg', name: 'SURVEYOR SNIPER RIFLE', level: 6,
	stats: [['dmg', 'Damage', '375', 'u'], ['acc', 'Accuracy', '96%', 'u'], ['hnd', 'Handling', '44%', 'w'], ['rel', 'Reload', '3.1 s', 'w'], ['rof', 'Fire rate', '45 rpm', 'w'], ['mag', 'Magazine', '5', 'w']],
	lines: ['Rounds punch through an enemy into the next', 'True Glass: +10% accuracy and range'], flavor: 'Every mile of track, measured twice.', element: '', value: '3,900' };

const keys = '<div style="margin:0 0 0 14px;display:flex;flex-direction:column;gap:10px;font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:1.5px;color:#f3f3f3;text-shadow:0 0 3px #000,0 0 6px #000">' +
	'<span><span class="gcap">E</span>TAKE</span><span><span class="gcap">HOLD E</span>EQUIP</span></div>';
const col = (x, t, d, g, shape, bg) => '<div style="position:absolute;left:' + x + 'px;top:126px;width:600px"><div class="h" style="font-size:20px;color:#8cf0d2">' + t + '</div>' +
	'<div class="note" style="margin-top:6px;height:40px;font-size:14px">' + d + '</div>' +
	'<div style="position:relative;margin-top:8px;height:720px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') ' + bg + '/2403px auto">' +
	'<div style="position:absolute;left:44px;top:44px;display:flex;align-items:flex-end">' + boldCard(g, shape) + '<div style="margin-bottom:60px">' + keys + '</div></div></div></div>';
const html = '<!doctype html><html><head><meta charset="utf-8"><style>' + css + ccss + bcss + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#11201d,#060909 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:30px;font-size:30px;color:#8cf0d2">GRAVE LIGHT, BOLDER AND LESS BOXY</div>' +
	'<div class="note" style="position:absolute;left:40px;top:72px">3 px outlines that follow each shape, a soft glow, heavier and bigger text, round-cornered tags and keys. Three frame shapes.</div>' +
	col(40, 'A · CUT CORNERS', 'All four corners cut: sharp but never a box.', A, 'cut', '-500px -300px') +
	col(670, 'B · SOFT CORNERS', 'Two round corners and two tight ones: a little curve.', B, 'round', '-700px -300px') +
	col(1300, 'C · TAG', 'A tag hanging over the gun: a notch on top, a point below it.', C, 'tag', '-900px -300px') +
	'</div></body></html>';
fs.writeFileSync(path.join(here, 'bold.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	await pg.goto('file:///' + path.join(here, 'bold.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1200);
	await pg.screenshot({ path: path.join(here, 'bold.png') });
	await browser.close();
	console.log('done');
})();
`;
fs.writeFileSync(path.join(here, 'bold.js'), head + tail);
console.log('bold.js written');
