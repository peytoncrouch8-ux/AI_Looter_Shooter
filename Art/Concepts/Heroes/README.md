# Hero upload views for Tripo (2026-10-27)

The user is remaking the five heroes in Tripo (paid plan, multi-view image-to-3D), as with the AR parts. Each hero
folder (`Ellis`, `Odessa`, `Crane`, `Gauge`, `Pike`) holds four views of the hero as they are in the game:
`upload/` (transparent PNGs: upload these) and `upload_white/` (the same on pure #ffffff), each with `front.png`,
`back.png`, `left.png`, `right.png`.

## What the views are

- Renders of the existing Blender hero models, not drawings: the game mesh with its baked textures, from
  `Art/Models/Characters/<Hero>.py`.
- Neutral A-pose: upper arms 42 degrees below horizontal, out to the sides, elbows bent 6 degrees, wrists straight,
  palms down, fingers as built (relaxed); legs at rest (straight, feet a little apart). The swing bones (coat skirts,
  capes, hair, bandana, lantern) at rest. Nothing in the hands: holstered and belt gear is outfit and stays.
- Orthographic Cycles, 1024 x 1024, the same scale in all four views of a hero; the figure ~85% of the frame's height,
  centred on its bounding box. 64 samples, OpenImageDenoise, AgX.
- Light rides with the camera (every view lit alike): soft area key from front-left above, fill from the right, rim
  from behind above, a light grey ambient world; no ground, so no ground shadow.
- Sides: front faces the camera (their left on the image's right); `left` shows the hero's own left side (they face
  the image's left); `right` their right side. If Tripo's preview comes out with the sides swapped, swap `left` and
  `right`.

## How they're made

`Tools/Blender/hero_views.py` (its docstring has the flags):

    Tools\artrun.ps1 -Script Tools\Blender\hero_views.py -ScriptArgs <Hero>

It runs the hero script's own `main()` as a `--preview` run (6 threads, so the baked maps fit; the bake is skipped
when the maps on disk are current) and takes over its preview render: saves the rigged, textured figure to
`Saved/HeroViews/<Hero>.blend`, poses it and renders. To render again without building (a light or pose change):

    blender -b Saved\HeroViews\<Hero>.blend --python Tools\Blender\hero_views.py -- <Hero> --render

## Tripo steps

1. Image to 3D, **multi-view**: front = `upload/front.png`, then left, back, right from the same folder. If the result
   comes out flat or doubled, try `upload_white/`.
2. Generate, then **Smart UV**, then **Texture** (PBR if offered).
3. If Tripo offers **auto-rigging** for humanoids, run it.
4. Export **GLB** and save it in the hero's `upload/` folder.
5. Main takes it from there (`Docs/Pipeline/CharacterModels.md`): scale to the hero's height, face Blender -Y,
   retarget from the mannequin as today's heroes are, split the glow parts into M_CharacterGlow slots.
