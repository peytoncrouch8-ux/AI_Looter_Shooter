"""Gusts on Ransom's Rest (World/Gust*): the wind swelling past the grass and the boards, and what it knocks loose.

- World.Gust.Pass (2D, the player's own ear): a gust rising in about a second and falling away over three to five. Wind
  is pressure at the ears (a low rush that swells with it), the mid body of air moving, and the grass's hiss on top, which
  brightens and flickers as the wind strengthens. The hiss's filter follows the gust and stays under about 2.8 kHz at its
  peak, so the 2-5 kHz band stays modest (it is checked like the frequent cues: gusts come every minute or so). Boards
  and eaves sing in it: edge tones, narrow resonances whose pitch climbs with the wind's speed and falls with it. Each
  side of the stereo pair is its own noise, and the gust crosses from one side to the other. The game plays it at 0.45-1
  of its volume by the gust's strength and +-6% pitch.
- World.Gust.Rattle (3D, from the thing itself): a loose shutter slammed against its frame two or three times with its
  louvre slats chattering after each blow and the hasp swinging (takes 1-2), or a hanging sign swinging on its bracket
  (takes 3-4): the chain's links clinking, the iron bracket creaking, the board knocking back. Thrown back by the false
  fronts.
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter, db2a
from lib import noise as N, env as E, filters as F, granular as G, modal as M
from recipes import cue
from recipes import kit
from recipes.world import _street


# --- The gust passing --------------------------------------------------------------------------------------------------

def _gust_curve(r, n, dur, rise, bump_at=None):
    """The gust's strength, 0..1 and back: up in `rise` seconds (slowly at first, then quickly), then a long fall that
    eases off in steps the way a gust does. A second, smaller gust may come in behind the first."""
    fall = dur - rise
    e = E.bp([(0.0, 0.0), (rise * 0.4, 0.12), (rise, 1.0), (rise + fall * 0.25, 0.55), (rise + fall * 0.55, 0.25),
              (rise + fall * 0.8, 0.08), (dur, 0.0)], n=n, curve='cos')
    if bump_at is not None:
        e = np.maximum(e, E.bp([(0.0, 0.0), (bump_at, 0.0), (bump_at + 0.7, 0.5), (bump_at + 1.6, 0.25), (dur, 0.0)],
                               n=n, curve='cos'))
    return e * np.clip(1.0 + 0.1 * N.smooth_random(n, child(r, 'flutter'), 2.5), 0.0, None)


def _wind_side(rr, n, env, low_db, hollow_db, hollow):
    """One ear's wind: pressure, the body of the air and the grass's hiss, all following the gust (the hiss most, its
    filter opening as the wind strengthens), and the boards' edge tones when given."""
    low = F.filt(N.shaped(n, child(rr, 'low'), -4.5, 70.0, 400.0), F.lp(340.0, 0.6), extend=False)
    low = normalize(low, 0.0) * env ** 1.15
    mid = F.sweep(N.shaped(n, child(rr, 'mid'), -3.0, 250.0, 2200.0), 'lp', 700.0 + 1500.0 * env, 0.7)
    mid = normalize(mid, 0.0) * env ** 1.3
    hiss = F.sweep(N.shaped(n, child(rr, 'hiss'), -1.5, 1000.0, 7000.0), 'lp', 1300.0 + 2200.0 * env, 0.7)
    flick = 0.45 + 0.55 * np.abs(N.smooth_random(n, child(rr, 'rustle'), 26.0)) ** 1.3
    hiss = normalize(hiss, 0.0) * flick * env ** 2.0
    x = low * db2a(low_db) + mid * db2a(-4.0) + hiss * db2a(-3.0)
    if hollow_db is not None:
        src = N.pink(n, child(rr, 'edge'))
        tone = np.zeros(n)
        for i, (f0, f1) in enumerate(hollow):
            fc = f0 * (f1 / f0) ** np.clip(env, 0.0, 1.0) * (1.0 + 0.05 * N.smooth_random(n, child(rr, 'w', i), 0.9))
            hw = F.sweep(F.sweep(src, 'bp', fc, 12.0, 12), 'bp', fc, 12.0, 12)
            tone += normalize(hw, 0.0) * (0.5 + 0.5 * np.abs(N.smooth_random(n, child(rr, 'a', i), 1.2))) / (1.0 + 0.5 * i)
        x = x + normalize(tone, 0.0) * db2a(hollow_db) * env ** 1.5
    return x


@cue('World.Gust.Pass', variations=4, space='2D', cls='Ambience', jitter=0.06, conc=1, level=-16.0, swell=True)
def gust_pass(v, r):
    # A gust coming through: grass hissing up on it, the low rush swelling, boards singing a little; one take rolls in
    # as plain grass (1), one has the eaves' edge tones (2), one a heavier roar (3), one a second small gust behind
    # the first (4). Crossing from one ear to the other, from the left on 1 and 3 and the right on 2 and 4.
    dur = [3.6, 4.4, 5.2, 6.0][v]
    rise = [0.9, 1.1, 1.0, 1.2][v]
    n = ns(dur)
    env = _gust_curve(child(r, 'curve'), n, dur, rise, 2.6 if v == 3 else None)
    low_db = 0.0 if v == 2 else -3.0
    hollow_db = [None, -16.0, -13.0, -19.0][v]
    hollow = [(360.0, 560.0), (540.0, 830.0)] if v != 2 else [(300.0, 520.0), (470.0, 760.0)]
    sides = [_wind_side(child(r, side), n, env, low_db, hollow_db, hollow) for side in ('left', 'right')]
    # The gust crosses the listener: constant-power pan from one ear's side to the other's.
    u = np.clip(times(n) / (dur * 0.8), 0.0, 1.0)
    p = (1.0 if v % 2 == 0 else -1.0) * 0.45 * (2.0 * (u * u * (3.0 - 2.0 * u)) - 1.0)
    a = (p + 1.0) * np.pi / 4.0
    x = np.vstack([sides[0] * np.cos(a), sides[1] * np.sin(a)]) * np.sqrt(2.0)
    return F.filt(x, F.hpn(60.0, 2), F.peak(3300.0, -3.0, 0.8), F.highshelf(6000.0, -3.0), extend=False)


# --- What the gust knocks loose ----------------------------------------------------------------------------------------

def _knock(r, f0):
    """A shutter slammed against its frame by the wind: the panel's knock, the slats, the frame's thud and the crack of
    the wood meeting."""
    panel = kit.thunk(f0, child(r, 'panel'), 0.12, 0.0007, 7, 0.1)
    slat = kit.thunk(f0 * 2.6, child(r, 'slat'), 0.045, 0.0003, 5, 0.1)
    frame = kit.noise_thump(0.12, child(r, 'frame'), 800.0, 100.0, 0.06, 1.0, 0.04)
    crack = kit.burst(0.01, child(r, 'crack'), 0.004, lo=700.0, hi=5500.0)
    return layers((panel, 0.0, 0.0), (slat, 0.002, -7.0), (frame, 0.0, -5.0), (crack, 0.0, -10.0))


def _louvre(r):
    """The louvre slats chattering for a moment after a blow: tiny quick wood taps, dying."""
    def tap(rr, i):
        return kit.thunk(rr.uniform(1500.0, 3200.0), rr, rr.uniform(0.006, 0.012), 0.0001, 3, 0.1)

    return normalize(G.rattle(0.12, child(r, 'slats'), int(r.integers(4, 8)), 0.11, tap, 0.0, 0.8), 0.0)


def _link(r, i=0):
    """One iron link knocking another: a handful of modes between 1 and 3.8 kHz (kept under the ear's sharpest band, the
    gusts come often) and the link's own dull knock under them."""
    ring = M.strike(M.parts(1000.0, 3800.0, r, count=6, t60=0.05, tilt=0.5), M.hammer(0.00015), 0.09)
    knock = kit.thunk(r.uniform(500.0, 800.0), r, 0.012, 0.0003, 3, 0.1)
    return normalize(layers(normalize(ring), (knock, 0.0, -6.0)), 0.0)


def _clinks(r, count, spread, db=0.0):
    """A chain's links knocking together as the sign swings: a short cluster that dies as the links settle."""
    return G.rattle(0.2 + spread, r, count, spread, _link, 0.0, 0.82) * float(db2a(db))


@cue('World.Gust.Rattle', variations=4, att='Creature', cls='Ambience', jitter=0.06, conc=2, level=-14.0)
def gust_rattle(v, r):
    # What the gust knocks loose, heard from across the street: a loose shutter slammed twice, its slats chattering
    # after each blow (1), slammed three times with its hasp swinging between them and the hinge squeaking (2); a hanging
    # sign swung out on its chain, clinking, the bracket creaking and the board knocking back (3), and a slower swing,
    # the bracket groaning and the chain clinking as it comes back (4).
    parts = []
    if v < 2:
        f0 = r.uniform(160.0, 220.0)
        t = 0.0
        for k in range(2 + v):
            rk = child(r, 'k', k)
            parts += [(_knock(rk, f0 * jitter(rk, 1.0, 0.05)), t, -3.0 * k), (_louvre(child(rk, 'l')), t + 0.01, -12.0 - 2.0 * k)]
            if v == 1:
                parts.append((kit.jingle(0.16, child(rk, 'hasp'), 4, 1400.0, 4200.0, 0.04, 0.12, 0.0), t + 0.03,
                              -13.0 - 3.0 * k))
            t += r.uniform(0.2, 0.32) if v == 0 else r.uniform(0.14, 0.3)
        if v == 0:
            parts.append((kit.thunk(f0 * 1.05, child(r, 'bounce'), 0.08, 0.0008, 6, 0.1), t - 0.15, -9.0))
        else:
            parts.append((kit.creak(0.2, child(r, 'hinge'), 140.0, 220.0, (900.0, 1900.0, 3000.0), 12.0, 0.2), t - 0.1,
                          -20.0))
    elif v == 2:
        parts += [(_clinks(child(r, 'a'), 7, 0.3), 0.0, 0.0),
                  (kit.creak(0.7, child(r, 'bracket'), 9.0, 20.0, (480.0, 1150.0, 2100.0), 8.0, 0.5), 0.05, -8.0),
                  (kit.thunk(r.uniform(140.0, 190.0), child(r, 'board'), 0.12, 0.0008, 6, 0.1), 0.8, -6.0),
                  (_clinks(child(r, 'b'), 4, 0.2), 0.83, -6.0)]
    else:
        parts += [(_clinks(child(r, 'a'), 4, 0.15), 0.0, 0.0),
                  (kit.creak(0.9, child(r, 'bracket'), 22.0, 8.0, (440.0, 1050.0, 1950.0), 8.0, 0.5), 0.12, -7.0),
                  (_clinks(child(r, 'b'), 6, 0.28), 0.7, -3.0),
                  (_clinks(child(r, 'c'), 1, 0.05), 1.1, -9.0)]
    x = F.filt(layers(*parts), F.lp(4600.0, 0.7), F.peak(3300.0, -4.0, 0.8), extend=False)
    return _street(x, r, -14.0)
