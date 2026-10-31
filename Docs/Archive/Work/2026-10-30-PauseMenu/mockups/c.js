// Concept C "Tag Rack": the gun card's tag shape laid on its side. Each option is a tag (notch with a gem, cut shoulders,
// a point) hung from a rail; the lit tag slides out gold. Hero and mission are tag cards top-right read like the gun card
// (gem in the notch, banded stat rows, the ornament); Settings opens as a big tag card pointing back at its tag.
const SET = MODE === 'settings';
const SEL = SET ? 1 : 0;
document.body.innerHTML = '<div class="world"></div><div class="vignette"></div>';
place('', `left:0;top:0;width:1920px;height:1080px;background:linear-gradient(90deg, rgba(10,12,15,.9) 0%, rgba(10,12,15,.74) 36%, rgba(10,12,15,${SET ? .6 : .18}) 60%, rgba(10,12,15,${SET ? .6 : .3}) 100%)`);

// Horizontal option tag: the notch on the left holds the gem, cut shoulders and a point on the right.
function hTagPts(w, h) {
	const m = h / 2;
	return [[34, 0], [w - 42, 0], [w - 18, 18], [w, m], [w - 18, h - 18], [w - 42, h], [34, h], [12, h - 22], [12, m + 28], [0, m + 18], [0, m - 18], [12, m - 28], [12, 22]]
		.map(p => p.join(',')).join(' ');
}
// Vertical tag card (the gun card): a raised notch in the top edge for the gem, cut corners, cut shoulders down to a point.
function vTagPts(w, h, o = {}) {
	const n = o.notch ?? 70, cut = 26, pt = o.point ?? 28, top = 12, c = w / 2;
	const pts = [[0, top + cut], [cut, top], [c - n - 12, top], [c - n, 0], [c + n, 0], [c + n + 12, top], [w - cut, top], [w, top + cut]];
	if (o.left !== undefined) {
		// Points left at y = o.left instead of down: the shoulders sit on the left edge.
		pts.push([w, h - cut], [w - cut, h], [cut, h], [0, h - cut], [0, o.left + 44], [-28, o.left], [0, o.left - 44]);
	} else if (pt) pts.push([w, h - pt - cut], [w - cut, h - pt], [c, h], [cut, h - pt], [0, h - pt - cut]);
	else pts.push([w, h - cut], [w - cut, h], [cut, h], [0, h - cut]);
	return pts.map(p => p.join(',')).join(' ');
}
function gemIcon(size, iconName, lit, col) {
	const h = size / 2;
	return `<div style="position:relative;width:${size}px;height:${size}px"><svg class="abs" style="left:0;top:0" width="${size}" height="${size}">
		<polygon points="${h},2 ${size - 2},${h} ${h},${size - 2} 2,${h}" fill="${lit ? 'rgba(66,58,40,1)' : C.glass2}" stroke="${col}" stroke-width="3.5"/></svg>
		<div class="abs" style="left:${h - 11}px;top:${h - 11}px">${icon(iconName, 22, col)}</div></div>`;
}
const ornament = w => `<svg width="${w}" height="14" style="display:block"><line x1="0" y1="7" x2="${w / 2 - 16}" y2="7" stroke="${C.salt}" stroke-opacity=".5" stroke-width="1.5"/>
	<polygon points="${w / 2},1 ${w / 2 + 6},7 ${w / 2},13 ${w / 2 - 6},7" fill="none" stroke="${C.gold}" stroke-width="2"/><line x1="${w / 2 + 16}" y1="7" x2="${w}" y2="7" stroke="${C.salt}" stroke-opacity=".5" stroke-width="1.5"/></svg>`;

// --- Header, as the gun card's top: the place (flavour gold) over the name, the ornament under ---
place(`<div style="font-weight:700;font-size:21px;letter-spacing:.32em;color:${C.flavor}">${DATA.area}</div>
	<div class="out" style="font-weight:700;font-size:62px;letter-spacing:.12em;line-height:1.1;margin-top:4px">GAME PAUSED</div>
	<div style="margin-top:14px">${ornament(500)}</div>`, 'left:130px;top:96px');

// --- The rack: a rail with a peg to each tag ---
const X = 140, W = 560, TH = 82, Y0 = 296, GAP = 100;
const tagY = i => Y0 + i * GAP + (i === 3 ? 30 : 0);
(function rail() {
	let s = `<line x1="0" y1="${tagY(0) - 20}" x2="0" y2="${tagY(3) + TH + 20}" stroke="${C.icon}" stroke-width="4"/>`;
	s += `<circle cx="0" cy="${tagY(0) - 20}" r="6" fill="${C.glass2}" stroke="${C.salt}" stroke-width="2.5"/><circle cx="0" cy="${tagY(3) + TH + 20}" r="6" fill="${C.glass2}" stroke="${C.salt}" stroke-width="2.5"/>`;
	DATA.options.forEach((o, i) => {
		const y = tagY(i) + TH / 2, sel = i === SEL;
		s += `<line x1="0" y1="${y}" x2="${(sel ? 34 : 10) + 20}" y2="${y}" stroke="${sel ? C.gold : C.icon}" stroke-width="${sel ? 3 : 2.5}"/><circle cx="0" cy="${y}" r="4.5" fill="${sel ? C.gold : C.icon}"/>`;
	});
	place(`<svg width="80" height="1080">${s}</svg>`, `left:${X - 30}px;top:0`);
})();

DATA.options.forEach((o, i) => {
	const sel = i === SEL, danger = !!o.danger;
	const col = sel ? C.gold : danger ? C.red : C.salt;
	const txt = sel ? C.gold : danger ? C.red : C.text;
	const w = sel ? W + 30 : W, dx = sel ? 26 : 0;
	const svg = panelSVG(w, TH, {
		points: hTagPts(w - 4, TH - 4).split(' ').map(p => p.split(',').map(v => +v + 2).join(',')).join(' '),
		sw: sel ? 4 : 3, stroke: col, strokeOpacity: sel ? 1 : danger ? .75 : .55, horizontal: true,
		top: sel ? 'rgba(255,230,166,.30)' : danger ? 'rgba(122,34,25,.6)' : C.glass, bottom: sel ? 'rgba(30,34,40,.9)' : C.glass2,
		extra: sel ? `<circle cx="${w - 46}" cy="14" r="3.5" fill="${C.gold}"/><circle cx="${w - 46}" cy="${TH - 14}" r="3.5" fill="${C.gold}"/>` : '',
	});
	place(`${svg}<div class="abs" style="left:-17px;top:${TH / 2 - 26}px">${gemIcon(52, OPTION_ICON[i], sel, col)}</div>
		<div class="abs" style="left:64px;top:0;height:${TH}px;display:flex;align-items:center;font-weight:700;font-size:36px;letter-spacing:.11em;color:${txt}">${o.key}</div>`,
		`left:${X + dx}px;top:${tagY(i)}px;width:${w}px;height:${TH}px`);
});

// --- Callout (hub): a small tag card pointing back at the lit tag ---
if (!SET) {
	const o = DATA.options[SEL], cw = 440, ch = 200, py = 52;
	const pts = `28,0 ${cw - 26},0 ${cw},26 ${cw},${ch - 26} ${cw - 26},${ch} 28,${ch} 28,${py + 30} 0,${py} 28,${py - 30}`;
	place(`${panelSVG(cw, ch, { points: pts, sw: 3, strokeOpacity: .8 })}
		<div class="abs" style="left:50px;top:22px;right:26px">
			<div style="font-weight:700;font-size:23px;letter-spacing:.16em;color:${C.gold}">${o.key}</div>
			<div style="margin:10px 0 10px">${ornament(cw - 80)}</div>
			<div style="font-size:23px;line-height:1.3">${o.help}</div>
			<div style="margin-top:6px;font-size:19px;font-style:normal;transform:skewX(-12deg);transform-origin:left;color:${C.flavor}">${o.sub}</div></div>`,
		`left:${X + W + 30 + 26 + 20}px;top:${tagY(SEL) + TH / 2 - py}px;width:${cw}px;height:${ch}px`);
}

// --- Session tag, bottom-left ---
(function session() {
	const s = DATA.session, w = W, h = 96;
	place(`${panelSVG(w, h, { points: hTagPts(w - 4, h - 4).split(' ').map(p => p.split(',').map(v => +v + 2).join(',')).join(' '), sw: 3, strokeOpacity: .45 })}
		<div class="abs" style="left:-14px;top:${h / 2 - 23}px">${gemSVG(46, s.slot, { stroke: C.salt, color: C.text })}</div>
		<div class="abs" style="left:52px;top:16px;right:56px;white-space:nowrap">
			<div style="display:flex;justify-content:space-between;align-items:baseline"><span style="font-weight:700;font-size:24px;letter-spacing:.14em">SESSION ${s.slot} <span style="color:${C.label};font-weight:400">OF ${s.of}</span></span>
				<span style="font-size:15px;letter-spacing:.3em;color:${C.icon}">${s.serial}</span></div>
			<div style="display:flex;gap:26px;margin-top:8px;font-size:20px;color:${C.label}"><span>Played <b style="color:${C.text}">${s.played}</b></span><span>Saved <b style="color:${C.text}">${s.saved}</b></span></div></div>`,
		`left:${X}px;top:874px;width:${w}px;height:${h}px`);
})();

// --- Hero card and mission tag, top-right ---
if (!SET) {
	const h = DATA.hero, cw = 480, ch = 400, cx = 1920 - 96 - cw, cy = 64;
	const row = (ic, icCol, label, value, band) => `<div style="display:flex;align-items:center;gap:14px;height:44px;padding:0 16px;background:${band ? 'rgba(212,238,244,.06)' : 'transparent'}">
		${icon(ic, 24, icCol)}<span style="flex:1;font-weight:700;font-size:20px;color:${C.label}">${label}</span><span style="font-weight:700;font-size:22px">${value}</span></div>`;
	place(`${panelSVG(cw, ch, { points: vTagPts(cw - 4, ch - 4).split(' ').map(p => p.split(',').map(v => +v + 2).join(',')).join(' '), sw: 4 })}
		<div class="abs" style="left:${cw / 2 - 24}px;top:-18px">${gemSVG(48, h.level)}</div>
		<div class="abs" style="left:24px;top:40px;right:24px">
			<div style="display:flex;align-items:center;gap:18px">${portraitHTML(96)}<div>
				<div style="font-weight:700;font-size:16px;letter-spacing:.26em;color:${C.flavor}">HERO · LEVEL ${h.level}</div>
				<div style="font-weight:700;font-size:30px;letter-spacing:.05em;margin-top:2px">${h.name}</div></div></div>
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
	place(`${panelSVG(cw, mh, { points: vTagPts(cw - 4, mh - 4, { point: 0, notch: 52 }).split(' ').map(p => p.split(',').map(v => +v + 2).join(',')).join(' '), sw: 3, strokeOpacity: .7 })}
		<div class="abs" style="left:${cw / 2 - 20}px;top:-14px;width:40px;height:40px"><svg width="40" height="40"><polygon points="20,2 38,20 20,38 2,20" fill="${C.glass2}" stroke="${C.gold}" stroke-width="3"/></svg><div class="abs" style="left:9px;top:9px">${icon('mission', 22, C.gold)}</div></div>
		<div class="abs" style="left:28px;top:36px;right:28px;white-space:nowrap">
			<div style="display:flex;align-items:center;justify-content:space-between"><span style="font-weight:700;font-size:16px;letter-spacing:.26em;color:${C.flavor}">TRACKED MISSION</span>
				<span style="display:flex;align-items:center;gap:5px">${steps}<b style="margin-left:6px;font-size:20px;color:${C.label}">${m.step} / ${m.steps}</b></span></div>
			<div style="font-weight:700;font-size:30px;letter-spacing:.08em;margin-top:6px">${m.name}</div>
			<div style="font-size:22px;margin-top:8px">• ${m.text}</div>
			<div style="font-size:18px;margin-top:6px;color:${C.label}">• Optional: ${m.optional}</div></div>`,
		`left:${cx}px;top:${my}px;width:${cw}px;height:${mh}px`);
}

// --- Settings: a big tag card pointing back at the Settings tag ---
if (SET) {
	const cx = 820, cy = 70, cw = 1920 - 96 - cx, ch = 790;
	const py = tagY(SEL) + TH / 2 - cy;
	let tabs = '';
	DATA.tabs.forEach((t, i) => tabs += `<div style="flex:1;text-align:center;padding:10px 0;font-weight:700;font-size:24px;letter-spacing:.07em;color:${i === 0 ? C.gold : C.label};
		${i === 0 ? `border-bottom:4px solid ${C.gold};background:linear-gradient(0deg, rgba(255,230,166,.16), rgba(255,230,166,0))` : 'border-bottom:4px solid rgba(212,238,244,.14)'}">${t}</div>`);
	let rows = '';
	DATA.rows.forEach((r, i) => {
		const hov = i === 0;
		let ctl = '';
		if (r.kind === 'choice') ctl = `<div style="display:flex;gap:10px">${r.values.map((v, j) => `<div style="height:44px;padding:0 15px;display:flex;align-items:center;border-radius:10px;font-weight:700;font-size:20px;letter-spacing:.06em;
			border:3px solid ${j === r.sel ? C.gold : 'rgba(212,238,244,.35)'};color:${j === r.sel ? C.gold : C.label};background:${j === r.sel ? 'rgba(255,230,166,.14)' : 'transparent'}">${v}</div>`).join('')}</div>`;
		else {
			const f = (r.value - r.min) / (r.max - r.min), sw = 250;
			ctl = `<div style="display:flex;align-items:center;gap:16px"><span style="font-size:17px;color:${C.label}">${r.min}°</span><div style="position:relative;width:${sw}px;height:20px">
				<div class="abs" style="left:0;top:8px;width:${sw}px;height:4px;background:rgba(212,238,244,.3)"></div><div class="abs" style="left:0;top:7px;width:${sw * f}px;height:6px;background:${C.gold}"></div>
				<svg class="abs" style="left:${sw * f - 13}px;top:-3px" width="26" height="26"><polygon points="13,1 25,13 13,25 1,13" fill="${C.glass2}" stroke="${C.gold}" stroke-width="3.5"/></svg></div>
				<span style="font-size:17px;color:${C.label}">${r.max}°</span><span style="font-weight:700;font-size:26px;width:60px;text-align:right">${r.value}°</span></div>`;
		}
		rows += `<div style="display:flex;align-items:center;height:84px;padding:0 22px;background:${hov ? 'linear-gradient(90deg, rgba(255,230,166,.2), rgba(255,230,166,.04))' : i % 2 ? 'transparent' : 'rgba(212,238,244,.06)'};${hov ? `box-shadow:inset 4px 0 0 ${C.gold}` : ''}">
			<span style="width:50px;font-weight:700;font-size:21px;color:${hov ? C.gold : C.icon}">0${i + 1}</span>
			<span style="flex:1;font-weight:700;font-size:27px;color:${hov ? C.gold : C.text}">${r.name}</span>${ctl}</div>`;
	});
	place(`${panelSVG(cw, ch, { points: vTagPts(cw - 4, ch - 4, { left: py, notch: 90 }).split(' ').map(p => p.split(',').map(v => +v + 2).join(',')).join(' '), sw: 4, strokeOpacity: .9 })}
		<div class="abs" style="left:${cw / 2 - 26}px;top:-20px">${gemIcon(52, 'gear', true, C.gold)}</div>
		<div class="abs" style="left:48px;top:50px;right:48px">
			<div style="text-align:center;font-weight:700;font-size:17px;letter-spacing:.3em;color:${C.flavor}">GAME PAUSED</div>
			<div style="text-align:center;font-weight:700;font-size:44px;letter-spacing:.14em;margin-top:2px">SETTINGS</div>
			<div style="margin:14px auto 0;width:420px">${ornament(420)}</div>
			<div style="display:flex;align-items:center;gap:18px;margin-top:26px"><span class="cap">PGUP</span><div style="flex:1;display:flex">${tabs}</div><span class="cap">PGDN</span></div>
			<div style="margin-top:26px">${rows}</div>
			<div style="margin-top:30px;padding:0 22px">
				<div style="font-weight:700;font-size:23px;letter-spacing:.16em;color:${C.gold}">${DATA.help.title}</div>
				<div style="font-size:22px;line-height:1.4;margin-top:8px">• ${DATA.help.text}</div>
				<div style="font-size:20px;line-height:1.4;margin-top:8px;color:${C.flavor};transform:skewX(-12deg);transform-origin:left">${DATA.help.detail}</div></div></div>`,
		`left:${cx}px;top:${cy}px;width:${cw}px;height:${ch}px`);
}

place(keysHTML(SET ? SET_KEYS : HUB_KEYS), `right:96px;top:${SET ? 994 : 986}px`);
