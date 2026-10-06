#include "Creatures/EncounterRules.h"
#include "Areas/AreaDefinition.h"
#include "Creatures/EncounterGroup.h"

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
	const double ApartSquared = FMath::Square(static_cast<double>(Spacing));
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
		if (!Ground.bFound || !Ground.bStandable || FMath::Abs(Ground.Point.Z - HomeZ) > MaxStep)
		{
			continue;
		}
		auto TooClose = [&Ground, ApartSquared](const FVector& Other)
		{
			return FVector::DistSquared2D(Other, Ground.Point) < ApartSquared;
		};
		if (Chosen.ContainsByPredicate(TooClose) || Occupied.ContainsByPredicate(TooClose))
		{
			continue;
		}
		Chosen.Add(Ground.Point);
	}
	return Chosen;
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
