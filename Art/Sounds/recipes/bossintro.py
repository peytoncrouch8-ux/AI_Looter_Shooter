"""The boss intros (2026-10-27, playtest round 4, SCENES; Scenes/BossIntro.h): each boss's bed under its scene, the
reveal's hit, and the Gravemother's dive into the Sink's floor and her burst out of it.

- Music.BossIntro.Gravemother (2D, ~7.8 s, one-shot): low bowed D and A out of nothing, the Sink's ground rumbling up
  from her dive (2.6 s), a soft deep drum on each burst of the line toward the player (3.0, 3.8, 4.6, 5.4, 6.2 s: the
  script's TrailFrom-TrailTo, 5 puffs), high strings shivering (tremolo) from 3.8 s, a riser into the reveal at 6.6 s.
- Music.BossIntro.Foreman (2D, ~6.8 s): the stope in the dark: water dripping, three slow knocks on timber far down the
  drift (1.2, 2.4, 3.6 s: the knockers who stopped warning), a low drone and one cold high harmonic, a riser into his turn
  at 5.6 s.
- Music.BossIntro.Abel (2D, ~6.8 s): the boards at dusk: the chapel bell far off at 0.3 s, low strings in D minor, the
  reed's call (the Rest's theme, slowed) from 1.6 s, a riser into his lantern at 5.6 s.
- Music.BossIntro.Reveal (2D, 2 takes, ~3 s): the boss revealed (the code plays it at LiveAt as the bed fades): one big
  drum, a low string chord struck hard and left ringing, a faint anvil on top.
- Boss.Gravemother.Burrow (3D, ~1.6 s): she dives: forelegs raking into the floor, the earth swallowing her with a deep
  thump, soil pouring in after, the rumble going away under the ground.
- Boss.Gravemother.Emerge (3D, ~2.2 s): she bursts out: a blast of packed earth thrown up, its crust cracking, clods
  raining back down, the Sink's cliffs answering. (The spider's own Emerge plays on top: this is the ground.)

Timing is the scripts' (Scenes/BossIntro.cpp MakeGravemother/MakeForeman/MakeAbel: LiveAt, DiveAt, the trail's puffs):
change both together. The beds are faded out by the code at LiveAt; anything after it is only their tail.
"""
from lib.core import ns, layers, normalize, child, fit, fade
from lib import filters as F
from recipes import cue
from recipes import kit
from recipes import band as B
from recipes.music import _reverb
from recipes.bosses import _pressure, _rumble, _sink, _master, _snaps
from recipes.gravemother_barrow import _earth, _lumps, _lv
from recipes.intro import _reed_call

GRAVEMOTHER_LIVE = 6.6
GRAVEMOTHER_DIVE = 2.6
GRAVEMOTHER_PUFFS = (3.0, 3.8, 4.6, 5.4, 6.2)
FOREMAN_LIVE = 5.6
ABEL_LIVE = 5.6
TAIL = 1.2


def _bus(x, circular):
    return B.master_bus(x, circular, glue=False)


def _riser(desk, track, r, live, dur=2.2):
    """Air drawn up into the reveal, stopping as it lands."""
    desk.add(track, B.swell(dur, child(r, 'rise'), 200.0, 6500.0), live - dur)


def _cut(out, live):
    """Nothing long after the reveal: the code fades the bed there; a short tail so a late fade never cuts a ring."""
    return fade(fit(out, ns(live + TAIL)), 0.02, TAIL * 0.8)


@cue('Music.BossIntro.Gravemother', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-14.0, align=False,
     swell=True)
def bossintro_gravemother(v, r):
    # The Sink before she comes: a low open fifth out of nothing, the ground rumbling up from her dive, a soft deep drum
    # under each burst as she comes under the floor, strings shivering, the air drawn up into her burst.
    desk = B.Desk(None, tail=2.0)
    desk.track('drone', -6.0, 0.0, -12.0, B.cello_body)
    desk.track('ground', -4.0, 0.0, -18.0)
    desk.track('drum', -2.0, 0.0, -12.0)
    desk.track('high', -12.0, 0.0, -8.0, B.cello_body, 1.2)
    desk.track('rise', -10.0, 0.0, -10.0)
    for j, (note, vel) in enumerate([('D1', 0.8), ('A1', 0.55)]):
        desk.add('drone', B.bowed(note, GRAVEMOTHER_LIVE + 0.6, child(r, 'drone', j), vel, attack=2.0, release=1.0,
                                  vib=0.03, bright=0.25, kmax=36), 0.0)
    rumble = _rumble(GRAVEMOTHER_LIVE - GRAVEMOTHER_DIVE + 0.8, child(r, 'rumble'), 160.0, 2.5,
                     GRAVEMOTHER_LIVE - GRAVEMOTHER_DIVE, 3.0)
    desk.add('ground', rumble, GRAVEMOTHER_DIVE)
    for k, t in enumerate(GRAVEMOTHER_PUFFS):
        # Nearer each time: louder and a little higher as she comes.
        desk.add('drum', B.big_drum(child(r, 'puff', k), 0.45 + 0.1 * k, f0=44.0 + 2.0 * k, t60=1.0), t)
    for j, note in enumerate(['D5', 'Eb5']):
        desk.add('high', B.ensemble(note, GRAVEMOTHER_LIVE - 3.8 + 0.3, child(r, 'high', j), 0.4, voices=3, attack=1.6,
                                    release=0.3, tremolo=11.0, bright=0.35, kmax=12), 3.8)
    _riser(desk, 'rise', r, GRAVEMOTHER_LIVE)
    return _cut(desk.mix(_reverb(r, 2.6), _bus), GRAVEMOTHER_LIVE)


@cue('Music.BossIntro.Foreman', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-14.0, align=False,
     swell=True)
def bossintro_foreman(v, r):
    # The stope in the dark: water dripping, three slow knocks on timber far down the drift, a low drone and one cold high
    # harmonic, the air drawn up into his turn.
    desk = B.Desk(None, tail=2.0)
    desk.track('drone', -6.0, 0.0, -12.0, B.cello_body)
    desk.track('drip', -14.0, 0.0, -6.0)
    desk.track('knock', -4.0, -0.3, -6.0)
    desk.track('high', -16.0, 0.2, -8.0, B.cello_body, 1.0)
    desk.track('rise', -10.0, 0.0, -10.0)
    desk.add('drone', B.bowed('D2', FOREMAN_LIVE + 0.6, child(r, 'drone'), 0.8, attack=1.5, release=1.0, vib=0.02,
                              bright=0.2, kmax=30), 0.0)
    rr = child(r, 'drips')
    t = 0.4
    k = 0
    while t < FOREMAN_LIVE - 0.4:
        drop = kit.bubble(rr.uniform(900.0, 1500.0), child(rr, 'd', k), 0.12, 1.0)
        desk.add('drip', drop, t, gain_db=-rr.uniform(0.0, 6.0), pan=rr.uniform(-0.6, 0.6))
        t += rr.uniform(0.5, 1.1)
        k += 1
    for k, at in enumerate((1.2, 2.4, 3.6)):
        desk.add('knock', B.wood_block(child(r, 'knock', k), 0.7, f0=420.0), at)
    desk.add('high', B.ensemble('A5', FOREMAN_LIVE - 1.0 + 0.3, child(r, 'high'), 0.35, voices=2, attack=2.0, release=0.3,
                                bright=0.2, vib=0.05, kmax=8), 1.0)
    _riser(desk, 'rise', r, FOREMAN_LIVE, 1.8)
    return _cut(desk.mix(_reverb(r, 3.0), _bus), FOREMAN_LIVE)


@cue('Music.BossIntro.Abel', variations=1, space='2D', cls='Music', jitter=0.0, conc=1, level=-14.0, align=False,
     swell=True)
def bossintro_abel(v, r):
    # The boards at dusk: the chapel bell far off, low strings in D minor, the reed's call slowed (the Rest's theme, his
    # home's), the air drawn up into his lantern.
    desk = B.Desk(None, tail=2.0)
    desk.track('bell', -10.0, -0.3, -4.0)
    desk.track('low', -6.0, 0.0, -10.0, B.cello_body)
    desk.track('reed', -4.0, 0.15, -8.0)
    desk.track('rise', -10.0, 0.0, -10.0)
    desk.add('bell', B.bell('D3', child(r, 'bell'), 0.6, t60=7.0, dur=6.0, bright=0.5), 0.3)
    for j, note in enumerate(['D2', 'A2', 'F3']):
        desk.add('low', B.bowed(note, ABEL_LIVE + 0.6, child(r, 'low', j), 0.75 if j == 0 else 0.5, attack=1.8, release=1.0,
                                vib=0.05, bright=0.35, kmax=30), 0.05 * j)
    _reed_call(desk, 'reed', r, 1.6, 0.85)
    _riser(desk, 'rise', r, ABEL_LIVE)
    return _cut(desk.mix(_reverb(r, 2.8), _bus), ABEL_LIVE)


@cue('Music.BossIntro.Reveal', variations=2, space='2D', cls='Music', jitter=0.0, conc=1, level=-10.0)
def bossintro_reveal(v, r):
    # The boss revealed: one big drum, a low string chord struck hard and left ringing, a faint anvil on top.
    desk = B.Desk(None, tail=2.5)
    desk.track('drum', 0.0, 0.0, -12.0)
    desk.track('chord', -3.0, 0.0, -10.0, B.cello_body)
    desk.track('anvil', -14.0, 0.2 if v else -0.2, -6.0)
    desk.add('drum', B.big_drum(child(r, 'drum'), 1.0, f0=46.0 + 3.0 * v, t60=1.3), 0.0)
    for j, note in enumerate(['D2', 'A2', 'D3', 'F3' if v == 0 else 'Eb3']):
        desk.add('chord', B.bowed(note, 2.4, child(r, 'chord', j), 0.85 if j == 0 else 0.6, attack=0.04, release=1.6,
                                  vib=0.04, bright=0.55, kmax=30), 0.01 * j)
    desk.add('anvil', B.anvil(child(r, 'anvil'), 0.5, f0=780.0 + 40.0 * v), 0.02)
    return desk.mix(_reverb(r, 2.4), _bus)


@cue('Boss.Gravemother.Burrow', variations=2, att='Gun', cls='Effects', jitter=0.04, conc=1, level=-6.0)
def gravemother_burrow(v, r):
    # She dives: forelegs raking fast into the floor, the earth swallowing her with a deep thump and a suck of air, soil
    # pouring in after her, and the rumble of her going away under the ground.
    rr = child(r, 'rake')
    rake = None
    for i in range(5):
        d = 0.12
        stroke = _earth(d, child(rr, 'g', i), 400.0, 3200.0, 0.06, 0.002, 70.0, 0.8)
        rake = layers(rake, (stroke, 0.07 * i, -8.0 + 1.5 * i))
    swallow = layers(_pressure(child(r, 'push'), 0.012, 0.18, 160.0),
                     (kit.noise_thump(0.4, child(r, 'suck'), 500.0, 55.0, 0.2, 1.0, 0.12), 0.0, -2.0))
    pour = _earth(0.9, child(r, 'pour'), 180.0, 2200.0, 0.5, 0.02, 45.0, 0.85)
    lumps = _lumps(0.8, child(r, 'lumps'), 6 + v, 0.05, 0.6, 150.0, 420.0)
    away = _rumble(1.2, child(r, 'away'), 180.0, 0.9, 0.15, 4.0)
    x = layers((rake, 0.0, 0.0), (swallow, 0.35, 3.0), (_lv(pour, swallow, -6.0), 0.4, 0.0),
               (_lv(lumps, swallow, -10.0), 0.45, 0.0), (_lv(away, swallow, -4.0), 0.4, 0.0))
    x = F.filt(x, F.lowshelf(70.0, -2.0), F.hp(30.0), F.peak(3300.0, -3.0, 0.8), extend=False)
    return _master(_sink(x, r, -15.0, 0.9), 2.0, 1.0)


@cue('Boss.Gravemother.Emerge', variations=2, att='Gun', cls='Effects', jitter=0.03, conc=1, level=-3.0)
def gravemother_emerge(v, r):
    # She bursts out: a blast of packed earth thrown up (a shove of air, the crust cracking), soil spraying, clods raining
    # back down for a second, the Sink's cliffs answering. The spider's own climb plays on top: this is the ground.
    blast = layers(_pressure(child(r, 'blast'), 0.02, 0.35, 140.0),
                   (kit.noise_thump(0.5, child(r, 'thump'), 600.0, 45.0, 0.25, 1.0, 0.08), 0.0, -1.0))
    crust = _snaps(0.25, child(r, 'crust'), lambda t: 60.0 - 150.0 * t, 250.0, 2200.0, 0.03)
    spray = _earth(0.8, child(r, 'spray'), 220.0, 3000.0, 0.35, 0.004, 55.0, 0.85)
    rain = _lumps(1.6, child(r, 'rain'), 12 + 2 * v, 0.2, 1.2, 140.0, 480.0, 0.86)
    under = _rumble(1.6, child(r, 'under'), 140.0, 1.1, 0.06, 5.0)
    x = layers((blast, 0.0, 0.0), (_lv(normalize(crust), blast, -8.0), 0.0, 0.0), (_lv(spray, blast, -4.0), 0.01, 0.0),
               (_lv(rain, blast, -9.0), 0.0, 0.0), (_lv(under, blast, -5.0), 0.0, 0.0))
    x = F.filt(x, F.lowshelf(70.0, -2.0), F.hp(28.0), F.peak(3300.0, -3.0, 0.8), extend=False)
    return _master(_sink(x, r, -13.0, 1.2), 3.0, 1.5)
