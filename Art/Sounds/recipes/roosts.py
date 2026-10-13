"""Hob's roosts (ARespawnMarker as the roost, URoostCarrySubsystem's carry; wave 3): the one save, respawn and fast-travel
point on every map. A weathered mast with a stork's nest of sticks round its top, a fire basket burning under a column of
light in Hob's Ember, an old iron bell, and Hob's crows.

Its palette is the game's own: the crows are fauna.py's (a hoarse, formant-made caw; wings as pushes of air with the
feathers' rustle), the bell is the vigil stones' iron bell (shrines.py: bells.py's hand bell made duller), the fire
basket's flame is bosses.py's flame, and the beacon's light is the graves' warm bell-hum swell (graves.py) rising. The
carry is the player's own and 2D: the crows' wings fill the ears from every side, panned and moving.

- World.Roost.Found (3D at the bell, carried far): the bell struck once and ringing on as it swings, the beacon's column
  rushing up out of the basket (a flame's roar climbing and a deep, warm swell rising under it), the crows bursting off in
  a clatter of wings and a chorus of caws.
- World.Roost.Remarked (3D, near, quieter): one soft tap of the bell, a low caw, the light's hum lifting a moment.
- World.Roost.Ambient (loop, 14 s, about 25 m): crows muttering and shuffling on the perches, a low caw now and then, the
  bell's clapper ticking in the wind, the basket's low flame and crackle, the beacon's faint hum. Sparse and soft.
- UI.Roost.CarryOut (2D, the fast travel's 0.75 s fade): wings closing in from every side, caws all round, a gust as the
  crows lift Ellis away, its rush carried on through the black.
- World.Roost.CarryIn (3D at the roost, near): wings bursting outward, crows scattering with caws, boots landing on dirt.
- UI.Roost.DeathDrag (2D, under the dying; black by 2.1 s): wingbeats gathering and circling closer, harsh caws, then a
  heavy drag through dirt and gravel under a rush of wings.
- World.Roost.DeathWake (3D at the roost, near): the flock exploding off Ellis, a sharp breath drawn in, Hob's single dry
  croak.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S, voice as V
from lib import analysis as A
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes.fauna import _caw, _wingbeat, _flight, _outdoors
from recipes.shrines import iron_bell
from recipes.bells import ring, _swinging, _across
from recipes.bosses import _flame, _crackle
from recipes.graves import _swell
from recipes.world import _fold, _pnoise, _slow, _circ

# The roost bell's strike note for each take: an old iron bell, bigger and lower than the vigil stones'.
BELL_PRIMES = (392.0, 370.0, 415.0)
# The beacon's hum: D, the score's key.
HUM = 146.8


def _flock(r, birds, spread, beats=(7, 10), period=(0.12, 0.15), slow=(0.19, 0.24), decay=(0.9, 1.4)):
    """A flock bursting off: each crow's wingbeats from its own moment (fast at first, slowing as it climbs, fading),
    returned as a list of (mono signal, start) so a 2D cue can pan each bird."""
    out = []
    for i in range(birds):
        rr = child(r, 'bird', i)
        start = rr.uniform(0.0, spread) if i else 0.0
        w = _flight(rr, rr.uniform(0.85, 1.1), 0.0, int(rr.integers(beats[0], beats[1])), rr.uniform(*period),
                    rr.uniform(*slow), rr.uniform(*decay))
        out.append((normalize(w) * rr.uniform(0.55, 1.0), start))
    return out


def _mono_flock(birds):
    """The flock's birds laid together (3D)."""
    return normalize(layers(*[(w, t, 0.0) for w, t in birds]))


def _caws(r, count, t0, t1, f_lo=480.0, f_hi=640.0, harsh=(0.4, 0.7), dur=(0.2, 0.32)):
    """A chorus of caws from several crows, as a list of (signal, time)."""
    out = []
    for i in range(count):
        rr = child(r, 'caw', i)
        c = _caw(rr, rr.uniform(f_lo, f_hi), rr.uniform(*dur), rr.uniform(*harsh))
        out.append((c * rr.uniform(0.5, 1.0), rr.uniform(t0, t1)))
    return out


def _moving_pan(x, p):
    """Mono to stereo with a pan that moves (p: -1..1 per sample), constant power."""
    a = (np.clip(p, -1.0, 1.0) + 1.0) * np.pi / 4.0
    return np.vstack([x * np.cos(a), x * np.sin(a)]) * np.sqrt(2.0)


def _round(r, items, circle=0.0):
    """Items (mono signal, start) placed round the listener, each at its own side; circle > 0 swings each round at about
    that many turns a second, so they seem to circle."""
    out = np.zeros((2, 1))
    for i, (x, t) in enumerate(items):
        rr = child(r, 'side', i)
        p0 = rr.uniform(-0.95, 0.95)
        if circle > 0:
            n = x.size
            p = np.sin(2.0 * np.pi * circle * rr.uniform(0.7, 1.3) * times(n) + np.arcsin(p0))
            out = layers(out, (_moving_pan(x, p), t, 0.0))
        else:
            out = layers(out, (S.pan(x, p0), t, 0.0))
    return out


def _rise(dur, r, f0=HUM, rise=2.0):
    """The beacon's column climbing: a deep, warm swell (a low hum and its fifth, slowly beating) gliding up an octave as
    it climbs, with a rush of air rising through it."""
    n = ns(dur)
    t = times(n)
    u = np.clip(t / dur, 0.0, 1.0)
    f = f0 * rise ** (u ** 0.7)
    tone = (np.sin(2 * np.pi * O.phase(f, n)) + 0.8 * np.sin(2 * np.pi * O.phase(f * 1.004, n) + 1.0)
            + 0.5 * np.sin(2 * np.pi * O.phase(f * 1.5, n) + 2.0) + 0.25 * np.sin(2 * np.pi * O.phase(f * 2.0, n)))
    tone = F.filt(normalize(tone), F.lp(1600.0, 0.7), extend=False)
    rush = kit.whoosh(dur, child(r, 'rush'), 160.0, 2200.0, 0.8, 0.75, -3.0, 1.2)
    env = E.bp([(0.0, 0.0), (0.05, 0.4), (dur * 0.6, 1.0), (dur, 0.0)], n=n, curve='cos')
    return normalize(layers((normalize(tone) * env, 0.0, 0.0), (rush, 0.0, -8.0)))


def _beacon_flame(dur, r, peak_at=0.35, fc=(220.0, 1100.0, 420.0)):
    """The fire basket's flame shooting up: its roar climbing to peak_at and settling to a steady burn."""
    roar = _flame(dur, child(r, 'roar'), [(0.0, fc[0]), (peak_at, fc[1]), (dur, fc[2])], 18.0, 0.5, 1.0)
    env = E.bp([(0.0, 0.3), (peak_at, 1.0), (dur * 0.7, 0.4), (dur, 0.0)], n=roar.size, curve='cos')
    whump = kit.noise_thump(0.25, child(r, 'whump'), 500.0, 80.0, 0.12, 0.9, 0.1, 0.002)
    sparks = _crackle(dur, child(r, 'sparks'), 110.0, dur * 0.4, 1500.0, 5000.0, 4000.0)
    return normalize(layers((whump, 0.0, -2.0), (normalize(roar * env), 0.0, 0.0), (sparks, 0.05, -16.0)))


# --- Found and marked again -------------------------------------------------------------------------------------------

@cue('World.Roost.Found', variations=2, att='Gun', cls='Effects', jitter=0.02, conc=1, level=-6.0)
def found(v, r):
    # A roost found: the old iron bell struck once, hard, ringing on as it swings; the beacon's column shooting up out of
    # the basket (the flame's roar climbing, a deep warm swell rising an octave under it); the roost's crows bursting off
    # the mast in a clatter of wings, and a chorus of caws as they go.
    m = iron_bell(BELL_PRIMES[v], child(r, 'bell'), t60=2.6)
    bell = _swinging(normalize(ring(m, 3.4, child(r, 'stroke'), 0.0002, -20.0)), child(r, 'swing'), 1.3, 0.25)
    flame = _beacon_flame(1.6, child(r, 'flame'))
    swell = _rise(1.8, child(r, 'rise'))
    flock = _mono_flock(_flock(child(r, 'flock'), 6 + int(r.integers(0, 3)), 0.35))
    flock = F.sweep(flock, 'lp', O.expsweep(9000.0, 2800.0, flock.size / SR, 1.0, n=flock.size), 0.7)
    scrabble = kit.grit_burst(0.25, child(r, 'scrabble'), 1800.0, 900.0, 4000.0, 0.14)
    caws = normalize(layers(*[(c, t, 0.0) for c, t in _caws(child(r, 'chorus'), 5, 0.15, 1.3)]))
    x = layers((bell, 0.0, 0.0), (flame, 0.04, -8.0), (swell, 0.08, -7.0), (scrabble, 0.06, -16.0), (flock, 0.08, -6.0),
               (caws, 0.0, -5.0))
    x = F.filt(x, F.hp(50.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _across(x, r, 2.6, -12.0)


@cue('World.Roost.Remarked', variations=3, att='Near', cls='Effects', jitter=0.03, conc=1, level=-12.0)
def remarked(v, r):
    # A roost found before, made the save point again: one soft tap of the bell, a low caw from the mast, the beacon's
    # flame and its hum lifting for a moment.
    m = iron_bell(BELL_PRIMES[v], child(r, 'bell'), t60=1.6)
    tap = normalize(M.strike(m, M.hammer(0.0009), 1.8))
    caw = _caw(child(r, 'caw'), r.uniform(420.0, 470.0), r.uniform(0.24, 0.3), 0.5)
    hum = _swell(HUM, 1.4, child(r, 'hum'), 0.25, 1.2)
    breath = _beacon_flame(0.8, child(r, 'breath'), 0.12, (260.0, 700.0, 360.0))
    x = layers((tap, 0.0, 0.0), (breath, 0.0, -10.0), (hum, 0.02, -8.0), (caw, 0.35 + 0.1 * r.random(), -8.0))
    x = F.filt(x, F.hp(60.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _outdoors(x, r, -16.0, 1.2)


# --- Near a roost -------------------------------------------------------------------------------------------------------

def _mutter(r, f0):
    """A crow muttering on its perch: a short, soft, low gurgle (a caw half-swallowed)."""
    d = r.uniform(0.1, 0.18)
    c = _caw(r, f0, d, r.uniform(0.6, 0.9))
    return F.filt(c, F.lp(2600.0, 0.7), extend=False) * r.uniform(0.4, 1.0)


@cue('World.Roost.Ambient', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=2, loop=True, level=-19.0,
     align=False)
def ambient(v, r):
    # Near a roost: the crows on the mast muttering to each other and shuffling (a wing shrugged, feet on the sticks), now
    # and then a low caw; the bell's clapper ticking against it in the wind; the fire basket's low flame and crackle; the
    # beacon's faint, low hum. Fourteen seconds, built in a circle, sparse and soft so it never tires.
    dur = 14.0
    L = ns(dur)
    t = times(L)
    events = []
    rm = child(r, 'mutter')
    for k, at in enumerate(N.times_poisson(dur, rm, 0.7, 0.0, 0.35)):
        events.append((_mutter(child(rm, k), rm.uniform(360.0, 440.0)) * 0.5, at))
    rs = child(r, 'shuffle')
    for k, at in enumerate(N.times_poisson(dur, rs, 0.45, 0.0, 0.8)):
        shrug = layers((_wingbeat(child(rs, 'wing', k), rs.uniform(0.8, 1.0)), 0.0, 0.0),
                       (kit.cloth(0.2, child(rs, 'feathers', k), 900.0, 5000.0, 180.0, 0.1, 0.6), 0.0, -6.0))
        events.append((normalize(shrug) * rs.uniform(0.15, 0.3), at))
        events.append((kit.grit_burst(0.12, child(rs, 'feet', k), 900.0, 700.0, 3000.0, 0.06) * 0.08, at + 0.05))
    for k, at in enumerate((3.1, 9.7)):
        rc = child(r, 'caw', k)
        events.append((_caw(rc, rc.uniform(440.0, 500.0), rc.uniform(0.26, 0.34), 0.5) * 0.45, at + rc.uniform(-0.4, 0.4)))
    m = iron_bell(BELL_PRIMES[0], child(r, 'bell'), t60=1.4)
    for k, at in enumerate((1.6, 6.2, 6.5, 11.8)):
        tick = normalize(M.strike(m, M.hammer(0.0012), 1.5)) * child(r, 'tick', k).uniform(0.06, 0.12)
        events.append((tick, at))
    crows = _fold(L, events)
    flame = _pnoise(L, child(r, 'flame'), 70.0, 900.0, -3.0) * _slow(L, child(r, 'fg'), 2.5, 0.55)
    crackle = G.grit(dur, child(r, 'crackle'), 90.0, 1500.0, 5000.0, 2.5, 3, periodic=True)
    hum = np.zeros(L)
    for i, (ratio, amp) in enumerate(((1.0, 1.0), (1.5, 0.45), (2.0, 0.2))):
        for split in (-0.2, 0.2):
            f = round((HUM * ratio + split) * dur) / dur
            hum += amp * np.sin(2 * np.pi * f * t + r.random() * 6.28)
    hum = hum * _slow(L, child(r, 'hg'), 0.12, 0.5)
    x = (_lv(crows, 0.0) + _lv(flame, -12.0) + _lv(crackle, -17.0) + _lv(hum, -16.0))
    return _circ(x, F.hp(60.0), F.peak(3200.0, -4.0, 0.8), F.highshelf(7000.0, -4.0))


def _lv(x, db):
    """x scaled so its loudest 200 ms sits at db: the loop's layers mixed by how loud they sound."""
    return x * float(db2a(db - A.loudness_max(x)))


# --- The carry ----------------------------------------------------------------------------------------------------------

@cue('UI.Roost.CarryOut', variations=2, space='2D', cls='Effects', jitter=0.02, conc=1, level=-8.0, swell=True)
def carry_out(v, r):
    # Fast travel leaving (the screen goes black over 0.75 s): wings closing in from every side, more and louder, caws all
    # round, then a gust as the crows lift Ellis off the ground, its rush carried on through the black and away.
    birds = []
    for i in range(10):
        rr = child(r, 'wing', i)
        start = 0.75 * (i / 10.0) ** 0.8 + rr.uniform(-0.04, 0.04)
        w = _flight(rr, rr.uniform(0.85, 1.1), 0.0, int(rr.integers(5, 8)), rr.uniform(0.11, 0.14), rr.uniform(0.13, 0.16),
                    rr.uniform(0.8, 1.2))
        birds.append((normalize(w) * (0.4 + 0.6 * min(1.0, start / 0.75 + 0.2)), max(start, 0.0)))
    wings = _round(child(r, 'wings'), birds, 0.0)
    caws = _round(child(r, 'caws'), _caws(child(r, 'chorus'), 5, 0.05, 0.8, 500.0, 660.0))
    lift = kit.whoosh(1.3, child(r, 'lift'), 220.0, 1800.0, 0.8, 0.55, -3.0, 1.2)
    gust = kit.noise_thump(0.5, child(r, 'gust'), 400.0, 70.0, 0.3, 0.9, 0.25, 0.004)
    rise = np.clip(times(wings.shape[-1]) / 0.75, 0.0, 1.0) ** 1.5
    wings = wings * (0.3 + 0.7 * rise)
    x = layers((wings, 0.0, 0.0), (normalize(caws), 0.0, -7.0), (S.widen(lift, child(r, 'lw'), 0.6), 0.25, -4.0),
               (S.widen(gust, child(r, 'gw'), 0.2), 0.7, -6.0))
    x = F.filt(x, F.hp(60.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    # Carried off into the black: gone as the screen comes back at the far roost (its own CarryIn plays there).
    x = x * E.bp([(0.0, 1.0), (1.15, 1.0), (1.7, 0.0), (x.shape[-1] / SR + 0.1, 0.0)], n=x.shape[-1], curve='cos')
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=1.2, lp=3500.0)
    wet = F.convolve(F.filt(x.mean(axis=0), F.hp(250.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, -16.0, 0.05))


@cue('World.Roost.CarryIn', variations=3, att='Near', cls='Effects', jitter=0.03, conc=1, level=-9.0)
def carry_in(v, r):
    # Set down at the roost by the crows: boots landing on the dirt, the crows that carried him bursting outward off him
    # in a clatter of wings, scattering with caws.
    boots = layers((kit.noise_thump(0.14, child(r, 'boots'), 700.0, 90.0, 0.06, 1.0, 0.04), 0.0, 0.0),
                   (kit.grit_burst(0.2, child(r, 'dirt'), 2500.0, 700.0, 3500.0, 0.1), 0.004, -10.0),
                   (kit.noise_thump(0.12, child(r, 'step'), 650.0, 90.0, 0.05, 1.0, 0.04), 0.09 + 0.03 * r.random(), -6.0))
    flock = _mono_flock(_flock(child(r, 'flock'), 7 + int(r.integers(0, 3)), 0.12, (6, 9), (0.1, 0.13)))
    flock = F.sweep(flock, 'lp', O.expsweep(9000.0, 2800.0, flock.size / SR, 1.0, n=flock.size), 0.7)
    burst = kit.whoosh(0.4, child(r, 'burst'), 600.0, 2400.0, 0.9, 0.2, -3.0)
    caws = normalize(layers(*[(c, t, 0.0) for c, t in _caws(child(r, 'chorus'), 3, 0.1, 0.7)]))
    x = layers((boots, 0.0, -2.0), (burst, 0.01, -9.0), (flock, 0.0, 0.0), (caws, 0.0, -6.0))
    x = F.filt(x, F.hp(50.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _outdoors(x, r, -15.0, 1.2)


@cue('UI.Roost.DeathDrag', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-10.0, swell=True)
def death_drag(v, r):
    # Hob's crows coming for a dead man (the screen black by 2.1 s): wingbeats gathering and circling closer and louder,
    # harsh caws among them, then a heavy drag through dirt and gravel (the body hauled, juddering) under a rush of wings.
    birds = []
    for i in range(12):
        rr = child(r, 'wing', i)
        start = 1.5 * (i / 12.0) ** 0.9 + rr.uniform(-0.05, 0.05)
        w = _flight(rr, rr.uniform(0.9, 1.15), 0.0, int(rr.integers(6, 10)), rr.uniform(0.14, 0.17), rr.uniform(0.15, 0.19),
                    rr.uniform(1.0, 1.6))
        birds.append((normalize(w), max(start, 0.0)))
    wings = _round(child(r, 'wings'), birds, 0.6)
    n = wings.shape[-1]
    wings = wings * (0.1 + 0.9 * np.clip(times(n) / 1.8, 0.0, 1.0) ** 1.5)
    caws = _round(child(r, 'caws'), _caws(child(r, 'chorus'), 6, 0.2, 1.9, 430.0, 600.0, (0.6, 0.9), (0.18, 0.3)), 0.4)
    d = 1.15
    m = ns(d)
    tt = times(m)
    speed = np.clip(tt / 0.3, 0.0, 1.0) * np.clip((d - tt) / 0.25, 0.0, 1.0)
    judder = 0.5 + 0.5 * np.abs(N.smooth_random(m, child(r, 'judder'), 14.0)) ** 1.2
    body = F.filt(N.pink(m, child(r, 'body'), lo=80.0), F.lp(900.0, 0.7), extend=False) * judder * speed
    gravel = G.grit(d, child(r, 'gravel'), 4000.0 * speed + 50.0, 600.0, 3500.0, 1.5, 4) * speed
    drag = layers((normalize(body), 0.0, 0.0), (normalize(gravel), 0.0, -6.0))
    rush = kit.whoosh(1.3, child(r, 'rush'), 250.0, 1600.0, 0.8, 0.6, -3.0, 1.0)
    x = layers((wings, 0.0, 0.0), (normalize(caws), 0.0, -6.0), (S.widen(normalize(drag), child(r, 'dw'), 0.3), 1.05, -3.0),
               (S.widen(rush, child(r, 'rw'), 0.6), 1.0, -6.0))
    x = F.filt(x, F.hp(50.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    # Gone with him by the time the screen is held black (2.5 s): the wake-up at the roost takes over.
    x = x * E.bp([(0.0, 1.0), (2.2, 1.0), (2.6, 0.0), (x.shape[-1] / SR + 0.1, 0.0)], n=x.shape[-1], curve='cos')
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=1.6, lp=3000.0)
    wet = F.convolve(F.filt(x.mean(axis=0), F.hp(250.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, -13.0, 0.05))


@cue('World.Roost.DeathWake', variations=2, att='Near', cls='Effects', jitter=0.02, conc=1, level=-8.0)
def death_wake(v, r):
    # Woken at the roost: the flock exploding off Ellis all at once, a sharp breath drawn in as he comes back, and from the
    # mast Hob's single dry croak.
    flock = _mono_flock(_flock(child(r, 'flock'), 9 + int(r.integers(0, 3)), 0.06, (6, 9), (0.09, 0.12)))
    flock = F.sweep(flock, 'lp', O.expsweep(10000.0, 2800.0, flock.size / SR, 1.0, n=flock.size), 0.7)
    burst = kit.whoosh(0.35, child(r, 'burst'), 500.0, 2600.0, 0.9, 0.15, -3.0)
    rustle = kit.cloth(0.3, child(r, 'rustle'), 700.0, 5000.0, 200.0, 0.05, 0.7)
    gasp = V.whisper(0.3, [(0.0, 'h'), (0.15, 'a'), (0.3, 'uh')], child(r, 'gasp'), 0.95)
    gasp = gasp * E.bp([(0.0, 0.0), (0.03, 0.5), (0.2, 1.0), (0.3, 0.0)], n=gasp.size, curve='cos')
    gasp = F.filt(gasp, F.hp(250.0), F.lp(6000.0), extend=False)
    croak = _caw(child(r, 'croak'), [290.0, 270.0][v], 0.32, 0.85)
    x = layers((burst, 0.0, -6.0), (flock, 0.0, 0.0), (rustle, 0.0, -10.0), (normalize(gasp), 0.22, -4.0),
               (croak, 0.75 + 0.1 * r.random(), -4.0))
    x = F.filt(x, F.hp(50.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return _outdoors(x, r, -15.0, 1.3)
