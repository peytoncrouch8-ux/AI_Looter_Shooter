#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
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
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/WidgetSwitcher.h"
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

	// The keyboard and controller focus starts at the top again, on the first page.
	NavStick = FVector2D::ZeroVector;
	NavStickMove = MenuNav::EMove::None;
	// Opened by the controller's Start (still down as the menu opens): the controller drives it from its first press.
	if (const APlayerController* PC = GetOwningPlayer(); PC && PC->IsInputKeyDown(EKeys::Gamepad_Special_Right))
	{
		LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Gamepad);
	}
	ShowPage(static_cast<int32>(ESettingsPage::Graphics));
	ResetNavigation();
	RefreshNavLook();
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

		// The tabs, then the page on show beside the description column. Every page is built now, each its own scrolling
		// list in the switcher, so turning a tab only shows another (nothing is made then).
		HelpEntries.Reset();
		Add(MakeTabBar(), 14.f);
		UHorizontalBox* Middle = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
		UHorizontalBoxSlot* PagesSlot = Middle->AddChildToHorizontalBox(Pages);
		PagesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UHorizontalBoxSlot* HelpSlot = Middle->AddChildToHorizontalBox(MakeHelpColumn());
		HelpSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
		HelpSlot->SetVerticalAlignment(VAlign_Fill);
		Add(Middle, 12.f, true);

		// The keyboard and controller stops and the descriptions are noted as the rows are made, page by page
		// (SettingsMenuNav.cpp, SettingsMenuTabs.cpp).
		PageNav.Reset();
		PageLists.Reset();
		BuildPage = INDEX_NONE;
		MakeGraphicsPage();
		MakeAudioPage();
		MakeInterfacePage();
		MakeControlsPage();
		BuildRows = nullptr;

		StatusText = MakeText(WidgetTree, TEXT(""), 13, Color::TextDim());
		StatusText->SetAutoWrapText(true);
		Add(StatusText, 8.f);

		// Save & Quit over the game or Back in the main menu (ApplyMode shows one of them), at the right.
		UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Footer->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, 0.f))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SaveQuitButton = MakeButton(ActionSaveQuit, 0, TEXT("Save & Quit"), 14, EButtonKind::Danger);
		Footer->AddChildToHorizontalBox(MakeSized(WidgetTree, SaveQuitButton, HelpColumnWidth));
		BackButton = MakeButton(ActionClose, 0, TEXT("Back"), 14, EButtonKind::Primary);
		Footer->AddChildToHorizontalBox(MakeSized(WidgetTree, BackButton, HelpColumnWidth));
		Describe(ResumeButton, RowHelp(ESettingRow::Resume), true);
		Describe(SaveQuitButton, RowHelp(ESettingRow::SaveQuit), true);
		Describe(BackButton, RowHelp(ESettingRow::Back), true);
		Add(Footer, 10.f);

		// What the keys or the controller's buttons do now, along the bottom (SettingsMenuNav.cpp fills it).
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		Add(PromptBar, 10.f);

		// Close button in the frame corner does the same as Resume or Back.
		UWidget* CloseSize = MakeSized(WidgetTree, MakeButton(ActionClose, 0, TEXT("X"), 15, EButtonKind::Normal), 34.f, 34.f);

		// One size for every page, so turning a tab never moves the frame (a long page scrolls inside it).
		Panel = MakeSized(WidgetTree, MakePanel(WidgetTree, TEXT("Settings"), Column, CloseSize), PanelWidth, PanelHeight);
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
		ShowPage(static_cast<int32>(ESettingsPage::Graphics));
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
	// A two-way switch lights the side that is the setting.
	auto ShowSwitch = [](ULooterButton* OnSide, ULooterButton* OffSide, bool bOn)
	{
		if (OnSide && OffSide)
		{
			OnSide->SetHighlighted(bOn);
			OffSide->SetHighlighted(!bOn);
		}
	};
	ShowSwitch(MotionBlurOn, MotionBlurOff, Graphics->IsMotionBlurEnabled());
	ShowSwitch(MinimapOn, MinimapOff, Graphics->IsMinimapShown());
	ShowSwitch(FrameRateOn, FrameRateOff, Graphics->IsFrameRateShown());
	ShowSwitch(HintsOn, HintsOff, Graphics->AreControlHintsShown());
	ShowSwitch(SubtitlesOn, SubtitlesOff, Graphics->AreSubtitlesShown());
	for (int32 Index = 0; Index < QualityButtons.Num(); ++Index)
	{
		QualityButtons[Index]->SetHighlighted(static_cast<int32>(Graphics->GetQuality()) == Index);
	}
	for (int32 Index = 0; Index < CrosshairColourButtons.Num(); ++Index)
	{
		CrosshairColourButtons[Index]->SetHighlighted(static_cast<int32>(Graphics->GetCrosshairColour()) == Index);
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
	if (Controls && ControllerRumbleSlider && ControllerRumbleValue)
	{
		const float Rumble = Controls->GetControllerRumble();
		ControllerRumbleSlider->SetValue(Rumble);
		ControllerRumbleValue->SetText(FText::FromString(PercentText(Rumble)));
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
