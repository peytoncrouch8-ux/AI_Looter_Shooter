#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Style/LooterUIFocus.h"
#include "PauseMenuWidget.generated.h"

class UCanvasPanel;
class UHorizontalBox;
class UImage;
class ULooterButton;
class UOverlay;
class USettingsMenuArt;
class USizeBox;
class UTextBlock;

/** The pause menu's rows, top to bottom (ItemOrder): Resume Game first and Save & Quit last, as the user asked. */
enum class EPauseMenuItem : uint8
{
	Resume,
	/** The inventory's pages, opened as their keys open them (Tab, M), the game still paused under them. */
	Inventory,
	Missions,
	Map,
	/** The Bestiary, the Ledger once Sexton has handed it over. */
	Bestiary,
	Embers,
	/** The settings menu, over the paused game; its Back comes back here. */
	Settings,
	SaveQuit,
	Count,
};

DECLARE_DELEGATE_OneParam(FOnPauseMenuChoice, EPauseMenuItem /*Item*/);

namespace PauseMenu
{
	/** The plate: narrower than the settings' (a list, no pages beside a description column), as tall as its rows want. */
	inline constexpr float PlateWidth = 900.f;
	inline constexpr float PlateHeight = 730.f;
}

/** A row of the pause menu's ledger: its button, focus look, number and words (PauseMenuPieces.cpp builds them). */
struct FPauseMenuLine
{
	EPauseMenuItem Item = EPauseMenuItem::Count;
	TWeakObjectPtr<ULooterButton> Button;
	/** Its gold wash and the ledger total's double rule, lit while it's the row marked. */
	TWeakObjectPtr<UWidget> Light;
	TWeakObjectPtr<UTextBlock> Number;
	TWeakObjectPtr<UTextBlock> Word;
	TWeakObjectPtr<UTextBlock> Detail;
};

/**
 * The pause menu (the user, 2026-10-29 night: "When I hit ESC in the game, it should pop up a menu that gives me options to
 * go to different menus. For example, settings. Hitting ESC shouldn't take me right to settings. Save and quit should be at
 * the bottom of these options and Resume Game at the top."). A short list over the paused game: RESUME GAME in c3's reticle
 * at the top, then the ledger's rows (Inventory, Missions, Map, Bestiary, Embers, Settings), then Save & Quit's rubber stamp
 * at the bottom. It says what was picked (OnChoice); ALooterHUD opens it, the screens it leads to, and brings it back when
 * they close (LooterHUDPause.cpp).
 *
 * Its look is the settings menu's round 3 (the user's mix of concepts 1 "Ledger & Brass" and 3 "Halo Reticle"): the slate
 * plate with its double salt trim, rivets and cut corners, the halo dial with the pause bars over its top-left corner, GAME
 * PAUSED over c3's dotted ruler with the area's stamped plate, the rows numbered in a ruled margin with c1's gold ribbon at
 * the row marked, the serial in the bottom-left (PauseMenuPieces.cpp). It drops in and its halo spins up as it opens
 * (PauseMenuMotion.cpp).
 *
 * The keyboard, a controller and the mouse drive it as they drive the settings menu (PauseMenuNav.cpp): up and down (the
 * D-pad, the stick, arrows, W S) round the ends, A or Enter picks, B, Esc or Start resume; the prompts under the plate name
 * the device last used, and the mouse points and clicks.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The rows, top to bottom. */
	static TConstArrayView<EPauseMenuItem> ItemOrder();

	/** The word a row shows (the bestiary's row: LEDGER once it's Sexton's book). */
	static FString ItemWord(EPauseMenuItem Item, bool bLedger);

	/**
	 * Shows it fresh: its words read again (the book's name, the area, the serial), the focus on Focus (Resume when it opens
	 * over the game; the row it left from when a screen it opened closes), the plate dropping in. Add it to the viewport after.
	 */
	void Open(EPauseMenuItem Focus = EPauseMenuItem::Resume);

	/** A row picked (clicked, or A / Enter on it), and Esc, B or Start: Resume. */
	FOnPauseMenuChoice OnChoice;

	/** A row's button (null for Count, or before it's built). */
	ULooterButton* GetItemButton(EPauseMenuItem Item) const;
	/** The word a row shows now. */
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
	/** The row with the focus (Count: none). */
	EPauseMenuItem GetFocusedItem() const;
	bool IsFocusLookShown() const;
	TArray<MenuNav::FPrompt> GetPrompts() const;

	/** The row the ribbon and the gold mark (Count: none yet). */
	EPauseMenuItem GetMarkedItem() const;

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
	// --- Building (the pieces: PauseMenuPieces.cpp; the rest PauseMenuWidget.cpp) ---

	UWidget* MakeHeader();
	UWidget* MakeResumeRow();
	/** A ledger row for Item, numbered Number in the margin, with its few words of what it opens at the right. */
	UWidget* MakeLedgerRow(EPauseMenuItem Item, int32 Number, const FString& Detail);
	UWidget* MakeFooter();
	/** The halo over the plate's top-left corner, the corner's range arc, the ribbon. */
	void MakePlateArt();
	ULooterButton* MakeItemButton(UWidget* Content, EPauseMenuItem Item);
	/** The words that change between visits: the book's name, the area's plate, the serial, the stamp's word. */
	void RefreshWords();
	/** What Open does once the widgets exist (RebuildWidget runs it for an Open before them). */
	void ApplyOpen();
	void HandleButton(ULooterButton* Button);
	void Choose(EPauseMenuItem Item);

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
	void AdvanceLook(float DeltaSeconds);
	/** The row to mark: the focus's under keys, the pointer's under the mouse; null to keep the one marked. */
	ULooterButton* FindMarkedButton() const;
	void MarkButton(ULooterButton* Button);
	const FPauseMenuLine* FindLine(const ULooterButton* Button) const;

	FMenuNavigator Nav;
	MenuNav::FRepeat NavRepeat;
	FVector2D NavStick = FVector2D::ZeroVector;
	MenuNav::EMove NavStickMove = MenuNav::EMove::None;
	LooterUI::EMenuDevice ShownDevice = LooterUI::EMenuDevice::Mouse;
	bool bNavLookApplied = false;
	FString ShownPrompts;

	/** Where the focus goes as it opens (Open before the widgets exist keeps it for RebuildWidget). */
	EPauseMenuItem OpenFocus = EPauseMenuItem::Resume;
	/** Save & Quit picked: the level is on its way out, nothing more is taken. */
	bool bLeaving = false;

	/** Every row with a button, Resume and Save & Quit included, in ItemOrder. */
	TArray<FPauseMenuLine> Lines;

	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> ItemButtons;
	UPROPERTY(Transient) TObjectPtr<UOverlay> Plate;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> PlateArt;
	UPROPERTY(Transient) TObjectPtr<USizeBox> Panel;
	UPROPERTY(Transient) TObjectPtr<USettingsMenuArt> Ribbon;
	UPROPERTY(Transient) TObjectPtr<UImage> HaloRing;
	UPROPERTY(Transient) TObjectPtr<UImage> HaloGold;
	UPROPERTY(Transient) TObjectPtr<UImage> ResumeReticle;
	UPROPERTY(Transient) TObjectPtr<USettingsMenuArt> AreaPlate;
	UPROPERTY(Transient) TObjectPtr<USettingsMenuArt> Stamp;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SerialText;
	/** The hollow keys under the plate. */
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;

	/** The row the ribbon marks, the time since opening, the ribbon's place and fade, the reticle's turn. */
	TWeakObjectPtr<ULooterButton> MarkedButton;
	float OpenAge = 0.f;
	float RibbonY = 0.f;
	bool bRibbonPlaced = false;
	float RibbonOpacity = 0.f;
	float ReticleAngle = 0.f;
};
