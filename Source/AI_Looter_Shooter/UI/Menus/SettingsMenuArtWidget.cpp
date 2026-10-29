// USettingsMenuArt (the painter SSettingsMenuArt in UMG; SettingsMenuArt.cpp paints) and the settings menu's own small
// pictures and words: the halo's gold arcs, the slider's ring knob, the plate's serial and its stamped words.

#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Slider.h"

using namespace LooterUI;
using namespace SettingsMenuArt;

namespace
{
	/** A circle's points, for the painted pictures (closed for a stroke). */
	TArray<FVector2D> CirclePoints(const FVector2D& Center, float Radius, bool bClosed)
	{
		TArray<FVector2D> Points;
		constexpr int32 Sides = 48;
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const float Angle = UE_TWO_PI * static_cast<float>(Step) / Sides;
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		if (bClosed)
		{
			const FVector2D First = Points[0];   // a copy: adding an element of the array to itself asserts
			Points.Add(First);
		}
		return Points;
	}

	/** An arc's points from FromDeg to ToDeg (y down, so -90 is straight up), every two degrees. */
	TArray<FVector2D> ArcPoints(const FVector2D& Center, float Radius, float FromDeg, float ToDeg)
	{
		TArray<FVector2D> Points;
		const int32 Steps = FMath::Max(2, FMath::CeilToInt(FMath::Abs(ToDeg - FromDeg) / 2.f));
		for (int32 Step = 0; Step <= Steps; ++Step)
		{
			const float Angle = FMath::DegreesToRadians(FMath::Lerp(FromDeg, ToDeg, static_cast<float>(Step) / Steps));
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	}
}

// ---------------------------------------------------------------------------
// In UMG
// ---------------------------------------------------------------------------

USettingsMenuArt* USettingsMenuArt::Make(UWidgetTree* Tree, ESettingsArt InKind)
{
	if (!Tree)
	{
		return nullptr;
	}
	USettingsMenuArt* Piece = Tree->ConstructWidget<USettingsMenuArt>(USettingsMenuArt::StaticClass());
	Piece->SetKind(InKind);
	Piece->SetVisibility(InKind == ESettingsArt::LedgerLine ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
	return Piece;
}

TSharedRef<SWidget> USettingsMenuArt::RebuildWidget()
{
	Art = SNew(SSettingsMenuArt);
	Apply();
	return Art.ToSharedRef();
}

void USettingsMenuArt::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Art.Reset();
}

void USettingsMenuArt::Apply()
{
	if (!Art.IsValid())
	{
		return;
	}
	Art->SetKind(Kind);
	Art->SetLabel(Label, FontSize, LetterSpacing);
	Art->SetButton(Button.Get());
	Art->SetSlider(Slider.Get(), MinLabel, MaxLabel, KnobWidth);
	Art->SetMarginX(MarginX);
}

void USettingsMenuArt::SetKind(ESettingsArt InKind)
{
	Kind = InKind;
	Apply();
}

void USettingsMenuArt::SetLabel(const FString& InLabel, int32 InFontSize, int32 InLetterSpacing)
{
	Label = InLabel;
	FontSize = InFontSize;
	LetterSpacing = InLetterSpacing;
	Apply();
}

void USettingsMenuArt::SetButton(ULooterButton* InButton)
{
	Button = InButton;
	Apply();
}

void USettingsMenuArt::SetSlider(USlider* InSlider, const FString& InMinLabel, const FString& InMaxLabel, float InKnobWidth)
{
	Slider = InSlider;
	MinLabel = InMinLabel;
	MaxLabel = InMaxLabel;
	KnobWidth = InKnobWidth;
	Apply();
}

void USettingsMenuArt::SetMarginX(float InMarginX)
{
	MarginX = InMarginX;
	Apply();
}

// ---------------------------------------------------------------------------
// The pictures and words
// ---------------------------------------------------------------------------

FPaintedIcon SettingsMenuArt::HaloArcs(float Radius)
{
	FPaintedIcon Icon;
	const float Half = Radius + 8.f;
	Icon.ViewBox = FVector2D(Half * 2.f, Half * 2.f);
	const FVector2D Center(Half, Half);
	const TArray<TArray<FVector2D>> Arcs = { ArcPoints(Center, Radius, -60.f, 0.f), ArcPoints(Center, Radius, 120.f, 180.f) };
	FPaintLayer& Edge = Icon.Layers.AddDefaulted_GetRef();
	Edge.Strokes = Arcs;
	Edge.StrokeWidth = 4.5f + EdgeExtra;
	Edge.Color = Color::Outline();
	Edge.Opacity = EdgeStrength;
	FPaintLayer& Gold = Icon.Layers.AddDefaulted_GetRef();
	Gold.Strokes = Arcs;
	Gold.StrokeWidth = 4.5f;
	Gold.Color = Color::SoulGold();
	return Icon;
}

FPaintedIcon SettingsMenuArt::Knob()
{
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(KnobSize, KnobSize);
	const FVector2D Center(KnobSize * 0.5f, KnobSize * 0.5f);
	FPaintLayer& Edge = Icon.Layers.AddDefaulted_GetRef();
	Edge.Fills.Add(CirclePoints(Center, KnobSize * 0.5f - 0.5f, false));
	Edge.Color = Color::Outline();
	FPaintLayer& Inside = Icon.Layers.AddDefaulted_GetRef();
	Inside.Fills.Add(CirclePoints(Center, KnobSize * 0.5f - 2.5f, false));
	Inside.Color = Color::SlateGem();
	FPaintLayer& Ring = Icon.Layers.AddDefaulted_GetRef();
	Ring.Strokes.Add(CirclePoints(Center, KnobSize * 0.5f - 3.5f, true));
	Ring.StrokeWidth = 3.f;
	Ring.Color = Color::SoulGold();
	FPaintLayer& Dot = Icon.Layers.AddDefaulted_GetRef();
	Dot.Fills.Add(CirclePoints(Center, 2.6f, false));
	Dot.Color = Color::SoulGold();
	return Icon;
}

FString SettingsMenuArt::SerialText(int32 Session, int32 Level)
{
	return FString::Printf(TEXT("SER. RV-%04d-%02d"), FMath::Max(0, Session), FMath::Clamp(Level, 0, 99));
}

FString SettingsMenuArt::PlateText(int32 Session, const FString& Area)
{
	const FString Place = Area.ToUpper();
	if (Session <= 0)
	{
		return Place;
	}
	return Place.IsEmpty() ? FString::Printf(TEXT("NO. %02d"), Session) : FString::Printf(TEXT("NO. %02d · %s"), Session, *Place);
}
