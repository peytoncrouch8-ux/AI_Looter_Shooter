#include "Missions/MissionDefinition.h"
#include "Areas/AreaDefinition.h"
#include "Missions/MissionObjective.h"
#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Modules/ModuleManager.h"

#define LOCTEXT_NAMESPACE "Missions"

FName UMissionDefinition::GetMissionId() const
{
	if (!Id.IsNone())
	{
		return Id;
	}
	FString AssetName = GetName();
	AssetName.RemoveFromStart(AssetPrefix);
	return FName(*AssetName);
}

const UMissionObjective* UMissionDefinition::GetObjective(int32 StepIndex, int32 Index) const
{
	return Steps.IsValidIndex(StepIndex) && Steps[StepIndex].Objectives.IsValidIndex(Index) ? Steps[StepIndex].Objectives[Index].Get() : nullptr;
}

bool UMissionDefinition::IsNamed(const FString& Words) const
{
	FString Asked = Words.TrimStartAndEnd();
	Asked.RemoveFromStart(AssetPrefix);
	const FString Simple = UAreaDefinition::Simplify(Asked);
	if (Simple.IsEmpty())
	{
		return false;
	}
	FString AssetName = GetName();
	AssetName.RemoveFromStart(AssetPrefix);
	return Simple == UAreaDefinition::Simplify(GetMissionId().ToString()) || Simple == UAreaDefinition::Simplify(AssetName)
		|| Simple == UAreaDefinition::Simplify(Title.ToString());
}

TArray<UMissionDefinition*> UMissionDefinition::LoadAll()
{
	// Every mission asset wherever it lives, so a new mission is just a new data asset.
	IAssetRegistry& Registry = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
	if (Registry.IsLoadingAssets())
	{
		// The editor finds its assets in the background while it starts up: look through the missions' folder now.
		Registry.ScanPathsSynchronous({ FString(AssetFolder) });
	}
	TArray<FAssetData> Assets;
	Registry.GetAssetsByClass(StaticClass()->GetClassPathName(), Assets, true);

	TArray<UMissionDefinition*> Missions;
	for (const FAssetData& Asset : Assets)
	{
		if (UMissionDefinition* Mission = Cast<UMissionDefinition>(Asset.GetAsset()))
		{
			Missions.Add(Mission);
		}
	}
	Missions.Sort([](const UMissionDefinition& A, const UMissionDefinition& B)
	{
		if (A.Kind != B.Kind)
		{
			return A.Kind < B.Kind;
		}
		if (A.SortOrder != B.SortOrder)
		{
			return A.SortOrder < B.SortOrder;
		}
		return A.GetMissionId().LexicalLess(B.GetMissionId());
	});
	return Missions;
}

FText UMissionDefinition::KindName(EMissionKind InKind)
{
	switch (InKind)
	{
	case EMissionKind::Main:     return LOCTEXT("KindMain", "Main");
	case EMissionKind::Side:     return LOCTEXT("KindSide", "Side");
	case EMissionKind::Tutorial: return LOCTEXT("KindTutorial", "Tutorial");
	}
	return FText::GetEmpty();
}

#undef LOCTEXT_NAMESPACE
