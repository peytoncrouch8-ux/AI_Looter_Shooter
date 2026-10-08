"""Bullets meeting the world: ground and stone, wood, metal, water. Each is the strike itself (a hard tick), what the
material does with it (dirt thuds and sprays, stone chips, wood knocks and splinters, metal rings, water splashes and
bubbles), sometimes a ricochet whining off, and a short slap-back from the surroundings.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, granular as G
from recipes import cue
from recipes import kit


def _slap(x, r, rel_db=-20.0, s1=0.07, s2=0.17):
    """A short outdoor answer, so a hit sits in the land rather than in a void."""
    ir = R.outdoor(child(r, 'ir'), dur=0.6, slaps=((s1, -4.0), (s2, -9.0)), tail_t60=0.4, tail_db=-12.0,
                   tail_start=0.02, tail_peak=0.08)
    wet = F.convolve(F.filt(x, F.hpn(250.0, 2), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.02))


def _pebbles(dur, r, count, start, lo=1800.0, hi=6000.0):
    """Bits of grit and stone landing after the hit: a few tiny, scattered ticks."""
    def grain(rr, i):
        m = M.parts(lo, hi, rr, 4, 0.012, 0.006)
        return normalize(M.strike(m, M.hammer(0.00006), 0.03))
    return G.rattle(dur, child(r, 'pebbles'), count, dur - start, grain, start, 0.8)


@cue('Impact.World', variations=6, att='Creature', jitter=0.06, conc=8, level=-10.0)
def impact_world(v, r):
    stone = v >= 3
    tick = kit.burst(0.004, child(r, 'tick'), 0.0015, lo=2000.0, hi=12000.0)
    thud = kit.noise_thump(0.12, child(r, 'thud'), 900.0, 120.0, 0.05, 1.0, 0.03)
    spray = kit.grit_burst(0.35, child(r, 'spray'), 7000.0, 1100.0, 5000.0, 0.22)
    puff = kit.burst(0.09, child(r, 'puff'), 0.045, lo=200.0, hi=1600.0, color=-3.0)
    parts = [(tick, 0.0, -2.0), (thud, 0.0, 0.0), (spray, 0.003, -9.0), (puff, 0.001, -10.0),
             (_pebbles(0.4, r, 4 + int(r.integers(0, 4)), 0.08), 0.0, -22.0)]
    if stone:
        # Stone is dense and rings short and high when chipped.
        chip = normalize(M.strike(M.parts(1700.0, 6500.0, child(r, 'chip'), 7, 0.03, 0.012), M.hammer(0.00005), 0.07))
        crack = kit.burst(0.02, child(r, 'crack'), 0.01, lo=900.0, hi=6000.0)
        parts += [(chip, 0.0, -6.0), (crack, 0.0, -6.0)]
    if v in (2, 5):
        ric = kit.ricochet(0.45, child(r, 'ric'), 4300.0 * jitter(r, 1, 0.1), 1800.0 * jitter(r, 1, 0.1), 40.0 + 20 * r.random())
        parts.append((F.filt(ric, F.peak(3500.0, -4.0, 1.0), extend=False), 0.006, -10.0))
    return _slap(layers(*parts), r, -21.0)


@cue('Impact.Wood', variations=6, att='Creature', jitter=0.06, conc=8, level=-10.0)
def impact_wood(v, r):
    # A board or a post: a sharp crack, the plank's hollow knock, its air cavity, splinters flying.
    crack = kit.burst(0.006, child(r, 'crack'), 0.003, lo=1500.0, hi=9000.0)
    f0 = 230.0 + 120.0 * r.random()
    knock = kit.thunk(f0, child(r, 'knock'), 0.11 * jitter(r, 1, 0.2), 0.00015, 7, 0.08)
    hollow = kit.thump(f0 * 0.6, f0 * 0.45, 0.12, 0.07)
    splinters = kit.grit_burst(0.2, child(r, 'splinter'), 5000.0, 2200.0, 8500.0, 0.09)
    fibers = kit.creak(0.08, child(r, 'fibers'), 900.0, 300.0, (1300.0, 2600.0, 4200.0), 4.0)
    parts = [(crack, 0.0, -4.0), (knock, 0.0, 0.0), (hollow, 0.0, -9.0), (splinters, 0.002, -13.0),
             (fibers, 0.004, -20.0), (_pebbles(0.35, r, 3, 0.07, 1200.0, 3500.0), 0.0, -26.0)]
    return _slap(layers(*parts), r, -21.0)


@cue('Impact.Metal', variations=6, att='Creature', jitter=0.05, conc=8, level=-9.0)
def impact_metal(v, r):
    # Steel plate or iron: a hard tick, the plate's bright inharmonic ring, the dull weight behind it; some whine off.
    tick = kit.burst(0.003, child(r, 'tick'), 0.0012, lo=3000.0, hi=14000.0)
    f0 = 480.0 + 420.0 * r.random()
    plate = M.plate(f0, child(r, 'plate'), 26, 0.45 * jitter(r, 1, 0.3), 0.55, tilt=0.75)
    # The lowest modes are tamed so it clangs with many partials instead of ringing one note like a bell.
    plate = (plate[0], plate[1], plate[2] * np.r_[0.6, 0.8, np.ones(plate[2].size - 2)])
    ping = normalize(M.strike(plate, M.hammer(0.00004), 0.9))
    ping = F.filt(ping, F.peak(3200.0, -9.0, 0.5), F.highshelf(6500.0, -4.0), extend=False)
    mass = kit.thunk(220.0 * jitter(r, 1, 0.15), child(r, 'mass'), 0.07, 0.0002)
    parts = [(tick, 0.0, -3.0), (normalize(ping), 0.0, 0.0), (mass, 0.0, -8.0)]
    if v in (1, 3, 5):
        ric = kit.ricochet(0.5, child(r, 'ric'), 4800.0 * jitter(r, 1, 0.1), 2000.0 * jitter(r, 1, 0.1), 50.0 + 20 * r.random())
        parts.append((F.filt(ric, F.peak(3500.0, -4.0, 1.0), extend=False), 0.004, -11.0))
    return _slap(layers(*parts), r, -22.0)


@cue('Impact.Water', variations=6, att='Creature', jitter=0.06, conc=8, level=-11.0)
def impact_water(v, r):
    # The surface slapped, the bullet's cavity closing with a low bloop, bubbles, a spray that falls back as drops.
    slap = kit.burst(0.02, child(r, 'slap'), 0.008, lo=300.0, hi=6000.0)
    bloop_n = ns(0.09)
    bloop = O.sine(O.expsweep(260.0 * jitter(r, 1, 0.15), 620.0, 0.09, 0.8)) * E.perc(0.09, 0.002, 0.07)
    entry = np.zeros(1)
    for i in range(3 + int(r.integers(0, 4))):
        entry = layers(entry, (kit.bubble(r.uniform(700.0, 2600.0), r), r.uniform(0.004, 0.05), r.uniform(-10.0, -2.0)))
    n = ns(0.45)
    spray = N.band(n, child(r, 'spray'), 1300.0, 7500.0)
    spray *= E.perc(0.45, 0.004, 0.22) * kit.turbulence(n, child(r, 'turb'), 120.0, 0.7)
    drops = np.zeros(1)
    # Drops fall back fewer and fainter as the spray thins out.
    for t in N.times_poisson(0.42, child(r, 'drops'), lambda tt: 70.0 * np.exp(-tt / 0.18), 0.06):
        drops = layers(drops, (kit.bubble(r.uniform(1400.0, 4500.0), r, 0.12), t, r.uniform(-10.0, -2.0) - 40.0 * t))
    parts = [(slap, 0.0, -3.0), (normalize(bloop), 0.006, -2.0), (normalize(entry), 0.003, -6.0),
             (normalize(spray), 0.002, -9.0), (normalize(drops), 0.0, -12.0)]
    return _slap(layers(*parts), r, -24.0)
