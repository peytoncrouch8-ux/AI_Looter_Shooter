// Concept E, the user's mix of C "Tag Rack" and D "Spur Rowel" with the main menu's dial (main_menu.png; the game draws
// it in UI/Menus/MainMenuArt.cpp LogoMark and UI/Style/LooterReticleArt.cpp Halo / ArcScale / SmallReticle):
// a hairline salt ring with ticks every 10 degrees (long at the cross), the Spur Rowel logo inside (ten slim points, a
// gold hub), and a thin range arc with ticks, the option numbers past it. The options are C's tags hanging off the arc;
// the rowel turns one click per option so a gold point aims at the lit tag. Choosing an option opens a full page
// (the user, 2026-10-30: "a page instead of a popup on the right"): MODE 'settings' draws the Settings page.
// VARIANT 'E1': the dial whole at the left, the tags laid along the arc. 'E2': the dial big at the left edge, partly
// off screen, the tags stacked straight, the arc's ticks feeding each one.
const SET = MODE === 'settings';
const E2 = VARIANT.startsWith('E2');
// 'b' versions (the user, 2026-10-30: "the sun symbol needs to go and the dial to fit the removal of the symbol"): no Spur
// Rowel; an open centre with a fine inner ring and a slim gold needle on a pivot, aimed at the lit option.
const NEEDLE = VARIANT.endsWith('b');
const SEL = E2 ? 1 : 0;
document.body.innerHTML = '<div class="world"></div><div class="vignette"></div>';

const rad = d => d * Math.PI / 180;
const on = (cx, cy, r, deg) => [cx + Math.cos(rad(deg)) * r, cy + Math.sin(rad(deg)) * r];
const ptsStr = a => a.map(p => p.map(v => +v.toFixed(2)).join(',')).join(' ');
// A line with the kit's dark edge under it (AddEdgedLine): the edge, then the line.
function edged(d, w, col, op = 1, extra = '') {
	return `<path d="${d}" fill="none" stroke="rgba(8,10,13,.85)" stroke-opacity="${.7 * op}" stroke-width="${w + 3}" stroke-linecap="round" ${extra}/>
		<path d="${d}" fill="none" stroke="${col}" stroke-opacity="${op}" stroke-width="${w}" stroke-linecap="round" ${extra}/>`;
}
const lineD = (a, b) => `M${a[0].toFixed(2)},${a[1].toFixed(2)} L${b[0].toFixed(2)},${b[1].toFixed(2)}`;
function arcD(cx, cy, r, a0, a1) {
	const p0 = on(cx, cy, r, a0), p1 = on(cx, cy, r, a1);
	return `M${p0[0].toFixed(2)},${p0[1].toFixed(2)} A${r},${r} 0 ${Math.abs(a1 - a0) > 180 ? 1 : 0} ${a1 > a0 ? 1 : 0} ${p1[0].toFixed(2)},${p1[1].toFixed(2)}`;
}

// --- The main menu's dial: LooterReticle::Halo round MainMenuArt::LogoMark, at any size ---
const ROWEL = [[-1.77, -8.31], [0, -21], [1.77, -8.31], [3.46, -7.77], [12.34, -16.99], [6.32, -5.69], [7.36, -4.25], [19.97, -6.49], [8.45, -0.89], [8.45, 0.89], [19.97, 6.49], [7.36, 4.25], [6.32, 5.69], [12.34, 16.99], [3.46, 7.77], [1.77, 8.31], [0, 21], [-1.77, 8.31], [-3.46, 7.77], [-12.34, 16.99], [-6.32, 5.69], [-7.36, 4.25], [-19.97, 6.49], [-8.45, 0.89], [-8.45, -0.89], [-19.97, -6.49], [-7.36, -4.25], [-6.32, -5.69], [-12.34, -16.99], [-3.46, -7.77]];
const circlePts = (r, n = 36) => Array.from({ length: n }, (_, i) => [Math.cos(i / n * 2 * Math.PI) * r, Math.sin(i / n * 2 * Math.PI) * r]);
function emblemSVG(cx, cy, R, o = {}) {
	const s = R / 34;
	// Turn the rowel so one of its points (at -90 + 36k) aims at the lit angle: the dial clicks one point per option.
	let rot = 0;
	if (o.aim !== undefined) { const k = Math.round((o.aim + 90) / 36); rot = o.aim - (-90 + 36 * k); }
	const map = (p, r = rot) => { const a = rad(r), x = p[0] * s, y = p[1] * s; return [cx + x * Math.cos(a) - y * Math.sin(a), cy + x * Math.sin(a) + y * Math.cos(a)]; };
	const gap = Math.max(4, R * .05), len = Math.max(6, R * .07), extra = Math.max(7, R * .09);
	const ringW = R >= 80 ? 3 : 2.5;
	let g = edged(arcD(cx, cy, R, 0, 359.99), ringW, C.salt);
	let shorts = '', longs = '';
	for (let i = 0; i < 36; i++) {
		const d = i * 10, long = i % 9 === 0;
		const a = on(cx, cy, R + gap, d), b = on(cx, cy, R + gap + len + (long ? extra : 0), d);
		(long ? (longs += lineD(a, b) + ' ') : (shorts += lineD(a, b) + ' '));
	}
	g += edged(shorts, R >= 80 ? 2 : 1.6, C.salt, .85) + edged(longs, R >= 80 ? 2.6 : 2, C.salt);
	if (o.inner) {
		// Big dials only: a faint inner scale between the rowel's points and the ring, so the size doesn't read empty.
		let t = '';
		for (let i = 0; i < 72; i++) { const d = i * 5; t += lineD(on(cx, cy, R * .86, d), on(cx, cy, R * (i % 2 ? .9 : .93), d)) + ' '; }
		g += `<path d="${t}" stroke="${C.salt}" stroke-opacity=".32" stroke-width="1.2"/>`;
		g += `<circle cx="${cx}" cy="${cy}" r="${R * .84}" fill="none" stroke="${C.salt}" stroke-opacity=".22" stroke-width="1.2"/>`;
	}
	if (NEEDLE) return g + needleCentre(cx, cy, R, o);
	// The rowel in salt with the hub's ring cut out (even-odd), then the gold hub with its axle hole.
	const star = ptsStr(ROWEL.map(p => map(p))), cut = ptsStr(circlePts(6.2).map(p => map(p)));
	g += `<path d="M${star.replace(/ /g, ' L')} Z M${cut.replace(/ /g, ' L')} Z" fill="${C.salt}" fill-rule="evenodd" stroke="rgba(8,10,13,.8)" stroke-width="1.5" paint-order="stroke"/>`;
	if (o.aim !== undefined) {
		// The point aiming at the lit option, gold, and the halo's gold arc inside the ring round it.
		const tri = ptsStr([[-1.77, -8.31], [0, -21], [1.77, -8.31]].map(p => map(p, o.aim + 90)));
		g += `<polygon points="${tri}" fill="${C.gold}" stroke="${C.gold}" stroke-width="1.5" stroke-linejoin="round"/>`;
		g += edged(arcD(cx, cy, R * .875 * (o.inner ? .9 : 1), o.aim - 14, o.aim + 14), Math.max(3, R * .03), C.gold);
	}
	const hub = ptsStr(circlePts(4.2).map(p => map(p))), hole = ptsStr(circlePts(1.4, 16).map(p => map(p)));
	g += `<path d="M${hub.replace(/ /g, ' L')} Z M${hole.replace(/ /g, ' L')} Z" fill="${C.gold}" fill-rule="evenodd"/>`;
	return g;
}
// The open centre that replaces the rowel: a fine inner ring with ticks pointing in (long at the cross), the halo's gold
// arc beside the aim, and a slim needle on a pivot: gold toward the lit option, a short salt tail behind.
function needleCentre(cx, cy, R, o) {
	const aim = o.aim ?? 0;
	const ri = R * .64;
	let g = edged(arcD(cx, cy, ri, 0, 359.99), 1.6, C.salt, .7);
	let sh = '', lo = '';
	for (let i = 0; i < 72; i++) {
		const d = i * 5, cross = i % 18 === 0, big = i % 2 === 0;
		const seg = lineD(on(cx, cy, ri, d), on(cx, cy, ri - (cross ? R * .13 : big ? R * .06 : R * .035), d)) + ' ';
		cross ? (lo += seg) : (sh += seg);
	}
	g += `<path d="${sh}" stroke="${C.salt}" stroke-opacity=".45" stroke-width="${R >= 80 ? 1.4 : 1.1}"/>`;
	g += edged(lo, R >= 80 ? 2 : 1.6, C.salt, .9);
	g += edged(arcD(cx, cy, R * .875 * (o.inner ? .9 : 1), aim - 14, aim + 14), Math.max(3, R * .03), C.gold);
	// The needle: a long thin diamond, its front gold to just short of the ring, the tail salt.
	const w = Math.max(2.8, R * .038), tip = R * .9, tail = R * .26;
	const P = (along, side) => { const a = rad(aim); return [cx + Math.cos(a) * along - Math.sin(a) * side, cy + Math.sin(a) * along + Math.cos(a) * side]; };
	g += `<polygon points="${ptsStr([P(0, -w), P(tip, 0), P(0, w)])}" fill="${C.gold}" stroke="rgba(8,10,13,.8)" stroke-width="1.5" paint-order="stroke" stroke-linejoin="round"/>`;
	g += `<polygon points="${ptsStr([P(0, -w), P(-tail, 0), P(0, w)])}" fill="${C.salt}" stroke="rgba(8,10,13,.8)" stroke-width="1.5" paint-order="stroke" stroke-linejoin="round"/>`;
	// The pivot: a hollow salt ring with a gold dot.
	const pr = Math.max(6, R * .075);
	g += `<circle cx="${cx}" cy="${cy}" r="${pr}" fill="${C.glass2}" stroke="${C.salt}" stroke-width="${R >= 80 ? 2.5 : 2}"/>`;
	g += `<circle cx="${cx}" cy="${cy}" r="${Math.max(2.2, pr * .38)}" fill="${C.gold}"/>`;
	return g;
}
// LooterReticle::SmallReticle: four quarter arcs, cross ticks, a middle dot.
function smallReticle(cx, cy, r, col) {
	let g = '';
	for (let q = 0; q < 4; q++) {
		g += edged(arcD(cx, cy, r, q * 90 + 14, q * 90 + 76), 3, col);
		g += edged(lineD(on(cx, cy, r + 5, q * 90), on(cx, cy, Math.max(r - 7, 2), q * 90)), 2.5, col);
	}
	return g + `<circle cx="${cx}" cy="${cy}" r="3.5" fill="${col}"/>`;
}
// The range arc (LooterReticle::ArcScale): a thin arc, ticks pointing in toward its centre.
function arcScale(cx, cy, r, a0, a1, o = {}) {
	let g = edged(arcD(cx, cy, r, a0, a1), 2, C.salt, .7);
	const step = (o.step ?? 10.7) / r * 180 / Math.PI;
	let sh = '', lo = '';
	for (let i = 0, d = a0; d <= a1 + 1e-6; i++, d += step) {
		const long = i % 5 === 0;
		const seg = lineD(on(cx, cy, r, d), on(cx, cy, r - (long ? 12 : 6), d)) + ' ';
		long ? (lo += seg) : (sh += seg);
	}
	return g + `<path d="${sh}" stroke="${C.salt}" stroke-opacity=".45" stroke-width="1.3"/><path d="${lo}" stroke="${C.salt}" stroke-opacity=".7" stroke-width="1.8"/>`;
}

// --- C's shapes ---
function hTagPts(w, h) {
	const m = h / 2;
	return [[34, 0], [w - 42, 0], [w - 18, 18], [w, m], [w - 18, h - 18], [w - 42, h], [34, h], [12, h - 22], [12, m + 28], [0, m + 18], [0, m - 18], [12, m - 28], [12, 22]];
}
function vTagPts(w, h, o = {}) {
	const n = o.notch ?? 70, cut = 26, pt = o.point ?? 28, top = 12, c = w / 2;
	const pts = [[0, top + cut], [cut, top], [c - n - 12, top], [c - n, 0], [c + n, 0], [c + n + 12, top], [w - cut, top], [w, top + cut]];
	if (pt) pts.push([w, h - pt - cut], [w - cut, h - pt], [c, h], [cut, h - pt], [0, h - pt - cut]);
	else pts.push([w, h - cut], [w - cut, h], [cut, h], [0, h - cut]);
	return pts;
}
const inset = (pts, d = 2) => ptsStr(pts.map(p => [p[0] + d, p[1] + d]));
function gemIcon(size, iconName, lit, col, iconSize = 22) {
	const h = size / 2;
	return `<div style="position:relative;width:${size}px;height:${size}px"><svg class="abs" style="left:0;top:0" width="${size}" height="${size}">
		<polygon points="${h},2 ${size - 2},${h} ${h},${size - 2} 2,${h}" fill="${lit ? 'rgba(66,58,40,1)' : C.glass2}" stroke="${col}" stroke-width="3.5"/></svg>
		<div class="abs" style="left:${h - iconSize / 2}px;top:${h - iconSize / 2}px">${icon(iconName, iconSize, col)}</div></div>`;
}
const ornament = w => `<svg width="${w}" height="14" style="display:block"><line x1="0" y1="7" x2="${w / 2 - 16}" y2="7" stroke="${C.salt}" stroke-opacity=".5" stroke-width="1.5"/>
	<polygon points="${w / 2},1 ${w / 2 + 6},7 ${w / 2},13 ${w / 2 - 6},7" fill="none" stroke="${C.gold}" stroke-width="2"/><line x1="${w / 2 + 16}" y1="7" x2="${w}" y2="7" stroke="${C.salt}" stroke-opacity=".5" stroke-width="1.5"/></svg>`;
Object.assign(ICON, {
	monitor: c => `<rect x="2.5" y="4" width="19" height="12.5" rx="1" fill="none" stroke="${c}" stroke-width="1.9"/><path d="M9 20.5h6M12 16.5v4" stroke="${c}" stroke-width="1.9"/>`,
	speaker: c => `<path d="M3.5 9h4l5-4v14l-5-4h-4z" fill="${c}"/><path d="M15.5 8.5a5 5 0 0 1 0 7M18.2 5.8a8.8 8.8 0 0 1 0 12.4" fill="none" stroke="${c}" stroke-width="1.9" stroke-linecap="round"/>`,
	layers: c => `<path d="M12 3l9.5 5-9.5 5-9.5-5z" fill="none" stroke="${c}" stroke-width="1.9" stroke-linejoin="round"/><path d="M2.5 12.5l9.5 5 9.5-5M2.5 16.5l9.5 5 9.5-5" fill="none" stroke="${c}" stroke-width="1.9" stroke-linejoin="round"/>`,
	keys: c => `<rect x="2" y="6" width="20" height="12.5" rx="2" fill="none" stroke="${c}" stroke-width="1.9"/><path d="M5.5 9.5h1.5M9.5 9.5H11M13.5 9.5H15M17.2 9.5h1.3M5.5 12.5h1.5M17.2 12.5h1.3M8 15.5h8" stroke="${c}" stroke-width="1.9" stroke-linecap="round"/>`,
});
const TAB_ICON = ['monitor', 'speaker', 'layers', 'keys'];

// --- The page shade: the main menu's (MainMenuArt::Shade: solid under the menu's side, then thinning) ---
place('', `left:0;top:0;width:1920px;height:1080px;background:${SET
	? 'linear-gradient(90deg, rgba(6,7,9,.82) 0%, rgba(6,7,9,.7) 50%, rgba(6,7,9,.62) 100%)'
	: 'linear-gradient(90deg, rgba(6,7,9,.86) 0%, rgba(6,7,9,.74) 34%, rgba(6,7,9,.22) 62%, rgba(6,7,9,.34) 100%)'}`);

// The main menu's lockup: the title over a ruler (ticks hanging), a small spaced line under it.
function lockup(title, sub, o = {}) {
	return `<div class="out" style="font-weight:700;font-size:${o.size ?? 52}px;letter-spacing:.24em;line-height:1">${title}</div>
		<div style="margin-top:12px">${rulerSVG(o.ruler ?? 430, { step: 10.7, major: 5, opacity: .6, long: 12, short: 6 })}</div>
		<div style="margin-top:4px;font-weight:700;font-size:17px;letter-spacing:.5em">${sub}</div>`;
}

// ===================================================================================================================
// The hub
// ===================================================================================================================
if (!SET) {
	place(lockup('GAME PAUSED', `<span style="color:${C.label}">${DATA.area}</span>`), 'left:96px;top:86px');

	// Geometry: the dial's centre and radius, the range arc's radius, each row's point on the arc.
	const G = E2
		? { cx: 40, cy: 560, R: 232, RA: 336, a0: -58, a1: 58, rows: [328, 448, 568, 712], tagX: 476, tagW: 430, inner: true }
		: { cx: 200, cy: 556, R: 150, RA: 320, a0: -68, a1: 68, angles: [-54, -18, 18, 54], tagW: 430 };
	const pts = DATA.options.map((_, i) => {
		if (!E2) return on(G.cx, G.cy, G.RA, G.angles[i]);
		const dy = G.rows[i] - G.cy;
		return [G.cx + Math.sqrt(G.RA * G.RA - dy * dy), G.rows[i]];
	});
	const aim = (() => { const [x, y] = pts[SEL]; return Math.atan2(y - G.cy, x - G.cx) * 180 / Math.PI; })();

	let s = emblemSVG(G.cx, G.cy, G.R, { aim, inner: G.inner });
	s += arcScale(G.cx, G.cy, G.RA, G.a0, G.a1);
	// The gold lead from the ring to the lit point on the arc, the gold run on the arc round it, the small reticle.
	const gap = Math.max(4, G.R * .05) + Math.max(6, G.R * .07) + 6;
	const [lx, ly] = pts[SEL];
	s += edged(lineD(on(G.cx, G.cy, G.R + gap, aim), on(G.cx, G.cy, G.RA - 28, aim)), 2, C.gold);
	s += edged(arcD(G.cx, G.cy, G.RA, aim - 32 / G.RA * 57.3, aim + 32 / G.RA * 57.3), 5, C.gold);
	s += smallReticle(lx, ly, 19, C.gold);
	place(`<svg width="1920" height="1080">${s}</svg>`, 'left:0;top:0');

	// The rows: a salt dash across the arc, the number past it, a peg out to the tag's gem.
	const TH = 76;
	DATA.options.forEach((o, i) => {
		const sel = i === SEL, danger = !!o.danger;
		const [x, y] = pts[i];
		const col = sel ? C.gold : danger ? C.red : C.salt;
		const txt = sel ? C.gold : danger ? C.red : C.text;
		const tx = (E2 ? G.tagX : x + 86) + (sel ? 26 : 0), w = G.tagW + (sel ? 30 : 0);
		let peg = '';
		if (!sel) peg += edged(lineD([x - 13, y], [x + 13, y]), 2, C.salt, .9);
		peg += edged(lineD([x + (sel ? 26 : 13), y], [x + 30, y]), 2, col, .8);
		peg += edged(lineD([x + 62, y], [tx + 2, y]), sel ? 2.5 : 2, col, sel ? 1 : .7);
		document.body.insertAdjacentHTML('beforeend', `<svg class="abs" style="left:0;top:0" width="1920" height="1080">${peg}</svg>`);
		place(`<span class="out" style="font-weight:700;font-size:19px;letter-spacing:.07em;color:${sel ? C.gold : danger ? C.red : C.label}">0${i + 1}</span>`,
			`left:${x + (sel ? 40 : 34)}px;top:${y - 13}px`);
		const svg = panelSVG(w, TH, {
			points: inset(hTagPts(w - 4, TH - 4)), sw: sel ? 4 : 3, stroke: col, strokeOpacity: sel ? 1 : danger ? .75 : .55, horizontal: true,
			top: sel ? 'rgba(255,230,166,.30)' : danger ? 'rgba(122,34,25,.6)' : C.glass, bottom: sel ? 'rgba(30,34,40,.9)' : C.glass2,
			extra: sel ? `<circle cx="${w - 46}" cy="14" r="3.5" fill="${C.gold}"/><circle cx="${w - 46}" cy="${TH - 14}" r="3.5" fill="${C.gold}"/>` : '',
		});
		place(`${svg}<div class="abs" style="left:-17px;top:${TH / 2 - 26}px">${gemIcon(52, OPTION_ICON[i], sel, col)}</div>
			<div class="abs" style="left:62px;top:0;height:${TH}px;display:flex;align-items:center;font-weight:700;font-size:34px;letter-spacing:.11em;color:${txt};white-space:nowrap">${o.key}</div>`,
			`left:${tx}px;top:${y - TH / 2}px;width:${w}px;height:${TH}px`);
		if (sel) {
			// C's callout: a small tag card pointing back at the lit tag.
			const cw = 384, ch = 214, py = 52, cx = tx + w + 26;
			const cp = `28,0 ${cw - 26},0 ${cw},26 ${cw},${ch - 26} ${cw - 26},${ch} 28,${ch} 28,${py + 30} 0,${py} 28,${py - 30}`;
			place(`${panelSVG(cw, ch, { points: cp, sw: 3, strokeOpacity: .8 })}
				<div class="abs" style="left:50px;top:22px;right:24px">
					<div style="font-weight:700;font-size:23px;letter-spacing:.16em;color:${C.gold}">${o.key}</div>
					<div style="margin:10px 0 10px">${ornament(cw - 76)}</div>
					<div style="font-size:23px;line-height:1.3">${o.help}</div>
					<div style="margin-top:6px;font-size:18px;white-space:nowrap;transform:skewX(-12deg);transform-origin:left;color:${C.flavor}">${o.sub}</div></div>`,
				`left:${cx}px;top:${y - py}px;width:${cw}px;height:${ch}px`);
		}
	});

	// --- C's session tag, bottom-left ---
	(function session() {
		const ss = DATA.session, w = 560, h = 96;
		place(`${panelSVG(w, h, { points: inset(hTagPts(w - 4, h - 4)), sw: 3, strokeOpacity: .45 })}
			<div class="abs" style="left:-14px;top:${h / 2 - 23}px">${gemSVG(46, ss.slot, { stroke: C.salt, color: C.text })}</div>
			<div class="abs" style="left:52px;top:16px;right:56px;white-space:nowrap">
				<div style="display:flex;justify-content:space-between;align-items:baseline"><span style="font-weight:700;font-size:24px;letter-spacing:.14em">SESSION ${ss.slot} <span style="color:${C.label};font-weight:400">OF ${ss.of}</span></span>
					<span style="font-size:15px;letter-spacing:.3em;color:${C.icon}">${ss.serial}</span></div>
				<div style="display:flex;gap:26px;margin-top:8px;font-size:20px;color:${C.label}"><span>Played <b style="color:${C.text}">${ss.played}</b></span><span>Saved <b style="color:${C.text}">${ss.saved}</b></span></div></div>`,
			`left:110px;top:${1080 - 84 - h}px;width:${w}px;height:${h}px`);
	})();

	// --- C's hero card and mission tag, top-right ---
	(function cards() {
		const h = DATA.hero, cw = 446, ch = 400, cx = 1920 - 96 - cw, cy = 64;
		const row = (ic, icCol, label, value, band) => `<div style="display:flex;align-items:center;gap:14px;height:44px;padding:0 16px;background:${band ? 'rgba(212,238,244,.06)' : 'transparent'}">
			${icon(ic, 24, icCol)}<span style="flex:1;font-weight:700;font-size:20px;color:${C.label}">${label}</span><span style="font-weight:700;font-size:22px">${value}</span></div>`;
		place(`${panelSVG(cw, ch, { points: inset(vTagPts(cw - 4, ch - 4)), sw: 4 })}
			<div class="abs" style="left:${cw / 2 - 24}px;top:-18px">${gemSVG(48, h.level)}</div>
			<div class="abs" style="left:24px;top:40px;right:24px;white-space:nowrap">
				<div style="display:flex;align-items:center;gap:18px">${portraitHTML(96)}<div>
					<div style="font-weight:700;font-size:16px;letter-spacing:.26em;color:${C.flavor}">HERO · LEVEL ${h.level}</div>
					<div style="font-weight:700;font-size:29px;letter-spacing:.05em;margin-top:2px">${h.name}</div></div></div>
				<div style="margin-top:12px">
					${row('health', C.icon, 'Health', `${h.hp} <span style="color:${C.label};font-weight:400">/ ${h.hpMax}</span>`, true)}
					${row('xp', C.icon, 'Experience', `${fmt(h.xp)} <span style="color:${C.label};font-weight:400">/ ${fmt(h.xpNext)}</span>`, false)}
					${row('flask', C.heal, 'Soul flask', `<span style="display:inline-flex;align-items:center;gap:10px">${pipsSVG(3, DATA.flask.charges, C.heal, { h: 18, w: 10 })}${DATA.flask.charges} / ${DATA.flask.max}</span>`, true)}
					${row('grenade', C.icon, 'Grenades', `<span style="display:inline-flex;align-items:center;gap:10px">${pipsSVG(3, DATA.grenades.count, C.salt, { h: 18, w: 10 })}${DATA.grenades.count} / ${DATA.grenades.max}</span>`, false)}
				</div>
				<div style="margin-top:12px">${barSVG(cw - 48, 8, h.xp / h.xpNext, C.flavor, { edgeOpacity: .3 })}</div></div>`,
			`left:${cx}px;top:${cy}px;width:${cw}px;height:${ch}px`);
		const m = DATA.mission, mh = 196, my = cy + ch + 22;
		let steps = '';
		for (let i = 0; i < m.steps; i++) steps += `<svg width="16" height="16"><polygon points="8,1 15,8 8,15 1,8" fill="${i < m.step - 1 ? C.salt : i === m.step - 1 ? C.gold : 'none'}" stroke="${i === m.step - 1 ? C.gold : C.icon}" stroke-width="1.8"/></svg>`;
		place(`${panelSVG(cw, mh, { points: inset(vTagPts(cw - 4, mh - 4, { point: 0, notch: 52 })), sw: 3, strokeOpacity: .7 })}
			<div class="abs" style="left:${cw / 2 - 20}px;top:-14px;width:40px;height:40px"><svg width="40" height="40"><polygon points="20,2 38,20 20,38 2,20" fill="${C.glass2}" stroke="${C.gold}" stroke-width="3"/></svg><div class="abs" style="left:9px;top:9px">${icon('mission', 22, C.gold)}</div></div>
			<div class="abs" style="left:28px;top:36px;right:28px;white-space:nowrap">
				<div style="display:flex;align-items:center;justify-content:space-between"><span style="font-weight:700;font-size:16px;letter-spacing:.26em;color:${C.flavor}">TRACKED MISSION</span>
					<span style="display:flex;align-items:center;gap:5px">${steps}<b style="margin-left:6px;font-size:20px;color:${C.label}">${m.step} / ${m.steps}</b></span></div>
				<div style="font-weight:700;font-size:30px;letter-spacing:.08em;margin-top:6px">${m.name}</div>
				<div style="font-size:22px;margin-top:8px">• ${m.text}</div>
				<div style="font-size:18px;margin-top:6px;color:${C.label}">• Optional: ${m.optional}</div></div>`,
			`left:${cx}px;top:${my}px;width:${cw}px;height:${mh}px`);
	})();
	place(keysHTML(HUB_KEYS), 'right:96px;top:986px');
}

// ===================================================================================================================
// The Settings page: the whole screen, its own layout
// ===================================================================================================================
if (SET) {
	const crumb = `<span style="color:${C.label}">PAUSED</span><span style="color:${C.icon};margin:0 .5em">/</span><span style="color:${C.gold}">SETTINGS</span>`;
	// Header: E1 is the main menu's lockup exactly (small dial beside the title); E2 keeps its hub's big dial, cut by the
	// screen's left edge, the title beside it.
	if (E2) {
		place(`<svg width="400" height="400">${emblemSVG(30, 150, 104, { aim: 0, inner: true })}</svg>`, 'left:0;top:0');
		place(lockup('SETTINGS', crumb, { size: 58, ruler: 520 }), 'left:220px;top:92px');
	} else {
		place(`<svg width="200" height="200">${emblemSVG(100, 100, 45)}</svg>`, 'left:26px;top:52px');
		place(lockup('SETTINGS', crumb, { size: 52, ruler: 430 }), 'left:200px;top:96px');
	}

	// Tabs. E1: C's tags in a row, the lit one gold and dropped a little. E2: the tabs hang as small gun-card pennants
	// off a long ruler across the page, the lit one gold on a long gold tick.
	const tabTop = E2 ? 262 : 256;
	if (!E2) {
		const tw = 355, th = 68, gapX = 22, x0 = 210;
		place(`<span class="cap">PGUP</span>`, `left:96px;top:${tabTop + 14}px`);
		DATA.tabs.forEach((t, i) => {
			const lit = i === 0, col = lit ? C.gold : C.salt;
			place(`${panelSVG(tw, th, { points: inset(hTagPts(tw - 4, th - 4)), sw: lit ? 4 : 3, stroke: col, strokeOpacity: lit ? 1 : .5, horizontal: true,
				top: lit ? 'rgba(255,230,166,.28)' : C.glass, bottom: lit ? 'rgba(30,34,40,.9)' : C.glass2 })}
				<div class="abs" style="left:-15px;top:${th / 2 - 23}px">${gemIcon(46, TAB_ICON[i], lit, col, 20)}</div>
				<div class="abs" style="left:52px;right:30px;top:0;height:${th}px;display:flex;align-items:center;justify-content:center;font-weight:700;font-size:26px;letter-spacing:.12em;color:${lit ? C.gold : C.label}">${t}</div>`,
				`left:${x0 + i * (tw + gapX)}px;top:${tabTop + (lit ? 6 : 0)}px;width:${tw}px;height:${th}px`);
		});
		place(`<span class="cap">PGDN</span>`, `left:${x0 + 4 * (tw + gapX) + 8}px;top:${tabTop + 14}px`);
	} else {
		const x0 = 96, w = 1728, tw = 300, th = 92;
		const centres = [0, 1, 2, 3].map(i => x0 + 190 + i * 370 + tw / 2);
		let r = `<svg width="${w}" height="40">`;
		r += edged(`M0,14 L${w},14`, 1.8, C.salt, .6);
		for (let x = 0, i = 0; x <= w; x += 10.7, i++) r += `<line x1="${x}" y1="14" x2="${x}" y2="${14 + (i % 5 ? 6 : 12)}" stroke="${C.salt}" stroke-opacity="${i % 5 ? .4 : .7}" stroke-width="${i % 5 ? 1.2 : 1.8}"/>`;
		centres.forEach((c, i) => r += edged(`M${c - x0},14 L${c - x0},36`, i === 0 ? 3 : 2, i === 0 ? C.gold : C.salt, i === 0 ? 1 : .8));
		r += `</svg>`;
		place(r, `left:${x0}px;top:${tabTop}px`);
		place(`<span class="cap">PGUP</span>`, `left:${x0}px;top:${tabTop + 52}px`);
		place(`<span class="cap">PGDN</span>`, `right:96px;top:${tabTop + 52}px`);
		DATA.tabs.forEach((t, i) => {
			const lit = i === 0, col = lit ? C.gold : C.salt;
			place(`${panelSVG(tw, th, { points: inset(vTagPts(tw - 4, th - 4, { point: 14, notch: 30 })), sw: lit ? 4 : 3, stroke: col, strokeOpacity: lit ? 1 : .5,
				top: lit ? 'rgba(255,230,166,.28)' : C.glass, bottom: lit ? 'rgba(30,34,40,.92)' : C.glass2 })}
				<div class="abs" style="left:${tw / 2 - 16}px;top:-14px;width:32px;height:32px"><svg width="32" height="32"><polygon points="16,2 30,16 16,30 2,16" fill="${lit ? 'rgba(66,58,40,1)' : C.glass2}" stroke="${col}" stroke-width="3"/></svg></div>
				<div class="abs" style="left:0;right:0;top:20px;display:flex;align-items:center;justify-content:center;gap:12px;font-weight:700;font-size:26px;letter-spacing:.12em;color:${lit ? C.gold : C.label}">${icon(TAB_ICON[i], 24, lit ? C.gold : C.icon)}${t}</div>`,
				`left:${centres[i] - tw / 2}px;top:${tabTop + 36}px;width:${tw}px;height:${th}px`);
		});
	}

	// The Graphics rows, large, across the page's left two thirds; banded like the gun card's stat list.
	const rx = 96, ry = E2 ? 432 : 384, rw = 1160, rh = E2 ? 132 : 142;
	let rows = '';
	DATA.rows.forEach((r, i) => {
		const hov = i === 0;
		let ctl = '';
		if (r.kind === 'choice') ctl = `<div style="display:flex;gap:14px">${r.values.map((v, j) => {
			const lit = j === r.sel, cw = v.length * 17 + 46;
			return `<div style="position:relative;width:${cw}px;height:58px">${panelSVG(cw, 58, { cuts: [12, 0, 12, 0], sw: lit ? 3.5 : 2.5, stroke: lit ? C.gold : C.salt, strokeOpacity: lit ? 1 : .4,
				top: lit ? 'rgba(255,230,166,.24)' : 'rgba(8,10,13,.35)', bottom: lit ? 'rgba(255,230,166,.08)' : 'rgba(8,10,13,.35)' })}
				<div class="abs" style="inset:0;display:flex;align-items:center;justify-content:center;font-weight:700;font-size:24px;letter-spacing:.08em;color:${lit ? C.gold : C.label}">${v}</div></div>`;
		}).join('')}</div>`;
		else {
			const f = (r.value - r.min) / (r.max - r.min), sw = 380;
			let tk = '';
			for (let k = 0; k <= 40; k++) tk += `<line x1="${k * sw / 40}" y1="22" x2="${k * sw / 40}" y2="${22 + (k % 10 ? 6 : 12)}" stroke="${C.salt}" stroke-opacity="${k % 10 ? .4 : .7}" stroke-width="${k % 10 ? 1.2 : 1.8}"/>`;
			ctl = `<div style="display:flex;align-items:center;gap:22px"><div style="position:relative;width:${sw}px;height:60px">
				<svg class="abs" style="left:0;top:0" width="${sw}" height="60">${tk}
					<line x1="0" y1="20" x2="${sw}" y2="20" stroke="${C.salt}" stroke-opacity=".45" stroke-width="2"/>
					<line x1="0" y1="20" x2="${sw * f}" y2="20" stroke="${C.gold}" stroke-width="5"/>
					<g transform="translate(${sw * f - 1},20)"><polygon points="0,-15 15,0 0,15 -15,0" fill="${C.glass2}" stroke="${C.gold}" stroke-width="3.5"/><circle r="3.5" fill="${C.gold}"/></g></svg>
				<div class="abs" style="left:-10px;top:38px;font-size:18px;color:${C.label}">${r.min}°</div><div class="abs" style="right:-16px;top:38px;font-size:18px;color:${C.label}">${r.max}°</div></div>
				<span style="font-weight:700;font-size:34px;width:80px;text-align:right">${r.value}°</span></div>`;
		}
		rows += `<div style="position:relative;display:flex;align-items:center;height:${rh}px;padding:0 34px 0 0;background:${hov ? 'linear-gradient(90deg, rgba(255,230,166,.22), rgba(255,230,166,.03))' : i % 2 ? 'rgba(18,21,26,.55)' : 'rgba(30,34,40,.72)'};
			border-bottom:1.5px solid rgba(212,238,244,.14);${hov ? `box-shadow:inset 5px 0 0 ${C.gold}` : ''}">
			<span style="width:92px;text-align:center;font-weight:700;font-size:26px;color:${hov ? C.gold : C.icon}">0${i + 1}</span>
			<span style="flex:1;font-weight:700;font-size:32px;color:${hov ? C.gold : C.text}">${r.name}</span>${ctl}</div>`;
	});
	place(`<div style="border-top:1.5px solid rgba(212,238,244,.3)">${rows}</div>`,
		`left:${rx}px;top:${ry}px;width:${rw}px`);

	// The help panel on the page's right: C's gun-card shape, gem in the notch.
	const hx = rx + rw + 40, hw = 1920 - 96 - hx, hy = ry - 8, hh = 3 * rh + 8;
	place(`${panelSVG(hw, hh, { points: inset(vTagPts(hw - 4, hh - 4, { point: 0, notch: 50 })), sw: 3, strokeOpacity: .75 })}
		<div class="abs" style="left:${hw / 2 - 24}px;top:-18px">${gemIcon(48, 'monitor', true, C.gold)}</div>
		<div class="abs" style="left:34px;right:30px;top:46px">
			<div style="font-weight:700;font-size:16px;letter-spacing:.3em;color:${C.flavor}">POINTED AT</div>
			<div style="font-weight:700;font-size:32px;letter-spacing:.12em;margin-top:4px">${DATA.help.title}</div>
			<div style="margin:12px 0 14px">${ornament(hw - 64)}</div>
			<div style="font-size:23px;line-height:1.4">${DATA.help.text}</div>
			<div style="margin-top:14px;font-size:21px;line-height:1.4;color:${C.flavor};transform:skewX(-12deg);transform-origin:left">${DATA.help.detail}</div></div>`,
		`left:${hx}px;top:${hy}px;width:${hw}px;height:${hh}px`);

	place(keysHTML([['ENTER', 'SELECT'], ['PGUP / PGDN', 'TABS'], ['ESC', 'BACK']]), 'right:96px;top:986px');
}
