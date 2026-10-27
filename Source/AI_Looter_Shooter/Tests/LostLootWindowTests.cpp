#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootStore.h"
#include "Loot/LostLootSubsystem.h"
#include "Story/StoryCondition.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/LostPropertyWindow.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Tests/AutomationCommon.h"

// Lost Property behind its quest (the user's note of 2026-10-27, "There should be a quest to unlock Lost Loot"): while the
// level's window is shut (Mr. Sloane's Lost and Found not turned in) the ground's clock holds, so no gun goes into a window
// nobody can open; once it opens the clock runs as before. A level with no window keeps today's rule
// (LostLootPlayTests.cpp).

namespace
{
	/** The guns lying loose in World. */
	int32 GroundCount(UWorld* World)
	{
		int32 Count = 0;
		for (TActorIterator<AWeaponBase> It(World); It; ++It)
		{
			Count += It->IsPickup() && !It->IsActorBeingDestroyed() ? 1 : 0;
		}
		return Count;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FLostLootShutWindowTest, "Looter.LostLoot.ShutWindow",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FLostLootShutWindowTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper Level;
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("Rifle definition"), Rifle) || !TestTrue(TEXT("Test level made"), Level.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = Level.GetTestWorld();
	ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(World);
	if (!TestNotNull(TEXT("Lost Property in the level"), LostLoot))
	{
		return false;
	}
	TestFalse(TEXT("No window in the level: not shut (the guns go to the session's shelves as always)"),
		ALostPropertyWindow::IsShutIn(World));

	// The depot's window, opening once Lost and Found is turned in (build_area_lostloot.py's open_when).
	ALostPropertyWindow* Window = World->SpawnActor<ALostPropertyWindow>(FVector(0.0, 5000.0, 0.0), FRotator::ZeroRotator);
	if (!TestNotNull(TEXT("A Lost Property window"), Window))
	{
		return false;
	}
	FStoryCondition AfterQuest;
	AfterQuest.AfterMissions = { FName(TEXT("LostAndFound")) };
	Window->OpenWhen = AfterQuest;
	TestFalse(TEXT("Before Lost and Found: the window isn't open"), Window->IsOpen());
	TestTrue(TEXT("...so Lost Property is shut in this level"), ALostPropertyWindow::IsShutIn(World) && LostLoot->IsHeldShut());

	FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 3);
	Gun.Seed = 7;
	AWeaponBase* Lying = UWeaponRollLibrary::SpawnWeapon(World, Gun, FTransform(FVector(3000.0, 0.0, 50.0)));
	if (!TestNotNull(TEXT("A gun lying loose"), Lying))
	{
		return false;
	}
	Lying->Toss(FVector::ZeroVector);
	const int32 OnShelves = LostLoot->GetStore().Num();

	// Twice its five minutes with the window shut: it lies there still, its time unspent.
	LostLoot->AdvanceAt(LostLootRules::GroundLifeSeconds * 2.f, TOptional<FVector>());
	TestEqual(TEXT("Shut: the gun still lies there"), GroundCount(World), 1);
	TestEqual(TEXT("...nothing went on the shelves"), LostLoot->GetStore().Num(), OnShelves);
	TestNearlyEqual(TEXT("...and its five minutes are whole"), LostLoot->GetLifeLeft(*Lying), LostLootRules::GroundLifeSeconds, 0.01f);

	// The window opens (no condition left): the clock runs, and its time up it goes to the shelves.
	Window->OpenWhen = FStoryCondition();
	TestFalse(TEXT("Open: not shut any more"), ALostPropertyWindow::IsShutIn(World) || LostLoot->IsHeldShut());
	LostLoot->AdvanceAt(LostLootRules::GroundLifeSeconds - 1.f, TOptional<FVector>());
	TestEqual(TEXT("Open, a second short: it lies there"), GroundCount(World), 1);
	LostLoot->AdvanceAt(2.f, TOptional<FVector>());
	TestEqual(TEXT("Open, its time up: off the ground"), GroundCount(World), 0);
	TestEqual(TEXT("...and on the shelves"), LostLoot->GetStore().Num(), OnShelves + 1);
	return true;
}

#endif
