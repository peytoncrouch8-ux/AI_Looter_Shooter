"""Signs of the living in Ransom's Rest's town (World/TownLife*: UTownLifeSubsystem, ATownLifeDoor), for build_area.py's
gameplay pass. The town is half empty since the saint went dark, and the half that stayed hides from the walking corpse
(Docs/Areas/RansomsRest.md: townsfolk are heard, not seen). Only a level whose layout has Ellis's grave gets any of it.

- The townsfolk's doors (ATownLifeDoor, in the Gameplay/TownLife folder, labelled TownLife_<key>): just outside the front
  doors of the lived-in houses whose people nobody has met (Pruitt's store, the north and south cottages), about head
  high, facing out. Each names its household; somebody inside mutters a line as the player passes (as a caption) and
  the house is heard through the door now and then. Delia's and Tilly's doors are their own speaker points already.
- Who lives in each lit house (AHouseLights, placed by build_area.py's house_lights as Lights_<key>): its Household, so
  the house is heard through its walls (Delia alone at the farmhouse, Tilly at her work, the Pruitts, the two cottages'
  families). The dark cottages have no lights and nobody in them.
- The store's shutters (AWindowShutter, build_area_story.py's Shutter_<L|R><n>): the Pruitts', heard through once shut.

From build_area.py's gameplay(), after the story's actors (the shutters must stand; the lights stand from effects()):
    importlib.reload(importlib.import_module('build_area_townlife')).place(self)
On its own, in the open editor (clears the Gameplay/TownLife folder, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_townlife.py RansomsRest"
Without the game module's classes yet it places nothing and says so.
"""
import json
import os
import sys

import unreal

CLASSES = '/Script/AI_Looter_Shooter.'
FOLDER = 'Gameplay/TownLife'

# The townsfolk's doors: the house's placement key, its household (TownLifeRules), and where its door is.
DOORS = (
    ('store', 'Pruitt'),
    ('cottageNorth', 'CottageNorth'),
    ('cottageSouth', 'CottageSouth'),
)

# Who lives in each lit house, by placement key (layout level.lights; the lights are labelled Lights_<key>).
HOUSEHOLDS = {
    'farmhouse': 'Ransom',
    'undertaker': 'Bright',
    'store': 'Pruitt',
    'cottageNorth': 'CottageNorth',
    'cottageSouth': 'CottageSouth',
}

# The settler's cottage's front door in the model's own frame (Art/Models/Buildings/Cottage.py, Cottage_Ransom: the front
# wall 3.2 m out along the model's front, which is Unreal's +X, the door's middle 1.01 m to its left, which is +Y), a
# little out on the stoop and about head high (cm).
COTTAGE_DOOR = unreal.Vector(345.0, 101.0, 150.0)
# Pruitt's store: its lamp (SOCKET_Light) hangs behind the double doors in the facade's middle, 0.65 m in from the front
# and 2.25 m over the floor (FalseFronts.py SHOP_LAMP): the door's spot is 1.25 m out from it and 0.75 m lower (cm).
STORE_OUT = 125.0
STORE_DOWN = 75.0


def actor_class(name):
    return unreal.load_class(None, CLASSES + name)


def level_actors(build):
    """The actors build_area.py placed for this area, by label."""
    found = {}
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if unreal.Name(build.tag) in actor.tags:
            found.setdefault(str(actor.get_actor_label()), actor)
    return found


def door_spot(house, key):
    """Where a house's door actor stands, (location, yaw), or None."""
    yaw = house.get_actor_rotation().yaw
    if key == 'store':
        mesh = house.static_mesh_component
        if mesh is None or not mesh.does_socket_exist('Light'):
            return None
        lamp = mesh.get_socket_transform('Light', unreal.RelativeTransformSpace.RTS_WORLD).translation
        ahead = house.get_actor_forward_vector()
        return unreal.Vector(lamp.x + ahead.x * STORE_OUT, lamp.y + ahead.y * STORE_OUT, lamp.z - STORE_DOWN), yaw
    return unreal.MathLibrary.transform_location(house.get_actor_transform(), COTTAGE_DOOR), yaw


def place_doors(build, found):
    cls = actor_class('TownLifeDoor')
    if cls is None:
        build.warn('no TownLifeDoor class (build the game module first): no townsfolk at their doors')
        return
    count = 0
    for key, household in DOORS:
        house = found.get(key)
        if house is None:
            build.warn(f'no {key} placed (build the whole level first): its door says nothing')
            continue
        spot = door_spot(house, key)
        if spot is None:
            build.warn(f'{key} has no Light socket: its door is left out')
            continue
        at, yaw = spot
        door = build.place(cls, (at.x, at.y, at.z), yaw, label=f'TownLife_{key}', folder=FOLDER)
        door.set_editor_property('household', unreal.Name(household))
        count += 1
        build.log(f'the {household} door at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')
    build.log(f'{count} townsfolk\'s doors')


def set_households(build, found):
    """Who lives in each lit house, and the store's shutters (all of them are the store's)."""
    lit = 0
    for key, household in HOUSEHOLDS.items():
        lights = found.get(f'Lights_{key}')
        if lights is None:
            build.warn(f'no Lights_{key} (build_area.py\'s effects place it): that house stays quiet')
            continue
        lights.set_editor_property('household', unreal.Name(household))
        lit += 1
    shutter_cls = actor_class('WindowShutter')
    shutters = 0
    if shutter_cls is not None:
        for label, actor in found.items():
            if label.startswith('Shutter_') and actor.get_class() == shutter_cls:
                actor.set_editor_property('household', unreal.Name('Pruitt'))
                shutters += 1
    build.log(f'households in {lit} lit houses and on {shutters} shutters')


def place(build):
    """Everything above (build_area.py's gameplay() calls this after the story's actors). Only Ransom's Rest has it."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    found = level_actors(build)
    place_doors(build, found)
    set_households(build, found)


def run(name):
    """Places the doors again on their own (clears the Gameplay/TownLife folder), sets the households, saves."""
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import build_area  # noqa: E402 (here, not at the top: build_area imports this module)
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('town life placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_townlife.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
