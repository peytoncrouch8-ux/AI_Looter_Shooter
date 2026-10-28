"""The gun makers' own layers (2026-10-28, the user's pick: Roundhouse heavy, Saint Maud ringing, Windfall rattly,
Meridian clean). The game shapes every shot's layers by its maker in code (Weapons/MakerSoundFlavor.cpp: pitch and each
layer's volume); two makers also add one layer of their own here, played with the action on the shooter's own gun
(GunshotSound::PlayShot, cue names MakerSoundCue in Weapons/MakerSoundFlavor.h; nothing plays until the bank has them):

    Weapon.Maker.SaintMaud.Ring    the blessed cast barrel ringing on after the shot: a short, clear iron ring
    Weapon.Maker.Windfall.Rattle   the salvage gun's bolted-on bits rattling as the action cycles: loose tin and steel

Played on every shot under the shot itself (pitched with the maker's own pitch, so they follow a kind's weight a little),
so each starts soft under the shot's crack, stays short, and keeps the ear's touchiest band (2-5 kHz) eased.
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, modal as M
from recipes import cue
from recipes import kit


def _ease(x, cut_db=-6.0, shelf_db=-3.0):
    return F.filt(x, F.peak(3300.0, cut_db, 0.8), F.highshelf(9000.0, shelf_db), extend=False)


@cue('Weapon.Maker.SaintMaud.Ring', variations=4, att='Near', jitter=0.02, conc=4, level=-20.0)
def saint_maud_ring(v, r):
    # A thick cast barrel struck by its own shot: near-harmonic tube modes (a low clear note that rings about half a
    # second), a few denser plate modes for the cast body (a little shimmer that dies first), struck softly (the shot's
    # own crack is the strike; a hard hammer here would click twice), then faded on its decay.
    f0 = [640.0, 690.0, 610.0, 665.0][v] * jitter(r, 1.0, 0.01)
    tube = M.shell(f0, child(r, 'tube'), t60=0.75, count=6, spread=0.006)
    body = M.scale(M.plate(f0 * 1.62, child(r, 'body'), count=10, t60=0.28, spread=0.015), amp=0.35)
    dur = 0.9
    ring = M.strike(M.merge(tube, body), M.hammer(0.0005), dur)
    # A slow beat in the ring (the casting isn't quite round): two copies a hair apart.
    beat = M.strike(M.scale(tube, freq=1.0 + 0.0035 * jitter(r, 1.0, 0.3), amp=0.5), M.hammer(0.0005), dur)
    x = layers((normalize(ring), 0.0, 0.0), (normalize(beat), 0.0, -8.0))
    x = F.filt(x, F.hp(250.0), F.lpn(6500.0, 2), extend=False)
    x = x * E.end_fade(np.ones(x.size), 0.03)
    return _ease(normalize(x), -6.0, -4.0)


@cue('Weapon.Maker.Windfall.Rattle', variations=5, att='Near', jitter=0.06, conc=4, level=-18.0)
def windfall_rattle(v, r):
    # The gun's odd bits shaken by the action: a few loose steel pieces knocking (a pin, a swivel, a tag on a wire) over
    # about 90 ms, a tin panel bolted on loose buzzing once against the frame, and a dull knock of a part that moves.
    bits = kit.jingle(0.2, child(r, 'bits'), 6 + int(r.integers(0, 3)), lo=1600.0, hi=6500.0, t60=0.03,
                      spread=0.09, start=0.008)
    panel = M.strike(M.plate([360.0, 410.0, 330.0, 390.0, 345.0][v], child(r, 'panel'), count=14, t60=0.07, spread=0.04),
                     M.hammer(0.0003), 0.14)
    # The panel buzzes against the frame: chopped by a fast flutter, so it's a rattle, not a tone.
    n = panel.size
    flutter = 0.5 + 0.5 * np.tanh(6.0 * np.sin(2.0 * np.pi * r.uniform(70.0, 95.0) * times(n)))
    panel = panel * (0.35 + 0.65 * flutter)
    knock = kit.thunk(r.uniform(420.0, 520.0), child(r, 'knock'), t60=0.04, contact=0.0008)
    x = layers((normalize(bits), 0.0, 0.0), (normalize(panel), 0.004, -5.0), (normalize(knock), 0.0, -8.0))
    x = F.filt(x, F.hp(200.0), extend=False)
    x = x * E.end_fade(np.ones(x.size), 0.012)
    return _ease(normalize(x), -6.0, -3.0)
