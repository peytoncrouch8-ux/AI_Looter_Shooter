"""The fight kit's sounds (Docs/Work/2026-10-28-Core/FIGHTS.md, lane 7; the director's events in Creatures/FightEvent.h):

- Creature.Slime.Split (3D, 0.7 s): a Restless slime dying into two slimelets: the gel bursting (a wet smack and a deep
  thump), a tearing suck as it comes apart in two, two plops at different pitches as the halves are thrown apart and land
  (one a little before the other), the gel wobbling and settling, bubbles.
- Fight.LeaderDown (2D sting, 1.2 s): the pack's head falls and the rest falter: a heavy, dull thump, the pack's cry
  cut and sinking (breathy voices falling), the iron bell choked on one cracked tone, and bones and grit scattering. Cold
  and final; not triumphant (the player has not won the fight yet).

The spiders keep their cues (the user's call 2026-10-28: leave them); the Spitter's flush lob has no cue of its own.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, modal as M, voice as V
from recipes import cue
from recipes import kit
from recipes import creatures as C
from recipes.bells import handbell, ring


@cue('Creature.Slime.Split', variations=4, att='Creature', jitter=0.06, conc=3, level=-8.0)
def slime_split(v, r):
    # A slime bursting into two: the skin giving with a wet smack and a deep thump, the gel torn apart (a thick suck, its
    # resonance climbing), two plops a few hundredths apart as the halves are flung off and land, the blobs wobbling and
    # settling, a scatter of bubbles. Each take a little different: which half lands first, how big the burst.
    size = (0.95, 1.05, 1.0, 1.12)[v] * jitter(r, 1.0, 0.04)
    smack = C._wet_slap(0.06, child(r, 'smack'), 220.0, 2600.0, 0.03)
    thump = kit.noise_thump(0.14, child(r, 'thump'), 560.0 / size, 70.0, 0.07, 1.0, 0.05)
    tear = C._stretch(0.16, child(r, 'tear'), 200.0 * size, 760.0)
    t1 = 0.12 + 0.02 * r.random()
    gap = 0.045 + 0.03 * r.random()
    plop1 = layers((C._bloop(170.0 * size * jitter(r, 1, 0.06), 340.0, 0.12, child(r, 'plop1'), 0.1), 0.0, 0.0),
                   (C._wet_slap(0.04, child(r, 'slap1'), 180.0, 1900.0, 0.025), 0.0, -4.0))
    plop2 = layers((C._bloop(250.0 * size * jitter(r, 1, 0.06), 470.0, 0.1, child(r, 'plop2'), 0.08), 0.0, 0.0),
                   (C._wet_slap(0.035, child(r, 'slap2'), 260.0, 2100.0, 0.02), 0.0, -4.0))
    land1 = layers((C._wet_slap(0.05, child(r, 'land1'), 150.0, 1700.0, 0.03), 0.0, 0.0),
                   (kit.noise_thump(0.08, child(r, 'lw1'), 420.0, 90.0, 0.04), 0.0, -4.0))
    land2 = C._wet_slap(0.045, child(r, 'land2'), 200.0, 2000.0, 0.025)
    wob1 = C._wobble(0.24, child(r, 'wob1'), 300.0 * size, 12.0 + 3.0 * r.random(), 0.16)
    wob2 = C._wobble(0.2, child(r, 'wob2'), 420.0 * size, 15.0 + 3.0 * r.random(), 0.13)
    bub = C._bubbles(0.18, child(r, 'bub'), 28.0, 350.0, 1200.0)
    x = layers((smack, 0.0, -4.0), (thump, 0.0, 0.0), (tear, 0.0, -3.0),
               (plop1, t1, 0.0), (plop2, t1 + gap, -2.0),
               (land1, t1 + 0.16, -6.0), (land2, t1 + gap + 0.2, -8.0),
               (wob1, t1 + 0.04, -11.0), (wob2, t1 + gap + 0.04, -13.0), (bub, t1 + 0.14, -13.0))
    return C._outside(F.filt(x, F.lp(3400.0, 0.7), F.hp(60.0), extend=False), r, -24.0, 0.06, 0.14, 0.3)


@cue('Fight.LeaderDown', variations=3, space='2D', jitter=0.02, conc=1, level=-9.0)
def leader_down(v, r):
    # The head of the pack falls: a heavy thump with a dull ring under it, the pack's cry sinking and cut off (breathy voices
    # falling away), a cracked iron bell choked on its first tone, bones and grit scattering. Cold, not a fanfare.
    dur = 0.9
    thump = kit.thump(120.0 * jitter(r, 1, 0.05), 40.0, 0.55, 0.4, drive_db=4.0)
    body = kit.noise_thump(0.3, child(r, 'body'), 380.0, 60.0, 0.15, 1.0, 0.1)
    cry = C._choir(child(r, 'cry'), [(0.0, 360.0 * jitter(r, 1, 0.05)), (dur * 0.5, 210.0), (dur, 110.0)], dur, [(0.0, 'a'), (dur * 0.5, 'o'), (dur, 'uh')],
                   count=4, detune=24.0, breath=0.6, shift=1.05, vib=(5.0, 0.25), rough=0.3)
    cry = F.filt(cry, F.hp(120.0), F.lp(2600.0), extend=False)
    cry = cry * E.ar(cry.size / SR, 0.04, cry.size / SR * 0.7, 2.0)
    m = handbell((250.0, 232.0, 270.0)[v], child(r, 'bell'), 1.6, 0.6, 0.8, crack=0.7)
    bell = normalize(ring(m, 1.1, child(r, 'stroke'), 0.0003, -20.0))
    bell = bell * E.perc(bell.size / SR, 0.002, 0.45)
    bones = kit.grit_burst(0.35, child(r, 'bones'), 600.0, 1400.0, 5200.0, 0.14)
    x = layers((normalize(thump), 0.0, 0.0), (normalize(body), 0.0, -6.0), (normalize(cry), 0.03, -7.0),
               (normalize(bell), 0.0, -10.0), (bones, 0.18, -17.0))
    x = F.filt(x, F.hp(35.0), F.peak(3200.0, -3.0, 0.8), extend=False)
    return C._haunt(x, r, -14.0, 1.1, 2800.0)
