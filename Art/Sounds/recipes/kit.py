"""Small building blocks the recipes share: clicks and clacks of metal and polymer, thumps, whooshes, cloth, scrapes,
creaks, bubbles, brass ticks. Each returns a mono signal peaking at 1, so layers mix by dB.
"""
import numpy as np

from lib.core import SR, ns, times, normalize, layers, child, jitter, fit, db2a, peak
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, granular as G


def env_peak(x, win=0.005):
    """The loudest short-window level of x (RMS over win, as a peak: x sqrt 2)."""
    m = x if x.ndim == 1 else x.mean(axis=0)
    k = max(1, ns(win))
    c = np.concatenate([[0.0], np.cumsum(m * m)])
    if m.size <= k:
        return float(np.sqrt(c[-1] / max(m.size, 1)) * np.sqrt(2.0))
    return float(np.sqrt(np.max(c[k:] - c[:-k]) / k) * np.sqrt(2.0))


def set_level(x, ref, rel_db, win=0.005):
    """x scaled so its loudest short window sits rel_db against ref's."""
    p = env_peak(x, win)
    return x * (env_peak(ref, win) * float(db2a(rel_db)) / p) if p > 0 else x


def nwave(width=0.0003):
    """A shock front: an N-shaped pressure pulse (a jump up, a straight fall through zero, a jump back), the first
    instant of a muzzle blast or a supersonic crack. Band-limited so its edges don't alias."""
    k = max(3, ns(width))
    x = np.concatenate([[0.0], np.linspace(1.0, -1.0, k), [0.0]])
    x = F.filt(np.pad(x, (4, 64)), F.lpn(17000.0, 4), extend=False)
    return normalize(x, 0.0)


def thump(f0, f1, dur, t60, r=None, attack=0.0008, sweep_shape=1.6, drive_db=0.0):
    """A pitch-dropping sine: the weight of an impact or a blast in the chest."""
    f = O.expsweep(f0, f1, dur, sweep_shape)
    x = O.sine(f) * E.perc(dur, attack, t60)
    if drive_db:
        x = D.drive(x, drive_db, 'tanh')
    return normalize(x, 0.0)


def noise_thump(dur, r, f0, f1, t60, q=1.1, sweep_time=None, attack=0.0008):
    """A low thud made of air rather than a tone (a blast's pressure, a body hitting the ground): dark noise through a
    resonant low-pass sliding down from f0 to f1. Less like a drum than a pure sine thump."""
    n = ns(dur)
    # Pink, not brown: brown noise's lows wander so much that every seed would weigh differently.
    x = N.pink(n, r, lo=45.0)
    k = ns(sweep_time or dur * 0.5)
    fc = np.full(n, float(f1))
    fc[:k] = O.expsweep(f0, f1, k / SR, 1.5, n=k)
    y = F.sweep(x, 'lp', fc, q)
    return normalize(y * E.perc(dur, attack, t60), 0.0)


def turbulence(n, r, rate=300.0, depth=0.45):
    """A fast, irregular amplitude flutter: gas tearing out of a muzzle, air rushing past. Multiply a noise layer by
    it to stop it sounding like a smooth snare-drum burst."""
    return (1.0 - depth) + depth * np.abs(N.smooth_random(n, r, rate)) * 2.0 * (0.5 + 0.5 * N.smooth_random(n, r, rate * 0.37))


def burst(dur, r, t60, lo=None, hi=None, color=0.0, attack=0.0002):
    """Shaped noise hit."""
    x = N.shaped(ns(dur), r, color) * E.perc(dur, attack, t60)
    secs = []
    if lo:
        secs.append(F.hp(lo, 0.7))
    if hi:
        secs.append(F.lp(hi, 0.7))
    if secs:
        x = F.filt(x, *secs, extend=False)
    return normalize(x, 0.0)


def metal_click(r, lo=2000.0, hi=8000.0, t60=0.04, contact=0.0001, count=6, tick_db=-10.0, tilt=0.3):
    """A small steel part snapping (a sear, a latch, a pin): a few bright modes and a hard tick."""
    m = M.parts(lo, hi, r, count=count, t60=t60, tilt=tilt)
    dur = t60 * 1.6 + 0.01
    ring = M.strike(m, M.hammer(contact), dur)
    tick = N.white(ns(0.0015), r) * E.decay(ns(0.0015), 0.0008)
    tick = F.filt(tick, F.hp(lo * 0.8), extend=False)
    return normalize(layers(normalize(ring), (normalize(tick), 0.0, tick_db)), 0.0)


def thunk(f0, r, t60=0.06, contact=0.0006, count=6, spread=0.06):
    """A dull knock of polymer or wood (a grip, a stock, a magazine body, a crate)."""
    m = M.wood(f0, r, t60=t60, count=count, spread=spread)
    return normalize(M.strike(m, M.hammer(contact), t60 * 1.8 + 0.01), 0.0)


def clack(r, f_body=700.0, lo=1800.0, hi=7000.0, t60_metal=0.05, t60_body=0.05, body_db=-4.0, low_db=-8.0,
          contact=0.0001):
    """A mechanical clack: steel parts meeting (bright), the frame around them (a mid knock) and a little weight."""
    metal = metal_click(r, lo, hi, t60_metal, contact)
    body = thunk(f_body, r, t60_body, contact * 4)
    low = thump(f_body * 0.32, f_body * 0.18, 0.08, 0.04)
    return normalize(layers(metal, (body, 0.0, body_db), (low, 0.0, low_db)), 0.0)


def whoosh(dur, r, f0, f1, q=0.9, peak_at=0.5, color=-3.0, shape=1.0):
    """Air moving: noise through a band-pass that slides from f0 to f1, swelling to its peak at peak_at (a fraction
    of dur)."""
    n = ns(dur)
    x = N.shaped(n, r, color)
    fc = O.expsweep(f0, f1, dur, shape, n=n)
    y = F.sweep(x, 'bp', fc, q)
    u = np.arange(n) / n
    p = max(peak_at, 1e-3)
    env = np.where(u < p, np.sin(0.5 * np.pi * u / p) ** 2,
                   np.cos(0.5 * np.pi * np.clip((u - p) / max(1 - p, 1e-3), 0, 1)) ** 2)
    return normalize(y * env, 0.0)


def cloth(dur, r, lo=350.0, hi=3200.0, crinkle=140.0, attack=0.35, release=0.6, color=-3.0):
    """Fabric moving: noise in the cloth band, broken into crinkles by a fast random swell."""
    n = ns(dur)
    x = N.shaped(n, r, color)
    am = 0.25 + 0.75 * np.abs(N.smooth_random(n, r, crinkle)) ** 1.6
    y = F.filt(x * am, F.hp(lo, 0.6), F.lp(hi, 0.6), extend=False)
    return normalize(y * E.ar(dur, dur * attack, dur * release, 2.0), 0.0)


def scrape(dur, r, f0, f1, q=14.0, rough=0.6, partials=(1.0, 1.47, 2.31, 3.7), attack=0.2, release=0.5,
           grit_rate=2500.0, hiss_db=-20.0):
    """Metal sliding on metal (a bolt, a magazine, a pump's rails): stick-slip micro-impacts (not smooth hiss) ringing
    a few sharp resonances that slide together as the contact moves."""
    n = ns(dur)
    exc = N.dust(n, r, grit_rate, (0.2, 1.0)) + float(db2a(hiss_db)) * N.white(n, r)
    exc *= (1.0 - rough) + rough * np.abs(N.smooth_random(n, r, 70.0))
    fc = O.expsweep(f0, f1, dur, 1.0, n=n)
    y = np.zeros(n)
    for i, k in enumerate(partials):
        y += F.sweep(exc, 'bp', np.minimum(fc * k, 18000.0), q) / (1.0 + i * 0.5)
    y = F.filt(y, F.hp(450.0, 0.7), F.lp(9000.0, 0.7), extend=False)
    return normalize(y * E.ar(dur, dur * attack, dur * release, 2.0), 0.0)


def creak(dur, r, rate0, rate1, body=(320.0, 760.0, 1500.0), q=8.0, wobble=0.25):
    """Stick-slip friction (a hinge, a board, leather): a train of tiny slips whose rate drifts, rung through the
    resonances of what's creaking."""
    n = ns(dur)
    rate = O.expsweep(rate0, rate1, dur, 1.0, n=n) * (1.0 + wobble * N.smooth_random(n, r, 6.0))
    ph = O.phase(rate, n)
    k = np.floor(ph)
    pulses = np.zeros(n)
    edges = np.nonzero(np.diff(k) > 0)[0] + 1
    pulses[edges] = r.uniform(0.5, 1.0, edges.size)
    # A slip is quick but not instant: soften the pulses, then ring them through doubled (steeper) resonances so
    # the creak has a pitch and a body, not a broadband buzz.
    pulses = F.filt(pulses, F.lp1(2500.0), extend=False)
    y = np.zeros(n)
    for i, f in enumerate(body):
        fc = f * jitter(r, 1.0, 0.05)
        y += F.filt(pulses, F.bp(fc, q), F.bp(fc, q), extend=False) / (1.0 + 0.5 * i)
    y = normalize(y)
    y += 0.05 * normalize(F.filt(N.white(n, r) * np.abs(N.smooth_random(n, r, 40.0)), F.bp(body[0] * 2, 2.0), extend=False))
    am = 0.6 + 0.4 * N.smooth_random(n, r, 7.0)
    return normalize(y * am * E.ar(dur, 0.04, 0.08, 1.0), 0.0)


def squelch(dur, r, f0, f1, q=3.0):
    """Something wet giving way (gel, flesh, mud): noise broken into fast irregular pulses, through a resonance that
    slides like a liquid's formant from f0 to f1."""
    n = ns(dur)
    x = N.pink(n, r) * turbulence(n, child(r, 'sq'), 220.0, 0.85)
    fc = O.expsweep(f0, f1, dur, 1.0, n=n)
    y = F.sweep(x, 'bp', fc, q) + 0.5 * F.sweep(x, 'bp', np.minimum(fc * 2.1, 16000.0), q * 1.3)
    y = F.filt(y, F.lp(4000.0, 0.7), extend=False)
    return normalize(y * E.perc(dur, 0.003, dur * 0.8), 0.0)


def bubble(f0, r, xi=0.1, amp=1.0):
    """One bubble's ring (a gas bubble in liquid sings at a pitch set by its size and rises as it closes): the
    physical bubble model, a decaying sine whose pitch climbs."""
    d = 0.13 * f0 + 0.0072 * f0 ** 1.5
    dur = min(0.25, 7.0 / d)
    n = ns(dur)
    t = times(n)
    f = f0 * (1.0 + xi * d * t)
    x = np.sin(2.0 * np.pi * O.phase(f, n)) * np.exp(-d * t)
    x[:ns(0.0003)] *= np.linspace(0, 1, ns(0.0003))
    return x * amp


def brass_tick(r, f0=1700.0, t60=0.035, count=4, contact=0.00008, low_db=-12.0, click_db=-8.0, bright=0.4):
    """A small brass tick: a few inharmonic modes of a thin brass piece, a hard click and a soft knock under it."""
    ratios = np.array([1.0, 1.71, 2.53, 3.62, 4.9][:count])
    f = f0 * ratios * (1.0 + 0.01 * (2 * r.random(count) - 1))
    a = ratios ** (-1.0 + bright) * (0.7 + 0.3 * r.random(count))
    d = t60 * ratios ** -0.6
    ring = M.strike((f, d, a), M.hammer(contact), t60 * 1.8 + 0.005)
    click = N.white(ns(0.001), r) * E.decay(ns(0.001), 0.0005)
    click = F.filt(click, F.bp(6500.0, 1.2), extend=False)
    knock = O.sine(np.full(ns(0.02), f0 * 0.14)) * E.perc(0.02, 0.0005, 0.012)
    return normalize(layers(normalize(ring), (normalize(click), 0, click_db), (normalize(knock), 0, low_db)), 0.0)


# A brass chime's partials (ratio, strength, share of the ring time): near-harmonic, so it sounds a clear note, with
# the slight stretch of a real bar or bowl keeping it metallic.
CHIME = [(1.0, 1.0, 1.0), (2.0, 0.5, 0.62), (3.01, 0.32, 0.42), (4.08, 0.2, 0.3), (5.22, 0.12, 0.22), (6.65, 0.07, 0.15)]


def chime(f0, r, t60=1.2, bright=1.0, contact=0.00012, beat=0.6, click_db=-24.0):
    """A small brass chime struck once: a clear note with a metallic shimmer (each partial a slowly beating pair)."""
    f, d, a = [], [], []
    for i, (ratio, amp, share) in enumerate(CHIME):
        base = f0 * ratio * (1.0 + 0.002 * (2 * r.random() - 1))
        for s in (-0.5, 0.5):
            f.append(base + s * beat * (1.0 + 0.5 * i) * (0.6 + 0.8 * r.random()))
            d.append(t60 * share)
            a.append(0.5 * amp * (bright ** i))
    ring = M.strike((np.array(f), np.array(d), np.array(a)), M.hammer(contact), t60 * 1.1)
    click = N.white(ns(0.0015), r) * E.decay(ns(0.0015), 0.0007)
    click = F.filt(click, F.bp(5000.0, 0.8), extend=False)
    return normalize(layers(normalize(ring), (normalize(click), 0.0, click_db)), 0.0)


def twang(f0, dur, r, bright=0.6, t60=2.0, pluck=0.16):
    """A plucked steel string on a resonating wooden box: the weird west's voice in the interface's stingers."""
    s = O.string(f0, dur, r, pluck, t60, bright, 0.00012, 0.3, detune=1.5)
    s = F.filt(s, F.peak(190.0, 4.0, 1.2), F.peak(620.0, 2.0, 1.4), F.peak(2700.0, 3.0, 2.0),
               F.highshelf(6500.0, -6.0), extend=False)
    pick = N.white(ns(0.002), r) * E.decay(ns(0.002), 0.001)
    pick = F.filt(pick, F.bp(3500.0, 1.0), extend=False)
    return normalize(layers(normalize(s), (normalize(pick), 0.0, -20.0)), 0.0)


NOTE_A4 = 440.0


def note(name):
    """A note's frequency from its name: 'A4', 'F#5', 'Bb3'."""
    steps = {'C': -9, 'D': -7, 'E': -5, 'F': -4, 'G': -2, 'A': 0, 'B': 2}
    k = steps[name[0]]
    rest = name[1:]
    if rest[0] == '#':
        k += 1
        rest = rest[1:]
    elif rest[0] == 'b':
        k -= 1
        rest = rest[1:]
    return NOTE_A4 * 2.0 ** ((k + 12 * (int(rest) - 4)) / 12.0)


def jingle(dur, r, count, lo=2500.0, hi=9000.0, t60=0.05, spread=0.06, start=0.0):
    """Small loose metal things shaken together (a buckle, a sling swivel, spare rounds)."""
    def grain(rr, i):
        return metal_click(rr, lo * jitter(rr, 1, 0.1), hi, t60 * jitter(rr, 1, 0.3), 0.00006, 4, -14.0)
    return normalize(G.rattle(dur, child(r, 'jingle'), count, spread, grain, start, 0.82), 0.0)


def grit_burst(dur, r, rate, lo=1500.0, hi=6000.0, decay_t60=None, attack=0.002):
    """Grit flying or crunching: dense at first, thinning out."""
    n = ns(dur)
    env = E.perc(dur, attack, decay_t60 or dur * 0.6)
    g = G.grit(dur, r, rate * env, lo, hi, 2.0, 4)
    return normalize(g * np.sqrt(env), 0.0)


def ricochet(dur, r, f0=4200.0, f1=1700.0, flutter=45.0):
    """A bullet glancing off and tumbling away: a whistle sliding down (its Doppler as it leaves), fluttering as it
    tumbles, with a breath of air around it."""
    n = ns(dur)
    t = times(n)
    f = O.expsweep(f0, f1, dur, 1.8, n=n)
    tone = np.sin(2 * np.pi * O.phase(f, n)) + 0.25 * np.sin(4 * np.pi * O.phase(f, n))
    am = 1.0 - 0.45 * (0.5 + 0.5 * np.sin(2 * np.pi * O.phase(flutter * (1 + 0.2 * N.smooth_random(n, r, 4.0)), n)))
    air = F.sweep(N.white(n, r), 'bp', f, 6.0)
    y = normalize(tone) * 0.8 + normalize(air) * 0.35
    env = E.perc(dur, 0.004, dur * 0.8) * am
    return normalize(y * env, 0.0)
