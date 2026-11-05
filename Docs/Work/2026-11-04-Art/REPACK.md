# REPACK lane (2026-11-04): Tripo atlases repacked, maps re-cut from the 4096 deliveries

Source: `Docs/Work/2026-11-03-ArtAudit/AUDIT.md` findings 2, 4, 7 and plan row 5; CONTRACT.md "Round 4" (REPACK); Main's
calls in the lane: buildings and Reliquary first, then the 1K props by loss, inside their map sizes (the user's caps: 2K
people and ships, 1K props); the Lamplighter, the Unpaid and the tender skipped; the barge only if worth it; beat Tripo's
own packing on every model or leave it. Not imported, not committed.

## The result, bluntly
- **Repacked (5):** the Tripo farmhouse, cottage and barn, the Company checkpoint sign and the Company sandbags. Their big
  surfaces gain 1.31-1.44x the texels per cm and no surface loses any.
- **Visible gain:** clear only on the **sandbags** (the burlap weave at 1 m). Slight on the sign. The three buildings
  barely change by eye, although their texels rose 1.3-1.4x: their 4096 sources hold little more than the old 2K maps
  already showed (shrinking to 2K lost 5.9-8.1% of the colour variance, the farmhouse the most).
- **Kept as they were (8):** the Reliquary, BarricadeBoards, DredgeHose, AbelsHat, BarricadeSacks, BarricadeTable,
  HoseIntake and AnchorWinch. Our packer can't beat Tripo's packing on them without making some surface softer. Without
  the density floor they would have reached +4-12% UV area, but about a quarter of their surface would have lost up to
  ~10% of its texels. Their maps and exports are byte-identical to before (no repack flag).
- **Why so little:** the audit read 28-60% "atlas use" as unused square. Most of it isn't: Tripo's atlases are packed by
  a raster packer as good as ours (xatlas-like: irregular islands with dilated padding). There is space to win only where
  the game mesh dropped faces from the atlas (the farmhouse's roof moved onto the trim sheet and its cuts, the sandbags
  cut to waist height, the sign's board moved onto PaintBlack). Evening out the islands' uneven density adds a little.
- **Skipped (Main):** the Lamplighter, the Unpaid and the tender. Their sources are soft (shrinking 4096 -> 2048 loses
  0.8%, 0.2% and 1.0% of the colour variance), so no re-cut can sharpen them. They are candidates for a rebake in our
  sets once the REBAKE lane's method is proven on the barge.
- **The barge:** not repacked (its main atlas loses 1.8% at 2K). Its house and casing walls get a texture set of their
  own (`own_set`, see "The barge's walls" below) once REWORK's round-5 mesh lands.

## Table (the game meshes as built, before -> after)
Use: the summed UV area of the model's Tripo faces (the audit's metric; the rest of the square is gaps, padding and
faces on other slots). Texels/cm: map px x sqrt(UV area / surface cm^2), whole model and the area-weighted median over the
biggest islands making up half the surface ("big"). Density p10/p50: each face's new density over its old, area-weighted.
Mip bleed: the share of island texels whose 8 x 8 (mip 3) or 16 x 16 (mip 4) block also holds another island's padding
or the free fill, each island owning PAD px. Sharpness: the mean squared Laplacian of luma on the model in Blender renders
(1920 x 1080, 90 degrees, the same camera at 1, 3 and 10 m from its biggest wall), after over before. Lost: the share of
the GLB's colour variance a map at the game size can't hold (the survey below).

| Model | Map | Use | Texels/cm | Big texels/cm | Density p10/p50 | Mip-3 bleed | Mip-4 bleed | Sharpness 1/3/10 m | Lost at map size |
|---|---|---|---|---|---|---|---|---|---|
| SM_Farmhouse_Ransom | 2048 | 0.313 -> 0.503 | 0.619 -> 0.784 (x1.27) | 0.585 -> 0.770 (x1.32) | 1.04 / 1.26 | 7.8 -> 5.9% | 38.2 -> 31.8% | 1.02 / 1.05 / 1.10 | 8.1% |
| SM_Cottage_Ransom | 2048 | 0.369 -> 0.521 | 0.765 -> 0.909 (x1.19) | 0.605 -> 0.864 (x1.43) | 1.00 / 1.21 | 6.5 -> 5.5% | 35.9 -> 30.2% | 1.04 / 1.03 / 1.08 | 6.0% |
| SM_Barn_Ransom | 2048 | 0.389 -> 0.542 | 0.540 -> 0.638 (x1.18) | 0.472 -> 0.617 (x1.31) | 1.00 / 1.17 | 5.8 -> 5.0% | 33.9 -> 29.5% | 0.97 / 1.02 / 1.02 | 5.9% |
| SM_CompanyCheckpointSign | 1024 | 0.465 -> 0.637 | 2.09 -> 2.45 (x1.17) | 1.69 -> 2.24 (x1.33) | 1.00 / 1.21 | 13.3 -> 12.0% | 35.3 -> 33.7% | 1.80 / 1.31 / 0.89 | 29%* |
| SM_CompanySandbags | 1024 | 0.289 -> 0.560 | 1.76 -> 2.45 (x1.39) | 1.69 -> 2.43 (x1.44) | 1.19 / 1.45 | 32.6 -> 24.3% | 74.6 -> 61.1% | 1.50 / 1.50 / 0.90 | 9.8% |
| SM_ReliquaryWhole | 2048 | 0.590 kept | 7.03 | 6.76 | | 1.8% | 12.1% | | 11.4% |
| SM_BarricadeBoards | 1024 | 0.466 kept | 2.26 | 1.97 | | 27.2% | 67.1% | | 15.8% |
| SM_DredgeHose | 1024 | 0.607 kept | 1.49 | 1.37 | | 7.7% | 22.4% | | 13.0% |
| SM_AbelsHat | 1024 | 0.580 kept | 11.2 | 10.4 | | 13.1% | 36.6% | | 10.9% |
| SM_BarricadeSacks | 1024 | 0.588 kept | 3.40 | 3.08 | | 13.4% | 37.8% | | 10.4% |
| SM_BarricadeTable | 1024 | 0.564 kept | 3.29 | 3.02 | | 15.9% | 42.8% | | 9.5% |
| SM_HoseIntake | 1024 | 0.615 kept | 1.73 | 1.60 | | 10.4% | 29.3% | | 7.7% |
| SM_AnchorWinch | 1024 | 0.471 kept | 3.18 | 2.99 | | 28.9% | 69.3% | | 5.7% |

\* The sign's 29% is the whole GLB atlas, its painted board included; the game paints that face PaintBlack, so what is
left on Tripo's paint (posts, frame) loses less.
- Sharpness at 10 m drops 10% on the two 1K props: the 10 m view is minified, and a denser map aliases less in the render
  (Eevee's mips). It is not lost detail.
- Build time with the repack: the pack is cached (Intermediate/TripoRepack); the re-cut runs at every export, +35-180 s
  a model (farmhouse 54 -> 165 s, cottage 63 -> 245 s, barn 52 -> 173 s, sign 18 -> 84 s, sandbags 12 -> 54 s, first runs
  with the pack; ~1 min less once cached).

## Suggestions for the user (the caps are the user's)
Props that still lose 10% or more of their source's colour detail at 1K (what a 2K map would still lose): BarricadeBoards
15.8% (6.5%), DredgeHose 13.0% (5.7%), AbelsHat 10.9% (5.4%), BarricadeSacks 10.4% (7.8%), the sign's posts and frame
(the GLB 29% -> 12% at 2K, overstated as above); at 2K already, the Reliquary 11.4% (only its 4096 itself would keep
it all). Raising those caps is the only way left to sharpen them from Tripo's paint; otherwise a rebake in our sets
(REBAKE's method) or the detail layer.

## Survey: every Tripo GLB as delivered (`tripo_repack_check.py survey`)
All 64 deliveries carry 4096 maps (BC JPEG; RM or ORM; NormalGL). Use = UV area; islands; Tripo's padding = mean luma 1-2
px outside the islands over the islands' own (0.86-1.16: Tripo dilates its islands itself); lost = share of the colour
variance lost at 2048 / 1024 / 512 px.

| GLB | Use | Islands | Lost 2048 / 1024 / 512 |
|---|---|---|---|
| FrontAssay (not in the game) | 0.361 | 1221 | 6.2 / 13.0 / 20.3% |
| CottageRansom | 0.443 | 646 | 6.0 / 13.0 / 20.8% |
| DredgeBarge | 0.456 | 1132 | 1.8 / 4.9 / 9.9% |
| FarmhouseRansom | 0.457 | 723 | 8.1 / 17.9 / 29.8% |
| BarricadeBoards | 0.466 | 445 | 6.5 / 15.8 / 27.9% |
| AnchorWinch | 0.471 | 589 | 2.3 / 5.7 / 10.9% |
| CompanySandbags | 0.472 | 727 | 5.9 / 9.8 / 14.4% |
| BarnRansom | 0.476 | 587 | 5.9 / 12.4 / 19.9% |
| CompanyHoseCart | 0.484 | 581 | 1.4 / 3.6 / 6.5% |
| WindmillRR | 0.492 | 591 | 1.3 / 3.5 / 6.6% |
| Lamplighter | 0.501 | 782 | 0.8 / 2.3 / 4.9% |
| SextonsFerry | 0.534 | 374 | 3.3 / 8.6 / 16.1% |
| Spitter | 0.542 | 647 | 0.2 / 0.8 / 2.1% |
| LanternPost | 0.557 | 201 | 1.4 / 4.4 / 9.8% |
| BarricadeTable | 0.564 | 208 | 3.8 / 9.5 / 16.9% |
| GreatLantern | 0.568 | 217 | 1.4 / 3.9 / 8.3% |
| CompanyTender | 0.569 | 305 | 1.0 / 3.1 / 7.6% |
| Unpaid | 0.570 | 1475 | 0.2 / 0.8 / 1.9% |
| Loot+chests | 0.575 | 1907 | 2.2 / 7.4 / 16.0% |
| ShroudedDead | 0.579 | 169 | 0.9 / 2.5 / 5.0% |
| AbelsHat | 0.580 | 145 | 5.4 / 10.9 / 17.5% |
| CompanyRifle | 0.586 | 111 | 1.4 / 4.6 / 10.7% |
| BarricadeSacks | 0.588 | 154 | 7.8 / 10.4 / 12.5% |
| CompanyBanner | 0.590 | 94 | 0.7 / 1.9 / 3.9% |
| ReliquaryWhole | 0.590 | 87 | 11.4 / 25.3 / 39.3% |
| Gravemother | 0.591 | 1019 | 2.2 / 8.0 / 17.9% |
| CompanyCheckpointSign | 0.599 | 133 | 12.2 / 29.0 / 47.3% |
| DeadMiner, Delia, Abel, DredgeHose, DeliaShotgun, Thaddeus, DredgerHoseMan, GrappleHook, HoseIntake, Hiram, MeadowWolf, Finch, SoulCoal, LoudspeakerPost, HoseNozzle, StoneSentinel, Holloway, Amos, Foreman, DredgerRifleman, Junie | 0.60-0.66 | 50-641 | 0.2-5.7 / 0.4-13.0 / 0.9-19.5% |
The chapel, depot, smithy, watchtower, workshop, Whitlock barn and the three fronts are not swapped in the game (their
scripted models are). The people (2K) all lose 0.2-2.5% at 2K except Holloway 2.5% and Delia's grey paint 3.3% (Delia's
own: 0.2%): nothing to gain.

## Method
- `Tools/Blender/looter_tripo_repack.py` (the step, the measures, the hooks' entries), `looter_tripo_repack_pack.py`
  (mesh arrays, rasterizer, UV islands, the raster packer), `looter_tripo_repack_bake.py` (the re-cut, padding, mip bleed,
  images), `tripo_repack_check.py` (survey, packers, before/after renders). The docstrings carry every rule and number.
- Islands are Tripo's own (polygons joined across edges whose corners carry the same UVs on both sides), never re-cut.
- The packer (xatlas-like, deterministic, numpy): each island turned to its smallest bounding box, scaled to one texel
  density by its 3D area but never below its old density (the floor), laid largest first at the first free place (top
  row, then left) among 0/90 degrees (small islands) or 0/90/180/270, on a 512-cell grid (4 px cells at 2K, 2 at 1K). The
  test is the island's row runs against the grid's row prefix sums, pruned by each row's longest free run (a summed-area
  box test for small boxy islands, an FFT correlation for the biggest). The scale: MIN_GAIN (1.05) times Tripo's area
  first (no fit: Tripo's layout stays), then bisection up to 1.6 x Tripo's scale. 723 islands: 8 s a try, ~50 s a pack.
- Gap: PAD 4 px a side at 2K, 2 at 1K (an 8 / 4 px gap, more with the cells' rounding). Checked on Abel's hat: Tripo 0.58
  use, ours 0.632 at 2 px, 0.665 at 1 px, both with no more mip bleed than Tripo's at any level; at 4 px ours was 0.558
  (Tripo's gaps at game size are as tight as ours).
- Re-cut: exact UV-to-UV (same mesh, barycentrics), the 4096 source box-filtered over each texel's footprint (k x k point
  taps, k = ceil(2 x source px per texel), up to 8). On Tripo's own layout it reproduces the old 2:1 / 4:1 shrink: RMS
  0.49 / 0.57 of 255, detail (Laplacian energy) 1.03 / 1.09x. Normals: decoded in the old UVs' MikkTSpace frame, encoded in
  the new one's (frames on a scratch mesh of the loop triangles: they match Blender's calc_tangents within 1e-6, and the
  farmhouse shell's n-gons work).
- Padding: each island's own texels spread PAD px out, the rest push-pull filled. Seams: none visible in the previews at
  1-10 m (colour, ORM and normals continuous over the turned islands).
- Rejected on the way: Blender's own packer (average islands scale + concave pack) gave 0.35 (8 px gap) and 0.47 (4 px) on
  the hat where Tripo has 0.58. A 2 x 2 average of bilinear taps came out softer at 1 m than Tripo's layout (the gain in
  texels lost to the filter). Evening the density out with no floor left 27.6% of the farmhouse's surface less dense
  than before (p10 0.91, the gable visibly softer). A 1024-cell grid at 2K packs 5% more area at 3.5x the time.

## Checks
- Export test: `$env:LOOTER_TRIPO_ONLY='CompanyCheckpointSign,CompanySandbags'; Tools\artrun.ps1 -Export
  Farmhouse,Cottage,Barn,WardensMains23 -Family Repack` -> `Intermediate\ArtExport_Repack`: SM_Farmhouse(_Ransom),
  SM_Cottage(_Ransom), SM_Barn(_Ransom), SM_PorchPlate(_Biscuits, _Jars), SM_ScreenDoor, SM_CompanyCheckpointSign,
  SM_CompanySandbags and the four manifests. The maps are written to Art/Textures/Tripo<Name>/ (BC, ORM, N) as before.
- Tangents (`tangentcheck.py`): every model clean except SM_Farmhouse_Ransom: 352 zero-tangent corners, all from its
  167 zero-area slivers. They predate the repack: the COLLISION lane's export had 361 from the same slivers. Faces without
  UV area: 11 -> 15.
- Repeatability: a second export (`-Family Repack_Repeat`) wrote the 15 maps byte-identical. The FBX files have the same
  sizes, and their geometry, UVs and slots are identical (only FBX ids and time stamps differ). After the module's split
  into `_bake.py` (no change meant), the sign's and sandbags' maps came out byte-identical again (`ArtExport_Repack_Split`).
- Previews: `Saved/ArtPreviews/Repack/<Model>_sheet.png` (1:1 crops of the 1080p renders, rows 1, 3, 10 m, Tripo's
  layout left, repacked right), the full renders `<Model>_<d>m_before|after.png`, all numbers in `results.json`.
- Run it: `Tools\artrun.ps1 -Script Tools\Blender\tripo_repack_check.py -ScriptArgs model,building:Farmhouse,m23:CompanySandbags`
  (kinds wardens, m23, m47, building); `survey`; `packers,<Name>`. Before/after = the same build with
  `LOOTER_TRIPO_REPACK=0`, then on.
- Shared writers: `lt.write_png` and `looter_tripo._save_colour` now retry on Windows' transient refusals (OSError 22/13:
  a PNG rewritten a second after it was written is still mapped by another process, likely the virus scanner). It hit 5 of
  13 builds before the retry; with it, the export retried twice and passed.

## The barge's walls (CONTRACT "Round 4, the barge's walls crisp", step 2)
The house's and casing's walls (the faces `looter_tripo_barge_paint.walls` picks; REWORK's `slots()` puts the same faces on
`TripoDredgeBarge_Walls`) get a texture set of their own, `TripoDredgeBargeWalls`, 2K.
- How (`looter_tripo_repack.own_set`, the build's `sets` spec key): before the main repack and the paint, those faces'
  islands are packed alone into a square of their own (no floor: they all gain; `OWN_PAD` 2 px a side at 2K, the fill
  searched from 60% of the square down), their maps cut from the GLB's 4096 maps at 4096 (REBAKE's paint works at 4096;
  `write_maps` shrinks the set to 2K), handed to the paint as `maps['Walls']`, written by `write_maps(maps['Walls'],
  'DredgeBargeWalls', ...)` (REBAKE's AlphaMask -> RGBA kept), and after `slots()` the slot's material is rebuilt on the
  set. The build stops if the slot's faces and the set's differ (one rule, checked).
- Switched on 2026-11-05 00:58 (Main's GO on REWORK's round-5 mesh): `SPECS['DredgeBarge']['sets'] = {'Walls':
  dict(faces=barge_paint.walls, maps=2048, slot='TripoDredgeBarge_Walls')}`.
- **Round 5, the export** (`$env:LOOTER_TRIPO_ONLY='DredgeBarge'; Tools\artrun.ps1 -Export Wardens -Family BargeWalls` ->
  `Intermediate\ArtExport_BargeWalls`: SM_DredgeBarge.fbx + Art_Models_Tripo_Wardens.json, 11 min; the manifest's
  TripoDredgeBarge_Walls on Art/Textures/TripoDredgeBargeWalls (BC 2048 RGBA with REBAKE's chip alpha, ORM, N), the three
  other slots on TripoDredgeBarge): 19,832 wall faces, 883 islands, 2,507 m2; fill 0.671 of their square; 15.4 -> 33.5
  px/m (big 13.6 -> 33.4, spread 0.57 -> 0.19); mip bleed at 4096 0 / 0 / 1.8 / 12.4% (mips 1-4). REBAKE's paint on the
  set: 11.25 M texels (2.38 M in the main atlas before), its footprint 30.6 px/m (14.0 before), chip weight 62.8%.
- Tangents: 36 zero-tangent corners, all from the mesh's 27 zero-area faces (REWORK's own export: 46); no face
  without UV area.
- The mesh: 148,769 triangles against 148,830 in REWORK's `ArtExport_TripoR5` (00:49:54). Its build seems to have started
  before `looter_tripo_barge.py`'s 00:43:01 change; ours used that code (flagged to Main).
- Previews (`tripo_repack_check.py -ScriptArgs barge,<scratch>`: REBAKE's cameras; before = the walls painted in the main
  atlas, after = their own set; `_paintdetail` = with MI_TripoDredgeBarge_Walls' proposed detail, PaintWorn 0.65 at
  1.6 m, emulated): `Saved/ArtPreviews/Repack/BargeWalls/house_1m_*`, `house_3m_*`, `casing_3m_*`, `house_walk_*`,
  `casing_walk_*`, `house_front_*`, `ship_*`. What they show, bluntly: the zones' borders are crisp curves where they were
  7 cm smears, and the rust shows its own mottling. But at 1 m the rust also shows Tripo's soft light/dark blotches,
  which the 14 px/m atlas used to average away (REBAKE's VALUE_KEEP term). With the proposed detail, the chip alpha
  keeps the PaintWorn chips off the rust as designed, so the rust zones read softer than the old chips-everywhere look.
  That is a look call for REBAKE and MATERIAL (a grain-only detail on rust, VALUE_KEEP lower at this density).
- Repeatability: a second build (the preview run's after state, maps into a scratch folder) wrote the three main-atlas
  maps byte-identical to the export's; the three walls-set maps differed. The difference is 0.13% of the colour's texels (5,452,
  2,038 by more than 2/255, the chip alpha among them), 0.04% of ORM's (metal up to 203/255) and N's (up to 25/255), in a
  band at the square's top (rows 0-240: a few islands). The own set itself repeats exactly: own_set run twice on one
  rework gave 0 differing texels in all three 4096 maps. So the variation enters in REBAKE's paint step on the set
  (`looter_tripo_barge_paint` / `looter_tripo_zones`; its Cycles bakes are the likely source). Flagged to Main for
  REBAKE.
- Also for REBAKE (seen in `casing_3m_*_paintdetail.png`): with the chip mask, the casing's red panel shows no chips. If
  that panel is PaintOxide (a paint zone), its chip weight should not be 0.
- Measured on the round-4 mesh (a dry run in memory; no barge map written): 19,936 faces, 2,508 m2, 881 Tripo islands;
  Tripo's atlas gives them 15.4 px/m (big surfaces 13.6).

| Grid | Pad (2K) | Fill of their square | px/m | Mip-3 / mip-4 bleed | Pack |
|---|---|---|---|---|---|
| 512 | 4 px | 0.600 | 31.7 | - (measured another way) | ~1 min |
| 512 | 2 px (Main's pick) | 0.668 | 33.4 | 3.9 / 22.5% | 73 s |
| 1024 | 2 px | 0.695 | 34.1 | 4.3 / 22.4% | 290 s |
| 1024 | 3 px | 0.650 | 33.0 | 3.7 / 22.5% | 336 s |
| 1024 | 4 px | 0.615 | 32.1 | 3.9 / 22.7% | 304 s |
The target of 35 px/m needs a 73% fill. Tripo's irregular wall islands, each REWORK copy of a slab on its own space
(REBAKE's paint is tri-planar in model space, so copies can't share texels), top out near 70% with mip-safe gaps. Main:
2 px on the 512 grid (2.2x Tripo's), no 1024 grid, no 4K set (the user's 2K cap for ships).
- Tested end to end on the round-4 mesh (the build with `sets` switched on in memory, the barge's maps redirected to a
  scratch folder): 19,838 wall faces packed alone, fill 0.671, 15.4 -> 33.5 px/m (big 13.6 -> 33.4), mip bleed at 4096 0 /
  1.8 / 12.4% (mips 2-4); REBAKE's paint wrote into the set (chip weight on 62.9%); slots: Hull, Deck, Super on
  TripoDredgeBarge and Walls on TripoDredgeBargeWalls, the face check passed; T_TripoDredgeBargeWalls_BC 2048 RGBA (the
  chip alpha), _ORM, _N. The whole barge build took 890 s (own set 165 s with the pack cached, paint 315 s).
- **Note for REWORK** (a cheap check, `tripo_repack_check`-style script in this lane's scratchpad: 32 rays over each wall
  face's outer hemisphere, an escape = no hit within 300 m): on the round-4 mesh 617 wall faces (51 m2, 2.0% of 2,512 m2)
  see no sky at all, 1,346 (112 m2, 4.5%) at most 2 of 32 rays, 2,150 (193 m2, 7.7%) at most 4. The enclosed ones lie
  23.2-30.1 m up, x -31.9..+11.8 m (house and casing), 184 facing down, 391 sideways, 42 up: inner faces between the house's
  and casing's shells. Dropping them from the rule would raise the walls' px/m by only 1%, so it isn't worth doing for the
  texels. The faces and their escape counts (that mesh only): `Saved/ArtPreviews/Repack/barge_walls_escapes.npy`.
