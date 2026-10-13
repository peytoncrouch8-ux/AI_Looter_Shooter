#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "World/RoostVeil.h"
#include "HudCrowVeilWidget.generated.h"

/**
 * Hob's crows over the screen (URoostCarrySubsystem shows it; FRoostVeil says where every crow is): a fast travel's carry
 * and a death's drag back to the nearest roost. Crow silhouettes in the kit's ink, painted in code in three wing poses
 * (LooterUI::PaintedIconBrush, no texture assets) and turned along their flight, over the dark the veil lays under them as
 * the screen goes black. The crows lighten a little as the dark deepens, so their wings still read against it. It's the
 * screen's own transition (the camera's fade is under it), not a panel, so the UI transparency setting leaves it be.
 * Collapsed between carries, when it costs nothing.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudCrowVeilWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Over the gameplay HUD (0), with Grave Sight (1: they never meet), under the captions (2) so Hob's word reads, and the menus. */
	static constexpr int32 ViewportZOrder = 1;

	/** Shows the veil as it is now (every frame of a carry); an idle one collapses it. */
	void ShowVeil(const FRoostVeil& InVeil);

	const FRoostVeil& GetVeil() const { return Veil; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

private:
	FRoostVeil Veil;
	/** The dark's plain brush and the crows' three wing poses (made once). */
	UPROPERTY(Transient)
	FSlateBrush ShadeBrush;
	UPROPERTY(Transient)
	FSlateBrush CrowBrushes[3];
	bool bBrushesMade = false;
	/** Reused each paint. */
	mutable TArray<FRoostVeilCrow> Crows;
};
