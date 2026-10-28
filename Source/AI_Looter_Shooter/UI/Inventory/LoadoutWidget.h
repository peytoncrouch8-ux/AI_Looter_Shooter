#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Cards/LooterCardBackdrop.h"
#include "UI/Inventory/LoadoutGunStage.h"
#include "UI/Inventory/LoadoutRules.h"
#include "Weapons/AmmoTypes.h"
#include "LoadoutWidget.generated.h"

class ALooterHUD;
class FSlateWindowElementList;
class UBorder;
class UCanvasPanel;
class UCanvasPanelSlot;
class UGunCardWidget;
class UHorizontalBox;
class UImage;
class ULoadoutPaintLayer;
class ULooterButton;
class UMaterialInstanceDynamic;
class UScrollBox;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UPlayerShieldComponent;
class UWeaponManagerComponent;
struct FShieldInstanceData;
struct FWeaponInstanceData;

/** The loadout page's layout, on the inventory's 1600 x 900 page (the user's 2026-10-28 ask: the backpack back on the right). */
namespace LoadoutLayout
{
	/** Left: EQUIPPED (a row per slot), WARD, then AMMO. */
	inline constexpr float ListX = 56.f;
	inline constexpr float ListWidth = 488.f;
	/** Right: the BACKPACK, a row per place in it (empty ones faint), scrolling when they don't fit. */
	inline constexpr float BackpackX = 1056.f;
	inline constexpr float BackpackWidth = 488.f;
	inline constexpr float ColumnTop = 104.f;
	inline constexpr float ColumnBottom = 836.f;
	/** A section's header line, the gap under it, and the gap over a section after the first. */
	inline constexpr float HeaderHeight = 22.f;
	inline constexpr float HeaderGap = 8.f;
	inline constexpr float SectionGap = 20.f;
	inline constexpr float SlotRowHeight = 72.f;
	/** The ward worn's row; the spares' are BackpackRowHeight. */
	inline constexpr float WardRowHeight = 56.f;
	inline constexpr float BackpackRowHeight = 46.f;
	inline constexpr float RowGap = 6.f;
	/** Where both columns' first row starts: A / D keep the nearest row by where the rows are laid out from here. */
	inline constexpr float FirstRowTop = ColumnTop + HeaderHeight + HeaderGap;
	/** Middle: the showcase, the gun under the cursor turning, with its card under it. */
	inline const FVector2D ShowcaseTopLeft(550.f, 96.f);
	inline const FVector2D ShowcaseSize(500.f, 500.f * ALoadoutGunStage::ImageHeight / ALoadoutGunStage::ImageWidth);
	/** The middle card (the gun card, or a ward's or an empty slot's card): its top; centred on the page. */
	inline constexpr float CardTop = 440.f;
	inline constexpr float CardWidth = 420.f;
	/** Inspect: the lists put away, the gun large on the left, every stat on a card in the backpack's place. */
	inline const FVector2D InspectShowcaseTopLeft(56.f, 150.f);
	inline const FVector2D InspectShowcaseSize(980.f, 980.f * ALoadoutGunStage::ImageHeight / ALoadoutGunStage::ImageWidth);
	inline constexpr float InspectCardX = BackpackX;
	inline constexpr float InspectCardWidth = BackpackWidth;
	/** The key hints, centered at the bottom. */
	inline const FVector2D PromptBarPosition(800.f, 852.f);
	/** A plain card's contents slide in over this long; a gun put into a row flashes for this long. */
	inline constexpr float CardIntroSeconds = 0.16f;
	inline constexpr float FlashSeconds = 0.55f;
	/** The card's open sound plays at most this often (browsing fast would rattle it). */
	inline constexpr double CardOpenCueGap = 0.12;
}

/**
 * The inventory's first page, the loadout (the 2026-10-08 redesign, laid out again 2026-10-28: the user wanted the
 * backpack back on the right). Two columns of rows in the "Wide Sweep" row look (ULooterCardBackdrop), the gun under the
 * cursor between them:
 *  - left: EQUIPPED (a row per equip slot: its number, lit while in hand, the gun's icon, name in its rarity's colour,
 *    kind, level and damage), the WARD (the ward worn, then the spares; LoadoutWidgetWard.cpp), then the ammo and silver
 *  - right: the BACKPACK (a row per gun: icon, name, level, damage, an arrow against the gun it would replace; sorted
 *    best-for-slot, by rarity, level or newest with R), then a faint EMPTY row per free place
 *  - middle: the gun under the cursor, built from its parts, turning slowly (drag to turn, click to inspect), and under it
 *    its gun card (UGunCardWidget, Loadout: compared with the gun of the slot it would go to). A ward or an empty slot
 *    gets a plain card there instead
 *  - Inspect (X, or a click on the gun) hides the lists, shows the gun large, and every stat, the parts and the history
 *    on a card wearing its maker's sweep where the backpack was
 *  - bottom: what the keys do right now; each hint is also a button
 * W / S move within a column (the slots then the wards; the backpack), A / D switch columns to the nearest row. E on a
 * slot picks it as the swap target and goes to the backpack; E on a backpack gun swaps it into the target slot (or equips
 * it into a free one). F takes a gun in hand, M moves a slot (then E puts it down), C stows a slot's gun in the backpack,
 * Q drops, R sorts, X inspects. Guns can also be dragged: onto a slot, a backpack gun, the backpack, or the showcase (in
 * hand). Esc backs out (a drag, a move, Inspect) then closes; Tab and I close; the number keys (or the tabs) turn pages.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULoadoutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(ALooterHUD* InHUD, UWeaponManagerComponent* InManager);
	void Close();

	/** A key as the screen takes it (every key press comes here; tests press keys with it). False when it isn't used. */
	bool HandleKey(const FKey& Key);

	/** What the screen shows, for tests. */
	struct FView
	{
		/** The cursor is on an equip slot (else on a ward's row or a backpack row). */
		bool bOnSlot = true;
		/** The cursor is on a ward's row (Cursor 0 the ward worn, then the spares). */
		bool bOnWard = false;
		int32 Cursor = 0;
		/** The slot a backpack gun goes into. */
		int32 TargetSlot = 0;
		bool bInspecting = false;
		LoadoutRules::ESort Sort = LoadoutRules::ESort::Match;
		/** Backpack indices, in the list's order. */
		TArray<int32> ListOrder;
		/** What the prompt bar says, "E: Swap into slot 1" and so on, in order. */
		TArray<FString> Prompts;
		/** For each backpack row, in the list's order: it carries the JUNK tag (its gun is marked junk). */
		TArray<bool> JunkRows;
		/** Where the columns stand on the page (their left edge), and the backpack's faint EMPTY rows. */
		float EquippedX = 0.f;
		float BackpackX = 0.f;
		int32 EmptyBackpackRows = 0;
		/** The middle gun card: shown, the gun's name on it and its verdict ("UPGRADE", or empty). */
		bool bGunCard = false;
		FString CardName;
		FString CardVerdict;
	};
	FView GetView() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;

private:
	/** Where the cursor is: the left column's slots or wards, or the right column's backpack. */
	enum class EZone : uint8 { Slots, Backpack, Wards };

	/** Where a dragged gun can be let go. */
	enum class EDropKind : uint8 { None, Slot, BackpackRow, Backpack, Stage };
	struct FDropTarget
	{
		EDropKind Kind = EDropKind::None;
		/** The slot, or the backpack list's row. */
		int32 Index = INDEX_NONE;
		bool operator==(const FDropTarget& Other) const { return Kind == Other.Kind && Index == Other.Index; }
	};

	/** What the key hints do when clicked (their buttons' index), and what the keys call. None: a hint that only informs. */
	enum class EAction : uint8 { Activate, Hold, Move, Stow, Drop, Inspect, Sort, Close, Cancel, None };

	/** One prompt on the bar: its key, its words and what clicking it does. */
	struct FPrompt
	{
		FString Key;
		FString Text;
		EAction Action;
	};

	/** A slot, ward or backpack row, and the parts that restyle as the cursor moves (the widgets live in the widget tree). */
	struct FRow
	{
		ULooterButton* Button = nullptr;
		/** The row's glass (a background) and its lines (the lit outline, the tick). */
		ULooterCardBackdrop* Fill = nullptr;
		ULooterCardBackdrop* Lines = nullptr;
		/** The lit wash flashed as a gun lands in the row (faded out by TickMotion). */
		ULooterCardBackdrop* Flash = nullptr;
		/** A slot's number, lit while its gun is in hand (slot rows only). */
		UBorder* Badge = nullptr;
		UTextBlock* BadgeText = nullptr;
		/** A word on the row's top edge: MOVING, SWAP TARGET, WORN. */
		UBorder* Chip = nullptr;
		UTextBlock* ChipText = nullptr;
		/** NEW, until the cursor has been on the gun. */
		UWidget* NewMark = nullptr;
		/** JUNK, on a backpack row whose gun is marked junk (null on the others; the list is rebuilt when the mark changes). */
		UWidget* JunkMark = nullptr;
		/** Its gun's identity (LoadoutRules::GunIdentity), 0 for an empty slot. */
		uint32 Identity = 0;
		FLinearColor Rarity = FLinearColor::White;
		bool bFilled = false;
	};

	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void HandleAmmoChanged(EAmmoType Type, int32 Carried);

	// --- The page (LoadoutWidgetLayout.cpp) ---
	UCanvasPanelSlot* PlaceOnPage(UWidget* Widget, const FVector2D& Position, const FVector2D& Size, const FVector2D& Alignment = FVector2D::ZeroVector);
	/** A section's header line, HeaderHeight tall: its title in orange, its count after it, and Extra on its right end. */
	UWidget* MakeSectionHeader(const TCHAR* Title, UTextBlock*& OutCount, UWidget* Extra = nullptr);
	void BuildLeftColumn();
	void BuildRightColumn();
	/** The middle's gun card, the plain card (a ward's, an empty slot's, Inspect's) and the drag ghost. */
	void BuildCards();
	/** Places the showcase and the cards and shows or hides the lists, for Inspect or not. */
	void ApplyMode();
	void ApplyStageBrush();

	// --- Rows (LoadoutWidgetRows.cpp) ---
	void RebuildSlots();
	FRow MakeSlotRow(int32 SlotIndex);
	/** The backpack list, in the sort's order (best for slot: the target slot's kind first), then its free places. */
	void RebuildList();
	FRow MakeBackpackRow(int32 Row);
	/** A faint EMPTY row for a free place in the backpack (nothing to press: a gun dropped on it is stowed). */
	UWidget* MakeEmptyBackpackRow();
	/** The parts every row shares: its backdrops, flash and chip, round its content, in a button. */
	FRow WrapRow(UWidget* Content, const FLinearColor& Rarity, bool bHasItem, float Height, FName Action, int32 Index);
	void ApplyRowState(const FRow& Row, ELooterRowState State) const;
	void Restyle();
	void RefreshCounts();
	void RefreshAmmo();
	void RefreshPrompts();
	TArray<FPrompt> CurrentPrompts() const;

	// --- The cards and the showcase (LoadoutWidgetInspect.cpp) ---
	/** The middle card for what's under the cursor (the gun card, a ward's, an empty slot's), or Inspect's. */
	void RefreshCard();
	/** Shows the plain card (else the gun card); Sweep dresses the plain card's head in that gun's maker's sweep (Inspect). */
	void ShowPlainCard(bool bPlain, const FWeaponInstanceData* Sweep = nullptr);
	/** The card's open sound, at most every CardOpenCueGap. */
	void PlayCardOpenCue();
	/** The plain card's line over the name: where the gun is, or how it compares; Arrow 1 / -1 puts the green up or red down arrow first. */
	void AddCardBanner(const FString& Words, const FLinearColor& WordsColor, int32 Arrow = 0);
	/** Every stat, against the gun it would replace (Against, when there is one of its kind). */
	void AddCardStats(const FWeaponInstanceData& Gun, const FWeaponInstanceData* Against);
	void AddCardInspect(const FWeaponInstanceData& Gun);
	void AddCardEmpty(const FString& Title, const FString& Words);
	/** The showcase's gun follows the cursor. */
	void RefreshShowcase();

	// --- The cursor in the two columns (LoadoutWidgetColumns.cpp) ---
	/** W / S: Rows up or down within the cursor's column. */
	void MoveCursor(int32 Rows);
	/** A / D: to the other column's row nearest the cursor's on the page. */
	void MoveColumn(bool bRight);
	/** Where a row's middle is laid out on the page (y), from the layout's numbers and the backpack's scroll. */
	float RowCenterY(EZone RowZone, int32 Index) const;
	/** bSound: a key moved it (the mouse's own hover sound plays from the button). */
	void MoveCursorTo(EZone NewZone, int32 NewIndex, bool bScrollIntoView, bool bSound);
	/** The cursor moved onto a gun: it isn't new any more. */
	void MarkSeen();
	/** A gun just landed in a row: it flashes there. */
	void FlashRow(EZone RowZone, int32 Index);
	void PlayCue(const TCHAR* Cue) const;

	// --- Actions (LoadoutWidgetInput.cpp) ---
	void RunAction(EAction Action);
	/** E / click: see the class comment. */
	void Activate();
	void Hold();
	/** M: pick up the slot under the cursor, to put it on another slot. */
	void PickSlot();
	void Stow();
	void Drop();
	void ToggleInspect();
	void CycleSort();
	void CancelPick();

	// --- Drag and drop (LoadoutWidgetDrag.cpp) ---
	/** The gun row under a screen point, as the zone and index it was pressed in (backpack: the list row). */
	bool FindGunCard(const FVector2D& ScreenPosition, EZone& OutZone, int32& OutIndex) const;
	FDropTarget FindDropTarget(const FVector2D& ScreenPosition) const;
	void BeginItemDrag();
	void UpdateItemDrag(const FVector2D& ScreenPosition);
	void FinishItemDrag(const FVector2D& ScreenPosition);
	void CancelItemDrag();
	/** CancelItemDrag's state alone (no restyle, no focus): the screen opening or closing mid-drag leaves no ghost card or press. */
	void ClearItemDrag();
	/** What letting the dragged gun go on Target does, in words ("Swap with slot 2"); empty if nothing. */
	FString DropActionText(const FDropTarget& Target) const;
	void ApplyDrop(const FDropTarget& Target);
	/** A point on the page (1600 x 900 layout units) from screen space. */
	FVector2D ToPage(const FVector2D& ScreenPosition) const;

	void HandleRowClicked(ULooterButton* Button);
	void HandleRowHovered(ULooterButton* Button);
	/** A title tab: go to that page of the inventory. */
	void HandleTabClicked(ULooterButton* Button);
	/** A key hint clicked: its action. */
	void HandlePromptClicked(ULooterButton* Button);

	// --- The showcase's ring and the screen's motion (LoadoutWidgetPaint.cpp) ---
	void PaintBack(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;
	void PaintFront(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;
	/** A point on the showcase's gun, in the paint layers' space (the page's). */
	bool ProjectToPage(const FVector& WorldLocation, FVector2f& OutPoint) const;
	/** The plain card sliding in, a row's flash fading. */
	void TickMotion(float DeltaTime);

	// --- The wards (LoadoutWidgetWard.cpp; their card LoadoutWidgetWardCard.cpp) ---
	/** Adds the WARD header and its list to the left column (after the slots). */
	void BuildWardSection(UVerticalBox* Column);
	/** The ward rows (the one worn, then the spares), and the shield bound so its changes refresh the screen. */
	void RebuildWards();
	FRow MakeWardRow(int32 Row);
	void RestyleWards();
	void RefreshWardCard();
	/** The ward showcase: its Inked icon, big, where the gun would turn. */
	void RefreshWardShowcase();
	TArray<FPrompt> WardPrompts() const;
	/** An action on a ward's row; false when it isn't one the wards take (close, cancel, sort). */
	bool RunWardAction(EAction Action);
	void WardActivate();
	void WardStow();
	void WardDrop();
	void HandleWardClicked(ULooterButton* Button);
	void HandleWardHovered(ULooterButton* Button);
	/** The ward on row Row (0 the one worn, then the spares), or null. */
	const FShieldInstanceData* WardItem(int32 Row) const;
	UPlayerShieldComponent* GetShield() const;

	// --- Queries (LoadoutWidget.cpp) ---
	const FWeaponInstanceData* SlotItem(int32 SlotIndex) const;
	/** The backpack gun on a row of the list. */
	const FWeaponInstanceData* ListItem(int32 Row) const;
	/** The gun under the cursor, or null (an empty slot, a ward). */
	const FWeaponInstanceData* CursorItem() const;
	/** What a backpack gun is compared with: the target slot's gun of the same kind, else null. */
	const FWeaponInstanceData* Baseline() const;
	int32 NumSlots() const;
	int32 NumWeapons() const;
	int32 GetActiveSlot() const;
	bool HasBackpackRoom() const;
	bool IsNew(uint32 Identity) const;
	FLinearColor RarityColor(const FWeaponInstanceData* Gun) const;

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<UWeaponManagerComponent> Manager;
	TWeakObjectPtr<ALoadoutGunStage> Stage;

	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Page;
	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> StageMaterial;
	UPROPERTY(Transient) TObjectPtr<UImage> StageImage;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> StageSlot;
	UPROPERTY(Transient) TObjectPtr<ULoadoutPaintLayer> BackLayer;
	UPROPERTY(Transient) TObjectPtr<ULoadoutPaintLayer> FrontLayer;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ShowcaseHint;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> ShowcaseHintSlot;
	/** The two columns (put away on Inspect). */
	UPROPERTY(Transient) TObjectPtr<UWidget> LeftColumn;
	UPROPERTY(Transient) TObjectPtr<UWidget> RightColumn;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EquippedCount;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SlotList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BackpackCount;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> SortButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SortText;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ListBox;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> AmmoRow;
	/** The middle's gun card. */
	UPROPERTY(Transient) TObjectPtr<UGunCardWidget> GunCard;
	/** The plain card: a ward's or an empty slot's in the middle, Inspect's on the right. Its head wears a maker's sweep on Inspect. */
	UPROPERTY(Transient) TObjectPtr<UWidget> PlainCard;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> PlainCardSlot;
	UPROPERTY(Transient) TObjectPtr<USizeBox> PlainCardSize;
	UPROPERTY(Transient) TObjectPtr<USizeBox> PlainCardLimit;
	UPROPERTY(Transient) TObjectPtr<ULooterCardBackdrop> PlainFill;
	UPROPERTY(Transient) TObjectPtr<ULooterCardBackdrop> PlainLines;
	UPROPERTY(Transient) TObjectPtr<UWidget> CardHead;
	UPROPERTY(Transient) TObjectPtr<UWidget> CardHeadGap;
	UPROPERTY(Transient) TObjectPtr<UImage> CardHeadMark;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CardHeadWord;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CardHeadQuirk;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> CardScroll;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> CardBox;
	UPROPERTY(Transient) TObjectPtr<UWidget> DragGhost;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> GhostSlot;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> GhostBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GhostAction;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
	UPROPERTY(Transient) TObjectPtr<UWidget> WardHeader;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> WardCount;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> WardList;
	UPROPERTY(Transient) TObjectPtr<UImage> WardShowcase;

	TArray<FRow> SlotRows;
	TArray<FRow> ListRows;
	/** The ward rows: the one worn (always, "No ward" when none), then a row per spare. */
	TArray<FRow> WardRows;
	/** The backpack's free places, shown as EMPTY rows after its guns. */
	int32 NumEmptyRows = 0;
	TWeakObjectPtr<UPlayerShieldComponent> BoundShield;
	/** Backpack indices in the order the list shows them. */
	TArray<int32> ListOrder;

	EZone Zone = EZone::Slots;
	int32 CursorIndex = 0;
	/** The slot a backpack gun swaps into: chosen with E on a slot (on opening, the one in hand). */
	int32 TargetSlot = 0;
	/** A slot picked up with M, to put somewhere else. */
	TOptional<int32> PickedSlot;
	bool bInspecting = false;
	LoadoutRules::ESort Sort = LoadoutRules::ESort::Match;
	/** One action can change the inventory several times: refresh once, when it's done. */
	bool bApplyingAction = false;

	/** Guns the cursor has been on, or that were carried when the screen first opened: the rest are NEW. */
	TSet<uint32> SeenGuns;
	bool bSeenPrimed = false;

	/** A gun row pressed with the mouse: a click, or a drag once the mouse moves far enough. */
	bool bPressPending = false;
	EZone PressZone = EZone::Slots;
	/** The slot, or the backpack index (not the row: the list can re-sort while dragging). */
	int32 PressIndex = INDEX_NONE;
	FVector2D PressPosition = FVector2D::ZeroVector;
	/** Dragging the pressed gun, and where it would land right now. */
	bool bItemDrag = false;
	FDropTarget DropTarget;

	/** The showcase pressed: a click (Inspect) unless the mouse moves, which turns the gun. */
	bool bShowcasePress = false;
	bool bTurning = false;

	/** Gamepad right stick, turning the showcase's gun. */
	float TurnInput = 0.f;

	// Motion (LoadoutWidgetPaint.cpp).
	/** What the plain card shows, to slide its contents in when it changes. */
	uint32 CardIdentity = 0;
	float CardIntroAge = 100.f;
	/** The shown gun's rarity colour (the showcase's ring); the top rarities' pool breathes. */
	FLinearColor CardRarity = FLinearColor::White;
	bool bCardBreathes = false;
	/** When the card's open sound last played (FPlatformTime seconds). */
	double LastCardOpenCue = -1.0;
	EZone FlashZone = EZone::Slots;
	int32 FlashIndex = INDEX_NONE;
	float FlashAge = 100.f;
	/** Seconds the screen has been open: the rarity pool breathes on it. */
	float Clock = 0.f;

	FSlateBrush DiscBrush;
};
