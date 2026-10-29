"""Main 4, "Hallowed Ground" (Docs/Areas/RansomsRest.md), for build_area_story.py: the story's pieces at the Chapel of
Saint Ada, placed from the chapel build_area.py placed (SM_Chapel, Art/Models/Buildings/Chapel.py: its sockets) and from
the layout's zones, obstacles, roads and placements, so they follow the level whenever it's rebuilt. build_area_story's
place() calls place() here (reloaded each run, as the editor keeps modules between runs) and gets Hob's two chapel perches
back for his. It uses build_area_story's helpers. Everything goes in the area's Gameplay folder; a piece whose model or
class isn't there yet is left out with a warning.

- The chapel's place (a target point tagged Place_Chapel) at the chapel's middle: "Go up to the Chapel of Saint Ada" ends
  within 15 m of it.
- The chapel yard's fight (ASetPieceSpawner ChapelYard, "Saint Ada's Yard", since 2026-10-28 an arena: YARD_PUSHES, placed
  through build_area_fights_mountain.place_one) on the chapel's spot, turned as the chapel is, its creatures' spots round
  the chapel in the chapel's own frame (YARD_SPOTS): three pushes, 6 Unpaid; 5 and a Restless leader; 4 and a Gravebound
  leader (the Sexton's Debt), the Bell-Ringer on the vestry steps from the second, a fog gate across the fence's gateway
  while it is fought, tagged Unpaid_ChapelYard. On during
  Main 4's second step only, springing as the player comes near, hunting only inside the churchyard's iron fence (the
  obstacle churchyardFence) and its 2 m margin. The yard is levelled on the knoll's top (layout churchyardPad, 7.5 m;
  2026-10-12: before, the knoll fell about 8 m from the chapel's door to the fence's far corners and the limits were 9 m
  and 10 m), so it's one level of ground with the spawner's own limits: spots up to 4 m above or below the chapel's
  floor, players up to 6 m.
- The bell (AChapelBell tagged Bell_Chapel) at the rope's grip (SOCKET_Interact), its bell (SM_ChapelBell) hung on the
  belfry's SOCKET_Bell: "Ring the chapel bell", any time.
- The smashed Reliquary (AChapelReliquary tagged Reliquary_Chapel) on the apse's plinth (SOCKET_Reliquary), facing the
  nave, looked at from Main 4's fourth step on: its model (the first of RELIQUARY_MESHES imported: the art session's
  Art/Models/Props/Reliquary.py, export family RR_Reliquary, with SOCKET_Ember and SOCKET_Interact), else the class's
  stand-in.
- Father Aldana's vestry door (ASpeakerPoint tagged Speaker_Aldana) on SOCKET_Speaker: barred before the yard is quiet
  (and before Main 4), the bell and the Reliquary next, the doc's words at Main 4's last step, the Sink after, and the
  lantern found after Main 5 (take it home to Delia).
- (The chapel yard's respawn grave is gone: Hob's roost at the chapel knoll's gate replaced it, build_area_roosts.py, the
  layout's gameplay.roosts ChapelYard, the same id and still open after Main 4. The fresh mound stays, as dressing.)
- The roaming Unpaid after Main 4 ("From now on the Unpaid walk the north road and boot hill"): AEncounterSpawner BootHill
  on boot hill (the zone bootHill its ground) and NorthRoad between the hanging tree and the north road (25 m round it),
  five each (three until 2026-10-21), their ranks the area's (8% Restless, 2% Gravebound), standing about until they notice the player, tagged
  Unpaid_<id>. On from Main 4's finish for good; like every encounter, a level loaded again starts them over.
"""
import importlib
import math

import unreal

import build_area_story as story

# The story's ids and tags, as the C++ (AChapelBell, AChapelReliquary) and the mission asset
# (Tools/Unreal/create_mission_assets.py, DA_Mission_Main4) name them.
MAIN4 = 'Main4'
MAIN5 = 'Main5'
PLACE_CHAPEL = 'Place_Chapel'
YARD = 'ChapelYard'
YARD_TAG = 'Unpaid_ChapelYard'
BELL_TAG = 'Bell_Chapel'
RELIQUARY_TAG = 'Reliquary_Chapel'
ALDANA_TAG = 'Speaker_Aldana'
BOOT_HILL = 'BootHill'
NORTH_ROAD = 'NorthRoad'

# The models: the chapel's bell, and the smashed Reliquary. Its name is still to be confirmed by the art session: the
# planned SM_Reliquary_Smashed, or SM_Reliquary as Reliquary.py names its object today; the first imported is used.
BELL_MESH = '/Game/Art/Buildings/SM_ChapelBell'
RELIQUARY_MESHES = ('/Game/Art/Props/SM_Reliquary_Smashed', '/Game/Art/Props/SM_Reliquary')

# The chapel's own frame for the yard's spots (cm): +X its front (the tower, the door, the road), -X its apse, +Y the side
# away from the vestry (the west grave rows), -Y the vestry's side (the east rows). The spots: the lawn before the door,
# the lanes either side of the nave between it and the grave rows, behind the apse, and behind the vestry, all inside the
# fence (X -1200 to 1000, Y -1700 to 1700 here). The spawner takes them in turn, skipping any within 8 m of the player.
# The dressing's graves, trees and props (build_area_dressing.py) keep YARD_CLEAR (cm) from them, so nobody rises in a
# headboard or a trunk; place_yard warns of any that doesn't (the fence's line is the yard's edge, and isn't counted).
YARD_SPOTS = (
    (800.0, 900.0), (800.0, -700.0), (850.0, 1500.0), (850.0, -1450.0), (100.0, 560.0), (-450.0, 560.0),
    (-950.0, 0.0), (-1000.0, 500.0), (-1000.0, -500.0), (150.0, -600.0), (-750.0, -700.0),
)
YARD_CLEAR = 150.0
# Saint Ada's Yard (FIGHTS lane 6, 2026-10-28; the user: "clearly harder", an elite in most fights, Main's call: the arena at
# the chapel yard only): the yard fight is a set piece in three pushes of Unpaid that rise from the yard's spots (indexes into
# YARD_SPOTS): 6; then 5 and a Restless leader; then 4 and a Gravebound leader, "the Sexton's Debt" (17 in all, the Bell-Ringer
# on the vestry steps from the second push besides: 18). At most 9 out at once; the next push comes when 3 are left or 35 s
# after the last. A fog gate shuts the iron fence's gateway (the south side, before the chapel's door) once the player is
# inside, and opens when the Unpaid are down or the player dies; the shower is the set piece's own (2 guns, 2 ammo, a box).
YARD_PUSHES = (
    # (Basic Unpaid, the leader's rank or None, the leader's name or None, the push's spots)
    (6, None, None, [0, 1, 2, 3, 4, 9]),
    (5, 'Rare', None, [5, 10, 7, 8, 4, 9]),
    (4, 'Epic', "the Sexton's Debt", [6, 7, 8, 0, 1]),
)
YARD_MAX_ALIVE = 9
YARD_PUSH_WHEN_LEFT = 3
YARD_PUSH_AFTER = 35.0
# The set piece springs this near the chapel (cm): the fence (X 5600-7800, Y -3600 to -200) and a little more, not on
# walking in (the player is already inside at Main 4's second step, standing by the chapel).
YARD_SPRING = 3200.0
# The fence's gateway (the double gate on the chapel's axis: build_area_dressing.py 'churchyardFence' gates) and the fog gate
# across it, from end to end (layout cm; the gateway is 2 m wide, the gate overlaps the fence 2.5 m either side), on the
# fence's south line; the yard lies north of it.
YARD_GATES = (((5600.0, -2250.0), (5600.0, -1550.0)),)
YARD_GATE_MARGIN = 150.0
# The Bell-Ringer stands this far out of Aldana's vestry door (SOCKET_Speaker) on the steps, tolling from the second push
# (cm), else this far behind the vestry in the chapel's frame.
VESTRY_OUT = 350.0
VESTRY_FALLBACK = (-200.0, -1050.0)
# The yard as one level of ground (cm): spots this far above or below the chapel's floor, players this far (the
# spawner's defaults, now the yard is flat).
YARD_STEP = 400.0
YARD_RISE = 600.0
# Hob's words for it (the beats' speaker; a caption), and the cues each push comes with (existing cues).
YARD_BEATS = (
    dict(wave=1, speaker='Hob', line='They were waiting in the graves, sunshine. And the fog has shut the gate behind you. Hold the yard.',
         sting='Boss.Phase', cue='Creature.Unpaid.Shriek', shake=0.2, shakeSeconds=0.6),
    dict(wave=2, speaker='Hob', line='Bell-ringer on the vestry steps! He keeps them coming. Drop the loud one first, sunshine.',
         sting='Creature.RankSting', shake=0.15, shakeSeconds=0.5),
    dict(wave=3, speaker='Hob', line="That's the Sexton's Debt walking up. Put him down and the rest lose their nerve.",
         sting='Boss.Phase', cue='Boss.Gravemother.Quake', shake=0.3, shakeSeconds=0.8),
)
YARD_CLEARED = dict(speaker='Hob', line='Fog is lifting. Quiet as a church, sunshine. Go knock on the vestry door.', cue='Boss.Loot.Burst')

# The roaming Unpaid after Main 4: how many Basic-or-promoted ones on each stretch (and a Restless leader on top: six),
# how far the north road's runs round its spawner, and how far each spreads them out (cm).
ROAMING_COUNT = 5
ROAD_STRETCH = 2500.0
ROAD_SPREAD = 900.0
HILL_SPREAD = 600.0


def nearest_on_path(path, point):
    """The point of a polyline ((x, y) pairs) nearest point, seen from above."""
    best, best_d = None, None
    for (ax, ay), (bx, by) in zip(path, path[1:]):
        dx, dy = bx - ax, by - ay
        length = dx * dx + dy * dy
        t = 0.0 if length < 1e-6 else max(0.0, min(1.0, ((point[0] - ax) * dx + (point[1] - ay) * dy) / length))
        x, y = ax + dx * t, ay + dy * t
        d = (x - point[0]) ** 2 + (y - point[1]) ** 2
        if best_d is None or d < best_d:
            best, best_d = (x, y), d
    return best


def roaming_group(cls, count):
    """FEncounterGroup: count of cls, their ranks the area's promotions ("plus 8% of spawns" Restless, 2% Gravebound)."""
    made = unreal.EncounterGroup()
    made.set_editor_property('creature_class', cls)
    made.set_editor_property('count', count)
    made.set_editor_property('rank_roll', unreal.EncounterRankRoll.AREA)
    return made


def leader_group(cls):
    """FEncounterGroup: one Restless Unpaid that leads its pack (FIGHTS lane 6: it wears Rallying; its fall makes them falter)."""
    made = unreal.EncounterGroup()
    made.set_editor_property('creature_class', cls)
    made.set_editor_property('count', 1)
    made.set_editor_property('rank_roll', unreal.EncounterRankRoll.FIXED)
    made.set_editor_property('rank', unreal.CreatureRank.RARE)
    made.set_editor_property('leader', True)
    return made


# ---------------------------------------------------------------------------
# The chapel
# ---------------------------------------------------------------------------

def place_chapel_marker(build, chapel):
    """The chapel's middle, on the ground under it, where "Go up to the Chapel of Saint Ada" points."""
    at = chapel.get_actor_location()
    z = story.ground(at.x, at.y, at.z, ignore=[chapel]) + story.MARKER_UP
    build.place(unreal.TargetPoint, (at.x, at.y, z), 0.0, label=PLACE_CHAPEL, folder='Gameplay', tags=(PLACE_CHAPEL,))
    build.log(f'the chapel\'s middle at ({at.x:.0f}, {at.y:.0f}), tagged {PLACE_CHAPEL}')


def chapel_point(chapel, x, y):
    """A point in the chapel's own frame (cm) as a layout point (x, y): the chapel's spot, turned as it is."""
    at = chapel.get_actor_location()
    yaw = math.radians(chapel.get_actor_rotation().yaw)
    return [at.x + math.cos(yaw) * x - math.sin(yaw) * y, at.y + math.sin(yaw) * x + math.cos(yaw) * y]


def vestry_steps(chapel):
    """Where the Bell-Ringer stands: out of Aldana's vestry door (the chapel's Speaker socket) on the steps, else behind the
    vestry. A layout point (x, y)."""
    door = story.socket(chapel, 'Speaker')
    if door is None:
        return chapel_point(chapel, *VESTRY_FALLBACK)
    yaw = math.radians(door.rotation.rotator().yaw)
    return [door.translation.x + math.cos(yaw) * VESTRY_OUT, door.translation.y + math.sin(yaw) * VESTRY_OUT]


def yard_entry(chapel, fence):
    """Saint Ada's Yard as a gameplay.encounters.fights entry (build_area_fights_mountain.place_one's format, with a
    setPiece block: build_area_setpieces.py): three pushes of Unpaid with their leaders, the Bell-Ringer from the second,
    the fog gate across the fence's gateway, Hob's words. Its spots are the chapel's, turned as it is."""
    at = chapel.get_actor_location()
    spots = [chapel_point(chapel, x, y) for x, y in YARD_SPOTS]
    groups, beats = [], []
    for wave, (basic, leader_rank, leader_name, indexes) in enumerate(YARD_PUSHES, start=1):
        groups.append({'class': 'UnpaidCreature', 'count': basic, 'rank': 'Basic', 'firstWave': wave, 'lastWave': wave})
        if leader_rank:
            leader = {'class': 'UnpaidCreature', 'count': 1, 'rank': leader_rank, 'firstWave': wave, 'lastWave': wave, 'leader': True}
            if leader_name:
                leader['name'] = leader_name
            groups.append(leader)
        beat = dict(YARD_BEATS[wave - 1])
        beat['spots'] = [spots[i] for i in indexes]
        beats.append(beat)
    return {
        'id': YARD, 'spawner': 'SetPieceSpawner', 'pass': 'valley',
        'where': "the chapel yard: Saint Ada's Yard, Main 4's arena (a set piece: the dead rise in three pushes behind a fog gate)",
        'when': {'during': MAIN4, 'fromStep': 1, 'beforeStep': 2},
        'groups': groups, 'tags': [YARD_TAG], 'center': [at.x, at.y], 'spots': spots,
        'huntOnSpawn': True, 'entrance': 'Rise',
        'waves': {'count': len(YARD_PUSHES), 'interval': YARD_PUSH_AFTER, 'waitForClear': False, 'maxAlive': YARD_MAX_ALIVE},
        'ground': {'polygon': fence['polygon'], 'maxStep': YARD_STEP, 'maxRise': YARD_RISE},
        'setPiece': {
            'doneFlag': 'SetPiece.ChapelYard', 'waves': len(YARD_PUSHES), 'pushWhenLeft': YARD_PUSH_WHEN_LEFT,
            'pushAfter': YARD_PUSH_AFTER, 'springRadius': YARD_SPRING,
            'beats': beats, 'cleared': dict(YARD_CLEARED),
            'placed': [{'class': 'BellRingerCreature', 'rank': 'Basic', 'wave': 2, 'at': vestry_steps(chapel)}],
            'reward': chapel_point(chapel, 700.0, 0.0),
            'arena': {'gates': [[list(a), list(b)] for a, b in YARD_GATES], 'margin': YARD_GATE_MARGIN},
        },
    }


def place_yard(build, chapel):
    """The chapel yard's fight as the set piece Saint Ada's Yard (spawner id ChapelYard, the missions' and the tests' name
    for it): 17 Unpaid in three pushes with a Restless and a Gravebound leader, a Bell-Ringer from the second, a fog gate."""
    fence = story.layout_entry(build, 'obstacles', 'churchyardFence')
    if fence is None:
        build.warn('no churchyardFence in the layout: no fight in the chapel yard')
        return
    fights = importlib.reload(importlib.import_module('build_area_fights_mountain'))
    fights.place_one(build, yard_entry(chapel, fence), None)
    unpaid = sum(basic + (1 if rank else 0) for basic, rank, _, _ in YARD_PUSHES)
    build.log(f"the chapel yard's set piece ({YARD}: {unpaid} Unpaid in {len(YARD_PUSHES)} pushes and a Bell-Ringer, a fog gate "
              f"at the fence's gateway) round the chapel, inside the churchyard fence, during {MAIN4}'s second step")
    check_yard_clear(build, chapel)


def check_yard_clear(build, chapel):
    """Warns of the dressing's pieces (build_area_dressing.footprints: the churchyard's graves and dead trees among
    them) standing within YARD_CLEAR of a yard spot, as the spawner turns the spots with the chapel. The fence's line,
    the yard's edge, isn't counted."""
    # Reloaded, as the editor keeps modules between runs: a gameplay build may come after the dressing's tables changed.
    dressing = importlib.reload(importlib.import_module('build_area_dressing'))
    at = chapel.get_actor_location()
    yaw = math.radians(chapel.get_actor_rotation().yaw)
    spots = [(at.x + math.cos(yaw) * x - math.sin(yaw) * y, at.y + math.sin(yaw) * x + math.cos(yaw) * y)
             for x, y in YARD_SPOTS]
    crowded = []
    for x, y, piece_yaw, half_y, half_x, line in dressing.footprints(build.source, build.layout['placements']):
        if line:
            continue
        turn = math.radians(piece_yaw)
        for sx, sy in spots:
            # The spot in the piece's own frame (its X its front), and how far it is outside the piece's box.
            dx, dy = sx - x, sy - y
            ahead, right = dx * math.cos(turn) + dy * math.sin(turn), -dx * math.sin(turn) + dy * math.cos(turn)
            if math.hypot(max(abs(ahead) - half_x, 0.0), max(abs(right) - half_y, 0.0)) < YARD_CLEAR:
                crowded.append(f'({sx:.0f}, {sy:.0f})')
    if crowded:
        build.warn(f'the dressing stands within {YARD_CLEAR / 100.0:.1f} m of the chapel yard\'s spots at '
                   f'{", ".join(sorted(set(crowded)))}: move the piece (build_area_dressing.py) or the spot (YARD_SPOTS)')


def place_bell(build, chapel):
    """The bell rung from the rope's grip, the bell itself hung on the belfry's socket."""
    cls = story.actor_class('ChapelBell')
    grip = story.socket(chapel, 'Interact')
    axis = story.socket(chapel, 'Bell')
    if cls is None or grip is None or axis is None:
        build.warn('no ChapelBell class (build the game module first): no bell to ring' if cls is None
                   else 'the chapel has no Interact or Bell socket: no bell to ring')
        return
    at = grip.translation
    bell = build.place(cls, (at.x, at.y, at.z), chapel.get_actor_rotation().yaw, label='ChapelBell', folder='Gameplay',
                       tags=(BELL_TAG,))
    swing = bell.get_editor_property('bell')
    if unreal.EditorAssetLibrary.does_asset_exist(BELL_MESH):
        swing.set_static_mesh(unreal.load_asset(BELL_MESH))
    else:
        build.warn(f'no {BELL_MESH} yet (import Chapel.py): the bell rings unseen')
    # Its origin on the headstock's axis, turned as the socket is: its X along the axis, as the code swings it.
    swing.set_world_location_and_rotation(axis.translation, axis.rotation.rotator(), False, True)
    build.log(f'the chapel bell, rung from ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}), hung at {axis.translation.z:.0f} cm')


def place_reliquary(build, chapel):
    """The smashed Reliquary on the apse's plinth, facing the nave: its model when imported, else its stand-in."""
    cls = story.actor_class('ChapelReliquary')
    plinth = story.socket(chapel, 'Reliquary')
    if cls is None or plinth is None:
        build.warn('no ChapelReliquary class (build the game module first): no Reliquary' if cls is None
                   else 'the chapel has no Reliquary socket: no Reliquary')
        return
    at = plinth.translation
    reliquary = build.place(cls, (at.x, at.y, at.z), plinth.rotation.rotator().yaw, label='Reliquary', folder='Gameplay',
                            tags=(RELIQUARY_TAG,))
    # Snapped to the socket: its whole turn, not only its yaw.
    reliquary.set_actor_rotation(plinth.rotation.rotator(), False)
    model = next((path for path in RELIQUARY_MESHES if unreal.EditorAssetLibrary.does_asset_exist(path)), None)
    if model:
        reliquary.get_editor_property('mesh').set_static_mesh(unreal.load_asset(model))
    else:
        build.warn(f'no {" or ".join(RELIQUARY_MESHES)} yet (Art/Models/Props/Reliquary.py, the art session\'s): the '
                   f'Reliquary stands in plain shapes until it\'s imported and this runs again')
    # From Main 4's fifth step on (from 0: 4; the knock at Aldana's door is the third, the bell the fourth). Set last, so
    # the editor builds it again and shows what play will.
    reliquary.set_editor_property('look_when', story.condition(during=MAIN4, from_step=4))
    build.log(f'the smashed Reliquary ({model or "its stand-in"}) on the plinth at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}), '
              f'looked at from {MAIN4}\'s fifth step')


def place_aldana(build, chapel):
    """Father Aldana's vestry door: barred, the bell and the Reliquary, the doc's words, the Sink, the lantern found."""
    cls = story.actor_class('SpeakerPoint')
    door = story.socket(chapel, 'Speaker')
    if cls is None or door is None:
        if cls is not None:
            build.warn('the chapel has no Speaker socket: Father Aldana can\'t be talked to')
        return
    at = door.translation
    # The first that holds is said (story_texts.py TOPICS): the bell and the Reliquary (from 0: 3 on), the knock (2), after
    # Main 5 (the lantern found), after Main 4 (the Sink); his Main 4 words are the turn-in's. With none (before Main 4, and
    # while the yard is held), the door stays barred.
    story.speaker(build, cls, (at.x, at.y, at.z), door.rotation.rotator().yaw, 'Speaker_Aldana', ALDANA_TAG,
                  name='Father Aldana', reach=story.TALK_REACH, line_set=story.lines(build, 'DA_Lines_AldanaBarred'),
                  topics=story.texts_topics(build, 'Speaker_Aldana'))
    build.log(f'Father Aldana\'s vestry door at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f})')


# ---------------------------------------------------------------------------
# After Main 4: the Unpaid walk boot hill and the north road
# ---------------------------------------------------------------------------

def roaming(build, cls, unpaid, spawner_id, x, y, z):
    """A roaming stretch's spawner: five Unpaid of the area's ranks and a Restless leader, on from Main 4's finish, standing about."""
    made = build.place(cls, (x, y, z + 50.0), 0.0, label=f'Encounter_{spawner_id}', folder='Gameplay')
    made.set_editor_property('spawner_id', unreal.Name(spawner_id))
    made.set_editor_property('groups', [roaming_group(unpaid, ROAMING_COUNT), leader_group(unpaid)])
    made.set_editor_property('creature_tags', [unreal.Name(f'Unpaid_{spawner_id}')])
    made.set_editor_property('active_when', story.condition(after=[MAIN4]))
    # They walk their stretch until someone comes near, rather than come at the player as they appear.
    made.set_editor_property('hunt_on_spawn', False)
    return made


def place_roaming(build):
    """Boot hill's Unpaid (the zone its ground) and the north road's, between the hanging tree and the road."""
    unpaid = story.actor_class('UnpaidCreature')
    cls = story.actor_class('EncounterSpawner')
    if unpaid is None or cls is None:
        build.warn('no EncounterSpawner or UnpaidCreature class (build the game module first): no Unpaid on boot hill or '
                   'the north road')
        return
    hill = story.layout_entry(build, 'zones', 'bootHill')
    if hill is not None:
        hx, hy = story.centroid(hill['polygon'])
        hz = story.ground_at(hx, hy)
        spawner = roaming(build, cls, unpaid, BOOT_HILL, hx, hy, hz)
        spawner.set_editor_property('spawn_radius', HILL_SPREAD)
        spawner.set_editor_property('ground_corners', [unreal.Vector(x, y, hz) for x, y in hill['polygon']])
        build.log(f'boot hill\'s Unpaid ({BOOT_HILL}: {ROAMING_COUNT}) at ({hx:.0f}, {hy:.0f}), after {MAIN4}')
    tree = story.layout_entry(build, 'obstacles', 'hangingTree')
    road = story.layout_entry(build, 'roads', 'northRoad')
    if tree is not None and road is not None and len(road['path']) >= 2:
        tx, ty = story.centroid(tree['polygon'])
        rx, ry = nearest_on_path(road['path'], (tx, ty))
        # Between the tree and the road where it passes nearest, so the stretch holds both.
        sx, sy = (tx + rx) * 0.5, (ty + ry) * 0.5
        spawner = roaming(build, cls, unpaid, NORTH_ROAD, sx, sy, story.ground_at(sx, sy))
        spawner.set_editor_property('give_up_radius', ROAD_STRETCH)
        spawner.set_editor_property('spawn_radius', ROAD_SPREAD)
        build.log(f'the north road\'s Unpaid ({NORTH_ROAD}: {ROAMING_COUNT}) by the hanging tree at ({sx:.0f}, {sy:.0f}), '
                  f'after {MAIN4}')


# ---------------------------------------------------------------------------
# Hob's perches, and everything
# ---------------------------------------------------------------------------

def hob_spots(chapel):
    """Hob's chapel perches, (x, y, z, yaw) each or None without its socket: on the door hood's ridge, facing out of the
    front as the socket does; on the vestry lantern's bracket, facing out of the vestry door as Aldana's socket does."""
    spots = {'hood': None, 'lantern': None}
    if chapel is None:
        return spots
    hood = story.socket(chapel, 'Perch_Hood')
    lantern = story.socket(chapel, 'Perch_Lantern')
    door = story.socket(chapel, 'Speaker')
    if hood is not None:
        at = hood.translation
        spots['hood'] = (at.x, at.y, at.z, hood.rotation.rotator().yaw)
    if lantern is not None:
        at = lantern.translation
        spots['lantern'] = (at.x, at.y, at.z, (door if door is not None else lantern).rotation.rotator().yaw)
    return spots


def place(build):
    """Everything above, in the Gameplay folder. Returns Hob's chapel perches for build_area_story's place_hob."""
    chapel = story.placed(build, 'Chapel')
    if chapel is not None:
        place_chapel_marker(build, chapel)
        place_yard(build, chapel)
        place_bell(build, chapel)
        place_reliquary(build, chapel)
        place_aldana(build, chapel)
    else:
        build.warn('no chapel: Main 4 has nowhere to happen (import Chapel.py and build the whole level)')
    place_roaming(build)
    return hob_spots(chapel)
