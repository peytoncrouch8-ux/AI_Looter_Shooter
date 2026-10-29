// Round 15 (the user, 2026-10-28: "I like concept C, let's remove the value and give me some more color options before I
// decide finally"): concept C (the level gem in the notch) without the silver value, in six colour themes. Builds colors.js
// from dist.js and renders colors.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'dist.js'), 'utf8');
let head = s.slice(0, s.indexOf('const SH = {'));
// No value on the card.
head = head.split("coin.replace(/13/g, '16') + g.value").join("(g.value ? coin.replace(/13/g, '16') + g.value : '')");
// Theme colours are read from GL when a card is made, so the card's inline colours follow the theme.
head = head.replace('const GL = {', 'let GL = {');

const tail = String.raw`
const THEMES = [
	{ id: 'h1', name: '1 · GRAVE LIGHT', note: 'Green-black, ghostly mint, soul-light gold.', top: 'rgba(12,28,26,.95)', foot: 'rgba(7,17,16,.93)', line: '#8cf0d2', acc: '#ffe6a6', lab: '#a8c9bf', text: '#eef9f5', fl: '#ffd38a', icon: '#8fb3a8', band: 'rgba(140,240,210,.06)', bg: '-500px -260px' },
	{ id: 'h2', name: '2 · GUNMETAL & BRASS', note: 'Charcoal steel, brass lines, a warm cream gem.', top: 'rgba(34,34,36,.95)', foot: 'rgba(20,20,22,.93)', line: '#d6aa5a', acc: '#f3e3bf', lab: '#c9bfae', text: '#f6efe2', fl: '#e8b77a', icon: '#b8a888', band: 'rgba(214,170,90,.07)', bg: '-650px -260px' },
	{ id: 'h3', name: '3 · OXBLOOD & BONE', note: 'Deep oxblood glass, bone-white lines, old gold.', top: 'rgba(48,14,16,.95)', foot: 'rgba(28,8,10,.93)', line: '#efe3c8', acc: '#e0b65a', lab: '#d8bfb6', text: '#fbf3e6', fl: '#f0c27a', icon: '#c4a69c', band: 'rgba(239,227,200,.06)', bg: '-800px -260px' },
	{ id: 'h4', name: '4 · SAGE & COPPER', note: 'Olive-sage glass, copper lines, cream: prairie colours.', top: 'rgba(30,36,24,.95)', foot: 'rgba(18,22,14,.93)', line: '#d98a52', acc: '#f2e6c4', lab: '#c3c9ae', text: '#f5f3e6', fl: '#f0b27a', icon: '#a9ae92', band: 'rgba(217,138,82,.07)', bg: '-500px -380px' },
	{ id: 'h5', name: '5 · DUSK', note: 'Plum glass, sunset amber lines, rose gold.', top: 'rgba(36,22,42,.95)', foot: 'rgba(22,13,28,.93)', line: '#ffb86c', acc: '#ffd9c2', lab: '#c8b6cf', text: '#f8f0fa', fl: '#ff9a86', icon: '#a898b0', band: 'rgba(255,184,108,.07)', bg: '-650px -380px' },
	{ id: 'h6', name: '6 · SLATE & SALT', note: 'Slate grey, pale grave-salt lines, soul-light gold.', top: 'rgba(30,34,40,.95)', foot: 'rgba(18,21,26,.93)', line: '#d4eef4', acc: '#ffe6a6', lab: '#b4bec8', text: '#f2f6f8', fl: '#ffd38a', icon: '#98a4ae', band: 'rgba(212,238,244,.06)', bg: '-800px -380px' },
];
const themeCss = THEMES.map(t => {
	const p = '.' + t.id + ' ';
	return p + '.Bc .lab{color:' + t.lab + '}' + p + '.Bc .val{color:' + t.text + '}' + p + '.Bc .bul{color:' + t.text + '}' + p + '.Bc .fl{color:' + t.fl + '}' +
		p + '.Bc [fill="#9fb6c4"]{fill:' + t.icon + '}' + p + '.Bc [stroke="#9fb6c4"]{stroke:' + t.icon + '}' +
		p + '.gcap{color:' + t.acc + ';box-shadow:inset 0 0 0 3.5px ' + t.acc + ',0 0 0 2px rgba(0,0,0,.6)}';
}).join('');
const RI = { maker: 'round', rarity: 'epic', name: 'BRUTE ASSAULT RIFLE', level: 7, kind: 'Assault rifle',
	stats: [['dmg', 'Damage', '34', 'u'], ['acc', 'Accuracy', '81%', 'u'], ['hnd', 'Handling', '58%', 'w'], ['rel', 'Reload', '2.2 s', 'w'], ['rof', 'Fire rate', '540 rpm', 'u'], ['mag', 'Magazine', '30', 'u']],
	lines: ['Sets them burning', 'Rolling Stock: +15% damage, more kick'], flavor: 'Thrown off a flatcar twice. Still fires.', element: ember, value: '' };
const base = GL;
const cell = (t, i) => {
	GL = { ...base, top: t.top, foot: t.foot, line: t.line, acc: t.acc, lab: t.lab, text: t.text, fl: t.fl, icon: t.icon };
	const card = distCard(RI, 'C').split('rgba(140,240,210,.06)').join(t.band).split('rgba(140,240,210,.16)').join(t.line + '33');
	const x = 40 + (i % 3) * 630, y = 108 + Math.floor(i / 3) * 488;
	return '<div class="' + t.id + '" style="position:absolute;left:' + x + 'px;top:' + y + 'px;width:600px"><div class="h" style="font-size:19px;color:' + t.line + '">' + t.name + '</div>' +
		'<div class="note" style="margin-top:4px;height:22px;font-size:13.5px">' + t.note + '</div>' +
		'<div style="position:relative;margin-top:6px;height:420px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') ' + t.bg + '/2403px auto">' +
		'<div style="position:absolute;left:100px;top:20px;transform:scale(.78);transform-origin:top left">' + card + keys + '</div></div></div>';
};
const cells = THEMES.map(cell).join('');
const html = '<!doctype html><html><head><meta charset="utf-8"><style>' + css + ccss + bcss + themeCss + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#1a1d22,#07080a 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:30px;font-size:30px;color:#efe8da">CONCEPT C IN SIX COLOURS</div>' +
	'<div class="note" style="position:absolute;left:40px;top:70px">No value on the card. Rarity (purple Epic) and the maker\'s badge keep their own colours; the theme would carry across the HUD, menus and inventory.</div>' +
	cells + '</div></body></html>';
fs.writeFileSync(path.join(here, 'colors.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	await pg.goto('file:///' + path.join(here, 'colors.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1200);
	await pg.screenshot({ path: path.join(here, 'colors.png') });
	await browser.close();
	console.log('done');
})();
`;
fs.writeFileSync(path.join(here, 'colors.js'), head + tail);
console.log('colors.js written');
