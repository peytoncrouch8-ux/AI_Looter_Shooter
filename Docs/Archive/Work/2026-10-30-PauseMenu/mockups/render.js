// Renders the pause-menu mockups (every *_hub.html / *_settings.html here, or the files named) to 1920x1080 PNGs,
// then the contact sheet (sheet.html -> sheet.png, 1920x1200) when the four hubs exist.
//   node render.js [A_hub.html ...]
const fs = require('fs');
const path = require('path');
let playwright;
try { playwright = require('playwright'); } catch { playwright = require('/opt/node22/lib/node_modules/playwright'); }

const here = __dirname;
let files = process.argv.slice(2);
if (!files.length) files = [...fs.readdirSync(here).filter(f => /^[A-D]_(hub|settings)\.html$/.test(f)).sort(), 'sheet.html'];

(async () => {
	const browser = await playwright.chromium.launch();
	const page = await browser.newPage({ viewport: { width: 1920, height: 1080 } });
	page.on('console', m => console.log('[page]', m.text()));
	page.on('pageerror', e => console.log('[error]', e.message));
	for (const f of files) {
		const isSheet = f === 'sheet.html', isSheet2 = f === 'sheet2.html' || f === 'sheet3.html';
		await page.setViewportSize({ width: 1920, height: isSheet ? 1200 : isSheet2 ? 720 : 1080 });
		await page.goto('file:///' + path.join(here, f).split(path.sep).join('/'));
		await page.evaluate(() => document.fonts.ready);
		await page.waitForTimeout(150);
		const out = path.join(here, f.replace(/\.html$/, '.png'));
		await page.screenshot({ path: out });
		console.log('->', out);
	}
	await browser.close();
})().catch(e => { console.error(e.message || e); process.exit(1); });
