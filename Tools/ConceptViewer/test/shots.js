// Loads the assembled viewer in headless Chromium (software GL), serving its files from disk and three.js from npm,
// then screenshots views of the chosen concepts through window.__viewer.
//   node shots.js <web dir> <out dir> [concepts 0,1,2,3] [views above:Island,foot:Spawn | all] [WxH] [clean|ui] [labels|nolabels]
const { chromium } = require('playwright');
const fs = require('fs');
const path = require('path');
const [WEB, OUT, conceptArg = '0,1,2,3', viewArg = 'all', sizeArg = '1280x720', uiArg = 'clean', labelArg = 'labels'] = process.argv.slice(2);
const concepts = conceptArg.split(',').map(Number);
const size = sizeArg.split('x').map(Number);
const types = { '.html': 'text/html', '.js': 'text/javascript', '.json': 'application/json', '.glb': 'model/gltf-binary', '.bin': 'application/octet-stream', '.png': 'image/png' };
(async () => {
  fs.mkdirSync(OUT, { recursive: true });
  const browser = await chromium.launch({ executablePath: process.env.CHROMIUM_PATH || undefined, chromiumSandbox: false,
    args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const page = await browser.newPage({ viewport: { width: size[0], height: size[1] }, deviceScaleFactor: 1 });
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
  const t0 = Date.now();
  await page.goto('http://viewer.local/test.html', { waitUntil: 'load' });
  try {
    await page.waitForFunction(() => window.__viewer && document.querySelector('#loading').style.display === 'none', null, { timeout: 240000 });
  } catch (e) {
    console.log('TIMEOUT; progress text:', await page.evaluate(() => document.querySelector('#progtext').textContent));
    issues.slice(0, 30).forEach((x) => console.log('  ', x));
    await page.screenshot({ path: `${OUT}/timeout.png` }); await browser.close(); process.exit(2);
  }
  console.log('loaded in', ((Date.now() - t0) / 1000).toFixed(1), 's');
  const frames = (n) => page.evaluate((k) => new Promise((r) => { const step = () => (k-- <= 0 ? r() : requestAnimationFrame(step)); step(); }), n);
  await page.evaluate(([u, l]) => { window.__viewer.ui(u === 'ui'); window.__viewer.labels(l === 'labels'); }, [uiArg, labelArg]);
  for (const c of concepts) {
    const t1 = Date.now();
    await page.evaluate((i) => window.__viewer.selectConcept(i), c);
    const st = await page.evaluate(() => window.__viewer.state());
    console.log(`concept ${c + 1} built in ${((Date.now() - t1) / 1000).toFixed(1)} s; ${st.counts} meshes; ratio ${st.ratio}`);
    const all = [...st.views.above.map((v) => ['above', v.name]), ...st.views.foot.map((v) => ['foot', v.name])];
    const want = viewArg === 'all' ? all : viewArg.split(',').map((s) => { const i = s.indexOf(':'); return [s.slice(0, i), s.slice(i + 1)]; });
    for (const [kind, name] of want) {
      // pose:<name>:X:Y:dist:yaw:pitch places an aerial camera directly (Unreal cm, meters, degrees).
      const ok = kind === 'pose'
        ? await page.evaluate((a) => { window.__viewer.pose('above', { X: +a[1], Y: +a[2], dist: +a[3], yaw: +a[4], pitch: +a[5], h: 0 }); return true; }, name.split(':').length > 1 ? name.split(':') : [name])
        : await page.evaluate(([k, n]) => window.__viewer.view(k, n), [kind, name]);
      if (!ok) { console.log('  no view', kind, name); continue; }
      await frames(3);
      const file = `${OUT}/c${c + 1}_${kind}_${name.split(':')[0].toLowerCase().replace(/\W+/g, '_')}.png`;
      await page.screenshot({ path: file });
    }
  }
  console.log('console issues:', issues.length); issues.slice(0, 25).forEach((e) => console.log('  ', e));
  await browser.close();
})().catch((e) => { console.error('FAILED', e.message); process.exit(1); });
