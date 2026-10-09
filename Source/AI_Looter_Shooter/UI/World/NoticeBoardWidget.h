#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Missions/MissionRewards.h"
#include "World/NoticeBoard.h"
#include "NoticeBoardWidget.generated.h"

class ALooterHUD;
class UImage;
class ULooterButton;
class UMissionDefinition;
class UMissionRunner;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;
struct FMissionRewards;

/**
 * A notice board's screen (ANoticeBoard; Skyreach's in the town square), one LooterUI panel over the dimmed world, opened
 * by reading the board (ALooterHUD::OpenNoticeBoard). It lists the board's postings as they stand
 * (ANoticeBoard::GatherPostings), each a card pinned up: its title, its notice in the town's words, its count while open,
 * its reward, and what can be done with it here:
 *  - one ready to turn in shows "[E] Turn in": turned in, its card says what it gave (the gun lands at the player's feet,
 *    in front of the board) in the reward's rarity colour, and a stamp and the mission's fanfare are heard;
 *  - one open and not tracked shows "[E] Track": the minimap and the HUD's tracker follow it;
 *  - one done is stamped done; one not up yet is faint, with the board's note (Skyreach's: bring a gun first).
 * W / S, the arrows or the D-pad choose among the cards that can be acted on; E, Enter or the face button act; a click on a
 * card acts on it; Esc closes. Every background it paints follows the UI transparency setting (MarkBackground).
 *
 * NoticeBoardWidget.cpp builds it and handles its keys; NoticeBoardWidgetCards.cpp makes the posting cards and acts on them.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UNoticeBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Opens on Board, its postings read afresh. */
	void Open(ALooterHUD* InHUD, ANoticeBoard* InBoard);

	/** Closes it: the HUD hands the game its input back. */
	void Close();

	/** The postings shown, in order. */
	const TArray<FNoticeBoardPosting>& GetPostings() const { return Postings; }

	/** What a posting's reward is, in a few words for its card ("Reward: Pump Shotgun", "No reward: it's practice"). */
	static FText DescribeReward(const FMissionRewards& Rewards);

	/** What a turn-in gave, for its card ("Rare Pump Shotgun, at your feet"); empty when it gave nothing. */
	static FText DescribeGiven(const FMissionRewards& Rewards, const FMissionRewardsGiven& Given);

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** A card's button (none for one that can't be acted on) and the parts that restyle as it's chosen. */
	struct FCard
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
	};

	// --- NoticeBoardWidget.cpp: the panel, its keys ---

	/** Reads the postings again and builds the panel for them, keeping the chosen card where it can. */
	void Refresh();
	void BuildPanel();
	void Select(int32 Index);
	/** Moves the choice by Direction to the next card that can be acted on. */
	void Step(int32 Direction);
	void Restyle();
	void SetStatus(const FText& Message, const FLinearColor& TextColor);
	void HandleButton(ULooterButton* Button);
	void HandleHovered(ULooterButton* Button);
	UMissionRunner* GetRunner() const;

	// --- NoticeBoardWidgetCards.cpp: the cards and acting on them ---

	UWidget* MakeCard(int32 Index);
	/** The card's state on its right: its word and colour ("Open  3 / 6", "Ready to turn in", "Done"). */
	FText StateWord(const FNoticeBoardPosting& Posting) const;
	FLinearColor StateColor(const FNoticeBoardPosting& Posting) const;
	/** Card Index can be acted on here: turned in (ready, given here) or tracked (open, not tracked yet). */
	bool CanAct(int32 Index) const;
	/** Turns in or tracks card Index. */
	void Act(int32 Index);
	/** The runner finished a mission: a turn-in here keeps what it gave, for its card. */
	void HandleMissionCompleted(const UMissionDefinition& Mission, const FMissionRewardsGiven& Given);

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<ANoticeBoard> Board;
	TWeakObjectPtr<UMissionRunner> BoundRunner;
	FDelegateHandle CompletedHandle;

	TArray<FNoticeBoardPosting> Postings;
	/** What each posting turned in while the screen was open gave, by mission id. */
	TMap<FName, FMissionRewardsGiven> GivenHere;

	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> CardsBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;

	TArray<FCard> Cards;
	int32 Selected = INDEX_NONE;
};
