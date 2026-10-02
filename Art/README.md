# Art sources

Blender sources for the game's models. Unreal assets are made from these, so change a model here and import it again;
never edit the imported mesh in Unreal.

```
Art/Models/<Category>/<Name>.blend   a model made by hand
Art/Models/<Category>/<Name>.py      a scripted model (Blender Python)
```

The category folder names the Unreal folder: `Art/Models/Props/LanternPost.py` becomes `/Game/Art/Props/SM_LanternPost`.

`Art/Backlog/<Category>/` holds finished models kept for later; `Tools\models.ps1` doesn't read it, so they stay out of
the game until moved here (`Art/Backlog/README.md`).

## Export and import

```
Tools\models.ps1                      every model
Tools\models.ps1 -Only LanternPost    just these sources (by file name)
```

The script runs Blender in the background for each source. It exports FBX files and a manifest into
`Intermediate\ArtExport`, then runs `Looter.ImportModels` in the open editor. With the editor closed, run that console
command after opening it. Importing again updates the assets in place, so placed actors keep their meshes.

## Rules

- **Units**: meters (Blender's default). 1 m is 100 Unreal centimeters.
- **Facing**: the model's front faces Blender's Front view (-Y), which becomes the actor's forward (+X) in Unreal. Up is
  +Z in both.
- **Pivot**: the model object's origin. Put it where the model meets the ground (the foot of a post, the bottom of a
  crate). Its rotation and scale are applied on export. Its position in the Blender scene doesn't matter.
- **One model** is one top-level mesh object. Everything parented under it is part of it:
  - other meshes merge into it;
  - meshes named `UCX_...` are its collision. Each must be convex (boxes and simple hulls); use several for other
    shapes. Without any, Unreal uses the mesh itself as collision.
  - empties named `SOCKET_<Name>` become sockets (attach points). A socket's front is the empty's -Y and its top the
    empty's +Z, like a model's. Models in one file can share socket names: Blender's numbering (`SOCKET_Muzzle.001`)
    is dropped.
- Objects whose names start with `_` are skipped (references, helpers). A file may hold several models; each becomes
  its own asset. The mesh asset is named `SM_` plus the object name.
- **Materials**: every face needs a material. Its name names the Unreal material instance (`MI_<name>` in
  `/Game/Art/Materials`), so models that should look alike share material names. The look comes from the game's
  stylized material: the Principled BSDF's Base Color, plus these optional custom properties on the material:

  | Property | Meaning |
  |---|---|
  | `Kind` | `Surface` (default), `Foliage` (two-sided, for leaves and grass) or `Glow` (additive light) |
  | `TopColor`, `TopBlend`, `TopThreshold` | moss or grass settling on upward faces (0 to 1) |
  | `GradHeight` (m), `GradDark` | darker toward the base, over this height |
  | `Strata` | horizontal banding for cliffs (0 to 1) |
  | `Wind` (m) | sway at GradHeight (needs GradHeight) |
  | `Glow` | emissive strength |
  | `Variation` | painterly color jitter (0 to 1, default 0.12) |
  | `UpNormal` | light it like the ground below (grass, flowers) |

  These flat-colored materials are the old style. **The textured style** (Docs/TutorialIsland.md) names a master
  material instead, with these custom properties (`Tools/Blender/looter_textures.py`'s `material()` sets them):

  | Property | Meaning |
  |---|---|
  | `Master` | `World` (opaque), `WorldFoliage` (masked, two-sided, wind), `Terrain`, `Water`, or the effects `Waterfall` and `Smoke` (translucent, scrolling; vertex color A is opacity, R foam) |
  | `TextureSet` | the folder `Art/Textures/<set>` holding `T_<set>_BC.png` (color), `_N.png` (DirectX normal map), `_ORM.png` (occlusion, roughness, metallic). For `Terrain`, the set's color map is the island's macro map |
  | `DetailSets` | `Terrain` only: `'GroundGrass,RockCliff'`, the tiled detail sets for grass/soil and rock |
  | `Tint` | `'#RRGGBB'`, multiplies the color (default white) |
  | `UVScale` | multiplies UV 0 (default 1) |

  The importer brings the textures to `/Game/Art/Textures/<set>` (color, normal-map or mask settings from the file's
  suffix; only again when a file changes) and makes `MI_<name>` an instance of `/Game/Art/Materials/Masters/M_<Master>`
  (built by `Tools/Unreal/build_world_materials.py`). Vertex color alpha is baked ambient occlusion; on foliage, R is
  the wind sway weight and G a phase offset. These models export their vertex colors linear.
- **Nanite** is on by default. Set a custom property `Nanite` = 0 on the model object to turn it off.
- Other model properties: `Fallback` = the percentage of triangles Nanite's fallback keeps (what Medium and Low draw;
  terrain always keeps 100), `LODs` = `'40,12'` (LOD1, LOD2 ... as percentages; for models without Nanite) with
  `LODScreens` = `'0.45,0.15'`, and `Collision` = `'None'` for no collision at all. Vegetation without Nanite gets LODs
  of 40% and 12% by default, and vegetation without `UCX_` hulls gets no collision. A plant's hulls are all of its
  collision, for bullets too: they meet a tree's trunk, never its leaf cards.
- Modifiers are applied on export. Hard and soft edges come through as shaded in Blender.

## Rigged models (skeletal meshes)

A top-level **armature** is a rigged model: a skeletal mesh whose code moves its bones (creatures).

- The meshes under the armature are its **skin**, merged into one skeletal mesh named `SK_` plus the first mesh's name.
  Weight every vertex to its bones (vertex groups, with an Armature modifier). The skeleton and a physics asset come
  with it: `SK_<Name>_Skeleton` and `PA_<Name>`.
- The armature itself becomes the root bone, called `root`, so no bone may have that name. Unreal keeps only where each
  bone starts: where a chain's tip matters (a foot), end it with a small bone of its own.
- **Hit zones** are what shots hit: meshes under the armature named `USP_<Name>` (a sphere; its size is the diameter),
  `UCP_<Name>` (a capsule along its own Z; its width is the diameter, its height the full length) or `UCX_<Name>` (a
  convex hull, which fits a part closely; the script helper `hit_hull` makes one around meshes). Each belongs to the
  bone it's parented to, or the one named in its custom property `Bone`, and the game reads which bone was hit. Shapes
  on the same bone make one body. Every visible part must lie inside a hit zone, or shots pass through it.
- Units, facing and materials follow the rules above. The pivot is the armature's origin.

## Scripted models

A `.py` model builds itself in an empty scene with the helpers in `Tools/Blender/looter_model.py` (boxes, cylinders,
materials in hex colors, hulls, sockets; for rigs: an armature, bones, skin and hit zones). See
`Art/Models/Props/LanternPost.py`, and `Source/LooterEditor/Tests/RigImport/RigTest.py` for a small rig.

Models ported from the game's old code-built meshes (the spider, the gun parts) are built in Unreal's space and units
with `Tools/Blender/looter_port.py`, so their numbers read like the C++ they came from.

Buildings (`Art/Models/Buildings/`) are built with `Tools/Blender/looter_buildings.py`: walls with openings, roofs of
tin or shakes, logs, stone, windows and doors mapped onto the house trim sheet, with hulls, sockets and baked occlusion.

Vegetation (`Art/Models/Vegetation/`) is built with `Tools/Blender/looter_plants.py`: bark tubes, leaf-cluster cards
and opaque blades, seeded so every run gives the same mesh, and `finish()`, which adds the foliage vertex colors
(wind, variation, occlusion), a trunk hull or no collision, Nanite off and the LODs.

## Gun parts

A gun is built from parts: `Art/Models/Weapons/<Gun>.py` (the Bullpup AR, the Ranchhand shotgun) makes each part as
its own model (`SM_BullpupBody_Standard`, `SM_BullpupBarrel_Heavy`, ...) with `Tools/Blender/looter_guns.py`, +X
toward the muzzle. The body sits at the gun's origin; every other part hangs from a socket on an earlier part (the
body's `Barrel`, `Magazine`, `Sight`, `Stock`; a barrel's `Muzzle` for muzzle devices) and is modeled around its own
origin there, so any part fits any body. `<Gun>.parts.csv` beside the script lists each part's key, name, name word,
lowest rarity, weight and stat ranges, and `Tools/Unreal/setup_gun_parts.py` reads it into the weapon definition
(`/Game/Weapons/Data/DA_*`): run it after adding or retuning a part. A dropped gun saves its parts by key, so never
rename or reuse one.

- Sockets the game reads: `Muzzle` (on the last part that has one), `Grip` (right hand), `Foregrip` (left hand), and
  `Aim` on sights: the point the eye lines up with when aiming down the sights.
- Material slots `GunPolymerSand` (the main color) and `GunPolymerGrey` (fittings) are tinted per gun (the
  definition's paints), and `GunAccentGlow` glows in the gun's rarity color.
- Gun materials (`lg.material`) use the `Gun` master: the World master's look plus per-gun wear (scuffs and grime),
  which each gun rolls from its seed (commons worn, legendaries nearly clean).
- Lenses and sight windows use `lg.LENS` (glass on the Glass master), and optics are open tubes, so they can be aimed
  through.
- Parts are small and held close: set the model's `Nanite` property to 0.
