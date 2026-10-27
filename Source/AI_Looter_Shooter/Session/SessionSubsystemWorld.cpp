// USessionSubsystem: what a session keeps of the player and each map's world, and putting it back (Lost Property's shelves
// too, and the guns a save kept lying handed in to them rather than put back on the ground).

#include "Session/SessionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Combat/GrenadeKinds.h"
#include "Combat/HealthComponent.h"
#include "Economy/SilverPickup.h"
#include "Economy/WalletSubsystem.h"
#include "Embers/EmberComponent.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/AmmoPickup.h"
#include "Loot/Chest.h"
#include "Loot/GrenadePickup.h"
#include "Loot/LostLootRules.h"
#include "Loot/LostLootStore.h"
#include "Loot/LostLootSubsystem.h"
#include "Loot/WeaponRack.h"
#include "Player/PlayerThrowComponent.h"
#include "Shields/ShieldPickup.h"
#include "Session/SessionSave.h"
#include "Tutorial/TutorialDirector.h"
#include "Weapons/WeaponBase.h"
#include "World/BreakableProp.h"
#include "World/GraveTravelSubsystem.h"
#include "World/HayBale.h"
#include "World/RespawnMarker.h"
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

	/**
	 * A pickup put back goes on with the seconds it had left, never more than the life it has just begun with. An older
	 * save kept none (0): it keeps the life it begins with.
	 */
	void ContinueLife(AActor& Pickup, float LifeLeft)
	{
		const float Whole = Pickup.GetLifeSpan();
		if (LifeLeft > 0.f && (Whole <= 0.f || LifeLeft < Whole))
		{
			Pickup.SetLifeSpan(LifeLeft);
		}
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
	if (const UPlayerThrowComponent* Throw = UPlayerThrowComponent::Find(Pawn))
	{
		Throw->SaveThrowables(Save.Throwables);
		Save.bHasThrowables = true;
	}
	if (const UEmberComponent* Embers = UEmberComponent::Find(Pawn))
	{
		Embers->SaveEmbers(Save.Embers);
		Save.bHasEmbers = true;
	}
	// The purse and the vendors' gun of the day, which live with the level's wallet and travel with the player.
	if (const UWalletSubsystem* Wallet = UWalletSubsystem::Get(World))
	{
		Save.Silver = Wallet->GetSilver();
		Save.VendorOffer = Wallet->GetOffer();
	}
	// Lost Property's shelves, the session's one store like the purse (a trip has handed in this map's ground guns already).
	const ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(World);
	if (LostLoot)
	{
		Save.LostLoot = LostLoot->GetStore();
	}

	// This map's world, filed under its own name so every other map's stays as it was left: what the racks still offer,
	// every other gun and ammo pickup lying around, the wanted posters torn down, the hay bales loaded, the chests opened,
	// and the tutorial's step. When its creatures were promoted and its Legendary monsters beaten is kept as it was.
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
	// Loaded bales the same way: Side 2's lasting objective counts what the world keeps in the stack.
	Here.LoadedBales.Reset();
	for (TActorIterator<AHayBale> It(World); It; ++It)
	{
		if (It->IsLoaded())
		{
			Here.LoadedBales.Add(It->GetFName());
		}
	}
	// Opened chests by their ids, once their loot is out (the loot itself is kept below, as any loot lying around).
	Here.OpenedChests.Reset();
	for (TActorIterator<AChest> It(World); It; ++It)
	{
		if (It->HasGivenLoot())
		{
			Here.OpenedChests.Add(It->GetSaveKey());
		}
	}
	// Broken crates and barrels by their ids (what they dropped is kept below, as any loot lying around).
	Here.BrokenProps.Reset();
	for (TActorIterator<ABreakableProp> It(World); It; ++It)
	{
		if (It->IsBroken())
		{
			Here.BrokenProps.Add(It->GetSaveKey());
		}
	}
	// The guns lying loose, with their time on the ground: the next load hands them in to Lost Property, oldest first.
	Here.LootWeapons.Reset();
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		if (It->IsPickup() && !It->IsActorBeingDestroyed() && It->GetInstance().Definition && !IsRackLoot(Racks, *It))
		{
			FSavedLootWeapon& Loot = Here.LootWeapons.AddDefaulted_GetRef();
			Loot.Weapon = It->GetInstanceForStorage();
			Loot.Transform = It->GetActorTransform();
			// Never exactly 0, which marks a save from before the life was kept.
			const float Life = LostLoot ? LostLoot->GetLifeLeft(**It) : LostLootRules::GroundLifeSeconds;
			Loot.LifeLeft = Life == 0.f ? -0.01f : Life;
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
			Ammo.LifeLeft = It->GetLifeSpan();
		}
	}
	// Wards lying around, with their time on the ground (put back where they lay: wards have no Lost Property yet).
	Here.ShieldPickups.Reset();
	for (TActorIterator<AShieldPickup> It(World); It; ++It)
	{
		if (!It->IsActorBeingDestroyed() && It->GetWard().Definition)
		{
			FSavedShieldPickup& Ward = Here.ShieldPickups.AddDefaulted_GetRef();
			Ward.Ward = It->GetWard();
			Ward.Location = It->GetActorLocation();
			Ward.LifeLeft = It->GetLifeLeft();
		}
	}
	// Silver lying around, as the ammo is (a pile on its way to the player is theirs in a moment: it's kept too).
	Here.SilverPickups.Reset();
	for (TActorIterator<ASilverPickup> It(World); It; ++It)
	{
		if (It->GetAmount() > 0 && !It->IsActorBeingDestroyed())
		{
			FSavedSilverPickup& Silver = Here.SilverPickups.AddDefaulted_GetRef();
			Silver.Amount = It->GetAmount();
			Silver.Location = It->GetActorLocation();
			Silver.LifeLeft = It->GetLifeSpan();
		}
	}
	// Grave-salt grenade tins the same way, with the seconds they had left. Not Lost Property's: only guns go there.
	Here.GrenadePickups.Reset();
	for (TActorIterator<AGrenadePickup> It(World); It; ++It)
	{
		if (It->GetAmount() > 0 && !It->IsActorBeingDestroyed())
		{
			FSavedGrenadePickup& Tin = Here.GrenadePickups.AddDefaulted_GetRef();
			Tin.Amount = It->GetAmount();
			Tin.Location = It->GetActorLocation();
			Tin.LifeLeft = It->GetLifeSpan();
			Tin.Kind = static_cast<uint8>(It->GetKind());
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

	// Lost Property's shelves as the session kept them (a save from before them: empty shelves of 30), first: this map's
	// ground guns are handed in to them below.
	ULostLootSubsystem* LostLoot = ULostLootSubsystem::Get(World);
	if (LostLoot)
	{
		LostLoot->SetStore(Save.LostLoot);
	}
	int32 HandedIn = 0;

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

		// The bales loaded before stand in the stack again, quietly (no remark).
		for (TActorIterator<AHayBale> It(World); It; ++It)
		{
			if (Here->LoadedBales.Contains(It->GetFName()))
			{
				It->RestoreLoaded();
			}
		}

		// The chests opened before stand open and empty: what they gave comes back below with the rest of the loot.
		for (TActorIterator<AChest> It(World); It; ++It)
		{
			if (Here->OpenedChests.Contains(It->GetSaveKey()))
			{
				It->RestoreOpened();
			}
		}
		// The crates and barrels broken before stand as stumps: what they dropped comes back below with the rest of the loot.
		for (TActorIterator<ABreakableProp> It(World); It; ++It)
		{
			if (Here->BrokenProps.Contains(It->GetSaveKey()))
			{
				It->RestoreBroken();
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
		for (TActorIterator<ASilverPickup> It(World); It; ++It)
		{
			Stale.Add(*It);
		}
		for (TActorIterator<AGrenadePickup> It(World); It; ++It)
		{
			Stale.Add(*It);
		}
		for (TActorIterator<AShieldPickup> It(World); It; ++It)
		{
			Stale.Add(*It);
		}
		for (AActor* Actor : Stale)
		{
			Actor->Destroy();
		}
		// The guns the save kept lying here never go back on the ground (the user's call, 2026-10-13): every load follows a
		// quit (Save & Quit, closing the game, a stopped play session), and a gun left on the ground then goes to Lost
		// Property, the longest lain first; an older save's too. The HUD tells how many once the player can see it. A gun
		// whose kind no longer exists (its data asset gone) can't come back anywhere.
		if (LostLoot)
		{
			TArray<FLostLootArrival> Older;
			TArray<FLostLootArrival> LeftAtQuit;
			for (const FSavedLootWeapon& Loot : Here->LootWeapons)
			{
				if (Loot.Weapon.Definition)
				{
					(Loot.LifeLeft == 0.f ? Older : LeftAtQuit).Add({ Loot.Weapon, Loot.LifeLeft });
				}
			}
			HandedIn += LostLoot->Receive(MoveTemp(Older), ELostLootReason::OldSave, MapPackage, /*bAnnounce*/ true);
			HandedIn += LostLoot->Receive(MoveTemp(LeftAtQuit), ELostLootReason::LeftAtQuit, MapPackage, /*bAnnounce*/ true);
		}
		else
		{
			// A kind of world without Lost Property: the guns lie where they lay, as before it.
			for (const FSavedLootWeapon& Loot : Here->LootWeapons)
			{
				if (Loot.Weapon.Definition)
				{
					if (AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(World, Loot.Weapon, Loot.Transform))
					{
						Weapon->Toss(FVector::ZeroVector);
					}
				}
			}
		}
		// Ammo, silver and grenade tins go on with the life they had left (a fresh one on every load kept them lying there for
		// good): a tin whose seconds run out is gone, as on the ground.
		for (const FSavedAmmoPickup& Ammo : Here->AmmoPickups)
		{
			if (AAmmoPickup* Pickup = AAmmoPickup::SpawnAmmo(World, Ammo.Type, Ammo.Amount, Ammo.Location))
			{
				Pickup->Toss(FVector::ZeroVector);
				ContinueLife(*Pickup, Ammo.LifeLeft);
			}
		}
		for (const FSavedShieldPickup& Ward : Here->ShieldPickups)
		{
			if (!Ward.Ward.Definition)
			{
				continue;
			}
			if (AShieldPickup* Pickup = AShieldPickup::SpawnWard(World, Ward.Ward, Ward.Location))
			{
				Pickup->ContinueLife(Ward.LifeLeft);
			}
		}
		for (const FSavedSilverPickup& Silver : Here->SilverPickups)
		{
			if (ASilverPickup* Pickup = ASilverPickup::SpawnSilver(World, Silver.Amount, Silver.Location))
			{
				Pickup->Toss(FVector::ZeroVector);
				ContinueLife(*Pickup, Silver.LifeLeft);
			}
		}
		for (const FSavedGrenadePickup& Tin : Here->GrenadePickups)
		{
			if (AGrenadePickup* Pickup = AGrenadePickup::SpawnKind(World, Tin.Amount, Tin.Location, GrenadeKinds::FromSaved(Tin.Kind)))
			{
				Pickup->Toss(FVector::ZeroVector);
				// Its own whole life is the cap, and the life when none was kept: set outright, so a tin put back before
				// its BeginPlay has run (a world not yet playing) still ends.
				Pickup->SetLifeSpan(Tin.LifeLeft > 0.f ? FMath::Min(Tin.LifeLeft, Pickup->LifeSeconds) : Pickup->LifeSeconds);
			}
		}
	}

	// The purse and the gun of the day, as the session kept them (a save from before money: none, and none rolled).
	if (UWalletSubsystem* Wallet = UWalletSubsystem::Get(World))
	{
		Wallet->SetSilver(Save.Silver);
		Wallet->SetOffer(Save.VendorOffer);
	}

	// The player, who has just been put at the level's start (or a trip's landing). Their spot is only good in the level
	// it was saved in, and only a Continue has one: a trip's arrival has none, and lands at its landing.
	APlayerController* Controller = World->GetFirstPlayerController();
	APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
	if (Pawn)
	{
		if (Save.HasPlayerSpotOn(MapPackage))
		{
			// Borderlands' way: a Continue wakes at the roost the player last found in this area (the save point, where a
			// death wakes too; a save naming a roost this level no longer has takes the newest found still standing), else
			// where they stood (none found here yet, or a level with no roosts: Skyreach). Never an open roost they never
			// walked up to: the user's rule (2026-10-14) is the last touched, not the nearest.
			const ARespawnMarker* Grave = ARespawnMarker::FindLastFound(World, Save.Campaign, ARespawnMarker::FindAreaId(World));
			if (Grave)
			{
				// On the roost's spot, facing its arrow, level, as a death's wake-up and a fast travel put them.
				UGraveTravelSubsystem::MoveToGrave(*Pawn, *Grave);
				UE_LOG(LogLooter, Log, TEXT("Session continued at the roost %s (the last found here)"), *Grave->GetMarkerId().ToString());
			}
			else
			{
				// Upright, facing the saved way; the view takes the saved pitch too.
				Pawn->TeleportTo(Save.PlayerLocation, FRotator(0.f, Save.PlayerView.Yaw, 0.f), /*bIsATest*/ false, /*bNoCheck*/ true);
				Controller->SetControlRotation(FRotator(Save.PlayerView.Pitch, Save.PlayerView.Yaw, 0.f));
			}
		}
		// The guns first: one in hand can change the player's max health (a Grasping iron's), keeping health's share,
		// so health set before it would come back lower on every load. The grenades go in before the guns, so the gun
		// that comes into hand doesn't give the starting two on top (and announce +2) of the count the session kept.
		if (Save.bHasThrowables)
		{
			if (UPlayerThrowComponent* Throw = UPlayerThrowComponent::Find(Pawn))
			{
				Throw->RestoreThrowables(Save.Throwables);
			}
		}
		// The equipped ember power and its cooldown left (a save from before the powers keeps the new game's: the first, ready).
		if (Save.bHasEmbers)
		{
			if (UEmberComponent* Embers = UEmberComponent::Find(Pawn))
			{
				Embers->RestoreEmbers(Save.Embers);
			}
		}
		if (UWeaponManagerComponent* Manager = Pawn->FindComponentByClass<UWeaponManagerComponent>())
		{
			if (Save.bHasInventory)
			{
				Manager->RestoreInventory(Save.Inventory);
			}
		}
		if (UHealthComponent* Health = Pawn->FindComponentByClass<UHealthComponent>())
		{
			if (Save.Health > 0.f)
			{
				Health->SetHealth(Save.Health);
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
	UE_LOG(LogLooter, Log, TEXT("Session restored in %s: %d guns carried, %d ground guns handed in to Lost Property (%d on its shelves), %d ammo pickups and %d grenade tins on the ground"),
		*FPackageName::GetShortName(MapPackage), Save.CountGuns(), HandedIn, LostLoot ? LostLoot->GetStore().Num() : Save.LostLoot.Num(),
		Here ? Here->AmmoPickups.Num() : 0, Here ? Here->GrenadePickups.Num() : 0);
}
