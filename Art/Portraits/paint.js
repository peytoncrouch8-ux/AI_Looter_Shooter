// Turns a marched bust into a painted portrait, in the page render.js opens.
// paintPortrait({ passes, size, frame, windowSvg, opts }) -> PNG data URL.
//   passes   data URLs from the march: [colour+coverage, glow/depth/id, normal], at size * supersampling
//   frame    Frame.svg text (drawn over everything), windowSvg the window the painting is clipped to
//   opts     the scene's // PAINT line: background colours, spark and dust colours, glow colour
//
// The painting is a stroke painter after Hertzmann's "Painterly Rendering with Curved Brush Strokes of
// Multiple Sizes" (1998): big strokes first, then smaller ones only where the canvas still differs from
// the picture, each stroke following the picture's edges. The focal areas (eyes, skin, silhouette edges)
// then get part of the crisp render back, as a painter tightens the face and leaves the coat loose.
async function paintPortrait({ passes, size, frame, windowSvg, opts }) {
	const S = size;
	const k = S / 1024;     // every radius below is for a 1024 square
	let seed = 1234567;
	const rnd = () => { seed |= 0; seed = seed + 0x6D2B79F5 | 0; let t = Math.imul(seed ^ seed >>> 15, 1 | seed); t = t + Math.imul(t ^ t >>> 7, 61 | t) ^ t; return ((t ^ t >>> 14) >>> 0) / 4294967296; };
	const load = src => new Promise((res, rej) => { const i = new Image(); i.onload = () => res(i); i.onerror = rej; i.src = src; });
	const svgUrl = text => URL.createObjectURL(new Blob([text], { type: 'image/svg+xml' }));
	const mk = (w = S, h = S) => { const c = document.createElement('canvas'); c.width = w; c.height = h; return c; };
	const ctx = c => c.getContext('2d', { willReadFrequently: true });
	const hex = h => [parseInt(h.slice(1, 3), 16), parseInt(h.slice(3, 5), 16), parseInt(h.slice(5, 7), 16)];
	const rgba = (c, a) => `rgba(${c[0] | 0},${c[1] | 0},${c[2] | 0},${a})`;
	const o = Object.assign({ bgTop: '#0d1622', bgBottom: '#05070b', bgGlow: '#2a3f57', glowAt: [0.7, 0.3], sparks: '#ff9f1c', dust: '#7fb6d6', glow: '#5ad8ff' }, opts);

	const imgs = await Promise.all(passes.map(load));
	const bust = mk(); const bc = ctx(bust); bc.imageSmoothingQuality = 'high'; bc.drawImage(imgs[0], 0, 0, S, S);
	const aux = mk(); const ac = ctx(aux); ac.imageSmoothingQuality = 'high'; ac.drawImage(imgs[1], 0, 0, S, S);
	const auxD = ac.getImageData(0, 0, S, S).data;
	const bustD = bc.getImageData(0, 0, S, S).data;

	// ---- background: a dark sky with a soft glow behind the head, painted in big soft dabs
	const bg = mk(); const g = ctx(bg);
	const lin = g.createLinearGradient(0, 0, 0, S); lin.addColorStop(0, o.bgTop); lin.addColorStop(1, o.bgBottom);
	g.fillStyle = lin; g.fillRect(0, 0, S, S);
	const gl = hex(o.bgGlow);
	const rad = g.createRadialGradient(o.glowAt[0] * S, o.glowAt[1] * S, 0, o.glowAt[0] * S, o.glowAt[1] * S, S * 0.65);
	rad.addColorStop(0, rgba(gl, 0.95)); rad.addColorStop(0.45, rgba(gl, 0.35)); rad.addColorStop(1, rgba(gl, 0));
	g.fillStyle = rad; g.fillRect(0, 0, S, S);
	for (let i = 0; i < 260; i++) {
		const x = rnd() * S, y = rnd() * S, r = (30 + rnd() * 90) * k;
		const tint = rnd() < 0.5 ? hex(o.bgTop) : gl;
		g.fillStyle = rgba(tint.map(v => v * (0.8 + rnd() * 0.5)), 0.10);
		g.beginPath(); g.ellipse(x, y, r, r * (0.4 + rnd() * 0.4), rnd() * 3.14, 0, 6.29); g.fill();
	}
	// Far bokeh and dust: soft discs, cool and warm.
	const dust = hex(o.dust), spark = hex(o.sparks);
	for (let i = 0; i < 70; i++) {
		const x = rnd() * S, y = rnd() * S * 0.95, r = (3 + rnd() * 14) * k;
		const c = rnd() < 0.65 ? dust : spark;
		const rg = g.createRadialGradient(x, y, 0, x, y, r);
		rg.addColorStop(0, rgba(c, 0.25 + rnd() * 0.3)); rg.addColorStop(0.7, rgba(c, 0.12)); rg.addColorStop(1, rgba(c, 0));
		g.fillStyle = rg; g.beginPath(); g.arc(x, y, r, 0, 6.29); g.fill();
	}
	g.filter = `blur(${2 * k}px)`; g.drawImage(bg, 0, 0); g.filter = 'none';

	// ---- the picture to paint: background + bust
	const ref = mk(); const rc = ctx(ref);
	rc.drawImage(bg, 0, 0); rc.drawImage(bust, 0, 0);
	// The lower body sinks into the dark, as in a painted bust.
	const fade = rc.createLinearGradient(0, S * 0.70, 0, S);
	fade.addColorStop(0, rgba(hex(o.bgBottom), 0)); fade.addColorStop(1, rgba(hex(o.bgBottom), 0.85));
	rc.fillStyle = fade; rc.fillRect(0, S * 0.7, S, S * 0.3);
	const refD = rc.getImageData(0, 0, S, S).data;

	// ---- direction field: the smoothed structure tensor of the picture's luminance
	const N = S * S;
	const lum = new Float32Array(N);
	for (let i = 0; i < N; i++) lum[i] = 0.3 * refD[i * 4] + 0.59 * refD[i * 4 + 1] + 0.11 * refD[i * 4 + 2];
	const boxBlur = (src, r) => {
		const tmp = new Float32Array(N), out = new Float32Array(N);
		for (let y = 0; y < S; y++) {
			let acc = 0; const row = y * S;
			for (let x = -r; x <= r; x++) acc += src[row + Math.min(S - 1, Math.max(0, x))];
			for (let x = 0; x < S; x++) {
				tmp[row + x] = acc / (2 * r + 1);
				acc += src[row + Math.min(S - 1, x + r + 1)] - src[row + Math.max(0, x - r)];
			}
		}
		for (let x = 0; x < S; x++) {
			let acc = 0;
			for (let y = -r; y <= r; y++) acc += tmp[Math.min(S - 1, Math.max(0, y)) * S + x];
			for (let y = 0; y < S; y++) {
				out[y * S + x] = acc / (2 * r + 1);
				acc += tmp[Math.min(S - 1, y + r + 1) * S + x] - tmp[Math.max(0, y - r) * S + x];
			}
		}
		return out;
	};
	const bl = boxBlur(boxBlur(lum, Math.max(1, Math.round(2 * k))), Math.max(1, Math.round(2 * k)));
	const exx = new Float32Array(N), eyy = new Float32Array(N), exy = new Float32Array(N), gmag = new Float32Array(N);
	for (let y = 1; y < S - 1; y++) for (let x = 1; x < S - 1; x++) {
		const i = y * S + x;
		const gx = (bl[i + 1 - S] + 2 * bl[i + 1] + bl[i + 1 + S]) - (bl[i - 1 - S] + 2 * bl[i - 1] + bl[i - 1 + S]);
		const gy = (bl[i + S - 1] + 2 * bl[i + S] + bl[i + S + 1]) - (bl[i - S - 1] + 2 * bl[i - S] + bl[i - S + 1]);
		exx[i] = gx * gx; eyy[i] = gy * gy; exy[i] = gx * gy; gmag[i] = Math.sqrt(gx * gx + gy * gy);
	}
	const tr = Math.max(2, Math.round(6 * k));
	const sxx = boxBlur(boxBlur(exx, tr), tr), syy = boxBlur(boxBlur(eyy, tr), tr), sxy = boxBlur(boxBlur(exy, tr), tr);
	// Tangent (along the edges): the eigenvector of the smaller eigenvalue.
	const tangent = i => {
		const a = sxx[i], b = sxy[i], d = syy[i];
		const l2 = 0.5 * (a + d - Math.sqrt((a - d) * (a - d) + 4 * b * b));
		let tx = b, ty = l2 - a;
		if (Math.abs(tx) + Math.abs(ty) < 1e-6) { tx = l2 - d; ty = b; }
		if (Math.abs(tx) + Math.abs(ty) < 1e-6) return [0.6, 0.8];
		const l = Math.hypot(tx, ty); return [tx / l, ty / l];
	};

	// ---- the strokes
	const can = mk(); const cc = ctx(can);
	cc.filter = `blur(${10 * k}px)`; cc.drawImage(ref, 0, 0); cc.filter = 'none';
	const layers = [{ r: 14, len: 9, T: 26, a: 0.85 }, { r: 7, len: 8, T: 22, a: 0.85 }, { r: 3.5, len: 6, T: 20, a: 0.8 }, { r: 1.8, len: 5, T: 22, a: 0.7 }];
	for (const L of layers) {
		const R = Math.max(1, L.r * k);
		const blurred = mk(); const blc = ctx(blurred);
		blc.filter = `blur(${R * 0.5}px)`; blc.drawImage(ref, 0, 0); blc.filter = 'none';
		const B = blc.getImageData(0, 0, S, S).data;
		const C = cc.getImageData(0, 0, S, S).data;
		const diff = i => Math.abs(C[i] - B[i]) + Math.abs(C[i + 1] - B[i + 1]) + Math.abs(C[i + 2] - B[i + 2]);
		const strokes = [];
		const step = Math.max(1, Math.round(R));
		for (let gy = 0; gy < S; gy += step) for (let gx = 0; gx < S; gx += step) {
			let err = 0, best = -1, bx = gx, by = gy, n = 0;
			for (let sy = 0; sy < step; sy += Math.max(1, step >> 2)) for (let sx = 0; sx < step; sx += Math.max(1, step >> 2)) {
				const x = Math.min(S - 1, gx + sx), y = Math.min(S - 1, gy + sy);
				const e = diff((y * S + x) * 4); err += e; n++;
				if (e > best) { best = e; bx = x; by = y; }
			}
			if (err / n < L.T) continue;
			const i0 = (by * S + bx) * 4;
			const col = [B[i0], B[i0 + 1], B[i0 + 2]];
			const pts = [[bx, by]];
			let x = bx, y = by, px = 0, py = 0;
			for (let s = 1; s < L.len; s++) {
				let [tx, ty] = tangent((Math.round(y) * S + Math.round(x)));
				if (tx * px + ty * py < 0) { tx = -tx; ty = -ty; }
				if (s > 1) { tx = 0.6 * tx + 0.4 * px; ty = 0.6 * ty + 0.4 * py; const l = Math.hypot(tx, ty) || 1; tx /= l; ty /= l; }
				x += tx * R; y += ty * R; px = tx; py = ty;
				if (x < 0 || y < 0 || x >= S || y >= S) break;
				const j = (Math.round(y) * S + Math.round(x)) * 4;
				const dRef = Math.abs(B[j] - col[0]) + Math.abs(B[j + 1] - col[1]) + Math.abs(B[j + 2] - col[2]);
				const dCan = Math.abs(C[j] - col[0]) + Math.abs(C[j + 1] - col[1]) + Math.abs(C[j + 2] - col[2]);
				if (s > 2 && dRef > dCan) break;
				pts.push([x, y]);
			}
			strokes.push({ pts, col });
		}
		for (let i = strokes.length - 1; i > 0; i--) { const j = (rnd() * (i + 1)) | 0; [strokes[i], strokes[j]] = [strokes[j], strokes[i]]; }
		cc.lineCap = 'round'; cc.lineJoin = 'round';
		for (const st of strokes) {
			const v = 0.94 + rnd() * 0.12;
			const path = () => {
				cc.beginPath(); cc.moveTo(st.pts[0][0], st.pts[0][1]);
				if (st.pts.length === 1) cc.lineTo(st.pts[0][0] + 0.1, st.pts[0][1]);
				for (let p = 1; p < st.pts.length; p++) cc.lineTo(st.pts[p][0], st.pts[p][1]);
			};
			cc.globalAlpha = L.a; cc.strokeStyle = rgba(st.col.map(c => Math.min(255, c * v)), 1); cc.lineWidth = R * (1.5 + rnd() * 0.5);
			path(); cc.stroke();
			// Bristle marks: a few thin lighter and darker lines inside the stroke.
			if (R > 2) {
				cc.lineWidth = Math.max(1, R * 0.25);
				for (let b = 0; b < 2; b++) {
					const off = (rnd() - 0.5) * R;
					const shade = rnd() < 0.5 ? 0.85 : 1.12;
					cc.globalAlpha = 0.22; cc.strokeStyle = rgba(st.col.map(c => Math.min(255, c * shade)), 1);
					cc.save(); cc.translate(off * 0.7, off * 0.7); path(); cc.stroke(); cc.restore();
				}
			}
		}
		cc.globalAlpha = 1;
	}

	// ---- tighten the focal areas: give back part of the crisp picture on the face, eyes and edges
	const out = cc.getImageData(0, 0, S, S);
	const OD = out.data;
	const focus = new Float32Array(N);
	for (let i = 0; i < N; i++) {
		const id = Math.round(auxD[i * 4 + 2] / 255 * 16), cov = bustD[i * 4 + 3] / 255;
		let f = 0.12;
		if (id === 1 || id === 2) f = 0.62;          // skin, eyes
		else if (id === 5 || id === 9) f = 0.30;     // bandana, hair
		else if (id === 8 || id === 11 || id === 4) f = 0.45; // brass, lead, leather details
		focus[i] = f * cov + Math.min(0.35, gmag[i] / 400);
	}
	const fb = boxBlur(focus, Math.max(1, Math.round(4 * k)));
	for (let i = 0; i < N; i++) {
		const f = Math.min(0.85, fb[i]);
		for (let c = 0; c < 3; c++) OD[i * 4 + c] = OD[i * 4 + c] * (1 - f) + refD[i * 4 + c] * f;
	}
	cc.putImageData(out, 0, 0);

	// ---- glow: the eyes (and anything else marked) bloom
	const glow = mk(); const glc = ctx(glow);
	const gD = glc.createImageData(S, S);
	for (let i = 0; i < N; i++) {
		const m = auxD[i * 4] / 255 * (bustD[i * 4 + 3] / 255);
		gD.data[i * 4] = bustD[i * 4] * m; gD.data[i * 4 + 1] = bustD[i * 4 + 1] * m; gD.data[i * 4 + 2] = bustD[i * 4 + 2] * m; gD.data[i * 4 + 3] = 255;
	}
	glc.putImageData(gD, 0, 0);
	cc.globalCompositeOperation = 'lighter';
	for (const [r, a] of [[3, 0.9], [10, 0.7], [28, 0.55], [70, 0.35]]) {
		cc.globalAlpha = a; cc.filter = `blur(${r * k}px)`; cc.drawImage(glow, 0, 0);
	}
	cc.filter = 'none'; cc.globalAlpha = 1;

	// ---- embers on the wind: crisp sparks, some streaked, a few big out-of-focus ones in front
	const sp = hex(o.sparks);
	for (let i = 0; i < 40; i++) {
		const x = rnd() * S, y = rnd() * S;
		// keep the face clear: embers drift round the bust's edges
		if (bustD[(Math.round(y) * S + Math.round(x)) * 4 + 3] > 0 && rnd() < 0.85) continue;
		const r = (0.7 + rnd() * 1.6) * k, len = (4 + rnd() * 18) * k, ang = -0.5 - rnd() * 0.6;
		const rg = cc.createRadialGradient(x, y, 0, x, y, r * 6);
		rg.addColorStop(0, rgba([255, 230, 170], 0.9)); rg.addColorStop(0.2, rgba(sp, 0.6)); rg.addColorStop(1, rgba(sp, 0));
		cc.fillStyle = rg; cc.beginPath(); cc.arc(x, y, r * 6, 0, 6.29); cc.fill();
		cc.strokeStyle = rgba([255, 210, 140], 0.5); cc.lineWidth = r; cc.beginPath(); cc.moveTo(x, y); cc.lineTo(x - Math.cos(ang) * len, y - Math.sin(ang) * len); cc.stroke();
	}
	cc.globalCompositeOperation = 'screen';
	for (let i = 0; i < 4; i++) {
		const x = rnd() < 0.5 ? rnd() * S * 0.18 : S * (0.82 + rnd() * 0.18), y = S * (0.15 + rnd() * 0.8), r = (14 + rnd() * 26) * k;
		const rg = cc.createRadialGradient(x, y, 0, x, y, r);
		rg.addColorStop(0, rgba(sp, 0.22)); rg.addColorStop(0.8, rgba(sp, 0.10)); rg.addColorStop(1, rgba(sp, 0));
		cc.fillStyle = rg; cc.beginPath(); cc.arc(x, y, r, 0, 6.29); cc.fill();
	}
	cc.globalCompositeOperation = 'source-over';

	// ---- grade: vignette and a fine canvas grain
	const vg = cc.createRadialGradient(S * 0.5, S * 0.42, S * 0.25, S * 0.5, S * 0.5, S * 0.75);
	vg.addColorStop(0, 'rgba(0,0,0,0)'); vg.addColorStop(1, 'rgba(0,0,0,0.55)');
	cc.fillStyle = vg; cc.fillRect(0, 0, S, S);
	const gr = cc.getImageData(0, 0, S, S);
	for (let i = 0; i < N; i++) { const n = (rnd() - 0.5) * 10; gr.data[i * 4] += n; gr.data[i * 4 + 1] += n; gr.data[i * 4 + 2] += n; }
	cc.putImageData(gr, 0, 0);

	// ---- clip to the frame's window and lay the frame over
	const fin = mk(); const fc = ctx(fin);
	fc.fillStyle = '#12131a'; fc.fillRect(0, 0, S, S);
	const lay = mk(); const lc = ctx(lay);
	lc.drawImage(can, 0, 0);
	if (windowSvg) { lc.globalCompositeOperation = 'destination-in'; lc.drawImage(await load(svgUrl(windowSvg)), 0, 0, S, S); lc.globalCompositeOperation = 'source-over'; }
	fc.drawImage(lay, 0, 0);
	if (frame) fc.drawImage(await load(svgUrl(frame)), 0, 0, S, S);
	return fin.toDataURL('image/png');
}
