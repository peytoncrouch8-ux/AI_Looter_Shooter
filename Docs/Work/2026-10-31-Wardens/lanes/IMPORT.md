# Lane IMPORT: the Tripo models into the game (2026-10-31/11-01)

Nothing built, no Unreal opened, nothing committed. Blender ran (inspection, renders, export test: the static export is
done and sits in `Intermediate/ArtExport_Tripo`; its texture sets are in `Art/Textures/Tripo<Name>/`, new files to commit).

## Files

| File | What |
|---|---|
| `Tools/Blender/looter_tripo_wardens.py` (new) | `SPECS` (one entry per static model: turn, size, pivot, cuts, glass, sockets, boxes, master, map size), `build(name)`, `write_maps` (the style pass). |
| `Art/Models/Tripo/Wardens.py` (new) | Model source, category Tripo: 15 models -> `/Game/Art/Tripo/SM_<Name>`. `LOOTER_TRIPO_ONLY=A,B` limits a run. |
| `Art/Models/Props/TripoProps.py` (new) | Model source, category Props: `SM_DeliaShotgun`, `SM_AbelsHat` (where the game already looks). |
| `Tools/Blender/tripo_people.py` (new) | The 6 people onto the UE5 mannequin skeleton -> `Intermediate/TripoPeople/SK_<Name>.fbx` + `TripoPeople.json` (materials only). |
| `Tools/Unreal/import_tripo.py` (new) | Editor steps: `manny` (export SKM_Manny_Simple FBX), `statics`, `people` (onto SK_Mannequin, PA_Mannequin, mesh sockets copied, 4 LODs, no Nanite), texture caps. |
| `Saved/tripo_import_probe.py` (new, read-only) | Bounds, tris per LOD, Nanite, hulls, sockets, MI parent, texture sizes/caps; people: skeleton, physics, HandGrip_R. Writes `Saved/tripo_import_probe.txt`. |
| `Saved/tripo_style_board.py` (new) | Transient board: each Tripo model beside the game model it replaces, 18 m ahead of the viewport camera; `clear` removes it. |

## Run order (editor open, no PIE)

1. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/import_tripo.py manny"` (writes `Intermediate/TripoRig/SKM_Manny_Simple.fbx`).
2. Statics (already exported once; rerun only after a SPECS change): `Tools\models.ps1 -Only Wardens,TripoProps -Out Intermediate\ArtExport_Tripo -NoImport` (~6 min).
3. `Tools\artrun.ps1 -Script Tools\Blender\tripo_people.py` (all six; `-ScriptArgs Delia,Junie` for some).
4. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Tools/Unreal/import_tripo.py"` (statics via Looter.ImportModels, then people). Idempotent.
5. `Tools\console.ps1 "py C:/Dev/AI_Looter_Shooter/Saved/tripo_import_probe.py"`; style check: `py .../Saved/tripo_style_board.py`, `Tools\grab.ps1`, then `... clear`.
6. Then build (`Tools\launch.ps1 -Build`: FLIGHT/DREDGER/RENAMES constants changed), rerun `build_area_landfall.py RansomsRest`
   and `build_area_farm.py` (barge, hose, speakers, barricade, hat are placed by them), play-check, `Tools\perf.ps1` before/after.
Order matters: the C++ now names the Tripo paths in constructors (`FObjectFinder`): build after step 4, or the Lamplighter,
tender, Great Lantern and the Dredgers' bodies load empty (error in the log). CREW and the shotgun fall back by themselves.

## Swaps (what changed where)

- CREW: none needed. `SK_Delia/Holloway/Finch/Junie` land at `/Game/Art/Characters/` (CREW's paths), on SK_Mannequin, real
  heights (1.60/1.75/1.80/1.65 m; worn at ModelScale 0.85). `SM_DeliaShotgun` lands at `/Game/Art/Props/` (sockets Grip
  (-19.8, 0.4, 1.2), Foregrip (13.2, -0.5, 5.1), Muzzle (55, -0.8, 5.1) cm; 110 cm, barrel +X, barrel line levelled).
- DREDGER `Creatures/DredgerLook.h`: `BodyMeshPath` -> `/Game/Art/Characters/SK_DredgerRifleman`, `HoseManBodyMeshPath` ->
  `SK_DredgerHoseMan` (1.80 / 1.85 m). `DredgerTuning.h` `Kit` default 1 -> 0 (the Tripo masks; the lane's rule). Not
  swapped: the company rifle (a weapon definition rolled from AR parts, not a mesh path) and the nozzle (engine shapes);
  `SM_CompanyRifle` (sockets Muzzle/Grip/Foregrip) and `SM_HoseNozzle` (Muzzle) are imported for when DREDGER wires them.
- FLIGHT `World/LamplighterShipParts.cpp`: `HullMeshPath` -> `/Game/Art/Tripo/SM_Lamplighter`, `LanternMeshPath` ->
  `SM_GreatLantern` (socket Light in its lamp), `LanternScale` 4.2 -> 1 (new `LanternFlameScale` 4.2 keeps the flame),
  deck boxes, rails, spots (table below), new `SternDeck` box (`LamplighterShip.h` member). `LamplighterShip.h`: `HullScale`
  (1,1,1), `DeckTop` 56 -> 422, `RailOut` 315 -> 375. `LamplighterShip.cpp`: Aboard aft/fore -1260/1110 -> -1200/1300.
  `LamplighterShipBoarding.cpp`: `BiteHeight` 95 -> 140, the bag-tear sound at Z 700 -> 1425. `LamplighterShipLanding.cpp`:
  `TearLocal` (150,520,1425), `KeelPoints` Z (0,0,45), `AnchorTieLocal` Y 375. `CompanyTender.cpp`: `HullPath` ->
  `SM_CompanyTender`, `HullScale` 1, `DeckTop` 43 -> 298, `HingeOut` 155 -> 120, `BagHeight` 558 -> 388, launchers
  (330,125,330)/(-450,110,330), riflemen (260,0)/(-450,0) (fore and aft of its bag). Not swapped: grapple, winch, spyglass
  (built from engine shapes, no single path; `SM_GrappleHook` (socket Eye) and `SM_AnchorWinch` (1.2 m, socket Top) are
  imported; there is no Tripo spyglass).
- WORLD `build_area_landfall.py`: `SHIP` = length 3100, beam 1054, keel_below 0, top_over 2077, deck_over 422, yaw 0.
  `STAND_INS` barge_hull, hose_section, hose_intake, speaker_post -> `/Game/Art/Tripo/SM_*`; new `TRIPO_FOLDER` rule: a
  Tripo path is used whole at real size in its own materials, the stand-in's extra shapes left out (stacks, vents, drum,
  collars, horns, plate). Code: `is_tripo`, `socket_at`, `tripo_barge` (smog on sockets Stack_1/2, hose from HoseDrum),
  `hose_parts` (sections keep their proportions: 1.5 m across, ~5.4 m each; intake mouth down, hose into its Hose socket),
  `place_loudspeakers` (post turned to face, target point at socket Horns). `build_area_farm.py` `BARRICADE`: the table,
  boards and sacks x2 -> `SM_BarricadeTable/Boards/Sacks` (long axes along X like the stand-ins; same yaws).
  `story_texts.py` `READABLE_MODELS['AbelsHat']` -> `SM_AbelsHat` (STORY's one name; `/Game/Art/Props`).
- RENAMES `World/RoostKit.h`: `PostPath` -> `/Game/Art/Tripo/SM_LanternPost` (3.2 m, arm +X, glass on `LanternGlow`,
  sockets Light = Beacon (110.5, 1, 191), Wake (170,0,0), Speaker (0,0,330), Bell (-59.5, 1.5, 256: Tripo's own bell, on the
  far side of the post), Perch_1-6 on the post top, the arm both ways and the brace).

## Numbers measured (Blender, GLB units x scale; cm in the actor's frame)

Lamplighter: GLB 3.731 long -> scale 8.3084 (31.0 m bag nose to tail; hull 28 m; 10.5 m beam at the bag, 20.8 m tall).
Pivot under the middle at the keel. Main deck (z -0.742 GLB, flat within 1 cm from x -0.95 to 1.22) 422 over the pivot,
7.5 m wide (rails at |y| 0.47-0.52 GLB). Stern deck x -1.40..-0.95 rises to +47 cm (box at +32). Forecastle x 1.27..1.52,
+118. Poop deck x < -1.45, +2.0 m (not walkable: the stern rail stops it). The hull's own prow lantern cut off (577 faces);
its pedestal top (socket Lantern) at (1329, 0, 644) = 222 over the deck. Hull's own wheel at x -681, hub 2.84 m over the
deck (out of reach). Bag top at x ~250 is 1512 over the deck.

| Spot (her frame, Z over DeckTop) | Was | Now |
|---|---|---|
| Main deck box centre/extent | (-200,0)/(1000,300,10) | (132,0)/(923,360,10) |
| Stern deck box | - | (-976,0, top +32)/(187,340,21) |
| Prow deck box (forecastle, solid) | (920,0)/(130,150,10) | (1159,0, top +118)/(104,200,64) |
| Side spans | -1200..-200, -200..150, 150..800 | -1163..-250, -250..150, 150..1055 |
| Lantern | (950,0) post 120 | (1329,0) post 222 |
| Wheel | (-1050,0,110) | (-1050,0,142) |
| Engine (2 barrels) | (-720, +-55) | (450, +-55) |
| Captain / Keeper / Engineer | (-1120,0,0) / (830,105,0) / (-720,-165,0) | (-1120,0,32) / (1210,105,118) / (450,-165,0) |
| Landing / prow marker / Hob perch | (-450,0,0) / (880,0,90) / (200,0,924) | (300,0,0) / (990,0,90) / (250,0,1512) |

Tender: GLB 3.092 (along Y; bow -Y already) -> 3.8804: 12.0 x 4.43 x 4.66 m, pivot at the keel's middle; top walk 298 over
the pivot (its hull box's top), 2.4 m wide; bag 2.3-3.9 m up over X -369..+132 (riflemen stand clear of it).

| Model | Size (cm, L x W x H) | Tris | Scale | Notes |
|---|---|---|---|---|
| SM_Lamplighter | 3100 x 1054 x 2077 | 76,819 | 8.308 | 2 hulls (hull, bag), socket Lantern |
| SM_DredgeBarge | 7500 x 2475 x 4849 | 75,775 | 40.41 | turned 180 (bow was -X); Stack_1 (-1107, 21, 4809), Stack_2 (-2373, 19, 4809), HoseDrum (-889, 323, 0) |
| SM_CompanyTender | 1200 x 443 x 466 | 38,560 | 3.880 | 2 hulls (hull to its top walk, bag) |
| SM_GreatLantern | 59 x 59 x 150 | 18,669 | 2.5 | lamp on LanternGlow (251 faces), socket Light (-0.6, -6, 59) |
| SM_GrappleHook | 54 x 31 x 100 | 7,678 | 3.33 | no collision, socket Eye |
| SM_DredgeHose | 150 x 150 x 537 | 7,936 | 4.28 | stood on Z, centre pivot, no collision |
| SM_HoseIntake | 300 x 262 x 170 | 9,410 | 3.39 | mouth down, socket Hose (top) |
| SM_AnchorWinch | 77 x 120 x 120 | 11,076 | 2.4 | box, socket Top |
| SM_LanternPost | 253 x 214 x 320 | 11,320 | 2.13 | glass on LanternGlow (490 faces), foot + post hulls |
| SM_LoudspeakerPost | 119 x 211 x 400 | 11,628 | 2.22 | horns +X, pole hull, socket Horns (61, 17, 338) |
| SM_BarricadeTable / Boards / Sacks | 160 x 94 x 121 / 200 x 53 x 128 / 120 x 102 x 95 | 7.6k / 7.5k / 7.7k | | one box each |
| SM_DeliaShotgun / SM_CompanyRifle | 110 x 12 x 25 / 110 x 14 x 36 | 11.7k / 11.9k | | barrel line fitted level, no collision |
| SM_HoseNozzle / SM_AbelsHat | 45 x 15 x 47 / 45 x 39 x 23 | 9.5k / 5.7k | | hat: one box |
| SK_ people | 1.60-1.85 m | 33.8k-39.8k | | 86 bones (the mannequin's, root as the object) |

All: Nanite off, LODs 50/25/12% at screen 0.5/0.25/0.12, triangles as Tripo made them (no remesh). Maps: ships and people
2048, props 1024 (4K in the GLBs).

## Style (the user, 2026-10-31: Tripo models match the game's look)

- Never Tripo's raw materials: every slot is `MI_Tripo<Name>` on the project's textured masters, made by Looter.ImportModels
  from the texture set, like the Tripo chests, AR parts and melee tools: `M_World` (props, ships), `M_Gun` (shotgun, rifle,
  nozzle: its worn edges and grime from custom primitive data, as the Ranchhand stand-in wore), `M_Character` (people).
  The lanterns' glass/lamp is the shared `MI_LanternGlow` (same name and look as LanternPost.py: no new instance).
- Colour map graded the chests' way (ChestTiers.write_maps): unsharp mask (sigma 1.2 px per 1K), saturation x0.84-0.92 and
  contrast x1.04-1.08 by kind (`GRADE`); roughness floored (`ROUGH_FLOOR`: props 0.38, ships 0.4, guns 0.28, people 0.35;
  metal keeps its own) since Tripo's glossy patches read wet beside the matte painted world; Tripo's own occlusion kept
  (ORM red, eased to 0.15 min); normal maps turned to DirectX.
- Left to tune in the MIs after the board: `Tint` (all masters), `RoughnessOffset` (M_World, M_Character),
  `RoughnessScale`/`NormalStrength`/`DiffuseAO` (M_Character). No saturation parameter exists on the masters: change
  `GRADE` and rerun step 2/3 if a model reads too saturated.
- Check: `Saved/tripo_style_board.py` (Tripo left of each pair: winch/barrel, lantern posts, sacks, boards/lumber,
  table/bench, hats, shotgun/Ranchhand body, Great Lantern/Keeper's lantern, Delia/Manny), grab at noon and dusk.

## Risks (unchecked: no editor)

1. People on the mannequin: each bone keeps the mannequin's own axes turned the shortest way onto Tripo's limb, so the
   mannequin's animations should pose them right; checked only in Blender against a stand-in UE5 skeleton (hierarchy,
   weights, export; the real mannequin FBX comes in step 1). Look at MM_Idle on SK_Delia in the mesh editor first.
2. If SK_Mannequin's bone translation retargeting is Animation (the default), the people take Manny's bone lengths (Delia
   1.6 m would stretch toward 1.8 m). Fix if seen: SK_Mannequin, Show Retargeting Options, root Animation, pelvis
   AnimationScaled, the rest Skeleton (Manny's own anims unchanged; hero bodies retarget anyway).
3. HandGrip_R: copied from SKM_Manny_Simple if it is a mesh socket; `add_socket` from Python unverified (warning says if not).
4. Python names unverified in this UE (5.8): `regenerate_lod`, `lod_info`, `SkeletalMeshExporterFBX`, `add_socket`; each logs
   a warning and the rest goes on. LOD screen sizes on the people may need setting by hand.
5. Lamplighter: the hull mesh has no collision in FLIGHT's code (as before): the deck clutter (console, struts, the big
   wheel's pedestal) is walk-through; the UCX boxes exist if FLIGHT turns pawn blocking on. The code's own wheel (1 m) and
   engine barrels stand beside the hull's modelled wheel and console. Finch on the forecastle is ~1.2 m from the lantern:
   tune `HandsShift`/his spot. Forecastle and poop deck aren't walkable (solid block / stern rail).
6. Hose-man: the stand-in tank barrel (`TankMeshPath`, the hit zone) still shows over the Tripo back tank: hide its mesh
   or point it at nothing visible (DREDGER's call). The Dredgers' FObjectFinder bodies need step 4 before the build.
7. Barricade table: Tripo made it tipped at an angle; its yaw (90) may need a turn so its top faces the yard.
8. Perf: Lamplighter 77k tris near the player for the whole flight, people 34-40k each (6 Dredgers on deck ~220k skinned
   at LOD0), barge 76k far off (LODs). Measure (`Tools\perf.ps1`, the deck fight); if heavy, lower the people's LOD0 in
   tripo_people.py (decimate before export) or the Dredgers' LOD screen sizes.
9. Barge is 48.5 m tall with its stacks (Tripo's proportions at 75 m long): its keel stays 45 m over the rim; check it in
   the reveal shot.
