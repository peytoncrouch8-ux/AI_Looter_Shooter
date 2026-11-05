# LODS lane (2026-11-04 night): the cliff faces' fallback, the far trees, the shrubs' shadows

Brief (Main): the four tour views still over 8.3 ms on Medium (Aerial 8.71, Aerial_Dusk 8.72, Farm 8.61, PointOverValley
8.40); PERF.md "The content list for a LODS lane". Scripts only: no generator run, no import, no Source, no commit. Added
mid-lane (the user's pick from PERF.md's Farm options): shrubs without shadows on Medium only; the grass thinning and the
nearer grass end were declined.

## 1. Cliff faces `SM_CliffFace_A..D`, `_A2..D2`: Nanite fallback 25% -> 12% (done in the script, exported, not imported)

- Free of the 100% rule: their simple collision is their own UCX hulls (6-11 a face), unchanged. Complex traces use the
  fallback: bullets (`BulletSubsystem.cpp` 248, `bTraceComplex` true) and the spiders' cling/climb/lurk traces. On Medium the
  fallback is what is drawn too, so marks and feet sit on the rock seen; on High/Epic (the Nanite source) they can sit off it
  by the deviation below (p95 3.3-4.7 cm, worst 15.5 cm, was 1.8-2.5 / 10).
- Change: `Art/Models/Rocks/Cliffs.py` `FALLBACK = 12` (the faces; was 25), new `PANEL_FALLBACK = 25` for the panels and the
  seam (the Sink's pit wall only, not in the LODS list, unchanged); `build()` writes `obj['Fallback']` by kind. The manifest's
  `fallbackPercent` carries it; `ModelImporter::ApplyMeshSettings` sets `FallbackPercentTriangles` (percent only: a relative
  error target would need the importer changed, not done). The meshes are the same: the 8 FBXes match the last imports'
  geometry (A2 checked vertex by vertex in Blender: 9,999 triangles, 5,019 vertices, 11 hulls, same bounds; the rest match
  to the byte bar FBX ids and times).
- Triangles (Unreal's fallback count shows in the import log, "Fallback [...], num tris"): source 10,000 each (D, A2 9,999);
  fallback 2,500 -> 1,200 per face. In the views (PERF.md's hidden-family drops, all passes, x 0.48): Aerial 0.77 M -> ~0.37 M,
  PointOverValley 0.96 -> ~0.46, Farm 0.49 -> ~0.24, the deck 0.12 -> ~0.06.
- Expected (PERF.md's list): Aerial/PointOverValley -0.15..-0.2 ms, Farm -0.1 (the faces cost 0.42 / 0.24-0.3 / 0.2 now; not
  all of it is triangles).
- Checked in the Medium look (`Tools/Blender/cliff_fallback_sheet.py`, Saved/ArtPreviews/Lods/): Decimate standing in for
  Nanite's cut, UVs projected again (Decimate alone smeared the texture in streaks across UV seams, which Nanite keeps), Eevee
  with sun shadows, the 1080p frame at the tour's 80 degrees, from 20 m (eye 1.7 m), 60 m (12 m up) and 150 m (50 m up), 25
  degrees off the front. Sheet: `CliffFallback_sheet.jpg` (A, B, D: the three that change most at 20 m; 25% left, 12% right).
  Reading: at 60 and 150 m the two can't be told apart; at 20 m the silhouette, ledges, joints and the stepped top hold, the
  12% is a little softer in the small relief. Kept 12% on all eight (15% was measured too: in between, see report.json).

| Piece | Source | 25% | 12% | Off the source, 25% / 12% (cm: mean, p95, max) | Pixels changed from the source at 20 / 60 / 150 m (%, 25% -> 12%) |
|---|---|---|---|---|---|
| CliffFace_A | 10000 | 2500 | 1200 | 0.97, 2.52, 7.89 / 1.86, 4.69, 15.51 | 7.15 -> 14.67 / 4.46 -> 10.86 / 1.88 -> 6.31 |
| CliffFace_B | 10000 | 2500 | 1200 | 0.77, 2.04, 5.01 / 1.45, 3.81, 9.87 | 5.95 -> 14.26 / 3.71 -> 10.55 / 1.24 -> 5.9 |
| CliffFace_C | 10000 | 2500 | 1200 | 0.89, 2.25, 6.91 / 1.74, 4.45, 11.45 | 7.7 -> 11.65 / 4.66 -> 7.86 / 1.78 -> 3.59 |
| CliffFace_D | 9999 | 2499 | 1199 | 0.81, 2.22, 6.06 / 1.61, 4.33, 13.11 | 6.39 -> 13.5 / 3.95 -> 9.4 / 1.59 -> 5.03 |
| CliffFace_A2 | 9999 | 2499 | 1199 | 0.95, 2.49, 7.03 / 1.74, 4.57, 12.41 | 5.79 -> 12.93 / 3.72 -> 9.81 / 1.67 -> 5.58 |
| CliffFace_B2 | 10000 | 2500 | 1200 | 0.61, 1.75, 6.2 / 1.24, 3.3, 11.88 | 5.05 -> 10.8 / 2.98 -> 7.55 / 0.89 -> 3.79 |
| CliffFace_C2 | 10000 | 2500 | 1200 | 0.83, 2.24, 9.99 / 1.66, 4.28, 12.38 | 5.27 -> 10.7 / 3.3 -> 8.32 / 1.24 -> 4.36 |
| CliffFace_D2 | 10000 | 2500 | 1200 | 0.66, 1.85, 6.5 / 1.39, 3.94, 11.61 | 5.23 -> 11.53 / 3.74 -> 8.87 / 1.37 -> 4.6 |

  Pixels changed: share of the crop (rock, sky and ground round it) moving more than 0.08 in a channel; the source is what
  High and Epic draw. Crops at 1080p: 455-853 px wide at 20 m, 172-303 at 60 m, 79-130 at 150 m.
- Every map with the kit changes (Ransom's Rest, the tutorial island's plateau, Skyreach): the near look at 20 m is the one to
  look at in play.

## 2. Far trees `FarTrees_SM_FarPine_A/B`, `SM_FarBroadleaf`: no cull made (a 1.5 km cull culls nothing)

- Where they stand (`layout_computed.json` farTrees, 2,603 instances: FarPine_A 925, FarPine_B 601, FarBroadleaf 1,077):
  x and y within +-547 m of the area's middle. From the tour views the farthest tree is 767-930 m away (Aerial 883,
  PointOverValley 874, RidgeFoot 844, Farm 820); from anywhere inside the boundary at most 1.04 km (its 86 corners).
  **No tree stands past 1.5 km of any view**: an `InstanceEndCullDistance` there removes nothing and saves nothing, on any preset.
- Per-preset culling: the engine scales every per-instance cull distance by `r.ViewDistanceScale` (ISM/HISM proxies and the
  GPU scene's `InstanceDrawDistance`, 5.8 `PrimitiveUniformShaderParameters.cpp` 15, `HierarchicalInstancedStaticMesh.cpp`
  1675): Low 0.4, Medium 0.6, High 0.8, Epic 1.0 (BaseScalability's ViewDistanceQuality; the project doesn't override it). A
  fixed `InstanceEndCullDistance` E culls Medium at 0.6 E and High at 0.8 E: Medium-only is not possible with it. A cull that
  bites on Medium (e.g. 700 m: E = 1,167 m, High 933 m, Epic 1,167 m: High loses the odd tree from the boundary's far
  corners) takes the far ridges' trees off the skyline in the views that need the gain (past 700 m: Aerial 236 of the 1,059
  trees in its field, PointOverValley 208 of 1,223, Farm 135 of 1,231): a visible change, not made. (`bNeverDistanceCull`
  only keeps the CullDistanceVolume off; per-instance distances still apply.)
- FamilySplit.json's `*_FarTreesCull1500m` stops (`Looter.Perf.Scatter FarPine|FarBroadleaf cull 15` = 1,500 m x 0.6 = 900 m
  on Medium) cull ~0 trees in Aerial/Farm/PointOverValley/Grave: they will read as no change. `*_NoFarTrees` is the bound.
- What they cost instead: each far tree is ~300 triangles (FarPine_A 300, FarPine_B 301, FarBroadleaf 298; no LODs, Nanite
  off, by `FarTrees.py`'s design), so the open views draw 0.23-0.37 M of them in the prepass and the base pass (in the field:
  Aerial 1,059 trees, PointOverValley 1,223, Farm 1,231, Grave 954, RidgeFoot 755), and 70-92% of those stand 500-900 m out,
  17-31 px tall at 1080p (a 13.6 m pine: 15,560 / distance px at the tour's 80 degrees): under a pixel per triangle.
  Instanced triangles don't show in `RHI/PrimitivesDrawn` (PERF.md's note), so PERF.md's "Beyond's triangles 0.06-0.07 M: its
  cost is pixels" leaves these out.
- **Proposal (not made; Main's call after FamilySplit's `_NoFarTrees` rows):** LODs on the three far trees in
  `Art/Models/Vegetation/FarTrees.py` (`finish()`, where it sets `obj['LODs'] = ''`): `obj['LODs'] = '50,25'` and
  `obj['LODScreens'] = '0.04,0.025'` (the importer reduces them in Unreal; the HISM picks a LOD per cluster of neighbouring
  instances). Screen size 0.04 is ~400 m on High/Epic and ~320 m on Medium (`r.StaticMeshLODDistanceScale` 1.25), 0.025 ~640
  / ~510 m (a far pine's ~7.5 m bounds radius at the 80 degree view: 15.9 / distance). In Aerial that is ~85k triangles
  instead of ~318k (-73%); a 75-triangle tree past 510 m is under 31 px tall. The 8x sight's zoom narrows the view, so LOD
  selection gives it LOD0 again. High/Epic change only past 400 m, where a tree is under 40 px. Re-import: `Tools\artrun.ps1
  -Export FarTrees -Family FarTreesLods`, then `Looter.ImportModels` on it; no level rebuild (the HISMs keep the meshes).
  Update the module's docstring ("no LODs (three draws in all") with it.

## 3. Shrubs without shadows on Medium only (the user's pick): tagged in the scatter script; the switch needs C++

- Where: `Tools/Unreal/build_island_scatter.py`. PCG spawns each layer's meshes from `PCGSoftISMComponentDescriptor`s made in
  `entry()`; `cast_shadow` there is one value for every preset, and no engine setting switches an instanced component's
  shadow by scalability (`grass.DisableDynamicShadows` is landscape grass only; `r.Shadow.RadiusThreshold` tests a
  component's bounds, and a scatter component's span the area). So: the script tags them, the game switches them.
- Done (script): `entry(..., medium_shadow=True)`; `medium_shadow=False` with `shadow=True` adds the component tag
  `SHADOW_HIGH_ONLY` = `ShadowHighOnly`. Set on Bushes (`BUSHES`), Rabbitbrush, Sagebrush (the mask's and the computed
  groups'), Big sagebrush, Pit sage. Not on the junipers (small trees with trunk hulls; the user's words were "sagebrush and
  bushes"), nor on trees, rocks, logs. Takes effect at the next scatter generation (Main's post-erosion scatter run).
- Needed (C++, not done: Source is outside this lane). One line in `Settings/GraphicsSettingsSubsystem.cpp`'s
  `QualityVariables`, after `r.StaticMeshLODDistanceScale`:
  `{ TEXT("Looter.Scatter.HighShadows"), { 0, 0, 1, 1 } },` with the comment: shrubs (scatter tagged ShadowHighOnly) cast no
  shadow on Low and Medium: about -0.1 ms at the Farm, the user's pick 2026-11-04; High and Epic keep them. `ApplyQuality`
  skips a CVar it can't find, so the variable must exist; and something must apply it to the components as they load. Best
  beside `ULumenScatterSubsystem` (it already walks every actor's instanced parts at BeginPlay and on level streaming):
  - `World/LumenScatterSubsystem.cpp`: `static TAutoConsoleVariable<int32> CVarScatterHighShadows(TEXT("Looter.Scatter.HighShadows"),
    1, TEXT("0: scatter tagged ShadowHighOnly (the shrubs) casts no shadow (Low and Medium, set by the graphics preset)."),
    FConsoleVariableDelegate::CreateStatic(&ApplyScatterShadowsEverywhere));` where the delegate walks every game world
    (`GEngine->GetWorldContexts()`, `IsGameWorld()`) and calls the actor pass below on each actor.
  - `static int32 ApplyScatterShadows(const AActor&)`: every `UInstancedStaticMeshComponent` with
    `ComponentHasTag(TEXT("ShadowHighOnly"))` gets `SetCastShadow(CVarScatterHighShadows.GetValueOnGameThread() != 0)` (it
    dirties the render state itself). Called from `OnWorldBeginPlay` and `HandleLevelAdded` next to `LeaveOutOf` (which
    skips parts over `SmallRadius`: the shadow rule must not sit behind that filter; bushes may be over 1.5 m).
  - A test beside `LumenScatterTests.cpp`: an actor with a tagged ISM: CVar 0 -> `CastShadow` false, 1 -> true, an
    untagged one untouched. `ApplyQuality` sets the CVar with `ECVF_SetByGameOverride` (the command line still wins).
- Expected: about -0.1 ms at the Farm (PERF.md option 3; most of scatter's 0.12 shadow share there). FamilySplit's
  `Farm_ShrubNoShadow` (`Sagebrush|Bush_|Juniper|Rabbitbrush shadow 0`) includes the junipers: an upper bound of this.
- Look on Medium: sagebrush and bushes without a ground shadow within the 100 m cascades (grass already has none).

## Files changed (this lane)

- `Art/Models/Rocks/Cliffs.py`: `FALLBACK` 12, `PANEL_FALLBACK` 25, `build()` by kind, docstring.
- `Tools/Unreal/build_island_scatter.py`: `SHADOW_HIGH_ONLY`, `entry(medium_shadow)`, the five shrub layers.
- `Tools/Blender/cliff_fallback_sheet.py` (new): the cliff previews and report.
- `CODEMAP.md` (the sheet script's line), `Docs/Pipeline/Assets.md` (cutting a hulled Nanite mesh's fallback),
  `Docs/Pipeline/Levels.md` (the shrubs' tag).
- Outputs (not in git): `Intermediate/ArtExport_Lods/` (the 8 faces' FBXes and a manifest trimmed to them: the panels'
  entries and FBXes taken out, unchanged at 25%), `Saved/ArtPreviews/Lods/` (96 crops, `report.json`, the sheet, its 9 tiles).

## Re-import, rebuild, measure (Main)

1. Cliffs (editor open, not in PIE): `Tools\console.ps1 "Looter.ImportModels C:/Dev/AI_Looter_Shooter/Intermediate/ArtExport_Lods"
   -Until 'Looter\.ImportModels: imported'`. Look for 8 "Fallback [...], num tris: ~1200" lines. No level rebuild: the
   placed actors keep `SM_CliffFace_*`; the complex collision recooks with the import. A fresh repeat of the export:
   `Tools\artrun.ps1 -Export Cliffs -Family Lods` (writes all 12; trim or import all, the panels are unchanged).
2. Measure (editor closed, `Saved\ArtPause.flag` set): `Tools\perf.ps1 -Label "LODS cliffs 12%" -Exec "Looter.Quality Medium"`
   and `Tools\tour.ps1` for Aerial, Aerial_Dusk, Farm, PointOverValley, the deck. Play-check a spider climbing a cliff face
   and bullet marks on one (High too: the cut's deviation).
3. Shrubs: after the C++ above is built (`Tools\launch.ps1 -Build`), the scatter regenerates with the tags at the next
   `Tools/Unreal/build_island_scatter.py RansomsRest` (and `TutorialIsland` if it should match); then `perf.ps1` at Farm on
   Medium and a High check that the shrubs still cast.
4. Far trees: nothing to import; the LOD proposal (2) waits on the FamilySplit `_NoFarTrees` rows.
