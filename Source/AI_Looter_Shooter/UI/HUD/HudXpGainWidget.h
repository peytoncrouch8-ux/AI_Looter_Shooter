#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HudXpGainWidget.generated.h"

class UTextBlock;

/**
 * The "+38 XP" by the experience bar (the user's call, 2026-10-14: experience shows both at the kill and by the bar), after
 * the bar's numbers in the player frame: floating outlined text in the experience's cyan, no panel. It pops in white and
 * settles to cyan; kills close together add up into one number that pops again with each; it holds while the bar is still
 * filling, then fades as it rises a little. The kill's own "+XP" floats at the kill (UXpPopupSubsystem); this one stays
 * where the bar is. UHudPlayerFrameWidget feeds it (AddGain on a gain, Advance every frame). It keeps its place in the line
 * while hidden, so nothing beside it moves when it shows.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UHudXpGainWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Experience gained: shown, or added to the number still showing. Nothing for none. */
	void AddGain(int64 Amount);

	/** Moves it on by DeltaSeconds; bBarFilling holds it at full strength until the bar has caught up. */
	void Advance(float DeltaSeconds, bool bBarFilling);

	/** How long it pops, holds at least, and fades. */
	static constexpr float PopSeconds = 0.3f;
	static constexpr float HoldSeconds = 1.6f;
	static constexpr float FadeSeconds = 0.6f;

	/** Its opacity and rise (pixels up) Age seconds into its show, and its scale Age seconds into a pop (each gain pops). */
	static float OpacityAt(float Age);
	static float ScaleAt(float Age);
	static float RiseAt(float Age);

	/** Its scale now, for the tests. */
	float GetShownScale() const { return ScaleAt(PopAge); }

	/** It's on screen. */
	bool IsShowing() const { return ShowAge < HoldSeconds + FadeSeconds; }

	/** What it says ("+78 XP"), and the total it shows. */
	FString GetShownText() const;
	int64 GetShownTotal() const { return Total; }

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	/** Its look at ShowAge and PopAge. */
	void Apply();

	UPROPERTY(Transient) TObjectPtr<UTextBlock> AmountText;

	int64 Total = 0;
	/** Seconds into its show (at or past HoldSeconds + FadeSeconds: gone), and into its latest pop (one per gain). */
	float ShowAge = 1000.f;
	float PopAge = 1000.f;
};
