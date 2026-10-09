#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CreatureBarkWidget.generated.h"

class USizeBox;
class UTextBlock;

/**
 * A creature's bark in words over it (ACreatureBarkActor carries it): the LooterUI kit's floating text, outlined with no
 * plate behind it, wrapped to a few words a line, lifted clear of the creature's tag. A mutter to itself is smaller and
 * dimmer than a word said to the player. Builds its own widget tree, so no Widget Blueprint is needed.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UCreatureBarkWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The words, said aloud or muttered. Safe before the widget is built; they're applied once it is. */
	void SetLine(const FText& Line, bool bMuttered);

	const FText& GetLine() const { return Shown; }

	/** Type sizes (the mockups' px, as the HUD's text) and the widest line before it wraps (px). */
	static constexpr int32 SaidSize = 16;
	static constexpr int32 MutteredSize = 14;
	static constexpr float WrapWidth = 300.f;

	/** Room left under the words for the creature's tag (its name and bar float at the same spot, px). */
	static constexpr float TagClearance = 40.f;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void ApplyLine();

	UPROPERTY(Transient)
	TObjectPtr<UTextBlock> LineText;

	FText Shown;
	bool bShownMuttered = false;
};
