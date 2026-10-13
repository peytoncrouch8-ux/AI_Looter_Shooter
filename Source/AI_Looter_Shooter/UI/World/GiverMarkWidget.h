#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Style/MissionMarkIcons.h"
#include "GiverMarkWidget.generated.h"

class UImage;

/**
 * A giver's mark floating over them (AGiverMarkActor's screen-space widget): the ! or ? in gold or white
 * (MissionMarkIcons), in a soft glow, bobbing gently. No panel behind it, like every label over the world. Its box is
 * fixed, so a mark that grows or shrinks with distance never lays the widget out again; the actor drives its look.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UGiverMarkWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The widget's box: room for the glyph at its biggest (the pop) and its glow. */
	static constexpr float BoxWidth = 76.f;
	static constexpr float BoxHeight = 88.f;
	/** The glyph's size at a scale of 1. */
	static constexpr float GlyphSize = 40.f;

	/** What it shows: a ! or a ?, gold or white. Rebuilds its brushes only when that changes. */
	void SetMark(MissionMarkIcons::EMark InMark, bool bInMain);

	/** How it looks this frame: Alpha (0-1), Scale of the glyph, Bob (pixels up), Glow (0-1, the halo's breath). */
	void SetLook(float Alpha, float Scale, float Bob, float Glow);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyBrushes();

	UPROPERTY(Transient) TObjectPtr<UImage> Halo;
	UPROPERTY(Transient) TObjectPtr<UImage> Glyph;

	MissionMarkIcons::EMark Mark = MissionMarkIcons::EMark::Offer;
	bool bMain = false;
	bool bBrushesSet = false;
};
