"""Menus and screens (2D): clicks, hovers, back, a page opening and closing, tabs, and a refusal. Clean, short and
mechanical, in brass and wood like the gunsmith's tools and a ledger's clasp; quiet in the mix and soft on the ear,
since they repeat constantly.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, stereo as S
from recipes import cue
from recipes import kit


def _ui(x, r, width=0.2, wet_db=-24.0, t60=0.18):
    """Centered with a little width and a hint of a small room, so it doesn't sound pasted on."""
    dry = S.widen(x, child(r, 'wide'), width)
    ir = R.stereo_ir(R.room, child(r, 'ir1'), child(r, 'ir2'), t60=t60, size=0.5)
    wet = F.convolve(F.filt(x, F.hp(400.0), extend=False), ir)
    return layers(dry, kit.set_level(wet, dry, wet_db, 0.01))


@cue('UI.Click', variations=3, space='2D', cls='Interface', jitter=0.02, conc=4, level=-17.0)
def click(v, r):
    # A small brass switch: pressed (tk), released (tik).
    press = kit.brass_tick(child(r, 'press'), 1350.0 * jitter(r, 1, 0.03), 0.02, 4, 0.0001, -9.0, -8.0, 0.1)
    release = kit.brass_tick(child(r, 'release'), 2100.0 * jitter(r, 1, 0.03), 0.012, 3, 0.00008, -18.0, -10.0, 0.0)
    x = layers((press, 0.0, 0.0), (release, 0.022 + 0.004 * r.random(), -9.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 1.0), extend=False)
    return _ui(x, r)


@cue('UI.Hover', variations=3, space='2D', cls='Interface', jitter=0.03, conc=3, level=-25.0)
def hover(v, r):
    # Barely there: a soft wooden tick, its top rounded off. It's also the settings sliders' tick, so it stays short
    # enough to run in quick strings.
    tick = kit.thunk(900.0 * jitter(r, 1, 0.08), child(r, 'tick'), 0.014, 0.0003, 4, 0.05)
    air = kit.burst(0.004, child(r, 'air'), 0.0015, lo=4000.0, hi=9000.0)
    x = layers((tick, 0.0, 0.0), (air, 0.0, -18.0))
    x = F.filt(x, F.lp(6000.0, 0.7), extend=False)
    return _ui(x, r, 0.15, -32.0, 0.08)


@cue('UI.Back', variations=2, space='2D', cls='Interface', jitter=0.02, conc=3, level=-17.0)
def back(v, r):
    # A click, then a softer, lower knock that falls away: stepping back out.
    press = kit.brass_tick(child(r, 'press'), 1250.0, 0.018, 4, 0.0001, -10.0, -9.0, 0.1)
    knock = kit.thunk(330.0 * jitter(r, 1, 0.05), child(r, 'knock'), 0.05, 0.001, 5, 0.04)
    tone = kit.chime(kit.note(['A4', 'G4'][v]), child(r, 'tone'), 0.25, 0.4, 0.0004, 0.3, -40.0)
    x = layers((press, 0.0, -2.0), (knock, 0.035, -3.0), (tone, 0.035, -12.0))
    return _ui(x, r)


@cue('UI.Open', variations=2, space='2D', cls='Interface', jitter=0.02, conc=2, level=-14.0)
def open_page(v, r):
    # A clasp snaps open and the page sweeps up, with a faint brass shimmer as it settles.
    clasp = kit.clack(child(r, 'clasp'), 900.0, 1500.0, 5000.0, 0.03, 0.03, -4.0, -14.0, 0.00012)
    clasp = F.filt(clasp, F.peak(3200.0, -4.0, 1.0), extend=False)
    sweep = kit.whoosh(0.2, child(r, 'sweep'), 350.0, 2600.0, 1.1, 0.6, -2.0, 1.2)
    a, b = [('E5', 'B5'), ('D5', 'A5')][v]
    c1 = kit.chime(kit.note(a), child(r, 'c1'), 0.45, 0.5, 0.0003)
    c2 = kit.chime(kit.note(b), child(r, 'c2'), 0.5, 0.5, 0.0003)
    x = layers((clasp, 0.0, 0.0), (sweep, 0.01, -9.0), (c1, 0.06, -17.0), (c2, 0.1, -19.0))
    return _ui(x, r, 0.35, -20.0, 0.3)


@cue('UI.Close', variations=2, space='2D', cls='Interface', jitter=0.02, conc=2, level=-15.0)
def close_page(v, r):
    # The page sweeps down and the clasp shuts with a lower, rounder clunk.
    sweep = kit.whoosh(0.17, child(r, 'sweep'), 2400.0, 380.0, 1.1, 0.45, -2.0, 1.0)
    clunk = kit.clack(child(r, 'clunk'), 520.0, 1200.0, 4200.0, 0.03, 0.06, 0.0, -8.0, 0.00018)
    clunk = F.filt(clunk, F.peak(3200.0, -4.0, 1.0), extend=False)
    x = layers((sweep, 0.0, -9.0), (clunk, 0.14 + 0.01 * v, 0.0))
    return _ui(x, r, 0.35, -21.0, 0.25)


@cue('UI.Tab', variations=3, space='2D', cls='Interface', jitter=0.03, conc=3, level=-18.0)
def tab(v, r):
    # A brass slide flicked over to the next stop: a short glide and a detent's tick.
    glide = kit.scrape(0.035, child(r, 'glide'), 1400.0, 2200.0, 8.0, 0.4, (1.0, 1.6), 0.3, 0.4, 3000.0, -24.0)
    detent = kit.brass_tick(child(r, 'detent'), 1550.0 * jitter(r, 1, 0.04), 0.018, 4, 0.0001, -10.0, -9.0, 0.1)
    x = layers((glide, 0.0, -14.0), (detent, 0.03, 0.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 1.0), extend=False)
    return _ui(x, r)


@cue('UI.Denied', variations=2, space='2D', cls='Interface', jitter=0.0, conc=2, level=-14.0)
def denied(v, r):
    # Not now: two dull, sour knocks on a locked lid, a low buzz under them.
    out = np.zeros(1)
    for i in range(2):
        knock = kit.thunk(300.0 * (1.0 - 0.06 * i), child(r, 'k', i), 0.07, 0.0012, 5, 0.05)
        sour = layers(kit.chime(kit.note('Bb4'), child(r, 's', i), 0.16, 0.5, 0.0005, 0.3, -40.0),
                      (kit.chime(kit.note('B4'), child(r, 't', i), 0.16, 0.5, 0.0005, 0.3, -40.0), 0.0, -2.0))
        n = ns(0.09)
        buzz = O.square(110.0 * (1.0 - 0.05 * i), n=n) * E.perc(0.09, 0.003, 0.06)
        buzz = F.filt(buzz, F.lp(1600.0, 0.8), extend=False)
        hit = layers((knock, 0.0, 0.0), (normalize(sour), 0.0, -4.0), (normalize(buzz), 0.0, -9.0))
        out = layers(out, (hit, 0.13 * i, -1.5 * i))
    return _ui(out, r, 0.2, -22.0, 0.2)


# --- The inventory (the loadout screen's gun moves) -----------------------------------------------------------------

def _loud(x, win=0.005):
    p = kit.env_peak(x, win)
    return x / p if p > 0 else x


def _seat(r, f_body=420.0, lo=900.0, hi=3400.0, t60=0.045, weight_f=1000.0, mix=(0.0, -2.0, -4.0), drive_db=4.0):
    """A gun seated in a rack or a slot: steel parts meeting (held to about 1-3.5 kHz, so it clanks rather than
    tinkles), the polymer and wood around them knocking, and the thud of its weight made of air, not a tone; mixed by
    how loud each is and driven a little so they fuse into one clack."""
    steel = M.strike(M.parts(lo, hi, child(r, 'steel'), 8, t60, t60 * 0.4, 0.45), M.hammer(0.0001), t60 * 1.8 + 0.01)
    body = M.strike(M.wood(f_body, child(r, 'body'), t60=0.05, count=6, spread=0.08), M.hammer(0.0005), 0.1)
    weight = kit.noise_thump(0.08, child(r, 'weight'), weight_f, 130.0, 0.04, 1.0, 0.03)
    x = layers((_loud(steel), 0.0, mix[0]), (_loud(body), 0.0, mix[1]), (_loud(weight), 0.0, mix[2]))
    return normalize(D.drive(normalize(x), drive_db, 'tanh'), 0.0)


@cue('UI.Equip', variations=3, space='2D', cls='Interface', jitter=0.03, conc=2, level=-13.0)
def inventory_equip(v, r):
    # A gun seated in its slot: a short slide home, then a solid steel-and-polymer clack with its weight behind it, and
    # the latch snapping over a hair later. The loadout's most satisfying sound.
    slide = kit.scrape(0.035, child(r, 'slide'), 650.0, 950.0, 6.0, 0.6, (1.0, 1.6, 2.5), 0.08, 0.4, 1600.0, -34.0)
    slide = F.filt(slide, F.lp(3000.0, 0.7), extend=False)
    seat = _seat(child(r, 'seat'), 400.0 * jitter(r, 1, 0.05), 850.0, 3200.0, 0.05, 1000.0, (0.0, -1.0, -3.0), 5.0)
    latch = kit.metal_click(child(r, 'latch'), 1500.0, 4200.0, 0.02, 0.0001, 6, -12.0)
    t = 0.026 + 0.006 * r.random()
    x = layers((slide, 0.0, -13.0), (seat, t, 0.0), (latch, t + 0.009 + 0.003 * r.random(), -9.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.9), extend=False)
    return _ui(x, r, 0.25, -22.0, 0.22)


@cue('UI.Stow', variations=3, space='2D', cls='Interface', jitter=0.03, conc=2, level=-15.0)
def inventory_stow(v, r):
    # Put away in the pack: the gun slides into canvas (cloth and a dark scuff) and settles with a soft thump, a buckle
    # ticking once.
    cloth = kit.cloth(0.2, child(r, 'cloth'), 250.0, 2400.0, 110.0, 0.06, 0.6)
    scuff = kit.scrape(0.16, child(r, 'scuff'), 500.0, 380.0, 4.0, 0.7, (1.0, 1.7), 0.1, 0.5, 1200.0, -30.0)
    scuff = F.filt(scuff, F.lp(2000.0, 0.7), extend=False)
    t = 0.16 + 0.03 * r.random()
    thump = kit.noise_thump(0.12, child(r, 'thump'), 650.0, 90.0, 0.06, 1.0, 0.05)
    knock = kit.thunk(240.0 * jitter(r, 1, 0.08), child(r, 'knock'), 0.05, 0.002, 5, 0.06)
    buckle = kit.metal_click(child(r, 'buckle'), 2000.0, 5500.0, 0.025, 0.00015, 4, -16.0)
    x = layers((cloth, 0.0, -5.0), (scuff, 0.005, -10.0), (thump, t, 0.0), (knock, t, -6.0), (buckle, t + 0.012, -20.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.9), extend=False)
    return _ui(x, r, 0.3, -22.0, 0.2)


@cue('UI.Drop', variations=3, space='2D', cls='Interface', jitter=0.03, conc=2, level=-12.0)
def inventory_drop(v, r):
    # Dropped on the boards: a heavy thud into the planks (their hollow knock, the gun's weight), its steel clanking,
    # one smaller bounce and a rattle as it settles.
    def hit(rr):
        boards = M.strike(M.wood(150.0 * jitter(rr, 1, 0.08), child(rr, 'boards'), t60=0.2, count=7, spread=0.08),
                          M.hammer(0.0012), 0.35)
        thud = kit.noise_thump(0.2, child(rr, 'thud'), 900.0, 70.0, 0.1, 1.0, 0.05)
        steel = M.strike(M.parts(700.0, 3000.0, child(rr, 'steel'), 8, 0.07, 0.03, 0.45), M.hammer(0.0001), 0.15)
        return layers((_loud(boards), 0.0, 0.0), (_loud(thud), 0.0, -1.0), (_loud(steel), 0.002, -7.0))
    first = hit(child(r, 'h1'))
    b = 0.085 + 0.02 * r.random()
    bounce = hit(child(r, 'h2'))
    settle = kit.jingle(0.25, child(r, 'settle'), 4, 1500.0, 4500.0, 0.04, 0.12, 0.0)
    x = layers((first, 0.0, 0.0), (bounce, b, -11.0), (settle, b + 0.02, -18.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.9), extend=False)
    return _ui(x, r, 0.25, -17.0, 0.4)


@cue('UI.Inspect', variations=2, space='2D', cls='Interface', jitter=0.03, conc=2, level=-18.0, swell=True)
def inventory_inspect(v, r):
    # Turning the gun up to look it over: a short whoosh of air rising, and a faint brass glint as it comes into the
    # light.
    rise = kit.whoosh(0.26, child(r, 'rise'), 260.0, 2100.0, 1.0, 0.75, -3.0, 1.3)
    glint = kit.chime(kit.note(['B5', 'A5'][v]), child(r, 'glint'), 0.3, 0.5, 0.0003, 0.4, -40.0)
    x = layers((rise, 0.0, 0.0), (glint, 0.2, -22.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), extend=False)
    return _ui(x, r, 0.45, -24.0, 0.2)


# --- The HUD's control hints and the notice board -------------------------------------------------------------------

@cue('UI.Hint', variations=2, space='2D', cls='Interface', jitter=0.02, conc=2, level=-19.0)
def hint(v, r):
    # A control hint coming up: a soft wooden tap and a low, round brass note under it, like a finger tapping the
    # ledger to point at something. A nudge (the HUD plays it quieter still): short and soft.
    tap = kit.thunk(720.0 * jitter(r, 1, 0.05), child(r, 'tap'), 0.025, 0.0006, 4, 0.05)
    f0 = kit.note(['E5', 'D5'][v])
    note = kit.chime(f0, child(r, 'note'), 0.4, 0.35, 0.0006, 0.3, -40.0)
    body = O.sine(f0, n=ns(0.35)) * E.perc(0.35, 0.004, 0.25)
    x = layers((tap, 0.0, 0.0), (note, 0.004, -5.0), (normalize(body), 0.004, -14.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.9), F.lp(7000.0, 0.7), extend=False)
    return _ui(x, r, 0.25, -22.0, 0.25)


@cue('UI.HintDone', variations=2, space='2D', cls='Interface', jitter=0.02, conc=2, level=-15.0)
def hint_done(v, r):
    # The hint's key used: a crisp brass tick as the keycap pops, and one small bright ping a fifth over the hint's
    # note, done.
    tick = kit.brass_tick(child(r, 'tick'), 1700.0 * jitter(r, 1, 0.03), 0.02, 4, 0.0001, -10.0, -8.0, 0.2)
    f0 = kit.note(['B5', 'A5'][v])
    ping = kit.chime(f0, child(r, 'ping'), 0.32, 0.7, 0.0002, 0.5, -30.0)
    body = O.sine(f0, n=ns(0.3)) * E.perc(0.3, 0.003, 0.2)
    x = layers((tick, 0.0, -2.0), (ping, 0.018, 0.0), (normalize(body), 0.018, -10.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.9), extend=False)
    return _ui(x, r, 0.25, -20.0, 0.3)


@cue('UI.BoardTurnIn', variations=2, space='2D', cls='Interface', jitter=0.02, conc=1, level=-10.0)
def board_turn_in(v, r):
    # A posting turned in at the board: the tack worked out of the wood (a short squeak of steel in timber and a pop as
    # it comes free), the paper torn off with a rip, then the clerk's stamp coming down on it (a solid wooden thump
    # with the rubber's slap, the board knocking behind it) and lifting with a tacky peel.
    squeak = kit.creak(0.08, child(r, 'squeak'), 260.0, 420.0, (1300.0, 2500.0, 3700.0), 9.0, 0.2)
    squeak = F.filt(squeak, F.peak(3000.0, -5.0, 1.0), extend=False)
    pop = kit.metal_click(child(r, 'pop'), 1600.0, 4800.0, 0.02, 0.0001, 5, -12.0)
    n = ns(0.12)
    rip = N.band(n, child(r, 'rip'), 900.0, 6000.0) * (0.2 + 0.8 * np.abs(N.smooth_random(n, child(r, 'ra'), 260.0)))
    rip = normalize(F.filt(rip * E.ar(0.12, 0.01, 0.06, 1.5), F.peak(3300.0, -4.0, 1.0), extend=False))
    t = 0.3 + 0.03 * r.random()
    stamp = kit.thunk(280.0 * jitter(r, 1, 0.05), child(r, 'stamp'), 0.07, 0.0015, 6, 0.08)
    thud = kit.noise_thump(0.12, child(r, 'thud'), 900.0, 120.0, 0.05, 1.0, 0.04)
    m = ns(0.03)
    slap = normalize(N.band(m, child(r, 'slap'), 250.0, 2500.0) * E.perc(0.03, 0.0005, 0.012))
    board = kit.thunk(150.0 * jitter(r, 1, 0.05), child(r, 'board'), 0.12, 0.002, 6, 0.08)
    k = ns(0.06)
    peel = normalize(N.band(k, child(r, 'peel'), 700.0, 3500.0) * (0.3 + 0.7 * np.abs(N.smooth_random(k, child(r, 'pa'), 180.0)))
                     * E.ar(0.06, 0.02, 0.03, 1.5))
    x = layers((squeak, 0.0, -14.0), (pop, 0.075, -10.0), (rip, 0.11, -14.0), (stamp, t, 0.0), (thud, t, -3.0),
               (slap, t, -8.0), (board, t + 0.002, -6.0), (peel, t + 0.14, -22.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.9), extend=False)
    return _ui(x, r, 0.25, -18.0, 0.3)
