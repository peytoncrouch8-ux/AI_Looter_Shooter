#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Interaction/InteractableProp.h"
#include "Interaction/InteractionComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Tests/InteractionTestWorld.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"

using namespace InteractionTestWorld;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionPropsTest, "Looter.Interaction.Props",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionPropsTest::RunTest(const FString& Parameters)
{
	// The generic prop as a door (swings about its hinge), a lantern post (its glass glows while lit) and a bell (its
	// delegate says who rang it and how), with their cooldowns and the on and off they keep.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();

	// The door: its leaf 50 cm to the right of the hinge, which is the prop's origin.
	AInteractableProp* Door = SpawnDoor(World, FVector(300.0, 0.0, 0.0), FVector(0.0, 50.0, 0.0));
	if (!TestNotNull(TEXT("Door placed"), Door))
	{
		return false;
	}
	const UStaticMeshComponent* Leaf = Door->Mesh;
	TestTrue(TEXT("Shut as placed"), Leaf->GetRelativeLocation().Equals(FVector(0.0, 50.0, 0.0), 0.01));
	TestTrue(TEXT("Opened"), Door->Use(nullptr, /*bHeld*/ false) && Door->IsOn());
	TestFalse(TEXT("Swinging: it can't be used"), Door->IsUsable());
	Door->Advance(0.2f);
	const double HalfwayYaw = Leaf->GetRelativeRotation().Yaw;
	TestTrue(TEXT("Halfway through the swing"), HalfwayYaw > 10.0 && HalfwayYaw < 80.0);
	Door->Advance(0.2f);
	TestTrue(TEXT("Swung a quarter turn about the hinge"), Leaf->GetRelativeLocation().Equals(FVector(-50.0, 0.0, 0.0), 0.5)
		&& FMath::IsNearlyEqual(Leaf->GetRelativeRotation().Yaw, 90.0, 0.5));
	TestFalse(TEXT("Still cooling down"), Door->IsUsable());
	Door->Advance(0.2f);
	TestTrue(TEXT("Usable again"), Door->IsUsable());
	TestEqual(TEXT("Its words now"), Door->GetCurrentPrompt().ToString(), FString(TEXT("Close the door")));
	TestTrue(TEXT("Shut"), Door->Use(nullptr, false) && !Door->IsOn());
	Door->Advance(0.4f);
	TestTrue(TEXT("Back where it was placed"), Leaf->GetRelativeLocation().Equals(FVector(0.0, 50.0, 0.0), 0.5)
		&& FMath::IsNearlyZero(Leaf->GetRelativeRotation().Yaw, 0.5));
	Door->SetOn(true, /*bInstant*/ true);
	TestTrue(TEXT("Opened by a script at once"), Leaf->GetRelativeLocation().Equals(FVector(-50.0, 0.0, 0.0), 0.5));
	Door->bEnabled = false;
	Door->Advance(5.f);
	TestFalse(TEXT("Turned off, it can't be used"), Door->IsUsable() || Door->Use(nullptr, false));

	// The lantern post: dark as placed, lit by a hold, and then it can't be lit again until it's put out.
	AInteractableProp* Lantern = SpawnLantern(World, FVector(0.0, 300.0, 0.0), /*bLit*/ false);
	if (!TestNotNull(TEXT("Lantern placed"), Lantern))
	{
		return false;
	}
	TestTrue(TEXT("Dark as placed"), Lantern->Mesh->GetMaterial(0) != LanternGlow());
	TestTrue(TEXT("Lit by a hold"), Lantern->Use(nullptr, /*bHeld*/ true) && Lantern->IsOn());
	TestTrue(TEXT("Its glass glows"), Lantern->Mesh->GetMaterial(0) == LanternGlow());
	Lantern->Advance(5.f);
	TestFalse(TEXT("Lit, it can't be lit again"), Lantern->IsUsable());
	Lantern->SetOn(false);
	TestTrue(TEXT("Put out: dark, and it can be lit again"), Lantern->Mesh->GetMaterial(0) != LanternGlow() && Lantern->IsUsable());

	// The bell: every ring the same, two seconds apart, and its delegate says who rang it and how.
	AInteractableProp* Bell = SpawnBell(World, FVector(0.0, -300.0, 0.0));
	AActor* Ringer = World->SpawnActor<AActor>();
	if (!TestTrue(TEXT("Bell and ringer placed"), Bell && Ringer))
	{
		return false;
	}
	int32 Rings = 0;
	const AActor* RungBy = nullptr;
	bool bRungByHold = false;
	Bell->OnUsedNative.AddLambda([&Rings, &RungBy, &bRungByHold](AInteractableProp&, AActor* User, bool bHeld)
	{
		++Rings;
		RungBy = User;
		bRungByHold = bHeld;
	});
	TestTrue(TEXT("Rung"), Bell->Use(Ringer, /*bHeld*/ true));
	TestTrue(TEXT("The delegate said who, and that it was held"), Rings == 1 && RungBy == Ringer && bRungByHold);
	TestFalse(TEXT("Still ringing: it can't be rung"), Bell->Use(Ringer, true));
	Bell->Advance(Bell->CooldownSeconds);
	TestTrue(TEXT("Rung again after its cooldown"), Bell->Use(Ringer, true) && Rings == 2);
	TestFalse(TEXT("A bell is never left on"), Bell->IsOn());
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FInteractionLootTest, "Looter.Interaction.Loot",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FInteractionLootTest::RunTest(const FString& Parameters)
{
	// Loot through the interaction component keeps its rules: the weapon manager offers the loot looked at, a tap picks it
	// up (into a free slot and the hand, else the backpack), a hold of PickupHoldSeconds takes it in hand (the gun in hand
	// going to the backpack, or onto the ground when that's full), and a tap never takes something else.
	FTestWorldWrapper TestLevel;
	if (!TestTrue(TEXT("Test level made"), TestLevel.CreateTestWorld(EWorldType::EditorPreview)))
	{
		return false;
	}
	UWorld* World = TestLevel.GetTestWorld();
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	UWeaponDefinition* Shotgun = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_PumpShotgun.DA_PumpShotgun"));
	if (!TestNotNull(TEXT("Rifle loads"), Rifle) || !TestNotNull(TEXT("Shotgun loads"), Shotgun))
	{
		return false;
	}

	UInteractionComponent* Interaction = nullptr;
	APawn* Player = SpawnPlayer(World, Interaction);
	if (!TestTrue(TEXT("Stand-in placed"), Player && Interaction))
	{
		return false;
	}
	UWeaponManagerComponent* Inventory = NewObject<UWeaponManagerComponent>(Player);
	Inventory->RegisterComponent();
	Inventory->MaxWeapons = 2;
	Inventory->BackpackCapacity = 1;

	auto Loot = [World](UWeaponDefinition* Definition, EWeaponRarity Rarity, const FVector& Where)
	{
		AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(World, UWeaponRollLibrary::RollWeaponWithRarity(Definition, Rarity, 1), FTransform(Where));
		if (Weapon)
		{
			Weapon->OnDropped();
		}
		return Weapon;
	};
	// On the floor in front of the stand-in, a little below its eyes: in reach and well inside the cone.
	const FVector LootSpot(120.0, 0.0, 30.0);

	// Offered by the weapon manager: it takes a tap and a hold of PickupHoldSeconds.
	AWeaponBase* First = Loot(Rifle, EWeaponRarity::Common, LootSpot);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The loot in front is focused"), First && Interaction->GetFocusedActor() == First);
	const FInteractionOptions LootOptions = Interaction->GetFocusedOptions();
	TestTrue(TEXT("Loot takes a tap and a hold"), LootOptions.bTap && LootOptions.bHold
		&& FMath::IsNearlyEqual(LootOptions.HoldSeconds, Inventory->PickupHoldSeconds));

	// A tap (let go before the hold's time): into the free slot and the hand.
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.1f);
	TestTrue(TEXT("While the key is down the hold bar fills"), Interaction->GetHoldProgress() > 0.f);
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Tapped: picked up and in hand"), Inventory->GetActiveWeapon() == First && !First->IsPickup());
	TestNull(TEXT("Nothing left to look at"), Interaction->GetFocusedActor());

	// A hold with a free slot: into it and the hand.
	AWeaponBase* Second = Loot(Shotgun, EWeaponRarity::Common, LootSpot);
	Interaction->UpdateInteraction(0.f);
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.25f);
	TestTrue(TEXT("Not before the hold's time"), Second && Second->IsPickup());
	Interaction->UpdateInteraction(0.25f);
	TestTrue(TEXT("Held: in the free slot and in hand"), Inventory->GetActiveWeapon() == Second && Inventory->GetActiveSlot() == 1);
	Interaction->ReleaseInteract();
	TestEqual(TEXT("Letting go after the hold takes nothing more"), Inventory->GetWeapons().Num(), 2);

	// The slots full, the backpack not: a tap sends it to the backpack.
	AWeaponBase* Third = Loot(Rifle, EWeaponRarity::Rare, LootSpot);
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The rare rifle is looked at"), Third && Interaction->GetFocusedActor() == Third);
	Interaction->PressInteract();
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Tapped with the slots full: into the backpack"), Inventory->GetBackpack().Num() == 1
		&& Inventory->GetBackpack()[0].Rarity == EWeaponRarity::Rare);
	TestTrue(TEXT("The shotgun still in hand"), Inventory->GetActiveWeapon() == Second);

	// Slots and backpack full: a hold takes it in hand, and the gun that was in hand drops as loot.
	AWeaponBase* Fourth = Loot(Rifle, EWeaponRarity::Epic, LootSpot);
	Interaction->UpdateInteraction(0.f);
	Interaction->PressInteract();
	Interaction->UpdateInteraction(0.5f);
	TestTrue(TEXT("Held with everything full: the epic rifle in hand"), Fourth && Inventory->GetActiveWeapon() == Fourth);
	TestTrue(TEXT("The shotgun dropped as loot"), IsValid(Second) && Second->IsPickup() && Second->GetOwner() == nullptr);
	Interaction->ReleaseInteract();

	// A tap never takes something else: the key goes down on the dropped shotgun, then the eyes move to other loot.
	AWeaponBase* Fifth = Loot(Shotgun, EWeaponRarity::Uncommon, FVector(0.0, 120.0, 30.0));
	Interaction->UpdateInteraction(0.f);
	TestTrue(TEXT("The dropped shotgun is looked at"), Interaction->GetFocusedActor() == Second);
	Interaction->PressInteract();
	Face(Player, 90.0);
	Interaction->UpdateInteraction(0.05f);
	TestTrue(TEXT("Now the other loot is looked at"), Fifth && Interaction->GetFocusedActor() == Fifth);
	Interaction->ReleaseInteract();
	TestTrue(TEXT("Neither was taken"), Second->IsPickup() && Fifth->IsPickup());
	TestTrue(TEXT("The epic rifle still in hand"), Inventory->GetActiveWeapon() == Fourth);
	return true;
}

#endif
