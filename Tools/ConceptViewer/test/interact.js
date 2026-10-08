// Drives the viewer like a person would: tour, concept keys, compare, walking, the minimap, double-click and the
// toggles. Prints what happened at each step and any console errors; saves a screenshot per step.
const { chromium } = require('playwright');
const fs = require('fs');
const path = require('path');
const [WEB, OUT] = process.argv.slice(2);
const types = { '.html': 'text/html', '.js': 'text/javascript', '.json': 'application/json', '.glb': 'model/gltf-binary', '.bin': 'application/octet-stream' };
(async () => {
  fs.mkdirSync(OUT, { recursive: true });
  const browser = await chromium.launch({ executablePath: process.env.CHROMIUM_PATH || undefined, chromiumSandbox: false,
    args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const page = await browser.newPage({ viewport: { width: 1280, height: 720 }, deviceScaleFactor: 1 });
  await page.route('http://viewer.local/**', (route) => {
    const rel = decodeURIComponent(new URL(route.request().url()).pathname).replace(/^\//, '') || 'test.html';
    const file = path.join(WEB, rel);
    if (!fs.existsSync(file)) return route.fulfill({ status: 404, body: 'missing ' + rel });
    route.fulfill({ status: 200, body: fs.readFileSync(file), contentType: types[path.extname(file)] || 'application/octet-stream' });
  });
  await page.route('https://cdn.jsdelivr.net/**', (route) => {
    const u = new URL(route.request().url()), m = u.pathname.match(/three@0\.160\.0\/(build\/.*|examples\/jsm\/.*)$/);
    const file = m && path.join(__dirname, 'node_modules', 'three', m[1]);
    if (!file || !fs.existsSync(file)) return route.fulfill({ status: 404, body: 'missing ' + u.pathname });
    route.fulfill({ status: 200, body: fs.readFileSync(file), contentType: 'text/javascript' });
  });
  await page.route('https://fonts.googleapis.com/**', (route) => route.fulfill({ status: 200, body: '', contentType: 'text/css' }));
  const issues = [];
  page.on('console', (m) => { if (['error', 'warning'].includes(m.type())) issues.push(m.type() + ': ' + m.text().slice(0, 300)); });
  page.on('pageerror', (e) => issues.push('pageerror: ' + e.message));
  await page.goto('http://viewer.local/test.html', { waitUntil: 'load' });
  await page.waitForFunction(() => window.__viewer && document.querySelector('#loading').style.display === 'none', null, { timeout: 240000 });
  const state = () => page.evaluate(() => { const s = window.__viewer.state(); delete s.views; return s; });
  const wait = (ms) => page.waitForTimeout(ms);
  const shot = (name) => page.screenshot({ path: `${OUT}/${name}.png` });
  const log = async (step) => console.log(step.padEnd(28), JSON.stringify(await state()));
  await log('loaded');

  await page.click('#tour'); await wait(2500);
  console.log('tour button reads', await page.textContent('#tour')); await shot('1_tour');
  await page.mouse.click(700, 360); await wait(300);
  console.log('after click, tour reads', await page.textContent('#tour'));

  await page.keyboard.press('2'); await page.waitForFunction(() => window.__viewer.state().concept === 1, null, { timeout: 120000 }); await wait(800);
  await log('key 2'); await shot('2_concept2');

  await page.click('#compareBtn');
  await page.waitForFunction(() => !document.querySelector('#compare').hidden, null, { timeout: 240000 }); await wait(500);
  console.log('compare images', await page.$$eval('#compareGrid img', (a) => a.map((i) => i.naturalWidth + 'x' + i.naturalHeight)));
  await shot('3_compare');
  await page.click('#compareGrid figure:nth-child(3)');
  await page.waitForFunction(() => window.__viewer.state().concept === 2, null, { timeout: 120000 }); await wait(500);
  await log('picked 3 in compare');

  await page.click('#mode'); await wait(3500);
  await log('walk here');
  const p0 = await page.evaluate(() => window.__viewer.state());
  await page.focus('#view'); await page.keyboard.down('KeyW'); await wait(1500); await page.keyboard.up('KeyW'); await wait(200);
  await log('walked forward'); await shot('4_walk');
  const box = await page.$eval('#map', (m) => { const r = m.getBoundingClientRect(); return [r.left, r.top, r.width, r.height]; });
  await page.mouse.click(box[0] + box[2] * 0.62, box[1] + box[3] * 0.35); await wait(3500);
  await log('map click'); await shot('5_mapwalk');
  await page.click('#mode'); await wait(3500);
  await log('fly up'); await shot('6_flyup');
  await page.mouse.dblclick(640, 400); await wait(3000);
  await log('double click'); await shot('7_dblclick');
  await page.click('#labelsBtn'); await wait(600);
  console.log('labels pressed', await page.getAttribute('#labelsBtn', 'aria-pressed'));
  await shot('8_toggles');
  await page.click('#fold'); await wait(300);
  console.log('details open', await page.getAttribute('#fold', 'aria-expanded'), 'bullets', await page.$$eval('#bullets li', (l) => l.length));
  await shot('9_details');
  console.log('console issues:', issues.length); issues.slice(0, 20).forEach((e) => console.log('  ', e));
  await browser.close();
})().catch((e) => { console.error('FAILED', e.message); process.exit(1); });
