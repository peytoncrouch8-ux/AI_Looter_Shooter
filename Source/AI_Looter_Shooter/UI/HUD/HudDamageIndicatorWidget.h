#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudDamageIndicatorWidget.generated.h"

class AActor;
class AController;
class APawn;
class UCanvasPanel;
class UHealthComponent;
class UImage;

/**
 * Where a hit on the player came from: a red arc on a ring round the crosshair, its point toward whatever hurt them,
 * which keeps pointing at it (wherever it goes) as the player turns, and fades over 1.6 s. Ahead is the ring's top,
 * behind its bottom. Up to four at once; a new hit from the same attacker brightens its arc again rather than adding one.
 * A harder hit (a bigger share of the bar) draws it bolder. The HUD's style: floating metal-and-ink shapes, no panel
 * (it's feedback, not a background, so the UI transparency setting leaves it alone). It watches the local player's
 * pawn's health itself, so nothing drives it; collapsed while there's nothing to show.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudDamageIndicatorWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * The arc's angle on the ring for a hit from Source, seen from ViewLocation looking along ViewYaw (degrees): 0 ahead
	 * (the top), 90 to the right, -90 to the left, 180 behind. Only the bearing counts, not the height.
	 */
	static float ScreenAngle(const FVector& ViewLocation, float ViewYaw, const FVector& Source);

	/** How far from the crosshair the arc's middle sits (px), and how long one shows. */
	static constexpr float RingRadius = 170.f;
	static constexpr float ShowSeconds = 1.6f;
	static constexpr int32 MaxArcs = 4;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	UFUNCTION()
	void HandleDamaged(float Damage, bool bCritical, FVector HitLocation, AController* InstigatedBy, AActor* DamageCauser);

	/** Follows the local player's pawn (a respawn gives a new one) and its health. */
	void WatchPawn();

	struct FArc
	{
		/** What hurt the player, followed while it lives; else where it was. */
		TWeakObjectPtr<AActor> Source;
		FVector Where = FVector::ZeroVector;
		float Age = ShowSeconds;
		/** 0-1: how hard the hit was (its share of the bar). */
		float Strength = 0.f;
	};

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Canvas;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> ArcImages;

	FArc Arcs[MaxArcs];
	TWeakObjectPtr<APawn> WatchedPawn;
	TWeakObjectPtr<UHealthComponent> WatchedHealth;
	bool bShowing = false;
};
