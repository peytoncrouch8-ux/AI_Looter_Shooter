"""Recorded layers for bullets meeting things (the user's call, 2026-10-21: gun and impact recordings are allowed,
processed here; the rest of each sound stays synthesized). Each helper returns one take's recorded layer, varied per take
(played a little faster or slower, a different tone, sometimes a later start), for impacts.py, creatures.py and
brass.py to lay under their synthesized hits at a level of their own:

    body(v, r)      a bullet into a body: a recorded body thump, dull and meaty (Creature.Hit, under a spider's shell)
    wet(v, r)       a wet, gritty slap (a slime's gel taking a round)
    stone(v, r)     a round into concrete and brick: the hard crack and the chips (Impact.Stone, stone takes of World)
    metal(v, r)     a round into heavy steel: the clank and its short ring (Impact.Metal)
    flyby(v, r)     a bullet's recorded pass, with a supersonic crack off a machine gun's rounds overhead (NearMiss.Whiz)

They're heard by the hundred in a firefight, so each keeps the ear's touchiest band (2-5 kHz) eased, as impacts.py does.
The recordings: Art/Sounds/packs/Sonniss/ (sources and licence in Art/Sounds/packs/README.md).
"""
import numpy as np

from lib.core import ns, layers, normalize, child, jitter, varispeed
from lib import env as E, filters as F
from lib import sample as S

# Where each recording's hit starts (s), and how much of it is used.
_HITS = {
    'Impact_Body.wav': (0.049, 0.4),
    'Impact_PunchWet.wav': (0.0, 0.45),
    'Impact_PunchBody.wav': (0.0, 0.4),
    'Impact_Concrete.wav': (0.039, 0.3),
    'Impact_MetalHeavy.wav': (0.068, 0.7),
    'Bullet_FlyBy.wav': (0.0, 0.5),
}
# The supersonic cracks overhead in Bullet_Cracks.wav (s): each a bright crack a few ms long before its gun's thump.
_CRACKS = [0.090, 0.184, 0.278, 0.374, 0.472, 0.565, 0.657, 0.753, 0.844, 0.940, 1.035, 1.129, 1.223, 1.318]


def _hit(name, r, speed=0.08, length=None):
    """A recording's hit, from its onset, played up to speed faster or slower (a smaller or bigger thing hit)."""
    at, use = _HITS[name]
    x = S.load(name)
    if at > 0.0:
        y = S.slice_at(x, at, length or use, fade_out=0.04)
    else:
        y = S.cut(x, 0, x.size, length or use, 0.0, 0.04)
    y = varispeed(y, jitter(r, 1.0, speed))
    return E.end_fade(y, 0.01) if y.ndim == 1 else y


def _ease(x, cut_db=-5.0, shelf_db=-3.0):
    return F.filt(x, F.peak(3300.0, cut_db, 0.8), F.highshelf(9000.0, shelf_db), extend=False)


def body(v, r):
    """A recorded body thump (a bullet into meat): its lows kept round, its top eased."""
    x = _hit('Impact_Body.wav', child(r, 'body'), 0.1, 0.2)
    other = _hit('Impact_PunchBody.wav', child(r, 'punch'), 0.1, 0.2)
    # Cut short: a creature's hit comes many times a second under fire (a hit's thud at most every 0.06 s).
    x = S.gate_tail(layers(normalize(x), (normalize(other), 0.0, -8.0 - 4.0 * (v % 2))), 0.04, 0.1)
    return _ease(F.filt(x, F.hp(70.0), F.lp(9000.0), extend=False), -6.0, -4.0)


def wet(v, r):
    """A recorded wet, gritty slap for gel: darker and softer than the body's, nothing hard in it."""
    x = S.gate_tail(_hit('Impact_PunchWet.wav', child(r, 'wet'), 0.12, 0.25), 0.05, 0.15)
    return F.filt(x, F.hp(80.0), F.lpn(3200.0, 2), F.peak(250.0, 2.0, 0.9), extend=False)


def stone(v, r):
    """A recorded round into concrete and brick: the crack, the chips and a little grit after."""
    # Only its first crack: the take hits twice (a chip landing 0.11 s later would read as a second round).
    x = S.gate_tail(_hit('Impact_Concrete.wav', child(r, 'stone'), 0.1, 0.1), 0.03, 0.08)
    return _ease(F.filt(x, F.hp(120.0), extend=False), -7.0, -3.0)


def metal(v, r):
    """A recorded round into heavy steel: the clank and the plate's short ring, cut a little shorter by take."""
    x = _hit('Impact_MetalHeavy.wav', child(r, 'metal'), 0.08, 0.35 + 0.1 * (v % 3))
    x = S.gate_tail(x, 0.08, 0.3)
    return _ease(F.filt(x, F.hp(150.0), extend=False), -8.0, -3.0)


def flyby(v, r):
    """A bullet's recorded pass (its louder channel by take, played faster or slower so the whine's pitch varies), with a
    recorded supersonic crack on its front: the crack alone, taken above the machine gun's thumps under it."""
    ch = ('left', 'right')[v % 2]
    x = S.load('Bullet_FlyBy.wav', channel=ch)
    x = varispeed(x, [1.0, 1.12, 0.92, 1.2, 1.06, 0.96][v % 6] * jitter(r, 1.0, 0.03))
    # The pass's loudest moment is its middle: the take starts just before it, so the whiz arrives with the crack (the
    # shock wave reaches the ear as the round goes by), as the synthesized whiz's does.
    k = int(np.argmax(np.abs(x)))
    x = x[max(0, k - ns(0.018)):]
    x = E.end_fade(x * E.bp([(0.0, 0.25), (0.018, 1.0), (x.size / 48000.0, 0.0)], n=x.size, curve='cos'), 0.01)
    cr = S.load('Bullet_Cracks.wav')
    c = S.slice_at(cr, _CRACKS[(3 * v + 1) % len(_CRACKS)], 0.012, fade_out=0.004)
    c = F.filt(c, F.hpn(1800.0, 4), extend=False)
    out = layers((normalize(x), 0.0, 0.0), (normalize(c), 0.0, -4.0))
    return F.filt(out, F.peak(3300.0, -6.0, 0.7), F.hp(350.0, 0.7), extend=False)


# --- Wood, dirt and flesh (2026-10-23: the user's yes to one more part of the same bundles) -----------------------------
# Boards struck and dropped, rocks thrown into dirt, a stab, a splat and a crunch: none of them a bullet, so each is played
# faster (a smaller, quicker thing hitting) and cut short. Their hits (s into each excerpt; make_pack.py prints them):
_TAKES = {
    'wood': [('Impact_WoodHard.wav', 0.051), ('Impact_WoodCrack.wav', 0.040), ('Impact_WoodPlank.wav', 0.038),
             ('Impact_WoodPlank.wav', 0.477)],
    'dirt': [('Impact_DirtRock.wav', t) for t in (0.035, 0.714, 1.396, 2.073, 2.754, 3.434)] +
            [('Impact_GravelRock.wav', 0.038)],
    'flesh': [('Flesh_Stab.wav', 0.032), ('Flesh_Splat.wav', 0.039), ('Flesh_Crunch.wav', 0.069),
              ('Flesh_Crunch.wav', 0.653)],
}


def _take(kind, v, r, length, speed):
    """Take v of a kind's list from its hit, length seconds, played speed times faster (jittered a little)."""
    name, at = _TAKES[kind][v % len(_TAKES[kind])]
    y = S.slice_at(S.load(name), at, length, fade_out=0.03, search=0.008)
    y = varispeed(y, speed * jitter(r, 1.0, 0.06))
    return E.end_fade(y, 0.01)


def wood(v, r):
    """A board or a post taking a round: a recorded board's crack and knock, played faster, its fizz and 3 kHz eased."""
    x = S.gate_tail(_take('wood', v, child(r, 'wood'), 0.3, [1.35, 1.2, 1.5, 1.3, 1.25, 1.45][v % 6]), 0.05, 0.12)
    return _ease(F.filt(x, F.hp(100.0), extend=False), -6.0, -4.0)


def dirt(v, r):
    """A round into packed earth: a recorded rock thrown into dirt (the thud and the soil it throws), played faster."""
    x = S.gate_tail(_take('dirt', v, child(r, 'dirt'), 0.35, [1.3, 1.4, 1.25, 1.35, 1.45, 1.3][v % 6]), 0.06, 0.15)
    return _ease(F.filt(x, F.hp(70.0), F.lpn(8000.0, 2), extend=False), -5.0, -3.0)


def flesh(v, r):
    """A round into flesh: a recorded stab, splat or crunch, short and meaty, its top eased."""
    x = S.gate_tail(_take('flesh', v, child(r, 'flesh'), 0.25, [1.1, 1.0, 1.15, 1.05, 1.0][v % 5]), 0.04, 0.1)
    return _ease(F.filt(x, F.hp(60.0), F.lpn(7000.0, 2), extend=False), -6.0, -4.0)
