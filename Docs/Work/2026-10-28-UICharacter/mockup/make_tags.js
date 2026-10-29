// Round 12 (the user, 2026-10-28: "C is in the right direction, let's build off of this, couple more concepts similar"):
// four more tag shapes in Grave Light. Builds tags.js from bold.js (its card, colours, styles) and renders tags.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'bold.js'), 'utf8');

// The shapes: i is how far in (px) the glass sits, so the outline stays even round every corner.
const shapes = String.raw`const SHAPES = {
	tag: (i) => 'polygon(0 ' + (14 - i) + 'px,' + (14 - i) + 'px 0,calc(50% - 40px) 0,calc(50% - 30px) 8px,calc(50% + 30px) 8px,calc(50% + 40px) 0,calc(100% - ' + (14 - i) + 'px) 0,100% ' + (14 - i) + 'px,100% calc(100% - 22px),50% 100%,0 calc(100% - 22px))',
	coffin: (i) => 'polygon(14% 0,86% 0,100% 13%,90% 100%,10% 100%,0 13%)',
	toe: (i) => 'polygon(0 ' + (24 - i) + 'px,' + (24 - i) + 'px 0,calc(100% - ' + (24 - i) + 'px) 0,100% ' + (24 - i) + 'px,100% calc(100% - 26px),50% 100%,0 calc(100% - 26px))',
	swallow: (i) => 'polygon(0 ' + (14 - i) + 'px,' + (14 - i) + 'px 0,calc(50% - 40px) 0,calc(50% - 30px) 8px,calc(50% + 30px) 8px,calc(50% + 40px) 0,calc(100% - ' + (14 - i) + 'px) 0,100% ' + (14 - i) + 'px,100% 100%,50% calc(100% - 24px),0 100%)',
	notch: (i) => 'polygon(0 ' + (14 - i) + 'px,' + (14 - i) + 'px 0,calc(100% - ' + (14 - i) + 'px) 0,100% ' + (14 - i) + 'px,100% 42%,calc(100% - 9px) 50%,100% 58%,100% calc(100% - 22px),calc(50% + 22px) calc(100% - 22px),50% 100%,calc(50% - 22px) calc(100% - 22px),0 calc(100% - 22px),0 58%,9px 50%,0 42%)',
};
`;
const clipStart = s.indexOf("\tconst clip = shape === 'cut'");
const clipEnd = s.indexOf("\treturn '<div style=\"position:relative;width:' + W + 'px;filter", clipStart);
if (clipStart < 0 || clipEnd < 0) throw new Error('clip block not found');
s = s.slice(0, clipStart) + "\tconst clip = SHAPES[shape];\n" + s.slice(clipEnd);
s = s.replace('function framed(shape, inner) {', shapes + 'function framed(shape, inner) {');
// The toe tag's punched grommet near its top, and the room each shape needs at its foot.
s = s.replace("'<div style=\"position:relative\">' + inner + '</div></div>';",
	"(shape === 'toe' ? '<div style=\"position:absolute;left:50%;top:22px;width:16px;height:16px;margin-left:-8px;border-radius:50%;border:3px solid ' + GL.line + ';background:#060b0a\"></div>' : '') +\n\t\t'<div style=\"position:relative\">' + inner + '</div></div>';");
s = s.replace("(shape === 'tag' ? '20px' : '14px') + ' 20px ' + (shape === 'tag' ? '30px' : '14px')",
	"(shape === 'toe' ? '30px' : shape === 'coffin' ? '16px' : '20px') + ' ' + (shape === 'coffin' ? '34px' : '20px') + ' ' + (shape === 'swallow' ? '34px' : '30px')");
s = s.replace("justify-content:' + (shape === 'tag' ? 'center' : 'space-between') + '", "justify-content:center");

// Four columns, keys under each card.
const layoutStart = s.indexOf('const keys = ');
const layoutEnd = s.indexOf('fs.writeFileSync(path.join(here, \'bold.html\')');
const layout = String.raw`const keys = '<div style="display:flex;justify-content:center;gap:22px;margin-top:12px;font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:1.5px;color:#f3f3f3;text-shadow:0 0 3px #000,0 0 6px #000">' +
	'<span><span class="gcap">E</span>TAKE</span><span><span class="gcap">HOLD E</span>EQUIP</span></div>';
const D = { maker: 'wind', rarity: 'unc', name: 'RATTLER SMG', level: 5,
	stats: [['dmg', 'Damage', '11', 'u'], ['acc', 'Accuracy', '58%', 'w'], ['hnd', 'Handling', '88%', 'u'], ['rel', 'Reload', '1.4 s', 'u'], ['rof', 'Fire rate', '900 rpm', 'u'], ['mag', 'Magazine', '40', 'u']],
	lines: ['Loose Fit: faster reload, more spread', 'Twin: two bullets a shot for one round'], flavor: 'Bolted from three other guns.', element: '', value: '640' };
const col = (x, t, d, g, shape, bg) => '<div style="position:absolute;left:' + x + 'px;top:126px;width:460px"><div class="h" style="font-size:19px;color:#8cf0d2">' + t + '</div>' +
	'<div class="note" style="margin-top:6px;height:40px;font-size:13.5px">' + d + '</div>' +
	'<div style="position:relative;margin-top:8px;height:760px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') ' + bg + '/2403px auto">' +
	'<div style="position:absolute;left:50px;top:36px">' + boldCard(g, shape) + keys + '</div></div></div>';
const html = '<!doctype html><html><head><meta charset="utf-8"><style>' + css + ccss + bcss + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#11201d,#060909 70%)">' +
	'<div class="h" style="position:absolute;left:30px;top:30px;font-size:30px;color:#8cf0d2">MORE TAGS, BUILT FROM C</div>' +
	'<div class="note" style="position:absolute;left:30px;top:72px">Same bold Grave Light card; four shapes that hang over the gun like a tag.</div>' +
	col(20, '1 · COFFIN TAG', 'Wide shoulders tapering down, like a coffin lid: the graveyard in the shape.', A, 'coffin', '-500px -300px') +
	col(495, '2 · TOE TAG', 'A punched grommet at the top and a point below: a tag tied on the gun.', B, 'toe', '-650px -300px') +
	col(970, '3 · PENNANT', 'The top notch, and a swallowtail of two points at the foot.', C, 'swallow', '-800px -300px') +
	col(1445, '4 · NOTCHED TAG', 'Side notches like a gun sight and a short point below: the most futuristic.', D, 'notch', '-950px -300px') +
	'</div></body></html>';
`;
s = s.slice(0, layoutStart) + layout + s.slice(layoutEnd);
s = s.split("'bold.html'").join("'tags.html'").split("'bold.png'").join("'tags.png'");
fs.writeFileSync(path.join(here, 'tags.js'), s);
console.log('tags.js written');
