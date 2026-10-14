#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Inventory/WeaponInventorySave.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "CoreGlobals.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "HAL/CriticalSection.h"
#include "Misc/OutputDevice.h"
#include "Misc/ScopeLock.h"
#include "Templates/UnrealTemplate.h"
#include "Tests/AutomationCommon.h"

// The player's guns never vanish unseen (2026-10-14: a play check lost every gun twice with no log line, when an editor
// property change on the moving player reran its Blueprint's construction script and made the weapon manager anew).

namespace GunsKeptTest
{
	/** A pawn holding a weapon manager that has begun play, as the player's has (only such a one ends play when destroyed). */
	UWeaponManagerComponent* MakeHolder(UWorld* World)
	{
		APawn* Holder = World->SpawnActor<APawn>();
		UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>(Holder);
		Inventory->RegisterComponent();
		Inventory->MaxWeapons = 3;
		Holder->DispatchBeginPlay();
		return Inventory;
	}

	AWeaponBase* Give(UWeaponManagerComponent& Inventory, UWeaponDefinition* Definition, EWeaponRarity Rarity)
	{
		return Inventory.GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Definition, Rarity, 1));
	}

	/** LogLooter's "Guns:" lines with their verbosity, taken at once on the thread that writes them (an unbuffered device). */
	class FGunsLines : public FOutputDevice
	{
	public:
		FGunsLines() { GLog->AddOutputDevice(this); }
		virtual ~FGunsLines() override { GLog->RemoveOutputDevice(this); }

		virtual bool CanBeUsedOnMultipleThreads() const override { return true; }

		using FOutputDevice::Serialize;
		virtual void Serialize(const TCHAR* Text, ELogVerbosity::Type Verbosity, const FName& Category) override
		{
			if (Category == LogLooter.GetCategoryName() && FCString::Strstr(Text, TEXT("Guns:")))
			{
				FScopeLock Lock(&Guard);
				Lines.Emplace(static_cast<ELogVerbosity::Type>(Verbosity & ELogVerbosity::VerbosityMask), FString(Text));
			}
		}

		/** How many lines of that verbosity hold the words. */
		int32 Count(ELogVerbosity::Type Verbosity, const TCHAR* Holding) const
		{
			FScopeLock Lock(&Guard);
			return Lines.FilterByPredicate([Verbosity, Holding](const TPair<ELogVerbosity::Type, FString>& Line)
				{
					return Line.Key == Verbosity && Line.Value.Contains(Holding);
				}).Num();
		}

	private:
		mutable FCriticalSection Guard;
		TArray<TPair<ELogVerbosity::Type, FString>> Lines;
	};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunsLostAloudTest, "Looter.Inventory.GunsLostAloud",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunsLostAloudTest::RunTest(const FString& Parameters)
{
	// Guns taken from the player any way but their own are a warning naming how.
	using namespace GunsKeptTest;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}

	// A carried gun destroyed by something else (what its EndPlay tells the holder): said aloud, and its slot lets it go,
	// the other gun coming into hand.
	UWeaponManagerComponent* Inventory = MakeHolder(World);
	AWeaponBase* First = Give(*Inventory, Rifle, EWeaponRarity::Common);
	AWeaponBase* Second = Give(*Inventory, Rifle, EWeaponRarity::Rare);
	if (!TestTrue(TEXT("Two guns given, the first in hand"), First && Second && Inventory->GetActiveWeapon() == First))
	{
		return false;
	}
	AddExpectedMessagePlain(TEXT("a gun destroyed while carried"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	Inventory->NoteGunDestroyed(*First);
	First->Destroy();
	TestTrue(TEXT("Its slot let it go"), Inventory->GetWeapons().Num() == 1 && Inventory->GetWeapons()[0] == Second);
	TestTrue(TEXT("The other gun in hand"), Inventory->GetActiveWeapon() == Second && Inventory->GetActiveSlot() == 0);
	TestEqual(TEXT("One gun carried"), Inventory->NumCarriedGuns(), 1);

	// The weapon manager made anew while its pawn plays on: the 2026-10-14 loss, its guns going with it.
	{
		UWeaponManagerComponent* Rebuilt = MakeHolder(World);
		Give(*Rebuilt, Rifle, EWeaponRarity::Common);
		AddExpectedMessagePlain(TEXT("the pawn's Blueprint rebuilt mid-play"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		TGuardValue<bool> Rebuilding(GIsReconstructingBlueprintInstances, true);
		Rebuilt->DestroyComponent();
	}
	// Destroyed alone any other way.
	{
		UWeaponManagerComponent* Alone = MakeHolder(World);
		Give(*Alone, Rifle, EWeaponRarity::Common);
		AddExpectedMessagePlain(TEXT("the weapon manager destroyed on its own"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
		Alone->DestroyComponent();
	}

	// A load whose gun of a kind that's gone can't come back: the rest come back, the missing one is told.
	UWeaponManagerComponent* Loaded = MakeHolder(World);
	FWeaponInventorySave Save;
	Save.Equipped.Add(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1));
	FWeaponInstanceData Gone = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 1);
	Gone.Definition = nullptr;
	Save.Backpack.Add(Gone);
	AddExpectedMessagePlain(TEXT("1 of the save's 2 guns didn't come back"), ELogVerbosity::Warning, EAutomationExpectedMessageFlags::Contains, 1);
	Loaded->RestoreInventory(Save);
	TestEqual(TEXT("The gun whose kind is there came back"), Loaded->NumCarriedGuns(), 1);

	// A manager with nothing to lose goes without a word (a warning here would fail the test).
	MakeHolder(World)->DestroyComponent();
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunsDroppedQuietlyTest, "Looter.Inventory.GunsDroppedQuietly",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunsDroppedQuietlyTest::RunTest(const FString& Parameters)
{
	// The player's own ways to be left with no gun (a drop, a sale, a clear, a load whose save has none) are a plain log
	// line naming the way, never a warning; moves between the slots and the backpack say nothing.
	using namespace GunsKeptTest;
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = MakeHolder(World);
	const FWeaponInstanceData Packed = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1);
	FGunsLines Lines;

	Give(*Inventory, Rifle, EWeaponRarity::Common);
	TestNotNull(TEXT("The drop key's gun dropped"), Inventory->DropActiveWeapon());

	Give(*Inventory, Rifle, EWeaponRarity::Common);
	Give(*Inventory, Rifle, EWeaponRarity::Rare);
	TestTrue(TEXT("One of two dropped from its slot"), Inventory->DropSlot(0));
	TestEqual(TEXT("...says nothing while one is left"), Lines.Count(ELogVerbosity::Log, TEXT("DropSlot")), 0);
	TestTrue(TEXT("The last dropped from its slot"), Inventory->DropSlot(0));

	FWeaponInstanceData Sold;
	TestTrue(TEXT("A backpack gun sold"), Inventory->AddToBackpack(Packed) && Inventory->TakeFromBackpack(0, Sold));
	TestTrue(TEXT("A backpack gun dropped"), Inventory->AddToBackpack(Packed) && Inventory->DropFromBackpack(0));

	// Moves never leave the player without a gun.
	Give(*Inventory, Rifle, EWeaponRarity::Common);
	TestTrue(TEXT("The only gun stashed"), Inventory->StashSlot(0));
	TestTrue(TEXT("...and taken out again"), Inventory->EquipFromBackpack(0));
	TestEqual(TEXT("Still one gun"), Inventory->NumCarriedGuns(), 1);

	Inventory->ClearInventory();
	Give(*Inventory, Rifle, EWeaponRarity::Common);
	Inventory->RestoreInventory(FWeaponInventorySave());
	TestEqual(TEXT("No guns after a load whose save has none"), Inventory->NumCarriedGuns(), 0);

	for (const TCHAR* Way : { TEXT("DropActiveWeapon"), TEXT("DropSlot"), TEXT("TakeFromBackpack"), TEXT("DropFromBackpack"),
		TEXT("ClearInventory"), TEXT("RestoreInventory") })
	{
		TestEqual(FString::Printf(TEXT("Left with none by %s: one log line naming it"), Way), Lines.Count(ELogVerbosity::Log, Way), 1);
	}
	TestEqual(TEXT("Six ways to none, six lines"), Lines.Count(ELogVerbosity::Log, TEXT("carries none now")), 6);
	TestEqual(TEXT("Never a warning"), Lines.Count(ELogVerbosity::Warning, TEXT("Guns:")), 0);
	return true;
}

#endif
