#pragma once

#include "CoreMinimal.h"
#include "Areas/StationBoard.h"
#include "Blueprint/UserWidget.h"
#include "StationBoardWidget.generated.h"

class ALooterHUD;
class UAreaDefinition;
class UImage;
class ULooterButton;
class UOverlay;
class USizeBox;
class UTextBlock;
class UVerticalBox;

/**
 * The station board (Docs/Areas/RansomsRest.md, "The exit and the unlock"), one LooterUI panel for every station and for
 * Skyreach's jetty, opened by holding Interact at it (ALooterHUD::OpenStationBoard). Over the dimmed world it lists the
 * board's lines as the story stands (StationBoard::BuildLines): every opened area (the one it stands in marked as here),
 * the blank line naming the mission that opens the next, and "Skyreach (practice)" after the first cast-off.
 *  - Choosing a line asks to confirm, then travels (UAreaTravelSubsystem::Depart): the first cast-off asks "Leave
 *    Skyreach? Your story begins. You can come back to practice any time." with Cast off and Not yet, opened by itself
 *    when it's the board's one trip; every later trip is a plain fade to the destination's station.
 *  - A destination whose level isn't in the game yet says so, and nobody goes.
 * W / S, the arrows or the D-pad choose; E, Enter or the face button take the line; Esc closes the confirm, then the
 * board. Every background it paints follows the UI transparency setting (MarkBackground).
 *
 * StationBoardWidget.cpp builds it and handles its lines and keys; StationBoardWidgetConfirm.cpp is the confirm and
 * leaving.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UStationBoardWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Opens on From's board (a jetty, a station; null: the level's own) with its words, the lines read afresh. */
	void Open(ALooterHUD* InHUD, AActor* InFrom, const FStationBoardWords& InWords);

	/** Closes it: the HUD hands the game its input back. */
	void Close();

	/** The lines shown, in order. */
	const TArray<FStationBoardLine>& GetLines() const { return Lines; }

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** A line's card, and the parts that restyle with the choice. */
	struct FLineCard
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
	};

	// StationBoardWidget.cpp: the panel, its lines and keys
	/** Builds the panel for the words and lines now (the tree is made once; the panel each time it opens). */
	void BuildPanel();
	UWidget* MakeLineCard(int32 Index);
	/** What a line's second row says: here, practice, not open yet, the story's start, the band of levels. */
	FText DescribeLine(const FStationBoardLine& Line) const;
	ULooterButton* MakeButton(FName Action, int32 Index, const FText& Label, int32 FontSize, bool bPrimary);
	void Select(int32 Index);
	/** Moves the choice by Direction to the next line that goes somewhere. */
	void Step(int32 Direction);
	void Restyle();
	/** Takes line Index: its confirm, or what keeps it from going. */
	void Choose(int32 Index);
	void SetStatus(const FText& Message, const FLinearColor& TextColor);
	void HandleButton(ULooterButton* Button);
	void HandleHovered(ULooterButton* Button);

	// StationBoardWidgetConfirm.cpp: the confirm and leaving
	/** Asks to confirm line Index. bAuto: it opened by itself (the first cast-off), so Not yet closes the board. */
	void OpenConfirm(int32 Index, bool bAuto);
	void CloseConfirm();
	/** Not yet: the confirm goes, and the board with it when it opened by itself. */
	void Decline();
	/** The trip goes: the board closes, and the line is taken (reopened with a word if nobody could go). */
	void ConfirmTrip();
	/** The confirm's buttons: the trip, or not yet. */
	void HandleConfirmButton(ULooterButton* Button);
	FText ConfirmTitle(const FStationBoardLine& Line) const;
	FText ConfirmBody(const FStationBoardLine& Line) const;

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<AActor> From;
	FStationBoardWords Words;
	TArray<FStationBoardLine> Lines;

	/** The areas the lines name, held while they're shown. */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UAreaDefinition>> Areas;

	/** The area the board stands in, by name ("Skyreach"); empty in a level that is no area's. */
	FText HereName;

	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> LinesBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	/** Over the panel: the confirm while it's open (built each time it opens). */
	UPROPERTY(Transient) TObjectPtr<UOverlay> PopupLayer;

	/** One per line, by index into Lines. */
	TArray<FLineCard> Cards;
	int32 Selected = INDEX_NONE;

	/** The line the confirm asks about, or INDEX_NONE while it's closed. */
	int32 ConfirmIndex = INDEX_NONE;
	bool bConfirmAuto = false;
};
