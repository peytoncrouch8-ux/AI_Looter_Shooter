"""The Stone Sentinel (Docs/Work/2026-10-29-ElementsMatter/CONTRACT.md, lane E; AStoneSentinelCreature): a granite grave
statue that wakes. All synthesized, 3D at the statue, in the Gravemother's headstone family (gravemother_barrow.py's
stone: a hard crack, short dense rings, chips ticking away; bosses.py's weight and the valley's answer), but slower and
heavier: a statue of tons that grinds where a creature would breathe.

- Creature.Sentinel.Wake (~2.2 s): the statue coming to: a first crack through its crust, stone grinding against stone
  as the head lifts and the clasped hands part (two long grinds, the second lower), moss and grit trickling off, a dull
  groan of the whole block shifting, a heavy settle at 1.6 s.
- Creature.Sentinel.Step (5 takes, ~0.45 s): a stone foot coming down on earth: a deep thud of weight, the plinth-dense
  knock, gravel crunched under it, a short grind of the knee.
- Creature.Sentinel.WindUp (~1.0 s): the arms heaving up over the head for the slam: a rising grind, its creak getting
  faster and higher, grit sifting from the joints, a breath of air as they reach the top (the tell; swells on purpose).
- Creature.Sentinel.Slam (~1.6 s, 'Gun' attenuation: heard across the yard): both fists into the ground: the push of air,
  a huge thud and boom, the ground cracking open, stones and clods thrown and raining back, the valley rolling it back.
- Creature.Sentinel.Chip (6 takes, ~0.2 s): a bullet on its granite (every hit plays it, so short and never harsh): the
  headstone's StoneHit pitched deeper and duller, a bigger flake breaking off.
- Creature.Sentinel.Crumble (~2.6 s, 'Gun'): its death: cracks running all through it in quickening snaps, a deep
  split, the statue breaking apart and falling in pieces (two big thuds, the head's last), rubble clattering and rocking
  down, dust pouring.

Stone is dense: the cracks' rings sit above 5 kHz and short, the ear's sharpest band (2-5 kHz) dipped on every cue,
the weight under 150 Hz.
"""
import numpy as np

from lib.core import SR, ns, layers, normalize, child, jitter, fit
from lib import noise as N, env as E, osc as O, filters as F, modal as M
from recipes import cue
from recipes import kit
from recipes import creatures as C
from recipes.bosses import _drum, _pressure, _rumble, _master, _outside
from recipes.gravemother_barrow import _earth, _tonks, _lumps, _lv, _chip, _snap_run


# --- Pieces ---------------------------------------------------------------------------------------------------------

def _grind(dur, r, rate0, rate1, body=(120.0, 290.0, 700.0), attack=0.12, release=0.3):
    """Stone grinding on stone: slow stick-slip rung through the block's low body, over a gritty low rasp torn by a
    crumbling flutter."""
    creak = kit.creak(dur, child(r, 'creak'), rate0, rate1, body, 4.0, 0.35)
    rasp = _earth(dur, child(r, 'rasp'), 140.0, 1600.0, dur, attack, 34.0, 0.75)
    x = layers((creak, 0.0, 0.0), (rasp, 0.0, -6.0))
    return normalize(x * fit(E.end_fade(E.ar(x.size / SR, attack, release, 2.0), 0.02), x.size), 0.0)


def _crack(r, lo=700.0, hi=6500.0, ring=(1500.0, 6500.0)):
    """A crack through granite: a hard burst and a few short high rings."""
    burst = kit.burst(0.02, child(r, 'b'), 0.008, lo=lo, hi=hi, attack=0.0001)
    rings = normalize(M.strike(M.parts(ring[0], ring[1], child(r, 'p'), 6, 0.025, 0.01, 0.2), M.hammer(0.00005), 0.06), 0.0)
    return normalize(layers((burst, 0.0, 0.0), (rings, 0.0, -5.0)), 0.0)


def _trickle(dur, r, rate, lo=900.0, hi=4200.0):
    """Grit and flakes of moss sifting off: a thin falling crunch."""
    return kit.grit_burst(dur, child(r, 'trickle'), rate, lo, hi, dur * 0.7, dur * 0.25)


def _thud(r, dur, f0, f1, t60, drum_f=48.0, drum_db=-5.0):
    """Tons coming down: the air pushed out, a dark thud and a felt drum's body under it."""
    return normalize(layers(_pressure(child(r, 'push'), 0.016, min(dur, 0.2), 170.0),
                            (kit.noise_thump(dur, child(r, 'thud'), f0, f1, t60, 1.0, t60 * 0.5), 0.0, -1.0),
                            (_drum(drum_f * jitter(r, 1, 0.04), dur, child(r, 'drum'), t60 * 1.3, 0.12, 0.003), 0.0, drum_db)), 0.0)


def _stone(x, clip=3.0, push=2.0):
    """The stone cues' last step: the touchy band dipped, the rumble cut under 30 Hz, the big ones through the master."""
    x = F.filt(x, F.peak(3300.0, -5.0, 0.8), F.hp(30.0), extend=False)
    return _master(x, clip, push)


# --- Cues -----------------------------------------------------------------------------------------------------------

@cue('Creature.Sentinel.Wake', variations=2, att='Creature', jitter=0.03, conc=2, level=-6.0)
def wake(v, r):
    # A first crack through the crust, the head lifting (a long grind), the hands parting (a second, lower grind), grit
    # and moss trickling off, the whole block groaning as it shifts, then a heavy settle as it takes its weight.
    first = _crack(child(r, 'first'))
    run = _snap_run(0.25, child(r, 'run'), 80.0, 300.0, 700.0, 5000.0, 0.012)
    head = _grind(0.9, child(r, 'head'), 9.0, 22.0, (130.0, 320.0, 780.0))
    hands = _grind(0.75, child(r, 'hands'), 7.0, 16.0, (95.0, 240.0, 560.0))
    trickle = _trickle(1.8, child(r, 'trickle'), 2200.0)
    groan = F.filt(N.pink(ns(1.4), child(r, 'groan')), F.lpn(160.0, 4), F.hp(35.0, 0.7), extend=False)
    groan = normalize(groan * E.bp([(0.0, 0.0), (0.5, 1.0), (1.4, 0.0)], n=ns(1.4), curve='cos'), 0.0)
    settle = _thud(child(r, 'settle'), 0.6, 700.0, 50.0, 0.28)
    knock = kit.thunk(jitter(r, 300.0, 0.1), child(r, 'knock'), 0.07, 0.0004, 6, 0.1)
    flakes = _tonks(0.6, child(r, 'flakes'), 6, 0.02, 0.45, 800.0, 2400.0, 0.84, 0.02)
    ts = 1.55 + 0.1 * v
    x = layers((first, 0.0, -2.0), (normalize(run), 0.0, -6.0), (head, 0.12, -3.0), (hands, 0.7, -4.0),
               (trickle, 0.15, -16.0), (groan, 0.2, -8.0), (settle, ts, 0.0), (knock, ts, -6.0),
               (_lv(flakes, settle, -12.0), ts + 0.03, 0.0))
    return C._outside(_stone(x), r, -18.0, 0.07, 0.18, 0.8)


@cue('Creature.Sentinel.Step', variations=5, att='Creature', jitter=0.05, conc=4, level=-12.0)
def step(v, r):
    # A stone foot set down: a deep thud of weight, a dense knock, gravel crunched under it, a short grind at the knee.
    thud = _thud(child(r, 'thud'), 0.35, 600.0 * jitter(r, 1, 0.1), 55.0, 0.14, 52.0, -6.0)
    knock = kit.thunk(jitter(r, 260.0, 0.12), child(r, 'knock'), 0.05, 0.0005, 6, 0.1)
    crunch = kit.grit_burst(0.22, child(r, 'crunch'), 6000.0, 500.0, 3200.0, 0.1, 0.002)
    knee = _grind(0.22, child(r, 'knee'), 14.0, 20.0, (150.0, 360.0, 820.0), 0.03, 0.12)
    x = layers((thud, 0.0, 0.0), (knock, 0.0, -6.0), (crunch, 0.004, -11.0), (knee, 0.0, -14.0))
    return C._outside(_stone(x, 2.0, 1.0), r, -22.0, 0.06, 0.15, 0.4)


@cue('Creature.Sentinel.WindUp', variations=2, att='Creature', jitter=0.03, conc=2, level=-9.0, swell=True)
def wind_up(v, r):
    # The arms heaved up over the head: a grind rising in rate and pitch as they climb, grit sifting from the joints, a
    # short breath of air as they reach the top.
    dur = 1.0
    grind = _grind(dur, child(r, 'grind'), 8.0, 38.0, (140.0, 360.0, 900.0), 0.35, 0.12)
    n = ns(dur)
    lift = F.sweep(N.pink(n, child(r, 'lift')), 'lp', O.expsweep(90.0, 320.0, dur, 1.0, n=n), 0.9)
    lift = normalize(lift * E.swell(dur, 0.85 * dur, 0.15, 2.0), 0.0)
    trickle = _trickle(dur, child(r, 'trickle'), 1800.0)
    breath = kit.whoosh(0.35, child(r, 'air'), 300.0, 1400.0, 0.9, 0.6, -3.0, 1.0)
    x = layers((grind, 0.0, 0.0), (lift, 0.0, -6.0), (trickle, 0.1, -16.0), (breath, 0.72, -12.0))
    return C._outside(_stone(x, 2.0, 1.0), r, -20.0, 0.07, 0.18, 0.6)


@cue('Creature.Sentinel.Slam', variations=3, att='Gun', jitter=0.03, conc=2, level=-3.0)
def slam(v, r):
    # Both fists into the ground: the push of air, a huge thud with a boom under it, the ground cracking open, stones and
    # clods thrown up and raining back, a second settling knock, the valley answering.
    fall = _thud(child(r, 'fall'), 1.0, 1100.0, 40.0, 0.45, 44.0, -4.0)
    knock = kit.thunk(jitter(r, 380.0, 0.1), child(r, 'knock'), 0.1, 0.0003, 7, 0.1)
    split = layers(_crack(child(r, 'split'), 500.0, 6000.0, (1200.0, 5600.0)),
                   (_snap_run(0.35, child(r, 'run'), 400.0, 30.0, 500.0, 4200.0, 0.02), 0.0, -6.0))
    thrown = layers(_tonks(1.3, child(r, 'stones'), 12, 0.15, 1.0, 400.0, 2000.0, 0.86, 0.05),
                    (_lumps(1.2, child(r, 'clods'), 8, 0.12, 0.9, 110.0, 340.0, 0.84, 0.04), 0.0, -2.0))
    dust = _earth(1.4, child(r, 'dust'), 140.0, 1800.0, 0.7, 0.05, 36.0, 0.75)
    rumble = _rumble(1.6, child(r, 'rumble'), 220.0, 1.0, 0.1, 3.0)
    x = layers((fall, 0.0, 3.0), (knock, 0.0, -4.0), (_lv(split, fall, -6.0), 0.004, 0.0),
               (_lv(thrown, fall, -12.0), 0.0, 0.0), (_lv(dust, fall, -14.0), 0.02, 0.0), (rumble, 0.0, -12.0))
    return _outside(_stone(x, 4.0, 3.0), r, -14.0)


@cue('Creature.Sentinel.Chip', variations=6, att='Creature', jitter=0.06, conc=6, level=-12.0)
def chip(v, r):
    # A bullet on the statue's granite (played on every hit, so short and kept soft on the ear): a hard crack, the
    # block's dense dead knock (a whole statue: lower and deader than the headstone's), a flake breaking off and ticking
    # away, a puff of grit.
    f = (300.0, 340.0, 380.0, 420.0, 460.0, 360.0)[v] * jitter(r, 1.0, 0.05)
    crack = kit.burst(0.012, child(r, 'crack'), 0.005, lo=900.0, hi=6500.0, attack=0.0001)
    tock = normalize(M.strike(M.plate(f, child(r, 'block'), 14, 0.05, 0.7, 1.37, 0.03, 0.6), M.hammer(0.00008), 0.1), 0.0)
    weight = kit.noise_thump(0.09, child(r, 'weight'), 700.0, 100.0, 0.03, 1.0, 0.02)
    cr = child(r, 'flakes')
    flakes = np.zeros(1)
    t = 0.014
    for i in range(1 + int(cr.integers(0, 3))):
        flakes = layers(flakes, (_chip(child(cr, 'c', i), cr.uniform(1100.0, 1900.0), cr.uniform(3600.0, 6400.0)), t,
                                 -3.0 * i - cr.uniform(0.0, 3.0)))
        t += cr.uniform(0.015, 0.05)
    grit = kit.burst(0.07, child(r, 'grit'), 0.03, lo=400.0, hi=2500.0, color=-2.0)
    x = layers((crack, 0.0, -4.0), (tock, 0.0, 0.0), (weight, 0.0, -4.0), (flakes, 0.0, -10.0), (grit, 0.002, -17.0))
    x = F.filt(x, F.peak(3300.0, -5.0, 0.8), F.highshelf(8000.0, -3.0), F.hp(60.0), extend=False)
    return C._outside(x, r, -24.0, 0.05, 0.12, 0.25)


@cue('Creature.Sentinel.Crumble', variations=2, att='Gun', jitter=0.03, conc=2, level=-4.0)
def crumble(v, r):
    # Its death: cracks running all through it in quickening snaps, a deep split ringing through the block, the statue
    # breaking and falling in pieces (a big thud, then the head's at about 1.1 s), rubble clattering and rocking down,
    # dust pouring, the valley rolling it back.
    first = _crack(child(r, 'first'))
    run = _snap_run(0.6, child(r, 'run'), 30.0, 520.0, 600.0, 5200.0, 0.016)
    split = layers(_crack(child(r, 'split'), 400.0, 5500.0, (900.0, 4800.0)),
                   (normalize(M.strike(M.plate(240.0, child(r, 'gong'), 16, 0.4, 0.5, 1.37, 0.03, 0.5), M.hammer(0.0002), 0.5), 0.0), 0.0, -6.0))
    grind = _grind(0.45, child(r, 'grind'), 18.0, 34.0, (120.0, 300.0, 720.0), 0.05, 0.2)
    ta, tb = 0.75, 1.1 + 0.08 * v
    fall = _thud(child(r, 'fall'), 0.9, 1000.0, 40.0, 0.42, 46.0, -4.0)
    head = _thud(child(r, 'head'), 0.5, 900.0, 70.0, 0.2, 70.0, -8.0)
    rubble = layers(_tonks(1.7, child(r, 'rubble'), 16, 0.03, 1.4, 360.0, 2200.0, 0.88, 0.06),
                    (_lumps(1.4, child(r, 'blocks'), 7, 0.05, 1.0, 110.0, 360.0, 0.85, 0.05), 0.0, -2.0))
    dust = layers(_earth(1.9, child(r, 'dust'), 140.0, 1900.0, 0.8, 0.08, 36.0, 0.75),
                  (kit.burst(1.4, child(r, 'puff'), 0.6, lo=120.0, hi=1200.0, color=-3.0, attack=0.06), 0.0, -3.0))
    rumble = _rumble(2.0, child(r, 'rumble'), 220.0, 1.2, 0.12, 3.0)
    x = layers((first, 0.0, -2.0), (normalize(run), 0.02, -4.0), (split, 0.55, -1.0), (grind, 0.58, -6.0),
               (fall, ta, 3.0), (_lv(head, fall, -4.0), tb, 0.0), (_lv(rubble, fall, -9.0), ta + 0.04, 0.0),
               (_lv(dust, fall, -13.0), ta + 0.02, 0.0), (rumble, ta, -12.0))
    return _outside(_stone(x, 4.0, 3.0), r, -13.0)


# The new attacks' cues (the boulder, the shockwave, its open window, its pebbles) live in their own file, on the helpers
# above; imported here so they render with this group whether or not GROUPS lists 'sentinel_attacks' (imported last, as it
# imports this module's helpers).
from recipes import sentinel_attacks  # noqa: E402,F401
