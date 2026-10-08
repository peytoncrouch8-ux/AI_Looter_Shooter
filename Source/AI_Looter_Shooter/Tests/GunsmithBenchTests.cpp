#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Interaction/InteractionComponent.h"
#include "Inventory/WeaponInventorySave.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Session/SessionSave.h"
#include "Session/SessionSubsystem.h"
#include "Tests/InteractionTestWorld.h"
#include "UI/Bench/BenchRules.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponPartSwap.h"
#include "Weapons/WeaponParts.h"
#include "World/GunsmithBench.h"
#include "World/MinimapSubsystem.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "Tests/AutomationCommon.h"

namespace
{
	const TCHAR* const BenchRiflePath = TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle");
	const TCHAR* const BenchShotgunPath = TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun");

	/** A pawn carrying a weapon manager with three equip slots, in a test level. */
	UWeaponManagerComponent* MakeBenchHolder(UWorld* World)
	{
		APawn* Holder = World->SpawnActor<APawn>();
		UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>(Holder);
		Inventory->RegisterComponent();
		Inventory->MaxWeapons = 3;
		return Inventory;
	}

	/** The gun's parts by key, one per slot, as it's built (its saved keys, or its seed's). */
	TArray<FName> KeysOf(const FWeaponInstanceData& Gun)
	{
		return WeaponParts::PartKeys(WeaponParts::Pick(Gun));
	}

	/** A part of Gun's kind that fits it in place of one it has: the first such, slot by slot. False when none does. */
	bool FindFittingPart(const FWeaponInstanceData& Gun, FBoxedWeaponPart& OutPart, int32& OutSlot)
	{
		if (!Gun.Definition)
		{
			return false;
		}
		for (int32 SlotIndex = 0; SlotIndex < Gun.Definition->Parts.Num(); ++SlotIndex)
		{
			const FWeaponPartSlot& Slot = Gun.Definition->Parts[SlotIndex];
			for (const FWeaponPartOption& Option : Slot.Options)
			{
				FBoxedWeaponPart Part;
				Part.Definition = Gun.Definition;
				Part.Slot = Slot.Name;
				Part.Key = Option.Key;
				if (WeaponPartSwap::CanFit(Gun, Part) == WeaponPartSwap::ECheck::Ok)
				{
					OutPart = Part;
					OutSlot = SlotIndex;
					return true;
				}
			}
		}
		return false;
	}

	/** The slot of the first part a gun could keep when scrapped (none when it has none). */
	FName FirstKeepSlot(const FWeaponInstanceData& Gun)
	{
		const TArray<FBoxedWeaponPart> Choices = WeaponPartSwap::ScrapChoices(Gun);
		return Choices.IsEmpty() ? NAME_None : Choices[0].Slot;
	}

	/** Any part of a kind of gun (its first slot's first option), for filling the box. */
	FBoxedWeaponPart AnyPart(UWeaponDefinition* Kind)
	{
		FBoxedWeaponPart Part;
		if (Kind && !Kind->Parts.IsEmpty() && !Kind->Parts[0].Options.IsEmpty())
		{
			Part.Definition = Kind;
			Part.Slot = Kind->Parts[0].Name;
			Part.Key = Kind->Parts[0].Options[0].Key;
		}
		return Part;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunsmithBenchScrapTest, "Looter.Weapons.Bench.Scrap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunsmithBenchScrapTest::RunTest(const FString& Parameters)
{
	// Scrapping a carried gun keeps the part chosen in the box and the rest of the gun is gone; the player is never left
	// with nothing to shoot.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, BenchRiflePath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = MakeBenchHolder(World);
	const FWeaponInstanceData First = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Legendary, 5);
	const FWeaponInstanceData Second = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 5);
	const FWeaponInstanceData Packed = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Epic, 5);
	if (!TestTrue(TEXT("Two rifles equipped, one in the backpack"), Inventory->GiveWeapon(First) && Inventory->GiveWeapon(Second) && Inventory->AddToBackpack(Packed)))
	{
		return false;
	}
	TestTrue(TEXT("The bench lists the slots' guns, then the backpack's"), BenchRules::CarriedGuns(*Inventory)
		== TArray<FCarriedGun>{ FCarriedGun::Equipped(0), FCarriedGun::Equipped(1), FCarriedGun::InBackpack(0) });

	// The backpack's gun, keeping its last part.
	const TArray<FBoxedWeaponPart> Choices = WeaponPartSwap::ScrapChoices(Inventory->GetBackpack()[0]);
	if (!TestTrue(TEXT("A rolled gun has parts to keep"), Choices.Num() > 0))
	{
		return false;
	}
	const FBoxedWeaponPart Keep = Choices.Last();
	const int32 KeepSlot = Rifle->Parts.IndexOfByPredicate([&Keep](const FWeaponPartSlot& Slot) { return Slot.Name == Keep.Slot; });
	TestTrue(TEXT("It can be scrapped"), Inventory->CanScrap(FCarriedGun::InBackpack(0)));
	TestFalse(TEXT("Not keeping a slot it doesn't have"), Inventory->ScrapGun(FCarriedGun::InBackpack(0), FName(TEXT("NoSuchSlot"))));
	TestEqual(TEXT("...which changes nothing"), Inventory->GetBackpack().Num(), 1);
	TestTrue(TEXT("Scrapped, keeping its part"), Inventory->ScrapGun(FCarriedGun::InBackpack(0), Keep.Slot));
	TestEqual(TEXT("The gun is gone"), Inventory->GetBackpack().Num(), 0);
	const TArray<FBoxedWeaponPart>& Box = Inventory->GetPartsBox();
	TestTrue(TEXT("Its part is in the box"), Box.Num() == 1 && Box[0] == Keep);
	TestTrue(TEXT("...by its kind, slot and key"), Box.Num() == 1 && Box[0].Definition == Rifle && KeepSlot != INDEX_NONE
		&& Box[0].Key == KeysOf(Packed)[KeepSlot]);

	// The gun in hand, with another still equipped: the other is taken in hand.
	TestEqual(TEXT("The first rifle in hand"), Inventory->GetActiveSlot(), 0);
	TestTrue(TEXT("The gun in hand scrapped"), Inventory->ScrapGun(FCarriedGun::Equipped(0), FirstKeepSlot(First)));
	TestEqual(TEXT("One gun left equipped"), Inventory->GetWeapons().Num(), 1);
	TestTrue(TEXT("...and it's in hand"), Inventory->GetActiveWeapon() && Inventory->GetActiveWeapon()->GetRarity() == EWeaponRarity::Rare);
	TestEqual(TEXT("Two parts in the box"), Inventory->GetPartsBox().Num(), 2);

	// Never the last gun carried, nor the last one in the slots.
	FText Why;
	TestFalse(TEXT("The only gun can't be scrapped"), Inventory->CanScrap(FCarriedGun::Equipped(0), &Why));
	TestFalse(TEXT("...and it says why"), Why.IsEmpty());
	TestFalse(TEXT("...nor scrapped anyway"), Inventory->ScrapGun(FCarriedGun::Equipped(0), FirstKeepSlot(Second)));
	TestEqual(TEXT("...so it's still carried"), Inventory->GetWeapons().Num(), 1);
	TestTrue(TEXT("Another in the backpack"), Inventory->AddToBackpack(Packed));
	TestFalse(TEXT("The last gun in the slots still can't be"), Inventory->CanScrap(FCarriedGun::Equipped(0)));
	TestTrue(TEXT("...though the backpack's can"), Inventory->CanScrap(FCarriedGun::InBackpack(0)));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunsmithBenchFitTest, "Looter.Weapons.Bench.Fit",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunsmithBenchFitTest::RunTest(const FString& Parameters)
{
	// Fitting a box part onto a carried gun swaps one part, keeps everything else about the gun, and puts the part that
	// came off in the box where the fitted one was; the gun in hand is rebuilt and stays in hand.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, BenchRiflePath);
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, BenchShotgunPath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = MakeBenchHolder(World);
	if (!TestTrue(TEXT("Two rifles equipped"), Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Legendary, 7))
		&& Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 7))))
	{
		return false;
	}
	const FWeaponInstanceData Before = Inventory->GetActiveWeapon()->GetInstance();
	FBoxedWeaponPart Part;
	int32 SlotIndex = INDEX_NONE;
	if (!TestTrue(TEXT("A part that fits the legendary"), FindFittingPart(Before, Part, SlotIndex)))
	{
		return false;
	}
	const TArray<FName> KeysBefore = KeysOf(Before);
	TestTrue(TEXT("Into the box"), Inventory->AddToPartsBox(Part));
	TestTrue(TEXT("It fits"), Inventory->CheckFit(FCarriedGun::Equipped(0), 0) == WeaponPartSwap::ECheck::Ok);
	TestTrue(TEXT("Fitted"), Inventory->FitPart(FCarriedGun::Equipped(0), 0));

	const AWeaponBase* InHand = Inventory->GetActiveWeapon();
	if (!TestTrue(TEXT("The gun is still in hand, in its slot"), InHand && Inventory->GetActiveSlot() == 0 && Inventory->GetWeapons().Num() == 2))
	{
		return false;
	}
	const FWeaponInstanceData& After = InHand->GetInstance();
	const TArray<FName> KeysAfter = KeysOf(After);
	TestTrue(TEXT("The slot has the new part"), KeysAfter.IsValidIndex(SlotIndex) && KeysAfter[SlotIndex] == Part.Key);
	bool bOthersKept = KeysAfter.Num() == KeysBefore.Num();
	for (int32 Index = 0; bOthersKept && Index < KeysAfter.Num(); ++Index)
	{
		bOthersKept = Index == SlotIndex || KeysAfter[Index] == KeysBefore[Index];
	}
	TestTrue(TEXT("...and only that slot changed"), bOthersKept);
	TestTrue(TEXT("Its seed, rarity and level kept"), After.Seed == Before.Seed && After.Rarity == Before.Rarity && After.Level == Before.Level);
	TestTrue(TEXT("Its magazine fits its new size"), InHand->GetCurrentMagazine() <= InHand->GetStats().MagazineSize);
	const TArray<FBoxedWeaponPart>& Box = Inventory->GetPartsBox();
	if (KeysBefore[SlotIndex].IsNone())
	{
		TestEqual(TEXT("An empty slot gave nothing back"), Box.Num(), 0);
	}
	else
	{
		TestTrue(TEXT("The old part is in the box where the new one was"), Box.Num() == 1 && Box[0].Definition == Rifle && Box[0].Slot == Part.Slot
			&& Box[0].Key == KeysBefore[SlotIndex]);
		// Fitting it again puts the gun back as it was.
		TestTrue(TEXT("Fitted back"), Inventory->FitPart(FCarriedGun::Equipped(0), 0));
		TestTrue(TEXT("...as it was"), Inventory->GetActiveWeapon() && KeysOf(Inventory->GetActiveWeapon()->GetInstance()) == KeysBefore);
		TestTrue(TEXT("...and the new part is in the box again"), Inventory->GetPartsBox().Num() == 1 && Inventory->GetPartsBox()[0] == Part);
	}

	// Another kind's part is refused, and nothing changes.
	Inventory->ClearPartsBox();
	const FBoxedWeaponPart Wrong = AnyPart(Shotgun);
	TestTrue(TEXT("A shotgun part in the box"), Inventory->AddToPartsBox(Wrong));
	TestTrue(TEXT("It's the wrong kind for a rifle"), Inventory->CheckFit(FCarriedGun::Equipped(0), 0) == WeaponPartSwap::ECheck::WrongKind);
	const TArray<FName> KeysUntouched = KeysOf(Inventory->GetActiveWeapon()->GetInstance());
	TestFalse(TEXT("...so it isn't fitted"), Inventory->FitPart(FCarriedGun::Equipped(0), 0));
	TestTrue(TEXT("...and the box and the gun are as they were"), Inventory->GetPartsBox().Num() == 1 && Inventory->GetPartsBox()[0] == Wrong
		&& KeysOf(Inventory->GetActiveWeapon()->GetInstance()) == KeysUntouched);

	// A backpack gun is refitted where it lies.
	Inventory->ClearPartsBox();
	if (!TestTrue(TEXT("A rifle in the backpack"), Inventory->AddToBackpack(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Legendary, 3))))
	{
		return false;
	}
	FBoxedWeaponPart PackPart;
	int32 PackSlot = INDEX_NONE;
	if (TestTrue(TEXT("A part that fits it"), FindFittingPart(Inventory->GetBackpack()[0], PackPart, PackSlot)))
	{
		Inventory->AddToPartsBox(PackPart);
		TestTrue(TEXT("Fitted in the backpack"), Inventory->FitPart(FCarriedGun::InBackpack(0), 0));
		const FWeaponInstanceData& Packed = Inventory->GetBackpack()[0];
		TestTrue(TEXT("...with its new part"), KeysOf(Packed)[PackSlot] == PackPart.Key);
		TestTrue(TEXT("...its magazine no fuller than its new size"), Packed.SavedMagazine <= Packed.Stats.MagazineSize);
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunsmithBenchSaveTest, "Looter.Weapons.Bench.Save",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunsmithBenchSaveTest::RunTest(const FString& Parameters)
{
	// The parts box goes through a session's save and back, part for part; a session saved before the bench reads it empty.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, BenchRiflePath);
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, BenchShotgunPath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = MakeBenchHolder(World);
	Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Rare, 4));
	const FWeaponInstanceData Scrapped = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Epic, 4);
	TArray<FBoxedWeaponPart> Parts = WeaponPartSwap::ScrapChoices(Scrapped);
	Parts.Add(AnyPart(Shotgun));
	for (const FBoxedWeaponPart& Part : Parts)
	{
		Inventory->AddToPartsBox(Part);
	}
	if (!TestEqual(TEXT("The box holds them all"), Inventory->GetPartsBox().Num(), Parts.Num()))
	{
		return false;
	}

	ULooterSessionSave* Save = NewObject<ULooterSessionSave>();
	Save->Version = ULooterSessionSave::CurrentVersion;
	Save->bHasInventory = true;
	Inventory->SaveInventory(Save->Inventory);
	TestTrue(TEXT("Saved with the inventory"), Save->Inventory.PartsBox == Parts);
	TArray<uint8> Bytes;
	const ULooterSessionSave* Read = UGameplayStatics::SaveGameToMemory(Save, Bytes) ? USessionSubsystem::ReadSave(Bytes) : nullptr;
	if (!TestNotNull(TEXT("Written and read back"), Read))
	{
		return false;
	}
	TestTrue(TEXT("Read back part for part"), Read->Inventory.PartsBox == Parts);

	UWeaponManagerComponent* Restored = MakeBenchHolder(World);
	Restored->RestoreInventory(Read->Inventory);
	TestTrue(TEXT("Restored into the player's box"), Restored->GetPartsBox() == Parts);

	// A session from before the bench has no box to read: it starts empty.
	Restored->RestoreInventory(FWeaponInventorySave());
	TestEqual(TEXT("An old session's box is empty"), Restored->GetPartsBox().Num(), 0);
	Inventory->ClearInventory();
	TestEqual(TEXT("Emptying the inventory empties the box"), Inventory->GetPartsBox().Num(), 0);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunsmithBenchCapTest, "Looter.Weapons.Bench.Cap",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunsmithBenchCapTest::RunTest(const FString& Parameters)
{
	// The box holds at most 100 parts: scrapping stops there, fitting (one out, one in) still works, and throwing a part
	// out makes room.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, BenchRiflePath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = MakeBenchHolder(World);
	if (!TestTrue(TEXT("Two rifles equipped"), Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Legendary, 2))
		&& Inventory->GiveWeapon(UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Uncommon, 2))))
	{
		return false;
	}
	TestEqual(TEXT("The cap"), UWeaponManagerComponent::MaxBoxedParts, 100);
	TestFalse(TEXT("A part of no kind is refused"), Inventory->AddToPartsBox(FBoxedWeaponPart()));

	FBoxedWeaponPart Fitting;
	int32 FittingSlot = INDEX_NONE;
	if (!TestTrue(TEXT("A part that fits the gun in hand"), FindFittingPart(Inventory->GetActiveWeapon()->GetInstance(), Fitting, FittingSlot)))
	{
		return false;
	}
	const bool bSlotFilled = !KeysOf(Inventory->GetActiveWeapon()->GetInstance())[FittingSlot].IsNone();
	TestTrue(TEXT("It goes in first"), Inventory->AddToPartsBox(Fitting));
	const FBoxedWeaponPart Filler = AnyPart(Rifle);
	while (Inventory->GetPartsBox().Num() < UWeaponManagerComponent::MaxBoxedParts && Inventory->AddToPartsBox(Filler))
	{
	}
	TestEqual(TEXT("Filled to the cap"), Inventory->GetPartsBox().Num(), UWeaponManagerComponent::MaxBoxedParts);
	TestFalse(TEXT("No more go in"), Inventory->AddToPartsBox(Filler));
	TestEqual(TEXT("...still at the cap"), Inventory->GetPartsBox().Num(), UWeaponManagerComponent::MaxBoxedParts);

	FText Why;
	TestFalse(TEXT("A full box stops scrapping"), Inventory->CanScrap(FCarriedGun::Equipped(1), &Why));
	TestFalse(TEXT("...and says why"), Why.IsEmpty());
	TestFalse(TEXT("...so the gun isn't scrapped"), Inventory->ScrapGun(FCarriedGun::Equipped(1), FirstKeepSlot(Inventory->GetWeapons()[1]->GetInstance())));
	TestEqual(TEXT("...and is still carried"), Inventory->GetWeapons().Num(), 2);

	TestTrue(TEXT("A full box still fits parts"), Inventory->FitPart(FCarriedGun::Equipped(0), 0));
	TestEqual(TEXT("...one out, one in"), Inventory->GetPartsBox().Num(), UWeaponManagerComponent::MaxBoxedParts - (bSlotFilled ? 0 : 1));

	TestTrue(TEXT("A part thrown out"), Inventory->DiscardPart(Inventory->GetPartsBox().Num() - 1));
	TestTrue(TEXT("...makes room to scrap again"), Inventory->CanScrap(FCarriedGun::Equipped(1)));
	TestFalse(TEXT("Nothing to throw out past the end"), Inventory->DiscardPart(UWeaponManagerComponent::MaxBoxedParts));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunsmithBenchNamedTest, "Looter.Weapons.Bench.Named",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunsmithBenchNamedTest::RunTest(const FString& Parameters)
{
	// A named gun (Heirloom) keeps its own parts: it can't be scrapped, and nothing is fitted to it, even a part that fits
	// a rolled gun of its kind.
	FTestWorldWrapper WorldWrapper;
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UNamedWeaponDefinition* Heirloom = UNamedWeaponDefinition::FindByName(TEXT("Heirloom"));
	if (!Heirloom)
	{
		AddError(TEXT("DA_Named_Heirloom isn't in /Game/Data/Weapons: run Tools/Unreal/create_named_weapons.py in the editor."));
		return false;
	}
	UWeaponDefinition* Shotgun = Heirloom->Weapon.Get();
	if (!TestNotNull(TEXT("Its kind"), Shotgun))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = MakeBenchHolder(World);
	const FWeaponInstanceData Named = Heirloom->MakeInstance(5);
	const FWeaponInstanceData Rolled = UWeaponRollLibrary::RollWeaponWithRarity(Shotgun, EWeaponRarity::Legendary, 5);
	if (!TestTrue(TEXT("Heirloom and a rolled shotgun equipped"), Inventory->GiveWeapon(Named) && Inventory->GiveWeapon(Rolled)))
	{
		return false;
	}
	TestFalse(TEXT("Its parts can't be changed"), WeaponPartSwap::CanModify(Named));
	TestEqual(TEXT("...so it has nothing to keep"), WeaponPartSwap::ScrapChoices(Named).Num(), 0);

	FText Why;
	TestFalse(TEXT("It can't be scrapped"), Inventory->CanScrap(FCarriedGun::Equipped(0), &Why));
	TestFalse(TEXT("...and the bench says why"), Why.IsEmpty());
	TestFalse(TEXT("...nor scrapped anyway"), Inventory->ScrapGun(FCarriedGun::Equipped(0), FirstKeepSlot(Rolled)));
	TestEqual(TEXT("...so it's still carried"), Inventory->GetWeapons().Num(), 2);

	FBoxedWeaponPart Part;
	int32 SlotIndex = INDEX_NONE;
	if (TestTrue(TEXT("A part that fits the rolled shotgun"), FindFittingPart(Rolled, Part, SlotIndex)))
	{
		Inventory->AddToPartsBox(Part);
		const TArray<FName> KeysBefore = KeysOf(Inventory->GetWeapons()[0]->GetInstance());
		TestTrue(TEXT("It's refused on Heirloom"), Inventory->CheckFit(FCarriedGun::Equipped(0), 0) == WeaponPartSwap::ECheck::NamedGun);
		TestFalse(TEXT("...and not fitted"), Inventory->FitPart(FCarriedGun::Equipped(0), 0));
		TestTrue(TEXT("...Heirloom as it was, the part still in the box"), KeysOf(Inventory->GetWeapons()[0]->GetInstance()) == KeysBefore
			&& Inventory->GetPartsBox().Num() == 1);
		TestTrue(TEXT("...while the rolled shotgun takes it"), Inventory->FitPart(FCarriedGun::Equipped(1), 0));
	}
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunsmithBenchInteractionTest, "Looter.Weapons.Bench.Interaction",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunsmithBenchInteractionTest::RunTest(const FString& Parameters)
{
	// The bench offers a tap of Interact ("Use the gunsmith's bench") from its front, any time; missions find it by its tag.
	using namespace InteractionTestWorld;
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	// Its front toward the player, who looks along +X.
	AGunsmithBench* Bench = World->SpawnActor<AGunsmithBench>(FVector::ZeroVector, FRotator(0.0, 180.0, 0.0));
	if (!TestTrue(TEXT("Player and bench placed"), Player && Interaction && Bench))
	{
		return false;
	}
	Bench->DispatchBeginPlay();

	const FInteractionOptions Options = Bench->GetInteractionOptions(*Interaction);
	TestTrue(TEXT("Usable from the start, with a tap"), Options.CanUse() && Options.bTap && !Options.bHold);
	TestEqual(TEXT("Its prompt"), Options.TapPrompt.ToString(), FString(TEXT("Use the gunsmith's bench")));
	TestTrue(TEXT("Tagged for missions and the minimap"), Bench->ActorHasTag(AGunsmithBench::BenchTag) && Bench->ActorHasTag(MinimapTags::Obstacle));
	TestFalse(TEXT("It never ticks"), Bench->PrimaryActorTick.bCanEverTick);

	const TOptional<FVector> Point = Bench->GetInteractionLocation();
	if (!TestTrue(TEXT("Used from a point on its front"), Point.IsSet()))
	{
		return false;
	}
	TestTrue(TEXT("...on the side it faces"), FVector::DotProduct(Point.GetValue() - Bench->GetActorLocation(), Bench->GetActorForwardVector()) > 0.0);
	// Moved so that point is a meter and a half straight ahead of the player's eyes: it's what the player would use.
	Bench->SetActorLocation(Ahead(150.0) - (Point.GetValue() - Bench->GetActorLocation()));
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("Looked at, it's focused"), Interaction->GetFocusedActor() == Bench);

	// A test level has no player's HUD to show the screen: using it opens nothing, so nothing is told to the missions.
	TestFalse(TEXT("No screen without a HUD"), Bench->Interact(*Interaction, false));
	TestFalse(TEXT("A hold isn't how it's used"), Bench->Interact(*Interaction, true));
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FGunsmithBenchListsTest, "Looter.Weapons.Bench.Lists",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FGunsmithBenchListsTest::RunTest(const FString& Parameters)
{
	// What the bench's screen lists for a slot (the parts that fit first, then its kind's that don't, then other kinds'),
	// and how it words a part's changes.
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, BenchRiflePath);
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, BenchShotgunPath);
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}
	const FWeaponInstanceData Gun = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Legendary, 1);
	FBoxedWeaponPart Fitting;
	int32 SlotIndex = INDEX_NONE;
	if (!TestTrue(TEXT("A part that fits"), FindFittingPart(Gun, Fitting, SlotIndex)))
	{
		return false;
	}
	// Its own part in that slot (already fitted: it doesn't fit again), another slot's part, and a shotgun's part.
	FBoxedWeaponPart Same = Fitting;
	Same.Key = KeysOf(Gun)[SlotIndex];
	FBoxedWeaponPart OtherSlot = Fitting;
	for (const FWeaponPartSlot& Slot : Rifle->Parts)
	{
		if (Slot.Name != Fitting.Slot && !Slot.Options.IsEmpty())
		{
			OtherSlot.Slot = Slot.Name;
			OtherSlot.Key = Slot.Options[0].Key;
		}
	}
	if (!TestTrue(TEXT("The rifle has another slot"), OtherSlot.Slot != Fitting.Slot))
	{
		return false;
	}
	// The shotgun's part for the same slot (its slots share the rifle's names).
	FBoxedWeaponPart OtherKind = AnyPart(Shotgun);
	OtherKind.Slot = Fitting.Slot;
	for (const FWeaponPartSlot& Slot : Shotgun->Parts)
	{
		if (Slot.Name == Fitting.Slot && !Slot.Options.IsEmpty())
		{
			OtherKind.Key = Slot.Options[0].Key;
		}
	}
	const TArray<FBoxedWeaponPart> Box = { OtherKind, Same, OtherSlot, Fitting };
	const TArray<BenchRules::FPartEntry> Entries = BenchRules::PartsForSlot(Box, Gun, Fitting.Slot);
	const bool bSameListed = !Same.Key.IsNone();
	if (TestEqual(TEXT("The slot's parts, and no other slot's"), Entries.Num(), bSameListed ? 3 : 2))
	{
		TestTrue(TEXT("The one that fits first"), Entries[0].BoxIndex == 3 && Entries[0].Fits());
		if (bSameListed)
		{
			TestTrue(TEXT("Then its own kind's that doesn't"), Entries[1].BoxIndex == 1 && !Entries[1].Fits());
		}
		TestTrue(TEXT("Then another kind's, saying so"), Entries.Last().BoxIndex == 0 && Entries.Last().Check == WeaponPartSwap::ECheck::WrongKind);
	}

	// A part's changes, the biggest first, each for the better or the worse.
	FWeaponStats Before;
	Before.Damage = 20.f;
	Before.Recoil = 1.f;
	Before.MagazineSize = 30;
	FWeaponStats After = Before;
	After.Damage = 22.f;
	After.Recoil = 1.2f;
	After.MagazineSize = 40;
	const TArray<BenchRules::FStatChange> Changes = BenchRules::StatChanges(Before, After);
	if (TestEqual(TEXT("Three changes"), Changes.Num(), 3))
	{
		TestTrue(TEXT("Ten more rounds, the biggest, for the better"), Changes[0].Text == TEXT("+10 Mag") && Changes[0].bBetter);
		TestTrue(TEXT("A fifth more recoil, for the worse"), Changes[1].Text == TEXT("+20% Recoil") && !Changes[1].bBetter);
		TestTrue(TEXT("A tenth more damage, for the better"), Changes[2].Text == TEXT("+10% Dmg") && Changes[2].bBetter);
	}
	TestEqual(TEXT("The same stats change nothing"), BenchRules::StatChanges(Before, Before).Num(), 0);
	return true;
}

#endif
