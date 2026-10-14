"""Ransom's Rest's town dressing (the "make it alive" polish, concept board A_town): town.json, the pieces
Tools/Unreal/build_area_dressing.py places instanced when layout.json's level.town names it.

Today every porch, yard and storefront of Ransom's Rest stands bare in raw grass. This script lays the town's small
stories out of the existing kit and the new frontier clutter (Art/Models/Props/FrontierClutter.py, look sheet 42),
building by building, in each building's own frame, so a vignette moves with its house:
- Main Street: boardwalks along the false fronts that have none (Boardwalk.py: the walk's top at the floors' 0.38 m), an
  awning over the sheriff's walk (the only front whose sign clears the awning's 3.3 m ledger), street lamps at the walks'
  ends and in the gaps, hitch rails and troughs down both sides, benches, barrels, two chairs, a spittoon and a crate of
  empties under the Gilt Spur's balcony with faded bunting on its rail, blade signs on brackets at Pruitt's, the barber's
  and the hotel, and a buckboard parked at Pruitt's with flour sacks and crates left half unloaded.
- The homes, each its own story (the townsfolk stay unseen: life shows in things left about): the Hasketts' saltbox
  (flower boxes, a lantern by the door, broom and bucket; out back the chopping block, a washtub, a laundry line, a kitchen
  garden and a chicken yard); the Mercers' porch house (a rocker with a shawl and a cat's spot on the porch, a horseshoe on
  the post by the steps, a washtub and a chair on the side porch; out back a pump, milk cans, laundry and a garden); the
  company manager's shut gallery house (a rocker left behind, a packing crate, its flower boxes gone to seed); the empty
  shack (a wheelbarrow left out); Pen Lane's empty shotgun house (a barrel lying in the weeds); Amos's empty cabin (his
  bench, the winter's wood he never burned, the axe still in the block); the dugout (laundry, a garden).
- Tilly's workshop (firewood for its stove, a rain barrel, a wheelbarrow, a horseshoe over the doors); the depot's
  platform past the station house (freight: a hand cart, crates, milk cans, flour, drums); Ransom Farm (Delia's kitchen
  garden, a chicken yard, her laundry and washtub, the woodpile and the block, milk cans at the barn, a wheelbarrow).

The new clutter waits for the user's approval: its pieces are planned and checked exactly as the rest (they keep their
ground) but written to town.json's deferredPieces, not its pieces, until their mesh is named in APPROVED (or on the
command line: --approve Name,Name). Approving moves nothing: the same spots, the same ids. A piece that stands on or
hangs from a deferred piece (the crate in the buckboard's bed) waits with it. The small animals (two chicken yards, a
porch cat) are spots, pending too.

Every piece is checked, as Skyreach's make_town.py checks Crossroads Town (highest priority first: walks and what's
fixed to buildings, lamps, rails and troughs, the big pieces, the small ones): off the roads' surfaces (the walks, rails,
troughs and lamp posts may stand on Main Street's very edge, as they're built to) and the worn trails; off every
building's walls, porches, posts, steps and its own dressing (a porch piece stands on its porch, inside its rails), and
off the ways to its doors; off the gameplay spots (the spawn, the landings, the roosts and the line to them from their 9 m
find circle, the notice board's front, the gunsmith's bench, the graves, the memorial, and with the probe every story
actor, speaker, vendor's hatch, town life door, mission prop, encounter's spawn points and patrol routes, and the
lootable world's chests and breakables; without it the ones these scripts place by rule); off the layout's obstacles
(fences, walls, rocks, ruins, trees, the dressing's prop groups) and the dressing's own instances; off each other; on
walkable ground. A piece in the way is nudged (round its spot, or along its wall for one fixed to a building) or dropped;
every nudge and drop is printed, and the result is checked again on its own at the end.

It reads layout.json, Saved/MeshBounds.json (the kit's real bounds), Art/Levels/area_shape.py's BUILDINGS (sizes of
buildings it has no model frame for) and, when Main has run it, Saved/RRTownProbe.json (Saved/rr_town_probe.py, read-only:
the RR buildings' and new kit's real bounds and sockets, every actor near the town with its folder and class, the
dressing's instances, the spawners' points, the terrain's heights on a 50 cm grid). Without the probe it says so and
checks against what the layout and the scripts' rules give; with it, the lifts of what stands on a porch or hangs on a
wall are measured from the building's own height, each walk is raised or sunk (sink) so its top meets its building's
floor, the ground under every piece is checked, and the dressing's instances and breakables that are this town's own
from the build before (the last town.json's spots) aren't obstacles.

What stands on a walk, a wagon's bed or another piece names it (hangFrom), so it follows wherever the dressing stands
it. The street lamps (and the porch lanterns, once approved) carry SOCKET_Light: town.json's lights list them, lit only
at dusk (no light by day), for the build to put a light at each.

Run with Blender's bundled Python (the standard library only; Blender itself isn't needed):
    "C:\\Program Files\\Blender Foundation\\Blender 4.4\\4.4\\python\\bin\\python.exe" Art/Levels/RansomsRest/make_town.py
The same inputs always give the same town.json (no randomness; ids follow the design's order; the output is sorted).
"""
import ast
import json
import math
import os
import sys
from collections import Counter, defaultdict

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.normpath(os.path.join(HERE, '..', '..', '..'))
LAYOUT_PATH = os.path.join(HERE, 'layout.json')
OUT_PATH = os.path.join(HERE, 'town.json')
BOUNDS_PATH = os.path.join(REPO, 'Saved', 'MeshBounds.json')
PROBE_PATH = os.path.join(REPO, 'Saved', 'RRTownProbe.json')
SHAPE_PATH = os.path.join(REPO, 'Art', 'Levels', 'area_shape.py')

# The new clutter (FrontierClutter.py) the user has approved: their pieces go into town.json's pieces.
APPROVED = ('BottleCrate', 'BroomBucket', 'Buckboard', 'Bunting', 'ChickenCoop', 'ChoppingBlock', 'FlourSacks', 'FlowerBox',
            'HandPump', 'Horseshoe', 'KitchenGarden', 'MilkCans', 'PorchChair', 'PorchLantern', 'RockingChair',
            'RockingChair_Shawl', 'SignBoard_Barber', 'SignBoard_Hotel', 'SignBoard_Pruitts', 'SignBracket', 'Spittoon',
            'Washtub')  # all of look sheet 42, the user's yes (2026-10-14)

# ---------------------------------------------------------------------------
# The kit
# ---------------------------------------------------------------------------

# The new clutter, measured from FrontierClutter.py (Unreal cm in the model's own frame: X its front, the Blender
# model's -Y; Y the Blender model's -X): its ground box (x0, x1, y0, y1), height, whether it collides, and how it stands
# (prop: tilts a little with the ground; upright: plumb; flat: lies on it; wall and air: fixed to a building, lifted).
NEW_KIT = {
    'RockingChair': ((-61.0, 47.0, -27.0, 27.0), 110.0, True, 'prop'),
    'RockingChair_Shawl': ((-61.0, 47.0, -27.0, 27.0), 112.0, True, 'prop'),
    'PorchChair': ((-25.0, 20.0, -24.0, 24.0), 98.0, True, 'prop'),
    'Washtub': ((-40.0, 40.0, -40.0, 40.0), 60.0, True, 'prop'),
    'FlowerBox': ((0.0, 23.0, -47.0, 47.0), 45.0, False, 'wall'),
    'PorchLantern': ((0.0, 34.0, -9.0, 9.0), 45.0, False, 'wall'),
    'BroomBucket': ((0.0, 45.0, -35.0, 35.0), 122.0, True, 'prop'),
    'Horseshoe': ((0.0, 2.0, -7.0, 7.0), 14.0, False, 'wall'),
    'ChoppingBlock': ((-50.0, 50.0, -50.0, 50.0), 75.0, True, 'upright'),   # its chips and billets round the block
    'HandPump': ((-91.0, 63.0, -63.0, 63.0), 145.0, True, 'upright'),
    'MilkCans': ((-26.0, 48.0, -40.0, 34.0), 62.0, True, 'prop'),
    'KitchenGarden': ((-118.0, 110.0, -150.0, 166.0), 175.0, True, 'flat'),
    'ChickenCoop': ((-114.0, 114.0, -124.0, 124.0), 150.0, True, 'upright'),
    'Spittoon': ((-13.0, 13.0, -13.0, 13.0), 18.0, True, 'prop'),
    'BottleCrate': ((-20.0, 24.0, -40.0, 44.0), 30.0, True, 'prop'),
    'FlourSacks': ((-62.0, 72.0, -90.0, 56.0), 86.0, True, 'prop'),
    'Buckboard': ((-225.0, 225.0, -82.0, 82.0), 140.0, True, 'prop'),
    'SignBracket': ((0.0, 96.0, -3.0, 3.0), 62.0, False, 'wall'),
    'SignBoard_Pruitts': ((-44.0, 44.0, -2.0, 2.0), 48.0, False, 'air'),
    'SignBoard_Barber': ((-44.0, 44.0, -2.0, 2.0), 48.0, False, 'air'),
    'SignBoard_Hotel': ((-44.0, 44.0, -2.0, 2.0), 48.0, False, 'air'),
    'Bunting': ((-2.0, 3.0, -300.0, 300.0), 60.0, False, 'air'),
}
BUCKBOARD_BED = (-93.0, 80.0)     # the bed's middle along its X and its boards' top (cm): the report line's 0.93 m
SIGN_SOCKET = (55.0, -5.35)       # SignBracket's SOCKET_Sign: out along its arm, under the arm (cm)

# What of a piece stands on the ground, where its bounds reach far over it (cm, own frame): a lamp's arm and an awning's
# roof are overhead, only the post is in the way.
GROUND_BOX = {
    'LampPost': (-12.5, 12.5, -12.5, 12.5),
    'Awning_4m': (152.0, 172.0, -10.0, 10.0),
    'Awning_Post': (152.0, 172.0, -10.0, 10.0),
}
# The existing kit's way of standing (the dressing's town_kind() gives the rest).
UPRIGHT = ('Boardwalk_4m', 'Boardwalk_2m', 'Boardwalk_Steps', 'Awning_4m', 'Awning_Post', 'LampPost', 'HitchRail',
           'FirewoodStack', 'LaundryLine')
# The kit's existing meshes the dressing has no size for yet (build_area_dressing.PIECES: Main adds them).
NEEDS_PIECES = ('Boardwalk_4m', 'Boardwalk_2m', 'Boardwalk_Steps', 'Awning_4m', 'Awning_Post')
WALK_TOP = 38.0                   # the boardwalk's top (looter_town.DECK_TOP), the false fronts' floors
PLATFORM_TOP = 40.0               # the depot platform's top (DepotPlatform_4m)

# Dusk lights on the pieces that carry SOCKET_Light (VillageProps.py: the lamp post's lantern, 0.64 m out along its
# front and 2.21 m up; FrontierClutter.py: the porch lantern's glass, about 0.29 m out and 0.18 m under its plate):
# candelas by day and at dusk, reach (cm).
LIGHTS = {
    'LampPost': dict(offset=(64.0, 0.0, 221.0), day=0.0, dusk=8.0, reach=650.0),
    'PorchLantern': dict(offset=(29.0, 0.0, -18.0), day=0.0, dusk=4.0, reach=350.0),
}

# What build_area_dressing.PIECES needs for the existing kit's meshes this town brings into the level (its size as it
# stands: along its Y, across its X, up; cull), and where a footprint's middle is off its pivot (FOOTPRINT_AHEAD along
# its X, FOOTPRINT_RIGHT along its Y): the report prints them, with the new clutter's, for Main to paste. town_pose()
# reads the ground at the pivot and half these sizes round it; align_walks() predicts the walks' heights the same way.
DRESSING_PIECES = {
    'Boardwalk_4m': ((400, 303, 50), 'FENCE_CULL', 0.0, -200.0),
    'Boardwalk_2m': ((200, 303, 50), 'FENCE_CULL', 0.0, -100.0),
    'Boardwalk_Steps': ((40, 302, 37), 'FENCE_CULL', 0.0, -16.0),
    'Awning_4m': ((20, 20, 360), 'FENCE_CULL', 162.0, 0.0),        # its post on the ground; the roof is overhead
    'Awning_Post': ((20, 20, 277), 'FENCE_CULL', 162.0, 0.0),
}
UPRIGHT_SINK = 40.0      # build_area_dressing's: how far an upright piece may sink to its lowest side
WALK_SINK_MOST = 30.0    # a walk raised or sunk more than this to meet its building's floor is warned of

# ---------------------------------------------------------------------------
# The buildings in their own frames (cm; X their front, the pivot's frame), from the model scripts
# (Art/Models/Buildings: Homestead.py's homes, pivot their front door; FalseFronts.py, pivot the walls' middle;
# FrontsRR.py, DepotRR.py, WorkshopBright.py, pivot the door; Farmhouse.py, Barn.py, pivot the middle):
#   walls: what's solid from the ground (boxes x0, x1, y0, y1); decks: porches ((box), floor height): only a porch piece
#   stands there; steps; posts (x, y): porch and balcony posts; rails: a porch's railings; builtin: the model's own
#   dressing (benches, barrels, woodpiles, a fallen post); doors: (x, y on the face, way out dx, dy, width, how far the
#   way runs); spots: places the story keeps clear (Tilly's window, where the player stands to talk).
# ---------------------------------------------------------------------------

KIT = {
    'RR_Home_A': dict(        # the saltbox (Hasketts)
        walls=[(-645.0, 5.0, -365.0, 365.0)], steps=[(0.0, 70.0, -78.0, 78.0)],
        builtin=[(0.0, 50.0, 140.0, 270.0), (-722.0, -658.0, -347.0, -283.0), (-690.0, -644.0, 115.0, 305.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 100.0, 220.0), (-645.0, -187.0, -1.0, 0.0, 95.0, 150.0)]),
    'RR_Home_H': dict(        # the porch house (Mercers)
        walls=[(-685.0, 5.0, -345.0, 345.0)],
        decks=[((0.0, 200.0, -545.0, 350.0), 50.0), ((-690.0, 0.0, -545.0, -340.0), 50.0)],
        steps=[(200.0, 262.0, -85.0, 85.0)],
        posts=[(188.0, 335.0), (188.0, 105.0), (188.0, -105.0), (188.0, -528.0), (-225.0, -528.0), (-455.0, -528.0),
               (-675.0, -528.0)],
        rails=[(180.0, 196.0, 112.0, 330.0), (180.0, 196.0, -536.0, -112.0), (-690.0, 196.0, -536.0, -520.0)],
        builtin=[(-662.0, -598.0, 353.0, 417.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 100.0, 412.0), (-347.0, -345.0, 0.0, -1.0, 95.0, 190.0)]),
    'RR_Home_D': dict(        # the gallery house (the company manager's, shut)
        walls=[(-545.0, 5.0, -405.0, 405.0)], decks=[((0.0, 200.0, -410.0, 410.0), 55.0)],
        steps=[(200.0, 262.0, -90.0, 90.0)], posts=[(188.0, 395.0), (188.0, 130.0), (188.0, -130.0), (188.0, -395.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 100.0, 412.0), (-545.0, 167.0, -1.0, 0.0, 95.0, 150.0)]),
    'RR_Home_J': dict(        # the shack (empty)
        walls=[(-365.0, 5.0, -215.0, 215.0)], posts=[(115.0, 92.0), (115.0, -92.0)],
        builtin=[(-30.0, 130.0, -375.0, -215.0), (-372.0, -308.0, -280.0, -216.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 90.0, 150.0)]),
    'RR_Home_C': dict(        # the shotgun house (empty, Pen Lane)
        walls=[(-965.0, 5.0, -124.0, 276.0)], decks=[((0.0, 180.0, -124.0, 276.0), 54.0)],
        steps=[(180.0, 242.0, -64.0, 56.0)], posts=[(168.0, -107.0)], builtin=[(55.0, 130.0, 81.0, 241.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 92.0, 392.0)]),
    'RR_Home_F': dict(        # the stone house (empty, Pen Lane), its woodshed falling against its side
        walls=[(-545.0, 25.0, -345.0, 345.0), (-480.0, -60.0, -560.0, -340.0)], steps=[(0.0, 45.0, -75.0, 75.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 100.0, 150.0)]),
    'RR_Home_G': dict(        # the claim cabin (Amos's, empty), its lean-to
        walls=[(-445.0, 3.0, -255.0, 255.0), (-445.0, -70.0, -485.0, -250.0)], steps=[(0.0, 40.0, -60.0, 60.0)],
        builtin=[(-92.0, -28.0, 260.0, 324.0)], doors=[(0.0, 0.0, 1.0, 0.0, 90.0, 150.0)]),
    'RR_Home_E': dict(        # the dugout: the mound, the arbor's poles, its bench, barrel, woodpile and block
        walls=[(-862.0, 25.0, -605.0, 605.0)], posts=[(230.0, 160.0), (230.0, -160.0), (30.0, 160.0), (30.0, -160.0)],
        builtin=[(2.0, 40.0, 100.0, 210.0), (17.0, 73.0, -223.0, -167.0), (2.0, 47.0, 290.0, 470.0),
                 (107.0, 153.0, 297.0, 343.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 90.0, 395.0)]),
    # The assay office's temple front: its pilasters' bases stand on the walk (the bounds reach 0.29 m out).
    'RR_Front_Assay': dict(walls=[(-935.0, 8.0, -358.0, 358.0)], doors=[(0.0, 0.0, 1.0, 0.0, 110.0, 150.0)]),
    'RR_Front_Hotel': dict(
        walls=[(-1240.0, 5.0, -517.0, 517.0)], decks=[((0.0, 300.0, -500.0, 500.0), WALK_TOP)],
        posts=[(282.0, 478.0), (282.0, 160.0), (282.0, -160.0), (282.0, -478.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 150.0, 450.0)]),
    'RR_Front_Barber': dict(walls=[(-835.0, 5.0, -488.0, 118.0)], doors=[(0.0, 0.0, 1.0, 0.0, 110.0, 150.0)]),
    'FalseFront_Saloon': dict(
        walls=[(-735.0, 703.0, -425.0, 425.0)], overhead=[(700.0, 1003.0, -435.0, 435.0)],   # the balcony posts=[(984.0, 400.0), (984.0, 125.0), (984.0, -125.0), (984.0, -400.0)],
        doors=[(700.0, 0.0, 1.0, 0.0, 130.0, 450.0), (-700.0, 150.0, -1.0, 0.0, 100.0, 150.0)]),
    'FalseFront_Store': dict(
        walls=[(-692.0, 650.0, -488.0, 488.0)], decks=[((650.0, 950.0, -470.0, 470.0), WALK_TOP)],
        posts=[(932.0, 448.0), (932.0, 145.0), (932.0, -145.0), (932.0, -448.0)],
        builtin=[(-850.0, -637.0, -255.0, 95.0)],     # the store backlot's lean-to on its back wall (dressing GROUPS)
        doors=[(650.0, 0.0, 1.0, 0.0, 130.0, 450.0), (-650.0, 172.0, -1.0, 0.0, 105.0, 150.0)]),
    'FalseFront_Sheriff': dict(
        walls=[(-535.0, 505.0, -350.0, 350.0)],        # the facade at 5 m (its trim reaches 5.29: on the walk)
        doors=[(500.0, 0.0, 1.0, 0.0, 100.0, 450.0), (-500.0, 105.0, -1.0, 0.0, 100.0, 150.0)]),
    'FalseFront_Undertaker': dict(
        walls=[(-738.0, 610.0, -366.0, 366.0)],        # the facade at 6 m (its cornice and urns overhead to 6.27)
        doors=[(600.0, 0.0, 1.0, 0.0, 110.0, 450.0), (-600.0, 122.0, -1.0, 0.0, 105.0, 200.0)],
        spots=[("Tilly's window (where the player stands to talk to her)", (600.0, 760.0, 105.0, 285.0))]),
    'RR_Workshop_Bright': dict(
        walls=[(-700.0, 10.0, -800.0, 200.0)], steps=[(0.0, 75.0, -100.0, 100.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 160.0, 225.0)]),
    'RR_Depot': dict(
        walls=[(-861.0, 0.0, -996.0, 496.0)],
        doors=[(0.0, 0.0, 1.0, 0.0, 120.0, 200.0), (0.0, -700.0, 1.0, 0.0, 190.0, 200.0)]),
    'Farmhouse_Ransom': dict(  # Delia's porch is the story's (her door, the plate, Hob on the rail): nothing of ours
        walls=[(-373.0, 315.0, -425.0, 425.0), (310.0, 598.0, -460.0, 460.0)],
        doors=[(598.0, 0.0, 1.0, 0.0, 100.0, 170.0)]),
    'Barn_Ransom': dict(
        walls=[(-650.0, 605.0, -457.0, 457.0)], overhead=[(605.0, 735.0, -200.0, 200.0)],   # its hoist beam
        doors=[(600.0, 0.0, 1.0, 0.0, 320.0, 285.0), (-650.0, 0.0, -1.0, 0.0, 120.0, 150.0)]),
    'RR_Well': dict(walls=[(-110.0, 160.0, -110.0, 300.0)]),           # the crib, its trough and the sweep's post
    'RR_Outhouse': dict(walls=[(-150.0, 5.0, -75.0, 75.0)], doors=[(0.0, 0.0, 1.0, 0.0, 80.0, 150.0)]),
    # The train at the platform (area_shape's sizes, their length along their own X: the track).
    'RR_CoalStage': dict(walls=[(-269.0, 269.0, -277.0, 230.0)]),     # DepotRR_report.json's bounds
    'HearseCar': dict(walls=[(-515.0, 515.0, -145.0, 145.0)]),
    'PassengerCar': dict(walls=[(-615.0, 615.0, -145.0, 145.0)]),
    'Locomotive_B': dict(walls=[(-440.0, 440.0, -140.0, 140.0)]),
    'DepotPlatform_4m': dict(decks=[((-200.0, 200.0, -400.0, 0.0), PLATFORM_TOP)]),
    'DepotPlatform_End': dict(walls=[(-200.0, 200.0, -246.0, 0.0)]),   # its ramp: nothing stands on it
    'DeadTree_A': dict(walls=[(-40.0, 40.0, -40.0, 40.0)]),          # its trunk (DeadTree.py; the crown is overhead)
    'NoticeBoard': dict(walls=[(-50.0, 50.0, -110.0, 110.0)],
                        spots=[("the notice board's front", (50.0, 200.0, -110.0, 110.0))]),
}
# Placements that aren't solid buildings: spots (handled with the gameplay spots) or nothing to keep off.
NOT_BUILDINGS = ('PlayerStart', 'Landing', 'Track_Straight_12m', 'Track_Straight_6m', 'TownGate')

# ---------------------------------------------------------------------------
# Clearances (cm)
# ---------------------------------------------------------------------------

SPOT_CLEAR = 150.0       # off a story actor or a gameplay spot
SPAWN_CLEAR = 250.0      # off an encounter's spawn point or patrol route (a walk: WALK_SPAWN_CLEAR)
WALK_SPAWN_CLEAR = 100.0
LOOT_CLEAR = 120.0       # off a chest or a breakable of the lootable world
ROOST_CLEAR = 250.0      # off a roost's mast
ROOST_FIND = 900.0       # its find circle: nothing tall and wide inside it (it would cut the line to the mast)
ROOST_TALL, ROOST_WIDE = 180.0, 80.0
GRAVE_CLEAR = 200.0
TRAIL_CLEAR = 100.0      # off a worn trail's line (solid ground pieces)
GAP = 8.0                # between two solid pieces, and from a building's walls
# A road's surface, by what stands near it: how far a piece's box must keep from the road's edge (negative: it may
# stand that far onto it; on Main Street and the yard road only, the street's furniture at its very edge).
ROAD_MARGIN = {'walk': 40.0, 'verge': 10.0, 'street': 40.0, 'big': 60.0, 'small': 40.0}
STREET_EDGE = {'walk': -35.0, 'verge': -35.0, 'street': -60.0}
STREET_ROADS = ('mainStreet', 'yardRoad')
# Layout obstacles by kind: how far a piece keeps from the polygon or path (props: the dressing's groups stand about
# their polygon; measured by the probe's instances when it has run).
OBSTACLE_CLEAR = {'fence': 40.0, 'wall': 80.0, 'props': 250.0, 'boulders': 60.0, 'trees': 60.0, 'ruin': 60.0,
                  'outcrop': 80.0, 'graves': 120.0, 'cairns': 80.0, 'biers': 120.0}
PROPS_CLEAR_PROBED = 60.0
KEEP_OUT_ZONES = ('orchard', 'sink')
# Walkable ground (with the probe's heights): the most the ground may rise and fall under a piece (cm).
GROUND_RANGE = {'walk': 30.0, 'verge': 40.0, 'street': 25.0, 'big': 25.0, 'small': 18.0, 'flat': 15.0}
# How far a piece may be nudged (cm), by class; fixed ones ('walk', 'wall', 'air') never move.
REACH = {'walk': 0.0, 'wall': 0.0, 'air': 0.0, 'porch': 60.0, 'verge': 150.0, 'street': 150.0, 'big': 250.0,
         'small': 150.0}
STEP = 10.0
CLASSES = ['walk', 'wall', 'air', 'porch', 'verge', 'street', 'big', 'small']

# The story's spots these scripts place by rule, for when the probe hasn't run (cm): Main 3's gate fight
# (build_area_story.GATE_SPOTS, from the gate's middle), Pruitt's hatch (build_area_vendors: 0.95 m out from the
# store's lamp socket, 0.65 m behind its facade), the town life doors (build_area_townlife HOME_DOOR), Delia's door
# (build_area_farm), the depot's platform spot (build_area_missionprops_valley PLATFORM_SPOT, in the station's frame).
GATE_SPOTS = ((600.0, 900.0), (1100.0, 400.0), (300.0, 1500.0), (1300.0, 1300.0))
STORY_RULES = [('store', (810.0, 0.0), "Pruitt's hatch (Vendor_Pruitt)", 200.0),
               ('cottageNorth', (60.0, 0.0), 'the Hasketts\' door (TownLife)', SPOT_CLEAR),
               ('cottageNorthEast', (60.0, 0.0), 'the Mercers\' door (TownLife)', SPOT_CLEAR),
               ('farmhouse', (310.0, 0.0), "Delia's screen door", SPOT_CLEAR),
               ('depot', (180.0, 120.0), "the depot fight's platform spot", SPAWN_CLEAR)]
# Story classes that aren't spots (zones and areas, whose middle means nothing), as build_area_loot keeps them.
NOT_STORY = ('SafeGround', 'PlayableArea', 'StoryLighting', 'LightingState', 'AmbientEmitter', 'HouseLights',
             'StaticMeshActor', 'InstancedProps', 'InstancedScenery', 'NoTrees', 'TriggerBox', 'TriggerVolume')
SPAWNERS = ('EncounterSpawner', 'AmbushSpawner', 'PatrolSpawner')
PASSABLE = ('Bush_', 'Reeds_', 'LilyPads_', 'Flowers_', 'Web_', 'Cocoon_', 'EggSac_', 'Grave_MoundSunken')
BREAKABLE = ('Crate_A', 'Crate_B', 'Barrel_A')   # build_area_loot.BREAKABLE_MESHES


def load(path):
    with open(path, encoding='utf-8') as file:
        return json.load(file)


def area_buildings():
    """area_shape.py's BUILDINGS (length along the front, width, metres), read from that file."""
    with open(SHAPE_PATH, encoding='utf-8') as file:
        tree = ast.parse(file.read())
    for node in tree.body:
        if isinstance(node, ast.Assign) and any(getattr(t, 'id', None) == 'BUILDINGS' for t in node.targets):
            return ast.literal_eval(node.value)
    return {}


# ---------------------------------------------------------------------------
# Plane geometry (cm; yaw in degrees from +X toward +Y, Unreal's)
# ---------------------------------------------------------------------------

def rot(lx, ly, yaw):
    a = math.radians(yaw)
    c, s = math.cos(a), math.sin(a)
    return lx * c - ly * s, lx * s + ly * c


def to_world(x, y, yaw, lx, ly):
    dx, dy = rot(lx, ly, yaw)
    return x + dx, y + dy


def to_local(x, y, yaw, px, py):
    return rot(px - x, py - y, -yaw)


class Box:
    """An oriented rectangle: its middle, its yaw and its half sizes along its own X and Y."""
    __slots__ = ('cx', 'cy', 'yaw', 'c', 's', 'hx', 'hy')

    def __init__(self, cx, cy, yaw, hx, hy):
        self.cx, self.cy, self.yaw, self.hx, self.hy = cx, cy, yaw, max(hx, 0.5), max(hy, 0.5)
        a = math.radians(yaw)
        self.c, self.s = math.cos(a), math.sin(a)

    @staticmethod
    def frame(x, y, yaw, x0, x1, y0, y1):
        """The box (x0, x1, y0, y1) of a frame at (x, y, yaw) in the world."""
        cx, cy = to_world(x, y, yaw, (x0 + x1) * 0.5, (y0 + y1) * 0.5)
        return Box(cx, cy, yaw, (x1 - x0) * 0.5, (y1 - y0) * 0.5)

    def local(self, px, py):
        dx, dy = px - self.cx, py - self.cy
        return dx * self.c + dy * self.s, -dx * self.s + dy * self.c

    def corners(self):
        out = []
        for a, b in ((1, 1), (-1, 1), (-1, -1), (1, -1)):
            lx, ly = a * self.hx, b * self.hy
            out.append((self.cx + lx * self.c - ly * self.s, self.cy + lx * self.s + ly * self.c))
        return out

    def samples(self):
        out = [(self.cx, self.cy)]
        for a, b in ((1, 1), (-1, 1), (-1, -1), (1, -1), (1, 0), (-1, 0), (0, 1), (0, -1)):
            lx, ly = a * self.hx, b * self.hy
            out.append((self.cx + lx * self.c - ly * self.s, self.cy + lx * self.s + ly * self.c))
        return out

    def bbox(self):
        ex = self.hx * abs(self.c) + self.hy * abs(self.s)
        ey = self.hx * abs(self.s) + self.hy * abs(self.c)
        return self.cx - ex, self.cy - ey, self.cx + ex, self.cy + ey

    def scaled(self, f):
        return Box(self.cx, self.cy, self.yaw, self.hx * f, self.hy * f)

    def grown(self, d):
        return Box(self.cx, self.cy, self.yaw, self.hx + d, self.hy + d)

    def contains(self, px, py, pad=0.0):
        lx, ly = self.local(px, py)
        return abs(lx) <= self.hx + pad and abs(ly) <= self.hy + pad


def sat_gap(a, b):
    """How far apart two boxes are along the axis that separates them most (negative: they overlap)."""
    best = -1e18
    for ux, uy in ((a.c, a.s), (-a.s, a.c), (b.c, b.s), (-b.s, b.c)):
        d = abs((b.cx - a.cx) * ux + (b.cy - a.cy) * uy)
        ra = a.hx * abs(a.c * ux + a.s * uy) + a.hy * abs(-a.s * ux + a.c * uy)
        rb = b.hx * abs(b.c * ux + b.s * uy) + b.hy * abs(-b.s * ux + b.c * uy)
        best = max(best, d - ra - rb)
    return best


def point_box_dist(box, px, py):
    lx, ly = box.local(px, py)
    return math.hypot(max(abs(lx) - box.hx, 0.0), max(abs(ly) - box.hy, 0.0))


def seg_point(a, b, p):
    dx, dy = b[0] - a[0], b[1] - a[1]
    l2 = dx * dx + dy * dy
    t = 0.0 if l2 < 1e-9 else max(0.0, min(1.0, ((p[0] - a[0]) * dx + (p[1] - a[1]) * dy) / l2))
    q = (a[0] + dx * t, a[1] + dy * t)
    return q, math.hypot(p[0] - q[0], p[1] - q[1])


def seg_hits_box(box, a, b):
    ax, ay = box.local(*a)
    bx, by = box.local(*b)
    t0, t1 = 0.0, 1.0
    dx, dy = bx - ax, by - ay
    for p, q in ((-dx, ax + box.hx), (dx, box.hx - ax), (-dy, ay + box.hy), (dy, box.hy - ay)):
        if abs(p) < 1e-12:
            if q < 0.0:
                return False
            continue
        r = q / p
        if p < 0.0:
            t0 = max(t0, r)
        else:
            t1 = min(t1, r)
        if t0 > t1:
            return False
    return True


def box_seg_dist(box, a, b):
    if seg_hits_box(box, a, b):
        return 0.0
    best = min(seg_point(a, b, c)[1] for c in box.corners())
    return min(best, point_box_dist(box, *a), point_box_dist(box, *b))


def box_line_dist(box, pts, closed=False):
    seq = list(pts) + ([pts[0]] if closed else [])
    if len(seq) == 1:
        return point_box_dist(box, *seq[0])
    return min(box_seg_dist(box, a, b) for a, b in zip(seq, seq[1:]))


def inside(poly, x, y):
    hit = False
    n = len(poly)
    for i in range(n):
        (x0, y0), (x1, y1) = poly[i], poly[(i + 1) % n]
        if (y0 > y) != (y1 > y) and x < x0 + (y - y0) * (x1 - x0) / (y1 - y0):
            hit = not hit
    return hit


def box_poly_dist(box, poly):
    """0 where the box and the polygon overlap, else their distance."""
    if any(inside(poly, px, py) for px, py in box.samples()) or any(box.contains(px, py) for px, py in poly):
        return 0.0
    return box_line_dist(box, poly, closed=True)


def rnd1(v):
    return round(float(v), 1)


def snake(name):
    out = []
    for i, ch in enumerate(name):
        if ch.isupper() and i and (name[i - 1].islower() or name[i - 1].isdigit()):
            out.append('_')
        out.append(ch.lower())
    return ''.join(out).replace('__', '_')


# ---------------------------------------------------------------------------
# What's measured: the kit's bounds, and the probe
# ---------------------------------------------------------------------------

class Meshes:
    """Every mesh's bounds by name without SM_: Saved/MeshBounds.json, the probe's over it, and the new clutter's."""

    def __init__(self, bounds, probe):
        self.bounds = {k[3:] if k.startswith('SM_') else k: v for k, v in bounds.items()}
        if probe:
            for k, v in probe.get('meshes', {}).items():
                self.bounds[k] = v
        self.sockets = {k: v.get('sockets', {}) for k, v in self.bounds.items()}

    def has(self, mesh):
        return mesh in self.bounds or mesh in NEW_KIT

    def box(self, mesh):
        """Its ground box (x0, x1, y0, y1), own frame, cm."""
        if mesh in GROUND_BOX:
            return GROUND_BOX[mesh]
        if mesh in NEW_KIT:
            return NEW_KIT[mesh][0]
        b = self.bounds[mesh]
        return b['min'][0], b['max'][0], b['min'][1], b['max'][1]

    def height(self, mesh):
        if mesh in NEW_KIT:
            return NEW_KIT[mesh][1]
        b = self.bounds.get(mesh)
        return b['max'][2] if b else 100.0


class Probe:
    """Saved/RRTownProbe.json: the level as built (see Saved/rr_town_probe.py), or nothing."""

    def __init__(self, data):
        self.data = data or {}
        self.ok = bool(data)
        self.grids = self.data.get('heights', [])
        self.actors = self.data.get('actors', [])
        self.by_label = {a['label']: a for a in self.actors}

    def ground(self, x, y):
        """The terrain's height under (x, y), bilinear on the probe's grid, or None."""
        for g in self.grids:
            fx, fy = (x - g['x0']) / g['step'], (y - g['y0']) / g['step']
            if not (0.0 <= fx <= g['nx'] - 1 and 0.0 <= fy <= g['ny'] - 1):
                continue
            i, j = min(int(fx), g['nx'] - 2), min(int(fy), g['ny'] - 2)
            a, b = fx - i, fy - j
            z = g['z']
            vals = [z[j * g['nx'] + i], z[j * g['nx'] + i + 1], z[(j + 1) * g['nx'] + i], z[(j + 1) * g['nx'] + i + 1]]
            if any(v is None for v in vals):
                return None
            return (vals[0] * (1 - a) + vals[1] * a) * (1 - b) + (vals[2] * (1 - a) + vals[3] * a) * b
        return None

    def actor_z(self, label):
        a = self.by_label.get(label)
        return a['at'][2] if a else None


# ---------------------------------------------------------------------------
# The world the pieces must fit in
# ---------------------------------------------------------------------------

class Building:
    """A placement as the pieces see it: its frame and its parts in the world."""

    def __init__(self, pid, model, x, y, yaw, z=None):
        self.id, self.model, self.x, self.y, self.yaw, self.z = pid, model, x, y, yaw, z
        self.walls, self.decks, self.steps, self.posts, self.rails, self.builtin = [], [], [], [], [], []
        self.ways, self.spots, self.overhead = [], [], []
        self.known = False

    def at(self, lx, ly):
        return to_world(self.x, self.y, self.yaw, lx, ly)

    def box(self, x0, x1, y0, y1):
        return Box.frame(self.x, self.y, self.yaw, x0, x1, y0, y1)

    def reach_box(self):
        boxes = self.walls + [d for d, _ in self.decks] + self.steps
        if not boxes:
            return None
        xs, ys = [], []
        for b in boxes:
            x0, y0, x1, y1 = b.bbox()
            xs += [x0, x1]
            ys += [y0, y1]
        return min(xs), min(ys), max(xs), max(ys)


class World:
    def __init__(self, layout, meshes, probe):
        self.layout, self.meshes, self.probe = layout, meshes, probe
        self.notes, self.warnings, self.unframed = [], [], []
        level = layout.get('level', {})
        self.models = level.get('models', {})
        self.places = {p['id']: p for p in layout['placements']}
        self.buildings = {}
        sizes = area_buildings()
        for p in layout['placements']:
            self.add_building(p['id'], p['kind'], p, sizes)
        for p in level.get('props', []):
            self.add_building(p['id'], p['kind'], p, sizes)
        self.roads = [(r['id'], [tuple(q[:2]) for q in r['path']], r['width'] * 0.5, r.get('kind'))
                      for r in layout.get('roads', [])]
        self.trails = []
        for t in layout.get('trails', []):
            pts = []
            for end in t:
                if isinstance(end, str):
                    if end in self.places:
                        pts.append(tuple(self.places[end]['location'][:2]))
                else:
                    pts.append(tuple(end[:2]))
            if len(pts) == 2:
                self.trails.append((f'trail {t[0]}-{t[1]}', pts))
        self.zones = {z['id']: [tuple(q) for q in z['polygon']] for z in layout.get('zones', [])}
        self.obstacles = []
        for o in layout.get('obstacles', []):
            kind = o['kind']
            if kind not in OBSTACLE_CLEAR:
                continue
            clear = OBSTACLE_CLEAR[kind]
            if kind == 'props' and probe.ok:
                clear = PROPS_CLEAR_PROBED
            if 'polygon' in o:
                self.obstacles.append((o['id'], kind, [tuple(q[:2]) for q in o['polygon']], True, clear))
            elif 'path' in o:
                self.obstacles.append((o['id'], kind, [tuple(q[:2]) for q in o['path']], False, clear))
        self.spots = []             # (name, Box or point, clearance, kind)
        self.roosts = []            # (name, (x, y))
        self.instances = []         # (mesh, Box) the dressing's solid pieces already in the level
        self.gameplay_spots()

    # --- Buildings ---

    def add_building(self, pid, kind, p, sizes):
        if kind in NOT_BUILDINGS:
            return
        model = self.models.get(kind, kind)
        x, y = p['location'][:2]
        yaw = p.get('yaw', 0.0)
        b = Building(pid, model, x, y, yaw, self.probe.actor_z(pid))
        spec = KIT.get(model)
        if spec:
            b.known = True
            b.walls = [b.box(*w) for w in spec.get('walls', [])]
            b.decks = [(b.box(*d), z) for d, z in spec.get('decks', [])]
            b.steps = [b.box(*s) for s in spec.get('steps', [])]
            b.posts = [b.box(px - 9.0, px + 9.0, py - 9.0, py + 9.0) for px, py in spec.get('posts', [])]
            b.rails = [b.box(*r) for r in spec.get('rails', [])]
            b.builtin = [b.box(*q) for q in spec.get('builtin', [])]
            for fx, fy, dx, dy, width, reach in spec.get('doors', []):
                cx, cy = b.at(fx + dx * reach * 0.5, fy + dy * reach * 0.5)
                b.ways.append(Box(cx, cy, yaw + math.degrees(math.atan2(dy, dx)), reach * 0.5, width * 0.5 + 40.0))
            for name, (x0, x1, y0, y1) in spec.get('spots', []):
                b.spots.append((name, b.box(x0, x1, y0, y1)))
            b.overhead = [b.box(*q) for q in spec.get('overhead', [])]
            self.check_kit(b)
        elif self.meshes.has(model):
            x0, x1, y0, y1 = self.meshes.box(model)
            s = p.get('scale', 1.0)
            b.walls = [b.box(x0 * s, x1 * s, y0 * s, y1 * s)]
        elif self.probe.by_label.get(pid):
            (ax0, ay0, _), (ax1, ay1, _) = self.probe.by_label[pid]['bounds']
            b.walls = [Box((ax0 + ax1) * 0.5, (ay0 + ay1) * 0.5, 0.0, (ax1 - ax0) * 0.5, (ay1 - ay0) * 0.5)]
        elif model in sizes:
            length, width = sizes[model]
            b.walls = [b.box(-width * 50.0, width * 50.0, -length * 50.0, length * 50.0)]
            self.unframed.append((pid, x, y, f'{pid} ({model}): no model frame or bounds; kept off as '
                                              f'area_shape\'s {length} x {width} m round its pivot'))
        else:
            return
        self.buildings[pid] = b

    def check_kit(self, b):
        """The script-measured frame against the mesh's real bounds (the probe's or MeshBounds'), when known."""
        if b.model not in self.meshes.bounds:
            return
        x0, x1, y0, y1 = self.meshes.box(b.model)
        spec = KIT[b.model]
        boxes = spec.get('walls', []) + [d for d, _ in spec.get('decks', [])] + spec.get('overhead', [])
        if not boxes or b.model in ('DeadTree_A', 'RR_Well', 'NoticeBoard'):
            return
        kx0, kx1 = min(w[0] for w in boxes), max(w[1] for w in boxes)
        ky0, ky1 = min(w[2] for w in boxes), max(w[3] for w in boxes)
        off = max(abs(kx0 - x0), abs(kx1 - x1), abs(ky0 - y0), abs(ky1 - y1))
        if off > 60.0:
            self.notes.append(f'{b.id} ({b.model}): the script frame ({kx0:.0f}..{kx1:.0f}, {ky0:.0f}..{ky1:.0f}) and the '
                              f'mesh bounds ({x0:.0f}..{x1:.0f}, {y0:.0f}..{y1:.0f}) differ by {off:.0f} cm (the bounds '
                              f'take in eaves and roofs; walls and porches are the script\'s)')

    # --- Gameplay spots ---

    def gameplay_spots(self):
        lay = self.layout
        g = lay.get('gameplay', {})
        for p in lay['placements']:
            x, y = p['location'][:2]
            if p['kind'] in ('PlayerStart', 'Landing'):
                self.spots.append((f'{p["id"]} ({p["kind"]})', (x, y), SPOT_CLEAR, 'spot'))
            elif p['kind'].startswith('Grave_'):
                self.spots.append((f'the grave {p["id"]}', (x, y), GRAVE_CLEAR, 'spot'))
            elif p['kind'] == 'TownMemorial':
                b = self.buildings.get(p['id'])
                if b and b.walls:
                    self.spots.append(('the town memorial', b.walls[0], SPOT_CLEAR, 'spot'))
        for r in g.get('roosts', []):
            self.roosts.append((f'the roost {r["id"]}', tuple(r['at'][:2])))
        for bench in g.get('benches', []):
            (x, y), yaw = bench['at'][:2], bench.get('yaw', 0.0)
            box = Box.frame(x, y, yaw, -44.0, 56.0, -102.0, 102.0)
            self.spots.append((f"the gunsmith's bench {bench['id']}", box, SPOT_CLEAR, 'spot'))
        for poster in g.get('posters', []):
            host = self.buildings.get(poster.get('host'))
            if host and 'at' in poster:
                lx, ly = poster['at'][:2]
                self.spots.append((f'the poster {poster["id"]} (keep it readable)', host.box(lx, lx + 120.0, ly - 40.0,
                                                                                            ly + 40.0), 0.0, 'tall'))
        for b in self.buildings.values():
            for name, box in b.spots:
                self.spots.append((f'{b.id}: {name}', box, 0.0, 'stand'))
        if self.probe.ok:
            self.probe_spots()
        else:
            self.rule_spots()

    def rule_spots(self):
        posts = [o for o in self.layout.get('obstacles', []) if o['id'] in ('townGateNorth', 'townGateSouth')]
        if len(posts) == 2:
            mids = [(sum(q[0] for q in o['polygon']) / len(o['polygon']), sum(q[1] for q in o['polygon']) /
                     len(o['polygon'])) for o in posts]
            gx, gy = (mids[0][0] + mids[1][0]) * 0.5, (mids[0][1] + mids[1][1]) * 0.5
            self.spots.append(('the town gate (Main 3)', (gx, gy), SPOT_CLEAR, 'spot'))
            for k, (dx, dy) in enumerate(GATE_SPOTS):
                self.spots.append((f'the gate fight\'s spawn point {k + 1}', (gx + dx, gy + dy), SPAWN_CLEAR, 'spawn'))
        for pid, (lx, ly), name, clear in STORY_RULES:
            b = self.buildings.get(pid)
            if b:
                self.spots.append((name, b.at(lx, ly), clear, 'spawn' if clear == SPAWN_CLEAR else 'spot'))
        self.notes.append('no probe (Saved/RRTownProbe.json): the story\'s actors, the encounters\' points, the lootable '
                          'world and the dressing\'s instances are the rules\' (STORY_RULES, GATE_SPOTS, the layout\'s '
                          'prop groups grown 2.5 m); the ground isn\'t checked and lifts assume level pads')

    def probe_spots(self):
        for a in self.probe.actors:
            folder, cls, label = a.get('folder', ''), a.get('class', ''), a.get('label', '')
            x, y = a['at'][:2]
            if '/Loot' in folder or folder.endswith('Loot'):
                if cls in ('Chest', 'BreakableProp'):
                    self.spots.append((f'the lootable world\'s {label}', (x, y), LOOT_CLEAR, 'loot'))
                continue
            if '/Gameplay' not in folder:
                continue
            if cls in SPAWNERS:
                for k, (px, py) in enumerate(a.get('points', [])):
                    self.spots.append((f'{label}\'s point {k + 1}', (px, py), SPAWN_CLEAR, 'spawn'))
                self.spots.append((label, (x, y), SPOT_CLEAR, 'spot'))
                continue
            if cls in NOT_STORY:
                continue
            if cls == 'RespawnMarker':
                self.roosts.append((f'the roost {label}', (x, y)))
                continue
            if cls in ('WantedPoster', 'MissionArtifact') and a['at'][2] - (self.probe.ground(x, y) or 0.0) > 120.0:
                # On a wall, over head height: only what would stand in front of it and hide it.
                self.spots.append((f'{label} ({cls}, keep it readable)', (x, y), 80.0, 'tall'))
                continue
            clear = 200.0 if cls == 'Vendor' else 100.0 if cls == 'HobBird' else SPOT_CLEAR
            self.spots.append((f'{label} ({cls})', (x, y), clear, 'spot'))
        for mesh, rows in self.probe.data.get('instances', {}).items():
            if mesh.startswith(PASSABLE) or not self.meshes.has(mesh):
                continue
            x0, x1, y0, y1 = self.meshes.box(mesh)
            for x, y, _z, yaw, scale, solid in rows:
                if not solid:
                    continue
                self.instances.append((mesh, Box.frame(x, y, yaw, x0 * scale, x1 * scale, y0 * scale, y1 * scale)))
        self.notes.append(f'probe {self.probe.data.get("written", "?")}: {len(self.probe.actors)} actors, '
                          f'{len(self.instances)} dressing instances, {len(self.probe.grids)} height grids')

    def forget_own(self, previous):
        """The dressing's instances that are this town's own pieces from a build before (same mesh, same spot)."""
        own = defaultdict(list)
        for p in previous:
            own[p['mesh']].append(tuple(p['at'][:2]))
        before = len(self.instances)
        self.instances = [(m, b) for m, b in self.instances
                          if not any(math.dist((b.cx, b.cy), q) < 200.0 and self.near_pivot(m, b, q) for q in own[m])]
        if before != len(self.instances):
            self.notes.append(f'{before - len(self.instances)} of the dressing\'s instances are this town\'s own from '
                              f'the last build: not obstacles')
        # The lootable world turns the dressing's wooden crates and barrels into breakables where they stood
        # (build_area_loot.py): the ones that were this town's are its own pieces too.
        mine = [q for m in BREAKABLE for q in own[m]]
        before = len(self.spots)
        self.spots = [s for s in self.spots if not (s[3] == 'loot' and isinstance(s[1], tuple) and
                                                    any(math.dist(s[1], q) < 10.0 for q in mine))]
        if before != len(self.spots):
            self.notes.append(f'{before - len(self.spots)} of the lootable world\'s breakables are this town\'s own '
                              f'crates and barrels from the last build: not obstacles')

    def near_pivot(self, mesh, box, q):
        x0, x1, y0, y1 = self.meshes.box(mesh)
        px, py = to_world(box.cx, box.cy, box.yaw, -(x0 + x1) * 0.5, -(y0 + y1) * 0.5)
        return math.dist((px, py), q) < 10.0


# ---------------------------------------------------------------------------
# The pieces
# ---------------------------------------------------------------------------

class Piece:
    """A piece to place: where the design wants it, what it stands on, what the checks may do to it.
    mount: walk (a boardwalk, its steps, an awning: Main Street's own surface), ground, porch (stands on a building's
    porch, a walk or the platform: on), wall (fixed to a building's wall: host), air (hangs from one).
    cls: walk, wall, air, porch, verge (lamps), street (rails, troughs), big, small (the order they're placed in)."""

    def __init__(self, mesh, x, y, yaw, group, cls, mount='ground', host=None, on=None, lift=0.0, against=None,
                 along=None, tilt=None, solid=None, box=None, why=''):
        self.mesh, self.x, self.y, self.yaw = mesh, x, y, yaw % 360.0
        self.group, self.cls, self.mount = group, cls, mount
        self.host, self.on, self.lift, self.against = host, on, lift, against
        self.along = along          # (yaw) a wall piece's nudges run along this direction only
        self.tilt, self.solid_override, self.box_override, self.why = tilt, solid, box, why
        self.id = None
        self.children = []
        self.deferred = False
        self.placed = False
        self.final_lift = None
        self.sink = 0.0
        if on is not None:
            on.children.append(self)

    @property
    def new(self):
        return self.mesh in NEW_KIT

    @property
    def solid(self):
        if self.solid_override is not None:
            return self.solid_override
        if self.mesh in NEW_KIT:
            return NEW_KIT[self.mesh][2]
        return not self.mesh.startswith(PASSABLE)

    @property
    def kind(self):
        if self.mesh in NEW_KIT:
            k = NEW_KIT[self.mesh][3]
            return k if k in ('prop', 'upright', 'flat') else 'upright'
        return 'upright' if self.mesh in UPRIGHT else 'prop'


class Design:
    """The town as designed, building by building (cm, layout frame), before any check."""

    def __init__(self, world):
        self.world = world
        self.pieces, self.lines, self.spots, self.no_trees = [], [], [], []

    def frame(self, pid):
        b = self.world.buildings.get(pid)
        if b is None:
            p = self.world.places.get(pid)
            if p is None:
                return None
            return p['location'][0], p['location'][1], p.get('yaw', 0.0)
        return b.x, b.y, b.yaw

    def add(self, mesh, x, y, yaw, group, cls, **kw):
        piece = Piece(mesh, x, y, yaw, group, cls, **kw)
        self.pieces.append(piece)
        return piece

    def local(self, pid, mesh, lx, ly, lyaw, group, cls, **kw):
        """A piece in a building's frame: (lx, ly) from its pivot, turned lyaw from its front."""
        f = self.frame(pid)
        if f is None:
            self.world.warnings.append(f'no placement {pid}: {group} {mesh} left out')
            return None
        x, y, yaw = f
        wx, wy = to_world(x, y, yaw, lx, ly)
        kw.setdefault('host', pid if kw.get('mount') in ('porch', 'wall', 'air') and kw.get('on') is None else None)
        return self.add(mesh, wx, wy, yaw + lyaw, group, cls, **kw)


# ---------------------------------------------------------------------------
# The design
# ---------------------------------------------------------------------------

# Main Street (layout road mainStreet): its middle line X = 0, 10 m wide; the facades at X = +-800; the walks 3 m deep in
# front of them, their pivot line 1.5 m out (Boardwalk.py), pieces every 4 or 2 m along it.
FACADE_X = 800.0
WALK_X = 650.0
LAMP_X = 482.0           # the lamps' posts on the street's very edge, their lanterns over it
RAIL_X = 462.0           # the hitch rails at the street's edge, in front of the walks


def walk_run(d, side, start, lengths, group, steps=(True, True)):
    """A run of boardwalk on Main Street's north (side +1: the pieces turned 180, running east from their pivots) or south
    side (-1: turned 0, running west), from start (Y, cm) along the street, with its steps at its ends. Returns the
    sections (a piece on the walk stands on the one under it)."""
    sections = []
    x = side * WALK_X
    yaw = 180.0 if side > 0 else 0.0
    y = start
    for length in lengths:
        mesh = 'Boardwalk_4m' if length == 4 else 'Boardwalk_2m'
        sections.append(d.add(mesh, x, y, yaw, group, 'walk', mount='walk'))
        y += side * length * 100.0
    if steps[0]:
        d.add('Boardwalk_Steps', x, start, yaw + 180.0, group, 'walk', mount='walk')
    if steps[1]:
        d.add('Boardwalk_Steps', x, y, yaw, group, 'walk', mount='walk')
    return sections


def section_under(sections, x, y):
    best = None
    for s in sections:
        x0, x1, y0, y1 = (-150.0, 153.0, -400.0 if s.mesh == 'Boardwalk_4m' else -200.0, 0.0)
        lx, ly = to_local(s.x, s.y, s.yaw, x, y)
        if x0 <= lx <= x1 and y0 <= ly <= y1:
            best = s
    return best or sections[0]


def on_walk(d, sections, mesh, x, y, yaw, group, cls='porch', **kw):
    return d.add(mesh, x, y, yaw, group, cls, mount='porch', on=section_under(sections, x, y), lift=WALK_TOP, **kw)


def main_street(d):
    """The street: walks, the sheriff's awning, lamps, rails and troughs, what stands on the walks, the saloon's bunting,
    the shops' blade signs, Pruitt's buckboard."""
    g = 'mainStreet'
    # North side. The assay office and the sheriff's office: one run from just short of the gate fight's spawn point in
    # front of the assay office (Main 3: build_area_story GATE_SPOTS' (600, -200)) to the notice board's side.
    n1 = walk_run(d, 1, -50.0, (4, 4, 4, 4), 'assay', steps=(True, False))
    # The Gilt Spur under its balcony, into the hotel's porch (it chains with the walk at 3,903).
    n2 = walk_run(d, 1, 2900.0, (4, 4, 2), 'saloon', steps=(True, False))
    # South side: the barber's; Bright & Daughter's.
    s1 = walk_run(d, -1, 1600.0, (4, 2), 'barber')
    s2 = walk_run(d, -1, 6400.0, (4, 4), 'undertaker')
    # The sheriff's awning: two bays and the end post, on the walk's line (Boardwalk.py: ledger on the facade at 3.3 m).
    d.add('Awning_4m', WALK_X, 750.0, 180.0, 'sheriff', 'walk', mount='walk')
    d.add('Awning_4m', WALK_X, 1150.0, 180.0, 'sheriff', 'walk', mount='walk')
    d.add('Awning_Post', WALK_X, 1550.0, 180.0, 'sheriff', 'walk', mount='walk')

    # Lamps: at the walks' ends and in the gaps, both sides, the lantern hanging out over the street.
    for x, y in ((LAMP_X, -480.0), (LAMP_X, 2830.0), (LAMP_X, 5100.0), (LAMP_X + 18.0, 6300.0)):
        d.add('LampPost', x, y, 180.0, g, 'verge')
    for y in (-330.0, 720.0, 1760.0, 4660.0, 6480.0):
        d.add('LampPost', -LAMP_X, y, 0.0, g, 'verge')
    # Hitch rails at the street's edge in front of the walks, troughs in the gaps.
    # (Not in front of a door; south of Pruitt's the fields path leaves the street at 44 m, so its rail stands west of
    # the store, past Pen Lane's mouth.)
    for x, y in ((RAIL_X, 950.0), (RAIL_X, 3120.0), (-RAIL_X, 1300.0), (-RAIL_X, 2380.0)):
        d.add('HitchRail', x, y, 0.0, g, 'street')
    d.add('WaterTrough', 680.0, 5100.0, 0.0, g, 'street')
    d.add('WaterTrough', -650.0, 760.0, 0.0, g, 'street')

    # On the walks: a bench by the assay office's door, a barrel between it and the sheriff's, the dead sheriff's chair
    # still out by his door, a barrel at the walk's end.
    on_walk(d, n1, 'Bench', FACADE_X - 40.0, 220.0, 180.0, 'assay')
    on_walk(d, n1, 'Barrel_A', FACADE_X - 48.0, 560.0, 180.0, 'assay')
    on_walk(d, n1, 'PorchChair', FACADE_X - 35.0, 1000.0, 200.0, 'sheriff')
    on_walk(d, n1, 'Barrel_A', FACADE_X - 48.0, 1470.0, 160.0, 'sheriff')
    # Under the Gilt Spur's balcony, closed for mourning: two chairs left out by the door with the spittoon between them,
    # a crate of empties under the boarded window, a barrel at the end.
    on_walk(d, n2, 'PorchChair', FACADE_X - 38.0, 3070.0, 190.0, 'saloon')
    on_walk(d, n2, 'PorchChair', FACADE_X - 38.0, 3175.0, 165.0, 'saloon')
    on_walk(d, n2, 'Spittoon', FACADE_X - 75.0, 3122.0, 0.0, 'saloon')
    on_walk(d, n2, 'BottleCrate', FACADE_X - 30.0, 3640.0, 180.0, 'saloon')
    on_walk(d, n2, 'Barrel_A', FACADE_X - 48.0, 3820.0, 210.0, 'saloon')
    # The bunting on the balcony's rail (FalseFronts.py: the rail on the posts' line 9.84 m out from the saloon's middle,
    # the posts' faces 9.93 m, the rail's top 4.46 m up), across the doors, its pins just proud of the posts, hanging
    # toward the street.
    d.local('saloon', 'Bunting', 996.0, 0.0, 0.0, 'saloon', 'air', mount='air', lift=446.0)
    # The barber's: a bench under the shop window, a barrel at the walk's end, the blade sign on the false front's
    # west edge (FrontsRR.py: the hood and its scroll brackets take the whole front up to 3.36 m), its arm west along
    # the street, the board (2.57-3.05 m up) facing it.
    on_walk(d, s1, 'Bench', -FACADE_X + 30.0, 1250.0, 0.0, 'barber')
    on_walk(d, s1, 'Barrel_A', -FACADE_X + 55.0, 1060.0, 40.0, 'barber')
    bracket = d.local('cabinSouth', 'SignBracket', -8.0, -471.0, -90.0, 'barber', 'wall', mount='wall', lift=310.0)
    sign_board(d, bracket, 'SignBoard_Barber', 'barber')
    # Bright & Daughter's: a bench under the right window, the broom and pail by Tilly's door (her window, where the
    # player talks to her, kept clear), a barrel at the walk's east end.
    on_walk(d, s2, 'Bench', -FACADE_X + 40.0, 5810.0, 0.0, 'undertaker')
    on_walk(d, s2, 'BroomBucket', -FACADE_X + 12.0, 5670.0, 0.0, 'undertaker', against='undertaker')
    on_walk(d, s2, 'Barrel_A', -FACADE_X + 55.0, 6355.0, 20.0, 'undertaker')
    return n1, n2, s1, s2


def sign_board(d, bracket, mesh, group):
    """A blade sign's board on its bracket's SOCKET_Sign (out along the arm, a little under it), turned as it is."""
    if bracket is None:
        return None
    sx, sy = to_world(bracket.x, bracket.y, bracket.yaw, SIGN_SOCKET[0], 0.0)
    return d.add(mesh, sx, sy, bracket.yaw, group, 'air', mount='air', host=bracket.host,
                 lift=bracket.lift + SIGN_SOCKET[1], why=f'on {mesh}\'s bracket')


def pruitts(d):
    """Pruitt's General Store (store: yaw 0, its porch 6.5-9.5 m out, the deck at the walk's height): flour sacks and a
    barrel on the porch, a bench under the west window, the blade sign on the porch's east post (under the beam's end);
    the buckboard parked east of the porch, half unloaded: a crate in its bed, crates stacked and a barrel on the ground
    between it and the porch."""
    g = 'store'
    d.local('store', 'FlourSacks', 790.0, 330.0, 0.0, g, 'porch', mount='porch', lift=WALK_TOP)
    d.local('store', 'Barrel_A', 700.0, -420.0, 0.0, g, 'porch', mount='porch', lift=WALK_TOP)
    d.local('store', 'Bench', 690.0, -265.0, 0.0, g, 'porch', mount='porch', lift=WALK_TOP)
    # The east post (FalseFronts.py: posts 0.22 m in from the porch's ends, 2.82 m out from the facade): the bracket on
    # its east face, the arm east along the street, the board's faces to the street (2.42-2.89 m up).
    f = d.frame('store')
    if f:
        px, py = to_world(f[0], f[1], f[2], 932.0, 457.0)
        bracket = d.add('SignBracket', px, py, f[2] + 90.0, g, 'wall', mount='wall', host='store', lift=285.0)
        sign_board(d, bracket, 'SignBoard_Pruitts', g)
        cart = d.local('store', 'Buckboard', 790.0, 830.0, 90.0, g, 'big')
        if cart is not None:
            bx, by = to_world(cart.x, cart.y, cart.yaw, BUCKBOARD_BED[0], 0.0)
            d.add('Crate_B', bx, by, cart.yaw + 10.0, g, 'small', mount='porch', on=cart, lift=BUCKBOARD_BED[1])
        low = d.local('store', 'Crate_A', 730.0, 540.0, 90.0, g, 'small')
        if low is not None:
            d.add('Crate_A', low.x, low.y, low.yaw + 8.0, g, 'small', mount='porch', on=low, lift=61.0)
        d.local('store', 'Barrel_A', 860.0, 540.0, 0.0, g, 'small')


def hotel(d):
    """The Larkspur House (cabinNorth: yaw 180, its 3 m porch at the walk's height): a bench under the west window, the
    blade sign on the porch's east post (its east face, the arm east along the street)."""
    g = 'hotel'
    d.local('cabinNorth', 'Bench', 30.0, 300.0, 0.0, g, 'porch', mount='porch', lift=WALK_TOP)
    d.local('cabinNorth', 'Barrel_A', 45.0, -440.0, 0.0, g, 'porch', mount='porch', lift=WALK_TOP)
    f = d.frame('cabinNorth')
    if f:
        px, py = to_world(f[0], f[1], f[2], 282.0, -486.0)
        bracket = d.add('SignBracket', px, py, f[2] - 90.0, g, 'wall', mount='wall', host='cabinNorth', lift=280.0)
        sign_board(d, bracket, 'SignBoard_Hotel', g)


def hasketts(d):
    """The Haskett house (cottageNorth, RR_Home_A, lived in: the couple and the old father), yaw 180, its front door the
    pivot. Front: flower boxes under the two lamplit windows (sills 1.35 m), a lantern by the door, broom and bucket.
    Back yard (behind its back wall, 6.45 m back): the block by the woodpile, the washtub by the back door, the laundry
    line, the kitchen garden, and the chicken yard with its coop."""
    g, h = 'haskett', 'cottageNorth'
    for ly in (244.0, -244.0):
        d.local(h, 'FlowerBox', 5.0, ly, 0.0, g, 'wall', mount='wall', lift=132.0)
    lantern = d.local(h, 'PorchLantern', 3.0, -95.0, 0.0, g, 'wall', mount='wall', lift=230.0)
    d.local(h, 'BroomBucket', 3.0, -190.0, 0.0, g, 'small', against=h, along=True)
    d.local(h, 'ChoppingBlock', -760.0, 220.0, 15.0, g, 'small')
    d.local(h, 'Washtub', -755.0, -10.0, 0.0, g, 'small')
    d.local(h, 'LaundryLine', -960.0, 60.0, 0.0, g, 'big')
    d.local(h, 'KitchenGarden', -1180.0, 230.0, 0.0, g, 'big')
    chicken_yard(d, h, g, (-1100.0, -1500.0, -60.0, -400.0), gate=(-1100.0, -300.0), coop=(-1340.0, -230.0, 0.0),
                 bales=())
    d.no_trees.append(polygon_in(d, h, [(-640.0, 400.0), (-1580.0, 400.0), (-1580.0, -470.0), (-640.0, -470.0)],
                                 'haskett_yard'))
    return lantern


def mercers(d):
    """The Mercer house (cottageNorthEast, RR_Home_H, lived in: the mother and her child), yaw 180. Its porch (0.5 m
    deck, railed): the rocker with the shawl turned toward the steps, the cat's spot beside it, broom and bucket by the
    door, a lantern by the door, flower boxes under the lamplit windows, a horseshoe on the post by the steps; the side
    porch's washtub and chair. Back yard: the pump, milk cans, the laundry line, the kitchen garden."""
    g, h = 'mercer', 'cottageNorthEast'
    d.local(h, 'RockingChair_Shawl', 100.0, 205.0, -20.0, g, 'porch', mount='porch', lift=50.0)
    d.local(h, 'BroomBucket', 3.0, -195.0, 0.0, g, 'porch', mount='porch', lift=50.0, against=h)
    lantern = d.local(h, 'PorchLantern', 3.0, 85.0, 0.0, g, 'wall', mount='wall', lift=250.0)
    for ly in (212.0, -212.0):
        d.local(h, 'FlowerBox', 5.0, ly, 0.0, g, 'wall', mount='wall', lift=132.0)
    d.local(h, 'Horseshoe', 197.0, 105.0, 0.0, g, 'wall', mount='wall', lift=240.0)
    d.local(h, 'Washtub', -520.0, -455.0, 0.0, g, 'porch', mount='porch', lift=50.0)
    d.local(h, 'PorchChair', -640.0, -410.0, -90.0, g, 'porch', mount='porch', lift=50.0)
    d.local(h, 'FirewoodStack', -300.0, 380.0, 90.0, g, 'big', against=h)
    d.local(h, 'HandPump', -820.0, 320.0, 0.0, g, 'big')
    d.local(h, 'MilkCans', -760.0, -200.0, 30.0, g, 'small')
    d.local(h, 'LaundryLine', -1000.0, 50.0, 0.0, g, 'big')
    d.local(h, 'KitchenGarden', -1150.0, -330.0, 0.0, g, 'big')
    cat = d.frame(h)
    if cat:
        x, y = to_world(cat[0], cat[1], cat[2], 60.0, 290.0)
        d.spots.append(dict(id='cat_mercer', kind='cat', at=[rnd1(x), rnd1(y)], lift=50.0, host=h,
                            yaw=rnd1((cat[2] + 200.0) % 360.0), group=g, pose='curled',
                            note='a cat on the Mercers\' porch beside the rocker, on the 0.5 m deck (lift from the ground '
                                 'under it; with the probe, from the house\'s height)'))
    d.no_trees.append(polygon_in(d, h, [(-690.0, 450.0), (-1300.0, 450.0), (-1300.0, -560.0), (-690.0, -560.0)],
                                 'mercer_yard'))
    return lantern


def manager(d):
    """The company manager's gallery house (cottageSouth, RR_Home_D, shut up and dark), yaw 0: a rocker left on the
    porch, a packing crate nailed shut, the outer flower boxes gone to seed."""
    g, h = 'manager', 'cottageSouth'
    d.local(h, 'RockingChair', 100.0, 230.0, 25.0, g, 'porch', mount='porch', lift=55.0)
    d.local(h, 'Crate_A', 110.0, -300.0, 10.0, g, 'porch', mount='porch', lift=55.0)
    for ly in (305.0, -305.0):
        d.local(h, 'FlowerBox', 5.0, ly, 0.0, g, 'wall', mount='wall', lift=137.0)


def empty_homes(d):
    """The empty ones: the shack by the gate (a wheelbarrow left out), Pen Lane's shotgun house (a barrel lying in the
    weeds), Amos's cabin (his bench by the door, the winter's wood he never burned, the axe still in the block)."""
    d.local('cottageSouthWest', 'Wheelbarrow', -120.0, 330.0, 70.0, 'shack', 'big')
    d.local('penLaneWest', 'Barrel_A', 120.0, 380.0, 0.0, 'penLane', 'small', lift=33.0, tilt=(0.0, 90.0),
            box=(-33.0, 33.0, -95.0, 95.0), mount='ground')
    d.local('amosHouse', 'Bench', 30.0, 180.0, 0.0, 'amos', 'small', against='amosHouse')
    d.local('amosHouse', 'FirewoodStack', -220.0, 295.0, 90.0, 'amos', 'big', against='amosHouse')
    d.local('amosHouse', 'ChoppingBlock', 90.0, 360.0, -20.0, 'amos', 'small')


def dugout(d):
    """The dugout on boot hill (lived in; its bench, barrel, washtub, woodpile and block are its own): a laundry line west
    of the arbor, a kitchen garden below it, off the trail to town."""
    d.add('LaundryLine', 1650.0, -1720.0, 90.0, 'dugout', 'big')
    d.add('KitchenGarden', 1550.0, -880.0, 180.0, 'dugout', 'big')
    d.no_trees.append(dict(id='dugout_yard', polygon=[[1350.0, -2300.0], [2100.0, -2300.0], [2100.0, -650.0],
                                                      [1350.0, -650.0]]))


def workshop(d):
    """Tilly's workshop (workshop, RR_Workshop_Bright, yaw 90, its yard doors east): firewood for the box stove along its
    north wall, a rain barrel at the front corner, a wheelbarrow south of it, a horseshoe over the doors."""
    g, h = 'workshop', 'workshop'
    d.local(h, 'Horseshoe', 3.0, 0.0, 0.0, g, 'wall', mount='wall', lift=275.0)
    d.local(h, 'FirewoodStack', -160.0, -840.0, -90.0, g, 'big', against=h)
    d.local(h, 'Barrel_A', 60.0, 170.0, 0.0, g, 'small')
    d.local(h, 'Wheelbarrow', -250.0, 370.0, 0.0, g, 'big')


def depot(d):
    """The depot's platform past the station house's east end (the waiting room's fight stays at its west end): a hand
    cart, freight crates (two stacked), milk cans, flour, two drums, along the platform's back half (the train side kept
    clear); a lamp where the yard road meets the platform's west ramp."""
    g = 'depot'
    items = [('Cart', 330.0, 12250.0, 90.0), ('Crate_A', 395.0, 12560.0, 90.0), ('Crate_B', 390.0, 12720.0, 75.0),
             ('MilkCans', 405.0, 12850.0, 180.0), ('FlourSacks', 375.0, 13040.0, 90.0),
             ('Barrel_B', 410.0, 13180.0, 0.0), ('Barrel_B', 335.0, 13240.0, 40.0)]
    for mesh, x, y, yaw in items:
        host = platform_at(d, x, y)
        if host is None:
            d.world.warnings.append(f'depot {mesh} at ({x:.0f}, {y:.0f}): no platform piece under it: left out')
            continue
        piece = d.add(mesh, x, y, yaw, g, 'porch', mount='porch', host=host, lift=PLATFORM_TOP)
        if mesh == 'Crate_A':
            d.add('Crate_A', x, y, yaw + 8.0, g, 'small', mount='porch', on=piece, lift=61.0)
    d.add('LampPost', 600.0, 10500.0, 180.0, g, 'verge')


def platform_at(d, x, y):
    for pid, b in d.world.buildings.items():
        if b.model == 'DepotPlatform_4m' and any(box.contains(x, y) for box, _ in b.decks):
            return pid
    return None


def farm(d):
    """Ransom Farm (Delia's: her porch is the story's and keeps its own rocker, broom and pail): her kitchen garden south
    of the house, a chicken yard with its coop and straw between the house and the privy, her laundry line and washtub
    behind the house, the woodpile on its south wall (the family plot path passes its north wall) and the block by it,
    milk cans at the barn's doors, a wheelbarrow."""
    g = 'farm'
    d.add('KitchenGarden', -4830.0, -5800.0, 0.0, g, 'big')
    chicken_yard(d, None, g, (-4850.0, -5250.0, -6200.0, -6650.0), gate=(-4850.0, -6300.0),
                 coop=(-5100.0, -6430.0, 0.0), bales=((-4770.0, -6470.0, 0.0),))
    d.add('LaundryLine', -4150.0, -6550.0, 0.0, g, 'big')
    d.add('Washtub', -4050.0, -6320.0, 0.0, g, 'small')
    d.add('FirewoodStack', -4460.0, -6000.0, 0.0, g, 'big', against='farmhouse')
    d.add('ChoppingBlock', -4570.0, -6160.0, 40.0, g, 'small')
    d.local('barn', 'MilkCans', 780.0, -270.0, 180.0, g, 'small')
    d.add('Wheelbarrow', -3250.0, -6500.0, 30.0, g, 'big')
    d.no_trees.append(dict(id='farm_yard', polygon=[[-5350.0, -6750.0], [-3400.0, -6750.0], [-3400.0, -5600.0],
                                                    [-5350.0, -5600.0]]))


def polygon_in(d, pid, pts, name):
    f = d.frame(pid)
    out = [list(map(rnd1, to_world(f[0], f[1], f[2], lx, ly))) for lx, ly in pts] if f else []
    return dict(id=name, polygon=out)


def chicken_yard(d, pid, group, rect, gate, coop, bales):
    """A picket chicken yard (rect: x0, x1, y0, y1 in pid's frame, or the world's without one), its gate, the coop inside
    facing it, straw bales, and the chickens' spot (the yard inside its fence, 40 cm in)."""
    f = d.frame(pid) if pid else (0.0, 0.0, 0.0)
    if f is None:
        return
    x0, x1, y0, y1 = rect

    def w(lx, ly):
        return to_world(f[0], f[1], f[2], lx, ly)
    corners = [w(x0, y0), w(x1, y0), w(x1, y1), w(x0, y1)]
    gx, gy = w(*gate)
    lid = f'{group}_chickens'
    d.lines.append(dict(id=lid, kit='picket', points=[[rnd1(x), rnd1(y)] for x, y in corners], closed=True,
                        gates=[[rnd1(gx), rnd1(gy), 100.0]], broken=0.0, group=group))
    cx, cy, cyaw = coop
    wx, wy = w(cx, cy)
    house = d.add('ChickenCoop', wx, wy, f[2] + cyaw, group, 'big', why='in its chicken yard')
    house.yard = lid
    for bx, by, byaw in bales:
        low = d.add('HayBale_Square', bx, by, byaw, group, 'small')
        d.add('HayBale_Square', bx, by, byaw + 8.0, group, 'small', mount='porch', on=low, lift=36.0)
    inset = 40.0
    sx = 1.0 if x1 > x0 else -1.0
    sy = 1.0 if y1 > y0 else -1.0
    inner = [w(x0 + sx * inset, y0 + sy * inset), w(x1 - sx * inset, y0 + sy * inset),
             w(x1 - sx * inset, y1 - sy * inset), w(x0 + sx * inset, y1 - sy * inset)]
    mx, my = w((x0 + x1) * 0.5, (y0 + y1) * 0.5)
    d.spots.append(dict(id=lid, kind='chickens', at=[rnd1(mx), rnd1(my)], polygon=[[rnd1(x), rnd1(y)] for x, y in inner],
                        count=4, coop=None, group=group, line=lid,
                        note='hens pecking about the picket yard; the coop (ChickenCoop) is theirs at night'))
    d.spots[-1]['_coop'] = house


def design(world):
    d = Design(world)
    main_street(d)
    pruitts(d)
    hotel(d)
    hasketts(d)
    mercers(d)
    manager(d)
    empty_homes(d)
    dugout(d)
    workshop(d)
    depot(d)
    farm(d)
    return d


# ---------------------------------------------------------------------------
# The checks
# ---------------------------------------------------------------------------

class Placer:
    CELL = 800.0

    def __init__(self, world, design):
        self.world, self.design = world, design
        self.meshes = world.meshes
        self.grid = defaultdict(list)
        self.placed = []
        self.nudged, self.dropped = [], []
        self.unmeasured = set()
        self.line_segments = []
        for line in design.lines:
            pts = [tuple(p) for p in line['points']]
            self.line_segments.append((line['id'], pts + [pts[0]] if line['closed'] else pts, line))

    def footprint(self, piece, x, y):
        if piece.box_override:
            x0, x1, y0, y1 = piece.box_override
        else:
            x0, x1, y0, y1 = self.meshes.box(piece.mesh)
        return Box.frame(x, y, piece.yaw, x0, x1, y0, y1)

    def cells(self, bbox, pad=0.0):
        x0, y0, x1, y1 = bbox
        for i in range(int(math.floor((x0 - pad) / self.CELL)), int(math.floor((x1 + pad) / self.CELL)) + 1):
            for j in range(int(math.floor((y0 - pad) / self.CELL)), int(math.floor((y1 + pad) / self.CELL)) + 1):
                yield i, j

    def near(self, box, pad):
        seen = set()
        for cell in self.cells(box.bbox(), pad):
            for rec in self.grid.get(cell, ()):
                if id(rec) not in seen:
                    seen.add(id(rec))
                    yield rec

    def surface(self, piece):
        """The boxes a porch piece must stand inside: its building's decks, the run of walk or the platform it's on, or
        the piece it rests on."""
        if piece.on is not None:
            host = piece.on
            if host.mount == 'walk':
                run = [p for p in self.design.pieces if p.mount == 'walk' and p.group == host.group and p.placed and
                       p.mesh.startswith('Boardwalk_') and p.mesh != 'Boardwalk_Steps']
                return [self.footprint(p, p.x, p.y) for p in run]
            return [self.footprint(host, host.x, host.y)]
        b = self.world.buildings.get(piece.host)
        if b is not None and b.model == 'DepotPlatform_4m':
            # The platform is one deck of 4 m pieces.
            return [box for o in self.world.buildings.values() if o.model == b.model for box, _ in o.decks]
        return [box for box, _ in b.decks] if b else []

    def problem(self, piece, x, y):
        """The first thing in the way of the piece at (x, y): (what, a push away from it), or None."""
        w = self.world
        box = self.footprint(piece, x, y)
        mount, cls = piece.mount, piece.cls
        if mount in ('wall', 'air'):
            if piece.host not in w.buildings and piece.host not in w.places:
                return f'its building {piece.host} isn\'t placed', None
            for rec in self.near(box, 100.0):
                o = rec
                if mount == 'wall' and o.mount == 'wall' and o.host == piece.host and o is not piece and \
                        sat_gap(box, rec.box) < 2.0 and \
                        abs(o.lift - piece.lift) < 0.5 * (self.meshes.height(o.mesh) + self.meshes.height(piece.mesh)):
                    return f'{o.id} ({o.mesh}) on the same wall', (x - o.x, y - o.y)
            return None
        on_piece = piece.on is not None

        # What it stands on.
        if mount == 'porch':
            surfaces = self.surface(piece)
            inner = box.scaled(0.92)
            if on_piece and piece.on.mount != 'walk':
                pass        # a crate in a wagon's bed, a crate or a bale on another: it rests where its host is
            elif not surfaces or not all(any(s.contains(px, py, 2.0) for s in surfaces) for px, py in inner.samples()):
                where = 'its walk' if on_piece else f'{piece.host}\'s porch' if piece.host else 'its deck'
                cx, cy = (surfaces[0].cx, surfaces[0].cy) if surfaces else (x, y)
                return f'off {where}', (cx - x, cy - y)
        elif w.probe.ok and mount in ('ground', 'walk'):
            heights = [w.probe.ground(px, py) for px, py in box.samples()]
            if any(h is None for h in heights):
                self.unmeasured.add(piece.id)
                heights = [h for h in heights if h is not None] or [0.0]
            limit = GROUND_RANGE['flat' if piece.kind == 'flat' else cls if cls in GROUND_RANGE else 'small']
            if max(heights) - min(heights) > limit:
                return f'uneven ground ({max(heights) - min(heights):.0f} cm under it)', None

        # Buildings: walls, porches, steps, posts, their own dressing, the ways to their doors.
        bx0, by0, bx1, by1 = box.bbox()
        for b in w.buildings.values():
            r = b.reach_box()
            if r is None or r[0] > bx1 + 700.0 or r[2] < bx0 - 700.0 or r[1] > by1 + 700.0 or r[3] < by0 - 700.0:
                continue
            home = mount == 'porch' and not on_piece and piece.host == b.id
            on_walk = on_piece and piece.on.mount == 'walk'
            against = piece.against == b.id
            if mount == 'walk':
                for wall in b.walls:
                    if sat_gap(box, wall) < -35.0:
                        return f'{b.id}\'s walls', (x - wall.cx, y - wall.cy)
                for part in b.builtin + b.steps:
                    if sat_gap(box, part) < 0.0:
                        return f'{b.id}\'s own dressing or steps', (x - part.cx, y - part.cy)
                for deck, _ in b.decks:
                    if sat_gap(box, deck) < -35.0:
                        return f'{b.id}\'s porch', (x - deck.cx, y - deck.cy)
                continue
            need = -2.0 if (against or home or on_walk) else GAP
            for wall in b.walls:
                if sat_gap(box, wall) < need:
                    return f'{b.id}\'s walls', (x - wall.cx, y - wall.cy)
            for part in b.posts + b.rails + b.builtin:
                if sat_gap(box, part) < 3.0:
                    return f'{b.id}\'s posts, rails or own dressing', (x - part.cx, y - part.cy)
            if not home and not on_piece:
                for deck, _ in b.decks:
                    if sat_gap(box, deck) < GAP:
                        return f'{b.id}\'s porch', (x - deck.cx, y - deck.cy)
            for part in b.steps:
                if sat_gap(box, part) < GAP:
                    return f'{b.id}\'s steps', (x - part.cx, y - part.cy)
            for way in b.ways:
                if sat_gap(box, way) < 0.0:
                    return f'the way to {b.id}\'s door', (x - way.cx, y - way.cy)

        # Gameplay spots.
        for name, spot, clear, kind in w.spots:
            if kind == 'tall' and self.meshes.height(piece.mesh) + piece.lift < 150.0:
                continue
            if mount == 'walk' and kind not in ('spawn', 'loot'):
                continue        # a walk is ground: the player stands on it to talk, read and trade
            if mount == 'walk' and kind == 'loot':
                clear = 60.0
            need = clear
            if kind == 'spawn' and mount == 'walk':
                need = WALK_SPAWN_CLEAR
            if isinstance(spot, Box):
                if sat_gap(box, spot) < need:
                    return name, (x - spot.cx, y - spot.cy)
            elif point_box_dist(box, *spot) < need:
                return name, (x - spot[0], y - spot[1])
        height = self.meshes.height(piece.mesh)
        for name, (rx, ry) in w.roosts:
            d = point_box_dist(box, rx, ry)
            if d < ROOST_CLEAR:
                return f'{name} ({ROOST_CLEAR / 100.0:.1f} m)', (x - rx, y - ry)
            if d < ROOST_FIND and piece.solid and height > ROOST_TALL and 2.0 * min(box.hx, box.hy) > ROOST_WIDE:
                return f'{name}\'s find circle (a tall piece in the line to its mast)', (x - rx, y - ry)

        # Roads and trails.
        if mount != 'porch':
            for rid, pts, half, rkind in w.roads:
                if rkind == 'rail' and mount == 'walk':
                    continue
                margin = ROAD_MARGIN.get(cls, 40.0)
                if rid in STREET_ROADS and cls in STREET_EDGE:
                    margin = STREET_EDGE[cls]
                need = half + margin
                if box_line_dist(box, pts) < need:
                    q, _ = min((seg_point(a, b, (x, y)) for a, b in zip(pts, pts[1:])), key=lambda t: t[1])
                    return f'the road {rid}', (x - q[0], y - q[1])
            if piece.solid and mount == 'ground':
                for tid, pts in w.trails:
                    if box_line_dist(box, pts) < TRAIL_CLEAR:
                        q, _ = seg_point(pts[0], pts[1], (x, y))
                        return f'the {tid}', (x - q[0], y - q[1])

            # Layout obstacles and zones kept out of.
            for oid, kind, pts, closed, clear in w.obstacles:
                xs, ys = [p[0] for p in pts], [p[1] for p in pts]
                if min(xs) > bx1 + clear or max(xs) < bx0 - clear or min(ys) > by1 + clear or max(ys) < by0 - clear:
                    continue
                dist = box_poly_dist(box, pts) if closed and kind != 'fence' else box_line_dist(box, pts, closed)
                if dist < clear:
                    mx, my = sum(xs) / len(xs), sum(ys) / len(ys)
                    return f'the layout\'s {kind} {oid}', (x - mx, y - my)
            for zid in KEEP_OUT_ZONES:
                poly = w.zones.get(zid)
                if poly and box_poly_dist(box, poly) < 50.0:
                    return f'the zone {zid}', None

            # The dressing already in the level (the probe's), and this town's fences.
            for mesh, other in w.instances:
                if abs(other.cx - x) > 1500.0 or abs(other.cy - y) > 1500.0:
                    continue
                if sat_gap(box, other) < (GAP if piece.solid else 0.0):
                    return f'the dressing\'s {mesh} at ({other.cx:.0f}, {other.cy:.0f})', (x - other.cx, y - other.cy)
            for lid, pts, line in self.line_segments:
                if getattr(piece, 'yard', None) == lid:
                    inner = box.grown(30.0)
                    if not all(inside(pts[:-1], px, py) for px, py in inner.corners()) or \
                            box_line_dist(box, pts) < 30.0:
                        return f'its yard\'s fence {lid}', None
                    continue
                if box_line_dist(box, pts) < 20.0 or inside(pts[:-1], x, y):
                    return f'the fence {lid}', None

        # Everything placed already: a walk's pieces join one another and the awnings stand over them; what stands on a
        # walk or a porch keeps off the other pieces there and the awnings' posts; a piece and what rests on it touch.
        for o in self.near(box, 300.0):
            if o is piece or o.mount in ('wall', 'air') or piece.on is o or o.on is piece:
                continue
            if mount == 'walk' and o.mount == 'walk':
                continue
            if mount == 'porch' and o.mount == 'walk' and not o.mesh.startswith('Awning'):
                continue
            if piece.on is not None and o.on is piece.on and piece.on.mount != 'walk':
                continue
            if not (piece.solid or o.solid):
                continue
            need = GAP if (piece.solid and o.solid) else 0.0
            if sat_gap(box, o.box) < need:
                return f'{o.id} ({o.mesh})', (x - o.x, y - o.y)
        return None

    def place(self, piece):
        """Stand the piece where the design wants it, or as near as its reach allows (along its wall for one fixed to a
        building), or drop it with what stands on it."""
        if piece.on is not None and not piece.on.placed:
            self.dropped.append(f'{piece.id} ({piece.mesh}): what it stands on, {piece.on.id}, was dropped')
            return False
        hit = self.problem(piece, piece.x, piece.y)
        if hit is None:
            return self.accept(piece, piece.x, piece.y)
        reason, push = hit
        reach = REACH.get(piece.mount if piece.mount in ('walk', 'wall', 'air', 'porch') else piece.cls, 150.0)
        if piece.on is not None and piece.on.mount != 'walk':
            reach = 0.0
        if reach > 0.0:
            if piece.along or piece.mount == 'porch':
                # Along its wall (the piece's own Y, both ways); on a porch, also in and out a little (its own X).
                ax, ay = rot(0.0, 1.0, piece.yaw)
                ox, oy = rot(1.0, 0.0, piece.yaw)
                tries = []
                r = STEP
                while r <= reach + 1e-6:
                    for sign in (1.0, -1.0):
                        tries.append((piece.x + ax * r * sign, piece.y + ay * r * sign, r))
                    if piece.mount == 'porch':
                        for sign in (1.0, -1.0):
                            tries.append((piece.x + ox * r * sign, piece.y + oy * r * sign, r))
                    r += STEP
            else:
                base = math.atan2(push[1], push[0]) if push and math.hypot(*push) > 1e-6 else 0.0
                tries = []
                r = STEP * 2.5
                while r <= reach + 1e-6:
                    for k in range(24):
                        turn = ((k + 1) // 2) * (1 if k % 2 else -1) * math.radians(15.0)
                        tries.append((piece.x + math.cos(base + turn) * r, piece.y + math.sin(base + turn) * r, r))
                    r += STEP * 2.5
            for tx, ty, r in tries:
                if self.problem(piece, tx, ty) is None:
                    self.nudged.append(f'{piece.id} ({piece.mesh}) {r:.0f} cm off {reason}')
                    return self.accept(piece, tx, ty)
        self.dropped.append(f'{piece.id} ({piece.mesh}) at ({piece.x:.0f}, {piece.y:.0f}): {reason}'
                            + (f', no free spot within {reach / 100.0:.1f} m' if reach > 0.0 else ' (it stays put or '
                               'goes)'))
        return False

    def accept(self, piece, x, y):
        dx, dy = x - piece.x, y - piece.y
        piece.x, piece.y = x, y
        piece.placed = True
        piece.box = self.footprint(piece, x, y)
        # What stands on it moves with it.
        if dx or dy:
            for child in piece.children:
                child.x += dx
                child.y += dy
        self.placed.append(piece)
        for cell in self.cells(piece.box.bbox()):
            self.grid[cell].append(piece)
        return True


# ---------------------------------------------------------------------------
# The build
# ---------------------------------------------------------------------------

def name_pieces(d):
    counters = Counter()
    for p in d.pieces:
        counters[(p.group, p.mesh)] += 1
        p.id = f'{p.group}_{snake(p.mesh)}_{counters[(p.group, p.mesh)]:02d}'


def defer(d, approved):
    """A new-kit piece waits for the user's approval; so does what stands on or hangs from one."""
    for p in d.pieces:
        p.deferred = p.new and p.mesh not in approved
    changed = True
    while changed:
        changed = False
        for p in d.pieces:
            if not p.deferred and p.on is not None and p.on.deferred:
                p.deferred = changed = True


def place_all(d, placer):
    order = {c: k for k, c in enumerate(CLASSES)}
    roots = [p for p in d.pieces if p.on is None]
    roots.sort(key=lambda p: (order[p.cls], d.pieces.index(p)))

    def go(piece):
        placer.place(piece)
        for child in sorted(piece.children, key=lambda c: (order[c.cls], d.pieces.index(c))):
            go(child)
    # The walks first (what stands on them comes after every walk is down, so a run's surface is whole).
    walks = [p for p in roots if p.cls == 'walk']
    for p in walks:
        placer.place(p)
    for p in walks:
        for child in sorted(p.children, key=lambda c: (order[c.cls], d.pieces.index(c))):
            go(child)
    for p in roots:
        if p.cls != 'walk':
            go(p)


def lifts(d, world):
    """Each lifted piece's lift as written: from its host piece (on), or from the ground under it, corrected by the probe
    for its building's own height (a porch's deck and a wall's height are the building's, not the ground's)."""
    probe = world.probe
    for p in d.pieces:
        if not p.placed:
            continue
        if p.lift <= 0.0:
            p.final_lift = None
            continue
        lift = p.lift
        if p.on is None and p.host and probe.ok:
            host_z = probe.actor_z(p.host)
            ground = probe.ground(p.x, p.y)
            if host_z is not None and ground is not None:
                lift = p.lift + (host_z - ground)
        p.final_lift = max(lift, 0.5)


def align_walks(d, world):
    """With the probe: each walk section, its steps and the awnings over it raised or sunk (the dressing's sink) so the
    walk's top meets its building's floor (the false fronts' 0.38 m over their pivots), predicting where the dressing's
    town_pose() would stand it (plumb, on the lowest ground under its sides, with DRESSING_PIECES' sizes). What stands on a
    walk hangs from it, so it follows. Without the probe the walks stand where the ground puts them."""
    probe = world.probe
    if not probe.ok:
        return []
    out = []
    walks = [p for p in d.pieces if p.placed and p.mount == 'walk']
    targets = {}
    for p in walks:
        shape = Box.frame(p.x, p.y, p.yaw, -150.0, 153.0, -400.0, 0.0)
        best, best_gap = None, 80.0
        for b in world.buildings.values():
            if b.z is None:
                continue
            for wall in b.walls:
                gap = sat_gap(shape, wall)
                if gap < best_gap:
                    best, best_gap = b, gap
        if best is not None:
            targets[p.id] = (best.z, best.id)
    for p in walks:
        if p.id not in targets:
            # Steps at a run's end: their run's nearest section's building.
            near = min((q for q in walks if q.group == p.group and q.id in targets),
                       key=lambda q: math.dist((q.x, q.y), (p.x, p.y)), default=None)
            if near is None:
                continue
            targets[p.id] = targets[near.id]
        (length, depth, _), _, _, _ = DRESSING_PIECES.get(p.mesh, ((100, 100, 100), None, 0.0, 0.0))
        fx, fy = math.cos(math.radians(p.yaw)), math.sin(math.radians(p.yaw))
        hx, hy = max(depth * 0.5, 10.0), max(length * 0.5, 10.0)
        pts = [(p.x, p.y), (p.x + fx * hx, p.y + fy * hx), (p.x - fx * hx, p.y - fy * hx),
               (p.x - fy * hy, p.y + fx * hy), (p.x + fy * hy, p.y - fx * hy)]
        g = [probe.ground(*q) for q in pts]
        if any(v is None for v in g):
            continue
        predicted = max(min(g), g[0] - UPRIGHT_SINK)
        target, bid = targets[p.id]
        sink = predicted - target
        if abs(sink) > WALK_SINK_MOST:
            out.append(f'{p.id}: the ground stands it {sink:+.0f} cm off {bid}\'s floor level; raised or sunk only '
                       f'{WALK_SINK_MOST:.0f}')
            sink = math.copysign(WALK_SINK_MOST, sink)
        p.sink = sink if abs(sink) >= 1.0 else 0.0
    return out


def verify(d, world, placer):
    """The result on its own: no solid piece in a building's walls, on a road's surface past what its class may, or
    overlapping another solid piece (a host and what stands on it aside)."""
    out = []
    placed = [p for p in d.pieces if p.placed]
    for p in placed:
        if p.mount in ('wall', 'air'):
            continue
        box = p.box
        for b in world.buildings.values():
            if p.mount in ('walk', 'porch') or p.against == b.id:
                continue
            for wall in b.walls:
                if sat_gap(box, wall) < 0.0:
                    out.append(f'verify: {p.id} stands in {b.id}')
    solids = [p for p in placed if p.solid and p.mount not in ('wall', 'air')]
    for i, a in enumerate(solids):
        for b in solids[i + 1:]:
            if abs(a.x - b.x) > 1500.0 or abs(a.y - b.y) > 1500.0:
                continue
            if a.on is b or b.on is a or (a.mount == 'walk' and b.mount == 'walk') or \
                    (a.on is not None and a.on is b.on and a.on.mount != 'walk'):
                continue
            if (a.mount == 'walk' and b.mount == 'porch') or (b.mount == 'walk' and a.mount == 'porch'):
                continue
            if sat_gap(a.box, b.box) < 0.0:
                out.append(f'verify: {a.id} and {b.id} overlap')
    return out


def piece_json(p):
    out = {'id': p.id, 'mesh': p.mesh, 'at': [rnd1(p.x), rnd1(p.y)], 'yaw': rnd1(p.yaw % 360.0), 'group': p.group}
    if p.final_lift:
        out['lift'] = rnd1(p.final_lift)
    if p.sink:
        out['sink'] = rnd1(p.sink)
    if p.tilt:
        out['tilt'] = [rnd1(p.tilt[0]), rnd1(p.tilt[1])]
    default_solid = not p.mesh.startswith(PASSABLE)
    if p.solid != default_solid:
        out['solid'] = bool(p.solid)
    out['kind'] = p.kind
    if p.on is not None:
        out['hangFrom'] = p.on.id
    return out


ABOUT = (
    "Ransom's Rest's town dressing (the 'make it alive' polish, concept board A_town): the pieces "
    "Tools/Unreal/build_area_dressing.py places instanced when layout.json's level.town names this file, the chicken "
    "yards' picket lines, the no-tree areas the scatter keeps out of, and what waits for the user's approval. Made by "
    "make_town.py beside this file from layout.json, Saved/MeshBounds.json and (when Main has run Saved/rr_town_probe.py) "
    "Saved/RRTownProbe.json; edit those and run it again, never this file. Unreal cm, X north, Y east, yaw in degrees "
    "from +X. pieces: mesh without SM_; at its pivot; lift cm over the ground under it (or over the piece hangFrom names: "
    "what stands on a walk, in a wagon's bed, on a crate); tilt [pitch, roll] added; solid overrides the mesh's default "
    "(wall and hanging pieces have none); kind prop, upright or flat. lines: picket fences (build_area_dressing.KITS), "
    "points in order, closed, gates [X, Y, width]. noTrees: polygons. deferred: the new clutter "
    "(Art/Models/Props/FrontierClutter.py, look sheet 42) waiting for approval, by mesh, with counts; deferredPieces: "
    "those pieces in full, at the spots they keep (make_town.py APPROVED moves a mesh's into pieces, unmoved). lights: "
    "the pieces that carry SOCKET_Light, lit at dusk (offset: the socket in the piece's frame, cm; candelas by day and "
    "at dusk; reach cm; deferred with their piece). spots: the small animals' places (chickens in their yards, the "
    "porch cat), pending approval.")


def write(d, world, approved):
    placed = [p for p in d.pieces if p.placed]
    pieces = sorted((piece_json(p) for p in placed if not p.deferred), key=lambda e: e['id'])
    later = sorted((piece_json(p) for p in placed if p.deferred), key=lambda e: e['id'])
    deferred = Counter(p.mesh for p in placed if p.deferred)
    lights = []
    for p in placed:
        if p.mesh in LIGHTS:
            spec = LIGHTS[p.mesh]
            entry = {'id': f'light_{p.id}', 'piece': p.id, 'socket': 'Light', 'offset': list(spec['offset']),
                     'day': spec['day'], 'dusk': spec['dusk'], 'reach': spec['reach'], 'tags': ['Flicker']}
            if p.deferred:
                entry['deferred'] = True
            lights.append(entry)
    lights.sort(key=lambda e: e['id'])
    spots = []
    for s in d.spots:
        coop = s.pop('_coop', None)
        if s.get('kind') == 'chickens':
            if coop is not None and coop.placed:
                s['coop'] = coop.id
                # The coop's footprint, which the hens walk round (its ramp and run open on its front).
                s['avoid'] = [[rnd1(x), rnd1(y)] for x, y in coop.box.corners()]
            line = next((l for l in d.lines if l['id'] == s.get('line')), None)
            if line is None or not line.get('_ok', True):
                continue
        if s.get('kind') == 'cat' and world.probe.ok:
            # On the porch's deck: its lift from the house's own height, as the porch pieces'.
            host_z, ground = world.probe.actor_z(s['host']), world.probe.ground(*s['at'])
            if host_z is not None and ground is not None:
                s['lift'] = rnd1(max(50.0 + host_z - ground, 0.5))
        s['deferred'] = True
        spots.append(s)
    lines = [{k: v for k, v in l.items() if not k.startswith('_')} for l in d.lines if l.get('_ok', True)]
    dumps = lambda v: json.dumps(v, ensure_ascii=False)  # noqa: E731
    parts = ['{', f'  "about": {dumps(ABOUT)},', '  "pieces": [', ',\n'.join(f'    {dumps(p)}' for p in pieces), '  ],',
             '  "lines": [', ',\n'.join(f'    {dumps(l)}' for l in sorted(lines, key=lambda l: l['id'])), '  ],',
             '  "noTrees": [', ',\n'.join(f'    {dumps(n)}' for n in d.no_trees if n.get('polygon')), '  ],',
             f'  "deferred": {dumps(dict(sorted(deferred.items())))},',
             '  "deferredPieces": [', ',\n'.join(f'    {dumps(p)}' for p in later), '  ],',
             '  "lights": [', ',\n'.join(f'    {dumps(l)}' for l in lights), '  ],',
             '  "spots": [', ',\n'.join(f'    {dumps(s)}' for s in sorted(spots, key=lambda s: s['id'])), '  ],',
             f'  "approved": {dumps(sorted(approved))}', '}']
    with open(OUT_PATH, 'w', encoding='utf-8', newline='\n') as file:
        file.write('\n'.join(parts) + '\n')
    return pieces, later, lights, spots, lines


def check_lines(d, world):
    """Each chicken yard's fence: off the buildings, the roads and the spots, or the yard is left out (and its coop and
    hens with it)."""
    for line in d.lines:
        pts = [tuple(p) for p in line['points']]
        closed = pts + [pts[0]]
        problem = None
        for b in world.buildings.values():
            for part in b.walls + b.ways + [deck for deck, _ in b.decks]:
                if box_line_dist(part, closed) < 30.0:
                    problem = f'{b.id}'
        for rid, rpts, half, _ in world.roads:
            for a, c in zip(closed, closed[1:]):
                for q in (a, c):
                    if min(seg_point(r0, r1, q)[1] for r0, r1 in zip(rpts, rpts[1:])) < half + 40.0:
                        problem = f'the road {rid}'
        for name, spot, clear, kind in world.spots:
            if kind in ('tall',):
                continue
            if isinstance(spot, Box):
                if box_line_dist(spot, closed) < clear:
                    problem = name
            elif min(seg_point(a, c, spot)[1] for a, c in zip(closed, closed[1:])) < clear:
                problem = name
        if problem:
            line['_ok'] = False
            world.warnings.append(f'the chicken yard {line["id"]} meets {problem}: left out with its coop and hens')
            for p in d.pieces:
                if getattr(p, 'yard', None) == line['id']:
                    p.cls = 'dropped'


def dressing_entry(mesh):
    """A mesh's line for build_area_dressing.PIECES (and FOOTPRINT_AHEAD / _RIGHT where its footprint's middle is off its
    pivot)."""
    if mesh in DRESSING_PIECES:
        size, cull, ahead, right = DRESSING_PIECES[mesh]
    else:
        (x0, x1, y0, y1), height, _, kind = NEW_KIT[mesh]
        size = (round(y1 - y0), round(x1 - x0), round(height))
        cull = 'FENCE_CULL' if kind in ('wall', 'air') or mesh in ('Buckboard', 'ChickenCoop', 'KitchenGarden') else             'SMALL_CULL'
        ahead, right = (x0 + x1) * 0.5, (y0 + y1) * 0.5
    extra = ''
    if abs(ahead) >= 5.0:
        extra += f"  FOOTPRINT_AHEAD['{mesh}'] = {ahead:.0f}"
    if abs(right) >= 5.0:
        extra += f"  FOOTPRINT_RIGHT['{mesh}'] = {right:.0f}"
    return f"'{mesh}': (({size[0]}, {size[1]}, {size[2]}), 'middle', {cull}),{extra}"


def report(d, world, placer, warnings, pieces, later, lights, spots, lines):
    print(f'town.json: {len(pieces)} pieces now, {len(later)} deferred until approved, {len(lines)} picket lines, '
          f'{len(lights)} dusk lights, {len(spots)} animal spots -> {OUT_PATH}')
    allp = pieces + later
    print(f'instances in all: {len(allp)} of {len(set(p["mesh"] for p in allp))} distinct meshes '
          f'({len(set(p["mesh"] for p in pieces))} now, {len(set(p["mesh"] for p in later))} waiting)')
    print('pieces by mesh: ' + ', '.join(f'{m} {n}' for m, n in sorted(Counter(p['mesh'] for p in pieces).items())))
    print('deferred by mesh: ' + ', '.join(f'{m} {n}' for m, n in sorted(Counter(p['mesh'] for p in later).items())))
    print('by group: ' + ', '.join(f'{g} {n}' for g, n in sorted(Counter(p['group'] for p in allp).items())))
    print('lights: ' + ', '.join(f'{l["piece"]}{" (deferred)" if l.get("deferred") else ""}' for l in lights))
    need = sorted(set(p['mesh'] for p in allp) & set(NEEDS_PIECES) | set(p['mesh'] for p in allp if p['mesh'] in
                                                                       NEW_KIT))
    print('build_area_dressing needs these (PIECES: size along Y, across X, up; pivot; cull / FOOTPRINT_AHEAD, '
          'FOOTPRINT_RIGHT, cm):')
    for mesh in need:
        print('  ' + dressing_entry(mesh))
    placed = [q for q in d.pieces if q.placed]
    for pid, x, y, text in world.unframed:
        if any(abs(q.x - x) < 3000.0 and abs(q.y - y) < 3000.0 for q in placed):
            print('note: ' + text)
    for line in world.notes:
        print('note: ' + line)
    if placer.unmeasured:
        print(f'note: no terrain measured under {len(placer.unmeasured)} pieces (outside the probe regions): '
              + ', '.join(sorted(placer.unmeasured)))
    print(f'nudged ({len(placer.nudged)}):')
    for line in placer.nudged:
        print('  ' + line)
    print(f'dropped ({len(placer.dropped)}):')
    for line in placer.dropped:
        print('  ' + line)
    print(f'warnings ({len(warnings)}):')
    for line in warnings:
        print('  ' + line)


def main():
    approved = set(APPROVED)
    for k, arg in enumerate(sys.argv[1:]):
        if arg == '--approve' and k + 2 <= len(sys.argv[1:]):
            approved |= {a.strip() for a in sys.argv[k + 2].split(',') if a.strip()}
    unknown = approved - set(NEW_KIT)
    if unknown:
        raise SystemExit(f'--approve: no such new piece: {", ".join(sorted(unknown))}')
    layout = load(LAYOUT_PATH)
    if not os.path.exists(BOUNDS_PATH):
        raise SystemExit(f'{BOUNDS_PATH} is missing: write it from the editor first (Saved/mesh_bounds.py)')
    probe = Probe(load(PROBE_PATH) if os.path.exists(PROBE_PATH) else None)
    meshes = Meshes(load(BOUNDS_PATH), probe.data)
    world = World(layout, meshes, probe)
    if os.path.exists(OUT_PATH):
        try:
            old = load(OUT_PATH)
            world.forget_own(old.get('pieces', []) + old.get('deferredPieces', []))
        except ValueError:
            pass
    missing = sorted({m for m in ('Boardwalk_4m', 'Boardwalk_2m', 'Boardwalk_Steps', 'Awning_4m', 'Awning_Post',
                                  'LampPost', 'Bench', 'HitchRail', 'WaterTrough', 'Barrel_A', 'Barrel_B', 'Crate_A',
                                  'Crate_B', 'LaundryLine', 'FirewoodStack', 'Wheelbarrow', 'Cart', 'HayBale_Square')
                      if not meshes.has(m)})
    if missing:
        raise SystemExit(f'no bounds for {", ".join(missing)} in {BOUNDS_PATH}')
    d = design(world)
    name_pieces(d)
    defer(d, approved)
    check_lines(d, world)
    d.pieces = [p for p in d.pieces if p.cls != 'dropped']
    placer = Placer(world, d)
    place_all(d, placer)
    lifts(d, world)
    warnings = world.warnings + align_walks(d, world) + verify(d, world, placer)
    pieces, later, lights, spots, lines = write(d, world, approved)
    report(d, world, placer, warnings, pieces, later, lights, spots, lines)


if __name__ == '__main__':
    main()
