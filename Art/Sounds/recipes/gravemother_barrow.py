"""The Gravemother's Barrow fight (2026-10-19): the grave-clod, the furrow, the unearthed brood and the headstone. All 3D,
in the Gravemother's family (bosses.py: chitin, packed earth and the Sink's cliffs answering) and the spider's.

- Boss.Gravemother.ClodDig (the 1.2 s tell; 1.9 s with its tail): her forelegs tear into the floor, the earth round the
  clod cracks and parts, and at about 1.0 s the clod is wrenched free (the cue's loudest moment), its crumbs falling
  back after. ClodThrow (1 s): the heave and the clod's dark, tumbling whoosh falling away. ClodBurst (1.5-1.8 s): it
  lands (a deep thud, a burst of dirt, rubble rattling). ClodShot (1.4 s): shot in her grip, it bursts on her and she
  screams.
- Boss.Gravemother.StoneHit (6 short takes, 0.2 s: it plays on every bullet on the headstone): a bullet on granite, a
  hard crack, a dense dead knock, chips flying. StoneCrack (1.1 s): the stone reaches a cracked state (the fracture
  running across it, the slab shifting on her back, her roots creaking taut); the code can play it again at the shattered
  state, pitched lower. StoneBreak (3.0-3.2 s): the fight's big moment (the roots snap one after another, the
  slab strains and splits at 0.3 s, grinds off her, and falls at 1.0 s; its pieces clatter, dust pours).
- Boss.Gravemother.Heave (the 2 s tell; 2.7 s with its tail): the ground heaves and splits along a line, dust hissing
  from the crack, everything rising to a crest at 1.8 s. Plough (2.1 s, built to be cut where she stops): the charge
  down it (a launch, earth rushing, legs drumming). BermCrumble (0.8-1.1 s): a berm slumping away.
- Boss.Gravemother.MoundBulge (the 1 s bulge; 1.2 s): earth stirring, a crust crazing, scratching under it. MoundBurst
  (0.7 s): a spiderling digs out (the earth; the spider's own Emerge plays on top). MoundPop (0.5 s): a bulging mound
  shot, a muffled burst and the earth caving in.

Packed earth is soft: dull knocks, no crack, a soil-band hiss torn by a crumbling flutter. Stone is dense: a hard crack
and short rings above 5 kHz, kept off the ear's sharpest band. The big moments go through bosses.py's soft clip and
limiter and the Sink's slap-backs; the small ones sit in a short open-air answer (creatures.py's _outside).
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter, length, fit
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, voice as V, granular as G
from recipes import cue
from recipes import kit
from recipes import creatures as C
from recipes.bosses import _drum, _pressure, _rumble, _sink, _master, _snaps, _stridulate
from recipes.reactions import _scrabble


# --- Pieces ---------------------------------------------------------------------------------------------------------

def _earth(dur, r, lo, hi, t60, attack=0.004, rate=60.0, depth=0.8):
    """Soil flung, sliding or pouring: noise in the soil band, torn by a crumbling flutter, under a decay."""
    n = ns(dur)
    x = N.shaped(n, r, -3.0, lo, hi) * kit.turbulence(n, child(r, 'turb'), rate, depth)
    return normalize(x * E.perc(dur, attack, t60), 0.0)


def _cascade(dur, r, count, start, spread, decay, grain):
    """Things falling and settling: count knocks at random times over spread seconds, crowded early and sparse late
    (a heap settling, not a ball bouncing), each a random strength and fainter the later it falls (decay is the
    share left at spread's end, as a fraction per four spreads: 0.82 fades to about half)."""
    rr = child(r, 'fall')
    out = np.zeros(ns(dur))
    for i, t in enumerate(np.sort(rr.random(count) ** 1.7) * spread + start):
        a = rr.uniform(0.3, 1.0) * decay ** (4.0 * (t - start) / max(spread, 1e-3))
        out = layers(out, (grain(rr, i) * a, float(t), 0.0))
    return out


def _lumps(dur, r, count, start, spread, lo, hi, decay=0.82, t60=0.02):
    """Clods of packed earth landing: dull, soft knocks, each a different size, fading."""
    def grain(rr, i):
        return kit.thunk(rr.uniform(lo, hi), rr, t60, 0.0009, 4, 0.15)
    return normalize(_cascade(dur, r, count, start, spread, decay, grain), 0.0)


def _tonks(dur, r, count, start, spread, lo, hi, decay=0.84, t60=0.045):
    """Pieces of stone landing on stone: each a short dense ring, pitched by its size."""
    def grain(rr, i):
        f = rr.uniform(lo, hi)
        m = M.parts(f, f * 3.2, rr, 4, t60, t60 * 0.4, 0.4)
        return normalize(M.strike(m, M.hammer(0.00015), t60 * 2.0 + 0.01))
    return normalize(_cascade(dur, r, count, start, spread, decay, grain), 0.0)


def _lv(x, ref, rel_db):
    """x set rel_db against ref's loudest 10 ms, so a noise layer never buries the thud it sits under."""
    return kit.set_level(x, ref, rel_db, 0.01)


def _chip(r, lo, hi, t60=0.012):
    """A chip of stone flying off and ticking away: a very short dense ring."""
    return normalize(M.strike(M.parts(lo, hi, r, 3, t60, t60 * 0.5, 0.3), M.hammer(0.00005), 0.03), 0.0)


def _snap_run(dur, r, rate0, rate1, lo, hi, t60):
    """A crack running: separate snaps coming faster (or slower) over dur."""
    return _snaps(dur, r, lambda t: rate0 + (rate1 - rate0) * (t / dur), lo, hi, t60)


# --- The Grave Clod -------------------------------------------------------------------------------------------------

@cue('Boss.Gravemother.ClodDig', variations=2, att='Gun', cls='Effects', jitter=0.04, conc=1, level=-8.0)
def clod_dig(v, r):
    # The tell: her forelegs rake into the floor (claws biting packed earth, each stroke a bite and a drag of grit,
    # quicker and harder as she goes), the earth round the clod cracks and parts (snaps coming faster), her joints creak
    # under the load, and at about 1 s the clod is wrenched free, the cue's peak: fibres ripping, a suck and a thump of
    # weight, soil crumbling off it and a few lumps falling back.
    rr = child(r, 'rake')
    rake = np.zeros(1)
    tt, i = 0.0, 0
    while tt < 0.9:
        u = tt / 0.9
        d = 0.16 - 0.05 * u
        stroke = G.grit(d, child(rr, 'g', i), 6500.0, 220.0, 2400.0, 1.3, 4) * E.ar(d, d * 0.3, d * 0.6, 1.5)
        stroke = normalize(F.filt(stroke, F.lpn(2800.0, 4), extend=False), 0.0)
        bite = kit.thunk(jitter(rr, 190.0 + 45.0 * (i % 2), 0.1), child(rr, 'b', i), 0.05, 0.002, 5, 0.1)
        rake = layers(rake, (layers(stroke, (bite, 0.0, -2.0)), tt, -12.0 + 8.0 * u - 2.0 * rr.random()))
        tt += (0.2 - 0.08 * u) * rr.uniform(0.88, 1.12)
        i += 1
    strain = kit.creak(1.0, child(r, 'strain'), 20.0, 34.0, (190.0, 440.0, 980.0), 5.0, 0.35)
    m = ns(1.3)
    tm = times(m)
    load = F.filt(N.pink(m, child(r, 'load')), F.lpn(150.0, 4), F.hp(32.0, 0.7), extend=False)
    load = load * kit.turbulence(m, child(r, 'lt'), 30.0, 0.5) * np.clip(tm / 1.0, 0.0, 1.0) ** 1.6 \
        * np.clip((1.3 - tm) / 0.25, 0.0, 1.0)
    parts = _snap_run(0.85, child(r, 'snaps'), 3.0, 37.0, 220.0, 1900.0, 0.03)
    tw = 0.98 + 0.03 * v
    rip = kit.creak(0.2, child(r, 'rip'), 70.0, 150.0, (500.0, 1100.0, 2000.0), 3.0, 0.4)
    tear = kit.whoosh(0.24, child(r, 'tear'), 300.0, 1800.0, 0.8, 0.5, -3.0)
    weight = layers(_pressure(child(r, 'push'), 0.008, 0.1, 190.0),
                    (kit.noise_thump(0.3, child(r, 'suck'), 650.0, 70.0, 0.14, 1.0, 0.1), 0.0, -1.0))
    crumble = _earth(0.55, child(r, 'crumble'), 250.0, 2400.0, 0.25, 0.004, 50.0, 0.85)
    fall = _lumps(0.5, child(r, 'fall'), 5, 0.12, 0.35, 160.0, 420.0)
    x = layers((rake, 0.0, 0.0), (strain, 0.05, -9.0), (normalize(load), 0.0, -7.0), (normalize(parts), 0.2, -6.0),
               (rip, tw - 0.12, -3.0), (tear, tw - 0.1, -7.0), (weight, tw, 3.0), (_lv(crumble, weight, -7.0), tw + 0.05, 0.0),
               (_lv(fall, weight, -11.0), tw + 0.15, 0.0))
    x = F.filt(x, F.lowshelf(70.0, -3.0), F.hp(30.0), F.peak(3300.0, -3.0, 0.8), extend=False)
    return _master(_sink(x, r, -16.0, 0.7), 2.0, 1.0)


@cue('Boss.Gravemother.ClodThrow', variations=2, att='Gun', cls='Effects', jitter=0.04, conc=1, level=-9.0)
def clod_throw(v, r):
    # The lob: she heaves the clod off her forelegs (a shove of air and weight, a chitin joint cracking through, a
    # hiss of breath out of her spiracles) and it flies off, a heavy lump tumbling through the air: a dark whoosh that
    # falls away as it goes, pulsing as it spins, shedding crumbs.
    push = _pressure(child(r, 'push'), 0.007, 0.1, 210.0)
    weight = kit.noise_thump(0.3, child(r, 'weight'), 900.0, 70.0, 0.14, 1.0, 0.1)
    joint = kit.thunk(jitter(r, 340.0, 0.1), child(r, 'joint'), 0.05, 0.0004, 6, 0.1)
    n = ns(0.3)
    breath = V.hiss(n, child(r, 'breath'), (1800.0, 3000.0, 5200.0), levels=(0.7, 0.6, 1.0)) * E.perc(0.3, 0.01, 0.22)
    breath = F.sweep(breath, 'lp', O.expsweep(6000.0, 1800.0, 0.3, n=n), 0.8)
    heave = layers((push, 0.0, 0.0), (weight, 0.0, -1.0), (joint, 0.0, -6.0), (normalize(breath), 0.0, -9.0))
    fd = 0.6 + 0.04 * v
    nf = ns(fd)
    fly = F.sweep(N.shaped(nf, child(r, 'fly'), -3.0), 'bp', O.expsweep(1200.0, 280.0, fd, 1.3, n=nf), 0.8)
    spin = 0.5 + 0.5 * np.sin(2.0 * np.pi * O.phase(O.expsweep(15.0, 8.0, fd, 1.0, n=nf), nf) + 6.28 * r.random())
    fly = normalize(fly * (0.55 + 0.45 * spin) * E.bp([(0.0, 0.0), (0.04, 1.0), (0.16, 0.85), (fd, 0.0)], n=nf, curve='cos'), 0.0)
    crumbs = G.grit(fd, child(r, 'crumbs'), 900.0 * np.exp(-times(nf) / 0.2) + 40.0, 300.0, 2600.0, 1.5, 4) * E.perc(fd, 0.01, 0.4)
    x = layers((heave, 0.0, 0.0), (_lv(fly, heave, -9.0), 0.03, 0.0), (normalize(crumbs), 0.06, -18.0))
    x = F.filt(x, F.lowshelf(70.0, -3.0), F.hp(32.0), F.peak(3300.0, -3.0, 0.8), extend=False)
    return _master(_sink(x, r, -16.0, 0.8), 1.0, 1.0)


@cue('Boss.Gravemother.ClodBurst', variations=3, att='Gun', cls='Effects', jitter=0.04, conc=2, level=-4.0)
def clod_burst(v, r):
    # The clod lands and bursts: a deep thud (the push of air, the ground taking the weight, the packed lump itself
    # knocking), the dirt bursting out in a dark spray with dust rolling up behind it, and rubble rattling down round
    # it, clods and small stones, fewer and fainter. Earth, not shell: a whump with no crack to it.
    core = layers(_pressure(child(r, 'push'), 0.011, 0.14, 200.0),
                  (kit.noise_thump(0.5, child(r, 'thud'), 1000.0, 52.0, 0.24, 1.0, 0.12), 0.0, -1.0),
                  (kit.thunk(jitter(r, 230.0, 0.1), child(r, 'lump'), 0.07, 0.002, 6, 0.1), 0.0, -9.0))
    burst = _earth(0.75, child(r, 'burst'), 220.0, 3000.0, 0.28, 0.006, 45.0, 0.85)
    dust = kit.burst(0.9, child(r, 'dust'), 0.4, lo=150.0, hi=1500.0, color=-3.0, attack=0.03)
    spray = kit.grit_burst(0.55, child(r, 'spray'), 7000.0, 500.0, 3400.0, 0.3, 0.003)
    rubble = layers(_lumps(1.1, child(r, 'lumps'), 9 + v, 0.1, 0.8, 140.0, 520.0, 0.86, 0.025),
                    (_tonks(1.1, child(r, 'tonks'), 6 + v, 0.1, 0.9, 500.0, 1800.0, 0.85), 0.0, -6.0))
    x = layers((core, 0.0, 0.0), (_lv(burst, core, -5.0), 0.004, 0.0), (_lv(dust, core, -12.0), 0.01, 0.0),
               (_lv(spray, core, -10.0), 0.004, 0.0), (_lv(rubble, core, -12.0), 0.06, 0.0))
    x = F.filt(x, F.lowshelf(60.0, -2.0), F.hp(28.0), F.peak(3300.0, -3.0, 0.8), extend=False)
    return _master(_sink(x, r, -13.0, 1.0), 3.0, 2.0)


@cue('Boss.Gravemother.ClodShot', variations=2, att='Gun', cls='Effects', jitter=0.04, conc=1, level=-4.0)
def clod_shot(v, r):
    # A bullet bursts the clod in her grip: a muffled whump of earth right at her face, soil spraying and lumps
    # dropping round her, and at once her scream: a short, pained shriek of her file (higher than her roar and
    # broken by a stutter), a gasp of air, mandibles chattering, legs scrabbling as she reels.
    p = [1.0, 0.9][v]
    core = layers(_pressure(child(r, 'push'), 0.006, 0.09, 260.0),
                  (kit.noise_thump(0.25, child(r, 'whump'), 1100.0, 90.0, 0.1, 1.0, 0.07), 0.0, -1.0))
    spray = kit.grit_burst(0.4, child(r, 'spray'), 7000.0, 500.0, 3600.0, 0.2, 0.002)
    soil = _earth(0.5, child(r, 'soil'), 250.0, 2800.0, 0.16, 0.003, 60.0, 0.85)
    fall = _lumps(0.7, child(r, 'fall'), 6, 0.12, 0.5, 170.0, 520.0)
    dur = 0.95
    n = ns(dur)
    shriek = _stridulate(dur, child(r, 'shriek'), [(0.0, 190 * p), (0.07, 470 * p), (0.3, 400 * p), (0.55, 330 * p), (dur, 150 * p)],
                         (700.0, 1350.0, 2400.0, 3700.0), 5.0, 0.14, 0.4, 14.0 + 2.0 * v, 0.55)
    shriek = shriek * E.bp([(0.0, 0.0), (0.012, 1.0), (0.4, 0.9), (dur, 0.0)], n=n, curve='cos')
    gasp = V.hiss(ns(0.35), child(r, 'gasp'), (1500.0, 2700.0, 4400.0), (2.5, 2.5, 2.0), (1.0, 0.8, 0.5), 500.0) * E.perc(0.35, 0.01, 0.25)
    chat = C.chitter(0.6, child(r, 'chitter'), 40.0, 20.0, 700.0, 2600.0, 0.35, 0.6)
    legs = _scrabble(child(r, 'legs'), 0.0, 8, 0.045, 0.08)
    cry = D.drive(normalize(layers((shriek, 0.0, 0.0), (normalize(gasp), 0.0, -8.0), (normalize(chat), 0.05, -9.0))), 3.5, 'tanh', bias=0.08)
    x = layers((core, 0.0, 2.0), (_lv(spray, core, -8.0), 0.003, 2.0), (_lv(soil, core, -6.0), 0.003, 2.0),
               (_lv(fall, core, -13.0), 0.04, 2.0), (_lv(cry, core, 0.0), 0.07, 0.0), (_lv(legs, core, -13.0), 0.25, 0.0))
    x = F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(7000.0, -4.0), F.hp(40.0), extend=False)
    return _master(_sink(x, r, -14.0, 0.9), 3.0, 2.0)


# --- The headstone --------------------------------------------------------------------------------------------------

@cue('Boss.Gravemother.StoneHit', variations=6, att='Creature', cls='Effects', jitter=0.06, conc=6, level=-11.0)
def stone_hit(v, r):
    # A bullet on granite (it plays on every bullet that lands on the headstone, so it is short and never harsh): a
    # hard crack, the slab's dense dead knock (a heavy stone, held on her back, rings only a few milliseconds), a
    # faint ping above the ear's sharpest band, a thud of weight and a few chips flying off and ticking away.
    f = (400.0, 450.0, 520.0, 580.0, 640.0, 480.0)[v] * jitter(r, 1.0, 0.05)
    crack = kit.burst(0.012, child(r, 'crack'), 0.005, lo=900.0, hi=7000.0, attack=0.0001)
    tock = normalize(M.strike(M.plate(f, child(r, 'slab'), 14, 0.07, 0.7, 1.37, 0.03, 0.6), M.hammer(0.00007), 0.12), 0.0)
    tock = F.filt(tock, F.peak(3300.0, -8.0, 0.7), extend=False)
    ping = normalize(M.strike(M.parts(5200.0, 9500.0, child(r, 'ping'), 4, 0.014, 0.006, 0.2), M.hammer(0.00004), 0.04), 0.0)
    weight = kit.noise_thump(0.09, child(r, 'weight'), 800.0, 120.0, 0.03, 1.0, 0.02)
    cr = child(r, 'chips')
    chips = np.zeros(1)
    t = 0.012
    for i in range(2 + int(cr.integers(0, 4))):
        chips = layers(chips, (_chip(child(cr, 'c', i), cr.uniform(1300.0, 2200.0), cr.uniform(3800.0, 7000.0)), t, -3.0 * i - cr.uniform(0.0, 4.0)))
        t += cr.uniform(0.012, 0.05)
    dust = kit.burst(0.07, child(r, 'dust'), 0.035, lo=400.0, hi=2500.0, color=-2.0)
    x = layers((crack, 0.0, -3.0), (tock, 0.0, 0.0), (ping, 0.0, -6.0), (weight, 0.0, -5.0), (chips, 0.0, -10.0),
               (dust, 0.002, -17.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(8000.0, -3.0), F.hp(60.0), extend=False)
    return C._outside(x, r, -24.0, 0.05, 0.12, 0.25)


@cue('Boss.Gravemother.StoneCrack', variations=2, att='Gun', cls='Effects', jitter=0.04, conc=1, level=-6.0)
def stone_crack(v, r):
    # The headstone splits (its cracked state; the shattered state plays it again, lower): a sharp crack, the
    # fracture running across the slab in snaps that come faster while the slab's ring drops to a lower note (two
    # halves ring where one did), the slab shifting on her back with a dull knock, her roots creaking taut, grit and
    # chips trickling down.
    first = layers(kit.burst(0.02, child(r, 'first'), 0.008, lo=700.0, hi=7000.0, attack=0.0001),
                   (normalize(M.strike(M.parts(1500.0, 7000.0, child(r, 'fp'), 7, 0.03, 0.01, 0.2), M.hammer(0.00004), 0.07), 0.0), 0.0, -4.0))
    run = _snap_run(0.22, child(r, 'run'), 60.0, 440.0, 700.0, 5200.0, 0.014)
    ring1 = normalize(M.strike(M.plate(780.0 * jitter(r, 1, 0.05), child(r, 'ring1'), 16, 0.2, 0.6, 1.37, 0.03, 0.5), M.hammer(0.00008), 0.3), 0.0)
    ring2 = normalize(M.strike(M.plate(520.0 * jitter(r, 1, 0.05), child(r, 'ring2'), 16, 0.28, 0.6, 1.37, 0.03, 0.5), M.hammer(0.0001), 0.4), 0.0)
    shift = layers(kit.noise_thump(0.25, child(r, 'shift'), 700.0, 80.0, 0.12, 1.0, 0.08),
                   (kit.thunk(jitter(r, 300.0, 0.1), child(r, 'knock'), 0.06, 0.0006, 6, 0.1), 0.0, -4.0))
    roots = kit.creak(0.4, child(r, 'roots'), 16.0, 30.0, (190.0, 430.0, 950.0), 5.0, 0.3)
    grit = kit.grit_burst(0.45, child(r, 'grit'), 2600.0, 600.0, 3500.0, 0.28, 0.004)
    chips = _tonks(0.5, child(r, 'chips'), 5, 0.05, 0.4, 900.0, 2400.0, 0.82, 0.02)
    x = layers((first, 0.0, 0.0), (normalize(run), 0.0, -3.0), (ring1, 0.0, -7.0), (ring2, 0.08, -8.0),
               (shift, 0.16, -8.0), (roots, 0.12, -12.0), (grit, 0.1, -14.0), (chips, 0.14, -17.0))
    x = F.filt(x, F.peak(3300.0, -5.0, 0.8), F.lowshelf(70.0, -3.0), F.hp(40.0), extend=False)
    return _master(_sink(x, r, -15.0, 0.9), 3.0, 2.0)


@cue('Boss.Gravemother.StoneBreak', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-1.5)
def stone_break(v, r):
    # The fight's big moment: the roots snap one after another (taut cords parting, fibres tearing), the slab splits
    # with a crack that rings through it and a boom under, grinds off her shell, tips and falls: a huge thud (the push
    # of air, the ground taking tons of granite), then its pieces clatter and rock down, a second settling thud, dust
    # pouring and the valley rolling the whole thing back.
    snaps = np.zeros(1)
    sr = child(r, 'roots')
    for i, (at, db) in enumerate([(0.0, 0.0), (0.07, -2.0), (0.14, -1.0), (0.24, -3.0), (0.32, -2.0), (0.43, -5.0)]):
        cord = normalize(M.strike(M.parts(500.0 + 200.0 * i, 2600.0, child(sr, 'p', i), 4, 0.05, 0.015, 0.3), M.hammer(0.0002), 0.1), 0.0)
        snap = layers(kit.burst(0.015, child(sr, 'b', i), 0.006, lo=700.0, hi=5000.0, attack=0.0002), (cord, 0.0, -4.0),
                      (kit.thunk(sr.uniform(300.0, 700.0), child(sr, 'k', i), 0.045, 0.0004, 6, 0.1), 0.0, -3.0))
        snaps = layers(snaps, (snap, at + 0.01 * sr.random(), db))
    fibres = kit.creak(0.5, child(r, 'fibres'), 90.0, 40.0, (600.0, 1300.0, 2400.0), 3.0, 0.4)
    ts = 0.3
    strain = F.filt(N.pink(ns(0.7), child(r, 'strain')), F.lpn(180.0, 4), F.hp(35.0, 0.7), extend=False)
    strain = strain * E.bp([(0.0, 0.0), (0.3, 1.0), (0.7, 0.0)], n=ns(0.7), curve='cos')
    split = layers(kit.burst(0.04, child(r, 'split'), 0.015, lo=500.0, hi=6000.0, attack=0.0001),
                   (_snap_run(0.4, child(r, 'run'), 500.0, 20.0, 500.0, 4200.0, 0.02), 0.0, -5.0),
                   (kit.grit_burst(0.5, child(r, 'grit'), 8000.0, 250.0, 3000.0, 0.25, 0.001), 0.0, -9.0))
    gong = layers(normalize(M.strike(M.plate(300.0, child(r, 'gong'), 16, 0.45, 0.5, 1.37, 0.03, 0.5), M.hammer(0.0002), 0.6), 0.0),
                  (normalize(M.strike(M.plate(110.0, child(r, 'low'), 12, 0.6, 0.4, 1.37, 0.03, 0.5), M.hammer(0.0004), 0.8), 0.0), 0.0, -5.0))
    boom = kit.noise_thump(0.6, child(r, 'boom'), 700.0, 45.0, 0.3, 1.0, 0.2)
    grind = layers(kit.creak(0.5, child(r, 'grind'), 14.0, 28.0, (140.0, 340.0, 800.0), 4.0, 0.3),
                   (_earth(0.5, child(r, 'gr'), 200.0, 1800.0, 0.5, 0.1, 30.0, 0.7), 0.0, -8.0))
    tf = 1.0
    fall = layers(_pressure(child(r, 'push'), 0.016, 0.18, 170.0),
                  (kit.noise_thump(0.9, child(r, 'thud'), 1100.0, 40.0, 0.45, 1.0, 0.22), 0.0, -1.0),
                  (_drum(46.0 * jitter(r, 1, 0.04), 1.0, child(r, 'drum'), 0.6, 0.12, 0.003), 0.0, -5.0),
                  (kit.thunk(jitter(r, 420.0, 0.1), child(r, 'slab'), 0.12, 0.0003, 7, 0.1), 0.0, -4.0),
                  (_rumble(1.8, child(r, 'rumble'), 220.0, 1.0, 0.12, 3.0), 0.0, -13.0))
    shards = layers(_tonks(1.6, child(r, 'shards'), 15, 0.04, 1.3, 380.0, 2200.0, 0.88, 0.07),
                    (_lumps(1.4, child(r, 'rocks'), 6, 0.05, 1.0, 120.0, 380.0, 0.85, 0.04), 0.0, -3.0))
    settle = layers(kit.noise_thump(0.35, child(r, 'settle'), 600.0, 60.0, 0.18, 1.0, 0.12),
                    (kit.thunk(jitter(r, 360.0, 0.1), child(r, 'sk'), 0.08, 0.0004, 6, 0.1), 0.0, -4.0))
    dust = layers(_earth(1.8, child(r, 'dust'), 140.0, 1900.0, 0.8, 0.08, 36.0, 0.75),
                  (kit.burst(1.4, child(r, 'puff'), 0.6, lo=120.0, hi=1200.0, color=-3.0, attack=0.06), 0.0, -3.0))
    x = layers((snaps, 0.0, 0.0), (normalize(strain, 0.0), 0.0, -5.0), (fibres, 0.0, -4.0), (split, ts, 0.0), (gong, ts + 0.002, -4.0), (_lv(boom, split, -4.0), ts, 0.0),
               (grind, 0.55, -5.0), (fall, tf, 4.0), (_lv(shards, fall, -9.0), tf + 0.04, 0.0), (_lv(settle, fall, -7.0), tf + 0.5, 0.0),
               (_lv(dust, fall, -12.0), tf + 0.02, 0.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.lowshelf(60.0, -2.0), F.hp(26.0), extend=False)
    return _master(_sink(x, r, -11.0, 1.4), 4.0, 3.0)


# --- The Furrow -----------------------------------------------------------------------------------------------------

@cue('Boss.Gravemother.Heave', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-6.0)
def heave(v, r):
    # The furrow's tell (2 s): the ground heaves and splits along a line. A first crack and a thump, then a dark
    # rumble lifting and opening up, the earth groaning in long creaks, cracks running along the line faster and
    # faster, dust hissing out of the crack (louder, brighter) and grit shaking loose, rising to a crest.
    dur = 2.1
    n = ns(dur)
    t = times(n)
    rise = np.clip(t / 1.9, 0.0, 1.0)
    rumble = F.sweep(N.pink(n, child(r, 'rumble')), 'lp', O.expsweep(60.0, 260.0, dur, 1.0, n=n), 0.9)
    rumble = normalize(rumble * (0.5 + 0.5 * np.abs(N.smooth_random(n, child(r, 'rw'), 6.0))) * rise ** 1.4)
    first = layers(kit.grit_burst(0.12, child(r, 'first'), 9000.0, 500.0, 3500.0, 0.06, 0.0005),
                   (kit.noise_thump(0.25, child(r, 'ft'), 800.0, 65.0, 0.12, 1.0, 0.06), 0.0, -2.0))
    groans = np.zeros(1)
    for i, at in enumerate([0.05, 0.6, 1.1]):
        g = kit.creak(0.9, child(r, 'groan', i), 8.0 + 4.0 * i, 20.0 + 8.0 * i, (110.0 * (1 + 0.15 * i), 260.0, 620.0), 5.0, 0.3)
        groans = layers(groans, (g, at, -2.0 * (2 - i)))
    parts = _snaps(dur, child(r, 'snaps'), lambda tt: 4.0 + 40.0 * (tt / dur) ** 1.8, 250.0, 2400.0, 0.03)
    dust = V.hiss(n, child(r, 'dust'), (1200.0, 2200.0, 3600.0), (2.5, 2.5, 2.0), (0.8, 1.0, 0.6), 500.0)
    dust = dust * kit.turbulence(n, child(r, 'dt'), 90.0, 0.7) * np.clip(t / 1.9, 0.0, 1.0) ** 1.6
    dust = F.sweep(dust, 'lp', O.expsweep(1800.0, 5200.0, dur, 1.0, n=n), 0.8)
    grit = G.grit(dur, child(r, 'grit'), 150.0 + 2800.0 * (t / dur) ** 2, 300.0, 2600.0, 1.6, 4)
    crest = layers(kit.grit_burst(0.12, child(r, 'crest'), 9000.0, 400.0, 3500.0, 0.08, 0.0005),
                   (kit.noise_thump(0.35, child(r, 'ct'), 900.0, 60.0, 0.18, 1.0, 0.1), 0.0, -1.0))
    # Everything but the first crack and the crest grows together, about 9 dB from the start to 1.8 s: it is a rise,
    # but it never goes quiet, since the player must hear the tell from its first moment.
    bed = layers((normalize(rumble), 0.0, -3.0), (normalize(groans), 0.0, -5.0), (normalize(parts), 0.0, -5.0),
                 (normalize(dust), 0.0, -9.0), (normalize(grit), 0.0, -11.0))
    bed = fit(bed, n) * (0.38 + 0.62 * np.clip(t / 1.8, 0.0, 1.0) ** 1.3)
    x = layers((first, 0.0, -3.0), (bed, 0.0, 2.0), (crest, 1.78, -2.0))
    x = F.filt(x, F.lowshelf(80.0, -3.0), F.hp(30.0), extend=False)
    x = x[:n] * E.bp([(0.0, 1.0), (dur - 0.12, 1.0), (dur, 0.0)], n=n)
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False)
    return _master(_sink(x, r, -15.0, 0.8), 3.0, 1.5)


@cue('Boss.Gravemother.Plough', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-5.0)
def plough(v, r):
    # The charge down the line (1.8 s; the code can stop it where she stops): a shove as she launches, then legs
    # drumming (eight feet in pairs, the strides quickening to her top pace and slowing out of it), earth rushing and
    # being thrown up either side (a dark roar of soil, gravel and clods knocking) and the berms heaped behind.
    dur = 1.8
    n = ns(dur)
    t = times(n)
    lr = child(r, 'legs')
    fore = [kit.noise_thump(0.14, child(lr, 'f', k), 700.0, 85.0, 0.07, 1.0, 0.05) for k in range(4)]
    hind = [kit.noise_thump(0.14, child(lr, 'h', k), 600.0, 80.0, 0.07, 1.0, 0.05) for k in range(4)]
    legs = np.zeros(1)
    tt, i = 0.12, 0
    while tt < dur - 0.25:
        u = np.interp(tt, [0.0, 0.5, 1.4, 1.8], [0.9, 1.0, 1.0, 0.7])
        pair = layers((fore[i % 4], 0.0, 0.0), (kit.thunk(jitter(lr, 380.0, 0.1), child(lr, 'tk', i), 0.025, 0.0003, 5, 0.1), 0.0, -8.0))
        legs = layers(legs, (pair, tt, -1.5 * (i % 2) - 6.0 * (1.0 - u)))
        legs = layers(legs, (hind[i % 4], tt + lr.uniform(0.03, 0.045), -4.0 - 1.5 * (i % 2) - 6.0 * (1.0 - u)))
        tt += np.interp(tt, [0.0, 0.5, 1.4, 1.8], [0.16, 0.105, 0.11, 0.17]) * lr.uniform(0.92, 1.08)
        i += 1
    shape = E.bp([(0.0, 0.0), (0.12, 0.7), (0.5, 1.0), (1.4, 0.9), (dur, 0.0)], n=n, curve='cos')
    rush = N.shaped(n, child(r, 'rush'), -3.0, 120.0, 3200.0) * kit.turbulence(n, child(r, 'rt'), 90.0, 0.7) * shape
    gravel = G.grit(dur, child(r, 'gravel'), 1500.0, 250.0, 2800.0, 1.4, 4) * shape
    shove = F.filt(N.pink(n, child(r, 'shove')), F.lpn(110.0, 4), F.hp(30.0, 0.7), extend=False) \
        * kit.turbulence(n, child(r, 'st'), 25.0, 0.5) * shape
    thrown = _lumps(dur - 0.3, child(r, 'thrown'), 22, 0.15, dur - 0.5, 130.0, 460.0, 0.97)
    launch = layers(_pressure(child(r, 'push'), 0.009, 0.12, 200.0),
                    (kit.noise_thump(0.3, child(r, 'lt'), 800.0, 70.0, 0.14, 1.0, 0.08), 0.0, -1.0))
    x = layers((launch, 0.0, 0.0), (_lv(legs, launch, -2.0), 0.0, 0.0), (_lv(rush, launch, -11.0), 0.0, 0.0),
               (_lv(gravel, launch, -18.0), 0.0, 0.0), (_lv(shove, launch, -9.0), 0.0, 0.0), (_lv(thrown, launch, -14.0), 0.0, 0.0))
    x = x[:n] * E.bp([(0.0, 1.0), (dur - 0.2, 1.0), (dur, 0.0)], n=n)
    x = F.filt(x, F.lowshelf(70.0, -2.0), F.hp(30.0), F.peak(3300.0, -3.0, 0.8), extend=False)
    return _master(_sink(x, r, -15.0, 0.7), 3.0, 1.5)


@cue('Boss.Gravemother.BermCrumble', variations=4, att='Creature', cls='Effects', jitter=0.08, conc=4, level=-14.0)
def berm_crumble(v, r):
    # A berm slumping away: a soft thump as its top gives, soil sliding and trickling off its flanks, lumps rolling
    # down and knocking, pebbles ticking; low and soft, with no crack, off the ear's sharpest band.
    slump = kit.noise_thump(0.3, child(r, 'slump'), 520.0, 70.0, 0.14, 1.0, 0.1, 0.008)
    slide = _earth(1.1, child(r, 'slide'), 140.0, 1900.0, 0.55, 0.06, 28.0, 0.9)
    m = ns(0.9)
    trickle = G.grit(0.9, child(r, 'trickle'), 1400.0 * np.exp(-times(m) / 0.5) + 120.0, 400.0, 3000.0, 1.5, 4) \
        * E.perc(0.9, 0.02, 0.6)
    lumps = _lumps(1.0, child(r, 'lumps'), 5 + int(r.integers(0, 3)), 0.12, 0.7, 130.0, 380.0, 0.85, 0.03)
    x = layers((slump, 0.0, 0.0), (_lv(slide, slump, -4.0), 0.02, 0.0), (_lv(normalize(trickle), slump, -13.0), 0.05, 0.0),
               (_lv(lumps, slump, -9.0), 0.08, 0.0))
    x = F.filt(x, F.lp(3800.0, 0.7), F.hp(40.0), extend=False)
    return C._outside(x, r, -22.0, 0.06, 0.15, 0.4)


# --- Unearthed ------------------------------------------------------------------------------------------------------

@cue('Boss.Gravemother.MoundBulge', variations=3, att='Creature', cls='Effects', jitter=0.05, conc=4, level=-13.0)
def mound_bulge(v, r):
    # Earth stirring (1 s): a first tick of the crust, a low grumble swelling as the mound heaves up from under, the
    # crust crazing in soft snaps that come faster, tiny legs scratching and scrabbling beneath it (muffled by the
    # earth, quicker), soil sliding down the swelling flanks. It crests at about 0.8 s and settles.
    dur = 1.05
    n = ns(dur)
    t = times(n)
    tick = kit.burst(0.012, child(r, 'tick'), 0.006, lo=300.0, hi=2600.0, attack=0.0003)
    crest = np.clip(t / 0.8, 0.0, 1.0) ** 1.4 * np.clip((dur - t) / 0.25, 0.0, 1.0)
    grumble = F.filt(N.pink(n, child(r, 'grumble')), F.lpn(230.0, 4), F.hp(45.0, 0.7), extend=False)
    grumble = grumble * (0.5 + 0.5 * np.abs(N.smooth_random(n, child(r, 'gw'), 7.0))) * crest
    crazing = _snap_run(dur - 0.1, child(r, 'craze'), 3.0, 24.0, 250.0, 1700.0, 0.025)
    scratch = F.filt(_scrabble(child(r, 'scratch'), 0.0, 12, 0.12, 0.05, 700.0, 1300.0), F.lpn(2200.0, 4), extend=False)
    scratch = scratch * E.bp([(0.0, 0.0), (0.15, 0.0), (0.6, 1.0), (1.0, 0.5)], n=length(scratch), curve='lin')
    soil = _earth(dur, child(r, 'soil'), 150.0, 1600.0, 0.6, 0.15, 24.0, 0.9) * crest
    x = layers((tick, 0.0, -14.0), (normalize(grumble), 0.0, -6.0), (normalize(crazing), 0.0, -3.0), (normalize(scratch), 0.1, 0.0),
               (normalize(soil), 0.0, -9.0))
    x = F.filt(x, F.lp(3600.0, 0.7), F.hp(40.0), extend=False)
    return C._outside(x, r, -23.0, 0.06, 0.15, 0.4)


@cue('Boss.Gravemother.MoundBurst', variations=3, att='Creature', cls='Effects', jitter=0.05, conc=4, level=-9.0)
def mound_burst(v, r):
    # A spiderling digs out: the crust bursting, a dull thump of earth thrown up, soil spraying and a puff of dust
    # behind it, lumps of the mound falling in, little legs scrabbling for a hold. (The spider's own emerge cue plays on
    # top: this is the mound.)
    crack = kit.burst(0.02, child(r, 'crack'), 0.008, lo=250.0, hi=3000.0, attack=0.0003)
    thump = kit.noise_thump(0.2, child(r, 'thump'), 700.0, 90.0, 0.09, 1.0, 0.07)
    soil = _earth(0.45, child(r, 'soil'), 200.0, 2800.0, 0.18, 0.004, 55.0, 0.85)
    puff = kit.burst(0.3, child(r, 'puff'), 0.15, lo=150.0, hi=1600.0, color=-3.0, attack=0.01)
    spray = kit.grit_burst(0.3, child(r, 'spray'), 4500.0, 500.0, 3200.0, 0.15, 0.002)
    lumps = _lumps(0.55, child(r, 'lumps'), 5 + v, 0.04, 0.35, 160.0, 480.0)
    legs = F.filt(_scrabble(child(r, 'legs'), 0.0, 6 + v, 0.05, 0.085), F.lpn(1900.0, 4), extend=False)
    x = layers((thump, 0.0, 0.0), (crack, 0.0, -6.0), (_lv(soil, thump, -3.0), 0.003, 0.0), (_lv(puff, thump, -12.0), 0.01, 0.0),
               (_lv(spray, thump, -10.0), 0.004, 0.0), (_lv(lumps, thump, -10.0), 0.05, 0.0), (_lv(legs, thump, -17.0), 0.1, 0.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.lp(5000.0, 0.7), F.hp(40.0), extend=False)
    return C._outside(x, r, -21.0, 0.06, 0.15, 0.4)


@cue('Boss.Gravemother.MoundPop', variations=3, att='Creature', cls='Effects', jitter=0.05, conc=4, level=-9.0)
def mound_pop(v, r):
    # A bulging mound shot: a muffled burst (the bullet and the spiderling's shell going through earth, a dull thump
    # and a crunch with the highs taken off), the mound caving in on it (soil sliding, lumps falling), a puff of dust.
    thump = kit.noise_thump(0.2, child(r, 'thump'), 650.0, 80.0, 0.09, 1.0, 0.07, 0.0015)
    crunch = F.filt(C._chitin(child(r, 'shell'), 450.0 * jitter(r, 1, 0.12), 0.03, 3), F.lp(1500.0, 0.7), extend=False)
    cave = _earth(0.55, child(r, 'cave'), 150.0, 1900.0, 0.22, 0.01, 40.0, 0.85)
    lumps = _lumps(0.5, child(r, 'lumps'), 6, 0.05, 0.3, 140.0, 420.0)
    puff = kit.burst(0.3, child(r, 'puff'), 0.14, lo=150.0, hi=1400.0, color=-3.0, attack=0.01)
    x = layers((thump, 0.0, 0.0), (crunch, 0.0, -5.0), (_lv(cave, thump, -5.0), 0.015, 0.0), (_lv(lumps, thump, -9.0), 0.06, 0.0),
               (_lv(puff, thump, -12.0), 0.01, 0.0))
    x = F.filt(x, F.lp(4200.0, 0.7), F.hp(40.0), extend=False)
    return C._outside(x, r, -22.0, 0.06, 0.15, 0.4)
