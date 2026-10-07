#include "Weapons/NamedWeaponDefinition.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Areas/AreaDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "Weapons/WeaponParts.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"

FName UNamedWeaponDefinition::GetNamedId() const
{
	FString Id = GetName();
	Id.RemoveFromStart(AssetPrefix);
	return FName(*Id);
}

TArray<FName> UNamedWeaponDefinition::GetPartKeys() const
{
	TArray<FName> Keys;
	if (!Weapon)
	{
		return Keys;
	}
	for (const FWeaponPartSlot& Slot : Weapon->Parts)
	{
		const FName* Key = Parts.Find(Slot.Name);
		Keys.Add(Key ? *Key : NAME_None);
	}
	return Keys;
}

FWeaponInstanceData UNamedWeaponDefinition::MakeInstance(int32 Level)
{
	FWeaponInstanceData Instance;
	Instance.Definition = Weapon;
	Instance.Named = this;
	Instance.Rarity = Rarity;
	Instance.Level = FMath::Max(Level, 1);
	Instance.Seed = Seed;
	Instance.Parts = GetPartKeys();
	Instance.Stats = UWeaponRollLibrary::ComputeInstanceStats(Instance);
	return Instance;
}

TArray<FString> UNamedWeaponDefinition::FindProblems() const
{
	TArray<FString> Problems;
	if (!Weapon)
	{
		Problems.Add(TEXT("it names no kind of gun (Weapon)"));
		return Problems;
	}
	if (DisplayName.IsEmpty())
	{
		Problems.Add(TEXT("it has no name"));
	}
	const FString Kind = Weapon->GetName();
	for (const TPair<FName, FName>& Named : Parts)
	{
		if (!Weapon->Parts.ContainsByPredicate([&Named](const FWeaponPartSlot& Slot) { return Slot.Name == Named.Key; }))
		{
			Problems.Add(FString::Printf(TEXT("%s has no %s slot"), *Kind, *Named.Key.ToString()));
		}
	}

	// Each slot's part: named, one of the slot's own, and allowed at its rarity (rarity unlocks the better parts).
	const UEnum* Rarities = StaticEnum<EWeaponRarity>();
	TArray<const FWeaponPartOption*> Chosen;
	for (const FWeaponPartSlot& Slot : Weapon->Parts)
	{
		const FName* Key = Parts.Find(Slot.Name);
		const FWeaponPartOption* Option = Key ? Slot.Options.FindByPredicate([Key](const FWeaponPartOption& Each) { return Each.Key == *Key; }) : nullptr;
		Chosen.Add(Option);
		if (!Key)
		{
			Problems.Add(FString::Printf(TEXT("no part for the %s slot"), *Slot.Name.ToString()));
		}
		else if (!Option)
		{
			Problems.Add(FString::Printf(TEXT("%s's %s slot has no part %s"), *Kind, *Slot.Name.ToString(), *Key->ToString()));
		}
		else if (Option->MinRarity > Rarity)
		{
			Problems.Add(FString::Printf(TEXT("the %s %s comes on %s guns and up, and this one is %s"), *Slot.Name.ToString(), *Key->ToString(),
				*Rarities->GetDisplayNameTextByValue(static_cast<int64>(Option->MinRarity)).ToString(),
				*Rarities->GetDisplayNameTextByValue(static_cast<int64>(Rarity)).ToString()));
		}
		else if (!Option->Mesh)
		{
			// A part without its model is never picked: the gun would get another in its place.
			Problems.Add(FString::Printf(TEXT("the %s %s has no model (import the gun's parts)"), *Slot.Name.ToString(), *Key->ToString()));
		}
	}

	// A part that needs a long enough part in an earlier slot (the 8-shell tube clamps to a barrel of 46 cm or more).
	for (int32 Index = 0; Index < Chosen.Num(); ++Index)
	{
		const FWeaponPartOption* Option = Chosen[Index];
		if (!Option || Option->Requires.Slot.IsNone())
		{
			continue;
		}
		const FName NeededSlot = Option->Requires.Slot;
		const int32 Needed = Weapon->Parts.IndexOfByPredicate([NeededSlot](const FWeaponPartSlot& Slot) { return Slot.Name == NeededSlot; });
		if (Needed == INDEX_NONE || Needed >= Index || !Chosen[Needed] || Chosen[Needed]->Length < Option->Requires.MinLength)
		{
			Problems.Add(FString::Printf(TEXT("the %s %s needs a %s of at least %.0f cm"), *Weapon->Parts[Index].Name.ToString(),
				*Option->Key.ToString(), *NeededSlot.ToString().ToLower(), Option->Requires.MinLength));
		}
	}
	return Problems;
}

bool UNamedWeaponDefinition::IsNamed(const FString& Words) const
{
	FString Asked = Words.TrimStartAndEnd();
	Asked.RemoveFromStart(AssetPrefix);
	const FString Simple = UAreaDefinition::Simplify(Asked);
	return !Simple.IsEmpty() && (Simple == UAreaDefinition::Simplify(GetNamedId().ToString())
		|| Simple == UAreaDefinition::Simplify(DisplayName.ToString()));
}

TArray<UNamedWeaponDefinition*> UNamedWeaponDefinition::LoadAll()
{
	// Every named gun wherever it lives, so a new one is just a new data asset.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	if (Registry.IsLoadingAssets())
	{
		// The editor finds its assets in the background while it starts up: look through the named guns' folder now.
		Registry.ScanPathsSynchronous({ FString(AssetFolder) });
	}
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(StaticClass()->GetClassPathName(), Assets, true);

	TArray<UNamedWeaponDefinition*> Named;
	for (const FAssetData& Asset : Assets)
	{
		if (UNamedWeaponDefinition* Gun = Cast<UNamedWeaponDefinition>(Asset.GetAsset()))
		{
			Named.Add(Gun);
		}
	}
	Named.Sort([](const UNamedWeaponDefinition& A, const UNamedWeaponDefinition& B)
	{
		return A.GetNamedId().ToString() < B.GetNamedId().ToString();
	});
	return Named;
}

UNamedWeaponDefinition* UNamedWeaponDefinition::FindByName(const FString& Words)
{
	for (UNamedWeaponDefinition* Gun : LoadAll())
	{
		if (Gun->IsNamed(Words))
		{
			return Gun;
		}
	}
	return nullptr;
}
