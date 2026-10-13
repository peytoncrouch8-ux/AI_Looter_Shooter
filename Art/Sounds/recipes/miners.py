"""The Providence's dead (wave 2c): the dead miners (ADeadMinerCreature) at work and in a fight, and the two dead friends in
Ransom's Rest, Thaddeus Bright at his bench and Hiram Coyle at his speaking tube. All 3D.

The dead miners are the Unpaid's kin, so their voices are the Unpaid's (creatures.py's choir of breathy formant voices, a
man's register, their cold ring), working men's grunts rather than wails. What they carry is a mine's: a steel pick (a
forged head, a free bar's stretched partials, dull and short; an ash haft that knocks and creaks), a carbide lamp on the
cap (the acetylene jet's hiss, a brass body, a glass lens) and a tin lunch pail. Rock struck by a pick cracks and chips:
a hard crack, a heavy low thud, small stones skittering. The galleries are bare rock: a hit there gets dense, close
reflections and a short, dark wash (world.py's Sink, smaller). Code plays the voices at each miner's own pitch.

- Creature.DeadMiner.Heave (on it, the 0.8 s wind-up): a grunt heaving the pick overhead, rising with effort, the haft
  creaking, the lamp's hiss rising.
- Creature.DeadMiner.Swing (on it, at the strike): the pick's heavy, short rush of air.
- Creature.DeadMiner.PickBite (carried): the pick biting the floor: an iron chunk into rock, a crack, chips skittering.
- Creature.DeadMiner.PickWall (quieter, across a gallery): one blow at the wall, a dull iron ping on rock.
- Creature.DeadMiner.LampBreak: the lamp shot out: the lens popping, the gas's hiss dying, glass tinkling down.
- Creature.DeadMiner.LampStruck: a bullet off a dead lamp: a small bright tink on its brass.
- Creature.DeadMiner.Reel: its swing broken: a choked gasp, the haft clattering in its hands.
- Creature.DeadMiner.Pail (quiet): the lunch pail's tin lid lifted and set back down.
- Creature.DeadMiner.Drop: the pick falling from its hands: iron clattering on stone, a bounce, the haft settling.
- World.Thaddeus.Knock (quiet, through the cold-room door): two knocks of knuckles on a coffin board.
- World.Thaddeus.Tape (quiet): a cloth tape drawn out of its case, then let slide back in.
- World.Hiram.Tube (quiet): the speaking tube's whistle cap pulled, a hollow breath down the pipe.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, add_at
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, voice as V
from lib import granular as G
from recipes import cue
from recipes import kit
from recipes.creatures import _choir, _cold
from recipes.bosses import _debris, _snaps
from recipes.givers import rub


# --- The mine ------------------------------------------------------------------------------------------------------------

def gallery(x, r, rel_db=-12.0, t60=0.9, size=2.4):
    """A rock gallery round a sound: bare rock throwing it straight back (dense, close reflections) and a short, dark
    wash. foreman.py uses it too."""
    ir = R.room(child(r, 'gallery'), t60=t60, size=size, damp=0.3)
    wet = F.convolve(F.filt(x, F.hp(150.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


def pick_head(r, f0=760.0, t60=0.12):
    """The pick's forged steel head struck: a free bar's stretched partials, dull and short (the haft damps it)."""
    m = M.bar(f0 * jitter(r, 1, 0.06), child(r, 'head'), t60, 0.7, 5, 0.01, 0.8)
    ring = normalize(M.strike(m, M.hammer(0.00012), t60 * 2.0 + 0.02))
    return F.filt(ring, F.peak(3300.0, -5.0, 0.8), F.highshelf(6000.0, -5.0), extend=False)


def rock_hit(r, weight=1.0):
    """Steel biting rock: the rock's hard crack, a heavy low thud of the mass behind it, and a few brittle snaps."""
    crack = kit.burst(0.008, child(r, 'crack'), 0.003, lo=600.0, hi=6000.0, attack=0.0002)
    thud = kit.noise_thump(0.22, child(r, 'thud'), 700.0, 70.0, 0.1, 1.0, 0.05, 0.0006)
    knock = kit.thunk(380.0 * jitter(r, 1, 0.1), child(r, 'knock'), 0.04, 0.0003, 6, 0.12)
    snaps = _snaps(0.2, child(r, 'snaps'), lambda t: 60.0 * np.exp(-t / 0.05), 600.0, 3000.0, 0.015)
    return normalize(layers((crack, 0.0, -3.0), (thud, 0.0, -2.0 + 3.0 * (weight - 1.0)), (knock, 0.0, -5.0),
                            (normalize(snaps), 0.002, -10.0)), 0.0)


def _haft(r, f0=420.0):
    """The ash haft knocking in a grip or on stone: a dull knock of hard wood."""
    return kit.thunk(f0 * jitter(r, 1, 0.08), child(r, 'haft'), 0.06, 0.0008, 6, 0.1)


def _carbide(dur, r, rise=True):
    """A carbide lamp's flame: the acetylene jet hissing through its tip (a narrow, fluttering hiss) with a little roar."""
    n = ns(dur)
    hiss = N.band(n, child(r, 'jet'), 2200.0, 7000.0) * (0.75 + 0.25 * np.abs(N.smooth_random(n, child(r, 'fl'), 45.0)))
    roar = N.band(n, child(r, 'roar'), 200.0, 900.0) * (0.6 + 0.4 * np.abs(N.smooth_random(n, child(r, 'rf'), 16.0)))
    x = normalize(hiss) + 0.4 * normalize(roar)
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), extend=False)
    env = E.bp([(0.0, 0.2), (dur * 0.9, 1.0), (dur, 0.0)], n=n, curve='cos') if rise else E.ar(dur, 0.02, dur * 0.5)
    return normalize(x * env, 0.0)


# --- A dead miner in a fight --------------------------------------------------------------------------------------------

@cue('Creature.DeadMiner.Heave', variations=3, att='Creature', cls='Effects', jitter=0.04, conc=3, level=-8.0)
def heave(v, r):
    # The wind-up (0.8 s, then the strike): the pick heaved overhead with a dead man's grunt that strains upward to the
    # top of the swing, the haft creaking in its hands, the lamp on its cap hissing louder as its head tips back.
    dur = 0.8
    f = [118.0, 108.0, 128.0][v] * jitter(r, 1, 0.03)
    grunt = _choir(child(r, 'grunt'), [(0.0, f * 0.9), (0.5, f * 1.1), (dur, f * 1.22)], dur,
                   [(0.0, 'uh'), (0.35, 'a'), (dur, 'ae')], 2, 14.0, 0.015, 0.5, 0.95, (5.0, 0.15), 0.45, 1.0)
    grunt = _cold(grunt, child(r, 'cold'), 31.0, 0.18)
    grunt = grunt * E.bp([(0.0, 0.0), (0.01, 0.2), (0.4, 0.5), (dur - 0.05, 1.0), (dur, 0.0)], n=grunt.size, curve='cos')
    creak = kit.creak(0.55, child(r, 'haft'), 30.0, 60.0, (340.0 * jitter(r, 1, 0.1), 820.0, 1600.0), 7.0, 0.3)
    lamp = _carbide(dur, child(r, 'lamp'))
    knock = _haft(child(r, 'grip'))
    x = layers((knock, 0.0, -10.0), (grunt, 0.0, 0.0), (creak, 0.08, -14.0), (lamp, 0.0, -20.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(80.0), extend=False)
    return gallery(x, r, -14.0, 0.8)


@cue('Creature.DeadMiner.Swing', variations=3, att='Creature', cls='Effects', jitter=0.06, conc=3, level=-10.0)
def swing(v, r):
    # The pick coming down: a heavy, short rush of air (a big, slow blade: low and dark), its head's whistle on top.
    rush = kit.whoosh(0.24, child(r, 'rush'), 260.0, 900.0, 1.0, 0.3, -4.0, 1.0)
    head = kit.whoosh(0.18, child(r, 'head'), 900.0, 1800.0, 3.0, 0.35, -2.0, 1.0)
    shove = kit.noise_thump(0.15, child(r, 'shove'), 400.0, 90.0, 0.06, 0.9, 0.06, 0.002)
    x = layers((rush, 0.0, 0.0), (head, 0.01, -12.0), (shove, 0.0, -9.0))
    return F.filt(x, F.hp(70.0), F.peak(3200.0, -3.0, 0.8), extend=False)


@cue('Creature.DeadMiner.PickBite', variations=4, att='Creature', cls='Effects', jitter=0.05, conc=3, level=-6.0)
def pick_bite(v, r):
    # The pick biting the floor: a heavy iron chunk into rock (the point's crack, the mass's thud, the head's short dull
    # ring), stone chips skittering away, a little grit pattering after, the gallery throwing it back.
    hit = rock_hit(child(r, 'hit'), 1.2)
    head = pick_head(child(r, 'head'), 720.0, 0.1)
    chips = _debris(0.5, child(r, 'chips'), 9, 0.02, 0.35, 1200.0, 4500.0)
    grit = kit.grit_burst(0.35, child(r, 'grit'), 3000.0, 700.0, 3500.0, 0.15)
    x = layers((hit, 0.0, 0.0), (head, 0.001, -8.0), (normalize(chips), 0.015, -13.0), (grit, 0.005, -15.0))
    x = F.filt(x, F.hp(45.0), F.peak(3300.0, -4.0, 0.8), extend=False)
    return gallery(x, r, -11.0, 0.9)


@cue('Creature.DeadMiner.PickWall', variations=4, att='Creature', cls='Effects', jitter=0.04, conc=3, level=-14.0)
def pick_wall(v, r):
    # A dead miner still working a wall, unaware: one blow, the pick's dull iron ping on rock ringing longer than in a
    # fight (it's struck square), a few chips falling, the gallery carrying it.
    hit = rock_hit(child(r, 'hit'), 0.8)
    head = pick_head(child(r, 'head'), [820.0, 760.0, 880.0, 790.0][v], 0.22)
    chips = _debris(0.4, child(r, 'chips'), 5, 0.02, 0.3, 1300.0, 4500.0)
    x = layers((hit, 0.0, -3.0), (head, 0.001, -3.0), (normalize(chips), 0.02, -16.0))
    x = F.filt(x, F.hp(80.0), F.peak(3300.0, -4.0, 0.8), F.highshelf(7000.0, -4.0), extend=False)
    return gallery(x, r, -7.0, 1.2, 3.0)


@cue('Creature.DeadMiner.LampBreak', variations=3, att='Creature', cls='Effects', jitter=0.05, conc=3, level=-9.0)
def lamp_break(v, r):
    # Its head lamp shot out: the lens popping (a hard little pop and the glass cracking), the acetylene's hiss
    # sputtering and dying, small shards tinkling down onto the rock.
    pop = layers((kit.burst(0.006, child(r, 'pop'), 0.002, lo=900.0, hi=8000.0, attack=0.0002), 0.0, 0.0),
                 (kit.noise_thump(0.08, child(r, 'puff'), 1200.0, 250.0, 0.03, 1.0, 0.03, 0.0005), 0.0, -1.0))

    def shard(rr, i):
        m = M.parts(1900.0, 6500.0, rr, 4, 0.05, 0.02, 0.3)
        s = normalize(M.strike(m, M.hammer(0.00004), 0.1))
        return F.filt(s, F.peak(3300.0, -9.0, 0.7), extend=False) * rr.uniform(0.5, 1.0)
    crack = normalize(G.rattle(0.05, child(r, 'crack'), 4, 0.03, shard, 0.0, 0.8))
    tinkle = normalize(G.rattle(0.3, child(r, 'tinkle'), 6, 0.35, shard, 0.12, 0.8))
    n = ns(0.7)
    hiss = _carbide(0.7, child(r, 'gas'), rise=False)
    gate = np.clip(np.abs(N.smooth_random(n, child(r, 'sputter'), 22.0)) * 2.0 - np.linspace(0.0, 1.6, n), 0.0, 1.0)
    hiss = hiss * F.filt(gate, F.lp1(80.0), extend=False) * E.perc(0.7, 0.005, 0.5)
    x = layers((pop, 0.0, 0.0), (crack, 0.001, -8.0), (normalize(hiss), 0.01, -13.0), (tinkle, 0.0, -16.0))
    x = F.filt(x, F.hp(100.0), F.highshelf(8000.0, -4.0), extend=False)
    return gallery(x, r, -16.0, 0.8)


@cue('Creature.DeadMiner.LampStruck', variations=4, att='Creature', cls='Effects', jitter=0.06, conc=4, level=-12.0)
def lamp_struck(v, r):
    # A bullet off a lamp already out: a small bright tink on its thin brass body, a tiny rattle of the reflector.
    body = normalize(M.strike(M.shell(1350.0 * jitter(r, 1, 0.08), child(r, 'brass'), 0.18, 7, 0.02), M.hammer(0.00005), 0.3))
    body = F.filt(body, F.peak(3300.0, -6.0, 0.8), F.highshelf(7000.0, -3.0), extend=False)
    tick = kit.metal_click(child(r, 'tick'), 5500.0, 9500.0, 0.01, 0.00004, 4, -12.0)
    rattle = kit.jingle(0.1, child(r, 'rattle'), 2, 1800.0, 4500.0, 0.02, 0.05, 0.01)
    x = layers((body, 0.0, 0.0), (tick, 0.0, -10.0), (rattle, 0.01, -18.0))
    return gallery(x, r, -18.0, 0.7)


@cue('Creature.DeadMiner.Reel', variations=3, att='Creature', cls='Effects', jitter=0.05, conc=3, level=-8.0)
def reel(v, r):
    # The swing broken by a shot on its lamp: a choked gasp, the pick's haft clattering in its hands, the head knocking
    # once against it as it fumbles.
    gasp = V.whisper(0.22, [(0.0, 'h'), (0.1, 'a'), (0.22, 'uh')], child(r, 'gasp'), 0.92)
    gasp = gasp * E.bp([(0.0, 0.0), (0.004, 0.4), (0.09, 1.0), (0.22, 0.0)], n=gasp.size, curve='cos')
    f = [112.0, 104.0, 122.0][v]
    choke = V.voice(O.glide([(0.0, f * 1.15), (0.12, f * 0.9)], n=ns(0.12)), [(0.0, 'uh'), (0.12, 'uh')], child(r, 'choke'),
                    breath=0.55, tilt=-1.0, shift=0.95, rough=0.4) * E.perc(0.12, 0.005, 0.09)
    choke = _cold(choke, child(r, 'cold'), 29.0, 0.2)
    out = np.zeros(1)
    t = 0.12
    for i in range(3):
        out = layers(out, (_haft(child(r, 'k', i), 400.0 + 60.0 * i), t, -2.0 * i))
        t += r.uniform(0.07, 0.13)
    head = pick_head(child(r, 'head'), 700.0, 0.08)
    x = layers((normalize(gasp), 0.0, 0.0), (normalize(choke), 0.16, -6.0), (out, 0.0, -5.0),
               (head, 0.2 + 0.05 * r.random(), -11.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(90.0), extend=False)
    return gallery(x, r, -14.0, 0.8)


def _tin(r, f0=780.0, t60=0.12, contact=0.0002):
    """The pail's tin lid: a thin sheet's dense modes, a short dull ring."""
    m = M.plate(f0 * jitter(r, 1, 0.06), child(r, 'tin'), 16, t60, 0.6, tilt=0.8)
    ring = normalize(M.strike(m, M.hammer(contact), t60 * 1.6 + 0.02))
    return F.filt(ring, F.peak(3300.0, -6.0, 0.8), F.highshelf(6000.0, -6.0), extend=False)


@cue('Creature.DeadMiner.Pail', variations=3, att='Near', cls='Effects', jitter=0.04, conc=2, level=-18.0)
def pail(v, r):
    # At its lunch pail, out of habit: the tin lid prised up (a scrape and a light clank), a moment, then set back down
    # with a duller clank and a little rattle.
    scrape = kit.scrape(0.06, child(r, 'scrape'), 1200.0, 1500.0, 6.0, 0.6, (1.0, 1.6), 0.2, 0.5, 2500.0, -24.0)
    lift = layers((_tin(child(r, 'lift'), 820.0, 0.1, 0.0003), 0.0, 0.0), (scrape, 0.0, -12.0))
    t = 0.45 + 0.15 * r.random()
    down = _tin(child(r, 'down'), 760.0, 0.14, 0.0006)
    rattle = kit.jingle(0.12, child(r, 'rattle'), 2, 1200.0, 3500.0, 0.03, 0.05, 0.01)
    x = layers((lift, 0.0, -3.0), (down, t, 0.0), (rattle, t + 0.01, -14.0))
    x = F.filt(x, F.hp(120.0), extend=False)
    return gallery(x, r, -18.0, 0.7)


@cue('Creature.DeadMiner.Drop', variations=3, att='Creature', cls='Effects', jitter=0.05, conc=3, level=-10.0)
def drop(v, r):
    # Its pick falling as it dies: the iron head clattering onto stone (a clank and the rock's knock), the haft slapping
    # down after it, one bounce of the head, and a scrape as it settles.
    def hit(rr, weight):
        head = pick_head(child(rr, 'head'), 740.0, 0.1)
        stone = kit.thunk(330.0 * jitter(rr, 1, 0.1), child(rr, 'stone'), 0.04, 0.0003, 6, 0.12)
        thud = kit.noise_thump(0.12, child(rr, 'thud'), 800.0, 120.0, 0.05, 1.0, 0.04, 0.0005)
        return normalize(layers((head, 0.0, 0.0), (stone, 0.0, -4.0), (thud, 0.0, -5.0 + 3.0 * weight)))
    first = hit(child(r, 'first'), 1.0)
    slap = _haft(child(r, 'slap'), 360.0)
    b = 0.12 + 0.04 * r.random()
    bounce = hit(child(r, 'bounce'), 0.3)
    settle = kit.scrape(0.12, child(r, 'settle'), 900.0, 700.0, 5.0, 0.7, (1.0, 1.6), 0.1, 0.6, 1500.0, -26.0)
    settle = F.filt(settle, F.lp(2500.0, 0.7), extend=False)
    x = layers((first, 0.0, 0.0), (slap, 0.05 + 0.02 * r.random(), -4.0), (bounce, b, -9.0), (settle, b + 0.06, -16.0))
    x = F.filt(x, F.hp(70.0), F.peak(3300.0, -4.0, 0.8), extend=False)
    return gallery(x, r, -12.0, 0.9)


# --- The dead friends in Ransom's Rest ----------------------------------------------------------------------------------

@cue('World.Thaddeus.Knock', variations=3, att='Creature', cls='Effects', jitter=0.04, conc=1, level=-16.0)
def thaddeus_knock(v, r):
    # Thaddeus at his bench, heard through the cold-room door as the cellar's knocking: two knocks of his knuckles on a
    # coffin board, the box answering hollow, muffled by the door and carried round the cellar's stone.
    out = np.zeros(1)
    t = 0.0
    for i in range(2):
        board = kit.thunk(230.0 * jitter(r, 1, 0.05) * (1.0 - 0.03 * i), child(r, 'board', i), 0.07, 0.0016, 6, 0.08)
        box = kit.thump(120.0 * jitter(r, 1, 0.05), 105.0, 0.14, 0.09, attack=0.002)
        out = layers(out, (board, t, -1.0 * i), (box, t, -7.0 - 1.0 * i))
        t += 0.2 + 0.05 * r.random()
    x = F.filt(out, F.lp(1500.0, 0.6), F.hp(60.0), extend=False)
    return gallery(x, r, -9.0, 0.8, 1.6)


@cue('World.Thaddeus.Tape', variations=2, att='Near', cls='Effects', jitter=0.04, conc=1, level=-19.0)
def thaddeus_tape(v, r):
    # Thaddeus's cloth measuring tape: drawn out of its leather case in one long pull (cloth whispering over the slot,
    # the case's spindle ticking), held, then let go and wound back in a quick whirr, ending in a soft snap home.
    d = 0.55 + 0.1 * r.random()
    pull = rub(d, child(r, 'pull'), 400.0, 4000.0, 1200.0, 0.15, 0.4, 40.0)
    ticks = np.zeros(1)
    for k, at in enumerate(np.cumsum(r.uniform(0.07, 0.11, 6))):
        if at < d:
            ticks = layers(ticks, (kit.metal_click(child(r, 'tick', k), 1500.0, 4000.0, 0.01, 0.0002, 3, -16.0), at, -6.0))
    t = d + 0.35 + 0.1 * r.random()
    n = ns(0.3)
    rate = O.expsweep(18.0, 40.0, 0.3, 1.0, n=n)
    clicks = np.zeros(n)
    edges = np.nonzero(np.diff(np.floor(O.phase(rate, n))) > 0)[0] + 1
    clicks[edges] = 1.0
    whirr = F.filt(clicks, F.bp(2200.0, 3.0), extend=False) + 0.4 * F.filt(clicks, F.bp(700.0, 2.0), extend=False)
    whirr = layers((normalize(whirr), 0.0, -6.0), (rub(0.3, child(r, 'back'), 500.0, 4500.0, 2000.0, 0.1, 0.3, 60.0), 0.0, 0.0))
    snap = kit.thunk(650.0 * jitter(r, 1, 0.06), child(r, 'snap'), 0.03, 0.0008, 5, 0.06)
    # The tape's brass tab pinched first, so the cue speaks at once.
    tab = kit.metal_click(child(r, 'tab'), 1300.0, 3800.0, 0.015, 0.0002, 3, -14.0)
    x = layers((tab, 0.0, -6.0), (pull, 0.0, 0.0), (normalize(ticks), 0.02, -14.0), (whirr, t, -2.0), (snap, t + 0.3, -4.0))
    x = F.filt(x, F.hp(150.0), F.peak(3300.0, -5.0, 0.8), F.highshelf(7000.0, -4.0), extend=False)
    return gallery(x, r, -22.0, 0.4, 0.8)


@cue('World.Hiram.Tube', variations=2, att='Near', cls='Effects', jitter=0.03, conc=1, level=-17.0)
def hiram_tube(v, r):
    # Hiram leaning to his speaking tube: the brass whistle cap pulled out of its mouth (a soft suction pop and a tick of
    # brass), then a long breath down the pipe, hollow, the tube's resonances ringing it like a far-off voice.
    pop = kit.noise_thump(0.05, child(r, 'pop'), 1500.0, 350.0, 0.015, 1.4, 0.02, 0.0004)
    tick = kit.brass_tick(child(r, 'cap'), 1400.0 * jitter(r, 1, 0.04), 0.03, 4, 0.0001, -12.0, -10.0, 0.2)
    tick = F.filt(tick, F.peak(3300.0, -5.0, 0.8), extend=False)
    d = 1.1 + 0.2 * r.random()
    breath = V.whisper(d, [(0.0, 'h'), (d * 0.4, 'u'), (d, 'h')], child(r, 'breath'), 0.95)
    breath = breath * E.bp([(0.0, 0.0), (0.12, 1.0), (d * 0.7, 0.8), (d, 0.0)], n=breath.size, curve='cos')
    # A brass pipe some metres long: its resonances a little under 100 Hz apart, the breath sounding through them.
    pipe = F.filt(breath, F.comb(1.0 / (88.0 * jitter(r, 1, 0.05)), 0.82, 2500.0), F.lp(3000.0, 0.7), F.hp(150.0),
                  extend=False)[:breath.size]
    x = layers((pop, 0.0, -2.0), (tick, 0.01, -6.0), (normalize(pipe), 0.18, -3.0), (normalize(breath), 0.18, -14.0))
    return gallery(x, r, -20.0, 0.5, 1.0)
