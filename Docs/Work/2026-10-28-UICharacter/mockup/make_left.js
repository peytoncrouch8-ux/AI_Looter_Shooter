// Round 6 (the user, 2026-10-28: "the damage stat should be alongside the RPM and mag stats instead of bottom left, but we
// will need to fill that empty spot"): damage becomes the first bar; three ways to fill the left column. Builds left.js
// from wide.js (its card, sweep and styles) and renders left.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'wide.js'), 'utf8');
let head = s.slice(0, s.indexOf('const LOOT ='));

const oldRow = 'return `<div class="st"><span>${l}</span>${segBar(v, o)}${a}</div>`;';
if (!head.includes(oldRow)) throw new Error('stat row not found');
head = head.replace(oldRow, 'return `<div class="st st2"><span>${l}</span>${segBar(v, o)}<span class="nv">${g.nums[i]}</span>${a}</div>`;');
head = head.replace('const stats = g.stats.map(([l, v, o]) => {', 'const stats = g.stats.map(([l, v, o], i) => {');
const oldDamage = "<div class=\"dm\"><b>${g.dmg}</b>${g.pellets ? `<span class=\"x\">×${g.pellets}</span>` : ''}<span>DAMAGE ${g.delta ? `<b class=\"u\">▲ +${g.delta}</b>` : ''}</span></div>";
if (!head.includes(oldDamage)) throw new Error('damage block not found');
head = head.replace(oldDamage, '<div class="left">${g.left}</div>');
head = head.replace('.C .t{', '.C .st2{grid-template-columns:74px 1fr 56px 12px}' +
	'.nv{font-family:Chakra;font-weight:700;font-size:12px;color:#e8eef2;text-align:right}' +
	'.left{display:flex;flex-direction:column;justify-content:center;align-items:center;height:100%;min-height:96px}\n.C .t{');

const tail = String.raw`
// A shotgun's Inked icon, cut from the inventory's slot row (Saved/Screenshots/Tools/ui_inv2.png).
const inv = 'file:///C:/Dev/AI_Looter_Shooter/Saved/Screenshots/Tools/ui_inv2.png';
const icon = '<div style="width:98px;height:30px;margin:8px 0;background:url(\'' + inv + '\') -287px -290px/1936px 1056px no-repeat;transform:scale(1.45);transform-origin:center;filter:drop-shadow(0 0 6px rgba(124,170,255,.35))"></div>';
const mark = MARK.maud('#9a6a1c').replace('width="15" height="15"', 'width="78" height="78"');
const base = { maker: 'maud', rarity: 'rare', name: 'SLUGGER PUMP SHOTGUN', level: 6, kind: 'Pump shotgun', verdict: 'UPGRADE', keys: false,
	stats: [['DAMAGE', 0.62, 0.3], ['FIRE RATE', 0.32, 0.25], ['MAGAZINE', 0.4, 0.3], ['RELOAD', 0.7, 0.55]],
	nums: ['21 ×8', '66 RPM', '10', '1.7 s'] };
const A = { ...base, left: icon + '<div style="margin-top:14px;font-family:Chakra;font-size:11px;letter-spacing:2px;color:#9fb6c4">PUMP SHOTGUN</div>' };
const B = { ...base, left: '<div style="opacity:.9">' + mark + '</div><div style="margin-top:4px;font-family:Chakra;font-size:11px;letter-spacing:2px;color:#9fb6c4">FURNACE HOLLOW</div>' };
const C = { ...base, left: '<div style="font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:3px;color:#7caaff">RARE</div>' +
	'<div style="position:relative;width:64px;height:64px;margin-top:6px"><svg width="64" height="64" viewBox="0 0 64 64"><path d="M32 3 L61 32 L32 61 L3 32 Z" fill="rgba(124,170,255,.18)" stroke="#7caaff" stroke-width="2.5"/></svg>' +
	'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:26px;color:#e8eef2">6</div></div>' +
	'<div style="margin-top:4px;font-family:Chakra;font-size:11px;letter-spacing:2px;color:#9fb6c4">LEVEL</div>' };
const keysRow = '<div style="display:flex;gap:22px;justify-content:center;margin-top:8px;font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:1.5px;color:#e8eef2;text-shadow:0 0 3px #000"><span><span class="cap">E</span>TAKE</span><span><span class="cap">HOLD E</span>EQUIP</span></div>';
const col = (x, t, d, g) => '<div style="position:absolute;left:' + x + 'px;top:140px;width:560px"><div class="h">' + t + '</div>' +
	'<div class="note" style="margin-top:8px;height:44px">' + d + '</div>' +
	'<div style="position:relative;margin-top:12px;height:350px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') -900px -480px/2403px auto">' +
	'<div style="position:absolute;left:45px;top:40px;width:470px">' + card(g, 'position:relative;width:470px') + keysRow + '</div></div></div>';
const html = '<!doctype html><html><head><meta charset="utf-8"><link href="https://fonts.googleapis.com/css2?family=Exo+2:wght@400;600;700&display=swap" rel="stylesheet"><style>' + css + '</style></head><body>' +
	'<div class="sheet" style="height:660px;background:radial-gradient(ellipse at 50% 20%,#132634,#05080b 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:36px;font-size:30px">DAMAGE WITH THE OTHER STATS: WHAT FILLS THE LEFT?</div>' +
	'<div class="note" style="position:absolute;left:40px;top:80px">Damage is now the first bar with its value. Three ways to use the space it left.</div>' +
	col(40, 'A \u00B7 THE GUN', 'The gun\'s own picture (the inked icon the HUD and inventory use) and its kind.', A) +
	col(680, 'B \u00B7 THE MAKER', 'The maker\'s mark big, in its colour, and where it\'s made.', B) +
	col(1320, 'C \u00B7 RARITY AND LEVEL', 'The rarity word over a level gem, so you see both at a glance.', C) +
	'</div></body></html>';
fs.writeFileSync(path.join(here, 'left.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 660 } });
	await pg.goto('file:///' + path.join(here, 'left.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1500);
	await pg.screenshot({ path: path.join(here, 'left.png') });
	await browser.close();
	console.log('done');
})();
`;
fs.writeFileSync(path.join(here, 'left.js'), head + tail);
console.log('left.js written');
