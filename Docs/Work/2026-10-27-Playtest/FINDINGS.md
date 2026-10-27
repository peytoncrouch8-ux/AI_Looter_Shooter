# Where each note lives (search agents, 2026-10-27; static reads, not checked in the editor)

SRC = Source/AI_Looter_Shooter, UE = Tools/Unreal. Numbers = NOTES.md items.

- 1 boot logos: nothing exists (no StartupMovies, no Content/Movies). New: MoviePlayer StartupMovies or a Slate splash before the menu.
- 2 menu: backdrop is Lvl_TutorialIsland (= Skyreach's island) via GameDefaultMap ?game=Menu; camera LooterMenuPlayerController.cpp:20-75;
  widgets UI/Menus/MainMenuWidget.cpp (MakeTitle l.150 "AI Looter Shooter" + "Teropa").
- 4 settings: UI/Menus/SettingsMenu*.cpp; one ScrollBox of Graphics/Audio/Interface/Controls (Widget.cpp:198-300); no row help;
  HandleButtonHovered (Nav.cpp:322) only moves focus; StatusText at Widget.cpp:341.
- 5 melee: code-driven, PlayerMeleeComponent/Strike/Motion.cpp, WeaponBaseMelee SetMeleePose; no per-hero tool.
- 6 Odessa Fifty-Two Pickup: Embers/Powers/EmberFiftyTwoPickup.cpp (FlyingCardSize 1.35 l.28, CardRadius 5); EmberFX::DrawCard
  (EmberFX.cpp:102-117) draws two streaks 4.5-9 cm: the "tiny particles". Also used by EmberLuckyStreak.cpp:264.
- 7 grenades (grave-salt jars): Combat/GraveSaltRules.h BaseDamage 900, Radius 400, Core 100; Player/PlayerThrowRules.h start 2, max 3;
  jar 7.5 cm (Art/Models/Throwables/SaltGrenade.py; GraveSaltGrenade.cpp:111-120); FX GraveSaltBurstEffects.cpp:45-83 (no fireball);
  the launcher's FWeaponFX::SpawnExplosion (WeaponFXFire.cpp:161) could be reused.
- 8 inventory pause: only the pause menu pauses (LooterHUD.cpp:417-455). Add SetGamePaused in OpenInventory/OpenInventoryPage/
  CloseInventory (LooterHUD.cpp:222-310); watch page sounds (bPlaysWhilePaused), LoadoutStage preview, world labels.
- 9 poster: layout.json gameplay.posters (l.3096-3114); posterWhitlock on whitlockBarn; amosHouse (-7000,1700) has none.
- 11 apples: baked into SM_Apple_A/B/C, Art/Models/Vegetation/Trees.py hang_apples l.162-173 (r*1.1+0.25 out from the clump, bare
  ball, no stem): float in card gaps since bf9d42ce. Fix offset + stem, models.ps1 -Only Trees, reimport.
- 12 bounty board: UE/build_area_bounties.py place_board l.132 (beside NoticeBoard_Rangers, Main Street). Depot (ATrainStation) at
  layout (465,11150), roost (750,10250).
- 13 Refill All Ammo: UI/Vendor/VendorWidget*.cpp (EEntry l.88, Buy column ~220-233, template MakeSellJunkRow VendorWidgetRows.cpp:237);
  Economy/VendorTrade.cpp:34-67; EconomyRules.cpp:146-170 (one box per click; AmmoPrice caps at a box).
- 14 legendaries: weights WeaponDefinition.cpp:24-30 (60/25/10/4/1); roll Affixes/WeaponRollLibrary.cpp:18-54 (x(1+Luck)^tier);
  rank odds CreatureRankSettings.cpp:117-143 + UE/create_rank_assets.py:27-34; chests ChestKinds.cpp (Strongbox Luck 1 = 15.5%);
  no level gate (LootLibrary.cpp:198-220). Odds table Docs/Story.md "Enemy ranks and legendary drops". Check: Looter.Loot.SimulateDrops.
- 15 stairs: many builders (looter_ruins.py flight 822-860, Lookout.py CliffStairs, Boardwalk.py, looter_rr_town.py:563 building stairs,
  Mine.py manway). No known cause; need which ones (user's shots).
- 16 roads: layout.json roads carve terrain (area_shape.py ROAD_STYLE), thin paint (area_places.py, M_Terrain Road term); no road on
  minimap (MinimapSubsystem.cpp) or map page.
- 17 legendary hitch: LootFanfareSubsystem.cpp:~159 SetGlobalTimeDilation 0.3 for 0.25 s (deliberate slow, reads as a stutter);
  StartShows (LootFanfareShows.cpp:~46-130) NewObject 45 m glow column + new MID + UWidgetComponent label built on landing; first-use
  PSO. Fix: drop the slow, pool/warm the column and label, add a big landing burst.
- 18 Reliquary: AChapelReliquary (World/ChapelReliquary.*), Saint Ada's smashed reliquary, Main 4 step "Look at the Reliquary."
  (UE/create_mission_assets.py:512); nothing says what it is.
- 19 money: UI/HUD/PlayerHUDWidgetMoney.cpp, placed PlayerHUDWidgetLayout.cpp:223-224.
- 20 Knocking Below: Encounter_TillyCellar, UE/build_area_missionprops_valley.py:195-203, 726-741 (+ CellarRats clone,
  build_area_placeholders.py:39); spawner home trace EncounterSpawnerSpots.cpp:19-20,93 (300 up / 2000 down; workshop floor 3.6 m
  over the cellar). Check in the editor.
- 21/34 Gravemother: GravemotherCreature.cpp:44-95 (300 base x12 Soulfed, lvl 6); aggro: build_area_den.py:58-63 MAX_RISE 300,
  CheckHunt OffGround (CreatureBaseHunting.cpp:19), bStartWhenHurt=false. Tombstone exists (AGravemotherStone, 20% of her health,
  GravemotherTypes.h:194) but doesn't gate her damage.
- 21 Abel: Bosses/AbelKeeper.cpp:94-98, AbelRules.h HealthScale 40 (6,400 base), damage 12; phases AbelRules.cpp:124.
- 22 bounties: UE/build_area_bounties.py ARENAS l.54-82: before Main 5 only the Scatter (Unpaid, (-5160,840), 37 m from Pruitt's
  store) and Whitlock's yard (Unpaid). BountyRules.cpp Draw l.302.
- 23 Unpaid AI: UnpaidCreature.cpp:79-118, UnpaidCreatureAttack.cpp (one shriek + straight lunge at the target's current spot),
  UnpaidCreaturePhase.cpp; shared CreatureBaseFight/Pursuit/Spacing.cpp.
- 24 roosts: layout.json gameplay.roosts l.3605-3628: SheriffsOffice (300,800), Depot (750,10250), CaldersPost (16350,-5300).
- 26 chests on map: ChestKinds.cpp bOnMap (Strongbox l.40, SupplyCrate l.148); pins UI/Inventory/MapPins.cpp:37-60. Placements:
  build_area_caches.py, story_texts.py SECRETS, build_area_loot.py, build_area_camps.py.
- 27 action skill anim: UEmberComponent::Fire (EmberComponent.cpp:~264-300) instant; pattern FirstPersonHandsThrow.cpp.
- 28/29 recoil/sway: WeaponBaseKick.cpp:11-46 GetRecoilProfile (Stats.Recoil only; accuracy = Stats.Spread is independent);
  sniper sway WeaponBaseKinds.cpp:374-399 UpdateScopeSway, GunKindsTuning.cpp:200 SwayAt.
- 30 fences: no fences or farmhouse placed on the mountain in layout; fences laid by build_area_dressing.py fence_steps l.1808 (step
  levelling hovers up to bury+hover). Pond: M_Water one-sided (build_world_materials.py:1526-1590), single-layer mesh
  (area_model.py:411-435): seen from under, it's culled. No swimming or underwater code anywhere.
- 31 Back Pay tags: story_texts.py:147-153 'Barricade' readable has text -> ReadableProp::Read opens the card. Clear text/heading.
- 32 Cyrus Teague: the Foreman (AForemanCreature), optional Back Pay after Dead Lift (after Main 5); main path M1-M7, Abel in M6;
  prerequisites UE/create_mission_assets.py:643-759.
- 33 bounty difficulty: BountyRules.cpp MakeGroups l.466, PossePushes l.40, GuardsWithMark l.53, GetTier l.193.
- 35 iron sights: Tools/Blender/looter_guns.py frontier_sight 'Iron' l.458-492 (Rustler, Thresher...); Bullpup.py:586; Drover.py:583-600;
  Ranchhand GhostRing l.745.
- 36 vigil stones: World/BossShrine*.cpp; per-stone record, but a Save & Quit readies every stone at once (BossShrineRules::Evaluate
  l.43 IsLoadedSince, global SessionLoads).
- 37 Lost Loot: ALostPropertyWindow (World/LostPropertyWindow.*), OpenWhen empty (open from the start); build_area_lostloot.py.
- 38 gates: objectives only count on the current step, but mission items/readables are shown and takeable from mission start
  (shown_when empty) and GiveItem records early carries. FStoryCondition (DuringMission/FromStep) on items via build_area_missionprops_*.py.
- 40 bounty chest: BountyBoardSubsystemFight.cpp HandleFightDone ~l.86 SpawnChest; turn-in Collect() BountyBoardSubsystemActions.cpp:201.
- 41 loot names: WeaponManagerPickups.cpp ~213-247 LabelRange 1500; LootLabelLayout::Arrange stacks them.
