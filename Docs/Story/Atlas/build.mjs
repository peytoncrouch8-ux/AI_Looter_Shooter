// Builds the Skyreach Story Atlas page: reads the story docs and the island layout, draws the charts and diagrams
// into static SVG, and writes StoryAtlas.html beside this file. Run: npm install, then node build.mjs
import fs from 'node:fs';
import path from 'node:path';
import vm from 'node:vm';
import { fileURLToPath } from 'node:url';
import { marked } from 'marked';
import { STORIES, POSTERS, POSTER_ICONS, MASTERWORK_TYPES, AS_BUILT, PLACES, CAPTIONS } from './src/data.mjs';

const HERE = path.dirname(fileURLToPath(import.meta.url));
const REPO = path.resolve(HERE, '../../..');
const SRC = path.join(HERE, 'src');
const read = (p) => fs.readFileSync(p, 'utf8');

// --- The island, from the level's own layout files ---
const layout = JSON.parse(read(path.join(REPO, 'Art/Levels/TutorialIsland/layout.json')));
const computed = JSON.parse(read(path.join(REPO, 'Art/Levels/TutorialIsland/layout_computed.json')));
const zone = (id) => layout.zones.find((z) => z.id === id).polygon;
const r1 = (v) => Math.round(v);
const ISLAND = {
  outline: layout.outline,
  features: layout.features,
  ramp: computed.ramp.points.map((p) => p.map(r1)),
  creek: computed.creek.points.map((p) => p.map(r1)),
  roads: Object.values(computed.roads).map((r) => ({ width: r.width, points: r.points.map((p) => p.map(r1)) })),
  zones: { forest: zone('forest'), farmstead: zone('farmstead'), orchard: zone('orchard') },
  placements: computed.placements,
  waterfall: computed.waterfall,
  orchardRows: computed.orchardRows.map((o) => ({ start: o.start, end: o.end })),
};

// --- Run the drawing code here too, so the page ships finished SVG ---
const ctx = vm.createContext({ window: { ISLAND }, console, Math, JSON });
for (const f of ['charts.js', 'diagrams.js', 'island3d.js']) vm.runInContext(read(path.join(SRC, f)), ctx, { filename: f });
const Charts = vm.runInContext('Charts', ctx);
const Diagrams = vm.runInContext('Diagrams', ctx);
const Island = vm.runInContext('Island', ctx);
const gunIcon = vm.runInContext('gunIcon', ctx);
const gunType = vm.runInContext('gunType', ctx);
const esc = vm.runInContext('esc', ctx);

// --- The story docs ---
const strip = (s) => s.replace(/\*\*/g, '').replace(/\*/g, '').trim();
function section(md, heading) {
  const i = md.indexOf('\n## ' + heading);
  if (i < 0) return '';
  const rest = md.slice(i + 1);
  const j = rest.indexOf('\n## ', 3);
  return j < 0 ? rest : rest.slice(0, j);
}
function tableRows(text, headerStart) {
  const lines = text.split('\n');
  const h = lines.findIndex((l) => l.startsWith(headerStart));
  if (h < 0) return [];
  const rows = [];
  for (let k = h + 2; k < lines.length && lines[k].startsWith('|'); k++) rows.push(lines[k].split('|').slice(1, -1).map((c) => c.trim()));
  return rows;
}
function firstQuote(text) {
  const lines = text.split('\n');
  const i = lines.findIndex((l) => l.startsWith('>'));
  if (i < 0) return '';
  const q = [];
  for (let k = i; k < lines.length && lines[k].startsWith('>'); k++) q.push(lines[k].replace(/^>\s?/, ''));
  return strip(q.join(' '));
}
function parseStory(md) {
  const lines = md.split('\n');
  const [, num, title] = lines[0].match(/^# (\d+)\. (.+)$/);
  let i = 1;
  while (!lines[i].startsWith('>')) i++;
  const tag = [];
  while (lines[i].startsWith('>')) tag.push(lines[i++].replace(/^>\s?/, ''));
  while (!lines[i].trim()) i++;
  const pitch = [];
  while (lines[i].trim() && !lines[i].startsWith('|')) pitch.push(lines[i++].trim());
  while (!lines[i].startsWith('|')) i++;
  const glance = {};
  for (; lines[i] && lines[i].startsWith('|'); i++) {
    const c = lines[i].split('|').slice(1, -1).map((x) => x.trim());
    if (c[0] && !c[0].startsWith('---')) glance[c[0]] = c[1];
  }
  const acts = [...md.matchAll(/^### Act (I{1,3}): (.+?) \(levels (\d+)–(\d+)\)$/gm)].map((m) => ({ n: m[1], name: m[2], a: +m[3], b: +m[4] }));
  const areas = tableRows(section(md, 'Areas'), '| Area |').map((r) => {
    const [a, b] = r[1].split('–').map(Number);
    return { name: strip(r[0]), a, b };
  });
  const legendaries = tableRows(md, '| Legendary |').map((r) => ({ name: strip(r[0]), base: strip(r[1]), effect: r[2], flavor: strip(r[3]) }));
  const masterworks = tableRows(md, '| Masterwork |').map((r) => ({ name: strip(r[0]), where: strip(r[1]), effect: r[2], flavor: strip(r[3]) }));
  const moments = section(md, 'Signature moments').split('\n').filter((l) => l.startsWith('- ')).map((l) => l.slice(2));
  const stepLines = section(md, 'Skyreach in this story').split('\n');
  const steps = [];
  for (const l of stepLines) {
    if (/^\d+\. /.test(l)) steps.push(l.replace(/^\d+\.\s+/, ''));
    else if (/^\s{2,}\S/.test(l) && steps.length) steps[steps.length - 1] += ' ' + l.trim();
  }
  const stepHtml = steps.map((s) => marked.parseInline(s.replace(/^\*\*.+?\*\*\s*/, '')));
  const body = lines.slice(1).join('\n');
  const html = marked.parse(body).replace(/<table>/g, '<div class="tablewrap"><table>').replace(/<\/table>/g, '</table></div>');
  return {
    num: +num, title, tagline: strip(tag.join(' ')), pitch: marked.parseInline(pitch.join(' ')), glance, acts, areas, legendaries, masterworks,
    moments: moments.map((m) => marked.parseInline(m)), steps: stepHtml,
    protQuote: firstQuote(section(md, 'Protagonist')), antQuote: firstQuote(section(md, 'Antagonist')), html,
  };
}

const stories = STORIES.map((s) => {
  const doc = parseStory(read(path.join(REPO, 'Docs/Story', s.file)));
  return { ...s, ...doc, beats: s.beats.map(([l, st, t, d]) => ({ l, s: st, t, d })) };
});

// --- Page pieces ---
const inline = (s) => marked.parseInline(s);
const splitWho = (s) => {
  const i = s.indexOf(', ');
  const cap = (t) => t.charAt(0).toUpperCase() + t.slice(1);
  return i < 0 ? [s, ''] : [s.slice(0, i), cap(s.slice(i + 2))];
};
const badge = (s) => (s.pick ? '<span class="badge">My pick</span>' : s.second ? '<span class="badge second">Runner-up</span>' : '');

function card(s) {
  return `<a class="card${s.pick ? ' pick' : ''}" href="#s-${s.id}">
    <canvas data-art="${s.scene}" aria-hidden="true"></canvas>
    <div class="meta"><div class="num">Story ${s.num}</div><h3>${esc(s.title)}${badge(s)}</h3><p>${esc(s.genre)}</p></div>
  </a>`;
}

function compare() {
  const rows = stories.map((s) => ({ title: s.title, ...s.rating, pick: s.pick, second: s.second }));
  const meter = (v) => `<span class="meter" aria-hidden="true"><i style="width:${v * 10}%"></i></span>${v}`;
  const table = `<div class="scroll-x"><table class="matrix">
    <thead><tr><th>Story</th><th>Fits what's built</th><th>Cheap to make</th><th>Endgame</th><th>Originality</th></tr></thead>
    <tbody>${stories.map((s) => `<tr class="${s.pick ? 'pick' : s.second ? 'second' : ''}"><td><a href="#s-${s.id}">${esc(s.title)}</a></td><td class="num">${meter(s.rating.fit)}</td><td class="num">${meter(+(10 - s.rating.cost).toFixed(1))}</td><td class="num">${meter(s.rating.endgame)}</td><td class="num">${meter(s.rating.orig)}</td></tr>`).join('')}</tbody>
  </table></div>`;
  return `<section class="block" id="compare"><div class="wrap">
    <header><div class="eyebrow">Side by side</div><h2>Which one to build</h2>
    <p>My ratings out of ten, from reading the stories against what's already built. Cost counts what each story needs that doesn't exist yet: airships, giant creatures, a second art kit, a dive builder.</p></header>
    <div class="compare">
      <figure class="panel fig">
        <div class="scroll-x">${Charts.scatter(rows)}</div>
        <div class="legend"><span class="lg-pick">My pick</span><span class="lg-second">Runner-up</span><span>The others</span></div>
        <figcaption>Cost to make against originality. Hover a point for all four ratings; the table has every number.</figcaption>
      </figure>
      <div class="panel">
        ${table}
        <div class="verdict">
          <h3>If I had to pick one: Revenant</h3>
          <ul>
            <li>It's built like a looter shooter: seven names on a list from the first hour, each a place, a boss and a legendary.</li>
            <li>It fits the art you've built. It's a western, and the Unpaid add a whole enemy family without a second art kit.</li>
            <li>It gives the Reliquary its best meaning and hands you an endgame, the Ledger's bounties, without contortions.</li>
          </ul>
          <p><b>Runner-up: Below.</b> The best endgame of the eight (dives that scale forever) and the most reuse of the island kit. Pick it if the long-term loop matters more than the campaign.</p>
          <p class="dim">Safest: The Lodestone War. Most original: Leviathan. Most loot-first: Boomtown. Best twist for Skyreach: Skyfarers. A mix that works: Revenant's campaign with Below's dives as its endgame, the dead going down into the clouds.</p>
        </div>
      </div>
    </div>
  </div></section>`;
}

function islandSection() {
  const STEPS = { built: AS_BUILT.map((h, i) => ({ where: PLACES[i], html: h, pin: [0, 2, 3, 4, 6].includes(i) })) };
  for (const s of stories) STEPS[s.id] = s.steps.map((h, i) => ({ where: PLACES[i], html: h, pin: [0, 2, 3, 4, 6].includes(i) }));
  const btns = [`<button type="button" data-story="built" aria-pressed="true">As built</button>`]
    .concat(stories.map((s) => `<button type="button" data-story="${s.id}" aria-pressed="false">${s.num}. ${esc(s.title)}</button>`)).join('');
  const html = `<section class="block" id="island"><div class="wrap">
    <header><div class="eyebrow">The tutorial island</div><h2>Skyreach, eight ways</h2>
    <p>Your island, rebuilt from <code>Art/Levels/TutorialIsland/layout.json</code>: the outline, the plateau and its ramp, the hills, pond, creek and waterfall, the roads, and every building and dummy where the level places them. Pick a story to read how each step of the tutorial plays in it.</p></header>
    <div class="island">
      <div class="model" id="model">
        <canvas aria-label="3D model of Skyreach. Drag to turn it."></canvas>
        <div class="pins" aria-hidden="true"></div>
        <div id="plan" hidden>${Island.plan()}</div>
        <div class="seg" role="group" aria-label="View"><button type="button" data-view="3d" aria-pressed="true">3D</button><button type="button" data-view="plan" aria-pressed="false">Plan</button></div>
        <div class="model-tools"><button type="button" data-zoom="in" aria-label="Zoom in">+</button><button type="button" data-zoom="out" aria-label="Zoom out">−</button><button type="button" data-zoom="reset">Reset</button></div>
        <div class="model-note">Drag to turn · built from layout.json</div>
      </div>
      <div>
        <div class="switch" role="group" aria-label="Story">${btns}</div>
        <ol class="steps" id="steps"></ol>
      </div>
    </div>
  </div></section>`;
  return { html, STEPS };
}

function figure(id, s) {
  if (id === 'posters') {
    const posters = POSTERS.map(([name, role, power, ground, icon, tilt]) => `<div class="poster" style="--tilt:${tilt}deg">
      <div class="w">Wanted</div>
      <svg viewBox="0 0 48 48" aria-hidden="true">${POSTER_ICONS[icon]}</svg>
      <div class="nm">${esc(name)}</div><div class="rl">${esc(role)}</div>
      <div class="pw">Ember: ${esc(power)}</div><div class="gr">Last seen: ${esc(ground)}</div>
    </div>`).join('');
    return `<figure class="fig panel"><div class="posters">${posters}</div><figcaption>${CAPTIONS.posters}</figcaption></figure>`;
  }
  const chartLike = { settling: Charts.settling, titans: Charts.titans, hettie: Charts.hettie };
  const svg = chartLike[id] ? chartLike[id]() : Diagrams[id]();
  let extra = '';
  if (id === 'settling') {
    extra = `<div class="series-key"><span>Skyreach</span><span class="k2">Tallow Flats</span><span class="k3">Hollowmount</span></div>`;
  }
  let svgOut = svg;
  if (id === 'hettie') svgOut = svg.replace('<svg class="chart"', '<svg class="chart wide"');
  if (id === 'below') svgOut = svg.replace('<svg class="diagram"', '<svg class="diagram tall"');
  let table = '';
  if (id === 'settling') {
    const yrs = [-30, -25, -20, -15, -10, -5, -4, -3, -2, -1, 0];
    const sky = (y) => -0.05 * (y + 30), tal = (y) => (y < -4 ? -0.06 * (y + 30) : [-1.6, -3.1, -5.2, -7.8, -12][y + 4]), hol = (y) => (y < -18 ? -0.05 * (y + 30) : -0.6 - (39.4 * (y + 18)) / 18);
    table = `<details class="table-view"><summary>Table view</summary><div class="scroll-x"><table><thead><tr><th>When</th><th class="num">Skyreach</th><th class="num">Tallow Flats</th><th class="num">Hollowmount</th></tr></thead><tbody>${yrs.map((y) => `<tr><td>${y === 0 ? 'This spring' : -y + ' years ago'}</td><td class="num">${sky(y).toFixed(1)} m</td><td class="num">${tal(y).toFixed(1)} m</td><td class="num">${hol(y).toFixed(1)} m</td></tr>`).join('')}</tbody></table></div></details>`;
  }
  if (id === 'titans') {
    table = `<details class="table-view"><summary>Table view</summary><table><thead><tr><th>Creature</th><th class="num">Length</th></tr></thead><tbody>${[['A person', '1.8 m'], ['Biscuit, a calf', '4 m'], ['A grazer titan', '150 m'], ['Old Hollow (Skyreach)', '320 m'], ['Marrowdeep', '1.2 km'], ['Bellwether', '6 km'], ['The Sovereign', '40 km']].map(([a, b]) => `<tr><td>${a}</td><td class="num">${b}</td></tr>`).join('')}</tbody></table></details>`;
  }
  return `<figure class="fig panel">${extra}<div class="scroll-x">${svgOut}</div><figcaption>${CAPTIONS[id]}</figcaption>${table}</figure>`;
}

function storySection(s) {
  const [pName, pDesc] = splitWho(strip(s.glance['Protagonist'] || ''));
  const [aName, aDesc] = splitWho(strip(s.glance['Antagonist'] || ''));
  const facts = ['Feels like', 'Themes', 'Hub', 'Endgame'].filter((k) => s.glance[k]).map((k) => `<div><dt>${k}</dt><dd>${inline(s.glance[k])}</dd></div>`).join('');
  const figs = s.figs.map((f) => figure(f, s)).join('');
  const beats = s.beats.map((b, i) => `<li data-beat="${i}"><span class="n">${i + 1}</span><div><b>${esc(b.t)}</b><em>LV ${b.l}</em><span>${esc(b.d)}</span></div></li>`).join('');
  let loot;
  if (s.masterworks.length) {
    loot = `<div class="sub">The Masterworks</div><div class="loot">${s.masterworks.map((m) => `<div class="lcard">
      <div class="rar">${m.name === 'The Last Word' ? 'The thirteenth' : 'Masterwork'}</div>${gunIcon(MASTERWORK_TYPES[m.name] || 'rifle')}
      <h4>${esc(m.name)}</h4><div class="where">${esc(m.where === '—' ? 'Hidden' : m.where)}</div><p>${inline(m.effect)}</p><p class="flavor">${esc(m.flavor)}</p></div>`).join('')}</div>`;
  } else {
    loot = `<div class="sub">Legendaries</div><div class="loot">${s.legendaries.map((l) => `<div class="lcard">
      <div class="rar">Legendary</div>${gunIcon(gunType(l.base))}<h4>${esc(l.name)}</h4><div class="base">${esc(l.base)}</div>
      <p>${inline(l.effect)}</p><p class="flavor">${esc(l.flavor)}</p></div>`).join('')}</div>`;
  }
  return `<section class="story" id="s-${s.id}"><div class="wrap">
    <div class="banner">
      <canvas data-art="${s.scene}" role="img" aria-label="Key art for ${esc(s.title)}"></canvas>
      <div class="over"><div class="num">Story ${s.num} · ${esc(s.genre)}${s.pick ? ' · my pick' : s.second ? ' · runner-up' : ''}</div><h2>${esc(s.title)}</h2><p>${esc(s.tagline)}</p></div>
    </div>
    <div class="lede"><p>${s.pitch}</p><dl class="facts">${facts}</dl></div>
    <div class="duo">
      <div class="panel who"><div class="role">Protagonist</div><h3>${esc(pName)}</h3><p>${esc(pDesc)}</p>${s.protQuote ? `<blockquote>${esc(s.protQuote)}</blockquote>` : ''}</div>
      <div class="panel who villain"><div class="role">Antagonist</div><h3>${esc(aName)}</h3><p>${esc(aDesc)}</p>${s.antQuote ? `<blockquote>${esc(s.antQuote)}</blockquote>` : ''}</div>
    </div>
    <div class="sub">The world, drawn</div>
    <div class="figs">${figs}</div>
    <div class="sub">Plotline</div>
    <figure class="fig panel plotline"><div class="scroll-x">${Charts.plotline(s).replace('<svg class="chart"', '<svg class="chart wide"')}</div>
      <figcaption>Stakes over the campaign, by player level: numbered beats, the three acts above, the areas below with their level bands. Hover a beat or a list item.</figcaption>
      <ol class="beats">${beats}</ol>
    </figure>
    ${loot}
    <div class="sub">Signature moments</div>
    <ul class="moments">${s.moments.map((m) => `<li>${m}</li>`).join('')}</ul>
    <details class="full"><summary>Read the full story</summary><div class="doc">${s.html}</div></details>
  </div></section>`;
}

const isl = islandSection();
const nav = stories.map((s) => `<a class="chip" href="#s-${s.id}"><b>${s.num}</b>${esc(s.title)}</a>`).join('');
const body = `
<div class="topbar"><div class="wrap"><a class="brand" href="#top">Story Atlas</a><nav class="chips" aria-label="Stories"><a class="chip" href="#compare">Compare</a><a class="chip" href="#island">Skyreach</a>${nav}</nav></div></div>
<header class="hero wrap" id="top">
  <div class="eyebrow">AI_Looter_Shooter · eight story directions</div>
  <h1>Skyreach <span>Story Atlas</span></h1>
  <p>Eight directions the game could take, drawn out: key art, maps and cross-sections, plotlines by level, the legendaries each world would drop, and the tutorial island as a 3D model with every story's beats pinned on it. Nothing here is in the game yet. The full text of each story is under its section, and in <code>Docs/Story/</code> in the repo.</p>
  <div class="select">${stories.map(card).join('')}</div>
</header>
${compare()}
${isl.html}
${stories.map(storySection).join('\n')}
<footer><div class="wrap">
  <p>Built from the story docs in <code>Docs/Story/</code> and the island's <code>layout.json</code> on the <code>claude/looter-shooter-stories-w7kbxp</code> branch. The plotline stakes, the ratings and the in-world figures (the Settling Book, the titans' sizes, Hettie's life) are design sketches, not game data.</p>
</div></footer>`;

const scripts = ['art.js', 'charts.js', 'diagrams.js', 'island3d.js', 'main.js'].map((f) => read(path.join(SRC, f))).join('\n');
const page = `<title>Skyreach Story Atlas</title>
<meta name="description" content="Eight story directions for the looter shooter, with key art, maps, plotlines, loot and a 3D model of the tutorial island.">
<link rel="preconnect" href="https://fonts.googleapis.com">
<link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
<link rel="stylesheet" href="https://fonts.googleapis.com/css2?family=Barlow:ital,wght@0,400;0,500;0,600;1,400&family=Chakra+Petch:wght@500;600;700&family=Rye&display=swap">
<style>
${read(path.join(SRC, 'style.css'))}
</style>
${body}
<script src="https://cdnjs.cloudflare.com/ajax/libs/three.js/r128/three.min.js"></script>
<script>
window.ISLAND = ${JSON.stringify(ISLAND)};
const STEPS = ${JSON.stringify(isl.STEPS)};
${scripts}
</script>
`;
fs.writeFileSync(path.join(HERE, 'StoryAtlas.html'), page);
console.log('wrote', (page.length / 1024).toFixed(0), 'KB;', stories.map((s) => `${s.id}: ${s.areas.length} areas, ${s.acts.length} acts, ${s.legendaries.length || s.masterworks.length} loot, ${s.steps.length} steps, ${s.moments.length} moments`).join(' | '));
