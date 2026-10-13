"""Notice boards as mission boards (World/NoticeBoard: ANoticeBoard) and Skyreach's lookout marker, for build_area.py's
gameplay pass (Docs/Polish/TutorialRework.md; 2026-10-13: every mission is picked up from a giver, boards among them).

- Skyreach (the layout whose gameplay.director is TutorialDirector): its board stands where the town's notice board does,
  the NoticeBoard piece of the layout's town (level.town, town.json beside layout.json: Crossroads Town's
  square_noticeboard), at its pivot and yaw on the ground, labeled NoticeBoard_Square. Its postings are the C++ defaults
  (the practice postings), turned in there (its tag Speaker_NoticeBoard). And the lookout's marker: a target point tagged
  Place_Lookout over the lookout's placement (layout_computed.json placements.lookout), which the "Up to the Lookout"
  posting's Reach objective finds by its tag.
- Any layout: the boards listed in gameplay.boards, each a bounty board whose jobs are the missions offered by its tags
  (FMissionOffer::GiverTag), e.g.
      "boards": [{"label": "NoticeBoard_Rangers", "piece": "NoticeBoard", "tags": ["NoticeBoard_Rangers"],
                  "title": "Rim Rangers", "subtitle": "Bounties, posted by the Rangers", "prompt": "Read the bounties"}]
  "piece" stands it where the town's piece with that mesh is (as Skyreach's); else "at": [x, y] and "yaw". "postings"
  (mission ids the board lists whatever their givers, in order) and "main" (the one tracked as they go up) are optional:
  a bounty board lists its offers by its tags, so both default to none, never Skyreach's practice postings.

The model is always the dressing's (build_area_dressing.py places it); the actor adds only the board's use: Interact reads
it and its screen lists the postings. Labeled boards go in the Gameplay folder. Without the game module's class yet the
boards are left out with a warning (the marker still goes in). Run with the rest of the gameplay pass:
build_area.py <Area> gameplay.
"""
import json
import os

import unreal

CLASSES = '/Script/AI_Looter_Shooter.'
DIRECTOR = 'TutorialDirector'
# Skyreach's town board is NoticeBoard_Town (NOTICES on the header); NoticeBoard is Ransom's Rest's (RIM RANGERS).
BOARD_MESHES = ('NoticeBoard', 'NoticeBoard_Town')
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


def town_pieces(build):
    """The pieces of the layout's town (town.json beside layout.json), or [] without one."""
    town_name = build.source.get('level', {}).get('town')
    if not town_name:
        return []
    path = os.path.join(os.path.dirname(build.computed_path), town_name)
    if not os.path.exists(path):
        build.warn(f'no {path}: its notice boards are left out')
        return []
    with open(path) as f:
        return json.load(f).get('pieces', [])


def board_spot(build, meshes=BOARD_MESHES):
    """The first town piece whose mesh is one of meshes, as (x, y, yaw), or None."""
    for piece in town_pieces(build):
        if piece.get('mesh') in meshes:
            x, y = piece['at'][0], piece['at'][1]
            return x, y, piece.get('yaw', 0.0)
    return None


def board_class(build):
    cls = unreal.load_class(None, CLASSES + 'NoticeBoard')
    if cls is None:
        build.warn('no NoticeBoard class (build the game module first): the notice boards are left out')
    return cls


def stand_board(build, cls, spot, label, tags=()):
    """An ANoticeBoard at spot (x, y, yaw) on the ground, labeled and tagged; None when there's no ground there."""
    x, y, yaw = spot
    z = ground(build, x, y)
    if z is None:
        build.warn(f'no ground under the notice board {label} at ({x}, {y})')
        return None
    # Its +X looks the way the board's face does (the piece's yaw), so the point read from sits in front of the face.
    board = build.place(cls, (x, y, z), yaw, label=label, folder='Gameplay', tags=tuple(tags))
    build.log(f'notice board {label} at ({x:.0f}, {y:.0f}, {z:.0f}), facing {yaw}')
    return board


def place_board(build):
    """Skyreach's board in Crossroads Town's square, with the C++'s practice postings."""
    spot = board_spot(build)
    if spot is None:
        build.warn('the town has no NoticeBoard piece: the notice board is left out')
        return
    cls = board_class(build)
    if cls is not None:
        stand_board(build, cls, spot, BOARD_LABEL)


def place_listed_boards(build):
    """The layout's own boards (gameplay.boards): bounty boards whose jobs are the missions their tags offer."""
    listed = build.settings.get('boards', [])
    if not listed:
        return
    cls = board_class(build)
    if cls is None:
        return
    for index, spec in enumerate(listed):
        label = spec.get('label', f'NoticeBoard_{index + 1}')
        if 'piece' in spec:
            spot = board_spot(build, (spec['piece'],))
            if spot is None:
                build.warn(f'{label}: the town has no {spec["piece"]} piece: the board is left out')
                continue
        elif 'at' in spec:
            spot = (spec['at'][0], spec['at'][1], spec.get('yaw', 0.0))
        else:
            build.warn(f'{label}: neither "piece" nor "at" says where it stands: the board is left out')
            continue
        tags = list(spec.get('tags', []))
        if not tags:
            build.warn(f'{label}: no tags, so no mission can name it as its giver')
        board = stand_board(build, cls, spot, label, tags)
        if board is None:
            continue
        # Never Skyreach's practice postings (the C++ defaults): a bounty board lists what its tags offer.
        board.set_editor_property('postings', [unreal.Name(each) for each in spec.get('postings', [])])
        board.set_editor_property('main_posting', unreal.Name(spec.get('main', '')))
        for field in ('title', 'subtitle', 'prompt'):
            if field in spec:
                board.set_editor_property(field, unreal.Text(spec[field]))
        # Skyreach's note (come back with a gun) is no other board's.
        board.set_editor_property('locked_note', unreal.Text(spec.get('lockedNote', '')))
        build.log(f'{label}: tags {", ".join(tags) or "none"}, {len(spec.get("postings", []))} postings listed')


def place_lookout_marker(build):
    lookout = build.layout.get('placements', {}).get('lookout')
    if not lookout:
        build.warn('no lookout placement: the Up to the Lookout posting has nowhere to point')
        return
    x, y, z = lookout['location']
    build.place(unreal.TargetPoint, (x, y, z + LOOKOUT_RISE), 0.0, label=LOOKOUT_TAG, folder='Gameplay', tags=(LOOKOUT_TAG,))
    build.log(f'lookout marker {LOOKOUT_TAG} at ({x:.0f}, {y:.0f})')


def place(build):
    if build.settings.get('director') == DIRECTOR:
        place_board(build)
        place_lookout_marker(build)
    place_listed_boards(build)
