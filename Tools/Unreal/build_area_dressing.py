"""The level's dressing for build_area.py: the layout's obstacles of the kinds fence, wall, ruin, graves, cairns and
props (Art/Levels/<Area>/layout.json obstacles; Docs/Areas/RansomsRest.md, "What breaks up the open ground": nothing
more than about 10 m from a break, and fences, low walls and cairns count) dressed with the art session's kit under
/Game/Art/Props (Art/Models/Props/Fences.py, Ruins.py, Graves.py, FarmProps.py, Containers.py, Boardwalk.py;
Undergrowth.py's log and stump). build_area.py's full build calls place() after its models(), and
"build_area.py <Area> dressing" places only this again. Everything goes in the area's Dressing folder, tagged with the
area's build tag and Dressing (Looter.Perf.HideTag Dressing measures it by difference), so building again replaces only
what this placed.

What each obstacle becomes is in the tables below (LINES, GRAVE_ROWS, CAIRN_LINES, RUINS, GROUPS, SPOTS, SKIP), by
obstacle id, so the art session can retune it without touching the code:
- Fences and walls run along their path or polygon in sections from evenly spaced joints, broken at corners sharper
  than 20 degrees and each stretched to its chord on the ground. Whitlock Fields' (Amos's fence, the barn yard's, the
  field wall) are here too; build_area_whitlock.py stands Amos in his fence's first span past its gate by run_head()
  and span_heights(), from the same plan. A run ends on a post (a wall on its finished end); a polygon's kit faces
  out of it (a fence's outside, a gate's leaves open inward), a path's away from its "inside". The broken, fallen and
  missing sections a note asks for are drawn per section from the obstacle's own seed. Gates are left open where the
  table puts them and wherever a road crosses the line (as much of the line as the road takes up, and a little more),
  so no fence closes a road; the iron and picket fences fill theirs with their gate units, swung open. The sheep fold's
  walls turn on Ruins.py's square corner and take half walls where whole ones won't fit. The Sink's fences carry a
  warning sign each by the ramp head's gap. A fence's run stops a metre short of a cliff piece or a rock it would run
  into (Rocks, from what build_area.py placed), ending on its post, and goes on past it.
- Grave rows get the old headboards and crosses, mixed by a fixed seed, a little out of line and leaning a little, boot
  hill's and the family plot's each over its sunken mound; the family plot's eight old graves and the churchyard's short
  rows on the knoll's south face (seen from boot hill and the town), which have no obstacle, come from EXTRA_GRAVE_ROWS,
  and each fresh mound the placements put down (the respawn graves) gets its fresh headboard (MOUND_BOARDS).
- Cairns stand along their path; the ruins and the keeper's three cairns are layout placements, which build_area.py's
  models() places already (a ruin whose placement is gone is placed here instead, as its own actor).
- Props become small groups laid out in the obstacle's own frame: along its longest side, facing its road or a
  building, and pieces set against a building's wall in its own frame (the backlots' lean-tos). A few pieces stand at
  spots of their own (SPOTS: Whitlock Fields' round bales, two more fallen pines by the Sink and north roads, the
  churchyard's dead trees).
Every piece stands on the terrain's tiles (traced like build_area_whitlock.py's Ground: the tiles alone, never a volume,
a tree or a building). A fence's sections stand plumb and level, stepped down a slope (fence_steps: split into shorter
steps where it's steep, a post at each step its own posts can't cover); a wall's follow the slope along their length
(the roll); both sink where the ground dips under their middle. Props tilt with the ground by at most MAX_TILT degrees
(mounds MOUND_TILT) and sink so no corner hovers; stacks, sheds and trees (UPRIGHT) stand level.

Performance (the doc's "Performance plan": about 700 draws at the heaviest view): every kit piece is instanced, one
AInstancedProps per mesh (World/InstancedProps.h: solid like a placed mesh, shadowed, each instance culled on its own
past the mesh's distance in PIECES; Medium's view distance scale draws them to 60% of it). A ruin is its own actor.

The scatter (build_island_scatter.py) keeps its layers out of the bounds of every actor tagged Obstacle, and an
instanced actor's bounds span the valley: AInstancedProps tags itself Obstacle only in play (for the minimap and the
creatures), and footprints() gives the scatter every piece's own box from the same plan instead.

Run plan_all()/footprints() anywhere (plain Python and the layout's JSON); place() needs the editor.
"""
import math
import random

import unreal

import build_area

CLASSES = '/Script/AI_Looter_Shooter.'
FOLDER = 'Dressing'
TAG = 'Dressing'

# How far (cm) each kind of piece is drawn (AInstancedProps.CullDistance, by instance; Medium draws to 60% of it).
SMALL_CULL = 12000.0   # yard props, graves, cairns: about 70 m on Medium
FENCE_CULL = 16000.0   # fence sections and posts: a line reads further than a crate
WALL_CULL = 25000.0    # stone walls, the big pieces
TREE_CULL = 0.0        # trees: never (a tree's silhouette is what tells a place from afar)

# Every kit piece, by its mesh's name without SM_ (build_area.mesh_index()'s keys): its size as it stands (cm: along
# its length, the actor's Y, which a fence runs along; across, the actor's X, its front; up), where its pivot is along
# its length ('start': a chained piece's first post or joint; 'middle'), and its cull distance.
PIECES = {
    'FenceRail': ((300, 16, 122), 'start', FENCE_CULL),
    'FenceBroken': ((300, 90, 122), 'start', FENCE_CULL),
    'FencePost': ((16, 16, 122), 'middle', FENCE_CULL),
    'Fence_PicketSection': ((200, 10, 105), 'start', FENCE_CULL),
    'Fence_PicketPost': ((10, 10, 108), 'middle', FENCE_CULL),
    'Fence_IronSection': ((200, 8, 120), 'start', FENCE_CULL),
    'Fence_IronPost': ((22, 22, 140), 'middle', FENCE_CULL),
    'Fence_IronGate': ((200, 90, 160), 'start', FENCE_CULL),
    'StoneWall': ((300, 72, 92), 'start', WALL_CULL),
    'StoneWall_Broken': ((300, 210, 92), 'start', WALL_CULL),
    'StoneWall_Fallen': ((300, 280, 55), 'start', WALL_CULL),
    'StoneWallEnd': ((90, 72, 92), 'start', WALL_CULL),
    'StoneWall_Corner': ((150, 72, 92), 'start', WALL_CULL),
    'Grave_Headboard_OldA': ((50, 12, 80), 'middle', SMALL_CULL),
    'Grave_Headboard_OldB': ((55, 12, 75), 'middle', SMALL_CULL),
    'Grave_Headboard_OldC': ((48, 12, 82), 'middle', SMALL_CULL),
    'Grave_Headboard_OldD': ((44, 12, 92), 'middle', SMALL_CULL),
    'Grave_Headboard_FreshA': ((50, 12, 82), 'middle', SMALL_CULL),
    'Grave_Headboard_FreshB': ((42, 12, 76), 'middle', SMALL_CULL),
    'Grave_Cross_A': ((60, 12, 112), 'middle', SMALL_CULL),
    'Grave_Cross_B': ((56, 28, 118), 'middle', SMALL_CULL),
    'Cairn_A': ((70, 70, 80), 'middle', SMALL_CULL),
    'Cairn_B': ((80, 80, 60), 'middle', SMALL_CULL),
    'Cairn_C': ((64, 64, 75), 'middle', SMALL_CULL),
    'HitchRail': ((304, 20, 122), 'middle', SMALL_CULL),
    'WaterTrough': ((186, 86, 54), 'middle', SMALL_CULL),
    'Barrel_A': ((66, 66, 92), 'middle', SMALL_CULL),
    'Barrel_B': ((60, 60, 88), 'middle', SMALL_CULL),
    'Crate_A': ((98, 73, 62), 'middle', SMALL_CULL),
    'Crate_B': ((104, 66, 55), 'middle', SMALL_CULL),
    'Cart': ((140, 300, 110), 'middle', SMALL_CULL),
    'Coffin_Closed': ((200, 66, 44), 'middle', SMALL_CULL),
    'Coffin_Broken': ((194, 62, 42), 'middle', SMALL_CULL),
    'HayBale_Round': ((120, 140, 132), 'middle', SMALL_CULL),
    'HayBale_Square': ((95, 50, 36), 'middle', SMALL_CULL),
    'FirewoodStack': ((180, 55, 112), 'middle', SMALL_CULL),
    'Outhouse': ((134, 134, 230), 'middle', WALL_CULL),
    'Log_A': ((340, 55, 50), 'middle', SMALL_CULL),
    'Stump_A': ((130, 130, 50), 'middle', SMALL_CULL),
    # The art session's second round (Art/Models/Props/Backlots.py, Graves.py, Ruins.py).
    'Woodshed': ((326, 282, 247), 'middle', WALL_CULL),
    'LeanTo': ((340, 203, 286), 'middle', WALL_CULL),
    'LumberStack': ((380, 234, 176), 'middle', WALL_CULL),
    'Dray': ((177, 543, 173), 'middle', WALL_CULL),
    'FallenPine': ((788, 152, 91), 'middle', FENCE_CULL),
    'SinkWarning': ((118, 25, 171), 'middle', FENCE_CULL),
    'Fence_PicketGate_Open': ((200, 100, 108), 'start', FENCE_CULL),
    'Grave_MoundSunken': ((100, 200, 25), 'middle', SMALL_CULL),
    'StoneWall_Half': ((150, 72, 95), 'start', WALL_CULL),
    # The churchyard's dead trees (DeadTree.py, about 6.5 m): their box the trunk's foot, so the scatter's grass grows
    # round it; never culled, like the scatter's trees.
    'DeadTree_A': ((80, 80, 650), 'middle', TREE_CULL),
}
# Where a piece's footprint's middle is, ahead of its pivot along its front (cm), where it isn't the pivot: the
# woodshed's chopping block stands in front of its posts, the lumber stack's planks lean out of its front, the lean-to's
# pivot is a little behind its middle (its back is on the wall).
FOOTPRINT_AHEAD = {'Woodshed': 21.0, 'LumberStack': 28.0, 'LeanTo': 6.0}

# The fence and wall kits: the section and its length (cm), the broken and fallen sections, the piece that closes a run,
# the iron fence's heavy corner post, the gate unit that fills a gate's gap (gate_clear: its way through, from and to
# along it (cm), centred on the gate's spot), the wall's finished end (it reaches end_length past its joint), square
# corner (arm: how far each arm reaches from the corner, cm) and half section (chained in where whole sections would
# be squashed or stretched, on the lines that ask for halves).
# A fence's sections stand plumb on a slope (the art session's churchyard note, 2026-10-07: tilted with the ground the
# iron fence's pickets and posts leaned off plumb down the knoll), level and stepped down it: the kits are rigid
# panels (Graves.py, Fences.py), and an instance can't be sheared to rack its rails, so a section steps instead. Each
# stands with its uphill end sunk at most bury into the slope and its downhill end at most hover over it (the post and
# pickets reach a little under the pivot, so a few cm doesn't show); a section the slope drops more than bury + hover
# is split into shorter level steps, down to shortest of its length (squashed, the panel's own post at each step); and
# where two sections step more than hide, which the section's own post can't cover (the rails' ends would show past
# it), step_post stands on the ground at the joint, plumb: the iron fence's heavy post at every step; the wooden kits'
# (step_down) only where the section after the joint stands lower, its own first post sunk there (stepping up, that
# post stands on the ground and takes the lower rails' ends, and a second post beside it would double it).
# clear_rock: a run stops this far (cm) short of a cliff piece or a rock it would run into (place() finds them:
# Rocks). Walls keep following the ground.
KITS = {
    'rail': {'section': 'FenceRail', 'length': 300.0, 'broken': 'FenceBroken', 'end': 'FencePost',
             'bury': 35.0, 'hover': 20.0, 'shortest': 0.34, 'step_post': 'FencePost', 'hide': 20.0, 'step_down': True,
             'clear_rock': 100.0},
    'picket': {'section': 'Fence_PicketSection', 'length': 200.0, 'end': 'Fence_PicketPost',
               'gate': 'Fence_PicketGate_Open', 'gate_clear': (4.5, 100.5),
               'bury': 30.0, 'hover': 15.0, 'shortest': 0.5, 'step_post': 'Fence_PicketPost', 'hide': 15.0,
               'step_down': True, 'clear_rock': 100.0},
    'iron': {'section': 'Fence_IronSection', 'length': 200.0, 'end': 'Fence_IronPost', 'corner_post': 'Fence_IronPost',
             'gate': 'Fence_IronGate', 'gate_clear': (11.0, 189.0),
             'bury': 45.0, 'hover': 15.0, 'shortest': 0.5, 'step_post': 'Fence_IronPost', 'hide': 20.0,
             'clear_rock': 100.0},
    'stone': {'section': 'StoneWall', 'length': 300.0, 'broken': 'StoneWall_Broken', 'fallen': 'StoneWall_Fallen',
              'end': 'StoneWallEnd', 'end_length': 90.0, 'corner': 'StoneWall_Corner', 'arm': 150.0, 'wall': True,
              'half': 'StoneWall_Half', 'half_length': 150.0},
}

# The fences and walls (layout obstacle ids). kit: KITS; broken, fallen, missing: the share of sections drawn so (never
# a run's first or last missing, nor two side by side); gates: [X, Y, width] (cm) left open, or 'road': one in the
# middle of the side nearest a road; roads crossing the line open their own (ROAD_GAPS); inside: a point or 'zone:<id>'
# the kit's back faces (a path's; a polygon's faces out of it); corners: a wall's square corners on Ruins.py's piece;
# halves: half sections where whole ones won't fit; inner: extra lines inside (the pens' divider); signs: [mesh, the
# point whose nearest end of the line it stands by, how far in from that end and out in front of the line (cm)], facing
# out of it. owner: another builder places it, so it's left out here.
LINES = {
    # Delia's salt line round the farm, kept up: whole rails, gates where the farm road and the keeper's path pass.
    'saltLine': {'kit': 'rail', 'inside': 'zone:farm', 'gates': [[-1100, -2400, 450], [-1000, -8400, 300]]},
    # The family plot's picket fence: its gates (Fence_PicketGate_Open, the leaf swung back) where the family plot path
    # (east) and the bluff approach (west) leave it.
    'familyPlotFence': {'kit': 'picket', 'gates': [[-3680, -7950, 250], [-3650, -9250, 250]]},
    # The churchyard's iron fence: the double gate on the chapel's axis, before its door, where the roads meet.
    'churchyardFence': {'kit': 'iron', 'gates': [[5600, -1900, 200]]},
    # The empty stock pens: two pens, the gate into the east one from the street side, a gate between them, some rails
    # down.
    'stockPens': {'kit': 'rail', 'broken': 0.2, 'gates': [[-2600, -150, 300]],
                  'inner': [{'path': [[-2600, -500], [-4100, -400]], 'gates': [[-3350, -450, 300]]}]},
    # The Sink's broken warning fences, either side of the ramp head's gap, each with its warning sign by the gap
    # (where the Sink road comes up), standing out in front of the fence, its face away from the Sink.
    'sinkFenceSouth': {'kit': 'rail', 'broken': 0.35, 'missing': 0.12, 'inside': 'zone:sink',
                       'signs': [['SinkWarning', [4480, 4250], 120.0, 70.0]]},
    'sinkFenceNorth': {'kit': 'rail', 'broken': 0.35, 'missing': 0.12, 'inside': 'zone:sink',
                       'signs': [['SinkWarning', [4480, 4250], 120.0, 70.0]]},
    # The glebe wall, mostly down; the old pound wall, broken.
    'glebeWall': {'kit': 'stone', 'fallen': 0.7, 'broken': 0.1},
    'poundWall': {'kit': 'stone', 'broken': 0.45, 'fallen': 0.15},
    # The sheep fold: four square corners, a 2.2 m gateway toward the west road between finished ends (each side of it
    # a corner's arm and a half wall, Ruins.py's 1.5 m step), a breach or two.
    'sheepFold': {'kit': 'stone', 'broken': 0.25, 'fallen': 0.1, 'corners': True, 'halves': True, 'gates': 'road',
                  'gate_width': 220},
    # Whitlock Fields. Amos's fence: open where the fields path crosses it (his gate), the section just past the gate at
    # the kit's own 3 m, unstretched: Amos leans in its first span (build_area_whitlock.amos_spot), his pose fitted to
    # FenceRail. Its order is the layout's (east along it), so its front faces south, into his hayfield.
    'amosFence': {'kit': 'rail', 'first_after_gate': 300.0},
    # The barn yard's fence round the Whitlock barn and the windmill, its gate where the fields path ends.
    'barnYardFence': {'kit': 'rail', 'gates': [[-6000, 4800, 300]]},
    # The low stone field wall with larkspur splitting the hay, finished at both ends.
    'fieldWall': {'kit': 'stone'},
}
# A road crossing a fence or wall opens a gap as wide as the road takes up along the line (the road at least MIN_GATE
# wide, cm) and GATE_MARGIN more, so the posts stand clear of its edges; a gate standing there opens at least that much.
ROAD_GAPS = True
MIN_GATE = 250.0
GATE_MARGIN = 50.0
# A line's corner turns more than this (degrees): its sections end there (build_area_whitlock's CORNER).
CORNER = 20.0
# A wall's corner within this of square (degrees) gets the corner piece.
SQUARE = 20.0
# A run shorter than this (cm) between gates, or a leg shorter than this share of a section, is left out.
MIN_RUN = 150.0
MIN_LEG = 0.35
# Rock in a fence's way (a kit's clear_rock): looked for every ROCK_STEP (cm) along the line, on it and ROCK_SIDE either
# side of it, as rock standing between ROCK_LOW and ROCK_HIGH over the ground there (a cliff's lip or a talus chip lower
# than ROCK_LOW is something a fence stands over, not one it runs into).
ROCK_STEP = 25.0
ROCK_SIDE = 15.0
ROCK_LOW = 25.0
ROCK_HIGH = 150.0
# The placed meshes that are rock (build_area.py's cliff faces, panels and seams, the outcrops, Den Rock, boulders), by
# name. The scatter's rocks aren't actors; they keep off the fences' lines already (footprints()).
ROCK_MESHES = ('SM_Cliff', 'SM_Outcrop_', 'SM_DenRock', 'SM_Boulder_')

# The grave rows: the mix (mesh, weight), and how they stand: spacing (cm), how far out of line along and across (cm),
# how far turned and leaning (degrees), the share left out (a gap in the row).
GRAVE_STYLES = {
    # The town's own, a few of them fresh (a town in mourning), as the art session's chapel vignette mixes them.
    'churchyard': {'mix': [('Grave_Headboard_OldA', 3), ('Grave_Headboard_OldB', 3), ('Grave_Headboard_OldC', 2),
                           ('Grave_Headboard_OldD', 3), ('Grave_Cross_A', 2), ('Grave_Cross_B', 2),
                           ('Grave_Headboard_FreshA', 1), ('Grave_Headboard_FreshB', 1)],
                   'spacing': 170.0, 'along': 12.0, 'across': 8.0, 'turn': 5.0, 'lean': 3.0, 'missing': 0.06},
    # The Ransoms' old dead: weathered boards and a wooden cross, kept in line, each over its settled mound.
    'family': {'mix': [('Grave_Headboard_OldA', 2), ('Grave_Headboard_OldB', 2), ('Grave_Headboard_OldC', 1),
                       ('Grave_Headboard_OldD', 2), ('Grave_Cross_A', 1)],
               'spacing': 200.0, 'along': 8.0, 'across': 6.0, 'turn': 4.0, 'lean': 2.5, 'missing': 0.0,
               'mound': 'Grave_MoundSunken'},
    # The churchyard's short rows on the knoll's south face (EXTRA_GRAVE_ROWS), seen from boot hill, the north road and
    # the town: the same boards with more of the tall crosses, which read as a graveyard from 30-70 m, and none left out
    # (a row of two or five has no board to spare).
    'churchyardFace': {'mix': [('Grave_Headboard_OldA', 2), ('Grave_Headboard_OldB', 2), ('Grave_Headboard_OldC', 1),
                               ('Grave_Headboard_OldD', 2), ('Grave_Cross_A', 3), ('Grave_Cross_B', 3),
                               ('Grave_Headboard_FreshA', 1)],
                       'spacing': 170.0, 'along': 12.0, 'across': 8.0, 'turn': 5.0, 'lean': 3.0, 'missing': 0.0},
    # Boot hill's: ragged rows, more crosses, each over its settled mound (turned a little less than its looks would
    # like, so the 1 m mounds 1.65 m apart don't run into each other).
    'bootHill': {'mix': [('Grave_Headboard_OldA', 2), ('Grave_Headboard_OldB', 2), ('Grave_Headboard_OldC', 3),
                         ('Grave_Headboard_OldD', 1), ('Grave_Cross_A', 4)],
                 'spacing': 165.0, 'along': 25.0, 'across': 25.0, 'turn': 7.0, 'lean': 6.0, 'missing': 0.15,
                 'mound': 'Grave_MoundSunken'},
}
# A style's mound (Graves.py's Grave_MoundSunken: 2 x 1 m, its board at its +Y end, about 1.1 m behind its middle) lies
# in front of its board, its middle this far out (cm), turned as the board is; it tilts with the ground by at most
# MOUND_TILT degrees (it has no collision: it only has to hug the slope).
MOUND_OUT = 110.0
MOUND_TILT = 12.0
# Each row: its style and the yaw its boards' faces look (0 north, 90 east): the churchyard's toward the lanes either
# side of the nave, where the player walks; boot hill's south, down toward the town, as its respawn mound faces.
GRAVE_ROWS = {
    'churchyardGravesWest1': ('churchyard', 90.0), 'churchyardGravesWest2': ('churchyard', 90.0),
    'churchyardGravesWest3': ('churchyard', 90.0), 'churchyardGravesEast1': ('churchyard', -90.0),
    'churchyardGravesEast2': ('churchyard', -90.0), 'churchyardGravesEast3': ('churchyard', -90.0),
    'bootHillGraves1': ('bootHill', 180.0), 'bootHillGraves2': ('bootHill', 180.0),
    'bootHillGraves3': ('bootHill', 180.0),
}

# Grave rows the layout has no obstacle for, by a name of their own: the family plot's eight old headboards ("8 old
# headboards, Ellis's fresh grave and Abel's frosted one"): one either side of Ellis and Abel in their row (the boards
# at their spots are left out, as every row's are near a story grave) and six in a row behind them by the north
# fence, all facing south as theirs do, clear of the wake-up spot at the foot of Ellis's grave.
# The churchyard's south face (the art session's note from boot hill, 2026-10-07: its 54 boards stand west and east of
# the nave, the west rows behind the chapel and the knoll's crest and the east ones end-on, so from boot hill, the north
# road and the town it read empty): three short rows in front of the rows' heads, on the ground those see (the area
# model's heights: the town sees only the strip west of the tower), their boards facing down the knoll toward them.
# East of the gate, in front of the east rows' heads (2 m north of the respawn mound's foot, 2 m and more from the
# fight's spots on the lawn, the tree by the respawn grave to its west); west of the gate, in front of the west rows'
# heads, between the lawn's two spots there (the dead tree by the fence beside them), and beside the tower, clear of
# the way from its steps round into the lane west of the nave. build_area_chapel warns if a piece of the dressing comes
# within its YARD_CLEAR of a fight's spot.
EXTRA_GRAVE_ROWS = {
    'familyPlotGraves': [('family', 180.0, [[-3520, -9020], [-3520, -8210]], 270.0),
                         ('family', 180.0, [[-3200, -9100], [-3200, -8100]], 200.0)],
    'churchyardFaceGraves': [('churchyardFace', 170.0, [[6060, -1060], [6060, -330]], 180.0),
                             ('churchyardFace', 145.0, [[5870, -3200], [5870, -2990]], 210.0),
                             ('churchyardFace', 145.0, [[5900, -2620], [5900, -2450]], 170.0)],
}

# A grave row's board is left out on a road (its half width and this, cm) or this near a story grave's placement
# (cm: Ellis's, the respawn mounds and their boards).
GRAVE_CLEAR_ROAD = 30.0
GRAVE_CLEAR = 200.0
# The fresh mounds the placements put down (the respawn graves) get a fresh headboard at their head, this far (cm)
# behind their pivot (Graves.py's mound is 1.95 m long), facing as the mound does.
MOUND_BOARDS = {'Grave_MoundFresh': ['Grave_Headboard_FreshA', 'Grave_Headboard_FreshB']}
MOUND_HEAD = 105.0

# Pieces at spots of their own, with no layout obstacle (cm): at: [x, y, yaw] each; or from and to: a piece lying from
# one point to the other (unstretched, its middle between them; a fallen pine's stump at from).
SPOTS = {
    # Whitlock Fields' round bales lying out in the east field, the hay Amos never got in (nothing to do with them;
    # Side 2's bales are build_area_whitlock's), clear of the creek's bottom and the field wall.
    'whitlockRoundBales': {'mesh': 'HayBale_Round',
                           'at': [(-4600, 6700, 30), (-5400, 7000, -15), (-6800, 6600, 70), (-7600, 6300, 10)]},
    # Two more fallen pines (the art's suggestion), on the emptiest ground by the roads it named: in the fork where the
    # Sink road leaves the north road, and by the north road below the chapel. Each lies 20 degrees off its road's line,
    # 5-7 m from its middle, at least 4 m from any obstacle or building and clear of the ground left open on purpose,
    # the Sink, the boundary and the Ranger caches' spots (the ground there was 10.4 m from its nearest break).
    'sinkRoadPine': {'mesh': 'FallenPine', 'from': (2869, 1846), 'to': (3440, 2285)},
    'northRoadPine': {'mesh': 'FallenPine', 'from': (4273, -1157), 'to': (4843, -1598)},
    # The churchyard's dead trees (the area doc: "about 50 headboards behind an iron fence, with dead trees"; the
    # scatter keeps its trees out of the chapel's zone): one over the respawn grave on the face boot hill and the north
    # road see, one inside the fence's south-west corner, one past the west rows' far end against the sky; each 2 m and
    # more from the fight's spots, the graves and the fence.
    'churchyardDeadTrees': {'mesh': 'DeadTree_A', 'at': [(5850, -900, 205), (5720, -3100, 60), (7650, -3150, 330)]},
}

# Cairns along a path: the meshes in turn and the spacing (cm); placements: layout placements that already stand there.
CAIRN_LINES = {
    'keepersCairns': {'mix': ['Cairn_A', 'Cairn_B', 'Cairn_C'], 'spacing': 500.0,
                      'placements': ['cairn1', 'cairn2', 'cairn3']},
    'keepersPathCairn': {'mix': ['Cairn_B'], 'spacing': 500.0},
}

# Each ruin's models and the layout placements that already place them (build_area.py's models()).
RUINS = {
    'burntHomestead': [('Ruin_Chimney', 'burntHomestead')],
    'lanternHouse': [('Ruin_LanternHouse', 'lanternHouse')],
    'springhouse': [('Ruin_Springhouse', 'springhouse')],
    'burntLivery': [('Ruin_LiveryFooting', 'burntLivery')],
    'quarryDerrick': [('Ruin_Derrick', 'quarryDerrick')],
    'burntWagons': [('Wagon_BurntA', 'wagonA'), ('Wagon_BurntB', 'wagonB')],
}

# The prop groups, in the obstacle's frame: u along its longest side, v toward its front (cm). front: 'road' (its
# nearest road's side), ['toward' or 'away', a placement id], or a yaw. Items: [mesh, u, v, turn, z, stack]: turn is
# from facing the front (degrees), z lifts it (cm), and the items of one stack stand level on the lowest ground under
# them. jitter: how far each item is moved and turned at random (cm, degrees), from the obstacle's seed. against:
# pieces set against a building placement's wall, in its own frame: [mesh, the placement's id, cm ahead of its pivot
# (its front: +X), cm to its right (+Y), turn from its yaw] (a lean-to on a false front's back wall, beside its back
# door). A path's items: [mesh, from, to (shares of the path), stretched to fit (default yes)].
# Pieces in UPRIGHT stand plumb (a shed against a wall, on its posts), sunk to their lowest corner, never tilted.
UPRIGHT = ('Woodshed', 'LeanTo', 'DeadTree_A')
UPRIGHT_SINK = 40.0
GROUPS = {
    # A hitch rail and a trough at the town gate, by the street.
    'gateTrough': {'front': 'road', 'jitter': (5.0, 3.0), 'items': [
        ['HitchRail', -40, -15, 0], ['WaterTrough', 30, 70, 0], ['Barrel_A', -230, 40, 0]]},
    # A hitch rail and a trough where Main Street ends at the undertaker's yard, facing the yard.
    'yardTrough': {'front': 90.0, 'jitter': (5.0, 3.0), 'items': [
        ['HitchRail', -40, -15, 0], ['WaterTrough', 40, 70, 0]]},
    # Behind the Gilt Spur: the woodshed (its open front and chopping block toward the saloon's back door) and the
    # privy at the back of the lot, a barrel and a crate by the door; a lean-to on the saloon's back wall beside its
    # back door (the door 1.0-2.0 m right of its middle, seen from the street; the wall's face 7 m behind its pivot).
    'saloonBacklot': {'front': ['toward', 'saloon'], 'jitter': (8.0, 5.0), 'items': [
        ['Woodshed', -130, -50, 0], ['Outhouse', 220, -60, 0], ['Crate_A', 60, 120, 15], ['Barrel_A', 130, 110, 0]],
        'against': [['LeanTo', 'saloon', -795.0, -100.0, 180.0]]},
    # Behind Pruitt's store: a woodshed, crates (two stacked), barrels; a lean-to on the store's back wall beside its
    # back door (the door 1.2-2.25 m right of its middle; the wall's face 6.5 m behind its pivot).
    'storeBacklot': {'front': ['toward', 'store'], 'jitter': (6.0, 6.0), 'items': [
        ['Woodshed', -60, -40, 0], ['Crate_A', 165, -80, 0, 0, 'crates'], ['Crate_A', 165, -80, 6, 62, 'crates'],
        ['Crate_B', 175, 45, -20], ['Barrel_A', 90, 150, 0], ['Barrel_B', 190, 150, 0]],
        'against': [['LeanTo', 'store', -745.0, -80.0, 180.0]]},
    # The undertaker's yard: the lumber stack (its leaning planks toward the road), coffin stacks (three and two), a
    # broken one, crates, a barrel.
    'yardStacks': {'front': 'road', 'jitter': (4.0, 3.0), 'items': [
        ['LumberStack', -150, -20, 0], ['Coffin_Closed', 195, -70, 0, 0, 'a'], ['Coffin_Closed', 195, -70, 3, 44, 'a'],
        ['Coffin_Closed', 195, -70, -2, 88, 'a'], ['Coffin_Closed', 195, 15, 0, 0, 'b'],
        ['Coffin_Closed', 195, 15, -4, 44, 'b'], ['Coffin_Broken', 110, 120, 25], ['Crate_B', 40, -150, 10],
        ['Crate_A', 300, 120, -15], ['Barrel_A', -330, 165, 0]]},
    # The undertaker's dray, retired by the hearse car: along the plot, its bed's middle on the plot's (the pivot is
    # 1.22 m ahead of it), its shafts down toward Main Street's end and over the plot's edge; a barrel behind it and a
    # crate beside it.
    'yardWagon': {'front': 'road', 'jitter': (3.0, 2.0), 'items': [
        ['Dray', -122, 0, 90], ['Barrel_A', 185, 20, 0], ['Crate_A', 40, 125, 20]]},
    # Hay stacked against the barn's east wall: square bales in three courses (four, three and two long; three, three
    # and two deep), and two round bales and two loose square ones before it.
    'farmHayStack': {'front': ['away', 'barn'], 'jitter': (3.0, 4.0), 'items':
        [['HayBale_Square', u, v, 0, 0, 'hay'] for u in (-142.5, -47.5, 47.5, 142.5) for v in (-150, -102, -54)] +
        [['HayBale_Square', u, v, 0, 36, 'hay'] for u in (-95, 0, 95) for v in (-150, -102, -54)] +
        [['HayBale_Square', u, v, 0, 72, 'hay'] for u in (-47.5, 47.5) for v in (-150, -102)] +
        [['HayBale_Round', -120, 110, 0], ['HayBale_Round', 140, 125, 70], ['HayBale_Square', 215, 20, 35],
         ['HayBale_Square', -215, 30, -20]]},
    # A few low bales north of the family plot: two stacked, two loose.
    'farmHayBales': {'front': 'road', 'jitter': (6.0, 6.0), 'items': [
        ['HayBale_Square', -90, -40, 10, 0, 'pair'], ['HayBale_Square', -90, -40, 18, 36, 'pair'],
        ['HayBale_Square', 70, 50, -30], ['HayBale_Square', 120, -90, 75]]},
    # A fallen pine by the west road (Backlots.py's FallenPine: its stump at the path's start, its top toward the end).
    'westRoadLog': {'path': True, 'items': [['FallenPine', 0.0, 1.0, False]]},
}

# Obstacles of these kinds that something else places.
SKIP = {
    'townGateNorth': "the town gate (layout level.props) stands on its posts",
    'townGateSouth': "the town gate (layout level.props) stands on its posts",
}
KINDS = ('fence', 'wall', 'ruin', 'graves', 'cairns', 'props')

# Props tilt with the ground by at most this (degrees) and sink at most this (cm) so no corner hovers; sections sink
# at most SAG under a dip in their middle (they reach 15 cm under their pivot).
MAX_TILT = 4.0
MAX_SINK = 20.0
SAG = 25.0


# ---------------------------------------------------------------------------
# Lines on the ground (build_area_whitlock.py's, so both lay a fence the same way)
# ---------------------------------------------------------------------------

def length(points):
    return sum(math.dist(a, b) for a, b in zip(points, points[1:]))


def point_at(points, d):
    """The point d along a polyline (clamped to its ends)."""
    last = len(points) - 2
    for index, (a, b) in enumerate(zip(points, points[1:])):
        step = math.dist(a, b)
        if d <= step or index == last:
            t = 0.0 if step < 1e-6 else max(0.0, min(1.0, d / step))
            return a[0] + (b[0] - a[0]) * t, a[1] + (b[1] - a[1]) * t
        d -= step
    return points[-1]


def along(points, p):
    """How far along a polyline its point nearest p is."""
    best, best_d, walked = None, 0.0, 0.0
    for a, b in zip(points, points[1:]):
        dx, dy = b[0] - a[0], b[1] - a[1]
        step2 = dx * dx + dy * dy
        t = 0.0 if step2 < 1e-6 else max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / step2))
        gap = math.dist(p, (a[0] + dx * t, a[1] + dy * t))
        if best is None or gap < best:
            best, best_d = gap, walked + math.sqrt(step2) * t
        walked += math.sqrt(step2)
    return best_d


def cut(points, d0, d1):
    """The polyline from d0 along it to d1, with its corners between."""
    out = [point_at(points, d0)]
    walked = 0.0
    for a, b in zip(points, points[1:]):
        walked += math.dist(a, b)
        if d0 < walked < d1:
            out.append(b)
    out.append(point_at(points, d1))
    return out


def crossings(points, road):
    """Every point where a road's path crosses a polyline, with the sine of the angle between them there (1: square
    across)."""
    found = []
    for a, b in zip(points, points[1:]):
        for c, d in zip(road, road[1:]):
            r = (b[0] - a[0], b[1] - a[1])
            s = (d[0] - c[0], d[1] - c[1])
            den = r[0] * s[1] - r[1] * s[0]
            if abs(den) < 1e-9:
                continue
            t = ((c[0] - a[0]) * s[1] - (c[1] - a[1]) * s[0]) / den
            u = ((c[0] - a[0]) * r[1] - (c[1] - a[1]) * r[0]) / den
            if 0.0 <= t <= 1.0 and 0.0 <= u <= 1.0:
                found.append(((a[0] + r[0] * t, a[1] + r[1] * t), abs(den) / (math.hypot(*r) * math.hypot(*s))))
    return found


def turn_at(a, b, c):
    """How far (degrees) a line turns at b, signed: positive to the left of its way (toward its front)."""
    h0 = math.atan2(b[1] - a[1], b[0] - a[0])
    h1 = math.atan2(c[1] - b[1], c[0] - b[0])
    return math.degrees((h1 - h0 + math.pi) % (2.0 * math.pi) - math.pi)


def legs(points):
    """The polyline in legs between its corners (turns sharper than CORNER)."""
    out, leg = [], [points[0]]
    for i in range(1, len(points) - 1):
        leg.append(points[i])
        if abs(turn_at(points[i - 1], points[i], points[i + 1])) > CORNER:
            out.append(leg)
            leg = [points[i]]
    leg.append(points[-1])
    out.append(leg)
    return out


def joints(leg, section, first=0.0):
    """Evenly spaced joints along a leg, about a section apart; with first, the first two exactly that far apart. The
    count is the one that stretches or squashes the sections least (a 2.95 m leg of 2 m pickets gets two of 1.48 m, not
    one of 2.95)."""
    total = length(leg)
    marks = [0.0]
    start = 0.0
    if first and total > first + section * 0.5:
        marks.append(first)
        start = first
    rest = max(total - start, 1e-3)
    fewer = max(1, int(rest // section))
    count = min((fewer, fewer + 1), key=lambda n: abs(math.log(rest / n / section)))
    marks += [start + rest * k / count for k in range(1, count + 1)]
    return [point_at(leg, d) for d in marks]


def kit_yaw(dx, dy):
    """The yaw that runs a kit piece (along its actor's -Y, its front the actor's +X) from its pivot toward (dx, dy)."""
    return math.degrees(math.atan2(dx, -dy))


def unit(a, b):
    d = math.dist(a, b)
    return ((b[0] - a[0]) / d, (b[1] - a[1]) / d) if d > 1e-6 else (1.0, 0.0)


def signed_area(points):
    return 0.5 * sum(x0 * y1 - x1 * y0 for (x0, y0), (x1, y1) in zip(points, list(points[1:]) + [points[0]]))


def centroid(points):
    area = signed_area(points)
    if abs(area) < 1e-6:
        return sum(p[0] for p in points) / len(points), sum(p[1] for p in points) / len(points)
    cx = sum((x0 + x1) * (x0 * y1 - x1 * y0) for (x0, y0), (x1, y1) in zip(points, list(points[1:]) + [points[0]]))
    cy = sum((y0 + y1) * (x0 * y1 - x1 * y0) for (x0, y0), (x1, y1) in zip(points, list(points[1:]) + [points[0]]))
    return cx / (6.0 * area), cy / (6.0 * area)


def nearest_on(points, p):
    """The point of a polyline nearest p, and how far it is."""
    best, best_d = points[0], None
    for a, b in zip(points, points[1:]):
        dx, dy = b[0] - a[0], b[1] - a[1]
        step2 = dx * dx + dy * dy
        t = 0.0 if step2 < 1e-6 else max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / step2))
        q = (a[0] + dx * t, a[1] + dy * t)
        if best_d is None or math.dist(p, q) < best_d:
            best, best_d = q, math.dist(p, q)
    return best, best_d


class Piece:
    """One kit piece: a span from a to b (its pivot at a, or at the middle with a middle pivot), or a piece at a with a
    yaw. stretch: a span stretched to its chord; level: a stack's id (it stands level) or None; lift: cm over its
    ground; lean: (pitch, roll) added (degrees)."""

    def __init__(self, mesh, owner, a, yaw=0.0, b=None, stretch=False, kind='prop', level=None, lift=0.0,
                 lean=(0.0, 0.0), extra=None):
        self.mesh, self.owner, self.a, self.b, self.yaw = mesh, owner, a, b, yaw
        self.stretch, self.kind, self.level, self.lift, self.lean, self.extra = stretch, kind, level, lift, lean, extra
        if b is not None:
            self.yaw = kit_yaw(b[0] - a[0], b[1] - a[1])


# ---------------------------------------------------------------------------
# The plan (plain Python: the layout's JSON in, pieces out)
# ---------------------------------------------------------------------------

class Plan:
    """What the dressing places for a layout (layout.json as source, layout_computed.json's placements): the instanced
    pieces, the ruins to place as actors ((model, owner, x, y, yaw)), per obstacle a line of what it got, and per fence
    or wall its runs' first sections (heads: [(how the run starts: 'open', 'gate', 'rock' or 'loop', its first joint,
    its second)], so a builder can stand someone at a span, as build_area_whitlock stands Amos at the one past his gate).
    rocks: where rock stands in a fence's way ((x, y) -> bool; place()'s Rocks, from the cliffs and rocks placed), so a
    run stops short of it; without it (plain Python, footprints(), run_head()) the runs go on as the layout draws them."""

    def __init__(self, source, placements=None, only=None, rocks=None):
        self.source = source
        self.rocks = rocks
        # layout_computed.json's placements (key: kind, location, yaw), which build_area.py's models() places.
        self.placed = dict(placements or {})
        self.roads = [(r['path'], float(r.get('width', MIN_GATE))) for r in source.get('roads', [])
                      if len(r['path']) > 1]
        self.zones = {z['id']: z['polygon'] for z in source.get('zones', [])}
        self.where = {p['id']: p['location'] for p in source.get('placements', [])}
        self.pieces, self.ruins, self.notes, self.warnings, self.heads = [], [], {}, [], {}
        for entry in source.get('obstacles', []):
            if entry.get('kind') not in KINDS or (only and entry['id'] not in only):
                continue
            self.obstacle(entry)
        if not only:
            for oid, rows in EXTRA_GRAVE_ROWS.items():
                before = len(self.pieces)
                texts = [self.graves(oid, {'path': path}, style, face, spacing) for style, face, path, spacing in rows]
                self.notes[oid] = f'{len(self.pieces) - before} pieces; ' + '; '.join(texts)
            for oid, spot in SPOTS.items():
                for x, y, yaw in spot.get('at', ()):
                    self.pieces.append(Piece(spot['mesh'], oid, (x, y), yaw, kind=piece_kind(spot['mesh'])))
                if 'from' in spot:
                    self.pieces.append(Piece(spot['mesh'], oid, tuple(spot['from']), b=tuple(spot['to']), kind='log'))
                self.notes[oid] = f"{len(spot.get('at', ())) + (1 if 'from' in spot else 0)} {spot['mesh']}"
            self.mound_boards()

    def on_road(self, point, margin=GRAVE_CLEAR_ROAD):
        """Whether a point stands on a road (within its half width and margin, cm)."""
        return any(nearest_on(road, point)[1] < width * 0.5 + margin for road, width in self.roads)

    def mound_boards(self):
        """A fresh headboard at the head of every fresh mound the placements put down (Graves.py: "put a headboard at
        its +Y end", the model's back, its actor's -X): the respawn graves in the chapel yard and on boot hill."""
        for key, spot in sorted(self.placed.items()):
            boards = MOUND_BOARDS.get(spot.get('kind'))
            if not boards:
                continue
            x, y = spot['location'][:2]
            yaw = math.radians(spot.get('yaw', 0.0))
            rnd = random.Random(key)
            at = (x - math.cos(yaw) * MOUND_HEAD, y - math.sin(yaw) * MOUND_HEAD)
            self.pieces.append(Piece(rnd.choice(boards), key, at, spot.get('yaw', 0.0) + rnd.uniform(-3.0, 3.0),
                                     lean=(rnd.uniform(-1.5, 1.5), rnd.uniform(-1.5, 1.5)), kind='board'))
            self.notes[key] = f'a fresh headboard at the head of its mound ({spot["kind"]})'

    def obstacle(self, entry):
        oid = entry['id']
        before = len(self.pieces)
        if oid in SKIP:
            self.notes[oid] = 'left out: ' + SKIP[oid]
            return
        if oid in LINES:
            spec = LINES[oid]
            if spec.get('owner'):
                self.notes[oid] = f"left out: {spec['owner']}.py places it"
                return
            text = self.line(oid, entry, spec)
            for k, inner in enumerate(spec.get('inner', [])):
                text += '; inner: ' + self.line(f'{oid}.inner{k + 1}', {'path': inner['path']},
                                                dict(spec, gates=inner.get('gates', []), inner=[]))
        elif oid in GRAVE_ROWS:
            text = self.graves(oid, entry, *GRAVE_ROWS[oid])
        elif oid in CAIRN_LINES:
            text = self.cairns(oid, entry, CAIRN_LINES[oid])
        elif oid in RUINS:
            text = self.ruin(oid, entry, RUINS[oid])
        elif oid in GROUPS:
            text = self.group(oid, entry, GROUPS[oid])
        else:
            self.warnings.append(f'obstacle {oid} ({entry["kind"]}) has no entry in the dressing tables: left out')
            return
        counts = {}
        for piece in self.pieces[before:]:
            counts[piece.mesh] = counts.get(piece.mesh, 0) + 1
        made = ', '.join(f'{n} {m}' for m, n in sorted(counts.items()))
        self.notes[oid] = f'{len(self.pieces) - before} pieces ({made}){"; " + text if text else ""}'

    def point_ref(self, ref):
        """A point from a table: [X, Y], 'zone:<id>' (its middle) or a placement id."""
        if isinstance(ref, str):
            if ref.startswith('zone:'):
                return centroid(self.zones[ref[5:]])
            return tuple(self.where[ref][:2])
        return tuple(ref[:2])

    # --- Fences and walls ---

    def gaps(self, line, closed, spec, kit):
        """The line's openings: (from, to along it, why, clear width), widened by a wall's ends so the opening stays
        clear. A road crossing opens as much of the line as the road takes up where it crosses (more than its width when
        it crosses at a slant) and GATE_MARGIN more, and a gate at a crossing opens at least that much."""
        total = length(line)
        wanted = []
        gates = spec.get('gates', [])
        if gates == 'road':
            # The middle of the side nearest a road.
            sides = list(zip(line, line[1:]))
            best = min(sides, key=lambda s: min(nearest_on(r, ((s[0][0] + s[1][0]) / 2, (s[0][1] + s[1][1]) / 2))[1]
                                                for r, _ in self.roads))
            middle = ((best[0][0] + best[1][0]) / 2, (best[0][1] + best[1][1]) / 2)
            gates = [middle + (spec.get('gate_width', MIN_GATE),)]
        for x, y, width in gates:
            at = along(line, (x, y))
            wanted.append([at - width * 0.5, at + width * 0.5, 'gate'])
        if ROAD_GAPS:
            for road, width in self.roads:
                for p, sine in crossings(line, road):
                    at = along(line, p)
                    half = (max(width, MIN_GATE) / max(sine, 0.35) + GATE_MARGIN) * 0.5
                    near = next((w for w in wanted if w[0] - half <= at <= w[1] + half), None)
                    if near is None:
                        wanted.append([at - half, at + half, 'road'])
                    else:
                        near[0], near[1] = min(near[0], at - half), max(near[1], at + half)
        out = []
        for d0, d1, why in wanted:
            middle = (d0 + d1) * 0.5
            if kit.get('gate'):
                # The gate unit fills its gap whole, its way through (gate_clear) centred on the gate's spot.
                unit_length = PIECES[kit['gate']][0][0]
                clear = kit.get('gate_clear', (0.0, unit_length))
                begin = middle - (clear[0] + clear[1]) * 0.5
                out.append((begin, begin + unit_length, why, clear[1] - clear[0]))
                continue
            reach = (d1 - d0) * 0.5 + kit.get('end_length', 0.0)
            out.append((middle - reach, middle + reach, why, d1 - d0))
        if not closed:
            out = [g for g in out if g[1] > 0.0 and g[0] < total]
        return sorted(out)

    def line(self, oid, entry, spec):
        kit = KITS[spec['kit']]
        closed = 'polygon' in entry
        points = [tuple(p) for p in (entry['polygon'] if closed else entry['path'])]
        if closed and signed_area(points) > 0.0:
            # The kit's front (its left, seen along its way) faces out of the polygon.
            points.reverse()
        if not closed and spec.get('inside'):
            inside = self.point_ref(spec['inside'])
            near = min(zip(points, points[1:]), key=lambda s: nearest_on(list(s), inside)[1])
            (ax, ay), (bx, by) = near
            if (bx - ax) * (inside[1] - ay) - (by - ay) * (inside[0] - ax) > 0.0:
                points.reverse()
        line = points + [points[0]] if closed else points
        total = length(line)
        gaps = self.gaps(line, closed, spec, kit)
        runs = []
        if not gaps:
            if closed:
                # Round from the middle of its longest side, so every corner lies inside the run.
                side = max(range(len(points)), key=lambda i: math.dist(line[i], line[i + 1]))
                start = length(line[:side + 1]) + math.dist(line[side], line[side + 1]) * 0.5
                runs.append((start, start + total, 'loop', 'loop'))
            else:
                runs.append((0.0, total, 'open', 'open'))
        elif closed:
            for i, gap in enumerate(gaps):
                nxt = gaps[(i + 1) % len(gaps)]
                d0, d1 = gap[1], nxt[0] + (total if i == len(gaps) - 1 else 0.0)
                if d0 < 0.0:
                    d0, d1 = d0 + total, d1 + total
                runs.append((d0, d1, 'gate', 'gate'))
        else:
            d0, kind0 = 0.0, 'open'
            for gap in gaps:
                runs.append((d0, gap[0], kind0, 'gate'))
                d0, kind0 = gap[1], 'gate'
            runs.append((d0, total, kind0, 'open'))
        # Where rock stands in its way, a run stops kit['clear_rock'] short of it and goes on as far past it.
        rock = self.rock_spans(line, total, kit)
        runs = stop_at_rock(runs, rock, total, closed)
        # A loop is cut from three rounds of it, so a run may start past its end and go round.
        track = line + line[1:] + line[1:] if closed else line
        lengths = []
        for index, (d0, d1, start, end) in enumerate(runs):
            if d1 - d0 < MIN_RUN:
                continue
            first = spec.get('first_after_gate', 0.0) if start == 'gate' and not closed else 0.0
            self.run(oid, index, cut(track, d0, d1), kit, spec, start, end, first)
            lengths.append(f'{(d1 - d0) / 100.0:.1f}')
        for mesh, near, inward, out in spec.get('signs', []):
            # By the line's end nearest near, inward along it, out in front of it (its front: the kit's), facing out.
            at = along(line, tuple(near))
            d = at + inward if at < total * 0.5 else at - inward
            p = point_at(track, d % total if closed else d)
            q = point_at(track, (d % total if closed else d) + 1.0)
            if math.dist(p, q) < 0.5:
                q, p = p, point_at(track, (d % total if closed else d) - 1.0)
            way = unit(p, q)
            front = (-way[1], way[0])
            self.pieces.append(Piece(mesh, oid, (p[0] + front[0] * out, p[1] + front[1] * out),
                                     math.degrees(math.atan2(front[1], front[0])), kind='upright'))
        if kit.get('gate'):
            # The gate itself fills its gap (the iron fence's double gate, the pickets' gate, their leaves open inward).
            for d0, d1, _, _ in gaps:
                d0 = d0 % total if closed else d0
                self.pieces.append(Piece(kit['gate'], oid, point_at(track, d0), b=point_at(track, d0 + (d1 - d0)),
                                         stretch=True, kind='span'))
        gap_text = ', '.join(f'{why} {w / 100.0:.1f} m at {((d0 + d1) / 2) % total / 100.0:.1f} m'
                             for d0, d1, why, w in gaps)
        rock_text = ', '.join(f'{(d1 - d0) / 100.0:.1f} m at {((d0 + d1) / 2) % total / 100.0:.1f} m' for d0, d1 in rock)
        return (f'{total / 100.0:.1f} m {"round" if closed else "long"}, runs {" + ".join(lengths) or "none"} m'
                + (f', gaps: {gap_text}' if gaps else '')
                + (f', left clear of rock (its margin included): {rock_text}' if rock else ''))

    def rock_spans(self, line, total, kit):
        """Where along a line (from, to: cm) rock stands in a fence's way, each widened by the kit's clear_rock either
        side; none without a rocks test or for a kit that runs into rock (a wall)."""
        clear = kit.get('clear_rock')
        if not self.rocks or not clear or not self.rocks.near(line, clear + ROCK_SIDE):
            return []
        count = max(1, int(math.ceil(total / ROCK_STEP)))
        hit = []
        for i in range(count + 1):
            d = total * i / count
            p = point_at(line, d)
            ahead = point_at(line, min(d + 1.0, total))
            way = unit(point_at(line, max(d - 1.0, 0.0)), ahead)
            side = (-way[1] * ROCK_SIDE, way[0] * ROCK_SIDE)
            if any(self.rocks((p[0] + side[0] * s, p[1] + side[1] * s)) for s in (0.0, 1.0, -1.0)):
                hit.append(d)
        spans = []
        step = total / count
        for d in hit:
            if spans and d - spans[-1][1] <= step + 1.0:
                spans[-1][1] = d
            else:
                spans.append([d, d])
        # The rock's edge lies within a step before its first sample and after its last: clear of that whole step.
        widened = []
        for d0, d1 in spans:
            d0, d1 = d0 - step - clear, d1 + step + clear
            if widened and d0 <= widened[-1][1]:
                widened[-1][1] = d1
            else:
                widened.append([d0, d1])
        return [(d0, d1) for d0, d1 in widened]

    def run(self, oid, index, poly, kit, spec, start, end, first):
        """One run between gates (or round a loop): its legs, corners, sections, and the posts or ends between them."""
        section = kit['length']
        parts = legs(poly)
        # Square corners where a wall turns away from its front (Ruins.py's StoneWall_Corner turns that way), each arm
        # taken off the legs either side.
        corner_at = set()
        if kit.get('corner') and spec.get('corners'):
            arm = kit['arm']
            for i in range(len(parts) - 1):
                turned = turn_at(parts[i][-2], parts[i][-1], parts[i + 1][1])
                if abs(abs(turned) - 90.0) <= SQUARE and turned < 0.0 and length(parts[i]) > arm + section * MIN_LEG \
                        and length(parts[i + 1]) > arm + section * MIN_LEG:
                    corner_at.add(i)
            # From the last corner back, so each cut leg is still whole when its other end is cut.
            for i in sorted(corner_at, reverse=True):
                parts[i] = cut(parts[i], 0.0, length(parts[i]) - arm)
                parts[i + 1] = cut(parts[i + 1], arm, length(parts[i + 1]))
        elements = []
        for i, leg in enumerate(parts):
            # A sliver of a leg at a run's end (a gate cut close to a corner) is left out; one inside a run isn't, or
            # the run would have a hole.
            if length(leg) >= section * MIN_LEG or 0 < i < len(parts) - 1:
                if spec.get('halves') and kit.get('half'):
                    spots, halves = half_joints(leg, section, kit['half_length'])
                else:
                    spots = joints(leg, section, first if i == 0 else 0.0)
                    halves = [False] * (len(spots) - 1)
                for a, b, half in zip(spots, spots[1:], halves):
                    elements.append({'type': 'section', 'a': a, 'b': b, 'dir': unit(a, b), 'half': half})
            if i < len(parts) - 1:
                if i in corner_at:
                    a, v = parts[i][-1], poly_vertex(parts[i], parts[i + 1])
                    elements.append({'type': 'corner', 'a': a, 'b': parts[i + 1][0], 'dir': unit(a, v),
                                     'out': unit(v, parts[i + 1][0]), 'v': v, 'state': 'C'})
                elif kit.get('corner_post'):
                    self.pieces.append(Piece(kit['corner_post'], oid, leg[-1], kit_yaw(*unit(leg[-2], leg[-1])),
                                             kind='post'))
        sections = [e for e in elements if e['type'] == 'section']
        if sections:
            self.heads.setdefault(oid, []).append((start, sections[0]['a'], sections[0]['b']))
        for k, e in enumerate(sections):
            rnd = random.Random(f'{oid} {index} {k}')
            draw = rnd.random()
            edge = k == 0 or k == len(sections) - 1
            prev_missing = k > 0 and sections[k - 1].get('state') == 'M'
            missing = 0.0 if edge or prev_missing else spec.get('missing', 0.0)
            fallen = spec.get('fallen', 0.0) if kit.get('fallen') else 0.0
            broken = spec.get('broken', 0.0) if kit.get('broken') else 0.0
            e['state'] = 'M' if draw < missing else 'F' if draw < missing + fallen else \
                'B' if draw < missing + fallen + broken else 'S'
            if e.get('half'):
                # The kit has no broken or fallen half: a half always stands.
                e['state'] = 'H'
        meshes = {'S': kit['section'], 'B': kit.get('broken'), 'F': kit.get('fallen'), 'H': kit.get('half')}
        for e in elements:
            if e['type'] == 'corner':
                self.pieces.append(Piece(kit['corner'], oid, e['a'], kit_yaw(*e['dir']), kind='corner',
                                         extra=(e['v'], e['b'])))
            elif e['state'] != 'M':
                self.pieces.append(Piece(meshes[e['state']], oid, e['a'], b=e['b'], stretch=True, kind='span'))
        if not elements:
            return
        if kit.get('wall'):
            self.wall_ends(oid, elements, kit, start, end)
        else:
            self.fence_posts(oid, elements, kit, end)

    def fence_posts(self, oid, elements, kit, end):
        """A post where a run's rails stop: before a missing section and at the run's end (its open end, or short of the
        rock it stops at; a gate unit's own posts close it for the iron and picket fences, and a loop closes on its own
        first post)."""
        for i, e in enumerate(elements):
            if e['state'] == 'M':
                continue
            last = i == len(elements) - 1
            if (last and end in ('open', 'rock')) or (last and end == 'gate' and not kit.get('gate')) or \
                    (not last and elements[i + 1]['state'] == 'M'):
                self.pieces.append(Piece(kit['end'], oid, e['b'], kit_yaw(*e['dir']), kind='post'))

    def wall_ends(self, oid, elements, kit, start, end):
        """A finished end wherever standing wall meets fallen, missing or open ground: past the standing piece's last
        joint, or turned back from its first."""
        def full(e):
            return e['state'] in ('S', 'B', 'C', 'H')

        def forward(e):
            d = e.get('out', e['dir'])
            self.pieces.append(Piece(kit['end'], oid, e['b'], b=(e['b'][0] + d[0] * kit['end_length'],
                                                                   e['b'][1] + d[1] * kit['end_length']), kind='span'))

        def back(e):
            d = e['dir']
            self.pieces.append(Piece(kit['end'], oid, e['a'], b=(e['a'][0] - d[0] * kit['end_length'],
                                                                   e['a'][1] - d[1] * kit['end_length']), kind='span'))
        count = len(elements)
        pairs = [(elements[i], elements[i + 1]) for i in range(count - 1)]
        if start == 'loop':
            pairs.append((elements[-1], elements[0]))
        for e0, e1 in pairs:
            if full(e0) and not full(e1):
                forward(e0)
            elif full(e1) and not full(e0):
                back(e1)
        if start != 'loop' and full(elements[0]):
            back(elements[0])
        if end != 'loop' and full(elements[-1]):
            forward(elements[-1])

    # --- Graves, cairns, ruins ---

    def graves(self, oid, entry, style_name, face, spacing=None):
        style = GRAVE_STYLES[style_name]
        path = [tuple(p) for p in entry['path']]
        total = length(path)
        count = max(1, round(total / (spacing or style['spacing']))) + 1
        names = [m for m, _ in style['mix']]
        weights = [w for _, w in style['mix']]
        across = (math.cos(math.radians(face)), math.sin(math.radians(face)))
        graves = [s['location'][:2] for s in self.placed.values() if str(s.get('kind', '')).startswith('Grave_')]
        left = cleared = 0
        for k in range(count):
            rnd = random.Random(f'{oid} {path[0]} {k}')
            if rnd.random() < style['missing']:
                left += 1
                continue
            mesh = rnd.choices(names, weights)[0]
            x, y = point_at(path, total * k / max(count - 1, 1) + rnd.uniform(-style['along'], style['along']))
            off = rnd.uniform(-style['across'], style['across'])
            lean = (rnd.uniform(-style['lean'], style['lean']), rnd.uniform(-style['lean'], style['lean']))
            at = (x + across[0] * off, y + across[1] * off)
            yaw = face + rnd.uniform(-style['turn'], style['turn'])
            # Its mound (when the style has one) out in front of the board, where the board faces.
            mound = (at[0] + math.cos(math.radians(yaw)) * MOUND_OUT, at[1] + math.sin(math.radians(yaw)) * MOUND_OUT)
            spots = (at, mound) if style.get('mound') else (at,)
            # Never on a road, nor crowding a story grave (a respawn mound and its board).
            if any(self.on_road(p) or any(math.dist(p, g) < GRAVE_CLEAR for g in graves) for p in spots):
                cleared += 1
                continue
            self.pieces.append(Piece(mesh, oid, at, yaw, lean=lean, kind='board'))
            if style.get('mound'):
                self.pieces.append(Piece(style['mound'], oid, mound, yaw, kind='mound'))
        return (f'{total / 100.0:.1f} m, {count - left - cleared} of {count} graves'
                + (' (each over its mound)' if style.get('mound') else '')
                + (f' ({cleared} cleared for a road or a story grave)' if cleared else ''))

    def cairns(self, oid, entry, spec):
        if spec.get('placements') and all(key in self.placed for key in spec['placements']):
            return f"placed by build_area.py's models() (placements {', '.join(spec['placements'])})"
        path = [tuple(p) for p in entry['path']]
        total = length(path)
        count = max(1, round(total / spec['spacing']) + (1 if total >= spec['spacing'] else 0))
        for k in range(count):
            d = total * 0.5 if count == 1 else total * k / (count - 1)
            rnd = random.Random(f'{oid} {k}')
            self.pieces.append(Piece(spec['mix'][k % len(spec['mix'])], oid, point_at(path, d),
                                     rnd.uniform(-180.0, 180.0), kind='cairn'))
        return f'{total / 100.0:.1f} m'

    def ruin(self, oid, entry, models):
        missing = [(m, key) for m, key in models if key not in self.placed]
        if not missing:
            return f"placed by build_area.py's models() (placements {', '.join(k for _, k in models)})"
        pts = [tuple(p) for p in entry.get('polygon') or entry['path']]
        for n, (model, key) in enumerate(missing):
            if 'polygon' in entry:
                (x, y), (u, _) = centroid(pts), frame_axes(pts)
            else:
                x, y = point_at(pts, length(pts) * (n + 0.5) / len(missing))
                u = unit(pts[0], pts[-1])
            self.ruins.append((model, oid, x, y, kit_yaw(*u)))
        return f'{len(missing)} ruin model(s) placed here (no placement {", ".join(k for _, k in missing)})'

    # --- Prop groups ---

    def group(self, oid, entry, spec):
        rnd = random.Random(oid)
        shift, twist = spec.get('jitter', (0.0, 0.0))
        if spec.get('path'):
            path = [tuple(p) for p in entry['path']]
            total = length(path)
            for item in spec['items']:
                mesh, t0, t1 = item[:3]
                a, b = point_at(path, total * t0), point_at(path, total * t1)
                if math.dist(a, b) < 1.0:
                    self.pieces.append(Piece(mesh, oid, a, kit_yaw(*unit(path[0], path[-1])) + rnd.uniform(0, 360),
                                             kind=piece_kind(mesh)))
                else:
                    self.pieces.append(Piece(mesh, oid, a, b=b, stretch=item[3] if len(item) > 3 else True,
                                             kind='log'))
            return f'{total / 100.0:.1f} m'
        pts = [tuple(p) for p in entry['polygon']]
        center = centroid(pts)
        _, normal = frame_axes(pts)
        front = spec.get('front', 'road')
        if isinstance(front, (int, float)):
            f = (math.cos(math.radians(front)), math.sin(math.radians(front)))
        else:
            if front == 'road':
                target = min((nearest_on(r, center) for r, _ in self.roads), key=lambda n: n[1])[0]
                sign = 1.0
            else:
                target = self.point_ref(front[1])
                sign = 1.0 if front[0] == 'toward' else -1.0
            to = ((target[0] - center[0]) * sign, (target[1] - center[1]) * sign)
            f = normal if normal[0] * to[0] + normal[1] * to[1] >= 0.0 else (-normal[0], -normal[1])
        u = (f[1], -f[0])
        facing = math.degrees(math.atan2(f[1], f[0]))
        for item in spec['items']:
            mesh, du, dv, turn = item[:4]
            lift = item[4] if len(item) > 4 else 0.0
            stack = item[5] if len(item) > 5 else None
            du += rnd.uniform(-shift, shift)
            dv += rnd.uniform(-shift, shift)
            x, y = center[0] + u[0] * du + f[0] * dv, center[1] + u[1] * du + f[1] * dv
            self.pieces.append(Piece(mesh, oid, (x, y), facing + turn + rnd.uniform(-twist, twist),
                                     level=f'{oid}.{stack}' if stack else None, lift=lift, kind=piece_kind(mesh)))
        for mesh, host, ahead, right, turn in spec.get('against', []):
            # In the building's own frame (its +X its front, its +Y its right), from the layout's placement.
            spot = next((p for p in self.source.get('placements', []) if p['id'] == host), None)
            if spot is None:
                self.warnings.append(f'{oid}: no placement {host} to stand its {mesh} against: left out')
                continue
            (hx, hy), yaw = spot['location'][:2], math.radians(spot.get('yaw', 0.0))
            x = hx + math.cos(yaw) * ahead - math.sin(yaw) * right
            y = hy + math.sin(yaw) * ahead + math.cos(yaw) * right
            self.pieces.append(Piece(mesh, oid, (x, y), spot.get('yaw', 0.0) + turn, kind=piece_kind(mesh)))
        return f'facing {facing:.0f}'


def piece_kind(mesh):
    """How a free-standing piece stands: plumb (UPRIGHT), or a prop tilted a little with the ground."""
    return 'upright' if mesh in UPRIGHT else 'prop'


def stop_at_rock(runs, rock, total, closed):
    """The runs ((from, to along the line, how each starts and ends)) with rock's spans ((from, to)) taken out: a run
    that meets one ends there ('rock') and the line goes on past it as a run of its own, starting 'rock'. A loop the
    rock breaks becomes a run from each span's far side round to the next one's near side."""
    if not rock:
        return runs
    if closed and runs and runs[0][2] == 'loop':
        start = runs[0][0]
        # The spans in the loop's own frame (0 at the run's start), split where they cross it, merged.
        parts = []
        for d0, d1 in rock:
            if d1 - d0 >= total:
                return []
            u0 = (d0 - start) % total
            u1 = u0 + (d1 - d0)
            parts.append((u0, min(u1, total)))
            if u1 > total:
                parts.append((0.0, u1 - total))
        merged = []
        for u0, u1 in sorted(parts):
            if merged and u0 <= merged[-1][1]:
                merged[-1][1] = max(merged[-1][1], u1)
            else:
                merged.append([u0, u1])
        if len(merged) > 1 and merged[0][0] <= 0.0 and merged[-1][1] >= total:
            first = merged.pop(0)
            merged[-1][1] = first[1] + total
        out = []
        for i, (u0, u1) in enumerate(merged):
            following = merged[(i + 1) % len(merged)][0] + (total if i == len(merged) - 1 else 0.0)
            if following > u1:
                out.append((start + u1, start + following, 'rock', 'rock'))
        return out
    # A closed line's runs are measured round it from anywhere in its first two rounds: the spans of each round.
    shifts = (-total, 0.0, total, 2.0 * total) if closed else (0.0,)
    spans = sorted((d0 + s, d1 + s) for d0, d1 in rock for s in shifts)
    out = []
    for d0, d1, start, end in runs:
        at, how = d0, start
        for b0, b1 in spans:
            if b1 <= at or b0 >= d1:
                continue
            if b0 > at:
                out.append((at, b0, how, 'rock'))
            at, how = max(at, b1), 'rock'
        if at < d1:
            out.append((at, d1, how, end))
    return out


def half_joints(leg, section, half):
    """Joints along a leg for whole sections and at most one half one (the last), the mix stretched least (whole ones
    win a tie); and which of the sections is the half."""
    total = max(length(leg), 1e-3)
    best = None
    for whole in range(int(total // section) + 2):
        for halves in (0, 1):
            nominal = whole * section + halves * half
            if nominal > 0.0 and (best is None or abs(math.log(total / nominal)) < best[0] - 1e-9):
                best = (abs(math.log(total / nominal)), whole, halves)
    _, whole, halves = best
    scale = total / (whole * section + halves * half)
    marks = [0.0]
    for k in range(whole + halves):
        marks.append(marks[-1] + (half if k >= whole else section) * scale)
    return [point_at(leg, d) for d in marks], [k >= whole for k in range(whole + halves)]


def poly_vertex(leg_in, leg_out):
    """Where two trimmed legs' lines meet: the corner they were cut back from."""
    (ax, ay), (bx, by) = leg_in[-2], leg_in[-1]
    (cx, cy), (dx, dy) = leg_out[0], leg_out[1]
    r, s = (bx - ax, by - ay), (dx - cx, dy - cy)
    den = r[0] * s[1] - r[1] * s[0]
    if abs(den) < 1e-9:
        return bx, by
    t = ((cx - ax) * s[1] - (cy - ay) * s[0]) / den
    return ax + r[0] * t, ay + r[1] * t


def frame_axes(points):
    """A polygon's long axis (its longest side's way) and that axis's left normal."""
    sides = list(zip(points, points[1:] + points[:1]))
    a, b = max(sides, key=lambda s: math.dist(*s))
    u = unit(a, b)
    return u, (-u[1], u[0])


def plan_all(source, placements=None, only=None, rocks=None):
    """The plan for a layout (layout.json) and what build_area.py's models() places (layout_computed.json's placements);
    only: just these obstacle ids (build_area_whitlock could take its fences' plan so); rocks: Plan's."""
    return Plan(source, placements, only, rocks)


# ---------------------------------------------------------------------------
# Standing on the ground (plain Python: ground is any (x, y) -> z)
# ---------------------------------------------------------------------------

def poses(plan, ground, stats=None):
    """Each piece's (mesh, (x, y, z), (roll, pitch, yaw), (sx, sy, sz), owner), on the ground ground(x, y) gives: a
    fence's section as its level steps (fence_steps), and its kit's step post wherever two steps meet further apart
    than its own posts cover. stats (a dict) counts what the slopes made of the fences: 'split' sections, 'step posts',
    and 'past': (degrees, owner) of each section steeper than its kit's shortest steps take cleanly."""
    stats = {} if stats is None else stats
    for key in ('split', 'step posts'):
        stats.setdefault(key, 0)
    stats.setdefault('past', [])
    bases = {}
    posts = {}
    for piece in plan.pieces:
        if piece.level:
            z = ground(*piece.a)
            bases[piece.level] = min(bases.get(piece.level, z), z)
        if piece.kind == 'post':
            posts.setdefault(piece.owner, []).append(piece.a)
    out = []
    # Each fence section's ends as it stands, by owner: (its joint, its base there, 'a' its start or 'b' its end).
    ends = {}

    def step_post(kit, at, yaw, owner, before, after):
        """The kit's step post at a joint between steps whose bases are before and after it, if the step needs one."""
        if abs(after - before) <= kit['hide'] or (kit.get('step_down') and after > before):
            return
        stats['step posts'] += 1
        out.append((kit['step_post'], (at[0], at[1], ground(*at)), (0.0, 0.0, yaw), (1.0, 1.0, 1.0), owner))

    for piece in plan.pieces:
        kit = fence_kit(piece.mesh) if piece.b is not None and piece.kind == 'span' else None
        if kit is None:
            out.append((piece.mesh,) + pose(piece, ground, bases) + (piece.owner,))
            continue
        # A fence's section, or its gate unit (one level piece between its own heavy posts, never split).
        gate = piece.mesh == kit.get('gate')
        steps, past = fence_steps(piece.a, piece.b, ground, kit, split=not gate)
        if len(steps) > 1:
            stats['split'] += 1
        if past:
            drop = abs(ground(*piece.b) - ground(*piece.a))
            stats['past'].append((math.degrees(math.atan2(drop, math.dist(piece.a, piece.b))), piece.owner))
        length_ = PIECES[piece.mesh][0][0]
        for k, (a, b, z) in enumerate(steps):
            # Plumb and level, from its pivot (its own first post) at a, squashed or stretched to reach b.
            stretch = math.dist(a, b) / length_ if piece.stretch else 1.0 / len(steps)
            out.append((piece.mesh, (a[0], a[1], z), (0.0, 0.0, piece.yaw), (1.0, stretch, 1.0), piece.owner))
            if k:
                step_post(kit, a, piece.yaw, piece.owner, steps[k - 1][2], z)
        if not gate:
            mine = ends.setdefault(piece.owner, [])
            mine.append((piece.a, steps[0][2], 'a', kit, piece.yaw))
            mine.append((piece.b, steps[-1][2], 'b', kit, piece.yaw))
    # Where one section ends and the next starts, a step their own posts don't cover gets a post, unless one stands there
    # already (a corner's, a run's end).
    for owner, mine in ends.items():
        starts = [e for e in mine if e[2] == 'a']
        for at, z, side, kit, yaw in mine:
            if side != 'b':
                continue
            following = next((e for e in starts if math.dist(e[0], at) < 2.0), None)
            if following is None or any(math.dist(q, at) < 30.0 for q in posts.get(owner, ())):
                continue
            step_post(kit, at, following[4], owner, z, following[1])
    return out


def fence_kit(mesh):
    """The fence kit (one of KITS that steps) whose section, broken section or gate unit mesh is, or None."""
    return next((kit for kit in KITS.values() if kit.get('bury') is not None
                 and mesh in (kit['section'], kit.get('broken'), kit.get('gate'))), None)


def fence_steps(a, b, ground, kit, split=True):
    """A fence's section from a to b as it stands: [(from, to, its base's height)], level steps, plumb: one, or as many
    equal ones as it takes (down to the kit's shortest share of the section) for none to drop more than bury + hover;
    and whether even the shortest drop more."""
    most = max(1, int(round(1.0 / kit['shortest']))) if split else 1
    clean = kit['bury'] + kit['hover']
    for count in range(1, most + 1):
        spots = [(a[0] + (b[0] - a[0]) * k / count, a[1] + (b[1] - a[1]) * k / count) for k in range(count + 1)]
        heights = [ground(*p) for p in spots]
        drop = max(abs(z1 - z0) for z0, z1 in zip(heights, heights[1:]))
        if drop <= clean:
            break
    steps = []
    for (p, q), (z0, z1) in zip(zip(spots, spots[1:]), zip(heights, heights[1:])):
        middle = ground((p[0] + q[0]) * 0.5, (p[1] + q[1]) * 0.5)
        steps.append((p, q, step_base(z0, z1, middle, kit)))
    return steps, drop > clean


def step_base(z0, z1, middle, kit):
    """A level step's base between ground z0 and z1 at its ends (middle under its middle): halfway between them while
    its low end hovers no more than kit hover; up the slope from there, its high end sunk no more than kit bury (past
    that, it hovers more); sunk under a dip in its middle, at most SAG."""
    low, high = min(z0, z1), max(z0, z1)
    base = max(min((low + high) * 0.5, low + kit['hover']), high - kit['bury'])
    return base + max(min(middle - (z0 + z1) * 0.5, 0.0), -SAG)


def pose(piece, ground, bases):
    (length_, depth, _), pivot, _ = PIECES.get(piece.mesh, ((100.0, 100.0, 100.0), 'middle', SMALL_CULL))
    x, y = piece.a
    if piece.b is not None:
        # A span (a wall, a lying log): along its chord, rolled with the slope (the far end, its -Y, rises with a
        # positive roll), stretched to reach the next joint over the ground, and sunk under a dip in its middle.
        za, zb = roll_heights(piece.a, piece.b, ground)
        chord = math.dist(piece.a, piece.b)
        mx, my = (piece.a[0] + piece.b[0]) * 0.5, (piece.a[1] + piece.b[1]) * 0.5
        roll = math.degrees(math.atan2(zb - za, chord))
        stretch = math.hypot(chord, zb - za) / length_ if piece.stretch else 1.0
        if pivot == 'middle' or piece.kind == 'log':
            return (mx, my, (za + zb) * 0.5), (roll, 0.0, piece.yaw), (1.0, stretch, 1.0)
        return (x, y, za), (roll, 0.0, piece.yaw), (1.0, stretch, 1.0)
    if piece.kind == 'post':
        return (x, y, ground(x, y)), (0.0, 0.0, piece.yaw), (1.0, 1.0, 1.0)
    if piece.kind == 'corner':
        (vx, vy), (ex, ey) = piece.extra
        z = min(ground(x, y), ground(vx, vy), ground(ex, ey))
        return (x, y, z), (0.0, 0.0, piece.yaw), (1.0, 1.0, 1.0)
    if piece.level:
        return (x, y, bases[piece.level] + piece.lift), (0.0, 0.0, piece.yaw), (1.0, 1.0, 1.0)
    # A prop: tilted with the ground under its sides (at most MAX_TILT; a mound, MOUND_TILT), sunk so no side hovers;
    # an upright piece isn't tilted, and sinks to its lowest side (at most UPRIGHT_SINK).
    yaw = math.radians(piece.yaw)
    fx, fy = math.cos(yaw), math.sin(yaw)     # the actor's +X
    hx, hy = max(depth * 0.5, 10.0), max(length_ * 0.5, 10.0)
    g0 = ground(x, y)
    gf, gb = ground(x + fx * hx, y + fy * hx), ground(x - fx * hx, y - fy * hx)
    # The actor's +Y is the front turned right: (-fy, fx) seen as (X north, Y east) is +Y.
    gr, gl = ground(x - fy * hy, y + fx * hy), ground(x + fy * hy, y - fx * hy)
    upright = piece.kind in ('cairn', 'upright')
    tilt = MOUND_TILT if piece.kind == 'mound' else MAX_TILT
    pitch = 0.0 if upright else clamp(math.degrees(math.atan2(gf - gb, 2.0 * hx)), tilt)
    roll = 0.0 if upright else clamp(math.degrees(math.atan2(gl - gr, 2.0 * hy)), tilt)
    tp, tr = math.tan(math.radians(pitch)), math.tan(math.radians(roll))
    z = min(g0, gf - hx * tp, gb + hx * tp, gl - hy * tr, gr + hy * tr)
    z = max(z, g0 - (UPRIGHT_SINK if piece.kind == 'upright' else MAX_SINK))
    return (x, y, z + piece.lift), (roll + piece.lean[1], pitch + piece.lean[0], piece.yaw), (1.0, 1.0, 1.0)


def clamp(value, limit):
    return max(-limit, min(limit, value))


def roll_heights(a, b, ground):
    """A wall's section's (or a lying log's) ends' heights as it stands from a to b: the ground at each, both sunk by
    the dip under its middle (at most SAG), so its baseline at any point between is their blend."""
    za, zb = ground(*a), ground(*b)
    sag = max(min(ground((a[0] + b[0]) * 0.5, (a[1] + b[1]) * 0.5) - (za + zb) * 0.5, 0.0), -SAG)
    return za + sag, zb + sag


def span_heights(a, b, ground, kit=None):
    """A fence section's baseline heights at its two ends as the dressing stands it from a to b (fence_steps: its first
    and last level steps' bases; one level step, the same at both), so the rail's foot anywhere between is their blend.
    kit: its KITS entry; a rail fence's by default (build_area_whitlock stands Amos in one). A kit that doesn't step
    (a wall) follows the ground (roll_heights)."""
    kit = KITS['rail'] if kit is None else kit
    if kit.get('bury') is None:
        return roll_heights(a, b, ground)
    steps, _ = fence_steps(a, b, ground, kit)
    return steps[0][2], steps[-1][2]


def run_head(source, placements, oid, start='gate'):
    """The first section of obstacle oid's first run that starts so ('gate': just past a gate; 'open': at the line's
    start; 'loop'), as (its pivot, its far joint): its first post and where the next section starts. None without one.
    A builder stands someone in a span by it (build_area_whitlock: Amos past his gate)."""
    plan = plan_all(source, placements, only=[oid])
    return next(((a, b) for kind, a, b in plan.heads.get(oid, []) if kind == start), None)


# Fence and wall pieces: grass may grow up to them (a strip bared along every fence would read as mown), so the scatter
# keeps only its trees, bushes and rocks off their lines; everything else keeps every layer off.
LINE_KINDS = ('span', 'post', 'corner')


def footprints(source, placements=None):
    """Every piece's box on the ground, for the scatter to keep its layers out of: (x, y, yaw, half along the actor's Y,
    half along its X, whether it's a fence or wall line) in cm and degrees, the yaw the piece's own (its X its front, its
    Y along a fence)."""
    out = []
    for piece in plan_all(source, placements).pieces:
        (length_, depth, _), pivot, _ = PIECES.get(piece.mesh, ((100.0, 100.0, 100.0), 'middle', SMALL_CULL))
        if piece.kind == 'corner':
            (vx, vy), (ex, ey) = piece.extra
            for (ax, ay), (bx, by) in ((piece.a, (vx, vy)), ((vx, vy), (ex, ey))):
                out.append(((ax + bx) * 0.5, (ay + by) * 0.5, kit_yaw(bx - ax, by - ay),
                            math.dist((ax, ay), (bx, by)) * 0.5 + depth * 0.5, depth * 0.5, True))
            continue
        x, y = piece.a
        half = length_ * 0.5
        if piece.b is not None:
            half = math.dist(piece.a, piece.b) * 0.5 if piece.stretch else half
            if pivot == 'start' and piece.kind != 'log':
                d = unit(piece.a, piece.b)
                x, y = x + d[0] * half, y + d[1] * half
            else:
                x, y = (piece.a[0] + piece.b[0]) * 0.5, (piece.a[1] + piece.b[1]) * 0.5
        ahead = FOOTPRINT_AHEAD.get(piece.mesh, 0.0)
        x, y = x + math.cos(math.radians(piece.yaw)) * ahead, y + math.sin(math.radians(piece.yaw)) * ahead
        out.append((x, y, piece.yaw, half, depth * 0.5, piece.kind in LINE_KINDS))
    return out


# ---------------------------------------------------------------------------
# In the editor
# ---------------------------------------------------------------------------

class Ground:
    """The terrain's height (its tiles alone: never a volume, a tree, a building or the dressing) under a point."""

    def __init__(self, build):
        self.tiles = build_area.terrain_tiles(build.tag)
        self.cache = {}
        self.missed = 0
        if not self.tiles:
            build.warn('no terrain tiles tagged Ground (build the whole level first): the dressing stands at height 0')

    def __call__(self, x, y):
        key = (round(x), round(y))
        if key not in self.cache:
            hit = build_area.terrain_hit(self.tiles, unreal.Vector(x, y, 20000.0), unreal.Vector(x, y, -20000.0))
            if hit is None:
                self.missed += 1
            self.cache[key] = 0.0 if hit is None else hit.z
        return self.cache[key]


def trace_z(component, start, end):
    """The height where the line from start to end first meets a component (complex collision), or None."""
    hit = component.line_trace_component(start, end, True, False, False)
    if not hit:
        return None
    location = hit[0] if isinstance(hit, tuple) else hit.to_tuple()[5]
    return location.z


class Rocks:
    """Where the rock build_area.py placed (its cliff faces, outcrops, Den Rock: ROCK_MESHES, on actors with the area's
    build tag; the whole build and "dressing" both place the dressing after the cliffs and models) stands in a fence's
    way at (x, y): rock between ROCK_LOW and ROCK_HIGH over the terrain there. Two vertical traces against each rock
    whose bounds hold the point: down from over its top to ROCK_LOW finds a top over that band, and up from ROCK_LOW
    tells an overhang (its underside higher than ROCK_HIGH: clear under it) from the point standing inside the rock (no
    underside, or the same surface seen from inside)."""

    def __init__(self, build, ground):
        self.ground = ground
        self.parts = []
        for actor in unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors():
            if unreal.Name(build.tag) not in actor.tags:
                continue
            # Placed rocks only: an instanced component is the scatter's (kept off the fences' lines already).
            rock = [c for c in actor.get_components_by_class(unreal.StaticMeshComponent)
                    if not isinstance(c, unreal.InstancedStaticMeshComponent)
                    and c.static_mesh is not None and c.static_mesh.get_name().startswith(ROCK_MESHES)]
            if rock:
                origin, extent = actor.get_actor_bounds(False)
                self.parts.append((rock, origin.x - extent.x, origin.x + extent.x, origin.y - extent.y,
                                   origin.y + extent.y, origin.z + extent.z))

    def near(self, line, reach):
        """Whether any rock's bounds come within reach (cm) of a line's bounds."""
        xs, ys = [p[0] for p in line], [p[1] for p in line]
        x0, x1, y0, y1 = min(xs) - reach, max(xs) + reach, min(ys) - reach, max(ys) + reach
        return any(p[1] <= x1 and p[2] >= x0 and p[3] <= y1 and p[4] >= y0 for p in self.parts)

    def __call__(self, point):
        x, y = point
        candidates = [p for p in self.parts if p[1] <= x <= p[2] and p[3] <= y <= p[4]]
        if not candidates:
            return False
        z = self.ground(x, y)
        low, high = z + ROCK_LOW, z + ROCK_HIGH
        for components, _, _, _, _, top in candidates:
            if top < low:
                continue
            above, foot = unreal.Vector(x, y, top + 10.0), unreal.Vector(x, y, low)
            for component in components:
                down = trace_z(component, above, foot)
                if down is None:
                    continue
                up = trace_z(component, foot, above)
                if up is None or up <= high or abs(up - down) < 2.0:
                    return True
        return False


def place(build, meshes):
    """The dressing for build's layout: one AInstancedProps per mesh in the Dressing folder, and any ruin whose
    placement is gone; logs what each obstacle got, and what the slopes and rocks made of the fences."""
    ground = Ground(build)
    rocks = Rocks(build, ground)
    plan = plan_all(build.source, build.layout.get('placements', {}), rocks=rocks)
    for message in plan.warnings:
        build.warn(message)
    batches, absent = {}, {}
    stats = {}
    for mesh, location, rotation, scale, owner in poses(plan, ground, stats):
        if mesh not in meshes:
            absent.setdefault(mesh, set()).add(owner)
            continue
        roll, pitch, yaw = rotation
        batches.setdefault(mesh, []).append(unreal.Transform(
            location=unreal.Vector(*location), rotation=unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw),
            scale=unreal.Vector(*scale)))
    for mesh, owners in sorted(absent.items()):
        build.warn(f'no SM_{mesh} yet: left out of {", ".join(sorted(owners))}')
    cls = unreal.load_class(None, CLASSES + 'InstancedProps')
    if cls is None:
        build.warn('no InstancedProps class (build the game module first): no fences, walls, graves or yard props')
        batches = {}
    for mesh, transforms in sorted(batches.items()):
        actor = build.place(cls, (0.0, 0.0, 0.0), label=f'Dressing_{mesh}', folder=FOLDER, tags=(TAG,))
        actor.set_editor_property('cull_distance', float(PIECES.get(mesh, (None, None, SMALL_CULL))[2]))
        actor.set_instances(unreal.load_asset(meshes[mesh]), transforms)
    for model, owner, x, y, yaw in plan.ruins:
        if model not in meshes:
            build.warn(f'no SM_{model} yet: no ruin at {owner}')
            continue
        # Its own actor: its bounds are its own, so it's tagged Obstacle like build_area.py's placements.
        build.place(unreal.load_asset(meshes[model]), (x, y, ground(x, y)), yaw, label=f'{owner}_{model}',
                    folder=FOLDER, tags=(TAG, 'Obstacle'))
    for oid, note in plan.notes.items():
        build.log(f'dressing {oid}: {note}')
    if ground.missed:
        build.warn(f'{ground.missed} dressing traces found no terrain: those pieces stand at height 0')
    build.log(f'dressing: fences plumb on their slopes: {stats["split"]} sections split into shorter steps, '
              f'{stats["step posts"]} step posts; {len(rocks.parts)} rocks looked at for the runs to stop short of')
    if stats['past']:
        steepest = {}
        for degrees, owner in stats['past']:
            steepest[owner] = max(steepest.get(owner, 0.0), degrees)
        build.warn('sections steeper than their kit steps cleanly (they hover more than its hover): '
                   + ', '.join(f'{owner} up to {degrees:.0f} degrees' for owner, degrees in sorted(steepest.items())))
    build.log(f'dressing: {sum(len(t) for t in batches.values())} instances of {len(batches)} meshes, '
              f'{len(plan.ruins)} ruins')
