"""The economy (Docs/Progression.md, "Economy"): silver found, and the vendor's counter (a purchase, a sale, a refusal).

Silver is a thin coin, struck: a small plate's few inharmonic modes, short and bright, with a hard tick of metal on metal.
Coins are pitched low for coins and the ear's touchiest band (2-5 kHz) is cut, since a fight's worth of pickups chimes
over and over. The counter is the frontier store's: pine boards, a small brass till bell, leather and a purse, in the
interface's brass-and-wood voice (ui.py), with a little of the store's room round them.

- Loot.Silver.Pickup (2D, the player's own): a few coins dropping into a palm one after another and tinkling together, a
  small bright two-note chime over them (a major third) for the find, the leather of the hand closing on them.
- Vendor.Buy (2D): coins counted out onto the counter (each a clink and the boards' knock), the till's bell, the goods
  slid across.
- Vendor.Sell (2D): the gun set down on the counter with a solid knock and a rattle of its steel, then coins counted into
  the hand, and a rising pair of chime notes.
- Vendor.NotEnough (2D): a light purse shaken (two lonely coins knocking), one coin pushed back across the boards, a low
  sour note under it.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S
from lib import granular as G
from recipes import cue
from recipes import kit


# --- What coins, the counter and the hand are -------------------------------------------------------------------------

def _coin(r, f0, t60=0.09):
    """One thin silver coin struck: a small disc's handful of inharmonic modes from f0 up, the hard tick of the contact."""
    modes = M.plate(f0, child(r, 'plate'), 8, t60, 0.5, 1.0, 0.03, 0.6)
    return normalize(M.strike(modes, M.hammer(0.00004), t60 * 1.6 + 0.01), 0.0)


def _coins(r, count, spread, f_lo=1650.0, f_hi=2350.0, start=0.0, decay=0.8):
    """count coins landing one after another and tinkling against each other as they settle."""
    def grain(rr, i):
        return _coin(child(rr, 'c', i), rr.uniform(f_lo, f_hi), rr.uniform(0.06, 0.12)) * rr.uniform(0.6, 1.0)
    return normalize(G.rattle(0.12, child(r, 'coins'), count, spread, grain, start, decay), 0.0)


def _counter(r, f0=230.0, t60=0.09):
    """The pine counter knocked: a dull wooden thunk with a little body."""
    return kit.thunk(f0 * jitter(r, 1, 0.08), child(r, 'board'), t60, 0.0014, 6, 0.08)


def _palm(r):
    """A gloved hand closing on the coins: leather, a soft thump."""
    thump = kit.noise_thump(0.07, child(r, 'thump'), 600.0, 130.0, 0.03, 1.0, 0.02)
    leather = kit.cloth(0.08, child(r, 'leather'), 300.0, 2200.0, 180.0, 0.15, 0.6)
    return layers((thump, 0.0, 0.0), (leather, 0.0, -8.0))


def _counted(r, count, gap, f_lo=1600.0, f_hi=2300.0, board_db=-12.0):
    """Coins counted out onto the counter one at a time: each a clink, the boards' knock under it; returns it and its length."""
    out = np.zeros(1)
    t = 0.0
    for i in range(count):
        clink = _coin(child(r, 'coin', i), r.uniform(f_lo, f_hi), r.uniform(0.05, 0.09))
        knock = _counter(child(r, 'knock', i), 280.0, 0.05)
        out = layers(out, (clink, t, -0.6 * i), (knock, t, board_db))
        t += gap * (0.85 + 0.3 * r.random())
    return out, t


def _store(x, r, wet_db=-22.0, t60=0.25, width=0.25):
    """Centred with a little width and the store's small timber room round it (the interface's way, ui._ui)."""
    dry = S.widen(x, child(r, 'wide'), width)
    ir = R.stereo_ir(R.room, child(r, 'ir1'), child(r, 'ir2'), t60=t60, size=0.6)
    wet = F.convolve(F.filt(x, F.hp(300.0), extend=False), ir)
    return layers(dry, kit.set_level(wet, dry, wet_db, 0.01))


def _calm(x, hp=90.0):
    """Heard over and over: the 2-5 kHz band and the very top kept soft."""
    return F.filt(x, F.peak(3300.0, -6.0, 0.8), F.highshelf(8000.0, -4.0), F.hp(hp), extend=False)


# --- Silver found --------------------------------------------------------------------------------------------------------

@cue('Loot.Silver.Pickup', variations=4, space='2D', cls='Effects', jitter=0.04, conc=3, level=-13.0)
def silver_pickup(v, r):
    # A few silver coins dropping into a palm one after another, tinkling together as they settle; a small bright chime
    # over them for the find (a major third, a different pair each take); the leather of the hand closing on them.
    coins = _coins(child(r, 'drop'), 3 + int(r.integers(0, 3)), 0.09)
    a, b = [('E6', 'G#6'), ('D6', 'F#6'), ('F6', 'A6'), ('C6', 'E6')][v]
    c1 = kit.chime(kit.note(a), child(r, 'c1'), 0.35, 0.55, 0.0002, 0.5, -34.0)
    c2 = kit.chime(kit.note(b), child(r, 'c2'), 0.42, 0.55, 0.0002, 0.5, -34.0)
    palm = _palm(child(r, 'palm'))
    x = layers((coins, 0.0, 0.0), (c1, 0.012, -9.0), (c2, 0.07, -11.0), (palm, 0.1 + 0.02 * r.random(), -10.0))
    return _store(_calm(x, 120.0), r, -28.0, 0.15, 0.2)


# --- At the counter ------------------------------------------------------------------------------------------------------

@cue('Vendor.Buy', variations=3, space='2D', cls='Interface', jitter=0.02, conc=2, level=-11.0)
def vendor_buy(v, r):
    # Paying: three or four coins counted out onto the pine boards, the till's little brass bell, and the goods slid across
    # the counter to you.
    counted, t = _counted(child(r, 'pay'), 3 + int(r.integers(0, 2)), 0.09)
    bell = kit.chime(kit.note(['A5', 'G5', 'B5'][v]), child(r, 'bell'), 0.7, 0.7, 0.00015, 0.8, -30.0)
    slide = kit.scrape(0.2, child(r, 'slide'), 520.0, 400.0, 4.0, 0.8, (1.0, 1.7), 0.15, 0.5, 1200.0, -30.0)
    slide = F.filt(slide, F.lp(2000.0, 0.7), extend=False)
    stop = _counter(child(r, 'stop'), 200.0, 0.08)
    x = layers((counted, 0.0, 0.0), (bell, t + 0.02, -6.0), (slide, t + 0.12, -14.0), (stop, t + 0.32, -12.0))
    return _store(_calm(x), r, -20.0, 0.3)


@cue('Vendor.Sell', variations=3, space='2D', cls='Interface', jitter=0.02, conc=2, level=-11.0)
def vendor_sell(v, r):
    # Selling a gun: it's set down on the counter (a solid knock of the boards, its steel rattling once), a beat while
    # it's looked over, then coins counted into your hand and a rising pair of notes: silver coming in.
    boards = M.strike(M.wood(150.0 * jitter(r, 1, 0.08), child(r, 'boards'), t60=0.16, count=7, spread=0.08), M.hammer(0.0012), 0.3)
    thud = kit.noise_thump(0.16, child(r, 'thud'), 800.0, 90.0, 0.08, 1.0, 0.04)
    steel = M.strike(M.parts(700.0, 2800.0, child(r, 'steel'), 8, 0.06, 0.03, 0.45), M.hammer(0.0001), 0.14)
    set_down = layers((normalize(boards), 0.0, 0.0), (normalize(thud), 0.0, -2.0), (normalize(steel), 0.003, -9.0))
    counted, t = _counted(child(r, 'paid'), 3 + int(r.integers(0, 2)), 0.08, 1750.0, 2450.0, -16.0)
    palm = _palm(child(r, 'palm'))
    a, b = [('D5', 'A5'), ('E5', 'B5'), ('C5', 'G5')][v]
    n1 = kit.chime(kit.note(a), child(r, 'n1'), 0.45, 0.5, 0.0003, 0.5, -36.0)
    n2 = kit.chime(kit.note(b), child(r, 'n2'), 0.6, 0.5, 0.0003, 0.5, -36.0)
    at = 0.34 + 0.04 * r.random()
    x = layers((set_down, 0.0, 0.0), (counted, at, -3.0), (palm, at + t + 0.02, -10.0), (n1, at + t + 0.03, -14.0),
               (n2, at + t + 0.13, -15.0))
    return _store(_calm(x, 70.0), r, -20.0, 0.3)


@cue('Vendor.NotEnough', variations=2, space='2D', cls='Interface', jitter=0.0, conc=2, level=-13.0)
def vendor_not_enough(v, r):
    # Not enough silver: a light purse shaken (leather and two lonely coins knocking inside), then one coin pushed back
    # across the boards to you, with a low, sour pair of notes under it (the interface's refusal, ui.denied, kin).
    shake = kit.cloth(0.16, child(r, 'purse'), 250.0, 1800.0, 90.0, 0.2, 0.5)
    lonely = _coins(child(r, 'lonely'), 2, 0.12, 1500.0, 1900.0, 0.02, 0.9)
    push = kit.scrape(0.16, child(r, 'push'), 900.0, 760.0, 6.0, 0.7, (1.0, 1.5), 0.1, 0.6, 1500.0, -30.0)
    push = F.filt(push, F.lp(2600.0, 0.7), extend=False)
    stop = _coin(child(r, 'stop'), 1500.0, 0.05)
    sour = layers(kit.chime(kit.note('Bb3'), child(r, 's1'), 0.3, 0.4, 0.0005, 0.3, -40.0),
                  (kit.chime(kit.note('B3'), child(r, 's2'), 0.3, 0.4, 0.0005, 0.3, -40.0), 0.0, -2.0))
    x = layers((shake, 0.0, -6.0), (lonely, 0.02, -4.0), (push, 0.24, -10.0), (stop, 0.4, -8.0), (normalize(sour), 0.26, -12.0))
    return _store(_calm(x), r, -22.0, 0.22)
