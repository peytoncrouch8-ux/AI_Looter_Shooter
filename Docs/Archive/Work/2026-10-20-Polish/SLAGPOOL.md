# Slag Bomb pool model (POLISH-ART, 2026-10-20)

Source `Art/Models/Props/SlagPool.py` (+ textures `Art/Textures/SlagPool/make_slag_pool.py`). Export test:
`Intermediate/ArtExport_SlagPool` (repeat export bit-identical; tangentcheck 0). Main imports it:
`Looter.ImportModels C:/Dev/AI_Looter_Shooter/Intermediate/ArtExport_SlagPool` -> `/Game/Art/Props/SM_*`
(`/Game/Art/Props` is already under DirectoriesToAlwaysCook, so `AEmberPropActor::Place`/`FindMesh` by path is fine).
Previews (9x, daylight, dusty uneven ground): `Saved/ArtPreviews/Props/SlagPool/SlagPool_top.png`,
`SlagPool_three.png`, `SlagPool_cooled_three.png`, `SlagSplats.png`.

## Meshes (all: pivot on the ground at the middle, Nanite off, no collision, no LODs, no sockets)

| Mesh | Tris | Size at scale 1 | What |
|---|---|---|---|
| `/Game/Art/Props/SM_SlagPool.SM_SlagPool` | 3864 | mean radius 50 cm (outline 45-55 cm, droplets to ~58), z -0.8..1.5 cm | the hot pool |
| `/Game/Art/Props/SM_SlagPoolCrust.SM_SlagPoolCrust` | 2190 | inside r 44 cm, z 0.5..1.4 cm | cooling crust overlay |
| `/Game/Art/Props/SM_SlagSplatA` / `SM_SlagSplatB` | 1971 / 2101 | ~1 m across, z -0.8..1.4 cm | small splashes |

Heights at scale 1: melt 0.6 cm above the pivot (0.75 in the middle); crust plates 0.3-0.7 cm above the melt (they
thin into the rim); a rounded crust lip runs down to -0.8 cm at the outline (buried, so mild bumps are swallowed).

## Placing it

- Scale XY = `Radius / 50` (Radius 450 -> 9). The previews use uniform 9 (melt 5.4 cm, plates up to ~13 cm, lip foot
  -7 cm). For a flatter pour or bumpier ground scale Z lower (e.g. 4-5); hug slopes by tilting it to a plane fitted
  through `EmberFX::GroundRing`'s points (`Edge`). Random yaw.
- Shadows: optional; `CastShadow = false` is fine (plates are a few cm tall) and saves the shadow pass.

## Material slots (slot name = Unreal slot; MI_<name> made by the importer)

| Index | Slot | Material | Default |
|---|---|---|---|
| 0 | `SlagCrust` | M_World, set SlagPool (iron-black crust) | no glow |
| 1 | `SlagMolten` | M_StylizedSurface (flat), the cracks | Color #FF6A14, **Glow 6** |
| 2 | `SlagWhiteHot` | M_StylizedSurface, the lake in the middle + cracks round it | Color #FFC45C, **Glow 9** |
| 3 | `SlagGlowEdge` | M_StylizedSurface, hot sides of the inner plates | Color #E2440C, **Glow 3.5** |

The crust overlay has only `SlagCrust` (index 0). Splats have all four, same order. Look slots up by name
(`GetMaterialIndex(TEXT("SlagMolten"))`), not by index.

## Meant to be driven by code (nothing is required: static, it reads as hot slag)

- **Glow (scalar `Glow`, vector `Color` on M_StylizedSurface; emissive = Color x Glow).** Make one MID per glow slot
  (`SlagMolten`, `SlagWhiteHot`, `SlagGlowEdge`) and `SetMaterial` it on every pool/splat component's matching slot,
  so one set of parameters drives all. Suggested, with `Cool = PoolAge / PoolSeconds`, `Heat = 1 - Cool^1.5`:
  `Glow = Default * lerp(0.15, 1, Heat) * Fade` (Fade = the existing last-0.6 s fade); `Color = lerp(#5A1406, Default,
  Heat)` so a dim crack reads dark red, not orange paint. A small flicker (0.9-1.0) is fine on Molten/WhiteHot.
- **Crust overlay (SM_SlagPoolCrust).** Attach it to the pool component (same pivot, inherits scale). Relative
  `Z = -0.9 * (1 - Crust)` (model cm; the parent's Z scale applies): at -0.9 it is entirely under the melt (invisible),
  at 0 the cracks are hairlines and the lake is crusted. Suggested `Crust = smoothstep(0.2, 0.9, Cool)`. Raising it
  grows crust out from each plate's edge into the cracks (`SlagPool_cooled_three.png` = Crust 1, glow x0.35).
- **End.** Over the last 0.6 s sink the whole pool (and splats) by ~2 cm model units while Glow -> 0, then hide/remove.
- **Splats (optional).** 1 m across at scale 1 (scale = wanted diameter in m, uniform), each on its own ground hit and
  normal: 2-3 at `Blobs` spots near the rim (0.8-1.5 m) break the round outline; same MIDs.
- The mesh replaces what `DrawPool` drew as surface (the `Edge` line ring and the blob streaks); sparks, smoke, haze and
  the High/Epic light can stay.
