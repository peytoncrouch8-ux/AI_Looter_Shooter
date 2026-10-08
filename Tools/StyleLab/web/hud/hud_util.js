// Small helpers for hud.js: easing curves, cached DOM setters and the effect clock that seeks the CSS animations.

export const clamp = (v, lo, hi) => Math.min(hi, Math.max(lo, v));
export const lerp = (a, b, t) => a + (b - a) * t;

/** CSS's cubic-bezier(x1, y1, x2, y2) as a function of progress 0..1 (the transitions in the mockup use ease-in/-out). */
export function bezier(x1, y1, x2, y2) {
	const cx = 3 * x1, bx = 3 * (x2 - x1) - cx, ax = 1 - cx - bx;
	const cy = 3 * y1, by = 3 * (y2 - y1) - cy, ay = 1 - cy - by;
	const sampleX = (t) => ((ax * t + bx) * t + cx) * t;
	const sampleY = (t) => ((ay * t + by) * t + cy) * t;
	return (x) => {
		if (x <= 0) return 0;
		if (x >= 1) return 1;
		let lo = 0, hi = 1, t = x;
		for (let i = 0; i < 24; i++) {
			const v = sampleX(t);
			if (Math.abs(v - x) < 1e-5) break;
			if (v < x) lo = t; else hi = t;
			t = (lo + hi) / 2;
		}
		return sampleY(t);
	};
}
export const easeIn = bezier(0.42, 0, 1, 1);
export const easeOut = bezier(0, 0, 0.58, 1);

// Cached writes: the HUD updates every frame and only touches the DOM when a value changed.
export function setText(el, value) {
	const v = String(value);
	if (el._lhT !== v) { el._lhT = v; el.textContent = v; }
}
export function setAttr(el, name, value) {
	const key = '_lhA' + name, v = String(value);
	if (el[key] !== v) { el[key] = v; el.setAttribute(name, v); }
}
export function setStyle(el, name, value) {
	const key = '_lhS' + name;
	if (el[key] !== value) { el[key] = value; el.style[name] = value; }
}
export function setClass(el, name, on) {
	const key = '_lhC' + name;
	if (el[key] !== !!on) { el[key] = !!on; el.classList.toggle(name, !!on); }
}

/**
 * The HUD's animation clock. The mockup's animations are CSS keyframes; here they are all paused and seeked: a one-shot
 * effect sets its class and a negative animation-delay for its age, a loop sets the delay from the clock modulo its
 * period. Nothing runs on real time, so freezing the clock freezes every animation, whatever the page does.
 */
export class Effects {
	constructor() {
		this.now = 0;
		this.active = new Map();   // element -> { cls, dur, start, onEnd }
		this.loops = [];           // { el, dur }
	}

	loop(el, dur) { this.loops.push({ el, dur }); }

	/** Starts (or restarts) the one-shot animation class `lh-fx-<name>` on el; dur is its length in seconds. */
	play(el, name, dur, onEnd) {
		const cls = 'lh-fx-' + name;
		const old = this.active.get(el);
		if (old && old.cls !== cls) el.classList.remove(old.cls);
		el.classList.remove(cls);
		el.style.animationDelay = '0s';
		void el.offsetWidth;   // the class has to be seen gone for the animation to start over
		el.classList.add(cls);
		this.active.set(el, { cls, dur, start: this.now, onEnd });
	}

	/** True while el's effect is still running. */
	running(el) { return this.active.has(el); }

	/** Moves the clock to t without changing how far along any running effect is (freeze(t) uses it). */
	rebase(t) {
		const shift = t - this.now;
		for (const rec of this.active.values()) rec.start += shift;
		this.now = t;
	}

	/** Puts every animation at its place for the clock's time. */
	tick() {
		for (const [el, rec] of this.active) {
			const age = this.now - rec.start;
			if (age >= rec.dur) {
				el.classList.remove(rec.cls);
				el.style.animationDelay = '';
				this.active.delete(el);
				if (rec.onEnd) rec.onEnd();
			} else {
				el.style.animationDelay = (-Math.max(0, age)).toFixed(4) + 's';
			}
		}
		for (const l of this.loops) l.el.style.animationDelay = (-(this.now % l.dur)).toFixed(4) + 's';
	}
}
