#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Bestiary/BestiaryStage.h"
#include "BestiaryWidget.generated.h"

/** Where the bestiary page shows its stand's picture: between the two columns of the 1600 x 900 page, as the loadout does. */
namespace BestiaryLayout
{
	inline const FVector2D StageTopLeft(520.f, 158.f);
	inline const FVector2D StageSize(560.f, 560.f * ABestiaryStage::ImageHeight / ABestiaryStage::ImageWidth);
}

class ABestiaryStage;
class ALooterHUD;
class FSlateWindowElementList;
class UBestiaryEntry;
class UHorizontalBox;
class UImage;
class ULoadoutPaintLayer;
class ULooterButton;
class UMaterialInstanceDynamic;
class UScrollBox;
class UTextBlock;
class UVerticalBox;

/**
 * The bestiary, the inventory's second page, laid out like the loadout page beside it:
 *  - left: every entry (UBestiaryEntry data assets) by section: creatures, enemies, NPCs, friends
 *  - middle: the chosen entry's model on a stand, turnable (drag it)
 *  - right: what it is, where it lives, its level, health, attack, experience and how many you've defeated, then a
 *    description and field notes
 * Until the player has met one (been hunted by it, hurt it or defeated it), an entry reads "???" and its model stands as
 * a dark silhouette.
 *  - bottom: what the keys do
 * W / S, the arrows or the D-pad choose an entry (so does the mouse); 1, the left shoulder or the Loadout tab go back to
 * the loadout, 3, the right shoulder or the Missions tab on to the missions; Esc, Tab and I close.
 *
 * From Main 2 it's Sexton's Ledger (Bestiary/Ledger.h): the same page in his words, with the pages written in it only
 * (the story's characters and the seven names). A story character's page has words (and the model it names, Hob's) but
 * no numbers; a Ledger name's has its whereabouts, blank until the story finds them. BestiaryWidgetDetails.cpp builds the
 * chosen entry's side.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UBestiaryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(ALooterHUD* InHUD);
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
	/** An entry's card in the list, and the parts that restyle with the selection. */
	struct FCard
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
	};

	void ApplyStageBrush();
	void RebuildList();
	FCard MakeEntryCard(int32 Index);
	void Select(int32 Index, bool bScrollIntoView);
	void Restyle();
	void RefreshDetails();
	void RefreshPrompts();

	void HandleCardClicked(ULooterButton* Button);
	void HandleCardHovered(ULooterButton* Button);
	void HandleTabClicked(ULooterButton* Button);
	void GoToLoadout();

	/** The ring the model stands on: its far half behind the model, its near half in front. */
	void PaintBack(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;
	void PaintFront(const FGeometry& Geometry, FSlateWindowElementList& Elements, int32 LayerId) const;
	bool ProjectToPage(const FVector& WorldLocation, FVector2f& OutPoint) const;

	const UBestiaryEntry* GetSelected() const;
	int32 GetDefeated(const UBestiaryEntry& Entry) const;
	/**
	 * The player has met one, or the story has met them: its page is open. Until then it reads "???" and its model stands
	 * as a silhouette.
	 */
	bool IsKnown(const UBestiaryEntry& Entry) const;
	/** A Ledger name's whereabouts are written in (the story has found them). */
	bool IsFound(const UBestiaryEntry& Entry) const;

	/** The book is Sexton's Ledger (read as the page opens): its words, and its pages listed. */
	bool bLedger = false;

	TWeakObjectPtr<ALooterHUD> OwningHUD;
	TWeakObjectPtr<ABestiaryStage> Stage;

	/** Every entry, in the list's order. */
	UPROPERTY(Transient) TArray<TObjectPtr<UBestiaryEntry>> Entries;

	UPROPERTY(Transient) TObjectPtr<UMaterialInstanceDynamic> StageMaterial;
	UPROPERTY(Transient) TObjectPtr<UImage> StageImage;
	UPROPERTY(Transient) TObjectPtr<ULoadoutPaintLayer> BackLayer;
	UPROPERTY(Transient) TObjectPtr<ULoadoutPaintLayer> FrontLayer;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ListTitle;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ListCount;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ListBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> DetailsHeader;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> DetailsBox;
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> NotesBox;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StageHint;
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;

	/** One per entry, by index into Entries. */
	TArray<FCard> Cards;
	int32 Selected = 0;
	/** The chosen entry has been met (checked when the details are rebuilt, not every frame). */
	bool bSelectedKnown = true;

	bool bDragging = false;
	/** Gamepad right stick, turning the model. */
	float TurnInput = 0.f;

	FSlateBrush DiscBrush;
};
