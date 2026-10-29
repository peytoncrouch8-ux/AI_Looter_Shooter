// Round 14 (the user, 2026-10-28: "Let's get a little more distinguished. Reads bland right now"): the notch-and-point tag
// with finer detail, three ways. Builds dist.js from mix.js (shapes, colours, styles, glyphs) and renders dist.png.
const fs = require('fs');
const path = require('path');
const here = __dirname;
let s = fs.readFileSync(path.join(here, 'mix.js'), 'utf8');
let head = s.slice(0, s.indexOf('function boldCard(g, shape)'));
const k0 = s.indexOf('const keys = '); const k1 = s.indexOf('const D = ', k0); head += s.slice(k0, k1);

const tail = String.raw`
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
		(g.element || '') + '<div style="display:flex;gap:6px;align-items:center;font-family:Chakra;font-weight:700;font-size:17px;color:' + GL.text + '">' + coin.replace(/13/g, '16') + g.value + '</div></div>';
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
const SH = { maker: 'maud', rarity: 'rare', name: 'SLUGGER PUMP SHOTGUN', level: 6, kind: 'Pump shotgun',
	stats: [['dmg', 'Damage', '21 × 8', 'u'], ['acc', 'Accuracy', '62%', 'w'], ['hnd', 'Handling', '70%', 'u'], ['rel', 'Reload', '1.7 s', 'u'], ['rof', 'Fire rate', '66 rpm', 'u'], ['mag', 'Magazine', '10', 'u']],
	lines: ['Salts and slows them: wards crumble', 'Blessed Iron: +12% damage, slower fire'], flavor: 'Sister Agnes blessed this one twice.', element: salt, value: '412' };
const SN = { maker: 'mer', rarity: 'leg', name: 'SURVEYOR SNIPER RIFLE', level: 6, kind: 'Sniper rifle',
	stats: [['dmg', 'Damage', '375', 'u'], ['acc', 'Accuracy', '96%', 'u'], ['hnd', 'Handling', '44%', 'w'], ['rel', 'Reload', '3.1 s', 'w'], ['rof', 'Fire rate', '45 rpm', 'w'], ['mag', 'Magazine', '5', 'w']],
	lines: ['Rounds punch through an enemy into the next', 'True Glass: +10% accuracy and range'], flavor: 'Every mile of track, measured twice.', element: '', value: '3,900' };
const RI = { maker: 'round', rarity: 'epic', name: 'BRUTE ASSAULT RIFLE', level: 7, kind: 'Assault rifle',
	stats: [['dmg', 'Damage', '34', 'u'], ['acc', 'Accuracy', '81%', 'u'], ['hnd', 'Handling', '58%', 'w'], ['rel', 'Reload', '2.2 s', 'w'], ['rof', 'Fire rate', '540 rpm', 'u'], ['mag', 'Magazine', '30', 'u']],
	lines: ['Sets them burning', 'Rolling Stock: +15% damage, more kick'], flavor: 'Thrown off a flatcar twice. Still fires.', element: ember, value: '1,280' };
const col = (x, t, d, g, st, bg) => '<div style="position:absolute;left:' + x + 'px;top:126px;width:600px"><div class="h" style="font-size:20px;color:#8cf0d2">' + t + '</div>' +
	'<div class="note" style="margin-top:6px;height:40px;font-size:14px">' + d + '</div>' +
	'<div style="position:relative;margin-top:8px;height:800px;border-radius:6px;overflow:hidden;background:url(\'' + shot + '\') ' + bg + '/2403px auto">' +
	'<div style="position:absolute;left:100px;top:34px">' + distCard(g, st) + keys + '</div></div></div>';
const html = '<!doctype html><html><head><meta charset="utf-8"><style>' + css + ccss + bcss + '</style></head><body>' +
	'<div class="sheet" style="background:radial-gradient(ellipse at 50% 20%,#11201d,#060909 70%)">' +
	'<div class="h" style="position:absolute;left:40px;top:30px;font-size:30px;color:#8cf0d2">MORE DISTINGUISHED</div>' +
	'<div class="note" style="position:absolute;left:40px;top:72px">The same notch-and-point tag, dressed three ways: finer lines, ornaments and the gun\'s rank worn with some pride.</div>' +
	col(40, 'A · ENGRAVED', 'An engraved inner line, rivets on the shoulders, the name on a band of its rarity colour.', SH, 'A', '-500px -260px') +
	col(670, 'B · CREST', 'The maker\'s crest faint behind the stats, the rarity word over the name, a rarity glow above.', SN, 'B', '-700px -260px') +
	col(1300, 'C · GEM', 'The level as a gem set in the notch, the maker and kind in italics, banded stat rows.', RI, 'C', '-900px -260px') +
	'</div></body></html>';
fs.writeFileSync(path.join(here, 'dist.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	await pg.goto('file:///' + path.join(here, 'dist.html').split(path.sep).join('/'));
	await pg.waitForTimeout(1200);
	await pg.screenshot({ path: path.join(here, 'dist.png') });
	await browser.close();
	console.log('done');
})();
`;
fs.writeFileSync(path.join(here, 'dist.js'), head + tail);
console.log('dist.js written');
