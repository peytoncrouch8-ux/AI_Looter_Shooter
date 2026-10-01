#include "UI/Menus/PauseMenuWidget.h"
#include "UI/Style/LooterButton.h"
#include "UI/HUD/LooterHUD.h"
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
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"

namespace
{
	const FName ActionResume(TEXT("Resume"));
	const FName ActionRebind(TEXT("Rebind"));
	const FName ActionResetOne(TEXT("ResetOne"));
	const FName ActionResetAll(TEXT("ResetAll"));
	const FName ActionQuit(TEXT("Quit"));
	const FName ActionQuality(TEXT("Quality"));
	const FName ActionMotionBlurOn(TEXT("MotionBlurOn"));
	const FName ActionMotionBlurOff(TEXT("MotionBlurOff"));
	const FName ActionMinimapOn(TEXT("MinimapOn"));
	const FName ActionMinimapOff(TEXT("MinimapOff"));
	const FName ActionFrameRateOn(TEXT("FrameRateOn"));
	const FName ActionFrameRateOff(TEXT("FrameRateOff"));
	const FName ActionHoldMode(TEXT("HoldMode"));
	const FName ActionToggleMode(TEXT("ToggleMode"));

	// Columns of the key list, shared by the key rows and the hold/toggle rows under them.
	constexpr float KeyColumnWidth = 180.f;
	constexpr float DefaultColumnWidth = 96.f;
	constexpr float QualityColumnWidth = 340.f;

	const EGraphicsQuality Qualities[] = { EGraphicsQuality::Low, EGraphicsQuality::Medium, EGraphicsQuality::High, EGraphicsQuality::Epic };

	/** What a preset gives, for the status line when it's picked. */
	const TCHAR* QualityHint(EGraphicsQuality Quality)
	{
		switch (Quality)
		{
		case EGraphicsQuality::Low:    return TEXT("Fastest: no Lumen or Nanite, simple shadows, thinner grass.");
		case EGraphicsQuality::Medium: return TEXT("60 fps at 1080p on a Radeon RX 580.");
		case EGraphicsQuality::High:   return TEXT("Lumen lighting and Nanite detail.");
		case EGraphicsQuality::Epic:   return TEXT("Everything at its best, with TSR anti-aliasing.");
		}
		return TEXT("");
	}
}

void UPauseMenuWidget::Open(ALooterHUD* InHUD)
{
	OwningHUD = InHUD;
	StopListening();
	SetIsFocusable(true);
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

void UPauseMenuWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

UKeyBindingSubsystem* UPauseMenuWidget::GetBindings() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UKeyBindingSubsystem>() : nullptr;
}

UGraphicsSettingsSubsystem* UPauseMenuWidget::GetGraphics() const
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	return LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
}

ULooterButton* UPauseMenuWidget::MakeButton(FName Action, int32 Index, const FString& Label, int32 FontSize, LooterUI::EButtonKind Kind)
{
	ULooterButton* Button = WidgetTree->ConstructWidget<ULooterButton>(ULooterButton::StaticClass());
	Button->Setup(WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass()), Action, Index, FText::FromString(Label), FontSize, Kind);
	Button->OnButtonClicked.BindUObject(this, &UPauseMenuWidget::HandleButton);
	return Button;
}

TSharedRef<SWidget> UPauseMenuWidget::RebuildWidget()
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

		Add(MakeText(WidgetTree, TEXT("Game paused   |   Esc: resume"), 12, Color::TextDim(), true, 150), 0.f);
		Add(MakeButton(ActionResume, 0, TEXT("Resume"), 16, EButtonKind::Primary), 10.f);

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
		TransparencySlider->OnValueChanged.AddDynamic(this, &UPauseMenuWidget::HandleTransparencyChanged);
		TransparencySlider->OnMouseCaptureEnd.AddDynamic(this, &UPauseMenuWidget::HandleTransparencyReleased);
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
		MinimapSlider->OnValueChanged.AddDynamic(this, &UPauseMenuWidget::HandleMinimapSizeChanged);
		MinimapSlider->OnMouseCaptureBegin.AddDynamic(this, &UPauseMenuWidget::HandleMinimapSizeGrabbed);
		MinimapSlider->OnMouseCaptureEnd.AddDynamic(this, &UPauseMenuWidget::HandleMinimapSizeReleased);
		USlider* MinimapZoom = nullptr;
		UTextBlock* MinimapZoomText = nullptr;
		Add(MakeSliderRow(TEXT("Minimap zoom"), UGraphicsSettingsSubsystem::MinMinimapZoom, UGraphicsSettingsSubsystem::MaxMinimapZoom,
			MinimapZoom, MinimapZoomText), 4.f);
		MinimapZoomSlider = MinimapZoom;
		MinimapZoomValue = MinimapZoomText;
		MinimapZoomSlider->OnValueChanged.AddDynamic(this, &UPauseMenuWidget::HandleMinimapZoomChanged);
		MinimapZoomSlider->OnMouseCaptureEnd.AddDynamic(this, &UPauseMenuWidget::HandleMinimapZoomReleased);
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

		UHorizontalBox* Footer = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* ResetSlot = Footer->AddChildToHorizontalBox(MakeButton(ActionResetAll, 0, TEXT("Reset all"), 14));
		ResetSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		ResetSlot->SetPadding(FMargin(0.f, 0.f, 8.f, 0.f));
		Footer->AddChildToHorizontalBox(MakeButton(ActionQuit, 0, TEXT("Quit Game"), 14, EButtonKind::Danger))->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		Add(Footer, 10.f);

		// Close button in the frame corner does the same as Resume.
		UWidget* CloseSize = MakeSized(WidgetTree, MakeButton(ActionResume, 0, TEXT("X"), 15, EButtonKind::Normal), 34.f, 34.f);

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
	}
	return Super::RebuildWidget();
}

UWidget* UPauseMenuWidget::MakeMinimapPreview()
{
	using namespace LooterUI;
	UOverlay* Stack = WidgetTree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	auto AddLayer = [this, Stack](const FSlateBrush& Brush, bool bFill)
	{
		UImage* Image = WidgetTree->ConstructWidget<UImage>(UImage::StaticClass());
		Image->SetBrush(Brush);
		UOverlaySlot* LayerSlot = Stack->AddChildToOverlay(Image);
		LayerSlot->SetHorizontalAlignment(bFill ? HAlign_Fill : HAlign_Center);
		LayerSlot->SetVerticalAlignment(bFill ? VAlign_Fill : VAlign_Center);
		return Image;
	};
	// The HUD minimap's glass, a brighter rim so it reads over the dimmed screen, and the player's arrow in the middle.
	const FLinearColor Glass = Color::ScreenBg();
	MarkBackground(AddLayer(CircleBrush(FLinearColor(Glass.R, Glass.G, Glass.B, 0.62f)), true));
	AddLayer(CircleBrush(FLinearColor::Transparent, Color::TileLine(), 2.f), true);
	AddLayer(MarkerBrush(EMarker::Arrow, FLinearColor::White), false)->SetDesiredSizeOverride(FVector2D(22.f, 22.f));
	// The caption sits in the lower part of the circle at any size (ShowMinimapScale places it).
	MinimapPreviewCaption = MakeText(WidgetTree, TEXT("Minimap"), 10, Color::Title(), true, 200);
	UOverlaySlot* CaptionSlot = Stack->AddChildToOverlay(MinimapPreviewCaption);
	CaptionSlot->SetHorizontalAlignment(HAlign_Center);
	CaptionSlot->SetVerticalAlignment(VAlign_Top);

	MinimapPreviewSize = MakeSized(WidgetTree, Stack, 0.f);
	return MinimapPreviewSize;
}

UWidget* UPauseMenuWidget::MakeToggleRow(const FString& Label, const FString& FirstText, const FString& SecondText, FName FirstAction,
	FName SecondAction, int32 Index, bool bKeyListRow, ULooterButton*& OutFirst, ULooterButton*& OutSecond)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	// Rows that belong to a key binding sit indented under it, a step quieter.
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Label, bKeyListRow ? 13 : 14, bKeyListRow ? Color::TextDim() : Color::Text()));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);
	NameSlot->SetPadding(FMargin(bKeyListRow ? 16.f : 0.f, 0.f, 0.f, 0.f));

	// Two-segment switch; the active side is highlighted like a selected tab.
	UHorizontalBox* Switch = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	OutFirst = MakeButton(FirstAction, Index, FirstText, 12, EButtonKind::Tab);
	OutSecond = MakeButton(SecondAction, Index, SecondText, 12, EButtonKind::Tab);
	UHorizontalBoxSlot* FirstSlot = Switch->AddChildToHorizontalBox(OutFirst);
	FirstSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	FirstSlot->SetPadding(FMargin(0.f, 0.f, 4.f, 0.f));
	Switch->AddChildToHorizontalBox(OutSecond)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));

	// Same width as the key buttons; in the key list it sits in their column, above an empty "Default" column.
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Switch, KeyColumnWidth))->SetPadding(bKeyListRow ? FMargin(8.f, 0.f) : FMargin(8.f, 0.f, 0.f, 0.f));
	if (bKeyListRow)
	{
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, nullptr, DefaultColumnWidth));
	}
	return MakeRow(WidgetTree, Line);
}

UWidget* UPauseMenuWidget::MakeChoiceRow(const FString& Label, const TArray<FString>& Choices, FName Action, float Width,
	TArray<ULooterButton*>& OutButtons)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Label, 14, Color::Text()));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// Segments like the two-way switch's, the chosen one highlighted like a selected tab.
	UHorizontalBox* Segments = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	for (int32 Index = 0; Index < Choices.Num(); ++Index)
	{
		ULooterButton* Button = MakeButton(Action, Index, Choices[Index], 12, EButtonKind::Tab);
		UHorizontalBoxSlot* SegmentSlot = Segments->AddChildToHorizontalBox(Button);
		SegmentSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		SegmentSlot->SetPadding(FMargin(0.f, 0.f, Index + 1 < Choices.Num() ? 4.f : 0.f, 0.f));
		OutButtons.Add(Button);
	}
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, Segments, Width))->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	return MakeRow(WidgetTree, Line);
}

UWidget* UPauseMenuWidget::MakeSliderRow(const FString& Label, float MinValue, float MaxValue, USlider*& OutSlider, UTextBlock*& OutValue)
{
	using namespace LooterUI;
	UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
	UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Label, 14, Color::Text()));
	NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	NameSlot->SetVerticalAlignment(VAlign_Center);

	// A thin cyan track with an orange handle, in 5% steps.
	FSliderStyle Style;
	const FSlateBrush Track = RectBrush(Color::SegmentOff(), Color::RowLine(), 1.f);
	Style.SetNormalBarImage(Track);
	Style.SetHoveredBarImage(Track);
	Style.SetDisabledBarImage(Track);
	FSlateBrush Handle = RectBrush(Color::Accent(), Color::AccentDark(), 1.f);
	Handle.ImageSize = FVector2D(12.f, 22.f);
	FSlateBrush HandleHovered = RectBrush(FLinearColor::FromSRGBColor(FColor(255, 190, 90)), Color::AccentDark(), 1.f);
	HandleHovered.ImageSize = Handle.ImageSize;
	Style.SetNormalThumbImage(Handle);
	Style.SetHoveredThumbImage(HandleHovered);
	Style.SetDisabledThumbImage(Handle);
	Style.SetBarThickness(6.f);

	OutSlider = WidgetTree->ConstructWidget<USlider>(USlider::StaticClass());
	OutSlider->SetWidgetStyle(Style);
	OutSlider->SetMinValue(MinValue);
	OutSlider->SetMaxValue(MaxValue);
	OutSlider->SetStepSize(0.05f);
	OutSlider->MouseUsesStep = true;
	// The menu keeps keyboard focus (Esc closes it); and the world behind keeps redrawing while the handle is dragged.
	OutSlider->IsFocusable = false;
	OutSlider->bPreventThrottling = true;

	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, OutSlider, KeyColumnWidth, 30.f))->SetPadding(FMargin(8.f, 0.f));

	OutValue = MakeText(WidgetTree, TEXT(""), 14, Color::Title());
	OutValue->SetJustification(ETextJustify::Right);
	Line->AddChildToHorizontalBox(MakeSized(WidgetTree, OutValue, 48.f))->SetVerticalAlignment(VAlign_Center);
	return MakeRow(WidgetTree, Line);
}

void UPauseMenuWidget::RefreshGraphics()
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

void UPauseMenuWidget::ShowMinimapScale(float Scale)
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

void UPauseMenuWidget::HandleMinimapSizeChanged(float Value)
{
	// The HUD takes the new size the moment it shows again; meanwhile the outline in its corner shows it.
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetMinimapScale(Value, /*bSave*/ false);
	}
	ShowMinimapScale(Value);
	PreviewLinger = 0.8f;
}

void UPauseMenuWidget::HandleMinimapSizeGrabbed()
{
	bMinimapSliderHeld = true;
}

void UPauseMenuWidget::HandleMinimapSizeReleased()
{
	bMinimapSliderHeld = false;
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		SetStatus(FString::Printf(TEXT("Minimap size %d%%."), FMath::RoundToInt(Graphics->GetMinimapScale() * 100.f)), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

void UPauseMenuWidget::HandleMinimapZoomChanged(float Value)
{
	// The HUD's map follows the moment the menu closes.
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetMinimapZoom(Value, /*bSave*/ false);
	}
	if (MinimapZoomValue)
	{
		MinimapZoomValue->SetText(FText::FromString(FString::Printf(TEXT("%.1fx"), Value)));
	}
}

void UPauseMenuWidget::HandleMinimapZoomReleased()
{
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		SetStatus(FString::Printf(TEXT("Minimap zoom %.1fx."), Graphics->GetMinimapZoom()), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

void UPauseMenuWidget::HandleTransparencyChanged(float Value)
{
	// Every UI follows at once, this menu included, so the effect shows while dragging.
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SetUITransparency(Value, /*bSave*/ false);
	}
	if (TransparencyValue)
	{
		TransparencyValue->SetText(FText::FromString(FString::Printf(TEXT("%d%%"), FMath::RoundToInt(Value * 100.f))));
	}
}

void UPauseMenuWidget::HandleTransparencyReleased()
{
	if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
	{
		Graphics->SaveSettings();
		SetStatus(FString::Printf(TEXT("UI transparency %d%%."), FMath::RoundToInt(Graphics->GetUITransparency() * 100.f)), LooterUI::Color::TextDim());
	}
	// Dragging handed focus to the slider's window; take it back so Esc still closes the menu.
	SetKeyboardFocus();
}

void UPauseMenuWidget::RebuildControls()
{
	using namespace LooterUI;
	ControlsList->ClearChildren();
	KeyButtons.Reset();
	ModeButtons.Reset();

	const UKeyBindingSubsystem* Bindings = GetBindings();
	if (!Bindings)
	{
		ControlsList->AddChild(MakeText(WidgetTree, TEXT("Key bindings unavailable."), 13, LooterUI::Color::Worse()));
		return;
	}

	FString LastCategory;
	const TArray<FRebindableKey>& List = Bindings->GetBindings();
	for (int32 Index = 0; Index < List.Num(); ++Index)
	{
		const FRebindableKey& Binding = List[Index];
		if (Binding.Category.ToString() != LastCategory)
		{
			LastCategory = Binding.Category.ToString();
			if (UScrollBoxSlot* HeaderSlot = Cast<UScrollBoxSlot>(ControlsList->AddChild(MakeSection(WidgetTree, LastCategory))))
			{
				HeaderSlot->SetPadding(FMargin(0.f, Index == 0 ? 0.f : 10.f, 0.f, 4.f));
			}
		}

		UHorizontalBox* Line = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UHorizontalBoxSlot* NameSlot = Line->AddChildToHorizontalBox(MakeText(WidgetTree, Binding.DisplayName.ToString(), 14, Color::Text()));
		NameSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		NameSlot->SetVerticalAlignment(VAlign_Center);

		ULooterButton* KeyButton = MakeButton(ActionRebind, Index, TEXT(""), 13, EButtonKind::Key);
		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, KeyButton, KeyColumnWidth))->SetPadding(FMargin(8.f, 0.f));
		KeyButtons.Add(KeyButton);

		Line->AddChildToHorizontalBox(MakeSized(WidgetTree, MakeButton(ActionResetOne, Index, TEXT("Default"), 10, EButtonKind::Mini), DefaultColumnWidth));

		if (UScrollBoxSlot* RowSlot = Cast<UScrollBoxSlot>(ControlsList->AddChild(MakeRow(WidgetTree, Line))))
		{
			RowSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
		}

		if (Binding.bSupportsToggle)
		{
			ULooterButton* HoldButton = nullptr;
			ULooterButton* ToggleButton = nullptr;
			UWidget* ModeRow = MakeToggleRow(FString::Printf(TEXT("%s mode"), *Binding.DisplayName.ToString()), TEXT("Hold"), TEXT("Toggle"),
				ActionHoldMode, ActionToggleMode, Index, true, HoldButton, ToggleButton);
			ModeButtons.Add(HoldButton);
			ModeButtons.Add(ToggleButton);
			if (UScrollBoxSlot* ModeSlot = Cast<UScrollBoxSlot>(ControlsList->AddChild(ModeRow)))
			{
				ModeSlot->SetPadding(FMargin(0.f, 0.f, 0.f, 4.f));
			}
		}
	}
	RefreshKeyLabels();
}


void UPauseMenuWidget::RefreshKeyLabels()
{
	const UKeyBindingSubsystem* Bindings = GetBindings();
	if (!Bindings)
	{
		return;
	}

	const TArray<FRebindableKey>& List = Bindings->GetBindings();
	for (ULooterButton* Button : KeyButtons)
	{
		if (!List.IsValidIndex(Button->Index))
		{
			continue;
		}
		const bool bListening = Button->Index == ListeningIndex;
		const FKey Key = Bindings->GetKey(List[Button->Index].Id);
		Button->SetLabel(bListening ? FText::FromString(TEXT("Press a key...")) : Key.GetDisplayName());
		Button->SetHighlighted(bListening);
	}

	for (ULooterButton* Button : ModeButtons)
	{
		if (List.IsValidIndex(Button->Index))
		{
			const bool bToggle = Bindings->IsToggleMode(List[Button->Index].Id);
			Button->SetHighlighted((Button->Action == ActionToggleMode) == bToggle);
		}
	}
}

void UPauseMenuWidget::SetStatus(const FString& Message, const FLinearColor& Color)
{
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(Message));
		StatusText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UPauseMenuWidget::HandleButton(ULooterButton* Button)
{
	UKeyBindingSubsystem* Bindings = GetBindings();
	ALooterHUD* HUD = OwningHUD.Get();
	if (!Button)
	{
		return;
	}

	if (Button->Action == ActionResume)
	{
		if (HUD) { HUD->ClosePauseMenu(); }
		return;
	}
	if (Button->Action == ActionQuit)
	{
		if (HUD) { HUD->QuitGame(); }
		return;
	}
	if (Button->Action == ActionQuality && Button->Index >= 0 && Button->Index < UE_ARRAY_COUNT(Qualities))
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const EGraphicsQuality Quality = Qualities[Button->Index];
			Graphics->SetQuality(Quality);
			SetStatus(FString::Printf(TEXT("Quality: %s. %s"), *UGraphicsSettingsSubsystem::QualityName(Quality), QualityHint(Quality)),
				LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionMotionBlurOn || Button->Action == ActionMotionBlurOff)
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const bool bBlur = Button->Action == ActionMotionBlurOn;
			Graphics->SetMotionBlurEnabled(bBlur);
			SetStatus(bBlur ? TEXT("Motion blur on.") : TEXT("Motion blur off."), LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionMinimapOn || Button->Action == ActionMinimapOff)
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const bool bMinimap = Button->Action == ActionMinimapOn;
			Graphics->SetMinimapShown(bMinimap);
			SetStatus(bMinimap ? TEXT("Minimap on.") : TEXT("Minimap off."), LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}
	if (Button->Action == ActionFrameRateOn || Button->Action == ActionFrameRateOff)
	{
		StopListening();
		if (UGraphicsSettingsSubsystem* Graphics = GetGraphics())
		{
			const bool bFrameRate = Button->Action == ActionFrameRateOn;
			Graphics->SetFrameRateShown(bFrameRate);
			SetStatus(bFrameRate ? TEXT("FPS counter on.") : TEXT("FPS counter off."), LooterUI::Color::TextDim());
		}
		RefreshGraphics();
		RefreshKeyLabels();
		SetKeyboardFocus();
		return;
	}

	if (Button->Action == ActionRebind)
	{
		StartListening(Button->Index);
	}
	else if ((Button->Action == ActionHoldMode || Button->Action == ActionToggleMode) && Bindings && Bindings->GetBindings().IsValidIndex(Button->Index))
	{
		StopListening();
		const FRebindableKey& Binding = Bindings->GetBindings()[Button->Index];
		const bool bToggle = Button->Action == ActionToggleMode;
		Bindings->SetToggleMode(Binding.Id, bToggle);
		const FString Name = Binding.DisplayName.ToString();
		SetStatus(bToggle ? FString::Printf(TEXT("%s: press to turn on or off."), *Name) : FString::Printf(TEXT("%s: hold the key."), *Name), LooterUI::Color::TextDim());
	}
	else if (Button->Action == ActionResetOne && Bindings && Bindings->GetBindings().IsValidIndex(Button->Index))
	{
		StopListening();
		Bindings->ResetKey(Bindings->GetBindings()[Button->Index].Id);
		SetStatus(TEXT("Restored default key."), LooterUI::Color::TextDim());
	}
	else if (Button->Action == ActionResetAll && Bindings)
	{
		StopListening();
		Bindings->ResetAll();
		SetStatus(TEXT("All controls reset to defaults."), LooterUI::Color::TextDim());
	}

	RefreshKeyLabels();
	// Buttons hand focus to the game viewport; take it back so we keep receiving keys.
	SetKeyboardFocus();
}

void UPauseMenuWidget::StartListening(int32 BindingIndex)
{
	ListeningIndex = BindingIndex;
	// Let the mouse wheel be captured as a key instead of scrolling the list.
	ControlsList->SetConsumeMouseWheel(EConsumeMouseWheel::Never);
	SetStatus(TEXT("Press the new key (Esc to cancel)."), LooterUI::Color::Accent());
	RefreshKeyLabels();
}

void UPauseMenuWidget::StopListening()
{
	ListeningIndex = INDEX_NONE;
	if (ControlsList)
	{
		ControlsList->SetConsumeMouseWheel(EConsumeMouseWheel::WhenScrollingPossible);
	}
}

void UPauseMenuWidget::AssignKey(const FKey& Key)
{
	UKeyBindingSubsystem* Bindings = GetBindings();
	if (!Bindings || !Bindings->GetBindings().IsValidIndex(ListeningIndex))
	{
		StopListening();
		return;
	}

	const FName Id = Bindings->GetBindings()[ListeningIndex].Id;
	StopListening();

	if (Key.IsGamepadKey())
	{
		SetStatus(TEXT("Gamepad buttons can't be assigned here; use a keyboard key or mouse button."), LooterUI::Color::Worse());
	}
	else
	{
		Bindings->SetKey(Id, Key);
		const FName Conflict = Bindings->FindConflict(Id, Key);
		if (Conflict.IsNone())
		{
			SetStatus(FString::Printf(TEXT("%s is now %s."), *Bindings->GetDisplayName(Id).ToString(), *Key.GetDisplayName().ToString()), LooterUI::Color::TextDim());
		}
		else
		{
			SetStatus(FString::Printf(TEXT("Warning: %s is also bound to %s."), *Key.GetDisplayName().ToString(),
				*Bindings->GetDisplayName(Conflict).ToString()), LooterUI::Color::Worse());
		}
	}
	RefreshKeyLabels();
}

FReply UPauseMenuWidget::NativeOnPreviewKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	const FKey Key = InKeyEvent.GetKey();

	if (ListeningIndex != INDEX_NONE)
	{
		if (Key == EKeys::Escape)
		{
			StopListening();
			SetStatus(TEXT("Cancelled."), LooterUI::Color::TextDim());
			RefreshKeyLabels();
		}
		else
		{
			AssignKey(Key);
		}
		return FReply::Handled();
	}

	if (Key == EKeys::Escape || Key == EKeys::Gamepad_Special_Right)
	{
		if (ALooterHUD* HUD = OwningHUD.Get())
		{
			HUD->ClosePauseMenu();
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewKeyDown(InGeometry, InKeyEvent);
}

FReply UPauseMenuWidget::NativeOnPreviewMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (ListeningIndex != INDEX_NONE)
	{
		AssignKey(InMouseEvent.GetEffectingButton());
		return FReply::Handled();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UPauseMenuWidget::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (ListeningIndex != INDEX_NONE)
	{
		AssignKey(InMouseEvent.GetWheelDelta() > 0.f ? EKeys::MouseScrollUp : EKeys::MouseScrollDown);
		return FReply::Handled();
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}
