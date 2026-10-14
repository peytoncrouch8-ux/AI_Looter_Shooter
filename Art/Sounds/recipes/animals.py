"""The small animals of Ransom's Rest's town (3D): the hens in the yards, the rooster, the barn cat. All cues are
Creature.Chicken.* and Creature.Cat.* (their names: the Animals section of Audio/LooterSoundCues.h).

A bird's voice is the syrinx's, but a chicken's calls are close enough to a small throat's that they are made like the
creatures' voices (lib/voice.py: a harmonic source shaped by moving vocal-tract resonances), with the tract scaled small
(shift > 1) and TWO sources a hair apart, one for each of the syrinx's two membranes, whose beating gives the calls their
reedy, rough edge. A cat is the same idea in a mammal's throat: one source, a nasal 'm' opening into a vowel glide.

- Creature.Chicken.Cluck: a hen's contented 'buk-buk-buk'. Short pulses, each a scoop up into a throaty vowel and a quick
  fall, stopped abruptly (the 'k'), a soft crop-thump on the front of each; low and kept off the 2-5 kHz band, since a yard
  of hens clucks all day. Four hens' takes (two to four pulses, one with a drawn-out last 'bukaak').
- Creature.Chicken.Squawk: the yard scattering. Ragged alarm squawks (pressed, driven, the pitch snapping up and falling
  broken) over a flurry of frantic wingbeats from two or three hens, their feathers' rustle, feet scrabbling in dirt.
- Creature.Chicken.Crow: the rooster, about two seconds, in the classic four parts: a short 'cock', a higher 'a', a long
  climbing 'doodle' and a long falling 'doo' whose voice breaks at the end. Bright, carrying; the valley and the false
  fronts throw it back.
- Creature.Cat.Meow: a barn cat's meow. 'mi-aou' (a hum opening to a vowel, rising and falling), a short questioning
  'mew', and a rolled 'mrow'.
- Creature.Cat.Hiss: a cat's hiss at something coming at it fast: a spat 'pft', a breathy noise burst through the teeth
  (the resonance sinking as the mouth narrows) over a rough throat growl, ~0.6 s.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, dist as D, reverb as R, voice as V
from recipes import cue
from recipes import kit
from recipes.fauna import _outdoors, _flight, _contour


# --- Shared ---------------------------------------------------------------------------------------------------------

def _two_sources(f, keys, r, ratio=1.012, second=0.55, **kw):
    """The syrinx's two membranes: the voice twice, the second a hair sharp and a little quieter, so they beat."""
    a = V.voice(f, keys, child(r, 'a'), **kw)
    b = V.voice(f * ratio, keys, child(r, 'b'), **kw)
    return normalize(a) + second * normalize(b)


def _town(x, r, rel_db=-12.0):
    """A call thrown across the town: slap-backs off the false fronts and a rolling tail off the ridges."""
    ir = R.outdoor(child(r, 'ir'), dur=1.9, slaps=((0.14, -8.0), (0.33, -12.0), (0.62, -17.0)), slap_lp=3000.0,
                   tail_t60=1.0, tail_db=-12.0, tail_start=0.05, tail_peak=0.25, tail_lp=2200.0)
    wet = F.convolve(F.filt(x, F.hp(250.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.05))


# --- Hens: the cluck ------------------------------------------------------------------------------------------------

def _cluck_pulse(r, f0, dur, shift=1.12):
    """One 'buk': the pitch scooping up into the note and falling as the throat closes, a throaty 'uh-o-uh', a quick
    rise, a round middle and an abrupt stop."""
    n = ns(dur)
    f = _contour([(0.0, f0 * 0.78), (0.2, f0 * 1.08), (0.55, f0 * 0.95), (1.0, f0 * 0.72)], n)
    keys = [(0.0, 'uh'), (dur * 0.45, 'o'), (dur, 'uh')]
    x = _two_sources(f, keys, r, breath=0.28, jitter=0.05, shimmer=0.2, tilt=1.5, shift=shift, rough=0.4, top=6000.0)
    # A hen's call is chaotic, not a clean tone: a band of noise rides on the voice where the tract's formants have
    # let it die away (the vocal tract's cascade falls fast above 2 kHz), so the 'buk' has its rattle and its edge.
    x = normalize(x) + 0.3 * normalize(N.band(n, child(r, 'chaos'), 1200.0, 4200.0))
    # Throaty: the vocal-fry rattle of an irregular, slow glottal closure (about 65 pulses a second) chops the voice.
    rate = 65.0 * (1.0 + 0.25 * N.smooth_random(n, child(r, 'fry'), 12.0))
    x = x * (1.0 - 0.35 * (0.5 + 0.5 * np.sin(2.0 * np.pi * O.phase(rate, n) + 6.28 * r.random())))
    env = E.bp([(0.0, 0.0), (0.005, 0.7), (dur * 0.28, 1.0), (dur * 0.7, 0.5), (dur - 0.014, 0.15), (dur, 0.0)], n=n,
               curve='cos')
    return normalize(x * env)


# (start, duration, gain dB, pitch factor) of each pulse, by take.
CLUCKS = [
    [(0.0, 0.09, 0.0, 1.0), (0.2, 0.085, -2.0, 0.97), (0.41, 0.095, -3.0, 0.94)],
    [(0.0, 0.1, 0.0, 1.0), (0.24, 0.08, -3.0, 0.9)],
    [(0.0, 0.08, 0.0, 1.0), (0.17, 0.08, -1.0, 1.02), (0.32, 0.08, -1.0, 1.04), (0.46, 0.17, -2.0, 0.95)],
    [(0.0, 0.09, 0.0, 1.0), (0.15, 0.09, -1.0, 0.96), (0.42, 0.11, -2.0, 0.9)],
]


@cue('Creature.Chicken.Cluck', variations=4, att='Near', cls='Ambience', jitter=0.05, conc=2, level=-18.0)
def chicken_cluck(v, r):
    # A hen scratching about the yard, contented: a few low, throaty 'buk's, a soft thump of the crop on the front of
    # each and the little tick of the throat closing on the end. Each take is another hen (a bigger, older one on the
    # fourth).
    f0 = [440.0, 500.0, 470.0, 395.0][v] * jitter(r, 1, 0.02)
    out = np.zeros(1)
    for k, (t, d, db, pf) in enumerate(CLUCKS[v]):
        rr = child(r, 'pulse', k)
        d = d * jitter(rr, 1, 0.06)
        t = t * jitter(rr, 1, 0.04) if k else 0.0
        pulse = _cluck_pulse(rr, f0 * pf * jitter(rr, 1, 0.025), d)
        thump = kit.noise_thump(0.035, child(rr, 'thump'), 520.0, 180.0, 0.02, 1.0, 0.015)
        tick = kit.burst(0.012, child(rr, 'tick'), 0.005, lo=1500.0, hi=4200.0)
        out = layers(out, (pulse, t, db), (thump, t, db - 13.0), (tick, t + d - 0.016, db - 21.0))
    out = F.filt(out, F.hp(170.0), F.peak(900.0, 3.0, 0.9), F.peak(2200.0, 3.0, 0.9), F.lp(5000.0, 0.7), extend=False)
    return _outdoors(out, r, -23.0, 0.45, 3500.0, 0.8)


# --- Hens: the squawk -----------------------------------------------------------------------------------------------

def _squawk(r, f0, dur, harsh):
    """One alarm squawk: the pitch snapping up and falling, ragged where the syrinx slaps and breaks, pressed, driven."""
    n = ns(dur)
    f = _contour([(0.0, f0 * 0.8), (0.12, f0 * 1.25), (0.4, f0 * 1.1), (1.0, f0 * 0.7)], n)
    f = f * (1.0 + 0.04 * N.smooth_random(n, child(r, 'wob'), 35.0))
    keys = [(0.0, 'ae'), (dur * 0.35, 'a'), (dur, 'uh')]
    x = _two_sources(f, keys, r, ratio=1.02, second=0.6, breath=0.35, jitter=0.05, shimmer=0.3, tilt=3.0, shift=1.3,
                     rough=harsh, top=8000.0)
    rasp = 1.0 - 0.5 * np.clip(N.smooth_random(n, child(r, 'rasp'), 120.0), 0.0, 1.0)
    x = D.drive(normalize(x * rasp), 9.0, 'tanh', bias=0.1)
    env = E.bp([(0.0, 0.0), (0.008, 1.0), (dur * 0.5, 0.85), (dur - 0.02, 0.4), (dur, 0.0)], n=n, curve='cos')
    return normalize(F.filt(x * env, F.hp(350.0), F.peak(1300.0, 3.0, 1.0), F.highshelf(2200.0, 4.0), F.lp(8000.0),
                            extend=False))


# (squawk times, hens flapping) by take.
SCATTER = [
    ([0.0, 0.34], 2),
    ([0.0, 0.26, 0.55], 2),
    ([0.0], 3),
    ([0.0, 0.2, 0.46], 3),
]


@cue('Creature.Chicken.Squawk', variations=4, att='Creature', cls='Effects', jitter=0.05, conc=2, level=-12.0)
def chicken_squawk(v, r):
    # The yard scattering as something comes at it: a hen's ragged alarm squawk (the next hens answering, lower), and
    # over it all a flurry of wings, two or three hens flapping and bolting, feathers rustling, feet scrabbling dirt.
    times_, hens = SCATTER[v]
    f_base = r.uniform(650.0, 780.0)
    squawks = np.zeros(1)
    for k, t in enumerate(times_):
        rr = child(r, 'sq', k)
        d = rr.uniform(0.3, 0.42) if k == 0 else rr.uniform(0.2, 0.32)
        s = _squawk(rr, f_base * (1.0 - 0.06 * k) * jitter(rr, 1, 0.05), d, rr.uniform(0.4, 0.8))
        squawks = layers(squawks, (s, t + (rr.uniform(0.0, 0.03) if k else 0.0), -1.5 * k))
    flurry = np.zeros(1)
    for h in range(hens):
        rr = child(r, 'hen', h)
        p0 = rr.uniform(0.075, 0.09)
        w = _flight(rr, rr.uniform(0.55, 0.7), 0.0, int(rr.integers(9, 14)), p0, p0 * rr.uniform(1.2, 1.35),
                    rr.uniform(0.55, 0.8), -8.0)
        flurry = layers(flurry, (normalize(w), rr.uniform(0.0, 0.1) if h else 0.02, rr.uniform(-4.0, 0.0)))
    flurry = normalize(flurry)
    feathers = kit.cloth(0.9, child(r, 'feathers'), 700.0, 5500.0, 170.0, 0.12, 0.7)
    scrabble = kit.grit_burst(0.55, child(r, 'scrabble'), 900.0, 800.0, 3500.0, 0.3)
    x = layers((squawks, 0.0, 0.0), (flurry, 0.0, -6.0), (feathers, 0.02, -12.0), (scrabble, 0.0, -16.0))
    x = F.filt(x, F.hp(120.0), F.peak(3300.0, -1.0, 1.0), extend=False)
    return _outdoors(x, r, -17.0, 0.9, 3800.0, 0.8)


# --- The rooster ----------------------------------------------------------------------------------------------------

def _crow_part(r, pts, dur, keys, gain_db=0.0, rough=0.1, attack=0.008, release=0.04, vib=None, swell=0.0, tilt=3.0):
    """One part of the crow: a bright harmonic voice (two membranes, a small throat) on a pitch curve through pts
    (fractions of the part, Hz), the pitch's own tremble on it and a little raggedness in its loudness. vib (Hz, depth)
    adds a waver; swell (s) a slower rise."""
    n = ns(dur)
    f = _contour(pts, n) * (1.0 + 0.008 * N.smooth_random(n, child(r, 'trem'), 55.0))
    if vib:
        f = f * (1.0 + vib[1] * np.sin(2.0 * np.pi * vib[0] * times(n) + 2.0 * np.pi * r.random()))
    x = _two_sources(f, keys, r, breath=0.1, jitter=0.018, shimmer=0.12, tilt=tilt, shift=1.4, rough=rough, top=8000.0)
    x = x * (1.0 - 0.12 * np.clip(N.smooth_random(n, child(r, 'rag'), 110.0), 0.0, 1.0))
    env = E.bp([(0.0, 0.0), (attack, 1.0 if swell == 0.0 else 0.6), (max(swell, attack * 2.0), 1.0),
                (dur - release, 0.8), (dur, 0.0)], n=n, curve='cos')
    return normalize(x * env), gain_db


@cue('Creature.Chicken.Crow', variations=3, att='Gun', cls='Ambience', jitter=0.03, conc=1, level=-9.0)
def chicken_crow(v, r):
    # The rooster on the fence at first light, the classic four parts: 'cock' (short, low, a hard start), 'a' (higher),
    # a long, climbing, wavering 'doodle', and a long falling 'doo' that loses its voice at the end and breaks. Take 3
    # is an older, lower bird that takes his time.
    b = [620.0, 660.0, 560.0][v] * jitter(r, 1, 0.02)
    sc = [1.0, 0.94, 1.1][v]
    d1, d2, d3, d4 = 0.17 * sc, 0.2 * sc, 0.6 * sc, 0.64 * sc
    p1 = _crow_part(child(r, 'p1'), [(0.0, 0.85 * b), (0.5, 1.0 * b), (1.0, 1.05 * b)], d1, [(0.0, 'uh'), (d1, 'a')],
                    -4.0, 0.15, 0.006, 0.03)
    p2 = _crow_part(child(r, 'p2'), [(0.0, 1.05 * b), (0.4, 1.3 * b), (1.0, 1.4 * b)], d2, [(0.0, 'a'), (d2, 'ae')],
                    -2.0, 0.1, 0.006, 0.03)
    p3 = _crow_part(child(r, 'p3'), [(0.0, 1.25 * b), (0.15, 1.5 * b), (0.6, 1.72 * b), (1.0, 1.55 * b)], d3,
                    [(0.0, 'a'), (d3 * 0.5, 'ae'), (d3, 'a')], 0.0, 0.1, 0.012, 0.05, (r.uniform(7.0, 9.5), 0.014),
                    0.06)
    # The last part begins sound and ends hoarse: its sound voice gives way to a rough, driven, ragged one.
    n4 = ns(d4)
    rr4 = child(r, 'p4')
    pts4 = [(0.0, 1.5 * b), (0.3, 1.45 * b), (0.7, 1.1 * b), (1.0, 0.72 * b)]
    keys4 = [(0.0, 'a'), (d4 * 0.5, 'o'), (d4, 'uh')]
    clean, _ = _crow_part(child(rr4, 'clean'), pts4, d4, keys4, 0.0, 0.1, 0.01, 0.07, (7.5, 0.01))
    hoarse, _ = _crow_part(child(rr4, 'hoarse'), pts4, d4, keys4, 0.0, 0.95, 0.01, 0.07, (7.5, 0.01), 0.0, 4.0)
    hoarse = D.drive(hoarse, 8.0, 'tanh', bias=0.1)
    mix = E.bp([(0.0, 0.0), (d4 * 0.5, 0.0), (d4 * 0.9, 1.0), (d4, 1.0)], n=n4, curve='cos')
    rag = 1.0 - 0.4 * mix * np.clip(N.smooth_random(n4, child(rr4, 'rag'), 140.0), 0.0, 1.0)
    p4 = normalize((normalize(clean) * (1.0 - mix) + normalize(hoarse) * mix) * rag
                   * E.bp([(0.0, 0.0), (0.01, 1.0), (d4 - 0.07, 0.8), (d4, 0.0)], n=n4, curve='cos'))
    t = 0.0
    out = np.zeros(1)
    for part, gap in ((p1, 0.05), (p2, 0.055), (p3, 0.05), (p4, 0.0)):
        sig, db = part if isinstance(part, tuple) else (part, -1.0)
        out = layers(out, (sig, t, db))
        t += sig.size / SR + gap
    # The syrinx's own harmonics carry further up than the vocal-tract model lets a voice go: lift the top back.
    out = F.filt(out, F.hp(300.0), F.peak(1500.0, 2.0, 1.0), F.highshelf(2000.0, 4.0), F.lp(8000.0, 0.7),
                 extend=False)
    return _town(normalize(out), r, -13.0)


# --- The barn cat ---------------------------------------------------------------------------------------------------

def _meow(r, f0, dur, kind):
    """A cat's voice: a nasal hum opening into a vowel (a lowpass sweeping open on the first 0.09 s), the pitch rising
    and falling. kind: 'meow' (mi-aou), 'mew' (short, questioning), 'mrow' (with a rolled r)."""
    n = ns(dur)
    if kind == 'mew':
        pts = [(0.0, 0.92 * f0), (0.35, 1.3 * f0), (1.0, 1.12 * f0)]
        keys = [(0.0, 'i'), (dur * 0.4, 'e'), (dur, 'ae')]
        env = E.bp([(0.0, 0.0), (0.012, 0.7), (dur * 0.3, 1.0), (dur * 0.7, 0.8), (dur, 0.0)], n=n, curve='cos')
        rough = 0.1
    elif kind == 'mrow':
        pts = [(0.0, 0.85 * f0), (0.25, 1.1 * f0), (0.55, 1.0 * f0), (1.0, 0.7 * f0)]
        keys = [(0.0, 'u'), (dur * 0.2, 'o'), (dur * 0.55, 'a'), (dur, 'oo')]
        env = E.bp([(0.0, 0.0), (0.02, 0.6), (dur * 0.25, 1.0), (dur * 0.8, 0.8), (dur, 0.0)], n=n, curve='cos')
        rough = 0.35
    else:
        pts = [(0.0, 0.78 * f0), (0.16, 0.95 * f0), (0.42, 1.38 * f0), (0.7, 1.18 * f0), (1.0, 0.8 * f0)]
        keys = [(0.0, 'u'), (dur * 0.12, 'i'), (dur * 0.4, 'ae'), (dur * 0.68, 'a'), (dur, 'oo')]
        env = E.bp([(0.0, 0.0), (0.02, 0.55), (dur * 0.14, 0.8), (dur * 0.45, 1.0), (dur * 0.8, 0.7), (dur, 0.0)], n=n,
                   curve='cos')
        rough = 0.15
    f = _contour(pts, n) * (1.0 + 0.01 * N.smooth_random(n, child(r, 'trem'), 45.0))
    x = V.voice(f, keys, child(r, 'voice'), breath=0.12, jitter=0.012, shimmer=0.2, tilt=-1.0, shift=1.55, rough=rough,
                top=6500.0)
    if kind == 'mrow':
        # The rolled 'r': the voice chopped by the throat's flutter, deepening toward the end.
        t = times(n)
        depth = 0.55 * np.clip((t - dur * 0.35) / (dur * 0.2), 0.0, 1.0)
        x = x * (1.0 - depth * (0.5 + 0.5 * np.sin(2.0 * np.pi * 27.0 * t)))
    cut = np.full(n, 9000.0)
    k = min(n, ns(0.09))
    cut[:k] = np.geomspace(700.0, 9000.0, k)
    x = F.sweep(x, 'lp', cut, 0.8)
    return normalize(x * env)


@cue('Creature.Cat.Meow', variations=3, att='Near', cls='Ambience', jitter=0.05, conc=1, level=-17.0)
def cat_meow(v, r):
    # A barn cat asking for something, now and then: 'mi-aou' (a hum opening to 'ee' then 'ah', up and down), a short
    # questioning 'mew', a rolled 'mrow'. The barn's wall gives it back softly.
    kind, dur, f0 = [('meow', 0.5, 560.0), ('mew', 0.3, 620.0), ('mrow', 0.46, 500.0)][v]
    x = _meow(r, f0 * jitter(r, 1, 0.03), dur * jitter(r, 1, 0.04), kind)
    x = F.filt(x, F.hp(200.0), F.peak(3300.0, -3.0, 0.8), F.lp(6500.0, 0.7), extend=False)
    return _outdoors(x, r, -21.0, 0.6, 3500.0, 0.8)


@cue('Creature.Cat.Hiss', variations=2, att='Near', cls='Effects', jitter=0.04, conc=1, level=-13.0)
def cat_hiss(v, r):
    # A cat's hiss at something running up to it: a spat 'pft' to start, then breath forced through its teeth (three
    # resonances sinking as its mouth narrows, flickering as the air tears), a rough throat growl under it. The second
    # take is two hisses in quick succession.
    dur = [0.52, 0.46][v] * jitter(r, 1, 0.04)
    n = ns(dur)
    t = times(n)
    fall = 1.0 - 0.18 * (t / dur)
    cents = (2300.0, 3700.0, 5600.0)
    levels = (0.7, 1.0, 0.8)
    qs = (2.0, 2.2, 1.8)
    air = np.zeros(n)
    for c, q, lv in zip(cents, qs, levels):
        air += lv * F.sweep(N.white(n, child(r, 'air', int(c))), 'bp', c * fall, q)
    air = F.filt(air, F.hp(1200.0, 0.7), extend=False)
    air = normalize(air * kit.turbulence(n, child(r, 'turb'), 90.0, 0.35))
    env = E.bp([(0.0, 0.0), (0.012, 1.0), (0.09, 0.75), (dur * 0.55, 0.65), (dur, 0.0)], n=n, curve='cos')
    if v == 1:
        # Two bursts: the first cut short, the second the longer.
        env = env * (1.0 - 0.8 * np.exp(-((t - dur * 0.28) / 0.03) ** 2))
    air = air * env
    f = O.glide([(0.0, 105.0), (dur * 0.5, 95.0), (dur, 70.0)], n=n) * (1.0 + 0.1 * N.smooth_random(n, child(r, 'fj'), 7.0))
    growl = V.voice(f, [(0.0, 'uh'), (dur * 0.5, 'o'), (dur, 'uh')], child(r, 'growl'), breath=0.6, jitter=0.12,
                    shimmer=0.4, tilt=1.0, shift=1.3, rough=0.9, top=3500.0)
    growl = F.filt(normalize(growl), F.hp(150.0), F.lp(2600.0, 0.7), extend=False)
    growl = growl * E.bp([(0.0, 0.0), (0.03, 0.8), (dur * 0.4, 1.0), (dur * 0.8, 0.5), (dur, 0.0)], n=n, curve='cos')
    spit = kit.burst(0.014, child(r, 'spit'), 0.007, lo=2000.0, hi=6500.0)
    x = layers((air, 0.0, 0.0), (normalize(growl), 0.0, -8.0), (spit, 0.0, -6.0))
    x = F.filt(x, F.hp(150.0), F.peak(3300.0, -2.0, 0.8), F.highshelf(8000.0, -3.0), extend=False)
    return _outdoors(x, r, -22.0, 0.35, 3500.0, 0.8)
