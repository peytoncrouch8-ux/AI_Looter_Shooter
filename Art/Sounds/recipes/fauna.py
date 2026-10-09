"""The ambient fauna (World/Fauna*): crows, sparrows, swallows and a hawk, flies and a dragonfly, tumbleweeds, dust
devils and washing on a line. All 3D, heard where the creature is (World.Fauna.*; their names: the Fauna section of
Audio/LooterSoundCues.h).

Birds sing with a syrinx, not a throat: two vibrating membranes that make nearly pure tones and bend them fast, so a
call is a pitch curve (sweeps, chevrons, warbles, trills) more than a vowel, with a few weak harmonics, a breath of
noise riding on the note and the membranes' tiny flutter, louder where the pitch is highest. The crow is the exception:
a hoarse caw rich in harmonics, made like the creatures' voices (formants, roughness, overdrive). Wings are pushes of
air: each downstroke a dark whump with the feathers' rustle on it, a crow's few and heavy, a sparrow's a fast whirr.
Every take is its own phrase.

The loops (the flies, the dust devil) are built in a circle like the world's: periodic noise and curves, a whole number
of wingbeats in the loop, filters with memory run over laps (band.periodic).
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a
from lib import noise as N, env as E, osc as O, filters as F, dist as D, reverb as R, voice as V
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes import band as B


def _outdoors(x, r, wet_db=-14.0, t60=1.2, lp=4000.0, density=0.7):
    """A one-shot in the open: a sparse, dark wash after it off the ground and the buildings."""
    ir = R.open_air(child(r, 'ir'), t60=t60, lp=lp, density=density)
    wet = F.convolve(F.filt(x, F.hp(200.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, wet_db, 0.05))


def _rms(x):
    return float(np.sqrt(np.mean(x * x))) + 1e-12


# --- The syrinx -------------------------------------------------------------------------------------------------------

def _contour(points, n):
    """A pitch curve through (fraction of the note, Hz) points: geometric between them and eased (cosine), so it bends
    like a bird's note rather than in straight lines."""
    pts = sorted(points)
    u = np.linspace(0.0, 1.0, n)
    xs = np.array([p[0] for p in pts], dtype=float)
    ys = np.log(np.array([p[1] for p in pts], dtype=float))
    i = np.clip(np.searchsorted(xs, u, side='right') - 1, 0, len(xs) - 2)
    w = np.clip((u - xs[i]) / np.maximum(xs[i + 1] - xs[i], 1e-9), 0.0, 1.0)
    w = 0.5 - 0.5 * np.cos(np.pi * w)
    return np.exp(ys[i] + (ys[i + 1] - ys[i]) * w)


def _note(r, points, dur, harm=(1.0, 0.1, 0.03), breath=0.04, flutter=0.004, attack=0.004, release=None, fm=None,
          am=None):
    """One syrinx note: a tone following the pitch points over dur seconds, with weak harmonics (harm), a breath of
    noise on its pitch, the membranes' flutter, a quick swell in and out, louder where the pitch is highest. fm (rate,
    depth): a warble on the pitch; am (rate, depth): a roll on the loudness (a rolled 'rrr')."""
    n = max(ns(dur), 16)
    f = _contour(points, n)
    if fm:
        f = f * (1.0 + fm[1] * np.sin(2.0 * np.pi * fm[0] * times(n) + 2.0 * np.pi * r.random()))
    f = f * (1.0 + flutter * N.smooth_random(n, child(r, 'flutter'), 150.0))
    ph = 2.0 * np.pi * O.phase(f, n)
    tone = np.zeros(n)
    for k, a in enumerate(harm, 1):
        if a:
            tone += a * np.clip((18000.0 - k * f) / 2000.0, 0.0, 1.0) * np.sin(k * ph + 0.3 * k)
    x = tone / _rms(tone)
    if breath:
        nz = F.sweep(N.white(n, child(r, 'breath')), 'bp', np.clip(f, 300.0, 16000.0), 5.0)
        x = x * np.sqrt(1.0 - breath) + nz / _rms(nz) * np.sqrt(breath)
    env = E.ar(dur, min(attack, dur * 0.4), release or dur * 0.55, 2.0)[:n]
    env = np.pad(env, (0, n - env.size))
    env = env * (f / f.max()) ** 0.7
    if am:
        env = env * (1.0 - am[1] + am[1] * (0.5 + 0.5 * np.sin(2.0 * np.pi * am[0] * times(n))))
    return E.end_fade(x * env, 0.002)


def _far(x, metres):
    """Air takes the top off a far sound."""
    lp = float(np.clip(15000.0 / (1.0 + metres / 15.0), 2500.0, 13000.0))
    return F.filt(x, F.lp(lp, 0.7), extend=False)


# --- Wings ------------------------------------------------------------------------------------------------------------

def _wingbeat(r, size=1.0):
    """One downstroke: a dark whump of pushed air (noise through a low resonance, higher for a small bird) with the
    feathers' rustle on it as they open and close. size 1 is a crow's wing, 0.3 a sparrow's."""
    dur = 0.05 + 0.1 * size
    n = ns(dur)
    fc = 240.0 / np.sqrt(size)
    whump = F.filt(N.pink(n, child(r, 'whump')), F.bp(fc * jitter(r, 1, 0.1), 0.9), F.lp(1500.0 / np.sqrt(size), 0.7),
                   extend=False)
    whump = whump * E.perc(dur, 0.008 * size + 0.002, dur * 0.6)
    rustle = N.band(n, child(r, 'rustle'), 1000.0, 6000.0) * kit.turbulence(n, child(r, 'rt'), 220.0 / size, 0.85)
    rustle = rustle * E.perc(dur, 0.004, dur * 0.45)
    return normalize(whump) + 0.35 * normalize(rustle)


def _flight(r, size, start, beats, period0, period1, decay, upstroke_db=-12.0):
    """One bird's wingbeats from start: beats strokes, their period easing from period0 to period1 (fast at take-off,
    slower as it climbs), fading by decay seconds as it flies off, each upstroke a softer rustle between."""
    out = np.zeros(1)
    t = start
    for k in range(beats):
        u = k / max(beats - 1, 1)
        period = period0 + (period1 - period0) * u
        a = -8.686 * (t - start) / decay + r.uniform(-1.5, 0.0)
        out = layers(out, (_wingbeat(child(r, 'beat', k), size * jitter(r, 1, 0.08)), t, a))
        up = N.band(ns(0.04 + 0.05 * size), child(r, 'up', k), 1500.0, 6500.0)
        up = up * E.perc(0.04 + 0.05 * size, 0.01, 0.03 + 0.03 * size)
        out = layers(out, (normalize(up), t + period * 0.5, a + upstroke_db))
        t += period * jitter(r, 1, 0.06)
    return out


# --- Crows ------------------------------------------------------------------------------------------------------------

def _caw(r, f0, dur, harsh=0.5):
    """A crow's caw: a hoarse, nasal 'kaaah' (a harmonic voice through a small, beaky tract), its pitch arching up and
    falling, broken into a rasp by the membranes slapping irregularly, overdriven."""
    n = ns(dur)
    f = O.glide([(0.0, f0 * 0.9), (dur * 0.18, f0 * 1.07), (dur * 0.55, f0), (dur, f0 * 0.8)], n=n)
    x = V.voice(f, [(0.0, 'ae'), (dur * 0.3, 'a'), (dur, 'uh')], child(r, 'voice'), breath=0.3, jitter=0.04,
                shimmer=0.3, tilt=2.0, shift=1.65, rough=harsh)
    rasp = 1.0 - 0.4 * np.clip(N.smooth_random(n, child(r, 'rasp'), 160.0), 0.0, 1.0)
    x = D.drive(normalize(x * rasp), 10.0, 'tanh', bias=0.15)
    x = x * E.ar(dur, 0.015, dur * 0.45, 2.0)
    return normalize(F.filt(x, F.hp(380.0), F.peak(1500.0, 3.0, 1.2), F.peak(3200.0, -3.0, 1.0), F.lp(6500.0),
                            extend=False))


@cue('World.Fauna.Crow.Caw', variations=5, att='Gun', cls='Ambience', jitter=0.04, conc=2, level=-9.0)
def crow_caw(v, r):
    # A crow on a post or circling over: two or three harsh caws in a row (one take a single long one), each a little
    # different, the last often lower, the ground and the buildings giving them back.
    count = [2, 3, 2, 1, 3][v]
    f0 = r.uniform(470.0, 560.0)
    out = np.zeros(1)
    t = 0.0
    for i in range(count):
        d = r.uniform(0.26, 0.38) if count > 1 else r.uniform(0.45, 0.55)
        caw = _caw(child(r, 'caw', i), f0 * (1.0 - 0.04 * i) * jitter(r, 1, 0.03), d, r.uniform(0.35, 0.65))
        out = layers(out, (caw, t, -1.0 * i - r.uniform(0.0, 1.5)))
        t += d + r.uniform(0.18, 0.34)
    return _outdoors(normalize(out), r, -17.0, 1.2)


@cue('World.Fauna.Crow.TakeOff', variations=3, att='Gun', cls='Ambience', jitter=0.04, conc=2, level=-9.0)
def crow_takeoff(v, r):
    # A crow flock bursting off its perches: a scrabble of feet and a rustle as they go, heavy wingbeats overlapping,
    # fast at first and slowing as they climb away, a startled caw or two over them.
    birds = 3 + int(r.integers(0, 3))
    flock = np.zeros(1)
    for i in range(birds):
        start = r.uniform(0.0, 0.3) if i else 0.0
        flock = layers(flock, (_flight(child(r, 'bird', i), 1.0, start, int(r.integers(7, 10)), r.uniform(0.13, 0.16),
                                       r.uniform(0.2, 0.24), r.uniform(1.0, 1.5)), 0.0, r.uniform(-4.0, 0.0)))
    flock = normalize(flock)
    n = flock.size
    # Climbing away: the top goes first.
    flock = F.sweep(flock, 'lp', O.expsweep(9000.0, 2500.0, n / SR, 1.0, n=n), 0.7)
    scrabble = kit.grit_burst(0.2, child(r, 'scrabble'), 1500.0, 900.0, 4000.0, 0.12)
    rustle = kit.cloth(0.25, child(r, 'rustle'), 700.0, 5000.0, 160.0, 0.1, 0.7)
    caws = np.zeros(1)
    for i in range(1 + int(r.integers(0, 2))):
        f0 = r.uniform(540.0, 620.0)
        caws = layers(caws, (_caw(child(r, 'caw', i), f0, r.uniform(0.18, 0.26), 0.7), r.uniform(0.05, 0.45),
                             -2.0 * i))
    x = layers((flock, 0.0, 0.0), (scrabble, 0.0, -14.0), (rustle, 0.0, -12.0), (normalize(caws), 0.0, 1.0))
    return _outdoors(x, r, -16.0, 1.2)


# --- Sparrows ---------------------------------------------------------------------------------------------------------

def _sparrow_note(r, kind, base):
    """One of a house sparrow's notes: 'cheep' (a quick rise and a fall), 'chirrup' (a cheep and a rolled, falling 'rup'),
    'chew' (a falling slur), 'tsip' (a thin, high tick)."""
    if kind == 'chirrup':
        a = _note(child(r, 'chi'), [(0.0, base * 0.8), (0.3, base * 1.3), (1.0, base * 1.1)], r.uniform(0.04, 0.055),
                  (1.0, 0.35, 0.12), 0.05)
        b = _note(child(r, 'rup'), [(0.0, base * 1.2), (0.4, base * 1.0), (1.0, base * 0.72)], r.uniform(0.06, 0.085),
                  (1.0, 0.3, 0.1), 0.06, am=(r.uniform(95.0, 130.0), 0.55))
        return layers((a, 0.0, 0.0), (b, a.size / SR + 0.008, -2.0))
    if kind == 'chew':
        return _note(r, [(0.0, base * 1.25), (0.3, base * 1.18), (1.0, base * 0.7)], r.uniform(0.06, 0.09),
                     (1.0, 0.3, 0.1), 0.05)
    if kind == 'tsip':
        return _note(r, [(0.0, base * 1.75), (1.0, base * 1.3)], r.uniform(0.028, 0.045), (1.0, 0.08), 0.03,
                     attack=0.003)
    return _note(r, [(0.0, base * 0.75), (0.22, base * 1.32), (1.0, base * 0.92)], r.uniform(0.07, 0.11),
                 (1.0, 0.35, 0.12), 0.05)


def _twitter(r, base, count):
    """A quick run of tiny notes, up and down by turns, a dozen a second and more."""
    out = np.zeros(1)
    t = 0.0
    for i in range(count):
        d = r.uniform(0.022, 0.034)
        hi, lo = base * r.uniform(1.25, 1.5), base * r.uniform(0.85, 1.0)
        pts = [(0.0, lo), (1.0, hi)] if i % 2 == 0 else [(0.0, hi), (1.0, lo)]
        out = layers(out, (_note(child(r, 't', i), pts, d, (1.0, 0.2, 0.05), 0.04, attack=0.003), t,
                           r.uniform(-4.0, 0.0)))
        t += d + r.uniform(0.012, 0.03)
    return out


@cue('World.Fauna.Sparrow.Chirp', variations=6, att='Creature', cls='Ambience', jitter=0.05, conc=3, level=-16.0)
def sparrow_chirp(v, r):
    # A few sparrows on a fence or a roof: two or three birds, each at its own pitch and distance, trading cheeps and
    # chirrups, a falling chew, a thin tsip, now and then a quick twittering run. Every take its own conversation.
    birds = []
    for i in range(2 + int(r.integers(0, 2))):
        birds.append((r.uniform(3000.0, 3900.0), -r.uniform(0.0, 7.0) * (i > 0), r.uniform(3.0, 20.0)))
    dur = r.uniform(1.4, 2.2)
    out = np.zeros(1)
    t = r.uniform(0.0, 0.02)
    k = 0
    while t < dur:
        base, gain, metres = birds[int(r.integers(0, len(birds)))]
        base *= jitter(r, 1, 0.04)
        if r.random() < 0.12:
            sig = _twitter(child(r, 'run', k), base * 1.1, int(r.integers(5, 9)))
        else:
            kind = str(r.choice(['cheep', 'cheep', 'chirrup', 'chirrup', 'chew', 'tsip']))
            sig = _sparrow_note(child(r, 'n', k), kind, base)
        out = layers(out, (_far(sig, metres), t, gain + r.uniform(-2.0, 0.0)))
        t += sig.size / SR + r.uniform(0.05, 0.28)
        k += 1
    out = F.filt(normalize(out), F.hp(1500.0), F.peak(3300.0, -2.0, 1.0), extend=False)
    return _outdoors(out, r, -21.0, 0.7)


@cue('World.Fauna.Sparrow.TakeOff', variations=4, att='Creature', cls='Ambience', jitter=0.05, conc=2, level=-15.0)
def sparrow_takeoff(v, r):
    # A small flock flushing: a flurry of quick little wingbeats (each bird a burst of twenty a second, the flock
    # together a soft 'frrrt' that thins as they scatter) and a sharp alarm cheep or two.
    flock = np.zeros(1)
    for i in range(int(r.integers(6, 11))):
        rr = child(r, 'bird', i)
        start = rr.uniform(0.0, 0.16)
        period = 1.0 / rr.uniform(17.0, 22.0)
        beats = int(rr.uniform(0.25, 0.5) / period)
        flock = layers(flock, (_flight(rr, 0.3, start, beats, period, period * 1.08, rr.uniform(0.35, 0.6), -14.0), 0.0,
                               rr.uniform(-6.0, 0.0)))
    flock = normalize(flock)
    n = flock.size
    flock = F.sweep(flock, 'lp', O.expsweep(11000.0, 4000.0, n / SR, 1.0, n=n), 0.7)
    alarm = np.zeros(1)
    for i in range(1 + int(r.integers(0, 2))):
        base = r.uniform(3300.0, 4000.0)
        alarm = layers(alarm, (_sparrow_note(child(r, 'alarm', i), 'cheep', base), r.uniform(0.0, 0.3), -3.0 * i))
    x = layers((flock, 0.0, 0.0), (F.filt(normalize(alarm), F.hp(1500.0), extend=False), 0.0, -7.0))
    return _outdoors(F.filt(x, F.hp(120.0), extend=False), r, -16.0, 0.9)


# --- Swallows ---------------------------------------------------------------------------------------------------------

@cue('World.Fauna.Swallow.Twitter', variations=5, att='Creature', cls='Ambience', jitter=0.04, conc=2, level=-15.0,
     swell=True)
def swallow_twitter(v, r):
    # A swallow swooping past: its liquid twitter (quick rising twits, warbling notes that gurgle, a dry zip), coming in
    # and going away, higher as it nears and lower as it leaves, brightest as it passes.
    dur = r.uniform(1.0, 1.4)
    tc = dur * r.uniform(0.4, 0.55)
    base = r.uniform(3200.0, 4200.0)

    def doppler(t):
        return 1.0 + 0.035 * np.tanh((tc - t) / 0.18)

    out = np.zeros(ns(dur) + ns(0.2))
    t = r.uniform(0.0, 0.05)
    k = 0
    while t < dur - 0.06:
        kind = str(r.choice(['twit', 'twit', 'warble', 'warble', 'gurgle', 'zip']))
        rr = child(r, 'n', k)
        s = doppler(t) * jitter(r, 1, 0.05)
        if kind == 'twit':
            sig = _note(rr, [(0.0, base * 0.8 * s), (1.0, base * 1.3 * s)], rr.uniform(0.03, 0.045), (1.0, 0.1, 0.03),
                        0.03)
        elif kind == 'warble':
            sig = _note(rr, [(0.0, base * s), (0.5, base * 1.15 * s), (1.0, base * 0.95 * s)], rr.uniform(0.06, 0.12),
                        (1.0, 0.12, 0.03), 0.03, fm=(rr.uniform(30.0, 45.0), rr.uniform(0.08, 0.14)))
        elif kind == 'gurgle':
            sig = _note(rr, [(0.0, base * 0.6 * s), (1.0, base * 0.7 * s)], rr.uniform(0.06, 0.1), (1.0, 0.35, 0.15),
                        0.05, fm=(rr.uniform(55.0, 75.0), rr.uniform(0.12, 0.18)))
        else:
            sig = _note(rr, [(0.0, base * 1.6 * s), (1.0, base * 1.0 * s)], rr.uniform(0.018, 0.026), (1.0, 0.1), 0.03,
                        attack=0.002)
        k0 = ns(t)
        out[k0:k0 + sig.size] += sig[:max(0, out.size - k0)] * float(db2a(r.uniform(-4.0, 0.0)))
        t += sig.size / SR + r.uniform(0.012, 0.05)
        k += 1
    n = out.size
    tt = times(n)
    near = 1.0 / (1.0 + ((tt - tc) / 0.22) ** 2)
    out = F.sweep(out * (0.06 + 0.94 * near), 'lp', 4500.0 + 8000.0 * near, 0.7)
    out = F.filt(normalize(out), F.hp(1500.0), F.peak(3300.0, -2.0, 1.0), extend=False)
    return _outdoors(out, r, -20.0, 0.8)


# --- The hawk ---------------------------------------------------------------------------------------------------------

@cue('World.Fauna.Hawk.Cry', variations=3, att='Gun', cls='Ambience', jitter=0.03, conc=1, level=-6.0, swell=True)
def hawk_cry(v, r):
    # A hawk's cry from high overhead: a thin, hoarse scream ('kee-eeeer') that rises a moment and falls away, a rasp of
    # breath torn into it, small and dulled with height, the land answering it softly.
    dur = r.uniform(1.2, 1.7)
    n = ns(dur)
    s = r.uniform(0.92, 1.06)
    f = _contour([(0.0, 2400.0 * s), (0.07, 3300.0 * s), (0.3, 3100.0 * s), (0.6, 2400.0 * s), (1.0, 1600.0 * s)], n)
    f = f * (1.0 + 0.006 * N.smooth_random(n, child(r, 'flutter'), 90.0))
    ph = 2.0 * np.pi * O.phase(f, n)
    tone = np.sin(ph) + 0.45 * np.sin(2.0 * ph + 0.4) + 0.2 * np.sin(3.0 * ph + 1.0) + 0.08 * np.sin(4.0 * ph)
    # The rasp kept close round the note, so it reads as a hoarse voice rather than a hiss beside it.
    rasp = F.sweep(N.white(n, child(r, 'rasp')), 'bp', np.clip(f, 500.0, 15000.0), 7.0)
    rasp = rasp + 0.5 * F.sweep(N.white(n, child(r, 'rasp2')), 'bp', np.clip(2.0 * f, 500.0, 15000.0), 7.0)
    hoarse = 1.0 - 0.35 * np.abs(N.smooth_random(n, child(r, 'hoarse'), 150.0))
    x = (tone / _rms(tone) + rasp / _rms(rasp) * 0.4) * hoarse * E.ar(dur, 0.06, dur * 0.5, 2.0)
    x = F.filt(x, F.hp(1000.0), F.lp(6500.0, 0.7), F.peak(3300.0, -3.0, 1.0), extend=False)
    return _outdoors(normalize(x), r, -9.0, 2.4, 3500.0, 0.5)


# --- Insects ----------------------------------------------------------------------------------------------------------

@cue('World.Fauna.Flies', variations=1, att='Near', cls='Ambience', jitter=0.0, conc=2, loop=True, level=-20.0,
     align=False)
def flies(v, r):
    # A cloud of flies over an outhouse or the den's larder: a few zipping close by (each a nasal buzz near 190 Hz that
    # swells as it comes near and fades as it goes, higher coming and lower going, now and then landing and going
    # quiet), more circling further in, and the faint hum of the rest.
    dur = 8.0
    L = ns(dur)

    def fly(rr, close):
        f0 = rr.uniform(165.0, 230.0)
        if close:
            near = (0.5 + 0.5 * N.smooth_random(L, child(rr, 'near'), 0.5, periodic=True)) ** 2.5
            # Coming in it's higher, going away lower: the pitch follows how fast it's closing.
            dn = np.roll(near, -1) - np.roll(near, 1)
            dn = dn / (np.max(np.abs(dn)) + 1e-12)
            amp = 0.03 + near
        else:
            dn = N.smooth_random(L, child(rr, 'zip'), 1.2, periodic=True)
            amp = 0.25 + 0.15 * N.smooth_random(L, child(rr, 'near'), 0.6, periodic=True)
        f = f0 * (1.0 + 0.05 * dn + 0.012 * N.smooth_random(L, child(rr, 'wob'), 7.0, periodic=True))
        # A whole number of wingbeats in the loop, so every fly carries on across the seam.
        cycles = np.sum(f) / SR
        f = f * (round(cycles) / cycles)
        buzz = O.additive(f, [1.0 / k ** 0.8 for k in range(1, 21)], n=L, phase0=rr.random())
        land = np.clip(N.smooth_random(L, child(rr, 'land'), 0.25, periodic=True) * 3.0 + 1.6, 0.0, 1.0)
        return buzz / _rms(buzz) * amp * land

    close = sum(fly(child(r, 'close', i), True) for i in range(4))
    far = sum(fly(child(r, 'far', i), False) for i in range(8))
    hum = N.shaped(L, child(r, 'hum'), -3.0, 160.0, 900.0, periodic=True)
    hum *= 0.7 + 0.3 * N.smooth_random(L, child(r, 'hg'), 3.0, periodic=True)
    x = close / _rms(close) + 0.2 * far / _rms(far) + 0.1 * hum
    return F.filt(x, F.hp(120.0), F.peak(650.0, 4.0, 0.8), F.lp(3500.0, 0.7), F.highshelf(2500.0, -4.0), circular=True)


@cue('World.Fauna.Dragonfly.Buzz', variations=4, att='Near', cls='Ambience', jitter=0.06, conc=2, level=-17.0,
     swell=True)
def dragonfly_buzz(v, r):
    # A dragonfly darting past close by: its four stiff wings clattering thirty-odd times a second, a dry papery rattle
    # with a soft flap under it, swelling as it passes and gone, the rate dropping a little as it leaves.
    dur = r.uniform(0.35, 0.6)
    n = ns(dur)
    rate = O.expsweep(r.uniform(36.0, 42.0), r.uniform(28.0, 33.0), dur, 1.0, n=n)
    ph = O.phase(rate, n)
    strokes = np.nonzero(np.diff(np.floor(ph)) > 0)[0] + 1
    out = np.zeros(n + ns(0.03))
    for j, k in enumerate(strokes):
        rr = child(r, 's', j)
        for wing, lag in enumerate((0.0, rr.uniform(0.003, 0.006))):
            m = ns(0.004)
            click = F.filt(N.white(m, child(rr, 'c', wing)) * E.decay(m, 0.0015), F.bp(rr.uniform(1800.0, 4200.0), 1.0),
                           extend=False)
            s = k + ns(lag)
            out[s:s + click.size] += click * rr.uniform(0.5, 1.0)
        m = ns(0.02)
        flap = F.filt(N.pink(m, child(rr, 'f')), F.bp(220.0, 1.0), extend=False) * E.perc(0.02, 0.002, 0.012)
        out[k:k + flap.size] += flap * 0.6
    tt = times(out.size)
    tc = dur * r.uniform(0.4, 0.6)
    near = 1.0 / (1.0 + ((tt - tc) / (dur * 0.2)) ** 2)
    out = F.sweep(out * near, 'lp', 2500.0 + 7000.0 * near, 0.7)
    out = F.filt(out, F.hp(150.0), F.peak(3300.0, -3.0, 1.0), extend=False)
    # In from nothing and gone to nothing: the pass's own fall isn't quite silence at the ends.
    return normalize(out * E.ar(out.size / SR, 0.03, out.size / SR * 0.4, 2.0)[:out.size], 0.0)


# --- Tumbleweeds, dust devils, washing ------------------------------------------------------------------------------

@cue('World.Fauna.Tumbleweed.Bounce', variations=5, att='Creature', cls='Ambience', jitter=0.08, conc=3, level=-15.0)
def tumbleweed_bounce(v, r):
    # A tumbleweed bouncing on hard ground: a light, hollow thump (a ball of dry twigs, all air), a crackle of tiny
    # stems snapping and flexing, a dry scrape as it rolls on, and a puff of dust.
    thump = kit.noise_thump(0.09, child(r, 'thump'), 700.0, 160.0, 0.035, 1.0, 0.03)

    def twig(rr, i):
        return kit.thunk(rr.uniform(1400.0, 3800.0), rr, rr.uniform(0.004, 0.01), 0.00008, 3, 0.1)

    twigs = normalize(G.rattle(0.16, child(r, 'twigs'), int(r.integers(14, 26)), 0.14, twig, 0.0, 0.85))
    d = r.uniform(0.3, 0.45)
    n = ns(d)
    scrape = N.band(n, child(r, 'scrape'), 900.0, 7000.0) * kit.turbulence(n, child(r, 'st'), 160.0, 0.85)
    scrape = normalize(scrape * E.ar(d, 0.02, d * 0.8, 2.0))
    snaps = kit.grit_burst(d, child(r, 'snaps'), 900.0, 1500.0, 6000.0, d * 0.6, 0.004)
    dust = kit.grit_burst(0.2, child(r, 'dust'), 3000.0, 2500.0, 8000.0, 0.1)
    x = layers((thump, 0.0, -4.0), (twigs, 0.0, 0.0), (scrape, 0.01, -8.0), (snaps, 0.01, -10.0), (dust, 0.0, -18.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 1.0), F.highshelf(8000.0, -3.0), extend=False)
    return _outdoors(x, r, -18.0, 0.8)


@cue('World.Fauna.DustDevil', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=2, loop=True,
     level=-13.0, align=False)
def dust_devil(v, r):
    # A dust devil spinning across the flats: a whirling, hissing gust (the hiss swinging up and down in pitch and
    # loudness as the column turns, a hollow note whirling in it), its low roar swelling and easing, sand and grit
    # rattling in it, a twig or a pebble ticking now and then.
    dur = 9.0
    L = ns(dur)
    t = times(L)
    turns = 12.0
    whirl = 0.5 + 0.5 * np.sin(2.0 * np.pi * turns / dur * t + 2.0 * np.pi * r.random())
    whirl = np.clip(whirl + 0.25 * N.smooth_random(L, child(r, 'wj'), 2.0, periodic=True), 0.0, 1.0)
    gust = 0.55 + 0.45 * N.smooth_random(L, child(r, 'gust'), 0.3, periodic=True)
    roar = N.shaped(L, child(r, 'roar'), -4.0, 40.0, 350.0, periodic=True) * (0.5 + 0.5 * gust)
    hiss = N.shaped(L, child(r, 'hiss'), -1.0, 900.0, 10000.0, periodic=True)
    fc = 1800.0 * 2.0 ** (1.2 * (whirl - 0.5))
    hiss = B.periodic(lambda y: F.sweep(y, 'bp', np.tile(fc, 3), 0.8), hiss) * (0.3 + 0.7 * whirl) * gust ** 1.5
    howl_src = N.shaped(L, child(r, 'howl'), -3.0, 150.0, 1500.0, periodic=True)
    howl = B.periodic(lambda y: F.sweep(y, 'bp', np.tile(380.0 * 2.0 ** (0.6 * (whirl - 0.5)), 3), 6.0), howl_src)
    howl *= gust
    grit = G.grit(dur, child(r, 'grit'), 700.0 + 3500.0 * gust * whirl, 1500.0, 7000.0, 2.0, 4, periodic=True)
    events = []
    rd = child(r, 'debris')
    for k, at in enumerate(N.times_poisson(dur, rd, 2.5)):
        tick = kit.thunk(rd.uniform(1200.0, 3200.0), child(rd, k), rd.uniform(0.006, 0.015), 0.0001, 3, 0.1)
        events.append((tick * rd.uniform(0.3, 1.0), at))
    debris = np.zeros(L + ns(1.0))
    for sig, at in events:
        k0 = ns(at)
        debris[k0:k0 + sig.size] += sig
    debris = B.fold(debris, L)
    x = (normalize(roar) * 0.5 + normalize(hiss) + normalize(howl) * 0.3 + normalize(grit) * 0.5
         + normalize(debris) * 0.25)
    return F.filt(x, F.hpn(35.0, 2), F.peak(3300.0, -3.0, 0.8), F.highshelf(8000.0, -4.0), circular=True)


@cue('World.Fauna.Cloth.Flap', variations=4, att='Near', cls='Ambience', jitter=0.06, conc=3, level=-15.0, swell=True)
def cloth_flap(v, r):
    # Washing on its line in a gust: the cloth fluttering up as the wind takes it, snapping taut once or twice with a
    # sharp crack, flapping on and settling as the gust passes; on one take the line creaks on its post.
    dur = r.uniform(0.7, 1.1)
    n = ns(dur)
    gust = E.bp([(0.0, 0.0), (dur * 0.3, 1.0), (dur * 0.6, 0.8), (dur, 0.0)], n=n, curve='cos')
    rate = r.uniform(14.0, 20.0) * (1.0 + 0.3 * N.smooth_random(n, child(r, 'rate'), 3.0)) * (0.7 + 0.5 * gust)
    flap = (0.5 - 0.5 * np.cos(2.0 * np.pi * O.phase(rate, n))) ** 3
    cloth = N.shaped(n, child(r, 'cloth'), -3.0, 250.0, 4500.0) * (0.25 + 0.75 * flap) * gust
    cloth = normalize(cloth * kit.turbulence(n, child(r, 'turb'), 120.0, 0.4))
    parts = [(cloth, 0.0, -3.0)]
    for i in range(1 + int(r.integers(0, 2))):
        crack = kit.burst(0.015, child(r, 'crack', i), 0.008, lo=400.0, hi=7000.0)
        body = kit.noise_thump(0.05, child(r, 'body', i), 1200.0, 250.0, 0.025, 1.0, 0.02)
        parts.append((layers((crack, 0.0, 0.0), (body, 0.0, -5.0)), r.uniform(0.28, 0.75) * dur, -2.0 * i))
    if v == 2:
        parts.append((kit.creak(0.3, child(r, 'line'), 25.0, 40.0, (600.0, 1400.0, 2500.0), 10.0, 0.3), dur * 0.35,
                      -16.0))
    x = F.filt(layers(*parts), F.peak(3300.0, -3.0, 1.0), F.hp(120.0), extend=False)
    return _outdoors(x, r, -18.0, 0.7)
