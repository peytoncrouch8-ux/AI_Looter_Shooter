"""The vendors (Economy/Vendor: AVendor; Docs/Progression.md, "Economy"), for build_area.py's gameplay pass. A vendor has
no model of its own: it stands at a shop's door, whose building the full build placed, and Interact there opens the
vendor's screen. Its spot is derived from the built level, as the town's doors are (build_area_townlife.py):

- Ransom's Rest (the layout with Ellis's grave): Pruitt's General Store on Main Street (placement 'store',
  FalseFront_Store). The Pruitts trade through the hatch in their door once Main Street is safe: open after Main 3 (Cold
  Welcome). Labelled Vendor_Pruitt. Its voice is Mrs. Pruitt (tag Speaker_Pruitt): missions are given and turned in at the
  hatch (AVendor::Voice), ahead of the shop's screen.
- Skyreach (the layout whose gameplay.director is TutorialDirector): the store front on Crossroads Town's square (its
  'store', the same model), a practice store with an honesty box on the counter, always open. Labelled Vendor_Crossroads.

The spot: the store's lamp (FalseFronts.py SHOP_LAMP: the model's SOCKET_Light, behind the double doors in the facade's
middle, 0.65 m in from the front and 2.25 m over the floor), 0.95 m out from it through the doors and 0.8 m lower (about
1.45 m over the porch, where the player's eyes meet the door), facing out the way the store faces. Placed in the
<area>/Gameplay/Vendors folder, which a gameplay build clears with the rest of Gameplay.

From build_area.py's gameplay(), after the town's life (the store must stand; it's placed by the full build's models()):
    importlib.reload(importlib.import_module('build_area_vendors')).place(self)
On its own, in the open editor (clears the Gameplay/Vendors folder, places, saves):
    Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area_vendors.py RansomsRest"
Without the game module's class yet it places nothing and says so.
"""
import json
import os
import sys

import unreal

CLASSES = '/Script/AI_Looter_Shooter.'
FOLDER = 'Gameplay/Vendors'
STORE_KEY = 'store'
STORE_MESH = 'FalseFront_Store'
DIRECTOR = 'TutorialDirector'

# From the store's lamp (SOCKET_Light) out through the doors and down to eye height at the door (cm).
LAMP_OUT = 95.0
LAMP_DOWN = 80.0
# A model without the lamp socket (an older import): the door's spot in the facade's own frame instead (cm; +X out of
# the front, the floor at 0), as the lamp would put it.
DOOR_FALLBACK = unreal.Vector(30.0, 0.0, 145.0)

# Each area's vendor: its label, the screen's title and greeting, the Interact prompt, and the missions before it opens;
# optionally the voice the story's missions speak through at its door (voice_name, voice_tag).
VENDORS = {
    'RansomsRest': dict(
        label='Vendor_Pruitt',
        title="Pruitt's General Store",
        greeting=('A hatch in the door slides open a hand\'s width. "Coin on the sill, and step back," says Harlan Pruitt. '
                  '"Ammunition, salt, whatever\'s on the shelf. I\'ll take guns off your hands, too."'),
        prompt="Trade at Pruitt's",
        after=('Main3',),
        # Missions are given and turned in at the hatch (the Gravemother's bounty to Mrs. Pruitt): the vendor's voice, by
        # name and by the speaker tag the missions find it with (AVendor::VoiceName, VoiceTag).
        voice_name="Mrs. Pruitt",
        voice_tag='Speaker_Pruitt',
    ),
    'TutorialIsland': dict(
        label='Vendor_Crossroads',
        title='Crossroads Mercantile',
        greeting=('Nobody minds the counter. An honesty box, a ledger, and a card in a neat hand: "Take what you pay for. '
                  'Leave what you\'d sell. Practice prices."'),
        prompt='Trade at the Mercantile',
        after=(),
    ),
}


def actor_class(name):
    return unreal.load_class(None, CLASSES + name)


def which_area(build):
    """RansomsRest, TutorialIsland (Skyreach) or None, by the layout's own marks."""
    if any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return 'RansomsRest'
    if build.settings.get('director') == DIRECTOR:
        return 'TutorialIsland'
    return None


def find_store(build):
    """The store front the build placed (its label is the placement's key), or None."""
    for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
        if unreal.Name(build.tag) in actor.tags and str(actor.get_actor_label()) == STORE_KEY:
            return actor
    return None


def door_spot(store):
    """Where the vendor stands at the store's door and the way it faces: (location, yaw)."""
    yaw = store.get_actor_rotation().yaw
    mesh = store.get_editor_property('static_mesh_component') if isinstance(store, unreal.StaticMeshActor) else None
    if mesh is not None and mesh.does_socket_exist('Light'):
        lamp = mesh.get_socket_transform('Light', unreal.RelativeTransformSpace.RTS_WORLD).translation
        ahead = store.get_actor_forward_vector()
        return unreal.Vector(lamp.x + ahead.x * LAMP_OUT, lamp.y + ahead.y * LAMP_OUT, lamp.z - LAMP_DOWN), yaw
    return unreal.MathLibrary.transform_location(store.get_actor_transform(), DOOR_FALLBACK), yaw


def condition(after):
    """FStoryCondition: after these missions (empty: always)."""
    made = unreal.StoryCondition()
    made.set_editor_property('after_missions', [unreal.Name(m) for m in after])
    return made


def place(build):
    """The area's vendor (build_area.py's gameplay() calls this after the town's life). Only Ransom's Rest and Skyreach have one."""
    area = which_area(build)
    if area is None:
        return
    spec = VENDORS[area]
    cls = actor_class('Vendor')
    if cls is None:
        build.warn('no Vendor class (build the game module first): no vendor placed')
        return
    store = find_store(build)
    if store is None:
        build.warn(f'no {STORE_KEY} placed (build the whole level first): {spec["label"]} is left out')
        return
    mesh = store.get_editor_property('static_mesh_component') if isinstance(store, unreal.StaticMeshActor) else None
    model = mesh.get_editor_property('static_mesh') if mesh is not None else None
    if model is None or STORE_MESH not in model.get_name():
        build.warn(f'{STORE_KEY} is not a {STORE_MESH} ({model.get_name() if model else "no mesh"}): the vendor stands at its middle front')
    at, yaw = door_spot(store)
    vendor = build.place(cls, (at.x, at.y, at.z), yaw, label=spec['label'], folder=FOLDER)
    vendor.set_editor_property('title', unreal.Text(spec['title']))
    vendor.set_editor_property('greeting', unreal.Text(spec['greeting']))
    vendor.set_editor_property('prompt', unreal.Text(spec['prompt']))
    vendor.set_editor_property('open_when', condition(spec['after']))
    if spec.get('voice_name'):
        vendor.set_editor_property('voice_name', unreal.Text(spec['voice_name']))
    if spec.get('voice_tag'):
        vendor.set_editor_property('voice_tag', unreal.Name(spec['voice_tag']))
    opens = f'after {", ".join(spec["after"])}' if spec['after'] else 'always'
    build.log(f'{spec["label"]} at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}) facing {yaw:.0f}, open {opens}')


def run(name):
    """Places the vendor again on its own (clears the Gameplay/Vendors folder), saves."""
    sys.path.append(os.path.dirname(os.path.abspath(__file__)))
    import build_area  # noqa: E402 (here, not at the top: build_area imports this module)
    build = build_area.AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('vendor placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_vendors.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
