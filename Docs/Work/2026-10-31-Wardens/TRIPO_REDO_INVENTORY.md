# Tripo redo inventory (2026-10-31)

The user, after seeing the Tripo people, ships and props: "Seeing those models makes me want to have you redo all our
designs you generated". This is the list to choose from. Read-only survey of `Content/Art/**`, `Art/Models/**`,
`Art/Tripo/**`, `Source/`, `CODEMAP.md`. Nothing was changed except this file.

## How to read it

- Credits: static model 30 (`Tools/tripo.py make`, text to 3D; a repaint adds 15-20, Delia cost 75); humanoid rigged onto the
  UE mannequin skeleton ("unreal" spec) 30 + 25 = 55. Add 20-30% for retries. Credits already spent in `Art/Tripo`: 800
  (the Wardens batch, 22 folders). The user's earlier Tripo models (AR parts, melee, chests, creature looks) were made on
  their own plan and are not in that number.
- Effort: **Easy** = static mesh placed by build scripts, re-import under the same name, fix scale and pivot. **Medium** =
  sockets, collision (UCX), pivots, split moving parts or hands/gun fit to redo (the Blender fit step every earlier Tripo
  import needed: `Tools/Blender/looter_tripo.py`). **Hard** = a custom rig, bone names, pose tables, hit zones or tests the
  code depends on.
- Three import recipes already proven in this repo:
  1. *Static*: GLB, fitted in Blender (sockets, UCX, rebaked maps), same `SM_` name (melee tools, chests, AR parts, Delia's props).
  2. *Our rig, Tripo shape*: the Tripo static mesh carried onto our existing skeleton in Blender (`looter_tripo_rig`: QuadriFlow,
     bone-heat weights, bone maps; hit hulls kept). Used for the spider, Spitter, Gravemother, Unpaid, Stone Sentinel. No
     Tripo rigging credits (30 each), but heavy Blender work each.
  3. *Mannequin skeleton*: Tripo rigs a humanoid with spec "unreal"; worn as the body (`WardenCrewBody`,
     `UBodyFigureComponent`, retargeter `Tools/Unreal/build_hero_retarget.py`). 55 each.

## Already Tripo (nothing to redo)

| Name | Where | Notes |
|---|---|---|
| AR bullpup parts, 24 (4 sets x 6 slots) | `Art/Models/Weapons/Tripo/parts_set1-4.py`, `Bullpup.py` | the other 27 Bullpup parts stay scripted (see Guns) |
| Melee tools, 5 | `Art/Models/Weapons/Melee/parts_tripo.py` | Grave Spade, Razor Fan, Ferry Gaff, Piston Fist, Rock Pick |
| Loot chests, 5 tiers | `Art/Models/Loot/ChestTiers.py` | |
| Spider and spiderling, Spitter, Gravemother, Unpaid, Stone Sentinel | `Art/Models/Creatures/Tripo/parts_*.py` | Tripo shapes on our rigs; the Meadow Wolf GLB is the spider's source |
| Wardens set, 22 models | `Art/Tripo/*` | rigged to the mannequin: Delia, Holloway, Finch, Junie, 2 Dredgers. Static: Delia's shotgun, Company rifle, hose nozzle, hose intake, hose, Great Lantern, Lamplighter, Dredge barge, Company tender, grapple hook, anchor winch, barricade table, loudspeaker post, lantern post, Abel's hat. Made, not all imported yet |

## 1. Heroes (player bodies)

Current: scripted in Blender from MakeHuman (CC0) with draped and simulated cloth, baked textures, on the mannequin's 63
bones plus twist bones and **swing bones** (Ellis 50 in 21 chains, Crane 144 bones in all, Odessa 92; Pike; Gauge is
hard-surface). Worn over the hidden, still-animated mannequin through an IK retargeter (`UBodyFigureComponent`,
`RTG_MannyTo<Hero>`). `BodySwing` springs the cloth, hair, lantern and hat bones by bone name; `UHeroDefinition` holds hold
offsets, idle and aim poses fitted to each body; the hero select stage and `WardenCrewBody` show them; tests `HeroTests`,
`BodySwingTests`, `Looter.Pose.*`, `fit_audit.py`.

| Name | Source | Used by | Effort | Credits |
|---|---|---|---|---|
| SK_Ellis | script (`Characters/Ellis.py`) | third-person body (F5), hero select, cutscenes | Hard | 55 |
| SK_Odessa | script | same | Hard | 55 |
| SK_Crane | script (hook, toll lantern) | same | Hard | 55 |
| SK_Gauge | script (hard-surface robot, steam vents) | same | Hard | 55 |
| SK_Pike | script (pack, spyglass) | same | Hard | 55 |
| SK_FP_Arms and 4 hero variants | script, 46 bones | first-person hands and grip math (`FirstPersonHands*`) | Hard | 0 (keep: Tripo cannot make these; re-texture to match the new bodies) |

Why Hard: a Tripo body on the "unreal" skeleton has mannequin bones only. The cloth, hair, lantern and hat swing chains and
the twist bones would be lost unless added to the mesh in Blender, and the hold, idle, aim and camera numbers are fitted
per body. The mannequin-people path exists, so a hero could ship without swing bones first. Unknown until tried: how many
bones Tripo's "unreal" spec makes (63?), and how Gauge (a robot) rigs. Try one hero (55) before the rest. The painted
portraits are separate art and stay.

## 2. Story people

Models that exist today (all custom-rigged or static). Every other speaking character is a voice behind a door or window.

| Name | Source | Used by | Effort | Credits |
|---|---|---|---|---|
| SK_Abel (Keeper, boss then friend; hat, lantern, pump are separate meshes) | script, Unpaid's bones at 1.3x | `AAbel*`, bone pose tables, hit zones | Hard | 30 (+ lantern 30, pump 30; hat already Tripo: `Art/Tripo/AbelsHat`) |
| SK_Amos (friendly ghost; hat, fork) | script, Unpaid's bones | `AAmosWhitlock` leans, sits | Hard | 30 (+ fork 30, hat 30) |
| SK_Hiram (ore lift engineman, sits on a stool; cap) | script, Unpaid's bones | `AHiramCoyle` | Hard | 30 (+ cap 30) |
| SK_Thaddeus (undertaker with a tape; spectacles) | script, Unpaid's bones | `AThaddeusBright` | Hard | 30 |
| SK_Foreman (Cyrus Teague, boss; bowler, lamp, pick handle, satchel, fuse stick) | script on Unpaid.py's code | `AForemanCreature`, attack rules, hurt zones | Hard | 30 (+ 5 props at 30) |
| SK_DeadMiner (3 outfits; cap, hat, lamp, pail, pick) | script on Unpaid.py's code | `ADeadMinerCreature`, encounters | Hard | 30 (+ 5 props at 30) |
| SM_MisterSexton, SM_SextonStand, SM_SextonLedger (seated on a rail, cloth-simulated coat, 2,611-line script) | script | cold open, Ransom's Rest step 15 | Medium (a static mesh, but Tripo must give a seated pose) | 60 (retries likely) |
| SK_Hob (the one-eyed crow, friend; HobRoost props are in Props) | script, small rig 4.4k tris | `AHob*`, roosts, perches | Hard | 30 |

The ghosts, the Foreman and the miners share the Unpaid's bones, which the Tripo Unpaid already fits (recipe 2, 30 each,
`parts_unpaid.py` is the template). They are still in the old Unpaid look beside the new Tripo Unpaid: the biggest visible
mismatch on this list. Sub-totals: 6 bodies 180; attached props (16 meshes after Abel's hat) 480; Sexton 60; Hob 30
(Hob is counted again in the phases, not twice in the totals).

Voices only, no model today (new, not a redo; 55 each on the mannequin skeleton, path ready in `WardenCrewBody`): Tilly
Bright, Aldana, the Pruitts, Mr Sloane, Fenn, the Deacon, Lucky Ned. The gang in the cold open is flat-black mannequin
silhouettes with `SM_ColdOpen_*` hats and guns on purpose. 7 x 55 = 385, optional.

## 3. Creatures

| Name | Source | Used by | Effort | Credits |
|---|---|---|---|---|
| Spider, spiderling, Spitter, Gravemother, Unpaid, Stone Sentinel | already Tripo on our rigs | | done | 0 |
| SM_GravemotherStone_0..2 + 5 shards, SM_StoneChip_1..3, SM_StoneSentinelPlinth | script | headstone states attach to her abdomen bone and crack in sequence | Hard | 0 (keep) |
| SK_Slime (clear gel on M_Gel, bone-scale squash, core crits) | script | `ASlimeCreature` | Hard | 30 (not recommended: Tripo cannot do translucent gel) |
| SK_BarnCat | script | porch ambience, posed in code | Hard | 30 |
| SK_HenBrown, SK_HenWhite, SK_Rooster | script | yard ambience | Hard | 90 |
| Fauna pieces: crow, swallow, cliff swallow, sparrow, hawk (perched, flying, wings, head), butterflies, dragonfly, firefly, fly, tumbleweed, dust devil, wind cloth (about 45 meshes) | script, one painted atlas | `World/Fauna*` draws instances, turns each piece on its own pivot | Hard | 0 (keep: tiny, instanced, procedural by design) |
| SM_UnpaidBell, SM_UnpaidBellRope | script | Bell-Ringer, pendulum from the fist | Medium | 0 (keep) |

Sub-total (cat, hens, rooster, slime): 150.

## 4. Guns and parts

Every gun is assembled from interchangeable part models; the body carries the sockets (`Barrel`, `Magazine`, `Sight`,
`Stock`, `Muzzle`, `Grip`, `Foregrip`, `Aim`). Parts are saved by key (never rename or reuse one). In first person the gun
is on screen all the time: the most visible art in the game.

| Family | Parts in game | Source | Effort | Credits: common-tier set (6 parts) / AR-style (24 parts) |
|---|---|---|---|---|
| Bullpup AR | 51 | 24 Tripo (6 slots x 4 sets); 27 scripted (4 bodies, 4 barrels, 4 muzzles, 4 sights, 4 stocks, 7 magazines, 3 of them recolours) | done; rest Medium | 0 / 720 for the rest |
| Ranchhand pump shotgun | 52 (+ pump) | script; Tripo views for 24 parts already rendered (`Art/Concepts/Weapons/Shotgun`) | Medium (pump slides) | 180 / 720 |
| Drover revolver | 39 | script; cylinder turns per shot and swings out on a `Crane` socket, grip carries the hand sockets | Medium-Hard | 180 / 720 |
| Longhorn sniper | 52 | script; the bolt cycles | Medium | 180 / 720 |
| Powderkeg rocket launcher | 44 | script; rocket kinds are the magazine, plus the flying rocket | Medium | 180 / 720 |
| Rustler SMG | 47 | script; magazine has a glowing spiral window | Medium | 180 / 720 |
| Thresher LMG | 48 | script; heat rings glow, belt box | Medium | 180 / 720 |

Six starter sets 1,080; all six AR-style 4,320; Bullpup remainder 720. Each new part needs the maker paint slot, element
glow and rarity accent slots (`GunPolymerSand`, `GunAccentGlow`...), the hand markers (`looter_guns.hand_mark`) where it
carries a grip, and the Blender fit (`looter_tripo.py`).

## 5. Held and loot items

| Name | Count | Source | Used by | Effort | Credits |
|---|---|---|---|---|---|
| SM_SoulFlask, SM_SoulFlaskUncorked | 2 | script | first-person drink animation (hands code) | Medium | 60 |
| Grenade jars: Cinder, Pitch, Scatter, Salt grenade | 4 | script | thrown jar in the hand (15 cm model-size test) | Medium | 120 |
| Ammo pickups (AssaultRifle, Pistol, Rocket, Shotgun, SMG, Sniper) | 6 | script, modeled from the inked icons | loot drops | Easy | 180 |
| SM_SilverCoins | 1 | script | loot | Easy | 30 |
| Ward boxes (Common to Legendary shields) | 5 | script | shield pickups | Easy | 150 |
| Old SupplyCrate and Strongbox (+ lids, wheel) | 2 | script | replaced by the Tripo chest tiers | Easy | 0 (retire instead) |

Sub-total 540.

## 6. Props (static meshes placed by build scripts, mostly Easy)

Counts from `Content/Art/Props` (268 SM). Redo counts are distinct looks; variants share a kit and a tint.

| Family | Meshes now | Source | Used by | Effort | Redo credits |
|---|---|---|---|---|---|
| Graves: headboards (13), crosses, mounds, coffins (8), bier, burial deck, memorial, Vigil stone (bell, flames), wreath, relics | about 45 | `Props/Graves.py`, `BurialDeck.py`, `VigilStone.py`, `GraveFight.py` | graveyards, respawn graves, fights | Easy; Medium for lidded coffins and the Vigil stone | 660 (22 distinct) |
| Farm and yard: carts, dray, buckboard, burnt wagons, wheelbarrow, hay, barrels, crates, milk cans, sacks, firewood, lumber, washtub, trough, pump, coop, woodshed, lean-to, chopping block, kitchen garden, flower box, laundry line, mailbox (door), small clutter | about 40 | `FarmProps.py`, `Containers.py`, `FrontierClutter.py`, `VillageProps.py` | Landfall farm, town backyards | Easy | 900 (30 distinct) |
| Town dressing: awnings (3), signboards and posts (7), notice boards (3), station board, town gate, lamp post, porch lantern, rocking chairs, porch chair, bench, gunsmith bench, bunting, crepe | about 28 | `Boardwalk.py`, `TownGate.py`, `VillageProps.py` | Main Street | Easy-Medium (emissive slots, sockets) | 540 (18 distinct) |
| Lanterns and lights: Keeper's lantern and post, lantern bail, mine lanterns (2); the Wardens' lantern post is already Tripo | 6 | `LanternPost.py`, `Mine.py` | respawn posts, lit windows | Medium (glow slot, light socket) | 180 |
| Railway: ore car, signal and arm, buffer stop | 4 | `Railway.py`, `Tramway.py` | station, tram | Medium (signal arm moved by code) | 90 |
| Nest pieces: cocoons (2), egg sacs (4, one bursts) | 6 | spider props | spider dens | Medium | 180 |
| Loot containers: footlocker (lid), payroll chest, loot coffin (lid), loot grave (lid, shovel), smashed reliquary | 5 | `Lootables.py` | `AChest` lootables with opening lids | Medium (lid split, pivot) | 150 |
| Mission readables, keys, hats, tags, badges, books, maps (CodeBook, Journal, PayrollBook, ReachesMap, DeaconLetter, CellarKey, MinersTag, SheriffBadge/Hat...) | about 25 | `MissionProps.py` | pickups, readable screens | Easy | 750 (skip: mostly seen as UI cards) |
| Ruins, cairns, dressed blocks, campfire ring, fallen pine, skiff jetty with bell/post/slate | about 15 | `Ruins.py`, `SkiffJetty.py` | scenery | Easy | 450 (skip) |
| **Keep:** fences and walls (13 + 8, snapping lengths and end caps), boardwalk and track pieces (4 + 11), breakables (24 fracture chunks per barrel/crate set), cold-open silhouettes (12), spider webs (13 alpha cards), slag pools, canyon fog, smoke plumes, wind wisps | about 90 | scripts | | modular or FX; worse in Tripo | 0 |

Sub-total 2,700 recommended, 3,900 with readables and ruins.

## 7. Buildings

Scripted with `looter_buildings.py`: walls with openings, roofs, windows and doors on the house trim sheet, UCX hulls,
sockets (lamp, smoke, porch), baked occlusion; build scripts add lights, smoke and doors. A Tripo building is one closed
shell: porches, openings, collision, sockets and the separate moving meshes (shutters, screen door, chapel bell, mill
fan, lift cage) must be rebuilt in Blender around it. Most homes are closed and solid; the chapel has an interior.

| Group | Meshes | Used by | Effort | Credits |
|---|---|---|---|---|
| Landfall farm: SM_Farmhouse_Ransom, SM_Barn_Ransom, SM_Cottage_Ransom | 3 | Delia's homestead, the opening of the Wardens story | Medium-Hard | 90 |
| Ransom's Rest civic: Depot, Chapel (+ bell), Workshop Bright, Barn Whitlock, Smithy, Hotel/Assay/Barber fronts, Watchtower, Windmill (+ fan) | 10 | main street, missions | Hard | 300 |
| Ransom's Rest homes A-J | 10 | valley | Medium | 300 |
| Small town pieces: Outhouse, Well, Lean-to, Coal stage, Powder house, Ranger cabin, Ore bin, Water tower, Keeper's Bridge | 9 | scenery | Medium | 270 |
| Legacy island/Skyreach/tutorial: Barn, Cottage, Farmhouse, Log cabin, Lookout (+ tower), Depot, Chapel, Well, Windmill, Outhouse, Coffin shed, Water tower, Bridge, Stairs, Gun rack, Mooring post | about 25 | tutorial island (a session's fallback), Skyreach | Medium | 750 (skip) |
| **Keep:** the mine kit (20 modular pieces on `Mine.layout.json`), ore lift pieces, tag boards, false fronts, door and shutter parts | about 30 | | Hard to snap | 0 |

Sub-total 960 (+ 750 legacy).

## 8. Vehicles

| Name | Source | Used by | Effort | Credits |
|---|---|---|---|---|
| SM_Skiff_A_Packet, SM_Skiff_A_Gang (+ SM_Skiff_Gangplank: hinge at origin, raised by code) | `Vehicles/Skiff.py` | cold open, tutorial departure, jetty | Medium | 60 (hulls only; gangplank stays) |
| SM_Locomotive_B, SM_PassengerCar, SM_HearseCar (+ door); wheel sets and coupling rod | `Vehicles/Train.py` | `ATrain` (wheels and rods driven by `TrainRunningGear`) | Hard (wheel and rod cutouts) | 90 (bodies only; wheels, rods, door stay scripted) |

Sub-total 150. (The Wardens ships, Lamplighter, barge and tender, are already Tripo.)

## 9. Procedural by design: leave alone

| Group | Count | Why |
|---|---|---|
| Terrain tiles, rings, backdrops, canyon walls, water (Ransom's Rest, Terrain Test, Tutorial Island) | about 115 | built from layout and height data; collision cooked from the Nanite fallback; macro textures, ground fit, minimap tags. Tripo cannot tile or share seams |
| Rocks: boulders, outcrops, cliff faces and panels, scree, far rocks (49, with sky islands) | 49 | scripted to fit the cliffs; PCG scatter with ground fit; hundreds of instances |
| Vegetation: pines, oaks, birches, junipers, bushes, grasses, flowers, reeds, logs, stumps (62) | 62 | wind weights in vertex colours, leaf cards, LODs, PCG scatter; Tripo trees would not sway |
| FX meshes: waterfall and mist cards, canyon fog, smoke plume, wind wisps, slag splats, Vigil flames, dust devil, mound burst, webs | about 30 | translucent or scrolling cards on the FX masters, vertex-colour driven |
| Old Skyreach assets (`Content/Environment`: clouds, crystals, floating islands) | 176 | the old level, kept |
| Cloud cards, decals, mist (`Art/Effects`) | | textures and materials, not models |

## Totals

| Group | Credits |
|---|---|
| Heroes (5 bodies) | 275 |
| Story people: 6 bodies 180, 16 attached props 480, Sexton 60, Hob 30 | 750 |
| New people (voices only today, optional) | 385 |
| Creatures (cat, hens, rooster, slime) | 150 |
| Guns: Bullpup remainder 720; six starter sets 1,080 now; all six AR-style 4,320 | 5,040 (full) |
| Held and loot items | 540 |
| Props (2,700 recommended + readables 750 + ruins 450) | 3,900 |
| Buildings (960 + legacy 750) | 1,710 |
| Vehicles | 150 |
| **Everything, full** | **12,900** (about 16,100 with 25% retries) |

## Recommended order (most visible per credit first)

| Phase | What | Credits | Cumulative |
|---|---|---|---|
| 1 | Guns: common-tier set (6 parts) for the 6 families 1,080 (on screen every second; pilot with the Rustler or Drover). Flask and 4 jars 180. Heroes 275 (pilot one hero first: 55). Story people: Abel and the Foreman first (bosses), then Amos, Hiram, Thaddeus, DeadMiner 180 + 5 key props 150 + Sexton 60. Landfall farm 90 and Ransom's Rest civic 300. Graves and headboards 660 | 2,975 | 2,975 |
| 2 | Farm and yard props 900, town dressing 540, lanterns 180, homes 300, small buildings 270, skiff and train bodies 150 | 2,340 | 5,315 |
| 3 | Hob and small animals 180, loot containers 150, nests 180, railway 90, ammo, coins and ward boxes 360 | 960 | 6,275 |
| 4 (low value; only with plenty of credits) | rest of the gun families 3,240 + Bullpup remainder 720, readables 750, ruins and jetty 450, legacy island buildings 750, new people 385, the other 330 of story props | 6,625 | 12,900 |

Recommended first three phases: **about 6,300 credits**
(about 7,600 with retries); phase 1 alone about 3,000.

Why this order: the gun and the hands are on screen all the time; bosses and mission givers are looked at from a metre;
the Landfall farm and Main Street are the first places seen after the Wardens opening. Small unlit scenery comes last.

## Cautions

- Tripo cannot make: translucent gel (Slime), modular kit pieces with snapping ends, terrain, cards with wind, or anything
  that must open or turn in separate pieces unless it is cut apart afterwards in Blender (chest lids, cylinders, bolts,
  shutters, the signal arm). The effort column counts that.
- Every earlier swap needed a Blender fit script and re-pinned tests (`Weapons.Parts`, `Melee.Tool`, `Creatures.*`,
  `Heroes.*`): the Blender and test work costs far more than the credits. Do one pilot per recipe before a family.
- Run `Tools\perf.ps1` after each family: Tripo meshes come at 10-40k triangles against our 1-5k, and the target is 120 fps
  on an RX 580 at Medium (Nanite fallback). Budget goes to the hero and boss models and the first-person guns.
- Hero swing bones, the hands code and the FP arms are the main risk for heroes; ask before touching them.
