#include "UI/Menus/SettingsMenuWidget.h"
#include "UI/Menus/SettingsMenuParts.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "UI/HUD/HudMinimapWidget.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "Settings/KeyBindingSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"

using namespace SettingsMenu;

void USettingsMenuWidget::Open(ESettingsMenuMode InMode)
{
	Mode = InMode;
	StopListening();
	SetIsFocusable(true);
	ApplyMode();
	RefreshKeyLabels();
	RefreshGraphics();
	SetStatus(TEXT(""), LooterUI::Color::TextDim());

	bMinimapSliderHeld = false;
	PreviewLinger = 0.f;
	PreviewOpacity = 0.f;
	if (MinimapPreview)
	{
		MinimapPreview->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void USettingsMenuWidget::ApplyMode()
{
	// Over the game it pauses: Resume, and Save & Quit back to the main menu. From the main menu it only goes back.
	const bool bPause = Mode == ESettingsMenuMode::Pause;
	if (HeaderText)
	{
		HeaderText->SetText(FText::FromString(bPause ? TEXT("Game paused   |   Esc: resume") : TEXT("Esc: back")));
	}
	const ESlateVisibility PauseOnly = bPause ? ESlateVisibility::Visible : ESlateVisibility::Collapsed;
	if (ResumeButton) { ResumeButton->SetVisibility(PauseOnly); }
	if (SaveQuitButton) { SaveQuitButton->SetVisibility(PauseOnly); }
	if (BackButton) { BackButton->SetVisibility(bPause ? ESlateVisibility::Collapsed : ESlateVisibility::Visible); }
}

void USettingsMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	if (!MinimapPreview)
	{
		return;
	}
	// The minimap's outline shows while its size is being set (the slider held or pointed at), and fades a moment after.
	const bool bSetting = bMinimapSliderHeld || (MinimapSlider && MinimapSlider->IsHovered());
	PreviewLinger = bSetting ? 0.8f : FMath::Max(PreviewLinger - InDeltaTime, 0.f);
	PreviewOpacity = FMath::FInterpConstantTo(PreviewOpacity, PreviewLinger > 0.f ? 1.f : 0.f, InDeltaTime, 6.f);
	MinimapPreview->SetRenderOpacity(PreviewOpacity);
	MinimapPreview->SetVisibility(PreviewOpacity > 0.f ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
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

ULooterButton* USettingsMenuWidget::MakeButton(FName Action, int32 Index, const FString& Label, int32 FontSize, LooterUI::EButtonKind Kind)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, Index, FText::FromString(Label), FontSize, Kind);
	Button->OnButtonClicked.BindUObject(this, &USettingsMenuWidget::HandleButton);
	return Button;
}

TSharedRef<SWidget> USettingsMenuWidget::RebuildWidget()
{
	using namespace LooterUI;
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UOverlay* Root = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass(), TEXT("Root"));
		WidgetTree->RootWidget = Root;

		UBorder* Backdrop = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Backdrop"));
		Backdrop->SetBrush(RectBrush(Color::Backdrop()));
		Backdrop->SetVisibility(ESlateVisibility::Visible);
		Backdrop->SetHorizontalAlignment(HAlign_Center);
		Backdrop->SetVerticalAlignment(VAlign_Center);
		MarkBackground(Backdrop);
		UOverlaySlot* BackdropSlot = Root->AddChildToOverlay(Backdrop);
		BackdropSlot->SetHorizontalAlignment(HAlign_Fill);
		BackdropSlot->SetVerticalAlignment(VAlign_Fill);

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

		Add(MakeSection(WidgetTree, TEXT("Graphics")), 18.f);
		TArray<FString> QualityNames;
		for (const EGraphicsQuality Quality : Qualities)
		{
			QualityNames.Add(UGraphicsSettingsSubsystem::QualityName(Quality));
		}
		TArray<ULooterButton*> Quality;
		Add(MakeChoiceRow(TEXT("Quality"), QualityNames, ActionQuality, QualityColumnWidth, Quality), 4.f);
		for (ULooterButton* Button : Quality)
		{
			QualityButtons.Add(Button);
		}
		ULooterButton* BlurOn = nullptr;
		ULooterButton* BlurOff = nullptr;
		Add(MakeToggleRow(TEXT("Motion blur"), TEXT("On"), TEXT("Off"), ActionMotionBlurOn, ActionMotionBlurOff, 0, false, BlurOn, BlurOff), 4.f);
		MotionBlurOn = BlurOn;
		MotionBlurOff = BlurOff;

		Add(MakeSection(WidgetTree, TEXT("Interface")), 18.f);
		USlider* Transparency = nullptr;
		UTextBlock* TransparencyText = nullptr;
		Add(MakeSliderRow(TEXT("UI transparency"), 0.f, 1.f, Transparency, TransparencyText), 4.f);
		TransparencySlider = Transparency;
		TransparencyValue = TransparencyText;
		TransparencySlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleTransparencyChanged);
		TransparencySlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleTransparencyReleased);
		ULooterButton* MapOn = nullptr;
		ULooterButton* MapOff = nullptr;
		Add(MakeToggleRow(TEXT("Minimap"), TEXT("On"), TEXT("Off"), ActionMinimapOn, ActionMinimapOff, 0, false, MapOn, MapOff), 4.f);
		MinimapOn = MapOn;
		MinimapOff = MapOff;
		USlider* MinimapSize = nullptr;
		UTextBlock* MinimapText = nullptr;
		Add(MakeSliderRow(TEXT("Minimap size"), UGraphicsSettingsSubsystem::MinMinimapScale, UGraphicsSettingsSubsystem::MaxMinimapScale,
			MinimapSize, MinimapText), 4.f);
		MinimapSlider = MinimapSize;
		MinimapValue = MinimapText;
		MinimapSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeChanged);
		MinimapSlider->OnMouseCaptureBegin.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeGrabbed);
		MinimapSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapSizeReleased);
		USlider* MinimapZoom = nullptr;
		UTextBlock* MinimapZoomText = nullptr;
		Add(MakeSliderRow(TEXT("Minimap zoom"), UGraphicsSettingsSubsystem::MinMinimapZoom, UGraphicsSettingsSubsystem::MaxMinimapZoom,
			MinimapZoom, MinimapZoomText), 4.f);
		MinimapZoomSlider = MinimapZoom;
		MinimapZoomValue = MinimapZoomText;
		MinimapZoomSlider->OnValueChanged.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomChanged);
		MinimapZoomSlider->OnMouseCaptureEnd.AddDynamic(this, &USettingsMenuWidget::HandleMinimapZoomReleased);
		ULooterButton* RateOn = nullptr;
		ULooterButton* RateOff = nullptr;
		Add(MakeToggleRow(TEXT("FPS counter"), TEXT("On"), TEXT("Off"), ActionFrameRateOn, ActionFrameRateOff, 0, false, RateOn, RateOff), 4.f);
		FrameRateOn = RateOn;
		FrameRateOff = RateOff;

		Add(MakeSection(WidgetTree, TEXT("Controls")), 18.f);
		UTextBlock* Hint = MakeText(WidgetTree, TEXT("Click a key to change it, then press the new key or mouse button. Esc cancels."), 11, Color::TextDim());
		Hint->SetAutoWrapText(true);
		Add(Hint, 4.f);
		ControlsList = WidgetTree->ConstructWidget<UScrollBox>(UScrollBox::StaticClass());
		StyleScrollBox(ControlsList);
		Add(ControlsList, 8.f, true);

		StatusText = MakeText(WidgetTree, TEXT(""), 13, Color::TextDim());
		StatusText->SetAutoWrapText(true);
		Add(StatusText, 8.f);

		// Reset all, then Save & Quit over the game or Back in the main menu (ApplyMode shows one of them).
		UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* ResetSlot = Footer->AddChildToHorizontalBox(MakeButton(ActionResetAll, 0, TEXT("Reset all"), 14));
		ResetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ResetSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		SaveQuitButton = MakeButton(ActionSaveQuit, 0, TEXT("Save & Quit"), 14, EButtonKind::Danger);
		Footer->AddChildToHorizontalBox(SaveQuitButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		BackButton = MakeButton(ActionClose, 0, TEXT("Back"), 14, EButtonKind::Primary);
		Footer->AddChildToHorizontalBox(BackButton)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Add(Footer, 10.f);

		// Close button in the frame corner does the same as Resume or Back.
		UWidget* CloseSize = MakeSized(WidgetTree, MakeButton(ActionClose, 0, TEXT("X"), 15, EButtonKind::Normal), 34.f, 34.f);

		USizeBox* Size = MakeSized(WidgetTree, MakePanel(WidgetTree, TEXT("Settings"), Column, CloseSize), 640.f);
		Size->SetMaxDesiredHeight(860.f);
		Backdrop->SetContent(Size);

		// On top, in the HUD minimap's corner: its outline, shown while its size is being set.
		UCanvasPanel* PreviewLayer = WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass());
		PreviewLayer->SetVisibility(ESlateVisibility::HitTestInvisible);
		UOverlaySlot* PreviewSlot = Root->AddChildToOverlay(PreviewLayer);
		PreviewSlot->SetHorizontalAlignment(HAlign_Fill);
		PreviewSlot->SetVerticalAlignment(VAlign_Fill);
		MinimapPreview = MakeMinimapPreview();
		MinimapPreview->SetVisibility(ESlateVisibility::Collapsed);
		UCanvasPanelSlot* MinimapSlot = PreviewLayer->AddChildToCanvas(MinimapPreview);
		MinimapSlot->SetAnchors(FAnchors(1.f, 0.f));
		MinimapSlot->SetAlignment(FVector2D(1.f, 0.f));
		MinimapSlot->SetPosition(FVector2D(-UHudMinimapWidget::Margin, UHudMinimapWidget::Margin));
		MinimapSlot->SetAutoSize(true);

		RebuildControls();
		RefreshGraphics();
		ApplyMode();
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
	for (int32 Index = 0; Index < QualityButtons.Num(); ++Index)
	{
		QualityButtons[Index]->SetHighlighted(static_cast<int32>(Graphics->GetQuality()) == Index);
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
