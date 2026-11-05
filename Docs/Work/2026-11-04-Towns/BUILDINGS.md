# BUILDINGS lane result (2026-11-05 early; PLAN steps 5 and 6: kits, RR_Home_K..P, outbuildings, town buildings)

16 new models, all script-made in our sets, nothing imported. Import folder `Intermediate\ArtExport_Buildings2` (the 16 new FBX
only; manifests cut to them and their materials). Full exports `Intermediate\ArtExport_Buildings2_Full` and `_Repeat` (all of
Homestead.py, Outbuildings.py, TownBuildings.py); baseline of today's Homestead.py `Intermediate\ArtExport_Buildings2_Base`.
Checks (all passed): two full exports equal model by model (`fbxdiff`: vertices, loops, UVs, vertex colours/AO, normals,
material slots, every hull's and socket's transform; Outbuildings re-checked as `_Out`/`_OutRepeat` after the last roughness
change); today's A-J, RR_Well, RR_Outhouse, RR_Windmill, RR_WindmillFan identical to the baseline (FBX and manifest entries
and all 15 material entries); `tangentcheck` 0 corners on all 30 FBX. Tris per model below; every new model 1-4 slots.
Totals: homes K-P 42,036 tris; outbuildings 9,912; town buildings 31,827.
Previews `Saved/ArtPreviews/Buildings2/`: one PNG per model, `Lineup_Homes.png` (A-J behind, K-P in front, one scale),
`Lineup_Outbuildings.png`, `Lineup_TownBuildings.png`; `Check/` holds back and eye-level street views of every new building.

## Files
- `Tools/Blender/looter_homes.py`: additions only (A-J unchanged). Moved in from Homestead.py unchanged: `wall_spaces`,
  `polygon_spaces`, `rect`, `gable_outline`, `notch`, `clad` (+ `backing`, `openings_for`, defaults unchanged), `skip_round`,
  `roof_z`, `hewn_wall`, `LAP`. Kits: `cover(m, slope, kind)` kinds R1 (town.shakes), R2 (`patched_shakes`: ragged patches of
  single new shakes, 70% WoodHoney 30% strip E), R3/R3T (`batten_roof`: boards down the slope, battens; A or WoodBlack), R4
  (`tar_paper`: 0.9 m rows lapped 8 cm, laths on every lap, eave fold, torn corners curling up), G (`corrugated`: modelled
  corrugations 10 cm pitch 1.8 cm deep, sheets lap one wave, rows lap 15 cm, loose/missing sheets); `ridge()` per kind;
  porches `porch()` P2/P3 (+ slack: corner post down on the deck, beam and Droop roof sag, rail gone, planks gone),
  `stoop()` P1 (stone or plank landing, shed or gable hood on scroll brackets, slack), `portico()` P4 (gable roof dying into
  the house's roof via `roof_back`, pediment, bench, slack); `porch_deck()` (town.deck + gone planks + hull); chimneys
  `chimney_stone` C1, `chimney_pipe` C2 out a wall with elbow, `chimney_pipe_roof` C2 through a roof, `chimney_clay` C3;
  `porch_lantern()` (glass on WindowGlow, iron on trim H3, SOCKET_Light_Porch); `raised_cut`/`raised_door` (doors up a wall);
  `seat()`, `step_count()`, `post_brackets()`; previews `lineup()`, `previews(out_dir=)`, preview-only `_preview_master_scalars`
  (RoughnessOffset/Specular shown in Blender renders; exports never call it).
- `Art/Models/Buildings/Homestead.py`: K-P builders appended; shared steps now aliases of looter_homes' (same code). `--lineup`.
- `Art/Models/Buildings/Outbuildings.py`, `Art/Models/Buildings/TownBuildings.py`: new.

## New material instances (all on M_World, no new texture sets)
- MI_TarPaper: Polymer tint 0x403c38, RoughnessOffset 0.3, Specular 0.25, mapped at 320 px/m (R4). Replaces the plan's
  PaintBlack: PaintWorn's satin read as blue-black plastic.
- MI_GalvanizedSheet: MetalWorn tint 0xb8b5ac (MI_MetalWorn_Windmill's grey), RoughnessOffset 0.36 (G, outbuildings only, D4).
  Own name on purpose: the importer would reparent MI_MetalWorn_Windmill (the level build makes it from MI_IronBlack).
- Reused as-is: HouseTrim, WoodPlanks, WindowGlow, IronBlack, WoodCream, WoodBlack, WoodHoney (Boardwalk.py's exact recipe),
  GroundDirt, Hay. RoughnessOffset/Specular come through the manifest as scalars (as MI_BoneDry, MI_BiscuitCrust).

## Conventions (as A-J)
Pivot = front door's middle on the front wall's outer face, ground z = 0; front Blender -Y (= Unreal +X). Sizes below in
Blender metres from the pivot: x across the front, y back from the front wall (negative = in front: porches, docks), z up;
"walls" = the body's outer faces. Foundations reach 0.35 m below ground. Homes and town buildings: Nanite on, fallback 100%
(Medium draws every triangle; no LOD chain). Outbuildings: Nanite 0, LODs 50/25 (default screens). Collision: UCX hulls only.
SOCKET_Window_n on every lamplit pane (lived), SOCKET_Light ~1 m behind the front windows at floor + 1.5 m, SOCKET_Smoke
(SOCKET_Smoke_2 second) at every chimney/stovepipe/vent top, SOCKET_Light_Porch in the porch lantern's glass (new: for the
LIGHTS lane's porch lamp), SOCKET_Fire on the smithy hearth and the bathhouse fire door (D3).

## Kit homes (Homestead.py)
| Model | State | Body / roof / porch / chimney / paint | Walls x; y; ridge | Bounds x; y | Tris | Slots | Sockets | Hulls |
|---|---|---|---|---|---|---|---|---|
| RR_Home_K | lived | 1.5-storey side gable, eyebrow windows / R2 / P3 whole front / C1 outside left gable / whitewash band_G, H2 trim | -4.2..4.2; 0..6.0; 6.9 | -5.12..4.60; -2.55..6.54 | 9586 | HouseTrim, WindowGlow, WoodPlanks, WoodHoney | Window_1-5, Smoke, Light_Porch, Light | 13 |
| RR_Home_L | empty | shotgun 2 rooms, front gable / R4 / P1 plank stoop, shed hood, slack / C2 out right wall / weathered A, teal H1 door nailed | -3.06..1.34; 0..7.4; 5.4 | -3.46..2.05; -1.75..7.75 | 5734 | HouseTrim, TarPaper, IronBlack, WoodPlanks | Smoke | 7 |
| RR_Home_M | lived | hall-and-parlor 1 storey / R1 / P2 5.2 m / C3 clay at ridge / WoodCream laps, H1 trim | -4.6..4.6; 0..5.6; 5.4 | -4.97..4.97; -2.54..6.11 | 5910 | HouseTrim, WoodCream, WindowGlow, WoodPlanks | Window_1-4, Smoke, Light_Porch, Light | 12 |
| RR_Home_N | empty | b&b cabin + bare-board lean-to behind / R3T (lean-to R3) / P4 portico, slack / C2 through lean-to / WoodBlack, band_G trim | -3.1..3.1; 0..7.2 (lean-to 5.0..7.2); 5.1 | -3.45..3.45; -2.07..7.56 | 7175 | HouseTrim, WoodBlack, IronBlack, WoodPlanks | Smoke | 10 |
| RR_Home_O | lived | narrow 2-storey, front gable / R2 / P2 / C1 x2 through ridge / H2 laps below, band_G above a belt | -1.08..3.92; 0..7.6; 8.0 | -1.58..4.43; -2.45..7.97 | 7602 | HouseTrim, WoodPlanks, WindowGlow, WoodHoney | Window_1-4, Smoke, Smoke_2, Light_Porch, Light | 12 |
| RR_Home_P | empty | log pen + lower clapboard addition / R1 both / short P3, slack left / C1 outside pen gable / chinked logs, A laps | -2.38..5.62; 0..5.0; 4.9 | -3.29..5.98; -2.21..5.45 | 6029 | HouseTrim, WoodPlanks | Smoke | 11 |

Lived/empty per D5 (K, M, O lived; L, N, P shut: boarded or shuttered windows, nailed doors, slack porches).

## Outbuildings (Outbuildings.py; Nanite off, LODs 50/25)
| Model | What | Walls x; y; height | Tris | Slots | Sockets | Hulls |
|---|---|---|---|---|---|---|
| RR_Shed_A | side-gable tool shed, b&b A, R1 shakes, plank door in the long front, shuttered gable window | -2.2..0.8; 0..2.4; 2.9 | 1774 | HouseTrim | - | 2 |
| RR_Shed_B | lean-to, level rough boards, G roof (high front 2.3, overhanging the door) | -1.15..1.25; 0..1.8; 2.35 | 2284 | HouseTrim, GalvanizedSheet | - | 2 |
| RR_Shed_C | front-gable shed, b&b A, G gable roof, 1.2 m plank door, boarded side windows | -1.3..1.3; 0..3.0; 3.05 | 3214 | HouseTrim, GalvanizedSheet | - | 2 |
| RR_Privy_B | two-holer, upright boards, R4 shed roof, diamond cut, vent slots | -0.9..0.9; 0..1.4; 2.3 | 806 | HouseTrim, TarPaper | - | 2 |
| RR_Smokehouse | hewn logs on a stone plinth, steep R1, vent cap on the ridge | -1.1..1.1; 0..2.2; 3.6 | 1338 | HouseTrim | Smoke (under the cap) | 2 |
| RR_RootCellar | sod mound (Hay/GroundDirt, the dugout's), stone headwall, timber frame, plank door | -2.0..2.0; 0..3.4 (mound to 2.12 sides); 1.42 | 496 | Hay, GroundDirt, HouseTrim | - | 2 |

## Town buildings (TownBuildings.py)
| Model | What | Walls x; y; ridge | Bounds x; y | Tris | Slots | Sockets | Hulls |
|---|---|---|---|---|---|---|---|
| RR_Smithy_Town | open front (walk in), hewn frame, b&b A, earth floor slab, R3, forge + hood + chimney, bellows, anvil on stump, tub, 3 wheels, bench + vise, SMITH & WHEELWRIGHT | -4.5..4.5; 0..7.0; 5.3 | -4.90..4.90; -0.54..7.44 | 8506 | GroundDirt, HouseTrim, IronBlack, WoodPlanks | Fire (hearth), Smoke | 17 |
| RR_FeedStore | warehouse, floor 1.0 m on stone, b&b A, sliding door 0.55 m open on dark, loft door, hoist beam + block + rope, FEED & GRAIN, R4, dock 2.5 m deep + steps right | -3.5..3.5; 0..10.0; 6.5 | -3.97..3.94; -3.69..10.35 | 7682 | HouseTrim, IronBlack, WoodPlanks, TarPaper | - | 8 |
| RR_BoardingHouse | lived; 2 storeys, band_G laps, H1 trim + belt, 8 front sashes (6 lit), P2 + lantern, outside stair up the right gable to an upper door (one ramp hull), ROOMS blade sign, C1 + C2 | -5.5..5.5; 0..8.0; 8.6 | -5.90..6.85; -2.65..8.55 | 10634 | HouseTrim, WindowGlow, WoodPlanks, IronBlack | Window_1-6, Smoke, Smoke_2, Light_Porch, Light | 17 |
| RR_Bathhouse | A laps, R4, BATHS . LAUNDRY, 2 lit front windows (open shop), wash bench + tubs, boiler on a stone firebox under its own R4 roof, tall stovepipe, ridge steam vent | -3.0..3.0; 0..5.0; 4.7 | -3.37..5.00; -0.51..5.39 | 5005 | HouseTrim, WindowGlow, TarPaper, IronBlack | Window_1-2, Light, Fire (fire door, faces +X), Smoke (stovepipe), Smoke_2 (vent) | 8 |

## For Main and DRESSING
- Import: `Looter.ImportModels C:/Dev/AI_Looter_Shooter/Intermediate/ArtExport_Buildings2` (16 models into /Game/Art/Buildings;
  new MIs MI_TarPaper, MI_GalvanizedSheet; WoodHoney etc. rewritten with identical values). Don't import the _Full folder
  blind: it re-imports A-J (identical) and RR_Windmill/Fan built from the REPACK lane's uncommitted `Tripo/parts_buildings.py`.
- Lights (LIGHTS lane): K, M, O, boarding house have SOCKET_Light, SOCKET_Window_n, WindowGlow and SOCKET_Light_Porch; the
  bathhouse has SOCKET_Light + 2 lit panes (shop). L, N, P, outbuildings, smithy, feed store: no lit glass. Fires: smithy
  SOCKET_Fire on the hearth top, bathhouse SOCKET_Fire at the firebox door (rotated to face out, +X in Blender).
- Smoke: every chimney has SOCKET_Smoke; the layout's smoke list decides which smoke (lived ones + smithy + bathhouse).
- Placement: the smithy is open-fronted and walkable inside (its earth floor slab -0.24..0.02 m); the feed store's dock is 1.0 m
  high, 2.5 m in front of the wall (bounds y -3.69 incl. the steps), the sliding door is shut to players (hull); the boarding
  house's stair foot sits 0.73 m inside the front line at x 5.7..6.7 (keep the right side clear 1.4 m); RR_Home_N's lean-to
  takes 2.2 m behind its back wall.
- TrimRecolor: roofs on strip E (R1/R2 and the porch shakes), siding on strip A (L, P's addition, the bathhouse, the sheds'
  battens, the smithy, the feed store) tint per actor; WoodCream/WoodBlack/H2 laps and TarPaper/Galvanized don't. R3 grey
  boards are strip A, so they take the siding tint (intended: an unpainted board roof weathers with its walls).
- New lived houses need TownLife households (plan risk); new roofs and porches are perches for build_area_fauna.
- CODEMAP: add `Art/Models/Buildings/Outbuildings.py` and `TownBuildings.py`; Homestead.py's line gains K-P, looter_homes'
  the kits. `Docs/Pipeline/Assets.md` could take the Lessons below.
- The other lanes: looter_homes' `stage()` now shows RoughnessOffset/Specular in every render that uses it (previews only;
  no export changes), and `previews()` reuses an existing `_Sun`/`_Human` in the same run.

## Blunt read
- Good: one family with A-J (same trim sheet, shakes, casings, sashes, AO); each new home reads apart by silhouette and
  porch; tar paper, tarred boards and galvanized sheet break the one-shake-roof look; the four trades read from the street
  (Rye signs, forge, dock and hoist, stair and blade sign, boiler).
- Weak: R2's new shakes (WoodHoney) still read orange against grey shakes; WoodCream "bleached" reads plain mid-brown;
  galvanized looks blue-grey under a blue sky (as the windpump's instance does): judge it in Smog/Day; P3 brackets chunky;
  the root cellar's sod reads as thatch (the dugout's Hay); the smithy's open inside is dark and sparse past the props.
- Not verified: anything in Unreal (import, Nanite build, TrimRecolor on these UVs, sockets' lights/fires/smoke, walking the
  stair, dock, porticos and smithy, perf). Outbuildings' LOD1/2 are Unreal's decimation (tangentcheck covers the full mesh).
  Blender previews only approximate RoughnessOffset/Specular.

## Lessons
- `corrugated()` needs at least two waves a sheet: a one-wave last sheet never advanced (an infinite loop hung a slot).
- A wall outline whose foot dips below a door notch fills with a sliver: zero tangents (smithy, fixed: foot at 0 + a sill).
- `clad()`'s 24 cm frieze runs off the 20 cm H1/H2 strips: K and M use a 19 cm `slim_frieze`.
- Doors up a wall: cut with `raised_cut` (a window-kind hole), build with `raised_door` (door() builds from z 0).
- `previews()` twice in one run made two suns; it now reuses the stage and figure.
