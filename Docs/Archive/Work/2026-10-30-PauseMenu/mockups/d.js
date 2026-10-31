// Concept D "Spur Rowel": the game's logo mark made the menu. A big spur rowel sits on the left with today's pause dial as
// its hub; its spikes are 30 degrees apart and the four options sit on an arc one spike each, so moving the selection
// turns the wheel one click and the gold spike points at the lit word (in reticle brackets). The callout and the info
// sit on "Ledger & Brass" plates (double salt trim, rivets) like today's pause menu. MODE ('hub' | 'settings').
const SET = MODE === 'settings';
const SEL = 1;
document.body.innerHTML = '<div class="world"></div><div class="vignette"></div>';
place('', `left:0;top:0;width:1920px;height:1080px;background:radial-gradient(circle 900px at 140px 560px, rgba(10,12,15,.9) 0%, rgba(10,12,15,.78) 55%, rgba(10,12,15,${SET ? .62 : .2}) 100%)`);
if (SET) place('', 'left:0;top:0;width:1920px;height:1080px;background:rgba(10,12,15,.35)');

// A Ledger & Brass plate: glass, a double salt trim, cut corners and rivets.
function brassPlate(w, h, o = {}) {
	const cut = o.cut ?? 18, col = o.stroke ?? C.salt;
	const inner = cutPts(9, 9, w - 18, h - 18, [cut - 6, cut - 6, cut - 6, cut - 6]);
	const riv = [[22, 22], [w - 22, 22], [w - 22, h - 22], [22, h - 22]].map(([x, y]) => `<circle cx="${x}" cy="${y}" r="3.5" fill="${col}" fill-opacity=".85"/>`).join('');
	return panelSVG(w, h, { cuts: [cut, cut, cut, cut], sw: o.sw ?? 3, stroke: col, strokeOpacity: o.op ?? .85,
		extra: `<polygon points="${inner}" fill="none" stroke="${col}" stroke-opacity=".35" stroke-width="1.5"/>${o.rivets === false ? '' : riv}${o.extra ?? ''}` });
}

// --- The rowel ---
const CX = 150, CY = 556, R_ARC = 340, STEP = 30;
const angle = i => (i - 1.5) * STEP;             // -45, -15, 15, 45 degrees
const rad = d => d * Math.PI / 180;
const optPos = i => [CX + Math.cos(rad(angle(i))) * R_ARC, CY + Math.sin(rad(angle(i))) * R_ARC];
(function rowel() {
	const Ro = 262, Ri = 176, n = 12;
	let s = '';
	// The range arc the words sit on.
	const a0 = rad(-62), a1 = rad(62);
	s += `<path d="M${CX + Math.cos(a0) * R_ARC},${CY + Math.sin(a0) * R_ARC} A${R_ARC},${R_ARC} 0 0 1 ${CX + Math.cos(a1) * R_ARC},${CY + Math.sin(a1) * R_ARC}" fill="none" stroke="${C.salt}" stroke-opacity=".3" stroke-width="2"/>`;
	for (let d = -62; d <= 62; d += 3) {
		const big = d % 15 === 0, r0 = R_ARC - (big ? 14 : 7);
		s += `<line x1="${CX + Math.cos(rad(d)) * r0}" y1="${CY + Math.sin(rad(d)) * r0}" x2="${CX + Math.cos(rad(d)) * R_ARC}" y2="${CY + Math.sin(rad(d)) * R_ARC}" stroke="${C.salt}" stroke-opacity="${big ? .55 : .3}" stroke-width="${big ? 2 : 1.2}"/>`;
	}
	// The rowel: 12 slim spikes 30 degrees apart, one at each option's angle; the lit one gold and longer.
	for (let k = 0; k < n; k++) {
		const d = angle(0) + k * STEP, lit = Math.abs(d - angle(SEL)) < .1 || Math.abs(d - angle(SEL) - 360) < .1;
		const tip = lit ? Ro + 20 : Ro, hw = 9;
		const P = (deg, r) => `${CX + Math.cos(rad(deg)) * r},${CY + Math.sin(rad(deg)) * r}`;
		s += `<polygon points="${P(d - hw, Ri - 8)} ${P(d, tip)} ${P(d + hw, Ri - 8)}" fill="${lit ? C.gold : 'rgba(46,52,60,.96)'}" stroke="${lit ? C.gold : C.salt}" stroke-width="3" stroke-linejoin="miter"/>` + (lit ? '' : `<line x1="${P(d, Ri + 4).split(',')[0]}" y1="${P(d, Ri + 4).split(',')[1]}" x2="${P(d, tip - 26).split(',')[0]}" y2="${P(d, tip - 26).split(',')[1]}" stroke="${C.salt}" stroke-opacity=".35" stroke-width="1.5"/>`);
	}
	s += `<circle cx="${CX}" cy="${CY}" r="${Ri}" fill="rgba(18,21,26,.95)" stroke="${C.salt}" stroke-width="3"/>`;
	for (let k = 0; k < n; k++) {
		const d = rad(angle(0) + k * STEP + STEP / 2);
		s += `<circle cx="${CX + Math.cos(d) * (Ri - 12)}" cy="${CY + Math.sin(d) * (Ri - 12)}" r="3.5" fill="${C.salt}" fill-opacity=".8"/>`;
	}
	const as = rad(angle(SEL));
	s += `<line x1="${CX + Math.cos(as) * (Ro + 22)}" y1="${CY + Math.sin(as) * (Ro + 22)}" x2="${CX + Math.cos(as) * (R_ARC - 6)}" y2="${CY + Math.sin(as) * (R_ARC - 6)}" stroke="${C.gold}" stroke-width="3"/>`;
	// Rings and ticks inside the star, then the hub: today's pause dial.
	s += `<circle cx="${CX}" cy="${CY}" r="160" fill="none" stroke="${C.salt}" stroke-opacity=".5" stroke-width="2"/>`;
	for (let k = 0; k < 72; k++) {
		const a = rad(k * 5), big = k % 6 === 0;
		s += `<line x1="${CX + Math.cos(a) * (big ? 142 : 150)}" y1="${CY + Math.sin(a) * (big ? 142 : 150)}" x2="${CX + Math.cos(a) * 158}" y2="${CY + Math.sin(a) * 158}" stroke="${C.salt}" stroke-opacity="${big ? .8 : .4}" stroke-width="${big ? 2 : 1.2}"/>`;
	}
	s += `<circle cx="${CX}" cy="${CY}" r="118" fill="${C.glass2}" stroke="${C.salt}" stroke-width="3"/>`;
	s += `<circle cx="${CX}" cy="${CY}" r="108" fill="none" stroke="${C.salt}" stroke-opacity=".35" stroke-width="1.5"/>`;
	const ga = rad(-80), gb = rad(10);
	s += `<path d="M${CX + Math.cos(ga) * 92},${CY + Math.sin(ga) * 92} A92,92 0 0 1 ${CX + Math.cos(gb) * 92},${CY + Math.sin(gb) * 92}" fill="none" stroke="${C.gold}" stroke-width="6" stroke-linecap="round"/>`;
	s += `<rect x="${CX - 28}" y="${CY - 36}" width="20" height="72" fill="${C.text}"/><rect x="${CX + 8}" y="${CY - 36}" width="20" height="72" fill="${C.text}"/>`;
		place(`<svg width="1920" height="1080">${s}</svg>`, 'left:0;top:0');
})();

// --- Header: GAME PAUSED over a dotted ruler, the area beside it ---
place(`<div class="out" style="font-weight:700;font-size:60px;letter-spacing:.14em;line-height:1">GAME PAUSED</div>
	<div style="margin-top:14px;display:flex;align-items:center;gap:18px">${rulerSVG(300, { step: 15, major: 4, opacity: .6 })}
		<span style="border:2px solid ${C.flavor};padding:4px 12px;font-weight:700;font-size:20px;letter-spacing:.16em;color:${C.flavor};background:rgba(18,21,26,.7)">${DATA.area}</span></div>`, 'left:96px;top:84px');

// --- The four words on the arc ---
DATA.options.forEach((o, i) => {
	const sel = i === SEL, danger = !!o.danger;
	const [x, y] = optPos(i);
	const col = sel ? C.gold : danger ? C.red : C.text;
	const size = sel ? 52 : 44;
	const dot = `<svg class="abs" style="left:${x - 9}px;top:${y - 9}px" width="18" height="18"><polygon points="9,1 17,9 9,17 1,9" fill="${sel ? C.gold : danger ? C.red : C.glass2}" stroke="${col}" stroke-width="2.5"/></svg>`;
	document.body.insertAdjacentHTML('beforeend', dot);
	const br = h => `<svg width="22" height="${h}" style="flex:none"><path d="M20,2 Q2,${h / 2} 20,${h - 2}" fill="none" stroke="${C.gold}" stroke-width="3.5"/><line x1="11" y1="${h / 2}" x2="0" y2="${h / 2}" stroke="${C.gold}" stroke-width="3"/></svg>`;
	const brR = h => `<svg width="22" height="${h}" style="flex:none"><path d="M2,2 Q20,${h / 2} 2,${h - 2}" fill="none" stroke="${C.gold}" stroke-width="3.5"/><line x1="11" y1="${h / 2}" x2="22" y2="${h / 2}" stroke="${C.gold}" stroke-width="3"/></svg>`;
	const inner = `<span style="font-weight:700;font-size:20px;color:${sel ? C.gold : C.icon};margin-right:16px">0${i + 1}</span><span class="out" style="font-weight:700;font-size:${size}px;letter-spacing:.1em;color:${col}">${o.key}</span>`;
	const html = sel
		? `<div style="display:flex;align-items:center;gap:10px;white-space:nowrap">${br(76)}<div style="display:flex;align-items:center;padding:0 16px;height:76px;background:linear-gradient(90deg, rgba(255,230,166,.26), rgba(255,230,166,.05))">${inner}</div>${brR(76)}</div>`
		: `<div style="display:flex;align-items:center;white-space:nowrap;height:76px;padding-left:42px">${inner}</div>`;
	place(html, `left:${x + 24}px;top:${y - 38}px`);
});

// --- Callout (hub): a small Ledger & Brass slip ---
if (!SET) {
	const o = DATA.options[SEL], [x, y] = optPos(SEL), cw = 380, ch = 196, left = 962;
	place(`<svg width="${left - x - 470}" height="20"><line x1="0" y1="10" x2="${left - x - 484}" y2="10" stroke="${C.gold}" stroke-width="2"/><polygon points="${left - x - 484},4 ${left - x - 472},10 ${left - x - 484},16" fill="${C.gold}"/></svg>`, `left:${x + 470}px;top:${y - 10}px`);
	place(`${brassPlate(cw, ch, { stroke: C.salt })}
		<div class="abs" style="left:34px;top:26px;right:30px">
			<div style="font-weight:700;font-size:22px;letter-spacing:.18em;color:${C.gold}">${o.key}</div>
			<div style="margin-top:8px">${rulerSVG(cw - 64, { step: 10, major: 5, opacity: .35, long: 7, short: 3 })}</div>
			<div style="margin-top:10px;font-size:23px;line-height:1.3">${o.help}</div>
			<div style="margin-top:6px;font-size:19px;color:${C.label}">${o.sub}</div></div>`,
		`left:${left}px;top:${y - 52}px;width:${cw}px;height:${ch}px`);
}

// --- Session slip, bottom-left under the wheel ---
(function session() {
	const s = DATA.session, w = 610, h = 112;
	place(`${brassPlate(w, h, { op: .6 })}
		<div class="abs" style="left:36px;top:22px;right:36px;white-space:nowrap">
			<div style="display:flex;align-items:center;gap:14px">${icon('save', 26, C.icon)}<span style="font-weight:700;font-size:24px;letter-spacing:.14em">SESSION ${s.slot}<span style="color:${C.label};font-weight:400"> OF ${s.of}</span></span>
				<span style="flex:1"></span><span style="font-size:15px;letter-spacing:.3em;color:${C.icon}">${s.serial}</span></div>
			<div style="display:flex;gap:30px;margin-top:10px;font-size:21px;color:${C.label}"><span>PLAYED <b style="color:${C.text}">${s.played}</b></span><span>SAVED <b style="color:${C.text}">${s.saved}</b></span></div></div>`,
		`left:96px;top:${1080 - 84 - h}px;width:${w}px;height:${h}px`);
})();

// --- The ledger, top-right: hero and mission on one plate with ruled rows ---
if (!SET) {
	const h = DATA.hero, w = 440, x = 1920 - 96 - w, y = 84, ph = 520;
	const line = (label, value, extra = '') => `<div style="display:flex;align-items:center;height:46px;border-bottom:1px solid rgba(212,238,244,.16)">
		<span style="width:132px;font-size:16px;letter-spacing:.2em;color:${C.label}">${label}</span>${extra}<span style="flex:1"></span><span style="font-weight:700;font-size:22px">${value}</span></div>`;
	let steps = '';
	for (let i = 0; i < DATA.mission.steps; i++) steps += `<svg width="16" height="16"><polygon points="8,1 15,8 8,15 1,8" fill="${i < DATA.mission.step - 1 ? C.salt : i === DATA.mission.step - 1 ? C.gold : 'none'}" stroke="${i === DATA.mission.step - 1 ? C.gold : C.icon}" stroke-width="1.8"/></svg>`;
	place(`${brassPlate(w, ph)}
		<div class="abs" style="left:34px;top:30px;right:34px;white-space:nowrap">
			<div style="display:flex;align-items:center;gap:20px"><div style="position:relative">${portraitHTML(104)}<div class="abs" style="left:-10px;bottom:-8px">${gemSVG(42, h.level)}</div></div><div style="flex:1">
				<div style="font-size:16px;letter-spacing:.26em;color:${C.label}">HERO</div>
				<div style="font-weight:700;font-size:30px;letter-spacing:.05em">${h.name}</div>
				<div style="margin-top:8px">${barSVG(236, 14, h.hp / h.hpMax, C.salt, { notches: 4 })}</div></div></div>
			<div style="margin-top:14px;border-top:1px solid rgba(212,238,244,.16)">
				${line('HEALTH', `${h.hp} <span style="color:${C.label};font-weight:400">/ ${h.hpMax}</span>`)}
				${line('LEVEL ' + h.level, `${fmt(h.xp)} <span style="color:${C.label};font-weight:400">/ ${fmt(h.xpNext)} XP</span>`)}
				${line('SOUL FLASK', `${DATA.flask.charges} / ${DATA.flask.max}`, `<span style="display:flex;gap:10px;align-items:center">${icon('flask', 24, C.heal)}${pipsSVG(3, DATA.flask.charges, C.heal, { h: 18, w: 10 })}</span>`)}
				${line('GRENADES', `${DATA.grenades.count} / ${DATA.grenades.max}`, `<span style="display:flex;gap:10px;align-items:center">${icon('grenade', 24, C.icon)}${pipsSVG(3, DATA.grenades.count, C.salt, { h: 18, w: 10 })}</span>`)}
			</div>
			<div style="margin-top:22px;display:flex;align-items:center;justify-content:space-between"><span style="font-size:16px;letter-spacing:.26em;color:${C.label}">TRACKED MISSION</span>
				<span style="display:flex;align-items:center;gap:5px">${steps}<b style="margin-left:6px;font-size:20px;color:${C.label}">${DATA.mission.step} / ${DATA.mission.steps}</b></span></div>
			<div style="display:flex;align-items:center;gap:12px;margin-top:8px">${icon('mission', 28, C.gold)}<span style="font-weight:700;font-size:30px;letter-spacing:.1em">${DATA.mission.name}</span></div>
			<div style="font-size:23px;margin-top:8px">${DATA.mission.text}</div>
			<div style="font-size:18px;margin-top:6px;color:${C.label}"><span style="letter-spacing:.14em">OPTIONAL:</span> ${DATA.mission.optional}</div></div>`,
		`left:${x}px;top:${y}px;width:${w}px;height:${ph}px`);
}

// --- Settings: a wide Ledger & Brass plate to the right of the words ---
if (SET) {
	const x = 940, y = 84, w = 1920 - 96 - x, h = 730;
	const [ox, oy] = optPos(SEL);
	place(`<svg width="${x - ox - 440}" height="20"><line x1="0" y1="10" x2="${x - ox - 452}" y2="10" stroke="${C.gold}" stroke-width="2.5"/><polygon points="${x - ox - 454},3 ${x - ox - 440},10 ${x - ox - 454},17" fill="${C.gold}"/></svg>`, `left:${ox + 440}px;top:${oy - 10}px`);
	let tabs = '';
	DATA.tabs.forEach((t, i) => tabs += `<div style="flex:1;text-align:center;font-weight:700;font-size:22px;letter-spacing:.06em;color:${i === 0 ? C.gold : C.label}">${t}</div>`);
	let rows = '';
	DATA.rows.forEach((r, i) => {
		const hov = i === 0;
		let ctl = '';
		if (r.kind === 'choice') ctl = `<div style="display:flex;gap:10px">${r.values.map((v, j) => `<div style="height:44px;padding:0 14px;display:flex;align-items:center;font-weight:700;font-size:20px;letter-spacing:.06em;clip-path:polygon(8px 0,100% 0,calc(100% - 8px) 100%,0 100%);
			background:${j === r.sel ? C.gold : 'rgba(212,238,244,.12)'};color:${j === r.sel ? '#12151A' : C.label}">${v}</div>`).join('')}</div>`;
		else {
			const f = (r.value - r.min) / (r.max - r.min), sw = 250;
			ctl = `<div style="display:flex;align-items:center;gap:16px"><div style="position:relative;width:${sw}px;height:44px">
				<div class="abs" style="left:0;top:12px">${rulerSVG(sw, { step: sw / 20, major: 5, opacity: .5, long: 12, short: 6 })}</div>
				<div class="abs" style="left:0;top:10px;width:${sw * f}px;height:4px;background:${C.gold}"></div>
				<svg class="abs" style="left:${sw * f - 12}px;top:-2px" width="24" height="24"><circle cx="12" cy="12" r="9" fill="${C.glass2}" stroke="${C.gold}" stroke-width="3"/><line x1="12" y1="0" x2="12" y2="24" stroke="${C.gold}" stroke-width="2"/><line x1="0" y1="12" x2="24" y2="12" stroke="${C.gold}" stroke-width="2"/></svg>
				<div class="abs" style="left:-6px;top:28px;font-size:15px;color:${C.label}">${r.min}°</div><div class="abs" style="right:-14px;top:28px;font-size:15px;color:${C.label}">${r.max}°</div></div>
				<span style="font-weight:700;font-size:28px;width:62px;text-align:right">${r.value}°</span></div>`;
		}
		rows += `<div style="position:relative;display:flex;align-items:center;height:88px;border-bottom:1px solid rgba(212,238,244,.16)">
			${hov ? `<div class="abs" style="left:70px;right:0;top:8px;bottom:8px;background:linear-gradient(90deg, rgba(255,230,166,.24), rgba(255,230,166,.02))"></div>
				<svg class="abs" style="left:0;top:22px" width="70" height="44"><polygon points="0,0 70,0 60,22 70,44 0,44" fill="${C.gold}"/></svg>` : ''}
			<span style="position:relative;width:70px;text-align:center;font-size:24px;color:${hov ? '#12151A' : C.label};font-weight:700;padding-right:8px">0${i + 1}</span>
			<span style="position:relative;flex:1;padding-left:22px;font-weight:700;font-size:27px;color:${hov ? C.gold : C.text}">${r.name}</span>
			<span style="position:relative;padding-right:14px">${ctl}</span></div>`;
	});
	place(`${brassPlate(w, h)}
		<div class="abs" style="left:44px;top:36px;right:44px">
			<div style="display:flex;align-items:center;gap:20px"><span style="font-weight:700;font-size:44px;letter-spacing:.14em">SETTINGS</span><span style="flex:1;padding-top:10px">${rulerSVG(w - 88 - 300, { step: 15, major: 4, opacity: .5 })}</span></div>
			<div style="display:flex;align-items:center;gap:16px;margin-top:30px"><span class="cap">PGUP</span><div style="flex:1;display:flex">${tabs}</div><span class="cap">PGDN</span></div>
			<div style="position:relative;margin-top:14px">${rulerSVG(w - 88, { step: 12, major: 5, opacity: .45 })}
				<div class="abs" style="left:${70 + 0}px;top:-6px;width:${(w - 88 - 140) / 4}px;height:4px;background:${C.gold}"></div></div>
			<div style="position:relative;margin-top:16px">
				<div class="abs" style="left:69px;top:0;width:2px;height:${3 * 88}px;background:rgba(212,238,244,.3)"></div>${rows}</div>
			<div style="margin-top:34px;display:flex;gap:24px"><div style="width:4px;background:${C.gold};flex:none"></div><div>
				<div style="font-weight:700;font-size:23px;letter-spacing:.16em;color:${C.gold}">${DATA.help.title}</div>
				<div style="font-size:22px;line-height:1.4;margin-top:8px">${DATA.help.text}</div>
				<div style="font-size:20px;line-height:1.4;margin-top:10px;color:${C.label}">${DATA.help.detail}</div></div></div></div>`,
		`left:${x}px;top:${y}px;width:${w}px;height:${h}px`);
}

place(keysHTML(SET ? SET_KEYS : HUB_KEYS), `right:96px;top:982px`);
