#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Combat/HealthComponent.h"
#include "Components/LightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRank.h"
#include "Creatures/SpiderCreature.h"
#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/EncounterTestWorld.h"
#include "Tests/InteractionTestWorld.h"
#include "Tests/KeepersLanternTestWorld.h"
#include "World/EggSac.h"
#include "World/KeepersLantern.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInterface.h"
#include "Tests/AutomationCommon.h"

// The Sink's props in Main 5, "The Keeper's Lantern" (Docs/Areas/RansomsRest.md): an egg sac shot down (its fall, its burst
// and its two spiders), shootable only in its step and down from the start after it; the Keeper's Lantern dark in the
// webbing, taken in its step and gone for good after. Main 5 itself, the floor's fight and the placed level are
// KeepersLanternTests.cpp's.

using namespace KeepersLanternTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeepersLanternEggSacTest, "Looter.Story.KeepersLantern.EggSac",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKeepersLanternEggSacTest::RunTest(const FString& Parameters)
{
	// A sac hangs 4 m over the floor. Outside Main 5's second step a shot takes nothing from it; in it, a killing shot drops it
	// to the ground under it, where it bursts and lets out two Basic spiders at its burst model's sockets, coming for the
	// player and gone for good once killed; the missions hear it land. It ticks only while it falls. Once the story is past
	// its step it lies burst from the start, empty; when the story goes back before it, it hangs again.
	// Outlives the test level, whose runner counts into it.
	int32 Heard = 0;
	FCampaignRecord Campaign;
	Campaign.ActiveMission = MainFive;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	ACharacter* Player = EncounterTestWorld::SpawnPlayer(World, FVector(700.0, 300.0, 90.0));
	if (!Runner || !Player)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player, Valley);
	Runner->OnEvent.AddLambda([&Heard](const FMissionEvent& Event) { Heard += Event.Name == AEggSac::BurstEvent ? 1 : 0; });

	AEggSac* Sac = SpawnSac(World, FVector(0.0, 0.0, 400.0), 0.0);
	if (!TestNotNull(TEXT("An egg sac"), Sac))
	{
		return false;
	}
	if (!Sac->Sac->GetStaticMesh() || !Sac->Burst->GetStaticMesh())
	{
		AddInfo(TEXT("SM_EggSac_A or SM_EggSac_Burst isn't imported in this checkout: the sac works unseen, its spiders at Sink.py's spots."));
	}
	else if (!Sac->Burst->DoesSocketExist(TEXT("Spawn_1")) || !Sac->Burst->DoesSocketExist(TEXT("Spawn_2")))
	{
		AddWarning(TEXT("SM_EggSac_Burst has no SOCKET_Spawn_1 or _2: its spiders come out at Sink.py's spots instead."));
	}
	const float Full = Sac->Health->GetMaxHealth();

	// Main 5's first step: up out of reach of the story.
	TestTrue(TEXT("Before its step it hangs intact, still"), Sac->GetState() == EEggSacState::Hanging && !Sac->IsActorTickEnabled()
		&& Sac->Sac->IsVisible() && !Sac->Burst->IsVisible());
	TestEqual(TEXT("...a shot takes nothing from it"), Shoot(Sac, 50.f), 0.f);
	TestEqual(TEXT("...its health stays full"), Sac->Health->GetHealth(), Full);
	TestFalse(TEXT("...nor can it be shot down from the console"), Sac->ShootDown(nullptr));
	TestTrue(TEXT("...still hanging"), Sac->GetState() == EEggSacState::Hanging);

	// The sacs' step: shots hurt it, and a killing one drops it.
	Campaign.ActiveMissionStep = 1;
	TestTrue(TEXT("In its step it can be shot"), Sac->CanBeShot());
	Shoot(Sac, 20.f);
	TestTrue(TEXT("...a shot hurts it, and it hangs on"), Sac->Health->GetHealth() < Full && Sac->GetState() == EEggSacState::Hanging);
	Shoot(Sac, Full);
	TestTrue(TEXT("Shot down: it falls, ticking now"), Sac->GetState() == EEggSacState::Falling && Sac->IsActorTickEnabled());
	TestFalse(TEXT("...and can't be shot again"), Sac->CanBeShot());
	const double Before = Sac->Sac->GetComponentLocation().Z;
	{
		FEditorScriptExecutionGuard RunActorEvents;
		Sac->Advance(0.2f);
	}
	TestTrue(TEXT("A fifth of a second in: lower, still falling"), Sac->Sac->GetComponentLocation().Z < Before - 10.0
		&& Sac->GetState() == EEggSacState::Falling && Heard == 0);
	FallAll(Sac);
	TestTrue(TEXT("Landed: burst on the ground, still again"), Sac->GetState() == EEggSacState::Burst && !Sac->IsActorTickEnabled());
	TestTrue(TEXT("...the burst sac shows where it fell, the intact one gone"), Sac->Burst->IsVisible() && !Sac->Sac->IsVisible()
		&& FVector::Dist(Sac->Burst->GetComponentLocation(), FVector::ZeroVector) < 1.0);
	TestEqual(TEXT("...the missions heard it land, once"), Heard, 1);

	// Its spiders: two Basic spiders at the burst sac's spots, coming for the player, gone for good once killed.
	const TArray<ACreatureBase*> Spiders = Sac->GetSpiders();
	const TArray<FTransform> Spots = Sac->GetSpawnSpots();
	TestEqual(TEXT("Two spiders out"), Spiders.Num(), 2);
	TestEqual(TEXT("...at two spots"), Spots.Num(), 2);
	for (int32 Index = 0; Index < Spiders.Num() && Index < Spots.Num(); ++Index)
	{
		const ACreatureBase* Spider = Spiders[Index];
		TestTrue(*FString::Printf(TEXT("Spider %d: a Basic spider"), Index + 1), Spider->IsA<ASpiderCreature>() && Spider->GetRank() == ECreatureRank::Basic);
		TestTrue(*FString::Printf(TEXT("Spider %d: at its spot in front of the mouth"), Index + 1),
			FVector::Dist2D(Spider->GetActorLocation(), Spots[Index].GetLocation()) < 100.0 && Spots[Index].GetLocation().X > 50.0);
		TestTrue(*FString::Printf(TEXT("Spider %d: coming for the player"), Index + 1), Spider->GetTarget() == Player);
		TestFalse(*FString::Printf(TEXT("Spider %d: gone for good once killed"), Index + 1), Spider->WillRespawn());
		TestTrue(*FString::Printf(TEXT("Spider %d: tagged, fighting on the floor"), Index + 1), Spider->ActorHasTag(SacSpiderTag)
			&& Spider->HuntingGround.IsSet() && FMath::IsNearlyEqual(Spider->HuntingGround.MaxRise, FloorRise));
	}

	// Its story: down from the start once the story is past it; up again when the story goes back before it.
	Campaign.ActiveMissionStep = 2;
	AEggSac* Late = SpawnSac(World, FVector(0.0, 3000.0, 400.0), 90.0);
	if (TestNotNull(TEXT("A sac in a level loaded after its step"), Late))
	{
		TestTrue(TEXT("...lies burst from the start"), Late->GetState() == EEggSacState::Burst && Late->Burst->IsVisible() && !Late->Sac->IsVisible());
		TestTrue(TEXT("...on the ground under it, empty, nothing told"), FMath::IsNearlyZero(Late->Burst->GetComponentLocation().Z, 1.0)
			&& Late->GetSpiders().Num() == 0 && Heard == 1);
		TestEqual(TEXT("...a shot takes nothing from it"), Shoot(Late, 50.f), 0.f);
	}
	Campaign.ActiveMissionStep = 1;
	AEggSac* Midway = SpawnSac(World, FVector(0.0, 6000.0, 400.0), 90.0);
	TestTrue(TEXT("A level loaded in the middle of the step: its sacs hang again, to be shot from the start"), Midway
		&& Midway->GetState() == EEggSacState::Hanging && Midway->CanBeShot());
	Campaign.ActiveMissionStep = 0;
	Sac->RefreshStory();
	TestTrue(TEXT("The story back before its step (the console): it hangs again, whole"), Sac->GetState() == EEggSacState::Hanging
		&& Sac->Sac->IsVisible() && !Sac->Burst->IsVisible() && Sac->Health->GetHealth() == Full);
	Campaign.ActiveMission = NAME_None;
	Campaign.CompletedMissions = { MainFive };
	Sac->RefreshStory();
	TestTrue(TEXT("After Main 5: burst on the floor"), Sac->GetState() == EEggSacState::Burst && Sac->GetSpiders().Num() == 2 && Heard == 1);

	// Against a wall: the sling's sac lands out from it, along its front.
	Campaign.CompletedMissions.Reset();
	Campaign.ActiveMission = MainFive;
	Campaign.ActiveMissionStep = 1;
	AEggSac* Slung = SpawnSac(World, FVector(0.0, -3000.0, 280.0), 0.0, 40.f);
	if (TestNotNull(TEXT("A sac in its sling"), Slung))
	{
		TestTrue(TEXT("...shot down from the console"), Slung->ShootDown(nullptr));
		FallAll(Slung);
		TestTrue(TEXT("...it lies 40 cm out along its front, off the wall"), Slung->GetState() == EEggSacState::Burst
			&& FVector::Dist(Slung->Burst->GetComponentLocation(), FVector(40.0, -3000.0, 0.0)) < 1.0);
		TestEqual(TEXT("...its two spiders out"), Slung->GetSpiders().Num(), 2);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FKeepersLanternLanternTest, "Looter.Story.KeepersLantern.Lantern",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FKeepersLanternLanternTest::RunTest(const FString& Parameters)
{
	// The Keeper's Lantern in the webbing: hung by its grip from the snare's cord, dark (no light; its glass the trim's window
	// glass); a tap takes it in Main 5's third step only, and it's gone for good once the story says Ellis has it.
	FCampaignRecord Story;
	TestFalse(TEXT("Before Main 5, Ellis hasn't the lantern"), AKeepersLantern::IsTaken(Story));
	Story.ActiveMission = MainFive;
	Story.ActiveMissionStep = AKeepersLantern::TakeStep;
	TestFalse(TEXT("...nor on its taking step"), AKeepersLantern::IsTaken(Story));
	Story.ActiveMissionStep = AKeepersLantern::TakeStep + 1;
	TestTrue(TEXT("Climbing out, Ellis has it"), AKeepersLantern::IsTaken(Story));
	Story.Complete(MainFive);
	TestTrue(TEXT("...and from Main 5's end on"), AKeepersLantern::IsTaken(Story));
	TestTrue(TEXT("Taken in Main 5, on the step after the egg sacs"), AKeepersLantern::Mission == MainFive && AKeepersLantern::TakeStep == 2);

	FCampaignRecord Campaign;
	Campaign.ActiveMission = MainFive;
	Campaign.ActiveMissionStep = 1;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = InteractionTestWorld::SpawnPlayer(World, Interaction);
	AKeepersLantern* Lantern = PlaceLantern(World, FVector(300.0, 0.0, 120.0), 180.0);
	if (!Runner || !Player || !Interaction || !Lantern)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Runner->BeginForTesting({}, Campaign, Player, Valley);
	Lantern->DispatchBeginPlay();
	MoveLanternTo(Lantern, InteractionTestWorld::Ahead(150.0));

	// Dark: no light at all, and its glass showing the trim's window glass instead of the glow.
	TArray<ULightComponent*> Lights;
	Lantern->GetComponents(Lights);
	TestEqual(TEXT("It has no light"), Lights.Num(), 0);
	const UStaticMesh* Model = Lantern->Lantern->GetStaticMesh();
	if (!Model || !Lantern->DarkGlass)
	{
		AddInfo(TEXT("SM_KeepersLantern or MI_HouseTrim isn't in this checkout: the lantern's glass isn't checked."));
	}
	else
	{
		const int32 Slot = Lantern->FindGlassSlot();
		if (TestTrue(TEXT("Its model has the LanternGlow slot"), Slot != INDEX_NONE))
		{
			TestTrue(TEXT("...dark: the trim's window glass"), Lantern->Lantern->GetMaterial(Slot) == Lantern->DarkGlass
				&& Model->GetMaterial(Slot) != Lantern->DarkGlass);
			Lantern->SetLit(true);
			TestTrue(TEXT("...lit (Main 6's, later): its own glow"), Lantern->Lantern->GetMaterial(Slot) == Model->GetMaterial(Slot));
			Lantern->SetLit(false);
			TestTrue(TEXT("...and dark again"), Lantern->Lantern->GetMaterial(Slot) == Lantern->DarkGlass);
		}
	}
	const UStaticMesh* Tangle = Lantern->Snare->GetStaticMesh();
	if (Model && Tangle && Lantern->Snare->DoesSocketExist(Lantern->HangSocket) && Lantern->Lantern->DoesSocketExist(Lantern->GripSocket))
	{
		TestTrue(TEXT("It hangs by its grip from the snare's cord"), FVector::Dist(Lantern->Lantern->GetSocketLocation(Lantern->GripSocket),
			Lantern->Snare->GetSocketLocation(Lantern->HangSocket)) < 1.0);
	}
	else
	{
		AddInfo(TEXT("SM_Web_Snare's SOCKET_Lantern or SM_KeepersLantern's SOCKET_Grip isn't in this checkout: it hangs where Sink.py's cord ends."));
	}
	TestFalse(TEXT("The snare casts no shadow, and nothing meets it"), Lantern->Snare->CastShadow
		|| Lantern->Snare->GetCollisionEnabled() != ECollisionEnabled::NoCollision);

	// Before its step it's only a lantern in a web.
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Before its step: never offered, not taken"), Interaction->GetFocusedActor() == nullptr && !Lantern->CanTake()
		&& !Lantern->Take(Player) && Lantern->IsHanging());

	// Its step: a tap takes it.
	Campaign.ActiveMissionStep = 2;
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Its step: looked at, a tap, \"Take the Keeper's Lantern\""), Interaction->GetFocusedActor() == Lantern
		&& Interaction->GetFocusedOptions().bTap && !Interaction->GetFocusedOptions().bHold
		&& Interaction->GetFocusedOptions().TapPrompt.ToString() == TEXT("Take the Keeper's Lantern"));
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Tapped: taken from the webbing, for good"), !Lantern->IsHanging() && Lantern->IsUsedUp() && !Lantern->Lantern->IsVisible());
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("...the empty snare stays, offering nothing"), Lantern->Snare->IsVisible() && Interaction->GetFocusedActor() == nullptr);

	// Its story: back in the webbing when the story goes back before its step (the console); gone once Ellis has it.
	Campaign.ActiveMissionStep = 0;
	Lantern->RefreshStory();
	TestTrue(TEXT("The story back before its step: it hangs there again"), Lantern->IsHanging() && Lantern->Lantern->IsVisible());
	Campaign.ActiveMissionStep = 3;
	AKeepersLantern* Later = PlaceLantern(World, FVector(0.0, 3000.0, 120.0), 0.0);
	if (TestNotNull(TEXT("The lantern in a level loaded while climbing out"), Later))
	{
		Later->DispatchBeginPlay();
		TestTrue(TEXT("...is gone from the snare: Ellis has it"), !Later->IsHanging() && !Later->Lantern->IsVisible() && !Later->CanTake());
	}
	Lantern->RefreshStory();
	TestTrue(TEXT("...and so is the first, once the story says so"), !Lantern->IsHanging());
	return true;
}

#endif
