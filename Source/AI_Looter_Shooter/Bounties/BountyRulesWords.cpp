// BountyRules' words: the tiers' and twists' words, the marks' names and flavour lines by kind, Hob's lines at the fight's
// beats, and the board's cards. Every name is the game's own (none of the story's people, nor the Seven).

#include "Bounties/BountyRules.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureTraitRules.h"
#include "Economy/EconomyRules.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

namespace
{
	/** A kind's names (8 each): the mark's tag and its card. */
	TConstArrayView<FText> NamesOf(FName Kind)
	{
		static const TArray<FText> Unpaid = {
			LOCTEXT("NameUnpaid0", "Hatless Gideon"), LOCTEXT("NameUnpaid1", "Widow Marchbanks"), LOCTEXT("NameUnpaid2", "Clem the Weeper"),
			LOCTEXT("NameUnpaid3", "Parson Hale"), LOCTEXT("NameUnpaid4", "Old Man Corrigan"), LOCTEXT("NameUnpaid5", "Silas Dunmore"),
			LOCTEXT("NameUnpaid6", "Bitter Annie Vail"), LOCTEXT("NameUnpaid7", "the Tallow Bride") };
		static const TArray<FText> Spider = {
			LOCTEXT("NameSpider0", "Old Stilts"), LOCTEXT("NameSpider1", "Velvet Nan"), LOCTEXT("NameSpider2", "Ninefoot"),
			LOCTEXT("NameSpider3", "the Larder Queen"), LOCTEXT("NameSpider4", "Blackcomb"), LOCTEXT("NameSpider5", "Tick-Tock Tess"),
			LOCTEXT("NameSpider6", "Grandmother Hush"), LOCTEXT("NameSpider7", "Sweetwater Sal") };
		static const TArray<FText> Spitter = {
			LOCTEXT("NameSpitter0", "Vinegar Jim"), LOCTEXT("NameSpitter1", "Spit-Tooth Polly"), LOCTEXT("NameSpitter2", "the Sour Widow"),
			LOCTEXT("NameSpitter3", "Bitterjaw"), LOCTEXT("NameSpitter4", "Old Gall"), LOCTEXT("NameSpitter5", "Green Mag"),
			LOCTEXT("NameSpitter6", "Sallow Kate"), LOCTEXT("NameSpitter7", "the Pickler") };
		static const TArray<FText> Miner = {
			LOCTEXT("NameMiner0", "Shift-Boss Harrow"), LOCTEXT("NameMiner1", "Lamp-Eye Morrow"), LOCTEXT("NameMiner2", "Tallyman Croft"),
			LOCTEXT("NameMiner3", "Deep Davy Penhallow"), LOCTEXT("NameMiner4", "Sump Jack"), LOCTEXT("NameMiner5", "Fuse-Eye Quill"),
			LOCTEXT("NameMiner6", "Ten-Hour Tom"), LOCTEXT("NameMiner7", "Old Cribbage") };
		if (Kind == TEXT("Unpaid"))
		{
			return Unpaid;
		}
		if (Kind == TEXT("Spider"))
		{
			return Spider;
		}
		if (Kind == TEXT("Spitter"))
		{
			return Spitter;
		}
		if (Kind == TEXT("DeadMiner"))
		{
			return Miner;
		}
		return {};
	}

	/** A kind's flavour lines (4 each), picked by the posting's serial. */
	TConstArrayView<FText> FlavourOf(FName Kind)
	{
		static const TArray<FText> Unpaid = {
			LOCTEXT("FlavourUnpaid0", "Walked off the boards before anyone sat up with it. Brought friends."),
			LOCTEXT("FlavourUnpaid1", "Knocks on doors after dark and won't say who it's looking for."),
			LOCTEXT("FlavourUnpaid2", "The Rangers put it down once. It didn't take."),
			LOCTEXT("FlavourUnpaid3", "Its coal burns colder than the rest. So does its company.") };
		static const TArray<FText> Spider = {
			LOCTEXT("FlavourSpider0", "Took a mule and a dog off the Rest this month. Getting bigger on it."),
			LOCTEXT("FlavourSpider1", "Webs the trail at night and waits. Patient as sin."),
			LOCTEXT("FlavourSpider2", "Three Rangers went in after it. Their hats came out."),
			LOCTEXT("FlavourSpider3", "Fed on the dark longer than most. Hits like it.") };
		static const TArray<FText> Spitter = {
			LOCTEXT("FlavourSpitter0", "Spits from the high ground and all but laughs about it."),
			LOCTEXT("FlavourSpitter1", "Burned the hide off a drover's horse at forty paces."),
			LOCTEXT("FlavourSpitter2", "Keeps its brood close and its venom closer."),
			LOCTEXT("FlavourSpitter3", "Loose in the rocks. Bring cover, not courage.") };
		static const TArray<FText> Miner = {
			LOCTEXT("FlavourMiner0", "Still working its last shift. Anyone in the way is the rock."),
			LOCTEXT("FlavourMiner1", "Swings a pick like the Company still pays by the ton."),
			LOCTEXT("FlavourMiner2", "Its lamp went out in the Providence. It's been looking for a light since."),
			LOCTEXT("FlavourMiner3", "Leads the dead crews out at the whistle. Nobody's blown it in years.") };
		if (Kind == TEXT("Unpaid"))
		{
			return Unpaid;
		}
		if (Kind == TEXT("Spider"))
		{
			return Spider;
		}
		if (Kind == TEXT("Spitter"))
		{
			return Spitter;
		}
		if (Kind == TEXT("DeadMiner"))
		{
			return Miner;
		}
		return {};
	}

	/** The mark's levels as a card says them: its zone's, its rank's on top. */
	FText LevelsWord(const FBountyPosting& Posting)
	{
		const int32 Offset = UCreatureRankSettings::Get(BountyRules::GetTier(Posting.Tier).MarkRank).LevelOffset;
		const int32 Low = FMath::Max(Posting.LevelLow, 1) + Offset;
		const int32 High = FMath::Max(Posting.LevelHigh, Posting.LevelLow) + Offset;
		return Low == High ? FText::Format(LOCTEXT("LevelOne", "level {0}"), FText::AsNumber(Low))
			: FText::Format(LOCTEXT("LevelRange", "level {0}-{1}"), FText::AsNumber(Low), FText::AsNumber(High));
	}
}

FText BountyRules::TierWord(EBountyTier Tier)
{
	switch (Tier)
	{
	case EBountyTier::Dangerous:
		return LOCTEXT("TierDangerous", "Dangerous");
	case EBountyTier::Deadly:
		return LOCTEXT("TierDeadly", "Deadly");
	default:
		return LOCTEXT("TierWanted", "Wanted");
	}
}

FText BountyRules::TwistWord(EBountyTwist Twist)
{
	if (const TOptional<ECreatureTrait> Trait = TraitOf(Twist))
	{
		// The tag's own word for the trait, so the card and the creature's tag agree.
		return CreatureTraitRules::Word(*Trait);
	}
	return Twist == EBountyTwist::BigPosse ? LOCTEXT("TwistBigPosse", "Big posse") : LOCTEXT("TwistSwiftPosse", "Swift posse");
}

FText BountyRules::PackLine(EBountyTwist Twist)
{
	switch (Twist)
	{
	case EBountyTwist::BigPosse:
		return LOCTEXT("PackBig", "Big posse: one more in every push");
	case EBountyTwist::SwiftPosse:
		return LOCTEXT("PackSwift", "Swift posse: its whole crowd runs Fleet");
	default:
		return FText::GetEmpty();
	}
}

int32 BountyRules::NumNames(FName Kind)
{
	return NamesOf(Kind).Num();
}

FText BountyRules::MarkName(FName Kind, int32 Index)
{
	const TConstArrayView<FText> Names = NamesOf(Kind);
	if (Names.IsEmpty())
	{
		return LOCTEXT("NameUnknown", "the mark");
	}
	return Names[((Index % Names.Num()) + Names.Num()) % Names.Num()];
}

FText BountyRules::Flavour(FName Kind, int32 Serial)
{
	const TConstArrayView<FText> Lines = FlavourOf(Kind);
	return Lines.IsEmpty() ? FText::GetEmpty() : Lines[FMath::Abs(Serial) % Lines.Num()];
}

FText BountyRules::KindLine(const FBountyPosting& Posting)
{
	const FBountyKindInfo* Kind = FindKind(Posting.Kind);
	const FText RankWord = UCreatureRankSettings::Get(GetTier(Posting.Tier).MarkRank).Word;
	return FText::Format(LOCTEXT("KindLine", "{0} {1} · {2}"), RankWord, Kind ? Kind->Plain : FText::FromName(Posting.Kind),
		LevelsWord(Posting));
}

FText BountyRules::TraitsLine(const FBountyPosting& Posting)
{
	TArray<FText> Words;
	for (const ECreatureTrait Trait : Posting.MarkTraits)
	{
		Words.Add(CreatureTraitRules::Word(Trait));
	}
	return Words.IsEmpty() ? LOCTEXT("NoTraits", "No tricks") : FText::Join(LOCTEXT("TraitSeparator", " · "), Words);
}

FText BountyRules::TitleFor(const FBountyPosting& Posting)
{
	return FText::Format(LOCTEXT("Title", "Bounty: {0}"), MarkName(Posting.Kind, Posting.NameIndex));
}

FText BountyRules::HobName()
{
	return LOCTEXT("Hob", "Hob");
}

FText BountyRules::PushLine(const FBountyPosting& Posting)
{
	const FText Name = MarkName(Posting.Kind, Posting.NameIndex);
	if (Posting.Kind == TEXT("Spider") || Posting.Kind == TEXT("Spitter"))
	{
		return FText::Format(LOCTEXT("PushSpider", "Legs, sunshine. {0}'s brood comes first."), Name);
	}
	if (Posting.Kind == TEXT("DeadMiner"))
	{
		return FText::Format(LOCTEXT("PushMiner", "Shift's clocking on. {0}'s crew comes first."), Name);
	}
	return FText::Format(LOCTEXT("PushUnpaid", "That's {0}'s crowd, sunshine. Thin them out and it'll show."), Name);
}

FText BountyRules::MarkArriveLine(const FBountyPosting& Posting)
{
	return FText::Format(LOCTEXT("MarkArrive", "There's {0}. Paper says dead, so make it dead."), MarkName(Posting.Kind, Posting.NameIndex));
}

FText BountyRules::MarkDownLine(const FBountyPosting& Posting)
{
	return FText::Format(LOCTEXT("MarkDown", "That's {0} done. The paper's good at the board."), MarkName(Posting.Kind, Posting.NameIndex));
}

FText BountyRules::ClearedLine(const FBountyPosting& Posting)
{
	return LOCTEXT("Cleared", "That's the lot. The box is yours; the purse waits at the board.");
}

FBountyCardView BountyRules::MakeCard(const FBountyCardInput& In)
{
	FBountyCardView Card;
	const FBountyPosting* Posting = In.Posting;
	if (!Posting || !Posting->IsPosting())
	{
		Card.EmptyLine = In.RefillIn > 0.0 ? FText::Format(LOCTEXT("EmptyRefill", "Next posting in {0}"), DescribeWait(In.RefillIn))
			: LOCTEXT("EmptyNone", "More bounties as the Reaches open up.");
		return Card;
	}
	Card.bEmpty = false;
	Card.Tier = Posting->Tier;
	Card.State = Posting->State;
	Card.Title = MarkName(Posting->Kind, Posting->NameIndex);
	Card.TierWord = TierWord(Posting->Tier);
	Card.KindLine = KindLine(*Posting);
	Card.Traits = TraitsLine(*Posting);
	for (const EBountyTwist Twist : Posting->Twists)
	{
		if (IsPackTwist(Twist))
		{
			Card.Posse = PackLine(Twist);
		}
	}
	Card.Flavour = Flavour(Posting->Kind, Posting->Serial);
	Card.Where = FText::Format(LOCTEXT("Where", "Last seen: {0}"), In.ArenaName.IsEmpty() ? FText::FromName(Posting->Arena) : In.ArenaName);
	TArray<FText> Parts;
	Parts.Add(In.Guns == 1 ? LOCTEXT("RewardBoxOne", "Strongbox, 1 gun")
		: FText::Format(LOCTEXT("RewardBox", "Strongbox, {0} guns"), FText::AsNumber(In.Guns)));
	if (In.Purse > 0)
	{
		Parts.Add(FText::Format(LOCTEXT("RewardSilver", "+{0} silver"), FText::FromString(LooterEconomy::Format(In.Purse))));
	}
	if (In.Experience > 0)
	{
		Parts.Add(FText::Format(LOCTEXT("RewardXP", "+{0} XP"), FText::AsNumber(In.Experience)));
	}
	Card.Reward = FText::Join(LOCTEXT("RewardSeparator", " · "), Parts);
	switch (Posting->State)
	{
	case EBountyState::Taken:
		Card.StateWord = LOCTEXT("StateTaken", "Taken");
		Card.bCanGiveUp = true;
		break;
	case EBountyState::Won:
		Card.StateWord = LOCTEXT("StateWon", "Claim it");
		Card.bCanCollect = true;
		break;
	default:
		Card.bNew = !In.bSeen;
		Card.StateWord = Card.bNew ? LOCTEXT("StateNew", "New") : FText::GetEmpty();
		Card.bCanTake = !In.bOtherTaken;
		break;
	}
	return Card;
}

FString BountyRules::Describe(const FBountyPosting& Posting)
{
	if (!Posting.IsPosting())
	{
		return TEXT("(empty)");
	}
	TArray<FString> Twists;
	for (const EBountyTwist Twist : Posting.Twists)
	{
		Twists.Add(TwistName(Twist));
	}
	TArray<FString> Traits;
	for (const ECreatureTrait Trait : Posting.MarkTraits)
	{
		Traits.Add(CreatureTraitRules::Name(Trait));
	}
	return FString::Printf(TEXT("#%d %s %s '%s' at %s (%d-%d), twists %s, traits %s: %s"), Posting.Serial, *TierName(Posting.Tier),
		*Posting.Kind.ToString(), *MarkName(Posting.Kind, Posting.NameIndex).ToString(), *Posting.Arena.ToString(), Posting.LevelLow,
		Posting.LevelHigh, Twists.IsEmpty() ? TEXT("none") : *FString::Join(Twists, TEXT(", ")),
		Traits.IsEmpty() ? TEXT("none") : *FString::Join(Traits, TEXT(", ")), *StateName(Posting.State));
}

#undef LOCTEXT_NAMESPACE
