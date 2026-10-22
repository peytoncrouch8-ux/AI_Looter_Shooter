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

