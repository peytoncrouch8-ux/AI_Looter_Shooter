#include "Creatures/CreatureTraitRules.h"
#include "Creatures/BellRingerCreature.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureTraitTuning.h"
#include "Creatures/DeadMinerCreature.h"
#include "Creatures/SlimeCreature.h"
#include "Creatures/SpiderCreature.h"
#include "Creatures/SpitterSpider.h"
#include "Creatures/UnpaidCreature.h"
#include "Math/RandomStream.h"

namespace
{
	/** The angles (degrees off the way from the hitter) a blink tries, in order: square aside, a little away, a little toward. */
	constexpr float BlinkAngles[] = { 90.f, 65.f, 115.f };
}

namespace CreatureTraitRules
{
	ECreatureTraitKind KindOf(const ACreatureBase& Creature)
	{
		// Most specific first: the Bell-Ringer and the dead miners are built on the Unpaid, the Spitter on the spider.
		if (Creature.IsA<ABellRingerCreature>())
		{
			return ECreatureTraitKind::BellRinger;
		}
		if (Creature.IsA<ADeadMinerCreature>())
		{
			return ECreatureTraitKind::DeadMiner;
		}
		if (Creature.IsA<AUnpaidCreature>())
		{
			return ECreatureTraitKind::Unpaid;
		}
		if (Creature.IsA<ASpitterSpider>())
		{
			return ECreatureTraitKind::Spitter;
		}
		if (Creature.IsA<ASpiderCreature>())
		{
			return ECreatureTraitKind::Spider;
		}
		if (Creature.IsA<ASlimeCreature>())
		{
			return ECreatureTraitKind::Slime;
		}
		return ECreatureTraitKind::Other;
	}

	bool Fits(ECreatureTrait Trait, ECreatureTraitKind Kind)
	{
		const bool bKeepsAway = Kind == ECreatureTraitKind::Spitter || Kind == ECreatureTraitKind::BellRinger;
		switch (Trait)
		{
		case ECreatureTrait::Fleet:
		case ECreatureTrait::GraveBurst:
			return !bKeepsAway;
		case ECreatureTrait::Vengeful:
			return Kind != ECreatureTraitKind::BellRinger;
		case ECreatureTrait::Mending:
		case ECreatureTrait::Warded:
		case ECreatureTrait::Rallying:
		case ECreatureTrait::Phasing:
			return true;
		default:
			return false;
		}
	}

	TArray<ECreatureTrait> Roll(int32 Count, bool bEthereal, ECreatureTraitKind Kind, int32 Seed)
	{
		TArray<ECreatureTrait> Rolled;
		if (Count <= 0)
		{
			return Rolled;
		}
		// The Ethereal's own first; nobody else ever phases.
		if (bEthereal && Fits(ECreatureTrait::Phasing, Kind))
		{
			Rolled.Add(ECreatureTrait::Phasing);
		}
		TArray<ECreatureTrait, TInlineAllocator<CreatureTraits::NumTraits>> Pool;
		for (const ECreatureTrait Trait : CreatureTraits::All)
		{
			if (Trait != ECreatureTrait::Phasing && Fits(Trait, Kind))
			{
				Pool.Add(Trait);
			}
		}
		// A shuffle from the seed alone (Fisher-Yates), so a load rolls the same traits again.
		FRandomStream Random(Seed);
		for (int32 Index = Pool.Num() - 1; Index > 0; --Index)
		{
			Pool.Swap(Index, Random.RandRange(0, Index));
		}
		for (const ECreatureTrait Trait : Pool)
		{
			if (Rolled.Num() >= Count)
			{
				break;
			}
			Rolled.Add(Trait);
		}
		return Rolled;
	}

	FText Word(ECreatureTrait Trait)
	{
		switch (Trait)
		{
		case ECreatureTrait::Fleet: return NSLOCTEXT("CreatureTraits", "Fleet", "Fleet");
		case ECreatureTrait::Mending: return NSLOCTEXT("CreatureTraits", "Mending", "Mending");
		case ECreatureTrait::Warded: return NSLOCTEXT("CreatureTraits", "Warded", "Warded");
		case ECreatureTrait::GraveBurst: return NSLOCTEXT("CreatureTraits", "GraveBurst", "Grave-burst");
		case ECreatureTrait::Rallying: return NSLOCTEXT("CreatureTraits", "Rallying", "Rallying");
		case ECreatureTrait::Vengeful: return NSLOCTEXT("CreatureTraits", "Vengeful", "Vengeful");
		case ECreatureTrait::Phasing: return NSLOCTEXT("CreatureTraits", "Phasing", "Phasing");
		default: return FText::GetEmpty();
		}
	}

	FString Name(ECreatureTrait Trait)
	{
		return StaticEnum<ECreatureTrait>()->GetNameStringByValue(static_cast<int64>(Trait));
	}

	bool Parse(const FString& Text, ECreatureTrait& OutTrait)
	{
		const FString Wanted = Text.TrimStartAndEnd();
		for (const ECreatureTrait Candidate : CreatureTraits::All)
		{
			if (Wanted.Equals(Name(Candidate), ESearchCase::IgnoreCase) || Wanted.Equals(Word(Candidate).ToString(), ESearchCase::IgnoreCase))
			{
				OutTrait = Candidate;
				return true;
			}
		}
		return false;
	}

	float AbsorbWard(float& InOutWard, float Damage, bool bCritical, float CritBreakScale)
	{
		if (InOutWard <= 0.f || Damage <= 0.f)
		{
			return FMath::Max(Damage, 0.f);
		}
		const float Scale = bCritical ? FMath::Max(CritBreakScale, 0.01f) : 1.f;
		const float Against = Damage * Scale;
		if (Against < InOutWard)
		{
			InOutWard -= Against;
			return 0.f;
		}
		// It breaks on this hit: the part of the hit it took to break it is spent, the rest goes on to health.
		const float Spent = InOutWard / Scale;
		InOutWard = 0.f;
		return FMath::Max(Damage - Spent, 0.f);
	}

	float MendAmount(float SinceHit, float UnhitSeconds, float HealSharePerSecond, float MaxHealth, float DeltaSeconds)
	{
		if (SinceHit < UnhitSeconds || DeltaSeconds <= 0.f || MaxHealth <= 0.f)
		{
			return 0.f;
		}
		return FMath::Max(HealSharePerSecond, 0.f) * MaxHealth * DeltaSeconds;
	}

	float BurstShare(float Distance, const FGraveBurstTraitTuning& Tuning)
	{
		if (Distance > Tuning.Radius)
		{
			return 0.f;
		}
		const float Full = FMath::Min(Tuning.FullRadius, Tuning.Radius);
		if (Distance <= Full || Tuning.Radius - Full <= UE_KINDA_SMALL_NUMBER)
		{
			return 1.f;
		}
		const float Across = (Distance - Full) / (Tuning.Radius - Full);
		return FMath::Lerp(1.f, FMath::Clamp(Tuning.EdgeShare, 0.f, 1.f), Across);
	}

	bool IsHardHit(float RecentDamage, float MaxHealth, float HardHitShare)
	{
		return MaxHealth > 0.f && RecentDamage >= HardHitShare * MaxHealth;
	}

	TArray<FVector> BlinkCandidates(const FVector& Feet, const FVector& Away, bool bRightFirst, const FPhasingTraitTuning& Tuning)
	{
		TArray<FVector> Candidates;
		FVector Flat(Away.X, Away.Y, 0.0);
		if (!Flat.Normalize())
		{
			Flat = FVector::ForwardVector;
		}
		const float Shortest = FMath::Min(Tuning.MinDistance, Tuning.MaxDistance);
		const float Longest = FMath::Max(Tuning.MinDistance, Tuning.MaxDistance);
		const float Distances[] = { Longest, (Shortest + Longest) * 0.5f, Shortest };
		Candidates.Reserve(static_cast<int32>(2 * UE_ARRAY_COUNT(BlinkAngles) * UE_ARRAY_COUNT(Distances)));
		for (const float Side : { bRightFirst ? 1.f : -1.f, bRightFirst ? -1.f : 1.f })
		{
			for (const float Angle : BlinkAngles)
			{
				const FVector Way = Flat.RotateAngleAxis(Side * Angle, FVector::UpVector);
				for (const float Distance : Distances)
				{
					Candidates.Add(Feet + Way * Distance);
				}
			}
		}
		return Candidates;
	}
}
