"""Bullets meeting the world: ground and stone, wood, metal, water. Each is the strike itself (a hard tick), what the
material does with it (dirt thuds and sprays, stone chips, wood knocks and splinters, metal rings, water splashes and
bubbles) and a short slap-back from the surroundings. A ricochet's whine is its own cue (Impact.Ricochet), which the game
layers on glancing hits.

By surface (2026-10-13, wave 3a: ImpactSurface::Cue picks one per physical material): dirt, grass, gravel, sand, stone,
sheet metal, glass, cloth, hay and bone, and a ricochet the game layers on about half the glancing hits on stone, gravel
and metal. They're heard by the hundred in a firefight, so each is short and dry, starts on its strike, and keeps the
ear's touchiest band (2-5 kHz) soft: what's bright in them (stone's chip, glass's shards, the ricochet's start) is cut
there and carried above 5 kHz or below 2 kHz instead. Soft surfaces are told apart by their bodies: dirt's heavy thud and
soil spray, grass's muffled thud and tearing blades, sand's breathy puff and hiss, hay's rustle, cloth's thwip.
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


def _ricochet_taken_out(r):
    """Impact.World's takes 3 and 6 and Impact.Metal's 2, 4 and 6 had a ricochet's whine of their own until
    Impact.Ricochet came to be layered on glancing hits (2026-10-13). The random numbers it drew are still drawn, so the
    rest of those takes (their slap-back) stays exactly as it was."""
    child(r, 'ric')
    r.random(3)


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
        _ricochet_taken_out(r)
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
        _ricochet_taken_out(r)
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


# --- By surface (2026-10-13) --------------------------------------------------------------------------------------------

def _ease(x, cut_db=-5.0, shelf_db=-3.0):
    """The 2-5 kHz band eased and the very top softened: these come by the hundred in a firefight."""
    return F.filt(x, F.peak(3300.0, cut_db, 0.8), F.highshelf(9000.0, shelf_db), extend=False)


def _strike(r, lo=2000.0, hi=12000.0, dur=0.004):
    """The round's contact: a hard, very short tick."""
    return kit.burst(dur, child(r, 'strike'), dur * 0.4, lo=lo, hi=hi, attack=0.0001)


def _clods(dur, r, count, start, lo=500.0, hi=2200.0):
    """Clods and soil falling back: dull, soft little knocks, fewer and fainter."""
    def grain(rr, i):
        return kit.thunk(rr.uniform(lo, hi), rr, 0.012, 0.0008, 4, 0.15) * rr.uniform(0.4, 1.0)
    return normalize(G.rattle(dur, child(r, 'clods'), count, dur - start, grain, start, 0.82), 0.0)


@cue('Impact.Dirt', variations=6, att='Creature', jitter=0.06, conc=8, level=-10.0)
def impact_dirt(v, r):
    # Packed dry earth: the strike's tick, a dull, heavy thud as the round buries itself, a puff of dust, and a gritty
    # spray of soil thrown up and pattering back down in clods.
    thud = kit.noise_thump(0.14, child(r, 'thud'), 750.0 * jitter(r, 1, 0.1), 95.0, 0.055, 1.0, 0.035)
    spray = kit.grit_burst(0.32, child(r, 'spray'), 6500.0, 800.0, 3800.0, 0.18)
    puff = kit.burst(0.08, child(r, 'puff'), 0.04, lo=180.0, hi=1400.0, color=-3.0)
    clods = _clods(0.4, child(r, 'fall'), 4 + int(r.integers(0, 4)), 0.09)
    x = layers((_strike(r, 1800.0, 9000.0), 0.0, -6.0), (thud, 0.0, 0.0), (puff, 0.001, -8.0), (spray, 0.003, -10.0),
               (clods, 0.0, -19.0))
    return _slap(_ease(x), r, -22.0)


@cue('Impact.Grass', variations=6, att='Creature', jitter=0.06, conc=8, level=-11.0)
def impact_grass(v, r):
    # Grass over soil: a softer, muffled thud (the turf takes it), a quick tearing swish of blades cut through, and a
    # little soil thrown.
    thud = kit.noise_thump(0.12, child(r, 'thud'), 560.0 * jitter(r, 1, 0.1), 85.0, 0.05, 0.9, 0.035, 0.0006)
    d = 0.05 + 0.02 * r.random()
    n = ns(d)
    tear = N.band(n, child(r, 'tear'), 1200.0, 6500.0) * (0.2 + 0.8 * np.abs(N.smooth_random(n, child(r, 'blades'), 420.0)))
    tear = normalize(tear * E.ar(d, 0.002, d * 0.7, 1.5))
    swish = kit.whoosh(0.07, child(r, 'swish'), 900.0, 2600.0, 1.2, 0.3, -2.0)
    soil = kit.grit_burst(0.22, child(r, 'soil'), 3500.0, 700.0, 3200.0, 0.12)
    x = layers((_strike(r, 1500.0, 7000.0), 0.0, -12.0), (thud, 0.0, 0.0), (tear, 0.001, -9.0), (swish, 0.002, -14.0),
               (soil, 0.004, -14.0), (_clods(0.3, child(r, 'fall'), 2 + int(r.integers(0, 2)), 0.08), 0.0, -24.0))
    return _slap(_ease(F.filt(x, F.lp(7000.0, 0.7), extend=False), -6.0), r, -23.0)


@cue('Impact.Gravel', variations=5, att='Creature', jitter=0.06, conc=8, level=-10.0)
def impact_gravel(v, r):
    # Gravel and ballast: a sharp crunch of many stones knocked at once, the thud under it, and small stones skittering
    # away and clicking apart.
    n = ns(0.07)
    crunch = G.grit(0.07, child(r, 'crunch'), 40000.0 * np.exp(-times(n) / 0.012), 500.0, 4500.0, 1.4, 5)
    crunch = normalize(crunch * E.end_fade(np.ones(n), 0.01))
    thud = kit.noise_thump(0.1, child(r, 'thud'), 800.0, 120.0, 0.04, 1.0, 0.03)

    def stone(rr, i):
        m = M.parts(1100.0 * rr.uniform(0.85, 1.2), 4200.0, rr, 4, 0.016, 0.008, 0.45)
        return normalize(M.strike(m, M.hammer(0.00008), 0.04)) * rr.uniform(0.5, 1.0)
    skitter = normalize(G.rattle(0.45, child(r, 'skitter'), 7 + int(r.integers(0, 5)), 0.35, stone, 0.02, 0.86))
    x = layers((_strike(r, 2000.0, 10000.0), 0.0, -6.0), (crunch, 0.0, -2.0), (thud, 0.0, -3.0), (skitter, 0.0, -11.0))
    return _slap(_ease(x, -6.0), r, -22.0)


@cue('Impact.Sand', variations=4, att='Creature', jitter=0.06, conc=8, level=-12.0)
def impact_sand(v, r):
    # Dry sand and dust: a soft, breathy puff as the grains give way (hardly a tick), a low soft thump, and a fine hiss of
    # grains settling after.
    puff = kit.burst(0.11, child(r, 'puff'), 0.06, lo=250.0, hi=2200.0, color=-2.0, attack=0.0015)
    thump = kit.noise_thump(0.1, child(r, 'thump'), 500.0, 90.0, 0.04, 0.9, 0.04, 0.0008)
    m = ns(0.45)
    hiss = N.band(m, child(r, 'hiss'), 2500.0, 9000.0) * E.perc(0.45, 0.01, 0.3)
    hiss = hiss * (0.6 + 0.4 * np.abs(N.smooth_random(m, child(r, 'grains'), 60.0)))
    fine = G.grit(0.4, child(r, 'fine'), 9000.0, 3000.0, 9000.0, 1.5, 3) * E.perc(0.4, 0.01, 0.25)
    x = layers((_strike(r, 1500.0, 6000.0), 0.0, -18.0), (puff, 0.0, 0.0), (thump, 0.0, -3.0),
               (normalize(hiss), 0.01, -14.0), (normalize(fine), 0.01, -16.0))
    return _slap(_ease(x, -6.0, -5.0), r, -24.0)


@cue('Impact.Stone', variations=6, att='Creature', jitter=0.05, conc=8, level=-10.0)
def impact_stone(v, r):
    # Rock and masonry: a hard, bright crack (the strike and a chip of stone flying), the dense mass knocking short under
    # it, a puff of rock dust, and a short rattle of grit.
    chip = normalize(M.strike(M.parts(1600.0, 7500.0, child(r, 'chip'), 7, 0.025, 0.01, 0.2), M.hammer(0.00004), 0.06))
    chip = F.filt(chip, F.peak(3300.0, -8.0, 0.7), extend=False)
    crack = kit.burst(0.014, child(r, 'crack'), 0.006, lo=900.0, hi=7000.0, attack=0.0001)
    mass = kit.thunk(620.0 * jitter(r, 1, 0.15), child(r, 'mass'), 0.025, 0.0001, 6, 0.12)
    thud = kit.noise_thump(0.08, child(r, 'thud'), 1100.0, 160.0, 0.03, 1.0, 0.02)
    dust = kit.burst(0.07, child(r, 'dust'), 0.035, lo=400.0, hi=2500.0, color=-2.0)
    grit = _pebbles(0.35, r, 3 + int(r.integers(0, 4)), 0.05)
    x = layers((_strike(r, 2500.0, 14000.0), 0.0, -3.0), (crack, 0.0, -4.0), (chip, 0.0, -5.0), (mass, 0.0, -3.0),
               (thud, 0.0, -5.0), (dust, 0.002, -14.0), (grit, 0.0, -18.0))
    return _slap(_ease(x, -5.0), r, -21.0)


@cue('Impact.SheetMetal', variations=5, att='Creature', jitter=0.05, conc=8, level=-10.0)
def impact_sheet_metal(v, r):
    # Thin sheet metal (a tin roof, a drum, a bucket): a hollow, tinny bang as the sheet is punched, its edges and rivets
    # rattling, and the sheet's wobbling ring dying away.
    from recipes.bosses import _sheet_wobble
    f0 = 210.0 + 140.0 * r.random()
    m = M.plate(f0, child(r, 'sheet'), 22, 0.25 * jitter(r, 1, 0.25), 0.6, tilt=0.7)
    bang = normalize(M.strike(m, M.hammer(0.00008), 0.4))
    wobble = _sheet_wobble(f0 * 1.7, 0.45, child(r, 'wobble'), 8.0 + 4.0 * r.random(), 0.04, 0.3)
    hollow = kit.thump(f0 * 0.7, f0 * 0.55, 0.09, 0.05)
    rattle = kit.jingle(0.18, child(r, 'rattle'), 3, 1300.0, 4200.0, 0.025, 0.08, 0.01)
    x = layers((_strike(r, 3000.0, 13000.0), 0.0, -5.0), (bang, 0.0, 0.0), (wobble, 0.002, -8.0), (hollow, 0.0, -7.0),
               (rattle, 0.006, -16.0))
    return _slap(_ease(x, -8.0, -4.0), r, -22.0)


@cue('Impact.Glass', variations=4, att='Creature', jitter=0.05, conc=6, level=-10.0)
def impact_glass(v, r):
    # Window glass: the pane's sharp crack as the round goes through (its high modes and a split of fracture), a dull
    # knock of the frame, and a spill of shards tinkling down after, their brightness carried above the ear's touchiest
    # band.
    pane = normalize(M.strike(M.plate(1300.0 * jitter(r, 1, 0.1), child(r, 'pane'), 20, 0.06, 0.6, tilt=0.5),
                              M.hammer(0.00004), 0.12))
    fracture = kit.burst(0.01, child(r, 'fracture'), 0.004, lo=3000.0, hi=12000.0, attack=0.0001)
    frame = kit.thunk(300.0 * jitter(r, 1, 0.1), child(r, 'frame'), 0.03, 0.0005, 5, 0.1)

    def shard(rr, i):
        mm = M.parts(rr.uniform(2200.0, 3200.0), 9000.0, rr, 4, 0.05, 0.02, 0.1)
        return normalize(M.strike(mm, M.hammer(0.00004), 0.1)) * rr.uniform(0.4, 1.0)
    spill = normalize(G.rattle(0.6, child(r, 'spill'), 9 + int(r.integers(0, 5)), 0.5, shard, 0.04, 0.85))
    x = layers((fracture, 0.0, -2.0), (pane, 0.0, -3.0), (frame, 0.0, -8.0), (spill, 0.0, -9.0))
    return _slap(_ease(x, -9.0, -2.0), r, -23.0)


@cue('Impact.Cloth', variations=4, att='Creature', jitter=0.06, conc=8, level=-13.0)
def impact_cloth(v, r):
    # Cloth, canvas, rope or leather: a muffled thwip as the round punches through (a quick rising rush of air in the
    # weave), the fabric snapping taut, and a soft flutter as it settles.
    thwip = kit.whoosh(0.035, child(r, 'thwip'), 500.0, 1800.0, 1.0, 0.25, -2.0, 1.0)
    snap = kit.noise_thump(0.05, child(r, 'snap'), 1100.0, 220.0, 0.018, 1.2, 0.02, 0.0004)
    n = ns(0.24)
    rate = 32.0 * (1.0 + 0.3 * N.smooth_random(n, child(r, 'rate'), 8.0))
    flutter = N.band(n, child(r, 'flutter'), 300.0, 2600.0) * (0.3 + 0.7 * np.abs(np.sin(2 * np.pi * O.phase(rate, n))))
    flutter = normalize(flutter * E.perc(0.24, 0.01, 0.16))
    x = layers((_strike(r, 1200.0, 5000.0), 0.0, -16.0), (thwip, 0.0, 0.0), (snap, 0.004, -3.0), (flutter, 0.012, -11.0))
    return _slap(_ease(F.filt(x, F.lp(6000.0, 0.7), extend=False)), r, -24.0)


@cue('Impact.Hay', variations=4, att='Creature', jitter=0.06, conc=8, level=-12.0)
def impact_hay(v, r):
    # Hay and straw: a soft, rustling thump into the bale, dry straws crackling as they're cut and pushed aside, a few
    # flicked loose and pattering down.
    thump = kit.noise_thump(0.12, child(r, 'thump'), 600.0, 100.0, 0.05, 0.9, 0.04, 0.0008)
    d = 0.16
    n = ns(d)
    crackle = G.grit(d, child(r, 'straws'), 9000.0 * np.exp(-times(n) / 0.05), 1100.0, 6000.0, 1.6, 4)
    crackle = normalize(crackle * E.end_fade(np.ones(n), 0.01))
    rustle = kit.cloth(0.2, child(r, 'rustle'), 400.0, 3500.0, 180.0, 0.05, 0.7)

    def straw(rr, i):
        return kit.thunk(rr.uniform(1200.0, 2400.0), rr, 0.008, 0.0004, 3, 0.2) * rr.uniform(0.4, 1.0)
    scatter = normalize(G.rattle(0.35, child(r, 'scatter'), 4 + int(r.integers(0, 3)), 0.3, straw, 0.06, 0.85))
    x = layers((_strike(r, 1500.0, 6000.0), 0.0, -14.0), (thump, 0.0, 0.0), (crackle, 0.002, -8.0),
               (rustle, 0.003, -10.0), (scatter, 0.0, -18.0))
    return _slap(_ease(x, -6.0, -4.0), r, -24.0)


@cue('Impact.Bone', variations=4, att='Creature', jitter=0.06, conc=8, level=-11.0)
def impact_bone(v, r):
    # Bone and skulls: a dry, hollow crack, brittle and light (a short snap with no ring, the hollow inside answering),
    # and a tick or two of fragments.
    from recipes.creatures import _hollow
    snap = normalize(M.strike(M.parts(900.0, 4200.0, child(r, 'snap'), 6, 0.012, 0.005, 0.35), M.hammer(0.00005), 0.03))
    crack = kit.burst(0.008, child(r, 'crack'), 0.003, lo=800.0, hi=6000.0, attack=0.0001)
    hollow = _hollow(child(r, 'hollow'), 340.0 * jitter(r, 1, 0.15), 0.06, 2600.0)
    knock = kit.thunk(700.0 * jitter(r, 1, 0.12), child(r, 'knock'), 0.018, 0.0002, 5, 0.15)
    bits = _pebbles(0.25, r, 2 + int(r.integers(0, 2)), 0.05, 1200.0, 3800.0)
    x = layers((snap, 0.0, -2.0), (crack, 0.0, -4.0), (hollow, 0.001, -3.0), (knock, 0.0, -6.0), (bits, 0.0, -18.0))
    return _slap(_ease(x, -6.0), r, -24.0)


@cue('Impact.Ricochet', variations=6, att='Creature', jitter=0.04, conc=3, level=-15.0)
def impact_ricochet(v, r):
    # A glancing round leaving the surface: a falling whine as it tumbles away. Grounded, not a cartoon's: the whine is
    # mostly air (narrow noise riding the pitch, a thin tone under it), it starts near 2 kHz (not a cartoon's 4-5, so a
    # fight's worth stays off the ear's touchiest band) and falls fast, flutters irregularly with the tumble, and darkens
    # and fades as it flies off. Pitches and lengths vary by take.
    dur = [0.42, 0.55, 0.36, 0.62, 0.48, 0.7][v] * jitter(r, 1, 0.08)
    n = ns(dur)
    f0 = [2150.0, 2000.0, 2350.0, 1850.0, 2100.0, 1750.0][v] * jitter(r, 1, 0.04)
    f1 = f0 * r.uniform(0.34, 0.42)
    f = O.expsweep(f0, f1, dur, 2.2, n=n) * (1.0 + 0.012 * N.smooth_random(n, child(r, 'wobble'), 30.0))
    tumble = 1.0 - 0.5 * np.abs(N.smooth_random(n, child(r, 'tumble'), r.uniform(45.0, 75.0))) ** 0.7
    air = F.sweep(N.white(n, child(r, 'air')), 'bp', f, 6.0)
    tone = np.sin(2 * np.pi * O.phase(f, n))
    whine = (normalize(air) + 0.25 * normalize(tone)) * tumble
    whine = F.sweep(whine, 'lp', np.geomspace(9000.0, 2600.0, n), 0.7)
    env = E.bp([(0.0, 0.0), (0.004, 0.85), (0.03, 1.0), (dur * 0.5, 0.45), (dur, 0.0)], n=n, curve='cos')
    x = normalize(whine * env)
    x = F.filt(x, F.peak(3300.0, -6.0, 0.6), F.hp(500.0), extend=False)
    return _slap(x, r, -18.0, 0.09, 0.22)
