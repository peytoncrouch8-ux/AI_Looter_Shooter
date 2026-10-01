#include "UI/HUD/HudFrameRateWidget.h"
#include "Settings/GraphicsSettingsSubsystem.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"

using namespace LooterUI;

namespace
{
	/** Seconds each shown number averages over. */
	constexpr float UpdateSeconds = 0.5f;

	/** A little quieter than the corners at their brightest: it's for reading now and then, not for watching. */
	constexpr float Opacity = 0.8f;

	/** Below these the number warns: orange under 60 fps, red under 30. */
	constexpr float SmoothFps = 60.f;
	constexpr float PoorFps = 30.f;
}

TSharedRef<SWidget> UHudFrameRateWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UVerticalBox* Box = WidgetTree->ConstructWidget<UVerticalBox>(UVerticalBox::StaticClass());

		// "FPS 120", read the way the XP bar's "LV 12" is: a small dim label, then the number.
		UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass());
		UTextBlock* Label = MakeFloatingText(WidgetTree, 12, Color::TextDim(), 100);
		Label->SetText(FText::FromString(TEXT("FPS")));
		UHorizontalBoxSlot* LabelSlot = Row->AddChildToHorizontalBox(Label);
		LabelSlot->SetVerticalAlignment(VAlign_Bottom);
		LabelSlot->SetPadding(FMargin(0.f, 0.f, 6.f, 3.f));
		RateValue = MakeFloatingText(WidgetTree, 20, Color::Text());
		Row->AddChildToHorizontalBox(RateValue)->SetVerticalAlignment(VAlign_Bottom);
		Box->AddChildToVerticalBox(Row);

		FrameTimeText = MakeFloatingText(WidgetTree, 11, Color::TextDim(), 60);
		Box->AddChildToVerticalBox(FrameTimeText)->SetPadding(FMargin(1.f, 0.f, 0.f, 0.f));

		Box->SetVisibility(ESlateVisibility::HitTestInvisible);
		Box->SetRenderOpacity(Opacity);
		Cluster = Box;
		WidgetTree->RootWidget = Box;
	}
	return Super::RebuildWidget();
}

void UHudFrameRateWidget::NativeConstruct()
{
	Super::NativeConstruct();
	Frames = 0;
	Elapsed = 0.f;
	RateValue->SetText(FText::GetEmpty());
	FrameTimeText->SetText(FText::GetEmpty());
	ApplySetting();
}

bool UHudFrameRateWidget::ApplySetting()
{
	const ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	const UGraphicsSettingsSubsystem* Settings = LocalPlayer ? LocalPlayer->GetSubsystem<UGraphicsSettingsSubsystem>() : nullptr;
	const bool bShown = !Settings || Settings->IsFrameRateShown();
	// Hidden rather than collapsed: the widget keeps its size, so it's still painted and ticks to notice the setting.
	const ESlateVisibility Wanted = bShown ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden;
	if (Cluster->GetVisibility() != Wanted)
	{
		Cluster->SetVisibility(Wanted);
	}
	return bShown;
}

void UHudFrameRateWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Counted in real time, so it measures frames even while the game is paused or slowed.
	++Frames;
	Elapsed += InDeltaTime;
	if (Elapsed < UpdateSeconds)
	{
		return;
	}

	const float Fps = Frames / Elapsed;
	const float Milliseconds = Elapsed * 1000.f / Frames;
	Frames = 0;
	Elapsed = 0.f;
	if (!ApplySetting())
	{
		return;
	}

	RateValue->SetText(FText::AsNumber(FMath::RoundToInt(Fps)));
	RateValue->SetColorAndOpacity(FSlateColor(Fps < PoorFps ? Color::Worse() : (Fps < SmoothFps ? Color::Accent() : Color::Text())));
	FrameTimeText->SetText(FText::FromString(FString::Printf(TEXT("%.1f MS"), Milliseconds)));
}
