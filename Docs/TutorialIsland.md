# Tutorial island: the new look

**Status:**
- **The tutorial is retired** (the user's call, 2026-10-13): it moved into Ransom's Rest's opening, taught by contextual
  hints in Mains 1–3 (`Docs/Areas/RansomsRest_Expansion.md` section 5). Skyreach's tutorial and "Skip the tutorial" go.
- `/Game/Maps/Lvl_TutorialIsland` is the main menu's backdrop map (`?game=Menu`).
- The island (Skyreach, with Crossroads Town) becomes story area 3 later: a sky island reached from the north side of the
  *Gilded Lily* by a skylift. It keeps today's house set until its own rework.
- Layout: Crossroads Town (the user's pick, 2026-10-03: concept 1 of `Docs/TutorialIslandConcepts.md`; plan in
  `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`); round 1 built 2026-10-08. How the island is built:
  `Docs/Pipeline.md`, "The tutorial island and the older levels".
- History (the first layout, the asset list, the 2026-09-29 performance capture, the work split):
  `Docs/Archive/TutorialIsland_history.md`.

This doc also holds the art bible and technical conventions of the textured "stylized realism" look. The user approved
it on 2026-09-30 (mock-up: `Docs/Art/StyleTarget_Outpost.png`); the island was the first level rebuilt in it.

Targets:
- **120 fps at 1080p on the Medium preset** on the reference PC (RX 580, i7-8700): 8.3 ms a frame, at the heaviest view,
  not just at spawn. High and Epic keep Lumen and Nanite and may cost more.
- Quality and efficiency first: few materials, shared textures, instancing, sensible triangle counts, nothing generated
  while the game runs.

## Art bible: stylized realism

Every asset follows:
- **Believable light and materials.** One warm sun, soft sky light, haze that fades the distance. Surfaces have real
  texture: wood grain, rust, stone, bark, soil. No ink outlines.
- **Slightly exaggerated shapes.** Real proportions, pushed a little: planks a bit thicker, beams chunkier, roofs
  sagging, details (nails, hinges, rivets) slightly oversized so they read at game distance. Silhouettes clean and
  readable, never noisy.
- **Rustic, harsh, lived-in.** Everything used and weathered: sun-bleached tops, grime and damp at the base, moss in the
  shade, rust streaks under metal, patched boards, worn paths where people walk, tools and clutter left out.
- **Palette.** Meadow greens (warm, a little desaturated), straw and dry grass, warm grey-brown wood, stone greys, rust
  orange, faded paint (teal, oxide red, cream) as accents. Saturated color is rare: flowers, a painted door, a lantern,
  loot beams.
- **Readability.** Paths and landmarks guide the player; the route is legible from the spawn.

## Technical conventions

These extend `Art/README.md` (units, facing, pivots, collision, sockets and naming still apply).

### Materials (Unreal side)
A handful of master materials, each lean enough for Medium (built by `Tools/Unreal/build_world_materials.py`; its
docstring lists every master):
- `M_World`: opaque, textured. BaseColor, Normal, ORM (R ambient occlusion, G roughness, B metallic), a tint, and the
  mesh's vertex color alpha as baked ambient occlusion. Most buildings, rocks and props.
- `M_WorldFoliage`: masked, two-sided, wind sway (off beyond a distance). Leaves and grass cards.
- `M_Terrain`: the island's macro color map (one texture over the whole island) plus two tiled detail textures chosen by
  the macro map's alpha (grass/soil, rock); the ground's layers inside it (2026-10-12): `Docs/Pipeline.md`, "Making an
  asset".
- `M_Water`: opaque, cheap, stylized.

SSAO is off on Medium, so shading contact comes from **baked ambient occlusion**: in the ORM texture for trim sheets and
tileables, and in vertex color alpha on meshes (bake it in Blender).

### Textures
Made by script (`Tools/Blender/looter_textures.py`, numpy in Blender's Python) into `Art/Textures/<Set>/`, as PNG:
- `T_<Name>_BC.png` base color (sRGB), `T_<Name>_N.png` normal map in **DirectX** convention (green down, as Unreal
  expects), `T_<Name>_ORM.png` (linear).
- Tileables are 1024 x 1024 and seamless; the house trim sheet is 2048 x 2048.
- Texel density about **320 px per meter** on buildings and props, 256 on terrain detail.

**House trim sheet** `T_HouseTrim` (2048 x 2048, eight strips of 256 px, each tiling along U; V measured from the
bottom). One material covers most of a house:

| Strip | V range | Content | World size of the strip height |
|---|---|---|---|
| A | 0.875 - 1.000 | Weathered siding: 4 boards running along U, nail rows | 0.8 m (boards 20 cm) |
| B | 0.750 - 0.875 | Logs: 2 debarked logs along U, chinking between | 0.8 m (logs 40 cm) |
| C | 0.625 - 0.750 | Hewn beams: 4 beam faces along U, dark edges | 0.8 m (faces 20 cm) |
| D | 0.500 - 0.625 | Fieldstone masonry with mortar | 0.8 m |
| E | 0.375 - 0.500 | Wooden shingles (shakes) in rows | 0.8 m |
| F | 0.250 - 0.375 | Corrugated tin, ridges along V, rust streaks | 0.8 m |
| G | 0.125 - 0.250 | Lime plaster, stained and cracked | 0.8 m |
| H | 0.000 - 0.125 | Four 64 px sub-strips: teal painted trim, oxide-red painted trim, iron strap with rivets, dark window glass | 0.2 m each |

U repeats every 2048 px = 6.4 m at 320 px/m. Faces that need vertical boards rotate their UVs.

### Meshes
- **Nanite** on for buildings, rocks, cliffs and terrain (High/Epic), with an explicit fallback budget (what Medium
  draws). Walkable surfaces (terrain, floors, decks) keep a full fallback so collision matches what's drawn.
- **Trees, bushes and ground cover** are not Nanite: classic LODs (the importer generates them), cull distances, and
  masked leaves only where needed.
- Collision: simple `UCX_` hulls for everything except the terrain (mesh collision). Clutter has none. A tree's trunk
  hull is all of its collision (bullets too). The scatter's trees, rocks, stumps and logs block with their hulls: PCG's
  instances only collide when the scatter says so (`build_island_scatter.py`); `Looter.World.TutorialIslandObstacles`
  checks it.
- Budgets (triangles, fallback / LOD0 on Medium): house 4-8k, shed or windmill 2-4k, tree 3-6k (LOD1 40%, LOD2 12%),
  bush 600-1.5k, rock 300-1k, cliff piece 1-3k, prop 100-1.5k, grass clump 50-200. Terrain at most 150k in total, split
  into tiles so off-screen ones are culled.
- Every asset script can render a **preview PNG** (Eevee) to `Saved/ArtPreviews/<Category>/` for review.

### Terrain and layout
The island (about 200 x 180 m) is built from one layout file, `Art/Levels/TutorialIsland/layout.json` (outline,
heights, plateaus with cliff edges, hills, roads, a creek and pond, building pads, zones). A Blender script turns it into
the terrain tiles and the macro color map, and an Unreal script places buildings and props from the same file, so the
two always agree. Scripts and order: `Docs/Pipeline.md`.

## Performance plan

- **Budgets for new art**, checked at several fixed viewpoints per zone (spawn, crossroads, meadow, grove, lookout), not
  just at spawn: GPU at most 7.5 ms, game and render threads at most 7 ms each, about 700 draws.
- Instancing wherever something repeats (PCG for trees, bushes, rocks, grass), cull distances by size, shadows only from
  things big enough to matter, LODs on vegetation.
- Every import step is measured (`Tools\perf.ps1`); anything that breaks the budget is fixed before the next step.
- Medium's settings the art relies on: SSAO off (AO is baked into the art), motion blur off by default (2026-09-30;
  results in `Docs/Performance.md`).
