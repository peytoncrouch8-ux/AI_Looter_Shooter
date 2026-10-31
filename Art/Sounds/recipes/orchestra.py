"""The orchestra for the score's orchestral pieces (the menu theme, the boss fights), on band.py's Desk. Every voice is
synthesized from its instrument's physics; nothing is recorded.

Conventions (shared with band.py): times in seconds, notes as names ('D3', 'F#4', 'Bb2') or Hz, every random choice
from the generator the recipe passes. vel (0..1) is how hard a note is played: it sets the loudness AND the
brightness, as on real instruments (a soft horn is round and dark, a loud one brassy).
- One player's note: fn(note, dur, r, vel=...) -> mono, starting at its onset (desk.add(track, x, t)).
- A Line (a melody: (start, length, note, vel, options) in absolute seconds; line() builds one from MAIN_MOTIF-style
  (note, beats) pairs) or a section (2-12 players, each a few cents, milliseconds and a vibrato phase apart):
  -> (signal, t0), t0 absolute: desk.add(track, *result). Note-based section calls take at= (the note's time).
  Sections start a little before their notes (players come in early or late, a bow or breath before the sound). In
  a loop that wraps round; in a one-shot Desk a negative time is clamped to 0 (the notes would land late), so a
  one-shot's first notes go at 0.2 s or later (the render trims the silence).
- Stereo results ignore the Desk track's pan: sections sit by SEATS (pan=, width= override). Mono ones use it.
- Takes(r): round-robin takes of short repeated notes (pizzicato, col legno, spiccato, marcato, brass stabs, and the
  struck percussion: timpani, taiko, rim, bass drum, anvil, stone) rendered once per key and reused (pass takes=), so
  a 32-bar ostinato costs a few renders, not hundreds.

    brass      brass(kind, note, dur, r, vel, art) / horn() trombone() trumpet() tuba(): a lip-buzz harmonic source
               whose spectrum opens with the level (the bore's brassiness), an attack blip and tongue puff, breath,
               vibrato only on long notes, through the bell's EQ. art: normal, stac, marc, sfz, swell, cresc, dim, fp.
               brass_line(kind, line, r) one player legato (lip slurs);  brass_section(kind, note | chord | line, ...)
    strings    string_legato(section, line, r) long bows, slurred glides, bow changes;  string_sustain (held notes or
               chords, long bows), string_marcato, string_spiccato, string_pizz, string_col_legno, string_tremolo
               (measured-free bowed tremolo, sul ponticello by pont=), string_shriek (the high clawing cluster).
               Sections: violins, violas, cellos, basses; each player has its own bow position, detune, vibrato,
               timing; each section its own body (a jagged set of wood resonances the vibrato moves through).
    choir      choir(part, notes, dur, r, vowel), choir_line(part, line, r, vowel): wordless 'ah', 'oh', 'oo', 'ee',
               'mm' (humming), 'hah' and 'hm' (a work chant's breathed syllables); parts bass, tenor, alto, soprano
               (or men / women, by pitch); 8+ singers a note, breathy, scooping, drifting, never in lockstep.
               whisper_choir(dur, r): many voices whispering, no pitch.
    percussion timpani(note), timpani_roll(note, dur), taiko(), taiko_rim(), bass_drum(), bass_drum_roll(),
               cymbal_crash(), cymbal_swell(dur), tam_tam(), anvil(kind='anvil' | 'rail'), stone(), stone_grind(dur)
    keys       harp(note), harp_gliss(lo, hi, dur), celesta(note), glockenspiel(note), piano(note) (the low strike)
    room       orchestra_hall(r): a scoring stage (wide, ~3 s, highs dying first);  orchestra_bus(x, circular): the
               master (35 Hz cut, mud and presence eased, gentle glue, a ceiling)
    MAIN_MOTIF the game's main motif (D minor), MOTIF_HARMONY its chords;  SEATS where each section sits (pan)

"""
import numpy as np

from lib.core import SR, ns, times, normalize, child, db2a, fit, layers, seed_of, smoothstep
from lib import noise as N, env as E, osc as O, filters as F, dist as D, reverb as R, stereo as S, voice as V
from lib import modal as M, dynamics as Y, granular as G
from lib.env import LN1000
from recipes import cue, kit
from recipes import band as B

hz = B.hz
human = B.human
lively = B.lively

# --- The main motif -------------------------------------------------------------------------------------------------

# Teropa's motif (2026-10-31, original): a rising fifth (the long road, noble), a dip and a reach past it to the
# minor sixth (the yearning), then a sigh down by thirds with a lift before it settles on the second degree, left
# open over the dominant (a dead man still walking). Singable in a minor sixth's range. The bosses quote the first
# four notes (D A G Bb). Beats are quarter notes; it fills three 4/4 bars.
MAIN_MOTIF = [('D4', 1.0), ('A4', 1.5), ('G4', 0.5), ('Bb4', 2.0), ('A4', 1.0), ('F4', 1.0), ('G4', 1.0), ('E4', 4.0)]
# Its harmony: (beat, chord). Dm under the fifth, Bb under the reach (the Bb4 tied over the bar line), F then Dm under
# the sigh, C under the lift, A (major: the half cadence) under the last note. In F major (the menu's B section) the
# same shape works a third up: F4 C5 Bb4 D5 C5 A4 Bb4 G4 over F, Bb (or Gm), F, Dm, C, C.
MOTIF_HARMONY = [(0.0, 'Dm'), (3.0, 'Bb'), (5.0, 'F'), (6.0, 'Dm'), (7.0, 'C'), (8.0, 'A')]
CHORDS = {'Dm': ('D', 'F', 'A'), 'Bb': ('Bb', 'D', 'F'), 'F': ('F', 'A', 'C'), 'C': ('C', 'E', 'G'),
          'A': ('A', 'C#', 'E'), 'Gm': ('G', 'Bb', 'D'), 'Am': ('A', 'C', 'E'), 'Eb': ('Eb', 'G', 'Bb'),
          'Cm': ('C', 'Eb', 'G'), 'G': ('G', 'B', 'D'), 'E': ('E', 'G#', 'B')}

# Where each section sits on the stage (pan), as section calls place them by default.
SEATS = {'violins': -0.45, 'violins2': -0.15, 'violas': 0.12, 'cellos': 0.38, 'basses': 0.55, 'horn': -0.3,
         'trumpet': 0.1, 'trombone': 0.32, 'tuba': 0.45, 'choir': 0.0, 'harp': -0.55, 'celesta': -0.4,
         'timpani': -0.08, 'perc': 0.15}

D_MINOR = ['D', 'E', 'F', 'G', 'A', 'Bb', 'C']


# --- Lines ----------------------------------------------------------------------------------------------------------

class Line:
    """A melody for one player or a section in unison: notes (start, length, note, vel, options), absolute seconds.
    Options: slur (default True: joined to the note before when it starts within 60 ms of its end), dyn (flat, swell,
    cresc, dim, fp, sfz, marc, chant), accent, scoop (semitones it slides up into the note from), fall (semitones it
    drops at the end), bend (semitones a long note sags by its end; negative climbs), vib (vibrato depth in
    semitones, overriding the instrument's), glide (seconds), att (attack seconds), release, vowel (choir)."""

    def __init__(self):
        self.notes = []

    def note(self, start, length, note, vel=0.7, **opts):
        self.notes.append([float(start), float(length), hz(note), float(vel), dict(opts)])
        return self

    def span(self):
        return self.notes[0][0], max(s + l for s, l, *_ in self.notes)

    def moved(self, semis=0.0, dt=0.0, vel=1.0):
        """A copy transposed by semis, moved by dt seconds, its velocities scaled by vel."""
        out = Line()
        k = 2.0 ** (semis / 12.0)
        out.notes = [[s + dt, l, f * k, min(1.0, v * vel), dict(o)] for s, l, f, v, o in self.notes]
        return out


def line(motif, t0, beat, vel=0.7, semis=0.0, slur=True, gap=0.0, count=None, **opts):
    """A Line from (note, beats) pairs (MAIN_MOTIF), from t0 with beat seconds a beat, transposed by semis. A note
    None is a rest. slur False tongues (or bows) every note; gap trims each note by that many seconds; count keeps
    only the first notes (count=4: the bosses' quote). opts go on every note."""
    out = Line()
    t = t0
    for i, (note, beats) in enumerate(motif[:count] if count else motif):
        if note is not None:
            o = dict(opts)
            if i > 0 and not slur:
                o['slur'] = False
            out.note(t, beats * beat - gap, hz(note) * 2.0 ** (semis / 12.0), vel, **o)
        t += beats * beat
    return out


def _dyn(kind, u, length):
    """A note's dynamic shape over its time u (seconds from its start) and length."""
    x = np.clip(u / max(length, 1e-3), 0.0, 1.0)
    up = np.maximum(u, 0.0)
    if kind == 'swell':
        y = np.where(x < 0.55, x / 0.55, (1.0 - x) / 0.45)
        return 0.25 + 0.75 * smoothstep(y)
    if kind == 'cresc':
        return 0.22 + 0.78 * x ** 1.5
    if kind == 'dim':
        return 1.0 - 0.72 * x ** 0.8
    if kind == 'fp':
        return np.maximum(0.3 + 0.7 * np.exp(-up / 0.07), 0.3 + 0.7 * x ** 1.8)
    if kind == 'sfz':
        return 0.55 + 0.6 * np.exp(-up / 0.11)
    if kind == 'marc':
        return 0.68 + 0.42 * np.exp(-up / 0.08)
    if kind == 'chant':
        return 0.45 + 0.65 * np.exp(-up / 0.1)
    return 0.96 + 0.06 * np.sin(np.pi * x)


def _wander(n, r, rate):
    """A slow random curve in [-1, 1] with about rate turns a second (noise.smooth_random worked out every 32
    samples and drawn between: the same curve at a fraction of the cost, for the many per-player wobbles)."""
    n = int(n)
    m = n // 32 + 3
    c = N.smooth_random(m, r, rate * 32.0)
    return np.interp(np.arange(n) / 32.0, np.arange(m), c)


def _contour(notes, r, t0, n, glide=0.04, attack=0.04, release=0.1, vib=0.0, vib_rate=5.5, vib_delay=0.3,
             vib_min=0.0, scoop=0.0, scoop_time=0.02, slur_dip=0.1, end_fall=0.0, blip=0.0, drift=3.0, jitter=0.0,
             cents=0.0, timing=0.0, port=0.0):
    """One player's pitch (Hz per sample) and level (vel x dynamics x envelope, per sample) through a line's notes:
    slurred notes glide into each other with a small dip (a lip slur, a finger change), separate ones speak again
    (a tongue, a bow change) with an optional blip; the attack starts scoop semitones flat; vibrato grows in on notes
    at least vib_min long; the player drifts a few cents and starts each note a few ms off (timing, seconds sd)."""
    t = t0 + times(n)
    semi = np.zeros(n)
    owned = np.zeros(n, dtype=bool)
    level = np.zeros(n)
    base = notes[0][2]
    rate = vib_rate * (1.0 + 0.05 * _wander(n, child(r, 'vr'), 0.7))
    vsin = np.sin(2.0 * np.pi * np.cumsum(rate) / SR + 2.0 * np.pi * r.random())
    shift = [human(r, timing, 3.0 * timing) if timing else 0.0 for _ in notes]
    onsets = []
    count = len(notes)
    for i, (s0, l, f, v, o) in enumerate(notes):
        s = s0 + shift[i]
        joined_in = i > 0 and o.get('slur', True) and s0 - (notes[i - 1][0] + notes[i - 1][1]) < 0.06
        joined_out = (i + 1 < count and notes[i + 1][4].get('slur', True) and
                      notes[i + 1][0] - (s0 + l) < 0.06)
        e = notes[i + 1][0] + shift[i + 1] if joined_out else s + l
        gl = o.get('glide', glide)
        rel = o.get('release', release)
        a_t = s - gl / 2 if joined_in else s
        b_t = e + gl / 2 if joined_out else e + rel
        a = max(0, int(round((a_t - t0) * SR)))
        b = min(n, int(round((b_t - t0) * SR)))
        if b <= a:
            continue
        tt = t[a:b]
        u = tt - s
        shape = _dyn(o.get('dyn', 'flat'), u, e - s)
        if joined_in:
            rise = np.clip((u + gl / 2) / gl, 0.0, 1.0)
        else:
            att = o.get('att', attack * (1.3 - 0.6 * v) / 0.88)
            rise = 0.5 - 0.5 * np.cos(np.pi * np.clip(u / att, 0.0, 1.0))
        if joined_out:
            fall = np.clip((e + gl / 2 - tt) / gl, 0.0, 1.0)
        else:
            fall = np.clip((e + rel - tt) / rel, 0.0, 1.0) ** 1.5
        env = rise * fall * shape * v * (1.25 if o.get('accent') else 1.0)
        if not joined_in and blip > 0:
            env = env * (1.0 + blip * v * np.exp(-np.maximum(u, 0.0) / 0.028))
        if joined_in and slur_dip > 0:
            env = env * (1.0 - slur_dip * np.exp(-(u / (gl * 0.6)) ** 2))
        level[a:b] += env
        target = B.semis(base, f)
        pitch = np.full(b - a, target)
        if joined_in:
            prev = B.semis(base, notes[i - 1][2])
            g2 = gl * (1.0 + port * min(abs(target - prev), 12.0) / 4.0)
            pitch = prev + (target - prev) * smoothstep((u + g2 / 2) / g2)
        else:
            sc = scoop * (1.6 - v)
            if sc:
                pitch = pitch - sc * np.exp(-np.maximum(u, 0.0) / scoop_time)
            if o.get('scoop'):
                pitch = pitch - float(o['scoop']) * np.clip(1.0 - u / 0.12, 0.0, 1.0) ** 2
        if o.get('fall'):
            k = min(0.25, (e - s) * 0.4)
            pitch = pitch - float(o['fall']) * np.clip((u - (e - s - k)) / k, 0.0, 1.0) ** 2
        if o.get('bend'):
            pitch = pitch - float(o['bend']) * np.clip(u / max(e - s, 1e-3), 0.0, 1.0) ** 2
        if end_fall and not joined_out:
            pitch = pitch - end_fall * np.clip((tt - e) / rel, 0.0, 1.0)
        depth = o.get('vib', vib if e - s >= vib_min else 0.0)
        if depth:
            pitch = pitch + depth * np.clip((u - vib_delay) / 0.45, 0.0, 1.0) * vsin[a:b]
        semi[a:b] = pitch
        owned[a:b] = True
        onsets.append((s, v, joined_in, o))
    if owned.any() and not owned.all():
        good = np.nonzero(owned)[0]
        semi = np.interp(np.arange(n), good, semi[good])
    slow = (cents + drift * _wander(n, child(r, 'drift'), 0.35)) / 100.0
    if jitter:
        slow = slow + jitter / 100.0 * _wander(n, child(r, 'jit'), 23.0)
    return base * 2.0 ** ((semi + slow) / 12.0), level, onsets


# --- The harmonic engine --------------------------------------------------------------------------------------------

def _harmonics(f0, level, slope, gains, r, top=9500.0, wobble=0.0, groups=3):
    """sum over k of gains[k-1] * level * m^(k-1) * sin(k * phase), where m = 10^(-slope * f0 / 20000): every
    harmonic loses slope dB per kHz against the one below, and slope follows the level, so the tone opens as it gets
    louder (brass and bowed strings both brighten with force). sin(k ph) comes from the Chebyshev recursion (cheap);
    harmonics fade out near top; wobble deals them round into interleaved groups, each with its own slow life, so the
    spectrum never holds still."""
    n = f0.size
    ph = 2.0 * np.pi * np.mod(O.phase(f0, n), 1.0)
    roll = 10.0 ** (-np.maximum(slope, 0.0) * f0 / 20000.0)
    edge = 0.08 * top
    bufs = [np.zeros(n) for _ in range(groups if wobble else 1)]
    K = len(gains)
    CH = 8192
    for a in range(0, n, CH):
        b = min(n, a + CH)
        lv = level[a:b]
        if lv.max() < 1e-7:
            continue
        p = ph[a:b]
        fc = f0[a:b]
        m = roll[a:b]
        fmin, fmax = float(fc.min()), float(fc.max())
        s0 = np.zeros(b - a)
        s1 = np.sin(p)
        c2 = 2.0 * np.cos(p)
        amp = lv.copy()
        for k in range(1, K + 1):
            if k * fmin >= top:
                break
            buf = bufs[(k - 1) % len(bufs)]
            if k * fmax > top - edge:
                buf[a:b] += (gains[k - 1] * amp) * s1 * np.clip((top - k * fc) / edge, 0.0, 1.0)
            else:
                buf[a:b] += (gains[k - 1] * amp) * s1
            s0, s1 = s1, c2 * s1 - s0
            amp *= m
            if k % 8 == 0 and amp.max() < 2e-5:
                break
    if len(bufs) == 1:
        return bufs[0]
    out = np.zeros(n)
    for j, buf in enumerate(bufs):
        out += buf * (1.0 + wobble * _wander(n, child(r, 'life', j), 2.5 + 1.7 * j))
    return out


def _unit(gains, slope, f0, top):
    """1 / the RMS a level-1 tone has at f0 (so a line's level reads as its loudness, whatever the instrument)."""
    m = 10.0 ** (-slope * f0 / 20000.0)
    k = np.arange(1, len(gains) + 1)
    a = np.asarray(gains) * m ** (k - 1) * (k * f0 < top)
    return 1.0 / np.sqrt(0.5 * np.sum(a * a) + 1e-12)


_FIRS = {}


def _fir(key, sections, taps=16384):
    """A chain of biquads as its impulse response (cached by key): filtering a minute of a section through twenty
    body resonances by one FFT convolution costs a fraction of evaluating the chain on every bin of the minute."""
    h = _FIRS.get(key)
    if h is None:
        imp = np.zeros(taps)
        imp[0] = 1.0
        h = F.filt(imp, *sections, extend=False)
        k = taps // 8
        h[-k:] *= 0.5 + 0.5 * np.cos(np.pi * np.arange(1, k + 1) / k)
        _FIRS[key] = h
    return h


def _through(x, h):
    return F.convolve(x, h)[..., :x.shape[-1]]


def _pans(count, pan, width, r):
    """Seats for count players around pan, width wide, each a hair off its place."""
    if count == 1:
        return [pan]
    return [float(np.clip(pan + width * (2.0 * (i + 0.5) / count - 1.0) * 0.5 + r.normal(0, 0.03), -1.0, 1.0))
            for i in range(count)]


def _spread_detunes(count, cents, r):
    """Players' tuning offsets: spread evenly over +-cents, shuffled, each nudged."""
    d = [cents * (2.0 * (i + 0.5) / count - 1.0) + r.normal(0, cents * 0.25) for i in range(count)]
    r.shuffle(d)
    return d


def _check(kind, rng_, f):
    lo, hi = hz(rng_[0]), hz(rng_[1])
    if f < lo / 1.03 or f > hi * 1.03:
        print(f'  orchestra: {kind} asked for {f:.1f} Hz, outside {rng_[0]}-{rng_[1]}', flush=True)


def _as_notes(what, dur, vel, at, opts):
    """A section's notes: a Line's, or one note or a chord (list) held from at for dur."""
    if isinstance(what, Line):
        return [what.notes]
    chord = what if isinstance(what, (list, tuple)) else [what]
    return [[[at, dur, hz(nt), vel, dict(opts)]] for nt in chord]


def _window(lines, pre=0.12, post=0.6):
    s = min(ln[0][0] for ln in lines) - pre
    e = max(n[0] + n[1] for ln in lines for n in ln) + post
    return s, ns(e - s)


class Takes:
    """Round-robin takes of short, often repeated notes, rendered once per key within one recipe and reused (a call
    given takes= may pass None for r: the takes have their own streams): count
    takes per key, one picked at random each time, a little louder or softer. Real sample libraries keep 4-8 round
    robins for the same reason (no machine-gun repeats); an ostinato costs a few renders instead of hundreds."""

    def __init__(self, r, count=5):
        self.seed = int(r.integers(0, 2 ** 31))
        self.pick = child(r, 'pick')
        self.count = count
        self.cache = {}

    def get(self, key, make):
        """make(r) renders one take (a signal, or a (signal, offset) pair); key names what it is."""
        lst = self.cache.setdefault(key, [])
        i = int(self.pick.integers(0, self.count))
        while len(lst) <= i:
            lst.append(make(np.random.default_rng(seed_of(self.seed, key, len(lst)))))
        g = float(db2a(self.pick.normal(0.0, 0.7)))
        take = lst[i]
        if isinstance(take, tuple):
            return take[0] * g, take[1]
        return take * g


def _taken(takes, key, at, vel, make):
    """A section hit through takes (if given), rendered at the nearest velocity step and scaled to the exact one:
    make(r, vel) -> (signal, offset from at)."""
    if takes is None:
        return None
    q = max(0.1, round(vel * 10.0) / 10.0)
    x, off = takes.get(key + (q,), lambda rr: make(rr, q))
    return x * (vel / q) ** 1.2, at + off


def _hit(takes, key, vel, make):
    """A struck hit through takes (if given): rendered at the nearest velocity step (its brightness), scaled to the
    exact velocity; make(r, vel) renders one."""
    if takes is None:
        return None
    q = max(0.1, round(vel * 10.0) / 10.0)
    return takes.get(key + (q,), lambda rr: make(rr, q)) * (vel / q) ** 1.2


# --- Brass ----------------------------------------------------------------------------------------------------------

# slope: dB per kHz each harmonic loses against the one below at pp and at ff (the bore's brassiness opening with the
# level; the bell's radiation lifts the harmonics over the fundamental); eq: the bell and bore (the horn's bell faces
# away and its sound comes off the back wall, dark; the trumpet's formant near 1.3 kHz); breath: the breath noise
# under the tone (dB); puff: the tongue's release (Hz, dB); vib: depth (semitones) and rate on long notes only; blip:
# the attack's overshoot (it brightens the first 30 ms); blare: how far the bore's shock-wave steepening (cuivre) bites
# at fortissimo.
BRASS = {
    'horn': dict(range=('B1', 'F5'), ref='F3', gp=0.15, slope=(36.0, 2.6), top=7000.0, breath=-37.0,
                 puff=(900.0, -21.0), vib=(0.04, 4.8), attack=0.05, release=0.13, glide=0.05, blip=0.22, blare=0.3,
                 scoop=0.22, players=4, width=0.3,
                 eq=[F.hpn(40.0, 2), F.hp1(250.0), F.peak(400.0, 3.0, 0.9), F.peak(1100.0, 1.0, 1.2),
                     F.peak(3200.0, -3.0, 0.9), F.lp(4000.0, 0.6), F.highshelf(5500.0, -6.0)]),
    'trombone': dict(range=('E2', 'F5'), ref='Bb3', gp=0.15, slope=(27.0, 1.6), top=9000.0, breath=-35.0,
                     puff=(1300.0, -19.0), vib=(0.07, 5.0), attack=0.032, release=0.11, glide=0.045, blip=0.32,
                     blare=0.6, scoop=0.25, players=3, width=0.25,
                     eq=[F.hpn(45.0, 2), F.hp1(350.0), F.peak(550.0, 2.5, 1.0), F.peak(1250.0, 2.5, 1.1),
                         F.peak(3300.0, -3.0, 0.9), F.lp(5500.0, 0.6), F.highshelf(6500.0, -5.0)]),
    'trumpet': dict(range=('F#3', 'D6'), ref='C5', gp=0.15, slope=(18.0, 1.3), top=11000.0, breath=-33.0,
                    puff=(1800.0, -18.0), vib=(0.11, 5.6), attack=0.022, release=0.09, glide=0.035, blip=0.4,
                    blare=0.45, scoop=0.3, players=3, width=0.22,
                    eq=[F.hpn(130.0, 2), F.hp1(700.0), F.peak(1300.0, 3.0, 0.9), F.peak(3400.0, -3.5, 0.9),
                        F.lp(7500.0, 0.6), F.highshelf(8000.0, -6.0)]),
    'tuba': dict(range=('D1', 'F4'), ref='F2', gp=0.15, slope=(48.0, 5.5), top=5000.0, breath=-38.0,
                 puff=(500.0, -22.0), vib=(0.04, 4.5), attack=0.065, release=0.15, glide=0.06, blip=0.2, blare=0.2,
                 scoop=0.2, players=1, width=0.1,
                 eq=[F.hpn(26.0, 2), F.hp1(120.0), F.peak(230.0, 4.0, 0.9), F.lp(1800.0, 0.6),
                     F.highshelf(2600.0, -8.0)]),
}

ARTS = {
    'normal': ({}, 1.0), 'stac': ({'release': 0.07}, 0.0), 'marc': ({'dyn': 'marc'}, 1.0),
    'sfz': ({'dyn': 'sfz'}, 1.0), 'swell': ({'dyn': 'swell'}, 1.0), 'cresc': ({'dyn': 'cresc'}, 1.0),
    'dim': ({'dyn': 'dim'}, 1.0), 'fp': ({'dyn': 'fp'}, 1.0),
}


def _art(art, dur):
    opts, keep = ARTS[art]
    return dict(opts), (dur if keep else min(dur, 0.2))


def _brass_voice(kind, notes, r, t0, n, cents=0.0, timing=0.0):
    """One brass player through notes into n samples from t0 (before the bell's EQ)."""
    P = BRASS[kind]
    for s, l, f, v, o in notes:
        _check(kind, P['range'], f)
    vd, vr = P['vib']
    f0, level, onsets = _contour(notes, r, t0, n, glide=P['glide'] * r.uniform(0.8, 1.25), attack=P['attack'],
                                 release=P['release'], vib=vd * r.uniform(0.7, 1.3),
                                 vib_rate=vr * r.uniform(0.92, 1.08), vib_delay=0.38 * r.uniform(0.8, 1.3),
                                 vib_min=0.85, scoop=P['scoop'] * r.uniform(0.5, 1.4), scoop_time=0.018,
                                 slur_dip=0.22, end_fall=0.12, blip=P['blip'], drift=2.5, jitter=1.2, cents=cents,
                                 timing=timing)
    lo, hi = P['slope']
    lv = np.clip(level, 0.0, 1.3)
    slope = lo + (hi - lo) * lv ** 0.75
    K = int(P['top'] / max(float(f0.min()), 20.0)) + 1
    gains = np.arange(1, min(K, 160) + 1) ** -P['gp']
    x = _harmonics(f0, level, slope, gains, child(r, 'life'), P['top'], wobble=0.07)
    x *= _unit(gains, hi, hz(P['ref']), P['top'])
    # Cuivre: at fortissimo the bore's air steepens into a shock front, so the loudest notes grow a hard edge.
    w = np.clip((lv - 0.62) / 0.4, 0.0, 1.0) ** 1.5 * P['blare']
    if w.max() > 0.01:
        loud = w > 0.3 * w.max()
        sat = D.drive(x / (np.max(np.abs(x)) + 1e-9), 9.0, 'tanh')
        sat *= np.sqrt(np.mean(x[loud] ** 2) / (np.mean(sat[loud] ** 2) + 1e-12))
        x = x + w * (sat - x)
    # Breath through the instrument, and the tongue's puff where a note is tongued.
    pf, pdb = P['puff']
    rn = child(r, 'breath')
    breath = N.shaped(n, rn, -1.5, max(300.0, pf * 0.6), pf * 2.8)
    breath *= lv * (0.8 + 0.2 * _wander(n, rn, 6.0)) * float(db2a(P['breath']))
    for s, v, joined, o in onsets:
        if joined and not o.get('tongue'):
            continue
        k = ns(0.016)
        at = ns(s - t0)
        if at + k > n or at < 0:
            continue
        puff = F.filt(N.white(k, rn) * E.perc(0.016, 0.0006, 0.012), F.bp(pf, 0.8), extend=False)
        breath[at:at + k] += puff / (np.std(puff) + 1e-9) * v * float(db2a(pdb)) * (0.5 if joined else 1.0) * 1.6
    return x + breath


def _brass_eq(kind):
    return _fir(('brass', kind), BRASS[kind]['eq'])


def brass(kind, note, dur, r, vel=0.7, art='normal'):
    """One brass player's note (kind: horn, trombone, trumpet, tuba), from its onset: art normal, stac (a short stab,
    whatever dur), marc, sfz, swell, cresc, dim, fp (forte-piano then a crescendo). Mono."""
    opts, length = _art(art, dur)
    notes = [[0.0, length, hz(note), vel, opts]]
    n = ns(length + 0.9)
    x = _brass_voice(kind, notes, r, 0.0, n)
    return _through(x, _brass_eq(kind))


def horn(note, dur, r, vel=0.7, art='normal'):
    """The French horn (B1-F5): round and dark, brassy only when pushed."""
    return brass('horn', note, dur, r, vel, art)


def trombone(note, dur, r, vel=0.7, art='normal'):
    """The trombone (E2-F5): warm at mf, a blaring edge at ff."""
    return brass('trombone', note, dur, r, vel, art)


def trumpet(note, dur, r, vel=0.7, art='normal'):
    """The trumpet (F#3-D6): bright and focused."""
    return brass('trumpet', note, dur, r, vel, art)


def tuba(note, dur, r, vel=0.7, art='normal'):
    """The tuba (D1-F4): the low floor."""
    return brass('tuba', note, dur, r, vel, art)


def brass_line(kind, ln, r):
    """One brass player through a Line (lip slurs between slurred notes, tongued ones re-speak). (mono, t0)."""
    t0, n = _window([ln.notes], 0.06, BRASS[kind]['release'] + 0.8)
    x = _brass_voice(kind, ln.notes, r, t0, n)
    return _through(x, _brass_eq(kind)), t0


def brass_section(kind, what, r, dur=1.0, vel=0.7, art='normal', at=0.0, players=None, pan=None, width=None,
                  detune=5.0, timing=0.007, takes=None):
    """A brass section: what is a note (players in unison), a chord (a list: players shared among its notes, at
    least one each) or a Line (players in unison, legato). art as brass(). (stereo, t0)."""
    P = BRASS[kind]
    players = players or P['players']
    pan = SEATS[kind] if pan is None else pan
    width = P['width'] if width is None else width
    if takes is not None and not isinstance(what, Line):
        key = ('brass', kind, str(what), round(dur, 3), art, players, round(pan, 2))
        return _taken(takes, key, at, vel, lambda rr, q: _brass_take(kind, what, rr, dur, q, art, players, pan,
                                                                      width, detune, timing))
    return _brass_take(kind, what, r, dur, vel, art, players, pan, width, detune, timing, at)


def _brass_take(kind, what, r, dur, vel, art, players, pan, width, detune, timing, at=0.0):
    P = BRASS[kind]
    opts, length = _art(art, dur)
    lines = _as_notes(what, length, vel, at, opts)
    per = max(1, int(round(players / len(lines))))
    t0, n = _window(lines, 0.06, P['release'] + 0.8)
    count = per * len(lines)
    pans = _pans(count, pan, width, child(r, 'pans'))
    cents = _spread_detunes(count, detune, child(r, 'cents'))
    out = np.zeros((2, n))
    j = 0
    for li, notes in enumerate(lines):
        for p in range(per):
            x = _brass_voice(kind, notes, child(r, 'player', li, p), t0, n, cents[j], timing)
            out += S.pan(x, pans[j]) / np.sqrt(2.0)
            j += 1
    out = _through(out, _brass_eq(kind)) / np.sqrt(per)
    return out, t0


# --- Strings --------------------------------------------------------------------------------------------------------

# slope: dB per kHz at pp and ff (more bow weight, more top); beta: bow positions (fractions of the string); vib:
# depth (semitones) and rate; hiss: the bow's noise band; air, wood: the body's main resonances; pizz: a pizzicato's
# ring at ref.
STRINGS = {
    'violins': dict(range=('G3', 'E7'), ref='A4', slope=(10.0, 0.4), gk=1.0, top=10500.0, beta=(0.085, 0.13),
                    vib=(0.17, 5.9), vib_delay=0.17, glide=0.035, attack=0.09, release=0.18, noise=-31.0,
                    hiss=2600.0, air=275.0, wood=470.0, bridge=2700.0, pizz=0.55, players=8, width=0.36),
    'violas': dict(range=('C3', 'E6'), ref='D4', slope=(11.0, 0.5), gk=0.95, top=9000.0, beta=(0.09, 0.135),
                   vib=(0.15, 5.6), vib_delay=0.18, glide=0.04, attack=0.1, release=0.2, noise=-31.0, hiss=2200.0,
                   air=230.0, wood=390.0, bridge=2200.0, pizz=0.75, players=6, width=0.3),
    'cellos': dict(range=('C2', 'A5'), ref='G3', slope=(12.0, 0.6), gk=0.88, top=8000.0, beta=(0.09, 0.14),
                   vib=(0.14, 5.3), vib_delay=0.2, glide=0.045, attack=0.11, release=0.24, noise=-31.0, hiss=1900.0,
                   air=105.0, wood=210.0, bridge=1900.0, pizz=1.3, players=6, width=0.3),
    'basses': dict(range=('C1', 'G4'), ref='D2', slope=(15.0, 1.0), gk=0.85, top=6000.0, beta=(0.1, 0.15),
                   vib=(0.08, 5.0), vib_delay=0.25, glide=0.055, attack=0.13, release=0.3, noise=-32.0, hiss=1400.0,
                   air=62.0, wood=115.0, bridge=1300.0, pizz=1.9, players=4, width=0.22),
}


def _string_body(section):
    """A section's body: the air and wood modes, the bridge's hill, and a dozen narrower resonances scattered as in
    real wood (the same every render: their seed is the section's), so a vibrato sweeps harmonics through peaks and
    dips and the tone shimmers as a real string's does."""
    P = STRINGS[section]
    rb = np.random.default_rng(seed_of('orchestra', section, 'body'))
    secs = [F.hpn(P['air'] * 0.55, 2), F.peak(P['air'], 3.0, 2.5), F.peak(P['wood'], 2.0, 2.0),
            F.peak(P['bridge'], 4.0, 1.2), F.peak(3500.0, -2.5, 1.0)]
    for f in np.exp(rb.uniform(np.log(P['air'] * 1.4), np.log(5200.0), 14)):
        secs.append(F.peak(float(f), float(np.clip(rb.normal(0.0, 3.5), -6.0, 6.0)), float(rb.uniform(5.0, 13.0))))
    secs += [F.highshelf(5500.0, -6.0), F.lp(9500.0, 0.6)]
    # The scattered peaks must not tilt the body: whatever octave-wide slope they add is taken back out.
    h = _fir(('strings-raw', section), secs)
    H = np.abs(np.fft.rfft(h, 65536))
    fr = np.fft.rfftfreq(65536, 1.0 / SR)
    fix = []
    for c in (250.0, 500.0, 1000.0, 2000.0):
        band = (fr > c / 1.41) & (fr < c * 1.41)
        high = {250.0: 1.0, 500.0: 0.0, 1000.0: -0.5, 2000.0: 2.0}
        low = {250.0: 1.0, 500.0: -0.5, 1000.0: -1.0, 2000.0: 1.0}
        want = (high if P['air'] > 150 else low)[c]
        have = 10.0 * np.log10(np.mean(H[band] ** 2) + 1e-12)
        fix.append(F.peak(c, float(np.clip(want - have, -8.0, 8.0)), 1.0))
    return _fir(('strings', section), secs + fix)


def _string_voice(section, notes, r, t0, n, cents=0.0, timing=0.0, tremolo=0.0, pont=0.0, port=0.25,
                  attack=None, release=None, scratch=-24.0, vib=1.0):
    """One string player through notes into n samples from t0 (before the body): the bowed harmonics (the bow
    position's comb, brightness from the bow's weight), the bow's hiss, a scratch where a bow change or an accent
    starts a note; tremolo shakes the bow rate strokes a second; pont (0..1) plays near the bridge (glassy)."""
    P = STRINGS[section]
    for s, l, f, v, o in notes:
        _check(section, P['range'], f)
    vd, vr = P['vib']
    f0, level, onsets = _contour(notes, r, t0, n, glide=P['glide'] * r.uniform(0.7, 1.6),
                                 attack=attack or P['attack'], release=release or P['release'],
                                 vib=vd * vib * r.uniform(0.75, 1.3), vib_rate=vr * r.uniform(0.92, 1.08),
                                 vib_delay=P['vib_delay'] * r.uniform(0.7, 1.4), vib_min=0.2, slur_dip=0.07,
                                 drift=3.5, jitter=1.6, cents=cents, timing=timing, port=port * r.uniform(0.0, 2.0))
    if tremolo:
        tt = times(n)
        stroke = tremolo * r.uniform(0.9, 1.1) * (1.0 + 0.06 * _wander(n, child(r, 'trate'), 1.5))
        bump = np.abs(np.sin(np.pi * np.cumsum(stroke) / SR + r.random() * np.pi)) ** 0.55
        level = level * (0.22 + 0.78 * bump * (1.0 + 0.12 * _wander(n, child(r, 'tj'), 9.0)))
    lo, hi = P['slope']
    lv = np.clip(level, 0.0, 1.3)
    slope = (lo + (hi - lo) * lv ** 0.8) * (1.0 - 0.45 * pont)
    K = int(P['top'] / max(float(f0.min()), 20.0)) + 1
    k = np.arange(1, min(K, 140) + 1)
    beta = r.uniform(*P['beta'])
    gains = k ** -P['gk'] * (0.25 + 0.75 * np.abs(np.sin(np.pi * k * beta)))
    if pont:
        gains = gains * k ** (0.85 * pont)
        gains[0] *= 1.0 - 0.75 * pont
        if gains.size > 1:
            gains[1] *= 1.0 - 0.4 * pont
    x = _harmonics(f0, level, slope, gains, child(r, 'life'), P['top'], wobble=0.13)
    x *= _unit((1.0 / k) * 0.6, hi, hz(P['ref']), P['top'])
    rn = child(r, 'bow')
    hiss = N.shaped(n, rn, -1.5, max(600.0, P['hiss'] * 0.45), P['hiss'] * 2.2)
    hiss *= lv * (0.75 + 0.25 * _wander(n, rn, 9.0)) * float(db2a(P['noise'] + 9.0 * pont))
    for s, v, joined, o in onsets:
        if joined:
            continue
        k2 = ns(0.04)
        at = ns(s - t0)
        if at < 0 or at + k2 > n:
            continue
        sc = F.filt(N.white(k2, rn) * E.perc(0.04, 0.001, 0.03), F.bp(1300.0 * (1.0 + pont), 0.7), extend=False)
        loud = scratch + (8.0 if o.get('dyn') in ('marc', 'sfz') or o.get('accent') else 0.0)
        hiss[at:at + k2] += sc / (np.std(sc) + 1e-9) * v * float(db2a(loud))
    return x + hiss


def _strings_out(section, lines, r, players, pan, width, detune, timing, **kw):
    P = STRINGS[section]
    players = players or P['players']
    pan = SEATS[section] if pan is None else pan
    width = P['width'] if width is None else width
    t0, n = _window(lines, 0.1, (kw.get('release') or P['release']) + 0.7)
    per = max(1, int(round(players / len(lines)))) if len(lines) > 1 else players
    count = per * len(lines)
    pans = _pans(count, pan, width, child(r, 'pans'))
    cents = _spread_detunes(count, detune, child(r, 'cents'))
    out = np.zeros((2, n))
    j = 0
    for li, notes in enumerate(lines):
        for p in range(per):
            x = _string_voice(section, notes, child(r, 'player', li, p), t0, n, cents[j], timing, **kw)
            out += S.pan(x, pans[j]) / np.sqrt(2.0)
            j += 1
    return _through(out, _string_body(section)) / np.sqrt(per), t0


def string_legato(section, ln, r, players=None, pan=None, width=None, pont=0.0, port=0.15, detune=6.0, vib=1.0):
    """A section (violins, violas, cellos, basses) through a Line: long bows, slurred notes gliding (port: how much
    the players slide on leaps; each slides a little differently), unslurred ones a bow change. (stereo, t0)."""
    return _strings_out(section, [ln.notes], r, players, pan, width, detune, 0.012, pont=pont, port=port, vib=vib)


def string_sustain(section, notes, dur, r, vel=0.6, at=0.0, players=None, dyn='flat', attack=0.35, release=0.6,
                   pont=0.0, pan=None, width=None, detune=6.0, vib=1.0):
    """Held notes or a chord (a list: players shared among its notes), long bows from at for dur; dyn shapes the
    bow (flat, swell, cresc, dim, fp). (stereo, t0)."""
    lines = _as_notes(notes, dur, vel, at, {'dyn': dyn})
    return _strings_out(section, lines, r, players, pan, width, detune, 0.015, pont=pont, attack=attack,
                        release=release, vib=vib)


def string_marcato(section, notes, dur, r, vel=0.8, at=0.0, players=None, pan=None, width=None, takes=None):
    """Accented full bows: a bite and a scratch, then the stroke easing. (stereo, t0)."""
    def make(rr, v, at_=0.0):
        lines = _as_notes(notes, dur, v, at_, {'dyn': 'marc'})
        return _strings_out(section, lines, rr, players, pan, width, 6.0, 0.006, attack=0.012, release=0.16,
                            scratch=-20.0, vib=0.6)
    hit = _taken(takes, ('marc', section, str(notes), round(dur, 3), players), at, vel, make)
    return hit if hit else make(r, vel, at)


def string_spiccato(section, notes, r, vel=0.7, at=0.0, players=None, length=0.11, pan=None, width=None, takes=None):
    """A short bounced stroke (the ostinato's): a quick bite, the bow's scratch, a short ring. (stereo, t0)."""
    def make(rr, v, at_=0.0):
        lines = _as_notes(notes, length, v, at_, {'dyn': 'marc'})
        return _strings_out(section, lines, rr, players, pan, width, 7.0, 0.005, attack=0.006, release=0.08,
                            scratch=-17.0, vib=0.0)
    hit = _taken(takes, ('spic', section, str(notes), players, round(length, 3)), at, vel, make)
    return hit if hit else make(r, vel, at)


def string_tremolo(section, notes, dur, r, vel=0.6, at=0.0, players=None, rate=13.0, dyn='cresc', pont=0.0,
                   pan=None, width=None, attack=0.25, release=0.3):
    """Bowed tremolo: every player's bow shaking rate strokes a second, out of step with the others (the shimmer of
    tension); pont 0..1 moves the bows to the bridge (glassy, eerie). (stereo, t0)."""
    lines = _as_notes(notes, dur, vel, at, {'dyn': dyn})
    return _strings_out(section, lines, r, players, pan, width, 8.0, 0.01, tremolo=rate, pont=pont, attack=attack,
                        release=release, scratch=-30.0, vib=0.5)


def string_shriek(dur, r, vel=0.8, at=0.0, low='E6', high='A6', players=10, rise=2.0, rate=15.0, pan=-0.3,
                  width=0.8):
    """The high clawing cluster: the violins packed between low and high in quarter tones, sul ponticello tremolo,
    a nervous fast vibrato, the whole cluster climbing rise semitones as it swells. (stereo, t0)."""
    fl, fh = hz(low), hz(high)
    lines = []
    rr = child(r, 'cluster')
    for i in range(players):
        f = fl * (fh / fl) ** (i / max(players - 1, 1)) * 2.0 ** (rr.normal(0, 0.12) / 12.0)
        lines.append([[at, dur, f, vel * rr.uniform(0.8, 1.0), {'dyn': 'cresc', 'bend': -rise * rr.uniform(0.7, 1.3),
                                                                 'vib': 0.3 * rr.uniform(0.6, 1.4)}]])
    sig, t0 = _strings_out('violins', lines, r, players, pan, width, 10.0, 0.02, tremolo=rate, pont=0.85,
                           attack=0.4, release=0.25, scratch=-26.0, vib=1.0)
    return sig, t0


def _pizz_voice(section, f, r, vel):
    """One plucked string: partials from the pluck's place over the fingerboard, the high ones dying first, two
    polarizations decaying apart, a little sharp as the pluck lets go, the fingertip's soft thump."""
    P = STRINGS[section]
    _check(section, P['range'], f)
    t60 = P['pizz'] * (hz(P['ref']) / f) ** 0.35
    dur = min(t60 * 1.6, 3.2)
    n = ns(dur)
    t = times(n)
    warp = t + 0.0025 * vel * 0.05 * (1.0 - np.exp(-t / 0.05))
    p = r.uniform(0.17, 0.26)
    out = np.zeros(n)
    k = 1
    while k < 80:
        fk = k * f * np.sqrt(1.0 + 2e-5 * k * k)
        if fk > 9000.0:
            break
        a = abs(np.sin(np.pi * k * p)) / k ** (1.7 - 0.6 * vel)
        tk = t60 / (1.0 + 0.35 * (k - 1) ** 1.2) / (1.0 + (fk / 2300.0) ** 2)
        m = min(n, ns(tk * 2.2))
        split = 1.0 + 0.0006 * (r.random() - 0.5)
        ph = 2 * np.pi * r.random() * 0.15
        out[:m] += a * (0.6 * np.exp(-LN1000 * t[:m] / tk) * np.sin(2 * np.pi * fk * warp[:m] + ph) +
                        0.4 * np.exp(-LN1000 * t[:m] / (tk * 1.8)) * np.sin(2 * np.pi * fk * split * warp[:m] + ph))
        k += 1
    B._rise(out, 0.0016)
    thump = F.filt(N.white(ns(0.008), r) * E.decay(ns(0.008), 0.004), F.lp(900.0), extend=False)
    out[:thump.size] += thump * 0.12 * np.max(np.abs(out)) / (np.max(np.abs(thump)) + 1e-9)
    return E.end_fade(normalize(out, 0.0), 0.02) * vel ** 1.2


def _col_legno_voice(section, f, r, vel):
    """The bow's wood struck on the string: a dry knock of the stick, the string ringing its pitch for a moment, the
    stick bouncing once."""
    n = ns(0.25)
    t = times(n)
    tone = np.zeros(n)
    for k in range(1, 9):
        if k * f > 8000:
            break
        tk = 0.09 / (1.0 + 0.45 * (k - 1))
        tone += (1.0 / k ** 1.1) * np.exp(-LN1000 * t / tk) * np.sin(2 * np.pi * k * f * t + 0.3 * r.random())
    stick = M.strike(M.wood(1500.0 * r.uniform(0.85, 1.15), r, t60=0.03, count=6), M.hammer(0.00012 + 0.00012 *
                     (1 - vel)), 0.06)
    x = layers((normalize(tone), 0.0, -3.0), (normalize(stick), 0.0, 0.0),
               (normalize(stick), r.uniform(0.018, 0.032), -11.0))
    return E.end_fade(fit(normalize(x, 0.0), n), 0.02) * vel ** 1.2


def _plucked_section(kind, section, notes, r, vel, at, players, pan, width, takes):
    P = STRINGS[section]
    players = players or max(2, P['players'] // (2 if kind == 'pizz' else 1))
    pan = SEATS[section] if pan is None else pan
    width = P['width'] if width is None else width
    chord = notes if isinstance(notes, (list, tuple)) else [notes]

    def make(rr, v, at_=0.0):
        per = max(1, int(round(players / len(chord))))
        pans = _pans(per * len(chord), pan, width, child(rr, 'pans'))
        voices = []
        j = 0
        for nt in chord:
            f = hz(nt)
            for p in range(per):
                rp = child(rr, kind, nt, p)
                dt = abs(rp.normal(0.0, 0.009 if kind == 'pizz' else 0.012))
                ff = f * 2.0 ** (rp.normal(0, 4.0) / 1200.0)
                x = _pizz_voice(section, ff, rp, lively(rp, v, 0.12)) if kind == 'pizz' else \
                    _col_legno_voice(section, ff, rp, lively(rp, v, 0.15))
                voices.append((S.pan(x, pans[j]) / np.sqrt(2.0), dt))
                j += 1
        m = max(v.shape[-1] + ns(dt) for v, dt in voices)
        out = np.zeros((2, m))
        for v, dt in voices:
            out[:, ns(dt):ns(dt) + v.shape[-1]] += v
        return _through(out, _string_body(section)) / np.sqrt(per), at_ - 0.002

    hit = _taken(takes, (kind, section, str(notes), players, round(pan, 2)), at, vel, make)
    return hit if hit else make(r, vel, at)


def string_pizz(section, notes, r, vel=0.7, at=0.0, players=None, pan=None, width=None, takes=None):
    """Pizzicato: each player's pluck a few ms apart (half the section plays by default). (stereo, t0)."""
    return _plucked_section('pizz', section, notes, r, vel, at, players, pan, width, takes)


def string_col_legno(section, notes, r, vel=0.7, at=0.0, players=None, pan=None, width=None, takes=None):
    """Col legno battuto: the section striking with the bow's wood, a dry many-legged clatter. (stereo, t0)."""
    return _plucked_section('legno', section, notes, r, vel, at, players, pan, width, takes)


# --- Choir ----------------------------------------------------------------------------------------------------------

# Sung (covered, choral) vowels: formants and bandwidths (Hz), and a low-pass for the closed mouth ('mm' sounds
# through the nose: almost nothing over a kilohertz). Physics of the vocal tract, averaged; nobody's recording.
CHOIR_VOWELS = {
    'ah': ([700.0, 1120.0, 2550.0, 3300.0, 4100.0], [100.0, 120.0, 160.0, 250.0, 300.0], None),
    'oh': ([470.0, 800.0, 2500.0, 3300.0, 4100.0], [90.0, 100.0, 150.0, 250.0, 300.0], None),
    'oo': ([340.0, 760.0, 2400.0, 3300.0, 4100.0], [70.0, 90.0, 150.0, 250.0, 300.0], None),
    'ee': ([300.0, 2050.0, 2800.0, 3500.0, 4200.0], [70.0, 120.0, 160.0, 250.0, 300.0], None),
    'er': ([490.0, 1350.0, 1690.0, 3300.0, 4100.0], [70.0, 100.0, 120.0, 250.0, 300.0], None),
    'mm': ([260.0, 1050.0, 2200.0, 3300.0, 4100.0], [60.0, 220.0, 380.0, 400.0, 450.0], 650.0),
    'h': ([700.0, 1200.0, 2500.0, 3500.0, 4500.0], [300.0, 300.0, 300.0, 350.0, 400.0], None),
}
# Parts: formant scale (the throat's size), range, breath share, the highest harmonic.
PARTS = {'bass': (0.93, ('E2', 'E4'), 0.16, 7000.0), 'tenor': (1.0, ('C3', 'A4'), 0.15, 8000.0),
         'alto': (1.1, ('F3', 'D5'), 0.2, 9000.0), 'soprano': (1.17, ('C4', 'B5'), 0.2, 9500.0)}
UPPER = V.UPPER


def _part(part, f):
    if part == 'men':
        return 'bass' if f < 155.0 else 'tenor'
    if part == 'women':
        return 'alto' if f < 430.0 else 'soprano'
    return part


def _vowel_keys(notes, t0, vowel):
    """(time, vowel) keys from notes ('hah' and 'hm' are a breathed 'h' then the vowel)."""
    keys = []
    for s, l, f, v, o in notes:
        vw = o.get('vowel', vowel)
        at = max(0.0, s - t0)
        if vw in ('hah', 'hm'):
            keys += [(at, 'h'), (at + 0.06, 'ah' if vw == 'hah' else 'mm')]
        else:
            keys.append((at, vw))
    keys.sort(key=lambda k: k[0])
    return keys


def _vowel_frames(keys, tc, shift, jit, f0c=None):
    """Formant tracks (8 x frames), bandwidths and the closed mouth's low-pass at frame times tc."""
    kt = np.array([k[0] for k in keys]) if keys else np.array([0.0])
    vs = [CHOIR_VOWELS[k[1]] for k in keys] if keys else [CHOIR_VOWELS['ah']]
    Fs = np.array([v[0] for v in vs]) * jit[None, :]
    Bs = np.array([v[1] for v in vs])
    lps = np.array([v[2] or 20000.0 for v in vs])
    Fc = [np.exp(np.interp(tc, kt, np.log(Fs[:, j]))) * shift for j in range(5)]
    if f0c is not None:
        # A high voice opens its jaw so the first formant rides over the note it sings.
        Fc[0] = np.maximum(Fc[0], 1.06 * f0c)
    Bc = [np.interp(tc, kt, Bs[:, j]) * shift for j in range(5)]
    Fc += [np.full(tc.size, f * shift) for f in UPPER[0]]
    Bc += [np.full(tc.size, b * shift) for b in UPPER[1]]
    lp = np.exp(np.interp(tc, kt, np.log(lps)))
    return np.stack(Fc), np.stack(Bc), lp


def _formant_filter(x, keys, shift, jit, f0=None, tilt=0.0, frame=1024, batch=160):
    """x through a vocal tract that moves along keys (time, vowel): short-time spectra, each multiplied by the tract's
    response at that moment, overlap-added (batched; a held vowel's response is worked out once a batch)."""
    n = x.size
    hop = frame // 4
    m = 2 * frame
    off = frame // 2
    win = 0.5 - 0.5 * np.cos(2 * np.pi * np.arange(frame) / frame)
    xp = np.concatenate([np.zeros(frame), x, np.zeros(2 * frame)])
    starts = np.arange(0, xp.size - frame + 1, hop)
    tc = (starts - frame + frame / 2) / SR
    f0c = None
    if f0 is not None:
        f0c = np.interp(tc, times(n), f0)
    Fc, Bc, lp = _vowel_frames(keys, tc, shift, jit, f0c)
    blocks = np.abs(xp[:(xp.size // hop) * hop]).reshape(-1, hop).max(axis=1)
    freqs = np.fft.rfftfreq(m, 1.0 / SR)
    tl = np.where(freqs > 400.0, (np.maximum(freqs, 1.0) / 400.0) ** (tilt / 6.0206), 1.0) if tilt else 1.0
    out = np.zeros(xp.size + m)
    live = []
    for j, s in enumerate(starts):
        b0 = s // hop
        if blocks[b0:b0 + 4].max() > 0:
            live.append(j)
    live = np.array(live, dtype=int)
    ar = np.arange(frame)
    for c in range(0, live.size, batch):
        sel = live[c:c + batch]
        idx = starts[sel]
        segs = xp[idx[:, None] + ar[None, :]] * win
        buf = np.zeros((sel.size, m))
        buf[:, off:off + frame] = segs
        Sx = np.fft.rfft(buf, axis=1)
        Fb, Bb, lb = Fc[:, sel], Bc[:, sel], lp[sel]
        if np.ptp(Fb, axis=1).max() < 0.5 and np.ptp(lb) < 0.5:
            Fb, Bb, lb = Fb[:, :1], Bb[:, :1], lb[:1]
        g = V.formant_gain(freqs[None, :], Fb[:, :, None], Bb[:, :, None])
        g = g * (1.0 + (freqs[None, :] / lb[:, None]) ** 2) ** -1.5
        y = np.fft.irfft(Sx * (g * tl), m, axis=1)
        for k, s in enumerate(idx):
            out[s:s + m] += y[k]
    return out[off + frame:off + frame + n] / 2.0


def _singer(part, notes, r, t0, n, vowel, breath, cents, timing):
    """One singer's source (before the tract): the glottal harmonics, brighter as the voice presses harder, with
    breath puffing at each cycle; the 'h' of a breathed syllable is breath alone."""
    shift, rng_, br, top = PARTS[part]
    for s, l, f, v, o in notes:
        _check(part, rng_, f)
    f0, level, onsets = _contour(notes, r, t0, n, glide=0.075 * r.uniform(0.7, 1.4), attack=0.16 * r.uniform(0.7, 1.4),
                                 release=0.32, vib=0.2 * r.uniform(0.55, 1.4), vib_rate=5.3 * r.uniform(0.87, 1.12),
                                 vib_delay=0.35 * r.uniform(0.6, 1.5), vib_min=0.45, scoop=0.22 * r.uniform(0.3, 1.3),
                                 scoop_time=0.055, slur_dip=0.12, end_fall=0.3, drift=6.0, jitter=5.0, cents=cents,
                                 timing=timing)
    gate = np.ones(n)
    puff = np.zeros(n)
    for s, v, joined, o in onsets:
        if o.get('vowel', vowel) in ('hah', 'hm'):
            a = max(0, ns(s - t0))
            k = min(ns(0.075), n - a)
            if k <= 0:
                continue
            u = np.arange(k) / SR
            gate[a:a + k] *= smoothstep((u - 0.045) / 0.03)
            puff[a:a + k] += v * np.sin(np.pi * np.clip(u / 0.075, 0, 1)) ** 0.7
    lv = np.clip(level, 0.0, 1.3)
    k = np.arange(1, int(top / max(float(f0.min()), 40.0)) + 2)
    gains = 1.0 / k
    slope = np.maximum(10.0 - 10.5 * lv ** 0.8, 0.0)
    voiced = _harmonics(f0, level * gate, slope, gains, child(r, 'life'), top, wobble=0.1)
    voiced *= _unit(gains, 1.5, 220.0, top) * (1.0 + 0.05 * _wander(n, child(r, 'shim'), 27.0))
    ph = 2 * np.pi * O.phase(f0, n)
    air = N.shaped(n, child(r, 'air'), 0.0, 350.0, 9000.0)
    share = np.clip(breath * br * (1.25 - 0.35 * lv), 0.02, 0.6)
    air = air * (0.55 + 0.45 * np.cos(ph)) * lv * share * 1.2 + air * puff * 1.2
    return voiced + air


def _choir_out(part, lines, r, vowel, voices, pan, width, breath, timing):
    t0, n = _window(lines, 0.15, 1.0)
    out = np.zeros((2, n))
    for li, notes in enumerate(lines):
        p = _part(part, notes[0][2])
        shift = PARTS[p][0]
        size = 2 if voices <= 8 else 3
        groups = int(np.ceil(voices / size))
        pans = _pans(groups, pan, width, child(r, 'pans', li))
        keys = _vowel_keys(notes, t0, vowel)
        for g in range(groups):
            rg = child(r, 'group', li, g)
            src = np.zeros(n)
            for s in range(min(size, voices - g * size)):
                rs = child(rg, 'singer', s)
                src += _singer(p, notes, rs, t0, n, vowel, breath * rs.uniform(0.7, 1.4), rs.normal(0, 7.0), timing)
            sh = shift * rg.uniform(0.96, 1.04)
            jit = rg.uniform(0.97, 1.03, 5)
            f0 = None
            if p in ('alto', 'soprano'):
                f0 = np.full(n, float(np.median([x[2] for x in notes])))
            y = _formant_filter(src, keys, sh, jit, f0)
            out += S.pan(y, pans[g]) / np.sqrt(2.0)
    # The tract's cascade peaks high: bring the section to a level that reads like the other instruments.
    out = F.filt(out, F.hp(70.0, 0.7), extend=False)
    return out * (1.6 / np.sqrt(voices * len(lines))), t0


def choir(part, notes, dur, r, vowel='ah', voices=8, vel=0.6, at=0.0, dyn='swell', pan=0.0, width=0.85, breath=1.0):
    """A held note or chord (a list) sung by voices singers a note (part: bass, tenor, alto, soprano, men, women;
    vowel: ah, oh, oo, ee, er, mm, hah, hm). dyn: flat, swell, cresc, dim, chant (a 'hah' hit). (stereo, t0)."""
    lines = _as_notes(notes, dur, vel, at, {'dyn': 'chant' if vowel in ('hah', 'hm') and dyn == 'swell' else dyn})
    return _choir_out(part, lines, r, vowel, voices, pan, width, breath, 0.012 if vowel in ('hah', 'hm') else 0.022)


def choir_line(part, ln, r, vowel='ah', voices=8, pan=0.0, width=0.85, breath=1.0):
    """A sung Line (notes may change vowel by their 'vowel' option); each singer scoops, slides and breathes on its
    own. (stereo, t0)."""
    return _choir_out(part, [ln.notes], r, vowel, voices, pan, width, breath, 0.02)


def whisper_choir(dur, r, voices=10, vel=0.5, at=0.0, rate=2.6, dyn='swell', pan=0.0, width=0.9, hiss=0.3):
    """Many voices whispering wordless syllables, out of step (no pitch): breath through moving vowels, a few
    hissed consonants. (stereo, t0)."""
    n = ns(dur + 0.6)
    t = times(n)
    total = _dyn(dyn, t - 0.05, dur) * np.clip((dur + 0.4 - t) / 0.4, 0.0, 1.0) * vel
    pans = _pans(voices, pan, width, child(r, 'pans'))
    out = np.zeros((2, n))
    vowels = ['ah', 'oh', 'ee', 'oo', 'er', 'h']
    for i in range(voices):
        rv = child(r, 'whisper', i)
        env = np.zeros(n)
        sib = np.zeros(n)
        keys = [(0.0, 'h')]
        tt = rv.uniform(0.0, 0.5)
        while tt < dur:
            length = rv.uniform(0.14, 0.42)
            a, b = ns(tt), min(n, ns(tt + length))
            u = np.arange(b - a) / SR
            env[a:b] += np.clip(u / 0.035, 0, 1) * np.clip((length - u) / (length * 0.6), 0, 1) * rv.uniform(0.5, 1.0)
            keys.append((tt, vowels[int(rv.integers(0, len(vowels)))]))
            if rv.random() < 0.35:
                k = min(ns(rv.uniform(0.04, 0.09)), n - a)
                sib[a:a + k] += np.sin(np.pi * np.arange(k) / max(k, 1)) * rv.uniform(0.4, 1.0)
            tt += length + rv.exponential(1.0 / rate)
        src = N.white(n, rv) * env
        y = _formant_filter(src, keys, rv.uniform(0.95, 1.2), rv.uniform(0.96, 1.04, 5), tilt=3.0)
        y /= np.sqrt(np.mean(y * y)) + 1e-9
        s = V.hiss(n, rv, centers=(2700.0, 4200.0, 6200.0), levels=(0.5, 0.8, 0.6)) * sib * hiss
        out += S.pan((y * env.max() + s) * total, pans[i]) / np.sqrt(2.0)
    out = F.filt(out, F.hp(180.0, 0.7), F.peak(3400.0, -3.0, 0.8), F.highshelf(7000.0, -5.0), extend=False)
    return out / np.sqrt(voices) * 0.2, at - 0.0


# --- Percussion -----------------------------------------------------------------------------------------------------

# The kettle-loaded membrane's modes against its principal: (ratio, strength, share of the ring). The kettle's air
# pulls the (n,1) modes near a harmonic series (1, 1.5, 2, 2.44, 2.9: the note the timpani is heard at); the (0,1)
# mode radiates its energy at once (the thump) and the (n,2) modes are short and out of tune.
TIMPANI = [(0.84, 0.55, 0.05), (1.0, 1.0, 1.0), (1.504, 0.7, 0.72), (1.71, 0.22, 0.06), (1.988, 0.5, 0.52),
           (2.2, 0.16, 0.18), (2.44, 0.34, 0.4), (2.78, 0.12, 0.14), (2.9, 0.22, 0.3), (3.33, 0.14, 0.22),
           (3.8, 0.08, 0.15)]


def _skin(f0, r, contact, dur=0.6, count=28, lo=4.0, hi=30.0, t60=0.14, level_db=-10.0):
    """A drum head's higher modes (a dense, short-lived cloud from lo to hi times its fundamental) struck by a contact
    of that many seconds: a soft or slow strike barely reaches them, a hard one rings them all. Scaled against a hard
    strike, so it carries the force's brightness (add it to the head's low modes, normalized at level_db)."""
    ratios = np.sort(r.uniform(lo, hi, count))
    m = M.modes(f0 * ratios, t60 * (lo / ratios) ** 0.6, (lo / ratios) ** 0.6 * r.uniform(0.5, 1.0, count))
    ring = M.ring(m, dur, r, random_phase=1.0)
    ref = np.max(np.abs(F.convolve(M.hammer(0.0002), ring[:ns(0.05)]))) + 1e-12
    return F.convolve(M.hammer(contact), ring)[:ring.size] / ref * float(db2a(level_db))


def _timp_ring(f, r, dur, muffle=0.0, glide=0.0):
    t60 = 3.6 * (73.4 / f) ** 0.5 * (1.0 - 0.75 * muffle)
    n = ns(dur)
    t = times(n)
    drop = 1.0 + glide * np.exp(-t / 0.08)
    out = np.zeros(n)
    for ratio, amp, share in TIMPANI:
        fk = f * ratio * (1.0 + 0.004 * (2 * r.random() - 1))
        tk = max(t60 * share, 0.03)
        m = min(n, ns(tk * 1.6))
        e = np.exp(-LN1000 * t[:m] / tk)
        ph = 2 * np.pi * O.phase(fk * drop[:m], m)
        # Each mode a pair a fraction of a hertz apart (the head's tension isn't even): the slow beat of a real drum.
        out[:m] += amp * e * (0.6 * np.sin(ph) + 0.4 * np.sin(ph * (1.0 + 0.25 / fk) + r.random()))
    return out


def timpani(note, r, vel=0.8, hard=0.4, muffle=0.0, dur=None, takes=None):
    """A timpano (D2-A3) struck by a felt mallet: harder (or louder) strikes are shorter contacts, so brighter; a
    loud one starts a little sharp as the head stretches. muffle 0..1 damps it. Mono."""
    hit = _hit(takes, ('timpani', str(note), hard, muffle, dur), vel,
               lambda rr, q: timpani(note, rr, q, hard, muffle, dur))
    if hit is not None:
        return hit
    f = hz(note)
    _check('timpani', ('C2', 'C4'), f)
    t60 = 3.6 * (73.4 / f) ** 0.5
    dur = dur or min(t60 * 1.25 + 0.3, 6.0)
    ring = _timp_ring(f, r, dur, muffle, 0.012 * vel)
    contact = (0.0056 - 0.0042 * vel) * (1.35 - 0.7 * hard)
    x = F.convolve(M.hammer(contact), ring)[:ring.size]
    x = x / (np.max(np.abs(x)) + 1e-12)
    sk = _skin(f, child(r, 'skin'), contact, 0.6, level_db=-9.0)
    x[:sk.size] += sk
    felt = F.filt(N.white(ns(0.012), r) * E.decay(ns(0.012), 0.005), F.lp(600.0 + 1400.0 * vel), extend=False)
    x[:felt.size] += felt * 0.05 * (0.4 + vel) / (np.max(np.abs(felt)) + 1e-9)
    return E.end_fade(normalize(B._rise(x, 0.0008), 0.0), 0.05) * vel ** 1.25


def timpani_roll(note, dur, r, vel=0.4, to=0.8, rate=14.0, hit=True, hard=0.3, muffle=0.0):
    """A timpani roll from vel to to over dur (two mallets alternating rate strokes a second, never quite even),
    with a last stroke on the beat at dur when hit (a roll into a note). Mono, starting at the roll."""
    f = hz(note)
    t60 = 3.6 * (73.4 / f) ** 0.5
    total = dur + min(t60 * 1.2, 5.0)
    n = ns(total)
    ring = _timp_ring(f, r, min(t60 * 1.3, 5.0), muffle)
    exc = np.zeros(n)
    t = 0.0
    side = 0
    while t < dur - 0.02:
        u = t / dur
        v = (vel + (to - vel) * u ** 1.3) * r.uniform(0.85, 1.1) * (1.0 if side else 0.9)
        h = M.hammer((0.0046 - 0.0024 * v) * (1.35 - 0.7 * hard))
        a = ns(t)
        exc[a:a + h.size] += h * v
        t += (1.0 / rate) * r.uniform(0.86, 1.14)
        side ^= 1
    if hit:
        h = M.hammer((0.004 - 0.0026 * to) * (1.35 - 0.7 * hard))
        exc[ns(dur):ns(dur) + h.size] += h * min(1.0, to * 1.15)
    x = F.convolve(exc, ring)[:n]
    # One full stroke's peak sets the scale, so a roll sits with single hits on the same track.
    ref = np.max(np.abs(F.convolve(M.hammer(0.0018), ring[:ns(0.1)])))
    return E.end_fade(x / (ref + 1e-9) * 0.9, 0.05)


def taiko(r, vel=0.9, size=1.0, takes=None):
    """A taiko or war drum (size 1: an o-daiko near 68 Hz; 0.6 a smaller drum) struck by a wooden bachi: the stiff
    hide's modes dropping in pitch as it relaxes, the far head answering, the barrel's wood, the stick's slap and the
    air it shoves. Mono."""
    hit = _hit(takes, ('taiko', size), vel, lambda rr, q: taiko(rr, q, size))
    if hit is not None:
        return hit
    f0 = 68.0 / size * r.uniform(0.97, 1.03)
    contact = 0.0028 - 0.0022 * vel
    head = O.membrane(f0, 1.8 * size, r, t60=1.15 * size, bend=0.28, bend_time=0.04)
    back = O.membrane(f0 * 1.045, 1.6 * size, child(r, 'back'), t60=0.9 * size, bend=0.15, bend_time=0.05)
    shell = M.strike(M.wood(260.0 / size, child(r, 'shell'), t60=0.12, count=6), M.hammer(contact), 0.25)
    slap = F.filt(N.white(ns(0.01), child(r, 'slap')) * E.decay(ns(0.01), 0.004), F.bp(1300.0, 0.7), extend=False)
    air = kit.noise_thump(0.7, child(r, 'air'), 260.0 + 420.0 * vel, 60.0, 0.35, 1.0, 0.035)
    head = normalize(head) + fit(_skin(f0, child(r, 'skin'), contact, 0.5, 30, 4.5, 40.0, 0.09, -8.0), head.size)
    x = layers((normalize(head), 0.0, 0.0), (normalize(back), 0.003, -8.0),
               (normalize(shell), 0.0, -18.0 + 6.0 * vel),
               (normalize(slap), 0.0, -31.0 + 17.0 * vel), (air, 0.0, -6.0))
    x = D.drive(normalize(B._rise(x, 0.0006), 0.0), 1.0 + 4.5 * vel, 'tanh')
    return E.end_fade(normalize(x, 0.0), 0.05) * vel ** 1.2


def taiko_rim(r, vel=0.7, takes=None):
    """The bachi on the taiko's rim (ka): a hard wooden click with the barrel behind it. Mono."""
    hit = _hit(takes, ('rim',), vel, lambda rr, q: taiko_rim(rr, q))
    if hit is not None:
        return hit
    k = M.strike(M.wood(1100.0 * r.uniform(0.95, 1.05), r, t60=0.05, count=6), M.hammer(0.00015), 0.12)
    body = M.strike(M.wood(330.0, child(r, 'b'), t60=0.08, count=5), M.hammer(0.0006), 0.15)
    x = layers((normalize(k), 0.0, 0.0), (normalize(body), 0.0, -9.0))
    return E.end_fade(normalize(x, 0.0), 0.02) * vel


def bass_drum(r, vel=0.8, dur=3.2, takes=None):
    """The orchestral bass drum (gran cassa, ~44 Hz) struck by a soft beater: a long dark boom and the air it moves.
    Mono."""
    hit = _hit(takes, ('bass_drum', dur), vel, lambda rr, q: bass_drum(rr, q, dur))
    if hit is not None:
        return hit
    f0 = 44.0 * r.uniform(0.98, 1.02)
    head = O.membrane(f0, dur, r, t60=2.4, bend=0.05, bend_time=0.06)
    exc = M.hammer(0.0068 - 0.0045 * vel)
    head = F.convolve(exc, head)[:head.size]
    head = normalize(head) + fit(_skin(f0, child(r, 'skin'), 0.0068 - 0.0045 * vel, 0.5, 26, 4.0, 30.0, 0.12, -6.0),
                                 head.size)
    air = kit.noise_thump(1.2, child(r, 'air'), 140.0 + 300.0 * vel, 45.0, 0.8, 0.9, 0.06)
    x = layers((normalize(head), 0.0, 0.0), (air, 0.0, -5.0))
    x = F.filt(x, F.lp(900.0 + 900.0 * vel), extend=False)
    return E.end_fade(normalize(B._rise(x, 0.002), 0.0), 0.08) * vel ** 1.2


def bass_drum_roll(dur, r, vel=0.3, to=0.8):
    """A bass drum roll: two soft beaters rumbling from vel to to over dur, the head never stopping. Mono."""
    n = ns(dur + 2.5)
    ring = O.membrane(44.0, 2.6, r, t60=2.2, bend=0.02, bend_time=0.05)
    exc = np.zeros(n)
    t = 0.0
    while t < dur:
        v = (vel + (to - vel) * (t / dur) ** 1.4) * r.uniform(0.8, 1.15)
        h = M.hammer(0.007 - 0.003 * v)
        exc[ns(t):ns(t) + h.size] += h * v
        t += r.uniform(0.055, 0.08)
    x = F.convolve(exc, ring)[:n]
    ref = np.max(np.abs(F.convolve(M.hammer(0.004), ring[:ns(0.2)])))
    x = F.filt(x, F.lp(700.0), extend=False)
    return E.end_fade(x / (ref + 1e-9) * 0.8, 0.1)


def _cymbal_modes(r, lo=330.0, hi=11000.0, count=120, t60=4.5):
    f = np.sort(np.exp(r.uniform(np.log(lo), np.log(hi), count)))
    d = t60 * (lo / f) ** 0.32
    a = (lo / f) ** 0.25 * (0.4 + 0.6 * r.random(count))
    return M.modes(f, d, a)


def cymbal_crash(r, vel=0.8, dur=5.0):
    """A suspended cymbal (or a pair of plates) crashed: a bright splash of dense, inharmonic modes and noise, the
    top dying first into a long shimmer; a harder crash is brighter. Mono."""
    ring = M.ring(_cymbal_modes(child(r, 'modes')), dur, child(r, 'ph'), random_phase=1.0)
    x = F.convolve(M.hammer(0.0005 - 0.00025 * vel), ring)[:ring.size]
    n = x.size
    t = times(n)
    wash = N.pink(n, child(r, 'wash'), lo=1200.0)
    wash = F.filt(wash, F.lp(5000.0 + 7000.0 * vel, 0.6), extend=False)
    wash *= np.exp(-LN1000 * t / (dur * 0.55)) * (0.4 + 0.6 * np.exp(-t / 0.25))
    x = layers((normalize(x), 0.0, 0.0), (normalize(wash), 0.0, -3.0))
    x = F.filt(x, F.hp(250.0), F.peak(3500.0, -3.0, 0.8), extend=False)
    return E.end_fade(normalize(B._rise(x, 0.0008), 0.0), 0.3) * vel


def cymbal_swell(dur, r, vel=0.8, tail=3.0):
    """A suspended cymbal rolled with soft mallets, swelling to its peak at dur and ringing on for tail: as it grows
    the energy climbs into the high modes, so the swell brightens as well as rises. Mono."""
    n = ns(dur + tail)
    modes = _cymbal_modes(child(r, 'modes'), t60=3.5)
    ring = M.ring(modes, tail + 0.5, child(r, 'ph'), random_phase=1.0)
    exc = np.zeros(n)
    t = 0.0
    while t < dur:
        v = (t / dur) ** 2.2 * r.uniform(0.7, 1.1)
        h = M.hammer(0.004)
        exc[ns(t):ns(t) + h.size] += h * v
        t += r.uniform(0.04, 0.065)
    x = F.convolve(exc, ring)[:n]
    tt = times(n)
    grow = np.clip(tt / dur, 0.0, 1.0) ** 2.0
    dark = F.filt(x, F.lp(1800.0, 0.6), extend=False)
    x = dark + grow * (x - dark)
    wash = N.pink(n, child(r, 'wash'), lo=900.0)
    wash = F.sweep(wash, 'lp', 900.0 + 7000.0 * grow, 0.7)
    wash *= np.clip(tt / dur, 0, 1) ** 2.6 * np.exp(-LN1000 * np.maximum(tt - dur, 0.0) / (tail * 0.7))
    x = layers((normalize(x), 0.0, 0.0), (normalize(wash), 0.0, -4.0))
    x = F.filt(x, F.hp(250.0), F.peak(3500.0, -3.0, 0.8), extend=False)
    return E.end_fade(normalize(x, 0.0), 0.3) * vel


def tam_tam(r, vel=0.8, dur=9.0):
    """A big tam-tam struck by a soft beater: a low dark bong at once, then the energy blooming up into a roaring
    shimmer over the first second or two, the whole thing ringing for many seconds. Mono."""
    n = ns(dur)
    t = times(n)
    low = M.ring(M.parts(55.0, 650.0, child(r, 'low'), 45, 7.5, 4.0, 0.2), dur, child(r, 'lp'), random_phase=1.0)
    low = F.convolve(M.hammer(0.007), low)[:n]
    high = M.ring(M.parts(650.0, 7500.0, child(r, 'high'), 160, 5.5, 2.5, 0.3), dur, child(r, 'hp'), random_phase=1.0)
    bloom = (1.0 - np.exp(-t / (0.45 + 0.4 * (1.0 - vel)))) ** 2
    high *= bloom * (1.0 + 0.25 * _wander(n, child(r, 'w'), 0.8))
    x = layers((normalize(low), 0.0, 0.0), (normalize(high), 0.0, -7.0 + 3.0 * vel))
    x = F.filt(x, F.peak(3500.0, -3.0, 0.8), F.highshelf(7000.0, -4.0), extend=False)
    return E.end_fade(normalize(B._rise(x, 0.003), 0.0), 0.5) * vel


def anvil(r, vel=0.8, kind='anvil', f0=None, takes=None):
    """Steel struck by a hammer: 'anvil' a blacksmith's anvil (a ringing tink over the block's clunk); 'rail' a
    length of rail hit with a sledge (a lower, longer clang). The work of the mine on the beat. Mono."""
    hit = _hit(takes, ('anvil', kind, f0), vel, lambda rr, q: anvil(rr, q, kind, f0))
    if hit is not None:
        return hit
    if kind == 'rail':
        f0 = f0 or 430.0
        m = M.merge(M.bar(f0 * r.uniform(0.98, 1.02), child(r, 'bar'), t60=2.6, count=6, tilt=0.6),
                    M.plate(f0 * 1.9, child(r, 'web'), count=14, t60=0.7, tilt=0.6))
        ring = M.strike(m, M.hammer(0.00025), 2.8)
        thud = kit.thunk(160.0, child(r, 'tie'), 0.08, 0.002, 5, 0.08)
        x = layers((normalize(ring), 0.0, 0.0), (normalize(thud), 0.0, -6.0))
    else:
        f0 = f0 or 780.0
        ring = M.strike(M.bar(f0 * r.uniform(0.98, 1.02), child(r, 'bar'), t60=1.6, count=5, tilt=0.8),
                        M.hammer(0.00012 + 0.0001 * (1 - vel)), 1.8)
        clunk = M.strike(M.wood(240.0, child(r, 'block'), t60=0.05, count=5), M.hammer(0.0008), 0.12)
        x = layers((normalize(ring), 0.0, 0.0), (normalize(clunk), 0.0, -4.0))
    x = F.filt(x, F.peak(3400.0, -4.0, 0.8), F.highshelf(6500.0, -6.0), extend=False)
    return E.end_fade(normalize(x, 0.0), 0.1) * vel


def stone(r, vel=0.8, size=1.0, takes=None):
    """Stone struck on stone: heavy and dry (a few rock modes dying in tens of milliseconds), the weight's thump
    under it, grit cracking off and a little crumble after. size > 1 is a bigger block. Mono."""
    hit = _hit(takes, ('stone', size), vel, lambda rr, q: stone(rr, q, size))
    if hit is not None:
        return hit
    m = M.parts(150.0 / size, 1500.0 / size, child(r, 'rock'), 9, 0.07, 0.025, 0.4)
    knock = M.strike(m, M.hammer(0.0004 + 0.0004 * (1 - vel)), 0.2)
    thump = kit.thump(110.0 / size, 42.0 / size, 0.3, 0.13)
    grit = kit.grit_burst(0.18, child(r, 'grit'), 2600.0, 700.0, 4200.0, 0.08)
    crumble = kit.grit_burst(0.45, child(r, 'crumb'), 500.0, 500.0, 3000.0, 0.3, 0.02)
    x = layers((normalize(knock), 0.0, 0.0), (thump, 0.0, -3.0), (grit, 0.0, -13.0),
               (crumble, r.uniform(0.03, 0.06), -24.0))
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), F.highshelf(6000.0, -6.0), extend=False)
    return E.end_fade(normalize(B._rise(x, 0.0005), 0.0), 0.04) * vel ** 1.2


def stone_grind(dur, r, vel=0.6, size=1.0):
    """Stone dragged over stone, slow: stick-slip catches rung through the rock, grit spilling, a rumble under it.
    Mono."""
    n = ns(dur)
    catches = kit.creak(dur, child(r, 'slip'), 18.0, 26.0, (180.0 / size, 420.0 / size, 900.0 / size), 4.0, 0.5)
    g = G.grit(dur, child(r, 'grit'), 900.0 * (0.6 + 0.4 * np.abs(_wander(n, child(r, 'gr'), 3.0))),
               500.0, 3500.0, 2.0, 4)
    rumble = F.filt(N.pink(n, child(r, 'rum')), F.lp(160.0 / size), F.hp(30.0), extend=False)
    x = layers((normalize(catches), 0.0, 0.0), (normalize(g), 0.0, -9.0), (normalize(rumble), 0.0, -4.0))
    x *= fit(E.ar(dur, min(0.4, dur * 0.3), min(0.6, dur * 0.4), 2.0), n)
    x = F.filt(x, F.peak(3300.0, -3.0, 0.8), extend=False)
    return normalize(x, 0.0) * vel


# --- Harp and keyboards ---------------------------------------------------------------------------------------------

_HARP_BOARD = [F.hp(55.0, 0.7), F.peak(180.0, 3.0, 1.5), F.peak(420.0, 2.5, 1.6), F.peak(900.0, 1.0, 1.4),
               F.peak(2600.0, -2.0, 1.0), F.highshelf(6000.0, -6.0)]


def harp(note, r, vel=0.6, dur=None):
    """A concert harp's string (Cb1-G#7) plucked by a fingertip near its middle: gut-like, nearly pure, a long ring
    in the bass and a short one up high, two polarizations beating, through the soundboard. Mono."""
    f = hz(note)
    t60 = float(np.clip(6.0 * (110.0 / f) ** 0.55, 0.8, 9.0))
    dur = dur or min(t60 * 1.2, 8.0)
    n = ns(dur)
    t = times(n)
    p = r.uniform(0.27, 0.36)
    out = np.zeros(n)
    for k in range(1, 50):
        fk = k * f * np.sqrt(1.0 + 1e-5 * k * k)
        if fk > 9000.0:
            break
        a = abs(np.sin(np.pi * k * p)) / k ** (1.95 - 0.9 * vel)
        tk = t60 / (1.0 + 0.25 * (k - 1) ** 1.3)
        m = min(n, ns(tk * 2.0))
        out[:m] += a * (0.6 * np.exp(-LN1000 * t[:m] / tk) * np.sin(2 * np.pi * fk * t[:m]) +
                        0.4 * np.exp(-LN1000 * t[:m] / (tk * 1.6)) * np.sin(2 * np.pi * fk * (1.0004) * t[:m] + 0.5))
    B._rise(out, 0.002)
    out = _through(out, _fir(('harp',), _HARP_BOARD))
    return E.end_fade(normalize(out, 0.0), 0.15) * vel ** 1.15


def _scale_notes(lo, hi, scale):
    names = scale or D_MINOR
    fl, fh = hz(lo), hz(hi)
    out = []
    for octave in range(0, 9):
        for nm in names:
            f = hz(f'{nm}{octave}')
            if fl / 1.001 <= f <= fh * 1.001:
                out.append(f)
    return sorted(out)


def harp_gliss(lo, hi, dur, r, vel=0.55, scale=None, up=True, at=0.0):
    """A glissando from lo to hi (or down) across the strings set to scale (pitch names, D minor by default), over
    dur, a little faster in its middle, each string ringing on. (mono, t0)."""
    fs = _scale_notes(lo, hi, scale)
    if not up:
        fs = fs[::-1]
    count = len(fs)
    ring = 2.5
    out = np.zeros(ns(dur + ring))
    for i, f in enumerate(fs):
        u = i / max(count - 1, 1)
        tt = dur * (0.5 - 0.5 * np.cos(np.pi * u)) * 0.6 + dur * u * 0.4 + human(r, 0.004)
        x = harp(f, child(r, 'g', i), lively(r, vel * (0.85 + 0.3 * u), 0.1), ring)
        a = ns(max(tt, 0.0))
        out[a:a + x.size] += x[:out.size - a]
    return E.end_fade(out, 0.2), at


def celesta(note, r, vel=0.6, dur=None):
    """A celesta (C4-C8): felt hammers on steel plates over wooden resonators: a sweet, nearly pure bell tone with the
    plate's faint upper modes and the box's soft knock. Mono."""
    f = hz(note)
    t60 = float(np.clip(2.4 * (523.0 / f) ** 0.4, 0.6, 3.2))
    dur = dur or t60 * 1.2
    m = M.modes([f, f * 1.0015, f * 2.76, f * 5.4, f * 4.0], [t60, t60 * 0.8, t60 * 0.25, t60 * 0.1, t60 * 0.3],
                [1.0, 0.5, 0.12 * vel + 0.04, 0.05 * vel, 0.04])
    x = M.strike(m, M.hammer(0.0007 - 0.0003 * vel), dur)
    knock = M.strike(M.wood(420.0, child(r, 'box'), t60=0.04, count=4), M.hammer(0.0008), 0.06)
    x = layers((normalize(x), 0.0, 0.0), (normalize(knock), 0.0, -24.0))
    return E.end_fade(normalize(x, 0.0), 0.1) * vel


def glockenspiel(note, r, vel=0.6, dur=None):
    """A glockenspiel (G5-C8): steel bars and a hard mallet: bright, ringing, its upper modes inharmonic. It sits in
    the ear's sharp band: use it sparingly and soft. Mono."""
    f = hz(note)
    t60 = float(np.clip(3.0 * (1047.0 / f) ** 0.3, 1.0, 4.0))
    dur = dur or t60 * 1.1
    m = M.modes([f, f * 2.71, f * 5.15, f * 8.5], [t60, t60 * 0.45, t60 * 0.22, t60 * 0.12],
                [1.0, 0.22 + 0.15 * vel, 0.08 * vel + 0.03, 0.03 * vel])
    x = M.strike(m, M.hammer(0.00035 - 0.00015 * vel), dur)
    x = F.filt(x, F.highshelf(6000.0, -4.0), extend=False)
    return E.end_fade(normalize(x, 0.0), 0.1) * vel


def piano(note, r, vel=0.7, dur=6.0):
    """A low piano strike (A0-C4): two or three stiff strings a cent apart (they beat and hand energy to each other:
    the prompt sound and the long aftersound), a felt hammer that is harder the harder it's hit, the soundboard.
    Mono."""
    f = hz(note)
    n = ns(dur)
    t = times(n)
    B_ = 0.00035 * (55.0 / f) ** 0.5
    t60 = float(np.clip(14.0 * (55.0 / f) ** 0.6, 4.0, 18.0))
    strings = 2 if f < 110 else 3
    out = np.zeros(n)
    for k in range(1, 70):
        fk = k * f * np.sqrt(1.0 + B_ * k * k)
        if fk > 6000.0:
            break
        a = abs(np.sin(np.pi * k / 8.3)) / k ** (1.5 - 0.7 * vel)
        tk = t60 / (1.0 + 0.05 * k ** 1.5)
        m = min(n, ns(tk * 1.5))
        for s in range(strings):
            d = 1.0 + (s - (strings - 1) / 2) * 0.0007 * r.uniform(0.6, 1.4)
            prompt = np.exp(-LN1000 * t[:m] / (tk * 0.22))
            after = np.exp(-LN1000 * t[:m] / tk)
            out[:m] += a / strings * (0.65 * prompt + 0.35 * after) * np.sin(2 * np.pi * fk * d * t[:m] + r.random())
    hammer = F.filt(N.white(ns(0.02), r) * E.decay(ns(0.02), 0.008), F.lp(1200.0), extend=False)
    out[:hammer.size] += hammer * 0.05 * np.max(np.abs(out))
    out = F.filt(out, F.peak(90.0, 2.0, 1.0), F.peak(220.0, 2.0, 1.2), F.peak(2500.0, -2.0, 1.0),
                 F.highshelf(5000.0, -6.0), extend=False)
    return E.end_fade(normalize(B._rise(out, 0.0015), 0.0), 0.3) * vel ** 1.2


# --- Room and bus ---------------------------------------------------------------------------------------------------

def orchestra_hall(r, t60=3.1, predelay=0.034, lp=6200.0, early_db=-3.5):
    """A scoring stage: early reflections off distant walls over the first 120 ms, then a dense tail that builds for a
    moment and decays, the lows longest, the highs first; each channel its own room's worth of noise, so it is wide.
    Unit energy, for Desk.mix."""
    dur = t60 * 1.15
    n = ns(dur)
    t = times(n)
    chans = []
    for c in range(2):
        rc = child(r, 'stage', c)
        tail = R.decaying_noise(dur, rc, t60, t60 * 0.4, 250.0, 5500.0)
        onset = np.clip((t - predelay) / 0.07, 0.0, 1.0) ** 1.5
        tail = F.filt(tail * onset, F.lp(lp, 0.6), F.hp(40.0, 0.6), extend=False)
        tail /= np.sqrt(np.sum(tail * tail)) + 1e-12
        early = R.early(rc, span=0.12, count=40, lp=6500.0, first=predelay * 0.45)
        chans.append(fit(tail, n) + fit(early, n) * db2a(early_db))
    ir = np.vstack(chans)
    return ir / np.sqrt(np.sum(ir * ir) / 2.0)


def orchestra_bus(x, circular, low_cut=35.0, presence_cut=-2.0, mud_cut=-1.0, top_cut=-2.5, glue=True):
    """The orchestra's master: rumble under 35 Hz cleared, the low-mid mud eased, the ear's sharp band (where gunfire
    and the creatures' tells live) dipped a little, the very top softened; a slow, gentle glue compressor (it holds
    the sections together without pumping) and a ceiling."""
    y = F.filt(x, F.hpn(low_cut, 2), F.peak(280.0, mud_cut, 0.7), F.peak(3300.0, presence_cut, 0.7),
               F.highshelf(10000.0, top_cut), extend=False, circular=circular)
    y = normalize(y, -6.0)
    if glue:
        comp = (lambda s: Y.compress(s, -17.0, 1.7, 0.03, 0.45, 10.0))
        y = B.periodic(comp, y) if circular else comp(y)
    lim = (lambda s: Y.limit(s, -1.0, 0.004, 0.15))
    return B.periodic(lim, normalize(y, -1.0)) if circular else lim(normalize(y, -1.0))
