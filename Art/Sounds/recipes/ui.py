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
