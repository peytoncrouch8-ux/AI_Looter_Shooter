"""The Stone Sentinel's new attacks (Docs/Work/2026-10-31-Creatures/CONTRACT.md, SENTINEL; AStoneSentinelCreature,
AStoneSentinelBoulder, AStoneSentinelShockwave): the hurled boulder, the ground-slam shockwave, its open window and the
pebbles off its joints. All synthesized, 3D at the statue (or the rock, or the ground), on sentinel.py's stone (its grind,
crack, trickle, thud and last stage) and the barrow fight's earth, stones and clods: a statue of tons that grinds where a
creature would breathe. Names in Creatures/StoneSentinelRules.h (StoneSentinelCue). Imported by sentinel.py, so it renders
with the sentinel's group whether or not it's listed in GROUPS.

- Creature.Sentinel.Tear (~1.4 s): the rock torn out of the ground: roots snapping, earth tearing, stone grinding loose, a
  lurch as it comes free, dirt and clods pouring off, the grind of the arms lifting it.
- Creature.Sentinel.Heave (~0.8 s): the throw: the trunk's stone wrenched round, a great rush of air, grit flung off.
- Creature.Sentinel.BoulderFly (~1.7 s, follows the rock): a low tumbling whoom wobbling as it turns, grit streaming off.
- Creature.Sentinel.BoulderImpact (~1.8 s, 'Gun'): it lands: a crushing thud, the rock bursting, stones raining back,
  clods, dust, the valley rolling it back.
- Creature.Sentinel.BoulderShatter (~1.3 s, 'Gun'): shot to pieces in its grip, it bursts over its head: a sharp shatter,
  stones raining on stone, the statue's grinding reel.
- Creature.Sentinel.ShockWindUp (~1.45 s, swells on purpose): rearing back with its arms spread: a deep grind rising, the
  ground groaning, cracks spreading under it, a breath of air as it tops out.
- Creature.Sentinel.ShockSlam (~2.0 s, 'Gun'): both fists into the ground: a huge thud and boom, the ground splitting, a
  crackle rolling away from it as the spikes run out, stones thrown, the valley answering.
- Creature.Sentinel.ShockSpikes (~0.7 s): rock spikes punching up out of the ground near the listener: a run of hard
  stone bursts, each with a short thud, earth spraying.
- Creature.Sentinel.Exposed (~1.3 s): it's open: a hollow crack through the block with a dull ring, a low soul-light
  hum swelling under it (two tones a fifth apart and a faint shimmer), grit pouring: "now".
- Creature.Sentinel.Pebbles (6 takes, ~0.35 s, quiet, 'Near'): a few small stones ticking down off a joint onto earth.

As sentinel.py: the cracks' rings above 5 kHz and short, the ear's sharpest band dipped on every cue, the weight under
150 Hz.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, modal as M
from recipes import cue
from recipes import kit
from recipes import creatures as C
from recipes.bosses import _drum, _pressure, _rumble, _outside
from recipes.gravemother_barrow import _earth, _tonks, _lumps, _lv, _chip, _snap_run
from recipes.sentinel import _grind, _crack, _trickle, _thud, _stone


# --- Pieces ---------------------------------------------------------------------------------------------------------

def _tumble(dur, r, lp=320.0, rate=2.8):
    """A heavy rock turning over in the air: dark noise wobbling at its spin, swelling in and held."""
    n = ns(dur)
    t = times(n)
    body = F.filt(N.pink(n, child(r, 'body')), F.lpn(lp, 4), F.hp(40.0, 0.7), extend=False)
    spin = jitter(r, rate, 0.12)
    wobble = 0.55 + 0.45 * np.sin(2.0 * np.pi * spin * t + r.uniform(0.0, 2.0 * np.pi))
    shape = E.bp([(0.0, 0.0), (0.25, 1.0), (dur - 0.2, 1.0), (dur, 0.0)], n=n, curve='cos')
    return normalize(body * wobble * shape, 0.0)


def _hum(dur, r, f0=110.0):
    """The soul-light in the stone: two low tones a fifth apart, beating slowly, and a faint shimmer over them."""
    n = ns(dur)
    t = times(n)
    f = jitter(r, f0, 0.03)
    low = np.sin(2.0 * np.pi * f * t) + 0.6 * np.sin(2.0 * np.pi * (f * 1.5 + 0.7) * t)
    shimmer = 0.12 * np.sin(2.0 * np.pi * f * 8.0 * t) * (0.5 + 0.5 * np.sin(2.0 * np.pi * 5.5 * t))
    shape = E.bp([(0.0, 0.0), (0.15, 1.0), (dur * 0.5, 0.7), (dur, 0.0)], n=n, curve='cos')
    return normalize(F.filt((low + shimmer) * shape, F.lp(1800.0, 0.7), F.hp(60.0, 0.7), extend=False), 0.0)


def _burst_run(dur, r, count, lo=500.0, hi=6000.0):
    """Stone punching up out of the ground, again and again: hard cracks each with a short thud, coming close together."""
    x = np.zeros(1)
    t = 0.0
    for i in range(count):
        rr = child(r, 'burst', i)
        crack = _crack(child(rr, 'c'), lo, hi, (rr.uniform(1100.0, 1600.0), rr.uniform(4800.0, 6400.0)))
        thud = kit.noise_thump(0.12, child(rr, 't'), rr.uniform(700.0, 1100.0), 110.0, 0.05, 1.0, 0.03)
        x = layers(x, (crack, t, -2.0 * i * 0.5 - rr.uniform(0.0, 2.0)), (thud, t, -3.0 - rr.uniform(0.0, 2.0)))
        t += rr.uniform(0.035, 0.07) * dur / 0.6
    return normalize(x, 0.0)


# --- The boulder ----------------------------------------------------------------------------------------------------

@cue('Creature.Sentinel.Tear', variations=2, att='Creature', jitter=0.03, conc=2, level=-7.0)
def tear(v, r):
    # The rock torn out of the ground: roots snapping and earth tearing as the hands dig under it, stone grinding loose, a
    # lurch as it comes free, dirt and clods pouring off, then the long grind of the arms lifting its weight.
    roots = _snap_run(0.4, child(r, 'roots'), 20.0, 60.0, 600.0, 3500.0, 0.012)
    earth = _earth(1.0, child(r, 'earth'), 120.0, 1500.0, 0.6, 0.02, 40.0, 0.8)
    loose = _grind(0.7, child(r, 'loose'), 10.0, 26.0, (110.0, 280.0, 650.0))
    lurch = _thud(child(r, 'lurch'), 0.4, 600.0, 60.0, 0.15, 58.0, -6.0)
    pour = _trickle(1.0, child(r, 'pour'), 2600.0, 500.0, 2600.0)
    clods = _lumps(0.9, child(r, 'clods'), 6, 0.1, 0.7, 120.0, 320.0, 0.84, 0.03)
    lift = _grind(0.6, child(r, 'lift'), 12.0, 30.0, (140.0, 340.0, 800.0), 0.2, 0.2)
    tl = 0.3 + 0.04 * v
    x = layers((normalize(roots), 0.0, -5.0), (earth, 0.02, -6.0), (loose, 0.05, -2.0), (lurch, tl, 0.0),
               (pour, tl + 0.05, -14.0), (_lv(clods, lurch, -8.0), tl + 0.05, 0.0), (lift, 0.62, -5.0))
    return C._outside(_stone(x, 3.0, 2.0), r, -18.0, 0.07, 0.18, 0.7)


@cue('Creature.Sentinel.Heave', variations=3, att='Creature', jitter=0.04, conc=2, level=-8.0)
def heave(v, r):
    # The throw: the trunk's stone wrenched round (a short hard grind), a great rush of air as the rock leaves its hands,
    # the push of it, grit flung off the arms.
    wrench = _grind(0.25, child(r, 'wrench'), 20.0, 40.0, (150.0, 360.0, 820.0), 0.02, 0.1)
    rush = kit.whoosh(0.7, child(r, 'rush'), 140.0, 700.0, 0.8, 0.35, -2.0, 1.0)
    push = _pressure(child(r, 'push'), 0.03, 0.3, 200.0)
    grit = kit.grit_burst(0.4, child(r, 'grit'), 3000.0, 600.0, 3500.0, 0.2, 0.002)
    x = layers((wrench, 0.0, -3.0), (rush, 0.05, 0.0), (push, 0.06, -6.0), (grit, 0.08, -14.0))
    return C._outside(_stone(x, 2.0, 1.0), r, -20.0, 0.07, 0.18, 0.5)


@cue('Creature.Sentinel.BoulderFly', variations=2, att='Creature', jitter=0.04, conc=3, level=-12.0, swell=True)
def boulder_fly(v, r):
    # The rock in flight (it follows the rock): a low tumbling whoom wobbling as it turns over, the rush of its passing,
    # grit streaming off it.
    dur = 1.7
    tumble = _tumble(dur, child(r, 'tumble'), 320.0, 2.6 + 0.4 * v)
    rush = kit.whoosh(dur, child(r, 'rush'), 120.0, 420.0, 0.7, 0.55, -3.0, 1.0)
    grit = _trickle(dur, child(r, 'grit'), 1400.0)
    x = layers((tumble, 0.0, 0.0), (rush, 0.0, -4.0), (grit, 0.1, -18.0))
    return C._outside(_stone(x, 2.0, 1.0), r, -22.0, 0.07, 0.18, 0.6)


@cue('Creature.Sentinel.BoulderImpact', variations=3, att='Gun', jitter=0.03, conc=2, level=-3.0)
def boulder_impact(v, r):
    # It lands: the push of air, a crushing thud with a boom under it, the rock bursting (a crack and its snaps), stones
    # and clods thrown up and raining back, a drift of dust, the valley rolling it back.
    fall = _thud(child(r, 'fall'), 1.0, 1200.0, 38.0, 0.45, 42.0, -4.0)
    knock = kit.thunk(jitter(r, 330.0, 0.1), child(r, 'knock'), 0.1, 0.0003, 7, 0.1)
    burst = layers(_crack(child(r, 'burst'), 500.0, 6500.0, (1100.0, 5800.0)),
                   (_snap_run(0.3, child(r, 'snaps'), 300.0, 40.0, 600.0, 4500.0, 0.016), 0.0, -6.0))
    thrown = layers(_tonks(1.4, child(r, 'stones'), 16, 0.12, 1.1, 380.0, 2200.0, 0.86, 0.05),
                    (_lumps(1.2, child(r, 'clods'), 6, 0.1, 0.9, 110.0, 340.0, 0.84, 0.04), 0.0, -3.0))
    dust = _earth(1.4, child(r, 'dust'), 140.0, 1800.0, 0.7, 0.05, 36.0, 0.75)
    rumble = _rumble(1.8, child(r, 'rumble'), 220.0, 1.1, 0.1, 3.0)
    x = layers((fall, 0.0, 3.0), (knock, 0.0, -4.0), (_lv(burst, fall, -5.0), 0.003, 0.0),
               (_lv(thrown, fall, -12.0), 0.02, 0.0), (_lv(dust, fall, -14.0), 0.02, 0.0), (rumble, 0.0, -12.0))
    return _outside(_stone(x, 4.0, 3.0), r, -14.0)


@cue('Creature.Sentinel.BoulderShatter', variations=2, att='Gun', jitter=0.03, conc=2, level=-5.0)
def boulder_shatter(v, r):
    # Shot to pieces in its grip, it bursts over its head: a sharp shatter and its snaps, a dense knock, stones raining down
    # on stone, a burst of dust, the statue's grinding reel under it.
    shatter = layers(_crack(child(r, 'shatter'), 800.0, 7000.0, (1600.0, 6800.0)),
                     (_snap_run(0.2, child(r, 'snaps'), 200.0, 40.0, 900.0, 5200.0, 0.012), 0.0, -4.0))
    knock = kit.thunk(jitter(r, 380.0, 0.1), child(r, 'knock'), 0.08, 0.0003, 7, 0.1)
    rain = _tonks(1.1, child(r, 'rain'), 18, 0.05, 0.9, 500.0, 2600.0, 0.86, 0.04)
    dust = kit.burst(1.0, child(r, 'dust'), 0.5, lo=150.0, hi=1500.0, color=-3.0, attack=0.04)
    reel = _grind(0.5, child(r, 'reel'), 16.0, 8.0, (120.0, 290.0, 700.0), 0.05, 0.25)
    x = layers((shatter, 0.0, 0.0), (knock, 0.0, -5.0), (_lv(rain, shatter, -9.0), 0.04, 0.0), (dust, 0.01, -10.0),
               (reel, 0.2, -5.0))
    return _outside(_stone(x, 3.0, 2.0), r, -15.0)


# --- The shockwave --------------------------------------------------------------------------------------------------

@cue('Creature.Sentinel.ShockWindUp', variations=2, att='Creature', jitter=0.03, conc=2, level=-8.0, swell=True)
def shock_wind_up(v, r):
    # Rearing back with its arms spread wide: a deep grind rising through the whole block, the ground groaning under it and
    # cracking in a ring round its feet, grit sifting off it, a breath of air as it tops out.
    dur = 1.45
    grind = _grind(1.35, child(r, 'grind'), 6.0, 30.0, (90.0, 230.0, 560.0), 0.4, 0.1)
    n = ns(dur)
    groan = F.filt(N.pink(n, child(r, 'groan')), F.lpn(140.0, 4), F.hp(32.0, 0.7), extend=False)
    groan = normalize(groan * E.swell(dur, 1.25, 0.15, 2.0), 0.0)
    cracks = _snap_run(1.2, child(r, 'cracks'), 4.0, 40.0, 500.0, 3800.0, 0.014)
    trickle = _trickle(dur, child(r, 'trickle'), 1600.0)
    air = kit.whoosh(0.4, child(r, 'air'), 250.0, 1100.0, 0.9, 0.7, -3.0, 1.0)
    x = layers((grind, 0.0, 0.0), (groan, 0.0, -4.0), (normalize(cracks), 0.1, -9.0), (trickle, 0.1, -16.0), (air, 1.0, -12.0))
    return C._outside(_stone(x, 2.0, 1.0), r, -20.0, 0.07, 0.18, 0.6)


@cue('Creature.Sentinel.ShockSlam', variations=3, att='Gun', jitter=0.03, conc=2, level=-2.0)
def shock_slam(v, r):
    # Both fists driven into the ground: a huge thud and a boom under it, the ground splitting, a crackle rolling away from
    # it as the spikes run out, earth sliding, stones thrown, the valley answering.
    fall = _thud(child(r, 'fall'), 1.1, 1000.0, 34.0, 0.5, 38.0, -3.0)
    boom = _drum(jitter(r, 36.0, 0.05), 1.4, child(r, 'boom'), 1.0, 0.12, 0.003)
    knock = kit.thunk(jitter(r, 300.0, 0.1), child(r, 'knock'), 0.12, 0.0003, 7, 0.1)
    split = layers(_crack(child(r, 'split'), 400.0, 6000.0, (1000.0, 5200.0)),
                   (_snap_run(0.4, child(r, 'run'), 300.0, 30.0, 500.0, 4200.0, 0.02), 0.0, -6.0))
    crackle = _snap_run(1.1, child(r, 'crackle'), 60.0, 12.0, 400.0, 3000.0, 0.02)
    crackle = normalize(crackle * E.bp([(0.0, 1.0), (1.1, 0.15)], n=crackle.size), 0.0)
    slide = _earth(1.2, child(r, 'slide'), 120.0, 1400.0, 0.8, 0.05, 30.0, 0.8)
    stones = _tonks(1.3, child(r, 'stones'), 10, 0.15, 1.0, 400.0, 2000.0, 0.86, 0.05)
    rumble = _rumble(2.0, child(r, 'rumble'), 200.0, 1.3, 0.12, 3.0)
    x = layers((fall, 0.0, 3.0), (boom, 0.0, -4.0), (knock, 0.0, -5.0), (_lv(split, fall, -6.0), 0.004, 0.0),
               (_lv(crackle, fall, -11.0), 0.08, 0.0), (_lv(slide, fall, -14.0), 0.05, 0.0), (_lv(stones, fall, -13.0), 0.02, 0.0),
               (rumble, 0.0, -10.0))
    return _outside(_stone(x, 4.0, 3.0), r, -13.0)


@cue('Creature.Sentinel.ShockSpikes', variations=4, att='Creature', jitter=0.05, conc=3, level=-9.0)
def shock_spikes(v, r):
    # Rock spikes punching up out of the ground near the listener: a run of hard stone bursts, each with its thud, earth
    # spraying up round them.
    run = _burst_run(0.6, child(r, 'run'), 5 + v % 3)
    spray = _earth(0.5, child(r, 'spray'), 200.0, 2500.0, 0.25, 0.005, 50.0, 0.8)
    x = layers((run, 0.0, 0.0), (_lv(spray, run, -9.0), 0.01, 0.0))
    return C._outside(_stone(x, 3.0, 2.0), r, -20.0, 0.06, 0.15, 0.4)


# --- Its open window and its joints ---------------------------------------------------------------------------------

@cue('Creature.Sentinel.Exposed', variations=2, att='Creature', jitter=0.02, conc=2, level=-9.0)
def exposed(v, r):
    # It's open: a hollow crack through the block with a dull ring, the soul-light's low hum swelling under it, grit
    # pouring out of the cracks: the player's "now".
    crack = _crack(child(r, 'crack'), 600.0, 6000.0, (900.0, 4200.0))
    ring = normalize(M.strike(M.plate(jitter(r, 190.0, 0.05), child(r, 'ring'), 14, 0.5, 0.5, 1.37, 0.03, 0.5), M.hammer(0.0002), 0.6), 0.0)
    hum = _hum(1.2, child(r, 'hum'), 104.0 + 8.0 * v)
    pour = _trickle(1.0, child(r, 'pour'), 2400.0)
    x = layers((crack, 0.0, -2.0), (ring, 0.0, -5.0), (hum, 0.04, -10.0), (pour, 0.05, -15.0))
    return C._outside(_stone(x, 2.0, 1.0), r, -18.0, 0.07, 0.18, 0.7)


@cue('Creature.Sentinel.Pebbles', variations=6, att='Near', jitter=0.08, conc=4, level=-22.0)
def pebbles(v, r):
    # A few small stones ticking down off a joint onto earth: short dense ticks, a soft knock or two, a whisper of grit.
    x = np.zeros(1)
    t = 0.0
    for i in range(2 + v % 3):
        rr = child(r, 'p', i)
        tick = _chip(child(rr, 'tick'), rr.uniform(1500.0, 2400.0), rr.uniform(4000.0, 6000.0))
        knock = kit.thunk(rr.uniform(500.0, 900.0), child(rr, 'knock'), 0.02, 0.0004, 4, 0.15)
        x = layers(x, (tick, t, -3.0 * i - rr.uniform(0.0, 3.0)), (knock, t + 0.002, -8.0 - rr.uniform(0.0, 3.0)))
        t += rr.uniform(0.03, 0.09)
    grit = kit.grit_burst(0.25, child(r, 'grit'), 2000.0, 1200.0, 5000.0, 0.12, 0.002)
    x = layers((normalize(x), 0.0, 0.0), (grit, 0.0, -14.0))
    x = F.filt(x, F.peak(3300.0, -5.0, 0.8), F.hp(120.0), extend=False)
    return C._outside(x, r, -26.0, 0.05, 0.12, 0.2)
