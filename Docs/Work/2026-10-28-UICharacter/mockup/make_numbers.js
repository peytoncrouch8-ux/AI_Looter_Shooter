// Round 5 (the user, 2026-10-28: "can I see what you would make it look like if it had number values?"): builds
// numbers.js from wide.js (its card, sweep and styles) with numbers after the bars, then numbers.js renders numbers.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'wide.js'), 'utf8');
let head = s.slice(0, s.indexOf('const LOOT ='));

const oldRow = 'return `<div class="st"><span>${l}</span>${segBar(v, o)}${a}</div>`;';
if (!head.includes(oldRow)) throw new Error('stat row not found');
head = head.replace(oldRow, [
	"const n = g.nums ? g.nums[i] : null;",
	"\t\tconst num = n ? `<span class=\"nv\">${n[0]}</span>` + (n[1] ? `<span class=\"nd ${n[2]}\">${n[1]}</span>` : '') : '';",
	"\t\treturn `<div class=\"st ${g.nums ? (g.delta2 ? 'st3' : 'st2') : ''}\"><span>${l}</span>${segBar(v, o)}${num}${a}</div>`;",
].join('\n'));
head = head.replace('const stats = g.stats.map(([l, v, o]) => {', 'const stats = g.stats.map(([l, v, o], i) => {');
head = head.replace('.C .t{', '.C .st2{grid-template-columns:74px 1fr 54px 12px}.C .st3{grid-template-columns:74px 1fr 40px 40px 12px}' +
	'.nv{font-family:Chakra;font-weight:700;font-size:12px;color:#e8eef2;text-align:right}' +
	'.nd{font-family:Chakra;font-weight:700;font-size:11px;text-align:right}.nd.u{color:#6dff7a}.nd.w{color:#ff7a6b}\n.C .t{');

const tail = String.raw`
const base = { maker: 'mer', rarity: 'leg', name: 'ARMORED SNIPER RIFLE', level: 6, kind: 'Sniper rifle', dmg: 375, pellets: 0, delta: 345,
	stats: [['FIRE RATE', 0.1, 0.62], ['MAGAZINE', 0.12, 0.6], ['RELOAD', 0.42, 0.7]], verdict: null, keys: false };
const A = { ...base };
const B = { ...base, nums: [['45 RPM'], ['5'], ['3.1 s']] };
const C = { ...base, delta2: true, nums: [['45', '-555', 'w'], ['5', '-25', 'w'], ['3.1s', '+1.0', 'w']] };
const keysRow = '<div style="display:flex;gap:22px;justify-content:center;margin-top:8px;font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:1.5px;color:#e8eef2;text-shadow:0 0 3px #000"><span><span class="cap">E</span>TAKE</span><span><span class="cap">HOLD E</span>EQUIP</span></div>';
const col = (x, t, d, g) => '<div style="position:absolute;left:' + x + 'px;top:140px;width:540px"><div class="h">' + t + '</div>' +
	'<div class="note" style="margin-top:8px;height:44px">' + d + '</div>' +
	'<div style="position:relative;margin-top:12px;height:330px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') -900px -480px/2403px auto">' +
	'<div style="position:absolute;left:50px;top:40px;width:440px">' + card(g, 'position:relative;width:440px') + keysRow + '</div></div></div>';
const html = '<!doctype html><html><head><meta charset="utf-8"><link href="https://fonts.googleapis.com/css2?family=Exo+2:wght@400;600;700&display=swap" rel="stylesheet"><style>' + css + '</style></head><body>' +
	'<div class="sheet" style="height:640px;background:radial-gradient(ellipse at 50% 20%,#132634,#05080b 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:36px;font-size:30px">THE CARD WITH NUMBERS</div>' +
	'<div class="note" style="position:absolute;left:40px;top:80px">Same gun (a legendary sniper rifle) against the rifle in your hand. The bars stay; the numbers sit after them.</div>' +
	col(40, 'NOW', 'Bars only: the orange tick is your gun, the arrow says better or worse.', A) +
	col(690, 'A · VALUES', 'Each bar ends in its number: rounds a minute, rounds a magazine, reload seconds.', B) +
	col(1340, 'B · VALUES + CHANGE', 'Its number, then how far it is from your gun in green or red.', C) +
	'</div></body></html>';
fs.writeFileSync(path.join(here, 'numbers.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 640 } });
	await pg.goto('file:///' + path.join(here, 'numbers.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1500);
	await pg.screenshot({ path: path.join(here, 'numbers.png') });
	await browser.close();
	console.log('done');
})();
`;
fs.writeFileSync(path.join(here, 'numbers.js'), head + tail);
console.log('numbers.js written');
