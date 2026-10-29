// Round 7 (the user, 2026-10-28: "None of these, give me more options. Think outside the box or from a different
// perspective"): six other things for the card's left side. Builds ideas.js from left.js's card (damage as a bar) and
// renders ideas.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'left.js'), 'utf8');
let head = s.slice(0, s.indexOf('// A shotgun\'s Inked icon'));

const tail = String.raw`
const inv = 'file:///C:/Dev/AI_Looter_Shooter/Saved/Screenshots/Tools/ui_inv2.png';
const T = (t, c = '#9fb6c4', sz = 11, sp = 2) => '<div style="font-family:Chakra;font-size:' + sz + 'px;letter-spacing:' + sp + 'px;color:' + c + ';text-align:center">' + t + '</div>';
const B = (t, c = '#e8eef2', sz = 22) => '<div style="font-family:Chakra;font-weight:700;font-size:' + sz + 'px;color:' + c + ';line-height:1">' + t + '</div>';

// 1. The real gun, its parts and paint, turning slowly (the inventory's showcase render, small).
const real = '<div style="width:150px;height:34px;margin-top:10px;background:url(\'' + inv + '\') -298px -124px/755px 412px no-repeat"></div>' +
	'<div style="width:120px;height:14px;margin-top:2px;border-radius:50%;box-shadow:0 0 0 1px rgba(124,170,255,.5),0 0 14px rgba(124,170,255,.3)"></div>' + T('TURNING SLOWLY', '#9fb6c4', 10);
// 2. One gauge: how much better it is overall (damage a second) than the gun in hand.
const arc = (pct) => { const r = 38, c = 2 * Math.PI * r * 0.75, f = c * Math.min(1, pct); return '<svg width="96" height="84" viewBox="0 0 96 84"><g transform="rotate(135 48 48)">' +
	'<circle cx="48" cy="48" r="' + r + '" fill="none" stroke="rgba(90,200,255,.15)" stroke-width="7" stroke-dasharray="' + c + ' 999"/>' +
	'<circle cx="48" cy="48" r="' + r + '" fill="none" stroke="#6dff7a" stroke-width="7" stroke-dasharray="' + f + ' 999"/></g>' +
	'<text x="48" y="54" text-anchor="middle" font-family="Chakra" font-weight="700" font-size="20" fill="#6dff7a">+38%</text></svg>'; };
const gauge = arc(0.69) + T('BETTER THAN YOURS', '#9fb6c4', 10);
// 3. Where its shots land: a target with this gun's pellet spread, your gun's as faint ghosts.
const dots = (pts, c, o) => pts.map(([x, y]) => '<circle cx="' + x + '" cy="' + y + '" r="2.6" fill="' + c + '" opacity="' + o + '"/>').join('');
const pattern = '<svg width="90" height="90" viewBox="0 0 90 90"><circle cx="45" cy="45" r="40" fill="none" stroke="rgba(90,200,255,.35)" stroke-width="1.5"/>' +
	'<circle cx="45" cy="45" r="24" fill="none" stroke="rgba(90,200,255,.25)" stroke-width="1"/><path d="M45 5 V85 M5 45 H85" stroke="rgba(90,200,255,.18)"/>' +
	dots([[30, 22], [62, 30], [20, 52], [70, 60], [40, 72], [56, 16], [14, 34], [76, 42]], '#e8eef2', 0.35) +
	dots([[38, 36], [52, 34], [36, 52], [55, 55], [45, 60], [48, 28], [30, 44], [60, 44]], '#ff9f1c', 1) + '</svg>' + T('TIGHTER SPREAD', '#9fb6c4', 10);
// 4. The ammo you carry for it: can you use it right now?
const shell = '<svg width="22" height="44" viewBox="0 0 22 44"><rect x="3" y="2" width="16" height="30" rx="3" fill="#c0392b" stroke="#111" stroke-width="2"/><rect x="2" y="30" width="18" height="12" rx="2" fill="#d9a441" stroke="#111" stroke-width="2"/></svg>';
const ammo = '<div style="display:flex;gap:4px">' + shell + shell + shell + '</div><div style="margin-top:6px">' + B('24<span style="font-size:13px;color:#9fb6c4"> / 48</span>') + '</div>' + T('SHELLS CARRIED', '#9fb6c4', 10);
// 5. Its story: where it came from and who carried it, stamped like a pawn ticket.
const story = '<div style="transform:rotate(-6deg);padding:7px 10px;border:2px solid rgba(255,159,28,.8);clip-path:polygon(6px 0,100% 0,100% calc(100% - 6px),calc(100% - 6px) 100%,0 100%,0 6px)">' +
	T('DROPPED BY', '#ff9f1c', 9) + B('GRAVEMOTHER', '#ff9f1c', 15) + T('RANSOM\'S REST', '#ff9f1c', 9) + '</div>' +
	'<div style="margin-top:10px">' + T('||||  ||||  ||  \u00B7  12 NOTCHES', '#e8eef2', 10, 1) + '</div>';
// 6. What it's built from: its parts, each with its tier's pips.
const part = (n, k) => '<div style="display:flex;justify-content:space-between;width:120px;font-family:Chakra;font-size:10px;letter-spacing:1.5px;color:#9fb6c4;margin:3px 0"><span>' + n + '</span><span>' +
	[0, 1, 2].map(i => '<span style="display:inline-block;width:9px;height:9px;margin-left:3px;transform:rotate(45deg) scale(.8);background:' + (i < k ? '#7caaff' : 'rgba(124,170,255,.18)') + '"></span>').join('') + '</span></div>';
const parts = part('BARREL', 2) + part('ACTION', 3) + part('STOCK', 1) + part('SIGHT', 2) + T('BUILT FROM 4 PARTS', '#9fb6c4', 9);

const base = { maker: 'maud', rarity: 'rare', name: 'SLUGGER PUMP SHOTGUN', level: 6, kind: 'Pump shotgun', verdict: 'UPGRADE', keys: false,
	stats: [['DAMAGE', 0.62, 0.3], ['FIRE RATE', 0.32, 0.25], ['MAGAZINE', 0.4, 0.3], ['RELOAD', 0.7, 0.55]],
	nums: ['21 ×8', '66 RPM', '10', '1.7 s'] };
const ideas = [
	['1 \u00B7 THE REAL GUN', 'Its own model, with the parts and paint it rolled, turning slowly. Every gun looks like itself.', real],
	['2 \u00B7 ONE GAUGE', 'A single dial: how much better it is than yours overall. The card\'s answer in one look.', gauge],
	['3 \u00B7 WHERE SHOTS LAND', 'Its spread on a target, your gun\'s as faint dots: accuracy you can see.', pattern],
	['4 \u00B7 YOUR AMMO FOR IT', 'The rounds you carry for it, so you know if you can fight with it now (red at zero).', ammo],
	['5 \u00B7 ITS STORY', 'Who dropped it and where, stamped like a pawn ticket; its notches under it.', story],
	['6 \u00B7 ITS PARTS', 'The parts it\'s built from, each with its tier, ties to the gunsmith\'s bench.', parts],
];
const cell = (i) => {
	const [t, d, left] = ideas[i];
	const x = 40 + (i % 3) * 630, y = 120 + Math.floor(i / 3) * 470;
	return '<div style="position:absolute;left:' + x + 'px;top:' + y + 'px;width:600px"><div class="h" style="font-size:20px">' + t + '</div>' +
		'<div class="note" style="margin-top:6px;height:40px;font-size:14px">' + d + '</div>' +
		'<div style="position:relative;margin-top:8px;height:300px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') -900px -480px/2403px auto">' +
		'<div style="position:absolute;left:65px;top:50px;width:470px">' + card({ ...base, left }, 'position:relative;width:470px') + '</div></div></div>';
};
const html = '<!doctype html><html><head><meta charset="utf-8"><link href="https://fonts.googleapis.com/css2?family=Exo+2:wght@400;600;700&display=swap" rel="stylesheet"><style>' + css + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#132634,#05080b 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:34px;font-size:30px">SIX OTHER IDEAS FOR THE LEFT SIDE</div>' +
	'<div class="note" style="position:absolute;left:40px;top:76px">From different angles: what the gun is, whether to take it, how it shoots, whether you can use it, where it came from, what it\'s made of.</div>' +
	[0, 1, 2, 3, 4, 5].map(cell).join('') + '</div></body></html>';
fs.writeFileSync(path.join(here, 'ideas.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	await pg.goto('file:///' + path.join(here, 'ideas.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1500);
	await pg.screenshot({ path: path.join(here, 'ideas.png') });
	await browser.close();
	console.log('done');
})();
`;
fs.writeFileSync(path.join(here, 'ideas.js'), head + tail);
console.log('ideas.js written');
