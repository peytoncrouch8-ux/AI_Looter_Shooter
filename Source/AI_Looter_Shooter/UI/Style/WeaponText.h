#pragma once

#include "CoreMinimal.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Weapons/WeaponTypes.h"

/** Shared formatting for weapon names/stats across the HUD, loot labels and inventory. */
namespace LooterWeaponText
{
	/** "Epic Scoped Assault Rifle": the rarity, the word the gun's parts give it (if any) and the kind of gun. */
	inline FString Name(const FWeaponInstanceData& Instance)
	{
		const FString Rarity = UEnum::GetDisplayValueAsText(Instance.Rarity).ToString();
		if (!Instance.Definition)
		{
			return FString::Printf(TEXT("%s Unknown"), *Rarity);
		}
		const FText Prefix = WeaponParts::NamePrefix(WeaponParts::Pick(Instance));
		const FString Weapon = Instance.Definition->DisplayName.ToString();
		return Prefix.IsEmpty() ? FString::Printf(TEXT("%s %s"), *Rarity, *Weapon)
			: FString::Printf(TEXT("%s %s %s"), *Rarity, *Prefix.ToString(), *Weapon);
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

	/** Magnification as people say it: "1.25x", "4x". */
	inline FString ZoomString(const FWeaponStats& Stats)
	{
		const int32 Hundredths = FMath::RoundToInt(Stats.Zoom * 100.f);
		const int32 Decimals = Hundredths % 100 == 0 ? 0 : (Hundredths % 10 == 0 ? 1 : 2);
		return FString::Printf(TEXT("%.*fx"), Decimals, Hundredths / 100.f);
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
	inline float RangeRating(const FWeaponStats& S)    { return FMath::Clamp(S.Range / 6000.f, 0.f, 1.f); }
	/** Recoil and handling are multipliers around 1 (the gun's parts move them by up to about half). */
	inline float RecoilRating(const FWeaponStats& S)   { return FMath::Clamp(1.5f - S.Recoil, 0.f, 1.f); }
	inline float HandlingRating(const FWeaponStats& S) { return FMath::Clamp(S.Handling - 0.5f, 0.f, 1.f); }
	inline float ZoomRating(const FWeaponStats& S)     { return FMath::Clamp((S.Zoom - 1.f) / 7.f, 0.f, 1.f); }
}
