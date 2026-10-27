"""The Unpaid's fighting kit (round 2, 2026-10-27; Source: Creatures/UnpaidCreatureMoves.cpp, UnpaidKitCue in
Creatures/UnpaidFightTuning.h). Each cue is a tell the player hears and answers, in the Unpaid's own voice (creatures.py:
breathy choirs of formant voices, cold ring modulation, the dark open-air wash):

- Rake: one claw swipe: a short, fast cut of air, a dry rattle of old knuckle bones, a hissed breath out.
- Cinder: the coal swelling at its chest before it throws: a crackle rising, a breath drawn in, a low ember hum.
- Flank: a cold whisper heard from where it will come back (played there), so the player knows which side.
- Rally: the pack's scream: several voices tearing upward together, far-carrying ('Gun' attenuation, ~60 m).
- Howl: the first sight of the player: a long, low-to-high howl that carries across the valley ('Gun').
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, dist as D, granular as G, voice as V
from recipes import cue
from recipes import kit
from recipes import creatures as C


@cue('Creature.Unpaid.Rake', variations=4, att='Creature', jitter=0.06, conc=4, level=-9.0)
def unpaid_rake(v, r):
    # A claw cutting the air fast, a dry rattle of finger bones at the end of the sweep, a short hiss of cold breath.
    dur = 0.3
    swish = kit.whoosh(dur, child(r, 'swish'), 900.0 * jitter(r, 1, 0.1), 4200.0 * jitter(r, 1, 0.1), 1.3, 0.35, -2.0, 1.4)
    rattle = kit.grit_burst(0.09, child(r, 'rattle'), 900.0, 1800.0, 5200.0, 0.05)
    hiss = V.whisper(0.22, [(0.0, 'h'), (0.1, 'a'), (0.22, 'h')], child(r, 'hiss'), 1.1) * E.perc(0.22, 0.01, 0.18)
    x = layers((swish, 0.0, 0.0), (rattle, 0.13, -9.0), (normalize(hiss), 0.02, -12.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(150.0), extend=False)
    return C._haunt(x, r, -16.0, 0.9, 3000.0)


@cue('Creature.Unpaid.Cinder', variations=3, att='Creature', jitter=0.04, conc=3, level=-10.0, swell=True)
def unpaid_cinder(v, r):
    # Its coal swelling to throw: embers crackling faster and faster, a breath drawn in through the teeth, a low hum of
    # heat rising under them; it ends as the cinder leaves its hand (0.7 s, Looter.Unpaid.CinderTell).
    dur = 0.75
    n = ns(dur)
    rate = 60.0 + 900.0 * (times(n) / dur) ** 2
    crackle = G.grit(dur, child(r, 'crackle'), rate, 1600.0, 6000.0, 2.5, 3)
    crackle = F.filt(crackle, F.lp(6500.0), extend=False) * E.swell(dur, 0.7, 0.08, 2.0)
    draw = V.whisper(0.6, [(0.0, 'h'), (0.35, 'i'), (0.6, 'h')], child(r, 'draw'), 1.05) * E.swell(0.6, 0.55, 0.1, 2.0)
    f = 85.0 * jitter(r, 1, 0.05)
    t = times(n)
    hum = np.sin(2 * np.pi * O.glide([(0.0, f), (dur, f * 1.6)], n=n).cumsum() / SR) + 0.3 * np.sin(2 * np.pi * f * 3.02 * t)
    hum = C._cold(hum * E.swell(dur, 0.68, 0.06, 1.6), child(r, 'cold'), 29.0, 0.25)
    x = layers((normalize(crackle), 0.0, -4.0), (normalize(draw), 0.05, -6.0), (normalize(hum), 0.0, -9.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(60.0), extend=False)
    return C._haunt(x, r, -14.0, 1.0)


@cue('Creature.Unpaid.Flank', variations=3, att='Creature', jitter=0.05, conc=3, level=-9.0, swell=True)
def unpaid_flank(v, r):
    # Where it will come back: a cold whisper close by, several breaths at once, rising, ring-modulated so it isn't quite
    # a voice, and a puff of grave dust at its end.
    dur = 0.6
    words = [[(0.0, 'h'), (0.2, 'e'), (0.45, 'i'), (dur, 'h')], [(0.0, 'h'), (0.25, 'u'), (0.45, 'a'), (dur, 'h')],
             [(0.0, 'h'), (0.2, 'i'), (0.4, 'er'), (dur, 'h')]][v]
    whisper = np.zeros(ns(dur + 0.05))
    for i in range(3):
        w = V.whisper(dur, words, child(r, 'w', i), 1.0 + 0.06 * (i - 1)) * E.swell(dur, 0.45, 0.2, 1.8)
        k = ns(0.015 * i)
        whisper[k:k + w.size] += normalize(w) * (1.0 if i == 0 else 0.6)
    whisper = C._cold(whisper, child(r, 'cold'), 41.0, 0.35)
    dust = kit.noise_thump(0.25, child(r, 'dust'), 900.0, 200.0, 0.15, 0.9, 0.1)
    x = layers((normalize(whisper), 0.0, 0.0), (dust, dur - 0.08, -14.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(220.0), extend=False)
    return C._haunt(x, r, -8.0, 1.6)


@cue('Creature.Unpaid.Rally', variations=3, att='Gun', jitter=0.03, conc=2, level=-3.0)
def unpaid_rally(v, r):
    # The pack screamed on: four torn voices snapping up together a fifth apart, held, wavering, with a cold buzz in them,
    # and the breath of the rest under it (0.8 s before they come, Looter.Unpaid.RallyTell; it rings on as they do).
    f = [520.0, 480.0, 560.0][v] * jitter(r, 1, 0.03)
    dur = 1.3
    out = np.zeros(ns(dur + 0.2))
    for i, ratio in enumerate([1.0, 1.5, 0.75, 1.26]):
        g = f * ratio
        scream = C._choir(child(r, 'scream', i), [(0.0, g * 0.7), (0.12, g * 1.3), (0.7, g * 1.22), (dur, g * 0.9)], dur,
                          [(0.0, 'a'), (0.2, 'ae'), (0.8, 'e'), (dur, 'a')], 2, 28.0, 0.015, 0.4, 1.2, (7.0, 0.5), 0.4, 3.0)
        k = ns(0.03 * i + 0.02 * r.random())
        out[k:k + scream.size] += normalize(scream) * (1.0 if i == 0 else 0.7)
    out = D.drive(normalize(out), 10.0, 'tanh', bias=0.1)
    out = C._cold(out, child(r, 'cold'), 70.0, 0.25) * E.perc(out.size / SR, 0.02, 1.1)
    breath = V.whisper(dur, [(0.0, 'h'), (0.6, 'a'), (dur, 'h')], child(r, 'breath'), 1.0) * E.swell(dur, 0.3, 0.9)
    x = layers((normalize(out), 0.0, 0.0), (normalize(breath), 0.0, -12.0))
    x = F.filt(x, F.peak(3300.0, -3.5, 0.7), F.hp(180.0), extend=False)
    return C._haunt(x, r, -8.0, 2.4)


@cue('Creature.Unpaid.Howl', variations=3, att='Gun', jitter=0.03, conc=2, level=-5.0, swell=True)
def unpaid_howl(v, r):
    # It has seen you, and every dead thing near knows: a long howl of many voices, low and rising slowly a sixth, held,
    # then falling away into the dark, carried far over the valley.
    f = [180.0, 165.0, 196.0][v] * jitter(r, 1, 0.02)
    dur = 2.2
    howl = C._choir(child(r, 'howl'), [(0.0, f * 0.85), (0.6, f * 1.45), (1.3, f * 1.5), (dur, f * 0.8)], dur,
                    [(0.0, 'u'), (0.6, 'o'), (1.2, 'a'), (dur, 'u')], 4, 20.0, 0.06, 0.5, 1.0, (4.5, 0.4), 0.15)
    howl = C._cold(howl, child(r, 'cold'), 31.0, 0.2) * E.swell(howl.size / SR, 0.7, 1.6, 1.8)
    air = V.whisper(dur, [(0.0, 'h'), (1.0, 'o'), (dur, 'h')], child(r, 'air'), 0.95) * E.swell(dur, 0.8, 1.3)
    x = layers((howl, 0.0, 0.0), (normalize(air), 0.1, -14.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(90.0), extend=False)
    return C._haunt(x, r, -6.0, 3.0, 3000.0)
