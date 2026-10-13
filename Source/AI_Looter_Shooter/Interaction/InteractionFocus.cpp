#include "Interaction/InteractionFocus.h"
#include "Interaction/Interactable.h"
#include "GameFramework/Actor.h"

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

float InteractionFocus::NarrowingCone(float NearAim, float FarAim, float Distance, float Reach)
{
	// Evenly from the close distance out to the reach (a reach inside the close distance: the near cone all the way).
	const float Span = Reach - ConeCloseDistance;
	const float Along = Span > UE_KINDA_SMALL_NUMBER ? FMath::Clamp((Distance - ConeCloseDistance) / Span, 0.f, 1.f) : 0.f;
	return FMath::Lerp(NearAim, FarAim, Along);
}

bool InteractionFocus::IsInReach(const FInteractionView& View, const FInteractionCandidate& Candidate, float MinAim)
{
	if (!Candidate.Options.CanUse())
	{
		return false;
	}
	const double ReachSquared = FMath::Square(static_cast<double>(Candidate.Reach));
	return FVector::DistSquared(Candidate.Location, View.ReachOrigin) <= ReachSquared && AimOf(View, Candidate) > Candidate.MinAim.Get(MinAim);
}

AActor* InteractionFocus::FindLookedAt(AActor* HitActor, const UPrimitiveComponent* Part)
{
	// The interactable the hit belongs to: the actor hit, or the one it's fixed to.
	for (AActor* Candidate = HitActor; Candidate; Candidate = Candidate->GetAttachParentActor())
	{
		if (const IInteractable* Interactable = Cast<IInteractable>(Candidate))
		{
			// Only its part to use offers it: the jetty's deck under the gangplank is just a deck.
			return Interactable->IsInteractionPart(Part) ? Candidate : nullptr;
		}
	}
	return nullptr;
}

bool InteractionFocus::DoesHitHide(const FInteractionCandidate& Candidate, const AActor* HitActor, const UPrimitiveComponent* Part, const FVector& HitPoint)
{
	if (HitActor && Candidate.Actor && (HitActor == Candidate.Actor || HitActor->IsAttachedTo(Candidate.Actor)))
	{
		// Its own part to use doesn't hide it. What it only stands in does (the depot's walls), as any wall would; something
		// offered by a player's component (loot) keeps the old rule: none of its own parts hide it.
		const IInteractable* Interactable = Cast<IInteractable>(Candidate.Actor);
		if (!Interactable || Interactable->IsInteractionPart(Part))
		{
			return false;
		}
	}
	return FVector::Dist(HitPoint, Candidate.Location) > SightTolerance;
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
