# Lane TRIPO-M23: Mains 2-3's Tripo models (2026-11-01)

Made, fitted and exported. Nothing imported, built or committed; Unreal never opened. Blender only through
`Tools\artrun.ps1`. Export: `Intermediate/ArtExport_TripoM23` (6 models). Pictures: `Saved/ArtPreviews/TripoM23/`.

## Credits (cap 250): 230 used

| Model | Credits | Result |
|---|---|---|
| SextonsFerry | 30 | used |
| ShroudedDead | 30 | try 1 (mummy-like, holes in the cloth, hollow between the feet): kept as `Art/Tripo/ShroudedDead/ShroudedDead_try1.glb` |
| ShroudedDead2 | 30 | the redo (a closed sewn shroud, four rope bands, no feet): **used**, copied to `Art/Tripo/ShroudedDead/ShroudedDead.glb` (record in `Art/Tripo/ShroudedDead2/`) |
| CompanyHoseCart | 30 | used |
| CompanySandbags | 30 | used (cut down, below) |
| CompanyCheckpointSign | 30 + repaint 20 | first paint was grey clay (`_grey.glb`); repaint with `Art/Tripo/style/cottage_preview.png` gave weathered wood posts/frame but pale parquet on the face: the face is the game's PaintBlack (below) |
| CompanyBanner | 30 | used |

Shared balance after: 85 (TRIPO-PEOPLE spends from it too). Specs: `Art/Tripo/specs/m23.json`
(`python Tools/tripo.py batch <cap> m23`). M23-2 is one model (the 1.1 m child: the same mesh at scale 0.61; no code uses one yet).

## Files (new)

- `Tools/Blender/looter_tripo_m23.py`: BASE specs on `looter_tripo_wardens.build` (cleaned mesh, graded maps, MI_Tripo<Name>
  on M_World, Nanite off, LODs 50/25/12) plus one fit per model (`FITS`). `LOOTER_TRIPO_ONLY=A,B` limits a run.
- `Art/Models/Tripo/WardensMains23.py`: the model source (category Tripo -> `/Game/Art/Tripo/SM_<Name>`).
- `Tools/Blender/tripo_m23_sheet.py`: `--check,Name` (Check_<Name>_q/side/front/top/back.png, hulls red, sockets
  magenta) and the sheet (Sheet.png = Sheet_Line.png beside a 1.8 m figure, the Tripo loudspeaker post and sacks, the
  scripted flour sacks and cocoon, then each model's check).
- `Art/Tripo/{SextonsFerry,ShroudedDead,ShroudedDead2,CompanyHoseCart,CompanySandbags,CompanyCheckpointSign,CompanyBanner}/`,
  `Art/Tripo/specs/m23.json`, `Art/Textures/Tripo{SextonsFerry,ShroudedDead,CompanyHoseCart,CompanySandbags,CompanyCheckpointSign,CompanyBanner}/`.
- CODEMAP lines (Main writes): `Tools/Blender/looter_tripo_m23.py` (Mains 2-3's Tripo models fitted to the code's spots),
  `Tools/Blender/tripo_m23_sheet.py` (their check renders and sheet), `Art/Models/Tripo/WardensMains23.py` (their source).

## The models (UE cm, actor frame; tris = LOD0)

| Asset | Size L x W x H | Tris | Hulls | Sockets | Fit |
|---|---|---|---|---|---|
| SM_SextonsFerry | 932 along Y x 160 x 322 | 13,016 | 6 (5 hull slices to the gunwale, pole) | Lantern (-1,-430,112), Seat (0,-333,6), BowEye (0,456,27), SternEye (0,-403,30) | bow +Y, pivot mid-hull on the gunwale (keel ~-90). Tripo's 2.4:1 skiff stretched along its planks to 9 m, beam 1.6, depth 0.9; its lantern pole stands on the transom (-Y, beside Sexton), kept round (stern squeezed back); glass on LanternGlow |
| SM_ShroudedDead | 180 x 58 x 59 | 7,932 | 1 | - | along X, head +X, pivot bottom middle |
| SM_CompanyHoseCart | 255 x 135 x 182 | 12,934 | 1 (wheels, bed, tank) | Valve (46,-1,180), Tank (-2,0,104), Reel (-100,0,118) | four-wheeled (Tripo's), +X the valve end, pivot on the ground under its middle; scaled evenly so the hand-wheel's hub is 1.8 m up and 0.46 m forward (the stand-in's hold spot). Tank r 43, 195 long, top 147. No pull handles |
| SM_CompanySandbags | 198 along X x 88 x 88 | 5,798 | 1 | - | Tripo made 12 rows as tall as long: cut to the bottom 6 rows + its slumped top row (bisected at the seams) |
| SM_CompanyCheckpointSign | 300 along Y x 64 x 212 | 5,589 | 3 (board, 2 posts) | Words (4,0,145) | face +X, board 0.8-2.1 m up, face flattened onto X = +3 (ACompanySign draws its words at +4, centre 145); face on `PaintBlack` (PaintWorn tinted 0x2a2622, box-mapped: the same material Graves/Lootables/Chests define); post tops cut flush at 2.12 |
| SM_CompanyBanner | 1200 along Y x 3 x 149 | 5,397 | none (Collision None) | RopeLeft (0,-600,70), RopeRight (0,600,70) | centre pivot, pressed to 3 cm (words at +-2.6), 10 cm sag, top corners at (+-600, 70) where the code ties its ropes; Tripo's rope ties cut |

Maps: ferry, cart, banner 2048; shroud, sandbags, board 1024. Look: graded by `write_maps` (sat x0.86, contrast x1.06,
roughness floor 0.38); judged on `Sheet.png` beside the Tripo loudspeaker post and sacks: same matte worn palette, no clay,
nothing glossy. The banner is plain pale canvas (the words make it); the shroud is pale linen (a little whiter than the
flour sacks).

## Code edits Main must make (the models land on the existing paths; these fix where the code's own spots sit)

1. **`World/SextonsFerry.cpp` (required for the lantern):** the Tripo ferry carries its own lantern on a pole at the stern.
   In the `if (TripoFerry)` branch also hide `Lantern` (SM_PorchLantern) and move the light:
   `Lantern->SetVisibility(false); Lantern->SetHiddenInGame(true); LanternLight->SetRelativeLocation(FVector(-1.0, -430.0, 112.0));`
   (or read the model's `Lantern` socket). Chains: `BowEye (0, 456, 27)`, `SternEye (0, -403, 30)` (were +-440, 10).
   Seat: `SeatAt (0, -333, 6)` (was (0, -320, 8); 13 cm, optional). The bow is +Y as before.
2. **`World/CompanyHoseCart.cpp` (recommended):** `TankAt (-2, 0, 104)` (was (5, 0, 125)); `TankHit->InitCapsuleSize(48, 100)`
   (tank r 43, 195 long; was 55/85); `ReelAt (-100, 0, 118)` (the tank's back end, where the hose leaves). `ValveAt (40, 0, 180)`
   already matches (model 46, 180). The cart is 2.55 x 1.35 m (stand-in ~2.2 x 1.3): check it clears the burial deck's
   aisle and the Gilt Spur's door in PIE.
3. **`World/CompanySign.cpp` (required): the barrier.** `Board->SetStaticMesh(Model ? Model : Cube)` gives the checkpoint
   board's model to every ACompanySign, so `Main3_Barrier` (900 x 14 board) would show a 3 m signboard. In the look update,
   use the model only for a signboard-sized board, else the cube:
   `const bool bWantModel = Model && BoardSize.X < 500.0 && BoardSize.Y > 60.0;` then
   `Board->SetStaticMesh(bWantModel ? Model : Cube); if (!bWantModel) Board->SetMaterial(0, Paint);` (keep the two
   meshes/material as members or statics), and `bModel` from that. With the model the posts are its own (code hides
   PostLeft/Right already).
4. **`Tools/Unreal/build_area_main3.py` (required): sandbags.** The Tripo wall is 2 m long; `SANDBAGS_PER_WALL = 2` at
   `SANDBAG_STEP = 110` overlaps it 0.9 m. In `place_checkpoint`, when the first path loaded:
   `per_wall = 1 if sacks.get_path_name().startswith(SANDBAG_MODELS[0]) else SANDBAGS_PER_WALL` and loop
   `range(per_wall)` with `offset = (k - (per_wall - 1) * 0.5) * SANDBAG_STEP`. (Three 2 m walls, 1.3 m gaps between.)
5. Shrouds, banner: no change (place_shrouds scales by bounds to 175; the banner's corners match `ClothLength/Height`).

## Import steps (Main)

1. Editor open, no PIE: `Tools\console.ps1 "Looter.ImportModels C:/Dev/AI_Looter_Shooter/Intermediate/ArtExport_TripoM23"`.
   New: SM_SextonsFerry, SM_ShroudedDead, SM_CompanyHoseCart, SM_CompanySandbags, SM_CompanyCheckpointSign,
   SM_CompanyBanner in `/Game/Art/Tripo`; MI_Tripo<each> on M_World; MI_LanternGlow and MI_PaintBlack reused (same
   definitions as their other sources).
2. Edits 1-4, build (`Tools\launch.ps1 -Build`), rerun `build_area.py RansomsRest gameplay` (main2 shrouds, main3 sandbags).
3. Tests: `Looter.World.SextonsFerry`, `Looter.World.HoseCart`, `Looter.World.CompanySign`, `Looter.World.CompanyBanner`.
4. PIE: ferry (lantern lit on its pole, chains on the eyes, Sexton on the thwart), cart (valve hold spot on the wheel,
   tank burst hits), checkpoint (words on the black face, not sunk; barrier is a pole), banner (words on both faces, falls
   flat), shrouds on the biers. Perf: the deck view gains ~13k (cart) + 6 x 8k (shrouds) tris at LOD0 (LODs 50/25/12).

## Risks

- The ferry's pole is 1.9 m over the gunwale: the lantern hangs ~1.1 m up, lower than the stand-in's 1.35.
- Sign words: PaintWorn's worn patches show through the black (the game's own worn paint); letters 20 cm are on a flat
  face, but the frame stands ~1-2 cm proud at the edges.
- Banner texture stretched ~4x along its length (plain canvas: hardly shows); thin slab, two-sided by its two faces.
- The cart has four wheels (prompt asked two) and no pull handles; the stand-in's handles were never read by code.
