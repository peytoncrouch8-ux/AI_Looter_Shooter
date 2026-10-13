"""Respawn graves as save points (ARespawnMarker, marked on approach): the grave's lantern catching as the player walks up,
and lifting again when a grave marked before becomes the save point once more. Played at the lantern (Creature
attenuation: the mark happens within 6 m, and it should reach the player whole).

A lantern catching is a flame, not a blast: combustion's soft roar (bosses.py's flame) swelling up in the first tenth of a
second, as the lantern's light does (GraveMarkRules::FlareRiseSeconds), and easing back over the flare's length, with the
wick's small crackle. The warmth under it is a bell's hum rather than a bell's strike: a low cast bell rung with a felt
mallet, so only its hum, prime and minor-third tierce sing (the tierce keeps it a little eerie), its level blooming in
like a reversed ring and fading slowly. The Gravewind settles round it: a breath of air falling in pitch and a whisper,
the dead's, fading with it. The whole sits in creatures.py's haunted open-air wash, lighter than the Unpaid's.

- World.Grave.Marked: the lantern catching with a breath of flame, the low warm swell, the Gravewind settling (1.1 s of
  flare, 2-3 s with its tails). Satisfying, a little eerie.
- World.Grave.Remarked: the flame lifting (0.7 s), a faint hum, a breath of wind: the same voice, quieter and shorter.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, modal as M, voice as V
from recipes import cue
from recipes import kit
from recipes.bosses import _flame, _crackle
from recipes.creatures import _haunt, _cold

# The hum of each take's bell (Hz): D3, C3 and E3, the score's key and its neighbours.
HUMS = (146.8, 130.8, 164.8)

RISE = 0.09  # GraveMarkRules::FlareRiseSeconds: the light peaks this soon, so the flame does too.


def _catch(r, dur, fc=(300.0, 950.0, 380.0)):
    """The lantern's flame catching: a soft puff of air as the wick takes, the flame's roar swelling to its peak at RISE
    and easing back over dur, the wick crackling as it settles."""
    puff = kit.noise_thump(0.16, child(r, 'puff'), 750.0, 120.0, 0.07, 0.9, 0.05, 0.0015)
    roar = _flame(dur, child(r, 'roar'), [(0.0, fc[0]), (RISE, fc[1] * jitter(r, 1, 0.08)), (dur, fc[2])], 20.0, 0.55, 1.1)
    n = ns(dur)
    # Up fast, then easing down to a small steady flame that fades out at the end.
    env = E.bp([(0.0, 0.25), (RISE, 1.0), (dur * 0.55, 0.38), (dur * 0.85, 0.2), (dur, 0.0)], n=n, curve='cos')
    roar = normalize(roar * env)
    wick = _crackle(dur, child(r, 'wick'), 60.0, dur * 0.4, 1500.0, 4500.0, 4000.0)
    return layers((puff, 0.0, -2.0), (roar, 0.0, -1.0), (wick, 0.02, -20.0))


def _swell(hum, dur, r, peak_at=0.38, t60=2.6):
    """A low warm bell-like swell: a cast bell's hum, prime and tierce rung softly (a felt mallet's long contact wakes
    little of the upper partials), its level blooming in to peak_at like a reversed ring and dying slowly; a pair of
    sines on the hum and the fifth, slowly beating, round it out."""
    m = M.bell(hum * 2.0, child(r, 'bell'), t60=t60, beat=0.5, bright=0.55, count=7)
    ring = M.strike(m, M.hammer(0.0016), dur)[:ns(dur)]
    ring = F.filt(normalize(ring), F.lp(1800.0, 0.6), extend=False)
    n = ring.size
    t = times(n)
    pad = (np.sin(2 * np.pi * hum * t) + np.sin(2 * np.pi * (hum + 0.45) * t + 1.0)
           + 0.5 * np.sin(2 * np.pi * hum * 1.5 * t + 2.0) + 0.5 * np.sin(2 * np.pi * (hum * 1.5 + 0.6) * t))
    x = layers((normalize(ring), 0.0, 0.0), (normalize(pad), 0.0, -7.0))
    return normalize(x * E.swell(dur, peak_at, t60 * 0.55, 1.8))


def _settle(dur, r, f0=1100.0, f1=260.0, whisper_db=-8.0):
    """The Gravewind settling round the grave: a breath of air falling in pitch and dying away, and in it a faint whisper,
    the dead's, cold and not quite of this world."""
    wind = kit.whoosh(dur, child(r, 'wind'), f0, f1, 0.8, 0.22, -3.0, 1.0)
    n = ns(dur)
    w = V.whisper(dur, [(0.0, 'h'), (dur * 0.3, 'u'), (dur * 0.7, 'o'), (dur, 'h')], child(r, 'whisper'),
                  r.uniform(0.9, 1.05))
    w = _cold(w, child(r, 'cold'), 31.0, 0.25) * E.swell(dur, dur * 0.3, dur * 0.6, 1.5)
    w = F.filt(w, F.hp(350.0), F.lp(4200.0), extend=False)
    return layers((wind, 0.0, 0.0), (normalize(w), 0.0, whisper_db))


@cue('World.Grave.Marked', variations=3, att='Creature', cls='Effects', jitter=0.02, conc=1, level=-9.0)
def grave_marked(v, r):
    # Walking up to a grave: its lantern catches with a breath of flame, flaring and settling over the light's 1.1 s; a low
    # warm bell-like hum blooms up under it and hangs; the Gravewind settles round the grave with a faint whisper in it.
    hum = HUMS[v] * jitter(r, 1, 0.01)
    flame = _catch(child(r, 'catch'), 1.25)
    swell = _swell(hum, 3.0, child(r, 'swell'), 0.36 + 0.06 * r.random(), 2.6)
    settle = _settle(1.7, child(r, 'settle'), 1150.0 * jitter(r, 1, 0.06), 260.0)
    x = layers((flame, 0.0, 0.0), (swell, 0.03, -3.0), (settle, 0.14 + 0.04 * r.random(), -8.0))
    x = F.filt(x, F.hp(55.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _haunt(x, r, -13.0, 2.0, 3000.0)


@cue('World.Grave.Remarked', variations=3, att='Creature', cls='Effects', jitter=0.03, conc=1, level=-15.0)
def grave_remarked(v, r):
    # A grave marked before made the save point again: the flame lifts for its 0.7 s and settles, a faint warm hum under
    # it, a breath of wind.
    hum = HUMS[v] * jitter(r, 1, 0.01)
    flame = _catch(child(r, "lift"), 0.8, (320.0, 760.0, 360.0))
    swell = _swell(hum, 1.8, child(r, 'swell'), 0.25, 1.4)
    breath = _settle(0.9, child(r, 'breath'), 900.0, 300.0, -12.0)
    x = layers((flame, 0.0, 0.0), (swell, 0.02, -9.0), (breath, 0.08, -15.0))
    x = F.filt(x, F.hp(60.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _haunt(x, r, -15.0, 1.6, 3000.0)
