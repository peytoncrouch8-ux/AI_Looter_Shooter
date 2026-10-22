#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "HudScopeWidget.generated.h"

/**
 * The sniper's scope picture (Phase 12.3; the Longhorn shows it while its scope is up, AWeaponBase::UpdateScope): the
 * world through a round lens, dark ink outside it and a thin metal rim, a fine cyan cross with heavier posts toward the rim
 * and an orange dot in the middle; under it, while the breath is being held or coming back, a slim slanted bar of the
 * breath left (orange, red when winded). Painted in code (the kit's colors, a PaintedIconBrush mask), no textures. The gun
 * and the hands are hidden meanwhile, so nothing of the 22 cm-away eyepiece crowds the view. It's the eye's own view, not
 * a panel: the UI transparency setting leaves it be. Collapsed when the scope is down.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudScopeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Under the gameplay HUD (0: its health, ammo and map stay on top of the lens's dark), over the world. */
	static constexpr int32 ViewportZOrder = -1;

	/**
	 * Shows the scope at Strength (0 hidden, 1 full: it fades in over the last of the scope's rise), with the breath left
	 * (0-1), whether it's being held, and whether the shooter is winded.
	 */
	void ShowScope(float InStrength, float InBreathLeft, bool bInHolding, bool bInWinded);

	float GetStrength() const { return Strength; }

	/** The lens's radius as a share of the screen's height. */
	static constexpr float LensShare = 0.46f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	float Strength = 0.f;
	float BreathLeft = 1.f;
	bool bHolding = false;
	bool bWinded = false;

	/** The lens mask (ink with a round hole and its rim) and the plain fill for the sides and the bar. */
	UPROPERTY(Transient)
	FSlateBrush MaskBrush;
	UPROPERTY(Transient)
	FSlateBrush FillBrush;
	bool bBrushesMade = false;
};
