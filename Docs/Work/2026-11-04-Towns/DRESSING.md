# DRESSING lane, step 4 (PROPS round 1 + DRESSING part 2; 2026-11-04 night)

Built: the rules, Main Street's street and storefront clutter, the ground strips, the walk and lamp variants, Pen Lane, the
back lots, the yards, homeB and homeI. Not built: the farms' FarmYard set (step 6; the Farm view is over its line), yard fences
round the home lots (below), per-instance tint data (needs LIGHTS' `AInstancedProps` custom data). Nothing in git, Unreal not run.

## Files
- `Art/Levels/RansomsRest/town_rules.py` (new, stdlib): KIT (all 48 PROPS meshes measured from the reports: box, height, solid,
  kind, slots, props weight, cull, shadow), CHOSEN (the town's 18 new types), FRAMES (RR_Home_B, RR_Home_I), TYPES/MODEL_TYPES,
  MENUS per type (zone, spot rule, mesh), SKIP, FRINGE (weeds per type), street() (events, crossings), extras() (hand spots),
  walk/lamp variants (WALK_SWAPS, LAMP_SWAPS), rule_spots() (reads build_area_landfall.TOWN_SPEAKERS, build_area_main3's
  checkpoint and UNPAID_SPOTS, the layout's encounters' spots, beats and routes by AST/JSON), density(), zones_table(), draws(),
  LEAN. A new building needs only its type: TYPES key or MODEL_TYPES model; no frame -> dressed from its mesh bounds, door at
  pivot (note printed: add a FRAMES entry for better).
- `make_town.py`: imports town_rules; `--out <file>` (default town.json; I only write `town_next.json`), `--skip Mesh,...`;
  classes event/flat/weed; rule-only spot kinds (rspawn, rspot, speaker); rule pieces keep 3 m off spawn points and routes; events
  keep the middle 3.5 m lane (175 cm off mainStreet/yardRoad's middle line); ground cover touches walls and porches, keeps off
  ways, steps, posts, solids, rise spots; wall pieces of the rules keep off doors (+30), lit windows (probe Window_ sockets) and
  high posters/letters/chalk/shutters (80); alternatives (alts) before nudges; follow (rings on posts). pin(): the town as built
  (town.json: what the level was last built from) keeps its spots (by id+mesh within 3 m, or mesh+spot): pinned pieces
  skip spawn, loot and roost checks (the fights were laid out round them). forget_own(): town-only meshes, any old spot (450 cm
  prefilter: a walk's middle is 2 m off its pivot), design spots, picket sections on the town's lines are own. Probe roosts the
  layout no longer has are ignored (Roost_MainStreet, FamilyPlot, SinkRim). WindowShutter = tall spot 40; ReadableProp and
  CompanySign high on a wall = tall 80; CompanyLoudspeaker 60. GROUND_RANGE walk 30 -> 35. Store barrel 1.2 m off the
  loudspeaker post. ID_BASE: lamp variants count with LampPost (ids kept).
- `Tools/Unreal/build_area_dressing.py`: PIECES + FOOTPRINT_AHEAD/RIGHT for all 48 + 3 walk variants; WEED_CULL 4000,
  WALK_VARIANT_CULL 15000 (90 m Medium: keeps variants out of the Farm view); CLUTTER_SHADOWLESS + weeds, ground cover, bills,
  low piles, LampPost_B, walk variants; SHADOWLESS_BY_AREA RansomsRest: Boardwalk_4m/2m/Steps, Bench, WaterTrough, Washtub (other
  levels untouched); PASSABLE + WeedFringe_, PlankCrossing_, StrawLitter_, FallenBoards_, Handbills_ (no Obstacle tag);
  TOWN_KINDS flat + those; GRASS_UNDER + WeedFringe_, FallenBoards_. Parses; plan_all/footprints run offline on town_next.json
  (stubbed unreal): 298 pieces, 0 warnings, no mesh missing from PIECES.
- `Art/Levels/RansomsRest/town_next.json` (generated). town.json untouched (HEAD). No layout patch needed. build_area_townlife:
  nothing (C++ HouseholdForMesh gives SM_RR_Home_B "Cottage").
- Plan views: `Saved/ArtPreviews/Towns/Step4/Plan_*.png` (top-down: orange new solid, blue as built, green weeds, yellow ground
  cover, magenta bills/corner lamps, red rings spawn 3 m, blue rings roost 2.5/9 m).

## The 18 new types (cap 24)
WeedFringe_A, _B, _C; PlankCrossing_A; StrawLitter_A; FallenBoards_A; Handbills_A; CrateStack_A, _B; BarrelStack_A;
WaterBarrel_Cart (CLUTTER_CULL: 66 m from the Farm camera); WagonFreight_Canvas; Vignette_Alley_A; ToolsLeaning_A; Ladder_A;
Boardwalk_4m_Patched; LampPost_B (shadowless), LampPost_C. Left out (draws): Boardwalk_4m_Worn (3 slots), Vignette_PorchEmpty_A
(3 slots, one porch with room: the hotel's, done with a tipped PorchChair + BottleCrate + FallenBoards), PorchLived/BackYard (kit
pieces tell it at 0 draws; homeB is in the Farm view at 56 m), ShopFront A/B (no open shop has a free front: step 6), Handbills_B,
singles, plain/broken wagons, PlankCrossing_B, Barrel_C, Steps_Worn, BarrelFire (main3's), signs, FarmYard.

## What's in town_next.json (299 pieces; HEAD 112; 12 lights = HEAD's)
Rules placed 179 of 260 (drops are ground cover mostly): weeds 43 A + 58 B + 13 C rings; FallenBoards 13; Handbills 8 (side walls
at street corners: assay W, sheriff W, hotel E, barber E, store W+E, undertaker E, workshop); PlankCrossing 4 (store's porch over
the south rut, yard road mouth, Pen Lane, fields path); events: water cart by the shack (-380, 450), canvas wagon opposite the
hotel (-400, 4985), barrels + crates at the east end; alley junk 4 (sheriff W wall, barber E (menu), store W between windows,
homeI's wing); stacks 7; ladders 2; tools 3; kit pieces (rocker, washtub, broom, firewood, block, laundry at homeB; chair at
Haskett's; spittoon, broom at the sheriff's; tipped chair, bottle crate at the hotel; hotel hitch rail; drums).
- HEAD pieces changed: lamps 02 (482, 2830) -> LampPost_B on the Gilt Spur's front-west corner 3 m up, 07 (-482, 1760) ->
  LampPost_B on the barber's front-east corner, 08 (-482, 4660) -> LampPost_C same foot (one light each, same count);
  saloon_boardwalk_4m_02 -> Patched (and undertaker_boardwalk_4m_01, restored); store_barrel_a_01 (-750, 2580) -> (-760, 2700)
  (a Barrel_A: loot candidate moved 1.2 m; it clipped Loudspeaker_MainStreet). Everything else at HEAD's spot.
- Restored (dropped since 10-14 for 31-34 cm uneven ground): assay_boardwalk_4m_03 + sheriff_porch_chair_01; undertaker's walk
  (2 sections, steps) + bench + broom; its barrel is Barrel_B now (not breakable) and still dropped (Speaker_Tilly). Warnings:
  those walks sit 30-35 cm off their floors, sink clamped at 30 (up to 5 cm step at the facade: check in PIE).
- Dropped of HEAD's design as before: store_bench_01 (Vendor). saloon_bottle_crate_01 stays (letter on the wall now a tall spot).

## Density (Main Street, 10 m bins gate -> east, the last 3 m in bin 8; target 5-9 at the front, cap 14)
```
north front  1 4 5 5 7 5 9 4  mean 5.0   (HEAD all 0 2 2 6 7 2 10 2 = 3.9)
south front  0 4 5 3 8 3 6 4  mean 4.1   (HEAD all 0 2 4 2 6 1 3 0 = 2.2)
north props  2 11 5 10 7 5 14 12 mean 8.2;  south props 1 6 10 8 12 5 6 10 mean 7.2   (vignette/stack = what it shows)
```
Below 5 at the front, and why: bin 1 (gate) both sides = Main 3's checkpoint and rise spots; south 4 (Pen Lane mouth) and 6 (fields
path mouth) = road mouths + the set piece's rise spots at (-400, 2200), (-800, 2250), (-400, 4400) every few metres on the strip;
north 2 = the roost's 2.5/9 m circles and the speaker post. Zones (front/side/back per building): every building >= its minimum
(lived 3+, empty 2+) except penLaneEast front 0 (its front is 0.8 m from Pen Lane: weeds only).

## Draws (estimate, make_town's report; Medium, no occlusion counted)
| view | new types | added draws low..high | shadow draws saved | with --skip LEAN |
|---|---|---|---|---|
| MainStreet | 13 | +28..97 | -8..32 | +17..59 |
| MainStreetMid | 14 | +33..109 | -8..32 | +22..71 |
| MainStreetEast | 14 | +33..107 | -8..32 | +20..63 |
| PenLane | 5 | +11..16 | 0 | +11..16 |
| BackLotsNorth | 11 | +25..79 | -9..36 | +16..48 |
| Farm | 0 (5 kit instances at homeB) | 0 | 0 | 0 |
| ChapelOverTown | 4 | +14..57 | -5..20 | +10..45 |
Low = one batch a slot + one cascade a slot for casters; high = 3 LODs, 2 cascades. Net low +20..25 in the street views (the plan
budgeted +20-40). LEAN = `--skip LampPost_C,LampPost_B,Ladder_A,CrateStack_B,WeedFringe_C,Boardwalk_4m_Patched` (the first
payback; the lamps fall back to LampPost on the same spots). Triangles: weeds ~120k in all, culled at 24 m.

## What Main runs (in order)
1. Swap: copy `town_next.json` over `town.json` (or rerun `make_town.py` without --out).
2. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/build_area.py RansomsRest dressing"` (dressing, town lamps,
   lootable world). Check its log: no "no SM_<mesh> yet", "no size for", lamps "12 town lamps".
3. `build_island_scatter.py RansomsRest` (the new footprints keep the scatter off; grass grows under weeds and boards), wait, save.
4. `build_area_fauna.py RansomsRest` (homeB's laundry line), `path_probe.py RansomsRest`.
5. Tour MainStreet, MainStreetMid(+_Dusk/_DuskSmog), MainStreet_Smog, MainStreetEast, PenLane, BackLotsNorth, YardRoad, Farm,
   ChapelOverTown; split `Looter.Perf.HideTag Dressing`. Over 8.3: rerun make_town with `--skip` from LEAN, rebuild 2.
6. Tests: `Looter.Breakable`, `Looter.Economy.Balance` (store barrel moved: candidate order may change, count should not),
   `Looter.World.HouseLights` (3 lamps moved), `Looter.UI.Minimap`, `Looter.Encounters.Camps`; PIE: Main 3 gate + street fight,
   the restored walks' facade step, homeB's porch, the hotel porch.
7. Fresh probe, then `make_town.py --out town_next.json`: it must come out the same (twice-run stable on today's probe).

## Not verified / open
- Nothing seen in Unreal: wall-piece heights (bills 1.45 m, corner lamps 3 m), weeds' fit to the real wall faces (KIT boxes),
  the tipped chair's pose (tilt roll 90, lift 61), the canvas wagon's facing (tongue east).
- Yard fences round lots (plan 4.4): not done. check_lines only checks buildings, roads and spots (not trees, rocks, groups or
  pieces) and fences would cut the set piece's alley routes; wants a lot plan (part 3).
- For GROUND: paint wet (G) under PlankCrossing_* and churn (B) under StrawLitter_* from town.json when the wear atlas comes.

# Round 2 (2026-11-05 morning; after DEMO2.md's PIE check of f4a0a3c7)
`town_next.json` regenerated on the 02:54 probe (twice: byte-identical); town.json untouched. 338 pieces (built: 299); 17 new
types (BarrelStack_A gone). Before the changes, a rerun on that probe gave the built town exactly back (only the new buildings'
dressing added): the as-built pinning holds after a build.

## Rules added (town_rules.py; make_town.py's placer)
- SPACING: a merged vignette never within 15 m of itself, a stack never within 6 m of its twin (rule pieces; make_town checks
  placed pieces). Pen Lane's facing pair: the store's west-wall junk stays; the barber's east-side one is in SKIP (its next
  spot, the barber's west wall, is 66 m from the Farm camera, inside its 72 m draw distance).
- Crossings only over ruts or mud (`Rules.unfit`): on a dirt road's bed (half width - 60 cm; Main Street counts: its dust has
  ruts) or within 2.5 m of a trough, pump, well or the water cart. Paths have no ruts to bridge.
- Stacks only where someone stacked them: within 2.5 m of an open store's or a yard's wall (shopOpen, yard), of a wagon or cart's
  box (Buckboard, Cart, Dray, WagonFreight*), or 4 m of a back-lot group (storeBacklot, saloonBacklot, yardStacks, yardWagon).
- Refused this run: BarrelStack_A (400, 6700), CrateStack_A (430, 6420) (east end), CrateStack_A (755, 430) (the shut assay's
  walk), PlankCrossing_A (-1300, 2060) (Pen Lane), (-1250, 4530) (fields path). Kept: Pruitt's crossing over the south rut, the
  yard road's mouth; the buckboard's crates (its wagon), the store's, undertaker's, workshop's stacks.
- Menu class: the rules' kit pieces place as 'small' (they placed 'big' when over 1.6 m, ahead of the hand extras).
- FRAMES for the BUILDINGS lane's models (BUILDINGS.md tables; porch/dock floors 0.45 m where it gives none): RR_Smithy_Town
  (open front = an 8 m way in), RR_Bathhouse (+ boiler), RR_FeedStore (1.0 m dock, steps), RR_BoardingHouse (+ outside stair),
  RR_Home_K, L, M, N (+ lean-to), O, P. homeL, bathhouse, smithyTown (placed in the layout since) are dressed by their type.

## Fronts toward 5-9 (kit pieces only: no new draws)
East end: a WaterTrough (525, 6640) and HitchRail (515, 6959), the riders' tie-up where the yard road comes in (as the gate has).
Pruitt's second rail (-462, 2700). A trough by the wagon's team at the fields path's mouth (-695, 4745). BottleCrate (772, 470)
and a Spittoon (765, 333) on the assay's walk. MilkCans (750, 6090) by the Mercers' porch. Washtub (-585, 600) by the water cart.
Coffin_Closed (-730, 6700) by Bright & Daughter's crates (drawn already in the yard; out of the MainStreet camera's 72 m).
```
north front  1 5 5 5 7 5 9 5  mean 5.2   (round 1: 1 4 5 5 7 5 9 4 = 5.0)
south front  0 5 5 4 8 4 6 5  mean 4.6   (round 1: 0 4 5 3 8 3 6 4 = 4.1)
```
Still under 5: both gate bins (Main 3's checkpoint), south 4 (Pen Lane's mouth: rise spots (-400, 2200), (-800, 2250)) and 6
(the fields path's mouth: (-400, 4400)). Zones: smithyTown front 0 (open front: the forge inside is its front), homeL and
penLaneEast front 0 (empty: weeds and boards).

## Cost
Against the built town: -1 type (BarrelStack_A) in MainStreetMid, MainStreetEast, BackLotsNorth; Coffin_Closed is drawn in the
yard already; YardRoad +2 types (the smithy's Handbills_A, Ladder_A: +3..11 draws). Expected well under +0.1 ms. The table's
totals (vs the town before the towns round): MainStreet +28..97, MainStreetMid +31..98, MainStreetEast +31..97, PenLane +9..14,
BackLotsNorth +23..70, YardRoad +3..11, Farm 0, ChapelOverTown +10..39.

## Diff of placements (town.json as built -> town_next.json; weeds summarised)
```
+ assay_bottle_crate_01 BottleCrate (772, 470)          - assay_crate_stack_a_01 CrateStack_A (754, 420)
+ assay_spittoon_01 Spittoon (765, 333)                  - cabinSouth_vignette_alley_a_01 Vignette_Alley_A (-1215, 1607)
+ mainStreet_hitch_rail_06 HitchRail (-462, 2700)        - penLane_plank_crossing_a_01 PlankCrossing_A (-1300, 2060)
+ mainStreet_hitch_rail_07 HitchRail (462, 4690)         - street_barrel_stack_a_01 BarrelStack_A (400, 6700)
  (the hotel's rail, was _05: ids follow the design)     - street_crate_stack_a_01 CrateStack_A (430, 6420)
~ mainStreet_hitch_rail_05 HitchRail -> (515, 6959)      - street_plank_crossing_a_03 PlankCrossing_A (-1250, 4530)
+ mainStreet_water_trough_03 WaterTrough (525, 6640)     ~ undertaker_barrel_b_02 Barrel_B (-700, 6640) -> (-600, 6486)
+ mainStreet_water_trough_04 WaterTrough (-695, 4745)    ~ workshop_crate_stack_b_01 CrateStack_B (-2797, 5858) -> (-2835, 5851)
+ mercer_milk_cans_02 MilkCans (750, 6090)
+ street_washtub_01 Washtub (-585, 600)
+ undertaker_coffin_closed_01 Coffin_Closed (-730, 6700)
+ bathhouse_ (CrateStack_A, CrateStack_B, Handbills_A, Ladder_A, StrawLitter_A, 5 WeedFringe_A)
+ smithyTown_ (CrateStack_B, Handbills_A, Ladder_A, StrawLitter_A, 8 WeedFringe_A)
+ homeL_ (2 FallenBoards_A, 9 WeedFringe_B)
weeds: + cabinSouth 2, + mainStreet 2 rings, - cottageNorthEast 1, ~ mainStreet 1 ring
```
Main runs: copy town_next.json over town.json, `build_area.py RansomsRest dressing`, scatter, fauna, path_probe, tour (MainStreet*,
PenLane, YardRoad, Farm), Looter.Breakable / Economy.Balance (no breakable mesh added or moved), PIE look at the east end and Pen Lane.
