"""The Unpaid's varied moves and its failed-lunge shriek (2026-10-29; Source: Creatures/UnpaidCreatureShriek.cpp,
UnpaidCreatureVariety.cpp; cue names in Audio/LooterSoundCues.h, LooterSoundCue::UnpaidScream). In the Unpaid's own voice
(creatures.py: breathy choirs of formant voices, cold ring modulation, the dark open-air wash). Each Unpaid plays them at its
own voice's pitch (and a little more of its own), so a pack's shrieks never sound copied; five takes of the scream on top.

- ShriekDraw (0.5 s, the tell): a breath sucked in through the teeth, rising, a low moan climbing under it, the coal's
  crackle running faster; it ends as the scream begins.
- ShriekScream (1.25 s and its tail, 'Gun' attenuation: it carries): several torn voices snapping up a fifth apart, pushed
  into harmonic distortion (a lopsided drive and a little fold-over), a sub-octave growl under them, the whole thing shaken
  by a violent tremble (the head's shake, about 14 Hz), a rasp of air through it, then a long cold tail off the valley.
- ShriekPant (0.6 s, the recovery): three ragged breaths through an open mouth, a wet rattle in the throat.
- ShriekChoke (0.55 s, its coal shot in the tell): the scream's first breath strangled into a gag, the coal cracking with a
  hollow knock, sputtering and sizzling.
- Player.Rattled (2D, 1 s, the player hit by the scream): a cold ringing in the ears, high and beating, over a dull thump
  of pressure, frost crackling at the edges; quiet (it's a feeling, not an event).
- Feint (0.35 s): its shroud snapping as it slides aside and a hissed breath, half a laugh.
- PhaseStrike (0.5 s): back at the player's side, a whisper close by rising to a hiss (the claw's own swish is the rake's).
- LowSwipe (0.65 s): claws dragging through dirt as it sinks low, a growl under it, the skim's low rush at its end.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, dist as D, granular as G, voice as V, stereo as S
from recipes import cue
from recipes import kit
from recipes import creatures as C


def _tremble(n, rate, depth, r, start=0.0, ramp=0.05):
    """A violent amplitude tremble (the head's shake in the scream): two uneven rates, eased in from start."""
    t = times(n)
    phase = 2 * np.pi * r.random()
    wob = 0.5 + 0.5 * (0.7 * np.sin(2 * np.pi * rate * t + phase) + 0.3 * np.sin(2 * np.pi * rate * 1.55 * t + 2.3 * phase))
    on = np.clip((t - start) / max(ramp, 1e-3), 0.0, 1.0)
    return 1.0 - depth * on * wob


@cue('Creature.Unpaid.ShriekDraw', variations=4, att='Creature', jitter=0.04, conc=3, level=-8.0, swell=True)
def unpaid_shriek_draw(v, r):
    # The tell: a breath dragged in through its teeth, swelling and rising; a moan climbing under it; its coal crackling
    # faster and faster. It lands on the scream (0.5 s, Looter.Unpaid.Shriek.Tell).
    dur = 0.52
    n = ns(dur)
    draw = V.whisper(dur, [(0.0, 'h'), (0.2, 'i'), (dur, 'i')], child(r, 'draw'), 1.1 * jitter(r, 1, 0.04))
    draw = F.sweep(draw, 'bp', np.clip(O.glide([(0.0, 900.0), (dur, 3600.0)], n=draw.size), 60.0, 16000.0), 0.9) * E.swell(draw.size / SR, 0.95, 0.05, 2.2)
    f = [150.0, 138.0, 162.0, 145.0][v] * jitter(r, 1, 0.03)
    moan = C._choir(child(r, 'moan'), [(0.0, f), (dur, f * 1.7)], dur, [(0.0, 'u'), (dur * 0.6, 'o'), (dur, 'a')], 3, 18.0, 0.02, 0.6, 1.0,
                    (5.0, 0.3), 0.3)
    moan = C._cold(moan * E.swell(moan.size / SR, 0.9, 0.08, 2.0), child(r, 'cold'), 31.0, 0.25)
    rate = 40.0 + 1100.0 * (times(n) / dur) ** 2
    crackle = G.grit(dur, child(r, 'crackle'), rate, 1500.0, 6000.0, 2.5, 3)
    crackle = F.filt(crackle, F.lp(6500.0), extend=False) * E.swell(dur, 0.92, 0.05, 2.0)
    suck = kit.whoosh(dur, child(r, 'suck'), 260.0, 1800.0, 0.8, 0.95, -4.0, 1.4)
    x = layers((normalize(draw), 0.0, 0.0), (normalize(moan), 0.0, -7.0), (normalize(crackle), 0.0, -9.0), (suck, 0.0, -10.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(90.0), extend=False)
    return C._haunt(x, r, -14.0, 0.9)


@cue('Creature.Unpaid.ShriekScream', variations=5, att='Gun', jitter=0.03, conc=3, level=-2.0)
def unpaid_shriek_scream(v, r):
    # The scream: torn voices snapping up together, held and wavering, driven into a lopsided distortion; a sub-octave growl
    # under them; a violent tremble through it all (its head's shake); a rasp of air; a long cold tail off the valley.
    f = [560.0, 610.0, 520.0, 655.0, 585.0][v] * jitter(r, 1, 0.025)
    dur = 1.25
    n = ns(dur)
    voices = np.zeros(ns(dur + 0.1))
    for i, ratio in enumerate([1.0, 1.5, 0.75]):
        g = f * ratio
        scream = C._choir(child(r, 'voice', i), [(0.0, g * 0.78), (0.07, g * 1.55), (0.45, g * 1.45), (0.95, g * 1.36), (dur, g * 0.95)], dur,
                          [(0.0, 'a'), (0.15, 'ae'), (0.6, 'e'), (1.0, 'i'), (dur, 'a')], 3, 32.0, 0.012, 0.35, 1.25, (7.5, 0.6), 0.5, 4.0)
        k = ns(0.012 * i + 0.01 * r.random())
        voices[k:k + scream.size] += normalize(scream) * (1.0 if i == 0 else 0.6)
    # Harmonic distortion: a lopsided tanh (even harmonics, a strained throat) with a little fold-over buzz mixed in.
    torn = D.drive(normalize(voices), 16.0, 'tanh', bias=0.22)
    torn = D.drive(normalize(torn), 6.0, 'fold', mix=0.25)
    growl = C._choir(child(r, 'growl'), [(0.0, f * 0.42), (0.1, f * 0.55), (dur, f * 0.46)], dur, [(0.0, 'uh'), (0.5, 'o'), (dur, 'uh')], 2, 20.0, 0.01,
                     0.5, 0.9, (5.0, 0.4), 0.85)
    growl = D.drive(normalize(growl), 12.0, 'tanh', bias=0.15)
    rasp = N.white(n, child(r, 'rasp')) * kit.turbulence(n, child(r, 'rt'), 170.0, 0.8)
    rasp = F.sweep(rasp, 'bp', np.clip(O.glide([(0.0, f * 2.2), (0.08, f * 4.6), (dur, f * 3.1)], n=n), 60.0, 16000.0), 2.4)
    frost = F.filt(N.pink(n, child(r, 'frost'), hi=12000.0), F.hp(4200.0), extend=False) * E.swell(dur, 0.3, 1.0)
    x = layers((normalize(torn), 0.0, 0.0), (normalize(growl), 0.0, -8.0), (normalize(rasp), 0.0, -10.0), (normalize(frost), 0.04, -24.0))
    # The head's violent shake, building in as it thrusts forward; the cold ring through it; snapped on, held, let go.
    x = x * _tremble(x.size, 14.3, 0.38, child(r, 'shake'), 0.06, 0.12)
    x = C._cold(x, child(r, 'cold'), 88.0, 0.28)
    x = x * E.perc(x.size / SR, 0.012, 0.7, 0.55)
    x = F.filt(x, F.peak(3300.0, -3.5, 0.7), F.hp(160.0), extend=False)
    # A slap off a far wall of the valley, then the long dark wash.
    x = C._outside(x, child(r, 'far'), -17.0, 0.21, 0.46, 1.1)
    return C._haunt(x, r, -6.0, 2.8)


@cue('Creature.Unpaid.ShriekPant', variations=4, att='Creature', jitter=0.05, conc=3, level=-12.0)
def unpaid_shriek_pant(v, r):
    # Hunched after the scream: three ragged breaths through an open mouth, each with a wet rattle in its throat.
    dur = 0.62
    out = np.zeros(ns(dur + 0.1))
    starts = [0.0, 0.2 + 0.02 * r.random(), 0.41 + 0.03 * r.random()]
    for i, at in enumerate(starts):
        out_breath = i % 2 == 0
        keys = [(0.0, 'h'), (0.08, 'a' if out_breath else 'uh'), (0.16, 'h')]
        b = V.whisper(0.16, keys, child(r, 'breath', i), 1.0 + 0.05 * (i - 1)) * E.perc(0.16, 0.02, 0.13)
        rattle = G.grit(0.12, child(r, 'rattle', i), 90.0, 300.0, 1400.0, 2.0, 2) * E.perc(0.12, 0.01, 0.09)
        k = ns(at)
        piece = layers((normalize(b), 0.0, 0.0), (normalize(rattle), 0.01, -14.0))
        out[k:k + piece.size] += piece * (1.0 - 0.15 * i)
    x = F.filt(out, F.peak(3200.0, -3.0, 0.8), F.hp(140.0), extend=False)
    return C._haunt(x, r, -16.0, 0.7)


@cue('Creature.Unpaid.ShriekChoke', variations=4, att='Creature', jitter=0.05, conc=3, level=-7.0)
def unpaid_shriek_choke(v, r):
    # Its coal shot as it rears up: the scream's first breath strangled into a gag (the voice chopped as the throat seizes),
    # the coal cracking with a hollow knock, sputtering and sizzling as it dims.
    f = [520.0, 480.0, 570.0, 500.0][v] * jitter(r, 1, 0.04)
    cut = C._choir(child(r, 'cut'), [(0.0, f * 1.2), (0.12, f * 0.7)], 0.14, [(0.0, 'ae'), (0.14, 'uh')], 2, 26.0, 0.01, 0.4, 1.2, (6.0, 0.4), 0.6)
    cut = D.drive(normalize(cut), 10.0, 'tanh', bias=0.2) * E.perc(cut.size / SR, 0.005, 0.12)
    gag_n = ns(0.24)
    gag = V.voice(O.glide([(0.0, f * 0.45), (0.24, f * 0.3)], n=gag_n), [(0.0, 'uh'), (0.24, 'er')], child(r, 'gag'), breath=0.6, rough=0.9)
    chop = (np.sin(2 * np.pi * 26.0 * times(gag_n) + 2 * np.pi * r.random()) > -0.2).astype(float)
    gag = F.filt(normalize(gag) * chop, F.lp(2600.0), extend=False) * E.perc(0.24, 0.01, 0.2)
    crack = kit.grit_burst(0.07, child(r, 'crack'), 5000.0, 2200.0, 8000.0, 0.04)
    knock = C._hollow(child(r, 'knock'), 210.0 * jitter(r, 1, 0.06), 0.1)
    thump = kit.noise_thump(0.14, child(r, 'thump'), 900.0, 110.0, 0.07)
    sizzle = F.filt(N.pink(ns(0.45), child(r, 'sizzle'), hi=11000.0), F.hp(2800.0), extend=False) * E.perc(0.45, 0.01, 0.38)
    sizzle = sizzle * (0.4 + 0.6 * np.abs(N.smooth_random(sizzle.size, child(r, 'spit'), 60.0)))
    sputter = G.grit(0.4, child(r, 'sputter'), 160.0, 1400.0, 5200.0, 2.5, 3) * E.perc(0.4, 0.005, 0.32)
    x = layers((normalize(cut), 0.0, 0.0), (normalize(gag), 0.1, -3.0), (crack, 0.0, -6.0), (knock, 0.0, -5.0), (thump, 0.0, -6.0),
               (normalize(sizzle), 0.03, -16.0), (normalize(sputter), 0.04, -12.0))
    x = F.filt(x, F.peak(3200.0, -3.5, 0.8), F.hp(80.0), extend=False)
    return C._haunt(x, r, -14.0, 1.0)


@cue('Player.Rattled', variations=3, space='2D', jitter=0.03, conc=1, level=-15.0)
def player_rattled(v, r):
    # The player hit by the scream: a cold ringing in the ears (two high tones beating, falling away), a dull thump of
    # pressure, frost crackling at the edges of hearing. Quiet: a feeling more than an event.
    dur = 1.0
    n = ns(dur)
    t = times(n)
    f = [6800.0, 7300.0, 6400.0][v] * jitter(r, 1, 0.01)
    ring = np.sin(2 * np.pi * f * t) + 0.8 * np.sin(2 * np.pi * (f + 3.5 + 2.0 * r.random()) * t + 2 * np.pi * r.random())
    ring = ring * E.perc(dur, 0.04, 0.9)
    thump = kit.thump(78.0 * jitter(r, 1, 0.05), 34.0, 0.4, 0.3)
    muffle = F.filt(N.pink(n, child(r, 'muffle')), F.lp(500.0), extend=False) * E.perc(dur, 0.01, 0.35)
    frost = G.grit(0.7, child(r, 'frost'), 140.0, 5000.0, 11000.0, 2.0, 3) * E.perc(0.7, 0.05, 0.55)
    x = layers((normalize(ring), 0.03, -10.0), (thump, 0.0, 0.0), (normalize(muffle), 0.0, -8.0), (normalize(frost), 0.06, -16.0))
    x = F.filt(x, F.hp(30.0), extend=False)
    return S.widen(x, child(r, 'wide'), 0.4)


@cue('Creature.Unpaid.Feint', variations=4, att='Creature', jitter=0.06, conc=3, level=-11.0)
def unpaid_feint(v, r):
    # Stopped short, sliding aside: its shroud snapping with the turn, a hissed breath with half a laugh in it, the air
    # moving past.
    dur = 0.36
    snap = kit.cloth(0.18, child(r, 'snap'), 400.0, 4200.0, 240.0, 0.08, 0.7, -2.0)
    breath = V.whisper(0.3, [(0.0, 'h'), (0.1, 'e'), (0.3, 'h')], child(r, 'hiss'), 1.1 * jitter(r, 1, 0.05)) * E.perc(0.3, 0.02, 0.26)
    breath = breath * _tremble(breath.size, 9.0, 0.5, child(r, 'laugh'), 0.04, 0.03)
    air = kit.whoosh(dur, child(r, 'air'), 1400.0, 500.0, 1.0, 0.3, -3.0, 1.2)
    x = layers((normalize(snap), 0.0, 0.0), (normalize(breath), 0.03, -5.0), (air, 0.0, -9.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(180.0), extend=False)
    return C._haunt(x, r, -16.0, 0.8)


@cue('Creature.Unpaid.PhaseStrike', variations=3, att='Creature', jitter=0.05, conc=3, level=-9.0, swell=True)
def unpaid_phase_strike(v, r):
    # Back at the player's side, close: a whisper rising to a hiss as its claw comes up (the strike lands 0.4 s on;
    # its own swish is the rake's cue), ring-modulated so it isn't quite a voice.
    dur = 0.45
    words = [[(0.0, 'h'), (0.15, 'i'), (dur, 'h')], [(0.0, 'h'), (0.18, 'e'), (dur, 'h')], [(0.0, 'h'), (0.12, 'u'), (0.3, 'i'), (dur, 'h')]][v]
    whisper = np.zeros(ns(dur + 0.04))
    for i in range(2):
        w = V.whisper(dur, words, child(r, 'w', i), 1.05 + 0.07 * i) * E.swell(dur, 0.85, 0.06, 1.8)
        k = ns(0.012 * i)
        whisper[k:k + w.size] += normalize(w) * (1.0 if i == 0 else 0.6)
    whisper = C._cold(whisper, child(r, 'cold'), 43.0, 0.35)
    hiss = V.hiss(ns(dur), child(r, 'hiss')) * E.swell(dur, 0.9, 0.04, 2.5)
    x = layers((normalize(whisper), 0.0, 0.0), (normalize(hiss), 0.0, -12.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(240.0), extend=False)
    return C._haunt(x, r, -10.0, 1.2)


@cue('Creature.Unpaid.LowSwipe', variations=4, att='Creature', jitter=0.05, conc=3, level=-9.0, swell=True)
def unpaid_low_swipe(v, r):
    # Sinking low: claws dragging through the dirt, a growl under it; then the low rush of the skim along the ground.
    dur = 0.66
    n = ns(0.5)
    drag = G.grit(0.5, child(r, 'drag'), 420.0, 700.0, 3200.0, 2.0, 4) * E.swell(0.5, 0.4, 0.3, 1.5)
    soil = F.filt(N.pink(n, child(r, 'soil')), F.lp(1100.0), F.hp(120.0), extend=False) * kit.turbulence(n, child(r, 'st'), 90.0, 0.6)
    soil = soil * E.swell(0.5, 0.45, 0.3, 1.5)
    f = [112.0, 104.0, 120.0, 98.0][v] * jitter(r, 1, 0.04)
    growl = C._choir(child(r, 'growl'), [(0.0, f), (0.5, f * 1.15)], 0.5, [(0.0, 'uh'), (0.5, 'o')], 2, 16.0, 0.02, 0.55, 0.9, (4.5, 0.3), 0.8)
    growl = C._cold(growl * E.swell(growl.size / SR, 0.45, 0.25, 1.6), child(r, 'cold'), 27.0, 0.2)
    rush = kit.whoosh(0.24, child(r, 'rush'), 220.0, 900.0, 0.9, 0.55, -4.0, 1.2)
    x = layers((normalize(drag), 0.0, 0.0), (normalize(soil), 0.0, -4.0), (normalize(growl), 0.02, -9.0), (rush, dur - 0.24, -3.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(70.0), extend=False)
    return C._haunt(x, r, -14.0, 1.0)
