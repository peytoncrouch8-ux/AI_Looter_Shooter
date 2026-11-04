# Barge kit props (lane PROPS, 2026-11-04): the manifest the KIT adopts

The art audit's plan row 3 ("the hose drum a cylinder on a cube, the furnace hall cones and cylinders"), made for real:
`Art/Models/Landfall/BargeKit.py`, 16 models, scripted on our sets, nothing imported. Contract: `CONTRACT.md` "Round 3,
more lanes". Previews `Saved/ArtPreviews/BargeKit/` (each alone, `lineup.png`, `lineup_small.png`, and the pieces laid as
the code lays them: `assembly_hall.png`, `assembly_hall_eye.png`, `assembly_deck.png`, `assembly_gantry.png`,
`assembly_burst.png`; furnace close-ups `BargeFurnace_front/_back/_porthole.png`, `BargeFurnaceBurst_front.png`).

**Round 2 (after the PIE look check, `Saved/ArtPreviews/BargeRework3/`; previews `Saved/ArtPreviews/BargeKit/r2/`, with
`BargeDrumGantry_aft/_fore/_side.png`):** SM_BargeDrumGantry dressed as her machinery (it read as "a plain stand-in box":
below); BargeGunmetal's tint `#8A98A8` -> `#6A6560` (darker, warmer: in the deck's low sun it read pale grey-white beside
Tripo's brown iron; every gunmetal piece changes with it, the instance only); the deck vent's louvres and under-bars
IronBlack -> BargeGunmetal (they read as a black void over the dark pit). Same pivots, sockets, slots (the gantry gains
three) and collision as below; exported to `Intermediate\ArtExport_Props` (16, 0 tangent warnings). Re-import overwrites
MI_BargeGunmetal's tint.

## How to read this

- Every number is Unreal cm in the piece's own frame (X forward, Y right, Z up), as `BargeRules` reads.
- **Folder:** the script is in `Art/Models/Landfall/` (not `Props/`): `models.ps1` names the Unreal folder after the
  script's folder, so the assets land at `/Game/Art/Landfall/SM_<Name>` as the contract asks. `/Game/Art/Landfall` is
  not under `DirectoriesToAlwaysCook`: `BargeLooks::Find` in constructors makes hard references (cooked); anything
  loaded only with `BargeLooks::Load` at play needs `+DirectoriesToAlwaysCook=(Path="/Game/Art/Landfall")` (or move the
  script to `Props/` and use `/Game/Art/Props/...`).
- **Import (Main):** `Tools\models.ps1 -Only BargeKit` (editor open). Export-tested: `Intermediate\ArtExport_Props`
  (`Tools\artrun.ps1 -Export BargeKit -Family Props`), tangent check 0 corners on all 16, a second export identical
  (only FBX IDs and time stamps differ).
- **Materials:** slots by name (`MI_<slot>` in `/Game/Art/Materials`). Shared, unchanged recipes: `IronBlack`, `BrassWorn`,
  `PaintOxide`, `PaintCream`, `PaintBlack`, `CinderGlow` (GrenadeJars' additive glow, Glow 4). **Two new instances**:
  `BargeGunmetal` (MetalRust tinted `#6A6560` since round 2, was `#8A98A8`: dark weathered iron with brown rust, as her
  casing and stacks; MI_MetalRust's grey-green read pale and busy beside her) and `HoseRubber` (Polymer `#36322E`). Texel density 1024 px/m everywhere
  except BargeGunmetal at 512 px/m (MetalRust's rust patches are painted for 320 px/m: at 1024 they shrink to a 3-5 cm
  print; previewed). Never `SetMaterial(0, ...)` on these meshes: slot 0 is not the glow. Use
  `GetMaterialIndex(TEXT("CinderGlow"))` (works on ISMs) and override that index.
- **Glow rule:** `CinderGlow` is additive (M_StylizedGlow), so every model is Nanite off (the importer forces it anyway).
  Cold/quiet = override the CinderGlow slot with `MI_IronBlack`; hot/tell/burn = leave it (the model's own).
- **Not done here:** no import, no editor, no `Source/`/`Content/`/`Tools/Unreal/` edit, nothing committed.

## The models

| Model | Size (cm) | Tris | Slots (in order) | Collision |
|---|---|---|---|---|
| SM_BargeFurnace | r 273 plinth, drum r 250, crown 600, collars to 641 | 7926 | IronBlack, BargeGunmetal, BrassWorn, CinderGlow, PaintCream, PaintBlack | 1 UCX hull |
| SM_BargeFurnaceBurst | as above; open door to x 411 | 7986 | same | 2 UCX hulls |
| SM_BargeCageShard | 44 x 30 x 12 | 98 | IronBlack | none |
| SM_BargeFurnaceGrate | 180 deep (radial) x 264 at its outer edge x 10 | 1149 | IronBlack, CinderGlow | 1 UCX hull |
| SM_BargeDraftValve | x -46..52, y +-36, z 0..301 | 2900 | IronBlack, BargeGunmetal, BrassWorn, PaintCream, PaintBlack | 2 UCX hulls |
| SM_BargeValveWheel | 89 across, x -6..10 | 888 | BrassWorn, IronBlack | none |
| SM_BargePipe | r 25 (flanges 33) x 300 | 896 | IronBlack, BargeGunmetal | 1 UCX hull |
| SM_BargeDeckVent | 200 x 200 (handles to x +-111) x 9.5 | 1220 | BargeGunmetal, IronBlack, CinderGlow | none |
| SM_BargeFurnaceVent | 150 x 150 x 105 (hangs, z -105..0) | 1068 | IronBlack, BargeGunmetal, CinderGlow | none |
| SM_BargeFireTongue | ~100 across x 100 (z -3..100) | 696 | CinderGlow | none |
| SM_BargeStack | r 150 tube, lip r 178, z 0..1400 | 5572 | BargeGunmetal, IronBlack, PaintOxide | 5 UCX hulls |
| SM_BargeStackFoot | 380 x 380 x 9.4 | 1004 | BargeGunmetal, IronBlack | 1 UCX box |
| SM_BargeHoseDrum | r 114 (flanges 110, teeth 114) x 278 | 5292 | IronBlack, BargeGunmetal, HoseRubber, BrassWorn | none |
| SM_BargeDrumGantry | x -254..207, y -222..141, z 0..380 | 7933 | BargeGunmetal, IronBlack, PaintOxide, BrassWorn, HoseRubber, PaintCream, PaintBlack | 5 UCX hulls |
| SM_BargeDrumLever | 14 x 15, z -15..112 | 340 | IronBlack, BrassWorn | none |
| SM_BargeRadio | case 30 x 45 x 30, aerial to 68 | 1956 | PaintOxide, IronBlack, PaintBlack, BrassWorn, PaintCream | 1 UCX box |

LODs (Nanite off): 50% and 25% at screens 0.45/0.18 on most; Stack 0.5/0.2; ValveWheel and Radio one LOD (50%); Shard,
FireTongue, Lever none. Baked vertex occlusion and soot (crown, stack top, the cage cavity, over the vents) in the alpha.

### SM_BargeFurnace, SM_BargeFurnaceBurst (ABargeFurnace)

- Pivot: the hall floor's middle (the actor's origin); +X the cage's window, as ABargeFurnace's frame. Reference
  `FurnaceRadius` 250, `FurnaceHeight` 600, `CageUp` 250: scale `(FurnaceRadius/250, FurnaceRadius/250,
  FurnaceHeight/600)` (1,1,1 today); sockets move with it.
- What it is: a riveted drum of three lapped courses (r 251.5 to z 200, 250 to 400, 248.5 to 521) on a plinth (r 273,
  0-14), iron bands (40-58, 392-408, the crown's seat 508-532 r 260), butt straps at +-60/+-135 degrees, a domed crown
  (531 -> 600) with a manhole (to 608) and three flanged pipe collars (r 31, flange r 40, to 640) at the code's pipe feet
  (-120, +-150) and (-220, 0); a stoke door at the back (-X, z 108, 74 x 62) and two ash vents at +-Y (z 80, 54 x 24),
  all three louvred over the CinderGlow slot; two brass gauges (+-40 degrees, z 445); a ladder up the back (z 158-510).
- **The soul cage (design change):** the heart hangs **inside** the furnace behind a round barred brass porthole (as
  the Tripo boiler's glowing port), not 30 cm outside the drum's face. The drum has a hole +-30 degrees, z 145-355; a
  flat riveted face plate closes it (front at x 214.8), a sill (z 139-147) and a brow (353-364, brass plate on it)
  follow the drum's curve; the porthole ring (clear r 82, outer r 100, front at x 236) has four vertical bars and one
  across at x 224-227, a hinge on +Y, a dog on -Y; behind it the heart's cavity, clear r 83, x 61..207.
- Code: `Body` = this mesh at the origin, scale as above (no material set). Hide `Crown` (it already hides when the body
  is found), hide `CageFrame`. From `Ironwork` drop the three bands and the five cage bars (the model has them); the
  three pipes start at SOCKET_Pipe_1..3 (z 640) instead of `PipeFoot` 575, as SM_BargePipe (below), or keep the
  cylinders from 640.
  - **HeartAt from SOCKET_Heart (150, 0, 250): `HeartOut` 30 -> -100.** Then: the heart (110-150 cm, its smog 120-165)
    fits the cavity (166 across); `CageLight` at HeartAt + (60,0,0) = (210, 0, 250) shines out of the porthole; the
    interaction point HeartAt + 80 = (230, 0, 250) sits at the porthole; the box at HeartAt + 40, half (110, 140, 140),
    reaches 64 cm in front of it (to x 300). `GetCageLocation`/`GetCageFacing` follow.
  - Burst: `SetCageBurst(true)` swaps `Body` to SM_BargeFurnaceBurst (same pivot, sockets, slots, hull plus one for the
    open door); `CageBurst` swaps it at the moment of the burst. The burst: porthole open 118 degrees on its hinge
    (to x 411, y 144), two bars snapped to stubs, two bent out, the cross bar bent, the dog torn off, cavity and face
    sooted.
  - Optional: `PutOut()` (and all three valves shut) sets the CinderGlow slot to IronBlack (the stoke door and ash vents
    go dark).
- Sockets: `Heart` (150, 0, 250); `Window` (236, 0, 250) facing +X; `Pipe_1` (-120, 150, 640), `Pipe_2` (-120, -150,
  640), `Pipe_3` (-220, 0, 640) up the pipe (+Z; their forward is +X); `Crown` (0, 0, 610) facing up (the valve sparks);
  `StokeDoor` (-260.5, 0, 108) facing -X.
- Collision: one convex hull, the drum and crown (r 273 at the floor, 262 at 532, 150 at 580, to 610): `BlockAll` as now
  (it's the mesh's simple collision). The niche and porthole sit inside it; shots at the porthole stop on the hull.

### SM_BargeCageShard (ABargeFurnace::Shards)

- A torn iron plate (folded 16 degrees, two rivets) with a bent bar stub on it, 44 x 30 x 12; pivot the middle of its
  foot. Code: `Shards` mesh = this, no material set; each instance at (Out, Across, **0**) (drop the cube's +8), its
  turns as now, uniform scale `FRandRange(0.6, 1.3)` (not the cube's thin-plate scales). No collision.

### SM_BargeFurnaceGrate (ABargeFurnace::GrateCold / GrateHot)

- One wedge of the ring: a riveted frame (rails 7 wide, 9 high), six concentric bars, two ribs, a dark pit floor and the
  ember bed (CinderGlow) under the bars. Built at `GrateInner` 340, `GrateOuter` 520, `Grates` 12 (half angle 15 degrees
  less a 4 cm gap between wedges at the middle radius).
- Pivot: on the floor at the ring's middle radius (`(GrateInner + GrateOuter)/2` = 430) on the wedge's bisector; +X out
  along the radius (as `WedgeTransform` lays its cube, but with Up 0).
- Code, `WedgeTransform(Wedge, 0)`: location `(cos a, sin a) * Middle`, z 0; yaw `a`; scale
  `X = (GrateOuter - GrateInner) / 180`, `Y = Middle * 12 / (430 * Grates)`, `Z = 1` (1,1,1 today; the smaller hall,
  R 650: X 0.72, Y 0.94). `GrateCold` = this mesh with its CinderGlow slot set to IronBlack; `GrateHot` = this mesh as
  it is (do not set slot 0 to Glow: that lights the iron).
- Fire over a burning wedge: SM_BargeFireTongue at `WedgeTransform(Wedge, 9)` (SOCKET_Fire, the rails' top), yaw 0,
  scale (1.6, 1.6, 1.3) as now, pivot at its foot (no centre offset).
- Collision: one hull (the wedge's outline, 9 high). The ISMs have none now (players stand on the hall floor's box, the
  grate 9 cm proud, feet sink into it); turn it on (`QueryAndPhysics`) if they should stand on it.

### SM_BargeDraftValve + SM_BargeValveWheel (AFurnaceDraftValve)

- Valve pivot: the actor's origin, on the floor `ValveIn` 30 in from the hall's wall, +X into the hall. Its pipe stub
  (r 22.5, the run in BargeGunmetal) at x -10 like the stand-in, 0 to 300: floor flange (r 36, 6 bolts), the valve body
  inline (z 95-166, r 34), the bonnet and stem out along +X at z 130 (flange r 21 with 6 bolts, stem to x 52), a bolted
  joint at 250-268 (the steam point), the top flange at 300; two wall straps (z 52, 228) whose lugs run into the wall
  (to x -46); a brass gauge (z 205), a brass drain cock.
- Code: `Pipe` = this mesh at the origin, scale 1 (drop the cylinder's location (-10,0,450) and scale); its two hulls
  (the stub, r 34 octagon 0-300; the body and bonnet) replace the cylinder's `BlockAll`. `Wheel` = SM_BargeValveWheel
  at `WheelAt` (45, 0, 130) = SOCKET_Wheel, **visible** (it's no longer hidden when the valve model exists), rotation
  `FQuat(FVector::XAxisVector, TWO_PI * SpinTurns * Eased)` only: drop `* FQuat(Y, HALF_PI)` (the wheel's axle is its
  own X). Steam at SOCKET_Steam (20, 0, 260), as the code has it.
- The riser to the ceiling: SM_BargePipe from SOCKET_PipeTop (-10, 0, 300) up `HallHeight - 300` (600 today):
  `N = max(1, round(L / 300))` sections (2), each scale (0.9, 0.9, L / N / 300) (45 cm pipe on the 50 cm model).
- Wheel: brass rim (r 41, tube 3.4, dished 3 back to -X), five spokes, hub r 7.8 (x +-4.5), iron nut, a spinner knob;
  pivot the hub's middle; no collision.

### SM_BargePipe (the furnace's three pipes, the valves' risers)

- A 300 cm section, 50 across (r 25; flanges r 33 at both ends with 8 bolts each, so stacked sections make a bolted
  joint), a riveted seam strap on its +Y side; pivot the middle of its foot, along +Z.
- Scale: XY = diameter / 50 (the furnace's 55 cm pipes 1.1; the valves' 45 cm 0.9); for a run of length L lay
  `N = max(1, round(L/300))` sections, each Z = L / N / 300 (flanges stretch at most 1.33x). Furnace pipes: from
  SOCKET_Pipe_N (z 640) to the ceiling's underside (`HallHeight` 900): L 260, one section, scale (1.1, 1.1, 0.867).
- Collision: an octagon hull r 33; the furnace's pipes had none (Ironwork ISM): keep `NoCollision` there.

### SM_BargeDeckVent (ABargeBoarding::VentCold / VentGlow / VentFire)

- The 2 x 2 m vent (`VentHalf` 100): a riveted coaming (`VentRimWidth` 12 wide, 8 high, BargeGunmetal), six louvres
  (`VentSlats`) tilted 28 degrees, two under-bars, a dark pit floor and the lumpy ember bed (CinderGlow) under them,
  two drop handles outside the rim on +-X. **Nothing goes under the deck** (the deck is the hull's: a pot below it would
  never show), so it stands 8 cm proud (9.5 at the bolts) instead of the stand-in's 1 cm; no collision, so nothing to
  trip on.
- Pivot: the vent's middle on the deck. Code: location `Spot` (z `WellDeckZ`; drop `+ VentGrilleUp`), no turn, scale
  `(VentHalf/100, VentHalf/100, 1)`. `VentCold` = this mesh with its CinderGlow slot set to IronBlack; `VentGlow` = this
  mesh as is (drop `SetMaterial(0, Glow)`). `VentFrame` is already skipped when the vent mesh differs from the cube.
- The column: `VentFire` = SM_BargeFireTongue at `Spot + (0, 0, 8)` (SOCKET_Fire), scale `(VentColumnRadius/50,
  VentColumnRadius/50, VentColumnHeight/100)` = (2.6, 2.6, 4.5); pivot at its foot, so no `+VentColumnHeight/2`.

### SM_BargeFurnaceVent (new: the deck vent seen from the hall; optional)

- A riveted hood under the deck: a 150 x 150 bolted flange plate against the ceiling, a square hood narrowing to a
  round flue collar (r 31) with a bell mouth (r 42, to z -103), the flue's throat on CinderGlow, a damper lever with its
  weight. Pivot: its top middle (on the ceiling's underside); everything hangs below it.
- Place: under each `VentSpot` whose hood stays inside the hall: `|Spot - HallCenter| + 106 <= HallRadius` (today 3:
  (2150, 0), (2700, -100), (3150, -150); (2750, 600) and (3300, 250) would cut the wall), at z `HallCeiling -
  CeilingThickness` (2200), yaw 0. (2700, -100) clears the furnace's pipe at (2580, -150) by 9 cm at its flanges and
  its collars by 1.5 m; (2150, 0) clears the back valve's riser by 25 cm. Its CinderGlow slot follows that deck vent's phase (Quiet: IronBlack). No collision.

### SM_BargeFireTongue (vent columns, burning grates, the stern fires)

- Six flattened licks twisting up, the middle one tallest, on CinderGlow (additive: crossings burn brighter); pivot the
  middle of its foot; ~100 across, 100 tall (a lick dips 3 cm under its foot). Scale `(radius/50, radius/50, height/100)`.
- Uses (each in place of the engine cone, located at the fire's foot, not its middle): the vents (above), the grates
  (above), `SternFire` at each `SternFireSpots` spot with scale (2.2, 2.2, 3.2) (drop the `+160` z). No collision.

### SM_BargeStack + SM_BargeStackFoot (ABargeBoarding::FallingStack, FallingStackPivot)

- The kit's third stack in the Tripo stacks' look: a gunmetal tube (r 150, two plate courses alternating r 151.2/150),
  five iron bands (r 155.5: z 222-238, 452-468, 682-698, 912-928, 1132-1148) and a heavier collar (r 158, 1196-1232),
  14 rivets each; a flared mouth with a rolled lip (r 178 at 1297, the throat sooted down to 1250); the Tripo dome rain
  cap (PaintOxide, r 123, 1334-1396, finial to 1400) on four struts; a bolted foot flange (r 175, 0-13, 16 bolts) with
  seven gussets to 74; a ladder up its +X side (rails at y +-22, 17 out, z 50-1230, 36 rungs); three guy eyes on the
  collar. Soot heaviest over the top third and down from the mouth.
- Pivot: the middle of its foot; its length along +Z. Reference `StackRadius` 150, `StackHeight` 1400 (cap's top):
  scale `(StackRadius/150, StackRadius/150, StackHeight/1400)` (1,1,1 today). Code: `FallingStack` = this mesh at
  relative location (0, 0, 0) on `FallingStackPivot` (drop `+StackHeight/2`), no material set; the ladder (+X) faces
  fore, onto the well deck, at yaw 0.
- Collision: five hulls (the foot and gussets; the tube in 0-470, 470-940, 940-1306 with the ladder and the mouth; the
  cap), `BlockAll` as now: lying on the deck after step 7 it rests on them along its length.
- The fall (a note, the code's call): `DropStack` turns the stack about its foot's middle, so at 88 degrees the near
  half of its foot (r 175) sinks into the deck. Pivoting at the flange's edge on the fall side (`FallingStackFoot +
  FallDir * 175`, the mesh at `-FallDir * 175` under the pivot) lands it on its side on the deck.
- SOCKET_Smoke (0, 0, 1300) at the mouth facing up (her smoke as she dies), SOCKET_Top (0, 0, 1400).
- SM_BargeStackFoot: the doubler plate it stands on (380 x 380 x 3, 20 bolts round r 186, 16 sheared bolt stubs where
  the stack's flange bolts were, a soot ring); pivot its middle on the deck. Lay it at `FallingStackFoot` on `Kit` (not
  on the pivot): it stays when the stack falls. Collision one flat box (4 high); none needed.

### SM_BargeHoseDrum (ABargeHoseDrum::Drum)

- The drum that turns: an iron core (r 75), gunmetal flanges (r 110, 0-20 and 220-240), the hose (HoseRubber, r 8.5)
  wound in one layer of ~10.6 turns (z 28-212, its ends tucked into the core), 32 ratchet teeth round the top flange's
  rim (to r 114), eight low ribs and rivets on top, a brass hub and axle (to 278).
- Pivot: the middle of its foot, on its axis (it turns about its own Z). Code: `Drum` = this mesh at (0, 0, 50)
  (= the gantry's SOCKET_Drum), scale 1, **visible** (drop `SetVisibility(Tripo == nullptr)`: the gantry has no drum), no
  material set (or set its `HoseRubber` slot to `MI_Landfall_HoseRubber` to match her hanging hose); its yaw turn as now.
  Its axle's top (z 328 in the gantry) sits in the gantry's bearing (322-352). No collision.
- SOCKET_Axle (0, 0, 278).

### SM_BargeDrumGantry + SM_BargeDrumLever (ABargeHoseDrum::Housing, ::Lever)

- The housing on the forecastle: a riveted box 400 x 260 x 140 (`DrumGantryHalf` 200 x 130; foot flanges to 414 x
  274) in BargeGunmetal, the drum's well (r 118, floor at 46), two bolted hand-hole covers. Round 2's dressing: angle
  irons (11 cm legs) riveted down every corner, round the top (a lipped cap, 3.9 proud: a shadow line) and along the foot
  (bolted to the deck); a riveted lap seam along each long side at z 72; three stiffener ribs a side (+Y x -40, 50, 132;
  -Y x -150, -60, 40); a raised oxide hatch on +Y (x -125, z 70: a riveted coaming, the stiffened door on it, five dogs,
  two hinges, a grip); on the aft end (-X, what the well deck sees) the fairlead's gear case (96 x 84, 16 proud, a
  rounded top, bolted flange), a bell-mouthed fairlead (z 46) with two brass rollers, the hose (HoseRubber) out of it and
  down a bolted hawse collar into the deck at x -235 (the hose and collar stand out to x -254: no collision), a pressure
  gauge (y 88) and the Company's brass plate (y -86: border, rivets, a lamp lozenge, two stamped lines; no real name),
  soot round the fairlead; on the fore end a louvred motor vent (y -60) and a seam. Over it the red gantry (PaintOxide): two legs
  at x +-165, knee braces, a crossbeam (352-380) with the drum's top bearing under it (322-352). On -Y the jam lever's
  bracket: two cheeks round an iron pin along X at (140, -172, 96), and an iron quadrant (r 64-74, -42..+36 degrees,
  brass stops at -35 and +32) on a strut, at x 146.
- Pivot: its middle on the deck. Scale `(DrumGantryHalf.X/200, DrumGantryHalf.Y/130, 1)` (1,1,1 today; never Z).
  Code: `Housing` = this mesh at the origin, scale as above (drop the cube's (0,0,70) and (4, 2.6, 1.4)); `BlockAll` as
  now on its five hulls (the housing 412 x 272 x 143, the two legs, the beam, the lever's bracket).
- The lever: `Lever` = SM_BargeDrumLever at **SOCKET_LeverPivot (140, -172, 96)** (not `LeverAt` (140, -160, 110), a
  middle, which the cylinder needed), scale 1, rolled about X as now: +35 thrown in (its top toward the housing: the
  grip clears the rim by ~8 cm), -35 out. Keep `LeverAt` for the interaction at the arm's middle: (140, -172, 150).
- Sockets: `Drum` (0, 0, 50), `LeverPivot` (140, -172, 96), `Sparks` (0, -110, 170) tipped 60 degrees up, out over the
  well's -Y edge where the pawl bites (the jam's sparks; the code's (0, 0, 170) also works).
- Lever model: an iron hub (r 7.5, along X, x +-7), a flat iron arm to 90, a turned brass grip (86-112), a latch rod
  and a brass trigger, a pawl tooth under the hub (to -15). Pivot the pin's middle. No collision.

### SM_BargeRadio (ABargeBoarding::Radio)

- The stoker's wireless: a riveted oxide case (30 deep x 45 wide x 28 on 2 cm feet), a black front panel with two
  brass-bezelled cream dials, a meter window and three knobs, a brass telegraph key on top, a lid band with rivets, a
  short aerial (iron rod with a brass ball on a cream insulator, to 68) at its back left corner, brass terminals on the
  back, carry handles at the sides. Pivot: the middle of its foot; the dials face +X.
- Code: `Radio` = this mesh at `RadioSpot(OnDesk) - (0, 0, RadioSize.Z / 2)` (its foot on the desk's top), scale 1, no
  material set, yaw `OfficeOut() > 0 ? 0 : 180` (the dials face into the room, the desk's front edge: with
  `RadioOnDesk.X` 30 its front lands on the desk's edge). `RadioSize` becomes (30, 45, 30) (the case). Collision: one
  box hull; the cube had none (`MakePiece(..., false)`), keep it off. SOCKET_Speaker (16, 0, 16) facing +X: Ned's call.

## BargeLooks paths (what the KIT puts in `BargeLooks.h`)

```
/Game/Art/Landfall/SM_BargeFurnace.SM_BargeFurnace            (TripoFurnace)
/Game/Art/Landfall/SM_BargeFurnaceBurst.SM_BargeFurnaceBurst  (TripoFurnaceBurst)
/Game/Art/Landfall/SM_BargeCageShard.SM_BargeCageShard        (new: Shards)
/Game/Art/Landfall/SM_BargeFurnaceGrate.SM_BargeFurnaceGrate  (TripoGrate)
/Game/Art/Landfall/SM_BargeDraftValve.SM_BargeDraftValve      (TripoValve)
/Game/Art/Landfall/SM_BargeValveWheel.SM_BargeValveWheel      (new: the valve's Wheel)
/Game/Art/Landfall/SM_BargePipe.SM_BargePipe                  (new: furnace pipes, valve risers)
/Game/Art/Landfall/SM_BargeDeckVent.SM_BargeDeckVent          (TripoVent)
/Game/Art/Landfall/SM_BargeFurnaceVent.SM_BargeFurnaceVent    (new, optional: the hall's hoods)
/Game/Art/Landfall/SM_BargeFireTongue.SM_BargeFireTongue      (new: VentFire, GrateFire, SternFire)
/Game/Art/Landfall/SM_BargeStack.SM_BargeStack                (TripoStack)
/Game/Art/Landfall/SM_BargeStackFoot.SM_BargeStackFoot        (new)
/Game/Art/Landfall/SM_BargeHoseDrum.SM_BargeHoseDrum          (new: the Drum)
/Game/Art/Landfall/SM_BargeDrumGantry.SM_BargeDrumGantry      (TripoHoseDrum: the Housing)
/Game/Art/Landfall/SM_BargeDrumLever.SM_BargeDrumLever        (new: the Lever)
/Game/Art/Landfall/SM_BargeRadio.SM_BargeRadio                (new: the Radio)
```

## Still engine shapes where the player looks (not modelled here)

The heart and its smog (spheres: they're the soul look's own materials, right as they are), the coal rain's fallback
sphere (SM_SoulCoal exists), the kit's stairs, catwalk, hall walls and floors (KitIron blocks with the world-metre
detail layer), the rail look (unused over the hull), the unseen rail walls.
