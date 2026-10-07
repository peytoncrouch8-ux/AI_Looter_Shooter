"""Main 6, "The Gravewind" (Docs/Areas/RansomsRest.md: Main 6, "The boss: Abel Ransom, the Keeper"), for build_area_story.py:
the burial boards deck on Gravewind Point dressed and the story's pieces on it, placed from the deck build_area.py placed
(SM_BurialDeck, Art/Models/Props/BurialDeck.py: its SOCKET_Bier_1..8 and SOCKET_LanternPost_1..3, its front the open end
over the canyon), from the layout's features (the Keeper's Gate's rocks gateSouth and gateNorth), placements (graveKeeper)
and boundary (its open edges along the Rim), so they follow the level whenever it's rebuilt. build_area_story's place()
calls place() here (reloaded each run, as the editor keeps modules between runs) and gets Hob's Main 6 perches back for
his. It uses build_area_story's helpers. Everything goes in the area's Gameplay folder; a piece whose model or class isn't
there yet is left out with a warning.

- The eight biers (SM_Bier, tagged Obstacle) on the deck's Bier sockets, turned as each socket is: the fight's cover.
- The three keeper's lantern posts (AKeeperLanternPost with SM_KeeperLanternPost, tagged LanternPost_Deck and Obstacle) on
  the LanternPost sockets. The third, by the entrance, is the keeper's post (bKeepersPost, also tagged LanternPost_Keeper):
  the Keeper's Lantern is hung there in Main 6's second step, hangs there from its third, and is lit and leaning north-east
  after it.
- Abel (AAbelKeeper, tagged Boss_Abel) in the aisle between the biers' two rows, facing the deck's open front (the sunset),
  in the world during Main 6: his three posts, his board, where he hangs in the fog (FOG_OUT past the deck's open front,
  FOG_UP over his middle), how far the open end is from him, and his boss's fog wall.
- Pa's board (AAbelOnBoard, tagged Speaker_Abel) on the SOCKET_Sit of the bier ABEL_BIER, facing the sunset, after Main 6,
  his words by the story (create_story_lines.py's DA_Lines_AbelOnBoard, and DA_Lines_AbelAfterMain7 after Main 7).
- The fog wall (ABossSeal, a gate) across the Keeper's Gate, between the gate's rocks (inset into each by GATE_INSET of its
  radius, so nobody slips round its ends), the deck ahead of it.
- Gravewind Point's place (a target point tagged Place_GravewindPoint) at the Keeper's Gate: "Carry the lantern to
  Gravewind Point" ends within 10 m of it (both roads end at the keeper's grave; the cairns lead on to the gate).
- The keeper's grave's respawn grave (ARespawnMarker KeepersGrave, open after Main 5, so a death in Abel's fight wakes Ellis
  there) at the foot of its mound (the placement graveKeeper), facing the gate.
- The story's dusk (AStoryLighting): Main 6 (and Main 7, which goes on in the evening) in the Dusk lighting state; once
  lit, the dusk lasts until the level loads again.
- The Gravewind's wisps and the canyon's fog (ADuskScenery, one per mesh, tagged Gravewind; the art session's
  SM_GravewindWisp_A-D and SM_CanyonFog_A-C with their own materials), seen only at dusk: five wisps along the deck's open
  front, about 2.8 per 10 m along Gravewind Point's open lips, about 1 per 10 m along the rest of the open Rim (a Curl,
  mirrored, at each corner), WISP_MOST in all; three fog banks off the point (FOGS).
Abel's adds need no spawner: his boss raises them round the deck's middle (AbelRules::RisingUnpaid).
Abel's ghost lantern lights what's round him and never him: it shines on a lighting channel of its own
(AbelRules::GhostLightChannel), so the deck and the biers take that channel as well as 0 here (the posts and his adds take
it in the C++).
Hob's perches (returned to build_area_story.place_hob): on the keeper's grave's board for the way there, on the Keeper's
Gate's north rock for the lantern and the fight, on the bier beside Pa's for the scene and after Main 6.
"""
import math
import random

import unreal

import build_area_story as story

# The story's ids and tags, as the C++ (AAbelKeeper, AKeeperLanternPost, AAbelOnBoard) and the mission asset
# (Tools/Unreal/create_mission_assets.py, DA_Mission_Main6) name them.
MAIN5 = 'Main5'
MAIN6 = 'Main6'
MAIN7 = 'Main7'
BOSS_TAG = 'Boss_Abel'
DECK_POST_TAG = 'LanternPost_Deck'
KEEPERS_POST_TAG = 'LanternPost_Keeper'
ABEL_TAG = 'Speaker_Abel'
PLACE_POINT = 'Place_GravewindPoint'
KEEPERS_GRAVE = 'KeepersGrave'
DUSK = 'Dusk'
GRAVEWIND_TAG = 'Gravewind'
# Main 6's steps, from 0: to the point, the lantern hung, Abel, the scene.
HANG_STEP = 1
FIGHT_STEP = 2

PROPS = '/Game/Art/Props/'
BIER_MESH = PROPS + 'SM_Bier'
POST_MESH = PROPS + 'SM_KeeperLanternPost'

# The deck (BurialDeck.py): 25 m long, its front (the open end over the canyon) 12.5 m out from its middle along the model's
# front (Unreal +X); its walking surface 0.4 m over its pivot. Eight biers, three posts; the keeper's post is the third.
DECK_HALF_LENGTH = 1250.0
DECK_SURFACE = 40.0
BIERS = 8
POSTS = 3
KEEPERS_POST = 3
# Pa's own bier: the front row's (the one nearest the sunset), right of the aisle. Hob perches on the one beside it.
ABEL_BIER = 3
HOB_BIER = 2
# The aisle's middle between the rows, where Abel stands: between these biers' sockets.
AISLE_BIERS = (2, 3, 6, 7)
# His middle over the boards at his size (the Unpaid's 90 cm capsule half height at 1.3), and a little more to settle.
ABEL_UP = 120.0
# Where he hangs in the fog: this far out past the deck's open front, this far up over his middle on the boards (cm).
FOG_OUT = 900.0
FOG_UP = 150.0
# Hob on a bier: over its head end (away from Pa's seat at its foot), on the board (cm; Bier: 2.2 m long, its top 0.9 m).
BIER_HEAD = 80.0
BIER_TOP = 92.0

# Abel's ghost lantern's lighting channel (AbelRules::GhostLightChannel): what should catch its light takes it too.
GHOST_LIGHT_CHANNEL = 'channel2'
# The fog wall's ends go this share of each rock's radius into it, so the wall meets the stone with no gap at its ends.
GATE_INSET = 0.6
# The Keeper's Grave's respawn: this far out of the mound's foot (cm), clear of its kerb.
GRAVE_STAND_OUT = 150.0

# --- The Gravewind's wisps and the canyon's fog (the art session's; plan metres turned into the level's cm) ---
WISP_MESHES = {key: PROPS + f'SM_GravewindWisp_{key}' for key in 'ABCD'}
FOG_MESHES = {key: PROPS + f'SM_CanyonFog_{key}' for key in 'ABC'}
# Five along the deck's open front, one every 3.6 m, turned west within 12 degrees, scaled 0.85-1.15.
DECK_WISPS = 5
DECK_WISP_STEP = 360.0
DECK_WISP_TURN = 12.0
WISP_SCALE = (0.85, 1.15)
# Along the open lips: Gravewind Point's (west of the Keeper's Gate) about 2.8 per 10 m, the rest of the Rim about 1 per 10 m;
# the Rim is the open edges west of RIM_WEST (cm, layout Y: Mill Falls' open edge is far east); at most WISP_MOST in all.
POINT_WISPS_PER_CM = 2.8 / 1000.0
RIM_WISPS_PER_CM = 1.0 / 1000.0
RIM_WEST = -8000.0
WISP_MOST = 35
# Which wisps the lips get: the Streamer (C) and the Curl (D) carry out nearly level; the Spill (A) pours straight down a
# face like a fall, so it's kept to the deck's west end. A lip wisp turns west, this share of the way out over the drop.
POINT_WISP_KINDS = 'CCCDD'
RIM_WISP_KINDS = 'BCCD'
LIP_OUT_SHARE = 0.35
# A lip turning more than this (degrees) at a corner gets a Curl there.
CORNER_TURN = 30.0
# The boundary stands a step past the walkable lip: a wisp leaves the lip this far in from it, at the ground found there.
LIP_IN = 60.0
# The fog banks: (mesh key, plan x, plan y in metres, base height under the boards in metres, compass bearing it faces).
FOGS = (
    ('A', -176.1, 12.0, -12.0, 270.0),
    ('B', -194.8, 18.0, -7.0, 270.0),
    # Off the point's south flank toward the south-south-west (checked against the flank below: a warning if it stands in it).
    ('C', -147.0, -14.5, -13.0, 202.5),
)
SEED = 6061


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def plan_to_level(x, y):
    """Plan metres (x east, y north) to the level's cm (X north, Y east)."""
    return y * 100.0, x * 100.0


def flat(vector):
    length = math.hypot(vector[0], vector[1])
    return (vector[0] / length, vector[1] / length) if length > 1e-6 else (1.0, 0.0)


def yaw_of(direction):
    return math.degrees(math.atan2(direction[1], direction[0]))


def place_rotated(build, what, transform, label, tags=()):
    """A model or a class on a socket's transform: its place, then its whole turn (not only its yaw)."""
    at = transform.translation
    actor = build.place(what, (at.x, at.y, at.z), transform.rotation.rotator().yaw, label=label, folder='Gameplay', tags=tags)
    actor.set_actor_rotation(transform.rotation.rotator(), False)
    return actor


def let_ghost_light_reach(actor):
    """Every part of a placed actor takes Abel's ghost light's channel as well as its own (0: the sun and sky), so his
    lantern lights it; his own body never takes it (so close to his chest it blew his coat out white)."""
    if actor is None:
        return
    for part in actor.get_components_by_class(unreal.PrimitiveComponent):
        channels = part.get_editor_property('lighting_channels')
        channels.set_editor_property(GHOST_LIGHT_CHANNEL, True)
        part.set_editor_property('lighting_channels', channels)


def mesh(build, path, what):
    if not unreal.EditorAssetLibrary.does_asset_exist(path):
        build.warn(f'no {path} yet ({what}): left out')
        return None
    return unreal.load_asset(path)


# ---------------------------------------------------------------------------
# The deck: biers, lantern posts, Abel, his board
# ---------------------------------------------------------------------------

def place_biers(build, deck):
    """The eight biers on their sockets; returns them by number (1-8), None where a socket is missing."""
    bier_mesh = mesh(build, BIER_MESH, 'BurialDeck.py\'s Bier')
    biers = {}
    for k in range(1, BIERS + 1):
        spot = story.socket(deck, f'Bier_{k}')
        if spot is None:
            build.warn(f'the deck has no Bier_{k} socket: that bier is left out')
            continue
        biers[k] = place_rotated(build, bier_mesh, spot, f'Bier_{k}', tags=('Obstacle',)) if bier_mesh else None
        let_ghost_light_reach(biers[k])
    build.log(f'{len([b for b in biers.values() if b])} biers on the deck')
    return biers


def place_posts(build, deck):
    """The three keeper's lantern posts on their sockets, the third the keeper's post; returns them in order."""
    cls = story.actor_class('KeeperLanternPost')
    if cls is None:
        build.warn('no KeeperLanternPost class (build the game module first): no lantern posts')
        return []
    post_mesh = mesh(build, POST_MESH, 'BurialDeck.py\'s KeeperLanternPost')
    posts = []
    for k in range(1, POSTS + 1):
        spot = story.socket(deck, f'LanternPost_{k}')
        if spot is None:
            build.warn(f'the deck has no LanternPost_{k} socket: that post is left out')
            continue
        keepers = k == KEEPERS_POST
        tags = (DECK_POST_TAG, 'Obstacle') + ((KEEPERS_POST_TAG,) if keepers else ())
        post = place_rotated(build, cls, spot, 'KeeperLanternPost_Keeper' if keepers else f'KeeperLanternPost_{k}', tags=tags)
        if post_mesh:
            post.get_editor_property('post').set_static_mesh(post_mesh)
        post.set_editor_property('keepers_post', keepers)
        if keepers:
            # Hung in Main 6's second step (from 0: 1); there from its third and after it; lit and leaning after it.
            post.set_editor_property('hang_when', story.condition(during=MAIN6, from_step=HANG_STEP, before_step=FIGHT_STEP))
            post.set_editor_property('hung_when', [story.condition(during=MAIN6, from_step=FIGHT_STEP), story.condition(after=[MAIN6])])
            post.set_editor_property('lit_when', story.condition(after=[MAIN6]))
        posts.append(post)
    build.log(f'{len(posts)} keeper\'s lantern posts on the deck, the keeper\'s post by the entrance')
    return posts


def place_board(build, biers):
    """Pa's place on his bier's Sit socket, facing the sunset, after Main 6."""
    cls = story.actor_class('AbelOnBoard')
    bier = biers.get(ABEL_BIER)
    seat = story.socket(bier, 'Sit') if bier else None
    if cls is None or seat is None:
        build.warn('no AbelOnBoard class (build the game module first): Pa has no board' if cls is None
                   else f'no Bier_{ABEL_BIER} with a Sit socket: Pa has no board')
        return None
    board = place_rotated(build, cls, seat, 'Abel_OnBoard', tags=(ABEL_TAG,))
    board.set_editor_property('shown_when', story.condition(after=[MAIN6]))
    # The first that holds is said: after Main 7 (the lantern leaning toward Ned), after Main 6. Each ember paid adds one.
    board.get_editor_property('speaker_point').set_editor_property('topics', story.topics(
        build,
        (story.condition(after=[MAIN7]), 'DA_Lines_AbelAfterMain7'),
        (story.condition(after=[MAIN6]), 'DA_Lines_AbelOnBoard'),
    ))
    at = seat.translation
    build.log(f'Pa\'s board on Bier_{ABEL_BIER} at ({at.x:.0f}, {at.y:.0f}, {at.z:.0f}), after {MAIN6}')
    return board


def place_abel(build, deck, biers, posts, board, seal):
    """Abel in the aisle between the rows, facing the sunset, with what his fight needs."""
    cls = story.actor_class('AbelKeeper')
    sockets = [story.socket(deck, f'Bier_{k}') for k in AISLE_BIERS]
    sockets = [spot for spot in sockets if spot is not None]
    if cls is None or not sockets:
        build.warn('no AbelKeeper class (build the game module first): no Abel' if cls is None else 'no Bier sockets: no aisle for Abel')
        return None
    x = sum(s.translation.x for s in sockets) / len(sockets)
    y = sum(s.translation.y for s in sockets) / len(sockets)
    deck_at = deck.get_actor_location()
    boards_z = deck_at.z + DECK_SURFACE
    forward = flat(deck.get_actor_forward_vector().to_tuple())
    abel = build.place(cls, (x, y, boards_z + ABEL_UP), yaw_of(forward), label='Abel', folder='Gameplay', tags=(BOSS_TAG,))
    abel.set_editor_property('lantern_posts', posts)
    if board is not None:
        abel.set_editor_property('on_board', board)
    # The open front: DECK_HALF_LENGTH out from the deck's middle; he hangs FOG_OUT past it, FOG_UP over his middle.
    front = (deck_at.x + forward[0] * DECK_HALF_LENGTH, deck_at.y + forward[1] * DECK_HALF_LENGTH)
    open_end = (front[0] - x) * forward[0] + (front[1] - y) * forward[1]
    abel.set_editor_property('open_end_distance', open_end)
    fog = unreal.Vector(front[0] + forward[0] * FOG_OUT, front[1] + forward[1] * FOG_OUT, boards_z + 117.0 + FOG_UP)
    abel.set_editor_property('fog_spot', fog)
    if seal is not None:
        abel.get_editor_property('boss').set_editor_property('seal', seal)
    build.log(f'Abel on the deck at ({x:.0f}, {y:.0f}), facing yaw {yaw_of(forward):.0f}; the open end {open_end:.0f} cm on, '
              f'the fog at ({fog.x:.0f}, {fog.y:.0f}, {fog.z:.0f}); during {MAIN6}')
    return abel


# ---------------------------------------------------------------------------
# The Keeper's Gate, the point's place, the keeper's grave, the dusk
# ---------------------------------------------------------------------------

def place_gate(build, deck):
    """The fog wall across the Keeper's Gate and Gravewind Point's place there. Returns (the wall, the gate's middle)."""
    rocks = [story.layout_entry(build, 'features', key) for key in ('gateSouth', 'gateNorth')]
    if any(rock is None for rock in rocks):
        return None, None
    (sx, sy), (nx, ny) = rocks[0]['center'], rocks[1]['center']
    across = flat((nx - sx, ny - sy))
    start = (sx + across[0] * rocks[0].get('radius', 0.0) * GATE_INSET, sy + across[1] * rocks[0].get('radius', 0.0) * GATE_INSET)
    end = (nx - across[0] * rocks[1].get('radius', 0.0) * GATE_INSET, ny - across[1] * rocks[1].get('radius', 0.0) * GATE_INSET)
    middle = ((start[0] + end[0]) * 0.5, (start[1] + end[1]) * 0.5)
    story.marker(build, middle[0], middle[1], PLACE_POINT)
    cls = story.actor_class('BossSeal')
    if cls is None:
        build.warn('no BossSeal class (build the game module first): no fog wall at the Keeper\'s Gate')
        return None, middle
    # Its front (the arena's side) toward the deck.
    deck_at = deck.get_actor_location()
    ahead = (-across[1], across[0])
    if ahead[0] * (deck_at.x - middle[0]) + ahead[1] * (deck_at.y - middle[1]) < 0.0:
        ahead = (-ahead[0], -ahead[1])
    yaw = yaw_of(ahead)
    gz = story.ground_at(middle[0], middle[1])
    # On the ground of the gap (an end inside a rock would find the rock's top, and the wall would stand off the ground).
    seal = build.place(cls, (start[0], start[1], gz), yaw, label='Seal_KeepersGate', folder='Gameplay')
    seal.set_editor_property('shape', unreal.BossSealShape.GATE)
    # The far end in the wall's own frame: X ahead (toward the deck), Y to its right.
    right = (math.cos(math.radians(yaw + 90.0)), math.sin(math.radians(yaw + 90.0)))
    dx, dy = end[0] - start[0], end[1] - start[1]
    seal.set_editor_property('gate_end', unreal.Vector(dx * ahead[0] + dy * ahead[1], dx * right[0] + dy * right[1], 0.0))
    build.log(f'the fog wall across the Keeper\'s Gate, {math.hypot(dx, dy) / 100.0:.1f} m, facing the deck; Gravewind Point\'s '
              f'place at ({middle[0]:.0f}, {middle[1]:.0f}), tagged {PLACE_POINT}')
    return seal, middle


def place_keepers_grave(build, gate):
    """The keeper's grave's respawn grave, open after Main 5, facing the gate. Returns the grave."""
    grave = story.placed(build, 'Grave_Keeper')
    cls = story.actor_class('RespawnMarker')
    if grave is None or cls is None:
        return grave
    rise = story.socket(grave, 'Respawn')
    heap = rise.translation if rise else grave.get_actor_location()
    foot = grave.get_actor_forward_vector()
    x, y = heap.x + foot.x * GRAVE_STAND_OUT, heap.y + foot.y * GRAVE_STAND_OUT
    yaw = story.facing((x, y), gate) if gate else grave.get_actor_rotation().yaw
    marker = build.place(cls, (x, y, story.ground(x, y, heap.z, ignore=[grave])), yaw, label='Respawn_KeepersGrave', folder='Gameplay')
    marker.set_editor_property('marker_id', unreal.Name(KEEPERS_GRAVE))
    marker.set_editor_property('active_after_mission', unreal.Name(MAIN5))
    build.log(f'the keeper\'s grave\'s respawn at ({x:.0f}, {y:.0f}), open after {MAIN5}')
    return grave


def place_dusk(build, at):
    """Main 6 and Main 7 at dusk (the lighting states' Dusk)."""
    cls = story.actor_class('StoryLighting')
    if cls is None:
        build.warn('no StoryLighting class (build the game module first): Main 6 starts in the afternoon')
        return
    light = build.place(cls, at, 0.0, label='StoryLighting', folder='Gameplay')
    rules = []
    for mission in (MAIN6, MAIN7):
        rule = unreal.StoryLightingRule()
        rule.set_editor_property('when', story.condition(during=mission))
        rule.set_editor_property('state', unreal.Name(DUSK))
        rules.append(rule)
    light.set_editor_property('rules', rules)
    build.log(f'the story\'s light: {MAIN6} and {MAIN7} at {DUSK}')


# ---------------------------------------------------------------------------
# The Gravewind's wisps and the canyon's fog, at dusk
# ---------------------------------------------------------------------------

def rim_edges(build):
    """The open edges along the Rim: (start (x, y, z), end (x, y, z), inward (x, y)), from layout_computed.json's boundary."""
    boundary = build.layout.get('boundary', {})
    corners, opens = boundary.get('corners', []), boundary.get('openEdges', [])
    if len(corners) < 3:
        return []
    # Inward is to one side of every edge: the side the polygon's area lies on (its winding).
    area = sum(a[0] * b[1] - b[0] * a[1] for a, b in zip(corners, corners[1:] + corners[:1]))
    turn = 1.0 if area > 0 else -1.0
    edges = []
    for i, is_open in enumerate(opens):
        a, b = corners[i], corners[(i + 1) % len(corners)]
        if not is_open or max(a[1], b[1]) > RIM_WEST:
            continue
        d = flat((b[0] - a[0], b[1] - a[1]))
        edges.append((a, b, (-d[1] * turn, d[0] * turn)))
    return edges


def lip_spot(a, b, inward, t):
    """A wisp's pivot on the lip: along the edge at t, LIP_IN in from the boundary, on the ground found a little further in
    (over the drop there's nothing to find)."""
    x = a[0] + (b[0] - a[0]) * t + inward[0] * LIP_IN
    y = a[1] + (b[1] - a[1]) * t + inward[1] * LIP_IN
    guess = a[2] + (b[2] - a[2]) * t
    z = story.ground(x + inward[0] * 90.0, y + inward[1] * 90.0, guess)
    return x, y, z


def wisp_transform(rng, x, y, z, yaw, key):
    scale = rng.uniform(*WISP_SCALE)
    # The Curl curls toward -Y: mirrored here and there so they curl both ways.
    side = -1.0 if key == 'D' and rng.random() < 0.5 else 1.0
    return unreal.Transform(unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=0.0, yaw=yaw), unreal.Vector(scale, scale * side, scale))


def gravewind_wisps(build, deck, gate):
    """Every wisp's (mesh key, transform): along the deck's front, the point's lips and the rest of the Rim."""
    rng = random.Random(SEED)
    placed = []
    deck_at = deck.get_actor_location()
    forward = flat(deck.get_actor_forward_vector().to_tuple())
    right = (-forward[1], forward[0])
    boards = deck_at.z + DECK_SURFACE
    front = (deck_at.x + forward[0] * DECK_HALF_LENGTH, deck_at.y + forward[1] * DECK_HALF_LENGTH)
    for k in range(DECK_WISPS):
        along = (k - (DECK_WISPS - 1) * 0.5) * DECK_WISP_STEP
        key = 'ABC'[k % 3] if k != DECK_WISPS // 2 else 'C'
        # Out over the drop, west, within a few degrees.
        yaw = yaw_of(forward) + rng.uniform(-DECK_WISP_TURN, DECK_WISP_TURN)
        placed.append((key, wisp_transform(rng, front[0] + right[0] * along, front[1] + right[1] * along, boards, yaw, key)))
    # The lips: Gravewind Point's (west of the gate) densely, the rest of the Rim lightly; the deck's own front is above.
    gate_y = gate[1] if gate else deck_at.y + 2000.0
    point, rim = [], []
    for a, b, inward in rim_edges(build):
        middle_y = (a[1] + b[1]) * 0.5
        on_deck_front = abs((a[0] - front[0]) * forward[0] + (a[1] - front[1]) * forward[1]) < 300.0 and \
            abs((b[0] - front[0]) * forward[0] + (b[1] - front[1]) * forward[1]) < 300.0
        if on_deck_front:
            continue
        (point if middle_y < gate_y else rim).append((a, b, inward))
    budget = WISP_MOST - len(placed)
    point_count = sum(math.hypot(b[0] - a[0], b[1] - a[1]) * POINT_WISPS_PER_CM for a, b, _ in point)
    rim_count = sum(math.hypot(b[0] - a[0], b[1] - a[1]) * RIM_WISPS_PER_CM for a, b, _ in rim)
    # Over budget, the rest of the Rim thins first.
    rim_share = 1.0 if point_count + rim_count <= budget else max(0.0, (budget - point_count) / max(rim_count, 1e-6))
    previous = None
    for edges, density, kinds in ((point, POINT_WISPS_PER_CM, POINT_WISP_KINDS), (rim, RIM_WISPS_PER_CM * rim_share, RIM_WISP_KINDS)):
        for a, b, inward in edges:
            length = math.hypot(b[0] - a[0], b[1] - a[1])
            count = int(round(length * density))
            outward = (-inward[0], -inward[1])
            # West with the wind, more along the lip than out over the drop: the Gravewind pours off the Rim westward.
            between = flat((outward[0] * LIP_OUT_SHARE, outward[1] * LIP_OUT_SHARE - 1.0))
            yaw = yaw_of(between)
            # A Curl where the lip turns sharply from the edge before.
            if previous is not None and previous[1] == a and len(placed) < WISP_MOST:
                d0 = flat((previous[1][0] - previous[0][0], previous[1][1] - previous[0][1]))
                d1 = flat((b[0] - a[0], b[1] - a[1]))
                if math.degrees(math.acos(max(-1.0, min(1.0, d0[0] * d1[0] + d0[1] * d1[1])))) > CORNER_TURN:
                    x, y, z = lip_spot(a, b, inward, 0.0)
                    placed.append(('D', wisp_transform(rng, x, y, z, yaw, 'D')))
            previous = (a, b)
            for k in range(count):
                if len(placed) >= WISP_MOST:
                    break
                x, y, z = lip_spot(a, b, inward, (k + 0.5) / count)
                key = rng.choice(kinds)
                placed.append((key, wisp_transform(rng, x, y, z, yaw + rng.uniform(-8.0, 8.0), key)))
    return placed


def place_gravewind(build, deck, gate):
    """The wisps and the fog banks as ADuskScenery, one per mesh, seen only at dusk."""
    cls = story.actor_class('DuskScenery')
    if cls is None:
        build.warn('no DuskScenery class (build the game module first): no Gravewind wisps or canyon fog')
        return
    by_mesh = {}
    for key, transform in gravewind_wisps(build, deck, gate):
        by_mesh.setdefault(WISP_MESHES[key], []).append(transform)
    boards = deck.get_actor_location().z + DECK_SURFACE
    for key, px, py, base, bearing in FOGS:
        x, y = plan_to_level(px, py)
        z = boards + base * 100.0
        # Over open canyon nothing is under it, which is right: only ground above its base is a fault.
        under = story.ground_at(x, y, z - 10000.0, warn=False)
        if under > z + 100.0:
            build.warn(f'canyon fog {key} at ({px}, {py}) stands {(under - z) / 100.0:.1f} m into the ground under it (the point\'s '
                       f'flank?): move it in FOGS')
        # A compass bearing is the level's yaw (0 north, +X; 90 east, +Y).
        by_mesh.setdefault(FOG_MESHES[key], []).append(
            unreal.Transform(unreal.Vector(x, y, z), unreal.Rotator(roll=0.0, pitch=0.0, yaw=bearing), unreal.Vector(1.0, 1.0, 1.0)))
    wisps = fogs = 0
    for path, transforms in sorted(by_mesh.items()):
        model = mesh(build, path, 'the art session\'s Gravewind and canyon fog')
        if model is None:
            continue
        name = path.rsplit('/', 1)[-1].replace('SM_', '')
        scenery = build.place(cls, (0.0, 0.0, 0.0), 0.0, label=f'Dusk_{name}', folder='Gameplay', tags=(GRAVEWIND_TAG,))
        scenery.set_instances(model, transforms)
        if 'Wisp' in path:
            wisps += len(transforms)
        else:
            fogs += len(transforms)
    build.log(f'the Gravewind at dusk: {wisps} wisps along the deck and the Rim, {fogs} fog banks off the point (tagged {GRAVEWIND_TAG})')


# ---------------------------------------------------------------------------
# Hob's perches, and everything
# ---------------------------------------------------------------------------

def hob_spots(build, grave, gate, biers):
    """Hob's Main 6 perches, (x, y, z, yaw) each or None: the keeper's grave's board, the gate's north rock, the bier beside Pa's."""
    spots = {'keeperGrave': story.hob_on_board(grave) if grave else None, 'gateRock': None, 'deckBier': None}
    north = story.layout_entry(build, 'features', 'gateNorth')
    if north is not None:
        rx, ry = north['center']
        toward = gate if gate else (rx, ry - 1000.0)
        spots['gateRock'] = (rx, ry, story.ground_at(rx, ry), story.facing((rx, ry), toward))
    bier = biers.get(HOB_BIER)
    if bier is not None:
        at = bier.get_actor_location()
        back = bier.get_actor_forward_vector()
        spots['deckBier'] = (at.x - back.x * BIER_HEAD, at.y - back.y * BIER_HEAD, at.z + BIER_TOP, bier.get_actor_rotation().yaw)
    return spots


def place(build):
    """Everything above, in the Gameplay folder. Returns Hob's Main 6 perches for build_area_story's place_hob."""
    deck = story.placed(build, 'BurialDeck')
    if deck is None:
        build.warn('no burial deck: Main 6 has nowhere to happen (import BurialDeck.py and build the whole level)')
        return {'keeperGrave': None, 'gateRock': None, 'deckBier': None}
    # The deck is build_area.py's; its boards catch Abel's lantern (his lighting channel), as the biers on it do.
    let_ghost_light_reach(deck)
    biers = place_biers(build, deck)
    posts = place_posts(build, deck)
    board = place_board(build, biers)
    seal, gate = place_gate(build, deck)
    place_abel(build, deck, biers, posts, board, seal)
    grave = place_keepers_grave(build, gate)
    at = deck.get_actor_location()
    place_dusk(build, (at.x, at.y, at.z + 500.0))
    place_gravewind(build, deck, gate)
    return hob_spots(build, grave, gate, biers)
