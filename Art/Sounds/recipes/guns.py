"""The guns: the bullpup rifle and the Ranchhand pump shotgun firing, dry fire, the cursed misfire, the reload steps,
taking a gun in hand and raising the sights.

A shot is built the way a real one happens: the muzzle blast's shock front (the crack), the expanding gas (a noise
burst that darkens as it spreads), the weight of it (a pitch-dropping thump), the action cycling (steel parts), and
the land answering (slap-backs off buildings and hills, then a rolling tail). The rifle is tight, bright and quick;
the shotgun is a wide, dark boom with a long roll.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, fit, peak
from lib import noise as N, env as E, osc as O, filters as F, dist as D, dynamics as Y, modal as M, reverb as R
from lib import granular as G
from recipes import cue
from recipes import kit


def _blast(r, dur, t60, f_start, f_end, sweep_time, res, drive_db, color=-2.0, attack=0.00025):
    """The gas burst: noise, bright at first and darkening as it spreads, with the gun's resonances. It's saturated at
    a steady level and shaped afterwards, so the drive adds density without stretching the decay."""
    n = ns(dur)
    x = N.shaped(n, r, color)
    x = F.filt(x, *[F.peak(f, g, q) for f, g, q in res], extend=False)
    x = D.drive(normalize(x), drive_db, 'tanh')
    k = ns(sweep_time)
    cutoff = np.full(n, float(f_end))
    cutoff[:k] = O.expsweep(f_start, f_end, sweep_time, 1.4, n=k)
    x = F.sweep(x, 'lp', cutoff, 0.75)
    # Turbulence: a real blast tears, it doesn't hiss smoothly like a drum's noise burst.
    x = x * kit.turbulence(n, child(r, 'turb'), 260.0, 0.5)
    return normalize(x * E.perc(dur, attack, t60), 0.0)


def _early(dry, r, rel_db, span, lp=5500.0):
    """The ground and whatever's near the shooter answering at once: fills the shot's body out over span seconds."""
    e = F.convolve(dry, R.early(child(r, 'early'), span, 18, lp))
    return layers(dry, kit.set_level(e, dry, rel_db, 0.02))


def _tail(dry, r, slaps, t60, lp, dur, rel_db, hp, tail_db=-12.0, tail_peak=0.11):
    """The land answering: the shot through an outdoor IR (no lows: they'd smear into a drone), its loudest 20 ms
    rel_db under the shot's loudest 20 ms (that's the first slap-back), the rolling tail tail_db under a 0 dB slap."""
    ir = R.outdoor(child(r, 'ir'), dur=dur, slaps=slaps, tail_t60=t60, tail_lp=lp, tail_db=tail_db,
                   tail_start=0.03, tail_peak=tail_peak)
    wet = F.convolve(F.filt(dry, F.hpn(hp, 4), extend=False), ir)
    return kit.set_level(wet, dry, rel_db, 0.02)


@cue('Weapon.Rifle.Fire', variations=5, att='Gun', jitter=0.03, conc=8, level=0.0)
def rifle_fire(v, r):
    crack = kit.nwave(0.00032 * jitter(r, 1.0, 0.12))
    hiss = kit.burst(0.012, child(r, 'hiss'), 0.005, lo=3500.0, hi=16000.0)
    body = _blast(child(r, 'blast'), 0.16, 0.06 * jitter(r, 1.0, 0.1), 11000.0, 2000.0, 0.04,
                  [(1150.0 * jitter(r, 1.0, 0.05), 6.0, 1.2), (480.0, 2.0, 1.0), (3300.0, -5.0, 1.0), (180.0, -3.0, 0.8)],
                  8.0, -1.0)
    snap = kit.burst(0.03, child(r, 'snap'), 0.008, lo=1200.0, hi=6000.0)
    # The weight: a short sine for the chest, and a dark burst of air for the push.
    punch = kit.thump(150.0 * jitter(r, 1.0, 0.05), 55.0, 0.1, 0.05, drive_db=4.0)
    push = kit.noise_thump(0.16, child(r, 'push'), 600.0, 90.0, 0.08, 1.2, 0.05)
    # The action: the bolt unlocks as the gas pushes it, then slams home on the next round.
    unlock = kit.metal_click(child(r, 'unlock'), 2600.0, 8500.0, 0.035, 0.00008)
    home = kit.clack(child(r, 'home'), 820.0, 1700.0, 6500.0, 0.06, 0.04, -6.0, -10.0)
    dry = layers((crack, 0.0, -2.0), (hiss, 0.0, -16.0), (body, 0.0, 0.0), (snap, 0.0, -9.0), (punch, 0.0, -9.0),
                 (push, 0.0, -5.0), (unlock, 0.004, -28.0), (home, 0.04 + 0.006 * r.random(), -24.0))
    dry = Y.transient(dry, 3.0, -1.0)
    dry = _early(dry, r, -9.0, 0.04)
    s1 = 0.09 + 0.05 * r.random()
    s2 = 0.22 + 0.1 * r.random()
    s3 = s2 + 0.14 + 0.08 * r.random()
    wet = _tail(dry, r, ((s1, -4.0), (s2, -8.0), (s3, -13.0)), 0.75, 2800.0, 1.2, -18.0, 260.0)
    return Y.limit(normalize(layers(dry, wet), 0.0) * db2a(4.0), -1.0, 0.001, 0.04)


@cue('Weapon.Shotgun.Fire', variations=5, att='Gun', jitter=0.03, conc=4, level=1.0)
def shotgun_fire(v, r):
    crack = kit.nwave(0.0006 * jitter(r, 1.0, 0.12))
    body = _blast(child(r, 'blast'), 0.28, 0.09 * jitter(r, 1.0, 0.1), 9500.0, 1500.0, 0.08,
                  [(520.0 * jitter(r, 1.0, 0.06), 6.0, 1.0), (200.0, 2.0, 0.8), (1400.0, 4.0, 1.4), (2700.0, -2.0, 1.0)],
                  11.0, -1.5)
    snap = kit.burst(0.05, child(r, 'snap'), 0.016, lo=900.0, hi=5000.0)
    boom = kit.thump(115.0 * jitter(r, 1.0, 0.05), 40.0, 0.22, 0.12, attack=0.001, sweep_shape=1.4, drive_db=6.0)
    push = kit.noise_thump(0.35, child(r, 'push'), 500.0, 60.0, 0.17, 1.3, 0.1)
    # The receiver and barrel ring a little after the blast.
    ring_set = M.parts(700.0, 2600.0, child(r, 'ring'), 7, 0.16, 0.08)
    ring = normalize(M.strike(ring_set, M.hammer(0.0002), 0.3))
    dry = layers((crack, 0.0, -3.0), (body, 0.0, 0.0), (snap, 0.0, -6.0), (boom, 0.0, -6.0), (push, 0.0, -3.0),
                 (ring, 0.002, -28.0))
    dry = Y.transient(dry, 2.0, 0.0)
    dry = _early(dry, r, -8.0, 0.05, 4000.0)
    s1 = 0.12 + 0.06 * r.random()
    s2 = 0.32 + 0.12 * r.random()
    s3 = s2 + 0.2 + 0.1 * r.random()
    wet = _tail(dry, r, ((s1, -3.0), (s2, -7.0), (s3, -12.0)), 1.3, 1900.0, 1.9, -17.0, 180.0, -11.0, 0.14)
    return Y.limit(normalize(layers(dry, wet), 0.0) * db2a(4.0), -1.0, 0.0015, 0.06)


@cue('Weapon.DryFire', variations=3, att='Near', jitter=0.03, conc=2, level=-15.0)
def dry_fire(v, r):
    # The trigger lets the sear go, and the hammer falls on an empty chamber: two clicks and the frame's knock.
    sear = kit.metal_click(child(r, 'sear'), 3200.0, 9000.0, 0.018, 0.00006)
    hammer = kit.metal_click(child(r, 'hammer'), 1500.0, 6000.0, 0.045, 0.0001)
    frame = kit.thunk(640.0 * jitter(r, 1.0, 0.06), child(r, 'frame'), 0.045, 0.0004)
    gap = 0.011 + 0.005 * r.random()
    return layers((sear, 0.0, -9.0), (hammer, gap, 0.0), (frame, gap, -7.0))


@cue('Weapon.Misfire', variations=3, att='Near', jitter=0.02, conc=2, level=-9.0)
def misfire(v, r):
    # A cursed round: the hammer falls, the primer pops weakly, the powder fizzles, and something in the iron groans.
    hammer = kit.clack(child(r, 'hammer'), 700.0, 1500.0, 6000.0, 0.05, 0.05, -5.0, -12.0)
    pop_at = 0.04 + 0.03 * r.random()
    pop = layers(kit.burst(0.08, child(r, 'pop'), 0.03, lo=120.0, hi=900.0),
                 (kit.thump(140.0, 60.0, 0.1, 0.06), 0.0, -4.0))
    dur = 1.1
    n = ns(dur)
    t = times(n)
    rate = 900.0 * np.exp(-t / 0.25) + 40.0 * np.exp(-t / 0.6)
    crackle = G.grit(dur, child(r, 'crackle'), rate, 2200.0, 7500.0, 3.0, 3)
    sizzle = N.band(n, child(r, 'sizzle'), 2500.0, 9000.0) * E.swell(dur, 0.05, 0.7)
    smoke = N.band(n, child(r, 'smoke'), 300.0, 1800.0) * E.swell(dur, 0.12, 0.6)
    fizzle = layers((normalize(crackle), 0.0, -4.0), (normalize(sizzle), 0.0, -12.0), (normalize(smoke), 0.0, -14.0))
    groan_n = ns(1.2)
    tg = times(groan_n)
    f = 74.0 * (1.0 - 0.08 * tg)
    groan = (O.sine(f) + O.sine(f * 1.06) * 0.8 + 0.3 * O.sine(f * 2.01)) * E.swell(1.2, 0.35, 0.8)
    groan = F.filt(D.drive(normalize(groan), 8.0), F.hp(60.0), F.lp(600.0), extend=False)
    out = layers((hammer, 0.0, 0.0), (pop, pop_at, -3.0), (fizzle, pop_at + 0.01, -8.0), (normalize(groan), 0.05, -21.0))
    ir = R.outdoor(child(r, 'ir'), dur=0.8, slaps=((0.11, -10.0), (0.26, -16.0)), tail_t60=0.6, tail_db=-16.0)
    return R.apply(out, ir, wet_db=-16.0, pre_hp=200.0)


@cue('Weapon.Rifle.MagOut', variations=3, att='Near', jitter=0.03, conc=2, level=-14.0)
def rifle_mag_out(v, r):
    # The release button clicks, the magazine slides out of the well, the rounds in it shift.
    release = kit.metal_click(child(r, 'button'), 2200.0, 7000.0, 0.03, 0.0001)
    unlatch = kit.clack(child(r, 'latch'), 900.0, 1600.0, 6000.0, 0.035, 0.03, -4.0, -14.0)
    slide = kit.scrape(0.12, child(r, 'slide'), 900.0 * jitter(r, 1, 0.1), 1500.0, 12.0, 0.5, attack=0.15, release=0.6)
    rounds = kit.jingle(0.25, child(r, 'rounds'), 4, 2600.0, 8000.0, 0.03, 0.05)
    t_slide = 0.03 + 0.01 * r.random()
    return layers((release, 0.0, -6.0), (unlatch, 0.012, -2.0), (slide, t_slide, -15.0), (rounds, t_slide + 0.02, -20.0))


@cue('Weapon.Rifle.MagIn', variations=3, att='Near', jitter=0.03, conc=2, level=-12.0)
def rifle_mag_in(v, r):
    # The magazine slides in and seats with a firm clack as the catch takes it.
    slide = kit.scrape(0.09, child(r, 'slide'), 1300.0, 800.0 * jitter(r, 1, 0.1), 12.0, 0.5, attack=0.2, release=0.3)
    seat = kit.clack(child(r, 'seat'), 560.0 * jitter(r, 1, 0.05), 1500.0, 6200.0, 0.06, 0.07, -1.0, -5.0, 0.00012)
    catch = kit.metal_click(child(r, 'catch'), 2800.0, 8000.0, 0.03, 0.00007)
    t = 0.07 + 0.01 * r.random()
    return layers((slide, 0.0, -16.0), (seat, t, 0.0), (catch, t + 0.006, -9.0))


@cue('Weapon.Rifle.Bolt', variations=3, att='Near', jitter=0.03, conc=2, level=-11.0)
def rifle_bolt(v, r):
    # The charging handle back against its spring (a rising scrape that clicks at the end), then let go: the bolt
    # slams forward and locks.
    back = kit.scrape(0.08, child(r, 'back'), 1100.0, 2100.0, 14.0, 0.4, attack=0.15, release=0.4)
    stop = kit.metal_click(child(r, 'stop'), 2000.0, 7000.0, 0.035, 0.0001)
    t = 0.12 + 0.03 * r.random()
    fwd = kit.scrape(0.04, child(r, 'fwd'), 2100.0, 1300.0, 14.0, 0.3, attack=0.4, release=0.2)
    slam = kit.clack(child(r, 'slam'), 760.0 * jitter(r, 1, 0.05), 1400.0, 6500.0, 0.08, 0.06, -3.0, -4.0, 0.00009)
    spring = O.sine(np.full(ns(0.12), 3300.0 * jitter(r, 1, 0.08))) * E.perc(0.12, 0.001, 0.08)
    return layers((back, 0.0, -15.0), (stop, 0.075, -7.0), (fwd, t, -17.0), (slam, t + 0.035, 0.0),
                  (normalize(spring), t + 0.036, -30.0))


@cue('Weapon.Shotgun.ShellIn', variations=4, att='Near', jitter=0.04, conc=2, level=-14.0)
def shotgun_shell_in(v, r):
    # A shell pushed past the loading gate into the tube: the brass rim taps the gate, the shell slides, the gate
    # snaps back behind it.
    tap = kit.brass_tick(child(r, 'rim'), 1500.0 * jitter(r, 1, 0.06), 0.025, 3, 0.00015, -14.0, -14.0, 0.0)
    push = kit.scrape(0.06, child(r, 'push'), 700.0, 1100.0, 10.0, 0.5, attack=0.3, release=0.5)
    gate = kit.clack(child(r, 'gate'), 520.0 * jitter(r, 1, 0.06), 1100.0, 4000.0, 0.03, 0.05, -2.0, -8.0)
    t = 0.05 + 0.015 * r.random()
    return layers((tap, 0.0, -10.0), (push, 0.01, -17.0), (gate, t, 0.0))


@cue('Weapon.Shotgun.Pump', variations=3, att='Near', jitter=0.03, conc=2, level=-9.0)
def shotgun_pump(v, r):
    # The wooden forend back (the action rails slide, the empty hull flips out), then forward to lock: shk-SHAK.
    back = kit.scrape(0.07, child(r, 'back'), 800.0, 1300.0, 12.0, 0.5, attack=0.15, release=0.3)
    stop_back = kit.clack(child(r, 'stopb'), 420.0 * jitter(r, 1, 0.05), 1300.0, 5000.0, 0.05, 0.08, 0.0, -4.0)
    hull = kit.thunk(1300.0, child(r, 'hull'), 0.03, 0.0003)
    t2 = 0.16 + 0.04 * r.random()
    fwd = kit.scrape(0.06, child(r, 'fwd'), 1300.0, 900.0, 12.0, 0.5, attack=0.3, release=0.3)
    lock = kit.clack(child(r, 'lock'), 360.0 * jitter(r, 1, 0.05), 1200.0, 5500.0, 0.07, 0.09, 1.0, -2.0, 0.00012)
    return layers((back, 0.0, -17.0), (stop_back, 0.065, -3.0), (hull, 0.075, -16.0), (fwd, t2, -17.0),
                  (lock, t2 + 0.05, 0.0))


@cue('Weapon.Equip', variations=3, att='Near', jitter=0.04, conc=2, level=-14.0)
def equip(v, r):
    # Swinging the gun up from the back or hip: cloth, the sling's hardware jingling, the hand closing on the grip.
    swish = kit.cloth(0.28, child(r, 'cloth'), 300.0, 2800.0, 120.0, 0.2, 0.5)
    jing = kit.jingle(0.3, child(r, 'jingle'), 3 + int(r.integers(0, 3)), 2200.0, 7500.0, 0.04, 0.08, 0.05)
    grip = kit.thunk(380.0 * jitter(r, 1, 0.08), child(r, 'grip'), 0.05, 0.0012)
    slap = kit.burst(0.04, child(r, 'slap'), 0.015, lo=200.0, hi=2500.0)
    settle = kit.clack(child(r, 'settle'), 640.0, 1600.0, 6000.0, 0.04, 0.05, -2.0, -8.0)
    # The hand catching the sling at once, so the draw answers the key with no lag.
    catch = kit.metal_click(child(r, 'catch'), 2000.0, 6000.0, 0.03, 0.0002, 4, -16.0)
    t = 0.19 + 0.04 * r.random()
    return layers((catch, 0.0, -14.0), (swish, 0.0, -6.0), (jing, 0.0, -16.0), (grip, t, -4.0), (slap, t, -8.0),
                  (settle, t + 0.03, -6.0))


@cue('Weapon.AimIn', variations=3, att='Near', jitter=0.03, conc=2, level=-19.0)
def aim_in(v, r):
    # Shouldering the gun to the eye: a short cloth swish, the cheek on the stock, the sight settling (a soft tick).
    swish = kit.cloth(0.16, child(r, 'cloth'), 250.0, 2200.0, 160.0, 0.25, 0.45)
    cheek = kit.thunk(300.0 * jitter(r, 1, 0.1), child(r, 'cheek'), 0.04, 0.002)
    tick = kit.metal_click(child(r, 'tick'), 1800.0, 5000.0, 0.02, 0.00015, tick_db=-18.0)
    t = 0.1 + 0.02 * r.random()
    return layers((swish, 0.0, -4.0), (cheek, t, -6.0), (tick, t + 0.012, -12.0))
