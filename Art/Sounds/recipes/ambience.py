"""The world's air: the beds under everything in each area and light, and the sweeteners the ambience places round the
listener (Audio/AmbienceSubsystem).

A bed is two stereo loops of different lengths played together, the air (wind, grass) in 19 s and the life (insects)
in 13 s, so the pair only lines up again after four minutes and nothing in it is heard to repeat. Every loop is built
in a circle: periodic noise, gust curves that come back to where they started, circular filters, events folded round
the seam, and anything with memory run over two laps with the second kept.

The sweeteners are single calls placed at random distances and times (birds, a crow, a hawk, an owl, insects, a
coyote, thunder far off): mono and in the world, so each comes from somewhere, except the thunder, which rolls in from
everywhere. Distance is mostly the world's (the attenuation's falloff and its muffling); what's far by nature (a hawk
over the canyon, the coyote, the thunder) is made far here too, darker and wetter.
"""
import numpy as np

from lib.core import SR, ns, times, normalize, child, db2a, fit, layers
from lib import noise as N, env as E, osc as O, filters as F, dist as D, reverb as R, stereo as S, voice as V
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes import band as B

AIR_SECONDS = 19.0
LIFE_SECONDS = 13.0


# --- Loop tools -------------------------------------------------------------------------------------------------------

def _band(n, r, lo, hi, slope=-3.0):
    """Periodic noise in a band (unit RMS)."""
    return N.shaped(n, r, slope, lo, hi, periodic=True)


def _gusts(n, r, rate=0.09, depth=0.45, flutter=0.15, floor=0.12):
    """A periodic gust curve, about 0..1: slow swells with a quicker flutter on them."""
    g = 0.55 + depth * N.smooth_random(n, child(r, 'slow'), rate, periodic=True)
    g += flutter * N.smooth_random(n, child(r, 'fast'), rate * 5.0, periodic=True)
    return np.clip(g, floor, 1.25)


def _circ(x, *sections):
    return F.filt(x, *sections, circular=True)


def _fold_events(L, events):
    """Events (signal, time) laid on a loop of L samples, tails folded round."""
    out = np.zeros(L + ns(4.0))
    for sig, t in events:
        start = ns(t) % L
        end = start + sig.size
        if end > out.size:
            out = np.pad(out, (0, end - out.size))
        out[start:end] += sig
    return B.fold(out, L)


def _wide(l, r_):
    return np.vstack([l, r_])


def _wind(n, r, g, lo_db=0.0, mid_db=-4.0, hiss_db=-12.0, rustle=30.0, hollow=None, hollow_db=-18.0, dry=False,
          floor=0.45):
    """One channel of wind over open ground: a low rush that's always there, a mid body that swells with the gusts,
    the grass's hiss over it (growing fastest with the gusts, broken into a rustle), and the hollow tones of a canyon
    when given (narrow resonances the wind sings in)."""
    low = _circ(_band(n, child(r, 'low'), 50.0, 420.0, -4.5), F.lp(350.0, 0.6)) * (floor + (1.0 - floor) * g)
    mid = _band(n, child(r, 'mid'), 280.0, 1600.0, -3.0) * g ** 1.4
    hiss = _band(n, child(r, 'hiss'), 1600.0, 9000.0, -1.5)
    flick = 0.35 + 0.65 * np.abs(N.smooth_random(n, child(r, 'rustle'), rustle, periodic=True)) ** 1.4
    hiss = hiss * flick * g ** 2.2
    if dry:
        # Dust: a fine grit lifting on the gusts.
        grit = G.grit(n / SR, child(r, 'grit'), 400.0 * g ** 3, 2500.0, 7000.0, 2.0, 3, periodic=True)
        hiss = hiss + 0.5 * normalize(grit) * np.sqrt(np.mean(hiss * hiss)) / (np.sqrt(np.mean(grit * grit)) + 1e-12) * 0.5
    x = (normalize(low) * db2a(lo_db) + normalize(mid) * db2a(mid_db) + normalize(hiss) * db2a(hiss_db))
    if hollow:
        tone = np.zeros(n)
        src = _band(n, child(r, 'hollow'), 40.0, 900.0, -3.0)
        for i, f in enumerate(hollow):
            swell = 0.4 + 0.6 * np.abs(N.smooth_random(n, child(r, 'hs', i), 0.12, periodic=True))
            tone += _circ(src, F.bp(f, 18.0), F.bp(f, 18.0)) * swell / (1.0 + 0.4 * i)
        x = x + normalize(tone) * db2a(hollow_db) * (0.5 + 0.5 * g)
    return x


def _air_bed(r, dur, rate, depth, lo_db, mid_db, hiss_db, rustle, hollow=None, hollow_db=-18.0, dry=False, floor=0.45):
    n = ns(dur)
    g = _gusts(n, child(r, 'gusts'), rate, depth)
    # The gust reaches one ear a moment before the other, as wind crosses.
    g2 = np.roll(g, ns(0.45))
    left = _wind(n, child(r, 'L'), g, lo_db, mid_db, hiss_db, rustle, hollow, hollow_db, dry, floor)
    right = _wind(n, child(r, 'R'), g2, lo_db, mid_db, hiss_db, rustle, hollow, hollow_db, dry, floor)
    x = _wide(left, right)
    return _circ(x, F.hpn(50.0, 2), F.highshelf(8000.0, -3.0))


# --- Insects ----------------------------------------------------------------------------------------------------------

def _cicada(dur, r, fc, rate, swell_in=2.5, hold=3.0, swell_out=2.0):
    """A cicada's song: its tymbals clicking a few hundred times a second, the clicks ringing its body (a band of
    noise high up), swelling in, holding, and dying away."""
    n = ns(dur)
    t = times(n)
    clicks = np.abs(np.sin(np.pi * rate * t)) ** 6
    body = F.filt(N.white(n, r), F.bp(fc, 3.5), F.bp(fc * 1.02, 3.5), extend=False)
    env = E.bp([(0.0, 0.0), (swell_in, 1.0), (swell_in + hold, 0.9), (swell_in + hold + swell_out, 0.0)], n=n,
               curve='cos')
    return normalize(body * (0.25 + 0.75 * clicks) * env, 0.0)


def _grasshopper(r, fc, count, rate):
    """A grasshopper's rasp: a leg drawn over its wing's file, a train of tiny ticks."""
    gap = 1.0 / rate
    out = np.zeros(ns(count * gap + 0.05))
    for i in range(count):
        tick = F.filt(N.white(ns(0.006), child(r, 't', i)) * E.decay(ns(0.006), 0.002), F.bp(fc, 2.5), extend=False)
        out[ns(i * gap * r.uniform(0.92, 1.08)):][:tick.size] += tick * r.uniform(0.5, 1.0)
    return normalize(out, 0.0)


def _cricket_chirp(r, fc, pulses=3, pulse=0.016, gap=0.036):
    """A field cricket's chirp: a few pulses of an almost pure tone (its wing's harp ringing as the file is drawn)."""
    n = ns(pulses * gap + pulse + 0.01)
    out = np.zeros(n)
    for i in range(pulses):
        k = ns(pulse)
        t = times(k)
        p = np.sin(2 * np.pi * fc * (1.0 + 0.004 * (2 * r.random() - 1)) * t) * np.sin(np.pi * t / pulse) ** 2
        out[ns(i * gap):ns(i * gap) + k] += p * (0.8 + 0.2 * r.random())
    return out


def _distance(x, metres):
    """Air takes the top off a far sound."""
    lp = float(np.clip(14000.0 / (1.0 + metres / 12.0), 2500.0, 12000.0))
    return F.filt(x, F.lp(lp, 0.7), extend=False) * (1.0 / (1.0 + metres / 6.0))


def _scatter_chorus(L, r, make, count, spread_m=(6.0, 40.0)):
    """A chorus of callers at random distances and places in the stereo field (each a mono loop of L samples made by
    make(r, i) and set by distance)."""
    out = np.zeros((2, L))
    for i in range(count):
        ri = child(r, 'c', i)
        x = make(ri, i)
        metres = ri.uniform(*spread_m)
        x = F.filt(x, F.lp(float(np.clip(14000.0 / (1.0 + metres / 12.0), 2500.0, 12000.0)), 0.7), circular=True)
        x = x / (1.0 + metres / 6.0)
        out += S.pan(x, ri.uniform(-0.85, 0.85)) / np.sqrt(2.0)
    return out


def _open_air(x, r, wet_db=-10.0, t60=1.6, circular=True):
    """A loop in the open: a sparse, dark wash of what comes back off the hills."""
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=t60, lp=4000.0, density=0.7)
    L = x.shape[-1]
    wet = B.circ_convolve(x, ir, L) if circular else F.convolve(x, ir)
    return (x if x.ndim == 2 else S.pan(x, 0.0) / np.sqrt(2.0)) + wet[..., :L] * db2a(wet_db)


# --- Beds -------------------------------------------------------------------------------------------------------------

@cue('Ambience.Skyreach.Air', variations=1, space='2D', cls='Ambience', jitter=0.0, conc=2, loop=True, level=-26.0,
     align=False)
def skyreach_air(v, r):
    # A sky island's breeze: soft, steady air over the meadow with easy swells, the grass and the hedges rustling
    # when it rises, and under it the faint, cool rush of the open sky all round the island.
    x = _air_bed(r, AIR_SECONDS, 0.08, 0.35, 0.0, -6.0, -13.0, 26.0)
    n = x.shape[-1]
    sky = _band(n, child(r, 'sky'), 300.0, 1400.0, -2.0)
    sky = _circ(sky, F.peak(650.0, 4.0, 1.5)) * (0.6 + 0.4 * N.smooth_random(n, child(r, 'skyg'), 0.05, periodic=True))
    sky = normalize(sky) * 0.08
    return x / np.max(np.abs(x)) + np.vstack([sky, np.roll(sky, ns(0.7))])


@cue('Ambience.Skyreach.Life', variations=1, space='2D', cls='Ambience', jitter=0.0, conc=2, loop=True, level=-29.0,
     align=False)
def skyreach_life(v, r):
    # The meadow by day: grasshoppers rasping here and there in the grass, and a hum of bees working the flowers off
    # to one side (the birds are the sweeteners').
    L = ns(LIFE_SECONDS)

    def hopper(ri, i):
        events = []
        t = ri.uniform(0.0, LIFE_SECONDS)
        for k in range(int(ri.integers(2, 5))):
            events.append((_grasshopper(child(ri, k), ri.uniform(5500.0, 8000.0), int(ri.integers(8, 30)),
                                        ri.uniform(28.0, 42.0)), t))
            t += ri.uniform(1.5, 4.5)
        return _fold_events(L, events)

    hoppers = _scatter_chorus(L, child(r, 'hop'), hopper, 7, (4.0, 30.0))
    t = times(L)
    hum = np.zeros(L)
    for i in range(5):
        ri = child(r, 'bee', i)
        f0 = ri.uniform(190.0, 250.0) * (1.0 + 0.02 * N.smooth_random(L, child(ri, 'f'), 1.5, periodic=True))
        tone = O.additive(f0, lambda k: 1.0 / k ** 1.1, n=L)
        hum += tone * (0.3 + 0.7 * np.abs(N.smooth_random(L, child(ri, 'a'), 0.3, periodic=True)))
    hum = _circ(hum, F.lp(1800.0, 0.6), F.hp(150.0))
    hum = S.pan(normalize(hum), 0.35) / np.sqrt(2.0) * 0.18
    x = hoppers / (np.max(np.abs(hoppers)) + 1e-9) + hum
    return _open_air(x, r, -14.0)


@cue('Ambience.RansomsRest.DayAir', variations=1, space='2D', cls='Ambience', jitter=0.0, conc=2, loop=True,
     level=-24.0, align=False)
def ransoms_rest_day_air(v, r):
    # The Rest by day: a dry wind off the canyon with real gusts, the late-summer grass hissing as they come through,
    # dust lifting on the strongest, and the canyon's hollow note far under it.
    x = _air_bed(r, AIR_SECONDS, 0.11, 0.58, 0.0, -3.0, -9.0, 34.0, hollow=(96.0, 143.0, 212.0), hollow_db=-24.0,
                 dry=True, floor=0.22)
    return x


@cue('Ambience.RansomsRest.DayLife', variations=1, space='2D', cls='Ambience', jitter=0.0, conc=2, loop=True,
     level=-31.0, align=False)
def ransoms_rest_day_life(v, r):
    # Cicadas in the heat: a few singers at different distances swelling up, holding and dying away, and grasshoppers
    # rasping in the dry grass between them.
    L = ns(LIFE_SECONDS)

    def singer(ri, i):
        fc = ri.uniform(4300.0, 6200.0)
        rate = ri.uniform(150.0, 240.0)
        sin, hold, sout = ri.uniform(1.5, 3.0), ri.uniform(2.0, 5.0), ri.uniform(1.5, 2.5)
        x = _cicada(sin + hold + sout, child(ri, 's'), fc, rate, sin, hold, sout)
        return _fold_events(L, [(x, ri.uniform(0.0, LIFE_SECONDS))])

    cicadas = _scatter_chorus(L, child(r, 'cic'), singer, 4, (14.0, 45.0))

    def hopper(ri, i):
        events = []
        t = ri.uniform(0.0, LIFE_SECONDS)
        for k in range(int(ri.integers(2, 4))):
            events.append((_grasshopper(child(ri, k), ri.uniform(5000.0, 7500.0), int(ri.integers(10, 26)),
                                        ri.uniform(25.0, 38.0)), t))
            t += ri.uniform(2.0, 5.0)
        return _fold_events(L, events)

    hoppers = _scatter_chorus(L, child(r, 'hop'), hopper, 5, (4.0, 25.0))
    x = cicadas / (np.max(np.abs(cicadas)) + 1e-9) + 0.6 * hoppers / (np.max(np.abs(hoppers)) + 1e-9)
    # Steady insect song is tiring right where the ear is sharpest: rounded off there.
    x = _circ(x, F.peak(4500.0, -3.0, 0.8), F.highshelf(9000.0, -6.0))
    return _open_air(x, r, -12.0)


@cue('Ambience.RansomsRest.DuskAir', variations=1, space='2D', cls='Ambience', jitter=0.0, conc=2, loop=True,
     level=-23.0, align=False)
def ransoms_rest_dusk_air(v, r):
    # The Gravewind at dusk: colder and steadier than the day's, pouring off the high ground toward the canyon with a
    # long moan in it, the canyon's hollow notes singing louder, the grass laid flat and quieter.
    return _air_bed(r, AIR_SECONDS, 0.06, 0.4, 0.0, -2.0, -14.0, 18.0, hollow=(82.0, 123.0, 184.0, 246.0),
                    hollow_db=-15.0)


@cue('Ambience.RansomsRest.DuskLife', variations=1, space='2D', cls='Ambience', jitter=0.0, conc=2, loop=True,
     level=-30.0, align=False)
def ransoms_rest_dusk_life(v, r):
    # Crickets at dusk: field crickets chirping in threes, each at its own pitch and pace, near and far, over the
    # soft steady trill of tree crickets in the scrub.
    L = ns(LIFE_SECONDS)

    def field(ri, i):
        fc = ri.uniform(4200.0, 5000.0)
        # A whole number of chirps in the loop, so each cricket keeps its pace across the seam.
        period = LIFE_SECONDS / round(LIFE_SECONDS / ri.uniform(0.38, 0.62))
        phase = ri.uniform(0.0, period)
        events = []
        t = phase
        k = 0
        while t < LIFE_SECONDS:
            # Now and then it pauses a beat, as crickets do.
            if ri.random() > 0.12:
                events.append((_cricket_chirp(child(ri, k), fc, int(ri.integers(3, 5))), t))
            t += period
            k += 1
        return _fold_events(L, events)

    crickets = _scatter_chorus(L, child(r, 'field'), field, 6, (3.0, 35.0))

    def tree(ri, i):
        rate = ri.uniform(38.0, 50.0)
        rate = round(rate * LIFE_SECONDS) / LIFE_SECONDS
        t = times(L)
        fc = ri.uniform(2200.0, 3200.0)
        pulses = np.abs(np.sin(np.pi * rate * t)) ** 4
        tone = np.sin(2 * np.pi * fc * t)
        swell = 0.4 + 0.6 * np.abs(N.smooth_random(L, child(ri, 'sw'), 0.15, periodic=True))
        return tone * pulses * swell

    trees = _scatter_chorus(L, child(r, 'tree'), tree, 3, (15.0, 40.0))
    x = crickets / (np.max(np.abs(crickets)) + 1e-9) + 0.18 * trees / (np.max(np.abs(trees)) + 1e-9)
    x = _circ(x, F.peak(4600.0, -2.0, 1.0), F.highshelf(9000.0, -6.0))
    return _open_air(x, r, -9.0, t60=2.0)


# --- Sweeteners -------------------------------------------------------------------------------------------------------

def _outdoors(x, r, wet_db=-12.0, t60=1.4, lp=4000.0, density=0.7):
    """A one-shot heard outdoors: a sparse, dark wash after it."""
    ir = R.open_air(child(r, 'ir'), t60=t60, lp=lp, density=density)
    wet = F.convolve(F.filt(x, F.hp(200.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, wet_db, 0.05))


def _song_note(r, f0, f1, dur, shape='slur', trill=0.0):
    """One whistled syllable of a songbird: a near-pure tone gliding (a slur, a chevron up and down, a sweep), with a
    trill on it when given (the syrinx's two sides beating)."""
    n = ns(dur)
    u = np.linspace(0.0, 1.0, n)
    if shape == 'chevron':
        f = f0 + (f1 - f0) * np.sin(np.pi * u)
    elif shape == 'hook':
        f = f0 * (f1 / f0) ** (u ** 0.4)
    else:
        f = f0 * (f1 / f0) ** u
    if trill:
        f = f * (1.0 + 0.06 * np.sin(2 * np.pi * trill * u * dur))
    ph = 2 * np.pi * O.phase(f, n)
    x = np.sin(ph) + 0.08 * np.sin(2 * ph)
    env = np.sin(np.pi * u) ** 0.7
    if trill:
        env = env * (0.55 + 0.45 * np.abs(np.sin(np.pi * trill * u * dur)))
    return x * env


@cue('Ambience.Bird.Songbird', variations=6, att='Gun', cls='Ambience', jitter=0.05, conc=3, level=-9.0)
def songbird(v, r):
    # A small songbird in a nearby tree: a phrase of whistled syllables (slurs, chevrons, a trill, a buzzy end), each
    # take its own song, as the meadow's birds each have theirs.
    out = np.zeros(1)
    t = 0.0
    base = r.uniform(2400.0, 3800.0)
    count = int(r.integers(4, 8))
    for i in range(count):
        kind = r.choice(['slur', 'chevron', 'hook', 'trill', 'slur'])
        f0 = base * r.uniform(0.75, 1.3)
        f1 = f0 * r.choice([0.7, 0.85, 1.2, 1.4])
        dur = r.uniform(0.05, 0.16)
        if kind == 'trill':
            dur = r.uniform(0.25, 0.5)
            x = _song_note(child(r, i), f0, f0 * r.uniform(0.9, 1.1), dur, 'slur', trill=r.uniform(18.0, 30.0))
        else:
            x = _song_note(child(r, i), f0, f1, dur, kind)
        out = layers(out, (x * r.uniform(0.6, 1.0), t))
        t += dur + r.uniform(0.03, 0.12)
    out = F.filt(out, F.hp(1200.0), extend=False)
    return _outdoors(normalize(out), r, -16.0, 1.2)


def _caw(r, f0, dur, rough=0.6):
    """A crow's caw: a hoarse, nasal voice with a noisy rasp, its pitch falling as it opens."""
    n = ns(dur)
    f = O.glide([(0.0, f0 * 1.08), (dur * 0.3, f0), (dur, f0 * 0.82)], n=n)
    x = V.voice(f, [(0.0, 'ae'), (dur * 0.35, 'a'), (dur, 'o')], child(r, 'v'), breath=0.35, jitter=0.03,
                shimmer=0.25, tilt=1.0, shift=1.7, rough=rough)
    x = D.drive(normalize(x), 8.0, 'tanh', bias=0.2)
    return F.filt(x * E.ar(dur, 0.02, dur * 0.4, 2.0), F.hp(350.0), F.peak(1600.0, 3.0, 1.2), F.lp(6000.0),
                  extend=False)


@cue('Ambience.Bird.Crow', variations=4, att='Gun', cls='Ambience', jitter=0.05, conc=2, level=-8.0)
def crow(v, r):
    # A crow off across the fields: one to three hoarse caws, the hills giving them back.
    out = np.zeros(1)
    t = 0.0
    for i in range([1, 2, 3, 2][v]):
        out = layers(out, (_caw(child(r, i), r.uniform(430.0, 560.0), r.uniform(0.28, 0.42)), t, -1.5 * i))
        t += r.uniform(0.45, 0.7)
    return _outdoors(normalize(out), r, -10.0, 1.8)


@cue('Ambience.Bird.Hawk', variations=3, att='Gun', cls='Ambience', jitter=0.03, conc=1, level=-4.0, swell=True)
def hawk(v, r):
    # A hawk's cry high over the canyon: a hoarse, rising scream that falls away, small with distance.
    dur = r.uniform(1.3, 1.8)
    n = ns(dur)
    f = O.glide([(0.0, 2000.0), (0.18, 2900.0), (dur * 0.55, 2700.0), (dur, 1700.0)], n=n)
    f = f * r.uniform(0.92, 1.06)
    ph = 2 * np.pi * O.phase(f, n)
    tone = np.sin(ph) + 0.35 * np.sin(2 * ph + 0.4) + 0.12 * np.sin(3 * ph)
    rasp = F.sweep(N.white(n, child(r, 'rasp')), 'bp', np.clip(f, 500.0, 15000.0), 4.0)
    rasp = rasp / (np.sqrt(np.mean(rasp * rasp)) + 1e-9)
    am = 1.0 + 0.25 * N.smooth_random(n, child(r, 'am'), 60.0)
    x = (normalize(tone) * 0.8 + normalize(rasp) * 0.5) * am * E.ar(dur, 0.12, dur * 0.6, 2.0)
    x = _distance(normalize(x), 60.0)
    return _outdoors(normalize(x), r, -7.0, 2.6, 3500.0, 0.5)


@cue('Ambience.Bird.Owl', variations=3, att='Gun', cls='Ambience', jitter=0.03, conc=1, level=-9.0, swell=True)
def owl(v, r):
    # A lone owl at dusk: soft, deep hoots in its rhythm (hoo, hoo-hoo, hooo), each one breathy and falling a little.
    pattern = [[(0.0, 0.35), (0.6, 0.18), (0.82, 0.2), (1.2, 0.55)], [(0.0, 0.45), (0.7, 0.45)],
               [(0.0, 0.25), (0.4, 0.22), (0.8, 0.6)]][v]
    f0 = r.uniform(290.0, 340.0)
    out = np.zeros(1)
    for i, (t, d) in enumerate(pattern):
        n = ns(d)
        f = O.glide([(0.0, f0 * 1.03), (d * 0.3, f0), (d, f0 * 0.9)], n=n)
        x = V.voice(f, [(0.0, 'u'), (d, 'oo')], child(r, i), breath=0.3, jitter=0.006, shimmer=0.05, tilt=-6.0,
                    shift=0.9, top=2500.0)
        x = x * E.ar(d, 0.06, d * 0.5, 2.0)
        out = layers(out, (x, t))
    out = F.filt(out, F.hp(150.0), F.lp(2500.0), extend=False)
    return _outdoors(normalize(out), r, -8.0, 2.2, 3000.0)


@cue('Ambience.Insect.Chirp', variations=4, att='Creature', cls='Ambience', jitter=0.04, conc=2, level=-16.0)
def insect_chirp(v, r):
    # Something small close by in the grass: a cricket's chirps (takes 1 and 2) or a grasshopper's rasp (3 and 4).
    if v < 2:
        fc = r.uniform(4300.0, 4900.0)
        out = np.zeros(1)
        for k in range(int(r.integers(3, 6))):
            out = layers(out, (_cricket_chirp(child(r, k), fc, 3), k * r.uniform(0.42, 0.5)))
    else:
        out = _grasshopper(child(r, 'g'), r.uniform(5200.0, 7000.0), int(r.integers(14, 30)), r.uniform(26.0, 38.0))
    out = F.filt(normalize(out), F.peak(4600.0, -3.0, 1.0), extend=False)
    return _outdoors(out, r, -14.0, 0.7)


@cue('Ambience.Insect.Bee', variations=3, att='Creature', cls='Ambience', jitter=0.06, conc=2, level=-14.0, swell=True)
def bee(v, r):
    # A bee working past: its wings' buzz swelling and falling as it comes and goes, its pitch dropping as it passes.
    dur = r.uniform(2.0, 3.2)
    n = ns(dur)
    u = np.linspace(-1.0, 1.0, n)
    f0 = r.uniform(200.0, 240.0) * (1.0 - 0.03 * np.tanh(3.0 * u)) * (1.0 + 0.015 * N.smooth_random(n, r, 6.0))
    tone = O.additive(f0, lambda k: 1.0 / k ** 1.05, n=n)
    near = 1.0 / (1.0 + 6.0 * u * u)
    x = F.filt(tone * near, F.hp(150.0), F.lp(2400.0), extend=False)
    return normalize(x * E.ar(dur, 0.3, 0.4, 2.0), 0.0)


@cue('Ambience.Coyote.Far', variations=3, att='Gun', cls='Ambience', jitter=0.04, conc=1, level=-5.0)
def coyote(v, r):
    # Bone hounds far off at dusk: a few yips climbing into a long wavering howl that breaks, the canyon carrying it.
    out = np.zeros(1)
    t = 0.0
    for k in range(int(r.integers(2, 5))):
        d = r.uniform(0.12, 0.2)
        f0 = r.uniform(700.0, 950.0)
        n = ns(d)
        f = O.glide([(0.0, f0 * 0.8), (d * 0.4, f0 * 1.15), (d, f0 * 0.9)], n=n)
        x = V.voice(f, [(0.0, 'oo'), (d, 'u')], child(r, 'y', k), breath=0.25, jitter=0.02, shimmer=0.1, tilt=0.0,
                    shift=1.6)
        out = layers(out, (x * E.ar(d, 0.01, d * 0.5, 2.0), t))
        t += d + r.uniform(0.05, 0.15)
    d = r.uniform(1.4, 2.2)
    n = ns(d)
    f = O.glide([(0.0, 600.0), (0.35, 980.0), (d * 0.7, 900.0), (d, 640.0)], n=n) * r.uniform(0.92, 1.08)
    f = f * (1.0 + 0.012 * np.sin(2 * np.pi * 5.5 * times(n)))
    howl = V.voice(f, [(0.0, 'u'), (0.4, 'oo'), (d, 'u')], child(r, 'howl'), breath=0.2, jitter=0.01, shimmer=0.06,
                   tilt=-1.0, shift=1.55)
    out = layers(out, (howl * E.ar(d, 0.15, 0.5, 2.0), t + 0.1))
    out = _distance(normalize(out), 80.0)
    return _outdoors(normalize(out), r, -5.0, 3.0, 2800.0, 0.5)


@cue('Ambience.Thunder.Far', variations=3, space='2D', cls='Ambience', jitter=0.0, conc=1, level=-16.0, swell=True)
def thunder(v, r):
    # Thunder far off past the canyon: no crack, only the long roll, a few swells of it coming back off the mesas,
    # low and wide, from everywhere at once.
    dur = r.uniform(4.5, 7.0)
    n = ns(dur)
    chans = []
    rolls = sorted(r.uniform(0.0, dur * 0.6, int(r.integers(3, 6))))
    for c in range(2):
        rc = child(r, 'c', c)
        x = N.brown(n, rc, lo=25.0, hi=500.0)
        env = np.zeros(n)
        for k, t0 in enumerate(rolls):
            env += E.bp([(0.0, 0.0), (t0, 0.0), (t0 + 0.4, 1.0 / (1 + 0.3 * k)), (t0 + 2.2, 0.15), (dur, 0.0)], n=n,
                        curve='cos')
        env *= 0.8 + 0.2 * N.smooth_random(n, child(rc, 'g'), 6.0)
        chans.append(F.filt(x * env, F.lp(380.0, 0.6), F.hp(30.0), extend=False))
    return normalize(np.vstack(chans), 0.0)
