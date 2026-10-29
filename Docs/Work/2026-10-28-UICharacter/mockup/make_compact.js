// Round 9 (the user, 2026-10-28, with Borderlands 2 screenshots: "Can we do something similar to this ... super nice but
// simple with all the needed details"): the same information order (level, name, a tight stat list with icons, values and
// arrows, bullet lines, a flavour line, maker and element at the foot, value), in our own Wide Sweep look. Builds
// compact.js from wide.js (sweep, edge, marks, styles) and renders compact.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'wide.js'), 'utf8');
let head = s.slice(0, s.indexOf('const LOOT ='));

const tail = String.raw`
const ccss = '.K .lr{display:grid;grid-template-columns:16px 1fr auto 14px;align-items:center;gap:6px;padding:1px 0}' +
	'.K .lab{font-family:Chakra;font-size:13px;color:#b9cddb;letter-spacing:.5px}' +
	'.K .val{font-family:Chakra;font-weight:700;font-size:14px;color:#e8eef2;text-align:right}' +
	'.K .arr{font-size:11px;text-align:right}' +
	'.K .bul{font-family:\'Exo 2\';font-size:12.5px;color:#dcefff;line-height:1.3;padding-left:10px;text-indent:-10px}' +
	'.K .fl{font-family:\'Exo 2\';font-style:italic;font-weight:600;font-size:12.5px;color:#ff7a5c;line-height:1.3}';
// Our own small glyphs for each stat (drawn here, nobody else's).
const G = {
	dmg: '<path d="M8 1 L10 6 L15 6 L11 9 L13 15 L8 11 L3 15 L5 9 L1 6 L6 6 Z" fill="#9fb6c4"/>',
	acc: '<circle cx="8" cy="8" r="5.5" fill="none" stroke="#9fb6c4" stroke-width="1.6"/><path d="M8 0 V5 M8 11 V16 M0 8 H5 M11 8 H16" stroke="#9fb6c4" stroke-width="1.6"/>',
	hnd: '<path d="M2 12 L8 3 L14 12" fill="none" stroke="#9fb6c4" stroke-width="2"/><path d="M5 14 H11" stroke="#9fb6c4" stroke-width="2"/>',
	rel: '<path d="M13 8 A5 5 0 1 1 8 3" fill="none" stroke="#9fb6c4" stroke-width="1.8"/><path d="M7 0 L11 3 L7 6 Z" fill="#9fb6c4"/>',
	rof: '<path d="M2 4 H10 M2 8 H13 M2 12 H10" stroke="#9fb6c4" stroke-width="1.8"/><path d="M12 2 L15 4 L12 6 Z M12 10 L15 12 L12 14 Z" fill="#9fb6c4"/>',
	mag: '<rect x="5" y="1" width="6" height="14" rx="1.5" fill="none" stroke="#9fb6c4" stroke-width="1.6"/><path d="M5 5 H11 M5 9 H11" stroke="#9fb6c4" stroke-width="1.2"/>',
	zoom: '<circle cx="7" cy="7" r="5" fill="none" stroke="#9fb6c4" stroke-width="1.6"/><path d="M11 11 L15 15" stroke="#9fb6c4" stroke-width="2"/>',
};
const icon = (k) => '<svg width="16" height="16" viewBox="0 0 16 16">' + G[k] + '</svg>';
const lr = ([k, l, v, a]) => '<div class="lr">' + icon(k) + '<span class="lab">' + l + '</span><span class="val">' + v + '</span><span class="arr ' + (a || '') + '">' + (a === 'u' ? '▲' : a === 'w' ? '▼' : '') + '</span></div>';
const ember = '<svg width="18" height="18" viewBox="0 0 18 18"><path d="M9 1 C12 5 15 7 14 11 C13 15 10 17 9 17 C6 17 3 15 3 11 C3 8 6 7 6 4 C8 6 8 7 9 1 Z" fill="#ff5e2e"/><path d="M9 9 C10 11 11 12 10.5 14 C10 15.5 8 15.5 7.5 14 C7 12.5 8.5 11.5 9 9 Z" fill="#ffd08a"/></svg>';
const salt = '<svg width="18" height="18" viewBox="0 0 18 18"><path d="M9 1 L16 9 L9 17 L2 9 Z" fill="none" stroke="#ccf0f6" stroke-width="2"/><path d="M9 5 L12.5 9 L9 13 L5.5 9 Z" fill="#ccf0f6"/></svg>';
const coin = '<svg width="13" height="13" viewBox="0 0 14 14"><circle cx="7" cy="7" r="6" fill="#c9d1d8" stroke="#6c757d" stroke-width="1.5"/><circle cx="7" cy="7" r="3" fill="none" stroke="#6c757d" stroke-width="1.2"/></svg>';

function compact(g) {
	const M = MAKER[g.maker];
	return '<div style="position:relative;width:340px">' +
		'<div style="display:inline-block;margin-left:14px;padding:2px 14px 3px 10px;font-family:Chakra;font-weight:700;font-size:11px;letter-spacing:2px;color:#9fb6c4;background:rgba(6,20,30,.86);clip-path:polygon(0 0,calc(100% - 10px) 0,100% 100%,0 100%)">LEVEL ' + g.level + '</div>' +
		'<div class="C K" style="position:relative;width:340px">' + edge +
		'<div style="padding:12px 16px 12px">' +
		'<div style="font-family:Chakra;font-weight:700;font-size:20px;color:' + RAR[g.rarity] + ';text-align:center;margin-bottom:6px">' + g.name + '</div>' +
		g.stats.map(lr).join('') +
		'<div style="height:1px;margin:7px 0 6px;background:linear-gradient(90deg,rgba(90,200,255,.35),transparent)"></div>' +
		g.lines.map(t => '<div class="bul">• ' + t + '</div>').join('') +
		(g.flavor ? '<div class="fl" style="margin-top:3px">' + g.flavor + '</div>' : '') +
		'<div style="display:flex;align-items:center;justify-content:space-between;margin-top:9px">' +
		'<div style="display:flex;align-items:center;gap:7px;padding:3px 12px 3px 8px;background:linear-gradient(90deg,' + M.a + ',' + M.b + ');clip-path:polygon(0 0,100% 0,calc(100% - 8px) 100%,0 100%);font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:2.5px;color:' + M.ink + '">' + MARK[g.maker](M.ink) + M.word + '</div>' +
		(g.element || '') + '</div></div></div>' +
		'<div style="display:flex;justify-content:flex-start;margin:6px 0 0 14px;gap:6px;align-items:center;font-family:Chakra;font-weight:700;font-size:13px;color:#e8eef2;text-shadow:0 0 3px #000">' + coin + g.value + '</div>' +
		'</div>';
}

const A = { maker: 'maud', rarity: 'rare', name: 'SLUGGER PUMP SHOTGUN', level: 6,
	stats: [['dmg', 'Damage', '21 × 8', 'u'], ['acc', 'Accuracy', '62%', 'w'], ['hnd', 'Handling', '70%', 'u'], ['rel', 'Reload', '1.7 s', 'u'], ['rof', 'Fire rate', '66 rpm', 'u'], ['mag', 'Magazine', '10', 'u']],
	lines: ['Salts and slows them: wards crumble', 'Blessed Iron: +12% damage, slower fire', 'Slug barrel: one heavy round a shot'],
	flavor: 'Sister Agnes blessed this one twice.', element: salt, value: '412' };
const B = { maker: 'round', rarity: 'epic', name: 'BRUTE ASSAULT RIFLE', level: 7,
	stats: [['dmg', 'Damage', '34', 'u'], ['acc', 'Accuracy', '81%', 'u'], ['hnd', 'Handling', '58%', 'w'], ['rel', 'Reload', '2.2 s', 'w'], ['rof', 'Fire rate', '540 rpm', 'u'], ['mag', 'Magazine', '30', 'u']],
	lines: ['Sets them burning', 'Rolling Stock: +15% damage, more kick'],
	flavor: '', element: ember, value: '1,280' };
const C = { maker: 'mer', rarity: 'leg', name: 'SURVEYOR SNIPER RIFLE', level: 6,
	stats: [['dmg', 'Damage', '375', 'u'], ['acc', 'Accuracy', '96%', 'u'], ['hnd', 'Handling', '44%', 'w'], ['rel', 'Reload', '3.1 s', 'w'], ['rof', 'Fire rate', '45 rpm', 'w'], ['mag', 'Magazine', '5', 'w'], ['zoom', 'Zoom', '6×', 'u']],
	lines: ['Rounds punch through an enemy into the next', 'True Glass: +10% accuracy and range'],
	flavor: 'Every mile of track, measured twice.', element: '', value: '3,900' };

const keys = '<div style="margin:0 0 0 12px;display:flex;flex-direction:column;gap:8px;font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:1.5px;color:#e8eef2;text-shadow:0 0 3px #000">' +
	'<span><span class="cap">E</span>TAKE</span><span><span class="cap">HOLD E</span>EQUIP</span></div>';
const col = (x, t, d, g) => '<div style="position:absolute;left:' + x + 'px;top:130px;width:600px"><div class="h" style="font-size:20px">' + t + '</div>' +
	'<div class="note" style="margin-top:6px;height:40px;font-size:14px">' + d + '</div>' +
	'<div style="position:relative;margin-top:8px;height:700px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') -' + (500 + x / 3) + 'px -300px/2403px auto">' +
	'<div style="position:absolute;left:60px;top:60px;display:flex;align-items:flex-end">' + compact(g) + '<div style="margin-bottom:46px">' + keys + '</div></div></div></div>';
const html = '<!doctype html><html><head><meta charset="utf-8"><link href="https://fonts.googleapis.com/css2?family=Exo+2:ital,wght@0,400;0,600;0,700;1,600&display=swap" rel="stylesheet"><style>' + css + ccss + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#132634,#05080b 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:32px;font-size:30px">THE COMPACT CARD</div>' +
	'<div class="note" style="position:absolute;left:40px;top:74px">Everything in one tight panel: level, name, stats with icons and arrows, what\'s special, a flavour line, maker and element, value. Keys beside it.</div>' +
	col(40, 'A SAINT MAUD SHOTGUN', 'Grave-salt element; its maker trait and its barrel part as lines; a flavour line.', A) +
	col(670, 'AN EPIC ROUNDHOUSE RIFLE', 'Ember element; no flavour line (only some guns have one).', B) +
	col(1300, 'A LEGENDARY MERIDIAN SNIPER', 'A sniper adds its zoom; Piercing part; no element.', C) +
	'</div></body></html>';
fs.writeFileSync(path.join(here, 'compact.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	await pg.goto('file:///' + path.join(here, 'compact.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1500);
	await pg.screenshot({ path: path.join(here, 'compact.png') });
	await browser.close();
	console.log('done');
})();
`;
fs.writeFileSync(path.join(here, 'compact.js'), head + tail);
console.log('compact.js written');
