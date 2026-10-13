// ULooterProgressRing: the kit's progress ring for a widget tree, and the key cap a hold prompt wears.

#include "UI/Style/LooterProgressRingWidget.h"
#include "UI/Style/LooterProgressRing.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Styling/SlateColor.h"

namespace
{
	/**
	 * The hold key cap: a square this big, the ring filling it and a dark disc just inside the ring's edge (the ring keeps
	 * 3.5 px free outside its line, and the line is 3 wide).
	 */
	constexpr float CapSize = 36.f;
	constexpr float CapThickness = 3.f;
	constexpr float CapDiscInset = 3.5f;
	/** How dark the disc is: the HUD's other faint backings (the bars' backing is the same). */
	constexpr float CapDiscAlpha = 0.6f;
}

// ---------------------------------------------------------------------------
// The widget
// ---------------------------------------------------------------------------

ULooterProgressRing::ULooterProgressRing(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	FillColor = LooterUI::Color::Accent();
	TrackColor = LooterUI::Color::Hairline();
	TrackColor.A = 0.28f;
}

void ULooterProgressRing::Configure(float InDiameter, float InThickness, int32 InSegments, float InSegmentGap)
{
	Diameter = InDiameter;
	Thickness = InThickness;
	Segments = InSegments;
	SegmentGap = InSegmentGap;
	if (Ring.IsValid())
	{
		Ring->Configure(Diameter, Thickness, Segments, SegmentGap);
	}
}

void ULooterProgressRing::SetColors(const FLinearColor& InFill, const FLinearColor& InTrack)
{
	FillColor = InFill;
	TrackColor = InTrack;
	if (Ring.IsValid())
	{
		Ring->SetColors(FillColor, TrackColor);
	}
}

void ULooterProgressRing::SetProgress(float InProgress)
{
	Progress = FMath::Clamp(InProgress, 0.f, 1.f);
	if (Ring.IsValid())
	{
		Ring->SetProgress(Progress);
	}
}

void ULooterProgressRing::Pop()
{
	PopLeft = LooterRing::PopSeconds;
	ApplyPop();
}

bool ULooterProgressRing::Advance(float DeltaSeconds)
{
	if (PopLeft <= 0.f)
	{
		return false;
	}
	PopLeft = FMath::Max(0.f, PopLeft - DeltaSeconds);
	ApplyPop();
	return true;
}

float ULooterProgressRing::GetPopScale() const
{
	return PopLeft > 0.f ? LooterRing::PopScale(1.f - PopLeft / LooterRing::PopSeconds) : 1.f;
}

float ULooterProgressRing::GetFlash() const
{
	return PopLeft > 0.f ? LooterRing::PopFlash(1.f - PopLeft / LooterRing::PopSeconds) : 0.f;
}

void ULooterProgressRing::ApplyPop()
{
	// Scaled about its middle (a widget's pivot starts there).
	SetRenderScale(FVector2D(GetPopScale()));
	if (Ring.IsValid())
	{
		Ring->SetFlash(GetFlash());
	}
}

TSharedRef<SWidget> ULooterProgressRing::RebuildWidget()
{
	Ring = SNew(SLooterProgressRing)
		.Progress(Progress)
		.Flash(GetFlash())
		.FillColor(FillColor)
		.TrackColor(TrackColor)
		.Diameter(Diameter)
		.Thickness(Thickness)
		.Segments(Segments)
		.SegmentGap(SegmentGap);
	return Ring.ToSharedRef();
}

void ULooterProgressRing::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Ring.Reset();
}

// ---------------------------------------------------------------------------
// The hold key cap
// ---------------------------------------------------------------------------

UWidget* LooterUI::MakeHoldKeyCap(UWidgetTree* Tree, ULooterProgressRing*& OutRing, UTextBlock*& OutKey)
{
	UOverlay* Cap = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());

	// A faint dark disc behind the ring, so the key reads over sky and snow; a background, so it fades with the
	// transparency setting like the HUD's other backings.
	FLinearColor DiscColor = Color::Backdrop();
	DiscColor.A *= CapDiscAlpha;
	UImage* Disc = MakeImage(Tree, CircleBrush(DiscColor));
	MarkBackground(Disc);
	FillOverlaySlot(Cap->AddChildToOverlay(Disc), FMargin(CapDiscInset));

	OutRing = Tree->ConstructWidget<ULooterProgressRing>(ULooterProgressRing::StaticClass());
	OutRing->Configure(CapSize, CapThickness);
	FillOverlaySlot(Cap->AddChildToOverlay(OutRing));

	OutKey = MakeFloatingText(Tree, HoldKeyTextSize(1), Color::Accent(), 0, ETextJustify::Center);
	UOverlaySlot* KeySlot = Cap->AddChildToOverlay(OutKey);
	KeySlot->SetHorizontalAlignment(HAlign_Center);
	KeySlot->SetVerticalAlignment(VAlign_Center);

	return MakeSized(Tree, Cap, CapSize, CapSize);
}

FString LooterUI::HoldKeyLabel(const FString& Key)
{
	FString Label = Key.TrimStartAndEnd();
	if (Label.Len() >= 2 && Label.StartsWith(TEXT("[")) && Label.EndsWith(TEXT("]")))
	{
		Label = Label.Mid(1, Label.Len() - 2);
	}
	return Label.ToUpper();
}

int32 LooterUI::HoldKeyTextSize(int32 Characters)
{
	if (Characters <= 1)
	{
		return 14;
	}
	return Characters == 2 ? 11 : Characters == 3 ? 9 : 8;
}

void LooterUI::SetHoldKeyLabel(UTextBlock* KeyText, const FString& Key)
{
	if (!KeyText)
	{
		return;
	}
	const FString Label = HoldKeyLabel(Key);
	if (!KeyText->GetText().ToString().Equals(Label))
	{
		KeyText->SetText(FText::FromString(Label));
		StyleFloatingText(KeyText, HoldKeyTextSize(Label.Len()), Color::Accent(), 0, ETextJustify::Center);
	}
}

void LooterUI::UpdateHoldKeyCap(ULooterProgressRing* Ring, UTextBlock* KeyText, float Progress, float DeltaSeconds)
{
	if (!Ring)
	{
		return;
	}
	const bool bChanging = Ring->Advance(DeltaSeconds);
	// The ring stays full for the pop, as the hold it just finished.
	Ring->SetProgress(Ring->IsPopping() ? 1.f : Progress);
	if (KeyText && bChanging)
	{
		KeyText->SetRenderScale(FVector2D(Ring->GetPopScale()));
		KeyText->SetColorAndOpacity(FSlateColor(FMath::Lerp(Color::Accent(), FLinearColor::White, Ring->GetFlash())));
	}
}
