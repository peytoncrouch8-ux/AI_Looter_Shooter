// Key art: one painted scene per story, drawn on canvas from simple seeded shapes so every banner and thumbnail
// renders at any size without image files.
const Art = (() => {
  const TAU = Math.PI * 2;

  function rng(seed) {
    let a = seed >>> 0;
    return () => {
      a = (a + 0x6D2B79F5) >>> 0;
      let t = a;
      t = Math.imul(t ^ (t >>> 15), t | 1);
      t ^= t + Math.imul(t ^ (t >>> 7), t | 61);
      return ((t ^ (t >>> 14)) >>> 0) / 4294967296;
    };
  }
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  function hex(c) {
    const n = parseInt(c.slice(1), 16);
    return [(n >> 16) & 255, (n >> 8) & 255, n & 255];
  }
  function mix(a, b, t) {
    const A = hex(a), B = hex(b);
    const r = A.map((v, i) => Math.round(v + (B[i] - v) * t));
    return '#' + r.map(v => v.toString(16).padStart(2, '0')).join('');
  }
  function rgba(c, a) {
    const [r, g, b] = hex(c);
    return `rgba(${r},${g},${b},${a})`;
  }

  // --- Sky, light and weather ---

  function sky(ctx, W, H, stops, angle) {
    const g = angle
      ? ctx.createLinearGradient(0, H, W, 0)
      : ctx.createLinearGradient(0, 0, 0, H);
    for (const [p, c] of stops) g.addColorStop(p, c);
    ctx.fillStyle = g;
    ctx.fillRect(0, 0, W, H);
  }
  function glow(ctx, x, y, r, color, alpha) {
    const g = ctx.createRadialGradient(x, y, 0, x, y, r);
    g.addColorStop(0, rgba(color, alpha));
    g.addColorStop(0.35, rgba(color, alpha * 0.45));
    g.addColorStop(1, rgba(color, 0));
    ctx.fillStyle = g;
    ctx.fillRect(x - r, y - r, r * 2, r * 2);
  }
  function stars(ctx, W, H, n, maxY, seed, color) {
    const R = rng(seed);
    ctx.fillStyle = color || '#ffffff';
    for (let i = 0; i < n; i++) {
      const x = R() * W, y = R() * maxY;
      const s = R() < 0.1 ? 1.6 : 0.9;
      ctx.globalAlpha = 0.25 + R() * 0.6 * (1 - y / maxY);
      ctx.fillRect(x, y, s, s);
    }
    ctx.globalAlpha = 1;
  }
  // A sea of cloud from y down to the bottom: a soft top edge of overlapping puffs over a gradient.
  function cloudSea(ctx, W, H, y, top, bottom, seed, puff) {
    const R = rng(seed);
    const g = ctx.createLinearGradient(0, y, 0, H);
    g.addColorStop(0, top);
    g.addColorStop(1, bottom);
    ctx.fillStyle = g;
    ctx.beginPath();
    ctx.moveTo(0, H);
    ctx.lineTo(0, y);
    const p = puff * W;
    for (let x = 0; x <= W + p; x += p * (0.45 + R() * 0.35)) {
      const r = p * (0.45 + R() * 0.55);
      ctx.arc(x, y + r * 0.35, r * 0.6, Math.PI, 0);
    }
    ctx.lineTo(W, H);
    ctx.closePath();
    ctx.fill();
    // Lit tops on the nearest puffs.
    for (let i = 0; i < 30; i++) {
      const x = R() * W, yy = y + (0.1 + R() * 0.6) * (H - y), r = p * (0.3 + R() * 0.6);
      glow(ctx, x, yy, r, top, 0.35);
    }
  }
  // Individual cloud: a cluster of circles with a lit top and shaded base.
  function cloud(ctx, x, y, w, light, shade, seed) {
    const R = rng(seed);
    const n = 7;
    const parts = [];
    for (let i = 0; i < n; i++) {
      const t = i / (n - 1);
      const r = w * (0.12 + 0.16 * Math.sin(t * Math.PI) + R() * 0.05);
      parts.push([x - w / 2 + t * w, y - r * 0.5 - Math.sin(t * Math.PI) * w * 0.08, r]);
    }
    ctx.fillStyle = shade;
    for (const [cx, cy, r] of parts) { ctx.beginPath(); ctx.arc(cx, cy + r * 0.25, r, 0, TAU); ctx.fill(); }
    ctx.fillStyle = light;
    for (const [cx, cy, r] of parts) { ctx.beginPath(); ctx.arc(cx - r * 0.12, cy - r * 0.08, r * 0.86, 0, TAU); ctx.fill(); }
  }
  function rays(ctx, x, y, n, len, color, alpha, seed) {
    const R = rng(seed);
    ctx.save();
    ctx.globalCompositeOperation = 'lighter';
    for (let i = 0; i < n; i++) {
      const a = R() * TAU, w = 0.03 + R() * 0.06;
      const g = ctx.createRadialGradient(x, y, 0, x, y, len);
      g.addColorStop(0, rgba(color, alpha));
      g.addColorStop(1, rgba(color, 0));
      ctx.fillStyle = g;
      ctx.beginPath();
      ctx.moveTo(x, y);
      ctx.arc(x, y, len, a - w, a + w);
      ctx.closePath();
      ctx.fill();
    }
    ctx.restore();
  }
  function birds(ctx, x, y, n, s, color, seed) {
    const R = rng(seed);
    ctx.strokeStyle = color;
    ctx.lineWidth = Math.max(1, s * 0.18);
    ctx.lineCap = 'round';
    for (let i = 0; i < n; i++) {
      const bx = x + (R() - 0.5) * s * 14, by = y + (R() - 0.5) * s * 6, k = s * (0.6 + R() * 0.6);
      ctx.beginPath();
      ctx.moveTo(bx - k, by - k * 0.35);
      ctx.quadraticCurveTo(bx - k * 0.4, by - k * 0.6, bx, by);
      ctx.quadraticCurveTo(bx + k * 0.4, by - k * 0.6, bx + k, by - k * 0.35);
      ctx.stroke();
    }
  }

  // --- Islands ---

  // A floating island: grassy top, jagged rock underside, optional details on top. (x, y) is the middle of its top.
  function island(ctx, x, y, w, o = {}) {
    const R = rng(o.seed || 1);
    const depth = w * (o.depth || 0.55);
    const cap = Math.max(2, w * (o.cap || 0.035));
    ctx.save();
    ctx.translate(x, y);
    if (o.tilt) ctx.rotate(o.tilt);
    if (o.glow) glow(ctx, 0, depth * 0.55, w * 0.75, o.glow, o.glowAlpha || 0.55);
    // Top surface, left to right.
    const top = [];
    const n = 14;
    for (let i = 0; i <= n; i++) {
      const t = i / n;
      const dome = Math.sin(t * Math.PI) * w * (o.dome || 0.035);
      top.push([-w / 2 + t * w, -dome + (R() - 0.5) * w * 0.012]);
    }
    // Underside, right to left: deepest near the middle, jagged.
    const under = [];
    const m = 22;
    for (let i = 0; i <= m; i++) {
      const t = 1 - i / m;
      const prof = Math.pow(Math.sin(t * Math.PI), 0.8);
      let d = depth * prof * (0.75 + R() * 0.35);
      if (R() < 0.25) d += depth * 0.18 * R();
      under.push([-w / 2 + t * w + (R() - 0.5) * w * 0.02, cap + d]);
    }
    const body = ctx.createLinearGradient(0, 0, 0, depth + cap);
    body.addColorStop(0, o.rock || '#6d5c4c');
    body.addColorStop(1, o.rockDeep || '#2c2420');
    ctx.fillStyle = body;
    ctx.beginPath();
    ctx.moveTo(top[0][0], top[0][1]);
    for (const p of top) ctx.lineTo(p[0], p[1]);
    for (const p of under) ctx.lineTo(p[0], p[1]);
    ctx.closePath();
    ctx.fill();
    // Strata lines on the rock.
    if (!o.flat) {
      ctx.strokeStyle = rgba(o.rockDeep || '#2c2420', 0.35);
      ctx.lineWidth = Math.max(1, w * 0.004);
      for (let k = 1; k <= 3; k++) {
        ctx.beginPath();
        const yy = cap + depth * k * 0.16;
        ctx.moveTo(-w * 0.42 * (1 - k * 0.12), yy);
        ctx.lineTo(w * 0.42 * (1 - k * 0.12), yy + w * 0.01);
        ctx.stroke();
      }
    }
    // Grass cap.
    ctx.fillStyle = o.grass || '#6f8f45';
    ctx.beginPath();
    ctx.moveTo(top[0][0], top[0][1]);
    for (const p of top) ctx.lineTo(p[0], p[1]);
    for (let i = top.length - 1; i >= 0; i--) ctx.lineTo(top[i][0], top[i][1] + cap * (0.6 + 0.6 * Math.sin((i / n) * Math.PI)));
    ctx.closePath();
    ctx.fill();
    if (o.rim) {
      ctx.strokeStyle = o.rim;
      ctx.lineWidth = Math.max(1, w * 0.006);
      ctx.beginPath();
      ctx.moveTo(top[0][0], top[0][1]);
      for (const p of top) ctx.lineTo(p[0], p[1]);
      ctx.stroke();
    }
    // Details stand on the top line; heightAt finds it.
    const heightAt = (px) => {
      const t = clamp((px + w / 2) / w, 0, 1) * n;
      const i = Math.min(n - 1, Math.floor(t));
      const f = t - i;
      return top[i][1] * (1 - f) + top[i + 1][1] * f;
    };
    const ink = o.ink || '#2a2a22';
    for (const d of o.items || []) {
      const px = d.at * w / 2;
      const py = heightAt(px);
      drawItem(ctx, d, px, py, w, ink, R);
    }
    if (o.waterfall) {
      const px = o.waterfall * w / 2;
      const py = heightAt(px);
      const g = ctx.createLinearGradient(0, py, 0, py + depth * 1.4);
      g.addColorStop(0, 'rgba(235,246,255,0.85)');
      g.addColorStop(1, 'rgba(235,246,255,0)');
      ctx.fillStyle = g;
      ctx.fillRect(px - w * 0.008, py, w * 0.016, depth * 1.4);
    }
    ctx.restore();
  }

  function drawItem(ctx, d, x, y, w, ink, R) {
    const s = w * (d.s || 1);
    ctx.fillStyle = d.color || ink;
    ctx.strokeStyle = d.color || ink;
    switch (d.k) {
      case 'tree': {
        const h = s * 0.07;
        ctx.fillRect(x - s * 0.004, y - h * 0.4, s * 0.008, h * 0.4);
        ctx.beginPath();
        ctx.arc(x, y - h * 0.62, h * 0.38, 0, TAU);
        ctx.arc(x - h * 0.25, y - h * 0.45, h * 0.28, 0, TAU);
        ctx.arc(x + h * 0.25, y - h * 0.47, h * 0.3, 0, TAU);
        ctx.fill();
        break;
      }
      case 'pine': {
        const h = s * 0.09;
        ctx.beginPath();
        ctx.moveTo(x, y - h);
        ctx.lineTo(x + h * 0.26, y);
        ctx.lineTo(x - h * 0.26, y);
        ctx.closePath();
        ctx.fill();
        break;
      }
      case 'house': {
        const bw = s * 0.07, bh = s * 0.04;
        ctx.fillRect(x - bw / 2, y - bh, bw, bh);
        ctx.beginPath();
        ctx.moveTo(x - bw * 0.6, y - bh);
        ctx.lineTo(x, y - bh - bw * 0.42);
        ctx.lineTo(x + bw * 0.6, y - bh);
        ctx.closePath();
        ctx.fill();
        if (d.lit) {
          ctx.fillStyle = d.lit;
          ctx.fillRect(x - bw * 0.25, y - bh * 0.7, bw * 0.12, bh * 0.3);
          ctx.fillRect(x + bw * 0.12, y - bh * 0.7, bw * 0.12, bh * 0.3);
        }
        break;
      }
      case 'barn': {
        const bw = s * 0.1, bh = s * 0.05;
        ctx.fillRect(x - bw / 2, y - bh, bw, bh);
        if (!d.noRoof) {
          ctx.beginPath();
          ctx.moveTo(x - bw * 0.55, y - bh);
          ctx.lineTo(x - bw * 0.3, y - bh - bw * 0.3);
          ctx.lineTo(x + bw * 0.3, y - bh - bw * 0.3);
          ctx.lineTo(x + bw * 0.55, y - bh);
          ctx.closePath();
          ctx.fill();
        }
        break;
      }
      case 'windmill': {
        const h = s * 0.16;
        ctx.beginPath();
        ctx.moveTo(x - h * 0.08, y);
        ctx.lineTo(x - h * 0.02, y - h);
        ctx.lineTo(x + h * 0.02, y - h);
        ctx.lineTo(x + h * 0.08, y);
        ctx.closePath();
        ctx.fill();
        ctx.save();
        ctx.translate(x, y - h);
        ctx.rotate(d.spin || 0.3);
        ctx.lineWidth = Math.max(1, h * 0.025);
        for (let i = 0; i < 6; i++) {
          ctx.rotate(TAU / 6);
          ctx.beginPath();
          ctx.moveTo(0, 0);
          ctx.lineTo(0, -h * 0.42);
          ctx.stroke();
          ctx.fillRect(-h * 0.035, -h * 0.42, h * 0.07, h * 0.22);
        }
        ctx.restore();
        break;
      }
      case 'tower': {
        const h = s * 0.2, bw = s * 0.022;
        ctx.lineWidth = Math.max(1, s * 0.003);
        ctx.beginPath();
        ctx.moveTo(x - bw, y);
        ctx.lineTo(x - bw * 0.55, y - h * 0.8);
        ctx.moveTo(x + bw, y);
        ctx.lineTo(x + bw * 0.55, y - h * 0.8);
        ctx.moveTo(x - bw * 0.8, y - h * 0.3);
        ctx.lineTo(x + bw * 0.8, y - h * 0.5);
        ctx.moveTo(x + bw * 0.8, y - h * 0.3);
        ctx.lineTo(x - bw * 0.8, y - h * 0.5);
        ctx.stroke();
        ctx.fillRect(x - bw * 0.9, y - h * 0.86, bw * 1.8, h * 0.1);
        if (!d.ruined) {
          ctx.beginPath();
          ctx.moveTo(x - bw * 1.1, y - h * 0.86);
          ctx.lineTo(x, y - h);
          ctx.lineTo(x + bw * 1.1, y - h * 0.86);
          ctx.fill();
        }
        break;
      }
      case 'pylon': {
        const h = s * 0.18;
        ctx.lineWidth = Math.max(1, s * 0.003);
        ctx.beginPath();
        ctx.moveTo(x - s * 0.015, y);
        ctx.lineTo(x, y - h);
        ctx.lineTo(x + s * 0.015, y);
        ctx.moveTo(x - s * 0.03, y - h * 0.85);
        ctx.lineTo(x + s * 0.03, y - h * 0.85);
        ctx.stroke();
        break;
      }
      case 'cross': {
        const h = s * 0.05;
        ctx.fillRect(x - s * 0.003, y - h, s * 0.006, h);
        ctx.fillRect(x - h * 0.3, y - h * 0.75, h * 0.6, s * 0.006);
        break;
      }
      case 'spire': {
        const h = s * (d.h || 0.3);
        ctx.beginPath();
        ctx.moveTo(x - s * 0.04, y);
        ctx.lineTo(x - s * 0.006, y - h);
        ctx.lineTo(x + s * 0.006, y - h);
        ctx.lineTo(x + s * 0.04, y);
        ctx.closePath();
        ctx.fill();
        break;
      }
      case 'dome': {
        const r = s * 0.05;
        ctx.beginPath();
        ctx.arc(x, y, r, Math.PI, 0);
        ctx.fill();
        ctx.fillRect(x - r * 0.08, y - r * 1.6, r * 0.16, r * 0.7);
        break;
      }
      case 'sail': {
        // The windmill rebuilt as a rig: a mast with three square sails filling with wind.
        const h = s * 0.32;
        ctx.fillStyle = ink;
        ctx.fillRect(x - s * 0.004, y - h, s * 0.008, h);
        const sails = d.sailColor || '#efe4c8';
        for (const [t0, t1, hw] of [[0.12, 0.36, 0.07], [0.4, 0.66, 0.095], [0.7, 0.94, 0.12]]) {
          const yTop = y - h * (1 - t0), yBot = y - h * (1 - t1);
          ctx.fillStyle = ink;
          ctx.fillRect(x - s * hw * 1.08, yTop - s * 0.004, s * hw * 2.16, s * 0.006);
          ctx.fillStyle = sails;
          ctx.beginPath();
          ctx.moveTo(x - s * hw, yTop);
          ctx.lineTo(x + s * hw, yTop);
          ctx.quadraticCurveTo(x + s * hw * 1.3, (yTop + yBot) / 2, x + s * hw * 0.92, yBot);
          ctx.lineTo(x - s * hw * 0.92, yBot);
          ctx.quadraticCurveTo(x - s * hw * 0.7, (yTop + yBot) / 2, x - s * hw, yTop);
          ctx.fill();
          ctx.strokeStyle = 'rgba(0,0,0,0.14)';
          ctx.lineWidth = Math.max(1, s * 0.002);
          ctx.beginPath();
          ctx.moveTo(x + s * hw * 0.2, yTop);
          ctx.quadraticCurveTo(x + s * hw * 0.45, (yTop + yBot) / 2, x + s * hw * 0.2, yBot);
          ctx.stroke();
        }
        ctx.fillStyle = d.flag || '#c0392b';
        ctx.beginPath();
        ctx.moveTo(x, y - h);
        ctx.lineTo(x - s * 0.05, y - h * 0.97);
        ctx.lineTo(x, y - h * 0.94);
        ctx.fill();
        break;
      }
      default:
        break;
    }
  }

  // --- Ships, creatures, people ---

  function airship(ctx, x, y, s, o = {}) {
    ctx.save();
    ctx.translate(x, y);
    if (o.flip) ctx.scale(-1, 1);
    const ink = o.ink || '#1d2128';
    if (o.kind === 'pirate') {
      ctx.fillStyle = ink;
      ctx.beginPath();
      ctx.moveTo(-s * 0.5, 0);
      ctx.quadraticCurveTo(-s * 0.45, s * 0.22, 0, s * 0.24);
      ctx.quadraticCurveTo(s * 0.45, s * 0.22, s * 0.62, -s * 0.05);
      ctx.lineTo(-s * 0.5, 0);
      ctx.fill();
      ctx.fillRect(-s * 0.12, -s * 0.75, s * 0.025, s * 0.75);
      ctx.fillRect(s * 0.2, -s * 0.6, s * 0.022, s * 0.6);
      ctx.fillStyle = o.sail || '#15171c';
      for (const [mx, h, wd] of [[-0.11, 0.7, 0.42], [0.21, 0.55, 0.34]]) {
        ctx.beginPath();
        ctx.moveTo(s * mx, -s * h);
        ctx.quadraticCurveTo(s * (mx + wd), -s * h * 0.6, s * mx, -s * 0.08);
        ctx.closePath();
        ctx.fill();
      }
    } else {
      // Balloon airship: envelope, ropes, gondola, propeller.
      const env = o.envelope || '#a8443a';
      const eg = ctx.createLinearGradient(0, -s * 0.55, 0, -s * 0.1);
      eg.addColorStop(0, mix(env, '#ffffff', 0.25));
      eg.addColorStop(1, mix(env, '#000000', 0.35));
      ctx.fillStyle = eg;
      ctx.beginPath();
      ctx.ellipse(0, -s * 0.33, s * 0.6, s * 0.22, 0, 0, TAU);
      ctx.fill();
      if (o.stripe) {
        ctx.fillStyle = o.stripe;
        ctx.fillRect(-s * 0.5, -s * 0.36, s * 1.0, s * 0.05);
      }
      ctx.strokeStyle = ink;
      ctx.lineWidth = Math.max(1, s * 0.012);
      ctx.beginPath();
      ctx.moveTo(-s * 0.35, -s * 0.15);
      ctx.lineTo(-s * 0.22, s * 0.02);
      ctx.moveTo(s * 0.35, -s * 0.15);
      ctx.lineTo(s * 0.22, s * 0.02);
      ctx.stroke();
      ctx.fillStyle = ink;
      ctx.beginPath();
      ctx.moveTo(-s * 0.3, s * 0.02);
      ctx.lineTo(s * 0.3, s * 0.02);
      ctx.lineTo(s * 0.22, s * 0.12);
      ctx.lineTo(-s * 0.24, s * 0.12);
      ctx.closePath();
      ctx.fill();
      ctx.fillRect(-s * 0.68, -s * 0.38, s * 0.04, s * 0.1);
      if (o.lights) {
        ctx.fillStyle = o.lights;
        for (let i = -2; i <= 2; i++) ctx.fillRect(i * s * 0.09 - s * 0.015, s * 0.05, s * 0.03, s * 0.025);
      }
    }
    ctx.restore();
  }

  function frigate(ctx, x, y, s, ink) {
    ctx.fillStyle = ink;
    ctx.beginPath();
    ctx.moveTo(x - s * 0.5, y);
    ctx.lineTo(x + s * 0.55, y - s * 0.05);
    ctx.lineTo(x + s * 0.35, y + s * 0.12);
    ctx.lineTo(x - s * 0.42, y + s * 0.12);
    ctx.closePath();
    ctx.fill();
    ctx.fillRect(x - s * 0.05, y - s * 0.45, s * 0.03, s * 0.45);
    ctx.beginPath();
    ctx.ellipse(x - s * 0.05, y - s * 0.55, s * 0.32, s * 0.1, 0, 0, TAU);
    ctx.fill();
    ctx.lineWidth = Math.max(1, s * 0.015);
    ctx.strokeStyle = ink;
    ctx.beginPath();
    ctx.moveTo(x + s * 0.5, y - s * 0.05);
    ctx.lineTo(x + s * 0.8, y - s * 0.12);
    ctx.stroke();
  }

  // A sky titan: whale-like body facing left, with an island grown on its back.
  function titan(ctx, x, y, L, o = {}) {
    const R = rng(o.seed || 7);
    ctx.save();
    ctx.translate(x, y);
    const h = L * 0.2;
    const body = ctx.createLinearGradient(0, -h, 0, h * 0.8);
    body.addColorStop(0, o.top || '#4a5a5e');
    body.addColorStop(1, o.belly || '#9aa9a4');
    ctx.fillStyle = body;
    ctx.beginPath();
    ctx.moveTo(-L * 0.5, 0);
    ctx.bezierCurveTo(-L * 0.5, -h * 0.9, -L * 0.25, -h * 1.05, 0, -h * 0.95);
    ctx.bezierCurveTo(L * 0.25, -h * 0.85, L * 0.38, -h * 0.4, L * 0.46, -h * 0.12);
    ctx.lineTo(L * 0.62, -h * 0.55);
    ctx.quadraticCurveTo(L * 0.6, -h * 0.05, L * 0.66, h * 0.32);
    ctx.lineTo(L * 0.47, h * 0.06);
    ctx.bezierCurveTo(L * 0.3, h * 0.55, -L * 0.2, h * 0.75, -L * 0.42, h * 0.45);
    ctx.quadraticCurveTo(-L * 0.52, h * 0.3, -L * 0.5, 0);
    ctx.fill();
    // Pectoral fin.
    ctx.fillStyle = o.top || '#4a5a5e';
    ctx.beginPath();
    ctx.moveTo(-L * 0.18, h * 0.45);
    ctx.quadraticCurveTo(-L * 0.12, h * 1.1, L * 0.02, h * 1.0);
    ctx.quadraticCurveTo(-L * 0.04, h * 0.7, -L * 0.02, h * 0.5);
    ctx.fill();
    // Grooves along the belly.
    ctx.strokeStyle = rgba('#000000', 0.12);
    ctx.lineWidth = Math.max(1, L * 0.003);
    for (let i = 0; i < 6; i++) {
      ctx.beginPath();
      ctx.moveTo(-L * 0.42 + i * L * 0.04, h * 0.42);
      ctx.quadraticCurveTo(-L * 0.1 + i * L * 0.05, h * 0.62, L * 0.2 + i * L * 0.03, h * 0.38);
      ctx.stroke();
    }
    // Eye.
    ctx.fillStyle = o.eye || '#ffd27a';
    ctx.beginPath();
    ctx.arc(-L * 0.4, -h * 0.05, L * 0.008, 0, TAU);
    ctx.fill();
    // The island on its back: a grass cap following the back curve, with the farm on it.
    if (o.island) {
      const pts = [];
      for (let i = 0; i <= 20; i++) {
        const t = i / 20;
        const px = -L * 0.32 + t * L * 0.6;
        const py = -h * (0.97 - 0.5 * Math.pow((t - 0.45) * 1.25, 2)) - L * 0.006;
        pts.push([px, py]);
      }
      ctx.fillStyle = o.island.grass || '#7e9a4d';
      ctx.beginPath();
      ctx.moveTo(pts[0][0], pts[0][1] + h * 0.12);
      for (const p of pts) ctx.lineTo(p[0], p[1]);
      ctx.lineTo(pts[20][0], pts[20][1] + h * 0.12);
      ctx.closePath();
      ctx.fill();
      for (const it of o.island.items) {
        const t = it.t;
        const i = Math.round(t * 20);
        drawItem(ctx, it, pts[i][0], pts[i][1] + L * 0.002, L * 0.6, o.island.ink || '#2b3326', R);
      }
    }
    ctx.restore();
  }

  // Standing or seated figure in silhouette. (x, y) is where the feet (or seat) are.
  function person(ctx, x, y, h, o = {}) {
    ctx.save();
    ctx.fillStyle = o.color || '#15161a';
    ctx.strokeStyle = o.color || '#15161a';
    const head = h * 0.11;
    if (o.seated) {
      // Seated on a rail at y, legs hanging.
      ctx.fillRect(x - h * 0.08, y - h * 0.5, h * 0.16, h * 0.5);
      ctx.beginPath();
      ctx.moveTo(x - h * 0.13, y - h * 0.45);
      ctx.lineTo(x + h * 0.13, y - h * 0.45);
      ctx.lineTo(x + h * 0.16, y + h * 0.03);
      ctx.lineTo(x - h * 0.16, y + h * 0.03);
      ctx.closePath();
      ctx.fill();
      ctx.lineWidth = h * 0.06;
      ctx.lineCap = 'round';
      ctx.beginPath();
      ctx.moveTo(x - h * 0.05, y);
      ctx.lineTo(x + h * 0.18, y + h * 0.04);
      ctx.lineTo(x + h * 0.2, y + h * 0.36);
      ctx.moveTo(x + h * 0.04, y);
      ctx.lineTo(x + h * 0.26, y + h * 0.06);
      ctx.lineTo(x + h * 0.3, y + h * 0.37);
      ctx.stroke();
      ctx.beginPath();
      ctx.arc(x, y - h * 0.5 - head, head, 0, TAU);
      ctx.fill();
      hat(ctx, x, y - h * 0.5 - head * 1.6, head, o.hat);
      ctx.restore();
      return;
    }
    // Legs.
    ctx.lineWidth = h * 0.07;
    ctx.lineCap = 'round';
    ctx.beginPath();
    ctx.moveTo(x - h * 0.05, y);
    ctx.lineTo(x - h * 0.03, y - h * 0.45);
    ctx.moveTo(x + h * 0.06, y);
    ctx.lineTo(x + h * 0.03, y - h * 0.45);
    ctx.stroke();
    // Body (a duster coat flares).
    ctx.beginPath();
    ctx.moveTo(x - h * 0.11, y - h * 0.8);
    ctx.lineTo(x + h * 0.11, y - h * 0.8);
    ctx.lineTo(x + (o.coat ? h * 0.16 : h * 0.1), y - (o.coat ? h * 0.18 : h * 0.42));
    ctx.lineTo(x - (o.coat ? h * 0.15 : h * 0.1), y - (o.coat ? h * 0.18 : h * 0.42));
    ctx.closePath();
    ctx.fill();
    // Arms.
    ctx.lineWidth = h * 0.055;
    ctx.beginPath();
    ctx.moveTo(x - h * 0.1, y - h * 0.76);
    ctx.lineTo(x - h * 0.15, y - h * 0.5);
    ctx.moveTo(x + h * 0.1, y - h * 0.76);
    if (o.rifle) {
      ctx.lineTo(x + h * 0.2, y - h * 0.6);
      ctx.stroke();
      ctx.lineWidth = h * 0.025;
      ctx.beginPath();
      ctx.moveTo(x + h * 0.04, y - h * 0.45);
      ctx.lineTo(x + h * 0.32, y - h * 0.98);
    } else {
      ctx.lineTo(x + h * 0.15, y - h * 0.5);
    }
    ctx.stroke();
    // Head.
    if (o.helmet) {
      ctx.beginPath();
      ctx.arc(x, y - h * 0.8 - head * 1.2, head * 1.5, 0, TAU);
      ctx.fill();
      ctx.fillStyle = o.visor || '#ffcf7a';
      ctx.beginPath();
      ctx.arc(x - head * 0.4, y - h * 0.8 - head * 1.2, head * 0.6, 0, TAU);
      ctx.fill();
    } else {
      ctx.beginPath();
      ctx.arc(x, y - h * 0.8 - head, head, 0, TAU);
      ctx.fill();
      hat(ctx, x, y - h * 0.8 - head * 1.6, head, o.hat);
    }
    ctx.restore();
  }
  function hat(ctx, x, y, head, kind) {
    if (kind === 'cowboy') {
      ctx.beginPath();
      ctx.ellipse(x, y + head * 0.2, head * 1.8, head * 0.35, 0, 0, TAU);
      ctx.fill();
      ctx.fillRect(x - head * 0.8, y - head * 0.7, head * 1.6, head * 0.9);
    } else if (kind === 'top') {
      ctx.fillRect(x - head * 1.4, y + head * 0.1, head * 2.8, head * 0.3);
      ctx.fillRect(x - head * 0.85, y - head * 2.1, head * 1.7, head * 2.3);
    }
  }
  function crow(ctx, x, y, s, color) {
    ctx.fillStyle = color;
    ctx.beginPath();
    ctx.ellipse(x, y, s * 0.5, s * 0.25, -0.2, 0, TAU);
    ctx.fill();
    ctx.beginPath();
    ctx.arc(x - s * 0.45, y - s * 0.15, s * 0.18, 0, TAU);
    ctx.fill();
    ctx.beginPath();
    ctx.moveTo(x - s * 0.6, y - s * 0.15);
    ctx.lineTo(x - s * 0.85, y - s * 0.1);
    ctx.lineTo(x - s * 0.6, y - s * 0.05);
    ctx.fill();
    ctx.beginPath();
    ctx.moveTo(x + s * 0.4, y);
    ctx.lineTo(x + s * 0.85, y + s * 0.2);
    ctx.lineTo(x + s * 0.4, y + s * 0.15);
    ctx.fill();
  }
  function beam(ctx, x, yTop, yBottom, w, color, alpha) {
    ctx.save();
    ctx.globalCompositeOperation = 'lighter';
    const g = ctx.createLinearGradient(0, yTop, 0, yBottom);
    g.addColorStop(0, rgba(color, 0));
    g.addColorStop(0.6, rgba(color, alpha * 0.6));
    g.addColorStop(1, rgba(color, alpha));
    ctx.fillStyle = g;
    ctx.fillRect(x - w / 2, yTop, w, yBottom - yTop);
    ctx.fillRect(x - w / 6, yTop, w / 3, yBottom - yTop);
    ctx.restore();
    glow(ctx, x, yBottom, w * 3, color, alpha * 0.7);
  }
  function firework(ctx, x, y, r, color, seed) {
    const R = rng(seed);
    glow(ctx, x, y, r * 1.3, color, 0.35);
    ctx.save();
    ctx.globalCompositeOperation = 'lighter';
    ctx.strokeStyle = color;
    ctx.fillStyle = color;
    ctx.lineCap = 'round';
    const n = 28;
    for (let i = 0; i < n; i++) {
      const a = (i / n) * TAU + R() * 0.1;
      const l = r * (0.7 + R() * 0.3);
      ctx.lineWidth = Math.max(1, r * 0.025);
      ctx.globalAlpha = 0.85;
      ctx.beginPath();
      ctx.moveTo(x + Math.cos(a) * l * 0.35, y + Math.sin(a) * l * 0.35);
      ctx.lineTo(x + Math.cos(a) * l, y + Math.sin(a) * l + l * 0.06);
      ctx.stroke();
      ctx.beginPath();
      ctx.arc(x + Math.cos(a) * l, y + Math.sin(a) * l + l * 0.06, Math.max(1, r * 0.03), 0, TAU);
      ctx.fill();
    }
    ctx.restore();
  }
  function streak(ctx, x1, y1, x2, y2, color, width) {
    ctx.save();
    ctx.globalCompositeOperation = 'lighter';
    const g = ctx.createLinearGradient(x1, y1, x2, y2);
    g.addColorStop(0, rgba(color, 0));
    g.addColorStop(1, rgba(color, 0.95));
    ctx.strokeStyle = g;
    ctx.lineCap = 'round';
    for (const [lw, a] of [[width * 4, 0.25], [width * 2, 0.5], [width, 1]]) {
      ctx.globalAlpha = a;
      ctx.lineWidth = lw;
      ctx.beginPath();
      ctx.moveTo(x1, y1);
      ctx.lineTo(x2, y2);
      ctx.stroke();
    }
    ctx.restore();
    glow(ctx, x2, y2, width * 14, color, 0.8);
  }
  function crystals(ctx, x, y, w, color, seed, n) {
    const R = rng(seed);
    glow(ctx, x, y, w * 1.1, color, 0.6);
    for (let i = 0; i < (n || 9); i++) {
      const cx = x + (R() - 0.5) * w, h = w * (0.12 + R() * 0.33), bw = w * (0.025 + R() * 0.04);
      const lean = (R() - 0.5) * 0.5;
      const g = ctx.createLinearGradient(0, y - h, 0, y);
      g.addColorStop(0, mix(color, '#ffffff', 0.55));
      g.addColorStop(1, mix(color, '#000000', 0.25));
      ctx.fillStyle = g;
      ctx.beginPath();
      ctx.moveTo(cx - bw, y);
      ctx.lineTo(cx + lean * h * 0.3, y - h);
      ctx.lineTo(cx + bw, y);
      ctx.closePath();
      ctx.fill();
    }
  }

  // --- Scenes ---

  const scenes = {
    lodestone(ctx, W, H) {
      sky(ctx, W, H, [[0, '#2d4a63'], [0.45, '#b88456'], [0.7, '#f0bf78'], [1, '#f6d6a4']]);
      glow(ctx, W * 0.2, H * 0.6, W * 0.45, '#ffd28a', 0.85);
      // The Skyline: pylon islands and cables.
      island(ctx, W * 0.1, H * 0.42, W * 0.08, { seed: 3, grass: '#9a8a62', rock: '#8a7462', rockDeep: '#b88f6a', ink: '#6d5a48', flat: true, items: [{ k: 'pylon', at: 0, s: 1.6, color: '#5b4a3c' }] });
      island(ctx, W * 0.37, H * 0.36, W * 0.07, { seed: 5, grass: '#9a8a62', rock: '#8a7462', rockDeep: '#b88f6a', ink: '#6d5a48', flat: true, items: [{ k: 'pylon', at: 0, s: 1.6, color: '#5b4a3c' }] });
      island(ctx, W * 0.86, H * 0.3, W * 0.1, { seed: 9, grass: '#9a8a62', rock: '#8a7462', rockDeep: '#b88f6a', ink: '#6d5a48', flat: true, items: [{ k: 'pylon', at: -0.2, s: 1.4, color: '#5b4a3c' }, { k: 'house', at: 0.4, s: 1.2, color: '#5b4a3c' }] });
      ctx.strokeStyle = '#4a3a2e';
      ctx.lineWidth = Math.max(1, W * 0.0012);
      const cable = (x1, y1, x2, y2, sag) => {
        ctx.beginPath();
        ctx.moveTo(x1, y1);
        ctx.quadraticCurveTo((x1 + x2) / 2, Math.max(y1, y2) + sag, x2, y2);
        ctx.stroke();
      };
      const p1 = [W * 0.1, H * 0.42 - W * 0.08 * 1.6 * 0.18], p2 = [W * 0.37, H * 0.36 - W * 0.07 * 1.6 * 0.18];
      const p3 = [W * 0.86 - W * 0.01, H * 0.3 - W * 0.1 * 1.4 * 0.18];
      cable(p1[0], p1[1], p2[0], p2[1], H * 0.06);
      cable(p2[0], p2[1], p3[0], p3[1], H * 0.1);
      // A cable car hanging from the far cable.
      const q = (t, a, c, b) => (1 - t) * (1 - t) * a + 2 * (1 - t) * t * c + t * t * b;
      const tx = q(0.62, p2[0], (p2[0] + p3[0]) / 2, p3[0]), ty = q(0.62, p2[1], Math.max(p2[1], p3[1]) + H * 0.1, p3[1]);
      ctx.fillStyle = '#4a3a2e';
      ctx.fillRect(tx - 1, ty, 2, H * 0.02);
      ctx.fillRect(tx - W * 0.022, ty + H * 0.02, W * 0.044, H * 0.03);
      cloudSea(ctx, W, H, H * 0.7, '#f7dcb4', '#d9a77a', 11, 0.08);
      // Tallow Flats, tilting and sinking, its windmill going over.
      island(ctx, W * 0.52, H * 0.6, W * 0.3, {
        seed: 21, tilt: 0.16, grass: '#c8a64e', rock: '#7a5e48', rockDeep: '#3e2f26', rim: '#e9cf86', ink: '#3b2f24',
        glow: '#ff9a3c', glowAlpha: 0.35,
        items: [{ k: 'barn', at: -0.4, s: 1 }, { k: 'house', at: -0.1, s: 1 }, { k: 'windmill', at: 0.35, s: 1, spin: 0.9 }, { k: 'tree', at: 0.7, s: 1 }],
      });
      cloudSea(ctx, W, H, H * 0.78, '#f4d3a8', '#cf9a6c', 13, 0.06);
      // Foreground: the plateau edge and the ruined lookout, with Rowan watching.
      ctx.fillStyle = '#231a14';
      ctx.beginPath();
      ctx.moveTo(W, H * 0.48);
      ctx.lineTo(W * 0.86, H * 0.5);
      ctx.lineTo(W * 0.8, H * 0.56);
      ctx.lineTo(W * 0.78, H * 0.7);
      ctx.lineTo(W * 0.81, H);
      ctx.lineTo(W, H);
      ctx.closePath();
      ctx.fill();
      drawItem(ctx, { k: 'tower', s: 1.3, ruined: true }, W * 0.93, H * 0.49, W, '#231a14', rng(2));
      person(ctx, W * 0.84, H * 0.505, H * 0.16, { hat: 'cowboy', coat: true, color: '#231a14' });
      birds(ctx, W * 0.32, H * 0.2, 5, W * 0.006, '#3b2a20', 4);
    },

    revenant(ctx, W, H) {
      sky(ctx, W, H, [[0, '#120c24'], [0.35, '#3b1a43'], [0.62, '#a3384b'], [0.8, '#ec8248'], [1, '#f7b066']]);
      stars(ctx, W, H, 140, H * 0.4, 31, '#f4e9ff');
      ctx.fillStyle = '#f3e7d8';
      ctx.beginPath();
      ctx.arc(W * 0.78, H * 0.2, H * 0.06, 0, TAU);
      ctx.fill();
      glow(ctx, W * 0.78, H * 0.2, H * 0.3, '#f3e7d8', 0.25);
      // Seven islands on the horizon: seven robbed saints.
      const xs = [0.38, 0.47, 0.55, 0.63, 0.72, 0.81, 0.92];
      xs.forEach((fx, i) => {
        const y = H * (0.6 + Math.sin(i * 1.7) * 0.03);
        island(ctx, W * fx, y, W * (0.035 + (i % 3) * 0.01), { seed: 40 + i, grass: '#3a1f33', rock: '#3a1f33', rockDeep: '#4d2238', flat: true, ink: '#3a1f33', items: [{ k: 'cross', at: 0, s: 2.2, color: '#3a1f33' }] });
        glow(ctx, W * fx, y - W * 0.02, W * 0.012, '#ff9a3c', 0.5);
      });
      cloudSea(ctx, W, H, H * 0.68, '#7a3044', '#2a1022', 33, 0.07);
      // Foreground: the lookout platform where Ellis died.
      ctx.fillStyle = '#0f0911';
      ctx.fillRect(0, H * 0.62, W * 0.42, H * 0.06);
      ctx.fillRect(W * 0.04, H * 0.68, W * 0.025, H * 0.32);
      ctx.fillRect(W * 0.33, H * 0.68, W * 0.025, H * 0.32);
      ctx.lineWidth = Math.max(1, W * 0.004);
      ctx.strokeStyle = '#0f0911';
      ctx.beginPath();
      ctx.moveTo(W * 0.05, H * 0.68);
      ctx.lineTo(W * 0.34, H);
      ctx.moveTo(W * 0.34, H * 0.68);
      ctx.lineTo(W * 0.05, H);
      ctx.stroke();
      // The railing, with Sexton sitting on it.
      ctx.fillRect(0, H * 0.5, W * 0.42, H * 0.018);
      for (let i = 0; i <= 6; i++) ctx.fillRect(W * (0.01 + i * 0.066), H * 0.5, W * 0.006, H * 0.12);
      person(ctx, W * 0.27, H * 0.5, H * 0.3, { seated: true, hat: 'top', color: '#0f0911' });
      crow(ctx, W * 0.25, H * 0.3, W * 0.02, '#0f0911');
      // The grave in the orchard.
      ctx.beginPath();
      ctx.ellipse(W * 0.88, H * 1.02, W * 0.18, H * 0.12, 0, Math.PI, 0);
      ctx.fill();
      drawItem(ctx, { k: 'cross', s: 2.6 }, W * 0.88, H * 0.91, W, '#0f0911', rng(3));
      birds(ctx, W * 0.6, H * 0.3, 6, W * 0.007, '#1a0c1c', 37);
    },

    skyfarers(ctx, W, H) {
      sky(ctx, W, H, [[0, '#1f6db4'], [0.55, '#62b2ea'], [1, '#d6efff']]);
      glow(ctx, W * 0.82, H * 0.12, W * 0.3, '#ffffff', 0.6);
      cloud(ctx, W * 0.2, H * 0.32, W * 0.3, '#ffffff', '#c6dff2', 51);
      cloud(ctx, W * 0.72, H * 0.42, W * 0.26, '#ffffff', '#c9e1f3', 52);
      cloud(ctx, W * 0.5, H * 0.2, W * 0.18, '#f6fbff', '#cfe4f4', 53);
      cloudSea(ctx, W, H, H * 0.74, '#ffffff', '#b9d6ee', 54, 0.07);
      // Wake: wisps behind the sailing island.
      ctx.fillStyle = 'rgba(255,255,255,0.7)';
      for (let i = 0; i < 6; i++) {
        ctx.beginPath();
        ctx.ellipse(W * (0.2 - i * 0.03), H * (0.63 + i * 0.012), W * (0.06 - i * 0.006), H * 0.012, 0, 0, TAU);
        ctx.fill();
      }
      island(ctx, W * 0.46, H * 0.62, W * 0.34, {
        seed: 55, tilt: -0.04, grass: '#7a9d47', rock: '#7d6a58', rockDeep: '#3a2f28', rim: '#a8c46a', ink: '#2e2b24',
        items: [{ k: 'house', at: -0.55, s: 0.9 }, { k: 'barn', at: -0.3, s: 0.9 }, { k: 'sail', at: 0.05, s: 1, sailColor: '#f3e9d2', flag: '#d4a017' }, { k: 'tree', at: 0.5, s: 0.9 }, { k: 'tower', at: 0.78, s: 0.8 }],
      });
      airship(ctx, W * 0.13, H * 0.36, W * 0.09, { kind: 'pirate', ink: '#1a1a1f', sail: '#111114' });
      airship(ctx, W * 0.86, H * 0.32, W * 0.1, { envelope: '#b0413a', stripe: '#f0d9a8', ink: '#2a2523', flip: true });
      birds(ctx, W * 0.62, H * 0.18, 4, W * 0.006, '#ffffff', 56);
    },

    leviathan(ctx, W, H) {
      sky(ctx, W, H, [[0, '#2a4d68'], [0.4, '#7fa9b6'], [0.75, '#e9d3a8'], [1, '#f6e2b8']]);
      glow(ctx, W * 0.66, H * 0.42, W * 0.45, '#fff0c8', 0.75);
      rays(ctx, W * 0.66, H * 0.42, 14, W * 0.7, '#fff3d4', 0.12, 61);
      // Distant grazers.
      titan(ctx, W * 0.14, H * 0.36, W * 0.12, { top: '#8aa3a6', belly: '#b9c6bd', seed: 62, island: { grass: '#9fb38a', items: [{ k: 'tree', t: 0.4, s: 1.2, color: '#7f9478' }] } });
      titan(ctx, W * 0.9, H * 0.3, W * 0.08, { top: '#93aaa9', belly: '#c3cec3', seed: 63, island: { grass: '#a9bb91', items: [{ k: 'tree', t: 0.5, s: 1.2, color: '#8a9d80' }] } });
      cloudSea(ctx, W, H, H * 0.76, '#fbeed2', '#c9b79a', 64, 0.08);
      // Shadow on the clouds.
      ctx.fillStyle = 'rgba(70,80,80,0.18)';
      ctx.beginPath();
      ctx.ellipse(W * 0.5, H * 0.86, W * 0.3, H * 0.04, 0, 0, TAU);
      ctx.fill();
      // Old Hollow, with Skyreach's farm on its back.
      titan(ctx, W * 0.48, H * 0.57, W * 0.62, {
        top: '#3e4b4e', belly: '#8c9a94', eye: '#ffcf7a', seed: 65,
        island: {
          grass: '#77924a', ink: '#283022',
          items: [{ k: 'house', t: 0.15, s: 0.9 }, { k: 'tree', t: 0.3, s: 1 }, { k: 'windmill', t: 0.48, s: 0.9 }, { k: 'barn', t: 0.65, s: 0.9 }, { k: 'tree', t: 0.8, s: 1 }, { k: 'tree', t: 0.9, s: 0.9 }],
        },
      });
      // Biscuit, floating ahead of the head.
      titan(ctx, W * 0.1, H * 0.6, W * 0.05, { top: '#6c7d7e', belly: '#b9c4bd', seed: 66 });
      frigate(ctx, W * 0.93, H * 0.58, W * 0.035, '#5d6668');
      birds(ctx, W * 0.33, H * 0.2, 4, W * 0.006, '#3d4f58', 67);
    },

    warden(ctx, W, H) {
      sky(ctx, W, H, [[0, '#0a1c2e'], [0.5, '#1c4a5d'], [1, '#5c9fa5']]);
      stars(ctx, W, H, 120, H * 0.45, 71, '#e7f6ff');
      // Beacons firing from every island.
      const far = [[0.08, 0.55, 0.06], [0.2, 0.48, 0.05], [0.8, 0.5, 0.06], [0.93, 0.58, 0.05], [0.68, 0.42, 0.04]];
      far.forEach(([fx, fy, fw], i) => {
        island(ctx, W * fx, H * fy, W * fw, { seed: 72 + i, grass: '#244150', rock: '#244150', rockDeep: '#2f5868', flat: true, ink: '#244150' });
        beam(ctx, W * fx, 0, H * fy - W * 0.01, W * 0.008, '#bff6ff', 0.55);
      });
      cloudSea(ctx, W, H, H * 0.74, '#6fa8ad', '#1e3e4c', 77, 0.07);
      // Skyreach, cut away: the farm on top, the machine beneath.
      const x = W * 0.48, y = H * 0.44, w = W * 0.46;
      island(ctx, x, y, w, { seed: 78, depth: 0.62, grass: '#5f8a45', rock: '#4d4238', rockDeep: '#1f1a17', ink: '#16201a', items: [{ k: 'house', at: -0.6, s: 0.8, lit: '#ffd27a' }, { k: 'barn', at: -0.35, s: 0.8 }, { k: 'tree', at: 0.05, s: 0.9 }, { k: 'windmill', at: 0.3, s: 0.8 }, { k: 'tower', at: 0.75, s: 0.7 }] });
      // The cutaway: a clean face and the hollow with its works.
      ctx.save();
      ctx.beginPath();
      ctx.moveTo(x - w * 0.36, y + w * 0.05);
      ctx.lineTo(x + w * 0.34, y + w * 0.05);
      ctx.lineTo(x + w * 0.16, y + w * 0.36);
      ctx.lineTo(x - w * 0.12, y + w * 0.38);
      ctx.closePath();
      ctx.fillStyle = '#0b1218';
      ctx.fill();
      ctx.strokeStyle = '#8fd8ff';
      ctx.lineWidth = Math.max(1, W * 0.0015);
      ctx.stroke();
      ctx.clip();
      ctx.strokeStyle = 'rgba(92,202,255,0.55)';
      for (let i = 0; i < 5; i++) {
        ctx.beginPath();
        ctx.moveTo(x - w * 0.36, y + w * (0.09 + i * 0.05));
        ctx.lineTo(x + w * 0.34, y + w * (0.09 + i * 0.05));
        ctx.stroke();
      }
      for (const [gx, gy, gr] of [[-0.2, 0.15, 0.05], [0.2, 0.14, 0.04], [-0.05, 0.27, 0.035], [0.1, 0.28, 0.03]]) {
        ctx.beginPath();
        ctx.arc(x + w * gx, y + w * gy, w * gr, 0, TAU);
        ctx.stroke();
        for (let k = 0; k < 10; k++) {
          const a = (k / 10) * TAU;
          ctx.beginPath();
          ctx.moveTo(x + w * gx + Math.cos(a) * w * gr, y + w * gy + Math.sin(a) * w * gr);
          ctx.lineTo(x + w * gx + Math.cos(a) * w * gr * 1.25, y + w * gy + Math.sin(a) * w * gr * 1.25);
          ctx.stroke();
        }
      }
      ctx.restore();
      // The lattice-core.
      const cx = x + w * 0.02, cy = y + w * 0.23;
      glow(ctx, cx, cy, w * 0.2, '#ff9f1c', 0.8);
      ctx.fillStyle = '#ffc266';
      ctx.beginPath();
      ctx.moveTo(cx, cy - w * 0.06);
      ctx.lineTo(cx + w * 0.03, cy);
      ctx.lineTo(cx, cy + w * 0.06);
      ctx.lineTo(cx - w * 0.03, cy);
      ctx.closePath();
      ctx.fill();
      // A hatch under the grove, and a ladder down.
      ctx.strokeStyle = '#8fd8ff';
      ctx.beginPath();
      ctx.moveTo(x + w * 0.02, y + w * 0.0);
      ctx.lineTo(x + w * 0.02, y + w * 0.1);
      ctx.stroke();
    },

    below(ctx, W, H) {
      sky(ctx, W, H, [[0, '#a6cde6'], [0.3, '#56758f'], [0.62, '#132033'], [0.85, '#1b1420'], [1, '#6a2e10']]);
      glow(ctx, W * 0.7, H * 1.05, W * 0.55, '#ff8a2a', 0.75);
      // The underside of an island across the top, roots and stalactites hanging.
      const R = rng(81);
      ctx.fillStyle = '#2a2420';
      ctx.beginPath();
      ctx.moveTo(0, 0);
      ctx.lineTo(W, 0);
      for (let i = 0; i <= 40; i++) {
        const t = 1 - i / 40;
        const d = H * (0.08 + 0.06 * Math.sin(t * Math.PI * 1.3) + R() * 0.04 + (R() < 0.2 ? R() * 0.1 : 0));
        ctx.lineTo(t * W, d);
      }
      ctx.closePath();
      ctx.fill();
      ctx.strokeStyle = '#3b3128';
      ctx.lineWidth = Math.max(1, W * 0.0015);
      for (let i = 0; i < 26; i++) {
        const rx = R() * W, rl = H * (0.08 + R() * 0.18);
        ctx.beginPath();
        ctx.moveTo(rx, H * 0.1);
        ctx.quadraticCurveTo(rx + (R() - 0.5) * W * 0.03, H * 0.1 + rl * 0.6, rx + (R() - 0.5) * W * 0.02, H * 0.1 + rl);
        ctx.stroke();
      }
      // The gantry, and the line paying out.
      ctx.fillStyle = '#1a1512';
      ctx.fillRect(W * 0.14, H * 0.08, W * 0.12, H * 0.02);
      ctx.fillRect(W * 0.24, H * 0.08, W * 0.008, H * 0.06);
      ctx.strokeStyle = 'rgba(230,220,200,0.8)';
      ctx.lineWidth = Math.max(1, W * 0.0012);
      ctx.beginPath();
      ctx.moveTo(W * 0.244, H * 0.14);
      ctx.quadraticCurveTo(W * 0.3, H * 0.35, W * 0.36, H * 0.52);
      ctx.stroke();
      // The cloudcrust with Lowtown's fallen house on it.
      ctx.fillStyle = 'rgba(176,190,205,0.55)';
      ctx.beginPath();
      ctx.ellipse(W * 0.62, H * 0.66, W * 0.5, H * 0.05, 0, 0, TAU);
      ctx.fill();
      ctx.save();
      ctx.translate(W * 0.66, H * 0.64);
      ctx.rotate(-0.12);
      drawItem(ctx, { k: 'house', s: 1.2, lit: '#ffcf7a' }, 0, 0, W, '#121a24', R);
      ctx.restore();
      ctx.save();
      ctx.translate(W * 0.82, H * 0.65);
      ctx.rotate(0.2);
      drawItem(ctx, { k: 'barn', s: 1.0, noRoof: true }, 0, 0, W, '#121a24', R);
      ctx.restore();
      glow(ctx, W * 0.64, H * 0.6, W * 0.04, '#ffcf7a', 0.6);
      // The Bloom below.
      crystals(ctx, W * 0.72, H * 1.0, W * 0.3, '#ff8a2a', 82, 14);
      crystals(ctx, W * 0.22, H * 1.02, W * 0.18, '#ff7a1a', 83, 8);
      // A lure, mimicking a lantern in the dark.
      glow(ctx, W * 0.12, H * 0.72, W * 0.025, '#ffd27a', 0.9);
      // The diver.
      person(ctx, W * 0.365, H * 0.66, H * 0.13, { helmet: true, color: '#0d1117', visor: '#ffcf7a' });
      glow(ctx, W * 0.39, H * 0.58, W * 0.05, '#ffd27a', 0.7);
    },

    dusk(ctx, W, H) {
      sky(ctx, W, H, [[0, '#121c36'], [0.35, '#3f4a78'], [0.62, '#e2a15b'], [0.85, '#fff0cf'], [1, '#ffffff']], true);
      stars(ctx, W, H, 60, H * 0.9, 91, '#dfe8ff');
      // The Lamp in its cage.
      const lx = W * 0.84, ly = H * 0.16;
      glow(ctx, lx, ly, W * 0.4, '#fff3d0', 0.95);
      rays(ctx, lx, ly, 18, W * 0.5, '#ffe9b8', 0.14, 92);
      ctx.fillStyle = '#ffffff';
      ctx.beginPath();
      ctx.arc(lx, ly, H * 0.06, 0, TAU);
      ctx.fill();
      ctx.strokeStyle = 'rgba(120,80,20,0.7)';
      ctx.lineWidth = Math.max(1, W * 0.0016);
      for (let i = -3; i <= 3; i++) {
        ctx.beginPath();
        ctx.moveTo(lx + i * H * 0.022, ly - H * 0.085);
        ctx.lineTo(lx + i * H * 0.022, ly + H * 0.085);
        ctx.stroke();
      }
      cloudSea(ctx, W, H, H * 0.86, '#2a3558', '#141b33', 93, 0.07);
      // The Stair: islands spiraling up from the dusk into the light.
      const tiers = [
        { x: 0.07, y: 0.83, w: 0.1, grass: '#c9d6e8', rock: '#2a3350', items: [{ k: 'house', at: 0, s: 1.4, color: '#1b2238', lit: '#ffcf7a' }] },
        { x: 0.25, y: 0.7, w: 0.17, grass: '#6f8f45', rock: '#5b4a3e', items: [{ k: 'house', at: -0.5, s: 1 }, { k: 'windmill', at: 0.1, s: 1 }, { k: 'tree', at: 0.5, s: 1 }] },
        { x: 0.42, y: 0.56, w: 0.12, grass: '#d9c27a', rock: '#8f7458', items: [{ k: 'tree', at: 0.3, s: 1.2 }] },
        { x: 0.53, y: 0.42, w: 0.07, grass: '#b8c8c8', rock: '#9a8a7a', items: [{ k: 'spire', at: 0, s: 1.6, h: 0.4 }] },
        { x: 0.63, y: 0.33, w: 0.1, grass: '#e8c56a', rock: '#b0895a', items: [{ k: 'dome', at: -0.2, s: 1.4 }, { k: 'spire', at: 0.4, s: 1.2, h: 0.35 }] },
        { x: 0.72, y: 0.24, w: 0.06, grass: '#fff4dc', rock: '#d8c5a0', items: [] },
      ];
      tiers.forEach((t, i) => {
        island(ctx, W * t.x, H * t.y, W * t.w, { seed: 94 + i, grass: t.grass, rock: t.rock, rockDeep: mix(t.rock, '#000000', 0.5), ink: mix(t.rock, '#000000', 0.55), items: t.items });
      });
      // The Cinder falling onto Skyreach.
      streak(ctx, lx - W * 0.04, ly + H * 0.06, W * 0.27, H * 0.62, '#ff9f1c', Math.max(1.5, W * 0.0025));
      // Kites over the Spires.
      ctx.fillStyle = '#f2e3c4';
      for (const [kx, ky] of [[0.5, 0.27], [0.56, 0.22]]) {
        ctx.beginPath();
        ctx.moveTo(W * kx, H * ky - W * 0.008);
        ctx.lineTo(W * kx + W * 0.005, H * ky);
        ctx.lineTo(W * kx, H * ky + W * 0.008);
        ctx.lineTo(W * kx - W * 0.005, H * ky);
        ctx.fill();
      }
    },

    boomtown(ctx, W, H) {
      sky(ctx, W, H, [[0, '#120d2e'], [0.5, '#2c2160'], [0.85, '#6b3b6f'], [1, '#a5566a']]);
      stars(ctx, W, H, 150, H * 0.6, 101, '#fff4e0');
      firework(ctx, W * 0.2, H * 0.22, W * 0.07, '#6dff7a', 102);
      firework(ctx, W * 0.75, H * 0.18, W * 0.08, '#5ccaff', 103);
      firework(ctx, W * 0.6, H * 0.35, W * 0.05, '#b07cff', 104);
      firework(ctx, W * 0.9, H * 0.4, W * 0.045, '#ff9f1c', 105);
      cloudSea(ctx, W, H, H * 0.76, '#6a4b7a', '#24183a', 106, 0.07);
      airship(ctx, W * 0.08, H * 0.48, W * 0.08, { envelope: '#7a3b2e', stripe: '#e8c06a', ink: '#1a1426', lights: '#ffd27a' });
      airship(ctx, W * 0.93, H * 0.6, W * 0.07, { envelope: '#2e4f7a', stripe: '#d8d0b8', ink: '#1a1426', lights: '#ffd27a', flip: true });
      const x = W * 0.45, y = H * 0.66, w = W * 0.42;
      island(ctx, x, y, w, { seed: 107, grass: '#3f5a33', rock: '#3a2e30', rockDeep: '#150f18', ink: '#120e18', items: [{ k: 'house', at: -0.65, s: 0.8, lit: '#ffd27a' }, { k: 'tree', at: -0.4, s: 0.9 }, { k: 'barn', at: 0.12, s: 1.2, noRoof: true }, { k: 'windmill', at: 0.6, s: 0.8 }] });
      // The barn's roof blowing off, in the blast.
      const bx = x + 0.06 * w, by = y - w * 0.06;
      glow(ctx, bx, by, w * 0.3, '#ffb347', 0.95);
      ctx.fillStyle = '#ffe2a0';
      ctx.beginPath();
      const R = rng(108);
      for (let i = 0; i < 18; i++) {
        const a = (i / 18) * TAU, r = w * (i % 2 ? 0.05 : 0.12) * (0.8 + R() * 0.4);
        ctx.lineTo(bx + Math.cos(a) * r, by + Math.sin(a) * r * 0.8);
      }
      ctx.closePath();
      ctx.fill();
      ctx.save();
      ctx.translate(bx + w * 0.02, by - w * 0.14);
      ctx.rotate(-0.35);
      ctx.fillStyle = '#120e18';
      ctx.beginPath();
      ctx.moveTo(-w * 0.07, 0);
      ctx.lineTo(-w * 0.04, -w * 0.035);
      ctx.lineTo(w * 0.04, -w * 0.035);
      ctx.lineTo(w * 0.07, 0);
      ctx.closePath();
      ctx.fill();
      ctx.restore();
      ctx.fillStyle = '#120e18';
      for (let i = 0; i < 9; i++) {
        ctx.save();
        ctx.translate(bx + (R() - 0.5) * w * 0.4, by - R() * w * 0.25);
        ctx.rotate(R() * TAU);
        ctx.fillRect(-w * 0.012, -w * 0.003, w * 0.024, w * 0.006);
        ctx.restore();
      }
      // A chicken on the weathervane.
      const cx = x + 0.6 * w / 2, cy = y - w * 0.165;
      ctx.beginPath();
      ctx.ellipse(cx, cy, w * 0.008, w * 0.006, 0, 0, TAU);
      ctx.fill();
    },
  };

  function render(canvas, id) {
    const rect = canvas.getBoundingClientRect();
    const dpr = Math.min(window.devicePixelRatio || 1, 2);
    const W = Math.max(1, Math.round(rect.width * dpr));
    const H = Math.max(1, Math.round(rect.height * dpr));
    if (canvas.width !== W || canvas.height !== H) {
      canvas.width = W;
      canvas.height = H;
    }
    const ctx = canvas.getContext('2d');
    ctx.setTransform(1, 0, 0, 1, 0, 0);
    ctx.clearRect(0, 0, W, H);
    (scenes[id] || scenes.lodestone)(ctx, W, H);
  }

  return { render, ids: Object.keys(scenes) };
})();
