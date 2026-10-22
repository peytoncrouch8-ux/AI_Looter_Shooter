#include "Shields/ShieldDefinition.h"
#include "Engine/StaticMesh.h"

const TCHAR* UShieldDefinition::WardBoxPath = TEXT("/Game/Data/Shields/DA_Shield_WardBox.DA_Shield_WardBox");

namespace
{
	FShieldRarityInfo MakeShieldRarity(float Weight, float Capacity, float Delay, float Rate, float TraitChance, float Strength,
		const FLinearColor& Color, float Glow)
	{
		FShieldRarityInfo Info;
		Info.Weight = Weight;
		Info.CapacityMultiplier = Capacity;
		Info.DelayMultiplier = Delay;
		Info.RateMultiplier = Rate;
		Info.TraitChance = TraitChance;
		Info.TraitStrength = Strength;
		Info.Color = Color;
		Info.Glow = Glow;
		return Info;
	}

	const TCHAR* RarityModelName(EWeaponRarity Rarity)
	{
		switch (Rarity)
		{
		case EWeaponRarity::Uncommon: return TEXT("Uncommon");
		case EWeaponRarity::Rare: return TEXT("Rare");
		case EWeaponRarity::Epic: return TEXT("Epic");
		case EWeaponRarity::Legendary: return TEXT("Legendary");
		default: return TEXT("Common");
		}
	}
}

UShieldDefinition::UShieldDefinition()
{
	DisplayName = FText::FromString(TEXT("Ward Box"));
	// The weights and colours are the guns' (UWeaponDefinition), so a ward reads in the same rarity order. Capacity climbs
	// faster than a gun's damage does (a ward is the one piece of armour the player has); the top tiers also wake sooner
	// and refill faster. Traits from Rare up (the user's contract): half the Rares, most Epics, every Legendary.
	//                                                        Weight Capacity Delay Rate  Trait  Strength
	RarityTable.Add(EWeaponRarity::Common,    MakeShieldRarity(60.f, 1.00f, 1.00f, 1.00f, 0.f,   1.00f, FLinearColor(0.8f, 0.8f, 0.8f), 1.2f));
	RarityTable.Add(EWeaponRarity::Uncommon,  MakeShieldRarity(25.f, 1.15f, 0.95f, 1.05f, 0.f,   1.00f, FLinearColor(0.2f, 0.8f, 0.2f), 1.8f));
	RarityTable.Add(EWeaponRarity::Rare,      MakeShieldRarity(10.f, 1.30f, 0.90f, 1.10f, 0.5f,  1.00f, FLinearColor(0.2f, 0.4f, 1.0f), 2.4f));
	RarityTable.Add(EWeaponRarity::Epic,      MakeShieldRarity( 4.f, 1.50f, 0.85f, 1.15f, 0.85f, 1.25f, FLinearColor(0.6f, 0.2f, 0.9f), 3.0f));
	RarityTable.Add(EWeaponRarity::Legendary, MakeShieldRarity( 1.f, 1.75f, 0.80f, 1.20f, 1.f,   1.60f, FLinearColor(1.0f, 0.6f, 0.0f), 3.6f));

	for (EWeaponRarity Rarity : { EWeaponRarity::Common, EWeaponRarity::Uncommon, EWeaponRarity::Rare, EWeaponRarity::Epic, EWeaponRarity::Legendary })
	{
		const FString Name = FString::Printf(TEXT("SM_WardBox_%s"), RarityModelName(Rarity));
		Models.Add(Rarity, TSoftObjectPtr<UStaticMesh>(FSoftObjectPath(FString::Printf(TEXT("/Game/Art/Shields/%s.%s"), *Name, *Name))));
	}
}

FPrimaryAssetId UShieldDefinition::GetPrimaryAssetId() const
{
	return FPrimaryAssetId(TEXT("ShieldDefinition"), GetFName());
}

UShieldDefinition* UShieldDefinition::GetWardBox()
{
	static TSoftObjectPtr<UShieldDefinition> Asset{ FSoftObjectPath(WardBoxPath) };
	if (UShieldDefinition* Loaded = Asset.LoadSynchronous())
	{
		return Loaded;
	}
	// Before the asset is made the class's own numbers stand in: a ward saved with it reads back as the same object.
	return GetMutableDefault<UShieldDefinition>();
}

const FShieldRarityInfo& UShieldDefinition::GetRarityInfo(EWeaponRarity Rarity) const
{
	static const FShieldRarityInfo Default;
	const FShieldRarityInfo* Found = RarityTable.Find(Rarity);
	return Found ? *Found : Default;
}

UStaticMesh* UShieldDefinition::LoadModel(EWeaponRarity Rarity) const
{
	for (const EWeaponRarity Try : { Rarity, EWeaponRarity::Common })
	{
		if (const TSoftObjectPtr<UStaticMesh>* Found = Models.Find(Try))
		{
			if (UStaticMesh* Mesh = Found->LoadSynchronous())
			{
				return Mesh;
			}
		}
	}
	return nullptr;
}
