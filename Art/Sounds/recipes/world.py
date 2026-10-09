"""The world: the chapel's bell tolling over the valley, the bell on Skyreach's jetty, and window shutters slamming
on Main Street. Bells are cast-bell partials (hum, prime, tierce, quint, nominal and the rest, each a slowly beating
pair), struck by an iron clapper, in the open air.

And the places that make sound where they are (Audio/AmbientEmitterComponent): the creeks, ponds and waterfalls, the
windmill turning and its head swinging, the chapel bell's hum at rest, the warm train's steam, the Rim's hollow wind,
the Sink's drips and its webs creaking, and Main Street's held breath (the wind in its gaps, its boards and signs, a
loose shutter, the living muffled behind their walls).
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, db2a, mono
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R
from lib import granular as G, voice as V
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


# --- Places that make sound (Audio/AmbientEmitterComponent plays these where the world has them) -----------------------
#
# Loops are mono and in the world: an emitter on a creek or a street follows the point nearest the listener, so one
# voice sounds like the whole line. Each is built in a circle (periodic noise, events folded round the seam, filters
# applied circularly, anything with memory run over two laps).

def _fold(L, events):
    """Events (signal, time) laid on a loop of L samples, their tails folded round to the start."""
    out = np.zeros(L + ns(4.0))
    for sig, t in events:
        start = ns(t) % L
        end = start + sig.size
        if end > out.size:
            out = np.pad(out, (0, end - out.size))
        out[start:end] += sig
    y = out[:L].copy()
    rest = out[L:]
    while rest.size:
        k = min(L, rest.size)
        y[:k] += rest[:k]
        rest = rest[k:]
    return y


def _pnoise(n, r, lo, hi, slope=-3.0):
    return N.shaped(n, r, slope, lo, hi, periodic=True)


def _slow(n, r, rate, floor=0.3):
    return floor + (1.0 - floor) * (0.5 + 0.5 * N.smooth_random(n, r, rate, periodic=True))


def _circ(x, *sections):
    return F.filt(x, *sections, circular=True)


def _cave(x, r, wet_db=-6.0, t60=1.1):
    """A wet, enclosed space (the Sink's walls): early slaps close together, then a dark, short wash; circular."""
    ir = R.room(child(r, 'cave'), t60=t60, size=3.0, damp=0.3)
    L = x.size
    X = np.fft.rfft(x, L)
    H = np.fft.rfft(ir[:L], L)
    wet = np.fft.irfft(X * H, L)
    return x + wet / (np.sqrt(np.mean(wet * wet)) + 1e-12) * np.sqrt(np.mean(x * x)) * db2a(wet_db)


def _cave_once(x, r, wet_db=-9.0, t60=0.9):
    """A one-shot in the Sink: its walls' early slaps and a short, dark wash after it (linear: the tail runs on)."""
    ir = R.room(child(r, 'cave'), t60=t60, size=3.0, damp=0.3)
    return layers(x, kit.set_level(F.convolve(x, ir), x, wet_db, 0.05))


def _street(x, r, wet_db=-14.0):
    """A one-shot on Main Street: the false fronts throwing it back."""
    ir = R.outdoor(child(r, 'street'), dur=0.9, slaps=((0.04, -4.0), (0.1, -8.0)), slap_lp=3500.0, tail_t60=0.6,
                   tail_db=-14.0, tail_start=0.03, tail_peak=0.1)
    return layers(x, kit.set_level(F.convolve(F.filt(x, F.hp(150.0), extend=False), ir), x, wet_db, 0.03))


@cue('World.Creek', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=3, loop=True, level=-17.0,
     align=False)
def creek(v, r):
    # A shallow creek running over stones: hundreds of small bubbles ringing as they close (the physics of a bubble's
    # note, high for small ones), a few deep gurgles, and the soft rush of the water over the bed.
    dur = 9.0
    L = ns(dur)
    events = []
    rb = child(r, 'bubbles')
    for k, t in enumerate(N.times_poisson(dur, rb, 70.0)):
        f0 = float(np.exp(rb.uniform(np.log(500.0), np.log(2600.0))))
        events.append((kit.bubble(f0, rb, xi=rb.uniform(0.05, 0.25), amp=rb.uniform(0.2, 1.0)), t))
    rg = child(r, 'gurgle')
    for k, t in enumerate(N.times_poisson(dur, rg, 4.0)):
        f0 = rg.uniform(160.0, 420.0)
        events.append((kit.bubble(f0, rg, xi=0.15, amp=rg.uniform(0.6, 1.2)), t))
    bubbles = _fold(L, events)
    rush = _pnoise(L, child(r, 'rush'), 250.0, 4000.0, -2.0) * _slow(L, child(r, 'rg'), 0.6, 0.6)
    x = normalize(bubbles) + 0.35 * normalize(rush)
    return _circ(x, F.hp(120.0), F.peak(900.0, 2.0, 0.8), F.highshelf(6000.0, -6.0))


@cue('World.Pond', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=3, loop=True, level=-25.0,
     align=False)
def pond(v, r):
    # Still water at its edge: small lapping slaps against the bank and the reeds, now and then a plip.
    dur = 8.0
    L = ns(dur)
    events = []
    rl = child(r, 'lap')
    t = 0.0
    k = 0
    while t < dur:
        n = ns(0.25)
        slap = N.pink(n, child(rl, k)) * E.perc(0.25, 0.006, 0.12)
        slap = F.filt(slap, F.bp(rl.uniform(280.0, 650.0), 1.6), F.lp(2500.0), extend=False)
        events.append((normalize(slap) * rl.uniform(0.4, 1.0), t))
        t += rl.uniform(0.5, 1.4)
        k += 1
    rp = child(r, 'plip')
    for k, t in enumerate(N.times_poisson(dur, rp, 0.6)):
        events.append((kit.bubble(rp.uniform(900.0, 1800.0), rp, xi=0.2, amp=0.5), t))
    x = _fold(L, events)
    bed = _pnoise(L, child(r, 'bed'), 200.0, 1500.0, -4.0) * 0.06
    return _circ(normalize(x) + bed, F.hp(150.0))


@cue('World.Waterfall', variations=1, att='Gun', cls='Ambience', jitter=0.0, conc=2, loop=True, level=-9.0,
     align=False)
def waterfall(v, r):
    # A waterfall's roar: a dense, wide rush of falling water churning in the plunge pool (a broad hump in the low
    # mids), the spray's crackle of countless small splashes on top, the pool's low throb under it.
    dur = 8.0
    L = ns(dur)
    roar = _pnoise(L, child(r, 'roar'), 60.0, 9000.0, -2.5)
    roar = _circ(roar, F.peak(450.0, 5.0, 0.7), F.peak(1800.0, 2.0, 1.0))
    churn = 0.75 + 0.25 * np.abs(N.smooth_random(L, child(r, 'churn'), 18.0, periodic=True))
    spray = G.grit(dur, child(r, 'spray'), 9000.0, 1500.0, 7000.0, 1.5, 5, periodic=True)
    throb = _pnoise(L, child(r, 'throb'), 30.0, 160.0, -6.0) * _slow(L, child(r, 'tg'), 1.4, 0.5)
    x = normalize(roar * churn) + 0.35 * normalize(spray) + 0.4 * normalize(throb)
    return _circ(x, F.hpn(35.0, 2), F.highshelf(7000.0, -4.0))


@cue('World.Windmill.Fan', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=2, loop=True, level=-20.0,
     align=False)
def windmill_fan(v, r):
    # The water-pump windmill turning at its usual pace (two turns in the loop; the game speeds it and slows it with
    # the gusts): eighteen blades swishing through the air, the shaft's low hum, and the pump rod's stroke once a turn,
    # a creak on the way up and a clunk at the bottom.
    turn = 60.0 / 18.0
    dur = 2 * turn
    L = ns(dur)
    t = times(L)
    blades = 18
    swish_rate = blades / turn
    phase = (t * swish_rate) % 1.0
    swish_env = np.exp(-((phase - 0.5) / 0.18) ** 2)
    air = _pnoise(L, child(r, 'air'), 250.0, 2500.0, -3.0)
    air = _circ(air, F.peak(700.0, 3.0, 1.0))
    swish = air * (0.25 + 0.75 * swish_env)
    hum = _pnoise(L, child(r, 'hum'), 40.0, 220.0, -5.0) * 0.5
    events = []
    for k in range(2):
        up = kit.creak(0.7, child(r, 'creak', k), 30.0, 55.0, (380.0, 900.0, 1700.0), 9.0, 0.3)
        clunk = kit.thunk(170.0, child(r, 'clunk', k), 0.1, 0.0015, 6, 0.08)
        events.append((up * 0.35, k * turn + 0.3))
        events.append((clunk * 0.5, k * turn + turn * 0.8))
    rod = _fold(L, events)
    x = normalize(swish) + 0.4 * normalize(hum) + 0.6 * normalize(rod)
    return _circ(x, F.hp(45.0), F.highshelf(6000.0, -4.0))


@cue('World.Windmill.Creak', variations=4, att='Creature', cls='Ambience', jitter=0.06, conc=2, level=-17.0,
     swell=True)
def windmill_creak(v, r):
    # The windmill's head swinging round on its post to the wind: iron grinding on iron, slow and complaining, with
    # the tower's timbers groaning under it.
    dur = r.uniform(0.7, 1.4)
    iron = kit.creak(dur, child(r, 'iron'), r.uniform(18.0, 30.0), r.uniform(30.0, 45.0), (520.0, 1300.0, 2400.0), 12.0,
                     0.35)
    wood = kit.creak(dur * 0.8, child(r, 'wood'), 12.0, 20.0, (210.0, 480.0, 950.0), 7.0, 0.3)
    x = layers((iron, 0.0, 0.0), (wood, 0.08, -6.0))
    return _street(x, r, -16.0)


@cue('World.ChapelBell.Hum', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=1, loop=True, level=-26.0,
     align=False)
def chapel_bell_hum(v, r):
    # The dark chapel's bell at rest, stirred by the wind through the belfry's louvres: its hum and prime barely
    # sounding, beating slowly, and the louvres' faint whistle. A held breath of bronze.
    dur = 12.0
    L = ns(dur)
    t = times(L)
    prime = 311.0
    tone = np.zeros(L)
    # Whole numbers of cycles in the loop keep it seamless; the pairs a fraction of a hertz apart beat slowly.
    for ratio, amp in ((0.5, 1.0), (1.0, 0.5), (1.19, 0.25), (2.0, 0.18)):
        for split in (-0.25, 0.25):
            f = round((prime * ratio + split) * dur) / dur
            tone += amp * np.sin(2 * np.pi * f * t + r.random() * 6.28)
    swell = _slow(L, child(r, 'swell'), 0.15, 0.25)
    whistle = _circ(_pnoise(L, child(r, 'louvre'), 500.0, 2500.0, 0.0), F.bp(1150.0, 9.0))
    whistle *= _slow(L, child(r, 'wg'), 0.25, 0.1)
    x = normalize(tone * swell) + 0.25 * normalize(whistle)
    return _circ(x, F.hp(100.0))


@cue('World.Train.Hiss', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=1, loop=True, level=-15.0,
     align=False)
def train_hiss(v, r):
    # A locomotive standing in steam: the boiler's low simmer, steam hissing from a valve, the air pump panting in
    # pairs of thumps, a tick of hot iron now and then.
    dur = 10.0
    L = ns(dur)
    hiss = _pnoise(L, child(r, 'hiss'), 1800.0, 11000.0, -1.0)
    hiss *= 0.75 + 0.25 * np.abs(N.smooth_random(L, child(r, 'flutter'), 25.0, periodic=True))
    simmer = _pnoise(L, child(r, 'simmer'), 40.0, 300.0, -5.0) * _slow(L, child(r, 'sg'), 0.3, 0.7)
    events = []
    period = dur / 6.0
    for k in range(6):
        for j, at in enumerate((0.0, 0.32)):
            thump = kit.noise_thump(0.35, child(r, 'pump', k, j), 500.0, 80.0, 0.18, 1.0, 0.1)
            chuff = kit.burst(0.25, child(r, 'chuff', k, j), 0.15, lo=700.0, hi=5000.0)
            events.append((layers((thump, 0.0, -2.0), (chuff, 0.0, -12.0)), k * period + at))
    rt = child(r, 'tick')
    for k, at in enumerate(N.times_poisson(dur, rt, 0.4)):
        events.append((kit.metal_click(child(rt, k), 1500.0, 6000.0, 0.05) * 0.2, at))
    x = 0.35 * normalize(hiss) + 0.5 * normalize(simmer) + normalize(_fold(L, events))
    return _circ(x, F.hp(35.0), F.peak(3500.0, -3.0, 1.0), F.highshelf(8000.0, -4.0))


@cue('World.Rim.Wind', variations=1, att='Gun', cls='Ambience', jitter=0.0, conc=2, loop=True, level=-15.0,
     align=False)
def rim_wind(v, r):
    # Wind over the Rim's edge: the canyon below singing in its hollow notes, swelling and fading by turns, with the
    # rush of air pouring over the lip.
    dur = 16.0
    L = ns(dur)
    src = _pnoise(L, child(r, 'src'), 40.0, 1200.0, -3.0)
    hollow = np.zeros(L)
    for i, f in enumerate((84.0, 126.0, 189.0, 252.0)):
        band = _circ(src, F.bp(f, 22.0), F.bp(f, 22.0))
        hollow += band * _slow(L, child(r, 'h', i), 0.1 + 0.03 * i, 0.15) / (1.0 + 0.3 * i)
    gust = 0.35 + 0.65 * (0.5 + 0.5 * N.smooth_random(L, child(r, 'g'), 0.12, periodic=True))
    rush = _pnoise(L, child(r, 'rush'), 150.0, 2500.0, -3.5) * gust ** 1.5
    x = normalize(hollow) + 0.55 * normalize(rush)
    return _circ(x, F.hpn(40.0, 2), F.highshelf(5000.0, -6.0))


@cue('World.Sink.Drip', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=1, loop=True, level=-19.0,
     align=False)
def sink_drip(v, r):
    # The Sink's damp floor: water dripping from the overhang into puddles, each drip ringing off the pit's walls,
    # and the cold, still air of a hole in the ground under it.
    dur = 14.0
    L = ns(dur)
    events = []
    rd = child(r, 'drip')
    t = rd.uniform(0.3, 0.6)
    k = 0
    while t < dur - 0.8:
        drip = kit.bubble(rd.uniform(900.0, 2400.0), child(rd, k), xi=rd.uniform(0.3, 0.6), amp=rd.uniform(0.4, 1.0))
        events.append((drip, t))
        t += rd.uniform(0.5, 1.9)
        k += 1
    drips = _cave(_fold(L, events), r, -3.0, 1.3)
    air = _pnoise(L, child(r, 'air'), 50.0, 350.0, -5.0) * _slow(L, child(r, 'ag'), 0.08, 0.6)
    x = normalize(drips) + 0.18 * normalize(air)
    return _circ(x, F.hp(60.0), F.highshelf(7000.0, -4.0))


@cue('World.Sink.Creak', variations=4, att='Creature', cls='Ambience', jitter=0.08, conc=2, level=-21.0,
     swell=True)
def sink_creak(v, r):
    # Silk under strain in the Webwood and the Sink: thick strands stretching as something heavy shifts on them, a
    # thin, high creaking and a dry rustle of web.
    dur = r.uniform(0.4, 0.9)
    strand = kit.creak(dur, child(r, 'silk'), r.uniform(70.0, 110.0), r.uniform(120.0, 200.0), (1900.0, 3100.0, 4600.0),
                       16.0, 0.4)
    strand = F.filt(strand, F.peak(3100.0, -4.0, 1.0), extend=False)
    rustle = kit.cloth(dur, child(r, 'rustle'), 900.0, 5000.0, 90.0, 0.2, 0.6)
    return _cave_once(layers((strand, 0.0, 0.0), (rustle, 0.05, -12.0)), r, -9.0, 0.9)


@cue('World.Town.Hush', variations=1, att='Creature', cls='Ambience', jitter=0.0, conc=2, loop=True, level=-26.0,
     align=False)
def town_hush(v, r):
    # Main Street holding its breath: the wind finding the gaps in the boarded windows and false fronts (thin, wavering
    # whistles), dust skittering along the boardwalk, nothing else.
    dur = 12.0
    L = ns(dur)
    whistle = np.zeros(L)
    src = _pnoise(L, child(r, 'src'), 400.0, 3000.0, 0.0)
    for i, f in enumerate((780.0, 1130.0, 1460.0)):
        whistle += _circ(src, F.bp(f, 14.0), F.bp(f, 14.0)) * _slow(L, child(r, 'w', i), 0.15, 0.0)
    low = _pnoise(L, child(r, 'low'), 60.0, 500.0, -4.0) * _slow(L, child(r, 'lg'), 0.1, 0.5)
    dust = G.grit(dur, child(r, 'dust'), 150.0, 2000.0, 6000.0, 2.0, 3, periodic=True)
    dust *= _slow(L, child(r, 'dg'), 0.2, 0.0) ** 2
    x = normalize(whistle) * 0.5 + normalize(low) + 0.25 * normalize(dust)
    return _circ(x, F.hp(50.0), F.highshelf(6000.0, -5.0))


@cue('World.Town.Creak', variations=4, att='Creature', cls='Ambience', jitter=0.06, conc=2, level=-17.0,
     swell=True)
def town_creak(v, r):
    # A shuttered town in the wind: a board giving under its own weight, a sign swinging on rusty hooks, a porch post.
    if v == 1:
        x = kit.creak(r.uniform(0.6, 1.0), child(r, 'hook'), 20.0, 34.0, (600.0, 1400.0, 2500.0), 12.0, 0.3)
        x = layers((x, 0.0, 0.0), (kit.creak(0.5, child(r, 'back'), 30.0, 22.0, (650.0, 1500.0, 2600.0), 12.0, 0.3),
                                   0.9, -4.0))
    else:
        x = kit.creak(r.uniform(0.35, 0.8), child(r, 'board'), r.uniform(14.0, 28.0), r.uniform(25.0, 45.0),
                      (r.uniform(220.0, 330.0), r.uniform(560.0, 800.0), r.uniform(1100.0, 1600.0)), 8.0, 0.3)
    return _street(x, r)


@cue('World.Shutter.Tap', variations=3, att='Creature', cls='Ambience', jitter=0.06, conc=2, level=-16.0)
def shutter_tap(v, r):
    # A loose shutter knocking against its frame in the wind: two to four taps, the wind deciding the gaps.
    out = np.zeros(1)
    t = 0.0
    f0 = r.uniform(160.0, 220.0)
    for k in range([2, 3, 4][v]):
        knock = layers((kit.thunk(f0 * r.uniform(0.95, 1.05), child(r, 'p', k), 0.09, 0.0006, 6, 0.1), 0.0, 0.0),
                       (kit.thunk(f0 * 2.6, child(r, 's', k), 0.04, 0.0003, 5, 0.1), 0.002, -8.0))
        out = layers(out, (knock, t, -2.0 * (k % 2) - r.uniform(0.0, 3.0)))
        t += r.uniform(0.18, 0.55)
    return _street(out, r)


@cue('World.Town.Murmur', variations=4, att='Creature', cls='Ambience', jitter=0.03, conc=1, level=-19.0,
     swell=True)
def town_murmur(v, r):
    # The living, behind their shutters, heard through the walls: a cough (1), two hushed voices (2), a hush and a
    # chair scraping back (3), someone weeping quietly (4). Muffled to almost nothing but their shape.
    if v == 0:
        out = np.zeros(1)
        for k in range(int(r.integers(1, 3))):
            d = r.uniform(0.18, 0.28)
            f0 = O.glide([(0.0, 180.0), (d, 120.0)], n=ns(d))
            c = V.voice(f0, [(0.0, 'a'), (d, 'uh')], child(r, 'c', k), breath=0.7, jitter=0.05, rough=0.5)
            out = layers(out, (c * E.perc(d, 0.005, d * 0.6), k * r.uniform(0.3, 0.45)))
    elif v == 1:
        out = np.zeros(1)
        t = 0.0
        for k in range(int(r.integers(6, 10))):
            d = r.uniform(0.12, 0.3)
            f0 = (r.uniform(110.0, 140.0) if k % 4 < 2 else r.uniform(190.0, 230.0)) * O.glide(
                [(0.0, 1.05), (d, 0.92)], n=ns(d))
            vowels = [str(x) for x in r.choice(['a', 'e', 'o', 'uh', 'i', 'er'], 2)]
            s = V.voice(f0, [(0.0, vowels[0]), (d, vowels[1])], child(r, 'm', k), breath=0.35, jitter=0.02)
            out = layers(out, (s * E.ar(d, 0.02, 0.05, 2.0), t))
            t += d + r.uniform(0.02, 0.2)
    elif v == 2:
        shh = V.whisper(0.6, [(0.0, 'i'), (0.6, 'i')], child(r, 'shh'))
        shh = F.filt(shh, F.hp(2000.0), extend=False) * E.ar(0.6, 0.08, 0.3, 2.0)
        chair = kit.creak(0.4, child(r, 'chair'), 60.0, 30.0, (300.0, 700.0, 1400.0), 6.0, 0.4)
        out = layers((shh, 0.0, 0.0), (chair, 0.9, -2.0))
    else:
        out = np.zeros(1)
        t = 0.0
        for k in range(int(r.integers(3, 5))):
            d = r.uniform(0.3, 0.6)
            f0 = O.glide([(0.0, 300.0), (d * 0.3, 340.0), (d, 250.0)], n=ns(d)) * r.uniform(0.9, 1.1)
            s = V.voice(f0, [(0.0, 'uh'), (d, 'u')], child(r, 'w', k), breath=0.5, jitter=0.03, shimmer=0.2,
                        vib_rate=7.0, vib_depth=0.4)
            out = layers(out, (s * E.ar(d, 0.05, 0.2, 2.0), t))
            t += d + r.uniform(0.25, 0.6)
    # Through a wall: only the low half gets out, with the room's box around it.
    out = F.filt(normalize(out), F.lp(700.0, 0.6), F.lp(900.0, 0.6), F.hp(90.0), extend=False)
    ir = R.room(child(r, 'room'), t60=0.35, size=0.6)
    return layers(out, kit.set_level(F.convolve(out, ir), out, -10.0, 0.05))
