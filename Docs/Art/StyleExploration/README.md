# Art style exploration (2026-10-02)

The user finds the approved look (`Docs/Art/StyleTarget_Outpost.png`, "stylized realism") bland and asked for eight
art styles on one game asset to choose from; the chosen style then goes onto every asset. References given: Sable
(ink lines, flat color, pastel skies) and Jumanji: The Video Game (chunky painted stylized realism).

The user turned down all of these styles on 2026-10-08; the next round is the Style Lab (`Tools/StyleLab/`).

The asset is the village cottage, `Art/Models/Buildings/Cottage.py`, with its real textures. It was built headlessly
in Blender and rendered once with Cycles (one sun from the left, a soft sky, the game's grass on the ground); every
style is then composed from the same render passes (albedo, the sun's shading and shadows, sky light, ambient
occlusion, world normals, depth, mist, two Freestyle line passes). Nothing is AI-generated, and no Unreal asset
changed. The comparison is fair: same model, same camera, same light.

- `contact_sheet.png`: all nine side by side (0 is the current look).
- `00_current_look.png` ... `08_pen_and_ink_crosshatch.png`: each style at 1600x900.

## The styles, and what each one means for the game

Costs are rough post-process estimates at 1080p on the Medium preset (RX 580, budget 8.3 ms a frame, no Lumen).

| # | Style | Looks like | How it would be built in Unreal | Cost |
|---|---|---|---|---|
| 1 | Ink & flat color | Sable, Moebius | Post-process outlines from depth and normal edges; a two-tone light ramp; a screen-space stipple in the shade band. Texture sets become flat palettes (tints), normal maps off. Sky: a gradient dome. Foliage cards need per-object silhouettes (custom depth) or no lines. | ~0.4 ms, and cheaper materials |
| 2 | Painted stylized | Jumanji, Fortnite | Mostly content: the texture generator makes softer, bolder, hand-painted sets (bigger shapes, baked soft occlusion and highlights, more saturation). Masters add a rim light (Fresnel × sky color); post adds bloom and a color grade. No lines. | ~0.3 ms; materials as now |
| 3 | Cel toon, colored lines | Wind Waker, Hi-Fi Rush | Post-process: light = scene color ÷ base color, quantized to three steps; outlines colored from the base color (×0.4); hard specular shapes. Textures simplified, chroma pushed. | ~0.4 ms |
| 4 | Gouache storybook | watercolor and pencil | Post-process: paper overlay, edge darkening, a half-resolution bleed blur, jittered pencil lines from depth edges. Softer texture sets. | ~0.6 ms (the blur) |
| 5 | Comic halftone | Spider-Verse print | Post-process: thick black outlines, halftone dots driven by the light term, posterized color, per-channel offset. Flat palettes as in 1. | ~0.4 ms |
| 6 | Flat-shaded graphic | Firewatch, Grow Home | Content: no textures and no normal maps, one color per material, hard-edged normals on export; one directional light with hard shadows and a gradient sky. Grass and terrain become flat color fields. | the cheapest of all |
| 7 | Oil-painted realism | Arcane, Dishonored | Post-process: an anisotropic Kuwahara (brush) filter, clarity, a teal/orange grade, vignette; keeps PBR textures. | 1.5 to 3 ms at full res: over budget unless half-res |
| 8 | Pen & ink crosshatch | ink drawing, light wash | Post-process: sketchy outlines, three screen-aligned hatch textures driven by light and occlusion, paper, 20% color wash. Readability of enemies and loot needs saturated accents. | ~0.4 ms |

Every option works without Lumen and Nanite. 1, 3, 5 and 8 share one piece of work (edge-detected outlines plus a
light term from the GBuffer); 2 and 4 are mostly texture work; 6 removes textures altogether.

## Re-running on another asset

`Tools/Blender/style_render.py` builds a scripted model and renders the passes; `Tools/Blender/style_compose.py`
composes the styles and the sheet. Both run inside Blender's Python (`blender -b --factory-startup --python ... --`)
or with the `bpy` module from PyPI; compose also needs `numpy`, `opencv-contrib-python-headless` and `pillow`.

    python Tools/Blender/style_render.py Art/Models/Buildings/Cottage.py Saved/StyleExplore/Cottage
    python Tools/Blender/style_compose.py Saved/StyleExplore/Cottage Saved/StyleExplore/Cottage/styles [--only 1,2]

The camera frames the model from the front left at about a player's eye height and scales with the model's size.
