# RR_Home_B and RR_Home_I placed (towns plan step 2, DRESSING part 1; 2026-11-04)

The user's call (plan 4.2 item 2): the dogtrot (B, lived) and the L house (I, empty) are built and imported but stood nowhere.
Plan 4.4 gave B lot 3 (back lane behind the north side, X 1900..2700, Y 2500..5500) and I lot 5 (Pen Lane's end, X -2600..-3300,
Y 3700..4300). Neither lot has room. Placed instead where the checks hold (below); both are `level.props`, so no terrain pad,
no `layoutSha1` change, no generator run.

**homeB is left out (Main, 2026-11-04):** it stands 2.8 m from the Unpaid's rise spot inside the TownGate fight, which Phase 3 just tuned and
the user has not played. Its layout entries are archived as `towns_homeB.patch.json` (re-apply with `layout_patch.py RansomsRest <file> --check`
when it has a lot of its own); only `homeI` is in `layout.json`, so read the B rows and lines below as the archived plan.

## Why not lots 3 and 5 (numbers from layout.json + layout_computed.json + the probe; clearances rasterised at 50 cm)

- The town (X -4600..4200, Y -2600..9200 = 88 x 118 m) is 83% blocked at the margins below; a 9 m square fits in one place (the
  yard road's west side, where `homeI` stands) and, once the gate fight's ground is ignored, in the open ground south-west of the gate.
  Margins: buildings 1.0-1.5 m, obstacles 0.6-1.2 m, road edges 0.8-1.3 m, pits 4-6 m, pads 1 m, town.json pieces 1.7 m,
  roosts/graves/spawns 3-6 m; fights and encounters checked apart.
- B is 13.3 x 8.3 m (probe bounds X -583..246, Y -662..667), I 8.9 x 8.8 m.
- **Lot 3** (B): X 1900..2300 is the saloon's and hotel's backs and the hotel pad (X to 2107), Haskett's yard and chicken run;
  X 2300..3200 is the Sink's south slope: ground falls 13 cm at X 2380, the pit polygon is 10-12 m away, `sinkRoad` crosses the
  NW corner (its edge 92 cm from the lot), `saloonBacklot` (props obstacle, X 2600..3000, Y 3200..3800) fills the middle. No
  position, any yaw, clears all of these for B; I (smaller) fits at about (2625..2875, 4275..4525), but the probe grids stop at X
  2400 so its heights are unknown, on a slope. Needs a pad from TERRAIN (plan section 6) and `saloonBacklot` moved.
- **Lot 5** (I): Home_F (`penLaneEast`, X -3268..-2632, Y 2816..3750), `storeBacklot` (props obstacle X -2800..-2400, Y 3900..4400),
  Pen Lane's end (-2450, 3700) and `fieldsPath` (diagonal, 4.9 m off at X -3000) leave a pocket of about 9.5 x 8.3 m at X
  -3900..-2950, Y 3870..4700: I fits only at (-3320, 4510, yaw 0) with 14-165 cm to its neighbours and most of it outside the
  frozen ground and the probe (X < -3400). Rejected.
- Both lots' heights are frozen (`towns_except.json`) but nothing was ever padded for them: `layout.json` has no lot pads.

## Placed

| id | kind | location (X, Y) | yaw | front faces | ground (probe, cm) | clearance |
|---|---|---|---|---|---|---|
| `homeI` | RR_Home_I (empty) | (-1400, 7300) | 15 | north-north-east, Main Street's east end and the yard road's start (50, 7200)-(95, 7675), 14 m off | door 42.5; footprint 29..48 (vs door -14..+6); all 324 cells known; inside `zone_mainStreet` (frozen) | undertaker 222 cm, workshop 307, `yardTrough` 238, springhouse ruin 464, Main Street edge 435, yard road edge 719; no fight, encounter, roost, piece or pit near |
| `homeB` | RR_Home_B (lived) | (-1750, 50) | 0 | north, the backs of Home_J (`cottageSouthWest`) and the barber, 10 m from Main Street's edge | door -7.4; footprint -33..+12 (vs door -26..+20); 378 of 459 cells known (X < -2200 unknown) | barber 280 cm, Home_J 315, `burntLivery` ruin 279, `stockPens` fence 266, nothing on Main Street, Pen Lane or the farm road |

- `homeB`'s door is inside the plan's lot 6 box ("the west end behind Home_J", X -1400..-1800, Y -900..200, planned for M and P);
  its body runs 5 m south of the box. It is **inside the Main 3 gate fight's ground** (`TownGate`, 30 m round (0, -1100); B's centre is
  22 m out) and 277 cm
  from the Unpaid rise spot (-1300, -800) (`build_area_main3.UNPAID_SPOTS[3]`): the dead rise in front of its porch. No other
  house fits there or anywhere else in the town; the alternative is leaving B out until TERRAIN pads a lot.
- Households: C++ `HouseholdForMesh` already gives SM_RR_Home_B the generic `Cottage` household (heard through its walls); nothing
  to add to `build_area_townlife.HOUSEHOLDS`.
- Lights: `level.lights.homeB` = the lived houses' level (lamp 3 / 16 cd, reach 400, `gloomGlow` 4: cottageNorth's); `homeI` =
  `dark` (it has no WindowGlow slot, so nothing changes). `level.smoke` + `homeB` (Smoke socket; B's second chimney `Smoke_2` is
  not used by `effects()`).
- Not placed: yards, fences, outhouses, woodpiles (plan 4.4 menus: `town_rules.py`, DRESSING part 2). An outhouse needs its own
  ground check and the rules' placer; left to part 2.

## The files

- `Art/Levels/RansomsRest/layout.json`: `level.props` + homeB, homeI; `level.lights` + homeB, homeI; `level.smoke` + homeB (the
  patch, applied with `Tools/layout_patch.py RansomsRest` after `--check`, kept as `towns_homes.patch.json` beside this note).
- `Tools/Unreal/build_area.py`: `place_props()` (the props loop, with `"lift"` cm per prop), the `props` mode, `replace_house_lights()`
  and `prop_smoke()` (shared with the `lights` mode).
- `Saved/rr_town_probe.py`: regions `backLane`, `penLaneEnd`, `southWest` added for heights past the old grids.
- `Docs/Pipeline/Levels.md`: the `props` mode.
- `make_town.py` already reads `level.props` as buildings (`World.add_building`), so a rerun keeps off both houses.

## Run (editor open on Lvl_RansomsRest, no PIE; none run yet)

1. Placement: `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area.py RansomsRest props" -Until "props placed and saved"`.
   Takes out `townGate`, `homeB`, `homeI` and their `Smoke_*` by label, places them again (tag Obstacle, folder Buildings; the minimap
   draws them), places the plumes for B, then the houses' `Lights_<key>` and their households (the `lights` mode's work).
2. Heights (if the sill floats or sinks): `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Saved/rr_town_probe.py" -Until "TOWNPROBE done"`
   (30-90 s; now covers the three new regions), then add `"lift": <cm>` to the prop in `level.props` and run step 1 again.
   Rule: the house's floor is its pivot (the front door's sill); the mesh goes 35 cm under it, so ground up to 35 cm below the pivot
   is hidden and up to about 50 cm above it buries the wall's foot. Offline helper: the probe's `heights` (50 cm grid, x along nx)
   under the rotated bounds of `meshes["RR_Home_B"]`.
3. Grass and clutter keep off the new walls: `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_island_scatter.py RansomsRest"`
   (wait for the instance count to settle, save), then `py .../build_area_loot.py RansomsRest` and `py .../build_area_fauna.py RansomsRest`
   (roofs become perches).
4. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/path_probe.py RansomsRest" -Until "PATHS done"`; tests `Looter.UI.Minimap`,
   `Looter.World.HouseLights`, `Looter.World.RansomsRest.InsideBoundary`; a PIE walk past both with a Main 3 fight in B's reach.

## Check

- Log: `props placed and saved (2 placed, 1 of the last build taken out)` the first time (the town gate; homeI is new);
  `lights in N houses` with the count 1 higher than before (homeB), `1 left dark` more; no `no SM_RR_Home_B yet` warning.
- Outliner `RansomsRest/Buildings`: `homeB`, `homeI`; `Effects`: `Lights_homeB` (household Cottage), `Smoke_homeB`.
- Viewport: both sills on the ground, B's breezeway walkable end to end, nothing grown through the walls, I's front toward the
  street's east end.

## For the plan (needs the user or TERRAIN)

- The six new homes (K-P), the smithy, feed store, boarding house and bathhouse will meet the same lack of room: the lots in 4.4 were
  drawn on the layout without the buildings' real bounds. Free ground exists only at the two places above, the ground south-west of
  the gate (about 24 x 24 m, in the gate fight's ground) and the yard road's ends (reserved for the feed store and smithy).
  Larger lots need pads and moved props groups (`saloonBacklot`, `storeBacklot`) from TERRAIN, or the buildings go to the farms and
  the west road's stretches outside the town.

# The 16 new buildings (BUILDINGS lane): sites, 2026-11-05 (overnight; checked again 07:00 against the 02:54 probe; nothing applied)

Method as for B and I: 50 cm occupancy raster of layout.json + layout_computed.json + the probe + town.json AND town_next.json (7b's 299
pieces, 2 lines) + towns_except.json (every category of the rules: buildings, obstacles, roads +1 m, pits, pads, features, water,
cliffs, roosts, fight and encounter grounds, spots, boundary); every footprint shifted over the raster at 15 degree yaws; exact
clearances from the polygons after. Result: the valley has two pockets. Patch: `towns_buildings.patch.json` (props, lights, smoke,
fires; **not applied**; `layout_patch.py --check` passes).
**Correction at 07:00:** the first pass used the footprints from BUILDINGS.md (Blender x -> Unreal Y), which are mirrored in Y for the
lopsided models; the imported meshes' real bounds (probe `meshes`: Bathhouse Y -500..337, Home_L Y -205..346, Home_O Y -443..158,
Smithy X -744..54 Y +-490) moved the bathhouse 1 m and L 1.5 m, the privy and the O alternative no longer fit.

## Placed (3 of 16)

| id | model | location (X, Y) | yaw | faces | clearance (cm, real bounds) | ground (probe, all cells known) |
|---|---|---|---|---|---|---|
| `smithyTown` | RR_Smithy_Town | (-875, 9775) | 345 | the yard road, 9 m ahead (`yardRoad`) | coalStage 229, coffinShed 352, rail bed 449, yardRoad edge 645, gapTalus 302 | door 13.7; footprint -1..+27 vs door (+2 front, +11 middle, +20 back wall): **`lift` 10** |
| `bathhouse` | RR_Bathhouse | (-5275, 6275) | 270 | west: the field wall 2.5 m ahead, `fieldsPath` 10 m ahead | fieldWall 253, path edge 784, workshop 1621; a hay bale instance 186, stone wall pieces 266 | door 25.4; -16..+6 vs door: OK, no lift |
| `homeL` | RR_Home_L (shut) | (-4225, 6425) | 270 | west, the field wall 4.6 m ahead, `fieldsPath` 9 m ahead | amosFence 268, fieldWall 460, workshop 562, path edge 700, Tilly's cellar 364 | door 43.6; -11..+10 vs door: OK, no lift |

- Town dressing (HEAD `town.json` and 7b `town_next.json`): no piece inside any footprint; nearest the workshop yard's wheelbarrow, 520 cm
  from L (both files), no line (fence kit) within 60 m. `NoTrees_WorkshopYard` (scatter exclusion actor) touches L's footprint: harmless, the
  scatter keeps off it anyway. The smithy sits inside `NoTrees_CoalStage`: same.
- Bathhouse-L gap 508, smithy-L 31 m. Why there. Smithy: the rail yard's west end, coal stage and coffin shed at hand, the yard road in front;
  the only pose that clears everything (9 poses within 4 m, all this spot and its neighbours); the pocket holds no second building.
  Bathhouse, L: the meadow strip south of the workshop between the workshop yard / Amos's fence and the field wall (the wall runs
  north-south at Y 5700-6000 and splits the hay); both face west over the wall to Amos's `fieldsPath`. Reads as the town's south edge on
  the field lane, not as a street. Off every story fight ground, encounter spot/route, roost circle, loot spot, Main 3 street, the 3.5 m
  street lane, rivers, the Sink rim and cliffs (the raster categories); WestRoadCrew's new ground is far.
- Lights: all three shut/dark (rule: lived = boarding house, K, M, O). The bathhouse mesh has two lit panes and SOCKET_Light (shop);
  `"dark": true` leaves them out. A lamp for it is one line in the patch (`{"lamp": [2, 8], "reach": 300, "gloomGlow": 6}`).
- Smoke: smithy (SOCKET_Smoke), bathhouse (stovepipe; its vent `Smoke_2` is not used by `effects()`). Fires (D3): `level.fires` for both
  (`build_area.town_fires()`, SOCKET_Fire; both sockets are in the imported meshes' probe). The bathhouse's fire door faces local -Y, i.e.
  south (world -X), toward the open yard; the smithy's hearth is at local (-595, 305, 83), inside the walk-in.

## Not placed (13) and why

- **RR_BoardingHouse** (12.8 x 11.2 m): no pose anywhere in the valley, even ignoring every fight ground (0 of 0).
- **RR_Home_K** (9.7 x 9.1): 0 poses under the rules. Ignoring fight grounds: only the west road's chapel end by Boot Hill, 2.7 m from the
  dugout, inside WestRoadCrew's ground.
- **RR_Home_O** (the lived alternative to L): the fresh raster leaves no O pose within 9 m of the strip (the first pass's (-4375, 6475) used
  mirrored bounds); the `_O` patch was deleted. **RR_Home_M, N, P**: poses only in the same two pockets (earlier counts, mirrored bounds:
  M 16, N 456, P 296) and the pen lane's back (-4300, 2100: 19-20 m from any road; one building); they compete with the bathhouse and L
  for the same ground; not re-searched with the real bounds.
- **RR_FeedStore** (7.9 x 14 m): poses only in the strip facing no road (a feed store off every road is wrong). Ignoring fight grounds: the
  west road's chapel end at (4275, -1775, 345), 172 cm off the road and 261 from the dugout, inside WestRoadCrew's ground.
- **Privy, sheds A-C, smokehouse, root cellar**: they belong behind homes; only L is placed, shut, and the strip around it has no free ground
  big enough with known heights (the privy's only poses are 15 m away or on the smithy's front yard). Dropped; none placed.
- Ignoring all fight grounds adds only: the west road's chapel end (M, O, K, Feed; Main 7's crew ground, Boot Hill graves 3-6 m off) and O
  south-west of the gate (-25, -2225, 180: inside the Main 3 gate fight, 96 cm from the gate stones). Not taken.

## Options for Main (a decision, not done)

- More buildings need ground, not pads: move the `saloonBacklot`/`storeBacklot` props groups and give the lots of plan 4.4 real sizes, or
  build K-P, the boarding house and the feed store on the farms and the west road's stretches outside the town core (a new probe region each).

## Run order (when the patch is accepted)

1. (Done 02:54: the probe covers `yardWest`, `workshopSouth`, `penLaneBack` and the 16 imported meshes.)
2. `python Tools/layout_patch.py RansomsRest Docs/Work/2026-11-04-Towns/towns_buildings.patch.json --check`, then without `--check`; delete
   or archive the patch.
3. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area.py RansomsRest props" -Until "props placed and saved"`
   (props with the smithy's `lift`, plumes, house lights, households, `town_fires`).
4. As for B and I: `build_island_scatter.py`, `build_area_loot.py`, `build_area_fauna.py`, `path_probe.py`; tests
   `Looter.World.TownFire`, `Looter.World.HouseLights`, `Looter.World.RansomsRest.InsideBoundary`, `Looter.UI.Minimap`, `Looter.Encounters.Camps`;
   a PIE walk into the smithy (walk-in floor against the rising back ground, forge fire) and the strip south of the workshop.
5. `make_town.py` reads `level.props` as buildings, so its next run keeps off all three. CODEMAP lines for
   `Art/Models/Buildings/Outbuildings.py` and `TownBuildings.py` are the BUILDINGS lane's.

# Round 4: a back street on a pad for the 13 unplaced buildings (the user's call; Main 08:25, 2026-11-05)

**Verdict: the ground is not there, and a pad cannot make it.** A pad changes heights; every blocker below is something else.
Placed: 6 of the 13 (homes K, M, O lived + shed C, smokehouse, privy) as an **east row** on the yard road, no pad, no terrain
rebuild. Patch `towns_backstreet.patch.json` (`--check` passes, not applied). Not placed (7): Boarding House, Feed Store, N, P,
sheds A and B, root cellar.

## What the valley holds (measured, 2026-11-05; offline)
- Needs: footprints of the 7 big buildings 628 m2 (K 88, M 86, N 66, O 63, P 71, Feed 111, Boarding 143), with 3 m yards and the lane
  about 1,800 m2; the six outbuildings 4-17 m2 each.
- Method: the 50 cm occupancy raster of the 08:50 layout + the 02:54 probe + town.json AND town_next.json, every rule category
  (fights, encounter spots/routes/grounds, roosts, loot gameplay actors, Main 3 street, the Sink rim line +9 m, cliffs, water,
  buildings, obstacles, features, pads, roads +1 m, boundary), then relaxed: dressing pieces, random breakables (loot rerun),
  tree stands, props obstacles (stock pens, the Scatter boulders, the dray, the lumber stacks) treated as movable. The terrain's
  true heights come from the exported tiles (Intermediate/ArtExport/SM_RansomsRest_Tile_*.fbx, 215,536 triangles, rasterised on
  50 cm; equals the probe to 0.3 cm): ground everywhere is lumpy within +-20 cm, so slope never limited anything.
- Result even so: the largest free rectangles are **12 x 24 m** (a handful of places); connected free ground is 750 m2 east of the
  yard road and about 800 m2 behind the south side. The strict raster has no free 10 x 10 m at all.
- **Behind Main Street's north side (the pitch):** X 2250..3500 holds 218 m2 connected even with the two Main 4 roaming fights
  (NorthRoad, BootHill) moved away: the MiserPlot ambush ground (to Y 1850), the north road / Sink road diagonal, the hanging tree, the
  saloon backlot, the Sink pit 9 m margin (pit polygon from X 2740) and the town's own backs (X 2250). The Sink fall is not the
  limit. Its east end, where the Sink road is far, is where the row below stands.

## Chosen: the east row (north verge of the yard road, Main Street's east end)
| id | model | location (X, Y) | yaw | lived | clearance (cm) | ground vs door |
|---|---|---|---|---|---|---|
| `homeK` | RR_Home_K | (1100, 6950) | 180 | yes | cottageNorthEast 271, Main St edge 296, yard road 478, Sink margin 986 | -12..+16 |
| `homeM` | RR_Home_M | (1350, 8150) | 180 | yes | the dray (yardWagon) 139, yard road 639, lumber stacks 653, Sink 1147 | -30..+1 |
| `homeO` | RR_Home_O | (1650, 9050) | 180 | yes | lumber stacks 105, yard road 895, Sink 1467 | -9..+17 |
| `shedC` | RR_Shed_C | (2275, 8475) | 180 | - | M 268, O 251, Sink 967 | -4..+13 |
| `smokehouse` | RR_Smokehouse | (2775, 8925) | 180 | smoke | O 288, shed C 196, Sink 959 | -13..+8 |
| `privyB` | RR_Privy_B | (2775, 9325) | 180 | - | O 284, smokehouse 156, Sink 1297 | -8..+2 |
- Gaps K-M 243, M-O 245. Doors face the yard road (south-west, -X); porch lines step back 846, 1096, 1405 along the road slow
  diagonal, so the verge widens east (3 m at K to 9 m at O: the dressing front/verge zones, street events). Rear yards: the
  outbuildings stand 7-13 m behind M and O; the rear of K (X 1754..1950) is a 2 m strip before the cottonwood sliver and the Sink margin.
- No dressing piece (town.json HEAD or town_next.json 7b) falls inside any footprint (nearest: mercer_barrel_b_01 238 from K, the weed
  fringes of H 292). Off every fight ground, encounter spot/route, roost circle (the undertaker yard, Y >= 9300 west of X 1500), the Main 3
  street, loot gameplay actors (the random breakables Barrel_A_845_8378 and Crate_A_699_8282 stay outside, 209 and 358 from M).
- **No pad:** K, M, O stand inside the frozen zone_mainStreet / zone_depot / lotFeedStore (terrain_compare must keep them at
  0.00 cm) and their ground is within +-30 cm of each door: nothing to level. A pad here would break the exceptions.
- Lights: K, M, O lived (lamp 3 / 16 cd, reach 400, gloomGlow 4, their SOCKET_Light and lamplit panes); all smoke (K, M, O, smokehouse).
  The outbuildings have no lit glass. Fires: none.
- Obstacle sinkRoadCottonwoods (the stand hiding the Sink from the street) ran through the row; the patch pares it to a strip behind
  it (X 1950..2640, Y 6550..8150; trees need the scatter rerun). The houses now do the hiding.
- Households (TownLife): build_area_townlife.HOUSEHOLDS knows no K/M/O: add homeK = Cottage, homeM = CottageSouth, homeO = CottageNorth
  (existing household names; not done here, a script outside this job) or the three stay silent.

## Considered and rejected
- **South back (the only place a pad would work):** the band X -5350..-3900, Y -500..2550 (the Scatter boulders and the stock pens
  taken out) holds two buildings: Boarding House at (-4490, 955, yaw 0) + N at (-4432, 2100, yaw 0) on a lane at X -4000, or K + M. Its
  pad, if ever wanted: southBackPad center [-4640, 1300], size [1480, 2300], yaw 0, corner 0.15, height 10 (mean ground 11, min -30,
  max 34), bank slope 25, round 200, toe 150, reach 100 (reach 100 keeps backPenLane and whitlockYardPad at 0.00). **No way in:**
  Pen Lane alley between the burnt livery and Home C is closed by the ruin obstacle polygon (to Y 2200; C starts 2239, 39 cm), and the
  route from Amos gate crosses the Whitlock encounter ground (Y 4900..5400) and the Side 2 bale discs. A paint-only lane (paint true)
  would carry no heights. Dropped; 2 buildings do not repay a terrain rebuild.
- East of the yard road other combinations: Boarding + M, Boarding + O, Boarding + K, Feed + K + O fit (2-3 buildings, max 3 anywhere
  in the strip); nothing else fits once K, M, O stand. Swap M for the Boarding House if the lived boarding house matters more than M.
- N and P, Feed Store, sheds A and B, root cellar have no free spot beside the row; the pen lane back (-4325, 1975) takes one house
  (22 m off a road).

## What would give more room (decisions, not done)
1. The Main 3 gate fight ground (r 30 m): 665 m2 connected in the west meadow (X -1400..2400, Y -4000..-250), about 4-5 buildings.
   Phase 3 just tuned that fight.
2. Remove the burnt livery ruin (placement, footing, obstacle): opens the alley to the south back (2 buildings, with the pad above).
3. Move the Side 2 bales and shrink the WhitlockHands ground: the south back frontage grows 20 m (2 more homes), still no access.
4. Nothing on the north side helps (218 m2).

## Plan for DRESSING (lot types; town_rules.py already types these models: homeLived, outbuilding)
- Add homeK, homeM, homeO, shedC, smokehouse, privyB to DRESSED (their ids are not in the table; MODEL_TYPES covers them).
- Lot K: X 700..1900, Y 6350..7500 (front on the verge; side alley 2.7 m to cottageNorthEast: rain barrel, tools leaning; rear: a
  laundry line against the cottonwood strip). Lot M: X 800..2150, Y 7650..8700 (the dray parked at the west side of its porch is already
  there: make it the lot front vignette; rear: shed C, a garden). Lot O: X 1300..2900, Y 8800..9500 (lumber stacks on its west side
  stay; rear: smokehouse, privy, woodpile). Yard fences on the side lines between lots (2.4 m gaps: picket, broken 0-0.1).
- Street events on the verge: not within 3 m of a porch line; the undertaker yard roosts at Y >= 9300 keep their 9 m circles.

## Run order (east row; no pad, so no generator, import, beyond or cliffs)
1. python Tools/layout_patch.py RansomsRest Docs/Work/2026-11-04-Towns/towns_backstreet.patch.json --check, then without --check;
   archive the patch.
2. DRESSING: DRESSED + HOUSEHOLDS (above); make_town.py reads level.props as buildings (town.json); 7b copy of town_next.json over
   town.json goes in the same pass.
3. Editor: Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area.py RansomsRest props" -Until "props placed and saved"
   (props, plumes, house lights, households).
4. build_island_scatter.py RansomsRest (the cottonwood strip and the grass keep off the new walls; wait for the count, save), then
   build_area_dressing.py, build_area_fauna.py (roof perches), path_probe.py RansomsRest. Loot rerun not needed (no breakable
   falls in a footprint).
5. Tests: Looter.World.HouseLights (3 more lit houses), Looter.World.TownLife, Looter.World.RansomsRest.InsideBoundary,
   Looter.UI.Minimap, Looter.Encounters.Camps; Tools\tour.ps1 + Tools\perf.ps1 (3 houses + 3 outbuildings, 3 lights, 4 plumes at
   Main Street east end: the town view toward the Sink and the depot).
