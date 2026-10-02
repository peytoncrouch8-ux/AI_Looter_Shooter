#include "Areas/AreaDefinition.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/Char.h"
#include "Misc/PackageName.h"
#include "Modules/ModuleManager.h"

FName UAreaDefinition::GetAreaId() const
{
	FString Id = GetName();
	Id.RemoveFromStart(AssetPrefix);
	return FName(*Id);
}

FString UAreaDefinition::GetMapPackage() const
{
	return Map.IsNull() ? FString() : Map.GetLongPackageName();
}

bool UAreaDefinition::HasMap() const
{
	const FString MapPackage = GetMapPackage();
	return !MapPackage.IsEmpty() && FPackageName::IsValidLongPackageName(MapPackage) && FPackageName::DoesPackageExist(MapPackage);
}

FName UAreaDefinition::GetDefaultLanding() const
{
	return Landings.Num() > 0 ? Landings[0] : NAME_None;
}

bool UAreaDefinition::IsNamed(const FString& Words) const
{
	FString Asked = Words.TrimStartAndEnd();
	Asked.RemoveFromStart(AssetPrefix);
	const FString Simple = Simplify(Asked);
	return !Simple.IsEmpty() && (Simple == Simplify(GetAreaId().ToString()) || Simple == Simplify(DisplayName.ToString()));
}

FString UAreaDefinition::Simplify(const FString& Words)
{
	FString Simple;
	Simple.Reserve(Words.Len());
	for (int32 Index = 0; Index < Words.Len(); ++Index)
	{
		const TCHAR Letter = Words[Index];
		if (FChar::IsAlnum(Letter))
		{
			Simple.AppendChar(FChar::ToLower(Letter));
		}
	}
	return Simple;
}

TArray<UAreaDefinition*> UAreaDefinition::LoadAll()
{
	// Every area asset wherever it lives, so a new area is just a new data asset.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	if (Registry.IsLoadingAssets())
	{
		// The editor finds its assets in the background while it starts up: look through the areas' folder now.
		Registry.ScanPathsSynchronous({ FString(AssetFolder) });
	}
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(StaticClass()->GetClassPathName(), Assets, true);

	TArray<UAreaDefinition*> Areas;
	for (const FAssetData& Asset : Assets)
	{
		if (UAreaDefinition* Area = Cast<UAreaDefinition>(Asset.GetAsset()))
		{
			Areas.Add(Area);
		}
	}
	Areas.Sort([](const UAreaDefinition& A, const UAreaDefinition& B)
	{
		if (A.SortOrder != B.SortOrder)
		{
			return A.SortOrder < B.SortOrder;
		}
		return A.GetAreaId().ToString() < B.GetAreaId().ToString();
	});
	return Areas;
}

UAreaDefinition* UAreaDefinition::FindByName(const FString& Words)
{
	for (UAreaDefinition* Area : LoadAll())
	{
		if (Area->IsNamed(Words))
		{
			return Area;
		}
	}
	return nullptr;
}

UAreaDefinition* UAreaDefinition::FindByMap(const FString& MapPackage)
{
	if (MapPackage.IsEmpty())
	{
		return nullptr;
	}
	for (UAreaDefinition* Area : LoadAll())
	{
		if (Area->GetMapPackage().Equals(MapPackage, ESearchCase::IgnoreCase))
		{
			return Area;
		}
	}
	return nullptr;
}
