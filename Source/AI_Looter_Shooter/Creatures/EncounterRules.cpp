#include "Creatures/EncounterRules.h"
#include "Areas/AreaDefinition.h"
#include "Creatures/CreatureBase.h"
#include "Creatures/CreatureRankSettings.h"
#include "Creatures/EncounterGroup.h"
#include "Components/CapsuleComponent.h"

namespace
{
	/** Each spot a golden angle round from the last spreads them evenly over a disc, with no rings or rows to see. */
	constexpr double GoldenAngleDegrees = 137.50776405;

	/** Rings round each of a spawner's own points, for creatures beyond one a point. */
	constexpr int32 PointRings = 3;
	constexpr int32 SpotsPerRing = 6;
}

TArray<FVector> EncounterRules::CandidateSpots(const FVector& Center, float Radius, const TArray<FVector>& Points, int32 Wanted,
	float Spacing, float StartDegrees)
{
	TArray<FVector> Candidates;
	// Plenty: a safe zone, a fence's line or a change of level can rule out most of a spawner's ground.
	const int32 Enough = FMath::Clamp(Wanted * 6, 16, 240);
	if (Points.Num() > 0)
	{
		// Each point first, level with the spawner's ground; then rings round them, a spacing out, then two, then three.
		for (const FVector& Point : Points)
		{
			Candidates.Emplace(Point.X, Point.Y, Center.Z);
		}
		const double Step = FMath::Max(Spacing, 50.f);
		for (int32 Ring = 1; Ring <= PointRings && Candidates.Num() < Enough; ++Ring)
		{
			for (const FVector& Point : Points)
			{
				for (int32 Side = 0; Side < SpotsPerRing; ++Side)
				{
					const double Radians = FMath::DegreesToRadians(StartDegrees + 60.0 * Side + 30.0 * Ring);
					Candidates.Emplace(Point.X + FMath::Cos(Radians) * Step * Ring, Point.Y + FMath::Sin(Radians) * Step * Ring, Center.Z);
				}
			}
		}
		return Candidates;
	}

	// A sunflower's spread: evenly over the disc from the middle out, so a small group stands together and a big one
	// fills its ground.
	const double Reach = FMath::Max(Radius, 0.f);
	for (int32 Index = 0; Index < Enough; ++Index)
	{
		const double Out = Reach * FMath::Sqrt((Index + 0.5) / Enough);
		const double Radians = FMath::DegreesToRadians(StartDegrees + GoldenAngleDegrees * Index);
		Candidates.Emplace(Center.X + FMath::Cos(Radians) * Out, Center.Y + FMath::Sin(Radians) * Out, Center.Z);
	}
	return Candidates;
}

TArray<FVector> EncounterRules::ChooseSpots(const TArray<FVector>& Candidates, int32 Wanted, double HomeZ, float MaxStep,
	float Spacing, const TArray<FVector>& Occupied, TFunctionRef<FEncounterGroundHit(const FVector&)> GroundAt,
	TFunctionRef<bool(const FVector&)> IsBlocked)
{
	TArray<FVector> Chosen;
	// Spots where only a smaller body fits: kept for when too few fit the whole of it.
	TArray<FEncounterGroundHit> Cramped;
	const double ApartSquared = FMath::Square(static_cast<double>(Spacing));
	auto IsApart = [&Chosen, &Occupied, ApartSquared](const FVector& Spot)
	{
		auto TooClose = [&Spot, ApartSquared](const FVector& Other)
		{
			return FVector::DistSquared2D(Other, Spot) < ApartSquared;
		};
		return !Chosen.ContainsByPredicate(TooClose) && !Occupied.ContainsByPredicate(TooClose);
	};
	for (const FVector& Candidate : Candidates)
	{
		if (Chosen.Num() >= Wanted)
		{
			break;
		}
		if (IsBlocked(Candidate))
		{
			continue;
		}
		// On the spawner's own level of ground, where a creature can stand: creatures never step off a drop of more than
		// about 4 m, so a group on two levels would fight as two.
		const FEncounterGroundHit Ground = GroundAt(Candidate);
		if (!Ground.bFound || !Ground.bStandable || Ground.Room <= 0.f || FMath::Abs(Ground.Point.Z - HomeZ) > MaxStep)
		{
			continue;
		}
		if (Ground.Room < 1.f)
		{
			// A spider's room beside a den's jamb isn't the Gravemother's: a spot that fits the whole body comes first.
			Cramped.Add(Ground);
			continue;
		}
		if (IsApart(Ground.Point))
		{
			Chosen.Add(Ground.Point);
		}
	}

	if (Chosen.Num() < Wanted && Cramped.Num() > 0)
	{
		// Too few fit the whole body: the roomiest of the rest, rather than none. The sort keeps candidate order among
		// equals, so a spawner's own point wins a tie with the spots round it.
		Cramped.StableSort([](const FEncounterGroundHit& A, const FEncounterGroundHit& B)
		{
			return A.Room > B.Room;
		});
		for (const FEncounterGroundHit& Each : Cramped)
		{
			if (Chosen.Num() >= Wanted)
			{
				break;
			}
			if (IsApart(Each.Point))
			{
				Chosen.Add(Each.Point);
			}
		}
	}
	return Chosen;
}

float EncounterRules::LargestRankSize(const FEncounterGroup& Group, const UAreaDefinition* Area)
{
	auto SizeOf = [](ECreatureRank Rank)
	{
		// As ACreatureBase::ApplySize reads it.
		return FMath::Max(UCreatureRankSettings::Get(Rank).Size, 0.05f);
	};
	const float Basic = SizeOf(ECreatureRank::Basic);
	switch (Group.RankRoll)
	{
	case EEncounterRankRoll::Fixed:
		return SizeOf(Group.Rank);

	case EEncounterRankRoll::Chances:
		return FMath::Max3(Basic, Group.RareChance > 0.f ? SizeOf(ECreatureRank::Rare) : 0.f,
			Group.EpicChance > 0.f ? SizeOf(ECreatureRank::Epic) : 0.f);

	default:
		return FMath::Max3(Basic, Area && Area->RarePromotionChance > 0.f ? SizeOf(ECreatureRank::Rare) : 0.f,
			Area && Area->EpicPromotionChance > 0.f ? SizeOf(ECreatureRank::Epic) : 0.f);
	}
}

FEncounterBody EncounterRules::CreatureBody(TSubclassOf<ACreatureBase> Class, float BodyScale, float RankSize)
{
	const ACreatureBase* Defaults = Class ? Class->GetDefaultObject<ACreatureBase>() : nullptr;
	const UCapsuleComponent* Capsule = Defaults ? Defaults->GetCapsuleComponent() : nullptr;
	if (!Capsule)
	{
		return FEncounterBody();
	}
	// The class default's capsule, as SpawnAtRuntime and ApplySize grow it: the group's BodyScale (or the class's), times
	// its rank's size.
	const float StartScale = BodyScale > 0.f ? BodyScale : Defaults->BodyScale;
	const float Scale = FMath::Max(StartScale, 0.05f) * FMath::Max(RankSize, 0.05f);
	FEncounterBody Made;
	Made.Radius = FMath::Max(Capsule->GetUnscaledCapsuleRadius() * Scale, 1.f);
	Made.HalfHeight = FMath::Max(Capsule->GetUnscaledCapsuleHalfHeight() * Scale, Made.Radius);
	return Made;
}

FEncounterBody EncounterRules::LargestBody(const TArray<FEncounterGroup>& Groups, const UAreaDefinition* Area)
{
	FEncounterBody Largest;
	bool bAny = false;
	for (const FEncounterGroup& Group : Groups)
	{
		if (!Group.CreatureClass)
		{
			continue;
		}
		const FEncounterBody Body = CreatureBody(Group.CreatureClass, Group.BodyScale, LargestRankSize(Group, Area));
		// The widest and the tallest, which may be two kinds (a spider and an Unpaid): room for either.
		Largest.Radius = bAny ? FMath::Max(Largest.Radius, Body.Radius) : Body.Radius;
		Largest.HalfHeight = bAny ? FMath::Max(Largest.HalfHeight, Body.HalfHeight) : Body.HalfHeight;
		bAny = true;
	}
	return Largest;
}

int32 EncounterRules::Room(int32 Alive, int32 Cap)
{
	return Cap > 0 ? FMath::Max(Cap - Alive, 0) : MAX_int32;
}

ECreatureRank EncounterRules::PickRank(const FEncounterGroup& Group, const UAreaDefinition* Area, float Roll)
{
	switch (Group.RankRoll)
	{
	case EEncounterRankRoll::Fixed:
		return Group.Rank;

	case EEncounterRankRoll::Chances:
	{
		// The rarer rank first, as the area's promotions roll it, so chances that add up past 1 still give it its share.
		const float Epic = FMath::Clamp(Group.EpicChance, 0.f, 1.f);
		const float Rare = FMath::Clamp(Group.RareChance, 0.f, 1.f);
		if (Roll < Epic)
		{
			return ECreatureRank::Epic;
		}
		return Roll < Epic + Rare ? ECreatureRank::Rare : ECreatureRank::Basic;
	}

	default:
		return Area ? Area->PickPromotion(Roll) : ECreatureRank::Basic;
	}
}

bool EncounterRules::JoinsWave(const FEncounterGroup& Group, int32 Wave)
{
	return Wave >= FMath::Max(Group.FirstWave, 1) && (Group.LastWave <= 0 || Wave <= Group.LastWave);
}

int32 EncounterRules::PlannedWaves(int32 NumWaves, int32 MaxTotal)
{
	if (NumWaves > 0)
	{
		return NumWaves;
	}
	return MaxTotal > 0 ? MAX_int32 : 1;
}

bool EncounterRules::HasWavesLeft(int32 WavesStarted, int32 NumWaves, int32 TotalQueued, int32 MaxTotal)
{
	return WavesStarted < PlannedWaves(NumWaves, MaxTotal) && (MaxTotal <= 0 || TotalQueued < MaxTotal);
}

bool EncounterRules::IsWaveDue(bool bWavesLeft, float WaveClock, float WaveInterval, bool bWaitForClear, int32 Remaining)
{
	if (!bWavesLeft)
	{
		return false;
	}
	if (bWaitForClear)
	{
		return Remaining <= 0 && WaveClock >= WaveInterval;
	}
	return WaveInterval > 0.f && WaveClock >= WaveInterval;
}
