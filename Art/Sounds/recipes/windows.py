"""The lit windows of Ransom's Rest (3D): somebody crossing a room, a curtain drawn, each heard from the street through
the glass and the wall.

- World.Window.Steps: boots on a plank floor, five or six footfalls. A floor is a thing of joists and air under the
  boards, so a step is a dull knock of plank (soft contact, no hard tok), the hollow under it and a lighter toe a hair
  after; the wall lets only that low half out. Takes: crossing toward the window (louder step by step), walking away
  from it (softer), pacing (three steps, a pause and a scuff of the turn, three back). A board or two creaks as the
  weight comes onto it.
- World.Window.Curtain: a curtain pulled on its rod. Cloth swishing (noise in the cloth band broken into crinkles) over a
  heavier, lower swing of the fabric's mass, rings sliding on the rod (a scrape) and knocking each other in a train of
  ticks that quickens as the slack is taken up (closing) or slows as the curtain bunches (opening). Brass rings, wooden
  rings, iron. Heard behind a pane: the room's box, then glass takes the top off.

Both begin on the small thing that starts them (a footfall, the first ring's tick), so the check doesn't call them late.
"""
import numpy as np

from lib.core import layers, normalize, child
from lib import filters as F, reverb as R
from recipes import cue
from recipes import kit
from recipes.voices import _wall


# --- Steps ---------------------------------------------------------------------------------------------------------------

def _footfall(r, f0):
    """One step on a plank floor: the board's dull knock (a boot's soft contact is several milliseconds long), the hollow
    of the joists and the air under them, the heel's tok (mostly lost to the wall) and the toe rolling down a hair after."""
    board = kit.thunk(f0, child(r, 'board'), 0.09, 0.0013, 6, 0.08)
    knock = kit.thunk(f0 * 2.6, child(r, 'knock'), 0.05, 0.0008, 5, 0.1)
    hollow = kit.thump(f0 * 0.55, f0 * 0.4, 0.16, 0.09, attack=0.003)
    heel = kit.thunk(f0 * 3.0, child(r, 'heel'), 0.02, 0.0004, 4, 0.06)
    toe = kit.thunk(f0 * 1.15, child(r, 'toe'), 0.08, 0.002, 5, 0.08)
    return layers((board, 0.0, 0.0), (knock, 0.0, -3.0), (hollow, 0.0, -8.0), (heel, 0.0, -7.0),
                  (toe, r.uniform(0.07, 0.1), -9.0))


def _board_creak(r):
    """A plank complaining as the weight comes onto it."""
    return kit.creak(r.uniform(0.22, 0.34), r, r.uniform(14.0, 24.0), r.uniform(26.0, 40.0),
                     (r.uniform(180.0, 250.0), r.uniform(440.0, 600.0), r.uniform(880.0, 1250.0)), 8.0, 0.3)


def _walk(r, gaps, weights_db, creaks=(), scuff_after=None):
    """Footfalls one after another: gaps[i] is the time from step i to step i + 1, weights_db each step's level (the
    walker nearing or leaving the wall), creaks the steps that set a board creaking. scuff_after: a step after which the
    walker turns on the spot (a sole swivelling on the boards)."""
    out = np.zeros(1)
    t = 0.0
    for i, w in enumerate(weights_db):
        rs = child(r, 'step', i)
        out = layers(out, (_footfall(rs, rs.uniform(105.0, 170.0)), t, w))
        if i in creaks:
            out = layers(out, (_board_creak(child(rs, 'creak')), t + 0.03, w - 17.0))
        if scuff_after == i:
            turn = kit.cloth(0.14, child(rs, 'turn'), 250.0, 1500.0, 160.0, 0.3, 0.6)
            out = layers(out, (turn, t + gaps[i] * 0.45, w - 13.0))
        if i < len(gaps):
            t += gaps[i] * rs.uniform(0.93, 1.07)
    return out


@cue('World.Window.Steps', variations=3, att='Near', cls='Ambience', jitter=0.04, conc=2, level=-20.0)
def window_steps(v, r):
    # Somebody crossing the room behind a lit window, heard through the wall: coming to the window, step by step louder
    # (1), walking away from it, softer (2), pacing: three steps, a pause and the scuff of the turn, three back (3).
    if v == 0:
        x = _walk(r, [0.5] * 4, [-10.0, -8.0, -5.0, -3.0, 0.0], creaks=(3,))
    elif v == 1:
        x = _walk(r, [0.52] * 4, [0.0, -1.0, -4.0, -7.0, -10.0], creaks=(1,))
    else:
        x = _walk(r, [0.46, 0.46, 0.62, 0.48, 0.48], [-6.0, -3.0, 0.0, -1.0, -4.0, -7.0], creaks=(2, 4), scuff_after=2)
    return _wall(x, r, 800.0, -9.0, 0.3)


# --- The curtain ---------------------------------------------------------------------------------------------------------

# The rings' ticks lead and the cloth's swish is the bed under them (a swish alone is only noise): dB against the
# layers' own peaks, cloth being noise (crest ~12 dB) and a tick a spike.
RINGS_DB = 4.0
CLOTH_DB = -8.0

def _ring_click(r, kind):
    """One curtain ring knocking its neighbour or the rod's end: brass rings ring a little, wooden ones tick, iron ones
    clink."""
    if kind == 'brass':
        return kit.brass_tick(r, r.uniform(1500.0, 2200.0), 0.03, 3, 0.0001, -14.0, -12.0, 0.2)
    if kind == 'wood':
        return kit.thunk(r.uniform(1300.0, 2300.0), r, 0.014, 0.00015, 3, 0.1)
    return kit.metal_click(r, 1300.0, 4200.0, 0.03, 0.0001, 4, -12.0)


def _ring_train(r, kind, count, gap0, ratio, start=0.0):
    """Rings knocking in turn as the pull reaches each: the gaps shrink (ratio < 1, the slack taken up, closing) or grow
    (ratio > 1, the curtain bunching as it opens). Returns (signal, time of the last click)."""
    out = np.zeros(1)
    t = start
    gap = gap0
    for k in range(count):
        out = layers(out, (_ring_click(child(r, 'ring', k), kind), t, -r.uniform(0.0, 7.0)))
        t += gap * r.uniform(0.8, 1.2)
        gap *= ratio
    return out, t


def _glass(x, r, lp=2400.0, room_db=-11.0):
    """Behind a pane: the room's box around the sound, then the glass, which takes the top off and leaves a boxy hum."""
    x = normalize(x)
    ir = R.room(child(r, 'room'), t60=0.32, size=0.8)
    roomy = layers(x, kit.set_level(F.convolve(x, ir), x, room_db, 0.05))
    return F.filt(roomy, F.lp(lp, 0.6), F.lp(lp * 1.3, 0.6), F.hp(110.0), F.peak(520.0, 2.0, 1.2), extend=False)


@cue('World.Window.Curtain', variations=3, att='Near', cls='Ambience', jitter=0.05, conc=2, level=-19.0)
def window_curtain(v, r):
    # A curtain moved on its rod behind a lit window: drawn shut, brass rings running together and knocking at the end
    # (1), pulled open, the rings slowing as the cloth bunches (2), a quick twitch aside by wooden rings, a peek (3).
    kind = ['brass', 'brass', 'wood'][v]
    grab = kit.burst(0.02, child(r, 'grab'), 0.01, lo=200.0, hi=1500.0)
    parts = [(grab, 0.0, -8.0)]
    if v == 0:
        rings, end = _ring_train(child(r, 'rings'), kind, 9, 0.085, 0.82)
        parts += [(kit.cloth(0.95, child(r, 'cloth'), 300.0, 3200.0, 110.0, 0.3, 0.55), 0.0, CLOTH_DB),
                  (kit.whoosh(0.9, child(r, 'mass'), 420.0, 260.0, 0.5, 0.4, -4.0), 0.02, -14.0),
                  (kit.scrape(0.42, child(r, 'slide'), 1000.0, 1500.0, 10.0, 0.7), 0.02, -12.0),
                  (rings, 0.0, RINGS_DB)]
        # The rings come to rest bunched against the rod's end, and the curtain's edge brushes the frame.
        bunch, _ = _ring_train(child(r, 'bunch'), kind, 3, 0.016, 1.0, end)
        parts += [(bunch, 0.0, RINGS_DB - 1.0), (kit.noise_thump(0.08, child(r, 'edge'), 400.0, 120.0, 0.04), end, -12.0),
                  (kit.cloth(0.3, child(r, 'shiver'), 300.0, 2800.0, 130.0, 0.3, 0.6), end + 0.05, -16.0)]
    elif v == 1:
        rings, end = _ring_train(child(r, 'rings'), kind, 7, 0.04, 1.12)
        parts += [(kit.cloth(0.85, child(r, 'cloth'), 300.0, 3000.0, 100.0, 0.3, 0.6), 0.0, CLOTH_DB),
                  (kit.whoosh(0.8, child(r, 'mass'), 360.0, 240.0, 0.5, 0.35, -4.0), 0.02, -14.0),
                  (kit.scrape(0.4, child(r, 'slide'), 1100.0, 1700.0, 10.0, 0.7), 0.0, -12.0),
                  (rings, 0.0, RINGS_DB),
                  (kit.cloth(0.28, child(r, 'fold'), 300.0, 2600.0, 120.0, 0.3, 0.6), end, -13.0)]
    else:
        rings, end = _ring_train(child(r, 'rings'), kind, 5, 0.07, 0.9)
        tug, _ = _ring_train(child(r, 'tug'), kind, 2, 0.05, 1.0)
        parts += [(kit.cloth(0.6, child(r, 'cloth'), 300.0, 3100.0, 130.0, 0.3, 0.5), 0.0, CLOTH_DB),
                  (kit.whoosh(0.5, child(r, 'mass'), 400.0, 280.0, 0.5, 0.4, -4.0), 0.02, -15.0),
                  (kit.scrape(0.3, child(r, 'slide'), 1000.0, 1400.0, 10.0, 0.7), 0.0, -13.0),
                  (rings, 0.0, RINGS_DB), (tug, 0.52, RINGS_DB - 5.0),
                  (kit.cloth(0.22, child(r, 'settle'), 300.0, 2600.0, 130.0, 0.3, 0.6), 0.5, -12.0)]
    return _glass(layers(*parts), r, 2800.0)
