"""Odessa Lark's ember powers (Embers/Powers: Lucky Streak, Fifty-Two Pickup, Double or Nothing): the cardsharp's luck, in
paper, gold and silver. What she does is hers (2D); what her cards and ricochets do out in the world is placed (3D).

- Lucky Streak dealt (2D): a deck riffled in one hand (paper ticks speeding up, the cards' breath), a gold coin spun on
  its edge on the felt, and a run of three small warm chimes rising (a major arpeggio: luck turning).
- A lucky round (2D, every third shot, ridden on the gunshot): one small gold coin's ring, high and short, kept off the
  ear's touchiest band.
- A ricochet (3D): a bright glance off the hit (a short falling whistle), a coin-like tink as it turns, and the snap of a
  card flicked off.
- Fifty-Two Pickup flung (2D): the deck sprung from the hand: a burst of fifty-odd card snaps over a third of a second,
  fanning from the middle out to both ears, the rush of air of the whole fan, a leather card case's thump.
- Cards slicing in (3D): two or three paper whips and the thin cuts of their edges, a soft wet thud under them.
- Double or Nothing called (2D): a coin flicked off the thumb, ringing as it spins up and falls, slapped down on the back
  of a gloved hand, a low drum of the table's felt.
- Double (2D, on the shot): a coin's bright ring, heads, with a small rising fifth. Nothing (2D): a dull coin dropped flat
  on felt, a short falling breath.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, fade
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S
from lib import granular as G
from recipes import cue
from recipes import kit


# --- What cards, coins and her hands are ---------------------------------------------------------------------------------

def _snap(r, f=2600.0, t60=0.012):
    """One card let go off the thumb: a stiff card's crisp little snap (a band of noise rung for a hair) and its flutter."""
    snap = kit.burst(0.01, child(r, 'snap'), t60 * 0.5, lo=f * 0.5, hi=f * 2.2, attack=0.0002)
    flick = N.band(ns(0.03), child(r, 'flick'), 900.0, 4500.0) * E.perc(0.03, 0.001, 0.02)
    return normalize(layers((snap, 0.0, 0.0), (normalize(flick), 0.002, -8.0)), 0.0)


def _riffle(dur, r, count, f_lo=1800.0, f_hi=3400.0, accel=True):
    """A deck riffled: count card snaps over dur, coming faster and faster (or evenly), each a little different."""
    out = np.zeros(ns(dur))
    for i in range(count):
        u = (i + r.uniform(0.0, 0.6)) / count
        t = dur * (1.0 - (1.0 - u) ** 1.6) if accel else dur * u
        g = _snap(child(r, 'card', i), r.uniform(f_lo, f_hi), r.uniform(0.008, 0.016)) * r.uniform(0.5, 1.0)
        out = layers(out, (g, t, 0.0))
    return normalize(out, 0.0)


def _coin(r, f0, t60=0.25):
    """A gold coin struck: a small thick disc's few inharmonic modes, ringing longer than the silver's thin ones."""
    modes = M.plate(f0, child(r, 'coin'), 7, t60, 0.5, 1.0, 0.02, 0.7)
    return normalize(M.strike(modes, M.hammer(0.00005), t60 * 1.5 + 0.01), 0.0)


def _spin(dur, r, f0, rate0=18.0, rate1=55.0):
    """A coin spinning down on its edge: its ring pulsing faster and faster as it wobbles flat, then still."""
    n = ns(dur)
    ring = _coin(child(r, 'ring'), f0, dur * 0.9)
    ring = ring[:n] if ring.size >= n else np.pad(ring, (0, n - ring.size))
    rate = O.expsweep(rate0, rate1, dur, 1.4, n=n)
    wobble = 0.35 + 0.65 * np.abs(np.sin(np.pi * O.phase(rate, n)))
    return normalize(ring * wobble * E.ar(dur, 0.01, dur * 0.4, 1.5), 0.0)


def _chime(f, r, t60=0.6):
    return kit.chime(f * jitter(r, 1, 0.005), r, t60, 0.55)


def _calm(x, hp=120.0):
    """Heard over and over: the 2-5 kHz band and the very top kept soft."""
    return F.filt(x, F.peak(3300.0, -6.0, 0.8), F.highshelf(8000.0, -4.0), F.hp(hp), extend=False)


def _own(x, r, wet_db=-18.0, t60=0.9):
    """Her own: centred with a little width and a short warm room round it."""
    ir = R.stereo_ir(R.room, child(r, 'ir1'), child(r, 'ir2'), t60=t60, size=0.7)
    dry = S.widen(x, child(r, 'wide'), 0.3) if x.ndim == 1 else x
    wet = F.convolve(F.filt(kit_mono(x), F.hp(250.0), extend=False), ir)
    return layers(dry, kit.set_level(wet, dry, wet_db, 0.02))


def kit_mono(x):
    return x if x.ndim == 1 else x.mean(axis=0)


def _yard(x, r, rel_db=-15.0, dur=1.0):
    """Out in the world: a couple of slap-backs and a short dark tail (mono, for a placed sound)."""
    ir = R.outdoor(child(r, 'yard'), dur=dur, slaps=((0.09, -9.0), (0.23, -14.0)), tail_t60=0.8, tail_db=-14.0, tail_lp=2600.0)
    wet = F.convolve(F.filt(x, F.hp(220.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


# --- Lucky Streak --------------------------------------------------------------------------------------------------------

@cue('Ember.LuckyStreak.Start', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-8.0)
def lucky_start(v, r):
    # The deck riffled in one hand, a gold coin spun on the felt under it, and three warm chimes rising: luck turning.
    riffle = _riffle(0.38, child(r, 'riffle'), 22)
    riffle = F.filt(riffle, F.peak(3300.0, -5.0, 0.8), extend=False)
    air = kit.whoosh(0.4, child(r, 'air'), 500.0, 1800.0, 0.8, 0.7, -3.0)
    spin = _spin(0.9, child(r, 'spin'), 1450.0 * jitter(r, 1, 0.03))
    root = 659.3 * (1.0 if v == 0 else 1.06)
    run = layers((_chime(root, child(r, 'c1')), 0.0, 0.0), (_chime(root * 1.26, child(r, 'c2')), 0.09, -1.0),
                 (_chime(root * 1.5, child(r, 'c3')), 0.18, -2.0), (_chime(root * 2.0, child(r, 'c4'), 0.9), 0.3, -6.0))
    felt = kit.noise_thump(0.12, child(r, 'felt'), 500.0, 120.0, 0.05)
    x = layers((S.pan(riffle, -0.25), 0.0, 0.0), (S.widen(air, child(r, 'aw'), 0.4), 0.05, -10.0), (felt, 0.36, -10.0),
               (S.pan(spin, 0.2), 0.36, -12.0), (S.widen(run, child(r, 'rw'), 0.35), 0.4, -6.0))
    return _own(_calm(x, 90.0), r, -17.0, 1.1)


@cue('Ember.LuckyStreak.Lucky', variations=3, space='2D', cls='Effects', jitter=0.03, conc=2, level=-18.0)
def lucky_round(v, r):
    # A lucky round leaving the gun: one small gold coin's ring, high and short, riding on the shot.
    coin = _coin(child(r, 'coin'), [1350.0, 1440.0, 1280.0][v], 0.18)
    glint = kit.chime(1760.0 * jitter(r, 1, 0.01), child(r, 'glint'), 0.25, 0.4)
    x = layers((coin, 0.0, 0.0), (glint, 0.012, -9.0))
    x = fade(normalize(x), 0.0, 0.03)
    return S.widen(_calm(x, 400.0), child(r, 'w'), 0.25)


@cue('Ember.LuckyStreak.Ricochet', variations=4, att='Near', jitter=0.05, conc=3, level=-12.0)
def lucky_ricochet(v, r):
    # A bright glance off the hit and away, a coin-bright tink as it turns, and the snap of a card flicked off.
    whine = kit.ricochet(0.32 + 0.05 * r.random(), child(r, 'whine'), 2500.0 * jitter(r, 1, 0.08), 1000.0, 38.0)
    tink = _coin(child(r, 'tink'), 1350.0 * jitter(r, 1, 0.06), 0.12)
    card = _snap(child(r, 'card'), 2400.0)
    x = layers((tink, 0.0, -2.0), (whine, 0.004, 0.0), (card, 0.03, -8.0))
    x = F.filt(x, F.peak(3300.0, -9.0, 0.7), F.highshelf(7000.0, -5.0), F.hp(300.0), extend=False)
    return _yard(normalize(x), r, -17.0, 0.8)


# --- Fifty-Two Pickup ----------------------------------------------------------------------------------------------------

@cue('Ember.FiftyTwoPickup.Throw', variations=3, space='2D', cls='Effects', jitter=0.03, conc=1, level=-9.0)
def fifty_two_throw(v, r):
    # The whole deck sprung from the hand: a burst of card snaps fanning out from the middle to both ears over a third of
    # a second, the rush of air of the fan going, and the card case's leather thump as the hand empties.
    count = 54
    dur = 0.34
    left = np.zeros(ns(dur))
    right = np.zeros(ns(dur))
    for i in range(count):
        t = dur * (i / count) ** 0.8 + r.uniform(0.0, 0.008)
        g = _snap(child(r, 'card', i), r.uniform(1700.0, 3600.0), r.uniform(0.008, 0.014)) * r.uniform(0.45, 1.0)
        p = np.clip(r.uniform(-1.0, 1.0) * (0.25 + 0.75 * i / count), -1.0, 1.0)
        a = (p + 1.0) * np.pi / 4.0
        left = layers(left, (g * np.cos(a) * np.sqrt(2.0), t, 0.0))
        right = layers(right, (g * np.sin(a) * np.sqrt(2.0), t, 0.0))
    m = max(left.size, right.size)
    burst = np.vstack([np.pad(left, (0, m - left.size)), np.pad(right, (0, m - right.size))])
    burst = F.filt(burst, F.peak(3300.0, -6.0, 0.8), extend=False)
    fan = kit.whoosh(0.5, child(r, 'fan'), 350.0, 2600.0, 0.9, 0.3, -3.0)
    case = layers((kit.noise_thump(0.1, child(r, 'case'), 700.0, 140.0, 0.04), 0.0, 0.0),
                  (kit.cloth(0.1, child(r, 'leather'), 300.0, 2000.0, 160.0, 0.15, 0.6), 0.0, -8.0))
    x = layers((case, 0.0, -6.0), (burst, 0.006, 0.0), (S.widen(fan, child(r, 'fw'), 0.5), 0.0, -7.0))
    return _own(_calm(x, 100.0), r, -18.0, 0.8)


@cue('Ember.FiftyTwoPickup.Hit', variations=4, att='Near', jitter=0.06, conc=3, level=-13.0)
def fifty_two_hit(v, r):
    # Cards slicing in: two or three paper whips and the thin cuts of their edges, a soft dull thud of the body under them.
    out = np.zeros(1)
    for i in range(2 + v % 2):
        n = ns(0.05)
        whip = F.sweep(N.pink(n, child(r, 'whip', i)), 'bp', O.expsweep(2600.0, 800.0, 0.05, 1.0, n=n), 2.2)
        whip = normalize(whip * E.perc(0.05, 0.0006, 0.03))
        cut = kit.burst(0.02, child(r, 'cut', i), 0.006, lo=900.0, hi=3500.0, attack=0.0002)
        out = layers(out, (whip, 0.022 * i + r.uniform(0.0, 0.01), -1.5 * i), (cut, 0.022 * i + 0.003, -6.0 - 1.5 * i))
    thud = kit.noise_thump(0.12, child(r, 'thud'), 500.0, 110.0, 0.05)
    x = layers((out, 0.0, 0.0), (thud, 0.0, -5.0))
    x = F.filt(x, F.peak(3300.0, -8.0, 0.7), F.highshelf(8000.0, -4.0), F.hp(150.0), extend=False)
    return _yard(normalize(x), r, -18.0, 0.7)


# --- Double or Nothing ---------------------------------------------------------------------------------------------------

@cue('Ember.DoubleOrNothing.Start', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-8.0)
def double_start(v, r):
    # The coin flicked off the thumb (a nail's click), ringing as it spins up and falls, slapped down on the back of a
    # gloved hand, and the table's felt drummed once under the call.
    flick = kit.metal_click(child(r, 'flick'), 1800.0, 6500.0, 0.02, 0.00006, 5, -12.0, 0.4)
    spin = _spin(0.55, child(r, 'spin'), 1650.0 * jitter(r, 1, 0.03), 26.0, 34.0)
    slap = layers((kit.noise_thump(0.09, child(r, 'slap'), 900.0, 180.0, 0.03), 0.0, 0.0),
                  (kit.cloth(0.07, child(r, 'glove'), 400.0, 2500.0, 200.0, 0.1, 0.6), 0.0, -9.0))
    muffle = F.filt(_coin(child(r, 'caught'), 1650.0, 0.06), F.lp(2500.0), extend=False)
    drum = kit.thump(78.0, 46.0, 0.45, 0.3)
    fifth = layers((_chime(523.3, child(r, 'n1'), 0.8), 0.0, 0.0), (_chime(784.0, child(r, 'n2'), 0.9), 0.07, -3.0))
    x = layers((flick, 0.0, -4.0), (S.pan(spin, 0.15), 0.01, -3.0), (slap, 0.58, 0.0), (muffle, 0.58, -10.0),
               (drum, 0.6, -6.0), (S.widen(fifth, child(r, 'fw'), 0.3), 0.62, -9.0))
    return _own(_calm(x, 60.0), r, -17.0, 1.0)


@cue('Ember.DoubleOrNothing.Double', variations=3, space='2D', cls='Effects', jitter=0.03, conc=2, level=-17.0)
def double_heads(v, r):
    # Heads: a coin's bright ring with a small rising fifth over it, short enough to ride every pull.
    coin = _coin(child(r, 'coin'), [1750.0, 1830.0, 1690.0][v], 0.2)
    up = layers((_chime(1046.5, child(r, 'a'), 0.35), 0.0, 0.0), (_chime(1568.0, child(r, 'b'), 0.4), 0.05, -2.0))
    x = fade(normalize(layers((coin, 0.0, 0.0), (up, 0.01, -5.0))), 0.0, 0.04)
    return S.widen(_calm(x, 350.0), child(r, 'w'), 0.25)


@cue('Ember.DoubleOrNothing.Nothing', variations=3, space='2D', cls='Effects', jitter=0.03, conc=2, level=-18.0)
def double_nothing(v, r):
    # Nothing: a coin dropped flat on felt (a dull, damped clack and the felt's soft pat), a short falling breath.
    coin = F.filt(_coin(child(r, 'coin'), 1500.0 * jitter(r, 1, 0.05), 0.05), F.lp(2200.0), extend=False)
    felt = kit.noise_thump(0.09, child(r, 'felt'), 600.0, 140.0, 0.035)
    sigh = kit.whoosh(0.22, child(r, 'sigh'), 900.0, 300.0, 0.8, 0.25, -3.0)
    x = layers((felt, 0.0, 0.0), (coin, 0.0, -5.0), (sigh, 0.01, -12.0))
    x = fade(normalize(x), 0.0, 0.04)
    return S.widen(_calm(x, 120.0), child(r, 'w'), 0.2)
