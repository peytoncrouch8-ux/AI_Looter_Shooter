// Charts: plotline graphs, the side-by-side comparison and the stories' own in-world charts. Every chart is an SVG
// string drawn to one scale; Tip and Crosshair add the hover layer after it is in the page.
const esc = (s) => String(s).replace(/[&<>"']/g, (c) => ({ '&': '&amp;', '<': '&lt;', '>': '&gt;', '"': '&quot;', "'": '&#39;' }[c]));
const fmt = (v, d = 1) => (Math.round(v * 10 ** d) / 10 ** d).toString().replace('-', '−');

// One tooltip for the whole page: the value leads, the label follows.
const Tip = (() => {
  let el;
  function ensure() {
    if (el) return el;
    el = document.createElement('div');
    el.className = 'tip';
    el.setAttribute('role', 'status');
    el.hidden = true;
    el.innerHTML = '<div class="tip-v"></div><div class="tip-l"></div>';
    document.body.appendChild(el);
    return el;
  }
  function show(v, l, x, y) {
    const t = ensure();
    t.querySelector('.tip-v').textContent = v || '';
    t.querySelector('.tip-l').textContent = l || '';
    t.hidden = false;
    const r = t.getBoundingClientRect();
    let left = x + 14, top = y + 14;
    if (left + r.width > window.innerWidth - 8) left = x - r.width - 14;
    if (top + r.height > window.innerHeight - 8) top = y - r.height - 14;
    t.style.left = Math.max(8, left) + 'px';
    t.style.top = Math.max(8, top) + 'px';
  }
  function hide() { if (el) el.hidden = true; }
  function bind(root) {
    root.querySelectorAll('[data-tip-v]').forEach((n) => {
      if (n.dataset.tipBound) return;
      n.dataset.tipBound = '1';
      const v = n.getAttribute('data-tip-v'), l = n.getAttribute('data-tip') || '';
      n.addEventListener('pointerenter', (e) => show(v, l, e.clientX, e.clientY));
      n.addEventListener('pointermove', (e) => show(v, l, e.clientX, e.clientY));
      n.addEventListener('pointerleave', hide);
      n.addEventListener('focus', () => {
        const r = n.getBoundingClientRect();
        show(v, l, r.right, r.top);
      });
      n.addEventListener('blur', hide);
    });
  }
  return { show, hide, bind };
})();

// Fritsch–Carlson monotone curve through points sorted by x: smooth without overshooting the beats.
function monotonePath(pts) {
  const n = pts.length;
  if (n < 2) return '';
  const dx = [], m = [];
  for (let i = 0; i < n - 1; i++) {
    dx[i] = pts[i + 1][0] - pts[i][0];
    m[i] = (pts[i + 1][1] - pts[i][1]) / dx[i];
  }
  const t = new Array(n);
  t[0] = m[0];
  t[n - 1] = m[n - 2];
  for (let i = 1; i < n - 1; i++) {
    if (m[i - 1] * m[i] <= 0) t[i] = 0;
    else {
      const w1 = 2 * dx[i] + dx[i - 1], w2 = dx[i] + 2 * dx[i - 1];
      t[i] = (w1 + w2) / (w1 / m[i - 1] + w2 / m[i]);
    }
  }
  let d = `M${pts[0][0].toFixed(1)},${pts[0][1].toFixed(1)}`;
  for (let i = 0; i < n - 1; i++) {
    const [x0, y0] = pts[i], [x1, y1] = pts[i + 1], h = dx[i];
    d += ` C${(x0 + h / 3).toFixed(1)},${(y0 + (t[i] * h) / 3).toFixed(1)} ${(x1 - h / 3).toFixed(1)},${(y1 - (t[i + 1] * h) / 3).toFixed(1)} ${x1.toFixed(1)},${y1.toFixed(1)}`;
  }
  return d;
}

// Rough label width in the chart's 12px UI face, generous so labels never collide.
const textW = (s, px = 12) => s.length * px * 0.6 + 4;

const Charts = {
  // The story's arc: stakes over the player's level, numbered beats, acts above, areas below.
  plotline(story) {
    const W = 1000, L = 48, Rt = 18, top = 34, plotH = 190;
    const maxLv = 70, campaignEnd = 50;
    const x = (lv) => L + ((lv - 1) / (maxLv - 1)) * (W - L - Rt);
    const y = (s) => top + plotH - (s / 10) * plotH;
    // Area track rows, packed so no label overlaps another.
    const rows = [];
    const placed = (story.areas || []).map((a) => {
      const x0 = x(a.a), x1 = x(a.b);
      const label = `${a.name} · ${a.a}–${a.b}`;
      const end = Math.max(x1, x0 + textW(label, 11.5)) + 10;
      let r = rows.findIndex((last) => last <= x0);
      if (r < 0) { r = rows.length; rows.push(0); }
      rows[r] = end;
      return { ...a, x0, x1, label, row: r };
    });
    const trackTop = top + plotH + 66;
    const rowH = 30;
    const H = trackTop + Math.max(1, rows.length) * rowH + 8;
    let s = `<svg class="chart" viewBox="0 0 ${W} ${H}" role="img" aria-label="${esc(story.title)} plotline: stakes rise across ${story.beats.length} beats from level ${story.beats[0].l} to level ${story.beats[story.beats.length - 1].l}; areas shown below by level band">`;
    // Act bands and the post-game.
    story.acts.forEach((act, i) => {
      s += `<rect x="${x(act.a).toFixed(1)}" y="${top}" width="${(x(act.b) - x(act.a)).toFixed(1)}" height="${plotH}" class="${i % 2 ? 'band-b' : 'band-a'}"/>`;
      s += `<text x="${(x(act.a) + 6).toFixed(1)}" y="${top - 12}" class="t-act">Act ${act.n} · ${esc(act.name)}</text>`;
    });
    s += `<rect x="${x(campaignEnd).toFixed(1)}" y="${top}" width="${(x(maxLv) - x(campaignEnd)).toFixed(1)}" height="${plotH}" class="band-post"/>`;
    s += `<text x="${(x(campaignEnd) + 6).toFixed(1)}" y="${top - 12}" class="t-act">Post-game</text>`;
    // Grid and axes.
    for (const g of [0, 5, 10]) {
      s += `<line x1="${L}" x2="${W - Rt}" y1="${y(g)}" y2="${y(g)}" class="${g === 0 ? 'axis' : 'grid'}"/>`;
      s += `<text x="${L - 8}" y="${y(g) + 4}" class="t-tick" text-anchor="end">${g}</text>`;
    }
    s += `<text x="${L - 8}" y="${top - 12}" class="t-tick" text-anchor="end">Stakes</text>`;
    for (const lv of [1, 10, 20, 30, 40, 50, 60, 70]) {
      s += `<line x1="${x(lv)}" x2="${x(lv)}" y1="${y(0)}" y2="${y(0) + 5}" class="axis"/>`;
      s += `<text x="${x(lv)}" y="${y(0) + 19}" class="t-tick" text-anchor="middle">${lv}</text>`;
    }
    s += `<text x="${W - Rt}" y="${y(0) + 36}" class="t-tick" text-anchor="end">Player level</text>`;
    // The curve, its wash and the beats.
    const pts = story.beats.map((b) => [x(b.l), y(b.s)]);
    const path = monotonePath(pts);
    s += `<path d="${path} L${pts[pts.length - 1][0].toFixed(1)},${y(0)} L${pts[0][0].toFixed(1)},${y(0)} Z" class="area"/>`;
    s += `<path d="${path}" class="line"/>`;
    story.beats.forEach((b, i) => {
      const [bx, by] = pts[i];
      s += `<g class="beat" tabindex="0" data-beat="${i}" data-tip-v="${i + 1}. ${esc(b.t)} · level ${b.l}" data-tip="${esc(b.d)}" aria-label="Beat ${i + 1}, level ${b.l}, stakes ${b.s}: ${esc(b.t)}. ${esc(b.d)}">`;
      s += `<circle cx="${bx.toFixed(1)}" cy="${by.toFixed(1)}" r="18" class="hit"/>`;
      s += `<circle cx="${bx.toFixed(1)}" cy="${by.toFixed(1)}" r="10.5" class="dot"/>`;
      s += `<text x="${bx.toFixed(1)}" y="${(by + 4).toFixed(1)}" class="t-dot" text-anchor="middle">${i + 1}</text></g>`;
    });
    // Area track.
    s += `<text x="${L}" y="${trackTop - 26}" class="t-act">Areas by level band</text>`;
    placed.forEach((a) => {
      const ry = trackTop + a.row * rowH;
      s += `<g class="area-bar" tabindex="0" data-tip-v="Levels ${a.a}–${a.b}" data-tip="${esc(a.name)}" aria-label="${esc(a.name)}, levels ${a.a} to ${a.b}">`;
      s += `<rect x="${a.x0.toFixed(1)}" y="${ry - 12}" width="${Math.max(a.x1 - a.x0, textW(a.label, 11.5)).toFixed(1)}" height="26" class="hit"/>`;
      s += `<text x="${a.x0.toFixed(1)}" y="${ry - 2}" class="t-area">${esc(a.label)}</text>`;
      s += `<rect x="${a.x0.toFixed(1)}" y="${ry + 3}" width="${Math.max(4, a.x1 - a.x0).toFixed(1)}" height="6" rx="3" class="bar"/></g>`;
    });
    return s + '</svg>';
  },

  // Cost to make against originality, my ratings out of ten. Emphasis: the pick and the runner-up.
  scatter(rows) {
    const W = 680, H = 460, L = 56, Rt = 24, T = 24, B = 56;
    const x = (v) => L + (v / 10) * (W - L - Rt);
    const y = (v) => T + (H - T - B) - (v / 10) * (H - T - B);
    let s = `<svg class="chart" viewBox="0 0 ${W} ${H}" role="img" aria-label="Cost to make against originality for the eight stories. Revenant and Below sit in the upper middle: original, at moderate cost.">`;
    for (const g of [0, 2.5, 5, 7.5, 10]) {
      s += `<line x1="${x(0)}" x2="${x(10)}" y1="${y(g)}" y2="${y(g)}" class="${g === 0 ? 'axis' : 'grid'}"/>`;
      s += `<line x1="${x(g)}" x2="${x(g)}" y1="${y(0)}" y2="${y(10)}" class="${g === 0 ? 'axis' : 'grid'}"/>`;
      s += `<text x="${x(0) - 8}" y="${y(g) + 4}" class="t-tick" text-anchor="end">${g}</text>`;
      s += `<text x="${x(g)}" y="${y(0) + 18}" class="t-tick" text-anchor="middle">${g}</text>`;
    }
    s += `<text x="${x(10)}" y="${y(0) + 40}" class="t-axis" text-anchor="end">Cost to make →</text>`;
    s += `<text x="${x(0)}" y="${T - 8}" class="t-axis">↑ Originality</text>`;
    s += `<text x="${x(0.3)}" y="${y(9.6)}" class="t-quad">Fresh, affordable</text>`;
    s += `<text x="${x(9.7)}" y="${y(0.5)}" class="t-quad" text-anchor="end">Familiar, costly</text>`;
    s += `<text x="${x(0.3)}" y="${y(0.5)}" class="t-quad">Safe, cheap</text>`;
    s += `<text x="${x(9.7)}" y="${y(9.6) + 0}" class="t-quad" text-anchor="end">Bold, costly</text>`;
    for (const r of rows) {
      const cx = x(r.cost), cy = y(r.orig);
      const cls = r.pick ? 'pt pick' : r.second ? 'pt second' : 'pt';
      const rad = r.pick || r.second ? 8 : 6;
      const lx = cx + (r.lab[0] || 12), ly = cy + (r.lab[1] || 4);
      s += `<g class="${cls}" tabindex="0" data-tip-v="${esc(r.title)}" data-tip="Cost ${fmt(r.cost)}/10 · originality ${fmt(r.orig)}/10 · endgame ${fmt(r.endgame)}/10 · fits what's built ${fmt(r.fit)}/10" aria-label="${esc(r.title)}: cost ${r.cost}, originality ${r.orig}">`;
      s += `<circle cx="${cx}" cy="${cy}" r="16" class="hit"/><circle cx="${cx}" cy="${cy}" r="${rad}" class="mark"/>`;
      s += `<text x="${lx}" y="${ly}" class="t-label" text-anchor="${r.lab[2] || 'start'}">${esc(r.title)}</text></g>`;
    }
    return s + '</svg>';
  },

  // The Settling Book: how far each island sank, year by year (story data). Crosshair reads all three at once.
  settling() {
    const years = [];
    for (let yr = -30; yr <= 0; yr++) years.push(yr);
    const sky = years.map((yr) => -0.05 * (yr + 30) + (yr % 3 === 0 ? -0.04 : 0));
    const tallow = years.map((yr) => (yr < -4 ? -0.06 * (yr + 30) : [-1.6, -3.1, -5.2, -7.8, -12][yr + 4]));
    const hollow = years.map((yr) => (yr < -18 ? -0.05 * (yr + 30) : -0.6 - (39.4 * (yr + 18)) / 18));
    const series = [
      { name: 'Hollowmount', cls: 's3', v: hollow },
      { name: 'Tallow Flats', cls: 's2', v: tallow },
      { name: 'Skyreach', cls: 's1', v: sky },
    ];
    const W = 800, H = 410, L = 56, Rt = 170, T = 60, B = 46;
    const x = (yr) => L + ((yr + 30) / 30) * (W - L - Rt);
    const y = (m) => T + (-m / 45) * (H - T - B);
    let s = `<svg class="chart" viewBox="0 0 ${W} ${H}" role="img" aria-label="Island heights over thirty years: Skyreach sank 1.5 metres, Tallow Flats 12 metres once the Company began digging there four years ago, Hollowmount 40 metres after eighteen years of mining." data-crosshair='${JSON.stringify({ x: years.map((yr) => +x(yr).toFixed(1)), labels: years.map((yr) => (yr === 0 ? 'This spring' : `${-yr} years ago`)), series: series.map((se) => ({ name: se.name, cls: se.cls, v: se.v.map((v) => +v.toFixed(1)), y: se.v.map((v) => +y(v).toFixed(1)) })), top: T, bottom: H - B })}'>`;
    for (const m of [0, -10, -20, -30, -40]) {
      s += `<line x1="${L}" x2="${W - Rt}" y1="${y(m)}" y2="${y(m)}" class="${m === 0 ? 'axis' : 'grid'}"/>`;
      s += `<text x="${L - 8}" y="${y(m) + 4}" class="t-tick" text-anchor="end">${m === 0 ? '0 m' : fmt(m) + ' m'}</text>`;
    }
    for (const yr of [-30, -20, -10, 0]) {
      s += `<text x="${x(yr)}" y="${H - B + 20}" class="t-tick" text-anchor="middle">${yr === 0 ? 'now' : -yr + ' yrs ago'}</text>`;
    }
    // Where the digs began.
    for (const [yr, m, label] of [[-18, -0.6, 'Hollowmount dig'], [-4, -1.6, 'Tallow Flats dig']]) {
      s += `<line x1="${x(yr)}" x2="${x(yr)}" y1="${y(m) - 4}" y2="${y(m) - 34}" class="axis"/>`;
      s += `<text x="${x(yr)}" y="${y(m) - 40}" class="t-note" text-anchor="middle">${label}</text>`;
    }
    for (const se of series) {
      const pts = se.v.map((v, i) => `${i ? 'L' : 'M'}${x(years[i]).toFixed(1)},${y(v).toFixed(1)}`).join(' ');
      s += `<path d="${pts}" class="sline ${se.cls}"/>`;
      const last = se.v[se.v.length - 1];
      s += `<circle cx="${x(0)}" cy="${y(last)}" r="4.5" class="sdot ${se.cls}"/>`;
      s += `<text x="${x(0) + 10}" y="${y(last) + 4}" class="t-label">${se.name} ${fmt(last)} m</text>`;
    }
    s += `<line class="xhair" x1="0" x2="0" y1="${T}" y2="${H - B}" visibility="hidden"/>`;
    s += `<rect class="xhair-hit" x="${L}" y="${T}" width="${W - L - Rt}" height="${H - T - B}"/>`;
    return s + '</svg>';
  },

  // Titans to scale on a log axis: each step is ten times longer.
  titans() {
    const items = [
      ['A person', 1.8], ['Biscuit, a calf', 4], ['A grazer titan', 150], ['Old Hollow (Skyreach)', 320],
      ['Marrowdeep', 1200], ['Bellwether', 6000], ['The Sovereign', 40000],
    ];
    const W = 760, L = 190, Rt = 70, T = 30, rowH = 34;
    const H = T + items.length * rowH + 40;
    const x = (m) => L + (Math.log10(m) / 5) * (W - L - Rt);
    const len = (m) => (m >= 1000 ? fmt(m / 1000) + ' km' : fmt(m, 0) + ' m');
    let s = `<svg class="chart" viewBox="0 0 ${W} ${H}" role="img" aria-label="Titans to scale on a log axis: a person 1.8 m, a calf 4 m, a grazer 150 m, Old Hollow 320 m, Marrowdeep 1.2 km, Bellwether 6 km, the Sovereign 40 km.">`;
    for (const [m, lab] of [[1, '1 m'], [10, '10 m'], [100, '100 m'], [1000, '1 km'], [10000, '10 km'], [100000, '100 km']]) {
      s += `<line x1="${x(m)}" x2="${x(m)}" y1="${T - 6}" y2="${T + items.length * rowH}" class="grid"/>`;
      s += `<text x="${x(m)}" y="${T + items.length * rowH + 20}" class="t-tick" text-anchor="middle">${lab}</text>`;
    }
    items.forEach(([name, m], i) => {
      const cy = T + i * rowH + rowH / 2;
      const hl = name.startsWith('Old Hollow');
      s += `<text x="${L - 14}" y="${cy + 4}" class="t-label" text-anchor="end">${esc(name)}</text>`;
      s += `<g class="pt${hl ? ' pick' : ''}" tabindex="0" data-tip-v="${len(m)}" data-tip="${esc(name)}"><circle cx="${x(m)}" cy="${cy}" r="14" class="hit"/><circle cx="${x(m)}" cy="${cy}" r="6" class="mark"/></g>`;
      s += `<text x="${x(m) + 12}" y="${cy + 4}" class="t-value">${len(m)}</text>`;
    });
    return s + '</svg>';
  },

  // Hettie Bellweather's life: the Masterworks above the line, the rest of it below.
  hettie() {
    const ev = [
      [12, 'm', 'First Love', 'Her first rifle, at twelve'],
      [18, 'm', 'Blue Ribbon', 'County Fair champion, six years running'],
      [24, 'm', 'Honeymoon, Till Death', 'Marries husband number one'],
      [29, 'm', 'Keep Your Head Down', 'The war; she builds Ol\' Bessie'],
      [33, 'l', 'The Bellweather Standard', 'Interchangeable parts: every gun since is built from them'],
      [38, 'l', 'Husband two: Gus', 'Colonel Gus Lindqvist'],
      [43, 'm', 'Alimony', 'Partner, then husband three: Vandersloot, for eleven months'],
      [45, 'm', 'The Rival', 'Built to beat Vandersloot at the Expo'],
      [48, 'm', 'Patent Pending', 'The patent wars'],
      [53, 'l', 'The Last Word', 'Built in a fit of guilt, then hidden'],
      [58, 'l', 'Husband four: the poet', 'His love letters are awful'],
      [66, 'l', 'Husband five', 'The quiet one'],
      [74, 'm', "Grandma's Cupboard", 'Plum, apricot and something experimental'],
      [83, 'm', 'Rainmaker', 'Her only weather gun; later stolen by the Holy Recoil'],
      [91, 'm', 'Forty-One', 'The forty-first explosion'],
      [100, 'm', 'Encore', 'Hidden in plain sight at the Expo'],
      [103, 'l', 'Prototype 42', 'The barn blows up. The funeral'],
    ];
    const W = 1000, L = 24, Rt = 24;
    const x = (age) => L + (age / 105) * (W - L - Rt);
    const tierH = 26;
    const place = (kind) => {
      const tiers = [];
      return ev.filter((e) => e[1] === kind).map((e) => {
        const w = textW(e[2], 12);
        const flip = x(e[0]) + w > W - Rt;
        const x0 = flip ? x(e[0]) - w - 4 : x(e[0]) - 6, x1 = x0 + w + 10;
        let t = tiers.findIndex((end) => end <= x0);
        if (t < 0) { t = tiers.length; tiers.push(0); }
        tiers[t] = x1;
        return { e, t, flip };
      });
    };
    const up = place('m'), down = place('l');
    const upT = Math.max(...up.map((p) => p.t)) + 1, downT = Math.max(...down.map((p) => p.t)) + 1;
    const midY = 30 + upT * tierH + 10;
    const H = midY + 40 + downT * tierH + 6;
    let s = `<svg class="chart" viewBox="0 0 ${W} ${H}" role="img" aria-label="Hettie Bellweather's life from birth to 103: twelve Masterworks above the line, marriages, the Bellweather Standard and the Last Word below.">`;
    s += `<line x1="${L}" x2="${W - Rt}" y1="${midY}" y2="${midY}" class="axis"/>`;
    for (let age = 0; age <= 100; age += 10) {
      s += `<line x1="${x(age)}" x2="${x(age)}" y1="${midY - 4}" y2="${midY + 4}" class="axis"/>`;
      s += `<text x="${x(age)}" y="${midY + 18}" class="t-tick" text-anchor="middle">${age}</text>`;
    }
    const draw = (arr, dir) => arr.forEach(({ e, t, flip }) => {
      const ex = x(e[0]);
      const ly = dir < 0 ? midY - 28 - t * tierH : midY + 40 + t * tierH;
      s += `<g class="pt${e[1] === 'm' ? ' pick' : ''}" tabindex="0" data-tip-v="Age ${e[0]} · ${esc(e[2])}" data-tip="${esc(e[3])}">`;
      s += `<line x1="${ex}" x2="${ex}" y1="${midY}" y2="${dir < 0 ? ly + 4 : ly - 12}" class="grid"/>`;
      s += `<circle cx="${ex}" cy="${midY}" r="14" class="hit"/><circle cx="${ex}" cy="${midY}" r="5.5" class="mark"/>`;
      s += `<text x="${flip ? ex + 4 : ex - 4}" y="${ly}" class="t-label" text-anchor="${flip ? 'end' : 'start'}">${esc(e[2])}</text></g>`;
    });
    draw(up, -1);
    draw(down, 1);
    s += `<text x="${W - Rt}" y="${midY - 8}" class="t-tick" text-anchor="end">Age</text>`;
    return s + '</svg>';
  },
};

// Crosshair for line charts: a hairline snaps to the nearest year and the tooltip lists every series there.
function wireCrosshair(root) {
  root.querySelectorAll('svg[data-crosshair]').forEach((svg) => {
    const d = JSON.parse(svg.getAttribute('data-crosshair'));
    const line = svg.querySelector('.xhair');
    const hit = svg.querySelector('.xhair-hit');
    const move = (e) => {
      const pt = svg.createSVGPoint();
      pt.x = e.clientX;
      pt.y = e.clientY;
      const p = pt.matrixTransform(svg.getScreenCTM().inverse());
      let best = 0;
      d.x.forEach((xx, i) => { if (Math.abs(xx - p.x) < Math.abs(d.x[best] - p.x)) best = i; });
      line.setAttribute('x1', d.x[best]);
      line.setAttribute('x2', d.x[best]);
      line.setAttribute('visibility', 'visible');
      const vals = d.series.map((se) => `${se.name} ${String(se.v[best]).replace('-', '−')} m`).join(' · ');
      Tip.show(vals, d.labels[best], e.clientX, e.clientY);
    };
    hit.addEventListener('pointermove', move);
    hit.addEventListener('pointerenter', move);
    hit.addEventListener('pointerleave', () => { line.setAttribute('visibility', 'hidden'); Tip.hide(); });
  });
}
