"""The Unpaid's 2026-10-31 round (Source: Creatures/UnpaidCreatureRise.cpp, UnpaidCreatureGait.cpp, UnpaidCreatureOpening.cpp;
the cue names live in UnpaidLifeCue, Creatures/UnpaidLifeTuning.h). In the Unpaid's own voice (creatures.py: breathy choirs of
formant voices, cold ring modulation, the dark open-air wash):

- RiseRumble: the ground stirring over one of the buried, the tell a moment before its hand breaks it (0.7 s, played at the
  grave): a low shove under the earth, soil shifting and settling, pebbles ticking, the coal's heat crackling up through it,
  swelling to the end.
- RiseBurst: the dirt bursting as its hand claws up out of it: a heavy crack of packed earth, clods thrown and falling, dirt
  spraying, a gasp still under the ground. (Its haul and first breath reuse Creature.Unpaid.Rise, creatures.py.)
- NeckCrack: its neck cracking over as it hangs about: a short, dry crackle of old bone and a tiny knock (near only).
- Opening: the pack hearing a reload: a sharp, hungry hiss through teeth from the one that comes, ring-modulated cold.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, granular as G, voice as V
from recipes import cue
from recipes import kit
from recipes import creatures as C


@cue('Creature.Unpaid.RiseRumble', variations=3, att='Creature', jitter=0.05, conc=4, level=-11.0, swell=True)
def unpaid_rise_rumble(v, r):
    # Under the ground: a low shove that grows, soil shifting over it, pebbles ticking loose, the coal's heat crackling up
    # through the cracks at the end. It swells into the burst (0.7 s, Looter.Unpaid.Rise.Rumble).
    dur = 0.75
    n = ns(dur)
    shove = kit.noise_thump(dur, child(r, 'shove'), 120.0 * jitter(r, 1, 0.1), 38.0, 0.6, 1.2, dur)
    shove = shove[:n] * E.swell(dur, 0.92, 0.05, 1.6)[:shove[:n].size]
    soil = N.shaped(n, child(r, 'soil'), -6.0, 40.0, 900.0)
    soil *= 0.5 + 0.5 * np.abs(N.smooth_random(n, child(r, 'sg'), 7.0))
    soil = normalize(soil * E.swell(dur, 0.9, 0.06, 1.8))
    ticks = kit.grit_burst(dur, child(r, 'ticks'), 14.0 + 10.0 * v, 1800.0, 5200.0, None, 0.004)
    ticks = ticks[:n] * E.swell(dur, 0.85, 0.1, 1.2)[:ticks[:n].size]
    m = ns(0.45)
    heat = G.grit(0.45, child(r, 'heat'), 40.0 + 400.0 * (times(m) / 0.45) ** 2, 2000.0, 6500.0, 3.0, 3)
    heat = F.filt(heat, F.lp(6000.0), extend=False) * E.swell(0.45, 0.9, 0.05, 2.0)
    x = layers((normalize(shove), 0.0, 0.0), (soil, 0.0, -10.0), (normalize(ticks), 0.0, -14.0), (normalize(heat), dur - 0.45, -20.0))
    x = F.filt(x, F.hp(30.0), F.lp(5000.0), extend=False)
    return C._haunt(x, r, -18.0, 0.9, 2500.0)


@cue('Creature.Unpaid.RiseBurst', variations=3, att='Creature', jitter=0.04, conc=4, level=-7.0)
def unpaid_rise_burst(v, r):
    # Its hand breaks the ground: packed earth cracking open (a heavy, short thump with a split in it), clods thrown up and
    # falling back, dirt spraying and pattering, and a gasp still under the ground (muffled, low-passed).
    crack = kit.noise_thump(0.35, child(r, 'crack'), 700.0 * jitter(r, 1, 0.1), 55.0, 0.18, 1.0, 0.12)
    split = kit.grit_burst(0.18, child(r, 'split'), 5200.0, 400.0, 3000.0, 0.08, 0.002)
    clods = np.zeros(1)
    for i in range(5 + int(r.integers(0, 3))):
        clod = kit.noise_thump(0.08, child(r, 'clod', i), 1000.0, 160.0, 0.035, 1.0, 0.03)
        clods = layers(clods, (clod, r.uniform(0.08, 0.55), r.uniform(-12.0, -3.0)))
    spray = G.grit(0.6, child(r, 'spray'), 1800.0 * np.exp(-times(ns(0.6)) / 0.2), 900.0, 4500.0, 2.0, 4)
    gasp = V.whisper(0.32, [(0.0, 'h'), (0.12, 'a'), (0.32, 'h')], child(r, 'gasp'), 0.9) * E.perc(0.32, 0.02, 0.25)
    gasp = F.filt(normalize(gasp), F.lp(1400.0), F.hp(180.0), extend=False)
    x = layers((normalize(crack), 0.0, 0.0), (normalize(split), 0.0, -6.0), (normalize(clods), 0.0, -6.0),
               (normalize(spray), 0.03, -11.0), (normalize(gasp), 0.06, -9.0))
    x = F.filt(x, F.peak(3000.0, -3.0, 0.8), F.hp(35.0), extend=False)
    return C._haunt(x, r, -15.0, 1.1, 3000.0)


@cue('Creature.Unpaid.NeckCrack', variations=4, att='Creature', jitter=0.08, conc=2, level=-14.0)
def unpaid_neck_crack(v, r):
    # Old bone giving: a few dry crackles close together and a tiny knock under them, a breath out after.
    crackle = kit.grit_burst(0.12, child(r, 'crackle'), 260.0 + 60.0 * v, 2500.0, 7500.0, 0.04, 0.001)
    knock = kit.thunk(380.0 * jitter(r, 1, 0.12), child(r, 'knock'), 0.03, 0.0004, 5, 0.08)
    breath = V.whisper(0.2, [(0.0, 'h'), (0.2, 'u')], child(r, 'breath'), 0.95) * E.perc(0.2, 0.01, 0.15)
    x = layers((normalize(crackle), 0.0, 0.0), (normalize(knock), 0.01, -6.0), (normalize(breath), 0.06, -18.0))
    x = F.filt(x, F.hp(250.0), F.lp(9000.0), extend=False)
    return C._haunt(x, r, -22.0, 0.5, 4000.0)


@cue('Creature.Unpaid.Opening', variations=3, att='Creature', jitter=0.05, conc=2, level=-9.0)
def unpaid_opening(v, r):
    # It heard the click: a sharp, hungry hiss through bared teeth, rising, two breaths of it at once, ring-modulated so it
    # isn't quite a voice, with a crackle of its coal flaring under it.
    dur = 0.5
    n = ns(dur)
    hiss = V.hiss(n, child(r, 'hiss')) * E.ar(dur, 0.05, 0.3, 2.0)
    hiss = F.filt(hiss, F.hp(1500.0), F.peak(4200.0 * jitter(r, 1, 0.05), 4.0, 1.4), extend=False)
    keys = [[(0.0, 'h'), (0.2, 'i'), (dur, 'h')], [(0.0, 'h'), (0.25, 'e'), (dur, 'h')], [(0.0, 'h'), (0.2, 'ae'), (dur, 'h')]][v]
    breath = V.whisper(dur, keys, child(r, 'breath'), 1.05) * E.swell(dur, 0.35, 0.18, 1.5)
    breath = C._cold(normalize(breath), child(r, 'cold'), 37.0, 0.3)
    m = ns(0.3)
    flare = G.grit(0.3, child(r, 'flare'), 300.0 + 600.0 * times(m) / 0.3, 2000.0, 6500.0, 3.0, 3) * E.perc(0.3, 0.02, 0.2)
    x = layers((normalize(hiss), 0.0, 0.0), (normalize(breath), 0.02, -5.0), (normalize(flare), 0.04, -16.0))
    x = F.filt(x, F.peak(3200.0, -2.0, 0.8), F.hp(300.0), extend=False)
    return C._haunt(x, r, -14.0, 0.9, 3500.0)
