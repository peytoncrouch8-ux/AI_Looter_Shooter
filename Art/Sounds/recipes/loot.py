"""Loot and the gunsmith's bench: ammo and gun pickups, loot landing, chests opening, the Strongbox's vault wheel; the
bench's scrapping and fitting. Physical first (brass rounds, steel, wood, cloth), with a soft gleam where the moment
should feel like a reward.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, mono, reverse
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, stereo as S
from lib import granular as G, dynamics as Y, voice as V
from recipes import cue
from recipes import kit


def _workshop(x, r, wet_db=-17.0):
    """The bench's room: a small timber workshop around a 2D sound, in stereo."""
    dry = S.widen(x, child(r, 'wide'), 0.3)
    ir = R.stereo_ir(R.room, child(r, 'ir1'), child(r, 'ir2'), t60=0.45, size=1.0, damp=0.4)
    wet = F.convolve(F.filt(x, F.hp(200.0), extend=False), ir)
    return layers(dry, kit.set_level(wet, dry, wet_db, 0.02))


def _outside(x, r, rel_db=-22.0):
    ir = R.outdoor(child(r, 'ir'), dur=0.6, slaps=((0.06, -4.0), (0.15, -9.0)), tail_t60=0.4, tail_db=-12.0,
                   tail_start=0.02, tail_peak=0.07)
    wet = F.convolve(F.filt(x, F.hpn(250.0, 2), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.02))


def _ratchet(r, count, rate0, rate1, lo=2400.0, hi=7500.0, body_f=700.0, stiffen=0.0):
    """A wrench's ratchet: clicks at a rate gliding from rate0 to rate1; stiffen > 0 makes later clicks heavier."""
    out = np.zeros(1)
    t = 0.0
    for i in range(count):
        u = i / max(count - 1, 1)
        click = kit.metal_click(child(r, 'c', i), lo, hi, 0.018, 0.00006, 4, -12.0)
        body = kit.thunk(body_f * (1.0 - 0.15 * stiffen * u), child(r, 'b', i), 0.025, 0.0003, 4, 0.06)
        out = layers(out, (layers(click, (body, 0.0, -6.0 + 4.0 * stiffen * u)), t, -2.0 * (1 - u) * (1 - stiffen)))
        t += 1.0 / (rate0 + (rate1 - rate0) * u) * r.uniform(0.9, 1.1)
    return out


def _drop_in_box(r):
    """A steel part dropped into a tin parts box: the clank, two bounces, loose bits rattling."""
    box = M.plate(330.0 * jitter(r, 1, 0.08), child(r, 'box'), 20, 0.35, 0.6)
    part = M.bar(950.0 * jitter(r, 1, 0.1), child(r, 'part'), 0.3, 0.6, 5)
    out = np.zeros(1)
    t = 0.0
    for i, (gap, db) in enumerate([(0.0, 0.0), (0.075, -7.0), (0.05, -13.0)]):
        t += gap * jitter(r, 1, 0.15)
        hit = layers(normalize(M.strike(box, M.hammer(0.0001), 0.5)), (normalize(M.strike(part, M.hammer(0.00006), 0.4)), 0.0, -3.0))
        out = layers(out, (normalize(hit), t, db))
    bits = kit.jingle(0.3, child(r, 'bits'), 5, 2000.0, 7000.0, 0.04, 0.12, 0.02)
    return layers(out, (bits, 0.02, -16.0))


@cue('Bench.Scrap', variations=3, space='2D', cls='Effects', jitter=0.03, conc=2, level=-10.0)
def bench_scrap(v, r):
    # The wrench backs the part's screws out, the part comes free with a scrape, and drops into the parts box.
    ratchet = _ratchet(child(r, 'ratchet'), 8 + int(r.integers(0, 4)), 22.0, 30.0)
    free = kit.scrape(0.07, child(r, 'free'), 1500.0, 900.0, 12.0, 0.5)
    t = 0.36 + 0.04 * r.random()
    drop = _drop_in_box(child(r, 'drop'))
    x = layers((ratchet, 0.0, -4.0), (free, t, -12.0), (drop, t + 0.2, 0.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 1.0), extend=False)
    return _workshop(x, r)


@cue('Bench.Fit', variations=3, space='2D', cls='Effects', jitter=0.03, conc=2, level=-10.0)
def bench_fit(v, r):
    # The part slides home and seats, the screws are run tight (each click stiffer), it locks, and a small brass
    # ring says it's right.
    slide = kit.scrape(0.12, child(r, 'slide'), 900.0, 1350.0, 12.0, 0.5, attack=0.08, release=0.3)
    seat = kit.clack(child(r, 'seat'), 600.0, 1400.0, 5500.0, 0.05, 0.06, -1.0, -6.0)
    ratchet = _ratchet(child(r, 'ratchet'), 5 + int(r.integers(0, 2)), 14.0, 10.0, 2000.0, 6500.0, 600.0, 1.0)
    lock = kit.clack(child(r, 'lock'), 430.0 * jitter(r, 1, 0.05), 1200.0, 5000.0, 0.08, 0.09, 1.0, -2.0, 0.00012)
    ring = kit.chime(kit.note(['E6', 'D6', 'F#6'][v]), child(r, 'ring'), 0.5, 0.6, 0.0001)
    t1 = 0.13
    t2 = t1 + 0.12
    t3 = t2 + 0.45 + 0.04 * r.random()
    x = layers((slide, 0.0, -12.0), (seat, t1, -3.0), (ratchet, t2, -5.0), (lock, t3, 0.0), (ring, t3 + 0.02, -15.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 1.0), extend=False)
    return _workshop(x, r)


@cue('Loot.AmmoPickup', variations=4, att='Near', jitter=0.05, conc=4, level=-12.0)
def ammo_pickup(v, r):
    # A handful of rounds scooped up: brass cartridges clinking together, the pouch taking them.
    # Rounds ring high and bright; they're kept short and their top softened, since pickups come by the dozen.
    def shell(rr, i):
        m = M.shell(rr.uniform(1150.0, 1800.0), rr, rr.uniform(0.04, 0.07), 5)
        m = (m[0], m[1], m[2] * np.array([1.0, 0.45, 0.35, 0.25, 0.2]))
        return normalize(M.strike(m, M.hammer(0.00007), 0.12))
    rounds = G.rattle(0.2, child(r, 'rounds'), 6 + int(r.integers(0, 5)), 0.09, shell, 0.0, 0.88)
    rounds = F.filt(normalize(rounds), F.peak(3200.0, -7.0, 0.6), F.highshelf(7000.0, -4.0), extend=False)
    pouch = kit.burst(0.05, child(r, 'pouch'), 0.03, lo=150.0, hi=1400.0, color=-3.0)
    swish = kit.cloth(0.12, child(r, 'swish'), 300.0, 2500.0, 160.0, 0.2, 0.6)
    box = kit.thunk(520.0 * jitter(r, 1, 0.1), child(r, 'box'), 0.04, 0.0005)
    return layers((swish, 0.0, -14.0), (rounds, 0.005, 0.0), (box, 0.004, -6.0), (pouch, 0.07, -6.0))


@cue('Loot.GunPickup', variations=3, att='Near', jitter=0.04, conc=2, level=-11.0)
def gun_pickup(v, r):
    # Taking up a gun: the hand closes on it, it's swung up (cloth, the sling's buckle), and settles with its weight.
    grab = kit.clack(child(r, 'grab'), 600.0 * jitter(r, 1, 0.06), 1500.0, 5500.0, 0.05, 0.06, -1.0, -6.0)
    grab = F.filt(grab, F.peak(3300.0, -3.0, 1.0), extend=False)
    swish = kit.cloth(0.25, child(r, 'swish'), 300.0, 2600.0, 140.0, 0.25, 0.5)
    jing = kit.jingle(0.3, child(r, 'jingle'), 3 + int(r.integers(0, 3)), 2200.0, 7000.0, 0.05, 0.1, 0.04)
    weight = kit.noise_thump(0.12, child(r, 'weight'), 500.0, 90.0, 0.06, 1.0, 0.05)
    settle = kit.clack(child(r, 'settle'), 480.0, 1300.0, 5000.0, 0.04, 0.05, 0.0, -6.0)
    t = 0.2 + 0.03 * r.random()
    return layers((grab, 0.0, 0.0), (swish, 0.01, -8.0), (jing, 0.02, -15.0), (weight, t, -6.0), (settle, t + 0.01, -5.0))


@cue('Loot.Land', variations=4, att='Near', jitter=0.06, conc=6, level=-14.0)
def loot_land(v, r):
    # Something tossed lands in the dirt: a soft thud, a puff of grit, a tick or two of metal.
    thud = kit.noise_thump(0.12, child(r, 'thud'), 700.0, 110.0, 0.05, 1.0, 0.04)
    knock = kit.thunk(420.0 * jitter(r, 1, 0.15), child(r, 'knock'), 0.04, 0.0008)
    grit = kit.grit_burst(0.18, child(r, 'grit'), 3000.0, 900.0, 4500.0, 0.1)
    ticks = kit.jingle(0.2, child(r, 'ticks'), 2 + int(r.integers(0, 2)), 1800.0, 6000.0, 0.03, 0.08, 0.01)
    bounce = 0.07 + 0.03 * r.random()
    x = layers((thud, 0.0, 0.0), (knock, 0.0, -6.0), (grit, 0.002, -12.0), (ticks, 0.0, -16.0),
               (thud, bounce, -12.0), (knock, bounce, -16.0))
    return _outside(F.filt(x, F.hp(70.0), extend=False), r, -24.0)


@cue('Loot.ChestOpen', variations=3, att='Near', jitter=0.03, conc=2, level=-8.0)
def chest_open(v, r):
    # The hasp unclasps, the old hinges creak as the lid swings, the lid knocks back on its stop, and a soft gleam
    # rises from what's inside.
    latch1 = kit.clack(child(r, 'l1'), 700.0, 1400.0, 5000.0, 0.04, 0.05, -3.0, -10.0)
    latch2 = kit.metal_click(child(r, 'l2'), 1600.0, 5000.0, 0.05, 0.0001)
    creak = kit.creak(0.5 + 0.1 * r.random(), child(r, 'creak'), 28.0 + 8 * r.random(), 85.0 + 25 * r.random(),
                      (380.0 * jitter(r, 1, 0.1), 920.0, 1750.0), 10.0, 0.3)
    t_lid = 0.68 + 0.08 * r.random()
    lid = kit.thunk(240.0 * jitter(r, 1, 0.08), child(r, 'lid'), 0.14, 0.0015, 7, 0.08)
    lid_air = kit.noise_thump(0.15, child(r, 'lidair'), 900.0, 120.0, 0.07, 1.0, 0.05)
    gleam = np.zeros(1)
    for i, nm in enumerate([['A6', 'E7', 'C#7'], ['G6', 'D7', 'B6'], ['B6', 'F#7', 'D#7']][v]):
        gleam = layers(gleam, (kit.chime(kit.note(nm), child(r, 'g', i), 1.4, 0.5, 0.0002), 0.08 * i, -3.0 * i))
    gleam = normalize(gleam) * E.swell(len(gleam) / SR, 0.12, 1.2)
    air = kit.whoosh(0.8, child(r, 'air'), 800.0, 6000.0, 0.8, 0.35, -2.0)
    x = layers((latch1, 0.0, 0.0), (latch2, 0.06, -6.0), (creak, 0.1, -9.0), (lid, t_lid, -1.0), (lid_air, t_lid, -9.0),
               (normalize(gleam), t_lid - 0.15, -14.0), (air, t_lid - 0.25, -22.0))
    return _outside(x, r, -22.0)


@cue('Loot.StrongboxWheel', variations=2, att='Near', jitter=0.02, conc=1, level=-10.0)
def strongbox_wheel(v, r):
    # The vault wheel spun: its detents click faster and faster, run, slow to a stop, and the bolts draw back with
    # two heavy clunks.
    spin = 1.25 + 0.1 * v
    knots = [(0.0, 5.0), (0.35, 26.0), (0.75, 30.0), (spin, 4.0)]
    tt = np.linspace(0.0, spin, 4000)
    rate = np.interp(tt, [k[0] for k in knots], [k[1] for k in knots])
    count = np.cumsum(rate) * (tt[1] - tt[0])
    times_ = [float(np.interp(k, count, tt)) for k in range(int(count[-1]))]
    clicks = np.zeros(1)
    cr = child(r, 'clicks')
    for i, t in enumerate(times_):
        c = layers(kit.metal_click(cr, 1300.0, 4800.0, 0.03, 0.00008, 5, -14.0), (kit.thunk(260.0, cr, 0.05, 0.0005), 0.0, -5.0))
        clicks = layers(clicks, (c, t, cr.uniform(-2.5, 0.0)))
    n = ns(spin)
    whirr = N.band(n, child(r, 'whirr'), 300.0, 1600.0) * np.interp(times(n), tt, rate / 30.0)
    whirr = F.filt(whirr, F.lp(1200.0), extend=False) * E.ar(spin, 0.1, 0.3)
    t_bolt = spin + 0.12
    b1 = kit.clack(child(r, 'b1'), 300.0, 900.0, 3800.0, 0.1, 0.14, 2.0, 0.0, 0.0002)
    b2 = kit.clack(child(r, 'b2'), 270.0, 850.0, 3600.0, 0.12, 0.16, 2.0, 1.0, 0.0002)
    x = layers((clicks, 0.0, -4.0), (normalize(whirr), 0.0, -20.0), (b1, t_bolt, 0.0), (b2, t_bolt + 0.1, 0.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 1.0), extend=False)
    return _outside(x, r, -22.0)


# --- The fanfare: a gun landing, by rarity --------------------------------------------------------------------------
# One family in E, in the interface's brass and steel string, growing with the rarity: an Uncommon's single glint, a
# Rare's fifth, an Epic's open arpeggio over a dark swell, a Legendary's full sting. Heard often, so the chimes are
# struck softly and kept off the ear's sharpest band.

def _glint(name, r, ring=0.4, bright=0.6, body_db=-9.0, contact=0.0002, hum_db=None):
    """One chime on a note: a small brass chime struck with a soft mallet, a sine on its note under it for a round
    body, and (hum_db) a bell's hum an octave below, so it rings like a little bell rather than clicks."""
    f0 = kit.note(name)
    c = kit.chime(f0, child(r, 'chime'), ring, bright, contact, 0.5, -30.0)
    body = O.sine(f0, n=ns(ring)) * E.perc(ring, 0.003, ring * 0.7)
    x = layers(c, (normalize(body), 0.0, body_db))
    if hum_db is not None:
        hum = O.sine(f0 * 0.5, n=ns(ring * 1.3)) * E.perc(ring * 1.3, 0.004, ring * 1.1)
        x = layers(x, (normalize(hum), 0.0, hum_db))
    return normalize(x, 0.0)


def _fanfare_space(x, r, t60, wet_db):
    """A small bright plate around a 2D fanfare (stereo in, stereo out)."""
    ir = R.stereo_ir(R.plate, child(r, 'ir1'), child(r, 'ir2'), t60=t60)
    wet = F.convolve(F.filt(mono(x), F.hp(300.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, wet_db, 0.05))


@cue('Loot.Drop.Uncommon', variations=3, att='Creature', jitter=0.01, conc=3, level=-17.0)
def drop_uncommon(v, r):
    # A soft glassy glint as it lands: one clear high note struck lightly (E6, its upper partials low, so it's glass
    # more than brass), a tiny tink an octave up and a breath of air over it. Quiet: it comes often.
    glint = _glint('E6', child(r, 'glint'), 0.32, 0.4, -6.0, 0.0003)
    tink = kit.chime(kit.note('E7'), child(r, 'tink'), 0.08, 0.3, 0.0002, 0.5, -40.0)
    air = kit.burst(0.012, child(r, 'air'), 0.005, lo=7000.0, hi=13000.0)
    x = layers((glint, 0.0, 0.0), (tink, 0.0, -18.0), (air, 0.0, -26.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), extend=False)
    return _outside(x, r, -26.0)


@cue('Loot.Drop.Rare', variations=2, att='Creature', jitter=0.0, conc=2, level=-13.0)
def drop_rare(v, r):
    # A rising two-note bell chime, a fifth up (E5, then B5): each a brass chime with a round body and a bell's hum
    # under it, the first warmed by a soft steel string an octave down, in a little plate.
    c1 = _glint('E5', child(r, 'c1'), 0.55, 0.6, -9.0, 0.00025, -14.0)
    c2 = _glint('B5', child(r, 'c2'), 0.75, 0.6, -9.0, 0.00025, -14.0)
    s = kit.twang(kit.note('E4'), 0.8, child(r, 'string'), 0.35, 0.6)
    s = F.filt(s, F.lp(2200.0, 0.7), extend=False)
    x = layers((c1, 0.0, -2.0), (c2, 0.095, 0.0), (s, 0.0, -15.0))
    ir = R.plate(child(r, 'ir'), t60=0.7)
    wet = F.convolve(F.filt(x, F.hp(300.0), extend=False), ir)
    x = layers(x, kit.set_level(wet, x, -18.0, 0.05))
    return F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False)


@cue('Loot.Drop.Epic', variations=1, space='2D', cls='Effects', jitter=0.0, conc=1, level=-9.0)
def drop_epic(v, r):
    # A rising three-note arpeggio, open and a little mysterious (E5, B5, E6), over a darker low swell (a bowed low
    # open fifth rising and falling away), and a shimmer of small chimes trailing off.
    notes = [('E5', 0.0, -3.0, -0.35, 0.6), ('B5', 0.085, -2.0, 0.0, 0.7), ('E6', 0.17, 0.0, 0.35, 0.95)]
    arp = np.zeros((2, 1))
    for i, (nm, t, db, pan, ring) in enumerate(notes):
        arp = layers(arp, (S.pan(_glint(nm, child(r, 'n', i), ring, 0.65, -9.0, 0.0002, -16.0), pan), t, db))
    dur = 1.1
    n = ns(dur)
    vib = 1.0 + 0.003 * np.sin(2.0 * np.pi * 4.5 * times(n))
    bowed = O.saw(kit.note('E2') * vib, n=n) + 0.7 * O.saw(kit.note('B2') * vib, n=n)
    bowed = F.filt(bowed, F.lp(650.0, 0.7), F.peak(190.0, 3.0, 1.2), F.hp(60.0), extend=False)
    bowed = normalize(bowed) * E.swell(dur, 0.28, 0.8, 1.6)
    breath = F.filt(N.pink(n, child(r, 'breath')), F.bp(300.0, 1.0), extend=False) * E.swell(dur, 0.25, 0.5)
    low = layers((normalize(bowed), 0.0, 0.0), (normalize(breath), 0.0, -18.0))
    sh = np.zeros((2, 1))
    sr = child(r, 'shimmer')
    for i in range(7):
        nm = ['E7', 'B6', 'E7', 'G#6'][i % 4]
        c = kit.chime(kit.note(nm), sr, 0.5, 0.4, 0.00012)
        sh = layers(sh, (S.pan(c, sr.uniform(-0.7, 0.7)), 0.24 + 0.085 * i + 0.02 * sr.random(), -3.0 * i))
    x = layers(arp, (S.widen(low, child(r, 'lw'), 0.3), 0.0, -9.0), (normalize(sh), 0.0, -19.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False)
    return _fanfare_space(x, r, 1.4, -13.0)


@cue('Loot.Drop.Legendary', variations=1, space='2D', cls='Effects', jitter=0.0, conc=1, level=-5.0, swell=True)
def drop_legendary(v, r):
    # The sting: the chord and a struck sheet of metal swelling up backwards, into a punchy low stab (a big drum and
    # the low strings), a twangy E major chord strummed across steel strings, and a bright cluster of bells over it,
    # ringing out in a plate.
    t0 = 0.3
    chord = ['E2', 'B2', 'E3', 'G#3', 'B3', 'E4']
    strum = np.zeros((2, 1))
    for i, nm in enumerate(chord):
        p = kit.twang(kit.note(nm), 2.0, child(r, 's', i), 0.65, 1.7)
        strum = layers(strum, (S.pan(p, -0.5 + 0.2 * i), 0.011 * i, -1.0 - 0.4 * i))
    sheet = normalize(M.strike(M.plate(400.0, child(r, 'sheet'), 36, 1.0, 0.4, tilt=0.45), M.hammer(0.0001), 1.0))
    k = ns(t0)
    rev = layers((normalize(reverse(mono(strum)[:k])), 0.0, 0.0), (normalize(reverse(sheet[:k])), 0.0, -3.0))
    rev = F.filt(rev, F.hp(150.0, 0.7), extend=False) * np.linspace(0.0, 1.0, k) ** 1.2
    head = O.membrane(58.0, 1.0, child(r, 'head'), 0.5, 0.12, 0.04)
    head = F.filt(F.convolve(head, M.hammer(0.0015))[:ns(1.0)], F.lp(600.0, 0.7), extend=False)
    air = kit.noise_thump(0.25, child(r, 'air'), 800.0, 70.0, 0.12, 1.0, 0.05)
    lows = layers(kit.twang(kit.note('E1'), 1.6, child(r, 'l1'), 0.5, 1.4), (kit.twang(kit.note('E2'), 1.6, child(r, 'l2'), 0.5, 1.3), 0.0, -3.0))
    stab = layers((normalize(head), 0.0, 0.0), (air, 0.0, -4.0), (normalize(lows), 0.004, -5.0))
    bells = np.zeros((2, 1))
    for i, nm in enumerate(['E6', 'G#6', 'B6', 'E7']):
        c = _glint(nm, child(r, 'b', i), 1.4 - 0.15 * i, 0.75, -10.0, 0.00015)
        bells = layers(bells, (S.pan(c, -0.45 + 0.3 * i), 0.006 * i, -1.5 * i))
    sp = np.zeros((2, 1))
    sr = child(r, 'sparkle')
    for i in range(9):
        s = kit.metal_click(sr, 4500.0, 9500.0, 0.03, 0.00004, 3, -20.0)
        sp = layers(sp, (S.pan(s, sr.uniform(-0.8, 0.8)), 0.05 + 0.55 * sr.random() ** 1.4, sr.uniform(-10.0, -2.0)))
    x = layers((S.widen(rev, child(r, 'rw'), 0.5), 0.0, -6.0), (S.widen(stab, child(r, 'sw'), 0.1), t0, 0.0),
               (strum, t0 + 0.008, -4.0), (bells, t0 + 0.035, -5.0), (normalize(sp), t0, -22.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.hp(30.0), extend=False)
    x = _fanfare_space(x, r, 1.8, -12.0)
    return Y.limit(normalize(D.drive(normalize(x), 2.0, 'tanh'), 0.0) * db2a(0.5), -1.0, 0.002, 0.08)


# --- Soul-motes (ASoulMotePickup): the wisp some kills leave, which heals ---------------------------------------------
# Glass and breath rather than the fanfare's brass, so a mote is never heard as a gun's rarity; warm and major, where an
# Unpaid's soul-light (Creature.Unpaid.Dissolve) is cold.

def _sparkle(r, count, start, spread, lo=2200.0, hi=3400.0):
    """A few tiny glass taps scattered after start, fading: the wisp's shimmer."""
    out = np.zeros(1)
    for i in range(count):
        f = float(np.exp(r.uniform(np.log(lo), np.log(hi))))
        c = kit.chime(f, child(r, 'tap', i), 0.3, 0.3, 0.0003, 0.4, -40.0)
        out = layers(out, (c, start + spread * r.random(), -2.5 * i - r.uniform(0.0, 3.0)))
    return normalize(out)


@cue('Loot.SoulMote.Drop', variations=3, att='Creature', jitter=0.02, conc=3, level=-13.0)
def soul_mote_drop(v, r):
    # A kill lets go of its soul-mote: a soft, glassy chime rising out of the body (two notes of a singing bowl a fifth
    # apart, warm and round, sliding up a hair as it lifts), a breath of air rising with it, a faint shimmer.
    from recipes import creatures as C
    a, b = [(587.3, 880.0), (523.3, 784.0), (659.3, 987.8)][v]
    tap = kit.chime(a * 2.0, child(r, 'tap'), 0.25, 0.4, 0.0003, 0.4, -40.0)
    c1 = C._bowl(a, 0.9, child(r, 'b1'), 0.02, 1.5)
    c2 = C._bowl(b, 1.0, child(r, 'b2'), 0.02, 1.8)
    lift = kit.whoosh(0.7, child(r, 'lift'), 350.0, 3000.0, 1.0, 0.45, -3.0, 0.8)
    shimmer = _sparkle(child(r, 'shimmer'), 4, 0.15, 0.45)
    x = layers((tap, 0.0, -10.0), (c1, 0.0, -2.0), (c2, 0.11, 0.0), (lift, 0.0, -15.0), (shimmer, 0.0, -21.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.hp(150.0), extend=False)
    return _outside(x, r, -18.0)


@cue('Loot.SoulMote.Pickup', variations=3, att='Near', jitter=0.02, conc=2, level=-10.0)
def soul_mote_pickup(v, r):
    # A mote taken into the player: a gentle chime as it touches, then a warm little swell (a soft major chord of glass
    # blooming up in a moment and settling, its root low and round), the air drawn in toward the body with it.
    root = [146.8, 130.8, 164.8][v]
    dur = 0.8
    n = ns(dur)
    t = times(n)
    pad = np.zeros(n)
    for i, (ratio, amp) in enumerate(((1.0, 1.0), (1.5, 0.55), (2.0, 0.7), (2.5, 0.45), (3.0, 0.3), (4.0, 0.18))):
        for s in (-0.5, 0.5):
            f = root * ratio + s * (0.8 + 0.3 * i) * r.uniform(0.7, 1.3)
            pad += 0.5 * amp * np.sin(2.0 * np.pi * f * t + 2.0 * np.pi * r.random())
    pad = normalize(pad * E.bp([(0.0, 0.0), (0.1, 1.0), (0.28, 0.65), (dur, 0.0)], n=n, curve='cos'))
    draw = V.whisper(0.22, [(0.0, 'h'), (0.22, 'u')], child(r, 'draw'), 1.1)
    draw = normalize(F.filt(draw * E.bp([(0.0, 0.0), (0.12, 1.0), (0.22, 0.0)], n=ns(0.22), curve='cos'), F.hp(400.0),
                            F.lp(6000.0), extend=False))
    chime = kit.chime(root * 8.0, child(r, 'chime'), 0.6, 0.5, 0.0003, 0.4, -36.0)
    shimmer = _sparkle(child(r, 'shimmer'), 3, 0.08, 0.3)
    x = layers((chime, 0.0, -5.0), (pad, 0.0, 0.0), (draw, 0.0, -12.0), (shimmer, 0.0, -22.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.hp(80.0), extend=False)
    return _outside(x, r, -22.0)
