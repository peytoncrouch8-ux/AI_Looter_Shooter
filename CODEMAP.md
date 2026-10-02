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
- `Combat/PlayerVitalsSubsystem`: red flash when hurt; fade out and respawn on death at the level's own start (never a
  trip's landing).
- `Combat/TargetDummy`: `ATargetDummy`, a training dummy that takes hits and flashes.
- `Combat/EnemyProjectileSubsystem`: `UEnemyProjectileSubsystem`, pellets creatures fire (a boss's spectral buckshot) as
  plain data: volleys and their tell, flight, hits on players' capsules; `EnemyProjectileSubsystemDraw.cpp` draws them on
  instanced meshes.
- `Combat/EnemyShotDamageType.h`: `UEnemyShotDamageType`, a creature's pellet (still a creature's attack).

## Weapons
- `Weapons/WeaponBase.cpp`: `AWeaponBase` construction, lifecycle, equip and holster, loot state and looks.
- `Weapons/WeaponBaseFiring.cpp`: `AWeaponBase` firing (fire modes, shots, aim point) and the muzzle flash.
- `Weapons/WeaponBaseReload.cpp`: `AWeaponBase` magazine and reload (progress, the moving magazine or pump).
- `Weapons/WeaponBase.h`: the weapon actor's declaration.
- `Weapons/WeaponDefinition`: `UWeaponDefinition`, the data asset for one kind of gun (stats, rarity table, parts,
  looks).
- `Weapons/WeaponTypes.h`: `EWeaponKind`, `EWeaponReloadPart`, `FWeaponStats`, `FWeaponRarityInfo`,
  `FWeaponInstanceData` (a rolled gun).
- `Weapons/AmmoTypes`: `EAmmoType` and `LooterAmmo`, ammo classes, carry limits and box sizes.
- `Weapons/WeaponParts`: `FWeaponPartSlot` and `FWeaponPaint` (a gun's part and color options; each part carries stat
  changes and a name word, the game's affixes) and `WeaponParts::Pick`, which picks a rolled gun's parts and colors by
  its seed.
- `Weapons/WeaponModelComponent`: `UWeaponModelComponent`, a rolled gun assembled from its parts (Blender meshes from
  `Art/Models/Weapons`), painted, with its muzzle, grips and moving reload part.
- `Weapons/WeaponRecoil`: `FWeaponRecoil` and `FWeaponRecoilProfile`, spring recoil on the gun and the aim.
- `Weapons/ReloadMotion`: `LooterReload`, the choreography of a reload over its progress.
- `Weapons/WeaponFX`: `FWeaponFX`, code-drawn tracers, impact sparks, dust and chips.

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
  the bell, a lantern post, the skiff's gangplank): its prompt, tap or hold, whether it can be used now, and the use.
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
- `Affixes/WeaponRollLibrary`: `UWeaponRollLibrary`, rolling rarity and stats from a seed, and spawning rolled guns.

## Loot
- `Loot/LootTable`: `ULootTable`, what a kill drops (ammo pickups of 18-36 rounds and their lean toward the kill
  weapon's ammo, weapon odds, luck) and `LooterLoot`, the ammo amounts (a chest's fixed 36).
- `Loot/LootLibrary`: `ULootLibrary`, rolling a loot table (`RollAmmo` then `RollWeaponPicks`, as a kill draws them)
  and spawning the results.
- `Loot/LootOdds`: `LootOdds`, a loot table's odds worked out exactly and counted over many kills from a seed
  (`Looter.Loot.SimulateDrops`, the `Looter.Loot.RankOdds` test).
- `Loot/LootDropComponent`: drops its owner's loot when it dies.
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
  turns that off).
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
- `Creatures/SlimeCreature`: `ASlimeCreature`, the meadow slime: SK_Slime (from `Art/Models/Creatures/Slime.py`) that
  only hops, squashing and stretching on springs, with a leap attack and crits through the gel at its core.
- `Creatures/CreaturePoseAnimInstance`: `UCreaturePoseAnimInstance`, applies the pose a creature's code works out
  (`ACreatureBase::GetBonePose`) to its skeleton.

## Bosses
- `Bosses/BossComponent`: `UBossComponent`, makes a creature a boss: the fight around it (started by a hit, a player near
  its spot or `StartFight`; until then it waits, hunting nobody), its phases, the player's death starting it over, its
  death winning it. `BossComponentPhases.cpp` runs the phases' events (untargetable spells, volleys),
  `BossComponentAdds.cpp` the waves of adds, `BossComponentArena.cpp` the fog wall and the bar.
- `Bosses/BossTypes.h`: a boss fight as data: `FBossPhase` and its events (`FBossPhaseEvent`: `FBossAddWave`,
  `FBossUntargetable`, `FBossVolley`, custom moments).
- `Bosses/BossRules`: `BossRules`, the fight's rules as plain functions (the phase for a share of health, how many adds a
  wave raises, when a spell ends, where adds rise).
- `Bosses/BossSeal`: `ABossSeal`, a fight's fog wall (a ring round the boss's spot, or a placed gate): it stops walking
  pawns only, drawn as rising ghost-light.
- `Bosses/BossTestSpider`: `BossTestSpider`, the test boss (`Looter.Boss.Test` and the boss tests): a big Boss-rank
  spider with a brood at 50%, venom volleys from 25% and a 15 m wall.

## Bestiary
- `Bestiary/BestiaryEntry`: `UBestiaryEntry` and `EBestiaryCategory`, one bestiary page as a data asset (in
  `/Game/Data/Bestiary`): words and stand model, with level, health, attack and experience read from its actor class.

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
- `Missions/MissionCombatObjectives`: kill N (class, tag, zone), kill a named actor, hit N.
- `Missions/MissionEventObjectives`: event, interact or hold, talk at a speaker point, play a scene, board.
- `Missions/MissionPlayerObjectives`: collect (guns carried, items picked up), open an inventory page.
- `Missions/MissionTargets`: `FMissionActorFilter`, `FMissionPlace` and `MissionTargets` (finding an objective's actors:
  nearest living, middle).
- `Missions/MissionRunner`: `UMissionRunner`, each level's mission runner (due missions, steps, objectives, display,
  campaign record). `MissionRunnerFlow.cpp` starts, steps, finishes and rewards; `MissionRunnerEvents.cpp` handles
  deaths and hits (spawned actors too), events and the display.
- `Missions/MissionActorWatch`: `UMissionActorWatch`, one actor's deaths and hits passed to the runner.
- `Missions/MissionRewards`: `MissionRewards`, the experience share, the reward gun with its rarity floor, rewards in
  words.
- `Missions/MissionText`: `MissionText::ResolveKeys`, `{Action}` as the player's bound key.

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
  level (the first level reward), and which kinds the player has met and how many defeated; the session being played
  gives it its progress and saves it.

## Session
- `Session/SessionSubsystem.h`, `Session/SessionSubsystem.cpp`: `USessionSubsystem`, the three save sessions: the main
  menu's summaries, playing, deleting, reading saves (brought up to date), autosaves and save holds, Save & Quit, and
  carrying the old progress save into session 1.
- `Session/SessionSubsystemWorld.cpp`: what a session keeps of the player and each map's world (where the player
  stands, health, guns, loot on the ground, the gun racks, the tutorial's step) and putting it back.
- `Session/SessionSubsystemTravel.cpp`: travel between maps (`TravelToArea`, `TravelToMap`): the world left kept under
  its map, the trip's save, the destination opened with the session, and arriving at the trip's landing.
- `Session/SessionSubsystemWords.cpp`: the session picker's words: play time, when saved, and places by area name or
  level file.
- `Session/SessionSave`: `ULooterSessionSave`, one session's save ("Session1" to "Session3"), version 2: the player, a
  world per map (`FSavedMapWorld`), the campaign record and where a trip arrives; the version 1 upgrade and a trip's
  bookkeeping.
- `Session/CampaignRecord.h`: `FCampaignRecord`, the story so far: missions finished and the one being played, areas
  opened, bosses beaten, the first cast-off, the cold open.
- `Session/SessionSaveGate`: `FSessionSaveGate`, when the session may save: nothing from a trip's save until its
  destination begins; autosaves held during rides and fades.
- `Session/SessionSubsystemPromotions.cpp`: when a map's creatures are promoted on arrival: at most once per 20 minutes
  of play, the time saved with the map's world.

## Areas
- `Areas/AreaDefinition`: `UAreaDefinition`, one area as a data asset in `/Game/Data/Areas` (`DA_Area_<Id>`, made by
  `Tools/Unreal/create_area_assets.py`): its name, level (which may not be built yet), landings, practice flag,
  opening mission, level band and promotion chances; finding areas by name or level.
- `Areas/AreaLandings`: `AreaLandings`, where trips arrive: an actor or player start tagged `Landing_<Place>`, found in
  a level; a level's own start is never one.
- `Areas/AreaRulesSubsystem`: `UAreaRulesSubsystem`, the area being played's rules for its creatures: each one's level
  from the area's band around the player's (bosses not rolled), and placed Basic creatures' promotions rolled as the
  player arrives (at most once per 20 minutes of play per map).

## World
- `World/MinimapSubsystem`: `UMinimapSubsystem`, bakes the top-down map picture at runtime at about a meter per texel,
  trees as crowns.
- `World/MinimapPaint`: `MinimapPaint`, the map picture's rules texel by texel (land, coasts, cliffs, contours; with a
  playable area, the outside dimmed and the closed edges drawn as its boundary line).
- `World/FallRecoverySubsystem`: brings the player back after a fall: off a playable area's open edge after 5 m,
  anywhere after 30 m; `OnRecovered` says who, why and from where.
- `World/FallRecoveryTracker`: `FFallRecoveryTracker`, one player's safe spot (taken only inside the playable area),
  how they left it, and when to bring them back.
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
- `World/PCGGroundFitFilter`: `UPCGGroundFitFilterSettings`, the meadow's PCG node that drops ground cover patches
  hanging off an edge and presses ones floating over a bump into the ground (editor-time; the graph ships with the level).

## Settings
- `Settings/KeyBindingSubsystem`: key rebinding, the global pause/inventory actions and the character actions
  (weapon slots 1-3 among them).
- `Settings/GraphicsSettingsSubsystem`: saved display options (quality preset, motion blur, first-person field of view, UI transparency, minimap on/off, size and zoom, FPS counter) and the `Looter.Quality` and `Looter.FieldOfView` commands.

## UI
- `UI/Style/LooterUIStyle`: `LooterUI`, the style kit every UI is built with (palette, shapes, icons, text, builders,
  transparency).
- `UI/Style/LooterUIInkedIcons.cpp`: the kit's Inked icons (`FInkedIcon`, the weapon and ammo icons) drawn into textures, as
  `Art/Icons/InkedIcons.py` draws its reference pictures.
- `UI/Style/InkedIconData.inl`: every Inked icon as C++ data, generated by `Art/Icons/InkedIcons.py` (don't edit it).
- `UI/Style/LooterButton`: `ULooterButton`, the kit's button.
- `UI/Style/WeaponText`: `LooterWeaponText`, weapon names, rarity colors and stat strings.
- `UI/HUD/LooterHUD`: `ALooterHUD`, owns the HUD, the inventory's pages (loadout, bestiary, missions) and the pause menu (the
  settings menu with Save & Quit), and their hotkeys.
- `UI/HUD/PlayerHUDWidget`: the gameplay HUD (health, ammo, crosshair, hit marker, loot card, interaction prompt,
  messages); `PlayerHUDWidgetPickupCard.cpp` fills the loot comparison card and the interaction prompt.
- `UI/HUD/HudVitalsWidget`: health at the bottom left: a ring with the number inside and a solid bar out of its lower
  side, with a damage chip, a hit flash and a low-health beat.
- `UI/HUD/HudMagazineWidget`: the magazine gauge in the ammo row: a cartridge whose inside drains from the nose as the
  gun fires (the reload bar while reloading), the count inside by the base.
- `UI/HUD/HudMinimapWidget`: the round minimap that turns with the view (size and zoom from the settings), with the
  tracked mission's waypoint on it, or a compass arrow and its distance on the rim.
- `UI/HUD/TutorialPromptWidget`: the tutorial's current instruction near the top of the screen.
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
- `UI/Menus/SettingsMenuWidget`: `USettingsMenuWidget`, the settings menu (graphics, interface, key bindings), over the
  paused game or from the main menu: its layout. `SettingsMenuRows.cpp` makes its rows and key list,
  `SettingsMenuInput.cpp` handles its buttons, sliders and keys, `SettingsMenuParts.h` holds what they share.
- `UI/Menus/MainMenuHUD`: `AMainMenuHUD`, the main menu's HUD: the menu and its settings.
- `UI/Menus/MainMenuWidget`: `UMainMenuWidget`, the main menu (Single Player, Multiplayer, Settings, Quit Game);
  `MainMenuSessions.cpp` is its session picker (each session's area by name) and the delete confirmation.
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
  tabs.
- `UI/Inventory/LoadoutPaintLayer`: `ULoadoutPaintLayer`, a see-through layer the screen draws on.
- `UI/Inventory/LoadoutStage`: `ALoadoutStage`, the off-screen stand-in of the character and its capture.
- `UI/Inventory/StageStudio`: `StageStudio`, what the inventory's off-screen stands share (spot, capture, studio lights,
  picture, projection).
- `UI/Bestiary/BestiaryWidget.cpp`: the bestiary, the inventory's second page: opening, layout and contents.
- `UI/Bestiary/BestiaryWidgetInput.cpp`: its keys, turning the model, and the ring it stands on.
- `UI/Bestiary/BestiaryWidget.h`: the bestiary's declaration.
- `UI/Bestiary/BestiaryStage`: `ABestiaryStage`, the off-screen stand that shows an entry's model, framed to its size.
- `UI/World/WeaponLabelWidget`: the label over loot guns.
- `UI/World/CreatureHealthBarWidget`: the tag over a hurt or hunting creature: floating level, rank word (in its rank's
  color) and name over a slim bar of fixed width, cut into quarters whatever the health.
- `UI/World/DamageNumberActor`, `UI/World/DamageNumberWidget`: floating damage numbers.

## Dev
- `Dev/WeaponDevCommands.cpp`: console commands for testing (`Looter.GiveWeapon`, `Looter.SpawnAmmo`).
- `Dev/ProgressionDevCommands.cpp`: console commands for levels (`Looter.GiveXP`, `Looter.SetLevel`,
  `Looter.ResetProgress`, `Looter.XP.Table`) and the bestiary (`Looter.ForgetBestiary`).
- `Dev/MissionDevCommands.cpp`: `Looter.Mission.Start <id> [step]`, `.Complete <id> [all]`, `.List`,
  `.Event <name> [tag] [hold]`.
- `Dev/SessionDevCommands.cpp`: `Looter.Session.Play <1-3>`, `.Save`, `.Menu`, `.List`, `.CheckSave [1-3 | file]` (a
  save's upgrade checked on a copy), `.Copy <from> <to>` (into an empty slot only), `Looter.Travel <area or level>
  [landing]`, `Looter.Area.List`.
- `Dev/CreatureDevCommands.cpp`: `Looter.CreatureHealth`, gives the nearest creatures chosen health (to compare their
  bars); `Looter.SpawnCreature <kind> [rank] [count] [chase] [size=] [level=]`, spawns ranked creatures in front of the
  player (gone for good once killed).
- `Dev/BossDevCommands.cpp`: `Looter.Boss.Test [phases]`, the test boss in front of the player with its fight started;
  `Looter.Boss.Reset`, `Looter.Boss.Kill`.
- `Dev/LootDevCommands.cpp`: `Looter.Loot.SimulateDrops <rank> [kills]`, rolls a rank's loot table and prints its odds.
- `Dev/InteractionDevCommands.cpp`: `Looter.Interaction.Spawn <door|bell|lantern>`, a greybox interactable in front of
  the player; `Looter.Interaction.Focus`, what the player would use now.
- `Dev/WorldDevCommands.cpp`: `Looter.InstanceCollision`, whether the world's instanced meshes (the scattered trees and
  rocks) have collision bodies; `Looter.World.Bounds`, draws the playable area's boundary and walls;
  `Looter.Perf.HideTag <tag> [1|0]`, hides a tagged group to measure its cost by the difference.
- `Dev/ViewTour`: `UViewTourSubsystem`, `Looter.Tour`: looks from each viewpoint of a level, measures frame times there and takes screenshots (`Tools/tour.ps1`).

## Tests (run with `Tools\runtests.ps1`)
- `Tests/AnimationTests.cpp`, `AreaTests.cpp`, `BestiaryTests.cpp`, `BossTests.cpp`, `BossCombatTests.cpp` (with `BossTestWorld.h`), `CreatureTests.cpp`, `CreatureRankTests.cpp`, `InteractionTests.cpp`, `InteractionPropTests.cpp` (with `InteractionTestWorld.h`), `InventoryTests.cpp`, `LevelBandTests.cpp`, `LocomotionTests.cpp`, `LootTests.cpp`, `LootRankTests.cpp`,
  `MinimapTests.cpp`, `MissionTests.cpp`, `MissionRunnerTests.cpp` (with `MissionTestWorld.h`), `PlayableAreaTests.cpp`, `ProgressionTests.cpp`, `SessionTests.cpp`, `SettingsTests.cpp`, `SlimeTests.cpp`, `TutorialTests.cpp`, `WeaponTests.cpp`,
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
  material instances, hull collision or none (a plant's hulls are all of its collision), sockets, LODs, Nanite
  fallback shares). `ModelImporterMaterials.cpp`
  imports texture sets and makes instances of the textured masters; `ModelImporterRig.cpp` imports rigged models as
  skeletal meshes and turns their hit zones into a physics asset.
- `SurfaceMaterials`: `SurfaceMaterials`, stylized material instance assets and the parents' usage flags.
- `Tests/StylizedPropTests.cpp`, `PropSettlerTests.cpp`, `ModelImportTests.cpp`, `TexturedImportTests.cpp`,
  `RigImportTests.cpp`, `SurfaceMaterialTests.cpp`: the `Looter.Editor.*` tests (prop shapes and ground cover size,
  settling on the ground and the terrain's full fallback, the Blender import settings against
  `Tests/ModelImport/AxisTest`, textured materials, LODs and no-collision against `Tests/TexturedImport/TexturedTest`,
  `Tests/RigImport/RigTest`, the stylized materials' usage flags).

## Terrain generator and level scripts (Python)
- `Art/Levels/area_shape.py`: `Area`, an area's shape from `Art/Levels/<Area>/layout.json`: map square and raster sizes,
  features as lists by type, the height raster and water surface.
- `Art/Levels/area_math.py`: noise, rasters over the map square, curves and distance fields.
- `Art/Levels/area_mesh.py`: the top's triangulation, terrain AO, the island's underside, the water mesh.
- `Art/Levels/area_macro.py`: the macro color map `T_<Area>Macro_BC`. `Art/Levels/area_scatter.py`: the PCG scatter
  mask `T_<Area>Scatter_BC`.
- `Art/Levels/area_computed.py`: `layout_computed.json`: placements at terrain height, cliff groups per feature, bridge,
  waterfall, orchard rows, the squares the maps cover.
- `Art/Levels/area_preview.py`: Blender preview renders and the annotated plan.
- `Art/Levels/area_model.py`: the Blender side (tile, underside and water objects and their materials);
  `Art/Models/Terrain/<Area>.py` wrappers call it.
- `Tools/Unreal/build_area.py`: `AreaBuild`, builds an area's level (`build_area.py <Area> [gameplay]`);
  `build_tutorial_island.py` wraps it for the tutorial island.
- `Tools/Unreal/build_island_scatter.py`: an area's PCG scatter graph and volume (`[Area]`).
  `Tools/Unreal/island_views.py`: an area's viewpoints as editor cameras and shots.
- `Tools/terrain_identity.ps1`, `.py`: checks an area regenerates exactly as committed (headless Blender, no editor);
  `Tools/Blender/terrain_fingerprint.py` fingerprints its meshes.
