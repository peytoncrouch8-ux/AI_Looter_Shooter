#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponBase.h"

namespace
{
	FWeaponRarityInfo MakeRarity(float Weight, float Damage, float FireRate, float Magazine, float Reload, float Spread, const FLinearColor& Color)
	{
		FWeaponRarityInfo Info;
		Info.Weight = Weight;
		Info.DamageMultiplier = Damage;
		Info.FireRateMultiplier = FireRate;
		Info.MagazineMultiplier = Magazine;
		Info.ReloadTimeMultiplier = Reload;
		Info.SpreadMultiplier = Spread;
		Info.Color = Color;
		return Info;
	}
}

UWeaponDefinition::UWeaponDefinition()
{
	WeaponClass = AWeaponBase::StaticClass();

	//                                                  Weight  Dmg    Rate   Mag    Reload Spread
	RarityTable.Add(EWeaponRarity::Common,    MakeRarity(60.f, 1.00f, 1.00f, 1.00f, 1.00f, 1.00f, FLinearColor(0.8f, 0.8f, 0.8f)));
	RarityTable.Add(EWeaponRarity::Uncommon,  MakeRarity(25.f, 1.10f, 1.03f, 1.10f, 0.95f, 0.95f, FLinearColor(0.2f, 0.8f, 0.2f)));
	RarityTable.Add(EWeaponRarity::Rare,      MakeRarity(10.f, 1.25f, 1.06f, 1.20f, 0.90f, 0.90f, FLinearColor(0.2f, 0.4f, 1.0f)));
	RarityTable.Add(EWeaponRarity::Epic,      MakeRarity( 4.f, 1.45f, 1.10f, 1.35f, 0.85f, 0.80f, FLinearColor(0.6f, 0.2f, 0.9f)));
	RarityTable.Add(EWeaponRarity::Legendary, MakeRarity( 1.f, 1.75f, 1.15f, 1.50f, 0.75f, 0.70f, FLinearColor(1.0f, 0.6f, 0.0f)));
}

FPrimaryAssetId UWeaponDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("WeaponDefinition"), GetFName());
}

const FWeaponRarityInfo& UWeaponDefinition::GetRarityInfo(EWeaponRarity Rarity) const
{
	static const FWeaponRarityInfo Default;
	const FWeaponRarityInfo* Found = RarityTable.Find(Rarity);
	return Found ? *Found : Default;
}
