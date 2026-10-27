"""The game's opening (2026-10-27, round 3, OPENING): the startup cards' stings and the main menu's arrival and
presses. All 2D. The stings speak the score's language (band.py: plucked steel strings, bowed strings, the chapel's
bell, small brass chimes) in D, so the menu's first notes and the score agree; the menu's sounds are the interface's
brass and wood (ui.py) with a little more weight, since the main menu is pressed rarely and should feel deliberate.

- UI.Startup.Studio (2D, ~3 s): the studio card fading in. A plucked low D and its fifth on a steel string, a soft
  bowed open fifth swelling under them, a small brass chime ringing the octave on top: a lantern lit in the dark.
- UI.Startup.Engine (2D, ~2.6 s): the engine card. Cooler and cleaner: a soft deep drum, a quick roll of brass chimes
  up a D major ninth, a breath of air rising through them, a low bowed D holding.
- UI.Menu.Reveal (2D, ~2 s, swells in): the menu arriving over the planet. Air drawn up past the ear, a low bowed open
  fifth, the chapel bell struck softly at the swell's top with a chime over it.
- UI.Menu.Select (2D, 2 takes): a main menu button pressed. The interface's brass switch, a wooden knock under it with
  its weight, and a short bright chime (D6 or E6) left ringing a moment: heavier than UI.Click.

Startup timing (UI/Menus/StartupLogosRules.h): a card fades in over 0.6 s and holds 2.2 s, so the stings' bodies sit in
the first 2.5 s and their tails die by the fade-out.
"""
from lib.core import layers, normalize, child, jitter, fit
from lib import env as E, filters as F
from recipes import cue
from recipes import kit
from recipes import band as B
from recipes.ui import _ui


def _hall(r, t60=2.2):
    return B.hall(child(r, 'hall'), t60=t60)


def _bus(x, circular):
    return B.master_bus(x, circular, glue=False)


@cue('UI.Startup.Studio', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-13.0, align=False)
def startup_studio(v, r):
    # A lantern lit in the dark: the pluck speaks first (a physical touch at 0, so the check never calls it late), the
    # fifth answers, the bowed fifth swells under them and the chime rings the octave on top as the card settles.
    desk = B.Desk(None, tail=3.0)
    desk.track('pluck', 0.0, -0.1, -12.0, B.guitar_body)
    desk.track('low', -7.0, 0.0, -9.0, B.cello_body)
    desk.track('chime', -9.0, 0.15, -6.0)
    desk.add('pluck', kit.twang(kit.note('D3'), 3.0, child(r, 'p1'), 0.55, 2.4, 0.18), 0.0)
    desk.add('pluck', kit.twang(kit.note('A3'), 2.6, child(r, 'p2'), 0.5, 2.0, 0.2), 0.32, -3.0, 0.1)
    for j, note in enumerate(['D2', 'A2']):
        desk.add('low', B.bowed(note, 2.8, child(r, 'l', j), 0.8, attack=0.7, release=1.4, vib=0.05, bright=0.4), 0.05)
    desk.add('chime', kit.chime(kit.note('D6'), child(r, 'c1'), 1.8, 0.7), 0.62)
    desk.add('chime', kit.chime(kit.note('A5'), child(r, 'c2'), 1.6, 0.6), 0.66, -6.0, -0.2)
    return desk.mix(_hall(r, 2.4), _bus)


@cue('UI.Startup.Engine', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-14.0, align=False)
def startup_engine(v, r):
    # Cooler and cleaner than the studio's: a soft deep drum under a quick roll of chimes up a D major ninth, a breath of
    # air rising through them, a low bowed D holding the floor.
    desk = B.Desk(None, tail=3.0)
    desk.track('drum', -4.0, 0.0, -14.0)
    desk.track('chime', 0.0, 0.0, -7.0)
    desk.track('air', -12.0, 0.0, -10.0)
    desk.track('low', -8.0, 0.0, -10.0, B.cello_body)
    desk.add('drum', B.big_drum(child(r, 'drum'), 0.55, f0=46.0, t60=1.1), 0.0)
    for j, note in enumerate(['D5', 'F#5', 'A5', 'E6']):
        desk.add('chime', kit.chime(kit.note(note), child(r, 'c', j), 1.7 - 0.2 * j, 0.75), 0.02 + 0.075 * j,
                 -1.5 * j, -0.35 + 0.23 * j)
    desk.add('air', kit.whoosh(1.1, child(r, 'air'), 500.0, 5200.0, 0.8, 0.7, -3.0, 1.0), 0.0)
    desk.add('low', B.bowed('D2', 2.4, child(r, 'low'), 0.8, attack=0.4, release=1.2, vib=0.04, bright=0.35), 0.0)
    return desk.mix(_hall(r, 2.2), _bus)


@cue('UI.Menu.Reveal', variations=1, space='2D', cls='Interface', jitter=0.0, conc=1, level=-16.0, align=False,
     swell=True)
def menu_reveal(v, r):
    # The menu arriving over the planet: air drawn up past the ear to its top at 0.6 s, where the chapel bell is struck
    # softly with a chime over it, a low bowed fifth swelling in under it all.
    top = 0.6
    desk = B.Desk(None, tail=3.0)
    desk.track('air', 0.0, 0.0, -12.0)
    desk.track('low', -5.0, 0.0, -10.0, B.cello_body)
    desk.track('bell', -6.0, 0.0, -6.0)
    desk.track('chime', -10.0, 0.2, -6.0)
    desk.add('air', kit.whoosh(top + 0.35, child(r, 'air'), 300.0, 2600.0, 0.9, top / (top + 0.35), -3.0, 1.2), 0.0)
    for j, note in enumerate(['D2', 'A2']):
        desk.add('low', B.bowed(note, 2.0, child(r, 'l', j), 0.7, attack=0.55, release=1.0, vib=0.04, bright=0.35), 0.0)
    desk.add('bell', B.bell('D4', child(r, 'bell'), 0.35, t60=3.5, dur=3.0), top)
    desk.add('chime', kit.chime(kit.note('A5'), child(r, 'chime'), 1.4, 0.6), top + 0.03)
    return desk.mix(_hall(r, 2.0), _bus)


@cue('UI.Menu.Select', variations=2, space='2D', cls='Interface', jitter=0.02, conc=2, level=-15.0)
def menu_select(v, r):
    # The interface's brass switch with a wooden knock and some weight under it, then a short bright chime left ringing:
    # a main menu choice made, heavier than UI.Click.
    press = kit.brass_tick(child(r, 'press'), 1300.0 * jitter(r, 1, 0.03), 0.022, 4, 0.0001, -8.0, -8.0, 0.1)
    knock = kit.thunk(360.0 * jitter(r, 1, 0.04), child(r, 'knock'), 0.06, 0.0008, 5, 0.04)
    weight = kit.noise_thump(0.12, child(r, 'weight'), 900.0, 120.0, 0.06, 1.0, 0.04)
    chime = kit.chime(kit.note(['D6', 'E6'][v]), child(r, 'chime'), 0.5, 0.55, 0.0003)
    chime = chime * fit(E.perc(0.6, 0.002, 0.45), chime.size)
    x = layers((press, 0.0, 0.0), (knock, 0.004, -3.0), (weight, 0.004, -9.0), (normalize(chime), 0.03, -13.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 1.0), extend=False)
    return _ui(x, r, 0.3, -21.0, 0.22)
