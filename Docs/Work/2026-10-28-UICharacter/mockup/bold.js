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
const GL = { top: 'rgba(12,28,26,.95)', foot: 'rgba(7,17,16,.93)', line: '#8cf0d2', acc: '#ffe6a6', lab: '#a8c9bf', text: '#eef9f5', fl: '#ffd38a', icon: '#8fb3a8' };
const bcss = '.Bc .lr{display:grid;grid-template-columns:18px 1fr auto 16px;align-items:center;gap:8px;padding:2px 0}' +
	'.Bc .lab{font-family:Chakra;font-weight:700;font-size:14px;color:' + GL.lab + ';letter-spacing:.5px}' +
	'.Bc .val{font-family:Chakra;font-weight:700;font-size:17px;color:' + GL.text + ';text-align:right}' +
	'.Bc .arr{font-size:12px;text-align:right}.Bc .u{color:#7dff8a}.Bc .w{color:#ff7a6b}' +
	'.Bc .bul{font-family:Chakra;font-weight:700;font-size:13.5px;color:' + GL.text + ';line-height:1.3;padding-left:12px;text-indent:-12px}' +
	'.Bc .fl{font-family:Chakra;font-style:italic;font-weight:700;font-size:13.5px;color:' + GL.fl + ';line-height:1.3}' +
	'.Bc [fill="#9fb6c4"]{fill:' + GL.icon + '}.Bc [stroke="#9fb6c4"]{stroke:' + GL.icon + ';stroke-width:2.2}' +
	'.gcap{display:inline-flex;align-items:center;justify-content:center;min-width:26px;height:24px;padding:0 7px;margin-right:8px;color:' + GL.acc + ';font-family:Chakra;font-weight:700;font-size:14px;' +
	'box-shadow:inset 0 0 0 3px ' + GL.acc + ',0 0 0 1.5px rgba(0,0,0,.55);border-radius:4px}';

// Frames: an outer shape in the line colour, the glass inside it 3 px in, so the outline follows the shape exactly.
const W = 360;
function framed(shape, inner) {
	if (shape === 'round') {
		return '<div style="position:relative;width:' + W + 'px;border:3px solid ' + GL.line + ';border-radius:22px 6px 22px 6px;background:linear-gradient(180deg,' + GL.top + ',' + GL.foot + ');box-shadow:0 0 0 1.5px rgba(0,0,0,.5),0 0 18px rgba(140,240,210,.18)">' + inner + '</div>';
	}
	const clip = shape === 'cut'
		? (i) => 'polygon(' + (18 - i) + 'px 0,calc(100% - ' + (18 - i) + 'px) 0,100% ' + (18 - i) + 'px,100% calc(100% - ' + (18 - i) + 'px),calc(100% - ' + (18 - i) + 'px) 100%,' + (18 - i) + 'px 100%,0 calc(100% - ' + (18 - i) + 'px),0 ' + (18 - i) + 'px)'
		: (i) => 'polygon(0 ' + (14 - i) + 'px,' + (14 - i) + 'px 0,calc(50% - 40px) 0,calc(50% - 30px) 8px,calc(50% + 30px) 8px,calc(50% + 40px) 0,calc(100% - ' + (14 - i) + 'px) 0,100% ' + (14 - i) + 'px,100% calc(100% - 22px),50% 100%,0 calc(100% - 22px))';
	return '<div style="position:relative;width:' + W + 'px;filter:drop-shadow(0 0 1.5px rgba(0,0,0,.7)) drop-shadow(0 0 14px rgba(140,240,210,.16))">' +
		'<div style="position:absolute;inset:0;background:' + GL.line + ';clip-path:' + clip(0) + '"></div>' +
		'<div style="position:absolute;inset:3px;background:linear-gradient(180deg,' + GL.top + ',' + GL.foot + ');clip-path:' + clip(1.3) + '"></div>' +
		'<div style="position:relative">' + inner + '</div></div>';
}
function boldCard(g, shape) {
	const M = MAKER[g.maker];
	const lvl = '<div style="display:inline-flex;margin:0 0 7px 16px;padding:3px 14px;font-family:Chakra;font-weight:700;font-size:12px;letter-spacing:2.5px;color:' + GL.acc + ';' +
		'border:2.5px solid ' + GL.acc + ';border-radius:12px;background:rgba(7,17,16,.85)">LEVEL ' + g.level + '</div>';
	const inner = '<div class="Bc" style="padding:' + (shape === 'tag' ? '20px' : '14px') + ' 20px ' + (shape === 'tag' ? '30px' : '14px') + '">' +
		'<div style="font-family:Chakra;font-weight:700;font-size:23px;letter-spacing:.5px;color:' + RAR[g.rarity] + ';text-align:center;text-shadow:0 0 10px rgba(0,0,0,.6);margin-bottom:8px">' + g.name + '</div>' +
		g.stats.map(lr).join('') +
		'<div style="height:2px;margin:9px 0 8px;background:linear-gradient(90deg,transparent,' + GL.line + ',transparent);opacity:.7"></div>' +
		g.lines.map(t => '<div class="bul">• ' + t + '</div>').join('') +
		(g.flavor ? '<div class="fl" style="margin-top:4px">' + g.flavor + '</div>' : '') +
		'<div style="display:flex;align-items:center;justify-content:' + (shape === 'tag' ? 'center' : 'space-between') + ';gap:14px;margin-top:11px">' +
		'<div style="display:flex;align-items:center;gap:7px;padding:4px 14px 4px 10px;border-radius:12px;background:linear-gradient(90deg,' + M.a + ',' + M.b + ');box-shadow:inset 0 0 0 2px ' + M.trim + ';font-family:Chakra;font-weight:700;font-size:13px;letter-spacing:2.5px;color:' + M.ink + '">' + MARK[g.maker](M.ink) + M.word + '</div>' +
		(g.element || '') + '</div></div>';
	return '<div>' + lvl + framed(shape, inner) +
		'<div style="display:flex;gap:6px;align-items:center;margin:8px 0 0 16px;font-family:Chakra;font-weight:700;font-size:15px;color:' + GL.text + ';text-shadow:0 0 3px #000">' + coin + g.value + '</div></div>';
}
const A = { maker: 'maud', rarity: 'rare', name: 'SLUGGER PUMP SHOTGUN', level: 6,
	stats: [['dmg', 'Damage', '21 × 8', 'u'], ['acc', 'Accuracy', '62%', 'w'], ['hnd', 'Handling', '70%', 'u'], ['rel', 'Reload', '1.7 s', 'u'], ['rof', 'Fire rate', '66 rpm', 'u'], ['mag', 'Magazine', '10', 'u']],
	lines: ['Salts and slows them: wards crumble', 'Blessed Iron: +12% damage, slower fire'],
	flavor: 'Sister Agnes blessed this one twice.', element: salt, value: '412' };
const B = { maker: 'round', rarity: 'epic', name: 'BRUTE ASSAULT RIFLE', level: 7,
	stats: [['dmg', 'Damage', '34', 'u'], ['acc', 'Accuracy', '81%', 'u'], ['hnd', 'Handling', '58%', 'w'], ['rel', 'Reload', '2.2 s', 'w'], ['rof', 'Fire rate', '540 rpm', 'u'], ['mag', 'Magazine', '30', 'u']],
	lines: ['Sets them burning', 'Rolling Stock: +15% damage, more kick'], flavor: '', element: ember, value: '1,280' };
const C = { maker: 'mer', rarity: 'leg', name: 'SURVEYOR SNIPER RIFLE', level: 6,
	stats: [['dmg', 'Damage', '375', 'u'], ['acc', 'Accuracy', '96%', 'u'], ['hnd', 'Handling', '44%', 'w'], ['rel', 'Reload', '3.1 s', 'w'], ['rof', 'Fire rate', '45 rpm', 'w'], ['mag', 'Magazine', '5', 'w']],
	lines: ['Rounds punch through an enemy into the next', 'True Glass: +10% accuracy and range'], flavor: 'Every mile of track, measured twice.', element: '', value: '3,900' };

const keys = '<div style="margin:0 0 0 14px;display:flex;flex-direction:column;gap:10px;font-family:Chakra;font-weight:700;font-size:15px;letter-spacing:1.5px;color:#f3f3f3;text-shadow:0 0 3px #000,0 0 6px #000">' +
	'<span><span class="gcap">E</span>TAKE</span><span><span class="gcap">HOLD E</span>EQUIP</span></div>';
const col = (x, t, d, g, shape, bg) => '<div style="position:absolute;left:' + x + 'px;top:126px;width:600px"><div class="h" style="font-size:20px;color:#8cf0d2">' + t + '</div>' +
	'<div class="note" style="margin-top:6px;height:40px;font-size:14px">' + d + '</div>' +
	'<div style="position:relative;margin-top:8px;height:720px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') ' + bg + '/2403px auto">' +
	'<div style="position:absolute;left:44px;top:44px;display:flex;align-items:flex-end">' + boldCard(g, shape) + '<div style="margin-bottom:60px">' + keys + '</div></div></div></div>';
const html = '<!doctype html><html><head><meta charset="utf-8"><style>' + css + ccss + bcss + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#11201d,#060909 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:30px;font-size:30px;color:#8cf0d2">GRAVE LIGHT, BOLDER AND LESS BOXY</div>' +
	'<div class="note" style="position:absolute;left:40px;top:72px">3 px outlines that follow each shape, a soft glow, heavier and bigger text, round-cornered tags and keys. Three frame shapes.</div>' +
	col(40, 'A · CUT CORNERS', 'All four corners cut: sharp but never a box.', A, 'cut', '-500px -300px') +
	col(670, 'B · SOFT CORNERS', 'Two round corners and two tight ones: a little curve.', B, 'round', '-700px -300px') +
	col(1300, 'C · TAG', 'A tag hanging over the gun: a notch on top, a point below it.', C, 'tag', '-900px -300px') +
	'</div></body></html>';
fs.writeFileSync(path.join(here, 'bold.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	await pg.goto('file:///' + path.join(here, 'bold.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1200);
	await pg.screenshot({ path: path.join(here, 'bold.png') });
	await browser.close();
	console.log('done');
})();
