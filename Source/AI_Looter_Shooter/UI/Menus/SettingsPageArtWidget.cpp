// USettingsPageArt (the painter SSettingsPageArt in UMG; SettingsPageArt.cpp paints) and the full page's small pictures:
// the slider's diamond knob, and the two ways the page reuses the pause menu's pictures (an icon in
// another colour, a picture's backgrounds apart from its lines).

#include "UI/Menus/SettingsPageArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/Slider.h"

using namespace LooterUI;
using namespace SettingsPageArt;

namespace
{
	/** Below this luminance a layer is a dark edge or glass, not a line or a light. */
	constexpr float DarkLuminance = 0.08f;
	/** Below this opacity a fill is a wash (a background), not a solid shape. */
	constexpr float WashOpacity = 0.6f;

	TArray<FVector2D> Diamond(const FVector2D& Center, float Radius, bool bClosed)
	{
		TArray<FVector2D> Points = { Center + FVector2D(0.f, -Radius), Center + FVector2D(Radius, 0.f), Center + FVector2D(0.f, Radius),
			Center + FVector2D(-Radius, 0.f) };
		if (bClosed)
		{
			const FVector2D First = Points[0];   // a copy: adding an element of the array to itself asserts
			Points.Add(First);
		}
		return Points;
	}

	TArray<FVector2D> Circle(const FVector2D& Center, float Radius)
	{
		TArray<FVector2D> Points;
		constexpr int32 Sides = 24;
		for (int32 Step = 0; Step < Sides; ++Step)
		{
			const float Angle = UE_TWO_PI * static_cast<float>(Step) / Sides;
			Points.Add(Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
		}
		return Points;
	}

	bool IsDark(const FPaintLayer& Layer)
	{
		return Layer.Color.GetLuminance() < DarkLuminance;
	}

	/** Glass or a wash: fills alone, dark or see-through. */
	bool IsBackground(const FPaintLayer& Layer)
	{
		return Layer.Strokes.IsEmpty() && !Layer.Fills.IsEmpty() && (IsDark(Layer) || Layer.Color.A * Layer.Opacity < WashOpacity);
	}
}

// ---------------------------------------------------------------------------
// In UMG
// ---------------------------------------------------------------------------

USettingsPageArt* USettingsPageArt::Make(UWidgetTree* Tree, ESettingsPageArt InKind, ESettingsPagePart InPart)
{
	if (!Tree)
	{
		return nullptr;
	}
	USettingsPageArt* Piece = Tree->ConstructWidget<USettingsPageArt>(USettingsPageArt::StaticClass());
	Piece->Kind = InKind;
	Piece->Part = InPart;
	Piece->Apply();
	// A row band's lines take the pointer for the whole row, so pointing anywhere on it marks it (the widget it is in reads
	// as hovered); everything else lets it through to what's under it.
	const bool bTakesPointer = InKind == ESettingsPageArt::RowBand && InPart == ESettingsPagePart::Front;
	Piece->SetVisibility(bTakesPointer ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
	if (InPart == ESettingsPagePart::Back)
	{
		MarkBackground(Piece);
	}
	return Piece;
}

TSharedRef<SWidget> USettingsPageArt::RebuildWidget()
{
	Art = SNew(SSettingsPageArt);
	Apply();
	return Art.ToSharedRef();
}

void USettingsPageArt::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Art.Reset();
}

void USettingsPageArt::Apply()
{
	if (!Art.IsValid())
	{
		return;
	}
	Art->SetKind(Kind, Part);
	Art->SetLabel(Label, FontSize, LetterSpacing);
	Art->SetButton(Button.Get());
	Art->SetSlider(Slider.Get(), MinLabel, MaxLabel, KnobWidth);
	Art->SetOdd(bOdd);
}

void USettingsPageArt::SetLabel(const FString& InLabel, int32 InFontSize, int32 InLetterSpacing)
{
	if (Label == InLabel && FontSize == InFontSize && LetterSpacing == InLetterSpacing)
	{
		return;
	}
	Label = InLabel;
	FontSize = InFontSize;
	LetterSpacing = InLetterSpacing;
	Apply();
}

void USettingsPageArt::SetButton(ULooterButton* InButton)
{
	Button = InButton;
	Apply();
}

void USettingsPageArt::SetSlider(USlider* InSlider, const FString& InMinLabel, const FString& InMaxLabel, float InKnobWidth)
{
	Slider = InSlider;
	MinLabel = InMinLabel;
	MaxLabel = InMaxLabel;
	KnobWidth = InKnobWidth;
	Apply();
}

void USettingsPageArt::SetOdd(bool bInOdd)
{
	bOdd = bInOdd;
	Apply();
}

UOverlay* SettingsPageArt::MakeLayered(UWidgetTree* Tree, ESettingsPageArt Kind, ULooterButton* Owner, USettingsPageArt*& OutBack,
	USettingsPageArt*& OutFront)
{
	UOverlay* Layers = Tree->ConstructWidget<UOverlay>(UOverlay::StaticClass());
	OutBack = USettingsPageArt::Make(Tree, Kind, ESettingsPagePart::Back);
	OutFront = USettingsPageArt::Make(Tree, Kind, ESettingsPagePart::Front);
	if (Owner)
	{
		OutBack->SetButton(Owner);
		OutFront->SetButton(Owner);
	}
	FillOverlaySlot(Layers->AddChildToOverlay(OutBack));
	FillOverlaySlot(Layers->AddChildToOverlay(OutFront));
	return Layers;
}

UWidget* SettingsPageArt::MakeKeyCap(UWidgetTree* Tree, const FString& Label, ULooterButton* Owner, USettingsPageArt*& OutFront)
{
	USettingsPageArt* Back = nullptr;
	UOverlay* Cap = MakeLayered(Tree, ESettingsPageArt::KeyCap, Owner, Back, OutFront);
	OutFront->SetLabel(Label.ToUpper(), KeyCapFontSize, KeyCapSpacing);
	Cap->SetVisibility(ESlateVisibility::HitTestInvisible);
	return Cap;
}

// ---------------------------------------------------------------------------
// The pictures
// ---------------------------------------------------------------------------

FPaintedIcon SettingsPageArt::Knob()
{
	// e.js: a diamond 15 from its middle, glass inside a 3.5 px gold line, a gold dot of 3.5; the kit's dark edge under it.
	FPaintedIcon Icon;
	Icon.ViewBox = FVector2D(KnobSize, KnobSize);
	const FVector2D Center(KnobSize * 0.5f, KnobSize * 0.5f);
	constexpr float Radius = 15.f;
	FPaintLayer& Edge = Icon.Layers.AddDefaulted_GetRef();
	Edge.Strokes.Add(Diamond(Center, Radius, true));
	Edge.StrokeWidth = 3.5f + 3.f;
	Edge.Color = Color::Outline();
	Edge.Opacity = 0.75f;
	FPaintLayer& Glass = Icon.Layers.AddDefaulted_GetRef();
	Glass.Fills.Add(Diamond(Center, Radius, false));
	Glass.Color = Color::SlateGlassFoot();
	FPaintLayer& Line = Icon.Layers.AddDefaulted_GetRef();
	Line.Strokes.Add(Diamond(Center, Radius, true));
	Line.StrokeWidth = 3.5f;
	Line.Color = Color::SoulGold();
	FPaintLayer& Dot = Icon.Layers.AddDefaulted_GetRef();
	Dot.Fills.Add(Circle(Center, 3.5f));
	Dot.Color = Color::SoulGold();
	return Icon;
}

FPaintedIcon SettingsPageArt::Recoloured(const FPaintedIcon& Icon, const FLinearColor& Tint)
{
	FPaintedIcon Out = Icon;
	for (FPaintLayer& Layer : Out.Layers)
	{
		if (IsDark(Layer))
		{
			continue;
		}
		Layer.Color = Tint.CopyWithNewOpacity(Tint.A * Layer.Color.A);
		Layer.GradientTo.Reset();
	}
	return Out;
}

FPaintedIcon SettingsPageArt::PictureLayers(const FPaintedIcon& Picture, bool bBack)
{
	FPaintedIcon Out;
	Out.ViewBox = Picture.ViewBox;
	for (const FPaintLayer& Layer : Picture.Layers)
	{
		if (IsBackground(Layer) == bBack)
		{
			Out.Layers.Add(Layer);
		}
	}
	return Out;
}
