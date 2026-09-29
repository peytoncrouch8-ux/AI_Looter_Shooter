# Art sources

Blender sources for the game's models. Unreal assets are made from these, so change a model here and import it again;
never edit the imported mesh in Unreal.

```
Art/Models/<Category>/<Name>.blend   a model made by hand
Art/Models/<Category>/<Name>.py      a scripted model (Blender Python)
```

The category folder names the Unreal folder: `Art/Models/Props/LanternPost.py` becomes `/Game/Art/Props/SM_LanternPost`.

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
    empty's +Z, like a model's.
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

  No textures: the look is painted by the material.
- **Nanite** is on by default. Set a custom property `Nanite` = 0 on the model object to turn it off.
- Modifiers are applied on export. Hard and soft edges come through as shaded in Blender.

## Scripted models

A `.py` model builds itself in an empty scene with the helpers in `Tools/Blender/looter_model.py` (boxes, cylinders,
materials in hex colors, hulls, sockets). See `Art/Models/Props/LanternPost.py`.
