"""Creatures (3D): any body taking a bullet; the brown spiders (mandible chitter, stridulating rasp, hiss, a thin
squeal); the meadow slimes (wet squelch, gel wobble, bubbles, a gloopy gulp of a voice); the Unpaid (breathy wails
of several voices at once, a rasping shriek, a rush of cold air, a long sinking death into the dark).

Voices are formant synthesis (lib/voice.py): a glottal source shaped by moving vocal-tract resonances, with breath,
pitch wobble and roughness, so they sound alive rather than electronic.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, mono, reverse
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, stereo as S
from lib import granular as G, voice as V
from recipes import cue
from recipes import kit


def _outside(x, r, rel_db=-20.0, s1=0.07, s2=0.18, t60=0.5):
    ir = R.outdoor(child(r, 'ir'), dur=max(0.7, t60 * 1.4), slaps=((s1, -4.0), (s2, -9.0)), tail_t60=t60,
                   tail_db=-10.0, tail_start=0.02, tail_peak=0.08)
    wet = F.convolve(F.filt(x, F.hpn(200.0, 2), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


def _haunt(x, r, rel_db=-8.0, t60=2.4, lp=3200.0):
    """The Unpaid's space: a wide, dark open-air wash, louder than for anything living."""
    ir = R.open_air(child(r, 'haunt'), t60=t60, lp=lp, predelay=0.02, density=0.8)
    wet = F.convolve(F.filt(x, F.hp(180.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.05))


# --- Any creature ---------------------------------------------------------------------------------------------------

@cue('Creature.Hit', variations=5, att='Creature', jitter=0.06, conc=6, level=-10.0)
def creature_hit(v, r):
    # A bullet into a body: a snap, a punch, the meaty slap of it, something wet, a crack of shell or bone.
    snap = kit.burst(0.005, child(r, 'snap'), 0.002, lo=1200.0, hi=7000.0)
    punch = kit.thump(160.0 * jitter(r, 1, 0.1), 70.0, 0.07, 0.04, drive_db=3.0)
    n = ns(0.08)
    slap = N.band(n, child(r, 'slap'), 600.0, 2400.0) * kit.turbulence(n, child(r, 'st'), 300.0, 0.6) * E.perc(0.08, 0.0005, 0.035)
    wet = np.zeros(1)
    for i in range(2 + int(r.integers(0, 3))):
        wet = layers(wet, (kit.bubble(r.uniform(450.0, 1300.0), r, 0.15), r.uniform(0.005, 0.04), r.uniform(-6.0, 0.0)))
    squish = kit.squelch(0.09, child(r, 'squish'), 1400.0, 400.0)
    crack = kit.grit_burst(0.04, child(r, 'crack'), 6000.0, 1800.0, 5500.0, 0.02)
    x = layers((snap, 0.0, -7.0), (punch, 0.0, -3.0), (normalize(slap), 0.0, 0.0), (normalize(wet), 0.0, -12.0),
               (squish, 0.004, -10.0), (crack, 0.0, -15.0))
    return F.filt(x, F.peak(3200.0, -3.0, 0.8))


# --- Spiders --------------------------------------------------------------------------------------------------------

def chitter(dur, r, rate0, rate1, lo=1700.0, hi=4800.0, spread=0.3, shape=1.0):
    """Mandibles clicking: a train of tiny chitin clicks at a rate gliding from rate0 to rate1, irregular, each its
    own small resonance."""
    clicks = [normalize(M.strike(M.parts(lo, hi, child(r, 'k', i), 3, 0.007, 0.004), M.hammer(0.00005), 0.02))
              for i in range(5)]
    out = np.zeros(ns(dur) + ns(0.03))
    t = 0.0
    while t < dur:
        u = (t / dur) ** shape
        rate = rate0 + (rate1 - rate0) * u
        k = ns(t)
        c = clicks[int(r.integers(0, 5))] * r.uniform(0.4, 1.0)
        out[k:k + c.size] += c[:max(0, out.size - k)]
        t += (1.0 / rate) * (1.0 + spread * (2 * r.random() - 1))
    return normalize(out)


def rasp(dur, r, rate0, rate1, body=(1300.0, 2700.0, 4100.0), q=2.5):
    """Stridulation: a ridged file scraped across a pick (how many spiders and insects make sound), a buzzing train
    of small hits rung through the body."""
    return kit.creak(dur, r, rate0, rate1, body, q, 0.15)


def _skitter(r, count, start, gap):
    """Legs tapping the ground fast and light."""
    out = np.zeros(1)
    t = start
    for i in range(count):
        tap = kit.thunk(r.uniform(900.0, 1500.0), child(r, 'tap', i), 0.012, 0.00015, 3, 0.1)
        out = layers(out, (tap, t, r.uniform(-8.0, 0.0)))
        t += gap * r.uniform(0.6, 1.4)
    return normalize(out)


def _squeal(r, f0_points, dur, keys, shift=1.7, rough=0.3):
    """A thin, pained squeal from something with a tiny throat, buzzing a little like an insect."""
    n = ns(dur)
    f0 = O.glide(f0_points, n=n)
    x = V.voice(f0, keys, r, breath=0.3, jitter=0.04, shimmer=0.15, tilt=2.0, shift=shift, rough=rough)
    buzz = 1.0 - 0.35 * (0.5 + 0.5 * np.sin(2 * np.pi * 70.0 * times(n)))
    return normalize(F.filt(x * buzz, F.peak(3400.0, -5.0, 0.8), F.hp(300.0), extend=False))


@cue('Creature.Spider.Alert', variations=3, att='Creature', jitter=0.06, conc=3, level=-7.0)
def spider_alert(v, r):
    # It's seen you: the mandibles start clicking and speed up, the rasp swells, a hiss rises behind it.
    ch = chitter(0.7, child(r, 'chitter'), 12.0, 36.0, shape=0.7)
    ch = ch[:ns(0.75)] * E.ar(0.75, 0.05, 0.25)[:ch[:ns(0.75)].size]
    rs = rasp(0.65, child(r, 'rasp'), 85.0 * jitter(r, 1, 0.1), 140.0)
    n = ns(0.8)
    hs = V.hiss(n, child(r, 'hiss')) * E.swell(0.8, 0.55, 0.4) * (0.75 + 0.25 * np.sin(2 * np.pi * 26.0 * times(n)))
    throat = kit.creak(0.6, child(r, 'throat'), 28.0, 36.0, (240.0, 520.0), 4.0)
    x = layers((ch, 0.0, 0.0), (rs, 0.1, -6.0), (normalize(hs), 0.15, -9.0), (throat, 0.1, -19.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r)


@cue('Creature.Spider.Attack', variations=4, att='Creature', jitter=0.06, conc=3, level=-7.0)
def spider_attack(v, r):
    # The lunge: a sharp hiss, legs skittering, the mandibles snapping shut twice with a crunch.
    n = ns(0.3)
    hs = V.hiss(n, child(r, 'hiss'), (2400.0, 4000.0, 6500.0)) * E.perc(0.3, 0.012, 0.25)
    legs = _skitter(child(r, 'legs'), 6 + int(r.integers(0, 4)), 0.0, 0.035)
    t = 0.14 + 0.04 * r.random()
    snap = layers(kit.clack(child(r, 's1'), 900.0, 1500.0, 5000.0, 0.02, 0.03, -2.0, -8.0),
                  (kit.grit_burst(0.04, child(r, 'c1'), 8000.0, 1500.0, 5000.0, 0.02), 0.0, -8.0))
    snap2 = kit.clack(child(r, 's2'), 1000.0, 1600.0, 5200.0, 0.02, 0.025, -3.0, -10.0)
    ch = chitter(0.25, child(r, 'chitter'), 40.0, 25.0)
    x = layers((normalize(hs), 0.0, -4.0), (legs, 0.0, -8.0), (snap, t, 0.0), (snap2, t + 0.03, -3.0), (ch, t + 0.05, -10.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r)


@cue('Creature.Spider.Hurt', variations=4, att='Creature', jitter=0.06, conc=3, level=-8.0)
def spider_hurt(v, r):
    # A thin squeal and a burst of frantic clicking.
    f = 680.0 * jitter(r, 1, 0.12)
    sq = _squeal(child(r, 'squeal'), [(0.0, f), (0.06, f * 1.3), (0.32, f * 0.85)], 0.32, [(0.0, 'i'), (0.32, 'e')])
    sq *= E.perc(0.32, 0.01, 0.28)
    ch = chitter(0.3, child(r, 'chitter'), 45.0, 30.0)
    legs = _skitter(child(r, 'legs'), 4, 0.02, 0.03)
    x = layers((sq, 0.0, 0.0), (ch, 0.0, -6.0), (legs, 0.0, -12.0))
    return _outside(x, r, -21.0)


@cue('Creature.Spider.Death', variations=3, att='Creature', jitter=0.05, conc=3, level=-7.0)
def spider_death(v, r):
    # The squeal sinks, the clicking slows and stops, the legs curl in with a few dry taps, and a last hiss.
    f = 760.0 * jitter(r, 1, 0.1)
    sq = _squeal(child(r, 'squeal'), [(0.0, f), (0.1, f * 1.15), (0.9, f * 0.4)], 0.9, [(0.0, 'i'), (0.4, 'e'), (0.9, 'uh')],
                 1.6, 0.45)
    sq *= E.swell(0.9, 0.08, 0.8)
    ch = chitter(1.1, child(r, 'chitter'), 30.0, 4.0, shape=0.6)
    ch *= np.linspace(1.0, 0.3, ch.size)
    curl = _skitter(child(r, 'curl'), 5, 0.0, 0.11)
    n = ns(0.6)
    last = V.hiss(n, child(r, 'last'), (2000.0, 3500.0, 5500.0)) * E.swell(0.6, 0.1, 0.5)
    last = F.sweep(last, 'lp', O.expsweep(7000.0, 1500.0, 0.6, n=n), 0.8)
    x = layers((sq, 0.0, 0.0), (ch, 0.0, -7.0), (curl, 0.5, -10.0), (normalize(last), 0.95, -12.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r)


# --- Slimes ---------------------------------------------------------------------------------------------------------

def _wobble(dur, r, f0, rate, t60):
    """The gel jiggling after it moves: a low tone pulsing as the blob wobbles, dying away."""
    n = ns(dur)
    tt = times(n)
    x = O.sine(f0 * (1.0 + 0.08 * np.sin(2 * np.pi * rate * tt)), n=n)
    am = 0.5 + 0.5 * np.sin(2 * np.pi * rate * tt)
    return normalize(x * am * E.perc(dur, 0.005, t60))


def _bubbles(dur, r, rate, lo, hi, start=0.0, decay=None):
    out = np.zeros(1)
    fn = rate if callable(rate) else (lambda t: rate)
    for t in N.times_poisson(dur, r, fn, start):
        a = 0.0 if decay is None else -20.0 * t / decay
        out = layers(out, (kit.bubble(r.uniform(lo, hi), r, 0.12), t, a + r.uniform(-8.0, 0.0)))
    return normalize(out) if out.size > 1 else out


def _gloop(r, f0_points, dur, keys, shift=0.7, gurgle=18.0, rough=0.4):
    """The slime's voice: a deep, wet, gulping sound with no mouth to shape it, gurgling."""
    n = ns(dur)
    f0 = O.glide(f0_points, n=n)
    x = V.voice(f0, keys, r, breath=0.2, jitter=0.03, shimmer=0.2, tilt=-3.0, shift=shift, rough=rough)
    am = 0.55 + 0.45 * np.abs(np.sin(np.pi * O.phase(gurgle * (1.0 + 0.3 * N.smooth_random(n, r, 4.0)), n)))
    return normalize(F.filt(x * am, F.lp(2600.0, 0.7), F.hp(60.0), extend=False))


@cue('Creature.Slime.Hop', variations=4, att='Creature', jitter=0.08, conc=4, level=-13.0)
def slime_hop(v, r):
    # Landing from a hop (it plays on every landing): a short wet splat with a little weight, the gel wobbling as it
    # settles, a bubble or two.
    splat = kit.squelch(0.09, child(r, 'splat'), 1600.0 * jitter(r, 1, 0.1), 350.0)
    slap = kit.burst(0.05, child(r, 'slap'), 0.03, lo=150.0, hi=3000.0)
    weight = kit.noise_thump(0.08, child(r, 'weight'), 600.0, 90.0, 0.05, 1.0, 0.04)
    wob = _wobble(0.25, child(r, 'wob'), 170.0 * jitter(r, 1, 0.1), 13.0 + 3 * r.random(), 0.18)
    bub = _bubbles(0.15, child(r, 'bub'), 12.0, 400.0, 1200.0)
    return _outside(layers((splat, 0.0, 0.0), (slap, 0.0, -4.0), (weight, 0.0, -6.0), (wob, 0.02, -12.0),
                           (bub, 0.02, -13.0)), r, -24.0)


@cue('Creature.Slime.Attack', variations=3, att='Creature', jitter=0.06, conc=3, level=-9.0)
def slime_attack(v, r):
    # It rears and throws itself: a deep gloopy gulp, then the wet slap of the hit, bubbles everywhere.
    f = 85.0 * jitter(r, 1, 0.1)
    voice = _gloop(child(r, 'voice'), [(0.0, f), (0.15, f * 1.35), (0.45, f * 0.8)], 0.45,
                   [(0.0, 'o'), (0.2, 'a'), (0.45, 'u')]) * E.perc(0.45, 0.03, 0.4)
    t = 0.3 + 0.04 * r.random()
    splat = layers(kit.squelch(0.12, child(r, 'squelch'), 1600.0, 300.0),
                   (kit.noise_thump(0.12, child(r, 'thump'), 900.0, 120.0, 0.06), 0.0, -3.0))
    bub = _bubbles(0.5, child(r, 'bub'), 25.0, 350.0, 1400.0, 0.0, 0.5)
    return _outside(layers((normalize(voice), 0.0, -2.0), (splat, t, 0.0), (bub, 0.05, -11.0)), r)


@cue('Creature.Slime.Hurt', variations=4, att='Creature', jitter=0.06, conc=3, level=-10.0)
def slime_hurt(v, r):
    # A pained, bubbling blub, the gel churning.
    f = 140.0 * jitter(r, 1, 0.1)
    voice = _gloop(child(r, 'voice'), [(0.0, f), (0.08, f * 1.15), (0.4, f * 0.7)], 0.4, [(0.0, 'u'), (0.4, 'oo')],
                   0.8, 26.0, 0.3) * E.perc(0.4, 0.01, 0.35)
    bub = _bubbles(0.4, child(r, 'bub'), lambda t: 70.0 * np.exp(-t / 0.2), 300.0, 1500.0)
    splat = kit.squelch(0.08, child(r, 'squelch'), 1200.0, 350.0)
    return _outside(layers((normalize(voice), 0.0, 0.0), (bub, 0.0, -7.0), (splat, 0.0, -6.0)), r, -21.0)


@cue('Creature.Slime.Death', variations=3, att='Creature', jitter=0.05, conc=3, level=-8.0)
def slime_death(v, r):
    # It bursts: a heavy splat, a gurgle that sinks and slows as it deflates, bubbles popping slower and slower,
    # the last drips.
    splat = layers(kit.squelch(0.18, child(r, 'squelch'), 1800.0, 250.0),
                   (kit.noise_thump(0.25, child(r, 'thump'), 1500.0, 110.0, 0.12, 1.0, 0.08), 0.0, -1.0),
                   (kit.burst(0.08, child(r, 'wet'), 0.04, lo=400.0, hi=5000.0), 0.0, -6.0))
    f = 110.0 * jitter(r, 1, 0.08)
    dur = 1.0
    voice = _gloop(child(r, 'voice'), [(0.0, f), (dur, 45.0)], dur, [(0.0, 'o'), (0.5, 'u'), (dur, 'u')], 0.75, 22.0, 0.5)
    voice *= E.swell(dur, 0.05, 0.9)
    bub = _bubbles(1.1, child(r, 'bub'), lambda t: 55.0 * np.exp(-t / 0.35) + 3.0, 250.0, 1300.0, 0.05, 1.1)
    drips = layers(*[(kit.bubble(r.uniform(900.0, 2200.0), r, 0.15), 0.85 + 0.12 * i + 0.05 * r.random(), -6.0 * i)
                     for i in range(3)])
    return _outside(layers((splat, 0.0, 0.0), (normalize(voice), 0.04, -5.0), (bub, 0.0, -9.0), (normalize(drips), 0.0, -16.0)), r)


# --- The Unpaid -----------------------------------------------------------------------------------------------------

def _choir(r, f0_points, dur, keys, count=3, detune=14.0, spread=0.04, breath=0.55, shift=1.1, vib=(5.0, 0.3),
           rough=0.0, tilt=0.0):
    """Several breathy voices on one cry, a little apart in pitch and time: one ghost sounds like many."""
    n = ns(dur)
    out = np.zeros(n + ns(spread * count))
    for i in range(count):
        rr = child(r, 'voice', i)
        cents = detune * (i - (count - 1) / 2.0) + rr.uniform(-3.0, 3.0)
        f0 = O.glide(f0_points, n=n) * 2.0 ** (cents / 1200.0)
        x = V.voice(f0, keys, rr, breath=breath, jitter=0.012, shimmer=0.12, tilt=tilt, shift=shift * rr.uniform(0.97, 1.03),
                    vib_rate=vib[0] * rr.uniform(0.85, 1.15), vib_depth=vib[1], rough=rough)
        k = ns(spread * i * rr.uniform(0.5, 1.0))
        out[k:k + n] += normalize(x) * (1.0 if i == 0 else rr.uniform(0.5, 0.8))
    return normalize(out)


def _cold(x, r, rate=33.0, depth=0.2):
    """A faint ring modulation: the voice not quite of this world."""
    n = x.size
    return x * (1.0 - depth + depth * np.sin(2 * np.pi * rate * times(n) + 2 * np.pi * r.random()))


@cue('Creature.Unpaid.Alert', variations=3, att='Creature', jitter=0.04, conc=3, level=-6.0, swell=True)
def unpaid_alert(v, r):
    # It has seen you: a low breath drawn in, then a wail of many voices rising and falling, cold and far-carrying.
    f = [250.0, 228.0, 275.0][v] * jitter(r, 1, 0.03)
    dur = 1.7
    wail = _choir(child(r, 'wail'), [(0.0, f * 0.9), (0.5, f * 1.19), (1.1, f * 1.12), (dur, f * 0.84)], dur,
                  [(0.0, 'u'), (0.45, 'o'), (0.9, 'a'), (dur, 'o')])
    wail = _cold(wail, child(r, 'cold')) * E.swell(wail.size / SR, 0.5, 1.4, 1.6)
    draw = V.whisper(0.45, [(0.0, 'h'), (0.45, 'u')], child(r, 'draw'), 0.9) * E.swell(0.45, 0.4, 0.1, 2.0)
    air = V.whisper(dur, [(0.0, 'h'), (0.8, 'a'), (dur, 'h')], child(r, 'air'), 1.1) * E.swell(dur, 0.7, 1.2)
    # The cold breath around the voices, high and thin.
    frost = F.filt(N.pink(ns(dur), child(r, 'frost'), hi=9000.0), F.hp(1800.0), extend=False)
    frost *= E.swell(dur, 0.8, 1.0) * (0.6 + 0.4 * N.smooth_random(ns(dur), child(r, 'fa'), 3.0))
    x = layers((normalize(draw), 0.0, -10.0), (wail, 0.25, 0.0), (normalize(air), 0.25, -12.0), (normalize(frost), 0.25, -22.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(120.0), extend=False)
    return _haunt(x, r, -7.0)


@cue('Creature.Unpaid.Shriek', variations=3, att='Creature', jitter=0.04, conc=2, level=-4.0)
def unpaid_shriek(v, r):
    # The shriek: a raw scream snapping up, wavering, torn at the edges, a cold buzz in it.
    f = [640.0, 700.0, 600.0][v] * jitter(r, 1, 0.03)
    dur = 1.05
    scream = _choir(child(r, 'scream'), [(0.0, f * 0.75), (0.1, f * 1.6), (0.5, f * 1.45), (dur, f * 1.05)], dur,
                    [(0.0, 'a'), (0.25, 'ae'), (0.6, 'e'), (dur, 'i')], 2, 30.0, 0.012, 0.35, 1.25, (7.5, 0.5), 0.45, 4.0)
    scream = D.drive(scream, 14.0, 'tanh', bias=0.15)
    scream = _cold(scream, child(r, 'cold'), 85.0, 0.3) * E.perc(scream.size / SR, 0.02, 0.9)
    n = ns(dur)
    rasp_ = N.white(n, child(r, 'rasp')) * kit.turbulence(n, child(r, 'rt'), 160.0, 0.8)
    rasp_ = F.sweep(rasp_, 'bp', fit_curve(O.glide([(0.0, f * 2.2), (0.1, f * 4.5), (dur, f * 3.0)], n=n)), 2.5) * E.perc(dur, 0.02, 0.7)
    x = layers((scream, 0.0, 0.0), (normalize(rasp_), 0.0, -11.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.7), F.hp(200.0), extend=False)
    return _haunt(x, r, -9.0, 2.0)


def fit_curve(c):
    return np.clip(c, 60.0, 16000.0)


@cue('Creature.Unpaid.Lunge', variations=3, att='Creature', jitter=0.05, conc=3, level=-7.0, swell=True)
def unpaid_lunge(v, r):
    # It rushes through the air at you: a cold whoosh swelling past, a hissing breath out, a low rumble under it.
    n = ns(0.75)
    rush = N.pink(n, child(r, 'rush'))
    fc = O.glide([(0.0, 450.0), (0.32, 2600.0), (0.75, 600.0)], n=n)
    rush = F.sweep(rush, 'bp', fc, 1.4) * E.swell(0.75, 0.33, 0.5, 2.0)
    breath = V.whisper(0.5, [(0.0, 'h'), (0.15, 'a'), (0.5, 'h')], child(r, 'breath'), 1.05) * E.perc(0.5, 0.03, 0.45)
    rumble = kit.noise_thump(0.6, child(r, 'rumble'), 400.0, 60.0, 0.4, 0.9, 0.3)
    f = 300.0 * jitter(r, 1, 0.08)
    cry = _choir(child(r, 'cry'), [(0.0, f), (0.3, f * 0.8)], 0.3, [(0.0, 'a'), (0.3, 'h')], 2, 20.0, 0.02, 0.7)
    cry = cry * E.perc(cry.size / SR, 0.02, 0.28)
    x = layers((normalize(rush), 0.0, 0.0), (normalize(breath), 0.18, -4.0), (rumble, 0.1, -10.0), (cry, 0.2, -12.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), extend=False)
    return _haunt(x, r, -10.0, 1.6)


@cue('Creature.Unpaid.Hurt', variations=4, att='Creature', jitter=0.05, conc=3, level=-8.0)
def unpaid_hurt(v, r):
    # A pained cry cut short, flickering as if the voice can't hold together.
    f = 360.0 * jitter(r, 1, 0.1)
    dur = 0.45
    cry = _choir(child(r, 'cry'), [(0.0, f * 1.1), (0.05, f * 1.2), (dur, f * 0.68)], dur, [(0.0, 'a'), (dur, 'u')], 2,
                 18.0, 0.015, 0.5, 1.1, (6.0, 0.3), 0.3)
    n = cry.size
    flicker = 0.5 + 0.5 * np.sign(np.sin(2 * np.pi * O.phase(30.0 * (1 + 0.3 * N.smooth_random(n, r, 6.0)), n)))
    flicker = F.filt(flicker, F.lp1(400.0), extend=False)
    cry = cry * (0.55 + 0.45 * flicker) * E.perc(n / SR, 0.01, 0.4)
    x = F.filt(cry, F.peak(3200.0, -3.0, 0.8), F.hp(150.0), extend=False)
    return _haunt(x, r, -10.0, 1.4)


@cue('Creature.Unpaid.Death', variations=3, att='Creature', jitter=0.04, conc=3, level=-6.0, swell=True)
def unpaid_death(v, r):
    # Laid to rest at last: a long wail sinking as its voices drift apart, turning to breath, the coal in its chest
    # crackling out, and a release of air rising away.
    f = [330.0, 300.0, 350.0][v] * jitter(r, 1, 0.03)
    dur = 2.4
    wail = _choir(child(r, 'wail'), [(0.0, f), (0.15, f * 1.12), (dur, f * 0.32)], dur, [(0.0, 'a'), (1.0, 'o'), (dur, 'u')],
                  3, 30.0, 0.05, 0.6, 1.1, (4.5, 0.35), 0.15)
    wail *= E.swell(wail.size / SR, 0.12, 2.2)
    breath = V.whisper(dur, [(0.0, 'a'), (1.2, 'u'), (dur, 'h')], child(r, 'breath'), 1.0) * E.swell(dur, 1.4, 1.2)
    n = ns(1.6)
    ember = G.grit(1.6, child(r, 'ember'), 220.0 * np.exp(-times(n) / 0.6), 1800.0, 6000.0, 3.0, 3)
    ember = F.filt(ember, F.lp(5000.0), extend=False)
    release = kit.whoosh(1.0, child(r, 'release'), 400.0, 5000.0, 1.1, 0.4, -2.0, 0.8)
    x = layers((wail, 0.0, 0.0), (normalize(breath), 0.3, -9.0), (normalize(ember), 0.1, -20.0), (release, 1.5, -14.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(120.0), extend=False)
    return _haunt(x, r, -6.0, 3.0)
