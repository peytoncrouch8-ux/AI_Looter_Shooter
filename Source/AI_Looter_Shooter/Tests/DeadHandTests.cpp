#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "GameFramework/DamageType.h"
#include "GameFramework/Pawn.h"
#include "Tests/AutomationCommon.h"
#include "UObject/Script.h"

// A dead hand fires nothing (playtest round 2: DEMO's fight bot landed shots ~2 s after "Player died"): from death until the
// respawn resets the holder's health, the trigger and the reload do nothing; after it, both work again. The sights drop too
// (UPlayerViewComponent::UpdateAim), which needs a player's view and is left to the play-check.

namespace
{
	/** A holder as the game has one: a pawn with health and a weapon manager, begun play. */
	APawn* SpawnHolder(UWorld* World, UHealthComponent*& OutHealth, UWeaponManagerComponent*& OutWeapons)
	{
		APawn* Holder = World->SpawnActor<APawn>();
		if (!Holder)
		{
			return nullptr;
		}
		OutHealth = NewObject<UHealthComponent>(Holder, TEXT("Health"));
		OutHealth->bShowDamageNumbers = false;
		Holder->AddInstanceComponent(OutHealth);
		OutHealth->RegisterComponent();
		OutWeapons = NewObject<UWeaponManagerComponent>(Holder, TEXT("Weapons"));
		Holder->AddInstanceComponent(OutWeapons);
		OutWeapons->RegisterComponent();
		OutWeapons->MaxWeapons = 3;
		Holder->DispatchBeginPlay();
		return Holder;
	}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDeadHandTest, "Looter.Weapons.DeadHand",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FDeadHandTest::RunTest(const FString& Parameters)
{
	FTestWorldWrapper WorldWrapper;
	UWeaponDefinition* Rifle = LoadObject<UWeaponDefinition>(nullptr, TEXT("/Game/Weapons/Data/DA_AssaultRifle.DA_AssaultRifle"));
	if (!TestTrue(TEXT("Test world created"), WorldWrapper.CreateTestWorld(EWorldType::EditorPreview)) || !TestNotNull(TEXT("Rifle loads"), Rifle))
	{
		return false;
	}
	UWorld* World = WorldWrapper.GetTestWorld();
	UHealthComponent* Health = nullptr;
	UWeaponManagerComponent* Weapons = nullptr;
	APawn* Holder = SpawnHolder(World, Health, Weapons);
	FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Rifle, EWeaponRarity::Common, 1);
	Instance.Seed = 2027;
	Instance.Parts.Reset();
	// Held but not drawn through the manager: a drawn gun waits out its draw before its first shot, and a test world's clock
	// never moves, so a given gun would never fire here. Owned by the holder, it reads their health and their ammo.
	AWeaponBase* Gun = Holder ? UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform::Identity) : nullptr;
	if (!TestNotNull(TEXT("A holder with a gun"), Gun))
	{
		return false;
	}
	Gun->SetOwner(Holder);
	Weapons->AddAmmo(Rifle->AmmoType, 120);
	const int32 Full = Gun->GetCurrentMagazine();

	// Killed: the trigger and the reload do nothing.
	{
		FEditorScriptExecutionGuard RunActorEvents;
		Holder->OnTakeAnyDamage.Broadcast(Holder, 100000.f, GetDefault<UDamageType>(), nullptr, nullptr);
	}
	if (!TestTrue(TEXT("The holder is dead"), Health->IsDead()))
	{
		return false;
	}
	Gun->StartFire();
	Gun->StopFire();
	TestEqual(TEXT("Dead: a pull fires nothing"), Gun->GetCurrentMagazine(), Full);
	TestFalse(TEXT("...and nothing is left waiting to fire"), Gun->HasShotPending());

	// Respawned (the vitals reset the same pawn's health): it fires, and a part-spent magazine reloads...
	Health->ResetHealth();
	Gun->StartFire();
	Gun->StopFire();
	TestEqual(TEXT("Alive again: a pull fires"), Gun->GetCurrentMagazine(), Full - 1);
	TestTrue(TEXT("...and the gun can reload"), Gun->CanReload());

	// ...but not once dead again.
	{
		FEditorScriptExecutionGuard RunActorEvents;
		Holder->OnTakeAnyDamage.Broadcast(Holder, 100000.f, GetDefault<UDamageType>(), nullptr, nullptr);
	}
	TestFalse(TEXT("Dead: no reload"), Gun->CanReload());
	Gun->Reload();
	TestFalse(TEXT("...a reload asked for doesn't start"), Gun->IsReloading());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
