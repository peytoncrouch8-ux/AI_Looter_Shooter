#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Economy/SecretRules.h"
#include "SecretsWidget.generated.h"

class APlayerController;
class UImage;
class ULooterButton;
class UScrollBox;
class USecretBrokerComponent;
class USecretSubsystem;
class USizeBox;
class UTextBlock;
class UVerticalBox;
class UWidget;

/** The secrets screen's measures, at the kit's sizes (CSS px as Slate sizes, the user's call). */
namespace SecretsLayout
{
	inline constexpr float PanelWidth = 900.f;
	inline constexpr float RowGap = 6.f;
	inline constexpr float RowHeight = 64.f;
	/** The list scrolls past this height. */
	inline constexpr float ListHeight = 460.f;
	/** The kind's mark on a row's left. */
	inline constexpr float MarkSize = 34.f;
	/** A buy's motion: the row's soul-gold flash, and MARKED stamped in (from StampFrom times its size). */
	inline constexpr float FlashSeconds = 0.7f;
	inline constexpr float StampSeconds = 0.35f;
	inline constexpr float StampFrom = 1.35f;
}

/**
 * Mister Sexton's secrets for silver (Docs/Work/2026-10-31-Wardens/MAINS_2_7.md section 12; lanes/SECRETS.md): one
 * LooterUI panel over the dimmed fog, in the kit's Slate & Salt (gold = chosen), opened by his tap at the ferry
 * (USecretBrokerComponent):
 *  - top: the catalog's title, his line for the story (USecretSubsystem::GetGreeting), the purse on the right (counts down
 *    on a buy with a red flash) and, once Paid on Delivery is done, A QUARTER OFF under it;
 *  - FOR SALE: a row a secret, its kind's keyhole in its colour, the find's name, the teaser and its region, the price on
 *    the right (red when too dear); MARKED below: the bought ones not found yet, his line in gold, how many places;
 *  - a status line, the keys, a Close button.
 * W / S (the arrows, the D-pad, the mouse wheel over the list) choose, E (Enter, the face button, a click on the chosen row)
 * buys, Esc (the right face button) closes. A buy: Vendor.Buy and World.Find.Glint, the row's soul-gold flash, MARKED
 * stamped in, the purse counting down; too dear: Vendor.NotEnough and what's short. It opens and closes itself like the
 * bounty board's screen (OpenFor, GetOpen; ALooterHUD::IsMenuOpen counts it), holds the keyboard and shows the mouse,
 * doesn't pause, and closes if the player walks StayRadius off or dies. Every background follows the UI transparency
 * setting (MarkBackground).
 *
 * SecretsWidget.cpp: opening, the panel, choosing, keys, buying, the purse. SecretsWidgetRows.cpp: the rows and their motion.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USecretsWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Opens InBroker's screen for InPlayer (closing one up already). bInAnywhere (the console): it doesn't close for
	 * distance. Null when it can't (no local player, no broker).
	 */
	static USecretsWidget* OpenFor(APlayerController* InPlayer, USecretBrokerComponent* InBroker, bool bInAnywhere = false);

	/** The secrets screen up now, if any. */
	static USecretsWidget* GetOpen();

	/** Closes it: the game has its keys and its mouse back. */
	void Close();

	/** The chosen row, and choosing one (wrapped round the list). */
	int32 GetSelected() const { return Selected; }
	void Select(int32 RowIndex, bool bSound = true);

	/** E on the chosen row: buys it. Whether it was bought. */
	bool BuySelected();

	/** The rows shown now (for sale, then marked), for the tests. */
	const TArray<FSecretRow>& GetRows() const { return Rows; }
	int32 GetRowCount() const { return Rows.Num(); }

	/** The status line's words now. */
	FText GetStatus() const { return Status; }

	/** The player walks this far (cm) from the broker and the screen closes (he sits out on the ferry's stern). */
	static constexpr float StayRadius = 650.f;

protected:
	virtual void NativeOnInitialized() override;
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	/** One row's widgets that restyle (the row lives in the tree). */
	struct FRowParts
	{
		ULooterButton* Button = nullptr;
		UImage* Fill = nullptr;
		UImage* Line = nullptr;
		UWidget* Stamp = nullptr;
	};

	// --- SecretsWidget.cpp ---
	/** Reads the rows again and builds the panel; KeepId stays chosen where it's still listed. */
	void Refresh(FName KeepId);
	void BuildPanel();
	void Restyle();
	void SetStatus(const FText& Words, const FLinearColor& TextColor);
	void HandleButton(ULooterButton* Button);
	void HandleHovered(ULooterButton* Button);
	bool ShouldClose() const;
	void UpdatePurse(float DeltaTime);
	USecretSubsystem* GetSecrets() const;

	// --- SecretsWidgetRows.cpp ---
	/** The list: FOR SALE rows, then MARKED; an empty list's line. */
	void BuildRows(UVerticalBox* List);
	UWidget* MakeRow(int32 RowIndex);
	/** The kind's keyhole in its colour, in a dark badge. */
	UWidget* MakeMark(const FSecretRow& Row) const;
	/** A buy's motion on the row FlashId: the soul-gold flash, the stamp. */
	void UpdateMotion(float DeltaTime);
	static FLinearColor KindColor(ESecretKind Kind);

	TWeakObjectPtr<USecretBrokerComponent> Broker;
	TWeakObjectPtr<APlayerController> Player;
	bool bAnywhere = false;

	TArray<FSecretRow> Rows;
	TArray<FRowParts> Parts;
	int32 Selected = 0;
	FText Status;
	FLinearColor StatusColor = FLinearColor::White;

	/** The row just bought (its id: rows are made again after a buy) and how far into its motion (s). */
	FName FlashId;
	float FlashAge = -1.f;

	/** The purse as shown: it counts to the silver carried, flashing red as it goes down. */
	float ShownSilver = 0.f;
	int32 PurseTarget = 0;
	float PurseFlash = 0.f;
	bool bPurseGain = false;

	UPROPERTY(Transient) TObjectPtr<USizeBox> PanelBox;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> ListScroll;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> PurseText;
	UPROPERTY(Transient) TObjectPtr<UWidget> PurseLine;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
};
