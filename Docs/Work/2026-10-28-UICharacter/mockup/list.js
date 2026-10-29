// Round 4 (the user, 2026-10-28): "Wide sweep, less curvy slightly more futuristic, the Hold E buttons should be hollow".
// The card in the world, the in-hand card, the four makers and the inventory rows. node wide.js -> wide.png
const fs = require('fs');
const path = require('path');
const here = __dirname;
const shot = 'file:///C:/Dev/AI_Looter_Shooter/Saved/Screenshots/Tools/maud_shotgun.png';
const RAR = { common: '#e7e7e7', unc: '#7ce77c', rare: '#7caaff', epic: '#cc7cf4', leg: '#ffcb00' };
const MARK = {
	round: c => `<svg width="15" height="15" viewBox="0 0 100 100"><circle cx="50" cy="50" r="40" fill="none" stroke="${c}" stroke-width="10"/>${[0, 45, 90, 135].map(a => `<line x1="50" y1="12" x2="50" y2="88" stroke="${c}" stroke-width="7" transform="rotate(${a} 50 50)"/>`).join('')}<circle cx="50" cy="50" r="13" fill="${c}"/></svg>`,
	maud: c => `<svg width="15" height="15" viewBox="0 0 100 100"><ellipse cx="50" cy="20" rx="24" ry="8" fill="none" stroke="${c}" stroke-width="6"/><path d="M14 42 H80 Q92 42 94 34 L96 34 Q96 52 78 54 L70 54 L66 64 L74 72 L74 84 L26 84 L26 72 L34 64 L30 54 Q16 54 14 42 Z" fill="${c}"/></svg>`,
	wind: c => `<svg width="15" height="15" viewBox="0 0 100 100"><path d="M50 8 V58" stroke="${c}" stroke-width="9" stroke-linecap="round"/><path d="M50 58 Q50 84 26 80 Q14 76 16 64 M50 58 Q50 84 74 80 Q86 76 84 64" fill="none" stroke="${c}" stroke-width="9" stroke-linecap="round"/><path d="M10 36 Q30 26 46 34 M54 34 Q74 42 92 28" fill="none" stroke="${c}" stroke-width="6" stroke-linecap="round"/></svg>`,
	mer: c => `<svg width="15" height="15" viewBox="0 0 100 100"><circle cx="50" cy="54" r="34" fill="none" stroke="${c}" stroke-width="8"/><path d="M50 26 V44 M50 64 V82 M22 54 H40 M60 54 H78" stroke="${c}" stroke-width="6"/><circle cx="50" cy="54" r="4" fill="${c}"/></svg>`,
};
// Each maker's sweep: its colours, its ink, and a trim drawn along the sweep's foot.
const MAKER = {
	round: { word: 'ROUNDHOUSE', quirk: 'ROLLING STOCK', a: '#8e3a2a', b: '#6b2a22', ink: '#efe3c4', trim: '#d9a441', deco: 'rivets' },
	maud: { word: 'SAINT MAUD', quirk: 'BLESSED IRON', a: '#e8e0cc', b: '#cfc4a8', ink: '#3a2a10', trim: '#9a6a1c', deco: 'gilt' },
	wind: { word: 'WINDFALL', quirk: 'LOOSE FIT', a: '#4f8f8a', b: '#9a5534', ink: '#fff6e0', trim: '#e6b43a', deco: 'patch' },
	mer: { word: 'MERIDIAN', quirk: 'TRUE GLASS', a: '#2f4766', b: '#1f2a38', ink: '#dfe6ea', trim: '#c99a4a', deco: 'ticks' },
};

const css = `
@font-face{font-family:Chakra;src:url('file:///C:/Dev/AI_Looter_Shooter/Content/UI/Fonts/ChakraPetch-Regular.ttf');font-weight:400}
@font-face{font-family:Chakra;src:url('file:///C:/Dev/AI_Looter_Shooter/Content/UI/Fonts/ChakraPetch-Bold.ttf');font-weight:700}
*{box-sizing:border-box;margin:0;padding:0}
body{background:#05080b;color:#e8eef2}
.sheet{position:relative;width:1920px;height:1080px;overflow:hidden;font-family:Chakra}
.u{color:#6dff7a}.w{color:#ff7a6b}
/* the card: tight corners, two angled cuts, a hairline frame that fades, segmented bars */
.C{position:absolute;width:420px;font-family:'Exo 2',sans-serif;
 background:linear-gradient(180deg,rgba(8,30,44,.92),rgba(6,20,30,.86));
 clip-path:polygon(0 0,calc(100% - 16px) 0,100% 16px,100% 100%,16px 100%,0 calc(100% - 16px));border-radius:4px}
.C .edge{position:absolute;inset:0;pointer-events:none}
.C .top{position:relative;height:28px}
.C .sweep{position:absolute;left:0;top:0;width:300px;height:28px}
.C .mw{position:absolute;left:14px;top:6px;display:flex;align-items:center;gap:7px;font-family:Chakra;font-weight:700;font-size:11px;letter-spacing:2.5px}
.C .v{position:absolute;right:22px;top:7px;font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:2px}
.C .q{position:absolute;right:22px;top:8px;font-family:Chakra;font-size:11px;letter-spacing:1.5px;color:#7f9aab}
.C .in{display:grid;grid-template-columns:150px 1fr;gap:4px 18px;padding:8px 18px 14px}
.C .n{grid-column:1/3;font-family:Chakra;font-weight:700;font-size:20px;letter-spacing:.5px}
.C .k{grid-column:1/3;font-size:13px;color:#9fb6c4;margin-top:-4px;letter-spacing:.5px}
.C .dm b{font-family:Chakra;font-size:40px;line-height:1}
.C .dm .x{font-family:Chakra;font-weight:700;font-size:18px}
.C .dm span{display:block;font-size:11px;color:#9fb6c4;letter-spacing:2px;margin-top:5px}
.C .dm span b{font-size:15px;line-height:1;letter-spacing:0}
.C .dm span.x{display:inline;font-size:18px;color:#e8eef2;letter-spacing:0;margin:0 0 0 2px}
.C .st{display:grid;grid-template-columns:74px 1fr 12px;align-items:center;gap:7px;margin-top:6px;font-size:11px;letter-spacing:1px;color:#9fb6c4}
.seg{position:relative;height:7px;display:flex;gap:2px}
.seg i{flex:1;background:rgba(90,200,255,.14);transform:skewX(-20deg)}
.seg i.on{background:#dcefff}
.seg u{position:absolute;top:-3px;bottom:-3px;width:3px;background:#ff9f1c}
.C .t{grid-column:1/3;font-size:13px;font-weight:700;color:#ff5e2e;margin-top:2px}.C .t span{color:#9fb6c4;font-weight:400}
.C .keys{grid-column:1/3;display:flex;gap:16px;margin-top:4px;font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:1.5px;align-items:center}
.cap{display:inline-flex;align-items:center;justify-content:center;min-width:22px;height:20px;padding:0 6px;margin-right:6px;color:#ff9f1c;
 font-family:Chakra;font-weight:700;font-size:12px;background:transparent;box-shadow:inset 0 0 0 2px #ff9f1c;
 clip-path:polygon(0 0,calc(100% - 5px) 0,100% 5px,100% 100%,5px 100%,0 calc(100% - 5px))}
.lbl{font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:3px;color:#8fb3cc}
.h{font-family:Chakra;font-weight:700;font-size:22px;letter-spacing:4px;color:#ff9f1c}
.note{font-family:Chakra;font-size:15px;color:#a9c1d0;line-height:1.35}.note b{color:#e8eef2}
/* rows */
.row{position:relative;width:470px;height:52px;margin-top:8px;display:flex;align-items:center;gap:12px;padding-left:16px;border-radius:3px;
 clip-path:polygon(0 0,calc(100% - 12px) 0,100% 12px,100% 100%,0 100%)}
.row .nm{font-family:Chakra;font-weight:700;font-size:16px}
.row .kk{font-size:12px;letter-spacing:1px;color:#9fb6c4;font-family:'Exo 2'}
`;

// The sweep: an S-curve out of the maker's colour, its trim along the foot.
function sweep(m) {
	const M = MAKER[m];
	const id = 'g' + m + Math.random().toString(36).slice(2, 7);
	const fill = m === 'wind'
		? `<linearGradient id="${id}" x1="0" x2="1"><stop offset="0" stop-color="#4f8f8a"/><stop offset=".42" stop-color="#4f8f8a"/><stop offset=".42" stop-color="#9a5534"/><stop offset=".7" stop-color="#9a5534"/><stop offset=".7" stop-color="#6f7550"/><stop offset="1" stop-color="#6f7550"/></linearGradient>`
		: `<linearGradient id="${id}" x1="0" x2="1"><stop offset="0" stop-color="${M.a}"/><stop offset="1" stop-color="${M.b}"/></linearGradient>`;
	const shape = 'M0 0 H232 C250 0 256 28 276 28 H0 Z';
	const foot = 'M0 27 H276';
	let deco = '';
	if (M.deco === 'rivets') deco = [8, 220].map(x => `<circle cx="${x}" cy="8" r="2.2" fill="${M.trim}"/><circle cx="${x}" cy="20" r="2.2" fill="${M.trim}"/>`).join('');
	if (M.deco === 'gilt') deco = `<path d="M196 14 l5 -6 l5 6 l-5 6 Z" fill="none" stroke="${M.trim}" stroke-width="1.3"/><path d="M178 14 H192 M210 14 H224" stroke="${M.trim}" stroke-width="1"/>`;
	if (M.deco === 'patch') deco = `<rect x="200" y="-2" width="30" height="10" fill="rgba(230,220,180,.75)" transform="rotate(-8 215 3)"/><circle cx="6" cy="6" r="2.6" fill="#b8b6b0"/><circle cx="224" cy="22" r="2.6" fill="#b8b6b0"/>`;
	if (M.deco === 'ticks') deco = Array.from({ length: 34 }, (_, i) => `<line x1="${4 + i * 8}" y1="${i % 5 ? 24 : 21}" x2="${4 + i * 8}" y2="28" stroke="${i % 5 ? '#dfe6ea' : M.trim}" stroke-width="1" opacity=".75"/>`).join('');
	return `<svg class="sweep" viewBox="0 0 300 28"><defs>${fill}<clipPath id="c${id}"><path d="${shape}"/></clipPath></defs>
	 <path d="${shape}" fill="url(#${id})"/><g clip-path="url(#c${id})">${deco}<path d="${foot}" stroke="${M.trim}" stroke-width="3"/></g>
	 <path d="M276 28 H300" stroke="rgba(90,200,255,.0)"/></svg>`;
}
// A hairline along the top from the sweep's end, down the right cut, and a short bracket bottom-left.
const edge = `<svg class="edge" viewBox="0 0 420 300" preserveAspectRatio="none"><path d="M280 0.7 H404 L419.3 16" fill="none" stroke="rgba(90,200,255,.85)" stroke-width="2" vector-effect="non-scaling-stroke"/></svg>
 <div style="position:absolute;left:0;bottom:0;width:60px;height:16px;pointer-events:none"><svg width="60" height="16"><path d="M60 15.3 H16 L0.7 0" fill="none" stroke="rgba(90,200,255,.7)" stroke-width="2"/></svg></div>`;
const segBar = (v, o) => `<div class="seg">${Array.from({ length: 10 }, (_, i) => `<i class="${i < Math.round(v * 10) ? 'on' : ''}"></i>`).join('')}${o == null ? '' : `<u style="left:${o * 100}%"></u>`}</div>`;

function card(g, style) {
	const M = MAKER[g.maker];
	const stats = g.stats.map(([l, v, o]) => {
		const a = o == null ? '' : v > o + 0.02 ? '<b class="u">▲</b>' : v < o - 0.02 ? '<b class="w">▼</b>' : '';
		return `<div class="st"><span>${l}</span>${segBar(v, o)}${a}</div>`;
	}).join('');
	return `<div class="C" style="${style}">${edge}
	 <div class="top">${sweep(g.maker)}<div class="mw" style="color:${M.ink}">${MARK[g.maker](M.ink)}${M.word}</div>
	  ${g.verdict ? `<span class="v u">▲ ${g.verdict}</span>` : `<span class="q">${M.quirk}</span>`}</div>
	 <div class="in"><div class="n" style="color:${RAR[g.rarity]}">${g.name}</div><div class="k">Level ${g.level} · ${g.kind}</div>
	  <div class="dm"><b>${g.dmg}</b>${g.pellets ? `<span class="x">×${g.pellets}</span>` : ''}<span>DAMAGE ${g.delta ? `<b class="u">▲ +${g.delta}</b>` : ''}</span></div>
	  <div>${stats}</div>
	  ${g.trait ? `<div class="t" style="color:${g.trait[1]}">◆ ${g.trait[0]} <span>${g.trait[2]}</span></div>` : ''}
	  ${g.keys ? `<div class="keys"><span><span class="cap">E</span>TAKE</span><span style="color:#9fb6c4"><span class="cap">HOLD E</span>EQUIP</span></div>` : ''}
	 </div></div>`;
}


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
