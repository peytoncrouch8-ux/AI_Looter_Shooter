// Before/after of the kit's Slate & Salt palette on the inventory screens (menushots before/after). node make.js
const path = require('path');
const S = 'file:///C:/Dev/AI_Looter_Shooter/Saved/Screenshots/MenuShots/';
const shots = ['01_loadout', '09_bestiary', '10_missions'];
const row = n => `<div style="display:flex;gap:12px;margin-bottom:12px">
 <div><div class="t">BEFORE · ${n.slice(3)}</div><img src="${S}before/${n}.png"></div>
 <div><div class="t">AFTER · ${n.slice(3)}</div><img src="${S}after/${n}.png"></div></div>`;
const html = `<html><head><style>@font-face{font-family:C;src:url('file:///C:/Dev/AI_Looter_Shooter/Content/UI/Fonts/ChakraPetch-Bold.ttf')}
body{margin:0;padding:14px;background:#0c0f12}img{width:944px;display:block;border-radius:4px}.t{font:20px C;color:#d4eef4;letter-spacing:2px;margin:4px 0}</style></head>
<body>${shots.map(row).join('')}</body></html>`;
(async () => {
	const pw = require('playwright');
	const b = await pw.chromium.launch();
	const p = await b.newPage({ viewport: { width: 1928, height: 100 } });
	require('fs').writeFileSync(path.join(__dirname, 'palette.html'), html); await p.goto('file:///' + path.join(__dirname, 'palette.html').split(path.sep).join('/')); await p.waitForTimeout(1500);
	await p.screenshot({ path: path.join(__dirname, 'palette.png'), fullPage: true });
	await b.close(); console.log('done');
})();
