// Looter.Makers.Dump: the gun makers' odds and quirks as tuned now, for checking the cvars (not shipping).
#include "Affixes/WeaponMakers.h"
#include "Affixes/WeaponMakerTuning.h"
#include "AI_Looter_Shooter.h"
#include "HAL/IConsoleManager.h"

#if !UE_BUILD_SHIPPING

namespace
{
	/** An enum value by its name, any case; Fallback when none matches. */
	template <typename TEnum>
	TEnum ParseEnum(const FString& Word, TEnum Fallback)
	{
		const UEnum* Enum = StaticEnum<TEnum>();
		for (int32 Index = 0; Enum && Index < Enum->NumEnums() - 1; ++Index)
		{
			if (Enum->GetNameStringByIndex(Index).Equals(Word, ESearchCase::IgnoreCase))
			{
				return static_cast<TEnum>(Enum->GetValueByIndex(Index));
			}
		}
		return Fallback;
	}

	FAutoConsoleCommand DumpMakers(
		TEXT("Looter.Makers.Dump"),
		TEXT("Looter.Makers.Dump [Kind] [Rarity]: each maker's share of guns of that kind and rarity (default Rifle Common), from its "
			"weights and from 4000 seeded rolls, and its live quirk."),
		FConsoleCommandWithArgsDelegate::CreateLambda([](const TArray<FString>& Args)
		{
			const EWeaponKind Kind = Args.IsValidIndex(0) ? ParseEnum(Args[0], EWeaponKind::Rifle) : EWeaponKind::Rifle;
			const EWeaponRarity Rarity = Args.IsValidIndex(1) ? ParseEnum(Args[1], EWeaponRarity::Common) : EWeaponRarity::Common;
			float Total = 0.f;
			for (const FWeaponMaker& Maker : WeaponMakers::All())
			{
				Total += WeaponMakers::Weight(Maker, Kind, Rarity);
			}
			TMap<FName, int32> Rolled;
			constexpr int32 Rolls = 4000;
			for (int32 Seed = 0; Seed < Rolls; ++Seed)
			{
				++Rolled.FindOrAdd(WeaponMakers::Roll(Kind, Seed * 7919 + 13, Rarity));
			}
			UE_LOG(LogLooter, Display, TEXT("MAKERS %s %s (quirk scale %.2f, default %s)"), *UEnum::GetValueAsString(Kind),
				*UEnum::GetValueAsString(Rarity), WeaponMakerTuning::QuirkScale(), *WeaponMakers::DefaultKey().ToString());
			for (const FWeaponMaker& Maker : WeaponMakers::All())
			{
				const float Weight = WeaponMakers::Weight(Maker, Kind, Rarity);
				const FWeaponMakerQuirk Q = WeaponMakers::QuirkOf(Maker);
				UE_LOG(LogLooter, Display, TEXT("  %-10s weight %.2f share %4.1f%% rolled %4.1f%% | dmg %+.0f acc %+.0f rng %+.0f rate %+.0f reload %+.0f "
					"recoil %+.0f handling %+.0f mag %+.0f wear %+.2f element x%.1f effect x%.1f"), *Maker.Key.ToString(), Weight,
					Total > 0.f ? 100.f * Weight / Total : 0.f, 100.f * Rolled.FindRef(Maker.Key) / Rolls, Q.Damage, Q.Accuracy, Q.Range,
					Q.FireRate, Q.Reload, Q.Recoil, Q.Handling, Q.Magazine, Q.Wear, Q.ElementPartBias, Q.EffectPartBias);
			}
		}));
}

#endif
