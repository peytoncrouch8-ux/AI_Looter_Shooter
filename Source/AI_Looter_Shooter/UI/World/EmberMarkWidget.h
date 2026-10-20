#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "EmberMarkWidget.generated.h"

class UImage;
class UTextBlock;

/**
 * An ember power's mark floating over a creature (AEmberMarkActor's screen-space widget, so it shows through walls and goes
 * with the other world labels under a menu or a scene): a glyph in the power's colour in a soft glow of it, and a word under
 * it in white floating text. Spyglass and Long Shot mark with the hostile diamond (the minimap's language) and the creature's
 * distance ("34 m"); Last Rites with its own Inked icon and the seconds the mark has left. No panel behind it, like every
 * label over the world. Its box is fixed, so a mark that grows or shrinks never lays the widget out again; the actor drives
 * its look.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UEmberMarkWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The widget's box (room for the glyph at its biggest, its glow and the word under it). */
	static constexpr float BoxWidth = 104.f;
	static constexpr float BoxHeight = 100.f;
	/** The diamond's size, and an Inked icon's box, at a scale of 1. */
	static constexpr float DiamondSize = 24.f;
	static constexpr float IconSize = 34.f;
	/** The glow behind the glyph. */
	static constexpr float HaloSize = 76.f;
	/** The word's size (floating text, px as Slate font size). */
	static constexpr int32 WordSize = 14;

	/** Its glyph: the hostile diamond in Color (IconName None), or the Inked icon named so (its own colours); the glow in Color. */
	void SetGlyph(FName IconName, const FLinearColor& Color);

	/** The word under the glyph ("34 m", "6"); empty for none. Set only when it changes. */
	void SetWords(const FString& InWords);
	FString GetWords() const { return Shown; }

	/** How it looks this frame: Alpha (0-1), Scale of the glyph, Glow (0-1, the halo's breath). */
	void SetLook(float Alpha, float Scale, float Glow);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyBrushes();

	UPROPERTY(Transient) TObjectPtr<UImage> Halo;
	UPROPERTY(Transient) TObjectPtr<UImage> Glyph;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Words;

	FName Icon;
	FLinearColor Tint = FLinearColor::White;
	bool bBrushesSet = false;
	FString Shown;
};
