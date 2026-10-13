#include "Missions/MissionRewards.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Areas/AreaDefinition.h"
#include "Economy/EconomyRules.h"
#include "Economy/WalletSubsystem.h"
#include "Inventory/WeaponManagerComponent.h"
#include "Loot/LootFanfareSubsystem.h"
#include "Loot/LootLibrary.h"
#include "Missions/MissionDefinition.h"
#include "Progression/XPCurve.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "World/WorldQueries.h"
#include "CollisionQueryParams.h"
#include "CollisionShape.h"
#include "Engine/EngineTypes.h"
#include "Engine/HitResult.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"

namespace
{
	/** How far ahead of the player a reward gun comes up (cm), over their middle by RiseAbove. */
	constexpr float AheadDistance = 150.f;
	constexpr float RiseAbove = 60.f;
	/** Its toss: up, and on toward the giver only where that's clear; how far that carries it on (about, cm). */
	constexpr float TossUp = 450.f;
	constexpr float TossOn = 120.f;
	constexpr float TossCarries = 140.f;
	/** How far short of a wall, window or fence ahead it keeps (cm), and the gun's size for the look (its loot sphere). */
	constexpr float WallClearance = 45.f;
	constexpr float GunRadius = 15.f;

	FString RarityName(EWeaponRarity Rarity)
	{
		return UEnum::GetDisplayValueAsText(Rarity).ToString();
	}
}

AWeaponBase* MissionRewards::DropInFront(UWorld* World, const FWeaponInstanceData& Instance, const AActor& Player)
{
	if (!World)
	{
		return nullptr;
	}
	// The player faces whoever they turned the mission in to, so the gun comes up between them, as if handed over. Givers
	// stand behind things, though (Tilly's shop window, Delia's screen door, Amos's fence): 1.5 m ahead was often on the
	// far side, and the gun fell into the shop or the yard, never seen. So it looks first, the whole way the toss would
	// carry it, and keeps on this side of whatever is there.
	const FVector Ahead = Player.GetActorForwardVector().GetSafeNormal2D();
	const FVector From = Player.GetActorLocation() + FVector(0.f, 0.f, RiseAbove);
	const float Look = AheadDistance + TossCarries + WallClearance;
	float Clear = Look;
	FHitResult Hit;
	const FCollisionQueryParams Params = LooterWorld::StaticGeometryParams(World, TEXT("MissionRewardDrop"), &Player, /*bTraceComplex*/ false);
	if (!Ahead.IsNearlyZero() && World->SweepSingleByObjectType(Hit, From, From + Ahead * Look, FQuat::Identity,
		FCollisionObjectQueryParams(ECC_WorldStatic), FCollisionShape::MakeSphere(GunRadius), Params))
	{
		Clear = Hit.bStartPenetrating ? 0.f : Hit.Distance;
	}
	// The whole toss clear: up ahead and on a little, as always. Something in the way: up from this side of it, and down
	// again where it rose (at the player's feet when they stand right at the window).
	const bool bAllClear = Clear >= Look - KINDA_SMALL_NUMBER;
	const float Out = bAllClear ? AheadDistance : FMath::Clamp(Clear - WallClearance, 0.f, AheadDistance);
	const FVector Spot = From + Ahead * Out;
	AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Spot));
	if (!Gun)
	{
		return nullptr;
	}
	Gun->Toss((bAllClear ? Ahead * TossOn : FVector::ZeroVector) + FVector(0.f, 0.f, TossUp));
	// Its rarity heard and its beam flaring as it lands, as any dropped gun's (an Epic or better announced).
	ULootFanfareSubsystem::ExpectLanding(Gun);
	return Gun;
}

int64 MissionRewards::ExperienceFor(float Share, int32 Level, const FXPCurve& Curve)
{
	if (Share <= 0.f)
	{
		return 0;
	}
	const int64 LevelTakes = Curve.XPToNextLevel(Level);
	return LevelTakes > 0 ? FMath::Max<int64>(1, static_cast<int64>(FMath::RoundToDouble(static_cast<double>(LevelTakes) * Share))) : 0;
}

int64 MissionRewards::ExperienceOf(const FMissionRewards& Rewards, int32 Level, const FXPCurve& Curve)
{
	if (Rewards.Experience > 0)
	{
		// Past the last level experience stops counting, fixed amounts too.
		return Curve.IsMaxLevel(Level) ? 0 : static_cast<int64>(Rewards.Experience);
	}
	return ExperienceFor(Rewards.ExperienceShare, Level, Curve);
}

EWeaponRarity MissionRewards::ApplyFloor(EWeaponRarity Rolled, EWeaponRarity Floor)
{
	return static_cast<uint8>(Rolled) < static_cast<uint8>(Floor) ? Floor : Rolled;
}

AWeaponBase* MissionRewards::DropGun(UWorld* World, const FMissionRewards& Rewards, int32 Level, const AActor* Player)
{
	if (!World || !Rewards.bGun || !Player)
	{
		return nullptr;
	}
	UWeaponDefinition* Kind = Rewards.GunKind.Get();
	if (!Kind)
	{
		// No kind named: whatever the game's own loot would drop, by the default table's weights.
		FRandomStream Random(FMath::Rand());
		Kind = ULootLibrary::PickWeaponWith(ULootLibrary::GetDefaultLootTable(), Random);
	}
	if (!Kind)
	{
		UE_LOG(LogLooter, Warning, TEXT("Missions: a reward gun had no kind, and the default loot table offered none."));
		return nullptr;
	}
	const EWeaponRarity Rarity = ApplyFloor(UWeaponRollLibrary::RollRarity(Kind), Rewards.GunRarityFloor);
	const FWeaponInstanceData Instance = UWeaponRollLibrary::RollWeaponWithRarity(Kind, Rarity, FMath::Max(Level, 1));
	AWeaponBase* Gun = DropInFront(World, Instance, *Player);
	if (Gun)
	{
		UE_LOG(LogLooter, Log, TEXT("Missions: reward gun %s %s (level %d)"), *RarityName(Rarity), *Kind->GetName(), Instance.Level);
	}
	return Gun;
}

AWeaponBase* MissionRewards::DropNamedGun(UWorld* World, const FMissionRewards& Rewards, int32 Level, const AActor* Player)
{
	if (!World || Rewards.NamedGun.IsNone() || !Player)
	{
		return nullptr;
	}
	UNamedWeaponDefinition* Named = UNamedWeaponDefinition::FindByName(Rewards.NamedGun.ToString());
	if (!Named || !Named->Weapon)
	{
		UE_LOG(LogLooter, Warning, TEXT("Missions: no named gun %s (%s%s in %s, made by Tools/Unreal/create_named_weapons.py), so it isn't given."),
			*Rewards.NamedGun.ToString(), UNamedWeaponDefinition::AssetPrefix, *Rewards.NamedGun.ToString(), UNamedWeaponDefinition::AssetFolder);
		return nullptr;
	}
	// Its own parts, rarity and fixed stats; only its level is the player's, as every reward gun's is.
	const FWeaponInstanceData Instance = Named->MakeInstance(Level);
	AWeaponBase* Gun = DropInFront(World, Instance, *Player);
	if (Gun)
	{
		UE_LOG(LogLooter, Log, TEXT("Missions: named gun %s, %s %s (level %d)"), *Named->GetNamedId().ToString(), *RarityName(Instance.Rarity),
			*Named->Weapon->GetName(), Instance.Level);
	}
	return Gun;
}

FNamedGunWhereabouts MissionRewards::FindNamedGun(UWorld* World, const UNamedWeaponDefinition* Named, const AActor* Player)
{
	FNamedGunWhereabouts Found;
	if (!World || !Named)
	{
		return Found;
	}
	// In the level: lying as loot, or held by someone (a gun in a slot is an actor its holder owns, in hand or on their back).
	for (TActorIterator<AWeaponBase> It(World); It; ++It)
	{
		AWeaponBase* Gun = *It;
		if (!IsValid(Gun) || Gun->IsActorBeingDestroyed() || Gun->GetInstance().Named != Named)
		{
			continue;
		}
		if (Gun->IsPickup())
		{
			Found.Lying.Add(Gun);
		}
		else if (Gun->GetOwner())
		{
			Found.bCarried = true;
		}
	}
	// In the backpack, where a gun is only its data.
	const UWeaponManagerComponent* Inventory = Player ? Player->FindComponentByClass<UWeaponManagerComponent>() : nullptr;
	if (Inventory && Inventory->GetBackpack().ContainsByPredicate([Named](const FWeaponInstanceData& Gun) { return Gun.Named == Named; }))
	{
		Found.bCarried = true;
	}
	return Found;
}

AWeaponBase* MissionRewards::EnsureNamedGun(UWorld* World, const FMissionRewards& Rewards, int32 Level, const AActor* Player)
{
	if (!World || Rewards.NamedGun.IsNone() || !Player)
	{
		return nullptr;
	}
	UNamedWeaponDefinition* Named = UNamedWeaponDefinition::FindByName(Rewards.NamedGun.ToString());
	if (!Named || !Named->Weapon)
	{
		UE_LOG(LogLooter, Warning, TEXT("Missions: no named gun %s (%s%s in %s, made by Tools/Unreal/create_named_weapons.py), so it isn't given."),
			*Rewards.NamedGun.ToString(), UNamedWeaponDefinition::AssetPrefix, *Rewards.NamedGun.ToString(), UNamedWeaponDefinition::AssetFolder);
		return nullptr;
	}
	const FNamedGunWhereabouts Where = FindNamedGun(World, Named, Player);
	if (Where.bCarried)
	{
		UE_LOG(LogLooter, Log, TEXT("Missions: %s was handed over and is carried: nothing more given."), *Named->GetNamedId().ToString());
		return nullptr;
	}
	if (!Where.Lying.IsEmpty())
	{
		// Handed over but never taken (still on Delia's porch, or dropped somewhere here): the reward is announced now, so the
		// very same gun (its level, its notches) comes up in front of the player, and none is left behind.
		AWeaponBase* Left = Where.Lying[0];
		const FWeaponInstanceData Instance = Left->GetInstanceForStorage();
		Left->Destroy();
		AWeaponBase* Brought = DropInFront(World, Instance, *Player);
		UE_LOG(LogLooter, Log, TEXT("Missions: %s was never taken: brought to the player at the turn-in."), *Named->GetNamedId().ToString());
		return Brought;
	}
	// Never handed out in this session (saved before it came out, or turned in before her words were done): given now.
	UE_LOG(LogLooter, Log, TEXT("Missions: %s was never handed over: given at the turn-in."), *Named->GetNamedId().ToString());
	return DropNamedGun(World, Rewards, Level, Player);
}

FText MissionRewards::NamedGunName(const FMissionRewards& Rewards)
{
	if (Rewards.NamedGun.IsNone())
	{
		return FText::GetEmpty();
	}
	// Its own name, or its id as words when its asset isn't there.
	const UNamedWeaponDefinition* Named = UNamedWeaponDefinition::FindByName(Rewards.NamedGun.ToString());
	return Named && !Named->DisplayName.IsEmpty() ? Named->DisplayName
		: FText::FromString(FName::NameToDisplayString(Rewards.NamedGun.ToString(), /*bIsBool*/ false));
}

FText MissionRewards::AreaName(FName AreaId)
{
	const UAreaDefinition* Area = UAreaDefinition::FindByName(AreaId.ToString());
	return Area && !Area->DisplayName.IsEmpty() ? Area->DisplayName : FText::FromName(AreaId);
}

int32 MissionRewards::SilverFor(FName MissionId)
{
	// Each giver's purse on Ransom's Rest, by the mission ids the campaign record keeps (never renamed). Docs/Progression.md
	// has the table and the playthrough it's balanced for.
	struct FPurse
	{
		const TCHAR* Mission;
		int32 Silver;
	};
	static const FPurse Purses[] = {
		{ TEXT("Main1"), 20 },   // Delia, through the screen door: Grandpa's egg money, pushed under it
		{ TEXT("Main3"), 40 },   // Tilly: what was in the pockets of the undertaker's unclaimed dead
		{ TEXT("Side1"), 50 },   // Tilly, once the posters are down: the reward money the town put up, which nobody can claim now
		{ TEXT("Main4"), 40 },   // Father Aldana: the poor box, for the bell rung again
		{ TEXT("Side2"), 60 },   // Amos: a season's wages he never drew
		{ TEXT("Main5"), 50 },   // Father Aldana, for the lantern brought home
		{ TEXT("Main7"), 60 },   // Tilly, for the road: her father's fare money
	};
	for (const FPurse& Purse : Purses)
	{
		if (MissionId == FName(Purse.Mission))
		{
			return Purse.Silver;
		}
	}
	return 0;
}

int32 MissionRewards::GiveSilver(UWorld* World, FName MissionId)
{
	const int32 Purse = SilverFor(MissionId);
	UWalletSubsystem* Wallet = Purse > 0 ? UWalletSubsystem::Get(World) : nullptr;
	if (!Wallet)
	{
		return 0;
	}
	const int32 Given = Wallet->Add(Purse, ESilverChange::Reward);
	UE_LOG(LogLooter, Log, TEXT("Missions: %s pays %d silver."), *MissionId.ToString(), Given);
	return Given;
}

TArray<FString> MissionRewards::Describe(const FMissionRewards& Rewards, FName MissionId)
{
	TArray<FString> Lines;
	if (Rewards.Experience > 0)
	{
		Lines.Add(FString::Printf(TEXT("+%s XP"), *FText::AsNumber(Rewards.Experience).ToString()));
	}
	else if (Rewards.ExperienceShare > 0.f)
	{
		Lines.Add(FString::Printf(TEXT("+%d%% of a level's experience"), FMath::RoundToInt32(Rewards.ExperienceShare * 100.f)));
	}
	// The giver's purse, beside the experience (a fixed amount at any level, so it can be named plainly).
	if (const int32 Purse = SilverFor(MissionId); Purse > 0)
	{
		Lines.Add(FString::Printf(TEXT("+%s silver"), *LooterEconomy::Format(Purse)));
	}
	if (Rewards.bGun)
	{
		// "Gun: Uncommon or better", "Gun: Shotgun, Rare or better".
		FString Gun = Rewards.GunKind && !Rewards.GunKind->DisplayName.IsEmpty() ? Rewards.GunKind->DisplayName.ToString() + TEXT(", ") : FString();
		Gun += Rewards.GunRarityFloor == EWeaponRarity::Common ? FString(TEXT("any rarity")) : RarityName(Rewards.GunRarityFloor) + TEXT(" or better");
		Lines.Add(TEXT("Gun: ") + Gun);
	}
	if (!Rewards.NamedGun.IsNone())
	{
		Lines.Add(TEXT("Named gun: ") + NamedGunName(Rewards).ToString());
	}
	for (const FName AreaId : Rewards.UnlockAreas)
	{
		Lines.Add(FString::Printf(TEXT("Opens %s"), *AreaName(AreaId).ToString()));
	}
	return Lines;
}
