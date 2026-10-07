"""Makes (or brings up to date) bestiary pages (UBestiaryEntry, Bestiary/BestiaryEntry.h) in /Game/Data/Bestiary: the
pages the Ledger shows. Run it in the open editor once the C++ with each page's actor class is built:
  Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/create_bestiary_pages.py [Unpaid Sexton ...]"
With no names it makes every page below. It prints a BESTIARY line per page and "BESTIARY done". Running it again writes
these fields again (name, section, kind, description, habitat, notes, page type, the Ledger's flags and conditions, actor
class, stand model, order) and leaves the others as they are; other pages are never touched.

Three kinds of page (UBestiaryEntry::Page, Docs/Areas/RansomsRest.md "The Ledger"):
  - an actor page (ACTOR): something met in the world, its numbers read from its actor class and its model on the stand.
    It waits for its model: Looter.Bestiary.Entries wants a model on every actor page's stand, so until the model is
    imported the page isn't made, and the script says so.
  - a story character's page (STORY_CHARACTER): words, and a model on the stand when it has one (Hob's SK_Hob), but no
    actor, numbers or kills (Mister Sexton, Grandma Delia, Tilly, Father Aldana, Ranger Calder: talking doors and windows,
    and a seated static model the stand can't show; Hob, who is never fought). Open once its known_when holds (the
    mission that introduces them).
  - a Ledger name (LEDGER_NAME): one of the seven names Sexton writes in the Ledger, its whereabouts (habitat) blank until
    found_when holds (the Keeper's Lantern finds Lucky Ned in Main 7; the others wait for their missions).
Pages ledger_only (and every Ledger name) are listed only once Sexton has handed the Ledger over, Main 2's last step; they
are written in his voice (first drafts, for the user to judge).

  DA_Bestiary_Unpaid      The Unpaid (AUnpaidCreature), in Enemies; written before the Ledger, in the field guide's voice.
  DA_Bestiary_Hob         Hob, in Friends: a story character's page with SK_Hob on the stand; in the Ledger, known after
                          Main 1 (he never fights, so the player never "meets" him the way they meet a creature).
  DA_Bestiary_Sexton      Mister Sexton, NPCs; in the Ledger, known from the start (he wrote it).
  DA_Bestiary_Delia       Grandma Delia Ransom, NPCs; known after Main 1.
  DA_Bestiary_Tilly       Tilly Bright, Friends (Docs/Story.md's cast); known after Main 3.
  DA_Bestiary_Aldana      Father Moses Aldana, NPCs; known after Main 4.
  DA_Bestiary_Ruth        Ranger Ruth Calder, Friends (the cast's); known after Side 1 (her note on the board).
  DA_Bestiary_Ned ... _Deacon  the seven names, in Enemies before the Unpaid, in the order the lantern finds them.
"""
import sys

import unreal

FOLDER = '/Game/Data/Bestiary'

# The seven names: the Ledger lists them first among the Enemies, in the order the lantern will find them.
NAMES_FIRST = -7

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
    'Hob': dict(
        asset='DA_Bestiary_Hob',
        page='STORY_CHARACTER',
        name='Hob',
        section='FRIEND',
        kind='One-eyed crow',
        model='/Game/Art/Creatures/SK_Hob',
        ledger_only=True,
        known_when=dict(after=['Main1']),
        habitat="Wherever you ought to be going next",
        description=("My messenger. Only you can hear him, which is a mercy to everyone else. He talks too much and flies "
                     "too little, and he's never far from where you ought to be."),
        notes=[
            "He perches near the next thing to be done.",
            "Go off a high edge and he'll catch you, and complain about the weight.",
        ],
        sort_order=0,
    ),

    # --- The story's characters: words only ---
    'Sexton': dict(
        asset='DA_Bestiary_Sexton',
        page='STORY_CHARACTER',
        name='Mister Sexton',
        section='NPC',
        kind='A man of business',
        ledger_only=True,
        habitat="Ransom's Point: the lookout's rail",
        description=("The undersigned. I attend every death, friend; it's my trade. I keep this book and the accounts in "
                     "it, and I see that what's owed is paid."),
        notes=[
            "Terms: seven embers, each brought to the lookout on Ransom's Point at the new moon.",
            "In return: your rest, and your father's fare across.",
            "Mister Sexton keeps his word. He would thank you to keep yours.",
        ],
        sort_order=0,
    ),
    'Delia': dict(
        asset='DA_Bestiary_Delia',
        page='STORY_CHARACTER',
        name='Grandma Delia Ransom',
        section='NPC',
        kind="The Keeper's mother",
        ledger_only=True,
        known_when=dict(after=['Main1']),
        habitat="Ransom Farm: behind the screen door",
        description=("Abel Ransom's mother. She buried her son and her grandchild on the same day, and her son got up on "
                     "the Wednesday night. She won't open her door to you, friend, but she still sets a plate."),
        notes=[
            "She keeps the salt line round the farm. The Unpaid won't cross it.",
            "Talk to her through the screen door. She doesn't open it to the dead.",
        ],
        sort_order=1,
    ),
    'Aldana': dict(
        asset='DA_Bestiary_Aldana',
        page='STORY_CHARACTER',
        name='Father Moses Aldana',
        section='NPC',
        kind="Preacher of Ransom's Rest",
        ledger_only=True,
        known_when=dict(after=['Main4']),
        habitat="The Chapel of Saint Ada: the vestry door",
        description=("The Rest's preacher. He is terrified of you, friend, and he has reason: his saint is dark and his "
                     "flock walks the chapel yard. Frightened men often come round. He will."),
        notes=[
            "He talks through the vestry door.",
        ],
        sort_order=2,
    ),
    'Tilly': dict(
        asset='DA_Bestiary_Tilly',
        page='STORY_CHARACTER',
        name='Tilly Bright',
        section='FRIEND',
        kind="The undertaker's daughter",
        ledger_only=True,
        known_when=dict(after=['Main3']),
        habitat="Main Street: Bright & Daughter, Undertakers",
        description=("She dressed you and your father for the box, and she was the first in town to speak to you after. "
                     "Practical, which in her trade is a kindness. Her late father's hearse car still runs on the line."),
        notes=[
            "She talks through her shop window. The card says she's back after the funeral.",
            "She knows the old rites.",
        ],
        sort_order=1,
    ),
    'Ruth': dict(
        asset='DA_Bestiary_Ruth',
        page='STORY_CHARACTER',
        name='Ranger Ruth Calder',
        section='FRIEND',
        kind='Rim Ranger',
        ledger_only=True,
        known_when=dict(after=['Side1']),
        habitat="Her signature on the wanted posters",
        description=("A Rim Ranger hunting the Dunne Gang. Her name is on every poster of you in this valley. She'll shoot "
                     "on sight, friend; she doesn't yet know what you are."),
        notes=[
            "She signs herself Capt. R. Calder.",
            "She leaves caches for her Rangers.",
        ],
        sort_order=2,
    ),

    # --- The seven names: Enemies, before the Unpaid, whereabouts blank until the lantern finds them ---
    'Ned': dict(
        asset='DA_Bestiary_Ned',
        page='LEDGER_NAME',
        name='\u201cLucky\u201d Ned Purcell',
        section='ENEMY',
        kind='The gambler',
        habitat='The Gilded Lily',
        found_when=dict(after=['Main7']),
        description=("Plays cards for a living and calls it luck. He shot your father on the bluff path. He carries an "
                     "ember that isn't his, and fortune follows it about like a dog."),
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST,
    ),
    'Ira': dict(
        asset='DA_Bestiary_Ira',
        page='LEDGER_NAME',
        name='\u201cWhistling\u201d Ira Gale',
        section='ENEMY',
        kind='The horse thief',
        description=("Steals horses and whistles while he does it. The wind has taken a liking to him since he took his "
                     "ember."),
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 1,
    ),
    'Constance': dict(
        asset='DA_Bestiary_Constance',
        page='LEDGER_NAME',
        name='Sister Constance Holloway',
        section='ENEMY',
        kind="The gang's medic",
        description=("She patched up the gang for years, and she believes she's helping still. Mercy is a dangerous thing to "
                     "carry about, friend."),
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 2,
    ),
    'Barrels': dict(
        asset='DA_Bestiary_Barrels',
        page='LEDGER_NAME',
        name='Bartholomew \u201cBarrels\u201d Kessler',
        section='ENEMY',
        kind='The powder man',
        description="He blew the safes and the bridges. His ember runs hot in him; he'll be burning by now.",
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 3,
    ),
    'Lena': dict(
        asset='DA_Bestiary_Lena',
        page='LEDGER_NAME',
        name='Lena \u201cSpyglass\u201d Okoro',
        section='ENEMY',
        kind='The sniper',
        description="She never misses, and she never forgets a face. She will have seen you before you see her.",
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 4,
    ),
    'Tobias': dict(
        asset='DA_Bestiary_Tobias',
        page='LEDGER_NAME',
        name='Tobias \u201cMule\u201d Grant',
        section='ENEMY',
        kind='The strongman',
        description=("He carried the gang's heavy loads and never once asked why. He doesn't want a fight. That won't save "
                     "him."),
        notes=["Owes one ember."],
        sort_order=NAMES_FIRST + 5,
    ),
    'Deacon': dict(
        asset='DA_Bestiary_Deacon',
        page='LEDGER_NAME',
        name='Ambrose Dunne, the Deacon',
        section='ENEMY',
        kind="The gang's leader",
        description=("A preacher thrown out of his church, and the man who shot you. He kept this valley's ember for "
                     "himself. Last on the list, friend, and the hardest."),
        notes=["Owes one ember: Saint Ada's."],
        sort_order=NAMES_FIRST + 6,
    ),
}


def entry_class():
    cls = getattr(unreal, 'BestiaryEntry', None)
    if cls is None or getattr(unreal, 'BestiaryPage', None) is None:
        raise RuntimeError('unreal.BestiaryEntry or unreal.BestiaryPage is missing: build the C++ first; nothing was changed')
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


def condition(after=(), before=()):
    """FStoryCondition: after these missions, before those (empty: always, or for found_when, never)."""
    made = unreal.StoryCondition()
    made.set_editor_property('after_missions', [unreal.Name(m) for m in after])
    made.set_editor_property('before_missions', [unreal.Name(m) for m in before])
    return made


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
    page = spec.get('page', 'ACTOR')
    if page == 'ACTOR' and unreal.load_class(None, spec['actor']) is None:
        raise RuntimeError(f"{spec['actor']} is not a class: build the C++ first; nothing was changed")
    if 'model' in spec and not unreal.EditorAssetLibrary.does_asset_exist(spec['model']):
        unreal.log_warning(f"BESTIARY {path} waits for its model {spec['model']}: import it first (Looter.Bestiary.Entries wants a "
                           f"model on every actor page's stand, and the one a page names); nothing was changed")
        return False
    asset, created = load_or_create(spec['asset'], cls)
    asset.set_editor_property('display_name', unreal.Text(spec['name']))
    asset.set_editor_property('category', getattr(unreal.BestiaryCategory, spec['section']))
    asset.set_editor_property('kind', unreal.Text(spec['kind']))
    asset.set_editor_property('description', unreal.Text(spec['description']))
    asset.set_editor_property('habitat', unreal.Text(spec.get('habitat', '')))
    asset.set_editor_property('notes', [unreal.Text(note) for note in spec['notes']])
    asset.set_editor_property('sort_order', spec['sort_order'])
    asset.set_editor_property('page', getattr(unreal.BestiaryPage, page))
    asset.set_editor_property('ledger_only', spec.get('ledger_only', False))
    asset.set_editor_property('known_when', condition(**spec.get('known_when', {})))
    asset.set_editor_property('found_when', condition(**spec.get('found_when', {})))
    if page == 'ACTOR':
        set_actor_class(asset, spec['actor'])
    if 'model' in spec:
        set_model(asset, spec['model'])
    if not unreal.EditorAssetLibrary.save_loaded_asset(asset, only_if_is_dirty=False):
        raise RuntimeError(f'{path} could not be saved')
    what = (f"actor {spec['actor']}, stand {spec['model']}" if page == 'ACTOR'
            else 'one of the seven names' if page == 'LEDGER_NAME'
            else 'a story character, no actor' + (f", stand {spec['model']}" if 'model' in spec else ''))
    when = spec.get('known_when', {}).get('after') or spec.get('found_when', {}).get('after')
    unreal.log(f"BESTIARY {path} {'made' if created else 'updated'}: \"{spec['name']}\" in {spec['section'].title()} "
               f"({spec['kind']}), {len(spec['notes'])} notes, {what}"
               f"{', in the Ledger' if spec.get('ledger_only') or page == 'LEDGER_NAME' else ''}"
               f"{', after ' + ', '.join(when) if when else ''}")
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
