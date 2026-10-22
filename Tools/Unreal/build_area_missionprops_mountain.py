"""The mountain's mission props, for build_area.py's gameplay pass (Docs/Areas/RansomsRest_Expansion.md, section 4: Dead
Lift, The Last Watch, Back Pay; section 6): placed from the mountain's buildings as build_area.py stood them, their sockets
(Art/Models/Buildings/OreLift.py, CalderPost.py, Diggings.py) and the mine's table (Art/Models/Mine/Mine.layout.json, in
the mine's frame: build_area_mine.py), so they follow the level whenever it's rebuilt. Everything goes in the area's
Gameplay/Mountain folder (the chests in Gameplay, as every chest), which a gameplay build clears; a missing building,
socket, model or class is warned of and left out. Only a level with the mountain gets any of it.

The names are the wave 4 contract's (the missions' data, Tools/Unreal/create_side_mission_assets.py, finds them by these):
- Places (target points, a player's middle over the floor): Place_OldQuarry, Place_LiftHead, Place_CalderPost,
  Place_Bench, Place_MineMouth, Place_UpperGallery, Place_LowerDrift, Place_Stope (the mine's from its frame, never traced:
  a trace from above finds the Bench over them).
- Hiram Coyle (AHiramCoyle, Speaker_Hiram) on the winch house's SOCKET_HiramStool, his seat on the stool's top, the
  speaking tube's side (TubeSide) from SOCKET_SpeakingTube.
- Dead Lift's fittings (AMissionFitting, tagged LiftFitting and LiftFitting_Boiler / _Pin): held 1.5 s at the house's
  SOCKET_Boiler (the firebox door, stoked from the quarry's own coal bin) and SOCKET_ClampPin (the bottom sheave), only
  in the fittings' step; the emptied coal sack (by SOCKET_CoalBin) and the pin show once done, and stay from the next step
  on. The crank stands on SOCKET_Crank from the start (the crank fetch went, 2026-10-21).
  Smoke from the stack (an AStoryProp, SOCKET_Smoke) once she raises steam, and for good after.
- The Last Watch at Ruth's cabin: its board (ANoticeBoard tagged NoticeBoard_CalderPost) at SOCKET_NoticeBoard, the lamp
  oil (AMissionItem LampOil) on SOCKET_LampOil, Ruth's journal (RuthJournal), the post's log (PostLog), and the code book
  (CodeBook: readable with the far light's answer only in its step; the plain book on the shelf otherwise), Calder's
  fourth cache (a Supply Crate, CalderCache_Post, at SOCKET_Cache, opening once The Last Watch is turned in).
- Back Pay (build_area_providence.py, with these helpers): at the portal's tag board J. Odell's tag (AMissionArtifact
  Artifact_MinersTag) on IN's free hook and the shift's voice (a speaker point Speaker_TagBoard, missions only); in the
  mine the payroll book (PayrollBook), the chalk (ChalkTally), the barricade (Barricade: reading it takes the eleven's
  tags), the payroll chest (the mission item Payroll, tagged PayrollChest too) from the Foreman's step, the company safe
  (a Strongbox, CompanySafe), and the manway's barred gate (an AMissionFitting that swings open from the Foreman's step
  on).
- The mountain's fights and the Foreman's lair (build_area_fights_mountain.py, from gameplay.encounters.fights).
Their words are the missions' data's (Tools/Unreal/story_texts.py: READABLES, ARTIFACTS, ITEMS, and the speakers' TOPICS
and MUTTERS for Hiram and the tag board); a missing one is warned of. The missions' steps these follow (STEPS) are the
design's order, counted from 0, as the data numbers them; layout.json's gameplay.mountain.steps overrides them.

Hook in build_area.py's gameplay(), after self.story.place(self) and before the vigil stones (the Foreman's stone finds
his lair, Encounter_Foreman):
    importlib.reload(importlib.import_module('build_area_missionprops_mountain')).place(self)
"""
import importlib
import json
import os
import sys

import unreal

sys.path.append(os.path.dirname(os.path.abspath(__file__)))
import build_area_story as story  # noqa: E402

FOLDER = 'Gameplay/Mountain'

# The missions and the story flags (the wave 4 contract's ids).
MAIN5 = 'Main5'
DEAD_LIFT = 'DeadLift'
LAST_WATCH = 'LastWatch'
BACK_PAY = 'BackPay'
PAID_FLAG = 'BackPay.Paid'
KEPT_FLAG = 'BackPay.Kept'

# Each mission's steps by what they ask, counted from 0 (the design's order, section 4).
STEPS = {
    DEAD_LIFT: {'items': 0, 'fittings': 1, 'steam': 2, 'ride': 3},
    LAST_WATCH: {'yard': 0, 'oil': 1, 'fire': 2, 'watch': 3, 'codeBook': 4},
    BACK_PAY: {'gallery': 0, 'upper': 1, 'office': 2, 'lower': 3, 'barricade': 4, 'foreman': 5, 'payroll': 6},
}

# The places (the contract's Place_<Name>): ('level', [X, Y]) on the ground there, ('zone', id) at its middle, ('mine',
# (x, y, z) metres) in the mine's frame, ('liftHead',) on the head's landing step. gameplay.mountain.places overrides one
# with [X, Y].
PLACES = {
    'OldQuarry': ('level', (10150.0, 2000.0)),
    'LiftHead': ('liftHead',),
    'CalderPost': ('level', (16900.0, -6500.0)),
    'Bench': ('zone', 'larkspurBench'),
    'MineMouth': ('mine', (0.0, -3.0, 0.0)),
    'UpperGallery': ('mine', (0.0, 42.0, 0.0)),
    'LowerDrift': ('mine', (-21.0, 66.0, -8.0)),
    'Stope': ('mine', (-17.0, 90.0, -8.0)),
}
LIFT_HEAD_PAD = (14600.0, 2700.0)

# The readables (AReadableProp): their words are the missions' data's (Tools/Unreal/story_texts.py READABLES, copied on by
# its apply_readable); here their models (the props agent's; none: the dressing's, the wall's chalk) and a title to stand in
# while story_texts.py lacks one.
READABLES = {
    'RuthJournal': dict(model='SM_Journal', title="Ruth Calder's journal"),
    'CodeBook': dict(model='SM_CodeBook', title="Ruth's code book"),
    'PostLog': dict(model='SM_PostLog', title="The post's log"),
    'PayrollBook': dict(model='SM_PayrollBook', title="The Foreman's payroll"),
    'ChalkTally': dict(model='', title='Chalk on the gallery wall'),
    'Barricade': dict(model='', title="The eleven's barricade", prompt='Take the tags'),
}
READ_REACH = 250.0
TALK_REACH = 300.0

# Hiram's seat (Hiram.py): the seat of his trousers 56 cm over his origin, the stool's middle 3 cm behind it.
HIRAM_SEAT = 56.0
HIRAM_BACK = 3.0

# The sounds of the mountain's uses (LooterSound cues the sound pass makes: Art/Sounds/recipes).
CUES = {'Crank': 'World.LiftFitting.Crank', 'Boiler': 'World.LiftFitting.Boiler', 'Pin': 'World.LiftFitting.Pin',
        'Gate': 'World.ManwayGate.Open'}

_MESHES = {}


# ---------------------------------------------------------------------------
# Helpers (build_area_mountain.py and build_area_mine.py use these too)
# ---------------------------------------------------------------------------

def step(build, mission, key):
    """A mission's step by what it asks, counted from 0: layout.json gameplay.mountain.steps, else the design's (STEPS)."""
    own = build.settings.get('mountain', {}).get('steps', {}).get(mission, {})
    return int(own.get(key, STEPS[mission][key]))


def when(after=(), before=(), during='', from_step=0, before_step=0, with_flags=(), without_flags=()):
    """FStoryCondition: after missions, before others, during one (from a step, before a step; from 0), with or without
    story flags."""
    made = story.condition(after=after, before=before, during=during, from_step=from_step, before_step=before_step)
    made.set_editor_property('with_flags', [unreal.Name(f) for f in with_flags])
    made.set_editor_property('without_flags', [unreal.Name(f) for f in without_flags])
    return made


def texts():
    """The missions' words for things in the level (Tools/Unreal/story_texts.py, the missions' data's), reloaded; None
    while it isn't there (this script's own READABLES stand in)."""
    try:
        return importlib.reload(importlib.import_module('story_texts'))
    except ImportError:
        return None


def spec_when(spec):
    """An FStoryCondition from a story_texts condition dict (after, before, during, from_step, before_step, with_flags,
    without_flags); None or {} is always."""
    spec = spec or {}
    return when(after=spec.get('after', ()), before=spec.get('before', ()), during=spec.get('during', ''),
                from_step=spec.get('from_step', 0), before_step=spec.get('before_step', 0),
                with_flags=spec.get('with_flags', ()), without_flags=spec.get('without_flags', ()))


def speak(build, point, tag):
    """A speaker point's words from story_texts.TOPICS and MUTTERS by its tag: its name, its default line set, its topics
    (the first that holds), its mutters, and missions only while it waits on its mission (missions_only_before: a topic
    after counts as business, so it talks then). Line sets not made yet are left out (create_story_lines.py makes them)."""
    words = texts()
    if words is None or tag not in getattr(words, 'TOPICS', {}):
        return
    spec = words.TOPICS[tag]
    if spec.get('name'):
        point.set_editor_property('speaker_name', unreal.Text(spec['name']))
    if spec.get('default'):
        line_set = story.lines(build, spec['default'])
        if line_set:
            point.set_editor_property('line_set', line_set)
    point.set_editor_property('topics', story.topics(build, *[(spec_when(w), name) for w, name in spec.get('topics', [])]))
    if spec.get('missions_only_before'):
        point.set_editor_property('missions_only', True)
    mutters = getattr(words, 'MUTTERS', {}).get(tag)
    if mutters:
        point.set_editor_property('mutters', story.topics(build, *[(spec_when(w), name) for w, name in mutters]))


def mesh(build, name):
    """A static mesh by asset name anywhere under /Game/Art, or None (quietly: the caller says what's left out)."""
    if not name:
        return None
    if build.tag not in _MESHES:
        import build_area
        _MESHES[build.tag] = build_area.mesh_index()
    path = _MESHES[build.tag].get(name[3:] if name.startswith('SM_') else name)
    return unreal.load_asset(path) if path else None


def first_placed(build, kind):
    """The mountain's building of this kind as build_area.py stood it, or None (with a warning)."""
    import build_area_mountain as mountain
    found = mountain.placed_all(build, (kind,))
    if not found:
        build.warn(f'no {kind} placed (build the whole level with the mountain\'s placements): what stands on it is left out')
        return None
    return found[0]


def at_socket(build, building, name):
    """A building's socket (world transform), or None with a warning naming it."""
    if building is None:
        return None
    found = story.socket(building, name)
    if found is None:
        build.warn(f'{building.get_actor_label()} has no SOCKET_{name}: what stands there is left out')
    return found


def xyz(vector):
    return (vector.x, vector.y, vector.z)


def readable(build, cls, readable_id, location, yaw, shown_when=None, label=None):
    """An AReadableProp: its words from story_texts.READABLES (else READABLES here), its model here, and a ShownWhen of
    the caller's over the words' own."""
    spec = READABLES[readable_id]
    prop = build.place(cls, location, yaw, label=label or f'Readable_{readable_id}', folder=FOLDER,
                       tags=(f'Readable_{readable_id}',))
    words = texts()
    if words is not None and readable_id in getattr(words, 'READABLES', {}):
        words.apply_readable(prop, readable_id)
    else:
        build.warn(f'no words for {readable_id} in Tools/Unreal/story_texts.py: it stands with its title alone')
        prop.set_editor_property('readable_id', unreal.Name(readable_id))
        prop.set_editor_property('title', unreal.Text(spec['title']))
        if spec.get('prompt'):
            prop.set_editor_property('prompt', unreal.Text(spec['prompt']))
    prop.set_editor_property('reach', READ_REACH)
    if shown_when is not None:
        prop.set_editor_property('shown_when', shown_when)
    model = mesh(build, spec['model'])
    if model is not None:
        prop.get_editor_property('mesh').set_static_mesh(model)
    elif spec['model']:
        build.warn(f'no {spec["model"]} yet (the props agent\'s): {readable_id} is read where it stands, unseen')
    return prop


def item(build, cls, item_id, location, yaw, model_name='', shown_when=None):
    """An AMissionItem: its prompt and when it's there from story_texts.ITEMS when it has the item (its shown_when, none:
    while its mission is taken), else the caller's shown_when."""
    made = build.place(cls, location, yaw, label=f'Item_{item_id}', folder=FOLDER, tags=(f'Item_{item_id}',))
    made.set_editor_property('item_id', unreal.Name(item_id))
    words = texts()
    spec = getattr(words, 'ITEMS', {}).get(item_id) if words is not None else None
    if spec is not None:
        if spec.get('prompt'):
            made.set_editor_property('prompt', unreal.Text(spec['prompt']))
        shown_when = spec_when(spec['shown_when']) if spec.get('shown_when') else None
    if shown_when is not None:
        made.set_editor_property('shown_when', shown_when)
    model = mesh(build, model_name)
    if model is not None:
        made.get_editor_property('mesh').set_static_mesh(model)
    return made


def fitting(build, cls, location, yaw, label, tags, prompt, usable, done, model=None, hide_until_done=True, hold=1.5,
            cue='', reach=READ_REACH):
    made = build.place(cls, location, yaw, label=label, folder=FOLDER, tags=tags)
    made.set_editor_property('prompt', unreal.Text(prompt))
    made.set_editor_property('hold', True)
    made.set_editor_property('hold_seconds', hold)
    made.set_editor_property('reach', reach)
    made.set_editor_property('usable_when', usable)
    made.set_editor_property('done_when', done)
    made.set_editor_property('hide_until_done', hide_until_done)
    made.set_editor_property('use_cue', unreal.Name(cue))
    if model is not None:
        made.get_editor_property('mesh').set_static_mesh(model)
    return made


# ---------------------------------------------------------------------------
# Places
# ---------------------------------------------------------------------------

def place_places(build, mine):
    own = build.settings.get('mountain', {}).get('places', {})
    for name, spec in PLACES.items():
        tag = f'Place_{name}'
        if name in own:
            x, y = own[name][:2]
            story.marker(build, x, y, tag)
            continue
        kind = spec[0]
        if kind == 'mine':
            if mine is None:
                continue
            x, y, z = mine.at(*spec[1])
            build.place(unreal.TargetPoint, (x, y, z + story.MARKER_UP), 0.0, label=tag, folder=FOLDER, tags=(tag,))
        elif kind == 'zone':
            zone = story.layout_entry(build, 'zones', spec[1])
            if zone is None:
                continue
            x, y = story.centroid(zone['polygon'])
            story.marker(build, x, y, tag)
        elif kind == 'liftHead':
            head = first_placed(build, 'RR_LiftHead')
            step_spot = story.socket(head, 'LandingStep') if head is not None else None
            if step_spot is not None:
                x, y, z = xyz(step_spot.translation)
                build.place(unreal.TargetPoint, (x, y, z + story.MARKER_UP), 0.0, label=tag, folder=FOLDER, tags=(tag,))
            else:
                story.marker(build, LIFT_HEAD_PAD[0], LIFT_HEAD_PAD[1], tag)
        else:
            story.marker(build, spec[1][0], spec[1][1], tag)
    build.log(f'the mountain\'s places: {", ".join("Place_" + n for n in PLACES)}')


# ---------------------------------------------------------------------------
# Dead Lift: Hiram, the fittings, the smoke
# ---------------------------------------------------------------------------

def place_hiram(build, house):
    cls = story.actor_class('HiramCoyle')
    seat = at_socket(build, house, 'HiramStool')
    if cls is None or seat is None:
        if cls is None:
            build.warn('no HiramCoyle class (build the game module first): no Hiram')
        return
    yaw = seat.rotation.rotator().yaw
    ahead = seat.rotation.rotator().get_forward_vector()
    x, y, z = xyz(seat.translation)
    hiram = build.place(cls, (x + ahead.x * HIRAM_BACK, y + ahead.y * HIRAM_BACK, z - HIRAM_SEAT), yaw, label='Hiram',
                        folder=FOLDER, tags=('Speaker_Hiram',))
    tube = story.socket(house, 'SpeakingTube')
    if tube is not None:
        right = hiram.get_actor_right_vector()
        dx, dy = tube.translation.x - x, tube.translation.y - y
        hiram.set_editor_property('tube_side', 1.0 if right.x * dx + right.y * dy >= 0.0 else -1.0)
    # His words through the story, and his mutter as the lift crew rise (story_texts.py TOPICS and MUTTERS).
    speak(build, hiram.get_editor_property('speaker_point'), 'Speaker_Hiram')
    build.log(f'Hiram Coyle on his stool by the boiler ({x:.0f}, {y:.0f}), the tube on his '
              f'{"right" if hiram.get_editor_property("tube_side") > 0 else "left"}')


def place_fittings(build, house):
    cls = story.actor_class('MissionFitting')
    if cls is None:
        build.warn('no MissionFitting class (build the game module first): Dead Lift has no crank, boiler or pin to fit')
        return
    fit = step(build, DEAD_LIFT, 'fittings')
    usable = when(during=DEAD_LIFT, from_step=fit, before_step=fit + 1)
    done = [when(during=DEAD_LIFT, from_step=fit + 1), when(after=[DEAD_LIFT])]
    placed = 0
    # The pin goes into the bottom sheave's clamp, about 4.2 m up the gantry (OreLift.py): reached from the landing under
    # it, so its reach is the Interact key's longest (InteractionComponentFocus's 4 m).
    for name, socket_name, prompt, model_name, model_at, reach in (
            ('Boiler', 'Boiler', 'Stoke the boiler', 'SM_CoalSack', 'CoalBin', READ_REACH),
            ('Pin', 'ClampPin', 'Drive the clamp pin', 'SM_ClampPin', None, 390.0)):
        spot = at_socket(build, house, socket_name)
        if spot is None:
            continue
        model = mesh(build, model_name)
        if model is None:
            build.warn(f'no {model_name} yet (the props agent\'s): LiftFitting_{name} is fitted unseen')
        made = fitting(build, cls, xyz(spot.translation), spot.rotation.rotator().yaw, f'LiftFitting_{name}',
                       ('LiftFitting', f'LiftFitting_{name}'), prompt, usable, done, model=model, cue=CUES[name],
                       reach=reach)
        if spot.translation.z - house.get_actor_location().z > 300.0:
            build.warn(f'LiftFitting_{name} is {(spot.translation.z - house.get_actor_location().z) / 100.0:.1f} m over the '
                       'winch house\'s ground: check it can be reached from the landing in play')
        if model_at and model is not None:
            lies = story.socket(house, model_at)
            if lies is not None:
                part = made.get_editor_property('mesh')
                part.set_world_location_and_rotation(lies.translation, lies.rotation.rotator(), False, True)
        placed += 1
    # The winch's crank never left her (2026-10-21: Dead Lift's crank fetch went, the user's cut): it stands on its shaft
    # from the start, a plain model, nothing to fit.
    crank_at = at_socket(build, house, 'Crank')
    crank = mesh(build, 'SM_WinchCrank')
    if crank_at is not None and crank is not None:
        x, y, z = xyz(crank_at.translation)
        shaft = build.place(crank, (x, y, z), crank_at.rotation.rotator().yaw, label='LiftCrank_OnShaft', folder=FOLDER)
        shaft.set_actor_rotation(crank_at.rotation.rotator(), False)
    build.log(f'Dead Lift\'s fittings: {placed} (held, in its step {fit}; done from step {fit + 1} and after)')


def place_smoke(build, house):
    cls = story.actor_class('StoryProp')
    plume = mesh(build, 'SM_SmokePlume')
    spot = at_socket(build, house, 'Smoke')
    if cls is None or plume is None or spot is None:
        if cls is None or plume is None:
            build.warn('no StoryProp class or SM_SmokePlume: the winch house\'s stack stays cold')
        return
    smoke = build.place(cls, xyz(spot.translation), 19.0, label='Smoke_LiftHouse', folder=FOLDER)
    smoke.get_editor_property('mesh').set_static_mesh(plume)
    smoke.set_editor_property('shown_when', [when(during=DEAD_LIFT, from_step=step(build, DEAD_LIFT, 'steam')),
                                             when(after=[DEAD_LIFT])])
    build.log('smoke from the winch house\'s stack once she raises steam')


# ---------------------------------------------------------------------------
# The Last Watch: Ruth's cabin
# ---------------------------------------------------------------------------

def place_cabin(build, cabin, readable_cls, item_cls):
    import build_area_caches as caches
    board_cls = story.actor_class('NoticeBoard')
    board = at_socket(build, cabin, 'NoticeBoard')
    if board_cls is not None and board is not None:
        made = build.place(board_cls, xyz(board.translation), board.rotation.rotator().yaw, label='NoticeBoard_CalderPost',
                           folder=FOLDER, tags=('NoticeBoard_CalderPost',))
        made.set_editor_property('postings', [])
        made.set_editor_property('main_posting', unreal.Name(''))
        made.set_editor_property('title', unreal.Text('Rim Rangers, Larkspur Post'))
        made.set_editor_property('subtitle', unreal.Text('Posted at the cabin door'))
        made.set_editor_property('prompt', unreal.Text('Read the board'))
        made.set_editor_property('locked_note', unreal.Text(''))
        build.log('Calder\'s Post\'s board (NoticeBoard_CalderPost) at the cabin door')
    elif board_cls is None:
        build.warn('no NoticeBoard class (build the game module first): Calder\'s Post\'s board offers nothing')
    if readable_cls is not None:
        for readable_id, socket_name in (('RuthJournal', 'Journal'), ('PostLog', 'PostLog')):
            spot = at_socket(build, cabin, socket_name)
            if spot is not None:
                readable(build, readable_cls, readable_id, xyz(spot.translation), spot.rotation.rotator().yaw)
        place_code_book(build, cabin, readable_cls)
    oil = at_socket(build, cabin, 'LampOil')
    if item_cls is not None and oil is not None:
        item(build, item_cls, 'LampOil', xyz(oil.translation), oil.rotation.rotator().yaw, 'SM_LampOil')
    cache = at_socket(build, cabin, 'Cache')
    chest_cls = story.actor_class('Chest')
    models = caches.models(build, 'SupplyCrate') if chest_cls is not None else None
    if cache is not None and models is not None:
        chest = caches.place_chest(build, chest_cls, 'SupplyCrate', models, 'CalderCache_Post', xyz(cache.translation),
                                   cache.rotation.rotator().yaw, ('CalderCache',))
        chest.set_editor_property('open_when', when(after=[LAST_WATCH]))
        build.log('Calder\'s fourth cache (CalderCache_Post) in the cabin, opening after The Last Watch')
    build.log('Ruth\'s cabin: the journal, the post\'s log, the code book, the lamp oil')


def place_code_book(build, cabin, readable_cls):
    """The code book: readable (the far post's answer) only in The Last Watch's code book step; on the shelf otherwise."""
    spot = at_socket(build, cabin, 'CodeBook')
    if spot is None:
        return
    reading = step(build, LAST_WATCH, 'codeBook')
    in_step = when(during=LAST_WATCH, from_step=reading, before_step=reading + 1)
    readable(build, readable_cls, 'CodeBook', xyz(spot.translation), spot.rotation.rotator().yaw, shown_when=in_step)
    story_cls = story.actor_class('StoryProp')
    model = mesh(build, READABLES['CodeBook']['model'])
    if story_cls is not None and model is not None:
        book = build.place(story_cls, xyz(spot.translation), spot.rotation.rotator().yaw, label='CodeBook_OnTheShelf',
                           folder=FOLDER)
        book.get_editor_property('mesh').set_static_mesh(model)
        book.set_editor_property('hidden_when', [in_step])


# ---------------------------------------------------------------------------
# All of it
# ---------------------------------------------------------------------------

def place(build):
    """Every mission prop of the mountain and its fights (build_area.py's gameplay() calls this after the story's)."""
    for name in ('build_area_mine', 'build_area_mountain', 'build_area_fights_mountain', 'build_area_providence'):
        importlib.reload(importlib.import_module(name))
    import build_area_fights_mountain as fights
    import build_area_mine as mine_kit
    import build_area_mountain as mountain
    import build_area_providence as providence
    if not mountain.is_mountain(build):
        return
    _MESHES.clear()
    mine = mine_kit.frame(build, quiet=True)
    readable_cls = story.actor_class('ReadableProp')
    item_cls = story.actor_class('MissionItem')
    if readable_cls is None or item_cls is None:
        build.warn('no ReadableProp or MissionItem class (build the game module first): the mountain\'s readables and '
                   'items are left out')
    place_places(build, mine)
    house = first_placed(build, 'RR_LiftHouse')
    if house is not None:
        place_hiram(build, house)
        place_fittings(build, house)
        place_smoke(build, house)
    cabin = first_placed(build, 'RR_RangerCabin')
    if cabin is not None:
        place_cabin(build, cabin, readable_cls, item_cls)
    # Back Pay's at the tag board and in the mine (build_area_providence.py, with these helpers).
    providence.place(build, mine, readable_cls, item_cls, sys.modules[__name__])
    fights.place(build, mine)
    build.log('the mountain\'s mission props placed')


def run(name):
    """Places them again on their own (clears Gameplay/Mountain; the Foreman's vigil stone needs a gameplay build), saves."""
    import build_area
    build = importlib.reload(build_area).AreaBuild(name)
    with open(build.computed_path) as f:
        build.layout = json.load(f)
    build.open_level(FOLDER)
    place(build)
    unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
    build.log('the mountain\'s mission props placed and saved')


if __name__ == '__main__':
    if len(sys.argv) < 2:
        raise SystemExit('usage: build_area_missionprops_mountain.py <Area> (a folder under Art/Levels)')
    run(sys.argv[1])
