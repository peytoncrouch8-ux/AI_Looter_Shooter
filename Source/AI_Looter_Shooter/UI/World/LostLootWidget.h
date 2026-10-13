#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Loot/LostLootStore.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Style/LooterUIFocus.h"
#include "LostLootWidget.generated.h"

class ALostPropertyWindow;
class APlayerController;
class UHorizontalBox;
class UImage;
class ULooterButton;
class ULostLootSubsystem;
class UOverlay;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWalletSubsystem;
class UWeaponManagerComponent;
class UWidget;

/** Lost Property's screen's measures, at the kit's sizes (CSS px as Slate sizes, the user's call). */
namespace LostLootLayout
{
	/** The two columns and the gap between them, and the frame's padding round them. */
	inline constexpr float PanelWidth = 1144.f;
	inline constexpr float ListWidth = 600.f;
	inline constexpr float CardWidth = 470.f;
	inline constexpr float ColumnGap = 26.f;
	/** The ticket list scrolls past this height, and the card past CardHeight, so 80 guns still fit a small screen. */
	inline constexpr float ListHeight = 430.f;
	inline constexpr float CardHeight = 470.f;
	inline constexpr float RowHeight = 64.f;
	inline constexpr float RowGap = 6.f;
	/** The ticket's stub: its number, the punched hole and the perforation it tears along. */
	inline constexpr float StubWidth = 78.f;
	inline constexpr float ConfirmWidth = 540.f;
	/** Page Up / Down and the bumpers move this many tickets. */
	inline constexpr int32 PageStep = 6;
}

/** What Lost Property's screen's buttons do (ULooterButton::Action). */
namespace LostLootActions
{
	inline const FName Ticket(TEXT("LostTicket"));
	inline const FName Claim(TEXT("LostClaim"));
	inline const FName Upgrade(TEXT("LostUpgrade"));
	inline const FName Close(TEXT("LostClose"));
	inline const FName Accept(TEXT("LostAccept"));
	inline const FName Back(TEXT("LostBack"));
}

/**
 * Lost Property's screen at the depot's window (ALostPropertyWindow; the user's call, 2026-10-13), one LooterUI panel over
 * the dimmed world laid out like a railway baggage office's counter:
 *  - top: the station agent's notice on the left; on the right the shelves' count (12 / 30) over a bar that fills toward
 *    full (red when it is), and the player's silver;
 *  - left: every gun on the shelves as a claim ticket, newest first: its stub (the ticket's number, a punched hole, the
 *    perforation), the gun's picture, its name in its rarity's colour, level, rarity and kind, where and when it was found;
 *    the one to go next is marked NEXT TO GO (the oldest, never a named gun while another is there), in red once the shelves
 *    are full; under the list, MORE SHELVING (its name, +10 spaces, its price) or the shelves at their most;
 *  - right: the chosen gun's card as the inventory shows one (picture, name, level, rarity, its verdict against the gun in
 *    hand, damage and the first-glance stats, its parts slot by slot, its notches and curse), and CLAIM with where it goes
 *    (the backpack, else a free slot, else "no room");
 *  - a status line saying what the last claim or purchase did, and the prompts for the keyboard or the gamepad.
 * Claiming stamps CLAIMED over the card (LooterSoundCue::LostLoot::Claim) and the gun is the player's; buying shelving asks
 * first. Keys: W / S, the arrows, the D-pad and the stick choose; Page Up / Down or the bumpers a page at a time; E (Interact),
 * Enter, Space or A claims; U or Y buys shelving; Esc or B backs out of the confirm, then closes. The mouse chooses by
 * hovering, claims and buys with the buttons, and scrolls. It holds the game's input but doesn't pause; it closes itself
 * when the player walks away from the window or falls. Every background follows the UI transparency setting.
 *
 * LostLootWidgetOpen.cpp: opening and closing, staying at the window, what it reads. LostLootWidget.cpp: the panel, choosing,
 * the purse, the motion. LostLootWidgetRows.cpp: the header, the tickets, the shelving row. LostLootWidgetCard.cpp: the
 * chosen gun's card and the stamp. LostLootWidgetActions.cpp: claiming, buying, the confirm, words and sounds.
 * LostLootWidgetInput.cpp: keys, the stick, the mouse and the prompts.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULostLootWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Opens Window's screen for Player (closing one already up). Null when it can't (no local player, no pawn). */
	static ULostLootWidget* OpenFor(APlayerController* Player, ALostPropertyWindow* Window);

	/** The screen up now, if any (ALooterHUD::IsMenuOpen counts it). */
	static ULostLootWidget* GetOpen();

	/**
	 * Fills the screen for Manager's guns at Window (either may be null: the console opens it with no window, a test with
	 * no player). OpenFor calls it; a test calls it on a widget it made, then TakeWidget.
	 */
	void Open(ALostPropertyWindow* InWindow, UWeaponManagerComponent* InManager);

	/** Closes it: the game has its keys and its mouse back, and the shutter comes down. */
	void Close();

	/** Claims the chosen ticket's gun: into the backpack, else a free slot. False (and the reason said) when it can't. */
	bool Claim();

	/** Buys the next shelving: with bConfirmed it's bought now, else the confirm asks first. Whether it was bought. */
	bool BuyUpgrade(bool bConfirmed);

	/** A key pressed in the screen, as NativeOnKeyDown takes it (the tests press keys this way). Whether it was used. */
	bool HandleKey(const FKey& Key, bool bRepeat = false);

	/** Moves its motion on (the stamp, the purse); the tick does it, tests call it. */
	void Advance(float DeltaSeconds);

	/** What the screen shows, for the tests. */
	struct FView
	{
		/** The tickets in the list's order (newest first). */
		TArray<int32> Tickets;
		/** The chosen ticket, or INDEX_NONE. */
		int32 Chosen = INDEX_NONE;
		/** The ticket marked NEXT TO GO, or INDEX_NONE (empty shelves); bFull: the shelves are full, so it's red. */
		int32 NextToGo = INDEX_NONE;
		bool bFull = false;
		/** "12 / 30". */
		FString Count;
		/** The next shelving's price, 0 when all are bought. */
		int32 UpgradePrice = 0;
		bool bConfirmOpen = false;
		bool bStamping = false;
		FString Status;
		/** "E: Claim" and so on, the main one first. */
		TArray<FString> Prompts;
	};
	FView GetView() const;

	/** The player walks this far (cm) from the window and the screen closes. */
	static constexpr float StayRadius = 450.f;

	/** CLAIMED: slammed down, held, lifting away; then the list is read again. */
	static constexpr float StampSlamSeconds = 0.12f;
	static constexpr float StampHoldSeconds = 0.45f;
	static constexpr float StampOutSeconds = 0.18f;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
		const FNavigationReply& InDefaultReply) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

private:
	/** One ticket in the list: its gun's ticket number and the parts that restyle as it's chosen. */
	struct FRow
	{
		int32 Ticket = 0;
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
	};

	// --- LostLootWidget.cpp: the panel, choosing, the purse and the motion ---

	/** Reads the shelves again and builds the panel, keeping the chosen ticket where it can. */
	void Refresh();
	void BuildPanel();
	/** Chooses the list's row at Index: its card shows; bScroll brings it into view (keys), bSound ticks. */
	void Select(int32 Index, bool bSound, bool bScroll);
	/** Moves the choice by RowsToMove, stopping at the ends. */
	void Step(int32 RowsToMove);
	void Restyle();
	void SetStatus(const FText& Message, const FLinearColor& TextColor);
	void UpdatePurse(float DeltaTime);
	/** The shelves changed by themselves (a gun handed in while the screen is up). */
	void HandleStoreChanged();

	// --- LostLootWidgetOpen.cpp: staying at the window, what it reads ---

	/** The player can't stay at the window any more (walked off, fallen, the window gone). */
	bool ShouldClose() const;
	UWeaponManagerComponent* GetManager() const;
	UWalletSubsystem* GetWallet() const;
	ULostLootSubsystem* GetLostLoot() const;
	int32 GetPlayerLevel() const;
	/** The chosen ticket's entry on the shelves, or null. */
	const FLostLootEntry* GetChosenEntry() const;

	// --- LostLootWidgetRows.cpp: the header, the tickets, the shelving row ---

	UWidget* MakeHeader();
	UWidget* MakeShelfMeter(int32 Count, int32 Capacity);
	/** A ticket in the list; bNextToGo marks the one that makes room next, in red while the shelves are full (bFull). */
	UWidget* MakeTicketRow(int32 RowIndex, const FLostLootEntry& Entry, bool bNextToGo, bool bFull);
	UWidget* MakeStub(const FLostLootEntry& Entry);
	UWidget* MakeUpgradeRow();
	/** "12 min ago", "just now", or empty when the clock can't say. */
	FText AgoWords(double HandedInAt) const;

	// --- LostLootWidgetCard.cpp: the chosen gun's card and the stamp ---

	/** Puts the chosen gun's card up (or the shelves' empty card), unless the stamp holds the claimed one there. */
	void RefreshCard();
	void FillCard(const FLostLootEntry& Entry);
	void FillEmptyCard();
	UWidget* MakeClaimBar(const FLostLootEntry& Entry);
	UWidget* MakeStamp();
	/** Puts the stamp on screen as its motion stands. */
	void ApplyStamp();

	// --- LostLootWidgetActions.cpp: claiming, buying, the confirm, words and sounds ---

	void OpenConfirm(const FString& Title, const FString& Body, const FText& AcceptLabel);
	void CloseConfirm();
	void AcceptConfirm();
	void PlayCue(const TCHAR* Cue, float Volume = 1.f) const;

	// --- LostLootWidgetInput.cpp: keys, the stick, the mouse and the prompts ---

	void HandleButton(ULooterButton* Button);
	void HandleHovered(ULooterButton* Button);
	/** The prompts for the device last used. */
	void RefreshPrompts();
	/** A key in the screen: notes the device. */
	void NoteKey(const FKey& Key);
	void NotePointer();
	/** The words the prompts say, the main one first ("E: Claim"). */
	TArray<FString> PromptWords() const;

	TWeakObjectPtr<ALostPropertyWindow> Window;
	/** Opened at a window (not the console's): it closes when the player walks away from it, or it's gone. */
	bool bAtWindow = false;
	TWeakObjectPtr<APlayerController> Player;
	TWeakObjectPtr<UWeaponManagerComponent> Manager;
	TWeakObjectPtr<ULostLootSubsystem> BoundLostLoot;
	FDelegateHandle StoreChangedHandle;

	TArray<FRow> Rows;
	int32 Selected = INDEX_NONE;
	/** The ticket shown on the card now (the stamp keeps the claimed one there until it lifts). */
	int32 CardTicket = INDEX_NONE;

	bool bConfirmOpen = false;
	/** The store changed while the confirm or the stamp was up: read again once they're gone. */
	bool bRefreshWanted = false;

	/** CLAIMED: whether it's on, how far along (s), and the gun it was stamped on (kept for the card while it shows). */
	bool bStamping = false;
	float StampAge = 0.f;
	UPROPERTY(Transient)
	FLostLootEntry Stamped;

	/** The purse as shown: it counts to the silver carried, flashing up (cyan) or down (red), and pops. */
	float ShownSilver = 0.f;
	int32 PurseTarget = 0;
	float PurseFlash = 0.f;
	bool bPurseGain = false;

	/** The stick held as a D-pad, and its repeats while held. */
	FVector2D NavStick = FVector2D::ZeroVector;
	MenuNav::EMove NavStickMove = MenuNav::EMove::None;
	MenuNav::FRepeat StickRepeat;
	LooterUI::EMenuDevice ShownDevice = LooterUI::EMenuDevice::Mouse;

	FText StatusWords;
	FLinearColor StatusColor = FLinearColor::White;

	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UWidget> Page;
	UPROPERTY(Transient) TObjectPtr<UOverlay> PopupLayer;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ListScroll;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> CardBox;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> CardScroll;
	UPROPERTY(Transient) TObjectPtr<UWidget> StampWidget;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StampTicketText;
	UPROPERTY(Transient) TObjectPtr<USizeBox> ClaimHolder;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PurseText;
	UPROPERTY(Transient) TObjectPtr<UWidget> PurseLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
};
