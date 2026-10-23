#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "Interaction/InteractionTypes.h"
#include "Templates/Function.h"

class AActor;
class UPrimitiveComponent;

/**
 * Choosing what the player means to use among the candidates near them, apart from the world so tests can feed it
 * candidates (and the hits a trace would make) directly (a test level's traces don't see new bodies).
 */
namespace InteractionFocus
{
	/**
	 * How much nearness counts against aim: a whole reach of distance costs this much of the aim's cosine, about 11
	 * degrees off the crosshair's middle. Aim decides, and nearer wins when two are about as centered (a door in front of
	 * the bell behind it).
	 */
	inline constexpr float DistanceWeight = 0.02f;

	/**
	 * What each step of being on the crosshair's line (FInteractionCandidate::LineRank) adds to a candidate's standing: more
	 * than aim and distance ever take.
	 */
	inline constexpr float OnLineBonus = 1.f;

	/** Nearer than this to the eyes (cm), a level's interactable takes its widest cone. */
	inline constexpr float ConeCloseDistance = 100.f;

	/** A sight line blocked this close to the point looked at still sees it (a poster's own wall, a board's backing). */
	inline constexpr float SightTolerance = 15.f;

	/**
	 * A small thing lying about (a page, a letter, coins, a plate) is looked at as at least this much round it (half sizes,
	 * cm), from the surface it lies on up (GrownBox): found looked at near it, not only dead on its few centimetres.
	 */
	inline constexpr double SmallThingHalfWidth = 16.0;
	inline constexpr double SmallThingHalfHeight = 10.0;

	/**
	 * The point of Box the crosshair's line (View, out to Length) means: where the line enters it when it passes through it
	 * (the line's start when that's inside), else the box's point nearest the line ahead of it. A box behind the line's
	 * start gives its point nearest the start (which lies behind: never looked at).
	 */
	FVector NearestOnBox(const FInteractionView& View, const FInteractionBox& Box, double Length);

	/**
	 * A box round world Bounds (axis-aligned) grown to at least MinHalfWidth across and MinHalfHeight up: wider about its
	 * middle, taller only upward from its bottom (a small thing lying on a ledge is never looked for inside the ledge).
	 */
	FInteractionBox GrownBox(const FBox& Bounds, double MinHalfWidth, double MinHalfHeight);

	/** A box Half each way round Middle, turned as Turn. */
	FInteractionBox BoxAround(const FVector& Middle, const FVector& Half, const FQuat& Turn = FQuat::Identity);

	/** How directly the candidate lies along the crosshair's line: the cosine of its angle off it. */
	float AimOf(const FInteractionView& View, const FInteractionCandidate& Candidate);

	/**
	 * The cone (a cosine) for something Distance from the eyes that can be used from Reach: NearAim within
	 * ConeCloseDistance, narrowing evenly to FarAim at its reach. A little help up close, where a pad's stick is coarse;
	 * farther off it must really be looked at.
	 */
	float NarrowingCone(float NearAim, float FarAim, float Distance, float Reach);

	/**
	 * It can take the focus at all: something can be done with it, it's within its reach of the eyes, and inside its cone
	 * (its own MinAim, else this MinAim).
	 */
	bool IsInReach(const FInteractionView& View, const FInteractionCandidate& Candidate, float MinAim);

	/**
	 * What the crosshair's line meeting Part of HitActor means to use: HitActor, or the actor it's fixed to (a door's
	 * parts, the moored skiff on its jetty), when that interactable counts Part as the thing to use
	 * (IInteractable::IsInteractionPart). Null otherwise: the line met a wall like any other.
	 */
	AActor* FindLookedAt(AActor* HitActor, const UPrimitiveComponent* Part);

	/**
	 * A blocking hit at HitPoint on the line from the eyes to the candidate hides it. Its own part to use never does (a
	 * door's leaf, the board's slate), nor what it stands against right at its point (SightTolerance), nor anything inside its
	 * own box (Candidate.Box, grown by SightTolerance: a brazier's basket, a gantry round its clamp); anything else does,
	 * its own walls included (a station's board isn't read through the depot).
	 */
	bool DoesHitHide(const FInteractionCandidate& Candidate, const AActor* HitActor, const UPrimitiveComponent* Part, const FVector& HitPoint);

	/** Its standing among the others: its aim, less a little for its distance. */
	float ScoreOf(const FInteractionView& View, const FInteractionCandidate& Candidate);

	/**
	 * The index of the candidate the player means, or INDEX_NONE: one in reach and in its cone (its own MinAim, else this
	 * MinAim: the cosine of the cone's half angle), the most directly looked at with nearer winning near-ties, and in sight. IsInSight is
	 * asked best first, so usually once. Keep (what the key is held on) keeps the focus for as long as it's still in
	 * reach, in the cone and in sight, so a hold isn't lost to something beside it scoring a hair better.
	 */
	int32 Select(const FInteractionView& View, TConstArrayView<FInteractionCandidate> Candidates, float MinAim,
		TFunctionRef<bool(const FInteractionCandidate&)> IsInSight, const AActor* Keep = nullptr);
}
