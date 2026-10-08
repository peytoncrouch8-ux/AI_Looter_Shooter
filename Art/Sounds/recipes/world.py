"""The world: the chapel's bell tolling over the valley, the bell on Skyreach's jetty, and window shutters slamming
on Main Street. Bells are cast-bell partials (hum, prime, tierce, quint, nominal and the rest, each a slowly beating
pair), struck by an iron clapper, in the open air.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, mono
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R
from recipes import cue
from recipes import kit


def _clapper(r, hardness=0.00025, clang_lo=2500.0, clang_hi=9000.0):
    """The iron clapper hitting the bronze lip: the strike's force, and a short metallic clang of high partials."""
    clang = normalize(M.strike(M.parts(clang_lo, clang_hi, child(r, 'clang'), 9, 0.08, 0.03), M.hammer(0.00006), 0.15))
    knock = kit.burst(0.01, child(r, 'knock'), 0.004, lo=800.0, hi=7000.0)
    return M.hammer(hardness), layers((clang, 0.0, 0.0), (knock, 0.0, -6.0))


def _bell(r, prime, t60, dur, beat=0.8, bright=0.9, hardness=0.00025, clang_db=-14.0):
    exc, clang = _clapper(child(r, 'clapper'), hardness)
    ring = normalize(M.strike(M.bell(prime, child(r, 'bell'), t60, beat, bright), exc, dur))
    return layers((ring, 0.0, 0.0), (normalize(clang), 0.0, clang_db))


@cue('World.ChapelBell.Toll', variations=2, att='Gun', cls='Ambience', jitter=0.0, conc=2, level=-3.0)
def chapel_bell(v, r):
    # The chapel's bell: a big bronze toll whose hum hangs over the valley, the valley's walls answering it.
    prime = 311.0 * [1.0, 0.985][v]
    bell = _bell(r, prime, 9.0, 8.0, 0.7, 1.0, 0.00025, -11.0)
    ir = R.outdoor(child(r, 'ir'), dur=3.5, slaps=((0.22, -4.0), (0.55, -8.0), (0.9, -12.0)), slap_lp=2500.0,
                   tail_t60=2.8, tail_db=-8.0, tail_start=0.05, tail_peak=0.35, tail_lp=2200.0, flutter=0.2)
    wet = F.convolve(F.filt(bell, F.hp(150.0), extend=False), ir)
    return layers(bell, kit.set_level(wet, bell, -14.0, 0.1))


@cue('World.JettyBell', variations=3, att='Gun', cls='Ambience', jitter=0.01, conc=2, level=-7.0, align=False)
def jetty_bell(v, r):
    # The small brass bell on the jetty's post: rung once, twice, or a quick three, bright and sharp in the open air.
    prime = 880.0 * jitter(r, 1, 0.005)
    out = np.zeros(1)
    strikes = [[0.0], [0.0, 0.42], [0.0, 0.28, 0.56]][v]
    for i, t in enumerate(strikes):
        b = _bell(child(r, 's', i), prime, 3.2, 3.6, 1.6, 1.0, 0.00015, -12.0)
        out = layers(out, (b, t, -1.5 * i))
    ir = R.open_air(child(r, 'ir'), t60=2.2, lp=4000.0, predelay=0.04, density=0.6)
    wet = F.convolve(F.filt(out, F.hp(300.0), extend=False), ir)
    return layers(out, kit.set_level(wet, out, -16.0, 0.1))


@cue('World.ShutterSlam', variations=4, att='Creature', cls='Ambience', jitter=0.05, conc=3, level=-7.0, swell=True)
def shutter_slam(v, r):
    # A wooden shutter banged shut against its frame: the hinge's squeal first (sometimes), the slam's crack and the
    # panel's knock, a bounce or two, the latch rattling, and the street's false fronts throwing it back.
    parts = []
    t = 0.0
    if v in (1, 3):
        squeal = kit.creak(0.22, child(r, 'squeal'), 160.0, 260.0, (1300.0, 2700.0, 4100.0), 12.0, 0.2)
        parts.append((F.filt(squeal, F.peak(3300.0, -4.0, 1.0), extend=False), 0.0, -20.0))
        t = 0.2
    f0 = 150.0 + 50.0 * r.random()
    panel = kit.thunk(f0, child(r, 'panel'), 0.16, 0.0006, 7, 0.1)
    crack = kit.burst(0.012, child(r, 'crack'), 0.005, lo=900.0, hi=9000.0)
    frame = kit.noise_thump(0.15, child(r, 'frame'), 900.0, 90.0, 0.07, 1.0, 0.05)
    slat = kit.thunk(f0 * 2.7, child(r, 'slat'), 0.05, 0.0003, 5, 0.12)
    parts += [(panel, t, 0.0), (crack, t, -4.0), (frame, t, -4.0), (slat, t + 0.003, -6.0)]
    bt = t
    for i, (gap, db) in enumerate([(0.05, -10.0), (0.035, -17.0)]):
        bt += gap * jitter(r, 1, 0.2)
        parts.append((kit.thunk(f0 * 1.1, child(r, 'b', i), 0.08, 0.0008, 6, 0.1), bt, db))
    latch = kit.jingle(0.25, child(r, 'latch'), 4, 1500.0, 5000.0, 0.04, 0.12, 0.0)
    parts.append((latch, t + 0.01, -16.0))
    x = layers(*parts)
    ir = R.outdoor(child(r, 'ir'), dur=1.0, slaps=((0.045, -3.0), (0.11, -6.0), (0.24, -11.0)), slap_lp=3500.0,
                   tail_t60=0.7, tail_db=-12.0, tail_start=0.03, tail_peak=0.1)
    wet = F.convolve(F.filt(x, F.hpn(200.0, 2), extend=False), ir)
    return layers(x, kit.set_level(wet, x, -14.0, 0.02))
