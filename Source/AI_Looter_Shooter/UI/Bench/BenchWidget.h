#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/WeaponManagerComponent.h"
#include "UI/Bench/BenchRules.h"
#include "UI/Bench/BenchStage.h"
#include "UI/Style/LooterUIStyle.h"
#include "BenchWidget.generated.h"

class ALooterHUD;
class FSlateWindowElementList;
class UBorder;
class UCanvasPanel;
class UHorizontalBox;
class UImage;
class ULoadoutPaintLayer;
class ULooterButton;
class UMaterialInstanceDynamic;
class UOverlay;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

/** Where the bench's screen puts things on its 1600 x 900 page (the loadout's page size, scaled to fit the screen). */
namespace BenchLayout
{
	inline constexpr float GunsX = 50.f;
	inline constexpr float GunsWidth = 320.f;
	inline constexpr float SlotsX = 390.f;
	inline constexpr float SlotsWidth = 300.f;
	inline constexpr float PartsX = 1200.f;
	inline constexpr float PartsWidth = 350.f;
	inline constexpr float ColumnTop = 150.f;
	inline constexpr float ColumnHeight = 690.f;
	/** The stand's picture, between the slots and the parts box, at the gun's long, low proportions. */
	inline const FVector2D StageTopLeft(710.f, 112.f);
	inline const FVector2D StageSize(470.f, 470.f * ABenchStage::ImageHeight / ABenchStage::ImageWidth);
	/** The chosen gun's card under the stand: its name, notches and curse, and its stats. */
	inline constexpr float GunCardTop = 440.f;
	inline constexpr float GunRowHeight = 58.f;
	inline constexpr float SlotRowHeight = 60.f;
	inline constexpr float PartRowHeight = 62.f;
	/** The confirm's width (scrapping a gun, throwing a part out). */
	inline constexpr float ConfirmWidth = 520.f;
}

/** What the bench screen's buttons do (ULooterButton::Action). */
namespace BenchActions
{
	inline const FName Gun(TEXT("BenchGun"));
	inline const FName Slot(TEXT("BenchSlot"));
	inline const FName Part(TEXT("BenchPart"));
	inline const FName Scrap(TEXT("BenchScrap"));
	inline const FName StopScrapping(TEXT("BenchStopScrapping"));
	inline const FName Accept(TEXT("BenchAccept"));
	inline const FName Back(TEXT("BenchBack"));
}

/**
 * The gunsmith's bench's screen (AGunsmithBench; ALooterHUD::OpenBench), laid out and worked like the loadout screen:
 *  - left: GUNS, every gun the player carries (the equip slots, then the backpack); the chosen one is lit
 *  - beside it: the chosen gun's slots down the side of its stand, each with the part in it now
 *  - middle: the chosen gun on a stand, turnable (drag it), the chosen slot's part marked on it; under it the gun's name,
 *    level, notches and curse (LoadoutParts::MakeGunIdeasRows) and its stats
 *  - right: the PARTS BOX for the chosen slot: the parts that fit first, each with what it changes against the part in the
 *    slot now (WeaponPartSwap::PreviewStats); the ones that don't, dimmed, saying why (WeaponPartSwap::CheckText). The part
 *    under the cursor shows on the stand and in the stats, as if fitted (and the gun's name as it would read).
 *  - top: the title, and under it a line saying what the last action did; bottom: what the keys do right now
 * Fitting: a click (or Enter, Space, F) on a part that fits. Scrapping: X (or the Scrap button) on the chosen gun, then the
 * part to keep from its slots, then the confirm ("Scrap <name>? Keep the <part>"). Q (or a right-click) on a box part
 * throws it out, after a confirm. W / S and the arrows move within a column, A / D between the columns; Esc backs out of a
 * confirm or a scrapping, then closes; the Interact key and the inventory key close it too. Named guns keep their parts:
 * the screen says so. It holds the game's input (the HUD gives it the keyboard and mouse) but doesn't pause.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBenchWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows the guns InManager carries, starting on the gun in hand. Bench: the bench used (null: the console's). */
	void Open(ALooterHUD* InHUD, UWeaponManagerComponent* InManager, AActor* InBench);
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
	/** The three columns the cursor moves between. */
	enum class EColumn : uint8 { Guns, Slots, Parts };

	/** What the confirm asks. */
	enum class EConfirm : uint8 { None, Scrap, Discard };

	/** A row in a column, and the parts that restyle as the cursor moves (the widgets live in the widget tree). */
	struct FRow
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
		/** KEEP, on the part being kept while scrapping (slot rows only). */
		UBorder* Chip = nullptr;
	};

	/** The player's guns or the parts box changed: everything is listed again. */
	UFUNCTION()
	void Refresh();

	// --- Layout (BenchWidget.cpp) ---

	void ApplyStageBrush();
	/** A column's heading: its title in the accent, a count beside it, and a rule to its end. */
	UWidget* MakeColumnHeader(UTextBlock*& OutTitle, const FString& Title, UTextBlock*& OutCount);

	// --- The columns (BenchWidgetLists.cpp) ---

	void RebuildGuns();
	FRow MakeGunRow(int32 Row);
	void RebuildSlots();
	FRow MakeSlotRow(int32 SlotIndex);
	void RebuildParts();
	FRow MakePartRow(int32 Row);
	/** A quiet card in place of a list: what's missing and what to do about it. */
	UWidget* MakeNote(const FString& Title, const FString& Body);
	void Restyle();
	const TArray<FRow>& RowsOf(EColumn InColumn) const;

	// --- The chosen gun: its card, its stand and the prompts (BenchWidgetGun.cpp) ---

	void RefreshGun();
	/** Puts the chosen gun on the stand, or the part under the cursor fitted to it. */
	void RefreshStage(bool bResetTurn);
	void RefreshPrompts();
	/** The line under the title: what the last action did (or the bench's hint). */
	void SetStatus(const FText& Text, const FLinearColor& TextColor);
	void PaintBack(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;
	void PaintFront(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;
	bool ProjectToPage(const FVector& WorldLocation, FVector2f& OutPoint) const;

	// --- Choosing and acting (BenchWidgetInput.cpp) ---

	void MoveCursor(int32 Columns, int32 Rows);
	void MoveCursorTo(EColumn NewColumn, int32 NewIndex, bool bScrollIntoView);
	/** Enter / a click: see the class comment. */
	void Activate();
	void ChooseGun(int32 Row);
	void ChooseSlot(int32 SlotIndex);
	void TryFit(int32 Row);
	/** X: choose the part to keep from the chosen gun (or the gun under the cursor). */
	void BeginScrap();
	void StopScrapping();
	void AskScrap(int32 SlotIndex);
	void AskDiscard(int32 Row);
	/** Something can't be done: the denied sound, and the line under the title says why. */
	void Deny(const FText& Why);
	void HandleRowClicked(ULooterButton* Button);
	void HandleRowHovered(ULooterButton* Button);
	void HandleButton(ULooterButton* Button);

	// --- The confirm (BenchWidgetConfirm.cpp) ---

	void OpenConfirm(EConfirm Kind, const FString& Title, const FString& Body, const FText& AcceptLabel);
	void CloseConfirm();
	void AcceptConfirm();
	void ScrapChosen();
	void DiscardChosen();
	ULooterButton* MakeButton(FName Action, const FText& Text, LooterUI::EButtonKind Kind);

	// --- Queries ---

	const FWeaponInstanceData* ChosenItem() const;
	FCarriedGun ChosenRef() const;
	/** The chosen gun's parts can be changed (a rolled gun, not a named one). */
	bool CanChange() const;
	int32 NumSlots() const;
	FName SlotNameAt(int32 SlotIndex) const;
	/** The box part on a row of the parts column. */
	const FBoxedWeaponPart* RowPart(int32 Row) const;
	/** The fitting part under the cursor, shown as if fitted (the stand, the stats): null when the cursor isn't on one. */
	const FBoxedWeaponPart* PreviewPart() const;
	/** The slot marked on the stand: the part being kept while scrapping, otherwise the chosen slot. */
	int32 MarkedSlot() const;
	void PlayCue(const TCHAR* Cue, float Volume = 1.f) const;

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<UWeaponManagerComponent> Manager;
	TWeakObjectPtr<AActor> Bench;
	TWeakObjectPtr<ABenchStage> Stage;

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> StageMaterial;
	UPROPERTY(Transient) TObjectPtr<UImage> StageImage;
	UPROPERTY(Transient) TObjectPtr<ULoadoutPaintLayer> BackLayer;
	UPROPERTY(Transient) TObjectPtr<ULoadoutPaintLayer> FrontLayer;
	UPROPERTY(Transient) TObjectPtr<UCanvasPanel> Page;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GunsCount;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> GunList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SlotsTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> SlotsCount;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> SlotList;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> ScrapButton;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> StopScrappingButton;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PartsCount;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PartsSub;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> PartList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GunTag;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GunName;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> GunSub;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> IdeasBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatsHeader;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> StatsBox;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
	UPROPERTY(Transient) TObjectPtr<UOverlay> PopupLayer;

	/** Every gun carried, in the guns column's order. */
	TArray<FCarriedGun> Guns;
	/** The box's parts for the chosen slot, in the parts column's order. */
	TArray<BenchRules::FPartEntry> PartEntries;

	TArray<FRow> GunRows;
	TArray<FRow> SlotRows;
	TArray<FRow> PartRows;

	/** The gun being worked on (a row of Guns) and its slot the parts column is for (its definition's slot index). */
	int32 ChosenGun = 0;
	int32 ChosenSlot = 0;

	EColumn Column = EColumn::Guns;
	int32 CursorIndex = 0;

	/** Choosing which of the chosen gun's parts to keep as it's scrapped (the slots column says KEEP). */
	bool bScrapping = false;

	EConfirm Confirm = EConfirm::None;
	/** What the confirm is about: the slot whose part a scrap keeps, the box part a discard throws out. */
	int32 ConfirmSlot = INDEX_NONE;
	int32 ConfirmBoxIndex = INDEX_NONE;

	/** One action can change the inventory several times: refresh once, when it's done. */
	bool bApplyingAction = false;

	/** Turning the stand with the mouse, and with the gamepad's right stick. */
	bool bDragging = false;
	float TurnInput = 0.f;

	FSlateBrush DiscBrush;
};
