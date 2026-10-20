"""Wendell Pike's ember powers (Embers/Powers: Spyglass, Tripwire, Long Shot): a surveyor's brass, wire and patience.
Small precise things, glass and breath. What he does is his (2D); what happens out in the world is placed (3D).

- The spyglass drawn out (2D): brass tubes sliding out one after another and clicking home, a lens's glint (a soft high
  shimmer) and a sweep of air going out wide from the middle to both ears (the ring running out over the ground).
- A creature found (2D, quiet, many in a row): a small brass tick and a high soft ping.
- The spyglass shut (2D): the tubes pushed home, three quick soft clicks.
- The wire strung (2D): two stakes knocked into the ground (dull wooden knocks, the second a little farther), the wire
  drawn taut (a rising zing) and humming.
- A creature caught (3D): the wire's twang (a plucked steel wire, struck hard) and a body hitting the ground.
- The wire let go (3D): a falling twang as it goes slack, a stake's wobble.
- Long Shot steadied (2D): a breath drawn in and held, a scope's ring turned in fine clicks, a far, high, held tone.
- A long shot landing (2D, every hit, quiet; the game pitches it up with the distance): a distant bright tink.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, fade
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S
from lib import granular as G
from recipes import cue
from recipes import kit


# --- Brass, wire and breath ----------------------------------------------------------------------------------------------

def _tube(r, f0=1400.0):
    """A brass tube section sliding home: a short rub and the tube's ring as it seats with a click."""
    rub = kit.scrape(0.08, child(r, 'rub'), 1800.0, 2400.0, 10.0, 0.4, attack=0.3, release=0.4)
    seat = kit.brass_tick(child(r, 'seat'), f0 * jitter(r, 1, 0.05), 0.05, 4, 0.0001, -10.0, -8.0, 0.4)
    ring = normalize(M.strike(M.shell(f0 * 0.7, child(r, 'ring'), 0.18, 6), M.hammer(0.0001), 0.3))
    return normalize(layers((rub, 0.0, -10.0), (seat, 0.07, 0.0), (ring, 0.07, -10.0)), 0.0)


def _wire(f0, dur, r, bright=0.7, t60=1.2):
    """A taut steel wire plucked: a string's harmonics with a little stiffness, and a thin metallic buzz on its attack."""
    s = O.string(f0, dur, r, 0.12, t60, bright, 0.0004, 0.25, detune=1.0)
    pick = N.white(ns(0.003), child(r, 'pick')) * E.decay(ns(0.003), 0.0015)
    pick = F.filt(pick, F.bp(3000.0, 1.0), extend=False)
    return normalize(layers(normalize(s), (normalize(pick), 0.0, -18.0)), 0.0)


def _calm(x, hp=120.0):
    return F.filt(x, F.peak(3300.0, -6.0, 0.8), F.highshelf(8000.0, -4.0), F.hp(hp), extend=False)


def _own(x, r, wet_db=-18.0, t60=1.4):
    """His own, in stereo, with a wide, light open-air tail (he works in the open)."""
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=t60, lp=3500.0)
    return R.apply(x, ir, wet_db, 0.0, pre_hp=200.0)


def _yard(x, r, rel_db=-14.0, dur=1.2):
    """Out in the world: slap-backs and a dark tail (mono, for a placed sound)."""
    ir = R.outdoor(child(r, 'yard'), dur=dur, slaps=((0.1, -8.0), (0.25, -13.0)), tail_t60=0.9, tail_db=-14.0, tail_lp=2600.0)
    wet = F.convolve(F.filt(x, F.hp(200.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


# --- Spyglass ------------------------------------------------------------------------------------------------------------

@cue('Ember.Spyglass.Start', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-9.0)
def spyglass_start(v, r):
    # Three brass sections drawn out and seated, a lens's glint, and a sweep of air spreading from the middle out to
    # both ears as the ring runs out over the ground.
    tubes = layers((_tube(child(r, 't1'), 1500.0), 0.0, 0.0), (_tube(child(r, 't2'), 1250.0), 0.12, -1.0),
                   (_tube(child(r, 't3'), 1050.0), 0.23, -2.0))
    glint = layers((kit.chime(2349.0, child(r, 'g1'), 0.8, 0.5), 0.0, 0.0), (kit.chime(3136.0, child(r, 'g2'), 0.6, 0.4), 0.04, -6.0))
    dur = 1.0
    n = ns(dur)
    sweep = F.sweep(N.pink(n, child(r, 'sweep')), 'bp', O.expsweep(500.0, 2600.0, dur, 1.0, n=n), 1.2) * E.ar(dur, 0.25, 0.7, 1.5)
    spread = np.linspace(0.0, 1.0, n)
    a = (spread + 1.0) * np.pi / 4.0
    b = (-spread + 1.0) * np.pi / 4.0
    wide = np.vstack([normalize(sweep) * np.cos(b) * np.sqrt(2.0), normalize(sweep) * np.sin(a) * np.sqrt(2.0)]) * 0.7
    x = layers((S.pan(tubes, 0.2), 0.0, 0.0), (S.widen(glint, child(r, 'gw'), 0.3), 0.3, -10.0), (wide, 0.3, -9.0))
    return _own(_calm(x, 150.0), r, -17.0, 1.6)


@cue('Ember.Spyglass.Mark', variations=4, space='2D', cls='Effects', jitter=0.02, conc=3, level=-20.0)
def spyglass_mark(v, r):
    # A creature found: a small brass tick and a high soft ping (the game raises its pitch a little with each).
    tick = kit.brass_tick(child(r, 'tick'), 1900.0 * jitter(r, 1, 0.03), 0.03, 3, 0.0001, -12.0, -10.0, 0.3)
    ping = kit.chime([1046.5, 1174.7, 1318.5, 1396.9][v], child(r, 'ping'), 0.3, 0.35)
    x = fade(normalize(layers((tick, 0.0, -2.0), (ping, 0.008, 0.0))), 0.0, 0.05)
    return S.widen(_calm(x, 500.0), child(r, 'w'), 0.3)


@cue('Ember.Spyglass.End', variations=2, space='2D', cls='Effects', jitter=0.02, conc=1, level=-14.0)
def spyglass_end(v, r):
    # The spyglass shut: the tubes pushed home, three quick soft clicks with a hush of brass sliding.
    clicks = layers(*[(kit.brass_tick(child(r, 'c', i), 1300.0 + 150.0 * i, 0.04, 4, 0.0001, -10.0, -8.0, 0.3), 0.05 * i, -1.5 * i)
                      for i in range(3)])
    slide = kit.scrape(0.18, child(r, 'slide'), 2200.0, 1600.0, 9.0, 0.4)
    x = layers((slide, 0.0, -9.0), (clicks, 0.02, 0.0))
    x = fade(_calm(normalize(x), 200.0), 0.0, 0.04)
    return S.widen(x, child(r, 'w'), 0.25)


# --- Tripwire ------------------------------------------------------------------------------------------------------------

@cue('Ember.Tripwire.Set', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-10.0)
def tripwire_set(v, r):
    # Two stakes knocked into the ground, the second a little farther off, then the wire drawn taut (a rising zing) and
    # humming a moment.
    knock1 = layers((kit.thunk(220.0 * jitter(r, 1, 0.05), child(r, 'k1'), 0.08, 0.001, 6, 0.08), 0.0, 0.0),
                    (kit.noise_thump(0.1, child(r, 'd1'), 500.0, 90.0, 0.04), 0.0, -5.0))
    knock2 = layers((kit.thunk(240.0 * jitter(r, 1, 0.05), child(r, 'k2'), 0.08, 0.001, 6, 0.08), 0.0, 0.0),
                    (kit.noise_thump(0.1, child(r, 'd2'), 500.0, 90.0, 0.04), 0.0, -5.0))
    dur = 0.55
    n = ns(dur)
    f = O.expsweep(180.0, 420.0, dur, 0.6, n=n)
    zing = np.sin(2.0 * np.pi * O.phase(f, n)) + 0.3 * np.sin(4.0 * np.pi * O.phase(f, n)) + 0.12 * np.sin(6.0 * np.pi * O.phase(f, n))
    zing = normalize(zing * E.ar(dur, 0.25, 0.3, 1.5))
    hum = _wire(420.0, 0.9, child(r, 'hum'), 0.5, 0.9)
    x = layers((S.pan(knock1, -0.45), 0.0, 0.0), (S.pan(knock2, 0.45), 0.28, -2.0), (S.widen(zing, child(r, 'zw'), 0.3), 0.45, -12.0),
               (S.widen(hum, child(r, 'hw'), 0.3), 0.95, -11.0))
    return _own(_calm(x, 60.0), r, -19.0, 1.2)


@cue('Ember.Tripwire.Trip', variations=3, att='Creature', jitter=0.05, conc=2, level=-9.0)
def tripwire_trip(v, r):
    # Caught: the wire's twang, struck hard and wobbling, and a body hitting the ground with dust.
    twang = _wire([330.0, 300.0, 360.0][v], 0.9, child(r, 'twang'), 0.8, 0.8)
    t = times(twang.size)
    twang = twang * (1.0 + 0.25 * np.sin(2.0 * np.pi * 9.0 * t) * np.exp(-t / 0.3))
    body = layers((kit.noise_thump(0.3, child(r, 'body'), 450.0, 60.0, 0.14), 0.0, 0.0), (kit.thump(70.0, 38.0, 0.3, 0.16), 0.0, -4.0))
    dust = kit.grit_burst(0.25, child(r, 'dust'), 3000.0, 500.0, 3500.0, 0.12)
    x = layers((normalize(twang), 0.0, 0.0), (body, 0.09, -2.0), (dust, 0.1, -12.0))
    return _yard(_calm(normalize(x), 50.0), r, -15.0)


@cue('Ember.Tripwire.Slack', variations=2, att='Near', jitter=0.04, conc=1, level=-15.0)
def tripwire_slack(v, r):
    # Let go: the wire's pitch falling as it goes slack, and a stake's wooden wobble.
    dur = 0.6
    n = ns(dur)
    f = O.expsweep(380.0, 150.0, dur, 1.4, n=n)
    fall = (np.sin(2.0 * np.pi * O.phase(f, n)) + 0.25 * np.sin(4.0 * np.pi * O.phase(f, n))) * E.perc(dur, 0.002, 0.4)
    wobble = kit.thunk(260.0, child(r, 'wobble'), 0.06, 0.001, 6, 0.08)
    x = layers((normalize(fall), 0.0, 0.0), (wobble, 0.05, -8.0))
    return _yard(_calm(normalize(x), 80.0), r, -18.0, 0.9)


# --- Long Shot -----------------------------------------------------------------------------------------------------------

@cue('Ember.LongShot.Start', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-9.0)
def longshot_start(v, r):
    # A breath drawn in and held, a scope's ring turned in fine clicks, and a far, high tone held under it all.
    dur = 0.6
    n = ns(dur)
    inhale = F.sweep(N.pink(n, child(r, 'in')), 'bp', O.expsweep(500.0, 1300.0, dur, 1.0, n=n), 1.0) * E.ar(dur, 0.4, 0.15, 1.5)
    clicks = layers(*[(kit.brass_tick(child(r, 'c', i), 2600.0 + 60.0 * (i % 3), 0.02, 3, 0.00008, -12.0, -10.0, 0.3), 0.04 * i,
                       -1.0 * (i % 2)) for i in range(7)])
    tdur = 1.4
    m = ns(tdur)
    tone = (np.sin(2.0 * np.pi * 1318.5 * times(m)) + 0.3 * np.sin(2.0 * np.pi * 1976.0 * times(m))) * E.swell(tdur, 0.5, 0.8)
    x = layers((S.pan(clicks, 0.25), 0.0, -6.0), (S.widen(normalize(inhale), child(r, 'iw'), 0.3), 0.0, -6.0),
               (S.widen(normalize(tone), child(r, 'tw'), 0.4), 0.3, -18.0))
    return _own(_calm(x, 120.0), r, -18.0, 1.8)


@cue('Ember.LongShot.Hit', variations=3, space='2D', cls='Effects', jitter=0.02, conc=2, level=-19.0)
def longshot_hit(v, r):
    # A long shot landing: a distant bright tink (the game raises its pitch with the distance).
    tink = kit.chime([1568.0, 1661.0, 1480.0][v], child(r, 'tink'), 0.35, 0.45)
    far = kit.noise_thump(0.06, child(r, 'far'), 1500.0, 500.0, 0.02)
    x = fade(normalize(layers((tink, 0.0, 0.0), (far, 0.0, -10.0))), 0.0, 0.05)
    return S.widen(_calm(x, 400.0), child(r, 'w'), 0.2)
