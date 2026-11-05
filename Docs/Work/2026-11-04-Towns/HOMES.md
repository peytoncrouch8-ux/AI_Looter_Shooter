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
