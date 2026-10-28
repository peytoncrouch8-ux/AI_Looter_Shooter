"""The loot chest tiers' show (2026-10-28, the user's "escalating show": a bigger reveal means better loot; Loot/ChestShow.h,
LooterSoundCue::ChestTiers): tier 1-2's sprung latches, tier 3's ring lock spinning and opening, tier 4's orb pulsing and
its gold trim's shimmer, tier 5's crest unfolding, its burst of light and the short rising tone. The lid itself is
Loot.ChestOpen; tier 5's fountain pops are Boss.Loot.Pop. Physical first (sprung steel, a ratchet, a bolt), with the
glassy, golden layers growing with the tier, in the fanfare's key (E) so a tier 5's tone leads into a gun's landing sting.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, reverb as R
from recipes import cue
from recipes import kit
from recipes.loot import _outside, _ratchet, _glint


@cue('Loot.Chest.LatchPop', variations=4, att='Near', jitter=0.04, conc=4, level=-13.0)
def latch_pop(v, r):
    # A sprung steel latch flipped open: the catch letting go (a bright click), the latch's arm slapping against its stop
    # (a tinny clack on the case) and the spring's short buzz after it.
    catch = kit.metal_click(child(r, 'catch'), 2200.0, 7500.0, 0.025, 0.00006, 5, -10.0)
    slap = kit.clack(child(r, 'slap'), 900.0 * jitter(r, 1, 0.1), 1600.0, 6000.0, 0.05, 0.04, -4.0, -12.0)
    n = ns(0.12)
    buzz = O.sine(310.0 * jitter(r, 1, 0.06), n=n) * (1.0 + 0.6 * O.sine(55.0, n=n)) * E.perc(0.12, 0.002, 0.07)
    buzz = F.filt(buzz, F.hp(200.0), extend=False)
    t_slap = 0.018 + 0.008 * r.random()
    x = layers((catch, 0.0, -2.0), (slap, t_slap, 0.0), (normalize(buzz), t_slap + 0.004, -20.0))
    return _outside(x, r, -24.0)


@cue('Loot.Chest.LockSpin', variations=2, att='Near', jitter=0.02, conc=2, level=-12.0)
def lock_spin(v, r):
    # The ring lock spun: its detents ticking faster as it spins up, slowing to its stop (about 0.45 s, the show's spin),
    # a fine whirr under them and a soft electric hum as its light brightens.
    clicks = _ratchet(child(r, 'ratchet'), 14 + v * 2, 18.0, 48.0, 2600.0, 8000.0, 1100.0)
    dur = 0.5
    n = ns(dur)
    whirr = N.band(n, child(r, 'whirr'), 900.0, 3200.0) * E.ar(dur, 0.12, 0.25)
    hum = O.sine(kit.note('B3') * (1.0 + 0.04 * times(n) / dur), n=n) * E.ar(dur, 0.3, 0.15)
    x = layers((clicks, 0.0, 0.0), (normalize(whirr), 0.0, -24.0), (normalize(hum), 0.0, -22.0))
    return _outside(F.filt(x, F.hp(120.0), extend=False), r, -24.0)


@cue('Loot.Chest.LockOpen', variations=2, att='Near', jitter=0.0, conc=2, level=-11.0)
def lock_open(v, r):
    # The lock gives: its bolt drawn back with a heavy clunk, and a bright two-note "open" (B5 then E6) as its light
    # turns green.
    bolt = kit.clack(child(r, 'bolt'), 330.0, 900.0, 3800.0, 0.09, 0.12, 2.0, 0.0, 0.0002)
    c1 = _glint('B5', child(r, 'c1'), 0.35, 0.55, -9.0, 0.0002)
    c2 = _glint('E6', child(r, 'c2'), 0.6, 0.55, -9.0, 0.0002)
    x = layers((bolt, 0.0, 0.0), (c1, 0.05, -7.0), (c2, 0.12, -6.0))
    return _outside(F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False), r, -22.0)


@cue('Loot.Chest.OrbPulse', variations=2, att='Near', jitter=0.01, conc=2, level=-12.0)
def orb_pulse(v, r):
    # The violet orb waking: two swells of a glassy low hum (an E3 with its fifth and a bowed-glass shimmer an octave and
    # a half up), each rising and falling with the light's pulses (about 0.45 s for both).
    dur = 0.75
    n = ns(dur)
    t = times(n)
    pulses = np.sin(np.pi * np.clip(t / 0.46, 0.0, 1.0) * 2.0) ** 2 + 0.35 * np.clip((t - 0.4) / 0.1, 0.0, 1.0) * np.exp(-np.maximum(t - 0.5, 0.0) / 0.12)
    f = kit.note('E3')
    hum = O.sine(f, n=n) + 0.5 * O.sine(f * 1.5, n=n) + 0.25 * O.sine(f * 2.0 * (1.0 + 0.003 * np.sin(2 * np.pi * 5.0 * t)), n=n)
    glass = F.filt(N.pink(n, child(r, 'glass')), F.bp(kit.note('B5'), 6.0), extend=False)
    x = layers((normalize(hum * pulses), 0.0, 0.0), (normalize(glass * pulses), 0.0, -18.0))
    x = x * E.end_fade(np.ones(len(x)), 0.05)
    ir = R.plate(child(r, 'ir'), t60=0.8)
    wet = F.convolve(F.filt(x, F.hp(250.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, -14.0, 0.05))


@cue('Loot.Chest.TrimSweep', variations=2, att='Near', jitter=0.0, conc=2, level=-14.0)
def trim_sweep(v, r):
    # The gold trim lighting from end to end (about 0.45 s): a run of tiny brass taps climbing an E major arpeggio over a
    # rising breath of air, ending on a held glint.
    names = ['E6', 'G#6', 'B6', 'E7', 'G#7']
    run = np.zeros(1)
    for i, nm in enumerate(names):
        c = kit.chime(kit.note(nm), child(r, 'tap', i), 0.35 + 0.15 * (i == len(names) - 1), 0.4, 0.00012)
        run = layers(run, (c, 0.09 * i + 0.01 * r.random(), -1.5 * (len(names) - 1 - i)))
    air = kit.whoosh(0.5, child(r, 'air'), 1500.0, 9000.0, 0.8, 0.85, -2.0)
    x = layers((normalize(run), 0.0, 0.0), (air, 0.0, -18.0))
    return F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False)


@cue('Loot.Chest.CrestUnfold', variations=2, att='Near', jitter=0.02, conc=1, level=-11.0)
def crest_unfold(v, r):
    # The gold crest swinging out on its hinge: a slow ratchet of heavy detents and a hinge's groan, locking into place
    # with a deep clank and a ring of the crest's metal (about 0.6 s, the show's unfold).
    detents = _ratchet(child(r, 'ratchet'), 6, 14.0, 22.0, 1400.0, 4800.0, 520.0, stiffen=0.6)
    groan = kit.creak(0.45, child(r, 'groan'), 30.0, 55.0, (240.0, 560.0, 1200.0), 8.0, 0.3)
    t_lock = 0.52 + 0.04 * v
    lock = kit.clack(child(r, 'lock'), 260.0, 800.0, 3600.0, 0.12, 0.16, 2.0, 0.0, 0.0002)
    ring = _glint('E5', child(r, 'ring'), 0.8, 0.5, -12.0, 0.0003, -16.0)
    x = layers((detents, 0.0, -3.0), (groan, 0.02, -12.0), (lock, t_lock, 0.0), (ring, t_lock + 0.005, -12.0))
    return _outside(x, r, -22.0)


@cue('Loot.Chest.Burst', variations=1, att='Creature', jitter=0.0, conc=1, level=-8.0)
def chest_burst(v, r):
    # Tier 5's burst of light: a soft deep whoomp of air, a bright shower of glassy sparkles flying off it, and a struck
    # chord of small bells ringing out in a plate.
    whoomp = kit.noise_thump(0.45, child(r, 'whoomp'), 700.0, 60.0, 0.22, 1.0, 0.08)
    low = O.sine(kit.note('E2'), n=ns(0.6)) * E.perc(0.6, 0.004, 0.4)
    bells = np.zeros(1)
    for i, nm in enumerate(['E6', 'G#6', 'B6', 'E7']):
        bells = layers(bells, (_glint(nm, child(r, 'b', i), 1.1 - 0.15 * i, 0.7, -10.0, 0.00015), 0.006 * i, -1.5 * i))
    sp = np.zeros(1)
    sr = child(r, 'sparkle')
    for i in range(10):
        s = kit.metal_click(sr, 4500.0, 9500.0, 0.03, 0.00004, 3, -20.0)
        sp = layers(sp, (s, 0.02 + 0.5 * sr.random() ** 1.4, sr.uniform(-10.0, -2.0)))
    x = layers((whoomp, 0.0, 0.0), (normalize(low), 0.0, -6.0), (normalize(bells), 0.01, -5.0), (normalize(sp), 0.01, -16.0))
    ir = R.plate(child(r, 'ir'), t60=1.4)
    wet = F.convolve(F.filt(x, F.hp(300.0), extend=False), ir)
    x = layers(x, kit.set_level(wet, x, -13.0, 0.05))
    return F.filt(x, F.peak(3300.0, -3.0, 0.8), F.hp(30.0), extend=False)


@cue('Loot.Chest.Rise', variations=1, att='Creature', jitter=0.0, conc=1, level=-10.0, swell=True)
def chest_rise(v, r):
    # The short rising tone over the burst as the lid lifts (about 0.9 s): an open E chord of soft saws gliding up a fourth
    # (E to A, the lid going up), swelling and cut off clean, an airy shimmer climbing with it.
    dur = 0.9
    n = ns(dur)
    t = times(n)
    glide = 2.0 ** ((5.0 / 12.0) * np.clip(t / (dur * 0.85), 0.0, 1.0) ** 1.6)
    chord = np.zeros(n)
    for nm, db in (('E3', 0.0), ('B3', -3.0), ('E4', -4.0), ('G#4', -8.0)):
        f = kit.note(nm) * glide
        chord = chord + (10.0 ** (db / 20.0)) * (O.saw(f * 1.003, n=n) + O.saw(f * 0.997, n=n))
    chord = F.filt(chord, F.lp(1800.0, 0.7), F.hp(90.0), extend=False)
    shimmer = F.filt(N.pink(n, child(r, 'air')), F.bp(4000.0, 1.2), extend=False) * (t / dur) ** 2
    env = E.ar(dur, dur * 0.8, dur * 0.2)
    x = layers((normalize(chord * env), 0.0, 0.0), (normalize(shimmer * env), 0.0, -20.0))
    ir = R.plate(child(r, 'ir'), t60=1.0)
    wet = F.convolve(F.filt(x, F.hp(300.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, -12.0, 0.05))
