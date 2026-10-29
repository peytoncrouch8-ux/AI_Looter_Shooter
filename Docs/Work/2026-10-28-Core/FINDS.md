# FINDS: things to find off the path (plan, 2026-10-28)

The user's complaint (playthrough 2026-10-21): "nothing to find off the path". Their 2026-10-28 answer (CONTRACT.md): finds pay a
mix: better chests and rare guns, lore and XP, a few unique named guns for the best-hidden. Build overnight with Main's calls.
Layout cm X north, Y east; metres in brackets are (x east, y north) as the area docs write them.

## 1. What exists, and why it still reads as "nothing"

- Nine secrets since 2026-10-22 (`Tools/Unreal/build_area_secrets.py`, `story_texts.SECRETS`, folder `Gameplay/Secrets`,
  each readable sets `Secret.<Id>`): Widow's strongbox (T5), Mooring locker, Surveyor's tin (Coffin Rock top, slab ramp),
  Averys' tin, Thaddeus's bottle, Keepers' stores, Spiders' hoard, Teague's powder box, Derrick crate. 7 of 9 are footlockers
  (1 gun, Luck 0.5-1.0); plus 6 Ledger pages and 2 Deacon letters (lore, mostly on mission paths).
- Missing, so they don't register: (a) no tell from afar (a footlocker behind a rock is invisible); (b) no moment (a
  footlocker opens like any mailbox: no banner, no sound, no XP); (c) no count (nothing says there are more); (d) no
  traversal: every one is a walk-up except Coffin Rock; (e) payouts below a camp crate (camps are T3).
- Fix = one find system over all of them (tally, moment, tell, map count) + 6 new finds that use climb, crouch, swim, a
  map clue, a guarded grave and a wandering elite. 15 finds in the tally; Ledger pages counted on their own line.

## 2. The fifteen finds

Columns: id (FindId, saved as flag `Find.<Id>`: never rename) | where | tell (how you notice) | reach | pays. XP per §3.
Regions (the map's rows): Farm & Point 4, West Road 3, Town & Boot Hill 2, South Fields 3, The Mountain 3.

Existing (E: chest ids kept; only an AFindSpot, a tell and loot changes added):
- **E1 WidowsBox** "The Widow's Strongbox": north side of widow2Rock [400,-7700] (-77, 4), Farm & Point. Tell: glint off the
  box's brass seen from the keeper's path past the slab; 2 circling crows. Reach: step off the path round the slab. Pays: T5
  as now (3 guns Epic+, legendary odds kept).
- **E2 Mooring** "The Gang's Locker": Mooring Ledge by mooringPost [-10000,-10100] (-101,-100), Farm & Point. Tell: Hob's hint
  at Main 2's turn-in (exists) + crows over the ledge. Reach: cliff stairs from the lookout. Pays: override 2 guns Luck 1.0,
  2 ammo (was 1 gun 0.5).
- **E3 CoffinRock** "The Surveyor's Tin": Coffin Rock top [-6000,-1400] (-14,-60) +7.7 m, South Fields. Tell: the butte's dead
  tree + crows; the slab ramp on its north face. Reach: walk the ramp (PIE-check it: the 10-22 note says "walk it in PIE").
  Pays: 2 guns Luck 1.0; its tin now also holds **Pike's field-book scrap** (readable `Find_PikeScrap`), which starts N4.
- **E4 Chimney** "The Averys' Savings": Burnt Homestead hearth [4900,-7200] (-72, 49), West Road. Tell: the town mutter after
  the gate (exists) + chimney crows. Pays: footlocker as now + 45 silver (savings: silver fits).
- **E5 Bottle** "Thaddeus's Bottle": springhouse [-1600,8400] (84,-16), South Fields. Tell: Thaddeus's word after Knocking
  Below (exists). Pays: as now (1 gun Luck 0.8).
- **E6 LanternHouse** "The Keepers' Stores": old lantern house [3500,-10200] (-102, 35), West Road. Tell: crows on the ruin.
  Pays: as now + 2 grenade tins (the kind rolled as drops roll it).
- **E7 Hoard** "The Spiders' Hoard": under the hanging tree [4650,2000] (20, 46), Town & Boot Hill. Tell: webbing + crows on
  the tree (it is a crow perch already: crows circling read as "something here"). Pays: as now (1 gun Luck 0.7).
- **E8 Powder** "Teague's Powder Box": behind benchGraniteG [20200,-7800] (-78, 202), The Mountain. Tell: glint. Pays: 2 guns
  Luck 1.0 (it proves Odell's report: worth more).
- **E9 Crate** "The Fallen Crate": quarry blocks [8413,2413] (24, 84), The Mountain. Pays: as now.

New (N: new chest ids, never renamed once placed):
- **N1 Tarn** "Where They Can't Wade": the bed of Larkspur Tarn (centre [20000,-2000], radii [600,900], water 4930, bed 4760:
  1.7 m deep, the swim), The Mountain, zone 5-6. Story: a Ranger cache Ruth sank where the dead can't follow (creatures never
  follow into deep water: `USwimWaterSubsystem::IsDeepAt`). Tell: a stake on the north shore [20650,-2000] (`SM_FencePost`)
  with a rope (`SM_HangingRope`, laid down the bank into the water) and a Ranger cache tag (`SM_CacheTag`) on it = readable
  `Find_RangerTag` ("R.C. Where they can't wade."); the box's glint under the surface. Reach: swim out, dive (crouch) along the
  rope, open on the bed (breath 22 s: ample). Chest `Find_TarnLockbox` ChestTier4 (two Rare+) + named gun **Still Water**
  (Longhorn sniper, Epic, "The dead don't wade.").
- **N2 BoltHole** "The Bolt-Hole": inside the Tumble, the rockfall under Ransom's Point's east face (bb X -10000..-7000,
  Y -7200..-3400; hollow near [-8600,-5200], (-52,-86)), Farm & Point. Story: where the gang waited for dusk on the seventh
  job (they came up by skiff to the Mooring Ledge, which the Tumble closes). Tell: a lantern's warm glow through a dark gap
  (`SM_PorchLantern`, emissive only, tag Flicker), seen from the farm terrace and the bluff path's lower bend. Reach: a crawl
  gap under a leaning `SM_FallenSlab_A` between two `SM_Boulder_D`: underside 130 cm over the ground (crouched capsule
  2 x 68 x 0.85 = 116 cm passes, standing 163 does not); crouch-walk or slide in; a 3 x 3 m hollow behind. Chest
  `Find_BoltHole` ChestTier4 + readable `Find_MarkedCard` (Ned Purcell's nicked ace pinned with a knife: a seed toward the
  Lily's rigged card game; Hob: "Purcell's. He never could leave a deck alone, sunshine.").
- **N3 RangerPost** "Ranger Post Three": the top of the Rimrock's middle piece (rimrockMiddleRock bb X -3727..-2573,
  Y -11003..-10377, 6 m) on the Rim's lip west of the farm, Farm & Point. Tell: a Ranger cache tag on a stake at its east
  foot [-3150,-10250] + 2 crows over the top; from the farm yard the tag reads against the sky. Reach: four flat-topped steps
  up its east face (`SM_Boulder_D` scaled flat, tops at 1.25 / 2.5 / 3.75 / 5.0 m, then the top at ~6 m: each rise <= 1.25 m,
  under the 1.5 m mantle) with a hidden cube collision slab on each top (the Coffin Rock lesson: rounded tops slide the player
  off). Top: chest `Find_RangerPost` ChestTier3 + readable `Find_RangerNote` ("RANGER POST 3. WATCH THE NOTCH AT DUSK. R.C.");
  the vista over Gravewind Canyon at sunset. The Rim beside it is Hob's catch (fall recovery, exists).
- **N4 PikesCache** "Pike's Cache" (map clue -> dig): the scrap from E3 reads "FIELD BOOK, W. PIKE. FROM B.M. 112, S. 40 E.,
  26 YARDS, TO THE NOSE'S FOOT. INSTRUMENTS, POWDER, MY GOOD RIFLE. DIG BY THE RED STONE." The spot: the Nose's north foot
  [-7900,450] (4.5,-79), ~26 m south-east of Coffin Rock's top (the bearing is honest: the scrap is a real clue), South
  Fields. Tell: a lone rust-red stone (`SM_Rock_C`, an MI of the rock master tinted oxide) + a knee-high survey cairn
  (`SM_Cairn_A` at 0.6) + a dug-over mound. Reach: walk, read the bearing, hold Interact to dig. Chest `Find_PikesCache`:
  kind Grave (the hold, the mound sinking, the box heaving up) with a variant body `SM_Footlocker`/`_Lid` (AChest keeps a
  script's model that isn't a kind's own), override 2 guns Luck 1.0, 2 ammo + named gun **Bench Mark** (Bullpup AR, Rare,
  "One hundred and twelve feet above the trouble."). Diggable whether or not the scrap was read.
- **N5 MiserGrave** "The Miser's Fare" (named grave + ambush): a headboard set apart past boot hill's third row (rows bb
  X 2900..3770, Y -750..350; spot [3950,450], (4.5, 39.5)), Town & Boot Hill. Readable headboard `Find_MiserBoard`
  (`SM_Grave_Headboard_OldC`): "EZEKIEL MOSS. HE TOOK HIS FARE WITH HIM." (the old fare: grave gold on the eyes; canon).
  Tell: `SM_GraveCoins` glinting on the mound; Tilly's door mutter after Main 3 ("Old Moss was buried with his fare. Coin on
  each eye. Pa wouldn't hear of it."). Reach: walk up; stepping onto the plot (3.5 m round it) springs ambush **MiserPlot**:
  4 Unpaid + 1 Restless rising from the rows (after Main 3; an encounter entry, see §5 lane E). Dig (Grave kind, hold):
  override 1 gun Luck 1.0 + 60 silver (grave gold) + `SM_GraveCoins` left in the open grave.
- **N6 Tollman** "The Tollman" (rare wandering elite): one named Gravebound (Epic) Unpaid, level 6, HealthScale 2.0, with
  2 Basic, walking the cairns road from the keeper's grave [1600,-11150] toward the Keeper's Gate (turn at Y -12300: never
  into Gravewind Point's boss ground) and back to the lantern house [3500,-10200], West Road. Active after Main 4; a lair:
  `AEncounterSpawner::LegendaryId` "Tollman" (gone after a kill, back on an arrival 20 min of play later: existing
  `SessionSubsystemLegendary.cpp`), so it is always there the first time. Creature tag `Find_Tollman`. Tell: the Pruitts'
  door news after Main 4 ("Somebody's collecting on the keeper's road. Wears a coin on each eye."). Pays on the first kill:
  its Epic-rank drop + named gun **Ferryman's Due** (Drover revolver, Epic, "Exact change only.") at the body.

Ledger pages (6, `LedgerPage_<Town>`): kind Lore, banner "LEDGER PAGE n/6", own map line; not in the 15.

## 3. The find system (the contract the lanes share)

- **Saved:** `FCampaignRecord::StoryFlags` `Find.<Id>` (session v4, no migration). Legacy credit, silent (no banner, no XP):
  on the first poll 1 s into play, a find whose source chest `IsUsedUp()` or whose readable's `SetsFlag` (`Secret.*`,
  `Ledger.Page.*`) is set counts as found.
- `World/FindSpot.h/.cpp` `AFindSpot` (hidden data actor, no tick, no collision; placed by `build_area_finds.py`, tag `Find`):
  `FName FindId`; `FText DisplayName`; `FText Region`; `EFindKind Kind` {Cache, Hidden, Grave, Elite, Lore};
  `TSoftObjectPtr<AActor> Source` (the chest or readable); `FName SourceEvent` (e.g. `Readable.Find_RangerTag`; optional);
  `FName SourceTag` (Elite: the creature's tag); `int32 Silver` (level-1 amount, grown as `LooterEconomy` grows kill
  silver; paid as an `ASilverPickup`); `FName NamedGun` (`DA_Named_<Id>`); `TArray<FStoryLine> Lines` (Hob after the banner, through
  `UCaptionSubsystem::Play`); `FVector GlintOffset`, `bool bGlint` (default true); `int32 Crows` (2; 0 none),
  `float CrowHeight` (700 cm over the spot); `TArray<TSoftObjectPtr<AActor>> Tells` (hidden once found: lanterns, coins).
- `World/FindRules.h/.cpp` (plain, tested): `FlagFor(Id)` "Find.<Id>"; `XpFor(Kind, ZoneLevel)` = base x (1 + 0.35 x
  (ZoneLevel - 1)), base Cache 100, Grave 160, Hidden 220, Elite 300 (on top of kill XP), Lore 40; `FFindTally {Found,
  Total, ByRegion}` from the spots and the record (Lore kept apart: `CountsInFinds`); `BannerCount` "4 / 15"; namespace
  `FindCue` {`World.Find.Discover`, `World.Find.Complete`, `World.Find.Glint`} (own header like `SwimCue`: no edit of the
  shared `LooterSoundCues.h`, which GUNS/HITS touch).
- `World/FindSubsystem.h/.cpp` `UFindSubsystem` (tickable world subsystem; game worlds only): spots register at BeginPlay;
  hears `UMissionRunner::OnEvent` (Interact on a Source; a named `SourceEvent`) and `UPlayerProgressionSubsystem::OnKillXP`
  (victim tagged `SourceTag`). A chest source counts when `HasGivenLoot()` turns true (polled 4 Hz while one opens, so the
  banner follows the lid, not the tap). On a find: set the flag; `ALooterHUD::ShowFind`; cue `World.Find.Discover` 2D;
  the spot's crows `Lift` and leave, Tells hidden; silver as `ASilverPickup` at the source; the named gun once
  (`MissionRewards::DropNamedGun` near the source; `EnsureNamedGun` keeps it single); XP `AddXP(.., EXPSource::Find)` when
  the banner ends (so LEVEL UP follows the find, never cuts it); `USessionSubsystem::SaveSoon` as a roost find does. 15/15: "EVERY FIND IN RANSOM'S
  REST", `World.Find.Complete`, +1,000 XP, Hob: "That's all of it, sunshine. Even I didn't know about the tarn."
  `FOnFindFound OnFound(Id, Found, Total)`; `IsFound`, `GetTally`, `GetLoreTally`, `MarkFound(Id, bAnnounce)`, `Forget`.
- **Tells (in play, none baked):** each unfound spot with `Crows > 0` spawns an `ARoostCrows` (deferred) with
  `MakeCircling(Spot + CrowHeight, Crows)`: low, slow, no beacon column, so never read as a roost (roosts: high over a red
  column); culled past `URoostSettings::CirclingCullDistance` 150 m. Glint: one pooled card (M_FX_Shape MI, additive is fine:
  not Nanite) at `GlintOffset`, a 0.3 s star every 3.5-5 s (seeded) while the player is 8-45 m off with a clear line and
  the sun's side lit; at most 2 at once; `World.Find.Glint` (a faint tink) the first flash within 15 m; the subsystem ticks
  per frame only while a glint plays. `Looter.Finds.Tells 0/1`.
- `Progression/PlayerProgressionSubsystem.h`: `EXPSource::Find` appended (one line).
- **Banner** `UI/HUD/HudFindBannerWidget.h/.cpp` (built like `UHudGraveMarkedWidget`, 640 x 150, 2.8 s, floating outlined
  text, no panel): the kit's `TallyBrush` inked tally in orange, "FOUND" in orange between cyan rules, the name in white,
  "4 / 15 · RANSOM'S REST" and "+N XP" counting up in cyan under it; Lore: "LEDGER PAGE · 2 / 6". Queue; waits for menus,
  scenes, pause, the level-up, mission-complete and roost banners (and the roost banner waits for it: one line in
  `UHudGraveMarkedWidget::SetOtherBanners`). Hook `ALooterHUD::ShowFind(const FFindBannerShow&)` ->
  `UPlayerHUDWidget::ShowFind`. Colours only `LooterUI::Color`; no `MarkBackground` needed (no backing).
- **Map count** `UI/Inventory/MapWidgetFinds.cpp` (new; one call from `MapWidget.cpp`'s build and refresh): under "MAP ·
  NORTH UP": "FINDS 4 / 15" with the tally brush, "LEDGER 2 / 6"; at the travel column's foot the five regions "Farm &
  Point 2 / 4" (a region done turns cyan with a tick). No pins (the user's note 26: chests off the map).
- **Console** `World/FindCommands.cpp`: `Looter.Finds.List` (id, region, found, source state), `.Mark <id>`, `.Forget <id|all>`,
  `.Goto <id>` (sets the player 6 m off, facing it; an ambush's ground is never teleported into), `.Tells 0/1`.
- **Pacing:** the 15 at their zone levels ~4,450 XP + ledger ~460 + completion 1,000: the full clear 50,900 -> ~56,800, level
  7.5 -> ~7.79 (L7 41,650, L8 60,620); the story path +~300 (pages on it): 6.91 -> ~6.93. Keep the full clear under 7.8
  (the Lily's floor is 8): if `Looter.Progression.Pacing` says over, drop the completion bonus to 500.

## 4. How it is built (data, scripts, assets)

- **Data:** `layout.json` `gameplay.finds` (new key; positions only): `{about, regions:[5 names in order], spots:[{id, kind,
  region, source:{chest|readable|creatureTag}, chest:{id, kind, at, yaw, onBed?, variant?}, readable, props:[{mesh, at, yaw,
  scale, tags}], steps|gap|rope (per-find builder args), glint:[x,y,z], crows}]}`. Existing E1-E9 entries carry only
  `id, kind, region, source:{chest:<Secret_ id>}, glint, crows`. Words (display names, Lines, readables, mutters) live in
  `story_texts.py`: `FINDS` (id -> name, lines), `READABLES` (the new six), `MUTTERS`/`TOPICS`; town news in
  `World/TownLifeRules.cpp`. Loot changes to E2/E3/E8 and the new chests' overrides: `story_texts.SECRETS` and the finds
  block; tiers in `chest_tiers.py` TIERS (`Find_TarnLockbox` 4, `Find_BoltHole` 4, `Find_RangerPost` 3).
- **Script** `Tools/Unreal/build_area_finds.py` (new; `build_area_secrets.py` is at 478 lines and must not grow): hooked in
  `build_area.py` gameplay() right after the secrets line (524); alone: `Tools/console.ps1 "py C:/Dev/AI_Looter_Shooter/
  Tools/Unreal/build_area_finds.py RansomsRest"` clears and refills `Gameplay/Finds`. Reuses `build_area_secrets`' `trace_hit`,
  `is_clear`, `ring_spot`, `readable`, `slab_pose`, `story.actor_class`. Per-find builders: `place_tarn` (chest on the bed by
  a down-trace under the water, its top >= 40 cm under the surface; stake + rope + tag), `place_bolt_hole` (probe the Tumble's
  boulders for a 3 x 3 m pocket, set slab + two boulders, a hidden cube floor if the ground is rough, check 125-140 cm
  clearance by traces up), `place_ranger_post` (four steps + hidden tops, like `place_steps`), `place_pikes_cache`,
  `place_miser_grave`, and an `AFindSpot` for every entry (E and N and the 6 Ledger pages, Source = the placed actor by label).
  Rock pieces of one find in one `AInstancedProps` per mesh (instances, not draws); solid ones tagged Obstacle (minimap).
  Logs one line per find; warns and skips a find whose spot fails (never a silent gap: `Looter.Finds.Level` catches it).
- **Assets: all existing** (the user's rule): SM_ChestTier3/4, SM_Footlocker(+Lid), SM_FallenSlab_A, SM_Boulder_D,
  SM_Rock_C, SM_Cairn_A, SM_FencePost, SM_HangingRope, SM_CacheTag, SM_PorchLantern, SM_GraveCoins,
  SM_Grave_Headboard_OldC, SM_LedgerPage/SM_DeaconsSlip (scrap, card and note models stand in for paper; the readable's card
  carries the words), engine Cube (hidden collision). New: one MI (rust-red rock tint), three sound recipes
  (`Art/Sounds/recipes/finds.py`: Discover = a struck tin bell over a low plucked open fifth and a crow's far caw, 1.6 s;
  Complete = the same a fourth up with a second bell, 2.4 s; Glint = a soft high tink, 0.15 s; `Tools\sounds.ps1`,
  `build_sound_bank.py`, cues.json), three named guns (`create_named_weapons.py`: parts from the guns' `.parts.csv` at
  their rarity; GUNS lane may be changing kinds' parts: pick keys after GUNS lands).
- **Run order:** C++ build (`Tools\launch.ps1 -Build`) -> `create_named_weapons.py` -> `create_story_lines.py` (only if a new
  line set) -> `create_hero_lines.py` (only if a new row: write shared lines with no family words, so none needed) ->
  `Tools\sounds.ps1` + `build_sound_bank.py` -> `build_area.py RansomsRest gameplay` (never the mountain script alone: drops
  the Foreman's stone) -> `set_chest_tiers.py` -> tests -> PIE -> perf.

## 5. Lanes (disjoint files; Main writes the §3 contract into CONTRACT.md first)

- **A. Find core (Opus):** `World/FindSpot.h/.cpp`, `World/FindRules.h/.cpp`, `World/FindSubsystem.h/.cpp`,
  `World/FindCommands.cpp`, `Progression/PlayerProgressionSubsystem.h` (the enum line), `Tests/FindRulesTests.cpp`,
  `Tests/FindWorldTests.cpp`. Tests: `Looter.Finds.Rules` (flags, XP table, tally, Lore apart, banner words),
  `Looter.Finds.World` (test world: chest source -> found after loot, not on the tap; readable event; tagged kill; XP after the
  banner; named gun once; silver), `Looter.Finds.Legacy` (opened Secret_ chest and `Secret.*` flag -> found, silent, no XP),
  `Looter.Finds.Tells` (crows spawned for unfound only; glint cap 2).
- **B. UI + sound (Sonnet, strong on the kit):** `UI/HUD/HudFindBannerWidget.h/.cpp`, `UI/HUD/LooterHUD.h/.cpp` (ShowFind),
  `UI/HUD/PlayerHUDWidget.h` + `PlayerHUDWidgetLayout.cpp` (create, SetOtherBanners), `UI/HUD/HudGraveMarkedWidget.h/.cpp`
  (wait on the find banner), `UI/Inventory/MapWidgetFinds.cpp` + the two calls in `MapWidget.h/.cpp`,
  `Art/Sounds/recipes/finds.py`, cues.json. Tests `Looter.UI.FindBanner` (ease, queue, waits on all four banners),
  `Looter.UI.Map.Finds` (lines and regions from a fake tally). `Tools\hudshots.ps1` gets a find state. HITS may touch
  `PlayerHUDWidget*`: Main merges B after HITS' HUD edits.
- **C. Level build (Opus: placement is the tricky part):** `Tools/Unreal/build_area_finds.py`, `layout.json`
  `gameplay.finds` only, `Tools/Unreal/chest_tiers.py` (TIERS), the one hook line in `build_area.py`, `Tests/FindLevelTests.cpp`:
  `Looter.Finds.Level` (Lvl_RansomsRest: 15 finds + 6 Lore, ids unique, every source exists and is a chest/readable/tag,
  ground under each, inside `APlayableArea`, the tarn box under deep water, the bolt-hole's gap 120-145 cm, Ranger steps each
  <= 1.4 m, no find within 3 m of a story actor). One editor lane: SendMessage Main before the gameplay rebuild.
- **D. Words + named guns (Sonnet):** `story_texts.py` (FINDS, READABLES Find_PikeScrap/Find_RangerTag/Find_MarkedCard/
  Find_RangerNote/Find_MiserBoard, SECRETS loot for E2/E3/E4/E6/E8, MUTTERS/TOPICS for Tilly and the Pruitts),
  `World/TownLifeRules.cpp` news lines, `create_named_weapons.py` (Still Water, Bench Mark, Ferryman's Due). Tests
  `Looter.Lines.Table`, `.NoFamily`, the named-gun tests (`Named 4/4` group -> 7/7).
- **E. Two encounter entries, handed to the FIGHTS lane** (it owns `gameplay.encounters` in layout.json and
  `build_area_camps.py`): ambush `MiserPlot` (trigger: a 3.5 m circle round [3950,450]; 4 Basic + 1 Rare Unpaid, entrance
  Rise, after Main 3, budget optional) and patrol `Tollman` (route [[1600,-11150],[1250,-12300],[3500,-10200]], group 1
  UnpaidCreature Fixed Epic level 6 HealthScale 2.0 + 2 Basic, after Main 4, `LegendaryId` Tollman, `CreatureTags`
  [Find_Tollman], a display name "The Tollman"). If FIGHTS' named elites add a name field to `FEncounterGroup`, use it; else
  FIGHTS adds `FEncounterGroup::DisplayName` (set on the spawned creature's `DisplayName`). Tests `Looter.Encounters.Camps.
  Placement`, `.Gating`, `Looter.Progression.Pacing` (`PacingPaths.h`: the explorer path gets the 15 finds, the ambush and
  the Tollman).
- **F. Demo (Sonnet, after merge):** PIE play-check below; perf; screenshots by single `grab.ps1` grabs only.
- CODEMAP lines for every new file (each lane its own); Pipeline: `Game.md` "Finds" section, `Levels.md` (the finds block
  and builder), `Story.md` ("things to find" bullet points to Game.md); Handoff "Decided unattended" (§7 calls).

## 6. PIE check (one pass, God off, from a session at Main 4+ and one at Dead Lift+)

1. Map page shows FINDS 0/15, LEDGER n/6, five regions. 2. From the keeper's path: crows over the Widows, glint by the slab;
open E1: banner after the lid ("FOUND · The Widow's Strongbox · 1 / 15 · +N XP"), cue heard, crows leave, count 1/15.
3. Climb the Rimrock (N3) by the four steps without a slide-off; the vista. 4. From the bluff path's lower bend see the
Tumble's glow; crouch in (N2); the slab doesn't clip the first-person view. 5. Coffin Rock ramp (E3), read the scrap, walk
the bearing, dig N4: named Bench Mark drops once. 6. Boot hill: MiserPlot springs at the plot, dig after the fight, silver.
7. The Tollman on the cairns road: named bar, kill, Ferryman's Due, find 7/15; reload: he stays gone (lair). 8. Mountain: dive
the tarn rope (N1): open on the bed, loot lands on the bed and is taken while swimming (else fallback below). 9. Old save
with E-secrets opened: they count, no banner storm. 10. `Looter.Finds.List` = the map. Every step PASS/FAIL with a grab.

## 7. Calls Main makes unattended (for "Decided unattended")

- 15 finds = the 9 secrets + 6 new; Ledger pages counted apart; Deacon letters and Aldana's page not counted.
- Small new facts (the 10-22 rule was "no new canon"; these stay inside known facts): Ruth sank a cache in the tarn; Pike's
  field book; Ned's marked card in the Tumble; Ezekiel Moss (a dressing name); the Tollman (an Unpaid who wears the old fare).
- Named guns: Still Water (Longhorn, Epic), Bench Mark (Bullpup, Rare), Ferryman's Due (Drover, Epic). No special effects.
- Tells: circling crows + glint on every unfound find; town mutters for N4-N6.

## 8. Perf (120 fps target: 8.3 ms, 1080p Medium, RX 580; RR now 7.6 ms, p95 7.9)

- Budget +0.1 ms. No new lights (lantern emissive only; at most 3 shadowless lights a view stays true). New static pieces
  ~35 actors, instanced per find per mesh: <= 12 new draws in any view; the heaviest view (Ransom's Point lookout) sees N2, N3,
  E3: <= 8. Crows: 2 x 15 = 30 fauna instances, updated by `UFaunaSubsystem`'s rate rules, culled at 150 m. Glints: 2 cards.
  Subsystem: 4 Hz distance checks over 21 spots (nothing per frame unless a glint or opening runs).
- Measure: `Tools\perf.ps1 -Label "finds" -Exec "Looter.Quality Medium"` before and after; `Tools\tour.ps1` incl. the lookout,
  the watchtower and the tarn shore.

## 9. Risks

- **Tarn:** loot tossed from a chest on a 1.7 m bed (toss settle under water; taking guns while swimming is allowed: only
  fire/reload are blocked in `WeaponManagerComponent`; ammo collects by distance). Fallback: the rope's stake becomes a hold
  "Haul the rope" that drags the box up onto the north shore (a `StoryProp` swap), still counted as found.
- **Climb/crawl:** rounded rock tops (Coffin Rock lesson): hidden flat colliders; first-person near plane under the slab;
  the bolt-hole pocket must not cut into the bluff path's 5 m clearance or the Tumble's existing collision.
- **Shared files:** `layout.json` (FIGHTS edits encounters, C adds a new key: merge by key, C after FIGHTS), PlayerHUDWidget
  (HITS), named gun parts (GUNS). A build compiles every lane's half-done files: warn first.
- **Banner storm** on loading an old save or after `.Mark`: legacy credit is silent; a queue of more than 2 collapses to one
  "3 FOUND" banner.
- **Ambush on boot hill** overlaps the BootHill story fight's ground and the roaming dead after Main 4: `.Gating` test; its
  ground kept off the north road.
- **Over-telling:** 15 circling pairs could crowd the sky with the roosts' crows; if the PIE check reads them as roosts, drop
  to glint only (Q2).
- **Pacing:** the full clear lands ~7.79, under the Lily's floor of 8 only by the completion bonus's margin.

## 10. Questions for the user (morning; Main builds the recommended answer overnight)

1. The map's count: **(A, rec.) total + per-region counts** / (B) the total only / (C) total + a mark on each find
   already found / (D) nothing on the map, the banner only.
2. Tells for finds not yet found: **(A, rec.) two crows circling low + a glint** / (B) the glint only, subtle / (C) none:
   pure exploring, town mutters as the only hints.
3. The Tollman (a named elite wandering the keeper's road, back 20 min after each kill): **(A, rec.) keep him as a find** /
   (B) make him a bounty-board mark instead / (C) drop him.
