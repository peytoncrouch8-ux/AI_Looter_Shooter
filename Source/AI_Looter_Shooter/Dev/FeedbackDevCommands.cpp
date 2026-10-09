// Developer console commands for the feedback pass (not in shipping builds): a dropped gun's fanfare by rarity, and a hit
// on the player from a side (the hurt jolt and the damage indicator).

#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "AI_Looter_Shooter.h"
#include "Combat/LooterDamageTypes.h"
#include "Loot/LootFanfareSubsystem.h"
#include "Loot/LootLibrary.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/Engine.h"
#include "Engine/TargetPoint.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"

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

	/** Looter.Feedback.Drop [rarity] [rifle|shotgun]: a gun dropped 3 m ahead of the player, as a kill drops it. */
	void Drop(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Player = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		const APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Feedback.Drop: no player in a game world."));
			return;
		}
		EWeaponRarity Rarity = EWeaponRarity::Legendary;
		if (Args.Num() > 0)
		{
			const int64 Value = StaticEnum<EWeaponRarity>()->GetValueByNameString(Args[0]);
			Rarity = Value == INDEX_NONE ? Rarity : static_cast<EWeaponRarity>(Value);
		}
		UWeaponDefinition* Definition = FindDefinition(Args.Num() > 1 && Args[1].Equals(TEXT("shotgun"), ESearchCase::IgnoreCase) ? TEXT("Shotgun") : TEXT("Rifle"));
		if (!Definition)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Feedback.Drop: no weapon definition found."));
			return;
		}
		const FVector Ahead = FRotator(0.f, Player->GetControlRotation().Yaw, 0.f).Vector();
		const FVector Where = Pawn->GetActorLocation() + Ahead * 300.f + FVector(0.f, 0.f, 60.f);
		const FWeaponInstanceData Instance = ULootLibrary::RollDroppedWeapon(Definition, Rarity, 1);
		if (AWeaponBase* Weapon = UWeaponRollLibrary::SpawnWeapon(GameWorld, Instance, FTransform(Where)))
		{
			Weapon->Toss(Ahead * 120.f + FVector(0.f, 0.f, 480.f));
			ULootFanfareSubsystem::ExpectLanding(Weapon);
			UE_LOG(LogLooter, Display, TEXT("Looter.Feedback.Drop: %s %s."), *UEnum::GetValueAsString(Rarity), *GetNameSafe(Definition));
		}
	}

	/** Looter.Feedback.Hurt [damage] [ahead|right|left|behind]: the player hit from that side (a stand-in attacker 6 m off). */
	void Hurt(const TArray<FString>& Args, UWorld* World)
	{
		UWorld* GameWorld = FindGameWorld(World);
		const APlayerController* Player = GameWorld ? GameWorld->GetFirstPlayerController() : nullptr;
		APawn* Pawn = Player ? Player->GetPawn() : nullptr;
		if (!Pawn)
		{
			UE_LOG(LogLooter, Warning, TEXT("Looter.Feedback.Hurt: no player in a game world."));
			return;
		}
		const float Damage = Args.Num() > 0 ? FMath::Max(FCString::Atof(*Args[0]), 1.f) : 15.f;
		const FString Side = Args.Num() > 1 ? Args[1].ToLower() : TEXT("right");
		const float Turn = Side == TEXT("ahead") ? 0.f : Side == TEXT("left") ? -90.f : Side == TEXT("behind") ? 180.f : 90.f;
		const FVector Toward = FRotator(0.f, Player->GetControlRotation().Yaw + Turn, 0.f).Vector();
		FActorSpawnParameters Params;
		Params.ObjectFlags |= RF_Transient;
		Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		// The arc keeps pointing where the attacker was once it's gone.
		ATargetPoint* Attacker = GameWorld->SpawnActor<ATargetPoint>(Pawn->GetActorLocation() + Toward * 600.f, FRotator::ZeroRotator, Params);
		if (Attacker)
		{
			Attacker->SetLifeSpan(2.f);
		}
		UGameplayStatics::ApplyDamage(Pawn, Damage, nullptr, Attacker, UCreatureAttackDamageType::StaticClass());
	}

	FAutoConsoleCommandWithWorldAndArgs DropCommand(
		TEXT("Looter.Feedback.Drop"),
		TEXT("Looter.Feedback.Drop [Common|Uncommon|Rare|Epic|Legendary] [rifle|shotgun]: a gun dropped 3 m ahead as a kill drops ")
		TEXT("it, with its landing's fanfare (Legendary and a rifle by default)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Drop));

	FAutoConsoleCommandWithWorldAndArgs HurtCommand(
		TEXT("Looter.Feedback.Hurt"),
		TEXT("Looter.Feedback.Hurt [damage, 15] [ahead|right|left|behind]: hurts the player from that side (the view's jolt and ")
		TEXT("the damage indicator's arc)."),
		FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&Hurt));
}

#endif
