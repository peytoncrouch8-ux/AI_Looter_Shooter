> **Brought in from the cloud session on 2026-10-07, for future use.** This is `HANDOFF.md` as the cloud session left it
> on branch `claude/island-concepts` (commit `740a649`, 2026-10-05), unchanged below this note. None of it is built
> yet. The files it points to were brought in too, at the user's request (2026-10-07): the Screen Print Wash docs and
> scripts, `crossroads_town.json`, the concept viewer, the HUD mockup, ember demo and hero page sources, the heroes'
> backlog models, and `Docs/Plan.md`'s phases 6-11. Where this file says `HANDOFF.md`, read this file. Unlike on the
> branch, `CLAUDE.md` neither imports it nor carries the branch's "Working with the user" rules (the user: "not yet").
> Decision 1 (Screen Print Wash everywhere) predates the stylized-realism art the user approved for Ransom's Rest from
> 2026-10-05 on; the user will decide later which look wins, so build none of decision 1 until they do. The images
> (style targets and sheets) were never on GitHub: `Docs/Art/REGENERATE_IMAGES.md` makes them again.
# Handoff: the art style, tutorial island, HUD, gun ideas, ember powers and heroes

The user made six decisions in a cloud session from 2026-10-02 to 2026-10-05, and plans to have them built starting
Monday, 2026-10-05, when their usage resets. Nothing in `Content/`, the materials, the level, the HUD's code, the
weapons' code or the character's has changed for any of them yet; the heroes' models exist only as Blender scripts in
the backlog. `CLAUDE.md` imports this file, so every session starts with it. When all six are built, delete this file
and that import, and record the outcome in `Docs/Plan.md`.

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
   bench, and **cursed irons** (a strong perk with a real drawback). See "Three gun ideas".
5. **Four ember powers** (chosen 2026-10-04). Another idea from the same list: each outlaw's ember leaves Ellis a
   power. The user played a demo of six and kept **Lucky Streak**, **Dust Devil**, **Slag Bomb** and **Spyglass**, for
   the main session to revise. With decision 6 they became four of the heroes' powers, and the outlaws' embers stopped
   giving powers. See "Ember powers".
6. **Five playable heroes** (chosen 2026-10-05). The user asked for five playable characters to choose from, like
   Borderlands' Vault Hunters, each unique and with three ember powers to choose from, modeled to replace the
   mannequin. Asked first, they chose Ellis plus four new heroes (the story stays Ellis's, and the four get their own
   reasons to hunt the gang), the four kept powers spread among the heroes with eleven new ones, and a mixed cast. They
   approved all of it: **Ellis Ransom** the Revenant, **Odessa Lark** the Cardsharp, **Hollis Crane** the Unpaid,
   **Gauge** the Iron Hand and **Wendell Pike** the Surveyor, with their looks, stories and powers. See "Five heroes".

## Where everything is

- **Branch `claude/island-concepts`** holds all of it: the art-style docs and the scripts that rendered the styles,
  the concept viewer, the concept doc, the exact placements, the HUD mockup's sources, the portrait's art, the ember
  demo's sources, the heroes' models, the hero page's sources and this file. It sits on top of `main` at `cf34a3c`:
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
- **The ember powers demo** is <https://claude.ai/artifact/MeXAwqdJdX4SYGDzhEy4xR>, private to the user's account. It
  is playable in Crossroads Town with the game's models, the print look and the new HUD: "Watch all six" shows each
  power, and "Play" starts a spider raid. Its sources are in `Docs/EmberDemo/`: `game.js` holds the powers' numbers,
  timings and effects, and `ui.html` the sockets, the ember page and the HUD around them (`__RIFLE__` and the like
  stand for the Inked icons). They run on the concept viewer's engine (`Tools/ConceptViewer/web`). The demo also has
  the two powers the user passed on, Raise the Flock and Landslide.
- **The hero page** is <https://claude.ai/artifact/5NVdW7yTzReFkP3wvaxbyg>, private to the user's account: the five
  heroes on stands, in the print look. Click one, or press 1 to 5, and they step forward with their story and powers;
  drag to turn them; Idle, Walk and A-pose show the rig at work. Its sources are in `Docs/HeroSelect/`: `heroes.js`
  holds the stories, the fifteen powers with their glyphs (SVG, vector art for `IconBrush`) and the poses, `ui.html`
  the screen's layout, and `build.py` exports the heroes from their scripts and assembles the page on the concept
  viewer's engine, in a few seconds (`python3 Docs/HeroSelect/build.py`, with the `bpy` module).

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

## Ember powers

**Where they come from.** In the demo, Ellis earned a power from each outlaw's ember and carried two, on Q and F. With
the heroes (decision 6), each hero has three powers of their own instead, and the outlaws' embers no longer give
powers. The four powers kept from the demo went to four heroes: Dust Devil to Ellis, Lucky Streak to Odessa, Slag Bomb
to Gauge and Spyglass to Pike. This section holds those four's numbers and what all fifteen powers share; the eleven
new ones are under "Five heroes". The numbers are the demo's, starting points to revise in play. The demo's rifle
deals 14 a shot, so the powers' damage below is against that, and it grows with the player's level the way a gun's
does (`UWeaponDefinition::DamagePerLevel`).

**The four powers kept from the demo.**
- **Lucky Streak**, Odessa's (Fortune). Cooldown 22 s.
  - For 8 s, every shot that hits ricochets into the nearest other creature within 12 m, for 10.
  - One shot in three is a critical hit wherever it lands (x1.5, `LooterCombat`).
  - On use a fan of playing cards bursts in front of the player. The ricochets draw gold tracers, and a card flicks off
    each hit.
- **Dust Devil**, Ellis's (Wind). Cooldown 7 s.
  - A dash of 9 m in 0.3 s the way the player is moving, or forward when standing still. Nothing hurts the player
    during it.
  - Creatures within 2.6 m of the path take 15, are flung 4 to 5 m to the side and left behind, and stay dizzy for
    1 s once they land.
  - Speed lines, the field of view widening 14 degrees and settling, a ring of dust where the dash starts and a smaller
    one where it ends. Flung creatures tumble and show dizzy sparks.
- **Slag Bomb**, Gauge's (Forge), lobbed from its firebox. Cooldown 12 s.
  - A bomb lobbed onto the creature under the crosshair, or else where the crosshair meets the ground, at most 20 m
    away.
  - It bursts for 34 within 2.5 m and 18 within 4.5 m.
  - It leaves a pool of slag 4.5 m in radius for 6 s. Whatever stands in it burns for 7 every 0.5 s, and for 0.6 s
    after it leaves.
  - The bomb trails sparks, and the burst is a flash, a ring and flying clods. The pool is orange with hot yellow blobs
    and a cooling crust, under rising sparks and pale smoke. Burning creatures are tinted orange.
- **Spyglass**, Pike's (Sight). Cooldown 18 s.
  - For 10 s, every creature within 60 m is marked, through walls: an orange silhouette and a marker over it with its
    distance.
  - Marked creatures take 30% more damage.
  - A cyan ring sweeps out over the ground to 60 m with a lens vignette, and each creature is marked as the ring
    reaches it.

**The HUD.** The power sockets sit at the bottom centre, the spot the HUD upgrade leaves empty: one for each power the
hero carries (the demo showed two; see the questions).
- Each socket is a 92 px gunmetal ring, built like the weapon slots, around the power's glyph (vector art through
  `IconBrush`). Two sockets stand 188 px apart, with the key's tab under each and the power's name under that.
- While a power cools down, a dark pie covers its socket and drains, with the seconds left in white. While a power
  lasts, an arc in its colour runs down round the ring. When it's ready, the socket gets an orange ring and a slow glow
  in its colour. Pressing the key too early shakes the socket.
- On use, the power's name pops up over the sockets in its colour.
- The colours go into `LooterUI::Color`: Lucky Streak `#ffcf4a`, Dust Devil `#e8f6ff`, Slag Bomb `#ff8a1c`, Spyglass
  `#ffb24d`, and one for each new power.

**The ember page.** It is a fourth inventory page after Missions (`EInventoryPage::Embers`), in the LooterUI kit. It
shows the hero's three powers, each with its glyph, name, nature, what it does and its cooldown, and lets the player
choose the power, or powers, the hero carries.

**Keys.** Q and F are free in play. The loadout screen uses them for its own drop and hold, but only while it is open.
- Add a rebindable action, "Ember 1", to `UKeyBindingSubsystem` under Combat, on Q, and "Ember 2" on F if a hero
  carries two.
- On a gamepad, use two buttons the weapon context leaves free.

**Building them.**
- **The component.** A `UEmberComponent` on the character holds the powers carried, the cooldowns and what is
  active, and uses the powers. It binds its keys with `FPawnInputBinding`.
- **The data.** A `UEmberDefinition` data asset per power (`DA_Ember_<Id>`) holds its key, name, hero, nature, glyph,
  colour, cooldown, duration and numbers. Each power's behaviour is in C++.
- **Saving.** The session saves the hero and the powers chosen (`ULooterSessionSave`). Ember keys never change, like
  part keys.
- **Creatures.** For the four, `ACreatureBase` needs four new states:
  - knocked: flung through the air like the slime's launch, landing with `FindGround`;
  - dizzy;
  - burning: the slag's ticks and an orange tint;
  - marked: the silhouette and 30% more damage taken.
- **The new powers** add slowed creatures (Sundown, Grave Ward), a drag (Toll Chain), a trip (Tripwire: a short knock,
  then dizzy), a cone of card shots (Fifty-Two Pickup), a piercing spike (Rail Spike), delayed damage on the player
  (Borrowed Time) and shot modifiers in `UBulletSubsystem` (Double or Nothing, Long Shot).
- **Lucky Streak's ricochet** goes through `UBulletSubsystem`. While the streak lasts, a hit on a creature sends a
  second bullet from the hit to the nearest other creature.
- **Spyglass's silhouette** draws the creature's custom depth with a stencil value of its own, because the print
  look's stencil keeps leaf cards out of its line pass. The post-process draws the silhouette where the creature is
  hidden behind something. The marker goes over the creature's tag (`UCreatureHealthBarWidget`).
- **Damage** uses a new `UEmberDamageType`. A power's kills give experience (`AwardKill`) but count for no gun's
  notches.
- **Testing.** `Looter.Hero <id>` switches the hero, and `Looter.Embers.Use <id>` fires any power.
- **Tests.** `Looter.Embers.*` covers cooldowns, choosing, saving, and each power's effect on a test creature.
- **Performance.** The effects are particles, rings and one stencil silhouette. Measure on Medium with `perf.ps1`.
- **Docs.** `Docs/Story.md` changes with the heroes (see "Five heroes"). Update `CODEMAP.md`, and tick the boxes in
  Phase 10 of `Docs/Plan.md`.

## Five heroes

**The idea.** The player picks one of five heroes, as in Borderlands, and each hero has three ember powers to choose
from. The user's answers before the models were made: Ellis plus four new heroes, Sexton raising five (the story stays
Ellis's, and the other four get their own reasons to hunt the gang); the four powers kept from the demo go into the
heroes' sets, with eleven new ones; a mixed cast, mostly human, with stranger ones that fit the world. Ellis is "they"
in all text, as `Docs/Story.md` says.

**The five.**

| Hero | Height, build | Look |
|---|---|---|
| Ellis Ransom, the Revenant | 1.80 m, medium | A cattleman hat; a red bandana over nose and mouth, eyes burning cyan in the brim's shadow; a long dusty duster open over a dark waistcoat; a belt of brass cartridges with a cross-draw holster; Pa's lantern at the right hip |
| Odessa Lark, the Cardsharp | 1.72 m, slight | A flat-crowned gambler hat with a playing card in its band; a wine-red tailcoat cut away at the waist with long tails; a gold brocade waistcoat with a watch chain; a white high collar and a black cravat; black gloves; tall heeled riding boots; a card case; gold earrings |
| Hollis Crane, the Unpaid | 1.95 m, gaunt and stooped | A marshal dead forty years: grey-green skin, a tarnished coin over the left eye and cyan light in the right socket, a drooping white mustache, long white hair under a battered slouch hat; an ember glowing through a tear in his shirt; a long tattered slate coat and a ragged shoulder cape; a tin star; the toll chain from shoulder to hip, with its hook |
| Gauge, the Iron Hand | 1.92 m, massive | The railroad's clockwork track-layer: iron plates and brass bands, a boiler chest with a pressure gauge for a heart, a firebox glowing in its belly, two smoking stacks, domed pauldrons, one headlamp eye, a grille mouth and a steam whistle, three-fingered riveted hands |
| Wendell Pike, the Surveyor | 1.68 m, stocky | A brown bowler with brass goggles; round spectacles; grey mutton chops joined to a big mustache; a teal neckerchief; a khaki field coat full of pockets under a cartridge belt; puttees; a huge pack with a folded tripod, a map tube and a bedroll, and a scoped rifle beside it; a brass spyglass at the hip |

**Their stories**, as the page tells them:
- **Ellis:** shot on Ransom's Point by the Dunne Gang and raised by Sexton to bring the stolen embers home. Pa's
  lantern still burns at the hip.
- **Odessa:** a riverboat gambler who caught Lucky Ned dealing seconds, and caught his bullet for it. Sexton bought her
  marker.
- **Crane:** marshal of a town whose saint went dark, forty years walking. One eye still wears the ferryman's coin.
  Sexton promised him a fare.
- **Gauge:** the railroad's track-laying engine, woken in the wreck the gang left behind by a spark of ember sealed in
  its firebox.
- **Pike:** he walked every mile the railroad will ever run, until the gang left him at the bottom of a ravine. He
  still knows the way.

**Their powers.** ★ marks the four from the demo, whose numbers are in "Ember powers". The new powers' numbers are
starting points, and their cooldowns are set in play.
- **Ellis:** ★Dust Devil (Wind); **Sundown** (Dusk): for 6 s the world slows to a crawl round Ellis, who moves and
  shoots at full speed; **Borrowed Time** (Debt): for 8 s the damage Ellis takes goes into Sexton's ledger instead, to
  be paid back afterwards, and every kill strikes some off.
- **Odessa:** ★Lucky Streak (Fortune); **Fifty-Two Pickup** (Fortune): she flings the whole deck, a cone of razor cards
  that cut whatever they meet; **Double or Nothing** (Risk): for 6 s every shot deals double damage or misses entirely,
  at even odds.
- **Crane:** **Toll Chain** (Debt): the chain hooks a creature and drags it to his feet, stunned; **Grave Ward**
  (Mercy): he plants his lantern, and for 8 s creatures in its light are slowed while he mends inside it; **Last
  Rites** (Judgment): he marks a creature; if it dies within 8 s its soul bursts on its pack, and if not, the mark
  strikes it hard.
- **Gauge:** ★Slag Bomb (Forge), lobbed from its firebox; **Iron Hide** (Forge): its plates lock, for 60% less damage
  over 6 s at a slower pace, and biters burn their teeth; **Rail Spike** (Steam): a spike driven from the forearm
  through every creature in a line.
- **Pike:** ★Spyglass (Sight); **Tripwire** (Craft): a wire strung between two stakes; whatever crosses it trips,
  takes a hit and lies stunned; **Long Shot** (Sight): for 8 s his shots hit harder the farther they fly, and marked
  creatures are always critically hit.

**The models.**
- Each hero is a Blender script in `Art/Backlog/Characters/` (`Ellis.py`, `Odessa.py`, `Crane.py`, `Gauge.py`,
  `Pike.py`) built on `Tools/Blender/looter_heroes.py`; each docstring holds the hero's look and powers. The backlog
  keeps them out of the game. To bring one in, move its file to `Art/Models/Characters/` and run
  `Tools\models.ps1 -Only <Name>`.
- Each exports through `looter_export.py` as `SK_Hero_<Name>`: one skinned mesh of 17k to 22k triangles.
- **The rig** has the UE5 mannequin's deform bones by name and hierarchy, 63 of them, in its A-pose: pelvis, spine_01
  to spine_05, neck_01, neck_02, head, the clavicles, arms and hands with every finger, the thighs, calves, feet and
  balls. It has no twist or IK bones.
- **Animation.** Each hero has its own skeleton. Make an IK Rig for it (Unreal builds one from the mannequin's names)
  and an IK Retargeter from the mannequin. Then either drive the hero at runtime from the hidden mannequin that already
  plays the game's animations (`Retarget Pose From Mesh`), or retarget the animations once into each hero. Give each
  skeleton the mannequin's weapon socket, and check the hands on the guns.
- **Facing.** Like every model, a hero faces Blender's -Y, which becomes Unreal's +X. The mannequin faces +Y, so a
  hero's mesh doesn't take the mannequin's -90° yaw on the character's mesh component.
- **Materials.** One flat-colored material per palette color, 10 to 19 per hero. Merge each hero's into one when
  bringing it in, and give it the print look with everything else (Screen Print Wash).
- **First person.** The view shows the hero's own arms, so check each hero's sleeves and hands in first person.
- **Portraits.** The HUD upgrade's player-frame portrait (`Art/Icons/HudPortrait.svg`) is Ellis's. The other four each
  need one in the same Inked style.

**In the game.**
- **Choosing.** A new session starts at a hero select screen in the LooterUI kit, laid out like the page: the five on
  stands, a roster of cards, and a panel with the hero's name, role, story and powers. A session keeps its hero, so
  playing another hero means another session.
- **The data.** A `UHeroDefinition` data asset per hero (`DA_Hero_<Id>`) holds the mesh and its retargeter, the name,
  role, story, colour, portrait and three powers (`UEmberDefinition`). `ALooterCharacter` takes its mesh and powers
  from the session's hero. Hero keys never change, like part keys.
- **Saving.** The session saves the hero's key with the powers chosen (`ULooterSessionSave`).
- **Tests.** `Looter.Heroes.*` checks that every definition is complete, its mesh and retargeter load, its powers
  exist, and the choice survives a save.
- **Story.** Add the five to `Docs/Story.md`: their stories, Gauge's waking, and how the story speaks to a hero who
  isn't Ellis (see the questions).
- **Performance.** A hero replaces the mannequin at about the same cost once its materials are merged. Measure on
  Medium with `perf.ps1`.
- Update `CODEMAP.md`, and tick the boxes in Phase 11 of `Docs/Plan.md`.

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

The heroes and their powers also come after step 4: the player frame shows the hero's portrait, and the power sockets
sit in the new HUD's empty bottom centre. Bring the five in and retarget them first, then the hero select and saving,
then the powers: the shared systems with the four from the demo, then the eleven new ones. Make their effects in the
print look.

## Questions for the user

- Should all eight spiders live in Web Hollow, as the concept shows, or stay spread through the woods?
- Should the townsfolk and livestock start as set dressing that stands and idles, or as living characters?
- Should part swapping wait for Ozias, who joins after the *Gilded Lily*, or should a plain workbench offer it from
  the start?
- Does a hero carry one of their three powers at a time, as the hero page shows, or two, on Q and F, as the ember demo
  had?
- Are all three of a hero's powers open from the start, or do they unlock as the hero levels?
- When the player picks someone other than Ellis, how does the story speak to them? Ellis could stay its centre as a
  companion, or its lines could change for each hero.
