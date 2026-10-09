"""The guns: the bullpup rifle and the Ranchhand pump shotgun firing, dry fire, the cursed misfire, the reload steps,
taking a gun in hand and raising the sights.

A shot is modelled the way a real one reaches a close microphone, not built from drum-machine parts (a swept sine
kick and smooth noise read as fake):
- the blast: the muzzle's pressure wave in Friedlander's form (a near-instant rise, a positive phase of 1-3 ms, a
  shallower suction after), with the bullet's crack on its front, and a slower pulse of shoved air under it for the
  weight; all of them roughened and different every take, none of them a tone;
- the gas: a dense spray of tiny explosions over turbulent noise, flickering and sputtering, then a darker roar;
- the ground: the blast's bounce off the ground a few milliseconds later, darker, comb-colouring it as on any
  recording;
- the recorder: the microphone and preamp overloading (lopsided clipping), the input filter turning that into a low,
  ringing heave, the gain sagging and swelling back after the overload (the bloom), the proximity lift in the low mids;
- the mechanism, loud enough to hear on every shot, the way big looter shooters mix it;
- the land: slap-backs off nearby buildings, then many fainter echoes from farther terrain (each darker and more
  smeared the farther it is) over a thunder-like rumble whose level wanders. The rifle's rolls for about a second,
  kept low so full-auto stays clear; the shotgun's for about a second and a half.
A soft clip rounds the transient before the limiter, and a dip at 3 kHz keeps the ear's touchiest band calm.

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


def _friedlander(r, T, b, dur, jag=0.25):
    """The muzzle blast's pressure wave as acoustics measures it (Friedlander's form): a near-instant rise, a positive
    phase T seconds long falling through zero, then a shallower, longer suction. Roughened, since a real front is
    jagged by the muzzle and the gas behind it, and never twice the same."""
    n = ns(dur)
    t = times(n)
    p = (1.0 - t / T) * np.exp(-b * t / T)
    p = p * (1.0 + jag * N.smooth_random(n, child(r, 'jag'), 2.0 / T))
    k = max(2, ns(0.00008))
    p[:k] *= np.linspace(0.0, 1.0, k)
    return E.end_fade(p)


def _heave(r, T, lp, dur):
    """The weight: a long, slow pressure pulse (the air the shot shoves) with only its lows kept. One push and pull
    of air, not a tone: it can't glide in pitch like a drum machine's kick, and every take's is shaped differently."""
    return _loud(F.filt(_friedlander(r, T, 1.0, dur, 0.15), F.lpn(lp, 4), extend=False), 0.005)


def _gas(r, dur, hold, t60, rate0, rate_tau, lo, hi, color, drive_db, roar_db, roar_t60, roar_lp, flick_rate=90.0,
         flick=0.55, puffs=3):
    """The gas tearing out of the muzzle: not smooth noise but a dense spray of tiny explosions (random impulses rung
    through a few bands) over turbulent noise, overdriven so it's dense and torn, its level flickering many times a
    second and puffing up again a few times as it sputters. Behind it the roar: the cloud still churning, darker and
    slower."""
    n = ns(dur)
    t = times(n)
    rate = rate0 * (np.exp(-t / rate_tau) + 0.02)
    crackle = G.grit(dur, child(r, 'crackle'), rate, lo, hi, 1.4, 5)
    turb = N.shaped(n, child(r, 'turb'), color) * kit.turbulence(n, child(r, 'tflut'), 500.0, 0.8)
    turb = F.filt(turb, F.hp(lo * 0.5, 0.7), F.lp(hi * 1.5, 0.7), extend=False)
    x = D.drive(normalize(normalize(crackle) + 0.8 * normalize(turb)), drive_db, 'tanh')
    rr = child(r, 'puffs')
    env = E.perc(dur, 0.0003, t60, hold)
    for i in range(puffs):
        at = rr.uniform(0.008, 0.6 * t60 + 0.01)
        tau = rr.uniform(0.004, 0.012)
        env = env + rr.uniform(0.15, 0.45) * np.exp(-np.clip(t - at, 0.0, None) / tau) * (t >= at) \
            * np.exp(-6.9 * at / t60)
    fl = (1.0 - flick) + flick * np.abs(N.smooth_random(n, child(r, 'flick'), flick_rate)) ** 0.7
    burst = x * E.end_fade(env * fl)
    roar = N.shaped(n, child(r, 'roar'), -3.0) * kit.turbulence(n, child(r, 'rflut'), 160.0, 0.75)
    roar = D.drive(normalize(F.filt(roar, F.hp(120.0, 0.7), F.lp(roar_lp, 0.7), extend=False)), 6.0, 'tanh')
    rfl = 0.4 + 0.6 * np.abs(N.smooth_random(n, child(r, 'rflick'), 35.0)) ** 0.8
    roar = roar * E.end_fade(E.perc(dur, 0.002, roar_t60, 0.0) * rfl)
    return _loud(layers(_loud(burst), (_loud(roar), 0.0, roar_db)))


def _ground(x, taps):
    """The blast's bounce off the ground and whatever's at the shooter's feet, a few milliseconds behind it: darker
    copies (soil and grass drink the highs) that comb-colour the shot as they do on every real recording."""
    out = x
    for at, db, lp in taps:
        out = layers(out, (F.filt(x, F.lp(lp, 0.7), extend=False), at, db))
    return out


def _recorder(x, drive_db, asym, hp_f, hp_q, lift_db, bloom_db, bloom_t, lift_f=120.0):
    """A close microphone and a field recorder taking a shot they can't hold: the diaphragm and preamp overload,
    clipping harder on the push than the pull; the input's coupling filter turns the lopsided waveform into a low,
    ringing heave (the boom of real recordings, different every take); the preamp is slow to recover, so the gain
    sags just after the overload and swells back (the bloom); and the microphone's proximity effect lifts the low
    mids."""
    y = D.drive(normalize(x), drive_db, 'tanh', bias=asym)
    y = np.pad(y, (0, ns(0.08)))
    y = F.filt(y, F.hp(hp_f, hp_q), extend=False)
    e = Y.envelope(y, 0.001, bloom_t)
    e = e / (e.max() + 1e-12)
    d = ns(0.0015)
    e = np.concatenate([np.zeros(d), e[:-d]])
    y = y * db2a(-bloom_db * e)
    y = F.filt(y, F.peak(lift_f, lift_db, 0.8), extend=False)
    return normalize(y, 0.0)


def _early(dry, r, rel_db, span, lp=5000.0, hp=300.0):
    """The ground and whatever's near the shooter answering at once: fills the shot's mids out over span seconds. Its
    lows are cut, since a few sparse echoes would comb the punch differently in every variation."""
    e = F.convolve(F.filt(dry, F.hpn(hp, 4), extend=False), R.early(child(r, 'early'), span, 16, lp))
    return layers(dry, kit.set_level(e, dry, rel_db, 0.02))


def _echo(rr, at, a, lp_ref=9000.0):
    """One echo off the land at a delay of at seconds: the farther the wall or hill, the darker (air and ground absorb
    the highs) and the more smeared in time (a rough face sends the sound back along many paths)."""
    dist = 343.0 * at / 2.0
    fc = float(np.clip(lp_ref * (25.0 / max(dist, 1.0)) ** 0.7, 450.0, lp_ref))
    m = ns(0.004 + 0.06 * at)
    b = N.white(m, rr) * np.exp(-np.arange(m) / (m / 3.0))
    b = F.filt(b, F.lpn(fc, 2), extend=False)
    return a * b / (np.sqrt(np.sum(b * b)) + 1e-12)


def _land(dry, r, dur, slaps, far, first, last, rumble_db, rumble_t60, rumble_lp, rumble_peak, rel_db, hp):
    """The land answering, as an outdoor recording hears it: a few strong slap-backs off nearby buildings (slaps:
    delay, dB), then many fainter echoes from farther terrain at random distances, each a darker, smeared copy of the
    shot, over a thunder-like rumble whose level wanders rather than decaying smoothly like a reverb's. Lows under hp
    are left out of it so full-auto can't build a drone, and its loudest 20 ms sit rel_db under the shot's."""
    rr = child(r, 'land')
    n = ns(dur)
    ir = np.zeros(n)
    events = [(at, float(db2a(db))) for at, db in slaps]
    for i in range(far):
        at = first + (last - first) * rr.random() ** 1.2
        events.append((at, rr.uniform(0.15, 0.6) * (first / at) ** 0.9))
    top = 0.0
    for at, a in events:
        k = ns(at)
        if k >= n:
            continue
        b = _echo(rr, at, a)
        m = min(b.size, n - k)
        ir[k:k + m] += b[:m]
        top = max(top, a)
    t = times(n)
    rum = N.pink(n, child(rr, 'rumble'))
    rum = F.filt(rum, F.lpn(rumble_lp, 4), F.hp(60.0, 0.7), extend=False)
    wander = (0.3 + 0.7 * np.abs(N.smooth_random(n, child(rr, 'wander'), 5.0))) ** 1.5
    rise = np.clip(t / rumble_peak, 0.0, 1.0) ** 2
    rum = E.end_fade(rum * wander * rise * np.exp(-6.9 * np.clip(t - rumble_peak, 0.0, None) / rumble_t60))
    ir = ir + rum / (np.sqrt(np.sum(rum * rum)) + 1e-12) * top * float(db2a(rumble_db)) * 6.0
    ir = E.end_fade(ir, 0.02)
    wet = F.convolve(F.filt(dry, F.hpn(hp, 4), extend=False), ir)
    return kit.set_level(wet, dry, rel_db, 0.02)


def _master(x, clip_db, push_db, lookahead, release):
    """The last stage: a soft clip rounds the transient off (it keeps the snap a limiter would squash), a dip at 3 kHz
    keeps the ear's touchiest band calm over hundreds of shots, then the limiter catches what's left."""
    x = D.drive(normalize(x), clip_db, 'tanh')
    x = F.filt(x, F.peak(3100.0, -2.5, 0.9), extend=False)
    return Y.limit(normalize(x, 0.0) * db2a(push_db), -1.0, lookahead, release)


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
    # A rifle round out of a short bullpup barrel, recorded close: the hammer falls, the blast and the bullet's crack
    # overload the microphone, the gas tears out and roars, the bolt carrier hits its buffer and slams home with a
    # crisp steel clack, and the farm and the hills answer for about a second.
    crack = _crack(child(r, 'crack'), 0.00028 * jitter(r, 1.0, 0.12), 0.006, 1600.0 * jitter(r, 1.0, 0.08), -2.0, -4.0)
    blast = _loud(_friedlander(child(r, 'blast'), 0.0013 * jitter(r, 1.0, 0.15), 1.2 * jitter(r, 1.0, 0.2), 0.03, 0.3),
                  0.002)
    gas = _gas(child(r, 'gas'), 0.3, 0.004, 0.1 * jitter(r, 1.0, 0.12), 30000.0, 0.012, 250.0, 5000.0, -2.0, 8.0,
               -4.0, 0.2, 2200.0)
    heave = _heave(child(r, 'heave'), 0.006 * jitter(r, 1.0, 0.15), 140.0, 0.08)
    src = layers((blast, 0.0, 6.0), (crack, 0.0, 0.0), (gas, 0.0004, 0.0), (heave, 0.0, 6.0))
    g1 = 0.0025 + 0.002 * r.random()
    src = _ground(src, [(g1, -3.0, 5000.0), (g1 + 0.002 + 0.003 * r.random(), -8.0, 2500.0)])
    rec = _loud(_recorder(src, 10.0 + 1.5 * (2.0 * r.random() - 1.0), 0.3, 75.0 * jitter(r, 1.0, 0.08), 1.3, 7.0, 3.0,
                          0.05))
    # The action, loud enough to hear on every shot: the hammer's tick a hair before the blast (it makes the gun feel
    # bigger), the carrier hitting the buffer, and the bolt slamming home (hard inharmonic steel, the polymer housing,
    # the buffer spring ringing faintly).
    tick = _loud(kit.metal_click(child(r, 'tick'), 2000.0, 6500.0, 0.012, 0.00005, 6, -8.0), 0.005)
    buffer = _clank(child(r, 'buffer'), (1200.0, 4200.0, 0.03), (600.0, 0.025), (220.0, 0.02), (0.0, -5.0, -10.0),
                    0.00006)
    steel = M.strike(M.parts(900.0, 5200.0, child(r, 'home'), 10, 0.05, 0.02, 0.35), M.hammer(0.00005), 0.12)
    spring = M.strike(M.parts(1800.0, 3600.0, child(r, 'spring'), 3, 0.22, 0.15, 0.2), M.hammer(0.0002), 0.35)
    home = _loud(layers((_loud(steel, 0.005), 0.0, 0.0), (_loud(spring, 0.005), 0.0, -14.0),
                        (_clank(child(r, 'housing'), (1000.0, 3000.0, 0.02), (480.0, 0.03), (180.0, 0.03),
                                (-6.0, 0.0, -6.0)), 0.0, -4.0)))
    pre = 0.003
    t_buf = pre + 0.026 + 0.006 * r.random()
    t_home = pre + 0.058 + 0.008 * r.random()
    dry = layers((tick, 0.0, -18.0), (rec, pre, 0.0), (buffer, t_buf, -22.0), (home, t_home, -18.0))
    dry = _early(dry, r, -13.0, 0.03)
    # A barn close by and the farmhouse, then the hills out to ~160 m, kept low so full-auto stays clear.
    s1 = 0.05 + 0.03 * r.random()
    s2 = s1 + 0.09 + 0.06 * r.random()
    wet = _land(dry, r, 1.4, ((s1, -2.0), (s2, -5.0)), 18, 0.22, 0.95, -6.0, 0.8, 700.0, 0.2, -20.0, 180.0)
    return _master(layers(dry, wet), 5.0, 2.0, 0.001, 0.03)


@cue('Weapon.Shotgun.Fire', variations=5, att='Gun', jitter=0.03, conc=4, level=1.0)
def shotgun_fire(v, r):
    # A 12-gauge pump, recorded close: a wide crack and a long, heavy blast (a big bore's pressure wave lasts twice a
    # rifle's), the shove of air in the chest, the gas roaring out, the pellets hissing away, the receiver ringing and
    # the forend and action knocking steel-on-wood as the gun kicks, then a long boom rolling across the land.
    crack = _crack(child(r, 'crack'), 0.0005 * jitter(r, 1.0, 0.12), 0.008, 1100.0 * jitter(r, 1.0, 0.08), -3.0, -3.0,
                   1400.0, 9000.0)
    blast = _loud(_friedlander(child(r, 'blast'), 0.0026 * jitter(r, 1.0, 0.15), 1.0 * jitter(r, 1.0, 0.2), 0.05, 0.3),
                  0.003)
    # The weight sits around 60-120 Hz (still 45-90 Hz when the boss's coach gun plays it at 0.75).
    heave = _heave(child(r, 'heave'), 0.009 * jitter(r, 1.0, 0.15), 120.0, 0.12)
    gas = _gas(child(r, 'gas'), 0.5, 0.008, 0.12 * jitter(r, 1.0, 0.12), 20000.0, 0.018, 180.0, 4000.0, -3.0, 8.0,
               -8.0, 0.25, 1600.0)
    src = layers((blast, 0.0, 6.0), (crack, 0.0, 0.0), (gas, 0.0005, 0.0), (heave, 0.0, 10.0))
    g1 = 0.003 + 0.003 * r.random()
    src = _ground(src, [(g1, -3.0, 4000.0), (g1 + 0.002 + 0.003 * r.random(), -7.0, 2000.0)])
    rec = _loud(_recorder(src, 12.0 + 1.5 * (2.0 * r.random() - 1.0), 0.35, 60.0 * jitter(r, 1.0, 0.08), 1.4, 7.0, 1.5,
                          0.08, 100.0))
    ring = _loud(M.strike(M.parts(500.0, 2500.0, child(r, 'ring'), 8, 0.25, 0.12, 0.4), M.hammer(0.0002), 0.4), 0.005)
    chunk = _clank(child(r, 'chunk'), (700.0, 2600.0, 0.05), (260.0 * jitter(r, 1.0, 0.06), 0.07),
                   (110.0 * jitter(r, 1.0, 0.06), 0.07), (0.0, -1.0, -3.0), 0.0001, drive_db=5.0)
    n = ns(0.25)
    sizzle = G.grit(0.25, child(r, 'sizzle'), 9000.0 * np.exp(-times(n) / 0.05), 3000.0, 9000.0, 2.0, 4)
    sizzle = _loud(sizzle * E.perc(0.25, 0.004, 0.14) * kit.turbulence(n, child(r, 'sflut'), 200.0, 0.6))
    t_chunk = 0.03 + 0.008 * r.random()
    dry = layers((rec, 0.0, 0.0), (ring, 0.002, -24.0), (chunk, t_chunk, -10.0), (sizzle, 0.003, -24.0))
    dry = _early(dry, r, -12.0, 0.04, 4000.0)
    # The yard's buildings, then hills and the valley out to ~250 m: a boom that rolls for a second and a half.
    s1 = 0.07 + 0.04 * r.random()
    s2 = s1 + 0.12 + 0.08 * r.random()
    wet = _land(dry, r, 2.1, ((s1, -2.0), (s2, -4.0)), 26, 0.3, 1.5, -3.0, 1.4, 500.0, 0.3, -17.0, 110.0)
    return _master(layers(dry, wet), 5.0, 2.0, 0.0015, 0.05)


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
