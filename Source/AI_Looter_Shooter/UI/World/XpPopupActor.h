#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "XpPopupActor.generated.h"

class UWidgetComponent;

/**
 * One floating "+38 XP" (UXpPopupWidget), kept by UXpPopupSubsystem and used again and again: a kill's is small and stays
 * on the kill's spot a beat after the killing blow's damage number, a little to its right, rising and fading; a turn-in's
 * is big and keeps its place before the player's view as they turn, popping in with the mission banner's rewards. Drawn
 * as the damage numbers are (a screen-space widget component, on the world labels' layer, so menus and scenes hide it).
 * Hidden and not ticking while it waits to be used again.
 */
UCLASS(NotBlueprintable)
class AI_LOOTER_SHOOTER_API AXpPopupActor : public AActor
{
	GENERATED_BODY()

public:
	AXpPopupActor();

	/**
	 * Shows "+Amount XP" at Where after Delay seconds: small (a kill's) or big (a turn-in's). Restarts one showing. bTier: a
	 * Hallowed or Ethereal kill's, in TierColor, a size up, popping harder and staying a little longer (TierSeconds).
	 */
	void Show(int64 Amount, bool bBig, const FVector& Where, float Delay, bool bTier = false, const FLinearColor& TierColor = FLinearColor::White);

	/** Keeps it at ViewOffset from the player's camera (cm: ahead, right, up) as the view moves: the turn-in's. */
	void FollowView(const FVector& ViewOffset);

	/** Moves it on (the tick; a test level, which doesn't tick, calls it directly). */
	void Advance(float DeltaSeconds);

	virtual void Tick(float DeltaSeconds) override;

	/** It's in use (waiting out its delay, or on screen). */
	bool IsShowing() const { return bShowing; }
	bool IsBig() const { return bBig; }
	/** A top tier's kill's (its colour, a size up). */
	bool IsTier() const { return bTier; }
	const FString& GetWords() const { return Words; }
	/** Seconds since it was asked for, delay included. */
	float GetAge() const { return Age; }

	/** How long each shows once it appears (s). */
	static constexpr float SmallSeconds = 1.35f;
	static constexpr float BigSeconds = 2.4f;
	/** A top tier's kill's: a little longer than a kill's, so its colour is seen. */
	static constexpr float TierSeconds = 1.6f;
	/** A kill's sits this far right of its spot on the screen (px), beside the damage number. */
	static constexpr float SmallBeside = 38.f;

	/** Its look Shown seconds after it appears, as plain math: screen offset (px, y down), scale and opacity. */
	struct FLook
	{
		FVector2D Offset = FVector2D::ZeroVector;
		float Scale = 1.f;
		float Opacity = 1.f;
	};
	static FLook LookAt(float Shown, bool bBig, bool bTier = false);

	/** How long one shows once it appears: a turn-in's, a top tier's kill's, a kill's. */
	static float SecondsFor(bool bForBig, bool bForTier) { return bForBig ? BigSeconds : (bForTier ? TierSeconds : SmallSeconds); }

private:
	/** Back in the pool: hidden, not ticking. */
	void Hide();
	/** Where the camera puts it while it follows the view. */
	void PlaceBeforeView();

	UPROPERTY(VisibleAnywhere, Category = "Components")
	TObjectPtr<UWidgetComponent> Widget;

	FString Words;
	int64 Amount = 0;
	FVector ViewOffset = FVector::ZeroVector;
	float Age = 0.f;
	float Delay = 0.f;
	bool bBig = false;
	bool bTier = false;
	FLinearColor TierColor = FLinearColor::White;
	bool bShowing = false;
	bool bFollowView = false;
};
