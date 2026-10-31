#include "UI/Menus/SettingsPageArt.h"
#include "UI/Style/LooterButton.h"
#include "UI/Style/LooterUIStyle.h"
#include "Components/Slider.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/RenderingCommon.h"
#include "Rendering/SlateRenderer.h"
#include "Styling/CoreStyle.h"

// The full-page settings' pieces (E2 "Big Dial", mockups/e.js MODE settings and common.css): the banded rows and their gold
// light, the cut-corner switches, the slider's ruler, the rulers the title and the tabs sit on, the tabs' hanging ticks, the
// key caps and the help card's rule. Painted as vertices and lines; the numbers are the mockup's. This is the painter;
// SettingsPageArtWidget.cpp has it in UMG and the page's small pictures.

using namespace LooterUI;
using namespace SettingsPageArt;

namespace
{
	/** How strong the dark edge under an edged line is, and how much wider (the kit's lines: e.js edged()). */
	constexpr float EdgeStrength = 0.7f;
	constexpr float EdgeExtra = 3.f;
	/** A key cap's corner radius and line (common.css .cap). */
	constexpr float CapRadius = 9.f;
	constexpr float CapLine = 3.5f;

	/** Filled triangles in one draw (bands, washes, fills). */
	struct FPagePainter
	{
		FSlateWindowElementList& Elements;
		int32 Layer;
		const FGeometry& Geometry;
		float Alpha;
		TArray<FSlateVertex> Verts;
		TArray<SlateIndex> Indices;

		FPagePainter(FSlateWindowElementList& InElements, int32 InLayer, const FGeometry& InGeometry, float InAlpha)
			: Elements(InElements), Layer(InLayer), Geometry(InGeometry), Alpha(InAlpha) {}

		SlateIndex Add(const FVector2f& Point, const FLinearColor& Tint)
		{
			FLinearColor Faded = Tint;
			Faded.A *= Alpha;
			Verts.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Geometry.GetAccumulatedRenderTransform(), Point,
				FVector2f(0.5f, 0.5f), Faded.ToFColor(true)));
			return static_cast<SlateIndex>(Verts.Num() - 1);
		}

		/** A convex polygon, each point coloured by its height from Top (y 0) to Bottom (y Height). */
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

	FLinearColor Faded(const FLinearColor& Tint, float Opacity)
	{
		return Tint.CopyWithNewOpacity(Tint.A * Opacity);
	}

	void Line(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, TArray<FVector2f> Points, const FLinearColor& Tint,
		float Alpha, float Width)
	{
		if (Points.Num() < 2)
		{
			return;
		}
		FSlateDrawElement::MakeLines(Elements, Layer, Geometry.ToPaintGeometry(), MoveTemp(Points), ESlateDrawEffect::None,
			Faded(Tint, Alpha), true, Width);
	}

	/** A line on its dark edge: the edge a step wider and under it, as every line in the kit. */
	void EdgedLine(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, const TArray<FVector2f>& Points,
		const FLinearColor& Tint, float Alpha, float Width)
	{
		Line(Elements, Layer, Geometry, Points, Color::Outline(), Alpha * EdgeStrength * Tint.A, Width + EdgeExtra);
		Line(Elements, Layer + 1, Geometry, Points, Tint, Alpha, Width);
	}

	/** A switch's outline: a box cut by Cut at its top-left and bottom-right, Inset in from its edges (e.js cutPts [c, 0, c, 0]). */
	TArray<FVector2f> CutBox(float W, float H, float Cut, float Inset, bool bClosed)
	{
		TArray<FVector2f> Points = {
			{ Inset + Cut, Inset }, { W - Inset, Inset }, { W - Inset, H - Inset - Cut },
			{ W - Inset - Cut, H - Inset }, { Inset, H - Inset }, { Inset, Inset + Cut } };
		if (bClosed)
		{
			const FVector2f First = Points[0];   // a copy: adding an element of the array to itself asserts
			Points.Add(First);
		}
		return Points;
	}

	/** A rounded box's outline, Inset in from its edges (a key cap). */
	TArray<FVector2f> RoundBox(float W, float H, float Radius, float Inset, bool bClosed)
	{
		const float R = FMath::Max(0.f, FMath::Min(Radius, FMath::Min(W, H) * 0.5f - Inset));
		const FVector2f Corners[4] = { { W - Inset - R, Inset + R }, { W - Inset - R, H - Inset - R }, { Inset + R, H - Inset - R }, { Inset + R, Inset + R } };
		TArray<FVector2f> Points;
		constexpr int32 Steps = 6;
		for (int32 Corner = 0; Corner < 4; ++Corner)
		{
			// y down: the top-right corner runs from straight up (-90) to the right (0), then on clockwise.
			const float From = -90.f + Corner * 90.f;
			for (int32 Step = 0; Step <= Steps; ++Step)
			{
				const float Angle = FMath::DegreesToRadians(From + 90.f * Step / Steps);
				Points.Add(Corners[Corner] + FVector2f(FMath::Cos(Angle), FMath::Sin(Angle)) * R);
			}
		}
		if (bClosed)
		{
			const FVector2f First = Points[0];
			Points.Add(First);
		}
		return Points;
	}

	FVector2f Measure(const FString& Text, const FSlateFontInfo& FontInfo)
	{
		if (Text.IsEmpty())
		{
			return FVector2f::ZeroVector;
		}
		if (!FSlateApplication::IsInitialized())
		{
			return FVector2f(Text.Len() * FontInfo.Size * 0.7f, FontInfo.Size * 1.4f);
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
		FSlateDrawElement::MakeText(Elements, Layer, Geometry.ToPaintGeometry(Size, FSlateLayoutTransform(At)), Text, FontInfo,
			ESlateDrawEffect::None, Faded(Tint, Alpha));
	}

	/** A ruler's ticks hanging from the line at LineY, every Step from x 0 (every fifth long). */
	void HangingTicks(FSlateWindowElementList& Elements, int32 Layer, const FGeometry& Geometry, float W, float LineY, float Alpha,
		float LongOpacity, float ShortOpacity)
	{
		int32 Tick = 0;
		for (float X = 0.f; X <= W + 0.1f; X += RulerStep, ++Tick)
		{
			const bool bLong = Tick % 5 == 0;
			Line(Elements, Layer, Geometry, { { X, LineY }, { X, LineY + (bLong ? 12.f : 6.f) } }, Color::SaltLine(),
				Alpha * (bLong ? LongOpacity : ShortOpacity), bLong ? 1.8f : 1.2f);
		}
	}
}

// ---------------------------------------------------------------------------
// The painter
// ---------------------------------------------------------------------------

void SSettingsPageArt::Construct(const FArguments& InArgs)
{
	SetCanTick(false);
	// Some pieces follow a button's or a slider's state as they paint: they paint every frame, whatever caches the rest.
	ForceVolatile(true);
}

void SSettingsPageArt::SetKind(ESettingsPageArt InKind, ESettingsPagePart InPart)
{
	Kind = InKind;
	Part = InPart;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SSettingsPageArt::SetLabel(const FString& InLabel, int32 InFontSize, int32 InLetterSpacing)
{
	Label = InLabel;
	FontSize = InFontSize;
	LetterSpacing = InLetterSpacing;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SSettingsPageArt::SetButton(ULooterButton* InButton)
{
	Button = InButton;
}

void SSettingsPageArt::SetSlider(USlider* InSlider, const FString& InMinLabel, const FString& InMaxLabel, float InKnobWidth)
{
	Slider = InSlider;
	MinLabel = InMinLabel;
	MaxLabel = InMaxLabel;
	KnobWidth = InKnobWidth;
	Invalidate(EInvalidateWidgetReason::Layout);
}

void SSettingsPageArt::SetOdd(bool bInOdd)
{
	bOdd = bInOdd;
	Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D SSettingsPageArt::ComputeDesiredSize(float) const
{
	const FVector2f Text = Measure(Label, Font(FontSize, true, LetterSpacing));
	switch (Kind)
	{
	case ESettingsPageArt::Switch:
		return FVector2D(FMath::Max(SwitchMinWidth, Text.X + SwitchPadX * 2.f), SwitchHeight);
	case ESettingsPageArt::KeyCap:
		return FVector2D(FMath::Max(KeyCapMinWidth, Text.X + KeyCapPadX * 2.f), KeyCapHeight);
	case ESettingsPageArt::SliderTrack:
		return FVector2D(SliderTrackWidth + KnobWidth * 2.f, SliderHeight);
	case ESettingsPageArt::TitleRuler:
		return FVector2D(TitleRulerWidth, TitleRulerHeight);
	case ESettingsPageArt::TabRuler:
		return FVector2D(8.f, TabRulerLineY + 14.f);
	case ESettingsPageArt::HangTick:
		return FVector2D(8.f, TabHangBottom);
	case ESettingsPageArt::Ornament:
		return FVector2D(8.f, OrnamentHeight);
	default:
		return FVector2D(8.f, 8.f);
	}
}

int32 SSettingsPageArt::OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
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
	const bool bHovered = Owner && (Owner->IsHovered() || Owner->IsPressed());
	const bool bFocused = Owner && Owner->IsFocusLit();
	const bool bBack = Part != ESettingsPagePart::Front;
	const bool bFront = Part != ESettingsPagePart::Back;
	const int32 LineLayer = LayerId + 1;
	const int32 TextLayer = LayerId + 4;
	const FLinearColor Gold = Color::SoulGold();
	const FLinearColor Salt = Color::SaltLine();
	FPagePainter Painter(OutDrawElements, LayerId, AllottedGeometry, Alpha);

	switch (Kind)
	{
	case ESettingsPageArt::RowBand:
		if (bBack)
		{
			// e.js: the bands alternate, the lighter glass at .72 and the darker at .55.
			const FLinearColor Band = bOdd ? Faded(Color::SlateGlassFoot(), 0.55f / Color::SlateGlassFoot().A)
				: Faded(Color::SlateGlassTop(), 0.72f / Color::SlateGlassTop().A);
			Painter.Fan({ { 0.f, 0.f }, { W, 0.f }, { W, H }, { 0.f, H } }, Band);
		}
		if (bFront)
		{
			Line(OutDrawElements, LineLayer, AllottedGeometry, { { 0.f, H - 0.75f }, { W, H - 0.75f } }, Salt, Alpha * 0.14f, 1.5f);
		}
		break;

	case ESettingsPageArt::RowLight:
		if (bBack)
		{
			Painter.Across({ 0.f, 0.f }, { W, H }, Faded(Gold, 0.22f), Faded(Gold, 0.03f));
		}
		if (bFront)
		{
			Painter.Fan({ { 0.f, 0.f }, { LitEdgeWidth, 0.f }, { LitEdgeWidth, H }, { 0.f, H } }, Gold);
		}
		break;

	case ESettingsPageArt::Switch:
	{
		const float LineWidth = bChosen ? 3.5f : 2.5f;
		if (bBack)
		{
			// The chosen one holds a gold wash fading down; the rest a faint dark glass, a little gold under the keys' focus.
			const TArray<FVector2f> Fill = CutBox(W, H, SwitchCut, LineWidth * 0.5f, false);
			if (bChosen)
			{
				const float Lift = bHovered || bFocused ? 0.08f : 0.f;
				Painter.Fan(Fill, Faded(Gold, 0.24f + Lift), Faded(Gold, 0.08f + Lift), H);
			}
			else if (bFocused)
			{
				Painter.Fan(Fill, Faded(Gold, 0.12f), Faded(Gold, 0.04f), H);
			}
			else
			{
				Painter.Fan(Fill, Faded(Color::Outline(), (bHovered ? 0.5f : 0.35f) / Color::Outline().A));
			}
		}
		if (bFront)
		{
			const TArray<FVector2f> Outline = CutBox(W, H, SwitchCut, LineWidth * 0.5f, true);
			if (bChosen)
			{
				Line(OutDrawElements, LineLayer, AllottedGeometry, Outline, Gold, Alpha, LineWidth);
			}
			else
			{
				const FLinearColor Edge = bFocused ? Gold : Salt;
				Line(OutDrawElements, LineLayer, AllottedGeometry, Outline, Edge, Alpha * (bFocused ? 0.9f : bHovered ? 0.85f : 0.4f), LineWidth);
			}
			const FLinearColor Word = bChosen ? Gold : (bHovered || bFocused ? Color::SlateText() : Color::SlateLabel());
			Words(OutDrawElements, TextLayer, AllottedGeometry, Label, Font(FontSize, true, LetterSpacing), { W * 0.5f, H * 0.5f }, Word, Alpha);
		}
		break;
	}

	case ESettingsPageArt::SliderTrack:
	{
		if (!bFront)
		{
			break;
		}
		// The knob is the slider's own: SSlider indents its handle, so the knob's middle runs from one knob's width in at
		// the left to one knob's width in at the right.
		const float Y = SliderLineY;
		const float X0 = KnobWidth;
		const float X1 = FMath::Max(X0 + 1.f, W - KnobWidth);
		constexpr int32 Ticks = 40;
		for (int32 Tick = 0; Tick <= Ticks; ++Tick)
		{
			const float X = FMath::Lerp(X0, X1, static_cast<float>(Tick) / Ticks);
			const bool bLong = Tick % 10 == 0;
			Line(OutDrawElements, LineLayer, AllottedGeometry, { { X, Y + 2.f }, { X, Y + 2.f + (bLong ? 12.f : 6.f) } }, Salt,
				Alpha * (bLong ? 0.7f : 0.4f), bLong ? 1.8f : 1.2f);
		}
		Line(OutDrawElements, LineLayer, AllottedGeometry, { { X0, Y }, { X1, Y } }, Salt, Alpha * 0.45f, 2.f);
		if (const USlider* Track = Slider.Get())
		{
			const float Range = Track->GetMaxValue() - Track->GetMinValue();
			const float Share = Range > UE_SMALL_NUMBER ? FMath::Clamp((Track->GetValue() - Track->GetMinValue()) / Range, 0.f, 1.f) : 0.f;
			const float KnobX = FMath::Lerp(X0, X1, Share);
			if (KnobX > X0 + 0.5f)
			{
				Line(OutDrawElements, LineLayer + 1, AllottedGeometry, { { X0, Y }, { KnobX, Y } }, Gold, Alpha, 5.f);
			}
		}
		// The end values under the ruler's ends, kept from reaching more than EndLabelOverhang past them, so the value column
		// beside the ruler always has a clear gap (a wide one, "3.00x", sits in from the end rather than under the value).
		const FSlateFontInfo EndFont = Font(18, false);
		const float MinHalf = Measure(MinLabel, EndFont).X * 0.5f;
		const float MaxHalf = Measure(MaxLabel, EndFont).X * 0.5f;
		Words(OutDrawElements, TextLayer, AllottedGeometry, MinLabel, EndFont, { FMath::Max(X0, X0 - EndLabelOverhang + MinHalf), Y + 29.f },
			Color::SlateLabel(), Alpha);
		Words(OutDrawElements, TextLayer, AllottedGeometry, MaxLabel, EndFont, { FMath::Min(X1, X1 + EndLabelOverhang - MaxHalf), Y + 29.f },
			Color::SlateLabel(), Alpha);
		break;
	}

	case ESettingsPageArt::TitleRuler:
		if (bFront)
		{
			Line(OutDrawElements, LineLayer, AllottedGeometry, { { 0.f, 1.f }, { W, 1.f } }, Salt, Alpha * 0.6f, 1.5f);
			HangingTicks(OutDrawElements, LineLayer, AllottedGeometry, W, 1.f, Alpha, 0.6f, 0.6f);
		}
		break;

	case ESettingsPageArt::TabRuler:
		if (bFront)
		{
			EdgedLine(OutDrawElements, LineLayer, AllottedGeometry, { { 0.f, TabRulerLineY }, { W, TabRulerLineY } }, Salt, Alpha * 0.6f, 1.8f);
			HangingTicks(OutDrawElements, LineLayer + 2, AllottedGeometry, W, TabRulerLineY, Alpha, 0.7f, 0.4f);
		}
		break;

	case ESettingsPageArt::HangTick:
		if (bFront)
		{
			const TArray<FVector2f> Tick = { { W * 0.5f, TabRulerLineY }, { W * 0.5f, TabHangBottom } };
			EdgedLine(OutDrawElements, LineLayer, AllottedGeometry, Tick, bChosen ? Gold : Salt, Alpha * (bChosen ? 1.f : 0.8f), bChosen ? 3.f : 2.f);
		}
		break;

	case ESettingsPageArt::KeyCap:
		if (bBack)
		{
			// A cap that is a button (Back) warms a little under the pointer.
			const FLinearColor Glass = bHovered ? Faded(Gold, 0.16f) : Faded(Color::SlateGlassFoot(), 0.55f / Color::SlateGlassFoot().A);
			Painter.Fan(RoundBox(W, H, CapRadius, CapLine * 0.5f, false), Glass);
		}
		if (bFront)
		{
			Line(OutDrawElements, LineLayer, AllottedGeometry, RoundBox(W, H, CapRadius, CapLine * 0.5f, true), Gold, Alpha, CapLine);
			Words(OutDrawElements, TextLayer, AllottedGeometry, Label, Font(FontSize, true, LetterSpacing), { W * 0.5f, H * 0.5f },
				bHovered ? Color::SlateText() : Gold, Alpha);
		}
		break;

	case ESettingsPageArt::Shade:
		if (bBack)
		{
			// The backdrop's dark in bands across the screen: near solid at the left edge and behind the rows (to about two
			// thirds across), thinning only behind the help card, so a bright sky never fights the words (the mockup's world
			// is dimmed and blurred under its shade; ours is the game as it is).
			struct FStop
			{
				float At;
				float Opacity;
			};
			constexpr FStop Stops[] = { { 0.f, 0.95f }, { 0.4f, 0.93f }, { 0.66f, 0.88f }, { 1.f, 0.8f } };
			const FLinearColor Dark = Color::Backdrop();
			for (int32 Stop = 0; Stop + 1 < static_cast<int32>(UE_ARRAY_COUNT(Stops)); ++Stop)
			{
				Painter.Across({ W * Stops[Stop].At, 0.f }, { W * Stops[Stop + 1].At, H }, Dark.CopyWithNewOpacity(Stops[Stop].Opacity),
					Dark.CopyWithNewOpacity(Stops[Stop + 1].Opacity));
			}
		}
		break;

	case ESettingsPageArt::Ornament:
		if (bFront)
		{
			const float Y = H * 0.5f;
			const float X = W * 0.5f;
			Line(OutDrawElements, LineLayer, AllottedGeometry, { { 0.f, Y }, { X - 16.f, Y } }, Salt, Alpha * 0.5f, 1.5f);
			Line(OutDrawElements, LineLayer, AllottedGeometry, { { X + 16.f, Y }, { W, Y } }, Salt, Alpha * 0.5f, 1.5f);
			Line(OutDrawElements, LineLayer, AllottedGeometry, { { X, Y - 6.f }, { X + 6.f, Y }, { X, Y + 6.f }, { X - 6.f, Y }, { X, Y - 6.f } },
				Gold, Alpha, 2.f);
		}
		break;
	}

	Painter.Flush();
	return TextLayer;
}
