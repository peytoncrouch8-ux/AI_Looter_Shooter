#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "Audio/LooterSound.h"
#include "Player/PlayerViewComponent.h"
#include "Settings/AudioSettingsSubsystem.h"
#include "Settings/ControlSettingsSubsystem.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

using namespace SettingsMenu;

void USettingsMenuWidget::Open(ESettingsMenuMode InMode)
{
	Mode = InMode;
	// Nothing held by a key from the last time (a slider let go now saves, before the status is cleared below).
	StopNavHold();
	StopListening();
	SetIsFocusable(true);
	ApplyMode();
	RefreshKeyLabels();
	RefreshGraphics();
	RefreshAudio();
	RefreshControlSettings();
	SetStatus(TEXT(""), LooterUI::Color::TextDim());
	// Over the game the HUD sounds its pages opening and closing (the pause menu among them); the main menu has no HUD.
	if (Mode == ESettingsMenuMode::MainMenu)
	{
		LooterSound::Play2D(this, LooterSoundCue::Open);
	}

	// No preview carries over from the last time it was open (the view goes back to the saved field of view, the menu to
	// its usual look, the minimap's two previews away).
	SizeState.Reset();
	ZoomState.Reset();
	FieldOfViewState.Reset();
	bFieldOfViewInView = false;
	if (UPlayerViewComponent* View = FindPlayerView())
	{
		View->EndFieldOfViewPreview();
	}
	ApplyViewFade(0.f);
	if (MinimapPreview)
	{
		MinimapPreview->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (MinimapZoomPreview)
	{
		MinimapZoomPreview->SetPreviewZoom(0.f);
		MinimapZoomPreview->SetVisibility(ESlateVisibility::Collapsed);
	}

	// The keyboard and controller focus starts at the top again.
	NavStick = FVector2D::ZeroVector;
	NavStickMove = MenuNav::EMove::None;
	// Opened by the controller's Start (still down as the menu opens): the controller drives it from its first press.
	if (const APlayerController* PC = GetOwningPlayer(); PC && PC->IsInputKeyDown(EKeys::Gamepad_Special_Right))
	{
		LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Gamepad);
	}
	ResetNavigation();
	RefreshNavLook();
	if (LooterUI::IsMenuFocusShown() && Body)
	{
		Body->ScrollToStart();
	}
}

void USettingsMenuWidget::ApplyMode()
{
	// Over the game it pauses: Resume, and Save & Quit back to the main menu. From the main menu it only goes back.
	const bool bPause = Mode == ESettingsMenuMode::Pause;
	if (HeaderText)
	{
		// How to resume or go back is the prompts' to say, for the keyboard or the controller.
		HeaderText->SetText(FText::FromString(TEXT("Game paused")));
		HeaderText->SetVisibility(bPause ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
	const ESlateVisibility PauseOnly = bPause ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (ResumeButton) { ResumeButton->SetVisibility(PauseOnly); }
	if (SaveQuitButton) { SaveQuitButton->SetVisibility(PauseOnly); }
	if (BackButton) { BackButton->SetVisibility(bPause ? ESlateVisibility::Collapsed : ESlateVisibility::Visible); }
}

void USettingsMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvancePreviews(InDeltaTime);
	AdvanceNav(InDeltaTime);
}

UKeyBindingSubsystem* USettingsMenuWidget::GetBindings() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
}

UGraphicsSettingsSubsystem* USettingsMenuWidget::GetGraphics() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
}

UControlSettingsSubsystem* USettingsMenuWidget::GetControls() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UControlSettingsSubsystem>() : nullptr;
}

UAudioSettingsSubsystem* USettingsMenuWidget::GetAudio() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UAudioSettingsSubsystem>() : nullptr;
}

ULooterButton* USettingsMenuWidget::MakeButton(FName Action, int32 Index, const FString& Label, int32 FontSize, LooterUI::EButtonKind Kind)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, Index, FText::FromString(Label), FontSize, Kind);
	Button->OnButtonClicked.BindUObject(this, &USettingsMenuWidget::HandleButton);
	Button->OnButtonHovered.BindUObject(this, &USettingsMenuWidget::HandleButtonHovered);
	// Resume, Back and the corner's X close the menu: the page's close (over the game the HUD plays the same cue in the
	// same moment, and the two are heard as one).
	if (Action == ActionClose)
	{
		Button->ClickCue = LooterSoundCue::Close;
	}
	return Button;
}

TSharedRef<SWidget> USettingsMenuWidget::RebuildWidget()
{
	using namespace LooterUI;
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The dimming over the game: a layer of its own (it takes the clicks), so the field of view preview can fade it apart
		// from the panel in front of it and let the view show.
		Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dimmer"));
		Dimmer->SetBrush(RectBrush(Color::Backdrop()));
		Dimmer->SetVisibility(ESlateVisibility::Visible);
		MarkBackground(Dimmer);
		UOverlaySlot* DimmerSlot = Root->AddChildToOverlay(Dimmer);
		DimmerSlot->SetHorizontalAlignment(HAlign_Fill);
		DimmerSlot->SetVerticalAlignment(VAlign_Fill);

		UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		auto Add = [Column](UWidget* Child, float Top, bool bFill = false)
		{
			UVerticalBoxSlot* ChildSlot = Column->AddChildToVerticalBox(Child);
			ChildSlot->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
			if (bFill)
			{
				ChildSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
			}
		};

		HeaderText = MakeText(WidgetTree, TEXT(""), 12, Color::TextDim(), true, 150);
		Add(HeaderText, 0.f);
		ResumeButton = MakeButton(ActionClose, 0, TEXT("Resume"), 16, EButtonKind::Primary);
		Add(ResumeButton, 10.f);

		// Every section in one list that scrolls between the header and the footer, so the key bindings at its end are in
		// reach at any screen size, whatever the sections above them take.
		Body = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		StyleScrollBox(Body);
		Add(Body, 18.f, true);
		UVerticalBox* Sections = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Body->AddChild(Sections);
		auto AddRow = [Sections](UWidget* Child, float Top)
		{
			Sections->AddChildToVerticalBox(Child)->SetPadding(FMargin(0.f, Top, 0.f, 0.f));
		};

		// The keyboard and controller stops are noted as the rows are made, section by section (SettingsMenuNav.cpp).
		BodyNav.Reset();
		SectionHeaders.Reset();
		BuildSection = INDEX_NONE;

		AddRow(NavSection(MakeSection(WidgetTree, TEXT("Graphics"))), 0.f);
		TArray<FString> QualityNames;
		for (const EGraphicsQuality Quality : Qualities)
		{
			QualityNames.Add(UGraphicsSettingsSubsystem::QualityName(Quality));
		}
		TArray<ULooterButton*> Quality;
		UWidget* QualityRow = MakeChoiceRow(TEXT("Quality"), QualityNames, ActionQuality, QualityColumnWidth, Quality);
		AddRow(NavChoices(QualityRow, Quality), 4.f);
		for (ULooterButton* Button : Quality)
		{
			QualityButtons.Add(Button);
		}
		ULooterButton* BlurOn = nullptr;
		ULooterButton* BlurOff = nullptr;
		UWidget* BlurRow = MakeToggleRow(TEXT("Motion blur"), TEXT("On"), TEXT("Off"), ActionMotionBlurOn, ActionMotionBlurOff, 0, false, BlurOn, BlurOff);
		AddRow(NavChoices(BlurRow, { BlurOn, BlurOff }), 4.f);
		MotionBlurOn = BlurOn;
		MotionBlurOff = BlurOff;
		// The first-person view only: third person keeps its own over-the-shoulder framing.
		USlider* FovSlider = nullptr;
		UTextBlock* FovText = nullptr;
		UWidget* FovRow = MakeSliderRow(TEXT("Field of view (1st person)"), UGraphicsSettingsSubsystem::MinFieldOfView,
			UGraphicsSettingsSubsystem::MaxFieldOfView, FovSlider, FovText, FieldOfViewStep);
		AddRow(NavSlider(FovRow, FovSlider), 4.f);
		FieldOfViewSlider = FovSlider;
		FieldOfViewValue = FovText;
		FieldOfViewSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewChanged);
		// Taken and let go by the mouse or a controller (a controller's lock on a focused slider), the same to the preview.
		FieldOfViewSlider->OnMouseCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewGrabbed);
		FieldOfViewSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewReleased);
		FieldOfViewSlider->OnControllerCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewGrabbed);
		FieldOfViewSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleFieldOfViewReleased);

		AddRow(NavSection(MakeSection(WidgetTree, TEXT("Audio"))), 18.f);
		AddRow(MakeAudioRows(), 0.f);

		AddRow(NavSection(MakeSection(WidgetTree, TEXT("Interface"))), 18.f);
		USlider* Transparency = nullptr;
		UTextBlock* TransparencyText = nullptr;
		UWidget* TransparencyRow = MakeSliderRow(TEXT("UI transparency"), 0.f, 1.f, Transparency, TransparencyText);
		AddRow(NavSlider(TransparencyRow, Transparency), 4.f);
		TransparencySlider = Transparency;
		TransparencyValue = TransparencyText;
		TransparencySlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleTransparencyChanged);
		// Let go by the mouse, or by the key or stick that stepped it (the menu's keys hold a slider as a controller does).
		TransparencySlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleTransparencyReleased);
		TransparencySlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleTransparencyReleased);
		ULooterButton* MapOn = nullptr;
		ULooterButton* MapOff = nullptr;
		UWidget* MapRow = MakeToggleRow(TEXT("Minimap"), TEXT("On"), TEXT("Off"), ActionMinimapOn, ActionMinimapOff, 0, false, MapOn, MapOff);
		AddRow(NavChoices(MapRow, { MapOn, MapOff }), 4.f);
		MinimapOn = MapOn;
		MinimapOff = MapOff;
		USlider* MinimapSize = nullptr;
		UTextBlock* MinimapText = nullptr;
		UWidget* MinimapSizeRow = MakeSliderRow(TEXT("Minimap size"), UGraphicsSettingsSubsystem::MinMinimapScale,
			UGraphicsSettingsSubsystem::MaxMinimapScale, MinimapSize, MinimapText);
		AddRow(NavSlider(MinimapSizeRow, MinimapSize), 4.f);
		MinimapSlider = MinimapSize;
		MinimapValue = MinimapText;
		MinimapSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeChanged);
		MinimapSlider->OnMouseCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeGrabbed);
		MinimapSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeReleased);
		MinimapSlider->OnControllerCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeGrabbed);
		MinimapSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeReleased);
		USlider* MinimapZoom = nullptr;
		UTextBlock* MinimapZoomText = nullptr;
		UWidget* MinimapZoomRow = MakeSliderRow(TEXT("Minimap zoom"), UGraphicsSettingsSubsystem::MinMinimapZoom,
			UGraphicsSettingsSubsystem::MaxMinimapZoom, MinimapZoom, MinimapZoomText);
		AddRow(NavSlider(MinimapZoomRow, MinimapZoom), 4.f);
		MinimapZoomSlider = MinimapZoom;
		MinimapZoomValue = MinimapZoomText;
		MinimapZoomSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomChanged);
		MinimapZoomSlider->OnMouseCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomGrabbed);
		MinimapZoomSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomReleased);
		MinimapZoomSlider->OnControllerCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomGrabbed);
		MinimapZoomSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomReleased);
		ULooterButton* RateOn = nullptr;
		ULooterButton* RateOff = nullptr;
		UWidget* RateRow = MakeToggleRow(TEXT("FPS counter"), TEXT("On"), TEXT("Off"), ActionFrameRateOn, ActionFrameRateOff, 0, false, RateOn, RateOff);
		AddRow(NavChoices(RateRow, { RateOn, RateOff }), 4.f);
		FrameRateOn = RateOn;
		FrameRateOff = RateOff;
		// A key and what it does, near the mission tracker, until the player has used that control.
		ULooterButton* ShowHints = nullptr;
		ULooterButton* HideHints = nullptr;
		UWidget* HintsRow = MakeToggleRow(TEXT("Control hints"), TEXT("On"), TEXT("Off"), ActionHintsOn, ActionHintsOff, 0, false, ShowHints, HideHints);
		AddRow(NavChoices(HintsRow, { ShowHints, HideHints }), 4.f);
		HintsOn = ShowHints;
		HintsOff = HideHints;

		AddRow(NavSection(MakeSection(WidgetTree, TEXT("Controls"))), 18.f);
		// Mouse and stick alike; a zoomed sight still slows the turn by its zoom on top of it.
		USlider* LookSlider = nullptr;
		UTextBlock* LookValueText = nullptr;
		UWidget* LookRow = MakeSliderRow(TEXT("Look sensitivity"), UControlSettingsSubsystem::MinLookSensitivity,
			UControlSettingsSubsystem::MaxLookSensitivity, LookSlider, LookValueText, UControlSettingsSubsystem::LookSensitivityStep);
		AddRow(NavSlider(LookRow, LookSlider), 4.f);
		LookSensitivitySlider = LookSlider;
		LookSensitivityValue = LookValueText;
		LookSensitivitySlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleLookSensitivityChanged);
		LookSensitivitySlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleLookSensitivityReleased);
		LookSensitivitySlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleLookSensitivityReleased);
		// Every kick and shake of the view (shots, hits, kills, a boss's slam); 0% keeps it still.
		USlider* ShakeSlider = nullptr;
		UTextBlock* ShakeValueText = nullptr;
		UWidget* ShakeRow = MakeSliderRow(TEXT("Camera shake"), UControlSettingsSubsystem::MinCameraShake,
			UControlSettingsSubsystem::MaxCameraShake, ShakeSlider, ShakeValueText, UControlSettingsSubsystem::CameraShakeStep);
		AddRow(NavSlider(ShakeRow, ShakeSlider), 4.f);
		CameraShakeSlider = ShakeSlider;
		CameraShakeValue = ShakeValueText;
		CameraShakeSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleCameraShakeChanged);
		CameraShakeSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleCameraShakeReleased);
		CameraShakeSlider->OnControllerCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleCameraShakeReleased);
		// The key list's own section for the bumpers, headed by how to change a key (RefreshNavLook words it per device).
		KeyHintText = MakeText(WidgetTree, TEXT(""), 11, Color::TextDim());
		KeyHintText->SetAutoWrapText(true);
		AddRow(NavSection(KeyHintText), 8.f);
		KeyList = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		AddRow(KeyList, 8.f);

		StatusText = MakeText(WidgetTree, TEXT(""), 13, Color::TextDim());
		StatusText->SetAutoWrapText(true);
		Add(StatusText, 8.f);

		// Reset all, then Save & Quit over the game or Back in the main menu (ApplyMode shows one of them).
		UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		ResetAllButton = MakeButton(ActionResetAll, 0, TEXT("Reset all"), 14);
		UHorizontalBoxSlot* ResetSlot = Footer->AddChildToHorizontalBox(ResetAllButton);
		ResetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ResetSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		SaveQuitButton = MakeButton(ActionSaveQuit, 0, TEXT("Save & Quit"), 14, EButtonKind::Danger);
		Footer->AddChildToHorizontalBox(SaveQuitButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BackButton = MakeButton(ActionClose, 0, TEXT("Back"), 14, EButtonKind::Primary);
		Footer->AddChildToHorizontalBox(BackButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Add(Footer, 10.f);

		// What the keys or the controller's buttons do now, along the bottom (SettingsMenuNav.cpp fills it).
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Add(PromptBar, 10.f);

		// Close button in the frame corner does the same as Resume or Back.
		UWidget* CloseSize = MakeSized(WidgetTree, MakeButton(ActionClose, 0, TEXT("X"), 15, EButtonKind::Normal), 34.f, 34.f);

		Panel = MakeSized(WidgetTree, MakePanel(WidgetTree, TEXT("Settings"), Column, CloseSize), 640.f);
		Panel->SetMaxDesiredHeight(860.f);
		UOverlaySlot* PanelSlot = Root->AddChildToOverlay(Panel);
		PanelSlot->SetHorizontalAlignment(HAlign_Center);
		PanelSlot->SetVerticalAlignment(VAlign_Center);

		// On top, in the HUD minimap's corner: its outline, shown while its size is being set, and a live copy of the
		// minimap while its zoom is (the HUD's own is put away under the menu).
		UCanvasPanel* PreviewLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		PreviewLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* PreviewSlot = Root->AddChildToOverlay(PreviewLayer);
		PreviewSlot->SetHorizontalAlignment(HAlign_Fill);
		PreviewSlot->SetVerticalAlignment(VAlign_Fill);
		auto PlaceInCorner = [PreviewLayer](UWidget* Widget)
		{
			UCanvasPanelSlot* CornerSlot = PreviewLayer->AddChildToCanvas(Widget);
			CornerSlot->SetAnchors(FAnchors(1.f, 0.f));
			CornerSlot->SetAlignment(FVector2D(1.f, 0.f));
			CornerSlot->SetPosition(FVector2D(-UHudMinimapWidget::Margin, UHudMinimapWidget::Margin));
			CornerSlot->SetAutoSize(true);
		};
		MinimapPreview = MakeMinimapPreview();
		MinimapPreview->SetVisibility(ESlateVisibility::Collapsed);
		PlaceInCorner(MinimapPreview);
		MinimapZoomPreview = WidgetTree->ConstructWidget<UHudMinimapWidget>(UHudMinimapWidget::StaticClass());
		MinimapZoomPreview->SetPreview(true);
		MinimapZoomPreview->SetVisibility(ESlateVisibility::Collapsed);
		PlaceInCorner(MinimapZoomPreview);

		RebuildControls();
		RefreshGraphics();
		RefreshAudio();
		RefreshControlSettings();
		ApplyMode();
		ResetNavigation();
		RefreshNavLook();
	}
	return Super::RebuildWidget();
}

void USettingsMenuWidget::RefreshGraphics()
{
	const UGraphicsSettingsSubsystem* Graphics = GetGraphics();
	if (!Graphics || !MotionBlurOn || !MotionBlurOff)
	{
		return;
	}
	// Giving a slider its saved value fires its change event: not a move, so nothing previews.
	TGuardValue<bool> Refreshing(bRefreshingSliders, true);
	const bool bBlur = Graphics->IsMotionBlurEnabled();
	MotionBlurOn->SetHighlighted(bBlur);
	MotionBlurOff->SetHighlighted(!bBlur);
	if (MinimapOn && MinimapOff)
	{
		const bool bMinimap = Graphics->IsMinimapShown();
		MinimapOn->SetHighlighted(bMinimap);
		MinimapOff->SetHighlighted(!bMinimap);
	}
	if (FrameRateOn && FrameRateOff)
	{
		const bool bFrameRate = Graphics->IsFrameRateShown();
		FrameRateOn->SetHighlighted(bFrameRate);
		FrameRateOff->SetHighlighted(!bFrameRate);
	}
	if (HintsOn && HintsOff)
	{
		const bool bHints = Graphics->AreControlHintsShown();
		HintsOn->SetHighlighted(bHints);
		HintsOff->SetHighlighted(!bHints);
	}
	for (int32 Index = 0; Index < QualityButtons.Num(); ++Index)
	{
		QualityButtons[Index]->SetHighlighted(static_cast<int32>(Graphics->GetQuality()) == Index);
	}

	if (FieldOfViewSlider && FieldOfViewValue)
	{
		const float Degrees = Graphics->GetFirstPersonFieldOfView();
		FieldOfViewSlider->SetValue(Degrees);
		FieldOfViewValue->SetText(FText::FromString(DegreesText(Degrees)));
	}
	if (TransparencySlider && TransparencyValue)
	{
		const float Transparency = Graphics->GetUITransparency();
		TransparencySlider->SetValue(Transparency);
		TransparencyValue->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Transparency * 100.f))));
	}
	if (MinimapSlider)
	{
		const float Scale = Graphics->GetMinimapScale();
		MinimapSlider->SetValue(Scale);
		ShowMinimapScale(Scale);
	}
	if (MinimapZoomSlider && MinimapZoomValue)
	{
		const float Zoom = Graphics->GetMinimapZoom();
		MinimapZoomSlider->SetValue(Zoom);
		MinimapZoomValue->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), Zoom)));
	}
}

void USettingsMenuWidget::RefreshControlSettings()
{
	const UControlSettingsSubsystem* Controls = GetControls();
	if (Controls && LookSensitivitySlider && LookSensitivityValue)
	{
		const float Sensitivity = Controls->GetLookSensitivity();
		LookSensitivitySlider->SetValue(Sensitivity);
		LookSensitivityValue->SetText(FText::FromString(LookSensitivityText(Sensitivity)));
	}
	if (Controls && CameraShakeSlider && CameraShakeValue)
	{
		const float Shake = Controls->GetCameraShake();
		CameraShakeSlider->SetValue(Shake);
		CameraShakeValue->SetText(FText::FromString(PercentText(Shake)));
	}
}

void USettingsMenuWidget::ShowMinimapScale(float Scale)
{
	if (MinimapValue)
	{
		MinimapValue->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Scale * 100.f))));
	}
	if (MinimapPreviewSize)
	{
		const float Diameter = UHudMinimapWidget::Diameter * Scale;
		MinimapPreviewSize->SetWidthOverride(Diameter);
		MinimapPreviewSize->SetHeightOverride(Diameter);
		if (UOverlaySlot* BezelSlot = MinimapPreviewBezel ? Cast<UOverlaySlot>(MinimapPreviewBezel->Slot) : nullptr)
		{
			// The bezel reaches this far past the map's edge, all round (it grows with the map).
			BezelSlot->SetPadding(FMargin(-UHudMinimapWidget::BezelReach * Scale));
		}
		if (UOverlaySlot* CaptionSlot = MinimapPreviewCaption ? Cast<UOverlaySlot>(MinimapPreviewCaption->Slot) : nullptr)
		{
			CaptionSlot->SetPadding(FMargin(0.f, Diameter * 0.62f, 0.f, 0.f));
		}
	}
}

void USettingsMenuWidget::SetStatus(const FString& Message, const FLinearColor& Color)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}
