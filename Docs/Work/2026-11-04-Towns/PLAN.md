# Towns and farms of Ransom's Rest: the plan (TOWNS lane, 2026-11-04; read-only research, nothing built)

The user's calls (2026-11-04 grill round): after the sharpening round, art goes to "Terrain and towns"; the towns lack all four of
**dressing and clutter**, **ground detail**, **building variety**, **dusk and night lighting**. New art is script-made in our sets (no new
Tripo); Tripo models already in stay and get sharpened; the user judges art in playtests; no per-asset budgets, but 120 fps on Medium
(8.3 ms, 1080p, RX 580) is a hard line for every view, measured per change.
Sources read: `CLAUDE.md`, `Docs/Pipeline/Levels.md`, `Assets.md`, `Docs/Areas/RansomsRest.md` (+ expansion section 9),
`Art/Levels/RansomsRest/{layout,town,views}.json`, `make_town.py`, `Tools/Unreal/build_area_dressing.py`, `build_area_town_rr.py`,
`build_area.py house_lights()`, `World/HouseLights.*`, `World/InstancedProps.h`, the model scripts' docstrings, `Saved/RRTownProbe.json`
(2026-10-14), `Docs/Work/2026-11-04-Art/{CONTRACT,PERF}.md`, the art audit, tour CSVs `Saved/ArtPreviews/Step1/tour_*.csv`, the shots
`Saved/ArtPreviews/Step1/before/{MainStreet,MainStreet_Dusk,Farm,Depot,WhitlockFields,ChapelOverTown}_Medium.png`, look sheet 35.
Frames: layout cm, X north, Y east. Main Street runs along Y at X 0 from Y -1300 (gate) to 7200 (yard road); its zone is 36 x 83 m.

## 1. What is there (measured)

**Buildings (town = Main Street, Pen Lane, Boot Hill's dugout, the undertaker's yard, the depot: 17).**
- Main Street 11: north side (X > 0) Assay front Y 0, Sheriff 1200, Home_A (Haskett, lived) 2480, Saloon 3400, Hotel front 4420,
  Home_H (Mercer, lived) 5600; south side Home_J (shack, empty) 250, Barber front 1485, Store 3000, Home_D (manager, shut) 5200,
  Undertaker 6000. Then Pen Lane Home_C, Home_F (both empty), dugout Home_E (lived), workshop, coffin shed, depot (+ water tower,
  coal stage, well, memorial). Farms: Ransom (Tripo farmhouse and barn, well, outhouse), Whitlock (barn, Tripo windmill, Home_G).
- **Model repeats: 0.** Every home model is built once (Homestead.py rule); only Well x2 and outhouse x2 repeat. RR_Home_B (dogtrot,
  lived) and RR_Home_I (L house, empty) are built and imported (probe: their sockets are listed) but **placed nowhere**.
- **What repeats is the finish:** 10 of the 11 Main Street roofs and 12 of the 15 town roofs are the same shake strip (HouseTrim E)
  in the same grey-brown (Home_J planks, Home_G battened boards, Home_E sod are the exceptions); weathered siding (strip A) on most
  walls; one window casing, one door, one fieldstone chimney kit; floors all at 0.38 m; 1-2 storeys; the back lots hold only 2
  woodsheds, 2 lean-tos, 1 privy and the coffin shed (ChapelOverTown shot: bare dust behind the north side).

**Dressing (town.json 112 pieces + build_area_dressing GROUPS).**
- Main Street zone: 78 town.json pieces = 28 structural (13 walk pieces, 3 awning, 9 lamp posts, 3 sign brackets) + 50 clutter.
  Clutter per 10 m bin per side, gate to east: north 0/2/2/5/7/2/8/5, south 0/1/5/2/5/2/4/0 = **3.1 per 10 m of one side**, and a
  third of it is back-yard (washtubs, chopping blocks), not frontage. The street bed (10 m x 85 m) holds 9 lamp posts and 4 hitch
  rails, nothing else: no wagon, crate, crossing or fire. Plus GROUPS: gate trough 3, saloon and store backlots 12, Landfall
  loudspeaker posts 3, the Company sandbags at the gate 3.
- Pen Lane: 1 piece (a barrel) and the stock pens' fence. Undertaker's yard: 13 + workshop 4 + yard trough 2. Depot: 9.
- Ransom farm: 10 town.json pieces + 33 hay bales + 6 barricade stand-ins + 2 picket lines; the farmyard pad is bare dirt.
  Whitlock: 3 pieces at Amos's house + 4 round bales + 3 fence lines; the barn yard is empty (no wagon, trough, tools).
- Repeated meshes: LampPost x10 identical (9 on Main Street), Barrel_A x9 (+5 in groups), FlowerBox x6, Boardwalk_4m x6 + 2m x2 +
  Steps x5 (one plank pattern), HitchRail x6, Bench x4.

**Ground.** One tiled place pattern (GroundPlaces R "dust", 3.6 m tile, straight ruts along U) over the macro colour; Main Street is
in `macro.roads.skip`, so the road term's curved ruts never reach it; **0 decals, 0 puddles, 0 mud**; worn trails 5 (farm 3,
workshop 1, dugout 1); the scatter keeps every layer out of each footprint box, so every wall stands in a bare ring with no grass at
its foot; boardwalk planks identical every 4 m. About 45% of the MainStreet frame (rows ~590-1080) is that uniform street.

**Dusk lights (unshadowed point lights, AHouseLights).** Main Street: 9 street lamps (0 cd by day, 8 at dusk, reach 650), 2 porch
lanterns (0/4, 350), house lamps sheriff 1.5/6 r300, Haskett 3/16 r400, Mercer 3/16 r400, store 3.5/30 r350, undertaker 1.75/5.5
r175 = **16 lights**; lit buildings 5 of 11 (6 boarded or empty, dark by the story). Town total 8 of 17 lit (+ depot 7 lamps,
workshop 5, dugout, farmhouse, Whitlock barn 2). **0 fires** in the valley. The 4 roadside lantern posts are emissive only.
- **Bug: the town is dark in the story's dusk.** `AHouseLights::ApplyState` lights only when the state == "Dusk"
  (`HouseLights.h:52`); `WindowLifeSubsystem.cpp:35,184` and `AmbientAirRules.cpp:54` name "Dusk" too. Main 6 and Main 7 (until
  Saint Ada wakes) run **DuskSmog** (`build_area_deck.py`), so every street lamp sits at 0 cd and the windows at day glow then.
  Main 1 to Main 6 run **Smog** (the choked golden hour, exposure 0.2): lamps off as by day. True "Dusk" shows only in the cold
  open (on the Point) and the end of Main 7. There is no night state.

**Perf of the town views now (tour 10:36-10:39, Medium):** MainStreet 8.30 ms (1,801 draws, GPU 7.02), MainStreet_Dusk 8.29
(1,702), Farm **8.58** (1,745), ChapelOverTown 8.06, WhitlockFields 7.90, DepotWaiting 7.06, Depot 6.90; Aerial 9.36 (the town from
above). Day 8.30 vs Dusk 8.29 on one camera: today's 16 lamps cost under the noise, but dusk's cheaper shadows (60 m) hide their
share, so it is unmeasured. From 10-14: lights pass 0.34-0.37 ms at heavy views, buildings 0.1-0.2 ms (240-370 draws), dressing
0-0.3 ms. **There is no margin:** every towns step must be paid for in the same views (section 5).

## 2. What we compare against

Plain words: the frontier towns of big-budget open-world westerns and the hub towns of big looter shooters, walked at eye level.
What they show (design judgement; our targets):
- **Dressing:** 5-9 props at eye level per 10 m of frontage in 2-3 clusters (stacks of 2-3 heights, a sign, a seat, a tool, notices
  on the wall); one "someone was just doing this" story per building; the street bed used (parked wagons, a crossing, a fire, a
  dropped crate) with a clear lane; back lots full of outbuildings, lines, woodpiles, junk.
- **Ground:** it changes every 2-5 m: ruts with standing water, churned mud at rails and troughs, planks over mud, straw and chips,
  worn aprons at doors and steps, drip lines under eaves, weeds at every wall foot, post and walk edge, boards of mixed age.
- **Buildings:** a block of six shows 4+ roof finishes, 3+ roof shapes, mixed heights, additions (lean-tos, porches, sheds), 2-3
  chimney kinds; no two neighbours alike; every business signed.
- **Dusk and night:** every occupied building shows 2+ lit panes and lamplight on its porch; lamps every 12-20 m pooling light on the
  ground; 1-3 open fires per street; far lights are emissive only; dark gaps where houses stand empty (mood, kept).

## 3. The gaps, ranked by how much the player feels them

1. **Ground detail.** In every town frame (~45% of the MainStreet shot): one dust tile, no water, mud, wear or weeds; 0 decals, 0
   puddles, bare rings at 17 buildings' feet. 0 of the targets above.
2. **Dressing and clutter.** 3.1 pieces per 10 m (target 5-9), the street bed empty, Pen Lane 1 piece, back lots ~12 pieces over
   ~5,000 m2, Whitlock's yard empty; the same lamp x10, barrel x14.
3. **Building variety.** No model repeats, but 12 of 15 town roofs share one shake strip and tint, so the town reads as one kit from
   the street, Boot Hill and the chapel; 2 built homes unused; no outbuildings or second row.
4. **Dusk lighting.** Seen in Main 6-7 (DuskSmog), where it is broken: 0 of 16 town lights on. Less exposure than 1-3 but the
   cheapest fix, so it goes first.

## 4. The plan per area

### 4.1 Dusk lighting (lane LIGHTS; first)
- **States:** `AHouseLights` takes three levels instead of one `DuskState`: Day (today's), **Gloom** (Smog, SmogThin: windows glow 4,
  the hall lamp at its day value, street lamps 3 cd, porch lanterns 2: the town lights its lamps against the smog; the user's call,
  D1) and **Dusk-like** (Dusk, DuskSmog: today's dusk values). One helper `LightingStates::Kind(FName)` (in `World/LightingStates.*`)
  used by HouseLights, WindowLife and AmbientAir, so a new state is added in one place. A house that sets nothing behaves as today.
- **Which sockets are lit when** (lit = the lamp at its level and the WindowGlow slot raised):

  | Building | Sockets | Day | Gloom | Dusk-like |
  |---|---|---|---|---|
  | Lived homes A, H, E, B, new lived (K, M, O), farmhouse | Light, Window_n, WindowGlow; farmhouse Light_Porch | lamp 2-3, glow 1 | glow 4 | today's dusk; + porch lamp 4 cd r350 |
  | Store, undertaker, sheriff | Light, WindowGlow | as now | glow 6 (open after Main 3) | as now |
  | Depot, workshop, Whitlock barn | their Light_* tables | as now | as now | as now |
  | Boarding house (new) | Light, Window_1..6 | glow 1 | glow 4 | lamp 6 r400, glow 14 |
  | Empty or boarded (assay, saloon, hotel, barber, C, D, F, G, J, I, L, N, P) | none | dark | dark | dark (story) |
  | Street lamps (LampPost, _B, _C), porch lanterns | Light | hidden | 3 / 2 cd | 8 / 4 cd |
  | Town fires (new `ATownFire`) | Fire | per spot (D3) | lit | lit |

- **Fires:** `World/TownFire.h/.cpp` (new): the `AWatchFire` recipe without its interaction: crossed glow cards (M_FX_Glow, one
  instanced draw), a soft halo card, one shadowless 2400 K point light 3-5 cd reach 450 tagged Flicker, a faint smoke wisp (M_Smoke),
  the sound `World.WatchFire.Burn` (or a new script-made `World.TownFire.Crackle`), shown by an `FStoryCondition`. Spots (D3): the
  Company's barrel fire at the gate's sandbags (while the Company holds the gate), the smithy's forge, Delia's wash kettle in the
  farmyard, the bathhouse boiler's fire door. At most 4 in the valley, 2 in any view.
- **Cost control:** no shadows (a cube shadow map is ~0.3-0.5 ms each on Medium); no light functions (they unbatch a light into its
  own pass; flicker stays intensity-only near the player, `ULightFlickerSubsystem`); no IES; `bAffectTranslucentLighting` off on
  every town light (the smoke plumes need no lamplight); `MaxDrawDistance` 4500 + fade 1000 on street, porch and fire lights (past
  it the glass's emissive and a halo card carry the read: one instanced halo set, shown in Gloom and Dusk-like only); a light at 0 cd
  is hidden (`SetVisibility(false)`), so day lamps cost nothing; reach <= 650 (street), <= 450 (porch, house, fire).
- **Count:** the worst view (MainStreetMid_Dusk) has ~16 lights within 60 m today; the plan adds porch lamps on 3 lived homes + the
  boarding house + 2 fires + 2 Pen Lane lamps = **<= 26 in any view, <= ~14 shading at once** inside the 45 m draw distance.
  Estimate from the 10-14 split (0.34-0.37 ms for ~16 lights): 0.01-0.03 ms per batched unshadowed light on screen, so +0.1-0.25 ms
  at dusk in the worst town view before savings; measured first (step 0) with `Looter.Perf.HideFolder Dressing/Lights`.

### 4.2 Building variety (lanes GROUND for the looks, BUILDINGS for the models, DRESSING for the lots)
1. **Re-finish all 17 town buildings with zero draws:** M_World gets a static switch `TrimRecolor` (on MI_HouseTrim only): a texel
   whose UV0 V lies in strip E (0.375-0.5) takes the roof tint, strip A (0.875-1.0) the siding tint, H1/H2 swap on a flag; the tints
   come from the actor's custom primitive data (indices 0-7; the default changes nothing, so every prop on HouseTrim looks as now)
   times the instance's per-instance custom data (AInstancedProps gains `NumCustomDataFloats` and per-instance values, so stacked
   crates and barrels vary too). ~10-15 ALU on HouseTrim pixels, no geometry change, no re-import. Tints (linear multipliers; a lerp
   to a colour for whitewash): roofs Silver (0.90, 0.94, 1.0), Honey (1.15, 1.0, 0.78), Dark (0.68, 0.66, 0.64), Moss (0.85, 0.92,
   0.74); siding Bleached (1.15, 1.12, 1.05), Brown (1, 1, 1), Tar (0.45, 0.43, 0.42), Whitewash (lerp 0.6 to cream 0xe6dac2).
   `Art/Levels/RansomsRest/town_looks.json` (new, GROUND) holds each building's looks; `Tools/Unreal/build_area_town_looks.py` (new,
   GROUND) sets them after the build. **Neighbour rule:** consecutive buildings on a side differ in roof (tint or kit) AND siding AND
   porch; facing buildings differ in 2 of 3; the script refuses a table that breaks it and prints the pairs.
2. **Place RR_Home_B and RR_Home_I** (built, unused) on new lots (4.4): 2 more house types for no art.
3. **Home kits** (`Tools/Blender/looter_homes.py` + `Homestead.py`, BUILDINGS): a home = body + roof kit + porch kit + chimney kit +
   paint; each kit a builder function on today's parts, so new homes are combinations, never copies:
   - Roofs: **R1** silver shakes (today's strip E), **R2** honey shakes with 2-4 replaced courses (strip E, another lane, lighter),
     **R3** board-and-batten roof (Home_G's builder, tarred or grey), **R4** rolled tar paper (PaintBlack over planks, lath battens
     every 0.9 m, torn corners). Sod stays Home_E's. Grey galvanized sheet modelled as geometry on MI_MetalWorn_Windmill (0xb8b5ac),
     never the trim's loot-coloured tin strip F, only on outbuildings: the user's call (D4).
   - Porches: **P1** stoop (stone or plank steps, a door hood on scroll brackets), **P2** shed porch (square posts, plank deck, no
     rail), **P3** turned porch (turned posts, baluster rail, hip or shed roof, an L-wrap option), **P4** gable portico (two posts, a
     bench). Each has a "slack" state for empty houses (a post down, sag, boards missing: today's C and I damage).
   - Chimneys: **C1** fieldstone (today), **C2** stovepipe with an elbow (IronBlack), **C3** clay-plastered (trim G).
   - New homes (6; 6-10 m wide, 5-9 m deep, 4-8 m tall, 4-9k triangles, HouseTrim + the shared tints, WindowGlow and Window_n on the
     lived ones):

     | Home | Body (builder from) | Roof | Porch | Chimney | Paint | State |
     |---|---|---|---|---|---|---|
     | RR_Home_K | 1.5-storey side gable (A's, no sweep) | R2 | P3 | C1 | whitewash, oxide trim | lived |
     | RR_Home_L | shotgun, 2 rooms deep (C's) | R4 | P1 | C2 | weathered, teal door | empty |
     | RR_Home_M | hall-and-parlor, 1 storey (A's) | R1 | P2 | C3 | bleached, teal trim | lived |
     | RR_Home_N | board-and-batten cabin + lean-to (G's) | R3 | P4 | C2 | tar black, cream trim | empty |
     | RR_Home_O | narrow 2-storey (D's, one bay) | R2 | P2 | C1 x2 | cream and oxide | lived |
     | RR_Home_P | one log pen + a frame addition (B's) | R1 | P3 short | C1 | chinked logs | empty |

4. **New town buildings** (`Art/Models/Buildings/TownBuildings.py`, new, BUILDINGS; looter_homes, looter_buildings, looter_town on
   HouseTrim, StoneWall, WoodPlanks tints, IronBlack, MI_Canvas; Nanite with a full fallback; UCX hulls; pivot = front door at the
   ground like the homes; Rye lettering, invented names):
   - **RR_Smithy_Town** 9 x 7 m, ridge 5.5 m: an open-fronted smith and wheelwright under R3; a stone forge with hood and chimney
     (SOCKET_Fire, SOCKET_Smoke), an anvil on a stump, a quench tub, racked wheels, a tyre bench; board-and-batten on a hewn frame.
   - **RR_FeedStore** 10 x 7 m, 6.5 m: a frame warehouse on a 1.0 m loading dock (2.5 m deep, two steps), a sliding door on a track,
     a hoist beam, FEED & GRAIN painted on the gable; R4. Faces the yard road (freight from the depot).
   - **RR_BoardingHouse** 11 x 8 m, 8.5 m, 2 storeys: a ROOMS blade sign, an outside stair to the upper door, 8 sashes (6 lamplit:
     Window_1..6), P2, C1 + C2, whitewashed siding; lived (D5).
   - **RR_Bathhouse** 6 x 5 m, 4 m: BATHS . LAUNDRY, a boiler with a tall stovepipe (SOCKET_Smoke as steam), a wash bench; R4.
5. **Outbuildings** (`Art/Models/Buildings/Outbuildings.py`, new, BUILDINGS; small: Nanite off, LODs 50/25, instanced through
   AInstancedProps at WALL_CULL, UCX): **RR_Shed_A** 3.0 x 2.4 x 2.8 m gable shed (board-and-batten, R1); **RR_Shed_B** 2.4 x 1.8 x
   2.3 m lean-to shed (R3); **RR_Privy_B** 1.8 x 1.4 x 2.4 m two-holer (R4); **RR_Smokehouse** 2.2 x 2.2 x 3.0 m log smokehouse with a
   vent cap (SOCKET_Smoke, faint); **RR_RootCellar** 2.5 x 3.0 x 1.4 m sod mound with a plank door frame (Hay, GroundDirt, the dugout's
   mound). Every home lot gets 1-2 (with the existing Woodshed, ChickenCoop, RR_Outhouse).

### 4.3 Ground detail (lane GROUND; PROPS for the strips)
**Pick: terrain material layers for everything flat, instanced mesh strips for everything with height, no decals.**
- Why: terrain layers follow the ground exactly (no floating edges on the 2-4% slopes), cost 0 draws, run only inside a branch within
  60 m and inside the town windows, get crisp edges from procedural thresholds over a soft mask, and the same data thins the scatter's
  grass (PlacesScatter), so ground and grass agree. Decals rejected for bulk: each is its own draw in a DBuffer pass, never
  instanced; ~150 needed = +8% draws on frames already at 1,700-1,800 draws and 8.3 ms; they can't thin grass. Mesh strips only where
  height shows (weeds, planks, straw piles, fallen boards).
- **The wear term (M_Terrain):** `Art/Levels/area_places.py` (GROUND) also writes `Art/Textures/RansomsRestPlaces/T_RansomsRestWear_M`
  (2048 px, three windows: the town X -3200..2400, Y -1600..14800 at ~8 cm/px in 768 x 2048 px; the farm X -5600..-1800,
  Y -7400..-4400 and Whitlock X -7800..-5600, Y 2800..4800 at ~6 cm/px in the rest). Channels: **R trodden** (packed bare earth,
  lighter, rougher; grass share 0 in PlacesScatter); **G wet** (0-0.4 damp: albedo x0.7, roughness 0.6; 0.4-1 standing water where
  noise > 1 - G: albedo x0.35, roughness 0.06, a flat normal, a 15-30 cm damp rim; Medium has no SSR (`r.ReflectionMethod` 0), so
  puddles read by roughness against the sky and captures); **B litter** (straw, chips, hoof-churn: the thatch pattern at 0.5 m,
  straw-tinted); **A drip line** (a 25-35 cm pebbled dark band 0.4-0.7 m out under eaves: the stony pattern, darker). Static switch
  `WearOn` on the 4 RR core `*_Steep` MIs only; `WearWindows` (3 rects + atlas offsets); `WearReach` 60 m (a dynamic branch, as the
  PlaceMap's); one Texture2DSampleGrad read; ~60-90 instructions inside the branch (M_Terrain today 815 instructions, 12 samplers;
  -> 13). `Tools/Blender/ground_preview.py` gets the term (the numpy port, kept in step).
- **Painted by rule, never by hand** (from placements, model sockets and bounds, town.json pieces): door aprons 1.6 x 1.2 m (R); step
  feet (R); hitch rails 3.5 x 2 m (R + B, G 0.3 churn); troughs, pumps, wells 1.5-2 m (G 0.6-1: a puddle); washtubs and laundry lines
  (G 0.4); woodpiles and chopping blocks 1.5 m (B chips); desire lines 0.6 m wide, curved, from back doors to privies, woodsheds and
  coops (R; the layout's `trails`, extended by patch); the eaves of every building (A: footprint + overhang from the probe); Main
  Street's 6-10 puddles, 0.8-2.5 m, in the ruts at low points (from TERRAIN's flow and low-point maps, section 6; until they come,
  every 12-18 m in a rut at the street's lowest cross-section).
- **Main Street's ruts:** take mainStreet out of `macro.roads.skip` (the road term draws curved ruts, a second pair 1.9 m out via
  `RoadRuts.w`); drop the place pattern's own rut strength inside the town zone so ruts don't double (check in ground_preview first).
  Wheel tracks into the yards (the feed store's dock, the smithy, the farmyard, Whitlock's barn) as paint-only `track` roads (D6,
  section 6).
- **Mesh strips** (PROPS models, DRESSING places): **WeedFringe_A** (short dry grass, lived houses) and **_B** (tall dock, mullein,
  thistle, empty houses), 2.0 x 0.45 m, 0.25-0.8 m tall, and **_C** (a post ring 0.6 m): FoliagePalette swatches (GrassDry, Straw,
  DryTan, Stem) on M_WorldFoliage, wind by vertex colour, no collision, no shadow, cull 4000 (24 m on Medium); along every wall foot
  outside doors (+-1 m), porches and walks, at both ends of each walk's rim, at fence posts. **PlankCrossing_A** (3 boards 1.2 x
  2.4 m) and **_B** (2 boards + a broken one, 1.0 x 3.0 m) over ruts and mud at the Pen Lane mouth, the fields path mouth, before the
  store, at the yard road (4-6): WoodPlanks + GroundDirt edges, flat, no collision. **StrawLitter_A** 1.6 x 1.2 m (Hay, flat,
  shadowless) at rails, barns, coops. **FallenBoards_A** 2.2 x 1.0 m (HouseTrim A, flat) in empty yards. **Boardwalk wear**
  (Boardwalk.py variants that replace, never add, never the same twice in a row): Boardwalk_4m_Worn (the middle 1.2 m of planks
  cupped 6 mm on a lighter lane of the strip, two replaced honey planks), Boardwalk_4m_Patched (a plank gone showing joists and dirt,
  two nailed-over boards), Boardwalk_Steps_Worn (treads dished 1 cm, edges rounded). Fallen fences: the existing kits'
  broken/fallen/missing shares, set per household (4.4).
- **Cost:** +1 sample and ~70 ALU on terrain pixels within 60 m inside the windows (35-50% of a street frame): estimate +0.05-0.15 ms
  GPU in the town views; VRAM +2.7 MB (BC1 2048 with mips) or +5.3 MB (BC7). Strips: 8 new mesh types, ~1-2 draws each, shadowless.

### 4.4 Dressing and clutter (lane DRESSING; PROPS for the models)
**Rules, not hand-drops:** `Art/Levels/RansomsRest/town_rules.py` (new, standard-library Python like make_town.py) turns building
types, zones and menus into vignettes; make_town.py keeps its placer and every check (roads, walks, doors, gameplay spots, spawn
points and patrols, the roosts' find circles, loot, obstacles, ground) and its determinism (the same inputs give the same town.json).
Today's hand-made vignettes (the Hasketts', the Mercers', Pruitt's...) stay as fixed entries the rules fill round.
- **Types** (placement key -> type): shopOpen store, undertaker, workshop, depot, smithy, feedStore, bathhouse; shopShut assay
  (cabinNorthWest), saloon, hotel (cabinNorth), barber (cabinSouth); office sheriff; homeLived cottageNorth (A), cottageNorthEast (H),
  dugout (E), farmhouse, B, K, M, O, boardingHouse; homeEmpty cottageSouth (D), cottageSouthWest (J), penLaneWest (C), penLaneEast (F),
  amosHouse (G), I, L, N, P; barn barn, whitlockBarn; yard the undertaker's yard, the depot platform, the coal stage.
- **Zones** in the building's frame: front 0-3 m (walk or porch), verge 3-5.5 m (the street's edge), side (alleys >= 3 m wide; a
  1.5 m lane kept clear), back 0-12 m, wall (facades).
- **Menus** (counts per building; picks by crc32(key, zone, slot) as orchard_swap does; yaw jitter +-8 degrees, stacks +-15):

  | Type | front | verge | side | back | wall |
  |---|---|---|---|---|---|
  | shopOpen | 6-10: CrateStack, SackPile_B, ToolBarrel, Keg_Trestle, Bench, BroomBucket, Spittoon, a Vignette_ShopFront | 1: HitchRail or WaterTrough or a cart | 2-3: Ladder_A, BarrelStack, LumberPile | woodshed, crates, lean-to, a delivery cart | blade sign, Handbills, lantern |
  | shopShut | 2-4: boards leaning, BottleCrate, a caught tumbleweed, Vignette_PorchEmpty | 0-1 | 1-2: weeds B, a broken crate | privy, junk | Handbills, a nailed notice |
  | office | 3-4: bench, barrel, a tipped chair | 1 hitch rail | 1 | woodpile | Handbills (wanted) |
  | homeLived | 3-5: chair or rocker, washtub, broom, pail, flower boxes, Vignette_PorchLived | 0 | 1-2: rain barrel, ToolsLeaning | 3-6: LaundryLine, woodpile + block, privy or shed, garden, coop, pump, Sawhorse_Log | lantern, horseshoe |
  | homeEmpty | 1-2: a tipped chair, leaves, Vignette_PorchEmpty | 0 | 1-2: weeds B, FallenBoards | 2-4: a leaning privy, a rotted woodpile, a broken cart, dense weeds, a dead garden | a hanging shutter |
  | barn | hay, StrawLitter, a wagon, a trough | - | ToolsLeaning, ladder | the FarmYard set (4.5) | horseshoes, tools |

- **Street events** (Main Street, the yard road, Pen Lane): every 25-35 m one of WagonFreight(_Canvas), the Buckboard, a
  PlankCrossing, a BarrelFire (D3), a WaterBarrel_Cart, a dropped CrateStack, in the 2.5 m strip beside a walk's verge; never in the
  middle 3.5 m lane, never within 3 m of a spawn point or patrol route, never in a roost's 9 m find circle, never on a loudspeaker
  post's spot.
- **Fringes and fences:** WeedFringe at every wall foot (lived _A, empty _B, one bin in three skipped for rhythm); yard fences round
  every home lot that has none (picket or rail kit): lived broken 0-0.1; empty broken 0.3, fallen 0.15, missing 0.1.
- **Density check** (printed per 10 m bin per side, as today): Main Street fronts 5-9 clutter pieces per 10 m (today 3.1), cap 14
  (visual noise and draws); every building >= 1 vignette in front; lived homes >= 3 (front, side, back), empty ones >= 2.
- **Draws:** new mesh types in the town <= 24 (each AInstancedProps costs 1-3 draws + its shadow); dense clusters are **merged
  vignette meshes** (1 draw each): Vignette_ShopFront_A/B, Vignette_PorchLived_A, Vignette_PorchEmpty_A, Vignette_Alley_A,
  Vignette_BackYard_A, baked in Blender from the kit pieces. `CLUTTER_SHADOWLESS` grows to every piece under 0.6 m, every wall and
  flat piece and the boardwalk family; small pieces CLUTTER_CULL 5000 (30 m on Medium), stacks and wagons SMALL_CULL 12000.
- **Loot:** the new meshes are not in `BREAKABLE` (build_area_loot), so the lootable count and `Looter.Economy.Balance` don't move.
- **New lots (candidates; DRESSING checks each with the probe and writes a patch):** (1) the yard road's east side X 800..1500,
  Y 7600..9000: RR_FeedStore facing the road; (2) its west side X -1200..-600, Y 7800..8800 (clear of the springhouse ruin at -1600,
  8400): RR_Smithy_Town; (3) a back lane behind the north side X 1900..2700, Y 2500..5500 (between Main Street's backs and the Sink
  road, outside the Boot Hill and Sink zones): RR_Home_K, RR_Home_B, RR_Home_N + sheds; (4) the south back X -1900..-2500,
  Y 4600..6100 (between the fields path and the workshop pad): RR_BoardingHouse or RR_Bathhouse with lines; (5) Pen Lane's far end
  X -2600..-3300, Y 3700..4300: RR_Home_I, RR_Home_L; (6) the west end behind Home_J X -1400..-1800, Y -900..200: RR_Home_M,
  RR_Home_P. Each needs a pad from TERRAIN unless its ground is under 3% (section 6); each keeps off Main 3's fight ground and the
  encounters' spots.
- **Approval:** new pieces go straight into pieces (the user judges in playtests); `APPROVED` stays as the rollback switch.

### 4.5 New props (lane PROPS; `Art/Models/Props/`, exported to `Intermediate\ArtExport_Towns`)
All on our sets (HouseTrim as a palette like FrontierClutter, WoodPlanks and PaintWorn tints, ToolWood, IronBlack, BrassWorn,
MetalRust, MI_Canvas = Polymer 0xd6c9a8, Hay, GroundDirt, FoliagePalette, BarkOak, Charcoal); Nanite off under ~2.5k triangles,
LODs 50/25; UCX on whatever blocks; pivot on the ground at the middle, front -Y; a wall piece's pivot on the wall's face; seeded,
repeatable exports.
- `TownClutter.py` (new): CrateStack_A 1.0 x 1.0 x 1.45 m (three crates, the top one askew, an ORRIN BROS. stencil); CrateStack_B
  2.0 x 1.0 x 1.0 (four crates in two courses, a sack on top); BarrelStack_A 1.9 x 0.95 x 1.5 (a pyramid on chocks); Crate_C 1.6 x 0.6
  x 0.5 (a long crate, rope handles); SackPile_B 1.2 x 0.9 x 0.6 (slumped sacks, a hay spill); Keg_Trestle 1.4 x 0.6 x 0.9;
  ToolBarrel 0.7 x 0.7 x 1.6 (handles in a barrel, a rake leaning); ToolsLeaning_A 1.0 x 0.3 x 1.7 (wall: shovel, fork, pick);
  Ladder_A 0.5 x 0.15 x 3.6 (wall); Sawhorse_Log 1.4 x 0.6 x 0.9 (a half-sawn log, a bow saw); LumberPile_Small 1.8 x 0.8 x 0.5;
  Handbills_A/B 1.2 x 0.9 m wall cards (4-6 nailed sheets, curled corners) on a new script-made atlas from
  `Tools/Blender/looter_posters.py` (invented notices, Rye/OFL), no collision, shadowless; SignBoard_Rooms, _Smith, _Feed, _Baths
  (88 x 48 cm, for SignBracket); BarrelFire 0.6 x 0.6 x 1.0 (a perforated drum on legs, char, SOCKET_Fire); WagonFreight 2.0 x 5.0 x
  1.9 and WagonFreight_Canvas (bows and canvas, 2.6 m) (Nanite + a full fallback, like the Dray); WaterBarrel_Cart 1.2 x 2.2 x 1.4;
  the six Vignette_* merges (4.4).
- `TownGround.py` (new): WeedFringe_A, _B, _C, PlankCrossing_A, _B, StrawLitter_A, FallenBoards_A (4.3).
- Variants at the end of existing files (each chains or stands like its original): Boardwalk.py Boardwalk_4m_Worn, _Patched,
  Boardwalk_Steps_Worn; VillageProps.py LampPost_B (a corner wall-bracket lamp, SOCKET_Light), LampPost_C (a two-arm crossing lamp,
  SOCKET_Light_1/_2), replacing 3 of the 9 LampPosts; Containers.py Barrel_C (a rain barrel, a lid, a downspout stub).
- `FarmYard.py` (new, round 2): Plough 2.2 x 0.6 x 1.0; DragHarrow 1.6 x 1.6 x 0.3; HayRake 2.8 x 1.6 x 1.3 (two tall wheels);
  FeedTrough 2.4 x 0.6 x 0.5; ButterChurn 0.45 x 0.45 x 1.05; WashKettle 1.2 x 1.2 x 1.3 (a tripod over a fire pit, SOCKET_Fire);
  Grindstone 1.0 x 0.5 x 1.0. Dressing: the Ransom farmyard (between the farmhouse, barn and well: kettle, churn, trough, rake, a
  wagon, StrawLitter, fringes; the barricade's 6 stand-ins and Delia's eye line kept) and Whitlock's barn yard (plough, harrow,
  trough, grindstone, a broken WagonFreight, fallen boards: Amos's empty farm).

## 5. Lanes, files, order, measurement

At most 5 lanes, disjoint files. Agents never build, open Unreal or touch git (Workflow.md); C++ goes in a holding tree
`Saved/TownsCode/` with HASHES.txt; layout.json is TERRAIN's now, so lots, placements, lights, smoke, trails and tracks go as
`Art/Levels/RansomsRest/patches/towns_*.json`. The art round 4 lanes (REPACK, REBAKE, REWORK, PERF, KIT) still run: with the
8-agent cap the towns lanes start as those finish, 2-3 at a time.

| Lane | Model | Owns |
|---|---|---|
| LIGHTS | Sonnet | `World/HouseLights.h/.cpp`, `World/LightingStates.h/.cpp` (the Kind helper), `World/WindowLifeSubsystem.cpp`, `World/AmbientAirRules.cpp`, `World/TownFire.h/.cpp` (new), `World/InstancedProps.h/.cpp` (custom data), `Tests/HouseLightsTests.cpp`, `Tests/WindowLifeTests.cpp`, `Tests/TownFireTests.cpp` (new), `Tools/Unreal/build_area_town_rr.py` (lamp tables, the farmhouse's Light_Porch) |
| PROPS | Opus | `Art/Models/Props/TownClutter.py`, `TownGround.py`, `FarmYard.py` (new); the variants in `Boardwalk.py`, `VillageProps.py`, `Containers.py`; `Tools/Blender/looter_clutter.py`; `Tools/Blender/looter_posters.py` (handbills) |
| BUILDINGS | Opus | `Tools/Blender/looter_homes.py` (kits), `Art/Models/Buildings/Homestead.py` (K-P), `TownBuildings.py`, `Outbuildings.py` (new) |
| DRESSING | Opus | `Art/Levels/RansomsRest/town_rules.py` (new), `make_town.py`, `town.json` (generated), `Tools/Unreal/build_area_dressing.py` (PIECES, FOOTPRINT_*, shadowless, cull, per-instance data, `place_town_lights` with per-state levels), `Tools/Unreal/build_area_townlife.py` (households of the new lived houses), `Saved/rr_town_probe.py` (read-only, Main runs it), `patches/towns_*.json` |
| GROUND | Opus | `Art/Levels/area_places.py`, `Tools/Unreal/build_world_materials.py` (M_Terrain WearOn, M_World TrimRecolor; only after art round 4's MATERIAL edits are committed; TERRAIN sends its M_Terrain needs to this owner), `Tools/Blender/ground_preview.py`, `Art/Levels/RansomsRest/town_looks.json`, `Tools/Unreal/build_area_town_looks.py` (new) |

Main: `views.json`, applying the patches (`Tools/layout_patch.py --check` first), imports, builds, rebuilds, the probe, perf, tests,
PIE, grabs, CODEMAP, `Docs/Pipeline/Levels.md` and `Assets.md`, commits.

**Order** (each step committed, measured and play-checked, then the user plays):
0. Main: the baseline (below) and town savings first: day lamps hidden, shadowless clutter and walks, the chimney plumes' cull
   distance, the PERF lane's findings in the town views. Goal: >= 0.3 ms margin in MainStreet, MainStreetMid and Farm before step 3.
1. LIGHTS: the state kinds (fixes Main 6-7's dark town), hidden at 0, MaxDrawDistance, no translucency lighting, porch lamps.
2. GROUND part 1 + DRESSING part 1: TrimRecolor and town_looks.json on the 17 buildings; RR_Home_B and RR_Home_I placed.
3. GROUND part 2: the wear atlas, Main Street's road term, puddles (TERRAIN's flow map when it lands).
4. PROPS round 1 + DRESSING part 2: town_rules.py, the street and storefront clutter, the ground strips, the boardwalk and lamp
   variants: Main Street, Pen Lane, the back lots, the yards.
5. BUILDINGS round 1 (kits, RR_Home_K..P, outbuildings) + DRESSING part 3 (lots, outbuildings per home; pads in TERRAIN's rebuild).
6. BUILDINGS round 2 (smithy, feed store, boarding house, bathhouse) + LIGHTS part 2 (ATownFire) + PROPS round 2 (FarmYard) +
   DRESSING part 4 (the farms).

Estimated costs before savings (to be measured): step 1 dusk +0.1-0.25 ms, day 0 or less; step 2 < 0.05; step 3 +0.05-0.15; step 4
+20-40 draws, +0.05-0.15; steps 5-6 +40-70 draws, +0.05-0.15, fires +2-3 lights. The worst town view +0.3-0.6 ms in all: it fits only
with step 0's savings. A step that pushes any view over 8.3 ms (or a view already over, Farm 8.58 or Aerial 9.36, above its baseline)
pays back before it is committed: more shadowless pieces, shorter culls, more vignette merges, fewer lights in the 45 m radius.

**What Main measures, before and after every step** (editor closed, `Saved\ArtPause.flag` on, Medium 1080p):
- `Tools\tour.ps1`: the 42 views + 11 new town views (Main adds them to views.json; x, y cm, X north, Y east, height 170, fov 80):
  MainStreetMid (0, 2600, yaw 90), MainStreetMid_Dusk, MainStreetMid_DuskSmog (`Looter.Light DuskSmog`: the story's), MainStreet_Smog
  (the MainStreet camera, `Looter.Light Smog`: most of the play time), MainStreetEast (0, 6800, yaw 270, into the sun), PenLane
  (-800, 2060, yaw 180), BackLotsNorth (2100, 1500, yaw 90), YardRoad (300, 7600, yaw 80), FarmYard (-3300, -5300, yaw 225),
  FarmYard_Dusk, WhitlockYard (-5900, 4300, yaw 210); Main tunes the cameras.
- `Tools\perf.ps1`: the 10-spider horde in town and the LMG fire row (as the Phase 3 rows).
- Splits: `Looter.Perf.HideTag Dressing`, `HideFolder Dressing/Lights`, `Effects` (house lights), `Town`; day vs dusk on one camera.
- Draws and primitives per view (the CSV); shader stats from the build_world_materials log (M_Terrain 815 instructions and 12
  samplers today; M_World 317 VS / 229 PS with the switch off); the `stat streaming` pool for the atlas; grabs to
  `Saved/ArtPreviews/Towns/<step>/`.
- Tests (only the changed work's, once, at commit): `Looter.World.HouseLights`, `Looter.World.Lighting.*`, `Looter.World.WindowLife`,
  `Looter.World.LightFlicker`, the new `Looter.World.TownFire`, `Looter.UI.Minimap`, `Looter.UI.Map`, `Looter.Encounters.Camps`,
  `Looter.World.RansomsRest.InsideBoundary`, `Looter.Breakable.*`, `Looter.Economy.Balance`, `Looter.World.RoadSign`; make_town's
  verify; `path_probe.py`; `cliff_in_buildings.py` after new lots; a PIE walk of Main Street in Smog, DuskSmog and Day and a Main 3
  fight (cover, pathing, no clipping).

## 6. What we need from the TERRAIN lane (adding erosion now)
1. **Heights unchanged** inside the mainStreet and depot zones, the farmyard and the farm's footprints, workshopPad, hotelPad,
   whitlockYardPad, the back-lot no-tree boxes (PenLane, WorkshopYard, Dugout, AmosHouse, CoalStage) and the six candidate lots in
   4.4, with a 6-10 m feathered skirt: erosion masked off there. Proof: `terrain_compare.ps1 -Except` max change inside <= 2 cm
   (target 0), so the probe's heights, the walk tops (0.38 m; WALK_SINK_MOST 30 cm) and every placed piece stay valid. If anything
   moves, a list of the tiles, so Main re-runs `Saved/rr_town_probe.py`, make_town.py and the dressing.
2. **The flow and low-point maps** erosion already computes (on its 12.5 cm raster), clipped to our three wear windows, as 16-bit PNG
   or .npy beside the layout: flow accumulation and depression depth. Puddles go where the depth is over 1 cm and the flow is high;
   mud fans where flow leaves a roof's drip line.
3. **Sediment colour off the town:** the macro's new sediment or flow tint at 0 inside the mainStreet zone and the lots (the wear term
   owns that ground), at most 0.2 on the farmyard.
4. **Pads for the new lots** taken in the same rebuild as erosion (we send `patches/towns_lots.json` before it), or the "rebuild only
   what changed" path (the user's 10-30 call) finished, so a later pad costs a tile rebuild, not ~45 min.
5. **Paint-only tracks:** a `paint: true` flag on a layout road (or a `macro.tracks` list) that the shape step ignores and
   area_places.py paints, for the wheel tracks into the yards; no carving.
6. **One owner per file:** layout.json (TERRAIN; we send patches); `build_world_materials.py` M_Terrain one owner at a time (GROUND
   after art round 4; TERRAIN sends its term to it as a patch); area_places.py and PlacesScatter ours; area_shape.py, area_macro.py and
   the scatter mask theirs; neither side changes `macro.places` or `macro.roads` without telling the other.
7. **No gullies across** Pen Lane, the fields path, the yard road or the north road within 30 m of the town zone, and none lowering
   Main Street's edges under the walks.

## 7. Decisions for the user (Main asks with choices)
- D1 Lamps under the smog (Main 1-6): dim lit (recommended) / dark as now.
- D2 A night state for the hub after Main 7 (none exists; a new lighting state is a system): later / no.
- D3 Fires: the Company's barrel fire at the gate, the smith's forge, Delia's wash kettle, the bathhouse boiler: all / some / none.
- D4 Grey galvanized sheet roofs (modelled, not the trim's loot-coloured tin) on outbuildings only: yes / no.
- D5 New buildings: how many lived in (the story: half the town fled): the boarding house and K, M, O lived (recommended) / fewer.
- D6 Wheel tracks into the yards as paint-only roads: yes / no.

## 8. Risks
- Perf: the town views have no margin (MainStreet 8.30, Farm 8.58); the plan depends on step 0's savings and the PERF lane.
- `TrimRecolor` tints by UV strip: a model that maps another part into strip E's range (awnings, woodshed roofs) recolours too
  (intended per actor; checked on the board); strip borders may bleed at far mips.
- Clutter in the street bed changes Main 3's cover and the town hordes' paths: the 3.5 m lane, the spawn and patrol clearances and a
  PIE fight check guard it.
- Rule-placed props are only as good as the probe: run `Saved/rr_town_probe.py` after every build that moves buildings.
- New lived houses need town-life households (`TownLifeRules`) or they stay silent; new roofs and posts become bird perches
  (`build_area_fauna.py` re-run).
- The handbills atlas and new sign lettering: invented text only, Rye (OFL); no copyrighted names.

## The user's answers (2026-11-04 evening)
- D1 lamps under the smog: **dim lit** (low glow in Smog/SmogThin, full in Dusk/DuskSmog).
- D2 a night state: not asked yet (later).
- D3 fires: **the Company's barrel fire at the gate, the smith's forge, the bathhouse boiler** (not Delia's wash kettle).
- D4 galvanized sheet roofs on outbuildings: **yes**.
- D5 lived in: **the boarding house and K, M, O**; the rest shut, boarded, weeds, dark windows.
- D6 wheel tracks into the yards: **no**.
- Also (perf, the same evening): 120 fps a hard line for every view; Medium's filtering 8x -> 4x and LODs 25% sooner (the user's
  yes), so step 0's savings come from the PERF lane's list first; Unreal Insights allowed to find the 1.2-1.6 ms gap.
