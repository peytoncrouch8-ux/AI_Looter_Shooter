#include "Interaction/InteractionFocus.h"

float InteractionFocus::AimOf(const FInteractionView& View, const FInteractionCandidate& Candidate)
{
	const FVector ToCandidate = Candidate.Location - View.Location;
	// Right where the line starts: as straight ahead as it gets.
	if (ToCandidate.IsNearlyZero())
	{
		return 1.f;
	}
	return static_cast<float>(FVector::DotProduct(ToCandidate.GetSafeNormal(), View.Direction.GetSafeNormal()));
}

bool InteractionFocus::IsInReach(const FInteractionView& View, const FInteractionCandidate& Candidate, float MinAim)
{
	if (!Candidate.Options.CanUse())
	{
		return false;
	}
	const double ReachSquared = FMath::Square(static_cast<double>(Candidate.Reach));
	return FVector::DistSquared(Candidate.Location, View.ReachOrigin) <= ReachSquared && AimOf(View, Candidate) > MinAim;
}

float InteractionFocus::ScoreOf(const FInteractionView& View, const FInteractionCandidate& Candidate)
{
	const float Distance = static_cast<float>(FVector::Dist(Candidate.Location, View.ReachOrigin));
	return AimOf(View, Candidate) - DistanceWeight * Distance / FMath::Max(Candidate.Reach, 1.f);
}

int32 InteractionFocus::Select(const FInteractionView& View, TConstArrayView<FInteractionCandidate> Candidates, float MinAim,
	TFunctionRef<bool(const FInteractionCandidate&)> IsInSight, const AActor* Keep)
{
	TArray<TPair<float, int32>, TInlineAllocator<16>> Ranked;
	for (int32 Index = 0; Index < Candidates.Num(); ++Index)
	{
		const FInteractionCandidate& Candidate = Candidates[Index];
		if (!IsInReach(View, Candidate, MinAim))
		{
			continue;
		}
		// The key is held on it: it stays the focus while it can, whatever else comes close.
		if (Keep && Candidate.Actor == Keep && IsInSight(Candidate))
		{
			return Index;
		}
		Ranked.Emplace(ScoreOf(View, Candidate), Index);
	}

	// Best first; equal scores keep the order they were gathered in.
	Ranked.StableSort([](const TPair<float, int32>& A, const TPair<float, int32>& B) { return A.Key > B.Key; });
	for (const TPair<float, int32>& Entry : Ranked)
	{
		if (IsInSight(Candidates[Entry.Value]))
		{
			return Entry.Value;
		}
	}
	return INDEX_NONE;
}
