# Code map

One line per file (a `.h`/`.cpp` pair counts as one), grouped by game area: the game module `Source/AI_Looter_Shooter`
first, then the editor-only module `Source/LooterEditor`. Keep it current when you add, move or delete a file. A big
class can spread its `.cpp` over a few files named `ClassTopic.cpp`.

## Module root
- `AI_Looter_Shooter.h/.cpp`: the module, the `LogLooter` log category, and moving Play-In-Editor's Stop key to F10 so
  Escape reaches the game.
- `AI_Looter_Shooter.Build.cs`: module dependencies; builds without unity.

## Core
- `Core/LooterGameMode`: `ALooterGameMode`, the project's default game mode: character, controller and HUD classes,
  loading the session its URL names (`?Session=N`) as the level starts, and the player's start (a trip's landing;
  never a landing otherwise).
- `Core/LooterMenuGameMode`: `ALooterMenuGameMode`, the main menu's game mode (`?game=Menu`, which the game starts
  with): no character, the menu over the level.
- `Core/LooterMenuPlayerController`: `ALooterMenuPlayerController`, the main menu's camera, slowly circling the island.
- `Core/LooterPlayerController`: `ALooterPlayerController`, which adds the always-on input contexts and sets the view's
  look limits.
- `Core/LooterCharacter`: `ALooterCharacter`, the player character: walking, looking and jumping, and its interaction
  component. Its data-only child is `/Game/Player/BP_LooterCharacter`.

## Player
- `Player/PlayerLocomotionComponent`: sprint and crouch, and the first-person motion that goes with them.
- `Player/StanceIntent.h`: `FStanceIntent`, what the sprint and crouch keys ask for (hold or toggle, the latest press wins).
- `Player/PlayerViewComponent`: first/third-person camera (F5 cycle), field of view (the player's first-person setting
  for the world, a fixed one for the gun), recoil on the aim, armed body animation.
- `Player/PawnInputBinding`: `FPawnInputBinding`, a gameplay component's own input component and mapping context.
- `Player/Animation/LooterCharacterAnimInstance`: parent of the character's Anim Blueprints; layers the procedural stance
  pose and holds guns in the loadout stand-in's hands.

## Combat
- `Combat/HealthComponent`: `UHealthComponent`, health, damage events, floating damage numbers, and what dealt the
  latest damage (the kill weapon).
- `Combat/CombatRules.h`: `LooterCombat`, game-wide rules (critical hit multiplier, damage variance).
- `Combat/CriticalSpotTarget.h`: `ICriticalSpotTarget`, targets that have a critical spot.
- `Combat/LooterDamageTypes.h`: weapon, critical-hit and creature-attack damage types.
- `Combat/BulletSubsystem`: `UBulletSubsystem`, every bullet in flight: travel, hits, damage, impact effects.
- `Combat/PlayerVitalsSubsystem`: red flash when hurt; fade out and respawn on death at the open respawn grave nearest
  where the player fell (`ARespawnMarker`), else the level's own start (never a trip's landing).
- `Combat/TargetDummy`: `ATargetDummy`, a training dummy that takes hits and flashes.
- `Combat/EnemyProjectileSubsystem`: `UEnemyProjectileSubsystem`, pellets creatures fire (a boss's spectral buckshot) as
  plain data: volleys and their tell, flight, hits on players' capsules; `EnemyProjectileSubsystemDraw.cpp` draws them on
  instanced meshes.
- `Combat/EnemyShotDamageType.h`: `UEnemyShotDamageType`, a creature's pellet (still a creature's attack).
- `Combat/MovementSlowComponent`: `UMovementSlowComponent`, a character's slows (a Gravebound Unpaid's shriek): the
  strongest share and the longest time win; it holds the walking speeds down after whatever sets them each frame (the
  player's sprint and aim), then gives them back.

## Weapons
- `Weapons/WeaponBase.cpp`: `AWeaponBase` construction, lifecycle, equip and holster, loot state and looks.
- `Weapons/WeaponBaseFiring.cpp`: `AWeaponBase` firing (fire modes, shots, aim point) and the muzzle flash.
- `Weapons/WeaponBaseReload.cpp`: `AWeaponBase` magazine and reload (progress, the moving magazine or pump).
- `Weapons/WeaponBase.h`: the weapon actor's declaration.
- `Weapons/WeaponDefinition`: `UWeaponDefinition`, the data asset for one kind of gun (stats, rarity table, parts,
  looks).
- `Weapons/WeaponTypes.h`: `EWeaponKind`, `EWeaponReloadPart`, `FWeaponStats`, `FWeaponRarityInfo`,
  `FWeaponInstanceData` (a rolled gun, or a named one: `Named`).
- `Weapons/NamedWeaponDefinition`: `UNamedWeaponDefinition`, a named gun as a data asset in `/Game/Data/Weapons`
  (`DA_Named_<Id>`, made by `Tools/Unreal/create_named_weapons.py`: Heirloom): its kind, fixed rarity and parts, its own
  name and flavor line, its stats at a fixed quality, its wear and seed; `MakeInstance` makes the gun at a level,
  `FindProblems` checks its parts.
- `Weapons/AmmoTypes`: `EAmmoType` and `LooterAmmo`, ammo classes, carry limits and box sizes.
- `Weapons/WeaponParts`: `FWeaponPartSlot` and `FWeaponPaint` (a gun's part and color options; each part carries stat
  changes and a name word, the game's affixes) and `WeaponParts::Pick`, which picks a rolled gun's parts and colors by
  its seed.
- `Weapons/WeaponModelComponent`: `UWeaponModelComponent`, a rolled gun assembled from its parts (Blender meshes from
  `Art/Models/Weapons`), painted, with its muzzle, grips and moving reload part.
- `Weapons/WeaponRecoil`: `FWeaponRecoil` and `FWeaponRecoilProfile`, spring recoil on the gun and the aim.
- `Weapons/ReloadMotion`: `LooterReload`, the choreography of a reload over its progress.
- `Weapons/WeaponFX`: `FWeaponFX`, code-drawn tracers, impact sparks, dust and chips; a scene's gunfire without a gun
  model (`SpawnFlash`) and grave dirt (`SpawnDirt`).

## Inventory
- `Inventory/WeaponManagerComponent.h`: `UWeaponManagerComponent`, the player's weapons, backpack and ammo.
- `Inventory/WeaponManagerComponent.cpp`: its lifecycle, ammo pools, firing passthrough and input.
- `Inventory/WeaponManagerSlots.cpp`: slots and backpack (give, equip, drop, stash, swap, move) and where guns are held.
- `Inventory/WeaponManagerPickups.cpp`: the loot it offers the player's interaction component (`IInteractionSource`),
  picking it up (a tap of the interact key) or equipping it in place of the gun in hand (a hold), and the loot labels.
- `Inventory/WeaponManagerSave.cpp`: what the player carries into a saved session and back, and emptying it.
- `Inventory/WeaponInventorySave.h`: `FWeaponInventorySave`, the guns, backpack and ammo as a session saves them.

## Interaction
- `Interaction/Interactable.h`: `IInteractable`, something the player uses with the Interact key (a door, a headboard,
  the bell, a lantern post, the skiff's gangplank): its prompt, tap or hold, whether it can be used now, and the use,
  and whether it's used up for good (a poster torn down).
- `Interaction/InteractionTypes.h`: `FInteractionOptions` (how the key uses a thing now: a tap, a hold and its time, the
  prompt's words), `FInteractionView` and `FInteractionCandidate` (what the component looks around with).
- `Interaction/InteractionComponent`: `UInteractionComponent`, the player's Interact key for everything it uses: the
  key's tap and hold, the use and its mission event; `InteractionComponentFocus.cpp` finds what the player means to use
  (the crosshair's line, else a forgiving cone in reach and in sight).
- `Interaction/InteractionFocus`: `InteractionFocus`, choosing among the candidates (the most directly looked at, nearer
  winning near-ties), apart from the world so the tests feed it candidates.
- `Interaction/InteractionSource.h`: `IInteractionSource`, a player component that offers things by its own rules (the
  weapon manager offers loot: a tap picks it up, a hold equips it).
- `Interaction/InteractionSubsystem`: `UInteractionSubsystem`, the level's interactables (each joins as its play
  begins), so the component looks among a few dozen.
- `Interaction/InteractableProp`: `AInteractableProp`, the greybox interactable for tests and greybox levels (a door
  that swings, a bell held to ring, a lantern post held to light), tagged for missions to find.

## Affixes
- `Affixes/WeaponRollLibrary`: `UWeaponRollLibrary`, rolling rarity and stats from a seed (a named gun's at its fixed
  quality: `ComputeInstanceStats`), and spawning rolled guns.

## Loot
- `Loot/LootTable`: `ULootTable`, what a kill drops (ammo pickups of 18-36 rounds and their lean toward the kill
  weapon's ammo, weapon odds, luck) and `LooterLoot`, the ammo amounts (a chest's fixed 36).
- `Loot/LootLibrary`: `ULootLibrary`, rolling a loot table (`RollAmmo` then `RollWeaponPicks`, as a kill draws them)
  and spawning the results.
- `Loot/LootOdds`: `LootOdds`, a loot table's odds worked out exactly and counted over many kills from a seed
  (`Looter.Loot.SimulateDrops`, the `Looter.Loot.RankOdds` test).
- `Loot/LootDropComponent`: drops its owner's loot when it dies (only ammo in a practice area the player has left).
- `Loot/LootTossComponent`: `ULootTossComponent`, throws loot so it pops out, lands and settles.
- `Loot/WeaponRack`: `AWeaponRack`, a rack with a weapon lying on it as loot and ammo beside it; restocks when the
  weapon is gone and the player has none (the tutorial's first rifle).
- `Loot/AmmoPickup`: `AAmmoPickup`, dropped ammo: a spinning bundle of rounds, the ammo's icon modeled in 3D (per type,
  from `Art/Models/Loot/Ammo.py`) under a small white beam, collected by running past it (within `CollectRadius`).

## Creatures
- `Creatures/CreatureBase`: `ACreatureBase`, a hostile creature's brain and life cycle (senses, chase, attack, death,
  respawn, a pack turning on its attacker). `CreatureBaseSteering.cpp` is its steering without a navmesh (obstacle and
  ledge probes, wander goals, the ground); `CreatureBaseRank.cpp` its rank, its level (from its area's band, with health and damage growing with it) and
  size (`BodyScale`), which
  creatures come back after a death, pack tags, spawning creatures in play (`SpawnAtRuntime`) and what a boss fight
  asks of one (held back, put home);
  `CreatureBaseUpdateRate.cpp` slows the ones far from the player or out of sight (`Looter.Creatures.UpdateRates 0`
  turns that off); `CreatureBaseHunting.cpp` says whom it hunts: a living player on its hunting ground
  (`HuntingGround`) and out of every safe zone that's on. An attack can start farther out or hold its aim
  (`GetAttackStartRange`, `TracksTargetInWindup`), and a named Legendary shows its name on its tag (`bNameIsRankWord`).
- `Creatures/CreatureRank.h`: `ECreatureRank`, a creature's rank (Basic, Rare "Restless", Epic "Gravebound", Legendary
  "Soulfed", Boss).
- `Creatures/CreatureRankSettings`: `UCreatureRankSettings` and `FCreatureRankInfo`, what each rank does (its tag's
  word and color, size, health, damage, experience and levels, loot table, pack call, whether it comes back), in
  Project Settings > Game > Creature Ranks. The rank loot tables (`DA_LootTable_Rare`, `_Epic`, `_Legendary`, `_Boss`)
  are made by `Tools/Unreal/create_rank_assets.py`.
- `Creatures/CreatureUpdateRate`: `FCreatureUpdateRate`, how often a creature updates by its distance and whether it
  is in view (zoom through a sight counts as nearer).
- `Creatures/SpiderCreature`: `ASpiderCreature`, the brown spider: SK_Spider (from `Art/Models/Creatures/Spider.py`)
  posed by code (stepping gait, leg IK, attack and death motion) at any size; its physics asset holds the hit zones.
  `SpiderCreatureRig.cpp` reads its rig from the skeleton (the bones it moves, the legs' layout) and builds the leg
  segments' frames.
- `Creatures/GravemotherCreature`: `AGravemotherCreature`, the Gravemother (Side 3): the brown spider at 1.8x in a pale
  hide (MI_SpiderBody_Pale), Legendary with her own name on her tag, crits on the head and abdomen.
  `GravemotherCreatureCharge.cpp` is her charge (a long telegraph: she tracks, then holds her aim while the ground
  cracks along her line; a straight dash that runs down whoever is in it; a slam and a burst);
  `GravemotherCreatureBrood.cpp` her brood (four spiderlings at 66% and 33%, each once a life; a spiderling is a brown
  spider at 0.45x with a fifth of its health, `SpawnSpiderling`).
- `Creatures/GroundCrack`: `AGroundCrack`, a charge's crack in the ground: a jagged fissure with a glowing seam laid on
  the ground, opened along its line, burst round a point, closing and gone by itself.
- `Creatures/SlimeCreature`: `ASlimeCreature`, the meadow slime: SK_Slime (from `Art/Models/Creatures/Slime.py`) that
  only hops, squashing and stretching on springs, with a leap attack and crits through the gel at its core.
- `Creatures/UnpaidCreature`: `AUnpaidCreature`, the Unpaid: SK_Unpaid (from `Art/Models/Creatures/Unpaid.py`; its
  bones are named in `Rig`, settable in `DefaultGame.ini`) floating and posed by code, with its hat (SM_UnpaidHat) on
  the hat bone; 160 health and 8 damage. It crits on its coal by the shot's line from the front, shows its rank in its
  coal and ember edge (custom primitive data, `UnpaidLook`, read by `M_Ghost`) and wears one of three sets of clothes.
  `UnpaidCreatureAttack.cpp`: the shriek (a Gravebound one's slowing ring) and the lunge; `UnpaidCreaturePhase.cpp`:
  the phase-step (stuck or far behind, it fades out and comes back 3-5 m nearer); `UnpaidCreatureRig.cpp`: its rig and
  pose (hover, lean, look, jaw, arms and fingers, the shroud's chains). A body built on its rig (Abel) adds bones, lays its own pose over the
  rig's and its look over its props (`LookParts`); `RiseIn` for a boss's adds.
- `Creatures/UnpaidRules`: `UnpaidRules` and `FPhaseStepRules`, the Unpaid's rules as plain functions: rank traits, the
  coal shot, when a phase-step comes and where it lands.
- `Creatures/ShroudChain`: `FShroudChain` and `FShroudChainSettings`, the shroud and side strips as damped chains: they
  trail its motion, ripple with speed, sway at rest and snap straight on a lunge.
- `Creatures/ShriekRing`: `AShriekRing`, a Gravebound shriek's glowing ring: it runs out over the ground and slows the
  player as it passes them.
- `Creatures/CreaturePoseAnimInstance`: `UCreaturePoseAnimInstance`, applies the pose a creature's code works out
  (`ACreatureBase::GetBonePose`) to its skeleton.
- `Creatures/HuntingGround`: `FHuntingGround`, where a creature fights: a radius or a polygon (a fence's line) with a
  height band; it hunts only players on it and gives up when they leave it.
- `Creatures/EncounterGroup.h`: `FEncounterGroup` and `EEncounterRankRoll`, one kind of creature a spawner brings:
  class, count, rank roll, level, size, health, the waves it joins, its kind's cap.
- `Creatures/EncounterRules`: `EncounterRules`, a spawner's rules as plain functions: candidate spots, spots on its
  level of ground, room under a cap, ranks, waves.
- `Creatures/EncounterSettings`: `UEncounterSettings`, Project Settings > Game > Encounters: 16 creatures within 80 m of
  the player, the Unpaid's 12, nothing spawning within 8 m of the player.
- `Creatures/EncounterSubsystem`: `UEncounterSubsystem`, the level's spawners, safe zones and creatures: cap counts
  without actor iteration, story refresh on mission changes, encounter events (`SendEvent`, and the missions' events
  that some spawner waits for), safe-zone queries.
- `Creatures/EncounterSpawner`: `AEncounterSpawner`, an encounter: groups and waves spawned when the player comes near,
  switched by the story, kept to the caps and its hunting ground, taken away while the player is far; nothing it
  spawns comes back once killed. `EncounterSpawnerWaves.cpp`: its waves, spots, spawning and taking away. A
  Legendary monster's lair (`LegendaryId`): its death kept in the session, the encounter cleared on every arrival until
  20 minutes of play have passed.

## Bosses
- `Bosses/BossComponent`: `UBossComponent`, makes a creature a boss: the fight around it (started by a hit, a player near
  its spot or `StartFight`; until then it waits, hunting nobody), its phases, the player's death starting it over, its
  death winning it. `BossComponentPhases.cpp` runs the phases' events (untargetable spells, volleys),
  `BossComponentAdds.cpp` the waves of adds, `BossComponentArena.cpp` the fog wall and the bar.
- `Bosses/BossTypes.h`: a boss fight as data: `FBossPhase` and its events (`FBossPhaseEvent`: `FBossAddWave`,
  `FBossUntargetable`, `FBossVolley`, custom moments; `bAroundSpot`: adds rising round the arena's middle).
- `Bosses/BossRules`: `BossRules`, the fight's rules as plain functions (the phase for a share of health, how many adds a
  wave raises, when a spell ends, where adds rise).
- `Bosses/BossSeal`: `ABossSeal`, a fight's fog wall (a ring round the boss's spot, or a placed gate): it stops walking
  pawns only, drawn as rising ghost-light.
- `Bosses/BossTestSpider`: `BossTestSpider`, the test boss (`Looter.Boss.Test` and the boss tests): a big Boss-rank
  spider with a brood at 50%, venom volleys from 25% and a 15 m wall.
- `Bosses/AbelKeeper`: `AAbelKeeper`, Abel Ransom, the Keeper (Main 6's boss): SK_Abel on the Unpaid's rig at 1.3 with
  his hat, ghost lantern (one shadowless light) and spectral pump, Boss rank; his fight is his `UBossComponent`'s
  (`AbelRules::MakePhases`). On the deck only during Main 6, dormant otherwise. `AbelKeeperFight.cpp`: the fight's
  moments (phase lines, buckshot after the lantern's flare, grieving with the coal open, the bell and the drift into the
  fog, the lanterns dragging him back, adds, the kneel); `AbelKeeperMoves.cpp`: each moment frame by frame, and the
  scene's hand-off to his board; `AbelKeeperWind.cpp`: the Gravewind (gusts toward the open end, the downdraft past it,
  Hob's word on a fall, the wisps); `AbelKeeperPose.cpp`: his pose table laid over the rig.
- `Bosses/AbelPoses`: `AbelPoses`, Abel's pose table (idle, flare, fire, lunge, sunset, kneel, sit) solved for his
  skeleton. `AbelPoseData.inl` is the table, generated by `Tools/abel_poses.py` from the art's
  `Intermediate/AbelModel/Abel_poses.json` (don't edit it).
- `Bosses/AbelRules`: `AbelRules`, Abel's fight as plain rules: his phases (60% the bell, 25% the Gravewind), the
  buckshot volley, the gusts and the fall past the open end, his and Hob's lines.

## Bestiary
- `Bestiary/BestiaryEntry`: `UBestiaryEntry`, `EBestiaryCategory` and `EBestiaryPage`, one bestiary page as a data asset
  (in `/Game/Data/Bestiary`): words and stand model (skinned, or a still one for a seated figure with still parts on
  its sockets: Sexton and his ledger on a length of the lookout's rail), with level, health, attack and experience read
  from its actor class (at the class's own rank: the Gravemother's Legendary), and what the actor wears on its bones (a
  hat) for the stand. Its page
  type: an actor met in the world, a story character with no actor (open once its story condition holds), or one of the
  Ledger's seven names (whereabouts blank until found); pages written in the Ledger only. `Tools/Unreal/create_bestiary_pages.py` writes pages from data (the
  Unpaid's and the Gravemother's; Hob's, Sexton's, Delia's, Tilly's, Aldana's and Ruth's; the seven names).
- `Bestiary/Ledger`: `Ledger`, the bestiary as Sexton's Ledger from Main 2's "Open the Ledger" step on (read from the
  campaign record), its name on the inventory's tab and key hints, and its words in his voice.

## Tutorial
- `Tutorial/TutorialDirector`: `ATutorialDirector`, the tutorial island's steps (move, reach the village, take the rifle,
  shoot the dummies, hunt spiders, open the loadout) as a data mission (`DA_Mission_Tutorial`) played by the mission
  runner: it starts it, shows each step's prompt, keeps the done flag and saved step; `TutorialDirectorMission.cpp`
  makes the built-in steps into the same mission when the asset is missing; `Looter.Tutorial restart|skip`.

## Missions
- `Missions/MissionSubsystem`: `UMissionSubsystem`, the missions going on in the world (title, objective, waypoint) and
  which one is tracked, the one the minimap's compass arrow points to, fed by `UMissionRunner`; `FMissionBook` is its
  bookkeeping.
- `Missions/MissionDefinition`: `UMissionDefinition`, one mission as a data asset in `/Game/Data/Missions`
  (`DA_Mission_<Id>`, the first made by `Tools/Unreal/create_mission_assets.py`): id, words, main/side/tutorial,
  prerequisites, area, what starts it, steps of instanced objectives (`FMissionStep`) and rewards (`FMissionRewards`).
- `Missions/MissionObjective`: `UMissionObjective`, the base of the objective kinds (a rule; the runner keeps each one's
  progress in `FMissionObjectiveState`), with `FMissionContext`, `FMissionEvent` (Interact, Talk, Collect, Scene.X,
  Board.X) and the waypoint setting.
- `Missions/MissionPlaceObjectives`: reach a place, travel a distance, defend for a time.
- `Missions/MissionCombatObjectives`: kill N (class, tag, zone), kill a named actor, hit N, clear an encounter
  (`UMissionClearObjective`: done once a spawner is cleared, its count read from the spawner, so kills before the step
  began count too: Main 2's nest, Main 3's gate, Main 4's chapel yard).
- `Missions/MissionEventObjectives`: event, interact or hold, talk at a speaker point, play a scene (done too when it
  played before the step began), board.
- `Missions/MissionLastingInteractObjective`: `UMissionLastingInteractObjective`, Interact whose count lives in the
  world: what stays used (`IInteractable::IsUsedUp`, a poster torn down) counts whenever it looks, so a side mission
  started over after a reload, or opened late, loses nothing.
- `Missions/MissionPlayerObjectives`: collect (guns carried, items picked up), open an inventory page.
- `Missions/MissionTargets`: `FMissionActorFilter`, `FMissionPlace` and `MissionTargets` (finding an objective's actors:
  nearest living, middle).
- `Missions/MissionRunner`: `UMissionRunner`, each level's mission runner (due missions, steps, objectives, display,
  campaign record). `MissionRunnerFlow.cpp` starts, steps, finishes and rewards; `MissionRunnerEvents.cpp` handles
  deaths and hits (spawned actors too), events (passed on as `OnEvent` once the missions have heard them) and the
  display.
- `Missions/MissionActorWatch`: `UMissionActorWatch`, one actor's deaths and hits passed to the runner.
- `Missions/MissionRewards`: `MissionRewards`, the experience share, the reward gun with its rarity floor, the named
  gun (`DropNamedGun`), rewards in words.
- `Missions/MissionText`: `MissionText::ResolveKeys`, `{Action}` as the player's bound key.

## Story
- `Story/StoryLine`: `FStoryLine`, one line said aloud: who says it, the words and its seconds (without seconds, a
  reading time for its words).
- `Story/StoryLineSet.h`: `UStoryLineSet`, lines said together as a data asset (`DA_Lines_<Name>` in `/Game/Data/Story`,
  made by `Tools/Unreal/create_story_lines.py`: Delia's, the headboards', Hob's, Sexton's deal, Tilly's, Aldana's,
  Abel's).
- `Story/StoryCondition`: `FStoryCondition`, when something of the story applies (after missions, before others, while
  one is played, from one of its steps and before a later one), read from the campaign record.
- `Story/CaptionQueue`: `FCaptionQueue`, the captions' rules apart from the world: lines one at a time in order, each
  for its seconds with its fades; a conversation that interrupts cuts the line on screen short.
- `Story/CaptionSubsystem`: `UCaptionSubsystem`, the level's captions: plays lines (interrupting, or after what's
  queued), logs each as it comes on, waits while a menu covers the game.
- `Story/SpeakerPointComponent`: `USpeakerPointComponent` and `FSpeakerTopic`, where someone talks to the player (from up
  to 4 m): a Talk tap plays their lines for this point in the story as captions and sends the missions a Talk event
  (and the topic's own event).
- `Story/SpeakerPoint`: `ASpeakerPoint`, someone talking through a door or window (Delia's screen door, Tilly's window):
  a speaker point, and a leaf of its own if needed that never opens.
- `Story/StoryCharacter`: `AStoryCharacter`, a non-hostile character of the story: a placeholder body posed by code
  (breathing, turning to whoever it talks to), a speaker point, shown or hidden by story state.
- `Story/HobBird`: `AHobBird`, Hob, the one-eyed crow: SK_Hob posed by code (plain shapes without his model) on fixed
  perches chosen by story condition, flying in when the story moves him (as long as the way needs at his speed, higher
  over a longer one) and saying his piece as he lands, talked to as a story character; `HobBirdRig.cpp` his rig (breathing, the head's small sudden steps, a ruffle now and then, the wings
  blended open from Hob.py's table and beating about the shoulder in flight).
- `Story/MisterSexton`: `AMisterSexton`, Mister Sexton on the lookout's rail (Main 2): SM_MisterSexton seated on the
  Lookout's Sit socket with SM_SextonLedger on his Ledger socket and his captions from his Speaker socket; a story
  character shown by its condition that never turns or breathes, solid to the player and the Interact line but not to
  shots; once the story is done with him he goes the next time nobody is looking or listening.
- `Story/GraveSightFlash`: `FGraveSightFlash`, a Grave Sight flash's rules apart from the world: its seconds (two by
  default), the overlay in quickly, held and out more slowly, and an ember seen through it rising, flaring and fading.
- `Story/GraveSightSubsystem`: `UGraveSightSubsystem`, Grave Sight on the local player's screen: for now its flash (one at
  a time, a new one starts over), shown on `UHudGraveSightWidget`, made the first time one plays; unseen without a local
  player (tests).
- `Story/AbelOnBoard`: `AAbelOnBoard`, Pa on his board after Main 6: SK_Abel posed once in the sit (a poseable mesh)
  with his props and his coal sunk to an ember; a story character (topics from `create_story_lines.py`) who turns his
  head and moves his jaw while he talks.

## Scenes
- `Scenes/SceneTimeline`: `FSceneTimeline`, a scene as a timeline with nothing in Sequencer: moves over spans of it,
  named moments that happen once, in order, and waits that hold the clock until a condition holds; a skip passes every
  wait, puts every move at its end and fires the moments still to come (`IsSkipping` while it does).
- `Scenes/SceneSubsystem`: `USceneSubsystem`, the level's scenes one at a time (`FScenePlay`, with its own frame and keys
  when it needs them: `OnTick`, `BindKeys`): the first cast-off's skiff ride (`PlaySkiffRide`), skipping
  (`Looter.Scene.Skip`, holding Interact, Escape twice), moments as `OnSceneEvent`, `Scene.<Name>` to the missions and
  which scenes have played here (`HasPlayed`, `MarkPlayed`), scenes off in tour and perf runs (`Looter.Scenes`,
  `-NoScenes`); `SceneSubsystemPlayer.cpp` holds the player meanwhile (the HUD put away, carried on the skiff or stood
  where a scene wants them, hidden, the scene's camera and the view handed back) and gives them back;
  `SceneSubsystemKeys.cpp` their keys meanwhile (look only, the scene's own, over every other key) and the skip keys.
- `Scenes/SkiffRide`: `SkiffRide`, the first cast-off: the skiff's course (easing out along its bow, a slow climb, a
  gentle turn to starboard and a bob), the white over its last 2.5 s, and the ride as a scene.
- `Scenes/ColdOpenSubsystem`: `UColdOpenSubsystem`, the story's opening on its first area: as the level begins it plays
  the cold open, then the grave wake-up, while they're due (the first cast-off made, the cold open not yet seen), records
  them seen at the end (`bColdOpenSeen`), and passes them where they can't play (scenes off, a level played without the
  story begun) so Main 1 goes on; `Looter.Scene.ColdOpen`, `.GraveWake` and `.Claw` play them by hand.
- `Scenes/ColdOpen`: `ColdOpen`, the cold open as a scene: dusk behind the arrival's white, REVENANT rising through it on
  the gang's skiff gliding out of the evening cloud, down Gravewind Canyon toward the Mooring Ledge ("Home, kid.";
  "Ransom's Rest. Seven days ago."); then dusk on Ransom's Point through Ellis's eyes (the ember, the bell, "El!", Ned's
  shot and Abel's fall, the Deacon's, the sky turning over onto Sexton on the far rail) and black. Captions, sounds when
  the set has them, skippable. `ColdOpenPoint.cpp` is the dusk on the Point; `ColdOpenBeats.h` what the two halves share
  (the scene's state, its lines, the view, captions, sounds and fades).
- `Scenes/ColdOpenCourse`: `FColdOpenCourse`, the gang's skiff's course as plain math: a smooth curve through the set's
  points, travelled at the packet skiff's drift out of the cloud, picking up over the plains, easing off into the canyon,
  leaning into its turns and heaving.
- `Scenes/ColdOpenSet`: `AColdOpenSet`, the cold open's marks and looks as data in the level, standing where the lookout
  stands (`Tools/Unreal/build_area_story.py`): the skiff's course in the world's frame; Ellis's views, the gang, Abel's
  run and Sexton's seat in the lookout's; the models it draws and its sounds (none made yet).
- `Scenes/ColdOpenCast`: `AColdOpenCast`, what the cold open shows that the level doesn't have: the gang's skiff in its
  dark paint with the gang on its deck, the gang again on the lookout's deck, the ember and Abel's lantern as glows with
  little lights, the muzzle flashes; `ColdOpenCastFigures.cpp` the figures (the UE mannequin posed by code, drawn flat
  black with `M_Backdrop`) and Sexton (SM_MisterSexton and his ledger on the lookout's Sit socket, black; plain shapes
  where he isn't imported).
- `Scenes/GraveWake`: `FGraveClawOut`, clawing out of the grave as rules (three Jump presses, each rising a third of the
  way and letting the light in) and `GraveWake`, the grave wake-up as a scene: in Ellis's coffin in the dark, the
  timeline waiting for the claws (grave dirt bursting, the prompt's pips), the climb out to the grave's foot facing the
  headboard, the view handed back to the player standing there.
- `Scenes/GraveClawPromptWidget`: `UGraveClawPromptWidget`, the wake-up's "PRESS [SPACE BAR] TO CLAW OUT" over three
  slanted pips that light as the player claws, floating outlined text with no panel.
- `Scenes/SitWithPa`: `SitWithPa`, the scene after Abel's fight: he speaks, rises, lights the Keeper's Lantern on the
  keeper's post and sits on his board facing the sunset; Hob speaks, and the player is handed back facing him.
- `Scenes/SceneCloudBank`: `ASceneCloudBank`, soft cloud for a scene to sail into (or out of, tinted for the evening):
  the game's smoke puff on a dozen camera-facing quads in one draw, thinning near the camera
  (`Looter.Scene.CloudBrightness`).
- `Scenes/SceneSkipPromptWidget`: the skip prompt in the bottom right during a scene ("HOLD [E] TO SKIP" over a filling
  bar, "PRESS [ESC] AGAIN TO SKIP").
- `Scenes/TransitionScreen`: `FTransitionScreen`, the transition screen's rules apart from the screen: a scene's white,
  fades, holds (thinning by themselves after 20 s), reveals (the title rising, then the white thinning).
- `Scenes/TransitionScreenSubsystem`: `UTransitionScreenSubsystem`, the white over the whole game window, owned by the
  game instance so it holds through level loads, and the REVENANT title card rising through it.

## Progression
- `Progression/XPCurve`: `FXPCurve`, the experience each level takes (exponential), level-ups from a gain, and the
  maximum level.
- `Progression/ProgressionSettings`: `UProgressionSettings`, the curve's and the level rules' numbers in Project
  Settings > Game > Progression (`DefaultGame.ini`).
- `Progression/LevelRules`: `FLevelRules`, what a level is worth: enemy health and damage growing 8% of their level 1
  values a level, kill experience (8% more a level, 15 points less for each level below the player, at least 10%) and
  the player's +8% health a level.
- `Progression/PlayerProgressData.h`: `FPlayerProgressData`, the player's level, experience, tutorial, kinds met and
  defeat counts, as a session saves them.
- `Progression/LooterProgressSave.h`: `ULooterProgressSave`, the one progress save from before sessions ("PlayerProgress"
  slot), read once to become session 1.
- `Progression/PlayerProgressionSubsystem`: `UPlayerProgressionSubsystem`, the player's level and experience (adding,
  level-up events), the experience a kill gives (the creature's level and the falloff), the player's health for their
  level (the first level reward), and which kinds the player has met and how many defeated (by exact class: a
  Blueprint child counts for its parent's page, a C++ child such as the Gravemother is its own); the session being
  played gives it its progress and saves it. A practice area's kills give no experience.

## Session
- `Session/SessionSubsystem.h`, `Session/SessionSubsystem.cpp`: `USessionSubsystem`, the three save sessions: the main
  menu's summaries, playing, deleting, reading saves (brought up to date), autosaves and save holds, Save & Quit, and
  carrying the old progress save into session 1.
- `Session/SessionSubsystemWorld.cpp`: what a session keeps of the player and each map's world (where the player
  stands, health, guns, loot on the ground, the gun racks, the wanted posters torn down, the tutorial's step) and
  putting it back.
- `Session/SessionSubsystemTravel.cpp`: travel between maps (`TravelToArea`, `TravelToMap`): the world left kept under
  its map, the trip's save, the destination opened with the session, and arriving at the trip's landing.
- `Session/SessionSubsystemSkip.cpp`: "Skip the tutorial": a new game counting as the first cast-off, a Common Bullpup
  in hand, opening on the story's first arrival behind the white.
- `Session/SessionSubsystemWords.cpp`: the session picker's words: play time, when saved, and places by area name or
  level file.
- `Session/SessionSave`: `ULooterSessionSave`, one session's save ("Session1" to "Session3"), version 2: the player, a
  world per map (`FSavedMapWorld`), the campaign record and where a trip arrives; the version 1 upgrade and a trip's
  bookkeeping.
- `Session/CampaignRecord.h`: `FCampaignRecord`, the story so far: missions finished and the one being played, areas
  opened, bosses beaten, respawn graves opened, the first cast-off, the cold open.
- `Session/SessionSaveGate`: `FSessionSaveGate`, when the session may save: nothing from a trip's save until its
  destination begins; autosaves held during rides and fades.
- `Session/SessionSubsystemPromotions.cpp`: when a map's creatures are promoted on arrival: at most once per 20 minutes
  of play, the time saved with the map's world.
- `Session/SessionSubsystemLegendary.cpp`: when a map's Legendary monsters come back: on an arrival at least 20 minutes
  of play after the last death (`LegendaryDefeatedAt`); noting and forgetting one.

## Areas
- `Areas/AreaDefinition`: `UAreaDefinition`, one area as a data asset in `/Game/Data/Areas` (`DA_Area_<Id>`, made by
  `Tools/Unreal/create_area_assets.py`): its name, level (which may not be built yet), landings, practice flag
  (a practice area gives no kill experience, and only ammo once the player has left it), opening mission, level band
  and promotion chances; finding areas by name or level.
- `Areas/AreaLandings`: `AreaLandings`, where trips arrive: an actor or player start tagged `Landing_<Place>`, or a
  component of an actor so tagged (the jetty's, the station's), found in a level; a level's own start is never one.
- `Areas/AreaRulesSubsystem`: `UAreaRulesSubsystem`, the area being played's rules for its creatures: each one's level
  from the area's band around the player's (bosses not rolled), and placed Basic creatures' promotions rolled as the
  player arrives (at most once per 20 minutes of play per map); the practice rules (`GivesKillExperience`, `DropsGuns`).
- `Areas/StationBoard`: `StationBoard`, `FStationBoardLine` and `FStationBoardWords`: a station board's lines as the
  story stands (every opened area, the blank line naming the mission that opens the next, "Skyreach (practice)" after
  the first cast-off; before it only the first cast-off to the story's first arrival), each board's words, and
  recording the first cast-off.
- `Areas/AreaTravelSubsystem`: `UAreaTravelSubsystem`, trips from the boards: the first cast-off's trip behind the white
  (`CompleteFirstCastOff`, `LeaveForFirstArrival`), plain fades to a station (`FadeTo`), and arriving (the cold open while
  it's due, which takes the held white; else the white revealed, REVENANT on the first arrival; a fade in after a plain
  trip).

## World
- `World/MinimapSubsystem`: `UMinimapSubsystem`, bakes the top-down map picture at runtime at about a meter per texel,
  trees as crowns.
- `World/MinimapPaint`: `MinimapPaint`, the map picture's rules texel by texel (land, coasts, cliffs, contours; with a
  playable area, the outside dimmed and the closed edges drawn as its boundary line).
- `World/FallRecoverySubsystem`: brings the player back after a fall: off a playable area's open edge after 5 m,
  anywhere after 30 m; `OnRecovered` says who, why and from where.
- `World/FallRecoveryTracker`: `FFallRecoveryTracker`, one player's safe spot (taken only inside the playable area),
  how they left it, and when to bring them back.
- `World/RespawnMarker`: `ARespawnMarker`, a respawn grave: open from the start, with its mission's finish or by the
  campaign record (kept there by id); `ChooseWakeSpot`, where a death wakes the player (the nearest open grave, else the
  level's own start, never a landing).
- `World/PlayableArea`: `APlayableArea`, a level's playable boundary (corners and open edges, set by the area's build
  script): `Contains()`, the invisible walls the game builds behind the closed edges (the `PlayableBounds` collision
  profile blocks only walking pawns), and its `Looter.World.Bounds` drawing.
- `World/PlayableBoundary`: `FPlayableBoundary`, a playable boundary as plain XY geometry (inside, crossings, the edge
  a path leaves over, the nearest edge, where the walls stand), shared by the area, fall recovery, the minimap and the
  tests.
- `World/WorldQueries`: `LooterWorld`, trace params for finding the ground (skipping volumes and the playable area's
  walls, and with the PCG volume what it scattered).
- `World/LightBeam`: `LightBeams`, a soft glowing light pillar (sky beacons, the rarity-colored beam over loot).
- `World/Windmill`: `AWindmill`, a water-pump windmill whose fan (a separate model on the tower's Fan socket) turns in gusts.
- `World/SkiffJetty`: `ASkiffJetty`, Skyreach's jetty with its bell, slate and landing (Landing_Jetty); the gangplank (up
  until the tutorial is done, always down after the first cast-off; lowering it rings the bell and offers "Board the
  skiff"); holding Interact at it opens the station board. `SkiffJettyMooring.cpp` is the moored skiff (its own actor
  in play) and its mooring lines; `SkiffJettyCastOff.cpp` casting off (the ride, the lines slipping, the trip at the
  whiteout).
- `World/TrainStation`: `ATrainStation`, an area's station (the depot by default) with its departures board (hold
  Interact to open the station board) and its landing on the platform (Landing_Depot).
- `World/SafeGround`: `ASafeGround`, a safe zone: ground where nothing hunts the player (Delia's salt line; Main
  Street after Main 3), a polygon or circle switched by its story condition.
- `World/WantedPoster`: `AWantedPoster`, Ellis's wanted poster as a decal of its atlas cell, torn down by holding
  Interact (it swaps to the remnant and a scrap falls; Hob remarks), or Calder's note read with a tap; torn ones stay
  torn with the session's map world. `WantedPosterTear.cpp` is the tear and the falling scrap.
- `World/WindowShutter`: `AWindowShutter`, one of Main Street's shutters on a false front's Shutter socket (Main 3): open
  flat against the wall until the player comes near, then it slams shut (after a moment of its own) and stays shut;
  shut from the start after Main 3.
- `World/ChapelBell`: `AChapelBell`, the Chapel of Saint Ada's bell (Main 4, tagged Bell_Chapel): held on the rope's grip
  (the chapel's SOCKET_Interact) it rings: SM_ChapelBell on the belfry's SOCKET_Bell swings about its axis, the swings
  dying away over 9 s, tolling at the ends of the hard ones (a sound when one is made); not again until still, nor
  while its story condition says not.
- `World/ChapelReliquary`: `AChapelReliquary`, Saint Ada's smashed Reliquary on the apse's plinth (Main 4, tagged
  Reliquary_Chapel): looked at (a tap, from Main 4's fourth step) it plays the two-second Grave Sight flash and her
  ember lifting off the lid (SOCKET_Ember; the cold open's glow), then tells the missions (`GraveSight.Reliquary`);
  plain shapes stand in without its model.
- `World/EggSac`: `AEggSac`, an egg sac in the Sink (Main 5): shootable only while its story allows (SM_EggSac_A/_B/_C);
  shot down it falls to the ground under it, becomes SM_EggSac_Burst and lets out two Basic spiders at its Spawn
  sockets (gone for good once killed), telling the missions `EggSac.Burst`; burst from the start once the story is past
  it. `EggSacFall.cpp` is the fall, the burst and the spiders.
- `World/KeepersLantern`: `AKeepersLantern`, the Keeper's Lantern hanging dark in the Sink's webbing (Web_Snare, no
  light, its glass the trim's window glass), taken with a tap in Main 5's third step; whether Ellis has it comes from
  the campaign record (`IsTaken`); `SetLit` for Main 6.
- `World/KeeperLanternPost`: `AKeeperLanternPost`, a lantern post on the burial deck: put out when Abel drifts, relit by
  holding Interact 1.5 s (which drags him back). The keeper's post takes the Keeper's Lantern in Main 6 with a tap, and
  Abel's scene lights it leaning north-east (`OnKeepersLanternLit`).
- `World/StoryLighting`: `AStoryLighting`, the level's lighting state by the story (rules of a condition and a state:
  Main 6 and Main 7 at Dusk), switched as the missions change. Nothing ticks.
- `World/DuskScenery`: `ADuskScenery`, one mesh's instances seen only in one lighting state (the Gravewind's wisps and
  the canyon fog at dusk): no collision or shadows, each instance culled at 70 m, switched by
  `ULightingStateSubsystem::OnChanged`. Nothing ticks.
- `World/HouseLights`: `AHouseLights`, a lived-in house's lights for the lighting states: a warm unshadowed lamp at the
  house's SOCKET_Light and its WindowGlow slot's glow (an instance of its own), both brighter at dusk; placed by
  `build_area.py` from `layout.json` `level.lights` (Delia's, the store, Tilly's, two cottages). Nothing ticks.
- `World/InstancedScenery`: `AInstancedScenery`, scenery far past a level's playable boundary drawn as one mesh's
  instances (Ransom's Rest's far trees, one actor per tree mesh, set by `build_area.py`): no collision, navigation
  or shadows, never distance-culled.
- `World/PCGGroundFitFilter`: `UPCGGroundFitFilterSettings`, the meadow's PCG node that drops ground cover patches
  hanging off an edge and presses ones floating over a bump into the ground (editor-time; the graph ships with the level).
- `World/LightingState`: `FLightingState`, one way a level can be lit (Day, Dusk): the sun by bearing and elevation with
  its shadows, the sky light, the sky's own color and ozone, the fog's density and colors, the exposure, and the tints
  `MPC_Lighting` carries to the unlit backdrop and clouds; blending two (the sun the short way round). Its defaults are
  the tutorial island's afternoon.
- `World/LightingStates`: `ALightingStates`, a level's lighting states as data, placed with its lights by
  `Tools/Unreal/build_area_environment.py` from `layout.json` (Day is the light it's built in; Dusk and others from
  `level.environment.states`), and the lights and collection they drive.
- `World/LightingTargets`: `FLightingTargets`, the lights a state drives (the states actor's, else the level's by kind:
  sun, sky light, sky atmosphere, height fog, post volume): sets a state on them and reads what they show.
- `World/LightingStateSubsystem`: `ULightingStateSubsystem`, switches the level between its states: behind the camera's
  fade with the sky light recaptured (the default), at once behind a caller's cover, or blended over seconds; writes
  `MPC_Lighting`; `OnChanged` for the cold open and Main 6. Ticks only during a switch.

## Settings
- `Settings/KeyBindingSubsystem`: key rebinding, the global pause/inventory actions and the character actions
  (weapon slots 1-3 among them).
- `Settings/GraphicsSettingsSubsystem`: saved display options (quality preset, motion blur, first-person field of view, UI transparency, minimap on/off, size and zoom, FPS counter) and the `Looter.Quality` and `Looter.FieldOfView` commands.

## UI
- `UI/Style/LooterUIStyle`: `LooterUI`, the style kit every UI is built with (palette, shapes, icons, text, builders,
  transparency, the display type for title cards).
- `UI/Style/LooterUIInkedIcons.cpp`: the kit's Inked icons (`FInkedIcon`, the weapon and ammo icons) drawn into textures, as
  `Art/Icons/InkedIcons.py` draws its reference pictures.
- `UI/Style/InkedIconData.inl`: every Inked icon as C++ data, generated by `Art/Icons/InkedIcons.py` (don't edit it).
- `UI/Style/LooterButton`: `ULooterButton`, the kit's button.
- `UI/Style/WeaponText`: `LooterWeaponText`, weapon names (a named gun's own), rarity colors, stat strings, and a
  named gun's flavor line.
- `UI/HUD/LooterHUD`: `ALooterHUD`, owns the HUD, the captions, the inventory's pages (loadout, bestiary, missions), the
  station board and the pause menu (the settings menu with Save & Quit), and their hotkeys; a scene that holds the
  player puts the gameplay HUD away.
- `UI/HUD/PlayerHUDWidget`: the gameplay HUD (health, ammo, crosshair, hit marker, loot card, interaction prompt,
  messages), run frame by frame; `PlayerHUDWidgetLayout.cpp` builds it; `PlayerHUDWidgetPickupCard.cpp` fills the loot
  comparison card and the interaction prompt.
- `UI/HUD/HudVitalsWidget`: health at the bottom left: a ring with the number inside and a solid bar out of its lower
  side, with a damage chip, a hit flash and a low-health beat.
- `UI/HUD/HudMagazineWidget`: the magazine gauge in the ammo row: a cartridge whose inside drains from the nose as the
  gun fires (the reload bar while reloading), the count inside by the base.
- `UI/HUD/HudMinimapWidget`: the round minimap that turns with the view (size and zoom from the settings), with the
  tracked mission's waypoint on it, or a compass arrow and its distance on the rim.
- `UI/HUD/TutorialPromptWidget`: the tutorial's current instruction near the top of the screen; it steps aside, its
  "complete" line waiting with its time held, while a menu, the pause menu or a scene covers the game.
- `UI/HUD/HudXPBarWidget`: the level and experience bar at the bottom center: the level in a circle in the middle of a
  slanted bar (two halves) ticked at every tenth that flashes what was just gained; `HudXPBarWidgetLayout.cpp` builds
  and paints it.
- `UI/HUD/HudWeaponSlotsWidget`: the weapon slots over the ammo, as circles in the guns' rarity colors with their Inked icons (tilted up) and their ammo's icon under each.
- `UI/HUD/HudFrameRateWidget`: the frame rate counter in the top-left corner.
- `UI/HUD/HudBossBarWidget`: `UHudBossBarWidget`, a boss's bar at the top of the screen: level, name, phase ticks, a
  lingering chip, grey while it can't be hurt, the phase's name under it.
- `UI/HUD/HudPickupFeedWidget`: ammo pickups left of the crosshair, in outlined white type that stacks, rises and fades.
- `UI/HUD/HudInteractPromptWidget`: what the Interact key does to the thing looked at, under the crosshair ("[E] OPEN
  THE DOOR", "HOLD [E] RING THE BELL" over a bar that fills while held); loot has its card instead.
- `UI/HUD/HudCaptionWidget`: the captions low on the screen over the experience bar: the speaker's name in the accent
  color over their words, outlined with no panel, fading in and out; steps aside and holds the lines while a menu is
  open.
- `UI/HUD/HudGraveSightWidget`: `UHudGraveSightWidget`, Grave Sight's flash on the screen: the kit's dark glass at the
  edges (a background) under a thin cyan frame with cut corners and brackets, faint scan lines, a sweep down the screen
  and a ring opening from the middle; `UGraveSightSubsystem` drives it.
- `UI/Menus/SettingsMenuWidget`: `USettingsMenuWidget`, the settings menu (graphics, interface, key bindings), over the
  paused game or from the main menu: its layout. `SettingsMenuRows.cpp` makes its rows and key list,
  `SettingsMenuInput.cpp` handles its buttons, sliders and keys, `SettingsMenuParts.h` holds what they share.
- `UI/Menus/MainMenuHUD`: `AMainMenuHUD`, the main menu's HUD: the menu and its settings.
- `UI/Menus/MainMenuWidget`: `UMainMenuWidget`, the main menu (Single Player, Multiplayer, Settings, Quit Game);
  `MainMenuSessions.cpp` is its session picker (each session's area by name), a new game's choice to play or skip the
  tutorial, and the delete confirmation.
- `UI/Inventory/LoadoutWidget.cpp`: the loadout screen: opening, layout and contents.
- `UI/Inventory/LoadoutWidgetInput.cpp`: its cursor, actions (swap, hold, drop), mouse handling and turning the stand-in.
- `UI/Inventory/LoadoutWidgetDrag.cpp`: dragging guns between slots, the backpack and the character.
- `UI/Inventory/LoadoutWidgetInspect.cpp`: the stats card that floats beside the gun under the cursor.
- `UI/Inventory/LoadoutWidgetPaint.cpp`: the stand's ring under the stand-in.
- `UI/Inventory/LoadoutWidget.h`: the loadout screen's declaration.
- `UI/Inventory/MissionsWidget`: the missions page, the inventory's third: opening, layout and the mission log;
  `MissionsWidgetDetails.cpp` the chosen mission's steps, objectives and rewards, the keys and tracking.
- `UI/Inventory/LoadoutRules`: `LoadoutRules`, the backpack list's compare and sort rules.
- `UI/Inventory/LoadoutParts`: `LoadoutParts`, the inventory pages' layout, colors, vector art, card builders and title
  tabs (the bestiary's titled "Ledger" once it's Sexton's, `TitlePageTabs`).
- `UI/Inventory/LoadoutPaintLayer`: `ULoadoutPaintLayer`, a see-through layer the screen draws on.
- `UI/Inventory/LoadoutStage`: `ALoadoutStage`, the off-screen stand-in of the character and its capture.
- `UI/Inventory/StageStudio`: `StageStudio`, what the inventory's off-screen stands share (spot, capture, studio lights,
  picture, projection).
- `UI/Bestiary/BestiaryWidget.cpp`: the bestiary, the inventory's second page (Sexton's Ledger from Main 2, with its
  own pages and words): opening, layout and the list.
- `UI/Bestiary/BestiaryWidgetDetails.cpp`: the chosen entry's card, description and notes, for each kind of page (an
  actor's numbers, a story character's words, a Ledger name's whereabouts).
- `UI/Bestiary/BestiaryWidgetInput.cpp`: its keys, turning the model, and the ring it stands on.
- `UI/Bestiary/BestiaryWidget.h`: the bestiary's declaration.
- `UI/Bestiary/BestiaryStage`: `ABestiaryStage`, the off-screen stand that shows an entry's model (skinned, or still
  with its parts on their sockets), framed to its size, wearing what the actor wears on its bones.
- `UI/World/WeaponLabelWidget`: the label over loot guns (a named gun's flavor line under its name when looked at).
- `UI/World/CreatureHealthBarWidget`: the tag over a hurt or hunting creature: floating level, rank word (in its rank's
  color) and name over a slim bar of fixed width, cut into quarters whatever the health.
- `UI/World/DamageNumberActor`, `UI/World/DamageNumberWidget`: floating damage numbers.
- `UI/World/StationBoardWidget`: `UStationBoardWidget`, the station board (a LooterUI panel over the dimmed world): its
  lines and keys; `StationBoardWidgetConfirm.cpp` the confirm ("Leave Skyreach? ..." with Cast off and Not yet on the
  first cast-off) and the trip.

## Dev
- `Dev/WeaponDevCommands.cpp`: console commands for testing (`Looter.GiveWeapon`, a named gun too:
  `Looter.GiveWeapon Heirloom [level]`; `Looter.SpawnAmmo`).
- `Dev/ProgressionDevCommands.cpp`: console commands for levels (`Looter.GiveXP`, `Looter.SetLevel`,
  `Looter.ResetProgress`, `Looter.XP.Table`) and the bestiary (`Looter.ForgetBestiary`).
- `Dev/MissionDevCommands.cpp`: `Looter.Mission.Start <id> [step]`, `.Complete <id> [all]`, `.List`,
  `.Event <name> [tag] [hold]`.
- `Dev/AbelDevCommands.cpp`: `Looter.Abel.Fight [here]`, `.Phase <1-3>`, `.Kneel`, `.Scene`, `.Back`, `.Reset`,
  `.Lanterns [dark|lit]`, `.Move <grieve|flare|drift|walkoff|pull|gust>`, `.State`, `.Perf [adds]` (the tour's dusk
  fight view).
- `Dev/SessionDevCommands.cpp`: `Looter.Session.Play <1-3>`, `.Save`, `.Menu`, `.List`, `.CheckSave [1-3 | file]` (a
  save's upgrade checked on a copy), `.Copy <from> <to>` (into an empty slot only), `Looter.Travel <area or level>
  [landing]`, `Looter.Area.List`.
- `Dev/CreatureDevCommands.cpp`: `Looter.CreatureHealth`, gives the nearest creatures chosen health (to compare their
  bars); `Looter.SpawnCreature <Spider|Spiderling|Gravemother|Slime|Unpaid> [rank] [count] [chase] [size=] [level=]` (no rank:
  the kind's own), spawns ranked creatures in front of the player (gone for good once killed); `Looter.Perf.Horde <kind> <count> [rank] [x y yaw]`, a fight measured where it
  happens (the player put there, unhurtable, the creatures coming at them; for `perf.ps1 -Exec`).
- `Dev/BossDevCommands.cpp`: `Looter.Boss.Test [phases]`, the test boss in front of the player with its fight started;
  `Looter.Boss.Reset`, `Looter.Boss.Kill`.
- `Dev/LegendaryDevCommands.cpp`: `Looter.Legendary.List`, `.Forget [id | all]`, `.Return [id]` (a Legendary
  monster's lair brought back now, whatever the story).
- `Dev/SinkDevCommands.cpp`: `Looter.Story.EggSacs [force | reset]` (every egg sac shot down, or hung up again),
  `Looter.Story.Lantern [force]` (the lantern taken, the missions told).
- `Dev/LootDevCommands.cpp`: `Looter.Loot.SimulateDrops <rank> [kills]`, rolls a rank's loot table and prints its odds.
- `Dev/InteractionDevCommands.cpp`: `Looter.Interaction.Spawn <door|bell|lantern>`, a greybox interactable in front of
  the player; `Looter.Interaction.Focus`, what the player would use now.
- `Dev/StoryDevCommands.cpp`: `Looter.Story.Caption [speaker words...] | stop`, `Looter.Story.Door` (Delia's talking
  test door), `Looter.Story.Character [mission id]` (a placeholder story character), `Looter.Story.Shutters [open |
  slam]` (Main Street's shutters), `Looter.Story.Ledger` (whether the bestiary is the Ledger yet, and its own pages), `Looter.Story.GraveSight [force]`
  (Grave Sight on the nearest Reliquary, or the flash alone), `Looter.Story.Bell` (rings the nearest chapel bell; the
  missions told).
- `Dev/RespawnDevCommands.cpp`: `Looter.Respawn.List`, `.Activate <id | all>`, `.Place [closed]` (a test grave where the
  player stands), `.Die`.
- `Dev/WorldDevCommands.cpp`: `Looter.InstanceCollision`, whether the world's instanced meshes (the scattered trees and
  rocks) have collision bodies; `Looter.World.Bounds`, draws the playable area's boundary and walls;
  `Looter.Perf.HideTag <tag> [1|0]`, hides a tagged group to measure its cost by the difference;
  `Looter.Perf.FarShadow <cascades> [metres]`, far shadow cascades drawn by actors tagged Obstacle, to see and measure.
- `Dev/LightingDevCommands.cpp`: `Looter.Light [state] [seconds | now]`, switches the level's lighting state (behind a
  fade, blended, or at once); with no state, lists them.
- `Dev/EncounterDevCommands.cpp`: `Looter.Encounter.List`, `.Wave <spawner id | event | nearest> [force]`,
  `.Zones [1|0]` (safe zones, spawners' ground, spots and approach rings), `.Test` (a test spawner where the player looks).
- `Dev/StationDevCommands.cpp`: `Looter.Station.Board`, `.CastOff`, `.Gangplank up|down`, `.SkipTutorial [stay]`,
  `.Lines`.
- `Dev/PosterDevCommands.cpp`: `Looter.Poster.Spawn [note]` (a wanted poster, or Calder's note, on the wall the
  player looks at), `Looter.Poster.TearAll [count]` (tears posters as a held Interact would; missions told).
- `Dev/SceneDevCommands.cpp`: `Looter.Scene.Skip`, `Looter.Scene.Ride` (the ride on the nearest skiff, then everything
  back and the white revealed with no trip), `Looter.Scene.Title [text]`, `Looter.Scene.ColdOpen` (the cold open then
  the wake-up, recording nothing), `Looter.Scene.GraveWake` (the claw-out alone), `Looter.Scene.Claw` (a claw, as Jump).
- `Dev/ViewTour`: `UViewTourSubsystem`, `Looter.Tour`: looks from each viewpoint of a level, measures frame times there and takes screenshots (`Tools/tour.ps1`); a view's `exec` and `after` commands measure a hidden group by the difference.

## Tests (run with `Tools\runtests.ps1`)
- `Tests/AnimationTests.cpp`, `AreaTests.cpp`, `BestiaryTests.cpp`, `BossTests.cpp`, `BossCombatTests.cpp` (with `BossTestWorld.h`), `CreatureTests.cpp`, `CreatureRankTests.cpp`, `EncounterTests.cpp`, `EncounterPlayTests.cpp` (with `EncounterTestWorld.h`), `InteractionTests.cpp`, `InteractionPropTests.cpp` (with `InteractionTestWorld.h`), `InventoryTests.cpp`, `LevelBandTests.cpp`, `LightingTests.cpp`, `LocomotionTests.cpp`, `LootTests.cpp`, `LootRankTests.cpp`,
  `MinimapTests.cpp`, `MissionTests.cpp`, `MissionRunnerTests.cpp` (with `MissionTestWorld.h`), `PlayableAreaTests.cpp`, `PosterTests.cpp`, `ProgressionTests.cpp`, `RespawnTests.cpp`, `SceneTests.cpp`, `ColdOpenTests.cpp` (the timeline's waits, the gang's skiff's course, the claw-out, the cold open on the first arrival only), `SessionTests.cpp`, `SettingsTests.cpp`, `SevenDaysTests.cpp` (Main 1's steps and reward, the headboards', Delia's and Hob's lines), `TalkBusinessTests.cpp` (Main 2: its steps, Main 1 first and its reward, the nest's spiders, Sexton shown by the story and his deal, the placed pieces), `LedgerTests.cpp` (the Ledger's step, the story-character page type, the seven names with their whereabouts blank), `ColdWelcomeTests.cpp` (Main 3: its steps, Main 2 first and its reward, the gate's fight by count and rank, Tilly's topics, the shutters, the placed pieces), `HallowedGroundTests.cpp` (Main 4: its steps, Main 3 first, the yard's two waves by count and rank, the bell held, the Reliquary's flash ending its step, Aldana's words and the chapel yard's grave; the Unpaid on boot hill and the north road after it; the placed pieces), `ChapelTests.cpp` (the bell's hold, swing and tolls; Grave Sight's flash and the Reliquary's look timing out; Aldana's topics) with `HallowedGroundTestWorld.h`, `EggSacTests.cpp` (the egg sac's fall, burst and spiders, shootable only in its step; the lantern dark, taken in its step, gone after), `KeepersLanternTests.cpp` (with `KeepersLanternTestWorld.h`; Main 5: its steps, Main 4 first, the floor's spiders, Side 3 after it, the placed pieces), `SkiffJettyTests.cpp`, `SlimeTests.cpp`, `StationTests.cpp`, `StoryTests.cpp`, `TutorialTests.cpp`, `UnpaidTests.cpp`, `UnpaidMotionTests.cpp`, `GravemotherTests.cpp` (her body, charge, brood, pack calls by tag,
  loot), `GravemotherSideTests.cpp` (her return after 20 minutes of play, her lair, Side 3), `AbelTests.cpp`, `AbelFightTests.cpp` (with `AbelTestWorld.h`: Abel's pose table, rules, body, phases, lanterns, reset, fog wall, the Gravewind and a fall, the kneel and the scene), `GravewindTests.cpp` (with `GravewindTestWorld.h`: Main 6 end to end, Pa on his board, the dusk scenery, the level as built), `WeaponTests.cpp`, `NamedWeaponTests.cpp` (named guns: the
  fixed-quality rules, Heirloom's asset and label, its save, the mission reward),
  `WeaponPartsTests.cpp`, `WorldTests.cpp`: the `Looter.*` automation tests, one file per area.

## LooterEditor (editor-only module; nothing here ships)
- `LooterEditor.Build.cs`: module dependencies (GeometryScript editor functions, asset tools, FBX import, JSON).
- `LooterEditorModule.cpp`: the module and its console commands: `Looter.BakeLevelProps`, `Looter.SettleProps`,
  `Looter.BakeGroundCover`, `Looter.ImportModels`, `Looter.FixStylizedMaterials`.
- `StylizedProp`: `AStylizedProp` and `EStylizedPropShape`, procedural props for building levels in the editor.
- `StylizedMeshKit`: `StylizedMesh`, GeometryScript helpers the props are built with.
- `StylizedSurface`: `FStylizedSurface`, `StylizedColors`, `StylizedSurfaces`, how each material slot is painted.
- `PropBaker`: `FPropBaker`, turns a level's props into Nanite static mesh assets, material instances and placed actors,
  and bakes the meadow's ground cover meshes again.
- `PropSettler`: `PropSettler`, seats a level's props on the terrain (no hovering edges; low, wide props lean with the slope).
- `ModelImporter`: `FModelImporter`, imports the Blender models `Tools/models.ps1` exported (fixed FBX settings,
  material instances (saved only when an import changes them), hull collision or none (a plant's hulls are all of its
  collision), sockets, LODs, Nanite with its fallback share). `ModelImporterMaterials.cpp`
  imports texture sets and makes instances of the textured masters; `ModelImporterRig.cpp` imports rigged models as
  skeletal meshes and turns their hit zones into a physics asset.
- `SurfaceMaterials`: `SurfaceMaterials`, stylized material instance assets and the parents' usage flags.
- `LooterMaterialGraphTools`: `ULooterMaterialGraphTools`, material graph helpers for the build scripts (from
  Python): `ClearMaterialGraph` empties a master to build it again in place, moving the nodes loaded as the editor
  started (rooted for good) out of it instead of deleting them.
- `LooterLevelTools`: `ULooterLevelTools`, level helpers for the build scripts (from Python):
  `FinishAssetCompilation` waits for a just-loaded level's meshes to finish building, so traces find the ground.
- `Tests/StylizedPropTests.cpp`, `PropSettlerTests.cpp`, `ModelImportTests.cpp`, `TexturedImportTests.cpp`,
  `RigImportTests.cpp`, `SurfaceMaterialTests.cpp`, `MaterialGraphToolsTests.cpp`: the `Looter.Editor.*` tests (prop
  shapes and ground cover size,
  settling on the ground and the terrain's full fallback, the Blender import settings against
  `Tests/ModelImport/AxisTest`, textured materials, LODs and no-collision against `Tests/TexturedImport/TexturedTest`,
  `Tests/RigImport/RigTest`, the stylized materials' usage flags, clearing a master whose nodes are rooted).

## Terrain generator and level scripts (Python)
- `Art/Levels/area_shape.py`: `Area`, an area's shape from `Art/Levels/<Area>/layout.json` (`TutorialIsland` is the
  island fixture, `TerrainTest` the grounded one, `RansomsRest` the first campaign area): map square and raster sizes,
  features as lists by type, the height raster and water surface; `layout_sha1()`, the layout's fingerprint without its
  build-only `level` and `gameplay` blocks.
- `Art/Levels/area_math.py`: noise, rasters over the map square, curves and distance fields.
- `Art/Levels/area_region.py`: the grounded setting: the regional height field around the core (valley floor and its
  benches, ridges and their saddles, an escarpment and its canyon), the core's edge, the playable boundary and the seam
  band.
- `Art/Levels/area_boundary.py`: the ground past a grounded area's playable boundary: its edges as open, blocked
  (closed by something built) or rock; the rock foot raised past every rock edge (`boundary.foot`, the cliff group
  `boundaryFoot`), and the rise a player meets past each edge (`boundary.rise` in `layout_computed.json`).
- `Art/Levels/area_features.py`: the feature types that came with Ransom's Rest (ridge, scarp, pit, mesa, knob, gully,
  and a creek's falls into a gorge) as steps of `Area.build()`, and the sides of a ramp that drop as cliffs.
- `Art/Levels/area_mesh.py`: the top's triangulation, terrain AO, the island's underside, the water mesh.
- `Art/Levels/area_macro.py`: the macro color map `T_<Area>Macro_BC`, in the season the layout grades it to
  (`macro.grade`). `Art/Levels/area_scatter.py`: the PCG scatter mask `T_<Area>Scatter_BC` (with
  `scatter.roadside: pebbles`, no rocks or boulders on or beside the roads).
- `Art/Levels/area_computed.py`: `layout_computed.json`: placements at terrain height (or their own), cliff groups per
  feature (a ramp's cliff side and the boundary's rock foot too), bridge, waterfall, orchard rows, the squares the maps
  cover, the rise past the boundary's closed edges, and the far trees (one per line).
- `Art/Levels/area_cliffs.py`: cliff dressing for those features and the grounded setting: a group per feature, walls
  over 12 m in stacked courses, a knob's outcrop, a gully's banks, gaps in a pit's ring for rock models.
- `Art/Levels/area_open.py`: the open-ground metric: how far each walkable meter inside the boundary is from its
  nearest break (cover), in `layout_computed.json` and the plan preview.
- `Art/Levels/area_beyond.py`: what lies past a grounded area's core: the surround ring, the canyon wall and the
  backdrop's silhouettes (per layer a skyline with rolling relief, soft U notches and landforms of five kinds: mesa,
  butte, stepped, broken, spire, with columns at their corners; each vertex's depth below its skyline for UV 1).
- `Art/Levels/area_fartrees.py`: the far trees past a grounded area's boundary (`region.farTrees`): groves and contour
  tree lines in the layout's woods, cottonwoods along the canyon's river, off steep ground, water, roads and the
  clear corridors (the line out of Stage Gap), thinned where nobody inside sees them, each seated on the core's and
  the ring's meshes; `farTrees` in `layout_computed.json`.
- `Art/Levels/area_preview.py`: Blender preview renders and the annotated plan: views at the planned sun, clay views
  with stand-ins for the buildings, obstacles and cliff courses, straight-down shadow views, and the far trees over
  the ring (`far_trees.png`).
- `Art/Levels/area_model.py`: the Blender side (tile, underside, water, ring, canyon wall and backdrop objects and
  their materials; the backdrop's tints, darker near and paler far, and its UV 1 `Depth`); `Art/Models/Terrain/<Area>.py`
  wrappers call it.
- `Tools/Unreal/build_area.py`: `AreaBuild`, builds an area's level (`build_area.py <Area>
  [gameplay|environment|beyond|cliffs]`), with what lies past the boundary tagged Beyond (a grounded area's ring, canyon
  wall, backdrop and far trees, the last as `AInstancedScenery`; an island's sky islands), and the boundary's rock pieces
  varied (mirrored, sunk, turned); `build_tutorial_island.py` wraps it for the tutorial island. From `layout.json`'s
  level block an area swaps in models of its own (`models`), places models the generator needn't know (`props`: the
  town gate), chooses which chimneys smoke (`smoke`) and which houses are lit (`lights`: AHouseLights, or dark windows),
  varies its cliffs' tops and leaves gaps (`cliffs`), and wears its own instances of shared materials (`materials`,
  `swaps`: Ransom's Rest's rock and orchard leaves).
- `Tools/Unreal/build_area_bounds.py`: a grounded area's bounds for `build_area.py`: the playable area from the
  computed boundary (its walls `level.wallSetback` behind the line, at the rock's foot), the KillZ 100 m under the
  canyon floor, and a cull distance volume (sizes to distances).
- `Tools/Unreal/build_area_environment.py`: every area's light, sky and fog for `build_area.py`: the tutorial island's
  afternoon by default, or the layout's `level.environment` (sun by bearing and elevation, haze and where it ends,
  atmosphere with the sky's own color and ozone, cloud dome), and the level's lighting states (`ALightingStates`: Day as
  placed, plus `level.environment.states` such as Dusk).
- `Tools/Unreal/lighting_collection.py`: `MPC_Lighting`, the material parameter collection the lighting states write
  (`BackdropTint`, `CloudTint`, the fog's colors), made by `build_world_materials.py` before `M_Backdrop`, which reads it.
- `Tools/Unreal/build_creature_materials.py`: the creatures' materials beside the world's masters: `M_Ghost` (the
  Unpaid's masked, dithered ghost, its rank, dissolve and flare from the creature's custom primitive data) and the
  clothing tints `MI_Ghost_B` and `_C`, and the Gravemother's pale hide `MI_SpiderBody_Pale` (`Spider.py`'s pale color
  map, imported whenever it changes, on the brown spider's normal and ORM maps); by name, only the ones asked for; it
  reuses `build_world_materials.py`'s helpers without running its build.
- `Tools/Unreal/build_decal_materials.py`: the decals' materials: `T_Posters_BC` from the art (or its stand-in),
  `M_PosterDecal` (deferred decal: an atlas cell, a hard 0.5 edge, Tint, Roughness), `M_PosterScrap` (the falling
  scrap: masked, two-sided, a dithered fade from custom primitive data) and their instances.
  `Tools/Unreal/create_side_mission_assets.py`: the side missions' data assets (`DA_Mission_Side1`, made once its
  prerequisite Main 3 exists; `DA_Mission_Side3`, once Main 5 does).
  `Tools/Unreal/build_area_den.py`: Side 3's pieces for `build_area_story.py`: the Gravemother's lair (after Main 5) at
  the den's mouth and the den's place (`Place_Den`), from Den Rock's sockets, else the layout. `Tools/Unreal/build_area_posters.py`: an area's posters from `layout.json`
  `gameplay.posters` (on a host's face, snapped by a trace, or on a socket), for `build_area.py` or on their own.
- `Tools/Unreal/create_named_weapons.py`: the named guns' data assets (`DA_Named_Heirloom`), checked (`FindProblems`)
  before they're saved.
- `Tools/Unreal/build_area_travel.py`: where an area's trips start and end, for `build_area.py`: the skiff jetty
  (`gameplay.jetty`), the depot as its station with the landing on the platform, the first arrival's player start
  beside the level's own (`gameplay.spawnLanding`), and markers for other landings.
- `Tools/Unreal/build_area_story.py`: the story's pieces placed from the placed models' sockets and the layout's zones,
  obstacles and roads, for `build_area.py`'s gameplay pass (Ransom's Rest only): the cold open's set at the lookout, the
  family plot's respawn grave (open after Main 1), the two headboards to read, Delia's speaker point at the farmhouse's
  screen door (with the farmhouse's screen door and plate, `build_area_farm.py`); Sexton on the lookout's rail, Ransom's
  Point's place and its spider nest (Main 2); the town gate's place and its Unpaid, Tilly's window, the store's
  shutters, the farm's and Main Street's safe zones (Main 3); Main 4's pieces from `build_area_chapel.py`; Main 5's
  from `build_area_sink.py`; the Gravemother's lair from `build_area_den.py`; Main 6's from `build_area_deck.py`; and Hob
  with his perches through Main 6
  (on the town gate's SOCKET_Perch for Main 3).
- `Tools/Unreal/build_area_sink.py`: Main 5's pieces for `build_area_story.py`: the Sink's floor (blocks and coffins), its
  web cards (shadows off), the three egg sacs on their lines and sling, the lantern in its snare, the floor's and ramp
  head's places, the floor's spiders, the den's web funnel, the Webwood's dead trees and Hob's Sink perches; transforms
  in tables in the Sink's own frame.
- `Tools/Unreal/build_area_chapel.py`: Main 4's pieces for `build_area_story.py`: the chapel's place, the chapel yard's
  fight (two waves of six Unpaid, the Restless one with the second, inside the churchyard fence), the bell at the rope's
  grip with SM_ChapelBell on the belfry socket, the smashed Reliquary, Aldana's vestry door, the chapel yard's respawn
  grave (after Main 4), the Unpaid on boot hill and the north road after Main 4, and Hob's chapel perches.
- `Tools/Unreal/build_area_farm.py`: Ransom Farm's pieces for `build_area_story.py`, on the lived-in farmhouse's
  sockets: Delia's door (SOCKET_Speaker; after Main 5, her word that starts Main 6), the screen door hung closed and the
  plate on the porch stool.
- `Tools/Unreal/build_area_deck.py`: Main 6's pieces for `build_area_story.py`: the burial deck's eight biers and three
  lantern posts (one the keeper's), Abel in the aisle, Pa's board on a bier's Sit socket, the fog wall across the
  Keeper's Gate, Gravewind Point's place, the keeper's grave's respawn (after Main 5), the story's dusk, the Gravewind's
  wisps and canyon fog, and Hob's Main 6 perches.
- `Tools/abel_poses.py`: writes `Bosses/AbelPoseData.inl` from Abel.py's exported poses; run it again after they
  change.
- `Tools/Unreal/build_island_scatter.py`: an area's PCG scatter graph and volume (`[Area]`); its mask is imported
  again whenever the PNG changes (its MD5 kept on the texture as metadata).
  `Tools/Unreal/island_views.py`: an area's viewpoints as editor cameras and shots.
- `Tools/terrain_identity.ps1`, `.py`: checks an area regenerates exactly as committed (headless Blender, no editor;
  layoutSha1 left out);
  `Tools/Blender/terrain_fingerprint.py` fingerprints its meshes.
- `Tools/terrain_check.ps1`, `.py`: checks an area's layout and computed layout before anything is built (feature rules,
  ramp grades, cliff courses of at most 12 m, the seam band, the boundary and the rock past its closed edges, the far
  trees, open ground).
- `Tools/ConceptViewer/`: the island concept viewer, a web page (`README.md`). `web_export.py` and `export_all.sh` export
  the scripted models and the terrain for it; `assemble.py` builds the page from `web/` (engine, procedural kit, the
  four concepts, the interface); `test/` takes screenshots and drives the interface, and `test/dump.js` writes a
  concept's placements as JSON (`Art/Levels/TutorialIsland/crossroads_town.json`, the chosen concept).
- From the cloud session (`Docs/Handoffs/CloudIslandConcepts_2026-10-05.md`, nothing in the game uses them yet):
  `Tools/Blender/style_render.py`, `style_compose.py` and `style_plugins/` render the art-style exploration
  (`Docs/Art/StyleExploration/`, `Docs/Art/REGENERATE_IMAGES.md`); `Tools/Blender/looter_heroes.py` builds the five
  heroes on the UE5 mannequin's bones (`Art/Backlog/Characters/`); `Docs/HudMockup/`, `Docs/EmberDemo/` and
  `Docs/HeroSelect/` are the HUD mockup's, the ember-powers demo's and the hero page's sources.
