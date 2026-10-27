"""The grenade kinds beside the grave-salt jar (round 4, 2026-10-27; GrenadeKindCue in Source/.../Combat/GrenadeKinds.h):
the pitch jar, the scatter jar and the cinder jar. Each kind's burst plays the salt jar's (throwables.py) under its own
layer, so these are what the kind adds, not whole explosions; the scatter's small jars and the swap are their own.

- PitchJar.Stick: the pitch jar slapping onto something and holding: a wet, heavy smack, the tin's knock damped by the
  pitch, and a sticky pull as it settles.
- PitchJar.Burst (over the salt burst): hot pitch thrown about: a low wet whump, a tearing splatter of globs landing all
  round, and the pitch sizzling as it burns.
- ScatterJar.Burst (over the salt burst): the tin splitting with a bright crack, the small jars whirring away and their
  copper shells clattering.
- ScatterJar.Pop: one small jar bursting: a short, sharp crack and a thump, a puff of salt crystals pattering down.
- CinderJar.Burst (over the salt burst): the coals catching: a deep whoomp of air drawn in, a roar of flame that dies
  back, coals crackling as they scatter.
- CinderJar.Bed (a loop): the bed of coals burning: a low flutter of flame, dense crackling, now and then a pop of sap
  or grit spitting.
- Jar.Swap (2D, the player's own): a new kind of jar taken up in place of the old: a tin pushed into a leather loop, its
  clasp clicked shut, the cork ticking.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, granular as G, reverb as R
from recipes import cue
from recipes import kit
from recipes import throwables as T


def _flutter_flame(n, r, lo, hi, rate, periodic=False):
    """Flame's breath: dark noise in a low band, its level fluttering fast and unevenly."""
    x = N.shaped(n, child(r, 'flame'), -1.0, lo, hi, periodic=periodic)
    am = 0.45 + 0.55 * np.abs(N.smooth_random(n, child(r, 'fl'), rate, periodic=periodic))
    return normalize(x * am, 0.0)


# --- The pitch jar -----------------------------------------------------------------------------------------------------

@cue('Throwable.PitchJar.Stick', variations=4, att='Near', jitter=0.05, conc=3, level=-12.0)
def pitch_stick(v, r):
    # A heavy tin coated in soft pitch landing and holding: the wet smack of the coat, a dull knock of the tin under it
    # (its ring damped dead), then the pitch pulling and settling round it.
    smack = kit.squelch(0.12, child(r, 'smack'), 1100.0 * jitter(r, 1, 0.1), 280.0, 2.4)
    thud = kit.noise_thump(0.1, child(r, 'thud'), 650.0, 110.0, 0.05)
    tin = T._tin(child(r, 'tin'), 760.0 * jitter(r, 1, 0.1), 0.022, 0.0005)
    pull = kit.squelch(0.2, child(r, 'pull'), 380.0, 1300.0, 4.0)
    x = layers((smack, 0.0, -1.0), (thud, 0.0, -3.0), (tin, 0.002, -16.0), (pull, 0.05 + 0.03 * r.random(), -13.0))
    return F.filt(x, F.peak(3200.0, -5.0, 0.8), F.highshelf(6500.0, -5.0), F.hp(60.0), extend=False)


@cue('Throwable.PitchJar.Burst', variations=4, att='Creature', jitter=0.04, conc=3, level=-5.0)
def pitch_burst(v, r):
    # Over the salt burst: the pitch thrown out wet and burning: a low wet whump, globs tearing loose and splattering
    # down all round over half a second, and the hot pitch sizzling as it lies there.
    whump = kit.noise_thump(0.32, child(r, 'whump'), 320.0, 55.0, 0.24)
    tear = kit.squelch(0.3, child(r, 'tear'), 1500.0 * jitter(r, 1, 0.1), 260.0, 2.0)

    def glob(rr, t, i):
        return kit.squelch(rr.uniform(0.04, 0.09), rr, rr.uniform(700.0, 1600.0), rr.uniform(200.0, 400.0), 3.0) * rr.uniform(0.3, 1.0)
    splats = normalize(G.cloud(0.7, child(r, 'splats'), lambda t: 40.0 * np.exp(-t / 0.22), glob, start=0.04), 0.0)
    n = ns(1.0)
    sizzle = G.grit(1.0, child(r, 'sizzle'), 9000.0 * np.exp(-times(n) / 0.35), 2400.0, 8000.0, 2.0, 5)
    sizzle = normalize(sizzle * E.perc(1.0, 0.02, 0.7), 0.0)
    x = layers((whump, 0.0, -2.0), (tear, 0.0, -4.0), (splats, 0.0, -6.0), (sizzle, 0.05, -15.0))
    return F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(8000.0, -4.0), F.hp(40.0), extend=False)


# --- The scatter jar ---------------------------------------------------------------------------------------------------

@cue('Throwable.ScatterJar.Burst', variations=4, att='Creature', jitter=0.04, conc=3, level=-6.0)
def scatter_burst(v, r):
    # Over the salt burst: the tin splitting with a bright crack, the small jars whirring away from it, and their copper
    # shells clattering against the bands as they tear loose.
    split = T._tin(child(r, 'split'), 1700.0 * jitter(r, 1, 0.1), 0.05, 0.00006)
    crack = kit.burst(0.05, child(r, 'crack'), 0.03, 900.0, 7000.0)
    whirr = kit.whoosh(0.45, child(r, 'whirr'), 700.0, 2600.0, 1.4, 0.2)
    whirr = normalize(whirr * kit.turbulence(whirr.size, child(r, 'wt'), 120.0, 0.7), 0.0)
    clatter = kit.jingle(0.18, child(r, 'clatter'), 6, 1800.0, 6500.0, 0.05, 0.08)
    x = layers((split, 0.0, -2.0), (crack, 0.0, -3.0), (whirr, 0.01, -9.0), (clatter, 0.004, -10.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(8000.0, -3.0), F.hp(80.0), extend=False)


@cue('Throwable.ScatterJar.Pop', variations=6, att='Creature', jitter=0.06, conc=6, level=-7.0)
def scatter_pop(v, r):
    # One small jar bursting a stone's throw off: a short, sharp crack with a little thump of air behind it, a puff of
    # salt crystals pattering down, and the ground answering for a moment. Heard four times in a second: short and kept
    # off the ear's touchiest band.
    from recipes import guns as Gn
    blast = Gn._loud(Gn._friedlander(child(r, 'blast'), 0.0018 * jitter(r, 1.0, 0.15), 0.9, 0.035, 0.3), 0.002)
    heave = Gn._heave(child(r, 'heave'), 0.006 * jitter(r, 1.0, 0.15), 170.0, 0.08)
    crack = kit.burst(0.03, child(r, 'crack'), 0.02, 1200.0, 8000.0)
    salt = normalize(T._crystals(0.35, child(r, 'salt'), 1400.0, 0.08), 0.0)
    dry = layers((blast, 0.0, 0.0), (heave, 0.0, 2.0), (crack, 0.0, -6.0), (salt, 0.006, -16.0))
    early = F.convolve(F.filt(dry, F.hpn(250.0, 2), extend=False), R.early(child(r, 'early'), 0.06, 12, 4000.0))
    x = layers(dry, kit.set_level(early, dry, -14.0, 0.01))
    return F.filt(x, F.peak(3100.0, -5.0, 0.8), F.highshelf(7500.0, -4.0), F.hp(50.0), extend=False)


# --- The cinder jar ----------------------------------------------------------------------------------------------------

@cue('Throwable.CinderJar.Burst', variations=4, att='Creature', jitter=0.04, conc=3, level=-5.0)
def cinder_burst(v, r):
    # Over the salt burst: the coals catching all at once: a deep whoomp as the fire draws its breath, a roar of flame
    # that swells and dies back over a second, and the coals crackling as they scatter over the ground.
    whoomp = kit.whoosh(0.55, child(r, 'whoomp'), 140.0, 700.0, 0.8, 0.18, -6.0)
    thump = kit.noise_thump(0.5, child(r, 'thump'), 220.0, 45.0, 0.4)
    dur = 1.3
    n = ns(dur)
    roar = _flutter_flame(n, child(r, 'roar'), 60.0, 1400.0, 28.0)
    roar = normalize(roar * E.ar(dur, 0.08, dur * 0.85, 2.0), 0.0)
    crackle = G.grit(dur, child(r, 'crackle'), 2600.0 * np.exp(-times(n) / 0.5), 1400.0, 6000.0, 2.0, 4)
    crackle = normalize(crackle * E.perc(dur, 0.05, 0.9), 0.0)
    x = layers((whoomp, 0.0, -2.0), (thump, 0.0, -4.0), (roar, 0.02, -6.0), (crackle, 0.05, -14.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(7000.0, -4.0), F.hp(35.0), extend=False)


@cue('Throwable.CinderJar.Bed', variations=1, att='Near', jitter=0.0, conc=4, loop=True, level=-16.0)
def cinder_bed(v, r):
    # A bed of coals burning in the open: a low flutter of flame, a dense crackle of embers, and every so often a pop as
    # sap or grit spits out of it. Built in a circle so it loops without a seam.
    dur = 2.4
    n = ns(dur)
    flame = _flutter_flame(n, child(r, 'flame'), 70.0, 900.0, 14.0, periodic=True)
    crackle = G.grit(dur, child(r, 'crackle'), 1800.0, 1500.0, 6000.0, 2.0, 4, periodic=True)
    crackle *= 0.55 + 0.45 * np.abs(N.smooth_random(n, child(r, 'ca'), 9.0, periodic=True))
    # The pops: short bright snaps at random times round the loop (their tails folded round).
    events = np.zeros(n)
    for t in N.times_poisson(dur, child(r, 'pops'), 3.0, 0.0, 0.2):
        k = ns(0.035)
        snap = N.band(k, child(r, 'snap', int(t * 1000)), 900.0, 4500.0) * E.perc(0.035, 0.0005, 0.02)
        idx = (ns(t) + np.arange(k)) % n
        events[idx] += snap * r.uniform(0.4, 1.0)
    x = normalize(flame) + 0.5 * normalize(crackle) + 0.45 * normalize(events)
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(7500.0, -4.0), F.hp(50.0), circular=True)


# --- The swap ----------------------------------------------------------------------------------------------------------

@cue('Throwable.Jar.Swap', variations=3, space='2D', cls='Effects', jitter=0.04, conc=1, level=-16.0)
def jar_swap(v, r):
    # A new kind of jar taken up in place of the old: a tin pushed into a leather loop on the belt, the clasp clicked
    # shut over it, the cork ticking in its neck.
    leather = kit.cloth(0.14, child(r, 'leather'), 250.0, 2600.0, 120.0, 0.25, 0.6)
    tin = T._tin(child(r, 'tin'), 940.0 * jitter(r, 1, 0.08), 0.05, 0.0004)
    clasp = kit.brass_tick(child(r, 'clasp'), 1900.0 * jitter(r, 1, 0.08))
    x = layers((leather, 0.0, -4.0), (tin, 0.05, -10.0), (clasp, 0.11, -6.0), (T._cork(child(r, 'cork')), 0.16, -12.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(7000.0, -4.0), F.hp(90.0), extend=False)
