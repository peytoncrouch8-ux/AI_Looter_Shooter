# Code map

One line per file (a `.h`/`.cpp` pair counts as one), grouped by game area: the game module `Source/AI_Looter_Shooter`
first, then the editor-only module `Source/LooterEditor`. Keep it current when you add, move or delete a file. A big
class can spread its `.cpp` over a few files named `ClassTopic.cpp`.

## Module root
- `AI_Looter_Shooter.h/.cpp`: the module, the `LogLooter` log category, and moving Play-In-Editor's Stop key to F10 so
  Escape reaches the game.
- `AI_Looter_Shooter.Build.cs`: module dependencies; builds without unity.

## Core
- `Core/LooterGameMode`: `ALooterGameMode`, the project's default game mode: character, controller and HUD classes.
- `Core/LooterPlayerController`: `ALooterPlayerController`, which adds the always-on input contexts and sets the view's
  look limits.
- `Core/LooterCharacter`: `ALooterCharacter`, the player character: walking, looking and jumping. Its data-only child is
  `/Game/Player/BP_LooterCharacter`.

## Player
- `Player/PlayerLocomotionComponent`: sprint and crouch, and the first-person motion that goes with them.
- `Player/StanceIntent.h`: `FStanceIntent`, what the sprint and crouch keys ask for (hold or toggle, the latest press wins).
- `Player/PlayerViewComponent`: first/third-person camera (F5 cycle), field of view, recoil on the aim, armed body
  animation.
- `Player/PawnInputBinding`: `FPawnInputBinding`, a gameplay component's own input component and mapping context.
- `Player/Animation/LooterCharacterAnimInstance`: parent of the character's Anim Blueprints; layers the procedural stance
  pose and holds guns in the loadout stand-in's hands.

## Combat
- `Combat/HealthComponent`: `UHealthComponent`, health, damage events and floating damage numbers.
- `Combat/CombatRules.h`: `LooterCombat`, game-wide rules (critical hit multiplier, damage variance).
- `Combat/CriticalSpotTarget.h`: `ICriticalSpotTarget`, targets that have a critical spot.
- `Combat/LooterDamageTypes.h`: weapon, critical-hit and creature-attack damage types.
- `Combat/BulletSubsystem`: `UBulletSubsystem`, every bullet in flight: travel, hits, damage, impact effects.
- `Combat/PlayerVitalsSubsystem`: red flash when hurt; fade out and respawn on death.
- `Combat/TargetDummy`: `ATargetDummy`, a training dummy that takes hits and flashes.

## Weapons
- `Weapons/WeaponBase.cpp`: `AWeaponBase` construction, lifecycle, equip and holster, loot state and looks.
- `Weapons/WeaponBaseFiring.cpp`: `AWeaponBase` firing (fire modes, shots, aim point) and the muzzle flash.
- `Weapons/WeaponBaseReload.cpp`: `AWeaponBase` magazine and reload (progress, the moving magazine or pump).
- `Weapons/WeaponBase.h`: the weapon actor's declaration.
- `Weapons/WeaponDefinition`: `UWeaponDefinition`, the data asset for one kind of gun (stats, rarity table, looks).
- `Weapons/WeaponTypes.h`: `EWeaponModel`, `FWeaponStats`, `FWeaponRarityInfo`, `FWeaponInstanceData` (a rolled gun).
- `Weapons/AmmoTypes`: `EAmmoType` and `LooterAmmo`, ammo classes, carry limits and box sizes.
- `Weapons/WeaponModelBuilder`: `WeaponModels`, the procedural rifle and shotgun meshes.
- `Weapons/WeaponRecoil`: `FWeaponRecoil` and `FWeaponRecoilProfile`, spring recoil on the gun and the aim.
- `Weapons/ReloadMotion`: `LooterReload`, the choreography of a reload over its progress.
- `Weapons/WeaponFX`: `FWeaponFX`, code-drawn tracers, impact sparks, dust and chips.

## Inventory
- `Inventory/WeaponManagerComponent.h`: `UWeaponManagerComponent`, the player's weapons, backpack and ammo.
- `Inventory/WeaponManagerComponent.cpp`: its lifecycle, ammo pools, firing passthrough and input.
- `Inventory/WeaponManagerSlots.cpp`: slots and backpack (give, equip, drop, stash, swap, move) and where guns are held.
- `Inventory/WeaponManagerPickups.cpp`: which loot the player is looking at, and picking it up.

## Affixes
- `Affixes/WeaponRollLibrary`: `UWeaponRollLibrary`, rolling rarity and stats from a seed, and spawning rolled guns.

## Loot
- `Loot/LootTable`: `ULootTable`, what a kill drops (ammo boxes, weapon odds, luck).
- `Loot/LootLibrary`: `ULootLibrary`, rolling a loot table and spawning the results.
- `Loot/LootDropComponent`: drops its owner's loot when it dies.
- `Loot/LootTossComponent`: `ULootTossComponent`, throws loot so it pops out, lands and settles.
- `Loot/AmmoPickup`: `AAmmoPickup`, an ammo box you walk over to collect.

## Creatures
- `Creatures/CreatureBase`: `ACreatureBase`, a hostile creature's brain and life cycle (senses, chase, attack, death,
  respawn, steering without a navmesh).
- `Creatures/SpiderCreature`: `ASpiderCreature`, the brown spider: procedural body, leg IK, hit shapes.

## World
- `World/MinimapSubsystem`: `UMinimapSubsystem`, bakes the top-down map picture at runtime.
- `World/FallRecoverySubsystem`: brings the player back when they fall off an island.
- `World/WorldQueries`: `LooterWorld`, trace params that see only real static geometry (skipping volumes).

## Procedural (code-built meshes: the spider, guns, ammo boxes, and the editor's props)
- `Procedural/StylizedMeshKit`: `StylizedMesh`, GeometryScript helpers for building chunky shapes.
- `Procedural/StylizedSurface`: `FStylizedSurface`, `StylizedColors`, `StylizedSurfaces`, how each material slot is painted.

## Settings
- `Settings/KeyBindingSubsystem`: key rebinding, the global pause/inventory actions and the character actions.
- `Settings/GraphicsSettingsSubsystem`: saved display options (quality preset, motion blur, UI transparency, minimap size) and the `Looter.Quality` command.

## UI
- `UI/Style/LooterUIStyle`: `LooterUI`, the style kit every UI is built with (palette, shapes, icons, text, builders,
  transparency).
- `UI/Style/LooterButton`: `ULooterButton`, the kit's button.
- `UI/Style/WeaponText`: `LooterWeaponText`, weapon names, rarity colors and stat strings.
- `UI/HUD/LooterHUD`: `ALooterHUD`, owns the HUD, inventory and pause menu and their hotkeys.
- `UI/HUD/PlayerHUDWidget`: the gameplay HUD (health, ammo, crosshair, hit marker, loot card, messages).
- `UI/HUD/HudMinimapWidget`: the round minimap that turns with the view.
- `UI/Menus/PauseMenuWidget`: the pause and settings menu (graphics, interface, key bindings).
- `UI/Inventory/LoadoutWidget.cpp`: the loadout screen: opening, layout and contents.
- `UI/Inventory/LoadoutWidgetInput.cpp`: its cursor, actions (swap, hold, drop) and turning the stand-in.
- `UI/Inventory/LoadoutWidgetPaint.cpp`: its stand ring and the callouts from slot cards to guns.
- `UI/Inventory/LoadoutWidget.h`: the loadout screen's declaration.
- `UI/Inventory/LoadoutRules`: `LoadoutRules`, the backpack list's compare and sort rules.
- `UI/Inventory/LoadoutParts`: `LoadoutParts`, the screen's page layout, colors, vector art and card builders.
- `UI/Inventory/LoadoutPaintLayer`: `ULoadoutPaintLayer`, a see-through layer the screen draws on.
- `UI/Inventory/LoadoutStage`: `ALoadoutStage`, the off-screen stand-in of the character and its capture.
- `UI/World/WeaponLabelWidget`: the label over loot guns.
- `UI/World/CreatureHealthBarWidget`: the bar over a hurt or hunting creature.
- `UI/World/DamageNumberActor`, `UI/World/DamageNumberWidget`: floating damage numbers.

## Dev
- `Dev/WeaponDevCommands.cpp`: console commands for testing (`Looter.GiveWeapon`).

## Tests (run with `Tools\runtests.ps1`)
- `Tests/AnimationTests.cpp`, `CreatureTests.cpp`, `InventoryTests.cpp`, `LocomotionTests.cpp`, `LootTests.cpp`,
  `MinimapTests.cpp`, `SettingsTests.cpp`, `WeaponTests.cpp`: the `Looter.*` automation tests, one file per area.

## LooterEditor (editor-only module; nothing here ships)
- `LooterEditor.Build.cs`: module dependencies (GeometryScript editor functions, asset tools, FBX import, JSON).
- `LooterEditorModule.cpp`: the module and its console commands: `Looter.BakeLevelProps`, `Looter.ImportModels`,
  `Looter.FixStylizedMaterials`.
- `StylizedProp`: `AStylizedProp` and `EStylizedPropShape`, procedural props for building levels in the editor.
- `PropBaker`: `FPropBaker`, turns a level's props into Nanite static mesh assets, material instances and placed actors.
- `ModelImporter`: `FModelImporter`, imports the Blender models `Tools/models.ps1` exported (fixed FBX settings,
  stylized material instances, hull collision, sockets).
- `SurfaceMaterials`: `SurfaceMaterials`, stylized material instance assets and the parents' usage flags.
- `Tests/StylizedPropTests.cpp`, `ModelImportTests.cpp`, `SurfaceMaterialTests.cpp`: the `Looter.Editor.*` tests (prop
  shapes, the Blender import settings against `Tests/ModelImport/AxisTest`, the stylized materials' usage flags).
