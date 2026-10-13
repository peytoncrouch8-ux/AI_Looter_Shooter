// UMissionOfferWidget: the card's words (FMissionOfferCard): its kind, title, who offers it and where, its story, the first
// step's objectives and the rewards, and where a mission is picked up for the Missions page; a turn-in's two choices with
// what each gives and what comes either way (MakeChoice); a readable thing's words (MakeReading).

#include "UI/World/MissionOfferWidget.h"
#include "Affixes/WeaponRollLibrary.h"
#include "Economy/EconomyRules.h"
#include "Loot/LootLibrary.h"
#include "Missions/MissionDefinition.h"
#include "Missions/MissionObjective.h"
#include "Missions/MissionRewards.h"
#include "Missions/MissionRunner.h"
#include "Progression/PlayerProgressionSubsystem.h"
#include "Session/SessionSubsystem.h"
#include "UI/Inventory/MapPlaces.h"
#include "UI/Style/LooterUIStyle.h"
#include "Weapons/NamedWeaponDefinition.h"
#include "Weapons/WeaponDefinition.h"
#include "World/MissionArtifact.h"
#include "World/NoticeBoard.h"
#include "World/ReadableProp.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Math/RandomStream.h"
#include "Misc/PackageName.h"

#define LOCTEXT_NAMESPACE "MissionOffer"

namespace
{
	/** A giver this near a named place (cm, on the map) is "at" it. */
	constexpr double PlaceReach = 6000.0;

	FText RarityName(EWeaponRarity Rarity)
	{
		return UEnum::GetDisplayValueAsText(Rarity);
	}

	/**
	 * The gun a reward's rarity colour is read from: its own kind, else one the default loot table would give (the rarity
	 * colours are the same from kind to kind; any stands for them).
	 */
	const UWeaponDefinition* ColourSource(const FMissionRewards& Rewards)
	{
		if (Rewards.GunKind)
		{
			return Rewards.GunKind;
		}
		FRandomStream Fixed(1);
		return ULootLibrary::PickWeaponWith(ULootLibrary::GetDefaultLootTable(), Fixed);
	}

	FLinearColor RarityColor(const UWeaponDefinition* Kind, EWeaponRarity Rarity)
	{
		return Kind ? UWeaponRollLibrary::GetRarityColor(Kind, Rarity) : LooterUI::Color::Text();
	}

	void AddReward(TArray<FMissionOfferReward>& Rewards, const FText& Label, const FText& Value, const FLinearColor& Color)
	{
		FMissionOfferReward& Reward = Rewards.AddDefaulted_GetRef();
		Reward.Label = Label;
		Reward.Value = Value;
		Reward.Color = Color;
	}

	/** The player's level, for amounts that grow with it (1 without a player). */
	int32 PlayerLevelIn(const UWorld* World)
	{
		const APlayerController* Controller = World ? World->GetFirstPlayerController() : nullptr;
		const ULocalPlayer* LocalPlayer = Controller ? Controller->GetLocalPlayer() : nullptr;
		const UPlayerProgressionSubsystem* Progression = LocalPlayer ? LocalPlayer->GetSubsystem<UPlayerProgressionSubsystem>() : nullptr;
		return Progression ? FMath::Max(Progression->GetLevel(), 1) : 1;
	}

	/**
	 * The rewards as the card lists them: experience, the purse (the rewards' own, else the giver's kept by PurseId), the gun
	 * with its rarity, the named gun, the areas opened.
	 */
	TArray<FMissionOfferReward> DescribeRewards(const FMissionRewards& Rewards, FName PurseId)
	{
		using namespace LooterUI;
		TArray<FMissionOfferReward> Found;
		if (Rewards.Experience > 0)
		{
			AddReward(Found, LOCTEXT("RewardXP", "Experience"), FText::Format(LOCTEXT("XPValue", "+{0} XP"), FText::AsNumber(Rewards.Experience)),
				Color::XPLight());
		}
		else if (Rewards.ExperienceShare > 0.f)
		{
			AddReward(Found, LOCTEXT("RewardXP", "Experience"), FText::Format(LOCTEXT("XPShare", "+{0}% of a level"),
				FText::AsNumber(FMath::RoundToInt32(Rewards.ExperienceShare * 100.f))), Color::XPLight());
		}
		// The purse: the rewards' own, else the giver's kept by mission id (MissionRewards::PurseOf).
		if (const int32 Purse = MissionRewards::PurseOf(Rewards, PurseId); Purse > 0)
		{
			AddReward(Found, LOCTEXT("RewardSilver", "Silver"), FText::Format(LOCTEXT("SilverValue", "+{0}"),
				FText::FromString(LooterEconomy::Format(Purse))), Color::Text());
		}
		if (Rewards.bGun)
		{
			// "Pump Shotgun, Rare or better", "Uncommon or better", "A gun", in the floor's rarity colour.
			const FText Kind = Rewards.GunKind && !Rewards.GunKind->DisplayName.IsEmpty() ? Rewards.GunKind->DisplayName : FText::GetEmpty();
			FText Value;
			if (Rewards.GunRarityFloor != EWeaponRarity::Common)
			{
				Value = Kind.IsEmpty() ? FText::Format(LOCTEXT("GunFloor", "{0} or better"), RarityName(Rewards.GunRarityFloor))
					: FText::Format(LOCTEXT("KindFloor", "{0}, {1} or better"), Kind, RarityName(Rewards.GunRarityFloor));
			}
			else
			{
				Value = Kind.IsEmpty() ? LOCTEXT("AnyGun", "A gun") : Kind;
			}
			AddReward(Found, LOCTEXT("RewardGun", "Gun"), Value, RarityColor(ColourSource(Rewards), Rewards.GunRarityFloor));
		}
		if (!Rewards.NamedGun.IsNone())
		{
			// A named gun in its fixed rarity's colour: the card's brightest line.
			const UNamedWeaponDefinition* Named = UNamedWeaponDefinition::FindByName(Rewards.NamedGun.ToString());
			const FLinearColor NamedColor = Named ? RarityColor(Named->Weapon, Named->Rarity) : Color::Accent();
			AddReward(Found, LOCTEXT("RewardNamed", "Named gun"), MissionRewards::NamedGunName(Rewards), NamedColor);
		}
		for (const FName AreaId : Rewards.UnlockAreas)
		{
			AddReward(Found, LOCTEXT("RewardArea", "Opens"), MissionRewards::AreaName(AreaId), Color::Title());
		}
		return Found;
	}
}

// ---------------------------------------------------------------------------
// The giver
// ---------------------------------------------------------------------------

EMissionGiverKind FMissionOfferCard::GiverKindOf(const UMissionDefinition& Mission, const AActor* Giver)
{
	if (Giver)
	{
		if (Giver->IsA<ANoticeBoard>())
		{
			return EMissionGiverKind::Board;
		}
		if (Giver->IsA<AMissionArtifact>())
		{
			return EMissionGiverKind::Artifact;
		}
		return EMissionGiverKind::Someone;
	}
	// Not in this level: its tag says what it is.
	const FString Tag = Mission.Offer.GiverTag.ToString();
	if (Tag.StartsWith(TEXT("NoticeBoard")))
	{
		return EMissionGiverKind::Board;
	}
	return Tag.StartsWith(TEXT("Artifact_")) ? EMissionGiverKind::Artifact : EMissionGiverKind::Someone;
}

FText FMissionOfferCard::GiverNameOf(const UMissionDefinition& Mission, const AActor* Giver)
{
	if (!Mission.Offer.GiverName.IsEmpty())
	{
		return Mission.Offer.GiverName;
	}
	// A board's or an artifact's own title says it better than its tag.
	if (const ANoticeBoard* Board = Cast<ANoticeBoard>(Giver); Board && !Board->Title.IsEmpty())
	{
		return Board->Title;
	}
	if (const AMissionArtifact* Artifact = Cast<AMissionArtifact>(Giver); Artifact && !Artifact->Title.IsEmpty())
	{
		return Artifact->Title;
	}
	return Mission.GetOfferGiverName();
}

FText FMissionOfferCard::PlaceNear(const UWorld* World, const FVector& Where)
{
	if (!World)
	{
		return FText::GetEmpty();
	}
	const TConstArrayView<FMapPlace> Places = MapPlaces::For(FPackageName::GetShortName(USessionSubsystem::MapOf(World)));
	const FMapPlace* Nearest = nullptr;
	double NearestDistance = PlaceReach;
	for (const FMapPlace& Place : Places)
	{
		const double Distance = FVector2D::Distance(Place.At, FVector2D(Where.X, Where.Y));
		if (Distance < NearestDistance)
		{
			Nearest = &Place;
			NearestDistance = Distance;
		}
	}
	return Nearest ? FText::FromString(Nearest->Name) : FText::GetEmpty();
}

FText FMissionOfferCard::WhereToGet(const UMissionDefinition& Mission, const AActor* Giver, const UWorld* World, const FText& ElsewhereArea)
{
	const FText Name = GiverNameOf(Mission, Giver);
	const FText Place = ElsewhereArea.IsEmpty() && Giver ? PlaceNear(World, Giver->GetActorLocation()) : FText::GetEmpty();
	switch (GiverKindOf(Mission, Giver))
	{
	case EMissionGiverKind::Board:
		if (!ElsewhereArea.IsEmpty())
		{
			return FText::Format(LOCTEXT("BoardElsewhere", "Posted on {0}, in {1}"), Name, ElsewhereArea);
		}
		return Place.IsEmpty() ? FText::Format(LOCTEXT("Board", "Posted on {0}"), Name)
			: FText::Format(LOCTEXT("BoardAt", "Posted on {0}, at {1}"), Name, Place);
	case EMissionGiverKind::Artifact:
		if (!ElsewhereArea.IsEmpty())
		{
			return FText::Format(LOCTEXT("ArtifactElsewhere", "{0}, somewhere in {1}"), Name, ElsewhereArea);
		}
		return Place.IsEmpty() ? FText::Format(LOCTEXT("Artifact", "{0}, waiting to be found"), Name)
			: FText::Format(LOCTEXT("ArtifactAt", "{0}, at {1}"), Name, Place);
	case EMissionGiverKind::Someone:
		break;
	}
	if (!ElsewhereArea.IsEmpty())
	{
		return FText::Format(LOCTEXT("SomeoneElsewhere", "Offered by {0}, in {1}"), Name, ElsewhereArea);
	}
	return Place.IsEmpty() ? FText::Format(LOCTEXT("Someone", "Offered by {0}"), Name)
		: FText::Format(LOCTEXT("SomeoneAt", "Offered by {0}, at {1}"), Name, Place);
}

// ---------------------------------------------------------------------------
// The card
// ---------------------------------------------------------------------------

FMissionOfferCard FMissionOfferCard::Make(const UMissionDefinition& Mission, const AActor* Giver, const UWorld* World)
{
	FMissionOfferCard Card;
	Card.bMain = Mission.Kind == EMissionKind::Main;
	Card.Kind = Card.bMain ? LOCTEXT("KindMain", "Main mission")
		: Mission.Kind == EMissionKind::Side ? LOCTEXT("KindSide", "Side mission") : LOCTEXT("KindPractice", "Practice");
	Card.Title = Mission.Title.IsEmpty() ? FText::FromName(Mission.GetMissionId()) : Mission.Title;

	// Who offers it, and where: "From Delia · Ransom Farm", "Posted on the notice board", "Found: a miner's tag".
	Card.GiverKind = GiverKindOf(Mission, Giver);
	const FText Name = GiverNameOf(Mission, Giver);
	const FText Place = Giver ? PlaceNear(World, Giver->GetActorLocation()) : FText::GetEmpty();
	FText From;
	switch (Card.GiverKind)
	{
	case EMissionGiverKind::Board:    From = FText::Format(LOCTEXT("FromBoard", "Posted on {0}"), Name); break;
	case EMissionGiverKind::Artifact: From = FText::Format(LOCTEXT("FromArtifact", "Found: {0}"), Name); break;
	case EMissionGiverKind::Someone:  From = FText::Format(LOCTEXT("FromSomeone", "From {0}"), Name); break;
	}
	if (Name.IsEmpty())
	{
		From = FText::GetEmpty();
	}
	else if (!Place.IsEmpty())
	{
		From = FText::Format(LOCTEXT("FromAt", "{0} · {1}"), From, Place);
	}
	Card.From = From;

	// The story: a found thing's own words when it has them, else the offer's briefing, else what the mission is about.
	const AMissionArtifact* Artifact = Cast<AMissionArtifact>(Giver);
	Card.bDocument = Card.GiverKind == EMissionGiverKind::Artifact;
	Card.Briefing = Artifact && !Artifact->Text.IsEmpty() ? Artifact->Text
		: !Mission.Offer.Briefing.IsEmpty() ? Mission.Offer.Briefing : Mission.Summary;

	// The first step only: what comes after is the job's to reveal. An optional objective says so.
	if (Mission.Steps.IsValidIndex(0))
	{
		for (const TObjectPtr<UMissionObjective>& Objective : Mission.Steps[0].Objectives)
		{
			if (Objective)
			{
				const FString Words = Objective->GetDisplayText(World);
				Card.Objectives.Add(Objective->bOptional ? FText::Format(LOCTEXT("OptionalObjective", "Optional: {0}"), FText::FromString(Words)).ToString()
					: Words);
			}
		}
	}
	Card.MoreSteps = FMath::Max(Mission.Steps.Num() - 1, 0);
	Card.Rewards = DescribeRewards(Mission.Rewards, Mission.GetMissionId());
	// Borderlands' way, the job says what going further pays: each optional objective's silver, and a choice at the end.
	if (Mission.CountOptional() > 0 && Mission.Rewards.OptionalSilver > 0)
	{
		AddReward(Card.Rewards, LOCTEXT("RewardOptional", "Optional"), FText::Format(LOCTEXT("OptionalValue", "+{0} silver each"),
			FText::FromString(LooterEconomy::Format(LooterEconomy::AtLevel(Mission.Rewards.OptionalSilver, PlayerLevelIn(World))))),
			LooterUI::Color::TextDim());
	}
	if (Mission.HasTurnInChoice())
	{
		AddReward(Card.Rewards, LOCTEXT("RewardChoice", "Your call"), LOCTEXT("ChoiceValue", "One of two rewards, chosen at the end"),
			LooterUI::Color::Title());
	}
	Card.Accept = LOCTEXT("Accept", "Accept");
	Card.Decline = LOCTEXT("Decline", "Not now");
	return Card;
}

FMissionOfferCard FMissionOfferCard::MakeChoice(const UMissionDefinition& Mission, const AActor* Giver, const UWorld* World)
{
	FMissionOfferCard Card;
	Card.bMain = Mission.Kind == EMissionKind::Main;
	Card.Kind = LOCTEXT("KindChoice", "Mission complete · Your call");
	Card.Title = Mission.Title.IsEmpty() ? FText::FromName(Mission.GetMissionId()) : Mission.Title;
	Card.GiverKind = GiverKindOf(Mission, Giver);
	const FText Place = Giver ? PlaceNear(World, Giver->GetActorLocation()) : FText::GetEmpty();
	Card.From = FText::Format(LOCTEXT("TurnInTo", "Turn in to {0}"), Mission.GetGiverName());
	if (!Place.IsEmpty())
	{
		Card.From = FText::Format(LOCTEXT("FromAt", "{0} · {1}"), Card.From, Place);
	}
	Card.Question = !Mission.TurnIn.ChoiceQuestion.IsEmpty() ? Mission.TurnIn.ChoiceQuestion
		: LOCTEXT("DefaultQuestion", "Two ways to settle it. Choose one: it can't be undone.");

	// What comes whichever is chosen: the mission's experience (unless a choice names its own), and the optional objectives'
	// bonus already earned.
	const FMissionTurnInChoice* First = Mission.GetTurnInChoice(0);
	const FMissionTurnInChoice* Second = Mission.GetTurnInChoice(1);
	const bool bOwnExperience = (First && First->Rewards.GivesExperience()) || (Second && Second->Rewards.GivesExperience());
	if (!bOwnExperience)
	{
		FMissionRewards Experience;
		Experience.Experience = Mission.Rewards.Experience;
		Experience.ExperienceShare = Mission.Rewards.ExperienceShare;
		Card.EitherWay = DescribeRewards(Experience, NAME_None);
	}
	const UMissionRunner* Runner = World ? World->GetSubsystem<UMissionRunner>() : nullptr;
	const int32 Done = Runner ? Runner->CountOptionalDone(Mission.GetMissionId()) : 0;
	if (const int32 Bonus = MissionRewards::OptionalBonus(Mission.Rewards, Done, PlayerLevelIn(World)); Bonus > 0)
	{
		AddReward(Card.EitherWay, FText::Format(LOCTEXT("OptionalDone", "Optional ({0} done)"), Done),
			FText::Format(LOCTEXT("BonusValue", "+{0} silver"), FText::FromString(LooterEconomy::Format(Bonus))), LooterUI::Color::Text());
	}

	for (int32 Index = 0; Index < 2; ++Index)
	{
		const FMissionTurnInChoice* Choice = Mission.GetTurnInChoice(Index);
		if (!Choice)
		{
			continue;
		}
		FMissionChoiceCard& Shown = Card.Choices.AddDefaulted_GetRef();
		Shown.Title = Choice->Title;
		Shown.Line = Choice->Line;
		// What it gives, never the giver's purse by id: a choice's silver is its own.
		Shown.Rewards = DescribeRewards(Choice->Rewards, NAME_None);
		if (Shown.Rewards.IsEmpty())
		{
			AddReward(Shown.Rewards, LOCTEXT("RewardNone", "Reward"), LOCTEXT("NoneValue", "Nothing but the choice"), LooterUI::Color::TextDim());
		}
		Shown.Stamp = Choice->Stamp.IsEmpty() ? LOCTEXT("Settled", "SETTLED") : Choice->Stamp;
	}
	Card.Accept = LOCTEXT("Choose", "Choose");
	Card.Decline = LOCTEXT("DecideLater", "Decide later");
	return Card;
}

FMissionOfferCard FMissionOfferCard::MakeReading(const AActor* Thing, const UWorld* World)
{
	FMissionOfferCard Card;
	const AReadableProp* Readable = Cast<AReadableProp>(Thing);
	Card.Kind = Readable && !Readable->Heading.IsEmpty() ? Readable->Heading : LOCTEXT("KindRead", "Written here");
	Card.Title = Readable ? Readable->Title : FText::GetEmpty();
	Card.From = Thing ? PlaceNear(World, Thing->GetActorLocation()) : FText::GetEmpty();
	Card.Briefing = Readable ? Readable->Text : FText::GetEmpty();
	Card.bDocument = true;
	Card.Decline = LOCTEXT("Close", "Close");
	return Card;
}

#undef LOCTEXT_NAMESPACE
