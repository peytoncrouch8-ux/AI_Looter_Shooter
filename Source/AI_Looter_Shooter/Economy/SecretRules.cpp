// SecretRules: Sexton's secrets as plain rules: the sold flag, the prices, a secret's state in a story, the words.

#include "Economy/SecretRules.h"
#include "Economy/EconomyRules.h"
#include "Session/CampaignRecord.h"
#include "World/FindRules.h"
#include "Engine/World.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "LooterSecrets"

namespace SecretRules
{
	FName SoldFlag(FName Id)
	{
		return Id.IsNone() ? NAME_None : FName(*(TEXT("SecretSold.") + Id.ToString()));
	}

	int32 BasePrice(ESecretKind Kind)
	{
		// MAINS_2_7.md section 12: footlockers and tins 40, strongboxes and named guns' finds 100, the Tollman 150.
		switch (Kind)
		{
		case ESecretKind::Cache:     return 40;
		case ESecretKind::Strongbox: return 100;
		case ESecretKind::Named:     return 100;
		case ESecretKind::Elite:     return 150;
		case ESecretKind::Lore:      return 60;
		case ESecretKind::Story:     return 60;
		}
		return 40;
	}

	int32 PriceAt(int32 Base, int32 PlayerLevel, float Discount)
	{
		if (Base <= 0)
		{
			return 0;
		}
		// Grown as everything the vendors sell is, so a secret costs about the same share of a purse at any level.
		const int32 Grown = LooterEconomy::AtLevel(Base, FMath::Max(PlayerLevel, 1));
		const float Off = FMath::Clamp(Discount, 0.f, 1.f);
		return FMath::Max(1, FMath::RoundToInt32(static_cast<float>(Grown) * (1.f - Off)));
	}

	int32 PriceFor(const FSecretOffer& Offer, int32 PlayerLevel, float Discount)
	{
		return PriceAt(Offer.Price > 0 ? Offer.Price : BasePrice(Offer.Kind), PlayerLevel, Discount);
	}

	bool IsFound(const FSecretOffer& Offer, const FCampaignRecord& Record)
	{
		if (Offer.FindIds.IsEmpty())
		{
			return false;
		}
		for (const FName FindId : Offer.FindIds)
		{
			if (!Record.HasFlag(FindRules::FlagFor(FindId)))
			{
				return false;
			}
		}
		return true;
	}

	ESecretState StateOf(const FSecretOffer& Offer, const FCampaignRecord& Record, const UMissionRunner* Runner)
	{
		// Found first: a secret found without buying it is no secret any more, bought or not.
		if (IsFound(Offer, Record) || (!Offer.DoneWhen.IsEmpty() && Offer.DoneWhen.IsMet(Record, Runner)))
		{
			return ESecretState::Done;
		}
		if (Record.HasFlag(SoldFlag(Offer.Id)))
		{
			return ESecretState::Sold;
		}
		return Offer.AvailableWhen.IsEmpty() || Offer.AvailableWhen.IsMet(Record, Runner) ? ESecretState::ForSale : ESecretState::Hidden;
	}

	bool IsListed(ESecretState State)
	{
		return State == ESecretState::ForSale || State == ESecretState::Sold;
	}

	FString CatalogPathFor(const FString& LevelName)
	{
		// The map's short name, with a play-in-editor prefix taken off and "Lvl_" dropped: DA_Secrets_RansomsRest.
		FString Name = FPackageName::GetShortName(LevelName);
		Name = UWorld::RemovePIEPrefix(Name);
		Name.RemoveFromStart(TEXT("Lvl_"));
		const FString Asset = TEXT("DA_Secrets_") + Name;
		return FString::Printf(TEXT("/Game/Data/Secrets/%s.%s"), *Asset, *Asset);
	}

	FText KindWord(ESecretKind Kind)
	{
		switch (Kind)
		{
		case ESecretKind::Cache:     return LOCTEXT("KindCache", "Cache");
		case ESecretKind::Strongbox: return LOCTEXT("KindStrongbox", "Strongbox");
		case ESecretKind::Named:     return LOCTEXT("KindNamed", "Named gun");
		case ESecretKind::Elite:     return LOCTEXT("KindElite", "Elite");
		case ESecretKind::Lore:      return LOCTEXT("KindLore", "Lore");
		case ESecretKind::Story:     return LOCTEXT("KindStory", "Story");
		}
		return FText::GetEmpty();
	}

	FText MarkedWords(const FText& Title, int32 Places)
	{
		return Places > 1
			? FText::Format(LOCTEXT("MarkedMany", "{0} places marked on your map: {1}."), FText::AsNumber(Places), Title)
			: FText::Format(LOCTEXT("MarkedOne", "Marked on your map: {0}."), Title);
	}

	FText ShortWords(int32 Price, int32 Carried)
	{
		return FText::Format(LOCTEXT("Short", "Not enough silver: {0} needed, you carry {1}."), FText::AsNumber(Price), FText::AsNumber(Carried));
	}
}

#undef LOCTEXT_NAMESPACE
