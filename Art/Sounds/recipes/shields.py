"""The player's ward (SHIELD, 2026-10-22; UPlayerShieldComponent, AShieldPickup; cue names in Shields/ShieldSounds.h): a
soul-coal in an iron belt box throwing a ward of light round its wearer, which soaks hits until it breaks and fills again
after a while unhurt. All synthesized. The ward is light held in a shape, so it rings like thin glass and hums like the
coal (A2 and its partials, the Soul-light family's key); the player's own sounds are 2D, Last Breath's burst is heard where
it goes off.

- Hit: a hit soaked: a bright glassy tick (a few high inharmonic modes rung for a hair), a fizz of fine crackle over it, a
  soft push of air under it and the ward's short "bwip" sinking a third. Heard over and over in a fight: kept short, its
  2-5 kHz band dipped, the tick's body above it.
- Break: the ward shattering, the moment a fight turns: a hard crack, a shower of glass slivers thinning out, a rush
  falling away, a low thoom and the coal's hum dropping out under it.
- RechargeStart: the coal catching again: the damper's small click, a whine rising toward the octave, the hum swelling in.
- Full: whole again: two clear glass notes a fifth apart, a few motes of shimmer, the hum settling.
- Nova (Last Breath): the ward thrown outward round the player: a deep whoomph of air, a roar spreading out, the ward's
  crackle riding it and its glass ring under all of it. 3D, carries like a grenade.
- Grace (Saint's Grace): a warm breath of choir under a small bell (the Soul-light family's), the ward's glass over it.
- Pickup: the box taken up and worn: its catch's snap and the strap's leather.
"""
import numpy as np

from lib.core import ns, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, stereo as S, granular as G
from recipes import cue
from recipes import kit
from recipes import elements as EL

HUM_ROOT = 110.0          # A2: the coal's hum


def _calm(x):
    """The touchy band dipped and the rumble cut: the last step of every ward cue (heard over and over in a fight)."""
    return F.filt(x, F.peak(3300.0, -6.0, 0.8), F.hp(55.0), extend=False)


def _glass(f0, r, t60, count=5):
    """Thin glass struck: a few inharmonic modes from f0 up past four times it, the highest dying first."""
    ring = M.strike(M.parts(f0, f0 * 4.6, r, count, t60, t60 * 0.4, 0.35), M.hammer(0.00004), t60 * 1.3 + 0.01)
    return normalize(ring, 0.0)


def _hum(dur, r, f0=HUM_ROOT, glide_to=None):
    """The coal's hum: the root, its octave and a hair-sharp twelfth, shimmering slowly; gliding to glide_to if given."""
    n = ns(dur)
    f = O.glide([(0.0, f0), (dur, glide_to or f0)], n=n)
    x = O.sine(f) + 0.5 * O.sine(f * 2.0) + 0.22 * O.sine(f * 3.006)
    shimmer = 0.82 + 0.18 * N.smooth_random(n, child(r, 'shimmer'), 6.0)
    return normalize(x * shimmer, 0.0)


def _wide(x, r, amount=0.3):
    return S.widen(x, child(r, 'wide'), amount)


@cue('Shield.Hit', variations=5, space='2D', cls='Effects', jitter=0.04, conc=3, level=-14.0)
def hit(v, r):
    # A hit soaked by the ward: a glassy tick, a fizz of crackle, a soft push of air and a short "bwip" sinking a third.
    tick = _glass(3900.0 * jitter(r, 1.0, 0.1), child(r, 'tick'), 0.055 + 0.015 * r.random(), 5)
    # The fizz under a hiss bed and faded out early: a lone late grain on its own reads as a click.
    fizz = kit.grit_burst(0.16, child(r, 'fizz'), 3200.0, 5800.0, 11500.0, 0.07)
    hiss = N.band(ns(0.16), child(r, 'hiss'), 5500.0, 11000.0)
    fizz = normalize(layers((fizz, 0.0, 0.0), (normalize(hiss, 0.0), 0.0, -14.0)) * E.ar(0.16, 0.002, 0.12, 2.0), 0.0)
    push = kit.noise_thump(0.2, child(r, 'push'), 900.0, 150.0, 0.08, 1.0, 0.05)
    bwip_dur = 0.13
    bwip = O.sine(O.expsweep(560.0 * jitter(r, 1.0, 0.05), 445.0, bwip_dur, 1.4)) * E.perc(bwip_dur, 0.002, 0.08)
    x = layers((tick, 0.0, -5.0), (fizz, 0.002, -11.0), (push, 0.0, -3.0), (normalize(bwip, 0.0), 0.0, -13.0))
    return _wide(_calm(normalize(x)), r, 0.25)


@cue('Shield.Break', variations=3, space='2D', cls='Effects', jitter=0.03, conc=2, level=-7.0)
def shatter(v, r):
    # The ward shattering: a hard crack, glass slivers showering and thinning, a rush falling away, a low thoom and the
    # coal's hum dropping out under it.
    crack = kit.burst(0.05, child(r, 'crack'), 0.025, lo=1400.0, hi=12000.0)

    def sliver(rr, t, i):
        return _glass(rr.uniform(2400.0, 7200.0), rr, rr.uniform(0.03, 0.09), 3) * rr.uniform(0.2, 1.0)

    shards = G.cloud(0.85, child(r, 'shards'), lambda t: 150.0 * np.exp(-t / 0.17) + 10.0, sliver)
    bed = kit.grit_burst(0.5, child(r, 'bed'), 1800.0, 5000.0, 11000.0, 0.25)
    rush = kit.whoosh(0.75, child(r, 'rush'), 4200.0, 450.0, 0.8, 0.12, -3.0)
    thoom = kit.thump(150.0 * jitter(r, 1.0, 0.05), 42.0, 0.7, 0.4, r=child(r, 'thoom'), drive_db=4.0)
    hum = _hum(0.4, child(r, 'hum'), HUM_ROOT, HUM_ROOT * 0.62) * E.ar(0.4, 0.004, 0.36)
    x = layers((crack, 0.0, -4.0), (normalize(shards, 0.0), 0.004, -5.0), (bed, 0.004, -14.0), (rush, 0.01, -10.0),
               (thoom, 0.0, -2.0), (normalize(hum, 0.0), 0.0, -11.0))
    return _wide(_calm(D.drive(normalize(x), 2.0, 'tanh')), r, 0.35)


@cue('Shield.RechargeStart', variations=3, space='2D', cls='Effects', jitter=0.03, conc=2, level=-17.0)
def recharge_start(v, r):
    # The coal catching again: the damper's click, a whine rising toward the octave, the hum swelling in under it.
    dur = 0.7
    n = ns(dur)
    click = kit.metal_click(child(r, 'click'), 1800.0, 6000.0, 0.03)
    f = O.glide([(0.0, 330.0 * jitter(r, 1.0, 0.03)), (dur * 0.85, 640.0), (dur, 660.0)], n=n)
    whine = (O.sine(f) + 0.3 * O.sine(f * 2.0) + 0.1 * O.sine(f * 3.0)) * E.swell(dur, dur * 0.7, 0.25, 1.6)
    hum = _hum(dur, child(r, 'hum')) * E.swell(dur, dur * 0.75, 0.3, 1.4)
    air = kit.whoosh(dur, child(r, 'air'), 600.0, 2600.0, 1.0, 0.75, -4.0)
    x = layers((click, 0.0, -4.0), (normalize(whine, 0.0), 0.02, -11.0), (hum, 0.02, -7.0), (air, 0.0, -19.0))
    return _wide(_calm(normalize(x)), r, 0.2)


@cue('Shield.Full', variations=3, space='2D', cls='Effects', jitter=0.02, conc=2, level=-15.0)
def full(v, r):
    # Whole again: two clear glass notes a fifth apart (E6 then B6), a few motes of shimmer, the hum settling.
    low = kit.chime(kit.note(['E6', 'D6', 'F#6'][v]), child(r, 'low'), 0.5, 0.5, 0.00008, 0.4, -30.0)
    high = kit.chime(kit.note(['B6', 'A6', 'C#7'][v]), child(r, 'high'), 0.45, 0.4, 0.00008, 0.4, -30.0)
    sh = EL._shimmer(child(r, 'shimmer'), 5, 0.04, 0.25)
    hum = _hum(0.5, child(r, 'hum')) * E.perc(0.5, 0.01, 0.35)
    x = layers((low, 0.0, -3.0), (high, 0.07, -4.0), (sh, 0.0, -16.0), (normalize(hum, 0.0), 0.0, -12.0))
    return _wide(_calm(x), r, 0.3)


@cue('Shield.Nova', variations=3, att='Creature', cls='Effects', jitter=0.03, conc=2, level=-6.0)
def nova(v, r):
    # Last Breath: the ward thrown outward: a deep whoomph, a roar spreading out, the ward's crackle riding it, its glass
    # ringing under all of it.
    whoomph = kit.noise_thump(0.6, child(r, 'whoomph'), 650.0, 48.0, 0.35, 1.1, 0.15)
    roar = D.drive(kit.whoosh(1.1, child(r, 'roar'), 280.0, 2400.0, 0.7, 0.18, -3.0), 4.0, 'tanh')
    crackle = kit.grit_burst(1.0, child(r, 'crackle'), 3200.0, 4800.0, 10500.0, 0.5)
    ring = _glass(1700.0 * jitter(r, 1.0, 0.06), child(r, 'ring'), 0.45, 6)
    thump = kit.thump(120.0, 38.0, 0.7, 0.45, r=child(r, 'thump'), drive_db=3.0)
    x = layers((whoomph, 0.0, 0.0), (normalize(roar, 0.0), 0.01, -5.0), (crackle, 0.02, -12.0), (ring, 0.0, -10.0),
               (thump, 0.0, -3.0))
    return EL._air(EL._calm(D.drive(normalize(x), 2.0, 'tanh')), r, -14.0, 1.4)


@cue('Shield.Grace', variations=2, space='2D', cls='Effects', jitter=0.01, conc=1, level=-11.0, swell=True)
def grace(v, r):
    # Saint's Grace: a warm breath of choir (A major) under a small bell, the ward's glass over it.
    root = kit.note(['A3', 'B3'][v])
    choir = EL._voices(child(r, 'choir'), [root, root * 1.26, root * 1.5], 0.95, 'a', 1, 0.5)
    bell = EL._bell(root * 4.0, child(r, 'bell'), 0.9, 0.6)
    glass = _glass(root * 8.0, child(r, 'glass'), 0.3, 4)
    x = layers((choir, 0.0, -3.0), (bell, 0.05, -2.0), (glass, 0.08, -11.0))
    return _wide(_calm(x), r, 0.35)


@cue('Shield.Pickup', variations=3, space='2D', cls='Interface', jitter=0.04, conc=2, level=-16.0)
def pickup(v, r):
    # The ward box taken up and worn: its catch's snap and the strap's leather.
    catch = kit.clack(child(r, 'catch'), 950.0 * jitter(r, 1.0, 0.08), 2000.0, 6500.0)
    strap = kit.cloth(0.26, child(r, 'strap'), 300.0, 2400.0, 90.0, 0.08, 0.7)
    creak = kit.creak(0.2, child(r, 'creak'), 40.0, 18.0, (420.0, 900.0, 1700.0), 7.0)
    x = layers((catch, 0.0, -3.0), (strap, 0.0, -10.0), (creak, 0.04, -15.0))
    return _wide(_calm(normalize(x)), r, 0.2)
