# Art style exploration (2026-10-02)

The user finds the approved look (`Docs/Art/StyleTarget_Outpost.png`, "stylized realism") bland and asked for eight
art styles on one game asset to choose from; the chosen style then goes onto every asset. References given: Sable
(ink lines, flat color, pastel skies) and Jumanji: The Video Game (chunky painted stylized realism).

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

## Round 2: mixes of 1 and 4

The user asked for three styles between 1 (ink & flat color) and 4 (gouache storybook). Six candidates were designed
independently, each from a different brief along that axis (two ink-leaning, two balanced, two wash-leaning), as
plugin styles in `Tools/Blender/style_plugins/`; three judges then scored each for blend fidelity, craft and game fit
(1 to 10). `hybrids/` holds all six (`contact_sheet_all_six.png`) and the three put forward (`contact_sheet.png`).

| # | Style | The mix | Blend / craft / game fit |
|---|---|---|---|
| 9 | Storybook ink | Balanced. Gouache fills that keep gentle color variation, wet-edge pooling, paper grain and a pencil underdrawing under dark-brown ink; Sable's two-tone shade, pastel sky and outlined flat clouds. | 7 / 7 / 7 |
| 10 | Screen Print Wash (**chosen**, `Docs/Art/ScreenPrintWash.md`) | Ink-leaning. Style 1's flat quantized fills and hard violet shade; style 4's pencil laid under a thinner ink line, granulation in the shade instead of stipple, cream paper, the color plate misregistered like a screen print. | 7 / 7 / 6 |
| 11 | Moebius Watercolor | Balanced. Clean uniform ink and a Moebius peach-to-lavender sky; pale luminous washes that grade softly from lit to shade, pigment bleed, paper, soft unlined clouds. | 7 / 6 / 6 |
| 12 | Inked Wash | Ink-leaning. Style 1's bold black ink, hard violet two-tone and flat outlined clouds over watercolor fills with granulation and pooling; the sky laid as a wash. | 6 / 6 / 7 |
| 13 | Inked Gouache | Wash-leaning. Style 4's washes, pooling, paper and pencil, with a thick black contour on silhouettes only. | 7 / 5 / 7 |
| 14 | Pastel flat gouache | Wash-leaning. Quantized flat fills and a three-tone shade with every edge softened into a wash boundary; no ink at all. | 6 / 6 / 5 |

The panel's own pick was 9, 10 and 13 (the best of each axis position); 13 was swapped for 11 on review: the same
mean score, cleaner contours, and a more distinct position between the parents. In Unreal all six are the pipeline of
styles 1 and 4 combined: edge-detected ink (one pass, or two for the pencil-under-ink pairs), a two-tone or soft light
ramp, a screen-space paper and granulation overlay, and regenerated softer texture sets. The doubled line pass is the
part most likely to swim in motion.

    python Tools/Blender/style_compose.py <passdir> <outdir> --no-builtin --plugins Tools/Blender/style_plugins/hybrid_storybook_ink.py,Tools/Blender/style_plugins/screen_print_wash.py,Tools/Blender/style_plugins/hybrid_moebius_watercolor.py
