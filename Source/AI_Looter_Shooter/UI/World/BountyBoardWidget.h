#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Bounties/BountyRules.h"
#include "BountyBoardWidget.generated.h"

class ABountyBoard;
class APlayerController;
class UBountyBoardSubsystem;
class UHorizontalBox;
class ULooterButton;
class USizeBox;
class UTextBlock;

/**
 * A bounty board's screen (ABountyBoard; Docs/Work/2026-10-22-Next/BOUNTY.md), one LooterUI panel (Concept C) over the
 * dimmed world, opened by a tap of Interact at the board: the board's line and when new postings go up, then a card per
 * slot side by side, each a posting (its tier's diamonds and word, NEW / TAKEN / CLAIM IT, the mark's name, its rank and
 * kind and levels, its traits, a pack twist, a flavour line, where it was last seen, what it pays) or an empty slot (when
 * the next goes up). One card is chosen (orange line): A/D, the arrows, the d-pad or the mouse choose; E (Enter, the
 * face button, a click on the chosen card) takes a posted one or collects a won one; R (the left face button) twice
 * gives the taken one up; Esc closes. A status line under the cards says what just happened or why not.
 *
 * Under the panel's tab sits the board's own plate (gunmetal, the ranger's star, BOUNTIES in display type, the three tiers' key,
 * an orange rule). It opens and closes itself (the board asks it to: OpenFor), holds the keyboard and shows the mouse while it's up, and
 * closes on its own if the player walks off or dies. Its cards follow the board's subsystem (OnChanged, and its clock
 * once a second). Every background it paints follows the UI transparency setting (MarkBackground).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBountyBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Opens Board's screen for Player (closing any board's screen up already). Null when it can't (no local player). */
	static UBountyBoardWidget* OpenFor(APlayerController* Player, ABountyBoard* Board);

	/** The board's screen up now, if any. */
	static UBountyBoardWidget* GetOpen();

	/** Closes it: the game has its keys and its mouse back. */
	void Close();

	ABountyBoard* GetBoard() const;

	/** The chosen card (its slot), and choosing one (wrapped round the slots). */
	int32 GetSelected() const { return Selected; }
	void Select(int32 CardIndex);

	/** E on the chosen card: takes it (posted) or collects it (won). Whether something happened. */
	bool ActOnSelected();

	/** R: the first press asks, the second gives the taken bounty up. Whether it was given up. */
	bool GiveUpSelected();

	/** The status line's words now, for the tests. */
	FText GetStatus() const { return Status; }

	/** The player walks this far (cm) from the board and the screen closes. */
	static constexpr float StayRadius = 450.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	// --- The panel (BountyBoardWidget.cpp) ---
	void BuildPanel();
	void MakeButtons();
	void HandleButton(ULooterButton* Button);
	void HandleChanged();
	bool ShouldClose() const;
	UBountyBoardSubsystem* GetBounties() const;
	void SetStatus(const FText& Words, bool bBad);

	// --- The cards (BountyBoardWidgetCards.cpp) ---
	/** The cards from the subsystem's view, made again only when their words or the choice changed. */
	void ShowCards(bool bForce);
	/** The BOUNTIES plate over the cards: medallion, title, tier legend (BountyBoardWidgetHeader.cpp). */
	UWidget* MakeHeader();
	UWidget* MakeCard(int32 CardIndex, const FBountyCardView& Card, bool bChosen);
	UWidget* MakeCardHeader(const FBountyCardView& Card);
	/** A card's words in a line, to tell when it changed. */
	static FString Signature(const FBountyCardView& Card);

	TWeakObjectPtr<ABountyBoard> Board;
	TWeakObjectPtr<APlayerController> Player;

	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> CardRow;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> RotationText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PaidText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> ButtonRow;

	TArray<FBountyCardView> Shown;
	FString ShownSignature;
	int32 ShownSelected = INDEX_NONE;
	int32 Selected = 0;
	bool bGiveUpArmed = false;
	FText Status;
	bool bStatusBad = false;
	float SinceClock = 0.f;
	FDelegateHandle ChangedHandle;
};
