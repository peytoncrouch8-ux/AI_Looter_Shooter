# Art round 1 (2026-11-04): the art audit's steps 1 and 2

Source: `Docs/Work/2026-11-03-ArtAudit/AUDIT.md` (findings 1 and 3, plan rows 1 and 2). The user's pick, 2026-11-04 morning:
"Art: steps 1-2"; they are not play-testing Phase 3 yet. No Tripo credits, no new models: code and config only.

## Lanes

- **Main**: step 1 (Medium keeps its texture and creature detail) and its measurement; every build (`Tools\launch.ps1 -Build`);
  verifies the BARGE lane's work.
- **BARGE** (one agent): step 2, the walk deck onto the Tripo barge's own deck. Never builds, never runs perf or the tour.

## Step 1 (Main): Medium keeps texture and creature detail

`Settings/GraphicsSettingsSubsystem.cpp` `QualityVariables`, values {Low, Medium, High, Epic}, the engine's own where a level is
not meant to change (`BaseScalability.ini` 5.8: TextureQuality MipBias 16/1/0/0, MaxAnisotropy 1/2/4/8; ViewDistanceQuality
SkeletalMeshLODBias 2/1/0/0):
- `r.Streaming.MipBias` {16, 0, 0, 0}: Medium draws every map at its full size (was half: a 2048 map as 1024).
- `r.MaxAnisotropy` {1, 8, 8, 8}: decks, ground and walls seen at a slant stop smearing (High rises 4 -> 8 too).
- `r.SkeletalMeshLODBias` {2, 1, 0, 0}: was to be 0 on Medium; measured 0.3 ms in a horde for a difference nobody saw at
  3 m, so the user kept the engine's Medium (1) and the other three lines (2026-11-04, after the grabs).
- `r.Streaming.PoolSize` {400, 1000, 1000, 1000}: the engine's Medium pool (600 MB) would drop the mips again.
`Tests/SettingsTests.cpp` holds the Medium values. Measure, editor closed, before and after at the same code otherwise: the 42-view
Ransom's Rest tour (`Tools\tour.ps1`), the 10-spider horde row and the LMG fire row (`Tools\perf.ps1`, as the 2026-11-04 Phase 3
rows). The audit's margin: 0.07 ms at the heaviest view; a view over 8.3 ms is the user's call, not a reason to undo.

## Step 2 (BARGE): the walk deck onto Tripo's deck

The audit's finding 1, measured: the Tripo hull `SM_DredgeBarge` (75 x 24.75 x 48.49 m, pivot under its middle at the bottom,
bow +X) has its open deck 22.5 m over the keel (up-facing area 524 m2 at 22.5-24.0 m; open deck at x -24..-20 m and +6 m);
the kit's walk deck is `BargeRules::DeckTop` 1400 (14 m, "by eye"), `KitLift` 0 and never set, so the player fights 8.5 m inside
the hull, which has no collision and culls from inside. Rays down from 60 m over the kit's rectangle (x -3450..3350, y +-1150):
100% meet Tripo geometry above the walk deck, median +14.2 m, 10-90% +8.5..+30.8 m (the +8.5 is the open deck, the rest the
superstructure).
Goal: the player stands on the Tripo deck, never inside the hull or its superstructure; the engine-cube deck plates hidden
where the hull's own deck shows (keep what the kit adds: boxes, rails, the hatch stair, the hall, the drum, the furnace); the
hatch stair and the hall refitted to the new height; the berth (`BerthUnderDeck`, the Lamplighter grappled alongside)
rechecked; `build_area_main6.py`'s kit_lift log line made honest (it says "set kit_lift if they disagree" and nobody did).
Settle by measurement in the editor (Lvl_RansomsRest holds `Barge_Pilot`; `Tools\runscript.ps1` Python, rays against the
hull component): the open deck's extent in the barge's frame at ~2250 (where there is headroom of at least 220 cm under the
superstructure, and where there is none), the hatch and hall positions against the hull's own openings, the rail line. Then
DeckTop (or KitLift on the placed barge) and the kit's rectangle (`DeckAft`, `DeckFore`, `DeckHalfBeam`) follow the measured deck.
Files: `World/BargeRules.h` (DeckTop, the deck rectangle, the berth), `World/BargeRules.cpp`, `World/BargeBoardingKit.cpp`
(plates, rails, stair), `World/BargeBoardingDeck.cpp`, `World/BargeBoarding.cpp` (hull placement, KitLift), `World/BargeFurnace.cpp`
(the hall), `Tools/Unreal/build_area_main6.py` (the placement log), `Tests/BargeBoardingTests.cpp` (Looter.World.Barge.*). Not
this lane's: `Settings/*`, `Tests/SettingsTests.cpp`, any content asset, any Tripo work (plan row 3 is later).
Done looks like: the numbers measured and written in `BARGE.md` beside this file (deck extent, headroom map, the chosen
DeckTop/KitLift, what was hidden); the code changed; the tests updated and passing; a PIE grab from the deck (`Tools\pie.ps1`,
`Tools\grab.ps1`) showing the player's feet on the Tripo deck with the hull around them, and one from the hall.
### Measured (BARGE_MEASURE.md, 06:40) and decided (the user, 07:00): a scripted hull
The Tripo hull has no deck to stand on: of the kit's rectangle 28% is standable (a 23 x 16 m foredeck in terraces at
2250-2371, side rims 1-3 m, an aft platform), 50% superstructure, 13% no surface; no opening; the hull below the deck is
V-shaped (half-beam 650-900 at z 1400) so the hall (R 1100) fits nowhere. The user's pick: **a Blender-scripted hull in our
texture sets replaces the Tripo one everywhere** (the Tripo silhouette kept for the spyglass view; crisp at any size; collision
from the model). Lanes: **HULL** (the model), **KIT** (the BARGE agent, round 2: the kit onto the modeled deck), **LEVEL**
(rebuild Main 6 and Landfall's barge after the import), Main (build, import, tests, PIE, the visuals to the user).

**The hull, shared numbers (cm, the barge's frame: pivot bottom-centre at z 0 where the hose drum hangs, bow +X, Z up):**
- `Art/Models/Vehicles/DredgeBarge.py` -> `/Game/Art/Vehicles/SM_DredgeBarge` (the Tripo `/Game/Art/Tripo/SM_DredgeBarge`
  stays, unused). Code and scripts point at the new path (`ABargeBoarding::HullPath`, `build_area_landfall.py` barge_hull).
- Length 7500 (x -3750..3750), max half-beam 1237.5. Keel plate flat at z 300, bilge chines to z 900, slab sides from z 900 to
  the deck, a rubbing strake band near z 2000; the stern square over a 450 taper, the bow a raked dredger's scoop over
  x 3000..3750.
- Open deck at `DeckTop` **2250** over x **-400..3350**, walkable |y| <= **1100** inside a bulwark 110 high (`RailHeight`),
  15 thick, at |y| 1237; a 300 wide bulwark gap with a sill at x **2000** (`BerthX`) on the **-Y** side (`BerthSide`): the
  gangway's landing. No deck surface under the block's footprint (the kit floors it).
- Aft block (the house) x **-3300..-400**, |y| <= 1150, from the deck to its roof at **2900**, hollow; a door opening 240 wide
  x 300 tall at y 0 in its fore face (x -400), threshold at 2250 (`GetHatchSpot`); two stacks at the kit's `StackSpots`
  (-1107, 21) and (-2373, 19), r 150 at the roof tapering to 120, to z 4600; a wheelhouse on the roof over x -2000..-1200,
  |y| <= 600, to z 3700, dark glass; the crane at `CraneSpot` (900, 700): post to 1200 over the deck, jib 900 toward +X.
- Hull hollow below the deck everywhere (the hall, R 1100 at (-1900, 0), floor 1250, lives in it: slab sides at 1237.5 from
  z 900 up are what make it fit).
- Collision in the model (`UCX_` boxes): the deck slab, the bulwarks round the gap, the hull body, the block's walls split round
  the door, its roof, the stacks, the wheelhouse, the crane post; nothing inside the block or the hull. Sockets `SOCKET_HoseDrum`
  (-889, 323, 0), `SOCKET_Door` (-400, 0, 2250) facing +X, `SOCKET_Stack_1` and `SOCKET_Stack_2` at the two stacks' tops
  (Landfall's build reads `Stack_1`, `Stack_2` and `HoseDrum` off the hull for the plumes and the hose, as it did from Tripo's). Nanite off, a fallback that keeps every triangle, one LOD at 30% for
  the spyglass range. Our sets (MetalWorn deck and house, MetalRust hull, PaintWorn bands) through material instances, reusing
  `MI_Landfall_BargeIron`, `MI_IronBlack`, `MI_BrassWorn` where they fit; at least 1024 px/m.
**The kit (KIT, round 2):** `DeckTop` 2250, `DeckAft` -300, `DeckFore` 3300, `DeckHalfBeam` 1100; no deck plates, rails, fixed
stacks or crane drawn over the open deck (the hull has them; the third, falling stack stays the kit's); no `DeckBlocks`; the
vestibule inside the block at deck level over x -800..-400 with the hole and the stair down to the hall (`HallUnderDeck` 1000);
`HallSkinHalfBeam` 1237.5; the drum's gantry on the roof or gone (the drum stays under the keel); every deck spot (fight spots,
cover, vents, horns, stern fires) inside the new rectangle; `HullPath` to the new asset; the tests follow.

### Round 2 (the user, 2026-11-04 08:35): the scripted hull OUT, the Tripo hull REWORKED
The user, after seeing the scripted hull in PIE: "remove the hull we just made, and use the Tripo model but rework the model to
fit our needs and expectations"; asked how far: **"Reshape for the kit"** (clear the midship superstructure, move the tall tower
onto the aft house, flatten the deck to one level over the arena, raise and widen the aft house to hold the furnace hall at
deck level with a real door, keep the stacks, bow scoop and rails); textures: **"Tripo paint + our tiled detail"** (the Tripo
colour as the large-scale look, our MetalWorn/MetalRust plate detail in meters: the audit's plan row 4, a material addition).
Lanes: **REWORK** (Blender, the Tripo mesh), **MATERIAL** (the detail layer in M_World and the barge's instance), **KIT**
(round 4: the kit to these numbers), **LEVEL** (paths back to the Tripo asset), Main (import, build, rebuild, tests, PIE).
`Art/Models/Vehicles/DredgeBarge.py` and the assets `/Game/Art/Vehicles/SM_DredgeBarge`, `MI_BargeIron`, `MI_BargeRust` are
deleted (never committed); `Tools/Blender/looter_barge.py` stays only if the rework uses it.

**The reworked Tripo hull, shared numbers (cm, the Tripo frame: pivot bottom-centre at z 0, bow +X, x +-3750, y +-1237.5):**
- Built by `Tools/Blender/looter_tripo_wardens.py` (the `DredgeBarge` spec plus a rework step on the loaded Tripo mesh, so a
  re-import reproduces it) -> `/Game/Art/Tripo/SM_DredgeBarge` as before; `ABargeBoarding::HullPath`, `build_area_landfall.py`
  and `build_area_main6.py` point back at it.
- Kept as Tripo made them: the hull body below the deck, the bow scoop, the two stacks (at (-1107, 21) and (-2373, 19), tops
  ~4700), the sockets `HoseDrum` (-889, 323, 0), `Stack_1`, `Stack_2`.
- The deck: one flat level at **2250** over x **-400..3350**, full width to the bulwark; the midship superstructure (the blocks
  and the 46 m tower over x -500..1900) removed, the deck filled where they stood; the foredeck terraces (2371, 2300) flattened
  to 2250 out to x 3350 (the scoop beyond stays).
- The house: the Tripo aft block made x **-3300..-400**, full beam y **+-1237.5** (flush with the hull sides), roof at **3300**;
  the tall tower moved onto its roof over x -2000..-1200 (top ~4600); a door opening 240 wide x 300 tall at y 0 in its fore
  face (x -400), threshold 2250 (`GetHatchSpot`); hollow inside (the kit builds the hall at deck level in it: `HallUnderDeck`
  0, hall centre (-2050, 0) R 1100, the office and vestibule in x -950..-400); the Tripo side rails beside the block go.
- Bulwark 110 high, 15 thick at |y| 1237.5 along x -400..3350, a 300 gap with a sill at x 2000 (`BerthX`) on the -Y side.
- Collision in the model (`UCX_`): the deck slab, the bulwarks split round the gap and at x 3200, the hull body, the house walls
  split round the door, its roof, the stacks, the tower, the scoop; nothing inside the house. New sockets `Door` (-400, 0, 2250)
  facing +X and `Gangway` (2000, -1237.5, 2250).
- UV0 keeps the Tripo atlas (new faces take coordinates from the nearest Tripo surface, or a plain island on its deck area);
  the detail layer needs no UV (object-space triplanar). Nanite off, fallback 100%, LODs as the Tripo import makes them.
**The detail layer (MATERIAL):** `Tools/Unreal/build_world_materials.py` M_World gains an opt-in detail layer (audit plan row 4):
object-space triplanar on the mesh's own normal, a tiled set's normal and its light/dark (the set's base colour luma) over the
macro colour, `DetailScale` (meters per tile, 1.0), `DetailStrength` (0.6), `DetailNormal` (0.7), fading from `DetailNear` 200
cm to `DetailFar` 1500; off by default (no cost for every other instance); the barge's instance turns it on with MetalWorn.
Measured before and after on the barge views (`Tools\perf.ps1`, the tour's DeckFight/SpyglassBarge views).

Editor lane: one editor, Main's. It is closed for Main's measurement for about 40 minutes from 05:30; before any editor call
check `Get-Process UnrealEditor` and `Tools\mcp.ps1` answer, and wait in 2-minute steps if not. Edit `Source` directly (Main
builds; say "ready to build" when the code is in), then run the tests and the PIE check after Main's build.
