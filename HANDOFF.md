# Handoff: the chosen art style, tutorial island, HUD and gun ideas

The user made four decisions in a cloud session from 2026-10-02 to 2026-10-04, and plans to have them built starting
Monday, 2026-10-05, when their usage resets. Nothing in `Content/`, the materials, the level, the HUD's code or the
weapons' code has changed for any of them yet. `CLAUDE.md` imports this file, so every session starts with it. When all
four are built, delete this file and that import, and record the outcome in `Docs/Plan.md`.

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
3. **The HUD upgrade** (chosen 2026-10-03). The user found the gameplay HUD simple and bland. They asked for a more
   personable HUD that wows, keeping the key concepts of the earlier HUD rounds, and gave a reference: an RPG unit
   frame with a portrait in an ornate diamond, the name, a thick health bar with numbers, a level gem and an
   experience bar. The user liked the mockup and asked for these changes over four rounds, all made in it:
   - the ammo cartridge 15% smaller and the gun icons about 20% bigger;
   - the tutorial prompt moved off the top of the screen, where its long sentence read badly, into a mission tracker
     on the left after a second reference, Borderlands 4's quest tracker. The first version followed it too closely,
     so the tracker is now drawn in the HUD's own metalwork;
   - the ammo counts inside the cartridge, and the cartridge standing upright on the right of the weapon slots, which
     stack in a column with slot 1 on top;
   - the player frame 15% smaller, with no name on it.
4. **Three gun ideas** (chosen 2026-10-04). Asked for ideas the game doesn't have yet, the user picked three of thirteen
   for the loot: **notches** (each gun counts its kills and wakes at milestones), **part swapping** at a gunsmith's
   bench, and **cursed irons** (a strong perk with a real drawback). See "Three gun ideas". Another idea from the same
   list, ember powers, waits on a demo the user asked to see first.

## Where everything is

- **Branch `claude/island-concepts`** holds all of it: the art-style docs and the scripts that rendered the styles,
  the concept viewer, the concept doc, the exact placements, the HUD mockup's sources, the portrait's art and this
  file. It sits on top of `main` at `cf34a3c`:
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
- **The HUD mockup** is <https://claude.ai/artifact/MjHmSiZunHXm2MqxCqhmJ1>, private to the user's account. Press Play
  on the new HUD and the buttons under the screen trigger every state; Fire also counts the tracker's objective, so
  its tick and the next step show too. The buttons only demonstrate the HUD and are not part of it. Beside it are the
  current HUD, redrawn at the same moment, and the portrait's reactions side by side. Its sources are in
  `Docs/HudMockup/`: `NewHud.dc.html` holds every size, colour and timing below, except that its player frame is drawn
  at 85% of the sizes its rules give (the numbers here are the final ones). `__BG__`, `__RIFLE__` and the like stand
  for the backdrop render and the Inked icons. The portrait's art is `Art/Icons/HudPortrait.svg`.

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

## Building the town in the game

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

## The HUD upgrade in brief

It keeps every gameplay-HUD rule in `CLAUDE.md`:

- built in C++ with the `LooterUI` kit, in its orange, cyan and Chakra Petch;
- no backing panels, only floating outlined text and slanted bars;
- `MarkBackground` on every background, so the UI transparency setting fades only those;
- vector art drawn by the kit, the portrait included, and the Inked weapon and ammo icons;
- the corner clusters' idle fade.

Sizes are in pixels at 1080p, measured from the screen's top-left.

**The player frame** (bottom-left) replaces the health ring and the experience bar at the bottom centre, which stays
empty. The user had it drawn 15% smaller than at first, and the sizes below are the final ones. It shrank toward its
bottom-left corner, so the horn's point stays 18 px from the left edge and the lower clamp's tip 27 px from the bottom.
It shows no name: the user had a name plate over the health bar removed.

- **Medallion.** A diamond 146 px tip to tip, centred at (113, 974).
  - A gunmetal bezel lit from the top-left: the upper half `#76818e` to `#2c333b`, the lower half `#2e353d` to
    `#111418`, a 2.7 px `#0e1116` edge, a light line inside the upper edges and a dark one inside the lower.
  - A window 128 px tip to tip, edged in `#0e1116`, with a 1.4 px cyan (`#5ac8ff`) hairline inside.
  - Orange (`#ff9f1c`) chevron clamps over the top tip and under the bottom tip, and a gunmetal horn on the left tip
    with an orange chevron inlay.
- **Portrait** in the window, at 0.64 px a unit of `Art/Icons/HudPortrait.svg`: a masked gunslinger in the Inked
  style, with eyes glowing cyan under the hat brim and a red bandana, on dark glass with faint scanlines.
- **Health bar.** 459 × 27 px from (130, 940), leaning 16° like every HUD bar, its left end tucked behind the medallion.
  - A 1.7 px `#0e1116` rim, and a gunmetal bezel (`#66717e` to `#15191e`) with a lit top line.
  - A dark track (`#051018` at 88%, a background) inset 3.4 px. It starts 34 px in, so an empty bar still begins at
    the medallion's edge.
  - The fill in Health red, in three flat bands: the top 36% `#ff8f80`, then `#ff5b4a`, the bottom 28% `#c63e2f`. Over
    it a 45° hatch (black at 13%, 5 px apart) and a light leading edge (`#fff3ef`).
  - The pale chip (`#ffe1db`), dark cuts at the quarters as on creature bars, and an orange "]" clamp over the far end.
  - The number in the middle: the health in white 18 px, then "/ 100" in `#d6e4ee` 12 px.
- **Level gem.** A diamond 37 px tip to tip, centred on the medallion's lower-right edge at (161, 999). It is cyan in
  two flat halves (`#bdeeff` over `#4ab5ee`), with a 2.4 px ink edge and the level in ink, 15 px.
- **Experience bar.** 323 × 10 px from (185, 994), in ten sections 2.5 px apart, each a tenth of the level as now. The
  sections are cyan halves (`#b9ecff` over `#45b4ee`) on a `#051018` track with a `#0e1116` edge, and "1,240 / 2,000 XP"
  sits at (518, 989). The fill is cyan now, not orange.

**Reactions** (timings from the mockup):

- **Calm.** Blinks every 5.2 s (the eyes squash to 8% for about 0.15 s) and breathes (the bust bobs 1.4 px over 4.2 s).
- **Hit.**
  - The portrait shakes ±4 px for 0.32 s, the window flashes red (`#ff3b2e`, from 60% to nothing over 0.45 s), and
    the eyes squint for 0.6 s.
  - The bar drops at once. The lost part stays as the chip for 0.45 s, then drains in 0.6 s.
  - The screen's edges flash red over 0.55 s.
- **Low health** (30% or less, as now). The squint holds. On a 0.9 s beat, the window pulses red (10% to 34%), the
  fill brightens and the screen's edges pulse. The number turns `#ffd9d3`.
- **Heal.** The fill rises over 0.5 s with a pale green shine sweeping along it, and "+30" in `#6dff7a` rises at the
  bar's end.
- **Experience.** As now: the stretch just earned shows white, holds, then fades (1.5 s), and "+160 XP" rises over the
  numbers.
- **Level up.**
  - The eyes flare, then fade over 1.8 s. The gem flashes, and a ring spreads out of it (to 2.6 times its size over
    1.1 s).
  - A **banner** replaces the message plate's "LEVEL UP!", centred 170 px from the top. A 92 px cyan gem holds the new
    level, with a soft glow and eight orange rays behind it.
  - Under the gem, "LEVEL UP" in orange 46 px, letter-spaced 12, between fading cyan rules, and "MAX HEALTH +8%" in
    `#9fe0ff`.
  - It pops in, holds and fades over 2.8 s.

**Weapons** (bottom-right), rebuilt as a column at the user's request: the slots stack with slot 1 on top, and the
cartridge stands upright on their right. The cluster keeps its corner, 48 px from the right edge.

- **Slots.** The same 64 px circles, now built in layers: a `#0e1116` rim, a gunmetal ring 2.5 px wide, the inner disc
  (a background), a cyan hairline and the rarity arc along the bottom 100°. They stack 76 px apart, centred on x 1775
  at y 812, 888 and 964.
  - Each has its key tab, a small chamfered plate 22 × 16 px, 8 px to its left: orange with a dark number for the gun
    in hand. The Inked icon of the ammo the gun takes (20 px) sits 8 px left of the tab, dimmed like the gun unless it's
    in hand.
  - The gun in hand moves 9 px left, toward the screen's centre, instead of up. It still grows 1.14 times, with its
    accent ring and a soft orange glow.
- **Gun icons about 20% bigger** (the user's request). The rifle is 56 px wide instead of 46, the shotgun 58 instead of
  48, still tilted 22° up; their ends now reach the ring.
- **Cartridge** 15% smaller than today's and standing upright, tip up: 51 × 196 px from (1821, 800), 14 px right of the
  slots, its base level with slot 3's bottom (`UHudMagazineWidget::Width` and `Height` become 51 and 196).
  - The fill drains from the tip down as the gun fires, with faint marks across it at a half and three quarters.
  - Inside, by the base, the counts stack, centred: the rounds in the magazine in white 23 px, and under them the
    reserve, "/120", in `#cfe2ef` 14 px, both outlined dark. The reserve turns red at 0, as now.
  - Its outline is doubled, dark under light, and the fill has two tones, the hatch and a light leading edge.
  - The colour rules stay: cyan, orange at a quarter left, a red count and a beating outline when empty, and an orange
    reload fill rising from the base.
  - The big ammo icon beside the cartridge goes: the slot in hand shows its ammo at full strength.
- **Under it**, right-aligned to the cluster's edge: the fire mode in `#8fb3cc` 13 px at y 1000, with the status
  ("RELOADING", "[R] RELOAD", "NO AMMO") in orange 12 px on the same line to its left. Below them is the gun's name in
  its rarity's colour, 18 px, ending in a small rarity gem at the edge.

**Mission tracker** (left). It replaces the tutorial prompt at the top centre, so the top of the screen stays clear
except for the boss bar. It shows the tracked mission (`UMissionSubsystem::GetTracked`), so it serves every mission,
not just the tutorial, and it stays up during boss fights. Its block starts at (36, 286), about a quarter of the way
down, clear of the minimap and the pickup feed. The user's first version followed Borderlands 4's tracker too closely
(a diamond badge, a dark strip fading out, a checkbox), so this one is drawn in the HUD's own metalwork.

- **Medal.** A ranger's star in a ring, 32 px across, centred at (53, 303): a `#0e1116` rim, a gunmetal ring, a dark
  glass disc (a background) with a cyan hairline, and an orange five-point star (10.4 px to its points, 4.5 px to its
  inner corners) with a 1.5 px ink edge and a light line along its upper-left edges.
- **Title.** The mission's name in caps, white, bold 17 px, letter-spaced 1.6, from (80, 283).
- **Step bar.** Under the title, 208 × 10 px from (83, 310), leaning 16° like the bars, with a section for each step,
  3 px apart: the steps done in the experience bar's cyan halves, the current one in orange halves (`#ffc06a` over
  `#ff9f1c`), the rest dark. Its track is `#051018` at 80% (a background) with a `#0e1116` edge. The step, "4 / 6",
  follows it in `#8fb3cc` 12 px at (298, 305). Missions of one step show no bar.
- **Route line.** A 1.2 px cyan (`#5ac8ff` at 80%) line over a dark one runs down from under the medal, from (53, 321)
  to (53, 344), and turns right to end 8 px before the objective.
- **Objective row** from (82, 332): an orange chevron 16 px across with a 1.6 px ink edge, then one short line in white
  18 px, 9 px after it, and the count after that in `#9fe0ff` ("2 / 5") when the objective counts more than one.
- **Key hint** from (107, 362), for steps that teach a key: the key as a keycap, then what it does in `#8fb3cc` 14 px
  ("[R] Reload"). A keycap is a dark plate (`#24465e` to `#0e2433`, a background) with the top-right and bottom-left
  corners cut 6 px, a 1.4 px cyan (`#5ac8ff`) edge and the key in white, here 22 px high. It shows the key the player
  bound, like the prompt's `{Reload}` does today.
- **Behaviour** (timings from the mockup):
  - On a count the number pops: 1.35 times its size and white, settling over 0.3 s.
  - When the objective is done, its chevron becomes a cyan tick over a dark line, the words dim to `#8fb3cc` and the
    step's section turns cyan. After 1.4 s the next objective slides in from 14 px to the left, fading in over 0.45 s,
    and the next section turns orange.
  - The tutorial's closing line ("You're ready. Explore the island...") shows as a last, ticked objective for as long
    as the prompt shows it today. Then the tracker fades out, or shows the next tracked mission.
  - It steps aside while a menu is open, as the prompt does, except on the inventory step. It takes no part in the
    idle fade.
- **Short lines.** The tracker gives each tutorial step a short line, like the reference's "Reach Carcadia
  outskirts", and the key moves into the hint. The mission's name, "Welcome to Skyreach", is the tracker's title, in
  caps, so step 1 drops it.

  | Step | Today's prompt | Tracker line | Hint |
  |---|---|---|---|
  | 1 | Welcome to Skyreach. Move with {Move} and look around with the mouse. | Move and look around | {Move} Move |
  | 2 | Hold {Sprint} to run. Follow the road to the village. | Follow the road to the village | {Sprint} Hold to run |
  | 3 | Grab the rifle on the gun rack: look at it and press {Interact}. | Grab the rifle from the gun rack | {Interact} Take it |
  | 4 | Shoot the target dummies in the meadow under the windmill. {Reload} reloads. | Shoot the target dummies, 0 / 5 | {Reload} Reload |
  | 5 | Spiders nest in the woods past the pond. Hunt down two of them. | Hunt spiders past the pond, 0 / 2 | none |
  | 6 | Press {Inventory} to see your loadout and your weapons' stats. | Check your loadout | {Inventory} Inventory |

  `Docs/Story.md` says Skyreach "keeps its six neutral steps and their wording". The six steps and their neutral words
  stay: the full sentences remain the objectives' text, which the Missions page shows, and only the tracker shortens
  them. Update that line of `Docs/Story.md` when building it.

**Elsewhere:**

- **Minimap.** The same map and size, in a gunmetal bezel (radius 86 to 95 px).
  - Ticks every 30° (longer at 90°) turn with the view, and the N sits in an orange-ringed disc on the bezel.
  - A fixed orange notch marks the top, and a cyan hairline runs inside the bezel.
  - Under the map are the place (15 px) and the island (10.5 px, dim), for example CROSSROADS over SKYREACH.
- **Boss bar.** 640 × 30 px at the top centre, built like the health bar: bezel, red fill, chip and end clamp.
  - The boss's level sits in a gem of its rank's colour.
  - The phase cuts light up once passed, and the phase name flashes orange when a new phase starts.
- **Crosshair.** It kicks to 1.45 times its size for 0.16 s on each shot. The hit marker, pickup feed and interaction
  prompt stay as they are.

## Building the HUD

- **Player frame.** `UHudVitalsWidget` becomes the player frame (`UHudPlayerFrameWidget`): medallion, health bar, gem
  and experience bar, at the final sizes above, with no name. It keeps its chip, flash and low-health logic.
  `UHudXPBarWidget`'s gain flash, catch-up and level-up announcement move into the frame's experience bar.
- **Portrait.** A new `UHudPortraitWidget` draws the portrait and its reactions.
  - The art becomes vector data drawn into textures once, like the Inked icons: an `Art/Icons/HudPortrait.py` beside
    `InkedIcons.py`, flattening the SVG's cubic curves.
  - `FInkedIcon` has three tones, but the portrait needs a colour per shape: either give the rasterizer a colour per
    shape, or stack single-colour `IconBrush` layers.
  - The three eye layers swap by opacity, and the glows are soft round brushes.
- **Level-up banner.** A new banner widget takes over the level-up line from the message plate (`OnAnnouncement`).
- **Mission tracker.** A new `UHudMissionTrackerWidget` replaces `UTutorialPromptWidget`, which `ATutorialDirector`
  adds to the viewport itself today. Delete the prompt and update `CODEMAP.md`.
  - It reads the tracked mission from `UMissionSubsystem` and listens to `OnMissionsChanged`.
  - `FMission` holds one string for the objective today: the words with "(2/4)" appended by `GetTrackerText`. The
    tracker needs the parts: the short line, the count and how many are needed (`FormatProgress` gives "2 / 5"), the
    step and the number of steps, and the hint. `UMissionRunner` fills them where it writes the line now
    (`MissionRunnerEvents.cpp`).
  - `UMissionObjective` gets two optional fields: the tracker's short line (empty: its `Text`), and the hint, an
    action whose bound key shows as a keycap plus its words.
  - The tutorial plays `DA_Mission_Tutorial`, which `Tools/Unreal/create_mission_assets.py` makes to match
    `ATutorialDirector`'s built-in steps, and `Looter.Missions.TutorialMission` compares the two objective by
    objective. Give both the short lines and hints from the table above (`FTutorialStep` gets the two fields, and
    `TutorialDirectorMission.cpp` copies them), and turn the count on for the dummies and the spiders, which hide it
    today (`bShowCount` off). Extend the test's comparison to the new fields, and run the script in the editor to
    rebuild the asset.
  - `ATutorialDirector` stops making the prompt. It hands its closing line to the tracker through `ALooterHUD`, and
    the tracker keeps the prompt's menu rule (`SetSuppressed`).
  - The medal, chevron and tick are vector art drawn by the kit (`IconBrush`). The step bar's track, the medal's glass
    disc and the keycaps' plates call `MarkBackground`; the text, star, ring, route line, chevron, tick and the
    sections' fills stay solid.
- **Other widgets that change:**
  - `UHudWeaponSlotsWidget`: the ring layers, the bigger gun icons, and a column instead of a row (slot 1 on top),
    with the tab and ammo icon on each slot's left and the gun in hand moving left instead of up;
  - `UHudMagazineWidget`: 51 × 196 and upright, its vector shapes drawn turned a quarter, with the counts stacked
    inside by the base (`SetMagazine` takes the reserve);
  - `UHudMinimapWidget`: the bezel, ticks, notch and place name;
  - `UHudBossBarWidget`: built like the health bar.

  `PlayerHUDWidget.cpp` places them all. Its weapon cluster becomes the slots' column and the cartridge side by side,
  over a line with the status and fire mode and the gun's name under that. `ReserveText` (with `ReserveWidth` and
  `ReserveGap`) and the big `AmmoClassIcon` go.
- **Colours.** New colours go into `LooterUI::Color` (the gunmetal tones, the gem's cyans, the health bands), never as
  literals in a widget.
- **Performance and tests.** Measure on Medium with `perf.ps1` before and after: the frame adds a handful of cached
  vector images and a few animated layers. Keep the `Looter.*` tests passing.
- **Docs.** When it's done, update `CLAUDE.md`'s UI rules to describe the player frame, the portrait and the mission
  tracker, and `Docs/Story.md`'s line on the tutorial's wording.

## Three gun ideas

All three work on `FWeaponInstanceData`, the rolled gun that a session saves (the loadout and backpack in
`FWeaponInventorySave`, and loot on the ground with each map's world), so every new field travels with its gun. The
numbers are starting points to tune in play.

**Notches.** Every gun counts its kills, and they show as notches cut into it.
- A creature's death counts for the gun that dealt the killing blow (`UHealthComponent::GetLastDamageCauser`), where
  `UPlayerProgressionSubsystem::AwardKill` already gives the kill's experience. Like experience, only creatures count, so
  the dummies can't be farmed. The count is a new `Kills` field on `FWeaponInstanceData` and stays with the gun when
  it's dropped, stashed or picked up again.
- The stock shows them as tally marks, four cuts and a slash, one cut per 5 kills, until the row is full at 25 marks.
  `M_Gun` draws them from a `Notches` parameter through a mask strip on the stock parts' UVs (the body on a gun whose
  stock slot is empty), so no new meshes.
- At milestones the gun wakes:
  - 50 kills, **Blooded**: +3% damage.
  - 250, **Named**: +3% more, and a nickname from a list for its kind, in quotes after its name (WHISPER BULLPUP
    "WIDOWMAKER").
  - 1,000, **Soul-forged**: +4% more (10% in all), and a faint soul-light in its rarity's colour stays on it in the hand.
  - A message says so as it happens ("WHISPER BULLPUP IS BLOODED · +3% DAMAGE"). The loot card and the loadout's stats
    card show the count ("137 NOTCHES").

**Part swapping.** At a gunsmith's bench the player scraps guns for parts and fits parts onto guns of the same kind.
- **Where.** Ozias Penhallow's bench in the hub. He joins after the *Gilded Lily* (`Docs/Story.md`), so the bench opens
  then; see the questions.
- **Scrapping.** Scrapping a gun keeps one part of the player's choice in a parts box, saved with the session as (gun
  kind, slot, key). The rest of the gun is gone.
- **Fitting.** A part from the box goes onto a gun of the same kind, in the same slot, and the part it replaces goes
  into the box, so trying parts costs nothing but the guns scrapped. Once grave gold exists, a fitting costs a small
  fee.
- **Rules.**
  - The gun's rarity must allow the part (`MinRarity`), and the part's needs must be met (`Requires`). A new barrel
    moves the muzzle by its `Length`, as on a rolled gun.
  - The gun keeps its seed, rarity, level, paint and notches. The new part's numbers fall where the gun's seed puts them
    in the part's range, as for the parts it rolled with, so moving a part back and forth never rerolls it.
  - The word in the gun's name follows the parts' `NamePriority`, so a swap can rename the gun.
  - Named guns (Heirloom) and the Hollow's signature legendaries have fixed parts: they can't be scrapped or changed.
- **Saving.** `FWeaponInstanceData::Parts` already holds a gun's parts by key, so fitting a part changes one key. Never
  rename or reuse a key (`CLAUDE.md`).
- **The screen.** A bench screen in the LooterUI kit: the gun on a stand as in the loadout, its slots down one side,
  and for the chosen slot the box's parts that fit, each with its stat changes against the current part.

**Cursed irons.** A few guns drop cursed: a strong perk with a real drawback.
- **How often.** 6% of the Rare, Epic and Legendary guns that drop roll a curse, from the gun's seed with its rarity
  (`UWeaponRollLibrary`). Common and Uncommon guns, named guns and signature legendaries are never cursed.
- **How it shows.** The rarity colour stays the game's one colour code. A cursed gun's label and cards add a
  cracked-coin glyph and the curse's name, the cards spell out both the perk and the drawback, and its loot beam
  gutters like a dying flame.
- **The curses** (perk; drawback). They live in a table of their own, a CSV beside the parts lists, keyed by names that
  never change, like part keys.
  - **Hungry**: +30% damage; each reload costs 3% of max health.
  - **Greedy**: kills with it roll their loot at +0.5 luck; each shot spends 2 rounds.
  - **Restless**: +40% fire rate; recoil doubles.
  - **Cold**: +20% damage and +25% critical damage; no sprinting with it in hand.
  - **Grasping**: kills heal 5% of max health; max health is 15% lower with it in hand.
  - **Unlucky**: critical hits deal triple damage; one shot in eight misfires.
- **Lifting a curse.** The drawback goes and the perk stays, so a cursed iron is worth carrying through its curse. It
  lifts by itself at 100 notches, or at once with Tilly's grave salt once her shop and grave gold exist.
- **Saving.** Two new fields on `FWeaponInstanceData`: `Curse` (its key, none when uncursed) and `bCurseLifted`.

**Building them.**
- Perks and drawbacks that are plain numbers change the gun's `FWeaponStats`. The others hook in where they act: firing
  (misfires, rounds per shot), reloading (the health cost), sprinting, health and the loot roll.
- Tests: `Looter.Weapons.Notches` (counting, milestones, saving), `Looter.Weapons.Curses` (the roll's rate over many
  seeds, the effects, lifting) and `Looter.Weapons.PartSwap` (the rules, names and seeds, and the box surviving a save).
- Update `CODEMAP.md` for new files, and tick the boxes in Phase 9 of `Docs/Plan.md`.

## Suggested order

1. Merge `claude/island-concepts`, then regenerate the style images.
2. Screen Print Wash steps 0 to 2: before pictures, docs, then the post-process. The whole game takes the new look at
   once, and the town is then built under it.
3. Crossroads Town with the models that exist: placements, roads, gameplay groups, scatter and tour views. Measure.
4. The HUD upgrade: the player frame and portrait first, then the mission tracker, the weapons, minimap, banner and
   boss bar. It doesn't depend on the rest; building it after step 2 means its colours are checked over the new
   pastel world.
5. Screen Print Wash steps 3 to 7: flat plates, masters, foliage and terrain, light, then guns and creatures.
6. The new models, in the print style, then the sheep, hens and townsfolk with their bestiary pages.
7. Screen Print Wash steps 8 to 10, with the final tour and performance record.

The gun ideas don't depend on any of these. Build notches first (curses lift by them), then cursed irons, then part
swapping, which needs its bench and screen; their cards use the new HUD's look, so after step 4 is best.

## Questions for the user

- Should all eight spiders live in Web Hollow, as the concept shows, or stay spread through the woods?
- Should the townsfolk and livestock start as set dressing that stands and idles, or as living characters?
- Should part swapping wait for Ozias, who joins after the *Gilded Lily*, or should a plain workbench offer it from
  the start?
