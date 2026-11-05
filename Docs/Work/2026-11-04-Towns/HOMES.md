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

# The 16 new buildings (BUILDINGS lane): sites, 2026-11-05 (overnight; nothing run, nothing applied)

Method as for B and I: 50 cm occupancy raster of layout.json + layout_computed.json + the probe + town.json + towns_except.json (every
category of the rules: buildings, obstacles, roads +1 m, pits, pads, features, water, cliffs, roosts, fight and encounter grounds, spots,
boundary); every footprint (BUILDINGS.md bounds) shifted over the raster at 15 degree yaws; exact clearances from the polygons after.
Result: the valley has two pockets. Patch: `towns_buildings.patch.json` (props, lights, smoke, fires; **not applied**; `--check` passes).

## Placed (4 of 16)

| id | model | location (X, Y) | yaw | faces | clearance (cm) | ground |
|---|---|---|---|---|---|---|
| `smithyTown` | RR_Smithy_Town | (-875, 9775) | 345 | the yard road, 8.5 m ahead (`yardRoad`) | coalStage 248, coffinShed 395, rail bed 449, yardRoad edge 645, gapTalus 302 | door 13.7; 26 of 320 cells known: 13.2..19.0, vs door 0..+5 |
| `bathhouse` | RR_Bathhouse | (-5275, 6375) | 285 | west, `fieldsPath` 10 m ahead | fieldWall 292, workshop 1462, path edge 905 | **unknown** (0 of 204 cells) |
| `homeL` | RR_Home_L (shut) | (-4075, 6375) | 270 | west, `fieldsPath` 9 m ahead | amosFence 252, fieldWall 412, workshop 553, path edge 650, undertaker 1732 | **unknown** (0 of 240) |
| `privyB` | RR_Privy_B | (-5875, 6375) | 30 | beside the bathhouse's south wall (gap 195) | fieldWall 204, path edge 928 | **unknown** |

- Why there. Smithy: the rail yard's west end, coal stage and coffin shed at hand, the yard road in front: the only spot where a 9.8 x
  8 m open-front shop clears everything with a road ahead (33 poses, one cluster; the pocket holds no second building). Bathhouse, L, privy: the strip south of the workshop between the workshop yard and Amos's field wall,
  facing the fields path (the field hands' way); Bath-L gap 358, Bath-privy 195 (all stamped with 150 cm margins). Both pockets are
  off every story fight ground, encounter spot/route, roost circle, loot spot, Main 3 street, the 3.5 m street lane, rivers, the Sink
  rim and cliffs (the raster categories); WestRoadCrew's new ground is far.
- **Heights are the open question.** The probe (23:31) stops short of both pockets (the smithy's 294 cells and all of B are unknown).
  `Saved/rr_town_probe.py` already has `yardWest` (-1900..-300, 9200..10900) and `workshopSouth` (-6000..-3800, 5900..7400): run it,
  then check each footprint's ground against the door's: it must stay within -35..+45 cm of the door ground (the mesh goes 35 cm under
  the pivot, ~45 cm above buries the wall's foot; about 3% over a 10 m footprint). Over that: drop the entry (no pad: pads do not create
  room, the limit is occupancy), or add `"lift"` for a small sink.
- Lights: all four shut/dark (rule: lived = boarding house, K, M, O). The mesh has two lit panes and SOCKET_Light on the bathhouse (shop);
  `"dark": true` leaves them out. A lamp for it is one line in the patch (`{"lamp": [2, 8], "reach": 300, "gloomGlow": 6}`).
- Smoke: smithy (SOCKET_Smoke), bathhouse (stovepipe; its vent `Smoke_2` is not used by `effects()`). Fires (D3): `level.fires` for both
  (`build_area.town_fires()`, SOCKET_Fire; needs the models imported with their sockets, else a warning and no fire).
- **Alternative `towns_buildings_O.patch.json`**: RR_Home_O (lived) at (-4375, 6475, 255) instead of L (gap to the bathhouse 227; fieldsPath
  10.5 m ahead; amosFence 265, workshop 527). Lamp 3/16 cd, reach 400, gloomGlow 4, smoke. Apply one of the two files, never both.
  Households: `HouseholdForMesh` only knows A/B/D/E/H, so O has none (silent inside) until one is added to `build_area_townlife.HOUSEHOLDS`.

## Not placed (12) and why

- **RR_BoardingHouse** (12.8 x 11.2 m): no pose anywhere in the valley, even ignoring every fight ground (0 of 0).
- **RR_Home_K** (9.7 x 9.1): 0 poses under the rules. Ignoring fight grounds: only the west road's chapel end by Boot Hill, 2.7 m from the
  dugout, inside WestRoadCrew's ground; heights unknown.
- **RR_Home_M**: 16 poses, 12 in the strip above (7 facing the path) and 4 in the smithy's pocket (1 facing the yard road); it competes with
  Bath/L/O for the same ground. Not taken: lived houses get lights and a household, and this strip is a field edge, not a street.
- **RR_Home_N, RR_Home_P**: poses only in the same two pockets (N 456, P 296; in the strip they face the path at 11 m) and in the pen lane's
  back (-4300, 2100: 19-20 m from any road, heights unknown, one building). A pocket holds one more of them only by dropping L/O.
- **RR_FeedStore** (7.9 x 14 m): 6 poses in pocket B, none facing a road. Not placed (a feed store off every road is wrong). Ignoring fight
  grounds: the west road's chapel end at (4275, -1775, 345), 172 cm off the road and 261 from the dugout, inside WestRoadCrew's ground.
- **Sheds A-C, smokehouse, root cellar**: they belong behind homes; the homes that fit are placed shut and the strip has no yards. Candidate
  spots exist behind the smithy (shed B or the smokehouse at about (-525..-575, 9875..9925), beside the smithy's front: too close to its
  yard) and none behind L. Only the privy is placed.
- Ignoring all fight grounds adds only: the west road's chapel end (M, O, K, Feed; Main 7's crew ground, Boot Hill graves 3-6 m off, heights
  unknown) and O south-west of the gate (-25, -2225, 180: inside the Main 3 gate fight, 96 cm from the gate stones). Not taken.

## Options for Main (a decision, not done)

- More buildings need ground, not pads: move the `saloonBacklot`/`storeBacklot` props groups and give the lots of plan 4.4 real sizes, or
  build K-P, the boarding house and the feed store on the farms and the west road's stretches outside the town core (a new probe region each).
- If the probe shows the strip too steep: the pen lane's back (-4325, 1975) takes one house (O, 22 m off any road, heights unknown).

## Run order (when the patch is accepted)

1. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Saved/rr_town_probe.py" -Until "TOWNPROBE done"` (30-90 s; covers `yardWest`,
   `workshopSouth`, `penLaneBack`); then re-check the four footprints' ground (rule above); drop or lift as needed.
2. Import the lane's models (BUILDINGS lane; `Intermediate\ArtExport_Buildings2`, 16 models + the outbuildings) so `SM_RR_Smithy_Town`,
   `SM_RR_Bathhouse`, `SM_RR_Home_L` (or `_O`), `SM_RR_Privy_B` exist.
3. `python Tools/layout_patch.py RansomsRest Docs/Work/2026-11-04-Towns/towns_buildings.patch.json --check`, then without `--check`; delete
   or archive the patch (and the unused `_O` one).
4. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area.py RansomsRest props" -Until "props placed and saved"`
   (props, plumes, house lights, households, `town_fires`).
5. As for B and I: `build_island_scatter.py`, `build_area_loot.py`, `build_area_fauna.py`, `path_probe.py`; tests
   `Looter.World.TownFire`, `Looter.World.HouseLights`, `Looter.World.RansomsRest.InsideBoundary`, `Looter.UI.Minimap`, `Looter.Encounters.Camps`;
   a PIE walk to the smithy (walk-in floor, forge fire) and the strip south of the workshop.
6. `make_town.py` reads `level.props` as buildings, so its next run keeps off all four. CODEMAP: lines for `Art/Models/Buildings/Outbuildings.py`
   and `TownBuildings.py` are the BUILDINGS lane's.
