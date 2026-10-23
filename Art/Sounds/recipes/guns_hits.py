"""What a heavy round and a critical hit add on top of the ordinary hit sounds (the gun feel pass, 2026-10-22): the guns
felt alike because every gun's hits sounded the same. The game layers these over the surface's and the creature's own
cue (Combat/BulletSubsystemImpacts.cpp, Audio/CreatureVoiceComponent.cpp):

    Impact.Heavy         under the surface cue for a big round (mark scale 1.2 and up: the revolver, the sniper): a sub
                         thump in the ground and debris thrown up, so the shot lands with weight
    Impact.Scatter       once per shotgun blast (not one per pellet): a pepper of pellets striking within 40 ms and the
                         dust they kick up across the pattern
    Creature.Hit.Heavy   at the wound under the creature's hit, for a big round or a big share of a blast: a deep, meaty
                         punch
    Creature.Crit.Shell  a crit through chitin (spiders, the Spitter, the Gravemother): the shell cracking open
    Creature.Crit.Gel    a crit through a slime's gel (and the clay): a wet pop as the core is hit
    Creature.Crit.Coal   a crit on the ember-dead (the Unpaid, the miners, the Foreman, the Bell-Ringer, Abel): a coal's
                         sharp snap and a spit of sparks

Heard many times a fight, so each starts on its strike, stays short and dry, and keeps the ear's touchiest band (2-5 kHz)
eased (check.py's frequent list); recorded layers come from guns_impacts.py and the pack (Art/Sounds/packs/README.md).
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter, varispeed
from lib import noise as N, env as E, filters as F, dist as D, modal as M
from lib import granular as GR
from lib import sample as S
from recipes import cue
from recipes import kit
from recipes import guns_impacts as REC
from recipes.guns_rec import _loud


def _ease(x, cut_db=-6.0, shelf_db=-3.0):
    return F.filt(x, F.peak(3300.0, cut_db, 0.8), F.highshelf(9000.0, shelf_db), extend=False)


def _debris(dur, r, count, start, lo=900.0, hi=4500.0):
    """Clods and chips thrown up by the hit, landing round it: a scatter of small dull knocks."""
    def grain(rr, i):
        m = M.parts(lo, hi, rr, 4, rr.uniform(0.008, 0.02), 0.006)
        knock = kit.thump(rr.uniform(180.0, 320.0), 90.0, 0.03, 0.012)
        return normalize(layers(normalize(M.strike(m, M.hammer(0.00008), 0.04)), (knock, 0.0, -6.0)))
    return GR.rattle(dur, child(r, 'debris'), count, dur - start, grain, start, 0.82)


@cue('Impact.Heavy', variations=5, att='Creature', jitter=0.05, conc=6, level=-11.0)
def impact_heavy(v, r):
    # A big round driven into the ground or a wall: the strike's sub thump through the ground (a slow push of air, not
    # a tone), a recorded concrete crack played slower (a bigger bite), the debris it throws up landing round it, and a
    # dust cloud's hush.
    sub = kit.thump(78.0 * jitter(r, 1.0, 0.08), 34.0, 0.3, 0.16, attack=0.0008, drive_db=4.0)
    body = kit.noise_thump(0.14, child(r, 'body'), 700.0, 110.0, 0.06, 1.0, 0.04)
    rec = varispeed(REC.stone(v, child(r, 'rec')), [0.72, 0.8, 0.68, 0.76, 0.84][v])
    debris = _debris(0.5, r, 6 + int(r.integers(0, 4)), 0.06)
    n = ns(0.5)
    dust = N.band(n, child(r, 'dust'), 400.0, 2600.0) * E.end_fade(E.perc(0.5, 0.01, 0.3))
    x = layers((_loud(sub, 0.005), 0.0, 0.0), (_loud(body, 0.005), 0.0, -3.0), (_loud(rec, 0.005), 0.0, -5.0),
               (_loud(debris, 0.005), 0.0, -14.0), (_loud(dust, 0.01), 0.005, -18.0))
    return _ease(F.filt(x, F.hp(30.0), extend=False), -6.0, -4.0)


@cue('Impact.Scatter', variations=5, att='Creature', jitter=0.06, conc=4, level=-10.0)
def impact_scatter(v, r):
    # A shotgun's pellets arriving across the pattern: a dozen small strikes inside 40 ms (each a tick and a little thud,
    # some recorded concrete bites played fast: small pieces), the dirt and grit they kick up, a soft hush of dust.
    rr = child(r, 'pellets')
    count = 10 + int(rr.integers(0, 5))
    out = np.zeros(ns(0.12))
    stone = REC.stone(v, child(r, 'stone'))
    for i in range(count):
        at = 0.04 * rr.random() ** 1.3 if i else 0.0
        tick = kit.burst(0.004, child(rr, 'tick', i), 0.0012, lo=2500.0, hi=11000.0)
        thud = kit.noise_thump(0.05, child(rr, 'thud', i), rr.uniform(700.0, 1200.0), 150.0, 0.02, 1.0, 0.015)
        # The pattern's stragglers land softer, their ticks dull (a lone sharp tick late in the tail clicks).
        p = layers((normalize(tick), 0.0, -4.0 - 300.0 * max(0.0, at - 0.02)), (normalize(thud), 0.0, 0.0))
        if i % 3 == 0:
            p = layers(p, (normalize(varispeed(stone, rr.uniform(1.3, 1.8))), 0.0, -4.0))
        out = layers(out, (p, at, rr.uniform(-9.0, 0.0)))
    spray = kit.grit_burst(0.3, child(r, 'spray'), 9000.0, 1100.0, 5000.0, 0.18) * E.perc(0.3, 0.001, 0.09)
    puff = kit.burst(0.18, child(r, 'puff'), 0.08, lo=200.0, hi=1600.0, color=-3.0)
    # The dust settling: a soft hush that outlasts the strikes, so the cue ends on a slow fade, not on a lone grain.
    n = ns(0.3)
    hush = F.filt(N.pink(n, child(r, 'hush')), F.bp(900.0, 0.6), extend=False) * E.end_fade(np.exp(-times(n) / 0.06), 0.02)
    x = layers((_loud(out, 0.01), 0.0, 0.0), (_loud(spray, 0.01), 0.002, -10.0), (_loud(puff, 0.01), 0.0, -9.0),
               (_loud(hush, 0.01), 0.004, -14.0))
    return _ease(x, -7.0, -3.0)


@cue('Creature.Hit.Heavy', variations=5, att='Creature', jitter=0.05, conc=4, level=-10.0)
def creature_hit_heavy(v, r):
    # A big round into a body: a deep punch felt more than heard (a recorded heavy, distorted punch and a body thump
    # played slower, over a sub thump), the flesh giving with a wet slap a beat after.
    sub = kit.thump(95.0 * jitter(r, 1.0, 0.08), 40.0, 0.22, 0.11, attack=0.0006, drive_db=5.0)
    punch = S.load('Impact_PunchHeavy.wav')
    punch = varispeed(S.cut(punch, 0, punch.size, 0.3, 0.0, 0.06), [0.9, 0.82, 0.95, 0.86, 0.78][v])
    punch = F.filt(punch, F.hp(60.0), F.lpn(5000.0, 2), extend=False)
    body = varispeed(REC.body(v, child(r, 'body')), 0.8)
    wet = REC.wet(v, child(r, 'wet'))
    x = layers((_loud(sub, 0.005), 0.0, -1.0), (_loud(punch, 0.005), 0.0, 0.0), (_loud(body, 0.005), 0.0, -4.0),
               (_loud(wet, 0.005), 0.012, -12.0))
    x = D.drive(normalize(x), 3.0, 'tanh')
    return _ease(F.filt(x, F.hp(35.0), extend=False), -6.0, -4.0)


@cue('Creature.Crit.Shell', variations=4, att='Creature', jitter=0.05, conc=4, level=-9.0)
def creature_crit_shell(v, r):
    # A round finding the gap in a spider's chitin: the shell plate cracking (a hard snap rung through stiff, inharmonic
    # plate modes), shards splintering off it, the meat under it giving with a dull thud.
    snap = kit.burst(0.004, child(r, 'snap'), 0.0012, lo=1500.0, hi=9000.0)
    plate = M.strike(M.plate(1150.0 * jitter(r, 1.0, 0.08), child(r, 'plate'), 14, 0.04, 0.6, 1.3, 0.04),
                     M.hammer(0.00006), 0.12)
    shards = kit.grit_burst(0.06, child(r, 'shards'), 9000.0, 2200.0, 8000.0, 0.03) * E.perc(0.06, 0.0005, 0.03)
    crack = varispeed(REC.stone(v, child(r, 'crack')), 1.25)
    thud = kit.thump(170.0 * jitter(r, 1.0, 0.1), 80.0, 0.07, 0.04, drive_db=3.0)
    # The hollow carapace ringing a moment after, so the crack ends on a slow decay rather than a cut.
    hollow = M.strike(M.shell(520.0 * jitter(r, 1.0, 0.08), child(r, 'hollow'), 0.09, 5, 0.04), M.hammer(0.0002), 0.2)
    x = layers((_loud(snap, 0.002), 0.0, -4.0), (_loud(plate, 0.005), 0.0, 0.0), (_loud(shards, 0.005), 0.002, -8.0),
               (_loud(crack, 0.005), 0.0, -4.0), (_loud(thud, 0.005), 0.0, -3.0), (_loud(hollow, 0.005), 0.003, -12.0))
    return _ease(x, -7.0, -3.0)


@cue('Creature.Crit.Gel', variations=4, att='Creature', jitter=0.06, conc=4, level=-9.0)
def creature_crit_gel(v, r):
    # A round through a slime's core: a wet pop (a big bubble bursting, a recorded wet slap), the gel squelching back and
    # a few small bubbles after.
    pop = kit.bubble(r.uniform(300.0, 420.0), child(r, 'pop'), 0.08)
    burst = kit.burst(0.012, child(r, 'burst'), 0.005, lo=600.0, hi=4000.0)
    wet = REC.wet(v, child(r, 'wet'))
    squelch = kit.squelch(0.12, child(r, 'squelch'), 900.0, 260.0)
    rr = child(r, 'after')
    after = np.zeros(1)
    for i in range(3):
        after = layers(after, (kit.bubble(rr.uniform(600.0, 1400.0), rr, 0.12), rr.uniform(0.04, 0.16), rr.uniform(-8.0, 0.0)))
    x = layers((_loud(burst, 0.002), 0.0, -6.0), (_loud(pop, 0.005), 0.0, 0.0), (_loud(wet, 0.005), 0.0, -2.0),
               (_loud(squelch, 0.005), 0.01, -8.0), (_loud(after, 0.005), 0.0, -14.0))
    return _ease(F.filt(x, F.hp(60.0), F.lpn(7000.0, 2), extend=False), -5.0, -2.0)


@cue('Creature.Crit.Coal', variations=4, att='Creature', jitter=0.05, conc=4, level=-9.0)
def creature_crit_coal(v, r):
    # A round into an ember-dead's burning heart: a coal's brittle snap (a sharp crack rung through a few hard, short
    # modes), a spit of sparks crackling off it, the ember flaring with a soft whump of air.
    snap = kit.burst(0.003, child(r, 'snap'), 0.001, lo=1800.0, hi=10000.0)
    coal = M.strike(M.parts(900.0, 6000.0, child(r, 'coal'), 7, 0.025, 0.01, 0.4), M.hammer(0.00005), 0.08)
    n = ns(0.35)
    t = times(n)
    # The grains' rate kept high to the end with a hiss bed under them (a crackle thinning to lone grains clicks).
    sparks = GR.grit(0.35, child(r, 'sparks'), 2600.0 * np.exp(-t / 0.08) + 1500.0, 5000.0, 11000.0, 3.0, 3)
    hiss = N.band(n, child(r, 'hiss'), 4500.0, 10000.0)
    sparks = layers(_loud(sparks, 0.005), (_loud(hiss, 0.005), 0.0, -8.0)) * E.end_fade(E.perc(0.35, 0.001, 0.2))
    whump = kit.noise_thump(0.2, child(r, 'whump'), 500.0, 90.0, 0.09, 0.8, 0.08, 0.004)
    thud = kit.thump(150.0 * jitter(r, 1.0, 0.1), 70.0, 0.06, 0.035, drive_db=3.0)
    x = layers((_loud(snap, 0.002), 0.0, -3.0), (_loud(coal, 0.005), 0.0, 0.0), (_loud(sparks, 0.005), 0.003, -9.0),
               (_loud(whump, 0.005), 0.004, -9.0), (_loud(thud, 0.005), 0.0, -5.0))
    return _ease(x, -7.0, -3.0)
