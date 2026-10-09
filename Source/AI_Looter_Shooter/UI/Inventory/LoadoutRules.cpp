#include "UI/Inventory/LoadoutRules.h"
#include "UI/Style/WeaponText.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "Weapons/WeaponTypes.h"
#include "Internationalization/Text.h"
#include "Templates/TypeHash.h"

namespace
{
	/** Verdicts call guns within this share of each other's damage per second similar. */
	constexpr float SimilarShare = 0.03f;
	/** A part's bonus smaller than this (percent) rounds to nothing on the card, so it isn't listed. */
	constexpr float SmallestBonus = 0.5f;

	int32 StatDecimals(LoadoutRules::EStat Stat)
	{
		using LoadoutRules::EStat;
		switch (Stat)
		{
		case EStat::Damage:   return 1;
		case EStat::Reload:   return 2;
		case EStat::Accuracy: return 1;
		case EStat::Zoom:     return 2;
		default:              return 0;
		}
	}

	/** The unit a change is written with ("+0.25s"), so a lone number beside an arrow still says what it is. */
	const TCHAR* StatUnit(LoadoutRules::EStat Stat)
	{
		using LoadoutRules::EStat;
		switch (Stat)
		{
		case EStat::Reload:   return TEXT("s");
		case EStat::Accuracy: return TEXT("°");
		case EStat::Range:    return TEXT(" m");
		case EStat::Recoil:   return TEXT("%");
		case EStat::Handling: return TEXT("%");
		case EStat::Zoom:     return TEXT("x");
		default:              return TEXT("");
		}
	}

	/** Signed, with fewer decimals for big changes (the values beside them are rounded too). */
	FString FormatSigned(float Delta, int32 Decimals)
	{
		FNumberFormattingOptions Options;
		Options.UseGrouping = false;
		Options.MinimumFractionalDigits = 0;
		Options.MaximumFractionalDigits = FMath::Abs(Delta) >= 10.f ? 0 : Decimals;
		return (Delta > 0.f ? TEXT("+") : TEXT("")) + FText::AsNumber(Delta, &Options).ToString();
	}

	/** -1, 0 or 1: which of two numbers comes first when larger is first; nearly equal ones tie. */
	int32 Larger(float A, float B, float Tolerance)
	{
		if (FMath::IsNearlyEqual(A, B, Tolerance))
		{
			return 0;
		}
		return A > B ? -1 : 1;
	}
}

float LoadoutRules::DamagePerSecond(const FWeaponStats& Stats)
{
	return Stats.Damage * Stats.PelletsPerShot * Stats.FireRate / 60.f;
}

LoadoutRules::EVerdict LoadoutRules::Compare(const FWeaponInstanceData& Candidate, const FWeaponInstanceData* Current)
{
	if (!Current || !Candidate.Definition || Candidate.Definition != Current->Definition)
	{
		return EVerdict::None;
	}
	const float Theirs = DamagePerSecond(Candidate.Stats);
	const float Mine = DamagePerSecond(Current->Stats);
	if (Theirs > Mine * (1.f + SimilarShare))
	{
		return EVerdict::Upgrade;
	}
	return Theirs < Mine * (1.f - SimilarShare) ? EVerdict::Weaker : EVerdict::Similar;
}

// ---------------------------------------------------------------------------
// The backpack list's order
// ---------------------------------------------------------------------------

const TCHAR* LoadoutRules::SortName(ESort Sort)
{
	switch (Sort)
	{
	case ESort::Match:  return TEXT("Best for slot");
	case ESort::Rarity: return TEXT("Rarity");
	case ESort::Level:  return TEXT("Level");
	case ESort::Newest: return TEXT("Newest");
	default:            return TEXT("");
	}
}

LoadoutRules::ESort LoadoutRules::NextSort(ESort Sort)
{
	return static_cast<ESort>((static_cast<int32>(Sort) + 1) % static_cast<int32>(ESort::Count));
}

int32 LoadoutRules::SortBackpack(const TArray<FWeaponInstanceData>& Backpack, const FWeaponInstanceData* Target, ESort Sort,
	TArray<int32>& OutOrder)
{
	OutOrder.Reset(Backpack.Num());
	for (int32 Index = 0; Index < Backpack.Num(); ++Index)
	{
		OutOrder.Add(Index);
	}
	// "Best for slot" leads with the target's kind; without a target it lists by rarity, as nothing is being replaced.
	const UWeaponDefinition* Kind = Sort == ESort::Match && Target ? Target->Definition.Get() : nullptr;

	auto Before = [&Backpack, Sort, Kind](int32 A, int32 B)
	{
		const FWeaponInstanceData& GunA = Backpack[A];
		const FWeaponInstanceData& GunB = Backpack[B];
		const int32 Rarity = Larger(static_cast<float>(static_cast<int32>(GunA.Rarity)), static_cast<float>(static_cast<int32>(GunB.Rarity)), 0.f);
		const int32 Level = Larger(static_cast<float>(GunA.Level), static_cast<float>(GunB.Level), 0.f);
		const int32 Damage = Larger(DamagePerSecond(GunA.Stats), DamagePerSecond(GunB.Stats), 0.01f);
		int32 Order = 0;
		switch (Sort)
		{
		case ESort::Match:
		{
			const bool bKindA = Kind && GunA.Definition == Kind;
			const bool bKindB = Kind && GunB.Definition == Kind;
			if (bKindA != bKindB)
			{
				return bKindA;
			}
			// The target's kind, best first, so the biggest upgrade tops the list; the rest as by rarity.
			Order = bKindA ? (Damage != 0 ? Damage : (Rarity != 0 ? Rarity : Level)) : (Rarity != 0 ? Rarity : (Level != 0 ? Level : Damage));
			break;
		}
		case ESort::Rarity:
			Order = Rarity != 0 ? Rarity : (Level != 0 ? Level : Damage);
			break;
		case ESort::Level:
			Order = Level != 0 ? Level : (Rarity != 0 ? Rarity : Damage);
			break;
		case ESort::Newest:
			// Guns join the backpack at its end: the last is the latest.
			return A > B;
		default:
			break;
		}
		return Order != 0 ? Order < 0 : A < B;
	};
	OutOrder.StableSort(Before);

	if (!Kind)
	{
		return 0;
	}
	int32 NumKind = 0;
	for (const int32 Index : OutOrder)
	{
		NumKind += Backpack[Index].Definition == Kind ? 1 : 0;
	}
	return NumKind;
}

// ---------------------------------------------------------------------------
// Stats on the card
// ---------------------------------------------------------------------------

TConstArrayView<LoadoutRules::EStat> LoadoutRules::FirstGlanceStats()
{
	static const EStat Stats[] = { EStat::Damage, EStat::FireRate, EStat::Magazine, EStat::Reload, EStat::Accuracy };
	return MakeArrayView(Stats);
}

TConstArrayView<LoadoutRules::EStat> LoadoutRules::AllStats()
{
	static const EStat Stats[] = { EStat::Damage, EStat::FireRate, EStat::Magazine, EStat::Reload, EStat::Accuracy, EStat::Range,
		EStat::Recoil, EStat::Handling, EStat::Zoom };
	return MakeArrayView(Stats);
}

const TCHAR* LoadoutRules::StatName(EStat Stat)
{
	switch (Stat)
	{
	case EStat::Damage:   return TEXT("Damage");
	case EStat::FireRate: return TEXT("Fire rate");
	case EStat::Magazine: return TEXT("Magazine");
	case EStat::Reload:   return TEXT("Reload");
	case EStat::Accuracy: return TEXT("Accuracy");
	case EStat::Range:    return TEXT("Range");
	case EStat::Recoil:   return TEXT("Recoil");
	case EStat::Handling: return TEXT("Handling");
	case EStat::Zoom:     return TEXT("Zoom");
	default:              return TEXT("");
	}
}

float LoadoutRules::StatValue(EStat Stat, const FWeaponStats& S)
{
	switch (Stat)
	{
	// The whole shot, so shotguns and rifles line up fairly.
	case EStat::Damage:   return S.Damage * S.PelletsPerShot;
	case EStat::FireRate: return S.FireRate;
	case EStat::Magazine: return static_cast<float>(S.MagazineSize);
	case EStat::Reload:   return S.ReloadTime;
	case EStat::Accuracy: return S.Spread;
	case EStat::Range:    return S.Range / 100.f;
	case EStat::Recoil:   return S.Recoil * 100.f;
	case EStat::Handling: return S.Handling * 100.f;
	case EStat::Zoom:     return S.Zoom;
	default:              return 0.f;
	}
}

FString LoadoutRules::StatText(EStat Stat, const FWeaponStats& S)
{
	switch (Stat)
	{
	case EStat::Damage:   return LooterWeaponText::DamageString(S);
	case EStat::FireRate: return FString::Printf(TEXT("%.0f"), S.FireRate);
	case EStat::Magazine: return FString::FromInt(S.MagazineSize);
	case EStat::Reload:   return FString::Printf(TEXT("%.2fs"), S.ReloadTime);
	case EStat::Accuracy: return FString::Printf(TEXT("%.1f°"), S.Spread);
	case EStat::Range:    return FString::Printf(TEXT("%.0f m"), S.Range / 100.f);
	case EStat::Recoil:   return FString::Printf(TEXT("%.0f%%"), S.Recoil * 100.f);
	case EStat::Handling: return FString::Printf(TEXT("%.0f%%"), S.Handling * 100.f);
	case EStat::Zoom:     return LooterWeaponText::ZoomString(S);
	default:              return FString();
	}
}

float LoadoutRules::StatRating(EStat Stat, const FWeaponStats& S)
{
	switch (Stat)
	{
	case EStat::Damage:   return LooterWeaponText::DamageRating(S);
	case EStat::FireRate: return LooterWeaponText::FireRateRating(S);
	case EStat::Magazine: return LooterWeaponText::MagazineRating(S);
	case EStat::Reload:   return LooterWeaponText::ReloadRating(S);
	case EStat::Accuracy: return LooterWeaponText::AccuracyRating(S);
	case EStat::Range:    return LooterWeaponText::RangeRating(S);
	case EStat::Recoil:   return LooterWeaponText::RecoilRating(S);
	case EStat::Handling: return LooterWeaponText::HandlingRating(S);
	case EStat::Zoom:     return LooterWeaponText::ZoomRating(S);
	default:              return 0.f;
	}
}

bool LoadoutRules::HigherIsBetter(EStat Stat)
{
	return Stat != EStat::Reload && Stat != EStat::Accuracy && Stat != EStat::Recoil;
}

LoadoutRules::EChange LoadoutRules::CompareStat(EStat Stat, const FWeaponStats& New, const FWeaponStats& Old)
{
	const float NewValue = StatValue(Stat, New);
	const float OldValue = StatValue(Stat, Old);
	// Within what the card's rounding shows (or half a percent), it's the same: no arrow for a change nobody can see.
	const float Tolerance = FMath::Max(0.5f * FMath::Pow(10.f, -static_cast<float>(StatDecimals(Stat))), 0.005f * FMath::Abs(OldValue));
	if (FMath::Abs(NewValue - OldValue) < Tolerance)
	{
		return EChange::Same;
	}
	return (NewValue > OldValue) == HigherIsBetter(Stat) ? EChange::Better : EChange::Worse;
}

FString LoadoutRules::DeltaText(EStat Stat, const FWeaponStats& New, const FWeaponStats& Old)
{
	if (CompareStat(Stat, New, Old) == EChange::Same)
	{
		return FString();
	}
	return FormatSigned(StatValue(Stat, New) - StatValue(Stat, Old), StatDecimals(Stat)) + StatUnit(Stat);
}

// ---------------------------------------------------------------------------
// The parts' bonuses
// ---------------------------------------------------------------------------

TArray<LoadoutRules::FBonus> LoadoutRules::Bonuses(const FWeaponPartTotals& Totals)
{
	const TPair<EStat, float> Each[] = {
		{ EStat::Damage, Totals.Damage }, { EStat::Accuracy, Totals.Accuracy }, { EStat::Range, Totals.Range },
		{ EStat::FireRate, Totals.FireRate }, { EStat::Reload, Totals.Reload }, { EStat::Recoil, Totals.Recoil },
		{ EStat::Handling, Totals.Handling } };
	TArray<FBonus> Result;
	for (const TPair<EStat, float>& Pair : Each)
	{
		if (FMath::Abs(Pair.Value) < SmallestBonus)
		{
			continue;
		}
		FBonus& Bonus = Result.AddDefaulted_GetRef();
		Bonus.Stat = Pair.Key;
		Bonus.Percent = Pair.Value;
		// Less reload time and less recoil are what a player wants. (Not HigherIsBetter: the card's Accuracy row is the
		// spread, where less is better, but the parts' accuracy is the other way round: +40% shoots tighter.)
		const bool bLessIsBetter = Pair.Key == EStat::Reload || Pair.Key == EStat::Recoil;
		Bonus.Goodness = bLessIsBetter ? -Pair.Value : Pair.Value;
	}
	Result.StableSort([](const FBonus& A, const FBonus& B) { return A.Goodness > B.Goodness; });
	return Result;
}

TArray<LoadoutRules::FBonus> LoadoutRules::TopBonuses(const FWeaponPartTotals& Totals, int32 MaxCount)
{
	TArray<FBonus> Result = Bonuses(Totals);
	Result.RemoveAll([](const FBonus& Bonus) { return Bonus.Goodness <= 0.f; });
	if (Result.Num() > MaxCount)
	{
		Result.SetNum(FMath::Max(MaxCount, 0));
	}
	return Result;
}

FString LoadoutRules::BonusText(const FBonus& Bonus)
{
	const TCHAR* Name = Bonus.Stat == EStat::Reload ? TEXT("Reload time") : StatName(Bonus.Stat);
	return FString::Printf(TEXT("%+.0f%% %s"), Bonus.Percent, Name).ToUpper();
}

FWeaponPartTotals LoadoutRules::PartTotals(const FWeaponInstanceData& Gun)
{
	if (!Gun.Definition)
	{
		return FWeaponPartTotals();
	}
	// As UWeaponRollLibrary works the stats out: a named gun's parts sit at its fixed quality, others roll from the seed.
	const UNamedWeaponDefinition* Named = Gun.Named;
	return WeaponParts::CombinedStats(WeaponParts::Pick(Gun), Gun.Seed, Named ? TOptional<float>(Named->StatQuality) : TOptional<float>());
}

uint32 LoadoutRules::GunIdentity(const FWeaponInstanceData& Gun)
{
	uint32 Hash = GetTypeHash(Gun.Definition.Get());
	Hash = HashCombine(Hash, GetTypeHash(Gun.Named.Get()));
	Hash = HashCombine(Hash, GetTypeHash(Gun.Seed));
	Hash = HashCombine(Hash, GetTypeHash(static_cast<uint8>(Gun.Rarity)));
	return HashCombine(Hash, GetTypeHash(Gun.Level));
}
