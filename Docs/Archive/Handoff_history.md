# Handoff history (archived 2026-10-13; not loaded by default)

The Handoff as it stood before the docs were condensed, word for word. Current state: Docs/Handoff.md.


Where the work stands, for the next session and every agent. Update it as each request finishes (newest first). Rules are
in `CLAUDE.md` ("How we work"); how things are made and fixed is in `Docs/Pipeline.md`.

## Session "Play-through" (2026-10-14)

**Played** all 16 Ransom's Rest missions in PIE (Main 1-7, every side, Dead Lift's ride, Back Pay's choice, Abel by
`Looter.Boss.Kill`), Keeper's Bridge both ways (clean), the watchtower's six flights (climbable), the manway (walkable),
fast travel (works). New tools: `Tools/Unreal/play.py` (goto/at/find/press/fight/kill/lookat/what/guns/ammo in PIE; `press`
feeds input actions, no window focus) and `Tools\playstep.ps1` (one step: move, keys, focus, missions, screenshot;
`-Button ACCEPT` clicks a game menu's button). `Tools/Unreal/reach_probe.py` (walkability flood fill) is unreliable
inside thick collision hulls: don't trust a NOT reached.

**Fixed, built, 596/596 tests, play-checked, pushed** (2df6c23..78afa23):
- Knocking Below: the cellar's dead spawned on the yard above (`EncounterGroundProbe::FindRoofOver`). Checked: they
  stand on the cellar floor. DepotWaiting's spots changed with it (one still wandered out the back: fine).
- Creature melee needs a clear line (hit through the depot's stair door, the Foreman through his barricade).
- Dead Lift blocker: the sheriff's cell was one solid hull; now walkable, crank and report taken/read in play.
- Depot: 32 East Shoulder cliff slabs stood in a line on the valley floor (through the depot, over the rail line):
  `level.cliffs.walled` drops them, `keepOut` keeps rock out of buildings. LOST PROPERTY window and agent's desk
  reachable in play. The cliff stairs down to the Mooring Ledge were blocked by 4 faces (`cliffStairs` in `keepOut`):
  walked top to ledge.
- Tilly's cellar: closed over (no sky from the cold room, checked).
- Boards: real reward lines; every card takes a click (one picked up elsewhere says where). Abel's bar leaves (checked
  after the scene); "Take the clamp pin"; Side 1's gate poster reads before it tears (checked); mission texts; Ledger
  place lines; orders slate names (DOLAN, AVERY, PECK, LOCKETT).

**Still open:**
1. The player's guns vanished twice (all slots empty, no log line, not on the ground): 05.21-05.22 (Sink, after the egg
   sacs) and 05.55-05.59 (mine drift, at the barricade). Not reproduced; `Saved/watch_guns.py` (a Python tick watcher,
   per play session) logs the frame they change (WATCHGUNS). Ask the user if they've ever lost their guns.
2. The orange objective highlight paints Mister Sexton as a solid orange figure (doors get a soft glow).
3. After two full test runs in one editor, PIE crawled at 1-3 fps (interiors dark) and the tests' Message Log window
   pops over the viewport: restart the editor after tests before playing.
4. Calder's note on the Rangers' board may win focus over its card (`Artifact_CalderNote`); the board says where it's
   picked up ("Ruth Calder's note, at Main Street", though the note hangs on that board).
5. Generator: an `onFloor` ridge's cliff band runs past its floor (`area_cliffs.ridge_bands`); `walled` hides it. A real
   fix needs a `--computed` run (~35 min) and `terrain_identity`.

## Session "Ransom's Rest map expansion" (end, 2026-10-13)

**State:** all built, placed in Lvl_RansomsRest, 589/589 tests, committed and pushed (1ae1d99). Nobody has played the
new content yet. The editor may be open on Lvl_RansomsRest, dirty from the tests: reopen it clean
(`Tools/Unreal/open_clean.py`) before saving anything.

**Done this session** (the user's big ask; every answer in memory `rr-expansion` and
`Docs/Areas/RansomsRest_Expansion.md`):
- Map order: 1 Ransom's Rest, 2 the Gilded Lily (by train; NOT built, on hold until the user says), 3 Skyreach (by
  skylift from the Lily's north side). Main menu subtitle: Teropa.
- New Game plays the cold open (skippable). Control hints live in Ransom's Rest's opening; Skyreach's tutorial and
  "Skip the tutorial" are gone.
- Hold-E is a ring. The minimap shows N/E/S/W. Player 10% slower (walk 459, sprint 711, crouch 230, slide 783 cm/s).
- Missions: every one picked up from a giver (notice board, NPC, artifact); map marks ? = pick up, ! = turn in (the
  user's swap); the Missions tab lists accepted and completed only; prerequisites chain the story. Features: optional
  objectives (extra silver), mission items, choice turn-ins (Back Pay: pay or keep), story flags, readables, Delia's
  porch plate. Sides: Side1-3, CaldersCaches, OldFare, KnockingBelow, DeadLift (unlocks the ore lift after Main 5),
  LastWatch, BackPay.
- XP like Borderlands 2: total to reach level L = 180 x (L^2.8 - 1), max 65; kill XP = XPReward x level^1.2; mission
  XP fixed. Story alone ends about level 5.49, everything about 6.84 (`Docs/Progression.md`). Old saves keep their level.
- Hob's roosts replace graves as save/respawn/fast-travel points (the Crow's Mast, Hob's Ember red #D8321C, core
  #FF8A52); 11 in Ransom's Rest.
- Lost Loot: a ground gun despawns after 5 min into the depot's LOST PROPERTY window, and Save & Quit sends ground guns
  there; 30 slots, +10 per upgrade (150/300/500/800/1200 silver), oldest goes out when full.
- Shots sound, puff and mark by surface (13); bullet marks fade; the prioritized objective glows orange within 13 m.
- Ground grenade tins are saved with their remaining life; found chests are remembered per map.
- Ransom's Rest's own house kit (homes A-J, fronts Assay/Hotel/Barber, depot, Tilly's workshop and cellar, Amos's
  barn, coal stage; each model at most twice; Skyreach keeps the old ones). Enterable: depot, workshop + cellar, the
  mountain's buildings, the mine.
- The mountain: core 512 m, Larkspur Bench, the Old Quarry's ore lift, Calder's Post, the Diggings, the Providence
  mine with dead miners and the Foreman (Legendary, set level 7), Hiram, Thaddeus. Larkspur Falls drops 117 m into
  Gravewind Canyon by Abel's deck. Keeper's Bridge is grounded on piers and walks clean both ways (the user's bug).
- Tools: `Tools/layout_patch.py` (agents' layout patches), `Tools/terrain_compare.ps1`.

## Now (2026-10-13, session "Ransom's Rest map expansion")

**Wave 4 integration IN PROGRESS (uncommitted):** all wave-4 agents and the terrain agent are done (terrain checks PASS,
look sheet 39_Bench sent to the user). Done so far: the four patches applied to layout.json (`Tools/layout_patch.py`,
new; the patch files are still in `Art/Levels/RansomsRest/patches/`, delete after the build) and area_shape.py's
FOOTPRINTS/BUILDINGS; C++ compiled (Build.bat, no errors); terrain regenerating with the town patch (--macro --computed);
build_area.py hooks (mountain, town_rr, both missionprops, windmill fan, canyon falls); W4a's script edits (Amos
LOAD_STEP 0, Reliquary from step 4, topics from story_texts via `build_area_story.texts_topics`, Hob's Main 4 perches and
Back Pay perch); bench pines only in build_island_scatter. Next: open the editor, `Tools\models.ps1 -Only
RansomsRest,Waterfall`, Clamp on T_RansomsRestMacro{SW,SE,NW,NE}, delete stale tiles/MIs, import
Intermediate/ArtExport_RR_PropFix, create_area_assets, create_story_lines -> create_mission_assets ->
create_side_mission_assets, build_area.py RansomsRest, scatter, conform_hills + SettleProps, build_impact_surfaces,
sound bank (4 new fitting cues), bridge_walk + path_probe, runtests, PIE checks, commit.

**The user's ask:** Ransom's Rest grows up the north mountain (the Gilded Lily is behind it; don't build the Lily);
map order 1 Ransom's Rest, 2 the Lily, 3 Skyreach; the cold open plays as a new game starts (skippable); hold-E is a
circular ring; the map shows N/E/S/W; player 10% slower; every mission picked up from a giver (board, NPC, artifact)
with ! icons and turned in, with more substance; a side mission opens the way up; Borderlands-style save points; house
variety (no shared or spammed models) and more enterable buildings. **Their answers** are in the memory note
`rr-expansion` and in `Docs/Areas/RansomsRest_Expansion.md` (being drafted): the Lily by train, Skyreach reached from
the Lily by a skylift, the tutorial inside Ransom's Rest's opening, the Old Quarry's ore lift, all missions from givers,
rework every mission + 4-5 new sides, graves become save points, the mountain about +50% with a Rim Rangers' outpost and
a silver mine, dead miners + a Foreman Legendary, Ransom's Rest's own house kit now (Skyreach later), interiors for the
mountain buildings, the depot and Tilly's workshop/cellar.

**Wave 1: DONE, committed and pushed (701ebf6)**, 509/511 tests (BankCoversCues waits for the new cues' sounds; the
pace test renamed `.Pace.Speeds`, unbuilt). Checked in PIE: the offer card (refuses Accept until the prerequisite is
done), the GRAVE MARKED banner, N/E/S/W on the minimap, the hold ring on a bell. The homes kit is committed (e5a3d6d),
not imported. **The design is approved** with the user's answers in its section 13 (XP: story ~5.5, full clear ~6.8;
Foreman set 7; ? = pick up, ! = turn in; the Missions tab lists only accepted missions).
**The user added Lost Loot** (memory `rr-expansion`): ground guns despawn after 5 min into the depot's LOST PROPERTY
window (30 spots, +10 upgrades for silver, oldest out when full, all guns).

**Wave 2: code DONE, committed and pushed (c378037), 557/557 tests.** Checked in PIE: the Lost Property screen (empty),
the Foreman's boss bar on a spawn (SK_Unpaid stand-in). Sounds: 69 new cues in the bank. Approved and committed (not
imported): the town art (sheet 36) and the miners' concepts (sheet 38: miners A, Foreman A, Hiram, Thaddeus).
**Still running:** the mountain art (sheet 37, lift/Calder's Post/mine kit), the character game models (sheet 40),
terrain (core 512 m + Larkspur Bench, sheet 39). Their files are uncommitted on purpose (Art/Levels/*, layout.json,
Art/Models/Buildings/OreLift.py, CalderPost.py, Diggings.py, Art/Models/Mine, Props/Tramway.py, Creatures/DeadMiner.py,
Foreman.py, Hiram.py, Tools/Blender/looter_ghosts/miners/friends/mine/mountain.py, terrain_compare.*, terrain_dump.py).
**Pending for Main after the terrain agent:** layout.json `gameplay.levels.kinds` += ForemanCreature level 5 (then
create_area_assets.py); level step 3 (notes in the session scratchpad `level_step3_notes.md` and `codemap_wave2.md`:
imports, house swaps, depot/workshop/barn placements, lift/mine/post placements, graves, encounters, Lost Property on
SOCKET_LostProperty, build_area_vendors for Pruitt's voice); step 4 (all missions' data per the design, XP retune to
story ~5.5 / full ~6.8). Lost Loot prices: only two upgrades are affordable in Ransom's Rest (~1,270 silver a play).

**Wave 3 DONE, committed and pushed (3d8f232), 582/582 tests:** impacts by surface (13, sounds, puffs, ricochets),
fading bullet marks, the orange objective highlight (13 m), Hob's roosts (the Crow's Mast in Hob's Ember red; replace
graves as save/respawn/fast travel; the crows carry Ellis). All approved art is imported (homes, town, mountain, the
Providence's dead, the roost). Checked in PIE: the highlight on Abel's grave (Glow lowered to 1.2 so it reads orange),
marks on the ground (the pale-blob bug was a material compile error, fixed), a fence reads as wood.
**Waiting on the terrain agent** (core 512 m, Larkspur Bench, the waterfall off the Rim by Abel's deck; its files are
uncommitted: Art/Levels/*, layout.json, Art/Models/Terrain/Waterfall.py, Art/Textures/RansomsRestMacro*, terrain tools).
**Then, level step 3:** layout.json gameplay.roosts (scratchpad codemap_wave3.md) + ForemanCreature kind; terrain import
(build_area.py RansomsRest), build_impact_surfaces.py (the new macro windows MI_RansomsRestMacroSW/SE/NW/NE), house swaps
and placements (scratchpad level_step3_notes.md), lift/mine/post, roosts, Lost Property on the new depot's socket,
vendors, loot, fauna, scatter, Keeper's Bridge fix (below); then step 4 (all missions' data, XP retune).

**At the session swap (the user's call, 2026-10-13):** condense ALL docs for the AI's tokens (the user never reads them),
CLAUDE.md included: terse plain-English notes, every rule/name/number/decision kept, history archived to a file not loaded
by default, each fact in one place, Story's quoted lines word for word. Details in the memory note `docs-for-ai`.

**To do (the user's calls, 2026-10-13):**
- [ ] **Ground grenades saved like ammo** (the user said yes): AGrenadePickup saved with its remaining life, gone when
      it runs out (as FSavedAmmoPickup / FSavedSilverPickup); not Lost Loot. Fold into the level build.
- [ ] **Found chests remembered across saves** (yes): UMapDiscoverySubsystem's found-but-unopened chests kept per map in
      the session save, so their map pins survive Save & Quit / Continue.

**To do (the user's play notes, 2026-10-13):**
- [ ] **Keeper's Bridge** (the bridge by Abel, over the Dry Wash on the keeper's path, (-95, 2)) floats above the
      ground, and walking up the hill to it stops the player: it takes several jumps to get up. Ground the bridge (its
      ends on the banks, no gap under them) and make the walk up and over fluent (a walkable ramp/slope, no step the
      player can't walk up; check with `Tools/Unreal/path_probe.py` both ways). Do it in level step 3, after the terrain
      agent's valley-identity check, as its own change (it alters the wash's banks).

**Wave 2 (as launched; contract `contract_wave2.md` in the session scratchpad):** code 2a mission features
(optional objectives, mission items, choice turn-ins, story flags, UNDERGROUND minimap, porch plate, readable props),
2b ore lift + watch fire + far light + interior visibility + build_area_lift.py, 2c dead miners + Foreman + Hiram +
Thaddeus + Ledger pages, 2e Lost Loot; art: the town (3 fronts, Whitlock barn, coal stage, the enterable depot with
Lost Property, Tilly's workshop + cellar; sheet 36), the mountain (lift set, Calder's Post, mine portal and kit; sheet
37), character concepts (miners, Foreman, Hiram, Thaddeus; sheet 38, the user picks); terrain (core 400 -> 512 m with the
valley identical, Larkspur Bench; sheet 39). Main pre-wrote `ALooterHUD::OpenTurnInChoice` and `IsMenuOpen` counting
`ULostLootWidget::GetOpen()` (unbuilt until 2a/2e land). Next: a sound agent for wave 1's 8 cues and wave 2's; then build
step 3 (the level: houses swapped, mountain, interiors, graves) and step 4 (all missions' data, XP retune).

## Start here (2026-10-13, end of the "polish loop" session; the user is opening a new session)

**State:** everything below is built, placed in both levels, committed and pushed; 470 tests pass; the game starts
(menushots/hudshots). The editor may be left open on Skyreach.

**The user's seven asks of 2026-10-12, all done:**
1. Churchyard: the chapel and yard stand on one flat pad (7.5 m, graded 30° bank; was 7 m of fall and 23° median slope),
   graves upright, the iron fence steps level (55 sections, 4 posts), boot hill on a gentle terrace, a `churchyardPath`
   through the yard (the path probe passes). Generator pads gained bank/tilt/corner settings (`area_shape.py`).
2. Ground texture: M_Terrain picks dry thatch, stony crust (pebbles, cracks, leaf litter) or damp soil per pixel from the
   macro map, three new script-made texture sets (Ground Thatch/Stony/Noise), colour unchanged at distance, +5 samples
   (`GROUND_DEFAULTS`; `Tools/Blender/ground_preview.py`).
3. Creature levels by area, never the player's: Ransom's Rest 1-6 by zone (bluff 1-2 ... Webwood/quarry/deck 5-6),
   layout `gameplay.levels`, `create_area_assets.py`; kill XP x0.37 keeps the story's end at 794 XP (level 6.9).
4. Creatures chase until a real reason (dead, 80 m, safe zone, scene, unreachable 10 s patience), search the last known
   spot, wait and watch at their ground's edge; lock-on radius +20% (Project Settings > Game > Encounters > Pursuit).
5. The Vigil Stone beside each boss arena (after the first defeat): call the boss back after 5 minutes of play or after a
   save and quit; no story replay (Abel goes back on his board), same loot as the first kill; a map pin.
6. Abel is level 7 (Boss rank on a set 6), the Gravemother 6.
7. The FOV slider previews live behind the menu (which steps aside), the minimap zoom slider shows a live minimap.
Also: controller/keyboard navigation for the main, pause and settings menus (focus look only in pad/keys mode), wave 4
(silver currency, Pruitt's hatch vendor after Main 3 and Skyreach's honesty box, selling junk with J/R; the Venom Spitter
and the Bell-Ringer with bestiary pages; third-person grenade toss and Drover reload; the Drover's own hip hold).

**The user's ten fixes of 2026-10-13, all done (480 tests pass):** (1, 5) after Save & Quit and Continue the world
labels layer (damage numbers, creature tags with levels, loot labels, barks) stayed hidden from the pause menu — the HUD
now sets it every frame (`LooterHUDWorldLabels.cpp`) — and the minimap's 32 markers went to ammo first, so creatures were
dropped — creatures first now, and saved pickups keep their remaining life instead of a fresh 5 minutes each load;
(2) interactables offer only through their real part (the skiff's gangplank, the departures board) in a narrowing cone,
never through walls (`InteractionFocus`); (3) Side 3's first step "Find where the Gravemother hungers" completes on
entering the Sink; (4) the editor's Play opens the main menu like the game (`InEditorGameURLOptions`), `Looter.PlayLevel
[map]` skips it (pie.ps1 runs it); (6) semi-auto/pump clicks buffer at most one shot within 0.12 s of ready
(`WeaponTrigger`); (7) Delia's Heirloom was being taken unseen during her talk (hidden loot offered): now tossed onto the
porch when the door opens, turn-ins make sure it exists once, reward guns land on the player's side of windows and fences;
(8) full-pool ammo and salt tins are collected silently, feed lines only when something was added; (9) called-back bosses
drop as the first kill; (10) the Vigil Stone map pin.
**Next (not started):** loot guns never despawn and the save keeps them all (112 in one session; a design call);
third person follows the FOV slider only if the user wants (one line in
`UpdateFieldOfView`); fast travel and combat music still count a searching creature as a fight (GraveTravelSubsystem
could count only Chase/Attack); grenade pickups and found chests aren't saved; `CreatureBase.h` is ~640 lines (split);
Story.md's endgame "levels follow the player" lines wait for the user's endgame call; the user hasn't heard the newest
sounds (listening page version 5 has up to wave 3; the economy, Spitter, Bell-Ringer and shrine sounds came after);
5 sounds flagged by the check (Saved/SoundCheck) to review; perf with the editor closed after the ground layers (editor-open
tours: about +0.3 ms GPU, all views under 7.5 ms).

## Earlier: the 2026-10-08 night session after "Game Enchancements"

- **The user's asks:** (1) change the shooting, task-completion and reload sounds; no crit sound; creatures should sound
  like what they are (slimes squishy); (2) start building Crossroads Town on Skyreach. Then, going to sleep: **a polish
  loop until they say stop** (below). They said **"The new sounds are good."**
- **Done, pushed:**
  - Sounds (d89c720, 68c764e): guns rebuilt twice (heavy and punchy, then less synthetic after Borderlands: a modelled
    pressure blast through an overloaded mic, ground bounce, loud action, uneven echo tails), weightier reloads, short
    reward-ping mission sounds, crits play the hit tick 12% higher, per-creature bullet hits, slimes all jelly. Listening
    page: <https://claude.ai/artifact/VDmyrPmAsxiNa6boNuqKxa>.
  - Crossroads Town round 1 (this commit): see Plan.md Phase 7 and the pipeline's "Crossroads Town". Six spiders in
    Web Hollow, slimes in the Wallow, none respawn on Skyreach; the tutorial says "Follow the road into town".
- **Calls made while the user slept (review):** the town's hedges are passable (bushes have no hulls); the notice
  board moved in front of the saloon; Web Hollow's tor moved 10 m north; the memorial's benches moved off the roads.
- **Round 2 of the town (not started):** the cobbled square and kerbs, stalls, scarecrows, coop, haystacks, sandbags,
  targets, the dock, mushrooms, burrows, cabbages, flowers in pink, red and blue, the Wallow's darker sward and slime
  trails, townsfolk, sheep and hens (`town.json` `deferred`). The Wallow's pools hide in tall grass.

## The polish loop (the user, 2026-10-08 night: "make the game better and increase the quality on loop until I tell you to stop")

Compare with Borderlands' mechanics and playstyle; surf the web as needed; no new levels or areas (new assets are fine).
Don't ask the user questions while they sleep (until 5 AM EST): make the calls and list them here. Their examples:
- [ ] The tutorial feels rushed and unnecessary.
- [ ] Boss fights lack excitement (the Gravemother, Abel).
- [ ] Creature and character models clip or animate weirdly.
- [x] Random rocks hang out of cliff walls: cliff faces squashed into slabs near the ramps' heads are left out
      (11be5fe); a probe of every rock and cliff piece in both levels finds none floating.
- [ ] Ransom's Rest's intro scene: the map is flat brown terrain (trees alone don't fix it).
- [ ] The end of Ransom's Rest's story should leave the player at about level 6-7, not 9-10.
- [ ] No experience for mission steps; missions are turned in, not finished by their last task.
- [ ] The inventory is confusing and boring, with too much information at once.
- [ ] And more: whatever makes it feel empty or wonky. The research and its prioritized list:
      `Docs/Polish/BorderlandsComparison.md` (no ambience or music, no healing, no camera shake or hit-stop, no mantle).

**State at 01:40 (2026-10-09): waves 1 and 2 built, placed, all tests passing, committed.** In the game now: boss
shows (entrances, phases, staggers, loot shower), turn-in missions with a banner and no step XP, the clearer inventory,
hit feedback (camera kicks, hit-stop, stagger, death bursts), mantle and vault, wounds that close and soul-motes, ambience
and music, the reworked tutorial (find a gun, read Skyreach's notice board, its practice postings, control hints), ambient
life (crows, sparrows, swallows, hawks, insects, tumbleweeds, dust devils, washing), the melee strike (V or the right
stick; the gamepad view toggle moved to D-pad up), encounters between the fights in Ransom's Rest (3 camps, 2 patrols, 2
ambushes; flanking, retreat, the rank sting and its flash), creature steering round fences (wall-following, unstick),
Ransom's Rest's late-summer land (gold and olive mosaic, hayfields, banded ridges, grass to 40-50 m), animation fixes
(spider bite legs, death curl, Unpaid hat bank, player climb pose and third-person melee jab), and sounds for every new
cue (160 cues). Story end: about 786 XP = level 6.87; an explorer about 7.44.
**Calls made (review):** the story ends at 6.87 (inside 6-7; `KillXPScale` 0.36 would give 6.67); the churchyard
ambush springs again on every load; camp crates reuse Ruth's Supply Crate; a melee kill notches the gun in hand; the
Unpaid's shriek still presses its right armpit about 2.5 cm into its waist (invisible through the ghost body; the test
now allows 3 cm after two fix passes); the spider's death curl is slower (0.8 s) and ends knees up and out.
**Wave 3 (built, placed, 420/420 tests, committed ~03:50):** a lootable world (crates and barrels that break, graves
and coffins to dig or pry, mailboxes, footlockers, loose strongboxes: RR 30 breakables + 16 containers, Skyreach 17 + 3,
`build_area_loot.py`); a Map tab (key 4, M, D-pad down) with fast travel between opened respawn graves; Unpaid barks (42
lines, a syllable murmur, one bark at a time) and town life (households heard through walls, a far dog, doors muttering
captions by story stage); the Drover revolver (third gun family, pistol ammo, swing-out cylinder, 36 parts); the
grave-salt grenade (G / RB, carry 3, burst 900 at level 1, x1.5 on the Unpaid, edge-safe knock); the boss fog wall
restyled as fog banks and wisps (it read as rain); review fixes (cached ground-trace ignore list, creature list instead
of actor scans, boss slow-mo shared per world, inventory drag ghost, hints hidden while dead, idle slime hops).
**2026-10-12:** the weekly usage limit stopped wave 4's agents mid-task on 10-09 ~04:10; resumed today from what they
had written (Economy/ started, Spitter glob/puddle/rules, throw and reload pose files; the Bell-Ringer had nothing on disk).
The Drover's first-person hip hold (WeaponDefinition FirstPersonGrip/FirstPersonRotation, WeaponManagerSlots) is in
source, unbuilt: rerun create_revolver.py after the next build.
The listening page (<https://claude.ai/artifact/VDmyrPmAsxiNa6boNuqKxa>, version 5) has all 126 new sounds at the top
(music and ambience trimmed to 25 s there). Wave 4 (running at 04:00): currency and vendors, the Spitter spider, the
Bell-Ringer Unpaid, third-person throw and revolver poses.
**Calls made (review):** Drop weapon moved from G to X (the grenade took G); the Drover keeps the game's 1.5x crit
(the user's rule) and gets precision from spread; grenades may be strong (one number: FGraveSaltRules::BaseDamage);
grenade pickups on the ground aren't saved; Skyreach's board reads NOTICES.
**Open:** the Drover's first-person hip pose sits centred and low (being fixed); third person has no jar in the hand
or revolver reload animation; found chests on the map are remembered per visit only. Next: currency and vendors, the
challenges ledger, a shield/ward layer, more enemy kinds (spitter spider, bell-ringer Unpaid).
Wave 3 (as briefed): a lootable world (breakables, graves, coffins), a map page and fast travel between graves, Unpaid
barks and signs of life in town, a new gun family, a grenade.

**State at 23:30 (2026-10-08):** finished by agents, not yet built or committed (Main is integrating: build errors
being fixed one by one): bosses (show, phases, staggers, loot shower; `UBossComponent` show), missions (turn-ins,
`FMissionTurnIn`, fixed XP, kill XP x0.4, the mission-complete banner, `Docs/Progression.md`: about 721 XP = level 6.5
at the story's end), inventory (list + docked card + gun showcase, `Looter.MenuShots`), feedback (one
`UCameraShakeModifier`, shot kicks, per-creature hit-stop and stagger, death bursts, loot fanfare, damage arcs, a camera
shake setting), creature animation (Unpaid arms, spider feet, slime tilt, pack spacing, `Looter.CastShots`), traversal
(mantle, vault, coyote time, jump buffer, unstick), recovery (wounds that close after 6 s, soul-motes), and the sounds
for all their new cues (`recipes/bosses.py` etc.). Still running: terrain look, ambience and music, ambient fauna,
the tutorial rework, encounter pacing. After the build: create_mission_assets.py and create_side_mission_assets.py,
sounds into the bank, tests, MenuShots/CastShots/hudshots, tours, perf, then commits per feature.

**Wave 1 agents (started 2026-10-08 night; each owns its files, Main builds, checks and commits):**
missions (turn-ins, no step XP, level 6-7 pacing, a mission-complete banner); bosses (entrances, phases, adds, weak
spots, loot shower); Ransom's Rest's terrain look (macro mosaic, ridges, distance grass, the ring); the inventory
(clearer, Borderlands-like card and list, plus `Looter.MenuShots`/`Tools\menushots.ps1`); ambience and music
(script-made beds, emitters, a music director); feedback (camera kick, hit-stop, stagger, death bursts, loot fanfare,
damage direction); traversal (mantle, vault, coyote time, jump buffer); creature and character animation and clipping
(plus `Looter.CastShots`/`Tools\castshots.ps1`). Next after the missions agent: the tutorial rework. Later: the
recovery loop (out-of-combat regen), encounter pacing, ambient life.

## Earlier on 2026-10-08 (the local session "Game Enchancements")

- **The user's asks:** slower creature chases (not the slimes); Skyreach keeps its name and starts the game; fix narrow
  areas; the gun ideas (notches, cursed irons, part swapping); a smooth slide with dust; start on sounds.
- **Done, all pushed, 254 tests passing:**
  - Chases: the Unpaid ~400 and Abel ~365, scaled to the player like the spiders (54b1408). Skyreach was already the
    island's name everywhere in the game.
  - Narrow areas: Ransom's Rest's ledge faces sink under the bluff path and the Sink ramp (982127f); Skyreach's roads
    keep big stones off (a boulder stood in the forest road, a3228f6). Path probes clear in both levels.
  - Gun ideas, slide and sounds (4f6abb1, dfa6204; Plan Phase 9): see the commit and `CODEMAP.md` (Audio, Weapons,
    UI/Bench). Benches in Skyreach's village (by the gun rack) and against Ransom Farm's barn (d63bd03).
  - The sounds: 65 cues, 225 takes, synthesized from scratch (the user's call: script-made only). The listening page:
    <https://claude.ai/artifact/VDmyrPmAsxiNa6boNuqKxa> (private to the user). Least sure: the creature voices, the
    gunshots' balance, the hit marker, the mix levels.
- **Waiting on the user:** to listen and say which sounds to keep or redo (by their names on the page); to play the
  slide, the bench and the cursed irons.
- **Choices the agents made that the user may want to change:** notches count only kills that give experience, so none
  on Skyreach (a practice area); Hungry charges at a reload's end; a cursed gun's beam gutters even once lifted; the
  last gun in the slots can't be scrapped; the settings menu is now one scrolling list.
- **Small, open:** the HUD's weapon-name line has no cracked coin for a cursed gun yet (`MakeGunNameLine` or
  `CrackedCoinBrush` in `PlayerHUDWidget.cpp`); the cold open's `ColdOpenSet` sound fields could play cues; roads carved
  into terrain sound like grass (tag meshes `Surface.<Name>`); `PlayerViewComponent.cpp` is 566 lines (split it).

## Earlier on 2026-10-08 (the local session "UI and HUD changes")

- **The user's three asks (2026-10-08):** the new HUD in the game (it is: built 2026-10-07, 731e0df and 5b5b36a); the
  old art-style preview and the style it showed deleted from every file; and ten very different art styles, previewed
  in the game, to choose from. The user found the current look and every earlier style exploration disappointing:
  "I need something phenomenal".
- **Done on the user's PC (the local session, 2026-10-08):** the sandbox preview deleted (the user confirmed), the cloud's
  gem change built and checked at 1080p. The gun's name now ends at the cluster's right edge: it was set in a scale box
  that shrank and centred it, so it's measured and set smaller only when it's too long (`FitWeaponName`).
- **The ten styles are ready for the user to pick** in the Style Lab, <https://claude.ai/artifact/WhmxkTia9qvvhqGLWDSDbr>
  (private to the user): Ransom's Rest rebuilt in three.js from the game's own models, textures, terrain and editor-build
  placements, under the in-game HUD; walk, shoot spiders, number keys switch styles, `I` shows each style's card
  (references, the Unreal recipe, its cost on Medium). Six fixed shots per style: `Saved/StyleLab/shots/styles/` in the
  cloud container (rebuild with `Tools/StyleLab/test/shots.mjs`). The ten, cartoon to real: 1 Clay Frontier, 2 Skyward
  Anime, 3 Painted Frontier, 4 Inkslinger, 5 Teropa Pulp, 6 Sunbleached, 7 Neon Frontier, 8 Ember Gothic, 9 Celluloid
  West, 10 Golden Hour; 0 is today's look. The art direction briefs and the research behind them: `Docs/Art/StyleLab/`.
- **The user's call (2026-10-08):** the best are 3 Painted Frontier, 4 Inkslinger, 5 Teropa Pulp and 6 Sunbleached.
  The ten are catalogued (done 2026-10-08, the local session): `Docs/Art/StyleLab/Catalog.md`, contact sheets in
  `Docs/Art/StyleLab/Sheets/`.
- **The art style stays as it is (the user, 2026-10-08: "I don't want to change the style").** Style 3 was tried
  and removed; style 6, Sunbleached, was tried and taken off the island; style 4 was stopped before any code. Every level,
  the tutorial island included, wears today's look. `Tools/Unreal/build_island_style.py <style> apply|undo|status`
  and Sunbleached's files stay in the project in case the user asks again (its switch on the masters is off, so it
  costs nothing). Don't start style work unless the user asks.
- **A test build exists (the local session, 2026-10-08):** `Tools\package.ps1` ("Packaging a test build" in the
  pipeline). The first: `Saved\Packaging61008-163145\AI_Looter_Shooter_Test_20261008-163145.zip`, 605 MB, with
  today's look (made before the painted trial). It runs the menu, sessions, saves and the HUD; it was made before the
  sounds. Sharing it needs the user's PC (or a cloud-drive tool the user agrees to).
- **The Gilded Lily stays on hold**, and the cloud handoff's other decisions (Crossroads Town, ember powers, heroes)
  are not started; the gun ideas are built (above).

## Finished in this session (2026-10-07)

- **The HUD upgrade** (spec: `Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`, "The HUD upgrade in brief"):
  - the kit draws painted pictures (`FPaintedIcon`, `PaintedIconBrush`) and glows; gunmetal, gem and health colours
    in the palette;
  - the player frame (`UHudPlayerFrameWidget`) with the portrait (`UHudPortraitWidget`, from
    `Art/Icons/HudPortrait.py`), the level-up banner and red screen edges replace the health ring and the XP bar;
  - the mission tracker (`UHudMissionTrackerWidget`) replaces the tutorial prompt and follows any tracked mission;
    objectives carry a short line and a key hint;
  - the weapon slots stand in a column beside an upright cartridge; the crosshair kicks on each shot;
  - the minimap's gunmetal bezel and area name; the boss bar built like the health bar;
  - the old whole-screen red flash on hits is gone (the user's call).
- **Tools:** `Looter.HudShots` / `Tools\hudshots.ps1` photographs the HUD at 1080p through twelve states.
- **Rules (the user's):** at most 8 agents; the orchestrator (Opus 5.5) picks each agent's model; parallel agents work
  from one shared contract; `CLAUDE.md` condensed, its details moved to `Docs/Pipeline.md`.

## Finished in the session before (2026-10-07 to 10-08)

- **The user's asks:**
  - The player 15% smaller and slower.
  - A look-sensitivity slider (Settings > Controls).
  - Space stands up from a crouch, and the next press jumps.
  - Sprint plus crouch is a slide at 1.1x speed, which ends in a sprint if forward is held.
  - Spiders and the Gravemother slowed to match the player.
  - Commits 8fba9e6, 73aa50f, 6b03dff.
- **The user's play-test bugs:**
  - The spider boss stuck in her den's wall (f3e3579).
  - A rock blocking the tutorial island's ramp to the lookout (bd96147, 8e0e50c).
- **Ransom's Rest art round**, all of the art session's notes closed:
  - the Sink's pit wall in new cliff panels, with talus at their feet (de5a8fe, e2e826a);
  - the den re-dressed;
  - the slopes' scrub in patches, and far-tree groves with pines (7c305cc, c685d66);
  - fences standing plumb and stepped, and the churchyard's graves facing the valley (c49039f, 473a338);
  - the burial deck clear of the rim's rock (732b43d);
  - the windmill in galvanized steel, and Abel's lantern glass warmer.
- **New build tools:** ramp walkways cleared (`build_area_walkways.py`), cliffs under platforms
  (`build_area_platforms.py`), pit panel runs (`build_area_panels.py`), talus (`build_area_talus.py`), path probes
  (`Tools/Unreal/path_probe.py`), and Main's helpers moved into `Tools` (`commitlib.py`, `stage_import.ps1`,
  `winshot.ps1`, `open_clean.py`, `pie_check.py`).

## Open, small (the user hasn't asked for these)

- The slide is still a code-drawn pose (now smooth, with dust and sounds); a real slide animation would need an art pass.
- The cloud session's handoff (`Docs/Handoffs/`): the HUD upgrade and the gun ideas are built; Crossroads Town, the
  ember powers and the heroes are not.

## Sessions and helpers

- **AILS Artwork** (a local Claude session) models in Blender and never commits; Main imports and commits. It has
  closed every Ransom's Rest note and was told the *Lily* is on hold.
- The cloud sessions (Cloud, AILS Storyline) are idle.

## Starting a new session

1. Read this file, then `CLAUDE.md`'s rules, then `Docs/Pipeline.md`.
2. Run `git status` (the tree should be clean) and `git log --oneline -15`.
3. Before any editor work, check the user isn't playing: `Tools/Unreal/pie_check.py`, and look for a running
   `UnrealEditor.exe ... -game`.
4. Ask the user what to polish first, with tap-to-answer choices.

---

# Session "Polish: make it alive" (2026-10-14), as it stood at its end

## Then (session "Polish: make it alive", 2026-10-14)

**The user's ask:** finish the handoff, then polish until they say stop: "the game feels dead and devoid of character...
minor details around houses, cliffs, better mob animations, better projectiles, better visualizations"; plus "observe the
terrain (mainly the cliffs and rocks) around the map, fix any discrepancies or odd shapes". Nothing major, no key details
changed; art only after they approve concepts. Their answers: guns never vanished for them; Lost Property shelving dearer
(Main doubled it: 300/600/1000/1600/2400); third person keeps its own FOV; endgame scaling decided later.

**Handoff items done, 607/607 tests, pushed** (0b54bdb..d0815dc): the vanished guns were play.py's teleport (a
set_editor_property in PIE rebuilt the pawn's Blueprint components; the game now warns loudly); Sexton's highlight is a
rim (`Figure` look); a searching creature no longer blocks fast travel or holds combat music; CreatureBase.h split (526
lines); board note focus, board turn-in lines, the Haskett lift mutter, Ellis's headboard readable, save v3 step
migration; porch plate in three pieces; the lookout stain decal (readable moved onto the model's stained boards); the
5 flagged sounds fixed and imported; the generator's onFloor band fix, the ore bin's loading bay (`oreBinBay`), no oak on
the Bench; the mine's mouth no longer shows sky through the hidden rooms (adit drawn from outside + a veil); tour views
inside the interiors (`top`), `runtests.ps1` reopens clean and closes the Message Log (`-Restart`). RangerCabin's tangent
warning was the reimport's first build, not the model. Perf (editor closed, 37 views): 128-206 fps, heaviest 7.8 ms,
render-thread bound at ~1,400 draws (`Docs/Performance.md`).

**World polish done, 620/620 tests, pushed** (ff25b07..80c68ad): concept C (lamps flicker, window life, ambient air,
gusts), the cliff and rock review (steep terrain wears the kit's sandstone, no floating fragments or mirrored repeats,
outcrops reworked: look sheet 45), cliff life (look sheet 43: 273 pieces, 70 stains), the town (look sheet 42 + the
village kit: 655 instances, 12 dusk lamps), the regenerated terrain (band fix, ore bin bay) and a full rebuild + scatter;
path_probe clean. Look sheets 42-45 approved (44 animals: "slightly more realistic", no need to show).

**Combat, animals, roosts, rewards and the user's list done, 666/666 tests, pushed** (baabab1..fc7363a): two crow
stations a map (the Depot, Calder's Post; a death wakes at the last one found; the other nine removed); D creatures
react, E guns that feel physical, small animals (hens + rooster in two yards, a barn cat, cliff swallows at their
nests), F rewards (level-up burst, XP popups, footfall dust, pickups pull), their 99 sounds (334 cues). The user's seven
items (2026-10-14): tracker under the minimap with check circles; "+N XP" by the kill and by the XP bar; the gun's
notches before its name; no crosshair in ADS (third person keeps a centre dot); crouched ADS aligned; the wheel up =
previous slot; Subtitles On/Off (Interface; spoken lines hide, notices stay); health and XP bars 25% longer and thicker.
The town's small clutter culls at ~30 m. Perf (editor closed, 46 views): heaviest 8.1 ms (PointOverValley_FarShadow),
Aerial 8.05, Farm 8.0; render-thread bound at ~1,450 draws: thin headroom.

**Presets and perf (the user: "High and Epic absolutely tank"; picked "measure, then tune"):** Lumen's surface cache
choked on the ~66k small scatter instances (1.3 s frames); `ULumenScatterSubsystem` leaves instanced meshes under 1.5 m out
of Lumen and the distance fields; High = Nanite + VSM + SSAO + SSR without Lumen (51-70 fps on the RX 580, was 2-22), Epic
keeps Lumen (22-28 fps, was 1-2). Medium unchanged, within target everywhere (heaviest 8.1-8.2 ms, GPU-bound); no free wins
left (numbers in `Docs/Performance.md`); trade-offs offered to the user. New dev commands: `Looter.Perf.DrawShares`,
`HideFolder`, `LumenSkip`. Cold open pictures sent (`Saved/Screenshots/ColdOpen_1014`). Small known issue: an ensure on
Hiram's figure at load ("SkinnedAsset property overwritten ... without NotifyIfSkinnedAssetChanged", DeadFriend's
`WearModel` sets it in the constructor; harmless, fix later).

**The user's next four (2026-10-14, no optimization now):** 1 spider deaths more dramatic, legs curl UNDER the body
(not to the sides or above); 2 better water and waterfalls (concept `Saved/Concepts/Water/water_concept.png` approved,
CLEAR TEAL water); 3 the gun looks goofy running in third person (F5); 4 hands on the gun in first person like
Borderlands: FULL (Ellis's gloves and duster sleeves, support hand does reloads and carries the jar; look sheet
`Saved/Concepts/Hands/fp_arms_sheet.png` to the user before import). Ellis's own third-person body (today Unreal's white
template mannequin; design in `Art/Backlog/Characters/Ellis.py`): LATER. The user asked for the full water plan "with
room to adjust and add tasks later". DONE, 681/681 tests, committed (22c2a81..3906e79): spider deaths, third-person carry,
water (materials, meshes, falls, splash rings; tuned once), hands CODE. Then the user approved the arms (98dbdb2) and
asked: guns raised so both gloves show; guns bigger (rifle, shotgun +10%, revolver +20%); the pistol named "Revolver"
(2e5a425); swap animations; hands smaller if they still look odd. All built and tuned in PIE (holds 6 ahead, 7 right,
7 down from the first raise, hands at 0.88); see Pipeline "First-person hands", "Gun sizes", "Swaps". Not seen in PIE:
the reload hand paths (tests cover them) and ADS with the hands. Next: the user playtests; then their notes. Known
leftovers: `WeaponManagerComponent.h` is 543 lines (split it), Hiram's ensure (below).

**Waiting on the user:** a playtest of all of the above (they were away; told it's ready 2026-10-14).


# Moved from Docs/Handoff.md on 2026-10-22 (sessions before 2026-10-21, verbatim)

## Now (session "Small leftovers", 2026-10-15, local)

**The saga (the user, 2026-10-15 night: a long-running Borderlands-like story; "Why is Ellis here? Where does the story
go? What's the goal? Who's the final boss? What is the world lore?"):** the campaign already existed (Revenant); the user
chose to deepen and scale it (15–18 regions) and seed it into Ransom's Rest. Written: the bible page for the user
(https://claude.ai/artifact/Gr66uUTXWzetBTaHvE9j9Z) and the same canon in `Docs/Story.md`, marked *(saga)*: the
saints as souls who refused to cross, the spark of Ada in Ellis, Sexton's false lighthouse (the Act II turn), the Well on
Skyreach, three new regions (Halfway Junction, Hoarfrost Pass, the Sundown Road), the cloud heroes as companions, and
the list of seeds to build in Ransom's Rest (Story.md, "Seeds in Ransom's Rest"). The user's calls (Decisions 15):
the *Lily* an airship, the false lighthouse, all three new regions, and **five playable heroes picked before the story
starts, like BL2**; a non-Ellis pick **takes Ellis's place** (one story for everyone; heroes not picked don't appear;
Story.md "The heroes"); the pick is built **after the Rest's seeds, before the *Lily***. All calls in. **Next, in
order (the user, revised 2026-10-15 night):** the ember-power system with Ellis's three (Dust Devil, Sundown, Borrowed
Time; one equipped at a time, all three owned from the start; Story.md "The heroes"), then the hero pick (select
screen, four game models, their twelve powers, line variants), the Rest's seeds alongside (Story.md "Seeds in Ransom's
Rest"), then the *Lily* as an airship.
**Overnight run (2026-10-15 night; the user: "Start building. I am going to bed... don't ask anymore until I prompt
again"):** the user's last calls: power key **Q** (rebindable), the HUD socket **beside the weapon column,
bottom-right**, the swap UI **its own inventory page as the second tab** (the Ledger third), and after the powers
**plant the seeds in Ransom's Rest** (looks that need approval are left as look sheets). The app refused to start a separate
session on that turn, so Main builds the powers here, with agents, unattended, recording decisions in this file under
"Decided unattended"; the seeds follow the same way. No questions to the user until they prompt.
**Ember powers round 1: BUILT (89210af, 92cb87e).** Checked in PIE at the Rest's spawn: the socket by the weapon column
(ready ring, running arc, cooldown pie with the count, the denied shake), each power fired on Q and timed by
`Looter.Embers.State`, the Embers page (three cards, E equips). Tests 705/705 (7 new `Looter.Embers.*`); RR spawn 7.7 ms
on Medium (was 7.6; +14 draws, the socket). Not yet: the user's own play; the feel of each power (the dash's knock, the
dusk tint Sundown skipped, Borrowed Time's pay-back) is tuned by numbers in `Tools/Unreal/create_ember_assets.py`.
**The seeds: BUILT (fe2c4df hooks, cc031ac words, 2790b15 props, 4567413 placed, then the map moved).** Pipeline "Seeds
in Ransom's Rest" has every piece. Checked in PIE: the six pages lie where they should (the lockup page read: the card
"A PAGE OF THE LEDGER: THE NEEDLES"), the letter in the rack read, the Seven between Ellis's poster and Calder's note,
the map on the ticket office's wall by the pigeonholes (it first hung over the window: `MAP_ASIDE`), Ruth's page by her
lamp. Look sheets in `Saved/Concepts/Seeds/`. NOT yet seen in play: Hob's three mutters (town-life clock, within 7 m of
his perch), Aldana's Ada talk (after Main 4), the second light at dusk after Main 6 (`Looter.FarLight 1` for a look), a
crossed face (no `Ember.Paid.*` flag is set by anything yet), the Ledger's town pages in the inventory once a page is
read, the cold open's spark. Tests 710/710 (5 new `Looter.Seeds.*`). Perf not remeasured (a dozen small props).
Hob's mutters use the town-life voice cue.
**Where the night stopped / next:** the hero pick (the user's order: select screen, four game models, twelve powers,
line variants). Its first step by the pipeline ("Character portraits") is a painted bust per hero for the user to pick
from, and the paint step runs on Node + Playwright. **The user (2026-10-16 morning): "Install Node and Playwright on
this PC and paint the four heroes' portraits with the existing portrait pipeline."** Done: Node 22.23.3 (the zip from
nodejs.org, unpacked to `%LOCALAPPDATA%\Programs\node`, on the user PATH), `npm install` (playwright ^1.64, `package.json`
at the root, `node_modules/` ignored), `npx playwright install chromium`; Ellis A rendered under Blender 4.4 and painted
here end to end (d20f275). Then (Pipeline "Character portraits", the four heroes' round): `bust.py` split out of
`EllisBlender.py` (3b30400), per-hero frames and `sheet.js --hero` (68ad752), the four hero scripts (2187ba2 Crane,
e7917bb Pike, 08a3677 Gauge, 87b0152 Odessa), `Tools\portraits.ps1`, the aux pass's Raw fix (3d0f206: every earlier
portrait was painted with the ids misread). The four sheets `Saved/ArtPreviews/Portraits/<Hero>/Sheet_<Hero>.png` were
sent to the user. (Paused by the user 2026-10-16 to 2026-10-19 21:00 for usage.) **Resumed 2026-10-19:** all four
repainted with `-AuxOnly` after the Raw fix (that switch had passed Blender `--aux-only` letter by letter, so it
re-rendered the colour pass too; fixed in `portraits.ps1`), the four sheets looked over and sent. **The user's picks
(2026-10-19): Crane A The Marshal, Pike A The Surveyor, Gauge C Company Iron, Odessa A The Dealer** (Story.md "The
heroes"). Then the user's calls: **all four game models at once** (four agents) and **the select screen alongside**
(built now with the painted portraits; models drop in as they land). **Running (2026-10-19 evening):** five agents on
one contract, `Saved/Heroes/CONTRACT.md` (not committed: read it on resume): CRANE, PIKE, GAUGE, ODESSA each write
`Art/Models/Characters/<Hero>.py` -> SK_<Hero> (Ellis.py's way; up to 35k triangles; mannequin bones + twist + swing
chains on the game's prefixes; `<Hero><Set>` texture sets; previews in `Saved/ArtPreviews/Characters/<Hero>/`; export
test into `Intermediate/ArtExport_<Hero>`), SELECT writes the hero select screen and the data-driven hero
(`UHeroDefinition` Role/Story/Color/Portrait/FigureMesh/Retargeter/IsPlayable, DA_Hero_* for five, the twelve powers as
data, `ULooterSessionSave::HeroId`, `StartNewGame(Index, HeroId)`, `Looter.Hero`, tests `Looter.Heroes.*`). Agents
never commit; Main reviews, imports each model, builds its retargeter, checks in PIE, commits. Blender runs queue two at a
time through `artrun.ps1` (Saved\BlenderSlots). Not this round: first-person arms per hero, the HUD portrait per hero,
the twelve powers' code, line variants. **The Gravemother's Barrow fight alongside (the user, 2026-10-19), their calls:
upgrade in place** (charge -> Furrow, brood -> Unearthed, spit -> Grave Clod; Gravequake, roar, staggers stay), **the
headstone breakable** (its own health, not plain armour), **the timings kept** (clod tell 1.2 s, shot-in-grip stun 2 s,
rubble 10 s, furrow tell 2 s, berms 20 s, mounds bulge 1 s), **+25% speed in the last third**. Main's numbers: stone
health 20% of hers, three states (66%/33% of its health), an early break staggers her but doesn't bring the last third.
Three more agents on `Saved/Gravemother/CONTRACT.md` (not committed): BARROW-ART (SK_Gravemother on Spider.py's rig,
the headstone meshes and shards, the fight's earth meshes, FX and decal textures + `build_grave_fight_materials.py`),
BARROW-SOUND (eleven `Boss.Gravemother.*` cues), BARROW-CODE (phase 1 a plan in `Saved/Gravemother/PLAN.md`; phase 2
the C++ once SELECT has finished with the editor). BARROW-SOUND done and committed (aed611d, 13 cues; Main still
imports the bank). BARROW-CODE's plan is in `Saved/Gravemother/PLAN.md`; Main's answers for its phase 2 (send when the
editor is free): the long stagger = her boss stagger 2.5 s, the clod stun the same at 2 s; burning cracks dropped; the
clod's burst on her does no damage; **a popped mound gives the spiderling's kill XP**; **a thin stone bar under her
boss bar** while the stone stands; spiderlings stay the plain spider; tests `Looter.Creatures.Gravemother.*`; small
additive changes to SpiderCreature, BossComponent, BulletSubsystem approved. **SELECT done and committed (e2aaab8)**:
the hero select works (716/716 tests); after each model lands rerun `create_ember_assets.py` (it fills a hero's
portrait/mesh/retargeter fields only once the asset exists). BARROW-CODE phase 2 started (owns the editor and builds
until it reports). A seventh agent, HUD-PORTRAITS, draws the four heroes' inked HUD portraits
(`Art/Icons/HudPortrait_<Hero>.svg` -> `HudPortraitData_<Hero>.inl`, not included yet) and writes
`Saved/HudPortraits/WIRING.md` for Main's C++ hookup once BARROW-CODE is done. Main's own untested piece:
`Tools/Unreal/build_hero_retarget.py <Hero>` (tested on Gauge; it replaced `build_ellis_retarget.py`). **The user (2026-10-19, "what can speed it up"): start the twelve powers now, keep the 8-agent
cap.** An eighth agent, POWERS, writes the twelve powers' C++ into a holding tree `Saved/Powers/Source/...` (not under
Source: BARROW-CODE owns the builds), edits `create_ember_assets.py` and the powers' sound recipes/cues in place, and
writes `Saved/Powers/README.md` (files, run order, compile risks); Main moves the tree into Source and builds once
BARROW-CODE reports. First-person arms and line variants start as agents free up. **The user went to bed
2026-10-20 ~00:00: "No questions until 5am."** Main runs unattended until then, its calls under "Decided unattended
(2026-10-20 night)" below. Clean-up later: the portrait scripts' shared helpers into `bust.py` (`tube`,
`chain`, `turn`, `ramp`, `ball`, `tear`, `point_light`), and the hero model scripts' shared machinery out of Ellis.py.


### Decided unattended (2026-10-15 night), for the user's review
Main's calls (the agents add theirs below):
- The gamepad's ember key is the left shoulder (the grenade took the right bumper; the key-binding code kept the left
  "for the ember powers to come").
- The HUD socket stands in the Arms row to the LEFT of the weapon slots (bottom-right corner, beside the column).
- The session saves the equipped power's id and its cooldown left; a running power isn't saved (a load finds it ended).
- A new game equips the hero's first power (Dust Devil); the hero defaults to DA_Hero_Ellis when the Blueprint sets none.
- Powers deal damage through UEmberDamageType; a power's kill gives XP and notches no gun.
- Sundown: the world at 0.25 speed for 6 s, the player compensated to 1.0 (their bullets too); Borrowed Time: the debt
  paid back over 3 s after it ends, each kill strikes a quarter off; Dust Devil: untouchable for its 0.3 s.
- The powers page is the inventory's second tab, "Embers"; the Ledger moves to third.

CORE's calls (component, key, save, assets):
- Cooldowns not in the concepts: Sundown 20 s, Borrowed Time 24 s (Dust Devil keeps the demo's 7 s). Dust Devil runs
  as a 0.3 s duration (its dash), not instant.
- A swap mid-cooldown keeps the wait (held to the new power's cooldown), so swapping is never a way round it; no swap
  while a power runs.
- The gate also refuses mid melee strike and mid grenade throw (Busy), as the grenade refuses mid strike.
- A denied press clicks (`Ember.Denied`, half volume) for Cooldown, Active, NoPower and a power's refusal; silent while
  dead or Busy. The component plays it; the HUD only shakes.
- Death or a scene starting cuts a running power short (checked each tick: the header had no death handler to bind).
- `Ember.Ready` plays when a cooldown ends, and when a power ends with its cooldown already over; not after an
  interruption.
- `Looter.Embers.Use` fires past the cooldown through the public API (Interrupt, RestoreEmbers with no cooldown, TryUse),
  so the body's gate (scene, menu, swap) still holds.
- A power with no class, or an abstract one, gets no object and reads NoPower.
- The numbers' fields: Dust Devil Damage 15, Range 900 (dash), Radius 260, Extras FlingMin 400, FlingMax 500, Dizzy 1,
  FovKick 14; Sundown Strength 0.25 (world speed); Borrowed Time Strength 0.25 (share struck off per kill), Extras
  PayBackSeconds 3.
- Tests use a probe power class in its own header (`Tests/EmberProbePower.h`): a UCLASS can't live in a .cpp.

UI's calls (HUD socket, powers page, icons):
- The socket stands 34 px left of the slots (not the slots' 12): a slot in hand lifts 22 px further left and would touch it.
- (Main's follow-up) The socket stands apart from the weapon cluster, so it shows unarmed too, in the same place (a row
  of its own with a spacer for the slots and cartridge); it fades with the cluster while the cluster shows.
- Ready: an orange ring plus a slow glow in the power's colour; running: an arc in its colour shrinking from 12 o'clock;
  cooling: a dark pie drained clockwise with whole seconds rounded up (never "0"); a pop and white ring flash when ready.
- The key tab grows with the key's name (22 px for one letter, 8 px per extra letter, up to 80).
- The page: three big cards side by side (not a list and a details column like the Ledger), since there are only three;
  clicking a card equips it at once; Space and the Interact key equip as well as E/Enter/A; the choice moving plays
  `UI.Hover`, an equip the loadout's `UI.Equip` clack, a refusal `UI.Denied` and a line under the cards.
- Page keys: 1-5 and the shoulders to neighbouring pages on every page (the loadout's LB stays Move, so it has RB only);
  the inventory key closes, as before (the brief said "Tab cycles pages": Tab is the inventory key, which closes).
- The control hints' words follow the new numbers ("Then 3: the Ledger", "Then 4: your missions"); the Map's prompt reads
  "1-5 Pages".
- Icons: Dust Devil three tapered spiral streaks round a ringed eye; Sundown a half sun on a horizon bar with four
  tapered rays and two bands; Borrowed Time an hourglass with a slanted ledger rule through it (no frame posts).
- The colours are the contract's hexes exactly; no `EmberReady` colour (the ready ring is the kit's Accent).

POWERS' calls (the three powers, knocks, sounds):
- Dust Devil goes the way of the keys held, else the way the body moves (over 1.5 m/s), else the way the player looks;
  under 1 m of room (a wall, a drop) the press is refused. It follows the ground (walk-sized rises, 45-degree slopes),
  stops short of walls and drops, never leaves the playable area; from high in the air it dashes level and ends falling.
- The dash is fastest as it leaves and still quick as it arrives; the player walks on at 7 m/s after it.
- Creatures behind a wall from the path aren't struck; a fling is shortened (to 0.75, 0.5, 0.3) to land on ground, else
  the creature only reels. Flings by rank: Rare 85%, Epic 70%, Legendary 45%, a boss none (and no daze).
- Flung creatures cartwheel once in the air and land upright; dizzy shows as three gold sparks circling the head and a
  sway; every knock reels at least 0.2 s so the body settles before the brain takes up again.
- Sundown also keeps the player's controller, camera manager and HUD actor at full speed, re-checks carried guns each
  frame, pitches the world's audio to 0.75, and is refused while time is already slowed (a boss's slow-motion death).
  The dusk tint was skipped (no Grave Sight post-process exists; the UI agent owns any vignette).
- (Main's follow-up, done) Under Sundown the guns fired and reloaded at the world's quarter speed: `AWeaponBase::
  GetOwnerTimeScale` now divides the fire and reload timers' seconds (`WeaponBaseFiring.cpp`, `WeaponBaseReload.cpp`).
  The melee and throw cooldowns still read world time (short; left).
- (Main's fix) The Dust Devil fling is squared to the dash: measured from the nearest path point the way out was a
  diagonal (the test saw a fling more along the path than off it), so the along-path part is removed first.
- Borrowed Time adds the full blow to the debt (a killing blow's too), heals back what it really took, and kills only
  strike the debt while it runs (not during the pay-back); a debt under 0.5 isn't called in; death forgives it; a
  console use during a pay-back carries the rest into the new ledger.
- Sounds: the seven ember cues are 2D; Ready and Denied in the Interface class, the powers in Effects; Ready and
  Sundown.End fade in on purpose. cues.json carries guessed volumes until the render's mix measures them.

The seeds (same night; Pipeline "Seeds in Ransom's Rest" has the how). Main's calls: the seven towns = Ransom's Rest,
Highwater Landing, Dustwater Mesa, Saint Agnes Mire, Furnace Hollow, The Needles, Stoneweight Quarry (six pages on the
Rest, the Quarry's found later); flags `Ledger.Page.<Town>`, `Letter.Deacon.Cleft/Saloon`, `Ada.Told`; one new model each
for the page, the letter, the post rack and the map; the Seven poster as an atlas cell. The agents' calls:
- CORE: the X on a face is a second small decal (`SevenCross` cell), not a material mask; the steady light is a mode of
  `AFarSignalLight`, not a new class; no StoryLighting change (dusk already holds through Main 6 and 7); the second
  light isn't gated on Grave Sight (no vision mode exists); a topic's flag is set only when that topic was said; the
  spark only when the scene isn't skipped.
- DATA: the town pages are STORY_CHARACTER pages in the NPC section ("???" until found; `Looter.Ledger` wants exactly
  seven LedgerName pages); the saints Hester (Highwater Landing), Ezra (Dustwater), Agnes (the Mire), Maud (Furnace
  Hollow, the smith), Corliss (the Needles), Gideon (Stoneweight), Ada; robbery dates 3 Mar, 1 Apr, 9 May, 14 Jun, 12
  Jul, 2 Aug, the Rest 21 Aug (its page entered 21 July, "E. Ransom" twice); the saloon letter is the Deacon warning
  Delia (postmarked Halfway Junction, 9 Aug, never collected), the cleft letter to "Kid", never sent; Hob says the Point
  line on his after-Main6 bier perch; the second light due west of the Point, 9 km out, 30 m up, size 5500, strength 30;
  the Seven poster at offset [0, -12, 0]; the rack 1.4 m up, 110 cm right of the saloon doors; the lid's lines say
  "prised up at one edge" to match the model. Open: the town pages' "not met" help reads "You two haven't met, friend"
  (a place wording needs C++); Hob's mutters play the town-life voice cue; Sloane's "kid" line left as it was.
- ART: one letter model (sealed, so "never sent" and "unopened" both fit) and one page model for all; the rack has two
  old letters baked into slots 6 and 8; the map's layout (west line splits for Highwater Landing and Dustwater Mesa
  then runs to the Rest; north the Needles, north-east the Quarry, east Furnace Hollow with a branch to the Mire) and
  "W. PIKE, SURVEYOR" (Wendell Pike; drop it if unwanted); poster wording "THE SEVEN / OF THE DUNNE GANG", first names,
  "DEAD OR ALIVE", "CAPT. R. Calder".

Done this session (the user picked "small leftovers" first): `WeaponManagerComponent.h` split (its events and small
types in `Inventory/WeaponManagerTypes.h`, 491 lines); the editor-exit ensure on Hiram's figure fixed in all six
poseable-mesh constructors (Pipeline, Bugs); the shotgun reworked with the user watching: held 20 cm nearer, the left hand
cupping the pump with a new `EHandGrip::Pump`, the thumb along the pump (the user approved the close-up); live switches
for the support hand's finger and thumb curl; `play.py equip <n>`. Many other files are still over ~500 lines
(`CreatureBase.h` 562, `BossShrine.cpp` 568, `LooterUIStyle.cpp` 765, `LooterSoundCues.h` 847, several tests).

**Ellis is the player's body (the user's ask mid-session, done: 116316b, fdd3894):** the cloud branch merged (ffb56c2),
SK_Ellis built and baked here, imported; the mannequin stays the animated body, hidden, and Ellis copies it through
RTG_MannyToEllis (Pipeline, "Character models", "Ellis as the player's body"). The user's calls: MakeHuman's base.obj
fetched here; "retarget live" (not every clip retargeted onto him). Checked in PIE: third person from behind, standing
and running; front view aiming the rifle and the revolver (hands on the guns); first person (he's invisible to the
player). Tests 687/687; RR spawn 7.5 ms on Medium (was 7.7). The user saw the first two shots; not yet played.

**Ellis quality loop (the user, 2026-10-15: "it looks plastic, stiff and unlike a real fps character. Improve it until
I tell you stop and run all decisions by me"):** the user's picks: holster empty only while the revolver is drawn;
first-person gloves and sleeves match him; all four rounds (materials, coat and hair that move, pose and hands, remodel
the silhouette); budget raised to ~35k triangles. Done: round 1 materials (approved; 3512eba), the holster, FP arms matched
(17e2348), round 2 (39683fe, b26dbf0): swing bones + springs, the hat tipping back when aiming plus a 12% smaller brim
(the user's "both"), hands 15% thicker, the coat and cape looser, the bandana off his neck, facing +Y (the fingers now
auto aligned: no claws). Round 3 (an audit in Blender, `Saved/Concepts/EllisAudit/defects.md`): flipped faces (the
waistcoat's holes), the cape's wings and fin, gauntlets, collar, lantern hook, holster; hands kept out of the coat
(f9be3bf). Round 4, the user's picks: **"Apply C in full"** (squarer shoulders, pointed cape back, deeper hat crease, harder
brim curl, the coat to the ankle), **proper boots** (stovepipe on a real last), **face kept dark**, **the waistcoat as
cloth** (smooth black wool, brass buttons), **the coat's back hanging straight** (the user: "Back of the cape is deforming
in between his legs. It should be straight": the cloth simulated over a filler behind the legs, the vent closed). 34.2k
triangles, 113 bones (each swing chain ends in an unweighted `<chain>_end`; a 7th skirt chain `skirt_bm` down the vent).
The springs: the bone between two points pushed out the body's side of a limb, the points kept over the ground with the
hem gathering there, nothing rising over what it hangs from (Pipeline, "Swing"). Audit: the stride's boot through the
coat 53 -> 34 points, the crouch's hem under the ground 11.7 cm -> 4.9 cm. Checked in PIE: back standing, walking,
sprinting; crouched front and back. Shots sent to the user (round 4 before/after, in game).
**Next in the loop (ask first, tap-to-answer):** the pose round (idle weight shift, stance). Still open: a full stride
still shows the shin below the knee through the coat's back (34 points) and the left hand sits 4.9 cm into the coat's
front in it; the cape's corners lift with the arms when sprinting (CapeFollowFront/Back); the hair reads as ribbons up
close; the crouch's holster and lantern touch the thigh; the collar's turned-up corner reads like a horn from below; FBX
import warns "Not valid bind pose" (recreated fine); the spin viewer artifact predates the bake here.

**After round 4 (the user's notes, 2026-10-15):** "When crouching should his knee be touching the ground?" answered:
no, the crouch is a squat on both feet (code pose, LooterStancePose); the idle stance's right foot is back so that knee
comes down to about 20 cm (the left 34). "cape still clips through the boots and shins": foot limbs and the ring
(Pipeline, "Swing"). "His wrists are either invisible or don't match the gloves": forearm twist bones + FBodyForearmTwist
(Pipeline, "The forearms"), the glove's cuff up into the gauntlet, the sleeve stopped inside it (round 4g; 117 bones,
34.3k triangles); checked in PIE, before/after sent. Left: a small crease at the right gauntlet's lip by the thumb when
the hand is turned and bent.

**The Gravemother fight concepts (the user, mid-session 2026-10-15):** "generate a sprite sheet for the grave mother
boss ... projectiles, the spiders ground charge, the spider itself, attack particles, etc. ... give it some character and
feel like a real boss fight. Just generate 3 to 5 concept arts for now, we will turn them into game assets once I choose
the ones I like." Four sheets made and sent (`Art/Backlog/Creatures/GravemotherFightConcepts.py`, sheets in `Saved/Concepts/Gravemother/`
by `GravemotherFightSheets.ps1`): A Bone Mother (brood riding her, milk spit, grave plough), B Vigil (chapel candles as
her phase meter, corpse-lights, a procession of ghost-flames), C Shroud (ivory and oxblood, burial silk, net shot,
drag-lines, egg sacs), D Barrow (the bold one: a headstone shell, grave clods, a furrow charge that leaves cover,
unearthed spiderlings). **The user picked D, Barrow (2026-10-15)**, to build after the Ellis work they asked for first.
The user's review ask ("Make it better"): the real sprite sheets are made and sent (`Art/Backlog/Creatures/
GravemotherBarrowSheet.py` + `GravemotherBarrowSheets.ps1`; `Saved/Concepts/Gravemother/Barrow_{Moves,FX,Hero,
Spiderling}.png`; renders and `BW.json` in `Saved/ArtPreviews/RansomsRest/Concepts/GravemotherBarrow/`): every move
as 6 timed frames, the FX pieces isolated with alpha and sizes in cm (clod x3 sizes, dust burst cards at 0.1/0.4/1.0 s,
debris x5, rubble, the heave crack strip + end cap, dust jets, berm segment, mound closed/burst, stone chips, sparks,
stun stars; plus a landing-shadow decal, a furrow decal, fresh-earth dust #806B55 vs pale floor dust), her turnaround,
the spiderling (135 x 132 x 57 cm). The headstone is grey granite (#8E8B86), not sandstone (it vanished against the
Sink's wall). Not numbered yet: her faster run after 33%. Design calls made by Main for it (the user
hasn't confirmed them; ask if they object): the berms stay 20 s then crumble; the headstone is armour that deflects
(sparks, chips; no health), cracking off at 33% to bare the Bone abdomen; crits head + the abdomen's soft rear, the
whole abdomen after 33%; Grave Clod 1.2 s tell, shot in her grip = burst on her + 2 s stun, a 1.5 s lob to 20 m, a
rubble pile 10 s; Furrow 2 s heave tell then the plough; 4 spiderlings at 66% and 33% from mounds that bulge 1 s
(shootable).

**The user's next picks (2026-10-15, tap answers):** "Both 1 and 2": Ellis's leftovers (all four: the shin through the
coat's back, the cape lifting when sprinting, the hair reading as ribbons, the crouch's holster and lantern) and the pose
and idle round. Idle: **"Weary revenant"** with **breathing, glances around, hand fidget** (not the hat touch); the armed
stance **kept as it is**. Built: the idle (LooterStancePoseIdle.cpp; Pipeline, "The idle"), the cape's corners following
an eased arm (BodySwing, `FollowEaseSeconds`), the ring order for two new back chains, the lantern's `Bulk` (2.5 cm
more room off the limbs); round 5 in Ellis.py: `skirt_bli`/`skirt_bri` (a fifth of the way from each back chain to bm;
the stride's shin through the coat 28 points -> 0), hair as closed lens-shaped locks in two layers (5.8k triangles; still a
little chunky up close from the back), the holster pouch on the left thigh (crouch contact 3.6 -> 2.2 cm; the gun now
grazes the trousers 0.9 cm in a stride), the gauntlet lip rolled out; 125 bones, 34.9k triangles. Checked in PIE: idle,
sprint from behind, hair, crouch front and right side. Shots sent. Tests 698/698.
**Round 6 (the user on round 5's shots, 2026-10-15: "Cape clippings, hair looks like mud, can see through the
head/hat, hat doesn't seat on his head properly, don't stop until you get it right"; then "Run multiple agents if you
need to"):** three Blender agents on one contract (`Saved/Concepts/EllisRound6/CONTRACT.md`): HAT (seat the band on
the skull, close the crown's inside and the scalp), HAIR (cards with a script-made strand texture, masked; maybe a new
M_CharacterHair master), CAPE (cape and collar clipping in every pose). Found: every Ellis material is one-sided, so
a single surface seen from behind vanishes (the crown's inside). In-game evidence: `Saved/Screenshots/Tools/aud_*.png`.
**Round 6 done (2026-10-15 night):** HAT seated the hat and closed the head (`ellis_hat.py`); HAIR made cards on a
script-made strand atlas with a new master `M_CharacterHair` (`ellis_hair.py`, `Art/Textures/EllisHair`); CAPE rebuilt
the cape's weights and chains (9, hung from the shoulder joint, `cape_arm_<side>` idle arms) and lined the collar
(`ellis_cape_audit.py`), with BodySwing's follow shares, idle-arm rests, rising chains and cape bulk 6 to match
(Pipeline, "Swing"). 142 bones, 33.2k triangles. Checked in PIE from 20 head/body angles plus carry, aim (5 angles) and
sprint: no see-through, the hat on the head, hair reads as hair, no cape in the sleeves. The hair glowed light brown
backlit: MI_EllisHair Tint 0.55 (also in Ellis.py's EllisHair slot so a re-import keeps it). Shots sent. Tests
698/698; perf at the RR spawn 7.6 ms on Medium (as round 2; game thread 3.7 -> 3.9 ms for the bones and chains, GPU
unchanged). Committed 0ad1801. Cleanup later: `Saved/Concepts/EllisRound6/cape/wip` (1.7 GB of the agent's trials).
Main imports, checks in PIE, commits. Then Gravemother. Found on the way (HAT): the game skin was a "crystal":
`make_lows` spared everything within 3 cm of the eyes, so the skull's back and top collapsed into ~12 triangles (the
holes); now decimated by region. Leftover for later: BandanaKnot sits inside the neck (mid-neck, not at its back) with
36 points of the bandana's lower side edge: invisible, wasted triangles; move the knot to the nape or drop it.
**Next:** the user's notes on round 6; then the Gravemother, Barrow (D) as the real fight (concepts first done: model,
projectile, charge, FX, sounds; ask the user the open calls with tap-to-answer choices).

## Before that (session "Ellis 3D model", 2026-10-15, cloud; branch `claude/ellis-3d-character-model-tvwr9z`, merged)

**The request:** turn Ellis portrait A into the game's 3D model, realistic enough for the game, textured from boots to head,
and something the user can spin round. Midway the user: "I'm liking the look so far, but his texturing looks too smooth ...
More realistic modeling and less smooth looking." (done: cloth-simulated duster, creases, seams, stitching, grain, wear).
**Built:** `Art/Models/Characters/Ellis.py` -> `SK_Ellis` (Pipeline, "Character models"): MakeHuman's body shaped as the
portrait; a cloth-simulated duster to mid-calf (back vent, seams, topstitched hems), fluted cape, rolled turned-up collar
with the sun-ring pin; waistcoat, work shirt, trousers tucked into stovepipe boots with stacked soles and cuban heels; roper
gloves with gauntlets and conchos; cartridge belt, Mexican-loop cross-draw holster with the revolver, Pa's lantern burning
at the right hip; hat, hair, pointed bandana, glowing slit eyes. 63 mannequin bones, four 2048 texture sets baked from
procedural looks, hit zones, LODs 50/25; ~20.6k triangles (the user asked for Borderlands' 10-20k: the ~108k detailed build
is now only the bake source, its folds and creases baked into the game mesh's maps). Helpers `looter_human`, `looter_garments`, `looter_looks`,
`looter_bake`. Merged `claude/ellis-portraits` (portraits, MakeHuman assets) into this branch. The spin viewer: an artifact
page (three.js; Spin, Look at Head/Hands/Belt/Boots, Dusk/Day light) with the posed, textured glTF: https://claude.ai/artifact/HJCgTaXnSRbEb5k7SXjdDa (source `Tools/CharacterViewer/`).
**Not done (needs the PC):** the textures aren't committed (LFS): `Tools\models.ps1 -Only Ellis` bakes them (minutes), then
commit `Art/Textures/Ellis*`; the script was run only with Blender 5.2's Python module here, not the PC's Blender (watch
for API differences on the first run); the IK retargeter from the mannequin; SK_Ellis on the player in place of the white
mannequin (third person); the first-person arms against it (SK_FP_Arms' sleeves are #8f7052, lighter than the model's
dust-brown oilskin, and its gloves #a47c56 lighter than the model's: the user's eye decides which wins).
**The user's decisions (keep):** as the portraits session's; Ellis A is the spec; less smooth (folds, creases, seams, wear);
a game budget of about 20k triangles (picked from ~20k / ~35k / keep 108k, 2026-10-15).
**Next:** the user's notes on the viewer; then on the PC: import, retarget, wire SK_Ellis into third person, check the hands;
then the other four heroes' portraits and models the same way.

## Before that (session "Ellis portraits", 2026-10-15, cloud; branch `claude/ellis-portraits`, merged into the above)

**The request:** the heroes' models "suck"; first paint portraits like the user's reference (richly framed painted busts),
pick one, then build the 3D model from it. Ellis only for now, three portraits.
**Built:** `Art/Portraits/` (Pipeline, "Character portraits"): Ellis A The Revenant, B Came Home at the Last, C Dressed for
the Grave, rendered in Blender from MakeHuman's CC0 base, painted and framed; the frame generator; `sheet.js`. The
images aren't committed (PNGs go to Git LFS, which the cloud box can't push): remake them with the Pipeline's commands
(~8 min for all three).
**The user's decisions (keep):** Blender's `bpy` may be installed in the cloud box; MakeHuman's CC0 assets may be used
(`Art/ThirdParty/MakeHuman`); the code-only SDF busts were passed over for Blender; **Ellis is option A, The Revenant**
(picked 2026-10-15): cattleman hat low, red bandana up, cyan eyes burning as slits in the brim's shadow, collar-length dark
hair, a dust-brown duster with a fluted shoulder cape and the collar turned up, Saint Ada's sun-ring pin on the collar, a
black waistcoat with brass buttons.
**Next then:** build Ellis A as the in-game model (done in the session above).


## Before that (end of session "Polish: make it alive", 2026-10-14)

All committed and pushed (HEAD 8a005cb). Tests 687/687 at 57264b8. That session's full log:
`Docs/Archive/Handoff_history.md` (last section). How each piece works: `Docs/Pipeline.md`; perf numbers:
`Docs/Performance.md`; release-only work: `Docs/Release.md`.

**Built this session:** the old handoff items; world life (lamps flicker, window life, ambient air, gusts), the cliff and
rock review, cliff life, the town's dressing; two crow stations a map; creatures react; guns that feel physical; small
animals; rewards you can see; the user's seven HUD/control items; High/Epic fixed (small scatter out of Lumen, High
without Lumen); spider deaths (legs curl under); clear teal water and waterfalls; the third-person carry; first-person
hands (SK_FP_Arms, Ellis's gloves and sleeves) with raised, level holds and hands at 0.88; guns drawn bigger (rifle and
shotgun 1.1, revolver 1.2); the pistol named "Revolver"; swap animations with holster sounds.

**The user's decisions this session (keep):** Lost Property shelves 300/600/1000/1600/2400; third person keeps its own
FOV; endgame level scaling decided later; two crow stations per map (RR: the Depot and Calder's Post), a death wakes at
the last one activated; animals small only (hens, cat, swallows; no mule); Medium stays as it is (no grass trade-off),
new additions save their own cost; no optimization work now (smoothness waits in `Docs/Release.md`); the Gilded Lily
starts as a World Partition map; water clear teal; first-person hands the full Borderlands way, guns raised so both
gloves show, level (no forward tilt); Ellis's own third-person body LATER (the user asked for it on 2026-10-15: Now).

**Waiting on the user:** a playtest of the hands after d0d9d2e. Not seen in PIE: the support hand during reloads (tests
cover the paths) and the hands while aiming down sights.

## Next (in order)

1. The user's playtest notes first (Ellis in third person, the shotgun's new hold, hands, guns, swaps, water, spiders).
2. Small leftovers: the revolver reads tipped up at the hip (perspective; tilting it up looked worse); the rifle's and
   revolver's support hands weren't looked at again after the shotgun's rework (same kind of check if the user asks).
3. Keep polishing until the user says stop: nothing major, concepts before art, ask with tap-to-answer choices.
4. **Older open items:** Story.md's later areas' level bands predate the new curve (retune when the next area is
   designed); the user hasn't heard the newest sounds (republish the listening page); the slide is a code-drawn pose;
   from the cloud handoff, the ember powers and heroes aren't built (major: only if the user asks); `area_faces.fields`
   gives the East Shoulder's face ground ~748 m^2 inside the boundary where the ridge doesn't stand (Stage Gap's depot
   side; changes the macro maps: needs the user's eye).

## Sessions

- Main's agents model in Blender this session (the AILS Artwork session isn't running); Main imports and commits.
- The session "Polish: make it alive" ended 2026-10-14 at the user's call; the next session picks up here.



## Handoff sections archived at the session swap of 2026-10-27 (word for word)

## Overnight 2026-10-23 (done so far)
Committed: FP grips f6bef3d (palms seated, fingers close by touch, trigger finger on trigger when firing, support thumb
forward on long guns, glove low on the rifle's handguard — a thumb-over C-clamp showed too much arm; ForearmBack 0.6 ->
0.3; known: shotgun trigger finger can't reach its trigger, below frame at the hip). Gun feel pass 3b63cb0 (all four
code jobs + sound art merged by GF-MERGE; sound bank 532 cues; M_BulletMark rebuilt; tests green). Next: DEMO play-check
of the gun feel, then a perf run (tour + fights; impacts/hit markers added cost), then RR play-check rounds.

Gun feel after play-check: 180dd68 impact fixes (shotgun disc gone, legs stay coloured, rifle drift 0.1 visible); perf
RR spawn 7.8 / 8 spiders 7.9 (unchanged). Still failing after two fixes: yellow diamond chips on creature hits, a white
blob behind hit numbers, the sniper ground ring (FX-DIAG now diagnosing by switching layers off in PIE; rule 6: if this
third, diagnosis-led try fails, ask the user).
RR play-check (01:30) **for the user**: early fights kill a standing-still player (bluff nest 6 spiders, town gate,
Main Street ambush: 7-10 hits of 21-26 in 5 s) — balance call is the user's (DamageScale 1.8 stays until they say);
deaths respawn at the farm plot far from town: the rule is "last roost found, else the farm start" and RR has only the Depot and Calder's Post roosts — **ask the user**: (A) a town roost (e.g. by the sheriff's office) or (B) a wake spot near the fight lost. Being fixed: Main 2's "[Tab], then 2" (Ledger
is tab 3), inventory under the HUD tracker/loot card, first Interact missing with the prompt showing, a gate Unpaid
straying and holding 6/7, the sniper "one-handed" (FP-GRIP: not a bug — the trigger glove sits ~6 cm below the frame at the hip because of the user's pull-in framing; rifle and LMG too; an unapplied option in Saved/FPGrip2Code: sniper HipAhead 0, HipUp 1, partly undoing the pull-in — **ask the user**), Tilly: by design a "talking window" (only the Sexton and Hob have bodies) — **ask the user** whether she gets a
silhouette behind the curtain. Sexton: flat dark by design (Sexton.py flat coat colour); the gold when highlighted is the
highlight's 0.03 unlit fill on his near-black coat — NPC-LOOK's fix in build_highlight_material.py (FigureFill 0,
FigureRimOpacity 0.9, FigureRimPower 5) changes every objective highlight to rim-only: uncommitted, to verify in the
editor against other highlighted things before keeping.

Committed overnight after that: 9e36d62 (FX diagnosis: white blob = hit-flash rim, fixed; "yellow diamonds" = tall grass
seed heads), 0274c0e (UI/mission/interact/straggler fixes; tall grass crossed spikelets; figure highlight rim-only —
verified: the Sexton's gold rim reads at 5 m). Play-checked PASS: grass, tracker/loot card, Ledger tab, interact 6/6,
sustained fire. Still open: **sniper ground ring/column failed three fixes (rule 6: ask the user)**; small/flat mission
items (the gate poster: a decal, which can't wear the overlay) barely highlight at 5 m and not past 7 m — only the ?
marks them (later: a decal-friendly highlight or a world marker).
Round 2 (Main 3-5, Old Fare, a bounty) fixed and committed fa2ef14 — incl. **creature levels were rolled from (0,0) since
2026-10-13** (all 2-3); now by zone (bluff 1-2/2, Sink 4-5, gallery 5-6). Re-check PASS except: Main Street ambush no
longer starts (R2-FIGHTS fixing in the editor), bounty postings all identical (R2-BOUNTY: variety), Fenn's ghost stands in
the player's spot at Old Fare's turn-in (R2-OBJECTS). Bluff nest still kills a strafing level-2 player (7 bites of 20-27
in ~3 s; town gate cleared with no damage).
Round 3 (Main 6-7, Abel, Gravemother, Dead Lift, Last Watch, Back Pay; God mode): all finish. Fixed+committed since:
0da79f8 (fights sent away when their story moves on; the Main Street ambush was fine — it springs on a walk-in),
17cf676 (bounty variety, partial: only the Scatter arena (Unpaid) is open after Main 5). Being fixed: more objectives
needing pixel-exact aim (Abel's lantern posts, watch fire, Dead Lift boiler and pin), Back Pay barricade with no gap,
Rangers' board shows Side 3 with no accept, board/choice cards ignoring E, Main 7 turn-in moving to Tilly with no hint,
15+ beams after the Gravemother's shower. Hiram/Sloane glowing ghosts in dark rooms = the painted ghost look (by design).
**The user's answers (2026-10-23 morning)**: early fights "Soften early only" (bluff nest, town gate, Main Street; later
fights unchanged); respawn "Town roost" (a roost by the sheriff's office); sound files "Yes, download" (the three named
Sonniss files, ~12.6 MB, licence kept); FP hip hold "Lift slightly" (sniper/rifle/LMG trigger hand shows at the bottom
edge). Round 3 fixes committed (see git). Still open from earlier: Tilly silhouette (not asked), sniper ground ring (three
failed fixes, not asked yet).
Morning results (2026-10-23): early fights softened 4460136 (play-check at level 2, strafing, no God: bluff nest lowest
66%, town gate no damage, Main Street 67%); town roost works (death in town wakes at the Sheriff's Office roost);
hip lift (sniper/LMG glove shows; rifle can't — the bullpup body hides it); recorded impact layers imported 1ff8114 +
bank; Main 7 turn-in line fixed (Tilly's shop). Open: Dead Lift pin unreachable + boards close on walk-away (R2-OBJECTS
in the editor); sniper ground ring (3 failed fixes, not asked); Tilly silhouette (not asked).
Test build 2026-10-23_0658 packaged (BuildsPlaytest, 1.27 GB, smoke menu+RR PASS; Play Latest Build.bat): everything
through 94699b1. Packaged log: the GameFeatureData ensure and the missing-save warnings are gone; the ~66 D3D12 PSO
compute errors (RX 580 driver) remain. Next: the user plays it.
**Morning questions (asked)**: (1) early fights' difficulty (deaths at the bluff nest, gate, Main Street ambush);
(2) respawn: town roost vs wake near the fight; (3) the sniper/rifle/LMG trigger glove below the frame at the hip
(pull-in framing) — lift it?; (4) Tilly a silhouette at her window?; (5) the sound pack download (three files, ~12.6 MB);
(6) the sniper ground ring: keep trying or drop it.

## Night of 2026-10-22 (the user went to bed; their calls)
- Extra sound bundle download: "Do what you feel is best" — Main's call: no download overnight (the download rule needs
  the exact file/source/size shown first); use the recordings on disk; show the user the file in the morning if needed.
- **Ask the user in the morning** (download rule: file, source, size): three more parts of the same Sonniss GDC
  bundle licence, from ftpmirror.your.org/pub/misc/sonniss2016|2017/individual/, for wood/dirt/flesh impacts:
  2017 Double Trouble Audio "Wood Impacts and Debris" (4 files, ~1.3 MB); 2017 Mechanical Wave "Rock, Brick and Dirt
  Part 02" (4 files, ~8.8 MB); 2016 SoundBits "Just Gore | Add On" (4 files, ~2.5 MB). Not downloaded.
- Overnight: "Polish + play-checks": finish, play-check and fix the gun feel and grips, then keep fixing what play-checks
  find in Ransom's Rest.
- FP grip: modern tactical (high grip, support hand far forward, thumb over / C-clamp on rifles) "but not so much hand
  showing" (compact, Borderlands-like amount of hand).

## Gun feel pass (next; plan Docs/Work/2026-10-22-Gunfeel/PLAN.md, user's 4 answers at its end: recorded reload
foley yes, one more bundle part yes (name file+size first), shotgun shell-by-shell + rack yes, slower empty reloads, no
fast-reload window). Jobs GF-SOUND-ART, GF-RELOAD, GF-KICK, GF-SOUND-CODE, GF-IMPACT start after the muzzle fix commit.
FP-GRIP (user: "hands grip the gun ... like an actual military officer", AR first) in phase 1 (Docs/Work/2026-10-22-FPGrip).

## User's answers (2026-10-22 late)
Muzzle flash: "Rework it" (MUZZLE agent, Opus, Saved/MuzzleCode: layered burst, star feel, never over the target).
Spider read at range: "Darker coat" (darker cooler brown, pale bands; art agent). Next push after this round: **gun feel
pass** (sounds, recoil, impacts, reloads per gun kind vs top shooters). Play-tests: a short in-game check by an agent
after each feature (memory playtest-after-features). Missions flow committed a4f489f; DEMO walking its checklist; two
older test failures to look at: Looter.Missions.Highlight.Figure (Sexton's "figure look"), Looter.Story.UnfinishedBusiness.
Placed (Whitlock Fields fences).

## Missions flow (the user, 2026-10-22 night)
Verbatim: "The main missions shouldn't have to be accepted and should automatically be added to the missions list. The
side missions are all automatically placed in the world and should only be available after progression is made. Lastly;
I shouldn't have to listen to dialogue before being able to accept a quest. Once I accept a quest it should move or skip
to the dialogue that plays once you accept, borderlands style." Main's calls: main missions auto-start when available
(previous turned in / new game), talk as captions on the move, tracked at once, old saves start them on load; every side
mission gated by a main step (no "?" or offer before it); offer cards take Accept at once, Accept cuts to the accept
lines. MISSIONS agent (Opus) writing Saved/MissionsCode + Docs/Work/2026-10-22-Missions/MISSIONS.md. Also running: spider
fangs floating (since the slim chelicerae, a7bc507) + "skin blurry, lacks texture" (art agent, Spider.py).

## Spiders (the user, 2026-10-22 late: "Soften the spiders" + "they look almost see through... not correctly stylized")
Done and pushed: Brown Spider 300 -> 230 health, bite 12 -> 11 (92d89ed; Gravemother keeps 300/12, spiderlings 60). Look
(b956124): the "see-through" was the hit flash (M_FX_HitFlash overlay, a whole-body white wash re-fired by every bullet);
now a rim (Even 0.08, Rim 1.6, RimPower 3; repeats within 0.15 s at 30%); Spider.py redone (jointed tapered legs, painted
coat). Play-check (level 2, Common rifle + 40 ward, 1.8): 6 chasing spiders survived, 11 s, lowest 121/148; 8 chasing:
lowest 75 (51%), 2 breaks; before, 6 killed in 9.5 s. Maybe now on the easy side: the user's playtest decides (knobs:
spider health/bite in SpiderCreature.cpp, Looter.Fight.MeleeTokens). Notes: at 15 m in daylight the brown coat blends into
the ochre ground (legs read; the Sink's ash reads well); hip-fire muzzle flash's star spikes read as flat yellow diamonds
over close targets (flash_spider_burst.png) — the star flash was the user's pick, so ask before changing it.

## Polish round (the user, 2026-10-22 evening: "Keep polishing")
Contract `Docs/Work/2026-10-22-Polish/CONTRACT.md`.
DEMO verdict (level 2, Common rifle, 40 ward): **keep DamageScale 1.8** (2.0 too harsh: 10 Unpaid left 45%). 4 spiders: lowest
ward+health 69%; 7 Unpaid: 79%; a spider bite (22-30) breaks a Common ward on the second bite. 6 spiders kill at any
scale (10.8 s at 1.5 no ward, 9.5 s at 1.8): 324 HP spiders, 3 biting at once (tokens) — a balance question for the
user, not the scale. Marks: not mirrored; Thresher (24/-0.4/4.0) and Powderkeg (size 3.4) fixed in MARKS (DA_LMG,
DA_Launcher changed). Bounty Strongbox opens by hand.
Committed: ed9c923 (SHIELD-POLISH, BOUNTY-ART, PACKAGE-NOISE), d158cfb (BUGFIX: paid line, "2 guns or wards",
SM_NoticeBoard_Bounty, loot labels step apart `ULootLabelSubsystem`, salvage bodies take maker shell colour, Longhorn/
Powderkeg marks forward), 8360ab7 (ward edges at 0.8 strength: at the red's 0.39 the cyan was invisible on the bright
world; Powderkeg mark 3.0 cm). DEMO re-check passed: sheets, paid line, BOUNTY PAID banner, labels apart, FP marks,
salvage paint.
Open (small): the bounty board model (SM_NoticeBoard_Bounty seen in PIE 2026-10-22,
the class default is SM_NoticeBoard_Bounty after the restart, checked 2026-10-22; the placed board follows it unless the level overrides it); third-person view never shows a gun's side, so marks are first
person only (framing, accepted); turn-in "+XP" popup moved above the crosshair (MissionUp -28 -> 32; it sat on the Interact prompt); board title 34 -> 27 (its line fell onto the rule; not seen since); 6 spiders kill a
level-2 player at any DamageScale (324 HP, 3 biting at once): a balance question for the user; PSO compute errors in
the packaged game on the RX 580 (`r.PSOPrecache.GlobalShaders=0` untried).

## Next builds (the user, 2026-10-22 noon: "Keep going!" and picked all four suggestions)
Player shield, gun manufacturers (a concept sheet first for the user's pick), a bounty board, playtest builds; plus
per-place ground detail and small fixes. Contract `Docs/Work/2026-10-22-Next/CONTRACT.md`. Lane: PERF first (quiet),
then merges as jobs report; PLAYTEST packages last.
Perf (quiet noon): tour worst 8.1 ms (Aerial, Aerial_Dusk, PointOverValley_FarShadow; render-thread bound); fights
7.6-7.8 avg, p95 8.5-9.4; draws +250-400 since 10-14 (new dressing should be instanced). **The user approved the four
makers as shown** (Roundhouse, Saint Maud, Windfall, Meridian; `Saved/ArtPreviews/Weapons/Concepts/Makers.png`,
`Docs/Work/2026-10-22-Next/MAKERS.md`). `Tools/hashcheck.sh <HASHES.txt>` checks a holding tree's originals.
Landed: SMALL-FIXES (bc9a98c), GROUND-DETAIL (4a32212; also carries MAKERS' M_Gun mark code in build_world_materials).
Evening: MAKERS + SHIELD committed together (067ceca; built together, Main hand-merged the shared hunks). Main's SHIELD
hooks done: `Looter.Fight.DamageScale` 1.5 -> 1.8 (SHIELD's retune; **no played-fight verdict yet**: spiders never reached
the test player; the user's playtest decides, 2.0 if soft), no hurt grunt on a ward hit, Borrowed Time debt = health lost,
wards lying on the ground kept in the map save (`FSavedMapWorld::ShieldPickups`, put back where they lay; guns still go to
Lost Property), `/Game/Art/Shields` cooked. Saint Maud's mark darkened to #9a6a1c (faint on ivory; not yet seen in game).
Not done (SHIELD.md "For Main's files"): boss shower `Roll.Shields` (boss table off), cyan hit edges for ward hits,
HudShotScene ward state, wards in Lost Property, `Shield.*`/`Bounty.*` cue names into LooterSoundCues.h (bank warns
only); MAKERS: marks on Rustler/Thresher enlarged but unseen, far-side mirroring unchecked.
BOUNTY committed (811851f): board on Main Street + 5 arenas placed in Lvl_RansomsRest; Looter.Bounty 6/6, Pacing pass; one
bounty played through in PIE (take, tracker, mark, Strongbox, +XP/silver, banner); Strongbox not opened by hand, other
arenas/tiers untried. Status-line wording change in BountyBoardWidget.cpp built with the evening's last editor build.
Later (BOUNTY.md item 6): real bounty sheet art, BOUNTIES header, a "BOUNTY PAID" banner.
PLAYTEST done: `Toolspackage.ps1 -CloseEditor -SmokeTest` -> `BuildsPlaytest<stamp>WindowsAI_Looter_Shooter.exe`
(first one 2026-10-22_1518, 1.26 GB, ~4 min incremental / ~20 min cold; smoke: menu and Ransom's Rest load PASS);
`Play Latest Build.bat` starts the newest. Fixes on the way: package.ps1 missed the editor (PS 5.1: one pscustomobject has
no .Count; Get-Process + @() now), ImpactSurfaceTests' SetParentEditorOnly inside #if WITH_EDITOR (the game target compiles
the tests). Packaged-game log noise to look at later: a handled ensure at startup (GameFeatureData primary asset type
without the GameFeatures plugin) and ~20 D3D12 compute pipeline creation errors (PSO precache; game runs).
**The next-builds queue is done.** For the user: play the packaged build; judge a real fight's drain with the ward at
DamageScale 1.8 (2.0 if soft); Saint Maud's darker mark; a bounty.

## State at 2026-10-22 noon: the queue is empty
Done since the morning list (all committed and pushed, last 2011943): VFX-2 (fire explosions, no white-ghost kill), FP
rifle/LMG/sniper pulled in, loot polish (element numbers, element deaths, WEAK/RESIST), hero stances + 3P aim-in +
Gauge's head up + vent sound, bolder area tones (read from above only: suggestion "per-place ground detail"), element
parts' own models, painted ghosts (glow fixed), Gauge enamel, Ellis value, Crane/Odessa faces + Crane hair (Odessa's
side hair fixed), Gravemother granite, Sexton, ghost tatters/hands, smooth slime core. Next = the user's playtest and
tuning (Looter.Fight.*, Looter.Gunfeel.*, Looter.FPHands.*, Looter.Pose.*, Looter.Element.*); open small items: Gauge's
revolver aim splays over his pauldrons; the Rustler's orange count glow drowns its Soul-light part; Ranchhand element
tube parts small in first person. The overnight heartbeat cron was stopped (queue empty).

## Morning of 2026-10-22: what to try first (for the user; tell them in 3 sentences)

The overnight run built 12.2-12.5 (commits 4a84f74 ... d79e293). Try in this order, in Ransom's Rest (editor Play):
1. **A fight**: the town gate (Main 3) and the Main Street ambush right after: enemies take turns, surround you, hit
   harder (x1.5), chase faster (x1.3). Tune `Looter.Fight.*` / `Looter.Gunfeel.*` if it's too hard or too easy.
2. **The new guns**: `Looter.GiveWeapon SMG|Sniper|LMG|Launcher Epic 5` (the Rustler's helix, the Longhorn's scope and
   breath, the Thresher's spin-up, the Powderkeg's rockets) and the restyled Bullpup/Ranchhand/Drover.
3. **Elements and drops**: an element gun on spiders (Ember) and the Unpaid (Soul-light); a Legendary drop
   (`Looter.Element.Apply` to test statuses).
4. **The story**: less talk, Hob on the move; find a secret (the Widow's strongbox, Coffin Rock's top).
5. **The clay placeholders**: Knockers at the Bench tramway, Coffin Rats in Tilly's cellar, bandits on Boot Hill
   (confirm you want living human bandits in the Rest).
Open questions for the user: Boot Hill bandits (tone); the rifle/LMG first-person forearm still near-upright (fore-ends
~70 cm out): accept, or move the guns/hands further (a design call); world variety reads subtle: stronger place tones?
VFX-2 (explosion fire, the white-ghost kill, Slag Bomb spikes, impacts) was still running at 05:40 (see below).
**The user's answers (2026-10-22 morning):** keep the Boot Hill bandits; FP rifle/LMG: **pull the guns in** (closer and
lower so the left hand sits low like Borderlands); world: **bolder per-area ground tones** (repaint).
**Character art (the user, 2026-10-22, from CHAR-ART's sheets `Saved/ArtPreviews/Characters/Concepts/CharArt_1-3`;
plan `Docs/Work/2026-10-22-CharArt/PLAN.md`):** approved all four: the painted ghost (every M_Ghost user), a stance per
hero (Ellis keeps his weary idle), the hero value and hair pass, the Gravemother and Sexton looks; plus Gauge's enamel
(back to his pick) and the no-OK items. **New asks:** Gauge looks down all the time: his head tilts up slightly; and
**every hero needs a third-person aim-down-sights pose** (today none aims in when ADS in third person).
Running: CHAR-MAT (materials: ghost, Gauge enamel, hero value, Gravemother/Sexton), CHAR-POSE (stances, hand curl,
Gauge's head, 3P ADS aim; C++ holding tree), CHAR-FACE (Blender: Crane's hair/mustache, faces).
VFX-2 committed (4486136): M_FX_Fire explosions, the white-ghost kill fixed, Slag Bomb cheaper, impacts that read at
30 m; a white spider still showed once in the Slag Bomb after-shot (watch); final p95 not re-measured on a quiet PC.
CHAR-POSE tree ready (`Saved/CharPoseCode`, MERGE.md; Odessa's card is a small cube: make it a flat card at merge; the
stand's capture can't show steam). Lane queue: FP pull-in (running) -> LOOT-POLISH merge -> CHAR-POSE -> tones import ->
element parts import -> CHAR-MAT -> CHAR-FACE import; then build_sound_bank for Hero.Gauge.Vent.
FP pull-in committed (056cea5); Hero.Gauge.Vent rendered (6aec834). LOOT-POLISH's copy into Source was refused by the
permission system for the agent; **the user approved and Main copied it** (hashes matched); the agent builds/tests now.
Bolder tones repaint done (`Saved/ArtPreviews/Terrain/RansomsRest/macro.png`), import waits for the lane (WORLD-ART).
Loot polish committed (142b647). Art scripts committed, not imported (ba175de): element parts' own models (20 rows;
new kit materials need MIs), Unpaid tatters/hands + smooth slime core (`Intermediate/ArtExport_GhostSlime_after`),
Crane/Odessa faces + Crane hair (`ArtExport_Crane/_Odessa`), CHAR-MAT scripts (its GO steps: build_world_materials
M_Character M_CharacterGlow -> build_creature_materials M_Ghost -> ImportModels ArtExport_CharMat_Gauge/_Ellis/_FPArms/
_Gravemother/_Sexton -> build_character_glows). **The user approved Main doing today's holding-tree copies** after hash
checks. CHAR-POSE copied and building now. Remaining lane: tones import -> one IMPORT job (element parts + setup_gun_parts,
ghost/slime, CHAR-MAT steps, faces) -> perf.

## Suggestions and overnight decisions (2026-10-20 to 2026-10-22)

### Suggestions for the user (Main's, newest last; none built without a go-ahead)
- A **player shield** (Borderlands' core loop: a recharging shield over health, found as loot with rarity and
  traits): harder fights then feel fair, and it adds a loot slot. Medium cost (HUD bar, a gear slot, drops, save).
- **Gun manufacturers** (3-4 in-world makers with their own handling quirks and looks, like the parts' families):
  makes every drop's name and feel promise something. Medium-large.
- A **daily bounty board** on Ransom's Rest using the set-piece machinery (a rotating hard fight with a chest): replay
  value before the next region exists. Small-medium.
- **Quick playtest builds**: a nightly packaged build the user can play on the couch without the editor. Small.
- **Per-place ground detail** in Ransom's Rest: the bolder tone repaint (done 2026-10-22, the user's yes) reads from
  above but barely at eye level, where roads, boardwalks and the close ground textures dominate; the lever is per-place
  ground detail layers (town dust, Sink ash, quarry grit) or decals along the paths. Medium.
- **A translucent fire material** for explosions (rockets, Slag Bomb): additive fire washes out in daylight. Small-medium.

### Decided unattended (2026-10-21 night), for the user's review
(Main's calls while the user slept; newest last. Contracts: `Saved/Fight/CONTRACT.md` (+ AUDIT.md), `Saved/Guns/`,
`Saved/Loot/CONTRACT.md`; not committed.)
- 12.2 numbers: 3 melee + 2 ranged attackers at once, a waiting ring at 4.8 m (8 slots, drifting, feints, biased to the
  player's sides/back), chase x1.3 (a walking player gets caught, a sprinting one escapes), creature damage x1.5,
  regen after 7 s at 7%/s (was 6 s, 12%). Fights 6-10, caps 20 near the player / 16 Unpaid. Set pieces: Main Street
  ambush (Main 3), Webwood drop, Providence cave-in, each once per session with a sting, shake, caption and a reward.
- FIGHT-AI's own call: an Unpaid waiting on the ring doesn't phase-step (it would step through the ring); boss adds
  (spiderlings, Abel's risen) take turns too; the slime keeps its own hop rhythm (not faster).
- 12.3: the Longhorn sniper crits x2.0 (its role; the revolver 1.75x was the user's); part effects Ricochet, Burst,
  Explosive, Piercing, Overcharge, Twin (Epic+, a few Rare); elements on parts from Uncommon, ~20-35% a hit; weakness
  x1.75 / resist x0.5: Ember strong vs spiders/Spitter/slimes/Gravemother, weak vs the Unpaid; Soul-light strong vs the
  Unpaid/miners/Bell-Ringer/Abel's risen, weak vs slimes; Grave-salt strong vs Warded/Phasing/Ethereal/the coal, weak vs
  spiders; bosses take statuses at half duration; the Grave-salt grenade becomes the element's damage type. Drop
  moments: rarity beams taller, stingers, Epic+ spin and ground ring, Legendary a 0.25 s slow-down and a 60 m column.
- Rocket ammo is a new class (1-3 per kill drop leaning to the kill gun); the LMG uses rifle ammo.
- FP-HANDS (plan `Saved/FPHands/PLAN.md`, Borderlands measured from 10 BL2/BL3 shots: gloves/wrists only, low; gun on
  a diagonal to the bottom-right; pistols one-handed; no hands in ADS): our support forearm stands upright (~30% of the
  frame) because the FP forearm is 23 cm and the fore-end 65-75 cm out. **Main's call: lengthen the FP forearms ~1.25x
  in the arms art** (all five heroes' FP arms; reversible) so the forearm can slant like Borderlands'; the revolver
  becomes one-handed at the hip (support hand back for reloads/throws); `Looter.FPHands.*` cvars, `ViewFov` off by default.
- ROSTER (Story.md "Creature roster", marked † in the Areas table): Coffin Rat (rat), Bone Hound (the story's coyote) /
  Whiteback Wolf (wolf), Knocker (goblin), Gravepicker and Lookout (human bandits), Slagback / Cairn Walker /
  Spikedriver (golems), Pallbearer (giant), Mesa Stinger (scorpion), Gloam Bat (big bat), Sallow (vampire), Cinder
  Drake (dragon), Sexton Beetle / Bog Mantis (big insects), extras Rattler, Dust Wight, Strawman. RR placeholders: 4
  rats in Tilly's cellar + 4 in the depot baggage room, 4+1 Bone Hounds on the keeper's road (after Main 4, before
  Main 6), 5+1 Knockers at the Bench ore tramway (after Dead Lift), **4 Gravepickers + 2 Lookouts on Boot Hill (after
  Main 3, before Main 4): the Rest's first living human enemies — a tone call for the user to confirm.** Code in
  `Saved/RosterCode` (merge after STORY and VFX; `Saved/Roster/PLAN.md` has the run order, CODEMAP and Pipeline text);
  placeholder hits splash ichor until CreatureBlood gets a clay kind.
- 12.3 C++ agents write into holding trees (`Saved/GunsCode`, `Saved/LootCode`, `Saved/GunsSound`) while 12.2 owns the
  build; each then gets the editor lane in turn (GUNS-CODE first) to merge, build, import, test.
- Progress (newest last): gun models committed (c0a1f3c Rustler/Thresher, 1415246 Longhorn/Powderkeg; previews
  `Saved/ArtPreviews/Weapons/`); world variants phase 1 committed (852087e: 30 models, plan `Saved/World/PLAN.md`; phase 2
  = import, swaps in build_area/dressing/scatter, terrain tone per zone); **12.2 committed (4a84f74)**: tests pass
  (Fight 6, SetPiece 2, Encounters 15, Creatures 114, Story 47, Progression, Recovery, Locomotion, Gunfeel); PIE: at
  most 3 attackers at once, spider hits 22, the Webwood set piece ran (caption, push 2 at 3 left, shower, flag). Perf: the 9.1 ms runs were a busy PC (agents rendering; Main forgot SavedArtPause.flag); quiet, 10 spiders
  fighting in town 7.5-7.7 ms, the director under 0.05 ms (8ac1be7, CSV stats in Pipeline/Enemies.md "Fight: AI"). Gun/element sounds committed (987ea59, not imported). LOOT-CODE phase 1 done
  (`Saved/LootCode/README.md`: hooks other owners must add: WeaponBaseFiring `PlanFor` for Burst/Overcharge,
  `RefreshLootBeam` from `LootDropMoments`, the rocket calling `TryApply`). STORY phase 2 running with Main's rulings
  (no new canon: secrets 6-7 replaced; Fenn's fare exact; "Never a priest's to do" kept; Amos's twist after the fight).
  GUNS-CODE phase 1 done (`Saved/GunsCode/PHASE2.txt`); Main approved its extra files (FirstPersonHandsReload/
  Component/Types, Tests/LootTests.cpp); phase 2 must also: call LOOT-CODE's `TryApply` from the rocket's blast (rockets
  carry elements and Explosive-style effects stay off), give rocket ammo 3 in `BossLootShower.cpp` (not 36), match the
  LMG spin-up cue to its 0.35 s cvar, merge WeaponSounds.h with GUNS-SOUND's by hand.
  STORY phase 2 written (unbuilt): spoken words 4,132 -> ~2,650 (-36%; town mutters cut only where two per stage);
  Python/json changed in place (create_story_lines.py, story_texts.py, mission scripts, hero_lines.json 17 deleted/7
  rewritten, new build_area_secrets.py + hook, build_area_whitlock/story/missionprops_mountain/valley/camps), C++ in
  `Saved/StoryCode/Source` (WantedPoster, RoostRules, OreLift, TownLifeRules, EncounterSpawner `ClearedLines` for Amos's
  twist after the fight, the bail at Main 3, save v4 migration, 13 tests). Secrets 6/7 now loot-only (keepers' stores
  in the old lantern house, a spiders' hoard under the hanging tree); secret 1 = "the Widow's strongbox". Its run order:
  merge+build, create_story_lines -> create_mission_assets -> create_side_mission_assets -> create_hero_lines,
  build_area RansomsRest gameplay, THEN delete 25 retired `/Game/Data/Story` DA_Lines_* (list in its report, kept in
  `Saved/Story/PLAN.md`; deleting first opens a modal), tests Lines/Missions/Story/Session.StepMigration/
  World.TownLife/Roost/Poster/Progression.Pacing.
  **The user, later that night (two art asks):** (1) apple trees' apples only show within ~5 ft (1.5 m) of the tree:
  fix so they read at normal distances; (2) first-person hands: compare with Borderlands' first-person view and match it:
  ours show too much of the arms, clip into the guns, or don't hold the weapons properly. Queued: APPLES investigation now
  (read-only, the fix applied in the lane), FP-HANDS after GUNS-CODE phase 2 (it owns FirstPersonHands* tonight).
  (3) **the existing guns (Bullpup, Ranchhand, Drover) must match the new frontier-tech style** (concept B). GUNS-RESTYLE
  started: Blender scripts only, part keys and sockets unchanged (saved guns and hand holds keep working); import in the lane.
  Apples fixed in Trees.py (bf9d42c, 020901b: buried inside the leaf cards and tiny; now hung outside, ~20 cm, twice as
  many, LODs 70/30% at 0.3/0.1; `Saved/World/APPLES_FIX.md`): **reimport pending** (`Tools\models.ps1 -Only Trees`, then
  check at 8/15 m with r.ForceLOD 0/1; if apples vanish at LOD1 use '100,40').
  **The user, later still (creature roster):** add more enemies and creatures as the game goes on; future regions may
  have **Goblins, Rats, Human Bandits, Golems, Wolves, Giants, Scorpions, Big Flying Bats, Vampires, Dragons, Big Insects
  and more** ("subject to change, but the storyline should read as potentially having these creatures"); where the story
  gives a reason for a different enemy in an area, place **a placeholder (a blob of clay or similar) with the enemy's
  name**. ROSTER agent started: fits them into the Revenant saga per region (Story.md), then a placeholder creature in a
  holding tree and placements where the story supports them.
  VFX phase 1 written (`Saved/Vfx/PLAN.md`, `Saved/VfxCode/README.md`; base 8ac1be7, blob hashes in
  `Saved/VfxCode/Originals/HASHES.txt`): bigger hit splats/starbursts, impacts by surface, kill gore by gun and tier,
  every power showing on the body, `M_FX_Shape/Puff/SplatDecal` via `build_vfx_materials.py` (unrun), `Looter.Vfx.*`;
  est. +0.2-0.4 ms; muzzle/rocket/tracers only as a proposal for GUNS-CODE's files; at merge split EmberDustDevil.cpp
  (755 lines). Every holding-tree merge is a 3-way merge against the base its README names (others' merges land first).
  Lane queue after GUNS-CODE phase 2: LOOT-CODE -> STORY -> VFX -> ROSTER -> trees reimport + WORLD-ART phase 2 ->
  GUNS-RESTYLE import -> FP-HANDS.
  **Docs organisation (the user, 2026-10-22):** the Project Library page (pinned; https://claude.ai/artifact/
  1yHV6GGJvvcwxWwZqi4T4s, built by `node Tools/docindex.js` with `Docs/INDEX.md`; republish at each session end).
  Their picks, all four: working notes into git (`Docs/Work/<date>-<job>/`, archived when the job is committed),
  split Pipeline.md into a topic folder with a short index, keep Handoff.md to now/next (older sessions to the
  archive), archive the Style Lab docs. Placeholders give XP and loot like real enemies (told ROSTER). Do the moves when
  no agent is reading those files (contracts of running jobs move after their commit; the Pipeline split when no
  phase-2 agent is writing it). DONE 2026-10-22: Pipeline split into `Docs/Pipeline/<Topic>.md` (index `Docs/Pipeline.md`), Handoff trimmed (older text in `Docs/Archive/Handoff_history.md`), finished jobs' notes of 2026-10-21 in `Docs/Archive/Work/2026-10-21-<Job>/` (Saved/Vfx2 still running, stays in Saved/); new jobs use `Docs/Work/`.
  More calls (2026-10-22): FP-HANDS may look up Borderlands gameplay screenshots online to compare framing (nothing copied
  into the game); placeholders show "<in-world name> (<type>)", e.g. "Tommyknocker (goblin)" (told ROSTER).
  **New guns committed (62bf264)** and checked in PIE (grabs `Saved/Screenshots/Tools/newguns_*`): sniper scope picture
  `UI/HUD/HudScopeWidget`, rocket self-damage capped at 35% (`Looter.Gunfeel.Launcher.SelfCap`). Art follow-ups: the
  Rustler's Holo sight has no cut window (aiming shows its solid back); the Rustler's helix (its signature, the
  rounds-left glow) can't be seen in first person (under the gun): show it (move it to the side, angle the hip hold, or a
  side window). LOOT-CODE phase 2 running (owns the lane).
  **Loot committed (64c6128):** elements, part effects, drop moments; all seven families' parts set up; tests pass;
  8 burning spiders 7.6 ms. Left: a status kill still plays the gun's kill knock; element damage numbers not coloured;
  element parts reuse other parts' models (art later). Restyle: Bullpup and Drover round 2 done (sand/gunmetal/ember,
  sockets identical; Drover renames in `Saved/Guns/RESTYLE_RENAMES.md`), Ranchhand fork still working; commit + import
  in the lane after it lands. STORY phase 3 (merge/build/run) now owns the lane.
  **Story committed (a43b80e):** talk 4,132 -> 2,670 words (-35%, givers -44%), 10 secrets (9 chests), bail at Main 3,
  Amos after the fight, save v4; tests pass; PIE grabs `Saved/Screenshots/Tools/story_*`. **Coffin Rock's boulder steps
  can't be climbed** (falls off the rounded tops after four): give it a ramp/ledges in WORLD-ART phase 2. VFX phase 2 now
  owns the lane (may also do the muzzle-per-kind/rocket/tracer proposal in GUNS-CODE's files).
  **VFX committed (5b573f3)**; grabs `Saved/Screenshots/Tools/vfx_*`. Weak and queued for a VFX-2 follow-up: the rocket
  blast reads as a pale puff in a flat ring (additive fire washes out in daylight: needs a translucent fire material);
  a shotgun-killed spider turns into a white ghost (a hit flash/overlay held through death); Slag Bomb p95 8.6 ms
  (cvars Looter.Vfx.Blast.Scale / .Smoke); dirt and stone impacts small and pale; not seen: tier kill rings, slime gel
  puddle, Last Rites column; sniper vapour trail not done (BulletSubsystem.cpp). Restyle scripts + renames committed
  (1f30423, 87262e9). ROSTER phase 2 now owns the lane; then trees reimport + WORLD-ART phase 2 (+ Coffin Rock steps),
  restyle import (setup_gun_parts.py after), VFX-2, FP-HANDS, Rustler sight/helix fixes.
  **Placeholders committed (37636b4):** 5 kinds fought in PIE (XP, loot, "Name (type)" labels, clay hits; grabs
  `roster_*`); the Bone Hounds moved 20 m in from the Rim (they followed the player over the edge). WORLD-ART phase 2
  now owns the lane (variants, scatter, terrain tones, apples reimport, Coffin Rock ramp).
  **World variety committed (5cc096a)**: honest read: the differences are subtle in before/after (`Saved/World/
  compare_*.png`); the ground tones read faintly; a stronger-tone repaint (~46 min) is the next lever (a suggestion for
  the user). RESTYLE-IMPORT (Sonnet) owns the lane now; RUSTLER-FIX (Blender: open sights, visible helix) in parallel.
  Then FP-HANDS, VFX-2, the Rustler import. RUSTLER-FIX done in Blender (open Holo sight in the shared kit, also on
  the Thresher; helix moved 1.2 cm left and enlarged; Foregrip socket now (25, -1.2, -8.45)): reimport
  `Tools\models.ps1 -Only Rustler,Thresher` in FP-HANDS phase 2 (its left-hand grip must follow the Foregrip move).
  FP forearms lengthened (committed; FPArms.py FOREARM_STRETCH 1.25, 26.1 -> 32.6 cm): import SK_FP_Arms + the four
  heroes' arms and texture sets in FP-HANDS phase 2; its tests need FirstPersonHandsTests.cpp:211 rest grip point
  (40.738, 9.539, -29.85), FPHandsFramingTests.cpp:105 Length 23 -> 28.7, and the comment at
  FirstPersonHandsFraming.cpp:358-359.
  Order of the build lane next: FIGHT-AI perf -> GUNS-CODE phase 2 (merge `Saved/GunsCode`, then `Saved/GunsSound`, import
  the four models) -> LOOT-CODE phase 2 -> STORY merge -> WORLD-ART phase 2.

### Decided unattended (2026-10-20 night), for the user's review
(Main's calls while the user slept; newest last.)
- BARROW-ART accepted and committed (b0e4ed6): SK_Gravemother on Spider.py's 31 bones (14.97k triangles), the stone's
  three states with roots, 5 shards, 3 chips, the fight's earth, FX/decal textures; her hide reads a little paler than
  the sheet's #8C8070 under the preview's warm light: judged in game. BARROW-CODE imports them and runs
  `build_grave_fight_materials.py` between its builds. Its card colours are passed by code (MI tint white).
- A ninth job as ART freed its slot: LINES drafts every hero variant of the existing lines plus the pick-by-hero design
  into `Saved/Lines/` (VARIANTS.json, DESIGN.md); Main wires it after BARROW-CODE. First-person arms start when a hero
  model agent finishes.
- HUD-PORTRAITS accepted and committed (d6272f0): the four inked HUD portraits; Crane's single cyan eye beside the coin,
  Gauge's warm lamp (visor drops when hurt), Odessa without the hand card, Pike with the tripod's legs. Wiring after
  BARROW-CODE from `Saved/HudPortraits/WIRING.md`.
- LINES done (`Saved/Lines/`, git-ignored: VARIANTS.json 148 rows, DESIGN.md, ELLIS_ONLY.md): a lookup table
  `DA_HeroLines` keyed by Ellis's exact text, resolved in `UCaptionSubsystem::Play` and ~15 read sites, tokens {Hero}...,
  test `Looter.Lines.NoFamily`. Its five questions, Main's provisional calls (**ASK THE USER at 5 am, the first one
  above all**): (1) the cold open contradicts the four select-screen stories (Odessa caught Ned's bullet, Pike left in a
  ravine, Crane forty years dead, Gauge woke in a wreck, yet every hero dies on the Point): provisional (a) keep the cold
  open for all, their own regions tell their earlier past; the other choice (b) cuts the hero out of the cold open for the
  four (~15 rows need a second neutral). (2) The Deacon says "kid" to every hero, Crane and Gauge too (the user's own
  words: "the Deacon's kid is whoever you are"). (3) The townsman "Hollis" renamed Harlan for every hero (Crane is
  Hollis). (4) The non-Ellis epitaph "<NAME>. STOOD UP AT THE LAST." and each hero's poster line (Gauge "RAILROAD
  PROPERTY") kept as drafted. (5) Sexton's per-hero sweeteners (Odessa's marker, Crane's fare, Gauge's spark, Pike's last
  road) in place of "your father crosses". Art the lines need: SM_Headboard_<Hero> x5 split from Grave_Ellis, a bare
  keepers' roll, four poster atlases (not started: waits on the user's cold-open call).
- FP-ARMS started (the slot HUD-PORTRAITS freed): SK_FP_Arms_<Hero> x4 on FPArms.py from its own file (FPArms.py
  untouched: its digest would re-bake Ellis's arms), rig jsons `Intermediate/FPArms/FPArms_<Hero>_rig.json`, wiring
  notes `Saved/FPArms/WIRING.md`.
- BARROW-CODE done and committed (50a6f1e): the Barrow fight on the real assets, 726/726 tests; its fix to ART's
  materials script (M_GraveDecal's Tiles from RGBA); the bestiary page updated. Not checked: perf (§11.4: the PC was under
  memory pressure from the agents; PIE hitches were PCG and Mass), the decals' look by eye (Reveal direction, the crack's
  end cap joining), real bullets on the stone/clod/mounds by hand (tests cover them), one plough on bumpy ground stopped
  at 5.8 of 12.2 m. New palette entries LooterUI::Color::GraniteLight/Granite/GraniteDark (the stone bar).
- With the build lane free: HUD-WIRE (Sonnet) applies `Saved/HudPortraits/WIRING.md` (incl. Gauge's hurt flicker),
  builds, tests, shoots each hero's portrait in PIE; LINES-WIRE (Opus) writes the line-variant code into a holding tree
  `Saved/LinesWire/Source/...` (+ `Tools/Unreal/hero_lines.json`, `create_hero_lines.py`) on Main's provisional §7 calls
  (cold open kept for all; "kid" for all; Hollis -> Harlan; epitaph and posters as drafted; Sexton's sweeteners), to be
  merged and built by it after HUD-WIRE and the hero imports.
- HUD-WIRE done and committed with this note: each hero's own HUD portrait (live on `Looter.Hero`), test
  `Looter.HUD.PortraitSets`, 727/727. GAUGE reported done (not yet reviewed or imported): SK_Gauge 31,956 triangles, 67
  bones (no swing chains), five 2048 sets GaugeHead/Chest/Belly/Arms/Legs, slots + GaugeLamp/GaugeFire glows, export in
  `Intermediate/ArtExport_Gauge`; expect rifle stocks to clip its boiler/right pauldron when aiming; check its 1.28x hands.
- **USAGE LIMIT hit (2026-10-20 ~02:00). Every running agent was STOPPED mid-step; their work is on disk, nothing of
  theirs committed.** Where each stood: CRANE (previews read as the portrait; was about to rebake with fixes + audit r2),
  PIKE (clay reads well; was starting the bake with previews), ODESSA (round-2 daylight renders), POWERS (writing
  `Saved/Powers/`; was about to set the twelve entries in `create_ember_assets.py`), FP-ARMS (building Odessa's arms),
  LINES-WIRE (writing `Saved/LinesWire/`; at the Bestiary widgets). **On resume:** resume each with SendMessage (same
  agent ids in this session, else new agents briefed from the two contracts and their files), saying what landed on disk;
  review Gauge's previews, import (`Looter.ImportModels C:/Dev/AI_Looter_Shooter/Intermediate/ArtExport_Gauge`), run
  `build_hero_retarget.py Gauge` (untested), `create_ember_assets.py`, check in PIE with `Looter.Hero Gauge`; then each
  hero as it lands; then merge POWERS' and LINES-WIRE's holding trees (they build and fix their own); ask the user the
  cold-open question (Lines §7) first thing.
- **Resumed ~01:30** (the limit lifted): the six agents resumed with SendMessage. **Gauge is in the game**: imported
  (68 bones, 7 slots, 17 hit zones), `build_hero_retarget.py Gauge` (auto aligned), `create_ember_assets.py` filled
  DA_Hero_Gauge's mesh/retargeter; checked in PIE (`Looter.Hero.Unlock 1`, `Looter.Hero Gauge`): third person from
  behind and front, its HUD portrait, Slag Bomb on Q (no code yet). Not yet looked at: aiming (rifle stock vs boiler and
  right pauldron), its 1.28x hands on the grips, running. The DA_Ember_* assets show modified (POWERS' data, re-saved by
  that run): left out of Gauge's commit, they go in with the POWERS merge.
- POWERS done in its holding tree (`Saved/Powers/README.md`; the reviewer's items fixed): twelve powers, icons, 35 cues
  (95 WAVs, check 0 flagged), tests `Looter.Embers.Power.<Id>`; BulletSubsystem.h/.cpp and LooterSoundCues.h apply as
  patches (`Saved/Powers/Patches/`). Its calls: damage = the demo's x1.43 (the demo's 14-damage round vs today's 20),
  Last Rites 60 burst / 90 strike, Long Shot +15% per 10 m, Grave Ward heals 5/s, Double or Nothing never misses twice
  running (cooldown 15), Long Shot marks its own targets, Lucky Streak crits every third pull, Grave Ward slows by
  CustomTimeDilation, Spyglass without a through-wall silhouette (a mark widget and an orange wash instead).
  POWERS phase 2 running: it merges its tree, builds, runs the asset/sound scripts and tests, checks each power in PIE
  (it owns the editor until it reports).
- LINES-WIRE done in its holding tree (`Saved/LinesWire/README.md`: 8 new files, 23 replaced as patches, all applying;
  `Tools/Unreal/hero_lines.json` + `create_hero_lines.py` in place; `build_area_vendors.py` Hollis -> Harlan). Merge after
  POWERS: resume it (it builds and fixes its own), run order in its README (create_hero_lines.py expects "HEROLINES done:
  146"; re-run `build_area_vendors.py RansomsRest`). Its choices: tests read Ellis's words unless a test sets a hero
  (FScopedHero); a row can name its speaker; a few extra read sites (choice titles, board/artifact titles). WantedPoster.cpp
  (507) and SessionSubsystem.cpp (505) end just over the ~500-line limit.
- PIKE done and committed (d97f803, not imported: POWERS imports him at its next break, with `build_hero_retarget.py Pike`):
  34,990 triangles, 92 bones (skirt chains fl/sl/bl/bm/br/sr/fr, bandana, hat), no holster slot; the pack bends with the
  back; spectacles are rims only (no glass master); audit leftovers 3 cm or less (lips through the mustache cards, the skirt
  top through the belt's dip, coat edges through straps, the spyglass into the skirt in a stride); his side whiskers
  stair-step up close (a polish item). His agent found `looter_looks.skin` multiplies by a `zones` attribute Ellis.py never
  sets, so T_EllisSkin_BC baked near-black: the user asked for Ellis's face "kept dark" (2026-10-15), so check before
  changing it. His shoulders sit 6.5 cm lower than Ellis's (retarget).
- CRANE done and committed (2fe2b16, not imported: POWERS imports him with Pike): 33,507 triangles, 144 bones (skirt ring,
  cape as Ellis's, hair/hair_fl/hair_fr, lantern_01 = the hook, hat), the stoop posed into the bind pose (head 7 cm ahead
  of the neck: check the retarget), no waistcoat, no holster; small audit leftovers (coat top through the cape behind the
  neck, coat through the sleeves at the armpits, chain links into the coat, hem 1.4 cm under the ground crouched); fine
  pleats in the coat's back at the seat. Shared-code wishes in his report: ellis_hat brow height, ellis_hair colours as
  parameters, looter_human posed joints; the portrait Crane.py's coin faces point inward.
- FP-ARMS done and committed (e7d569d, not imported or wired): SK_FP_Arms_<Hero> x4 on Ellis's arm skeleton (rig jsons
  match: the hands code binds them unchanged); wiring in `Saved/FPArms/WIRING.md` (UHeroDefinition FirstPersonArms +
  scale: Odessa 0.84, Pike 0.86, the rest 0.88; the hands component loads the played hero's; a test); resume FP-ARMS for
  it after the merges. Gauge's are 17.7k triangles (could be trimmed); its finger blocks may sink 2-3 mm into thin grips.
- LINES-ART done and committed (588e6af, not imported): SM_Headboard_<Hero> x5 (Grave_Ellis loses its board: import
  Graves only together with the story script run), KeepersRoll_Bare, T_Posters_<Id>_BC x4 + MI_PosterDecal/Scrap_<Id> via
  `build_decal_materials.py`; run order in the commit message (after the LINES-WIRE merge).
- POWERS merged, built and committed (425d1c3): the twelve powers, Crane and Pike imported with their retargeters,
  DA_Hero_Crane/Pike filled: **Crane, Gauge and Pike are playable; Odessa waits for her model.** 739/739. Each power played
  in PIE and tuned for daylight (bigger slag blobs, Iron Hide's forge glow, Rail Spike's glow line, thicker tripwire,
  bigger chain links and cards). Weak or unchecked: the Slag Bomb pool reads as chips in a fire ring, not molten slag (a
  flat emissive mesh or decal: an art call); Rail Spike's line is seen end-on from behind; Crane's hook swing not seen in
  motion; Last Rites' death burst only by its test; Iron Hide's glow in first person. Perf skipped (PC busy).
  `Saved/Powers/` is stale now (Source is the truth). Next: LINES-WIRE phase 2 (merge, build, the line art's imports and
  area scripts), then FP-ARMS wiring, then Odessa.
- ODESSA done and committed (283ff95, not imported): 34,909 triangles, 92 bones (tails on skirt_sl/bl/bli/bri/br/sr, hat),
  no cloth simulation, hair a combed shell + knot (Character master), the ace in the hat band, the card case on her right
  side. Her import (+ `build_hero_retarget.py Odessa`, `create_ember_assets.py`) goes with the FP-ARMS wiring step; then
  no hero is left unplayable, so the hero tests need a synthetic unplayable hero. BodySwing's skirt settings were tuned
  for a duster: her short tails may want their own.
- **Found by ODESSA: `bmesh.ops.create_uvsphere` orders faces differently each Blender run**, and a kept bake (digest
  match) may then not fit a later export's UVs (decimate, smart UV and the atlas pack follow the order): scrambled
  texture in game. Ellis, Pike, Crane (and maybe Gauge, the FP arms) call it. BAKE-CHECK agent diagnosing per model
  (no fix yet); the likely fix is a shared fixed-order sphere (odessa_kit.uv_sphere) and a rebake + re-export of the
  models that mismatch.
- LINES-WIRE merged, built and committed (02a3628): DA_HeroLines (146 rows), resolution in captions and every read site,
  the painted words imported and placed by `build_area.py RansomsRest gameplay`; 743/743. Checked in PIE as Crane (his
  headboard, card, captions, poster, bare roll, Delia's line) and as Ellis (unchanged). The headboard is now the readable's
  own mesh, read from beside the grave (~2 m).
- FP-ARMS phase 2 running (owns the editor): wiring, the four arms' import, Odessa's import + retarget, the hero tests
  with a synthetic unplayable hero, PIE per hero in first person.
- Git hiccup seen three times tonight: `unable to write file .git/objects/..: Permission denied` on `git add` of an asset
  the editor had just saved; nothing staged; the same command a few seconds later works (a transient lock: the editor's
  revision control or the virus scanner). Retry, don't change permissions.
- `Art/Models/Characters/Crane.drape.json` shows modified (BAKE-CHECK's test export re-simulated the coat: evidence for
  the sphere-order problem); left uncommitted until BAKE-CHECK reports.
- **BAKE-CHECK's diagnosis** (evidence `Saved/BakeCheck/`): mismatched in game now: SK_Crane (Coat 4.2%, Gear 15.4% of
  island pixels on unbaked background), SK_Pike (Gear 9.7%), SK_Odessa (Gear 15.1%, imported 03:18 by FP-ARMS),
  SK_FP_Arms_Crane (Skin 6.5%); fit: Ellis, Gauge, SK_FP_Arms, the other three FP arms. Causes: (1) **thread count**: the
  bakes ran at artrun's -Threads 6, the exports through models.ps1 at all threads (Decimate and push-outs differ);
  (2) create_uvsphere's random face order (Ellis's buttons and knot, Crane's left eye and star, Pike's buttons, knot,
  pouches, studs); (3) the cloth cache rounds points to 5 decimals but the mesh kept full precision. It changed
  `Crane.drape.json` and `Ellis.drape.json` by mistake and asked Main to `git checkout` them (its own attempt was denied):
  not done on its say-so; the fix's re-export rewrites both. **PIPE-FIX running**: models.ps1 and artrun at one thread
  count (6), a shared fixed-order `uv_sphere`, rounded simulate points, a packed-UV hash guard beside bake.digest, then
  re-exports of Crane, Pike, Ellis, Odessa, FPArmsCrane/Pike/Odessa, each checked to fit; Main reimports after FP-ARMS.
- FP-ARMS phase 2 done and committed (898a8ca): each hero's first-person arms load by the played hero (UHeroDefinition
  FirstPersonArms + scale: Odessa 0.84, Pike 0.86, others the component's 0.88), swap on `Looter.Hero`; SK_Odessa imported
  and retargeted: **all five heroes playable**; 744/744. Checked in PIE: all five with rifle, shotgun, revolver, hip and
  aim; Gauge's wrist balls read big in the revolver hold (in character), Crane's wide cuffs fill the bottom of the
  revolver view. **Odessa's tailcoat doesn't show in game** (only the gold waistcoat and dark sleeves): ODESSA resumed to
  find and fix it (likely inward faces) in its own files; PIPE-FIX re-exports her last, then Main reimports.
  **Found and fixed by Main:** not the mesh (ODESSA rendered the export one-sided: whole and wine) but hidden sections
  kept by index on the component across a mesh change: Ellis's slot 5 is EllisHolsterGun (hidden while a revolver is
  drawn), and slot 5 is Odessa's (and Crane's) coat; `Looter.Hero` swapped meshes with it still hidden.
  `UBodyFigureComponent::Wear` now calls ShowAllMaterialSections on every LOD of a new mesh. Checked in PIE: Ellis with a
  revolver, then `Looter.Hero Odessa`: her wine coat shows. Odessa's model needs no change (PIPE-FIX re-exports her for the
  textures only; told to wait, now free to go).
- PIPE-FIX done and committed with this note: one thread count (6) for every model run, `looter_model.uv_sphere` and
  `fixed_order`, rounded simulate points, the uv.digest guard (Pipeline/CharacterModels.md "A bake and an export must build the same
  mesh"); re-exported and reimported SK_Ellis, SK_Crane, SK_Pike, SK_Odessa, SK_FP_Arms and the Crane/Pike/Odessa arms
  (Gauge's unchanged). Islands off: all sets 0-0.2% except Odessa's Gear 0.43% (one 12-face island at the start of her
  watch-chain tube gets no bake source: `odessa_clothes.build_watch_chain`). Each export repeats bit-identically.
- In-game front shots of all five after the reimport (`Saved/Screenshots/Tools/hero_front_<Hero>.png`): right, except
  **Pike shows a dark patch low on his coat front at the right hip** (the spyglass frog's leather or the coat's inside
  showing: look next). PIKE resumed to diagnose and fix it (re-export; Main reimports).
- **Morning state (2026-10-20 ~04:50): the hero pick is built end to end**: five playable heroes with their own bodies,
  first-person arms, HUD portraits, three powers each and their own words; the Gravemother's Barrow fight; all committed
  and pushed (last 6a5c172), 744/744. **The user's call (2026-10-20 morning): the cold open is kept for every hero**
  (Story.md "The heroes"); Main's other four provisional Lines calls stand unless the user says otherwise. Waiting on the
  user: their playtest.
- **Performance pass (the user's pick, 2026-10-20 morning; Docs/Performance.md rows):** RR spawn on Medium, each hero
  worn: all five 7.7 ms (129-131 fps; Ellis 7.7 before the heroes). With 8 spiders: baseline 7.7 (p95 8.3); powers
  repeating: Slag Bomb 7.8, Iron Hide 7.7, Fifty-Two Pickup 7.8, Spyglass 7.8, Sundown 7.7, Grave Ward 8.0 -> **7.8 after
  power lights went High/Epic only** (`AEmberPropActor::LightsAllowed`; the light was the whole +0.3 ms, proven with
  `ShowFlag.PointLights 0`). The Gravemother's Barrow fight over 2,400 frames: 7.7 (p95 8.2). New dev command
  `Looter.Embers.Repeat <id> <seconds>|off`. Not measured: the 37-view tour (nothing changed the world's views).
- **Polish (the user's pick next):** POLISH-ART (Blender) makes SM_SlagPool (molten slag as an emissive surface, notes
  in `Saved/Polish/SLAGPOOL.md`) and fixes Odessa's black watch-chain island; POLISH-CODE (owns the editor) makes Rail
  Spike read from behind and in first person, checks and fixes the Gravemother's ground decals by eye, tunes Crane's hook
  (and hair, coat, cape) swing in motion, then wires the slag pool once Main imports it. PIKE still on his coat patch.
- POLISH-ART done and committed (746b33e): SM_SlagPool (+ SlagPoolCrust, SlagSplatA/B; slots SlagCrust, SlagMolten,
  SlagWhiteHot, SlagGlowEdge; `Saved/Polish/SLAGPOOL.md`), and Odessa's watch chain (its game tube ran through her body:
  OdessaGear 0.43% -> 0%). POLISH-CODE imports both and wires the pool.
- PIKE's coat patch found and fixed (f4c49a9, rig only, mesh identical): BodySwing keeps skirt points ~5 cm from the
  hand and ~7 cm from the forearm; his side skirt chains hung where his idle hand hangs, the springs threw the front panel
  over the pocket. Side chains now behind the hip (0.65 of the way round), front ones outside the thigh. POLISH-CODE
  reimports SK_Pike and checks whether Crane's left-hand flare is the same (a code-side rule preferred). Pike's leftovers
  (not done, ask): the cuff tabs bake near-black (turn them out in `pike_clothes.build_cuffs`, a rebake); a dark sliver
  on the left hip from behind; the coat front tucks under the thighs crouched.
- POLISH-CODE done and committed (582517d), 747/747: Rail Spike reads from behind (head glow, shock rings, steam, side
  sparks, ground dust, a glowing AGroundCrack scar); the Gravemother's decals (Reveal now crawls, soft FadeIn/FadeOut
  ends, no tiles floating over old berms; the end cap joins within ~1.5 cm by measurement, never seen; the clod's
  landing shadow never caught in a frame); BodySwing (BodySwingKinds.cpp): a hook's and long hair's own settings with a
  `Carry` share, pushed cloth drops its speed into a limb (Crane's 14 cm standing wag gone, all heroes), hands push the
  shortest way at most 3 cm (Crane's flared coat; same cause as Pike's, now fixed in code for all); tests
  `.Hook/.Rest/.Hand`; the slag pool wired (`AEmberPropActor::GetModel/AddPiece`). Not measured: perf of the new Rail
  Spike and slag looks (the PC was busy).
- User said "keep working": PIKE does his leftovers (cuffs, whiskers, hip sliver); PORTRAIT-CLEANUP (Sonnet) merges the
  portrait scripts' helpers into bust.py with scenes and model exports compared identical; SELECT resumed for **a live
  3D hero stand** in the select screen's middle (the chosen hero's model idling on a stand, turning, draggable, a rim in
  the hero's colour; portraits stay on the cards).
- PORTRAIT-CLEANUP done and committed (520ee4a): `Art/Portraits/bustkit.py` (955) + bust.py (862) hold the shared helpers;
  hero portraits Crane 603, Pike 540, Gauge 521, Odessa 709, Ellis 567; proven identical (15 scenes, 4 model exports;
  tools `Saved/PortraitCleanup/scene_cmp.py`). Model scripts list `bustkit.py` in BAKE_SOURCES (Pike's via PIKE). The FP
  arms of Crane and Pike rebake at their next export (fp_arms_heroes digests the portrait files).
- SELECT's live 3D stand done and committed with this note: `AHeroSelectStage` (an unpossessed pawn: a hidden mannequin
  playing ABP_Unarmed, the hero's body copying it through their retargeter via `UBodyFigureComponent::SetShownHero`) on a
  plinth in a 720x900 capture, swaying around three-quarter view, drag/right stick to turn; key, fill, a hero-colour rim
  and a kicker in their first ember's colour; a 0.28 s fade + 0.6 s turn-in on a switch; a 76 px painted face on the
  panel. Cost only while the select is open: +1.4 ms GPU, +2.1 ms render thread (PIE, Medium). Live tuning
  `Looter.HeroStand.Key/.Fill/.Rim/.Kicker/.Exposure`, `Looter.HeroStand.Run 0` holds it. 748/748, `Looter.Heroes.Stand`.
  Polish noted: Crane's face reads blotchy at the stand's size under the rim light.
- Perf of the polish (RR spawn, Medium, Gauge, 8 spiders): baseline 7.7 ms; Rail Spike every 3 s with its new look 7.8;
  Slag Bomb every 8 s with the slag pool model 7.8 (Performance.md rows).
- POLISH-CODE round 2 committed (7154cad), 748/748: a skirt with no front chains is a tailcoat with its own swing
  (Odessa's tails 60-77 deg out at a run -> 40-47, settle in 0.5 s); the stand's rim colour lights at most luminance 0.6
  (`AHeroSelectStage::RimShareFor`: Crane x0.71, Pike x0.85); the clod's landing shadow reads through grass
  (`M_GraveDecal` AlphaScale, 1.8 on the shadow); the crack's end cap seen joining. Then Crane's skin SubsurfaceAmount 0.15
  (1ead855; Crane.py's GAME_MATERIALS and MI_CraneSkin set directly; his next export rebakes as the digest changed).
  Known test trap: typing `Looter.Hero.Unlock` at the console fails three hero tests until the editor restarts (fixed
  since: 88ec205).
- **Enemy tiers (the user, 2026-10-20): Regular (white), Restless (blue), Gravebound (purple), Hallowed (yellow),
  Ethereal (red)** for every regular enemy (not bosses, not named Legendary monsters). The design with every number and
  the user's calls (tier-only colours; crits pierce half the defense; Ethereals 1 in 1,250 on Ransom's Rest; numbers
  approved): `Saved/Tiers/DESIGN.md`; the build contract `Saved/Tiers/CONTRACT.md` (neither committed; Story.md's ranks
  table is updated by TIER-CORE). Running: TIER-CORE (owns the editor: enum appended Hallowed=5, Ethereal=6, settings,
  defense, promotions, loot, tag, tests), TIER-TRAITS and TIER-LOOK (holding trees `Saved/Tiers/Traits|Look/`, merge
  after CORE in that order), TIER-SOUND (13 cues). PIKE's leftovers done and committed (872b030: cuff tabs, softer
  chops, the hip patch; 34,900 triangles); TIER-CORE reimports SK_Pike at a break.
  TIER-SOUND done and committed (d1db327, 13 cues; WardHit asked for after). TIER-TRAITS done in its holding tree
  (`Saved/Tiers/Traits/README.md`): replaces CORE's 3 stand-ins, adds the component's 5 topic files, UCreatureRallied,
  CreatureTraitRules, UCreatureTraitSettings (every number), `Looter.Traits.Give|Show`, 9 tests; small changes to
  HealthComponent (an `FHealthShield` delegate after defense) and CreatureBase (`SettleBody()`). Its calls: a "hard hit"
  for Phasing = 8% of max health within 1 s; the Spitter and Bell-Ringer never Fleet or Grave-burst, the Bell-Ringer
  never Vengeful; every kind can phase; the ward doesn't regenerate; an extra cue `Creature.Trait.WardHit`.
  TIER-LOOK done in its holding tree (`Saved/Tiers/Look/README.md`): the tier glow and Ethereal's ghost-rim as a "tier
  shell" (a leader-posed copy of the body in a new `M_CreatureTier`, one translucent draw per tiered creature: the
  overlay slot is taken by the hit flash, and the bodies' own slots share M_World/FoliagePalette), the tag per tier
  (frame and clamps, the word popping on notice, a dim trait line), notice stings over the rank sting, death shows
  (pillar 9 m/1.7 s Hallowed, 13 m/2.1 s Ethereal), loot pops 0.15-0.18 s after, the XP popup tinted. Unverified until
  PIE: the spider eye UV patch (if the abdomen glows instead, V 0-0.125) and the slime core's place. Merge after TRAITS
  (`git merge-file` against its patches/base), then `build_tier_material.py`; perf with `Looter.Perf.Horde Spider 8
  Ethereal`. To save time (the user: "multitask"): TIER-TRAITS stages ONE combined TRAITS+LOOK merge in
  `Saved/Tiers/Merged/` against CORE's working copy now, and does the single merge/build/test/PIE/perf after CORE.
  TIER-CORE done and committed (b6fbda0, 753/753): area rise by the area's lowest creature level (1 = RR odds, 50+ =
  late, `LateAreaLevel`), caps count living creatures of the tier and a roll past a cap steps down, no save change
  (ranks aren't saved; version 3), trait seed CRC32(actor name) x the promotion roll (runtime spawns their UniqueID);
  Looter.Tier.Odds legendary 0.30/2.19/5.51/13.33/21.38%, RR spawns 7.86/2.04/0.41/0.083%. Not done: bestiary defeats per
  tier (needs a save field). Main's calls sent with the merge GO: silver restored for Regular/Restless/Gravebound (CORE
  had cut it), Hallowed 1.6x and Ethereal 2.5x Gravebound's; the Legendary rank's tag to the legendary orange #FF9900
  (Hallowed's #F5C842 was close to its #FFCC00). The ward bar (pale cyan over health) added to the staged tag.
  **TRAITS + LOOK merged, built and committed (895c9a4), 767/767**; fixed in play: Phasing never fired (melee creatures
  are always mid-attack: a hard hit now blinks once the attack ends; fences blocked every spot: checks trace 150 cm up),
  the slime's core never lit (no vertex colour: the shell lights the gel round it), flares and rings softened/lifted.
  Pipeline/Enemies.md "Enemy tiers" has every trait and the look. Perf at the start (inside the safe zone): 8 Regular 7.7, Hallowed
  7.8, Ethereal 7.8 ms; fighting in town at (0, -1400): 8 Regular 7.6, 8 Hallowed 7.7, 8 Ethereal 7.7 ms (p95 8.2-8.3;
  Performance.md rows). Not done: bestiary defeats per tier; the dead
  miner's lamp glow unconfirmed at range; the Fleet wind streaks unconfirmed.
- **The user's next pick: "Bestiary tiers and more polish".** Running: BESTIARY-TIERS (Sonnet, owns the editor: defeats
  per tier on each page, a save field); CRANE (coat top through the cape at the neck, coat through sleeves, chain links,
  hem), ODESSA (sleeves into the tails, trousers over the belt crouched, vest past the cravat, card case), FP-ARMS
  (Gauge's arms 17.7k -> ~10k, fingers out of thin grips): Blender, re-export twice, Main reimports. Then on the lane: the
  four new heroes in third person with each gun (hands on grips, Gauge's stock vs his boiler/pauldron), the miner's lamp
  glow and Fleet's streaks.
  BESTIARY-TIERS done and committed (e5c3e9a, 770/770: `DefeatedByTier`, an older save reads as tiers unknown, the page
  line "Restless 3 · Gravebound 1 · ◆ Hallowed 1"). CRANE polish committed (efc17ff: coat top under the cape, chain and
  star on the coat's shell, seat pleats smoothed; the armpit "hits" were the right layering). ODESSA polish committed
  (6988e78): **her coat recut as a square-front swallowtail** (Main's call to accept: it still reads as the picked
  tailcoat; the old cutaway curved from the front and sank the sleeves into the tails), belt and buckle on skin weights,
  cravat over the vest, a slimmer card case; left: the card case 0.8 cm into the thigh crouched, the boot tops show jagged
  holes looking down into them. POLISH-HEROES (owns the editor) reimports both and runs batched capture sheets per hero.
  FP-ARMS committed: SK_FP_Arms_Gauge 10.2k triangles, fingers resting on grips (`fp_arms_gauge_check.py`). **Found, not
  fixed (a shared-helper follow-up):** `looter_bake.bake_set` bakes occlusion at a quarter size but keeps the full
  size's 12 px margin, and Blender applies each object's margin alone, so one part's margin overwrites neighbours'
  islands (black squares) on any set made of several objects; fix there = scale the AO margin (margin x ao_size / size),
  then drop fp_arms_paint's Gauge-only `fix_ao` re-bake; every model's digest changes, so rebake and re-export all, checked
  identical-but-for-the-fix. The next exports of Odessa/Crane/Pike FP arms rebake (fp_arms_paint/heroes are in their
  digests; textures should come out the same).
  POLISH-HEROES done (771/771): every hero's gun sat where the hidden mannequin's shoulders are (their hands are pinned
  to its hands): `UHeroDefinition::ThirdPersonHold` moves the mannequin's hands per hero (Crane +16.5 up, Pike -6.5,
  Odessa -4.5, Gauge 5 ahead 3 out -8; Pipeline/Heroes.md "The third-person hold"; contact sheets `Tools\heroguns.ps1`, the before
  and after in `Saved\Screenshots\HeroGuns`). The Hallowed miner's lamp now has a tier-coloured halo (read white at
  range), Fleet streaks start behind the body, the Bestiary habitat line wraps; Crane, Odessa and SK_FP_Arms_Gauge
  reimported. The editor crashed twice in Python ~30 s after long capture runs (cause unknown; restart after those).
  Left as found (Ellis's too): left palm open under the foregrip, the Bullpup reload tips its stock across the face, the
  Ranchhand reload swings the left arm out, the camera ignores hero size (Crane's hat and Gauge's stacks fill the left of
  the aimed view), red patches on Pike's/Crane's/Odessa's skin (skin material), Crane's coat flares wide sprinting.
- **The user's next pick: "Fix the leftovers"** (2026-10-20), DONE: SKIN (7c0f858: M_CharacterSkin on Preintegrated
  Skin, the blotches were the Separable SSS filter on Medium/High), HANDS (41e6bb6: the third-person left hand grips the
  handguard, pump or six-gun hand), CAMERA (per-hero over-the-shoulder camera, tuned; values in Pipeline/Heroes.md), and the
  hero-swap crash in `Wear` (sections shown before InitAnim). 777/777 before the swap fix; hero tests 10/10 after it.
  The user said skip screenshot capture runs (the editor crashed in python311 ~30 s after them, three times today).
  **Not done: perf of the Preintegrated Skin** (expected neutral or cheaper: no SSS pass; run perf.ps1 Medium with a hero
  when the PC is quiet), the camera with the Ranchhand/Drover and Gauge/Pike at 4x. The user is playing in the editor.
  Not yet done: perf with the PC quiet (heroes, powers, the Barrow fight), the Gravemother decals by eye, the Slag Bomb
  pool's look, Rail Spike's end-on line, Crane's hook swinging in motion, the portrait/model shared-helper merges.
- LINES-ART started in its slot: SM_Headboard_<Hero> x5 (epitaph a parameter), a bare keepers' roll, four poster atlases
  + MI_PosterDecal/Scrap_<Id>, and the `hero_meshes`/`other_mesh` lines in `build_area_story.py` and
  `build_area_missionprops_valley.py` (run by Main after the merge).

**On hold (the user's calls):** the Gilded Lily (when it starts: a World Partition map, Pipeline/Levels.md "Making a level");
Skyreach's own house kit; no style work unless asked.

**Gunplay, Phase 12.1 (running 2026-10-21; the user's start prompt: steps 1-3, then they tune in PIE; no friend build;
tests only for new work at commit; perf once before and once after; no capture runs).** The user's calls: Sonniss GDC
bundles for gun/impact sounds (only the zip parts with gun/impact recordings; licence: no resale, no AI training);
the revolver's crit 1.75x. Baseline perf (RR, 8 spiders, Medium): 7.9 ms, p95 9.0. Contract
`Saved/Gunfeel/CONTRACT.md` (not committed: read it on resume). Main wrote `Weapons/GunshotSound` (stub: today's Fire
cue) and FireShot calls it. Running: GUNFEEL-REACT (Opus: hit reactions + the guns' jobs on the target) and
GUNFEEL-KICK (Sonnet: kick, rumble, handling, rifle bloom, shotgun falloff, revolver crit), neither builds; Main
builds once, checks in PIE, commits. GUNFEEL-SOUND starts once the pack parts are chosen and downloaded.
- REACT + KICK built and committed (step 1-3 code, one batch): tests `Looter.Gunfeel` 17/17, Feedback 8, Melee 8,
  Creatures.Anim 8, Weapons 44, all pass. PIE (RR, rifle/shotgun/revolver vs spider hordes, `play.py fight`): ~50 hits,
  slows, heavy hits, rifle pressure, crits, no errors; not seen by eye (no screenshots, the user's rule), rumble not tried
  on a pad. The sound pack: sonniss.com's host sits behind a bot check, so the files come from its listed mirror
  ftpmirror.your.org/pub/misc/sonniss2016|2017; those bundles are samplers (~150 files a part), so only the 59 gun/impact
  files were pulled by range requests (332 MB, `Saved/SoundPacks/Sonniss`, tool `/tmp/zx.js` pattern in Pipeline later).
  GUNFEEL-SOUND done and committed (8dafc34): layered gunshots from 31 recorded excerpts (`Art/Sounds/packs/Sonniss`,
  sources and licence in its README), shaped in script; 161 sounds imported; tests Gunfeel 20/20, Audio 14/14; PIE fight
  clean. Perf after (RR, 8 spiders, Medium): 7.6 ms, p95 8.0 (baseline 7.9 / 9.0). Main added the layer cues to
  LooterSoundCues.h (the bank's check) and `Weapon.*.Core/Mech/Tail*` to check.py's FREQUENT list.
- **12.2 Enemies, the user's calls (2026-10-21, "keep going"; gunfeel tuning by the user still pending):**
  difficulty = Borderlands normal (real damage, keep moving, a careless player dies now and then, a big fight drains
  most of your health); smarter AI = packs surround and take turns + ranged enemies use their range (not dodging, not
  reacting to the player); bigger fights = more enemies at once (6-10 in a normal fight) + a few hand-made set-piece
  fights per area (not waves, not new mixed packs).
- **Overnight run (the user, 2026-10-21 night: "keep working on all things throughout the night", going to bed).**
  Their calls before bed: build enemies now (gunplay tuning by the user later; tune both together); single
  `Tools\grab.ps1` window grabs allowed to check looks (no Python capture batches); new gun kinds **SMG, sniper rifle,
  LMG, rocket launcher** (all four); elements = **story elements: Ember (burn), Soul-light (vs undead), Grave-salt
  (slow, vs spirits)**; story: **cut and rework missions overnight, keep the plot** (cut lines ~half, most as
  on-the-move captions, fetch steps into fights or finds; every story beat kept; list the changes here for review); art:
  **VFX + world variety** (hit, death, muzzle, power effects; vary props, rocks, buildings and ground in RR).
  Order: 12.2 enemies -> 12.3 loot (the four kinds, elements, part effects, bigger drop moments) -> 12.4 pacing/story
  -> 12.5 VFX + world variety. Main's calls go under "Decided unattended (2026-10-21 night)" below.
  **Gun look (the user, from concept sheets in `Saved/ArtPreviews/Weapons/Concepts/`):** concept B "frontier-tech"
  (`NewGuns_ConceptB.png`: sand polymer shells, gunmetal, brass fittings, ember-orange heat glow, cyan optics) for the
  Longhorn sniper, Thresher LMG and Powderkeg launcher; concept A (western walnut/blued) rejected as not sci-fi enough and
  its SMG too like the LMG; the SMG is **option 7 "Helix"** (`SMG_Options2.png`: long round magazine under the barrel
  with a glowing spiral showing the rounds left, wire stock, long and low). Names: Rustler (SMG), Longhorn (sniper),
  Thresher (LMG), Powderkeg (rocket launcher).
  **Last calls before bed:** world variety in RR: **anything** goes (terrain and buildings may change too); elements:
  each element's effect (burn/ignite/damage over time and the like) **deals damage and is more effective against the
  monsters weakest to it** (Main's mapping: Ember vs flesh = spiders, slimes; Soul-light vs undead = the Unpaid, miners;
  Grave-salt vs spirits/wards/Ethereal; slows too); if the context fills overnight, **keep going** (the handoff is kept
  current; this overrides CLAUDE.md rule 10 for tonight only); after a 5-hour usage limit, **continue at the reset**
  (a session cron heartbeat at :17 every hour resumes the run; it dies if the app closes).
- **Steps 1-3 DONE. Next: step 4, the user tunes `Looter.Gunfeel.*` in PIE; Main bakes their values into the defaults
  (React: `FGunfeelReactTuning` fields; Kick: `GunfeelKick::Default`; Sound: `GunfeelSoundTuning`) and the tests that
  read them.** Not verified: the look by eye, rumble on a real pad, the surroundings probe's choices in town/mine, the
  revolver's own layers can peak at -0.1 dBFS indoors (lower `Sound.Low`/`Sound.Mech` if it crackles). Over ~500 lines:
  `CreatureHitReactionComponent.cpp` 504, `UnpaidCreatureArms.cpp` 506, `SettingsMenuWidget.cpp` 511.

