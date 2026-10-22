// BountyRules: the kinds and tiers, the draw and the fight's groups. The timer, the pay and the names in code are in
// BountyRulesPay.cpp; the words (names, flavour, Hob's lines, the cards) in BountyRulesWords.cpp.

#include "Bounties/BountyRules.h"
#include "Bounties/BountyTuning.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/CreatureTraitRules.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Economy/EconomyRules.h"
#include "Loot/Chest.h"
#include "Math/RandomStream.h"
#include "Progression/LevelRules.h"
#include "Progression/XPCurve.h"

#define LOCTEXT_NAMESPACE "LooterBounty"

namespace
{
	/** Every twist, in the enum's order. */
	constexpr EBountyTwist AllTwists[] = { EBountyTwist::Fleet, EBountyTwist::Warded, EBountyTwist::Mending, EBountyTwist::Rallying,
		EBountyTwist::Vengeful, EBountyTwist::GraveBurst, EBountyTwist::BigPosse, EBountyTwist::SwiftPosse };

	/** Every tier, in the enum's order. */
	constexpr EBountyTier AllTiers[] = { EBountyTier::Wanted, EBountyTier::Dangerous, EBountyTier::Deadly };

	/** The posse in a push: Basic ones (and one more with a Big posse), Restless ones, the support. */
	struct FPushPlan
	{
		int32 Basic = 0;
		int32 Restless = 0;
		int32 Support = 0;
	};

	/** Each tier's pushes before the mark's, then the guards that come with the mark (Docs/Work/.../BOUNTY.md's table). */
	TArray<FPushPlan> PossePushes(EBountyTier Tier)
	{
		switch (Tier)
		{
		case EBountyTier::Dangerous:
			return { { 4, 1, 0 }, { 3, 0, 1 } };
		case EBountyTier::Deadly:
			return { { 4, 1, 0 }, { 3, 1, 1 } };
		default:
			return { { 4, 0, 0 } };
		}
	}

	FPushPlan GuardsWithMark(EBountyTier Tier)
	{
		switch (Tier)
		{
		case EBountyTier::Dangerous:
		case EBountyTier::Deadly:
			return { 0, 2, 0 };
		default:
			return { 2, 1, 0 };
		}
	}

	FEncounterGroup MakeGroup(TSubclassOf<ACreatureBase> Class, int32 Count, ECreatureRank Rank, int32 Wave)
	{
		FEncounterGroup Made;
		Made.CreatureClass = Class;
		Made.Count = FMath::Clamp(Count, 1, 16);
		// Fixed: never promoted on arrival nor stepped down by the area's caps, so the card says exactly what comes.
		Made.RankRoll = EEncounterRankRoll::Fixed;
		Made.Rank = Rank;
		Made.FirstWave = Wave;
		Made.LastWave = Wave;
		return Made;
	}

	/** Adds a push's posse groups (none of a count of 0). */
	void AddPush(FBountyFightPlan& Plan, const FBountyKindInfo& Kind, const FPushPlan& Push, int32 Wave, bool bBigPosse)
	{
		const int32 Basic = Push.Basic + (bBigPosse ? 1 : 0);
		if (Basic > 0)
		{
			Plan.Groups.Add(MakeGroup(Kind.Posse, Basic, ECreatureRank::Basic, Wave));
			Plan.Total += Basic;
		}
		if (Push.Restless > 0)
		{
			Plan.Groups.Add(MakeGroup(Kind.Posse, Push.Restless, ECreatureRank::Rare, Wave));
			Plan.Total += Push.Restless;
		}
		if (Push.Support > 0 && Kind.Support)
		{
			Plan.Groups.Add(MakeGroup(Kind.Support, Push.Support, ECreatureRank::Basic, Wave));
			Plan.Total += Push.Support;
		}
	}

	/** A weighted pick among Weights (all 0: the first). */
	int32 PickWeighted(TConstArrayView<float> Weights, FRandomStream& Random)
	{
		float Sum = 0.f;
		for (const float Weight : Weights)
		{
			Sum += FMath::Max(Weight, 0.f);
		}
		if (Sum <= 0.f)
		{
			return 0;
		}
		float Roll = Random.FRand() * Sum;
		for (int32 Index = 0; Index < Weights.Num(); ++Index)
		{
			Roll -= FMath::Max(Weights[Index], 0.f);
			if (Roll < 0.f)
			{
				return Index;
			}
		}
		return Weights.Num() - 1;
	}

	/** Picks one of Pool (and takes it out of it); none when it's empty. */
	TOptional<EBountyTwist> TakeTwist(TArray<EBountyTwist>& Pool, FRandomStream& Random)
	{
		if (Pool.IsEmpty())
		{
			return {};
		}
		const int32 Index = Random.RandRange(0, Pool.Num() - 1);
		const EBountyTwist Picked = Pool[Index];
		Pool.RemoveAt(Index);
		return Picked;
	}
}

// ---------------------------------------------------------------------------
// Kinds and tiers
// ---------------------------------------------------------------------------

TConstArrayView<FBountyKindInfo> BountyRules::Kinds()
{
	// Made on first use, not at static init: the classes' StaticClass needs the module up.
	static const TArray<FBountyKindInfo> Table = []()
	{
		TArray<FBountyKindInfo> Made;
		FBountyKindInfo& Unpaid = Made.AddDefaulted_GetRef();
		Unpaid.Id = TEXT("Unpaid");
		Unpaid.Mark = AUnpaidCreature::StaticClass();
		Unpaid.Posse = AUnpaidCreature::StaticClass();
		Unpaid.Support = ABellRingerCreature::StaticClass();
		Unpaid.Plain = LOCTEXT("KindUnpaid", "Unpaid");
		Unpaid.bRises = true;

		FBountyKindInfo& Spider = Made.AddDefaulted_GetRef();
		Spider.Id = TEXT("Spider");
		Spider.Mark = ASpiderCreature::StaticClass();
		Spider.Posse = ASpiderCreature::StaticClass();
		Spider.Support = ASpitterSpider::StaticClass();
		Spider.Plain = LOCTEXT("KindSpider", "spider");

		FBountyKindInfo& Spitter = Made.AddDefaulted_GetRef();
		Spitter.Id = TEXT("Spitter");
		Spitter.Mark = ASpitterSpider::StaticClass();
		Spitter.Posse = ASpiderCreature::StaticClass();
		Spitter.Support = ASpitterSpider::StaticClass();
		Spitter.Plain = LOCTEXT("KindSpitter", "Venom Spitter");

		FBountyKindInfo& Miner = Made.AddDefaulted_GetRef();
		Miner.Id = TEXT("DeadMiner");
		Miner.Mark = ADeadMinerCreature::StaticClass();
		Miner.Posse = ADeadMinerCreature::StaticClass();
		Miner.Support = ABellRingerCreature::StaticClass();
		Miner.Plain = LOCTEXT("KindMiner", "dead miner");
		Miner.bRises = true;
		return Made;
	}();
	return Table;
}

const FBountyKindInfo* BountyRules::FindKind(FName Id)
{
	for (const FBountyKindInfo& Kind : Kinds())
	{
		if (Kind.Id == Id)
		{
			return &Kind;
		}
	}
	return nullptr;
}

const FBountyTierInfo& BountyRules::GetTier(EBountyTier Tier)
{
	static const FBountyTierInfo Wanted = { ECreatureRank::Epic, 2, 1, 0.f, 1.f, 50.f };
	static const FBountyTierInfo Dangerous = { ECreatureRank::Hallowed, 3, 1, 0.35f, 1.f, 35.f };
	static const FBountyTierInfo Deadly = { ECreatureRank::Ethereal, 3, 1, 1.f, 0.5f, 15.f };
	switch (Tier)
	{
	case EBountyTier::Dangerous:
		return Dangerous;
	case EBountyTier::Deadly:
		return Deadly;
	default:
		return Wanted;
	}
}

TOptional<ECreatureTrait> BountyRules::TraitOf(EBountyTwist Twist)
{
	switch (Twist)
	{
	case EBountyTwist::Fleet:
		return ECreatureTrait::Fleet;
	case EBountyTwist::Warded:
		return ECreatureTrait::Warded;
	case EBountyTwist::Mending:
		return ECreatureTrait::Mending;
	case EBountyTwist::Rallying:
		return ECreatureTrait::Rallying;
	case EBountyTwist::Vengeful:
		return ECreatureTrait::Vengeful;
	case EBountyTwist::GraveBurst:
		return ECreatureTrait::GraveBurst;
	default:
		return {};
	}
}

bool BountyRules::IsPackTwist(EBountyTwist Twist)
{
	return Twist == EBountyTwist::BigPosse || Twist == EBountyTwist::SwiftPosse;
}

bool BountyRules::TwistFits(EBountyTwist Twist, const FBountyKindInfo& Kind)
{
	const ACreatureBase* Mark = Kind.Mark ? Kind.Mark->GetDefaultObject<ACreatureBase>() : nullptr;
	const ACreatureBase* Posse = Kind.Posse ? Kind.Posse->GetDefaultObject<ACreatureBase>() : nullptr;
	if (const TOptional<ECreatureTrait> Trait = TraitOf(Twist))
	{
		return Mark && CreatureTraitRules::Fits(*Trait, CreatureTraitRules::KindOf(*Mark));
	}
	// A swift posse is its posse Fleet (never a support kind's: the Spitter and the Bell-Ringer keep back); a big one fits all.
	if (Twist == EBountyTwist::SwiftPosse)
	{
		return Posse && CreatureTraitRules::Fits(ECreatureTrait::Fleet, CreatureTraitRules::KindOf(*Posse));
	}
	return true;
}

int32 BountyRules::SeedFor(FName BoardId, int32 Serial)
{
	// The name's characters, not the FName's own hash, which changes from run to run.
	return static_cast<int32>(HashCombineFast(FCrc::StrCrc32(*BoardId.ToString()), static_cast<uint32>(Serial) * 2654435761u));
}

FName BountyRules::NameKey(FName Kind, int32 NameIndex)
{
	return FName(*FString::Printf(TEXT("%s.%d"), *Kind.ToString(), NameIndex));
}

// ---------------------------------------------------------------------------
// The draw
// ---------------------------------------------------------------------------

TArray<ECreatureTrait> BountyRules::MarkTraitsFor(const FBountyKindInfo& Kind, EBountyTier Tier, TConstArrayView<EBountyTwist> Twists, int32 Seed)
{
	TArray<ECreatureTrait> Traits;
	for (const EBountyTwist Twist : Twists)
	{
		if (const TOptional<ECreatureTrait> Trait = TraitOf(Twist))
		{
			Traits.AddUnique(*Trait);
		}
	}
	const ECreatureRank Rank = GetTier(Tier).MarkRank;
	const int32 Wanted = Traits.Num() + FMath::Max(UCreatureRankSettings::Get(Rank).TraitCount, 0);
	const ACreatureBase* Mark = Kind.Mark ? Kind.Mark->GetDefaultObject<ACreatureBase>() : nullptr;
	if (!Mark)
	{
		return Traits;
	}
	// Its rank's own on top, from the seed, as the tier's roll would give them (the Ethereal's Phasing first).
	const TArray<ECreatureTrait> Rolled = CreatureTraitRules::Roll(CreatureTraits::NumTraits, Rank == ECreatureRank::Ethereal,
		CreatureTraitRules::KindOf(*Mark), Seed);
	for (const ECreatureTrait Trait : Rolled)
	{
		if (Traits.Num() >= Wanted)
		{
			break;
		}
		Traits.AddUnique(Trait);
	}
	return Traits;
}

FBountyPosting BountyRules::Draw(const FBountyDrawInput& In)
{
	FBountyPosting Made;
	FRandomStream Random(In.Seed);

	// The arena: one not busy, never one that is (two postings at one arena would stack two fights there).
	TArray<const FBountyArenaChoice*> Free;
	for (const FBountyArenaChoice& Arena : In.Arenas)
	{
		bool bHostsAny = false;
		for (const FName Kind : Arena.Kinds)
		{
			bHostsAny |= FindKind(Kind) != nullptr;
		}
		if (bHostsAny && !In.BusyArenas.Contains(Arena.Id))
		{
			Free.Add(&Arena);
		}
	}
	if (Free.IsEmpty())
	{
		return Made;
	}
	const FBountyArenaChoice& Arena = *Free[Random.RandRange(0, Free.Num() - 1)];

	// The mark's kind: one the arena hosts.
	TArray<const FBountyKindInfo*> Hosted;
	for (const FName Kind : Arena.Kinds)
	{
		if (const FBountyKindInfo* Info = FindKind(Kind))
		{
			Hosted.AddUnique(Info);
		}
	}
	const FBountyKindInfo& Kind = *Hosted[Random.RandRange(0, Hosted.Num() - 1)];

	// The tier, by weight among those the story allows.
	TArray<float, TInlineAllocator<3>> Weights;
	for (const EBountyTier Tier : AllTiers)
	{
		Weights.Add(static_cast<int32>(Tier) <= static_cast<int32>(In.HighestTier) ? GetTier(Tier).Weight : 0.f);
	}
	const EBountyTier Tier = AllTiers[PickWeighted(Weights, Random)];
	const FBountyTierInfo& TierInfo = GetTier(Tier);

	// Its twists: the mark's for sure, then perhaps one more (a pack twist, or another on the mark), each fitting its kind.
	TArray<EBountyTwist> MarkPool;
	TArray<EBountyTwist> PackPool;
	for (const EBountyTwist Twist : AllTwists)
	{
		if (TwistFits(Twist, Kind))
		{
			(IsPackTwist(Twist) ? PackPool : MarkPool).Add(Twist);
		}
	}
	for (int32 Count = 0; Count < TierInfo.MarkTwists; ++Count)
	{
		if (const TOptional<EBountyTwist> Twist = TakeTwist(MarkPool, Random))
		{
			Made.Twists.Add(*Twist);
		}
	}
	if (Random.FRand() < TierInfo.ExtraTwistChance)
	{
		const bool bPack = Random.FRand() < TierInfo.ExtraIsPackShare;
		if (const TOptional<EBountyTwist> Twist = TakeTwist(bPack ? PackPool : MarkPool, Random))
		{
			Made.Twists.Add(*Twist);
		}
	}

	// A name not up already (wrapping round its list, from a random start).
	const int32 Names = FMath::Max(NumNames(Kind.Id), 1);
	const int32 Start = Random.RandRange(0, Names - 1);
	int32 NameIndex = Start;
	for (int32 Step = 0; Step < Names; ++Step)
	{
		const int32 Candidate = (Start + Step) % Names;
		if (!In.BusyNames.Contains(NameKey(Kind.Id, Candidate)))
		{
			NameIndex = Candidate;
			break;
		}
	}

	Made.Serial = FMath::Max(In.Serial, 1);
	Made.Kind = Kind.Id;
	Made.Tier = Tier;
	Made.Arena = Arena.Id;
	Made.LevelLow = FMath::Max(Arena.LevelLow, 1);
	Made.LevelHigh = FMath::Max(Arena.LevelHigh, Made.LevelLow);
	Made.NameIndex = NameIndex;
	Made.MarkTraits = MarkTraitsFor(Kind, Tier, Made.Twists, In.Seed);
	Made.State = EBountyState::Posted;
	Made.PostedAt = In.Now;
	return Made;
}

// ---------------------------------------------------------------------------
// The fight
// ---------------------------------------------------------------------------

FBountyFightPlan BountyRules::MakeGroups(const FBountyPosting& Posting)
{
	FBountyFightPlan Plan;
	const FBountyKindInfo* Kind = FindKind(Posting.Kind);
	if (!Kind || !Kind->Mark || !Kind->Posse)
	{
		return Plan;
	}
	const FBountyTierInfo& Tier = GetTier(Posting.Tier);
	const bool bBigPosse = Posting.HasTwist(EBountyTwist::BigPosse);
	const TArray<FPushPlan> Pushes = PossePushes(Posting.Tier);
	for (int32 Index = 0; Index < Pushes.Num(); ++Index)
	{
		AddPush(Plan, *Kind, Pushes[Index], Index + 1, bBigPosse);
	}
	// The mark's push last: its group first, so it takes that push's first spot (the arena's mark spot).
	const int32 MarkWave = Pushes.Num() + 1;
	Plan.Groups.Add(MakeGroup(Kind->Mark, 1, Tier.MarkRank, MarkWave));
	Plan.Total += 1;
	AddPush(Plan, *Kind, GuardsWithMark(Posting.Tier), MarkWave, bBigPosse);
	Plan.Pushes = MarkWave;
	Plan.MarkRank = Tier.MarkRank;
	Plan.Mark = Kind->Mark;
	return Plan;
}

#undef LOCTEXT_NAMESPACE
