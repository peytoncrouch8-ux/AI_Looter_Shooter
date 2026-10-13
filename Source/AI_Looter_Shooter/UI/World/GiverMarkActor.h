#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/Style/MissionMarkIcons.h"
#include "GiverMarkActor.generated.h"

class UWidgetComponent;
struct FGiverMark;

/**
 * A giver's ! or ? floating over their head (UGiverMarkSubsystem places one per marked giver and reuses them): a
 * screen-space UGiverMarkWidget, so it joins the other labels over the world and goes with them under a menu or a scene
 * (ALooterHUD::ShowWorldLabels). It follows its giver, eases with the player's distance (full near, gone past about 40 m,
 * a little smaller far off), bobs, and pops in as it appears or changes (an offer to a turn-in); retired, it fades out
 * and waits to be used again. It ticks only while it shows.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AGiverMarkActor : public AActor
{
	GENERATED_BODY()

public:
	AGiverMarkActor();

	virtual void Tick(float DeltaSeconds) override;

	/** Shows Mark over its giver; a new giver, or another mark (offer to turn-in, side to main), pops in again. */
	void Show(const FGiverMark& Mark);

	/** Its giver has nothing more for the player: it fades out, then hides until it's shown again. */
	void Retire();

	/** Hidden and free for another giver. */
	bool IsFree() const { return !Giver.IsValid() || (bRetired && Presence <= 0.f); }

	AActor* GetGiver() const { return Giver.Get(); }

	/** Moves its look on (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	/** How visible a mark is Distance cm from the player: whole near, fading from 34 m to nothing at 40 m. */
	static float AlphaAt(float Distance);

	/** How big: whole up to 8 m, then smaller with distance to 70% at 40 m. */
	static float ScaleAt(float Distance);

	/** The pop as it appears, Age seconds in: from nothing, overshooting, settling to 1. */
	static float PopAt(float Age);

	/** Seconds a fade in or out takes. */
	static constexpr float FadeSeconds = 0.25f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Widget;

private:
	TWeakObjectPtr<AActor> Giver;
	/** Where the mark floats from the giver's own spot (kept as it's shown: a giver turning in place doesn't move it). */
	FVector Offset = FVector::ZeroVector;
	MissionMarkIcons::EMark Mark = MissionMarkIcons::EMark::Offer;
	bool bMain = false;
	bool bRetired = true;
	/** Seconds since it last popped in. */
	float Age = 0.f;
	/** Its fade, 0 (gone) to 1. */
	float Presence = 0.f;
	/** Its bob's own phase, so marks side by side don't bob together. */
	float Phase = 0.f;
};
