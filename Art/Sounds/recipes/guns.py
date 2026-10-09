"""The guns: the bullpup rifle and the Ranchhand pump shotgun firing, dry fire, the cursed misfire, the reload steps,
taking a gun in hand and raising the sights.

A shot is built in layers, the way action games build theirs, then glued into one dense hit:
- the crack: the muzzle's shock front (an N-wave), a few milliseconds of bright snap and a short knock that gives it a
  pitch, the sharp edge you hear first;
- the punch: a sine whose pitch dives (about 150 Hz to 50 Hz) in a few tens of milliseconds, torn by turbulence so it's
  a shove of air, overdriven so its harmonics carry it on small speakers too: the hit in the chest (for the shotgun
  part of it decays slowly: the sub boom);
- the body: a burst of overdriven noise with a low-mid hump, darkening as it spreads: the blast's mass;
- the mechanism: the action's steel and polymer parts, so it's a gun and not a drum.
The bus soft-clips the first peaks, compresses the body up to them (a low crest factor: loud for its peak) and dips
3 kHz, where the ear tires first. Then a short outdoor tail: two or three slap-backs and a quick dark roll with its lows
cut, so full-auto stays tight instead of droning. Each shot's weight lands in its first ~80 ms.

The reload steps are built from one piece, the clank: steel parts meeting (their modes held to about 1-4 kHz, so they
clank rather than tinkle), the polymer or wood around them knocking, and a low pitch-dropping thud for the gun's mass,
lightly overdriven so the three fuse into one solid, slightly larger-than-life clack. Slides between the hits are kept
low and dark.

Every layer here comes out with its loudest 10 ms at 1 (_loud), not its single loudest sample, so layers mix by how
loud they are, and a stray noise peak can't make one variation lighter or heavier than the next.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, fit, peak
from lib import noise as N, env as E, osc as O, filters as F, dist as D, dynamics as Y, modal as M, reverb as R
from lib import granular as G
from recipes import cue
from recipes import kit


# --- A shot's layers --------------------------------------------------------------------------------------------------

def _loud(x, win=0.01):
    """x scaled so its loudest win seconds sit at 1 (RMS, read as a peak)."""
    p = kit.env_peak(x, win)
    return x / p if p > 0 else x


def _crack(r, width, snap_t60, knock_f, snap_db=-4.0, knock_db=-6.0, lo=1800.0, hi=11000.0):
    """The sharp edge: the shock front, a few milliseconds of bright snap, and a short band of noise under it (a knock
    with a pitch) so the crack has some body instead of being a bare tick."""
    front = kit.nwave(width)
    snap = kit.burst(snap_t60 * 1.6 + 0.004, child(r, 'snap'), snap_t60, lo=lo, hi=hi)
    k_dur = snap_t60 * 4.0 + 0.01
    knock = N.white(ns(k_dur), child(r, 'knock')) * E.perc(k_dur, 0.0002, snap_t60 * 2.5)
    knock = F.filt(knock, F.bp(knock_f, 1.4), F.bp(knock_f, 1.4), extend=False)
    return _loud(layers((front, 0.0, 6.0), (_loud(snap, 0.002), 0.0, snap_db), (_loud(knock, 0.002), 0.0, knock_db)))


def _kick(r, f0, f1, tau, t60, dur, drive_db, hold=0.003, tear=0.3, boom=(0.0, 0.3), attack=0.0004, lp=2000.0):
    """The punch: a sine whose pitch dives from f0 towards f1 (most of the dive within tau seconds) and dies by t60,
    torn by turbulence so it's a blast of air rather than a drum's tone, and overdriven so it hits like a beater.
    boom (share, t60) hands part of it to a slower decay: the shotgun's sub boom. All the lows come from here, one
    deterministic layer, since low noise would swing the weight of every variation (a few random cycles of it add to
    or cancel the sine)."""
    n = ns(dur)
    t = times(n)
    f = f1 + (f0 - f1) * np.exp(-t / tau)
    env = E.perc(dur, attack, t60, hold)
    if boom[0] > 0:
        env = env * (1.0 - boom[0]) + boom[0] * E.perc(dur, attack, boom[1], hold)
    x = np.sin(2.0 * np.pi * O.phase(f, n)) * env
    if tear > 0:
        x = x * kit.turbulence(n, child(r, 'tear'), 300.0, tear)
    x = D.drive(x, drive_db, 'tanh')
    return _loud(F.filt(x, F.lp(lp, 0.7), F.hp(28.0, 0.7), extend=False))


def _body(r, dur, hold, t60, res, lp0, lp1, sweep, drive_db, color=-3.0, hp=220.0, turb=0.35):
    """The blast's mass: noise with the gun's resonances, overdriven at a steady level (so the drive adds density
    without stretching the decay), darkened from lp0 to lp1 as the gas spreads, and shaped into a held burst that
    falls away fast. Turbulence makes it tear rather than hiss. Cut under hp: the punch owns the lows."""
    n = ns(dur)
    x = N.shaped(n, r, color)
    x = F.filt(x, F.hpn(hp, 4), *[F.peak(f, g, q) for f, g, q in res], extend=False)
    x = D.drive(normalize(x), drive_db, 'tanh')
    k = ns(sweep)
    fc = np.full(n, float(lp1))
    fc[:k] = O.expsweep(lp0, lp1, sweep, 1.5, n=k)
    x = F.sweep(x, 'lp', fc, 0.7)
    x = x * kit.turbulence(n, child(r, 'turb'), 240.0, turb)
    # Cut again: the drive and the flutter make low difference tones of their own.
    return _loud(F.filt(x * E.perc(dur, 0.0003, t60, hold), F.hpn(hp * 0.8, 2), extend=False))


def _bus(dry, drive_db, thresh_db, ratio, dip_db=-3.0):
    """Glues the dry layers into one hit: a soft clip shaves the crack's and the punch's first peaks, a fast compressor
    lifts the body up towards them, and a dip at 3 kHz keeps the ear's touchiest band calm over hundreds of shots."""
    x = D.drive(normalize(dry), drive_db, 'tanh')
    x = Y.compress(normalize(x), thresh_db, ratio, 0.0008, 0.04, 6.0)
    return normalize(F.filt(x, F.peak(3100.0, dip_db, 0.9), extend=False), 0.0)


def _early(dry, r, rel_db, span, lp=5000.0, hp=300.0):
    """The ground and whatever's near the shooter answering at once: fills the shot's mids out over span seconds. Its
    lows are cut, since a few sparse echoes would comb the punch differently in every variation."""
    e = F.convolve(F.filt(dry, F.hpn(hp, 4), extend=False), R.early(child(r, 'early'), span, 16, lp))
    return layers(dry, kit.set_level(e, dry, rel_db, 0.02))


def _tail(dry, r, slaps, t60, lp, dur, rel_db, hp, tail_db, tail_peak):
    """The land answering: distinct slap-backs, then a quick dark roll. The lows are cut before it (they'd smear into
    a drone under full-auto), and its loudest 20 ms sit rel_db under the shot's loudest 20 ms."""
    ir = R.outdoor(child(r, 'ir'), dur=dur, slaps=slaps, slap_lp=2600.0, tail_t60=t60, tail_db=tail_db,
                   tail_start=0.02, tail_peak=tail_peak, tail_lp=lp)
    wet = F.convolve(F.filt(dry, F.hpn(hp, 4), extend=False), ir)
    return kit.set_level(wet, dry, rel_db, 0.02)


# --- The mechanism's pieces -------------------------------------------------------------------------------------------

def _clank(r, steel=(900.0, 3600.0, 0.05), body=(420.0, 0.05), knock=(160.0, 0.06), mix=(0.0, -4.0, -8.0),
           contact=0.0001, tick_db=-14.0, drive_db=4.0):
    """One mechanical hit: steel parts meeting (lo, hi, ring time), the polymer or wood around them (pitch, ring time)
    and the gun's mass thudding (a sine dropping from its pitch to about half), mixed by how loud each is (mix: steel,
    body, knock in dB) and overdriven a little so they fuse into one clack. Mixed by loudness, not peaks: a ringing
    steel part peaks high for its weight, and a sine thud would bury it."""
    lo, hi, t60 = steel
    s = M.strike(M.parts(lo, hi, child(r, 'steel'), 8, t60, t60 * 0.4, 0.45), M.hammer(contact), t60 * 1.8 + 0.01)
    tick = N.white(ns(0.002), child(r, 'tick')) * E.decay(ns(0.002), 0.0007)
    tick = F.filt(tick, F.hp(1500.0, 0.7), F.lp(7000.0, 0.7), extend=False)
    f0, bt = body
    b = M.strike(M.wood(f0, child(r, 'body'), t60=bt, count=6, spread=0.08), M.hammer(contact * 5.0), bt * 1.8 + 0.01)
    kf, kt = knock
    k = kit.thump(kf, kf * 0.55, kt * 1.5 + 0.01, kt, attack=0.0006, sweep_shape=2.0)
    x = layers((_loud(s, 0.005), 0.0, mix[0]), (_loud(tick, 0.005), 0.0, mix[0] + tick_db),
               (_loud(b, 0.005), 0.0, mix[1]), (_loud(k, 0.005), 0.0, mix[2]))
    return _loud(D.drive(normalize(x), drive_db, 'tanh'))


def _slide(dur, r, f0, f1, attack=0.3, release=0.5):
    """A part sliding along another (a magazine in its well, a bolt or a forend on its rails): stick-slip grit rung
    through low, wide resonances and kept dark, so it reads as movement rather than hiss. Mixed well under the hits:
    a slide is steady, so it's loud for its peak, and too much of it is what makes a reload sound like hiss."""
    y = kit.scrape(dur, r, f0, f1, 5.0, 0.7, (1.0, 1.6, 2.5), attack, release, 1600.0, -34.0)
    return _loud(F.filt(y, F.lp(3200.0, 0.7), extend=False))


def _rounds(r, count, spread):
    """Cartridges shifting in a magazine: a few dull brass knocks."""
    def grain(rr, i):
        return kit.brass_tick(rr, 1500.0 * jitter(rr, 1.0, 0.2), 0.02, 3, 0.0002, -10.0, -18.0, 0.1)
    return _loud(G.rattle(0.05, child(r, 'rounds'), count, spread, grain, 0.0, 0.7))


# --- Firing -----------------------------------------------------------------------------------------------------------

@cue('Weapon.Rifle.Fire', variations=6, att='Gun', jitter=0.03, conc=8, level=0.0)
def rifle_fire(v, r):
    # A rifle round out of a short bullpup barrel: a hard crack, a tight punch, a bark of gas, the bolt carrier
    # slamming back into its buffer and home again.
    crack = _crack(child(r, 'crack'), 0.00028 * jitter(r, 1.0, 0.12), 0.01, 1500.0 * jitter(r, 1.0, 0.06), -1.0, -3.0)
    punch = _kick(child(r, 'punch'), 158.0 * jitter(r, 1.0, 0.05), 46.0 * jitter(r, 1.0, 0.04), 0.016, 0.12, 0.16, 9.0,
                  0.004, 0.3)
    body = _body(child(r, 'body'), 0.13, 0.008, 0.07 * jitter(r, 1.0, 0.1),
                 [(350.0 * jitter(r, 1.0, 0.06), 3.0, 0.9), (1000.0 * jitter(r, 1.0, 0.06), 4.0, 1.2),
                  (3000.0, -5.0, 1.0)], 9000.0, 2000.0, 0.04, 10.0, -1.5)
    carrier = _clank(child(r, 'carrier'), (1100.0, 3800.0, 0.025), (520.0, 0.03), (190.0, 0.03), (0.0, -2.0, -6.0))
    home = _clank(child(r, 'home'), (900.0, 3200.0, 0.03), (450.0, 0.035), (170.0, 0.035), (0.0, -2.0, -5.0))
    dry = layers((crack, 0.0, 0.0), (punch, 0.0, 0.0), (body, 0.0, 0.0),
                 (carrier, 0.006 + 0.003 * r.random(), -28.0), (home, 0.055 + 0.008 * r.random(), -27.0))
    dry = _bus(dry, 9.0, -16.0, 3.0)
    dry = _early(dry, r, -11.0, 0.025)
    # A wall close by, the farm's buildings, a hillside: then a short roll, mostly gone by 0.4 s.
    s1 = 0.045 + 0.02 * r.random()
    s2 = 0.14 + 0.05 * r.random()
    s3 = s2 + 0.11 + 0.05 * r.random()
    wet = _tail(dry, r, ((s1, -3.0), (s2, -7.0), (s3, -13.0)), 0.4, 1800.0, 0.6, -16.0, 220.0, -7.0, 0.08)
    return Y.limit(normalize(layers(dry, wet), 0.0) * db2a(5.0), -1.0, 0.001, 0.03)


@cue('Weapon.Shotgun.Fire', variations=5, att='Gun', jitter=0.03, conc=4, level=1.0)
def shotgun_fire(v, r):
    # A 12-gauge pump: a wide crack, a deep punch and a sub boom you feel, a heavy dark blast, the barrel ringing
    # faintly, and a longer roll off the land.
    crack = _crack(child(r, 'crack'), 0.0005 * jitter(r, 1.0, 0.12), 0.01, 1100.0 * jitter(r, 1.0, 0.06),
                   -3.0, -2.0, 1400.0, 9000.0)
    # The punch hands almost half of itself to a slow decay at ~52 Hz: the boom, still ~39 Hz (and its overdriven
    # harmonics well above) when the boss's coach gun plays it at 0.75.
    punch = _kick(child(r, 'punch'), 140.0 * jitter(r, 1.0, 0.05), 52.0 * jitter(r, 1.0, 0.04), 0.028, 0.16, 0.7, 10.0,
                  0.01, 0.35, (0.45, 0.42))
    body = _body(child(r, 'body'), 0.28, 0.016, 0.14 * jitter(r, 1.0, 0.1),
                 [(300.0 * jitter(r, 1.0, 0.06), 4.0, 0.8), (600.0 * jitter(r, 1.0, 0.06), 3.0, 1.0),
                  (1150.0, 2.0, 1.3), (3000.0, -6.0, 1.0)], 7000.0, 1300.0, 0.07, 12.0, -2.5, 180.0)
    ring = M.strike(M.parts(600.0, 2400.0, child(r, 'ring'), 7, 0.14, 0.06, 0.4), M.hammer(0.0002), 0.26)
    dry = layers((crack, 0.0, 0.0), (punch, 0.0, 2.0), (body, 0.0, 0.0), (_loud(ring), 0.002, -27.0))
    dry = _bus(dry, 9.0, -18.0, 3.0)
    dry = _early(dry, r, -10.0, 0.035, 4000.0)
    s1 = 0.08 + 0.04 * r.random()
    s2 = 0.2 + 0.06 * r.random()
    s3 = s2 + 0.14 + 0.06 * r.random()
    wet = _tail(dry, r, ((s1, -3.0), (s2, -7.0), (s3, -12.0)), 0.75, 1400.0, 0.95, -12.0, 160.0, -6.0, 0.12)
    return Y.limit(normalize(layers(dry, wet), 0.0) * db2a(5.0), -1.0, 0.0015, 0.05)


@cue('Weapon.DryFire', variations=3, att='Near', jitter=0.03, conc=2, level=-15.0)
def dry_fire(v, r):
    # The trigger breaks and the hammer falls on an empty chamber: a small sear click, then a dull steel clack into
    # the frame with only a little knock, and nothing behind it.
    sear = _clank(child(r, 'sear'), (1600.0, 4200.0, 0.015), (800.0, 0.015), (260.0, 0.015), (0.0, -6.0, -12.0),
                  tick_db=-10.0)
    hammer = _clank(child(r, 'hammer'), (1000.0, 3400.0, 0.035), (560.0 * jitter(r, 1.0, 0.06), 0.035),
                    (200.0, 0.035), (0.0, -3.0, -9.0))
    gap = 0.012 + 0.006 * r.random()
    return layers((sear, 0.0, -8.0), (hammer, gap, 0.0))


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


# --- Reloading --------------------------------------------------------------------------------------------------------

@cue('Weapon.Rifle.MagOut', variations=3, att='Near', jitter=0.03, conc=2, level=-14.0)
def rifle_mag_out(v, r):
    # The thumb presses the release (a steel click into the stock), the catch lets go with a clack as the magazine
    # drops in the well, it slides out, and the rounds inside knock against its walls.
    button = _clank(child(r, 'button'), (1400.0, 4200.0, 0.02), (700.0, 0.02), (240.0, 0.02), (0.0, -6.0, -12.0),
                    tick_db=-12.0)
    unlatch = _clank(child(r, 'latch'), (900.0, 3200.0, 0.04), (380.0 * jitter(r, 1.0, 0.05), 0.05), (150.0, 0.05),
                     (0.0, -2.0, -4.0))
    slide = _slide(0.1, child(r, 'slide'), 520.0 * jitter(r, 1.0, 0.08), 760.0, 0.25, 0.55)
    rounds = _rounds(r, 3, 0.04)
    t1 = 0.018 + 0.006 * r.random()
    return layers((button, 0.0, -8.0), (unlatch, t1, 0.0), (slide, t1 + 0.012, -18.0), (rounds, t1 + 0.02, -16.0))


@cue('Weapon.Rifle.MagIn', variations=3, att='Near', jitter=0.03, conc=2, level=-12.0)
def rifle_mag_in(v, r):
    # The magazine's lips find the well (a light polymer tap and a short slide), then the palm slaps it home: a heavy
    # polymer-and-steel clack with the whole gun's thud under it, and the catch snapping over a hair later.
    tap = _clank(child(r, 'tap'), (1200.0, 3600.0, 0.02), (650.0, 0.025), (220.0, 0.025), (0.0, -3.0, -10.0))
    slide = _slide(0.05, child(r, 'slide'), 700.0, 480.0 * jitter(r, 1.0, 0.08), 0.3, 0.3)
    seat = _clank(child(r, 'seat'), (850.0, 3000.0, 0.05), (330.0 * jitter(r, 1.0, 0.05), 0.06),
                  (135.0 * jitter(r, 1.0, 0.05), 0.07), (0.0, 0.0, -2.0), 0.00015, drive_db=5.0)
    palm = _loud(kit.noise_thump(0.06, child(r, 'palm'), 1400.0, 300.0, 0.025, 0.9, 0.02, 0.0004))
    catch = _clank(child(r, 'catch'), (1500.0, 4200.0, 0.025), (800.0, 0.02), (260.0, 0.02), (0.0, -6.0, -12.0),
                   tick_db=-12.0)
    rounds = _rounds(r, 2, 0.02)
    t = 0.055 + 0.01 * r.random()
    return layers((tap, 0.0, -10.0), (slide, 0.004, -20.0), (seat, t, 0.0), (palm, t, -8.0),
                  (catch, t + 0.007 + 0.003 * r.random(), -6.0), (rounds, t + 0.01, -18.0))


@cue('Weapon.Rifle.Bolt', variations=3, att='Near', jitter=0.03, conc=2, level=-11.0)
def rifle_bolt(v, r):
    # The charging handle: fingers catch it and yank (a click as it unlocks), the bolt rides back on its rails and
    # hits the rear stop, then the spring drives it forward into battery with a heavy steel chunk, and the spring
    # buzzes for a moment.
    grab = _clank(child(r, 'grab'), (1300.0, 3800.0, 0.02), (600.0, 0.02), (220.0, 0.02), (0.0, -5.0, -11.0))
    back = _slide(0.06, child(r, 'back'), 600.0, 1000.0, 0.2, 0.3)
    stop = _clank(child(r, 'stop'), (1000.0, 3400.0, 0.035), (480.0, 0.035), (170.0, 0.04), (0.0, -3.0, -7.0))
    fwd = _slide(0.035, child(r, 'fwd'), 1000.0, 650.0, 0.3, 0.2)
    slam = _clank(child(r, 'slam'), (700.0 * jitter(r, 1.0, 0.05), 2800.0, 0.07), (360.0, 0.06),
                  (125.0 * jitter(r, 1.0, 0.05), 0.08), (0.0, -1.0, -2.0), 0.00009, drive_db=6.0)
    spring = M.strike(M.parts(1800.0, 3200.0, child(r, 'spring'), 3, 0.14, 0.1, 0.2), M.hammer(0.0002), 0.2)
    t_stop = 0.06 + 0.01 * r.random()
    t_slam = t_stop + 0.085 + 0.02 * r.random()
    return layers((grab, 0.0, -6.0), (back, 0.004, -18.0), (stop, t_stop, -3.0), (fwd, t_slam - 0.03, -20.0),
                  (slam, t_slam, 0.0), (_loud(spring), t_slam + 0.001, -24.0))


@cue('Weapon.Shotgun.ShellIn', variations=4, att='Near', jitter=0.04, conc=2, level=-14.0)
def shotgun_shell_in(v, r):
    # A shell thumbed past the loading gate into the tube: the brass rim knocks the gate, the hull slides in against
    # the spring, then the gate snaps shut behind it with a solid clack and the gun's knock.
    rim = _loud(kit.brass_tick(child(r, 'rim'), 1250.0 * jitter(r, 1.0, 0.06), 0.03, 4, 0.00015, -6.0, -14.0, 0.2))
    push = _slide(0.045, child(r, 'push'), 480.0, 720.0, 0.3, 0.5)
    gate = _clank(child(r, 'gate'), (850.0 * jitter(r, 1.0, 0.05), 2800.0, 0.035), (430.0 * jitter(r, 1.0, 0.06), 0.04),
                  (150.0 * jitter(r, 1.0, 0.05), 0.05), (0.0, -1.0, -3.0), drive_db=5.0)
    t = 0.04 + 0.012 * r.random()
    return layers((rim, 0.0, -8.0), (push, 0.006, -18.0), (gate, t, 0.0))


@cue('Weapon.Shotgun.Pump', variations=3, att='Near', jitter=0.03, conc=2, level=-9.0)
def shotgun_pump(v, r):
    # The forend racked back (the action unlocks, the bars ride the rails, the bolt hits the rear and the empty hull
    # kicks out: shk), then shoved forward into lock with the heaviest clack in the kit: CHAK.
    # Timed to the reload's motion: back at ~0.1 s, home at ~0.25 s.
    t_rear = 0.1 + 0.015 * r.random()
    t_lock = 0.235 + 0.025 * r.random()
    unlock = _clank(child(r, 'unlock'), (1100.0, 3400.0, 0.025), (500.0, 0.03), (190.0, 0.03), (0.0, -5.0, -10.0))
    back = _slide(t_rear - 0.004, child(r, 'back'), 520.0, 820.0, 0.15, 0.15)
    rear = _clank(child(r, 'rear'), (900.0, 3000.0, 0.045), (300.0 * jitter(r, 1.0, 0.05), 0.06),
                  (120.0 * jitter(r, 1.0, 0.05), 0.06), (0.0, -1.0, -3.0), drive_db=5.0)
    hull = _loud(layers(kit.thunk(900.0, child(r, 'hull'), 0.03, 0.0003),
                        (kit.brass_tick(child(r, 'head'), 1700.0, 0.025, 3, 0.0002, -14.0, -18.0, 0.1), 0.0, -6.0)))
    fwd = _slide(0.06, child(r, 'fwd'), 800.0, 540.0, 0.3, 0.2)
    lock = _clank(child(r, 'lock'), (750.0, 2800.0, 0.07), (270.0 * jitter(r, 1.0, 0.05), 0.08),
                  (105.0 * jitter(r, 1.0, 0.05), 0.09), (0.0, 0.0, -1.0), 0.00012, drive_db=6.0)
    return layers((unlock, 0.0, -7.0), (back, 0.008, -18.0), (rear, t_rear, -2.0), (hull, t_rear + 0.012, -12.0),
                  (fwd, t_lock - 0.055, -19.0), (lock, t_lock, 0.0))


# --- Handling ---------------------------------------------------------------------------------------------------------

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
