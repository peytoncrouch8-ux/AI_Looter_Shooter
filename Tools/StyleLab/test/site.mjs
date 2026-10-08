// Serving the Style Lab from disk, shared by serve.mjs, shots.mjs and interact.mjs.
// A site is either the assembled one (Saved/StyleLab/site: index.html, engine/, styles/, hud/, data/) or the sources
// (Tools/StyleLab/web) with the export mapped to data/. three.js comes from this folder's node_modules in place of the
// CDN, index.html (a fragment, as the artifact host wants it) gets a document skeleton, and styles/index.js lists the
// style files on disk.
import fs from 'node:fs';
import path from 'node:path';
import { fileURLToPath } from 'node:url';

const HERE = path.dirname(fileURLToPath(import.meta.url));
export const REPO = path.resolve(HERE, '..', '..', '..');
export const THREE_DIR = path.join(HERE, 'node_modules', 'three');
export const TYPES = {
  '.html': 'text/html; charset=utf-8', '.js': 'text/javascript; charset=utf-8', '.mjs': 'text/javascript; charset=utf-8',
  '.json': 'application/json', '.glb': 'model/gltf-binary', '.bin': 'application/octet-stream', '.png': 'image/png',
  '.webp': 'image/webp', '.jpg': 'image/jpeg', '.css': 'text/css; charset=utf-8', '.svg': 'image/svg+xml', '.woff2': 'font/woff2',
  '.md': 'text/markdown; charset=utf-8',
};

// The export to use when the site has no data/: the real one once its manifest exists, else the stub.
export function defaultData() {
  const real = path.join(REPO, 'Saved', 'StyleLab', 'export');
  if (fs.existsSync(path.join(real, 'manifest.json'))) return real;
  return path.join(REPO, 'Saved', 'StyleLab', 'stub');
}

export function wrapPage(fragment) {
  return '<!doctype html><html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">'
    + '</head><body>' + fragment + '</body></html>';
}

export function styleFiles(stylesDir) {
  return fs.readdirSync(stylesDir).filter((f) => /^\d\d_[\w-]+\.js$/.test(f)).sort();
}

export function rewriteStyleIndex(src, files) {
  return src.replace(/\/\*FILES\*\/[\s\S]*?\/\*END\*\//, '/*FILES*/' + files.map((f) => `'${f}'`).join(', ') + '/*END*/');
}

// Resolves a request path to { body, type } or null. opts: { site, data, localThree }
export function resolve(rel, opts) {
  rel = decodeURIComponent(rel).replace(/^\/+/, '');
  if (rel === '' || rel === 'index.html' || rel === 'test.html') {
    const test = path.join(opts.site, 'test.html');
    if (rel === 'test.html' && fs.existsSync(test)) return { body: fs.readFileSync(test), type: TYPES['.html'] };
    let html = fs.readFileSync(path.join(opts.site, 'index.html'), 'utf8');
    if (opts.localThree) html = html.replace(/https:\/\/cdn\.jsdelivr\.net\/npm\/three@0\.170\.0\//g, '/__three/');
    if (!/^\s*<!doctype/i.test(html)) html = wrapPage(html);
    return { body: Buffer.from(html), type: TYPES['.html'] };
  }
  if (rel.startsWith('__extra/') && opts.extra) {
    // dev only: style modules from outside the site (?extra=__extra/<file>)
    const f = path.join(opts.extra, path.basename(rel));
    return fs.existsSync(f) ? { body: fs.readFileSync(f), type: TYPES['.js'] } : null;
  }
  if (rel.startsWith('__three/')) {
    const f = path.join(THREE_DIR, rel.slice(8));
    return fs.existsSync(f) ? { body: fs.readFileSync(f), type: TYPES['.js'] } : null;
  }
  let file = path.join(opts.site, rel);
  if (rel.startsWith('data/') && opts.overrides && fs.existsSync(path.join(opts.overrides, rel.slice(5)))) {
    // dev only: a corrected copy of an export file
    const f = path.join(opts.overrides, rel.slice(5));
    return { body: fs.readFileSync(f), type: TYPES[path.extname(f).toLowerCase()] || 'application/octet-stream' };
  }
  if (rel.startsWith('data/') && !fs.existsSync(file) && opts.data) file = path.join(opts.data, rel.slice(5));
  if (!file.startsWith(opts.site) && !(opts.data && file.startsWith(opts.data))) return null;
  if (!fs.existsSync(file) || fs.statSync(file).isDirectory()) return null;
  let body = fs.readFileSync(file);
  if (rel === 'styles/index.js' && body.includes('/*FILES*/')) {
    body = Buffer.from(rewriteStyleIndex(body.toString('utf8'), styleFiles(path.dirname(file))));
  }
  return { body, type: TYPES[path.extname(file).toLowerCase()] || 'application/octet-stream' };
}

// Playwright: route the page's origin and the CDN to disk. Returns the page URL base.
export async function routePage(page, opts) {
  const base = 'http://stylelab.local/';
  await page.route(base + '**', (route) => {
    const u = new URL(route.request().url());
    const r = resolve(u.pathname, opts);
    if (!r) return route.fulfill({ status: 404, body: 'missing ' + u.pathname });
    return route.fulfill({ status: 200, body: r.body, contentType: r.type });
  });
  await page.route('https://cdn.jsdelivr.net/**', (route) => {
    const u = new URL(route.request().url());
    const m = u.pathname.match(/three@0\.170\.0\/(build\/.*|examples\/jsm\/.*)$/);
    const f = m && path.join(THREE_DIR, m[1]);
    if (!f || !fs.existsSync(f)) return route.fulfill({ status: 404, body: 'missing ' + u.pathname });
    return route.fulfill({ status: 200, body: fs.readFileSync(f), contentType: TYPES['.js'] });
  });
  await page.route(/https:\/\/fonts\.(googleapis|gstatic)\.com\/.*/, (route) => route.fulfill({ status: 200, body: '', contentType: 'text/css' }));
  return base;
}

export const CHROME_ARGS = ['--use-gl=angle', '--use-angle=swiftshader', '--enable-unsafe-swiftshader', '--ignore-gpu-blocklist',
  '--enable-webgl', '--disable-gpu-sandbox', '--no-sandbox'];

export function parseArgs(argv) {
  const pos = [], named = {};
  for (let i = 0; i < argv.length; i++) {
    if (argv[i].startsWith('--')) { const k = argv[i].slice(2); const v = argv[i + 1] && !argv[i + 1].startsWith('--') ? argv[++i] : 'true'; named[k] = v; }
    else pos.push(argv[i]);
  }
  return { pos, named };
}

export function siteOptions(siteArg, dataArg, overrides, extra) {
  const site = path.resolve(siteArg || path.join(REPO, 'Tools', 'StyleLab', 'web'));
  const hasData = fs.existsSync(path.join(site, 'data', 'manifest.json'));
  const data = dataArg ? path.resolve(dataArg) : (hasData ? null : defaultData());
  return { site, data, localThree: false, overrides: overrides ? path.resolve(overrides) : null, extra: extra ? path.resolve(extra) : null };
}
