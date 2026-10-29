"""Things found off the path (UFindSubsystem, Docs/Work/2026-10-28-Core/FINDS.md; the names are `FindCue` in World/FindRules.h).
The graveyard's own voice: a struck tin bell like the pail hung on a crossroads post, a low plucked open fifth under it as
if from a guitar left in a doorway, and a crow somewhere far off. Played in key D (the score's, and the roosts' beacon hum).

- World.Find.Discover (2D, 1.6 s): the find counts. The tin bell struck once (each take a different note of a D triad, D F#
  A, so finds heard in a row are not the same sound), a low open fifth (D and A) plucked under it, a crow's far caw after.
- World.Find.Complete (2D, 2.4 s): the last find of the area. The same a fourth up (G and D), a second, higher bell a beat
  behind the first, two caws.
- World.Find.Glint (3D, near, 0.2 s): an unfound find's metal catching the light as you come near: a faint, high tink.
  Quiet; it comes every few seconds while one is in view, so it must never tire.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, modal as M
from recipes import cue
from recipes import kit
from recipes.fauna import _caw, _outdoors

# A struck tin cup's partials against its strike note: (ratio, strength, share of the ring time). A thin tin wall rings
# bright and short; the upper partials are not harmonic and die first.
TIN = [(1.0, 1.0, 1.0), (2.28, 0.55, 0.6), (3.86, 0.38, 0.4), (5.61, 0.22, 0.26), (7.72, 0.12, 0.16)]

D3 = 146.83
G2 = 98.0


def _tin_bell(f0, r, t60=1.2, beat=1.4, bright=1.0):
    """A tin bell struck once: each partial a slowly beating pair (tin is never quite round), the clapper's click on top."""
    f, d, a = [], [], []
    for i, (ratio, amp, share) in enumerate(TIN):
        base = f0 * ratio * (1.0 + 0.003 * (2 * r.random() - 1))
        for s in (-0.5, 0.5):
            f.append(base + s * beat * (1.0 + 0.6 * i) * (0.6 + 0.8 * r.random()))
            d.append(t60 * share)
            a.append(0.5 * amp * (bright ** i))
    ring = M.strike((np.array(f), np.array(d), np.array(a)), M.hammer(0.0002), t60 * 1.1)
    tick = kit.burst(0.003, child(r, 'tick'), 0.0015, lo=3000.0, hi=9000.0)
    return normalize(layers(normalize(ring), (tick, 0.0, -20.0)), 0.0)


def _open_fifth(root, dur, r, t60=1.8):
    """A low open fifth, strummed: the root, and the fifth above it a hair after, on a wooden body."""
    low = kit.twang(root, dur, child(r, 'root'), 0.5, t60, 0.2)
    high = kit.twang(root * 1.5, dur, child(r, 'fifth'), 0.45, t60 * 0.85, 0.18)
    x = layers((low, 0.0, 0.0), (high, 0.04 + 0.01 * r.random(), -3.0))
    return normalize(F.filt(x, F.lp(2600.0, 0.7), extend=False), 0.0)


def _far_caw(r, f0, dur=0.3, rel_db=-8.0):
    """A crow a long way off: its caw dulled by the distance (the highs gone), the open ground giving it back."""
    c = _caw(child(r, 'caw'), f0, dur, 0.5)
    c = F.filt(c, F.hp(300.0), F.lp(2300.0, 0.7), extend=False)
    return _outdoors(c, r, rel_db, 1.6, 2500.0)


@cue('World.Find.Discover', variations=3, space='2D', jitter=0.02, conc=1, level=-8.0)
def discover(v, r):
    # A find counts: the tin bell on a note of the D triad, the open fifth plucked under it, a crow's far caw after.
    bell = _tin_bell((587.33, 739.99, 880.0)[v] * jitter(r, 1, 0.003), child(r, 'bell'), 1.3)
    fifth = _open_fifth(D3, 1.6, child(r, 'fifth'))
    caw = _far_caw(child(r, 'far'), r.uniform(390.0, 470.0), r.uniform(0.26, 0.34))
    x = layers((fifth, 0.0, -5.0), (bell, 0.03, 0.0), (caw, 0.62 + 0.12 * r.random(), -13.0))
    return F.filt(x, F.hp(60.0), F.peak(3300.0, -2.0, 0.8), extend=False)


@cue('World.Find.Complete', variations=2, space='2D', jitter=0.02, conc=1, level=-6.0)
def complete(v, r):
    # Every find in the area: the same, a fourth up (G and D, the bell on G), a second bell a fifth above the first a beat
    # behind it, and two crows answering each other, farther and nearer.
    bell = _tin_bell((783.99, 783.99 * 1.01)[v] * jitter(r, 1, 0.003), child(r, 'bell'), 1.7)
    answer = _tin_bell(1174.66 * jitter(r, 1, 0.003), child(r, 'answer'), 1.5, 1.8, 0.9)
    fifth = _open_fifth(G2, 2.4, child(r, 'fifth'), 2.6)
    caw1 = _far_caw(child(r, 'far1'), r.uniform(400.0, 460.0), r.uniform(0.28, 0.34))
    caw2 = _far_caw(child(r, 'far2'), r.uniform(340.0, 400.0), r.uniform(0.3, 0.38), -6.0)
    x = layers((fifth, 0.0, -5.0), (bell, 0.03, 0.0), (answer, 0.26, -4.0), (caw1, 0.95, -13.0), (caw2, 1.35 + 0.1 * r.random(), -15.0))
    return F.filt(x, F.hp(50.0), F.peak(3300.0, -2.0, 0.8), extend=False)


@cue('World.Find.Glint', variations=4, att='Near', jitter=0.08, conc=2, level=-22.0)
def glint(v, r):
    # Metal catching the light: one faint, high tink (a small chime's bright partials, shortened), nothing under it.
    f0 = r.uniform(4200.0, 5200.0)
    x = kit.chime(f0, child(r, 'tink'), 0.18, 0.7, 0.00008, 2.0, -26.0)
    return F.filt(x, F.hp(2200.0), F.lp(10500.0), extend=False)
