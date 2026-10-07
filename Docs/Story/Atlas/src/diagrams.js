// Diagrams: each draws the mechanism a story turns on (what holds an island up, where the dead go, why titans
// roll...). Plain SVG; colors come from the page's tokens through classes, so both themes read.
const Diagrams = (() => {
  let uid = 0;
  const defs = (p) => `<defs>
    <marker id="${p}-a" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" class="ah"/></marker>
    <marker id="${p}-o" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" class="ah-accent"/></marker>
    <marker id="${p}-w" viewBox="0 0 10 10" refX="9" refY="5" markerWidth="7" markerHeight="7" orient="auto-start-reverse"><path d="M0,0 L10,5 L0,10 z" class="ah-warn"/></marker>
    <radialGradient id="${p}-g"><stop offset="0" class="glow-stop" stop-opacity="0.6"/><stop offset="1" class="glow-stop" stop-opacity="0"/></radialGradient>
  </defs>`;
  const mk = { '': 'a', accent: 'o', warn: 'w' };
  // A straight or curved arrow with an optional label beside its middle.
  function arrow(p, pts, kind, label, lx, ly, anchor) {
    const cls = kind ? `d-arrow ${kind}` : 'd-arrow';
    let d;
    if (pts.length === 2) d = `M${pts[0][0]},${pts[0][1]} L${pts[1][0]},${pts[1][1]}`;
    else if (pts.length === 3) d = `M${pts[0][0]},${pts[0][1]} Q${pts[1][0]},${pts[1][1]} ${pts[2][0]},${pts[2][1]}`;
    else d = 'M' + pts.map((q) => q.join(',')).join(' L');
    let s = `<path d="${d}" class="${cls}" marker-end="url(#${p}-${mk[kind || '']})"/>`;
    if (label) s += `<text x="${lx}" y="${ly}" class="d-note" text-anchor="${anchor || 'middle'}">${esc(label)}</text>`;
    return s;
  }
  // A box with one or two centered lines.
  function box(x, y, w, h, lines, kind) {
    const cls = kind ? `d-box ${kind}` : 'd-box';
    let s = `<rect x="${x}" y="${y}" width="${w}" height="${h}" rx="6" class="${cls}"/>`;
    const ls = Array.isArray(lines) ? lines : [lines];
    const cy = y + h / 2 - (ls.length - 1) * 8 + 4;
    ls.forEach((t, i) => {
      s += `<text x="${x + w / 2}" y="${cy + i * 16}" class="${i === 0 ? 'd-text' : 'd-sub'}" text-anchor="middle">${esc(t)}</text>`;
    });
    return s;
  }
  const open = (W, H, label) => {
    const p = `dg${++uid}`;
    return [p, `<svg class="diagram" viewBox="0 0 ${W} ${H}" role="img" aria-label="${esc(label)}">${defs(p)}`];
  };

  return {
    // The Lodestone War: water feeds the heart, the heart lifts the island, the bore cuts the roots.
    lodestone() {
      const [p, head] = open(760, 480, 'Cross-section of a Rim island: the windmill pumps rainwater down the Keeper well to the lodestone heart, whose roots lift the island; the Company bore cuts a root, and an island with cut roots sinks.');
      let s = head;
      s += `<path d="M70,130 Q380,102 690,130 L652,176 L604,246 L528,326 L436,398 L380,414 L326,396 L238,318 L160,240 L102,174 Z" class="d-rock"/>`;
      s += `<path d="M70,130 Q380,102 690,130 L686,142 Q380,116 74,142 Z" class="d-grass"/>`;
      // Roots and the heart.
      for (const d of ['M380,306 C330,272 262,238 196,176', 'M380,306 C432,266 520,232 604,176', 'M380,306 C368,256 352,206 322,150', 'M380,306 C398,252 430,206 478,150', 'M380,306 C350,330 300,330 268,300', 'M380,306 C412,334 462,332 500,300']) {
        s += `<path d="${d}" class="d-root"/>`;
      }
      s += `<circle cx="380" cy="306" r="70" fill="url(#${p}-g)"/>`;
      s += `<path d="M380,276 L398,306 L380,336 L362,306 Z" class="d-crystal"/><path d="M356,292 L366,306 L356,322 L346,306 Z" class="d-crystal"/><path d="M404,292 L414,306 L404,320 L394,306 Z" class="d-crystal"/>`;
      s += `<text x="380" y="360" class="d-text" text-anchor="middle">Lodestone heart</text>`;
      s += `<text x="380" y="376" class="d-sub" text-anchor="middle">falls upward; lifts the island</text>`;
      // The windmill, the pump and the Keeper well.
      s += `<path d="M196,118 L202,52 L208,118" class="d-ink"/>`;
      for (let i = 0; i < 6; i++) {
        const a = (i / 6) * Math.PI * 2 + 0.3;
        s += `<line x1="202" y1="52" x2="${(202 + Math.cos(a) * 26).toFixed(1)}" y2="${(52 + Math.sin(a) * 26).toFixed(1)}" class="d-ink"/>`;
      }
      s += `<text x="202" y="24" class="d-text" text-anchor="middle">Windmill pump</text>`;
      s += `<rect x="244" y="110" width="22" height="12" rx="2" class="d-stone"/>`;
      s += `<path d="M255,124 C262,180 300,240 356,290" class="d-water"/>`;
      s += arrow(p, [[214, 116], [242, 116]], 'accent');
      s += arrow(p, [[270, 150], [300, 214], [334, 262]], 'accent', 'rainwater down the Keeper well', 150, 222, 'middle');
      // The Company's bore.
      s += `<path d="M548,118 L562,62 L576,118 M552,100 L572,100 M556,82 L568,82" class="d-ink"/>`;
      s += `<text x="562" y="44" class="d-text" text-anchor="middle">Company bore</text>`;
      s += `<path d="M562,122 L494,212" class="d-bore"/>`;
      s += `<path d="M482,200 L502,220 M502,200 L482,220" class="d-cut"/>`;
      s += `<text x="512" y="232" class="d-note warn">cuts a root</text>`;
      s += `<path d="M566,250 L576,262 L566,274 L556,262 Z" class="d-dead"/>`;
      s += `<text x="584" y="266" class="d-note">cut stone runs hot</text>`;
      // The Shroud below, and the sinking.
      s += `<path d="M0,450 Q60,436 120,450 T240,450 T360,450 T480,450 T600,450 T720,450 L760,450 L760,480 L0,480 Z" class="d-cloud"/>`;
      s += arrow(p, [[470, 380], [470, 436]], 'warn', 'roots cut: the island sinks', 482, 412, 'start');
      return s + '</svg>';
    },

    // Revenant: where the dead go, with the saint's light and without it.
    revenant() {
      const [p, head] = open(820, 300, 'Two lanes. With the saint\'s light, a soul rides the Gravewind to the Far Shore. With the ember stolen, the soul lingers as one of the Unpaid, and only Sexton\'s Toll Gate is left.');
      let s = head;
      const xs = [20, 225, 430, 635], w = 165, h = 54;
      s += `<text x="20" y="28" class="d-label">With the saint's light</text>`;
      s += `<text x="20" y="168" class="d-label">With the ember stolen</text>`;
      const top = [['A soul', 'dies on the island'], ["Saint's ember", 'lights the way'], ['The Gravewind', 'blows at dusk'], ['The Far Shore', 'rest']];
      const bot = [['A soul', 'dies on the island'], ['No light', 'the Hollow took it'], ['Lingers', 'as one of the Unpaid'], ["Sexton's Toll Gate", 'pay him, or serve him']];
      top.forEach((t, i) => { s += box(xs[i], 44, w, h, t, i === 1 ? 'accent' : ''); });
      bot.forEach((t, i) => { s += box(xs[i], 184, w, h, t, i >= 1 ? 'warn' : ''); });
      for (let i = 0; i < 3; i++) {
        s += arrow(p, [[xs[i] + w + 4, 71], [xs[i + 1] - 6, 71]], i === 0 ? 'accent' : '');
        s += arrow(p, [[xs[i] + w + 4, 211], [xs[i + 1] - 6, 211]], 'warn');
      }
      s += `<circle cx="${xs[1] + 18}" cy="62" r="5" class="d-ember"/>`;
      s += `<path d="M${xs[1] + 12},${190} L${xs[1] + 24},${202} M${xs[1] + 24},${190} L${xs[1] + 12},${202}" class="d-cut"/>`;
      s += `<text x="410" y="280" class="d-note" text-anchor="middle">Ellis returns the embers one by one. Each relit saint lets an island's dead cross again.</text>`;
      return s + '</svg>';
    },

    // Skyfarers: the voyage on Jory's and Margo's halves of the map.
    halfmap() {
      const [p, head] = open(820, 470, 'The voyage in order: Skyreach, Port Gallant, the Shoals, Gallowglass Fort, the Brass Cemetery, the Tempest Wall, the Penance, and back to Skyreach, where the X is.');
      let s = head;
      // Two halves of parchment with a ragged tear between them.
      const tear = [[404, 0], [414, 40], [398, 86], [416, 130], [400, 180], [418, 226], [402, 270], [420, 318], [404, 362], [416, 410], [402, 470]];
      s += `<path d="M0,0 L${tear.map((q) => q.join(',')).join(' L')} L0,470 Z" class="d-paper"/>`;
      s += `<path d="M${tear.map((q) => [q[0] + 8, q[1]].join(',')).join(' L')} L820,470 L820,0 Z" class="d-paper"/>`;
      s += `<text x="24" y="32" class="m-title">Jory's half</text><text x="796" y="32" class="m-title" text-anchor="end">Margo's half</text>`;
      // Islands.
      const isl = (x, y, rx, ry) => `<ellipse cx="${x}" cy="${y}" rx="${rx}" ry="${ry}" class="m-isle"/><ellipse cx="${x}" cy="${y + ry * 0.55}" rx="${rx * 0.6}" ry="${ry * 0.5}" class="m-under"/>`;
      const pts = {
        sky: [140, 392], gallant: [262, 262], shoals: [150, 132], fort: [360, 96], cemetery: [560, 132], storm: [680, 300], penance: [530, 392],
      };
      pts.fort = [330, 96];
      s += isl(...pts.sky, 34, 14) + isl(...pts.gallant, 30, 12) + isl(pts.gallant[0] + 40, pts.gallant[1] + 14, 16, 7) + isl(pts.gallant[0] - 38, pts.gallant[1] + 10, 14, 6);
      for (let i = 0; i < 7; i++) s += isl(pts.shoals[0] - 40 + (i % 4) * 26, pts.shoals[1] - 12 + Math.floor(i / 4) * 24, 9, 4);
      s += isl(...pts.fort, 28, 12) + isl(...pts.cemetery, 40, 15);
      s += `<rect x="${pts.penance[0] - 26}" y="${pts.penance[1] - 8}" width="52" height="14" rx="3" class="m-isle"/>`;
      // The Tempest Wall: a ring of storm.
      for (let r = 62; r <= 96; r += 8) s += `<circle cx="${pts.storm[0]}" cy="${pts.storm[1]}" r="${r}" class="m-storm"/>`;
      // The route.
      const order = ['sky', 'gallant', 'shoals', 'fort', 'cemetery', 'storm', 'penance', 'sky'];
      let d = '';
      order.forEach((k, i) => { d += (i ? ' L' : 'M') + pts[k][0] + ',' + pts[k][1]; });
      s += `<path d="${d}" class="m-route"/>`;
      // The X.
      s += `<path d="M${pts.sky[0] - 12},${pts.sky[1] - 12} L${pts.sky[0] + 12},${pts.sky[1] + 12} M${pts.sky[0] + 12},${pts.sky[1] - 12} L${pts.sky[0] - 12},${pts.sky[1] + 12}" class="m-x"/>`;
      const lab = (k, name, lv, dx, dy, anchor) => `<text x="${pts[k][0] + dx}" y="${pts[k][1] + dy}" class="m-name" text-anchor="${anchor || 'middle'}">${esc(name)}</text><text x="${pts[k][0] + dx}" y="${pts[k][1] + dy + 15}" class="m-lv" text-anchor="${anchor || 'middle'}">${esc(lv)}</text>`;
      s += lab('sky', 'Skyreach', 'start, and the X', 0, 38);
      s += lab('gallant', 'Port Gallant', 'levels 5–12', 0, 38);
      s += lab('shoals', 'The Shoals', 'levels 10–18', 0, 44);
      s += lab('fort', 'Gallowglass Fort', 'levels 16–24', 0, -26);
      s += lab('cemetery', 'Brass Cemetery', 'levels 22–30', 0, -28);
      s += lab('storm', 'Tempest Wall', 'levels 28–36', 0, 4);
      s += lab('penance', 'The Penance', 'levels 34–40', 0, 30);
      // Compass rose.
      s += `<g transform="translate(760,420)"><path d="M0,-26 L6,0 L0,26 L-6,0 Z" class="m-ink"/><path d="M-26,0 L0,-5 L26,0 L0,5 Z" class="m-ink"/><text x="0" y="-32" class="m-lv" text-anchor="middle">N</text></g>`;
      return s + '</svg>';
    },

    // Leviathan: the loop the Spear is caught in, and the way out of it.
    leviathan() {
      const [p, head] = open(860, 420, 'A waking titan either panics and rolls, which drops its town, which recruits the Spear, whose kills scream and panic more titans; or a Listener calms it and it carries its people to the Calving Grounds.');
      let s = head;
      const B = { w: 176, h: 50 };
      const n = {
        wake: [342, 16], roll: [150, 110], fall: [20, 214], swear: [150, 330], kill: [400, 290], scream: [400, 180],
        calm: [640, 110], ride: [640, 260],
      };
      s += box(...n.wake, B.w, B.h, ['A titan wakes', 'for the Migration']);
      s += box(...n.roll, B.w, B.h, ['Frightened:', 'it rolls'], 'warn');
      s += box(...n.fall, B.w, B.h, ['Its town falls', 'off its back'], 'warn');
      s += box(...n.swear, B.w, B.h, ['The Spear swears', 'to kill them all'], 'warn');
      s += box(...n.kill, B.w, B.h, ['The Spear', 'kills a titan'], 'warn');
      s += box(...n.scream, B.w, B.h, ['Its death-scream', 'frightens the rest'], 'warn');
      s += box(...n.calm, B.w, B.h, ['Calmed by a Listener:', 'its back stays level'], 'accent');
      s += box(...n.ride, B.w, B.h, ['Its people ride it', 'to the Calving Grounds'], 'accent');
      const c = (k, dx, dy) => [n[k][0] + B.w / 2 + (dx || 0), n[k][1] + B.h / 2 + (dy || 0)];
      s += arrow(p, [c('wake', -60, 25), c('roll', 30, -25)], 'warn', 'afraid', 250, 92, 'end');
      s += arrow(p, [c('wake', 60, 25), c('calm', -30, -25)], 'accent', 'a Listener sings', 600, 92, 'start');
      s += arrow(p, [c('roll', -60, 25), c('fall', 20, -25)], 'warn');
      s += arrow(p, [c('fall', 20, 25), c('swear', -60, -25)], 'warn');
      s += arrow(p, [c('swear', 88, 0), c('kill', -88, 12)], 'warn');
      s += arrow(p, [c('kill', 0, -25), c('scream', 0, 25)], 'warn');
      s += arrow(p, [c('scream', -88, -6), c('roll', 88, 10)], 'warn', 'the herd hears it', 344, 186, 'middle');
      s += arrow(p, [c('calm', 0, 25), c('ride', 0, -25)], 'accent');
      s += `<text x="728" y="350" class="d-note" text-anchor="middle">The way out of the loop</text>`;
      s += `<text x="300" y="404" class="d-note" text-anchor="middle">The Spear's loop: its own kills make the wakings violent</text>`;
      return s + '</svg>';
    },

    // Warden: what is under the farm.
    warden() {
      const [p, head] = open(820, 470, 'Cutaway of Skyreach in Warden: the farm on top, a soil layer, the Undercroft corridors with a Cradle, the hatch under the grove, the lift engine with its orange lattice-core, and a conduit up to the Reliquary on the plateau, which fires a beacon.');
      let s = head;
      // Ground: meadow, then the plateau on the right.
      s += `<path d="M30,128 L590,128 L604,86 L790,86 L770,150 L700,250 L560,360 L430,440 L300,380 L160,270 L60,170 Z" class="d-rock"/>`;
      s += `<path d="M30,128 L590,128 L604,86 L790,86 L786,100 L612,100 L598,142 L34,142 Z" class="d-grass"/>`;
      s += `<path d="M34,142 L598,142 L594,160 L44,160 Z" class="d-soil"/>`;
      s += `<text x="40" y="176" class="d-sub">soil, a few meters</text>`;
      // Surface landmarks.
      s += `<path d="M86,128 L86,112 L98,102 L110,112 L110,128" class="d-ink"/><text x="98" y="94" class="d-sub" text-anchor="middle">farm</text>`;
      s += `<path d="M150,128 L150,108 L182,108 L182,128 M146,108 L166,96 L186,108" class="d-ink"/><text x="166" y="88" class="d-sub" text-anchor="middle">barn</text>`;
      s += `<path d="M286,128 L286,114 M296,128 L296,114 M282,116 L300,116" class="d-ink"/><text x="291" y="104" class="d-sub" text-anchor="middle">gun rack</text>`;
      s += `<path d="M386,128 L386,112 M400,128 L400,110 M414,128 L414,113" class="d-ink"/><text x="400" y="100" class="d-sub" text-anchor="middle">range</text>`;
      for (const tx of [470, 494, 518]) s += `<circle cx="${tx}" cy="114" r="10" class="d-tree"/><line x1="${tx}" y1="122" x2="${tx}" y2="128" class="d-ink"/>`;
      s += `<text x="494" y="96" class="d-sub" text-anchor="middle">grove</text>`;
      s += `<path d="M736,86 L742,30 L754,30 L760,86 M738,58 L758,58" class="d-ink"/><text x="748" y="20" class="d-sub" text-anchor="middle">lookout</text>`;
      // The Reliquary and its beacon.
      s += `<rect x="648" y="74" width="24" height="12" rx="2" class="d-stone"/><circle cx="660" cy="64" r="5" class="d-ember"/>`;
      s += `<line x1="660" y1="56" x2="660" y2="8" class="d-beam"/>`;
      s += `<text x="672" y="24" class="d-note" text-anchor="start">beacon</text>`;
      // The Undercroft.
      s += `<rect x="80" y="196" width="600" height="40" rx="4" class="d-hollow"/>`;
      s += `<text x="96" y="221" class="d-text">The Undercroft</text>`;
      s += `<rect x="236" y="203" width="44" height="26" rx="12" class="d-cradle"/><text x="258" y="252" class="d-sub" text-anchor="middle">Cradle: Ward is rebuilt here</text>`;
      // The hatch under the grove, and a ladder down.
      s += `<rect x="484" y="139" width="20" height="5" class="d-ember-rect"/>`;
      s += `<path d="M488,144 L488,196 M500,144 L500,196 M488,156 L500,156 M488,168 L500,168 M488,180 L500,180" class="d-ink"/>`;
      s += `<text x="512" y="176" class="d-note" text-anchor="start">hatch under the grove</text>`;
      // Tenders.
      for (const tx of [600, 630]) s += `<circle cx="${tx}" cy="222" r="5" class="d-tender"/><path d="M${tx - 9},230 L${tx - 4},224 M${tx + 9},230 L${tx + 4},224 M${tx - 9},214 L${tx - 4},220 M${tx + 9},214 L${tx + 4},220" class="d-ink"/>`;
      s += `<text x="615" y="252" class="d-sub" text-anchor="middle">Tenders</text>`;
      // The lift engine and its lattice-core.
      s += `<rect x="300" y="282" width="230" height="86" rx="6" class="d-engine"/>`;
      s += `<circle cx="415" cy="325" r="60" fill="url(#${p}-g)"/>`;
      s += `<path d="M415,301 L433,325 L415,349 L397,325 Z" class="d-crystal"/>`;
      s += `<text x="415" y="390" class="d-text" text-anchor="middle">Lift engine and lattice-core</text>`;
      // Conduit up to the Reliquary.
      s += `<path d="M530,320 L660,320 L660,92" class="d-conduit"/>`;
      s += arrow(p, [[660, 150], [660, 94]], 'accent');
      s += `<text x="670" y="300" class="d-note" text-anchor="start">power</text>`;
      s += `<path d="M0,452 Q60,440 120,452 T240,452 T360,452 T480,452 T600,452 T720,452 L820,452 L820,470 L0,470 Z" class="d-cloud"/>`;
      return s + '</svg>';
    },

    // Below: a dive from top to bottom. Depth sets the Hush and the salvage.
    below() {
      const [p, head] = open(600, 780, 'A dive from the islands to the Throat, not to scale. Each layer is deeper and darker; the Hush grows with depth, and so does the rarity of what you find. The lifeline runs from the gantry at the top.');
      let s = head;
      const layers = [
        ['The islands', 'Skyreach, Lanternhold', '1–12', 20, 92, '#cde2fb', '#0b1e2e'],
        ['The Undersides', 'roots and hanging halls', '8–16', 92, 172, '#9ec5f4', '#0b1e2e'],
        ['The Fallfield', 'cloudcrust, Lowtown', '14–24', 172, 272, '#6da7ec', '#0b1e2e'],
        ['The Hushwood', 'Bloom forest, no sound', '22–30', 272, 382, '#3987e5', '#ffffff'],
        ["Saint Ebba's", 'the Sounding\'s sanctum', '28–35', 382, 482, '#256abf', '#ffffff'],
        ['The Floor', 'the old world', '34–44', 482, 622, '#184f95', '#ffffff'],
        ['The Throat', 'into the Fathom', '44–50', 622, 742, '#0d366b', '#ffffff'],
      ];
      const salvage = ['common', 'common', 'uncommon', 'rare', 'epic', 'legendary', 'legendary'];
      layers.forEach(([name, sub, lv, y0, y1, fill, ink], i) => {
        s += `<rect x="60" y="${y0}" width="300" height="${y1 - y0 - 3}" rx="${i === 0 ? 6 : 2}" fill="${fill}"/>`;
        s += `<text x="80" y="${y0 + 26}" class="d-band-name" fill="${ink}">${esc(name)}</text>`;
        s += `<text x="80" y="${y0 + 44}" class="d-band-sub" fill="${ink}">${esc(sub)} · levels ${lv}</text>`;
        s += `<rect x="470" y="${y0 + 14}" width="18" height="18" rx="4" class="r-${salvage[i]}"/>`;
        s += `<text x="496" y="${y0 + 28}" class="d-sub">${salvage[i]}</text>`;
      });
      // The Bloom's glow at the bottom.
      s += `<ellipse cx="210" cy="760" rx="220" ry="46" fill="url(#${p}-g)"/>`;
      // The Hush: one hue, stronger with depth.
      s += `<defs><linearGradient id="${p}-h" x1="0" y1="0" x2="0" y2="1"><stop offset="0" class="hush-stop" stop-opacity="0.05"/><stop offset="1" class="hush-stop" stop-opacity="0.95"/></linearGradient></defs>`;
      s += `<rect x="392" y="20" width="22" height="722" rx="4" fill="url(#${p}-h)"/>`;
      s += `<text x="403" y="12" class="d-label" text-anchor="middle">Hush</text>`;
      s += `<text x="479" y="12" class="d-label" text-anchor="middle">Salvage</text>`;
      // The lifeline and the diver.
      s += `<path d="M30,6 L30,20 M18,6 L44,6" class="d-ink"/>`;
      s += `<path d="M30,20 L30,250" class="d-line-life"/>`;
      s += `<circle cx="30" cy="258" r="8" class="d-diver"/><circle cx="30" cy="258" r="13" fill="url(#${p}-g)"/>`;
      s += `<text x="30" y="290" class="d-note" text-anchor="middle">you</text>`;
      s += `<path d="M30,300 L30,600" class="d-line-faint"/>`;
      s += `<path d="M22,604 L38,604 M30,596 L30,612" class="d-ink"/>`;
      s += `<text x="80" y="${482 + 64}" class="d-band-sub" fill="#ffffff">Imogen's record dive ended here</text>`;
      return s + '</svg>';
    },

    // The Long Dusk: the Stair's tiers, the Lamp at the top, the Dusk climbing from below.
    stair() {
      const [p, head] = open(860, 600, 'The Stair from the Gloaming to the Crown. The Lamp burns at the top; Corvina draws its fire into the Crown and the High tiers; the Dusk climbs from below and reaches Skyreach at the end of Act II. In the ending, every island gets its own lamp.');
      let s = head;
      const tiers = [
        ['The Gloaming', 'blue twilight, frost', '3–12', 60, 520, '#4f6aa3', '#ffffff'],
        ['Skyreach', 'warm afternoon', '1–5', 320, 452, '#e3b25e', '#1d1405'],
        ['The Noon Steppes', 'hard white noon', '12–20', 580, 384, '#f2ead6', '#1d1405'],
        ['The Windward Spires', 'thin, clear, windy', '18–28', 320, 316, '#cfe3ea', '#0f1d24'],
        ['The Gilded Terraces', 'endless gold', '26–38', 60, 248, '#e9c25c', '#1d1405'],
        ['The Glare', 'blinding white heat', '36–44', 320, 180, '#fff8e8', '#1d1405'],
        ['The Crown', 'inside the light', '44–50', 580, 112, '#ffe9b0', '#1d1405'],
      ];
      // The Dusk, below Skyreach's level.
      s += `<defs><linearGradient id="${p}-d" x1="0" y1="1" x2="0" y2="0"><stop offset="0" class="dusk-stop" stop-opacity="0.75"/><stop offset="1" class="dusk-stop" stop-opacity="0"/></linearGradient></defs>`;
      s += `<rect x="0" y="476" width="860" height="124" fill="url(#${p}-d)"/>`;
      s += arrow(p, [[836, 590], [836, 488]], '', '', 0, 0);
      s += `<text x="826" y="560" class="d-note" text-anchor="end">the Dusk climbs</text>`;
      s += `<text x="826" y="576" class="d-note" text-anchor="end">one island at a time</text>`;
      // The climb.
      for (let i = 0; i < tiers.length - 1; i++) {
        const a = tiers[i], b = tiers[i + 1];
        const from = [a[3] + (b[3] > a[3] ? 220 : 0), a[4] + 10], to = [b[3] + (b[3] > a[3] ? 0 : 220), b[4] + 30];
        s += arrow(p, [from, to], '');
      }
      tiers.forEach(([name, light, lv, x, y, fill, ink], i) => {
        s += `<rect x="${x}" y="${y}" width="220" height="44" rx="8" fill="${fill}" class="d-tier${i === 1 ? ' here' : ''}"/>`;
        s += `<text x="${x + 12}" y="${y + 19}" class="d-band-name" fill="${ink}">${esc(name)}</text>`;
        s += `<text x="${x + 12}" y="${y + 35}" class="d-band-sub" fill="${ink}">${esc(light)} · levels ${lv}</text>`;
        s += `<circle cx="${x + 206}" cy="${y + 14}" r="4" class="d-ember"/>`;
      });
      s += `<text x="330" y="${452 + 62}" class="d-note" text-anchor="start">you start here</text>`;
      // The Lamp and its cage.
      s += `<circle cx="770" cy="56" r="46" fill="url(#${p}-g)"/><circle cx="770" cy="56" r="20" class="d-lamp"/>`;
      for (let i = -2; i <= 2; i++) s += `<line x1="${770 + i * 9}" y1="30" x2="${770 + i * 9}" y2="82" class="d-cage"/>`;
      s += `<text x="770" y="16" class="d-text" text-anchor="middle">The Lamp</text>`;
      s += arrow(p, [[744, 74], [804, 112]], 'accent', 'Corvina draws its fire', 690, 100, 'end');
      s += arrow(p, [[578, 132], [330, 112], [210, 244]], 'accent', 'light, hoarded for the High', 360, 104, 'middle');
      s += `<text x="40" y="44" class="d-label">Ending: a lamp on every island</text>`;
      s += `<circle cx="28" cy="40" r="4" class="d-ember"/>`;
      return s + '</svg>';
    },
  };
})();

// Small gun silhouettes for loot cards, by weapon type.
function gunIcon(type) {
  const parts = {
    rifle: 'M4,14 L20,12 L22,10 L52,10 L52,13 L64,13 L64,15 L52,15 L46,17 L44,23 L40,23 L41,17 L28,17 L26,22 L22,22 L22,17 L8,19 Z',
    shotgun: 'M4,15 L22,12 L24,10 L64,10 L64,13 L44,13 L44,16 L30,16 L28,20 L24,20 L24,16 L8,19 Z',
    pistol: 'M14,10 L46,10 L46,15 L28,15 L26,24 L19,24 L21,15 L14,15 Z',
    smg: 'M10,12 L48,11 L48,14 L58,14 L58,16 L40,16 L38,26 L34,26 L35,16 L26,16 L24,21 L20,21 L20,16 L10,16 Z',
    sniper: 'M2,15 L18,12 L20,10 L44,10 L44,12 L66,12 L66,14 L44,14 L40,16 L26,16 L24,21 L20,21 L20,16 L6,19 Z M26,6 L42,6 L42,9 L26,9 Z',
    launcher: 'M6,9 L58,9 L58,18 L6,18 Z M24,18 L30,18 L29,25 L24,25 Z M36,18 L40,18 L40,22 L36,22 Z',
    revolver: 'M12,10 L50,10 L50,13 L34,13 L33,17 L27,17 L25,25 L18,25 L21,16 L12,15 Z M26,12 A4,4 0 1,0 34,12 Z',
  };
  return `<svg class="gun" viewBox="0 0 68 28" aria-hidden="true"><path d="${parts[type] || parts.rifle}"/></svg>`;
}
function gunType(base) {
  const b = base.toLowerCase();
  if (b.includes('revolver')) return 'revolver';
  if (b.includes('pistol') || b.includes('sidearm')) return 'pistol';
  if (b.includes('sniper')) return 'sniper';
  if (b.includes('smg')) return 'smg';
  if (b.includes('launcher') || b.includes('harpoon')) return 'launcher';
  if (b.includes('shotgun') || b.includes('blunderbuss')) return 'shotgun';
  return 'rifle';
}
