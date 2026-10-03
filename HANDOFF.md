# Handoff: the chosen art style and the chosen tutorial island

The user made two decisions in a cloud session on 2026-10-02 and 2026-10-03, and plans to have them built starting
Monday, 2026-10-05, when their usage resets. Nothing in `Content/`, the materials or the level has changed for either
yet. `CLAUDE.md` imports this file, so every session starts with it. When both are built, delete this file and that
import, and record the outcome in `Docs/Plan.md`.

## The decisions

1. **The art style is Screen Print Wash** (chosen 2026-10-02). It replaces textured "stylized realism"
   (`Docs/Art/StyleTarget_Outpost.png`) everywhere in the game. The user found the old look bland. From eight styles
   and six mixes rendered on the cottage (`Docs/Art/StyleExploration/README.md`), they picked style 10, a mix of
   style 1 (ink and flat color, like Sable) and style 4 (gouache storybook). The full spec and a ten-step transition
   plan are in `Docs/Art/ScreenPrintWash.md`; the plan's boxes are Phase 6 of `Docs/Plan.md`.
2. **The tutorial island's layout is Crossroads Town** (chosen 2026-10-03), concept 1 of the four in
   `Docs/TutorialIslandConcepts.md`. The user had found the island bland and lonely. The crossroads grows into a small
   town, the slimes and spiders get grounds of their own, and the roads get brick kerbs, cover and planting. The user
   liked the 3D model as it is: build what it shows, fixing only the overlaps listed under "Fix while building".

## Where everything is

- **Branch `claude/island-concepts`** holds all of it: the art-style docs and the scripts that rendered the styles,
  the concept viewer, the concept doc, the exact placements and this file. It sits on top of `main` at `cf34a3c`:
  `git fetch origin claude/island-concepts`, then `git merge origin/claude/island-concepts`. It already contains
  `claude/art-style-exploration-text`; there is nothing else to merge.
- **The images are not on GitHub.** The cloud session's network policy blocks the Git LFS host, so the PNGs (the
  target `Docs/Art/StyleTarget_ScreenPrintWash.png` and the exploration sheets) never got uploaded.
  `Docs/Art/REGENERATE_IMAGES.md` makes them again in about four minutes with Blender 4.5. The user also has them in
  that session's chat. The target is the picture the transition is checked against, so make it first.
- **The concept viewer** is <https://claude.ai/artifact/NoY9cGCEDzz4yB1ZwPb49e>, private to the user's account. It shows
  the island from above and on foot, and "Compare all 4" shows one viewpoint in every concept. Its source and rebuild
  steps are in `Tools/ConceptViewer/README.md`; `conceptCrossroads()` in `Tools/ConceptViewer/web/concepts.js` is the
  layout's source.
- **The exact placements** are in `Art/Levels/TutorialIsland/crossroads_town.json`, written from the viewer by
  `Tools/ConceptViewer/test/dump.js`. It holds 784 placed models, 46 creatures, the roads, fences, hedges, fields,
  ground patches, pools, slime trails, webs, chimneys, vegetation rules and the viewer's camera views. Small, numerous
  things are counted, not listed: 4,135 grass tufts, 4,123 cobbles, 1,083 kerb bricks, 478 cabbages and 421 flowers.
  Its `about` field explains the conventions.

## Screen Print Wash in brief

A flat-color print on cream watercolor paper. Pixel sizes are given at 1600 px wide and scale with the resolution.

- **Fills.** Each surface is one flat fill from a small palette, lifted toward the paper:
  `fill = 0.83 × color + 0.17 × #f6eedc`. Textures carry no detail of their own.
- **Light.** Two tones with a hard edge. The shade is the fill × (0.70, 0.64, 0.86), a violet tone, and cast shadows
  use it too. The shade gets pigment granulation (a soft mottle and sparse dark specks); there is no stipple or
  halftone.
- **Edges and lines.** Fills darken 10 % along their edges, and the color bleeds slightly. There are two line plates:
  a sketchy pencil line (`#4a3f44`, 68 %) offset under a thin 1 px ink line (`#2a2024`, 88 %).
- **Print effects.** The color plate is misregistered 2.5 px right and 1.5 px down from the lines. Cream paper texture
  covers everything.
- **Sky and distance.** The sky is a pastel gradient from `#f6dcb8` at the horizon to `#9ec1ea` at the top. Clouds are
  flat paper-white shapes outlined in pencil. The ground fades toward `#f1dcc0` with distance.
- **The accent rule.** Only what the player should notice is saturated: doors, loot, rarity, interactables, the
  lantern. The teal accent shades to deep teal, not violet. Everything else stays pastel.
- **In Unreal.** One post-process material, `M_PP_ScreenPrint`, is applied from C++ (budget about 0.8 ms on the
  RX 580). Every texture set gets flat plates from `looter_textures.py --style print`, and the masters get a `Print`
  switch. A stencil keeps leaf cards out of the line pass. Steps 0 to 10 in `Docs/Art/ScreenPrintWash.md` give the
  order and what to measure.

## Crossroads Town in brief

Coordinates are Unreal centimeters (X north, Y east), with yaw in degrees from +X: the convention of `layout.json`.

**Buildings facing the square** (their fronts sit about 14 m from the centre):

| Building | Model | Position | Yaw | With it |
|---|---|---|---|---|
| Saloon | `FalseFront_Saloon` | (2321, 622) | 195 | hitch rail and trough in front |
| Cottage | `Cottage` | (574, 1765) | 252 | kitchen garden, washing line |
| Log cabin | `LogCabin` | (-1099, 1512) | 306 | kitchen garden, scarecrow |
| Sheriff | `FalseFront_Sheriff` | (-1887, 401) | 348 | |
| Cottage, scale 0.95 | `Cottage` | (-811, -1591) | 63 | washing line, blue and white flowers |
| Store | `FalseFront_Store` | (413, -2340) | 100 | |
| Undertaker | `FalseFront_Undertaker` | (1543, -1390) | 138 | |

**Houses along the farm road:**

| House | Model | Position | Yaw | With it |
|---|---|---|---|---|
| Log cabin | `LogCabin` | (-2932, -2915) | 135 | garden, washing line, outhouse |
| Cottage | `Cottage` | (-3260, -590) | 301.4 | garden, scarecrow |
| Farmhouse, scale 0.88 | `Farmhouse` | (-1148, -1945) | 116.6 | garden, washing line (moves: see below) |

The homes have a bench and a lamp by the door, a flower bed along the front, firewood at the side, barrels behind,
and smoke from their chimneys. The square is paved with cobbles, 26 m on a side, centred on the crossroads inside the
existing 34 m village pad. In it stand:

- the `TownMemorial` at (0, 0), facing yaw 200;
- a second `Well` (-450, 650), the existing `GunRack` (500, -500) and a `NoticeBoard` (950, -250);
- three market stalls, two benches, a cart, barrels and crates;
- lamp posts at the corners.

**Roads.** The farm road (farm to square, 4.6 m wide) and the range road (square to range, 4.4 m) are dirt with wheel
ruts, a kerb two bricks wide on each side and grass verges. Lamp posts stand every 15 to 17 m, alternating sides. The
forest road stays dirt with ruts, and the paths keep their widths. A hedge runs along the south-east side of the farm
road's first leg and along both sides of the range road past the square. A dry-stone wall runs beside the windmill
path. A barricade, sandbags and crates by the range road, from (3300, -800) to (2600, -650), give cover, and so do
the range's own sandbags and barricade.

**Slimes: the Wallow.** A bog 29 m across, centred on (2400, -5600), where the game's slimes already spawn. It has:

- a darker sward and five pools (the largest 4.2 m in radius) with lily pads, ringed by reeds;
- mossy rocks, broken fences and a signpost;
- a cart sunk 35 cm, tilted 14° and slimed green;
- six mushroom clumps, and three slime trails heading toward town.

The viewer shows five slimes.

**Spiders: Web Hollow.** A clearing strewn with pine needles, 35 m across, centred on (5800, 5600) on the forest
rise. It has:

- a ring of seven `DeadTree_A`, with webs strung between neighbours 1.6 to 4.4 m up;
- two burrows covered by sheet webs;
- four cocoons hanging 2.4 to 3.2 m up, and two clusters of egg sacs;
- the `Outcrop_TorB` and some boulders.

The viewer shows eight spiders.

**Life.**

- Fourteen townsfolk, at the stalls, on the porches, at the well and the notice board, and on the roads.
- Two farmhands at the farm.
- Nine sheep in an 18 by 22 m rail-fenced pasture at (-5900, -100), with a trough and a shed.
- Eight hens at a new coop by the barn.

**Green.**

- 106 trees: oak and birch groves round the town, a pine grove in the north, the trees left round Web Hollow,
  lone trees over the meadows and the orchard's apple trees.
- 141 bushes, three wildflower meadows, reeds round the pond and the bog, and birches along the creek.

**Kept from today's island, as in every concept:** the farmstead (with a new coop, haystacks and a shed), the orchard
filled out to full rows, the windmill, the range (dummies, new targets, hay bales and cover), the plateau with the
lookout, jetty and skiff, and the pond (with a new dock).

## Fix while building

The concept has these mistakes. The JSON keeps them as the viewer drew them.

1. **The farm road's farmhouse runs into the square's south-west cottage.** The farmhouse at (-1148, -1945)
   overlaps the cottage at (-811, -1591) and that cottage's yard: two barrels, a crate and the washing line. Move the
   farmhouse 10 m back toward the farm, to 37 m along the farm road instead of 47 m (`besideRoad(S, FARM_ROAD, 3700,
   ...)` in `concepts.js`). That puts it at about (-1915, -2407), yaw 121.4, clear of every building, garden and prop
   (checked against the footprints). Its yard moves with it.
2. **Two corner lamp posts stand inside porches.**
   - The north-west lamp (1150, -1150) is inside the undertaker's front, with the barrels and crate beside it at
     (1050, -1050), (1150, -900) and (1180, -1180).
   - The south-east lamp (-1150, 1150) is inside the log cabin's front, with a crate.

   Move each group in along its diagonal, to about (900, -900) and (-900, 900): 12.7 m from the centre instead of
   16.3 m. That leaves 1.4 m or more in front of each porch.
3. **Check in the game.** The saloon's hitch rail and trough sit at the edge of its porch. The north-east corner lamp
   stands at the edge of the forest road.

## Building it in the game

Today the island is built in four steps:

1. `Art/Levels/TutorialIsland/layout.json` describes the layout.
2. `Art/Models/Terrain/TutorialIsland.py --computed` builds the terrain and its flat footprints, and writes
   `layout_computed.json`.
3. `Tools/Unreal/build_area.py TutorialIsland` places the models, the gameplay actors and the chimney smoke.
4. `Tools/Unreal/build_island_scatter.py` adds the PCG grass, flowers, trees and rocks.

What changes:

- **Placements** (`layout.json`). The village's `log_cabin` (1300, -1500) and `cottage` (-1100, 1400) give way to the
  town, the `outhouse` moves from (2100, -2300) to (2450, -2500), and `gun_rack` stays where it is. The four false
  fronts need entries in `FOOTPRINTS` and `BUILDINGS` in `Art/Levels/area_shape.py`. `build_area.py` places every
  placement whose kind has an `SM_<kind>` mesh, so props can go in the same list; only kinds listed in `FOOTPRINTS`
  flatten the ground.
- **Roads.** Split `main` into the farm road and the range road, both kerbed. Add a `brick` kind to `ROAD_STYLE`, or
  chain a kerb model the way `FenceRail` chains. Pave the square with cobbles; its yard already keeps the scatter off
  it.
- **Gameplay** (`gameplay.creatures` in `layout.json`). The slimes keep their centre, (2400, -5600), but their spread
  goes from 450 to about 1000 so they fill the Wallow. The spiders would move from anywhere in the `forest` zone to
  centre (5800, 5600) with spread 1250, which is Web Hollow. Ask the user first, because eight spiders in one hollow
  is harder than eight spread through the woods.
- **Scatter** (`Art/Levels/area_scatter.py`, `build_island_scatter.py`). Add the groves, lone trees, bushes and
  wildflower meadows from the JSON's `vegetation` and `placements`, and reeds round the bog. Keep trees out of the
  creature grounds, gardens and pasture (`noTrees`).
- **Tour** (`Art/Levels/TutorialIsland/views.json`). Add the viewer's on-foot views from the JSON's `views.foot`
  (Square, Farm road, Slimes, Spiders) so `tour.ps1` measures them.

**Models that already exist.** These only need the print look, which the transition gives them:

- every building above, and the props: `TownMemorial`, `NoticeBoard` and `HitchRail` from Main Street's kit, plus
  `LampPost`, `Bench`, `LaundryLine`, `Signpost`, carts, hay, firewood, barrels, crates, troughs, fences and stone
  walls;
- `DeadTree_A`, rocks, outcrops, cairns, lily pads and reeds (`Reeds_A`, `Reeds_B`);
- the trees (`Oak_*`, `Birch_*`, `Apple_A`, `Pine_*`), bushes (`Bush_*`), grass and flowers (`GroundCover.py`), and the
  chimney smoke (`SmokePlume`).

The viewer drew its own trees, bushes, reeds, grass and flowers only because it bakes one flat color per face and has
no texture masks, so the game's leaf cards would show as solid quads. Use the game's.

**Models to make.** These are the viewer's `k:` kit pieces. Make them in the print style from the start.

- Town and farm: market stall (3), shed (3), hen coop, haystack (2), scarecrow (3), picket fence for the gardens,
  cabbage rows, hedge, barricade, sandbags, practice targets (3) and a pond dock.
- Ground: brick kerbs and cobbles; flowers in pink, red and blue (`GroundCover.py` has only yellow, white and
  purple).
- Creature grounds: mushrooms, burrows, cocoons, egg sacs, webs (strung between trees, and sheets over burrows), slime
  trails and bog pools.

**Creatures to make:** sheep, hens and townsfolk (the viewer has four looks). Each needs a model, a C++ actor class and
a bestiary page (`DA_Bestiary_<Name>`, per `CLAUDE.md`). The viewer has them stand and idle; ask the user before
giving them more behavior.

**Performance.** The town adds about ten buildings and a few hundred props round the square. Measure on Medium before
and after with `perf.ps1`, and with `tour.ps1` including the new Square view. The budgets are in
`Docs/TutorialIsland.md`: 8.3 ms at the heaviest view, and 4k to 8k triangles a house.

## Suggested order

1. Merge `claude/island-concepts`, then regenerate the style images.
2. Screen Print Wash steps 0 to 2: before pictures, docs, then the post-process. The whole game takes the new look at
   once, and the town is then built under it.
3. Crossroads Town with the models that exist: placements, roads, gameplay groups, scatter and tour views. Measure.
4. Screen Print Wash steps 3 to 7: flat plates, masters, foliage and terrain, light, then guns and creatures.
5. The new models, in the print style, then the sheep, hens and townsfolk with their bestiary pages.
6. Screen Print Wash steps 8 to 10, with the final tour and performance record.

## Questions for the user

- Should all eight spiders live in Web Hollow, as the concept shows, or stay spread through the woods?
- Should the townsfolk and livestock start as set dressing that stands and idles, or as living characters?
