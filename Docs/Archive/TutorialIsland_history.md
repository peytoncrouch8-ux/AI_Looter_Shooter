# Tutorial island: history

Moved verbatim from `Docs/TutorialIsland.md` when it was condensed (2026-10-13). Never load by default. Superseded where
marked; the live doc holds what still holds.

## The doc's old status note (superseded: Crossroads Town round 1 was built 2026-10-08; the tutorial is retired, 2026-10-13)

> **A new layout is planned.** On 2026-10-03 the user chose a new layout for the island, Crossroads Town (concept 1 of
> `Docs/TutorialIslandConcepts.md`; plan in `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`). Until it is built, this
> document describes the game as it is.

The user approved a new art style on 2026-09-30 (mock-up: `Docs/Art/StyleTarget_Outpost.png`) and asked to rebuild the
tutorial island in it: Skyreach stays a grassy meadow floating in the sky, with rustic houses, roads, varied trees,
reshaped terrain (hills, cliffs, a road network) and an experience worth remembering. Other areas come later.

Hard targets:
- **120 fps at 1080p on the Medium preset** on the reference PC (RX 580, i7-8700): 8.3 ms a frame, at the heaviest view
  on the island, not just at spawn. High and Epic keep Lumen and Nanite and may cost more.
- Quality and efficiency first: few materials, shared textures, instancing, sensible triangle counts, nothing
  generated while the game runs.

Art bible intro: "What the mock-up shows, and every asset follows:"; Readability: "the tutorial route is legible from the
spawn".

`M_Water` was listed as "`M_Water` (later): opaque, cheap, stylized." (It is built now.)

## Asset list (tutorial island) (the 2026-09-30 rebuild's work list; done)

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

## Island layout (first draft) (superseded in part by Crossroads Town, round 1, 2026-10-08; `layout.json` is the truth)

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

## Performance plan (as written 2026-09-29; step 1 done 2026-09-30, 48dccdc: Medium 12.1 -> 6.7 ms, `Docs/Performance.md`)

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

## Work breakdown (the 2026-09-30 rebuild's agents; today's split: `Docs/Pipeline.md`, "Who does what")

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
