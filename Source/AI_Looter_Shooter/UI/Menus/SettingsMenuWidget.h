#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "SettingsMenuWidget.generated.h"

class UControlSettingsSubsystem;
class UGraphicsSettingsSubsystem;
class UImage;
class UKeyBindingSubsystem;
class ULooterButton;
class UScrollBox;
class USizeBox;
class USlider;
class UTextBlock;
class UVerticalBox;

/** Where the settings menu is open. */
enum class ESettingsMenuMode : uint8
{
	/** Over the game, which is paused under it: Resume, and Save & Quit back to the main menu. */
	Pause,
	/** From the main menu: Back. */
	MainMenu,
};

DECLARE_DELEGATE(FOnSettingsMenuAction);

/**
 * The settings menu: graphics, audio (the volumes) and interface options, and the controls (look sensitivity and key
 * bindings), one scrolling list between the header and the footer. Escape opens it over the game (ALooterHUD, which
 * pauses the game) and the main menu's Settings opens it there. The HUD hides under it, so while the minimap size is
 * being set an outline of the minimap (the map's circle and the bezel round it) shows where it sits, at the size chosen.
 *
 * SettingsMenuWidget.cpp builds and fills it; SettingsMenuRows.cpp makes its rows and the key list;
 * SettingsMenuInput.cpp handles its buttons, sliders and keys; SettingsMenuAudio.cpp is the Audio section.
 */
UCLASS()
class AI_LOOTER_SHOOTER_API USettingsMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Shows it fresh (settings read again, status cleared) for Mode. Add it to the viewport after. */
	void Open(ESettingsMenuMode InMode);

	ESettingsMenuMode GetMode() const { return Mode; }

	/** Resume (over the game) or Back (main menu), the corner's X, or Escape. */
	FOnSettingsMenuAction OnClose;

	/** Save & Quit, over the game. */
	FOnSettingsMenuAction OnSaveAndQuit;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

private:
	UKeyBindingSubsystem* GetBindings() const;
	UGraphicsSettingsSubsystem* GetGraphics() const;
	UControlSettingsSubsystem* GetControls() const;
	UAudioSettingsSubsystem* GetAudio() const;
	ULooterButton* MakeButton(FName Action, int32 Index, const FString& Label, int32 FontSize = 13,
		LooterUI::EButtonKind Kind = LooterUI::EButtonKind::Normal);
	/** A labelled two-way switch (On/Off, Hold/Toggle). bKeyListRow indents it under a key binding and aligns it to that list's columns. */
	UWidget* MakeToggleRow(const FString& Label, const FString& FirstText, const FString& SecondText, FName FirstAction, FName SecondAction,
		int32 Index, bool bKeyListRow, ULooterButton*& OutFirst, ULooterButton*& OutSecond);
	/** A labelled row of choices, one highlighted at a time; each button's Index is its choice. */
	UWidget* MakeChoiceRow(const FString& Label, const TArray<FString>& Choices, FName Action, float Width, TArray<ULooterButton*>& OutButtons);
	/**
	 * A labelled slider that moves in steps of StepSize (5% of a 0-1 range by default), with a text for its value beside
	 * it (the caller writes it: a percentage, a zoom, degrees).
	 */
	UWidget* MakeSliderRow(const FString& Label, float MinValue, float MaxValue, USlider*& OutSlider, UTextBlock*& OutValue,
		float StepSize = 0.05f);
	/** The outline of the HUD's minimap, in the same corner, for previewing its size. */
	UWidget* MakeMinimapPreview();
	void RebuildControls();
	void RefreshKeyLabels();
	void RefreshGraphics();
	/** The Controls section's settings above the key list (which RefreshKeyLabels fills). */
	void RefreshControlSettings();
	/** Shows the parts that differ between the pause menu and the main menu's settings. */
	void ApplyMode();
	/** The size shown beside the minimap slider and by the preview outline. */
	void ShowMinimapScale(float Scale);

	// --- The Audio section (SettingsMenuAudio.cpp) ---

	/** The four volume sliders (Master, Effects, Interface, Music), each a row with its percentage. */
	UWidget* MakeAudioRows();
	void RefreshAudio();
	/** A volume slider moved: heard at once, with a soft tick at each step; saved when it's let go. */
	void ChangeVolume(EAudioVolume Which, float Value);

	UFUNCTION()
	void HandleMasterVolumeChanged(float Value);

	UFUNCTION()
	void HandleEffectsVolumeChanged(float Value);

	UFUNCTION()
	void HandleInterfaceVolumeChanged(float Value);

	UFUNCTION()
	void HandleMusicVolumeChanged(float Value);

	/** Any volume slider let go: the volumes are saved. */
	UFUNCTION()
	void HandleVolumeReleased();

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

	UFUNCTION()
	void HandleMinimapZoomChanged(float Value);

	UFUNCTION()
	void HandleMinimapZoomReleased();

	UFUNCTION()
	void HandleFieldOfViewChanged(float Value);

	UFUNCTION()
	void HandleFieldOfViewReleased();

	UFUNCTION()
	void HandleLookSensitivityChanged(float Value);

	UFUNCTION()
	void HandleLookSensitivityReleased();
	void HandleButton(ULooterButton* Button);
	void StartListening(int32 BindingIndex);
	void StopListening();
	void AssignKey(const FKey& Key);
	void SetStatus(const FString& Message, const FLinearColor& Color);

	ESettingsMenuMode Mode = ESettingsMenuMode::Pause;

	/** Everything between the header and the footer, scrolling as one: the sections and, last, the key list. */
	UPROPERTY(Transient) TObjectPtr<UScrollBox> Body;
	/** The key bindings' rows (RebuildControls fills it). */
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> KeyList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	/** "Game paused | Esc: resume" or "Esc: back". */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> HeaderText;
	/** Resume and Save & Quit, over the game only. */
	UPROPERTY(Transient) TObjectPtr<ULooterButton> ResumeButton;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> SaveQuitButton;
	/** Back, in the main menu only. */
	UPROPERTY(Transient) TObjectPtr<ULooterButton> BackButton;
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> KeyButtons;
	/** Hold/Toggle switch halves; Index is the binding, Action says which half. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> ModeButtons;
	/** Low, Medium, High, Epic, in order. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> QualityButtons;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MotionBlurOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MotionBlurOff;
	/** The first-person field of view, in degrees. */
	UPROPERTY(Transient) TObjectPtr<USlider> FieldOfViewSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> FieldOfViewValue;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MinimapOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> MinimapOff;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> FrameRateOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> FrameRateOff;
	UPROPERTY(Transient) TObjectPtr<USlider> TransparencySlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TransparencyValue;
	UPROPERTY(Transient) TObjectPtr<USlider> MinimapSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapValue;
	UPROPERTY(Transient) TObjectPtr<USlider> MinimapZoomSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapZoomValue;
	/** The look sensitivity, as a multiple of the game's own turn. */
	UPROPERTY(Transient) TObjectPtr<USlider> LookSensitivitySlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LookSensitivityValue;
	/** The volume sliders and their percentages, in EAudioVolume's order. */
	UPROPERTY(Transient) TArray<TObjectPtr<USlider>> VolumeSliders;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> VolumeValues;
	UPROPERTY(Transient) TObjectPtr<UWidget> MinimapPreview;
	UPROPERTY(Transient) TObjectPtr<USizeBox> MinimapPreviewSize;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapPreviewCaption;
	/** The preview's outer outline, out at the HUD minimap's bezel (the map's circle and glass sit inside it). */
	UPROPERTY(Transient) TObjectPtr<UImage> MinimapPreviewBezel;

	/** The minimap slider's handle is held; the preview shows while it is (or the slider is hovered), then lingers a moment. */
	bool bMinimapSliderHeld = false;
	float PreviewLinger = 0.f;
	float PreviewOpacity = 0.f;

	/** The volume slider moved last, for the status line when it's let go. */
	EAudioVolume LastVolumeMoved = EAudioVolume::Master;

	/** Index into the subsystem's binding list we're waiting on a key for, or INDEX_NONE. */
	int32 ListeningIndex = INDEX_NONE;
};
