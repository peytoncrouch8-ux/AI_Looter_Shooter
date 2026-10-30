#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuPreview.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "SettingsMenuWidget.generated.h"

class UBorder;
class UCanvasPanel;
class UControlSettingsSubsystem;
class UGraphicsSettingsSubsystem;
class UHorizontalBox;
class UHudMinimapWidget;
class UImage;
class UKeyBindingSubsystem;
class ULooterButton;
class ULooterHollowKey;
class UOverlay;
class UPlayerViewComponent;
class UScrollBox;
class USizeBox;
class USlider;
class UTextBlock;
class UVerticalBox;
class UWidgetSwitcher;

/** Where the settings menu is open. */
enum class ESettingsMenuMode : uint8
{
	/**
	 * Over the paused game, opened from the pause menu's list (UPauseMenuWidget): Back returns to the list. Save & Quit is
	 * the list's alone (the user, 2026-10-29 night: "Remove save and quit from the settings menu at the bottom").
	 */
	Pause,
	/** From the main menu: Back. */
	MainMenu,
};

DECLARE_DELEGATE(FOnSettingsMenuAction);

/** A row of the ledger (SettingsMenuLedger.cpp): its focus look, the number in its margin, the page it's on. */
struct FSettingsLedgerLine
{
	TWeakObjectPtr<UWidget> Row;
	TWeakObjectPtr<UWidget> Light;
	/** "01" in the margin (none for an unnumbered row: the key list's head). */
	TWeakObjectPtr<UTextBlock> Number;
	int32 Page = INDEX_NONE;
	/** One of the key list's rows, which RebuildControls makes again. */
	bool bKeyList = false;
};

/**
 * The settings menu, in four tabbed pages (the user, 2026-10-27: tabs, not one long scroll): Graphics, Audio (the volumes),
 * Interface, and Controls (the view's feel and the key bindings). Each page scrolls on its own if it must. Beside the page
 * a description column says what the row pointed at (or with the keyboard or controller focus) does, in plain words, and
 * for graphics what it costs (SettingsMenuHelp.h). The pause menu's Settings opens it over the paused game (ALooterHUD) and
 * the main menu's Settings opens it there; its Back goes back to whichever menu opened it. The HUD hides under it, so some sliders preview what they set while they move:
 * the minimap size shows an outline of the minimap (the map's circle and the bezel round it) where it sits, at the size
 * chosen; the minimap zoom shows a live copy of the minimap there; and the first-person field of view changes the view
 * behind the menu, which fades back (the dimming goes, the panel ghosts) for as long as the slider is held, and a moment
 * after. Esc while one of the last two is held lets go of it with the setting as it was.
 *
 * A controller or the keyboard drive all of it, Borderlands' way: the D-pad (or the left stick, arrows, W A S D) moves the
 * focus down the page, steps a slider (held, faster and faster) or a switch; A (Enter) presses or changes; LB / RB (Page
 * Up / Down) turn the tabs, round the ends; B (Esc) lets go of a held preview, then closes. The focus look
 * (LooterUIFocus.h) and the prompts along the bottom follow the device last used; the mouse clicks the tabs.
 *
 * SettingsMenuWidget.cpp builds the frame and fills it; SettingsMenuPages.cpp builds the Graphics, Interface and Controls
 * pages; SettingsMenuRows.cpp makes their rows and the key list; SettingsMenuTabs.cpp is the tabs and the description
 * column; SettingsMenuInput.cpp handles its buttons, sliders and keys; SettingsMenuAudio.cpp is the Audio page;
 * SettingsMenuPreview.cpp is the field of view and minimap zoom sliders and the previews; SettingsMenuNav.cpp is the
 * keyboard and controller focus, its look and the prompts.
 *
 * Its look (round 3, the user's mix of concepts 1 and 3, 2026-10-29): a slate plate with a double salt trim, rivets and cut
 * corners, a halo dial breaking out over its top-left corner; every row on a ruled ledger line with its number in the
 * margin and a brass ribbon at the row pointed at; c3's reticle Back, tabs, choices and rulers. SettingsMenuLedger.cpp
 * builds those pieces and moves them; SettingsMenuArt.h paints them (the pause menu's list uses them too).
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows it fresh (settings read again, status cleared) for Mode. Add it to the viewport after. */
	void Open(ESettingsMenuMode InMode);

	ESettingsMenuMode GetMode() const { return Mode; }

	/** Back (to the pause menu's list over the game, to the main menu from it), or Escape. */
	FOnSettingsMenuAction OnClose;

	/**
	 * Escape (or the controller's menu button) while the field of view or minimap zoom slider is held: lets go of the
	 * slider with the setting back as it was when it was taken, and keeps the menu open. False when neither is held.
	 */
	bool CancelPreviewDrag();

	/** The player's view component the field of view preview drives, for a menu with no controller to find the pawn through (tests). */
	void UseViewForPreview(UPlayerViewComponent* View);

	/** A frame of the previews: they fade in and out with their sliders (NativeTick does this every frame; tests step it). */
	void AdvancePreviews(float DeltaSeconds);

	USlider* GetFieldOfViewSlider() const;
	USlider* GetMinimapZoomSlider() const;
	/** The live minimap the zoom preview shows (hidden until the zoom slider is held or pointed at). */
	UHudMinimapWidget* GetMinimapZoomPreview() const;
	bool IsFieldOfViewPreviewActive() const { return FieldOfViewState.IsActive(); }
	bool IsMinimapZoomPreviewActive() const { return ZoomState.IsActive(); }
	/** How far the game's view is uncovered by the field of view preview now: 0 = the menu as usual, 1 = dimming gone, panel ghosted. */
	float GetViewFade() const { return AppliedViewFade; }

	// --- The keyboard and a controller (SettingsMenuNav.cpp) ---

	/**
	 * A key from the keyboard or a gamepad while the menu isn't waiting for a new key binding (every key comes here, and
	 * the tests send theirs): the D-pad, arrows or W A S D move the focus, or step the slider or switch it's on; A or Enter
	 * presses or changes; LB / RB or Page Up / Down turn the tabs; B, Esc or Start let go of a held preview, then close
	 * the menu. The first move or press while the mouse drove the menu only brings the focus look up. bRepeat: the
	 * platform's own repeat of a held key, ignored (held keys repeat on the menu's clock, AdvanceNav). True when it was
	 * the menu's.
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
	/** The prompts along the bottom as they stand, for the device last used. */
	TArray<MenuNav::FPrompt> GetPrompts() const;
	ULooterButton* GetResumeButton() const;
	USlider* GetMinimapSizeSlider() const;

	// --- The tabs and the description column (SettingsMenuTabs.cpp) ---

	/** The page on show (ESettingsPage). */
	int32 GetPage() const { return CurrentPage; }
	/** Turns to Page as a click on its tab does (the focus to its first setting). */
	void SelectPage(int32 Page);
	/** The tab of Page along the top (lit while it's the page on show). */
	ULooterButton* GetTabButton(int32 Page) const;
	/** The description column as it stands: the row's name and what it does. */
	FString GetShownHelpTitle() const;
	FString GetShownHelpText() const;
	/** A frame of the description column: it follows the focus under keys, the pointer under the mouse (AdvanceNav runs it). */
	void AdvanceHelp();

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
	ULooterButton* MakeButton(FName Action, int32 Index, const FString& Label, int32 FontSize = 13,
		LooterUI::EButtonKind Kind = LooterUI::EButtonKind::Normal);
	/** A labelled two-way switch (On/Off, Hold/Toggle). bKeyListRow indents it under a key binding and aligns it to that list's columns. */
	UWidget* MakeToggleRow(const FString& Label, const FString& FirstText, const FString& SecondText, FName FirstAction, FName SecondAction,
		int32 Index, bool bKeyListRow, ULooterButton*& OutFirst, ULooterButton*& OutSecond);
	/** A labelled row of choices, one highlighted at a time; each button's Index is its choice (Width: the old fixed column, unused). */
	UWidget* MakeChoiceRow(const FString& Label, const TArray<FString>& Choices, FName Action, float Width, TArray<ULooterButton*>& OutButtons);
	/**
	 * A labelled slider that moves in steps of StepSize (5% of a 0-1 range by default), with a text for its value beside
	 * it (the caller writes it: a percentage, a zoom, degrees). EndLabel words its ruler's two ends as the value is worded
	 * (none: as a percentage).
	 */
	UWidget* MakeSliderRow(const FString& Label, float MinValue, float MaxValue, USlider*& OutSlider, UTextBlock*& OutValue,
		float StepSize = 0.05f, TFunction<FString(float)> EndLabel = nullptr);
	/** The outline of the HUD's minimap, in the same corner, for previewing its size. */
	UWidget* MakeMinimapPreview();

	// --- Round 3's look: the plate, the ledger, the halo and the motion (SettingsMenuLedger.cpp) ---

	/** A button that is a painted piece (a chip, a tab) showing its state, sounding as the kit's SoundKind button of Action. */
	ULooterButton* MakeArtButton(FName Action, int32 Index, const FString& Label, ESettingsArt Look, int32 FontSize, int32 LetterSpacing,
		LooterUI::EButtonKind SoundKind);
	/** Content on a ruled line at least Height tall, numbered in the margin (bNumbered), its focus look under it. */
	UWidget* MakeLedgerRow(UWidget* Content, float Height, bool bNumbered, bool bKeyList = false);
	/** A ledger row's focus look (null for anything else). */
	UWidget* LedgerLightOf(const UWidget* Row) const;
	UWidget* MakeLedgerSection(const FString& Label);
	/** SETTINGS and its ruler and plate; Back in its reticle; the serial and the (hidden) Back chip. */
	UWidget* MakeHeader();
	UWidget* MakeResumeRow();
	UWidget* MakeFooter();
	/** The free art over the plate: the halo on its top-left corner, the range arc in the bottom-left, the ribbon. */
	void MakePlateArt();
	/** The plate's words (the area and session, the serial with the player's level) and the halo's pause bars. */
	void RefreshPlate();
	/** The open, the ribbon and the tab mark sliding to theirs, the reticle turning (SettingsMenuLedgerMotion.cpp). */
	void BeginOpenMotion();
	void AdvanceLook(float DeltaSeconds);
	/** The row the ribbon belongs at: the focus's under keys, the pointer's under the mouse; bOutHide: none. */
	UWidget* FindMarkedRow(bool& bOutHide) const;
	void MarkRow(UWidget* Row);
	/** The top row (Back) over the game, else the page's first row. */
	UWidget* DefaultMarkedRow() const;

	// --- The pages (SettingsMenuPages.cpp; the Audio page is SettingsMenuAudio.cpp) ---

	void MakeGraphicsPage();
	void MakeInterfacePage();
	/** The view's feel, then the key list's hint and Reset all keys, then the key list (RebuildControls fills it). */
	void MakeControlsPage();

	// --- The tabs and the description column (SettingsMenuTabs.cpp) ---

	/** The tabs along the top, with the keys that turn them at each end. */
	UWidget* MakeTabBar();
	/** The description column beside the page. */
	UWidget* MakeHelpColumn();
	/** A new page's scrolling list, added to the switcher: the rows, stops and descriptions made after it are its own. */
	UVerticalBox* BeginPage();
	/** A row on the page being built (a gap above it). Returns Row. */
	UWidget* AddPageRow(UWidget* Row, float Top = 0.f);
	/** Shows Page: its tab lit, its list from the top, the focus to its first setting, its own words in the column. */
	void ShowPage(int32 Page);
	/** LB / RB: the page beside, round the ends, with the tab's sound. */
	void StepPage(int32 Step);
	/** Notes Target's words for the description column (on the page being built, or every page). Returns Target. */
	UWidget* Describe(UWidget* Target, const SettingsMenu::FSettingHelp& Help, bool bEveryPage = false, bool bQuality = false, bool bKeyList = false);
	/** The words for Widget (a row or a button), on the page on show or every page; null when it has none. */
	const SettingsMenu::FHelpEntry* FindHelp(const UWidget* Widget) const;
	/** Puts Help in the column (a no-op when it's already there). */
	void ShowHelp(const SettingsMenu::FSettingHelp& Help);
	/** The keys that turn the tabs, at each end of them, named for the device last used. */
	void RefreshTabKeys();
	void RebuildControls();
	void RefreshKeyLabels();
	void RefreshGraphics();
	/** The Controls section's settings above the key list (which RefreshKeyLabels fills). */
	void RefreshControlSettings();
	/** Shows the parts that differ between the pause menu and the main menu's settings. */
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

	/** The field of view slider moved: the setting follows at once, and the view behind the menu shows it. */
	UFUNCTION()
	void HandleFieldOfViewChanged(float Value);

	UFUNCTION()
	void HandleFieldOfViewGrabbed();

	UFUNCTION()
	void HandleFieldOfViewReleased();

	/** The player's view, which the field of view preview drives (none in the main menu). */
	UPlayerViewComponent* FindPlayerView() const;
	/** The view behind the menu takes Degrees; the menu steps aside while the slider is held (third person has none to show). */
	void ShowFieldOfViewPreview(float Degrees);
	/** Fades the dimming and the panel for the field of view preview; Fade 0 is the menu as usual. */
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

	/** A slider's row as a page is built: a stop of that page, the row lit by its focus look. Returns Row. (These three: SettingsMenuTabs.cpp.) */
	UWidget* NavSlider(UWidget* Row, USlider* Slider);
	/** A row of choices (a switch, the quality presets) as a page is built. Returns Row. */
	UWidget* NavChoices(UWidget* Row, const TArray<ULooterButton*>& Choices);
	/** A button on a page as it's built (Reset all keys), in Row (what scrolls into view for it). */
	void NavButton(ULooterButton* Button, UWidget* Row);
	/**
	 * The stops, top to bottom: the top row (Back), the page on show (and on Controls the key list), the footer. Prefer:
	 * where the focus goes (else it stays, else the first stop).
	 */
	void RebuildNavigation(const UWidget* Prefer = nullptr);
	/** The focus to where the menu starts: the top row (Back) over the game, the first setting from the main menu. */
	void ResetNavigation();
	/** The first stop of the page on show (null on an empty page). */
	UWidget* FirstPageStop() const;
	/** A direction pressed: one step now, then repeats while it's held. */
	void PressNav(const FKey& Key, MenuNav::EMove Move);
	void StepNav(MenuNav::EMove Move);
	void ActivateNav();
	/** Notes Key's device. False when the focus look wasn't showing before (that first press only brings it up). */
	bool RevealNav(const FKey& Key);
	/** The look for the device driving the menus: the focus look, the hidden pointer, the prompts, the key list's hint. */
	void RefreshNavLook();
	/**
	 * Under the mouse, the slider or choices row the pointer is over wears the focus look (the keys' focus look, which the
	 * mouse never lit: playtest 2026-10-27), and takes the focus so the keys carry on from it. AdvanceNav runs it.
	 */
	void AdvanceHoverLook();
	void RefreshPrompts();
	/** Brings the focused stop into the list's view (Resume: the list's top). */
	void ScrollToFocus();
	/** Held keys stop repeating and a slider they held is let go. */
	void StopNavHold();
	/** The mouse was used: the focus look goes, the prompts name the keyboard's keys. */
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
	/** The row's focus look lit by the pointer (AdvanceHoverLook), so it is put out when the pointer leaves or the keys take over. */
	TWeakObjectPtr<UWidget> HoverLit;
	/** The prompts on show, as one line: the bar is rebuilt only when they change. */
	FString ShownPrompts;

	/** The pages, one scrolling list each, of which the switcher shows the current one (Body is that one's list). */
	UPROPERTY(Transient) TObjectPtr<UWidgetSwitcher> Pages;
	UPROPERTY(Transient) TArray<TObjectPtr<UScrollBox>> PageLists;
	/** The page being built's rows (BeginPage, AddPageRow). */
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> BuildRows;
	/** The tabs along the top, by page. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> TabButtons;
	/** The hollow key caps at each end of the tabs: LB and RB, or PgUp and PgDn. */
	UPROPERTY(Transient) TObjectPtr<ULooterHollowKey> TabKeyPrevious;
	UPROPERTY(Transient) TObjectPtr<ULooterHollowKey> TabKeyNext;

	// --- Round 3's look (SettingsMenuLedger.cpp) ---

	/** Every ledger row, the next number on each page as it's built, and the key list's first (after the page's sliders). */
	TArray<FSettingsLedgerLine> LedgerLines;
	TArray<int32> PageRowNumbers;
	int32 KeyNumberStart = 1;
	/** The plate under the panel's contents, and the free art over it (the halo, the ribbon, the arc). */
	UPROPERTY(Transient) TObjectPtr<UOverlay> Plate;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> PlateArt;
	UPROPERTY(Transient) TObjectPtr<USettingsMenuArt> Ribbon;
	/** The halo's ring and gold arcs (they spin up as it opens), its pause bars; Resume's reticle (it turns while marked). */
	UPROPERTY(Transient) TObjectPtr<UImage> HaloRing;
	UPROPERTY(Transient) TObjectPtr<UImage> HaloGold;
	UPROPERTY(Transient) TObjectPtr<UWidget> PauseBars;
	UPROPERTY(Transient) TObjectPtr<UImage> ResumeReticle;
	/** The tabs' bar, the ruler under them and the gold mark that slides along it to the lit tab. */
	UPROPERTY(Transient) TObjectPtr<UOverlay> TabBar;
	UPROPERTY(Transient) TObjectPtr<UWidget> TabRuler;
	UPROPERTY(Transient) TObjectPtr<USettingsMenuArt> TabMark;
	/** The stamped plate beside the title ("NO. 01 · RANSOM'S REST") and the serial: over the game only. */
	UPROPERTY(Transient) TObjectPtr<USettingsMenuArt> AreaPlate;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SerialText;
	/** The row the ribbon marks (its number gold), the time since opening, the slides' places, the ribbon's fade. */
	TWeakObjectPtr<UWidget> MarkedRow;
	float OpenAge = 0.f;
	float RibbonY = 0.f;
	bool bRibbonPlaced = false;
	float RibbonOpacity = 0.f;
	float TabMarkX = 0.f;
	bool bTabMarkPlaced = false;
	float ReticleAngle = 0.f;

	/** Every described row and button (Describe), and the description column's three lines. */
	TArray<SettingsMenu::FHelpEntry> HelpEntries;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpBody;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HelpDetail;
	/** What the column shows: it's rewritten only when that changes. */
	SettingsMenu::FSettingHelp ShownHelp;

	/** Each key binding's Default button, beside its key. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> DefaultButtons;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> ResetAllButton;
	/** The key hints along the bottom ("A Select  B Back"). */
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
	/** The line over the key list: how to change a key, for the keyboard or a gamepad. */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyHintText;

	ESettingsMenuMode Mode = ESettingsMenuMode::Pause;

	/** The page on show's scrolling list (one of PageLists). */
	UPROPERTY(Transient) TObjectPtr<UScrollBox> Body;
	/** The key bindings' rows (RebuildControls fills it). */
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> KeyList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	/** The title: SETTINGS (the pause menu's list over it says GAME PAUSED; the prompts under the plate say how to go back). */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeaderText;
	/** The reticle row under the title: Back, to the pause menu's list or the main menu (GetResumeButton, its old name). */
	UPROPERTY(Transient) TObjectPtr<ULooterButton> ResumeButton;
	/** The footer's Back chip, kept hidden: going back is always the top row. */
	UPROPERTY(Transient) TObjectPtr<ULooterButton> BackButton;
	/** The reticle row's word: BACK. */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ResumeWord;
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
	/** The dimming over the game, and the panel in front of it: the field of view preview fades both. */
	UPROPERTY(Transient) TObjectPtr<UBorder> Dimmer;
	UPROPERTY(Transient) TObjectPtr<USizeBox> Panel;

	/**
	 * The previews, each up while its slider is held (or pointed at, for the minimap's two), then a moment after. The minimap's
	 * size and zoom sliders show in the same corner, so whichever is held or pointed at now takes it.
	 */
	SettingsMenu::FSliderPreview SizeState;
	SettingsMenu::FSliderPreview ZoomState;
	SettingsMenu::FSliderPreview FieldOfViewState;
	/** The field of view change shows in the view behind the menu (first person), so the menu steps aside while it is up. */
	bool bFieldOfViewInView = false;
	/** How far the menu has stepped aside (ApplyViewFade). */
	float AppliedViewFade = 0.f;
	/** Set while the sliders are given their saved values: that is not a move, and previews nothing. */
	bool bRefreshingSliders = false;
	TWeakObjectPtr<UPlayerViewComponent> ViewForPreview;

	/** The volume slider moved last, for the status line when it's let go. */
	EAudioVolume LastVolumeMoved = EAudioVolume::Master;

	/** Index into the subsystem's binding list we're waiting on a key for, or INDEX_NONE. */
	int32 ListeningIndex = INDEX_NONE;
};
