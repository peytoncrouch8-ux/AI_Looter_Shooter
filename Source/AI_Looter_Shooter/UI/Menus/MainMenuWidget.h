#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "MainMenuWidget.generated.h"

class AMainMenuHUD;
class ULooterButton;
class UOverlay;
class UTextBlock;
class UVerticalBox;
struct FSessionSummary;

/**
 * The main menu, drawn over the tutorial island (a camera circles it behind the menu). A tall glass column on the left
 * holds the game's title and the menu; the rest of the screen is left to the island.
 *  - Single Player shows the session picker in place of the main buttons: a card per session (USessionSubsystem) naming
 *    the area it's in (Skyreach, Ransom's Rest), each continued or started from its card, and a saved one deleted after
 *    a confirmation popup.
 *  - Multiplayer doesn't exist yet: it shows dimmed, with a SOON tag, and does nothing.
 *  - Settings opens the settings menu over this one (AMainMenuHUD owns both).
 *  - Quit Game closes the game.
 * Escape closes the popup, then goes back from the picker; on the main buttons it does nothing.
 *
 * MainMenuWidget.cpp builds the screen and handles its buttons and keys; MainMenuSessions.cpp makes the picker, its
 * session cards and the delete confirmation.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Back to the main buttons, with the picker and any popup closed. */
	void ShowMainButtons();

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	ULooterButton* MakeButton(FName Action, int32 Index, const FString& Label, int32 FontSize, LooterUI::EButtonKind Kind);
	/** The glass column: the title, the menu and the key hint at the bottom. */
	UWidget* MakeColumn();
	UWidget* MakeTitle();
	UWidget* MakeMainButtons();
	AMainMenuHUD* GetMenuHUD() const;

	/** Every button's click: the main buttons here, the picker's and the popup's in HandleSessionButton. */
	void HandleButton(ULooterButton* Button);
	void ShowPicker();
	/** The picker's status line: a session loading or deleted, or what went wrong. */
	void SetStatus(const FString& Message, const FLinearColor& TextColor);
	/** The key hint at the bottom of the column, for what's on screen. */
	void RefreshFooter();

	// --- The session picker and the delete confirmation (MainMenuSessions.cpp) ---

	UWidget* MakePicker();
	/** Reads every session slot again and rebuilds the cards. */
	void RefreshSessions();
	UWidget* MakeSessionCard(int32 Index, const FSessionSummary& Summary);
	void HandleSessionButton(ULooterButton* Button);
	/** Continues session Index, or starts a new game in it, which opens its level. */
	void StartSession(int32 Index);
	void OpenDeleteConfirm(int32 Index);
	void CloseDeleteConfirm();
	void ConfirmDelete();

	/** The main buttons, and the session picker shown in their place. */
	UPROPERTY(Transient) TObjectPtr<UWidget> MainButtons;
	UPROPERTY(Transient) TObjectPtr<UWidget> Picker;
	/** The picker's cards, rebuilt each time it shows and after a delete. */
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SessionList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FooterText;
	/** Over everything: the delete confirmation while it's open (built each time it opens). */
	UPROPERTY(Transient) TObjectPtr<UOverlay> PopupLayer;

	bool bPickerShown = false;
	/** The session the delete confirmation asks about, or INDEX_NONE while it's closed. */
	int32 DeleteIndex = INDEX_NONE;
	/** A session's level is opening: the menu goes with this world, so it takes no more clicks or keys. */
	bool bLoading = false;
};
