"""The spiders' ambush (2026-10-31, the user's pick: spiders drop from walls and ledges; SpiderAmbushCue in
Source/AI_Looter_Shooter/Creatures/SpiderAmbushRules.h; ASpiderCreature, SpiderCreatureLurk.cpp and SpiderCreatureLeap.cpp
play them). Each is a tell or a payoff the player hears and answers, on the brown spider's own materials (creatures.py:
chitin, mandibles chittering, the stridulating rasp, spiracle hisses, legs skittering), at the spider, 3D:

- Creature.Spider.Lurk: a lurker's faint chitter as the player comes near it (quiet, 'Near': only the one who listens
  hears it before it springs).
- Creature.Spider.Leap: springing (its 0.35 s crouch, then off the rock): the rasp worked hard and fast, rising, a hiss
  tearing out over it, and eight legs tearing off the stone into the air's rush. The tell: it must read at once.
- Creature.Spider.Land: down from a leap: the body's weight arriving (a low thump), the shell's knock, eight feet striking
  in a scatter of ticks and grit thrown, a skitter as it gathers itself.
- Creature.Spider.Climb: scrambling head first down a wall: claws ticking fast on rock, grit trickling (the game plays it
  about every 0.45 s while it climbs).
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, voice as V
from recipes import cue
from recipes import kit
from recipes import creatures as C


def _shape(x, attack, release):
    """x through an attack-release envelope its own length."""
    e = E.ar(x.size / SR, attack, release)
    if e.size < x.size:
        e = np.pad(e, (0, x.size - e.size))
    return x * e[:x.size]


@cue('Creature.Spider.Lurk', variations=3, att='Near', jitter=0.06, conc=2, level=-20.0)
def spider_lurk(v, r):
    # Waiting, it knows you're there: its mandibles working in slow dry clicks, a short low rasp under them, a breath of
    # hiss. Small and close, like something in the rocks.
    ch = C.chitter(0.55, child(r, 'chitter'), 8.0 + 2.0 * v, 15.0, 1500.0, 4000.0, 0.4, 0.8)
    rs = _shape(normalize(C.rasp(0.4, child(r, 'rasp'), 32.0 * jitter(r, 1, 0.1), 50.0, (1000.0, 2100.0, 3300.0), 3.0)), 0.12, 0.25)
    n = ns(0.5)
    hs = V.hiss(n, child(r, 'hiss'), (2200.0, 3800.0, 6000.0)) * E.swell(0.5, 0.25, 0.2)[:n]
    x = layers((ch, 0.0, 0.0), (normalize(rs), 0.1, -8.0), (normalize(hs), 0.05, -18.0))
    return C._outside(F.filt(x, F.peak(3300.0, -4.0, 0.8), F.hp(300.0), extend=False), r, -24.0)


@cue('Creature.Spider.Leap', variations=4, att='Creature', jitter=0.05, conc=3, level=-6.0, swell=True)
def spider_leap(v, r):
    # Its crouch and spring: the stridulation file scraped hard, rising fast over 0.35 s, a hiss tearing out of it, then
    # the legs ripping off the stone (a burst of ticks and grit) into the rush of the leap.
    coil = 0.33 + 0.04 * r.random()
    rs = C.rasp(coil, child(r, 'rasp'), 70.0 * jitter(r, 1, 0.08), 260.0, (1200.0, 2400.0, 3800.0), 3.0)
    rs = normalize(rs) * E.bp([(0.0, 0.2), (0.8 * coil, 1.0), (coil, 0.6)], n=rs.size, curve='cos')
    n = ns(coil + 0.15)
    fc = np.interp(times(n), [0.0, coil, coil + 0.15], [1600.0, 5200.0, 2500.0])
    hiss = F.sweep(N.white(n, child(r, 'hiss')), 'bp', fc, 1.3) * kit.turbulence(n, child(r, 'ht'), 160.0, 0.5)
    hiss = normalize(hiss) * E.bp([(0.0, 0.0), (0.6 * coil, 0.6), (coil, 1.0), (coil + 0.15, 0.0)], n=n, curve='cos')
    off = C._skitter(child(r, 'off'), 8, 0.0, 0.006)
    grit = kit.grit_burst(0.1, child(r, 'grit'), 4000.0, 1000.0, 5000.0, 0.05)
    rush = kit.whoosh(0.3, child(r, 'rush'), 300.0, 1100.0, 1.0, 0.35, -3.0)
    clack = C._chitin(child(r, 'clack'), 780.0 * jitter(r, 1, 0.08), 0.02, 2)
    x = layers((normalize(rs), 0.0, 0.0), (normalize(hiss), 0.0, -6.0), (clack, coil - 0.01, -4.0), (off, coil, -3.0),
               (grit, coil, -12.0), (rush, coil + 0.02, -9.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.hp(150.0), extend=False)
    return C._outside(x, r, -20.0)


@cue('Creature.Spider.Land', variations=4, att='Creature', jitter=0.05, conc=3, level=-8.0)
def spider_land(v, r):
    # Down from a leap: the weight arriving (a low thump and a dark thud of air), the shell's hollow knock, eight feet
    # striking in a quick scatter, grit thrown, then a short skitter as it gathers itself to come at you.
    size = (0.95, 1.0, 1.05, 1.1)[v] * jitter(r, 1.0, 0.04)
    mass = kit.thump(95.0 / size, 45.0, 0.3, 0.16, attack=0.0015, drive_db=2.0)
    body = kit.noise_thump(0.25, child(r, 'body'), 600.0 / size, 90.0, 0.1, 1.1, 0.05, 0.001)
    knock = kit.thunk(600.0 * jitter(r, 1, 0.08) / size, child(r, 'knock'), 0.035, 0.0004, 6, 0.12)
    feet = C._skitter(child(r, 'feet'), 8, 0.0, 0.005)
    grit = kit.grit_burst(0.2, child(r, 'grit'), 3500.0, 700.0, 4200.0, 0.1)
    after = C._skitter(child(r, 'after'), 4 + int(r.integers(0, 3)), 0.0, 0.03)
    ch = C.chitter(0.14, child(r, 'chitter'), 32.0, 22.0)
    x = layers((mass, 0.0, 0.0), (body, 0.0, -3.0), (knock, 0.002, -6.0), (feet, 0.0, -4.0), (grit, 0.005, -13.0),
               (after, 0.16, -10.0), (ch, 0.2, -15.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False)
    return C._outside(x, r, -22.0)


@cue('Creature.Spider.Climb', variations=4, att='Creature', jitter=0.08, conc=2, level=-14.0)
def spider_climb(v, r):
    # Head first down the rock at a scramble: claws ticking fast and uneven on stone, a little grit trickling after.
    taps = C._skitter(child(r, 'taps'), 9 + int(r.integers(0, 4)), 0.0, 0.032)
    grit = _shape(normalize(kit.grit_burst(0.35, child(r, 'grit'), 900.0, 900.0, 5000.0, 0.25)), 0.05, 0.2)
    scrape = _shape(normalize(N.band(ns(0.3), child(r, 'scrape'), 1800.0, 6000.0)), 0.04, 0.2)
    x = layers((taps, 0.0, 0.0), (normalize(grit), 0.04, -14.0), (normalize(scrape), 0.02, -20.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.hp(250.0), extend=False)
    return C._outside(x, r, -24.0)
