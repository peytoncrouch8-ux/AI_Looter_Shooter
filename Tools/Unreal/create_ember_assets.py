"""Makes (or brings up to date) the ember powers' data assets in /Game/Data/Embers (Embers/EmberDefinition.h,
Embers/HeroDefinition.h): one UEmberDefinition per power and Ellis's UHeroDefinition listing his three. Run it in the open
editor once the C++ (the definitions and the three power classes) is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_ember_assets.py"
It prints an EMBERS line per asset and "EMBERS done". Running it again writes every field below again (id, words, colour,
icon, clocks, numbers, extras, power class; the hero's id, name and list) and leaves anything else as it is; other assets
are never touched. Ids never change once a session has saved one (the session keeps the equipped power by its id).

  DA_Ember_DustDevil     Dust Devil (Wind), UEmberDustDevil: a 9 m dash in 0.3 s, untouchable; creatures by the path
                         take 15, are flung 4-5 m aside and dizzy 1 s. Cooldown 7 s (the demo's numbers).
  DA_Ember_Sundown       Sundown (Dusk), UEmberSundown: 6 s of the world at a quarter speed, Ellis at full speed (their
                         bullets too). Cooldown 20 s (decided unattended, 2026-10-15 night; tuned in play).
  DA_Ember_BorrowedTime  Borrowed Time (Debt), UEmberBorrowedTime: 8 s of damage taken going into Sexton's ledger, paid
                         back over 3 s after; each kill strikes a quarter off. Cooldown 24 s (decided unattended).
  DA_Hero_Ellis          Ellis Ransom: the three in that order (the first is equipped in a new game).

What each number means to its power (UEmberDefinition's fields; each power class documents what it reads):
  Damage    against a level 1 rifle's 14 a shot, grown with the player's level (UEmberComponent::GetLevelScale).
  Range     cm: Dust Devil's dash length.
  Radius    cm: Dust Devil's reach either side of its path.
  Strength  a share: Sundown's world speed (0.25), Borrowed Time's share of the debt a kill strikes off (0.25).
  Extras    by name: Dust Devil's FlingMin/FlingMax (cm), Dizzy (s), FovKick (degrees); Borrowed Time's PayBackSeconds.
"""
import unreal

FOLDER = '/Game/Data/Embers'
SCRIPT = '/Script/AI_Looter_Shooter'

POWERS = [
    dict(
        asset='DA_Ember_DustDevil',
        id='DustDevil',
        name='Dust Devil',
        nature='Wind',
        description=("A dash of 9 meters in a third of a second, the way you're moving, or forward when standing still. "
                     "Nothing can hurt you during it. Creatures within 2.6 meters of your path take 15 damage, are flung "
                     "4 to 5 meters aside and stay dizzy for a second once they land."),
        color='#e8f6ff',
        icon='DustDevil',
        cooldown=7.0,
        duration=0.3,
        damage=15.0,
        range=900.0,
        radius=260.0,
        strength=0.0,
        extras={'FlingMin': 400.0, 'FlingMax': 500.0, 'Dizzy': 1.0, 'FovKick': 14.0},
        power_class='EmberDustDevil',
    ),
    dict(
        asset='DA_Ember_Sundown',
        id='Sundown',
        name='Sundown',
        nature='Dusk',
        description=("For 6 seconds the world slows to a quarter of its speed, the light going gold as at dusk, while you "
                     "move, aim and shoot at full speed, your bullets too."),
        color='#ffb24d',
        icon='Sundown',
        cooldown=20.0,
        duration=6.0,
        damage=0.0,
        range=0.0,
        radius=0.0,
        strength=0.25,
        extras={},
        power_class='EmberSundown',
    ),
    dict(
        asset='DA_Ember_BorrowedTime',
        id='BorrowedTime',
        name='Borrowed Time',
        nature='Debt',
        description=("For 8 seconds the damage you take goes into Sexton's ledger instead of your hide. When it ends the "
                     "debt comes due, paid back over 3 seconds, and every kill while it runs strikes a quarter of it off."),
        color='#c9b4ff',
        icon='BorrowedTime',
        cooldown=24.0,
        duration=8.0,
        damage=0.0,
        range=0.0,
        radius=0.0,
        strength=0.25,
        extras={'PayBackSeconds': 3.0},
        power_class='EmberBorrowedTime',
    ),
]

HEROES = [
    dict(asset='DA_Hero_Ellis', id='Ellis', name='Ellis Ransom', powers=['DA_Ember_DustDevil', 'DA_Ember_Sundown', 'DA_Ember_BorrowedTime']),
]


def need_class(name):
    cls = getattr(unreal, name, None)
    if cls is None:
        raise RuntimeError(f'unreal.{name} is missing: build the C++ first; nothing was changed')
    return cls


def load_or_create(asset_name, cls):
    path = f'{FOLDER}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not a {cls.__name__}: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def srgb_to_linear(channel):
    """One 0-255 sRGB channel as linear light, the way FLinearColor(FColor) converts it (LooterUI::Hex's colours)."""
    c = channel / 255.0
    return c / 12.92 if c <= 0.04045 else ((c + 0.055) / 1.055) ** 2.4


def linear_color(hex_text):
    h = hex_text.lstrip('#')
    r, g, b = (int(h[i:i + 2], 16) for i in (0, 2, 4))
    return unreal.LinearColor(srgb_to_linear(r), srgb_to_linear(g), srgb_to_linear(b), 1.0)


def save(asset, path):
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f'{path} could not be saved')


def setup_power(spec, cls):
    path = f"{FOLDER}/{spec['asset']}"
    asset, created = load_or_create(spec['asset'], cls)
    asset.set_editor_property('id', unreal.Name(spec['id']))
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    asset.set_editor_property('nature', unreal.Text(spec['nature']))
    asset.set_editor_property('description', unreal.Text(spec['description']))
    asset.set_editor_property('color', linear_color(spec['color']))
    asset.set_editor_property('icon_name', unreal.Name(spec['icon']))
    asset.set_editor_property('cooldown_seconds', spec['cooldown'])
    asset.set_editor_property('duration_seconds', spec['duration'])
    asset.set_editor_property('damage', spec['damage'])
    asset.set_editor_property('range', spec['range'])
    asset.set_editor_property('radius', spec['radius'])
    asset.set_editor_property('strength', spec['strength'])
    extras = unreal.Map(unreal.Name, float)
    for key, value in spec['extras'].items():
        extras[unreal.Name(key)] = float(value)
    asset.set_editor_property('extras', extras)
    # The power's C++ class by its path (the POWERS work: Embers/Powers/*). Missing, the asset is still written and the
    # game says the power can't be used (UEmberComponent logs it); run this again once the C++ is built.
    class_path = f"{SCRIPT}.{spec['power_class']}"
    power_class = unreal.load_class(None, class_path)
    if power_class is None:
        unreal.log_warning(f'EMBERS {path}: {class_path} is not a class yet; its power class is left empty')
    else:
        asset.set_editor_property('power_class', power_class)
    save(asset, path)
    unreal.log(f"EMBERS {path} {'made' if created else 'updated'}: {spec['name']} ({spec['nature']}), cooldown "
               f"{spec['cooldown']} s, duration {spec['duration']} s, {spec['color']}, "
               f"{class_path if power_class else 'no power class'}")
    return asset


def setup_hero(spec, cls, made):
    path = f"{FOLDER}/{spec['asset']}"
    powers = []
    for name in spec['powers']:
        power = made.get(name)
        if power is None:
            raise RuntimeError(f'{path}: its power {name} was not made: nothing was saved')
        powers.append(power)
    asset, created = load_or_create(spec['asset'], cls)
    asset.set_editor_property('id', unreal.Name(spec['id']))
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    asset.set_editor_property('powers', powers)
    save(asset, path)
    unreal.log(f"EMBERS {path} {'made' if created else 'updated'}: {spec['name']} with {', '.join(spec['powers'])}")


def run():
    ember_cls = need_class('EmberDefinition')
    hero_cls = need_class('HeroDefinition')
    made = {spec['asset']: setup_power(spec, ember_cls) for spec in POWERS}
    for spec in HEROES:
        setup_hero(spec, hero_cls, made)
    unreal.log('EMBERS done')


run()
