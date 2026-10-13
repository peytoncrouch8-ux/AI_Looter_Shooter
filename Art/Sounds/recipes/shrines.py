"""The vigil stones (ABossShrine): a boss's stone beside its arena, a granite stele on two steps with grave candles on its
steps and a small iron hand bell hanging from a black iron crook. Ring the bell and the laid-to-rest walk again.

An iron bell rings duller and shorter than the Bell-Ringer's bronze (bells.py's hand bell made of iron: its upper partials
dying fast, its pairs split wider by a rougher casting, a little of the metal's buzz), lower for its size. Candle flames are
small: combustion's breath (bosses.py's flame, kept low and soft), torn by flicker, a wick's crackle. Stone grinding up
through earth is a rough low judder with grit in it, clods pattering off.

- Rise (3D): the stone grinding up out of the ground, rising a little in pitch as it comes, dirt and clods falling off it,
  a settling thud, then the candles catching with a soft breath of flame.
- Activate (3D, near): kneeling at it, its menu opening: the candle flames gusting up and settling, a low hum in the stone,
  the bell's clapper stirring once against its side.
- Unavailable (2D, the player's own): the call refused: the clapper caught in a hand, a dull choked knock and a short
  muffled rattle, a breath let out.
- Summon (3D, carried like a gunshot): the iron bell rung three times, hard, its ring waxing and waning as it swings; the
  candles flaring; the grave-wind rising under it with whispering voices; a deep thud far off as the dead answer.
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter, add_at
from lib import noise as N, env as E, filters as F, modal as M, voice as V
from recipes import cue
from recipes import kit
from recipes.bells import handbell, ring, _swinging, _across
from recipes.bosses import _flame, _crackle, _debris, _rumble
from recipes.creatures import _haunt, _outside

# The iron bell's strike note for each take: low for a bell its size (iron is heavy and dull).
IRON_PRIMES = (430.0, 455.0, 412.0)


def iron_bell(prime, r, t60=1.8):
    """The stone's iron hand bell: a hand bell's partials, duller (the upper ones die fast), pairs split wider, a little
    crack's buzz in the casting."""
    return handbell(prime, r, t60=t60, beat=1.5, bright=0.76, crack=0.18)


def _candles(dur, r, peak_at, fc=520.0, crackle_rate=40.0):
    """A row of candle flames gusting: soft flame breath swelling to peak_at and settling, with the wicks' crackle."""
    breath = _flame(dur, child(r, 'breath'), [(0.0, fc * 0.6), (peak_at, fc), (dur, fc * 0.7)], 22.0, 0.55, 1.0)
    breath = breath * E.swell(dur, peak_at, max(dur - peak_at, 0.1) * 0.7, 1.6)
    wicks = _crackle(dur, child(r, 'wicks'), crackle_rate, max(dur * 0.5, 0.1), 1800.0, 5200.0, 4500.0)
    return layers((normalize(breath), 0.0, 0.0), (wicks, 0.02, -18.0))


def _grind(dur, r, f0=60.0, f1=150.0):
    """Stone forced up through packed earth: low noise through a resonance rising from f0 to f1, juddering as it sticks and
    slips, grit grinding in it; it swells in, holds and eases off."""
    n = ns(dur)
    t = times(n)
    low = F.sweep(N.pink(n, child(r, 'low'), lo=35.0), 'bp', np.geomspace(f0, f1, n), 1.3)
    judder = 0.4 + 0.6 * np.abs(N.smooth_random(n, child(r, 'judder'), 19.0)) ** 1.4
    grit = N.band(n, child(r, 'grit'), 800.0, 3600.0) * (0.25 + 0.75 * np.abs(N.smooth_random(n, child(r, 'bite'), 37.0)) ** 2)
    env = E.bp([(0.0, 0.0), (0.3, 1.0), (dur * 0.75, 0.85), (dur, 0.0)], n=n, curve='cos')
    # A little louder as it frees itself near the top.
    env = env * (0.8 + 0.2 * np.clip(t / dur, 0.0, 1.0))
    return layers((normalize(low * judder * env), 0.0, 0.0), (normalize(grit * env), 0.0, -13.0))


@cue('World.Shrine.Rise', variations=2, att='Creature', cls='Effects', jitter=0.03, conc=1, level=-9.0)
def shrine_rise(v, r):
    # The stone comes up out of the grave dirt (2.6 s), dirt and clods falling off it all the way; it settles with a thud;
    # the candles catch.
    dur = 2.6
    grind = _grind(dur, child(r, 'grind'), 58.0 * jitter(r, 1, 0.06), 150.0 * jitter(r, 1, 0.06))
    clods = _debris(dur, child(r, 'clods'), 16, 0.15, dur - 0.4, 700.0, 3200.0)
    settle = kit.noise_thump(0.5, child(r, 'settle'), 200.0, 55.0, 0.22)
    catch = _candles(0.9, child(r, 'catch'), 0.12)
    x = layers((grind, 0.0, 0.0), (clods, 0.0, -12.0), (settle, dur - 0.12, -3.0), (catch, dur + 0.05, -11.0))
    x = F.filt(x, F.hp(40.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _outside(x, r, -16.0)


@cue('World.Shrine.Activate', variations=3, att='Near', cls='Effects', jitter=0.03, conc=1, level=-14.0, swell=True)
def shrine_activate(v, r):
    # Kneeling at the stone: the candles gust up and settle, a low hum wakes in the stone a moment, and the bell's clapper
    # stirs once against its side.
    dur = 1.3
    n = ns(dur)
    candles = _candles(dur, child(r, 'candles'), 0.28 + 0.06 * r.random())
    t = times(n)
    f = 92.0 * jitter(r, 1, 0.05)
    hum = np.sin(2 * np.pi * f * t) + 0.45 * np.sin(2 * np.pi * f * 2.01 * t + 2 * np.pi * r.random())
    hum = hum * E.bp([(0.0, 0.0), (0.35, 1.0), (dur, 0.0)], n=n, curve='cos')
    m = iron_bell(IRON_PRIMES[v], child(r, 'bell'), t60=0.9)
    tap = normalize(M.strike(m, M.hammer(0.0009), 0.9))
    x = layers((candles, 0.0, 0.0), (normalize(hum), 0.05, -12.0), (tap, 0.3 + 0.1 * r.random(), -16.0))
    x = F.filt(x, F.hp(60.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _outside(x, r, -22.0)


@cue('World.Shrine.Unavailable', variations=3, space='2D', cls='Interface', jitter=0.03, conc=1, level=-16.0)
def shrine_unavailable(v, r):
    # Not yet: the clapper caught in a hand mid-swing, a dull choked knock on the iron, a short muffled rattle, and a
    # breath let out.
    m = iron_bell(IRON_PRIMES[v] * 0.98, child(r, 'bell'), t60=0.18)
    knock = normalize(M.strike(m, M.hammer(0.0016), 0.3))
    knock = F.filt(knock, F.lp(1500.0, 0.7), extend=False)
    body = kit.thunk(210.0 * jitter(r, 1, 0.08), child(r, 'body'), 0.07, 0.0012)
    rattle = np.zeros(ns(0.25))
    soft = normalize(M.strike(m, M.hammer(0.002), 0.12))
    at = 0.05
    for k in range(2 + int(r.integers(0, 2))):
        rattle = add_at(rattle, soft * (0.6 ** k) * r.uniform(0.6, 1.0), at)
        at += r.uniform(0.03, 0.05)
    rattle = F.filt(rattle, F.lp(1200.0, 0.7), extend=False)
    d = 0.5
    sigh = V.whisper(d, [(0.0, 'h'), (d, 'a')], child(r, 'sigh'), 0.9) * E.swell(d, 0.12, 0.3)
    sigh = normalize(F.filt(sigh, F.hp(350.0), F.lp(4500.0), extend=False))
    x = layers((knock, 0.0, 0.0), (body, 0.0, -6.0), (normalize(rattle), 0.0, -12.0), (sigh, 0.08, -22.0))
    x = F.filt(x, F.hp(90.0), F.peak(3000.0, -4.0, 0.9), extend=False)
    return x


@cue('World.Shrine.Summon', variations=2, att='Gun', cls='Effects', jitter=0.02, conc=1, level=-5.0)
def shrine_summon(v, r):
    # The bell rung three times, hard, swinging; the candles flare as the dead are called; the grave-wind rises under it
    # with whispering voices; far off, a deep thud: something answers.
    prime = IRON_PRIMES[v] * jitter(r, 1, 0.01)
    m = iron_bell(prime, child(r, 'bell'), t60=2.2)
    dur = 4.6
    peals = np.zeros(ns(dur))
    for k, (at, level) in enumerate(((0.0, 1.0), (0.62, 0.92), (1.24, 1.0))):
        stroke = ring(m, dur - at, child(r, 'stroke', k), 0.00022, -20.0)
        back = ring(m, dur - at - 0.2, child(r, 'back', k), 0.0003, -24.0)
        peal = layers((stroke, 0.0, 0.0), (back, 0.13 + 0.04 * r.random(), -10.0))
        peals = add_at(peals, normalize(peal) * level, at)
    peals = _swinging(normalize(peals), child(r, 'swing'), 1.5, 0.2)
    flare = _candles(1.2, child(r, 'flare'), 0.2, 700.0, 70.0)
    n = ns(3.0)
    voices = np.zeros(n + ns(0.2))
    for i in range(3):
        rr = child(r, 'voice', i)
        w = V.whisper(3.0, [(0.0, 'h'), (0.9 + 0.2 * rr.random(), 'u'), (2.0, 'o'), (3.0, 'a')], rr, shift=rr.uniform(0.85, 1.1))
        env = E.bp([(0.0, 0.0), (1.2 + 0.3 * rr.random(), 1.0), (3.0, 0.0)], n=n, curve='cos')
        voices = add_at(voices, normalize(w * env) * rr.uniform(0.6, 1.0), 0.1 * i)
    voices = F.filt(voices, F.hp(300.0), F.lp(5500.0), extend=False)
    wind = kit.whoosh(3.2, child(r, 'wind'), 220.0, 700.0, 0.8, 0.55, -4.0)
    answer = layers((kit.noise_thump(0.9, child(r, 'answer'), 120.0, 38.0, 0.5), 0.0, 0.0),
                    (_rumble(1.6, child(r, 'rumble'), 220.0, 1.2, 0.15), 0.02, -6.0))
    x = layers((peals, 0.0, 0.0), (flare, 0.05, -14.0), (normalize(voices), 1.0, -17.0), (wind, 0.8, -18.0),
               (normalize(answer), 2.9, -9.0))
    # Heard across the valley and over and over in a long session: its brightest band eased, as the Bell-Ringer's toll is.
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(45.0), extend=False)
    return _across(x, r, 2.8, -12.0)
