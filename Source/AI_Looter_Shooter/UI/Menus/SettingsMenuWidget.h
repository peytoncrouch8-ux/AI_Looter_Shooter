#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "UI/Menus/MenuNav.h"
#include "UI/Menus/MenuNavigator.h"
#include "UI/Menus/SettingsMenuPreview.h"
#include "UI/Style/LooterUIFocus.h"
#include "UI/Style/LooterUIStyle.h"
#include "SettingsMenuWidget.generated.h"

class UBorder;
class UControlSettingsSubsystem;
class UGraphicsSettingsSubsystem;
class UHorizontalBox;
class UHudMinimapWidget;
class UImage;
class UKeyBindingSubsystem;
class ULooterButton;
class UPlayerViewComponent;
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
 * pauses the game) and the main menu's Settings opens it there. The HUD hides under it, so some sliders preview what
 * they set while they move: the minimap size shows an outline of the minimap (the map's circle and the bezel round it)
 * where it sits, at the size chosen; the minimap zoom shows a live copy of the minimap there; and the first-person field
 * of view changes the view behind the menu, which fades back (the dimming goes, the panel ghosts) for as long as the
 * slider is held, and a moment after. Esc while one of the last two is held lets go of it with the setting as it was.
 *
 * A controller or the keyboard drive all of it, Borderlands' way: the D-pad (or the left stick, arrows, W A S D) moves the
 * focus down the list, steps a slider (held, faster and faster) or a switch; A (Enter) presses or changes; LB / RB (Page
 * Up / Down) turn the sections; B (Esc) lets go of a held preview, then closes. The focus look (LooterUIFocus.h) and the
 * prompts along the bottom follow the device last used; the mouse works as it always did.
 *
 * SettingsMenuWidget.cpp builds and fills it; SettingsMenuRows.cpp makes its rows and the key list;
 * SettingsMenuInput.cpp handles its buttons, sliders and keys; SettingsMenuAudio.cpp is the Audio section;
 * SettingsMenuPreview.cpp is the field of view and minimap zoom sliders and the previews; SettingsMenuNav.cpp is the
 * keyboard and controller focus, its look and the prompts.
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

	/**
	 * Escape (or the controller's menu button) while the field of view or minimap zoom slider is held: lets go of the
	 * slider with the setting back as it was when it was taken, and keeps the menu open. False when neither is held.
	 */
	bool CancelPreviewDrag();

	/** The player's view component the field of view preview drives, for a menu with no controller to find the pawn through (tests). */
	void UseViewForPreview(UPlayerViewComponent* View);

	/** A frame of the previews: they fade in and out with their sliders (NativeTick does this every frame; tests step it). */
	void AdvancePreviews(float DeltaSeconds);

	USlider* GetFieldOfViewSlider() const;
	USlider* GetMinimapZoomSlider() const;
	/** The live minimap the zoom preview shows (hidden until the zoom slider is held or pointed at). */
	UHudMinimapWidget* GetMinimapZoomPreview() const;
	bool IsFieldOfViewPreviewActive() const { return FieldOfViewState.IsActive(); }
	bool IsMinimapZoomPreviewActive() const { return ZoomState.IsActive(); }
	/** How far the game's view is uncovered by the field of view preview now: 0 = the menu as usual, 1 = dimming gone, panel ghosted. */
	float GetViewFade() const { return AppliedViewFade; }

	// --- The keyboard and a controller (SettingsMenuNav.cpp) ---

	/**
	 * A key from the keyboard or a gamepad while the menu isn't waiting for a new key binding (every key comes here, and
	 * the tests send theirs): the D-pad, arrows or W A S D move the focus, or step the slider or switch it's on; A or Enter
	 * presses or changes; LB / RB or Page Up / Down turn the sections; B, Esc or Start let go of a held preview, then close
	 * the menu. The first move or press while the mouse drove the menu only brings the focus look up. bRepeat: the
	 * platform's own repeat of a held key, ignored (held keys repeat on the menu's clock, AdvanceNav). True when it was
	 * the menu's.
	 */
	bool HandleNavKey(const FKey& Key, bool bRepeat = false);
	/** A key let go: a held direction stops repeating, and a slider it stepped is let go (and saved, as after a drag). */
	void HandleNavKeyUp(const FKey& Key);
	/** The left stick's position (-1 to 1, up +Y), read as a D-pad. */
	void HandleNavStick(const FVector2D& Stick);
	/** A frame of held keys' repeats, of the device check and the focus look's breath (NativeTick; tests step it). */
	void AdvanceNav(float DeltaSeconds);
	/** The widget the focus is on: a button, a slider, or a switch's first segment. */
	UWidget* GetFocusedWidget() const;
	/** The focus look is lit on the focused stop (it is only while keys or a gamepad drive the menus). */
	bool IsFocusLookShown() const;
	/** The prompts along the bottom as they stand, for the device last used. */
	TArray<MenuNav::FPrompt> GetPrompts() const;
	ULooterButton* GetResumeButton() const;
	USlider* GetMinimapSizeSlider() const;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;
	virtual FReply NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnKeyUp(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;
	virtual FReply NativeOnAnalogValueChanged(const FGeometry& InGeometry, const FAnalogInputEvent& InAnalogEvent) override;
	virtual FReply NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FCursorReply NativeOnCursorQuery(const FGeometry& InGeometry, const FPointerEvent& InCursorEvent) override;
	virtual FNavigationReply NativeOnNavigation(const FGeometry& MyGeometry, const FNavigationEvent& InNavigationEvent,
		const FNavigationReply& InDefaultReply) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;

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

	// --- The minimap zoom and field of view sliders, and their previews (SettingsMenuPreview.cpp) ---

	/** The zoom slider moved: the setting follows at once, and the live minimap in the corner shows it. */
	UFUNCTION()
	void HandleMinimapZoomChanged(float Value);

	/** The slider was taken, by the mouse or a controller. */
	UFUNCTION()
	void HandleMinimapZoomGrabbed();

	UFUNCTION()
	void HandleMinimapZoomReleased();

	/** The field of view slider moved: the setting follows at once, and the view behind the menu shows it. */
	UFUNCTION()
	void HandleFieldOfViewChanged(float Value);

	UFUNCTION()
	void HandleFieldOfViewGrabbed();

	UFUNCTION()
	void HandleFieldOfViewReleased();

	/** The player's view, which the field of view preview drives (none in the main menu). */
	UPlayerViewComponent* FindPlayerView() const;
	/** The view behind the menu takes Degrees; the menu steps aside while the slider is held (third person has none to show). */
	void ShowFieldOfViewPreview(float Degrees);
	/** Fades the dimming and the panel for the field of view preview; Fade 0 is the menu as usual. */
	void ApplyViewFade(float Fade);

	UFUNCTION()
	void HandleLookSensitivityChanged(float Value);

	UFUNCTION()
	void HandleLookSensitivityReleased();

	UFUNCTION()
	void HandleCameraShakeChanged(float Value);

	UFUNCTION()
	void HandleCameraShakeReleased();

	UFUNCTION()
	void HandleControllerRumbleChanged(float Value);

	UFUNCTION()
	void HandleControllerRumbleReleased();
	void HandleButton(ULooterButton* Button);
	void StartListening(int32 BindingIndex);
	void StopListening();
	void AssignKey(const FKey& Key);
	void SetStatus(const FString& Message, const FLinearColor& Color);

	// --- The keyboard and a controller (SettingsMenuNav.cpp) ---

	/** A section's header as the list is built: the stops after it are that section's, and the bumpers bring it to the top. */
	UWidget* NavSection(UWidget* Header);
	/** A slider's row as the list is built: a stop of the current section, the row lit by its focus look. Returns Row. */
	UWidget* NavSlider(UWidget* Row, USlider* Slider);
	/** A row of choices (a switch, the quality presets) as the list is built. Returns Row. */
	UWidget* NavChoices(UWidget* Row, const TArray<ULooterButton*>& Choices);
	/** The stops, top to bottom: Resume, the sections, the key list, the footer. Prefer: where the focus goes (else it stays). */
	void RebuildNavigation(const UWidget* Prefer = nullptr);
	/** The focus to where the menu starts: Resume over the game, the first setting from the main menu. */
	void ResetNavigation();
	/** A direction pressed: one step now, then repeats while it's held. */
	void PressNav(const FKey& Key, MenuNav::EMove Move);
	void StepNav(MenuNav::EMove Move);
	void JumpNav(int32 Step);
	void ActivateNav();
	/** Notes Key's device. False when the focus look wasn't showing before (that first press only brings it up). */
	bool RevealNav(const FKey& Key);
	/** The look for the device driving the menus: the focus look, the hidden pointer, the prompts, the key list's hint. */
	void RefreshNavLook();
	void RefreshPrompts();
	/** Brings the focused stop into the list's view (Resume: the list's top). */
	void ScrollToFocus();
	/** Held keys stop repeating and a slider they held is let go. */
	void StopNavHold();
	/** The mouse was used: the focus look goes, the prompts name the keyboard's keys. */
	void NotePointer();
	/** The mouse moved onto a button: with the mouse in charge the focus follows it, so keys carry on from there. */
	void HandleButtonHovered(ULooterButton* Button);

	FMenuNavigator Nav;
	MenuNav::FRepeat NavRepeat;
	FVector2D NavStick = FVector2D::ZeroVector;
	MenuNav::EMove NavStickMove = MenuNav::EMove::None;
	/** The stops as the list is built: its sections, and the key list (RebuildControls). */
	TArray<FMenuNavRow> BodyNav;
	TArray<FMenuNavRow> KeyNav;
	/** The section the list's building has reached (NavSection). */
	int32 BuildSection = INDEX_NONE;
	/** The device the look was last set for (RefreshNavLook). */
	LooterUI::EMenuDevice ShownDevice = LooterUI::EMenuDevice::Mouse;
	bool bNavLookApplied = false;
	/** The prompts on show, as one line: the bar is rebuilt only when they change. */
	FString ShownPrompts;
	/** The sections' headers, in order (the bumpers scroll each to the top). */
	UPROPERTY(Transient) TArray<TObjectPtr<UWidget>> SectionHeaders;
	/** Each key binding's Default button, beside its key. */
	UPROPERTY(Transient) TArray<TObjectPtr<ULooterButton>> DefaultButtons;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> ResetAllButton;
	/** The key hints along the bottom ("A Select  B Back"). */
	UPROPERTY(Transient) TObjectPtr<UHorizontalBox> PromptBar;
	/** The line over the key list: how to change a key, for the keyboard or a gamepad. */
	UPROPERTY(Transient) TObjectPtr<UTextBlock> KeyHintText;

	ESettingsMenuMode Mode = ESettingsMenuMode::Pause;

	/** Everything between the header and the footer, scrolling as one: the sections and, last, the key list. */
	UPROPERTY(Transient) TObjectPtr<UScrollBox> Body;
	/** The key bindings' rows (RebuildControls fills it). */
	UPROPERTY(Transient) TObjectPtr<UVerticalBox> KeyList;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> StatusText;
	/** "Game paused", over the game (the prompts along the bottom say how to resume). */
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
	/** The contextual control hints' switch. */
	UPROPERTY(Transient) TObjectPtr<ULooterButton> HintsOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> HintsOff;
	/** The subtitles' switch (the spoken lines' words on screen). */
	UPROPERTY(Transient) TObjectPtr<ULooterButton> SubtitlesOn;
	UPROPERTY(Transient) TObjectPtr<ULooterButton> SubtitlesOff;
	UPROPERTY(Transient) TObjectPtr<USlider> TransparencySlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> TransparencyValue;
	UPROPERTY(Transient) TObjectPtr<USlider> MinimapSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapValue;
	UPROPERTY(Transient) TObjectPtr<USlider> MinimapZoomSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapZoomValue;
	/** The look sensitivity, as a multiple of the game's own turn. */
	UPROPERTY(Transient) TObjectPtr<USlider> LookSensitivitySlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> LookSensitivityValue;
	/** How strongly the view kicks and shakes, 0-100%. */
	UPROPERTY(Transient) TObjectPtr<USlider> CameraShakeSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> CameraShakeValue;
	/** How strongly a gamepad rumbles (shots, hits taken), 0-100%. */
	UPROPERTY(Transient) TObjectPtr<USlider> ControllerRumbleSlider;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> ControllerRumbleValue;
	/** The volume sliders and their percentages, in EAudioVolume's order. */
	UPROPERTY(Transient) TArray<TObjectPtr<USlider>> VolumeSliders;
	UPROPERTY(Transient) TArray<TObjectPtr<UTextBlock>> VolumeValues;
	UPROPERTY(Transient) TObjectPtr<UWidget> MinimapPreview;
	UPROPERTY(Transient) TObjectPtr<USizeBox> MinimapPreviewSize;
	UPROPERTY(Transient) TObjectPtr<UTextBlock> MinimapPreviewCaption;
	/** The preview's outer outline, out at the HUD minimap's bezel (the map's circle and glass sit inside it). */
	UPROPERTY(Transient) TObjectPtr<UImage> MinimapPreviewBezel;

	/** The live minimap the zoom preview shows in the HUD minimap's corner (SetPreview). */
	UPROPERTY(Transient) TObjectPtr<UHudMinimapWidget> MinimapZoomPreview;
	/** The dimming over the game, and the panel in front of it: the field of view preview fades both. */
	UPROPERTY(Transient) TObjectPtr<UBorder> Dimmer;
	UPROPERTY(Transient) TObjectPtr<USizeBox> Panel;

	/**
	 * The previews, each up while its slider is held (or pointed at, for the minimap's two), then a moment after. The minimap's
	 * size and zoom sliders show in the same corner, so whichever is held or pointed at now takes it.
	 */
	SettingsMenu::FSliderPreview SizeState;
	SettingsMenu::FSliderPreview ZoomState;
	SettingsMenu::FSliderPreview FieldOfViewState;
	/** The field of view change shows in the view behind the menu (first person), so the menu steps aside while it is up. */
	bool bFieldOfViewInView = false;
	/** How far the menu has stepped aside (ApplyViewFade). */
	float AppliedViewFade = 0.f;
	/** Set while the sliders are given their saved values: that is not a move, and previews nothing. */
	bool bRefreshingSliders = false;
	TWeakObjectPtr<UPlayerViewComponent> ViewForPreview;

	/** The volume slider moved last, for the status line when it's let go. */
	EAudioVolume LastVolumeMoved = EAudioVolume::Master;

	/** Index into the subsystem's binding list we're waiting on a key for, or INDEX_NONE. */
	int32 ListeningIndex = INDEX_NONE;
};
