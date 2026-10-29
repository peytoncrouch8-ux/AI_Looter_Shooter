#include "UI/Menus/SettingsMenuArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Slider.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

// The settings menu's look, round 3 (the user's mix of concept 1 "Ledger & Brass" and concept 3 "Halo Reticle",
// Docs/Work/2026-10-29-UI3Build/CONTRACT.md; sizes from Docs/Work/2026-10-29-Morning/ui3/make_a.js, c1Pause and c3Pause).
// Painted as vertices and lines, so every piece is crisp at any size and needs no texture; every line sits on a dark edge
// as the kit's lines do, so it reads over the game behind the glass. This is the painter; SettingsMenuArtWidget.cpp has it
// in UMG and the menu's small pictures.

using namespace LooterUI;
using namespace SettingsMenuArt;

namespace
{
	/** Filled triangles in one draw (the plate's glass, the ribbon, the washes). */
	struct FArtPainter
	{
		FSlateWindowElementList& Elements;
		int32 Layer;
		const FGeometry& Geometry;
		float Alpha;
		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;

		FArtPainter(FSlateWindowElementList& InElements, int32 InLayer, const FGeometry& InGeometry, float InAlpha)
			: Elements(InElements), Layer(InLayer), Geometry(InGeometry), Alpha(InAlpha) {}

		SlateIndex Add(const FVector2f& Point, const FLinearColor& Tint)
		{
			FLinearColor Faded = Tint;
			Faded.A *= Alpha;
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Geometry.GetAccumulatedRenderTransform(), Point,
				FVector2f(0.5f, 0.5f), Faded.ToFColor(true)));
			return static_cast<SlateIndex>(Verts.Num() - 1);
		}

		/** A polygon every point of which the first can see (convex, or a star round it), each point coloured by its height. */
		void Fan(const TArray<FVector2f>& Points, const FLinearColor& Top, const FLinearColor& Bottom, float Height)
		{
			if (Points.Num() < 3)
			{
				return;
			}
			auto At = [&](const FVector2f& Point)
			{
				return FMath::Lerp(Top, Bottom, FMath::Clamp(Point.Y / FMath::Max(Height, 1.f), 0.f, 1.f));
			};
			const SlateIndex First = Add(Points[0], At(Points[0]));
			SlateIndex Last = Add(Points[1], At(Points[1]));
			for (int32 Index = 2; Index < Points.Num(); ++Index)
			{
				const SlateIndex Next = Add(Points[Index], At(Points[Index]));
				Indices.Append({ First, Last, Next });
				Last = Next;
			}
		}

		void Fan(const TArray<FVector2f>& Points, const FLinearColor& Tint)
		{
			Fan(Points, Tint, Tint, 1.f);
		}

		/** A box running from Left's colour at its left to Right's at its right. */
		void Across(const FVector2f& TopLeft, const FVector2f& BottomRight, const FLinearColor& Left, const FLinearColor& Right)
		{
			const SlateIndex A = Add(TopLeft, Left);
			const SlateIndex B = Add(FVector2f(BottomRight.X, TopLeft.Y), Right);
			const SlateIndex C = Add(BottomRight, Right);
			const SlateIndex D = Add(FVector2f(TopLeft.X, BottomRight.Y), Left);
			Indices.Append({ A, B, C, A, C, D });
		}

		/** A round dot (a rivet's parts). */
		void Dot(const FVector2f& Center, float Radius, const FLinearColor& Tint)
		{
			TArray<FVector2f> Points;
			constexpr int32 Sides = 16;
			for (int32 Step = 0; Step < Sides; ++Step)
			{
				const float Angle = UE_TWO_PI * static_cast<float>(Step) / Sides;
				Points.Add(Center + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * Radius);
			}
			Fan(Points, Tint);
		}

		void Flush()
		{
			if (Verts.Num() == 0 || !FSlateApplication::IsInitialized())
			{
				return;
			}
			const FSlateBrush* White = FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"));
			const FSlateResourceHandle Handle = FSlateApplication::Get().GetRenderer()->GetResourceHandle(*White);
			FSlateDrawElement::MakeCustomVerts(Elements, Layer, Handle, Verts, Indices, nullptr, 0, 0);
			Verts.Reset();
			Indices.Reset();
		}
	};

	void Line(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, TArray<FVector2f> Points, const FLinearColor& Tint,
		float Alpha, float Width)
	{
		if (Points.Num() < 2)
		{
			return;
		}
		FLinearColor Faded = Tint;
		Faded.A *= Alpha;
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None, Faded, true, Width);
	}

	/** A line on its dark edge: the edge a step wider and under it, as every line in the kit. */
	void EdgedLine(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, const TArray<FVector2f>& Points,
		const FLinearColor& Tint, float Alpha, float Width)
	{
		Line(Elements, Layer, Geometry, Points, Color::Outline() * FLinearColor(1.f, 1.f, 1.f, EdgeStrength), Alpha, Width + EdgeExtra);
		Line(Elements, Layer + 1, Geometry, Points, Tint, Alpha, Width);
	}

	/** A box's outline with every corner cut by Cut, clockwise from the top-left, closed (its first point again at the end). */
	TArray<FVector2f> Chamfer(float X, float Y, float W, float H, float Cut, bool bClosed = true)
	{
		TArray<FVector2f> Points = {
			{ X + Cut, Y }, { X + W - Cut, Y }, { X + W, Y + Cut }, { X + W, Y + H - Cut },
			{ X + W - Cut, Y + H }, { X + Cut, Y + H }, { X, Y + H - Cut }, { X, Y + Cut } };
		if (bClosed)
		{
			const auto First = Points[0];   // a copy: adding an element of the array to itself asserts
			Points.Add(First);
		}
		return Points;
	}

	TArray<FVector2f> Box(float X, float Y, float W, float H)
	{
		return { { X, Y }, { X + W, Y }, { X + W, Y + H }, { X, Y + H }, { X, Y } };
	}

	FSlateFontInfo ArtFont(int32 Size, int32 Spacing)
	{
		return Font(Size, true, Spacing);
	}

	FVector2f Measure(const FString& Text, const FSlateFontInfo& FontInfo)
	{
		if (Text.IsEmpty())
		{
			return FVector2f::ZeroVector;
		}
		if (!FSlateApplication::IsInitialized())
		{
			return FVector2f(Text.Len() * FontInfo.Size * 0.8f, FontInfo.Size * 1.4f);
		}
		return FVector2f(FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Text, FontInfo));
	}

	/** Words centred on Center. */
	void Words(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, const FString& Text, const FSlateFontInfo& FontInfo,
		const FVector2f& Center, const FLinearColor& Tint, float Alpha)
	{
		if (Text.IsEmpty())
		{
			return;
		}
		const FVector2f Size = Measure(Text, FontInfo);
		// Letter spacing adds room after the last letter too: half of it back keeps the word in the middle.
		const float Trail = FontInfo.LetterSpacing * FontInfo.Size * 0.001f * 0.5f;
		const FVector2f At = Center - Size * 0.5f + FVector2f(Trail, 0.f);
		FLinearColor Faded = Tint;
		Faded.A *= Alpha;
		FSlateDrawElement::MakeText(Elements, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(At)), Text, FontInfo,
			ESlateDrawEffect::None, Faded);
	}

	/** Save & Quit's red: the kit's warm red (the mockup's #ff7a6b), bright enough to read as a line on the dark glass. */
	FLinearColor StampRed()
	{
		return Color::Worse();
	}
}

// ---------------------------------------------------------------------------
// The painter
// ---------------------------------------------------------------------------

void SSettingsMenuArt::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
	// Some pieces follow a button's or a slider's state as they paint: they paint every frame, whatever caches the rest.
	ForceVolatile(true);
}

void SSettingsMenuArt::SetKind(ESettingsArt InKind)
{
	Kind = InKind;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SSettingsMenuArt::SetLabel(const FString& InLabel, int32 InFontSize, int32 InLetterSpacing)
{
	Label = InLabel;
	FontSize = InFontSize;
	LetterSpacing = InLetterSpacing;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SSettingsMenuArt::SetButton(ULooterButton* InButton)
{
	Button = InButton;
}

void SSettingsMenuArt::SetSlider(USlider* InSlider, const FString& InMinLabel, const FString& InMaxLabel, float InKnobWidth)
{
	Slider = InSlider;
	MinLabel = InMinLabel;
	MaxLabel = InMaxLabel;
	KnobWidth = InKnobWidth;
	Invalidate(EInvalidateWidgetReason::Paint);
}

void SSettingsMenuArt::SetMarginX(float InMarginX)
{
	MarginX = InMarginX;
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SSettingsMenuArt::ComputeDesiredSize(float) const
{
	const FVector2f Text = Measure(Label, ArtFont(FontSize, LetterSpacing));
	switch (Kind)
	{
	case ESettingsArt::Chip:
		return FVector2D(FMath::Max(ChipMinWidth, Text.X + ChipPadX * 2.f), ChipHeight);
	case ESettingsArt::Tab:
		return FVector2D(Text.X + 20.f, 34.f);
	case ESettingsArt::Stamp:
		return FVector2D(Text.X + 72.f, 84.f);
	case ESettingsArt::AreaPlate:
		return FVector2D(Text.X + 28.f, 34.f);
	case ESettingsArt::Ribbon:
		return FVector2D(RibbonWidth, RibbonHeight);
	case ESettingsArt::SliderRuler:
		return FVector2D(190.f, 46.f);
	case ESettingsArt::TabMark:
		return FVector2D(TabMarkWidth, TabMarkHeight);
	default:
		return FVector2D(8.f, 8.f);
	}
}

int32 SSettingsMenuArt::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const
{
	const FVector2f Size = FVector2f(AllottedGeometry.GetLocalSize());
	const float W = Size.X;
	const float H = Size.Y;
	if (W < 2.f || H < 2.f)
	{
		return LayerId;
	}
	const ULooterButton* Owner = Button.Get();
	const bool bEnabled = !Owner || Owner->GetIsEnabled();
	const float Alpha = InWidgetStyle.GetColorAndOpacityTint().A * (bEnabled ? 1.f : 0.5f);
	const bool bChosen = Owner && Owner->IsHighlighted();
	const bool bHovered = Owner && Owner->IsHovered();
	const bool bFocused = Owner && Owner->IsFocusLit();
	const bool bLit = bHovered || bFocused || (Owner && Owner->IsPressed());
	const FSlateFontInfo LabelFont = ArtFont(FontSize, LetterSpacing);
	const FVector2f Middle(W * 0.5f, H * 0.5f);
	const int32 FillLayer = LayerId;
	const int32 LineLayer = LayerId + 1;
	const int32 TextLayer = LayerId + 3;
	FArtPainter Painter(OutDrawElements, FillLayer, AllottedGeometry, Alpha);

	switch (Kind)
	{
	case ESettingsArt::PlateFill:
		Painter.Fan(Chamfer(0.f, 0.f, W, H, PlateCut, false), Color::SlateGlassTop(), Color::SlateGlassFoot(), H);
		break;

	case ESettingsArt::PlateLines:
	{
		// c1's plate with c3's cut corners: the salt line on its edge, a fainter one inside it (the double trim), a rivet in
		// each corner.
		EdgedLine(OutDrawElements, LineLayer, AllottedGeometry, Chamfer(1.5f, 1.5f, W - 3.f, H - 3.f, PlateCut), Color::SaltLine(), Alpha, 3.f);
		Line(OutDrawElements, LineLayer + 1, AllottedGeometry, Chamfer(TrimInset, TrimInset, W - TrimInset * 2.f, H - TrimInset * 2.f, PlateCut - 6.f),
			Color::SaltLine(), Alpha * 0.4f, 1.4f);
		for (const FVector2f& Corner : { FVector2f(RivetInset, RivetInset), FVector2f(W - RivetInset, RivetInset),
			FVector2f(RivetInset, H - RivetInset), FVector2f(W - RivetInset, H - RivetInset) })
		{
			Painter.Dot(Corner, 7.f, Color::Outline());
			Painter.Dot(Corner, 5.5f, Color::SlateIcon());
			Painter.Dot(Corner + FVector2f(-1.4f, -1.4f), 2.4f, Color::SlateText() * FLinearColor(1.f, 1.f, 1.f, 0.6f));
		}
		break;
	}

	case ESettingsArt::Chip:
	{
		// c3's choice: hollow salt, the chosen one gold with a faint gold inside and a tick above and below its middle.
		const TArray<FVector2f> Outline = Chamfer(1.5f, 1.5f, W - 3.f, H - 3.f, ChipCut);
		if (bChosen)
		{
			Painter.Fan(Chamfer(1.5f, 1.5f, W - 3.f, H - 3.f, ChipCut, false), Color::SoulGold() * FLinearColor(1.f, 1.f, 1.f, bLit ? 0.22f : 0.14f));
			EdgedLine(OutDrawElements, LineLayer, AllottedGeometry, Outline, Color::SoulGold(), Alpha, 3.f);
			Line(OutDrawElements, LineLayer + 1, AllottedGeometry, { { W * 0.5f, -7.f }, { W * 0.5f, 0.f } }, Color::SoulGold(), Alpha, 2.5f);
			Line(OutDrawElements, LineLayer + 1, AllottedGeometry, { { W * 0.5f, H }, { W * 0.5f, H + 7.f } }, Color::SoulGold(), Alpha, 2.5f);
		}
		else
		{
			if (bFocused)
			{
				Painter.Fan(Chamfer(1.5f, 1.5f, W - 3.f, H - 3.f, ChipCut, false), Color::SoulGold() * FLinearColor(1.f, 1.f, 1.f, 0.08f));
			}
			const FLinearColor Edge = bFocused ? Color::SoulGold() : Color::SaltLine();
			Line(OutDrawElements, LineLayer, AllottedGeometry, Outline, Edge, Alpha * (bLit ? 0.8f : 0.32f), bFocused ? 2.f : 1.6f);
		}
		const FLinearColor Word = bChosen ? Color::SoulGold() : (bLit ? Color::SlateText() : Color::SlateLabel());
		Words(OutDrawElements, TextLayer, AllottedGeometry, Label, LabelFont, Middle, Word, Alpha);
		break;
	}

	case ESettingsArt::Tab:
	{
		const FLinearColor Word = bChosen ? Color::SoulGold() : (bLit ? Color::SlateText() : Color::SlateLabel());
		Words(OutDrawElements, TextLayer, AllottedGeometry, Label, LabelFont, Middle, Word, Alpha);
		break;
	}

	case ESettingsArt::StampWash:
		Painter.Fan(Box(3.f, 3.f, W - 6.f, H - 6.f), Color::Danger() * FLinearColor(1.f, 1.f, 1.f, bLit ? 0.6f : 0.38f));
		break;

	case ESettingsArt::Stamp:
	{
		// c1's rubber stamp: a heavy red line, a thin one inside it, the red spaced word (the button tilts it).
		EdgedLine(OutDrawElements, LineLayer, AllottedGeometry, Box(3.f, 3.f, W - 6.f, H - 6.f), StampRed(), Alpha, bLit ? 4.5f : 3.5f);
		Line(OutDrawElements, LineLayer + 1, AllottedGeometry, Box(10.f, 10.f, W - 20.f, H - 20.f), StampRed(), Alpha * 0.8f, 1.3f);
		Words(OutDrawElements, TextLayer, AllottedGeometry, Label, LabelFont, Middle, bLit ? Color::SlateText() : StampRed(), Alpha);
		break;
	}

	case ESettingsArt::Ribbon:
	{
		// c1's swallowtail in brass gold, its notched tails at the left, its point at the right, a brass rivet near the point.
		const float Notch = 14.f;
		const float Point = 10.f;
		const TArray<FVector2f> Shape = { { Notch, H * 0.5f }, { 0.f, 0.f }, { W - Point, 0.f }, { W, H * 0.5f }, { W - Point, H }, { 0.f, H } };
		Painter.Fan(Shape, Color::SoulGold());
		TArray<FVector2f> Outline = Shape;
		Outline.Add(Shape[0]);
		Line(OutDrawElements, LineLayer, AllottedGeometry, Outline, Color::Outline(), Alpha * 0.6f, 1.5f);
		Line(OutDrawElements, LineLayer, AllottedGeometry, { { 4.f, 4.f }, { W - Point - 2.f, 4.f } }, Color::SlateText(), Alpha * 0.7f, 1.f);
		const FVector2f Rivet(W - Point - 8.f, H * 0.5f);
		Painter.Dot(Rivet, 5.f, Color::Outline());
		Painter.Dot(Rivet, 3.6f, Color::SlateFlavor());
		Painter.Dot(Rivet + FVector2f(-1.f, -1.f), 1.4f, Color::SlateText());
		break;
	}

	case ESettingsArt::LedgerLine:
		Line(OutDrawElements, LineLayer, AllottedGeometry, { { 0.f, H - 0.6f }, { W, H - 0.6f } }, Color::SaltLine(), Alpha * 0.15f, 1.2f);
		break;

	case ESettingsArt::MarginRule:
		Line(OutDrawElements, LineLayer, AllottedGeometry, { { MarginX, 0.f }, { MarginX, H } }, Color::SoulGold(), Alpha * 0.5f, 1.6f);
		Line(OutDrawElements, LineLayer, AllottedGeometry, { { MarginX + 5.f, 0.f }, { MarginX + 5.f, H } }, Color::SoulGold(), Alpha * 0.3f, 0.8f);
		break;

	case ESettingsArt::RowWash:
	{
		const float Left = MarginX + 7.f;
		const float Fade = FMath::Lerp(Left, W, 0.85f);
		const FLinearColor Gold = Color::SoulGold();
		Painter.Across({ Left, 1.f }, { Fade, H - 1.f }, Gold * FLinearColor(1.f, 1.f, 1.f, 0.24f), Gold * FLinearColor(1.f, 1.f, 1.f, 0.02f));
		break;
	}

	case ESettingsArt::RowRule:
	{
		const float Left = MarginX + 14.f;
		const float Right = W - 10.f;
		Line(OutDrawElements, LineLayer, AllottedGeometry, { { Left, H - 9.f }, { Right, H - 9.f } }, Color::SoulGold(), Alpha, 1.2f);
		Line(OutDrawElements, LineLayer, AllottedGeometry, { { Left, H - 4.f }, { Right, H - 4.f } }, Color::SoulGold(), Alpha, 2.6f);
		break;
	}

	case ESettingsArt::ResumeWash:
	{
		// c3's gold wash between the brackets, strong at the left, fading to nothing at the right bracket (a wash cut off
		// short of it read as a box).
		const float Left = 16.f;
		const FLinearColor Gold = Color::SoulGold();
		Painter.Across({ Left, 4.f }, { W - 16.f, H - 4.f }, Gold * FLinearColor(1.f, 1.f, 1.f, bLit ? 0.38f : 0.24f), Gold * FLinearColor(1.f, 1.f, 1.f, 0.f));
		break;
	}

	case ESettingsArt::SliderRuler:
	{
		// c3's range scale: a salt line, ticks up from it (every fifth long), the gold run out to the knob, the ends' values
		// under the ends. The knob is the slider's own: SSlider indents its handle, so the knob's middle runs from one
		// knob's width in at the left to one knob's width in at the right.
		const float Y = H * 0.5f;
		const float X0 = KnobWidth;
		const float X1 = FMath::Max(X0 + 1.f, W - KnobWidth);
		Line(OutDrawElements, LineLayer, AllottedGeometry, { { X0, Y }, { X1, Y } }, Color::SaltLine(), Alpha * 0.55f, 1.6f);
		constexpr int32 Ticks = 20;
		for (int32 Tick = 0; Tick <= Ticks; ++Tick)
		{
			const float X = FMath::Lerp(X0, X1, static_cast<float>(Tick) / Ticks);
			const bool bLong = Tick % 5 == 0;
			Line(OutDrawElements, LineLayer, AllottedGeometry, { { X, Y }, { X, Y - (bLong ? 11.f : 5.f) } }, Color::SaltLine(),
				Alpha * (bLong ? 0.85f : 0.45f), bLong ? 2.f : 1.2f);
		}
		if (const USlider* Track = Slider.Get())
		{
			const float Range = Track->GetMaxValue() - Track->GetMinValue();
			const float Share = Range > UE_SMALL_NUMBER ? FMath::Clamp((Track->GetValue() - Track->GetMinValue()) / Range, 0.f, 1.f) : 0.f;
			const float KnobX = FMath::Lerp(X0, X1, Share);
			if (KnobX > X0 + 0.5f)
			{
				EdgedLine(OutDrawElements, LineLayer + 2, AllottedGeometry, { { X0, Y }, { KnobX, Y } }, Color::SoulGold(), Alpha, 4.f);
			}
		}
		const FSlateFontInfo EndFont = ArtFont(12, 0);
		Words(OutDrawElements, TextLayer, AllottedGeometry, MinLabel, EndFont, { X0, Y + 21.f }, Color::SlateLabel(), Alpha);
		Words(OutDrawElements, TextLayer, AllottedGeometry, MaxLabel, EndFont, { X1, Y + 21.f }, Color::SlateLabel(), Alpha);
		break;
	}

	case ESettingsArt::AreaPlate:
		Line(OutDrawElements, LineLayer, AllottedGeometry, Box(0.8f, 0.8f, W - 1.6f, H - 1.6f), Color::SoulGold(), Alpha * 0.8f, 1.6f);
		Line(OutDrawElements, LineLayer, AllottedGeometry, Box(4.5f, 4.5f, W - 9.f, H - 9.f), Color::SoulGold(), Alpha * 0.45f, 0.8f);
		Words(OutDrawElements, TextLayer, AllottedGeometry, Label, LabelFont, Middle, Color::SoulGold(), Alpha);
		break;

	case ESettingsArt::ColumnRuler:
	{
		Line(OutDrawElements, LineLayer, AllottedGeometry, { { 1.f, 6.f }, { 1.f, H - 6.f } }, Color::SaltLine(), Alpha * 0.45f, 1.6f);
		int32 Tick = 0;
		for (float Y = 6.f; Y <= H - 6.f; Y += 12.f, ++Tick)
		{
			Line(OutDrawElements, LineLayer, AllottedGeometry, { { 1.f, Y }, { Tick % 4 == 0 ? 11.f : 6.f, Y } }, Color::SaltLine(), Alpha * 0.5f, 1.2f);
		}
		break;
	}

	case ESettingsArt::TabMark:
		EdgedLine(OutDrawElements, LineLayer, AllottedGeometry, { { 0.f, 2.f }, { W, 2.f } }, Color::SoulGold(), Alpha, 4.f);
		Painter.Fan({ { W * 0.5f, 6.f }, { W * 0.5f + 9.f, 18.f }, { W * 0.5f - 9.f, 18.f } }, Color::SoulGold());
		break;
	}

	Painter.Flush();
	return TextLayer;
}
