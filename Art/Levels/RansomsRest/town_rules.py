"""Ransom's Rest's town dressing by rule (Docs/Work/2026-11-04-Towns/PLAN.md 4.3-4.4, the DRESSING lane, step 4): the
building types, their zones and menus, the wall-foot weeds, the street's events and plank crossings, the boardwalk and
lamp variants, and the checks' extra spots, for make_town.py (beside this file), which keeps its placer, its checks and
its determinism. Standard-library Python like make_town.py; make_town.py imports it and calls dress() after its hand-made
vignettes (the Hasketts', the Mercers', Pruitt's...), which stay as fixed entries the rules fill round.

How a building is dressed: its placement key (TYPES) or else its model (MODEL_TYPES) gives its type; its frame comes from
make_town.KIT (FRAMES adds the dogtrot and the L house; a new model adds its frame there and its type here, nothing
else). Zones in the building's own frame (X its front, cm): front (0-3 m out: the walk or the porch), side (its side walls,
alleys; a 1.5 m lane kept clear past what stands there), back (0-12 m behind it), wall (hung on its faces). MENUS lists
each type's items in order; each item names a zone, a placing rule (SPOTS below) and a mesh, or a choice of meshes picked
by crc32(key, zone, slot) as orchard_swap does. Yaw jitter +-8 degrees (stacks +-15), the same crc. A piece that won't
fit where its rule puts it tries the rule's other spots, then make_town's nudges, then is dropped and printed.
street() lays the street's events and crossings, extras() the hand-placed pieces the menus can't know (the shut hotel's
porch, the alleys' junk, the new homes' yards), and SKIP takes menu items off a building whose front is full already.

What it may place: CHOSEN, the town's new mesh types (24 at most, the plan's cap: each AInstancedProps costs 1-3 draws a
material slot, plus its shadow). The rest of the PROPS lane's 48 are measured in KIT for later steps (the feed store's
and smithy's fronts, the farms' yards) and refused until chosen. Existing kit types (Barrel_B, Washtub, FirewoodStack...)
cost no new draws and are preferred where they tell the same story. Never Crate_A, Crate_B or Barrel_A (build_area_loot's
BREAKABLE: the lootable count must not move).

Every rule piece keeps 3 m off the encounters' spawn points and patrol routes (SPAWN_CLEAR_RULES), the street's events
keep the middle 3.5 m lane clear (LANE_HALF), nothing tall and wide stands in a roost's 9 m find circle, nothing on a
loudspeaker post's spot (rule_spots(): build_area_landfall.TOWN_SPEAKERS, build_area_main3's checkpoint, the layout's
encounters, read from those files so they stay in step).
"""
import ast
import math
import os
import zlib

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
UNREAL_TOOLS = os.path.join(REPO, 'Tools', 'Unreal')

# ---------------------------------------------------------------------------
# The new kit (the PROPS lane, 2026-11-04: Docs/Work/2026-11-04-Towns/PROPS.md, Saved/ArtPreviews/TownProps/*.report.json),
# in Unreal cm in the model's own frame (X its front = the Blender model's -Y, Y = the Blender model's -X): its ground box
# (x0, x1, y0, y1), height, solid (it has hulls), how it stands (prop: tilts a little; upright: plumb; flat: lies on the
# ground; wall: hung on a building's face, lifted), material slots, the props it shows (the density's weight: a vignette
# is several), cull (build_area_dressing's constant) and whether it casts a shadow.
# Wall pieces' pivot is on the wall's face (they stand toward +X); the rest the ground under the middle.
# ---------------------------------------------------------------------------

KIT = {
    # TownClutter.py
    'CrateStack_A': ((-45.0, 39.0, -51.0, 51.0), 149.0, True, 'upright', 2, 3, 'SMALL_CULL', True),
    'CrateStack_B': ((-45.0, 46.0, -100.0, 101.0), 104.0, True, 'upright', 1, 4, 'SMALL_CULL', True),
    'Crate_C': ((-28.0, 28.0, -83.0, 83.0), 48.0, True, 'prop', 2, 1, 'CLUTTER_CULL', False),
    'BarrelStack_A': ((-47.0, 49.0, -100.0, 100.0), 135.0, True, 'upright', 1, 5, 'SMALL_CULL', True),
    'Keg_Trestle': ((-39.0, 39.0, -70.0, 70.0), 82.0, True, 'upright', 1, 3, 'CLUTTER_CULL', True),
    'ToolBarrel': ((-42.0, 63.0, -85.0, 33.0), 170.0, True, 'upright', 1, 2, 'CLUTTER_CULL', True),
    'SackPile_B': ((-55.0, 72.0, -88.0, 58.0), 28.0, True, 'prop', 2, 4, 'CLUTTER_CULL', False),
    'LumberPile_Small': ((-49.0, 71.0, -94.0, 96.0), 34.0, True, 'prop', 2, 1, 'CLUTTER_CULL', False),
    'Sawhorse_Log': ((-38.0, 38.0, -102.0, 74.0), 126.0, True, 'upright', 2, 2, 'CLUTTER_CULL', True),
    'BarrelFire': ((-30.0, 33.0, -30.0, 30.0), 101.0, True, 'upright', 2, 1, 'CLUTTER_CULL', True),
    'ToolsLeaning_A': ((0.0, 50.0, -43.0, 44.0), 148.0, True, 'upright', 1, 3, 'CLUTTER_CULL', True),
    'Ladder_A': ((0.0, 39.0, -26.0, 29.0), 360.0, True, 'upright', 1, 1, 'SMALL_CULL', True),
    'Handbills_A': ((0.0, 4.0, -56.0, 55.0), 77.0, False, 'wall', 1, 1, 'CLUTTER_CULL', False),
    'Handbills_B': ((0.0, 5.0, -51.0, 55.0), 67.0, False, 'wall', 1, 1, 'CLUTTER_CULL', False),
    'SignBoard_Rooms': ((-44.0, 44.0, -2.0, 2.0), 48.0, False, 'air', 1, 1, 'FENCE_CULL', True),
    'SignBoard_Smith': ((-44.0, 44.0, -2.0, 2.0), 48.0, False, 'air', 1, 1, 'FENCE_CULL', True),
    'SignBoard_Feed': ((-44.0, 44.0, -2.0, 2.0), 48.0, False, 'air', 1, 1, 'FENCE_CULL', True),
    'SignBoard_Baths': ((-44.0, 44.0, -2.0, 2.0), 48.0, False, 'air', 1, 1, 'FENCE_CULL', True),
    'WagonFreight': ((-261.0, 261.0, -96.0, 96.0), 201.0, True, 'prop', 1, 1, 'SMALL_CULL', True),
    'WagonFreight_Canvas': ((-265.0, 265.0, -96.0, 96.0), 262.0, True, 'prop', 2, 1, 'SMALL_CULL', True),
    'WagonFreight_Broken': ((-305.0, 305.0, -96.0, 96.0), 201.0, True, 'prop', 1, 1, 'SMALL_CULL', True),
    # One water cart in the town, by the shack at the gate end: drawn to 30 m on Medium so it stays out of the Farm view
    # (56-72 m off, over budget), which SMALL_CULL's 72 m would reach.
    'WaterBarrel_Cart': ((-111.0, 111.0, -74.0, 74.0), 146.0, True, 'prop', 1, 1, 'CLUTTER_CULL', True),
    'Vignette_ShopFront_A': ((0.0, 126.0, -102.0, 125.0), 122.0, True, 'upright', 2, 6, 'SMALL_CULL', True),
    'Vignette_ShopFront_B': ((0.0, 128.0, -139.0, 128.0), 92.0, True, 'upright', 2, 5, 'SMALL_CULL', True),
    'Vignette_PorchLived_A': ((0.0, 115.0, -113.0, 93.0), 122.0, True, 'upright', 3, 4, 'SMALL_CULL', True),
    'Vignette_PorchEmpty_A': ((0.0, 111.0, -135.0, 142.0), 137.0, True, 'upright', 3, 5, 'SMALL_CULL', True),
    'Vignette_Alley_A': ((2.0, 131.0, -101.0, 124.0), 181.0, True, 'upright', 3, 5, 'SMALL_CULL', True),
    'Vignette_BackYard_A': ((-70.0, 117.0, -101.0, 89.0), 110.0, True, 'upright', 3, 3, 'SMALL_CULL', True),
    # TownGround.py (no collision). The weeds' boxes start at the wall's face (WeedFringe_B's leaves reach 16 cm back
    # into it, which the checks would read as standing in the wall).
    'WeedFringe_A': ((0.0, 57.0, -106.0, 103.0), 48.0, False, 'flat', 1, 0, 'WEED_CULL', False),
    'WeedFringe_B': ((0.0, 62.0, -106.0, 107.0), 80.0, False, 'flat', 1, 0, 'WEED_CULL', False),
    'WeedFringe_C': ((-28.0, 32.0, -31.0, 26.0), 50.0, False, 'flat', 1, 0, 'WEED_CULL', False),
    'PlankCrossing_A': ((-131.5, 131.5, -73.5, 73.5), 6.0, False, 'flat', 2, 0, 'CLUTTER_CULL', False),
    'PlankCrossing_B': ((-162.5, 162.5, -64.5, 64.5), 8.0, False, 'flat', 2, 0, 'CLUTTER_CULL', False),
    'StrawLitter_A': ((-66.0, 66.0, -84.5, 84.5), 4.0, False, 'flat', 1, 0, 'CLUTTER_CULL', False),
    'FallenBoards_A': ((-49.5, 49.5, -102.5, 102.5), 13.0, False, 'flat', 1, 1, 'CLUTTER_CULL', False),
    # FarmYard.py (round 2: the farms; not in the town).
    'Plough': ((-118.0, 118.0, -44.0, 44.0), 108.0, True, 'prop', 2, 1, 'SMALL_CULL', True),
    'DragHarrow': ((-80.0, 104.0, -82.0, 82.0), 29.0, True, 'flat', 1, 1, 'CLUTTER_CULL', False),
    'HayRake': ((-109.0, 109.0, -142.0, 142.0), 130.0, True, 'prop', 1, 1, 'SMALL_CULL', True),
    'FeedTrough': ((-32.0, 61.0, -121.0, 121.0), 52.0, True, 'prop', 2, 1, 'CLUTTER_CULL', False),
    'ButterChurn': ((-20.0, 20.0, -20.0, 20.0), 107.0, True, 'prop', 1, 1, 'CLUTTER_CULL', True),
    'WashKettle': ((-45.0, 65.0, -61.0, 53.0), 135.0, True, 'upright', 3, 1, 'CLUTTER_CULL', True),
    'Grindstone': ((-30.0, 30.0, -50.0, 74.0), 100.0, True, 'prop', 2, 1, 'CLUTTER_CULL', True),
    # The variants (Boardwalk.py, VillageProps.py, Containers.py): they chain and stand like their originals. LampPost_B
    # hangs on a building's outside corner (the building fills its -X -Y quarter; the arm leaves at 45 degrees, its
    # lantern 0.78 m out), ~3.0 m up; LampPost_C stands on its post's foot, its lanterns 0.7 m out along its Y.
    'LampPost_B': ((0.0, 70.0, 0.0, 70.0), 90.0, True, 'wall', 2, 0, 'SMALL_CULL', False),   # a bracket: no shadow
    'LampPost_C': ((-11.0, 11.0, -11.0, 11.0), 322.0, True, 'upright', 2, 0, 'SMALL_CULL', True),
    'Barrel_C': ((-33.0, 33.0, -33.0, 45.0), 100.0, True, 'upright', 2, 1, 'CLUTTER_CULL', True),
}
# The walks' variants take the original's frame and bounds (make_town's walk code treats them as their original).
WALK_VARIANTS = {'Boardwalk_4m_Worn': 'Boardwalk_4m', 'Boardwalk_4m_Patched': 'Boardwalk_4m',
                 'Boardwalk_Steps_Worn': 'Boardwalk_Steps'}
WALK_SLOTS = {'Boardwalk_4m_Worn': 3, 'Boardwalk_4m_Patched': 2, 'Boardwalk_Steps_Worn': 2}

# The town's new types (the plan's cap: 24). Chosen for what the player sees most and what no existing piece can show:
# the wall feet's weeds, the shut buildings' junk, notices on the walls, stacks a store would have, the street's events,
# the walks' and lamps' variety. Left out (measured above, for later steps): Handbills_B (one set reads as notices; a
# second costs a draw for a mirror image), the shop-front vignettes (no open shop has a free front today: the feed store
# and smithy, step 6), PorchLived_A and BackYard_A (the rocker, washtub, firewood and block are kit pieces already: no new
# draws, and homeB stands in the Farm view), the single crates, kegs, sacks and tools (the stacks and the alley junk
# stand for them), Barrel_C and Boardwalk_Steps_Worn (every walk run has two steps only), the plain and broken wagons and
# PlankCrossing_B (one of each kind in the street), BarrelFire (build_area_main3's fire, ATownFire: its own lane), the
# signs (the new buildings), the farm set (step 6; the Farm view is over its line).
CHOSEN = ('WeedFringe_A', 'WeedFringe_B', 'WeedFringe_C', 'PlankCrossing_A', 'StrawLitter_A', 'FallenBoards_A',
          'Handbills_A', 'CrateStack_A', 'CrateStack_B', 'BarrelStack_A', 'WaterBarrel_Cart', 'WagonFreight_Canvas',
          'Vignette_Alley_A', 'ToolsLeaning_A', 'Ladder_A', 'Boardwalk_4m_Patched', 'LampPost_B', 'LampPost_C')
# Also left out for the draws (2026-11-04, the first estimate: +29-36 draws in the street views before these cuts):
# Boardwalk_4m_Worn (three slots: WoodHoney; Patched is two and reads stronger) and Vignette_PorchEmpty_A (three slots
# and a shadow for the one shut porch with room for it, the hotel's: a tipped chair, a crate of empties and fallen boards
# from the kit tell it there).
TYPE_CAP = 24
# The first payback if a town view measures over its line (make_town.py --skip with these): the pieces with the least
# read per draw, cut before anything the street's fronts stand on (each saves its slots and its shadow in every view it's
# drawn in; the report prints the views' draws without them).
LEAN = ('LampPost_C', 'LampPost_B', 'Ladder_A', 'CrateStack_B', 'WeedFringe_C', 'Boardwalk_4m_Patched')

# Existing types this dressing adds instances of (no new draws where the type is drawn in that view already).
REUSED = ('Barrel_B', 'Washtub', 'BroomBucket', 'RockingChair', 'PorchChair', 'FirewoodStack', 'ChoppingBlock',
          'LaundryLine', 'MilkCans', 'Wheelbarrow', 'HayBale_Square')
BREAKABLE = ('Crate_A', 'Crate_B', 'Barrel_A')

# Build_area_dressing's culls (cm; Medium draws to 60% of it) and the weeds' own (24 m on Medium).
CULLS = {'CLUTTER_CULL': 5000.0, 'SMALL_CULL': 12000.0, 'FENCE_CULL': 16000.0, 'WALL_CULL': 25000.0, 'WEED_CULL': 4000.0}
MEDIUM_DISTANCE = 0.6
# The existing town kit's culls and slots (build_area_dressing.PIECES, the manifests), for the draw estimate.
EXISTING_CULL = {m: 'CLUTTER_CULL' for m in ('BottleCrate', 'BroomBucket', 'ChoppingBlock', 'FlourSacks', 'FlowerBox',
                                              'Horseshoe', 'MilkCans', 'PorchChair', 'PorchLantern', 'RockingChair',
                                              'RockingChair_Shawl', 'Spittoon', 'Washtub')}
EXISTING_CULL.update({m: 'FENCE_CULL' for m in ('Buckboard', 'Bunting', 'ChickenCoop', 'KitchenGarden', 'SignBracket',
                                                'LampPost', 'LaundryLine', 'Awning_4m', 'Awning_Post', 'Boardwalk_2m',
                                                'Boardwalk_4m', 'Boardwalk_Steps', 'SignBoard_Barber', 'SignBoard_Hotel',
                                                'SignBoard_Pruitts')})
EXISTING_SLOTS = {'Boardwalk_4m': 2, 'Boardwalk_2m': 2, 'Boardwalk_Steps': 2, 'LampPost': 2, 'Crate_B': 2}
# The existing types build_area_dressing now draws without a shadow in Ransom's Rest (SHADOWLESS_BY_AREA: under 0.6 m,
# or the walks' family), whose shadow draws the town's views save.
NOW_SHADOWLESS = ('Boardwalk_4m', 'Boardwalk_2m', 'Boardwalk_Steps', 'Bench', 'WaterTrough', 'Washtub')

# ---------------------------------------------------------------------------
# Frames the kit has no entry for yet (make_town.KIT's form: walls, decks (box, floor), steps, posts, builtin, doors), from
# Homestead.py (pivot: the front door at the ground, X out of the front).
# ---------------------------------------------------------------------------

FRAMES = {
    # The dogtrot (home_b: pens 4.4 m either side of a 2.6 m breezeway, 5 m deep; a 2.1 m porch the whole length at the
    # pens' 0.45 m floor, six posts; steps front and back of the breezeway; a chimney outside each end; a bench and a
    # washtub in the breezeway, a rain barrel out back).
    'RR_Home_B': dict(
        walls=[(-505.0, 5.0, 125.0, 575.0), (-505.0, 5.0, -575.0, -125.0), (-372.0, -237.0, 570.0, 667.0),
               (-268.0, -142.0, -662.0, -570.0)],
        decks=[((0.0, 210.0, -580.0, 580.0), 45.0), ((-500.0, 0.0, -130.0, 130.0), 45.0)],
        steps=[(210.0, 250.0, -80.0, 80.0), (-540.0, -500.0, -80.0, 80.0)],
        posts=[(198.0, 558.0), (198.0, 345.0), (198.0, 150.0), (198.0, -150.0), (198.0, -345.0), (198.0, -558.0)],
        builtin=[(-310.0, -170.0, 95.0, 130.0), (-415.0, -365.0, -120.0, -70.0), (-585.0, -525.0, 485.0, 545.0)],
        doors=[(250.0, 0.0, 1.0, 0.0, 160.0, 200.0), (-540.0, 0.0, -1.0, 0.0, 160.0, 220.0)]),
    # The L house (home_i: the main block 8 x 4.8 m, the gabled wing 3 m wide out 3 m to the front, a porch in the crook at
    # 0.45 m, its corner post fallen in the grass before it; the back door boarded).
    # The BUILDINGS lane's (2026-11-05, Docs/Work/2026-11-04-Towns/BUILDINGS.md "walls" and "bounds"): the body from the
    # walls, a porch, stoop or dock from the bounds in front of it (its floor taken at 0.45 m where the table gives none).
    'RR_Smithy_Town': dict(     # open front: the whole front is its way in
        walls=[(-700.0, 5.0, -450.0, 450.0)], doors=[(5.0, 0.0, 1.0, 0.0, 800.0, 250.0)]),
    'RR_Bathhouse': dict(       # the boiler and its firebox roof on its left (Blender +x: Unreal -Y) side
        walls=[(-500.0, 5.0, -300.0, 300.0), (-450.0, -50.0, -500.0, -300.0)], doors=[(5.0, 0.0, 1.0, 0.0, 110.0, 200.0)]),
    'RR_FeedStore': dict(       # the 1.0 m dock 2.5 m deep, its steps on the right
        walls=[(-1000.0, 5.0, -350.0, 350.0)], decks=[((0.0, 250.0, -350.0, 350.0), 100.0)],
        steps=[(250.0, 369.0, -350.0, -200.0)], doors=[(250.0, 0.0, 1.0, 0.0, 200.0, 200.0)]),
    'RR_BoardingHouse': dict(   # the outside stair up the right gable (keep 1.4 m clear there)
        walls=[(-800.0, 5.0, -550.0, 550.0), (-700.0, -73.0, -685.0, -550.0)],
        decks=[((0.0, 230.0, -550.0, 550.0), 45.0)], doors=[(230.0, 0.0, 1.0, 0.0, 120.0, 220.0)]),
    'RR_Home_K': dict(walls=[(-600.0, 5.0, -420.0, 420.0)], decks=[((0.0, 245.0, -460.0, 460.0), 45.0)],
                      doors=[(245.0, 0.0, 1.0, 0.0, 110.0, 200.0)]),
    'RR_Home_L': dict(walls=[(-740.0, 5.0, -134.0, 306.0)], steps=[(0.0, 175.0, -70.0, 70.0)],
                      doors=[(175.0, 0.0, 1.0, 0.0, 100.0, 150.0)]),
    'RR_Home_M': dict(walls=[(-560.0, 5.0, -460.0, 460.0)], decks=[((0.0, 245.0, -260.0, 260.0), 45.0)],
                      doors=[(245.0, 0.0, 1.0, 0.0, 110.0, 200.0)]),
    'RR_Home_N': dict(walls=[(-500.0, 5.0, -310.0, 310.0), (-720.0, -500.0, -310.0, 310.0)],
                      decks=[((0.0, 200.0, -150.0, 150.0), 45.0)], doors=[(200.0, 0.0, 1.0, 0.0, 110.0, 200.0)]),
    'RR_Home_O': dict(walls=[(-760.0, 5.0, -392.0, 108.0)], decks=[((0.0, 235.0, -443.0, 158.0), 45.0)],
                      doors=[(235.0, 0.0, 1.0, 0.0, 110.0, 200.0)]),
    'RR_Home_P': dict(walls=[(-500.0, 5.0, -562.0, 238.0)], decks=[((0.0, 210.0, -150.0, 150.0), 45.0)],
                      doors=[(210.0, 0.0, 1.0, 0.0, 110.0, 200.0)]),
    'RR_Home_I': dict(
        walls=[(-485.0, 5.0, -565.0, 245.0), (0.0, 305.0, -525.0, -215.0)],
        decks=[((0.0, 190.0, -215.0, 245.0), 45.0)],
        steps=[(190.0, 260.0, -65.0, 65.0)],
        posts=[(178.0, -5.0), (178.0, -208.0)],
        builtin=[(250.0, 350.0, 110.0, 280.0)],
        doors=[(260.0, 0.0, 1.0, 0.0, 100.0, 150.0), (-485.0, -82.0, -1.0, 0.0, 95.0, 120.0)]),
}

# ---------------------------------------------------------------------------
# Types
# ---------------------------------------------------------------------------

TYPES = {
    'store': 'shopOpen', 'undertaker': 'shopOpen', 'workshop': 'shopOpen', 'depot': 'yard', 'coalStage': 'yard',
    'cabinNorthWest': 'shopShut', 'saloon': 'shopShut', 'cabinNorth': 'shopShut', 'cabinSouth': 'shopShut',
    'sheriff': 'office',
    'cottageNorth': 'homeLived', 'cottageNorthEast': 'homeLived', 'dugout': 'homeLived', 'farmhouse': 'homeLived',
    'homeB': 'homeLived',
    'cottageSouth': 'homeEmpty', 'cottageSouthWest': 'homeEmpty', 'penLaneWest': 'homeEmpty', 'penLaneEast': 'homeEmpty',
    'amosHouse': 'homeEmpty', 'homeI': 'homeEmpty',
    'barn': 'barn', 'whitlockBarn': 'barn',
}
# A placement the table doesn't name takes its model's type (the towns plan's new buildings, steps 5-6).
MODEL_TYPES = {
    'RR_Home_B': 'homeLived', 'RR_Home_K': 'homeLived', 'RR_Home_M': 'homeLived', 'RR_Home_O': 'homeLived',
    'RR_BoardingHouse': 'homeLived',
    'RR_Home_I': 'homeEmpty', 'RR_Home_L': 'homeEmpty', 'RR_Home_N': 'homeEmpty', 'RR_Home_P': 'homeEmpty',
    'RR_Smithy_Town': 'shopOpen', 'RR_FeedStore': 'shopOpen', 'RR_Bathhouse': 'shopOpen',
}
# What the rules dress (the town: Main Street, Pen Lane, Tilly's workshop, the two placed homes; Amos's cabin gets its
# weeds only). The farmhouse is the story's (Delia's porch), the depot and the barns are later steps.
DRESSED = ('cabinNorthWest', 'sheriff', 'cottageNorth', 'saloon', 'cabinNorth', 'cottageNorthEast', 'cottageSouthWest',
           'cabinSouth', 'store', 'cottageSouth', 'undertaker', 'penLaneWest', 'penLaneEast', 'workshop', 'homeB',
           'homeI', 'amosHouse')
FRINGE_ONLY = ('amosHouse',)
# The weeds by type: a lived house's and an open shop's wall foot short dry grass, a shut or empty one's tall dock and
# thistle; one strip in SKIP_EVERY left out for rhythm.
FRINGE = {'homeLived': 'WeedFringe_A', 'shopOpen': 'WeedFringe_A', 'office': 'WeedFringe_A',
          'homeEmpty': 'WeedFringe_B', 'shopShut': 'WeedFringe_B'}
FRINGE_STEP = 200.0
SKIP_EVERY = 3
FRINGE_DOOR = 100.0      # a strip keeps this far (cm, along the wall) off a door's middle past its half width


def type_of(pid, model):
    return TYPES.get(pid) or MODEL_TYPES.get(model)


# ---------------------------------------------------------------------------
# The menus: (zone, spot rule, mesh or (meshes...), lift). SPOTS' rules (the building's own frame):
#   front_ends    against the front, at its ends (the left end, then the right)
#   front_sides   against the front, between the front door and the ends
#   side_front    hung on a side wall 1.1 m back from its front corner (left side, then right)
#   side_mid      against a side wall halfway back (left, then right)
#   side_back     against a side wall 0.8 m short of its back corner
#   back_door     against the back wall beside the back door (either side)
#   back_out      on the ground 2.5 m behind the back wall, off the back door's way
#   back_far      on the ground 4.5 m behind the back wall
#   deck_ends     on the front porch's deck against the front wall at its ends
# ---------------------------------------------------------------------------

MENUS = {
    'shopOpen': [('wall', 'side_front', 'Handbills_A', 145.0), ('back', 'back_door', 'CrateStack_B', 0.0),
                 ('back', 'back_out', 'StrawLitter_A', 0.0), ('side', 'side_back', 'Ladder_A', 0.0),
                 ('front', 'front_ends', 'CrateStack_A', 0.0)],
    'shopShut': [('wall', 'side_front', 'Handbills_A', 145.0), ('side', 'side_mid', 'Vignette_Alley_A', 0.0),
                 ('back', 'back_out', 'FallenBoards_A', 0.0)],
    'office': [('wall', 'side_front', 'Handbills_A', 145.0), ('back', 'back_out', 'FirewoodStack', 0.0),
               ('back', 'back_out', 'ChoppingBlock', 0.0)],
    'homeLived': [('front', 'deck_ends', ('RockingChair', 'PorchChair'), 0.0), ('front', 'deck_ends', 'Washtub', 0.0),
                  ('side', 'side_back', 'ToolsLeaning_A', 0.0), ('back', 'back_out', 'FirewoodStack', 0.0),
                  ('back', 'back_out', 'ChoppingBlock', 0.0), ('back', 'back_far', 'LaundryLine', 0.0)],
    'homeEmpty': [('front', 'deck_ends', 'Vignette_PorchEmpty_A', 0.0), ('side', 'side_mid', 'FallenBoards_A', 0.0),
                  ('back', 'back_out', 'FallenBoards_A', 0.0)],
}
# Menu items a building leaves out (its front, sides or back are full, or the story keeps them clear): (key, zone, mesh).
SKIP = {
    ('store', 'front', 'CrateStack_A'),         # Pruitt's porch: the hatch, the speaker post, the sacks, the buckboard
    ('undertaker', 'front', 'CrateStack_A'),    # Tilly's walk: her window, the bench, the broom
    ('workshop', 'side', 'Ladder_A'),            # its sides: the firewood and the cellar
    ('undertaker', 'side', 'Ladder_A'),          # its ladder is the yard side's (extras)
    ('cottageNorth', 'back', 'FirewoodStack'), ('cottageNorth', 'back', 'ChoppingBlock'),
    ('cottageNorth', 'back', 'LaundryLine'), ('cottageNorth', 'front', 'RockingChair'),
    ('cottageNorth', 'front', 'PorchChair'), ('cottageNorth', 'front', 'Washtub'),   # the Hasketts' yard is hand-made
    ('cottageNorthEast', 'back', 'FirewoodStack'), ('cottageNorthEast', 'back', 'ChoppingBlock'),
    ('cottageNorthEast', 'back', 'LaundryLine'), ('cottageNorthEast', 'front', 'RockingChair'),
    ('cottageNorthEast', 'front', 'PorchChair'), ('cottageNorthEast', 'front', 'Washtub'),   # the Mercers' likewise
    ('cottageSouth', 'front', 'Vignette_PorchEmpty_A'),   # the manager's porch keeps its rocker and crate
    ('cabinNorth', 'side', 'Vignette_Alley_A'),            # the hotel's sides: the saloon's balcony end, the lamp gap
    ('cabinSouth', 'side', 'Vignette_Alley_A'),            # the store's across Pen Lane's mouth is the one there (SPACING);
                                                            # its next spot, the barber's west wall, is in the Farm view
    ('cabinNorthWest', 'side', 'Vignette_Alley_A'),   # the assay's west side is the gate fight's ground
    ('cabinNorthWest', 'back', 'FallenBoards_A'),
    ('homeB', 'back', 'FirewoodStack'), ('homeB', 'back', 'ChoppingBlock'),
    ('homeB', 'back', 'LaundryLine'),                       # the dogtrot's yard is by hand (the stock pens behind it)
}

# Round 2 (2026-11-05, the PIE check DEMO2.md):
# - SPACING: no merged vignette within 15 m of another of itself (two of one cluster facing each other across Pen Lane read
#   copy-pasted), no stack within 6 m of its twin.
# - Crossings only over ruts or mud: on a dirt road's bed (wagon ruts: the road term's, or Main Street's own dust ruts),
#   or within CROSSING_WET of water spilt on the ground (a trough, a pump, a well: the wear term's wet). A path has one
#   trodden line, no ruts to bridge (the plank crossing on Pen Lane read as a dropped pallet).
# - Stacks (crates, barrels) only where someone stacked them: within STACK_ANCHOR of an open store's or a yard's wall, of
#   a wagon or cart (unloading), or of a back lot's yard group (the east end's pile stood in open street, no door behind).
SPACING = {'Vignette_': 1500.0, 'CrateStack_': 600.0, 'BarrelStack_': 600.0}
CROSSINGS = ('PlankCrossing_',)
CROSSING_ROADS = ('dirt',)
CROSSING_WET = 250.0
WET = ('WaterTrough', 'HandPump', 'RR_Well', 'Well', 'WaterBarrel_Cart')
STACKS = ('CrateStack_', 'BarrelStack_')
STACK_ANCHOR = 250.0
STACK_HOSTS = ('shopOpen', 'yard')
WAGONS = ('Buckboard', 'Cart', 'Dray', 'WagonFreight')
YARD_GROUPS = ('storeBacklot', 'saloonBacklot', 'yardStacks', 'yardWagon')

# Spawn points and patrol routes: every rule piece keeps this far off (the job's 3 m; make_town's own pieces keep 2.5 m).
SPAWN_CLEAR_RULES = 300.0
# The street's middle lane (Main Street, the yard road): no event's box comes within this of the road's middle line.
LANE_HALF = 175.0
EVENT_ROADS = ('mainStreet', 'yardRoad')
# A loudspeaker post's spot (build_area_landfall.TOWN_SPEAKERS): nothing within this of it.
SPEAKER_CLEAR = 100.0


def crc(*parts):
    return zlib.crc32('|'.join(str(p) for p in parts).encode('utf-8')) & 0xffffffff


def jitter(key, span):
    """A yaw jitter in [-span, span] degrees from the crc of key."""
    return ((crc(key, 'yaw') % 2001) / 1000.0 - 1.0) * span


def pick(options, *key):
    if isinstance(options, str):
        return options
    return options[crc(*key) % len(options)]


# ---------------------------------------------------------------------------
# The kit's tables for make_town
# ---------------------------------------------------------------------------

def new_kit():
    """make_town.NEW_KIT's entries for the new meshes: (box, height, solid, kind)."""
    return {m: (v[0], v[1], v[2], v[3]) for m, v in KIT.items()}


def dressing_size(mesh):
    """build_area_dressing.PIECES' (size along Y, across X, up), its cull's name and the footprint's offsets."""
    (x0, x1, y0, y1), height, _, _, _, _, cull, _ = KIT[mesh]
    return (round(y1 - y0), round(x1 - x0), round(height)), cull, (x0 + x1) * 0.5, (y0 + y1) * 0.5


# ---------------------------------------------------------------------------
# The extra spots the checks keep off (always added: the probe on disk may predate them)
# ---------------------------------------------------------------------------

def module_constants(path, names):
    """Top-level constants of another script (its literal tuples), read without running it."""
    out = {}
    try:
        with open(path, encoding='utf-8') as file:
            tree = ast.parse(file.read())
    except (OSError, SyntaxError):
        return out
    for node in tree.body:
        if isinstance(node, ast.Assign):
            for target in node.targets:
                if getattr(target, 'id', None) in names:
                    try:
                        out[target.id] = ast.literal_eval(node.value)
                    except ValueError:
                        pass
    return out


def rule_spots(world, mt):
    """The spots the rules keep off, read from the scripts that place them: the town's loudspeaker posts
    (build_area_landfall), Main 3's checkpoint at the gate (build_area_main3: the sandbag walls, the riflemen's and the
    hose-man's posts, the barrel fire, the Unpaid's rise spots, the Gilt Spur's hose cart), and every encounter's spawn
    spots, set-piece beats and patrol routes in the layout (its gameplay.encounters)."""
    added = 0
    land = module_constants(os.path.join(UNREAL_TOOLS, 'build_area_landfall.py'), ('TOWN_SPEAKERS',))
    for k, (x, y) in enumerate(land.get('TOWN_SPEAKERS', ())):
        world.spots.append((f'the loudspeaker post {k + 1} (build_area_landfall)', (float(x), float(y)), SPEAKER_CLEAR,
                            'speaker'))
        added += 1
    m3 = module_constants(os.path.join(UNREAL_TOOLS, 'build_area_main3.py'),
                          ('SANDBAG_WALLS', 'SANDBAGS_PER_WALL', 'SANDBAG_STEP', 'RIFLE_SPOTS', 'HOSEMAN_SPOT', 'FIRE_AT',
                           'UNPAID_SPOTS', 'SALOON_CART_OUT', 'SALOON_CART_ALONG'))
    gate = gate_frame(world)
    if gate is not None:
        (gx, gy), (ix, iy), (sx, sy) = gate

        def at(along_in, along_side):
            return gx + ix * along_in + sx * along_side, gy + iy * along_in + sy * along_side
        n = m3.get('SANDBAGS_PER_WALL', 2)
        step = m3.get('SANDBAG_STEP', 110.0)
        for k, (a, s) in enumerate(m3.get('SANDBAG_WALLS', ())):
            for j in range(n):
                off = (j - (n - 1) * 0.5) * step
                world.spots.append((f'the checkpoint\'s sandbag wall {k + 1} (build_area_main3)', at(a, s + off), 120.0,
                                    'rspot'))
                added += 1
        for k, (a, s) in enumerate(m3.get('RIFLE_SPOTS', ())):
            world.spots.append((f'the checkpoint\'s rifleman {k + 1} (build_area_main3)', at(a, s), SPAWN_CLEAR_RULES,
                                'rspawn'))
            added += 1
        if 'HOSEMAN_SPOT' in m3:
            world.spots.append(('the checkpoint\'s hose-man (build_area_main3)', at(*m3['HOSEMAN_SPOT']),
                                SPAWN_CLEAR_RULES, 'rspawn'))
            added += 1
        if 'FIRE_AT' in m3:
            world.spots.append(('the Company\'s barrel fire (build_area_main3)', at(*m3['FIRE_AT']), 150.0, 'rspot'))
            added += 1
        for k, (dx, dy) in enumerate(m3.get('UNPAID_SPOTS', ())):
            # Layout X north and Y east off the gate's middle.
            world.spots.append((f'the gate fight\'s rise spot {k + 1} (build_area_main3)', (gx + dx, gy + dy),
                                SPAWN_CLEAR_RULES, 'rspawn'))
            added += 1
    saloon = world.places.get('saloon')
    if saloon and 'SALOON_CART_OUT' in m3:
        sx, sy = saloon['location'][:2]
        way = 1.0 if sx > 0.0 else -1.0
        b = world.buildings.get('saloon')
        face = (min(w.bbox()[0] for w in b.walls) if way > 0 else max(w.bbox()[2] for w in b.walls)) if b else sx
        world.spots.append(('the Gilt Spur\'s hose cart (build_area_main3, Main 3)',
                            (face - way * m3['SALOON_CART_OUT'], sy + m3.get('SALOON_CART_ALONG', 0.0)), 200.0, 'rspot'))
        added += 1
    enc = world.layout.get('gameplay', {}).get('encounters', {})
    for group in ('camps', 'patrols', 'ambushes', 'fights', 'lurks'):
        for e in enc.get(group, []) or []:
            if e.get('frame'):          # the mine's own frame, not the valley's
                continue
            spots = list(e.get('spots') or [])
            for beat in (e.get('setPiece') or {}).get('beats', []) or []:
                spots += beat.get('spots') or []
            for s in spots:
                if near_town(s):
                    world.spots.append((f'{group} {e.get("id")}\'s spawn spot', (float(s[0]), float(s[1])),
                                        SPAWN_CLEAR_RULES, 'rspawn'))
                    added += 1
            route = e.get('route') or []
            if len(route) > 1 and any(near_town(p) for p in route):
                world.routes.append((f'{group} {e.get("id")}\'s route', [tuple(map(float, p[:2])) for p in route]))
                added += 1
    world.notes.append(f'rules: {added} spots and routes from build_area_landfall, build_area_main3 and the layout\'s '
                       f'encounters kept clear ({SPAWN_CLEAR_RULES / 100.0:.0f} m off spawn points and routes for the '
                       f'rules\' pieces)')


def near_town(p):
    return -9000.0 < p[0] < 5000.0 and -9000.0 < p[1] < 15000.0


def gate_frame(world):
    """The town gate between its posts (build_area_main3.Gate): its middle, in along Main Street, side across it."""
    posts = [o for o in world.layout.get('obstacles', []) if o['id'] in ('townGateNorth', 'townGateSouth')]
    if len(posts) != 2:
        return None
    mids = [(sum(q[0] for q in o['polygon']) / len(o['polygon']), sum(q[1] for q in o['polygon']) / len(o['polygon']))
            for o in posts]
    gx, gy = (mids[0][0] + mids[1][0]) * 0.5, (mids[0][1] + mids[1][1]) * 0.5
    street = next((r for r in world.layout.get('roads', []) if r['id'] == 'mainStreet'), None)
    ahead = street['path'][1] if street and len(street['path']) > 1 else (gx, gy + 1000.0)
    dx, dy = ahead[0] - gx, ahead[1] - gy
    length = math.hypot(dx, dy) or 1.0
    ix, iy = dx / length, dy / length
    return (gx, gy), (ix, iy), (-iy, ix)


# ---------------------------------------------------------------------------
# A building as the rules see it, in its own frame
# ---------------------------------------------------------------------------

class Frame:
    def __init__(self, mt, b, spec, kind):
        self.b, self.kind = b, kind
        self.walls = [tuple(w) for w in spec.get('walls', [])]
        # Its body: the walls' boxes that are a part of the house (a dogtrot's two pens), not a chimney or a shed on it.
        biggest = max((w[1] - w[0]) * (w[3] - w[2]) for w in self.walls)
        main = [w for w in self.walls if (w[1] - w[0]) * (w[3] - w[2]) >= 0.3 * biggest]
        x0, x1 = min(w[0] for w in main), max(w[1] for w in main)
        y0, y1 = min(w[2] for w in main), max(w[3] for w in main)
        self.body = (x0, x1, y0, y1)
        self.decks = [(tuple(d), z) for d, z in spec.get('decks', [])]
        self.steps = [tuple(s) for s in spec.get('steps', [])]
        self.posts = [tuple(p) for p in spec.get('posts', [])]
        self.doors = [tuple(dd) for dd in spec.get('doors', [])]
        self.front_deck = next(((d, z) for d, z in self.decks if d[0] >= x1 - 10.0 and d[1] > x1 + 50.0), None)

    def door_spans(self, face):
        """The doors on a face: (along-the-face middle, half width)."""
        out = []
        for fx, fy, dx, dy, width, _ in self.doors:
            if face == 'front' and dx > 0.5:
                out.append((fy, width * 0.5))
            elif face == 'back' and dx < -0.5:
                out.append((fy, width * 0.5))
            elif face == 'left' and dy > 0.5:
                out.append((fx, width * 0.5))
            elif face == 'right' and dy < -0.5:
                out.append((fx, width * 0.5))
        return out


def frames(mt, world):
    """Every dressed building's frame and type, in the design's order (DRESSED, then any typed placement not named)."""
    out = []
    for pid in DRESSED:
        b = world.buildings.get(pid)
        if b is None:
            continue
        kind = type_of(pid, b.model)
        spec = mt.KIT.get(b.model)
        if kind is None or not spec or not spec.get('walls'):
            continue
        out.append(Frame(mt, b, spec, kind))
    # A new building (steps 5-6) the table doesn't name: its model's type is enough. Without a frame in make_town.KIT or
    # FRAMES it's dressed from its mesh's bounds (eaves and all, so its weeds stand a little off its walls), its front
    # door taken at its pivot (the homes' and the town buildings' rule) and no back door: give it a frame for better.
    for pid, b in sorted(world.buildings.items()):
        if pid in DRESSED or pid in TYPES:
            continue
        kind = MODEL_TYPES.get(b.model)
        spec = mt.KIT.get(b.model)
        if kind and not spec and b.model in world.meshes.bounds:
            x0, x1, y0, y1 = world.meshes.box(b.model)
            spec = dict(walls=[(x0, x1, y0, y1)], doors=[(x1, 0.0, 1.0, 0.0, 120.0, 250.0)])
            world.notes.append(f'rules: {pid} ({b.model}, {kind}) has no frame: dressed from its bounds, its door at its '
                               f'pivot (add its frame to town_rules.FRAMES)')
        if kind and spec and spec.get('walls'):
            out.append(Frame(mt, b, spec, kind))
    return out


# ---------------------------------------------------------------------------
# Placing
# ---------------------------------------------------------------------------

class Rules:
    def __init__(self, mt, d, skip):
        self.mt, self.d, self.world = mt, d, d.world
        self.skip = set(skip or ())
        self.refused = []
        self.unfit_why = []
        self.made = []

    def unfit(self, mesh, x, y):
        """Why a crossing or a stack has no business at (x, y) (the round 2 rules above), or None."""
        w = self.world
        if mesh.startswith(CROSSINGS):
            for rid, pts, half, kind in w.roads:
                if (kind or 'dirt') in CROSSING_ROADS and min_seg(pts, (x, y)) <= half - 60.0:
                    return None
            if self.near_mesh(WET, x, y, CROSSING_WET):
                return None
            return 'no ruts or mud to cross (a path or bare ground)'
        if mesh.startswith(STACKS):
            for pid, b in w.buildings.items():
                if type_of(pid, b.model) in STACK_HOSTS and any(dist_box(wall, x, y) <= STACK_ANCHOR for wall in b.walls):
                    return None
            if self.near_mesh(WAGONS, x, y, STACK_ANCHOR, boxes=True):
                return None
            for o in w.layout.get('obstacles', []):
                if o['id'] in YARD_GROUPS and o.get('polygon') and \
                        min_seg([tuple(v[:2]) for v in o['polygon']] + [tuple(o['polygon'][0][:2])], (x, y)) <= 400.0:
                    return None
            return 'no store, yard or wagon by it (who stacked it here?)'
        return None

    def walk_at(self, group, y):
        """The walk section of a run under a spot along the street (for a piece that stands on it)."""
        for s in self.d.pieces:
            if s.mount == 'walk' and s.group == group and s.mesh.startswith('Boardwalk_') and s.mesh != 'Boardwalk_Steps':
                span = 400.0 if s.mesh.startswith('Boardwalk_4m') else 200.0
                lo, hi = sorted((s.y, s.y + span * (1.0 if s.yaw == 180.0 else -1.0)))
                if lo <= y <= hi:
                    return s
        return None

    def near_mesh(self, names, x, y, reach, boxes=False):
        """Whether a design piece or a layout placement of these meshes stands within reach (its box, for a wagon)."""
        for o in self.d.pieces:
            if o.mesh.startswith(names):
                if boxes:
                    bx = self.world.meshes.box(o.mesh)
                    box = self.mt.Box.frame(o.x, o.y, o.yaw, *bx)
                    if self.mt.point_box_dist(box, x, y) <= reach:
                        return True
                elif math.hypot(o.x - x, o.y - y) <= reach:
                    return True
        for b in self.world.buildings.values():
            if b.model.startswith(names) and any(dist_box(wall, x, y) <= reach for wall in b.walls):
                return True
        return False

    def allowed(self, mesh):
        if mesh in self.skip:
            return False
        if mesh in KIT or mesh in WALK_VARIANTS:
            if mesh not in CHOSEN:
                self.refused.append(mesh)
                return False
        if mesh in BREAKABLE:
            self.refused.append(mesh)
            return False
        return True

    def add(self, mesh, x, y, yaw, group, cls, alts=(), **kw):
        if not self.allowed(mesh):
            return None
        why = self.unfit(mesh, x, y)
        if why:
            self.unfit_why.append(f'{group} {mesh} at ({x:.0f}, {y:.0f}): {why}')
            return None
        on_road = kw.pop('on_road', False)
        group = kw.pop('group_override', group)
        p = self.d.add(mesh, x, y, yaw, group, cls, **kw)
        p.on_road = on_road
        p.rule = True
        p.alts = list(alts)
        p.spawn_clear = SPAWN_CLEAR_RULES
        self.made.append(p)
        return p

    def local(self, f, mesh, lx, ly, lyaw, cls, alts=(), group=None, **kw):
        """A piece in f's frame; alts: other (lx, ly, lyaw) to try first if it won't fit."""
        b = f.b
        x, y = b.at(lx, ly)
        world_alts = [(*b.at(ax, ay), b.yaw + ayaw) for ax, ay, ayaw in alts]
        kw.setdefault('host', b.id if kw.get('mount') in ('porch', 'wall', 'air') and kw.get('on') is None else None)
        return self.add(mesh, x, y, b.yaw + lyaw, group or b.id, cls, alts=world_alts, **kw)

    # --- The menus ---

    def menu(self, f):
        if f.b.id in FRINGE_ONLY:
            return
        for k, (zone, rule, meshes, lift) in enumerate(MENUS.get(f.kind, [])):
            mesh = pick(meshes, f.b.id, zone, k)
            if (f.b.id, zone, mesh) in SKIP:
                continue
            spots = self.spots(f, rule, mesh)
            if not spots:
                continue
            (lx, ly, lyaw, mount), rest = spots[0], spots[1:]
            span = 15.0 if mesh in ('CrateStack_A', 'CrateStack_B', 'BarrelStack_A') else 8.0
            j = 0.0 if KIT.get(mesh, (None, 0, 0, ''))[3] == 'wall' or mesh.startswith('Vignette_') or \
                mesh in ('ToolsLeaning_A', 'Ladder_A') else jitter((f.b.id, zone, k), span)
            alts = [(ax, ay, ayaw + j) for ax, ay, ayaw, amount in rest if amount == mount]
            if mount == 'wall':
                self.local(f, mesh, lx, ly, lyaw, 'wall', alts=alts, mount='wall', lift=lift)
            elif mount == 'deck':
                deck, z = f.front_deck
                self.local(f, mesh, lx, ly, lyaw + j, 'porch', alts=alts, mount='porch', lift=z,
                           against=f.b.id if hugs(mesh) else None)
            else:
                cls = 'small' if mesh in KIT or self.height(mesh) < 160.0 else 'big'
                self.local(f, mesh, lx, ly, lyaw + j, cls, alts=alts, against=f.b.id if hugs(mesh) else None,
                           along=True if hugs(mesh) else None)

    def spots(self, f, rule, mesh):
        """The rule's spots in f's frame, in the order to try: (lx, ly, lyaw, mount) with mount 'ground', 'deck' or
        'wall' (a wall piece's pivot on the face; another piece's middle a little out from it)."""
        x0, x1, y0, y1 = f.body
        box = self.mt.NEW_KIT[mesh][0] if mesh in self.mt.NEW_KIT else self.mt_box(mesh)
        bx0, bx1, by0, by1 = box
        depth, width = bx1 - bx0, by1 - by0
        wall_pivot = bx0 >= -1.0              # its pivot on its back (a wall piece, a vignette, tools)
        out = []

        flat = KIT.get(mesh, (None, 0, 0, ''))[3] == 'flat'

        def against(face, along, gap=4.0):
            # Ground cover turned a little off its wall keeps a hand's breadth from it.
            gap = max(gap, 15.0) if flat else gap
            """A spot against a face at 'along' (the face's own coordinate), the piece's back to it."""
            if face == 'front':
                yaw, nx = 0.0, x1
                lx = nx + gap + (0.0 if wall_pivot else -bx0)
                return lx, along, yaw
            if face == 'back':
                yaw, nx = 180.0, x0
                lx = nx - gap - (0.0 if wall_pivot else -bx0)
                return lx, along, yaw
            if face == 'left':
                ly = y1 + gap + (0.0 if wall_pivot else -bx0)
                return along, ly, 90.0
            ly = y0 - gap - (0.0 if wall_pivot else -bx0)
            return along, ly, -90.0

        if rule in ('front_ends', 'front_sides', 'deck_ends'):
            mount = 'deck' if rule == 'deck_ends' else 'ground'
            if rule == 'deck_ends' and not f.front_deck:
                return []
            if mount == 'deck':
                deck = f.front_deck[0]
                lo, hi = deck[2] + 25.0, deck[3] - 25.0
            else:
                lo, hi = y0 + 30.0, y1 - 30.0
            doors = f.door_spans('front')
            ends = [hi - width * 0.5, lo + width * 0.5]
            if rule == 'front_sides' and doors:
                dy, dh = doors[0]
                ends = [(dy + dh + 60.0 + hi) * 0.5, (dy - dh - 60.0 + lo) * 0.5]
            for along in ends:
                if any(abs(along - dy) < dh + width * 0.5 + 40.0 for dy, dh in doors):
                    continue
                if mount == 'deck':
                    lx = x1 + 4.0 + (0.0 if wall_pivot else -bx0)
                    out.append((lx, along, 0.0, 'deck'))
                else:
                    lx, ly, yaw = against('front', along)
                    out.append((lx, ly, yaw, 'ground'))
            return out
        if rule == 'side_front':
            for face in ('left', 'right'):
                if not self.open_side(f, face):
                    continue
                along = x1 - 110.0
                if any(abs(along - dx) < dh + 70.0 for dx, dh in f.door_spans(face)):
                    along -= 150.0
                lx, ly, yaw = against(face, along, gap=1.0)
                out.append((lx, ly, yaw, 'wall'))
            return out
        if rule in ('side_mid', 'side_back'):
            for face in ('left', 'right'):
                if not self.open_side(f, face):
                    continue
                along = (x0 + x1) * 0.5 if rule == 'side_mid' else x0 + width * 0.5 + 80.0
                lx, ly, yaw = against(face, along)
                out.append((lx, ly, yaw, 'ground'))
            return out
        if rule == 'back_door':
            doors = f.door_spans('back')
            for dy, dh in doors or [((y0 + y1) * 0.5, 0.0)]:
                for side in (1.0, -1.0):
                    along = dy + side * (dh + width * 0.5 + 60.0)
                    if lo_hi_ok(along, width, y0, y1):
                        lx, ly, yaw = against('back', along)
                        out.append((lx, ly, yaw, 'ground'))
            return out
        if rule in ('back_out', 'back_far'):
            out_by = 250.0 if rule == 'back_out' else 450.0
            doors = f.door_spans('back')
            mid = (y0 + y1) * 0.5
            cands = [mid + (y1 - y0) * 0.28, mid - (y1 - y0) * 0.28, mid]
            for along in cands:
                if any(abs(along - dy) < dh + width * 0.5 + 60.0 for dy, dh in doors):
                    continue
                out.append((x0 - out_by - depth * 0.5, along, 180.0, 'ground'))
            return out
        return out

    def height(self, mesh):
        if mesh in KIT:
            return KIT[mesh][1]
        return self.world.meshes.height(mesh)

    def open_side(self, f, face, need=250.0):
        """Whether a side wall faces open ground (an alley at least need wide), not a neighbour's wall 1-2 m off."""
        x0, x1, y0, y1 = f.body
        ly = y1 + need if face == 'left' else y0 - need
        for t in (0.2, 0.5, 0.8):
            wx, wy = f.b.at(x0 + (x1 - x0) * t, ly)
            for o in self.world.buildings.values():
                if o is f.b:
                    continue
                if any(w.contains(wx, wy) for w in o.walls):
                    return False
        return True

    def mt_box(self, mesh):
        try:
            return self.world.meshes.box(mesh)
        except KeyError:
            return (-50.0, 50.0, -50.0, 50.0)

    # --- The wall feet ---

    def fringe(self, f):
        """Weeds along every wall foot of the building (and its porch's skirt), strips of FRINGE_STEP laid end to end,
        one in SKIP_EVERY left out (by crc), none over a door's way or steps (the placer drops what meets anything)."""
        mesh = FRINGE.get(f.kind)
        if mesh is None or not self.allowed(mesh):
            return
        boxes = [w for w in f.walls] + [d for d, _ in f.decks]
        for wi, (x0, x1, y0, y1) in enumerate(boxes):
            is_deck = wi >= len(f.walls)
            for face in ('front', 'back', 'left', 'right'):
                if face in ('front', 'back'):
                    a0, a1 = y0, y1
                else:
                    a0, a1 = x0, x1
                length = a1 - a0
                if length < 120.0:
                    continue
                n = max(1, int(round(length / FRINGE_STEP)))
                step = length / n
                doors = f.door_spans(face) if not is_deck else []
                for k in range(n):
                    along = a0 + step * (k + 0.5)
                    if crc(f.b.id, wi, face, k) % SKIP_EVERY == 0:
                        continue
                    if any(abs(along - dm) < dh + FRINGE_DOOR for dm, dh in doors):
                        continue
                    if face == 'front':
                        lx, ly, yaw = x1 + 1.0, along, 0.0
                    elif face == 'back':
                        lx, ly, yaw = x0 - 1.0, along, 180.0
                    elif face == 'left':
                        lx, ly, yaw = along, y1 + 1.0, 90.0
                    else:
                        lx, ly, yaw = along, y0 - 1.0, -90.0
                    # Skip a face that's inside another of the building's boxes (a wing's join, a porch's wall side).
                    px, py = probe_out(lx, ly, yaw, 20.0)
                    if any(bx0 - 1.0 <= px <= bx1 + 1.0 and by0 - 1.0 <= py <= by1 + 1.0
                           for j, (bx0, bx1, by0, by1) in enumerate(boxes) if j != wi):
                        continue
                    # (Strips closer than their 2.1 m overlap a little: their ends thin out, so no seam shows.)
                    self.local(f, mesh, lx, ly, yaw, 'weed', group=f.b.id, against=f.b.id, along=True)

    def ring(self, host, group):
        """A ring of dry grass round a post's foot (WeedFringe_C): it follows the post wherever the placer stands it."""
        if not self.allowed('WeedFringe_C'):
            return
        p = self.add('WeedFringe_C', host.x, host.y, host.yaw + jitter(host.mesh + group, 180.0), group, 'weed',
                     on_road=True)
        if p is not None:
            p.follow = (host, 0.0, 0.0)
            p.ring_of = host

    def rail_rings(self, rail, group):
        if not self.allowed('WeedFringe_C'):
            return
        for k, ly in enumerate((-145.0, 145.0)):
            if crc(rail.x, rail.y, k) % SKIP_EVERY == 0:
                continue
            p = self.add('WeedFringe_C', rail.x, rail.y, rail.yaw + jitter((rail.x, k), 180.0), group, 'weed',
                         on_road=True)
            if p is not None:
                p.follow = (rail, 0.0, ly)
                p.ring_of = rail


def min_seg(pts, p):
    """The distance from p to a polyline."""
    best = 1e18
    for (ax, ay), (bx, by) in zip(pts, pts[1:]):
        dx, dy = bx - ax, by - ay
        l2 = dx * dx + dy * dy
        t = 0.0 if l2 < 1e-9 else max(0.0, min(1.0, ((p[0] - ax) * dx + (p[1] - ay) * dy) / l2))
        best = min(best, math.hypot(p[0] - ax - dx * t, p[1] - ay - dy * t))
    return best


def dist_box(box, x, y):
    lx, ly = box.local(x, y)
    return math.hypot(max(abs(lx) - box.hx, 0.0), max(abs(ly) - box.hy, 0.0))


def spacing_of(mesh):
    for prefix, gap in SPACING.items():
        if mesh.startswith(prefix):
            return gap
    return 0.0


def hugs(mesh):
    """A piece that stands with its back to a wall (its pivot on the wall's face)."""
    entry = KIT.get(mesh)
    return bool(entry and entry[0][0] >= -1.0 and entry[3] != 'flat')


def lo_hi_ok(along, width, y0, y1):
    return y0 + width * 0.5 + 20.0 <= along <= y1 - width * 0.5 - 20.0


def probe_out(lx, ly, yaw, dist):
    a = math.radians(yaw)
    return lx + math.cos(a) * dist, ly + math.sin(a) * dist


# ---------------------------------------------------------------------------
# The town's own extras (cm, layout frame; worked out against the spots, walks and lanes, 2026-11-04)
# ---------------------------------------------------------------------------

def street(r):
    """Main Street's, the yard road's and Pen Lane's events and crossings, every 25-35 m, in the 2.5 m strip beside the
    walks (|X| 250-500 on Main Street), never in the middle 3.5 m lane, 3 m off every spawn point and route, out of the
    roosts' find circles, off the loudspeaker posts. These are candidates: Rules.unfit() takes off a crossing with
    nothing to cross and a stack nobody would have stacked there (round 2: Pen Lane's and the fields path's crossings,
    the east end's pile)."""
    g = 'street'
    # The gate end: a water cart drawn up by the shack and the trough (a barrel cart for the houses without wells).
    r.add('WaterBarrel_Cart', -380.0, 450.0, 90.0 + jitter('watercart', 6.0), g, 'event')
    # Pruitt's: a crossing of planks laid out over the south rut from the store's porch steps (Pruitt's and the
    # saloon's traffic); flat, no collision, so it neither covers nor blocks.
    r.add('PlankCrossing_A', -365.0, 3000.0, 0.0 + jitter('crossing store', 4.0), g, 'flat', on_road=True)
    # A traveller's covered wagon drawn up across from the hotel, tongue east toward the yard road.
    r.add('WagonFreight_Canvas', -400.0, 4985.0, 90.0 + jitter('canvas', 3.0), g, 'event')
    # Barrels off the depot, left on their sleepers at the street's east end for the store (the yard road's freight).
    r.add('BarrelStack_A', 400.0, 6700.0, 90.0 + jitter('barrels east', 12.0), g, 'event')
    # Instead at the street's east end, where the yard road comes in from the depot: a trough and a rail at its edge, the
    # riders' tie-up, as the gate has at the west end (kit pieces: no new draws).
    r.add('WaterTrough', 525.0, 6640.0, jitter('east trough', 3.0), 'mainStreet', 'street')
    r.add('HitchRail', 490.0, 6960.0, jitter('east rail', 2.0), 'mainStreet', 'street')
    # Front bins under 5 with room the fights allow (round 2; kit pieces, no new draws): a spittoon at the end of the
    # assay office's bench, milk cans out by the Mercers' porch for the dairy, a washtub by the water cart, a coffin
    # out by Bright & Daughter's walk with the timber crates.
    r.add('Spittoon', 765.0, 333.0, 0.0, 'assay', 'porch', mount='porch', on=r.walk_at('assay', 333.0), lift=38.0)
    r.add('MilkCans', 750.0, 6090.0, 180.0 + jitter('mercer cans', 20.0), 'mercer', 'small')
    r.add('Washtub', -560.0, 600.0, jitter('cart tub', 20.0), 'street', 'small')
    r.add('Coffin_Closed', -740.0, 6800.0, jitter('undertaker coffin', 6.0), 'undertaker', 'small')
    # Pruitt's second rail, before the west end of his porch (his is the busiest front; the first stands at 23.8 m).
    r.add('HitchRail', -462.0, 2700.0, 0.0, 'mainStreet', 'street')
    # The fields path's mouth: a trough by the parked wagon's team, between the crossing lamp and the manager's porch.
    r.add('WaterTrough', -700.0, 4720.0, jitter('fields trough', 3.0), 'mainStreet', 'street')
    # The yard road's mouth: planks across its mud toward the undertaker's yard.
    r.add('PlankCrossing_A', 60.0, 7450.0, -4.0 + jitter('crossing yard', 4.0), g, 'flat', on_road=True)
    # Pen Lane: planks over its wet stretch past the bend from the street.
    r.add('PlankCrossing_A', -1300.0, 2060.0, 0.0 + jitter('crossing pen', 6.0), 'penLane', 'flat', on_road=True)
    # The fields path's mouth: planks along it where it leaves the street's ruts.
    r.add('PlankCrossing_A', -1250.0, 4530.0, 169.5 + jitter('crossing fields', 4.0), g, 'flat', on_road=True)


def extras(r):
    """What the menus can't know (2026-11-04, worked out against the walks, the doors, the shutters, the posters, the
    fights' spots and the roost's find circle): the fronts' stories building by building, the alleys' junk, the new
    homes' porches and yards. Hand-made pieces of the town (make_town.py) stand already; these fill round them."""
    f = {fr.b.id: fr for fr in r.frames}
    walks = {}
    for p in r.d.pieces:
        if p.mount == 'walk' and p.mesh.startswith('Boardwalk_') and p.mesh != 'Boardwalk_Steps':
            walks.setdefault(p.group, []).append(p)

    def on_walk(group, mesh, x, y, yaw, cls='porch', **kw):
        run = walks.get(group)
        if not run:
            return None
        host = min(run, key=lambda s: abs((s.y - 200.0 if s.yaw == 180.0 else s.y + 200.0) - y)
                   if s.mesh.startswith('Boardwalk_4m') else abs(s.y - y))
        best = None
        for s in run:
            lo, hi = sorted((s.y, s.y + (400.0 if s.mesh.startswith('Boardwalk_4m') else 200.0) *
                             (1.0 if s.yaw == 180.0 else -1.0)))
            if lo <= y <= hi:
                best = s
        return r.add(mesh, x, y, yaw, group, cls, mount='porch', on=best or host, lift=38.0, **kw)

    # The assay office's alley mouth (north side, Y 358-850): a stack of crates left on the walk where it crosses the
    # gap, and junk against the sheriff's west wall 4-6 m back (out of the roost's 9 m find circle).
    on_walk('assay', 'CrateStack_A', 755.0, 430.0, 180.0 + jitter('assay crates', 12.0), against=None)
    on_walk('assay', 'BottleCrate', 772.0, 470.0, 180.0 + jitter('assay empties', 10.0), against=None)
    if 'sheriff' in f:
        fr = f['sheriff']
        x0, x1, y0, y1 = fr.body
        r.local(fr, 'Vignette_Alley_A', -30.0, y1 + 4.0, 90.0, 'small', against='sheriff', along=True, group='sheriff',
                alts=[(-150.0, y1 + 4.0, 90.0)])
    # The dead sheriff's spittoon by his chair; Pruitt's and the Gilt Spur's notices on the walls by the street.
    on_walk('assay', 'Spittoon', 760.0, 1062.0, 0.0, group_override='sheriff')
    on_walk('assay', 'BroomBucket', 788.0, 1345.0, 180.0, group_override='sheriff', against='sheriff')
    if 'cottageNorthEast' in f:
        # The Mercers': a wheelbarrow left by the porch's east end, a rain barrel at its corner.
        r.add('Wheelbarrow', 640.0, 6040.0, 100.0 + jitter('mercer barrow', 10.0), 'mercer', 'small')
        r.add('Barrel_B', 1045.0, 6200.0, 0.0, 'mercer', 'small')
    if 'cottageNorth' in f:
        # The Hasketts' front: the old father's chair out by the door, turned to the street.
        r.local(f['cottageNorth'], 'PorchChair', 45.0, -140.0, 15.0, 'small', group='haskett',
                alts=[(45.0, -320.0, -10.0)])
    if 'cabinNorth' in f:
        # The Larkspur House, shut: its porch's east end left as the last guest did, a chair tipped over, a crate of
        # empties, boards down off the boarded window (the west end is the Main Street fight's).
        fr = f['cabinNorth']
        r.local(fr, 'PorchChair', 70.0, -250.0, 75.0, 'porch', mount='porch', lift=38.0 + 23.0, tilt=(0.0, 90.0),
                box=(-25.0, 25.0, -100.0, 100.0), group='hotel', alts=[(80.0, -330.0, 60.0)])
        r.local(fr, 'BottleCrate', 25.0, -330.0, 8.0, 'porch', mount='porch', lift=38.0, group='hotel',
                against='cabinNorth', alts=[(25.0, -180.0, -6.0)])
        r.local(fr, 'FallenBoards_A', 170.0, -390.0, 80.0, 'flat', mount='porch', lift=38.0, group='hotel',
                alts=[(160.0, -200.0, 95.0)])
        # Its hitch rail at the street's edge east of the door (the hotel's guests', clear of its door's way).
        r.add('HitchRail', 462.0, 4690.0, 0.0, 'mainStreet', 'street')
    # The street's east end, north: crates by the barrels off the depot.
    r.add('CrateStack_A', 430.0, 6420.0, 90.0 + jitter('east crates', 12.0), 'street', 'event')
    # Pruitt's freight: crates unloaded behind his buckboard's tail, a drum by them (clear of the fields path's mouth).
    r.add('CrateStack_A', -760.0, 4170.0, 4.0 + jitter('buckboard crates', 12.0), 'store', 'small')
    r.add('Barrel_B', -880.0, 4090.0, jitter('buckboard drum', 40.0), 'store', 'small')
    if 'cabinSouth' in f:
        # The barber's east side, the alley to Pen Lane: junk against its wall, 2-4 m back from the walk.
        fr = f['cabinSouth']
        x0, x1, y0, y1 = fr.body
        # (Its east side's junk is the menu's, side_mid.) A barrel by the trough between the shack and the barber's.
        r.add('Barrel_B', -760.0, 880.0, jitter('trough barrel', 30.0), 'mainStreet', 'small')
    if 'store' in f:
        fr = f['store']
        x0, x1, y0, y1 = fr.body
        # Pruitt's: his notices on both side walls by the street (the menu hangs one), junk against the west wall between
        # its two windows (the shutters at 2.9-3.9 m and 8.7-9.7 m back), stock stacked against the east wall past the
        # buckboard's crates.
        r.local(fr, 'Handbills_A', x1 - 110.0, y0 - 1.0, -90.0, 'wall', mount='wall', lift=145.0, group='store')
        r.local(fr, 'Vignette_Alley_A', 85.0, y0 - 4.0, -90.0, 'small', against='store', along=True, group='store',
                alts=[(40.0, y0 - 4.0, -90.0)])
        r.local(fr, 'CrateStack_B', x1 - 400.0, y1 + 50.0, 90.0 + jitter('store east', 6.0), 'small',
                group='store', against='store', along=True, alts=[(x1 - 520.0, y1 + 50.0, 90.0)])
    if 'undertaker' in f:
        fr = f['undertaker']
        x0, x1, y0, y1 = fr.body
        # Bright & Daughter's: a ladder against the east wall, coffin timber crated off the walk's east end.
        r.local(fr, 'Ladder_A', x0 + 180.0, y1 + 2.0, 90.0, 'small', against='undertaker', along=True,
                group='undertaker', alts=[(x0 + 320.0, y1 + 2.0, 90.0)])
        r.local(fr, 'CrateStack_A', x1 + 100.0, y1 + 160.0, 12.0, 'small', group='undertaker',
                alts=[(x1 + 160.0, y1 + 220.0, -10.0)])
        r.local(fr, 'Barrel_B', x1 + 90.0, y1 + 274.0, 0.0, 'small', group='undertaker',
                alts=[(x1 + 190.0, y1 + 120.0, 0.0)])
    if 'penLaneWest' in f:
        # The shotgun house (empty): boards fallen off its porch into the weeds.
        r.local(f['penLaneWest'], 'FallenBoards_A', 300.0, -230.0, 25.0, 'flat', group='penLane',
                alts=[(320.0, 330.0, -20.0)])
    if 'cottageSouthWest' in f:
        # The shack (empty): boards down in front of it, clear of the water cart.
        r.local(f['cottageSouthWest'], 'FallenBoards_A', 150.0, -150.0, 70.0, 'flat', group='shack',
                alts=[(160.0, 130.0, 110.0)])
    if 'homeB' in f:
        fr = f['homeB']
        # The dogtrot (lived): the broom by the breezeway (the menu puts the rocker and the washtub on the porch's
        # ends). Its back is the stock pens' fence 3.4 m off: the yard's pieces by hand, between them. Kit pieces only:
        # homeB stands in the Farm view at 56 m, and these types are drawn there already (the farm's own).
        r.local(fr, 'BroomBucket', 4.0, 170.0, 0.0, 'porch', mount='porch', lift=45.0, against='homeB', group='homeB')
        r.local(fr, 'FirewoodStack', -560.0, -300.0, 0.0, 'big', group='homeB', against='homeB',
                alts=[(-560.0, 330.0, 0.0)])
        r.local(fr, 'ChoppingBlock', -690.0, -260.0, 20.0, 'small', group='homeB', alts=[(-690.0, 260.0, -10.0)])
        r.local(fr, 'LaundryLine', -720.0, 330.0, 0.0, 'big', group='homeB', alts=[(-700.0, -330.0, 0.0)])
    if 'homeI' in f:
        fr = f['homeI']
        # The L house (empty): junk against the wing's gable to the street.
        r.local(fr, 'Vignette_Alley_A', 307.0, -370.0, 0.0, 'small', against='homeI', along=True, group='homeI')


def dress(mt, d, skip=()):
    """The rules' pieces for the design d (after its hand-made ones)."""
    r = Rules(mt, d, skip)
    r.frames = frames(mt, d.world)
    rule_spots(d.world, mt)
    street(r)
    extras(r)
    for f in r.frames:
        r.menu(f)
    for f in r.frames:
        r.fringe(f)
    # Rings of dry grass at the street lamps' and hitch rails' feet (one in SKIP_EVERY left bare).
    for p in list(d.pieces):
        if p.mesh in ('LampPost', 'LampPost_C') and p.group in ('mainStreet', 'depot'):
            if crc(p.x, p.y, 'ring') % SKIP_EVERY:
                r.ring(p, p.group)
        elif p.mesh == 'HitchRail' and p.group == 'mainStreet':
            r.rail_rings(p, p.group)
    return r


# ---------------------------------------------------------------------------
# The walks' and lamps' variants
# ---------------------------------------------------------------------------

# The runs that take variants (WALK_SWAPS): the Gilt Spur's and Bright & Daughter's. The assay office's and the
# barber's runs stand 68-78 m from the Farm view's camera (over its line), inside the walks' draw distance (FENCE_CULL:
# 96 m on Medium), and a new type there would be drawn in it; the variants' own cull (WALK_VARIANT_CULL, 90 m) keeps the
# Gilt Spur's out of it.
WALK_RUNS = ('saloon', 'undertaker')
# The variants each run takes: (run, the section's place along it from its first, the variant). One a run, never two
# alike in a row; on the section with the least standing on it (a plank gone under a chair would show through): the Gilt
# Spur's at its door (a crate of empties on it), Bright & Daughter's in front of Tilly's window (nothing on it).
WALK_SWAPS = (('saloon', 1, 'Boardwalk_4m_Patched'), ('undertaker', 0, 'Boardwalk_4m_Patched'))


def walk_variants(d, skip=()):
    """Swap the chosen 4 m sections for their variants in place (same spot, same id: only the mesh changes)."""
    runs = {}
    for p in d.pieces:
        if p.mount == 'walk' and p.mesh == 'Boardwalk_4m' and p.group in WALK_RUNS:
            runs.setdefault(p.group, []).append(p)
    swapped = 0
    for group, k, mesh in WALK_SWAPS:
        if mesh not in CHOSEN or mesh in skip or k >= len(runs.get(group, [])):
            continue
        runs[group][k].mesh = mesh
        swapped += 1
    return swapped


# Street lamps that take a variant (the plan: 3 of the 9): (the lamp post's spot as make_town's main_street() puts it,
# the variant, the building whose corner a wall lamp hangs on, the corner in its frame, the wall lamp's yaw from the
# building's). A wall lamp hangs ~3 m up on the corner nearest the old post; a crossing lamp stands on the old post's foot.
LAMP_SWAPS = (
    ((482.0, 2830.0), 'LampPost_B', 'saloon', 'front_left'),       # the Gilt Spur's front west corner, under its balcony
    ((-482.0, 1760.0), 'LampPost_B', 'cabinSouth', 'front_left'),  # the barber's front east corner, by Pen Lane's mouth
    ((-482.0, 4660.0), 'LampPost_C', None, None),                  # the fields path's mouth: a crossing lamp
)
WALL_LAMP_LIFT = 300.0


def lamp_swap(mt, d, x, y):
    """The variant for a street lamp make_town would stand at (x, y): (mesh, None) for one on the same foot, (mesh, (wx,
    wy, yaw, host, lift)) for a corner lamp, or None (the plain lamp post)."""
    for (sx, sy), mesh, host, corner in LAMP_SWAPS:
        if abs(sx - x) > 1.0 or abs(sy - y) > 1.0 or mesh not in CHOSEN or mesh in mt.SKIP:
            continue
        if host is None:
            return mesh, None
        b = d.world.buildings.get(host)
        spec = mt.KIT.get(b.model) if b is not None else None
        if not spec:
            return None
        x0, x1, y0, y1 = spec['walls'][0]
        # LampPost_B's building fills its own -X -Y quarter, the arm leaving between +X and +Y. At the front's +Y corner
        # the building lies behind (-X) and to its -Y: turned with the building. At the -Y corner: turned -90.
        if corner == 'front_left':
            lx, ly, lyaw = x1, y1, 0.0
        else:
            lx, ly, lyaw = x1, y0, -90.0
        wx, wy = b.at(lx, ly)
        return mesh, (wx, wy, (b.yaw + lyaw) % 360.0, host, WALL_LAMP_LIFT)
    return None


# ---------------------------------------------------------------------------
# The report: the density check and the draws
# ---------------------------------------------------------------------------

STRUCTURAL = ('Boardwalk_', 'Awning_', 'LampPost', 'SignBracket')
GROUND_STRIPS = ('WeedFringe_', 'PlankCrossing_', 'StrawLitter_', 'FallenBoards_')
MAIN_STREET = dict(y0=-1100.0, y1=7200.0, x=1800.0, front=1100.0, bin=1000.0)
# The views to estimate (views.json's and the plan's new ones; x, y cm, yaw, fov 80 across).
VIEWS = (('MainStreet', 60.0, -1600.0, 90.0), ('MainStreetMid', 0.0, 2600.0, 90.0), ('MainStreetEast', 0.0, 6800.0, 270.0),
         ('PenLane', -800.0, 2060.0, 180.0), ('BackLotsNorth', 2100.0, 1500.0, 90.0), ('YardRoad', 300.0, 7600.0, 80.0),
         ('Farm', -3900.0, -5100.0, 64.0), ('ChapelOverTown', 5300.0, -1900.0, 137.0))
FOV = 80.0
SHADOW_REACH = 6000.0     # Medium's sun shadows (Smog's shadowDistance; Dusk's is 60 m too)


def is_clutter(mesh):
    return not mesh.startswith(STRUCTURAL) and not mesh.startswith(GROUND_STRIPS)


def density(pieces):
    """Main Street's clutter per 10 m bin per side, gate to east: all in the zone and the fronts' (within 3 m of the
    facades), as pieces and as props (a vignette or a stack is several)."""
    ms = MAIN_STREET
    nbins = int((ms['y1'] - ms['y0']) // ms['bin'])
    table = {side: {'all': [0] * nbins, 'front': [0] * nbins, 'props': [0] * nbins} for side in ('north', 'south')}
    for p in pieces:
        x, y = p['at']
        if not (ms['y0'] <= y < ms['y1'] and abs(x) <= ms['x']) or not is_clutter(p['mesh']):
            continue
        side = 'north' if x > 0 else 'south'
        k = min(int((y - ms['y0']) // ms['bin']), nbins - 1)      # the last 3 m (to the yard road) with the bin before
        table[side]['all'][k] += 1
        if abs(x) <= ms['front']:
            table[side]['front'][k] += 1
        table[side]['props'][k] += KIT[p['mesh']][5] if p['mesh'] in KIT else 1
    return table


def zones_table(frames, pieces):
    """Each dressed building's clutter by zone in its own frame (front: within 3.5 m before its front, porch or walk
    included; side: beside it within 4 m; back: within 12 m behind it), each piece counted for its nearest building:
    the plan's check that every building has a story in front and the lived homes 3+, the empty ones 2+."""
    out = {f.b.id: {'front': 0, 'side': 0, 'back': 0, 'kind': f.kind} for f in frames}
    for p in pieces:
        if (not is_clutter(p['mesh']) and p['mesh'] != 'FallenBoards_A') or p['mesh'] in ('HitchRail', 'WaterTrough'):
            continue        # (an empty house's fallen boards are its story; the other ground cover isn't counted)
        best = None
        for f in frames:
            x0, x1, y0, y1 = f.body
            lx, ly = f.b.local_xy(*p['at'])
            if x1 <= lx <= x1 + 350.0 and y0 - 150.0 <= ly <= y1 + 150.0:
                zone, dist = 'front', lx - x1
            elif x0 - 1200.0 <= lx < x0 and y0 - 300.0 <= ly <= y1 + 300.0:
                zone, dist = 'back', x0 - lx
            elif x0 - 100.0 <= lx <= x1 + 100.0 and (y1 < ly <= y1 + 400.0 or y0 - 400.0 <= ly < y0):
                zone, dist = 'side', min(abs(ly - y1), abs(ly - y0))
            else:
                continue
            if best is None or dist < best[2]:
                best = (f.b.id, zone, dist)
        if best:
            out[best[0]][best[1]] += 1
    return out


def in_view(view, x, y, reach):
    _, cx, cy, yaw = view
    dx, dy = x - cx, y - cy
    dist = math.hypot(dx, dy)
    if dist > reach or dist < 1.0:
        return dist <= reach and dist < 1.0
    ang = (math.degrees(math.atan2(dy, dx)) - yaw + 540.0) % 360.0 - 180.0
    # A piece's own size takes it into the frame a little past the edge.
    return abs(ang) <= FOV * 0.5 + math.degrees(math.atan2(150.0, dist))


def draws(pieces, cull_of, slots_of, shadow_of, new_types, shadowless_now):
    """Per view: the new types drawn (an instance in the frame within Medium's distance), their added draws (low: a
    batch a slot, and one shadow cascade a slot where it casts; high: three LODs where the instances span them and two
    cascades), the existing types made shadowless (saved shadow draws), the instances added."""
    out = []
    for view in VIEWS:
        types, low, high, inst = {}, 0, 0, 0
        for p in pieces:
            mesh = p['mesh']
            reach = cull_of(mesh) * MEDIUM_DISTANCE
            x, y = p['at']
            if not in_view(view, x, y, reach):
                continue
            if p.get('_new'):
                inst += 1
            if mesh in new_types:
                dist = math.hypot(x - view[1], y - view[2])
                t = types.setdefault(mesh, {'n': 0, 'near': 1e18, 'far': 0.0})
                t['n'] += 1
                t['near'] = min(t['near'], dist)
                t['far'] = max(t['far'], dist)
        for mesh, t in types.items():
            s = slots_of(mesh)
            lods = 1 + (t['far'] > 1500.0) + (t['far'] > 3000.0) if not mesh.startswith('WagonFreight') else 1
            shadow = shadow_of(mesh) and t['near'] <= SHADOW_REACH
            low += s + (s if shadow else 0)
            high += s * lods + (2 * s * lods if shadow else 0)
        saved_low = saved_high = 0
        for mesh in shadowless_now:
            if any(q['mesh'] == mesh and in_view(view, q['at'][0], q['at'][1], min(cull_of(mesh) * MEDIUM_DISTANCE,
                                                                                   SHADOW_REACH)) for q in pieces):
                s = slots_of(mesh)
                saved_low += s
                saved_high += 2 * s * 2
        out.append((view[0], sorted(types), low, high, saved_low, saved_high, inst))
    return out
