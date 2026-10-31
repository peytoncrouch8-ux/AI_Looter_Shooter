# Contract: the turn-in card + Landfall (Main 1), round 1 of the Wardens story (2026-10-31 night)

Main (the orchestrator) wrote this; every agent reads it whole before starting, plus `DECISIONS.md` and `TRIPO_MODELS.md`
here, and `CLAUDE.md`. The user's words drive it: "First main mission is strange and has no substance or reason ... main
missions should have a UI that pops up to turn it in if the final step is to talk to someone or turn in to a board. Don't
be afraid to add more to the map, change terrain, and adjust models. Aim for high quality work and make the game exciting
with a reason behind every action the player does." Quality bar: the best big-studio games. Every action rewarding
(CLAUDE.md rule 7: sounds, visual feedback). No copyrighted content.

## Rules for every agent

- Never build, never open Unreal, never run git. Edit only the files your lane owns (below), with the Edit/Write tools only
  (no PowerShell `-replace`, no sed on C++). A needed change in a file another lane owns goes into your README as an exact
  edit (old text, new text) for Main to apply.
- Write `Docs/Work/2026-10-31-Wardens/lanes/<LANE>.md` at the end: files changed/added, what each does, CODEMAP lines (Main
  writes CODEMAP), the exact hooks Main must add in others' files, the console/script run order, tests added (exact names),
  the PIE check Main should do, risks you couldn't check, numbers to tune.
- C++ rules (CLAUDE.md "Code rules"): one class per file, files under ~500 lines (split by topic), tabs, UE naming, comments
  say why in plain words, log to `LogLooter`, no unity build (include what you use; anonymous-namespace names must be unique
  per file). Colours only from `LooterUI::Color`. UI with the `LooterUI` kit. Gameplay in C++, data in assets.
- Every new model is a STAND-IN until the user's Tripo models land (`TRIPO_MODELS.md`): load meshes through one soft path
  per piece (a `UPROPERTY(Config)` or a constant in one place) so swapping is a one-line change, and note the stand-in in
  your README.
- Performance target: 120 fps at 1080p Medium on an RX 580 (8.3 ms; the heaviest views have 0.2-0.5 ms margin): no
  per-frame allocation, no new Niagara, no volumetrics, instanced dressing, distance culling.
- Names: the hero is **Sully Graves** (display only; the internal hero id stays `Ellis`, assets keep their names). "They" for
  Sully. The ship is **the Lamplighter**. Crew: **Captain Mercy Holloway**, **Brother Tobiah Finch** (lantern-keeper),
  **Junie Calloway** (engineer), **Hob** (the crow, lookout, says "sunshine"). Villain: **Cornelius Vane**, chairman of
  **Vane Consolidated** ("the Company"); his men: **Dredgers**. Delia is **Delia Ransom**, Abel's mother (not the player's
  family now). The legendary gun named "Lamplighter" (Game.md) keeps its name for now; Main notes the clash.

## The story in one paragraph

Cornelius Vane's dredge barges suck the dead out of the ground and press them into soul coal for his trains; the runoff,
soul smog, sours the land and stirs the restless dead (the Unpaid). The Lantern Wardens fly the Lamplighter wherever the
dead are taken. A week ago Vane's barge came to Ransom's Rest, hung over the Sink, and the Dunne Gang (Vane's hired
thieves; Lucky Ned runs this barge) tore the soul out of Abel Ransom, Saint Ada's keeper, as he held the chapel door: a
keeper's soul burns longest, so Abel is the barge's pilot light. The Lamplighter's Great Lantern leans toward stolen souls.
Sully Graves, the newest sworn Warden, goes down to bring Abel back to his saint.

## Landfall (Main 1): the mission, step by step (indices are the contract)

Mission id stays **`Main1`** (saves, conditions and tests key on it). Title **"Landfall"**. Automatic, area RansomsRest,
no prerequisites, sort 1. XP 450, purse 20 silver (unchanged). Golden hour; the valley under smog.

Before step 0: a new game plays the planet intro (new words, STORY) and opens Ransom's Rest with the player standing on
the Lamplighter's deck as she sails in from the west over Gravewind Canyon (FLIGHT). The crew talk over the ship's
speaking-tube and from their stations (captions); Hob rides the rigging.

| # | Objective (tracker words: STORY) | Kind and tag (exact) | Owner of the thing | Why the player does it |
|---|---|---|---|---|
| 0 | Go to the Great Lantern at the prow | Reach, tag `Lamplighter_Prow`, radius 350 | FLIGHT (marker actor on the ship) | Finch: the Lantern is leaning hard; see where. Teaches walking. Vane breaks in on the ship's horn as you arrive. |
| 1 | Fight off Vane's boarders | Event `Landfall.DeckClear` (1) | FLIGHT sends it; DREDGER makes the enemies | A Company tender grapples the Lamplighter and stops her; Dredgers board. Teaches shooting, cover, the hose-man. |
| 2 | Cut the grapple lines | Interact (hold) tag `Landfall_Grapple`, count 2 | FLIGHT (grapple actors) | The grapples pin the ship; riflemen on the tender keep firing while you cut. Teaches hold-to-use under fire. |
| 3 | (hidden: the hard landing plays) | Scene objective `HardLanding` | FLIGHT (the scene) | The torn bag: Holloway takes her down hard onto the Mooring Ledge. |
| 4 | Drop the anchor / Fight off the dead | Interact (hold) tag `Landfall_AnchorWinch` (1) + Clear spawner `LedgeClimbers` | FLIGHT (winch), WORLD (spawner) | Without the anchor she slides off the ledge into the canyon; the smog-stirred dead climb up from it. |
| 5 | Climb to Ransom's Point | Reach tag `Place_RansomsPoint` (existing place; height counted as today) | existing | Holloway wants eyes on the valley; the Lantern needs to see Sully from high ground. |
| 6 | Look over the Rest | Interact tag `Landfall_Spyglass` (1) | FLIGHT (the spyglass and its scene `SmogReveal`) | The reveal: the barge over the Sink, its hose in the ground, the smog; Vane's broadcast over the valley's loudspeakers. Then the Lantern finds Abel's trail. |
| 7 | Follow Abel's trail / Optional: Pick up Abel's hat | Reach tag `Speaker_Delia`, radius 1800; optional Interact tag `Landfall_AbelsHat` | WORLD (trail, hat) | The glowing trail runs from the Point down the bluff path to the farm: where the gang dragged his soul out. The hat unlocks Delia's best line. |
| 8 | Help Delia hold the porch | Clear spawner `FarmPorch` | WORLD (spawner), CREW (Delia fights) | Gunfire at the farm: Dredgers and the dead at Delia's steps; she fires from the porch. |
| Turn in | Talk to Delia | speaker tag `Speaker_Delia`, giver "Delia Ransom" | TURNIN (card), CREW (Delia), STORY (lines) | The card first (what you did, rewards, Turn In), then her lines: what the gang did, the hose, where to go next. |

Encounters (CreatureLevels: Ransom's Rest zones, level 1-2 here; the user's rule: levels from the area, never the player):
- Deck boarders (FLIGHT spawns from the tender, using DREDGER's classes): 3 riflemen + 1 hose-man in two waves (2 then 2),
  plus 2 riflemen who stay on the tender's deck and fire across during steps 1-2 (they don't count for DeckClear; they
  fall away with the tender).
- `LedgeClimbers` (WORLD): 4 Unpaid rising from the canyon side of the Mooring Ledge (entrance Rise), active during step 4.
- `FarmPorch` (WORLD): 3 riflemen + 2 hose-men + 3 Unpaid, spawning as the player comes within ~45 m of the farmhouse
  during step 8 (or 7), coming at the porch from the orchard and the farm road.

## Lanes (7 agents) and what each owns

### TURNIN (Opus): the turn-in card, for every mission
Card first, then the talk, for every mission whose turn-in is a talk (speaker point), a board or a mission artifact
(the user: "Every mission", "Card, then talk"). Choice turn-ins keep their choice card (already card-first).
- Owns: `UI/World/MissionOfferWidget*.{h,cpp}` (add `EMissionCardMode::TurnIn`, appended; `FMissionOfferCard::MakeTurnIn`;
  `OpenTurnIn`; a new `MissionOfferWidgetTurnIn.cpp`), `UI/HUD/LooterHUDMissionOffer.cpp`, `UI/HUD/LooterHUD.h` (declare
  `bool OpenTurnInCard(FName MissionId, AActor* Giver)`), `Story/SpeakerPointComponent*.{h,cpp}`, `World/NoticeBoard.{h,cpp}`,
  `UI/World/NoticeBoardWidgetCards.cpp`, `World/MissionArtifact.{h,cpp}`, new `Tests/MissionTurnInCardTests.cpp`, and fixes to
  existing tests that break (list them).
- The card: the mission's name, "Mission complete", "Turn in to <giver> · <place>", **what you did** (each step's main
  objective line, checked, in order; optional ones done marked as a bonus), **rewards** (`DescribeRewards` plus the
  optional silver; a gun reward in its rarity's colour), and **Turn In** (Interact / Enter / click / gamepad A) with the
  ACCEPTED-style stamp, a burst and a sound; **Not now** (Esc / B) closes it and the mission stays ready. After Turn In:
  `Runner->TurnIn`, the card closes, then the giver's `TurnIn.Lines` play as captions in the giver's voice, and only after
  those the follow-up offer opens (today's `QueueOffer(..., bWaitForWords)` path). Without a HUD (tests, headless) keep
  today's direct path so the existing tests pass.
- Tests: `Looter.Missions.TurnInCard.*` (card made from a mission; card first then lines; Not now keeps it ready; board and
  artifact paths; choice missions unchanged; no-HUD fallback).

### DREDGER (Opus): Vane's Dredgers, a new human enemy
- Owns: new `Creatures/Dredger*.{h,cpp}` (e.g. `DredgerCreature`, `DredgerRifleman`, `DredgerHoseMan`, `DredgerRules`,
  `DredgerTuning`, `DredgerHoseComponent`, `DredgerBody` for the animated body), `Dev/DredgerDevCommands.cpp`,
  `Tests/DredgerTests.cpp`, the Dredger pages in `Tools/Unreal/create_bestiary_pages.py` (only add entries).
- Public API (exact): `enum class EDredgerRole : uint8 { Rifleman, HoseMan };` `ADredgerCreature : ACreatureBase` with
  `UPROPERTY(EditAnywhere) EDredgerRole Role`; subclasses `ADredgerRifleman`, `ADredgerHoseMan` (fixed roles; spawners
  and bestiary pages name these); `static TSubclassOf<ADredgerCreature> ADredgerCreature::ClassFor(EDredgerRole Role)`.
- Look: a human body animated by the project's mannequin anims (`/Game/Characters/Mannequins/Anims/Rifle`: ABP_Rifle,
  BS_Rifle_Locomotion, MF_Rifle_Idle_ADS, reload; Unarmed set) the way the player's third-person body is
  (`UBodyFigureComponent`, Docs/Pipeline/Heroes.md "The hero pick", "The third-person hold"), wearing a stand-in mesh (pick
  a hero mesh or the mannequin, darkened to a black rubber coat with a gas-mask-ish tint; one soft path to swap for the
  Tripo Dredger later, which will be rigged to the mannequin skeleton). A stand-in rifle from the game's gun parts (or a
  simple mesh) in the hand; the hose-man a tank on the back (stand-in: an existing barrel/canister prop scaled).
- Fight: riflemen keep 10-25 m, step to cover-like spots (behind obstacles found by traces), peek and fire bursts of real
  bullets the player can see (the enemy projectile/tracer way; `UEnemyProjectileSubsystem` or `FWeaponFX`), reload, call out
  (bark sound), flank in pairs, throw nothing. Hose-men walk in, and at 8 m latch the soul-hose: a visible line from the
  nozzle to the player (code-drawn or cable), the player is pulled slowly and drained (about 5 HP/s at level 1, a cyan-black
  edge on the screen, a hiss), and the line breaks if the player gets 14 m away, shoots the hose-man's tank (a crit spot
  that bursts: a big satisfying pop that staggers him), or staggers him. The tank is the hose-man's weak spot, the mask's
  lenses the rifleman's (crits, 1.5x crit rule). They use the fight director's turns (`EFightRole` Ranged / Melee), the
  rank/tier system, hit reactions (flinch, stagger, kill styles), loot and XP like any creature, and must work standing on
  the Lamplighter's deck while she's grappled (stopped; static collision under them).
- Numbers to start: rifleman 140 HP, 7 per bullet, burst of 3 every 2.2 s, 70% accuracy at 15 m falling off; hose-man
  220 HP, tank 30% of his health as a crit spot; both level 1-2 on Ransom's Rest. Cvars `Looter.Dredger.*`.
- Dev: `Looter.Dredger.Spawn <Rifleman|HoseMan> [count]`. Tests: `Looter.Creatures.Dredger.*` (rules: range keeping, latch,
  break, tank crit; spawn on a flat test floor).

### CREW (Opus): Delia and the Lamplighter's crew as characters
- Owns: new `Story/WardenCrew*.{h,cpp}` (`AWardenCrewMember`), new `Story/DeliaRansom*.{h,cpp}` (`ADeliaRansom`), new
  `Tests/WardenCrewTests.cpp`.
- `enum class EWardenCrew : uint8 { Captain, Keeper, Engineer };` `AWardenCrewMember` (a story character: a human body with
  the mannequin's unarmed idle, like DREDGER's body, or `AStoryCharacter` posed by code: pick the one that looks alive)
  with `UPROPERTY EWardenCrew Crew`, a `USpeakerPointComponent` (names "Captain Holloway", "Brother Finch", "Junie
  Calloway"; tags `Speaker_Holloway`, `Speaker_Finch`, `Speaker_Junie`), station poses: Holloway at the wheel (hands on it,
  looking ahead), Finch at the Great Lantern (hands up to it), Junie at the engine (leaning in, wrench). They brace and
  sway when the ship lurches: `void Brace(float Seconds)`. Stand-in meshes: the heroes' or the mannequin's, tinted per
  role (one soft path each for the Tripo models).
- `ADeliaRansom`: tag `Speaker_Delia`, a `USpeakerPointComponent` named "Delia Ransom" (the turn-in giver), standing on her
  porch with a shotgun (stand-in), idle when calm, and **fighting** while creatures are within 18 m of the porch: she
  shoulders the gun, fires a blast every ~1.8 s at the nearest (muzzle flash, pellets via the game's hit path; about 25%
  of a Dredger's health a hit so the player still does the work), reloads after two, never dies (the dead and Dredgers
  can target her; she shrugs it off and shouts). `void SetFighting(bool)` override for scripts. Barks while fighting
  (STORY's line set `DA_Lines_DeliaFight`, played as queued captions, rate-limited).
- Tests: `Looter.Story.WardenCrew.*` and `Looter.Story.Delia.*` (fights only with creatures near, never dies, talks).

### FLIGHT (Opus): the Lamplighter, the tender, the landing, the opening flow
- Owns: new `World/Lamplighter*.{h,cpp}` (`ALamplighterShip`), new `World/CompanyTender*.{h,cpp}` (`ACompanyTender`), new
  `World/LandfallGrapple*`, `World/LandfallWinch*`, `World/LandfallSpyglass*`, `World/LandfallMarker*` (a tiny tagged
  actor for Reach objectives), new `Scenes/HardLanding*`, `Scenes/SmogReveal*`, new `Story/VaneBroadcast*`
  (`static void UVaneBroadcast::Say(UObject* Context, const TArray<FStoryLine>& Lines)`: the radio-in cue, captions with
  the speaker "Cornelius Vane", the hiss, radio-out), new `Dev/LandfallDevCommands.cpp`, new `Tests/LandfallFlightTests.cpp`,
  and edits to `Scenes/ColdOpenSubsystem.cpp/.h`, `Areas/StationBoard.cpp`, `Session/SessionSubsystemNewGame.cpp`,
  `Session/SessionSubsystemTravel.cpp` (if needed), `Areas/AreaTravelSubsystem.cpp` (if needed), `Tests/ColdOpenTests.cpp`.
- `ALamplighterShip`: the stand-in hull (SM_Skiff_A_Packet scaled up for now, with its own simple walkable deck collision
  about 24 x 7 m and rail walls; one soft path for the Tripo ship), a course (world points set by WORLD's build script:
  `UPROPERTY TArray<FVector> Course`, `int32 GrappleStop`, `FTransform Moored`), states `EShipFlight { Sailing, Grappled,
  Breaking, Landing, Moored }`, smooth kinematic motion with the player riding the deck with full control (moving base),
  gentle bob and creak while sailing. Child pieces: the `Lamplighter_Prow` marker, the crew (CREW's class, attached at their
  stations), the Great Lantern stand-in (a lantern mesh with a flame card leaning toward the farm; `ALanternFlame` maths),
  Hob's perch on the rigging (Hob himself is placed by WORLD/STORY; while sailing he can be hidden).
- The sequence: Sailing from the course start (new game: the player is placed on deck, armed with the Bullpup as today);
  step 0 done at the prow -> Vane broadcast (STORY lines `DA_Lines_LandfallVaneHorn`) -> the tender comes alongside, fires
  two grapples (`ALandfallGrapple`, tag `Landfall_Grapple`, a hold-to-use of ~1.2 s, a visible chain/cable to the tender)
  -> the ship lurches to a stop (Grappled: completely still, so creatures can stand on the deck) -> boarders in two waves
  over a gangplank (DREDGER classes) -> all boarders dead = mission event `Landfall.DeckClear` -> the player cuts both
  grapples (the tender's riflemen fire meanwhile) -> Breaking: the tender falls away smoking into the canyon, the bag is
  torn (a hiss) -> scene `HardLanding` (the player held, the camera shakes, the ship drops and scrapes onto the Mooring
  Ledge, a crash, dust; ~5 s; skippable) -> Moored at `Moored` (stays there for the rest of the game as the crew's ship)
  -> the anchor winch (`ALandfallWinch`, tag `Landfall_AnchorWinch`, hold ~2.5 s with a crank sound; placed on the ledge
  by WORLD or spawned by the ship) secures her.
- `ALandfallSpyglass` (tag `Landfall_Spyglass`; placed by WORLD at the lookout): Interact plays scene `SmogReveal`: the
  camera through the glass at the barge (actor tag `Landfall_Barge`), the hose, the smog; Vane's valley broadcast (STORY
  `DA_Lines_LandfallVaneValley`); Finch on the tube: the Lantern has Abel's trail; back to the player.
- Opening flow: a new game no longer plays the Rest intro, the cold open or the grave wake-up; it plays the planet intro
  (in the menu world, new words from STORY), then opens the level with the player on the deck (the ship is the first
  landing: `StationBoard::FirstArrivalLanding()` returns `Landing_Lamplighter`, a tag the ship carries; placing the pawn
  on a moving deck must base it correctly). `Looter.PlayLevel` / `Tools\pie.ps1 -Map RansomsRest` starts the same way.
  An old save with Main1 running at an old step: the ship is moored and Main1 moves to step 5. Keep the old scene code
  compiling (don't delete it this round), just unreachable for new games; `Looter.Scene.ColdOpen` may stay as a dev command.
- Dev: `Looter.Landfall [step]` (jumps there: ship state, player position), `Looter.Landfall.Ship <state>`.
- Tests: `Looter.Story.Landfall.Flight.*` (course motion, grapple stop/cut, the event, the landing state, old-save move).

### WORLD (Opus): Ransom's Rest's new pieces (build scripts)
- Owns: new `Tools/Unreal/build_area_landfall.py` (hooked into `build_area_story.place()`: give Main the one-line hook),
  `Tools/Unreal/build_area_farm.py` (Delia: `ADeliaRansom` replaces the screen-door speaker point; the porch barricade),
  `Tools/Unreal/build_area_story.py` (remove Ellis's grave and headboard from the family plot and the HeadboardEllis read;
  Abel's grave stays; the hook; Hob's Main1 perches if they live here), `Tools/Unreal/build_area_missionprops_valley.py`
  (the porch plate no longer a Main1 step: STORY says what it becomes), `Art/Levels/RansomsRest/layout.json` (only the
  environment's new `Smog` state and props entries), new `Tools/Unreal/build_landfall_materials.py` if materials are needed
  (soul-smog plume tint, Abel's trail glow, barge stand-in tint), new probe scripts in `Saved/` for Main to run.
- Places: the Lamplighter (`ALamplighterShip`, its `Course` measured with traces: from ~350 m west over Gravewind Canyon,
  low over the plains, into the canyon toward the Mooring Ledge; `GrappleStop` about 120 m short of the ledge over open
  air; `Moored` on the ledge clear of the Mire page, the letter and `Secret_MooringLocker`; coordinates in DECISIONS
  research: ledge about UE (-10100,-10200, z -25), frames: layout.json X north, Y east), the winch on the ledge, the
  spyglass at the lookout (UE (-8600,-10000), deck z ~2660), **Vane's dredge barge** over the Sink (UE (4600,6500); stand-in:
  scaled SM_Skiff_A_Gang pieces, iron-dark, 60+ m long, ~45 m over the rim, tag `Landfall_Barge`, an `AStoryProp`), its hose
  (stand-in cylinders chained down into the Sink's floor), dark smoke plumes from it (SM_SmokePlume, a soul-smog
  material: black-violet with a faint cyan glint), the **Smog** lighting state (thicker, greyer-green fog, dimmer sun,
  lower exposure; a `level.environment.states.Smog` entry) applied by an `AStoryLighting` rule during Main1 through Main5
  (Main6's Dusk rule stays), **Abel's trail** (instanced glowing motes/wisps, SM_GravewindWisp_* in a warm gold-white
  material, from the lookout down the bluff path to the farm porch; an `AStoryProp` shown during Main1 from step 7), Abel's
  hat (`Landfall_AbelsHat`, an existing interactable class or `AStoryProp` + interaction, on the bluff path), the porch
  barricade from existing props, encounter spawners `LedgeClimbers` and `FarmPorch` (classes from DREDGER and the Unpaid;
  active only during their steps), and company loudspeaker poles (stand-in: a pole + horns from existing props) on the Point
  and in town.
- Perf: everything instanced or a few big meshes, culled; Main measures `Tools\perf.ps1` before/after.

### STORY (Opus): the words, the mission data, the rename
- Owns: `Tools/Unreal/create_mission_assets.py` (Main1 -> Landfall exactly per the table; its turn-in; the header doc),
  `Tools/Unreal/create_story_lines.py` (all new line sets), `Tools/Unreal/story_texts.py` (Delia's and Hob's topics and
  perches for Main1, the porch plate's new use), `Tools/Unreal/hero_lines.json` and `Tools/Unreal/create_ember_assets.py`
  (display name "Sully Graves" only), the planet intro's caption table (find where `PlanetCaptions()` lives; edit only the
  words), `Tests/SevenDaysTests.cpp` (rewrite as Landfall's: `Looter.Story.Landfall.Mission`, `.Lines`), `Tests/PacingPaths.h`
  (Main 1's stop), `Tests/MissionTests.cpp` (only Main1's lines if they break), `Docs/Areas/RansomsRest_Expansion.md` Main 1
  section (rewrite), `Docs/Story.md` (a new top section "The Wardens story (2026-10-31)" saying what replaced what; don't
  rewrite the rest yet).
- Line sets (exact names; speaker names exact): `DA_Lines_NewGameIntro` only if its slot count stays the same (else the
  code table only); `DA_Lines_LandfallDeck` (crew on the deck, sailing: Holloway, Finch, Junie, Hob, 4-6 lines),
  `DA_Lines_LandfallVaneHorn` (Vane over the ship's horn at step 0), `DA_Lines_LandfallBoarders` (crew and Hob during the
  fight), `DA_Lines_LandfallGrapples`, `DA_Lines_LandfallLanding` (Holloway taking her down), `DA_Lines_LandfallAnchor`,
  `DA_Lines_LandfallPoint` (Holloway on the tube at the Point), `DA_Lines_LandfallVaneValley` (Vane over the loudspeakers in
  the reveal), `DA_Lines_LandfallTrail` (Finch: the Lantern has him), `DA_Lines_DeliaFight` (barks, 6-8), `DA_Lines_DeliaLandfall`
  (her turn-in lines, 3-5, with a variant if Abel's hat was picked up: say how the variant is chosen), `DA_Lines_HobLandfall*`
  as needed. Voice: Weird West, dry humour (Hob), Vane smug and funny, never cartoonish; Delia hard and grieving; every line
  gives the player a reason or a feeling; short (captions).
- Tests: Landfall's mission asset loads with these steps and tags; every line set exists and is short enough.

### SOUND (Sonnet): the cues
- Owns: `Audio/LooterSoundCues.h` (add the names below), new `Art/Sounds/recipes/landfall.py` and
  `Art/Sounds/recipes/dredger.py`, their registration in `Art/Sounds/recipes/__init__.py`, `Art/Sounds/README.md` (a short
  section). Render only your own cues (`Tools\sounds.ps1 <your patterns>`); never all.
- Cues (constant names in `LooterSoundCue::`, strings as given): `VaneRadioIn` "Story.Vane.RadioIn", `VaneRadioOut`
  "Story.Vane.RadioOut", `VaneRadioHiss` "Story.Vane.RadioHiss" (loop), `ShipEngine` "Ship.Lamplighter.Engine" (loop),
  `ShipCreak` "Ship.Lamplighter.Creak" (loop), `ShipLurch` "Ship.Lamplighter.Lurch", `ShipBagTear` "Ship.Lamplighter.BagTear",
  `ShipHardLanding` "Ship.Lamplighter.HardLanding", `GrappleFire` "Ship.Grapple.Fire", `GrappleHit` "Ship.Grapple.Hit",
  `GrappleStrain` "Ship.Grapple.Strain" (loop), `GrappleCut` "Ship.Grapple.Cut", `TenderFall` "Ship.Tender.Fall",
  `AnchorCrank` "Ship.Anchor.Crank" (loop), `AnchorSet` "Ship.Anchor.Set", `SpyglassOpen` "Story.Spyglass.Open",
  `LanternLean` "Story.Lantern.Lean", `SmogAmbience` "Ambience.Smog" (loop), `DredgerRifleFire` "Creature.Dredger.Rifle.Fire",
  `DredgerRifleReload` "Creature.Dredger.Rifle.Reload", `DredgerBark` "Creature.Dredger.Bark" (muffled through a mask, takes),
  `DredgerHurt` "Creature.Dredger.Hurt", `DredgerDeath` "Creature.Dredger.Death", `DredgerHoseLatch`
  "Creature.Dredger.Hose.Latch", `DredgerHoseDrain` "Creature.Dredger.Hose.Drain" (loop), `DredgerHoseSnap`
  "Creature.Dredger.Hose.Snap", `DredgerTankBurst` "Creature.Dredger.Tank.Burst", `DeliaShotgun` "Story.Delia.Shotgun",
  `DeliaReload` "Story.Delia.Reload", `TurnInStamp` "UI.TurnIn.Stamp".
- Script-made (the user's rule); guns may use the licensed libraries already in the project (ask Main before any new
  download). Loudness-matched like the rest (`level` in the recipe).

## Main's run order (for the READMEs to fit)

1. Apply cross-lane hooks from the READMEs. 2. Build (`Tools\launch.ps1 -Build`). 3. Render and import sounds
(`Tools\sounds.ps1 <patterns>`, `build_sound_bank.py`). 4. `create_story_lines.py`, `create_mission_assets.py`,
`create_ember_assets.py`, `create_bestiary_pages.py`. 5. `build_landfall_materials.py`, then `build_area.py RansomsRest
gameplay` (and environment if the Smog state needs it). 6. Tests by filter. 7. Play Landfall through in PIE from a new
game. 8. Perf before/after. 9. Docs, CODEMAP, commit, push.
