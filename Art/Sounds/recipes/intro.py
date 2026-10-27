"""The new game's intro (2026-10-27, round 3, INTRO; Story/NewGameIntro.h): its score and its two small cues. All 2D,
class Music. In the score's language (band.py, music.py: D minor, the Rest's 6/8 at 75, the reed's call, the chapel's
bell) so the intro, the cold open after it and Ransom's Rest's own theme are one piece of music.

- Music.Intro.Planet (2D, 28 s, one-shot): Teropa from orbit (or Westward over the canyon in its place). A low bowed D
  and A swelling out of nothing, high strings like light on the clouds, the chapel bell far off at the TEROPA card (1.0
  s) and lower at THE REACHES (13.9 s), the reed's call (the Rest's theme) from 17.5 s as the dead are given to the
  wind, and air rising into the white from 25.8 s; everything gone by 28 s, where the level opens.
- Music.Intro.Rest (2D, 50.5 s and a 4 s tail): Ransom's Rest. The guitar alone in drop D over the Rim (0-15 s), the bell
  for the chapel (15 s) and the guitar filling out, the reed's lament over the farm (27-37 s), a big drum and low
  strings under the player's own name (37.6 s), a heartbeat under the hook (41.6-48 s), the air rising into the white
  (48.2-50.2 s) and one D minor chord left ringing under the cold open's REVENANT.
- Music.Intro.Card (2D, 3 takes, ~3 s): a card coming up (a place's name, the hero's): a soft deep drum like a far
  door, a breath of air and a small brass chime (D5, A5, F5).
- Music.Intro.Whiteout (2D, ~2.3 s, swells in): the white rising at a part's end: air drawn up past the ear and a high
  string shimmer swelling to the white, stopping as it is full.

Timing is the intro's (NewGameIntro.h, NewGameIntro.cpp captions): change both together.
"""
from lib.core import ns, child, fit, fade
from lib import env as E
from recipes import cue
from recipes import kit
from recipes import band as B
from recipes.music import SIX_EIGHT, SHAPES, SPARSE, FULL, _pick_bar, _reverb

PLANET_SECONDS = 28.0
REST_SECONDS = 50.5
WHITE_RISE = 2.0


def _bus(x, circular):
    return B.master_bus(x, circular, glue=False)


def _reed_call(desk, track, r, at, scale=1.0):
    """The Rest's call (music.py's reed tune, its first breath) starting at `at` seconds."""
    p = B.Phrase()
    p.note(at, 1.1 * scale, 'D5', scoop=1.0).note(at + 1.15 * scale, 0.4 * scale, 'C5')
    p.note(at + 1.6 * scale, 0.75 * scale, 'A4').note(at + 2.4 * scale, 0.4 * scale, 'G4')
    p.note(at + 2.85 * scale, 2.2 * scale, 'A4', vib=0.2)
    x, t0 = B.reed(p, child(r, 'call', at))
    desk.add(track, x, t0)


@cue('Music.Intro.Planet', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-15.0, align=False,
     swell=True)
def intro_planet(v, r):
    # A world seen whole: the low open fifth swelling out of silence, the high strings like light on the clouds, the bell
    # far off for each name, the reed's call for the dead, and air rising into the white.
    desk = B.Desk(None, tail=2.0)
    desk.track('drone', -6.0, 0.0, -10.0, B.cello_body)
    desk.track('high', -12.0, 0.0, -6.0, B.cello_body, 1.3)
    desk.track('bell', -10.0, -0.3, -4.0)
    desk.track('reed', -4.0, 0.15, -8.0)
    desk.track('air', -14.0, 0.0, -10.0)
    for j, (note, vel) in enumerate([('D2', 0.8), ('A2', 0.55)]):
        desk.add('drone', B.bowed(note, PLANET_SECONDS - 0.5, child(r, 'drone', j), vel, attack=5.0, release=4.0,
                                  vib=0.04, bright=0.3, kmax=36), 0.0)
    for j, note in enumerate(['A4', 'D5', 'F5']):
        desk.add('high', B.ensemble(note, 22.0, child(r, 'high', j), 0.45, voices=3, attack=6.0, release=5.0, bright=0.2,
                                    vib=0.07, width=0.9, kmax=14), 3.0 + 0.6 * j)
    desk.add('bell', B.bell('D4', child(r, 'bell', 0), 0.7, t60=8.0, dur=8.0, bright=0.6), 1.0)
    desk.add('bell', B.bell('A3', child(r, 'bell', 1), 0.5, t60=7.0, dur=7.0, bright=0.55), 13.9)
    _reed_call(desk, 'reed', r, 17.5, 1.15)
    rise = PLANET_SECONDS - WHITE_RISE - 0.2
    desk.add('air', B.swell(WHITE_RISE + 0.2, child(r, 'rise'), 250.0, 7000.0), rise)
    out = desk.mix(_reverb(r, 3.0), _bus)
    # Nothing past the part's end: the level opens there.
    return fade(fit(out, ns(PLANET_SECONDS)), 0.02, 0.3)


@cue('Music.Intro.Rest', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-15.0, align=False)
def intro_rest(v, r):
    # Ransom's Rest: the guitar alone over the Rim, the bell for the chapel, the reed over the farm, the drum under the
    # player's name, a heartbeat under the hook, the air into the white and the chord left ringing under the title.
    m = SIX_EIGHT
    desk = B.Desk(None, tail=4.5)
    desk.track('guitar', 0.0, -0.18, -16.0, B.guitar_body)
    desk.track('drone', -9.0, 0.0, -12.0, B.cello_body)
    desk.track('bell', -12.0, -0.3, -4.0)
    desk.track('reed', -2.0, 0.15, -9.0)
    desk.track('big', -3.0, 0.0, -12.0)
    desk.track('low', -5.0, 0.0, -10.0, B.cello_body)
    desk.track('heart', -12.0, 0.0, -16.0)
    desk.track('air', -12.0, 0.0, -10.0)
    desk.track('end', -1.0, 0.0, -8.0, B.cello_body)

    # The guitar: alone and sparse over the Rim (bars 0-8), fuller past the chapel (9-16), quiet under the farm (17-22),
    # gone for the name card (bar 23.5 = 37.6 s).
    chords = ['Dm', 'Dm', 'C', 'Dm', 'Bb', 'F', 'C', 'A', 'Dm',
              'Bb', 'F', 'C', 'Dm', 'Bb', 'F', 'Gm', 'A',
              'Dm', 'C', 'Bb', 'F', 'C', 'A']
    gtr = B.Guitar(child(r, 'guitar'), bright=0.42)
    for bar, shape in enumerate(chords):
        pattern = SPARSE if bar < 9 or bar >= 17 else FULL
        _pick_bar(gtr, m, bar, shape, pattern, 0.85 if bar >= 17 else 1.0)
    gtr.render(desk, 'guitar')

    desk.add('drone', B.bowed('D2', 37.0, child(r, 'drone'), 0.7, attack=3.5, release=3.0, vib=0.05, bright=0.3,
                              kmax=36), 0.0)
    desk.add('bell', B.bell('D4', child(r, 'bell', 0), 0.75, t60=8.0, dur=8.0, bright=0.7), 15.0)
    _reed_call(desk, 'reed', r, 27.6, 1.2)

    # The player's name: one deep drum and the low strings, the bell lower.
    name = 37.6
    desk.add('big', B.big_drum(child(r, 'big'), 0.8, f0=50.0, t60=1.4), name)
    for j, note in enumerate(['D2', 'A2', 'F3']):
        desk.add('low', B.bowed(note, 10.5, child(r, 'low', j), 0.8 if j == 0 else 0.6, attack=0.4, release=2.5,
                                vib=0.06, bright=0.4, kmax=30), name + 0.02 * j)
    desk.add('bell', B.bell('A3', child(r, 'bell', 1), 0.45, t60=7.0, dur=7.0, bright=0.55), name + 0.05)

    # The hook: a slow heartbeat (the one the player hasn't got) from 41.6 s to the white.
    beat = 0.95
    t = 41.6
    k = 0
    while t < REST_SECONDS - WHITE_RISE - 0.5:
        desk.add('heart', B.frame_drum(child(r, 'heart', k, 0), 0.55), t)
        desk.add('heart', B.frame_drum(child(r, 'heart', k, 1), 0.35), t + 0.28)
        t += beat
        k += 1

    # Into the white, then one chord ringing under the title.
    white = REST_SECONDS - WHITE_RISE - 0.3
    desk.add('air', B.swell(WHITE_RISE + 0.3, child(r, 'rise'), 250.0, 7000.0), white)
    end = REST_SECONDS - 0.3
    end_gtr = B.Guitar(child(r, 'endg'), bright=0.5)
    end_gtr.strum(end, [x for x in SHAPES['Dm'] if x], 0.7, True, 0.06)
    end_gtr.render(desk, 'guitar', max_ring=4.5)
    for j, note in enumerate(['D2', 'A2', 'D3', 'F3']):
        desk.add('end', B.bowed(note, 4.2, child(r, 'end', j), 0.8 if j == 0 else 0.55, attack=0.25, release=3.0,
                                vib=0.06, bright=0.4, kmax=30), end + 0.02 * j)
    return desk.mix(_reverb(r, 2.6), _bus)


# A small chime for each card, the score's notes.
CARD_CHIMES = ('D5', 'A5', 'F5')


@cue('Music.Intro.Card', variations=3, space='2D', cls='Music', jitter=0.0, conc=2, level=-17.0)
def intro_card(v, r):
    # A name coming up: a soft deep drum like a far door, a breath of air over it, a small brass chime on top.
    desk = B.Desk(None, tail=2.0)
    desk.track('drum', 0.0, 0.0, -14.0)
    desk.track('air', -10.0, 0.0, -10.0)
    desk.track('chime', -8.0, 0.2 if v % 2 else -0.2, -6.0)
    desk.add('drum', B.big_drum(child(r, 'drum'), 0.5, f0=52.0, t60=1.1), 0.0)
    desk.add('air', kit.whoosh(1.4, child(r, 'air'), 600.0, 2400.0, 0.7, 0.3, -3.0), 0.0)
    desk.add('chime', kit.chime(B.hz(CARD_CHIMES[v % len(CARD_CHIMES)]), child(r, 'chime'), t60=2.2, bright=0.6), 0.05)
    return desk.mix(_reverb(r, 2.4), _bus)


@cue('Music.Intro.Whiteout', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-16.0, align=False,
     swell=True)
def intro_whiteout(v, r):
    # The white rising: air drawn up past the ear and a high string shimmer swelling with it, stopping as it is full.
    dur = WHITE_RISE + 0.3
    desk = B.Desk(None, tail=0.6)
    desk.track('air', 0.0, 0.0, -10.0)
    desk.track('shimmer', -6.0, 0.0, -6.0, B.cello_body, 1.3)
    desk.add('air', kit.whoosh(dur, child(r, 'air'), 300.0, 6000.0, 0.8, 0.95, -3.0, 1.4), 0.0)
    for j, note in enumerate(['D6', 'A6']):
        env = fit(E.swell(dur, dur * 0.95, 0.3, 2.2), ns(dur))
        desk.add('shimmer', B.ensemble(note, dur, child(r, 'sh', j), 0.4, voices=3, tremolo=12.0, env=env, bright=0.5,
                                       kmax=10), 0.0)
    out = desk.mix(_reverb(r, 1.6), _bus)
    return fade(fit(out, ns(dur + 0.15)), 0.0, 0.15)
