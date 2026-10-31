// Concept A "Salt Rail": the reference's structure in our look. Slim cut-corner glass plates on a left rail, the lit
// plate gold with a tag-shaped callout beside it, salt corner brackets round the screen, hero/mission panels top-right,
// the session card bottom-left. MODE ('hub' | 'settings') is set by the page.
const SET = MODE === 'settings';
const SEL = SET ? 1 : 0;
document.body.innerHTML = '<div class="world"></div><div class="vignette"></div>';
place('', `left:0;top:0;width:1920px;height:1080px;background:linear-gradient(90deg, rgba(12,14,18,.92) 0%, rgba(12,14,18,.78) 34%, rgba(12,14,18,${SET ? .62 : .2}) 62%, rgba(12,14,18,${SET ? .55 : .1}) 100%)`);

// --- The screen frame: corner brackets, hairline edges, a ruler at the foot ---
(function frame() {
	const W = 1920, H = 1080, m = 30, L = 120;
	let s = '';
	for (const [x, y, dx, dy] of [[m, m, 1, 1], [W - m, m, -1, 1], [W - m, H - m, -1, -1], [m, H - m, 1, -1]]) {
		s += `<path d="M${x + dx * L},${y} L${x},${y} L${x},${y + dy * L}" fill="none" stroke="${C.salt}" stroke-width="3"/>`;
		s += `<path d="M${x + dx * 10},${y + dy * (L - 34)} L${x + dx * 10},${y + dy * 10} L${x + dx * (L - 34)},${y + dy * 10}" fill="none" stroke="${C.salt}" stroke-opacity=".4" stroke-width="1.5"/>`;
		s += `<circle cx="${x + dx * 22}" cy="${y + dy * 22}" r="3.5" fill="${C.salt}" fill-opacity=".8"/>`;
	}
	s += `<line x1="${m + L + 18}" y1="${m}" x2="${W - m - L - 18}" y2="${m}" stroke="${C.salt}" stroke-opacity=".16"/>`;
	s += `<line x1="${m + L + 18}" y1="${H - m}" x2="${W - m - L - 18}" y2="${H - m}" stroke="${C.salt}" stroke-opacity=".16"/>`;
	s += `<line x1="${m}" y1="${m + L + 18}" x2="${m}" y2="${H - m - L - 18}" stroke="${C.salt}" stroke-opacity=".16"/>`;
	s += `<line x1="${W - m}" y1="${m + L + 18}" x2="${W - m}" y2="${H - m - L - 18}" stroke="${C.salt}" stroke-opacity=".16"/>`;
	for (let x = 780, i = 0; x <= 1140; x += 12, i++) s += `<line x1="${x}" y1="${m}" x2="${x}" y2="${m + (i % 5 ? 6 : 12)}" stroke="${C.salt}" stroke-opacity=".5" stroke-width="${i % 5 ? 1.2 : 2}"/>`;
	s += `<polygon points="960,${m + 16} 952,${m + 26} 968,${m + 26}" fill="${C.gold}"/>`;
	place(`<svg width="${W}" height="${H}">${s}</svg>`, 'left:0;top:0');
})();

// --- Header: today's pause dial and GAME PAUSED over a ruler ---
place(`<div style="display:flex;align-items:center;gap:26px">${dialSVG(104)}
	<div><div class="out" style="font-weight:700;font-size:60px;letter-spacing:.14em;line-height:1">GAME PAUSED</div>
	<div style="margin-top:16px">${rulerSVG(470, { step: 12, major: 4, opacity: .6 })}</div>
	<div style="margin-top:10px;font-size:16px;letter-spacing:.32em;color:${C.icon}">${DATA.session.serial}</div></div></div>`, 'left:104px;top:96px');

// --- The option plates ---
const X = 112, W = 580, PH = 76, Y0 = 290, GAP = 92;
const plateY = i => Y0 + i * GAP + (i === 3 ? 34 : 0);
DATA.options.forEach((o, i) => {
	const sel = i === SEL, danger = !!o.danger;
	const line = sel ? C.gold : danger ? C.red : C.salt;
	const txt = sel ? C.gold : danger ? C.red : C.text;
	const svg = panelSVG(W, PH, {
		cuts: [16, 0, 16, 0], sw: sel ? 4 : 2, stroke: line, strokeOpacity: sel ? 1 : danger ? .7 : .42, horizontal: true,
		top: sel ? 'rgba(255,230,166,.34)' : danger ? 'rgba(122,34,25,.55)' : 'rgba(30,34,40,.92)',
		bottom: sel ? 'rgba(30,34,40,.88)' : danger ? 'rgba(18,21,26,.82)' : 'rgba(18,21,26,.72)',
		extra: `<line x1="76" y1="14" x2="76" y2="${PH - 14}" stroke="${line}" stroke-opacity="${sel ? .7 : .3}" stroke-width="2"/>` +
			(sel ? `<circle cx="${W - 18}" cy="16" r="3.5" fill="${C.gold}"/><circle cx="${W - 18}" cy="${PH - 16}" r="3.5" fill="${C.gold}"/>` : ''),
	});
	const marker = sel ? `<svg class="abs" style="left:-34px;top:${PH / 2 - 14}px" width="22" height="28"><polygon points="0,0 22,14 0,28 6,14" fill="${C.gold}"/></svg>` : '';
	place(`${svg}${marker}<div class="abs" style="left:24px;top:${PH / 2 - 15}px">${icon(OPTION_ICON[i], 30, txt)}</div>
		<div class="abs" style="left:102px;top:0;height:${PH}px;display:flex;align-items:center;font-weight:700;font-size:36px;letter-spacing:.11em;color:${txt}">${o.key}</div>`,
		`left:${X}px;top:${plateY(i)}px;width:${W}px;height:${PH}px`);
});
// A hairline over Quit Game sets the danger apart.
place(`<svg width="${W}" height="2"><line x1="0" y1="1" x2="${W}" y2="1" stroke="${C.salt}" stroke-opacity=".25" stroke-dasharray="2 8"/></svg>`, `left:${X}px;top:${plateY(3) - 22}px`);

// --- Callout beside the lit plate (hub only): a small tag pointing at it ---
if (!SET) {
	const o = DATA.options[SEL], cw = 440, ch = 156, ty = 38;
	const pts = `14,0 ${cw},0 ${cw},${ch - 16} ${cw - 16},${ch} 14,${ch} 14,${ty + 13} 0,${ty} 14,${ty - 13}`;
	place(`${panelSVG(cw, ch, { points: pts, sw: 2, strokeOpacity: .75 })}
		<div class="abs" style="left:14px;top:0;width:5px;height:${ch}px;background:${C.gold}"></div>
		<div class="abs" style="left:40px;top:20px;right:24px">
			<div style="font-weight:700;font-size:22px;letter-spacing:.16em;color:${C.gold}">${o.key}</div>
			<div style="margin-top:10px;font-size:23px;line-height:1.3;color:${C.text}">${o.help}</div>
			<div style="margin-top:8px;font-size:19px;color:${C.label}">${o.sub}</div></div>`,
		`left:${X + W + 22}px;top:${plateY(SEL) + PH / 2 - ty}px;width:${cw}px;height:${ch}px`);
}

// --- Session card, bottom-left (the reference's news card) ---
(function session() {
	const s = DATA.session, w = W, h = 172;
	let pips = '';
	for (let i = 0; i < s.of; i++) pips += `<svg width="20" height="20"><polygon points="10,1 19,10 10,19 1,10" fill="${i === s.slot - 1 ? C.gold : 'none'}" stroke="${i === s.slot - 1 ? C.gold : C.icon}" stroke-width="2"/></svg>`;
	place(`${panelSVG(w, h, { cuts: [0, 22, 0, 0], sw: 2, strokeOpacity: .5 })}
		<div class="abs" style="left:28px;top:20px;right:28px;display:flex;align-items:center;gap:14px">
			${icon('save', 28, C.icon)}<span style="font-weight:700;font-size:28px;letter-spacing:.12em">SESSION ${s.slot}</span>
			<span style="font-size:20px;color:${C.label};letter-spacing:.1em">OF ${s.of}</span>
			<span style="flex:1"></span><span style="display:flex;gap:8px">${pips}</span></div>
		<div class="abs" style="left:28px;top:66px">${rulerSVG(w - 56, { step: 10, major: 6, opacity: .35, long: 8, short: 4 })}</div>
		<div class="abs" style="left:28px;top:90px;display:flex;gap:56px">
			<div><div style="font-size:17px;letter-spacing:.2em;color:${C.label}">PLAYED</div><div style="font-weight:700;font-size:27px;margin-top:2px">${s.played}</div></div>
			<div><div style="font-size:17px;letter-spacing:.2em;color:${C.label}">LAST SAVED</div><div style="font-weight:700;font-size:27px;margin-top:2px">${s.saved}</div></div></div>`,
		`left:${X}px;top:822px;width:${w}px;height:${h}px`);
})();

// --- Hero and mission panels, top-right (the reference's party and friends) ---
if (!SET) {
	const h = DATA.hero, pw = 540, px = 1920 - 92 - pw, ph = 268;
	const lab = t => `<span style="font-size:16px;letter-spacing:.22em;color:${C.label}">${t}</span>`;
	place(`${panelSVG(pw, ph, { cuts: [22, 0, 0, 0], sw: 2, strokeOpacity: .5 })}
		<div class="abs" style="left:20px;top:24px">${portraitHTML(140)}</div>
		<div class="abs" style="left:180px;top:22px;width:336px;white-space:nowrap">
			<div style="display:flex;align-items:center;justify-content:space-between">
				<div>${lab('HERO')}<div style="font-weight:700;font-size:30px;letter-spacing:.06em;margin-top:2px">${h.name}</div></div>${gemSVG(48, h.level)}</div>
			<div style="display:flex;justify-content:space-between;align-items:baseline;margin-top:12px">${lab('HEALTH')}
				<span style="font-weight:700;font-size:21px">${h.hp}<span style="color:${C.label};font-weight:400"> / ${h.hpMax}</span></span></div>
			<div style="margin-top:4px">${barSVG(336, 15, h.hp / h.hpMax, C.salt, { notches: 4 })}</div>
			<div style="display:flex;justify-content:space-between;align-items:baseline;margin-top:10px">${lab('LEVEL ' + h.level)}
				<span style="font-size:19px;color:${C.label}"><b style="color:${C.text}">${fmt(h.xp)}</b> / ${fmt(h.xpNext)} XP</span></div>
			<div style="margin-top:4px">${barSVG(336, 8, h.xp / h.xpNext, C.flavor, { edgeOpacity: .3 })}</div>
		</div>
		<div class="abs" style="left:20px;right:20px;top:206px;height:1px;background:rgba(212,238,244,.18)"></div>
		<div class="abs" style="left:24px;right:24px;top:220px;display:flex;align-items:center;justify-content:space-between;white-space:nowrap">
			<span style="display:flex;align-items:center;gap:12px">${icon('flask', 30, C.heal)}${pipsSVG(3, DATA.flask.charges, C.heal)}${lab('SOUL FLASK')}</span>
			<span style="display:flex;align-items:center;gap:12px">${icon('grenade', 30, C.icon)}${pipsSVG(3, DATA.grenades.count, C.salt)}${lab('GRENADES')}</span></div>`,
		`left:${px}px;top:86px;width:${pw}px;height:${ph}px`);

	const m = DATA.mission;
	let steps = '';
	for (let i = 0; i < m.steps; i++) steps += `<svg width="16" height="16"><polygon points="8,1 15,8 8,15 1,8" fill="${i < m.step - 1 ? C.salt : i === m.step - 1 ? C.gold : 'none'}" stroke="${i === m.step - 1 ? C.gold : C.icon}" stroke-width="1.8"/></svg>`;
	place(`${panelSVG(pw, 214, { cuts: [0, 0, 22, 0], sw: 2, strokeOpacity: .5 })}
		<div class="abs" style="left:26px;top:22px;right:26px;white-space:nowrap">
			<div style="display:flex;align-items:center;justify-content:space-between">
				<span style="font-size:17px;letter-spacing:.26em;color:${C.label}">${DATA.area}</span>
				<span style="font-size:17px;letter-spacing:.18em;color:${C.label}">TRACKED MISSION</span></div>
			<div style="margin-top:8px">${rulerSVG(pw - 52, { step: 10, major: 6, opacity: .35, long: 8, short: 4 })}</div>
			<div style="display:flex;align-items:center;gap:14px;margin-top:12px">${icon('mission', 30, C.gold)}
				<span style="font-weight:700;font-size:32px;letter-spacing:.1em">${m.name}</span><span style="flex:1"></span>
				<span style="display:flex;gap:5px">${steps}</span><span style="font-weight:700;font-size:22px;color:${C.label}">${m.step} / ${m.steps}</span></div>
			<div style="font-size:24px;margin-top:12px;color:${C.text}">${m.text}</div>
			<div style="font-size:18px;margin-top:8px;color:${C.label};letter-spacing:.04em"><span style="letter-spacing:.14em">OPTIONAL:</span> ${m.optional}</div></div>`,
		`left:${px}px;top:372px;width:${pw}px;height:214px`);
}

// --- Settings, opened to the right of the column ---
if (SET) {
	const px = 772, py = 86, pw = 1920 - 92 - px, ph = 760;
	// Gold lead from the lit plate into the panel.
	const ly = plateY(SEL) + PH / 2;
	place(`<svg width="${px - X - W + 2}" height="20"><line x1="0" y1="10" x2="${px - X - W}" y2="10" stroke="${C.gold}" stroke-width="3"/><polygon points="${px - X - W - 12},2 ${px - X - W},10 ${px - X - W - 12},18" fill="${C.gold}"/></svg>`, `left:${X + W}px;top:${ly - 10}px`);
	let tabs = '';
	DATA.tabs.forEach((t, i) => tabs += `<div style="width:200px;text-align:center;font-weight:700;font-size:26px;letter-spacing:.1em;color:${i === 0 ? C.gold : C.label}">${t}</div>`);
	const rowsW = pw - 80;
	let rows = '';
	DATA.rows.forEach((r, i) => {
		const hov = i === 0;
		let ctl = '';
		if (r.kind === 'choice') ctl = `<div style="display:flex;gap:10px">${r.values.map((v, j) => `<div style="height:44px;padding:0 14px;display:flex;align-items:center;border-radius:8px;font-weight:700;font-size:20px;letter-spacing:.06em;
			border:${j === r.sel ? 3 : 2}px solid ${j === r.sel ? C.gold : 'rgba(212,238,244,.4)'};color:${j === r.sel ? C.gold : C.label};background:${j === r.sel ? 'rgba(255,230,166,.14)' : 'rgba(8,10,13,.35)'}">${v}</div>`).join('')}</div>`;
		else {
			const f = (r.value - r.min) / (r.max - r.min), sw = 230;
			ctl = `<div style="display:flex;align-items:center;gap:18px"><div style="position:relative;width:${sw}px;height:44px">
				<div class="abs" style="left:0;top:14px">${rulerSVG(sw, { step: sw / 8, major: 4, opacity: .5, long: 10, short: 5 })}</div>
				<div class="abs" style="left:0;top:12px;width:${sw * f}px;height:4px;background:${C.gold}"></div>
				<svg class="abs" style="left:${sw * f - 13}px;top:1px" width="26" height="26"><circle cx="13" cy="13" r="10" fill="${C.glass2}" stroke="${C.gold}" stroke-width="3"/><circle cx="13" cy="13" r="3.5" fill="${C.gold}"/></svg>
				<div class="abs" style="left:-8px;top:28px;font-size:15px;color:${C.label}">${r.min}°</div><div class="abs" style="right:-12px;top:28px;font-size:15px;color:${C.label}">${r.max}°</div></div>
				<span style="font-weight:700;font-size:28px;width:62px">${r.value}°</span></div>`;
		}
		rows += `<div style="position:relative;height:88px;display:flex;align-items:center;border-bottom:1px solid rgba(212,238,244,.14)">
			${hov ? `<div class="abs" style="left:64px;right:0;top:6px;bottom:6px;background:linear-gradient(90deg, rgba(255,230,166,.22), rgba(255,230,166,.03));border-left:4px solid ${C.gold}"></div>` : ''}
			<div style="position:relative;width:64px;font-size:24px;color:${hov ? C.gold : C.label};text-align:center">0${i + 1}</div>
			<div style="position:relative;flex:1;padding-left:22px;font-weight:700;font-size:27px;color:${hov ? C.gold : C.text}">${r.name}</div>
			<div style="position:relative;padding-right:16px">${ctl}</div></div>`;
	});
	place(`${panelSVG(pw, ph, { cuts: [26, 0, 26, 0], sw: 2, strokeOpacity: .55 })}
		<div class="abs" style="left:40px;top:30px;display:flex;align-items:baseline;gap:20px"><span style="font-weight:700;font-size:44px;letter-spacing:.14em">SETTINGS</span>
			<span style="font-size:19px;letter-spacing:.24em;color:${C.label}">GAME PAUSED</span></div>
		<span class="cap abs" style="left:40px;top:118px">PGUP</span><span class="cap abs" style="right:40px;top:118px">PGDN</span>
		<div class="abs" style="left:${(pw - 800) / 2}px;top:124px;display:flex">${tabs}</div>
		<div class="abs" style="left:40px;top:172px">${rulerSVG(pw - 80, { step: 12, major: 5, opacity: .5 })}</div>
		<div class="abs" style="left:${(pw - 800) / 2 + 40}px;top:166px;width:120px;height:4px;background:${C.gold}"></div>
		<svg class="abs" style="left:${(pw - 800) / 2 + 100 - 9}px;top:180px" width="18" height="12"><polygon points="9,0 18,12 0,12" fill="${C.gold}"/></svg>
		<div class="abs" style="left:40px;top:214px;width:${rowsW}px">
			<div class="abs" style="left:64px;top:0;width:2px;height:${3 * 88}px;background:rgba(212,238,244,.25)"></div>${rows}</div>
		<div class="abs" style="left:40px;top:${214 + 3 * 88 + 48}px;width:${pw - 80}px;display:flex;gap:26px">
			<div style="width:4px;background:${C.gold};flex:none"></div><div style="flex:1;padding:2px 0">
			<div style="font-weight:700;font-size:24px;letter-spacing:.16em;color:${C.gold}">${DATA.help.title}</div>
			<div style="font-size:22px;line-height:1.4;margin-top:10px;color:${C.text}">${DATA.help.text}</div>
			<div style="font-size:20px;line-height:1.4;margin-top:12px;color:${C.label}">${DATA.help.detail}</div></div></div>`,
		`left:${px}px;top:${py}px;width:${pw}px;height:${ph}px`);
}

place(keysHTML(SET ? SET_KEYS : HUB_KEYS), `right:104px;top:986px`);
