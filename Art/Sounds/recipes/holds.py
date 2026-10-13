"""Hold to interact (the round hold ring in HudInteractPromptWidget, the scene skip): the ring filling all the way.

It plays at the end of every hold, so it is built like the hit marker and the menu click: a small brass switch's soft
click with its note under 2 kHz, a bright tick of steel above 5 kHz on top of it (a latch catching), and next to nothing
in between, so a long session of holds never tires the ear.

- UI.Hold.Complete (2D): the soft click and, a hair after it, the bright tick; about a tenth of a second.
"""
from lib.core import layers, child, jitter
from lib import filters as F
from recipes import cue
from recipes import kit
from recipes.ui import _ui


@cue('UI.Hold.Complete', variations=3, space='2D', cls='Interface', jitter=0.02, conc=2, level=-16.0)
def hold_complete(v, r):
    # A small brass switch pressed home (a soft, low tk with a dull knock under it), and 9-12 ms later a steel latch's
    # bright tick catching over it: done.
    click = kit.brass_tick(child(r, 'click'), 1150.0 * jitter(r, 1, 0.03), 0.018, 4, 0.00012, -14.0, -14.0, 0.0)
    click = F.filt(click, F.lp(3000.0, 0.7), extend=False)
    tick = kit.metal_click(child(r, 'tick'), 5600.0, 9800.0, 0.011, 0.00005, 4, -12.0, 0.2)
    tick = F.filt(tick, F.hp(4800.0, 0.7), extend=False)
    knock = kit.thump(330.0, 230.0, 0.025, 0.012, attack=0.0004)
    x = layers((click, 0.0, 0.0), (knock, 0.0, -17.0), (tick, 0.009 + 0.003 * r.random(), -7.0))
    x = F.filt(x, F.peak(3300.0, -6.0, 0.8), extend=False)
    return _ui(x, r, 0.2, -26.0, 0.12)
