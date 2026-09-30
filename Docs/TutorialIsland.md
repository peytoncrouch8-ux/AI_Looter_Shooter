# Tutorial island: the new look

The user approved a new art style on 2026-09-30 (mock-up: `Docs/Art/StyleTarget_Outpost.png`) and asked to rebuild the
tutorial island in it: Skyreach stays a grassy meadow floating in the sky, with rustic houses, roads, varied trees,
reshaped terrain (hills, cliffs, a road network) and an experience worth remembering. Other areas come later.

Hard targets:
- **120 fps at 1080p on the Medium preset** on the reference PC (RX 580, i7-8700): 8.3 ms a frame, at the heaviest view
  on the island, not just at spawn. High and Epic keep Lumen and Nanite and may cost more.
- Quality and efficiency first: few materials, shared textures, instancing, sensible triangle counts, nothing
  generated while the game runs.

## Art bible: stylized realism

What the mock-up shows, and every asset follows:
- **Believable light and materials.** One warm sun, soft sky light, haze that fades the distance. Surfaces have real
  texture: wood grain, rust, stone, bark, soil. No ink outlines.
- **Slightly exaggerated shapes.** Real proportions, pushed a little: planks a bit thicker, beams chunkier, roofs
  sagging, details (nails, hinges, rivets) slightly oversized so they read at game distance. Silhouettes stay clean and
  readable, never noisy.
- **Rustic, harsh, lived-in.** Everything is used and weathered: sun-bleached tops, grime and damp at the base, moss
  in the shade, rust streaks under metal, patched boards, worn paths where people walk, tools and clutter left out.
- **Palette.** Meadow greens (warm, a little desaturated), straw and dry grass, warm grey-brown wood, stone greys,
  rust orange, faded paint (teal, oxide red, cream) as accents. Saturated color is rare: flowers, a painted door, a
  lantern, loot beams.
- **Readability.** Paths and landmarks guide the player; the tutorial route is legible from the spawn.

## Technical conventions

These extend `Art/README.md` (units, facing, pivots, collision, sockets and naming still apply).

### Materials (Unreal side)
A handful of master materials, each lean enough for Medium:
- `M_World`: opaque, textured. BaseColor, Normal, ORM (R ambient occlusion, G roughness, B metallic), a tint, and the
  mesh's vertex color alpha as baked ambient occlusion. Most buildings, rocks and props.
- `M_WorldFoliage`: masked, two-sided, wind sway (off beyond a distance). Leaves and grass cards.
- `M_Terrain`: the island's macro color map (one texture over the whole island) plus two tiled detail textures chosen
  by the macro map's alpha (grass/soil, rock).
- `M_Water` (later): opaque, cheap, stylized.
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
  the masked leaves only where needed.
- Collision: simple `UCX_` hulls for everything except the terrain (mesh collision). Clutter has none.
- Budgets (triangles, fallback / LOD0 on Medium): house 4-8k, shed or windmill 2-4k, tree 3-6k (LOD1 40%, LOD2 12%),
  bush 600-1.5k, rock 300-1k, cliff piece 1-3k, prop 100-1.5k, grass clump 50-200. Terrain at most 150k in total,
  split into tiles so off-screen ones are culled.
- Every asset script can render a **preview PNG** (Eevee) to `Saved/ArtPreviews/<Category>/` for review.

### Terrain and layout
The island is built from one layout file, `Art/Levels/TutorialIsland/layout.json` (outline, heights, plateaus with
cliff edges, hills, roads, a creek and pond, building pads, zones). A Blender script turns it into the terrain tiles
and the macro color map, and an Unreal script places buildings and props from the same file, so the two always agree.

## Asset list (tutorial island)

Priority 1 is what the island needs to read as the new style; 2 makes it rich; 3 is polish.

| Group | Assets | Priority |
|---|---|---|
| Buildings | Plank farmhouse with tin roof and porch; log cabin with stone chimney; timber-frame cottage (plaster, shingle roof); barn or large shed; outhouse | 1 (barn, outhouse 2) |
| Structures | Water-pump windmill (turning), well, wooden footbridge, lookout tower (ruined, on the plateau) | 1 windmill, well, bridge; 2 tower |
| Roads and paths | Dirt roads painted and carved into the terrain; worn footpaths; edge scatter (pebbles, tufts); signposts | 1 |
| Trees | Oak (2 variants), birch (2), pine (2), apple (orchard), dead tree | 1 oak, birch, pine; 2 the rest |
| Undergrowth | Bushes (3), ferns, reeds (pond), stumps, fallen logs | 1 bushes; 2 the rest |
| Rocks and cliffs | Cliff face kit (4 large pieces), boulders (3), rocks (4), pebble scatter | 1 |
| Props | Rail fence (segment, post, broken), stone wall (segment, end), barrels, crates, cart, hay bales, firewood stack, lamp post with lantern, water trough, laundry line, wheelbarrow, bench, campfire ring, gun rack (tutorial weapon pickup) | 1 fence, barrels, crates, hay, lamp post, gun rack; 2 the rest |
| Ground cover | Grass clumps (3), wildflowers (3 colors), clover patches; PCG scatter per zone | 1 |
| Sky and atmosphere | Warm afternoon sun, sky light, height fog, cheap cloud layer; chimney smoke, birds | 1 lighting; 2 effects |

## Island layout (first draft)

About 200 x 180 m. The tutorial route runs along the road, and every step of it is visible from the one before.
1. **Farmstead (spawn), southwest.** The farmhouse, barn, well, orchard rows and fences. Movement prompts.
2. **Village crossroads, center.** Log cabin and cottage around a small square, signposts, lamp posts, a cart, the gun
   rack with the first weapon.
3. **Target meadow, north.** A fenced field with the target dummies and hay bales, under the windmill on its hill.
4. **Creek and pond, east.** The road crosses the creek on the footbridge; reeds and a small pond; the creek falls off
   the island edge.
5. **Forest grove, northeast.** Oak, birch and pine with undergrowth and rocks; the spiders' ground.
6. **Rocky plateau, southeast.** Raised 8-10 m with cliff faces, reached by a winding path; the ruined lookout gives a
   view over the sky and the floating islands (where the next area will be).

## Performance plan

The frame on Medium today is 12.1 ms (83 fps) at spawn. Measured passes (2026-09-29 capture): ambient occlusion 1.6 ms,
shadow depths 1.5, shadow projection 0.9, post 0.75, base pass 0.7, sky light 0.6, velocities 0.6, TAA 0.4, local lights
0.4, reflections 0.4, motion blur 0.35, clouds 0.4, outline 0.2. Most of it is fixed screen cost, which is where the
savings are:
1. **First, make today's island hit about 7 ms on Medium** (headroom for the new art): SSAO off (AO is baked into the
   art), motion blur off by default, no velocity from wind, a cheaper sky and clouds, two shadow cascades over a shorter
   distance, no outline pass, fewer dynamic lights. Each change measured with `Tools\perf.ps1`.
2. **Budgets for the new art**, checked at several fixed viewpoints per zone (spawn, crossroads, meadow, grove,
   lookout), not just at spawn: GPU at most 7.5 ms, game and render threads at most 7 ms each, about 700 draws.
3. Instancing wherever something repeats (PCG for trees, bushes, rocks, grass), cull distances by size, shadows only
   from things big enough to matter, LODs on vegetation.
4. Every import step is measured; anything that breaks the budget is fixed before the next step.

## Work breakdown

The main session coordinates, owns the editor and integrates. Helper agents work only in Blender and on files in
their own folders; they never open the Unreal Editor, never commit, and export test FBX files with
`Tools\models.ps1 -NoImport -Out Intermediate\ArtExport_<Agent> -Only <Model>`.

| Owner | Work | Folders |
|---|---|---|
| Main session | Performance (Medium to about 7 ms first), texture and LOD support in the exporter and importer, the master materials, the layout, integration, lighting, tests | `Source/`, `Config/`, `Content/`, `Tools/` (except below), `Docs/` |
| Textures agent | `looter_textures.py`, the tileables, the house trim sheet, leaf and bark textures, a contact sheet | `Tools/Blender/looter_textures.py`, `Art/Textures/` |
| Buildings agent | Houses, barn, outhouse, windmill, well, bridge, tower | `Art/Models/Buildings/` |
| Vegetation agent | Trees, bushes, undergrowth, grass and flowers | `Art/Models/Vegetation/` |
| Rocks and props agent | Cliff kit, boulders, rocks, fences, walls, props | `Art/Models/Rocks/`, `Art/Models/Props/` (new files) |
| Terrain agent | The terrain generator and macro color map from `layout.json` | `Art/Levels/TutorialIsland/`, `Art/Models/Terrain/` |
