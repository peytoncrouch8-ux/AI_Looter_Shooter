"""Makes (or brings up to date) the placeholder creatures (Docs/Story.md, "Creature roster"; the user's ask, 2026-10-21:
"a placeholder blob of clay with the name of the enemy"): for each kind, in /Game/Data/Placeholders,
  DA_Placeholder_<Name>   its UPlaceholderCreatureDefinition (names, role, clay size and colour, numbers, shots),
  BP_Placeholder_<Name>   a data-only child of APlaceholderCreature naming it, its numbers written into its defaults
                          (APlaceholderCreature::BakeDefinition: what play, spawners and the bestiary read),
and its bestiary page /Game/Data/Bestiary/DA_Bestiary_<Name> (ActorClass the BP, SK_Slime's dome on the stand, named as
its tag is, "Name (plain type)", and a placeholder note). Run it in the open editor once the C++ is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_placeholder_creatures.py"           the Rest's kinds
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_placeholder_creatures.py all"       every kind
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_placeholder_creatures.py CoffinRat"  named ones
With no names it makes only the kinds placed somewhere now (PLACED: Ransom's Rest's, build_area_placeholders.py), so the
bestiary lists no silhouettes of creatures from regions not built yet; a later region adds its kinds to PLACED as they're
placed. Running it again writes every field again. It prints a PLACEHOLDER line per kind and "PLACEHOLDER done".
Numbers at level 1: role presets (ROLES) under each kind's own (calibrated on the Unpaid 160/8, the dead miner 180/9, the
slime 120/6, the Spitter 200/10; kill XP 10 a regular kind, the user's 2026-10-22 call: placeholders pay like real enemies).
Looter.Creatures.Placeholder.Pages checks what this writes.
"""
import sys

import unreal

FOLDER = '/Game/Data/Placeholders'
PAGES = '/Game/Data/Bestiary'
BLOB = '/Game/Art/Creatures/SK_Slime'
PLACEHOLDER_NOTE = 'Placeholder: a clay stand-in until its model is made (Docs/Story.md, "Creature roster").'
# After the real pages in each section.
SORT_FIRST = 100

# The kinds placed in the game now: made with no names given.
PLACED = ['CoffinRat', 'BoneHound', 'Knocker', 'Gravepicker', 'GravepickerLookout']

# Role presets: every number a kind doesn't give (health, attack, kill XP at level 1; cm and cm/s; seconds).
ROLES = {
    'Melee': dict(max_health=160.0, attack_damage=8.0, xp_reward=10, walk_speed=180.0, chase_speed=430.0, attack_range=220.0,
                  attack_windup=0.6, attack_recovery=0.5, attack_cooldown=1.2, aggro_radius=2600.0, pack_alert_radius=1500.0,
                  strike_strength=1.0),
    'Swarm': dict(max_health=50.0, attack_damage=4.0, xp_reward=4, walk_speed=220.0, chase_speed=520.0, attack_range=160.0,
                  attack_windup=0.4, attack_recovery=0.35, attack_cooldown=0.9, aggro_radius=2200.0, pack_alert_radius=2200.0,
                  strike_strength=0.7),
    'Ranged': dict(max_health=130.0, attack_damage=8.0, xp_reward=10, walk_speed=170.0, chase_speed=380.0, attack_range=220.0,
                   attack_windup=0.85, attack_recovery=0.5, attack_cooldown=2.2, aggro_radius=3000.0, pack_alert_radius=1800.0,
                   strike_strength=1.0, shot_range=2400.0, close_in_to=1500.0, back_off_within=800.0, shot_speed=2800.0,
                   shot_radius=8.0, shot_count=1, shot_spread=0.0, shot_damage_share=1.0, lead_share=0.4),
    'Big': dict(max_health=600.0, attack_damage=18.0, xp_reward=30, walk_speed=120.0, chase_speed=300.0, attack_range=330.0,
                attack_windup=1.1, attack_recovery=1.0, attack_cooldown=2.4, aggro_radius=2800.0, pack_alert_radius=1200.0,
                strike_strength=1.6),
    'Flyer': dict(max_health=110.0, attack_damage=7.0, xp_reward=9, walk_speed=260.0, chase_speed=560.0, attack_range=210.0,
                  attack_windup=0.7, attack_recovery=0.6, attack_cooldown=1.6, aggro_radius=3200.0, pack_alert_radius=2000.0,
                  strike_strength=0.9, hover_height=170.0),
}

# Each kind: its name and plain type (the tag's "Name (type)"), role, borrowed voice, bestiary section, body (length, width,
# height cm), head (size cm, height share, forward share), clay colour (sRGB hex), its own numbers over the role's, and its
# page's words. 'where' is the roster's region(s) (Docs/Story.md); the Rest's kinds also say where they stand there.
KINDS = {
    # --- Ransom's Rest (placed: build_area_placeholders.py) ---
    'CoffinRat': dict(
        name='Coffin Rat', type='rat', role='Swarm', voice='Beast', section='CREATURE',
        body=(70, 35, 30), head=(22, 0.6, 0.85), color='6B5A4E',
        numbers=dict(max_health=45.0, attack_damage=4.0, xp_reward=3, chase_speed=560.0, pack_tag='CoffinRat'),
        where="Ransom's Rest: Bright & Daughter's cellar, the depot's baggage room",
        description=("Rats that live off the dead. A body waits seven days in its box before the boards, and since the "
                     "keeper was shot they wait longer, so the rats have grown fat and bold under the undertaker's floor "
                     "and among the coffins bound for Gravesend. They come in a rush and scatter as fast."),
        notes=['They come in numbers and die to anything: sweep them, don\'t aim.',
               'The head, low at the front, is the weak spot.']),
    'BoneHound': dict(
        name='Bone Hound', type='coyote', role='Melee', voice='Beast', section='CREATURE',
        body=(130, 50, 80), head=(34, 0.8, 0.9), color='C9B99A',
        numbers=dict(max_health=140.0, attack_damage=9.0, walk_speed=220.0, chase_speed=600.0, pack_alert_radius=3000.0,
                     attack_windup=0.5, pack_tag='BoneHound'),
        where="Ransom's Rest: the keeper's road, between the cairns and the old lantern house",
        description=("Coyotes that raid the burial boards on nights when no keeper sits up. With the keepers shot all over "
                     "the Reaches there are more of them every month, and the Rest's boards have had no keeper since "
                     "Abel fell. They hunt in packs and they're fast."),
        notes=['Hurt one and the whole pack turns on you.', 'Faster than you: stand and shoot, don\'t run.',
               'The head is the weak spot.']),
    'Knocker': dict(
        name='Knocker', type='goblin', role='Swarm', voice='Dead', section='CREATURE',
        body=(45, 40, 95), head=(30, 0.85, 0.2), color='5C5A55',
        numbers=dict(max_health=70.0, attack_damage=5.0, xp_reward=5, chase_speed=500.0, pack_tag='Knocker'),
        where="Ransom's Rest: Larkspur Bench, round the ore cars off the tramway's rails",
        description=("Miners' folk tell of the knockers, little grey people of the rock who knock on the timbers before a "
                     "fall. They knocked all the night before Teague sent the eleven down. Nobody listened. Since the saint "
                     "went dark they've stopped warning anyone, and they come out of the workings at dusk to pick at "
                     "the ore the dead left in the cars."),
        notes=['Small and quick, in a crowd: keep them in front of you.', 'The head is the weak spot.']),
    'Gravepicker': dict(
        name='Gravepicker', type='bandit', role='Melee', voice='Clay', section='ENEMY',
        body=(60, 55, 175), head=(30, 0.93, 0.15), color='8A7356',
        numbers=dict(max_health=170.0, attack_damage=10.0, xp_reward=12, pack_alert_radius=2500.0, pack_tag='Gravepicker'),
        where="Ransom's Rest: Boot Hill, after Cold Welcome and before Hallowed Ground",
        description=("Grave robbers, living ones, up through Stage Gap on the rumour that the Rest's old graves hold grave "
                     "gold and that nobody is keeping them now. A polite buyer pays well for the old coins. They dig with "
                     "shovels and fight with them too."),
        notes=['Living men: they bleed, and they run when the dead walk Boot Hill.', 'The head is the weak spot.']),
    'GravepickerLookout': dict(
        name='Gravepicker Lookout', type='bandit rifleman', role='Ranged', voice='Clay', section='ENEMY',
        body=(60, 55, 178), head=(30, 0.93, 0.15), color='6E5B44',
        numbers=dict(max_health=120.0, attack_damage=9.0, xp_reward=12, attack_windup=0.9, attack_cooldown=2.4,
                     shot_range=2600.0, close_in_to=1600.0, back_off_within=900.0, shot_speed=3200.0, shot_radius=7.0,
                     pack_alert_radius=2500.0, pack_tag='Gravepicker', shot_color='FFC27A'),
        where="Ransom's Rest: Boot Hill, keeping watch over the diggers",
        description=("The diggers' lookouts, with rifles, kept back where they can see the road. They shoot at whatever "
                     "comes up the hill, the living and the walking dead alike."),
        notes=['A glow at the muzzle is the tell: step behind something.', 'Close the distance and it backs off; corner it.',
               'The head is the weak spot.']),

    # --- Later regions (made with "all"; placed when their regions are built) ---
    'MesaStinger': dict(
        name='Mesa Stinger', type='scorpion', role='Melee', voice='Beast', section='CREATURE',
        body=(150, 100, 60), head=(30, 0.55, 0.95), color='B08A57',
        numbers=dict(max_health=220.0, attack_damage=11.0, xp_reward=12),
        where='Dustwater Mesa: the dry cistern and the wind-cut ledges',
        description=('Scorpions as long as a man, nesting in the cistern Saint Ezra dug, dry since the saint went dark.'),
        notes=['The tail strikes over its back: watch it rise.']),
    'GloamBat': dict(
        name='Gloam Bat', type='giant bat', role='Flyer', voice='Beast', section='CREATURE',
        body=(70, 130, 50), head=(26, 0.6, 0.7), color='3E3540',
        where='The Needles: the spire caves; Gravesend: the mausoleums',
        description=('Bats with a wingspan wider than a door, roosting where the Gravewind never reaches. They drink the '
                     'soul-light off the dying.'),
        notes=['It dives on its strike: shoot it on the way down.']),
    'Sallow': dict(
        name='Sallow', type='vampire', role='Melee', voice='Dead', section='ENEMY',
        body=(55, 50, 185), head=(28, 0.93, 0.15), color='9A9088',
        numbers=dict(max_health=190.0, attack_damage=11.0, chase_speed=520.0, xp_reward=14),
        where="Saint Agnes Mire: Constance's night congregation; Hanging Moon nights",
        description=('Living folk who drink the coals out of the Unpaid to stay young. Constance calls it communion. Pale, '
                     'quick and old, and the dawn wind frightens them.'),
        notes=['Fast and close: keep moving.']),
    'Slagback': dict(
        name='Slagback', type='golem', role='Big', voice='Clay', section='CREATURE',
        body=(150, 120, 240), head=(44, 0.92, 0.2), color='4A3F3A',
        where="Furnace Hollow: the slag rivers under Barrels's forge",
        description=("Slag poured out of Saint Maud's forge and stood up by Barrels's ember. It walks while it's hot."),
        notes=['Slow, and it hits like a falling crane.']),
    'CairnWalker': dict(
        name='Cairn Walker', type='stone golem', role='Big', voice='Clay', section='CREATURE',
        body=(160, 130, 260), head=(46, 0.92, 0.1), color='8F8B84',
        where="Stoneweight Quarry: the cut-stone terraces",
        description=("Cut blocks piled into the shape of a man by Tobias's ember of burden, heavy enough to walk."),
        notes=['Slow; the blow throws you.']),
    'Pallbearer': dict(
        name='Pallbearer', type='giant', role='Big', voice='Dead', section='ENEMY',
        body=(160, 130, 420), head=(56, 0.94, 0.1), color='3B3A3A',
        numbers=dict(max_health=900.0, attack_damage=22.0, xp_reward=40),
        where="Hoarfrost Pass: the Chainman's columns; Gravesend",
        description=("Giants in mourning black who carry the unclaimed dead to the Toll Gate, older than the saints. "
                     "Sexton's oldest servants."),
        notes=['Its coffin is its shield: get round it.']),
    'WhitebackWolf': dict(
        name='Whiteback Wolf', type='wolf', role='Melee', voice='Beast', section='CREATURE',
        body=(140, 55, 90), head=(36, 0.8, 0.9), color='D8D6CF',
        numbers=dict(chase_speed=620.0, pack_alert_radius=3200.0, pack_tag='WhitebackWolf'),
        where="Hoarfrost Pass: following the Chainman's columns through the snow",
        description=('Snow wolves that follow the chained dead up the pass and take the ones who fall.'),
        notes=['In packs; they circle.']),
    'CinderDrake': dict(
        name='Cinder Drake', type='dragon', role='Flyer', voice='Beast', section='CREATURE',
        body=(160, 200, 80), head=(40, 0.6, 0.9), color='2E2724',
        numbers=dict(max_health=420.0, attack_damage=14.0, xp_reward=25, hover_height=260.0),
        where='Furnace Hollow: the chimneys and the blast furnace',
        description=('Small dragons of slag-glass hatched in the forge where an ember spark fell. Their mother is still in '
                     'the blast furnace.'),
        notes=['It dives; the glow in its throat is the tell.']),
    'SextonBeetle': dict(
        name='Sexton Beetle', type='giant insect', role='Big', voice='Clay', section='CREATURE',
        body=(210, 140, 110), head=(36, 0.5, 0.95), color='2F3A2C',
        numbers=dict(max_health=480.0, attack_damage=14.0, xp_reward=24),
        where='Gravesend: the bone gardens; Saint Agnes Mire',
        description=('Burying beetles the size of a cart that drag the unclaimed dead underground. The towns named them for '
                     'the gravedigger.'),
        notes=['Armoured on top: shoot the head.']),
    'BogMantis': dict(
        name='Bog Mantis', type='giant insect', role='Melee', voice='Clay', section='CREATURE',
        body=(90, 60, 200), head=(28, 0.95, 0.3), color='5E6B3A',
        where='Saint Agnes Mire: the reeds and the drowned mission',
        description=('A mantis taller than a man that waits in the reeds as still as a cross.'),
        notes=['It strikes fast from still: keep your distance.']),
    'RimrockRattler': dict(
        name='Rimrock Rattler', type='giant snake', role='Melee', voice='Beast', section='CREATURE',
        body=(280, 45, 45), head=(30, 0.7, 1.0), color='8C6D4A',
        where='Dustwater Mesa; the Needles',
        description=('A rattlesnake as long as a wagon. You hear it before you see it.'),
        notes=['The rattle is the tell.']),
    'DustWight': dict(
        name='Dust Wight', type='wind spirit', role='Ranged', voice='Dead', section='ENEMY',
        body=(70, 70, 190), head=(30, 0.9, 0.0), color='C2A77F',
        numbers=dict(shot_count=3, shot_spread=10.0, shot_color='E8D3A6'),
        where="Dustwater Mesa: inside Ira's dust storm",
        description=("The dead of Dustwater caught in Ira's wind, throwing grit hard enough to cut."),
        notes=['It throws a spread: sidestep.']),
    'Strawman': dict(
        name='Strawman', type='scarecrow', role='Melee', voice='Dead', section='ENEMY',
        body=(70, 50, 190), head=(32, 0.93, 0.1), color='B89B5E',
        where="Ransom's Rest: Hanging Moon nights in the orchard (endgame)",
        description=('On a Hanging Moon the wind doesn\'t blow and the dead walk the orchards; some of them wear the '
                     'scarecrows.'),
        notes=['Slow until it isn\'t.']),
    'Spikedriver': dict(
        name='Spikedriver', type='clockwork golem', role='Big', voice='Clay', section='ENEMY',
        body=(140, 120, 230), head=(40, 0.9, 0.2), color='5B5E61',
        where='Halfway Junction: the mail-train wreck and the yards',
        description=("The railroad's clockwork track-layers, broken in the wreck and woken by the same ember sparks that "
                     "woke Gauge, without its mind."),
        notes=['Heavy blows; the boiler on its back is hot.']),
    # The story's own creatures with no model yet (Docs/Story.md, "Enemies and creatures"): placeholders too.
    'CarrionGull': dict(
        name='Carrion Gull', type='gull', role='Flyer', voice='Beast', section='CREATURE',
        body=(50, 90, 35), head=(20, 0.6, 0.8), color='DADAD4',
        numbers=dict(max_health=60.0, attack_damage=5.0, xp_reward=5, hover_height=200.0, pack_tag='CarrionGull'),
        where='Highwater Landing and the canyon; Gravesend',
        description=('Gulls that follow the dead, out over the canyon cloud and round the hearse cars.'),
        notes=['In flocks; they dive one at a time.']),
    'MossBack': dict(
        name='MossBack', type='swamp turtle', role='Big', voice='Clay', section='CREATURE',
        body=(220, 180, 110), head=(40, 0.45, 1.0), color='4C5B36',
        where='Saint Agnes Mire', description=('Old snapping turtles grown huge on the drowned dead, moss on their backs.'),
        notes=['The shell takes nothing: the head.']),
    'EmberTarantula': dict(
        name='Ember Tarantula', type='spider', role='Melee', voice='Beast', section='CREATURE',
        body=(120, 110, 60), head=(28, 0.55, 0.9), color='7A2E1A',
        where="Furnace Hollow: the slag", description=('Tarantulas that live in the warm slag and glow like coals.'),
        notes=['It burns where it bites.']),
    'CliffHuntsman': dict(
        name='Cliff Huntsman', type='spider', role='Melee', voice='Beast', section='CREATURE',
        body=(150, 150, 50), head=(26, 0.6, 0.9), color='6D6458',
        numbers=dict(chase_speed=620.0),
        where='The Needles: the spire faces', description=('Flat, fast spiders that run on the spires\' faces.'),
        notes=['Fast: let it come to you.']),
    'SkyJumper': dict(
        name='Sky Jumper', type='jumping spider', role='Flyer', voice='Beast', section='CREATURE',
        body=(80, 80, 50), head=(26, 0.6, 0.9), color='3F4A5A',
        numbers=dict(hover_height=140.0),
        where="The Gilded Lily's rigging; Skyreach",
        description=("Jumping spiders that live in airships' rigging and leap from line to line."),
        notes=['It leaps at you: shoot it in the air.']),
}


def linear(hex_color):
    """An sRGB hex colour as a linear one."""
    def channel(value):
        c = value / 255.0
        return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4
    r, g, b = (int(hex_color[i:i + 2], 16) for i in (0, 2, 4))
    return unreal.LinearColor(channel(r), channel(g), channel(b), 1.0)


def tag_name(spec):
    return f"{spec['name']} ({spec['type']})"


def classes():
    definition = getattr(unreal, 'PlaceholderCreatureDefinition', None)
    creature = getattr(unreal, 'PlaceholderCreature', None)
    entry = getattr(unreal, 'BestiaryEntry', None)
    if definition is None or creature is None or entry is None:
        raise RuntimeError('unreal.PlaceholderCreatureDefinition, PlaceholderCreature or BestiaryEntry is missing: build the C++ '
                           'first; nothing was changed')
    return definition, creature, entry


def load_or_create_data(folder, asset_name, cls):
    path = f'{folder}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, folder, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def write_definition(key, spec, cls):
    asset, created = load_or_create_data(FOLDER, f'DA_Placeholder_{key}', cls)
    numbers = dict(ROLES[spec['role']])
    numbers.update(spec.get('numbers', {}))
    length, width, height = spec['body']
    head_size, head_height, head_forward = spec['head']
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    asset.set_editor_property('plain_type', unreal.Text(spec['type']))
    asset.set_editor_property('role', getattr(unreal.PlaceholderRole, spec['role'].upper()))
    asset.set_editor_property('voice', getattr(unreal.PlaceholderVoice, spec['voice'].upper()))
    for prop, value in (('length', length), ('width', width), ('height', height), ('head_size', head_size),
                        ('head_height', head_height), ('head_forward', head_forward)):
        asset.set_editor_property(prop, float(value))
    asset.set_editor_property('clay_color', linear(spec['color']))
    for prop, value in numbers.items():
        if prop == 'pack_tag':
            asset.set_editor_property(prop, unreal.Name(value))
        elif prop == 'shot_color':
            asset.set_editor_property(prop, linear(value))
        elif prop in ('xp_reward', 'shot_count'):
            asset.set_editor_property(prop, int(value))
        else:
            asset.set_editor_property(prop, float(value))
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f'{asset.get_path_name()} could not be saved')
    return asset, created


def generated_class(blueprint):
    try:
        return unreal.BlueprintEditorLibrary.generated_class(blueprint)
    except Exception:
        return blueprint.generated_class()


def write_blueprint(key, definition, creature_cls):
    name = f'BP_Placeholder_{key}'
    path = f'{FOLDER}/{name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        blueprint = unreal.load_asset(path)
    else:
        factory = unreal.BlueprintFactory()
        factory.set_editor_property('parent_class', creature_cls)
        blueprint = unreal.AssetToolsHelpers.get_asset_tools().create_asset(name, FOLDER, unreal.Blueprint, factory)
        if blueprint is None:
            raise RuntimeError(f'{path} could not be created')
        unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    defaults = unreal.get_default_object(generated_class(blueprint))
    defaults.set_editor_property('definition', definition)
    # Its numbers into its defaults (C++: one mapping, which Looter.Creatures.Placeholder.Pages checks).
    if not creature_cls.bake_definition(defaults):
        raise RuntimeError(f'{path}: its definition could not be written into it')
    blueprint.modify()
    if not unreal.EditorAssetLibrary.save_loaded_asset(blueprint, only_if_is_dirty=False):
        raise RuntimeError(f'{path} could not be saved')
    return f'{path}.{name}_C'


def write_page(key, spec, class_path, entry_cls, order):
    if not unreal.EditorAssetLibrary.does_asset_exist(BLOB):
        unreal.log_warning(f'PLACEHOLDER {key}: no {BLOB} for the stand: its page waits')
        return
    page, created = load_or_create_data(PAGES, f'DA_Bestiary_{key}', entry_cls)
    page.set_editor_property('display_name', unreal.Text(tag_name(spec)))
    page.set_editor_property('category', getattr(unreal.BestiaryCategory, spec['section']))
    page.set_editor_property('kind', unreal.Text(f"{spec['type'][0].upper()}{spec['type'][1:]} (placeholder)"))
    page.set_editor_property('description', unreal.Text(spec['description']))
    page.set_editor_property('habitat', unreal.Text(spec.get('where', '')))
    page.set_editor_property('notes', [unreal.Text(note) for note in spec.get('notes', [])] + [unreal.Text(PLACEHOLDER_NOTE)])
    page.set_editor_property('sort_order', SORT_FIRST + order)
    page.set_editor_property('page', unreal.BestiaryPage.ACTOR)
    page.set_editor_property('ledger_only', False)
    page.set_editor_property('known_when', unreal.StoryCondition())
    page.set_editor_property('found_when', unreal.StoryCondition())
    try:
        page.set_editor_property('actor_class', unreal.SoftClassPath(class_path))
    except TypeError:
        page.set_editor_property('actor_class', unreal.load_class(None, class_path))
    try:
        page.set_editor_property('preview_mesh', unreal.SoftObjectPath(f'{BLOB}.SK_Slime'))
    except TypeError:
        page.set_editor_property('preview_mesh', unreal.load_asset(BLOB))
    if not unreal.EditorAssetLibrary.save_loaded_asset(page, only_if_is_dirty=False):
        raise RuntimeError(f'{page.get_path_name()} could not be saved')
    return created


def run():
    definition_cls, creature_cls, entry_cls = classes()
    asked = [name for name in sys.argv[1:] if name in KINDS]
    if 'all' in sys.argv[1:]:
        wanted = list(KINDS)
    else:
        wanted = asked or list(PLACED)
    unknown = [name for name in sys.argv[1:] if name not in KINDS and name != 'all']
    if unknown:
        unreal.log_warning(f'PLACEHOLDER no kinds called {", ".join(unknown)} (KINDS has {", ".join(KINDS)})')
    for key in wanted:
        spec = KINDS[key]
        definition, made = write_definition(key, spec, definition_cls)
        class_path = write_blueprint(key, definition, creature_cls)
        page_made = write_page(key, spec, class_path, entry_cls, list(KINDS).index(key))
        unreal.log(f'PLACEHOLDER {key}: "{tag_name(spec)}" {spec["role"]}, {"made" if made else "updated"}; class {class_path}; '
                   f'page DA_Bestiary_{key} {"made" if page_made else "updated"}; {spec.get("where", "")}')
    unreal.log(f'PLACEHOLDER done: {len(wanted)} kinds')


run()
