"""The watch fire on Calder's Post's tower (AWatchFire, wave 2b): an iron brazier of oiled kindling and split pine on a windy
tower top. All 3D.

Fire is bosses.py's flame (combustion's low broadband roar torn by flicker, through a loose resonance) made big and open:
the wind blowing it about is a slow swell of its level and its pitch, gusts brightening it. Wood in a fire crackles (fine
grit of sap and fibre snapping) and pops (a pocket of steam bursting: a short, hard crack with a little low body). Lamp oil
catching is a soft, breathy whump: air drawn in and thrust out as the vapour takes all at once. The brazier is cast iron: a
knuckle on its bowl rings a few dull, dense modes.

- World.WatchFire.Light (near, carried a little): the oil going up with a breathy whump, the fire rushing up through the
  kindling over its 0.6 s catch to the flare's peak (0.25 s) and settling over the flare's 1.75 s, crackling.
- World.WatchFire.Burn (loop, 13 s): a big open fire on a windy tower: crackle and pops over a wind-blown roar that rises
  and falls.
- World.WatchFire.Dry (near, short): a knuckle on the empty iron bowl, cold ash shifting.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, modal as M
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes.bosses import _flame, _crackle
from recipes.creatures import _outside
from recipes.world import _fold, _pnoise, _slow, _circ


def _pop(r):
    """A pocket of steam bursting in a log: a short hard crack and a little low body."""
    crack = kit.burst(0.006, child(r, 'crack'), 0.002, lo=700.0, hi=7000.0, attack=0.0002)
    body = kit.thunk(r.uniform(350.0, 700.0), child(r, 'body'), 0.025, 0.0004, 4, 0.12)
    return normalize(layers((crack, 0.0, 0.0), (body, 0.0, -6.0)), 0.0)


@cue('World.WatchFire.Light', variations=2, att='Creature', cls='Effects', jitter=0.03, conc=1, level=-8.0)
def light(v, r):
    # The torch to the oiled brazier: a breathy whump as the oil's vapour takes all at once, then the fire rushing up
    # through the kindling (its roar climbing and brightening to the flare's peak at 0.25 s), crackling harder as the
    # sticks catch, and settling into a steady burn by the flare's end.
    dur = 2.0
    n = ns(dur)
    whump = kit.noise_thump(0.45, child(r, 'whump'), 420.0, 70.0, 0.25, 0.9, 0.2, 0.004)
    breath = kit.whoosh(0.35, child(r, 'breath'), 260.0, 1100.0, 0.8, 0.3, -3.0)
    roar = _flame(dur, child(r, 'roar'), [(0.0, 240.0), (0.25, 900.0 * jitter(r, 1, 0.08)), (0.6, 1300.0), (dur, 650.0)],
                  16.0, 0.5, 0.9)
    env = E.bp([(0.0, 0.2), (0.25, 1.0), (0.6, 0.85), (1.75, 0.5), (dur, 0.0)], n=n, curve='cos')
    roar = normalize(roar * env)
    crackle = G.grit(dur, child(r, 'kindling'), 900.0 * E.bp([(0.0, 0.1), (0.4, 1.0), (dur, 0.5)], n=n), 1200.0, 6000.0,
                     2.5, 4)
    crackle = normalize(F.filt(crackle, F.lp(5000.0, 0.7), extend=False) * E.end_fade(np.ones(n), 0.05))
    pops = G.cloud(dur - 0.2, child(r, 'pops'), 5.0, lambda rr, t, i: _pop(child(rr, 'pop', i)) * rr.uniform(0.4, 1.0),
                   start=0.2)
    x = layers((whump, 0.0, 0.0), (breath, 0.0, -10.0), (roar, 0.02, -2.0), (crackle, 0.1, -15.0),
               (normalize(pops), 0.0, -13.0))
    x = F.filt(x, F.hp(45.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return _outside(x, r, -17.0, 0.08, 0.22, 0.7)


@cue('World.WatchFire.Burn', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=1, loop=True, level=-14.0,
     align=False)
def burn(v, r):
    # The watch fire burning on the windy tower: a broad roar the wind blows about (its level and its brightness rising
    # and falling with the gusts, a fast flicker tearing at it), the wood crackling all the time and popping now and then,
    # the wind itself rushing past the tower top. Thirteen seconds, built in a circle.
    dur = 13.0
    L = ns(dur)
    gust = 0.5 + 0.5 * N.smooth_random(L, child(r, 'gust'), 0.35, periodic=True)
    src = _pnoise(L, child(r, 'roar'), 60.0, 6000.0, -3.0)
    low = _circ(src, F.lp(450.0, 0.7))
    high = _circ(src, F.bp(1100.0, 0.7))
    flicker = 0.55 + 0.45 * np.abs(N.smooth_random(L, child(r, 'flicker'), 14.0, periodic=True)) ** 0.8
    roar = (normalize(low) * (0.6 + 0.4 * gust) + 0.6 * normalize(high) * gust ** 1.5) * flicker
    crackle = G.grit(dur, child(r, 'crackle'), 650.0 * (0.6 + 0.4 * gust), 1300.0, 6000.0, 2.5, 4, periodic=True)
    crackle = _circ(crackle, F.lp(5000.0, 0.7))
    events = []
    rp = child(r, 'pops')
    for k, t in enumerate(N.times_poisson(dur, rp, 1.6)):
        events.append((_pop(child(rp, k)) * rp.uniform(0.3, 1.0), t))
    pops = _fold(L, events)
    wind = _pnoise(L, child(r, 'wind'), 200.0, 3000.0, -3.0) * (0.3 + 0.7 * gust) ** 2
    x = normalize(roar) + 0.22 * normalize(crackle) + 0.35 * normalize(pops) + 0.3 * normalize(wind)
    return _circ(x, F.hp(45.0), F.peak(3300.0, -3.0, 0.9), F.highshelf(7000.0, -4.0))


@cue('World.WatchFire.Dry', variations=2, att='Near', cls='Effects', jitter=0.04, conc=1, level=-15.0)
def dry(v, r):
    # Nothing to burn: a knuckle rapped on the empty cast-iron bowl (a dull ring of its dense modes), and the cold ash in
    # it shifting with a soft, dry hiss.
    bowl = M.plate(310.0 * jitter(r, 1, 0.08), child(r, 'bowl'), 20, 0.45, 0.6, 1.0, 0.03, 0.8)
    rap = normalize(M.strike(bowl, M.hammer(0.0014), 0.6))
    rap = F.filt(rap, F.lp(2600.0, 0.7), extend=False)
    rap2 = normalize(M.strike(bowl, M.hammer(0.0016), 0.5))
    rap2 = F.filt(rap2, F.lp(2400.0, 0.7), extend=False)
    knuckle = kit.thunk(600.0 * jitter(r, 1, 0.06), child(r, 'knuckle'), 0.02, 0.0012, 4, 0.06)
    ash = G.grit(0.4, child(r, 'ash'), 2200.0, 400.0, 2600.0, 1.2, 3) * E.ar(0.4, 0.06, 0.25, 1.5)
    gap = 0.12 + 0.03 * r.random()
    x = layers((rap, 0.0, 0.0), (knuckle, 0.0, -10.0), (rap2, gap, -3.0), (knuckle, gap, -12.0),
               (normalize(ash), gap + 0.04, -12.0))
    x = F.filt(x, F.hp(70.0), extend=False)
    return _outside(x, r, -22.0)
