// Drives the Style Lab like a player: node interact.mjs [siteDir] [outDir] [--data dir] [--size 1280x720]
// Loads the page live (not still), switches through every style (keys and lab.setStyle) and every shot, walks with
// WASD, sprints, jumps, fires bursts at the spiders and reloads, toggles the HUD and the card, and reports console
// errors, per-style switch times, frame rates and the player's state. Saves a screenshot per step.
import fs from 'node:fs';
import path from 'node:path';
import { chromium } from 'playwright';
import { routePage, CHROME_ARGS, parseArgs, siteOptions, REPO } from './site.mjs';

const { pos, named } = parseArgs(process.argv.slice(2));
const opts = siteOptions(pos[0], named.data, named.overrides, named.extra);
const OUT = path.resolve(pos[1] || path.join(REPO, 'Saved', 'StyleLab', 'shots', 'interact'));
const [W, H] = (named.size || '960x540').split('x').map(Number);

(async () => {
  fs.mkdirSync(OUT, { recursive: true });
  const browser = await chromium.launch({ args: CHROME_ARGS });
  const page = await browser.newPage({ viewport: { width: W, height: H } });
  const errors = [], warnings = [];
  page.on('console', (m) => {
    const t = m.text();
    if (m.type() === 'error') errors.push(t.slice(0, 400));
    else if (m.type() === 'warning' && !/GPU stall|parallel_shader_compile/.test(t)) warnings.push(t.slice(0, 300));
  });
  page.on('pageerror', (e) => errors.push('pageerror: ' + e.message));
  page.on('crash', () => { console.log('PAGE CRASHED'); errors.push('page crashed'); });
  const step = (t) => console.log(`[${((Date.now() - t0) / 1000).toFixed(0)} s] ${t}`);
  page.setDefaultTimeout(600000);
  const t0 = Date.now();
  const base = await routePage(page, opts);
  // headless Chromium leaks memory without bound while the pointer is locked (gigabytes a minute), so the test never
  // locks it: the trigger is pulled through the player, the keys go to the page as usual
  if (named.lock !== 'true') await page.addInitScript(() => { HTMLCanvasElement.prototype.requestPointerLock = function () { return Promise.resolve(); }; });
  const extra = named.extra ? '&extra=' + fs.readdirSync(named.extra).filter((f) => f.endsWith('.js')).map((f) => '__extra/' + f).join(',') : '';
  await page.goto(`${base}index.html?style=0&shot=street&stats=1&sync=1${extra}`, { waitUntil: 'load' });
  await page.waitForFunction(() => window.lab && window.lab.isReady === true, null, { timeout: 600000, polling: 500 });
  console.log(`loaded in ${((Date.now() - t0) / 1000).toFixed(1)} s`);
  const wait = (ms) => page.waitForTimeout(ms);
  const state = () => page.evaluate(() => window.lab.state());
  const shot = (n) => page.screenshot({ path: path.join(OUT, n + '.png') });
  const styles = await page.evaluate(() => window.lab.styles);
  console.log('styles:', styles.map((s) => `${s.number} ${s.name}`).join(' | '));

  // the card, the HUD toggle, the fold
  await page.keyboard.press('KeyI'); await wait(300); await shot('01_card');
  const cardShown = await page.evaluate(() => !document.querySelector('#card').hidden);
  await page.keyboard.press('KeyI');
  await page.keyboard.press('KeyH'); await wait(300);
  const hudOff = await page.evaluate(() => !window.lab.engine.hudOn);
  await page.keyboard.press('KeyH');
  console.log('card opens:', cardShown, '| HUD toggles:', hudOff);

  // pointer lock from a click (headless may refuse it; the test then drives the trigger directly)
  await page.mouse.click(W / 2, H / 2); await wait(300);
  const locked = await page.evaluate(() => window.lab.engine.locked);
  console.log('pointer lock:', locked);

  step('walk');
  // walk, sprint, jump, look
  const p0 = (await state()).player.pos;
  // software GL draws a frame every few seconds, so the world is stepped at 30 Hz between real key events
  const tick = (sec) => page.evaluate((x) => window.lab.tick(x), sec);
  await page.keyboard.down('KeyW'); await tick(1.5);
  await page.keyboard.down('ShiftLeft'); await tick(1.2); await page.keyboard.up('ShiftLeft');
  await page.keyboard.press('Space'); await tick(0.6);
  await page.keyboard.up('KeyW');
  await page.keyboard.down('KeyA'); await tick(0.6); await page.keyboard.up('KeyA');
  if (locked) { await page.mouse.move(W / 2 + 120, H / 2); await wait(200); }
  else await page.evaluate(() => window.lab.engine.player.onMouse(-200, 20));
  const p1 = (await state()).player.pos;
  console.log('walked', Math.hypot(p1[0] - p0[0], p1[2] - p0[2]).toFixed(1), 'm');
  await shot('02_walked');

  step('fire');
  // fire bursts toward the nearest spider, reload
  const aim = () => page.evaluate(() => {
    const e = window.lab.engine, p = e.player;
    const alive = e.spiders.list.filter((s) => s.alive && s.state !== 'dead');
    if (!alive.length) return false;
    const cam = e.camera.position;
    alive.sort((a, b) => a.pos.distanceTo(cam) - b.pos.distanceTo(cam));
    const t = alive[0].pos.clone(); t.y += e.spiders.height * 0.55;
    const d = t.sub(cam).normalize();
    p.yaw = (Math.atan2(-d.x, d.z) * 180 / Math.PI + 360) % 360; p.pitch = Math.asin(d.y) * 180 / Math.PI;
    return true;
  });
  let kills = 0;
  for (let burst = 0; burst < 6; burst++) {
    await aim();
    if (locked) await page.mouse.down(); else await page.evaluate(() => { const p = window.lab.engine.player; p.firing = true; p.tap = true; });
    await tick(0.45);
    if (locked) await page.mouse.up(); else await page.evaluate(() => { window.lab.engine.player.firing = false; });
    await tick(0.25);
  }
  const st1 = await state();
  const shotsFired = 24 - st1.player.mag;
  kills = st1.spiders.filter((s) => s.state === 'dead' || !s.alive).length;
  await shot('03_fired');
  await page.keyboard.press('KeyR'); await tick(2.1);
  const st2 = await state();
  console.log(`fired ${shotsFired} rounds: mag ${st1.player.mag} -> after reload ${st2.player.mag}; spiders down ${kills}; player health ${st2.player.health}`);

  step('styles');
  // every style by key and every shot
  const times = {};
  for (const s of styles) {
    const key = s.number === 0 ? 'Backquote' : s.number === 10 ? 'Digit0' : s.number < 10 ? 'Digit' + s.number : null;
    const t1 = Date.now();
    if (key) await page.keyboard.press(key); else await page.evaluate((n) => { window.lab.setStyle(n); }, s.number);
    await page.waitForFunction((n) => window.lab.state().style === n, s.number, { timeout: 120000 });
    await wait(400);
    times[s.number] = { wall: Date.now() - t1, engine: Math.round((await state()).timings['style' + s.number] || 0) };
    await shot(`10_style_${String(s.number).padStart(2, '0')}`);
    step('style ' + s.number + ' ' + times[s.number].wall + ' ms');
  }
  for (let i = 0; i < 6; i++) { await page.keyboard.press('KeyV'); await wait(700); }
  await page.keyboard.press('KeyB'); await wait(500);
  // back and forth again: cached styles should switch faster
  const again = {};
  for (const s of styles) {
    const t1 = Date.now();
    await page.evaluate((n) => window.lab.setStyle(n), s.number);
    again[s.number] = Date.now() - t1;
  }
  const fin = await state();
  console.log('style switch (ms, first time wall/engine | cached):');
  for (const s of styles) console.log(`  ${String(s.number).padStart(2)} ${s.name.padEnd(28)} ${times[s.number].wall} / ${times[s.number].engine} | ${again[s.number]}`);
  console.log(`fps ${fin.fps ? fin.fps.toFixed(1) : '?'} (SwiftShader), draws ${fin.draws}, triangles ${fin.triangles}, programs ${fin.programs}`);
  console.log('console errors:', errors.length);
  errors.slice(0, 30).forEach((e) => console.log('  ', e));
  console.log('warnings:', warnings.length);
  warnings.slice(0, 15).forEach((e) => console.log('  ', e));
  await browser.close();
  process.exit(errors.length ? 3 : 0);
})().catch((e) => { console.error('FAILED', e); process.exit(1); });
