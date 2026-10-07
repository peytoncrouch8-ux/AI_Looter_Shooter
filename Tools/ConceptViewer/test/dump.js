// Writes one concept's scene as JSON, exactly as the viewer builds it: every placed model (buildings, props, trees,
// rocks, fences, hedges), the creatures, livestock and townsfolk, and the roads, fields, ground patches, pools, trails,
// webs and vegetation rules. Grass tufts, flowers, cobbles, kerb bricks and crops are only counted; their areas and
// lines are in the rest of the file. Units are Unreal's: centimeters, X north, Y east, yaw in degrees from +X.
//   node dump.js <web dir> <concept index 0-3> <out.json>
const { chromium } = require('playwright');
const fs = require('fs');
const path = require('path');
const [WEB, INDEX = '0', OUT = 'concept.json'] = process.argv.slice(2);
const types = { '.html': 'text/html', '.js': 'text/javascript', '.json': 'application/json' };

// Runs inside the page's module, after boot: the same expansion as buildLayer() in app.js, without drawing anything.
const HOOK = `
window.__dumpConcept = (index) => {
  const S = CONCEPTS[index]();
  S.items.push(...expandFences(S), ...expandHedges(S), ...expandFlowerBeds(S));
  for (const c of S.cobbles) S.items.push(...cobbleItems(c));
  const decals = [], fieldItems = [], roadItems = [];
  for (const f of S.fields) fieldParts(S, f, decals, fieldItems);
  for (const r of S.roads) roadParts(r, decals, roadItems);
  S.items.push(...fieldItems, ...roadItems);
  S.items.push(...scatterVeg(S, makeBlocker(S)));
  const smoke = S.smoke.map((s) => ({ model: s.model, X: s.X, Y: s.Y }));
  return JSON.parse(JSON.stringify({ ...S, smoke, sources: Object.fromEntries(Object.entries(MANIFEST.models).map(([k, v]) => [k, v.source])) }));
};
`;
// Small, many things: counted, not listed.
const FINE = /^k:(tuft|flower|cobble|brick|wheat|cabbage|post)\b/;
// Long pieces, stretched along their run (the viewer scales their local x).
const LONG = /^(FenceRail|FenceBroken|StoneWall|k:picket)$/;

const r = (v) => Math.round(v);
const r2 = (v) => Math.round(v * 100) / 100;
const yaw = (v) => Math.round((((v % 360) + 360) % 360) * 10) / 10 % 360;
const pts = (list) => list.map(([X, Y]) => [r(X), r(Y)]);
const strip = (html) => html.replace(/<[^>]+>/g, '');

(async () => {
  const browser = await chromium.launch({ executablePath: process.env.CHROMIUM_PATH || undefined, chromiumSandbox: false,
    args: ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist'] });
  const page = await browser.newPage({ viewport: { width: 640, height: 360 }, deviceScaleFactor: 1 });
  await page.route('http://viewer.local/**', (route) => {
    const rel = decodeURIComponent(new URL(route.request().url()).pathname).replace(/^\//, '') || 'test.html';
    const file = path.join(WEB, rel);
    if (!fs.existsSync(file)) return route.fulfill({ status: 404, body: 'missing ' + rel });
    let body = fs.readFileSync(file);
    if (rel === 'test.html') {
      const html = body.toString(), at = html.lastIndexOf('</script>');
      body = html.slice(0, at) + HOOK + html.slice(at);
    }
    route.fulfill({ status: 200, body, contentType: types[path.extname(file)] || 'application/octet-stream' });
  });
  await page.route('https://cdn.jsdelivr.net/**', (route) => {
    const u = new URL(route.request().url()), m = u.pathname.match(/three@0\.160\.0\/(build\/.*|examples\/jsm\/.*)$/);
    const file = m && path.join(__dirname, 'node_modules', 'three', m[1]);
    if (!file || !fs.existsSync(file)) return route.fulfill({ status: 404, body: 'missing ' + u.pathname });
    route.fulfill({ status: 200, body: fs.readFileSync(file), contentType: 'text/javascript' });
  });
  await page.route('https://fonts.googleapis.com/**', (route) => route.fulfill({ status: 200, body: '', contentType: 'text/css' }));
  const issues = [];
  page.on('pageerror', (e) => issues.push('pageerror: ' + e.message));
  page.on('console', (m) => { if (m.type() === 'error') issues.push('error: ' + m.text().slice(0, 300)); });
  await page.goto('http://viewer.local/test.html', { waitUntil: 'load' });
  try {
    await page.waitForFunction(() => window.__viewer && window.__dumpConcept && document.querySelector('#loading').style.display === 'none', null, { timeout: 240000 });
  } catch (e) {
    console.log('TIMEOUT; progress text:', await page.evaluate(() => document.querySelector('#progtext').textContent));
    issues.slice(0, 20).forEach((x) => console.log('  ', x));
    await browser.close(); process.exit(2);
  }
  const S = await page.evaluate((i) => window.__dumpConcept(i), Number(INDEX));
  await browser.close();
  if (issues.length) { issues.forEach((e) => console.log(e)); process.exit(1); }

  const counts = {}, fine = {};
  const placements = [];
  for (const it of S.items) {
    const key = it.model.startsWith('k:') ? 'k:' + it.model.split(':')[1] : it.model;
    counts[key] = (counts[key] || 0) + 1;
    if (FINE.test(it.model)) { fine[key] = (fine[key] || 0) + 1; continue; }
    const p = { model: it.model, X: r(it.X), Y: r(it.Y), yaw: yaw(it.yaw || 0) };
    if (Array.isArray(it.scale)) { if (LONG.test(it.model)) p.stretch = r2(it.scale[0]); else p.scale = it.scale.map(r2); }
    else if (it.scale !== undefined && Math.abs(it.scale - 1) > 0.005) p.scale = r2(it.scale);
    if (it.Z !== undefined) p.Z = r(it.Z);
    if (it.lift) p.lift = r(it.lift * 100);
    if (it.sink && it.sink >= 0.2) p.sunk = r(it.sink * 100);
    if (it.tiltZ && !LONG.test(it.model)) p.tilt = r2(it.tiltZ * 180 / Math.PI);
    if (it.color && !/^#([0-9a-f]{2})\1\1$/i.test(it.color)) p.tint = it.color;
    placements.push(p);
  }
  placements.sort((a, b) => (a.model < b.model ? -1 : a.model > b.model ? 1 : a.X - b.X || a.Y - b.Y));
  const models = {};
  for (const [key, n] of Object.entries(counts).sort()) {
    models[key] = { count: n, source: key.startsWith('k:') ? 'viewer kit (no game model yet)' : S.sources[key] ? `Art/Models/${S.sources[key]}` : 'unknown' };
  }
  const out = {
    about: 'The scene of one island concept exactly as the concept viewer (Tools/ConceptViewer) draws it, written by '
      + 'Tools/ConceptViewer/test/dump.js. Unreal world centimeters: X north, Y east; yaw in degrees from +X, the convention of '
      + 'layout.json, so a game model takes the same yaw as an actor. Long pieces (FenceRail, FenceBroken, StoneWall, k:picket, '
      + 'k:hedge, k:barricade, k:sandbags) run across their facing, at yaw = run direction + 90, like the game\'s FenceRail; '
      + 'stretch is their scale along the run. Models named k:<kind>[:<variant>] come from the viewer\'s procedural kit and have '
      + 'no game model yet. Optional fields: Z (absolute height, cm) where the concept fixes it, lift (cm above the ground: '
      + 'cocoons hang this high), sunk (cm into the ground: the bogged cart), tilt (degrees, roll), tint (a tint the viewer '
      + 'applies: moss on rocks, slime on the cart). Everything else sits on the ground. Small, many things are counted in '
      + 'fineCounts and placed from the roads, fields, flowerBeds, vegetation and tuft rules instead.',
    concept: S.text.name,
    tagline: S.text.tagline,
    bullets: S.text.bullets.map(strip),
    labels: S.labels.map((l) => ({ text: l.text, X: r(l.X), Y: r(l.Y), kind: l.kind || undefined })),
    views: S.views,
    models,
    fineCounts: fine,
    creatures: S.creatures.map((c) => ({ kind: c.kind, variant: c.variant || undefined, X: r(c.X), Y: r(c.Y), yaw: yaw(c.yaw || 0) })),
    roads: S.roads.map((x) => ({ kind: x.kind, width: x.width, points: pts(x.points) })),
    fences: S.fences.map((f) => ({ kind: f.kind, points: pts(f.points), broken: f.broken || undefined, gap: f.gap })),
    hedges: S.hedges.map((h) => ({ points: pts(h.pts) })),
    fields: S.fields.map((f) => ({ kind: f.kind, X: r(f.X), Y: r(f.Y), length: f.length, width: f.width, yaw: yaw(f.yaw), corners: pts(f.poly) })),
    cobbles: S.cobbles.map((c) => ({ polygon: pts(c.poly), spacing: c.spacing, keepClear: (c.keep || []).map((k) => ({ X: r(k.X), Y: r(k.Y), r: k.r })) })),
    grounds: S.patches.map((p) => ({ color: p.color, polygon: pts(p.poly) })),
    pools: S.water.map((w) => ({ X: r(w.X), Y: r(w.Y), r: r(w.r) })),
    slimeTrails: S.trails.map((t) => ({ width: t.width, points: pts(t.pts) })),
    webs: S.webs.map((w) => (w.sheet ? { sheet: true, X: r(w.X), Y: r(w.Y), r: w.r } : { from: pts([w.a])[0], to: pts([w.b])[0], low: r(w.h0 * 100), high: r(w.h1 * 100) })),
    chimneySmoke: S.smoke.map((s) => ({ building: s.model, X: r(s.X), Y: r(s.Y) })),
    flowerBeds: S.flowerBeds.map((b) => ({ points: pts(b.pts), colors: b.colors })),
    vegetation: S.veg.map((v) => JSON.parse(JSON.stringify(v, (k, val) => (typeof val === 'number' && Math.abs(val) > 50 ? Math.round(val) : val)))),
    grassVerges: S.tufts.map((t) => ({ points: pts(t.along), offset: r(t.offset) })),
    noTrees: S.noTrees.map((z) => (z.poly ? { polygon: pts(z.poly) } : { X: r(z.X), Y: r(z.Y), r: r(z.r) })),
    placements,
  };
  // One entry per line inside the long lists, so the file reads and diffs well.
  const lines = ['{'];
  const keys = Object.keys(out);
  keys.forEach((k, i) => {
    const v = out[k], end = i < keys.length - 1 ? ',' : '';
    if (Array.isArray(v) && v.length && typeof v[0] === 'object') lines.push(`  ${JSON.stringify(k)}: [`, v.map((e) => '    ' + JSON.stringify(e)).join(',\n'), `  ]${end}`);
    else if (v && typeof v === 'object' && !Array.isArray(v) && k === 'models') lines.push(`  ${JSON.stringify(k)}: {`, Object.entries(v).map(([m, e]) => `    ${JSON.stringify(m)}: ${JSON.stringify(e)}`).join(',\n'), `  }${end}`);
    else lines.push(`  ${JSON.stringify(k)}: ${JSON.stringify(v)}${end}`);
  });
  lines.push('}');
  fs.writeFileSync(OUT, lines.join('\n') + '\n');
  console.log(`${S.text.name}: ${placements.length} placements, ${out.creatures.length} creatures, ${Object.values(fine).reduce((a, b) => a + b, 0)} fine items counted -> ${OUT}`);
})().catch((e) => { console.error('FAILED', e.message); process.exit(1); });
