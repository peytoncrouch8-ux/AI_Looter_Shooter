"""Loot and the gunsmith's bench: ammo and gun pickups, loot landing, chests opening, the Strongbox's vault wheel; the
bench's scrapping and fitting. Physical first (brass rounds, steel, wood, cloth), with a soft gleam where the moment
should feel like a reward.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, mono
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R, stereo as S
from lib import granular as G
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
