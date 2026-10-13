"""Mission features (2026-10-13, wave 2a): optional objectives, mission items, the turn-in's choice card, readables, Delia's
porch plate and the minimap going underground.

They speak the givers' language (givers.py: paper on pine, the clerk's stamp) and the mission pings' (feedback.py: small
brass chimes, steel strings), with the score's bowed strings (band.py) where a choice should weigh something. Physical
first: a pencil's graphite is a soft grainy friction with the tip's tap; glass jars of salt clink with a thick glass's few
damped modes and the salt shifting inside; a dry biscuit snaps with a crack and a crumble and is chewed behind closed lips
(its crunch muffled by the cheeks).

- UI.Mission.OptionalDone (2D): a pencil's tick on paper (the check's two strokes), then a small bright two-note sparkle
  rising, lighter than a step's pings.
- World.MissionItem.Take (3D, near): a small weighty thing lifted and pocketed (a knock as it leaves the ground, cloth,
  the leather pocket taking it) with a soft bright glint over it.
- World.MissionItem.Drop (3D): a small hard thing thudding into the dirt, then a soft rising shimmer as its gold beam
  comes up.
- UI.Mission.ChoiceOpen (2D): the turn-in's card, heavier than the offer's: paper drawn across the boards, a solid settle,
  under a low bowed note with a suspended fourth left hanging over it.
- UI.Mission.ChoiceHold (2D, 1 s, the ring's 0.9 s fill): a low bowed swell rising a fifth as the ring fills, a breath
  drawn in with it; the game fades it if the key is let go.
- UI.Mission.ChoiceMade (2D, final): a heavy stamp landing on the card's slam (0.12 s), a deep boom and a low bell under
  it, and a deep D major chord of strings settling: a debt paid.
- World.Readable.Read (3D, near): something read: a page turned (1), a board brushed off with a little chalk dust (2), a
  page brushed clean and turned (3).
- UI.Readable.Close (2D, quiet): the paper let go and set back down.
- World.PorchPlate.Take (3D, near): two glass jars of salt lifted clinking off a tin plate, the salt shifting in them, a
  folded slip of paper opened.
- World.PorchPlate.Eat (3D, near, slow): a cold dry biscuit broken in two, crumbs ticking on the tin, chewed without hurry
  behind closed lips, a swallow.
- UI.Minimap.Underground (2D, soft, under the music): the wind outside shut off as if by a door (its top closing down, a
  muffled thud), leaving a low hush that fades.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, mono, add_at
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S, voice as V
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes import band as B
from recipes.ui import _ui
from recipes.feedback import _pings
from recipes.givers import crackle, rub, flap, unfold, stamp, swish, calm, STAMP_LANDS, _outside


# --- Shared pieces ----------------------------------------------------------------------------------------------------

def _graphite(dur, r, level=1.0):
    """A pencil stroke: graphite dragged over paper, a soft grainy friction (many tiny catches through broad bands),
    rounded at both ends."""
    n = ns(dur)
    g = G.grit(dur, child(r, 'grit'), 7000.0, 700.0, 5000.0, 1.1, 4)
    hiss = N.band(n, child(r, 'hiss'), 600.0, 4500.0)
    y = normalize(g) + 0.35 * normalize(hiss)
    return normalize(y * E.ar(dur, dur * 0.2, dur * 0.45, 1.5)) * level


def _glass(f0, r, t60=0.07, damp=1.0):
    """A thick glass jar struck: a few stiff, widely spaced modes, short because the salt in it damps them."""
    ratios = np.array([1.0, 1.62, 2.47, 3.3])
    f = f0 * ratios * (1.0 + 0.02 * (2 * r.random(4) - 1))
    a = ratios ** -0.9 * (0.6 + 0.4 * r.random(4))
    d = t60 * damp * ratios ** -0.5
    ring = M.strike((f, d, a), M.hammer(0.00007), t60 * 2.0 + 0.01)
    return normalize(F.filt(ring, F.peak(3300.0, -5.0, 0.8), F.highshelf(7000.0, -4.0), extend=False), 0.0)


def _salt(dur, r):
    """Coarse salt shifting in a jar: a dry hiss of grains sliding, rising and settling."""
    n = ns(dur)
    g = G.grit(dur, child(r, 'grains'), 9000.0, 1200.0, 6500.0, 1.5, 4)
    return normalize(F.filt(g, F.peak(3300.0, -4.0, 0.8), extend=False) * E.ar(dur, dur * 0.3, dur * 0.6, 1.5), 0.0)


def _tin_plate(r, f0=520.0, t60=0.22):
    """A tin plate knocked: a thin sheet's dense modes, a short dull ring."""
    m = M.plate(f0 * jitter(r, 1, 0.06), child(r, 'plate'), 16, t60, 0.6, tilt=0.8)
    ring = normalize(M.strike(m, M.hammer(0.00015), t60 * 1.5 + 0.02))
    return normalize(F.filt(ring, F.peak(3300.0, -6.0, 0.8), F.highshelf(6000.0, -6.0), extend=False), 0.0)


def _strings(notes, dur, r, attack=0.3, release=1.0, vel=0.7, vib=0.1):
    """A few bowed strings held together (the score's cello and bass: band.bowed through its cello body), centred with a
    little spread."""
    out = np.zeros((2, ns(dur)))
    for i, (name, db, pan) in enumerate(notes):
        x = B.bowed(name, dur, child(r, 'string', i), vel, attack, release, vib, 5.2, 0.35, 0.45)
        x = B.cello_body(x)
        out = layers(out, (S.pan(normalize(x), pan), 0.0, db))
    return normalize(out)


# --- Optional objectives ----------------------------------------------------------------------------------------------

@cue('UI.Mission.OptionalDone', variations=2, space='2D', cls='Interface', jitter=0.0, conc=2, level=-13.0)
def optional_done(v, r):
    # An optional objective ticked: the pencil's tip tapping down and drawing the check (a short stroke down, a longer one
    # up), then a small bright sparkle, two quick high pings rising a fifth: lighter and higher than a step's.
    tap = kit.thunk(950.0 * jitter(r, 1, 0.05), child(r, 'tap'), 0.015, 0.0004, 4, 0.05)
    s1 = _graphite(0.028, child(r, 's1'), 0.8)
    s2 = _graphite(0.055, child(r, 's2'), 1.0)
    tick = layers((tap, 0.0, -6.0), (s1, 0.0, 0.0), (s2, 0.034, -1.0))
    tick = _ui(calm(tick, 300.0), child(r, 'room'), 0.2, -26.0, 0.15)
    # Its notes sit just under 2 kHz (a step's pings are lower still), so the sparkle stays off the ear's sharpest band.
    a, b = [('E6', 'B6'), ('G6', 'B6')][v]
    sparkle = _pings(child(r, 'sparkle'), [(a, 0.0, -3.0, -0.25, 0.16), (b, 0.065, 0.0, 0.25, 0.26)], (), -18.0, 0.4)
    sparkle = F.filt(sparkle, F.peak(3300.0, -5.0, 0.8), extend=False)
    return layers((tick, 0.0, 0.0), (sparkle, 0.11, -5.0))


# --- Mission items ----------------------------------------------------------------------------------------------------

@cue('World.MissionItem.Take', variations=3, att='Near', cls='Effects', jitter=0.04, conc=2, level=-12.0)
def item_take(v, r):
    # A small weighty thing (a ledger, a lockbox key, a miner's lamp) lifted out of the dirt and pocketed: a dull knock as
    # it leaves the ground, grit falling off it, the sleeve's cloth, the leather pocket creaking as it takes the weight;
    # a soft bright glint over it, so it reads as the mission's.
    knock = kit.thunk(380.0 * jitter(r, 1, 0.1), child(r, 'knock'), 0.05, 0.0012, 6, 0.1)
    grit = kit.grit_burst(0.16, child(r, 'grit'), 2200.0, 700.0, 3500.0, 0.07)
    cloth = kit.cloth(0.26, child(r, 'cloth'), 280.0, 2400.0, 130.0, 0.25, 0.55)
    t = 0.24 + 0.04 * r.random()
    leather = kit.creak(0.12, child(r, 'leather'), 50.0, 30.0, (280.0, 640.0, 1350.0), 6.0, 0.3)
    pocket = kit.noise_thump(0.11, child(r, 'pocket'), 600.0, 110.0, 0.05, 1.0, 0.05)
    a, b = [('E6', 'B6'), ('D6', 'A6'), ('F#6', 'C#7')][v]
    glint = layers((kit.chime(kit.note(a), child(r, 'g1'), 0.7, 0.5, 0.0003, 0.6, -40.0), 0.0, 0.0),
                   (kit.chime(kit.note(b), child(r, 'g2'), 0.9, 0.5, 0.0003, 0.6, -40.0), 0.06, -2.0))
    glint = normalize(glint) * E.swell(glint.size / SR, 0.03, 0.8, 1.2)
    x = layers((knock, 0.0, -2.0), (grit, 0.0, -14.0), (cloth, 0.04, -9.0), (leather, t, -10.0), (pocket, t + 0.03, -3.0),
               (normalize(glint), 0.08, -10.0))
    return _outside(calm(x, 70.0), r, -24.0)


@cue('World.MissionItem.Drop', variations=3, att='Creature', cls='Effects', jitter=0.03, conc=2, level=-11.0)
def item_drop(v, r):
    # It falls from the creature that carried it: a small hard thing thudding into the dirt (a knock, a puff of grit, a
    # tick as it settles), then a soft shimmer of small chimes rising up a D major arpeggio as its gold beam comes up.
    thud = kit.noise_thump(0.14, child(r, 'thud'), 750.0, 95.0, 0.06, 1.0, 0.04)
    knock = kit.thunk(330.0 * jitter(r, 1, 0.1), child(r, 'knock'), 0.06, 0.0007, 6, 0.1)
    grit = kit.grit_burst(0.2, child(r, 'grit'), 3000.0, 800.0, 4000.0, 0.09)
    settle = kit.thunk(420.0 * jitter(r, 1, 0.1), child(r, 'settle'), 0.03, 0.0006, 5, 0.1)
    notes = [['D6', 'F#6', 'A6', 'D7'], ['A5', 'D6', 'F#6', 'A6'], ['B5', 'D6', 'F#6', 'B6']][v]
    shimmer = np.zeros(1)
    for i, name in enumerate(notes):
        c = kit.chime(kit.note(name) * (1.0 + 0.002 * r.standard_normal()), child(r, 'c', i), 0.9 - 0.1 * i, 0.5,
                      0.0004, 0.8, -40.0)
        shimmer = layers(shimmer, (c, 0.11 * i + 0.02 * r.random(), -1.0 * i))
    n = shimmer.size
    shimmer = normalize(shimmer) * E.bp([(0.0, 0.35), (0.35, 1.0), (n / SR, 1.0)], n=n, curve='cos')
    air = kit.whoosh(0.7, child(r, 'air'), 900.0, 3800.0, 0.9, 0.7, -3.0, 1.3)
    rise = layers((normalize(shimmer), 0.0, 0.0), (air, 0.0, -16.0))
    rise = F.filt(rise, F.peak(3300.0, -4.0, 0.8), F.lp(7000.0, 0.7), extend=False)
    x = layers((thud, 0.0, 0.0), (knock, 0.0, -5.0), (grit, 0.002, -14.0), (settle, 0.07 + 0.02 * r.random(), -14.0),
               (rise, 0.16, -12.0))
    return _outside(F.filt(x, F.hp(60.0), extend=False), r, -22.0)


# --- The turn-in's choice ---------------------------------------------------------------------------------------------

@cue('UI.Mission.ChoiceOpen', variations=2, space='2D', cls='Interface', jitter=0.0, conc=1, level=-12.0)
def choice_open(v, r):
    # The turn-in's choice card, heavier than the offer's: the paper drawn slowly across the boards and settling with a
    # solid knock, under a low bowed note; a fourth above it creeps in and is left hanging (a suspension that never
    # resolves): the question waits.
    d = 0.32 + 0.04 * r.random()
    drawn = rub(d, child(r, 'drawn'), 220.0, 3200.0, 600.0, 0.3, 0.5)
    settle = flap(child(r, 'settle'), 0.1, 600.0, 130.0)
    knock = kit.thunk(170.0 * jitter(r, 1, 0.05), child(r, 'knock'), 0.1, 0.002, 6, 0.08)
    paper = layers((drawn, 0.0, -2.0), (settle, d - 0.03, -3.0), (knock, d - 0.025, -6.0))
    paper = _ui(calm(paper), child(r, 'room'), 0.3, -20.0, 0.3)
    root, sus = [('D3', 'G3'), ('C3', 'F3')][v]
    dur = 1.9
    held = _strings([(root, 0.0, -0.1), (B.hz(root) / 2.0, -8.0, 0.1)], dur, child(r, 'held'), 0.25, 0.9, 0.7, 0.08)
    hang = _strings([(sus, 0.0, 0.25)], dur - 0.35, child(r, 'hang'), 0.4, 0.8, 0.6, 0.12)
    strings = layers((held, 0.0, 0.0), (hang, 0.35, -9.0))
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=1.6, lp=3000.0)
    wet = F.convolve(F.filt(mono(strings), F.hp(150.0), extend=False), ir)
    strings = layers(strings, kit.set_level(wet, strings, -14.0, 0.05))
    return layers((paper, 0.0, 0.0), (strings, 0.02, -7.0))


@cue('UI.Mission.ChoiceHold', variations=1, space='2D', cls='Interface', jitter=0.0, conc=1, level=-14.0, swell=True)
def choice_hold(v, r):
    # Holding a choice down while its ring fills (0.9 s): a low bowed swell rising a fifth (D2 to A2, the cello's octave
    # over it) and growing, a breath drawn in with it; it ends just after the ring is full, where ChoiceMade's stamp takes
    # over. Let go sooner and the game fades it out.
    fill = 0.9
    dur = 1.1
    n = ns(dur)
    t = times(n)
    u = np.clip(t / fill, 0.0, 1.0)
    f = B.hz('D2') * 1.5 ** (u * u * (3.0 - 2.0 * u))
    env = np.clip(t / fill, 0.0, 1.0) ** 1.6
    # Past the fill it lets go in 0.15 s, to silence before the end (the bow's hiss and the body ring on otherwise).
    env = env * np.cos(0.5 * np.pi * np.clip((t - fill) / 0.15, 0.0, 1.0)) ** 2
    low = B.cello_body(B.bowed(f, dur, child(r, 'low'), 1.0, env=env, vib=0.0, bright=0.5, noise_db=-26.0))
    high = B.cello_body(B.bowed(f * 2.0, dur, child(r, 'high'), 1.0, env=env ** 1.4, vib=0.06, vib_delay=0.2,
                                bright=0.45, noise_db=-28.0))
    breath = V.whisper(dur, [(0.0, 'h'), (fill * 0.5, 'o'), (fill, 'u'), (dur, 'u')], child(r, 'breath'), 0.95)
    breath = F.filt(breath, F.hp(300.0), F.lp(3500.0), extend=False) * env
    x = layers((S.pan(normalize(low), -0.1), 0.0, 0.0), (S.pan(normalize(high), 0.15), 0.0, -7.0),
               (S.widen(normalize(breath), child(r, 'bw'), 0.4), 0.0, -18.0))
    return F.filt(x, F.hp(45.0), extend=False)


@cue('UI.Mission.ChoiceMade', variations=2, space='2D', cls='Interface', jitter=0.0, conc=1, level=-6.0)
def choice_made(v, r):
    # Done, and for good: a heavy stamp swung down onto the card on its slam (0.12 s), a deep boom and a low bell under it,
    # then a deep D major chord of bowed strings settling out of the blow, a plucked low string ringing in it: a debt paid.
    land = STAMP_LANDS
    s = stamp(child(r, 'stamp'), 210.0 * jitter(r, 1, 0.04), -26.0)
    boom = kit.thump(78.0, 40.0, 0.7, 0.4, attack=0.002, drive_db=3.0)
    blow = layers((swish(r), 0.0, -13.0), (s, land, 0.0), (boom, land, -6.0))
    blow = _ui(calm(blow, 35.0), child(r, 'room'), 0.25, -16.0, 0.4)
    bell = normalize(M.strike(M.bell(B.hz('D3'), child(r, 'bell'), 4.0, 0.5, 0.6, 9), M.hammer(0.0012), 3.0))
    bell = F.filt(bell, F.lp(2200.0, 0.7), extend=False)
    voicing = [[('D2', -2.0, -0.3), ('A2', -3.0, 0.3), ('D3', -4.0, -0.1), ('F#3', -6.0, 0.2)],
               [('D2', -2.0, -0.3), ('A2', -3.0, 0.3), ('F#3', -5.0, -0.1), ('A3', -7.0, 0.2)]][v]
    chord = _strings(voicing, 2.8, child(r, 'chord'), 0.12, 1.6, 0.8, 0.1)
    pluck = layers(kit.twang(kit.note('D2'), 2.6, child(r, 'p1'), 0.4, 2.4),
                   (kit.twang(kit.note('A2'), 2.4, child(r, 'p2'), 0.4, 2.2), 0.012, -3.0))
    tail = layers((chord, 0.0, 0.0), (S.widen(normalize(pluck), child(r, 'pw'), 0.3), 0.0, -8.0),
                  (S.widen(bell, child(r, 'bw'), 0.2), 0.0, -12.0))
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=2.2, lp=3000.0)
    wet = F.convolve(F.filt(mono(tail), F.hp(150.0), extend=False), ir)
    tail = layers(tail, kit.set_level(wet, tail, -12.0, 0.05))
    return layers((blow, 0.0, 0.0), (tail, land + 0.01, -6.0))


# --- Readables --------------------------------------------------------------------------------------------------------

def _chalk_dust(dur, r):
    """A little chalk dust: dry grains drifting off a board, a faint high sifting."""
    n = ns(dur)
    g = G.grit(dur, child(r, 'dust'), 1500.0 * np.exp(-times(n) / (dur * 0.4)), 1800.0, 7000.0, 1.6, 3)
    return normalize(F.filt(g, F.peak(3300.0, -5.0, 0.8), extend=False), 0.0)


def _brush(r, strokes=2, lo=250.0, hi=2600.0):
    """A hand brushing a board or a page clean: a stroke or two of soft friction, a breath of air with each."""
    out = np.zeros(1)
    t = 0.0
    for i in range(strokes):
        d = r.uniform(0.11, 0.16)
        s = rub(d, child(r, 'stroke', i), lo, hi, 400.0, 0.3, 0.5, 18.0)
        out = layers(out, (s, t, -2.0 * i))
        t += d + r.uniform(0.02, 0.06)
    return normalize(out, 0.0), t


@cue('World.Readable.Read', variations=3, att='Near', cls='Effects', jitter=0.04, conc=2, level=-15.0)
def readable_read(v, r):
    # Leaning in to read something: a page turned (a book, a roll, a report), or a chalked board brushed off with the side
    # of a hand and a little dust sifting down, or a page brushed clean and turned.
    if v == 0:
        touch = kit.burst(0.012, child(r, 'touch'), 0.005, lo=400.0, hi=3000.0, attack=0.0008)
        lift = crackle(0.04, child(r, 'lift'), 900.0, 1000.0, 6000.0, 30.0) * E.ar(0.04, 0.003, 0.03, 1.5)
        air = kit.whoosh(0.22, child(r, 'leaf'), 260.0, 1300.0, 0.8, 0.55, -3.0)
        down = flap(child(r, 'down'), 0.08, 700.0, 170.0, -10.0)
        x = layers((touch, 0.0, -10.0), (normalize(lift), 0.0, -3.0), (air, 0.02, -6.0), (down, 0.22, -2.0))
    elif v == 1:
        brush, t = _brush(child(r, 'brush'), 2, 250.0, 2400.0)
        dust = _chalk_dust(0.6, child(r, 'dust'))
        knock = kit.thunk(240.0 * jitter(r, 1, 0.08), child(r, 'board'), 0.06, 0.003, 5, 0.08)
        x = layers((knock, 0.0, -8.0), (brush, 0.01, 0.0), (dust, 0.08, -14.0))
    else:
        brush, t = _brush(child(r, 'brush'), 1, 350.0, 3200.0)
        lift = crackle(0.035, child(r, 'lift'), 900.0, 1000.0, 6000.0, 30.0) * E.ar(0.035, 0.003, 0.025, 1.5)
        air = kit.whoosh(0.2, child(r, 'leaf'), 280.0, 1200.0, 0.8, 0.55, -3.0)
        down = flap(child(r, 'down'), 0.08, 700.0, 170.0, -10.0)
        dust = _chalk_dust(0.4, child(r, 'dust'))
        at = t + 0.04
        x = layers((brush, 0.0, -2.0), (dust, 0.05, -20.0), (normalize(lift), at, -3.0), (air, at + 0.02, -6.0),
                   (down, at + 0.2, -2.0))
    return _outside(calm(x, 100.0), r, -24.0)


@cue('UI.Readable.Close', variations=2, space='2D', cls='Interface', jitter=0.03, conc=2, level=-19.0)
def readable_close(v, r):
    # The reading put away: the paper let go with a soft crackle, and laid back down with a pat.
    let_go = crackle(0.05, child(r, 'let'), 600.0, 900.0, 5500.0, 25.0) * E.ar(0.05, 0.004, 0.035, 1.5)
    down = flap(child(r, 'down'), 0.1, 520.0, 130.0, -11.0)
    knock = kit.thunk(230.0 * jitter(r, 1, 0.06), child(r, 'knock'), 0.04, 0.003, 5, 0.06)
    t = 0.06 + 0.02 * r.random()
    x = layers((normalize(let_go), 0.0, -8.0), (down, t, 0.0), (knock, t + 0.004, -14.0))
    return _ui(calm(x), r, 0.25, -24.0, 0.2)


# --- Delia's porch plate ----------------------------------------------------------------------------------------------

@cue('World.PorchPlate.Take', variations=2, att='Near', cls='Effects', jitter=0.03, conc=1, level=-12.0)
def porch_take(v, r):
    # The two jars of grave salt taken off the tin plate: the first lifted with a clink of glass on tin, the salt shifting
    # in it, the second knocking the first as it comes; then Delia's folded slip opened.
    out = np.zeros(1)
    t = 0.0
    for i in range(2):
        jar = _glass(r.uniform(1150.0, 1450.0), child(r, 'jar', i), 0.07)
        tin = _tin_plate(child(r, 'tin', i), 520.0, 0.2)
        salt = _salt(0.2, child(r, 'salt', i))
        out = layers(out, (jar, t, 0.0), (tin, t, -6.0), (salt, t + 0.02, -12.0))
        t += 0.2 + 0.05 * r.random()
    clink = _glass(r.uniform(1700.0, 1900.0), child(r, 'clink'), 0.05)
    out = layers(out, (clink, t - 0.05, -5.0))
    slip, ts = unfold(child(r, 'slip'), 2, 0.1)
    x = layers((out, 0.0, 0.0), (slip, t + 0.2, -3.0))
    return _outside(calm(x, 90.0), r, -24.0)


def _chew(r, crunch):
    """One slow chew behind closed lips: the jaw closing on dry biscuit (a crunch muffled by the cheeks) and the soft
    damp knock of the teeth meeting through it."""
    d = 0.16
    n = ns(d)
    grains = G.grit(d, child(r, 'crunch'), 2500.0 * crunch + 400.0, 500.0, 3000.0, 1.3, 4)
    grains = F.filt(grains, F.lp(2000.0, 0.7), extend=False) * E.ar(d, 0.03, 0.1, 1.5)
    jaw = kit.noise_thump(0.08, child(r, 'jaw'), 500.0, 120.0, 0.035, 1.0, 0.03, 0.006)
    wet = kit.squelch(0.1, child(r, 'wet'), 500.0, 300.0, 3.0)
    return layers((normalize(grains), 0.0, -2.0 + 4.0 * (crunch - 1.0)), (jaw, 0.02, -6.0),
                  (wet, 0.04, -20.0 + 8.0 * (1.0 - crunch)))


@cue('World.PorchPlate.Eat', variations=2, att='Near', cls='Effects', jitter=0.02, conc=1, level=-16.0)
def porch_eat(v, r):
    # A cold dry biscuit broken in two (a crack and a crumble), crumbs ticking on the tin plate; then chewed slowly, five
    # times, the crunch going soft; a swallow. Ellis can't taste it: no hurry, no pleasure.
    crack = kit.burst(0.008, child(r, 'crack'), 0.003, lo=1000.0, hi=6000.0)
    crumble = kit.grit_burst(0.14, child(r, 'crumble'), 5000.0, 900.0, 5000.0, 0.06)
    body = kit.thunk(520.0 * jitter(r, 1, 0.1), child(r, 'body'), 0.03, 0.0006, 5, 0.12)
    crumbs = np.zeros(1)
    for i in range(3 + int(r.integers(0, 2))):
        tick = _tin_plate(child(r, 'crumb', i), 900.0, 0.05)
        crumbs = layers(crumbs, (tick, 0.08 + 0.18 * r.random(), -6.0 - 3.0 * i))
    brk = layers((crack, 0.0, -4.0), (crumble, 0.0, -6.0), (body, 0.0, -4.0), (normalize(crumbs), 0.0, -14.0))
    chews = np.zeros(1)
    t = 0.0
    for i in range(5):
        chews = layers(chews, (_chew(child(r, 'chew', i), 1.0 - 0.15 * i), t, -1.0 * i))
        t += r.uniform(0.5, 0.6)
    gulp = kit.squelch(0.14, child(r, 'gulp'), 650.0, 240.0, 4.0)
    click = kit.thunk(900.0, child(r, 'throat'), 0.012, 0.001, 3, 0.05)
    down = kit.noise_thump(0.12, child(r, 'down'), 400.0, 90.0, 0.05, 1.0, 0.06, 0.004)
    swallow = layers((gulp, 0.0, 0.0), (down, 0.02, -4.0),
                     (click, 0.03, -16.0))
    swallow = F.filt(swallow, F.lp(1800.0, 0.7), extend=False)
    x = layers((brk, 0.0, 0.0), (chews, 0.65 + 0.05 * r.random(), -6.0), (normalize(swallow), 0.75 + t, -8.0))
    return _outside(calm(x, 80.0), r, -26.0)


# --- The minimap going underground ------------------------------------------------------------------------------------

@cue('UI.Minimap.Underground', variations=1, space='2D', cls='Interface', jitter=0.0, conc=1, level=-21.0)
def underground(v, r):
    # Going under: the wind outside, then shut off as if by a door, its top closing down to a muffled low hush with a
    # soft dull thud; the hush fades. Soft: it plays under the music as the map goes dark.
    dur = 1.5
    n = ns(dur)
    t = times(n)
    shut = 0.16
    cutoff = np.where(t < shut, 2600.0, 2600.0 * (260.0 / 2600.0) ** np.clip((t - shut) / 0.09, 0.0, 1.0))

    def side(rr):
        wind = N.pink(n, child(rr, 'wind'), lo=120.0)
        gust = 0.75 + 0.25 * N.smooth_random(n, child(rr, 'gust'), 3.0)
        return F.sweep(wind * gust, 'lp', cutoff, 0.8)

    wind = np.vstack([side(child(r, 'l')), side(child(r, 'r'))])
    env = E.bp([(0.0, 0.85), (shut, 1.0), (shut + 0.12, 0.55), (dur, 0.0)], n=n, curve='cos')
    wind = normalize(wind * env)
    thud = kit.noise_thump(0.3, child(r, 'thud'), 380.0, 55.0, 0.14, 1.0, 0.08, 0.003)
    x = layers((wind, 0.0, 0.0), (S.widen(thud, child(r, 'tw'), 0.1), shut, -6.0))
    return F.filt(x, F.hp(40.0), extend=False)
