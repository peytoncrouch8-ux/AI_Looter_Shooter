// Round 10 (the user, 2026-10-28: "I also don't know how I feel about the blue and orange style; let's get some different
// ideas for me to look at"): the compact card in six colour themes. Builds themes.js from compact.js and renders themes.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'compact.js'), 'utf8');
let head = s.slice(0, s.indexOf('const A = {'));

const tail = String.raw`
const A = { maker: 'maud', rarity: 'rare', name: 'SLUGGER PUMP SHOTGUN', level: 6,
	stats: [['dmg', 'Damage', '21 × 8', 'u'], ['acc', 'Accuracy', '62%', 'w'], ['hnd', 'Handling', '70%', 'u'], ['rel', 'Reload', '1.7 s', 'u'], ['rof', 'Fire rate', '66 rpm', 'u'], ['mag', 'Magazine', '10', 'u']],
	lines: ['Salts and slows them: wards crumble', 'Blessed Iron: +12% damage, slower fire'],
	flavor: 'Sister Agnes blessed this one twice.', element: salt, value: '412' };

// Each theme: the glass (top, foot), its line, the accent (keys, level tab), labels, text, the flavour line.
const THEMES = [
	{ id: 't1', name: '1 · NIGHT GLASS (NOW)', note: 'Navy glass, cyan lines, orange keys.', top: 'rgba(8,30,44,.92)', foot: 'rgba(6,20,30,.86)', line: 'rgba(90,200,255,.85)', acc: '#ff9f1c', lab: '#b9cddb', text: '#e8eef2', fl: '#ff7a5c', icon: '#9fb6c4', bg: '-500px -300px' },
	{ id: 't2', name: '2 · GUNMETAL & BRASS', note: 'Charcoal steel, brass lines and keys, warm white text: a gunsmith\'s bench.', top: 'rgba(34,34,36,.94)', foot: 'rgba(20,20,22,.9)', line: 'rgba(214,170,90,.9)', acc: '#d9aa55', lab: '#c9bfae', text: '#f3ece0', fl: '#e0a46a', icon: '#b8a888', bg: '-700px -300px' },
	{ id: 't3', name: '3 · BONE & INK', note: 'Pale bone card, dark ink, oxblood accents: a printed ticket from the frontier.', top: 'rgba(232,224,206,.95)', foot: 'rgba(214,204,182,.94)', line: 'rgba(90,30,24,.85)', acc: '#8e2a20', lab: '#5a5045', text: '#1e1a16', fl: '#8e2a20', icon: '#6b6052', bg: '-900px -300px', dark: true },
	{ id: 't4', name: '4 · GRAVE LIGHT', note: 'Deep green-black, ghostly mint lines, soul-light gold keys: the Revenant\'s own colours.', top: 'rgba(10,24,22,.93)', foot: 'rgba(6,16,15,.9)', line: 'rgba(140,240,210,.75)', acc: '#ffe6a6', lab: '#a8c9bf', text: '#eaf7f2', fl: '#ffd38a', icon: '#8fb3a8', bg: '-500px -420px' },
	{ id: 't5', name: '5 · EMBER BLACK', note: 'Near-black, ember red lines and keys, warm grey labels: hot, loud, simple.', top: 'rgba(16,13,12,.94)', foot: 'rgba(10,8,8,.9)', line: 'rgba(255,94,46,.85)', acc: '#ff5e2e', lab: '#bdb2aa', text: '#f5efe9', fl: '#ff9a6b', icon: '#a89c94', bg: '-700px -420px' },
	{ id: 't6', name: '6 · DUSK', note: 'Dusky plum glass, sunset amber lines and keys: Teropa at dusk.', top: 'rgba(36,22,42,.93)', foot: 'rgba(22,13,28,.9)', line: 'rgba(255,184,108,.85)', acc: '#ffb86c', lab: '#c8b6cf', text: '#f6eef8', fl: '#ff8f7a', icon: '#a898b0', bg: '-900px -420px' },
];
const themeCss = THEMES.map(t => '.' + t.id + ' .C{background:linear-gradient(180deg,' + t.top + ',' + t.foot + ')}' +
	'.' + t.id + ' .edge path,.' + t.id + ' .C > div > svg path{stroke:' + t.line + '}' +
	'.' + t.id + ' .cap{color:' + t.acc + ';box-shadow:inset 0 0 0 2px ' + t.acc + '}' +
	'.' + t.id + ' .lab{color:' + t.lab + '}.' + t.id + ' .val{color:' + t.text + '}.' + t.id + ' .bul{color:' + t.text + '}.' + t.id + ' .fl{color:' + t.fl + '}' +
	'.' + t.id + ' .K [fill="#9fb6c4"]{fill:' + t.icon + '}.' + t.id + ' .K [stroke="#9fb6c4"]{stroke:' + t.icon + '}' +
	'.' + t.id + ' .lvl{color:' + (t.dark ? '#3a332c' : t.lab) + ';background:' + t.top + '}' +
	(t.dark ? '.' + t.id + ' .nm{text-shadow:none;filter:brightness(.62) saturate(1.4)}.' + t.id + ' .u{color:#2e8b3a}.' + t.id + ' .w{color:#b3261e}' : '')).join('');

// The compact card with hooks for the themes (the level tab and the name).
const themed = (g) => compact(g).replace('LEVEL ' + g.level + '</div>', 'LEVEL ' + g.level + '</div>').replace('<div style="display:inline-block;margin-left:14px', '<div class="lvl" style="display:inline-block;margin-left:14px')
	.replace('<div style="font-family:Chakra;font-weight:700;font-size:20px;color:', '<div class="nm" style="font-family:Chakra;font-weight:700;font-size:20px;color:');
const keys = (t) => '<div style="margin:0 0 0 12px;display:flex;flex-direction:column;gap:8px;font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:1.5px;color:#f3f3f3;text-shadow:0 0 3px #000">' +
	'<span><span class="cap">E</span>TAKE</span><span><span class="cap">HOLD E</span>EQUIP</span></div>';
const cell = (t, i) => {
	const x = 40 + (i % 3) * 630, y = 110 + Math.floor(i / 3) * 480;
	return '<div class="' + t.id + '" style="position:absolute;left:' + x + 'px;top:' + y + 'px;width:600px"><div class="h" style="font-size:19px">' + t.name + '</div>' +
		'<div class="note" style="margin-top:4px;height:22px;font-size:13.5px">' + t.note + '</div>' +
		'<div style="position:relative;margin-top:6px;height:400px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') ' + t.bg + '/2403px auto">' +
		'<div style="position:absolute;left:60px;top:24px;display:flex;align-items:flex-end">' + themed(A) + '<div style="margin-bottom:46px">' + keys(t) + '</div></div></div></div>';
};
const html = '<!doctype html><html><head><meta charset="utf-8"><link href="https://fonts.googleapis.com/css2?family=Exo+2:ital,wght@0,400;0,600;0,700;1,600&display=swap" rel="stylesheet"><style>' + css + ccss + themeCss + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#1a1d22,#07080a 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:30px;font-size:30px;color:#e8e2d6">SIX COLOUR THEMES</div>' +
	'<div class="note" style="position:absolute;left:40px;top:70px">The same compact card in each. A theme would carry to the HUD, menus and inventory. Rarity colours (blue Rare here) and maker colours stay.</div>' +
	THEMES.map(cell).join('') + '</div></body></html>';
fs.writeFileSync(path.join(here, 'themes.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	await pg.goto('file:///' + path.join(here, 'themes.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1500);
	await pg.screenshot({ path: path.join(here, 'themes.png') });
	await browser.close();
	console.log('done');
})();
`;
fs.writeFileSync(path.join(here, 'themes.js'), head + tail);
console.log('themes.js written');
