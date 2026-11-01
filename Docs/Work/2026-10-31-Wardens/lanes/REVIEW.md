# Lane REVIEW: compile-and-logic review of the C++ (2026-10-31)

Not built (lane rule); every fix is with the Edit tool; public APIs kept (only parameter / local names changed).
Lanes covered: TURNIN, CREW, STORY's C++, SOUND's header, FLIGHT, DREDGER. S = `Source/AI_Looter_Shooter/`.
Evidence that shadowing by a static member function's parameter IS an error here: `Bounties/BountyFight.cpp:44` renamed
`Posting` -> `InPosting` for exactly that reason (its header keeps `Posting`). Locals hiding a data member of the class
(also in static members) are the same C4458.

## Fixed

| # | File:line | What | Fix |
|---|---|---|---|
| 1 | `S/UI/World/MissionOfferWidget.h:245`, `MissionOfferWidgetTurnIn.cpp:189` | static `TurnInAtGiver(..., const UMissionDefinition& Mission, AActor* Giver, ...)`: params hide the widget's members `Mission`, `Giver` | renamed `ReadyMission`, `AtGiver` (header + body); callers unaffected |
| 2 | `S/UI/World/MissionOfferWidgetCard.cpp:411` (`FMissionOfferCard::MakeTurnIn`, static) | local `Objectives` hides `FMissionOfferCard::Objectives` | renamed `StepObjectives` |
| 3 | `S/World/LamplighterShip.cpp` ~376-403 `SetFlight` | `case Breaking: case Landing:` declares a local with no braces, then `case Moored:` (C2360, certain error) | case body braced |
| 4 | `S/World/CompanyTender.h/.cpp` static `SpawnFor(ALamplighterShip& Ship)` | hides member `Ship` | `ForShip`; an unused `FActorSpawnParameters Params` removed |
| 5 | `S/World/LandfallGrapple.h/.cpp` static `Fire(Ship, Tender, ..., BiteLocal)` | all three hide members | `ForShip`, `FromTender`, `InBiteLocal` |
| 6 | `S/Creatures/DredgerCreature.h:51` static `ClassFor(EDredgerRole Role)` | hides member `Role` | `InRole` (the .cpp already used it) |
| 7 | `S/Creatures/DredgerCreature.h:135`, `DredgerCreatureKit.cpp:71` static `FitInBox(const UStaticMesh* Mesh, ...)` | hides ACharacter's `Mesh` | `PartMesh` |
| 8 | `S/Creatures/DredgerRifleman.h` `IsClaimed(const FVector& Spot)` | hides member `Spot` | `Where` (as the .cpp) |
| 9 | `S/Creatures/DredgerHoseComponent.h` `Fire(APawn* Victim)` | hides member `Victim` | `InVictim` (as the .cpp) |
| 10 | `S/Creatures/DredgerTuning.h` `FrostEdge` default 0 | with DREDGER's HUD hook 2 not yet applied, a latch had no screen-edge feedback | default 1 (its README's tunables already said 1; set 0 once the HUD edge is in) |
| 11 | `S/Creatures/DredgerHoseComponent.cpp` `StepLatched`, `TouchVictim` | the pull (`AddMovementInput`) and slow applied to any pawn (Delia: input piles up unspent) | only when `Pawn->IsPlayerControlled()`. Delia is otherwise safe: her `LaunchCharacter` is a flinch, rattle/frost skip non-local players, HUD edge asks the player's pawn only, drain hits invulnerable health, tank splash hits `ACreatureBase` only |

## The STORY hook (TURNIN, critical): verified, no change needed

`USpeakerPointComponent::TalkNow` (SpeakerPointComponent.cpp ~197) takes `GetLinesNow(&TopicIndex)` first, keeps them
when the turn-in's `TurnIn.Lines` is empty (TopicIndex kept, so the topic's Event/SetsFlag fire), and only then sends
`FMissionEvent::Talked` that turns it in. The card path reaches it before any turn-in: `UMissionOfferWidget::TurnIn` ->
`TurnInAtGiver` (checks running + ready) -> `USpeakerPointComponent::TurnInFromCard` -> `TalkNow`; the runner's own
`TurnIn` runs only if the talk somehow left it running. So "during Main1 from_step 9 (with Landfall.AbelsHat)" still holds
when Delia's words are chosen. `Looter.Story.Landfall.Mission` (LandfallStoryTests.cpp) exercises it (no-HUD path).

## Checked and fine (no change)

- TURNIN: every include present (MissionOfferWidgetCard.cpp has NoticeBoard/MissionArtifact/SpeakerPoint/CampaignRecord/
  MissionEventObjectives); APIs exist with these signatures: `UMissionRunner::FindDefinition/IsRunning/IsReadyToTurnIn/
  FindTurnInAt(const AActor*)/TurnIn/CountOptionalDone/GetCampaign`, `FCampaignRecord::HasDoneOptional`,
  `MissionRewards::OptionalBonus`, `LooterEconomy::Format`, `ANoticeBoard::TurnIn(UMissionRunner&, FName) const`,
  `UCaptionSubsystem::Play(const TArray<FStoryLine>&, ECaptionPlay)`, the LooterUI helpers (`IconBrush`, `GlowBrush`,
  `MakeSized(Tree, W, Width, Height=0)`, `MakeSection`, `LoadoutParts::Label`), `FVectorIcon::Strokes/Fills`
  (`TArray<TArray<FVector2D>>`), `UE_DOUBLE_TWO_PI` (UE 5.8 UnrealMathUtility.h:157), `FWidgetTransform` 4-arg ctor,
  `UUserWidget::GetOwningPlayerPawn()`, `ALooterHUD` members used. Locals named `Tick` in a UUserWidget compile already
  (HudBossBarWidgetLayout.cpp). `LooterSoundCue::TurnInStamp` = "UI.TurnIn.Stamp" at the namespace's top level (SOUND),
  in cues.json. Tests `Looter.Missions.TurnInCard.*` macros and names unique.
- SOUND: 30 constants, no duplicate names in `LooterSoundCue`, strings as the contract.
- CREW: graph-less `FWardenCrewAnimInstanceProxy::Evaluate_WithRoot` override is called by UE 5.8's
  `EvaluateAnimation_WithRoot` (AnimInstanceProxy.cpp:1448), so CREW risk 1 is answered (no ref-pose stall).
  `FAnimationPoseData(Pose, Curve, CustomAttributes)`, `FAnimExtractContext(double)`, `GetRootNode`, `GetComponentTransform`,
  `AnimationCore::SolveTwoBoneIK` (AnimationCore is in Build.cs), `FBoneReference`/`FCSPose::LocalBlendCSBoneTransforms`/
  `ConvertComponentPosesToLocalPoses`, `SetSkinnedAssetAndUpdate`, `ShowMaterialSection(..., INDEX_NONE, ...)` (as
  BodyFigureComponent does), `UStaticMeshComponent::GetStaticMesh()` returns TObjectPtr (`.Get()` fine), `LoadObject`
  (FStringView), `FBulletShot`/`FBulletLanding` fields, `OnBulletLanding`, `UHealthComponent::OnDamaged` (5 params, the
  UFUNCTION handler matches), `UEncounterSubsystem::GetTrackedCreatures`, `FWeaponFX::SpawnFlash/SpawnMuzzleBurst`,
  `FVfxMuzzle`, test kit (`PlayerAnimTestKit`, `EncounterTestWorld::SpawnCreature`, public `UBulletSubsystem::Tick`).
  Implicit double->float (NormalizeAxis, Size) compiles in this project already (HobBird.cpp:243, BulletSubsystem.cpp:460).
- STORY: `NewGameIntro.cpp` words/timing only; `PacingPaths.h` `ScriptCreatureOf` (FindObject by /Script path, no Dredger
  include), `MainOneStop(Foes)` both callers updated, no other caller; `LandfallStoryTests.cpp` (Main renamed it from
  SevenDaysTests.cpp) every objective field, `FStoryCondition` field, `FMissionObjectiveView` field and runner call exists.
- FLIGHT: includes, generated.h last, `UTickableWorldSubsystem`, scene/sound/FX/interaction/mission/session APIs, all soft
  paths exist in Content, every cue in LooterSoundCues.h, line set names = STORY's. CREW's API matches (FLIGHT sets the
  public `Crew` before FinishSpawning and calls `Brace`; no `SetCrew`/`BraceAll`/`SetAtStation` calls). No Build.cs change.
- DREDGER: pose/anim, ragdoll (`SetAllPhysicsLinearVelocity`, `AddImpulseToAllBodiesBelow`), `OverlapAnyTestByObjectType`,
  instanced mesh batch calls, mixed float/double `FMath::Clamp` (MIX_FLOATS_3_ARGS), `RemoveAt(.., EAllowShrinking::No)`,
  every ACreatureBase virtual/member used, 9 cues; 6 test classes unique; no Build.cs change.
- No lane includes another lane's unfinished classes except FLIGHT -> DREDGER (below); Delia knows hose-men by class
  name ("HoseMan"), PacingPaths by /Script path.

## DREDGER calls FLIGHT makes (all match the current headers)

| File:line | Call as used | DREDGER header |
|---|---|---|
| `World/LamplighterShipBoarding.cpp:8`, `World/CompanyTender.cpp:8` | `#include "Creatures/DredgerCreature.h"` | exists |
| `World/LamplighterShipBoarding.cpp:51,54,55` | `EDredgerRole::Rifleman`, `::HoseMan` | `DredgerTypes.h:12` UENUM `uint8` |
| `World/LamplighterShipBoarding.cpp:289` | `TSubclassOf<ACreatureBase> Class = ADredgerCreature::ClassFor(static_cast<EDredgerRole>(Role));` | `static TSubclassOf<ADredgerCreature> ClassFor(EDredgerRole InRole)` |
| `World/CompanyTender.cpp:298` | `ADredgerCreature::ClassFor(EDredgerRole::Rifleman)` | same |
| `World/CompanyTender.cpp:316-318` | `Cast<ADredgerCreature>(Rifleman)->SetHoldsGround(true)` | `UFUNCTION void SetHoldsGround(bool bHold)` |

Spawning goes through `ACreatureBase::SpawnAtRuntime` (not DREDGER's). Also matching: `/Script/AI_Looter_Shooter.DredgerRifleman`
/ `.DredgerHoseMan` (PacingPaths.h, `build_area_landfall.py`), Delia's "HoseMan" class-name test.

## Left for Main (with reason)

1. **Lines said twice (decision between lanes):** STORY.md says FLIGHT must not play `DA_Lines_LandfallAnchor`, `Climb`,
   `Point` (Hob's perches say them), but FLIGHT plays `AnchorLines()` in `FinishLanding` (`World/LamplighterShipLanding.cpp`
   ~154-159) and `PointLines()` at Main1 step 5 (`World/LamplighterShipStory.cpp` ~159-163). Drop FLIGHT's two `Say` calls
   or the perches' arrival lines (`story_texts.LANDFALL_PERCHES`); left because it's a story-flow call, not a bug in one file.
2. **Old-save timing (FLIGHT, PIE check):** `ApplyStart` rewrites `Campaign.ActiveMissionStep` (old save -> step 5;
   mid-air save -> 0); it only works if `BeginLandfall` runs before the runner's first update resumes Main1. The arrival path
   (`HandleLevelBegun` -> `BeginIfDue`) should be first; the ship's 1 s self-start fallback would be late. Check once in PIE
   with an old save.
3. **TURNIN's optional board hook** (TURNIN.md hook 2, `UI/World/NoticeBoardWidget.cpp` `HandleOfferClosed`): without it a
   board turn-in through the card says no "turned in. <rewards>" status on the board. Cosmetic; Main's call.
4. **Perf (DREDGER):** `DredgerHoseComponentDraw.cpp` `Draw` builds a ~21-entry `TArray<FTransform>` per frame per visible
   hose-man (contract: no per-frame allocation). Make it a member reused with `Reset()` if perf.ps1 shows it; left (cheap,
   and `AddInstances` takes a default-allocator TArray).
5. **Asset checks in the editor:** `HandGrip_R` is a socket on the `SK_Mannequin` skeleton (not on SKM_Manny_Simple's
   mesh); `DoesSocketExist`/`GetSocketTransform` search skeleton sockets, so Delia's gun, Junie's wrench and the Dredger
   rifle should hang there (a warning logs if not). `SKM_Manny_Simple` references `PA_Mannequin`
   (`Characters/Mannequins/Rigs`): confirm it is the mesh's physics asset (Dredger ragdoll). All anim clips used exist
   (16 `MF_Unarmed_Walk_*/Jog_*`, `MM_Idle`, `MF_Rifle_Idle_ADS`, `MM_Rifle_Reload`).
6. **Ragdoll (DREDGER risk):** the death code's corpse knock moves only the empty capsule once the mesh is detached: see in PIE.
7. **Not compile-checkable:** CREW bone-space signs (lean/sway direction), station hand spots against FLIGHT's props
   (`HandsShift`), stand-in heights; the hose against Delia in a real fight (`EnemyProjectileSubsystem` victims are players
   only: Dredger bullets pass through Delia unless CREW's optional hook 2 is applied).
8. `MissionTurnInCardTests.cpp` is 525 lines (a little over ~500); fine to leave, or split the Board/Artifact tests out.
