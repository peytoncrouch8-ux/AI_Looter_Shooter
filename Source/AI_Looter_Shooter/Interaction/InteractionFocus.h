#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "Interaction/InteractionTypes.h"
#include "Templates/Function.h"

class AActor;

/**
 * Choosing what the player means to use among the candidates near them, apart from the world so tests can feed it
 * candidates directly (a test level's traces don't see new bodies).
 */
namespace InteractionFocus
{
	/**
	 * How much nearness counts against aim: a whole reach of distance costs this much of the aim's cosine, about 11
	 * degrees off the crosshair's middle. Aim decides, and nearer wins when two are about as centered (a door in front of
	 * the bell behind it).
	 */
	inline constexpr float DistanceWeight = 0.02f;

	/** How directly the candidate lies along the crosshair's line: the cosine of its angle off it. */
	float AimOf(const FInteractionView& View, const FInteractionCandidate& Candidate);

	/** It can take the focus at all: something can be done with it, it's within its reach of the eyes, and inside the cone. */
	bool IsInReach(const FInteractionView& View, const FInteractionCandidate& Candidate, float MinAim);

	/** Its standing among the others: its aim, less a little for its distance. */
	float ScoreOf(const FInteractionView& View, const FInteractionCandidate& Candidate);

	/**
	 * The index of the candidate the player means, or INDEX_NONE: one in reach and in the cone (MinAim, the cosine of
	 * the cone's half angle), the most directly looked at with nearer winning near-ties, and in sight. IsInSight is
	 * asked best first, so usually once. Keep (what the key is held on) keeps the focus for as long as it's still in
	 * reach, in the cone and in sight, so a hold isn't lost to something beside it scoring a hair better.
	 */
	int32 Select(const FInteractionView& View, TConstArrayView<FInteractionCandidate> Candidates, float MinAim,
		TFunctionRef<bool(const FInteractionCandidate&)> IsInSight, const AActor* Keep = nullptr);
}
