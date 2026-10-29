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


// Grave Light: deep green-black glass, ghostly mint lines, soul-light gold accents.
let GL = { top: 'rgba(12,28,26,.95)', foot: 'rgba(7,17,16,.93)', line: '#8cf0d2', acc: '#ffe6a6', lab: '#a8c9bf', text: '#eef9f5', fl: '#ffd38a', icon: '#8fb3a8' };
const bcss = '.Bc .lr{display:grid;grid-template-columns:20px 1fr auto 18px;align-items:center;gap:9px;padding:3px 0}' +
	'.Bc .lab{font-family:Chakra;font-weight:700;font-size:16px;color:' + GL.lab + ';letter-spacing:.5px}' +
	'.Bc .val{font-family:Chakra;font-weight:700;font-size:20px;color:' + GL.text + ';text-align:right}' +
	'.Bc .arr{font-size:12px;text-align:right}.Bc .u{color:#7dff8a}.Bc .w{color:#ff7a6b}' +
	'.Bc .bul{font-family:Chakra;font-weight:700;font-size:15px;color:' + GL.text + ';line-height:1.3;padding-left:12px;text-indent:-12px}' +
	'.Bc .fl{font-family:Chakra;font-style:italic;font-weight:700;font-size:15px;color:' + GL.fl + ';line-height:1.3}' +
	'.Bc [fill="#9fb6c4"]{fill:' + GL.icon + '}.Bc [stroke="#9fb6c4"]{stroke:' + GL.icon + ';stroke-width:2.6}.Bc .lr svg{width:18px;height:18px}' +
	'.gcap{display:inline-flex;align-items:center;justify-content:center;min-width:30px;height:28px;padding:0 8px;margin-right:9px;color:' + GL.acc + ';font-family:Chakra;font-weight:700;font-size:16px;' +
	'box-shadow:inset 0 0 0 3.5px ' + GL.acc + ',0 0 0 2px rgba(0,0,0,.6);border-radius:4px}';

// Frames: an outer shape in the line colour, the glass inside it 3 px in, so the outline follows the shape exactly.
const W = 400;
const SHAPES = {
	mix: (i) => 'polygon(0 ' + (26 - i) + 'px,' + (26 - i) + 'px 0,calc(50% - 46px) 0,calc(50% - 34px) 11px,calc(50% + 34px) 11px,calc(50% + 46px) 0,calc(100% - ' + (26 - i) + 'px) 0,100% ' + (26 - i) + 'px,100% calc(100% - 28px),50% 100%,0 calc(100% - 28px))',
	tag: (i) => 'polygon(0 ' + (14 - i) + 'px,' + (14 - i) + 'px 0,calc(50% - 40px) 0,calc(50% - 30px) 8px,calc(50% + 30px) 8px,calc(50% + 40px) 0,calc(100% - ' + (14 - i) + 'px) 0,100% ' + (14 - i) + 'px,100% calc(100% - 22px),50% 100%,0 calc(100% - 22px))',
	coffin: (i) => 'polygon(14% 0,86% 0,100% 13%,90% 100%,10% 100%,0 13%)',
	toe: (i) => 'polygon(0 ' + (24 - i) + 'px,' + (24 - i) + 'px 0,calc(100% - ' + (24 - i) + 'px) 0,100% ' + (24 - i) + 'px,100% calc(100% - 26px),50% 100%,0 calc(100% - 26px))',
	swallow: (i) => 'polygon(0 ' + (14 - i) + 'px,' + (14 - i) + 'px 0,calc(50% - 40px) 0,calc(50% - 30px) 8px,calc(50% + 30px) 8px,calc(50% + 40px) 0,calc(100% - ' + (14 - i) + 'px) 0,100% ' + (14 - i) + 'px,100% 100%,50% calc(100% - 24px),0 100%)',
	notch: (i) => 'polygon(0 ' + (14 - i) + 'px,' + (14 - i) + 'px 0,calc(100% - ' + (14 - i) + 'px) 0,100% ' + (14 - i) + 'px,100% 42%,calc(100% - 9px) 50%,100% 58%,100% calc(100% - 22px),calc(50% + 22px) calc(100% - 22px),50% 100%,calc(50% - 22px) calc(100% - 22px),0 calc(100% - 22px),0 58%,9px 50%,0 42%)',
};
function framed(shape, inner) {
	if (shape === 'round') {
		return '<div style="position:relative;width:' + W + 'px;border:3px solid ' + GL.line + ';border-radius:22px 6px 22px 6px;background:linear-gradient(180deg,' + GL.top + ',' + GL.foot + ');box-shadow:0 0 0 1.5px rgba(0,0,0,.5),0 0 18px rgba(140,240,210,.18)">' + inner + '</div>';
	}
	const clip = SHAPES[shape];
	return '<div style="position:relative;width:' + W + 'px;filter:drop-shadow(0 0 1.5px rgba(0,0,0,.7)) drop-shadow(0 0 14px rgba(140,240,210,.16))">' +
		'<div style="position:absolute;inset:0;background:' + GL.line + ';clip-path:' + clip(0) + '"></div>' +
		'<div style="position:absolute;inset:4px;background:linear-gradient(180deg,' + GL.top + ',' + GL.foot + ');clip-path:' + clip(1.3) + '"></div>' +
		(shape === 'toe' ? '<div style="position:absolute;left:50%;top:22px;width:20px;height:20px;margin-left:-10px;border-radius:50%;border:4px solid ' + GL.line + ';background:#060b0a"></div>' : '') +
		'<div style="position:relative">' + inner + '</div></div>';
}
const keys = '<div style="display:flex;justify-content:center;gap:22px;margin-top:12px;font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:1.5px;color:#f3f3f3;text-shadow:0 0 3px #000,0 0 6px #000">' +
	'<span><span class="gcap">E</span>TAKE</span><span><span class="gcap">HOLD E</span>EQUIP</span></div>';

const RARW = { common: 'COMMON', unc: 'UNCOMMON', rare: 'RARE', epic: 'EPIC', leg: 'LEGENDARY' };
const hexA = (h, a) => { const n = parseInt(h.slice(1), 16); return 'rgba(' + (n >> 16) + ',' + ((n >> 8) & 255) + ',' + (n & 255) + ',' + a + ')'; };
// The ornament between the stats and the lines: a rule, a diamond, a rule.
const orn = (c) => '<div style="display:flex;align-items:center;gap:8px;margin:10px 0 9px"><div style="flex:1;height:2px;background:linear-gradient(90deg,transparent,' + c + ')"></div>' +
	'<svg width="16" height="16" viewBox="0 0 16 16"><path d="M8 1 L15 8 L8 15 L1 8 Z" fill="none" stroke="' + c + '" stroke-width="2"/><path d="M8 5 L11 8 L8 11 L5 8 Z" fill="' + c + '"/></svg>' +
	'<div style="flex:1;height:2px;background:linear-gradient(90deg,' + c + ',transparent)"></div></div>';
const rivet = (x, y) => '<div style="position:absolute;left:' + x + ';top:' + y + ';width:9px;height:9px;border-radius:50%;background:radial-gradient(circle at 35% 35%,#d9fff2,' + GL.line + ' 45%,#2e6b5c);box-shadow:0 0 0 1.5px rgba(0,0,0,.6)"></div>';

function distCard(g, style) {
	const M = MAKER[g.maker];
	const rc = RAR[g.rarity];
	// The name: on a band in its rarity's colour (A), plain with a maker crest behind the stats (B), or with the level gem (C).
	let top = '';
	if (style === 'A') {
		top = '<div style="margin:0 -22px 4px;padding:6px 22px 7px;background:linear-gradient(90deg,transparent,' + hexA(rc, .22) + ' 20%,' + hexA(rc, .22) + ' 80%,transparent);border-top:2px solid ' + hexA(rc, .55) + ';border-bottom:2px solid ' + hexA(rc, .55) + '">' +
			'<div style="font-family:Chakra;font-weight:700;font-size:26px;letter-spacing:1px;color:' + rc + ';text-align:center;text-shadow:0 0 12px ' + hexA(rc, .45) + '">' + g.name + '</div></div>' +
			'<div style="font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:3px;color:' + GL.lab + ';text-align:center;margin-bottom:8px">LEVEL ' + g.level + ' · ' + RARW[g.rarity] + ' · ' + M.word + '</div>';
	} else if (style === 'B') {
		top = '<div style="font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:4px;color:' + rc + ';text-align:center;opacity:.9">' + RARW[g.rarity] + '</div>' +
			'<div style="font-family:Chakra;font-weight:700;font-size:26px;letter-spacing:1px;color:' + GL.text + ';text-align:center;margin:2px 0 4px">' + g.name + '</div>' +
			'<div style="height:3px;width:120px;margin:0 auto 10px;background:linear-gradient(90deg,transparent,' + rc + ',transparent)"></div>';
	} else {
		top = '<div style="font-family:Chakra;font-weight:700;font-size:26px;letter-spacing:1px;color:' + rc + ';text-align:center;margin:6px 0 2px">' + g.name + '</div>' +
			'<div style="font-family:Chakra;font-style:italic;font-weight:700;font-size:13px;letter-spacing:1px;color:' + GL.lab + ';text-align:center;margin-bottom:8px">' + M.word.charAt(0) + M.word.slice(1).toLowerCase() + ' · ' + g.kind + '</div>';
	}
	const stats = style === 'C'
		? g.stats.map((r, i) => '<div style="background:' + (i % 2 ? 'transparent' : 'rgba(140,240,210,.06)') + ';margin:0 -8px;padding:0 8px;border-radius:6px">' + lr(r) + '</div>').join('')
		: g.stats.map(lr).join('');
	const crest = style === 'B' ? '<div style="position:absolute;left:50%;top:52%;transform:translate(-50%,-50%);opacity:.08">' + MARK[g.maker]('#ffffff').replace('width="15" height="15"', 'width="230" height="230"') + '</div>' : '';
	const foot = '<div style="display:flex;align-items:center;justify-content:center;gap:14px;margin-top:11px">' +
		'<div style="display:flex;align-items:center;gap:7px;padding:4px 14px 4px 10px;border-radius:12px;background:linear-gradient(90deg,' + M.a + ',' + M.b + ');box-shadow:inset 0 0 0 2px ' + M.trim + ';font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:2.5px;color:' + M.ink + '">' + MARK[g.maker](M.ink) + M.word + '</div>' +
		(g.element || '') + '<div style="display:flex;gap:6px;align-items:center;font-family:Chakra;font-weight:700;font-size:17px;color:' + GL.text + '">' + (g.value ? coin.replace(/13/g, '16') + g.value : '') + '</div></div>';
	const inner = '<div class="Bc" style="position:relative;padding:' + (style === 'C' ? '34px' : '26px') + ' 22px 42px">' + crest + top + stats + orn(style === 'A' ? GL.line : GL.acc) +
		g.lines.map(t => '<div class="bul">• ' + t + '</div>').join('') + (g.flavor ? '<div class="fl" style="margin-top:4px">' + g.flavor + '</div>' : '') + foot + '</div>';
	// A: an engraved inner line 7 px in, following the shape, and rivets on the shoulders.
	const engraved = style === 'A' ? '<div style="position:absolute;inset:9px;pointer-events:none"><div style="position:absolute;inset:0;background:' + hexA('#8cf0d2', .45) + ';clip-path:' + SHAPES.mix(-4) + '"></div>' +
		'<div style="position:absolute;inset:1.5px;background:linear-gradient(180deg,' + GL.top + ',' + GL.foot + ');clip-path:' + SHAPES.mix(-3) + '"></div></div>' : '';
	const rivets = style === 'A' ? rivet('8px', '30px') + rivet('calc(100% - 17px)', '30px') : '';
	// C: the level as a gem set into the top notch.
	const gem = style === 'C' ? '<div style="position:absolute;left:50%;top:-14px;width:44px;height:44px;margin-left:-22px;z-index:2"><svg width="44" height="44" viewBox="0 0 44 44"><path d="M22 2 L42 22 L22 42 L2 22 Z" fill="#0b1c19" stroke="' + GL.acc + '" stroke-width="3.5"/><path d="M22 8 L36 22 L22 36 L8 22 Z" fill="none" stroke="' + hexA('#ffe6a6', .35) + '" stroke-width="1.5"/></svg>' +
		'<div style="position:absolute;inset:0;display:flex;align-items:center;justify-content:center;font-family:Chakra;font-weight:700;font-size:17px;color:' + GL.acc + '">' + g.level + '</div></div>' : '';
	const lvl = style === 'B' ? '<div style="display:inline-flex;margin:0 0 7px 16px;padding:3px 14px;font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:2.5px;color:' + GL.acc + ';border:2.5px solid ' + GL.acc + ';border-radius:12px;background:rgba(7,17,16,.85)">LEVEL ' + g.level + '</div>' : '<div style="height:8px"></div>';
	const glow = style === 'B' ? 'drop-shadow(0 -6px 16px ' + hexA(rc, .35) + ')' : 'drop-shadow(0 0 14px rgba(140,240,210,.16))';
	return '<div>' + lvl + '<div style="position:relative;width:' + W + 'px;filter:drop-shadow(0 0 1.5px rgba(0,0,0,.7)) ' + glow + '">' + gem +
		'<div style="position:absolute;inset:0;background:' + GL.line + ';clip-path:' + SHAPES.mix(0) + '"></div>' +
		'<div style="position:absolute;inset:4px;background:linear-gradient(180deg,' + GL.top + ',' + GL.foot + ');clip-path:' + SHAPES.mix(1.3) + '"></div>' +
		engraved + rivets + '<div style="position:relative">' + inner + '</div></div></div>';
}

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
