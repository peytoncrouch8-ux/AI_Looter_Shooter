"""Spent brass and bullets going by (all 3D, quiet, heard over and over in a fight).

Casings (Weapon.Casing.<surface>, played where the case lands: UBulletSubsystem/FWeaponBrass pick the surface): a case is
a thin brass tube closed at one end. Struck, it rings a few near-harmonic bending modes (a short, bright "tink") over a
muted lower note of the whole case flexing, and it tumbles, so every landing rings a different pitch. It bounces: each hit
is softer than the one before by the restitution, and the gaps shrink by the same factor (a ball's bounce, e^k), so
"tink... tink.tink" falls out of one rule. What the surface adds is the rest:
- Stone: a hard dry knock under a long, bright ring (3-4 bounces).
- Wood: a hollow board (few irregular low modes, a short thump) under a muted ring: a tock, then a softer tick.
- Metal: the sheet rings clearly: the case's own ring, long and shimmering (each partial a beating pair) over the sheet's
  low note.
- Dirt (also grass and sand): the ground takes the energy: a dull thud and a ring that barely speaks.
- Water: a tiny bubble's plip, a speck of splash, a smaller bubble as the case sinks, the ripples lapping out.
- Hull: a paper shotgun hull: its hollow tube clacks low, the brass head clinks, then the hull rolls a short way, its
  ticks slowing over a soft hush of rolling.
They repeat by the dozen in a burst, so the rings sit above 4.5 kHz (the case's bending modes) and the 2-5 kHz band is
kept modest: the check holds them to it (check.py's FREQUENT list). Attenuation 'Near' (about 15 m); mixed far under the
shot. Every one ends on something that decays over 30 ms or more: a tail that falls 60 dB in 20 ms leaves the last 10 ms
looking cut off to the check (CUT TAIL), since it measures the last 10 ms against the peak.

Fly-bys (Weapon.NearMiss.*, played at the bullet's closest point to the player's head, a few metres off; NearMiss.cpp):
- Whiz: a bullet past the ear: the crack on its front (a short N-shaped pressure pulse), then the whine of air round it
  falling in pitch as it leaves (narrow noise on a falling pitch, tumbling), and the air it shoves aside.
- Hiss: a venom glob going by: slower, so no crack: a wet rush of air through the liquid's sputter, its pitch falling
  as it passes, droplets flicking off it.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a
from lib import noise as N, env as E, osc as O, filters as F, modal as M, granular as G
from recipes import cue
from recipes import kit

# A brass case's bending modes against its lowest (a thin tube closed at one end: near-harmonic with strays).
CASE = np.array([1.0, 2.04, 2.97, 3.95, 5.12])


def _case(r, f0, t60=0.08, contact=0.00006, body_db=-6.0):
    """A brass case struck once: its ring (modes from f0 up, higher ones dying first) over a muted lower note of the
    whole case flexing."""
    k = CASE * (1.0 + 0.015 * (2.0 * r.random(CASE.size) - 1.0))
    f = np.append(f0 * k, f0 * 0.43 * jitter(r, 1, 0.1))
    a = np.append(k ** -0.9 * (0.6 + 0.4 * r.random(k.size)), float(db2a(body_db)))
    t = np.append(t60 * k ** -0.4, t60 * 0.4)
    return normalize(M.strike((f, t, a), M.hammer(contact), t60 * 1.8 + 0.005), 0.0)


def _bounces(r, count, gap0, restitution=(0.5, 0.65)):
    """When a bouncing case lands and how hard (dB): each hit softer by the restitution, each gap shorter by it."""
    e = r.uniform(*restitution)
    out = []
    t = 0.0
    for k in range(count):
        out.append((t, 20.0 * np.log10(e ** k)))
        t += gap0 * jitter(r, 1, 0.2) * e ** k
    return out


def _ease(x, cut_db=-4.0):
    """The ear's touchy band eased, the rumble and the very top taken off."""
    return F.filt(x, F.hp(300.0, 0.7), F.peak(3300.0, cut_db, 0.8), F.highshelf(11000.0, -3.0), extend=False)


# --- Casings ----------------------------------------------------------------------------------------------------------

@cue('Weapon.Casing.Stone', variations=8, att='Near', jitter=0.07, conc=4, level=-18.0)
def casing_stone(v, r):
    # Brass on rock or gravel: a bright high tink (the case's ring, long on a hard surface), a dry knock of stone under
    # it, then a quick second bounce, a third and sometimes a fourth, each softer and closer than the last.
    out = np.zeros(1)
    for i, (t, db) in enumerate(_bounces(child(r, 'b'), 3 + int(v % 3 == 0), 0.125)):
        rr = child(r, 'hit', i)
        ring = _case(rr, rr.uniform(5200.0, 6800.0), 0.1 * jitter(rr, 1, 0.2), 0.00005)
        m = M.parts(900.0, 2600.0, child(rr, 'stone'), 3, 0.012, 0.005, 0.4)
        knock = normalize(M.strike(m, M.hammer(0.00008), 0.04), 0.0)
        out = layers(out, (layers((ring, 0.0, 0.0), (knock, 0.0, -8.0)), t, db))
    return _ease(out, -5.0)


@cue('Weapon.Casing.Wood', variations=6, att='Near', jitter=0.07, conc=4, level=-19.0)
def casing_wood(v, r):
    # Brass on boards: a hollow woody tock (the board's few low modes and a thump of the joist under it, the brass ring
    # muted by the soft surface), then a softer, lighter tick as it bounces, and a last small one.
    out = np.zeros(1)
    for i, (t, db) in enumerate(_bounces(child(r, 'b'), 3, 0.13, (0.45, 0.6))):
        rr = child(r, 'hit', i)
        f0 = rr.uniform(330.0, 560.0) * (1.0 + 0.12 * i)
        board = normalize(M.strike(M.wood(f0, child(rr, 'board'), t60=0.09 / (1.0 + i), count=7, spread=0.1), M.hammer(0.0004),
                                   0.2), 0.0)
        joist = kit.thump(f0 * 0.55, f0 * 0.4, 0.08, 0.045, attack=0.001)
        ring = _case(rr, rr.uniform(4600.0, 6400.0), 0.05, 0.0001)
        out = layers(out, (layers((board, 0.0, 0.0), (joist, 0.0, -8.0), (ring, 0.0, -5.0)), t, db * 1.2))
    return _ease(F.filt(out, F.lp(10000.0, 0.7), extend=False), -4.0)


@cue('Weapon.Casing.Metal', variations=6, att='Near', jitter=0.06, conc=4, level=-16.0)
def casing_metal(v, r):
    # Brass on steel or tin: a clear ringing ting. The ring is the case's own (a few partials, each a pair split by a
    # few tens of Hz so it shimmers as it dies) lifted by the sheet's hard, ringing surface, over the sheet's low note;
    # a smaller, quicker bounce follows.
    f0 = [5100.0, 5500.0, 5900.0, 6300.0, 5300.0, 6600.0][v] * jitter(r, 1, 0.03)
    t60 = 0.55 * jitter(r, 1, 0.12)

    def ting(rr, scale):
        f, d, a = [], [], []
        for k, (ratio, amp, share) in enumerate([(1.0, 1.0, 1.0), (1.52, 0.55, 0.7), (2.06, 0.35, 0.5), (2.74, 0.2, 0.35)]):
            base = f0 * ratio * jitter(rr, 1, 0.01)
            for s in (-0.5, 0.5):
                f.append(base + s * rr.uniform(16.0, 42.0) * (1.0 + 0.3 * k))
                d.append(t60 * share * scale)
                a.append(amp * (0.65 if s < 0 else 0.35))
        sheet = M.plate(rr.uniform(1100.0, 1500.0), child(rr, 'sheet'), 5, 0.16 * scale, 0.5, tilt=0.8)
        m = M.merge(M.modes(f, d, a), M.scale(sheet, 1.0, 1.0, 0.5))
        return normalize(M.strike(m, M.hammer(0.00005), t60 * 1.8), 0.0)
    out = layers((ting(child(r, 'h1'), 1.0), 0.0, 0.0),
                 (ting(child(r, 'h2'), 0.5), 0.09 + 0.03 * r.random(), -8.0))
    return _ease(out, -4.0)


@cue('Weapon.Casing.Dirt', variations=6, att='Near', jitter=0.08, conc=3, level=-27.0)
def casing_dirt(v, r):
    # A case into dirt, grass or sand: the ground takes all of it, a dull muffled thud with a faint dry tick of brass
    # through it, a speck of grit and the case settling. Barely there.
    thud = kit.noise_thump(0.12, child(r, 'thud'), r.uniform(380.0, 650.0), 110.0, 0.05, 1.0, 0.05, 0.0012)
    tick = F.filt(_case(child(r, 'case'), r.uniform(4200.0, 5600.0), 0.03, 0.0002), F.lp(3600.0, 0.7), extend=False)
    grit = kit.grit_burst(0.1, child(r, 'grit'), 2500.0, 700.0, 3000.0, 0.06)
    settle = kit.noise_thump(0.1, child(r, 'settle'), 300.0, 100.0, 0.05, 1.0, 0.05, 0.001)
    out = layers((thud, 0.0, 0.0), (tick, 0.0, -14.0), (grit, 0.004, -15.0), (settle, 0.07 + 0.03 * r.random(), -9.0))
    return _ease(F.filt(out, F.lp(5000.0, 0.7), extend=False), -3.0)


@cue('Weapon.Casing.Water', variations=4, att='Near', jitter=0.08, conc=2, level=-24.0)
def casing_water(v, r):
    # A case dropping into water: a tiny bubble's plip (the cavity it opens rings and closes), a speck of splash round
    # it, a smaller bubble as the case sinks away, and ripples lapping out.
    plip = normalize(kit.bubble(r.uniform(950.0, 1500.0), child(r, 'plip'), 0.2), 0.0)
    pip2 = normalize(kit.bubble(r.uniform(1700.0, 2600.0), child(r, 'pip2'), 0.15), 0.0)
    n = ns(0.04)
    splash = N.band(n, child(r, 'splash'), 1500.0, 7000.0) * E.perc(0.04, 0.0008, 0.02)
    sink = normalize(kit.bubble(r.uniform(520.0, 900.0), child(r, 'sink'), 0.2), 0.0)
    m = ns(0.24)
    ripple = N.band(m, child(r, 'ripple'), 500.0, 2600.0) * (0.45 + 0.55 * np.abs(N.smooth_random(m, child(r, 'lap'), 38.0)))
    ripple = normalize(ripple * E.perc(0.24, 0.01, 0.2))
    # The last thing in it is the ripples' slow fade, never a bubble: a bubble's ring falls 60 dB in about 15 ms and would
    # leave the cue looking cut off.
    out = layers((plip, 0.0, 0.0), (pip2, 0.004 + 0.004 * r.random(), -7.0), (normalize(splash), 0.0, -9.0),
                 (sink, 0.07 + 0.04 * r.random(), -10.0), (ripple, 0.02, -17.0),
                 (kit.bubble(r.uniform(1200.0, 2000.0), child(r, 'pip3'), 0.15), 0.12 + 0.03 * r.random(), -16.0))
    return _ease(out, -3.0)


@cue('Weapon.Casing.Hull', variations=6, att='Near', jitter=0.07, conc=3, level=-19.0)
def casing_hull(v, r):
    # A paper shotgun hull on hard ground: the tube's hollow low clack (a paper cylinder: a few low modes, quickly
    # damped), the brass head's clink (a thick disc: a short, higher ring), then it rolls a little way, its ticks slowing
    # over a soft hush of rolling.
    f0 = r.uniform(340.0, 520.0)
    tube = normalize(M.strike(M.wood(f0, child(r, 'tube'), t60=0.06, count=5, spread=0.1), M.hammer(0.0006), 0.14), 0.0)
    thump = kit.thump(f0 * 0.6, f0 * 0.35, 0.09, 0.05, attack=0.001)
    clink = _case(child(r, 'head'), r.uniform(4800.0, 6200.0), 0.07, 0.00008)
    roll = np.zeros(1)
    rr = child(r, 'roll')
    t = 0.07 + 0.03 * rr.random()
    count = 5 + int(rr.integers(0, 4))
    start = t
    for i in range(count):
        u = i / max(count - 1, 1)
        tick = kit.thunk(rr.uniform(190.0, 320.0), child(rr, 'tick', i), 0.02, 0.0007, 3, 0.1)
        roll = layers(roll, (tick, t, -8.0 - 3.0 * i))
        if i % 3 == 0:
            roll = layers(roll, (_case(child(rr, 'c', i), rr.uniform(4800.0, 6000.0), 0.03, 0.0001), t, -22.0 - 2.0 * i))
        t += (0.022 + 0.07 * u ** 1.4) * rr.uniform(0.85, 1.2)
    span = t - start + 0.06
    m = ns(span)
    hush = N.band(m, child(rr, 'hush'), 250.0, 1500.0) * (0.6 + 0.4 * np.abs(N.smooth_random(m, child(rr, 'hm'), 30.0)))
    hush = normalize(hush * E.perc(span, 0.02, span * 0.9))
    out = layers((tube, 0.0, 0.0), (thump, 0.0, -7.0), (clink, 0.0, -10.0), (roll, 0.0, 0.0), (hush, start - 0.02, -18.0))
    return _ease(out, -4.0)


# --- Fly-bys ------------------------------------------------------------------------------------------------------------

@cue('Weapon.NearMiss.Whiz', variations=6, att='Near', jitter=0.05, conc=3, level=-9.0)
def nearmiss_whiz(v, r):
    # A bullet past the ear: the crack on its front, the whine of air round it falling as it leaves (narrow noise on a
    # falling pitch with a thin tone under it, tumbling), and the air it shoves aside. Pitches and lengths vary by take.
    dur = [0.2, 0.26, 0.17, 0.3, 0.22, 0.24][v] * jitter(r, 1, 0.06)
    n = ns(dur)
    f0 = [2300.0, 2150.0, 2450.0, 1950.0, 2250.0, 2050.0][v] * jitter(r, 1, 0.04)
    f = O.expsweep(f0, f0 * r.uniform(0.34, 0.42), dur, 2.2, n=n) * (1.0 + 0.01 * N.smooth_random(n, child(r, 'wob'), 30.0))
    tumble = 1.0 - 0.4 * np.abs(N.smooth_random(n, child(r, 'tumble'), r.uniform(60.0, 90.0))) ** 0.7
    air = F.sweep(N.white(n, child(r, 'air')), 'bp', f, 5.0)
    whine = (normalize(air) + 0.3 * np.sin(2.0 * np.pi * O.phase(f, n))) * tumble
    whine = F.sweep(whine, 'lp', np.geomspace(9000.0, 2800.0, n), 0.7)
    whine = normalize(whine * E.bp([(0.0, 0.0), (0.004, 0.9), (0.03, 1.0), (dur * 0.5, 0.35), (dur, 0.0)], n=n, curve='cos'))
    crack = F.filt(kit.nwave(0.0006 * jitter(r, 1, 0.15)), F.hp(500.0, 0.7), extend=False)
    shove = kit.whoosh(dur, child(r, 'shove'), 1100.0, 280.0, 0.8, 0.12, -3.0)
    x = layers((crack, 0.0, 0.0), (whine, 0.0, -7.0), (shove, 0.0, -15.0))
    return F.filt(x, F.peak(3300.0, -4.0, 0.7), F.hp(350.0, 0.7), extend=False)


@cue('Weapon.NearMiss.Hiss', variations=5, att='Near', jitter=0.05, conc=2, level=-13.0)
def nearmiss_hiss(v, r):
    # A venom glob rushing past: no crack, it's slow. A wet hiss through the liquid's sputter (noise through a band that
    # slides down as the glob leaves, chopped fast and irregularly), the thick low rush of the air it pushes, and
    # droplets flicking off it, fewer as it goes. Quick to arrive, long to leave.
    dur = [0.38, 0.45, 0.33, 0.5, 0.41][v] * jitter(r, 1, 0.06)
    n = ns(dur)
    fc = O.expsweep(2300.0 * jitter(r, 1, 0.08), 800.0, dur, 1.7, n=n)
    sputter = 0.25 + 0.75 * np.clip(np.abs(N.smooth_random(n, child(r, 'sp'), 85.0)) * 2.0, 0.0, 1.0)
    hiss = F.sweep(N.white(n, child(r, 'hiss')), 'bp', fc, 2.2) + 0.35 * F.sweep(N.white(n, child(r, 'h2')), 'bp', fc * 1.6, 2.6)
    hiss = normalize(hiss * sputter * kit.turbulence(n, child(r, 'turb'), 160.0, 0.6))
    rush = N.pink(n, child(r, 'rush'), lo=150.0, hi=1100.0) * kit.turbulence(n, child(r, 'rt'), 60.0, 0.7)
    rush = F.sweep(rush, 'lp', O.expsweep(900.0, 260.0, dur, 1.4, n=n), 0.8)
    env = E.bp([(0.0, 0.0), (0.004, 0.7), (0.05, 1.0), (dur * 0.3, 0.75), (dur, 0.0)], n=n, curve='cos')
    drops = np.zeros(1)
    dr = child(r, 'drops')
    for tt in N.times_poisson(dur, dr, lambda u: 60.0 * np.exp(-u / 0.15), 0.01):
        drops = layers(drops, (kit.bubble(dr.uniform(900.0, 2200.0), dr, 0.12), tt, dr.uniform(-8.0, 0.0) - 25.0 * tt))
    wet = kit.burst(0.006, child(r, 'wet'), 0.003, lo=700.0, hi=4500.0, attack=0.0003)
    x = layers((wet, 0.0, -8.0), (hiss * env, 0.0, 0.0), (normalize(rush) * env, 0.0, -5.0), (normalize(drops), 0.0, -14.0))
    return F.filt(x, F.peak(3300.0, -4.0, 0.7), F.hp(250.0, 0.7), F.highshelf(9000.0, -4.0), extend=False)
