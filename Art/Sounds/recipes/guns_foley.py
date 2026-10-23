"""The guns' handling (the gun feel pass, 2026-10-22): the reloads of the Bullpup rifle, the Ranchhand shotgun and the
Drover revolver rebuilt on recorded foley (the user's yes, 2026-10-22: reload and handling sounds may use the licensed
pack, like the shots), the Ranchhand's rack between shots, the Longhorn's own box magazine, the Rustler's charging knob,
the last-round sounds, the Thresher's hot barrel, the cloth at a reload's start and the draws and holsters by weight.

How a reload step is made: the recorded take carries the real detail (a polymer magazine's clatter, a charging handle's
steel, a pump's forend on its rails, a cylinder's latch); the recordings are close-miked and bright (their centroids sit
at 6-9 kHz, nothing under 150 Hz), so their air is eased and the 3.3 kHz band dipped, and the old synthesized clank's
body and knock (recipes/guns.py, _clank with its steel turned down) laid under the hit for the gun's weight. Each step's
loudest moment sits where the old synthesized step's did, so it still lands on Weapons/ReloadMotion.cpp's motion:

    Weapon.Rifle.MagOut     the release clicks at once, the magazine unlatches at 0.06 s
    Weapon.Rifle.MagIn      seated at 0.06 s
    Weapon.Rifle.Bolt       the handle yanked at 0.03 s, slammed home at 0.19 s (the empty reload's charging handle)
    Weapon.Shotgun.ShellIn  the gate snaps behind the shell at 0.05 s
    Weapon.Shotgun.Pump     back at 0.10 s, home at about 0.32 s (the reload's last step, only when it ran dry)
    Weapon.Shotgun.Rack     back at once, home at 0.18-0.21 s (between shots: the rack's back and home beats)
    Weapon.Revolver.*       out at 0.13 s; the rod at once; rounds at 0.04 s; home at 0.03 s
    Weapon.Sniper.MagOut/In the heavier box: unlatched at 0.07 s, seated at 0.07 s
    Weapon.SMG.Charge       the knob drawn at once, let go home at 0.13 s

The last-round sounds play with the magazine's last shot (Weapons/GunshotSound.cpp's low-ammo hook, at 0 rounds): each
starts with a small tick at once and puts what tells the player (a bolt locking back, an empty tube's spring, a helix
winding down, a belt's tail) 40-90 ms on, after the shot's front.

Recordings: Art/Sounds/packs/Sonniss/Foley_*.wav, SMG_TMP_Bolt.wav, Rifle_Cock.wav, Foley_Brass_Drop.wav (sources and
licence in Art/Sounds/packs/README.md).
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, fade, varispeed
from lib import noise as N, env as E, filters as F, dist as D, modal as M
from lib import granular as GR
from lib import sample as S
from recipes import cue
from recipes import kit
from recipes import guns as G
from recipes import guns_smg as SMG
from recipes import guns_lmg as LMG
from recipes.guns_rec import _loud, _take, _gated, _ping

GAP = 0.25  # the silence make_pack.py puts between an excerpt's windows

# Excerpts whose windows are all the same length with their event (the loudest moment) the same time in:
# name: (window length, lead to the event, windows).
_FOLEY = {
    'Foley_G36C_MagIn.wav': (0.5, 0.2, 6),
    'Foley_G36C_MagOut.wav': (0.8, 0.5, 6),
    'Foley_Saiga_Pull.wav': (0.35, 0.05, 6),
    'Foley_Saiga_Release.wav': (0.5, 0.15, 6),
    'Foley_M29_Open.wav': (0.32, 0.12, 6),
    'Foley_M29_Close.wav': (0.31, 0.06, 6),
}
# Excerpts with events at their own times (s into the excerpt; make_pack.py prints them): where to look for each.
_EVENTS = {
    'Foley_TS_Pump.wav': [((0.0, 0.2), (0.25, 0.45)), ((0.88, 1.06), (1.1, 1.26))],   # (back, home) per rack
    'Foley_TS_ShellLoad.wav': [(0.2, 0.5), (1.25, 1.55)],                              # the gate's snap per shell
    'Foley_Saiga_Rack.wav': [((0.06, 0.18), (0.4, 0.52)), ((1.3, 1.42), (1.64, 1.76))],  # (pulled, home) per rack
}


def _loudest(x, t0, t1):
    """The time (s) of x's loudest sample between t0 and t1."""
    s0 = max(0, ns(t0))
    s1 = min(x.size, ns(t1))
    return (s0 + int(np.argmax(np.abs(x[s0:s1])))) / SR


def _cut(x, t, before, after, lo=0.0, hi=None):
    """x from before seconds ahead of t to after seconds past it (kept inside lo..hi), softly edged; and where t falls."""
    hi = x.size / SR if hi is None else hi
    a = max(lo, t - before)
    b = min(hi, t + after)
    y = fade(x[ns(a):ns(b)].copy(), 0.002, min(0.04, 0.3 * (b - a)))
    return y, t - a


def _foley(name, k, before, after, speed=1.0):
    """Take k of an evenly windowed excerpt around its event, played speed times faster; (samples, the event's time)."""
    length, lead, count = _FOLEY[name]
    x = S.load(name)
    start = (k % count) * (length + GAP)
    t = _loudest(x, start + lead - 0.025, start + lead + 0.025)
    y, at = _cut(x, t, before, after, start, start + length)
    if speed != 1.0:
        y = varispeed(y, speed)
    return y, at / speed


def _tame(x, shelf_db=-6.0, dip_db=-4.0, hp=150.0):
    """A close-miked foley take made to sit with the game's guns: its fizz above 7 kHz eased, the ear's touchy band
    dipped, its rumble and handling noise cut."""
    return F.filt(x, F.hpn(hp, 2), F.peak(3300.0, dip_db, 0.8), F.highshelf(7000.0, shelf_db), extend=False)


def _weight(r, f_body, f_knock, t60=0.05, knock_db=-2.0, drive_db=5.0):
    """The gun's mass under a recorded hit: the old clank with its steel turned well down (the recording has the steel),
    leaving the wood or polymer body and the low knock."""
    return G._clank(r, (800.0, 2600.0, t60 * 0.6), (f_body * jitter(r, 1.0, 0.05), t60), (f_knock * jitter(r, 1.0, 0.05),
                    t60 * 1.2), (-14.0, 0.0, knock_db), 0.00015, -30.0, drive_db)


def _place(sig, event_at, want_at):
    """Where to put sig so its event (event_at seconds in) lands at want_at; never before 0."""
    return max(0.0, want_at - event_at)


# --- The Bullpup rifle's magazine and charging handle ------------------------------------------------------------------

@cue('Weapon.Rifle.MagOut', variations=4, att='Near', jitter=0.03, conc=2, level=-14.0)
def rifle_mag_out(v, r):
    # The thumb presses the release (a small steel click at once), the catch lets go and the polymer magazine unlatches
    # and slides out of the well (the recorded G36C's), the rounds inside knocking against its walls.
    button = G._clank(child(r, 'button'), (1400.0, 4200.0, 0.02), (700.0, 0.02), (240.0, 0.02), (0.0, -6.0, -12.0),
                      tick_db=-12.0)
    rec, at = _foley('Foley_G36C_MagOut.wav', v + (v // 3), 0.05, 0.28, jitter(r, 1.0, 0.03))
    rec = _tame(rec)
    t = 0.06
    body = _weight(child(r, 'body'), 380.0, 150.0, 0.05)
    rounds = G._rounds(r, 3, 0.04)
    return layers((button, 0.0, -9.0), (_loud(rec), _place(rec, at, t), 0.0), (body, t, -6.0), (rounds, t + 0.02, -18.0))


@cue('Weapon.Rifle.MagIn', variations=4, att='Near', jitter=0.03, conc=2, level=-12.0)
def rifle_mag_in(v, r):
    # The fresh magazine's lips find the well and it is slapped home (the recorded G36C's seat: polymer and steel), the
    # palm's thud and the whole gun's knock under it, the rounds settling.
    rec, at = _foley('Foley_G36C_MagIn.wav', v + (v // 3), 0.06, 0.3, jitter(r, 1.0, 0.03))
    rec = _tame(rec)
    t = 0.06
    seat = _weight(child(r, 'seat'), 330.0, 135.0, 0.06, -1.0, 6.0)
    palm = _loud(kit.noise_thump(0.06, child(r, 'palm'), 1400.0, 300.0, 0.025, 0.9, 0.02, 0.0004))
    rounds = G._rounds(r, 2, 0.02)
    return layers((_loud(rec), _place(rec, at, t), 0.0), (seat, t, -4.0), (palm, t, -9.0), (rounds, t + 0.01, -20.0))


@cue('Weapon.Rifle.Bolt', variations=4, att='Near', jitter=0.03, conc=2, level=-11.0)
def rifle_bolt(v, r):
    # The empty reload's charging handle: fingers catch it and yank it back (a recorded rifle handle's pull), the spring
    # slams the bolt home into battery (its release): chk-CHAK, the gun's mass thudding under the slam, the spring buzzing.
    t_pull, t_slam = 0.03, 0.19 + 0.015 * r.random()
    if v < 2:
        # A whole recorded rack, its two hits re-spaced to the reload's beat.
        x = S.load('Foley_Saiga_Rack.wav')
        (p0, p1), (h0, h1) = _EVENTS['Foley_Saiga_Rack.wav'][v]
        tp, th = _loudest(x, p0, p1), _loudest(x, h0, h1)
        pull, pa = _cut(x, tp, 0.03, min(0.14, th - tp - 0.05))
        home, ha = _cut(x, th, 0.04, 0.3, tp + 0.15)
    else:
        pull, pa = _foley('Foley_Saiga_Pull.wav', 2 * v, 0.03, 0.14)
        home, ha = _foley('Foley_Saiga_Release.wav', 2 * v + 1, 0.04, 0.3)
    pull, home = _tame(pull, -7.0), _tame(home, -7.0)
    slam = _weight(child(r, 'slam'), 360.0, 125.0, 0.07, -1.0, 6.0)
    stop = _weight(child(r, 'stop'), 480.0, 170.0, 0.035, -6.0)
    spring = M.strike(M.parts(1800.0, 3200.0, child(r, 'spring'), 3, 0.14, 0.1, 0.2), M.hammer(0.0002), 0.2)
    grab = kit.metal_click(child(r, 'grab'), 1600.0, 5000.0, 0.015, 0.0001, 4, -12.0)
    return layers((_loud(grab, 0.005), 0.0, -14.0), (_loud(pull), _place(pull, pa, t_pull), -4.0), (stop, t_pull, -8.0),
                  (_loud(home), _place(home, ha, t_slam), 0.0), (slam, t_slam, -3.0),
                  (_loud(spring), t_slam + 0.002, -26.0))


# --- The Ranchhand pump shotgun: shells, the reload's pump, the rack between shots ---------------------------------------

def _pump(v, r, back_at, home_at, speed=1.0):
    """A recorded pump stroke (the forend back: the action unlocking, the hull kicked out; then shoved home into lock),
    its two hits placed at back_at and home_at, with the gun's mass under the lock and the hull's clack."""
    x = S.load('Foley_TS_Pump.wav')
    (b0, b1), (h0, h1) = _EVENTS['Foley_TS_Pump.wav'][v % 2]
    tb, th = _loudest(x, b0, b1), _loudest(x, h0, h1)
    back, ba = _cut(x, tb, 0.012, th - tb - 0.06)
    home, ha = _cut(x, th, 0.05, 0.3, tb + 0.08)
    if speed != 1.0:
        back, home, ba, ha = varispeed(back, speed), varispeed(home, speed), ba / speed, ha / speed
    back, home = _tame(back, -6.0, -4.0, 120.0), _tame(home, -6.0, -4.0, 120.0)
    rear = _weight(child(r, 'rear'), 300.0, 120.0, 0.06, -3.0)
    lock = _weight(child(r, 'lock'), 270.0, 105.0, 0.08, 0.0, 6.0)
    hull = _loud(layers(kit.thunk(900.0, child(r, 'hull'), 0.03, 0.0003),
                        (kit.brass_tick(child(r, 'head'), 1700.0, 0.025, 3, 0.0002, -14.0, -18.0, 0.1), 0.0, -6.0)))
    slide = G._slide(max(0.03, home_at - back_at - 0.07), child(r, 'fwd'), 800.0, 540.0, 0.3, 0.2)
    return layers((_loud(back), _place(back, ba, back_at), -2.0), (rear, back_at, -8.0), (hull, back_at + 0.012, -14.0),
                  (slide, back_at + 0.04, -24.0), (_loud(home), _place(home, ha, home_at), 0.0), (lock, home_at, -3.0))


@cue('Weapon.Shotgun.ShellIn', variations=4, att='Near', jitter=0.04, conc=2, level=-14.0)
def shotgun_shell_in(v, r):
    # A shell thumbed past the loading gate into the tube (a recorded 12 gauge's): the hull slides in against the spring,
    # the gate snaps shut behind it with a solid clack, the gun's knock under it.
    x = S.load('Foley_TS_ShellLoad.wav')
    t0, t1 = _EVENTS['Foley_TS_ShellLoad.wav'][v % 2]
    rec, at = _cut(x, _loudest(x, t0, t1), 0.05, 0.22)
    rec = _tame(varispeed(rec, (1.0, 1.0, 0.94, 0.94)[v] * jitter(r, 1.0, 0.02)), -7.0, -6.0)
    at /= (1.0, 1.0, 0.94, 0.94)[v]
    t = 0.05
    gate = _weight(child(r, 'gate'), 430.0, 150.0, 0.04, -3.0)
    rim = _loud(kit.brass_tick(child(r, 'rim'), 1250.0 * jitter(r, 1.0, 0.06), 0.03, 4, 0.00015, -6.0, -14.0, 0.2))
    return layers((rim, 0.0, -16.0), (_loud(rec), _place(rec, at, t), 0.0), (gate, t, -5.0))


@cue('Weapon.Shotgun.Pump', variations=3, att='Near', jitter=0.03, conc=2, level=-9.0)
def shotgun_pump(v, r):
    # The reload's last step when the tube ran dry: the forend racked back (the action unlocks, the bars ride the rails,
    # the bolt hits the rear) and shoved forward into lock: the heaviest clack in the kit. Unhurried: back at 0.1 s.
    return _pump(v, r, 0.1, 0.31 + 0.02 * r.random(), (1.0, 1.0, 0.93)[v])


@cue('Weapon.Shotgun.Rack', variations=4, att='Near', jitter=0.03, conc=2, level=-9.5)
def shotgun_rack(v, r):
    # Between shots (the Ranchhand pumps after every one): the forend snapped back at once, the spent hull kicked out,
    # then shoved home, two quick hard beats; the rack's length in the game is 0.22-0.5 s, so home lands by 0.21 s.
    return _pump(v, r, 0.0, (0.19, 0.2, 0.18, 0.21)[v], (1.06, 1.0, 1.1, 0.97)[v])


# --- The Drover revolver's cylinder ------------------------------------------------------------------------------------

@cue('Weapon.Revolver.CylinderOut', variations=4, att='Near', jitter=0.03, conc=2, level=-14.0)
def revolver_cylinder_out(v, r):
    # The thumb pushes the latch (a small click at once), the cylinder swings out on its crane (a recorded .44's), stopping
    # against its limit, the rounds shifting in their chambers.
    latch = G._clank(child(r, 'latch'), (1500.0, 4400.0, 0.018), (750.0, 0.018), (240.0, 0.02), (0.0, -6.0, -12.0),
                     tick_db=-12.0)
    rec, at = _foley('Foley_M29_Open.wav', v + (v // 3), 0.1, 0.18, jitter(r, 1.0, 0.03))
    rec = _tame(rec, -5.0)
    t = 0.13
    swing = G._slide(0.1, child(r, 'swing'), 650.0, 980.0, 0.2, 0.4)
    stop = _weight(child(r, 'stop'), 420.0, 160.0, 0.045, -5.0)
    rounds = G._rounds(r, 4, 0.05)
    return layers((latch, 0.0, -7.0), (swing, 0.02, -22.0), (_loud(rec), _place(rec, at, t), 0.0), (stop, t, -6.0),
                  (rounds, t + 0.006, -15.0))


@cue('Weapon.Revolver.Eject', variations=3, att='Near', jitter=0.04, conc=2, level=-12.0)
def revolver_eject(v, r):
    # The palm slaps the ejector rod (a solid steel knock and the spring's twang); the six empties slide out together and
    # drop, recorded brass landing and bouncing apart under the synthesized patter.
    slap = _loud(kit.noise_thump(0.05, child(r, 'palm'), 1200.0, 260.0, 0.02, 0.9, 0.015, 0.0004))
    rod = G._clank(child(r, 'rod'), (900.0, 3200.0, 0.045), (380.0, 0.05), (140.0, 0.05), (0.0, -1.0, -3.0), drive_db=5.0)
    spring = M.strike(M.parts(1700.0, 3200.0, child(r, 'spring'), 3, 0.12, 0.08, 0.2), M.hammer(0.0002), 0.18)
    out = G._slide(0.03, child(r, 'out'), 1100.0, 1500.0, 0.2, 0.4)

    def empty(rr, i):
        return kit.brass_tick(rr, 2300.0 * jitter(rr, 1.0, 0.15), 0.06, 4, 0.00008, -14.0, -10.0, 0.5)
    land = GR.rattle(0.45, child(r, 'land'), 7, 0.35, empty, 0.0, 0.82)
    drop = S.load('Foley_Brass_Drop.wav')
    rr = child(r, 'drops')
    brass = np.zeros(1)
    for i in range(3):
        d = _tame(varispeed(drop, rr.uniform(1.05, 1.35)), -4.0, -6.0, 400.0)
        brass = layers(brass, (_loud(d), rr.uniform(0.0, 0.09), rr.uniform(-6.0, 0.0)))
    t_land = 0.22 + 0.04 * r.random()
    return layers((slap, 0.0, -6.0), (rod, 0.0, 0.0), (_loud(spring), 0.002, -22.0), (out, 0.01, -18.0),
                  (_loud(land), t_land, -14.0), (_loud(brass), t_land + 0.01, -11.0))


@cue('Weapon.Revolver.RoundsIn', variations=3, att='Near', jitter=0.04, conc=2, level=-14.0)
def revolver_rounds_in(v, r):
    # A speedloader: six rounds drop into their chambers together (a quick brass shuffle and their seating knock), the
    # knob is twisted to let them go (a recorded cylinder click, small), and the empty loader is pulled away.
    def round_in(rr, i):
        return kit.brass_tick(rr, 1500.0 * jitter(rr, 1.0, 0.12), 0.025, 4, 0.0001, -8.0, -14.0, 0.2)
    drop = GR.rattle(0.06, child(r, 'drop'), 6, 0.03, round_in, 0.0, 0.92)
    slide = G._slide(0.035, child(r, 'slide'), 700.0, 520.0, 0.3, 0.4)
    seat = G._clank(child(r, 'seat'), (1000.0, 3200.0, 0.03), (480.0, 0.035), (170.0, 0.04), (-4.0, 0.0, -6.0))
    knob, at = _foley('Foley_M29_Open.wav', v + 3, 0.02, 0.1, 1.25)
    knob = _tame(knob, -6.0, -6.0, 300.0)
    away = kit.thunk(620.0 * jitter(r, 1.0, 0.08), child(r, 'away'), 0.03, 0.001)
    t_seat = 0.035 + 0.008 * r.random()
    t_knob = t_seat + 0.09 + 0.02 * r.random()
    return layers((slide, 0.0, -20.0), (_loud(drop), 0.004, -6.0), (seat, t_seat, -2.0),
                  (_loud(knob), _place(knob, at, t_knob), -8.0), (away, t_knob + 0.06, -16.0))


@cue('Weapon.Revolver.CylinderIn', variations=4, att='Near', jitter=0.03, conc=2, level=-11.0)
def revolver_cylinder_in(v, r):
    # The cylinder slapped home (a recorded .44's snap): the crane hits the frame with the heaviest clack in the
    # revolver's kit, the latch catching it, and the cylinder ticks round as the bolt finds its notch.
    rec, at = _foley('Foley_M29_Close.wav', v + (v // 3), 0.03, 0.25, jitter(r, 1.0, 0.03))
    rec = _tame(rec, -8.0, -5.0)
    t = 0.03
    home = _weight(child(r, 'home'), 330.0, 125.0, 0.07, -1.0, 6.0)
    tick = kit.metal_click(child(r, 'tick'), 2000.0, 5600.0, 0.015, 0.0001, 4, -10.0)
    swing = G._slide(0.025, child(r, 'swing'), 980.0, 700.0, 0.3, 0.2)
    return layers((swing, 0.0, -22.0), (_loud(rec), _place(rec, at, t), 0.0), (home, t, -3.0),
                  (_loud(tick), t + 0.06 + 0.015 * r.random(), -16.0))


# --- The Longhorn's box magazine, the Rustler's knob -------------------------------------------------------------------

def _box(r, f0, t60):
    """A steel box magazine's hollow ring when it's knocked."""
    return M.strike(M.shell(f0 * jitter(r, 1.0, 0.05), child(r, 'box'), t60, 6, 0.03), M.hammer(0.00015), t60 * 2.5)


@cue('Weapon.Sniper.MagOut', variations=3, att='Near', jitter=0.03, conc=2, level=-13.0)
def sniper_mag_out(v, r):
    # The Longhorn's steel box: the release lever thumbed (a heavy click at once), the box drops out of the well into the
    # palm (a recorded magazine played slower and lower: a bigger box), its hollow steel ring, the fat rounds knocking.
    lever = G._clank(child(r, 'lever'), (1200.0, 3800.0, 0.025), (560.0, 0.025), (190.0, 0.025), (0.0, -5.0, -10.0),
                     tick_db=-12.0)
    rec, at = _foley('Foley_G36C_MagOut.wav', v + 3, 0.05, 0.3, 0.8)
    rec = _tame(rec, -8.0, -5.0)
    t = 0.07
    body = _weight(child(r, 'body'), 300.0, 120.0, 0.06, -1.0)
    rounds = G._rounds(child(r, 'rounds'), 2, 0.04)
    return layers((lever, 0.0, -8.0), (_loud(rec), _place(rec, at, t), 0.0), (body, t, -4.0),
                  (_loud(_box(r, 520.0, 0.07), 0.005), t + 0.002, -13.0), (rounds, t + 0.025, -20.0))


@cue('Weapon.Sniper.MagIn', variations=3, att='Near', jitter=0.03, conc=2, level=-11.0)
def sniper_mag_in(v, r):
    # The full box shoved up into the well and slapped home (a recorded seat, slower and lower), the catch snapping over
    # its lip, steel on steel with the stock's thud under it.
    rec, at = _foley('Foley_G36C_MagIn.wav', v + 3, 0.06, 0.3, 0.8)
    rec = _tame(rec, -8.0, -5.0)
    t = 0.07
    lip = kit.metal_click(child(r, 'lip'), 1400.0, 4200.0, 0.015, 0.0001, 4, -12.0)
    seat = _weight(child(r, 'seat'), 290.0, 115.0, 0.07, 0.0, 6.0)
    catch = G._clank(child(r, 'catch'), (1300.0, 3800.0, 0.025), (700.0, 0.02), (240.0, 0.02), (0.0, -6.0, -12.0))
    return layers((_loud(lip, 0.005), 0.0, -16.0), (_loud(rec), _place(rec, at, t), 0.0), (seat, t, -3.0),
                  (_loud(_box(r, 480.0, 0.08), 0.005), t, -14.0),
                  (catch, t + 0.012, -10.0))


# The TMP's bolt cycles in SMG_TMP_Bolt.wav (guns_smg.SHOTS' indexes): drawn back, then let go home.
_TMP_CYCLES = [(0, 1), (3, 4), (6, 7), (8, 9), (10, 11), (12, 13)]


@cue('Weapon.SMG.Charge', variations=3, att='Near', jitter=0.03, conc=2, level=-11.0)
def smg_charge(v, r):
    # The empty reload's knob: drawn back against the spring (a recorded open bolt's), let go to slam home, and the fresh
    # helix's spring winding the first round up against the bolt (a quick whirr).
    a, b = _TMP_CYCLES[(2 * v + 1) % len(_TMP_CYCLES)]
    pull = _tame(_gated(_take('SMG_TMP_Bolt.wav', a, 0.12), 0.05, 0.05, hp=250.0), -6.0)
    home = _tame(_gated(_take('SMG_TMP_Bolt.wav', b, 0.2), 0.05, 0.1, hp=250.0), -6.0)
    t_home = 0.13 + 0.015 * r.random()
    slam = _weight(child(r, 'slam'), 700.0, 230.0, 0.025, -6.0)
    whirr = SMG._whirr(child(r, 'whirr'), 0.07, 300.0 * jitter(r, 1.0, 0.05), -8.0)
    grab = kit.metal_click(child(r, 'grab'), 2000.0, 6000.0, 0.012, 0.0001, 4, -12.0)
    return layers((_loud(grab, 0.005), 0.0, -12.0), (_loud(pull), 0.002, -3.0), (_loud(home), t_home, 0.0),
                  (slam, t_home, -6.0), (whirr, t_home + 0.03, -12.0))


# --- The last round ----------------------------------------------------------------------------------------------------

def _sear(r, db=-14.0):
    """The tick every last-round sound starts on, so it begins with the shot."""
    return (_loud(kit.metal_click(child(r, 'sear'), 2400.0, 7000.0, 0.008, 0.00005, 4, -12.0), 0.003), 0.0, db)


@cue('Weapon.Rifle.LastRound', variations=3, att='Near', jitter=0.02, conc=2, level=-10.0)
def rifle_last_round(v, r):
    # The magazine's last round gone: the follower pushes the bolt catch up and the bolt locks back on it with a hard
    # steel clack (a recorded handle's pull, smaller), and the empty polymer magazine rings hollow.
    rec, at = _foley('Foley_Saiga_Pull.wav', v, 0.01, 0.22, 1.2)
    rec = _tame(rec, -10.0, -8.0, 300.0)
    t = 0.05 + 0.01 * r.random()
    catch = _weight(child(r, 'catch'), 520.0, 180.0, 0.04, -4.0)
    hollow = _ping(child(r, 'hollow'), [980.0, 1040.0, 930.0][v], 0.16, -14.0)
    return layers(_sear(r), (_loud(rec), _place(rec, at, t), -2.0), (catch, t, -4.0), (_loud(hollow, 0.005), t + 0.01, -3.0))


@cue('Weapon.Shotgun.LastRound', variations=3, att='Near', jitter=0.02, conc=2, level=-10.0)
def shotgun_last_round(v, r):
    # The tube's last shell gone: the follower runs forward on its spring and knocks the tube's end, an empty steel tube
    # ringing low with the spring's buzz.
    knock = _weight(child(r, 'knock'), 380.0, 140.0, 0.04, -3.0)
    tube = M.strike(M.shell([610.0, 650.0, 580.0][v], child(r, 'tube'), 0.12, 6, 0.02), M.hammer(0.0002), 0.3)
    tube = F.filt(tube, F.peak(3300.0, -8.0, 0.7), extend=False)
    spring = M.strike(M.parts(1500.0, 2600.0, child(r, 'spring'), 3, 0.16, 0.1, 0.2), M.hammer(0.0002), 0.25)
    t = 0.06 + 0.01 * r.random()
    return layers(_sear(r), (knock, t, 0.0), (_loud(tube, 0.005), t, -3.0), (_loud(spring), t + 0.003, -18.0))


@cue('Weapon.Revolver.LastRound', variations=3, att='Near', jitter=0.02, conc=2, level=-11.0)
def revolver_last_round(v, r):
    # The sixth chamber spent: the hammer down, the hand turns the cylinder onto an empty chamber and the bolt drops into
    # its notch: a recorded cylinder's small snap and a short steel ring.
    rec, at = _foley('Foley_M29_Close.wav', v + 2, 0.01, 0.12, 1.15)
    rec = _tame(rec, -9.0, -6.0, 400.0)
    t = 0.08 + 0.01 * r.random()
    ring = M.strike(M.parts(1150.0, 1900.0, child(r, 'ring'), 4, 0.1, 0.05, 0.3), M.hammer(0.00008), 0.25)
    turn = kit.metal_click(child(r, 'turn'), 1800.0, 5200.0, 0.02, 0.0001, 5, -10.0)
    return layers(_sear(r), (_loud(turn, 0.005), t - 0.03, -10.0), (_loud(rec), _place(rec, at, t), 0.0),
                  (_loud(ring, 0.005), t, -10.0))


@cue('Weapon.SMG.LastRound', variations=3, att='Near', jitter=0.02, conc=2, level=-11.0)
def smg_last_round(v, r):
    # The helix runs out: its spring unwinds with nothing to push, the follower's ratchet ticking slower and slower and the
    # rotary hum falling, ending on a slack knock.
    dur = 0.32
    n = ns(dur)
    t = times(n)
    rot = 360.0 * jitter(r, 1.0, 0.05) * np.exp(-t / 0.12) + 40.0
    ph = 2.0 * np.pi * np.cumsum(rot) / SR
    hum = sum(np.sin(h * ph + r.uniform(0, 6.28)) / h for h in (3, 4, 5))
    hum = F.filt(hum, F.bp(1100.0, 1.0), extend=False) * E.end_fade(E.perc(dur, 0.002, dur * 0.6))
    ticks = np.zeros(n)
    teeth = np.floor(ph / (2.0 * np.pi / 4.0))
    rr = child(r, 'pawl')
    for i, h in enumerate(np.nonzero(np.diff(teeth) > 0)[0][:40]):
        c = kit.metal_click(child(rr, 'c', i), 5500.0, 10500.0, 0.006, 0.00003, 3, -16.0)
        m = min(c.size, n - h)
        ticks[h:h + m] += c[:m] * (0.9 ** (i * 0.25))
    slack = _weight(child(r, 'slack'), 620.0, 210.0, 0.03, -6.0)
    return layers(_sear(r, -12.0), (_loud(hum, 0.005), 0.0, -4.0), (_loud(ticks, 0.003), 0.0, -8.0),
                  (slack, dur - 0.02, -6.0))


@cue('Weapon.Sniper.LastRound', variations=3, att='Near', jitter=0.02, conc=2, level=-11.0)
def sniper_last_round(v, r):
    # The box's last round gone: its follower snaps up against the feed lips with nothing on it, a hollow steel box ring
    # with the spring's short buzz.
    snap = _weight(child(r, 'snap'), 560.0, 190.0, 0.03, -6.0)
    box = _box(r, [450.0, 480.0, 430.0][v], 0.1)
    spring = M.strike(M.parts(1700.0, 3000.0, child(r, 'spring'), 3, 0.12, 0.08, 0.2), M.hammer(0.0002), 0.2)
    t = 0.05 + 0.01 * r.random()
    return layers(_sear(r), (snap, t, 0.0), (_loud(box, 0.005), t, -4.0), (_loud(spring), t + 0.002, -18.0))


@cue('Weapon.LMG.LastRound', variations=3, att='Near', jitter=0.02, conc=2, level=-10.0)
def lmg_last_round(v, r):
    # The belt's tail runs through: the last links clatter out of the tray and drop, and the empty feed pawl snaps over
    # with nothing under it.
    links = LMG._links(child(r, 'links'), 6, 0.08)
    pawl = _weight(child(r, 'pawl'), 480.0, 170.0, 0.035, -4.0)
    tray = M.strike(M.shell(700.0 * jitter(r, 1.0, 0.05), child(r, 'tray'), 0.06, 6, 0.03), M.hammer(0.0002), 0.18)
    t = 0.04 + 0.01 * r.random()
    return layers(_sear(r), (links, 0.005, -2.0), (pawl, t + 0.06, 0.0), (_loud(tray, 0.005), t + 0.06, -12.0))


# --- The Thresher's hot barrel -----------------------------------------------------------------------------------------

@cue('Weapon.LMG.HeatHiss', variations=3, att='Near', jitter=0.04, conc=2, level=-15.0)
def lmg_heat_hiss(v, r):
    # A long burst let go with the barrel glowing: the air boiling off the hot steel (a hiss with a soft roar under it,
    # fading over a second), the barrel ticking as it starts to cool, and a faint sizzle of oil on it.
    dur = 1.5
    n = ns(dur)
    t = times(n)
    env = np.exp(-t / 0.45) * (0.75 + 0.25 * np.abs(N.smooth_random(n, child(r, 'flick'), 8.0)))
    hiss = F.filt(N.white(n, child(r, 'hiss')), F.hp(3000.0, 0.7), F.lp(9500.0, 0.7), F.peak(3600.0, -6.0, 0.8),
                  extend=False) * kit.turbulence(n, child(r, 'turb'), 120.0, 0.5)
    roar = F.filt(N.pink(n, child(r, 'roar')), F.bp(700.0, 0.7), extend=False)
    sizzle = GR.grit(dur, child(r, 'sizzle'), 1800.0 * np.exp(-t / 0.5) + 200.0, 4500.0, 10000.0, 3.0, 3)
    rr = child(r, 'ticks')
    ticks = np.zeros(n)
    for i in range(7):
        at = 0.08 + 1.2 * rr.random() ** 1.3
        c = kit.metal_click(child(rr, 'c', i), 3500.0, 9000.0, 0.02, 0.00005, 4, -12.0) * rr.uniform(0.3, 1.0)
        k = ns(at)
        m = min(c.size, n - k)
        ticks[k:k + m] += c[:m]
    onset = _loud(kit.burst(0.03, child(r, 'onset'), 0.012, lo=4000.0, hi=10000.0), 0.003)
    body = layers((_loud(hiss * env), 0.0, 0.0), (_loud(roar * env), 0.0, -12.0),
                  (_loud(sizzle * np.sqrt(env)), 0.0, -10.0))
    return layers((onset, 0.0, -6.0), (E.end_fade(body, 0.05), 0.0, 0.0), (_loud(ticks, 0.003), 0.0, -12.0))


# --- Handling by weight: the cloth of a reload, light and heavy draws, a heavy holster --------------------------------

@cue('Weapon.Reload.Cloth', variations=4, att='Near', jitter=0.05, conc=2, level=-19.0)
def reload_cloth(v, r):
    # Every reload's start: the glove closing on the gun (a small leather squeak of a grip), the sleeve swishing as the
    # off hand goes for the next magazine, the sling's swivel ticking at once.
    swivel = kit.metal_click(child(r, 'swivel'), 1800.0, 5200.0, 0.02, 0.00012, 4, -16.0)
    swish = kit.cloth(0.22 + 0.05 * r.random(), child(r, 'cloth'), 280.0, 2600.0, 140.0, 0.12, 0.55)
    glove = kit.scrape(0.06, child(r, 'glove'), 420.0, 380.0, 4.0, 0.8, (1.0, 1.7, 2.6), 0.2, 0.6, 1800.0, -30.0)
    glove = _loud(F.filt(glove, F.lp(2400.0, 0.7), extend=False))
    pat = kit.burst(0.03, child(r, 'pat'), 0.012, lo=180.0, hi=1800.0)
    t = 0.12 + 0.05 * r.random()
    return layers((_loud(swivel, 0.005), 0.0, -14.0), (swish, 0.0, -2.0), (glove, 0.01, -10.0), (pat, t, -8.0))


def _draw(r, cloth_dur, jingles, grip_f, settle_f, steel_db, heavy):
    """A long gun swung up off the back: the sling's hardware ticking at once, the cloth swishing, the hand closing on the
    grip, the gun settling into the shoulder; heavy guns swing longer and land with a recorded steel knock pitched low."""
    catch = kit.metal_click(child(r, 'catch'), 2000.0, 6000.0, 0.03, 0.0002, 4, -16.0)
    swish = kit.cloth(cloth_dur, child(r, 'cloth'), 260.0 if heavy else 320.0, 2600.0, 120.0, 0.2, 0.5)
    jing = kit.jingle(cloth_dur, child(r, 'jingle'), jingles, 2000.0, 7000.0, 0.04, 0.08, 0.05)
    grip = kit.thunk(grip_f * jitter(r, 1, 0.08), child(r, 'grip'), 0.06 if heavy else 0.045, 0.0012)
    slap = kit.burst(0.04, child(r, 'slap'), 0.015, lo=200.0, hi=2500.0)
    t = cloth_dur * 0.7 + 0.03 * r.random()
    out = [(_loud(catch, 0.005), 0.0, -14.0), (swish, 0.0, -6.0), (jing, 0.0, -16.0), (_loud(grip, 0.005), t, -4.0),
           (slap, t, -9.0)]
    if heavy:
        settle = _weight(child(r, 'settle'), settle_f, settle_f * 0.4, 0.06, -2.0)
        rec, at = _foley('Foley_Saiga_Pull.wav', int(r.integers(0, 6)), 0.01, 0.26, 0.78)
        out += [(settle, t + 0.03, -4.0), (_loud(_tame(rec, -9.0, -6.0, 200.0)), _place(rec, at, t + 0.03), steel_db)]
    else:
        settle = kit.clack(child(r, 'settle'), settle_f, 1800.0, 6500.0, 0.03, 0.04, -3.0, -10.0)
        out += [(settle, t + 0.02, -6.0)]
    return layers(*out)


@cue('Weapon.Equip.Light', variations=3, att='Near', jitter=0.04, conc=2, level=-15.0)
def equip_light(v, r):
    # A light gun (the Rustler) flicked up into the hands: a quick swish, the grip caught, a small polymer clack.
    return _draw(r, 0.18, 2 + int(r.integers(0, 2)), 460.0, 760.0, 0.0, False)


@cue('Weapon.Equip.Heavy', variations=3, att='Near', jitter=0.04, conc=2, level=-13.0)
def equip_heavy(v, r):
    # A heavy gun (the Longhorn, the Thresher, the Powderkeg) hauled up off the back: a longer swing, the sling's heavier
    # hardware, the grip taken with a thud and the steel settling into the shoulder.
    return _draw(r, 0.32, 4 + int(r.integers(0, 3)), 300.0, 260.0, -10.0, True)


@cue('Weapon.Holster.Heavy', variations=3, att='Near', jitter=0.04, conc=2, level=-15.0)
def holster_heavy(v, r):
    # A heavy gun swung down onto the back: the swivel ticking at once, a long swish, the hardware jingling, the gun's
    # weight landing against the back (a deep padded knock), its steel knocking under it.
    catch = kit.metal_click(child(r, 'catch'), 1800.0, 5200.0, 0.025, 0.00015, 4, -16.0)
    swish = kit.cloth(0.3, child(r, 'cloth'), 240.0, 2200.0, 140.0, 0.15, 0.55)
    jing = kit.jingle(0.26, child(r, 'jingle'), 3 + int(r.integers(0, 3)), 1800.0, 6000.0, 0.04, 0.08, 0.02)
    settle = kit.thunk(170.0 * jitter(r, 1, 0.08), child(r, 'settle'), 0.09, 0.0025)
    pat = kit.burst(0.045, child(r, 'pat'), 0.018, lo=150.0, hi=1800.0)
    steel = _weight(child(r, 'steel'), 300.0, 110.0, 0.05, -2.0)
    t = 0.19 + 0.03 * r.random()
    return layers((_loud(catch, 0.005), 0.0, -16.0), (swish, 0.0, -4.0), (jing, 0.05, -17.0),
                  (_loud(settle, 0.005), t, -3.0), (pat, t, -10.0), (steel, t + 0.006, -9.0))
