#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "MainMenuWidget.generated.h"

class AMainMenuHUD;
class UHeroSelectWidget;
class UHorizontalBox;
class ULooterButton;
class UOverlay;
class UTextBlock;
class UVerticalBox;
struct FSessionSummary;

/**
 * The main menu, drawn over the menu's backdrop (Teropa from orbit; ALooterMenuPlayerController frames it in the right
 * 60% of a 16:9 screen). The menu keeps to the left 40%: a dark shade thinning out to the right (no box), the game's
 * title lockup (its mark, REVENANT in the kit's display type, TEROPA under an orange rule), the main buttons as words
 * with a lit tick and wash (no boxes), and the key hints at the bottom. It arrives in motion (BeginReveal: the shade, the
 * title, the buttons one after another, UI.Menu.Reveal); a lit button eases its tick, wash and words in; the mark's ember
 * breathes; a page (the buttons, the picker) fades in when it changes (MainMenuMotion.cpp).
 *  - Single Player shows the session picker in place of the main buttons: a card per session (USessionSubsystem) naming
 *    its hero (their portrait and name) and the area it's in (Ransom's Rest, Skyreach), each continued or started from
 *    its card, and a saved one deleted after a confirmation popup. An empty slot's New Game opens the hero select
 *    (UHeroSelectWidget, over the whole screen, this column hidden): confirming a hero starts the story with them (the
 *    cold open on Ransom's Rest, then the grave, with a Common Bullpup: USessionSubsystem::StartNewGame; the controls
 *    are taught there by the hints); Back returns to the picker.
 *  - Multiplayer doesn't exist yet: it shows dimmed, with a SOON tag, and does nothing.
 *  - Settings opens the settings menu over this one (AMainMenuHUD owns both).
 *  - Quit Game closes the game.
 * Escape (or B) closes the popup, then goes back from the hero select and the picker; on the main buttons it does nothing.
 *
 * A controller or the keyboard drive it all: the D-pad (left stick, arrows, W A S D) moves the focus over the buttons,
 * the session cards' and the popups' (round the ends), A or Enter presses. The focus look and the key hints at the
 * bottom follow the device last used (LooterUIFocus.h).
 *
 * MainMenuWidget.cpp builds the screen and handles its buttons and keys; MainMenuMotion.cpp its motion; MainMenuArt the
 * pictures; MainMenuSessions.cpp makes the picker, its session cards, the delete confirmation and the way into and out
 * of the hero select; MainMenuNav.cpp is the keyboard and controller focus and the key hints (handed to the hero select
 * while it shows).
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

	// --- The hero select (MainMenuSessions.cpp) ---

	/** Opens the hero select for session Index's new game, over the whole screen (the column hidden under it). */
	void OpenHeroSelect(int32 Index);
	/** Back to the session picker, the focus on that session's New Game. */
	void CloseHeroSelect();
	bool IsHeroSelectShown() const { return bHeroSelectShown; }
	UHeroSelectWidget* GetHeroSelect() const { return HeroSelect; }
	/** A main button by its action ("SinglePlayer", "Multiplayer", "Settings", "Quit"), for the tests. */
	ULooterButton* GetMainButton(FName Action) const;
	ULooterButton* GetPickerBackButton() const;

	// --- The look and its motion (MainMenuMotion.cpp) ---

	/** The menu's side of a 16:9 screen (40% of its 1920 units, the user's split with the planet, 2026-10-27). */
	static constexpr float ColumnWidth = 768.f;

	/** Keeps the menu unseen (the startup cards are over it) until BeginReveal. */
	void HoldReveal();
	/** The menu arrives: the shade, the title and the buttons fade and slide in one after another, with UI.Menu.Reveal. */
	void BeginReveal();
	/** The arrival is over (everything at full). */
	bool IsRevealed() const;
	/**
	 * A frame of the menu's motion: the arrival, the lit button's tick, wash and words easing in, the mark's ember
	 * breathing, a new page fading in (NativeTick; tests step it). A menu nobody held starts arriving on its first frame.
	 */
	void AdvanceLook(float DeltaSeconds);
	/** How far a main button's lit look has come on, 0 to 1 (by its action). */
	float GetButtonLit(FName Action) const;
	/** The title's words (the game's title, from UTransitionScreenSubsystem::GameTitle). */
	UTextBlock* GetTitleText() const { return TitleText; }

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
	/** A main button's parts its look moves (held weakly: the widget tree keeps them). */
	struct FMainRow
	{
		TWeakObjectPtr<ULooterButton> Button;
		TWeakObjectPtr<UTextBlock> Label;
		TWeakObjectPtr<UTextBlock> Number;
		TWeakObjectPtr<UWidget> Tick;
		TWeakObjectPtr<UWidget> Wash;
		/** Its words lit (orange; Quit Game's warning red). */
		FLinearColor LitColor;
		float Lit = 0.f;
	};

	/** A part of the menu that arrives in turn: how long after the start it begins, and how far it slides in from the left. */
	struct FRevealPart
	{
		TWeakObjectPtr<UWidget> Widget;
		float Delay = 0.f;
		float Slide = 0.f;
	};

	ULooterButton* MakeButton(FName Action, int32 Index, const FString& Label, int32 FontSize, LooterUI::EButtonKind Kind);
	/** A main button: its number, its words, its tick and wash (no box), played with UI.Menu.Select. */
	UWidget* MakeMainButton(FName Action, int32 Number, const FString& Label, bool bDanger, const FString& Tag = FString());
	/** The menu's side: the shade, the title, the menu and the key hints at the bottom. */
	UWidget* MakeColumn();
	UWidget* MakeTitle();
	UWidget* MakeMainButtons();
	AMainMenuHUD* GetMenuHUD() const;

	// --- Motion (MainMenuMotion.cpp) ---

	void AddRevealPart(UWidget* Widget, float Delay, float Slide);
	/** The arrival as it stands at RevealAge. */
	void ApplyReveal();
	/** The main buttons' lit looks, eased on by DeltaSeconds. */
	void ApplyRows(float DeltaSeconds);
	/** The page on show (the main buttons or the picker) fading in after a change. */
	void ApplyPage();
	/** A page just changed: it fades in from the start. */
	void RestartPage();

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
	/** The hero select's pick, after its beat: session SlotIndex's new game starts with HeroId. */
	void HandleHeroConfirmed(int32 SlotIndex, FName HeroId);

	/** The menu's side on the left, its shade under it (hidden while the hero select shows). */
	UPROPERTY(Transient) TObjectPtr<UWidget> ColumnBox;
	/** The dark shade under the menu's side (a background: the UI transparency setting fades it). */
	UPROPERTY(Transient) TObjectPtr<UWidget> Shade;
	/** The title lockup's parts that move: the ember's glow, the orange rule (it draws out), the title's words. */
	UPROPERTY(Transient) TObjectPtr<UWidget> TitleGlow;
	UPROPERTY(Transient) TObjectPtr<UWidget> TitleRule;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	/** Over the whole screen while an empty slot's New Game chooses its hero; collapsed otherwise. */
	UPROPERTY(Transient) TObjectPtr<UHeroSelectWidget> HeroSelect;

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
	bool bHeroSelectShown = false;
	/** The session the delete confirmation asks about, or INDEX_NONE while it's closed. */
	int32 DeleteIndex = INDEX_NONE;
	/** A session's level is opening: the menu goes with this world, so it takes no more clicks or keys. */
	bool bLoading = false;

	// --- Motion (MainMenuMotion.cpp) ---

	TArray<FMainRow> MainRows;
	TArray<FRevealPart> RevealParts;
	/** Seconds since the arrival began. */
	float RevealAge = 0.f;
	bool bRevealHeld = false;
	bool bRevealStarted = false;
	/** Seconds the menu has been on screen (the ember's breath). */
	float LookClock = 0.f;
	/** Seconds since the page on show changed (large: no fade running). */
	float PageAge = 1000.f;
};
