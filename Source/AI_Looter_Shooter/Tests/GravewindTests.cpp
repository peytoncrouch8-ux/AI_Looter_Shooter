#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Bosses/AbelKeeper.h"
#include "Bosses/BossComponent.h"
#include "Bosses/BossSeal.h"
#include "Combat/HealthComponent.h"
#include "Interaction/InteractionComponent.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionRunner.h"
#include "Scenes/SceneSubsystem.h"
#include "Scenes/SitWithPa.h"
#include "Session/CampaignRecord.h"
#include "Story/AbelOnBoard.h"
#include "Story/HobBird.h"
#include "Story/SpeakerPoint.h"
#include "Story/SpeakerPointComponent.h"
#include "Tests/AbelTestWorld.h"
#include "Tests/GravewindTestWorld.h"
#include "Tests/MissionTestWorld.h"
#include "World/DuskScenery.h"
#include "World/KeeperLanternPost.h"
#include "World/LightingStateSubsystem.h"
#include "World/RespawnMarker.h"
#include "World/StoryLighting.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Level.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// Main 6, "The Gravewind" (Docs/Areas/RansomsRest.md): Delia's word at the door, dusk, the lantern carried to Gravewind
// Point and hung on the keeper's post, Abel's fight, and sitting with Pa; his board after it; the level as built.

using namespace GravewindTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravewindMissionTest, "Looter.Story.Gravewind.Mission",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravewindMissionTest::RunTest(const FString& Parameters)
{
	// Main 6 as its asset has it: id exactly Main6, after Main 5 on Ransom's Rest, started by Delia's word at her door;
	// to Gravewind Point, the lantern hung (a tap on the keeper's post), Abel defeated, the scene; 30% of a level.
	if (FPackageName::DoesPackageExist(TEXT("/Game/Data/Missions/DA_Mission_Main6")))
	{
		const UMissionDefinition* Asset = LoadObject<UMissionDefinition>(nullptr, TEXT("/Game/Data/Missions/DA_Mission_Main6.DA_Mission_Main6"));
		if (TestNotNull(TEXT("DA_Mission_Main6 loads"), Asset))
		{
			TestTrue(TEXT("Main6, a main mission on Ransom's Rest after Main5, started at Delia's door"), Asset->GetMissionId() == MainSix
				&& Asset->Kind == EMissionKind::Main && Asset->Start == EMissionStart::OnEvent && Asset->StartEvent == DeliaEvent
				&& Asset->Area == FName(TEXT("RansomsRest")) && Asset->Prerequisites == TArray<FName>({ MainFive }));
			TestEqual(TEXT("Four steps"), Asset->Steps.Num(), MainSixSteps);
			const UMissionReachObjective* Point = Cast<UMissionReachObjective>(Asset->GetObjective(0, 0));
			const UMissionInteractObjective* Hang = Cast<UMissionInteractObjective>(Asset->GetObjective(1, 0));
			const UMissionKillNamedObjective* Defeat = Cast<UMissionKillNamedObjective>(Asset->GetObjective(2, 0));
			const UMissionSceneObjective* Sit = Cast<UMissionSceneObjective>(Asset->GetObjective(3, 0));
			TestTrue(TEXT("1: to Gravewind Point"), Point && Point->Place.Actor.ActorTag == PointPlace);
			TestTrue(TEXT("2: the lantern hung on the keeper's post, a tap"), Hang && Hang->Target.ActorTag == AKeeperLanternPost::KeepersPostTag && !Hang->bHold);
			TestTrue(TEXT("3: Abel defeated"), Defeat && Defeat->ActorTag == AAbelKeeper::BossTag);
			TestTrue(TEXT("4: sit with Pa"), Sit && Sit->Scene == SitWithPa::SceneName());
			TestEqual(TEXT("Its reward: 30% of a level"), Asset->Rewards.ExperienceShare, 0.3f);
			TestTrue(TEXT("Its steps are the ones Abel's story counts"), AAbelKeeper::HangStep == 1 && AAbelKeeper::FightStep == 2 && AAbelKeeper::SceneStep == 3);
		}
	}
	else
	{
		AddWarning(TEXT("DA_Mission_Main6 isn't made yet: run Tools/Unreal/create_mission_assets.py. The flow below runs on a copy."));
	}
	const TCHAR* const LineSets[] = { TEXT("DA_Lines_DeliaMain6"), TEXT("DA_Lines_HobMain6Way"), TEXT("DA_Lines_HobMain6Post"),
		TEXT("DA_Lines_HobMain6Fight"), TEXT("DA_Lines_HobMain6"), TEXT("DA_Lines_AbelOnBoard") };
	int32 LinesMade = 0;
	for (const TCHAR* Name : LineSets)
	{
		LinesMade += FPackageName::DoesPackageExist(FString(TEXT("/Game/Data/Story/")) + Name) ? 1 : 0;
	}
	if (LinesMade == 0)
	{
		AddWarning(TEXT("Main 6's line sets aren't made yet: run Tools/Unreal/create_story_lines.py."));
	}
	else
	{
		TestEqual(TEXT("Main 6's line sets are made, every one"), LinesMade, static_cast<int32>(UE_ARRAY_COUNT(LineSets)));
	}

	// Played through in a test level, the deck's middle at the origin and its gate 16 m toward -X.
	AbelTestWorld::FScenesSetting ScenesOn(2);
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
	ASpeakerPoint* Door = PlaceDeliasDoor(World, FVector(-5000.0, 300.0, 150.0));
	AActor* Point = MissionTestWorld::SpawnMarker(World, FVector(-1600.0, 0.0, 120.0), PointPlace);
	ARespawnMarker* Grave = World->SpawnActor<ARespawnMarker>(FVector(-2600.0, 0.0, 0.0), FRotator::ZeroRotator);
	TArray<AKeeperLanternPost*> Posts;
	AAbelOnBoard* OnBoard = nullptr;
	AAbelKeeper* Abel = AbelTestWorld::SpawnAbel(World, /*bStory*/ true, &Posts, &OnBoard);
	if (!Runner || !Lighting || !Scenes || !Player || !Interaction || !States || !StoryLight || !Door || !Point || !Grave || !Abel)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	AKeeperLanternPost* KeeperPost = Abel->GetKeepersPost();
	SetUpKeepersPost(*KeeperPost);
	Grave->MarkerId = KeepersGrave;
	Grave->ActiveAfterMission = MainFive;
	Grave->DispatchBeginPlay();
	UPackage* Scratch = CreatePackage(nullptr);
	UMissionDefinition* MainSixMission = MakeMainSix(Scratch);
	Runner->BeginForTesting({ MakeMainFive(Scratch), MainSixMission }, Campaign, Player, Valley);
	StoryLight->DispatchBeginPlay();
	Runner->Update(0.f);

	TestTrue(TEXT("Main 5 first; Main 6 waits for it"), Runner->IsRunning(MainFive) && Runner->GetStatus(*MainSixMission) == EMissionStatus::Locked);
	TestFalse(TEXT("...Abel isn't on the boards in the day"), Abel->IsPresent());
	TestFalse(TEXT("...the keeper's grave isn't open yet"), Grave->IsActive(Campaign));

	Runner->NotifyEvent(FMissionEvent::Named(TEXT("Test.MainFiveDone")));
	TestTrue(TEXT("Main 5 done: Main 6 waits for Delia's word"), Campaign.HasCompleted(MainFive) && !Runner->IsRunning(MainSix));
	TestTrue(TEXT("...the keeper's grave is open (Ellis wakes there if Pa wins)"), Grave->IsActive(Campaign));
	TestTrue(TEXT("...still day"), Lighting->GetState() == ALightingStates::DayState);

	// Delia, through the door: "Take him the lantern..." starts it, and the Rest fades to dusk.
	TestEqual(TEXT("Delia's word"), Door->SpeakerPoint->GetLinesNow().Num() > 0 ? Door->SpeakerPoint->GetLinesNow()[0].Text.ToString() : FString(), FString(DeliaLine));
	TestTrue(TEXT("Talked to at her door"), Door->SpeakerPoint->Talk(Player));
	TestTrue(TEXT("Main 6 starts: carry the lantern to Gravewind Point"), Runner->IsRunning(MainSix) && Runner->GetStep(MainSix) == 0);
	TestTrue(TEXT("...the Rest at dusk"), Lighting->GetState() == Dusk);
	TestTrue(TEXT("...Pa walks the boards"), Abel->IsPresent() && Abel->IsPassive());
	TestTrue(TEXT("...unhurt by anything, his fight not yet to start"), Abel->FindComponentByClass<UHealthComponent>()->bInvulnerable
		&& Abel->GetBoss()->EngageRadius == 0.f && !Abel->GetBoss()->IsFighting());
	TestFalse(TEXT("...nothing to hang yet"), KeeperPost->CanHang());

	// At Gravewind Point: hang the lantern on the keeper's post.
	Player->SetActorLocation(Point->GetActorLocation() + FVector(200.0, 0.0, 0.0));
	Runner->Update(0.2f);
	TestEqual(TEXT("At the Keeper's Gate: hang it on the keeper's post"), Runner->GetStep(MainSix), 1);
	TestTrue(TEXT("...Ellis has the lantern, and the post takes it"), KeeperPost->CanHang());
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
	TestEqual(TEXT("...defeat Abel"), Runner->GetStep(MainSix), 2);
	TestTrue(TEXT("...a player on the deck starts his fight"), Abel->GetBoss()->EngageRadius >= FVector::Dist2D(Player->GetActorLocation(), Abel->GetHome().GetLocation()));

	// His fight (started as a player on the deck starts it; a test level has no player controller to find).
	Abel->GetBoss()->StartFight(Player);
	TestTrue(TEXT("The fight is on, and he can be hurt"), Abel->GetBoss()->IsFighting() && !Abel->FindComponentByClass<UHealthComponent>()->bInvulnerable);
	BossTestWorld::Kill(Abel);
	TestEqual(TEXT("Beaten: sit with Pa"), Runner->GetStep(MainSix), 3);
	TestTrue(TEXT("...he kneels, still there"), Abel->IsPresent() && Abel->GetMove() == EAbelMove::Kneel);
	AbelTestWorld::Run(Abel, 2.2f);
	TestTrue(TEXT("The scene plays"), Scenes->IsPlaying() && Scenes->GetPlayingName() == SitWithPa::SceneName());
	Scenes->SkipScene();
	Runner->Update(0.2f);
	TestTrue(TEXT("Main 6 is finished"), Campaign.HasCompleted(MainSix) && !Runner->IsRunning(MainSix));
	TestTrue(TEXT("...the Keeper's Lantern hangs lit, leaning north-east"), KeeperPost->IsKeepersLanternLit() && KeeperPost->GetLeanDirection().Y > 0.6);
	TestTrue(TEXT("...Pa on his board, the boss gone"), OnBoard->IsShown() && !Abel->IsPresent());
	TestTrue(TEXT("...and it stays dusk"), Lighting->GetState() == Dusk);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravewindFriendTest, "Looter.Story.Gravewind.Friend",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravewindFriendTest::RunTest(const FString& Parameters)
{
	// Pa on his board for the rest of the game: there after Main 6, sat, wearing his things, turning his head to whoever talks.
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	ACharacter* Listener = BossTestWorld::SpawnPlayer(World, AbelTestWorld::Board + FVector(150.0, 250.0, 0.0));
	AAbelOnBoard* OnBoard = AbelTestWorld::PlaceBoard(World);
	if (!Runner || !Listener || !OnBoard)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	OnBoard->SpeakerPoint->Lines = { FStoryLine::Make(FText::GetEmpty(), FText::FromString(TEXT("Sit a while, El."))) };
	UPackage* Scratch = CreatePackage(nullptr);
	Runner->BeginForTesting({ MakeMainFive(Scratch), MakeMainSix(Scratch) }, Campaign, Listener, Valley);
	OnBoard->DispatchBeginPlay();
	TestTrue(TEXT("Shown after Main 6, by the class's own story"), OnBoard->ShownWhen.AfterMissions.Contains(MainSix));
	TestFalse(TEXT("Before Main 6 his board is empty"), OnBoard->IsShown());
	Campaign.Complete(MainFive);
	Campaign.Complete(MainSix);
	OnBoard->RefreshShown();
	TestTrue(TEXT("After it, there he sits"), OnBoard->IsShown());
	TestTrue(TEXT("Tagged for the console and the missions"), OnBoard->ActorHasTag(AAbelOnBoard::SpeakerTag));
	TestTrue(TEXT("His speaker point is at his head, over the board"), OnBoard->SpeakerPoint->GetComponentLocation().Z - OnBoard->GetActorLocation().Z > 60.0);
	TestTrue(TEXT("His ghost light lights his bier and the deck, never him (its own lighting channel)"),
		AbelRules::IsOnGhostChannel(*OnBoard->LanternLight) && !AbelRules::DoesGhostLightReach(*OnBoard->Figure)
		&& !AbelRules::DoesGhostLightReach(*OnBoard->Hat) && !AbelRules::DoesGhostLightReach(*OnBoard->Lantern) && !AbelRules::DoesGhostLightReach(*OnBoard->Pump));

	if (!OnBoard->Figure->GetSkinnedAsset())
	{
		AddWarning(TEXT("SK_Abel isn't imported (or wasn't when the editor started): his seated body isn't checked."));
		return true;
	}
	TestTrue(TEXT("Seated"), OnBoard->IsSeated());
	const FVector Pelvis = OnBoard->Figure->GetBoneLocationByName(TEXT("pelvis"), EBoneSpaces::ComponentSpace);
	TestTrue(FString::Printf(TEXT("...his pelvis a hand over the board (%.1f cm, at size 1)"), Pelvis.Z), FMath::IsNearlyEqual(Pelvis.Z, 10.0, 1.5));
	TestTrue(TEXT("His hat, lantern and pump on their bones"), OnBoard->Hat->GetAttachSocketName() == FName(TEXT("hat"))
		&& OnBoard->Lantern->GetAttachSocketName() == FName(TEXT("lantern")) && OnBoard->Pump->GetAttachSocketName() == FName(TEXT("gun")));

	// Talked to, he turns his head to the listener while his line plays.
	const FQuat Still = OnBoard->Figure->GetBoneRotationByName(TEXT("head"), EBoneSpaces::ComponentSpace).Quaternion();
	TestTrue(TEXT("Talked to"), OnBoard->SpeakerPoint->Talk(Listener));
	for (int32 Step = 0; Step < 20; ++Step)
	{
		OnBoard->UpdatePose(0.05f);
	}
	const FQuat Turned = OnBoard->Figure->GetBoneRotationByName(TEXT("head"), EBoneSpaces::ComponentSpace).Quaternion();
	TestTrue(FString::Printf(TEXT("...he turns his head to them (%.0f degrees)"), FMath::RadiansToDegrees(Still.AngularDistance(Turned))),
		FMath::RadiansToDegrees(Still.AngularDistance(Turned)) > 10.0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGravewindPlacedTest, "Looter.Story.Gravewind.Placed",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGravewindPlacedTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest as built (Tools/Unreal/build_area_deck.py): Abel on the deck facing the sunset, its three lantern posts
	// (one the keeper's), the eight biers, his board, the fog wall across the Keeper's Gate pointed to by his boss, the story's
	// dusk, Gravewind Point's place, the keeper's grave, and Hob's perches in Main 6.
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
	const AAbelOnBoard* OnBoard = nullptr;
	const AStoryLighting* StoryLight = nullptr;
	const AHobBird* Hob = nullptr;
	const ARespawnMarker* Grave = nullptr;
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
		OnBoard = OnBoard ? OnBoard : Cast<AAbelOnBoard>(Actor);
		StoryLight = StoryLight ? StoryLight : Cast<AStoryLighting>(Actor);
		Hob = Hob ? Hob : Cast<AHobBird>(Actor);
		if (const AKeeperLanternPost* Post = Cast<AKeeperLanternPost>(Actor))
		{
			Posts.Add(Post);
		}
		if (const ARespawnMarker* Marker = Cast<ARespawnMarker>(Actor); Marker && Marker->GetMarkerId() == KeepersGrave)
		{
			Grave = Marker;
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
	if (!Abel && Posts.IsEmpty() && !OnBoard)
	{
		AddWarning(TEXT("Main 6's pieces aren't placed yet: build the C++, then run Tools/Unreal/build_area.py RansomsRest gameplay."));
		return true;
	}
	if (TestNotNull(TEXT("Abel on the deck"), Abel))
	{
		TestTrue(TEXT("...tagged for Main 6, during Main 6"), Abel->ActorHasTag(AAbelKeeper::BossTag) && Abel->PresentWhen.DuringMission == MainSix);
		const ABossSeal* Seal = Abel->GetBoss() ? Abel->GetBoss()->Seal.Get() : nullptr;
		TestTrue(TEXT("...his fog wall across the Keeper's Gate"), Seal && Seal->Shape == EBossSealShape::Gate && Seal->IsInside(Abel->GetActorLocation(), 300.f));
		TestEqual(TEXT("...his three lantern posts"), Abel->LanternPosts.Num(), 3);
		TestTrue(TEXT("...his board"), Abel->OnBoard != nullptr);
		TestTrue(TEXT("...the fog's spot out over the canyon, past the open end"), Abel->FogSpot.IsNearlyZero()
			|| FVector::Dist2D(Abel->FogSpot, Abel->GetActorLocation()) > Abel->OpenEndDistance);
	}
	TestEqual(TEXT("Three lantern posts"), Posts.Num(), 3);
	TestEqual(TEXT("...one of them the keeper's"), Posts.FilterByPredicate([](const AKeeperLanternPost* Post) { return Post->bKeepersPost; }).Num(), 1);
	for (const AKeeperLanternPost* Post : Posts)
	{
		if (Post->bKeepersPost)
		{
			TestTrue(TEXT("...its lantern hung in Main 6's second step, lit after it"), Post->HangWhen.DuringMission == MainSix
				&& Post->HangWhen.FromStep == AAbelKeeper::HangStep && Post->LitWhen.AfterMissions.Contains(MainSix));
		}
		TestTrue(TEXT("...each the post's model"), Post->Post->GetStaticMesh() && Post->Post->GetStaticMesh()->GetName() == TEXT("SM_KeeperLanternPost"));
	}
	TestEqual(TEXT("Eight biers on the deck"), Biers, 8);
	TestEqual(TEXT("...each lit by Abel's lantern (his lighting channel; if not, run build_area.py RansomsRest gameplay again)"), BiersLit, Biers);
	if (TestNotNull(TEXT("Pa's board"), OnBoard))
	{
		TestTrue(TEXT("...after Main 6"), OnBoard->ShownWhen.AfterMissions.Contains(MainSix));
	}
	if (TestNotNull(TEXT("The story's dusk"), StoryLight))
	{
		TestTrue(TEXT("...Main 6 at dusk"), StoryLight->Rules.ContainsByPredicate([](const FStoryLightingRule& Rule)
		{
			return Rule.When.DuringMission == MainSix && Rule.State == Dusk;
		}));
	}
	if (TestNotNull(TEXT("The keeper's grave"), Grave))
	{
		TestEqual(TEXT("...open once Main 5 is done"), Grave->ActiveAfterMission, MainFive);
	}
	TestTrue(TEXT("Gravewind Point's place"), bPoint);
	if (TestNotNull(TEXT("Hob"), Hob))
	{
		TestTrue(TEXT("Hob has perches in Main 6"), Hob->Perches.ContainsByPredicate([](const FHobPerch& Perch) { return Perch.When.DuringMission == MainSix; }));
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
