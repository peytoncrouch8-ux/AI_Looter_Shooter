// Concept B "Open Range": no plates at all. The HUD's floating outlined type on a dark sweep from the left edge, a ruler
// down the left with a gold pointer at the lit word, the callout a floating sentence on a hairline with a gold arrowhead,
// hero and mission as floating HUD-style type top-right. MODE ('hub' | 'settings') is set by the page.
const SET = MODE === 'settings';
const SEL = 1;
document.body.innerHTML = '<div class="world"></div><div class="vignette"></div>';
place('', `left:0;top:0;width:1920px;height:1080px;background:linear-gradient(90deg, rgba(8,10,13,.94) 0%, rgba(8,10,13,.82) 28%, rgba(8,10,13,${SET ? .7 : .3}) 52%, rgba(8,10,13,${SET ? .7 : .05}) 70%, rgba(8,10,13,${SET ? .78 : .55}) 100%)`);
place('', 'left:0;top:0;width:1920px;height:220px;background:linear-gradient(180deg, rgba(8,10,13,.6), rgba(8,10,13,0))');
place('', 'left:0;top:860px;width:1920px;height:220px;background:linear-gradient(0deg, rgba(8,10,13,.75), rgba(8,10,13,0))');

// --- Header: PAUSED as hollow salt type, the area under it in flavour gold with a slim slanted bar ---
place(`<div style="font-weight:700;font-size:128px;line-height:1;letter-spacing:.06em;color:transparent;-webkit-text-stroke:2.5px ${C.salt}">PAUSED</div>
	<div style="display:flex;align-items:center;gap:16px;margin-top:14px">
		<svg width="90" height="10"><polygon points="7,0 90,0 83,10 0,10" fill="${C.flavor}"/></svg>
		<span class="out" style="font-weight:700;font-size:26px;letter-spacing:.3em;color:${C.flavor}">${DATA.area}</span></div>`, 'left:126px;top:84px');

// --- The ruler down the left edge with the gold pointer ---
const X = 150, Y0 = 326, GAP = 112;
const rowY = i => Y0 + i * GAP + (i === 3 ? 36 : 0);
(function ruler() {
	let s = '';
	const top = 300, bot = 760;
	s += `<line x1="0" y1="${top}" x2="0" y2="${bot}" stroke="${C.salt}" stroke-opacity=".45" stroke-width="2"/>`;
	for (let y = top, i = 0; y <= bot; y += 10, i++) s += `<line x1="0" y1="${y}" x2="${i % 5 ? 7 : 16}" y2="${y}" stroke="${C.salt}" stroke-opacity="${i % 5 ? .3 : .6}" stroke-width="${i % 5 ? 1 : 2}"/>`;
	const py = rowY(SEL) + 36;
	s += `<polygon points="0,${py - 14} 22,${py} 0,${py + 14}" fill="${C.gold}"/><line x1="-14" y1="${py}" x2="0" y2="${py}" stroke="${C.gold}" stroke-width="3"/>`;
	place(`<svg width="40" height="1080">${s}</svg>`, 'left:96px;top:0');
})();

// --- The four words ---
DATA.options.forEach((o, i) => {
	const sel = i === SEL, danger = !!o.danger;
	const col = sel ? C.gold : danger ? C.red : C.text;
	const size = sel ? 66 : 54;
	const num = `<span style="font-weight:700;font-size:22px;letter-spacing:.1em;color:${sel ? C.gold : C.icon};width:46px;display:inline-block">0${i + 1}</span>`;
	const bar = sel ? `<svg class="abs" style="left:0;bottom:-12px" width="100%" height="8" viewBox="0 0 100 8" preserveAspectRatio="none"><polygon points="2,0 100,0 98,8 0,8" fill="${C.gold}"/></svg>` : '';
	place(`<div style="display:flex;align-items:center;white-space:nowrap">${num}<span class="out" style="position:relative;font-weight:700;font-size:${size}px;letter-spacing:.08em;color:${col};opacity:${sel || danger ? 1 : .86}">${o.key}${bar}</span></div>`,
		`left:${X}px;top:${rowY(i) + (sel ? 0 : 8)}px`);
});

// --- Callout (hub): a floating sentence on a hairline, the level-up's gold arrowhead ---
if (!SET) {
	const o = DATA.options[SEL], y = rowY(SEL) + 42, x0 = 560;
	place(`<svg width="120" height="20"><line x1="0" y1="10" x2="104" y2="10" stroke="${C.gold}" stroke-width="2"/><polygon points="104,3 118,10 104,17" fill="${C.gold}"/></svg>`, `left:${x0}px;top:${y - 10}px`);
	place(`<div class="out" style="font-weight:700;font-size:22px;letter-spacing:.2em;color:${C.gold}">${o.key}</div>
		<div class="out" style="font-size:27px;line-height:1.3;margin-top:6px;width:430px">${o.help}</div>
		<div class="out" style="font-size:21px;margin-top:6px;color:${C.label}">${o.sub}</div>`, `left:${x0 + 140}px;top:${y - 26}px`);
}

// --- Session, bottom-left: one floating line ---
(function session() {
	const s = DATA.session;
	place(`<div style="display:flex;align-items:center;gap:14px;white-space:nowrap">${icon('save', 26, C.icon)}
		<span class="out" style="font-weight:700;font-size:24px;letter-spacing:.14em">SESSION ${s.slot}<span style="color:${C.label};font-weight:400"> / ${s.of}</span></span>
		<span style="width:2px;height:24px;background:rgba(212,238,244,.35)"></span>${icon('clock', 24, C.icon)}
		<span class="out" style="font-size:23px;color:${C.label}">PLAYED <b style="color:${C.text}">${s.played}</b></span>
		<span style="width:2px;height:24px;background:rgba(212,238,244,.35)"></span>
		<span class="out" style="font-size:23px;color:${C.label}">SAVED <b style="color:${C.text}">${s.saved}</b></span></div>
		<div style="margin-top:12px;font-size:15px;letter-spacing:.34em;color:${C.icon}">${s.serial}</div>`, 'left:126px;top:960px');
})();

// --- Hero and mission, top-right, floating like the HUD ---
if (!SET) {
	const h = DATA.hero;
	place(`<div style="display:flex;align-items:center;gap:22px;white-space:nowrap">
		<div style="text-align:right">
			<div class="out" style="font-weight:700;font-size:34px;letter-spacing:.08em">${h.name}</div>
			<div style="display:flex;justify-content:flex-end;align-items:center;gap:12px;margin-top:10px">
				<span class="out" style="font-weight:700;font-size:22px">${h.hp}<span style="color:${C.label};font-weight:400"> / ${h.hpMax}</span></span>${barSVG(300, 18, h.hp / h.hpMax, C.salt, { notches: 4, edgeOpacity: .7 })}</div>
			<div style="display:flex;justify-content:flex-end;align-items:center;gap:12px;margin-top:8px">
				<span class="out" style="font-size:19px;color:${C.label}">${fmt(h.xp)} / ${fmt(h.xpNext)} XP</span>${barSVG(300, 8, h.xp / h.xpNext, C.flavor, { edgeOpacity: .4 })}</div>
			<div style="display:flex;justify-content:flex-end;align-items:center;gap:12px;margin-top:12px">
				${icon('flask', 28, C.heal)}${pipsSVG(3, DATA.flask.charges, C.heal)}<span style="width:18px"></span>${icon('grenade', 28, C.icon)}${pipsSVG(3, DATA.grenades.count, C.salt)}</div>
		</div>
		<div style="position:relative">${portraitHTML(150)}<div class="abs" style="left:-14px;bottom:-6px">${gemSVG(50, h.level)}</div></div></div>`, 'right:96px;top:84px');

	const m = DATA.mission;
	let steps = '';
	for (let i = 0; i < m.steps; i++) steps += `<svg width="18" height="18"><polygon points="9,1 17,9 9,17 1,9" fill="${i < m.step - 1 ? C.salt : i === m.step - 1 ? C.gold : 'rgba(8,10,13,.6)'}" stroke="${i === m.step - 1 ? C.gold : C.icon}" stroke-width="1.8"/></svg>`;
	place(`<div style="text-align:right;white-space:nowrap">
		<div style="display:flex;justify-content:flex-end">${rulerSVG(380, { step: 10, major: 6, opacity: .45, long: 9, short: 4 })}</div>
		<div class="out" style="font-size:18px;letter-spacing:.3em;color:${C.label};margin-top:12px">TRACKED MISSION</div>
		<div style="display:flex;justify-content:flex-end;align-items:center;gap:14px;margin-top:6px"><span class="out" style="font-weight:700;font-size:36px;letter-spacing:.1em">${m.name}</span>${icon('mission', 34, C.gold)}</div>
		<div style="display:flex;justify-content:flex-end;align-items:center;gap:6px;margin-top:6px">${steps}<span class="out" style="font-weight:700;font-size:21px;color:${C.label};margin-left:8px">${m.step} / ${m.steps}</span></div>
		<div class="out" style="font-size:27px;margin-top:10px">${m.text}</div>
		<div class="out" style="font-size:19px;margin-top:6px;color:${C.label}"><span style="letter-spacing:.14em">OPTIONAL:</span> ${m.optional}</div></div>`, 'right:96px;top:352px');
}

// --- Settings: floating type on the right, no boxes ---
if (SET) {
	const x0 = 790, w = 1920 - 96 - x0;
	const ly = rowY(SEL) + 42;
	place(`<svg width="200" height="20"><line x1="0" y1="10" x2="184" y2="10" stroke="${C.gold}" stroke-width="2"/><polygon points="184,3 198,10 184,17" fill="${C.gold}"/></svg>`, `left:${x0 - 236}px;top:${ly - 10}px`);
	let tabs = '';
	DATA.tabs.forEach((t, i) => tabs += `<div style="position:relative;padding:0 6px"><span class="out" style="font-weight:700;font-size:30px;letter-spacing:.12em;color:${i === 0 ? C.gold : C.label}">${t}</span>
		${i === 0 ? `<svg class="abs" style="left:6px;top:46px" width="170" height="8"><polygon points="6,0 170,0 164,8 0,8" fill="${C.gold}"/></svg>` : ''}</div>`);
	let rows = '';
	DATA.rows.forEach((r, i) => {
		const hov = i === 0;
		let ctl = '';
		if (r.kind === 'choice') ctl = `<div style="display:flex;gap:26px">${r.values.map((v, j) => `<div style="position:relative"><span class="out" style="font-weight:700;font-size:25px;letter-spacing:.08em;color:${j === r.sel ? C.gold : C.label};opacity:${j === r.sel ? 1 : .8}">${v}</span>
			${j === r.sel ? `<svg class="abs" style="left:-4px;top:36px" width="${v.length * 18 + 8}" height="6"><polygon points="5,0 ${v.length * 18 + 8},0 ${v.length * 18 + 3},6 0,6" fill="${C.gold}"/></svg>` : ''}</div>`).join('')}</div>`;
		else {
			const f = (r.value - r.min) / (r.max - r.min);
			ctl = `<div style="display:flex;align-items:center;gap:14px"><span class="out" style="font-size:18px;color:${C.label}">${r.min}°</span>
				<div style="position:relative">${barSVG(280, 14, f, C.gold, { edgeOpacity: .6 })}
				<svg class="abs" style="left:${280 * f - 9}px;top:-8px" width="18" height="30"><polygon points="4,0 18,0 14,30 0,30" fill="${C.text}" stroke="${C.ink}" stroke-width="2"/></svg></div>
				<span class="out" style="font-size:18px;color:${C.label}">${r.max}°</span><span class="out" style="font-weight:700;font-size:30px;width:70px;text-align:right">${r.value}°</span></div>`;
		}
		rows += `<div style="position:relative;height:90px;display:flex;align-items:center;border-bottom:1.5px solid rgba(212,238,244,.16)">
			${hov ? `<svg class="abs" style="left:-34px;top:31px" width="20" height="28"><polygon points="0,0 20,14 0,28" fill="${C.gold}"/></svg>` : ''}
			<span class="out" style="font-size:22px;width:56px;color:${hov ? C.gold : C.icon}">0${i + 1}</span>
			<span class="out" style="flex:1;font-weight:700;font-size:30px;color:${hov ? C.gold : C.text}">${r.name}</span>${ctl}</div>`;
	});
	place(`<div style="display:flex;align-items:center;gap:22px"><span class="out" style="font-weight:700;font-size:54px;letter-spacing:.12em">SETTINGS</span></div>
		<div style="margin-top:12px">${rulerSVG(w, { step: 12, major: 5, opacity: .45 })}</div>
		<div style="display:flex;align-items:center;gap:30px;margin-top:30px"><span class="cap">PGUP</span>${tabs}<span class="cap">PGDN</span></div>
		<div style="margin-top:42px;border-top:1.5px solid rgba(212,238,244,.16)">${rows}</div>
		<div style="display:flex;gap:22px;margin-top:40px"><svg width="10" height="150" style="flex:none"><polygon points="5,0 10,0 5,150 0,150" fill="${C.gold}"/></svg>
			<div><div class="out" style="font-weight:700;font-size:24px;letter-spacing:.2em;color:${C.gold}">${DATA.help.title}</div>
			<div class="out" style="font-size:24px;line-height:1.38;margin-top:8px">${DATA.help.text}</div>
			<div class="out" style="font-size:20px;line-height:1.4;margin-top:10px;color:${C.label}">${DATA.help.detail}</div></div></div>`,
		`left:${x0}px;top:94px;width:${w}px`);
}

place(keysHTML(SET ? SET_KEYS : HUB_KEYS), `right:96px;top:982px`);
