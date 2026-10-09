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
  hill's and the family plot's each over its sunken mound; the churchyard's east rows re-laid across the knoll's flank,
  each board turned down its slope as far as it still faces the valley (GRAVE_ROWS' 'downhill'); the family plot's
  eight old graves and the churchyard's short rows on the knoll's south face (seen from boot hill and the town), which
  have no obstacle, come from EXTRA_GRAVE_ROWS, and each fresh mound the placements put down (the respawn graves) gets
  its fresh headboard (MOUND_BOARDS).
- Cairns stand along their path; the ruins and the keeper's three cairns are layout placements, which build_area.py's
  models() places already (a ruin whose placement is gone is placed here instead, as its own actor).
- Props become small groups laid out in the obstacle's own frame: along its longest side, facing its road or a
  building, and pieces set against a building's wall in its own frame (the backlots' lean-tos). A few pieces stand at
  spots of their own (SPOTS: Whitlock Fields' round bales, two more fallen pines by the Sink and north roads, the
  churchyard's dead trees).
- A town (the layout's level.town names a file beside it, Art/Levels/<Area>/town.json, made by that folder's
  make_town.py: the tutorial island's Crossroads Town) adds its own pieces, lines and webs, read as they are:
  - pieces: a mesh at a spot and yaw, scaled, lifted (a cocoon's hanging point over the ground), sunk and tilted (the
    bogged cart), floating on a pond's water (lily pads: layout_computed.json's ponds), standing plumb, as a prop
    tilted a little with the ground, or lying flat on it (TOWN_KINDS: trees, lamps, posts, stacks and plants plumb);
    a dead tree with "crown" wears Web_Crown with its own transform, as the Sink's Webwood does;
  - lines: picket fences, rail fences and stone walls by the kits above (closed round a garden or open), with their
    gates and their broken share;
  - webs: a strand of silk (Web_Line) between two points at heights over the ground, stretched and pitched to the span
    from the line's real 4 m and tied into a dead tree's leaning trunk where it starts or ends at one, with threads
    (Web_Strands) hanging from its middle where it's high enough; and a sheet (Web_Ground cards round a middle one,
    scaled to its radius).
  Its noTrees zones and a crown's clearance round each of its trees are no_tree_boxes(), which build_area.py turns into
  the boxes the scatter's trees keep out of.
Every piece stands on the terrain's tiles (traced like build_area_whitlock.py's Ground: the tiles alone, never a volume,
a tree or a building). A fence's sections stand plumb and level, stepped down a slope (fence_steps: split into shorter
steps where it's steep, a post at each step its own posts can't cover); a wall's follow the slope along their length
(the roll); both sink where the ground dips under their middle. Props tilt with the ground by at most MAX_TILT degrees
(mounds MOUND_TILT) and sink so no corner hovers; stacks, sheds and trees (UPRIGHT) stand level.

Performance (the doc's "Performance plan": about 700 draws at the heaviest view): every kit piece is instanced, one
AInstancedProps per mesh (World/InstancedProps.h: solid like a placed mesh, shadowed, each instance culled on its own
past the mesh's distance in PIECES; Medium's view distance scale draws them to 60% of it). A mesh some of whose pieces
the player walks through (PASSABLE: reeds, bushes, webs, cocoons; a town piece's "solid" overrides it, a hedge's bushes
are solid) gets a second actor without collision, and cards (SHADOWLESS: webs, reeds) cast no shadow. A ruin is its own
actor.

The scatter (build_island_scatter.py) keeps its layers out of the bounds of every actor tagged Obstacle, and an
instanced actor's bounds span the valley: AInstancedProps tags itself Obstacle only in play (for the minimap and the
creatures), and footprints() gives the scatter every piece's own box from the same plan instead.

Run plan_all()/footprints()/no_tree_boxes() anywhere (plain Python and the layout's JSON); place() needs the editor.
"""
import copy
import json
import math
import os
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
BUSH_CULL = 15000.0    # bushes (a hedge is a wall of them): about 90 m on Medium
REED_CULL = 7000.0     # reeds and flowers: about 42 m on Medium (the scatter's reeds go at 30)
PAD_CULL = 6000.0      # lily pads, flat on the water: about 36 m on Medium
WEB_CULL = 8000.0      # silk threads and mats: about 48 m on Medium, past which a thread is less than a pixel

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
    # A town's (town.json), sized from the imported meshes (Saved/MeshBounds.json, 2026-10-08). The trees (Trees.py,
    # Pines.py), like the dead tree, by their trunk's foot with its root flare (the scatter's grass grows round it; its
    # crown is in TREE_CROWNS), their height the mesh's; never culled.
    'Oak_A': ((100, 100, 960), 'middle', TREE_CULL),
    'Oak_B': ((120, 120, 1100), 'middle', TREE_CULL),
    'Oak_C': ((190, 190, 1327), 'middle', TREE_CULL),
    'Birch_A': ((40, 40, 1095), 'middle', TREE_CULL),
    'Birch_B': ((60, 40, 1103), 'middle', TREE_CULL),     # two stems from one root, side by side along its Y
    'Pine_A': ((65, 65, 1175), 'middle', TREE_CULL),
    'Pine_B': ((75, 75, 1375), 'middle', TREE_CULL),
    'Apple_A': ((40, 40, 512), 'middle', TREE_CULL),
    # Plants (Undergrowth.py, Pond.py, GroundCover.py: no collision of their own).
    'Bush_A': ((185, 172, 132), 'middle', BUSH_CULL),
    'Bush_B': ((214, 196, 123), 'middle', BUSH_CULL),
    'Bush_C': ((193, 190, 208), 'middle', BUSH_CULL),
    'Reeds_A': ((163, 119, 154), 'middle', REED_CULL),
    'Reeds_B': ((79, 78, 138), 'middle', REED_CULL),
    'LilyPads_A': ((138, 93, 7), 'middle', PAD_CULL),
    'Flowers_Yellow': ((58, 68, 43), 'middle', REED_CULL),
    'Flowers_White': ((62, 74, 48), 'middle', REED_CULL),
    'Flowers_Purple': ((58, 70, 51), 'middle', REED_CULL),
    # The village's props (VillageProps.py, Main Street's kit). A lamp post's, lantern post's and signpost's box is its
    # post (the arm and boards are overhead); they read down a road as far as a fence does.
    'LampPost': ((25, 25, 275), 'middle', FENCE_CULL),
    'LanternPost': ((36, 36, 230), 'middle', FENCE_CULL),
    'Signpost': ((25, 25, 240), 'middle', FENCE_CULL),
    'Bench': ((171, 46, 47), 'middle', SMALL_CULL),
    'LaundryLine': ((388, 70, 210), 'middle', FENCE_CULL),
    'NoticeBoard': ((220, 99, 268), 'middle', WALL_CULL),
    'TownMemorial': ((130, 104, 178), 'middle', WALL_CULL),
    'Wheelbarrow': ((72, 183, 74), 'middle', SMALL_CULL),
    # Rocks (Rocks.py, Outcrops.py); an outcrop is a landmark, never culled.
    'Rock_A': ((94, 82, 47), 'middle', SMALL_CULL),
    'Rock_B': ((68, 65, 34), 'middle', SMALL_CULL),
    'Rock_C': ((65, 73, 28), 'middle', SMALL_CULL),
    'Rock_D': ((38, 28, 16), 'middle', SMALL_CULL),
    'Boulder_A': ((201, 255, 133), 'middle', WALL_CULL),
    'Boulder_B': ((159, 160, 111), 'middle', WALL_CULL),
    'Boulder_C': ((273, 233, 155), 'middle', WALL_CULL),
    'Outcrop_TorA': ((935, 703, 454), 'middle', TREE_CULL),
    'Outcrop_TorB': ((615, 595, 985), 'middle', TREE_CULL),
    'Outcrop_TorC': ((766, 509, 710), 'middle', TREE_CULL),
    'Outcrop_TorD': ((656, 389, 461), 'middle', TREE_CULL),
    # Web Hollow's (Sink.py, DenDressing.py). Cocoon_Hung hangs 2.3 m under its pivot, the hanging point; the threads
    # and the strands' edge run along the actor's Y (Web_Line from its pivot to Y = -400 cm, sagging 11 cm). Web_Crown
    # takes its dead tree's transform, so it stands by the tree's trunk box.
    'Cocoon_Hung': ((49, 46, 236), 'middle', SMALL_CULL),
    'Cocoon_Lying': ((223, 91, 24), 'middle', SMALL_CULL),
    'EggSac_A': ((151, 149, 218), 'middle', SMALL_CULL),
    'EggSac_B': ((162, 125, 189), 'middle', SMALL_CULL),
    'EggSac_C': ((151, 116, 239), 'middle', SMALL_CULL),
    'EggSac_Burst': ((168, 260, 72), 'middle', SMALL_CULL),
    'Web_Line': ((400, 2, 12), 'start', WEB_CULL),
    'Web_Strands': ((122, 16, 225), 'middle', WEB_CULL),
    'Web_Tatters': ((133, 12, 239), 'middle', WEB_CULL),
    'Web_Ground': ((260, 130, 4), 'middle', WEB_CULL),
    'Web_Crown': ((80, 80, 650), 'middle', WALL_CULL),
}
# Where a piece's footprint's middle is, ahead of its pivot along its front (cm), where it isn't the pivot: the
# woodshed's chopping block stands in front of its posts, the lumber stack's planks lean out of its front, the lean-to's
# pivot is a little behind its middle (its back is on the wall). A town's pieces: the memorial's plinth steps, the burst
# sac's trailing strands, the tors' masses off their pivots (MeshBounds' middles); FOOTPRINT_RIGHT likewise to its right.
FOOTPRINT_AHEAD = {'Woodshed': 21.0, 'LumberStack': 28.0, 'LeanTo': 6.0,
                   'TownMemorial': 14.0, 'EggSac_Burst': 32.0, 'Bush_B': 14.0,
                   'Outcrop_TorA': 15.0, 'Outcrop_TorB': 48.0, 'Outcrop_TorC': -19.0}
FOOTPRINT_RIGHT = {'EggSac_Burst': -16.0, 'Outcrop_TorA': 33.0, 'Outcrop_TorC': 65.0, 'Outcrop_TorD': 66.0}

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
# Each row: its style and the yaw its boards' faces look (0 north, 90 east), or 'downhill' (each board as far down the
# slope under it as it can face and still face the valley: downhill_yaw); and, where the dressing re-lays the layout's
# row, the rows it stands in instead (paths). The churchyard's west rows face the lane west of the nave, where the player
# walks; boot hill's south, down toward the town, as its respawn mound faces.
# The churchyard's east rows (the art session, 2026-10-07: "turn the east rows to face south, down the knoll ... boards
# facing downhill read better than edge-on rows") stood in three lines down the knoll's east flank facing the nave, so
# from boot hill, the north road and the town they were seen edge-on. The flank there falls east to north-east (the
# area model's heights: 105 degrees by the respawn grave, 30-50 at the north fence), away from everywhere the
# churchyard is seen from, so each board faces down it only as far as it still faces the valley: within DOWNHILL_WITHIN
# of its bearing to boot hill (GRAVE_TOWARD), about south-east, down the knoll toward boot hill, the Sink road and the
# quarry flat. Re-laid in the same ground across that facing (rows 40 degrees east of north, 2.3 m apart, the boards
# 1.7 m apart), so they stand side by side seen from below rather than one behind another: 25 places as before, kept
# 2.2 m and more from the fight's spots, off the lane to the vestry door, 1.5 m and more inside the fence and clear of
# the south face's rows and dead tree.
GRAVE_ROWS = {
    'churchyardGravesWest1': ('churchyard', 90.0), 'churchyardGravesWest2': ('churchyard', 90.0),
    'churchyardGravesWest3': ('churchyard', 90.0),
    'churchyardGravesEast1': ('churchyard', 'downhill', [[[7326, -943], [7586, -725]], [[6918, -985], [7569, -439]]]),
    'churchyardGravesEast2': ('churchyard', 'downhill', [[[6509, -1028], [7291, -372]]]),
    'churchyardGravesEast3': ('churchyard', 'downhill', [[[6231, -961], [6882, -415]], [[6344, -566], [6604, -348]]]),
    'bootHillGraves1': ('bootHill', 180.0), 'bootHillGraves2': ('bootHill', 180.0),
    'bootHillGraves3': ('bootHill', 180.0),
}
# A 'downhill' board's valley: the placement it turns toward (boot hill's respawn grave), how far (degrees) from its
# bearing to it a board may turn to face down its slope, and how far either side of the board (cm) the slope is read.
GRAVE_TOWARD = 'graveBootHill'
DOWNHILL_WITHIN = 45.0
DOWNHILL_PROBE = 75.0

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
# EXTRA_GRAVE_ROWS and SPOTS have no obstacle to say which layout they belong to (the tables keyed by obstacle id apply
# only where the obstacle is): they're Ransom's Rest's. Every layout once got them, so Skyreach's dressing build put
# Ransom's Rest's family plot graves, churchyard face rows, dead trees, bales and fallen pines at those spots on the
# island (by the range, among them).
EXTRAS_AREA = 'RansomsRest'
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

# A town (the layout's level.town: town.json in the layout's folder under LEVELS; layout_computed.json beside it gives
# the ponds' water).
LEVELS = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', '..', 'Art', 'Levels'))
TREES = ('Oak_A', 'Oak_B', 'Oak_C', 'Birch_A', 'Birch_B', 'Pine_A', 'Pine_B', 'Apple_A', 'DeadTree_A')
# How a town piece stands when it doesn't say ("kind": "prop", "upright" or "flat"): plumb (trees, plants, posts, lamps,
# stacks, sheds, rocks sunk to their lowest side, sacs, what hangs), flat on the ground (tilted with it up to FLAT_TILT:
# lily pads on the water, web mats, the lying cocoon, the burst sac), or else a prop tilted a little (MAX_TILT).
TOWN_KINDS = {
    'upright': TREES + ('LampPost', 'LanternPost', 'Signpost', 'NoticeBoard', 'TownMemorial', 'FirewoodStack',
                        'Woodshed', 'LeanTo', 'LumberStack', 'LaundryLine', 'HitchRail', 'Outhouse', 'Cocoon_Hung',
                        'EggSac_A', 'EggSac_B', 'EggSac_C', 'Web_Crown', 'Web_Strands', 'Web_Tatters', 'Bush_',
                        'Reeds_', 'Flowers_', 'Rock_', 'Boulder_', 'Outcrop_', 'Cairn_'),
    'flat': ('LilyPads_', 'Web_Ground', 'Web_Corner', 'Cocoon_Lying', 'EggSac_Burst'),
}
FLAT_TILT = 15.0
# Lily pads float this far (cm) over their pond's water (the scatter's do too).
WATER_LIFT = 1.0
# What the player and the creatures go through (no collision, never an Obstacle: AInstancedProps' bSolid off), by the
# start of the mesh's name; a town piece's "solid" overrides it (a hedge's bushes are solid). Egg sacs and cocoons are
# shootable story pieces in Ransom's Rest's Sink; here they're dressing.
PASSABLE = ('Bush_', 'Reeds_', 'LilyPads_', 'Flowers_', 'Web_', 'Cocoon_', 'EggSac_')
# Cards that cast no shadow (the art's rule for the webs; the scatter's reeds and pads cast none either).
SHADOWLESS = ('Web_', 'Reeds_', 'LilyPads_', 'Flowers_')
# A town piece's footprint lets the scatter's grass and flowers grow up to it, as a fence's does (only the woody and
# rocky layers keep off it): plants, and the laundry's line between its posts.
GRASS_UNDER = ('Bush_', 'Reeds_', 'Flowers_', 'LaundryLine')
# Town pieces with no footprint: up in the air (threads, a crown's webs, what hangs) or on the water.
NO_FOOTPRINT = ('Web_Line', 'Web_Strands', 'Web_Tatters', 'Web_Crown', 'Cocoon_Hung', 'LilyPads_')

# Each tree's crown in its own frame (cm: X from, X to, Y from, Y to; MeshBounds), and how far (cm) past it the
# scatter's trees keep their trunks (no_tree_boxes), so a scattered crown can touch a placed one but never grow into it.
TREE_CROWNS = {
    'Oak_A': (-490.6, 497.6, -647.2, 534.4), 'Oak_B': (-443.3, 467.4, -635.3, 523.4),
    'Oak_C': (-663.5, 774.9, -733.3, 736.0), 'Birch_A': (-294.3, 275.8, -293.0, 250.8),
    'Birch_B': (-211.1, 258.5, -319.3, 313.1), 'Pine_A': (-359.6, 367.6, -353.8, 348.9),
    'Pine_B': (-364.1, 336.1, -371.1, 349.8), 'Apple_A': (-268.5, 263.5, -242.8, 258.8),
    'DeadTree_A': (-198.8, 133.9, -292.8, 175.4),
}
CROWN_CLEAR = 250.0
# A town's noTrees circle is kept as a cross of two boxes (its width and NO_TREE_CROSS of it, both ways), which covers
# it and reaches 22% past it at the diagonals where its square would reach 41%; a polygon as its smallest box, or, where
# that box is more than NO_TREE_FILL empty, as strips across it about NO_TREE_STRIP (cm) wide.
NO_TREE_CROSS = 0.71
NO_TREE_FILL = 0.8
NO_TREE_STRIP = 600.0

# Web strands. A strand tied at a tree (an end within TIE_SNAP cm of a town tree's foot) is tied to its trunk's line at
# that height: the dead tree's trunk leans off its foot (DeadTree.py's Bezier, without its gnarl: cm off the foot in its
# own frame, X and Y, by height). Each end reaches TIE_IN past its tie, into the bark. Threads hang from a strand's middle
# (Web_Strands, scaled down to clear the ground by STRANDS_CLEAR) where they'd still be STRANDS_SHORTEST of their length.
TIE_SNAP = 80.0
TIE_IN = 15.0
TRUNK_LINES = {'DeadTree_A': ((0.0, 0.0, -1.0), (100.0, 0.0, -9.0), (200.0, -1.0, -23.0), (300.0, -4.0, -37.0),
                              (400.0, -10.0, -45.0), (500.0, -20.0, -35.0))}
WEB_LINE_SAG = 11.0
STRANDS_CLEAR = 40.0
STRANDS_SHORTEST = 0.55
# A sheet web: a Web_Ground card in its middle (scaled to the radius over SHEET_MIDDLE, within SHEET_SCALE) and a ring of
# them pointing out from it at SHEET_RING of the radius, one every SHEET_GAP cm round (3 to 6), the ring's cards
# SHEET_STEP cm over the middle one and its neighbours a step apart (one, two, one, two, and three for an odd one out),
# so no two overlapping mats fight in one plane. A sheet smaller than SHEET_ALONE is its middle card only.
SHEET_MIDDLE = 220.0
SHEET_SCALE = (0.9, 1.8)
SHEET_RING = 0.55
SHEET_GAP = 230.0
SHEET_STEP = 1.0
SHEET_ALONE = 150.0
# How far (cm) a hanging piece reaches under its pivot (its hanging point): one whose bottom would come within
# HANG_CLEAR of the ground at its lift (or under it, without one) is warned of.
HANGS = {'Cocoon_Hung': 226.0, 'Web_Strands': 225.0, 'Web_Tatters': 239.0}
HANG_CLEAR = 60.0


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
    ground; lean: (pitch, roll) added (degrees). A town's piece (town: town_pose() stands it) also has its scale
    (x, y, z), how far it sinks (cm), whether it's solid (None: by its mesh, PASSABLE) and the water it floats on (z, cm)
    or None."""

    def __init__(self, mesh, owner, a, yaw=0.0, b=None, stretch=False, kind='prop', level=None, lift=0.0,
                 lean=(0.0, 0.0), extra=None, scale=(1.0, 1.0, 1.0), sink=0.0, solid=None, water=None, town=False):
        self.mesh, self.owner, self.a, self.b, self.yaw = mesh, owner, a, b, yaw
        self.stretch, self.kind, self.level, self.lift, self.lean, self.extra = stretch, kind, level, lift, lean, extra
        self.scale, self.sink, self.solid, self.water, self.town = scale, sink, solid, water, town
        if b is not None:
            self.yaw = kit_yaw(b[0] - a[0], b[1] - a[1])


def solid_of(piece):
    """Whether a piece blocks: its own "solid", or by its mesh (PASSABLE ones don't)."""
    return piece.solid if piece.solid is not None else not piece.mesh.startswith(PASSABLE)


# ---------------------------------------------------------------------------
# The plan (plain Python: the layout's JSON in, pieces out)
# ---------------------------------------------------------------------------

class Plan:
    """What the dressing places for a layout (layout.json as source, layout_computed.json's placements): the instanced
    pieces, the ruins to place as actors ((model, owner, x, y, yaw)), per obstacle a line of what it got, and per fence
    or wall its runs' first sections (heads: [(how the run starts: 'open', 'gate', 'rock' or 'loop', its first joint,
    its second)], so a builder can stand someone at a span, as build_area_whitlock stands Amos at the one past his gate).
    rocks: where rock stands in a fence's way ((x, y) -> bool; place()'s Rocks, from the cliffs and rocks placed), so a
    run stops short of it; without it (plain Python, footprints(), run_head()) the runs go on as the layout draws them.
    A layout with a town (level.town) gets its pieces, lines and webs last (town()), unless only some obstacles are
    asked for."""

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
        if only:
            return
        if source.get('name', EXTRAS_AREA) == EXTRAS_AREA:
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
        self.town()

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
            style, face = GRAVE_ROWS[oid][:2]
            # Re-laid rows stand in the layout's row's place (its obstacle keeps its id and its ground).
            paths = GRAVE_ROWS[oid][2] if len(GRAVE_ROWS[oid]) > 2 else [entry['path']]
            text = '; '.join(self.graves(oid, {'path': path}, style, face) for path in paths)
            if len(GRAVE_ROWS[oid]) > 2:
                text = f're-laid in {len(paths)} row(s) facing {face}: {text}'
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
        """A row of graves along entry's path, their boards facing face (a yaw), or 'downhill': each its bearing to the
        valley (GRAVE_TOWARD) here, turned down its slope when it stands on the ground (poses(): downhill_yaw); a
        mound style faces its rows by a yaw, as its mounds are laid in the plan."""
        style = GRAVE_STYLES[style_name]
        path = [tuple(p) for p in entry['path']]
        total = length(path)
        count = max(1, round(total / (spacing or style['spacing']))) + 1
        names = [m for m, _ in style['mix']]
        weights = [w for _, w in style['mix']]
        toward = self.point_ref(GRAVE_TOWARD) if face == 'downhill' else None
        graves = [s['location'][:2] for s in self.placed.values() if str(s.get('kind', '')).startswith('Grave_')]
        left = cleared = 0
        for k in range(count):
            rnd = random.Random(f'{oid} {path[0]} {k}')
            if rnd.random() < style['missing']:
                left += 1
                continue
            mesh = rnd.choices(names, weights)[0]
            x, y = point_at(path, total * k / max(count - 1, 1) + rnd.uniform(-style['along'], style['along']))
            facing = math.degrees(math.atan2(toward[1] - y, toward[0] - x)) if toward else face
            across = (math.cos(math.radians(facing)), math.sin(math.radians(facing)))
            off = rnd.uniform(-style['across'], style['across'])
            lean = (rnd.uniform(-style['lean'], style['lean']), rnd.uniform(-style['lean'], style['lean']))
            at = (x + across[0] * off, y + across[1] * off)
            turn = rnd.uniform(-style['turn'], style['turn'])
            yaw = facing + turn
            # Its mound (when the style has one) out in front of the board, where the board faces.
            mound = (at[0] + math.cos(math.radians(yaw)) * MOUND_OUT, at[1] + math.sin(math.radians(yaw)) * MOUND_OUT)
            spots = (at, mound) if style.get('mound') else (at,)
            # Never on a road, nor crowding a story grave (a respawn mound and its board).
            if any(self.on_road(p) or any(math.dist(p, g) < GRAVE_CLEAR for g in graves) for p in spots):
                cleared += 1
                continue
            self.pieces.append(Piece(mesh, oid, at, yaw, lean=lean, kind='board',
                                     extra=('downhill', toward, turn) if toward else None))
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

    # --- A town (town.json) ---

    def town(self):
        """The town the layout's level.town names: its pieces, then its lines and webs (a strand ties into the trees
        among the pieces), each group's counted in notes. A town file that's named but missing or broken is a warning
        and no town."""
        town, problem = load_town(self.source)
        if problem:
            self.warnings.append(problem)
        if not town:
            return
        water = pond_levels(self.source)
        groups = {}

        def counted(group, pieces):
            for piece in pieces:
                groups.setdefault(group, {}).setdefault(piece.mesh, 0)
                groups[group][piece.mesh] += 1

        crowned, by_id, hung = [], {}, []
        for entry in town.get('pieces', []):
            group = entry.get('group', 'town')
            piece = town_piece(entry, f'town:{group}', water, self.warnings)
            if piece is None:
                continue
            self.pieces.append(piece)
            counted(group, [piece])
            by_id[entry.get('id')] = piece
            if entry.get('crown'):
                crowned.append((piece, group))
            if entry.get('hangFrom'):
                hung.append((piece, entry['hangFrom']))
        # A cocoon hung from a dead tree's limb measures its lift from that tree's foot, where the tree stands (sunk to
        # its lowest side), so its top meets the limb on a slope too (town_pose()).
        for piece, host in hung:
            if host in by_id:
                piece.extra = dict(piece.extra or {}, host=by_id[host])
            else:
                self.warnings.append(f'town: {piece.mesh} at {piece.a} hangs from {host}, which is not placed: its '
                                     f'lift is measured from the ground under it')
        # A dead tree with "crown" wears Web_Crown with its own transform (fitted to DeadTree_A's limbs), unless the
        # town places one there itself.
        crowns = [p.a for p in self.pieces if p.town and p.mesh == 'Web_Crown']
        for tree, group in crowned:
            if tree.mesh != 'DeadTree_A':
                self.warnings.append(f'town: Web_Crown is fitted to DeadTree_A, not {tree.mesh}: no crown at {tree.a}')
            elif not any(math.dist(tree.a, at) < 10.0 for at in crowns):
                crown = copy.copy(tree)
                crown.mesh, crown.kind, crown.solid = 'Web_Crown', 'upright', None
                self.pieces.append(crown)
                counted(group, [crown])
        trees = [p for p in self.pieces if p.town and p.mesh in TREES and not p.lift and p.water is None]
        for entry in town.get('lines', []):
            self.town_line(entry)
        for k, web in enumerate(town.get('webs', [])):
            group = web.get('group', 'webs')
            name = web.get('id', f'web{k + 1}')
            made = sheet_cards(web, f'town:{group}', name) if web.get('sheet') else \
                self.strand(web, f'town:{group}', name, trees)
            self.pieces += made
            counted(group, made)
        for group, counts in sorted(groups.items()):
            self.notes[f'town {group}'] = (f'{sum(counts.values())} pieces ('
                                           + ', '.join(f'{n} {m}' for m, n in sorted(counts.items())) + ')')
        deferred = town.get('deferred') or {}
        if deferred:
            self.notes['town deferred'] = 'no model yet, left for round 2: ' + ', '.join(
                f'{n} {kind}' for kind, n in sorted(deferred.items()))

    def town_line(self, entry):
        """A town's fence or wall by its kit (KITS: rail, picket, stone), round its points (closed) or along them, with
        its gates and broken share, laid as the layout's obstacles are (line())."""
        oid = f"town:{entry.get('id', 'line')}"
        points = [tuple(p[:2]) for p in entry.get('points', [])]
        closed = bool(entry.get('closed'))
        if closed and len(points) > 3 and math.dist(points[0], points[-1]) < 1.0:
            # Closed round its first point again: line() closes a polygon itself.
            points = points[:-1]
        if entry.get('kit') not in KITS or len(points) < (3 if closed else 2):
            self.warnings.append(f'{oid}: kit {entry.get("kit")} with {len(points)} points: left out')
            return
        spec = {'kit': entry['kit'], 'gates': [list(g[:3]) for g in entry.get('gates', [])]}
        for key in ('broken', 'missing', 'fallen', 'inside', 'corners', 'halves', 'gate_width', 'first_after_gate'):
            if key in entry:
                spec[key] = entry[key]
        before = len(self.pieces)
        text = self.line(oid, {'polygon': points} if closed else {'path': points}, spec)
        counts = {}
        for piece in self.pieces[before:]:
            counts[piece.mesh] = counts.get(piece.mesh, 0) + 1
        self.notes[oid] = (f'{len(self.pieces) - before} pieces ('
                           + ', '.join(f'{n} {m}' for m, n in sorted(counts.items())) + f'); {text}')

    def strand(self, web, owner, name, trees):
        """A strand of silk between web's from and to ([X, Y, cm over the ground]): one Web_Line, each end tied where it
        meets a tree (tie()). The threads hanging from its middle are poses()' (they need the ground)."""
        try:
            (ax, ay, up_a), (bx, by, up_b) = (tuple(web['from'][:3]), tuple(web['to'][:3]))
        except (KeyError, TypeError, ValueError):
            self.warnings.append(f'town web {name}: no from and to [X, Y, up]: left out')
            return []
        a, foot_a = tie((ax, ay), float(up_a), trees)
        b, foot_b = tie((bx, by), float(up_b), trees)
        if math.dist(a, b) < 50.0:
            self.warnings.append(f'town web {name}: its ends are {math.dist(a, b):.0f} cm apart: left out')
            return []
        return [Piece('Web_Line', owner, a, b=b, stretch=True, kind='strand', town=True,
                      extra={'up': (float(up_a), float(up_b)), 'feet': (foot_a, foot_b), 'id': name})]


def town_path(source):
    """The town file the layout's level.town names (in its own folder under LEVELS), or None."""
    name = source.get('level', {}).get('town')
    return os.path.join(LEVELS, source.get('name', ''), name) if name else None


def load_town(source):
    """The layout's town (town.json, as read) and what's wrong with it, if anything: (None, None) without one."""
    path = town_path(source)
    if path is None:
        return None, None
    if not os.path.isfile(path):
        return None, f"level.town names {path}, which isn't there (make_town.py writes it): no town"
    try:
        with open(path) as f:
            return json.load(f), None
    except ValueError as error:
        return None, f'{path} is not JSON ({error}): no town'


def pond_levels(source):
    """Each pond's water height (cm) by its id, from layout_computed.json beside the layout (its ponds; one written
    before ponds had ids gives its one pond as 'pond')."""
    try:
        with open(os.path.join(LEVELS, source.get('name', ''), 'layout_computed.json')) as f:
            data = json.load(f)
    except (OSError, ValueError):
        return {}
    levels = {key: pond['waterZ'] for key, pond in (data.get('ponds') or {}).items() if 'waterZ' in pond}
    if 'pond' not in levels and (data.get('pond') or {}).get('waterZ') is not None:
        levels['pond'] = data['pond']['waterZ']
    return levels


def town_kind(mesh):
    """How a town piece stands when it doesn't say (TOWN_KINDS): 'flat', 'upright' or 'prop'."""
    for kind in ('flat', 'upright'):
        if mesh.startswith(TOWN_KINDS[kind]):
            return kind
    return 'prop'


def town_piece(entry, owner, water, warnings):
    """A town.json piece as a Piece: its scale (one number, or X, Y, Z), lift, sink, tilt ([pitch, roll], or a roll
    alone), solidity, kind (or its mesh's: town_kind()) and, on a pond, that pond's water. None (with a warning) when it
    can't stand."""
    mesh, at, label = entry.get('mesh'), entry.get('at'), entry.get('id', '?')
    if not mesh or not at or len(at) < 2:
        warnings.append(f'town piece {label}: no mesh or spot: left out')
        return None
    scale = entry.get('scale', 1.0)
    scale = (float(scale),) * 3 if isinstance(scale, (int, float)) else tuple(float(s) for s in scale[:3])
    if len(scale) != 3:
        warnings.append(f'town piece {label}: scale {entry.get("scale")} is neither one number nor three: unscaled')
        scale = (1.0, 1.0, 1.0)
    tilt = entry.get('tilt') or (0.0, 0.0)
    tilt = (0.0, float(tilt)) if isinstance(tilt, (int, float)) else (float(tilt[0]), float(tilt[1]))
    kind = entry.get('kind') or town_kind(mesh)
    if kind not in ('prop', 'upright', 'flat'):
        warnings.append(f'town piece {label}: no kind {kind}: it stands as {town_kind(mesh)}')
        kind = town_kind(mesh)
    z = None
    if entry.get('onWater'):
        z = water.get(entry['onWater'])
        if z is None:
            warnings.append(f"town piece {label}: no pond {entry['onWater']} in layout_computed.json (regenerate "
                            f"it): left out")
            return None
    if mesh not in PIECES:
        warnings.append(f'town piece {label}: no size for {mesh} in PIECES: it stands, and keeps the scatter off, as a '
                        f'1 m box')
    lift = float(entry.get('lift', 0.0))
    if mesh in HANGS and lift - HANGS[mesh] * scale[2] < HANG_CLEAR:
        warnings.append(f'town piece {label}: {mesh} hangs {HANGS[mesh] * scale[2]:.0f} cm under its pivot, so at lift '
                        f'{lift:.0f} its bottom is {lift - HANGS[mesh] * scale[2]:.0f} cm over the ground')
    return Piece(mesh, owner, (float(at[0]), float(at[1])), float(entry.get('yaw', 0.0)), kind=kind, lift=lift,
                 lean=tilt, scale=scale, sink=float(entry.get('sink', 0.0)), solid=entry.get('solid'), water=z,
                 town=True)


def trunk_line(mesh, height):
    """Where a tree's trunk is (cm off its foot, X and Y in its own frame) at height cm up it (TRUNK_LINES; a straight
    trunk is over its foot)."""
    table = TRUNK_LINES.get(mesh)
    if not table:
        return 0.0, 0.0
    if height <= table[0][0]:
        return table[0][1:]
    for (h0, x0, y0), (h1, x1, y1) in zip(table, table[1:]):
        if height <= h1:
            t = (height - h0) / (h1 - h0)
            return x0 + (x1 - x0) * t, y0 + (y1 - y0) * t
    return table[-1][1:]


def tie(end, up, trees):
    """Where a strand's end at (x, y), up cm over the ground, is tied, and the foot its height is measured from: on the
    trunk's line of the town tree standing within TIE_SNAP of it (its foot's ground), or where it is."""
    near = min(trees, key=lambda t: math.dist(t.a, end), default=None)
    if near is None or math.dist(near.a, end) > TIE_SNAP:
        return end, end
    sx, sy, sz = near.scale
    lx, ly = trunk_line(near.mesh, up / max(sz, 0.1))
    lx, ly = lx * sx, ly * sy
    yaw = math.radians(near.yaw)
    return (near.a[0] + math.cos(yaw) * lx - math.sin(yaw) * ly,
            near.a[1] + math.sin(yaw) * lx + math.cos(yaw) * ly), near.a


def sheet_cards(web, owner, name):
    """A sheet web at web's at ([X, Y]), r cm across its middle: a Web_Ground card in the middle scaled to it and a ring
    of them pointing out from it, each its own seeded turn and size, lying flat on the ground."""
    try:
        x, y = float(web['at'][0]), float(web['at'][1])
    except (KeyError, TypeError, IndexError, ValueError):
        return []
    r = float(web.get('r', SHEET_MIDDLE))
    rnd = random.Random(f'{name} sheet')
    middle = max(SHEET_SCALE[0], min(SHEET_SCALE[1], r / SHEET_MIDDLE))
    cards = [Piece('Web_Ground', owner, (x, y), rnd.uniform(0.0, 180.0), kind='flat', scale=(middle,) * 3,
                   town=True, extra={'rise': 0.0})]
    if r < SHEET_ALONE:
        return cards
    ring = r * SHEET_RING
    count = max(3, min(6, round(2.0 * math.pi * ring / SHEET_GAP)))
    turn = rnd.uniform(0.0, 360.0)
    for k in range(count):
        bearing = turn + 360.0 * k / count + rnd.uniform(-12.0, 12.0)
        at = (x + math.cos(math.radians(bearing)) * ring, y + math.sin(math.radians(bearing)) * ring)
        size = max(0.8, min(1.3, r / 300.0 * rnd.uniform(0.9, 1.1)))
        step = 3 if count % 2 and k == count - 1 else 1 + k % 2
        # Its length (the card's Y) points out from the middle.
        cards.append(Piece('Web_Ground', owner, at, (bearing - 90.0 + rnd.uniform(-15.0, 15.0)) % 360.0, kind='flat',
                           scale=(size,) * 3, town=True, extra={'rise': SHEET_STEP * step}))
    return cards


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
    """Each piece's (mesh, (x, y, z), (roll, pitch, yaw), (sx, sy, sz), owner, solid), on the ground ground(x, y) gives:
    a fence's section as its level steps (fence_steps), and its kit's step post wherever two steps meet further apart
    than its own posts cover; a town's piece as town_poses() stands it. stats (a dict) counts what the slopes made of
    the fences: 'split' sections, 'step posts', and 'past': (degrees, owner) of each section steeper than its kit's
    shortest steps take cleanly."""
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
        out.append((kit['step_post'], (at[0], at[1], ground(*at)), (0.0, 0.0, yaw), (1.0, 1.0, 1.0), owner,
                    not kit['step_post'].startswith(PASSABLE)))

    for piece in plan.pieces:
        if piece.town:
            out += town_poses(piece, ground)
            continue
        if piece.kind == 'board' and piece.extra and piece.extra[0] == 'downhill':
            # A board facing down its slope (graves()' 'downhill'): its yaw from the ground it stands on.
            _, toward, turn = piece.extra
            piece = copy.copy(piece)
            piece.yaw = downhill_yaw(piece.a, ground, toward) + turn
        kit = fence_kit(piece.mesh) if piece.b is not None and piece.kind == 'span' else None
        if kit is None:
            out.append((piece.mesh,) + pose(piece, ground, bases) + (piece.owner, solid_of(piece)))
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
            out.append((piece.mesh, (a[0], a[1], z), (0.0, 0.0, piece.yaw), (1.0, stretch, 1.0), piece.owner,
                        solid_of(piece)))
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


def downhill_yaw(at, ground, toward):
    """The yaw a board at at faces down the slope under it (read DOWNHILL_PROBE either side), turned back to within
    DOWNHILL_WITHIN of its bearing to toward where the ground falls further away from that: as far down the slope as it
    can face and still face the valley. On flat ground (under about 2 degrees), toward it."""
    x, y = at
    bearing = math.degrees(math.atan2(toward[1] - y, toward[0] - x))
    gx = (ground(x + DOWNHILL_PROBE, y) - ground(x - DOWNHILL_PROBE, y)) / (2.0 * DOWNHILL_PROBE)
    gy = (ground(x, y + DOWNHILL_PROBE) - ground(x, y - DOWNHILL_PROBE)) / (2.0 * DOWNHILL_PROBE)
    if math.hypot(gx, gy) < 0.035:
        return bearing
    fall = math.degrees(math.atan2(-gy, -gx))
    return bearing + clamp((fall - bearing + 180.0) % 360.0 - 180.0, DOWNHILL_WITHIN)


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


def town_poses(piece, ground):
    """A town piece's poses (poses()' tuples): a strand's line and the threads under it (strand_poses()), or the piece
    as town_pose() stands it."""
    if piece.kind == 'strand':
        return strand_poses(piece, ground)
    return [(piece.mesh,) + town_pose(piece, ground) + (piece.owner, solid_of(piece))]


def town_pose(piece, ground):
    """A town piece's (x, y, z), (roll, pitch, yaw), scale: on its pond's water, level (lily pads); lifted its lift over
    the ground under it, plumb (a hanging cocoon: its pivot is the hanging point); plumb and sunk to the lowest ground
    under its sides (an upright piece: UPRIGHT_SINK at most); or tilted with the ground (a prop by at most MAX_TILT, a
    flat piece by FLAT_TILT) and sunk so no side hovers (MAX_SINK at most). Then sunk its own sink, its tilt added, and a
    sheet's card raised its rise. Its sides are read at its scaled size."""
    (length_, depth, _), _, _ = PIECES.get(piece.mesh, ((100.0, 100.0, 100.0), 'middle', SMALL_CULL))
    x, y = piece.a
    tilt_pitch, tilt_roll = piece.lean
    rise = piece.extra.get('rise', 0.0) if isinstance(piece.extra, dict) else 0.0
    if piece.water is not None:
        return (x, y, piece.water + WATER_LIFT - piece.sink), (tilt_roll, tilt_pitch, piece.yaw), piece.scale
    if piece.lift > 0.0:
        host = piece.extra.get('host') if isinstance(piece.extra, dict) else None
        base = town_pose(host, ground)[0][2] if host is not None else ground(x, y)
        return (x, y, base + piece.lift - piece.sink), (tilt_roll, tilt_pitch, piece.yaw), piece.scale
    yaw = math.radians(piece.yaw)
    fx, fy = math.cos(yaw), math.sin(yaw)     # the actor's +X
    hx, hy = max(depth * piece.scale[0] * 0.5, 10.0), max(length_ * piece.scale[1] * 0.5, 10.0)
    g0 = ground(x, y)
    gf, gb = ground(x + fx * hx, y + fy * hx), ground(x - fx * hx, y - fy * hx)
    gr, gl = ground(x - fy * hy, y + fx * hy), ground(x + fy * hy, y - fx * hy)
    if piece.kind == 'upright':
        pitch = roll = 0.0
        z = max(min(g0, gf, gb, gr, gl), g0 - UPRIGHT_SINK)
    else:
        tilt = FLAT_TILT if piece.kind == 'flat' else MAX_TILT
        pitch = clamp(math.degrees(math.atan2(gf - gb, 2.0 * hx)), tilt)
        roll = clamp(math.degrees(math.atan2(gl - gr, 2.0 * hy)), tilt)
        tp, tr = math.tan(math.radians(pitch)), math.tan(math.radians(roll))
        z = max(min(g0, gf - hx * tp, gb + hx * tp, gl - hy * tr, gr + hy * tr), g0 - MAX_SINK)
    return (x, y, z - piece.sink + rise), (roll + tilt_roll, pitch + tilt_pitch, piece.yaw), piece.scale


def strand_poses(piece, ground):
    """A strand of silk (a 'strand' piece from Plan.strand()): Web_Line from its tie at a to its tie at b, each end its
    up over its foot's ground and reaching TIE_IN on into the bark; its pivot at a, running along its -Y, rolled to
    climb or fall to b (a positive roll lifts its -Y end), stretched (Y and Z, so its sag keeps its shape) to the span
    from the line's real length. Under its middle, where the threads clear the ground by STRANDS_CLEAR at
    STRANDS_SHORTEST of their length or more, Web_Strands along it, shortened (Z) to clear it."""
    up_a, up_b = piece.extra['up']
    foot_a, foot_b = piece.extra['feet']
    za, zb = ground(*foot_a) + up_a, ground(*foot_b) + up_b
    chord = math.dist(piece.a, piece.b)
    way = unit(piece.a, piece.b)
    climb = (zb - za) / chord
    a = (piece.a[0] - way[0] * TIE_IN, piece.a[1] - way[1] * TIE_IN, za - climb * TIE_IN)
    reach = chord + 2.0 * TIE_IN
    rise = climb * reach
    stretch = math.hypot(reach, rise) / PIECES['Web_Line'][0][0]
    roll = math.degrees(math.atan2(rise, reach))
    out = [('Web_Line', a, (roll, 0.0, piece.yaw), (1.0, stretch, stretch), piece.owner, solid_of(piece))]
    mx, my = (piece.a[0] + piece.b[0]) * 0.5, (piece.a[1] + piece.b[1]) * 0.5
    mz = (za + zb) * 0.5 - WEB_LINE_SAG * stretch
    hang = PIECES['Web_Strands'][0][2]
    share = min(1.0, (mz - ground(mx, my) - STRANDS_CLEAR) / hang)
    if share >= STRANDS_SHORTEST:
        turn = random.Random(f"{piece.extra.get('id')} strands").uniform(-6.0, 6.0)
        out.append(('Web_Strands', (mx, my, mz), (0.0, 0.0, piece.yaw + turn), (1.0, 1.0, share), piece.owner,
                    solid_of(piece)))
    return out


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


def footprints(source, placements=None, meshes=None):
    """Every piece's box on the ground, for the scatter to keep its layers out of: (x, y, yaw, half along the actor's Y,
    half along its X, whether grass may grow up to it: a fence or wall line, a town's plants) in cm and degrees, the yaw
    the piece's own (its X its front, its Y along a fence). A town's pieces in the air or on the water have none
    (NO_FOOTPRINT), and with meshes (names, as build_area.mesh_index()'s) a town piece whose mesh isn't there has none
    (place() leaves it out)."""
    out = []
    for piece in plan_all(source, placements).pieces:
        if piece.town:
            box = town_footprint(piece) if meshes is None or piece.mesh in meshes else None
            if box:
                out.append(box)
            continue
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


def town_footprint(piece):
    """A town piece's box on the ground (footprints()' tuple) at its scaled size, its middle where its mesh's is
    (FOOTPRINT_AHEAD, FOOTPRINT_RIGHT); None for a strand, a piece on the water or lifted off the ground, or one in the
    air (NO_FOOTPRINT). A tree's is its trunk's foot, so the grass grows round it."""
    if piece.kind == 'strand' or piece.water is not None or piece.lift > 0.0 or piece.mesh.startswith(NO_FOOTPRINT):
        return None
    (length_, depth, _), _, _ = PIECES.get(piece.mesh, ((100.0, 100.0, 100.0), 'middle', SMALL_CULL))
    sx, sy, _ = piece.scale
    ahead, right = FOOTPRINT_AHEAD.get(piece.mesh, 0.0) * sx, FOOTPRINT_RIGHT.get(piece.mesh, 0.0) * sy
    yaw = math.radians(piece.yaw)
    x = piece.a[0] + math.cos(yaw) * ahead - math.sin(yaw) * right
    y = piece.a[1] + math.sin(yaw) * ahead + math.cos(yaw) * right
    return x, y, piece.yaw, length_ * sy * 0.5, depth * sx * 0.5, piece.mesh.startswith(GRASS_UNDER)


def no_tree_boxes(source):
    """Boxes the scatter's tree layers keep out of, for the layout's town: (x, y, yaw, half along the box's X, half along
    its Y, a label) in cm and degrees. Its noTrees zones ({"at": [X, Y], "r": R}: a cross of two boxes, NO_TREE_CROSS;
    {"polygon": [...]}: its smallest box, or strips across it where that's mostly empty), and each of its trees' crowns
    (TREE_CROWNS at the tree's scale and yaw) and CROWN_CLEAR round it, unless a zone's box holds it already.
    build_area.py places a NoTrees box for each. None without a town."""
    town, _ = load_town(source)
    if not town:
        return []
    boxes = []
    for k, zone in enumerate(town.get('noTrees', [])):
        label = zone.get('id', f'Town_{k + 1}')
        if zone.get('polygon'):
            boxes += polygon_boxes([tuple(p[:2]) for p in zone['polygon']], label)
        elif zone.get('at') and zone.get('r'):
            (x, y), r = zone['at'][:2], float(zone['r'])
            boxes += [(x, y, 0.0, r, r * NO_TREE_CROSS, f'{label}_a'), (x, y, 0.0, r * NO_TREE_CROSS, r, f'{label}_b')]
    zones = list(boxes)
    for entry in town.get('pieces', []):
        crown = TREE_CROWNS.get(entry.get('mesh'))
        if crown is None or entry.get('lift') or entry.get('onWater') or not entry.get('at'):
            continue
        scale = entry.get('scale', 1.0)
        sx, sy = (scale, scale) if isinstance(scale, (int, float)) else scale[:2]
        x0, x1, y0, y1 = crown
        cx, cy = (x0 + x1) * 0.5 * sx, (y0 + y1) * 0.5 * sy
        yaw = float(entry.get('yaw', 0.0))
        c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
        x, y = entry['at'][0] + c * cx - s * cy, entry['at'][1] + s * cx + c * cy
        box = (x, y, yaw, (x1 - x0) * 0.5 * sx + CROWN_CLEAR, (y1 - y0) * 0.5 * sy + CROWN_CLEAR,
               f"Crown_{entry.get('id', len(boxes))}")
        if not any(all(in_box(corner, zone) for corner in box_corners(box)) for zone in zones):
            boxes.append(box)
    return boxes


def box_corners(box):
    """A (x, y, yaw, half along X, half along Y, ...) box's four corners."""
    x, y, yaw, hx, hy = box[:5]
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    return [(x + c * u - s * v, y + s * u + c * v) for u, v in ((hx, hy), (hx, -hy), (-hx, -hy), (-hx, hy))]


def in_box(point, box):
    """Whether a point is inside a (x, y, yaw, half along X, half along Y, ...) box."""
    x, y, yaw, hx, hy = box[:5]
    c, s = math.cos(math.radians(yaw)), math.sin(math.radians(yaw))
    dx, dy = point[0] - x, point[1] - y
    return abs(c * dx + s * dy) <= hx + 1e-6 and abs(-s * dx + c * dy) <= hy + 1e-6


def convex_hull(points):
    """The convex hull of points, counter-clockwise (Andrew's monotone chain)."""
    pts = sorted(set(points))
    if len(pts) < 3:
        return pts

    def cross(o, a, b):
        return (a[0] - o[0]) * (b[1] - o[1]) - (a[1] - o[1]) * (b[0] - o[0])
    lower, upper = [], []
    for p in pts:
        while len(lower) >= 2 and cross(lower[-2], lower[-1], p) <= 0.0:
            lower.pop()
        lower.append(p)
    for p in reversed(pts):
        while len(upper) >= 2 and cross(upper[-2], upper[-1], p) <= 0.0:
            upper.pop()
        upper.append(p)
    return lower[:-1] + upper[:-1]


def clip_strip(points, u0, u1):
    """A polygon given in (u, v) cut to the strip u0 <= u <= u1 (Sutherland-Hodgman: kept on the inside of one edge,
    then of the other)."""
    for edge, side in ((u0, 1.0), (u1, -1.0)):
        out = []
        for a, b in zip(points, points[1:] + points[:1]):
            a_in, b_in = (a[0] - edge) * side >= 0.0, (b[0] - edge) * side >= 0.0
            if a_in != b_in:
                out.append((edge, a[1] + (b[1] - a[1]) * (edge - a[0]) / (b[0] - a[0])))
            if b_in:
                out.append(b)
        points = out
        if not points:
            break
    return points


def polygon_boxes(points, label):
    """A noTrees polygon as boxes (no_tree_boxes()' tuples): its smallest box (along one of its hull's sides), or where
    that box is more than NO_TREE_FILL empty, strips across its long side about NO_TREE_STRIP wide, each as wide as the
    polygon is there, so a triangle or an L doesn't take the ground beside it."""
    hull = convex_hull(points)
    if len(hull) < 3:
        return []
    best = None
    for a, b in zip(hull, hull[1:] + hull[:1]):
        u = unit(a, b)
        us = [p[0] * u[0] + p[1] * u[1] for p in hull]
        vs = [-p[0] * u[1] + p[1] * u[0] for p in hull]
        area = (max(us) - min(us)) * (max(vs) - min(vs))
        if best is None or area < best[0]:
            best = (area, u, min(us), max(us), min(vs), max(vs))
    area, u, u0, u1, v0, v1 = best
    if u1 - u0 < v1 - v0:
        # Strips go across the long side: turn the frame a quarter so u runs along it.
        u = (-u[1], u[0])
        u0, u1, v0, v1 = v0, v1, -u1, -u0
    yaw = math.degrees(math.atan2(u[1], u[0]))

    def box(a0, a1, b0, b1, name):
        mu, mv = (a0 + a1) * 0.5, (b0 + b1) * 0.5
        return (u[0] * mu - u[1] * mv, u[1] * mu + u[0] * mv, yaw, (a1 - a0) * 0.5, (b1 - b0) * 0.5, name)
    if abs(signed_area(points)) >= NO_TREE_FILL * area:
        return [box(u0, u1, v0, v1, label)]
    local = [(p[0] * u[0] + p[1] * u[1], -p[0] * u[1] + p[1] * u[0]) for p in points]
    count = max(2, min(8, int(round((u1 - u0) / NO_TREE_STRIP))))
    out = []
    for k in range(count):
        cut_ = clip_strip(local, u0 + (u1 - u0) * k / count, u0 + (u1 - u0) * (k + 1) / count)
        if len(cut_) >= 3:
            us, vs = [p[0] for p in cut_], [p[1] for p in cut_]
            out.append(box(min(us), max(us), min(vs), max(vs), f'{label}_{k + 1}'))
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


def prop_flags(build, actor, mesh, solid, town):
    """An AInstancedProps' settings past its cull distance, set before its instances: passable (bSolid off) for pieces
    the player goes through, no shadow for cards (SHADOWLESS), and a town's trees tagged Tree on their component, so the
    minimap draws their crowns as it does the scatter's."""
    try:
        if not solid:
            actor.set_editor_property('solid', False)
        if mesh.startswith(SHADOWLESS):
            actor.set_editor_property('cast_shadows', False)
    except Exception as error:
        build.warn(f'Dressing_{mesh}: {error} (build the game module first): it stays solid and shadowed')
    if town and mesh in TREES:
        actor.get_editor_property('instances').set_editor_property('component_tags', [unreal.Name('Tree')])


def place(build, meshes):
    """The dressing for build's layout: one AInstancedProps per mesh (two where some of its pieces are passable and
    some solid) in the Dressing folder, and any ruin whose placement is gone; logs what each obstacle and the town's
    groups got, and what the slopes and rocks made of the fences."""
    ground = Ground(build)
    rocks = Rocks(build, ground)
    plan = plan_all(build.source, build.layout.get('placements', {}), rocks=rocks)
    for message in plan.warnings:
        build.warn(message)
    batches, absent, towns = {}, {}, set()
    stats = {}
    for mesh, location, rotation, scale, owner, solid in poses(plan, ground, stats):
        if mesh not in meshes:
            absent.setdefault(mesh, set()).add(owner)
            continue
        roll, pitch, yaw = rotation
        batches.setdefault((mesh, solid), []).append(unreal.Transform(
            location=unreal.Vector(*location), rotation=unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw),
            scale=unreal.Vector(*scale)))
        if owner.startswith('town:'):
            towns.add((mesh, solid))
    for mesh, owners in sorted(absent.items()):
        build.warn(f'no SM_{mesh} yet: left out of {", ".join(sorted(owners))}')
    cls = unreal.load_class(None, CLASSES + 'InstancedProps')
    if cls is None:
        build.warn('no InstancedProps class (build the game module first): no fences, walls, graves or yard props')
        batches = {}
    for (mesh, solid), transforms in sorted(batches.items()):
        label = f'Dressing_{mesh}' if solid else f'Dressing_{mesh}_Passable'
        actor = build.place(cls, (0.0, 0.0, 0.0), label=label, folder=FOLDER, tags=(TAG,))
        actor.set_editor_property('cull_distance', float(PIECES.get(mesh, (None, None, SMALL_CULL))[2]))
        prop_flags(build, actor, mesh, solid, (mesh, solid) in towns)
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
    build.log(f'dressing: {sum(len(t) for t in batches.values())} instances of {len(set(m for m, _ in batches))} '
              f'meshes in {len(batches)} actors, {len(plan.ruins)} ruins')
