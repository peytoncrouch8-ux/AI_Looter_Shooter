"""The story's actors for build_area.py (Docs/Areas/RansomsRest.md, Main 1 "Seven Days"), placed from the models
build_area.py placed and their sockets (Art/Models/Props/Graves.py, Buildings/Lookout.py, Buildings/Farmhouse.py), so
they follow the level whenever it's rebuilt. Only a level whose layout has Ellis's grave (Ransom's Rest) gets any of it,
and each piece waits for what it's placed from (no lookout: no cold open). Everything goes in the area's Gameplay
folder, so a "gameplay" build places it all again.

- The cold open's set (AColdOpenSet) where the lookout stands, facing as it faces: its marks on Ransom's Point are the
  lookout's (Scenes/ColdOpenSet.h); the gang's skiff's course is in the world's own frame.
- The family plot's respawn grave (ARespawnMarker FamilyPlot, open once Main1 is finished) at the foot of Ellis's grave,
  where the wake-up stands the player (GraveWake::StandOut), facing as the level's start does (toward the bell tower).
- The two headboards to read (ASpeakerPoint tagged Headboard_Ellis and Headboard_Abel: "Read", from 2.5 m) at the graves'
  Interact sockets.
- Grandma Delia's screen door (ASpeakerPoint tagged Speaker_Delia) on the farmhouse's front door: her Main 1 lines while
  it lasts, the porch after.
- Hob (AHobBird) on Ellis's headboard: silent from the claw-out on, his "Morning, sunshine" once Ellis is out, and there
  after Main 1. Talked to, a line for where things stand.
Their words are line sets in /Game/Data/Story (Tools/Unreal/create_story_lines.py makes them first); a set that's
missing is left out with a warning, and its speaker says nothing until the sets are made and this runs again.
"""
import unreal

CLASSES = '/Script/AI_Looter_Shooter.'
LINES = '/Game/Data/Story/'

# The story's ids and tags, as the C++ and the mission asset (Tools/Unreal/create_mission_assets.py) name them.
MAIN1 = 'Main1'
FAMILY_PLOT = 'FamilyPlot'

# How far out of the grave's foot the wake-up stands the player, from the hole's middle (cm; GraveWake::StandOut).
STAND_OUT = 190.0
# Hob's grip on top of a headboard, from its Interact socket (55 cm up its face): up to the board's top, and back over
# its thickness and lean (cm).
PERCH_UP = 30.0
PERCH_BACK = 12.0
# The farmhouse's front door, in its model's frame (Farmhouse.py: the door in the middle of the front wall, which stands
# 3.1 m out from the pivot, its sill on the 0.6 m foundation): the speaker point stands on its face.
DOOR = unreal.Vector(312.0, 0.0, 60.0)
# Reading a headboard is done close to (cm).
READ_REACH = 250.0


def actor_class(name):
    return unreal.load_class(None, CLASSES + name)


def placed(build, kind):
    """The actor build_area.py placed for the first placement of this kind (labelled with its key), or None."""
    keys = [key for key, spot in build.layout['placements'].items() if spot['kind'] == kind]
    if not keys:
        return None
    level_actors = unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
    for actor in level_actors:
        if actor.get_actor_label() == keys[0] and unreal.Name(build.tag) in actor.tags:
            return actor
    build.warn(f'no {kind} placed for {keys[0]} (build the whole level first): what stands on it is left out')
    return None


def lines(build, name):
    """A line set the story script made, or None (with a warning)."""
    path = LINES + name
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        build.warn(f'no {path} yet (run Tools/Unreal/create_story_lines.py first)')
        return None
    return unreal.load_asset(path)


def condition(after=(), before=(), during='', from_step=0):
    """FStoryCondition: after these missions, before those, during one (from a step, counted from 0)."""
    made = unreal.StoryCondition()
    made.set_editor_property('after_missions', [unreal.Name(m) for m in after])
    made.set_editor_property('before_missions', [unreal.Name(m) for m in before])
    made.set_editor_property('during_mission', unreal.Name(during))
    made.set_editor_property('from_step', from_step)
    return made


def topic(when, line_set):
    made = unreal.SpeakerTopic()
    made.set_editor_property('when', when)
    if line_set:
        made.set_editor_property('line_set', line_set)
    return made


def socket(actor, name):
    """A socket of the actor's model in the world (an unreal.Transform), or None when the model has none so named."""
    mesh = actor.static_mesh_component
    if mesh is None or not mesh.does_socket_exist(name):
        return None
    return mesh.get_socket_transform(name, unreal.RelativeTransformSpace.RTS_WORLD)


def ground(x, y, z, ignore=()):
    """The ground's height under (x, y), looking from a little above z, past the actors in ignore (the grave itself,
    whose heap reaches toward its foot), as the wake-up's own trace does (GraveWake.cpp)."""
    world = unreal.EditorLevelLibrary.get_editor_world()
    hit = unreal.SystemLibrary.line_trace_single(world, unreal.Vector(x, y, z + 150.0), unreal.Vector(x, y, z - 300.0),
                                                 unreal.TraceTypeQuery.TRACE_TYPE_QUERY1, True, list(ignore),
                                                 unreal.DrawDebugTrace.NONE, True)
    return z if hit is None else hit.to_tuple()[5].z


def speaker(build, cls, location, yaw, label, tag, name='', prompt='', reach=0.0, line_set=None, topics=()):
    """A speaker point (a door, a headboard) with its point at location when reach is set (a headboard's face), else
    over it (a door's middle)."""
    point = build.place(cls, location, yaw, label=label, folder='Gameplay', tags=(tag,))
    talk = point.get_editor_property('speaker_point')
    talk.set_editor_property('speaker_name', unreal.Text(name))
    if prompt:
        talk.set_editor_property('prompt', unreal.Text(prompt))
    if reach:
        talk.set_editor_property('reach', reach)
        talk.set_relative_location(unreal.Vector(0.0, 0.0, 0.0), False, True)
    if line_set:
        talk.set_editor_property('line_set', line_set)
    if topics:
        talk.set_editor_property('topics', list(topics))
    return point


def place_cold_open(build):
    lookout = placed(build, 'Lookout')
    cls = actor_class('ColdOpenSet')
    if lookout is None or cls is None:
        if cls is None:
            build.warn('no ColdOpenSet class (build the game module first): no cold open')
        return
    at = lookout.get_actor_location()
    build.place(cls, (at.x, at.y, at.z), lookout.get_actor_rotation().yaw, label='ColdOpenSet', folder='Gameplay')
    build.log(f'the cold open\'s set at the lookout ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')


def place_family_plot(build):
    grave = placed(build, 'Grave_Ellis')
    marker_cls = actor_class('RespawnMarker')
    if grave is None or marker_cls is None:
        return None
    rise = socket(grave, 'Respawn')
    hole = rise.translation if rise else grave.get_actor_location()
    foot = grave.get_actor_forward_vector()
    x, y = hole.x + foot.x * STAND_OUT, hole.y + foot.y * STAND_OUT
    spawn = next((s for s in build.layout['placements'].values() if s['kind'] == 'PlayerStart'), None)
    yaw = spawn['yaw'] if spawn else grave.get_actor_rotation().yaw
    marker = build.place(marker_cls, (x, y, ground(x, y, hole.z, ignore=[grave])), yaw, label='Respawn_FamilyPlot',
                         folder='Gameplay')
    marker.set_editor_property('marker_id', unreal.Name(FAMILY_PLOT))
    marker.set_editor_property('active_after_mission', unreal.Name(MAIN1))
    build.log(f'the family plot\'s respawn grave at ({x:.0f}, {y:.0f}), open after {MAIN1}')
    return grave


def place_headboards(build, grave_ellis):
    cls = actor_class('SpeakerPoint')
    if cls is None:
        return
    for kind, tag, line_set in (('Grave_Ellis', 'Headboard_Ellis', 'DA_Lines_HeadboardEllis'),
                                ('Grave_Abel', 'Headboard_Abel', 'DA_Lines_HeadboardAbel')):
        grave = grave_ellis if kind == 'Grave_Ellis' else placed(build, kind)
        face = socket(grave, 'Interact') if grave else None
        if face is None:
            if grave:
                build.warn(f'{kind} has no Interact socket: its headboard can\'t be read')
            continue
        at = face.translation
        speaker(build, cls, (at.x, at.y, at.z), face.rotation.rotator().yaw, tag, tag, prompt='Read', reach=READ_REACH,
                line_set=lines(build, line_set))
    build.log('the headboards, to read')


def place_delia(build):
    farmhouse = placed(build, 'Farmhouse')
    cls = actor_class('SpeakerPoint')
    if farmhouse is None or cls is None:
        return
    door = farmhouse.get_actor_transform().transform_location(DOOR)
    during = lines(build, 'DA_Lines_DeliaMain1')
    speaker(build, cls, (door.x, door.y, door.z), farmhouse.get_actor_rotation().yaw, 'Speaker_Delia', 'Speaker_Delia',
            name='Grandma Delia', line_set=lines(build, 'DA_Lines_DeliaPorch'),
            topics=[topic(condition(before=[MAIN1]), during)] if during else ())
    build.log(f'Grandma Delia\'s screen door at ({door.x:.0f}, {door.y:.0f}, {door.z:.0f})')


def place_hob(build, grave_ellis):
    cls = actor_class('HobBird')
    face = socket(grave_ellis, 'Interact') if grave_ellis else None
    if cls is None or face is None:
        return
    # A Quat has no forward vector in Python: the rotator's does.
    front = face.rotation.rotator().get_forward_vector()
    at = face.translation
    x, y, z = at.x - front.x * PERCH_BACK, at.y - front.y * PERCH_BACK, at.z + PERCH_UP
    yaw = face.rotation.rotator().yaw

    def perch(when, arrival=None):
        made = unreal.HobPerch()
        made.set_editor_property('when', when)
        made.set_editor_property('location', unreal.Vector(x, y, z))
        made.set_editor_property('yaw', yaw)
        if arrival:
            made.set_editor_property('arrival_set', arrival)
        return made

    hob = build.place(cls, (x, y, z), yaw, label='Hob', folder='Gameplay', tags=('Speaker_Hob',))
    # The first that holds is his: on the board saying his piece once Ellis is out (Main 1's step 3, from 0: 2), on it
    # silently while Ellis claws out (step 2), and on it after Main 1.
    hob.set_editor_property('perches', [
        perch(condition(during=MAIN1, from_step=2), lines(build, 'DA_Lines_HobWakes')),
        perch(condition(during=MAIN1, from_step=1)),
        perch(condition(after=[MAIN1])),
    ])
    talk = hob.get_editor_property('speaker_point')
    talk.set_editor_property('topics', [
        topic(condition(during=MAIN1), lines(build, 'DA_Lines_HobMain1')),
        topic(condition(after=[MAIN1]), lines(build, 'DA_Lines_HobAfterMain1')),
    ])
    build.log(f'Hob on Ellis\'s headboard at ({x:.0f}, {y:.0f}, {z:.0f})')


def place(build):
    """Everything above, in the Gameplay folder (build_area.py's gameplay() calls this last). Only the Ransom farm has
    it: the level whose layout has Ellis's grave (the tutorial island has a farmhouse too, and no Delia behind its door)."""
    if not any(spot['kind'] == 'Grave_Ellis' for spot in build.layout['placements'].values()):
        return
    place_cold_open(build)
    grave_ellis = place_family_plot(build)
    place_headboards(build, grave_ellis)
    place_delia(build)
    place_hob(build, grave_ellis)
