"""Every cue's recipe, by group. A recipe is a function (v, r) -> samples, registered with @cue: v is the variation
(0, 1, ...) and r a random generator seeded by the cue's name and v, so every render gives the same files.

@cue's arguments become the cue's entry in cues.json (see Art/Sounds/README.md):
    variations  how many files (NN from 01)
    space       '3D' (mono, placed in the world) or '2D' (stereo allowed: the interface, the player's own feedback)
    cls         the sound class: 'Effects', 'Interface', 'Ambience' or 'Music'
    att         attenuation: 'Gun' (~60 m), 'Creature' (~35 m), 'Near' (~15 m) or 'None' (2D)
    jitter      pitch jitter (0.04 = up to 4% either way per play)
    conc        how many may play at once
    loop        a seamless loop (the harness doesn't trim or fade it)
    level       how loud it should sound against the rifle's shot (dB; the rifle is 0): the mix step turns it into
                volume_db from each cue's measured loudness
    align       match the variations' loudness (off when they differ on purpose)
    swell       it fades in on purpose (a riser, a ghost's wail): the check doesn't call its soft start late
"""
import importlib

REGISTRY = {}
GROUPS = ['guns', 'impacts', 'feedback', 'ui', 'loot', 'player', 'creatures', 'world']


class Cue:
    def __init__(self, name, fn, variations, space, cls, att, jitter, conc, loop, level, align, group, swell=False):
        self.swell = swell
        self.name = name
        self.fn = fn
        self.variations = variations
        self.space = space
        self.cls = cls
        self.att = att
        self.jitter = jitter
        self.conc = conc
        self.loop = loop
        self.level = level
        self.align = align
        self.group = group

    @property
    def stem(self):
        return self.name.replace('.', '_')

    def file(self, v):
        return f'{self.stem}_{v + 1:02d}.wav'

    def files(self):
        return [self.file(v) for v in range(self.variations)]


def cue(name, variations=3, space='3D', cls='Effects', att=None, jitter=0.03, conc=4, loop=False, level=-12.0,
        align=True, swell=False):
    if space not in ('2D', '3D'):
        raise ValueError(f'{name}: space must be 2D or 3D')
    if att is None:
        att = 'None' if space == '2D' else 'Near'
    if space == '2D':
        att = 'None'

    def register(fn):
        if name in REGISTRY:
            raise ValueError(f'{name} has two recipes')
        group = fn.__module__.rsplit('.', 1)[-1]
        REGISTRY[name] = Cue(name, fn, variations, space, cls, att, jitter, conc, loop, level, align, group, swell)
        return fn

    return register


def load_all():
    for g in GROUPS:
        try:
            importlib.import_module(f'recipes.{g}')
        except ModuleNotFoundError as e:
            # A group not written yet is skipped; anything else missing is a real error.
            if e.name != f'recipes.{g}':
                raise
    return REGISTRY
