"""Granular textures: many tiny events scattered in time (grit, gravel, splinters, droplets, rattles, sizzles)."""
import numpy as np

from .core import SR, ns, times, add_at
from .noise import dust, times_poisson
from . import filters as F


def cloud(dur, r, rate, grain, env=None, start=0.0, min_gap=0.0):
    """Grains at random times: grain(r, t, i) makes each one (a short signal); rate is per second (a number or a
    function of time); env(t) scales each grain by when it falls."""
    out = np.zeros(ns(dur))
    for i, t in enumerate(times_poisson(dur, r, rate, start, min_gap)):
        g = grain(r, t, i)
        if g is None:
            continue
        a = env(t) if env is not None else 1.0
        out = add_at(out, g * a, t)
    return out


def grit(dur, r, rate, lo=1500.0, hi=6000.0, q=2.0, bands=4, env=None, amp=(0.15, 1.0), periodic=False):
    """Fine grit: random impulses rung through a few band-passes spread between lo and hi (each grain catches one
    band, like stones of different sizes). rate: per second, a number or an array per sample; env scales it in time."""
    n = ns(dur)
    t = times(n)
    rate = np.broadcast_to(np.asarray(rate, dtype=float), (n,)).copy()
    centers = np.geomspace(lo, hi, bands) if bands > 1 else np.array([np.sqrt(lo * hi)])
    out = np.zeros(n)
    for c in centers:
        d = dust(n, r, rate / bands, amp)
        cc = c * (1.0 + 0.08 * (2 * r.random() - 1))
        out += F.filt(d, F.bp(cc, q), circular=periodic) if periodic else F.filt(d, F.bp(cc, q), extend=False)
    if env is not None:
        out *= np.broadcast_to(np.asarray(env, dtype=float), (n,))
    return out


def rattle(dur, r, count, spread, grain, start=0.0, decay=0.6):
    """count small hits clustered after start over about spread seconds, each weaker on average (a settling rattle):
    grain(r, i) makes each hit. The result runs as long as the last hit rings (dur is the shortest it will be)."""
    out = np.zeros(ns(dur))
    t = start
    for i in range(count):
        t += spread / count * r.uniform(0.3, 1.7)
        a = (decay ** i) * r.uniform(0.6, 1.0)
        g = grain(r, i)
        out = add_at(out, g * a, t)
    return out
