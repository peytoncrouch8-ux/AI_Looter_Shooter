#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/HudWardHit.h"
#include "HudScreenEdgeWidget.generated.h"

class UWidget;

/**
 * The screen's edges in red (Docs/Handoffs/CloudIslandConcepts_2026-10-05.md, "Hit" and "Low health"): they flash over
 * 0.55 s when the player is hurt, and pulse on the player frame's 0.9 s beat while health is low (30% or less). A hit the
 * ward soaked whole tints them cyan instead (lighter, quicker: Looter.Shield.Hud.Edge*), with no red; a hit that breaks the
 * ward flashes both (HudWardHit::Decide). Four soft strips a colour, each fading from its edge inward, fill the whole HUD
 * behind everything else, the red over the cyan. It watches the local player's pawn's health and ward itself, every
 * tick (FHudWardHitWatch), so nothing drives it; a new pawn (a respawn) starts without a flash. Collapsed, it
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
	/** The four red strips, and the four cyan ones under them; each set faded and shown or collapsed as one. */
	UPROPERTY(Transient) TObjectPtr<UWidget> Edges;
	UPROPERTY(Transient) TObjectPtr<UWidget> WardEdges;

	/** The pawn's health and ward from frame to frame. */
	FHudWardHitWatch Watch;
	/** Seconds left of a wound's red flash, and of a ward hit's cyan one. */
	float FlashTime = 0.f;
	float WardFlashTime = 0.f;
	/** The ward flash's length as it began (its cvar may change while it plays). */
	float WardFlashLength = 1.f;
	float ShownOpacity = -1.f;
	float ShownWardOpacity = -1.f;
};
