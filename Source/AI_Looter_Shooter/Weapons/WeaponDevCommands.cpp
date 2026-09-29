// Developer console commands for weapons and the inventory (not in shipping builds).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponManagerComponent.h"
#include "Weapons/WeaponRollLibrary.h"
#include "AI_Looter_Shooter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
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

	void GiveWeapon(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Controller = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Controller ? Controller->GetPawn() : nullptr;
		UWeaponManagerComponent* Inventory = Pawn ? Pawn->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
		if (!Inventory)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.GiveWeapon: no player with a weapon inventory (start the game first)."));
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
		const FWeaponInstanceData Instance = Rarity != INDEX_NONE
			? UWeaponRollLibrary::RollWeaponWithRarity(Definition, static_cast<EWeaponRarity>(Rarity), Level)
			: UWeaponRollLibrary::RollWeapon(Definition, Level);

		// Into a free slot if there is one, otherwise the backpack (never swapping out the weapon in hand).
		const bool bGiven = Inventory->GetWeapons().Num() < Inventory->MaxWeapons
			? Inventory->GiveWeapon(Instance) != nullptr
			: Inventory->AddToBackpack(Instance);
		UE_LOG(LogLooter, Log, TEXT("Looter.GiveWeapon: %s %s (level %d) %s."), *StaticEnum<EWeaponRarity>()->GetNameStringByValue(static_cast<int64>(Instance.Rarity)),
			*Definition->GetName(), Level, bGiven ? TEXT("given") : TEXT("did not fit"));
	}

	FAutoConsoleCommandWithWorldAndArgs GiveWeaponCommand(
		TEXT("Looter.GiveWeapon"),
		TEXT("Looter.GiveWeapon <weapon, e.g. Rifle or Shotgun> [Common|Uncommon|Rare|Epic|Legendary] [level]: gives the player a weapon, ")
		TEXT("into a free slot or else the backpack. The rarity is rolled when it's left out."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&GiveWeapon));
}

#endif
