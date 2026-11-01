#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelCorruption.h"
#include "Bosses/AbelKeeper.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossSeal.h"
#include "Combat/HealthComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionPlayerObjectives.h"
#include "Missions/MissionRunner.h"
#include "Scenes/FreeAbel.h"
#include "Scenes/SceneSubsystem.h"
#include "Session/CampaignRecord.h"
#include "Story/HobBird.h"
#include "Tests/AbelTestWorld.h"
#include "Tests/GravewindTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/DuskScenery.h"
#include "World/KeeperLanternPost.h"
#include "World/LightingStateSubsystem.h"
#include "World/StoryLighting.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Abel's fight on the burial boards at Gravewind Point, Main 7 "Given to the Wind" since 2026-11-01 (MAINS_2_7.md section 11;
// it was Main 6 "The Gravewind", whose tests these were): the lantern carried there and hung on the keeper's post, Abel
// called up out of the canyon fog, his fight with its call-backs, the last call and the scene, the lantern taken down lit,
// the dusk under thin smog; the level as built; the Gravewind's scenery. (Main 7's asset and words are
// GivenToTheWindTests.cpp's, lane W1; the carry home and the Reliquary are lane W5's.)

using namespace GravewindTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravewindFightTest, "Looter.Story.Gravewind.Fight",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravewindFightTest::RunTest(const FString& Parameters)
{
	// Played through in a test level, the deck's middle at the origin and its gate 16 m toward -X, on a copy of Main 7's
	// fight steps: to the point, the lantern hung, Abel freed, the scene, then the carry.
	AbelTestWorld::FScenesSetting ScenesOn(2);
	AbelTestWorld::FTendrilsOff NoTendrils;
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	ULightingStateSubsystem* Lighting = World->GetSubsystem<ULightingStateSubsystem>();
	USceneSubsystem* Scenes = World->GetSubsystem<USceneSubsystem>();
	ACharacter* Player = BossTestWorld::SpawnPlayer(World, FVector(-5000.0, 0.0, 120.0));
	UInteractionComponent* Interaction = Player ? NewObject<UInteractionComponent>(Player, TEXT("Interaction")) : nullptr;
	if (Interaction)
	{
		Player->AddInstanceComponent(Interaction);
		Interaction->RegisterComponent();
	}
	ALightingStates* States = SpawnDuskLevel(World);
	AStoryLighting* StoryLight = SpawnStoryLighting(World);
	AActor* Point = MissionTestWorld::SpawnMarker(World, FVector(-1600.0, 0.0, 120.0), PointPlace);
	TArray<AKeeperLanternPost*> Posts;
	AAbelKeeper* Abel = AbelTestWorld::SpawnAbel(World, /*bStory*/ true, &Posts);
	if (!Runner || !Lighting || !Scenes || !Player || !Interaction || !States || !StoryLight || !Point || !Abel)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	AKeeperLanternPost* KeeperPost = Abel->GetKeepersPost();
	SetUpKeepersPost(*KeeperPost);
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* MainSevenMission = MakeMainSeven(Scratch);
	Runner->BeginForTesting({ MakeMainSix(Scratch), MainSevenMission }, Campaign, Player, Valley);
	StoryLight->DispatchBeginPlay();
	Runner->Update(0.f);
	// Main 5 carried Abel's lantern up out of the Sink: the post's "Ellis has it" rule reads Main 5 past its taking step.
	Campaign.Complete(TEXT("Main5"));

	TestFalse(TEXT("Before Main 7, Abel isn't on the boards"), Abel->IsPresent());
	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainSixDone")));
	Runner->NotifyEvent(FMissionEvent::Named(StartEvent));
	TestTrue(TEXT("Main 7 starts: to Gravewind Point"), Runner->IsRunning(MainSeven) && Runner->GetStep(MainSeven) == 0);
	TestTrue(TEXT("...the Rest at dusk, under thin smog"), Lighting->GetState() == DuskSmog);
	TestFalse(TEXT("...the boards empty: he's down in the canyon fog"), Abel->IsPresent());

	// At Gravewind Point: hang the lantern on the keeper's post; his own lantern calls him up.
	Player->SetActorLocation(Point->GetActorLocation() + FVector(200.0, 0.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("At the Keeper's Gate: hang it on the keeper's post"), Runner->GetStep(MainSeven), 1);
	TestTrue(TEXT("...Sully has the lantern, and the post takes it"), KeeperPost->CanHang());
	const TOptional<FVector> PostAt = KeeperPost->GetInteractionLocation();
	if (TestTrue(TEXT("The keeper's post can be found"), PostAt.IsSet()))
	{
		Player->SetActorLocationAndRotation(PostAt.GetValue() - FVector(150.0, 0.0, Player->BaseEyeHeight), FRotator::ZeroRotator);
	}
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Looked at: \"Hang the Keeper's Lantern\", a tap"), Interaction->GetFocusedActor() == KeeperPost
		&& Interaction->GetFocusedOptions().bTap && Interaction->GetFocusedOptions().TapPrompt.ToString() == TEXT("Hang the Keeper's Lantern"));
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Hung, dark"), KeeperPost->IsKeepersLanternHung() && !KeeperPost->IsKeepersLanternLit());
	TestEqual(TEXT("...free Abel"), Runner->GetStep(MainSeven), 2);
	TestTrue(TEXT("...he's there now (in a test level he's on the boards at once; in play he comes up out of the fog)"), Abel->IsPresent());
	TestTrue(TEXT("...a player on the deck starts his fight"), Abel->GetBoss()->EngageRadius >= FVector::Dist2D(Player->GetActorLocation(), Abel->GetHome().GetLocation()));

	// His fight: the bar locks at 66% and at 33% until he's called back at the post; his pips fill.
	Abel->GetBoss()->StartFight(Player);
	TestTrue(TEXT("The fight is on, and he can be hurt"), Abel->GetBoss()->IsFighting() && !Abel->FindComponentByClass<UHealthComponent>()->bInvulnerable);
	UAbelCorruptionComponent* Corruption = Abel->GetCorruption();
	BossTestWorld::HurtTo(*Abel->GetBoss(), AbelRules::HoseShare);
	TestTrue(TEXT("At 66%: locked, knelt for the call-back"), Corruption->IsLocked() && Corruption->IsKneeling() && KeeperPost->IsCallBackOpen());
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("...the keeper's post says \"Call him back\", a hold"), Interaction->GetFocusedActor() == KeeperPost
		&& Interaction->GetFocusedOptions().bHold && Interaction->GetFocusedOptions().HoldPrompt.ToString() == TEXT("Call him back"));
	TestTrue(TEXT("Called back: the first pip"), AbelTestWorld::CallBack(Abel, Player) && Corruption->GetPips() == 1);
	TestTrue(TEXT("...the hung lantern warms a third"), FMath::IsNearlyEqual(KeeperPost->GetCallGlow(), 1.f / 3.f, 0.01f));
	// (The fog and its lanterns are AbelFightTests'; here, back on the deck for the last third.)
	for (AKeeperLanternPost* Post : Posts)
	{
		Post->Relight(Player);
	}
	AbelTestWorld::Run(Abel, Abel->DriftSeconds + Abel->DragBackSeconds + 4.f);
	BossTestWorld::KillAdds(*Abel->GetBoss());
	BossTestWorld::HurtTo(*Abel->GetBoss(), AbelRules::KeeperShare);
	TestTrue(TEXT("At 33%: locked again"), Corruption->IsLocked());
	TestTrue(TEXT("Called back: the second pip"), AbelTestWorld::CallBack(Abel, Player) && Corruption->GetPips() == 2);
	TestTrue(TEXT("...and none of the six drunk: they're saved (the optional's event)"), Corruption->NumDrunk() == 0);

	// At zero: the last call; then the scene; the lantern taken down lit; the carry home.
	BossTestWorld::Kill(Abel);
	TestEqual(TEXT("Beaten: call him home"), Runner->GetStep(MainSeven), 3);
	TestTrue(TEXT("...he kneels, still there"), Abel->IsPresent() && Abel->GetMove() == EAbelMove::Kneel);
	AbelTestWorld::Run(Abel, 2.2f);
	TestTrue(TEXT("The last call is open at the post"), Corruption->IsFinalCallOpen() && KeeperPost->IsCallBackOpen() && !Scenes->IsPlaying());
	TestTrue(TEXT("Called home"), AbelTestWorld::CallBack(Abel, Player));
	AbelTestWorld::Run(Abel, AbelTestWorld::Frame);
	TestTrue(TEXT("The scene plays"), Scenes->IsPlaying() && Scenes->GetPlayingName() == FreeAbel::SceneName());
	Scenes->SkipScene();
	Runner->Update(0.2f);
	TestEqual(TEXT("Main 7 goes on: carry him home"), Runner->GetStep(MainSeven), CarryStep);
	TestTrue(TEXT("...the Keeper's Lantern lit and off the hook (carried)"), KeeperPost->IsKeepersLanternLit() && KeeperPost->IsKeepersLanternTaken()
		&& !KeeperPost->KeepersLantern->IsVisible());
	TestTrue(TEXT("...Abel gone from the boards, in his lantern"), !Abel->IsPresent() && Abel->IsEndingDone());
	KeeperPost->RefreshStory();
	TestTrue(TEXT("...and it stays off the hook as the story reads it again"), !KeeperPost->KeepersLantern->IsVisible());
	TestTrue(TEXT("...still dusk under thin smog until Saint Ada wakes"), Lighting->GetState() == DuskSmog);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravewindPlacedTest, "Looter.Story.Gravewind.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravewindPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (Tools/Unreal/build_area_deck.py): Abel on the deck facing the sunset with the six dead's spots,
	// its three lantern posts (one the keeper's, its call-back's post), the eight biers, the fog wall across the Keeper's Gate
	// pointed to by his boss, the story's dusk (DuskSmog in Main 7), Gravewind Point's place, and Hob's perches in Main 7. Pa's
	// board is gone (2026-11-01: Abel is carried home in his lantern).
	if (!FPackageName::DoesPackageExist(TEXT("/Game/Maps/Lvl_RansomsRest")))
	{
		AddInfo(TEXT("Lvl_RansomsRest isn't built: skipped."));
		return true;
	}
	const UWorld* Map = LoadObject<UWorld>(nullptr, TEXT("/Game/Maps/Lvl_RansomsRest.Lvl_RansomsRest"));
	const ULevel* Level = Map ? Map->PersistentLevel.Get() : nullptr;
	if (!TestNotNull(TEXT("Lvl_RansomsRest loads"), Level))
	{
		return false;
	}
	const AAbelKeeper* Abel = nullptr;
	const AStoryLighting* StoryLight = nullptr;
	const AHobBird* Hob = nullptr;
	TArray<const AKeeperLanternPost*> Posts;
	int32 Biers = 0;
	int32 BiersLit = 0;
	int32 Wisps = 0;
	int32 Fogs = 0;
	bool bPoint = false;
	for (const AActor* Actor : Level->Actors)
	{
		if (!Actor)
		{
			continue;
		}
		Abel = Abel ? Abel : Cast<AAbelKeeper>(Actor);
		StoryLight = StoryLight ? StoryLight : Cast<AStoryLighting>(Actor);
		Hob = Hob ? Hob : Cast<AHobBird>(Actor);
		if (const AKeeperLanternPost* Post = Cast<AKeeperLanternPost>(Actor))
		{
			Posts.Add(Post);
		}
		if (const AStaticMeshActor* Placed = Cast<AStaticMeshActor>(Actor))
		{
			const UStaticMesh* Mesh = Placed->GetStaticMeshComponent() ? Placed->GetStaticMeshComponent()->GetStaticMesh() : nullptr;
			const bool bBier = Mesh && Mesh->GetName() == TEXT("SM_Bier");
			Biers += bBier ? 1 : 0;
			BiersLit += bBier && AbelRules::DoesGhostLightReach(*Placed->GetStaticMeshComponent()) ? 1 : 0;
		}
		if (const ADuskScenery* Scenery = Cast<ADuskScenery>(Actor); Scenery && Scenery->Instances->GetStaticMesh())
		{
			const FString MeshName = Scenery->Instances->GetStaticMesh()->GetName();
			Wisps += MeshName.StartsWith(TEXT("SM_GravewindWisp_")) ? Scenery->Instances->GetInstanceCount() : 0;
			Fogs += MeshName.StartsWith(TEXT("SM_CanyonFog_")) ? Scenery->Instances->GetInstanceCount() : 0;
		}
		bPoint |= Actor->ActorHasTag(PointPlace);
	}
	if (!Abel && Posts.IsEmpty())
	{
		AddWarning(TEXT("Main 7's fight isn't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	if (TestNotNull(TEXT("Abel on the deck"), Abel))
	{
		TestTrue(TEXT("...tagged for Main 7, there from its fight step"), Abel->ActorHasTag(AAbelKeeper::BossTag) && Abel->PresentWhen.DuringMission == MainSeven
			&& Abel->PresentWhen.FromStep == AAbelKeeper::FightStep);
		const ABossSeal* Seal = Abel->GetBoss() ? Abel->GetBoss()->Seal.Get() : nullptr;
		TestTrue(TEXT("...his fog wall across the Keeper's Gate"), Seal && Seal->Shape == EBossSealShape::Gate && Seal->IsInside(Abel->GetActorLocation(), 300.f));
		TestEqual(TEXT("...his three lantern posts"), Abel->LanternPosts.Num(), 3);
		TestEqual(TEXT("...the six dead's spots on the boards (build_area_main2.py's shrouds)"), Abel->GetCorruption()->BoardsDead.Num(), 6);
		TestTrue(TEXT("...the fog's spot out over the canyon, past the open end"), Abel->FogSpot.IsNearlyZero()
			|| FVector::Dist2D(Abel->FogSpot, Abel->GetActorLocation()) > Abel->OpenEndDistance);
	}
	TestEqual(TEXT("Three lantern posts"), Posts.Num(), 3);
	TestEqual(TEXT("...one of them the keeper's"), Posts.FilterByPredicate([](const AKeeperLanternPost* Post) { return Post->bKeepersPost; }).Num(), 1);
	for (const AKeeperLanternPost* Post : Posts)
	{
		if (Post->bKeepersPost)
		{
			TestTrue(TEXT("...its lantern hung in Main 7's second step, gone from the hook from its carry step"), Post->HangWhen.DuringMission == MainSeven
				&& Post->HangWhen.FromStep == AAbelKeeper::HangStep && Post->GoneWhen.ContainsByPredicate([](const FStoryCondition& When)
				{
					return When.DuringMission == MainSeven && When.FromStep == CarryStep;
				}));
		}
		TestTrue(TEXT("...each the post's model"), Post->Post->GetStaticMesh() && Post->Post->GetStaticMesh()->GetName() == TEXT("SM_KeeperLanternPost"));
	}
	TestEqual(TEXT("Eight biers on the deck"), Biers, 8);
	TestEqual(TEXT("...each lit by Abel's lantern (his lighting channel; if not, run build_area.py RansomsRest gameplay again)"), BiersLit, Biers);
	if (TestNotNull(TEXT("The story's dusk"), StoryLight))
	{
		TestTrue(TEXT("...Main 7 at dusk under thin smog"), StoryLight->Rules.ContainsByPredicate([](const FStoryLightingRule& Rule)
		{
			return Rule.When.DuringMission == MainSeven && Rule.State == DuskSmog;
		}));
	}
	TestTrue(TEXT("Gravewind Point's place"), bPoint);
	if (TestNotNull(TEXT("Hob"), Hob))
	{
		TestTrue(TEXT("Hob has perches in Main 7"), Hob->Perches.ContainsByPredicate([](const FHobPerch& Perch) { return Perch.When.DuringMission == MainSeven; }));
	}
	if (Wisps == 0 && Fogs == 0)
	{
		AddWarning(TEXT("The Gravewind's wisps and canyon fog aren't placed (their meshes weren't imported when the level was built?)."));
	}
	else
	{
		TestTrue(TEXT("The Gravewind's wisps along the deck and the Rim (25-35)"), Wisps >= 25 && Wisps <= 35);
		TestEqual(TEXT("...and three fog banks off the point"), Fogs, 3);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravewindSceneryTest, "Looter.Story.Gravewind.Scenery",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravewindSceneryTest::RunTest(const FString& Parameters)
{
	// The Gravewind's wisps and the canyon's fog: seen only at dusk, switched by the lighting's word (nothing of them ticks),
	// never in anything's way and never a shadow, each instance gone by itself at 70 m.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	ULightingStateSubsystem* Lighting = World->GetSubsystem<ULightingStateSubsystem>();
	ALightingStates* States = SpawnDuskLevel(World);
	ADuskScenery* Scenery = World->SpawnActor<ADuskScenery>();
	UStaticMesh* Cube = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (!Lighting || !States || !Scenery || !Cube)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	// One plain and one mirrored, as the build script mirrors some wisps.
	Scenery->SetInstances(Cube, { FTransform(FVector(100.0, 0.0, 0.0)), FTransform(FQuat::Identity, FVector(400.0, 0.0, 0.0), FVector(1.0, -1.0, 1.0)) });
	Scenery->DispatchBeginPlay();
	const UInstancedStaticMeshComponent* Instances = Scenery->Instances;
	TestEqual(TEXT("Its instances"), Instances->GetInstanceCount(), 2);
	TestTrue(TEXT("...no collision, no shadow"), Instances->GetCollisionEnabled() == ECollisionEnabled::NoCollision && !Instances->CastShadow);
	TestTrue(TEXT("...each gone at its cull distance"), Instances->InstanceEndCullDistance == static_cast<int32>(Scenery->CullDistance));
	TestFalse(TEXT("...never ticking"), Scenery->PrimaryActorTick.bCanEverTick);
	TestTrue(TEXT("By day: hidden"), Scenery->IsHidden() && !Scenery->IsShownNow());

	TestTrue(TEXT("Dusk switched to"), Lighting->SetState(Dusk, ELightingSwitch::Instant));
	TestTrue(TEXT("At dusk: shown"), !Scenery->IsHidden() && Scenery->IsShownNow());
	TestTrue(TEXT("Day switched back to"), Lighting->SetState(ALightingStates::DayState, ELightingSwitch::Instant));
	TestTrue(TEXT("...hidden again"), Scenery->IsHidden() && !Scenery->IsShownNow());
	return true;
}

#endif
