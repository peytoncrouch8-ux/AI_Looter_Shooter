"""The Bell-Ringer's bell (ABellRingerCreature), all 3D: an oversized bronze hand bell, 18 cm across the mouth, on a
turned wooden handle, rung by an iron clapper.

A cast hand bell is a thin-walled bronze shell: struck, it rings a set of partials that are not a harmonic series, each
split into a slowly beating pair by the casting's imperfect roundness, the upper ones dying first. Its hum is weak (a hand
bell keeps little of a church bell's hum); the strike note leads, with strong partials a little over two and near three
times it. The clapper is iron, so its contact is short and bright, with a click of its own. Rung in the hand the bell
keeps swinging, turning its mouth toward and away from the listener, so its ring waxes and wanes.

- Toll: one stroke and the clapper's softer bounce off the far side, the ring waxing and waning as the bell swings, carried
  across the fields (slap-backs off the buildings and the ridges, the valley's long wash): heard across a fight.
- TollCracked: a Gravebound ringer's bell, cracked: its pairs split wide and beating hard, sour strays a tritone and a
  ninth off the strike note, the crack's edges chattering in a buzz on the ring, choked short.
- Lift: the tell, before a toll: the bell coming up out of a cloth rustle, the clapper knocking about inside it as it's
  shaken, faster and harder toward the stroke, and a cold breath drawn in.
- Struck: a bullet ringing off the bell: a bright, hard clang (a small hard thing on its waist wakes its upper partials),
  the clapper clattering as it's jolted, the handle jarring in the fist.
- Raise: at a fallen one's grave as the toll calls it up: many whispering voices drawing up out of the ground over the
  bell's hum held long and cold, and grave dust.
- Swipe: the bell swung as a club when it's run down: its rush through the air, its own clapper clanking, choked by the
  swing, and a dull knock.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter, add_at
from lib import noise as N, env as E, filters as F, dist as D, modal as M, reverb as R, voice as V
from recipes import cue
from recipes import kit
from recipes.creatures import _haunt, _outside, _cold

# A hand bell's partials against its strike note: (ratio, strength, share of the ring time). Measured physics of thin
# bronze bells; the weak hum first.
HANDBELL = [(0.57, 0.13, 1.0), (1.0, 1.0, 1.0), (2.11, 0.55, 0.6), (2.66, 0.32, 0.45), (3.01, 0.40, 0.4),
            (4.18, 0.26, 0.28), (5.31, 0.18, 0.2), (6.62, 0.12, 0.15), (8.07, 0.08, 0.1), (9.81, 0.05, 0.07)]

# The bell's strike note for each take: a little apart, as several ringers' bells would be.
PRIMES = (720.0, 690.0, 760.0, 705.0)


def handbell(prime, r, t60=3.2, beat=0.9, bright=1.0, crack=0.0):
    """The bell's modes: each partial a beating pair (wider apart and quicker to die the more it's cracked)."""
    f, d, a = [], [], []
    for i, (ratio, amp, share) in enumerate(HANDBELL):
        base = prime * ratio * (1.0 + 0.004 * (2 * r.random() - 1))
        amp = amp * (bright ** max(i - 2, 0))
        split = beat * (0.4 + r.random()) * (1.0 + 0.4 * i) + crack * (5.0 + 9.0 * r.random()) * (1.0 + 0.2 * i)
        for s in (-0.5, 0.5):
            f.append(base + s * split)
            d.append(t60 * share * (1.0 - 0.7 * crack))
            a.append(amp * (0.5 + 0.12 * (2 * r.random() - 1)))
    return M.modes(f, d, a)


def ring(m, dur, r, contact=0.00018, tick_db=-22.0):
    """The bell rung once by the iron clapper: the modes struck by a short force pulse, with the contact's own click."""
    rung = normalize(M.strike(m, M.hammer(contact), dur))
    tick = kit.burst(0.003, child(r, 'tick'), 0.0015, lo=2500.0, hi=9000.0)
    return layers(rung, (tick, 0.0, tick_db))


def _swinging(x, r, rate=1.5, depth=0.22):
    """The bell keeps swinging in the hand after the stroke, turning its mouth toward and away: its ring waxes and wanes."""
    t = times(x.size)
    return x * (1.0 - depth * (0.5 + 0.5 * np.cos(2 * np.pi * (rate + 0.2 * r.random()) * t)))


def _across(x, r, dur=2.6, rel_db=-12.0):
    """Carried across the fields: slap-backs off the buildings and the ridges, then the valley's long, dark wash."""
    ir = R.outdoor(child(r, 'ir'), dur=dur, slaps=((0.17, -7.0), (0.42, -12.0), (0.81, -17.0)), tail_t60=2.0,
                   tail_db=-12.0, tail_lp=2600.0)
    wet = F.convolve(F.filt(x, F.hp(200.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


@cue('Creature.BellRinger.Toll', variations=4, att='Gun', jitter=0.02, conc=2, level=-6.0)
def toll(v, r):
    # One stroke, then the clapper bouncing off the far side as the bell swings back under it, softer and a little later.
    prime = PRIMES[v] * jitter(r, 1, 0.01)
    m = handbell(prime, child(r, 'bell'), t60=3.4, beat=0.9, bright=0.92)
    dur = 3.8
    first = ring(m, dur, child(r, 'first'))
    back = ring(m, dur - 0.2, child(r, 'back'), 0.00026, -26.0)
    x = layers((first, 0.0, 0.0), (back, 0.12 + 0.05 * r.random(), -9.0 - 3.0 * r.random()))
    x = _swinging(x, child(r, 'swing'))
    # The air the bell swings through, faint under it.
    air = kit.whoosh(0.35, child(r, 'air'), 300.0, 1200.0, 0.8, 0.3, -3.0)
    x = layers((x, 0.0, 0.0), (air, 0.0, -28.0))
    # It comes every few seconds in a fight: its brightest band eased so it rings out without tiring the ear.
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(90.0), extend=False)
    return _across(x, r)


@cue('Creature.BellRinger.TollCracked', variations=3, att='Gun', jitter=0.02, conc=2, level=-6.0)
def toll_cracked(v, r):
    # The same stroke on a cracked bell: sour, buzzing, choked short.
    prime = (650.0, 620.0, 680.0)[v] * jitter(r, 1, 0.01)
    m = handbell(prime, child(r, 'bell'), t60=2.2, beat=1.0, bright=0.8, crack=1.0)
    strays = M.modes([prime * 1.41, prime * 1.43, prime * 2.27, prime * 2.31], [0.7, 0.7, 0.45, 0.45], [0.5, 0.45, 0.3, 0.28])
    m = M.merge(m, strays)
    dur = 2.4
    rung = ring(m, dur, child(r, 'ring'), 0.0003, -18.0)
    n = rung.size
    # The crack's edges chatter as it rings: a buzz riding the ring's own level, and the ring clipped lopsided.
    level = np.clip(F.filt(np.abs(rung), F.lp(30.0), extend=False), 0.0, None)
    level /= max(float(np.max(level)), 1e-9)
    rate = 70.0 + 30.0 * r.random()
    gate = 0.5 + 0.5 * np.tanh(6.0 * np.sin(2 * np.pi * rate * times(n) + 2 * np.pi * r.random()))
    buzz = N.band(n, child(r, 'buzz'), 900.0, 4000.0) * gate * level ** 1.5
    dirty = D.drive(rung, 14.0, 'tanh', bias=0.25)
    back = ring(m, dur - 0.3, child(r, 'back'), 0.0004, -20.0)
    x = layers((rung, 0.0, 0.0), (normalize(dirty), 0.0, -8.0), (normalize(buzz), 0.0, -14.0),
               (back, 0.13 + 0.04 * r.random(), -11.0))
    x = _swinging(x, child(r, 'swing'), 1.4, 0.18)
    x = F.filt(x, F.lp(4200.0, 0.7), F.peak(3200.0, -2.0, 0.8), F.hp(90.0), extend=False)
    return _across(x, r, 2.2, -13.0)


@cue('Creature.BellRinger.Lift', variations=3, att='Creature', jitter=0.04, conc=2, level=-12.0, swell=True)
def lift(v, r):
    # The tell (1.3 s): the bell drawn up out of the cloth, its clapper knocking about inside as it's shaken, quicker and
    # harder toward the stroke; a cold breath drawn in under it.
    dur = 1.3
    m = handbell(PRIMES[v], child(r, 'bell'), t60=1.1, beat=0.9, bright=0.7)
    glance = normalize(M.strike(m, M.hammer(0.0009), 1.0))
    knock = normalize(M.strike(m, M.hammer(0.0004), 1.0))
    taps = np.zeros(ns(dur + 1.0))
    t = 0.2 + 0.05 * r.random()
    while t < dur - 0.05:
        u = t / dur
        # Early on it glances off the sides; toward the stroke it knocks.
        tap = glance if u < 0.55 else knock
        taps = add_at(taps, tap * (0.06 + 0.4 * u ** 2) * r.uniform(0.6, 1.0), t)
        t += (0.17 - 0.11 * u) * r.uniform(0.7, 1.3)
    cloth = kit.cloth(0.7, child(r, 'cloth'), 350.0, 3000.0, 140.0, 0.3, 0.6)
    d = 1.0
    breath = V.whisper(d, [(0.0, 'h'), (0.6, 'u'), (d, 'o')], child(r, 'breath'), 1.0) * E.swell(d, 0.85, 0.3)
    breath = normalize(F.filt(breath, F.hp(300.0), F.lp(5500.0), extend=False))
    x = layers((cloth, 0.0, -9.0), (normalize(taps), 0.0, 0.0), (breath, 0.25, -15.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(120.0), extend=False)
    return _outside(x, r, -20.0)


@cue('Creature.BellRinger.Struck', variations=4, att='Creature', jitter=0.05, conc=3, level=-8.0)
def struck(v, r):
    # A bullet on the bell: a small hard thing on its waist wakes its upper partials, a bright clang; the bell jolts, so
    # its clapper clatters against the sides; the handle jars in the fist.
    prime = PRIMES[v] * jitter(r, 1, 0.01)
    m = handbell(prime, child(r, 'bell'), t60=1.6, beat=1.0, bright=1.12)
    clang = ring(m, 1.8, child(r, 'clang'), 0.00006, -14.0)
    soft = normalize(M.strike(m, M.hammer(0.0004), 0.8))
    clatter = np.zeros(ns(1.2))
    t = 0.03
    for k in range(3 + int(r.integers(0, 2))):
        clatter = add_at(clatter, soft * (0.7 ** k) * r.uniform(0.7, 1.0), t)
        t += r.uniform(0.04, 0.08)
    jar = kit.thunk(240.0 * jitter(r, 1, 0.1), child(r, 'jar'), 0.05, 0.001)
    x = layers((clang, 0.0, 0.0), (normalize(clatter), 0.02, -8.0), (jar, 0.0, -14.0))
    x = F.filt(x, F.peak(3300.0, -4.0, 0.9), F.hp(120.0), extend=False)
    return _outside(x, r, -18.0)


@cue('Creature.BellRinger.Raise', variations=3, att='Creature', jitter=0.04, conc=3, level=-10.0, swell=True)
def raise_dead(v, r):
    # The dead called up: four whispering voices drawing up out of the ground, each in its own time, over the bell's hum
    # held long, beating slowly, a little ring-modulated so it isn't quite of this world; grave dust lifting.
    dur = 1.7
    n = ns(dur)
    voices = np.zeros(n + ns(0.2))
    for i in range(4):
        rr = child(r, 'voice', i)
        keys = [(0.0, 'h'), (0.35 + 0.1 * rr.random(), 'u'), (0.9 + 0.1 * rr.random(), 'o'), (dur, 'a')]
        w = V.whisper(dur, keys, rr, shift=rr.uniform(0.9, 1.15))
        env = E.bp([(0.0, 0.0), (0.25 + 0.2 * rr.random(), 0.4), (1.2, 1.0), (dur, 0.0)], n=n, curve='cos')
        voices = add_at(voices, normalize(w * env) * rr.uniform(0.6, 1.0), 0.05 * i * rr.random())
    voices = F.filt(voices, F.hp(280.0), F.lp(6000.0), extend=False)
    prime = PRIMES[v]
    t = times(n)
    beat = 1.2 + 1.5 * r.random()
    hum = np.sin(2 * np.pi * prime * 0.57 * t) + 0.8 * np.sin(2 * np.pi * (prime * 0.57 + beat) * t + 2 * np.pi * r.random())
    hum += 0.3 * np.sin(2 * np.pi * prime * t + 2 * np.pi * r.random())
    hum = _cold(hum * E.bp([(0.0, 0.0), (1.1, 1.0), (dur, 0.0)], n=n, curve='cos'), child(r, 'cold'), 29.0, 0.25)
    dust = N.pink(n, child(r, 'dust'), lo=300.0, hi=2500.0) * E.swell(dur, 1.2, 0.4)
    x = layers((normalize(voices), 0.0, 0.0), (normalize(hum), 0.0, -9.0), (normalize(dust), 0.2, -20.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(120.0), extend=False)
    return _haunt(x, r, -10.0, 2.0, 3000.0)


@cue('Creature.BellRinger.Swipe', variations=3, att='Creature', jitter=0.05, conc=3, level=-9.0, swell=True)
def swipe(v, r):
    # The bell swung as a club: its rush through the air, its clapper clanking against the sides, choked by the swing,
    # and a dull knock as it comes round.
    rush = kit.whoosh(0.32, child(r, 'rush'), 260.0, 1100.0, 0.9, 0.5, -3.0)
    m = handbell(PRIMES[v] * jitter(r, 1, 0.02), child(r, 'bell'), t60=0.35, beat=1.2, bright=0.6)
    clank = ring(m, 0.6, child(r, 'clank'), 0.0005, -18.0)
    knock = kit.thunk(180.0 * jitter(r, 1, 0.1), child(r, 'knock'), 0.07, 0.002)
    x = layers((rush, 0.0, 0.0), (clank, 0.15, -4.0), (knock, 0.15, -10.0))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(100.0), extend=False)
    return _outside(x, r, -20.0)
