#include "Economy/EconomyRules.h"
#include "Creatures/CreatureRank.h"
#include "Loot/Chest.h"
#include "Math/RandomStream.h"
#include "Story/HeroLines.h"
#include "Weapons/NamedWeaponDefinition.h"

#define LOCTEXT_NAMESPACE "LooterEconomy"

namespace
{
	/** A kill's silver at level 1, by rank (EnumIndex order: Basic, Restless, Gravebound, Soulfed, Boss). */
	const LooterEconomy::FDropOdds UnpaidKills[] = {
		{ 0.55f, 3, 7 },
		{ 0.85f, 9, 15 },
		{ 1.f, 18, 30 },
		{ 1.f, 50, 80 },
		{ 1.f, 100, 140 },
	};
	const LooterEconomy::FDropOdds BeastKills[] = {
		{ 0.2f, 2, 4 },
		{ 0.5f, 5, 9 },
		{ 0.8f, 10, 18 },
		{ 1.f, 40, 60 },
		{ 1.f, 80, 120 },
	};
	static_assert(UE_ARRAY_COUNT(UnpaidKills) == LooterRanks::NumRanks && UE_ARRAY_COUNT(BeastKills) == LooterRanks::NumRanks,
		"One kill drop per rank");

	/** What a vendor pays and asks at level 1, in EWeaponRarity order. A vendor never stocks a Common. */
	constexpr int32 SellBases[] = { 8, 18, 40, 90, 200 };
	constexpr int32 BuyBases[] = { 70, 150, 320, 700, 1500 };

	/** The gun of the day's rarity odds, in EWeaponRarity order. */
	constexpr float OfferChances[] = { 0.f, 0.70f, 0.24f, 0.05f, 0.01f };

	/** A box of rounds, every type alike, and a jar of grave salt, at level 1. */
	constexpr int32 AmmoBoxBase = 15;
	constexpr int32 GrenadeBase = 25;

	/** A box holds a chest pickup's rounds, at most this share of what can be carried. */
	constexpr int32 BoxRounds = 36;
	constexpr float BoxMaxShare = 0.5f;

	int32 RarityIndex(EWeaponRarity Rarity)
	{
		return FMath::Clamp(static_cast<int32>(Rarity), 0, 4);
	}
}

float LooterEconomy::LevelScale(int32 Level)
{
	return 1.f + LevelGrowth * static_cast<float>(FMath::Max(Level, 1) - 1);
}

int32 LooterEconomy::AtLevel(int32 Base, int32 Level)
{
	if (Base <= 0)
	{
		return 0;
	}
	return FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Base) * LevelScale(Level)));
}

FString LooterEconomy::Format(int32 Silver)
{
	return FText::AsNumber(Silver).ToString();
}

FText LooterEconomy::Words(int32 Silver)
{
	return FText::Format(LOCTEXT("SilverWords", "{0} silver"), FText::AsNumber(Silver));
}

LooterEconomy::FDropOdds LooterEconomy::KillOdds(ECreatureRank Rank, bool bUnpaid)
{
	const int32 Index = FMath::Clamp(static_cast<int32>(Rank), 0, LooterRanks::NumRanks - 1);
	return bUnpaid ? UnpaidKills[Index] : BeastKills[Index];
}

LooterEconomy::FDropOdds LooterEconomy::ContainerOdds(EChestKind Kind)
{
	switch (Kind)
	{
	// Ruth's Ranger caches and the camps' crates: a little coin with the supplies.
	case EChestKind::SupplyCrate: return { 0.9f, 10, 20 };
	// The gang's Strongbox: Sexton's advance, what's left of it.
	case EChestKind::Strongbox:   return { 1.f, 40, 70 };
	// The dead were buried with what was in their pockets (never grave gold: that's the old fare, long gone).
	case EChestKind::Coffin:      return { 0.7f, 6, 14 };
	case EChestKind::Grave:       return { 0.75f, 6, 16 };
	// Letters with a dollar or two folded in.
	case EChestKind::Mailbox:     return { 0.8f, 4, 10 };
	// A traveller's or a miner's savings.
	case EChestKind::Footlocker:  return { 0.9f, 8, 18 };
	}
	return { 0.f, 0, 0 };
}

LooterEconomy::FDropOdds LooterEconomy::BreakableOdds()
{
	return { 0.25f, 2, 6 };
}

int32 LooterEconomy::Roll(const FDropOdds& Odds, int32 Level, FRandomStream& Random)
{
	// Both numbers every time, so a roll made after this one sees the same stream whatever this one gave.
	const float Chance = Random.FRand();
	const int32 Amount = Random.RandRange(FMath::Min(Odds.Min, Odds.Max), FMath::Max(Odds.Min, Odds.Max));
	if (Chance >= Odds.Chance || Amount <= 0)
	{
		return 0;
	}
	return AtLevel(Amount, Level);
}

float LooterEconomy::Expected(const FDropOdds& Odds, int32 Level)
{
	return Odds.Average() * LevelScale(Level);
}

int32 LooterEconomy::AmmoBoxRounds(EAmmoType Type)
{
	const int32 Carried = LooterAmmo::GetInfo(Type).MaxCarried;
	return FMath::Max(1, FMath::Min(BoxRounds, FMath::FloorToInt32(static_cast<float>(Carried) * BoxMaxShare)));
}

int32 LooterEconomy::AmmoBoxPrice(EAmmoType Type, int32 PlayerLevel)
{
	return AtLevel(AmmoBoxBase, PlayerLevel);
}

int32 LooterEconomy::AmmoPrice(EAmmoType Type, int32 Rounds, int32 PlayerLevel)
{
	if (Rounds <= 0)
	{
		return 0;
	}
	const int32 Box = AmmoBoxRounds(Type);
	const int32 Full = AmmoBoxPrice(Type, PlayerLevel);
	if (Rounds >= Box)
	{
		return Full;
	}
	// The box's share, rounded up: a few rounds to top off never come free.
	return FMath::Clamp(FMath::CeilToInt32(static_cast<float>(Full) * static_cast<float>(Rounds) / static_cast<float>(Box)), 1, Full);
}

int32 LooterEconomy::GrenadePrice(int32 PlayerLevel)
{
	return AtLevel(GrenadeBase, PlayerLevel);
}

int32 LooterEconomy::SellBase(EWeaponRarity Rarity)
{
	return SellBases[RarityIndex(Rarity)];
}

int32 LooterEconomy::BuyBase(EWeaponRarity Rarity)
{
	return BuyBases[RarityIndex(Rarity)];
}

int32 LooterEconomy::GunBuyPrice(const FWeaponInstanceData& Gun)
{
	return AtLevel(BuyBase(Gun.Rarity), Gun.Level);
}

int32 LooterEconomy::GunSellPrice(const FWeaponInstanceData& Gun)
{
	if (!CanSell(Gun))
	{
		return 0;
	}
	const int32 Price = AtLevel(SellBase(Gun.Rarity), Gun.Level);
	return Gun.Curse.IsNone() ? Price : FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Price) * CursedSellShare));
}

bool LooterEconomy::CanSell(const FWeaponInstanceData& Gun, FText* OutWhy)
{
	if (!Gun.Definition)
	{
		if (OutWhy)
		{
			*OutWhy = LOCTEXT("NoGun", "There's no gun there.");
		}
		return false;
	}
	if (Gun.Named)
	{
		if (OutWhy)
		{
			const FText Name = Gun.Named->DisplayName.IsEmpty() ? LOCTEXT("ThisGun", "This gun") : Gun.Named->DisplayName;
			// The pattern in the played hero's words before the gun's name goes in ("the family" is Ellis's: Story/HeroLines).
			*OutWhy = FText::Format(HeroLines::Resolve(LOCTEXT("NamedNotForSale", "{0} isn't for sale. Some things stay in the family.")), Name);
		}
		return false;
	}
	return true;
}

bool LooterEconomy::SaleNeedsConfirm(const FWeaponInstanceData& Gun)
{
	return static_cast<uint8>(Gun.Rarity) >= static_cast<uint8>(EWeaponRarity::Rare);
}

float LooterEconomy::OfferRarityChance(EWeaponRarity Rarity)
{
	return OfferChances[RarityIndex(Rarity)];
}

EWeaponRarity LooterEconomy::RollOfferRarity(FRandomStream& Random)
{
	float Pick = Random.FRand();
	for (int32 Index = 0; Index < UE_ARRAY_COUNT(OfferChances); ++Index)
	{
		if (Pick < OfferChances[Index])
		{
			return static_cast<EWeaponRarity>(Index);
		}
		Pick -= OfferChances[Index];
	}
	return EWeaponRarity::Uncommon;
}

#undef LOCTEXT_NAMESPACE
