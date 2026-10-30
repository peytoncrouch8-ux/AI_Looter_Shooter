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
	// The plate drops in again and the halo spins up, the ribbon at the top row over the game (or the page's first row).
	BeginOpenMotion();
}

void USettingsMenuWidget::ApplyMode()
{
	// Either way it only goes back: over the game to the pause menu's list (GAME PAUSED, Resume and Save & Quit are the
	// list's since the user's call of 2026-10-29 night), from the main menu to it.
	const bool bPause = Mode == ESettingsMenuMode::Pause;
	if (HeaderText)
	{
		// How to go back is the prompts' to say, for the keyboard or the controller.
		HeaderText->SetText(FText::FromString(TEXT("SETTINGS")));
		HeaderText->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	// The reticle row under the title is Back in both; the footer's own Back chip stays hidden, so going back is always the
	// top row.
	if (ResumeWord) { ResumeWord->SetText(FText::FromString(TEXT("BACK"))); }
	for (auto& Entry : HelpEntries)
	{
		if (Entry.Target == ResumeButton)
		{
			Entry.Help = RowHelp(bPause ? ESettingRow::BackToPause : ESettingRow::Back);
		}
	}
	if (BackButton) { BackButton->SetVisibility(ESlateVisibility::Collapsed); }
	// The area's plate, the serial and the halo's pause bars are the pause menu's (SettingsMenuLedger.cpp).
	RefreshPlate();
}

void USettingsMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvancePreviews(InDeltaTime);
	AdvanceNav(InDeltaTime);
	AdvanceLook(InDeltaTime);
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
	// Back and the corner's X close the menu: the page's close (over the game the HUD plays the same cue in the same
	// moment, and the two are heard as one).
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

		// The title and the area's plate beside the halo, Resume in its reticle under them (SettingsMenuLedger.cpp).
		Add(MakeHeader(), 0.f);
		Add(MakeResumeRow(), 14.f);

		// The tabs, then the page on show beside the description column. Every page is built now, each its own scrolling
		// list in the switcher, so turning a tab only shows another (nothing is made then). Under the pages, the ledger's
		// margin rule runs the page's height.
		HelpEntries.Reset();
		// Far enough down that the PGUP key clears the halo's lowest ticks.
		Add(MakeTabBar(), 30.f);
		UHorizontalBox* Middle = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UOverlay* PageArea = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USettingsMenuArt* Margin = USettingsMenuArt::Make(WidgetTree, ESettingsArt::MarginRule);
		Margin->SetMarginX(LedgerMargin);
		FillOverlaySlot(PageArea->AddChildToOverlay(Margin));
		Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
		FillOverlaySlot(PageArea->AddChildToOverlay(Pages));
		UHorizontalBoxSlot* PagesSlot = Middle->AddChildToHorizontalBox(PageArea);
		PagesSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		UHorizontalBoxSlot* HelpSlot = Middle->AddChildToHorizontalBox(MakeHelpColumn());
		HelpSlot->SetPadding(FMargin(16.f, 0.f, 0.f, 0.f));
		HelpSlot->SetVerticalAlignment(VAlign_Fill);
		Add(Middle, 10.f, true);

		// The keyboard and controller stops, the descriptions and the ledger's rows are noted as the rows are made, page by
		// page (SettingsMenuNav.cpp, SettingsMenuTabs.cpp, SettingsMenuLedger.cpp).
		PageNav.Reset();
		PageLists.Reset();
		LedgerLines.Reset();
		PageRowNumbers.Reset();
		BuildPage = INDEX_NONE;
		MakeGraphicsPage();
		MakeAudioPage();
		MakeInterfacePage();
		MakeControlsPage();
		BuildRows = nullptr;

		StatusText = MakeText(WidgetTree, TEXT(""), 13, Color::SlateLabel());
		StatusText->SetAutoWrapText(true);
		Add(StatusText, 6.f);

		// The serial at the left (over the game), the hidden Back chip at the right. Save & Quit's stamp was here until it
		// moved to the pause menu's list (the user, 2026-10-29 night).
		Add(MakeFooter(), 4.f);
		Describe(ResumeButton, RowHelp(Mode == ESettingsMenuMode::Pause ? ESettingRow::BackToPause : ESettingRow::Back), true);
		Describe(BackButton, RowHelp(ESettingRow::Back), true);

		// The plate: its glass (a background), its double trim and rivets, its contents, and the free art over them (the halo
		// breaking out over the top-left corner, the corner's range arc, the ribbon), which takes no clicks.
		Plate = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
		USettingsMenuArt* Glass = USettingsMenuArt::Make(WidgetTree, ESettingsArt::PlateFill);
		MarkBackground(Glass);
		FillOverlaySlot(Plate->AddChildToOverlay(Glass));
		FillOverlaySlot(Plate->AddChildToOverlay(USettingsMenuArt::Make(WidgetTree, ESettingsArt::PlateLines)));
		FillOverlaySlot(Plate->AddChildToOverlay(Column), FMargin(PlatePadX, PlatePadTop, PlatePadX, PlatePadBottom));
		PlateArt = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		PlateArt->SetVisibility(ESlateVisibility::HitTestInvisible);
		FillOverlaySlot(Plate->AddChildToOverlay(PlateArt));
		MakePlateArt();

		// What the keys or the controller's buttons do now, as hollow keys under the plate (SettingsMenuNav.cpp fills it).
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());

		// One size for every page, so turning a tab never moves the plate (a long page scrolls inside it). The panel is the
		// plate and the keys under it: it drops in as it opens and ghosts for the field of view's preview.
		UVerticalBox* Stack = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());
		Stack->AddChildToVerticalBox(MakeSized(WidgetTree, Plate, PanelWidth, PanelHeight));
		UVerticalBoxSlot* PromptSlot = Stack->AddChildToVerticalBox(PromptBar);
		PromptSlot->SetHorizontalAlignment(HAlign_Center);
		PromptSlot->SetPadding(FMargin(0.f, 22.f, 0.f, 0.f));
		Panel = MakeSized(WidgetTree, Stack, 0.f);
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
