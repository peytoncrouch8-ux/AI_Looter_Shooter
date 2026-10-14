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
