// USessionSubsystem: what a session keeps of the player and each map's world, and putting it back.

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
#include "World/WantedPoster.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Misc/PackageName.h"

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
	const FString MapPackage = MapOf(World);

	// The player: the level they're in, where they stand and look, their health and what they carry. Saved while dead,
	// they come back at the level's start with full health, as a respawn would bring them.
	Save.Map = MapPackage;
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

	// This map's world, filed under its own name so every other map's stays as it was left: what the racks still offer,
	// every other gun and ammo pickup lying around, the wanted posters torn down, and the tutorial's step. When its
	// creatures were promoted and its Legendary monsters beaten is kept as it was.
	FSavedMapWorld& Here = Save.FindOrAddWorld(MapPackage);
	const TArray<AWeaponRack*> Racks = FindRacks(World);
	Here.Racks.Reset();
	for (const AWeaponRack* Rack : Racks)
	{
		FSavedWeaponRack& State = Here.Racks.AddDefaulted_GetRef();
		State.Rack = Rack->GetFName();
		State.bWeaponOffered = Rack->IsWeaponOffered();
		State.AmmoPickupsLeft = Rack->GetAmmoPickupsLeft();
	}
	// Torn posters by name, as racks are: Side 1's lasting objective counts what the world keeps down.
	Here.TornPosters.Reset();
	for (TActorIterator<AWantedPoster> It(World); It; ++It)
	{
		if (It->IsTorn())
		{
			Here.TornPosters.Add(It->GetFName());
		}
	}
	Here.LootWeapons.Reset();
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		if (It->IsPickup() && !It->IsActorBeingDestroyed() && It->GetInstance().Definition && !IsRackLoot(Racks, *It))
		{
			FSavedLootWeapon& Loot = Here.LootWeapons.AddDefaulted_GetRef();
			Loot.Weapon = It->GetInstanceForStorage();
			Loot.Transform = It->GetActorTransform();
		}
	}
	Here.AmmoPickups.Reset();
	for (TActorIterator<AAmmoPickup> It(World); It; ++It)
	{
		if (It->GetAmount() > 0 && !It->IsActorBeingDestroyed() && !IsRackLoot(Racks, *It))
		{
			FSavedAmmoPickup& Ammo = Here.AmmoPickups.AddDefaulted_GetRef();
			Ammo.Type = It->GetAmmoType();
			Ammo.Amount = It->GetAmount();
			Ammo.Location = It->GetActorLocation();
		}
	}

	TActorIterator<ATutorialDirector> Director(World);
	Here.TutorialStep = Director ? Director->GetCurrentStep() : INDEX_NONE;
}

void USessionSubsystem::RestoreWorld(UWorld* World, const ULooterSessionSave& Save)
{
	if (!World)
	{
		return;
	}
	const FString MapPackage = MapOf(World);

	// This map's world, when the session has been here before; on a first visit the level starts as it was built.
	const FSavedMapWorld* Here = Save.FindWorld(MapPackage);
	if (Here)
	{
		// The racks laid out their loot as the level began; what the player had already taken goes again.
		const TArray<AWeaponRack*> Racks = FindRacks(World);
		for (AWeaponRack* Rack : Racks)
		{
			const FName RackName = Rack->GetFName();
			if (const FSavedWeaponRack* State = Here->Racks.FindByPredicate([RackName](const FSavedWeaponRack& Saved) { return Saved.Rack == RackName; }))
			{
				Rack->RestoreOffer(State->bWeaponOffered, State->AmmoPickupsLeft);
			}
		}

		// The posters torn down before come down again, quietly (no scrap, no remark).
		for (TActorIterator<AWantedPoster> It(World); It; ++It)
		{
			if (Here->TornPosters.Contains(It->GetFName()))
			{
				It->RestoreTorn();
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
		for (const FSavedLootWeapon& Loot : Here->LootWeapons)
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
		for (const FSavedAmmoPickup& Ammo : Here->AmmoPickups)
		{
			if (AAmmoPickup* Pickup = AAmmoPickup::SpawnAmmo(World, Ammo.Type, Ammo.Amount, Ammo.Location))
			{
				Pickup->Toss(FVector::ZeroVector);
			}
		}
	}

	// The player, who has just been put at the level's start (or a trip's landing). Their spot is only good in the level
	// it was saved in.
	APlayerController* Controller = World->GetFirstPlayerController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (Pawn)
	{
		if (Save.HasPlayerSpotOn(MapPackage))
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
	if (!Save.Progress.bTutorialDone && Here && Here->TutorialStep > 0)
	{
		for (TActorIterator<ATutorialDirector> It(World); It; ++It)
		{
			It->ResumeAtStep(Here->TutorialStep);
		}
	}
	UE_LOG(LogLooter, Log, TEXT("Session restored in %s: %d guns carried, %d guns and %d ammo pickups on the ground"),
		*FPackageName::GetShortName(MapPackage), Save.CountGuns(), Here ? Here->LootWeapons.Num() : 0, Here ? Here->AmmoPickups.Num() : 0);
}
