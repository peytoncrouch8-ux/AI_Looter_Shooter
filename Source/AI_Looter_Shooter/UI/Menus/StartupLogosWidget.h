#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menus/StartupLogosRules.h"
#include "StartupLogosWidget.generated.h"

class UImage;
class UTextBlock;
class UWorld;

DECLARE_DELEGATE(FOnStartupLogosEvent);

/**
 * The startup cards before the main menu (the user's note of 2026-10-27: creator and software logos on opening, as
 * Borderlands does): on a plain dark ground, the studio's card (a placeholder name from UStartupLogosSettings under an
 * original mark, the user renames it later), then "Powered by Unreal Engine" (Epic's official logo once it's imported at
 * /Game/UI/Logos/T_UnrealEngineLogo, a plain wordmark until then), each with its sting (UI.Startup.Studio,
 * UI.Startup.Engine). Then the dark fades off the menu, which arrives under it (OnRevealing), and the cards go
 * (OnFinished). FStartupLogos keeps the clock.
 *
 * Any key or click skips the card on screen, one card a press, except on the first launch, which shows every card in
 * full (Looter.Startup.FirstLaunchSkips 1 lets that one be skipped too). They play once a run, as the game starts at
 * the menu (AMainMenuHUD): never again on a return to the menu, never in tests, nor with -NoLogos, nor in the editor's
 * Play unless Looter.Startup.InEditor 1; Looter.Startup.Replay plays them again on the menu.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UStartupLogosWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The cards are due in World (its menu just opened): on, not played yet this run, not a test or -NoLogos, not PIE. */
	static bool ShouldPlay(const UWorld* World);
	/** No run of this game has seen the cards to their end yet (kept in the user's GameUserSettings.ini). */
	static bool IsFirstLaunch();
	/** Notes in the user's settings that the cards were seen to their end: from now on a key skips them. */
	static void MarkSeen();

	/** Starts the cards from the first (skippable unless this is the first launch, or bForceSkippable). */
	void Begin(bool bForceSkippable = false);

	/** A frame of the cards (NativeTick; tests step it). */
	void Advance(float DeltaSeconds);

	/** A key: skips a card if the cards allow it. Every key is the cards' while they show. */
	bool HandleKey(const FKey& Key, bool bRepeat = false);

	const FStartupLogos& GetRules() const { return Rules; }
	int32 NumCards() const { return Cards.Num(); }
	UTextBlock* GetStudioNameText() const { return StudioNameText; }
	/** The engine card shows Epic's imported logo (false: the plain wordmark, or no engine card). */
	bool IsShowingEngineLogo() const { return bEngineLogoShown; }

	/** The cards are done and the dark starts to fade off the menu: give the menu the keys and let it arrive. */
	FOnStartupLogosEvent OnRevealing;
	/** The dark is gone: take this widget away. */
	FOnStartupLogosEvent OnFinished;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FCursorReply NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent) override;

private:
	UWidget* MakeStudioCard();
	UWidget* MakeEngineCard();
	/** The cards, the cover and the hint as the clock has them now. */
	void Apply();

	/** The dark ground over the menu while the cards show. */
	UPROPERTY(Transient) TObjectPtr<UImage> Cover;
	/** The cards in the order they show. */
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> Cards;
	/** Which card is the studio's (it drifts in a little; Epic's logo is never moved or scaled), or INDEX_NONE. */
	int32 StudioCard = INDEX_NONE;
	/** Each card's sting, by card. */
	TArray<FName> CardCues;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StudioNameText;
	/** "Press any key to skip", bottom right, while a skippable card shows. */
	UPROPERTY(Transient) TObjectPtr<UWidget> SkipHint;

	FStartupLogos Rules;
	bool bBegun = false;
	bool bRevealSent = false;
	bool bFinishedSent = false;
	bool bEngineLogoShown = false;
};
