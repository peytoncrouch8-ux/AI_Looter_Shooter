#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudScreenEdgeWidget.generated.h"

class APawn;
class UHealthComponent;
class UWidget;

/**
 * The screen's edges in red (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Hit" and "Low health"): they flash over
 * 0.55 s when the player is hit, and pulse on the player frame's 0.9 s beat while health is low (30% or less). Four soft
 * strips, each fading from its edge inward, fill the whole HUD behind everything else. It watches the local player's
 * pawn's health itself, every tick, so nothing drives it; a new pawn (a respawn) starts without a flash. Collapsed, it
 * costs nothing, and that's how it rests. It is feedback, not a background: the UI transparency setting leaves it alone.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudScreenEdgeWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	/** The four strips; faded and shown or collapsed as one. */
	UPROPERTY(Transient) TObjectPtr<UWidget> Edges;

	/** The pawn watched and its health, found again when the pawn changes. */
	TWeakObjectPtr<APawn> WatchedPawn;
	TWeakObjectPtr<UHealthComponent> WatchedHealth;
	/** Its health last tick (-1 before the first look at this pawn). */
	float LastHealth = -1.f;
	/** Seconds left of a hit's flash. */
	float FlashTime = 0.f;
	float ShownOpacity = -1.f;
};
