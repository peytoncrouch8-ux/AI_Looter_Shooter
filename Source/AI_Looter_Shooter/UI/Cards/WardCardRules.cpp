#include "UI/Cards/WardCardRules.h"
#include "Shields/ShieldPickupRules.h"
#include "Shields/ShieldRules.h"
#include "Shields/ShieldText.h"
#include "UObject/Class.h"

// The ward card's words and numbers: the gun card's layout (GunCardRules) with a ward's capacity, recharge delay and
// recharge rate on its rows, its build and trait under them, and the take keys of ShieldPickupRules.

namespace
{
	/** Within this share the two numbers read as the same (ShieldRules::Compare's 3%). */
	constexpr float SameShare = 0.03f;

	/** "+20%" / "-15%" for a lean (1.2 / 0.85). */
	int32 LeanPercent(float Lean)
	{
		return FMath::RoundToInt32((Lean - 1.f) * 100.f);
	}
}

TConstArrayView<WardCardRules::EStat> WardCardRules::CardStats()
{
	static const EStat Stats[] = { EStat::Capacity, EStat::Delay, EStat::Rate };
	return MakeArrayView(Stats);
}

FString WardCardRules::StatLabel(EStat Stat)
{
	switch (Stat)
	{
	case EStat::Capacity: return TEXT("Capacity");
	case EStat::Delay:    return TEXT("Recharge delay");
	case EStat::Rate:     return TEXT("Recharge rate");
	default:              return FString();
	}
}

FString WardCardRules::StatValueText(EStat Stat, const FShieldStats& Stats)
{
	switch (Stat)
	{
	case EStat::Capacity: return ShieldText::CapacityString(Stats);
	case EStat::Delay:    return ShieldText::DelayString(Stats);
	case EStat::Rate:     return ShieldText::RateString(Stats);
	default:              return FString();
	}
}

LoadoutRules::EChange WardCardRules::CompareStat(EStat Stat, const FShieldStats& New, const FShieldStats& Old)
{
	float Mine = 0.f;
	float Theirs = 0.f;
	float Floor = 0.f;
	bool bHigherBetter = true;
	switch (Stat)
	{
	case EStat::Capacity:
		Mine = New.Capacity;
		Theirs = Old.Capacity;
		Floor = 1.f;
		break;
	case EStat::Delay:
		Mine = New.RechargeDelay;
		Theirs = Old.RechargeDelay;
		Floor = 0.1f;
		bHigherBetter = false;
		break;
	case EStat::Rate:
		Mine = New.RechargeRate;
		Theirs = Old.RechargeRate;
		Floor = 1.f;
		break;
	default:
		return LoadoutRules::EChange::Same;
	}
	// The same within 3%, and never a change smaller than what the card prints (a point, a tenth of a second).
	const float Tolerance = FMath::Max(SameShare * FMath::Abs(Theirs), Floor * 0.5f);
	if (FMath::Abs(Mine - Theirs) <= Tolerance)
	{
		return LoadoutRules::EChange::Same;
	}
	const bool bMore = Mine > Theirs;
	return bMore == bHigherBetter ? LoadoutRules::EChange::Better : LoadoutRules::EChange::Worse;
}

FString WardCardRules::ArrowText(LoadoutRules::EChange Change)
{
	switch (Change)
	{
	case LoadoutRules::EChange::Better: return TEXT("▲");
	case LoadoutRules::EChange::Worse:  return TEXT("▼");
	default:                            return FString();
	}
}

TArray<WardCardRules::FExtraRow> WardCardRules::ExtraRows(const FShieldInstanceData& Ward)
{
	TArray<FExtraRow> Rows;
	Rows.Add({ TEXT("Build"), ShieldText::BuildName(Ward.Build), false });
	const FString Trait = ShieldText::TraitName(Ward.Trait);
	if (!Trait.IsEmpty())
	{
		Rows.Add({ TEXT("Trait"), Trait, true });
	}
	return Rows;
}

FString WardCardRules::BuildLine(EShieldBuild Build)
{
	if (Build == EShieldBuild::Standard)
	{
		return FString();
	}
	// Said from the build's own lean (ShieldRules::LeanOf), so a retuned build's card follows it.
	const ShieldRules::FBuildLean Lean = ShieldRules::LeanOf(Build);
	auto Part = [](int32 Percent, const TCHAR* More, const TCHAR* Less, const TCHAR* What)
	{
		return Percent == 0 ? FString() : FString::Printf(TEXT("%s %d%% %s"), Percent > 0 ? More : Less, FMath::Abs(Percent), What);
	};
	TArray<FString> Parts;
	for (const FString& Said : { Part(LeanPercent(Lean.Capacity), TEXT("holds"), TEXT("holds"), Lean.Capacity >= 1.f ? TEXT("more") : TEXT("less")),
		Part(LeanPercent(Lean.Delay), TEXT("waits"), TEXT("wakes"), Lean.Delay >= 1.f ? TEXT("longer") : TEXT("sooner")),
		Part(LeanPercent(Lean.Rate), TEXT("refills"), TEXT("refills"), Lean.Rate >= 1.f ? TEXT("faster") : TEXT("slower")) })
	{
		if (!Said.IsEmpty())
		{
			Parts.Add(Said);
		}
	}
	return Parts.IsEmpty() ? FString() : FString::Printf(TEXT("%s: %s."), *ShieldText::BuildName(Build), *FString::Join(Parts, TEXT(", ")));
}

TArray<FString> WardCardRules::Lines(const FShieldInstanceData& Ward)
{
	TArray<FString> Out;
	const FString Trait = ShieldText::TraitLine(Ward);
	if (!Trait.IsEmpty())
	{
		Out.Add(Trait);
	}
	const FString Build = BuildLine(Ward.Build);
	if (!Build.IsEmpty())
	{
		Out.Add(Build);
	}
	return Out;
}

FString WardCardRules::VerdictWord(const FShieldInstanceData& Ward, const FShieldInstanceData* Worn)
{
	return Worn && Worn->IsSet() && ShieldRules::Compare(Ward, Worn) > 0 ? FString(TEXT("UPGRADE")) : FString();
}

FString WardCardRules::TopWord(const FShieldInstanceData& Ward)
{
	const UEnum* Enum = StaticEnum<EWeaponRarity>();
	const FString Rarity = Enum ? Enum->GetDisplayNameTextByValue(static_cast<int64>(Ward.Rarity)).ToString() : FString();
	return FString::Printf(TEXT("%s WARD"), *Rarity).ToUpper().TrimStartAndEnd();
}

TArray<GunCardRules::FKeyHint> WardCardRules::KeyHints(const FWardCardKeys& Keys)
{
	TArray<GunCardRules::FKeyHint> Hints;
	const FString HoldCap = FString::Printf(TEXT("HOLD %s"), *Keys.Key);
	switch (ShieldPickupRules::TapFor(Keys.bWorn, !Keys.bBackpackFull))
	{
	case ShieldPickupRules::ETap::Wear:
		// None worn: the tap wears it, so there's no hold to show.
		Hints.Add({ Keys.Key, TEXT("WEAR"), false, false });
		break;
	case ShieldPickupRules::ETap::ToBackpack:
		Hints.Add({ Keys.Key, TEXT("TO BACKPACK"), false, false });
		Hints.Add({ HoldCap, TEXT("WEAR"), true, false });
		break;
	default:
		// The backpack full: the tap wears it and the worn one drops where the player stands.
		Hints.Add({ Keys.Key, TEXT("SWAP"), false, false });
		Hints.Add({ FString(), TEXT("BACKPACK FULL"), false, true });
		break;
	}
	return Hints;
}
