// Round 8 (the user, 2026-10-28): "Borderlands-style list": the stats as a list of big numbers with arrows, the maker's
// mark in the corner, no bars; on the Wide Sweep card. Builds list.js from wide.js (sweep, edge, styles) and renders list.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'wide.js'), 'utf8');
let head = s.slice(0, s.indexOf('const LOOT ='));

const tail = String.raw`
const listCss = '.L .lrow{display:flex;align-items:baseline;justify-content:space-between;padding:4px 0;border-bottom:1px solid rgba(90,200,255,.12)}' +
	'.L .lab{font-family:Chakra;font-size:12px;letter-spacing:2px;color:#9fb6c4}' +
	'.L .val{font-family:Chakra;font-weight:700;font-size:21px;color:#e8eef2;letter-spacing:.5px}' +
	'.L .val small{font-size:13px;color:#9fb6c4;font-weight:400;margin-left:2px}' +
	'.L .arr{display:inline-block;width:16px;text-align:right;font-size:13px;margin-left:6px}' +
	'.L .grid{display:grid;grid-template-columns:1fr 1fr;column-gap:26px}';
const STATS = [['DAMAGE', '21<small>×8</small>', 'u'], ['ACCURACY', '62<small>%</small>', 'w'], ['HANDLING', '70<small>%</small>', 'u'],
	['RELOAD', '1.7<small>s</small>', 'u'], ['FIRE RATE', '66<small>rpm</small>', 'u'], ['MAGAZINE', '10', 'u']];
const row = ([l, v, a]) => '<div class="lrow"><span class="lab">' + l + '</span><span><span class="val">' + v + '</span><span class="arr ' + a + '">' + (a === 'u' ? '▲' : '▼') + '</span></span></div>';
const markBig = MARK.maud('#9a6a1c').replace('width="15" height="15"', 'width="58" height="58"');
function listCard(cols, width) {
	const M = MAKER.maud;
	const body = cols === 1 ? STATS.map(row).join('') : '<div class="grid"><div>' + STATS.slice(0, 3).map(row).join('') + '</div><div>' + STATS.slice(3).map(row).join('') + '</div></div>';
	return '<div class="C L" style="position:relative;width:' + width + 'px">' + edge +
		'<div class="top">' + sweep('maud') + '<div class="mw" style="color:' + M.ink + '">' + MARK.maud(M.ink) + M.word + '</div><span class="v u">▲ UPGRADE</span></div>' +
		'<div style="padding:8px 20px 14px">' +
		'<div style="font-family:Chakra;font-weight:700;font-size:21px;color:' + RAR.rare + '">SLUGGER PUMP SHOTGUN</div>' +
		'<div style="font-family:\'Exo 2\';font-size:13px;color:#9fb6c4;margin:-1px 0 8px">Level 6 · Pump shotgun</div>' + body +
		'<div style="display:flex;align-items:flex-end;justify-content:space-between;margin-top:10px">' +
		'<div style="font-family:\'Exo 2\';font-size:13px;font-weight:700;color:#ccf0f6">◆ GRAVE-SALT <span style="font-weight:400;color:#9fb6c4">salts and slows them</span></div>' +
		'<div style="opacity:.85;margin:-20px -4px -6px 0">' + markBig + '</div></div></div></div>';
}
const keysRow = '<div style="display:flex;gap:22px;justify-content:center;margin-top:8px;font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:1.5px;color:#e8eef2;text-shadow:0 0 3px #000"><span><span class="cap">E</span>TAKE</span><span><span class="cap">HOLD E</span>EQUIP</span></div>';
const col = (x, t, d, html, w) => '<div style="position:absolute;left:' + x + 'px;top:140px;width:880px"><div class="h">' + t + '</div>' +
	'<div class="note" style="margin-top:8px;height:40px">' + d + '</div>' +
	'<div style="position:relative;margin-top:10px;height:600px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') -700px -380px/2403px auto">' +
	'<div style="position:absolute;left:' + ((880 - w) / 2) + 'px;top:60px;width:' + w + 'px">' + html + keysRow + '</div></div></div>';
const html = '<!doctype html><html><head><meta charset="utf-8"><link href="https://fonts.googleapis.com/css2?family=Exo+2:wght@400;600;700&display=swap" rel="stylesheet"><style>' + css + listCss + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#132634,#05080b 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:36px;font-size:30px">THE STATS AS A LIST</div>' +
	'<div class="note" style="position:absolute;left:40px;top:80px">Big numbers with arrows against your gun, no bars; the maker\'s mark in the corner. Same sweep, lines and keys as now.</div>' +
	col(40, 'A · ONE COLUMN', 'Six stats down the card: easy to scan top to bottom. Taller, narrower.', listCard(1, 380), 380) +
	col(1000, 'B · TWO COLUMNS', 'Three and three: wider and shorter, covers less of the world.', listCard(2, 500), 500) +
	'</div></body></html>';
fs.writeFileSync(path.join(here, 'list.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	await pg.goto('file:///' + path.join(here, 'list.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1500);
	await pg.screenshot({ path: path.join(here, 'list.png') });
	await browser.close();
	console.log('done');
})();
`;
fs.writeFileSync(path.join(here, 'list.js'), head + tail);
console.log('list.js written');
