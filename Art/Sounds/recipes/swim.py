"""Swimming (round 4, the user's pick 2026-10-27): going into deep water and climbing out, the stroke at the surface and
under it, the gasp on surfacing short of breath, and the world with the head under: the pressed, dark hum, the gulp of the
water closing over and the air coming back, a breath let out as bubbles past the ears. The game plays them by the names in
Player/UnderwaterView.h (SwimCue); under water the world's own sounds are muffled by SMX_Underwater (a low-pass), so the
2D underwater cues here are made dark and close already.

Water is noise broken up by turbulence, little rising sine "bubbles" (kit.bubble, the physical bubble's ring), and slow
sloshing swells; the bigger the body, the lower the cavity's bloop and the longer the crown's rush.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, voice as V
from recipes import cue
from recipes import kit


def _water(dur, r, lo, hi, rate=110.0, depth=0.75, slope=-3.0, periodic=False):
    """Moving water: shaped noise in a band, broken by a fast irregular flutter."""
    n = ns(dur)
    x = N.shaped(n, r, slope, lo, hi, periodic=periodic)
    return x * kit.turbulence(n, child(r, 'turb'), rate, depth)


def _slosh(dur, r, lo, hi, rate0, rate1):
    """Water surging and settling back: low wet noise rising and falling in slow waves."""
    n = ns(dur)
    x = N.band(n, r, lo, hi) * kit.turbulence(n, child(r, 'turb'), 90.0, 0.6)
    waves = 0.5 - 0.5 * np.cos(2.0 * np.pi * O.phase(O.expsweep(rate0, rate1, dur, 1.0, n=n), n))
    return normalize(x * (0.3 + 0.7 * waves) * E.perc(dur, 0.012, dur * 0.45), 0.0)


def _bubbles(dur, r, count, lo, hi, start=0.0, spread=None, fall_db=0.0):
    """A run of bubbles over dur (their pitch lo..hi Hz), each a little softer than the first by up to fall_db."""
    out = np.zeros(1)
    span = spread if spread is not None else dur - start
    for i in range(count):
        at = start + r.uniform(0.0, max(span, 1e-3))
        out = layers(out, (kit.bubble(r.uniform(lo, hi), r, r.uniform(0.08, 0.16)), at,
                           r.uniform(-8.0, 0.0) - fall_db * at / max(dur, 1e-3)))
    return out


def _drops(dur, r, rate0, decay, lo=700.0, hi=2200.0, start=0.0):
    """Drops pattering back onto the water, thinning out (rate0 a second at first, falling over decay s)."""
    out = np.zeros(1)
    for t in N.times_poisson(dur, r, lambda tt: rate0 * np.exp(-tt / decay), start):
        out = layers(out, (kit.bubble(r.uniform(lo, hi), r, 0.12), t, r.uniform(-8.0, 0.0) - 30.0 * t / max(dur, 1e-3)))
    return out


def _wrap(x, n):
    """x folded round a loop of n samples (its tail laid back over the start), for loops built from one-shots."""
    out = np.zeros(n)
    for start in range(0, x.size, n):
        piece = x[start:start + n]
        out[:piece.size] += piece
    return out


def _breath(r, dur, keys, attack, release, shift=1.05):
    """A breath through the mouth (no voice): a whisper of the vowels, shaped."""
    x = V.whisper(dur, keys, r, shift) * E.ar(dur, attack, release)
    return normalize(F.filt(x, F.hp(200.0), F.lp(6500.0), extend=False))


# --- In and out ---------------------------------------------------------------------------------------------------------

@cue('Player.Swim.In', variations=3, att='Near', jitter=0.05, conc=2, level=-9.0)
def swim_in(v, r):
    # A body plunging into deep water (the game plays it louder the harder the fall): the slap of the surface, the whump of
    # the body going through it, the cavity it tears open closing as a deep bloop, a cloud of bubbles, the crown of water
    # thrown up rushing and falling back, drops pattering, the slosh settling. Each take a little bigger.
    size = (0.95, 1.08, 1.2)[v] * jitter(r, 1.0, 0.05)
    slap = kit.burst(0.07, child(r, 'slap'), 0.025, lo=280.0, hi=4200.0, color=-3.0, attack=0.001)
    whump = kit.noise_thump(0.4, child(r, 'whump'), 480.0 / size, 55.0, 0.2, 1.0, 0.05, 0.001)
    bloop = normalize(O.sine(O.expsweep(85.0 / size, 260.0 / size, 0.2, 0.8)) * E.perc(0.2, 0.004, 0.14), 0.0)
    hit = layers((slap, 0.0, -3.0), (whump, 0.0, 0.0), (bloop, 0.02, -5.0))
    cloud = _bubbles(0.55, child(r, 'cloud'), 9 + int(r.integers(0, 5)), 280.0 / size, 1100.0, 0.02, 0.4, 10.0)
    cd = 0.55 * size
    crown = _water(cd, child(r, 'crown'), 300.0, 4800.0, 120.0, 0.8) * E.perc(cd, 0.03, cd * 0.6)
    crown = F.filt(crown, F.lp(3600.0, 0.7), extend=False)
    drops = _drops(1.1, child(r, 'drops'), 45.0, 0.25, start=0.25)
    wash = _slosh(1.0, child(r, 'wash'), 110.0 / size, 900.0, 5.0, 2.0)
    x = layers((hit, 0.0, 0.0), (kit.set_level(cloud, hit, -9.0, 0.01), 0.02, 0.0),
               (kit.set_level(crown, hit, -5.0, 0.01), 0.03, 0.0), (kit.set_level(drops, hit, -16.0, 0.01), 0.0, 0.0),
               (kit.set_level(wash, hit, -10.0, 0.01), 0.08, 0.0))
    return F.filt(x, F.peak(3200.0, -2.5, 0.8), F.highshelf(5500.0, -4.0), F.hp(38.0), extend=False)


@cue('Player.Swim.Out', variations=3, att='Near', jitter=0.05, conc=2, level=-13.0)
def swim_out(v, r):
    # Hauling out of the water: a surge as the body lifts, water pouring off the coat in streams, the wet cloth slapping,
    # then the drips running off and thinning.
    surge = _slosh(0.45, child(r, 'surge'), 140.0, 1300.0, 6.0, 3.0)
    pour = normalize(_water(1.0, child(r, 'pour'), 600.0, 6000.0, 160.0, 0.85, -2.0) * E.ar(1.0, 0.06, 0.85), 0.0)
    pour = F.filt(pour, F.lp(5000.0, 0.7), extend=False)
    cloth = kit.cloth(0.35, child(r, 'cloth'), 180.0, 2200.0, 120.0, 0.15, 0.6)
    drips = _drops(1.4, child(r, 'drips'), 30.0, 0.5, 900.0, 2600.0, 0.3)
    x = layers((surge, 0.0, 0.0), (kit.set_level(pour, surge, -6.0, 0.01), 0.05, 0.0),
               (kit.set_level(cloth, surge, -10.0, 0.01), 0.08, 0.0), (kit.set_level(drips, surge, -14.0, 0.01), 0.0, 0.0))
    return F.filt(x, F.peak(3300.0, -2.5, 0.8), F.highshelf(6000.0, -4.0), F.hp(45.0), extend=False)


# --- The stroke ---------------------------------------------------------------------------------------------------------

@cue('Player.Swim.Stroke', variations=6, att='Near', jitter=0.06, conc=3, level=-17.0)
def swim_stroke(v, r):
    # A stroke at the surface (every stroke of a swim, so soft and never the same): the hands sweeping the water apart (a
    # wet swish), the slosh round the shoulders, a bubble or two, a few drips off the hands coming round.
    dur = 0.65
    n = ns(dur)
    swish = _water(dur, child(r, 'swish'), 200.0, 3000.0, 140.0, 0.7, -3.0)
    fc = O.expsweep(jitter(r, 380.0, 0.15), jitter(r, 1100.0, 0.15), dur, 1.0, n=n)
    swish = F.sweep(swish, 'bp', fc, 0.8) * E.ar(dur, dur * 0.3, dur * 0.6)
    slosh = _slosh(0.6, child(r, 'slosh'), 120.0, 800.0, 4.0, 2.5)
    pops = _bubbles(0.4, child(r, 'pops'), 1 + int(r.integers(0, 3)), 600.0, 1500.0, 0.1)
    drips = _drops(0.6, child(r, 'drips'), 18.0, 0.3, start=0.25)
    x = layers((normalize(swish, 0.0), 0.0, 0.0), (kit.set_level(slosh, swish, -4.0, 0.01), 0.05, 0.0),
               (kit.set_level(pops, swish, -12.0, 0.01), 0.0, 0.0), (kit.set_level(drips, swish, -16.0, 0.01), 0.0, 0.0))
    return F.filt(x, F.peak(3000.0, -3.0, 0.8), F.highshelf(5000.0, -5.0), F.hp(60.0), extend=False)


@cue('Player.Swim.StrokeUnder', variations=4, space='2D', cls='Effects', jitter=0.05, conc=2, level=-20.0)
def swim_stroke_under(v, r):
    # A stroke with the head under: the water rushing past the ears, low and close, a soft burble of air off the hands.
    dur = 0.75
    rush = kit.whoosh(dur, child(r, 'rush'), jitter(r, 110.0, 0.1), jitter(r, 420.0, 0.1), 0.7, 0.4, -4.0)
    burble = _bubbles(0.5, child(r, 'burble'), 3 + int(r.integers(0, 3)), 250.0, 700.0, 0.12)
    x = layers((rush, 0.0, 0.0), (kit.set_level(burble, rush, -10.0, 0.01), 0.0, 0.0))
    return F.filt(x, F.lp(1600.0, 0.7), F.hp(35.0), extend=False)


@cue('Player.Swim.Gasp', variations=3, space='2D', cls='Effects', jitter=0.03, conc=1, level=-13.0)
def swim_gasp(v, r):
    # Surfacing short of breath: a breath dragged in hard through the mouth, water spluttered off the lips, then a short
    # breath out.
    shift = (1.0, 1.06, 0.95)[v]
    gasp = _breath(child(r, 'in'), 0.38, [(0.0, 'h'), (0.06, 'a'), (0.3, 'uh'), (0.38, 'h')], 0.04, 0.2, shift)
    splutter = kit.burst(0.05, child(r, 'splutter'), 0.03, lo=1500.0, hi=7000.0, color=-2.0, attack=0.002)
    out = _breath(child(r, 'out'), 0.3, [(0.0, 'uh'), (0.3, 'h')], 0.02, 0.22, shift)
    return layers((splutter, 0.0, -10.0), (gasp, 0.01, 0.0), (out, 0.5, -7.0))


# --- Under water --------------------------------------------------------------------------------------------------------

@cue('Player.Underwater.Loop', variations=1, space='2D', cls='Ambience', jitter=0.0, conc=1, loop=True, level=-24.0)
def underwater_loop(v, r):
    # With the head under: the pressed, dark hum water carries (a low rumble swelling slowly), a faint deep tone in the
    # ears, and far-off bubbles burbling now and then. Built in a circle so it loops without a seam.
    dur = 8.0
    n = ns(dur)
    rumble = N.shaped(n, child(r, 'rumble'), -6.0, 30.0, 380.0, periodic=True)
    rumble *= 0.65 + 0.35 * N.smooth_random(n, child(r, 'swell'), 0.6, periodic=True)
    t = times(n)
    # Two low tones a whole number of cycles in the loop (so the seam doesn't jump), breathing slowly.
    tones = np.sin(2.0 * np.pi * round(52.0 * dur) / dur * t) + 0.5 * np.sin(2.0 * np.pi * round(78.0 * dur) / dur * t)
    tones *= 0.6 + 0.4 * N.smooth_random(n, child(r, 'tone'), 0.4, periodic=True)
    far = np.zeros(n)
    for at in np.arange(0.6, dur, 1.7) + r.uniform(-0.3, 0.3, int(np.ceil((dur - 0.6) / 1.7))):
        burst = _bubbles(0.6, child(r, 'far', float(at)), 3 + int(r.integers(0, 4)), 200.0, 520.0)
        far += _wrap(layers((burst, float(at) % dur, -6.0)), n)
    far = F.filt(far, F.lp(900.0, 0.7), circular=True)
    x = normalize(rumble) + 0.25 * normalize(tones) + 0.3 * normalize(far)
    return F.filt(x, F.lp(700.0, 0.7), F.hp(25.0), circular=True)


@cue('Player.Underwater.Enter', variations=3, space='2D', cls='Effects', jitter=0.04, conc=1, level=-14.0)
def underwater_enter(v, r):
    # The head going under: the water closing over the ears in a dull gulp, a rush pressed down to a murmur, a few bubbles.
    gulp = normalize(O.sine(O.expsweep(jitter(r, 150.0, 0.1), 70.0, 0.25, 1.2)) * E.perc(0.25, 0.003, 0.18), 0.0)
    rush = kit.whoosh(0.5, child(r, 'rush'), 1400.0, 260.0, 0.7, 0.15, -3.0)
    press = kit.noise_thump(0.3, child(r, 'press'), 300.0, 60.0, 0.15, 1.0, 0.04, 0.002)
    pops = _bubbles(0.45, child(r, 'pops'), 3 + int(r.integers(0, 3)), 300.0, 800.0, 0.05)
    x = layers((gulp, 0.0, 0.0), (rush, 0.0, -4.0), (press, 0.0, -6.0), (kit.set_level(pops, gulp, -10.0, 0.01), 0.0, 0.0))
    return F.filt(x, F.lp(2500.0, 0.7), F.hp(30.0), extend=False)


@cue('Player.Underwater.Exit', variations=3, space='2D', cls='Effects', jitter=0.04, conc=1, level=-15.0)
def underwater_exit(v, r):
    # The head coming up: the air back in a bright wash as the water runs out of the ears, drips off the hair, a breath out.
    dur = 0.5
    wash = normalize(_water(dur, child(r, 'wash'), 900.0, 7000.0, 180.0, 0.8, -1.5) * E.perc(dur, 0.006, 0.3), 0.0)
    drips = _drops(0.7, child(r, 'drips'), 30.0, 0.25, 1000.0, 2600.0, 0.05)
    breath = _breath(child(r, 'breath'), 0.3, [(0.0, 'h'), (0.12, 'uh'), (0.3, 'h')], 0.02, 0.2)
    x = layers((wash, 0.0, 0.0), (kit.set_level(drips, wash, -14.0, 0.01), 0.0, 0.0), (breath, 0.12, -9.0))
    return F.filt(x, F.peak(3300.0, -3.0, 0.8), F.highshelf(6500.0, -4.0), F.hp(80.0), extend=False)


@cue('Player.Underwater.Bubbles', variations=4, space='2D', cls='Effects', jitter=0.05, conc=1, level=-17.0)
def underwater_bubbles(v, r):
    # A breath let out under water: bubbles bubbling up past the ears, a low gurgle under the first ones, muffled.
    gurgle = normalize(_water(0.35, child(r, 'gurgle'), 90.0, 500.0, 60.0, 0.9, -4.0) * E.perc(0.35, 0.01, 0.25), 0.0)
    run = _bubbles(0.9, child(r, 'run'), 7 + int(r.integers(0, 6)), 230.0, 900.0, 0.0, 0.7, 8.0)
    x = layers((kit.set_level(gurgle, run, -6.0, 0.01), 0.0, 0.0), (run, 0.02, 0.0))
    return F.filt(x, F.lp(2400.0, 0.7), F.hp(40.0), extend=False)
