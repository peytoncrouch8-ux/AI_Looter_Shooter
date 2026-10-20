#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EmberMarkActor.generated.h"

class UWidgetComponent;

/**
 * An ember power's mark over a creature (Spyglass and Long Shot's diamond and distance, Last Rites' sigil and its seconds):
 * a screen-space UEmberMarkWidget, so it shows through walls and goes with the other world labels under a menu or a scene
 * (ALooterHUD::ShowWorldLabels hides their layer). It follows its creature, pops in as it's marked, breathes, eases a little
 * smaller far off (never gone: a mark is meant to be seen across the field), and fades out once retired, then goes.
 * The power that marks keeps it and retires it (its time up, the creature dead); a mark whose creature is gone retires
 * itself.
 */
UCLASS(NotPlaceable, Transient)
class AI_LOOTER_SHOOTER_API AEmberMarkActor : public AActor
{
	GENERATED_BODY()

public:
	AEmberMarkActor();

	/** A new mark over InTarget in Color (IconName None: the diamond; else that Inked icon). Null without a world. */
	static AEmberMarkActor* Mark(AActor* InTarget, const FLinearColor& Color, FName IconName = NAME_None);

	virtual void Tick(float DeltaSeconds) override;

	/** Shows the distance from the player under it (Spyglass, Long Shot); off for a countdown. On by default. */
	void SetShowsDistance(bool bShows) { bShowsDistance = bShows; }

	/** Shows whole seconds left under it (Last Rites; rounded up, never 0); a negative clears it. */
	void SetCountdown(float Seconds) { Countdown = Seconds; }

	/** A strong beat of the glow and a pop (the mark struck, a long shot landed). */
	void Flare();

	/** Fades out, then goes. */
	void Retire();
	bool IsRetired() const { return bRetired; }

	AActor* GetTarget() const { return Target.Get(); }

	/** Moves its look on (the tick does; a test level never ticks, so tests call it). */
	void Advance(float DeltaSeconds);

	/** What it says under its glyph now ("34 m", "6"), for the tests. */
	FString GetWords() const;

	/** Where it floats over InTarget: over the top of its body (and its tag), by its size. */
	static FVector MarkPoint(const AActor& InTarget);

	/** The pop as it appears, Age seconds in: from nothing, overshooting, settling to 1. */
	static float PopAt(float Age);

	/** "34 m": whole metres. */
	static FString DistanceWords(float Centimetres);

	/** Seconds a fade out takes. */
	static constexpr float FadeSeconds = 0.3f;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
	TObjectPtr<UWidgetComponent> Widget;

private:
	FString WordsNow() const;

	TWeakObjectPtr<AActor> Target;
	FLinearColor Tint = FLinearColor::White;
	FName Icon;
	bool bShowsDistance = true;
	float Countdown = -1.f;
	bool bRetired = false;
	float Age = 0.f;
	float Presence = 1.f;
	float FlareAge = 10.f;
	float Phase = 0.f;
};
