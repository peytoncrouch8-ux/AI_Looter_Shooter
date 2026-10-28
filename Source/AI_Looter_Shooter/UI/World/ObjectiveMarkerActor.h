#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ObjectiveMarkerActor.generated.h"

class UWidgetComponent;

/**
 * The tracked objective's marker over a paper (a wanted poster, a note: UObjectiveHighlightSubsystem::NeedsMarker), whose
 * orange highlight is hard to see past a few metres: a decal has no mesh to wear the overlay, and a sheet seen at an angle
 * is a sliver. A screen-space UObjectiveMarkerWidget (an orange ring-diamond round a dot, in a soft glow of it), so it is
 * seen across the highlight's whole radius and goes with the other labels over the world under a menu or a scene
 * (ALooterHUD::ShowWorldLabels). It stands just over the paper, pops in as it lights, breathes, fades with the highlight,
 * and fades out close up (NearFade), where the paper reads by itself and a mark would only sit over what's being read.
 *
 * The highlight subsystem keeps them pooled and drives each one every frame while its paper is lit (Advance): it has no
 * tick of its own, and a free one is hidden.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AObjectiveMarkerActor : public AActor
{
	GENERATED_BODY()

public:
	/** Gone this near the player (cm), whole from FullFrom on. */
	static constexpr float GoneWithin = 250.f;
	static constexpr float FullFrom = 450.f;
	/** Seconds the near fade takes to follow the distance (which the subsystem looks at a few times a second). */
	static constexpr float NearFadeSeconds = 0.25f;
	/** How far over the paper's top it stands (cm). */
	static constexpr float OverTop = 15.f;
	/** Parts of the paper bigger than this (bounds' radius, cm) aren't the paper: a board, a wall. */
	static constexpr float MostPaperRadius = 100.f;

	AObjectiveMarkerActor();

	/** Shows it for InTarget, standing at Point (MarkPoint); popping in from nothing. */
	void Show(AActor* InTarget, const FVector& Point);

	/** Hidden and free for another paper. */
	void Release();

	bool IsFree() const { return !Target.IsValid(); }
	AActor* GetTarget() const { return Target.Get(); }

	/**
	 * Its look this frame: Strength the highlight's as shown (0-1, eased), Distance its paper's from the player (cm), Point
	 * where it stands. The subsystem calls it every frame while the paper is lit or easing out; tests too.
	 */
	void Advance(float DeltaSeconds, float Strength, float Distance, const FVector& Point);

	/** How solid it is now (0-1): the highlight's strength by the near fade. */
	float GetPresence() const { return Presence; }

	/** Where it stands over InTarget: Focus's spot, a little over the top of its paper (its decals and small meshes). */
	static FVector MarkPoint(const AActor& InTarget, const FVector& Focus);

	/** How much of it shows Distance cm from the player: nothing within GoneWithin, all from FullFrom. */
	static float NearFade(float Distance);

	/** The pop as it appears, Age seconds in: from nothing, overshooting, settling to 1. */
	static float PopAt(float Age);

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Widget;

private:
	TWeakObjectPtr<AActor> Target;
	/** Seconds since it was shown. */
	float Age = 0.f;
	float Presence = 0.f;
	/** The near fade as it follows the distance; below 0 until the first look (then it starts where it should be). */
	float Near = -1.f;
	/** Its breath's own phase, so two marks side by side don't breathe together. */
	float Phase = 0.f;
};
