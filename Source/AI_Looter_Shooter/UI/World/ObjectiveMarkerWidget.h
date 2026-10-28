#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ObjectiveMarkerWidget.generated.h"

class UImage;

/**
 * The tracked objective's marker over a paper (AObjectiveMarkerActor's screen-space widget): a ring-diamond round a small
 * diamond in the tracker's orange (LooterUI::Color::Accent, the highlight's), edged dark so it reads on a bright wall, in a
 * soft glow of the same orange. Hollow, so it never reads as the minimap's solid hostile diamond. No panel behind it, like
 * every label over the world. Its box is fixed, so the pop never lays the widget out again; the actor drives its look.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UObjectiveMarkerWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The widget's box: room for the mark at its pop's biggest and its glow. */
	static constexpr float BoxSize = 64.f;
	/** The mark's size at a scale of 1. */
	static constexpr float MarkSize = 26.f;
	/** The glow behind it. */
	static constexpr float HaloSize = 58.f;

	/** How it looks this frame: Alpha (0-1), Scale of the mark (the pop), Glow (0-1, the halo's breath). */
	void SetLook(float Alpha, float Scale, float Glow);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	UPROPERTY(Transient) TObjectPtr<UImage> Halo;
	UPROPERTY(Transient) TObjectPtr<UImage> Mark;
};
