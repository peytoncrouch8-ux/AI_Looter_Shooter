"""Voices (3D): the Unpaid's murmured barks, the creatures' idle calls, and the living heard through Ransom's Rest's walls.

- The Unpaid's barks are lines from their lives (Source/.../Audio/CreatureVoiceBarkLines.cpp), shown in words over them
  and murmured, never spoken. The game plays one syllable of Creature.Unpaid.Murmur (voiced: a spot, a hurt, a fallen
  packmate) or Creature.Unpaid.Mutter (whispered: muttering to itself, last words) for each syllable of the line, in its
  rhythm and contour (CreatureBarks::BuildMurmur: about 0.155 s apart, held at commas and dots, rising at a question),
  pitched for each Unpaid. So a syllable here starts at once and fades soon enough to run into the next: an onset (none,
  a breath, a hum opening up, a soft tap) gliding into a vowel and away, two breathy voices a hair apart (one ghost sounds
  like many), a faint cold ring, and a short dark wash like the Unpaid's other voices.
- Spiders and slimes have no words, only a rare idle call: a spider's mandibles working slowly, a slime's contented
  gurgle.
- The living (World/TownLifeSubsystem), who hide from the walking corpse behind their shutters: hushed voices, coughs, a
  child's music box (its own little tune, written for the game), a door's latch, a floorboard, a mother's hush, Tilly's
  saw and hammer, each in a small room's box and then through a wall, so only the low half and the shape get out; and a
  dog barking far off across town, mostly echo.
"""
import numpy as np

from lib.core import SR, ns, times, layers, normalize, child, jitter
from lib import noise as N, env as E, osc as O, filters as F, dist as D, modal as M, reverb as R
from lib import voice as V
from recipes import cue
from recipes import kit
from recipes.creatures import chitter, rasp, _skitter, _outside


# --- Shared --------------------------------------------------------------------------------------------------------

def _cold(x, r, rate=33.0, depth=0.12):
    """The faint ring modulation the Unpaid's voices carry: not quite of this world."""
    return x * (1.0 - depth + depth * np.sin(2 * np.pi * rate * times(x.size) + 2 * np.pi * r.random()))


def _wash(x, r, rel_db=-15.0, t60=0.9, lp=3000.0):
    """The Unpaid's dark open-air wash, short, so syllables smear together a little rather than ring."""
    ir = R.open_air(child(r, 'wash'), t60=t60, lp=lp, predelay=0.015, density=0.8)
    wet = F.convolve(F.filt(x, F.hp(180.0), extend=False), ir)
    return layers(x, kit.set_level(wet, x, rel_db, 0.03))


def _wall(x, r, lp=800.0, room_db=-9.0, t60=0.35):
    """Somebody in a small room heard from the street: the room's box first, then the wall, which lets only the low half
    and the shape of it out."""
    x = normalize(x)
    ir = R.room(child(r, 'room'), t60=t60, size=0.6)
    roomy = layers(x, kit.set_level(F.convolve(x, ir), x, room_db, 0.05))
    return F.filt(roomy, F.lp(lp, 0.6), F.lp(lp * 1.3, 0.6), F.hp(80.0), extend=False)


def _far(x, r, wet_db=-2.0):
    """Far off across town: dark and thin, more echo off the false fronts and the ridges than sound."""
    dry = F.filt(normalize(x), F.lp(1900.0, 0.6), F.hp(260.0), extend=False)
    ir = R.outdoor(child(r, 'far'), dur=2.2, slaps=((0.17, -5.0), (0.41, -9.0), (0.78, -14.0)), slap_lp=1800.0,
                   tail_t60=1.5, tail_db=-7.0, tail_start=0.05, tail_peak=0.3, tail_lp=1600.0)
    return layers(dry, kit.set_level(F.convolve(dry, ir), dry, wet_db, 0.05))


# Vowel glides a syllable can take (from, to), and the onsets it can start with.
GLIDES = [('a', 'uh'), ('o', 'u'), ('e', 'i'), ('uh', 'er'), ('ae', 'a'), ('oo', 'u'), ('i', 'e'), ('er', 'uh'),
          ('a', 'o'), ('o', 'uh'), ('e', 'a'), ('u', 'oo')]
ONSETS = ['', 'h', 'm', 't']


def _onset(x, kind, r, dur):
    """A syllable's start: a breath before the voice, a hum opening into the vowel (a lip closing and parting), or a soft
    tap of the tongue."""
    if kind == 'h':
        lead = 0.03
        breath = V.whisper(lead + 0.04, [(0.0, 'h'), (lead + 0.04, 'a')], child(r, 'breath'))
        breath = normalize(breath * E.ar(lead + 0.04, 0.01, 0.04, 2.0))
        return layers((breath, 0.0, -9.0), (x, lead, 0.0))
    if kind == 'm':
        n = x.size
        cut = np.full(n, 6500.0)
        k = min(n, ns(0.05))
        cut[:k] = np.geomspace(380.0, 6500.0, k)
        return F.sweep(x, 'lp', cut, 0.8)
    if kind == 't':
        tap = N.band(ns(0.008), child(r, 'tap'), 1400.0, 4200.0) * E.perc(0.008, 0.0004, 0.005)
        return layers((normalize(tap), 0.0, -12.0), (x, 0.006, 0.0))
    return x


def _syllable(r, dur, f0, vowels, onset, breath, voices=2, rough=0.12, shift=1.06, tilt=-2.0):
    """One voiced syllable: the vowel glide on a little lift and fall of pitch, sung by voices a hair apart."""
    n = ns(dur)
    v1, v2 = vowels
    f = O.glide([(0.0, f0 * 1.02), (dur * 0.35, f0 * 1.04), (dur, f0 * 0.93)], n=n)
    keys = [(0.0, v1), (dur * 0.45, v1), (dur, v2)]
    spread = ns(0.01)
    out = np.zeros(n + spread * voices)
    for i in range(voices):
        rr = child(r, 'voice', i)
        cents = 12.0 * (i - (voices - 1) / 2.0) + rr.uniform(-3.0, 3.0)
        x = V.voice(f * 2.0 ** (cents / 1200.0), keys, rr, breath=breath, jitter=0.012, shimmer=0.1, tilt=tilt,
                    shift=shift * rr.uniform(0.98, 1.02), rough=rough)
        k = int(spread * i * rr.uniform(0.5, 1.0))
        out[k:k + n] += normalize(x) * (1.0 if i == 0 else 0.6)
    out = out[:n + spread * voices]
    env = np.zeros(out.size)
    env[:n] = E.ar(dur, 0.016, dur * 0.6, 2.0)
    env[n:] = 0.0
    return _onset(out * env, onset, r, dur)


# --- The Unpaid's murmur -------------------------------------------------------------------------------------------

@cue('Creature.Unpaid.Murmur', variations=12, att='Creature', jitter=0.03, conc=3, level=-11.0)
def unpaid_murmur(v, r):
    # One syllable of a bark said aloud: a breathy, cold voice on a vowel glide, never a word. The game strings them in
    # the line's rhythm and pitches them per Unpaid and per syllable.
    dur = r.uniform(0.15, 0.21)
    f0 = 178.0 * jitter(r, 1, 0.03)
    x = _syllable(child(r, 'syl'), dur, f0, GLIDES[v % len(GLIDES)], ONSETS[v % len(ONSETS)], breath=0.45)
    x = _cold(normalize(x), child(r, 'cold'))
    x = F.filt(x, F.peak(3200.0, -3.0, 0.8), F.hp(120.0), extend=False)
    return _wash(x, r)


@cue('Creature.Unpaid.Mutter', variations=10, att='Creature', jitter=0.03, conc=3, level=-16.0)
def unpaid_mutter(v, r):
    # One syllable muttered under the breath (to itself, or dying): a whispered vowel glide, a ghost of a voice under
    # it, now and then a hiss of a consonant leading in.
    dur = r.uniform(0.14, 0.2)
    v1, v2 = GLIDES[(v * 5) % len(GLIDES)]
    keys = [(0.0, v1), (dur * 0.5, v1), (dur, v2)]
    whisper = V.whisper(dur, keys, child(r, 'whisper'), shift=1.06)
    f = O.glide([(0.0, 165.0), (dur, 150.0)], n=ns(dur)) * jitter(r, 1, 0.03)
    under = V.voice(f, keys, child(r, 'under'), breath=0.85, jitter=0.02, shimmer=0.15, tilt=-4.0, shift=1.06)
    x = layers((normalize(whisper), 0.0, 0.0), (normalize(under), 0.0, -7.0)) * E.ar(dur, 0.014, dur * 0.6, 2.0)
    if v % 3 == 2:
        s = V.hiss(ns(0.05), child(r, 'hiss'), (2400.0, 3600.0, 5600.0)) * E.ar(0.05, 0.01, 0.03, 2.0)
        x = layers((normalize(s), 0.0, -10.0), (x, 0.035, 0.0))
    x = _cold(normalize(x), child(r, 'cold'), 29.0, 0.1)
    x = F.filt(x, F.peak(3300.0, -4.0, 0.8), F.hp(150.0), extend=False)
    return _wash(x, r, -13.0, 0.8)


# --- Idle calls ----------------------------------------------------------------------------------------------------

@cue('Creature.Spider.Idle', variations=4, att='Creature', jitter=0.06, conc=2, level=-15.0)
def spider_idle(v, r):
    # A spider at rest: its mandibles working slowly and unevenly, a short soft rasp of its file now and then, a leg
    # shifting with a tap or two. Unhurried, nothing like its alarm.
    dur = r.uniform(0.4, 0.6)
    ch = chitter(dur, child(r, 'chitter'), r.uniform(7.0, 10.0), r.uniform(12.0, 16.0), spread=0.45)
    ch = ch * E.ar(ch.size / SR, 0.04, 0.15, 2.0)
    parts = [(ch, 0.0, 0.0)]
    if v % 2 == 1:
        rs = rasp(0.22, child(r, 'rasp'), r.uniform(50.0, 65.0), r.uniform(40.0, 50.0), (1100.0, 2300.0, 3800.0), 3.0)
        parts.append((rs * E.ar(0.22, 0.03, 0.12, 2.0), dur * 0.6, -8.0))
    parts.append((_skitter(child(r, 'legs'), 2, 0.0, 0.12), r.uniform(0.05, 0.3), -14.0))
    x = layers(*parts)
    return _outside(F.filt(x, F.peak(3300.0, -4.0, 0.8), F.highshelf(7000.0, -3.0), extend=False), r, -22.0)


@cue('Creature.Slime.Idle', variations=4, att='Creature', jitter=0.06, conc=2, level=-16.0)
def slime_idle(v, r):
    # A slime at rest: a few bubbles working up through its jelly and popping at the top, a lazy wobble of its body, a
    # low contented burble under it.
    out = np.zeros(1)
    t = 0.0
    for i in range(int(r.integers(3, 7))):
        out = layers(out, (kit.bubble(r.uniform(260.0, 650.0), child(r, 'b', i), 0.12), t, r.uniform(-6.0, 0.0)))
        t += r.uniform(0.06, 0.16)
    wobble = kit.squelch(0.35, child(r, 'wobble'), r.uniform(380.0, 460.0), r.uniform(220.0, 280.0), 2.5)
    burble = F.filt(N.pink(ns(0.5), child(r, 'burble')) * kit.turbulence(ns(0.5), child(r, 'bt'), 40.0, 0.8),
                    F.lp(500.0, 0.8), F.hp(90.0), extend=False) * E.ar(0.5, 0.1, 0.3, 2.0)
    x = layers((normalize(out), 0.0, 0.0), (wobble, r.uniform(0.0, 0.2), -9.0), (normalize(burble), 0.0, -16.0))
    return _outside(F.filt(x, F.lp(5000.0, 0.7), extend=False), r, -24.0)


# --- The living, through their walls -------------------------------------------------------------------------------

def _phrase(r, f0, count, t0, breath=0.45):
    """A hushed phrase: count syllables on a falling line, their vowels random, a little apart. Returns (signal, end)."""
    out = np.zeros(1)
    t = t0
    for k in range(count):
        d = r.uniform(0.11, 0.22)
        fall = 1.06 - 0.14 * k / max(1, count - 1)
        f = O.glide([(0.0, f0 * fall * 1.03), (d, f0 * fall * 0.95)], n=ns(d)) * jitter(r, 1, 0.03)
        vowels = [str(x) for x in r.choice(['a', 'e', 'o', 'uh', 'i', 'er', 'ae'], 2)]
        s = V.voice(f, [(0.0, vowels[0]), (d, vowels[1])], child(r, 'syl', k), breath=breath, jitter=0.02, tilt=-3.0)
        out = layers(out, (normalize(s) * E.ar(d, 0.02, d * 0.5, 2.0), t, r.uniform(-4.0, 0.0)))
        t += d + r.uniform(0.02, 0.08)
    return out, t


@cue('World.TownLife.Voices', variations=4, att='Creature', cls='Ambience', jitter=0.03, conc=1, level=-19.0, swell=True)
def townlife_voices(v, r):
    # Two people behind a wall, keeping their voices down, trading a few words: a man and a woman (1), three quick turns
    # of a woman, a man, the woman again (2), two women (3), a man and an old man slower and lower (4).
    speakers = [[118.0, 212.0], [205.0, 122.0, 210.0], [198.0, 232.0], [124.0, 102.0]][v]
    out = np.zeros(1)
    t = 0.0
    for i, f0 in enumerate(speakers):
        turns = 1 if len(speakers) > 2 else int(r.integers(1, 3))
        for j in range(turns):
            phrase, t = _phrase(child(r, 'turn', i, j), f0 * jitter(r, 1, 0.03), int(r.integers(3, 8)), t)
            out = layers(out, phrase)
            t += r.uniform(0.12, 0.25)
        t += r.uniform(0.2, 0.45)
    return _wall(out, r, 760.0)


def _cough(r, f0, rough, scale=1.0):
    """One cough: the glottis bursting open on a rough, breathy 'a' falling to 'uh', and the air tearing out with it."""
    d = r.uniform(0.16, 0.26) * scale
    n = ns(d)
    f = O.glide([(0.0, f0 * 1.3), (d * 0.2, f0), (d, f0 * 0.7)], n=n)
    voiced = V.voice(f, [(0.0, 'a'), (d, 'uh')], child(r, 'v'), breath=0.75, jitter=0.05, rough=rough, tilt=2.0)
    air = V.whisper(d, [(0.0, 'a'), (d, 'h')], child(r, 'air'))
    return layers((normalize(voiced), 0.0, 0.0), (normalize(air), 0.0, -4.0)) * E.perc(d, 0.004, d * 0.55)


@cue('World.TownLife.Cough', variations=4, att='Creature', cls='Ambience', jitter=0.04, conc=1, level=-18.0)
def townlife_cough(v, r):
    # A cough through a wall: a man's two (1), a woman's dry one or two (2), an old woman's small cough and a clearing of
    # her throat (3), a child's three quick little ones (4).
    out = np.zeros(1)
    if v == 0:
        for k in range(2):
            out = layers(out, (_cough(child(r, 'c', k), 120.0 * jitter(r, 1, 0.05), 0.55), k * r.uniform(0.32, 0.42)))
    elif v == 1:
        for k in range(int(r.integers(1, 3))):
            out = layers(out, (_cough(child(r, 'c', k), 225.0 * jitter(r, 1, 0.05), 0.35), k * r.uniform(0.28, 0.36)))
    elif v == 2:
        out = _cough(child(r, 'c'), 205.0, 0.4, 0.8)
        d = 0.32
        f = O.glide([(0.0, 150.0), (d, 125.0)], n=ns(d))
        clear = V.voice(f, [(0.0, 'er'), (d, 'uh')], child(r, 'clear'), breath=0.6, jitter=0.04, rough=0.7, tilt=-1.0)
        out = layers(out, (normalize(clear) * E.ar(d, 0.03, 0.15, 2.0), 0.42, -5.0))
    else:
        for k in range(3):
            out = layers(out, (_cough(child(r, 'c', k), 330.0 * jitter(r, 1, 0.05), 0.2, 0.6), k * r.uniform(0.19, 0.24)))
    return _wall(out, r, 820.0)


# A music box's comb tooth is a bar clamped at one end: its overtones sit at 6.27 and 17.55 times the note, and die fast.
TINE = [(1.0, 1.0, 1.0), (6.27, 0.22, 0.32), (17.55, 0.05, 0.1)]

# The music box's tunes (written for the game): a slow lullaby phrase in D minor, its answer, and the first one's end
# winding down as the spring runs out (the comb's pitch holds; only the cylinder slows). Notes and beats; a bass tooth
# on the first and fifth beats.
TUNES = [
    (['D5', 'F5', 'A5', 'G5', 'F5', 'E5', 'F5', 'D5'], [1, 1, 1, 1, 1, 1, 1, 2], ['D4', 'A3']),
    (['A4', 'D5', 'E5', 'F5', 'E5', 'D5', 'C#5', 'D5'], [1, 1, 1, 1, 1, 1, 1, 2], ['D4', 'A3']),
    (['F5', 'E5', 'D5', 'A4', 'Bb4', 'A4', 'G4', 'A4'], [1, 1, 1, 1, 1, 1, 1, 1], ['D4', 'F4']),
]


def _tine(f0, r, t60=1.6):
    """A comb tooth plucked by the cylinder's pin: its bar modes rung, and the pin's small tick as it slips off."""
    m = M.modes([f0 * k for k, _, _ in TINE], [t60 * s for _, _, s in TINE], [a for _, a, _ in TINE])
    ring = M.strike(m, M.hammer(0.00015), t60 * 1.1)
    tick = N.band(ns(0.002), r, 3000.0, 9000.0) * E.perc(0.002, 0.0002, 0.0012)
    return normalize(layers(normalize(ring), (normalize(tick), 0.0, -22.0)), 0.0)


@cue('World.TownLife.MusicBox', variations=3, att='Creature', cls='Ambience', jitter=0.0, conc=1, level=-20.0)
def townlife_music_box(v, r):
    # A child's music box behind the shutters: its little tune on the comb, the box's wood under it (a third take winds
    # down, each note later than the last).
    notes, beats, bass = TUNES[v]
    beat = 0.38
    out = np.zeros(1)
    t = 0.0
    for k, (name, length) in enumerate(zip(notes, beats)):
        out = layers(out, (_tine(kit.note(name), child(r, 'n', k)), t, r.uniform(-2.0, 0.0)))
        if k in (0, 4):
            out = layers(out, (_tine(kit.note(bass[0 if k == 0 else 1]), child(r, 'b', k), 2.0), t + 0.004, -5.0))
        slow = 1.0 + (0.09 * k ** 1.3 if v == 2 else 0.0)
        t += beat * length * slow * r.uniform(0.97, 1.03)
    # The little wooden box rings under the comb.
    out = F.filt(out, F.peak(520.0, 4.0, 1.4), F.peak(1100.0, 2.0, 1.6), extend=False)
    return _wall(out, r, 1500.0, -8.0)


def _bark(r, f0, dur):
    """One bark: a rough, pressed yelp snapping up in pitch and falling, through a small dog's throat, driven hard."""
    n = ns(dur)
    f = O.glide([(0.0, f0 * 0.8), (dur * 0.25, f0 * 1.15), (dur, f0 * 0.7)], n=n)
    x = V.voice(f, [(0.0, 'a'), (dur * 0.4, 'ae'), (dur, 'uh')], r, breath=0.35, jitter=0.03, rough=0.5, tilt=3.0,
                shift=1.3)
    x = D.drive(normalize(x), 8.0, 'tanh')
    return normalize(x * E.perc(dur, 0.006, dur * 0.6))


@cue('World.TownLife.DogFar', variations=3, att='Creature', cls='Ambience', jitter=0.05, conc=1, level=-16.0)
def townlife_dog_far(v, r):
    # A dog barking far off across town, shut in somewhere: two, three or four barks, and the echo off the false fronts
    # and the ridges louder than the barks themselves.
    f0 = [420.0, 480.0, 390.0][v] * jitter(r, 1, 0.04)
    out = np.zeros(1)
    t = 0.0
    for k in range([2, 3, 4][v]):
        out = layers(out, (_bark(child(r, 'bark', k), f0 * jitter(r, 1, 0.05), r.uniform(0.12, 0.18)), t,
                           r.uniform(-3.0, 0.0)))
        t += r.uniform(0.28, 0.45)
    return _far(out, r)


@cue('World.TownLife.Latch', variations=3, att='Creature', cls='Ambience', jitter=0.05, conc=1, level=-18.0)
def townlife_latch(v, r):
    # A door's latch inside, checked again: a thumb latch lifted and let fall (1), a bolt slid and shot home (2), a chain
    # rattled into its slot (3). The door's wood knocks under each.
    door = kit.thunk(r.uniform(160.0, 200.0), child(r, 'door'), 0.09, 0.001, 6, 0.1)
    if v == 0:
        lift = kit.metal_click(child(r, 'lift'), 2500.0, 7000.0, 0.03, 0.0001, 5, -12.0)
        drop = kit.clack(child(r, 'drop'), 520.0, 2000.0, 6500.0, 0.05, 0.05, -4.0, -10.0)
        t = r.uniform(0.25, 0.4)
        x = layers((lift, 0.0, -6.0), (drop, t, 0.0), (door, t + 0.002, -6.0))
    elif v == 1:
        slide = kit.scrape(0.18, child(r, 'slide'), 900.0, 1500.0, 10.0, 0.6)
        home = kit.clack(child(r, 'home'), 600.0, 1800.0, 6000.0, 0.06, 0.05, -3.0, -8.0)
        x = layers((slide, 0.0, -8.0), (home, 0.17, 0.0), (door, 0.172, -5.0))
    else:
        chain = kit.jingle(0.3, child(r, 'chain'), 7, 2200.0, 8000.0, 0.05, 0.05)
        seat = kit.metal_click(child(r, 'seat'), 1800.0, 6000.0, 0.05, 0.0001, 6, -10.0)
        x = layers((chain, 0.0, -4.0), (seat, 0.32, 0.0), (door, 0.322, -9.0))
    return _wall(x, r, 2000.0, -11.0, 0.3)


@cue('World.TownLife.Creak', variations=4, att='Creature', cls='Ambience', jitter=0.06, conc=2, level=-19.0, swell=True)
def townlife_creak(v, r):
    # Somebody inside shifting their weight on an old floorboard (1-3), or a rocking chair going (4): two slow creaks of
    # the rockers on the boards.
    if v == 3:
        out = np.zeros(1)
        for k in range(2):
            c = kit.creak(r.uniform(0.45, 0.6), child(r, 'rock', k), 12.0, 22.0, (170.0, 420.0, 900.0), 7.0, 0.3)
            out = layers(out, (c, k * r.uniform(0.85, 1.0), -2.0 * k))
    else:
        out = kit.creak(r.uniform(0.4, 0.8), child(r, 'board'), r.uniform(12.0, 26.0), r.uniform(25.0, 45.0),
                        (r.uniform(180.0, 260.0), r.uniform(450.0, 650.0), r.uniform(900.0, 1300.0)), 8.0, 0.3)
    return _wall(out, r, 1100.0, -9.0)


@cue('World.TownLife.Hush', variations=3, att='Creature', cls='Ambience', jitter=0.03, conc=1, level=-21.0, swell=True)
def townlife_hush(v, r):
    # A mother hushing her child behind the shutters: a long "shh" and the child's small whimper after it (1), the
    # whimper first and the hush on it (2), two short hushes (3).
    def shh(dur, rr):
        s = V.hiss(ns(dur), rr, (1900.0, 2700.0, 3900.0), (2.5, 2.5, 2.0), (0.8, 1.0, 0.5), 900.0)
        return normalize(s * E.ar(dur, dur * 0.25, dur * 0.4, 2.0))

    def whimper(rr):
        d = r.uniform(0.4, 0.55)
        f = O.glide([(0.0, 380.0), (d * 0.3, 420.0), (d, 320.0)], n=ns(d))
        w = V.voice(f, [(0.0, 'uh'), (d, 'u')], rr, breath=0.5, jitter=0.03, shimmer=0.2, vib_rate=6.5, vib_depth=0.3,
                    shift=1.25)
        return normalize(w * E.ar(d, 0.05, d * 0.5, 2.0))

    if v == 0:
        x = layers((shh(0.7, child(r, 'shh')), 0.0, 0.0), (whimper(child(r, 'whimper')), 0.85, -6.0))
    elif v == 1:
        x = layers((whimper(child(r, 'whimper')), 0.0, -6.0), (shh(0.6, child(r, 'shh')), 0.5, 0.0))
    else:
        x = layers((shh(0.3, child(r, 'shh', 0)), 0.0, 0.0), (shh(0.35, child(r, 'shh', 1)), 0.45, -2.0))
    return _wall(x, r, 1900.0, -10.0)


def _saw_stroke(r, dur, push):
    """A handsaw's stroke through a board: its teeth catching and slipping (a fast stick-slip) rung through the board and
    the blade, louder on the cutting push than the pull back."""
    rate = r.uniform(70.0, 95.0) * (1.0 if push else 0.8)
    teeth = kit.creak(dur, child(r, 'teeth'), rate, rate * 1.1, (720.0, 1500.0, 2600.0), 4.0, 0.12)
    n = ns(dur)
    dust = N.band(n, child(r, 'dust'), 1500.0, 6000.0) * np.abs(np.sin(np.pi * np.arange(n) * rate / SR)) ** 4
    x = layers((teeth, 0.0, 0.0), (normalize(dust), 0.0, -12.0)) * E.ar(dur, dur * 0.2, dur * 0.3, 2.0)
    return normalize(x) * (1.0 if push else 0.55)


def _hammer_tap(r):
    """A hammer on a nail into a pine lid: the steel's bright tick on the nail head and the board's knock."""
    nail = kit.metal_click(child(r, 'nail'), 2200.0, 7500.0, 0.05, 0.0001, 6, -10.0)
    board = kit.thunk(r.uniform(200.0, 240.0), child(r, 'board'), 0.07, 0.0008, 6, 0.1)
    return normalize(layers((board, 0.0, 0.0), (nail, 0.0, -5.0)))


@cue('World.TownLife.Workshop', variations=3, att='Creature', cls='Ambience', jitter=0.04, conc=1, level=-18.0)
def townlife_workshop(v, r):
    # Tilly at her work in the back of the shop: saw strokes through a board, then a few hammer taps (1), nails going
    # into a lid, tap by tap (2), the saw alone, slowing as the cut ends (3).
    out = np.zeros(1)
    t = 0.0
    if v in (0, 2):
        strokes = int(r.integers(4, 7))
        for k in range(strokes):
            d = r.uniform(0.28, 0.38) * (1.0 + (0.12 * k if v == 2 else 0.0))
            out = layers(out, (_saw_stroke(child(r, 'stroke', k), d, k % 2 == 0), t))
            t += d + r.uniform(0.02, 0.06)
    if v in (0, 1):
        t += r.uniform(0.35, 0.5) if v == 0 else 0.0
        for k in range(3 if v == 0 else 4):
            out = layers(out, (_hammer_tap(child(r, 'tap', k)), t, -2.0 if k == 0 else 0.0))
            t += r.uniform(0.32, 0.42)
    return _wall(out, r, 1200.0, -9.0)
