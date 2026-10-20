"""Makes (or brings up to date) the ember powers' and the heroes' data assets in /Game/Data/Embers (Embers/EmberDefinition.h,
Embers/HeroDefinition.h): one UEmberDefinition per power (fifteen, three per hero) and one UHeroDefinition per hero (five)
with their words, colour, portrait, body and three powers. Run it in the open editor once the C++ (the definitions and the
power classes that exist) is built, and after Tools/Unreal/import_hero_portraits.py for the portraits:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_ember_assets.py"
It prints an EMBERS line per asset and "EMBERS done". Running it again writes every field below again (id, words, colour,
icon, clocks, numbers, extras, power class when it has one; the hero's id, name, role, story, colour, the portrait, mesh
and retargeter paths, and the list) and leaves anything else as it is; other assets are never touched. Ids never change
once a session has saved one (the session keeps the equipped power and the hero by their ids).

Ellis's three (the first round, 2026-10-15; code in Embers/Powers):
  DA_Ember_DustDevil     Dust Devil (Wind), UEmberDustDevil: a 9 m dash in 0.3 s, untouchable; creatures by the path
                         take 15, are flung 4-5 m aside and dizzy 1 s. Cooldown 7 s (the demo's numbers).
  DA_Ember_Sundown       Sundown (Dusk), UEmberSundown: 6 s of the world at a quarter speed, Ellis at full speed (their
                         bullets too). Cooldown 20 s (decided unattended, 2026-10-15 night; tuned in play).
  DA_Ember_BorrowedTime  Borrowed Time (Debt), UEmberBorrowedTime: 8 s of damage taken going into Sexton's ledger, paid
                         back over 3 s after; each kill strikes a quarter off. Cooldown 24 s (decided unattended).
The other twelve (the hero pick, 2026-10-19; their code 2026-10-20, Embers/Powers/Ember<Id>, each class's comment says
what it reads): words, nature, colour, icon (Art/Icons/InkedIcons.py) and numbers, the demo's from
Docs/Handoffs/CloudIslandConcepts_2026-10-05.md ("Ember powers", "Five heroes") brought to today's game: the demo's damage
was set against a 14 rifle round, today's level 1 rifle deals 20, so damage is x1.43 unless an entry says otherwise (each
entry's comment gives its numbers and why). Starting points: tuned in play. A hero is playable once their body and
retargeter are in the game too (UHeroDefinition::IsPlayable). power_class None leaves the field as it is (a class set by
hand survives), icon '' leaves none.

What each number means to its power (UEmberDefinition's fields; each power class documents what it reads):
  Damage    against a level 1 rifle's 20 a shot (the demo's was 14), grown with the player's level
            (UEmberComponent::GetLevelScale).
  Range     cm: a dash's length, a hook's or a lob's reach, a mark's range, a wire's length.
  Radius    cm: the reach either side of a path, a burst's or a light's radius, a line's half width.
  Strength  a share or a rate: Sundown's world speed (0.25), Borrowed Time's share struck off a kill (0.25), Double or
            Nothing's multiplier (2), Grave Ward's slow (0.5: half speed), Iron Hide's damage cut (0.6), Spyglass's extra
            damage taken (0.3), Long Shot's bonus per 10 m (0.15).
  Extras    by name: Dust Devil's FlingMin/FlingMax (cm), Dizzy (s), FovKick (degrees); Borrowed Time's PayBackSeconds;
            the rest as each entry says.

Heroes (UHeroDefinition): the roster's five (Ellis, Odessa, Crane, Gauge, Pike; UHeroDefinition::RosterIds), each with
role, short name (the id: what a caption calls them), story (the concept doc's lines; Ellis is "they" in all text), colour (the contract's), the portrait
/Game/UI/Portraits/T_Portrait_<Id>, the body /Game/Art/Characters/SK_<Id> and its retargeter
/Game/Player/Retarget/RTG_MannyTo<Id>, the first-person arms /Game/Art/Characters/SK_FP_Arms_<Id> (Ellis's SK_FP_Arms)
(each field set once its asset exists; the game finds a missing one by the same name, so a hero lights up as Main lands
their model; rerun this after each to fill the fields), and the arms' size against the guns (ARMS_SCALE).
"""
import unreal

FOLDER = '/Game/Data/Embers'
SCRIPT = '/Script/AI_Looter_Shooter'


def power(asset, id, name, nature, description, color, cooldown, duration, damage=0.0, range=0.0, radius=0.0,
          strength=0.0, extras=None, icon='', power_class=None):
    return dict(asset=asset, id=id, name=name, nature=nature, description=description, color=color, icon=icon,
                cooldown=cooldown, duration=duration, damage=damage, range=range, radius=radius, strength=strength,
                extras=extras or {}, power_class=power_class)


POWERS = [
    # --- Ellis, the Revenant ---
    power('DA_Ember_DustDevil', 'DustDevil', 'Dust Devil', 'Wind',
          "A dash of 9 meters in a third of a second, the way you're moving, or forward when standing still. "
          "Nothing can hurt you during it. Creatures within 2.6 meters of your path take 15 damage, are flung "
          "4 to 5 meters aside and stay dizzy for a second once they land.",
          '#e8f6ff', 7.0, 0.3, damage=15.0, range=900.0, radius=260.0,
          extras={'FlingMin': 400.0, 'FlingMax': 500.0, 'Dizzy': 1.0, 'FovKick': 14.0},
          icon='DustDevil', power_class='EmberDustDevil'),
    power('DA_Ember_Sundown', 'Sundown', 'Sundown', 'Dusk',
          "For 6 seconds the world slows to a quarter of its speed, the light going gold as at dusk, while you "
          "move, aim and shoot at full speed, your bullets too.",
          '#ffb24d', 20.0, 6.0, strength=0.25, icon='Sundown', power_class='EmberSundown'),
    power('DA_Ember_BorrowedTime', 'BorrowedTime', 'Borrowed Time', 'Debt',
          "For 8 seconds the damage you take goes into Sexton's ledger instead of your hide. When it ends the "
          "debt comes due, paid back over 3 seconds, and every kill while it runs strikes a quarter of it off.",
          '#c9b4ff', 24.0, 8.0, strength=0.25, extras={'PayBackSeconds': 3.0},
          icon='BorrowedTime', power_class='EmberBorrowedTime'),

    # --- Odessa Lark, the Cardsharp ---
    # The demo's 8 s and 12 m; the ricochet 14 (the demo's 10 against its 14 rifle: about 0.7 of a round, kept against
    # today's 20); every third pull critical.
    power('DA_Ember_LuckyStreak', 'LuckyStreak', 'Lucky Streak', 'Fortune',
          "For 8 seconds every shot that hits ricochets into the nearest other creature within 12 meters for 14 damage, "
          "and every third shot is a critical hit wherever it lands.",
          '#ffcf4a', 22.0, 8.0, damage=14.0, range=1200.0, extras={'CritEvery': 3.0},
          icon='LuckyStreak', power_class='EmberLuckyStreak'),
    # 52 cards in a fan 35 degrees wide and 12 tall, to 14 m, 8 each (the demo-era 6 against a 14 rifle, x1.43): a fan
    # point blank lands 15 to 25 cards on a spider; at 10 m a handful.
    power('DA_Ember_FiftyTwoPickup', 'FiftyTwoPickup', 'Fifty-Two Pickup', 'Fortune',
          "You fling the whole deck: fifty-two razor cards fan out up to 14 meters, each cutting whatever it meets for "
          "8 damage.",
          '#ff5d6c', 14.0, 0.0, damage=8.0, range=1400.0, extras={'Cards': 52.0, 'ConeDegrees': 35.0, 'FanTall': 12.0},
          icon='FiftyTwoPickup', power_class='EmberFiftyTwoPickup'),
    # Even odds at double pay nothing on average; never two misses running (MissStreak 1) makes it about 4 in 3 of her
    # damage, so it earns its slot beside Lucky Streak; cooldown 15 (from 18).
    power('DA_Ember_DoubleOrNothing', 'DoubleOrNothing', 'Double or Nothing', 'Risk',
          "For 6 seconds every shot you fire deals double damage or misses entirely, at even odds. Luck never leaves "
          "you twice running: after a miss, the next shot pays double.",
          '#7be0a0', 15.0, 6.0, strength=2.0, extras={'Odds': 0.5, 'MissStreak': 1.0},
          icon='DoubleOrNothing', power_class='EmberDoubleOrNothing'),

    # --- Hollis Crane, the Unpaid ---
    # An 18 m hook for 20 (one rifle round: a single target, against Dust Devil's 15 to everything by its path), stunned
    # 1.5 s; the duration is the throw and the haul.
    power('DA_Ember_TollChain', 'TollChain', 'Toll Chain', 'Debt',
          "Your chain hooks the creature in your sights up to 18 meters away and drags it to your feet for 20 damage, "
          "stunned for a second and a half.",
          '#d6a35c', 10.0, 0.5, damage=20.0, range=1800.0, extras={'Stun': 1.5},
          icon='TollChain', power_class='EmberTollChain'),
    # A 5 m light for 8 s, creatures at half speed, 5 health a second inside (40 over the 8 s: from 4, so it weighs
    # against Borrowed Time's 8 s of no damage at the same cooldown, though only while he stands in it).
    power('DA_Ember_GraveWard', 'GraveWard', 'Grave Ward', 'Mercy',
          "You plant your lantern. For 8 seconds creatures in its light, 5 meters round, move at half speed, and you "
          "mend 5 health a second while you stand in it.",
          '#9ff0d0', 24.0, 8.0, radius=500.0, strength=0.5, extras={'HealPerSecond': 5.0},
          icon='GraveWard', power_class='EmberGraveWard'),
    # A 40 m mark for 8 s; its soul bursts for 60 within 4.5 m, or the mark strikes for 90 (the demo-era 30 and 45 x1.43,
    # then up: both pay only after a wait or a kill; the strike is about a third of a spider).
    power('DA_Ember_LastRites', 'LastRites', 'Last Rites', 'Judgment',
          "You mark the creature in your sights. If it dies within 8 seconds its soul bursts on the creatures within "
          "4.5 meters for 60 damage; if it lives, the mark strikes it for 90.",
          '#e9e4ff', 16.0, 8.0, damage=60.0, range=4000.0, radius=450.0, extras={'Strike': 90.0},
          icon='LastRites', power_class='EmberLastRites'),

    # --- Gauge, the Iron Hand ---
    # The demo's lob x1.43 (its rifle 14, today's 20): 48 within 2.5 m, 26 within 4.5 m, a 4.5 m pool for 6 s burning
    # 10 every 0.5 s; the duration 7 s is the flight (under a second) and the pool.
    power('DA_Ember_SlagBomb', 'SlagBomb', 'Slag Bomb', 'Forge',
          "A bomb lobbed from the firebox onto the creature in your sights, or the ground there, up to 20 meters. It "
          "bursts for 48 within 2.5 meters and 26 within 4.5, and leaves a pool of slag for 6 seconds that burns "
          "whatever stands in it for 10 every half second.",
          '#ff8a1c', 12.0, 7.0, damage=48.0, range=2000.0, radius=450.0,
          extras={'InnerRadius': 250.0, 'OuterDamage': 26.0, 'BurnDamage': 10.0, 'BurnInterval': 0.5, 'BurnLinger': 0.6,
                  'PoolSeconds': 6.0},
          icon='SlagBomb', power_class='EmberSlagBomb'),
    # 60% less damage for 6 s at three-quarter pace; a biter burns for 12 (8 x1.43).
    power('DA_Ember_IronHide', 'IronHide', 'Iron Hide', 'Forge',
          "Your plates lock: for 6 seconds you take 60% less damage and move at three-quarter pace, and whatever "
          "bites you burns its teeth for 12 damage.",
          '#b8c4cf', 20.0, 6.0, strength=0.6, extras={'Pace': 0.75, 'BiteBurn': 12.0},
          icon='IronHide', power_class='EmberIronHide'),
    # A 25 m line, 60 to each creature on it (40 x1.43), each driven back 1.5 m and dazed a moment.
    power('DA_Ember_RailSpike', 'RailSpike', 'Rail Spike', 'Steam',
          "A spike driven from your forearm on a burst of steam, through every creature in a line 25 meters long, "
          "for 60 damage each, driving each one back.",
          '#a8dcff', 9.0, 0.0, damage=60.0, range=2500.0, radius=60.0, extras={'Shove': 150.0, 'Dizzy': 0.3},
          icon='RailSpike', power_class='EmberRailSpike'),

    # --- Wendell Pike, the Surveyor ---
    # The demo's numbers: 10 s, every creature within 60 m marked through walls, 30% more damage taken.
    power('DA_Ember_Spyglass', 'Spyglass', 'Spyglass', 'Sight',
          "For 10 seconds every creature within 60 meters is marked, even through walls, with its distance, and "
          "marked creatures take 30% more damage.",
          '#ffb24d', 18.0, 10.0, range=6000.0, strength=0.3,
          icon='Spyglass', power_class='EmberSpyglass'),
    # A 6 m wire standing 20 s; a creature crossing it takes 30 (20 x1.43) and lies stunned 2 s.
    power('DA_Ember_Tripwire', 'Tripwire', 'Tripwire', 'Craft',
          "You string a wire between two stakes up to 6 meters apart. For 20 seconds whatever crosses it trips, "
          "takes 30 damage and lies stunned for 2 seconds.",
          '#e2c98f', 14.0, 20.0, damage=30.0, range=600.0, extras={'Stun': 2.0},
          icon='Tripwire', power_class='EmberTripwire'),
    # 8 s, 15% harder per 10 m flown (from 10%: the game's fights are 20-30 m, where 10% was hardly felt), up to double;
    # the creatures in his sights (30 degrees, 60 m) are marked as it starts, and any he hits from 25 m or farther.
    power('DA_Ember_LongShot', 'LongShot', 'Long Shot', 'Sight',
          "For 8 seconds your shots hit 15% harder for every 10 meters they fly, up to double. The creatures in your "
          "sights as it starts, and any you hit from 25 meters or farther, are marked, and marked creatures are always "
          "critically hit.",
          '#6fe3d0', 20.0, 8.0, range=6000.0, strength=0.15,
          extras={'MaxBonus': 1.0, 'ConeDegrees': 30.0, 'MarkFrom': 2500.0},
          icon='LongShot', power_class='EmberLongShot'),
]


# The first-person arms' size against the guns (UHeroDefinition::FirstPersonArmsScale; 0: the hands component's 0.88,
# the user's for Ellis): a slight or short hero's smaller (Saved/FPArms/WIRING.md, the look sheets' sizes).
ARMS_SCALE = {'Ellis': 0.0, 'Odessa': 0.84, 'Crane': 0.0, 'Gauge': 0.0, 'Pike': 0.86}

# Where each body holds a gun in third person against the mannequin's hold (UHeroDefinition::ThirdPersonHold; cm on the
# full-size body: along the aim, to the right, up). Measured in play aiming the Bullpup (2026-10-20, the shoulders and
# head against the gun's grip, Ellis's being the mannequin's): Crane's shoulders and head sit ~14 cm (world) higher, so
# he held every gun at his chest; Pike's head ~6.6 lower (the sight over his eyes), Odessa's ~3; Gauge's head is lower and
# his shoulder joints 5.5 cm out and 6.5 back in the pauldrons, the rifle ran through his helm: lower, ahead and out.
THIRD_PERSON_HOLD = {'Ellis': (0.0, 0.0, 0.0), 'Odessa': (0.0, 0.0, -4.5), 'Crane': (0.0, 0.0, 16.5),
                     'Gauge': (5.0, 3.0, -8.0), 'Pike': (0.0, 0.0, -6.5)}

# Where the over-the-shoulder camera sits for each body against Ellis's, which it was tuned to (UHeroDefinition::
# ThirdPersonCamera; cm on the full-size body: extra arm length (+ farther back), extra to the right, extra up). Starting
# estimates from Saved/Screenshots/HeroGuns/checks/game_camera_aim_all.png (2026-10-20), tuned in play with the
# Looter.ThirdPerson.* cvars: Crane's hat (head and shoulders ~14 cm up, a wide brim) and Gauge's smokestacks (shoulders
# ~12 cm wider, stacks on his back) filled the left of the aimed view: Crane up with his head and back and out a little,
# Gauge out and back. Odessa and Pike, a little smaller than Ellis, come a touch closer and lower with their heads.
THIRD_PERSON_CAMERA = {'Ellis': (0.0, 0.0, 0.0), 'Odessa': (0.0, 0.0, -3.0), 'Crane': (20.0, 8.0, 16.0),
                       'Gauge': (35.0, 14.0, 8.0), 'Pike': (-5.0, 0.0, -5.0)}


def hero(id, name, role, story, color, powers):
    arms = 'SK_FP_Arms' if id == 'Ellis' else f'SK_FP_Arms_{id}'
    return dict(asset=f'DA_Hero_{id}', id=id, name=name, role=role, story=story, color=color, powers=powers,
                portrait=f'/Game/UI/Portraits/T_Portrait_{id}.T_Portrait_{id}',
                mesh=f'/Game/Art/Characters/SK_{id}.SK_{id}',
                retargeter=f'/Game/Player/Retarget/RTG_MannyTo{id}.RTG_MannyTo{id}',
                arms=f'/Game/Art/Characters/{arms}.{arms}', arms_scale=ARMS_SCALE[id], hold=THIRD_PERSON_HOLD[id],
                camera=THIRD_PERSON_CAMERA[id])


# The roster's order (UHeroDefinition::RosterIds); each hero's first power is equipped in a new game.
HEROES = [
    hero('Ellis', 'Ellis Ransom', 'the Revenant',
         "Shot on Ransom's Point by the Dunne Gang and raised by Sexton to bring the stolen embers home. Pa's lantern "
         "still burns at the hip.",
         '#5AD8FF', ['DA_Ember_DustDevil', 'DA_Ember_Sundown', 'DA_Ember_BorrowedTime']),
    hero('Odessa', 'Odessa Lark', 'the Cardsharp',
         "A riverboat gambler who caught Lucky Ned dealing seconds, and caught his bullet for it. Sexton bought her "
         "marker.",
         '#D04A68', ['DA_Ember_LuckyStreak', 'DA_Ember_FiftyTwoPickup', 'DA_Ember_DoubleOrNothing']),
    hero('Crane', 'Hollis Crane', 'the Unpaid',
         "Marshal of a town whose saint went dark, forty years walking. One eye still wears the ferryman's coin. "
         "Sexton promised him a fare.",
         '#B9D98C', ['DA_Ember_TollChain', 'DA_Ember_GraveWard', 'DA_Ember_LastRites']),
    hero('Gauge', 'Gauge', 'the Iron Hand',
         "The railroad's track-laying engine, woken in the wreck the gang left behind by a spark of ember sealed in "
         "its firebox.",
         '#FFB347', ['DA_Ember_SlagBomb', 'DA_Ember_IronHide', 'DA_Ember_RailSpike']),
    hero('Pike', 'Wendell Pike', 'the Surveyor',
         "He walked every mile the railroad will ever run, until the gang left him at the bottom of a ravine. He "
         "still knows the way.",
         '#3FBFAA', ['DA_Ember_Spyglass', 'DA_Ember_Tripwire', 'DA_Ember_LongShot']),
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
    asset.set_editor_property('icon_name', unreal.Name(spec['icon']) if spec['icon'] else unreal.Name('None'))
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
    # The power's C++ class by its path (Embers/Powers/*). None: its code isn't written yet, and the field is left as it
    # is. A named class not built yet: the asset is still written and the game says the power can't be used
    # (UEmberComponent logs it); run this again once the C++ is built.
    power_class = None
    class_words = 'no power class yet'
    if spec['power_class']:
        class_path = f"{SCRIPT}.{spec['power_class']}"
        power_class = unreal.load_class(None, class_path)
        if power_class is None:
            unreal.log_warning(f'EMBERS {path}: {class_path} is not a class yet; its power class is left empty')
            class_words = 'no power class'
        else:
            asset.set_editor_property('power_class', power_class)
            class_words = class_path
    save(asset, path)
    unreal.log(f"EMBERS {path} {'made' if created else 'updated'}: {spec['name']} ({spec['nature']}), cooldown "
               f"{spec['cooldown']} s, duration {spec['duration']} s, {spec['color']}, {class_words}")
    return asset


def setup_hero(spec, cls, made):
    path = f"{FOLDER}/{spec['asset']}"
    powers = []
    for name in spec['powers']:
        made_power = made.get(name)
        if made_power is None:
            raise RuntimeError(f'{path}: its power {name} was not made: nothing was saved')
        powers.append(made_power)
    asset, created = load_or_create(spec['asset'], cls)
    asset.set_editor_property('id', unreal.Name(spec['id']))
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    # What a caption calls them (the player character's lines, Story/HeroLines' {Hero}): the id is the word for all five.
    asset.set_editor_property('short_name', unreal.Text(spec['id']))
    asset.set_editor_property('role', unreal.Text(spec['role']))
    asset.set_editor_property('story', unreal.Text(spec['story']))
    asset.set_editor_property('color', linear_color(spec['color']))
    # The portrait, body and retargeter by path (Python sets a soft pointer only from a loaded object), each only once its
    # asset is in the game: a soft pointer to a missing package fails the editor's asset validation. Until then the game
    # finds them by the same naming rule (UHeroDefinition::GetFigureMeshPath), so run this again after each model lands.
    looks = {what: spec[what] if unreal.EditorAssetLibrary.does_asset_exist(spec[what].split('.')[0]) else ''
             for what in ('portrait', 'mesh', 'retargeter', 'arms')}
    asset.set_look_paths(looks['portrait'], looks['mesh'], looks['retargeter'])
    # The first-person arms the same way (a hero without theirs wears Ellis's), and their size.
    asset.set_first_person_arms(looks['arms'], spec['arms_scale'])
    # The gun at this body's own shoulder and eye in third person.
    asset.set_editor_property('third_person_hold', unreal.Vector(*spec['hold']))
    # And the over-the-shoulder camera fitted to this body's size (the hat or pack clear of the aimed view).
    asset.set_editor_property('third_person_camera', unreal.Vector(*spec['camera']))
    asset.set_editor_property('powers', powers)
    save(asset, path)
    there = [what for what, object_path in looks.items() if object_path]
    unreal.log(f"EMBERS {path} {'made' if created else 'updated'}: {spec['name']}, {spec['role']}, {spec['color']}, "
               f"with {', '.join(spec['powers'])}; in the game: {', '.join(there) if there else 'nothing yet'}; "
               f"arms' size {spec['arms_scale'] or 'as the hands component has it'}")


def run():
    ember_cls = need_class('EmberDefinition')
    hero_cls = need_class('HeroDefinition')
    made = {spec['asset']: setup_power(spec, ember_cls) for spec in POWERS}
    for spec in HEROES:
        setup_hero(spec, hero_cls, made)
    unreal.log('EMBERS done')


run()
