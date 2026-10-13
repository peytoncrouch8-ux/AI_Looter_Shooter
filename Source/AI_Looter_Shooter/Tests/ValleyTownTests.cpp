#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractionComponent.h"
#include "Interaction/InteractionTypes.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionEventObjectives.h"
#include "Missions/MissionRunner.h"
#include "Session/CampaignRecord.h"
#include "Tests/MissionTestWorld.h"
#include "World/LockedDoor.h"
#include "World/TownLifeRules.h"
#include "Engine/World.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Package.h"

// The valley's town as wave 4 placed it (2026-10-13): Tilly's cellar door, locked until the cellar key is carried and open
// for good once Knocking Below is done; and the town's lived-in homes of Ransom's Rest's own kit heard through their walls.

using namespace MissionTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLockedDoorTest, "Looter.World.LockedDoor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLockedDoorTest::RunTest(const FString& Parameters)
{
	FCampaignRecord Campaign;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UMissionRunner* Runner = World->GetSubsystem<UMissionRunner>();
	AActor* Player = SpawnMarker(World, FVector::ZeroVector);
	ALockedDoor* Cellar = World->SpawnActor<ALockedDoor>(FVector(150.0, 0.0, 0.0), FRotator::ZeroRotator);
	ALockedDoor* Plain = World->SpawnActor<ALockedDoor>(FVector(150.0, 300.0, 0.0), FRotator::ZeroRotator);
	UInteractionComponent* Interaction = Player ? NewObject<UInteractionComponent>(Player, TEXT("Interaction")) : nullptr;
	if (!Runner || !Player || !Cellar || !Plain || !Interaction)
	{
		AddError(TEXT("The test level isn't whole."));
		return false;
	}
	Interaction->RegisterComponent();

	// Knocking Below, as far as the door cares: a mission running, its key an item, the door open for good once it's done.
	UMissionDefinition* Knocking = NewMission(CreatePackage(nullptr), TEXT("TestKnocking"), EMissionKind::Side, EMissionStart::Automatic, NAME_None);
	AddObjective<UMissionEventObjective>(Knocking, 0)->Event = TEXT("Test.Never");
	Cellar->KeyItem = TEXT("TestCellarKey");
	Cellar->UnlockedWhen.AfterMissions = { FName(TEXT("TestKnocking")) };
	Runner->BeginForTesting({ Knocking }, Campaign, Player);
	Cellar->DispatchBeginPlay();
	Plain->DispatchBeginPlay();
	Runner->Update(0.f);

	TestTrue(TEXT("A door with neither key nor story is a plain door"), !Plain->HasLock() && Plain->IsUnlocked());
	TestTrue(TEXT("The cellar door has a lock"), Cellar->HasLock());
	TestFalse(TEXT("Locked without the key"), Cellar->IsUnlocked());

	const FInteractionOptions Locked = Cellar->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("Locked, it can still be tried with a tap"), Locked.bUsable && Locked.bTap && !Locked.bHold);
	TestTrue(TEXT("...and its prompt says why it won't open"), Locked.TapPrompt.EqualTo(Cellar->LockedPrompt));
	TestFalse(TEXT("A locked try opens nothing"), Cellar->Interact(*Interaction, false));
	TestFalse(TEXT("...the door still shut"), Cellar->IsOn());
	TestEqual(TEXT("...but the latch rattled"), Cellar->GetRattles(), 1);
	TestTrue(TEXT("Tried again at once: no second rattle yet"), Cellar->TryLocked() && Cellar->GetRattles() == 1);

	Campaign.CarryItem(TEXT("TestCellarKey"));
	TestTrue(TEXT("Unlocked with the key in the pocket"), Cellar->IsUnlocked());
	const FInteractionOptions Unlocked = Cellar->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("...its prompt the door's own"), Unlocked.bUsable && Unlocked.TapPrompt.EqualTo(Cellar->Prompt));
	TestTrue(TEXT("...and a tap opens it"), Cellar->Interact(*Interaction, false) && Cellar->IsOn());

	// The key goes as the mission finishes: the door stays open to anyone after it.
	Campaign.RemoveItem(TEXT("TestCellarKey"));
	// (Its swing's cooldown holds it a moment after a use; nothing ticks here, so only the words are asked.)
	TestTrue(TEXT("Open, it can always be shut again, key or none"),
		Cellar->GetInteractionOptions(*Interaction).TapPrompt.EqualTo(Cellar->PromptWhenOn));
	TestFalse(TEXT("Without the key and before the story's end it's locked again"), Cellar->IsUnlocked());
	Campaign.Complete(TEXT("TestKnocking"));
	TestTrue(TEXT("Once Knocking Below is done, it's open to anyone"), Cellar->IsUnlocked());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTownLifeHomesKitTest, "Looter.World.TownLife.HomesKit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FTownLifeHomesKitTest::RunTest(const FString& Parameters)
{
	// Ransom's Rest's own homes (Art/Models/Buildings/Homestead.py): the five built lived-in (lamplit windows, smoke) are
	// somebody's home when their lights name nobody; the five built empty, and every other building, are nobody's.
	const FName Cottage(TEXT("Cottage"));
	static const TCHAR* const LivedIn[] = { TEXT("SM_RR_Home_A"), TEXT("SM_RR_Home_B"), TEXT("SM_RR_Home_D"),
		TEXT("SM_RR_Home_E"), TEXT("SM_RR_Home_H") };
	static const TCHAR* const Nobody[] = { TEXT("SM_RR_Home_C"), TEXT("SM_RR_Home_F"), TEXT("SM_RR_Home_G"),
		TEXT("SM_RR_Home_I"), TEXT("SM_RR_Home_J"), TEXT("SM_RR_Front_Hotel"), TEXT("SM_RR_Depot"), TEXT("SM_RR_Workshop_Bright") };
	for (const TCHAR* Home : LivedIn)
	{
		TestEqual(FString::Printf(TEXT("%s is a lived-in home"), Home), TownLifeRules::HouseholdForMesh(Home), Cottage);
	}
	for (const TCHAR* Empty : Nobody)
	{
		TestTrue(FString::Printf(TEXT("%s has nobody in it"), Empty), TownLifeRules::HouseholdForMesh(Empty).IsNone());
	}
	TestEqual(TEXT("The older cottage still is"), TownLifeRules::HouseholdForMesh(TEXT("SM_Cottage_Ransom")), Cottage);
	return true;
}

#endif
