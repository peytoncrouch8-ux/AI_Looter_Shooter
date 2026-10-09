"""The lootable world (Docs/Polish/BorderlandsComparison.md, item 10): crates and barrels bursting, a grave dug up, a
coffin pried open and its lid shoved off, a mailbox's tin door, a footlocker's lid. Physical first: boards cracking
and splintering, nails squealing out of pine, staves and a hoop springing apart, a spade biting packed earth, rusty
tin; the pieces clattering down after a break, so it lands as an event. Heard often (every crate in a yard), so the
cracks are kept off the ear's sharpest band.
"""
import numpy as np

from lib.core import SR, ns, layers, normalize, child, jitter
from lib import env as E, filters as F, modal as M, reverb as R, granular as G
from recipes import cue
from recipes import kit


def _outside(x, r, rel_db=-21.0, s1=0.06, s2=0.15):
    """A short outdoor answer, so a break sits in the yard rather than in a void."""
    ir = R.outdoor(child(r, 'ir'), dur=0.7, slaps=((s1, -4.0), (s2, -9.0)), tail_t60=0.45, tail_db=-12.0,
                   tail_start=0.02, tail_peak=0.08)
    wet = F.convolve(F.filt(x, F.hpn(220.0, 2), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.02))


def _soften(x):
    """The 2-5 kHz band eased: a break or a dig comes again and again in a yard."""
    return F.filt(x, F.peak(3300.0, -4.0, 0.9), extend=False)


def _board_crack(r, f0):
    """One board giving way: the crack, the plank's knock, its hollow, fibres tearing."""
    crack = kit.burst(0.009, child(r, 'crack'), 0.004, lo=1100.0, hi=8500.0)
    knock = kit.thunk(f0, child(r, 'knock'), 0.1 * jitter(r, 1, 0.2), 0.00025, 7, 0.1)
    hollow = kit.thump(f0 * 0.62, f0 * 0.42, 0.12, 0.07)
    fibres = kit.creak(0.09, child(r, 'fibres'), 900.0, 260.0, (1200.0, 2300.0, 3900.0), 5.0)
    return normalize(layers((crack, 0.0, -3.0), (knock, 0.0, 0.0), (hollow, 0.0, -8.0), (fibres, 0.004, -15.0)), 0.0)


def _clatter(r, count, spread, start, lo=260.0, hi=900.0):
    """Broken pieces of board coming down and knocking about: wooden knocks, fewer and fainter, a bounce or two each."""
    def grain(rr, i):
        f0 = float(np.exp(rr.uniform(np.log(lo), np.log(hi))))
        knock = kit.thunk(f0, rr, 0.05 * jitter(rr, 1, 0.3), 0.0005, 5, 0.12)
        thud = kit.noise_thump(0.06, rr, 900.0, 160.0, 0.03, 1.0, 0.02)
        return normalize(layers(knock, (thud, 0.0, -9.0)), 0.0)
    return normalize(G.rattle(spread + 0.15, child(r, 'clatter'), count, spread, grain, start, 0.8), 0.0)


@cue('Loot.Break.Crate', variations=4, att='Creature', jitter=0.05, conc=4, level=-8.0)
def break_crate(v, r):
    # A crate bursting under a blow: two or three boards cracking a hair apart, the nails' short squeal, splinters
    # flying, the body's dull thump, then its pieces clattering down round it.
    f0 = 210.0 + 110.0 * r.random()
    parts = [(_board_crack(child(r, 'b1'), f0), 0.0, 0.0)]
    t = 0.0
    for i in range(2 + int(r.integers(0, 2))):
        t += r.uniform(0.012, 0.035)
        parts.append((_board_crack(child(r, 'b', i), f0 * r.uniform(1.15, 1.7)), t, r.uniform(-7.0, -3.0)))
    body = kit.noise_thump(0.16, child(r, 'body'), 700.0, 95.0, 0.08, 1.0, 0.05)
    splinters = kit.grit_burst(0.32, child(r, 'splinters'), 6500.0, 2000.0, 8000.0, 0.12)
    squeal = kit.creak(0.07, child(r, 'squeal'), 260.0, 140.0, (1900.0, 3400.0, 5200.0), 14.0, 0.2)
    parts += [(body, 0.0, -5.0), (splinters, 0.003, -11.0), (squeal, 0.02, -22.0)]
    parts.append((_clatter(child(r, 'down'), 6 + int(r.integers(0, 4)), 0.55, 0.2), 0.0, -9.0))
    return _outside(_soften(layers(*parts)), r)


@cue('Loot.Break.Barrel', variations=4, att='Creature', jitter=0.05, conc=4, level=-8.0)
def break_barrel(v, r):
    # A barrel bursting: its staves cracking and springing out, a hoop ringing as it springs off and drops, the
    # empty body's boom, the staves clattering down.
    f0 = 170.0 + 80.0 * r.random()
    parts = [(_board_crack(child(r, 'b1'), f0), 0.0, 0.0)]
    t = 0.0
    for i in range(3):
        t += r.uniform(0.01, 0.03)
        parts.append((_board_crack(child(r, 'b', i), f0 * r.uniform(1.1, 1.5)), t, r.uniform(-8.0, -4.0)))
    boom = kit.thump(f0 * 0.5, f0 * 0.32, 0.22, 0.12)
    body = kit.noise_thump(0.2, child(r, 'body'), 600.0, 80.0, 0.1, 1.2, 0.06)
    # The hoop: a thin steel band's few rings, struck as it springs, then its clank landing.
    band = M.parts(420.0, 2600.0, child(r, 'band'), 8, 0.45, 0.2, 0.5)
    spring = normalize(M.strike(band, M.hammer(0.0002), 0.6))
    spring = F.filt(spring, F.peak(3200.0, -8.0, 0.6), F.highshelf(6000.0, -5.0), extend=False)
    land = normalize(M.strike(M.parts(500.0, 3000.0, child(r, 'land'), 7, 0.2, 0.1, 0.5), M.hammer(0.0001), 0.3))
    land = F.filt(land, F.peak(3200.0, -8.0, 0.6), extend=False)
    splinters = kit.grit_burst(0.3, child(r, 'splinters'), 5000.0, 1800.0, 7000.0, 0.12)
    t_land = 0.32 + 0.1 * r.random()
    parts += [(boom, 0.0, -9.0), (body, 0.0, -6.0), (spring, 0.01, -12.0), (splinters, 0.004, -12.0),
              (land, t_land, -14.0), (land, t_land + 0.09, -22.0)]
    parts.append((_clatter(child(r, 'down'), 7 + int(r.integers(0, 3)), 0.6, 0.22, 200.0, 700.0), 0.0, -8.0))
    return _outside(_soften(layers(*parts)), r)


def _spade_bite(r):
    """The spade driven into packed earth: a gritty chunk, a small scrape of iron on a stone, the soil crumbling."""
    chunk = kit.noise_thump(0.11, child(r, 'chunk'), 520.0, 110.0, 0.06, 1.1, 0.05)
    grit = kit.grit_burst(0.16, child(r, 'grit'), 4000.0, 300.0, 3200.0, 0.07)
    stone = kit.scrape(0.05, child(r, 'stone'), 1700.0, 1300.0, 9.0, 0.7, attack=0.1, release=0.6)
    return normalize(layers((chunk, 0.0, 0.0), (grit, 0.004, -6.0), (stone, 0.008, -18.0)), 0.0)


def _dirt_thrown(r):
    """A spadeful landing on the heap: a soft heavy pat and the crumbs pattering down after it."""
    pat = kit.noise_thump(0.09, child(r, 'pat'), 420.0, 120.0, 0.05, 1.0, 0.04)
    crumbs = kit.grit_burst(0.3, child(r, 'crumbs'), 3500.0, 250.0, 2600.0, 0.15)
    return normalize(layers((pat, 0.0, 0.0), (crumbs, 0.01, -5.0)), 0.0)


@cue('Loot.Grave.Dig', variations=3, att='Near', jitter=0.04, conc=2, level=-9.0)
def grave_dig(v, r):
    # Digging up an old grave in a little over a second: three bites of the spade, each spadeful thrown on the heap,
    # then the spade's iron striking the coffin's lid with a hollow knock.
    parts = []
    for i, t in enumerate((0.0, 0.34, 0.62)):
        t += r.uniform(-0.02, 0.02)
        parts.append((_spade_bite(child(r, 'bite', i)), t, -1.0 * i))
        parts.append((_dirt_thrown(child(r, 'throw', i)), t + 0.17 + r.uniform(0.0, 0.03), -5.0))
    t_lid = 0.93 + 0.04 * r.random()
    lid = kit.thunk(175.0 * jitter(r, 1, 0.08), child(r, 'lid'), 0.16, 0.0006, 7, 0.1)
    hollow = kit.thump(120.0, 85.0, 0.18, 0.1)
    clink = kit.metal_click(child(r, 'clink'), 1300.0, 4200.0, 0.05, 0.0001, 5, -14.0)
    parts += [(lid, t_lid, 0.0), (hollow, t_lid, -6.0), (clink, t_lid, -16.0)]
    return _outside(_soften(layers(*parts)), r, -24.0)


@cue('Loot.Coffin.Pry', variations=3, att='Near', jitter=0.04, conc=2, level=-10.0)
def coffin_pry(v, r):
    # A crowbar's bite under the lid, and the old nails squealing out of the pine as it jumps: twice, the second
    # longer, the boards groaning with them.
    bite = kit.clack(child(r, 'bite'), 380.0, 1400.0, 4800.0, 0.04, 0.05, -2.0, -10.0, 0.00015)
    parts = [(bite, 0.0, 0.0)]
    for i, (t, dur, db) in enumerate(((0.04, 0.16, -4.0), (0.29, 0.22, 0.0))):
        squeal = kit.creak(dur, child(r, 'squeal', i), 300.0 * jitter(r, 1, 0.1), 110.0, (1700.0, 3000.0, 4600.0), 13.0, 0.2)
        groan = kit.creak(dur * 1.1, child(r, 'groan', i), 90.0, 45.0, (240.0, 560.0, 1050.0), 6.0, 0.3)
        knock = kit.thunk(260.0 * jitter(r, 1, 0.1), child(r, 'knock', i), 0.07, 0.0006, 6, 0.1)
        parts += [(squeal, t, db - 9.0), (groan, t, db - 5.0), (knock, t + dur, db - 4.0)]
    return _outside(_soften(layers(*parts)), r, -24.0)


@cue('Loot.Coffin.LidOff', variations=3, att='Near', jitter=0.05, conc=2, level=-9.0)
def coffin_lid_off(v, r):
    # The loose lid shoved off: a dry scrape of board on board across the box, then its knock coming down on the
    # heap or the ground, a little bounce.
    dur = 0.32 + 0.06 * r.random()
    n = ns(dur)
    rub = G.grit(dur, child(r, 'rub'), 1800.0, 350.0, 2400.0, 2.0, 4) * E.ar(dur, dur * 0.3, dur * 0.4, 2.0)
    rub = normalize(rub, 0.0)
    drag = kit.creak(dur, child(r, 'drag'), 140.0, 80.0, (300.0, 700.0, 1400.0), 5.0, 0.35)
    t_land = dur + 0.03
    land = kit.thunk(190.0 * jitter(r, 1, 0.1), child(r, 'land'), 0.13, 0.0008, 7, 0.1)
    thud = kit.noise_thump(0.14, child(r, 'thud'), 650.0, 100.0, 0.07, 1.0, 0.05)
    bounce = kit.thunk(230.0, child(r, 'bounce'), 0.06, 0.0008, 6, 0.1)
    # The shove itself: the lid knocked off its seat as it starts to move.
    push = kit.thunk(320.0 * jitter(r, 1, 0.1), child(r, 'push'), 0.05, 0.0006, 6, 0.1)
    return _outside(_soften(layers((push, 0.0, -9.0), (rub, 0.0, -6.0), (drag, 0.0, -10.0), (land, t_land, 0.0),
                                   (thud, t_land, -4.0), (bounce, t_land + 0.07, -14.0))), r, -22.0)


@cue('Loot.Mailbox.Open', variations=3, att='Near', jitter=0.05, conc=2, level=-13.0)
def mailbox_open(v, r):
    # A tin mailbox's door dropped open: its catch letting go, a short rusty squeak, and the thin sheet clanking on
    # its stop with a little rattle.
    catch = kit.metal_click(child(r, 'catch'), 1500.0, 5200.0, 0.04, 0.0001, 5, -10.0)
    squeak = kit.creak(0.16, child(r, 'squeak'), 170.0, 70.0, (950.0, 2100.0, 3700.0), 12.0, 0.25)
    sheet = M.plate(640.0 * jitter(r, 1, 0.1), child(r, 'sheet'), 18, 0.22, 0.6, tilt=0.7)
    clank = normalize(M.strike(sheet, M.hammer(0.00015), 0.35))
    clank = F.filt(clank, F.peak(3200.0, -9.0, 0.6), F.highshelf(6000.0, -6.0), extend=False)
    rattle = kit.jingle(0.18, child(r, 'rattle'), 3, 1400.0, 4500.0, 0.04, 0.08, 0.0)
    t = 0.2 + 0.03 * r.random()
    return _outside(layers((catch, 0.0, -4.0), (squeak, 0.02, -8.0), (clank, t, 0.0), (rattle, t + 0.01, -14.0)), r, -24.0)


@cue('Loot.Footlocker.Open', variations=3, att='Near', jitter=0.03, conc=2, level=-9.0)
def footlocker_open(v, r):
    # The hasp flipped up, the trunk's hinges creaking as the lid swings, the lid knocking back on its strap, and a
    # soft gleam from what's inside.
    hasp = kit.clack(child(r, 'hasp'), 650.0, 1500.0, 5200.0, 0.04, 0.05, -3.0, -10.0)
    creak = kit.creak(0.5 + 0.08 * r.random(), child(r, 'creak'), 30.0 + 8.0 * r.random(), 80.0 + 20.0 * r.random(),
                      (330.0 * jitter(r, 1, 0.1), 820.0, 1600.0), 9.0, 0.3)
    t_lid = 0.62 + 0.06 * r.random()
    lid = kit.thunk(220.0 * jitter(r, 1, 0.08), child(r, 'lid'), 0.13, 0.0012, 7, 0.08)
    strap = kit.noise_thump(0.1, child(r, 'strap'), 800.0, 120.0, 0.05, 1.0, 0.04)
    gleam = np.zeros(1)
    for i, name in enumerate([['E6', 'B6', 'G#6'], ['D6', 'A6', 'F#6'], ['F#6', 'C#7', 'A6']][v]):
        gleam = layers(gleam, (kit.chime(kit.note(name), child(r, 'g', i), 1.2, 0.45, 0.0002), 0.08 * i, -3.0 * i))
    gleam = normalize(gleam) * E.swell(len(gleam) / SR, 0.1, 1.0)
    x = layers((hasp, 0.0, 0.0), (creak, 0.08, -9.0), (lid, t_lid, -1.0), (strap, t_lid, -8.0),
               (normalize(gleam), t_lid - 0.15, -17.0))
    return _outside(_soften(x), r, -22.0)
