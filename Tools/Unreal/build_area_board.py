"""Skyreach's notice board as a mission board (World/NoticeBoard: ANoticeBoard) and the lookout's marker, for
build_area.py's gameplay pass (Docs/Polish/TutorialRework.md). Only the tutorial island gets them (the layout whose
gameplay.director is TutorialDirector): Ransom's Rest's notice board is the story's, not a mission board.

- The board stands where the town's notice board does: the NoticeBoard piece of the layout's town (level.town, town.json
  beside layout.json: Crossroads Town's square_noticeboard), at its pivot and yaw on the ground. The model is the
  dressing's (build_area_dressing.py places it); the actor adds only the board's use: Interact reads it, its screen lists
  the postings, and they're turned in there (its tag Speaker_NoticeBoard). Labeled NoticeBoard_Square in the Gameplay
  folder.
- The lookout's marker: a target point tagged Place_Lookout over the lookout's placement (layout_computed.json
  placements.lookout), which the "Up to the Lookout" posting's Reach objective finds by its tag.

Without the game module's class yet the board is left out with a warning (the marker still goes in). Run with the rest
of the gameplay pass: build_area.py TutorialIsland gameplay.
"""
import json
import os

import unreal

CLASSES = '/Script/AI_Looter_Shooter.'
DIRECTOR = 'TutorialDirector'
BOARD_MESH = 'NoticeBoard'
BOARD_LABEL = 'NoticeBoard_Square'
LOOKOUT_TAG = 'Place_Lookout'
# The marker floats this far over the lookout's foot (cm): the tower's middle, so the minimap's arrow points at it.
LOOKOUT_RISE = 200.0


def ground(build, x, y):
    """The walkable terrain under (x, y), or None: its tiles alone (build_area.terrain_hit), never the dressing's own
    board standing there, a fence rail or a porch (a visibility trace stopped at the board's instance and found nothing)."""
    import build_area
    hit = build_area.terrain_hit(build_area.terrain_tiles(build.tag), unreal.Vector(x, y, 30000.0),
                                 unreal.Vector(x, y, -30000.0))
    return None if hit is None else hit.z


def board_spot(build):
    """The town's notice board piece as (x, y, yaw), or None: the first piece whose mesh is NoticeBoard in town.json."""
    town_name = build.source.get('level', {}).get('town')
    if not town_name:
        return None
    path = os.path.join(os.path.dirname(build.computed_path), town_name)
    if not os.path.exists(path):
        build.warn(f'no {path}: the notice board is left out')
        return None
    with open(path) as f:
        town = json.load(f)
    for piece in town.get('pieces', []):
        if piece.get('mesh') == BOARD_MESH:
            x, y = piece['at'][0], piece['at'][1]
            return x, y, piece.get('yaw', 0.0)
    return None


def place_board(build):
    spot = board_spot(build)
    if spot is None:
        build.warn('the town has no NoticeBoard piece: the notice board is left out')
        return
    cls = unreal.load_class(None, CLASSES + 'NoticeBoard')
    if cls is None:
        build.warn('no NoticeBoard class (build the game module first): the notice board is left out')
        return
    x, y, yaw = spot
    z = ground(build, x, y)
    if z is None:
        build.warn(f'no ground under the notice board at ({x}, {y})')
        return
    # Its +X looks the way the board's face does (the piece's yaw), so the point read from sits in front of the face.
    build.place(cls, (x, y, z), yaw, label=BOARD_LABEL, folder='Gameplay')
    build.log(f'notice board at ({x:.0f}, {y:.0f}, {z:.0f}), facing {yaw}')


def place_lookout_marker(build):
    lookout = build.layout.get('placements', {}).get('lookout')
    if not lookout:
        build.warn('no lookout placement: the Up to the Lookout posting has nowhere to point')
        return
    x, y, z = lookout['location']
    build.place(unreal.TargetPoint, (x, y, z + LOOKOUT_RISE), 0.0, label=LOOKOUT_TAG, folder='Gameplay', tags=(LOOKOUT_TAG,))
    build.log(f'lookout marker {LOOKOUT_TAG} at ({x:.0f}, {y:.0f})')


def place(build):
    if build.settings.get('director') != DIRECTOR:
        return
    place_board(build)
    place_lookout_marker(build)
