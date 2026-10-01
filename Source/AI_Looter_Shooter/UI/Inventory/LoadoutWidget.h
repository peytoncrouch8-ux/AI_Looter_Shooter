#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Weapons/AmmoTypes.h"
#include "LoadoutWidget.generated.h"

class ALoadoutStage;
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
class UWeaponDefinition;
class UWeaponManagerComponent;
struct FWeaponInstanceData;

/**
 * The inventory, as a loadout screen: your character stands in the middle, live and turnable (drag it), carrying your guns
 * where they are, in hand, on the back and at the hip.
 *  - left: EQUIPPED, one card per weapon slot, over the ammo you carry
 *  - right: the BACKPACK, every slot of it (scrolling when they don't fit): its guns, the chosen slot's kind first, each
 *    marked as an upgrade or not over that slot's gun, then its free slots
 *  - beside the gun under the cursor: a card with its stats (a backpack gun's against the chosen slot's gun)
 *  - bottom: what the keys do right now
 * Guns move by drag and drop: a gun dragged onto a slot goes there (swapping with the gun there), onto a backpack gun swaps
 * with it, onto a free backpack slot (or anywhere on the backpack) is stored, and onto the character is taken in hand.
 * The cursor follows the mouse, or arrows / WASD / D-pad: up and down within a column, left and right between the slots and
 * the backpack list. E (or a click) on a backpack gun swaps it into the chosen slot; on a free backpack slot it stores the
 * chosen slot's gun there. On a slot it picks that gun up, to put
 * it on another slot (swapping or moving) or swap it with a backpack gun. F holds a gun, Q drops it, Esc backs out of a
 * drag or a pick, then closes; Tab and I close. 2 (or the right shoulder, or its title tab) turns to the bestiary page.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API ULoadoutWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(ALooterHUD* InHUD, UWeaponManagerComponent* InManager);
	void Close();

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
	/** The two columns the cursor moves between. */
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

	/** A slot card or backpack row, and the parts that restyle as the cursor moves (the widgets live in the widget tree). */
	struct FCard
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
		/** SELECTED, on a picked-up slot's top edge. */
		UBorder* Chip = nullptr;
	};

	UFUNCTION()
	void Refresh();

	UFUNCTION()
	void HandleAmmoChanged(EAmmoType Type, int32 Carried);

	/** Shows the stand's picture, once both the image and its material exist. */
	void ApplyStageBrush();
	void RebuildSlots();
	FCard MakeSlotCard(int32 SlotIndex);
	/** The backpack list, for the chosen slot (its kind of gun first). */
	void RebuildList();
	FCard MakeListCard(int32 Row);
	/** A free backpack slot, after the guns: E stores the chosen slot's gun in it. */
	FCard MakeFreeListCard(int32 Row);
	void Restyle();
	/** The stats card for the gun under the cursor: its contents (RefreshInspect) and where it floats (PlaceInspect). */
	void RefreshInspect();
	void PlaceInspect();
	/** The card under the cursor, if it has a gun. */
	const FCard* CursorCard() const;
	void RefreshAmmo();
	void RefreshPrompts();

	void MoveCursor(int32 Columns, int32 Rows);
	void MoveCursorTo(EZone NewZone, int32 NewIndex, bool bScrollIntoView);
	/** E / click: see the class comment. */
	void Activate();
	/** F: take the gun under the cursor into your hands. */
	void Hold();
	/** Q: drop the gun under the cursor. */
	void Drop();
	void CancelPick();

	// Drag and drop (LoadoutWidgetDrag.cpp).
	/** The gun card under a screen point, as the zone and index it was pressed in (backpack: the list row). */
	bool FindGunCard(const FVector2D& ScreenPosition, EZone& OutZone, int32& OutIndex) const;
	FDropTarget FindDropTarget(const FVector2D& ScreenPosition) const;
	void BeginItemDrag();
	void UpdateItemDrag(const FVector2D& ScreenPosition);
	void FinishItemDrag(const FVector2D& ScreenPosition);
	void CancelItemDrag();
	/** What letting the dragged gun go on Target does, in words ("Swap with slot 2"); empty if nothing. */
	FString DropActionText(const FDropTarget& Target) const;
	void ApplyDrop(const FDropTarget& Target);
	/** A point on the page (1600 x 900 layout units) from screen space. */
	FVector2D ToPage(const FVector2D& ScreenPosition) const;

	void HandleCardClicked(ULooterButton* Button);
	void HandleCardHovered(ULooterButton* Button);
	/** A title tab: go to that page of the inventory. */
	void HandleTabClicked(ULooterButton* Button);

	/** Behind the stand-in: the far half of the ring it stands on, its glow and the stand's pillars. */
	void PaintBack(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;
	/** In front of it: the near half of the ring. */
	void PaintFront(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;
	/** A point on the stand-in, in the paint layers' space (the page's). */
	bool ProjectToPage(const FVector& WorldLocation, FVector2f& OutPoint) const;

	const FWeaponInstanceData* SlotItem(int32 SlotIndex) const;
	/** The backpack gun on a row of the list. */
	const FWeaponInstanceData* ListItem(int32 Row) const;
	int32 NumSlots() const;
	int32 NumWeapons() const;
	int32 GetActiveSlot() const;

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<UWeaponManagerComponent> Manager;
	TWeakObjectPtr<ALoadoutStage> Stage;

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> StageMaterial;
	UPROPERTY(Transient) TObjectPtr<UImage> StageImage;
	UPROPERTY(Transient) TObjectPtr<ULoadoutPaintLayer> BackLayer;
	UPROPERTY(Transient) TObjectPtr<ULoadoutPaintLayer> FrontLayer;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> EquippedCount;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SlotList;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> AmmoRow;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Page;
	UPROPERTY(Transient) TObjectPtr<UWidget> InspectCard;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> InspectSlot;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> InspectHeader;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> InspectBox;
	UPROPERTY(Transient) TObjectPtr<UWidget> DragGhost;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanelSlot> GhostSlot;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> GhostBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GhostAction;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ListHeader;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ListCount;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ListBox;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;

	TArray<FCard> SlotCards;
	TArray<FCard> ListCards;
	/** Backpack indices in the order the list shows them. */
	TArray<int32> ListOrder;
	/** How many of the listed guns are the chosen slot's kind (they come first). */
	int32 NumSameKind = 0;

	EZone Zone = EZone::Slots;
	int32 CursorIndex = 0;
	/** The slot the right side is about, and the one a backpack gun swaps into. */
	int32 ChosenSlot = 0;
	/** A slot picked up with E, to put somewhere else. */
	TOptional<int32> PickedSlot;
	/** One action can change the inventory several times: refresh once, when it's done. */
	bool bApplyingAction = false;

	bool bDragging = false;

	/** A gun card pressed with the mouse: a click, or a drag once the mouse moves far enough. */
	bool bPressPending = false;
	EZone PressZone = EZone::Slots;
	/** The slot, or the backpack index (not the row: the list can re-sort while dragging). */
	int32 PressIndex = INDEX_NONE;
	FVector2D PressPosition = FVector2D::ZeroVector;
	/** Dragging the pressed gun, and where it would land right now. */
	bool bItemDrag = false;
	FDropTarget DropTarget;
	/** The prompt bar's text while dragging (the bar keeps a pointer to it). */
	FString DragPromptText;

	/** The stats card shows for the cursor when it was moved with keys or a gamepad, or the mouse is over its card. */
	bool bKeyboardCursor = false;
	FVector2D LastMousePosition = FVector2D::ZeroVector;

	/** Gamepad right stick, turning the stand-in. */
	float TurnInput = 0.f;

	FSlateBrush DiscBrush;
};
