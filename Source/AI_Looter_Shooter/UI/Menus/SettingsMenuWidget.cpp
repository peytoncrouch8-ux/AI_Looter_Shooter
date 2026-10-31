// USettingsMenuWidget's frame: opening it, the page's layout on its canvas (the shade, the header, the tabs, the rows' list,
// the help card, the status line and the key hints, the previews over all), and giving the controls their saved values.

#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuHelp.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Menus/SettingsPageArt.h"
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
#include "Components/WidgetSwitcher.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

using namespace SettingsMenu;
using namespace SettingsMenu::Layout;

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
	SetStatus(TEXT(""), LooterUI::Color::SlateLabel());
	// Over the game the HUD sounds its pages opening and closing (the pause menu among them); the main menu has no HUD.
	if (Mode == ESettingsMenuMode::MainMenu)
	{
		LooterSound::Play2D(this, LooterSoundCue::Open);
	}

	// No preview carries over from the last time it was open (the view goes back to the saved field of view, the page to
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

	// The keyboard and controller focus starts on the first page's first setting (the mockup: row 01 lit).
	NavStick = FVector2D::ZeroVector;
	NavStickMove = MenuNav::EMove::None;
	// Opened by the controller's Start (still down as the page opens): the controller drives it from its first press.
	if (const APlayerController* PC = GetOwningPlayer(); PC && PC->IsInputKeyDown(EKeys::Gamepad_Special_Right))
	{
		LooterUI::SetMenuDevice(LooterUI::EMenuDevice::Gamepad);
	}
	ShowPage(static_cast<int32>(ESettingsPage::Graphics));
	ResetNavigation();
	RefreshNavLook();
	// The dial swings up, the title, tabs, rows, card and hints arrive; the first row lit.
	BeginOpenMotion();
}

void USettingsMenuWidget::ApplyMode()
{
	// Either way the page only goes back: over the game to the pause menu (SETTINGS lit there), from the main menu to it.
	RefreshCrumb();
	const bool bPause = Mode == ESettingsMenuMode::Pause;
	for (SettingsMenu::FHelpEntry& Entry : HelpEntries)
	{
		if (Entry.Target == ResumeButton)
		{
			Entry.Help = RowHelp(bPause ? ESettingRow::BackToPause : ESettingRow::Back);
		}
	}
}

void USettingsMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	AdvancePreviews(InDeltaTime);
	AdvanceNav(InDeltaTime);
	AdvanceLook(InDeltaTime);
	AdvanceScroll(InDeltaTime);
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

void USettingsMenuWidget::PlaceOnPage(UWidget* Widget, const FAnchors& Anchor, const FVector2D& Position, const FVector2D& Alignment)
{
	if (!PageCanvas || !Widget)
	{
		return;
	}
	UCanvasPanelSlot* PageSlot = PageCanvas->AddChildToCanvas(Widget);
	PageSlot->SetAnchors(Anchor);
	PageSlot->SetAlignment(Alignment);
	PageSlot->SetPosition(Position);
	PageSlot->SetAutoSize(true);
}

TSharedRef<SWidget> USettingsMenuWidget::RebuildWidget()
{
	using namespace LooterUI;
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		// The shade over the game (or the main menu's planet): a layer of its own that takes the clicks, so the field of view
		// preview can fade it apart from the page in front of it and let the view show.
		Dimmer = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Shade"));
		Dimmer->SetBrush(RectBrush(FLinearColor::Transparent));
		Dimmer->SetPadding(FMargin(0.f));
		Dimmer->SetHorizontalAlignment(HAlign_Fill);
		Dimmer->SetVerticalAlignment(VAlign_Fill);
		Dimmer->SetVisibility(ESlateVisibility::Visible);
		// The shade itself is painted (SettingsPageArt's Shade, marked: it fades with the UI transparency); the border holds it
		// and takes the field of view preview's fade on top of that.
		Dimmer->SetContent(USettingsPageArt::Make(WidgetTree, ESettingsPageArt::Shade, ESettingsPagePart::Back));
		FillOverlaySlot(Root->AddChildToOverlay(Dimmer));

		// The page: every piece at its mockup place on a canvas over the whole screen (the help card and the hints keep to
		// the right edge, the rows stretch between).
		PageCanvas = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), TEXT("Page"));
		Panel = PageCanvas;
		FillOverlaySlot(Root->AddChildToOverlay(PageCanvas));

		// The dial, SETTINGS and the breadcrumb (SettingsMenuHeader.cpp).
		MakeHeader();

		// The ruler and its pennants across the page (SettingsMenuTabs.cpp).
		HelpEntries.Reset();
		UCanvasPanelSlot* TabSlot = PageCanvas->AddChildToCanvas(MakeTabBar());
		TabSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		TabSlot->SetOffsets(FMargin(PageMargin, TabStripTop, PageMargin, TabStripHeight));

		// The rows: every page built now, each its own scrolling list in the switcher, so turning a tab only shows another.
		Pages = WidgetTree->ConstructWidget<UWidgetSwitcher>(UWidgetSwitcher::StaticClass());
		UCanvasPanelSlot* RowsSlot = PageCanvas->AddChildToCanvas(Pages);
		// Stretched across, its height set each frame to whole rows only (SettingsMenuScroll.cpp): the list never ends in a
		// cut row above the key hints.
		RowsSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 0.f));
		RowsSlot->SetOffsets(FMargin(PageMargin, RowsTop, PageMargin + HelpCardWidth + HelpCardGap, ListRoom()));
		PagesSlot = RowsSlot;
		// The stops, the descriptions and the rows are noted as the rows are made, page by page.
		PageNav.Reset();
		PageLists.Reset();
		LedgerLines.Reset();
		PageRowNumbers.Reset();
		PageBandCount.Reset();
		BuildPage = INDEX_NONE;
		MakeGraphicsPage();
		MakeAudioPage();
		MakeInterfacePage();
		MakeControlsPage();
		BuildRows = nullptr;

		// The help card on the right (SettingsMenuHelpCard.cpp).
		MakeHelpCard();

		// What a change did, bottom-left across from the hints.
		StatusText = MakeText(WidgetTree, TEXT(""), StatusSize, Color::SlateLabel());
		StatusText->SetFont(Font(StatusSize, false));
		StatusText->SetAutoWrapText(true);
		PlaceOnPage(MakeSized(WidgetTree, StatusText, StatusWidth), FAnchors(0.f, 1.f), FVector2D(PageMargin, -HintsFromBottom),
			FVector2D(0.f, 0.5f));

		// The key hints, bottom-right: the ones the focus changes (SettingsMenuNav.cpp fills them), then ESC BACK, a button.
		UHorizontalBox* HintRow = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		PromptBar = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		HintRow->AddChildToHorizontalBox(PromptBar)->SetVerticalAlignment(VAlign_Center);
		ResumeButton = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
		UHorizontalBox* BackLook = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		USettingsPageArt* BackCap = nullptr;
		BackLook->AddChildToHorizontalBox(SettingsPageArt::MakeKeyCap(WidgetTree, TEXT("Esc"), ResumeButton, BackCap))->SetVerticalAlignment(VAlign_Center);
		BackKey = BackCap;
		BackWord = MakeFloatingText(WidgetTree, HintWordSize, Color::SlateText(), HintWordSpacing);
		BackWord->SetText(FText::FromString(TEXT("BACK")));
		UHorizontalBoxSlot* BackWordSlot = BackLook->AddChildToHorizontalBox(BackWord);
		BackWordSlot->SetVerticalAlignment(VAlign_Center);
		BackWordSlot->SetPadding(FMargin(HintKeyGap, 0.f, 0.f, 0.f));
		ResumeButton->SetupContent(BackLook, ActionClose, 0, EButtonKind::Bare);
		// Going back is the page's close (over the game the HUD plays the same cue in the same moment, heard as one).
		ResumeButton->ClickCue = LooterSoundCue::Close;
		ResumeButton->OnButtonClicked.BindUObject(this, &USettingsMenuWidget::HandleButton);
		ResumeButton->OnButtonHovered.BindUObject(this, &USettingsMenuWidget::HandleButtonHovered);
		HintRow->AddChildToHorizontalBox(ResumeButton)->SetVerticalAlignment(VAlign_Center);
		Hints = HintRow;
		PlaceOnPage(HintRow, FAnchors(1.f, 1.f), FVector2D(-PageMargin, -HintsFromBottom), FVector2D(1.f, 0.5f));
		Describe(ResumeButton, RowHelp(Mode == ESettingsMenuMode::Pause ? ESettingRow::BackToPause : ESettingRow::Back), true);

		// On top, in the HUD minimap's corner: its outline, shown while its size is being set, and a live copy of the
		// minimap while its zoom is (the HUD's own is put away under the page).
		UCanvasPanel* PreviewLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		PreviewLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		FillOverlaySlot(Root->AddChildToOverlay(PreviewLayer));
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
		BeginOpenMotion();
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
