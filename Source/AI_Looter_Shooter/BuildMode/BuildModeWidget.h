#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/LooterUIStyle.h"
#include "BuildModeWidget.generated.h"

class UBorder;
class UBuildModeComponent;
class UHorizontalBox;
class ULooterButton;
class UScrollBox;
class UUniformGridPanel;
class UTextBlock;
class UVerticalBox;

/** Build Mode palette: category tabs, placeable items, tool toggles, save/undo/clear, status and hotkey help. */
UCLASS()
class AI_LOOTER_SHOOTER_API UBuildModeWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Init(UBuildModeComponent* InBuildMode);

	/** True while the mouse is over the palette panel (so clicks there don't place objects). */
	bool IsPointerOverPanel() const;

	/** Palette indices shown in the current tab, in display order (for the 1-9 hotkeys). */
	TArray<int32> GetVisibleEntries() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	ULooterButton* MakeButton(FName Action, int32 Index, const FText& Text, int32 FontSize = 12, LooterUI::EButtonKind Kind = LooterUI::EButtonKind::Normal);
	UTextBlock* MakeText(const FString& Text, int32 Size, const FLinearColor& Color);
	void RebuildCategories();
	void RebuildEntries();
	void RefreshToggles();
	void HandleButton(ULooterButton* Button);

	TWeakObjectPtr<UBuildModeComponent> BuildMode;

	UPROPERTY(Transient) TObjectPtr<UBorder> Panel;
	UPROPERTY(Transient) TObjectPtr<UUniformGridPanel> CategoryRow;
	UPROPERTY(Transient) TObjectPtr<UScrollBox> EntryList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CountText;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> Crosshair;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ModeHint;
	UPROPERTY(Transient) TObjectPtr<UWidget> ModeHintPlate;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> RandomRotationButton;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> RandomScaleButton;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> AlignButton;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> BrushButton;
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> CategoryButtons;
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> EntryButtons;

	TArray<FName> Categories;
	FName ActiveCategory;
	int32 ShownSelection = INDEX_NONE - 1;
};
