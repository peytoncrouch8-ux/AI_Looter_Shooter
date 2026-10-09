#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Inventory/LoadoutGunStage.h"
#include "UI/Inventory/LoadoutRules.h"
#include "Weapons/AmmoTypes.h"
#include "LoadoutWidget.generated.h"

class ALooterHUD;
class FSlateWindowElementList;
class UBorder;
class UCanvasPanel;
class UCanvasPanelSlot;
class UHorizontalBox;
class UImage;
class ULoadoutPaintLayer;
class ULooterButton;
class UMaterialInstanceDynamic;
class UScrollBox;
class UTextBlock;
class UVerticalBox;
class UWeaponManagerComponent;
struct FWeaponInstanceData;

/** The loadout page's layout, on the inventory's 1600 x 900 page. */
namespace LoadoutLayout
{
	/** Left: everything carried, as one list (the equip slots over the backpack, then the ammo). */
	inline constexpr float ListX = 56.f;
	inline constexpr float ListWidth = 488.f;
	inline constexpr float ColumnTop = 104.f;
	inline constexpr float ColumnBottom = 836.f;
	inline constexpr float SlotRowHeight = 80.f;
	inline constexpr float BackpackRowHeight = 42.f;
	inline constexpr float RowGap = 6.f;
	/** Right: the card of the gun under the cursor, always in the same place. */
	inline constexpr float CardX = 1144.f;
	inline constexpr float CardWidth = 400.f;
	/** Middle: the showcase, the gun under the cursor turning; on Inspect it takes the list's place too. */
	inline const FVector2D ShowcaseTopLeft(566.f, 250.f);
	inline const FVector2D ShowcaseSize(556.f, 556.f * ALoadoutGunStage::ImageHeight / ALoadoutGunStage::ImageWidth);
	inline const FVector2D InspectShowcaseTopLeft(56.f, 150.f);
	inline const FVector2D InspectShowcaseSize(1040.f, 1040.f * ALoadoutGunStage::ImageHeight / ALoadoutGunStage::ImageWidth);
	/** The key hints, centered at the bottom. */
	inline const FVector2D PromptBarPosition(800.f, 852.f);
	/** A new card's contents slide in over this long; a gun put into a row flashes its rarity for this long. */
	inline constexpr float CardIntroSeconds = 0.16f;
	inline constexpr float FlashSeconds = 0.55f;
}

/**
 * The inventory's first page, the loadout (the 2026-10-08 redesign, after the user found the character stand confusing
 * and dull): one thing in focus at a time.
 *  - left, one list: EQUIPPED (a row per equip slot: its number, lit while in hand, the gun's icon, name in its rarity's
 *    colour, kind, level and damage) over the BACKPACK (a row per gun: rarity stripe, icon, name, level, damage, and an
 *    arrow against the gun it would replace; sorted best-for-slot, by rarity, level or newest with R), then the ammo
 *  - middle: the gun under the cursor, built from its parts, swinging in and turning slowly (drag to turn, click to
 *    inspect)
 *  - right: that gun's card, always in the same place: name, kind and level, its damage big, four stats with bars and
 *    green or red arrows against the gun it would replace, its parts' best bonuses, and its notches or curse as badges
 *  - Inspect (X, or a click on the gun) hides the list, shows the gun large and opens the card to everything: every stat,
 *    fire mode and ammo, the parts and their bonuses, the curse's perk and drawback
 *  - bottom: what the keys do right now; each hint is also a button
 * E on a slot picks it as the swap target and goes to the backpack; E on a backpack gun swaps it into the target slot (or
 * equips it into a free one). F takes a gun in hand, M moves a slot (then E puts it down), C stows a slot's gun in the
 * backpack, Q drops. Guns can also be dragged: onto a slot, a backpack gun, the backpack, or the showcase (in hand). W / S
 * browse, A / D jump between the slots and the backpack, Esc backs out (a drag, a move, Inspect) then closes; Tab and I
 * close; 2 and 3 (or the tabs) turn to the ledger and the missions.
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
		/** The cursor is on an equip slot (else on a backpack row). */
		bool bOnSlot = true;
		int32 Cursor = 0;
		/** The slot a backpack gun goes into. */
		int32 TargetSlot = 0;
		bool bInspecting = false;
		LoadoutRules::ESort Sort = LoadoutRules::ESort::Match;
		/** Backpack indices, in the list's order. */
		TArray<int32> ListOrder;
		/** What the prompt bar says, "E: Swap into slot 1" and so on, in order. */
		TArray<FString> Prompts;
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
	/** The list's two parts. */
	enum class EZone : uint8 { Slots, Backpack };

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

	/** A slot or backpack row, and the parts that restyle as the cursor moves (the widgets live in the widget tree). */
	struct FRow
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
		/** The rarity's flash as a gun lands in the row. */
		UImage* Flash = nullptr;
		/** A slot's number, lit while its gun is in hand (slot rows only). */
		UBorder* Badge = nullptr;
		UTextBlock* BadgeText = nullptr;
		/** A word on the row's top edge: MOVING, SWAP TARGET. */
		UBorder* Chip = nullptr;
		UTextBlock* ChipText = nullptr;
		/** NEW, until the cursor has been on the gun. */
		UWidget* NewMark = nullptr;
		/** Its gun's identity (LoadoutRules::GunIdentity), 0 for an empty slot. */
		uint32 Identity = 0;
		FLinearColor Rarity = FLinearColor::White;
	};

	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void HandleAmmoChanged(EAmmoType Type, int32 Carried);

	// --- Rows and the list (LoadoutWidgetRows.cpp) ---
	void RebuildSlots();
	FRow MakeSlotRow(int32 SlotIndex);
	/** The backpack list, in the sort's order (best for slot: the target slot's kind first). */
	void RebuildList();
	FRow MakeBackpackRow(int32 Row);
	/** The parts every row shares: its fill, outline, rarity stripe and flash, round its content, in a button. */
	FRow WrapRow(UWidget* Content, const FLinearColor& Rarity, bool bHasGun, float Height, FName Action, int32 Index);
	void Restyle();
	void RefreshCounts();
	void RefreshAmmo();
	void RefreshPrompts();
	TArray<FPrompt> CurrentPrompts() const;

	// --- The card and the showcase (LoadoutWidgetInspect.cpp) ---
	/** The card's contents for the gun under the cursor: at a glance, or everything while inspecting. */
	void RefreshCard();
	/** The line over the name: where the gun is, or how it compares; Arrow 1 / -1 puts the green up or red down arrow first. */
	void AddCardBanner(const FString& Words, const FLinearColor& WordsColor, int32 Arrow = 0);
	/** The stats, against the gun it would replace (Against, when there is one of its kind). */
	void AddCardStats(const FWeaponInstanceData& Gun, const FWeaponInstanceData* Against);
	void AddCardBadges(const FWeaponInstanceData& Gun);
	void AddCardInspect(const FWeaponInstanceData& Gun, const FWeaponInstanceData* Against);
	void AddCardEmpty(const FString& Title, const FString& Words);
	/** The showcase's gun follows the cursor. */
	void RefreshShowcase();
	/** Places the showcase and shows or hides the list, for Inspect or not. */
	void ApplyMode();
	void ApplyStageBrush();

	// --- Cursor and actions (LoadoutWidgetInput.cpp) ---
	void MoveCursor(int32 Rows);
	void JumpTo(EZone NewZone);
	/** bSound: a key moved it (the mouse's own hover sound plays from the button). */
	void MoveCursorTo(EZone NewZone, int32 NewIndex, bool bScrollIntoView, bool bSound);
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
	/** The cursor moved onto a gun: it isn't new any more. */
	void MarkSeen();
	/** A gun just landed in a row: its rarity flashes there. */
	void FlashRow(EZone RowZone, int32 Index);
	void PlayCue(const TCHAR* Cue) const;

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
	/** The card sliding in, a row's flash fading, the rarity glow breathing. */
	void TickMotion(float DeltaTime);

	// --- Queries (LoadoutWidget.cpp) ---
	const FWeaponInstanceData* SlotItem(int32 SlotIndex) const;
	/** The backpack gun on a row of the list. */
	const FWeaponInstanceData* ListItem(int32 Row) const;
	/** The gun under the cursor, or null (an empty slot). */
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
	UPROPERTY(Transient) TObjectPtr<UWidget> ListColumn;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EquippedCount;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SlotList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> BackpackCount;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> SortButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SortText;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ListBox;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> AmmoRow;
	UPROPERTY(Transient) TObjectPtr<UImage> CardGlow;
	UPROPERTY(Transient) TObjectPtr<UImage> CardFill;
	UPROPERTY(Transient) TObjectPtr<UImage> CardLine;
	UPROPERTY(Transient) TObjectPtr<UImage> CardStripe;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> CardScroll;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> CardBox;
	UPROPERTY(Transient) TObjectPtr<UWidget> DragGhost;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> GhostSlot;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> GhostBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GhostAction;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;

	TArray<FRow> SlotRows;
	TArray<FRow> ListRows;
	/** Backpack indices in the order the list shows them. */
	TArray<int32> ListOrder;
	/** How many of the listed guns are the target slot's kind (they come first when sorted best for slot). */
	int32 NumSameKind = 0;

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
	/** The gun the card shows, to slide its contents in when it changes. */
	uint32 CardIdentity = 0;
	float CardIntroAge = 100.f;
	/** The card's gun's rarity colour; the top rarities' glow breathes. */
	FLinearColor CardRarity = FLinearColor::White;
	bool bCardBreathes = false;
	EZone FlashZone = EZone::Slots;
	int32 FlashIndex = INDEX_NONE;
	float FlashAge = 100.f;
	/** Seconds the screen has been open: the rarity glow breathes on it. */
	float Clock = 0.f;

	FSlateBrush DiscBrush;
};
