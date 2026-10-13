#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "MainMenuWidget.generated.h"

class AMainMenuHUD;
class UHorizontalBox;
class ULooterButton;
class UOverlay;
class UTextBlock;
class UVerticalBox;
struct FSessionSummary;

/**
 * The main menu, drawn over the tutorial island (a camera circles it behind the menu). A tall glass column on the left
 * holds the game's title and the menu; the rest of the screen is left to the island.
 *  - Single Player shows the session picker in place of the main buttons: a card per session (USessionSubsystem) naming
 *    the area it's in (Ransom's Rest, Skyreach), each continued or started from its card, and a saved one deleted after
 *    a confirmation popup. An empty slot's New Game starts the story at once: the cold open on Ransom's Rest, then the
 *    grave, with a Common Bullpup (USessionSubsystem::StartNewGame; the controls are taught there by the hints).
 *  - Multiplayer doesn't exist yet: it shows dimmed, with a SOON tag, and does nothing.
 *  - Settings opens the settings menu over this one (AMainMenuHUD owns both).
 *  - Quit Game closes the game.
 * Escape (or B) closes the popup, then goes back from the picker; on the main buttons it does nothing.
 *
 * A controller or the keyboard drive it all: the D-pad (left stick, arrows, W A S D) moves the focus over the buttons,
 * the session cards' and the popups' (round the ends), A or Enter presses. The focus look and the key hints at the
 * bottom follow the device last used (LooterUIFocus.h).
 *
 * MainMenuWidget.cpp builds the screen and handles its buttons and keys; MainMenuSessions.cpp makes the picker, its
 * session cards and the delete confirmation; MainMenuNav.cpp is the keyboard and controller focus and the key hints.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UMainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Back to the main buttons, with the picker and any popup closed. */
	void ShowMainButtons();

	// --- The keyboard and a controller (MainMenuNav.cpp) ---

	/**
	 * A key from the keyboard or a gamepad (every key comes here, and the tests send theirs): the D-pad, arrows or W A S D
	 * move the focus, A or Enter presses, B or Esc step back. The first move or press while the mouse drove the menu only
	 * brings the focus look up. bRepeat: the platform's own repeat, ignored (the menu repeats held keys itself). True
	 * when it was the menu's.
	 */
	bool HandleNavKey(const FKey& Key, bool bRepeat = false);
	void HandleNavKeyUp(const FKey& Key);
	/** The left stick's position (-1 to 1, up +Y), read as a D-pad. */
	void HandleNavStick(const FVector2D& Stick);
	/** A frame of held keys' repeats, of the device check and the focus look's breath (NativeTick; tests step it). */
	void AdvanceNav(float DeltaSeconds);
	UWidget* GetFocusedWidget() const;
	bool IsFocusLookShown() const;
	/** The key hints at the bottom as they stand, for the device last used. */
	TArray<MenuNav::FPrompt> GetPrompts() const;
	bool IsPickerShown() const { return bPickerShown; }
	bool IsPopupOpen() const { return DeleteIndex != INDEX_NONE; }
	/** A main button by its action ("SinglePlayer", "Multiplayer", "Settings", "Quit"), for the tests. */
	ULooterButton* GetMainButton(FName Action) const;
	ULooterButton* GetPickerBackButton() const;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FCursorReply NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent) override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
		const FNavigationReply& InDefaultReply) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

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
	/** The key hints at the bottom of the column, for what's on screen and the device last used. */
	void RefreshFooter();

	// --- The keyboard and a controller (MainMenuNav.cpp) ---

	/** The stops for what's on screen: the popup's, the picker's or the main buttons. Prefer: where the focus goes. */
	void RebuildNavigation(const UWidget* Prefer = nullptr);
	/** One step back: the popup, then the picker. False on the main buttons (nothing to go back to). */
	bool StepBack();
	void PressNav(const FKey& Key, MenuNav::EMove Move);
	void StepNav(MenuNav::EMove Move);
	/** Notes Key's device. False when the focus look wasn't showing before (that first press only brings it up). */
	bool RevealNav(const FKey& Key);
	void RefreshNavLook();
	void StopNavHold();
	void NotePointer();
	/** The mouse moved onto a button: with the mouse in charge the focus follows it, so keys carry on from there. */
	void HandleButtonHovered(ULooterButton* Button);
	/** The popup's stops, as it opens; the focus goes back to what opened it when it closes. */
	void OpenPopupNav(TArray<FMenuNavRow> Rows, const UWidget* Prefer);
	void ClosePopupNav();

	// --- The session picker and the delete confirmation (MainMenuSessions.cpp) ---

	UWidget* MakePicker();
	/** Reads every session slot again and rebuilds the cards. */
	void RefreshSessions();
	UWidget* MakeSessionCard(int32 Index, const FSessionSummary& Summary);
	void HandleSessionButton(ULooterButton* Button);
	/** Continues session Index, or starts a new game in it (the story from its start), which opens its level. */
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
	/** The key hints at the bottom of the column ("A Select  B Back"). */
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> FooterPrompts;
	/** Over everything: the delete confirmation while it's open (built each time it opens). */
	UPROPERTY(Transient) TObjectPtr<UOverlay> PopupLayer;

	/** The main buttons top to bottom (Multiplayer among them: the focus passes over it while it's disabled). */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> MainNavButtons;
	/** Each session card's Continue / New Game, and its Delete (none for an empty slot), by slot. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> PlayButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> DeleteButtons;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> PickerBackButton;

	FMenuNavigator Nav;
	MenuNav::FRepeat NavRepeat;
	FVector2D NavStick = FVector2D::ZeroVector;
	MenuNav::EMove NavStickMove = MenuNav::EMove::None;
	/** The open popup's stops (none while it's closed), and the stop that opened it. */
	TArray<FMenuNavRow> PopupNav;
	TWeakObjectPtr<UWidget> PopupReturn;
	LooterUI::EMenuDevice ShownDevice = LooterUI::EMenuDevice::Mouse;
	bool bNavLookApplied = false;
	/** The key hints on show, as one line: rebuilt only when they change. */
	FString ShownPrompts;

	bool bPickerShown = false;
	/** The session the delete confirmation asks about, or INDEX_NONE while it's closed. */
	int32 DeleteIndex = INDEX_NONE;
	/** A session's level is opening: the menu goes with this world, so it takes no more clicks or keys. */
	bool bLoading = false;
};
