# Handoff

Where the work stands; newest first. Rules: `CLAUDE.md`. How-to: `Docs/Pipeline.md`. Older sessions, word for word:
`Docs/Archive/Handoff_history.md` (read only when needed).

## Now (session "Play-through", 2026-10-14, in progress)

**Played** all 16 Ransom's Rest missions in PIE (Main 1-7, every side, Dead Lift's ride, Back Pay's choice, Abel by
`Looter.Boss.Kill`), Keeper's Bridge both ways (clean), the watchtower's six flights (climbable), the manway (walkable),
fast travel (works). New tools: `Tools/Unreal/play.py` (goto/at/find/press/fight/kill/lookat/what/guns/ammo in PIE; `press`
feeds input actions, no window focus) and `Tools\playstep.ps1` (one step: move, keys, focus, missions, screenshot;
`-Button ACCEPT` clicks a game menu's button). `Tools/Unreal/reach_probe.py` (walkability flood fill) is unreliable
inside thick collision hulls: don't trust a NOT reached.

**Bugs found** (fix in this order; ✔ = fixed in code, not built yet):
1. ✔ Knocking Below: the cellar's dead spawned on the yard above (encounter home and spots looked for from 3-5 m up,
   above the vault's collision). Fix in `EncounterGroundProbe` (`FindRoofOver`), wander goals too; test
   `Looter.Encounters.Cellar`. Needs build + tests; DepotWaiting's spots change too (play-check them).
2. Dead Lift blocker: the winch crank on the bunk in the sheriff's cell (1234,1032,120) can't be reached: the player
   stops at the cell doorway, collision fills the cell (SM_FalseFront_Sheriff, `FalseFronts.py` ~1091-1260; placement
   `build_area_missionprops_valley.py` CELL_CRANK/CELL_REPORT). Odell's report likely too.
3. Depot: cliff pieces `Cliff_eastShoulder_23`/`_24` (SM_CliffFace_B) stand inside the depot: the LOST PROPERTY window
   (670,11915) can't be reached, the agent's desk (1182,11845) is inside the rock (The Old Fare's optional), rock shows
   in the baggage room and by the stair. The cliff build must keep out of enterable buildings.
4. Tilly's cellar: the cold room (Thaddeus, -3115,5702,-281) has no ceiling past the workshop's footprint: sky and grass
   seen from inside (terrain is culled from below). `WorkshopBright.py` needs a roof over the cellar's rooms.
5. The player's guns vanished twice (all slots empty, no log line, not on the ground): 05.21-05.22 (Sink, after the egg
   sacs) and 05.55-05.59 (mine drift, at the barricade). Not reproduced on purpose yet; `Saved/watch_guns.py` logs the
   frame they change (WATCHGUNS).
6. Notice boards show "NO REWARD: IT'S PRACTICE" under real postings (Rangers' board, Calder's Post); a click on an
   unselected posting does nothing (only W/S select).
7. Abel's boss bar ("HE KNEELS") stays on the HUD after his defeat, all over the map (seen after `Looter.Boss.Kill`).
8. Side 1: the gate poster's hold-to-tear takes focus over its offer (Read the poster): a tap does nothing, the offer
   shows only once the poster is torn.
9. The orange objective highlight paints Mister Sexton as a solid orange figure (doors get a soft glow).
10. Ledger: the Brown Spider's place line still says the tutorial island.
11. The depot's Unpaid hit the player through the stair door (melee without a line check?).
12. Small: prompts "Take The clamp pin" / "Take The Gravemother's fang"; Side 3's "Find where the Gravemother hungers"
    has no full stop; Supply Crates open on a tap while Calder's Caches says hold; the orders slate's "KESSLER OAK"
    clashes with Barrels Kessler; Knocking Below's slate optional vanishes once the key (beside it) is taken.

## Before (end of session "Ransom's Rest map expansion", 2026-10-13)

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

## Next (in order)

1. **Play it through in PIE** (`Tools\pie.ps1`; never while the user plays): Main 1-7 from their givers; Dead Lift (the
   clamp pin's fitting is 4.2 m up the winch gantry: check it's reachable from the landing; the bottom landing near the
   quarry's back wall), the ride; Calder's Post and The Last Watch; Back Pay in the mine (`Looter.Interior.Status`, sky
   or sun leaking in on Medium, `Looter.Encounter.Wave Foreman force`, the manway gate swinging into the passage,
   `Looter.Story.Flag BackPay.Paid` moves the tags to OUT); Tilly's cellar (key, locked door, stair, fight, Thaddeus);
   the depot (The Old Fare, Fenn's coffin, Lost Property); the roosts; the bridge. Fix what breaks.
2. **Perf** with the editor closed (`Tools\perf.ps1`, `Tools\tour.ps1` on Lvl_RansomsRest): the watchtower view, the
   mine's gallery (13 lights, 26 lanterns), the depot fight (7 unshadowed lamps), the four 4096 macro textures.
3. **Known small gaps:** the ore bin's trestle stands 0.5 m over the ground (the Diggings yard wants a 3.6 m step, has
   3.1); the porch plate is one mesh (the jars stay drawn after they're taken: split SM_PorchPlate_Jars/_Biscuits);
   LookoutStains has no decal; the Haskett "lift" mutter isn't in (needs a TownLife mutter stage after DeadLift);
   notice-board turn-ins (CaldersCaches, LastWatch) play no turn-in lines (`ANoticeBoard::TurnIn`); Ellis's headboard
   isn't a readable (Main 1's optional can't count); Main1 and CaldersCaches data have no porch-plate / strongbox
   interact step (the scripts fall back to the design's); old saves mid Main 3/4/Side 2 land on shifted steps;
   SM_RR_RangerCabin has near-zero tangent warnings; one lone oak on the Bench (meadow layer).
4. **Older open items:** `CreatureBase.h` is ~640 lines (split); fast travel and combat music count a searching
   creature as a fight; Lost Loot prices (only two upgrades affordable in Ransom's Rest, ~1,270 silver a play); third
   person following the FOV slider (the user's call); Story's endgame "levels follow the player" waits for the user;
   Story.md's later areas' level bands (Lily 9-15 ... Toll Gate 47-50) predate the new curve (about 2 levels an area,
   ~30 areas to 65): retune when the next area is designed; the Bell-Ringer and Venom Spitter aren't in Story.md's
   enemy list;
   the user hasn't heard the newest sounds; 5 sounds flagged by the check (Creature.Spitter.Sizzle x3,
   World.Shrine.Rise x2); the slide is a code-drawn pose; from the cloud handoff, Crossroads Town has round 1 built on
   Skyreach (2026-10-08, `Docs/Plan.md` Phase 7), the ember powers and heroes aren't built.

**On hold (the user's calls):** the Gilded Lily; Skyreach's own house kit; no style work unless asked.

## Sessions

- AILS Artwork (a local session) models in Blender and never commits; Main imports and commits.
- The cloud sessions are idle.

## Starting a new session

1. Read this file, `CLAUDE.md`, `Docs/Pipeline.md`.
2. `git status` (clean) and `git log --oneline -15`.
3. Before editor work, check the user isn't playing: `Tools/Unreal/pie_check.py`; no `UnrealEditor.exe ... -game`.
4. Ask the user what comes first, with tap-to-answer choices.
