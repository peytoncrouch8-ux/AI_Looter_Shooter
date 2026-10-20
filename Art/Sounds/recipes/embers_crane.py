"""Hollis Crane's ember powers (Embers/Powers: Toll Chain, Grave Ward, Last Rites): a dead marshal's iron, lantern and
bell, cold and slow. What he does is his (2D); what lands out in the world is placed (3D).

- The toll chain thrown (2D): heavy iron links rattling off the shoulder, faster as they pay out, a whip of air crossing
  in front, and one far, low toll of a bell under it (what he's owed).
- The hook biting (3D): a heavy iron hook driven in (a thick bar's clank over a dull meaty thud), the chain snapping taut
  (a short, hard ring of iron under strain) and its links rattling home as it hauls.
- The lantern planted (3D): its iron foot set down on stone (a knock and a tin rattle of its glass), the wick catching
  (a soft breath of flame swelling) and a warm open chord rising in it: mercy.
- The lantern burning (3D loop): a small flame's soft flutter over a low warm hum, built in a circle.
- The lantern going out (3D): the flame snuffed in a puff, the glass's small tink, the hum let go downward.
- A creature marked (2D): one low funeral bell, struck and let ring, and a whispered breath of a chord under it.
- Its soul bursting (3D): a soft heavy thump and a bright bell's crash at once, souls rushing out in a wide hiss, a
  choir's cry cut short in it.
- The mark striking (3D): a heavy bell's blow, a crack of light (a sharp electric snap), a deep boom rolling off.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, fade
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S, dist as D
from lib import granular as G
from recipes import cue
from recipes import kit


# --- Iron, bells and breath -------------------------------------------------------------------------------------------

def _link(r, lo=700.0, hi=2800.0):
    """One heavy chain link knocking on the next: thick iron's short, dull clink."""
    m = M.parts(lo * r.uniform(0.85, 1.15), hi, r, 5, 0.05, 0.02, 0.5)
    return normalize(M.strike(m, M.hammer(0.00015), 0.09), 0.0)


def _links(dur, r, count, accel=1.0, lo=700.0, hi=2800.0):
    """count links clinking over dur: evenly, or bunching toward the end (accel over 1) as the chain runs faster."""
    out = np.zeros(ns(dur))
    for i in range(count):
        u = (i + r.uniform(0.0, 0.7)) / count
        t = dur * u ** accel
        out = layers(out, (_link(child(r, 'link', i), lo, hi) * r.uniform(0.4, 1.0), t, 0.0))
    return normalize(out, 0.0)


def _bell(prime, r, t60=4.0, bright=0.8):
    """A cast iron-and-bronze bell's modes (the church bell's partial series), struck once by a heavy clapper."""
    m = M.bell(prime, child(r, 'bell'), t60, 0.7, bright, 10)
    rung = normalize(M.strike(m, M.hammer(0.0003), t60 * 0.8))
    tick = kit.burst(0.004, child(r, 'tick'), 0.002, lo=1500.0, hi=6000.0)
    return normalize(layers(rung, (tick, 0.0, -20.0)), 0.0)


def _chord(freqs, dur, r, bright=0.4, detune=0.2):
    """A soft breathy chord of near-sine voices, each a slightly detuned pair."""
    n = ns(dur)
    x = np.zeros(n)
    for i, f in enumerate(freqs):
        for s in (-1.0, 1.0):
            freq = np.full(n, f * (1.0 + s * detune * 0.01 * (1.0 + 0.3 * r.random())))
            x += O.additive(freq, [1.0, 0.3 * bright, 0.1 * bright], n=n) / (1.0 + 0.4 * i)
    return normalize(x, 0.0)


def _breath(dur, r, lo=500.0, hi=3500.0):
    """A whisper of air through a chord: band noise in slow waves."""
    n = ns(dur)
    x = N.band(n, child(r, 'breath'), lo, hi) * (0.5 + 0.5 * np.abs(N.smooth_random(n, child(r, 'wave'), 3.0)))
    return normalize(x, 0.0)


def _cold(x, r, wet_db=-14.0, t60=2.2, lp=2400.0):
    """His own, in stereo, with a long dark open-air tail: a dead man's world is wide and cold."""
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=t60, lp=lp)
    return R.apply(x, ir, wet_db, 0.0, pre_hp=150.0)


def _yard(x, r, rel_db=-14.0, dur=1.4):
    """Out in the world: slap-backs and a dark tail (mono, for a placed sound)."""
    ir = R.outdoor(child(r, 'yard'), dur=dur, slaps=((0.1, -8.0), (0.26, -13.0)), tail_t60=1.0, tail_db=-13.0, tail_lp=2400.0)
    wet = F.convolve(F.filt(x, F.hp(200.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


def _soft(x, hp=60.0):
    return F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(8000.0, -4.0), F.hp(hp), extend=False)


# --- Toll Chain ----------------------------------------------------------------------------------------------------------

@cue('Ember.TollChain.Throw', variations=3, space='2D', cls='Effects', jitter=0.03, conc=1, level=-9.0)
def toll_throw(v, r):
    # The links rattling off the shoulder, faster as the chain pays out, a whip of air crossing in front, and far under it
    # one low toll of a bell: what he's owed.
    links = _links(0.32, child(r, 'links'), 14, 1.6)
    whip = kit.whoosh(0.28, child(r, 'whip'), 300.0, 2200.0, 1.0, 0.65, -3.0)
    n = whip.size
    a = (np.linspace(0.4, -0.6, n) + 1.0) * np.pi / 4.0
    whip = np.vstack([whip * np.cos(a), whip * np.sin(a)]) * np.sqrt(2.0)
    toll = F.filt(_bell(98.0 * jitter(r, 1, 0.02), child(r, 'toll'), 3.0, 0.6), F.lp(1800.0), extend=False)
    x = layers((S.pan(links, 0.35), 0.0, 0.0), (whip, 0.04, -4.0), (S.widen(toll, child(r, 'tw'), 0.3), 0.06, -12.0))
    return _cold(_soft(x), r, -15.0, 2.0)


@cue('Ember.TollChain.Hook', variations=3, att='Creature', jitter=0.04, conc=2, level=-9.0)
def toll_hook(v, r):
    # The hook driven in (a heavy iron clank over a dull, meaty thud), the chain snapping taut (a short hard ring of iron
    # under strain), and its links rattling home as it hauls.
    clank = layers((_link(child(r, 'clank'), 500.0, 2200.0), 0.0, 0.0),
                   (normalize(M.strike(M.bar(420.0 * jitter(r, 1, 0.05), child(r, 'bar'), 0.25, 0.6, 5), M.hammer(0.0002), 0.35)), 0.0, -4.0))
    thud = kit.noise_thump(0.18, child(r, 'thud'), 700.0, 90.0, 0.08)
    taut = normalize(M.strike(M.bar(880.0 * jitter(r, 1, 0.04), child(r, 'taut'), 0.4, 0.6, 5), M.hammer(0.0001), 0.5))
    haul = _links(0.42, child(r, 'haul'), 16, 1.0)
    x = layers((thud, 0.0, -1.0), (clank, 0.0, 0.0), (taut, 0.05, -8.0), (haul, 0.08, -6.0))
    return _yard(_soft(normalize(x), 70.0), r, -15.0)


# --- Grave Ward ----------------------------------------------------------------------------------------------------------

def _flame_breath(dur, r, periodic=False):
    """A small flame's soft flutter: low-mid noise breathing quickly and unevenly."""
    n = ns(dur)
    x = N.shaped(n, child(r, 'flame'), -3.0, 150.0, 2200.0, periodic=periodic)
    x *= 0.55 + 0.45 * np.abs(N.smooth_random(n, child(r, 'flutter'), 14.0, periodic=periodic))
    return normalize(x, 0.0)


@cue('Ember.GraveWard.Plant', variations=2, att='Creature', jitter=0.02, conc=1, level=-10.0)
def ward_plant(v, r):
    # Its iron foot set down on stone (a knock and its glass's tin rattle), the wick catching (a soft breath of flame
    # swelling), and a warm open chord rising in it: mercy.
    knock = layers((kit.thunk(240.0 * jitter(r, 1, 0.05), child(r, 'knock'), 0.07, 0.001, 6, 0.08), 0.0, 0.0),
                   (_link(child(r, 'iron'), 900.0, 3000.0), 0.0, -8.0))
    rattle = kit.jingle(0.12, child(r, 'glass'), 4, 2600.0, 6000.0, 0.03, 0.05, 0.01)
    flame = _flame_breath(0.9, child(r, 'catch')) * E.swell(0.9, 0.35, 0.6)
    chord = _chord([196.0, 293.7, 392.0, 587.3], 1.3, child(r, 'chord'), 0.35) * E.swell(1.3, 0.45, 0.9)
    x = layers((knock, 0.0, 0.0), (rattle, 0.01, -14.0), (normalize(flame), 0.08, -10.0), (normalize(chord), 0.15, -9.0))
    return _yard(_soft(normalize(x), 80.0), r, -16.0, 1.6)


@cue('Ember.GraveWard.Hum', variations=1, att='Near', jitter=0.0, conc=2, loop=True, level=-20.0, align=False)
def ward_hum(v, r):
    # The lantern burning: a small flame's soft flutter over a low warm hum (two near-sine tones beating slowly), built in a
    # circle so it loops without a seam.
    dur = 2.4
    n = ns(dur)
    flame = _flame_breath(dur, child(r, 'flame'), periodic=True)
    t = times(n)
    # Whole numbers of cycles in the loop, so the hum joins itself.
    f1, f2 = round(98.0 * dur) / dur, round(147.5 * dur) / dur
    beat = 0.8 + 0.2 * np.cos(2.0 * np.pi * (round(0.8 * dur) / dur) * t)
    hum = (np.sin(2.0 * np.pi * f1 * t) + 0.5 * np.sin(2.0 * np.pi * f2 * t) + 0.12 * np.sin(4.0 * np.pi * f1 * t)) * beat
    x = normalize(flame) * 0.7 + normalize(hum) * 0.5
    return F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(6000.0, -6.0), F.hp(50.0), circular=True)


@cue('Ember.GraveWard.Out', variations=2, att='Near', jitter=0.03, conc=1, level=-14.0)
def ward_out(v, r):
    # Snuffed: a puff of air through the flame, the glass's small tink, and the hum let go sinking.
    puff = kit.noise_thump(0.15, child(r, 'puff'), 900.0, 200.0, 0.06)
    hiss = kit.whoosh(0.35, child(r, 'hiss'), 1400.0, 500.0, 0.9, 0.15, -3.0)
    tink = kit.chime(1760.0 * jitter(r, 1, 0.02), child(r, 'tink'), 0.35, 0.5)
    n = ns(0.7)
    t = times(n)
    sink = np.sin(2.0 * np.pi * np.cumsum(98.0 * (1.0 - 0.25 * t / 0.7)) / SR) * E.perc(0.7, 0.01, 0.45)
    x = layers((puff, 0.0, 0.0), (hiss, 0.0, -8.0), (tink, 0.04, -10.0), (normalize(sink), 0.0, -9.0))
    return _yard(_soft(normalize(x), 50.0), r, -18.0, 1.0)


# --- Last Rites ----------------------------------------------------------------------------------------------------------

@cue('Ember.LastRites.Mark', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-9.0)
def rites_mark(v, r):
    # One low funeral bell, struck and let ring, and a whispered chord under it breathing in.
    bell = _bell([174.6, 164.8][v], child(r, 'bell'), 4.5, 1.0)
    chord = _chord([174.6, 207.7, 261.6], 1.8, child(r, 'chord'), 0.3, 0.3) * E.swell(1.8, 0.6, 1.0)
    breath = _breath(1.6, child(r, 'air')) * E.swell(1.6, 0.5, 0.9)
    x = layers((S.widen(bell, child(r, 'bw'), 0.25), 0.0, 0.0), (S.widen(normalize(chord), child(r, 'cw'), 0.4), 0.05, -9.0),
               (S.widen(normalize(breath), child(r, 'aw'), 0.5), 0.1, -18.0))
    return _cold(_soft(x, 50.0), r, -13.0, 2.6)


@cue('Ember.LastRites.Burst', variations=3, att='Gun', jitter=0.03, conc=2, level=-6.0)
def rites_burst(v, r):
    # Its soul bursting on its pack, all at once (it lands on the death): a soft heavy thump and a bright bell's crash,
    # souls rushing out in a wide hiss, and a choir's cry cut short in it.
    from recipes import creatures as C
    f = 330.0 * jitter(r, 1, 0.06)
    gasp = C._choir(child(r, 'gasp'), [(0.0, f * 0.8), (0.18, f), (0.3, f * 1.1)], 0.3, [(0.0, 'o'), (0.3, 'a')], 3, 22.0, 0.02,
                    0.75, 1.0, (5.0, 0.3), 0.25)
    gasp = normalize(gasp * E.ar(gasp.size / SR, 0.01, 0.2, 1.5))
    crash = _bell(392.0 * jitter(r, 1, 0.03), child(r, 'crash'), 2.2, 1.2)
    rush = kit.whoosh(0.9, child(r, 'rush'), 600.0, 3000.0, 0.8, 0.12, -1.0)
    thump = kit.noise_thump(0.35, child(r, 'thump'), 400.0, 60.0, 0.18)
    x = layers((thump, 0.0, 0.0), (crash, 0.0, -3.0), (rush, 0.0, -7.0), (gasp, 0.0, -8.0))
    return _yard(_soft(normalize(x), 50.0), r, -11.0, 1.8)


@cue('Ember.LastRites.Strike', variations=3, att='Gun', jitter=0.03, conc=2, level=-5.0)
def rites_strike(v, r):
    # The mark striking the one that lived: a crack of light (a sharp, dry electric snap), a heavy bell's blow on top of
    # it, and a deep boom rolling off over the land.
    n = ns(0.12)
    snap = N.white(n, child(r, 'snap')) * E.perc(0.12, 0.0003, 0.03) * (0.4 + 0.6 * np.abs(N.smooth_random(n, child(r, 'zap'), 400.0)))
    snap = F.filt(snap, F.hp(900.0), F.peak(3300.0, -6.0, 0.8), extend=False)
    blow = _bell(220.0 * jitter(r, 1, 0.03), child(r, 'blow'), 3.0, 1.2)
    boom = layers((kit.thump(62.0, 34.0, 1.2, 0.8), 0.0, 0.0), (kit.noise_thump(1.0, child(r, 'roll'), 300.0, 50.0, 0.7), 0.04, -4.0))
    x = layers((normalize(snap), 0.0, 0.0), (blow, 0.004, 0.0), (boom, 0.0, -6.0))
    x = D.drive(normalize(x), 3.0, 'tanh')
    return _yard(_soft(normalize(x), 30.0), r, -10.0, 2.0)
