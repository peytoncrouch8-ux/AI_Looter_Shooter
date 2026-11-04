# BARGE lane (2026-11-04): the kit on the reworked Tripo hull

Contract: `CONTRACT.md`, "Round 2 ... the Tripo hull REWORKED" (the user, 08:35, after seeing the scripted hull in PIE: "remove
the hull we just made, and use the Tripo model but rework the model to fit our needs and expectations"). REWORK reshapes
`/Game/Art/Tripo/SM_DredgeBarge` in Blender; LEVEL rebuilds the placements (`BARGE_LEVEL.md`); MEASURE measured the original
Tripo hull (`BARGE_MEASURE.md`). This lane: the barge's C++ and tests. Route: **DeckTop** = the hull's deck; `KitLift` stays 0.

## Status
- **Round 1** (05:30-06:20): the parametric code. **Round 2** (07:00): the kit on the scripted hull. **Round 3** (07:20):
  the review's fixes. **Round 4** (08:40): the kit on the reworked Tripo hull, the hall at deck level in the house (below).
  Round 4 is in `Source/`, not built, no tests run.
- **Next**: after Main's build and the reworked model's import, `Looter.World.Barge.*` and the PIE check at the end.
- The sections after "Round 4" are round 2-3's. The arena, berth, plank, jump, spots, horns, vents, crates, falling stack
  and gantry still hold. The vestibule, stair, hole, cover, office and hall parts there are superseded by Round 4.

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
