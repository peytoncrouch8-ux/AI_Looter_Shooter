#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudCarriedLanternWidget.generated.h"

class UCanvasPanel;
class UImage;
class USizeBox;
class UTextBlock;

/**
 * Abel's light while Sully carries him home (Main 7 step 4 on; UCarriedLanternSubsystem adds and drives it), in the gameplay
 * HUD's style (no backing panel): a keeper's lantern drawn in the kit's brushes, its glass glowing with what's left of his
 * light, "ABEL'S LIGHT" in floating outlined text, and a slim slanted bar in soul gold over the dark track (the track is
 * the only background, faded by the UI transparency setting). Bottom middle, clear of the player frame (bottom left), the
 * weapon column (bottom right) and the tracker (top left). While a hose-man's line drains it the gold dims toward the
 * track's grey and pulses, and a cyan "DRAINING" takes the label's place; filling again the bar's edge glints. It fades in
 * and out, and steps aside behind menus and in scenes (the subsystem asks the scene's word).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudCarriedLanternWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Over the gameplay HUD, under the boss bar (3), the inventory and the pause menu. */
	static constexpr int32 ViewportZOrder = 2;

	/** The bar's length on screen. */
	static constexpr float BarWidth = 220.f;

	/** Fades it in or out (cheap every frame). */
	void SetShown(bool bInShown);
	bool IsShown() const { return bWanted || Opacity > 0.f; }

	/** His light (0-1), whether a line drains it now, and the pulse's clock (cheap every frame). */
	void SetLight(float InLight, bool bInDraining, float Clock);

	float GetShownLight() const { return Shown; }
	bool IsDrainingShown() const { return bDraining; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	void Apply(float Clock);
	bool IsMenuOpen() const;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Cluster;
	UPROPERTY(Transient) TObjectPtr<UImage> LanternGlass;
	UPROPERTY(Transient) TObjectPtr<UImage> GlassGlow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Label;
	UPROPERTY(Transient) TObjectPtr<USizeBox> FillBox;
	UPROPERTY(Transient) TObjectPtr<UImage> Fill;
	UPROPERTY(Transient) TObjectPtr<UImage> FillEdge;

	bool bWanted = false;
	float Opacity = 0.f;
	float Light = 1.f;
	/** The light the bar shows, easing after Light (so a drain reads as a slide, not a jump). */
	float Shown = 1.f;
	bool bDraining = false;
	bool bLabelDraining = false;
	float PulseClock = 0.f;
	/** A glint on the edge as it fills (0-1, fading). */
	float Glint = 0.f;
	float LastShownWidth = -1.f;
};
