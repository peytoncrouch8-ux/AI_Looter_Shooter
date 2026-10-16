"""Ember powers (Embers/Powers/*, UEmberComponent, the HUD's socket): Ellis's three and the socket's two. All the player's
own, so 2D; each a short scene of what the power is, physically, with a little of the uncanny on top.

- Dust Devil's dash: a gust ripped past both ears from behind (noise through a band-pass sweeping up then down, the left
  ear a hair ahead of the right), the low whump of the air shoved aside, dry grit hissing up off the ground, and a little
  whirlwind spinning off into the distance as it settles (band noise beating faster then slower, panned round).
- Sundown falling: a tape-stop: the world's hum (a low open chord) sinking an octave as it slows, a low bronze chime
  struck and played slow, a warm drone swelling in under it, and one heavy heartbeat thud at the end. Lifting: the
  chord wound back up to speed and a bright shimmer as time catches up.
- Borrowed Time opened: a ledger's heavy cover thrown open (a dull board knock and the leather's flap), a steel nib
  scratching a line, a deep clock tick, and a cold minor chord breathing in behind it. Settled: the ledger slammed
  shut, three slow heavy ticks, each lower, and the chord sinking flat.
- Ready: a small ember catching (a soft breath of flame) and a warm, bright chime. Denied: a dull ember's dry fizzle and
  a muted wooden click. Both quiet: they come every few seconds in a fight.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, varispeed
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S
from lib import granular as G
from recipes import cue
from recipes import kit


# --- Shared pieces -----------------------------------------------------------------------------------------------------

def _space(x, r, wet_db=-16.0, t60=1.6, lp=3000.0):
    """The player's own sound with a dark open-air tail behind it, in stereo."""
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=t60, lp=lp)
    return R.apply(x, ir, wet_db, 0.0, pre_hp=150.0)


def _chord(freqs, dur, glide, r, bright=0.5, detune=0.15):
    """A soft organ-like chord of near-sine tones, each tone's pitch following glide(t) (a multiplier curve per sample),
    each a pair of slightly detuned voices so it breathes."""
    n = ns(dur)
    x = np.zeros(n)
    for i, f in enumerate(freqs):
        for s in (-1.0, 1.0):
            freq = f * glide * (1.0 + s * detune * 0.01 * (1.0 + 0.3 * r.random()))
            x += O.additive(freq, [1.0, 0.35 * bright, 0.15 * bright, 0.06 * bright], n=n) * (1.0 / (1.0 + 0.4 * i))
    return normalize(x, 0.0)


def _rush(dur, r, f0, fpeak, f1, peak_at, q=1.3):
    """Air torn past the ears: pink noise through a band-pass sweeping f0 -> fpeak -> f1, swelling to peak_at."""
    n = ns(dur)
    fc = O.glide([(0.0, f0), (dur * peak_at, fpeak), (dur, f1)], n=n)
    air = F.sweep(N.pink(n, r) * kit.turbulence(n, child(r, 'turb'), 90.0, 0.4), 'bp', fc, q)
    u = np.arange(n) / n
    env = np.where(u < peak_at, np.sin(0.5 * np.pi * np.minimum(u / peak_at, 1.0)) ** 2.5,
                   np.exp(-5.0 * (u - peak_at) / max(1.0 - peak_at, 1e-3)))
    return normalize(air * E.end_fade(env, 0.01), 0.0)


# --- Dust Devil --------------------------------------------------------------------------------------------------------

@cue('Ember.DustDevil.Use', variations=3, space='2D', cls='Effects', jitter=0.04, conc=2, level=-8.0)
def dust_devil(v, r):
    # The gust ripped past from behind: one rush per ear, the left a hair ahead, crossing front as it goes by.
    dur = 0.55 + 0.05 * r.random()
    left = _rush(dur, child(r, 'rl'), 350.0, 2600.0 * jitter(r, 1, 0.1), 500.0, 0.2)
    right = _rush(dur, child(r, 'rr'), 330.0, 2400.0 * jitter(r, 1, 0.1), 480.0, 0.24)
    rush = np.vstack([left, right])
    # The air shoved aside: a low whump at the start.
    whump = kit.noise_thump(0.3, child(r, 'whump'), 260.0, 55.0, 0.18, 1.0, 0.08)
    # Grit kicked up off the ground: a dry hiss of sand that dies away.
    gn = ns(0.45)
    grit = G.grit(0.45, child(r, 'grit'), 7000.0 * np.exp(-times(gn) / 0.12), 2200.0, 7500.0, 2.0, 4)
    grit = normalize(grit * E.perc(0.45, 0.004, 0.35))
    # The little whirlwind spinning off: band noise beating ever slower, panned round, fading into the distance.
    wd = 0.75
    wn = ns(wd)
    t = times(wn)
    spin_rate = 11.0 * np.exp(-t / 0.6) + 3.0
    spin = 2.0 * np.pi * np.cumsum(spin_rate) / SR
    whirl = F.sweep(N.band(wn, child(r, 'whirl'), 300.0, 2500.0), 'bp', O.glide([(0.0, 1400.0), (wd, 550.0)], n=wn), 2.0)
    whirl = normalize(whirl * (0.55 + 0.45 * np.sin(spin)) * E.ar(wd, 0.12, 0.5, 2.0))
    pan = 0.6 * np.sin(spin * 0.5 + r.uniform(0.0, np.pi))
    a = (pan + 1.0) * np.pi / 4.0
    whirl = np.vstack([whirl * np.cos(a), whirl * np.sin(a)]) * np.sqrt(2.0)
    x = layers((rush, 0.0, 0.0), (whump, 0.0, -5.0), (S.widen(grit, child(r, 'gw'), 0.4), 0.02, -13.0), (whirl, 0.22, -15.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.highshelf(8000.0, -3.0), F.hp(40.0), extend=False)
    return _space(x, r, -20.0, 1.1, 2500.0)


# --- Sundown -----------------------------------------------------------------------------------------------------------

@cue('Ember.Sundown.Start', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-7.0)
def sundown_start(v, r):
    # A tape-stop: the world's low hum sinking an octave as everything slows, the fall quick at first and dragging at the end.
    dur = 1.7
    n = ns(dur)
    t = times(n)
    fall = 0.5 + 0.5 * np.exp(-t / 0.45)
    hum = _chord([73.42, 110.0, 146.83, 220.0], dur, fall, child(r, 'hum'), 0.7)
    hum = normalize(hum * E.ar(dur, 0.01, 0.9, 1.5))
    # The noise of the world dragged down with it.
    drag = F.sweep(N.pink(n, child(r, 'drag')), 'lp', 4000.0 * fall ** 3, 0.9)
    drag = normalize(drag * E.perc(dur, 0.005, 0.9))
    # A low bronze chime, struck and played slow.
    bell = varispeed(kit.chime(330.0 * jitter(r, 1, 0.02), child(r, 'bell'), 2.4, 0.7), 0.6)
    # The warm drone swelling in under it.
    drone = _chord([55.0, 82.41], dur, np.ones(n), child(r, 'drone'), 0.4)
    drone = normalize(drone * E.swell(dur, 0.9, 1.4))
    # One heavy, slow heartbeat at the bottom of the fall.
    thud = kit.thump(70.0, 38.0, 0.45, 0.32)
    x = layers((S.widen(hum, child(r, 'hw'), 0.25), 0.0, 0.0), (S.widen(drag, child(r, 'dw'), 0.4), 0.0, -12.0),
               (S.widen(bell, child(r, 'bw'), 0.3), 0.03, -9.0), (S.widen(drone, child(r, 'drw'), 0.2), 0.0, -6.0),
               (thud, 1.15, -4.0))
    x = F.filt(x, F.highshelf(6000.0, -4.0), F.hp(30.0), extend=False)
    return _space(x, r, -14.0, 2.2, 2200.0)


@cue('Ember.Sundown.End', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-9.0, swell=True)
def sundown_end(v, r):
    # The fall run backwards: the hum wound back up to speed, quick at the end, and a bright shimmer as time catches up.
    dur = 0.95
    n = ns(dur)
    t = times(n)
    rise = 0.5 + 0.5 * (1.0 - np.exp(-np.maximum(t - 0.05, 0.0) / 0.22))
    hum = _chord([73.42, 110.0, 146.83, 220.0], dur, rise, child(r, 'hum'), 0.7)
    hum = normalize(hum * E.ar(dur, 0.2, 0.35, 1.5))
    whoosh = kit.whoosh(0.7, child(r, 'whoosh'), 400.0, 3500.0, 1.0, 0.8, -3.0)
    shimmer = layers((kit.chime(1318.5, child(r, 's1'), 0.9, 0.6), 0.0, 0.0), (kit.chime(1975.5, child(r, 's2'), 0.7, 0.5), 0.03, -5.0),
                     (kit.chime(2637.0, child(r, 's3'), 0.5, 0.4), 0.06, -9.0))
    x = layers((S.widen(hum, child(r, 'hw'), 0.25), 0.0, -2.0), (S.widen(whoosh, child(r, 'ww'), 0.4), 0.15, -8.0),
               (S.widen(shimmer, child(r, 'sw'), 0.5), 0.62, -11.0))
    x = F.filt(x, F.peak(3000.0, -3.0, 0.8), F.highshelf(9000.0, -3.0), F.hp(35.0), extend=False)
    return _space(x, r, -16.0, 1.6, 4000.0)


# --- Borrowed Time -----------------------------------------------------------------------------------------------------

def _nib(dur, r):
    """A steel nib scratching a line on rough paper: fine high grit pulsed by the stroke, with a faint squeak."""
    n = ns(dur)
    t = times(n)
    stroke = 0.5 + 0.5 * np.abs(np.sin(2.0 * np.pi * 7.0 * t + r.uniform(0.0, np.pi)))
    grit = G.grit(dur, child(r, 'g'), 5000.0 * stroke, 2500.0, 8000.0, 3.0, 4)
    squeak = O.sine(O.glide([(0.0, 2900.0), (dur, 3300.0)], n=n)) * 0.08 * stroke
    return normalize((normalize(grit) + squeak) * E.ar(dur, 0.03, 0.08, 2.0), 0.0)


def _tick(f0, r, weight=1.0):
    """A big clock's escapement tick: a brass tick over a hollow wooden knock."""
    tick = kit.brass_tick(child(r, 'tick'), f0, 0.05, 4, 0.0001, -10.0, -8.0, 0.3)
    case = kit.thunk(f0 * 0.18, child(r, 'case'), 0.09, 0.0006, 6, 0.08)
    return normalize(layers((tick, 0.0, -3.0), (case, 0.0, -2.0 + 2.0 * weight)), 0.0)


@cue('Ember.BorrowedTime.Start', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-8.0)
def borrowed_start(v, r):
    # The ledger's heavy cover thrown open: the board's knock, the leather's flap.
    cover = kit.thunk(170.0 * jitter(r, 1, 0.05), child(r, 'cover'), 0.12, 0.002, 6, 0.08)
    flap = kit.cloth(0.18, child(r, 'flap'), 250.0, 2500.0, 90.0, 0.2, 0.6)
    # A line written into it, then the clock's one deep tick.
    nib = _nib(0.32, child(r, 'nib'))
    tick = _tick(1250.0, child(r, 'tick'))
    # A cold minor chord breathing in behind, a breath of air through it.
    dur = 1.3
    chord = _chord([110.0, 130.81, 164.81, 220.0], dur, np.ones(ns(dur)), child(r, 'chord'), 0.5, 0.3)
    breath = N.band(ns(dur), child(r, 'breath'), 500.0, 3000.0)
    swell = E.swell(dur, 0.55, 0.9)
    cold = normalize(normalize(chord) * swell + 0.12 * normalize(breath) * swell)
    x = layers((cover, 0.0, -2.0), (flap, 0.0, -10.0), (S.pan(nib, 0.25), 0.16, -12.0), (tick, 0.5, -4.0),
               (S.widen(cold, child(r, 'cw'), 0.35), 0.08, -6.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.highshelf(8000.0, -3.0), F.hp(45.0), extend=False)
    return _space(x, r, -15.0, 1.8, 2600.0)


@cue('Ember.BorrowedTime.Settle', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-7.0)
def borrowed_settle(v, r):
    # The ledger slammed shut: a heavy board's clap with air pushed out of it.
    slam = layers((kit.thunk(140.0 * jitter(r, 1, 0.05), child(r, 'slam'), 0.14, 0.001, 6, 0.08), 0.0, 0.0),
                  (kit.noise_thump(0.25, child(r, 'air'), 400.0, 90.0, 0.15), 0.0, -5.0))
    # The clock gone slow and heavy: three ticks, each later and lower.
    ticks = layers((_tick(1100.0, child(r, 't1'), 1.0), 0.0, 0.0), (_tick(950.0, child(r, 't2'), 1.4), 0.42, 0.0),
                   (_tick(800.0, child(r, 't3'), 1.8), 0.92, 1.0))
    # The chord sinking flat as the debt is paid.
    dur = 1.5
    n = ns(dur)
    sink = 1.0 - 0.12 * (1.0 - np.exp(-times(n) / 0.6))
    chord = _chord([98.0, 116.54, 146.83], dur, sink, child(r, 'chord'), 0.5, 0.3)
    chord = normalize(chord * E.ar(dur, 0.05, 0.9, 1.5))
    thud = kit.thump(60.0, 35.0, 0.5, 0.35)
    x = layers((slam, 0.0, 0.0), (ticks, 0.25, -6.0), (S.widen(chord, child(r, 'cw'), 0.3), 0.05, -7.0), (thud, 0.0, -6.0))
    x = F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(7500.0, -4.0), F.hp(35.0), extend=False)
    return _space(x, r, -14.0, 2.0, 2200.0)


# --- The socket's own --------------------------------------------------------------------------------------------------

@cue('Ember.Ready', variations=2, space='2D', cls='Interface', jitter=0.02, conc=1, level=-14.0, swell=True)
def ready(v, r):
    # An ember catching: a soft breath of flame swelling in, then a warm, bright chime.
    dur = 0.3
    n = ns(dur)
    flame = F.sweep(N.pink(n, child(r, 'flame')) * kit.turbulence(n, child(r, 'ft'), 40.0, 0.5), 'bp',
                    O.glide([(0.0, 600.0), (dur, 2200.0)], n=n), 1.1)
    flame = normalize(flame * E.ar(dur, 0.12, 0.15, 2.0))
    chime = kit.chime(1046.5 * jitter(r, 1, 0.01), child(r, 'chime'), 0.7, 0.55)
    fifth = kit.chime(1568.0, child(r, 'fifth'), 0.5, 0.4)
    x = layers((S.widen(flame, child(r, 'fw'), 0.3), 0.0, -8.0), (chime, 0.1, 0.0), (fifth, 0.13, -9.0))
    x = F.filt(x, F.peak(3200.0, -4.0, 0.8), F.highshelf(8000.0, -4.0), F.hp(200.0), extend=False)
    return _space(x, r, -20.0, 0.9, 5000.0)


@cue('Ember.Denied', variations=2, space='2D', cls='Interface', jitter=0.03, conc=2, level=-16.0)
def denied(v, r):
    # A dull ember's dry fizzle and a muted wooden click: pressed, and nothing there.
    dur = 0.16
    n = ns(dur)
    fizz = G.grit(dur, child(r, 'fizz'), 6000.0 * np.exp(-times(n) / 0.05), 1500.0, 5000.0, 2.0, 3)
    fizz = normalize(fizz * E.perc(dur, 0.002, 0.12))
    click = kit.thunk(620.0 * jitter(r, 1, 0.04), child(r, 'click'), 0.04, 0.0004, 5, 0.06)
    x = layers((click, 0.0, 0.0), (fizz, 0.004, -9.0))
    x = F.filt(x, F.peak(3200.0, -5.0, 0.8), F.lp(6000.0), F.hp(150.0), extend=False)
    return S.widen(x, child(r, 'w'), 0.15)
