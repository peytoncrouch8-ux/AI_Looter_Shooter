"""The grave-salt grenade (UPlayerThrowComponent, AGraveSaltGrenade, GraveSaltBurst, AGrenadePickup): a stoppered tin
of grave salt with a waxed fuse, thrown from the off hand.

- The throw (2D, the player's own): the tin swung up out of the hand, the salt shifting inside it with a dry hiss, the
  stopper ticking in its neck, and a rush of air as it's slung, crossing from the left (the off hand) toward the
  middle.
- The bounce: a tin knocked on the ground or a wall. Thin steel rings as a shell (near-harmonic modes, short), its base
  knocks dull, and the salt inside rattles a beat behind. Heard two or three times a throw, so it's kept short and off
  the ear's touchiest band.
- The fuse (a loop): a waxed fuse burning: a fierce fizz of tiny sparks (a crackle of impulses up high), the wax
  hissing, and now and then a sputter as it spits.
- The burst: a punchy blast built the way the guns' shots are (a Friedlander pressure wave, the shoved air's heave,
  the recorder overloading, the land answering), shorter and rounder than a gun's crack, with the salt on top: a bright,
  crystalline crackle of grains pattering down on everything round it, the tin's shards ringing, and a hiss of salt
  settling. It carries like a gunshot.
- The sear: salt landing on one of the Unpaid: a frying sizzle that dies away, a ghostly gasp under it, a breath of
  steam.
- The pickup: the tin taken up: a hand on it, the salt sliding over inside, the cork's tick.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, stereo as S
from lib import granular as G, voice as V, dynamics as Y
from recipes import cue
from recipes import kit


# --- What the tin and the salt are made of ---------------------------------------------------------------------------

def _tin(r, f0, t60=0.05, contact=0.00012):
    """The tin knocked: a thin steel can rings as a shell (near-harmonic modes, short, damped by the salt packed in it)
    over a few strays of its lid and seams."""
    shell = M.shell(f0, child(r, 'shell'), t60, 6, 0.03)
    seams = M.parts(f0 * 2.2, f0 * 5.5, child(r, 'seams'), 4, t60 * 0.6, t60 * 0.3, 0.4)
    ring = M.strike(M.merge(shell, M.scale(seams, amp=0.5)), M.hammer(contact), t60 * 2.2 + 0.01)
    return normalize(ring, 0.0)


def _salt_slide(dur, r, rate, lo=2200.0, hi=8000.0, attack=0.3, release=0.6):
    """Salt sliding over itself inside the tin: dense fine grit, a dry hiss with no tone, swelling and falling."""
    n = ns(dur)
    grit = G.grit(dur, child(r, 'grit'), rate, lo, hi, 2.2, 5)
    hiss = N.band(n, child(r, 'hiss'), lo * 0.8, hi) * kit.turbulence(n, child(r, 'ht'), 180.0, 0.7)
    x = normalize(grit) + 0.35 * normalize(hiss)
    return normalize(x * E.ar(dur, dur * attack, dur * release, 2.0), 0.0)


def _salt_rattle(r, count, spread, start=0.0):
    """The salt inside thrown against the tin's wall: a short clustered patter of tiny hard grains."""
    def grain(rr, i):
        m = M.parts(2500.0, 7000.0, rr, 3, rr.uniform(0.004, 0.009), None, 0.3)
        return M.strike(m, M.hammer(0.00004), 0.015) * rr.uniform(0.4, 1.0)
    return normalize(G.rattle(0.05, child(r, 'rattle'), count, spread, grain, start, 0.85), 0.0)


def _crystals(dur, r, rate0, tau, lo=3200.0, hi=10000.0):
    """Salt crystals raining down and skittering over the ground: many tiny bright pings (each a few high modes rung
    for a hair), dense at first and thinning out; the crystalline top of the burst."""
    def grain(rr, t, i):
        m = M.parts(lo, hi, rr, 3, rr.uniform(0.003, 0.01), None, 0.25)
        return M.strike(m, M.hammer(0.00003), 0.018) * rr.uniform(0.15, 1.0)
    return G.cloud(dur, child(r, 'crystals'), lambda t: rate0 * np.exp(-t / tau), grain)


def _cork(r):
    """The cork stopper ticking in the tin's neck: a small, dull wooden tock."""
    return kit.thunk(1300.0 * jitter(r, 1, 0.1), child(r, 'cork'), 0.012, 0.0002, 4, 0.08)


def _pan_sweep(x, p0, p1):
    """Mono to stereo, moving from p0 to p1 (-1 left, +1 right) over the sound: the throw crossing the view."""
    n = x.size
    a = (np.linspace(p0, p1, n) + 1.0) * np.pi / 4.0
    return np.vstack([x * np.cos(a), x * np.sin(a)]) * np.sqrt(2.0)


# --- The throw ---------------------------------------------------------------------------------------------------------

@cue('Throwable.SaltGrenade.Throw', variations=4, space='2D', cls='Effects', jitter=0.05, conc=2, level=-14.0)
def throw(v, r):
    # The tin swung up out of the off hand and slung: a sleeve's flick, the salt shifting over inside with a dry hiss,
    # the cork ticking in the neck as the wrist cocks, then the rush of air as the arm comes through, crossing from the
    # left toward the middle and falling away fast. Some takes carry a short breath of effort.
    dur = 0.36 + 0.04 * r.random()
    n = ns(dur)
    peak_at = 0.55 + 0.06 * r.random()
    fc = O.glide([(0.0, 300.0), (dur * peak_at, 1500.0 * jitter(r, 1, 0.1)), (dur, 420.0)], n=n)
    air = F.sweep(N.pink(n, child(r, 'air')) * kit.turbulence(n, child(r, 'turb'), 80.0, 0.35), 'bp', fc, 1.6)
    mass = F.filt(N.pink(n, child(r, 'mass'), lo=50.0, hi=300.0), F.lp(240.0, 0.7), extend=False)
    u = np.arange(n) / n
    env = np.where(u < peak_at, np.sin(0.5 * np.pi * np.minimum(u / peak_at, 1.0)) ** 3.0,
                   np.exp(-7.0 * (u - peak_at) / (1.0 - peak_at)))
    rush = normalize(normalize(air) + 0.45 * normalize(mass))
    rush = normalize(rush * E.end_fade(env, 0.01))
    flick = kit.cloth(0.09, child(r, 'flick'), 400.0, 3400.0, 260.0, 0.03, 0.8)
    slide = _salt_slide(0.2, child(r, 'slide'), 9000.0, 2000.0, 7000.0, 0.2, 0.7)
    rattle = _salt_rattle(child(r, 'rattle'), 5, 0.04)
    parts = [(flick, 0.0, -17.0), (slide, 0.02, -16.0), (_cork(child(r, 'tick')), 0.07, -20.0),
             (rattle, dur * peak_at - 0.03, -19.0)]
    if v in (1, 3):
        from recipes import player as P
        parts.append((P._effort(child(r, 'breath'), 0.16, False), 0.06, -18.0))
    detail = layers(*parts)
    x = layers(_pan_sweep(rush, -0.5, 0.1), (S.widen(detail, child(r, 'w'), 0.2), 0.0, 0.0))
    return F.filt(x, F.highshelf(6500.0, -4.0), F.peak(3000.0, -3.0, 0.8), F.hp(45.0), extend=False)


# --- The bounce --------------------------------------------------------------------------------------------------------

@cue('Throwable.SaltGrenade.Bounce', variations=6, att='Near', jitter=0.06, conc=4, level=-12.0)
def bounce(v, r):
    # The tin knocked on the ground: its base's dull knock, the thin steel's short hollow ring (damped by the salt
    # packed inside), a puff of dirt, and the salt inside rattling against the wall a beat behind.
    f0 = 820.0 * jitter(r, 1, 0.12)
    tin = _tin(child(r, 'tin'), f0, 0.07 + 0.03 * r.random())
    knock = kit.thunk(240.0 * jitter(r, 1, 0.1), child(r, 'knock'), 0.035, 0.0009, 5, 0.1)
    weight = kit.noise_thump(0.07, child(r, 'weight'), 700.0, 120.0, 0.03, 1.0, 0.025)
    dirt = kit.grit_burst(0.08, child(r, 'dirt'), 3500.0, 700.0, 3500.0, 0.05)
    # The salt's patter comes inside the tin's ring, so the knock ends on the ring dying away, not on a grain.
    rattle = _salt_rattle(child(r, 'rattle'), 3 + int(r.integers(0, 3)), 0.025)
    x = layers((knock, 0.0, -2.0), (tin, 0.0, -1.0), (weight, 0.0, -6.0), (dirt, 0.001, -14.0),
               (rattle, 0.006 + 0.006 * r.random(), -14.0))
    # The ground and whatever's near answering at once: a short, soft tail.
    early = F.convolve(F.filt(x, F.hpn(300.0, 2), extend=False), R.early(child(r, 'early'), 0.035, 12, 4500.0))
    x = layers(x, kit.set_level(early, x, -18.0, 0.01))
    # Heard over and over: the 2-5 kHz band kept calm, the very top soft.
    return F.filt(x, F.peak(3200.0, -5.0, 0.8), F.highshelf(7000.0, -5.0), F.hp(70.0), extend=False)


# --- The fuse ----------------------------------------------------------------------------------------------------------

@cue('Throwable.SaltGrenade.Fuse', variations=1, att='Near', jitter=0.0, conc=4, loop=True, level=-19.0)
def fuse(v, r):
    # A waxed fuse burning down: a fierce fizz of tiny sparks (a crackle of impulses up high), the wax hissing under it,
    # and every so often a sputter as it spits a spark out. Built in a circle so it loops without a seam.
    dur = 1.6
    n = ns(dur)
    fizz = G.grit(dur, child(r, 'fizz'), 14000.0, 2500.0, 9000.0, 1.8, 5, periodic=True)
    fizz *= 0.6 + 0.4 * np.abs(N.smooth_random(n, child(r, 'fa'), 35.0, periodic=True))
    hiss = N.shaped(n, child(r, 'hiss'), 0.0, 1800.0, 10000.0, periodic=True)
    hiss *= 0.55 + 0.45 * np.abs(N.smooth_random(n, child(r, 'flutter'), 60.0, periodic=True))
    # The sputters: short puffs of low-mid noise and a pop, at random times round the loop (their tails folded round).
    events = np.zeros(n)
    for t in N.times_poisson(dur, child(r, 'spits'), 5.0, 0.0, 0.12):
        k = ns(0.05)
        puff = N.band(k, child(r, 'puff', int(t * 1000)), 500.0, 3000.0) * E.perc(0.05, 0.001, 0.025)
        start = ns(t)
        idx = (start + np.arange(k)) % n
        events[idx] += puff * r.uniform(0.4, 1.0)
    x = normalize(fizz) + 0.45 * normalize(hiss) + 0.5 * normalize(events)
    return F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(9000.0, -3.0), F.hp(250.0), circular=True)


# --- The burst ---------------------------------------------------------------------------------------------------------

@cue('Throwable.SaltGrenade.Burst', variations=5, att='Gun', jitter=0.04, conc=4, level=2.0)
def burst(v, r):
    # The tin bursting, recorded close: a round, heavy blast (a longer, softer pressure wave than a gun's crack, more
    # shove than snap), the air it pushes thumping in the chest, the microphone overloading, and on top of it the salt:
    # a bright, crystalline crackle of grains pattering down on everything round it, shards of the tin ringing as they
    # fly, and a hiss of salt settling. The land answers for a second and a half.
    from recipes import guns as Gn
    blast = Gn._loud(Gn._friedlander(child(r, 'blast'), 0.004 * jitter(r, 1.0, 0.15), 0.9 * jitter(r, 1.0, 0.2), 0.06, 0.35),
                     0.003)
    heave = Gn._heave(child(r, 'heave'), 0.012 * jitter(r, 1.0, 0.15), 110.0, 0.16)
    crack = Gn._crack(child(r, 'crack'), 0.0007 * jitter(r, 1.0, 0.12), 0.01, 900.0 * jitter(r, 1.0, 0.08), -6.0, -2.0,
                      1200.0, 8000.0)
    gas = Gn._gas(child(r, 'gas'), 0.45, 0.01, 0.14 * jitter(r, 1.0, 0.12), 16000.0, 0.02, 160.0, 3800.0, -3.0, 8.0,
                  -6.0, 0.3, 1400.0)
    src = layers((blast, 0.0, 6.0), (heave, 0.0, 10.0), (crack, 0.0, -2.0), (gas, 0.0006, -1.0))
    g1 = 0.003 + 0.003 * r.random()
    src = Gn._ground(src, [(g1, -3.0, 3500.0), (g1 + 0.003 + 0.003 * r.random(), -7.0, 1800.0)])
    rec = Gn._loud(Gn._recorder(src, 12.0 + 1.5 * (2.0 * r.random() - 1.0), 0.35, 55.0 * jitter(r, 1.0, 0.08), 1.4, 7.0, 1.5,
                                0.08, 95.0))

    # The salt: the crackle of crystals raining down, dense at the burst and thinning over half a second...
    crystals = Gn._loud(_crystals(0.9, child(r, 'salt'), 2600.0, 0.18), 0.01)
    # ...a crunch of it hitting the ground at once...
    n = ns(0.3)
    crunch = G.grit(0.3, child(r, 'crunch'), 9000.0 * np.exp(-times(n) / 0.05), 1800.0, 7000.0, 2.0, 4)
    crunch = Gn._loud(crunch * E.perc(0.3, 0.002, 0.16))
    # ...the tin's shards ringing as they're flung (thin steel, a few bright rings)...
    shards = Gn._loud(layers(*[(_tin(child(r, 'shard', i), r.uniform(1600.0, 3200.0), 0.08, 0.00006), r.uniform(0.0, 0.05),
                                r.uniform(-8.0, 0.0)) for i in range(3)]), 0.005)
    # ...and a long hiss of salt settling on everything.
    m = ns(1.4)
    settle = N.shaped(m, child(r, 'settle'), -1.0, 2500.0, 11000.0) * E.perc(1.4, 0.06, 0.9)
    settle *= 0.6 + 0.4 * np.abs(N.smooth_random(m, child(r, 'sa'), 25.0))
    settle = Gn._loud(normalize(settle), 0.02)

    dry = layers((rec, 0.0, 0.0), (crunch, 0.004, -12.0), (crystals, 0.012, -12.0), (shards, 0.006, -20.0),
                 (settle, 0.03, -24.0))
    dry = Gn._early(dry, r, -12.0, 0.04, 4000.0)
    s1 = 0.07 + 0.04 * r.random()
    s2 = s1 + 0.12 + 0.08 * r.random()
    wet = Gn._land(dry, r, 2.0, ((s1, -2.0), (s2, -4.0)), 24, 0.3, 1.4, -3.0, 1.3, 450.0, 0.3, -16.0, 110.0)
    return Gn._master(layers(dry, wet), 5.0, 2.0, 0.0015, 0.05)


# --- The sear ----------------------------------------------------------------------------------------------------------

@cue('Throwable.SaltGrenade.Sear', variations=4, att='Creature', jitter=0.05, conc=3, level=-12.0)
def sear(v, r):
    # Salt burning one of the Unpaid: a fierce frying sizzle that dies away (fat on a hot pan, too many tiny pops to
    # count), a breath of steam rising off it, and under it a ghostly gasp, several voices at once, cut short.
    from recipes import creatures as C
    dur = 0.9
    n = ns(dur)
    t = times(n)
    sizzle = G.grit(dur, child(r, 'sizzle'), 12000.0 * np.exp(-t / 0.3), 2200.0, 8000.0, 2.0, 5)
    sizzle = normalize(sizzle * E.perc(dur, 0.004, 0.6))
    steam = kit.whoosh(0.7, child(r, 'steam'), 1500.0, 5500.0, 1.2, 0.25, 0.0, 0.8)
    f = 420.0 * jitter(r, 1, 0.1)
    gasp = C._choir(child(r, 'gasp'), [(0.0, f), (0.04, f * 1.15), (0.4, f * 0.7)], 0.4, [(0.0, 'a'), (0.4, 'h')], 2, 22.0,
                    0.015, 0.75, 1.1, (6.0, 0.3), 0.2)
    gasp = C._cold(gasp * E.perc(gasp.size / SR, 0.01, 0.3), child(r, 'cold'), 37.0, 0.25)
    x = layers((sizzle, 0.0, 0.0), (steam, 0.02, -10.0), (normalize(gasp), 0.015, -9.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(8000.0, -3.0), F.hp(140.0), extend=False)
    return C._haunt(x, r, -16.0, 1.0, 3000.0)


# --- The pickup --------------------------------------------------------------------------------------------------------

@cue('Throwable.SaltGrenade.Pickup', variations=3, att='Near', jitter=0.05, conc=2, level=-15.0)
def pickup(v, r):
    # The tin taken up off the ground: a glove closing on it, the salt sliding over inside, the cork ticking in its neck,
    # the tin's faint ring as it's tucked away.
    grab = kit.cloth(0.1, child(r, 'grab'), 350.0, 3000.0, 200.0, 0.1, 0.7)
    slide = _salt_slide(0.24, child(r, 'slide'), 7000.0, 2000.0, 7000.0, 0.3, 0.6)
    tin = _tin(child(r, 'tin'), 900.0 * jitter(r, 1, 0.08), 0.06, 0.0004)
    x = layers((grab, 0.0, -6.0), (slide, 0.03, -4.0), (_cork(child(r, 'cork')), 0.08, -8.0), (tin, 0.14, -14.0))
    return F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(7000.0, -4.0), F.hp(80.0), extend=False)
