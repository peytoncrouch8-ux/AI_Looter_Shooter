"""The soul flask (2026-10-30, Docs/Work/2026-10-30-SoulFlask/CONTRACT.md): a small corked glass flask of soul-light the
player drinks on a key to heal mid-fight, filled again by the soul of kills. Every cue is the player's own, 2D, played by
Player/FlaskFX.cpp by the names in Player/PlayerFlaskCues.h (FlaskCue).

Glass, cork and breath, with the soul-motes' warm major glass (loot.py: Loot.SoulMote.*) as the light's voice, so the
flask is heard as kin to the motes that fill it; never brass (brass is the guns' rarity), never anything bubbling like a
fantasy potion. Physical first:
- Player.Flask.Uncork: the cork twisted in the glass neck (stick-slip, rung through the neck's modes), then pulled: the
  pop is the flask's air cavity ringing as a Helmholtz resonator (about 380 Hz for 2 cm of neck over 200 ml) on the
  pressure step of the cork leaving, a woody tock, and the light stirring (a breath of glassy shimmer).
- Player.Flask.Drink: the glass at the lips, the flask glugging as air goes in (bubble rings through the neck), two
  swallows (a throat's low wet gulp), a short breath out.
- Player.Flask.Heal: the soul taken in: a warm major chord of glass rising and blooming (the mote pickup's swell, grown),
  a breath of air drawn up with it, a few glints; stereo, wide.
- Player.Flask.ChargeFilled: a clear little glass chime (a note and its fifth, a hair apart), the light settling.
- Player.Flask.Empty: a dry glass clink: the empty flask knocked twice on its tin cup, no liquid in it.
- Player.Flask.Full: a soft, muted tap: a fingertip on the cork, the full flask's dull slosh (no need).
- Player.Flask.Refilled: a soft three-note fill rising (the major triad of the heal), quiet: the respawn's top-up.
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter, smoothstep
from lib import noise as N, env as E, osc as O, filters as F, modal as M, stereo as S, voice as V
from recipes import cue
from recipes import kit


# --- Shared pieces --------------------------------------------------------------------------------------------------

def _glass(r, f0, t60=0.25, contact=0.00008):
    """A small blown-glass vessel struck: a few inharmonic shell modes (a bottle's), bright and quick, the upper ones
    dying first."""
    ratios = np.array([1.0, 2.32, 4.25, 6.63, 9.1])
    f = f0 * ratios * (1.0 + 0.012 * (2.0 * r.random(ratios.size) - 1.0))
    d = t60 * ratios ** -0.55
    a = ratios ** -0.65 * (0.6 + 0.4 * r.random(ratios.size))
    return normalize(M.strike(M.modes(f, d, a), M.hammer(contact), t60 * 1.6 + 0.01), 0.0)


def _cavity(r, f0, t60=0.07, rise=0.08):
    """The flask's air ringing as a Helmholtz resonator: one low tone, sliding up a little as the neck clears (the pop's
    body). Struck by a soft pressure step, so it begins round, not with a click."""
    dur = t60 * 1.6
    n = ns(dur)
    t = times(n)
    f = f0 * (1.0 + rise * smoothstep(np.clip(t / (dur * 0.6), 0.0, 1.0)))
    tone = np.sin(2.0 * np.pi * O.phase(f, n)) * E.perc(dur, 0.0012, t60)
    return normalize(tone, 0.0)


def _pad(root, dur, r, chord=((1.0, 1.0), (1.25, 0.5), (1.5, 0.55), (2.0, 0.6), (2.5, 0.32), (3.0, 0.25), (4.0, 0.14)),
         beat=0.9, bend=0.0):
    """A soft chord of glass (sines in beating pairs) over dur, its pitch sliding up by bend as it goes."""
    n = ns(dur)
    t = times(n)
    glide = 1.0 + bend * smoothstep(t / dur)
    out = np.zeros(n)
    for i, (ratio, amp) in enumerate(chord):
        for s in (-0.5, 0.5):
            f = root * ratio * glide + s * beat * (0.8 + 0.3 * i) * r.uniform(0.7, 1.3)
            out += 0.5 * amp * np.sin(2.0 * np.pi * O.phase(f, n) + 2.0 * np.pi * r.random())
    return normalize(out)


def _glints(r, count, start, spread, lo=2100.0, hi=3300.0, fall=2.5):
    """A few tiny glass taps scattered after start, each softer: the light's shimmer (as the motes' sparkle)."""
    out = np.zeros(1)
    for i in range(count):
        f = float(np.exp(r.uniform(np.log(lo), np.log(hi))))
        c = kit.chime(f, child(r, 'glint', i), 0.3, 0.3, 0.0003, 0.4, -40.0)
        out = layers(out, (c, start + spread * r.random(), -fall * i - r.uniform(0.0, 3.0)))
    return normalize(out)


def _close(x, r, wet_db=-20.0):
    """Close to the ears: a little stereo width and the faint body of a small space round the player (the flask is in
    their own hands), so the dry synthesis doesn't sit in a void."""
    from lib import reverb as R
    dry = S.widen(x, child(r, 'wide'), 0.25)
    ir = R.stereo_ir(R.room, child(r, 'ir1'), child(r, 'ir2'), t60=0.3, size=0.6, damp=0.5)
    wet = F.convolve(F.filt(x, F.hp(250.0), extend=False), ir)
    return layers(dry, kit.set_level(wet, dry, wet_db, 0.02))


# --- The drink ------------------------------------------------------------------------------------------------------

@cue('Player.Flask.Uncork', variations=3, space='2D', cls='Effects', jitter=0.04, conc=1, level=-11.0)
def uncork(v, r):
    # The cork twisted a quarter turn in the glass neck (a short squeak: cork sticking and slipping on glass, rung
    # through the neck), then drawn: the pop (the cavity's ring on the pressure step), the cork's woody tock as it
    # clears the lip, and the light stirring inside in a breath of glassy shimmer.
    squeak = kit.creak(0.11 + 0.02 * v, child(r, 'squeak'), 190.0 * jitter(r, 1, 0.1), 330.0 * jitter(r, 1, 0.1),
                       (880.0, 1750.0, 3100.0), 9.0, 0.3)
    squeak = F.filt(squeak, F.peak(3300.0, -5.0, 0.8), extend=False)
    grip = kit.cloth(0.07, child(r, 'grip'), 400.0, 2800.0, 160.0, 0.2, 0.6)
    at = 0.105 + 0.02 * v
    pop = _cavity(child(r, 'pop'), [380.0, 352.0, 405.0][v] * jitter(r, 1, 0.02), 0.065, 0.1)
    puff = N.band(ns(0.03), child(r, 'puff'), 300.0, 2600.0) * E.perc(0.03, 0.0008, 0.018)
    tock = kit.thunk(1250.0 * jitter(r, 1, 0.08), child(r, 'tock'), 0.014, 0.00025, 4, 0.08)
    ring = _glass(child(r, 'ring'), 2350.0 * jitter(r, 1, 0.05), 0.12)
    shimmer = _glints(child(r, 'shimmer'), 3, 0.05, 0.22)
    x = layers((grip, 0.0, -14.0), (squeak, 0.006, -7.0), (normalize(puff), at, -9.0), (pop, at, 0.0),
               (tock, at + 0.004, -5.0), (ring, at + 0.006, -20.0), (shimmer, at + 0.02, -24.0))
    x = F.filt(x, F.hp(90.0), F.highshelf(7500.0, -3.0), extend=False)
    return _close(x, r)


def _glug(r, f0):
    """One glug: air pushing into the tipped flask through its neck, a bubble ringing up, the liquid knocked."""
    b = kit.bubble(f0, child(r, 'b'), r.uniform(0.09, 0.14))
    slap = N.band(ns(0.025), child(r, 'slap'), 250.0, 1800.0) * E.perc(0.025, 0.002, 0.015)
    return normalize(layers((b, 0.0, 0.0), (normalize(slap), 0.0, -12.0)))


def _gulp(r, f0=95.0):
    """A swallow: the throat's low wet gulp (a dull falling thump of the larynx and a short squelch, all kept under 1 kHz
    as heard from inside the head)."""
    thump = kit.thump(f0 * 1.4, f0 * 0.7, 0.14, 0.08, attack=0.004)
    wet = kit.squelch(0.11, child(r, 'wet'), 700.0, 260.0, 3.0)
    x = layers((thump, 0.0, 0.0), (wet, 0.012, -5.0))
    return normalize(F.filt(x, F.lp(1100.0, 0.7), extend=False))


@cue('Player.Flask.Drink', variations=3, space='2D', cls='Effects', jitter=0.03, conc=1, level=-12.0)
def drink(v, r):
    # The gulp: the glass touching the lips (a tiny dull tick), the flask glugging three or four times as air goes in, two
    # swallows under them, and a short breath out at the end, through the nose, satisfied.
    lip = _glass(child(r, 'lip'), 1900.0 * jitter(r, 1, 0.05), 0.05, 0.0004)
    out = np.zeros(1)
    t = 0.04
    for i in range(3 + (v % 2)):
        g = _glug(child(r, 'glug', i), r.uniform(430.0, 640.0))
        out = layers(out, (g, t, -2.0 * i - r.uniform(0.0, 2.0)))
        t += r.uniform(0.085, 0.12)
    g1 = _gulp(child(r, 'gulp1'), 98.0 * jitter(r, 1, 0.06))
    g2 = _gulp(child(r, 'gulp2'), 90.0 * jitter(r, 1, 0.06))
    breath = V.whisper(0.26, [(0.0, 'h'), (0.26, 'a')], child(r, 'breath'), 1.0)
    breath = normalize(F.filt(breath * E.bp([(0.0, 0.0), (0.05, 1.0), (0.26, 0.0)], n=ns(0.26), curve='cos'),
                              F.hp(300.0), F.lp(4500.0), extend=False))
    x = layers((lip, 0.0, -14.0), (out, 0.0, -3.0), (g1, 0.2, 0.0), (g2, 0.47 + 0.03 * v, -1.5),
               (breath, 0.62 + 0.03 * v, -18.0))
    x = F.filt(x, F.hp(60.0), F.peak(3300.0, -4.0, 0.8), extend=False)
    return _close(x, r, -24.0)


@cue('Player.Flask.Heal', variations=2, space='2D', cls='Effects', jitter=0.01, conc=1, level=-9.0, swell=True)
def heal(v, r):
    # The soul taken in: a warm major chord of glass rising out of the gulp and blooming through the body (the motes'
    # pickup swell grown: a root low and round, its third, fifth and octaves), sliding up a hair as it fills, a breath of
    # air drawn up with it, glints of light over the top, then settling warm. About as long as the heal (0.8 s) and its
    # afterglow. The rise is in its brightness as much as its level: the chord's upper voices (a glass bowl an octave and
    # a fifth up, sliding up as the light goes in) open over the warm low ones as it blooms, so it lifts, not drones.
    from recipes import creatures as C
    root = [146.8, 164.8][v]
    dur = 1.5
    n = ns(dur)
    pad = _pad(root, dur, child(r, 'pad'), bend=0.03)
    env = E.bp([(0.0, 0.0), (0.06, 0.18), (0.5, 1.0), (0.8, 0.7), (dur, 0.0)], n=n, curve='cos')
    pad = normalize(F.filt(pad * env, F.lp(3800.0, 0.7), extend=False))
    low = O.sine(np.full(n, root * 0.5)) * E.bp([(0.0, 0.0), (0.45, 1.0), (dur, 0.0)], n=n, curve='cos')
    rise = layers((C._bowl(root * 2.0, 1.25, child(r, 'bowl1'), 0.06, 1.6), 0.0, 0.0),
                  (C._bowl(root * 3.0, 1.15, child(r, 'bowl2'), 0.06, 2.2), 0.08, -3.0))
    rise = normalize(rise * E.bp([(0.0, 0.0), (0.12, 0.15), (0.55, 1.0), (1.3, 0.0)], n=rise.size, curve='cos'))
    lift = kit.whoosh(0.9, child(r, 'lift'), 300.0, 2600.0, 1.0, 0.55, -3.0, 0.9)
    touch = kit.chime(root * 8.0, child(r, 'touch'), 0.5, 0.45, 0.0003, 0.4, -36.0)
    glints = _glints(child(r, 'glints'), 5, 0.3, 0.55)
    x = layers((touch, 0.0, -14.0), (pad, 0.0, 0.0), (normalize(low), 0.0, -18.0), (rise, 0.04, -4.0),
               (lift, 0.05, -10.0), (glints, 0.0, -15.0))
    x = F.filt(x, F.hp(70.0), F.peak(3300.0, -4.0, 0.8), extend=False)
    return _close(x, r, -18.0)


# --- Filling and refusing -------------------------------------------------------------------------------------------

@cue('Player.Flask.ChargeFilled', variations=3, space='2D', cls='Effects', jitter=0.0, conc=2, level=-12.0)
def charge_filled(v, r):
    # A charge filled by soul: a clear little glass chime, its note and the fifth over it a breath apart (bright, short,
    # kept off the 2-5 kHz edge), and the light settling in the flask in a soft low swell under it.
    a = [1174.7, 1318.5, 1108.7][v]
    c1 = kit.chime(a, child(r, 'c1'), 0.9, 0.5, 0.00015, 0.5, -30.0)
    c2 = kit.chime(a * 1.5, child(r, 'c2'), 0.8, 0.4, 0.00015, 0.5, -30.0)
    settle = _pad(a / 4.0, 0.7, child(r, 'settle'), chord=((1.0, 1.0), (1.5, 0.5), (2.0, 0.45)), beat=0.7)
    settle = normalize(settle * E.bp([(0.0, 0.0), (0.12, 1.0), (0.7, 0.0)], n=ns(0.7), curve='cos'))
    x = layers((c1, 0.0, 0.0), (c2, 0.07, -3.0), (settle, 0.02, -21.0))
    x = F.filt(x, F.hp(120.0), F.peak(3300.0, -5.0, 0.8), F.highshelf(8000.0, -3.0), extend=False)
    return _close(x, r, -20.0)


@cue('Player.Flask.Empty', variations=3, space='2D', cls='Effects', jitter=0.04, conc=1, level=-13.0)
def empty(v, r):
    # Nothing left: the empty flask knocked twice on its tin cup as it's shaken, a dry glass clink each time (the glass's
    # modes, no liquid damping them), the tin's dull tick under it, and the glass's hollow ring dying away.
    f0 = [1650.0, 1780.0, 1540.0][v]
    out = np.zeros(1)
    for i, at in enumerate((0.0, 0.085 + 0.01 * v)):
        clink = _glass(child(r, 'clink', i), f0 * jitter(r, 1, 0.03), 0.085, 0.00006)
        tin = M.strike(M.shell(720.0 * jitter(r, 1, 0.05), child(r, 'tin', i), 0.03, 5, 0.03), M.hammer(0.0002), 0.06)
        hollow = _cavity(child(r, 'hollow', i), 330.0 * jitter(r, 1, 0.03), 0.09, 0.0)
        hit = layers((clink, 0.0, 0.0), (normalize(tin), 0.0, -10.0), (hollow, 0.0, -18.0))
        out = layers(out, (hit, at, -3.0 * i))
    x = F.filt(out, F.hp(150.0), F.peak(3300.0, -4.0, 0.8), extend=False)
    return _close(x, r, -22.0)


@cue('Player.Flask.Full', variations=3, space='2D', cls='Effects', jitter=0.04, conc=1, level=-16.0)
def full(v, r):
    # No need: a fingertip's soft tap on the cork (dull, woody, muffled by the hand round the flask) and the full flask's
    # small, heavy slosh, dying away low.
    tap = kit.thunk(520.0 * jitter(r, 1, 0.06), child(r, 'tap'), 0.03, 0.0012, 5, 0.1)
    n = ns(0.24)
    slosh = N.band(n, child(r, 'slosh'), 180.0, 900.0) * kit.turbulence(n, child(r, 'st'), 70.0, 0.6)
    slosh = normalize(slosh * E.perc(0.24, 0.02, 0.15))
    knock = kit.thump(160.0, 120.0, 0.12, 0.07, attack=0.002)
    x = layers((tap, 0.0, 0.0), (knock, 0.0, -9.0), (slosh, 0.01, -13.0))
    x = F.filt(x, F.hp(80.0), F.lp(3000.0, 0.7), extend=False)
    return _close(x, r, -24.0)


@cue('Player.Flask.Refilled', variations=2, space='2D', cls='Effects', jitter=0.0, conc=1, level=-16.0)
def refilled(v, r):
    # The respawn's top-up, quiet: three soft glass notes rising (the heal's major triad, a breath apart, the way the
    # three charges light), the light swelling softly in under them.
    root = [587.3, 659.3][v]
    out = np.zeros(1)
    for i, ratio in enumerate((1.0, 1.25, 1.5)):
        c = kit.chime(root * ratio, child(r, 'note', i), 0.9, 0.35, 0.0005, 0.4, -40.0)
        out = layers(out, (c, 0.13 * i, -1.5 * (2 - i)))
    swell = _pad(root / 4.0, 1.0, child(r, 'swell'), chord=((1.0, 1.0), (1.25, 0.4), (1.5, 0.5), (2.0, 0.4)), beat=0.6)
    swell = normalize(swell * E.bp([(0.0, 0.0), (0.35, 1.0), (1.0, 0.0)], n=ns(1.0), curve='cos'))
    x = layers((out, 0.0, 0.0), (swell, 0.0, -20.0))
    x = F.filt(x, F.hp(100.0), F.peak(3300.0, -5.0, 0.8), F.highshelf(7000.0, -4.0), extend=False)
    return _close(x, r, -18.0)
