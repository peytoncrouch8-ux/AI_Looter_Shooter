#include "Areas/AreaDefinition.h"
#include "Creatures/CreatureBase.h"
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

bool FAreaLevelZone::Contains(const FVector2D& Point) const
{
	// Even-odd: a line from the point crosses the outline an odd number of times when it's inside.
	bool bInside = false;
	for (int32 Index = 0, Previous = Outline.Num() - 1; Index < Outline.Num(); Previous = Index++)
	{
		const FVector2D& A = Outline[Index];
		const FVector2D& B = Outline[Previous];
		if ((A.Y > Point.Y) != (B.Y > Point.Y) && Point.X < (B.X - A.X) * (Point.Y - A.Y) / (B.Y - A.Y) + A.X)
		{
			bInside = !bInside;
		}
	}
	return Outline.Num() >= 3 && bInside;
}

int32 UAreaDefinition::KeepInBand(int32 Level) const
{
	return HasLevelBand() ? FMath::Clamp(Level, MinLevel, GetBandTop()) : FMath::Max(Level, 1);
}

const FAreaLevelZone* UAreaDefinition::FindLevelZone(const FVector& Spot) const
{
	const FVector2D Point(Spot.X, Spot.Y);
	return LevelZones.FindByPredicate([&Point](const FAreaLevelZone& Zone) { return Zone.Contains(Point); });
}

void UAreaDefinition::GetLevelsAt(const FVector& Spot, int32& OutLowest, int32& OutHighest) const
{
	if (const FAreaLevelZone* Zone = FindLevelZone(Spot))
	{
		OutLowest = Zone->MinLevel;
		OutHighest = Zone->GetTop();
	}
	else
	{
		// Somewhere no zone covers (an area whose zones aren't drawn yet): the band's floor, or one above.
		OutLowest = MinLevel;
		OutHighest = MinLevel + 1;
	}
	OutLowest = KeepInBand(OutLowest);
	OutHighest = FMath::Max(KeepInBand(OutHighest), OutLowest);
}

int32 UAreaDefinition::FindKindLevel(const UClass* Kind) const
{
	if (!Kind)
	{
		return 0;
	}
	const FAreaKindLevel* Found = KindLevels.FindByPredicate([Kind](const FAreaKindLevel& Each)
	{
		return Each.Kind.Get() && Each.Level > 0 && Kind->IsChildOf(Each.Kind.Get());
	});
	return Found ? KeepInBand(Found->Level) : 0;
}

int32 UAreaDefinition::PickLevel(const FVector& Spot, const UClass* Kind, float Roll) const
{
	if (const int32 Set = FindKindLevel(Kind))
	{
		return Set;
	}
	int32 Lowest = 1;
	int32 Highest = 1;
	GetLevelsAt(Spot, Lowest, Highest);
	const int32 Choices = Highest - Lowest + 1;
	return Lowest + FMath::Clamp(FMath::FloorToInt32(FMath::Clamp(Roll, 0.f, 1.f) * Choices), 0, Choices - 1);
}

int32 UAreaDefinition::LevelFor(int32 PlayerLevel, int32 Spread, int32 OwnLevel) const
{
	if (!HasLevelBand())
	{
		return FMath::Max(OwnLevel, 1);
	}
	return FMath::Clamp(PlayerLevel + Spread, MinLevel, GetBandTop());
}

bool UAreaDefinition::HasPromotions() const
{
	return RarePromotionChance > 0.f || EpicPromotionChance > 0.f;
}

ECreatureRank UAreaDefinition::PickPromotion(float Roll) const
{
	// The rarer rank first, so chances that add up past 1 still give it its share.
	const float EpicChance = FMath::Clamp(EpicPromotionChance, 0.f, 1.f);
	const float RareChance = FMath::Clamp(RarePromotionChance, 0.f, 1.f);
	if (Roll < EpicChance)
	{
		return ECreatureRank::Epic;
	}
	if (Roll < EpicChance + RareChance)
	{
		return ECreatureRank::Rare;
	}
	return ECreatureRank::Basic;
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

int32 UAreaDefinition::FindKindLevelInAnyArea(const UClass* Kind)
{
	for (const UAreaDefinition* Area : LoadAll())
	{
		if (const int32 Set = Area->FindKindLevel(Kind))
		{
			return Set;
		}
	}
	return 0;
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
