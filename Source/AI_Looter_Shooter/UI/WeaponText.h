#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponRollLibrary.h"
#include "Weapons/WeaponTypes.h"

/** Shared formatting for weapon names/stats across the HUD, loot labels and inventory. */
namespace LooterWeaponText
{
	inline FString Name(const FWeaponInstanceData& Instance)
	{
		const FString Rarity = UEnum::GetDisplayValueAsText(Instance.Rarity).ToString();
		const FString Weapon = Instance.Definition ? Instance.Definition->DisplayName.ToString() : TEXT("Unknown");
		return FString::Printf(TEXT("%s %s"), *Rarity, *Weapon);
	}

	inline FLinearColor Color(const FWeaponInstanceData& Instance)
	{
		return UWeaponRollLibrary::GetRarityColor(Instance.Definition, Instance.Rarity);
	}

	/**
	 * The weapon's damage, per pellet for shotguns ("7 x9"). This is the number the weapon rolled with; each hit then
	 * lands within LooterCombat::DamageVariance of it (and critical hits deal 1.5x), which the card doesn't spell out.
	 */
	inline FString DamageString(const FWeaponStats& Stats)
	{
		const FString Damage = FString::FromInt(FMath::Max(1, FMath::RoundToInt(Stats.Damage)));
		return Stats.PelletsPerShot > 1 ? FString::Printf(TEXT("%s x%d"), *Damage, Stats.PelletsPerShot) : Damage;
	}

	inline FString FireModeName(const FWeaponInstanceData& Instance)
	{
		return Instance.Definition ? UEnum::GetDisplayValueAsText(Instance.Definition->FireMode).ToString() : FString();
	}

	/** 0-1 ratings for stat bars (rough scale across all weapon types). */
	inline float DamageRating(const FWeaponStats& S)   { return FMath::Clamp(S.Damage * S.PelletsPerShot / 90.f, 0.f, 1.f); }
	inline float FireRateRating(const FWeaponStats& S) { return FMath::Clamp(S.FireRate / 900.f, 0.f, 1.f); }
	inline float MagazineRating(const FWeaponStats& S) { return FMath::Clamp(S.MagazineSize / 50.f, 0.f, 1.f); }
	inline float ReloadRating(const FWeaponStats& S)   { return FMath::Clamp(1.f - (S.ReloadTime - 1.f) / 3.f, 0.f, 1.f); }
	inline float AccuracyRating(const FWeaponStats& S) { return FMath::Clamp(1.f - S.Spread / 6.f, 0.f, 1.f); }
}
