// Round 13 (the user, 2026-10-28: "Mix toe tag and pennant, why is silver underneath? Let's make it bolder"): the toe tag's
// top (grommet, cut shoulders) with the pennant's swallowtail foot, 4 px outlines, bigger heavier text; the silver value
// moved into the foot (or left off). Builds mix.js from tags.js and renders mix.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'tags.js'), 'utf8');

// The mixed shape.
s = s.replace('const SHAPES = {', String.raw`const SHAPES = {
	mix: (i) => 'polygon(0 ' + (26 - i) + 'px,' + (26 - i) + 'px 0,calc(50% - 46px) 0,calc(50% - 34px) 11px,calc(50% + 34px) 11px,calc(50% + 46px) 0,calc(100% - ' + (26 - i) + 'px) 0,100% ' + (26 - i) + 'px,100% calc(100% - 28px),50% 100%,0 calc(100% - 28px))',`);
// Bolder: 4 px outline, glass 4 px in; the grommet on the mix too.
s = s.replace("inset:3px;background:linear-gradient", "inset:4px;background:linear-gradient");

s = s.replace("width:16px;height:16px;margin-left:-8px;border-radius:50%;border:3px solid", "width:20px;height:20px;margin-left:-10px;border-radius:50%;border:4px solid");
s = s.replace("(shape === 'toe' ? '30px' : shape === 'coffin' ? '16px' : '20px') + ' ' + (shape === 'coffin' ? '34px' : '20px') + ' ' + (shape === 'swallow' ? '34px' : '30px')",
	"(shape === 'toe' ? '36px' : shape === 'mix' ? '26px' : shape === 'coffin' ? '16px' : '20px') + ' ' + (shape === 'coffin' ? '34px' : '22px') + ' ' + (shape === 'mix' ? '40px' : shape === 'swallow' ? '40px' : '30px')");
// Bigger, heavier words.
const bump = [['font-size:14px;color:\' + GL.lab', 'font-size:16px;color:\' + GL.lab'], ['font-size:17px;color:\' + GL.text + \';text-align:right', 'font-size:20px;color:\' + GL.text + \';text-align:right'],
	['font-size:13.5px;color:\' + GL.text + \';line-height:1.3', 'font-size:15px;color:\' + GL.text + \';line-height:1.3'], ['font-size:13.5px;color:\' + GL.fl', 'font-size:15px;color:\' + GL.fl'],
	['font-size:23px;letter-spacing:.5px', 'font-size:27px;letter-spacing:.5px'], ['grid-template-columns:18px 1fr auto 16px;align-items:center;gap:8px;padding:2px 0', 'grid-template-columns:20px 1fr auto 18px;align-items:center;gap:9px;padding:3px 0'],
	["box-shadow:inset 0 0 0 3px ' + GL.acc + ',0 0 0 1.5px rgba(0,0,0,.55)", "box-shadow:inset 0 0 0 3.5px ' + GL.acc + ',0 0 0 2px rgba(0,0,0,.6)"],
	['min-width:26px;height:24px;padding:0 7px;margin-right:8px;color:\' + GL.acc + \';font-family:Chakra;font-weight:700;font-size:14px', 'min-width:30px;height:28px;padding:0 8px;margin-right:9px;color:\' + GL.acc + \';font-family:Chakra;font-weight:700;font-size:16px'],
	["const W = 360;", "const W = 400;"]];
for (const [a, b] of bump) { if (!s.includes(a)) throw new Error('missing: ' + a); s = s.split(a).join(b); }
s = s.replace(".Bc [stroke=\"#9fb6c4\"]{stroke:' + GL.icon + ';stroke-width:2.2}", ".Bc [stroke=\"#9fb6c4\"]{stroke:' + GL.icon + ';stroke-width:2.6}.Bc .lr svg{width:18px;height:18px}");

// The silver value: in the foot beside the maker (g.valueIn), or nowhere (g.noValue); no longer under the card.
const footOld = "(g.element || '') + '</div></div>';";
if (!s.includes(footOld)) throw new Error('foot not found');
s = s.replace(footOld, "(g.element || '') + (g.valueIn ? '<div style=\"display:flex;gap:6px;align-items:center;font-family:Chakra;font-weight:700;font-size:17px;color:' + GL.text + '\">' + coin.replace(/13/g, '16') + g.value + '</div>' : '') + '</div></div>';");
const underStart = s.indexOf("'<div style=\"display:flex;gap:6px;align-items:center;margin:8px 0 0 16px");
const underEnd = s.indexOf("</div></div>';", underStart);
if (underStart < 0) throw new Error('value row not found');
s = s.slice(0, underStart) + "'</div>';" + s.slice(underEnd + "</div></div>';".length);

// Two columns.
const layoutStart = s.indexOf('const col = ');
const layoutEnd = s.indexOf("fs.writeFileSync(path.join(here, 'tags.html')");
s = s.slice(0, layoutStart) + String.raw`const col = (x, t, d, g, bg) => '<div style="position:absolute;left:' + x + 'px;top:126px;width:900px"><div class="h" style="font-size:21px;color:#8cf0d2">' + t + '</div>' +
	'<div class="note" style="margin-top:6px;height:30px;font-size:14.5px">' + d + '</div>' +
	'<div style="position:relative;margin-top:8px;height:800px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') ' + bg + '/2403px auto">' +
	'<div style="position:absolute;left:250px;top:30px">' + boldCard(g, 'mix') + keys + '</div></div></div>';
const html = '<!doctype html><html><head><meta charset="utf-8"><style>' + css + ccss + bcss + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#11201d,#060909 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:30px;font-size:30px;color:#8cf0d2">PENNANT NOTCH + TOE-TAG POINT</div>' +
	'<div class="note" style="position:absolute;left:40px;top:72px">The pennant top notch, the toe tag cut shoulders, and its point down to the gun. 4 px outlines, bigger and heavier text and keys. The silver value is what a vendor pays for it.</div>' +
	col(40, 'A · VALUE IN THE FOOT', 'The silver it sells for sits in the foot, beside the maker.', { ...A, valueIn: true }, '-500px -260px') +
	col(980, 'B · NO VALUE', 'No silver on the card: you see it at the vendor.', { ...C }, '-800px -260px') +
	'</div></body></html>';
` + s.slice(layoutEnd);
s = s.split("'tags.html'").join("'mix.html'").split("'tags.png'").join("'mix.png'");
fs.writeFileSync(path.join(here, 'mix.js'), s);
console.log('mix.js written');
