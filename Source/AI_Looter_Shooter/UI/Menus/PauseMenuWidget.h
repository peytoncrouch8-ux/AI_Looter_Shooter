#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/Style/LooterUIStyle.h"
#include "PauseMenuWidget.generated.h"

class ALooterHUD;
class UGraphicsSettingsSubsystem;
class UKeyBindingSubsystem;
class ULooterButton;
class UScrollBox;
class USizeBox;
class USlider;
class UTextBlock;

/**
 * Escape menu: resume, graphics and interface options, rebind keys, quit game. Pauses the game while open.
 * The HUD hides under it, so while the minimap size is being set an outline of the minimap shows where it sits, at the
 * size chosen.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API UPauseMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void Open(ALooterHUD* InHUD);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UKeyBindingSubsystem* GetBindings() const;
	UGraphicsSettingsSubsystem* GetGraphics() const;
	ULooterButton* MakeButton(FName Action, int32 Index, const FString& Label, int32 FontSize = 13,
		LooterUI::EButtonKind Kind = LooterUI::EButtonKind::Normal);
	/** A labelled two-way switch (On/Off, Hold/Toggle). bKeyListRow indents it under a key binding and aligns it to that list's columns. */
	UWidget* MakeToggleRow(const FString& Label, const FString& FirstText, const FString& SecondText, FName FirstAction, FName SecondAction,
		int32 Index, bool bKeyListRow, ULooterButton*& OutFirst, ULooterButton*& OutSecond);
	/** A labelled slider in 5% steps, with its value as a percentage beside it. */
	UWidget* MakeSliderRow(const FString& Label, float MinValue, float MaxValue, USlider*& OutSlider, UTextBlock*& OutValue);
	/** The outline of the HUD's minimap, in the same corner, for previewing its size. */
	UWidget* MakeMinimapPreview();
	void RebuildControls();
	void RefreshKeyLabels();
	void RefreshGraphics();
	/** The size shown beside the minimap slider and by the preview outline. */
	void ShowMinimapScale(float Scale);

	UFUNCTION()
	void HandleTransparencyChanged(float Value);

	/** The slider was let go: save the setting (not on every step of a drag). */
	UFUNCTION()
	void HandleTransparencyReleased();

	UFUNCTION()
	void HandleMinimapSizeChanged(float Value);

	UFUNCTION()
	void HandleMinimapSizeGrabbed();

	UFUNCTION()
	void HandleMinimapSizeReleased();
	void HandleButton(ULooterButton* Button);
	void StartListening(int32 BindingIndex);
	void StopListening();
	void AssignKey(const FKey& Key);
	void SetStatus(const FString& Message, const FLinearColor& Color);

	TWeakObjectPtr<ALooterHUD> OwningHUD;

	UPROPERTY(Transient) TObjectPtr<UScrollBox> ControlsList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> KeyButtons;
	/** Hold/Toggle switch halves; Index is the binding, Action says which half. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> ModeButtons;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MotionBlurOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MotionBlurOff;
	UPROPERTY(Transient) TObjectPtr<USlider> TransparencySlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TransparencyValue;
	UPROPERTY(Transient) TObjectPtr<USlider> MinimapSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapValue;
	UPROPERTY(Transient) TObjectPtr<UWidget> MinimapPreview;
	UPROPERTY(Transient) TObjectPtr<USizeBox> MinimapPreviewSize;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapPreviewCaption;

	/** The minimap slider's handle is held; the preview shows while it is (or the slider is hovered), then lingers a moment. */
	bool bMinimapSliderHeld = false;
	float PreviewLinger = 0.f;
	float PreviewOpacity = 0.f;

	/** Index into the subsystem's binding list we're waiting on a key for, or INDEX_NONE. */
	int32 ListeningIndex = INDEX_NONE;
};
