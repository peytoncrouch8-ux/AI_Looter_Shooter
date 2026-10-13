#include "Missions/MissionRewards.h"
#include "AI_Looter_Shooter.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Areas/AreaDefinition.h"
#include "Economy/EconomyRules.h"
#include "Economy/WalletSubsystem.h"
#include "Loot/LootLibrary.h"
#include "Missions/MissionDefinition.h"
#include "Progression/XPCurve.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponBase.h"
#include "Weapons/WeaponDefinition.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"

namespace
{
	FString RarityName(EWeaponRarity Rarity)
	{
		return UEnum::GetDisplayValueAsText(Rarity).ToString();
	}

	/** Spawns the gun a couple of metres ahead of Player, tossed up so it lands and settles as loot does, beam and all. */
	AWeaponBase* TossAhead(UWorld* World, const FWeaponInstanceData& Instance, const AActor& Player)
	{
		// The player faces whoever they turned the mission in to, so the gun comes up between them, as if handed over.
		const FVector Ahead = Player.GetActorForwardVector().GetSafeNormal2D();
		const FVector Spot = Player.GetActorLocation() + Ahead * 150.f + FVector(0.f, 0.f, 60.f);
		AWeaponBase* Gun = UWeaponRollLibrary::SpawnWeapon(World, Instance, FTransform(FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f), Spot));
		if (Gun)
		{
			Gun->Toss(Ahead * 120.f + FVector(0.f, 0.f, 450.f));
		}
		return Gun;
	}
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
	AWeaponBase* Gun = TossAhead(World, Instance, *Player);
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
	AWeaponBase* Gun = TossAhead(World, Instance, *Player);
	if (Gun)
	{
		UE_LOG(LogLooter, Log, TEXT("Missions: named gun %s, %s %s (level %d)"), *Named->GetNamedId().ToString(), *RarityName(Instance.Rarity),
			*Named->Weapon->GetName(), Instance.Level);
	}
	return Gun;
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
