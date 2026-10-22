"""The bounty board (Bounties/; Docs/Work/2026-10-22-Next/BOUNTY.md): the board's paper, a bounty taken and given up, its
mark's arrival and fall, the strongbox landing, the purse paid.

The board is paper on pine, so its cues speak in givers.py's paper (rubbed, creased, flapping) and tacks (a small steel
tick, a pine knock). The fight's two stings are the frontier's: a struck iron bar (a rail spike, an anvil) over a low
felt drum, a dry rattle rising like a snake's under the mark's arrival; an iron clang falling away and a shimmer of coin
as it goes down (the Rangers' purse). The strongbox is iron-bound oak into dirt. The purse is economy.py's coins and
counter, heavy, with givers.py's stamp: PAID.

- Bounty.Board.Open (3D, near): the board's sheets rustling as a hand goes over them, a tack worked loose.
- Bounty.Board.Posted (2D, quiet): a tack tapped home twice, a new sheet's flap against the boards.
- Bounty.Take (2D): a sheet torn off its tack, folded twice, into a pocket.
- Bounty.Abandon (2D): the sheet crumpled in a fist and tossed.
- Bounty.Mark.Arrive (2D sting, about 2 s): a low drum and a struck iron bar together, a rattle rising under them, a dark
  low string left humming.
- Bounty.Mark.Down (2D sting, about 1.8 s): an iron clang, a second a fourth lower, falling away; coins shimmering over it.
- Bounty.Chest.Land (3D): an iron-bound box into dirt: its weight, the oak's knock, the hasp and bands rattling, dirt
  pattering down round it.
- Bounty.Paid (2D): a heavy purse dropped on the counter (leather, the coins inside shifting), a stamp, a small till bell.
"""
import numpy as np

from lib.core import ns, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, modal as M
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes.ui import _ui
from recipes.givers import crackle, rub, flap, stamp, calm, _outside as _near
from recipes.economy import _coins, _counter, _store, _calm as _coin_calm
from recipes.bosses import _drum, _stage, _master, _debris, _pressure, _outside as _yard


# --- The board's paper and tacks -------------------------------------------------------------------------------------

def _tack(r, f0=2600.0, knock_f=320.0):
    """A tack in a pine board: the steel head's tiny tick, the board's knock under it."""
    tick = kit.metal_click(child(r, 'tick'), f0 * 0.8, f0 * 2.4, 0.02, 0.00005, 4, -12.0, 0.2)
    knock = kit.thunk(knock_f * jitter(r, 1, 0.06), child(r, 'knock'), 0.05, 0.0006, 5, 0.06)
    return normalize(layers((tick, 0.0, -2.0), (knock, 0.0, -6.0)), 0.0)


def _tear(r, dur=0.16):
    """Paper torn from a tack: a fast run of crackle tightening as the tear runs, over a ripping hiss rising in pitch."""
    n = ns(dur)
    bite = crackle(dur, child(r, 'bite'), 2600.0, 900.0, 6000.0, 30.0)
    hiss = F.sweep(N.pink(n, child(r, 'rip')), 'bp', np.geomspace(900.0, 2600.0, n), 1.2)
    env = E.ar(dur, dur * 0.15, dur * 0.5, 1.6)
    return normalize(layers((normalize(bite * env), 0.0, 0.0), (normalize(hiss * env), 0.0, -5.0)), 0.0)


def _fold(r, gap=0.12):
    """A sheet folded twice: each fold a burst of crackle and a soft pat as it's pressed flat; returns it and its length."""
    out = np.zeros(1)
    t = 0.0
    for i in range(2):
        d = 0.05 + 0.03 * r.random()
        c = crackle(d, child(r, 'fold', i), 1100.0, 900.0, 6000.0, 18.0) * E.ar(d, 0.003, d * 0.6, 1.5)
        pat = flap(child(r, 'pat', i), 0.06, 800.0, 220.0, -10.0)
        out = layers(out, (normalize(c), t, -1.0 * i), (pat, t + d * 0.7, -8.0))
        t += gap * jitter(r, 1, 0.15)
    return out, t


# --- The board -------------------------------------------------------------------------------------------------------

@cue('Bounty.Board.Open', variations=3, att='Near', cls='Effects', jitter=0.03, conc=2, level=-14.0)
def board_open(v, r):
    # A hand going over the board's sheets: paper rubbing on paper and on the boards, a crease ticking, one sheet lifted
    # at its corner and let fall; a tack worked loose with a small steel tick.
    d = 0.3 + 0.05 * r.random()
    touch = kit.burst(0.01, child(r, 'touch'), 0.004, lo=500.0, hi=3200.0, attack=0.0008)
    over = rub(d, child(r, 'over'), 300.0, 3600.0, 800.0, 0.2, 0.5)
    tick = crackle(0.06, child(r, 'crease'), 600.0, 1000.0, 6000.0, 20.0)
    fall = flap(child(r, 'fall'), 0.08, 700.0, 180.0, -8.0)
    tack = _tack(child(r, 'tack'), 2400.0 * jitter(r, 1, 0.05))
    x = layers((touch, 0.0, -10.0), (over, 0.0, 0.0), (tick, d * 0.35, -14.0), (fall, d * 0.8, -4.0), (tack, d * 0.55, -10.0))
    return _near(calm(x), r, -22.0)


@cue('Bounty.Board.Posted', variations=2, space='2D', cls='Interface', jitter=0.02, conc=1, level=-18.0)
def board_posted(v, r):
    # Someone has put up a new bounty: a tack tapped home twice with a hammer's cheek (a steel tick, the pine's knock), the
    # sheet's flap against the boards as it's smoothed.
    first = _tack(child(r, 'one'), 2500.0, 300.0)
    second = _tack(child(r, 'two'), 2700.0, 310.0)
    gap = 0.14 + 0.03 * r.random()
    smooth = rub(0.14, child(r, 'smooth'), 320.0, 3200.0, 600.0, 0.25, 0.5)
    x = layers((first, 0.0, -1.0), (second, gap, 0.0), (smooth, gap + 0.08, -10.0),
               (flap(child(r, 'flap'), 0.07, 650.0, 160.0, -10.0), gap + 0.2, -6.0))
    return _ui(calm(x), r, 0.25, -22.0, 0.22)


# --- Taken, given up -------------------------------------------------------------------------------------------------

@cue('Bounty.Take', variations=3, space='2D', cls='Interface', jitter=0.02, conc=1, level=-11.0)
def bounty_take(v, r):
    # Taken: the sheet torn off its tack (the tack pinging free), folded twice and pushed into a coat pocket.
    tear = _tear(child(r, 'tear'))
    ping = kit.metal_click(child(r, 'ping'), 2800.0, 6500.0, 0.03, 0.00005, 3, -14.0, 0.2)
    folded, t = _fold(child(r, 'fold'))
    pocket = kit.cloth(0.2, child(r, 'pocket'), 280.0, 2200.0, 120.0, 0.25, 0.6)
    x = layers((tear, 0.0, 0.0), (ping, 0.02, -16.0), (folded, 0.2, -3.0), (pocket, 0.2 + t, -8.0))
    return _ui(calm(x), r, 0.25, -20.0, 0.22)


@cue('Bounty.Abandon', variations=2, space='2D', cls='Interface', jitter=0.03, conc=1, level=-16.0)
def bounty_abandon(v, r):
    # Given up: the sheet crumpled in a fist (dense crackle in tightening clusters) and tossed, a soft pat where it lands.
    d = 0.32 + 0.05 * r.random()
    # The fingers closing on it speak first (a soft touch of paper), so the cue starts at once.
    touch = kit.burst(0.012, child(r, 'touch'), 0.005, lo=500.0, hi=3200.0, attack=0.0006)
    crumple = crackle(d, child(r, 'crumple'), 2200.0, 700.0, 5500.0, 9.0) * E.ar(d, d * 0.04, d * 0.5, 1.4)
    grip = kit.cloth(d, child(r, 'grip'), 250.0, 2000.0, 90.0, 0.3, 0.5)
    toss = kit.whoosh(0.14, child(r, 'toss'), 300.0, 900.0, 0.8, 0.4, -3.0)
    land = flap(child(r, 'land'), 0.07, 600.0, 160.0, -8.0)
    x = layers((touch, 0.0, -6.0), (normalize(crumple), 0.0, 0.0), (grip, 0.0, -10.0), (toss, d, -14.0), (land, d + 0.18, -8.0))
    return _ui(calm(x), r, 0.25, -24.0, 0.2)


# --- The fight's stings ----------------------------------------------------------------------------------------------

def _iron_bar(f0, r, t60=1.4, dur=1.8):
    """A struck iron bar (a rail spike, an anvil's horn): a few bright inharmonic modes, the hammer's hard contact."""
    return normalize(M.strike(M.bar(f0, child(r, 'bar'), t60=t60, count=6), M.hammer(0.00012), dur), 0.0)


def _rattle_rising(dur, r, rate0=9.0, rate1=34.0):
    """A dry rattle rising (a snake's, felt more than heard): short keratin clicks quickening from rate0 to rate1 a second,
    swelling as they come."""
    out = np.zeros(ns(dur))
    t = 0.0
    i = 0
    while t < dur:
        u = t / dur
        click = kit.burst(0.008, child(r, 'click', i), 0.003, lo=1800.0, hi=6500.0, attack=0.0003)
        out = layers(out, (click, t, -18.0 + 16.0 * u))
        t += 1.0 / (rate0 + (rate1 - rate0) * u * u) * r.uniform(0.85, 1.15)
        i += 1
    return normalize(F.filt(out, F.peak(3300.0, -5.0, 0.8), extend=False), 0.0)


@cue('Bounty.Mark.Arrive', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-6.0)
def mark_arrive(v, r):
    # The mark comes out: a low felt drum and an iron bar struck together (the hit), a dry rattle rising under them, and a
    # dark low string left humming a minor second off its root: trouble with a name.
    f_bar = [410.0, 380.0][v]
    hit = layers(_drum(52.0, 1.0, child(r, 'drum'), 0.6, 0.15, 0.002),
                 (_pressure(child(r, 'push'), 0.01, 0.1, 200.0), 0.0, -4.0),
                 (_iron_bar(f_bar, child(r, 'bar')), 0.0, -5.0))
    root = kit.note(['D2', 'C2'][v])
    hum = layers(kit.twang(root, 1.8, child(r, 'h1'), 0.4, 1.6), (kit.twang(root * 2 ** (1 / 12) * 2, 1.8, child(r, 'h2'), 0.4, 1.4), 0.0, -8.0))
    hum = F.filt(normalize(hum), F.lp(1400.0, 0.7), extend=False)
    rattle = _rattle_rising(0.9, child(r, 'rattle'))
    x = layers((rattle, 0.0, -9.0), (hit, 0.75, 0.0), (hum, 0.78, -9.0))
    x = F.filt(x, F.lowshelf(70.0, -3.0), F.hp(32.0), extend=False)
    x = _stage(x, r, 'open', 1.4, -15.0, 0.3)
    return _master(x, 2.0, 0.5)


@cue('Bounty.Mark.Down', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-7.0)
def mark_down(v, r):
    # The mark falls, the bounty won: an iron clang, a second a fourth lower answering it and falling away, a soft drum
    # under the first, and a shimmer of coin rising out of them (the Rangers' purse).
    f0 = [520.0, 480.0][v]
    first = _iron_bar(f0, child(r, 'first'), 1.2, 1.6)
    second = _iron_bar(f0 * 0.75, child(r, 'second'), 1.6, 1.8)
    drum = _drum(60.0, 0.7, child(r, 'drum'), 0.45, 0.14, 0.003)
    coins = kit.jingle(0.7, child(r, 'coins'), 14, 1900.0, 6000.0, 0.06, 0.45, 0.0)
    x = layers((first, 0.0, 0.0), (drum, 0.0, -6.0), (second, 0.26, -2.0), (coins, 0.3, -13.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.hp(40.0), extend=False)
    x = _stage(x, r, 'open', 1.5, -14.0, 0.35)
    return _master(x, 1.5, 0.5)


# --- The strongbox ---------------------------------------------------------------------------------------------------

@cue('Bounty.Chest.Land', variations=3, att='Creature', cls='Effects', jitter=0.04, conc=1, level=-9.0)
def chest_land(v, r):
    # An iron-bound oak strongbox dropped into dirt: its weight (a dark thump, the ground's give), the oak's knock, the hasp
    # and the iron bands rattling as it settles, dirt pattering down round it.
    weight = layers(kit.noise_thump(0.35, child(r, 'weight'), 700.0, 55.0, 0.18, 1.0, 0.05),
                    (_pressure(child(r, 'push'), 0.008, 0.08, 180.0), 0.0, -6.0))
    oak = kit.thunk(150.0 * jitter(r, 1, 0.06), child(r, 'oak'), 0.12, 0.0015, 7, 0.08)

    def iron(rr, i):
        return normalize(M.strike(M.parts(600.0, 2400.0, child(rr, 'band', i), 6, 0.05, 0.03, 0.4), M.hammer(0.0001), 0.12), 0.0)
    hasp = normalize(G.rattle(0.25, child(r, 'hasp'), 4, 0.18, iron, 0.01, 0.6), 0.0)
    dirt = _debris(0.6, child(r, 'dirt'), 10, 0.03, 0.4, 900.0, 4000.0)
    x = layers((weight, 0.0, 0.0), (oak, 0.0, -4.0), (hasp, 0.0, -10.0), (dirt, 0.02, -14.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.9), F.hp(35.0), extend=False)
    return _yard(x, r, -19.0)


# --- Paid ------------------------------------------------------------------------------------------------------------

@cue('Bounty.Paid', variations=3, space='2D', cls='Interface', jitter=0.02, conc=1, level=-9.0)
def bounty_paid(v, r):
    # Paid at the board: a heavy purse dropped on the counter (the boards' knock, the leather's slump, the coins inside
    # shifting, muffled), the clerk's stamp, PAID, and the till's small brass bell.
    knock = _counter(child(r, 'knock'), 190.0, 0.1)
    slump = kit.cloth(0.16, child(r, 'leather'), 250.0, 2000.0, 110.0, 0.1, 0.6)
    inside = F.filt(_coins(child(r, 'inside'), 6, 0.14, 1400.0, 2100.0, 0.0, 0.75), F.lp(2400.0, 0.7), extend=False)
    pressed = stamp(child(r, 'stamp'), 270.0 * jitter(r, 1, 0.04))
    bell = kit.chime(kit.note(['A5', 'G5', 'B5'][v]), child(r, 'bell'), 0.8, 0.7, 0.00015, 0.8, -30.0)
    x = layers((knock, 0.0, 0.0), (slump, 0.0, -6.0), (inside, 0.005, -8.0), (pressed, 0.34, -2.0), (bell, 0.52, -7.0))
    return _store(_coin_calm(x, 70.0), r, -20.0, 0.3)
