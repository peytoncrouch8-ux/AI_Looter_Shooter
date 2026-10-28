"""The gun card's sounds (2026-10-28, UI character round 1; Docs/Work/2026-10-28-UICharacter/CONTRACT.md, "Motion"; names
in LooterSoundCue::Card, Audio/LooterSoundCues.h). The card unfolds over a gun on the ground and beside the inventory's
cursor, so it plays often: every cue here is short (0.15-0.4 s), quiet, soft in the ear's 2-5 kHz band and 2D.

    UI.Card.Open                 every card: a light mechanical tick with a tiny airy swish (a leaf of brass unfolding)
    UI.Card.Maker.Roundhouse     the railroad maker: a short heavy iron-on-iron clank, low
    UI.Card.Maker.SaintMaud      the blessed foundry: a small bright chapel-bell ting, short decay
    UI.Card.Maker.Windfall       the salvage maker: bolted scrap knocking loose, two or three tinny knocks
    UI.Card.Maker.Meridian       the precision maker: a surveyor's transit detent, one crisp small click

A maker's cue plays with Open, so each sits a touch under it (level) and in its own register: Roundhouse below 700 Hz,
SaintMaud a ringing pitch, Windfall tinny and uneven in time, Meridian one dry high click with no tail.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, modal as M, stereo as S
from recipes import cue
from recipes import kit


def _card(x, r, width=0.12):
    """Centered with a hint of width and a very short end fade; no room tail, so repeats stay clean."""
    x = S.widen(x, child(r, 'wide'), width)
    n = x.shape[-1]
    fade = E.end_fade(np.ones(n), 0.02)
    return x * fade


def _ease(x, cut_db=-5.0):
    return F.filt(x, F.peak(3300.0, cut_db, 1.0), extend=False)


@cue('UI.Card.Open', variations=3, space='2D', cls='Interface', jitter=0.03, conc=2, level=-20.0)
def card_open(v, r):
    # A thin brass leaf swinging open: a light tick as it catches (a small brass tick and a soft wood knock under it)
    # and a tiny breath of air as it moves, rising a little. Kept soft and short, since the inventory plays it on
    # every cursor move.
    tick = kit.brass_tick(child(r, 'tick'), 1900.0 * jitter(r, 1, 0.05), 0.016, 3, 0.0001, -12.0, -12.0, 0.1)
    knock = kit.thunk(760.0 * jitter(r, 1, 0.06), child(r, 'knock'), 0.02, 0.0004, 4, 0.05)
    swish = kit.whoosh(0.15, child(r, 'swish'), 1900.0, 3800.0 + 300.0 * v, 0.9, 0.3, -2.0, 1.0)
    # The tick leads (so the card feels instant) and the air follows as the leaf swings out.
    x = layers((tick, 0.0, 0.0), (knock, 0.0, -9.0), (swish, 0.018, -12.0))
    x = F.filt(x, F.hp(300.0), extend=False)
    x = x * E.end_fade(np.ones(x.size), 0.05)
    return _card(_ease(normalize(x), -5.0), r)


@cue('UI.Card.Maker.Roundhouse', variations=3, space='2D', cls='Interface', jitter=0.03, conc=2, level=-23.0)
def maker_roundhouse(v, r):
    # Iron on iron in a rail yard: a thick plate struck (low, dense modes that ring about 0.15 s), a heavy knock
    # of the frame and a deep thump under it; dark on top, nothing above a few kHz.
    f0 = [210.0, 190.0, 232.0][v] * jitter(r, 1.0, 0.03)
    plate = M.strike(M.plate(f0, child(r, 'plate'), count=12, t60=0.3, spread=0.03), M.hammer(0.0005), 0.4)
    body = kit.clack(child(r, 'body'), 330.0 * jitter(r, 1, 0.05), 900.0, 3200.0, 0.05, 0.07, 0.0, -2.0, 0.0003)
    low = kit.thump(75.0, 45.0, 0.14, 0.07)
    x = layers((normalize(plate), 0.0, 0.0), (body, 0.0, -4.0), (low, 0.0, -6.0))
    x = F.filt(x, F.hp(70.0), F.lpn(4200.0, 2), extend=False)
    x = x * E.end_fade(np.ones(x.size), 0.05)
    return _card(_ease(normalize(x), -4.0), r, 0.08)


@cue('UI.Card.Maker.SaintMaud', variations=3, space='2D', cls='Interface', jitter=0.02, conc=2, level=-24.0)
def maker_saint_maud(v, r):
    # A small chapel bell touched once with a brass tongue: the bell's own partials (hum, strike tone, tierce, quint)
    # in a short decay, a soft tick of the clapper. Pitched high enough to sing out over Open without a harsh edge.
    prime = [1175.0, 1245.0, 1108.0][v] * jitter(r, 1.0, 0.004)
    bell = M.strike(M.bell(prime, child(r, 'bell'), t60=0.5, beat=2.0, bright=0.7, count=8), M.hammer(0.0003), 0.45)
    clap = kit.brass_tick(child(r, 'clap'), 2300.0, 0.012, 3, 0.00008, -16.0, -10.0, 0.1)
    x = layers((normalize(bell), 0.0, 0.0), (clap, 0.0, -14.0))
    x = F.filt(x, F.hp(380.0), F.lpn(7500.0, 2), extend=False)
    x = x * E.end_fade(np.ones(x.size), 0.08)
    return _card(_ease(normalize(x), -6.0), r, 0.2)


@cue('UI.Card.Maker.Windfall', variations=3, space='2D', cls='Interface', jitter=0.05, conc=2, level=-23.0)
def maker_windfall(v, r):
    # Scrap bolted on loose, shaken: three tinny knocks of different pieces (a panel, a tag, a bracket), uneven in time
    # and size, with loose bits jingling between them.
    gaps = [(0.0, 0.055, 0.125), (0.0, 0.07, 0.115), (0.0, 0.05, 0.14)][v]
    fs = [(330.0, 520.0, 410.0), (380.0, 450.0, 590.0), (300.0, 560.0, 440.0)][v]
    amps = [(0.0, -4.0, -8.0), (0.0, -6.0, -3.0), (-2.0, 0.0, -7.0)][v]
    parts = []
    for i in range(3):
        panel = M.strike(M.plate(fs[i] * jitter(r, 1.0, 0.04), child(r, f'knock{i}'), count=14, t60=0.05, spread=0.05),
                         M.hammer(0.0003), 0.1)
        parts.append((normalize(panel), gaps[i], amps[i]))
    bits = kit.jingle(0.22, child(r, 'bits'), 7, lo=1700.0, hi=5200.0, t60=0.025, spread=0.1, start=0.0)
    parts.append((bits, 0.0, -11.0))
    x = layers(*parts)
    x = F.filt(x, F.hp(220.0), F.lpn(6200.0, 2), extend=False)
    x = x * E.end_fade(np.ones(x.size), 0.03)
    return _card(_ease(normalize(x), -5.0), r, 0.16)


@cue('UI.Card.Maker.Meridian', variations=3, space='2D', cls='Interface', jitter=0.02, conc=2, level=-25.5)
def maker_meridian(v, r):
    # A surveyor's transit locking into its detent: one dry, exact click (a hard steel tick over a tiny stiff ring) and
    # a fainter one a few ms later as the vernier seats. No tail: precision is quiet.
    f0 = [2700.0, 2500.0, 2900.0][v] * jitter(r, 1.0, 0.01)
    seat = kit.metal_click(child(r, 'seat'), 3000.0, 6500.0, 0.012, 0.00006, 4, -8.0)
    main = kit.brass_tick(child(r, 'main'), f0, 0.008, 3, 0.00006, -14.0, -6.0, 0.2)
    base = kit.thunk(1250.0, child(r, 'base'), 0.012, 0.0003, 3, 0.05)
    x = layers((main, 0.0, 0.0), (base, 0.0, -10.0), (seat, 0.045, -9.0))
    # The housing's faint stiff ring after the seat, so the click ends in a breath, not a cut.
    ring = M.strike(M.parts(1500.0, 3400.0, child(r, 'ring'), count=5, t60=0.14), M.hammer(0.0002), 0.25)
    x = layers((x, 0.0, 0.0), (normalize(ring), 0.045, -15.0))
    x = F.filt(x, F.hp(500.0), extend=False)
    x = x * E.end_fade(np.ones(x.size), 0.05)
    return _card(_ease(normalize(x), -6.0), r, 0.06)
