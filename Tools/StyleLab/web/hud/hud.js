// The game's gameplay HUD as an HTML/CSS/SVG overlay: player frame with portrait, weapon column with the cartridge,
// minimap, mission tracker, crosshair, edge flash, level-up banner and boss bar, as Docs/HudMockup/NewHud.dc.html draws
// them (text a third bigger, a hit flashes only the screen's edges). The API is the Style Lab contract's; see the notes at
// each method. Everything animated runs on the HUD's own clock (update(dt) advances it), so freeze() holds a still.
import { buildMarkup } from './markup.js';
import { ICONS, AMMO, FONT } from './assets.js';
import { Effects, clamp, lerp, easeIn, easeOut, setText, setAttr, setStyle, setClass } from './hud_util.js';

const DESIGN_W = 1920;
const DESIGN_H = 1080;
const LOW_FRACTION = 0.3;          // UHudPlayerFrameWidget::LowFraction
const CHIP_HOLD = 0.45;            // the lost health lingers this long after a hit, then drains
const CHIP_DRAIN = 0.6;
const HEAL_RISE = 0.5;
const EDGE_HIT_SECONDS = 0.55;     // UHudScreenEdgeWidget
const EDGE_HIT_PEAK = 0.85;
const EDGE_LOW = [0.25, 0.6];
const EDGE_STRENGTH = 0.39;
const LOW_BEAT = 0.9;
const MAG_FILL_RUN = 211;          // the cartridge's fill, in its drawing units (UHudMagazineWidget)
const MAG_DRAIN_SPEED = 18;
const MAG_LOW = 0.25;
const COUNT_ROOM = 44 / 0.85;      // the counts shrink to fit this wide (44 px on screen), in the cartridge's units
const MAP_RANGE = 35;              // metres from the player to the minimap's edge (BaseRange 3500 cm)
const MAP_RADIUS = 86 - 4;         // pixels from its centre to that range
const OBJECTIVE_HOLD = 1.4;        // a finished objective stays ticked this long before the next slides in
const RARITY = ['#d8e2ea', '#6dff7a', '#4aa8ff', '#cb7cf3', '#ffcb00'];   // Common .. Legendary
const RELOAD_SECONDS = { rifle: 1.7, shotgun: 2.2 };
const PHASES = ['THE BROOD STIRS', 'OLD HUNGER', 'UNDER SILK'];
const DUR = {
	flinch: 0.32, hurt: 0.45, squint: 0.6, calm: 0.6, flare: 1.8, ring: 1.1, gem: 0.9, shine: 0.75, heal: 1.2, gain: 1.5,
	xpfloat: 1.4, kick: 0.16, hit: 0.18, hitkill: 0.3, phase: 1.6, banner: 2.8, objin: 0.45, count: 0.3, feed: 2.6,
};


const DEFAULT_SLOTS = [
	{ icon: 'rifle', rarity: 3, name: 'WHISPER BULLPUP', fireMode: 'AUTO' },
	{ icon: 'shotgun', rarity: 4, name: 'GILDED RANCHHAND', fireMode: 'PUMP' },
	{ icon: null, rarity: 0, name: '', fireMode: '' },
];

function ensureResources() {
	const head = document.head;
	if (!head.querySelector('style[data-lh-font]')) {
		// The bundled face (assets.js), used where Google Fonts doesn't load (headless screenshots, offline).
		const face = document.createElement('style');
		face.dataset.lhFont = '1';
		face.textContent = `@font-face { font-family: 'Chakra Petch Local'; font-style: normal; font-weight: 700; font-display: swap; src: url(${FONT}) format('woff2'); }`;
		head.appendChild(face);
	}
	let cssLink = head.querySelector('link[data-lh-css]');
	if (!cssLink) {
		cssLink = document.createElement('link');
		cssLink.rel = 'stylesheet';
		cssLink.href = new URL('./hud.css', import.meta.url).href;
		cssLink.dataset.lhCss = '1';
		head.appendChild(cssLink);
	}
	const cssReady = new Promise((resolve) => {
		if (cssLink.sheet) return resolve();
		cssLink.addEventListener('load', () => resolve());
		cssLink.addEventListener('error', () => resolve());
	});
	if (!head.querySelector('link[href*="family=Chakra+Petch"]')) {
		const pre = document.createElement('link');
		pre.rel = 'preconnect';
		pre.href = 'https://fonts.googleapis.com';
		const font = document.createElement('link');
		font.rel = 'stylesheet';
		font.href = 'https://fonts.googleapis.com/css2?family=Chakra+Petch:wght@400;600;700&display=swap';
		head.append(pre, font);
	}
	return cssReady;
}

class Hud {
	constructor(root, container) {
		this.root = root;
		this.container = container;
		this.el = {};
		root.querySelectorAll('[data-r]').forEach((e) => { this.el[e.dataset.r] = e; });
		this.fx = new Effects();
		this.frozen = false;
		this.live = false;
		this.s = {};
		this.size = [0, 0];
		this.hp = { init: false, shown: 1, target: 1, chipFrom: 1, holdUntil: -1e9, rise: null, last: 0, max: 0, low: false, lastHit: -1e9, lastHeal: -1e9 };
		this.xpGain = null;
		this.mag = { shown: -1, reloadAt: 0, wasReloading: false, key: '' };
		this.slotKeys = ['', '', ''];
		this.map = { image: null, rect: null, northUp: false, key: '', cx: 0, cz: 0, yaw: 0 };
		this.mission = { cur: null, pending: null, doneAt: -1e9, segs: 0, key: '' };
		this.boss = { cur: null, shown: 1, chipFrom: 1, holdUntil: -1e9, phase: '' };
		this.feed = [];
		this.feedId = 0;
		this.hitAt = -1e9;
		this.lastEdge = -1;

		const e = this.el;
		this.fx.loop(e.blink, 5.2);
		this.fx.loop(e.breath, 4.2);
		this.fx.loop(e.hpfill, LOW_BEAT);
		this.fx.loop(e.lowfx, LOW_BEAT);
		this.fx.loop(e.magline, 0.6);
		for (let i = 0; i < 3; i++) this._applySlot(i, DEFAULT_SLOTS[i], false);
		setText(e.place, '');
		this.ready = ensureResources().then(() => (document.fonts && document.fonts.ready ? document.fonts.ready : null)).catch(() => {});
		this.update(0, {});
		this.setMinimap(null, null);
	}

	// ---- placement ----

	/** Keeps the 1920 x 1080 design scaled uniformly to the canvas; the clusters keep their own corners if the shape differs. */
	resize(w, h) {
		if (!(w > 0 && h > 0)) return;
		this.size = [w, h];
		const k = Math.min(w / DESIGN_W, h / DESIGN_H);
		const stage = this.el.stage;
		stage.style.width = (w / k) + 'px';
		stage.style.height = (h / k) + 'px';
		stage.style.transform = `scale(${k})`;
		stage.style.setProperty('--lh-ex', String(w / k - DESIGN_W));
		stage.style.setProperty('--lh-ey', String(h / k - DESIGN_H));
	}

	setVisible(visible) { this.root.style.display = visible ? '' : 'none'; }

	// ---- the clock ----

	/** Stops every clock at time t (seconds on the HUD's clock; omitted: where it is). Effects already running keep their age. */
	freeze(t) {
		this.frozen = true;
		this.root.classList.add('lh-frozen');
		if (typeof t === 'number') this.fx.rebase(t);
		this.update(0, this.s);
	}

	unfreeze() {
		this.frozen = false;
		this.root.classList.remove('lh-frozen');
	}

	/** While frozen, moves the clock on by dt (a still of "0.15 s after the hit": freeze(0), fire the event, step(0.15)). */
	step(dt) {
		this.fx.now += dt;
		this.update(0, this.s);
	}

	get time() { return this.fx.now; }

	// ---- per frame ----

	/**
	 * Call every frame. s: { yawDeg, pos: [x, z], health, maxHealth, level, xp, xpNext, mag, magMax, reserve, slot,
	 * slots: [{ icon, rarity, name, fireMode }], reloading, lowAmmo, reloadSeconds? } (all optional).
	 */
	update(dt, s) {
		s = s || {};
		dt = clamp(dt || 0, 0, 0.25);
		if (!this.frozen) this.fx.now += dt;
		this.s = s;
		// Until the page sends a real state the HUD shows defaults and treats the first real one as where it starts
		// (no hit, heal or reload effect for jumping from the defaults to it).
		if (!this.live && Object.keys(s).length) { this.live = true; this.hp.init = false; this.mag.shown = -1; }
		const snap = this.frozen;
		this._health(s);
		this._xp(s);
		this._weapons(s, dt, snap);
		this._minimap(s);
		this._missionTick();
		this._bossTick();
		this._feedTick();
		this._edges();
		this.fx.tick();
	}

	// ---- health, portrait ----

	_shownHealth(now) {
		const h = this.hp;
		if (!h.rise) return h.target;
		const p = clamp((now - h.rise.t0) / HEAL_RISE, 0, 1);
		if (p >= 1) { h.rise = null; return h.target; }
		return lerp(h.rise.from, h.rise.to, easeOut(p));
	}

	_chip(now, shown, target) {
		const h = this.hp;
		if (now < h.holdUntil) return h.chipFrom;
		const p = clamp((now - h.holdUntil) / CHIP_DRAIN, 0, 1);
		return Math.max(target, lerp(h.chipFrom, target, easeIn(p)));
	}

	_health(s) {
		const now = this.fx.now, h = this.hp, e = this.el;
		const max = Math.max(1, s.maxHealth ?? 100);
		const health = clamp(s.health ?? max, 0, max);
		const f = health / max;
		const low = health > 0 && f <= LOW_FRACTION;
		if (!h.init) {
			Object.assign(h, { init: true, shown: f, target: f, chipFrom: f, holdUntil: -1e9, rise: null, last: health, max, low });
		} else if (max !== h.max) {
			// Max health changed (a level-up): the bar re-measures, no hit or heal.
			Object.assign(h, { shown: f, target: f, chipFrom: Math.max(f, h.chipFrom * h.max / max), rise: null, last: health, max });
		} else if (health < h.last - 0.01) {
			h.chipFrom = Math.max(this._chip(now, 0, h.target), this._shownHealth(now));
			h.holdUntil = now + CHIP_HOLD;
			h.rise = null;
			h.target = f;
			h.low = low;
			this._hurtFx();
		} else if (health > h.last + 0.01) {
			const from = this._shownHealth(now);
			h.rise = { from, to: f, t0: now };
			h.target = f;
			h.chipFrom = f;
			h.holdUntil = -1e9;
			this._healFx(health - h.last);
		} else {
			h.target = f;
		}
		h.last = health;
		if (low !== h.low) {
			h.low = low;
			if (!low && this.hp.init) this._calmReturn();
		}
		const shown = this._shownHealth(now);
		const chip = this._chip(now, shown, h.target);
		setStyle(e.hpfill, 'width', (shown * 100).toFixed(2) + '%');
		setStyle(e.hpchip, 'width', (Math.max(chip, shown) * 100).toFixed(2) + '%');
		setText(e.hpval, Math.round(health));
		setText(e.hpmax, '/ ' + Math.round(max));
		setClass(e.hpfill, 'lh-low', low);
		setClass(e.hpnum, 'lh-low', low);
		setClass(e.lowfx, 'lh-low', low);
		// The calm eyes show unless health is low, when the squint holds (a hit or a heal plays the crossfade instead).
		if (!this.fx.running(e.calmfx)) setAttr(e.calmfx, 'opacity', low ? 0 : 1);
		if (!this.fx.running(e.squint)) setAttr(e.squint, 'opacity', low ? 1 : 0);
	}

	_hurtFx() {
		const now = this.fx.now, e = this.el;
		if (now - this.hp.lastHit < 0.08) return;
		this.hp.lastHit = now;
		this.hitAt = now;
		this.fx.play(e.face, 'flinch', DUR.flinch);
		this.fx.play(e.hurtfx, 'hurt', DUR.hurt);
		if (!this.hp.low) {
			this.fx.play(e.squint, 'squint', DUR.squint);
			this.fx.play(e.calmfx, 'calm', DUR.calm);
		}
	}

	_calmReturn() {
		this.fx.play(this.el.squint, 'squint', DUR.squint);
		this.fx.play(this.el.calmfx, 'calm', DUR.calm);
	}

	_healFx(amount) {
		const now = this.fx.now, e = this.el;
		if (now - this.hp.lastHeal < 0.08) return;
		this.hp.lastHeal = now;
		if (amount > 0) setText(e.healfloat, '+' + Math.round(amount));
		this.fx.play(e.hpshine, 'shine', DUR.shine);
		this.fx.play(e.healfloat, 'heal', DUR.heal);
	}

	/** The player was hit: the portrait flinches, its window flashes, the edges flash and the bar's chip lingers. */
	onPlayerHit(amount) {
		const h = this.hp, now = this.fx.now;
		h.chipFrom = Math.max(this._chip(now, 0, h.target), this._shownHealth(now));
		h.holdUntil = now + CHIP_HOLD;
		this._hurtFx();
		this.fx.tick();
	}

	/** Health came back: a pale shine sweeps the bar and "+amount" rises. (The bar itself follows update()'s health.) */
	onHeal(amount) {
		this._healFx(amount);
		this.fx.tick();
	}

	// ---- experience, level ----

	_xp(s) {
		const e = this.el, now = this.fx.now;
		const next = Math.max(1, s.xpNext ?? 1000), xp = clamp(s.xp ?? 0, 0, next), at = xp / next;
		const gain = this.xpGain;
		const gaining = gain && now - gain.start < DUR.gain;
		for (let i = 0; i < 10; i++) {
			const lo = i / 10, hi = (i + 1) / 10;
			setStyle(e['xpfill' + i], 'width', (clamp((at - lo) * 10, 0, 1) * 100).toFixed(1) + '%');
			let left = 0, width = 0;
			if (gaining) {
				const g0 = Math.max(lo, (xp - gain.amount) / next), g1 = Math.min(hi, at);
				if (g1 > g0) { left = (g0 - lo) * 1000; width = (g1 - g0) * 1000; }
			}
			setStyle(e['xpgain' + i], 'left', left.toFixed(1) + '%');
			setStyle(e['xpgain' + i], 'width', width.toFixed(1) + '%');
		}
		setText(e.xpnow, Math.round(xp).toLocaleString('en-US'));
		setText(e.xpmax, '/ ' + Math.round(next).toLocaleString('en-US') + ' XP');
		setText(e.gemtext, s.level ?? 1);
	}

	/** Experience gained: the stretch just earned shows white and fades, "+amount XP" rises over the numbers. */
	onXp(amount) {
		const e = this.el;
		this.xpGain = { amount, start: this.fx.now };
		setText(e.xpfloat, '+' + Math.round(amount) + ' XP');
		for (let i = 0; i < 10; i++) this.fx.play(e['xpgain' + i], 'gain', DUR.gain);
		this.fx.play(e.xpfloat, 'xpfloat', DUR.xpfloat);
		this._xp(this.s);
		this.fx.tick();
	}

	/** The banner, the eyes' flare and the gem's flash and ring. sub: the banner's second line (default "MAX HEALTH +8%"). */
	onLevelUp(level, sub) {
		const e = this.el;
		setText(e.bannerlevel, level ?? this.s.level ?? 1);
		setText(e.bannersub, sub ?? 'MAX HEALTH +8%');
		e.banner.style.display = 'block';
		this.fx.play(e.banner, 'banner', DUR.banner, () => { e.banner.style.display = 'none'; });
		this.fx.play(e.flare, 'flare', DUR.flare);
		this.fx.play(e.gemring, 'ring', DUR.ring);
		this.fx.play(e.gemcore, 'gem', DUR.gem);
		this.fx.tick();
	}

	// ---- weapons ----

	_applySlot(i, slot, inHand) {
		const e = this.el;
		const box = e['slot' + i];
		const icon = slot && slot.icon;
		const key = icon ? icon + '|' + (slot.rarity | 0) : '';
		if (key !== this.slotKeys[i]) {
			this.slotKeys[i] = key;
			setClass(box, 'lh-empty', !icon);
			if (icon) {
				const rarity = RARITY[clamp(slot.rarity | 0, 0, 4)];
				setAttr(e['arc' + i], 'stroke', rarity);
				// A legendary's arc is orange-ish gold: its accent ring goes light so the two don't merge (HudWeaponSlotsWidget).
				setAttr(e['ring' + i], 'stroke', (slot.rarity | 0) === 4 ? '#dcefff' : '#ff9f1c');
				e['gun' + i].src = ICONS[icon] || ICONS.rifle;
				e['ammo' + i].src = AMMO[icon] || AMMO.rifle;
				setClass(e['gun' + i], 'lh-shotgun', icon === 'shotgun');
			}
		}
		setClass(box, 'lh-on', !!icon && inHand);
	}

	_weapons(s, dt, snap) {
		const e = this.el, now = this.fx.now;
		const slots = s.slots || DEFAULT_SLOTS;
		const slotIndex = s.slot ?? 0;
		for (let i = 0; i < 3; i++) this._applySlot(i, slots[i], i === slotIndex);
		const cur = slots[slotIndex] || {};
		const colour = cur.icon ? RARITY[clamp(cur.rarity | 0, 0, 4)] : '#dcefff';
		setText(e.firemode, cur.fireMode || '');
		setText(e.gunname, cur.name || '');
		setStyle(e.gunname, 'color', colour);
		setStyle(e.raritygem, 'background', colour);
		setStyle(e.raritygem, 'display', cur.icon ? '' : 'none');

		const max = Math.max(1, s.magMax ?? 30), mag = clamp(s.mag ?? max, 0, max), reserve = Math.max(0, s.reserve ?? 0);
		const m = this.mag, reloading = !!s.reloading;
		const frac = mag / max;
		const low = s.lowAmmo !== undefined ? !!s.lowAmmo : frac <= MAG_LOW;
		const empty = mag <= 0;
		if (reloading && !m.wasReloading) { m.reloadAt = now; m.shown = 0; }
		m.wasReloading = reloading;
		const duration = s.reloadSeconds ?? RELOAD_SECONDS[cur.icon] ?? 1.7;
		const target = reloading ? clamp((now - m.reloadAt) / duration, 0, 1) : frac;
		m.shown = snap || m.shown < 0 ? target : m.shown + (target - m.shown) * clamp(dt * MAG_DRAIN_SPEED, 0, 1);
		const width = m.shown > 0.001 ? MAG_FILL_RUN * m.shown : 0;
		const orange = reloading || low;
		setAttr(e.magfill, 'width', width.toFixed(2));
		setAttr(e.maghatch, 'width', width.toFixed(2));
		setAttr(e.magfill, 'fill', orange ? 'url(#lh-magOrange)' : 'url(#lh-magCyan)');
		setAttr(e.magedge, 'x', (15 + Math.max(0, width - 1.8)).toFixed(2));
		setStyle(e.magedge, 'display', width > 0 ? '' : 'none');
		setText(e.magcount, mag);
		setText(e.magreserve, '/' + reserve);
		setAttr(e.magcount, 'fill', empty ? '#ff7a6b' : low ? '#ff9f1c' : '#ffffff');
		setAttr(e.magreserve, 'fill', reserve <= 0 ? '#ff7a6b' : '#cfe2ef');
		this._fit(e.magcount);
		this._fit(e.magreserve);
		setClass(e.magline, 'lh-loop-empty', empty && !reloading);
		setText(e.status, reloading ? 'RELOADING' : empty ? (reserve > 0 ? '[R] RELOAD' : 'NO AMMO') : '');
	}

	/** A count too long for the cartridge's inside shrinks to fit it (the game's ScaleBox, down only). */
	_fit(text) {
		if (!text.getComputedTextLength) return;
		const len = text.getComputedTextLength();
		const k = len > COUNT_ROOM ? COUNT_ROOM / len : 1;
		setStyle(text, 'transform', k < 1 ? `scale(${k.toFixed(3)})` : '');
	}

	/** A shot: the crosshair kicks out to 1.45 times its size and settles in 0.16 s. */
	onFire() {
		this.fx.play(this.el.xh, 'kick', DUR.kick);
		this.fx.tick();
	}

	/** A hit marker over the crosshair; kill: a bigger, red one that holds a little longer. */
	onHitMarker(kill) {
		const hm = this.el.hm;
		setClass(hm, 'lh-kill', !!kill);
		this.fx.play(hm, 'hit', kill ? DUR.hitkill : DUR.hit);
		this.fx.tick();
	}

	/** Extra to the contract: an ammo pickup line, "+24 [icon] RIFLE ROUNDS", stacking up from the crosshair's left. */
	onPickup(count, name, icon) {
		const id = ++this.feedId;
		this.feed.push({ id, count, name, icon: icon === 'shotgun' ? 'shotgun' : 'rifle', start: this.fx.now });
		if (this.feed.length > 3) this.feed.shift();
		const line = this.feed[this.feed.length - 1];
		const k = id % 3, e = this.el;
		setText(e['feedn' + k], '+' + count);
		setText(e['feedname' + k], name || '');
		e['feedicon' + k].src = AMMO[line.icon];
		this.fx.play(e['feed' + k], 'feed', DUR.feed);
		this._feedTick();
		this.fx.tick();
	}

	_feedTick() {
		const now = this.fx.now;
		this.feed = this.feed.filter((f) => now - f.start < DUR.feed);
		const newest = this.feedId;
		for (const f of this.feed) setStyle(this.el['feed' + (f.id % 3)], 'top', (528 - (newest - f.id) * 28) + 'px');
	}

	// ---- minimap, place ----

	/** The place under the minimap and the area's name under that. */
	setArea(place, area) {
		setText(this.el.place, place || '');
		setText(this.el.area, area || '');
	}

	/**
	 * image: an HTMLImageElement or HTMLCanvasElement of the region from above; rect: [x0, z0, x1, z1] in three metres.
	 * The image is a top-down view as an orthographic camera looking down -y with up = -z sees it: x runs to the right
	 * and z down the image (the layout of the heights file). opts.northUp: the image has north up and east right instead.
	 * The HUD rotates it with the player's yaw under a fixed arrow; it shows 35 m around the player.
	 */
	setMinimap(image, rect, opts) {
		this.map.image = image || null;
		this.map.rect = rect || null;
		this.map.northUp = !!(opts && opts.northUp);
		this.map.key = '';
		this._minimap(this.s);
	}

	_minimap(s) {
		const e = this.el, m = this.map;
		const yaw = s.yawDeg ?? 0, px = s.pos ? s.pos[0] : 0, pz = s.pos ? s.pos[1] : 0;
		// The bezel's ticks and the N turn with the view; the arrow and the orange notch stay put.
		setAttr(e.mapticks, 'transform', `translate(98 98) rotate(${(-yaw).toFixed(2)})`);
		setAttr(e.mapn, 'transform', `translate(98 98) rotate(${(-yaw).toFixed(2)}) translate(0 -90.5) rotate(${yaw.toFixed(2)})`);
		const key = [yaw.toFixed(2), px.toFixed(3), pz.toFixed(3), m.image ? 1 : 0].join('|');
		if (key === m.key) return;
		m.key = key;
		const c = e.mapcanvas, ctx = c.getContext('2d');
		const size = c.width, half = size / 2, k = size / 172;
		ctx.clearRect(0, 0, size, size);
		ctx.save();
		ctx.beginPath();
		ctx.arc(half, half, half, 0, Math.PI * 2);
		ctx.clip();
		ctx.fillStyle = '#0a1219';
		ctx.fillRect(0, 0, size, size);
		if (m.image && m.rect) {
			const [x0, z0, x1, z1] = m.rect;
			const ppm = (MAP_RADIUS / MAP_RANGE) * k;
			ctx.translate(half, half);
			ctx.rotate((180 - yaw) * Math.PI / 180);
			ctx.scale(ppm, ppm);
			if (m.northUp) {
				ctx.translate((x0 + x1) / 2 - px, (z0 + z1) / 2 - pz);
				ctx.rotate(Math.PI);
				ctx.drawImage(m.image, -(x1 - x0) / 2, -(z1 - z0) / 2, x1 - x0, z1 - z0);
			} else {
				ctx.drawImage(m.image, x0 - px, z0 - pz, x1 - x0, z1 - z0);
			}
		}
		ctx.restore();
	}

	// ---- mission tracker ----

	_complete(m) { return !!m.done || ((m.need | 0) > 1 && (m.count | 0) >= (m.need | 0)); }

	/**
	 * { title, step, steps, objective, count, need, hintKey, hintText, done? }: step is 1-based ("4 / 6"). null hides the
	 * tracker. A finished objective (done, or count >= need) shows its tick for 1.4 s before the next one slides in.
	 */
	setMission(m) {
		const t = this.mission, now = this.fx.now;
		if (!m) { t.cur = null; t.pending = null; this.el.track.style.display = 'none'; t.key = ''; return; }
		const key = JSON.stringify([m.title, m.step, m.steps, m.objective, m.count, m.need, m.hintKey, m.hintText, !!m.done]);
		if (key === t.key || (t.pending && key === t.pendingKey)) return;
		const prev = t.cur;
		const changed = !prev || prev.objective !== m.objective || prev.title !== m.title || prev.step !== m.step;
		if (prev && changed && this._complete(prev) && now - t.doneAt < OBJECTIVE_HOLD) {
			t.pending = m;
			t.pendingKey = key;
			return;
		}
		t.pending = null;
		this._applyMission(m, key, changed);
	}

	_applyMission(m, key, slide) {
		const t = this.mission, e = this.el, now = this.fx.now, prev = t.cur;
		t.key = key;
		const wasDone = prev ? this._complete(prev) : false;
		t.cur = m;
		const done = this._complete(m);
		if (done && !(wasDone && !slide)) t.doneAt = now;
		e.track.style.display = '';
		setText(e.tracktitle, String(m.title || '').toUpperCase());
		const steps = Math.max(0, m.steps | 0), step = steps ? clamp(m.step | 0 || 1, 1, steps) : 0;
		const bar = e.tracksteps;
		bar.style.display = steps > 1 ? '' : 'none';
		if (steps !== t.segs) {
			t.segs = steps;
			bar.replaceChildren(...Array.from({ length: steps }, () => Object.assign(document.createElement('div'), { className: 'lh-trackseg' })));
		}
		[...bar.children].forEach((seg, i) => {
			seg.classList.toggle('lh-done', i < step - 1 || (i === step - 1 && done));
			seg.classList.toggle('lh-now', i === step - 1 && !done);
		});
		setText(e.trackstep, steps > 1 ? `${step} / ${steps}` : '');
		setText(e.objtext, m.objective || '');
		const counted = (m.need | 0) > 1;
		const count = counted ? `${m.count | 0} / ${m.need | 0}` : '';
		setText(e.trackcount, count);
		setClass(e.trackobj, 'lh-done', done);
		setAttr(e.chevron, 'opacity', done ? 0 : 1);
		setAttr(e.tick1, 'opacity', done ? 1 : 0);
		setAttr(e.tick2, 'opacity', done ? 1 : 0);
		setStyle(e.trackhint, 'display', m.hintKey ? '' : 'none');
		setText(e.hintkey, m.hintKey || '');
		setText(e.hinttext, m.hintText || '');
		if (slide && prev) this.fx.play(e.trackobj, 'objin', DUR.objin);
		if (prev && counted && (m.count | 0) > (prev.count | 0) && !slide) this.fx.play(e.trackcount, 'count', DUR.count);
	}

	_missionTick() {
		const t = this.mission;
		if (t.pending && this.fx.now >= t.doneAt + OBJECTIVE_HOLD) {
			const m = t.pending;
			t.pending = null;
			this._applyMission(m, t.pendingKey, true);
		}
	}

	// ---- boss ----

	/** null hides the bar; { name, level, fraction (0..1), phase (its name) }. Damage leaves a chip that drains. */
	setBoss(b) {
		const e = this.el, o = this.boss, now = this.fx.now;
		if (!b) { o.cur = null; e.boss.style.display = 'none'; return; }
		const f = clamp(b.fraction ?? 1, 0, 1);
		const first = !o.cur;
		e.boss.style.display = 'block';
		setText(e.bossname, String(b.name || '').toUpperCase());
		setText(e.bosslevel, b.level ?? 1);
		if (first) {
			Object.assign(o, { shown: f, chipFrom: f, holdUntil: -1e9 });
		} else if (f < o.shown - 1e-4) {
			o.chipFrom = Math.max(this._bossChip(now), o.shown);
			o.holdUntil = now + CHIP_HOLD;
		}
		o.shown = f;
		const phase = typeof b.phase === 'number' ? PHASES[b.phase] || '' : String(b.phase || '');
		setText(e.bossphase, phase);
		if (!first && phase !== o.phase) this.fx.play(e.bossphase, 'phase', DUR.phase);
		o.phase = phase;
		o.cur = b;
		setStyle(e.bosscut1, 'background', f < 0.7 ? '#dcefff' : '#0e1116');
		setStyle(e.bosscut2, 'background', f < 0.4 ? '#dcefff' : '#0e1116');
		this._bossTick();
		this.fx.tick();
	}

	_bossChip(now) {
		const o = this.boss;
		if (now < o.holdUntil) return o.chipFrom;
		return Math.max(o.shown, lerp(o.chipFrom, o.shown, easeIn(clamp((now - o.holdUntil) / CHIP_DRAIN, 0, 1))));
	}

	_bossTick() {
		const o = this.boss;
		if (!o.cur) return;
		setStyle(this.el.bossfill, 'width', (o.shown * 100).toFixed(2) + '%');
		setStyle(this.el.bosschip, 'width', (Math.max(o.shown, this._bossChip(this.fx.now)) * 100).toFixed(2) + '%');
	}

	// ---- the screen's edges ----

	_edges() {
		const now = this.fx.now;
		const age = now - this.hitAt;
		let strength = age >= 0 && age < EDGE_HIT_SECONDS ? EDGE_HIT_PEAK * (1 - age / EDGE_HIT_SECONDS) ** 2 : 0;
		if (this.hp.low) {
			const beat = 0.5 - 0.5 * Math.cos(2 * Math.PI * ((now % LOW_BEAT) / LOW_BEAT));
			strength = Math.max(strength, lerp(EDGE_LOW[0], EDGE_LOW[1], beat));
		}
		const opacity = strength * EDGE_STRENGTH;
		const shown = opacity > 0.001 ? opacity.toFixed(4) : '0';
		if (shown === this.lastEdge) return;
		this.lastEdge = shown;
		this.el.edges.style.display = opacity > 0.001 ? 'block' : 'none';
		this.el.edges.style.opacity = shown;
	}
}

/**
 * Appends the HUD's root over the container's canvas and returns the HUD. Its CSS and fonts are added to the page if
 * they are missing; hud.ready resolves when they are loaded. The container should be positioned (it is made relative if not).
 */
export function createHud(container) {
	if (getComputedStyle(container).position === 'static') container.style.position = 'relative';
	const root = document.createElement('div');
	root.className = 'lh-root';
	root.setAttribute('aria-hidden', 'true');
	root.innerHTML = buildMarkup();
	container.appendChild(root);
	const hud = new Hud(root, container);
	hud.resize(container.clientWidth || DESIGN_W, container.clientHeight || DESIGN_H);
	if (typeof ResizeObserver !== 'undefined') {
		new ResizeObserver(() => hud.resize(container.clientWidth, container.clientHeight)).observe(container);
	}
	return hud;
}
