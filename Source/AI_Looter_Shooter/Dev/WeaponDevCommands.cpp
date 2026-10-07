// Developer console commands for weapons and the inventory (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/AmmoPickup.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "AI_Looter_Shooter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"

namespace
{
	/** The game world the command is for: the one it was typed in, or the running PIE session when typed in the editor. */
	UWorld* FindGameWorld(UWorld* World)
	{
		if (World && World->IsGameWorld())
		{
			return World;
		}
		for (const FWorldContext& Context : GEngine->GetWorldContexts())
		{
			if (Context.World() && Context.World()->IsGameWorld())
			{
				return Context.World();
			}
		}
		return nullptr;
	}

	/** The weapon definition whose asset name contains the text ("Rifle" finds DA_AssaultRifle). */
	UWeaponDefinition* FindDefinition(const FString& Text)
	{
		TArray<FAssetData> Assets;
		IAssetRegistry::GetChecked().GetAssetsByClass(UWeaponDefinition::StaticClass()->GetClassPathName(), Assets, true);
		for (const FAssetData& Asset : Assets)
		{
			if (Asset.AssetName.ToString().Contains(Text))
			{
				return Cast<UWeaponDefinition>(Asset.GetAsset());
			}
		}
		return nullptr;
	}

	/** Puts the named parts ("Sight=Variable") on a rolled gun in place of the ones it rolled, and rolls its stats again. */
	void ForceParts(FWeaponInstanceData& Instance, const TArray<FString>& Overrides)
	{
		const UWeaponDefinition* Definition = Instance.Definition;
		for (const FString& Override : Overrides)
		{
			FString SlotName;
			FString Key;
			Override.Split(TEXT("="), &SlotName, &Key);
			const int32 Slot = Definition->Parts.IndexOfByPredicate([&SlotName](const FWeaponPartSlot& Part) { return Part.Name.ToString().Equals(SlotName, ESearchCase::IgnoreCase); });
			const FWeaponPartOption* Option = Slot != INDEX_NONE ? Definition->Parts[Slot].Options.FindByPredicate([&Key](const FWeaponPartOption& Part)
				{
					return Part.Key.ToString().Equals(Key, ESearchCase::IgnoreCase);
				}) : nullptr;
			if (!Option || !Instance.Parts.IsValidIndex(Slot))
			{
				UE_LOG(LogLooter, Warning, TEXT("Looter.GiveWeapon: %s has no part %s."), *Definition->GetName(), *Override);
				continue;
			}
			Instance.Parts[Slot] = Option->Key;
		}
		Instance.Stats = UWeaponRollLibrary::ComputeStatsWithParts(Definition, Instance.Rarity, Instance.Level, Instance.Seed, Instance.Parts);
	}

	/** Into a free slot if there is one, otherwise the backpack (never swapping out the weapon in hand). */
	bool Give(UWeaponManagerComponent& Inventory, const FWeaponInstanceData& Instance)
	{
		return Inventory.GetWeapons().Num() < Inventory.MaxWeapons ? Inventory.GiveWeapon(Instance) != nullptr : Inventory.AddToBackpack(Instance);
	}

	/** A named gun (Heirloom): its own parts and rarity, at the level asked for or else the player's, as missions give it. */
	void GiveNamedWeapon(UNamedWeaponDefinition& Named, const TArray<FString>& Args, bool bPartsAsked, const APlayerController& Controller,
		UWeaponManagerComponent& Inventory)
	{
		if (bPartsAsked)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.GiveWeapon: %s keeps its own parts; the Slot=Key arguments are left out."), *Named.GetNamedId().ToString());
		}
		const ULocalPlayer* LocalPlayer = Controller.GetLocalPlayer();
		const UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
		const int32 Level = Args.Num() > 1 && Args[1].IsNumeric() ? FMath::Max(FCString::Atoi(*Args[1]), 1) : (Progression ? Progression->GetLevel() : 1);
		const FWeaponInstanceData Instance = Named.MakeInstance(Level);
		if (!Instance.Definition)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.GiveWeapon: %s names no kind of gun."), *Named.GetName());
			return;
		}
		for (const FString& Problem : Named.FindProblems())
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.GiveWeapon: %s: %s."), *Named.GetName(), *Problem);
		}
		const bool bGiven = Give(Inventory, Instance);
		UE_LOG(LogLooter, Log, TEXT("Looter.GiveWeapon: %s, %s %s (level %d) %s."), *Named.GetNamedId().ToString(),
			*StaticEnum<EWeaponRarity>()->GetNameStringByValue(static_cast<int64>(Instance.Rarity)), *Instance.Definition->GetName(), Instance.Level,
			bGiven ? TEXT("given") : TEXT("did not fit"));
	}

	void GiveWeapon(const TArray<FString>& AllArgs, UWorld* World)
	{
		// Slot=Key arguments pick parts; the rest are the weapon, rarity and level in order.
		TArray<FString> Args;
		TArray<FString> Overrides;
		for (const FString& Arg : AllArgs)
		{
			(Arg.Contains(TEXT("=")) ? Overrides : Args).Add(Arg);
		}
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		UWeaponManagerComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.GiveWeapon: no player with a weapon inventory (start the game first)."));
			return;
		}

		// A named gun by its id or name first: named guns match whole names only, so "Rifle" still finds a rifle.
		if (UNamedWeaponDefinition* Named = Args.Num() > 0 ? UNamedWeaponDefinition::FindByName(Args[0]) : nullptr)
		{
			GiveNamedWeapon(*Named, Args, !Overrides.IsEmpty(), *Controller, *Inventory);
			return;
		}

		UWeaponDefinition* Definition = FindDefinition(Args.Num() > 0 ? Args[0] : FString(TEXT("Rifle")));
		if (!Definition)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.GiveWeapon: no weapon definition matches '%s'."), Args.Num() > 0 ? *Args[0] : TEXT(""));
			return;
		}
		const int32 Level = Args.Num() > 2 ? FMath::Max(FCString::Atoi(*Args[2]), 1) : 1;
		const int64 Rarity = Args.Num() > 1 ? StaticEnum<EWeaponRarity>()->GetValueByNameString(Args[1]) : INDEX_NONE;
		FWeaponInstanceData Instance = Rarity != INDEX_NONE
			? UWeaponRollLibrary::RollWeaponWithRarity(Definition, static_cast<EWeaponRarity>(Rarity), Level)
			: UWeaponRollLibrary::RollWeapon(Definition, Level);
		if (!Overrides.IsEmpty())
		{
			ForceParts(Instance, Overrides);
		}

		const bool bGiven = Give(*Inventory, Instance);
		UE_LOG(LogLooter, Log, TEXT("Looter.GiveWeapon: %s %s (level %d) %s."), *StaticEnum<EWeaponRarity>()->GetNameStringByValue(static_cast<int64>(Instance.Rarity)),
			*Definition->GetName(), Level, bGiven ? TEXT("given") : TEXT("did not fit"));
	}

	void SpawnAmmo(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.SpawnAmmo: no player (start the game first)."));
			return;
		}
		// One pickup of each class (or just the one named) in an arc a few meters ahead, dropped onto the ground.
		const int64 Only = Args.Num() > 0 ? StaticEnum<EAmmoType>()->GetValueByNameString(Args[0]) : INDEX_NONE;
		const FVector Ahead = Pawn->GetActorForwardVector().GetSafeNormal2D();
		const FVector Side = FVector::CrossProduct(FVector::UpVector, Ahead);
		int32 Spawned = 0;
		for (const EAmmoType Type : LooterAmmo::AllTypes())
		{
			if (Only != INDEX_NONE && static_cast<int64>(Type) != Only)
			{
				continue;
			}
			const float Offset = (static_cast<float>(static_cast<int32>(Type)) - (LooterAmmo::NumTypes - 1) * 0.5f) * 90.f;
			const FVector Spot = Pawn->GetActorLocation() + Ahead * 350.f + Side * (Only != INDEX_NONE ? 0.f : Offset) + FVector::UpVector * 40.f;
			if (AAmmoPickup* Pickup = AAmmoPickup::SpawnAmmo(GameWorld, Type, LooterAmmo::GetInfo(Type).PickupAmount, Spot))
			{
				Pickup->Toss(FVector::ZeroVector);
				++Spawned;
			}
		}
		UE_LOG(LogLooter, Log, TEXT("Looter.SpawnAmmo: %d pickups dropped ahead of the player."), Spawned);
	}

	FAutoConsoleCommandWithWorldAndArgs SpawnAmmoCommand(
		TEXT("Looter.SpawnAmmo"),
		TEXT("Looter.SpawnAmmo [AssaultRifle|Shotgun|Pistol|SMG|Sniper]: drops an ammo pickup of every class (or the one named) a few meters ahead of the player."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&SpawnAmmo));

	FAutoConsoleCommandWithWorldAndArgs GiveWeaponCommand(
		TEXT("Looter.GiveWeapon"),
		TEXT("Looter.GiveWeapon <weapon, e.g. Rifle or Shotgun> [Common|Uncommon|Rare|Epic|Legendary] [level] [Slot=Key ...]: gives the player ")
		TEXT("a weapon, into a free slot or else the backpack. The rarity is rolled when it's left out; Slot=Key (Sight=Variable) picks a part. ")
		TEXT("A named gun by its id, Looter.GiveWeapon Heirloom [level], comes with its own parts and rarity, at the player's level unless one is given."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveWeapon));
}

#endif
