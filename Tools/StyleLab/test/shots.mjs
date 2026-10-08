// Stills of the Style Lab: node shots.mjs <siteDir> <outDir> [--styles 0,1,2] [--shots street,chapel] [--size 1920x1080]
//   [--data <export dir>] [--hud 0] [--timeout 900]
// Writes <outDir>/<NN>_<style-id>__<shot>.png. Serves the site from disk and three.js from node_modules, runs headless
// Chromium with SwiftShader WebGL, opens the page with still=1 and, per style and shot, waits for lab.ready.
import fs from 'node:fs';
import path from 'node:path';
import { chromium } from 'playwright';
import { routePage, CHROME_ARGS, parseArgs, siteOptions } from './site.mjs';

const { pos, named } = parseArgs(process.argv.slice(2));
if (pos.length < 2) { console.log('usage: node shots.mjs <siteDir> <outDir> [--styles 0,1] [--shots street,chapel] [--size 1920x1080] [--data dir]'); process.exit(1); }
const opts = siteOptions(pos[0], named.data, named.overrides, named.extra);
const OUT = path.resolve(pos[1]);
const [W, H] = (named.size || '1920x1080').split('x').map(Number);
const SHOTS_ALL = ['street', 'fight', 'chapel', 'boothill', 'sink', 'aerial'];
const wantShots = named.shots ? named.shots.split(',') : SHOTS_ALL;
const timeout = (+named.timeout || 900) * 1000;

(async () => {
  fs.mkdirSync(OUT, { recursive: true });
  const browser = await chromium.launch({ args: CHROME_ARGS });
  const page = await browser.newPage({ viewport: { width: W, height: H }, deviceScaleFactor: 1 });
  const issues = [];
  page.on('console', (m) => { if (['error', 'warning'].includes(m.type())) issues.push(m.type() + ': ' + m.text().slice(0, 400)); });
  page.on('pageerror', (e) => issues.push('pageerror: ' + e.message));
  const base = await routePage(page, opts);
  page.setDefaultTimeout(timeout);
  const t0 = Date.now();
  const first = named.styles ? +named.styles.split(',')[0] : 0;
  await page.goto(`${base}index.html?style=${first}&shot=${wantShots[0]}&still=1&ui=0&hud=${named.hud === '0' ? 0 : 1}${named.extra ? '&extra=' + fs.readdirSync(named.extra).filter((f) => f.endsWith('.js')).map((f) => '__extra/' + f).join(',') : ''}`, { waitUntil: 'load' });
  try {
    await page.waitForFunction(() => window.lab && window.lab.isReady === true, null, { timeout, polling: 500 });
  } catch (e) {
    console.log('TIMEOUT waiting for the first still; loading text:', await page.evaluate(() => document.querySelector('#loadtext') && document.querySelector('#loadtext').textContent));
    issues.slice(0, 30).forEach((x) => console.log('  ', x));
    await page.screenshot({ path: path.join(OUT, 'timeout.png') });
    await browser.close(); process.exit(2);
  }
  console.log(`loaded in ${((Date.now() - t0) / 1000).toFixed(1)} s`);
  const styles = await page.evaluate(() => window.lab.styles);
  const want = named.styles ? named.styles.split(',').map(Number) : styles.map((s) => s.number);
  for (const n of want) {
    const st = styles.find((s) => s.number === n);
    if (!st) { console.log('no style', n); continue; }
    for (const shot of wantShots) {
      const t1 = Date.now();
      await page.evaluate(async ([s, sh]) => { await window.lab.view(s, sh); await window.lab.ready; }, [n, shot]);
      const file = path.join(OUT, `${String(n).padStart(2, '0')}_${st.id}__${shot}.png`);
      await page.screenshot({ path: file });
      console.log(`${path.basename(file)}  ${((Date.now() - t1) / 1000).toFixed(1)} s`);
    }
  }
  const state = await page.evaluate(() => window.lab.state());
  console.log('draws', state.draws, 'triangles', state.triangles, 'programs', state.programs);
  console.log('console issues:', issues.length);
  issues.slice(0, 40).forEach((e) => console.log('  ', e));
  await browser.close();
})().catch((e) => { console.error('FAILED', e); process.exit(1); });
