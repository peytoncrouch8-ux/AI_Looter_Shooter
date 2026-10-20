#include "Missions/MissionDefinition.h"
#include "Areas/AreaDefinition.h"
#include "Missions/MissionObjective.h"
#include "Story/HeroLines.h"
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

FText UMissionDefinition::GetGiverName() const
{
	if (!TurnIn.GiverName.IsEmpty())
	{
		// "Grandma Delia" and "Pa" are Ellis's words for them (Story/HeroLines).
		return HeroLines::Resolve(TurnIn.GiverName);
	}
	if (TurnIn.SpeakerTag.IsNone())
	{
		return FText::GetEmpty();
	}
	// Speakers are tagged Speaker_<Name>, as the talk objective reads them ("Talk to Delia").
	FString Name = TurnIn.SpeakerTag.ToString();
	Name.RemoveFromStart(TEXT("Speaker_"));
	return FText::FromString(FName::NameToDisplayString(Name, /*bIsBool*/ false));
}

FString UMissionDefinition::GetTurnInShortText() const
{
	return FText::Format(LOCTEXT("TurnInShort", "Turn in to {0}"), GetGiverName()).ToString();
}

FString UMissionDefinition::GetTurnInText() const
{
	return FText::Format(LOCTEXT("TurnInFull", "Ready to turn in: talk to {0}"), GetGiverName()).ToString();
}

FText UMissionDefinition::GetOfferGiverName() const
{
	if (!Offer.GiverName.IsEmpty())
	{
		return HeroLines::Resolve(Offer.GiverName);
	}
	if (Offer.GiverTag.IsNone())
	{
		return FText::GetEmpty();
	}
	// Givers are tagged by their kind and name (Speaker_Delia, NoticeBoard_Rangers, Artifact_MinersTag): people read the name.
	FString Name = Offer.GiverTag.ToString();
	for (const TCHAR* Prefix : { TEXT("Speaker_"), TEXT("NoticeBoard_"), TEXT("Artifact_") })
	{
		if (Name.RemoveFromStart(Prefix))
		{
			break;
		}
	}
	return FText::FromString(FName::NameToDisplayString(Name, /*bIsBool*/ false));
}

const FMissionTurnInChoice* UMissionDefinition::GetTurnInChoice(int32 Choice) const
{
	// Only the first two count: the card has room for two, and a choice of three would be a different design.
	return HasTurnInChoice() && Choice >= 0 && Choice < 2 ? &TurnIn.Choices[Choice] : nullptr;
}

FMissionRewards UMissionDefinition::RewardsFor(int32 Choice) const
{
	const FMissionTurnInChoice* Chosen = GetTurnInChoice(Choice);
	if (!Chosen)
	{
		return Rewards;
	}
	FMissionRewards Given = Chosen->Rewards;
	// The mission's own experience either way, unless the choice names its own: the card says it above the two.
	if (!Given.GivesExperience())
	{
		Given.Experience = Rewards.Experience;
		Given.ExperienceShare = Rewards.ExperienceShare;
	}
	// The optional objectives were done before the choice: their bonus is the mission's.
	Given.OptionalSilver = Rewards.OptionalSilver;
	return Given;
}

const FMissionItemInfo* UMissionDefinition::FindItem(FName ItemId) const
{
	return ItemId.IsNone() ? nullptr : Items.FindByPredicate([ItemId](const FMissionItemInfo& Item) { return Item.Id == ItemId; });
}

int32 UMissionDefinition::CountOptional() const
{
	int32 Count = 0;
	for (const FMissionStep& Step : Steps)
	{
		for (const TObjectPtr<UMissionObjective>& Objective : Step.Objectives)
		{
			Count += Objective && Objective->bOptional ? 1 : 0;
		}
	}
	return Count;
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
