"""The band that plays the score (music.py): its instruments and the desk they're mixed on, all synthesized from the
physics of each instrument, with nothing recorded.

    guitar     a steel-string acoustic: each partial of the plucked string with its own ring (the stiff string's
               stretch, the pluck's place, two polarizations beating and decaying apart, the pitch settling as a hard
               pluck lets go), the wooden body's resonances on the track, the fingertip's noise
    reed       a harmonica-like free reed: a buzzing harmonic source shaped by the player's mouth and cupped hands
               (lib.voice's formant model), scoops, bends, falls and a delayed vibrato
    whistle    a whistled tune: an almost pure tone with breath around it, gliding between notes
    bowed      a bowed string (cello, bass, viola): a sawtooth-like source with a lively bow, delayed vibrato, the bow's
               hiss and the body's formants; long drones, short spiccato strokes, bowed tremolo in an ensemble
    drums      a big drum, toms, a frame drum, a shaker, wood blocks, a church bell
    Desk       the sequencer and mixer: events at times, tracks with their effects, pans and reverb sends; a loop is
               rendered in a circle (tails folded round to the start, every filter and the reverb applied circularly)

Times are seconds; notes are names ('D3', 'F#4', 'Bb2') or Hz. Every random choice comes from the generator a recipe
passes, so the same recipe always plays the same take.
"""
import numpy as np

from lib.core import SR, ns, times, normalize, child, db2a, fit, layers
from lib import noise as N, env as E, osc as O, filters as F, dist as D, reverb as R, stereo as S, voice as V
from lib import modal as M, dynamics as Y
from lib.env import LN1000
from recipes import kit


def hz(note):
    """A note's frequency: a name ('D3', 'F#4', 'Bb2') or a frequency already."""
    return kit.note(note) if isinstance(note, str) else float(note)


def semis(a, b):
    """Semitones from frequency a up to b."""
    return 12.0 * np.log2(b / a)


def human(r, sd=0.006, limit=0.016):
    """A player's timing: a few milliseconds early or late (seconds)."""
    return float(np.clip(r.normal(0.0, sd), -limit, limit))


def lively(r, v, spread=0.12):
    """A velocity as a player plays it: never twice the same."""
    return float(np.clip(v * (1.0 + spread * (2.0 * r.random() - 1.0)), 0.05, 1.0))


def _rise(x, sec):
    k = min(ns(sec), x.shape[-1])
    if k > 1:
        x[..., :k] *= 0.5 - 0.5 * np.cos(np.pi * np.arange(k) / k)
    return x


# --- Guitar ---------------------------------------------------------------------------------------------------------

def guitar(f0, dur, r, vel=0.7, bright=0.5, pluck=None, release=0.07):
    """One plucked steel string, rung for dur seconds and then damped (by the next note on its string, or a hand).
    Each partial rings on its own: stretched a little by the string's stiffness, weighted by where it was plucked
    (a fingertip near the soundhole), the high ones dying first; two polarizations a hair apart in pitch decay at
    different rates, so the ring swims and has the double decay of a real string. A hard pluck starts a few cents
    sharp and settles as the string's tension relaxes. The body's resonances go on the track (guitar_body)."""
    f0 = hz(f0)
    n = ns(dur)
    if n < 16:
        return np.zeros(max(n, 1))
    p = pluck if pluck is not None else r.uniform(0.11, 0.17)
    stiff = 4.0e-5
    # How long the fundamental rings: the low strings longest.
    ring = float(np.clip(4.6 * (110.0 / f0) ** 0.35, 1.5, 6.0))
    # A soft pluck gives a rounder tone (the partials fall faster), a hard one more top.
    tilt = 1.08 - 0.3 * vel - 0.32 * bright
    t = np.arange(n) / SR
    sharp = 0.0024 * vel
    tau = 0.07
    warp = t + sharp * tau * (1.0 - np.exp(-t / tau))
    out = np.zeros(n)
    k = 1
    while k <= 90:
        fk = k * f0 * np.sqrt(1.0 + stiff * k * k)
        if fk > 15000.0:
            break
        a = abs(np.sin(np.pi * k * p)) / k ** tilt
        if a > 1.5e-4:
            t60 = ring / (1.0 + 0.11 * (k - 1) ** 1.4) / (1.0 + (fk / 3600.0) ** 2)
            m = min(n, ns(t60 * 2.2))
            tt, w = t[:m], warp[:m]
            split = 1.0 + 0.0008 * (r.random() - 0.5)
            fast = np.exp(-LN1000 * tt / t60) * np.sin(2.0 * np.pi * fk * w)
            slow = np.exp(-LN1000 * tt / (t60 * 2.2)) * np.sin(2.0 * np.pi * fk * split * w + 0.4 * r.random())
            out[:m] += a * (0.62 * fast + 0.38 * slow)
        k += 1
    # The fingertip leaves the string over a millisecond or so, not in an instant.
    _rise(out, 0.0012)
    # The flesh and nail on the string: a breath of noise at the attack, brighter for a harder pluck.
    tick = N.white(ns(0.005), r) * E.decay(ns(0.005), 0.0016)
    tick = F.filt(tick, F.bp(2200.0 + 1800.0 * vel, 0.9), extend=False)
    out[:tick.size] += tick * (0.05 + 0.08 * vel) * (np.max(np.abs(out[:ns(0.03)])) + 1e-9) / (np.max(np.abs(tick)) + 1e-9)
    return kit_fade_out(out, release) * vel


def kit_fade_out(x, sec):
    """x with a cosine fade over its last sec (a string damped by a fingertip)."""
    y = x.copy()
    k = min(ns(sec), y.size)
    if k > 1:
        y[-k:] *= 0.5 + 0.5 * np.cos(np.pi * np.arange(1, k + 1) / k)
    return y


def guitar_body(x, circular=False):
    """The acoustic guitar's box on a track of strings: the air mode's boom near 100 Hz, the top plate's near 200, the
    dip under 400 that keeps it from honking, a little wood in the mids and the steel's shimmer, the top rolled off."""
    return F.filt(x, F.hpn(82.0, 2), F.peak(104.0, 4.0, 2.4), F.peak(212.0, 3.0, 2.0), F.peak(400.0, -3.5, 1.3),
                  F.peak(640.0, 1.0, 1.5), F.peak(1250.0, 2.0, 1.0), F.peak(2900.0, 2.0, 1.1),
                  F.highshelf(7500.0, -5.0), extend=False, circular=circular)


class Guitar:
    """A guitar part: notes on strings (0 the lowest), each ringing until the next note on its string (or until a
    damp), as fingers really leave them."""

    def __init__(self, r, bright=0.5):
        self.r = r
        self.bright = bright
        self.notes = []
        self.damps = []

    def pick(self, t, string, note, vel=0.6, bright=None):
        self.notes.append((t + human(self.r, 0.005), string, hz(note), lively(self.r, vel, 0.14),
                           self.bright if bright is None else bright))

    def strum(self, t, notes, vel=0.6, down=True, spread=0.035, bright=None):
        """A chord strummed across its strings (notes low to high, one per string from the lowest given): a down
        stroke from the bass up, an up stroke from the treble down and lighter."""
        count = len(notes)
        first = 6 - count
        order = list(range(count)) if down else list(range(count))[::-1]
        t0 = t + human(self.r, 0.006)
        for i, j in enumerate(order):
            at = t0 + spread * (i / max(count - 1, 1)) * self.r.uniform(0.8, 1.2)
            v = vel * ((1.0 - 0.05 * i) if down else (0.72 + 0.04 * i))
            b = self.bright if bright is None else bright
            self.notes.append((at, first + j, hz(notes[j]), lively(self.r, v, 0.1), b + (0.12 if not down else 0.0)))

    def damp(self, t):
        """Every string stopped at t (a hand laid across them)."""
        self.damps.append(t)

    def render(self, desk, track, max_ring=4.5, gain_db=0.0):
        notes = sorted(self.notes)
        damps = sorted(self.damps)
        for i, (t, string, f, vel, bright) in enumerate(notes):
            end = t + max_ring
            for t2, s2, *_ in notes[i + 1:]:
                if s2 == string and t2 > t:
                    end = min(end, t2 + 0.01)
                    break
            for d in damps:
                if d > t + 0.05:
                    end = min(end, d)
                    break
            sig = guitar(f, max(end - t, 0.05), child(self.r, 'n', i), vel, bright)
            desk.add(track, sig, t, gain_db)


# --- Reed and whistle -----------------------------------------------------------------------------------------------

class Phrase:
    """A sung line (reed, whistle, bowed lead) as notes on a time line: (start, length, note, options). Options:
    scoop (semitones it slides up into the note from), fall (semitones it drops at the end), vib (vibrato depth in
    semitones), slur (no breath between it and the note before), accent (louder), bend (semitones a long note sags
    by its end)."""

    def __init__(self):
        self.notes = []

    def note(self, start, length, note, **opts):
        self.notes.append((start, length, hz(note), opts))
        return self

    def span(self):
        return self.notes[0][0], max(s + l for s, l, _, _ in self.notes)


def _pitch_line(notes, n, t0, glide=0.035, vib_rate=5.2, vib_delay=0.32, r=None):
    """The phrase's pitch per sample (Hz) and its amplitude envelope: glides between notes, scoops, falls, sags and
    a vibrato that grows in after the note has spoken."""
    t = times(n) + t0
    semi = np.zeros(n)
    amp = np.zeros(n)
    base = notes[0][2]
    vib_phase = r.random() * 2 * np.pi if r is not None else 0.0
    for i, (s, l, f, o) in enumerate(notes):
        target = semis(base, f)
        a = int(max(0, (s - t0) * SR))
        b = int(min(n, (s + l - t0) * SR))
        if b <= a:
            continue
        u = t[a:b] - s
        line = np.full(b - a, target)
        if o.get('scoop'):
            sc = float(o['scoop'])
            k = 0.13
            line -= sc * np.clip(1.0 - u / k, 0.0, 1.0) ** 2
        if o.get('bend'):
            line -= float(o['bend']) * np.clip(u / max(l, 1e-3), 0.0, 1.0) ** 2
        if o.get('fall'):
            k = min(0.22, l * 0.4)
            line -= float(o['fall']) * np.clip((u - (l - k)) / k, 0.0, 1.0) ** 2
        depth = float(o.get('vib', 0.16))
        grow = np.clip((u - vib_delay) / 0.35, 0.0, 1.0)
        rate = vib_rate * (1.0 + 0.04 * np.sin(2 * np.pi * 0.7 * u))
        line += depth * grow * np.sin(2 * np.pi * np.cumsum(rate) / SR + vib_phase)
        # Into this note from the last: a quick glide when they're joined (not after a breath).
        joined = i > 0 and o.get('slur', True) and s - (notes[i - 1][0] + notes[i - 1][1]) < 0.05
        if joined:
            prev = semis(base, notes[i - 1][2])
            g = np.clip(u / glide, 0.0, 1.0)
            g = g * g * (3 - 2 * g)
            line = prev + (line - prev) * g
        semi[a:b] = line
        acc = 1.25 if o.get('accent') else 1.0
        att = 0.018 if joined else 0.035
        follows = i + 1 < len(notes) and notes[i + 1][0] - (s + l) < 0.05 and notes[i + 1][3].get('slur', True)
        rel = 0.03 if follows else 0.09
        e = np.minimum(np.clip(u / att, 0, 1), np.clip((l - u) / rel, 0, 1))
        # A long note swells a little, then eases, as breath does.
        e = e * (0.88 + 0.12 * np.sin(np.pi * np.clip(u / max(l, 1e-3), 0, 1)))
        amp[a:b] = np.maximum(amp[a:b], e * acc)
    # Hold the pitch through the gaps, so nothing jumps where it's silent.
    filled = semi
    on = amp > 0
    if on.any() and not on.all():
        good = np.nonzero(on)[0]
        filled = np.interp(np.arange(n), good, semi[good])
    return base * 2.0 ** (filled / 12.0), amp


def reed(phrase, r, shift=1.22, tilt=4.0, breath=0.025, wah=True, top=8000.0, tremolo=11.0):
    """A harmonica-like free reed playing a phrase: the reed's buzzing harmonics through the player's mouth and cupped
    hands (lib.voice's formant model with a brighter source), a little breath, the cupped hands opening on long notes.
    tremolo (cents): a second reed on each note tuned that far sharp, as in a tremolo harmonica, so the two beat a
    few times a second (the shimmer of the old western harmonica). Returns (signal, start time)."""
    t0, t1 = phrase.span()
    t0 -= 0.05
    n = ns(t1 - t0 + 0.4)
    f0, amp = _pitch_line(phrase.notes, n, t0, glide=0.03, vib_rate=5.4, vib_delay=0.38, r=r)
    keys = []
    for s, l, f, o in phrase.notes:
        keys.append((max(0.0, s - t0), 'er'))
        if wah and l > 0.5:
            keys.append((max(0.0, s - t0) + min(0.45, l * 0.5), 'o' if l < 1.2 else 'ae'))
            keys.append((max(0.0, s - t0) + l * 0.95, 'er'))
    keys.sort(key=lambda k: k[0])
    x = V.voice(f0, keys, child(r, 'voice'), breath=breath, jitter=0.0035, shimmer=0.045, tilt=tilt, shift=shift,
                top=top)
    if tremolo:
        x2 = V.voice(f0 * 2.0 ** (tremolo / 1200.0), keys, child(r, 'voice2'), breath=breath, jitter=0.0035,
                     shimmer=0.045, tilt=tilt, shift=shift, top=top)
        x = x + 0.8 * x2
    x = x * amp
    # The reed's edge: a touch of grit, the plate's buzz.
    x = D.drive(normalize(x, -6.0), 4.0, 'tanh', bias=0.08)
    x = F.filt(x, F.hp(220.0, 0.7), F.peak(1300.0, 2.5, 1.0), F.peak(2400.0, 1.5, 1.4), F.lp(8000.0, 0.7),
               extend=False)
    return normalize(x, 0.0), t0


def whistle(phrase, r):
    """A whistled phrase: a nearly pure tone with a whisper of its upper partials and the breath around it, gliding
    between notes, a vibrato growing in on the long ones. Returns (signal, start time)."""
    t0, t1 = phrase.span()
    t0 -= 0.05
    n = ns(t1 - t0 + 0.4)
    f0, amp = _pitch_line(phrase.notes, n, t0, glide=0.06, vib_rate=5.6, vib_delay=0.3, r=r)
    ph = 2 * np.pi * O.phase(f0, n)
    tone = np.sin(ph) + 0.05 * np.sin(2 * ph + 0.3) + 0.012 * np.sin(3 * ph + 1.1)
    tone *= 1.0 + 0.04 * N.smooth_random(n, child(r, 'sh'), 9.0)
    air = F.sweep(N.white(n, child(r, 'air')), 'bp', np.clip(f0, 300.0, 9000.0), q=9.0)
    air = air / (np.sqrt(np.mean(air * air)) + 1e-9) * 0.05
    hiss = F.filt(N.white(n, child(r, 'hiss')), F.hp(3500.0), F.lp(9000.0), extend=False) * 0.008
    x = (tone + air + hiss) * amp
    return normalize(F.filt(x, F.hp(250.0), extend=False), 0.0), t0


# --- Bowed strings --------------------------------------------------------------------------------------------------

def bowed(f0, dur, r, vel=0.7, attack=0.5, release=0.8, vib=0.12, vib_rate=5.3, vib_delay=0.3, bright=0.5,
          kmax=48, env=None, noise_db=-30.0, detune=0.0):
    """A bowed string: the stick-slip of the bow drives a sawtooth-like wave whose harmonics live a little apart
    (the bow's pressure and speed never hold still), a vibrato grows in after the note speaks, the bow hisses on
    the string. f0 may be a curve (Hz per sample, its length is the note's). env overrides the swell (attack and
    release in seconds)."""
    if np.isscalar(f0) or isinstance(f0, str):
        n = ns(dur)
        base = np.full(n, hz(f0))
    else:
        base = np.asarray(f0, dtype=float)
        n = base.size
    t = times(n)
    phase0 = r.random() * 2 * np.pi
    grow = np.clip((t - vib_delay) / 0.4, 0.0, 1.0)
    drift = 0.02 * N.smooth_random(n, child(r, 'drift'), 2.5)
    rate = vib_rate * (1.0 + 0.05 * N.smooth_random(n, child(r, 'rate'), 0.8))
    cents = (vib * grow * np.sin(2 * np.pi * np.cumsum(rate) / SR + phase0) + drift) + detune / 100.0
    f = base * 2.0 ** (cents / 12.0)
    ph = 2 * np.pi * O.phase(f, n)
    top = min(kmax, int(7000.0 / max(float(base.max()), 1.0)))
    lives = [N.smooth_random(n, child(r, 'life', j), 3.0 + 1.5 * j) for j in range(4)]
    out = np.zeros(n)
    beta = r.uniform(0.09, 0.14)
    for k in range(1, max(top, 1) + 1):
        a = (1.0 / k) * (0.3 + 0.7 * abs(np.sin(np.pi * k * beta))) * k ** ((bright - 0.6) * 0.7)
        w = r.random(4) * 0.5
        life = 1.0 + 0.16 * (w[0] * lives[0] + w[1] * lives[1] + w[2] * lives[2] + w[3] * lives[3])
        out += a * life * np.sin(k * ph)
    if env is None:
        env = E.ar(n / SR, attack, release, 2.0)
        env = fit(env, n)
    out *= env
    rms = np.sqrt(np.mean(out * out)) + 1e-9
    hiss = F.filt(N.white(n, child(r, 'bow')), F.bp(2600.0, 0.7), extend=False)
    hiss *= env * (0.7 + 0.3 * np.abs(N.smooth_random(n, child(r, 'press'), 7.0)))
    hiss = hiss / (np.sqrt(np.mean(hiss * hiss)) + 1e-9) * rms * db2a(noise_db)
    return (out + hiss) * vel


def cello_body(x, circular=False):
    """A cello's body on a bowed track: the wood's resonances in the low mids and the bridge's hill near 2-3 kHz,
    rounded off above, so the bowed saw sounds like wood, not a synth."""
    return F.filt(x, F.hp(40.0, 0.7), F.peak(220.0, 3.0, 1.4), F.peak(450.0, 2.0, 1.2), F.peak(850.0, -2.0, 1.0),
                  F.peak(1400.0, 2.5, 1.2), F.peak(2600.0, 2.0, 1.6), F.highshelf(5000.0, -9.0),
                  extend=False, circular=circular)


def spiccato(f0, r, vel=0.8, length=0.22):
    """A short, bouncing bow stroke (the low strings' ostinato): a quick bite, the bow's scratch, a short ring."""
    n = ns(length + 0.25)
    env = E.perc(n / SR, 0.006, length, curve=1.2)
    x = bowed(f0, n / SR, r, 1.0, vib=0.0, vib_delay=9.0, bright=0.65, kmax=30, env=env, noise_db=-24.0)
    scratch = F.filt(N.white(ns(0.02), child(r, 'scratch')), F.bp(1400.0, 0.8), extend=False)
    scratch *= E.decay(ns(0.02), 0.012)
    x[:scratch.size] += scratch * np.max(np.abs(x)) * 0.25 / (np.max(np.abs(scratch)) + 1e-9)
    return x * vel


def ensemble(f0, dur, r, vel=0.6, voices=4, attack=0.6, release=1.0, tremolo=0.0, detune=7.0, width=0.7,
             bright=0.5, vib=0.11, env=None, kmax=30):
    """A section of bowed strings on one note: voices a few cents apart, each with its own vibrato and bow, spread
    across the stereo field. tremolo > 0: each player's bow shakes back and forth that many strokes a second, every
    stroke speaking again (the scratchy shimmer of tension). Returns stereo."""
    n = ns(dur) if np.isscalar(f0) or isinstance(f0, str) else len(f0)
    out = np.zeros((2, n))
    for v in range(voices):
        rv = child(r, 'voice', v)
        d = detune * (2.0 * (v + 0.5) / voices - 1.0) + rv.normal(0, 1.5)
        x = bowed(f0, dur, rv, 1.0, attack * rv.uniform(0.8, 1.2), release, vib * rv.uniform(0.8, 1.25),
                  5.0 + rv.random() * 0.9, 0.2 + 0.2 * rv.random(), bright, kmax=kmax, env=env, detune=d,
                  noise_db=-26.0 if tremolo else -30.0)
        if tremolo:
            t = times(n)
            stroke = tremolo * rv.uniform(0.92, 1.08)
            # Each stroke is a rise and a fall of the bow's grip: |sin| bumps, a hair uneven.
            bump = np.abs(np.sin(np.pi * stroke * t + rv.random() * np.pi)) ** 0.6
            jitter = 1.0 + 0.15 * N.smooth_random(n, child(rv, 'tj'), stroke)
            x = x * (0.25 + 0.75 * bump * jitter)
        pan = width * (2.0 * (v + 0.5) / voices - 1.0)
        out += S.pan(x, pan)
    return out * (vel / np.sqrt(voices))


# --- Drums and bells ------------------------------------------------------------------------------------------------

def big_drum(r, vel=0.9, f0=58.0, t60=0.9):
    """A big bass drum or taiko: the head's low modes dropping in pitch as it relaxes, the air it pushes, the beater."""
    head = O.membrane(f0 * r.uniform(0.98, 1.02), 1.4, r, t60=t60, bend=0.3, bend_time=0.035)
    air = kit.noise_thump(0.5, child(r, 'air'), 700.0, 70.0, 0.25, 1.0, 0.04)
    beater = F.filt(N.white(ns(0.006), child(r, 'beater')) * E.decay(ns(0.006), 0.002), F.bp(1800.0, 0.8),
                    extend=False)
    x = layers((normalize(head), 0.0, 0.0), (air, 0.0, -7.0), (normalize(beater), 0.0, -22.0 + 6.0 * vel))
    return normalize(_rise(x, 0.0008), 0.0) * vel


def tom(f0, r, vel=0.8, t60=0.38):
    """A tom: a tighter head, its pitch bending down, the stick's crack."""
    head = O.membrane(hz(f0) * r.uniform(0.985, 1.015), t60 * 1.6, r, t60=t60, bend=0.14, bend_time=0.03)
    stick = F.filt(N.white(ns(0.012), child(r, 'stick')) * E.decay(ns(0.012), 0.004), F.bp(1500.0, 0.7),
                   extend=False)
    x = layers((normalize(head), 0.0, 0.0), (normalize(stick), 0.0, -17.0 + 5.0 * vel))
    return normalize(_rise(x, 0.0006), 0.0) * vel


def frame_drum(r, vel=0.7):
    """A frame drum slapped near its rim: a short, dark head and the skin's papery slap."""
    head = O.membrane(185.0 * r.uniform(0.97, 1.03), 0.35, r, t60=0.16, bend=0.08, bend_time=0.02)
    slap = F.filt(N.pink(ns(0.08), child(r, 'slap')) * E.perc(0.08, 0.0008, 0.05), F.hp(300.0), F.lp(5000.0),
                  extend=False)
    x = layers((normalize(head), 0.0, 0.0), (normalize(slap), 0.0, -7.0))
    return normalize(_rise(x, 0.0006), 0.0) * vel


def shaker(r, vel=0.5, length=0.07):
    """A gourd shaker's flick: its seeds hitting the shell together, a soft swell and a quick stop."""
    n = ns(length)
    seeds = N.dust(n, r, 9000.0, (0.3, 1.0))
    seeds = F.filt(seeds, F.bp(5200.0, 0.9), F.highshelf(8000.0, -6.0), extend=False)
    env = E.ar(length, length * 0.3, length * 0.6, 2.0)
    return normalize(seeds * fit(env, n), 0.0) * vel


def wood_block(r, vel=0.6, f0=900.0):
    """A wood block (a horse's hoof on hard ground, in a gallop): a hollow knock with a short pitch."""
    x = kit.thunk(f0 * r.uniform(0.97, 1.03), r, 0.05, 0.0004, 5, 0.04)
    return normalize(x, 0.0) * vel


def bell(prime, r, vel=0.7, t60=7.0, dur=7.0, bright=0.8):
    """A bronze bell struck by its clapper (the chapel's): the bell's partials, each a slowly beating pair."""
    m = M.bell(hz(prime), child(r, 'bell'), t60, 0.8, bright)
    ring = M.strike(m, M.hammer(0.0003), dur)
    clang = M.strike(M.parts(2500.0, 8000.0, child(r, 'clang'), 7, 0.06, 0.02), M.hammer(0.00008), 0.12)
    x = layers((normalize(ring), 0.0, 0.0), (normalize(clang), 0.0, -20.0))
    return normalize(x, 0.0) * vel


def anvil(r, vel=0.7, f0=820.0):
    """A struck steel bar (an anvil, a railroad spike): bright, ringing, inharmonic."""
    m = M.bar(f0, child(r, 'bar'), t60=1.4, count=6)
    x = M.strike(m, M.hammer(0.00012), 1.6)
    return normalize(x, 0.0) * vel


def swell(dur, r, lo=200.0, hi=6000.0):
    """A reversed cymbal: noise brightening and swelling to a stop at dur (a riser into a hit)."""
    n = ns(dur)
    x = N.pink(n, r)
    fc = O.expsweep(lo, hi, dur, 0.6, n=n)
    y = F.sweep(x, 'lp', fc, 0.8)
    env = (np.arange(n) / n) ** 3
    return normalize(y * env, 0.0)


# --- Reverb ---------------------------------------------------------------------------------------------------------

def hall(r, t60=2.3, predelay=0.024, lp=7000.0, early_db=-5.0):
    """A stereo hall for the score: sparse early reflections, then a dense tail whose highs die first; the two
    channels are separate rooms' worth of noise, so the reverb is wide."""
    dur = t60 * 1.1
    n = ns(dur)
    chans = []
    for c in range(2):
        rc = child(r, 'hall', c)
        tail = R.decaying_noise(dur, rc, t60, t60 * 0.38, 300.0, 6000.0)
        onset = np.clip((times(n) - predelay) / 0.035, 0.0, 1.0) ** 2
        tail = F.filt(tail * onset, F.lp(lp, 0.6), extend=False)
        tail /= np.sqrt(np.sum(tail * tail)) + 1e-12
        early = R.early(rc, span=0.07, count=24, lp=7500.0, first=predelay * 0.4)
        chans.append(fit(tail, n) + fit(early, n) * db2a(early_db))
    ir = np.vstack(chans)
    return ir / np.sqrt(np.sum(ir * ir) / 2.0)


def circ_convolve(x, ir, L):
    """x (a loop of L samples, mono or stereo) through ir, circularly: the tail past the end comes round to the
    start, as it would on the loop's second time through."""
    X = np.fft.rfft(x, L, axis=-1)
    H = np.fft.rfft(ir[..., :L], L, axis=-1)
    if X.ndim == 1:
        X = X[None, :]
    if H.ndim == 1:
        H = H[None, :]
    return np.fft.irfft(X * H, L, axis=-1)


def periodic(fn, x):
    """fn (something with memory or foresight: a compressor, a look-ahead limiter) applied to a loop as a loop: run on
    three laps, the middle kept, so its start carries on from its own end and its end sees its own start coming (a
    limiter at the last bar must already be ducking for the downbeat's hit)."""
    L = x.shape[-1]
    y = fn(np.concatenate([x, x, x], axis=-1))
    return y[..., L:2 * L]


def fold(x, L):
    """A loop's events with their tails: whatever runs past L comes round to the start."""
    y = x[..., :L].copy()
    rest = x[..., L:]
    while rest.shape[-1] > 0:
        k = min(L, rest.shape[-1])
        y[..., :k] += rest[..., :k]
        rest = rest[..., k:]
    return y


def loudest(x, win=0.4):
    """The loudest win-second RMS of x (its loudest moment, as a level to mix by)."""
    m2 = x * x if x.ndim == 1 else np.mean(x * x, axis=0)
    k = max(1, ns(win))
    c = np.concatenate([[0.0], np.cumsum(m2)])
    if m2.size <= k:
        return float(np.sqrt(c[-1] / max(m2.size, 1)))
    return float(np.sqrt(np.max(c[k:] - c[:-k]) / k))


# --- The desk -------------------------------------------------------------------------------------------------------

class Desk:
    """The score's mixer. A loop (length seconds) is rendered in a circle; a one-shot (length None) runs as long as
    it rings. Tracks are set up with their effects, level (dB against the others, by each track's loudest moment),
    pan, width and reverb send; events are added at times."""

    def __init__(self, length=None, tail=7.0):
        self.L = ns(length) if length else None
        self.n = (self.L or 0) + ns(tail)
        self.tracks = {}
        self.order = []

    def track(self, name, level=0.0, pan=0.0, send=-14.0, fx=None, width=1.0):
        self.tracks[name] = {'buf': np.zeros((2, self.n)), 'level': level, 'pan': pan, 'send': send, 'fx': fx,
                             'width': width}
        self.order.append(name)

    def add(self, name, sig, t, gain_db=0.0, pan=None):
        tr = self.tracks[name]
        if sig.ndim == 1:
            sig = S.pan(sig, tr['pan'] if pan is None else pan) / np.sqrt(2.0)
        start = max(0, ns(t)) if t >= 0 else 0
        if t < 0 and self.L:
            # Before the loop's start: it lands at the end of the loop, and folds round.
            start = self.L + ns(t)
        end = start + sig.shape[-1]
        if end > self.n:
            grow = end - self.n
            for k in self.tracks.values():
                k['buf'] = np.pad(k['buf'], ((0, 0), (0, grow)))
            self.n = end
        tr['buf'][:, start:end] += sig * db2a(gain_db)

    def mix(self, ir, master=None, wet_db=0.0):
        """Every track through its effects, at its level and width, summed with the reverb of their sends; master
        (x, circular) -> x finishes the bus."""
        loop = self.L is not None
        L = self.L if loop else self.n
        dry = np.zeros((2, L))
        send = np.zeros((2, L))
        for name in self.order:
            tr = self.tracks[name]
            x = fold(tr['buf'], L) if loop else tr['buf'][:, :L]
            if not np.any(x):
                continue
            if tr['fx']:
                x = tr['fx'](x, loop)
            if tr['width'] != 1.0:
                x = S.width(x, tr['width'])
            lv = loudest(x)
            if lv <= 0:
                continue
            x = x * (db2a(tr['level']) * 0.1 / lv)
            dry += x
            if tr['send'] is not None and tr['send'] > -80:
                send += x * db2a(tr['send'])
        if loop:
            wet = circ_convolve(send, ir, L)
        else:
            wet = F.convolve(send, ir)
            dry = fit(dry, wet.shape[-1])
        out = dry + wet * db2a(wet_db)
        if master:
            out = master(out, loop)
        return out


def master_bus(x, circular, low_cut=45.0, presence_cut=-2.5, top_cut=-3.0, glue=True):
    """The score's bus, mixed to sit under the game: the sub-bass cleared (the guns' weight lives there), a dip at
    the ear's sharpest band (where gunfire's crack and the creatures' tells live), the very top eased; a gentle glue
    compressor and a ceiling."""
    y = F.filt(x, F.hpn(low_cut, 2), F.peak(3300.0, presence_cut, 0.8), F.highshelf(9500.0, top_cut),
               extend=False, circular=circular)
    y = normalize(y, -6.0)
    if glue:
        comp = (lambda s: Y.compress(s, -16.0, 2.0, 0.02, 0.3, 8.0))
        y = periodic(comp, y) if circular else comp(y)
    lim = (lambda s: Y.limit(s, -1.0, 0.003, 0.12))
    return periodic(lim, normalize(y, -1.0)) if circular else lim(normalize(y, -1.0))


# --- Time -----------------------------------------------------------------------------------------------------------

class Meter:
    """Where the beats fall: bpm counts the beat (the dotted quarter in 6/8, the quarter in 4/4), sub divides it
    (3 eighths a beat in 6/8, 4 sixteenths in 4/4), beats is per bar."""

    def __init__(self, bpm, beats, sub):
        self.beat = 60.0 / bpm
        self.beats = beats
        self.sub = sub
        self.step = self.beat / sub
        self.bar = self.beat * beats

    def at(self, bar, step=0.0):
        """The time of a step (in sub-beats, from 0) of a bar (from 0)."""
        return bar * self.bar + step * self.step
