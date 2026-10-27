"""The heroes' melee tools (round 4, 2026-10-27, the user's "melee polish"; Source: Player/PlayerMeleeTool*.cpp, MeleeFX in
Weapons/WeaponFXMelee.h). Each tool has its own swing (2D, the player's own, crossing the ears the way the tool swings) and
its own hit (3D, 'Near'), played on top of the shared blow (player.py: Player.Melee.Hit's weight and the body's layer), so
these carry what the tool is made of, not the blow's weight:

- Spade (Ellis's grave spade, 95 cm, a flat backhand swat high right to low left): the flat blade buffeting the air; on a
  hit a ringing clang of black iron plate with the ash shaft's knock under it.
- Fan (Odessa's razor fan, five steel-edged cards, a backhand flick low left to high right): a thin quick cut of air and a
  riffle as the fan snaps open; on a hit a slicing snick, a steel edge drawn fast, with a papery flutter after.
- Gaff (Crane's ferry gaff, 120 cm, a heavy overhand hooking chop then a yank): a long pole's slow deep rush, the ash
  creaking and the fare coin's tick; on a hit the hook's bite: a dull knock, the spike's short iron ring and the tearing
  drag of the yank.
- Piston (Gauge's piston fist, a quick jab): a short punch of air over the valve charging; on a hit the hiss-thump: the
  knuckle plate's thump, the piston's iron clack, a jet of steam.
- Pick (Pike's rock pick, 35 cm, a quick overhand peck): a short sharp whistle; on a hit a bright steel chink with grit.

Heard over and over in a fight, so each has its 2-5 kHz band trimmed (check.py's HARSH flag).
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R
from recipes import cue
from recipes import kit


def _pan(x, p0, p1):
    """Mono to stereo, the place moving from p0 to p1 (-1 left, +1 right) over the sound: the tool crossing the view."""
    n = x.size
    a = (np.linspace(p0, p1, n) + 1.0) * np.pi / 4.0
    return np.vstack([x * np.cos(a), x * np.sin(a)]) * np.sqrt(2.0)


def _rush(r, dur, f0, f_peak, f1, peak_at, q, flutter):
    """Air pushed aside by something swung: a band of noise sliding up to f_peak as it goes by fastest, back down after,
    buffeted (a flat blade or a pole flutters the air), quiet as it starts."""
    n = ns(dur)
    fc = O.glide([(0.0, f0), (dur * peak_at, f_peak), (dur, f1)], n=n)
    src = N.pink(n, child(r, 'air')) * kit.turbulence(n, child(r, 'turb'), flutter, 0.45)
    body = F.sweep(src, 'bp', fc, q)
    u = np.arange(n) / n
    env = np.where(u < peak_at, np.sin(0.5 * np.pi * np.minimum(u / peak_at, 1.0)) ** 3.0,
                   np.exp(-6.0 * (u - peak_at) / (1.0 - peak_at)))
    return normalize(body * E.end_fade(env, 0.01))


def _tame(x):
    """The swing's and hit's shared finish: the 2-5 kHz edge trimmed, nothing under 45 Hz."""
    return F.filt(x, F.peak(3000.0, -3.0, 0.8), F.highshelf(7000.0, -3.0), F.hp(45.0), extend=False)


def _space(x, r, db=-24.0):
    """A little of the open ground's slap-back, as the shared hit has."""
    ir = R.outdoor(child(r, 'ir'), dur=0.5, slaps=((0.05, -5.0), (0.13, -10.0)), tail_t60=0.3, tail_db=-14.0,
                   tail_start=0.02, tail_peak=0.06)
    return layers(x, kit.set_level(F.convolve(F.filt(x, F.hpn(220.0, 2), extend=False), ir), x, db, 0.02))


# --- Ellis's grave spade ---------------------------------------------------------------------------------------------

@cue('Player.Melee.Spade.Swing', variations=4, space='2D', jitter=0.05, conc=2, level=-13.0)
def spade_swing(v, r):
    # The flat blade swatted through the air: a broad, low rush that flutters as the flat catches it, crossing from high
    # on the right to low on the left, the D-grip's ash creaking in the fist as it starts.
    dur = 0.34 + 0.04 * r.random()
    rush = _rush(r, dur, 220.0, 1100.0 * jitter(r, 1, 0.1), 300.0, 0.45, 1.4, 60.0)
    creak = kit.creak(0.08, child(r, 'grip'), 60.0, 100.0, (380.0, 900.0, 1800.0), 6.0, 0.3)
    x = layers(_pan(rush, 0.5, -0.45), (_pan(creak, 0.4, 0.4), 0.0, -22.0))
    return _tame(x)


@cue('Player.Melee.Spade.Hit', variations=4, att='Near', jitter=0.05, conc=3, level=-11.0)
def spade_hit(v, r):
    # Black iron plate struck flat: a ringing clang (dense plate modes, the low ones holding), a hard contact tick, the ash
    # shaft's dull knock under it, a little of the ground's echo.
    plate = M.strike(M.plate(480.0 * jitter(r, 1, 0.08), child(r, 'plate'), 20, 0.7, 0.5), M.hammer(0.0003), 0.9)
    tick = kit.metal_click(child(r, 'tick'), 1800.0, 6000.0, 0.03, 0.0001, 5, -8.0)
    shaft = kit.thunk(260.0 * jitter(r, 1, 0.1), child(r, 'shaft'), 0.07, 0.0008, 6, 0.1)
    x = layers((normalize(plate), 0.0, 0.0), (tick, 0.0, -8.0), (shaft, 0.0, -6.0))
    x = D.drive(normalize(x), 3.0, 'tanh')
    return _space(_tame(x), r)


# --- Odessa's razor fan ----------------------------------------------------------------------------------------------

@cue('Player.Melee.Fan.Swing', variations=4, space='2D', jitter=0.06, conc=2, level=-14.0)
def fan_swing(v, r):
    # A quick backhand flick: a thin, fast cut of air rising from low on the left to high on the right, and the five cards
    # riffling open mid-cut (a quick run of papery ticks ending in a snap).
    dur = 0.2 + 0.03 * r.random()
    rush = _rush(r, dur, 700.0, 3200.0 * jitter(r, 1, 0.1), 1200.0, 0.5, 1.8, 120.0)
    n = ns(0.07)
    riffle = N.dust(n, child(r, 'riffle'), 180.0, (0.4, 1.0)) * E.ar(0.07, 0.02, 0.05, 1.5)
    riffle = F.filt(riffle, F.bp(2600.0, 1.2), extend=False)
    snap = kit.cloth(0.04, child(r, 'snap'), 900.0, 5000.0, 400.0, 0.05, 0.6)
    detail = layers((normalize(riffle), 0.0, 0.0), (snap, 0.06, -4.0))
    x = layers(_pan(rush, -0.45, 0.5), (_pan(detail, -0.1, 0.1), dur * 0.35, -12.0))
    return _tame(x)


@cue('Player.Melee.Fan.Hit', variations=4, att='Near', jitter=0.06, conc=3, level=-12.0)
def fan_hit(v, r):
    # Steel-edged cards slicing: a fast, bright snick (an edge drawn across, sliding up), a tiny steel tick, then the cards'
    # papery flutter settling.
    snick = kit.scrape(0.07, child(r, 'snick'), 2400.0 * jitter(r, 1, 0.1), 3600.0, 18.0, 0.4, (1.0, 1.52, 2.4), 0.05, 0.6, 6000.0)
    tick = kit.metal_click(child(r, 'tick'), 3000.0, 9000.0, 0.02, 0.00006, 4, -6.0)
    flutter = kit.cloth(0.12, child(r, 'flutter'), 700.0, 4500.0, 260.0, 0.1, 0.8)
    x = layers((snick, 0.0, 0.0), (tick, 0.0, -6.0), (flutter, 0.04, -14.0))
    return _tame(x)


# --- Crane's ferry gaff ----------------------------------------------------------------------------------------------

@cue('Player.Melee.Gaff.Swing', variations=4, space='2D', jitter=0.04, conc=2, level=-12.0)
def gaff_swing(v, r):
    # A long, heavy pole brought over and down: a slow, deep rush (more air than any other tool), the old ash creaking
    # under the strain, and the fare coin nailed to it ticking once.
    dur = 0.42 + 0.05 * r.random()
    rush = _rush(r, dur, 140.0, 800.0 * jitter(r, 1, 0.1), 180.0, 0.4, 1.2, 45.0)
    creak = kit.creak(0.16, child(r, 'ash'), 40.0, 80.0, (300.0, 700.0, 1300.0), 7.0, 0.35)
    coin = kit.metal_click(child(r, 'coin'), 2200.0, 6500.0, 0.06, 0.0001, 4, -10.0)
    x = layers(_pan(rush, 0.4, -0.15), (_pan(creak, 0.3, 0.3), 0.0, -18.0), (_pan(coin, 0.2, 0.2), dur * 0.4, -24.0))
    return _tame(x)


@cue('Player.Melee.Gaff.Hit', variations=4, att='Near', jitter=0.05, conc=3, level=-11.0)
def gaff_hit(v, r):
    # The hook biting in: the head's dull knock, the iron spike's short dark ring, then the yank, a tearing drag back with
    # grit in it.
    knock = kit.thunk(180.0 * jitter(r, 1, 0.1), child(r, 'knock'), 0.06, 0.0009, 6, 0.1)
    ring = kit.metal_click(child(r, 'ring'), 900.0, 3200.0, 0.12, 0.0002, 6, -12.0, 0.5)
    drag = kit.scrape(0.22, child(r, 'drag'), 700.0, 380.0, 9.0, 0.8, (1.0, 1.47, 2.31), 0.15, 0.6, 1800.0, -14.0)
    grit = kit.grit_burst(0.18, child(r, 'grit'), 1400.0, 900.0, 4500.0, 0.1)
    x = layers((knock, 0.0, 0.0), (ring, 0.0, -6.0), (drag, 0.06, -8.0), (grit, 0.07, -14.0))
    return _space(_tame(x), r, -26.0)


# --- Gauge's piston fist ---------------------------------------------------------------------------------------------

@cue('Player.Melee.Piston.Swing', variations=4, space='2D', jitter=0.04, conc=2, level=-14.0)
def piston_swing(v, r):
    # A short straight jab: a punch of air ahead, the valve charging (a quick hiss rising) and its click as it arms.
    dur = 0.18 + 0.03 * r.random()
    rush = _rush(r, dur, 300.0, 1500.0 * jitter(r, 1, 0.1), 500.0, 0.55, 1.4, 90.0)
    n = ns(0.12)
    charge = N.band(n, child(r, 'charge'), 1500.0, 7000.0) * E.ar(0.12, 0.1, 0.02, 2.0)
    click = kit.metal_click(child(r, 'valve'), 1300.0, 5000.0, 0.03, 0.0002, 5, -10.0)
    x = layers(_pan(rush, 0.25, 0.0), (_pan(normalize(charge), 0.2, 0.2), 0.0, -16.0), (_pan(click, 0.2, 0.2), 0.11, -14.0))
    return _tame(x)


@cue('Player.Melee.Piston.Hit', variations=4, att='Near', jitter=0.04, conc=3, level=-10.0)
def piston_hit(v, r):
    # The hiss-thump: the knuckle plate driven in by the piston (a deep thump and the iron's clack), then the cylinder
    # venting, a jet of steam that swells and fades.
    thump = kit.thump(78.0 * jitter(r, 1, 0.08), 42.0, 0.2, 0.1, attack=0.001, drive_db=5.0)
    clack = kit.clack(child(r, 'clack'), 520.0, 1500.0, 5000.0, 0.06, 0.05, -4.0, -8.0)
    dur = 0.45 + 0.1 * r.random()
    n = ns(dur)
    steam = N.band(n, child(r, 'steam'), 1800.0, 9000.0) * kit.turbulence(n, child(r, 'tur'), 80.0, 0.4)
    steam = F.filt(steam, F.highshelf(6500.0, -5.0), F.peak(3300.0, -4.0, 0.9), extend=False)
    steam = normalize(steam * E.ar(dur, 0.03, dur * 0.8, 1.6))
    x = layers((thump, 0.0, 0.0), (clack, 0.0, -4.0), (steam, 0.025, -10.0))
    return _space(_tame(x), r)


# --- Pike's rock pick ------------------------------------------------------------------------------------------------

@cue('Player.Melee.Pick.Swing', variations=4, space='2D', jitter=0.06, conc=2, level=-15.0)
def pick_swing(v, r):
    # A small hammer pecked down: a short, sharp whistle of air from high on the right to the middle.
    dur = 0.16 + 0.03 * r.random()
    rush = _rush(r, dur, 600.0, 2600.0 * jitter(r, 1, 0.1), 900.0, 0.55, 2.2, 100.0)
    return _tame(_pan(rush, 0.35, -0.05))


@cue('Player.Melee.Pick.Hit', variations=4, att='Near', jitter=0.06, conc=3, level=-12.0)
def pick_hit(v, r):
    # Tool steel on whatever it strikes: a bright, short chink (a hammer head's bar modes), a hard tick, a spit of grit.
    chink = M.strike(M.bar(1500.0 * jitter(r, 1, 0.08), child(r, 'bar'), 0.22, 0.6, 6), M.hammer(0.00012), 0.3)
    tick = kit.metal_click(child(r, 'tick'), 2500.0, 8000.0, 0.02, 0.00008, 4, -6.0)
    grit = kit.grit_burst(0.1, child(r, 'grit'), 1800.0, 1500.0, 6000.0, 0.06)
    handle = kit.thunk(330.0 * jitter(r, 1, 0.1), child(r, 'handle'), 0.05, 0.0007, 5, 0.1)
    x = layers((normalize(chink), 0.0, 0.0), (tick, 0.0, -8.0), (grit, 0.003, -14.0), (handle, 0.0, -10.0))
    return _space(_tame(x), r, -26.0)
