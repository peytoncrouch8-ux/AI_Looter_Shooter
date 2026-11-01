# Lane TRIPO-M47: Mains 4-7's Tripo models (2026-11-01)

Made, fitted and export-tested. Nothing imported, built or committed; Unreal never opened. Blender only through
`Tools\artrun.ps1`. Export: `Intermediate/ArtExport_TripoM47` (2 models). Pictures: `Saved/ArtPreviews/TripoM47/`
(`Sheet.png`; `Check_<Name>_q/front/side/top/back.png`, hulls red, sockets magenta; `Raw_<Name>_*.png` Tripo's GLB as delivered).

## Credits (cap 85): 60 used, 25 left

| Model | Credits | Result |
|---|---|---|
| ReliquaryWhole (M45-1) | 30 | used: granite chest, gold bands/pillars/lid rim, gold-ringed recess, diamond plaque on the front; no repaint needed |
| SoulCoal (M67-3) | 30 | used: black faceted lump, cyan veins on one face |

Why these two: the Reliquary is the centre of Mains 4-7 and SaintAdaWakes (looked at in Main 4, drunk by the hose, lit in
Main 7) and the build warns it's missing; the soul coal is the most-seen of the rest: ~110 lumps in four fields over the
Rest from Main 6's end until Saint Ada wakes, 12 the player cracks up close, and BargeFall's coal rain. Abel's black
lantern (M67-9) has no swap path and is seen in a fast boss fight: skipped. No repaint was spent (both on palette after the
grade; a repaint costs 20 and would leave 5). Records: `Art/Tripo/<Name>/task.json`; specs `Art/Tripo/specs/m47.json`
(the Reliquary's spec carries a `paint` + `paint_style` = our scripted Reliquary render, unused).

## Files (new)

- `Tools/Blender/looter_tripo_m47.py`: BASE specs on `looter_tripo_wardens.build` (graded maps, MI_Tripo<Name> on M_World,
  Nanite off) plus one fit per model. `LOOTER_TRIPO_ONLY=A,B` limits a run; `LOOTER_COAL_SPLITS` tries other vein splits.
- `Art/Models/Tripo/WardensMains47.py`: the model source (category Tripo -> `/Game/Art/Tripo/SM_<Name>`).
- `Tools/Blender/tripo_m47_sheet.py`: `--check,Name` and the sheet (line-up beside a 1.8 m figure, the scripted smashed
  Reliquary, the chapel bell, SM_AbelLantern, the stand-in rock; coal close by day and dusk; the checks).
- `Art/Tripo/{ReliquaryWhole,SoulCoal}/`, `Art/Tripo/specs/m47.json`, `Art/Textures/Tripo{ReliquaryWhole,SoulCoal}/`.
- CODEMAP lines (Main writes): `Tools/Blender/looter_tripo_m47.py` (Mains 4-7's Tripo models fitted to the code's spots),
  `Tools/Blender/tripo_m47_sheet.py` (their check renders and sheet), `Art/Models/Tripo/WardensMains47.py` (their source).

## The models (UE cm, actor frame; tris = LOD0)

| Asset | Size X x Y x Z | Tris | Hulls | Sockets | Maps | LODs |
|---|---|---|---|---|---|---|
| SM_ReliquaryWhole | 74 x 120 x 72 | 12,980 | 1 box | Ember (-0.4, -6.0, 66.7), Interact (32.4, -0.1, 36.2) | 2048 | 50/25/12 |
| SM_SoulCoal | 30 x 30 x 22 | 4,821 | 1 k-DOP | - | 1024 | 40/15/6 (screens 0.4/0.18/0.07) |

- **Reliquary:** front +X (the diamond plaque) toward the nave, length across (UE Y: the plinth's 1.56 m side; 18 cm
  spare each end, 14 cm front and back on its 1.02 m), pivot bottom middle (stands on SOCKET_Reliquary). Tripo's chest
  scaled 1.70 and pressed to 0.72 m high (uniform would be 0.85; the stand-in is 0.66), depth 0.74. Tripo's recess was a
  10 cm deep bowl (radius 19 cm): the ember (code: 3 cm over the socket, 11 cm glow) would hide under the rim from the
  nave, so its floor is raised to 4 cm under the lid top; Ember sits on that floor (lid top 70.7), +Z up. Interact on the
  plaque's face. One material slot `TripoReliquaryWhole` (granite + gold in one set). Lid top 0.42 + 0.72 = 1.14 m over
  the sanctuary floor: below eye height, the ring and ember read from the nave.
- **Soul coal:** Tripo's veined face turned up, pivot bottom middle; the code sizes it by bounds (lumps 32 cm, fields
  16-42), so the native size only matters to the coal rain (below). Slots: 0 `TripoSoulCoal`, 1 `SoulCoalVein`
  (emissive surface 0x52dcff, Glow 4, like LanternGlow: no translucency). Vein faces cut per face from Tripo's paint after
  one split of the faces at least half on a vein (951 faces; 3,691 -> 4,821 tris; splitting twice tripled it, collapsing
  back tore the shape).
- Look: graded by `write_maps` (sat x0.86, contrast x1.06, roughness floor 0.38). On the sheet: granite light-mid grey
  with darker flecks like our RockGranite, gold reads as worn brass; coal matte-glassy black. Neither is clay or plastic.

## Code edits Main must make

1. **`World/BargeBoardingDying.cpp` `LayCoal` (required):** the rain's scale `FRandRange(0.25, 0.45)` assumes the 100 cm
   engine sphere; the Tripo coal is 30 cm, so lumps would fall 7-13 cm. Scale by the mesh: e.g.
   `const float Fit = 100.f / FMath::Max(1.f, 2.f * CoalRain->GetStaticMesh()->GetBounds().BoxExtent.GetMax());` and
   `FVector(Spot.FRandRange(0.25f, 0.45f) * Fit)`.
2. **`World/SoulCoal.cpp` `ApplyCrackedLook` (required for "its glow out, dull grey"):** the vein slot keeps glowing on a
   cracked lump. `const int32 Vein = Lump->GetMaterialIndex(TEXT("SoulCoalVein")); if (Vein != INDEX_NONE) Lump->SetMaterial(Vein, Charcoal);`
   (Charcoal is a constructor static: keep it as a member or load MI_Charcoal there; check the imported slot name).
3. **`World/SoulCoalField.cpp` (recommended):** with the Tripo coal skip the Crust layer (a second 0.82-scale copy of
   every lump inside it, in M_Smoke; the veins glow on their own): build crust instances only when `!Tripo`.
4. `build_area_chapel.py`, `AChapelReliquary`, `build_area_main7.py`: no change (paths and socket names match).
   Optional: the stand-in's `ChestLength` runs along X (1.2 m deep on a 1.02 m plinth); swap it with `ChestWidth`.

## Import steps (Main)

1. Commit (LFS): the files above.
2. Editor open, no PIE: `Tools\console.ps1 "Looter.ImportModels C:/Dev/AI_Looter_Shooter/Intermediate/ArtExport_TripoM47"`.
   New: SM_ReliquaryWhole, SM_SoulCoal in `/Game/Art/Tripo`; MI_TripoReliquaryWhole, MI_TripoSoulCoal on M_World,
   MI_SoulCoalVein.
3. Edits 1-3, build (`Tools\launch.ps1 -Build`: SoulCoalLump/Field and BargeBoarding find the coal in constructors),
   rerun `build_area.py RansomsRest gameplay` (chapel: the Reliquary's model; main7: fields sized by the new bounds).
4. Tests: `Looter.World.SoulCoal`, `Looter.World.ReliquaryLantern`, `Looter.Scenes.SaintAdaWakes`, the ChapelReliquary
   tests (`-Filter Looter.World.ChapelReliquary` or what the group is called).
5. PIE: the apse (Reliquary on the plinth facing the nave, ember glowing just over the gold ring, prompt on the plaque,
   flash); Main 7's coal fields at dusk (veins read, perf in the valley view: ~110 lumps, mostly LOD2-3, 290-720 tris);
   a cracked lump goes dull; BargeFall's rain lump size.

## Risks

- The Reliquary's ember is 6 cm left of centre (Tripo's recess is off-centre); the ring's inner wall was pressed with
  the floor (316 vertices), so Tripo's normal map may shade it a little off there.
- Tripo left two small carved slots and a pin on the lid (read as worn carving).
- Vein glow at LOD2-3 is reduced by Unreal's LOD builder; far lumps rely on their glow quad (drawn within 35 m).
