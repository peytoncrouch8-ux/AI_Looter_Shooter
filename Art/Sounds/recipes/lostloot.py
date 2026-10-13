"""Lost Property (the depot's LOST PROPERTY window, ALostPropertyWindow and ULostLootWidget; guns handed in by
ULostLootSubsystem): an old railway baggage office of pine, brass, paper tickets and a rubber stamp.

The window's shutter is a pine panel sliding in wooden grooves: stick-slip friction rung through the panel's low
resonances (a creak with a rumble under it), a knock or two as it wobbles in its grooves, a thump at the stop. The
counter's bell is a small brass dome on a plunger: a palm's soft thump and a bright ring of a few inharmonic modes. The
paper is givers.py's (rubbing, crackle, the stamp), the coins and the counter's knock economy.py's, and the room is the
store's small timber room (economy._store) round the 2D cues, a smaller one round the window's.

- World.LostProperty.Open (3D, near): the shutter run up in its grooves to its stop, then the counter bell dinged by a palm.
- World.LostProperty.Close (3D, near): the shutter drawn down on its runners to the sill, and its iron latch dropping.
- UI.LostProperty.Claim (2D, rewarding): a rubber stamp thumped onto the ticket (CLAIMED, landing as the card's stamp does,
  0.12 s in), the stub torn off along its perforation, the gun lifted off a wooden shelf with a light knock of steel and
  a faint brass glint.
- UI.LostProperty.NoRoom (2D, short): the ticket slid back across the counter and a dull knuckle-rap on the boards.
- UI.LostProperty.Upgrade (2D, rewarding): coins counted onto the counter, three hammer blows driving a nail home, a plank
  set down on its brackets, and a rising pair of chimes.
- UI.LostProperty.Sent (2D, soft, under the HUD's line): a ledger page turned, a quick pen scratch, a station bell rung
  twice far off.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, add_at
from lib import noise as N, env as E, osc as O, filters as F, modal as M, reverb as R, stereo as S
from recipes import cue
from recipes import kit
from recipes.givers import crackle, rub, flap, stamp, swish, calm, STAMP_LANDS
from recipes.economy import _counted, _store
from recipes.bells import handbell
from recipes.world import _bell


# --- The window -------------------------------------------------------------------------------------------------------

def _office(x, r, wet_db=-18.0):
    """The baggage office behind the window: a small pine room answering a 3D sound."""
    ir = R.room(child(r, 'office'), t60=0.35, size=0.8, damp=0.4)
    wet = F.convolve(F.filt(x, F.hp(180.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, wet_db, 0.02))


def _slide(dur, r, f0, f1, speed):
    """A pine shutter sliding in its wooden grooves: stick-slip friction rung through the panel (a creak at speed's rate),
    a rumble of the panel's low resonance gliding from f0 to f1, juddering as it catches; it rattles once or twice in its
    grooves. speed: [(t, 0..1)] how fast it's moving, which sets the level and the creak's rate."""
    n = ns(dur)
    move = E.bp(speed, n=n, curve='cos')
    rumble = F.sweep(N.pink(n, child(r, 'rumble'), lo=60.0), 'bp', np.geomspace(f0, f1, n), 1.2)
    judder = 0.45 + 0.55 * np.abs(N.smooth_random(n, child(r, 'judder'), 24.0)) ** 1.3
    rumble = normalize(rumble * judder * move)
    creak = kit.creak(dur, child(r, 'creak'), 35.0, 70.0, (230.0 * jitter(r, 1, 0.08), 560.0, 1150.0), 6.0, 0.35)
    creak = normalize(creak * (0.3 + 0.7 * move))
    out = layers((rumble, 0.0, 0.0), (creak, 0.0, -5.0))
    for i in range(2):
        t = dur * r.uniform(0.2, 0.75)
        rattle = kit.thunk(260.0 * jitter(r, 1, 0.1), child(r, 'rattle', i), 0.05, 0.0008, 5, 0.1)
        out = layers(out, (rattle, t, -11.0 - 3.0 * i))
    return normalize(out, 0.0)


def _stop(r, f0=170.0, weight_db=-4.0):
    """The panel meeting its stop: a dull knock of pine, the push of its weight, its slats answering."""
    panel = kit.thunk(f0 * jitter(r, 1, 0.06), child(r, 'panel'), 0.12, 0.0012, 7, 0.1)
    weight = kit.noise_thump(0.13, child(r, 'weight'), 800.0, 100.0, 0.06, 1.0, 0.05)
    slat = kit.thunk(f0 * 2.6, child(r, 'slat'), 0.04, 0.0005, 5, 0.12)
    return layers((panel, 0.0, 0.0), (weight, 0.0, weight_db), (slat, 0.003, -9.0))


def _counter_bell(r, prime):
    """The counter's bell dinged by a palm: the palm's soft thump on the plunger, the striker's tick, and the little
    brass dome's ring (a hand bell's partials, small and bright, with the ear's sharpest band eased)."""
    m = handbell(prime, child(r, 'dome'), t60=1.3, beat=1.2, bright=0.8)
    ring = normalize(M.strike(m, M.hammer(0.00012), 1.6))
    ring = F.filt(ring, F.peak(3300.0, -5.0, 0.8), F.highshelf(7000.0, -4.0), extend=False)
    palm = kit.thunk(420.0 * jitter(r, 1, 0.06), child(r, 'palm'), 0.03, 0.004, 5, 0.06)
    pat = kit.noise_thump(0.06, child(r, 'pat'), 700.0, 160.0, 0.025, 0.9, 0.03)
    return layers((palm, 0.0, -6.0), (pat, 0.0, -9.0), (ring, 0.006, 0.0))


@cue('World.LostProperty.Open', variations=3, att='Near', cls='Effects', jitter=0.03, conc=1, level=-11.0)
def window_open(v, r):
    # The shutter pushed up in its grooves (quick, then easing as it reaches the top), a thump at its stop, the spring
    # catch clicking over; then a palm on the counter's brass bell: ding.
    d = 0.42 + 0.06 * r.random()
    up = _slide(d, child(r, 'up'), 260.0, 420.0, [(0.0, 0.5), (d * 0.3, 1.0), (d * 0.85, 0.7), (d, 0.25)])
    stop = _stop(child(r, 'stop'), 175.0)
    catch = kit.metal_click(child(r, 'catch'), 1400.0, 4200.0, 0.025, 0.0001, 5, -12.0)
    t_bell = d + 0.3 + 0.08 * r.random()
    bell = _counter_bell(child(r, 'bell'), [1620.0, 1700.0, 1560.0][v])
    x = layers((up, 0.0, -3.0), (stop, d, 0.0), (catch, d + 0.03, -14.0), (bell, t_bell, -4.0))
    x = F.filt(x, F.hp(70.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return _office(x, r)


@cue('World.LostProperty.Close', variations=3, att='Near', cls='Effects', jitter=0.03, conc=1, level=-12.0)
def window_close(v, r):
    # The shutter drawn down on its runners (it gathers speed as it falls), a heavier knock on the sill, and the iron
    # latch dropping into its keeper with a short rattle.
    d = 0.36 + 0.06 * r.random()
    down = _slide(d, child(r, 'down'), 420.0, 240.0, [(0.0, 0.3), (d * 0.6, 0.9), (d, 1.0)])
    sill = _stop(child(r, 'sill'), 150.0, -1.0)
    t = d + 0.13 + 0.05 * r.random()
    latch = kit.clack(child(r, 'latch'), 560.0, 1200.0, 4200.0, 0.05, 0.05, -3.0, -10.0, 0.00012)
    latch = F.filt(latch, F.peak(3300.0, -4.0, 0.9), extend=False)
    rattle = kit.jingle(0.18, child(r, 'rattle'), 3, 1200.0, 4000.0, 0.035, 0.07, 0.0)
    x = layers((down, 0.0, -3.0), (sill, d, 0.0), (latch, t, -5.0), (rattle, t + 0.012, -16.0))
    x = F.filt(x, F.hp(70.0), F.peak(3300.0, -3.0, 0.9), extend=False)
    return _office(x, r)


# --- The counter (2D) -------------------------------------------------------------------------------------------------

def _perforation(dur, r, rate=190.0):
    """A ticket's stub torn along its perforation: the bridges of paper between the holes snapping one after another (a
    fast zip of tiny ticks, quickening as the tear runs), the fibres tearing round them."""
    n = ns(dur)
    t = times(n)
    rate_curve = rate * (0.7 + 0.6 * t / dur) * (1.0 + 0.2 * N.smooth_random(n, child(r, 'hand'), 25.0))
    ph = O.phase(rate_curve, n)
    edges = np.nonzero(np.diff(np.floor(ph)) > 0)[0] + 1
    pulses = np.zeros(n)
    pulses[edges] = r.uniform(0.4, 1.0, edges.size)
    snaps = np.zeros(n)
    for c, q in ((1300.0, 2.0), (2500.0, 2.5), (5200.0, 2.0)):
        snaps += F.filt(pulses, F.bp(c * jitter(r, 1, 0.06), q), extend=False)
    fibres = N.band(n, child(r, 'fibres'), 900.0, 5000.0) * (0.3 + 0.7 * np.abs(N.smooth_random(n, child(r, 'fa'), 200.0)))
    env = E.ar(dur, 0.006, dur * 0.35, 1.5)
    return normalize(layers((normalize(snaps) * env, 0.0, 0.0), (normalize(fibres) * env, 0.0, -9.0)), 0.0)


def _lift_gun(r):
    """A gun lifted off a pine shelf: the hand's grip and a short scrape of its stock on the board, a light knock of
    steel as it comes free, the sleeve's cloth."""
    scrape = rub(0.12, child(r, 'scrape'), 220.0, 2600.0, 500.0, 0.15, 0.6)
    steel = normalize(M.strike(M.parts(800.0, 3000.0, child(r, 'steel'), 7, 0.045, 0.02, 0.45), M.hammer(0.0001), 0.1))
    knock = kit.thunk(240.0 * jitter(r, 1, 0.08), child(r, 'shelf'), 0.06, 0.0015, 6, 0.08)
    cloth = kit.cloth(0.2, child(r, 'cloth'), 300.0, 2400.0, 130.0, 0.2, 0.6)
    weight = kit.noise_thump(0.1, child(r, 'weight'), 600.0, 110.0, 0.04, 1.0, 0.04)
    return layers((knock, 0.0, -4.0), (scrape, 0.005, -10.0), (steel, 0.07, -7.0), (cloth, 0.03, -14.0),
                  (weight, 0.12, -8.0))


@cue('UI.LostProperty.Claim', variations=3, space='2D', cls='Interface', jitter=0.02, conc=1, level=-10.0)
def claim(v, r):
    # Yours again: the clerk's stamp swung down and thumped onto the ticket (CLAIMED), the stub torn off along its
    # perforation, then the gun lifted off its shelf with a light knock of steel and a faint brass glint.
    land = STAMP_LANDS
    s = stamp(child(r, 'stamp'), 260.0 * jitter(r, 1, 0.04))
    tear_at = 0.36 + 0.04 * r.random()
    tear = _perforation(0.12 + 0.03 * r.random(), child(r, 'tear'))
    tear = F.filt(tear, F.peak(3300.0, -4.0, 0.9), extend=False)
    lift_at = tear_at + 0.22 + 0.04 * r.random()
    lift = _lift_gun(child(r, 'lift'))
    a, b = [('E6', 'B6'), ('D6', 'A6'), ('F#6', 'C#7')][v]
    glint = layers((kit.chime(kit.note(a), child(r, 'g1'), 0.6, 0.45, 0.0003, 0.5, -40.0), 0.0, 0.0),
                   (kit.chime(kit.note(b), child(r, 'g2'), 0.7, 0.45, 0.0003, 0.5, -40.0), 0.07, -3.0))
    x = layers((swish(r), 0.0, -14.0), (s, land, 0.0), (tear, tear_at, -9.0), (lift, lift_at, -3.0),
               (normalize(glint), lift_at + 0.08, -17.0))
    return _store(calm(x, 60.0), r, -20.0, 0.3)


@cue('UI.LostProperty.NoRoom', variations=2, space='2D', cls='Interface', jitter=0.0, conc=2, level=-14.0)
def no_room(v, r):
    # No room on you: the ticket slid back across the counter to you, and two dull raps of a knuckle on the boards.
    d = 0.15 + 0.03 * r.random()
    back = rub(d, child(r, 'back'), 260.0, 3200.0, 650.0, 0.2, 0.55)
    out = np.zeros(1)
    t = d + 0.05
    for i in range(2):
        rap = kit.thunk(330.0 * jitter(r, 1, 0.05) * (1.0 - 0.04 * i), child(r, 'rap', i), 0.05, 0.0018, 5, 0.06)
        board = kit.thunk(160.0 * jitter(r, 1, 0.05), child(r, 'board', i), 0.08, 0.0025, 6, 0.08)
        out = layers(out, (rap, t, -1.5 * i), (board, t + 0.001, -8.0 - 1.5 * i))
        t += 0.1 + 0.02 * r.random()
    x = layers((back, 0.0, -6.0), (out, 0.0, 0.0))
    return _store(calm(x, 70.0), r, -22.0, 0.25)


def _hammer_blow(r, strength, nail_f):
    """A hammer driving a nail into pine: the steel face on the nail's head (a short ring of the hammer's head and the
    nail's thin shank), the board's knock and the push of the blow."""
    head = normalize(M.strike(M.bar(880.0 * jitter(r, 1, 0.05), child(r, 'head'), 0.07, 0.6, 5), M.hammer(0.00008), 0.12))
    nail = normalize(M.strike(M.bar(nail_f, child(r, 'nail'), 0.05, 0.7, 4), M.hammer(0.00006), 0.08))
    nail = F.filt(nail, F.peak(3300.0, -6.0, 0.8), F.highshelf(6000.0, -6.0), extend=False)
    board = kit.thunk(190.0 * jitter(r, 1, 0.06), child(r, 'board'), 0.09, 0.0008, 7, 0.1)
    push = kit.noise_thump(0.1, child(r, 'push'), 900.0, 110.0, 0.045, 1.0, 0.03)
    hit = layers((board, 0.0, 0.0), (push, 0.0, -3.0 + 2.0 * strength), (head, 0.0, -8.0), (nail, 0.0, -14.0))
    return normalize(hit) * (0.5 + 0.5 * strength)


@cue('UI.LostProperty.Upgrade', variations=2, space='2D', cls='Interface', jitter=0.02, conc=1, level=-10.0)
def upgrade(v, r):
    # More shelving: silver counted out onto the counter, then three hammer blows behind it driving a nail home (tap, tap,
    # the last one hard), a plank set down on its brackets one end then the other, and a rising pair of chimes.
    counted, t = _counted(child(r, 'pay'), 3 + int(r.integers(0, 2)), 0.085)
    t += 0.22
    for i, (strength, gap) in enumerate(((0.55, 0.22), (0.7, 0.26), (1.0, 0.0))):
        blow = _hammer_blow(child(r, 'blow', i), strength, 2300.0 * (1.0 + 0.05 * i))
        counted = layers(counted, (blow, t, 0.0))
        t += gap * jitter(r, 1, 0.08)
    t += 0.32
    end1 = kit.thunk(150.0 * jitter(r, 1, 0.06), child(r, 'end1'), 0.11, 0.002, 7, 0.08)
    end2 = kit.thunk(165.0 * jitter(r, 1, 0.06), child(r, 'end2'), 0.1, 0.002, 7, 0.08)
    thud = kit.noise_thump(0.12, child(r, 'thud'), 700.0, 100.0, 0.05, 1.0, 0.04)
    rattle = kit.jingle(0.15, child(r, 'rattle'), 2, 1200.0, 3600.0, 0.03, 0.05, 0.0)
    plank = layers((end1, 0.0, 0.0), (thud, 0.0, -5.0), (end2, 0.035 + 0.01 * r.random(), -2.0), (rattle, 0.04, -18.0))
    a, b = [('D5', 'A5'), ('E5', 'B5')][v]
    n1 = kit.chime(kit.note(a), child(r, 'n1'), 0.45, 0.5, 0.0003, 0.5, -36.0)
    n2 = kit.chime(kit.note(b), child(r, 'n2'), 0.7, 0.5, 0.0003, 0.5, -36.0)
    x = layers((counted, 0.0, 0.0), (plank, t, -1.0), (n1, t + 0.08, -12.0), (n2, t + 0.18, -12.0))
    return _store(calm(x, 60.0), r, -19.0, 0.32)


def _page_turn(r, dur=0.24):
    """A ledger's page turned: its corner lifted with a crackle, the leaf swinging over on a breath of air, fluttering,
    and laid down flat."""
    lift = crackle(0.04, child(r, 'lift'), 900.0, 1000.0, 6000.0, 30.0) * E.ar(0.04, 0.003, 0.03, 1.5)
    n = ns(dur)
    air = kit.whoosh(dur, child(r, 'air'), 260.0, 1300.0, 0.8, 0.55, -3.0)
    air = air * (0.75 + 0.25 * np.abs(N.smooth_random(n, child(r, 'flutter'), 26.0)))
    down = flap(child(r, 'down'), 0.08, 700.0, 160.0, -10.0)
    return layers((normalize(lift), 0.0, -4.0), (air, 0.02, -6.0), (down, dur - 0.02, -3.0))


def _pen(r, strokes=((0.06, 1.0), (0.1, 0.8))):
    """A steel nib scratching across paper: the nib's stick-slip (a fast, rough train of tiny catches) rung through its
    few thin resonances, a stroke at a time. Soft: it's under the HUD's line."""
    out = np.zeros(1)
    t = 0.0
    for i, (d, level) in enumerate(strokes):
        n = ns(d)
        rate = O.expsweep(r.uniform(380.0, 520.0), r.uniform(260.0, 360.0), d, 1.0, n=n)
        rate = rate * (1.0 + 0.3 * N.smooth_random(n, child(r, 'wob', i), 60.0))
        ph = O.phase(rate, n)
        edges = np.nonzero(np.diff(np.floor(ph)) > 0)[0] + 1
        pulses = np.zeros(n)
        pulses[edges] = r.uniform(0.3, 1.0, edges.size)
        y = np.zeros(n)
        for c, q in ((1500.0, 3.0), (2900.0, 3.5), (5600.0, 3.0)):
            y += F.filt(pulses, F.bp(c * jitter(r, 1, 0.05), q), extend=False)
        grain = N.band(n, child(r, 'grain', i), 1500.0, 7000.0)
        stroke = (normalize(y) + 0.3 * normalize(grain)) * E.ar(d, 0.008, d * 0.5, 1.5)
        out = add_at(out, normalize(stroke) * level, t)
        t += d + r.uniform(0.03, 0.05)
    return normalize(out, 0.0)


@cue('UI.LostProperty.Sent', variations=2, space='2D', cls='Interface', jitter=0.02, conc=1, level=-18.0)
def sent(v, r):
    # Your guns went to Lost Property: a ledger's page turned, a quick scratch of the clerk's pen entering them, and far
    # off a station bell rung twice. Soft, informative, under the HUD's line.
    page = _page_turn(child(r, 'page'))
    pen = _pen(child(r, 'pen'))
    pen = F.filt(pen, F.peak(3300.0, -7.0, 0.8), F.highshelf(6000.0, -6.0), F.lp(7500.0, 0.7), extend=False)
    near = layers((page, 0.0, 0.0), (pen, 0.3 + 0.03 * r.random(), -10.0))
    near = _store(calm(near, 120.0), r, -24.0, 0.25, 0.2)
    prime = [990.0, 935.0][v]
    bells = layers((_bell(child(r, 'b1'), prime, 1.6, 1.8, 1.4, 0.8, 0.0002, -16.0), 0.0, 0.0),
                   (_bell(child(r, 'b2'), prime, 1.6, 1.8, 1.4, 0.8, 0.0002, -16.0), 0.33, -1.5))
    bells = F.filt(normalize(bells), F.lp(2600.0, 0.7), F.hp(300.0), extend=False)
    dry = S.widen(bells, child(r, 'bw'), 0.3)
    ir = R.stereo_ir(R.open_air, child(r, 'ir1'), child(r, 'ir2'), t60=1.8, lp=2400.0, predelay=0.04, density=0.7)
    wet = F.convolve(F.filt(bells, F.hp(300.0), extend=False), ir)
    far = layers(dry, kit.set_level(wet, dry, -4.0, 0.05))
    return layers((near, 0.0, 0.0), (far, 0.55 + 0.05 * r.random(), -15.0))
