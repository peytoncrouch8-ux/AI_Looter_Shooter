// USessionSubsystem: what a session keeps of the player and the world, and putting it back.

#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Combat/HealthComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/AmmoPickup.h"
#include "Loot/WeaponRack.h"
#include "Session/SessionSave.h"
#include "Tutorial/TutorialDirector.h"
#include "Weapons/WeaponBase.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

namespace
{
	/** Loot a gun rack laid out itself: the rack's own state covers it, so it isn't saved as loose loot. */
	bool IsRackLoot(const TArray<AWeaponRack*>& Racks, const AActor* Loot)
	{
		return Racks.ContainsByPredicate([Loot](const AWeaponRack* Rack) { return Rack->Offers(Loot); });
	}

	TArray<AWeaponRack*> FindRacks(UWorld* World)
	{
		TArray<AWeaponRack*> Racks;
		for (TActorIterator<AWeaponRack> It(World); It; ++It)
		{
			Racks.Add(*It);
		}
		return Racks;
	}
}

void USessionSubsystem::CaptureWorld(UWorld* World, ULooterSessionSave& Save)
{
	if (!World)
	{
		return;
	}

	// The player: where they stand and look, their health and what they carry. Saved while dead, they come back at the
	// level's start with full health, as a respawn would bring them.
	APlayerController* Controller = World->GetFirstPlayerController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	const UHealthComponent* Health = Pawn ? Pawn->FindComponentByClass<UHealthComponent>() : nullptr;
	const bool bAlive = Pawn && !(Health && Health->IsDead());
	Save.bHasPlayerSpot = bAlive;
	Save.Health = bAlive && Health ? Health->GetHealth() : 0.f;
	if (bAlive)
	{
		Save.PlayerLocation = Pawn->GetActorLocation();
		Save.PlayerView = Controller->GetControlRotation();
	}
	if (const UWeaponManagerComponent* Manager = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr)
	{
		Manager->SaveInventory(Save.Inventory);
		Save.bHasInventory = true;
	}

	// The world: what the racks still offer, and every other gun and ammo pickup lying around.
	Save.bHasWorld = true;
	const TArray<AWeaponRack*> Racks = FindRacks(World);
	Save.Racks.Reset();
	for (const AWeaponRack* Rack : Racks)
	{
		FSavedWeaponRack& State = Save.Racks.AddDefaulted_GetRef();
		State.Rack = Rack->GetFName();
		State.bWeaponOffered = Rack->IsWeaponOffered();
		State.AmmoPickupsLeft = Rack->GetAmmoPickupsLeft();
	}
	Save.LootWeapons.Reset();
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		if (It->IsPickup() && !It->IsActorBeingDestroyed() && It->GetInstance().Definition && !IsRackLoot(Racks, *It))
		{
			FSavedLootWeapon& Loot = Save.LootWeapons.AddDefaulted_GetRef();
			Loot.Weapon = It->GetInstanceForStorage();
			Loot.Transform = It->GetActorTransform();
		}
	}
	Save.AmmoPickups.Reset();
	for (TActorIterator<AAmmoPickup> It(World); It; ++It)
	{
		if (It->GetAmount() > 0 && !It->IsActorBeingDestroyed() && !IsRackLoot(Racks, *It))
		{
			FSavedAmmoPickup& Ammo = Save.AmmoPickups.AddDefaulted_GetRef();
			Ammo.Type = It->GetAmmoType();
			Ammo.Amount = It->GetAmount();
			Ammo.Location = It->GetActorLocation();
		}
	}

	TActorIterator<ATutorialDirector> Director(World);
	Save.TutorialStep = Director ? Director->GetCurrentStep() : INDEX_NONE;
}

void USessionSubsystem::RestoreWorld(UWorld* World, const ULooterSessionSave& Save)
{
	if (!World)
	{
		return;
	}

	if (Save.bHasWorld)
	{
		// The racks laid out their loot as the level began; what the player had already taken goes again.
		const TArray<AWeaponRack*> Racks = FindRacks(World);
		for (AWeaponRack* Rack : Racks)
		{
			const FName RackName = Rack->GetFName();
			if (const FSavedWeaponRack* State = Save.Racks.FindByPredicate([RackName](const FSavedWeaponRack& Saved) { return Saved.Rack == RackName; }))
			{
				Rack->RestoreOffer(State->bWeaponOffered, State->AmmoPickupsLeft);
			}
		}

		// Any other loot the level began with is in the save if it was still lying around: it comes back from there.
		TArray<AActor*> Stale;
		for (TActorIterator<AWeaponBase> It(World); It; ++It)
		{
			if (It->IsPickup() && !IsRackLoot(Racks, *It))
			{
				Stale.Add(*It);
			}
		}
		for (TActorIterator<AAmmoPickup> It(World); It; ++It)
		{
			if (!IsRackLoot(Racks, *It))
			{
				Stale.Add(*It);
			}
		}
		for (AActor* Actor : Stale)
		{
			Actor->Destroy();
		}
		for (const FSavedLootWeapon& Loot : Save.LootWeapons)
		{
			// A gun whose kind no longer exists (its data asset gone) can't come back.
			if (Loot.Weapon.Definition)
			{
				if (AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(World, Loot.Weapon, Loot.Transform))
				{
					// Loot again, settling where it lay.
					Weapon->Toss(FVector::ZeroVector);
				}
			}
		}
		for (const FSavedAmmoPickup& Ammo : Save.AmmoPickups)
		{
			if (AAmmoPickup* Pickup = AAmmoPickup::SpawnAmmo(World, Ammo.Type, Ammo.Amount, Ammo.Location))
			{
				Pickup->Toss(FVector::ZeroVector);
			}
		}
	}

	// The player, who has just been put at the level's start.
	APlayerController* Controller = World->GetFirstPlayerController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (Pawn)
	{
		if (Save.bHasPlayerSpot)
		{
			// Upright, facing the saved way; the view takes the saved pitch too.
			Pawn->TeleportTo(Save.PlayerLocation, FRotator(0.f, Save.PlayerView.Yaw, 0.f), /*bIsATest*/ false, /*bNoCheck*/ true);
			Controller->SetControlRotation(FRotator(Save.PlayerView.Pitch, Save.PlayerView.Yaw, 0.f));
		}
		if (UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>())
		{
			if (Save.Health > 0.f)
			{
				Health->SetHealth(Save.Health);
			}
		}
		if (UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			if (Save.bHasInventory)
			{
				Manager->RestoreInventory(Save.Inventory);
			}
		}
	}

	// The tutorial picks up at the step it was on (its first steps would otherwise ask again for what was done).
	if (!Save.Progress.bTutorialDone && Save.TutorialStep > 0)
	{
		for (TActorIterator<ATutorialDirector> It(World); It; ++It)
		{
			It->ResumeAtStep(Save.TutorialStep);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Session restored: %d guns carried, %d guns and %d ammo pickups on the ground"),
		Save.Inventory.Equipped.Num() + Save.Inventory.Backpack.Num(), Save.LootWeapons.Num(), Save.AmmoPickups.Num());
}
