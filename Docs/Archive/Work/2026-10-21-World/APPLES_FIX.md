# APPLES fix (apples only show within ~1.5 m of the tree)

## Cause
1. Buried in the leaf cards (main cause, from Art/Models/Vegetation/Trees.py + Tools/Blender/looter_plants.py).
   Apples sit at `center + d * radius * 0.92..1.02` of each leaf clump, mostly on the underside (`- UP * 0.35`).
   `leaf_clump` puts ~22 double-sided masked cards per clump at `radius * 0.5..1.0` of the same clump, each 1.0-1.25 m
   across, overlapping: the shell is covered, so the 12-14 cm apples (radius 0.06-0.07, 5 sides x 3 rings) are
   inside the foliage and only show through gaps when you stand under the crown.
2. Likely compounding (not measurable now: the editor is in PIE, so the LOD/subsystem queries return -1): the
   importer gives Vegetation the default LODs 40% / 12% at screens 0.45 / 0.1575 (ModelImporter.cpp ~l.256; no
   `lods` in the Trees export json). Apple_A's bounds sphere is 340 cm, so LOD1 starts at about 7-13 m; quadric reduction
   to 40% eats 20-triangle isolated balls first. Apple_B/C are not imported yet (no asset), same code path.
Ruled out: the material (M_WorldFoliage opacity = palette alpha, opaque; apples use constant UVs so no mip
fade), placement (orchard trees: build_area.py l.624, TREE_CULL = 0, no per-instance cull, scale 1, no max draw distance).

## Fix: Art/Models/Vegetation/Trees.py (needs reimport of Apple_A/B/C, no level rebuild: same mesh path/pivot)
Replace both `apples` bodies (apple_a and apple_tree) with one shared helper and give the apple trees gentler LODs.

```diff
+def hang_apples(plant, rng, crowns, fewest, most):
+    """Fruit hung just outside each lower clump's leaf cards (cards reach the clump surface and beyond), big enough
+    to read at orchard distances (about 20 cm across)."""
+    palette = lt.material('FoliagePalette')
+    apple_uv = swatch('FruitRed', 0.7, 0.5)
+    for center, radius in crowns[0]:
+        for _ in range(rng.randint(fewest, most)):
+            d = (random_unit(rng) * Vector((1.0, 1.0, 0.5)) - UP * 0.35).normalized()
+            plant.ball(center + d * (radius * 1.1 + 0.25), rng.uniform(0.09, 0.11), palette, apple_uv,
+                       wind=0.8, occlusion=0.95, segments=6, rings=4)
```
- apple_a: `def apples(...)` body -> `hang_apples(plant, rng, crowns, 2, 4)`; add to `broadleaf(...)`:
  `lods='70,30', lod_screens='0.3,0.1'`.
- apple_tree: body -> `hang_apples(plant, rng, crowns, *fruit)`; `broadleaf(name, seed, [settings], hull_height=1.2,
  extra=apples, lods='70,30', lod_screens='0.3,0.1')`.
(`broadleaf` already forwards `lods`/`lod_screens` to `finish`.) If an apple still pops at LOD1 in the check below,
use `lods='100,40'` for the apples' trees (trees only; the orchard is 18 trees).

## Apply
`Tools\models.ps1 -Only Trees` (or the file that holds Apple_A/B/C: `-Only Trees`), then `Looter.ImportModels`
(overwrites SM_Apple_A and creates B/C; Oak/Birch/Oak_C export identically, since only the extra callback's rng draws
change and it runs last). Orchard actors pick the new SM_Apple_A up with no rebuild. Apple_B/C first appear when
the variety pass places them (build_area gameplay/world step, unchanged).

## Check
PIE at the orchard: stand 8 m and 15 m from a tree, compare `r.ForceLOD 0` vs `r.ForceLOD 1`; red must show from the
street. Then `Tools\perf.ps1` before/after is optional.

## Perf cost
Per tree: ~21 apples x 36 tris (was ~20) = about +0.35k tris on an already ~4-8k tree; LOD1 70% instead of 40%
adds about +1-2k tris per tree only between ~7 and 20 m. 18 orchard trees: under +40k tris worst case total, no new
draws (same three material slots). Negligible against 8.3 ms.
