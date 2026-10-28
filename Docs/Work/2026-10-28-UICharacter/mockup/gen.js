// UI character mockups (the user's ask, 2026-10-28): the gun card over the gun, the in-hand card on the right, the four
// makers' cards, the motion, and the inventory with the backpack back on the right. node gen.js -> sheet1-3.png
const fs = require('fs');
const path = require('path');
const here = __dirname;
const shot = 'file:///C:/Dev/AI_Looter_Shooter/Saved/Screenshots/Tools/maud_shotgun.png';

const MARK = {
	round: `<svg class="mk" viewBox="0 0 100 100"><circle cx="50" cy="50" r="40" fill="none" stroke="currentColor" stroke-width="9"/>${[0, 45, 90, 135].map(a => `<line x1="50" y1="12" x2="50" y2="88" stroke="currentColor" stroke-width="6" transform="rotate(${a} 50 50)"/>`).join('')}<path d="M50 50 L22 22 A40 40 0 0 0 22 78 Z" fill="currentColor"/><circle cx="50" cy="50" r="12" fill="currentColor"/></svg>`,
	maud: `<svg class="mk" viewBox="0 0 100 100"><ellipse cx="50" cy="20" rx="24" ry="8" fill="none" stroke="currentColor" stroke-width="5"/><path d="M14 42 H80 Q92 42 94 34 L96 34 Q96 52 78 54 L70 54 L66 64 L74 72 L74 84 L26 84 L26 72 L34 64 L30 54 Q16 54 14 42 Z" fill="currentColor"/></svg>`,
	wind: `<svg class="mk" viewBox="0 0 100 100"><path d="M50 8 V58" stroke="currentColor" stroke-width="8" stroke-linecap="round"/><circle cx="50" cy="10" r="6" fill="none" stroke="currentColor" stroke-width="4"/><path d="M50 58 Q50 84 26 80 Q14 76 16 64" fill="none" stroke="currentColor" stroke-width="8" stroke-linecap="round"/><path d="M50 58 Q50 84 74 80 Q86 76 84 64" fill="none" stroke="currentColor" stroke-width="8" stroke-linecap="round"/><path d="M10 36 Q30 26 46 34 M54 34 Q74 42 92 28" fill="none" stroke="currentColor" stroke-width="5" stroke-linecap="round"/></svg>`,
	mer: `<svg class="mk" viewBox="0 0 100 100"><circle cx="50" cy="54" r="34" fill="none" stroke="currentColor" stroke-width="7"/><line x1="50" y1="26" x2="50" y2="44" stroke="currentColor" stroke-width="5"/><line x1="50" y1="64" x2="50" y2="82" stroke="currentColor" stroke-width="5"/><line x1="22" y1="54" x2="40" y2="54" stroke="currentColor" stroke-width="5"/><line x1="60" y1="54" x2="78" y2="54" stroke="currentColor" stroke-width="5"/><circle cx="50" cy="54" r="3.5" fill="currentColor"/><path d="M50 2 L56 16 L50 13 L44 16 Z" fill="currentColor"/></svg>`,
};
const MAKER = {
	round: { word: 'ROUNDHOUSE', quirk: 'ROLLING STOCK', cls: 'm-round' },
	maud: { word: 'SAINT MAUD', quirk: 'BLESSED IRON', cls: 'm-maud' },
	wind: { word: 'WINDFALL', quirk: 'LOOSE FIT', cls: 'm-wind' },
	mer: { word: 'MERIDIAN', quirk: 'TRUE GLASS', cls: 'm-mer' },
};
const RAR = { common: '#e7e7e7', unc: '#7ce77c', rare: '#7caaff', epic: '#cc7cf4', leg: '#ffcb00' };

function bandDeco(m) {
	if (m === 'round') return `<span class="riv" style="left:5px;top:5px"></span><span class="riv" style="left:5px;bottom:5px"></span><span class="riv" style="right:5px;top:5px"></span><span class="riv" style="right:5px;bottom:5px"></span>`;
	if (m === 'wind') return `<span class="tape" style="right:70px;top:-3px"></span><span class="bolt" style="left:4px;top:4px"></span><span class="bolt" style="right:5px;bottom:5px"></span>`;
	if (m === 'maud') return `<svg style="position:absolute;right:120px;top:3px" width="40" height="28" viewBox="0 0 40 28"><path d="M20 4 L24 14 L20 24 L16 14 Z" fill="none" stroke="#9a6a1c" stroke-width="1.4"/><path d="M2 14 H14 M26 14 H38" stroke="#9a6a1c" stroke-width="1"/></svg>`;
	return '';
}

// One gun card.
function card(g, style = '') {
	const M = MAKER[g.maker];
	const stats = (g.stats || []).map(([l, v, o]) => {
		const a = o == null ? '' : v > o + 0.02 ? '<b class="up">▲</b>' : v < o - 0.02 ? '<b class="dn">▼</b>' : '';
		return `<div class="stat"><span class="lab">${l}</span><div class="bar"><i style="width:${v * 100}%"></i>${o == null ? '' : `<u style="left:${o * 100}%"></u>`}</div><span class="a">${a}</span></div>`;
	}).join('');
	const delta = g.dmgDelta ? `<span class="d ${g.dmgDelta > 0 ? 'up' : 'dn'}">${g.dmgDelta > 0 ? '▲ +' : '▼ '}${g.dmgDelta}</span>` : '';
	const verdictStyle = g.verdict === 'UPGRADE' ? 'color:#6dff7a' : 'color:#8fb3cc;box-shadow:inset 0 0 0 1px rgba(143,179,204,.5)';
	return `<div class="card ${g.inHand ? 'inhand' : ''}" style="${style}">
	 <div class="band ${M.cls}">${bandDeco(g.maker)}${MARK[g.maker]}<span>${M.word}</span><span class="q">${M.quirk}</span></div>${g.verdict ? "" : `<span class="quirk" style="right:16px">${M.quirk}</span>`}
	 <div class="rstrip" style="background:${RAR[g.rarity]}"></div>
	 ${g.verdict ? `<div class="verdict ${g.verdict === 'UPGRADE' ? 'up' : ''}" style="${verdictStyle}">${g.verdict}</div>` : ''}
	 <div class="body">
	  <div class="name" style="color:${RAR[g.rarity]};">${g.name}</div>
	  <div class="kind">LV ${g.level} · ${g.kind}</div>
	  <div class="dmg"><span class="n">${g.dmg}</span>${g.pellets ? `<span class="x">×${g.pellets}</span>` : ''}<span class="l">DAMAGE</span>${delta}</div>
	  ${stats}
	  ${g.trait ? `<div class="trait" style="color:${g.trait[1]}">◆ ${g.trait[0]} <span>${g.trait[2]}</span></div>` : ''}
	  ${g.keys ? `<div class="keys"><span><span class="cap">E</span>TAKE</span><span style="color:#8fb3cc">HOLD <span class="cap">E</span>EQUIP</span></div>` : ''}
	 </div>
	 <div class="mono" style="color:${g.maker === 'maud' ? '#9a6a1c' : '#dcefff'}">${MARK[g.maker]}</div>
	</div>`;
}

const LOOT = {
	maker: 'round', rarity: 'rare', name: 'ROUNDHOUSE BRUTE', level: 6, kind: 'PUMP SHOTGUN', dmg: 14, pellets: 8, dmgDelta: 22,
	stats: [['FIRE RATE', 0.42, 0.55], ['MAGAZINE', 0.62, 0.4], ['RELOAD', 0.7, 0.5]],
	trait: ['EMBER', '#ff5e2e', 'sets them alight'], verdict: 'UPGRADE', keys: true,
};
const HAND = {
	maker: 'mer', rarity: 'unc', name: 'MERIDIAN LONGHORN', level: 5, kind: 'RIFLE', dmg: 90, dmgDelta: 0,
	stats: [['FIRE RATE', 0.55], ['MAGAZINE', 0.4], ['RELOAD', 0.5]], inHand: true,
};

function page(body, extra = '') {
	return `<!doctype html><html><head><meta charset="utf-8"><link rel="stylesheet" href="card.css"><style>${extra}</style></head><body>${body}</body></html>`;
}

// Sheet 1: in the world.
const s1 = page(`<div class="sheet">
 <img src="${shot}" style="position:absolute;left:-226px;top:-174px;width:2403px;filter:brightness(.92)">
 <div style="position:absolute;left:688px;top:770px;width:120px;height:26px;border-radius:50%;background:radial-gradient(rgba(124,170,255,.55),transparent 70%)"></div>
 <div style="position:absolute;left:746px;top:620px;width:4px;height:160px;background:linear-gradient(rgba(255,255,255,0),rgba(255,255,255,.8))"></div>
 <svg style="position:absolute;left:700px;top:752px" width="100" height="40" viewBox="0 0 100 40"><path d="M2 22 L70 14 L96 16 L96 22 L72 24 L60 30 L40 26 L8 30 Z" fill="#2a2b2e" stroke="#8e3a2a" stroke-width="2"/></svg>
 <div style="position:absolute;left:747px;top:572px;width:1.5px;height:56px;background:rgba(90,200,255,.7)"></div>
 <div style="position:absolute;left:741px;top:622px;width:13px;height:13px;transform:rotate(45deg);box-shadow:inset 0 0 0 1.5px #7caaff"></div>
 ${card(LOOT, 'left:583px;top:262px')}
 <div style="position:absolute;right:46px;top:596px;font-weight:700;font-size:13px;letter-spacing:3px;color:#8fb3cc">IN HAND</div>
 ${card(HAND, 'right:40px;top:620px;width:300px')}
 <div style="position:absolute;left:40px;top:24px;padding:14px 18px;background:rgba(5,8,11,.85);max-width:540px">
  <div style="font-weight:700;font-size:26px;letter-spacing:4px;color:#ff9f1c">1 · LOOKING AT A GUN</div>
  <div class="note" style="margin-top:6px">The card rises <b>over the gun on the ground</b>, tied to it by a thin line. The gun <b>in your hand</b> sits on the right in the same card, so you compare left to right.<br><br>Only what decides a pick: damage, three bars (the orange tick is your gun), one special line, and an <b>UPGRADE</b> tag. Every other stat moves to the inventory's Inspect.</div>
 </div>
</div>`);

// Sheet 2: the four makers and the motion.
const variants = [
	{ ...LOOT, maker: 'round', verdict: null, keys: false, dmgDelta: 0, trait: null },
	{ ...LOOT, maker: 'maud', rarity: 'epic', name: 'SAINT MAUD VESPER', kind: 'REVOLVER', dmg: 112, pellets: 0, verdict: null, keys: false, dmgDelta: 0, trait: ['SOUL-LIGHT', '#ffe6a6', 'reveals the Unpaid'] },
	{ ...LOOT, maker: 'wind', rarity: 'unc', name: 'WINDFALL RATTLER', kind: 'SMG', dmg: 11, pellets: 0, verdict: null, keys: false, dmgDelta: 0, trait: null },
	{ ...LOOT, maker: 'mer', rarity: 'leg', name: 'MERIDIAN SURVEYOR', kind: 'SNIPER RIFLE', dmg: 240, pellets: 0, verdict: null, keys: false, dmgDelta: 0, trait: ['GRAVE-SALT', '#ccf0f6', 'slows and shatters'] },
];
const notes = [
	['ROUNDHOUSE', 'Rail-car red, brass rivets, a heavy brass rule. Opens with a <b>clank</b>.'],
	['SAINT MAUD', 'Ivory enamel, antique-gold filigree. Opens with a <b>bell ting</b>.'],
	['WINDFALL', 'Patchwork plates, tape and bolts. Opens with a <b>rattle</b>.'],
	['MERIDIAN', 'Navy steel, survey-ruler ticks, silver. Opens with a <b>click</b>.'],
];
const frames = [
	['0.00 s', 'The label, as now', 'name'],
	['0.10 s', 'Unfolds up from the label, a slight overshoot', 'unfold'],
	['0.25 s', 'Bars fill one by one, damage counts up', 'fill'],
	['0.45 s', 'Settled. Epic and Legendary: a shine sweeps the band', 'done'],
];
function miniFrame(kind) {
	const base = 'position:absolute;left:30px;width:200px;';
	if (kind === 'name') return `<div style="${base}top:120px;text-align:center;font-weight:700;font-size:15px;color:#7caaff;letter-spacing:1px">ROUNDHOUSE BRUTE</div>`;
	const h = kind === 'unfold' ? 70 : 150;
	const bars = [0.42, 0.62, 0.7].map((v, i) => `<div style="height:5px;margin-top:7px;background:rgba(90,200,255,.13);transform:skewX(-18deg)"><div style="height:5px;width:${(kind === 'fill' ? [1, 0.6, 0.15][i] : 1) * v * 100}%;background:#dcefff"></div></div>`).join('');
	const body = kind === 'unfold' ? '' : `<div style="padding:8px 10px"><div style="font-weight:700;font-size:13px;color:#7caaff">ROUNDHOUSE BRUTE</div>
	  <div style="font-weight:700;font-size:24px;margin-top:4px">${kind === 'fill' ? '9' : '14'}<span style="font-size:12px">×8</span></div>${bars}</div>`;
	const shine = kind === 'done' ? '<span style="position:absolute;left:40%;top:0;bottom:0;width:30px;background:linear-gradient(90deg,transparent,rgba(255,255,255,.55),transparent);transform:skewX(-20deg)"></span>' : '';
	return `<div style="${base}top:${150 - h}px;height:${h}px;overflow:hidden;background:linear-gradient(100deg,rgba(5,18,29,.92),rgba(5,18,29,.75) 65%,rgba(5,18,29,0));clip-path:polygon(10px 0,100% 0,100% 100%,0 100%,0 10px)">
	  <div class="band m-round" style="height:18px;margin:6px 0 0 8px;padding:0 14px 0 6px;font-size:9px;gap:5px">${MARK.round.replace('class="mk"', 'style="width:12px;height:12px"')}ROUNDHOUSE${shine}</div>${body}</div>`;
}
const s2 = page(`<div class="sheet" style="background:radial-gradient(ellipse at 50% 30%,#0c2333,#05080b 70%)">
 <div class="title">2 · EACH MAKER HAS ITS OWN CARD</div>
 <div class="sub">Same layout for every gun (easy to read); the maker's band, colours, trim and opening sound give it character. Rarity stays the name's colour and the stripe on the left.</div>
 ${variants.map((v, i) => card(v, `left:${60 + i * 460}px;top:130px;width:340px`)).join('')}
 ${notes.map(([w, t], i) => `<div class="note" style="position:absolute;left:${60 + i * 460}px;top:470px;width:340px"><b>${w}</b><br>${t}</div>`).join('')}
 <div class="title" style="top:580px">MOTION</div>
 ${frames.map(([t, d, k], i) => `<div style="position:absolute;left:${60 + i * 460}px;top:630px;width:260px;height:180px;background:radial-gradient(ellipse at 50% 60%,rgba(40,70,90,.35),transparent 70%)">${miniFrame(k)}</div>
  <div class="note" style="position:absolute;left:${60 + i * 460}px;top:822px;width:300px"><b>${t}</b> ${d}</div>`).join('')}
 <div class="note" style="position:absolute;left:60px;top:910px;width:1780px;font-size:17px"><b>Elsewhere, same idea:</b> menus slide in row by row with a tick each, buttons kick on hover, numbers count up (XP, silver, damage), level-up and mission banners stamp in with a thud, every click has a sound. No new information anywhere, just motion and sound.</div>
</div>`);

// Sheet 3: the inventory, backpack back on the right.
const slots = [
	{ n: 1, name: 'MERIDIAN LONGHORN', r: 'unc', k: 'RIFLE · LV 5', hand: true },
	{ n: 2, name: 'ROUNDHOUSE BRUTE', r: 'rare', k: 'SHOTGUN · LV 6' },
	{ n: 3, name: 'SAINT MAUD VESPER', r: 'epic', k: 'REVOLVER · LV 6' },
];
const pack = [
	['WINDFALL RATTLER', 'unc', 'SMG · LV 4', 'up'], ['ROUNDHOUSE DROVER', 'common', 'REVOLVER · LV 3', 'dn'], ['MERIDIAN SURVEYOR', 'leg', 'SNIPER · LV 6', 'up'],
	['WINDFALL SCRAPPER', 'common', 'SHOTGUN · LV 2', 'dn'], ['SAINT MAUD MATINS', 'rare', 'RIFLE · LV 5', 'up'],
];
const row = (x, y, w, h, inner, sel, faint) => `<div style="position:absolute;left:${x}px;top:${y}px;width:${w}px;height:${h}px;background:${sel ? "linear-gradient(90deg,rgba(255,159,28,.28),rgba(255,159,28,0) 80%)" : faint ? "none" : "linear-gradient(90deg,rgba(5,18,29,.75),rgba(5,18,29,0) 92%)"}">${sel ? "<div style=\"position:absolute;left:0;top:8px;bottom:8px;width:3px;background:#ff9f1c\"></div>" : ""}<div style="position:absolute;left:0;right:15%;bottom:0;height:1px;background:linear-gradient(90deg,rgba(90,200,255,${faint ? .15 : .3}),transparent)"></div>${inner}</div>`;
const gunSvg = (w, c) => `<svg width="${w}" viewBox="0 0 300 90"><path d="M6 46 L180 36 L292 38 L292 52 L190 56 L160 70 L120 64 L96 80 L60 84 L70 58 L10 60 Z" fill="#2c2d30" stroke="${c}" stroke-width="3"/><rect x="186" y="28" width="60" height="8" fill="#8e3a2a"/><circle cx="140" cy="50" r="7" fill="#d9a441"/></svg>`;
const slotRows = slots.map((s, i) => row(70, 150 + i * 92, 470, 84, `<div style="position:absolute;left:14px;top:28px;width:26px;height:26px;text-align:center;font-weight:700;line-height:26px;transform:skewX(-12deg);${s.hand ? 'background:#ff9f1c;color:#23140a' : 'color:#8fb3cc'}">${s.n}</div>
   <div style="position:absolute;left:56px;top:14px">${gunSvg(110, RAR[s.r])}</div>
   <div style="position:absolute;left:186px;top:18px;font-weight:700;font-size:18px;color:${RAR[s.r]}">${s.name}</div>
   <div style="position:absolute;left:186px;top:46px;font-size:13px;color:#8fb3cc;letter-spacing:1.5px">${s.k}${s.hand ? '  ·  IN HAND' : ''}</div>`, i === 1)).join('');
const packRows = pack.map(([n, r, k, a], i) => row(1380, 150 + i * 56, 470, 50, `<div style="position:absolute;left:0;top:10px;bottom:10px;width:3px;background:${RAR[r]};transform:skewX(-12deg)"></div>
   <div style="position:absolute;left:14px;top:8px">${gunSvg(80, RAR[r])}</div>
   <div style="position:absolute;left:110px;top:6px;font-weight:700;font-size:16px;color:${RAR[r]}">${n}</div>
   <div style="position:absolute;left:110px;top:28px;font-size:12px;color:#8fb3cc;letter-spacing:1.5px">${k}</div>
   <div style="position:absolute;right:16px;top:13px;font-weight:700;font-size:18px" class="${a}">${a === 'up' ? '▲' : '▼'}</div>`)).join('');
const emptyRows = [5, 6, 7, 8, 9, 10, 11].map(i => row(1380, 150 + i * 56, 470, 50, `<div style="position:absolute;left:18px;top:15px;font-size:13px;color:rgba(143,179,204,.35);letter-spacing:2px">—  EMPTY</div>`, false, true)).join('');
const s3 = page(`<div class="sheet">
 <img src="${shot}" style="position:absolute;left:-226px;top:-174px;width:2403px;filter:blur(6px) brightness(.45)">
 <div style="position:absolute;inset:0;background:rgba(2,8,14,.55)"></div>
 <div style="position:absolute;left:0;right:0;top:40px;text-align:center;font-weight:700;font-size:22px;letter-spacing:5px;color:#8fb3cc">
  <span style="color:#dcefff;border-bottom:3px solid #ff9f1c;padding:0 10px 8px">LOADOUT</span>&nbsp;&nbsp;&nbsp;&nbsp;EMBERS&nbsp;&nbsp;&nbsp;&nbsp;BESTIARY&nbsp;&nbsp;&nbsp;&nbsp;MISSIONS&nbsp;&nbsp;&nbsp;&nbsp;MAP</div>
 <div style="position:absolute;left:70px;top:120px;font-weight:700;font-size:16px;letter-spacing:3px;color:#ff9f1c">EQUIPPED 3 / 3</div>
 ${slotRows}
 <div style="position:absolute;left:70px;top:440px;font-weight:700;font-size:16px;letter-spacing:3px;color:#ff9f1c">WARD</div>
 ${row(70, 468, 470, 58, `<div style="position:absolute;left:18px;top:16px;font-weight:700;font-size:16px;color:#7caaff">GRAVEWARD BOX</div><div style="position:absolute;right:16px;top:19px;font-size:13px;color:#8fb3cc">CAPACITY 120</div>`)}
 <div style="position:absolute;left:70px;top:556px;font-weight:700;font-size:16px;letter-spacing:3px;color:#ff9f1c">AMMO</div>
 <div style="position:absolute;left:70px;top:586px;width:470px;font-size:15px;color:#dcefff;line-height:1.9;letter-spacing:1px">RIFLE 120 / 240 &nbsp;&nbsp; SHELLS 24 / 48 &nbsp;&nbsp; PISTOL 66 / 200<br>SMG 140 / 360 &nbsp;&nbsp; SNIPER 12 / 40 &nbsp;&nbsp; ROCKETS 0 / 8</div>
 <div style="position:absolute;left:600px;top:150px;width:720px;height:300px;display:flex;align-items:center;justify-content:center">
  <div style="position:absolute;width:420px;height:110px;bottom:20px;border-radius:50%;box-shadow:0 0 0 1px rgba(124,170,255,.45),0 0 50px rgba(124,170,255,.18)"></div>${gunSvg(520, '#7caaff')}</div>
 ${card({ ...LOOT, verdict: null, keys: false, dmgDelta: 0 }, 'left:790px;top:470px;width:340px')}
 <div class="note" style="position:absolute;left:640px;top:830px;width:640px;text-align:center">The gun under the cursor turns in the middle with its card under it. <b>X</b> inspects: every stat, parts, maker story.</div>
 <div style="position:absolute;left:1380px;top:120px;font-weight:700;font-size:16px;letter-spacing:3px;color:#ff9f1c">BACKPACK 5 / 12</div>
 <div style="position:absolute;left:1730px;top:122px;font-size:13px;color:#8fb3cc;letter-spacing:1.5px">R SORT: BEST</div>
 ${packRows}${emptyRows}
 <div style="position:absolute;left:0;right:0;top:1030px;text-align:center;font-weight:700;font-size:14px;letter-spacing:2px;color:#dcefff">
  <span class="cap">E</span>SWAP &nbsp;&nbsp; <span class="cap">X</span>INSPECT &nbsp;&nbsp; <span class="cap">F</span>HOLD &nbsp;&nbsp; <span class="cap">Q</span>DROP &nbsp;&nbsp; <span class="cap">TAB</span>CLOSE</div>
 <div style="position:absolute;left:70px;top:700px;font-weight:700;font-size:20px;letter-spacing:4px;color:#ff9f1c">3 · INVENTORY</div>
 <div class="note" style="position:absolute;left:70px;top:732px;width:470px">Equipped, ward and ammo on the left, the <b>backpack on the right</b> like before.</div>
</div>`);

const pages = [['sheet1', s1], ['sheet2', s2], ['sheet3', s3]];
for (const [n, h] of pages) fs.writeFileSync(path.join(here, n + '.html'), h);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	for (const [n] of pages) {
		await pg.goto('file:///' + path.join(here, n + '.html').replace(/\\/g, '/'));
		await pg.waitForTimeout(400);
		await pg.screenshot({ path: path.join(here, n + '.png') });
	}
	await browser.close();
	console.log('done');
})();
