"""Gun identity (2026-10-28, Docs/Work/2026-10-28-Core/GUNS.md lane D): the layers that tell the seven kinds apart by ear
beyond their shots. Names in Audio/LooterSoundCues.h (Gunshot::<Kind>Punch, <Kind>AimIn, Weapon::AimOut); the code that
plays them is Weapons/GunshotSound.cpp (Punch) and Weapons/WeaponSounds.h (AimIn by kind, falling back to Weapon.AimIn).

    Weapon.<Kind>.Punch    2D, the shooter's own gun: the shove of the shot in the body, under Core and Low. A short
                           recorded body punch's front (Impact_PunchBody/Impact_Body, low-passed: the thud, not the slap)
                           over a synthesized push of air and the chest's own short ring at the kind's pitch (one pulse,
                           never a swept sine). Rifle 80 Hz 60 ms (every 3rd shot, 4 dB down in code), LMG 60 Hz 90 ms
                           (every 2nd), revolver 70 Hz 120 ms with a dry crack, shotgun 50 Hz 220 ms, sniper 45 Hz
                           250 ms, launcher 40 Hz 300 ms with a whoomph of air. The SMG has none.
    Weapon.<Kind>.AimIn    the sights coming up, by kind: SMG a light polymer tick; rifle cloth and a rail tap; LMG a
                           heavy shoulder thump and the belt's jingle; revolver the hammer cocked (two clicks: the
                           six-gun's tell); shotgun the stock's thump and a wood creak; sniper the cheek on the stock
                           and the scope's ring; launcher the tube's clank on the shoulder.
    Weapon.AimOut          the sights lowered (every kind): a falling cloth swish and the gun settling in the hands.

The punch's peak adds to the Core's and the Low's (the game plays them together and can't limit the sum), so it rises
over 3-6 ms (its top lands after the Core's crack), is overdriven for density (loud for its peak) and kept under its
level; check.py's gun table adds every own-gun layer up and flags a sum over -1 dBFS.
"""
import numpy as np

from lib.core import ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, filters as F, dist as D, modal as M
from lib import sample as S
from recipes import cue
from recipes import kit
from recipes import guns as G
from recipes import guns_impacts as REC
from recipes.guns_rec import _cloth_rec, _pat

_loud = G._loud


# --- The punch ---------------------------------------------------------------------------------------------------------

def _push(r, f, dur, rise):
    """The body shoved: one slow pressure pulse (a push and a pull of air, no tone) with only its lows kept, rising over
    rise seconds, and the chest's short ring at f under it (a damped resonance, its pitch fixed and a hair different
    every take, so it is a thud rather than a drum machine's falling kick)."""
    n = ns(dur + 0.03)
    t = times(n)
    heave = G._heave(child(r, 'heave'), 0.5 / f * jitter(r, 1.0, 0.08), f * 2.2, dur + 0.03)
    fr = f * jitter(r, 1.0, 0.04)
    ring = np.sin(2.0 * np.pi * fr * t) * np.exp(-6.9 * t / dur)
    ring = ring * (1.0 + 0.12 * N.smooth_random(n, child(r, 'rough'), 30.0))
    rum = F.filt(N.pink(n, child(r, 'rum')), F.lpn(f * 2.5, 4), F.hpn(f * 0.5, 2), extend=False)
    rum = rum * E.perc((n) / 48000.0, 0.004, dur * 0.7)
    x = layers((_loud(heave, 0.005), 0.0, 0.0), (_loud(ring, 0.005), 0.0, -3.0), (_loud(rum, 0.005), 0.0, -12.0))
    x = x[:n]
    env = np.clip(t / rise, 0.0, 1.0) ** 1.5
    x = x * env
    # Dense for its peak: a round, overdriven shove sits loud in the chest without topping the core's crack.
    x = D.drive(normalize(x), 5.0, 'tanh')
    return E.end_fade(x * np.exp(-6.9 * np.clip(t - dur * 0.6, 0.0, None) / dur), 0.02)


def _thud(v, r, lp, rec='Impact_PunchBody.wav'):
    """The front of a recorded body punch, low-passed to its thud: the knock that starts the shove at once."""
    x = REC._hit(rec, child(r, 'thud'), 0.1, 0.05)
    x = S.gate_tail(x, 0.012, 0.03)
    return F.filt(x, F.hpn(60.0, 2), F.lpn(lp, 4), extend=False)


def _punch(v, r, f, dur, rise, thud_db, lp, rec='Impact_PunchBody.wav'):
    push = _push(child(r, 'push'), f, dur, rise)
    thud = _thud(v, child(r, 'rec'), lp, rec)
    x = layers((_loud(push, 0.01), 0.0, 0.0), (_loud(thud, 0.005), 0.0, thud_db))
    return F.filt(x, F.lpn(lp * 1.4, 2), extend=False)


# Per kind: the ring's pitch (Hz) and length (s), how fast it rises, the recorded thud's level against the push and its
# low-pass (lighter kinds keep a little more knock), the recording. level: loud enough to feel, with the own-gun sum's
# worst peak under -1 dBFS (check.py's gun table).
PUNCH = {
    'Rifle': dict(f=80.0, dur=0.06, rise=0.012, thud_db=-6.0, lp=1400.0, level=-23.0),
    'LMG': dict(f=60.0, dur=0.09, rise=0.018, thud_db=-7.0, lp=1200.0, level=-24.0),
    'Revolver': dict(f=70.0, dur=0.12, rise=0.02, thud_db=-6.0, lp=1300.0, level=-20.0),
    'Shotgun': dict(f=50.0, dur=0.22, rise=0.03, thud_db=-8.0, lp=1000.0, level=-19.0, rec='Impact_Body.wav'),
    'Sniper': dict(f=45.0, dur=0.25, rise=0.03, thud_db=-8.0, lp=1000.0, level=-19.0, rec='Impact_Body.wav'),
    'Launcher': dict(f=40.0, dur=0.30, rise=0.035, thud_db=-9.0, lp=900.0, level=-19.0, rec='Impact_Body.wav'),
}


def _register_punch(kind, p):
    @cue(f'Weapon.{kind}.Punch', variations=4, space='2D', jitter=0.02, conc=3, level=p['level'])
    def punch(v, r):
        x = _punch(v, r, p['f'], p['dur'], p['rise'], p['thud_db'], p['lp'], p.get('rec', 'Impact_PunchBody.wav'))
        if kind == 'Revolver':
            # The .44's dry crack on top of the shove: a few ms of bright snap, kept above the ear's touchy band.
            crack = kit.burst(0.006, child(r, 'crack'), 0.0015, lo=5500.0, hi=12000.0)
            x = layers(x, (kit.set_level(crack, x, -16.0, 0.002), 0.0))
        if kind == 'Launcher':
            # The whoomph: the back-blast's air shoved past the shoulder, a dark swell under the push.
            air = kit.whoosh(0.28, child(r, 'air'), 140.0, 520.0, 0.8, 0.22, -3.0, 1.0)
            x = layers(x, (kit.set_level(air, x, -8.0, 0.02), 0.004))
        return x


for _kind, _p in PUNCH.items():
    _register_punch(_kind, _p)


# --- Aiming ------------------------------------------------------------------------------------------------------------
# Each opens on the small physical touch that starts it (a tick, a knock) so it answers the button at once; the cloth and
# rings come after. 2-5 kHz eased: the sights come up hundreds of times a session.
# 2026-10-29: a recorded T-shirt's swish (guns_rec._cloth_rec, the 2026 bundle) under each synthesized swish, the
# synthesized one 3 dB down and the recording 1 dB under where it was, so the cloth sounds worn and real while the ticks
# and taps stay on top; the heavier kinds also land the stock on a recorded pat. Their streams are drawn last, after
# the synthesized layers', so those stay as they were.

def _ease(x, cut_db=-5.0, shelf_db=-3.0):
    return F.filt(x, F.peak(3300.0, cut_db, 0.8), F.highshelf(9000.0, shelf_db), extend=False)


def _swish(r, dur, lo=250.0, hi=2200.0, attack=0.25):
    return kit.cloth(dur, child(r, 'cloth'), lo, hi, 160.0, attack, 0.5)


@cue('Weapon.SMG.AimIn', variations=3, att='Near', jitter=0.04, conc=2, level=-21.0)
def smg_aim_in(v, r):
    # A light gun snapped up: the polymer grip's hollow tick in the palm, a quick little swish, the stock's pad on the
    # cheek: small and quick.
    tick = kit.thunk(1150.0 * jitter(r, 1, 0.06), child(r, 'tick'), 0.018, 0.0002, 5, 0.08)
    shine = kit.metal_click(child(r, 'shine'), 5600.0, 9500.0, 0.012, 0.00004, 3, -18.0)
    pad = kit.thunk(520.0 * jitter(r, 1, 0.08), child(r, 'pad'), 0.03, 0.0012)
    t = 0.055 + 0.01 * r.random()
    x = layers((tick, 0.0, 0.0), (shine, 0.001, -12.0), (_swish(r, 0.09, 400.0, 2600.0, 0.2), 0.0, -12.0), (pad, t, -6.0))
    rec = _cloth_rec(child(r, 'rec'), 0.09, v, 1.3, 0.2, 0.5, 400.0)
    return _ease(layers(x, (rec, 0.002, -10.0)))


@cue('Weapon.Rifle.AimIn', variations=3, att='Near', jitter=0.03, conc=2, level=-19.0)
def rifle_aim_in(v, r):
    # Shouldering the rifle: a short cloth swish, the cheek on the stock, the off hand's fingers tapping the rail.
    touch = kit.thunk(700.0 * jitter(r, 1, 0.06), child(r, 'touch'), 0.02, 0.0005, 5, 0.06)
    cheek = kit.thunk(300.0 * jitter(r, 1, 0.1), child(r, 'cheek'), 0.04, 0.002)
    rail = kit.metal_click(child(r, 'rail'), 1300.0, 3600.0, 0.016, 0.0001, 5, -16.0)
    t = 0.1 + 0.02 * r.random()
    x = layers((touch, 0.0, -10.0), (_swish(r, 0.16), 0.0, -7.0), (cheek, t, -6.0), (rail, t + 0.018, -10.0))
    rec = _cloth_rec(child(r, 'rec'), 0.16, v, 1.2, 0.25)
    pat, pa = _pat(child(r, 'pat'))
    return _ease(layers(x, (rec, 0.002, -5.0), (pat, max(0.0, t - pa), -12.0)))


@cue('Weapon.LMG.AimIn', variations=3, att='Near', jitter=0.03, conc=2, level=-17.5)
def lmg_aim_in(v, r):
    # The heavy gun hauled to the shoulder: cloth dragged, a solid thump as the stock lands (the weight in it), the belt's
    # rounds jingling as it settles.
    grab = kit.thunk(420.0 * jitter(r, 1, 0.06), child(r, 'grab'), 0.03, 0.0008)
    swish = _swish(r, 0.22, 200.0, 1800.0, 0.3)
    thump = kit.noise_thump(0.14, child(r, 'thump'), 700.0, 95.0, 0.06, 1.0, 0.05, 0.002)
    stock = kit.thunk(210.0 * jitter(r, 1, 0.06), child(r, 'stock'), 0.06, 0.0015)
    belt = kit.jingle(0.16, child(r, 'belt'), 4 + int(r.integers(0, 2)), 1700.0, 6500.0, 0.03, 0.06, 0.004)
    t = 0.13 + 0.02 * r.random()
    x = layers((grab, 0.0, -8.0), (swish, 0.0, -9.0), (thump, t, 0.0), (stock, t, -4.0), (belt, t + 0.006, -13.0))
    rec = _cloth_rec(child(r, 'rec'), 0.22, v, 1.1, 0.3, 0.5, 200.0)
    pat, pa = _pat(child(r, 'pat'), 1800.0)
    return _ease(layers(x, (rec, 0.002, -7.0), (pat, max(0.0, t - pa), -10.0)), -6.0)


@cue('Weapon.Revolver.AimIn', variations=3, att='Near', jitter=0.02, conc=2, level=-18.0)
def revolver_aim_in(v, r):
    # The thumb draws the hammer back as the iron comes up: the first click (the sear catching at half), the cylinder
    # turning a chamber on under it, the second, firmer click at full cock. Two clicks: the six-gun's tell.
    # Their steel held under 3.4 kHz and a hair of shine over 5.5: clicks heard on every raise mustn't bite.
    first = kit.metal_click(child(r, 'half'), 1100.0, 3200.0, 0.018, 0.00012, 5, -12.0)
    turn = kit.metal_click(child(r, 'turn'), 1300.0, 3400.0, 0.012, 0.00012, 4, -16.0)
    full = G._clank(child(r, 'full'), (1000.0, 3200.0, 0.022), (620.0 * jitter(r, 1.0, 0.05), 0.02), (200.0, 0.02),
                    (0.0, -4.0, -10.0), 0.00008)
    shine = kit.metal_click(child(r, 'shine'), 5800.0, 9000.0, 0.01, 0.00004, 3, -18.0)
    swish = _swish(r, 0.12, 300.0, 2400.0, 0.2)
    gap = 0.075 + 0.015 * r.random()
    x = layers((first, 0.0, -2.0), (turn, gap * 0.55, -14.0), (full, gap, 0.0), (shine, gap, -14.0), (swish, 0.0, -17.0))
    rec = _cloth_rec(child(r, 'rec'), 0.12, v, 1.25, 0.2, 0.5, 300.0)
    return _ease(layers(x, (rec, 0.002, -15.0)), -8.0)


@cue('Weapon.Shotgun.AimIn', variations=3, att='Near', jitter=0.03, conc=2, level=-18.5)
def shotgun_aim_in(v, r):
    # The long gun's wooden stock shoved into the shoulder: a dull wood thump with the shoulder's give, the stock
    # creaking against the coat, the forend hand tightening.
    touch = kit.thunk(520.0 * jitter(r, 1, 0.06), child(r, 'touch'), 0.025, 0.0006)
    thump = kit.thunk(230.0 * jitter(r, 1, 0.06), child(r, 'stock'), 0.07, 0.002)
    give = kit.noise_thump(0.12, child(r, 'give'), 500.0, 90.0, 0.05, 1.0, 0.04, 0.002)
    creak = kit.creak(0.13, child(r, 'creak'), 55.0, 30.0, (330.0, 720.0, 1350.0), 8.0, 0.2)
    swish = _swish(r, 0.14, 220.0, 2000.0, 0.25)
    t = 0.09 + 0.02 * r.random()
    x = layers((touch, 0.0, -10.0), (swish, 0.0, -10.0), (thump, t, 0.0), (give, t, -6.0), (creak, t + 0.02, -12.0))
    rec = _cloth_rec(child(r, 'rec'), 0.14, v, 1.15, 0.25, 0.5, 220.0)
    pat, pa = _pat(child(r, 'pat'), 2000.0)
    x = layers(x, (rec, 0.002, -8.0), (pat, max(0.0, t - pa), -10.0))
    x = x * E.end_fade(np.ones(x.size), 0.05)
    return _ease(x)


@cue('Weapon.Sniper.AimIn', variations=3, att='Near', jitter=0.02, conc=2, level=-19.5)
def sniper_aim_in(v, r):
    # Settling in behind the glass: a slow cloth swish, the cheek welded to the stock (a soft press, held), and the
    # scope's rings giving a small, clear ring as the gun steadies.
    touch = kit.thunk(640.0 * jitter(r, 1, 0.06), child(r, 'touch'), 0.02, 0.0006, 5, 0.06)
    swish = _swish(r, 0.24, 220.0, 1900.0, 0.35)
    cheek = kit.noise_thump(0.1, child(r, 'cheek'), 600.0, 140.0, 0.05, 0.9, 0.05, 0.006)
    f0 = [1480.0, 1560.0, 1420.0][v]
    ring = M.strike(M.parts(f0, f0 * 1.31, child(r, 'ring'), 3, 0.18, 0.12, 0.2), M.hammer(0.0003), 0.35)
    shine = M.strike(M.parts(5800.0, 7600.0, child(r, 'shine'), 3, 0.08, 0.05, 0.2), M.hammer(0.0001), 0.15)
    t = 0.16 + 0.02 * r.random()
    x = layers((touch, 0.0, -10.0), (swish, 0.0, -8.0), (cheek, t, -3.0), (_loud(ring, 0.005), t + 0.01, -11.0),
               (_loud(shine, 0.005), t + 0.012, -20.0))
    rec = _cloth_rec(child(r, 'rec'), 0.24, v, 1.1, 0.35, 0.5, 220.0)
    pat, pa = _pat(child(r, 'pat'), 2000.0)
    x = layers(x, (rec, 0.002, -6.0), (pat, max(0.0, t - pa), -9.0))
    x = x * E.end_fade(np.ones(x.size), 0.02)
    return _ease(x, -6.0)


@cue('Weapon.Launcher.AimIn', variations=3, att='Near', jitter=0.03, conc=2, level=-18.0)
def launcher_aim_in(v, r):
    # The tube heaved onto the shoulder: the hand's knock on the steel, the long tube ringing hollow, a heavy thump
    # as it lands, the sling's buckle.
    knock = kit.thunk(480.0 * jitter(r, 1, 0.06), child(r, 'knock'), 0.025, 0.0006)
    f0 = 300.0 * jitter(r, 1.0, 0.05)
    tube = M.strike(M.shell(f0, child(r, 'tube'), 0.28, 8, 0.02), M.hammer(0.0006), 0.5)
    thump = kit.noise_thump(0.16, child(r, 'thump'), 650.0, 85.0, 0.07, 1.0, 0.05, 0.002)
    buckle = kit.metal_click(child(r, 'buckle'), 1400.0, 4200.0, 0.02, 0.0001, 4, -14.0)
    swish = _swish(r, 0.18, 200.0, 1800.0, 0.3)
    t = 0.12 + 0.02 * r.random()
    x = layers((knock, 0.0, -6.0), (swish, 0.0, -11.0), (thump, t, 0.0), (_loud(tube, 0.005), t + 0.002, -7.0),
               (buckle, t + 0.03, -16.0))
    rec = _cloth_rec(child(r, 'rec'), 0.18, v, 1.1, 0.3, 0.5, 200.0)
    pat, pa = _pat(child(r, 'pat'), 1800.0)
    x = layers(x, (rec, 0.002, -9.0), (pat, max(0.0, t - pa), -12.0))
    x = x * E.end_fade(np.ones(x.size), 0.03)
    return _ease(x, -6.0)


@cue('Weapon.AimOut', variations=3, att='Near', jitter=0.04, conc=2, level=-22.0)
def aim_out(v, r):
    # The sights lowered: the cheek leaving the stock (a soft tick of the grip turning in the hand), a cloth swish
    # falling away, the gun settling at the hip with a light knock.
    tick = kit.thunk(760.0 * jitter(r, 1, 0.06), child(r, 'tick'), 0.018, 0.0005, 5, 0.06)
    swish = kit.cloth(0.17, child(r, 'cloth'), 220.0, 1800.0, 150.0, 0.15, 0.7)
    settle = kit.thunk(360.0 * jitter(r, 1, 0.08), child(r, 'settle'), 0.04, 0.0015)
    t = 0.12 + 0.02 * r.random()
    x = layers((tick, 0.0, -8.0), (swish, 0.0, -6.0), (settle, t, -8.0))
    rec = _cloth_rec(child(r, 'rec'), 0.17, v, 1.15, 0.15, 0.7, 220.0)
    x = layers(x, (rec, 0.002, -4.0))
    x = x * E.end_fade(np.ones(x.size), 0.03)
    return _ease(x)
