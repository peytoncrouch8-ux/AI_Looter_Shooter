// A local server for the Style Lab: node serve.mjs [siteDir] [--data <export dir>] [--port 8090]
// siteDir defaults to Tools/StyleLab/web (the sources, with the export under data/); three.js is served from
// node_modules, so it works offline. Open http://localhost:8090/?style=0&shot=street
import http from 'node:http';
import { resolve, parseArgs, siteOptions } from './site.mjs';

const { pos, named } = parseArgs(process.argv.slice(2));
const opts = siteOptions(pos[0], named.data, named.overrides, named.extra);
opts.localThree = named.cdn !== 'true';
const port = +(named.port || 8090);
http.createServer((req, res) => {
  const u = new URL(req.url, 'http://x');
  try {
    const r = resolve(u.pathname, opts);
    if (!r) { res.writeHead(404); res.end('missing ' + u.pathname); return; }
    res.writeHead(200, { 'Content-Type': r.type, 'Cache-Control': 'no-store' });
    res.end(r.body);
  } catch (e) { res.writeHead(500); res.end(String(e)); }
}).listen(port, () => console.log(`Style Lab on http://localhost:${port}/  (site ${opts.site}${opts.data ? ', data ' + opts.data : ''})`));
