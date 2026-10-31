#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateBrush.h"
#include "Types/SlateEnums.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Style/LooterUIFocus.h"
#include "PauseMenuWidget.generated.h"

class UCanvasPanel;
class UCanvasPanelSlot;
class UHorizontalBox;
class UImage;
class ULooterButton;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;
namespace LooterUI { struct FPaintedIcon; }

/** The pause menu's options, top to bottom (ItemOrder; the user, 2026-10-31: only what the game has, nothing of the
 *  inventory's tabs). */
enum class EPauseMenuItem : uint8
{
	/** BACK TO GAME: closes the menu. */
	Resume,
	/** The settings page, full screen; its Back comes back here with SETTINGS lit. */
	Settings,
	/** Saves the session and goes back to the main menu. */
	SaveQuit,
	/** Saves the session and closes the game. */
	QuitGame,
	Count,
};

DECLARE_DELEGATE_OneParam(FOnPauseMenuChoice, EPauseMenuItem /*Item*/);

/** One option's pieces on the hub (PauseMenuPieces.cpp builds them; PauseMenuMotion.cpp lights and moves them). */
struct FPauseMenuOption
{
	EPauseMenuItem Item = EPauseMenuItem::Count;
	int32 Row = 0;
	/** QUIT GAME reads red until lit. */
	bool bDanger = false;
	TWeakObjectPtr<ULooterButton> Button;
	/** Everything on its row (the dash, pegs, number, tag): it slides in as the hub opens. */
	TWeakObjectPtr<UWidget> RowGroup;
	/** The tag's two looks (each its glass and lines) and the gem's, crossfaded as it lights. */
	TWeakObjectPtr<UWidget> PlainLook;
	TWeakObjectPtr<UWidget> LitLook;
	TWeakObjectPtr<UWidget> PlainGem;
	TWeakObjectPtr<UWidget> LitGem;
	TWeakObjectPtr<UImage> Icon;
	TWeakObjectPtr<UTextBlock> Word;
	TWeakObjectPtr<UTextBlock> Number;
	/** The salt dash across the arc (gone while lit), the short peg past it, the long peg out to the tag (each over its
	 *  dark edge). */
	TWeakObjectPtr<UWidget> Dash;
	TWeakObjectPtr<UImage> ShortPeg;
	TWeakObjectPtr<UImage> LongPeg;
	TWeakObjectPtr<UWidget> LongPegEdge;
	/** The callout beside it while it's lit, and its flavor line (it says what a save does, or that nothing is saved). */
	TWeakObjectPtr<UWidget> Callout;
	TWeakObjectPtr<UTextBlock> CalloutFlavor;
	float CalloutHeight = 0.f;
	/** The navigator's focus light: an empty box (the lit tag itself shows the focus). */
	TWeakObjectPtr<UWidget> FocusLight;
	/** How lit it is now, 0 to 1 (eased toward 1 while it's the option marked). */
	float Lit = 0.f;
};

/** The hero card's live pieces (PauseMenuCards.cpp). */
struct FPauseHeroCard
{
	TWeakObjectPtr<UTextBlock> Level;
	TWeakObjectPtr<UTextBlock> Label;
	TWeakObjectPtr<UTextBlock> Name;
	TWeakObjectPtr<UTextBlock> Health;
	TWeakObjectPtr<UTextBlock> HealthMax;
	TWeakObjectPtr<UTextBlock> XP;
	TWeakObjectPtr<UTextBlock> XPNext;
	TWeakObjectPtr<UTextBlock> Flask;
	TWeakObjectPtr<UTextBlock> Grenades;
	TArray<TWeakObjectPtr<UImage>> FlaskPips;
	TArray<TWeakObjectPtr<UImage>> GrenadePips;
	TWeakObjectPtr<USizeBox> XPFill;
};

/** The tracked-mission card's live pieces. */
struct FPauseMissionCard
{
	TWeakObjectPtr<USizeBox> Box;
	TWeakObjectPtr<UImage> Glass;
	TWeakObjectPtr<UImage> Lines;
	TWeakObjectPtr<UTextBlock> Count;
	TWeakObjectPtr<UTextBlock> Name;
	TWeakObjectPtr<UTextBlock> Line;
	TWeakObjectPtr<UHorizontalBox> Steps;
	TWeakObjectPtr<UVerticalBox> Optionals;
	float Height = 0.f;
};

/** The session tag's live pieces. */
struct FPauseSessionCard
{
	TWeakObjectPtr<UTextBlock> Gem;
	TWeakObjectPtr<UTextBlock> Title;
	TWeakObjectPtr<UTextBlock> Of;
	TWeakObjectPtr<UTextBlock> Serial;
	TWeakObjectPtr<UTextBlock> PlayedLabel;
	TWeakObjectPtr<UTextBlock> Played;
	TWeakObjectPtr<UTextBlock> SavedLabel;
	TWeakObjectPtr<UTextBlock> Saved;
};

/**
 * The pause menu (Esc), full screen over the paused game: E2 "Big Dial" (the user's pick, 2026-10-30/31; contract
 * Docs/Archive/Work/2026-10-30-PauseMenu/CONTRACT.md, picture mockups/E2b_hub.png). The user: "the ESC menu should encompass the
 * entire screen and the options should be on the left hand side ... Esc Menu should not have whats in the inventory tabs."
 *
 * The game dims under it, darker to the left. GAME PAUSED top-left over a ruler and the area's name. A big dial cut by the
 * screen's left edge (the main menu's ring, no sun: a fine inner ring and a slim gold needle) with a range arc whose ticks
 * feed the four options' tags by pegs: BACK TO GAME, SETTINGS, SAVE & QUIT, QUIT GAME (red). The lit tag turns gold and
 * slides out, the needle swings to its tick (PauseMenuLayout::NeedleSwing), the small reticle sits on it, and a callout
 * beside it says what it does. Top-right the hero card (portrait, level, health, experience, soul flask, grenades, the
 * experience bar) and the tracked mission; bottom-left the session tag; bottom-right the key hints.
 *
 * It says what was picked (OnChoice); ALooterHUD acts on it (LooterHUDPause.cpp). Driven by the keyboard, a controller and
 * the mouse as the settings are (PauseMenuNav.cpp): up and down round the ends, A or Enter picks, B, Esc or Start resume;
 * the mouse lights what it points at and clicks. Built in PauseMenuWidget.cpp, PauseMenuPieces.cpp (title, dial, options,
 * callouts, keys) and PauseMenuCards.cpp (the cards); it moves in PauseMenuMotion.cpp; its layout and rules are
 * PauseMenuLayout, its pictures PauseMenuArt and PauseMenuIcons.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The options, top to bottom. */
	static TConstArrayView<EPauseMenuItem> ItemOrder();

	/** An option's words on its tag ("BACK TO GAME"). */
	static FString ItemWord(EPauseMenuItem Item);

	/** The callout's line for an option (what it does), and its gold flavor line (bSession: a session is being played, so
	 *  its saves are real; without one nothing is saved, and the flavor says so). */
	static FString ItemHelp(EPauseMenuItem Item);
	static FString ItemFlavor(EPauseMenuItem Item, bool bSession);

	/** The option saves the session before it leaves (Save & Quit, Quit Game). */
	static bool SavesFirst(EPauseMenuItem Item);

	/**
	 * Shows it fresh: its words and cards read again (the area, the hero, the mission, the session), the focus on Focus
	 * (Resume when it opens over the game; Settings when the settings page closes), and its opening motion: whole over the
	 * game, a quick fade when bReturning from a page it opened. Add it to the viewport after.
	 */
	void Open(EPauseMenuItem Focus = EPauseMenuItem::Resume, bool bReturning = false);

	/** An option picked (clicked, or A / Enter on it), and Esc, B or Start: Resume. */
	FOnPauseMenuChoice OnChoice;

	/** An option's button (null for Count, or before it's built). */
	ULooterButton* GetItemButton(EPauseMenuItem Item) const;
	/** The words an option's tag shows now. */
	FString GetShownWord(EPauseMenuItem Item) const;

	// --- The keyboard and a controller (PauseMenuNav.cpp) ---

	/**
	 * A key from the keyboard or a gamepad (every key comes here, and the tests send theirs): up and down move the focus round
	 * the ends, A or Enter picks, B, Esc or Start resume. The first move or press while the mouse drove the menu only brings
	 * the focus look up. bRepeat: the platform's repeat of a held key, ignored (the Esc that opened it may still be down).
	 * True when it was the menu's.
	 */
	bool HandleNavKey(const FKey& Key, bool bRepeat = false);
	void HandleNavKeyUp(const FKey& Key);
	/** The left stick's position (-1 to 1, up +Y), read as a D-pad. */
	void HandleNavStick(const FVector2D& Stick);
	/** A frame of held keys' repeats, the device check and the focus look's breath (NativeTick; tests step it). */
	void AdvanceNav(float DeltaSeconds);
	UWidget* GetFocusedWidget() const;
	/** The option with the focus (Count: none). */
	EPauseMenuItem GetFocusedItem() const;
	bool IsFocusLookShown() const;
	TArray<MenuNav::FPrompt> GetPrompts() const;

	// --- The look (PauseMenuMotion.cpp) ---

	/** A frame of the motion: the opening, the lit tag, the needle's swing (NativeTick; tests step it). */
	void AdvanceLook(float DeltaSeconds);
	/** The option lit gold, the needle aimed at it (Count: none yet). */
	EPauseMenuItem GetMarkedItem() const;
	/** Where the needle points now, and where it's swinging to (degrees, PauseMenuLayout::RowDegrees). */
	float GetNeedleDegrees() const { return NeedleNow; }
	float GetNeedleTarget() const { return NeedleTo; }
	/** The tracked mission's card is up (hidden with no mission tracked). */
	bool IsMissionCardShown() const;
	/** The session tag's two lines as shown ("SESSION 1 OF 3 | Played 3 h 05 min Saved Today 14:32"), for tests. */
	FString GetSessionText() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FCursorReply NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent) override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
		const FNavigationReply& InDefaultReply) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

private:
	// --- Building: shared helpers (PauseMenuWidget.cpp) ---

	/** A picture of the kit's painter as a brush at its own size; its texture's name is Base and the picture's size, so two
	 *  sizes of one shape never share a texture. Resolution 0: PauseMenuLayout::PictureResolution. */
	FSlateBrush PictureBrush(const TCHAR* Base, const LooterUI::FPaintedIcon& Picture, float Resolution = 0.f) const;
	/** The same as an image (it ignores the mouse). */
	UImage* MakePicture(const TCHAR* Base, const LooterUI::FPaintedIcon& Picture, float Resolution = 0.f);
	/** Content centred up and down in a box Height tall (Width 0: the content's own), aligned across as asked. */
	USizeBox* MakeBand(UWidget* Content, float Width, float Height, EHorizontalAlignment Across = HAlign_Left);
	/** Puts Widget on Canvas at Position, Size big (zero: its own size); bRight keeps it to the canvas's right edge. */
	UCanvasPanelSlot* Put(UCanvasPanel* Canvas, UWidget* Widget, const FVector2D& Position, const FVector2D& Size, bool bRight = false);
	/** A canvas of its own on the stage, Size big at Position (a group that moves as one). */
	UCanvasPanel* MakeGroup(const FVector2D& Position, const FVector2D& Size, bool bRight = false);
	/** A shape's glass (a background) and its lines, both at -Pad round a box Size big, in a group of their own. */
	UWidget* MakeShape(const TCHAR* Base, const LooterUI::FPaintedIcon& Glass, const LooterUI::FPaintedIcon& Lines, const FVector2D& Size);

	// --- Building: the pieces (PauseMenuPieces.cpp) ---

	void MakeTitle();
	void MakeDial();
	void MakeOption(EPauseMenuItem Item, int32 Row);
	void MakeCallout(FPauseMenuOption& Option);
	void MakeKeys();

	// --- The cards (PauseMenuCards.cpp) ---

	void MakeHeroCard();
	void MakeMissionCard();
	void MakeSessionCard();
	/** Reads them again: the hero, the tracked mission (its card fitted to its words), the session. */
	void RefreshCards();
	void RefreshHero();
	void RefreshMission();
	void RefreshSession();

	// --- Words and picking (PauseMenuWidget.cpp) ---

	/** The words that change between visits: the area's name, the flavor lines (a session or none), the options' words. */
	void RefreshWords();
	/** What Open does once the widgets exist (RebuildWidget runs it for an Open before them). */
	void ApplyOpen();
	/** Keeps the stage 1080 tall and at least 1920 wide for the screen's shape. */
	void FitStage();
	void HandleButton(ULooterButton* Button);
	void Choose(EPauseMenuItem Item);
	FPauseMenuOption* FindOption(EPauseMenuItem Item);
	const FPauseMenuOption* FindOption(EPauseMenuItem Item) const;
	const FPauseMenuOption* FindOption(const ULooterButton* Button) const;

	// --- The keyboard and a controller (PauseMenuNav.cpp) ---

	void RebuildNavigation(const UWidget* Prefer);
	void PressNav(const FKey& Key, MenuNav::EMove Move);
	void StepNav(MenuNav::EMove Move);
	void ActivateNav();
	/** Notes Key's device. False when the focus look wasn't showing before (that first press only brings it up). */
	bool RevealNav(const FKey& Key);
	void RefreshNavLook();
	void RefreshPrompts();
	void StopNavHold();
	void NotePointer();
	void HandleButtonHovered(ULooterButton* Button);

	// --- The motion (PauseMenuMotion.cpp) ---

	void BeginOpenMotion();
	/** The option to mark: the focus's under keys, the pointer's under the mouse; null to keep the one marked. */
	ULooterButton* FindMarkedButton() const;
	void MarkButton(ULooterButton* Button);
	/** Sets an option's pieces for how lit it is (its tag, gem, words, pegs, number, callout). */
	void ApplyLit(FPauseMenuOption& Option);
	/** The needle and the reticle at the needle's angle now. */
	void ApplyNeedle();

	FMenuNavigator Nav;
	MenuNav::FRepeat NavRepeat;
	FVector2D NavStick = FVector2D::ZeroVector;
	MenuNav::EMove NavStickMove = MenuNav::EMove::None;
	LooterUI::EMenuDevice ShownDevice = LooterUI::EMenuDevice::Mouse;
	bool bNavLookApplied = false;
	FString ShownPrompts;

	/** Where the focus goes as it opens, and whether it comes back from a page (Open before the widgets exist keeps them). */
	EPauseMenuItem OpenFocus = EPauseMenuItem::Resume;
	bool bOpenReturning = false;
	/** Save & Quit or Quit Game picked: the level is on its way out, nothing more is taken. */
	bool bLeaving = false;

	TArray<FPauseMenuOption> Options;
	FPauseHeroCard Hero;
	FPauseMissionCard Mission;
	FPauseSessionCard Session;

	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> ItemButtons;
	/** The dimmed game behind it all (a background). */
	UPROPERTY(Transient) TObjectPtr<UImage> Backdrop;
	/** The design space (PauseMenuLayout), fitted to the screen, and the canvas everything stands on. */
	UPROPERTY(Transient) TObjectPtr<USizeBox> StageBox;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Stage;
	/** The groups that move as the hub opens. */
	UPROPERTY(Transient) TObjectPtr<UWidget> TitleGroup;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(Transient) TObjectPtr<UWidget> TitleRuler;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> AreaText;
	UPROPERTY(Transient) TObjectPtr<UWidget> DialGroup;
	UPROPERTY(Transient) TObjectPtr<UImage> Needle;
	UPROPERTY(Transient) TObjectPtr<UImage> Reticle;
	UPROPERTY(Transient) TObjectPtr<UWidget> HeroGroup;
	UPROPERTY(Transient) TObjectPtr<UWidget> MissionGroup;
	UPROPERTY(Transient) TObjectPtr<UWidget> SessionGroup;
	UPROPERTY(Transient) TObjectPtr<UWidget> KeysGroup;
	/** The key hints, bottom-right. */
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;

	/** The option marked, the time since opening, the needle's swing (from, to, its time and length), the reticle's turn. */
	TWeakObjectPtr<ULooterButton> MarkedButton;
	float OpenAge = 0.f;
	float NeedleFrom = 0.f;
	float NeedleTo = 0.f;
	float NeedleNow = 0.f;
	float SwingAge = 0.f;
	float SwingLength = 0.f;
	float SwingDelay = 0.f;
	float ReticleAngle = 0.f;
	/** The stage's width now (PauseMenuLayout::DesignWidth or wider). */
	float StageWidth = 0.f;
};
