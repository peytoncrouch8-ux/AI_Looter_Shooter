"""Makes (or brings up to date) bestiary pages (UBestiaryEntry, Bestiary/BestiaryEntry.h) in /Game/Data/Bestiary: the
pages the Ledger shows. Run it in the open editor once the C++ with each page's actor class is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_bestiary_pages.py [Unpaid ...]"
With no names it makes every page below. It prints a BESTIARY line per page and "BESTIARY done". Running it again writes
these fields again (name, section, kind, description, habitat, notes, actor class, stand model, order) and leaves the
others as they are; other pages are never touched.

  DA_Bestiary_Unpaid  The Unpaid (AUnpaidCreature), in Enemies. It waits for its model: Looter.Bestiary.Entries wants a
                      model on every page's stand, so until SK_Unpaid is imported (Art/Models/Creatures/Unpaid.py) the
                      page isn't made, and the script says so.
"""
import sys

import unreal

FOLDER = '/Game/Data/Bestiary'

PAGES = {
    'Unpaid': dict(
        asset='DA_Bestiary_Unpaid',
        name='The Unpaid',
        section='ENEMY',
        kind='Restless dead',
        actor='/Script/AI_Looter_Shooter.UnpaidCreature',
        model='/Game/Art/Creatures/SK_Unpaid',
        habitat="Ransom's Rest: the town gate, the chapel yard, boot hill and the north road",
        description=("The dead of Ransom's Rest, come home. When Saint Ada went dark, every soul of the town still on the "
                     "Sundown Road lost the light and drifted back to its own grave. They're pale shapes in the clothes "
                     "they died in, fading below the waist into a trailing shroud, with a coal burning where the heart "
                     "should be. Cold, hungry and angry, and they know a gun in a dead hand can hurt them."),
        notes=[
            "The coal in its chest is the weak spot: a shot whose line runs through it is critical, for 1.5 times the "
            "damage. It only shows from the front, and an arm in the way takes the hit.",
            "It drifts after you, then shrieks and lunges. The shriek is the warning: back off and the lunge falls short.",
            "Leave it stuck behind a fence, or far behind you, and it fades out and comes back three to five meters closer.",
            "Its coal burns in its rank's color: dull red, blue for the Restless (two and a half times as tough, and quicker "
            "to lunge), purple for the Gravebound (five times as tough, and its shriek sends out a ring that slows you for "
            "two seconds).",
            "They won't cross Delia's salt line round the farm.",
            "Every Unpaid drops ammo, most often for the gun that killed it, and about one in three drops a weapon. The "
            "ones a fight brings don't come back.",
        ],
        sort_order=0,
    ),
}


def entry_class():
    cls = getattr(unreal, 'BestiaryEntry', None)
    if cls is None:
        raise RuntimeError('unreal.BestiaryEntry is missing: build the C++ first; nothing was changed')
    return cls


def load_or_create(asset_name, cls):
    path = f'{FOLDER}/{asset_name}'
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        asset = unreal.load_asset(path)
        if not isinstance(asset, cls):
            raise RuntimeError(f'{path} exists but is a {type(asset).__name__}, not a BestiaryEntry: nothing was changed')
        return asset, False
    factory = unreal.DataAssetFactory()
    factory.set_editor_property('data_asset_class', cls)
    asset = unreal.AssetToolsHelpers.get_asset_tools().create_asset(asset_name, FOLDER, cls, factory)
    if asset is None:
        raise RuntimeError(f'{path} could not be created')
    return asset, True


def set_actor_class(asset, path):
    """The page's actor class, a soft class reference, by its path; read back to be sure."""
    try:
        asset.set_editor_property('actor_class', unreal.SoftClassPath(path))
    except TypeError:
        asset.set_editor_property('actor_class', unreal.load_class(None, path))
    stored = asset.get_editor_property('actor_class')
    text = stored.export_text() if hasattr(stored, 'export_text') else (stored.get_path_name() if stored else '')
    if path not in text:
        raise RuntimeError(f"{asset.get_path_name()}: its actor class reads back as '{text}', not {path}: nothing was saved")


def set_model(asset, path):
    """The stand's model, set on the page itself: the class's own mesh is found only once the editor has started with it."""
    object_path = f"{path}.{path.rsplit('/', 1)[-1]}"
    try:
        asset.set_editor_property('preview_mesh', unreal.SoftObjectPath(object_path))
    except TypeError:
        asset.set_editor_property('preview_mesh', unreal.load_asset(path))


def setup(spec, cls):
    path = f"{FOLDER}/{spec['asset']}"
    if unreal.load_class(None, spec['actor']) is None:
        raise RuntimeError(f"{spec['actor']} is not a class: build the C++ first; nothing was changed")
    if not unreal.EditorAssetLibrary.does_asset_exist(spec['model']):
        unreal.log_warning(f"BESTIARY {path} waits for its model {spec['model']}: import it first (Looter.Bestiary.Entries wants a "
                           f"model on every page's stand); nothing was changed")
        return False
    asset, created = load_or_create(spec['asset'], cls)
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    asset.set_editor_property('category', getattr(unreal.BestiaryCategory, spec['section']))
    asset.set_editor_property('kind', unreal.Text(spec['kind']))
    asset.set_editor_property('description', unreal.Text(spec['description']))
    asset.set_editor_property('habitat', unreal.Text(spec['habitat']))
    asset.set_editor_property('notes', [unreal.Text(note) for note in spec['notes']])
    asset.set_editor_property('sort_order', spec['sort_order'])
    set_actor_class(asset, spec['actor'])
    set_model(asset, spec['model'])
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f'{path} could not be saved')
    unreal.log(f"BESTIARY {path} {'made' if created else 'updated'}: \"{spec['name']}\" in {spec['section'].title()} "
               f"({spec['kind']}), {len(spec['notes'])} notes, actor {spec['actor']}, stand {spec['model']}")
    return True


def run():
    cls = entry_class()
    wanted = [name for name in sys.argv[1:] if name in PAGES] or list(PAGES)
    waiting = [key for key in wanted if not setup(PAGES[key], cls)]
    if waiting:
        unreal.log(f"BESTIARY done; waiting for their models: {', '.join(waiting)}")
    else:
        unreal.log('BESTIARY done')


run()
