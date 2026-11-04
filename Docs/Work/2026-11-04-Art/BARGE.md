# BARGE lane (2026-11-04): the kit on the jumboed Tripo hull

Contract: `CONTRACT.md`, "Round 2 ... the Tripo hull REWORKED" (the user, 08:35, after seeing the scripted hull in PIE: "remove
the hull we just made, and use the Tripo model but rework the model to fit our needs and expectations"). REWORK reshapes
`/Game/Art/Tripo/SM_DredgeBarge` in Blender; LEVEL rebuilds the placements (`BARGE_LEVEL.md`); MEASURE measured the original
Tripo hull (`BARGE_MEASURE.md`). This lane: the barge's C++ and tests. Route: **DeckTop** = the hull's deck; `KitLift` stays 0.

## Status
- **Round 1** (05:30-06:20): the parametric code. **Round 2** (07:00): the kit on the scripted hull. **Round 3** (07:20):
  the review's fixes. **Round 4** (08:40): the kit on the reworked Tripo hull, the hall at deck level in the house (below).
  Rounds 4-5 were built and committed (a81c52ab).
- **Round 5** (10:30): stairs climbable, crane dropped, vents flush. **Round 6** (11:00): the jumboed hull's zones, the
  hall below deck with the hatch stair and the side room, props; stubs until BARGE_LAYOUT.md (below). In `Source/`, not
  built, no tests run.
- **Rounds 7-9** (12:00-12:40): the review's fixes, the PROPS lane's models, BARGE_LAYOUT.md plugged (below). Built by Main
  (13:20, clean): Cage, Dying, Fall, Grates, Valves pass; Kit, Deck, Hall, Hull failed (Round 10).
- **Round 10** (14:05): each failure's cause (below); the kit's and the tests' fixes in `Source/`, not built.
- **Round 11** (14:40): the PIE look check's three (below): the Tripo horn post, barrels for the sandbags, work lamps.
  Not built.
- **Round 12** (15:50): REWORK's round-4 hull: Barge 8/9 (`.Hull`'s deck heights); the zones re-plugged (below). Not built.
- **Next**: Main's build, then `Looter.World.Barge.Kit`, `.Deck`, `.Hall`; `.Hull` after REWORK's round 4 (its lid, rail, coaming); PIE.
- Round 6 supersedes everything below it on the house, the arena rectangle, the deck-level hall and office, and the spots;
  the berth math, the catwalk openings (round 5), the vents' clock and the dying/fall still hold. Rounds 2-5 are history.

## Round 12 (15:50): REWORK's round-4 hull; `.Hull`'s deck heights (not built)
- **Cause** (`.Hull` "Her decks at their zones' heights", every zone): the zones' heights against the hull's own tops to
  2 cm. Round 4's floor hulls top at 2253.8 (well deck), 2373.4 (forecastle), 2260.2 (cross walk), 2257.1 / 2257.8
  (walks), 2252.0 (stern) by `LineTraceComponent` (the editor's traces agree). Main had set ForecastleZ 2371 and
  CrossWalkZ 2253 from PIE (round 3: the level's drops found 2370-2371 over a 2376.8 top, 2252.6 over 2260.2), and the
  level's drops in the test level agree with PIE (`.Kit`'s spots and `.Hall`'s restart pass to 5 cm on them). So the
  game's drops find some decks a few cm under the hulls' tops, the hull's own traces the tops. Likely Chaos's collision
  margin (5% of a hull's thinnest side: 6 on the 119-thick forecastle boxes matches round 3); not proven. The stern's fore
  corners also stood on the walks' floors (the stern platform's hull ends at x -3360, the casing's aft wall -3411..-3361).
- **Fix** (kit and test):
  - Each zone's height sits between the hull's top and the game's drop, within 5 cm of both:
    - WellDeckZ 2250 and ForecastleZ 2371 (kept).
    - CrossWalkZ 2253 -> 2256 (the hatch stair's rise 376: run 481, foot y -256, outer corner r 724).
    - WalkZ 2257 (kept); SternZ 2252 -> 2250.
  - `.Hull`'s deck check holds to that 5 cm (as every floor check does) and names the hull's top at each failure. An
    info line per zone gives the hull's top and the level's drop: the next run's log settles the margin question.
- **Re-plugged** from the regenerated layout and a 20 cm grid of the forecastle (traced clear to 2 m over its floor):
  - The scoop frame is now 17 tube hulls over x 4320..4518; DeckFittings {4320, 4518, -395, 558}.
  - ForecastleStep + ForecastlePort -> **ForecastleAft** x 4130..4320, y -490..540. The scoop walls are at -508 / 566;
    the stairs top out in it.
  - **Forecastle** x 4530..5015, y -360..420, the walls narrowing fore; it was 4730..5015, ±350.
  - Spot F2 (4370, -440), now off every zone, -> (4250, 380): the starboard stair head's side, 1.9 m from its work lamp.
  - CrossWalkFore -300 -> -320 (the house's aft wall's hull from -315); SternFore -3300 -> -3420.
  - Walks kept (-790..-725, 745..815: inside the layout's every-station strips -792..-718, 744..815).
- Checked on the placed barge, hull alone: every zone corner and middle within 5 cm of its zone's height, nothing over any
  to 2 m, F1-F3 and the restart floored and clear. Ran `.Hull` and `.Hall` here before the fix (`.Hull`: the list above;
  `.Hall`: pass).

## Round 11 (14:40): the PIE look check (grabs `Saved/ArtPreviews/BargeRework3/`); not built
Main had set `ForecastleZ` 2371 and `CrossWalkZ` 2253 (the built hull's traces); kept.
- **Horns** (pale new lumber beside the rust): `ACompanyLoudspeaker::ModelPath` is now the user's Tripo post
  `/Game/Art/Tripo/SM_LoudspeakerPost` (IMPORT.md: 119 x 211 x 400, horns +X, pole hull, socket Horns (61, 17, 338)); it
  was `SM_CompanyLoudspeaker`, never made. With it: the post's foot on the ground from its bounds, the pole on the spot,
  the horns' target capsule and `GetHornsLocation` at the socket; the stand-in's horns and plate hidden (as before for a
  model). The fence-post stand-in stays the fallback. Every placed ACompanyLoudspeaker takes it (the barge's four, Mains
  2-3's gates), the same post WORLD's instanced Main Street posts already use. Shot out, the model's horns stay drawn
  (sparks and the cut-off only).
- **Sandbags** (huge, lumpy, ragged): the wall at (3000, 150) is a barrel trio now, turned 30 (its three at (3000, 114),
  (2969, 168), (3031, 168); footprint x 2926..3074, y 72..210: 71 cm off the vent at (3000, -100), 226 off W3). The sandbag
  kind and its second course stay in the code, unused.
- **Work lamps** (`BargeRules::WorkLamps`, nine; the mine lantern, instanced with its iron: two draws; each a point
  light 12 cd, reach 1200, no shadows, DeckLampColor; in `DeckLamps`, so out as she dies and back on reset):
  - Bulwarks, standing on the rim's top (|y| 792, z 2341) on a foot plate: (2400, ±792), (3550, ±792). Nothing over the
    rail but a lantern's height: under the Lamplighter's bag even listed at the jump (79 over the rail), 150 / 200 cm
    from the grapples' bites (x 2550, 3750).
  - Forecastle: posts 50 up out of the scoop walls' tops, a 40 cm arm inboard, the lantern hung from it: (4170, 590)
    on +Y (top 2564), (4170, -530) on -Y (top 2569); lanterns at y 550 / -490, 2 m over the floor; 94 cm off F1.
  - Walk mouths: rods 80 down from the house's overhang (underside 2701) at (1950, -757) and (1950, 780): lanterns
    3.3 m over the walks, 1 m fore of M1/M2.
  - Hatch: a rod from the house's aft overhang (x -358..-315, underside 2700) at (-336, 300): over the restart.
  - Posts and rods start 10 cm inside the iron they're fixed to (the layout gives those tops and undersides by their
    hulls): none floats off it. Not checked against the model by eye: PIE.
- Test (`.Deck`): the deck's lights lit, shadowless, on her, no lantern over a fight spot's head. Perf: nine more
  unshadowed point lights over the deck (`Toolsperf.ps1` on the barge's view when Main next measures).

## Round 10 (14:05): Main's build of rounds 8-9 tested; causes and fixes (not built)
Reran here `.Kit` and `.Hall` (same failures as Main's). Probes (read-only, scratchpad `r10_probe*.py`): traces on the placed
`Barge_Pilot`, the hull alone (`LineTraceComponent`) and the level; probes 8-9 spawned throwaway barges in the editor world,
the map reopened clean after (`open_clean.py`, nothing saved).
- `.Deck` "vents ... over its round": **the test** (round 9 put the vents on the well deck on purpose, the deck fight's
  hazards; the hall is aft under the casing). Now: each vent on the well deck.
- `.Deck` "Cover5_0/Cover5_1 off its deck": **kit**. The forecastle's barrel pair at x 4760: the second barrel turned 37
  degrees has a box half of 42, to x 4718, aft of the zone's 4730. Pair to x 4790; spot F3 4870 -> 4885 (its shoulders clear
  of the pair: pair to 4832 + 30 < 4885).
- `.Hall` "through the hole ... inside the hall's wall": **kit (rules)**. The hatch stair's outer foot corner at r 733, past
  R - SkinGap 730. Stair 110 -> 100 wide, its line x -555 -> -558: corners r 619 / 713 (top), 633 / 726 (foot); in the hole
  with 3 cm aft, 72 cm fore. Arc -22.6..19.7 (the same two spans open).
- `.Hall` "floor stairs ... the catwalk open over them": **the test**. FloorStairSpan 70 (round 9) is 3.5 spans: the test
  rounded to 4 and wanted the ring's end at the foot. Now it counts the spans over stairs by their middles (as the rule
  opens them) and lets the ring run on past a foot where the stair under it is a man's headroom (240) below the catwalk's
  underside: at the ring's ends (139.4 and 220.6 degrees) the stair is 88 cm up, 560 is the underside.
- `.Hall` "down the hatch's stair" and `.Hull` "hole open", "coaming ... open on the stair's side", "nothing ... over the
  hatch's stair": **the hull**. Its collision covers the whole hatch hole with a solid lid, 2260.2 to 2310 (the coaming's
  top): every drop through the hole stops at 2310, the coaming's across traces start inside it, the stair's top steps hit
  its underside. REWORK: the coaming as three thin hulls round the hole, 50 high to 2310 (aft x -631..-611 and fore -436..-416
  along y -240..245, -Y y -240..-220 across x -631..-416), none on +Y (the stair's top), and no hull inside x -611..-436 x y -220..225 from
  the coaming's top down to the catwalk (1880). The deck's underside round the hole at 2220 is fine (the kit's ceiling is
  2200..2220).
- `.Hull` "rail closed past it": **the hull** (known: REWORK round 4). No -Y bulwark hull at x 2985 or 3315 (z 2295, y -645
  to -847); the gap must stay open over x 3025..3275.
- `.Kit` "nine deck fight spots, each on its deck" and `.Hall` "put back by the hatch: on open deck": **not reproduced in
  the editor**. Every probe finds the hull at each spot's height (2250, 2376.8, 2257.1 / 2257.8) and at the start
  (-555, 375: 2260.2), with object, Visibility and Camera traces. That holds on the placed barge, on each of its 162 parts
  traced alone (only the hull answers), and on fresh barges with the pieces BeginPlay spawns, their collision toggled as
  Away -> Hanging. The tests' level never ticks; in it the hull's own traces (`.Hull`'s zones pass) find her decks where the
  level's drops do not. So:
  - `SpawnBarge` now flushes the level's physics scene (`FPhysScene::Flush`: commands and the query structure brought up
    to date, as each frame does in play).
  - Both messages now name what a failing drop hit (part, actor, height), and the hatch-stair climb names its first stop
    (`FloorWhat`, `HeadroomWhat` in BargeTestWorld.h).
- Lesson: in this Git Bash, `sed -i` rewrites a CRLF file with LF endings; edit with node and keep `crlf.js` after.

## Round 9 (12:40): BARGE_LAYOUT.md plugged (the built hull); not built
- **Plugged** (BargeRules.h / BargeHallRules.h):
  - Hull x -3750..5590, top 4809.
  - Zones (clear boxes, z = the floor hulls' tops):

    | Zone | x | y | z |
    |---|---|---|---|
    | WellDeck | 2245..4031 | ±665 | 2250 |
    | ForecastleStep | 4130..4155 | ±600 | 2377 |
    | ForecastlePort | 4170..4415 | -520..-362 | 2377 |
    | Forecastle (fore of the frame) | 4730..5015 | ±350 | 2377 |
    | WalkPort | -294..2000 | -790..-725 | 2257 |
    | WalkStarboard | -294..2000 | 745..815 | 2257 |
    | CrossWalk (new kind) | -665..-300 | ±935 | 2260 |
    | Stern | -3650..-3300 | ±500 | 2252 |

    The walk boxes are the strips clear at every station; the walks are 2.4 m at their median.
  - `DeckFittings`: the scoop's frame, x 4155..4724, y -358..593.
- **Rail walls.** `RailLine` holds 42 runs taken from the UCX bulwark hulls (inner faces). The -Y well-deck run mirrors the
  +Y one at -757, because the hull has no collision there. It is cut at the gap. One more wall runs across the stern's aft edge
  (±700). `FDeckZone::Rails` is gone.
- **Forecastle stairs** are the model's: `bKitForecastleStairs` false, |y| 150..450, from (4130, z 2377) down to (3950, z 2250),
  35°.
- **Berth.** Values: `BerthX` 3150, `GapHalf` 145, rail rim 90, `BulwarkThick` 70, side 827, `SillUp` 12, `BerthOut` 1520,
  `JumpOut` 1630. Clearances: plank over the sill 16.4, bag 166, jump 79.
- **Hall.**
  - Centre (-1185, 0), R 750, floor 1300, ceiling 2220 (kit ceiling 20 thick, so walls are 900). `HallSkinHalfBeam` 827,
    `HullWall` 20 (the gap to the skin's inside). Alcoves 37 deep, `AlcoveUp` 380 (no longer tied to the catwalk).
  - Ceiling: the square closes the hollow (no deck inside the casing), cut round the hole.
- **Hatch.**
  - Hole x -611..-436, y -220..225, on the cross walk; coaming 50 high.
  - The hole sits over the hall's fore rim (the hall's wall is at -435) with hull 6 solid fore of it. No side room fits, so
    the office went.
  - New `HatchStair`: 38°, 110 wide, at x -555, from (−555, 225, 2260) down -Y to (−555, −261, 1880). It stays over the catwalk
    inside the wall all the way: corners at r 617..733.
  - `HatchHoleLeast` 384 (the hole is 445). `CatwalkUp` 400 -> 580 so that stair fits; the catwalk has 3.2 m headroom.
- **Catwalk and floor stairs.** The catwalk is open under the hatch stair (spans 350, 10). An end wall stands at 19.4° by the
  stair's top. Floor stairs span 60 -> 70 (37.9°), feet at 150/210, their inner corners at r 586 (grates 340..570).
- **Stoker's corner** (on the catwalk, between the hatch stair's foot and the port floor stair):
  - Desk at 316°, its long side along the ring; the radio on it, dials in (`RadioYaw`).
  - Safe at 40° (across the hatch: the player steps off the stair toward the wall).
  - The office lantern on the wall above the desk.
  - Coal sacks against the wall beside the stokehold doors.
- **Flow:** the well-deck fight -> a house walk -> the cross walk -> the hatch stair onto the catwalk (the desk with the manifest
  is there) -> along the ring to the port floor stair (280° -> 210°) -> the floor, the furnace and the valves. Step 7: back up
  the same way, then out along the port side to the gap.
- **Fight plan** (the compact 17.9 x 13.3 m well deck; landing (3150, -515), 2 m apron):
  - Fight spots:
    - well deck: (3700, 300), (3750, -350), (2700, 180), (2350, -200, behind the gantry);
    - forecastle: (4250, -440), (4370, -440) in the port strip, and (4870, 150) fore of the frame;
    - house walk mouths: (1850, -757) and (1850, 780).
  - Cover: Crate (3616, 176), BarrelTrio (3605, -310), HoseCart (2760, 60), Sandbags (3000, 150), Barrel (3400, -250, the first
    off the plank), BarrelPair (4760, 150, forecastle), Winch (4950, -200, forecastle, yaw 90).
  - The gantry sits on the well deck at (2500, -430); its jam lever is on its port side.
  - Stack foot (2440, 420). It falls along the starboard side (`StackFallAft` -90) from its flange's edge (2615) to x 3915,
    clear of the landing, the port way and the stairs.
  - Vents (3000, -100), (3350, 350), (2550, -130), (3800, -50). No vent is over the hall now, so it gets no hoods.
  - Horns (2700, ±640) and (3700/2800, ±640).
  - Fires: stern (-3500, ±400); casing roof z 3200 at (-3000, -400), (-2900, 450), (-1950, -470), (-1200, 450).
  - Lamp at (2300, 0, 3200). Smoke at (-2000, 0, 3350).
  - Offline sight check (cover and blocker bounds): the plank sees 5 of 9 (W1, W2, W4, F1, F2); each stair top sees 4 (the well
    deck four). `PlankSightLines` is 4, `StairSightLines` 4.
- **What doesn't work for the kit (hull fixes):**
  1. The -Y well-deck bulwark has no collision over x 2590..3854: UCX -Y 22-25 are missing, and 21/26 are 17 cm slivers. Shots
     and props pass the port rail; only the kit's pawn walls hold. `.Hull`'s "rail closed past the gap" fails until it's fixed.
  2. Both house walks are blocked at x 1450..1600: headroom 38-119, and 40 at x 1500 on both sides. That is the only route to
     the hatch; REWORK's round 4 has to clear it. `.Hull`'s "walks clear end to end" catches it.
  3. The scoop frame's collision (from 27 cm over the floor) covers the forecastle's aft half. The +Y stair tops out under it;
     only the -Y stair's outer part reaches clear floor (the port strip, 1.6 m). The frame also blocks the fore part's lines.
     Lift or trim the frame.
  4. The coaming must be open (or at most a 45 cm step) on the hole's +Y side, where the stair starts. A closed 50 cm rim
     stops the player; `.Hull` and `.Hall` both catch that.
  5. The skin at 827 leaves the stokehold alcoves 37 cm deep (doors onto a niche).
- **Tests rewritten** for all this: `.Deck` (zones, rail walls never inside a zone), `.Hall` (the hatch stair inside the hall,
  the corner on the ring, the walk from the stair's foot past the desk to the floor stair, three ring ends walled), `.Hull`
  (hollow on 4 diagonals at 3 heights, the office checks gone).

## Round 8 (12:30): the PROPS lane's kit models adopted (BARGE_PROPS.md); not built
- **Paths.** 16 `BargeLooks` paths under `/Game/Art/Landfall/SM_Barge*` replace the old never-imported Tripo* names. Each
  falls back to the engine shape.
- **Slots.** Set by name (`SetSlot`, `GlowSlot` CinderGlow, `HoseSlot`). Cold vents, grates and hoods have the glow slot set to
  IronBlack; hot ones are left as the model has them.
- **Model sizes** live in `BargeLooks` (furnace 250/600, stack 150/1400, gantry 200x130, grate 180 deep at r 430 for 12,
  pipe 50x300, tongue 50x100, vent half 100 with its fire 8 up, hood diagonal 106, drum 50 on the gantry).
- **Furnace.**
  - The boiler body scales by those sizes; the burst model swaps in on `SetCageBurst`.
  - Its glow slot goes dark when `PutOut` runs or all three valves are shut.
  - The heart is at the Heart socket: `HeartIn` -100 gives x 150. The stand-in drum keeps `HeartOut` +30.
  - The light is at heart + 60 (210). The interaction point is heart + 80 (230), and the box at heart + 40 reaches x 300.
  - New `GetCageMouth` (the Window socket + 60 = 296, clear of the plinth): CageBurst now centres there.
  - Pipes are kit sections from the Pipe_N sockets (640) to the ceiling.
  - Shards are the model laid flat, 0.6-1.3 scale, fanned ±70° inside the hall. The old placement reached 10 m, past the new
    wall.
  - The grate wedge scales by the model's sizes. The cage code moved to `BargeFurnaceCage.cpp`.
- **Valve.** The model at the origin. The wheel model spins about its own X only. The riser is pipe sections from (−10, 0, 300)
  to the ceiling, 45 cm, blocking.
- **Hose drum.** The gantry is scaled flat to `DrumGantryHalf`. The drum model sits at 50 with its hose slot set to her
  hose's rubber. The lever model turns about its pin (140, -172, 96); `LeverAt` (140, -172, 150) is the grab point.
- **Deck vents and fire.** Vents stand on the deck (8 cm proud); the frame rim and slats are now stand-in only. Fire tongues
  (`FireAt`) replace the cones: vent columns, grate fire, stern fires (2.2 x 3.2).
- **Hall hoods.** The hood model goes under any vent whose hood fits inside the hall; it glows with its vent.
- **Stack.**
  - The model on its foot, scaled. It topples about its flange's edge (`StackFootRadius` 175; the pivot is the edge, the mesh
    sits under it). `StackFootHalf` 190 is in the keep-apart test.
  - The doubler plate (`StackFoot`) stays behind when it falls.
  - Its height is now 1300, so it lies on the deck.
- **Radio.** The model stands on its feet on the desk, dials in. `RadioSize` is now (30, 45, 30).

## Round 7: the review's fixes to round 6 (not built)
- Tests: the landing's end may lie on the office's edge (`IsInsideOrOn`); the ring's end may sit the spans' stretch (1.2 deg)
  short of a stair's foot; the hall's height reads `HallLeastHeight` 900 (the tautology is gone); the doorway's clearance is
  `DoorwayClearDegrees` (2 panels); new: every vent over the hall's round.
- Alcove width = the door's half chord `HallDoorHalf()` (R sin 10 deg = 121.6; was 190), so the sacks (`CoalSackIn` 60) show.
- No flicker: the doorway's lower wall run and the catwalk's grating look stop `LookDrop` 1 under 1700; the stand-in box's
  top is `WellDeckZ - PlateThickness`.
- The office's hall-side wall stands on the tangent into the room (x 3400..3430), never in the hall's air; the desk and the
  lantern start from its room face.
- ForceStep's reset turns the deck lamp and the lanterns back on (BeginDying puts them out).
- Vent 6 (3650, -650) -> (3200, -450), over the hall; barrel trio (3415, -415) -> (3390, -440): 64 cm to fight spot
  (3550, -350) past a shoulder at the real barrel size.
- Named: `HallWallPanels` 36, `HallWallPanelStretch` 1.04, `DoorGapDegrees`, `HallFloorLookThick` 10, `CatwalkThick` 20,
  `CatwalkRailIn/Up/Width/Depth` 6/100/6/8, `AlcoveFloorThick` 30, `AlcoveDoorThick` 12, `AlcoveDoorGap` 20,
  `OfficeFloorThick` 20.

## Round 6 (11:00-): the jumboed Tripo hull, the hall below deck (CONTRACT.md "Round 3")
In `Source/`, not built, no tests run. Every "the hull's" number is a STUB until `BARGE_LAYOUT.md` lands (plug list below).
- **Zones** (`BargeRules::DeckZones`, `FDeckZone`: name, `EDeckZone` WellDeck/Forecastle/Walk/Stern, box, Z, `Rails` bits
  `RailPort/RailStarboard/RailAft/RailFore`). `DeckZoneAt`, `OnDeck` (a point at its zone's Z), `IsOpenDeck` (on a zone,
  off `DeckFittings()` (stub none), the hatch's coaming, the forecastle's stairs). `RailWallLayout()`: a pawn-only wall
  `RailWallThick` 30 just outside each railed side, at its zone's Z, the well deck's port side cut at `BerthX +- GapHalf`;
  names `RailWall_<Zone>_<Side>` (`PortAft`/`PortFore` at the gap). Stand-in only: a plate per zone, `CutAround` the hole.
  - Stubs: WellDeck x 1950..4150 |y| 900 z 2250 (rails port, stbd); Forecastle x 4150..5250 |y| 700 z 2370 (port, stbd,
    fore); WalkPort/WalkStarboard x -3250..1950 |y| 790..940 z 2250 (outboard); Stern x -3550..-3250 |y| 900 z 2250
    (port, stbd, aft). `WalkLeastWidth` 150, `WalkMouth` 300. Hull bounds x -3750..5350, half beam 1237.5, top 4850.
  - Forecastle stairs `ForecastleStair(0/1)`: at its aft edge, y -+550, run 200 (31 deg), `bKitForecastleStairs` true
    (the kit lays ramp + treads; false when the model has them).
- **Gone:** the house interior (vestibule, its lining, door, the deck-level office, `VestibuleArea`, `DeckPlates`,
  `InteriorFloorLift`, `House*`, `Door*`, `DeckAft/DeckFore/DeckHalfBeam/DeckTop`, `HullLength`), `DeckBlocks`/`MakeBlock`/
  `GetDeckBlocks`, `CoverSpots`/`CoverHalf`/`CoverLook`, `DesignHallRadius`, `HallUnderDeck`, `ValveInset`, `WallPanels`.
  Nothing of the kit stands in or on the house.
- **Hall below deck** (`World/BargeHallRules.h/.cpp`, included by BargeRules.h): centre x 2700 (stub), R 700, floor 1300,
  ceiling 2230 (the deck's underside; kit ceiling 2200..2230), walls 900, `HallSkinHalfBeam` 850, `HallLeastRadius` 650.
  - Hall angles run from `OfficeAngle` (0: the office fore of the hall, 180: aft; derived from the hole's side), so the
    furnace (yaw `OfficeAngle`, cage facing the doorway), valves, stairs, stokeholds and lanterns turn with it.
  - Catwalk 4 m up, `CatwalkDepth` 200 (500..700), 18 spans; floor stairs tops 80/280, ways +1/-1, span 60 deg (chord 600,
    33.7 deg), feet 140/220 between the stokeholds (90/270) and the back valve (180); 6 spans open; end walls 139.4/220.6.
    Feet's inner corners at r 532 > `GrateOuter` 520.
  - Furnace R 300 -> 250 (5 m); grates `GrateInner` = R+90 = 340, `GrateOuter` = HallRadius-180 = 520; `HeartAt` follows.
  - Office's doorway at 0 deg, catwalk level (hall wall below, lintel from `CatwalkUp + OfficeDoorUp` 300); stokehold doors
    as before; alcoves 110 deep (830 = skin - 20).
- **Hatch and office:** hole stub x 3550..3770, y 20..520 (500 long), `HatchWay` (0,-1); coaming 20 thick, 60 up (the
  hull's; kit coaming over the stand-in only, open on the stair's side). `HatchStair()`: top at the hole's +Y edge flush with
  the deck (3660, 520, 2250), 32 deg, down to the office floor 1700: run 880, foot y -360, landing 150. `HatchHoleLeast()` 464
  (240 head over the feet clears the ceiling's 2200 past the hole). `HatchSpot()` = the stair's top (`GetHatchSpot`, the
  Reach marker +50); player start after the manifest 150 behind it facing down.
  - `OfficeArea()` derived: from the hall's tangent (x 3400) round the hole, the stair (+-110) and the landing, and the
    doorway: x 3400..3770, y -510..520 at z 1700..2200; walls KitIron (side, far, the hall wall with a 240 x 300 doorway, lintel),
    floor KitIronDark (40 cm into the doorway), ceiling cut round the hole.
  - Desk: `SM_GunsmithBench` at its bounds, long side along the hall wall, at the room's end away from the landing (stub
    centre ~(3455, 420)); `GetOfficeSpot` = the desk's bounds' top middle (LEVEL's manifest +3 cm). Radio cube on it
    (`RadioSpot`). Safe `SM_Strongbox` under the stair's high end (`SafeSpot` (3660, 400)).
  - Lanterns: `SM_RR_Mine_Lantern` x4 on the hall wall at 45/135/225/315, 250 up; `SM_PorchLantern` beside the doorway
    (y -200, 260 up); each a shadowless point light 8 cd, 900 reach, warm; out as she dies.
  - Coal: `SM_CoalSack` 2 per alcove, `SM_SoulCoal` x3 by the furnace (130/205/235); looks only.
- **Spot plan** (stubs; the tests hold the rules): plank at `BerthX` 2700 (landing (2700, -750), 2 m apron clear).
  - Fight 9: well deck (3300, 650) (3550, -350) (2400, 300) (2170, -320), each behind cover toward the landing; forecastle
    (4450, -300) (4450, 300) (4950, 450) (high ground, behind the winch / sandbags / barrels); walk mouths (1850, -+865).
  - Cover 9 (`CoverPieces`, kind/x/y/z/yaw): Crate (3240, 510, 15), BarrelTrio (3415, -415), HoseCart (2440, 155),
    Sandbags (2315, -445, 50), Winch (4300, -300, 90, fc), Sandbags (4300, 300, 90, fc), BarrelPair (4800, 450, fc),
    Barrel (2950, -450: the player's first cover), Crate (3000, 150, -10). Offline check (fallback sizes): plank sees 8/9,
    each forecastle stair top 9/9 (rules: `PlankSightLines` 5, `StairSightLines` 4).
  - Vents 6 over the hall: (2150, 0) (2700, -100) (2750, 600) (3150, -150) (3300, 250) (3650, -650); KitIronDark plate +
    rim 12 wide 6 up + 6 slats 7 wide 3 up (looks only).
  - Horns (2350, 830) (3400, -830) (3600, 830) (4700, -620, fc). Gantry on the forecastle (4900, 0, 2370). Falling stack
    (2150, 600), `StackFallAft` 0: falls straight to port across the well deck's aft end (clear of the hatch and the landing).
    Lamp (2010, 0, 3200) on the house's fore face. Fires: stern (-3400, -+450), walks (-2500, -865) (-1200, 865), casing roof
    z 2830 (-2900, -400) (-1600, 450); `CasingRoof` x -3200..-750 |y| 700.
  - Berth: `BulwarkHalfBeam` 950, `BerthOut` 1650 (bag 173 clear), `JumpOut` 1760 (82 clear at 12 deg), plank over the sill 10.2.
- **Looks:** every kit room surface, stair and vent on `BargeLooks::KitIron` / `KitIronDark` (`FindOr` falls back to Iron /
  IronBlack). Stairs: unseen ramp (`MakeRamp`) + treads every 20 cm and two stringers (`StairLook`). Props laid by
  `MakeProp`: bounds' bottom middle on the spot, the mesh's simple collision, else an unseen block of its bounds; without
  the mesh the kit block (`CoverFallbackHalves`). Sandbags stacked two high when under 90 cm. No stack mesh exists
  (`SM_BargeStack` not imported): the falling stack is a KitIronDark cylinder.
- **Files:** new `World/BargeHallRules.h/.cpp`, `World/BargeBoardingProps.cpp`, `Tests/BargeBoardingHallTests.cpp`;
  BargeTestWorld.h gained the shared traces. Kit.cpp, Hall.cpp, Spots.cpp, Rules, tests rewritten; Furnace (`HeartAt`),
  Dying (lanterns out, wording), BargeBoarding.cpp (stand-in box over the bounds, furnace/valve yaws).

### Plug list (BARGE_LAYOUT.md -> code)
| Layout item | Constant(s) |
|---|---|
| Hull bounds | `HullAft`, `HullFore`, `HullHalfBeam`, `HullTop` |
| Well deck box, z | `WellDeckAft/Fore/HalfBeam/Z` |
| Forecastle box, z; its stairs (model's or not) | `WellDeckFore`, `ForecastleFore/HalfBeam/Z`; `ForecastleStairAcross/Run`, `bKitForecastleStairs` |
| Walks (narrowest clear strip), z | `WalkAft`, `WalkInner`, `WalkOuter`, `WalkZ` |
| Stern deck box, z | `SternAft`, `SternHalfBeam`, `SternZ` (zone x ends at `WalkAft`) |
| Rail sides facing a drop | each `FDeckZone::Rails` |
| Rail at the berth: top, thickness, outer face, sill; berth gap middle and half width (Gangway socket) | `RailHeight`, `BulwarkThick`, `BulwarkHalfBeam`, `SillUp`, `BerthX`, `GapHalf`; then recheck `BerthOut`/`JumpOut` |
| Hatch hole box, stair way; coaming thick/high, open side | `HatchHoleX/YMin/Max`, `HatchWay`, `HatchCoamingThick/Up` |
| Hall cavity: centre x, R, floor, deck underside; narrowest half beam over it | `HallCenterX`, `HallRadius`, `HallFloor`, `HallCeiling`, `HallSkinHalfBeam` |
| Fittings to avoid | `DeckFittings()` (make it a static array) |
| Casing roof box, z | `CasingRoofAft/Fore/HalfBeam/Z` |
| Drum socket | `DrumUnderKeel` |
| Then re-plan | `DeckFightSpots`, `CoverPieces`, `VentSpots`, `HornSpots`, `DrumOnDeck`, `FallingStackFoot`, `DeckLampSpots`, `SternFireSpots`, `FallSmokeSpot` (the Deck test names every clash) |
- Requirements on the model: the hole at least `HatchHoleLeast()` long along its stair; the coaming open on the stair's top
  side; no hull collision in the hall's cylinder, the office's box (`OfficeArea()`, z 1700..2200) or over the stair; the
  deck slab's underside at `HallCeiling` over the hall; the office side room fits the beam at z 1700-2200.
- **Tests:** `.Kit` (berth on the well deck, clearances, rail walls by layout and Z, landing, hall floor, 9 + 8 spots on
  their floors, grapples, horns, desk); `.Deck` (zones in the hull, walks 1.5 m and joined, forecastle stairs walkable,
  every spot on open deck at its zone's Z, the plan's 3/2/rest, fires, the deck's pieces apart, the stack's fall clear, plates
  per zone over the stand-in only, every cover part on its deck and apart by its real bounds, sight lines from the plank
  and the stair tops, nothing past the beam); `.Hall` (below deck, 9 m, inside the skin, the hatch on the well/stern deck,
  the stair flush/walkable/hole long enough, the office round hole/stair/landing, desk/radio/safe, nothing in the doorway,
  floor stairs' feet off the grates, the catwalk openings; ceilings traced; a capsule every 50 cm down the hatch stair and
  each floor stair, the office walk to the catwalk, the ring ends walled, the restart by the hatch, the real desk and the
  manifest's spot, 5 shadowless lanterns, alcoves inside); `.Hull` (no plates; each zone's corners and middle at its Z; the
  gap; the hole open to the office, the coaming on 3 sides; nothing in the hall, office or over the stair; the deck's
  underside at `HallCeiling`; nothing over the hall floor; the walks clear). `.Grates` reads the rules' ring.

## Round 5 (10:30): fixes from DEMO's PIE walk of round 4 (grabs in `Saved/ArtPreviews/BargeRework/`)
- **Stairs.** Round 4's two floor stairs (radius 944-1010) ran wholly under the catwalk ring (deck 2630-2650), so the capsule
  stopped halfway up.
  - Now the ring is open over each stair: `IsCatwalkSpanLaid` drops the spans whose middles lie over a stair's arc (4 of
    the 18; the looks follow).
  - The stairs are re-laid on span edges as mirror images: `FloorStairAngles` {220, 140}, new `FloorStairWays` {+1, -1},
    `FloorStairSpan` 35, down to 255° and 105°.
  - Each rises through the opening and lands on the span past its top (210°, 150°). Their feet now sit on the floor's top,
    2252 (`InteriorFloorLift`).
  - An unseen pawn wall (`CatwalkEnd0/1`, 30 thick, `RailWallHeight` tall) closes the ring's end past each foot, at
    `CatwalkEndAngle` 259.4° and 100.6°.
  - `CatwalkSpans` 18 and `CatwalkSpanStretch` 1.06 moved to the rules; new `CatwalkSpanAngle`.
- **Crane: dropped.** The reworked hull has none, and an engine-shape stand-in is the look the audit named. Gone:
  `CraneSpot` and its constants, the stand-in crane, the crane-head lamp, `BargeLooks::TripoCrane`. `DeckLampSpots` keeps only
  the door lamp.
- **Vents: sunk flush.** `VentGrilleThick` 10, `VentGrilleUp` -4: 1 cm proud instead of 9. No collision, nothing to trip on;
  the glow and fire use the same transform.
- **Tests.**
  - `.Hall`: 4 spans open; the landing spans laid; the ring ends past the feet. A capsule (r 30, 30 cm over the feet, 210
    high) every 50 cm from each foot to each top, with the stair under the feet; round 4 fails this. The landing at 2650 past
    each top, and a pawn trace off each ring end blocked.
  - `.Deck`: the crane is out of the spots and the keep-apart test.

## Round 4 (08:40): the hall at deck level in the reworked house
- **Why.** The reworked house is raised to a 3300 roof and widened to the full beam. The hall fits inside it at the deck's
  level, so you walk in through the door on one level (door -> vestibule -> office -> hall). The stair, the hole in the floor
  and the hatch cover have nothing left to do, and the hull's body below the deck stays Tripo's (it may be solid).
- **Hull.** `HullPath` = `/Game/Art/Tripo/SM_DredgeBarge` again; the engine-box stand-in is still the fallback.
- **House.** `HouseHalfBeam` 1150 -> 1237.5 (full beam, 20 cm walls: inside ±1217.5). `HouseRoof` 2900 -> 3300. The door
  stays at (-400, 0, 2250) = `GetHatchSpot`. The two `RailWallHouse*` walls are gone (no side walks any more), leaving 4 rail
  walls: far side, berth fore and aft of the gap, bow.
- **Hall.**
  - `HallUnderDeck` 1000 -> 0, so `HallFloor` = 2250. `HallCenter` (-1900, 0) -> (-2050, 0). R 1100, walls 2250..3150.
  - `CeilingThickness` 30 (its own constant now): the ceiling sits at 3150..3180, under the roof, over the hall only.
  - `HallSkinHalfBeam` = `HouseHalfBeam - HouseWallThick` = 1217.5. `HallFits(1100)`: 1215 <= 1217.5. `AlcoveDepth` 97.5 ->
    77.5 (the alcove backs end at 1197.5).
  - Catwalk kept at 4 m, with the two stairs (225->260°, 315->350° here; re-laid in round 5) now climbing up from the hall floor. My call: it keeps
    the fight's height, with Dredgers on the catwalk shooting down and the player able to climb.
- **Floors.** `InteriorFloorLift` 2: the kit's floors inside the house (vestibule plate, office floor, hall floor) stand 2 cm
  over the deck, so they never fight a Tripo surface left at 2250.
- **Vestibule.** `VestibuleAft` -800 -> -580, `VestibuleHalfWidth` 1000 -> 300; the room is x -580..-440, |y| <= 300,
  height 360.
  - Kit lining round the door at x -440..-420, roof at 2610.
  - Its aft wall (x -600..-580) is also the office's fore wall, spanning |y| <= 520, with a 240 x 300 doorway at y 0.
- **Office.** x -950..-600 (`OfficeDepth` 400 -> 350), |y| <= 500 (`OfficeHalfWidth` 1000 -> 500), height 360 with its own
  roof (`OfficeHeight`, `OfficeWall` 20). `OfficeWallUp` is gone.
  - Its aft wall (x -980..-950) has the doorway |y| < 190, 300 tall (`OfficeDoorUp`), onto the hall floor; the hall's wall
    panels over it are a lintel from 300.
  - Wood floor.
  - Desk at (-830, -300), top 2334 (`DeskAcross` -600 -> -300). It is the manifest's spot. The radio is on it at
    (-800, -350, 2349).
  - Safe at (-655, 400) (`SafeAcross` -900 -> 400, new `SafeOut` 120).
- **Gone:** `HatchStair`, `HatchHole`, `CutAround`, `HatchStairX`/`TopY`/`Slope`, `HatchApproach`, `HatchHeadroom`,
  `bHatchCover`, `HatchCoverLift`, the `HatchCover` component and `BuildHatchCover`, `BargeLooks::Plane`/`HoleDark`, the hatch
  walls and coaming. The kit's plates are no longer cut.
- **Other changes.**
  - `HallCeilingArea` is the hall only.
  - `IsAboard`: z up to `HouseRoof + AboardOverRoof` (300; new).
  - One roof fire moved off the tower's footprint: (-1600, -850) -> (-600, -800).
  - `DeckLampSpots[0]` follows the roof: (-340, 0, 3180).
- **Tests.**
  - `.Hall` now checks:
    - the floor at the deck's level; the ceiling under the roof; the hall inside the house;
    - the office from the hall's rim to the vestibule, and the vestibule to the lining;
    - the desk in the office and off the walkway, the radio on it;
    - the stairs up to the catwalk;
    - the vestibule and the office roofed;
    - a pawn walk from the deck through the door, vestibule and office into the hall, floored all the way at 2252;
    - the alcoves inside 1217.5.
  - `.Deck`: plate tops at 2250 (open deck) and 2252 (interior); the cover check is gone.
  - `.Hull`:
    - deck corners, gap, door;
    - the house empty at 2350 and 3100: fore-aft at y 0 (through both stacks' x), across at x -1850 and x -2050;
    - the under-deck check is dropped (Tripo's hull body).
  - `.Kit`: 4 rail walls by layout.
- **For REWORK:** the stacks and the tower must stop at the roof (3300). If either reaches into the house, it shows inside
  the hall (Stack_2 at (-2373, 19) is over the furnace) and `.Hull` fails at 3100.
- **PIE check (after the import, the build and LEVEL's rebuild):**
  1. `Tools\runtests.ps1 -Filter Looter.World.Barge`, then `-Filter Looter.World.PilotSortie`.
  2. `Tools\pie.ps1`, then `Tools\console.ps1 "Looter.Barge.Show 3 aboard"`. Grab from the plank: the flat deck, the bulwark
     gap, the house aft with its door.
  3. `Looter.Barge.Show 4 aboard`. The player stands at (-250, 0) facing the door. Walk in (W only) through the vestibule
     into the office and grab the desk and the doorway onto the hall.
  4. Walk on and grab the hall from its floor by the office's doorway: ceiling, no shell or sky, the alcoves, the catwalk
     and its stairs, the furnace with pipes to the ceiling.
  5. `stat unit` on the deck and in the hall.

## Shared numbers with the hull (BargeRules.h, "the hull's"; change both together)
- **Hull and deck.** Hull 7500 long, half-beam 1237.5, pivot bottom-centre (the hose drum's socket (-889, 323, 0)).
  `DeckTop` 2250 (open deck x -400..3350).
- **Bulwark.** 110 high (`RailHeight`), 15 thick (`BulwarkThick`), outer face at 1237.5 (`BulwarkHalfBeam`). Gap 300 wide
  (`GapHalf` 150, was 220) at `BerthX` 2000 on -Y. `SillUp` 15 is a margin: the model's sill top is +2 cm. The model's
  bulwark collision is flat at 2360 over x -400..3200, 15 thick (inner face 1222.5), rising into the bow from 3200.
- **House.** `HouseAft` -3300, `HouseFore` -400, `HouseHalfBeam` 1150, `HouseRoof` 2900. Door 240 x 300 at y 0 in the fore
  face (`DoorHalfWidth` 120, `DoorHeight` 300) = `HatchSpot()` (-400, 0, 2250) = `GetHatchSpot` = the model's SOCKET_Door.
  `HouseWallThick` 20 = the model's walls (fore wall collision x -420..-400; the kit's lining stands at x -440..-420). The
  model's 72 cm side walks beside the house (collision (-1850, ±1186)) are closed by the kit's two `RailWallHouse*` walls.
- **Stacks and crane.** The hull's stacks ((-1107, 21) and (-2373, 19), on the roof, to 4600: the model's SOCKET_Stack_1/2) and
  crane (`CraneSpot` (900, 700), post 1200, jib 900 toward +X): the kit builds nothing there over the hull.
- **What the kit needs from the model's collision:** the deck slab top at exactly 2250; the bulwark round its gap; the
  house's walls split round the door; the roof; nothing inside the house or below the deck. If the hull body is one solid
  UCX, the hall and the vestibule are inside collision.

## The kit now (what moved where and why)
- **Hull component.** `HullPath` = `/Game/Art/Vehicles/SM_DredgeBarge`. With the model the Hull is BlockAll WorldStatic: it is
  walked and shot on as it is. Without it, the engine-box stand-in (7500 x 2475 x 2250) has no collision, and the kit lays its
  own open-deck plate (to the bulwark's inner face), kit rails and stand-in crane.
- **The arena.** `DeckAft` -300 / `DeckFore` 3300 / `DeckHalfBeam` 1100: 36 x 22 m, the house its aft wall, the berth's gap
  on its port side. Over the hull, nothing kit is drawn or blocked over the open deck except:
  - the unseen pawn-only rail walls, standing on the bulwark's line (y 1222.5..1237.5, 320 over the deck) so nobody climbs the
    110 cm bulwark and falls, open at the gap;
  - a bow wall at x 3300;
  - two walls closing the strips beside the house (|y| 1150..1237.5 at x -430..-400);
  - the hatch's three walls (9 rail walls in all, was 8; the stern wall is gone, since the house closes the aft end);
  - cover boxes, vents, horns, the falling stack, the drum's gantry.
- **Removed.** The kit's fixed stacks and their walls, the crane over the hull, the old deck plates.
  - Kept as switches: `DeckBlocks()` (none), `bHatchCover` (false), `bDrawRails` (false: the kit's rails show only over the
    stand-in).
  - `DeckPlates` keeps two plates: the open deck (`bOnHullDeck`, laid only over the stand-in) and the vestibule floor (always).
- **The vestibule** (new, `BuildVestibule`): x -800..-440 inside, |y| <= 1000, 360 high.
  - Its walls are kit iron (BlockAll, no shadow): aft at x -820..-800, sides at |y| 1000..1020, roof at 2610..2630, and the
    lining at x -440..-420 split round the door (its lintel from 2550).
  - Its floor is a deck plate with the hatch's hole cut. The house's shell is empty, so the vestibule closes itself in.
- **The hatch stair:** straight, not a ring chord any more.
  - It runs at x -700 (`HatchStairX`) from y 850 (`HatchStairTopY` = 1000 - `HatchApproach` 150) down toward -Y at 32° to y -110.
    The rise is 600: from 2250 (flush with the vestibule's floor: no lip at the hole) to 1650, the office's floor at the
    catwalk's level; run 960.
  - The hole is derived from it: x -790..-610, y 354..850 (HatchHeadroom 210 under the ceiling at its far end). Hatch walls sit
    on the hole's fore, aft and port edges. You enter from the +Y end: from the door along the 1.4 m passage fore of the hole
    (x -580..-440), to y ~925, then onto the stair.
  - Why: the contract's vestibule (x -800..-400) sits fore of the hall's rim (x -800 at y 0), so no ring-chord stair could top
    out in it. The office was already under it (radius 1100..1500 at 0° = x -800..-400). So the stair comes down into the
    office, and the office opens onto the catwalk.
- **The office:** `OfficeHalfWidth` 1000 (was 200), as wide as the vestibule over it.
  - New aft wall at x -830..-800, either side of a doorway |y| < 190 (`OfficeDoorHalf`, the hall wall's 10° gap). Walls run
    to the ceiling (`OfficeWallUp` 500).
  - The desk moved across to y -600 (`DeskAcross`), clear of the stair's foot: `DeskTop()` = (-580, -600, 1732). It is the
    manifest's spot (`GetOfficeSpot`; LEVEL reads it live). The safe is at y -900 (`SafeAcross`).
  - Flow: door -> vestibule -> stair -> office (manifest) -> doorway -> catwalk -> hall.
- **The hall:** `HallCenter` (-1900, 0) (was -1800, the contract's). `HallFloor` 1250 (2250 - 1000). Walls to 2150, ceiling
  2150..2220 over x -3030..-380, |y| <= 1130 with the hole cut.
  - `HallSkinHalfBeam` 1237.5 holds: slab sides from z 900. `AlcoveDepth` 97.5. Office, stairs, catwalk and valves as before
    around the new centre.
  - The floor stairs (225->260°, 315->350°, 33.9°) are untouched. They don't cross the hatch stair, which is now in the office.
- **The berth** (rechecked against the bulwark):
  - `BerthUnderDeck` 150 -> **-100**: her deck 1 m over the barge's. At 150 the plank from her rail (y -1575) to the landing
    (y -950) passed the bulwark's line at 2181, 69 cm under the deck, through the hull's side. Now its floor is 18.6 over a
    15 cm sill at the bulwark's inner face (`PlankSillClearance`).
  - `BerthOut` 1950 holds: her bag (half-beam 527) is 185.5 outside the bulwark (`BerthBagClearance`).
  - `JumpOut` 1650 -> **2050**. At the full 12° list the bulwark's top swings 611 inboard of her keel line, and 1026 over her
    keel. That leaves her bag 84 clear (`JumpBagClearance`); 1650 put it 114 inside.
  - The jump from the gap is now about 2.6 m out and 5 m down to her deck (it was about 3.5 m down).
  - `GetGrappleAnchor` bites the bulwark's top: (2000±600, -1230, 2360); it used to bite mid-air at y -1120.
  - `BoardingInset` 150: the landing is (2000, -950, 2250). The player start by the plank is (2000, -900).
  - After the manifest the player is put back before the door: (-250, 0, 2350), facing the door.
- **The drum.** `DrumUnderKeel` (-889, 323, 0) stays: the hose still hangs from it, and `AdvanceHose` needs nothing on deck.
  - The gantry (the jam-the-drum optional) moved onto the deck at `DrumOnDeck` (-100, -750): port side by the house, a drive
    shaft implied. The drum is under the house, which has no deck to stand it on, and the roof is out of reach.
- **The falling third stack.** `FallingStackFoot` (-240, 800), 10 cm clear of the house's fore wall (its collision x -420..-400) on the starboard side.
  `StackFallAft` -25: it falls toward the port lane, a little forward, and lies from the foot to (352, -469) across the after
  deck before the door. On the escape you go round its tip on the port side.
- **Lamps.** `DeckLampSpots`: over the door (-340, 0, 2780), and at the crane's head (900, 700, 3370).
- **Fires.** `SternFireSpots` carry their own surface: 2 on the after deck, 4 on the house's roof. `FallSmokeSpot` is over
  the roof (-2600, 0, 3050). `IsAboard` covers the whole barge: x -3400..3450, |y| <= 1297.5, z 1100..3150.

## The arena's layout (barge frame, cm)

| Piece | Spots |
|---|---|
| Fight spots (9; LEVEL and the groups count on 9) | (2350,350) (2250,-350) (1550,-300) (1550,750) (1000,200) (950,-750) (400,700) (350,-250) (-50,350) |
| Cover (10, ±110 x ±80 x 130) | (2500,350) (2400,-350) (1700,-300) (1700,750) (1150,200) (1100,-750) (550,700) (500,-250) (100,350) (-50,-350) |
| Vents (8, 2 x 2 m) | (2300,0) (1950,400) (1400,-550) (1400,450) (850,0) (750,-700) (250,0: before the door) (2900,450) |
| Horns (4, against the bulwark) | (2800,-980) (1900,980) (800,-980) (300,980) |
| Stern fires (6) | deck (-200,-450) (150,750); roof (-2900,-600) (-2400,700) (-1600,-850) (-900,500) |
| Drum gantry / falling stack / crane / lamps | (-100,-750) / (-240,800) / (900,700) hull's / door, crane head |

- Each fight spot is 150 aft of a crate (40 cm behind its aft face), so the riflemen face the berth.
- The `Looter.World.Barge.Deck` test checks that every spot is in the arena and that crates, vents and spots don't overlap.

## Files
- **`World/BargeRules.h` / `.cpp`:** all the numbers above, plus `VestibuleArea`, `OfficeArea`, `PlankSillClearance`,
  `BerthBagClearance`, `JumpBagClearance` and the new `HatchStair`, `HatchSpot`, `IsOpenDeck` (the arena), `IsAboard` (whole
  barge) and `DeskTop` (across). `StackSpots` -> `FallingStackFoot`; `HatchStairFromAngle` -> `HatchStairX`
  / `HatchStairTopY`; `bUnderTripoDeck` -> `bOnHullDeck`.
- **`World/BargeBoarding.h` / `.cpp`:** `HullPath`; the Hull collides with the model; `HasTripoHull` -> `HasHull`;
  `BuildVestibule`; the getters moved to the new **`World/BargeBoardingSpots.cpp`** (BargeBoarding.cpp 596 -> 479 lines).
- **`World/BargeBoardingKit.cpp`:** plates, rail walls, hatch walls, the stack, the stand-in crane, lamps, rails.
- **`World/BargeBoardingHall.cpp`:** vestibule, office, straight hatch stair.
- **`World/BargeBoardingDying.cpp`:** the fires, the stack and the smoke read the rules.
- **Tests:**
  - `Tests/BargeBoardingTests.cpp` (Kit, Hall, Deck) and the new `Tests/BargeBoardingBeatsTests.cpp` (Grates, Valves, Cage,
    Dying, Fall) share the new `Tests/BargeTestWorld.h`.
  - The new checks: the plank, bag and jump clearances; the ship's numbers against `ALamplighterShip`; the bulwark bites;
    the stair in the vestibule and its foot in the office; the desk clear of the stair; the way in (pawn traces from the door
    to the stair's top); the vestibule roofed; the hull collides only when it's ours; the plates over the stand-in only; the
    crates, vents and spots apart.
- **`CODEMAP.md`.**

## Round 3 (the review, 07:20)
- The HallFloor box is no wider than the skin less SkinGap (`HallFloorOut` 140 along, `HallFloorThick` 30); the hatch stair's top is flush
  with the vestibule's floor (`TopUp` = `HallUnderDeck`); `RadioOnDesk` (30, -50, 15), on the desk.
- `IsOpenDeck` is the arena and the blocks (the hole is in the vestibule). The rail walls come from `RailWallLayout()` (named
  footprints; `RailWallThick` 30); the Kit test checks each wall against it.
- New `Looter.World.Barge.Hull`: the imported hull's collision alone (component traces): the deck's top at the arena's corners,
  the gap open at BerthX ± 130 and closed at ± 170, the door open at y 0 and closed at ± 140, nothing inside the house at 2350
  or in the hull's hollow at the hall. Skips with an info line without the asset.
- The keep-apart test now holds the crane's post (`CraneFootHalf` 106), the falling stack's foot and the gantry
  (`DrumGantryHalf` (200, 130), ABargeHoseDrum's housing) too.
- Named: `PlankStartIn` 200, `StandUp` 100, `SpotUp` 20, `HallSpotAcross` 120, `HallSpotIn` 150, `HallSpotRow` 180,
  `DeckLampCandelas` 35, `DeckLampReach` 2600, `DeckLampColor`, `StandInMastWidth` 70, `StandInJibWidth` 45,
  `StandInJibDepth` 55, `StandInJibOut` 0.45, `StandInJibDrop` 60, `StandInJibTilt` 8.
- Gone: `HullStackSpots`, `ShipBagBottom`, `NumDeckPlates`, `BargeLooks::TripoDeckPlate`, `TripoRail`, `GetWalkables` and its
  `Walkables` list.

## Not settled
- The hull's UCX must leave the inside of the house and hull empty: `Looter.World.Barge.Hull` checks it once imported.
- The vestibule and the office have no light of their own: the door lamp outside and the cage light through the doorway.
  Judge it in PIE.
- The 5 m drop for the jump: no fall damage was checked; Hob's catch stays as the net.

## PIE check (after Main's build, the model's import and LEVEL's rebuild, the level saved)
1. Tests:
   - `Tools\runtests.ps1 -Filter Looter.World.Barge`: Kit, Hall, Deck, Grates, Valves, Cage, Dying, Fall.
   - `Tools\runtests.ps1 -Filter Looter.World.PilotSortie`: the berth and jump berth moved.
2. Start PIE: `Tools\pie.ps1` on Lvl_RansomsRest.
3. The deck: `Tools\console.ps1 "Looter.Barge.Show 3 aboard"`. The player stands at the plank (2000, -900) facing inboard.
   - Grab (`Tools\grab.ps1`): feet on the hull's deck; the bulwark's gap; the crates and vents; the house aft; no kit plate or
     rail over the deck.
   - Walk to the bulwark (W only, never Ctrl or Delete): it stops you; you can't climb it.
4. The door: `Looter.Barge.Show 4 aboard`. The player stands at (-250, 0) facing the house's door.
   - Grab the door: the vestibule's iron through it.
   - Walk in, along the passage fore of the hole, to the stair's top at y ~925.
   - Grab down the stair: the office below, no sky.
   - Walk down: the office; the desk at its port end; the doorway onto the catwalk.
5. The hall: from the catwalk by the office's doorway, looking aft.
   - Grab: no sky through the ceiling or the doors, the alcoves closed, the furnace pipes ending at the ceiling.
6. `stat unit` on the deck and in the hall.
