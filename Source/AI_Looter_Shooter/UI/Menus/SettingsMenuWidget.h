#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuPreview.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "SettingsMenuWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UCanvasPanelSlot;
class UControlSettingsSubsystem;
class UGraphicsSettingsSubsystem;
class UHorizontalBox;
class UHudMinimapWidget;
class UImage;
class UKeyBindingSubsystem;
class ULooterButton;
class UOverlay;
class UPlayerViewComponent;
class UScrollBox;
class USettingsPageArt;
class USizeBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;
struct FAnchors;

/** Where the settings page is open. */
enum class ESettingsMenuMode : uint8
{
	/** Over the paused game, opened from the pause menu (UPauseMenuWidget): Back returns to it, SETTINGS lit. */
	Pause,
	/** From the main menu: Back returns to it. */
	MainMenu,
};

DECLARE_DELEGATE(FOnSettingsMenuAction);

/** A row of the page (SettingsMenuLedger.cpp): its gold light, its number and name (gold while it's the row marked), its page. */
struct FSettingsLedgerLine
{
	TWeakObjectPtr<UWidget> Row;
	TWeakObjectPtr<UWidget> Light;
	/** "01" in the number column (none for an unnumbered row: the key list's head). */
	TWeakObjectPtr<UTextBlock> Number;
	TWeakObjectPtr<UTextBlock> Name;
	/** The name's colour while it isn't marked (a Hold / Toggle row under its key is quieter). */
	FLinearColor NameColor = FLinearColor::White;
	int32 Page = INDEX_NONE;
	/** One of the key list's rows, which RebuildControls makes again. */
	bool bKeyList = false;
};

/**
 * The settings, a full page (the user, 2026-10-30: "a page instead of a popup"; their pick E2 "Big Dial", mockups
 * E2b_settings.png in Docs/Archive/Work/2026-10-30-PauseMenu): the same page from the pause menu and from the main menu. Top-left the
 * dial cut by the corner, its needle on SETTINGS over a ruler and the breadcrumb (PAUSED / SETTINGS or MAIN MENU / SETTINGS);
 * a long ruler with the four tabs hanging off it as pennants (Graphics, Audio, Interface, Controls), PGUP / PGDN at its ends;
 * big banded rows over the left two thirds (number, name, the control at the right: cut-corner switches, the gold one the
 * setting, or a ruler slider with a diamond knob), the row pointed at lit gold with a gold edge; the help card on the right
 * in the gun card's shape saying what that row does (SettingsMenuHelp.h) and, for graphics, what it costs; the key hints
 * bottom-right, ESC BACK clickable.
 *
 * Some sliders preview what they set while they move, the HUD being hidden under the page: the minimap size shows an
 * outline of the minimap where it sits, the minimap zoom a live copy of it there, and the first-person field of view
 * changes the view behind the page, which steps aside (the shade goes, the page ghosts) for as long as the slider is held,
 * and a moment after. Esc while one of the last two is held lets go of it with the setting as it was.
 *
 * A controller or the keyboard drive all of it, Borderlands' way: the D-pad (or the left stick, arrows, W A S D) moves the
 * focus down the page, steps a slider (held, faster and faster) or a switch; A (Enter) presses or changes; LB / RB (Page
 * Up / Down) turn the tabs, round the ends; B (Esc) lets go of a held preview, then goes back. The focus look and the
 * prompts follow the device last used; the mouse points, clicks the tabs and ESC BACK.
 *
 * SettingsMenuWidget.cpp builds the frame and fills it; SettingsMenuHeader.cpp is the dial, title and breadcrumb;
 * SettingsMenuTabs.cpp the pennants and the pages; SettingsMenuHelpCard.cpp the help card; SettingsMenuLedger.cpp a row's
 * band, light, number and switches; SettingsMenuRows.cpp the rows and the key list; SettingsMenuPages.cpp and
 * SettingsMenuAudio.cpp the pages; SettingsMenuInput.cpp its buttons, sliders and keys; SettingsMenuPreview.cpp the
 * previews; SettingsMenuNav.cpp the keyboard and controller focus and the key hints; SettingsMenuScroll.cpp the list
 * scrolling by whole rows; SettingsMenuLedgerMotion.cpp the
 * motion and the row marked. SettingsPageArt.h paints the pieces; PauseMenuArt.h draws the dial, pennants, card and gems.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows it fresh (settings read again, status cleared) for Mode. Add it to the viewport after. */
	void Open(ESettingsMenuMode InMode);

	ESettingsMenuMode GetMode() const { return Mode; }

	/** Back (to the pause menu over the game, to the main menu from it): Esc, B, or ESC BACK clicked. */
	FOnSettingsMenuAction OnClose;

	/**
	 * Escape (or the controller's menu button) while the field of view or minimap zoom slider is held: lets go of the
	 * slider with the setting back as it was when it was taken, and keeps the page open. False when neither is held.
	 */
	bool CancelPreviewDrag();

	/** The player's view component the field of view preview drives, for a page with no controller to find the pawn through (tests). */
	void UseViewForPreview(UPlayerViewComponent* View);

	/** A frame of the previews: they fade in and out with their sliders (NativeTick does this every frame; tests step it). */
	void AdvancePreviews(float DeltaSeconds);

	USlider* GetFieldOfViewSlider() const;
	USlider* GetMinimapZoomSlider() const;
	/** The live minimap the zoom preview shows (hidden until the zoom slider is held or pointed at). */
	UHudMinimapWidget* GetMinimapZoomPreview() const;
	bool IsFieldOfViewPreviewActive() const { return FieldOfViewState.IsActive(); }
	bool IsMinimapZoomPreviewActive() const { return ZoomState.IsActive(); }
	/** How far the game's view is uncovered by the field of view preview now: 0 = the page as usual, 1 = shade gone, page ghosted. */
	float GetViewFade() const { return AppliedViewFade; }

	// --- The keyboard and a controller (SettingsMenuNav.cpp) ---

	/**
	 * A key from the keyboard or a gamepad while the page isn't waiting for a new key binding (every key comes here, and
	 * the tests send theirs): the D-pad, arrows or W A S D move the focus, or step the slider or switch it's on; A or Enter
	 * presses or changes; LB / RB or Page Up / Down turn the tabs; B, Esc or Start let go of a held preview, then go back.
	 * The first move or press while the mouse drove the page only brings the focus look up. bRepeat: the platform's own
	 * repeat of a held key, ignored (held keys repeat on the page's clock, AdvanceNav). True when it was the page's.
	 */
	bool HandleNavKey(const FKey& Key, bool bRepeat = false);
	/** A key let go: a held direction stops repeating, and a slider it stepped is let go (and saved, as after a drag). */
	void HandleNavKeyUp(const FKey& Key);
	/** The left stick's position (-1 to 1, up +Y), read as a D-pad. */
	void HandleNavStick(const FVector2D& Stick);
	/** A frame of held keys' repeats, of the device check and the focus look's breath (NativeTick; tests step it). */
	void AdvanceNav(float DeltaSeconds);
	/** The widget the focus is on: a button, a slider, or a switch's first segment. */
	UWidget* GetFocusedWidget() const;
	/** The focus look is lit on the focused stop (it is only while keys or a gamepad drive the menus). */
	bool IsFocusLookShown() const;
	/** The key hints as they stand, for the device last used. */
	TArray<MenuNav::FPrompt> GetPrompts() const;
	/** ESC BACK in the key hints, a button: clicked, it goes back (its old name, from when Back was a row under the title). */
	ULooterButton* GetResumeButton() const;
	USlider* GetMinimapSizeSlider() const;

	// --- The tabs and the help card (SettingsMenuTabs.cpp, SettingsMenuHelpCard.cpp) ---

	/** The page on show (ESettingsPage). */
	int32 GetPage() const { return CurrentPage; }
	/** Turns to Page as a click on its tab does (the focus to its first setting). */
	void SelectPage(int32 Page);
	/** The tab of Page along the top (lit while it's the page on show). */
	ULooterButton* GetTabButton(int32 Page) const;
	/** The help card as it stands: the row's name and what it does. */
	FString GetShownHelpTitle() const;
	FString GetShownHelpText() const;
	/** A frame of the help card: it follows the focus under keys, the row pointed at (or marked) under the mouse (AdvanceNav runs it). */
	void AdvanceHelp();

	// --- The look (SettingsMenuHeader.cpp, SettingsMenuLedgerMotion.cpp) ---

	/** "PAUSED / SETTINGS" over the game, "MAIN MENU / SETTINGS" from the main menu. */
	FString GetBreadcrumb() const;
	/** The row lit gold now (the focus's under keys, the pointer's or the page's first under the mouse). */
	UWidget* GetLitRow() const { return MarkedRow.Get(); }
	/** A frame of the motion and of the row lit (NativeTick does this every frame; tests step it). */
	void AdvanceLook(float DeltaSeconds);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FCursorReply NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent) override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
		const FNavigationReply& InDefaultReply) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

private:
	UKeyBindingSubsystem* GetBindings() const;
	UGraphicsSettingsSubsystem* GetGraphics() const;
	UControlSettingsSubsystem* GetControls() const;
	UAudioSettingsSubsystem* GetAudio() const;
	/** Puts Widget on the page's canvas at Position from Anchor's corner, Alignment of it there, at its own size. */
	void PlaceOnPage(UWidget* Widget, const FAnchors& Anchor, const FVector2D& Position, const FVector2D& Alignment);
	/** A labelled two-way switch (On/Off, Hold/Toggle). bKeyListRow sets it under a key binding, in the key list's columns. */
	UWidget* MakeToggleRow(const FString& Label, const FString& FirstText, const FString& SecondText, FName FirstAction, FName SecondAction,
		int32 Index, bool bKeyListRow, ULooterButton*& OutFirst, ULooterButton*& OutSecond);
	/** A labelled row of choices, one lit at a time; each button's Index is its choice (Width: the old fixed column, unused). */
	UWidget* MakeChoiceRow(const FString& Label, const TArray<FString>& Choices, FName Action, float Width, TArray<ULooterButton*>& OutButtons);
	/**
	 * A labelled slider that moves in steps of StepSize (5% of a 0-1 range by default), with a text for its value beside
	 * it (the caller writes it). EndLabel words its ruler's two ends as the value is worded (none: as a percentage).
	 */
	UWidget* MakeSliderRow(const FString& Label, float MinValue, float MaxValue, USlider*& OutSlider, UTextBlock*& OutValue,
		float StepSize = 0.05f, TFunction<FString(float)> EndLabel = nullptr);
	/** The outline of the HUD's minimap, in the same corner, for previewing its size. */
	UWidget* MakeMinimapPreview();

	// --- A row's pieces (SettingsMenuLedger.cpp) ---

	/** A switch button (its Back and Front pieces) showing its state, sounding as the kit's SoundKind button of Action. */
	ULooterButton* MakeArtButton(FName Action, int32 Index, const FString& Label, int32 FontSize, LooterUI::EButtonKind SoundKind,
		USettingsPageArt** OutLabel = nullptr);
	/**
	 * Content on a banded row at least Height tall, numbered (bNumbered), its gold light under it. Name: the row's name in
	 * Content, gold while it's the row marked (NameColor otherwise).
	 */
	UWidget* MakeLedgerRow(UWidget* Content, UTextBlock* Name, float Height, bool bNumbered, bool bKeyList = false,
		const FLinearColor& NameColor = FLinearColor::White);
	/** A row's gold light (null for anything else). */
	UWidget* LedgerLightOf(const UWidget* Row) const;
	/** A key list heading ("MOVEMENT"), gold and spaced. */
	UWidget* MakeLedgerSection(const FString& Label);
	/** A row's name, its size and colour (it wraps rather than run under the control). */
	UTextBlock* MakeRowName(const FString& Label, int32 Size, const FLinearColor& Tint);

	// --- The header (SettingsMenuHeader.cpp) ---

	/** The dial in the corner, SETTINGS over its ruler, the breadcrumb, on the page's canvas. */
	void MakeHeader();
	/** The breadcrumb's first word: PAUSED or MAIN MENU. */
	void RefreshCrumb();

	// --- The motion and the row marked (SettingsMenuLedgerMotion.cpp) ---

	/** The open: the dial swings up, the title, tabs, rows, card and hints arrive one after another. */
	void BeginOpenMotion();
	/** A tab turned: the dial clicks a tick that way, the lit pennant drops in, the rows arrive again. */
	void BeginPageMotion(int32 Direction);
	/** The row the light belongs at: the focus's under keys, the pointer's under the mouse; bOutHide: none. */
	UWidget* FindMarkedRow(bool& bOutHide) const;
	void MarkRow(UWidget* Row);
	/** The page's first numbered row. */
	UWidget* DefaultMarkedRow() const;
	/** Each tab's look: its pennant lit or plain, its icon and word gold, white pointed at, else grey (SettingsMenuTabs.cpp). */
	void RefreshTabLooks();

	// --- The pages (SettingsMenuPages.cpp; the Audio page is SettingsMenuAudio.cpp) ---

	void MakeGraphicsPage();
	void MakeInterfacePage();
	/** The view's feel, then the key list's hint and Reset all keys, then the key list (RebuildControls fills it). */
	void MakeControlsPage();

	// --- The tabs and the help card (SettingsMenuTabs.cpp, SettingsMenuHelpCard.cpp) ---

	/** The ruler across the page, the pennants hanging off it, PGUP / PGDN at its ends. */
	UWidget* MakeTabBar();
	/** The help card on the page's right. */
	void MakeHelpCard();
	/** A new page's scrolling list, added to the switcher: the rows, stops and descriptions made after it are its own. */
	UVerticalBox* BeginPage();
	/** A row on the page being built (a gap above it). Returns Row. */
	UWidget* AddPageRow(UWidget* Row, float Top = 0.f);
	/** Shows Page: its tab lit, its list from the top, the focus to its first setting, its own words on the card. */
	void ShowPage(int32 Page);
	/** LB / RB: the page beside, round the ends, with the tab's sound. */
	void StepPage(int32 Step);
	/** Notes Target's words for the help card (on the page being built, or every page). Returns Target. */
	UWidget* Describe(UWidget* Target, const SettingsMenu::FSettingHelp& Help, bool bEveryPage = false, bool bQuality = false, bool bKeyList = false);
	/** The words for Widget (a row or a button), on the page on show or every page; null when it has none. */
	const SettingsMenu::FHelpEntry* FindHelp(const UWidget* Widget) const;
	/** Puts Help on the card (a no-op when it's already there); bPageWords: a page's own words, not a row's. */
	void ShowHelp(const SettingsMenu::FSettingHelp& Help, bool bPageWords = false);
	/** The help card's gem shows the page's icon. */
	void ShowHelpIcon(int32 Page);
	/** The keys that turn the tabs, at each end of them, named for the device last used. */
	void RefreshTabKeys();
	void RebuildControls();
	void RefreshKeyLabels();
	void RefreshGraphics();
	/** The Controls page's settings above the key list (which RefreshKeyLabels fills). */
	void RefreshControlSettings();
	/** Shows the parts that differ between the pause menu's page and the main menu's. */
	void ApplyMode();
	/** The size shown beside the minimap slider and by the preview outline. */
	void ShowMinimapScale(float Scale);

	// --- The Audio page (SettingsMenuAudio.cpp) ---

	/** The page of the four volume sliders (Master, Effects, Interface, Music), each a row with its percentage. */
	void MakeAudioPage();
	void RefreshAudio();
	/** A volume slider moved: heard at once, with a soft tick at each step; saved when it's let go. */
	void ChangeVolume(EAudioVolume Which, float Value);

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);
	UFUNCTION()
	void HandleEffectsVolumeChanged(float Value);
	UFUNCTION()
	void HandleInterfaceVolumeChanged(float Value);
	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);
	/** Any volume slider let go: the volumes are saved. */
	UFUNCTION()
	void HandleVolumeReleased();
	UFUNCTION()
	void HandleTransparencyChanged(float Value);
	/** The slider was let go: save the setting (not on every step of a drag). */
	UFUNCTION()
	void HandleTransparencyReleased();
	UFUNCTION()
	void HandleMinimapSizeChanged(float Value);
	UFUNCTION()
	void HandleMinimapSizeGrabbed();
	UFUNCTION()
	void HandleMinimapSizeReleased();

	// --- The minimap zoom and field of view sliders, and their previews (SettingsMenuPreview.cpp) ---

	/** The zoom slider moved: the setting follows at once, and the live minimap in the corner shows it. */
	UFUNCTION()
	void HandleMinimapZoomChanged(float Value);
	/** The slider was taken, by the mouse or a controller. */
	UFUNCTION()
	void HandleMinimapZoomGrabbed();
	UFUNCTION()
	void HandleMinimapZoomReleased();
	/** The field of view slider moved: the setting follows at once, and the view behind the page shows it. */
	UFUNCTION()
	void HandleFieldOfViewChanged(float Value);
	UFUNCTION()
	void HandleFieldOfViewGrabbed();
	UFUNCTION()
	void HandleFieldOfViewReleased();
	/** The player's view, which the field of view preview drives (none in the main menu). */
	UPlayerViewComponent* FindPlayerView() const;
	/** The view behind the page takes Degrees; the page steps aside while the slider is held (third person has none to show). */
	void ShowFieldOfViewPreview(float Degrees);
	/** Fades the shade and the page for the field of view preview; Fade 0 is the page as usual. */
	void ApplyViewFade(float Fade);

	UFUNCTION()
	void HandleLookSensitivityChanged(float Value);
	UFUNCTION()
	void HandleLookSensitivityReleased();
	UFUNCTION()
	void HandleCameraShakeChanged(float Value);
	UFUNCTION()
	void HandleCameraShakeReleased();
	UFUNCTION()
	void HandleControllerRumbleChanged(float Value);
	UFUNCTION()
	void HandleControllerRumbleReleased();
	void HandleButton(ULooterButton* Button);
	void StartListening(int32 BindingIndex);
	void StopListening();
	void AssignKey(const FKey& Key);
	void SetStatus(const FString& Message, const FLinearColor& Color);

	// --- The keyboard and a controller (SettingsMenuNav.cpp) ---

	/** A slider's row as a page is built: a stop of that page, the row lit by its light. Returns Row. (These three: SettingsMenuTabs.cpp.) */
	UWidget* NavSlider(UWidget* Row, USlider* Slider);
	/** A row of choices (a switch, the quality presets) as a page is built. Returns Row. */
	UWidget* NavChoices(UWidget* Row, const TArray<ULooterButton*>& Choices);
	/** A button on a page as it's built (Reset all keys), in Row (what scrolls into view for it). */
	void NavButton(ULooterButton* Button, UWidget* Row);
	/** The stops, top to bottom: the page on show (and on Controls the key list). Prefer: where the focus goes. */
	void RebuildNavigation(const UWidget* Prefer = nullptr);
	/** The focus to the page's first setting. */
	void ResetNavigation();
	/** The first stop of the page on show (null on an empty page). */
	UWidget* FirstPageStop() const;
	/** A direction pressed: one step now, then repeats while it's held. */
	void PressNav(const FKey& Key, MenuNav::EMove Move);
	void StepNav(MenuNav::EMove Move);
	void ActivateNav();
	/** Notes Key's device. False when the focus look wasn't showing before (that first press only brings it up). */
	bool RevealNav(const FKey& Key);
	/** The look for the device driving the menus: the focus look, the hidden pointer, the hints, the key list's hint. */
	void RefreshNavLook();
	/** Under the mouse, the slider or choices row the pointer is over takes the focus, so the keys carry on from it. */
	void AdvanceHoverLook();
	void RefreshPrompts();
	/** Brings the focused stop's row whole into the list's view, by whole rows. */
	void ScrollToFocus();
	/** The most the rows' list may be tall: from RowsTop down to the key hints' band. */
	float ListRoom() const;
	/** The page on show's rows (key list rows each), their tops down the list and the list's end; false before they're laid out. */
	bool GetRowSpans(TArray<float>& OutTops, float& OutEnd, TArray<const UWidget*>* OutRows = nullptr) const;
	/** Scrolls (eased) so Row is wholly in view, the view starting on a row's top. */
	void ScrollRowIntoView(const UWidget* Row);
	/** The mouse wheel: Rows rows down (up when negative), whole rows at a time. */
	void ScrollByRows(int32 Rows);
	/** A frame of the list: a scroll under way eases on, a still list snaps to a row's top, its height shows whole rows only. */
	void AdvanceScroll(float DeltaSeconds);
	/** The help card as tall as its words need (at least HelpCardHeight), its pictures redrawn at that height. */
	void SizeHelpCard();
	/** Held keys stop repeating and a slider they held is let go. */
	void StopNavHold();
	/** The mouse was used: the focus look goes, the hints name the keyboard's keys. */
	void NotePointer();
	/** The mouse moved onto a button: with the mouse in charge the focus follows it, so keys carry on from there. */
	void HandleButtonHovered(ULooterButton* Button);

	FMenuNavigator Nav;
	MenuNav::FRepeat NavRepeat;
	FVector2D NavStick = FVector2D::ZeroVector;
	MenuNav::EMove NavStickMove = MenuNav::EMove::None;
	/** Each page's stops, top to bottom, as the pages are built (by ESettingsPage), and the key list's (RebuildControls). */
	TArray<TArray<FMenuNavRow>> PageNav;
	TArray<FMenuNavRow> KeyNav;
	/** The page the building has reached (BeginPage). */
	int32 BuildPage = INDEX_NONE;
	/** The page on show (ESettingsPage). */
	int32 CurrentPage = 0;
	/** The device the look was last set for (RefreshNavLook). */
	LooterUI::EMenuDevice ShownDevice = LooterUI::EMenuDevice::Mouse;
	bool bNavLookApplied = false;
	/** The hints on show, as one line: they are rebuilt only when they change. */
	FString ShownPrompts;

	/** The page's canvas (everything but the shade and the previews): it ghosts for the field of view's preview. */
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> PageCanvas;
	/** The pages, one scrolling list each, of which the switcher shows the current one (Body is that one's list). */
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient) TArray<TObjectPtr<UScrollBox>> PageLists;
	/** The page being built's rows (BeginPage, AddPageRow). */
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> BuildRows;
	/** The tabs, by page: the buttons, their cells (which arrive in the motion), the plain and lit pennants, icons, words. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> TabButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> TabCells;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> TabPlainLooks;
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> TabLitLooks;
	UPROPERTY(Transient) TArray<TObjectPtr<UImage>> TabIcons;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> TabWords;
	/** Each tab's look as last set (0 plain, 1 pointed at, 2 lit), so it's set only when it changes. */
	TArray<uint8> TabLookShown;
	/** The key caps at each end of the tabs: LB and RB, or PGUP and PGDN. */
	UPROPERTY(Transient) TObjectPtr<USettingsPageArt> TabKeyPrevious;
	UPROPERTY(Transient) TObjectPtr<USettingsPageArt> TabKeyNext;

	// --- The header ---

	/** The dial in the corner (it turns as the page opens and clicks as a tab turns), the title block, the crumb's first word. */
	UPROPERTY(Transient) TObjectPtr<UImage> Dial;
	UPROPERTY(Transient) TObjectPtr<UWidget> TitleBlock;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CrumbFirst;

	/** Every row, the next number on each page as it's built, and the key list's first (after the page's sliders). */
	TArray<FSettingsLedgerLine> LedgerLines;
	TArray<int32> PageRowNumbers;
	int32 KeyNumberStart = 1;
	/** The Controls page's band count where the key list starts, so its rows alternate on from the sliders'. */
	int32 KeyBandStart = 0;
	/** Each page's rows so far, for the bands' alternation. */
	TArray<int32> PageBandCount;
	/** The row lit (its number and name gold), the times since the page opened, turned, and the card's words changed. */
	TWeakObjectPtr<UWidget> MarkedRow;
	float OpenAge = 10.f;
	float PageAge = 10.f;
	float HelpAge = 10.f;
	/** The dial's click a tab turn makes, that way (+1 or -1). */
	float DialClickAge = 10.f;
	float DialClickDirection = 1.f;

	// --- The rows' list scrolling by whole rows (SettingsMenuScroll.cpp) ---

	/** The list's slot on the canvas (its height is whole rows), where the list was last set, a scroll under way. */
	TWeakObjectPtr<UCanvasPanelSlot> PagesSlot;
	float ScrollSetOffset = 0.f;
	float ScrollTarget = -1.f;
	/** How long the list has been still since the scroll bar moved it, and its height as shown. */
	float ScrollIdle = 0.f;
	float ShownListHeight = 0.f;

	/** Every described row and button (Describe), and the help card's lines. */
	TArray<SettingsMenu::FHelpEntry> HelpEntries;
	UPROPERTY(Transient) TObjectPtr<UWidget> HelpCard;
	UPROPERTY(Transient) TObjectPtr<USizeBox> HelpCardBox;
	/** The card's glass (marked) and lines, redrawn at the height its words need (SizeHelpCard), and that height. */
	UPROPERTY(Transient) TObjectPtr<UImage> HelpCardFill;
	UPROPERTY(Transient) TObjectPtr<UImage> HelpCardLines;
	float HelpCardShownHeight = 0.f;
	/** The card fits its words as they stand (they are measured once the card is on screen: AdvanceLook). */
	bool bHelpCardSized = false;
	UPROPERTY(Transient) TObjectPtr<UWidget> HelpWords;
	UPROPERTY(Transient) TObjectPtr<UImage> HelpIcon;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpLabel;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpBody;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpDetail;
	/** What the card shows: it's rewritten only when that changes. */
	SettingsMenu::FSettingHelp ShownHelp;

	/** Each key binding's Default button, beside its key, and the key buttons' words (their painted labels). */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> DefaultButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<USettingsPageArt>> KeyLabels;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> ResetAllButton;
	/** The key hints bottom-right: the ones that change with the focus, then ESC BACK (ResumeButton, its key and word). */
	UPROPERTY(Transient) TObjectPtr<UWidget> Hints;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
	UPROPERTY(Transient) TObjectPtr<USettingsPageArt> BackKey;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BackWord;
	/** The line over the key list: how to change a key, for the keyboard or a gamepad. */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyHintText;

	ESettingsMenuMode Mode = ESettingsMenuMode::Pause;

	/** The page on show's scrolling list (one of PageLists). */
	UPROPERTY(Transient) TObjectPtr<UScrollBox> Body;
	/** The key bindings' rows (RebuildControls fills it). */
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> KeyList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	/** ESC BACK: back to the pause menu or the main menu. */
	UPROPERTY(Transient) TObjectPtr<ULooterButton> ResumeButton;
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> KeyButtons;
	/** Hold/Toggle switch halves; Index is the binding, Action says which half. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> ModeButtons;
	/** Low, Medium, High, Epic, in order. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> QualityButtons;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MotionBlurOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MotionBlurOff;
	/** The first-person field of view, in degrees. */
	UPROPERTY(Transient) TObjectPtr<USlider> FieldOfViewSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FieldOfViewValue;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MinimapOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MinimapOff;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> FrameRateOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> FrameRateOff;
	/** The crosshair colours, in AllCrosshairColours' order. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> CrosshairColourButtons;
	/** The contextual control hints' switch. */
	UPROPERTY(Transient) TObjectPtr<ULooterButton> HintsOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> HintsOff;
	/** The subtitles' switch (the spoken lines' words on screen). */
	UPROPERTY(Transient) TObjectPtr<ULooterButton> SubtitlesOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> SubtitlesOff;
	UPROPERTY(Transient) TObjectPtr<USlider> TransparencySlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TransparencyValue;
	UPROPERTY(Transient) TObjectPtr<USlider> MinimapSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapValue;
	UPROPERTY(Transient) TObjectPtr<USlider> MinimapZoomSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapZoomValue;
	/** The look sensitivity, as a multiple of the game's own turn. */
	UPROPERTY(Transient) TObjectPtr<USlider> LookSensitivitySlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LookSensitivityValue;
	/** How strongly the view kicks and shakes, 0-100%. */
	UPROPERTY(Transient) TObjectPtr<USlider> CameraShakeSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CameraShakeValue;
	/** How strongly a gamepad rumbles (shots, hits taken), 0-100%. */
	UPROPERTY(Transient) TObjectPtr<USlider> ControllerRumbleSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ControllerRumbleValue;
	/** The volume sliders and their percentages, in EAudioVolume's order. */
	UPROPERTY(Transient) TArray<TObjectPtr<USlider>> VolumeSliders;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> VolumeValues;
	UPROPERTY(Transient) TObjectPtr<UWidget> MinimapPreview;
	UPROPERTY(Transient) TObjectPtr<USizeBox> MinimapPreviewSize;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapPreviewCaption;
	/** The preview's outer outline, out at the HUD minimap's bezel (the map's circle and glass sit inside it). */
	UPROPERTY(Transient) TObjectPtr<UImage> MinimapPreviewBezel;

	/** The live minimap the zoom preview shows in the HUD minimap's corner (SetPreview). */
	UPROPERTY(Transient) TObjectPtr<UHudMinimapWidget> MinimapZoomPreview;
	/** The shade over the game, and the page in front of it: the field of view preview fades both. */
	UPROPERTY(Transient) TObjectPtr<UBorder> Dimmer;
	UPROPERTY(Transient) TObjectPtr<UWidget> Panel;

	/**
	 * The previews, each up while its slider is held (or pointed at, for the minimap's two), then a moment after. The minimap's
	 * size and zoom sliders show in the same corner, so whichever is held or pointed at now takes it.
	 */
	SettingsMenu::FSliderPreview SizeState;
	SettingsMenu::FSliderPreview ZoomState;
	SettingsMenu::FSliderPreview FieldOfViewState;
	/** The field of view change shows in the view behind the page (first person), so the page steps aside while it is up. */
	bool bFieldOfViewInView = false;
	/** How far the page has stepped aside (ApplyViewFade). */
	float AppliedViewFade = 0.f;
	/** Set while the sliders are given their saved values: that is not a move, and previews nothing. */
	bool bRefreshingSliders = false;
	TWeakObjectPtr<UPlayerViewComponent> ViewForPreview;

	/** The volume slider moved last, for the status line when it's let go. */
	EAudioVolume LastVolumeMoved = EAudioVolume::Master;

	/** Index into the subsystem's binding list we're waiting on a key for, or INDEX_NONE. */
	int32 ListeningIndex = INDEX_NONE;
};
