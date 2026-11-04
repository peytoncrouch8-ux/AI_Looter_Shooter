# Art audit, 2026-11-03 (read-only: no game file changed, no Tripo credits, the editor not touched)

The user (asked which moment disappoints most): "The art and assets of the game. The new Tripo models are blurry, don't
read as intended, and the collisions are messed up. The barge ships, terrain, etc."
Method: every GLB parsed (175, `Art/**/*.glb`), every texture's size read (659 under `Art/Textures`), texel density measured
on the exported game meshes (`Intermediate/ArtExport*/*.fbx`, UV area against surface area), the colour maps' real detail
measured (luma variance per octave, 2x2 pyramid, used texels only), collision measured against the look (hull surface
sampled against the visible mesh; rays down onto decks; visible surface against hit zones), the engine's scalability ini
read, Tripo GLBs rendered in Blender at play distance (90 deg across, 1920 x 1080). Scripts kept in the audit session's
scratchpad only (glbinfo, texel, fbxtexel, texdetail, collide, decks, renders, sheet). Sheet: `Saved/ArtPreviews/ArtAudit/compare.png`.
Texel need at 1080p, 90 deg: a pixel is distance/960 wide, so 1:1 needs ~19 texels/cm at 50 cm (first-person gun), ~5 at 2 m
(a deck underfoot), ~3.2 at 3 m (a wall), ~1.9 at 5 m (a creature).

## (a) Findings, ranked by how much they hurt the player

1. **The barge you board is not the barge you saw** (measured). `BargeRules.h:35` DeckTop 1400 ("measured by eye ... tune if
   it floats or sinks"), `BargeBoarding.h:218` KitLift 0, never set (`build_area_main6.py:242` only logs "set kit_lift if they
   disagree"). Tripo's own deck (SM_DredgeBarge, 75 x 24.75 x 48.49 m) is at 22.5 m over the keel (up-facing area 524 m2 at
   22.5-24.0 m; open deck hits 22.51 m at x -24..-20 and +6). Rays down from 60 m over the kit's deck rectangle (x -3450..3350,
   y +-1150): 100% meet Tripo geometry above the walk deck, median +14.2 m, 10-90% +8.5..+30.8 m. The player fights 8.5 m
   inside the hull; the hull has no collision (`BargeBoarding.cpp:75`) and its faces cull from inside, so it vanishes round
   you. What is left is `BargeLooks.h`'s engine shapes: 10 of its 12 Tripo kit paths are not in the project (DeckPlate, Rail,
   Stack, Crane, HoseDrum, FurnaceVent, SoulFurnace, SoulFurnaceBurst, FurnaceGrate, DraftValve; only SoulCoal and DredgeHose
   exist), so the deck is engine cubes, the hose drum a cylinder on a cube, the furnace hall cones and cylinders (grabs
   `Saved/Screenshots/Demo47/m6_12_deck.png`, `m6_16_hall.png`). The forward deck plate is one engine cube 58.9 x 23 m with
   MI_Landfall_BargeIron (MetalWorn 1024, cube UVs 0-1 per face): 0.17 x 0.45 texels/cm, stretched 2.6:1.
2. **Big Tripo surfaces have almost no texels where you stand** (measured). Game maps, texels/cm (Medium in brackets, see 3):
   barge 0.14 (0.07: one 2048 map over 9,833 m2), Lamplighter 0.32 (0.16: 1,928 m2; a texel is 3 cm, 6 on Medium, underfoot
   for all of Landfall and the sortie), Company tender 1.24 (0.62), Tripo farmhouse shell 0.62 (0.31: 343 m2, and the atlas
   uses 31% of its square, UV area 0.313). The scripted homes beside it: 3.18 (1.59; HouseTrim 2048). So the farmhouse is 5x
   blurrier than its neighbours and the ship decks 15-30x short of ~5. Cause: `looter_tripo_wardens.py` "people and ships 2K
   maps, props 1K (the user's caps)", buildings 2048 (`looter_tripo_buildings.py:623`), all from 4096 deliveries.
3. **Medium halves every texture and drops every creature a LOD** (measured config, effect inferred). `ApplyQuality`
   (`GraphicsSettingsSubsystem.cpp:263-268`) takes the engine's level 1 and overrides none of: `TextureQuality@1`
   r.Streaming.MipBias 1 (a 2048 map draws as 1024), r.MaxAnisotropy 2 (decks, ground and walls seen at a slant smear),
   pool 600 MB; `ViewDistanceQuality@1` r.SkeletalMeshLODBias 1 (LOD0 never drawn on Medium: SK_Unpaid 23,926 tris draws LOD1
   ~12k within 5.75 m and LOD2 ~6k beyond; the spider LOD1 within 12.9 m) and r.ViewDistanceScale 0.6 (`BaseScalability.ini`).
   Medium is render-thread bound with the GPU at 6.4-6.8 ms of 8.3 (Performance.md), so these are GPU-side costs it may afford.
4. **Tripo's maps are soft at the source; bigger maps won't fix it** (measured). Share of variance in the finest octave:
   Tripo 0.4-2.6% (barge 1.6, Lamplighter 0.9, Unpaid 0.7, spider 1.2, chest 3 1.3, Delia 0.4, Sentinel 2.1, AR Armored 2.6;
   only the farmhouse repaint 5.5); our tiled sets MetalWorn 23.4%, GroundThatch 32.1%, RockCliff 5.5% (11.0 next). A 4096
   Tripo map keeps 97% of its variance at 512 px (Delia), 1024 (Unpaid, Lamplighter, AR Armored, Drover) or 2048 (barge, spider,
   chests, Sentinel); the Unpaid's 4096 colour map is an 851 KB JPEG. The written maps keep it: T_UnpaidPaint, T_SpiderTripo,
   T_TripoBullpupBodyStandard, T_TripoDelia hold 1024 px of detail in 2048. The unsharp mask (0.85 at 1.2 px/1024,
   `looter_tripo_wardens.write_maps`) only lifts what is there (barge finest octave 1.6% -> 5.6%). The one Tripo path the user
   passed as crisp is the melee rebake (`looter_rebake.py`, 2026-10-28: our sets cast onto Tripo's shape, edge wear from
   curvature, normal from our colour).
5. **Fitting bends shapes until they stop reading** (measured / doc). Spider: Tripo legs 114-159 cm fitted to the rig's
   230-238 cm, stretched 1.5-2.6x along but at most 1.4x across (`ACROSS_POWER` 0.75, Enemies.md) -> thin dark poles
   (n_spider1 vs the source's thick banded tan legs). Unpaid: QuadriFlow remesh (28,997 -> 23,926 tris), hands halved, arms
   pulled in, robe swept back, then the arms-out floating rest pose (`Unpaid.py` "rest pose is the game's idle") -> a pale
   scarecrow (n_unpaid) where Tripo made a hunched dark figure with hands low. Per-axis stretch: Tripo barn 1.44
   (TRIPO_BUILDINGS.md), Bullpup Body_Standard 1.31 (width x0.84), farmhouse ~1.2; the other 11 Bullpup parts 1.01-1.17 (fine).
   LODs on top: chests LOD1 (50%) from ~1.5 m and LOD2 (20%, Tier 1 ~935 tris) from ~4.2 m (importer default screens
   0.45/0.1575); Tier 1 already kept only 21% of Tripo's 22,338 tris (Tier 3 33%, Tier 5 40%).
6. **Collision misses the Tripo looks** (measured; share of the hull's outer surface more than 25 cm off the visible mesh):
   SM_Farmhouse_Ransom 48% (>10 cm 71%) vs the scripted SM_Farmhouse 19% (24%) and SM_RR_Home_A 22%; SM_Barn_Ransom 30% with
   2 hulls; Cottage 14%. Cause: `looter_tripo_buildings.refit_hulls` makes a 26-direction k-DOP of every shell vertex inside the
   old box grown 25 cm sideways (trim, sills, eaves, posts all swell the wall). Single-box props (`looter_tripo_wardens` SPECS
   `boxes='box'`): BarricadeTable 88% of its hull >10 cm off, HoseIntake 52% >50 cm (a 3 m block), AnchorWinch 35% >25 cm.
   SM_LanternPost: a 19 cm slab over the whole 2.1 x 2.5 m footprint (`boxes ... 0.06`) and nothing on 40% of its look (arm,
   lantern, bell). Decks are flat boxes: Lamplighter (`LamplighterShipParts.cpp:39-44`) visible floor >10 cm under the
   collision top on 21% of the deck, >10 cm over it on 26%; tender: median 17 cm under (riflemen float). Fine: chests
   (<=14% >10 cm, none >25), CompanySandbags 0%, Reliquary 2%, hit zones (spider 2%, Unpaid 7% of the surface >8 cm outside).
7. **Small creatures and chests are thin on texels** (measured). Unpaid paint atlas (UV 1) uses 28% of its 2048 square (body
   0.230 + hat 0.053): 5.8 texels/cm, 2.9 on Medium (fine at 5 m, soft in a lunge at 2 m). Stone Sentinel 3.7 (1.85; UV area
   0.537). Chest Tier 1 8.6 (4.3) on a 1024 map, opened at ~1.5 m where ~6-10 is needed; Tier 3 10.6, Tier 5 11.9. Spider 8.1
   (4.1): enough. Guns are not short: Bullpup Tripo body 39 (20 on Medium), Drover 94, scripted Ranchhand 10.2.
8. **Terrain reads as a generator** (measured numbers, read inferred). 512 m square, 190,000 triangles (`layout.json mesh`;
   vertices 0.3-2.4 m apart, ~1.7 m on average): the 12.5 cm height raster never reaches the mesh. Macro colour 6.25 cm/px
   in four 4096 windows holds what a 512 px map holds (smooth by design); ground detail tiles Thatch 3.2 texels/cm, Stony 3.7
   (1.6/1.8 on Medium, aniso 2). Cliff kit rocks draw 24-30% of their triangles on Medium (Nanite fallback, 33 of 40 Nanite rock
   models). No erosion: the user's 2026-10-30 call (hydraulic + thermal in `area_shape.py`, rebuild only what changed) is
   unbuilt; playtest note 10 (2026-10-27, "terrain is still hella whack") still waits on the user's screenshots. Far trees are
   lollipop cards, steep faces flat strata (Tour `RidgeFoot_Medium.png`).
9. **Open from playtests** (doc). Note 15 "staircases around the map are invisible" and note 30 floating fences at the
   mountain farmhouse, open since 2026-10-27 (screenshots asked, none). Fixed since: the lift cage in the mountain
   (2026-10-30, `Bugs.md`), the barge deck's Floor tag and the tender fall trace (2026-11-01, Handoff).

## (b) Protect
The Tripo silhouettes at range (barge in `SpyglassBarge_Medium`, the Lamplighter's bag); Tripo gun parts (20-94 texels/cm,
stretch <=1.17 bar Body_Standard); the melee rebake (`looter_rebake.py`) as the quality bar; the chests' and M23 props' tight
hulls (`looter_tripo_m23` per-model k-DOPs); the creature hit zones; DeckTop measured off the mesh for the Lamplighter (422,
matches 4.0-4.5 m) and tender (298); the farmhouse at 100% fallback; the trim-sheet chroma grade that sits Tripo buildings in
the town palette; the editor-free checks (`tripo_buildings_check.py`, Fix1031 renders).

## (c) The one change first
Medium's texture and creature detail: in `QualityVariables` (`GraphicsSettingsSubsystem.cpp:23-47`) add Medium values
r.Streaming.MipBias 0, r.MaxAnisotropy 8, r.SkeletalMeshLODBias 0 (and r.Streaming.PoolSize ~1000). Three lines, no art or
credits, every asset at once: near surfaces get twice the texels (Lamplighter deck 0.16 -> 0.32, farmhouse 0.31 -> 0.62,
Unpaid 2.9 -> 5.8, ground 1.6 -> 3.2), slanted decks and ground stop smearing, creatures draw their full mesh. Measure with
`Tools\perf.ps1` (tour heaviest views, Horde Spider/Unpaid rows) before and after; the margin is 0.07 ms at the heaviest view.
Then the barge (plan 2), which is the worst single moment.

## (d) Plan (small steps, in order)
| # | Change | Files | The user will see | Risk | Credits / eye |
|---|---|---|---|---|---|
| 1 | Medium: MipBias 0, aniso 8, skeletal LOD bias 0, pool 1000 | `Settings/GraphicsSettingsSubsystem.cpp` (+ Settings tests) | everything near ~2x sharper; creatures full detail | GPU/VRAM: perf.ps1 rows | none / eye |
| 2 | Barge walk deck onto Tripo's: DeckTop 1400 -> ~2250 (or KitLift 850), cube plates hidden where the hull's deck shows (keep the boxes), hatch stair and hall re-fitted, berth (BerthUnderDeck) rechecked | `World/BargeRules.h`, `BargeBoardingKit.cpp`, `build_area_main6.py`, barge tests | you board the barge you saw: its stacks, bridge, rails round you | medium: spots, cover, berth, the fall scene | none / eye |
| 3 | Barge kit made for real: deck plate, rail, stack, crane, drum, vent, furnace, grate, valve as Blender scripts on our sets (1024 px/m), or Tripo when funded | new `Art/Models/Tripo/BargeKit.py` (or Props), `BargeLooks.h` paths unchanged | no more cubes, cylinders, cones | art time | Tripo optional (25 left, 30 a model) / eye |
| 4 | Detail layer: M_World opt-in object-space triplanar detail (normal + light/dark of a tiled set, 0.5-1 m tile, fading 2 -> 15 m), set on ships, Tripo buildings and big props | `Tools/Unreal/build_world_materials.py` M_World, `looter_tripo_wardens.write_maps`, `looter_tripo_buildings.swap` | rivets, grain, rust at arm's length on decks and walls | shader cost (static switch: only opted MIs pay) | none / eye |
| 5 | Repack atlases and recast from the 4096 source: Unpaid paint 28% -> ~70%, farmhouse 31%, barge 46%, Lamplighter 50% | `Art/Models/Creatures/Tripo/parts_unpaid_paint.py`, `looter_tripo_wardens.py`, `looter_tripo_buildings.py` | those 1.5-1.7x sharper | seams, bake time | none |
| 6 | Hulls that fit: refit margin 25 -> 5 cm on planes fitted to the walls, a capsule swept through every door; per-part k-DOPs for BarricadeTable, HoseIntake, AnchorWinch; LanternPost foot box + arm hull instead of the slab | `looter_tripo_buildings.refit_hulls`, `looter_tripo_wardens` SPECS | shots hit walls, no bumping into air | Delia/Hob/barricade traces (re-run) | none |
| 7 | Decks flush: flatten the Lamplighter's walk faces onto 422 (the farmhouse porch's `flatten`), tender DeckTop 298 -> ~281 | `looter_tripo_wardens.py`, `World/CompanyTender.cpp` | feet on the boards | low | none |
| 8 | Spider legs as thick as they are long (`ACROSS_POWER` 0.75 -> 1.0) or a shorter rig | `parts_spider_build.py` (rig: `Spider.py`) | legs, not poles | gait, IK, hit zones | none / the user's call |
| 9 | Unpaid hunched idle, hands low (Phase 2 #11) | `Unpaid.py` rest pose, `UnpaidCreatureRig.cpp` | reads as the Tripo figure | anim layers | none / eye |
| 10 | Terrain erosion + rebuild-what-changed (the user's 10-30 call); ask for note 10's screenshots | `Art/Levels/area_shape.py`, `area_macro.py` | gullies, fans, worn slopes | 45 min rebuilds; terrain_compare | none / eye |
| 11 | Rest of Mains 4-7 Tripo kit (TRIPO_MODELS.md M45, M67) | lanes TRIPO_M47 pattern | set pieces lose stand-ins | money | credits: ask |
Not worth it: raising the 2K/1K caps alone (finding 4: the 4096 maps hold 512-2048 px of detail).

## (e) Sheet
`Saved/ArtPreviews/ArtAudit/compare.png` (1440 x 4981, two columns: Tripo source in Blender | in game; rows barge far, barge
deck, furnace hall, Lamplighter deck, farmhouse, Unpaid, spider, Bullpup body, terrain). Its renders:
`Saved/ArtPreviews/ArtAudit/src/` (barge_far, barge_deck, lamp_deck, farm_wall, unpaid_far, spider_far, arbody_far, ...).
Grabs used: `Saved/Screenshots/Tour/SpyglassBarge_Medium.png`, `RidgeFoot_Medium.png`, `Demo47/m6_12_deck.png`, `m6_16_hall.png`,
`m6_04_lift.png`, `Tools/farm.png`, `n_unpaid.png`, `n_spider1.png`, `g_hud.png` (the n_/g_ grabs are a 1130 x 520 PIE window).
