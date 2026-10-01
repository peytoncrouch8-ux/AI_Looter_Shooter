#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Weapons/AmmoTypes.h"
#include "LoadoutWidget.generated.h"

class ALoadoutStage;
class ALooterHUD;
class FSlateWindowElementList;
class UBorder;
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
 *  - right: the chosen slot's gun and its stats, over the backpack's guns to swap into that slot (the same kind of gun
 *    first, each marked as an upgrade or not)
 *  - bottom: what the keys do right now
 * The cursor follows the mouse, or arrows / WASD / D-pad: up and down within a column, left and right between the slots and
 * the backpack list. E (or a click) on a backpack gun swaps it into the chosen slot. On a slot it picks that gun up, to put
 * it on another slot (swapping or moving) or swap it with a backpack gun. F holds a gun, Q drops it, Esc backs out of a
 * pick, then closes; Tab and I close. 2 (or the right shoulder, or its title tab) turns to the bestiary page.
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
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseCaptureLost(const FCaptureLostEvent& CaptureLostEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;

private:
	/** The two columns the cursor moves between. */
	enum class EZone : uint8 { Slots, Backpack };

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
	void Restyle();
	void RefreshDetails();
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
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailsHeader;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> DetailsBox;
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
	/** Gamepad right stick, turning the stand-in. */
	float TurnInput = 0.f;

	FSlateBrush DiscBrush;
};
