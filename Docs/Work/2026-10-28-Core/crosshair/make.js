// GUNS lane F (Docs/Work/2026-10-28-Core/GUNS.md): a crosshair per gun kind, mockup only (Main's call overnight: built
// after the user sees it). node make.js -> crosshairs.png
const fs = require('fs');
const path = require('path');
const here = __dirname;
const shot = 'file:///C:/Dev/AI_Looter_Shooter/Saved/Screenshots/Tools/maud_shotgun.png';

// Every line is drawn twice: a dark outline under a white line, as the HUD's floating art is.
const L = (x1, y1, x2, y2, w = 2) => `<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="#05080b" stroke-width="${w + 2.4}" stroke-linecap="round" opacity=".85"/>` +
	`<line x1="${x1}" y1="${y1}" x2="${x2}" y2="${y2}" stroke="#f2f6f8" stroke-width="${w}" stroke-linecap="round"/>`;
const C = (r, w = 2, dash = '') => `<circle cx="0" cy="0" r="${r}" fill="none" stroke="#05080b" stroke-width="${w + 2.4}" opacity=".85" ${dash}/>` +
	`<circle cx="0" cy="0" r="${r}" fill="none" stroke="#f2f6f8" stroke-width="${w}" ${dash}/>`;
const D = (x, y, r = 2) => `<circle cx="${x}" cy="${y}" r="${r + 1.2}" fill="#05080b" opacity=".85"/><circle cx="${x}" cy="${y}" r="${r}" fill="#f2f6f8"/>`;
const P = (d, w = 2) => `<path d="${d}" fill="none" stroke="#05080b" stroke-width="${w + 2.4}" stroke-linecap="round" stroke-linejoin="round" opacity=".85"/>` +
	`<path d="${d}" fill="none" stroke="#f2f6f8" stroke-width="${w}" stroke-linecap="round" stroke-linejoin="round"/>`;

const KINDS = [
	['SMG', 'Rattler, spray', 'A tight dot and four short ticks: fast, light, close.', D(0, 0, 1.6) + L(0, -14, 0, -9) + L(0, 9, 0, 14) + L(-14, 0, -9, 0) + L(9, 0, 14, 0)],
	['ASSAULT RIFLE', 'the all-rounder', 'The classic cross with a gap that opens as spread grows.', L(0, -22, 0, -8) + L(0, 8, 0, 22) + L(-22, 0, -8, 0) + L(8, 0, 22, 0)],
	['LMG', 'heavy, sustained', 'Four corner brackets round a dot: wide, steady, built for long bursts.', D(0, 0, 1.8) + P('M -18 -10 L -18 -18 L -10 -18') + P('M 18 -10 L 18 -18 L 10 -18') + P('M -18 10 L -18 18 L -10 18') + P('M 18 10 L 18 18 L 10 18')],
	['REVOLVER', 'one heavy shot', 'A sharp chevron over a dot: one precise, heavy round.', D(0, 3, 2) + P('M -12 -4 L 0 -14 L 12 -4', 2.4)],
	['PUMP SHOTGUN', 'the spread', 'A broken ring the size of the pellet spread: where the pellets land.', C(24, 2.2, 'stroke-dasharray="12 6.85"') + D(0, 0, 1.6)],
	['SNIPER (HIP)', 'aim to shoot', 'Thin long hairs and an open middle: unsteady from the hip, aim it.', L(0, -30, 0, -14, 1.4) + L(0, 14, 0, 30, 1.4) + L(-30, 0, -14, 0, 1.4) + L(14, 0, 30, 0, 1.4) + D(0, 0, 1.2)],
	['LAUNCHER', 'lob and blast', 'A diamond and drop marks below it: the rocket\'s arc, its blast.', P('M 0 -12 L 12 0 L 0 12 L -12 0 Z', 2.2) + L(-6, 20, 6, 20) + L(-4, 27, 4, 27) + L(-2, 34, 2, 34)],
];
const cell = ([name, tag, words, art], i) => {
	const x = 40 + (i % 4) * 465, y = 130 + Math.floor(i / 4) * 450;
	return `<div style="position:absolute;left:${x}px;top:${y}px;width:440px">
	 <div style="font-family:Chakra;font-weight:700;font-size:22px;letter-spacing:3px;color:#d4eef4">${name}</div>
	 <div style="font-family:Chakra;font-size:14px;color:#ffe6a6;letter-spacing:1px;margin-top:2px">${tag}</div>
	 <div style="position:relative;margin-top:10px;height:280px;border-radius:6px;overflow:hidden;background:url('${shot}') ${-600 - i * 60}px -330px/2403px auto">
	  <svg style="position:absolute;left:50%;top:50%;transform:translate(-50%,-50%) scale(1.6)" width="120" height="120" viewBox="-60 -60 120 120">${art}</svg></div>
	 <div style="font-family:Chakra;font-size:14px;color:#b4bec8;margin-top:8px;line-height:1.35">${words}</div></div>`;
};
const html = `<!doctype html><html><head><meta charset="utf-8"><style>
@font-face{font-family:Chakra;src:url('file:///C:/Dev/AI_Looter_Shooter/Content/UI/Fonts/ChakraPetch-Regular.ttf');font-weight:400}
@font-face{font-family:Chakra;src:url('file:///C:/Dev/AI_Looter_Shooter/Content/UI/Fonts/ChakraPetch-Bold.ttf');font-weight:700}
body{margin:0;background:#0c0f12}</style></head><body><div style="position:relative;width:1920px;height:1080px;overflow:hidden;background:radial-gradient(ellipse at 50% 20%,#1a1f25,#07080a 70%)">
 <div style="position:absolute;left:40px;top:34px;font-family:Chakra;font-weight:700;font-size:30px;letter-spacing:4px;color:#d4eef4">A CROSSHAIR FOR EACH GUN KIND</div>
 <div style="position:absolute;left:40px;top:76px;font-family:Chakra;font-size:16px;color:#b4bec8">Each kind's own shape (shown 1.6x). White with a dark edge like the HUD; gaps still widen with spread; each kicks on a shot by its own amount (SMG small and quick, sniper and launcher big and slow).</div>
 ${KINDS.map(cell).join('')}
 <div style="position:absolute;left:1435px;top:580px;width:440px;font-family:Chakra;font-size:15px;color:#b4bec8;line-height:1.45">
  <div style="font-weight:700;color:#ffe6a6;font-size:18px;letter-spacing:2px">NOT BUILT YET</div>
  Mockup only (my call overnight): it changes what you look at every second, so you choose first. Pick a shape per kind, keep today's single cross, or mix.</div>
</div></body></html>`;
fs.writeFileSync(path.join(here, 'crosshairs.html'), html);
(async () => {
	const pw = require('playwright');
	const browser = await pw.chromium.launch();
	const pg = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	await pg.goto('file:///' + path.join(here, 'crosshairs.html').split(path.sep).join('/'));
	await pg.waitForTimeout(800);
	await pg.screenshot({ path: path.join(here, 'crosshairs.png') });
	await browser.close();
	console.log('done');
})();
